#include "PythonImportRules.h"
#include "PythonImportRuleControl.h"
#include "PythonImportChecks.h"
#include "PythonImportNumeric.h"
#include "PythonImportNumericNext.h"
#include "PythonImportObservations.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"

#include <tuple>

using namespace mlir;

namespace acir::compiler::detail {
namespace {

void visitImmediateAstChildren(
    const AstNode &node, llvm::function_ref<void(const AstNode &)> visitor) {
  for (NamedAttribute field : node.fields()) {
    if (auto child = dyn_cast<DictionaryAttr>(field.getValue());
        child && child.getAs<StringAttr>("kind")) {
      visitor(node.child(field.getName()));
      continue;
    }
    if (auto array = dyn_cast<ArrayAttr>(field.getValue()))
      for (size_t index = 0; index < array.size(); ++index) {
        AstNode childNode = node.item(field.getName(), index);
        if (!childNode)
          continue;
        visitor(childNode);
      }
  }
}

void collectRuleTargetReads(const AstNode &node, RuleReadCallback formalRead,
                            RuleReadCallback memberRead) {
  if (node.kind() == "Tuple" || node.kind() == "List") {
    ArrayAttr elements = node.array("elts");
    for (size_t index = 0; index < elements.size(); ++index)
      collectRuleTargetReads(node.item("elts", index), formalRead, memberRead);
    return;
  }
  if (node.kind() == "Starred") {
    collectRuleTargetReads(node.child("value"), formalRead, memberRead);
    return;
  }
  if (node.kind() == "Subscript") {
    collectRuleTargetReads(node.child("value"), formalRead, memberRead);
    collectRuleExpressionReads(node.child("slice"), formalRead, memberRead);
    return;
  }
  if (node.kind() == "Attribute") {
    collectRuleTargetReads(node.child("value"), formalRead, memberRead);
    return;
  }
  if (node.kind() != "Name")
    collectRuleExpressionReads(node, formalRead, memberRead);
}

} // namespace

void collectRuleExpressionReads(const AstNode &node,
                                RuleReadCallback formalRead,
                                RuleReadCallback memberRead) {
  if (node.child("ctx").kind() == "Store") {
    collectRuleTargetReads(node, formalRead, memberRead);
    return;
  }
  if ((node.kind() == "Attribute" || node.kind() == "Name") &&
      node.child("ctx").kind() != "Load")
    return;
  if (isModuleSelfMember(node)) {
    StringRef member;
    isModuleSelfMember(node, &member);
    memberRead(member, node);
    return;
  }
  if (node.kind() == "Attribute" && node.child("value").kind() == "Name") {
    StringRef local = node.child("value").string("id");
    formalRead(local, node);
    return;
  }
  if (node.kind() == "Name") {
    formalRead(node.string("id"), node);
    return;
  }
  visitImmediateAstChildren(node, [&](const AstNode &child) {
    collectRuleExpressionReads(child, formalRead, memberRead);
  });
}

void collectRuleStatementReads(const AstNode &node, RuleReadCallback formalRead,
                               RuleReadCallback memberRead) {
  if (node.kind() == "Return" || node.kind() == "Expr") {
    AstNode value = node.child("value");
    if (value)
      collectRuleExpressionReads(value, formalRead, memberRead);
    return;
  }
  if (node.kind() == "Assert") {
    collectRuleExpressionReads(node.child("test"), formalRead, memberRead);
    AstNode message = node.child("msg");
    if (message)
      collectRuleExpressionReads(message, formalRead, memberRead);
    return;
  }
  if (node.kind() == "Assign") {
    collectRuleExpressionReads(node.child("value"), formalRead, memberRead);
    ArrayAttr targets = node.array("targets");
    for (size_t index = 0; index < targets.size(); ++index)
      collectRuleTargetReads(node.item("targets", index), formalRead,
                             memberRead);
    return;
  }
  if (node.kind() == "AnnAssign") {
    AstNode value = node.child("value");
    if (value)
      collectRuleExpressionReads(value, formalRead, memberRead);
    collectRuleTargetReads(node.child("target"), formalRead, memberRead);
    return;
  }
  if (node.kind() != "If")
    return;
  collectRuleExpressionReads(node.child("test"), formalRead, memberRead);
  for (StringRef arm : {"body", "orelse"}) {
    ArrayAttr statements = node.array(arm);
    for (size_t index = 0; index < statements.size(); ++index)
      collectRuleStatementReads(node.item(arm, index), formalRead, memberRead);
  }
}

namespace {

mlir::DictionaryAttr stateReference(OpBuilder &builder,
                                    const ModuleModel &module,
                                    const ModuleMember &member) {
  if (member.kind == ModuleMember::Kind::Connection)
    return builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("formal")),
        builder.getNamedAttr("parameter",
                             builder.getStringAttr(member.parameter)),
        builder.getNamedAttr("ordinal", builder.getUnitAttr()),
    });
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("owned")),
      builder.getNamedAttr(
          "declaration",
          occurrence(builder, module.symbol,
                     relativeToModule(module.declaration, member.declaration))),
      builder.getNamedAttr("element", builder.getArrayAttr({})),
  });
}

size_t canonicalStateMember(const ModuleModel &module, size_t memberIndex) {
  const ModuleMember &member = module.members[memberIndex];
  if (member.canonicalMemberIndex) {
    size_t canonical = *member.canonicalMemberIndex;
    if (canonical >= module.members.size() ||
        module.members[canonical].canonicalMemberIndex)
      llvm_unreachable(
          "module member has a dangling or cyclic canonical alias");
    return canonical;
  }
  if (member.kind != ModuleMember::Kind::Connection)
    return memberIndex;
  for (size_t index = 0; index < module.members.size(); ++index) {
    const ModuleMember &candidate = module.members[index];
    if (candidate.kind == ModuleMember::Kind::Connection &&
        candidate.parameter == member.parameter)
      return index;
  }
  llvm_unreachable("connection member has no canonical parameter member");
}

} // namespace

size_t bindRuleInput(RulePlan &plan, size_t argumentIndex) {
  size_t memberIndex = plan.arguments[argumentIndex].memberIndex;
  auto [entry, inserted] =
      plan.inputSlotForMember.try_emplace(memberIndex, plan.inputs.size());
  if (inserted)
    plan.inputs.push_back(argumentIndex);
  return entry->second;
}

std::optional<size_t> rulePlanInputSlot(const RulePlan &plan,
                                        size_t memberIndex) {
  auto found = plan.inputSlotForMember.find(memberIndex);
  if (found == plan.inputSlotForMember.end())
    return std::nullopt;
  return found->second;
}

RuleCompiler::RuleCompiler(RecordCompiler &sourceCompiler, ModuleModel &module)
    : sourceCompiler(sourceCompiler), module(module) {}

LogicalResult RuleCompiler::analyzeRegistration(size_t registrationIndex,
                                                RulePlan &plan) {
  const RuleRegistration &registration =
      module.registrations[registrationIndex];
  auto signature =
      parseFunctionSignature(registration.method.child("args"), false,
                             sourceCompiler.emitError, "lexical module rule");
  if (failed(signature))
    return failure();
  if (!signature->parameters.empty())
    return sourceCompiler.emitError()
           << "lexical rules take no parameters; capture module regs instead";
  plan.registrationIndex = registrationIndex;
  plan.signature = *signature;
  ArrayAttr body = registration.method.array("body");
  SmallVector<std::string> nonlocalOrder;
  auto scanNonlocals = [&](auto &&self, const AstNode &parent,
                           StringRef field) -> LogicalResult {
    ArrayAttr statements = parent.array(field);
    for (size_t index = 0; index < statements.size(); ++index) {
      AstNode statement = parent.item(field, index);
      if (statement.kind() == "Nonlocal") {
        for (Attribute rawName : statement.array("names")) {
          StringRef name = cast<StringAttr>(rawName).getValue();
          if (!plan.nonlocalNames.insert(name).second)
            return sourceCompiler.emitError()
                   << "nonlocal target '" << name << "' is repeated";
          nonlocalOrder.push_back(name.str());
        }
        continue;
      }
      if (statement.kind() == "If")
        for (StringRef arm : {"body", "orelse"})
          if (failed(self(self, statement, arm)))
            return failure();
    }
    return success();
  };
  if (failed(scanNonlocals(scanNonlocals, registration.method, "body")))
    return failure();
  auto scanLocals = [&](auto &&self, const AstNode &parent,
                        StringRef field) -> LogicalResult {
    ArrayAttr statements = parent.array(field);
    for (size_t index = 0; index < statements.size(); ++index) {
      AstNode statement = parent.item(field, index);
      if (statement.kind() == "Assign") {
        ArrayAttr targets = statement.array("targets");
        if (targets && targets.size() == 1) {
          AstNode target = statement.item("targets", 0);
          if (target.kind() == "Name" &&
              !plan.nonlocalNames.contains(target.string("id")))
            plan.localNames.insert(target.string("id"));
        }
      } else if (statement.kind() == "AnnAssign") {
        AstNode target = statement.child("target");
        if (target.kind() == "Name" &&
            !plan.nonlocalNames.contains(target.string("id")))
          plan.localNames.insert(target.string("id"));
      } else if (statement.kind() == "If") {
        for (StringRef arm : {"body", "orelse"})
          if (failed(self(self, statement, arm)))
            return failure();
      }
    }
    return success();
  };
  if (failed(scanLocals(scanLocals, registration.method, "body")))
    return failure();

  llvm::DenseSet<size_t> outputStates;
  for (const std::string &name : nonlocalOrder) {
    auto member = llvm::find_if(module.members, [&](const ModuleMember &item) {
      return item.name == name &&
             item.kind != ModuleMember::Kind::ChildInstance;
    });
    if (member == module.members.end())
      return sourceCompiler.emitError()
             << "nonlocal target '" << name << "' is not a module reg";
    size_t memberIndex = static_cast<size_t>(member - module.members.begin());
    member->write = true;
    member->writeSites.push_back(registration.method);
    size_t canonical = canonicalStateMember(module, memberIndex);
    if (outputStates.insert(canonical).second)
      plan.outputs.push_back(canonical);
  }

  llvm::StringSet<> boundInputs;
  auto lexicalRead = [&](StringRef name, const AstNode &site) {
    if (plan.localNames.contains(name))
      return;
    auto member = llvm::find_if(module.members, [&](const ModuleMember &item) {
      return item.name == name &&
             item.kind != ModuleMember::Kind::ChildInstance;
    });
    if (member == module.members.end())
      return;
    size_t memberIndex = static_cast<size_t>(member - module.members.begin());
    member->read = true;
    member->readSites.push_back(site);
    if (!boundInputs.insert(name).second)
      return;
    BoundRuleArgument argument;
    argument.localName = name.str();
    argument.memberIndex = canonicalStateMember(module, memberIndex);
    argument.actual = site;
    plan.arguments.push_back(std::move(argument));
    bindRuleInput(plan, plan.arguments.size() - 1);
  };
  auto retiredMemberRead = [&](StringRef, const AstNode &) {};
  for (size_t index = 0; index < body.size(); ++index) {
    AstNode statement = registration.method.item("body", index);
    if (statement.kind() == "Nonlocal")
      continue;
    collectRuleStatementReads(statement, lexicalRead, retiredMemberRead);
  }

  llvm::sort(plan.arguments,
             [](const BoundRuleArgument &left, const BoundRuleArgument &right) {
               return std::tie(left.memberIndex, left.localName) <
                      std::tie(right.memberIndex, right.localName);
             });
  plan.inputs.clear();
  plan.inputSlotForMember.clear();
  for (size_t index = 0; index < plan.arguments.size(); ++index)
    bindRuleInput(plan, index);

  plans.push_back(std::move(plan));
  return success();
}

LogicalResult RuleCompiler::analyze() {
  for (size_t index = 0; index < module.registrations.size(); ++index) {
    RulePlan plan;
    if (failed(analyzeRegistration(index, plan)))
      return failure();
  }
  return success();
}

FailureOr<Value>
RuleCompiler::expression(const RulePlan &plan, const AstNode &node,
                         const llvm::StringMap<Value> &values,
                         const llvm::StringMap<Value> &entryValues,
                         OpBuilder &builder, DictionaryAttr resultType) {
  if (node.kind() == "Name") {
    auto value = values.find(node.string("id"));
    if (value != values.end())
      return value->second;
    auto entry = entryValues.find(node.string("id"));
    if (entry == entryValues.end())
      return sourceCompiler.emitError()
             << "rule expression uses an unbound local or current name '"
             << node.string("id") << "'";
    DictionaryAttr origin = occurrence(
        builder, module.symbol, relativeToModule(module.declaration, node));
    Operation *read = createSourceOperation(
        builder,
        node.location(builder.getContext(), sourceCompiler.source.path),
        ac::SourceReadOp::getOperationName(), ValueRange{entry->second},
        TypeRange{entry->second.getType()},
        {builder.getNamedAttr("ac.origin", origin)});
    return read->getResult(0);
  }
  if (node.kind() == "Attribute") {
    StringRef memberName;
    if (isModuleSelfMember(node, &memberName)) {
      std::string key = (Twine("self.") + memberName).str();
      auto value = values.find(key);
      if (value == values.end())
        return sourceCompiler.emitError()
               << "rule state read is absent from input bindings";
      return value->second;
    }
    auto base = expression(plan, node.child("value"), values, entryValues,
                           builder, resultType);
    if (failed(base))
      return failure();
    auto recordType = dyn_cast<ac::StructType>((*base).getType());
    if (!recordType)
      return sourceCompiler.emitError()
             << "U02-A rule field read requires a nominal record";
    FlatSymbolRefAttr symbol =
        FlatSymbolRefAttr::get(builder.getContext(), recordType.getName());
    auto record = dyn_cast_or_null<ac::StructOp>(
        sourceCompiler.lookupCanonicalDeclaration(symbol));
    if (!record)
      return sourceCompiler.emitError()
             << "rule field read record has no canonical source/header "
                "declaration";
    StringRef fieldName = node.string("attr");
    for (Attribute rawField : record.getFields()) {
      auto field = cast<DictionaryAttr>(rawField);
      if (field.getAs<StringAttr>("name").getValue() != fieldName)
        continue;
      auto logical = field.getAs<DictionaryAttr>("type");
      Type physical = physicalType(logical, builder.getContext());
      Operation *get = createSourceOperation(
          builder,
          node.location(builder.getContext(), sourceCompiler.source.path),
          ac::StructGetOp::getOperationName(), ValueRange{*base},
          TypeRange{physical},
          {builder.getNamedAttr("field", builder.getStringAttr(fieldName))});
      return get->getResult(0);
    }
    return sourceCompiler.emitError()
           << "unknown nominal record field '" << fieldName << "'";
  }
  if (node.kind() == "Constant")
    return sourceCompiler.constant(node, resultType, builder);
  return sourceCompiler.emitError() << "U02-A rule body currently supports "
                                       "references, record fields and literals";
}

LogicalResult RuleCompiler::emit(size_t registrationIndex,
                                 Operation *moduleOperation) {
  auto plan = llvm::find_if(plans, [&](const RulePlan &candidate) {
    return candidate.registrationIndex == registrationIndex;
  });
  if (plan == plans.end())
    return sourceCompiler.emitError() << "registered rule analysis is missing";
  const RuleRegistration &registration =
      module.registrations[registrationIndex];
  OpBuilder &builder = sourceCompiler.builder;
  bool composed = needsComposedRule(registration.method);
  FailureOr<std::optional<PythonNumericNextPlan>> numericNext =
      composed ? FailureOr<std::optional<PythonNumericNextPlan>>(
                     std::optional<PythonNumericNextPlan>())
               : analyzePythonNumericNext(*plan, module, registration.method,
                                          sourceCompiler.emitError);
  if (failed(numericNext))
    return failure();
  FailureOr<std::optional<PythonImportNumericPlan>> numeric =
      (composed || *numericNext)
          ? FailureOr<std::optional<PythonImportNumericPlan>>(
                std::optional<PythonImportNumericPlan>())
          : analyzePythonImportNumericRule(*plan, module, registration.method,
                                           sourceCompiler.emitError);
  if (failed(numeric))
    return failure();
  SmallVector<Value> inputs, outputs;
  SmallVector<Attribute> inputBindings, outputBindings, inputTypes, outputTypes;
  for (size_t inputIndex : plan->inputs) {
    BoundRuleArgument &argument = plan->arguments[inputIndex];
    ModuleMember &member = module.members[argument.memberIndex];
    Value handle = member.kind == ModuleMember::Kind::OwnedState
                       ? member.handle
                       : member.currentHandle;
    if (!handle)
      return sourceCompiler.emitError()
             << "registered rule input has no current reg handle";
    inputs.push_back(handle);
    inputBindings.push_back(stateReference(builder, module, member));
    inputTypes.push_back(member.logicalType);
  }
  for (size_t outputIndex : plan->outputs) {
    ModuleMember &member = module.members[outputIndex];
    Value handle = member.kind == ModuleMember::Kind::OwnedState
                       ? member.handle
                       : member.nextHandle;
    if (!handle)
      return sourceCompiler.emitError()
             << "registered rule output has no next reg handle";
    outputs.push_back(handle);
    outputBindings.push_back(stateReference(builder, module, member));
    outputTypes.push_back(member.logicalType);
  }
  const RuleRegistration &call = registration;
  AstNode relativeRegistration =
      relativeToModule(module.declaration, call.call);
  AstNode relativeDefinition =
      relativeToModule(module.declaration, call.method);
  SmallVector<NamedAttribute> attributes{
      builder.getNamedAttr("name",
                           builder.getStringAttr(call.method.string("name"))),
      builder.getNamedAttr("registration", occurrence(builder, module.symbol,
                                                      relativeRegistration)),
      builder.getNamedAttr(
          "operandSegmentSizes",
          builder.getDenseI32ArrayAttr({static_cast<int32_t>(inputs.size()),
                                        static_cast<int32_t>(outputs.size())})),
      builder.getNamedAttr(
          "ac.origin", occurrence(builder, module.symbol, relativeDefinition)),
      builder.getNamedAttr("ac.source_owner", sourceCompiler.owner),
      builder.getNamedAttr("ac.input_bindings",
                           builder.getArrayAttr(inputBindings)),
      builder.getNamedAttr("ac.output_bindings",
                           builder.getArrayAttr(outputBindings)),
      builder.getNamedAttr("ac.input_types", builder.getArrayAttr(inputTypes)),
      builder.getNamedAttr("ac.output_types",
                           builder.getArrayAttr(outputTypes)),
  };
  Block &moduleBody = moduleOperation->getRegion(0).front();
  builder.setInsertionPoint(moduleBody.getTerminator());
  SmallVector<Value> operands;
  llvm::append_range(operands, inputs);
  llvm::append_range(operands, outputs);
  SmallVector<Type> resultTypes;
  for (size_t outputIndex : plan->outputs) {
    ModuleMember &member = module.members[outputIndex];
    resultTypes.push_back(member.payloadType);
    resultTypes.push_back(builder.getI1Type());
  }
  Operation *ruleOp = createSourceOperation(
      builder,
      call.method.location(builder.getContext(), sourceCompiler.source.path),
      ac::RuleOp::getOperationName(), operands, resultTypes, attributes, 1);
  if (*numeric)
    attachPythonImportNumericContract(**numeric, ruleOp, module.symbol,
                                      builder);
  Region &bodyRegion = ruleOp->getRegion(0);
  Block *body = new Block();
  bodyRegion.push_back(body);
  llvm::StringMap<Value> values;
  llvm::StringMap<Value> entryValues;
  llvm::StringMap<DictionaryAttr> valueTypes;
  for (size_t index = 0; index < plan->inputs.size(); ++index) {
    const BoundRuleArgument &argument = plan->arguments[plan->inputs[index]];
    const ModuleMember &member = module.members[argument.memberIndex];
    body->addArgument(member.payloadType,
                      argument.actual.location(builder.getContext(),
                                               sourceCompiler.source.path));
    entryValues[argument.localName] = body->getArgument(index);
    valueTypes[argument.localName] = member.logicalType;
  }
  for (size_t argumentIndex = 0; argumentIndex < plan->arguments.size();
       ++argumentIndex) {
    const BoundRuleArgument &argument = plan->arguments[argumentIndex];
    std::optional<size_t> inputSlot =
        rulePlanInputSlot(*plan, argument.memberIndex);
    if (inputSlot)
      entryValues[argument.localName] =
          body->getArgument(static_cast<unsigned>(*inputSlot));
    if (inputSlot)
      valueTypes[argument.localName] =
          module.members[argument.memberIndex].logicalType;
  }
  OpBuilder at = OpBuilder::atBlockEnd(body);
  ArrayAttr statements = call.method.array("body");
  SmallVector<Value> proposedValues(plan->outputs.size());
  SmallVector<Value> proposalEnables(plan->outputs.size());
  SmallVector<Attribute> requiredObservations;
  SmallVector<Attribute> requiredChecks;
  auto logicalTypeOf = [&](auto &&self,
                           const AstNode &node) -> FailureOr<DictionaryAttr> {
    if (node.kind() == "Name") {
      auto type = valueTypes.find(node.string("id"));
      if (type != valueTypes.end())
        return type->second;
      if (plan->localNames.contains(node.string("id")))
        return sourceCompiler.emitError()
               << "local '" << node.string("id")
               << "' is read before definition; assignment target is local or "
                  "lacks a nonlocal reg declaration";
      return sourceCompiler.emitError()
             << "rule expression uses an unbound lexical snapshot '"
             << node.string("id") << "'";
    }
    if (node.kind() == "Attribute") {
      if (isModuleSelfMember(node))
        return sourceCompiler.emitError()
               << "rule expressions require lexical names, not self members";
      auto base = self(self, node.child("value"));
      if (failed(base))
        return failure();
      auto kind = (*base).template getAs<StringAttr>("kind");
      auto symbol = (*base).template getAs<FlatSymbolRefAttr>("symbol");
      auto record = kind && kind.getValue() == "record" && symbol
                        ? dyn_cast_or_null<ac::StructOp>(
                              sourceCompiler.lookupCanonicalDeclaration(symbol))
                        : ac::StructOp();
      if (!record)
        return sourceCompiler.emitError()
               << "rule field read requires a canonical nominal record";
      for (Attribute rawField : record.getFields()) {
        auto field = cast<DictionaryAttr>(rawField);
        if (field.getAs<StringAttr>("name").getValue() == node.string("attr"))
          return field.getAs<DictionaryAttr>("type");
      }
      return sourceCompiler.emitError()
             << "unknown nominal record field '" << node.string("attr") << "'";
    }
    if (node.kind() == "Constant")
      return DictionaryAttr();
    return sourceCompiler.emitError()
           << "rule expressions support lexical names, record fields and "
              "bool/integer literals";
  };
  auto outputOrdinal = [&](StringRef name) -> std::optional<size_t> {
    auto member = llvm::find_if(module.members, [&](const ModuleMember &item) {
      return item.name == name &&
             item.kind != ModuleMember::Kind::ChildInstance;
    });
    if (member == module.members.end())
      return std::nullopt;
    size_t state = canonicalStateMember(
        module, static_cast<size_t>(member - module.members.begin()));
    for (size_t ordinal = 0; ordinal < plan->outputs.size(); ++ordinal)
      if (plan->outputs[ordinal] == state)
        return ordinal;
    return std::nullopt;
  };
  auto trueValue = [&](const AstNode &site) {
    return Value(at.create<arith::ConstantOp>(
        site.location(builder.getContext(), sourceCompiler.source.path),
        builder.getI1Type(), builder.getBoolAttr(true)));
  };
  PythonImportObservationProducer observations(
      at, sourceCompiler.source.path, module.symbol, module.declaration,
      ruleOp->getAttrOfType<DictionaryAttr>("registration"),
      sourceCompiler.emitError, values, entryValues, valueTypes);
  PythonImportCheckProducer checks(
      at, sourceCompiler.source.path, module.symbol, module.declaration,
      ruleOp->getAttrOfType<DictionaryAttr>("registration"),
      sourceCompiler.emitError, values, entryValues, valueTypes);
  auto observationIntrinsic = [&](const AstNode &statement) -> StringRef {
    AstNode call = statement.child("value");
    AstNode callee = call.child("func");
    if (statement.kind() != "Expr" || call.kind() != "Call" ||
        callee.kind() != "Name")
      return {};
    StringRef name = callee.string("id");
    if (plan->localNames.contains(name) || plan->nonlocalNames.contains(name))
      return {};
    if (sourceCompiler.namespaceBindings.contains(name) ||
        llvm::any_of(module.members, [&](const ModuleMember &member) {
          return member.name == name;
        }))
      return {};
    if (sourceCompiler.isCompilerDecorator(name, "log"))
      return "log";
    if (sourceCompiler.isCompilerDecorator(name, "report"))
      return "report";
    if (name != "print")
      return {};
    return "print";
  };
  if (composed)
    return emitComposedRule(at, cast<ac::RuleOp>(ruleOp), *plan, module,
                            call.method, sourceCompiler.source.path,
                            entryValues, valueTypes, observationIntrinsic,
                            sourceCompiler.emitError);
  auto emitAssignment = [&](const AstNode &statement, Value enable,
                            bool conditional) -> LogicalResult {
    ArrayAttr targets = statement.array("targets");
    if (!targets || targets.size() != 1 ||
        statement.item("targets", 0).kind() != "Name")
      return sourceCompiler.emitError()
             << "lexical rule assignment requires one name target";
    StringRef targetName = statement.item("targets", 0).string("id");
    AstNode rhs = statement.child("value");
    if (plan->localNames.contains(targetName)) {
      if (conditional)
        return sourceCompiler.emitError()
               << "branch-local assignment requires a local phi and is not "
                  "available in this rule subset";
      auto rhsLogical = logicalTypeOf(logicalTypeOf, rhs);
      if (failed(rhsLogical))
        return failure();
      if (!*rhsLogical)
        return sourceCompiler.emitError()
               << "local assignment requires an RHS with an exact inferred "
                  "LogicalType";
      auto rhsValue =
          expression(*plan, rhs, values, entryValues, at, *rhsLogical);
      if (failed(rhsValue))
        return failure();
      if ((*rhsValue).getType() !=
          physicalType(*rhsLogical, builder.getContext()))
        return sourceCompiler.emitError()
               << "local assignment physical type disagrees with its exact "
                  "LogicalType";
      values[targetName] = *rhsValue;
      valueTypes[targetName] = *rhsLogical;
      return success();
    }

    auto ordinal = outputOrdinal(targetName);
    if (!ordinal)
      return sourceCompiler.emitError()
             << "assignment to '" << targetName
             << "' is local or lacks a nonlocal reg declaration";
    if (proposedValues[*ordinal])
      return sourceCompiler.emitError()
             << "overlapping writes to one nonlocal reg require explicit "
                "conflict semantics and are rejected";
    ModuleMember &target = module.members[plan->outputs[*ordinal]];
    auto rhsLogical = logicalTypeOf(logicalTypeOf, rhs);
    if (failed(rhsLogical))
      return failure();
    if (*rhsLogical && *rhsLogical != target.logicalType)
      return sourceCompiler.emitError()
             << "nonlocal proposal logical type does not exactly match target";
    auto rhsValue =
        expression(*plan, rhs, values, entryValues, at, target.logicalType);
    if (failed(rhsValue))
      return failure();
    if ((*rhsValue).getType() != target.payloadType)
      return sourceCompiler.emitError()
             << "nonlocal proposal physical type does not match target";
    Value path = enable ? enable : trueValue(statement);
    Value valid = trueValue(statement);
    DictionaryAttr useOrigin =
        occurrence(builder, module.symbol,
                   relativeToModule(module.declaration, statement));
    DictionaryAttr sourceOrigin;
    if (auto read = (*rhsValue).getDefiningOp<ac::SourceReadOp>())
      sourceOrigin = read->getAttrOfType<DictionaryAttr>("ac.origin");
    else
      sourceOrigin = occurrence(
          builder, module.symbol,
          relativeToModule(module.declaration, statement.child("value")));
    DictionaryAttr useID = builder.getDictionaryAttr({
        builder.getNamedAttr("origin", useOrigin),
        builder.getNamedAttr("role", builder.getStringAttr("next")),
        builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
    });
    DictionaryAttr valueID = builder.getDictionaryAttr({
        builder.getNamedAttr("origin", sourceOrigin),
        builder.getNamedAttr("slot", builder.getI32IntegerAttr(0)),
    });
    DictionaryAttr useTarget = builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("next_scalar")),
        builder.getNamedAttr("state", stateReference(builder, module, target)),
    });
    Operation *use = createSourceOperation(
        at,
        statement.location(builder.getContext(), sourceCompiler.source.path),
        ac::SourceUseOp::getOperationName(), ValueRange{*rhsValue, valid, path},
        TypeRange{target.payloadType, builder.getI1Type()},
        {builder.getNamedAttr("id", useID),
         builder.getNamedAttr("source", valueID),
         builder.getNamedAttr("target", useTarget)});
    proposedValues[*ordinal] = use->getResult(0);
    proposalEnables[*ordinal] = use->getResult(1);
    return success();
  };

  bool returned = false;
  bool conditionalSeen = false;
  for (size_t index = 0; index < statements.size(); ++index) {
    AstNode statement = call.method.item("body", index);
    if (isModuleDocstring(statement))
      continue;
    if (statement.kind() == "Nonlocal")
      continue;
    if (returned)
      return sourceCompiler.emitError()
             << "lexical rule has reachable statements after return";
    if (statement.kind() == "Return") {
      if (statement.child("value"))
        return sourceCompiler.emitError()
               << "lexical rules cannot return data; use nonlocal next writes";
      returned = true;
      continue;
    }
    if (conditionalSeen)
      return sourceCompiler.emitError()
             << "statements after a conditional proposal require control-flow "
                "join semantics and are rejected";
    if (statement.kind() == "Assign") {
      if (*numericNext && statement.value == (**numericNext).assignment.value) {
        auto current = entryValues.find((**numericNext).name);
        if (current == entryValues.end())
          return sourceCompiler.emitError() << "numeric next input is absent";
        auto &target = module.members[plan->outputs.front()];
        auto pair = emitPythonNumericNext(
            **numericNext, current->second,
            stateReference(builder, module, target), ruleOp, module.symbol,
            module.declaration, sourceCompiler.source.path, at);
        if (failed(pair))
          return failure();
        proposedValues[0] = (*pair)[0];
        proposalEnables[0] = (*pair)[1];
        continue;
      }
      if (*numeric && statement.value == (**numeric).assignment.value) {
        auto current = entryValues.find((**numeric).inputName);
        if (current == entryValues.end())
          return sourceCompiler.emitError()
                 << "numeric source current input is absent from rule bindings";
        if (failed(emitPythonImportNumericRecipe(
                **numeric, current->second, module.symbol, module.declaration,
                sourceCompiler.source.path, at, sourceCompiler.emitError)))
          return failure();
        continue;
      }
      if (failed(emitAssignment(statement, {}, false)))
        return failure();
      continue;
    }
    if (statement.kind() == "Expr") {
      auto emitted =
          observations.emit(statement, observationIntrinsic(statement),
                            trueValue(statement), requiredObservations);
      if (failed(emitted))
        return failure();
      if (*emitted)
        continue;
      return sourceCompiler.emitError()
             << "rule expression is unsupported; only canonical "
                "print/log/report observation calls are allowed";
    }
    if (statement.kind() == "Assert") {
      if (failed(checks.emitAssert(statement, trueValue(statement),
                                   requiredChecks.size(), requiredChecks)))
        return failure();
      continue;
    }
    if (statement.kind() != "If")
      return sourceCompiler.emitError()
             << "lexical rule supports SSA assignments, one conditional "
                "proposal, and bare return";

    auto conditionLogical =
        logicalTypeOf(logicalTypeOf, statement.child("test"));
    if (failed(conditionLogical))
      return failure();
    auto conditionKind = *conditionLogical
                             ? (*conditionLogical).getAs<StringAttr>("kind")
                             : StringAttr();
    if (!conditionKind || conditionKind.getValue() != "bool")
      return sourceCompiler.emitError()
             << "conditional proposal requires an exact boolean current/local "
                "condition";
    auto condition = expression(*plan, statement.child("test"), values,
                                entryValues, at, *conditionLogical);
    if (failed(condition))
      return failure();
    if (!(*condition).getType().isInteger(1))
      return sourceCompiler.emitError()
             << "conditional proposal condition must lower to i1";

    bool conditionalProposal = false;
    auto emitArm = [&](StringRef arm, Value enable) -> LogicalResult {
      bool armReturned = false;
      ArrayAttr armStatements = statement.array(arm);
      for (size_t armIndex = 0; armIndex < armStatements.size(); ++armIndex) {
        AstNode nested = statement.item(arm, armIndex);
        if (isModuleDocstring(nested))
          continue;
        if (armReturned)
          return sourceCompiler.emitError()
                 << "conditional proposal arm has reachable statements after "
                    "return";
        if (nested.kind() == "Return") {
          if (nested.child("value"))
            return sourceCompiler.emitError()
                   << "conditional proposal arms require bare return";
          armReturned = true;
          continue;
        }
        if (nested.kind() == "Assign") {
          if (failed(emitAssignment(nested, enable, true)))
            return failure();
          conditionalProposal = true;
          continue;
        }
        if (nested.kind() == "Assert") {
          if (failed(checks.emitAssert(nested, enable, requiredChecks.size(),
                                       requiredChecks)))
            return failure();
          continue;
        }
        if (nested.kind() == "Expr") {
          auto emitted = observations.emit(nested, observationIntrinsic(nested),
                                           enable, requiredObservations);
          if (failed(emitted))
            return failure();
          if (*emitted)
            continue;
          return sourceCompiler.emitError()
                 << "conditional rule expression is unsupported; only "
                    "canonical print/log/report observation calls are allowed";
        }
        return sourceCompiler.emitError()
               << "conditional proposal arms support assignments, asserts, "
                  "observations and bare return only";
      }
      return success();
    };
    if (failed(emitArm("body", *condition)))
      return failure();
    if (!statement.array("orelse").empty()) {
      Value one = trueValue(statement.child("test"));
      Value inverted = at.create<arith::XOrIOp>(
          statement.child("test").location(builder.getContext(),
                                           sourceCompiler.source.path),
          *condition, one);
      if (failed(emitArm("orelse", inverted)))
        return failure();
    }
    conditionalSeen = conditionalProposal;
  }
  if (!requiredObservations.empty())
    ruleOp->setAttr("ac.required_observations",
                    builder.getArrayAttr(requiredObservations));
  if (!requiredChecks.empty())
    ruleOp->setAttr("ac.required_checks", builder.getArrayAttr(requiredChecks));
  SmallVector<Value> yields;
  for (size_t ordinal = 0; ordinal < plan->outputs.size(); ++ordinal) {
    ModuleMember &target = module.members[plan->outputs[ordinal]];
    if (!proposedValues[ordinal])
      return sourceCompiler.emitError()
             << "nonlocal reg has no proposal value on any admitted path";
    yields.push_back(proposedValues[ordinal]);
    yields.push_back(proposalEnables[ordinal]);
  }
  createSourceOperation(
      at,
      call.method.location(builder.getContext(), sourceCompiler.source.path),
      ac::YieldOp::getOperationName(), yields, {}, {});
  return success();
}

} // namespace acir::compiler::detail

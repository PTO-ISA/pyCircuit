#include "PythonImportRules.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

void visitAstChildren(const AstNode &node,
                      llvm::function_ref<void(const AstNode &)> visitor) {
  for (NamedAttribute field : node.fields()) {
    if (auto child = dyn_cast<DictionaryAttr>(field.getValue());
        child && child.getAs<StringAttr>("kind")) {
      AstNode childNode = node.child(field.getName());
      visitor(childNode);
      visitAstChildren(childNode, visitor);
      continue;
    }
    if (auto array = dyn_cast<ArrayAttr>(field.getValue()))
      for (size_t index = 0; index < array.size(); ++index) {
        AstNode childNode = node.item(field.getName(), index);
        if (!childNode)
          continue;
        visitor(childNode);
        visitAstChildren(childNode, visitor);
      }
  }
}

void collectRuleReads(
    const AstNode &node,
    llvm::function_ref<void(StringRef, const AstNode &)> formalRead,
    llvm::function_ref<void(StringRef, const AstNode &)> memberRead) {
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
  visitAstChildren(node, [&](const AstNode &child) {
    collectRuleReads(child, formalRead, memberRead);
  });
}

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

} // namespace

RuleCompiler::RuleCompiler(RecordCompiler &sourceCompiler, ModuleModel &module)
    : sourceCompiler(sourceCompiler), module(module) {}

LogicalResult RuleCompiler::analyzeRegistration(size_t registrationIndex,
                                                RulePlan &plan) {
  const RuleRegistration &registration =
      module.registrations[registrationIndex];
  AstNode argumentsNode = registration.method.child("args");
  auto signature = parseFunctionSignature(
      argumentsNode, true, sourceCompiler.emitError, "module rule method");
  if (failed(signature))
    return failure();
  plan.registrationIndex = registrationIndex;
  plan.signature = *signature;

  ArrayAttr positional = registration.call.array("args");
  ArrayAttr keywords = registration.call.array("keywords");
  if (positional.size() > signature->parameters.size())
    return sourceCompiler.emitError()
           << "too many registered rule positional arguments";
  SmallVector<AstNode> actuals(signature->parameters.size());
  SmallVector<bool> bound(signature->parameters.size(), false);
  for (size_t index = 0; index < positional.size(); ++index) {
    if (signature->parameters[index].binding == "keyword_only")
      return sourceCompiler.emitError()
             << "keyword-only rule parameter passed positionally";
    actuals[index] = registration.call.item("args", index);
    bound[index] = true;
  }
  for (size_t index = 0; index < keywords.size(); ++index) {
    AstNode keyword = registration.call.item("keywords", index);
    StringRef name = keyword.string("arg");
    if (name.empty())
      return sourceCompiler.emitError()
             << "registered rule keyword unpacking is unsupported";
    size_t parameterIndex = 0;
    while (parameterIndex < signature->parameters.size() &&
           signature->parameters[parameterIndex].parameter.string("arg") !=
               name)
      ++parameterIndex;
    if (parameterIndex == signature->parameters.size())
      return sourceCompiler.emitError()
             << "unknown registered rule keyword '" << name << "'";
    if (signature->parameters[parameterIndex].binding == "positional_only")
      return sourceCompiler.emitError()
             << "positional-only rule parameter passed by keyword";
    if (bound[parameterIndex])
      return sourceCompiler.emitError()
             << "duplicate registered rule argument '" << name << "'";
    actuals[parameterIndex] = keyword.child("value");
    bound[parameterIndex] = true;
  }

  for (size_t index = 0; index < signature->parameters.size(); ++index) {
    const ParameterSyntax &syntax = signature->parameters[index];
    if (!bound[index]) {
      if (syntax.defaultValue)
        return sourceCompiler.emitError()
               << "U02-A registered rule defaults are "
                  "deferred to the full binding slice";
      return sourceCompiler.emitError()
             << "missing registered rule argument '"
             << syntax.parameter.string("arg") << "'";
    }
    StringRef actualMemberName;
    AstNode actual = actuals[index];
    if (!isModuleSelfMember(actual, &actualMemberName)) {
      if (actual.kind() == "Name") {
        auto member =
            llvm::find_if(module.members, [&](const ModuleMember &item) {
              return item.kind == ModuleMember::Kind::Connection &&
                     item.parameter == actual.string("id");
            });
        if (member != module.members.end())
          actualMemberName = member->name;
      }
    }
    if (actualMemberName.empty())
      return sourceCompiler.emitError() << "U02-A registered rule actual must "
                                           "name a declared module member";
    auto member = llvm::find_if(module.members, [&](const ModuleMember &item) {
      return item.name == actualMemberName &&
             item.kind != ModuleMember::Kind::ChildInstance;
    });
    if (member == module.members.end())
      return sourceCompiler.emitError() << "registered rule actual is not a "
                                           "module-owned or connection value";
    auto expected =
        sourceCompiler.annotation(syntax.parameter.child("annotation"));
    if (failed(expected))
      return failure();
    if (*expected != member->logicalType)
      return sourceCompiler.emitError()
             << "registered rule actual logical type does not match its formal";
    plan.arguments.push_back(
        {syntax.parameter.string("arg").str(),
         static_cast<size_t>(member - module.members.begin()), syntax.parameter,
         actual});
  }

  llvm::StringMap<size_t> formalArgumentIndices;
  for (size_t index = 0; index < plan.arguments.size(); ++index)
    formalArgumentIndices[plan.arguments[index].localName] = index;
  llvm::StringSet<> inputLocals;
  auto formalRead = [&](StringRef name, const AstNode &site) {
    auto found = formalArgumentIndices.find(name);
    if (found == formalArgumentIndices.end() ||
        !inputLocals.insert(name).second)
      return;
    size_t argumentIndex = found->second;
    plan.inputs.push_back(argumentIndex);
    ModuleMember &member =
        module.members[plan.arguments[argumentIndex].memberIndex];
    member.read = true;
    member.readSites.push_back(site);
  };
  auto memberRead = [&](StringRef name, const AstNode &site) {
    auto found = llvm::find_if(module.members, [&](const ModuleMember &item) {
      return item.name == name &&
             item.kind != ModuleMember::Kind::ChildInstance;
    });
    if (found == module.members.end())
      return;
    size_t memberIndex = static_cast<size_t>(found - module.members.begin());
    if (llvm::is_contained(plan.inputs, memberIndex))
      return;
    BoundRuleArgument argument;
    argument.localName = (Twine("self.") + name).str();
    argument.memberIndex = memberIndex;
    argument.formal = {};
    argument.actual = site;
    plan.arguments.push_back(std::move(argument));
    plan.inputs.push_back(plan.arguments.size() - 1);
    found->read = true;
    found->readSites.push_back(site);
  };

  ArrayAttr body = registration.method.array("body");
  for (size_t index = 0; index < body.size(); ++index) {
    AstNode statement = registration.method.item("body", index);
    if (statement.kind() == "Return")
      collectRuleReads(statement.child("value"), formalRead, memberRead);
    else if (statement.kind() == "Assign")
      collectRuleReads(statement.child("value"), formalRead, memberRead);
    else if (statement.kind() == "If") {
      collectRuleReads(statement.child("test"), formalRead, memberRead);
      for (StringRef arm : {"body", "orelse"})
        for (size_t childIndex = 0; childIndex < statement.array(arm).size();
             ++childIndex)
          collectRuleReads(statement.item(arm, childIndex), formalRead,
                           memberRead);
    }
  }

  if (!registration.outputMember.empty()) {
    auto output = llvm::find_if(module.members, [&](const ModuleMember &item) {
      return item.name == registration.outputMember &&
             item.kind != ModuleMember::Kind::ChildInstance;
    });
    if (output == module.members.end())
      return sourceCompiler.emitError()
             << "rule result target is not a module member";
    auto resultType =
        sourceCompiler.annotation(registration.method.child("returns"));
    if (failed(resultType) || *resultType != output->logicalType)
      return sourceCompiler.emitError()
             << "registered rule return type does not match its next target";
    plan.outputs.push_back(
        static_cast<size_t>(output - module.members.begin()));
  }
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

FailureOr<Value> RuleCompiler::expression(const RulePlan &plan,
                                          const AstNode &node,
                                          const llvm::StringMap<Value> &values,
                                          OpBuilder &builder,
                                          DictionaryAttr resultType) {
  if (node.kind() == "Name") {
    auto value = values.find(node.string("id"));
    if (value == values.end())
      return sourceCompiler.emitError()
             << "U02-A rule expression uses an unbound local name '"
             << node.string("id") << "'";
    return value->second;
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
    auto base =
        expression(plan, node.child("value"), values, builder, resultType);
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
             << "registered rule input has no current DFFE handle";
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
             << "registered rule output has no next DFFE handle";
    outputs.push_back(handle);
    outputBindings.push_back(stateReference(builder, module, member));
    outputTypes.push_back(member.logicalType);
  }
  auto returnType =
      sourceCompiler.annotation(registration.method.child("returns"));
  if (failed(returnType) && !plan->outputs.empty())
    return failure();
  if (plan->outputs.size() > 1)
    return sourceCompiler.emitError() << "U02-A rule supports one data result";

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
  Operation *ruleOp = createSourceOperation(
      builder,
      call.method.location(builder.getContext(), sourceCompiler.source.path),
      ac::RuleOp::getOperationName(), operands, {}, attributes, 4);
  Region &bodyRegion = ruleOp->getRegion(2);
  Block *body = new Block();
  bodyRegion.push_back(body);
  llvm::StringMap<Value> values;
  for (size_t index = 0; index < plan->inputs.size(); ++index) {
    const BoundRuleArgument &argument = plan->arguments[plan->inputs[index]];
    const ModuleMember &member = module.members[argument.memberIndex];
    body->addArgument(member.payloadType,
                      argument.actual.location(builder.getContext(),
                                               sourceCompiler.source.path));
    values[argument.localName] = body->getArgument(index);
  }
  for (size_t argumentIndex = 0; argumentIndex < plan->arguments.size();
       ++argumentIndex) {
    const BoundRuleArgument &argument = plan->arguments[argumentIndex];
    auto inputIndex = llvm::find(plan->inputs, argumentIndex);
    if (inputIndex != plan->inputs.end())
      values[argument.localName] = body->getArgument(
          static_cast<unsigned>(inputIndex - plan->inputs.begin()));
  }
  OpBuilder at = OpBuilder::atBlockEnd(body);
  ArrayAttr statements = call.method.array("body");
  AstNode returned;
  for (size_t index = 0; index < statements.size(); ++index) {
    AstNode statement = call.method.item("body", index);
    if (isModuleDocstring(statement))
      continue;
    if (statement.kind() != "Return" || returned)
      return sourceCompiler.emitError()
             << "U02-A rule body supports one return statement";
    returned = statement.child("value");
  }
  SmallVector<Value> yields;
  if (!plan->outputs.empty()) {
    if (!returned)
      return sourceCompiler.emitError()
             << "registered result rule requires a return value";
    ModuleMember &target = module.members[plan->outputs[0]];
    auto value = expression(*plan, returned, values, at, target.logicalType);
    if (failed(value))
      return failure();
    if ((*value).getType() != target.payloadType)
      return sourceCompiler.emitError()
             << "rule return physical type does not match result target";
    Value enabled = at.create<arith::ConstantOp>(
        returned.location(builder.getContext(), sourceCompiler.source.path),
        builder.getI1Type(), builder.getBoolAttr(true));
    yields.push_back(*value);
    yields.push_back(enabled);
  } else if (returned) {
    return sourceCompiler.emitError()
           << "U02-A does not allow discarding a rule data result";
  }
  createSourceOperation(
      at,
      call.method.location(builder.getContext(), sourceCompiler.source.path),
      ac::YieldOp::getOperationName(), yields, {}, {});
  return success();
}

} // namespace acir::compiler::detail

#include "PythonImportInternal.h"
#include "PythonImportRules.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

bool isHelperDocstring(const AstNode &node) {
  if (node.kind() != "Expr")
    return false;
  AstNode value = node.child("value");
  return value.kind() == "Constant" && isa<StringAttr>(value.get("value"));
}

void collectTypeDeclarations(Type type,
                             SmallVectorImpl<FlatSymbolRefAttr> &references);

void collectDeclarationReferences(
    Attribute attribute, SmallVectorImpl<FlatSymbolRefAttr> &references) {
  if (auto symbol = dyn_cast<FlatSymbolRefAttr>(attribute)) {
    references.push_back(symbol);
    return;
  }
  if (auto type = dyn_cast<TypeAttr>(attribute)) {
    collectTypeDeclarations(type.getValue(), references);
    return;
  }
  if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
    for (NamedAttribute field : dictionary)
      collectDeclarationReferences(field.getValue(), references);
    return;
  }
  if (auto array = dyn_cast<ArrayAttr>(attribute))
    for (Attribute element : array)
      collectDeclarationReferences(element, references);
}

void collectTypeDeclarations(Type type,
                             SmallVectorImpl<FlatSymbolRefAttr> &references) {
  if (auto record = dyn_cast<ac::StructType>(type))
    references.push_back(
        FlatSymbolRefAttr::get(type.getContext(), record.getName()));
  else if (auto function = dyn_cast<FunctionType>(type)) {
    for (Type input : function.getInputs())
      collectTypeDeclarations(input, references);
    for (Type result : function.getResults())
      collectTypeDeclarations(result, references);
  } else if (auto shaped = dyn_cast<ShapedType>(type))
    collectTypeDeclarations(shaped.getElementType(), references);
}

void collectOperationDeclarations(
    Operation *operation, SmallVectorImpl<FlatSymbolRefAttr> &references) {
  for (NamedAttribute attribute : operation->getAttrs())
    collectDeclarationReferences(attribute.getValue(), references);
  for (Type type : operation->getOperandTypes())
    collectTypeDeclarations(type, references);
  for (Type type : operation->getResultTypes())
    collectTypeDeclarations(type, references);
  for (Region &region : operation->getRegions())
    for (Block &block : region)
      for (BlockArgument argument : block.getArguments())
        collectTypeDeclarations(argument.getType(), references);
  for (Region &region : operation->getRegions())
    for (Block &block : region)
      for (Operation &nested : block)
        collectOperationDeclarations(&nested, references);
}

} // namespace

LogicalResult RuleCompiler::validateInactiveMethods() {
  for (const AstNode &method : module.inactiveRuleMethods) {
    auto signature = parseFunctionSignature(method.child("args"), false,
                                            sourceCompiler.emitError,
                                            "inactive lexical module rule");
    if (failed(signature))
      return failure();
    if (!signature->parameters.empty())
      return sourceCompiler.emitError()
             << "lexical rules take no parameters; capture module regs instead";
    llvm::StringSet<> nonlocals;
    llvm::StringSet<> requiredTargets;
    auto stateKey = [&](const ModuleMember &member) {
      if (member.kind == ModuleMember::Kind::Connection)
        return (Twine("formal:") + member.parameter).str();
      return (Twine("owned:") + member.name).str();
    };
    ArrayAttr statements = method.array("body");
    for (size_t index = 0; index < statements.size(); ++index) {
      AstNode statement = method.item("body", index);
      if (statement.kind() != "Nonlocal")
        continue;
      for (Attribute rawName : statement.array("names")) {
        StringRef name = cast<StringAttr>(rawName).getValue();
        auto member =
            llvm::find_if(module.members, [&](const ModuleMember &item) {
              return item.name == name;
            });
        if (member == module.members.end())
          return sourceCompiler.emitError()
                 << "inactive lexical rule has unknown nonlocal target '"
                 << name << "'";
        if (member->kind == ModuleMember::Kind::ChildInstance)
          return sourceCompiler.emitError()
                 << "inactive lexical rule cannot capture child module '"
                 << name << "'";
        if (!nonlocals.insert(name).second)
          return sourceCompiler.emitError()
                 << "inactive lexical rule repeats nonlocal target '" << name
                 << "'";
        requiredTargets.insert(stateKey(*member));
      }
    }

    auto typeOf = [&](auto &&self,
                      const AstNode &expression) -> FailureOr<DictionaryAttr> {
      if (expression.kind() == "Name") {
        StringRef name = expression.string("id");
        auto member =
            llvm::find_if(module.members, [&](const ModuleMember &item) {
              return item.name == name;
            });
        if (member == module.members.end())
          return sourceCompiler.emitError()
                 << "inactive lexical rule reads unknown name '" << name << "'";
        if (member->kind == ModuleMember::Kind::ChildInstance)
          return sourceCompiler.emitError()
                 << "inactive lexical rule cannot capture child module '"
                 << name << "'";
        return member->logicalType;
      }
      if (expression.kind() == "Attribute") {
        if (isModuleSelfMember(expression))
          return sourceCompiler.emitError()
                 << "inactive lexical rules require lexical names, not self "
                    "members";
        auto base = self(self, expression.child("value"));
        if (failed(base))
          return failure();
        auto kind = (*base).template getAs<StringAttr>("kind");
        auto symbol = (*base).template getAs<FlatSymbolRefAttr>("symbol");
        auto record =
            kind && kind.getValue() == "record" && symbol
                ? dyn_cast_or_null<ac::StructOp>(
                      sourceCompiler.lookupCanonicalDeclaration(symbol))
                : ac::StructOp();
        if (!record)
          return sourceCompiler.emitError()
                 << "inactive lexical rule field read requires a canonical "
                    "record";
        for (Attribute rawField : record.getFields()) {
          auto field = cast<DictionaryAttr>(rawField);
          if (field.getAs<StringAttr>("name").getValue() ==
              expression.string("attr"))
            return field.getAs<DictionaryAttr>("type");
        }
        return sourceCompiler.emitError()
               << "inactive lexical rule reads unknown record field '"
               << expression.string("attr") << "'";
      }
      if (expression.kind() == "Constant") {
        DictionaryAttr value =
            staticValue(sourceCompiler.builder, expression.get("value"),
                        sourceCompiler.emitError);
        if (!value)
          return sourceCompiler.emitError()
                 << "inactive lexical rule literal must be bool or integer";
        return DictionaryAttr();
      }
      return sourceCompiler.emitError()
             << "inactive lexical rule contains an unsupported expression";
    };

    llvm::StringSet<> assignedTargets;
    bool returned = false;
    for (size_t index = 0; index < statements.size(); ++index) {
      AstNode statement = method.item("body", index);
      if (isModuleDocstring(statement))
        continue;
      if (returned)
        return sourceCompiler.emitError()
               << "inactive lexical rule has reachable statements after "
                  "return";
      if (statement.kind() == "Nonlocal")
        continue;
      if (statement.kind() == "Return") {
        if (!statement.child("value")) {
          returned = true;
          continue;
        }
        return sourceCompiler.emitError()
               << "lexical rules cannot return data; use nonlocal next writes";
      }
      if (statement.kind() != "Assign")
        return sourceCompiler.emitError()
               << "inactive lexical rule contains unsupported source syntax";
      ArrayAttr targets = statement.array("targets");
      if (!targets || targets.size() != 1 ||
          statement.item("targets", 0).kind() != "Name")
        return sourceCompiler.emitError()
               << "inactive lexical rule assignment requires one name target";
      StringRef targetName = statement.item("targets", 0).string("id");
      if (!nonlocals.contains(targetName))
        return sourceCompiler.emitError()
               << "inactive lexical rule assignment target '" << targetName
               << "' lacks a nonlocal declaration";
      auto target =
          llvm::find_if(module.members, [&](const ModuleMember &item) {
            return item.name == targetName &&
                   item.kind != ModuleMember::Kind::ChildInstance;
          });
      if (target == module.members.end())
        return sourceCompiler.emitError()
               << "inactive lexical rule nonlocal target disappeared";
      if (!assignedTargets.insert(stateKey(*target)).second)
        return sourceCompiler.emitError()
               << "inactive lexical rule writes one canonical nonlocal state "
                  "more than once";
      auto valueType = typeOf(typeOf, statement.child("value"));
      if (failed(valueType))
        return failure();
      if (*valueType && *valueType != target->logicalType)
        return sourceCompiler.emitError()
               << "inactive lexical rule assignment logical type does not "
                  "exactly match its nonlocal target";
      if (!*valueType) {
        DictionaryAttr value = staticValue(
            sourceCompiler.builder, statement.child("value").get("value"),
            sourceCompiler.emitError);
        auto resolver = [&](FlatSymbolRefAttr symbol) {
          return sourceCompiler.headers.resolveRecord(symbol);
        };
        if (failed(ac::detail::verifyStaticValueMatchesType(
                value, target->logicalType,
                ac::detail::ExpectedTypeKind::Logical, resolver,
                sourceCompiler.emitError)))
          return failure();
      }
    }
    if (assignedTargets.size() != requiredTargets.size())
      return sourceCompiler.emitError()
             << "inactive lexical rule requires one unconditional assignment "
                "to every canonical nonlocal target";
  }
  return success();
}

LogicalResult
RecordCompiler::cloneImportedDeclarations(bool includeBodySnapshots) {
  llvm::DenseSet<Attribute> visited;
  SmallVector<FlatSymbolRefAttr> pending;
  for (const NamespaceImportUse &use : namespaceImportUses)
    pending.push_back(use.target);
  llvm::sort(pending, [](FlatSymbolRefAttr left, FlatSymbolRefAttr right) {
    return left.getValue() < right.getValue();
  });
  for (size_t index = 0; index < pending.size(); ++index) {
    FlatSymbolRefAttr target = pending[index];
    if (!visited.insert(target).second)
      continue;
    Operation *sourceOperation = headers.lookupDeclaration(target);
    if (!sourceOperation)
      return emitError() << "imported name lacks a verified declaration";
    Operation *clone = sourceOperation->clone();
    clone->setAttr("ac.declaration_role",
                   builder.getStringAttr("import_snapshot"));
    interface->getBody()->push_back(clone);
    if (includeBodySnapshots) {
      Operation *bodySnapshot = sourceOperation->clone();
      bodySnapshot->setAttr("ac.declaration_role",
                            builder.getStringAttr("import_snapshot"));
      body->getBody()->push_back(bodySnapshot);
    }

    SmallVector<FlatSymbolRefAttr> references;
    collectOperationDeclarations(sourceOperation, references);
    llvm::sort(references, [](FlatSymbolRefAttr left, FlatSymbolRefAttr right) {
      return left.getValue() < right.getValue();
    });
    for (FlatSymbolRefAttr reference : references)
      if (!visited.contains(reference) && headers.lookupDeclaration(reference))
        pending.push_back(reference);
  }
  return success();
}

FailureOr<Value> RecordCompiler::constant(const AstNode &node,
                                          DictionaryAttr expected,
                                          OpBuilder &at) {
  Attribute raw = node.get("value");
  DictionaryAttr value = staticValue(builder, raw, emitError);
  if (!value)
    return emitError() << "U01 expression requires bool or integer literal";
  auto resolver = [&](FlatSymbolRefAttr symbol) {
    return headers.resolveRecord(symbol);
  };
  if (failed(ac::detail::verifyStaticValueMatchesType(
          value, expected, ac::detail::ExpectedTypeKind::Logical, resolver,
          emitError)))
    return failure();
  Type type = physicalType(expected, builder.getContext());
  if (auto boolean = dyn_cast_or_null<BoolAttr>(raw))
    return at
        .create<arith::ConstantOp>(
            node.location(builder.getContext(), source.path), type, boolean)
        .getResult();
  auto integer = value.getAs<ac::MathIntAttr>("value");
  llvm::APSInt number(integer.getCanonicalValue());
  auto storage = cast<IntegerType>(type);
  APInt bits = number.isSigned() ? number.sextOrTrunc(storage.getWidth())
                                 : number.zextOrTrunc(storage.getWidth());
  return at
      .create<arith::ConstantOp>(
          node.location(builder.getContext(), source.path),
          IntegerAttr::get(storage, bits))
      .getResult();
}

FailureOr<Value> RecordCompiler::recordCall(const AstNode &node,
                                            func::FuncOp function,
                                            OpBuilder &at, Value &liveValid) {
  AstNode callee = node.child("func");
  if (callee.kind() != "Name")
    return emitError() << "U01 constructor call requires an imported name";
  auto binding = namespaceBindings.find(callee.string("id"));
  if (binding == namespaceBindings.end())
    return emitError() << "U01 constructor call must resolve from a header";
  Operation *declaration = lookupCanonicalDeclaration(binding->second.target);
  if (auto helper = dyn_cast_or_null<func::FuncOp>(declaration)) {
    auto kind = helper->getAttrOfType<StringAttr>("ac.helper_kind");
    if (!kind || kind.getValue() != "value")
      return emitError() << "constructor target is not a nominal record";
    if (!node.array("args").empty() || !node.array("keywords").empty())
      return emitError() << "U01 value-helper calls do not take arguments";
    auto call = at.create<func::CallOp>(
        node.location(builder.getContext(), source.path),
        binding->second.target, helper.getFunctionType().getResults(),
        ValueRange{liveValid});
    liveValid = call.getResult(1);
    return call.getResult(0);
  }
  auto record = dyn_cast_or_null<ac::StructOp>(declaration);
  if (!record)
    return emitError() << "constructor target is not a nominal record";
  auto constructor = dyn_cast_or_null<func::FuncOp>(
      lookupCanonicalDeclaration(record.getConstructorAttr()));
  if (!constructor)
    return emitError() << "record header has no verified constructor";
  ArrayAttr metadata = constructor->getAttrOfType<ArrayAttr>("ac.parameters");
  ArrayAttr positional = node.array("args");
  ArrayAttr keywords = node.array("keywords");
  SmallVector<std::optional<Value>> bound(metadata.size());
  if (positional.size() > metadata.size())
    return emitError() << "too many constructor positional arguments";
  for (size_t index = 0; index < positional.size(); ++index) {
    auto parameter = cast<DictionaryAttr>(metadata[index]);
    if (parameter.getAs<StringAttr>("binding").getValue() == "keyword_only")
      return emitError()
             << "keyword-only constructor parameter passed positionally";
    DictionaryAttr type = parameter.getAs<DictionaryAttr>("constraint")
                              .getAs<DictionaryAttr>("type");
    auto lowered = constant(node.item("args", index), type, at);
    if (failed(lowered))
      return failure();
    bound[index] = *lowered;
  }
  for (size_t keywordIndex = 0; keywordIndex < keywords.size();
       ++keywordIndex) {
    AstNode keyword = node.item("keywords", keywordIndex);
    StringRef keywordName = keyword.string("arg");
    if (keywordName.empty())
      return emitError() << "unpacked constructor keywords are unsupported";
    std::optional<size_t> parameterIndex;
    for (size_t index = 0; index < metadata.size(); ++index)
      if (cast<DictionaryAttr>(metadata[index])
              .getAs<StringAttr>("name")
              .getValue() == keywordName) {
        parameterIndex = index;
        break;
      }
    if (!parameterIndex)
      return emitError() << "unknown constructor keyword '" << keywordName
                         << "'";
    auto parameter = cast<DictionaryAttr>(metadata[*parameterIndex]);
    if (parameter.getAs<StringAttr>("binding").getValue() == "positional_only")
      return emitError()
             << "positional-only constructor parameter passed by keyword";
    if (bound[*parameterIndex])
      return emitError() << "duplicate constructor argument '" << keywordName
                         << "'";
    DictionaryAttr type = parameter.getAs<DictionaryAttr>("constraint")
                              .getAs<DictionaryAttr>("type");
    auto lowered = constant(keyword.child("value"), type, at);
    if (failed(lowered))
      return failure();
    bound[*parameterIndex] = *lowered;
  }
  SmallVector<Value> values;
  for (size_t index = 0; index < metadata.size(); ++index) {
    auto parameter = cast<DictionaryAttr>(metadata[index]);
    StringRef parameterName = parameter.getAs<StringAttr>("name").getValue();
    if (bound[index]) {
      values.push_back(*bound[index]);
      continue;
    }
    auto defaultValue = parameter.getAs<DictionaryAttr>("default");
    if (!defaultValue.getAs<BoolAttr>("present").getValue())
      return emitError() << "missing constructor argument '" << parameterName
                         << "'";
    DictionaryAttr staticDefault = defaultValue.getAs<DictionaryAttr>("value");
    Attribute raw = staticDefault.get("value");
    DictionaryAttr type = parameter.getAs<DictionaryAttr>("constraint")
                              .getAs<DictionaryAttr>("type");
    Type physical = physicalType(type, builder.getContext());
    if (auto boolean = dyn_cast<BoolAttr>(raw))
      values.push_back(
          at.create<arith::ConstantOp>(function.getLoc(), physical, boolean));
    else {
      auto integer = cast<ac::MathIntAttr>(raw);
      llvm::APSInt number(integer.getCanonicalValue());
      auto storage = cast<IntegerType>(physical);
      APInt bits = number.isSigned() ? number.sextOrTrunc(storage.getWidth())
                                     : number.zextOrTrunc(storage.getWidth());
      values.push_back(at.create<arith::ConstantOp>(
          function.getLoc(), IntegerAttr::get(storage, bits)));
    }
  }
  values.push_back(liveValid);
  auto call = at.create<func::CallOp>(
      node.location(builder.getContext(), source.path),
      record.getConstructorAttr(), constructor.getFunctionType().getResults(),
      ValueRange(values));
  liveValid = call.getResult(1);
  return call.getResult(0);
}

FailureOr<Value> RecordCompiler::expression(const AstNode &node,
                                            func::FuncOp function,
                                            OpBuilder &at, Value &liveValid) {
  if (node.kind() == "Attribute") {
    auto base = expression(node.child("value"), function, at, liveValid);
    if (failed(base))
      return failure();
    auto recordType = dyn_cast<ac::StructType>((*base).getType());
    if (!recordType)
      return emitError() << "field access requires a nominal record";
    auto symbol =
        FlatSymbolRefAttr::get(builder.getContext(), recordType.getName());
    auto record =
        dyn_cast_or_null<ac::StructOp>(lookupCanonicalDeclaration(symbol));
    if (!record)
      return emitError()
             << "field access record is absent from header registry";
    StringRef fieldName = node.string("attr");
    for (Attribute rawField : record.getFields()) {
      auto field = cast<DictionaryAttr>(rawField);
      if (field.getAs<StringAttr>("name").getValue() != fieldName)
        continue;
      Type result = physicalType(field.getAs<DictionaryAttr>("type"),
                                 builder.getContext());
      return at
          .create<ac::StructGetOp>(
              node.location(builder.getContext(), source.path), result, *base,
              fieldName)
          .getResult();
    }
    return emitError() << "unknown nominal record field '" << fieldName << "'";
  }
  if (node.kind() == "Call")
    return recordCall(node, function, at, liveValid);
  return emitError() << "unsupported U01 value-helper expression";
}

LogicalResult RecordCompiler::emitValueHelper(const AstNode &node) {
  if (!node.array("decorator_list").empty() ||
      (node.array("type_params") && !node.array("type_params").empty()))
    return emitError() << "value helpers reject decorators and type parameters";
  StringRef name = node.string("name");
  std::string symbolText = qualifiedName(name);
  auto symbol = FlatSymbolRefAttr::get(builder.getContext(), symbolText);
  auto resultType = annotation(node.child("returns"));
  AstNode arguments = node.child("args");
  if (failed(resultType) || !arguments.array("posonlyargs").empty() ||
      !arguments.array("args").empty() ||
      !arguments.array("kwonlyargs").empty() ||
      !arguments.array("defaults").empty() ||
      !arguments.array("kw_defaults").empty() ||
      (arguments.get("vararg") && !isa<UnitAttr>(arguments.get("vararg"))) ||
      (arguments.get("kwarg") && !isa<UnitAttr>(arguments.get("kwarg"))))
    return emitError()
           << "U01 value helper requires no parameters and one type";
  ArrayAttr statements = node.array("body");
  AstNode returned;
  for (size_t index = 0; index < statements.size(); ++index) {
    AstNode statement = node.item("body", index);
    if (isHelperDocstring(statement))
      continue;
    if (statement.kind() != "Return" || returned)
      return emitError() << "U01 value helper supports one return statement";
    returned = statement.child("value");
  }
  if (!returned)
    return emitError() << "U01 value helper requires a return value";
  Type physical = physicalType(*resultType, builder.getContext());
  auto functionType = builder.getFunctionType(
      TypeRange{builder.getI1Type()}, TypeRange{physical, builder.getI1Type()});
  auto function =
      func::FuncOp::create(node.location(builder.getContext(), source.path),
                           symbolText, functionType);
  function->setAttr("ac.source_owner", owner);
  function->setAttr("ac.origin", occurrence(builder, symbol, {}));
  function->setAttr("ac.declaration_role", builder.getStringAttr("definition"));
  function->setAttr("ac.helper_kind", builder.getStringAttr("value"));
  function->setAttr("ac.parameters", builder.getArrayAttr({}));
  function->setAttr("ac.return_form", builder.getStringAttr("single"));
  function->setAttr(
      "ac.result_constraints",
      builder.getArrayAttr({valueConstraint(builder, *resultType)}));
  function->setAttr("ac.check_templates", builder.getArrayAttr({}));
  Block *entry = function.addEntryBlock();
  OpBuilder at = OpBuilder::atBlockEnd(entry);
  Value liveValid = entry->getArgument(0);
  auto value = expression(returned, function, at, liveValid);
  if (failed(value) || (*value).getType() != physical)
    return emitError() << "value helper return type does not match annotation";
  at.create<func::ReturnOp>(function.getLoc(), ValueRange{*value, liveValid});
  interface->getBody()->push_back(function);
  registerLocalDeclaration(symbol, function);
  return success();
}

} // namespace acir::compiler::detail

#include "PythonImportInternal.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

bool isHelperDocstring(const AstNode &node) {
  if (node.kind() != "Expr")
    return false;
  AstNode value = node.child("value");
  return value.kind() == "Constant" && isa<StringAttr>(value.get("value"));
}

} // namespace
LogicalResult RecordCompiler::cloneImportedDeclarations() {
  llvm::DenseSet<Operation *> cloned;
  SmallVector<ImportBinding> ordered;
  for (const auto &[name, binding] : imports)
    ordered.push_back(binding);
  llvm::sort(ordered,
             [](const ImportBinding &left, const ImportBinding &right) {
               return left.symbol.getValue() < right.symbol.getValue();
             });
  for (const ImportBinding &binding : ordered) {
    Operation *sourceOperation = nullptr;
    if (auto alias = headers.lookupAlias(binding.symbol))
      sourceOperation = alias;
    else if (auto record = headers.lookupRecord(binding.symbol))
      sourceOperation = record;
    if (!sourceOperation)
      continue;
    auto cloneSnapshot = [&](Operation *operation) {
      if (!cloned.insert(operation).second)
        return;
      Operation *clone = operation->clone();
      clone->setAttr("ac.declaration_role",
                     builder.getStringAttr("import_snapshot"));
      interface->getBody()->push_back(clone);
    };
    cloneSnapshot(sourceOperation);
    if (auto record = dyn_cast<ac::StructOp>(sourceOperation)) {
      auto constructor = headers.lookupHelper(record.getConstructorAttr());
      if (!constructor)
        return emitError() << "imported record lacks a verified constructor";
      cloneSnapshot(constructor);
    }
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
  auto imported = imports.find(callee.string("id"));
  if (imported == imports.end())
    return emitError() << "U01 constructor call must resolve from a header";
  auto record = headers.lookupRecord(imported->second.symbol);
  if (!record)
    return emitError() << "constructor target is not a nominal record";
  auto constructor = headers.lookupHelper(record.getConstructorAttr());
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
    auto record = headers.lookupRecord(symbol);
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
  return success();
}

} // namespace acir::compiler::detail

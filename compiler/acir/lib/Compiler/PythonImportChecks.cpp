#include "PythonImportChecks.h"

#include "PythonImportContext.h"
#include "PythonImportModules.h"

#include "acir/Dialect/ACIR/ACIROps.h"

using namespace mlir;

namespace acir::compiler::detail {

PythonImportCheckProducer::PythonImportCheckProducer(
    OpBuilder &builder, StringRef sourcePath, FlatSymbolRefAttr moduleSymbol,
    const AstNode &moduleDeclaration, DictionaryAttr registration,
    ac::detail::EmitError emitError, const llvm::StringMap<Value> &localValues,
    const llvm::StringMap<Value> &entryValues,
    const llvm::StringMap<DictionaryAttr> &valueTypes)
    : builder(builder), sourcePath(sourcePath), moduleSymbol(moduleSymbol),
      moduleDeclaration(moduleDeclaration), registration(registration),
      emitError(emitError), localValues(localValues), entryValues(entryValues),
      valueTypes(valueTypes) {}

LogicalResult PythonImportCheckProducer::emitAssert(
    const AstNode &statement, Value path, uint64_t obligation,
    SmallVectorImpl<Attribute> &requiredChecks) {
  if (statement.kind() != "Assert" || !path)
    return emitError() << "assert check requires one active rule path";
  AstNode conditionNode = statement.child("test");
  if (conditionNode.kind() != "Name")
    return emitError()
           << "assert check condition must be a direct persistent bool name";
  StringRef name = conditionNode.string("id");
  if (localValues.contains(name))
    return emitError()
           << "assert check condition cannot use a local or derived value";
  auto entry = entryValues.find(name);
  auto logical = valueTypes.find(name);
  auto kind = logical == valueTypes.end()
                  ? StringAttr()
                  : logical->second.getAs<StringAttr>("kind");
  if (entry == entryValues.end() || logical == valueTypes.end() || !kind ||
      kind.getValue() != "bool" || !entry->second.getType().isInteger(1))
    return emitError()
           << "assert check condition must be an exact persistent logical bool";

  AstNode message = statement.child("msg");
  if (message) {
    auto text = dyn_cast_or_null<StringAttr>(message.get("value"));
    if (message.kind() != "Constant" || !text)
      return emitError() << "assert check message must be a static string";
  }

  DictionaryAttr readOrigin =
      occurrence(builder, moduleSymbol,
                 relativeToModule(moduleDeclaration, conditionNode));
  Operation *read = createSourceOperation(
      builder, conditionNode.location(builder.getContext(), sourcePath),
      ac::SourceReadOp::getOperationName(), ValueRange{entry->second},
      TypeRange{entry->second.getType()},
      {builder.getNamedAttr("ac.origin", readOrigin)});
  DictionaryAttr check = occurrence(
      builder, moduleSymbol, relativeToModule(moduleDeclaration, statement));
  DictionaryAttr id = builder.getDictionaryAttr({
      builder.getNamedAttr("registration", registration),
      builder.getNamedAttr("check", check),
      builder.getNamedAttr("obligation", builder.getI64IntegerAttr(obligation)),
  });
  DictionaryAttr location = sourceSpan(builder, sourcePath, statement);
  createSourceOperation(
      builder, statement.location(builder.getContext(), sourcePath),
      ac::SourceExpectOp::getOperationName(),
      ValueRange{read->getResult(0), path}, {},
      {builder.getNamedAttr("kind", builder.getStringAttr("assert")),
       builder.getNamedAttr("ac.check_id", id),
       builder.getNamedAttr("location", location)});
  requiredChecks.push_back(builder.getDictionaryAttr({
      builder.getNamedAttr("id", id),
      builder.getNamedAttr("kind", builder.getStringAttr("assert")),
      builder.getNamedAttr("location", location),
  }));
  return success();
}

} // namespace acir::compiler::detail

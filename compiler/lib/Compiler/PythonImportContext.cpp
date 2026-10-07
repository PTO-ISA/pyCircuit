#include "PythonImportContext.h"

#include "pycircuit/Dialect/ACIR/ACIRDialect.h"

using namespace mlir;

namespace acir::compiler::detail {
FailureOr<ac::MathIntAttr> parseStaticInteger(OpBuilder &builder,
                                              StringRef spelling,
                                              ac::detail::EmitError emitError) {
  return ac::detail::parseMathIntAttr(builder.getContext(), spelling,
                                      emitError);
}

DictionaryAttr staticValue(OpBuilder &builder, Attribute raw,
                           ac::detail::EmitError emitError) {
  if (auto boolean = dyn_cast_or_null<BoolAttr>(raw))
    return builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("bool")),
        builder.getNamedAttr("value", boolean),
    });
  auto encoded = dyn_cast_or_null<DictionaryAttr>(raw);
  auto spelling = encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
  if (!spelling)
    return {};
  auto value = parseStaticInteger(builder, spelling.getValue(), emitError);
  if (failed(value))
    return {};
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("value", *value),
  });
}

Type physicalType(DictionaryAttr logical, MLIRContext *context) {
  StringRef kind = logical.getAs<StringAttr>("kind").getValue();
  if (kind == "bool")
    return IntegerType::get(context, 1);
  if (kind == "integer")
    return logical.getAs<TypeAttr>("storage").getValue();
  auto symbol = logical.getAs<FlatSymbolRefAttr>("symbol");
  return symbol ? Type(ac::StructType::get(
                      context, StringAttr::get(context, symbol.getValue())))
                : Type();
}

DictionaryAttr valueConstraint(OpBuilder &builder, DictionaryAttr type) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("logical")),
      builder.getNamedAttr("type", type),
  });
}

DictionaryAttr absentDefault(OpBuilder &builder) {
  return builder.getDictionaryAttr(
      {builder.getNamedAttr("present", builder.getBoolAttr(false))});
}

Operation *createSourceOperation(OpBuilder &builder, Location location,
                                 StringRef name, ValueRange operands,
                                 TypeRange results,
                                 ArrayRef<NamedAttribute> attributes,
                                 unsigned regionCount) {
  OperationState state(location, name);
  state.addOperands(operands);
  state.addTypes(results);
  state.addAttributes(attributes);
  for (unsigned index = 0; index < regionCount; ++index)
    state.addRegion();
  return builder.create(state);
}

} // namespace acir::compiler::detail

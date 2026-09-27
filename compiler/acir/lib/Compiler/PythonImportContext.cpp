#include "PythonImportContext.h"

#include "acir/Dialect/ACIR/ACIRDialect.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

std::string sourceModuleName(DictionaryAttr owner) {
  StringRef package = owner.getAs<StringAttr>("package").getValue();
  StringRef path = owner.getAs<StringAttr>("path").getValue();
  SmallVector<StringRef> parts;
  path.drop_back(3).split(parts, '/');
  if (!parts.empty() && parts.back() == "__init__")
    parts.pop_back();
  std::string result = package.str();
  for (StringRef part : parts) {
    if (!result.empty())
      result.push_back('.');
    result.append(part);
  }
  return result;
}

} // namespace

PythonImportContext::PythonImportContext(const CapturedSource &source,
                                         DictionaryAttr owner,
                                         const SourceHeaderRegistry &headers,
                                         ac::detail::EmitError emitError)
    : source(source), owner(owner), headers(headers), emitError(emitError),
      builder(owner.getContext()), module(sourceModuleName(owner)) {}

std::string PythonImportContext::qualifiedName(StringRef name) const {
  return module.empty() ? name.str() : (Twine(module) + "." + name).str();
}

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

} // namespace acir::compiler::detail

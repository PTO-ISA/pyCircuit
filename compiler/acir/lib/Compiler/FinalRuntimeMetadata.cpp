#include "FinalRuntimeMetadata.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"
using namespace mlir;
namespace acir::compiler {
namespace {
FailureOr<llvm::json::Value> convert(Attribute value,
                                     ac::detail::EmitError error) {
  if (auto item = dyn_cast<StringAttr>(value))
    return llvm::json::Value(item.getValue());
  if (auto item = dyn_cast<FlatSymbolRefAttr>(value))
    return llvm::json::Value(item.getValue());
  if (auto item = dyn_cast<BoolAttr>(value))
    return llvm::json::Value(item.getValue());
  if (auto item = dyn_cast<IntegerAttr>(value)) {
    if (item.getValue().getBitWidth() > 64)
      return error() << "runtime metadata integer exceeds u64";
    return llvm::json::Value(item.getValue().getZExtValue());
  }
  if (auto item = dyn_cast<ac::MathIntAttr>(value))
    return llvm::json::Value(item.getCanonicalValue());
  if (isa<UnitAttr>(value))
    return llvm::json::Value(nullptr);
  if (auto items = dyn_cast<ArrayAttr>(value)) {
    llvm::json::Array out;
    for (Attribute item : items) {
      auto converted = convert(item, error);
      if (failed(converted))
        return failure();
      out.push_back(std::move(*converted));
    }
    return llvm::json::Value(std::move(out));
  }
  if (auto items = dyn_cast<DictionaryAttr>(value)) {
    llvm::json::Object out;
    for (NamedAttribute item : items) {
      auto converted = convert(item.getValue(), error);
      if (failed(converted))
        return failure();
      out[item.getName().strref()] = std::move(*converted);
    }
    return llvm::json::Value(std::move(out));
  }
  return error() << "runtime metadata contains an unsupported attribute";
}
} // namespace
FailureOr<std::string> runtimeMetadataJson(Attribute value,
                                           ac::detail::EmitError error) {
  auto converted = convert(value, error);
  if (failed(converted))
    return failure();
  std::string text;
  llvm::raw_string_ostream out(text);
  out << *converted;
  return text;
}
} // namespace acir::compiler

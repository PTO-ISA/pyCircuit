#include "mlir/IR/Diagnostics.h"
#include "pycircuit/Dialect/ACIR/ACIRTypes.h"
using namespace mlir;
namespace acir::ac {
namespace {
std::optional<llvm::APSInt> literalInteger(StaticExprAttr expression) {
  if (!expression)
    return std::nullopt;
  DictionaryAttr tree = expression.getTree();
  auto kind = tree ? tree.getAs<StringAttr>("kind") : StringAttr();
  auto value = tree ? tree.getAs<DictionaryAttr>("value") : DictionaryAttr();
  auto valueKind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  auto integer = value ? value.getAs<MathIntAttr>("value") : MathIntAttr();
  if (!kind || kind.getValue() != "literal" || !valueKind ||
      valueKind.getValue() != "integer" || !integer)
    return std::nullopt;
  return llvm::APSInt(integer.getCanonicalValue());
}
bool isPositive(const llvm::APSInt &value) {
  return !value.isNegative() && !value.isZero();
}
} // namespace
} // namespace acir::ac

namespace acir::ac {

namespace detail {
LogicalResult shapeSchema(ArrayAttr shape,
                          function_ref<InFlightDiagnostic()> error) {
  if (!shape || shape.empty())
    return error() << "table shape requires positive rank";
  uint64_t product = 1;
  for (auto raw : shape) {
    auto expr = dyn_cast<StaticExprAttr>(raw);
    if (!expr || failed(StaticExprAttr::verify(error, expr.getTree())))
      return error() << "table extent requires valid StaticExpr";
    auto tree = expr.getTree();
    auto value = tree.getAs<DictionaryAttr>("value");
    auto literal = value ? value.getAs<MathIntAttr>("value") : MathIntAttr();
    if (!literal)
      continue;
    llvm::APSInt n(literal.getCanonicalValue());
    if (n.isNegative() || n.isZero() || n.getActiveBits() > 63)
      return error() << "table extent must be positive and fit signed 64 bits";
    if (product >
        uint64_t(std::numeric_limits<int64_t>::max()) / n.getZExtValue())
      return error() << "table element count exceeds signed 64 bits";
    product *= n.getZExtValue();
  }
  return success();
}
} // namespace detail

LogicalResult StructType::verify(llvm::function_ref<InFlightDiagnostic()> error,
                                 StringAttr name) {
  if (!name || name.getValue().empty() || name.getValue().contains('\0'))
    return error() << "struct type requires a nonempty nominal name";
  return success();
}
LogicalResult
TypeParamType::verify(llvm::function_ref<InFlightDiagnostic()> error,
                      FlatSymbolRefAttr owner, StringAttr name) {
  if (!owner || !name || name.getValue().empty() ||
      name.getValue().contains('\0'))
    return error() << "type parameter requires owner and nonempty name";
  return success();
}
namespace {
LogicalResult resourcePayload(llvm::function_ref<InFlightDiagnostic()> error,
                              Type payload) {
  if (!isa<BitsType, StructType, EnumType, TableType, TypeParamType>(payload))
    return error() << "resource payload must be hardware data, not a resource or module reference";
  return success();
}
}
LogicalResult PrimitiveType::verify(llvm::function_ref<InFlightDiagnostic()> error, Type payload) {
  return resourcePayload(error, payload);
}
LogicalResult StateType::verify(llvm::function_ref<InFlightDiagnostic()> error, Type payload) {
  return resourcePayload(error, payload);
}
LogicalResult QueueType::verify(llvm::function_ref<InFlightDiagnostic()> error, Type payload) {
  return resourcePayload(error, payload);
}
LogicalResult ModuleRefType::verify(llvm::function_ref<InFlightDiagnostic()> error,
                                   FlatSymbolRefAttr module) {
  if (!module || module.getValue().empty() || module.getValue().contains('\0'))
    return error() << "module reference requires a nonempty module symbol";
  return success();
}
LogicalResult
BitsType::verify(llvm::function_ref<InFlightDiagnostic()> emitError,
                 StaticExprAttr width) {
  if (!width || failed(StaticExprAttr::verify(emitError, width.getTree())))
    return emitError() << "bits width requires one valid StaticExpr";
  if (auto value = literalInteger(width); value && !isPositive(*value))
    return emitError() << "bits width must be positive";
  return success();
}


LogicalResult EnumType::verify(llvm::function_ref<InFlightDiagnostic()> error,
                               StringAttr name) {
  if (!name || name.getValue().empty() || name.getValue().contains('\0'))
    return error() << "enum type requires a nonempty nominal name";
  return success();
}

LogicalResult TableType::verify(function_ref<InFlightDiagnostic()> error,
                                ArrayAttr shape, Type elementType) {
  if (failed(detail::shapeSchema(shape, error)))
    return failure();
  if (!isa<BitsType, StructType, EnumType, TypeParamType, TableType>(elementType))
    return error() << "table element must be bits, struct, enum or type parameter";
  return success();
}

} // namespace acir::ac

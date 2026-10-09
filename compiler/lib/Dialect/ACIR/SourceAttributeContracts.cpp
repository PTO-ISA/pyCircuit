#include "pycircuit/Dialect/ACIR/ACIRAttributes.h"

#include "ACIRSourceContracts.h"
#include "SourceSyntaxSiteContracts.h"
#include "llvm/ADT/APSInt.h"

using namespace mlir;

namespace acir::ac {
namespace {

LogicalResult verifyStaticBound(Attribute value, detail::EmitError emitError) {
  auto expression = dyn_cast_or_null<StaticExprAttr>(value);
  if (!expression)
    return emitError() << "source type integer bound requires #ac.static_expr";
  return StaticExprAttr::verify(emitError, expression.getTree());
}

} // namespace

LogicalResult
SourceDomainAttr::verify(llvm::function_ref<InFlightDiagnostic()> emitError,
                         DictionaryAttr value) {
  auto kind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  if (!kind)
    return emitError()
           << "SourceDomain requires a dictionary with StringAttr kind";
  if (kind.getValue() == "bool") {
    if (value.size() != 1)
      return emitError() << "bool SourceDomain requires only kind";
    return success();
  }
  if (kind.getValue() != "integer" || value.size() != 3)
    return emitError()
           << "SourceDomain must be bool or integer with lower/upper";
  auto lower = value.getAs<MathIntAttr>("lower");
  auto upper = value.getAs<MathIntAttr>("upper");
  if (!lower || !upper)
    return emitError() << "integer SourceDomain bounds require #ac.math_int";
  // Canonical source values are arbitrary precision. There is deliberately no
  // storage-width or signedness feasibility check in this source carrier.
  if (failed(detail::parseMathIntAttr(value.getContext(),
                                      lower.getCanonicalValue(), emitError)) ||
      failed(detail::parseMathIntAttr(value.getContext(),
                                      upper.getCanonicalValue(), emitError)))
    return failure();
  llvm::APSInt lowerValue(lower.getCanonicalValue());
  llvm::APSInt upperValue(upper.getCanonicalValue());
  if (llvm::APSInt::compareValues(lowerValue, upperValue) >= 0)
    return emitError() << "integer SourceDomain requires lower < upper";
  return success();
}

LogicalResult
SourceTypeExprAttr::verify(llvm::function_ref<InFlightDiagnostic()> emitError,
                           DictionaryAttr value) {
  auto kind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  if (!kind)
    return emitError()
           << "SourceTypeExpr requires a dictionary with StringAttr kind";
  if (kind.getValue() == "bool") {
    if (value.size() != 1)
      return emitError() << "bool SourceTypeExpr requires only kind";
    return success();
  }
  if (kind.getValue() == "integer") {
    if (value.size() != 3)
      return emitError() << "integer SourceTypeExpr requires kind/lower/upper";
    if (failed(verifyStaticBound(value.get("lower"), emitError)) ||
        failed(verifyStaticBound(value.get("upper"), emitError)))
      return failure();
    return success();
  }
  if (kind.getValue() != "alias" || value.size() != 3)
    return emitError() << "SourceTypeExpr must be bool, integer or lexical "
                          "alias with name/site";
  if (failed(detail::verifyLexicalPath(value.getAs<ArrayAttr>("name"),
                                       emitError)) ||
      failed(detail::verifyNamespaceSite(value.getAs<DictionaryAttr>("site"),
                                         emitError)))
    return failure();
  return success();
}

} // namespace acir::ac

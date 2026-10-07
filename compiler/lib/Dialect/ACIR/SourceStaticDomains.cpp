#include "SourceStaticDomains.h"

#include "ACIRStaticEvaluation.h"
#include "SourceSyntaxSiteContracts.h"
#include "mlir/IR/Builders.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/APSInt.h"

using namespace mlir;

namespace acir::ac::detail {
FailureOr<SourceDomainAttr> resolveSourceDomain(SourceTypeExprAttr expression,
                                                Operation *declaringScope,
                                                EmitError error) {
  Operation *module = declaringScope;
  while (module && !isa<ModuleOp, ModuleImportOp>(module))
    module = module->getParentOp();
  auto owner = module ? module->getAttrOfType<DictionaryAttr>("source_owner")
                      : DictionaryAttr();
  if (!module || failed(verifySourceTypeExprOwner(expression, owner, error)))
    return error()
           << "source domain resolution requires actual declaring module owner";
  auto fields = expression.getValue();
  auto kind = fields.getAs<StringAttr>("kind").getValue();
  Builder builder(expression.getContext());
  DictionaryAttr domain;
  if (kind == "bool") {
    domain = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("bool"))});
  } else if (kind == "integer") {
    SmallVector<Attribute> bounds;
    for (StringRef field : {"lower", "upper"}) {
      auto value = evaluateSourceStaticExpr(fields.getAs<StaticExprAttr>(field),
                                            declaringScope, error);
      if (failed(value))
        return failure();
      auto integer = (*value).getAs<MathIntAttr>("value");
      if ((*value).getAs<StringAttr>("kind").getValue() != "integer" ||
          !integer)
        return error()
               << "source integer bound must evaluate to mathematical integer";
      bounds.push_back(integer);
    }
    domain = builder.getDictionaryAttr(
        {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
         builder.getNamedAttr("lower", bounds[0]),
         builder.getNamedAttr("upper", bounds[1])});
  } else {
    return error() << "source alias domain requires deferred explicit "
                      "declaration authority resolution";
  }
  if (failed(SourceDomainAttr::verify(error, domain)))
    return failure();
  return SourceDomainAttr::get(expression.getContext(), domain);
}

LogicalResult verifyStaticValueMatchesSourceDomain(DictionaryAttr value,
                                                   SourceDomainAttr domain,
                                                   EmitError error) {
  if (!domain)
    return error() << "source membership requires a resolved SourceDomain";
  if (failed(SourceDomainAttr::verify(error, domain.getValue())) ||
      failed(verifyStaticValueStructure(value, error)))
    return failure();
  auto fields = domain.getValue();
  auto expected = fields.getAs<StringAttr>("kind");
  if (value.getAs<StringAttr>("kind") != expected)
    return error()
           << "static value bool/integer kind differs from source domain";
  if (expected.getValue() == "bool")
    return success();
  auto integer = value.getAs<MathIntAttr>("value");
  if (!integer || failed(parseMathIntAttr(value.getContext(),
                                          integer.getCanonicalValue(), error)))
    return error() << "source integer value must be canonical MathInt";
  llvm::APSInt actual(integer.getCanonicalValue());
  llvm::APSInt lower(fields.getAs<MathIntAttr>("lower").getCanonicalValue());
  llvm::APSInt upper(fields.getAs<MathIntAttr>("upper").getCanonicalValue());
  if (llvm::APSInt::compareValues(actual, lower) < 0 ||
      llvm::APSInt::compareValues(actual, upper) >= 0)
    return error() << "static value is outside half-open source domain";
  return success();
}
} // namespace acir::ac::detail

#ifndef ACIR_LIB_DIALECT_ACIR_SOURCESTATICDOMAINS_H
#define ACIR_LIB_DIALECT_ACIR_SOURCESTATICDOMAINS_H

#include "ACIRSourceContracts.h"
#include "mlir/IR/Operation.h"

namespace acir::ac::detail {
mlir::FailureOr<SourceDomainAttr>
resolveSourceDomain(SourceTypeExprAttr expression,
                    mlir::Operation *declaringScope, EmitError error);
mlir::LogicalResult
verifyStaticValueMatchesSourceDomain(mlir::DictionaryAttr value,
                                     SourceDomainAttr domain, EmitError error);
} // namespace acir::ac::detail
#endif

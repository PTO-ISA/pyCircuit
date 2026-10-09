#ifndef ACIR_LIB_DIALECT_ACIR_SOURCESYNTAXSITECONTRACTS_H
#define ACIR_LIB_DIALECT_ACIR_SOURCESYNTAXSITECONTRACTS_H

#include "ACIRSourceContracts.h"

namespace acir::ac::detail {

mlir::LogicalResult verifyLexicalPath(mlir::ArrayAttr path, EmitError error);
mlir::LogicalResult
verifyNamespaceSite(mlir::DictionaryAttr site, EmitError error,
                    mlir::DictionaryAttr expectedOwner = {});

mlir::LogicalResult
verifySourceTypeExprOwner(SourceTypeExprAttr expression,
                          mlir::DictionaryAttr expectedOwner, EmitError error);
mlir::LogicalResult verifyStaticExprOwner(StaticExprAttr expression,
                                          mlir::DictionaryAttr expectedOwner,
                                          EmitError error);

} // namespace acir::ac::detail
#endif

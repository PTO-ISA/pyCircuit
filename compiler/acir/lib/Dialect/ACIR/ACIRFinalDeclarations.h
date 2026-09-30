#ifndef ACIR_LIB_DIALECT_ACIR_ACIRFINALDECLARATIONS_H
#define ACIR_LIB_DIALECT_ACIR_ACIRFINALDECLARATIONS_H

#include "ACIRSourceContracts.h"
#include "acir/Dialect/ACIR/ACIROps.h"

namespace acir::ac::final_detail {

mlir::FailureOr<std::string>
sourceImportModuleName(mlir::DictionaryAttr owner, detail::EmitError emitError);

mlir::FailureOr<std::string>
sourceImportModuleIdentity(mlir::DictionaryAttr owner,
                           detail::EmitError emitError);

mlir::FailureOr<mlir::DictionaryAttr>
sourceOwnerCaseFoldIdentity(mlir::DictionaryAttr owner,
                            detail::EmitError emitError);

mlir::LogicalResult verifyFinalQualifiedSymbol(mlir::DictionaryAttr owner,
                                               llvm::StringRef symbol,
                                               detail::EmitError emitError);

mlir::FailureOr<mlir::FlatSymbolRefAttr>
verifyFinalScalarDeclaration(mlir::Operation *operation,
                             mlir::DictionaryAttr enclosingOwner,
                             detail::EmitError emitError);

} // namespace acir::ac::final_detail

#endif // ACIR_LIB_DIALECT_ACIR_ACIRFINALDECLARATIONS_H

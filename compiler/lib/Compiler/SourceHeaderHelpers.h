#ifndef ACIR_LIB_COMPILER_SOURCEHEADERHELPERS_H
#define ACIR_LIB_COMPILER_SOURCEHEADERHELPERS_H

#include "SourceUnit.h"

namespace acir::compiler::detail {

bool sameDeclaration(mlir::Operation *definition, mlir::Operation *snapshot);

mlir::LogicalResult verifySourcePath(mlir::DictionaryAttr sourceSpan,
                                     mlir::DictionaryAttr owner,
                                     llvm::StringRef description,
                                     ac::detail::EmitError emitError);

mlir::LogicalResult verifyHeaderHelper(mlir::func::FuncOp helper,
                                       const SourceHeaderRegistry &registry,
                                       ac::detail::EmitError emitError);

} // namespace acir::compiler::detail

#endif // ACIR_LIB_COMPILER_SOURCEHEADERHELPERS_H

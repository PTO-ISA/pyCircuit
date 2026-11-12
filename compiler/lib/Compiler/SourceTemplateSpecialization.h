#ifndef ACIR_LIB_COMPILER_SOURCETEMPLATESPECIALIZATION_H
#define ACIR_LIB_COMPILER_SOURCETEMPLATESPECIALIZATION_H
#include "SourceLink.h"
namespace acir::compiler {
mlir::FailureOr<std::string> specializeSourceTemplates(
    mlir::ModuleOp candidate, const SourceHeaderRegistry &headers,
    llvm::StringRef top, llvm::ArrayRef<SourceLinkParameterBinding> parameters,
    ac::detail::EmitError emitError);
}
#endif

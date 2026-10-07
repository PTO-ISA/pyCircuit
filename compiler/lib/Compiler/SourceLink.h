#ifndef ACIR_LIB_COMPILER_SOURCELINK_H
#define ACIR_LIB_COMPILER_SOURCELINK_H

#include "SourceUnit.h"

namespace acir::compiler {

struct SourceLinkUnit {
  mlir::ModuleOp body;
  mlir::ModuleOp header;
};

mlir::FailureOr<SourceHeaderRegistry>
admitSourceLinkUnits(llvm::ArrayRef<SourceLinkUnit> units,
                     ac::detail::EmitError emitError);

mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
linkHardwareUnits(llvm::ArrayRef<SourceLinkUnit> units, llvm::StringRef top,
                  ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_SOURCELINK_H

#ifndef ACIR_LIB_COMPILER_FINALHARDWARE_H
#define ACIR_LIB_COMPILER_FINALHARDWARE_H

#include "FinalDeclarations.h"
#include "ModuleGraph.h"

namespace acir::compiler {

mlir::FailureOr<mlir::OwningOpRef<mlir::ModuleOp>>
materializeFinalHardwarePackage(
    llvm::SmallVectorImpl<mlir::OwningOpRef<mlir::ModuleOp>> &ownedUnits,
    llvm::ArrayRef<SourceLinkUnit> units, const ModuleGraph &modules,
    llvm::SmallVectorImpl<FinalDeclarationProjection> &declarations,
    ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALHARDWARE_H

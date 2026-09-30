#ifndef ACIR_LIB_COMPILER_FINALDECLARATIONS_H
#define ACIR_LIB_COMPILER_FINALDECLARATIONS_H

#include "SourceLink.h"
#include "SourceUnit.h"

namespace acir::compiler {

struct FinalDeclarationProjection {
  mlir::DictionaryAttr sourceOwner;
  mlir::StringAttr sourceUnitKind;
  mlir::OwningOpRef<mlir::ModuleOp> declarations;
};

mlir::FailureOr<llvm::SmallVector<FinalDeclarationProjection, 0>>
projectFinalDeclarations(llvm::ArrayRef<SourceLinkUnit> units,
                         const SourceHeaderRegistry &registry,
                         ac::detail::EmitError emitError);

} // namespace acir::compiler

#endif // ACIR_LIB_COMPILER_FINALDECLARATIONS_H

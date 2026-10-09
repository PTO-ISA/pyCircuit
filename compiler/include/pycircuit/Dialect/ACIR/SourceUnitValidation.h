#ifndef PYCIRCUIT_DIALECT_ACIR_SOURCEUNITVALIDATION_H
#define PYCIRCUIT_DIALECT_ACIR_SOURCEUNITVALIDATION_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/FunctionExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/SmallVector.h"

namespace acir::ac {

/// Structural facts only; pointers borrow the unit and mutation invalidates
/// them. Provider-header authority and canonical builtin admission remain with
/// callers.
struct SourceUnitStructure {
  mlir::DictionaryAttr owner;
  llvm::StringMap<mlir::Operation *> declarations;
};

mlir::FailureOr<SourceUnitStructure> verifySourceBodyStructure(
    mlir::ModuleOp unit,
    llvm::function_ref<mlir::InFlightDiagnostic()> emitError);

/// Retained provenance only; this does not authenticate original source units.
/// Declaration pointers borrow the unchanged final package.
struct FinalSourceUnitView {
  mlir::DictionaryAttr owner;
  llvm::SmallVector<mlir::Operation *> declarations;
};
mlir::FailureOr<llvm::SmallVector<FinalSourceUnitView>> collectFinalSourceUnits(
    mlir::ModuleOp package,
    llvm::function_ref<mlir::InFlightDiagnostic()> emitError);
mlir::FailureOr<SourceUnitStructure> verifySourceInterfaceStructure(
    mlir::ModuleOp unit,
    llvm::function_ref<mlir::InFlightDiagnostic()> emitError);

} // namespace acir::ac

#endif // PYCIRCUIT_DIALECT_ACIR_SOURCEUNITVALIDATION_H

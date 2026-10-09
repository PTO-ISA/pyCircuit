#include "pycircuit/Dialect/ACIR/SourceEffects.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
namespace acir::ac {
void SourceObserveOp::getEffects(
    llvm::SmallVectorImpl<mlir::MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(mlir::MemoryEffects::Write::get(),
                       SourceObservationResource::get());
}
void SourceExpectOp::getEffects(
    llvm::SmallVectorImpl<mlir::MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(mlir::MemoryEffects::Write::get(),
                       SourceCheckResource::get());
}
} // namespace acir::ac

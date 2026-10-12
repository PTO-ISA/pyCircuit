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
void SourceStateOp::getEffects(
    llvm::SmallVectorImpl<mlir::MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(mlir::MemoryEffects::Allocate::get(),
                       SourceStateResource::get());
  effects.emplace_back(mlir::MemoryEffects::Read::get(),
                       SourceStateResource::get());
}
void SourceStateReadOp::getEffects(
    llvm::SmallVectorImpl<mlir::MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(mlir::MemoryEffects::Read::get(),
                       SourceStateResource::get());
  // Index and control checks remain obligations even when the payload is dead.
  // Lowering may discharge them; pre-proof DCE must retain the source read.
  effects.emplace_back(mlir::MemoryEffects::Write::get(),
                       SourceCheckResource::get());
}
void SourceStateWriteOp::getEffects(
    llvm::SmallVectorImpl<mlir::MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(mlir::MemoryEffects::Write::get(),
                       SourceStateResource::get());
}
void SourceMethodCallOp::getEffects(
    llvm::SmallVectorImpl<mlir::MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(mlir::MemoryEffects::Read::get(),
                       SourceStateResource::get());
  effects.emplace_back(mlir::MemoryEffects::Write::get(),
                       SourceStateResource::get());
  effects.emplace_back(mlir::MemoryEffects::Write::get(),
                       SourceCheckResource::get());
}
void SourceTransactionOp::getEffects(
    llvm::SmallVectorImpl<mlir::MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(mlir::MemoryEffects::Read::get(),
                       SourceStateResource::get());
  effects.emplace_back(mlir::MemoryEffects::Write::get(),
                       SourceStateResource::get());
  effects.emplace_back(mlir::MemoryEffects::Write::get(),
                       SourceCheckResource::get());
}
} // namespace acir::ac

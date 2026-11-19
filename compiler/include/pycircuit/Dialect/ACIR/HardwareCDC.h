#ifndef PYCIRCUIT_DIALECT_ACIR_HARDWARECDC_H
#define PYCIRCUIT_DIALECT_ACIR_HARDWARECDC_H
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include <optional>
namespace acir::ac {
/// Authenticated logical native resource. Payload type retains the lexical
/// actual; bindings resolve geometry without rewriting a shared definition.
struct CDCPrimitiveLayout {
  InstanceOp allocation;
  ModuleImportOp primitive;
  HardwareBindings bindings;
  llvm::StringRef kind;
  mlir::Type payloadType;
  std::optional<uint64_t> depth;
  bool isFIFO() const { return kind == "async_fifo"; }
  mlir::Value sourceClock() const { return allocation->getOperand(0); }
  mlir::Value targetClock() const { return allocation->getOperand(1); }
  mlir::Value reset() const { return allocation->getOperand(2); }
  mlir::Value input() const { return allocation->getOperand(3); }
  mlir::Value output() const { return allocation->getResult(0); }
  mlir::Value pushIntent() const {
    return isFIFO() ? allocation->getOperand(4) : mlir::Value();
  }
  mlir::Value popIntent() const {
    return isFIFO() ? allocation->getOperand(5) : mlir::Value();
  }
  mlir::Value writeAvailable() const {
    return isFIFO() ? allocation->getResult(1) : mlir::Value();
  }
  mlir::Value readAvailable() const {
    return isFIFO() ? allocation->getResult(2) : mlir::Value();
  }
};
bool isCDCPrimitive(mlir::Operation *declaration);
bool isCDCInstance(InstanceOp instance);
mlir::LogicalResult verifyCDCPrimitiveDeclaration(
    const HardwareAnalysis &analysis, ModuleImportOp declaration);
mlir::FailureOr<CDCPrimitiveLayout> getCDCPrimitiveLayout(
    InstanceOp instance, const HardwareBindings &parent = {});
mlir::LogicalResult verifyBoundHardwareCDCOperation(
    const HardwareAnalysis &analysis, mlir::Operation *operation,
    const HardwareBindings &bindings);
mlir::LogicalResult verifyCDCRefinementContract(mlir::Operation *witness);
/// Authenticate FIFO endpoint authority independently of its component graph.
/// This proof does not invoke component verification.
mlir::LogicalResult verifyCDCRefinementIntents(
    const HardwareAnalysis &analysis, CDCRefinementOp witness,
    const HardwareBindings &bindings);
mlir::FailureOr<llvm::DenseMap<mlir::Operation *, mlir::Operation *>>
getVerifiedCDCComponentOwners(mlir::ModuleOp package);
mlir::LogicalResult verifyHardwareCDCComponent(mlir::Operation *witness);
mlir::LogicalResult verifyFinalHardwareCDCComponents(mlir::ModuleOp package);
struct HardwareCDCEndpointEffect {
  mlir::Operation *resource;
  mlir::Value clock, reset, intent, availability;
  RegisterEdge edge;
  RegisterResetPolarity polarity;
  bool write;
};
mlir::FailureOr<llvm::SmallVector<HardwareCDCEndpointEffect>>
getAsyncFIFOEndpointEffects(RuleOp rule);
bool hasLogicalCDC(mlir::Operation *root);
} // namespace acir::ac
#endif

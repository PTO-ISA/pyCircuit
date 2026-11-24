#ifndef PYCIRCUIT_DIALECT_GFIR_STRUCTUREDPROGRAM_H
#define PYCIRCUIT_DIALECT_GFIR_STRUCTUREDPROGRAM_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/GFIR/GFIROps.h"
#include <optional>

namespace acir::gfir {

struct StructuredCall {
  InstanceOp receiver;
  unsigned definition;
  unsigned output;
};

struct StructuredWire {
  ac::WireEndpoint endpoint;
  unsigned low;
  unsigned width;
  llvm::SmallVector<unsigned> dependencies;
  std::optional<StructuredCall> call;
};

struct StructuredOutput {
  ac::PortEndpoint port;
  unsigned wire;
  llvm::SmallVector<unsigned> inputs;
};

struct StructuredDefinition {
  ModuleOp module;
  llvm::SmallVector<StructuredWire, 0> wires;
  llvm::SmallVector<unsigned> order;
  llvm::SmallVector<StructuredOutput> outputs;
  llvm::DenseMap<mlir::Value, llvm::SmallVector<unsigned>> valueWires;
};

/// Verified facts reference the original, unmodified structured package.
/// Definitions are in callee-before-caller order. Wire dependencies and output
/// input summaries index leaf values in their enclosing definition.
struct StructuredProgram {
  RootOp root;
  unsigned rootIndex;
  llvm::SmallVector<StructuredDefinition, 0> definitions;
};

mlir::FailureOr<StructuredProgram> analyzeStructured(mlir::ModuleOp package);
void registerStructuredGFIRPasses();

} // namespace acir::gfir

#endif // PYCIRCUIT_DIALECT_GFIR_STRUCTUREDPROGRAM_H

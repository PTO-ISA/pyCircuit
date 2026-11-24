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

enum class StructuredFifoQuery { Ready, Valid, Data };
enum class StructuredFifoAccess { RawIn, RawOut, Status, In, Out, View };

struct StructuredFifoUse {
  unsigned fifo;
  StructuredFifoAccess access;
};

struct StructuredFifo {
  InstanceOp instance;
  mlir::Type element;
  uint64_t depth;
  uint64_t latency;
  bool downstreamPop;
  CallOp rawInput;
  CallOp rawOutput;
  std::optional<unsigned> producer;
  std::optional<unsigned> consumer;
};

struct StructuredRequirement {
  unsigned fifo;
  bool requiresReady;
  bool ownPop;
};

struct StructuredRule {
  RuleOp operation;
  llvm::SmallVector<StructuredRequirement> requirements;
  llvm::SmallVector<unsigned> states;
  llvm::SmallVector<unsigned> writes;
  llvm::SmallVector<unsigned> dependencies;
};

struct StructuredClocked {
  llvm::SmallVector<VarOp> variables;
  llvm::SmallVector<MethodOp> methods;
  llvm::SmallVector<unsigned> methodOrder;
  llvm::SmallVector<StructuredFifo> fifos;
  llvm::SmallVector<StructuredRule, 0> rules;
  llvm::DenseMap<mlir::Operation *, StructuredFifoUse> fifoCalls;
  llvm::DenseMap<mlir::Operation *, unsigned> stateAccesses;
  llvm::DenseMap<mlir::Operation *, unsigned> methodCalls;
};

struct StructuredWire {
  ac::WireEndpoint endpoint;
  unsigned low;
  unsigned width;
  llvm::SmallVector<unsigned> dependencies;
  std::optional<StructuredCall> call;
  std::optional<std::pair<unsigned, StructuredFifoQuery>> fifoQuery;
  std::optional<unsigned> state;
};

struct StructuredOutput {
  ac::PortEndpoint port;
  unsigned wire;
  llvm::SmallVector<unsigned> inputs;
};

struct StructuredDefinition {
  ModuleOp module;
  llvm::SmallVector<StructuredWire, 0> wires;
  /// Entries after the wire IDs (wires.size() + i) evaluate Rule i's grant.
  llvm::SmallVector<unsigned> order;
  llvm::SmallVector<StructuredOutput> outputs;
  llvm::DenseMap<mlir::Value, llvm::SmallVector<unsigned>> valueWires;
  std::optional<StructuredClocked> clocked;
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

#ifndef PYCIRCUIT_DIALECT_GFIR_STRUCTUREDPROGRAM_H
#define PYCIRCUIT_DIALECT_GFIR_STRUCTUREDPROGRAM_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/GFIR/GFIROps.h"
#include "llvm/ADT/DenseSet.h"
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

struct StructuredMemory {
  InstanceOp instance;
  mlir::Type element;
  llvm::StringRef kind;
  uint64_t depth;
  unsigned addressWidth;
  unsigned maskWidth;
  CallOp raw;

  bool asynchronous() const { return kind == "byte_mem"; }
  unsigned readPorts() const { return kind == "sync_mem_dp" ? 2 : 1; }
  unsigned writeOffset() const { return asynchronous() ? 1 : 2 * readPorts(); }
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
  llvm::SmallVector<ResultOp> resultUses;
  bool sampled = false;
};

struct StructuredClocked {
  llvm::SmallVector<VarOp> variables;
  llvm::SmallVector<MethodOp> methods;
  llvm::SmallVector<unsigned> methodOrder;
  llvm::DenseSet<unsigned> sampledMethods;
  llvm::SmallVector<StructuredFifo> fifos;
  llvm::SmallVector<StructuredMemory> memories;
  llvm::SmallVector<StructuredRule, 0> rules;
  llvm::DenseMap<mlir::Operation *, StructuredFifoUse> fifoCalls;
  llvm::DenseMap<mlir::Operation *, unsigned> memoryCalls;
  llvm::DenseMap<mlir::Operation *, unsigned> stateAccesses;
  llvm::DenseMap<mlir::Operation *, unsigned> methodCalls;
  /// Locally written state in either arm, including nested branches.
  llvm::DenseMap<mlir::Operation *, llvm::SmallVector<unsigned>> branchWrites;
  llvm::DenseSet<unsigned> conditionalWrites;
  llvm::DenseSet<mlir::Operation *> checkedBranches;
};

struct StructuredWire {
  ac::WireEndpoint endpoint;
  unsigned low;
  unsigned width;
  llvm::SmallVector<unsigned> dependencies;
  std::optional<StructuredCall> call;
  std::optional<std::pair<unsigned, StructuredFifoQuery>> fifoQuery;
  std::optional<unsigned> state;
  std::optional<std::pair<unsigned, unsigned>> ruleResult;
  std::optional<unsigned> memory;
};

struct StructuredOutput {
  ac::PortEndpoint port;
  unsigned wire;
  llvm::SmallVector<unsigned> inputs;
  bool requiresPreparation;
};

struct StructuredClockedCall {
  CallOp operation;
  InstanceOp receiver;
  unsigned definition;
  llvm::SmallVector<unsigned> dependencies;
};

struct StructuredDefinition {
  ModuleOp module;
  llvm::SmallVector<StructuredWire, 0> wires;
  /// Wires, Rule evaluations, then clocked instance preparations.
  llvm::SmallVector<unsigned> order;
  llvm::SmallVector<StructuredOutput> outputs;
  llvm::DenseMap<mlir::Value, llvm::SmallVector<unsigned>> valueWires;
  std::optional<StructuredClocked> clocked;
  llvm::SmallVector<StructuredClockedCall> clockedCalls;
  llvm::SmallVector<unsigned> preparationInputs;
  bool hasChecks = false;

  unsigned firstClockedCallNode() const {
    return wires.size() + (clocked ? clocked->rules.size() : 0);
  }

  llvm::ArrayRef<unsigned> dependencies(unsigned node) const {
    if (node < wires.size())
      return wires[node].dependencies;
    if (node < firstClockedCallNode())
      return clocked->rules[node - wires.size()].dependencies;
    return clockedCalls[node - firstClockedCallNode()].dependencies;
  }
};

/// Verified facts reference the original, unmodified structured package.
/// Definitions are in callee-before-caller order. Wire dependencies and output
/// input summaries index leaf values in their enclosing definition.
struct StructuredProgram {
  RootOp root;
  mlir::ArrayAttr samples;
  unsigned rootIndex;
  llvm::SmallVector<StructuredDefinition, 0> definitions;
};

mlir::FailureOr<StructuredProgram> analyzeStructured(mlir::ModuleOp package);
void registerStructuredGFIRPasses();

} // namespace acir::gfir

#endif // PYCIRCUIT_DIALECT_GFIR_STRUCTUREDPROGRAM_H

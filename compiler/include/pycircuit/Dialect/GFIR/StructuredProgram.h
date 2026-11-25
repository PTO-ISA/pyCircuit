#ifndef PYCIRCUIT_DIALECT_GFIR_STRUCTUREDPROGRAM_H
#define PYCIRCUIT_DIALECT_GFIR_STRUCTUREDPROGRAM_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/GFIR/GFIROps.h"
#include "llvm/ADT/DenseSet.h"
#include <array>
#include <compare>
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
  /// Ordered producer receives the admitted pop of a distinct local Rule.
  bool localPopCredit = false;
};

enum class StructuredMemoryAccess { Request, Result, Write, Read };

struct StructuredMemoryUse {
  unsigned memory;
  StructuredMemoryAccess access;
  unsigned port;
  unsigned rule;
};

struct StructuredMemory {
  InstanceOp instance;
  mlir::Type element;
  llvm::StringRef kind;
  uint64_t depth;
  unsigned addressWidth;
  unsigned maskWidth;
  CallOp raw;
  bool transactional = false;
  std::array<CallOp, 2> requests{};
  CallOp writer;
  CallOp reader;

  bool asynchronous() const { return kind == "byte_mem"; }
  unsigned readPorts() const { return kind == "sync_mem_dp" ? 2 : 1; }
  unsigned writeOffset() const { return asynchronous() ? 1 : 2 * readPorts(); }
};

struct StructuredRequirement {
  unsigned fifo;
  bool requiresReady;
  bool ownPop;
};

struct StructuredCheckSite {
  mlir::DictionaryAttr id;
  mlir::DictionaryAttr location;
  mlir::Operation *operation = nullptr;
};

struct StructuredWritePath {
  mlir::ArrayAttr fields;
  llvm::SmallVector<mlir::Type> types;
  unsigned low;
  unsigned width;
};

/// One actual state declaration, relative to the Module that owns the plan.
/// An empty path refers to that Module's own state.
struct StructuredStateRoute {
  llvm::SmallVector<InstanceOp, 2> path;
  unsigned definition;
  unsigned state;

  bool operator==(const StructuredStateRoute &) const = default;
};

struct StructuredChildState {
  StructuredStateRoute route;
  std::optional<unsigned> writer;
};

struct StructuredIncomingState {
  StructuredStateRoute route;
  /// Pure remote readers need Q but do not acquire a proposal input.
  bool writable;
};

struct StructuredFormalBinding {
  unsigned formal;
  /// Local variable ordinal or remote candidate slot.
  unsigned slot;
  bool local;
  bool writes;
};

struct StructuredChildMethod {
  llvm::SmallVector<InstanceOp, 2> path;
  unsigned definition;
  unsigned method;
  /// Callee state ordinal and transaction slot in the calling definition.
  llvm::SmallVector<std::pair<unsigned, unsigned>> states;
  llvm::SmallVector<StructuredFormalBinding> formals;
};

struct StructuredObservation {
  EffectOp operation;
  mlir::DictionaryAttr id;
  mlir::DictionaryAttr location;
  unsigned firstSlot = 0;
};

struct StructuredMethodEffects {
  uint64_t expandedWork;
  unsigned depth;
  llvm::SmallVector<unsigned> states;
  llvm::SmallVector<unsigned> writes;
  llvm::SmallVector<unsigned> formalWrites;
};

struct StructuredAvailabilitySource {
  enum class Kind { Rule, ChildOutput, ChildField, Select, Input, PureOutput };
  Kind kind;
  /// Rule or clocked call ordinal in the enclosing definition.
  unsigned index;
  /// Logical result port or private field-output ordinal for a child call.
  unsigned port = 0;
  auto operator<=>(const StructuredAvailabilitySource &) const = default;
};

using StructuredAvailability = llvm::SmallVector<StructuredAvailabilitySource, 1>;

struct StructuredAvailabilitySelect {
  unsigned wire;
  unsigned selector;
  StructuredAvailability yes;
  StructuredAvailability no;
};

struct StructuredInputAvailability {
  unsigned port;
  mlir::StringAttr name;
};

struct StructuredGuardCapture {
  unsigned capture;
  StructuredAvailability sources;
};

struct StructuredDataCaptureUse {
  unsigned capture;
  unsigned operand;
  bool returned;
};

struct StructuredRule {
  RuleOp operation;
  llvm::SmallVector<StructuredRequirement> requirements;
  llvm::SmallVector<unsigned> states;
  llvm::SmallVector<unsigned> writes;
  llvm::SmallVector<unsigned> dependencies;
  llvm::SmallVector<ResultOp> resultUses;
  llvm::SmallVector<StructuredGuardCapture> guardCaptures;
  llvm::SmallVector<EffectOp> assertions;
  llvm::SmallVector<StructuredObservation> observations;
  bool sampled = false;
  bool readsByteMemory = false;
  llvm::SmallVector<unsigned> childStates;
  llvm::SmallVector<StructuredCheckSite> checks;
  mlir::Operation *afterSnapshot = nullptr;
};

struct StructuredClocked {
  llvm::SmallVector<VarOp> variables;
  llvm::SmallVector<MethodOp> methods;
  llvm::SmallVector<unsigned> methodOrder;
  llvm::DenseSet<unsigned> sampledMethods;
  llvm::DenseMap<unsigned, StructuredMethodEffects> directMethodEffects;
  llvm::SmallVector<StructuredFifo> fifos;
  llvm::SmallVector<StructuredMemory> memories;
  llvm::SmallVector<StructuredRule, 0> rules;
  llvm::DenseMap<mlir::Operation *, StructuredFifoUse> fifoCalls;
  llvm::DenseMap<mlir::Operation *, unsigned> memoryCalls;
  llvm::DenseMap<mlir::Operation *, StructuredMemoryUse> transactionMemoryCalls;
  llvm::DenseMap<mlir::Operation *, unsigned> stateAccesses;
  llvm::DenseMap<mlir::Operation *, unsigned> formalAccesses;
  llvm::SmallDenseSet<unsigned> boundVariables;
  /// A forwarding Method call maps its formal ordinal to a caller formal.
  llvm::DenseMap<mlir::Operation *, llvm::SmallVector<std::pair<unsigned, unsigned>>>
      forwardedFormals;
  llvm::DenseMap<mlir::Operation *, StructuredWritePath> writePaths;
  llvm::DenseMap<mlir::Operation *, unsigned> methodCalls;
  llvm::DenseMap<mlir::Operation *, StructuredChildMethod> childMethodCalls;
  /// Locally written state in either arm, including nested branches.
  llvm::DenseMap<mlir::Operation *, llvm::SmallVector<unsigned>> branchWrites;
  llvm::DenseSet<unsigned> conditionalWrites;
  llvm::DenseSet<mlir::Operation *> checkedBranches;
  /// Relative ordinal of a check or callee entry in its callable's inventory.
  llvm::DenseMap<mlir::Operation *, uint64_t> checkOffsets;
  llvm::DenseMap<mlir::Operation *, llvm::SmallVector<StructuredDataCaptureUse>> dataCaptureUses;
  llvm::DenseMap<mlir::Operation *, unsigned> guardedCaptureUses;
  llvm::DenseMap<mlir::Operation *, mlir::DictionaryAttr> orderedOccurrences;
};

struct StructuredWire {
  ac::WireEndpoint endpoint;
  unsigned low;
  unsigned width;
  mlir::Type type;
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
  std::optional<unsigned> fieldOutput;
};

struct StructuredOutputAvailability {
  unsigned port;
  mlir::StringAttr name;
  StructuredAvailability sources;
};

struct StructuredFieldOutput {
  /// Leaf in the original output plan. It carries the exact logical field path.
  unsigned output;
  mlir::StringAttr name;
  mlir::StringAttr availabilityName;
  StructuredAvailability sources;
};

struct StructuredClockedCall {
  CallOp operation;
  InstanceOp receiver;
  unsigned definition;
  llvm::SmallVector<unsigned> dependencies;
  std::optional<std::pair<unsigned, unsigned>> domainInputs;
};

struct StructuredDefinition {
  bool explicitDomains = false;
  bool fourStateAvailability = false;
  ModuleOp module;
  llvm::SmallVector<StructuredWire, 0> wires;
  /// Wires, Rule evaluations, then clocked instance preparations.
  llvm::SmallVector<unsigned> order;
  llvm::SmallVector<StructuredOutput> outputs;
  llvm::SmallVector<StructuredOutputAvailability> outputAvailability;
  llvm::SmallVector<StructuredInputAvailability> inputAvailability;
  llvm::SmallVector<StructuredAvailability> wireAvailability;
  llvm::SmallVector<StructuredAvailabilitySelect> availabilitySelects;
  llvm::SmallVector<StructuredFieldOutput> fieldOutputs;
  llvm::DenseMap<mlir::Value, llvm::SmallVector<unsigned>> valueWires;
  std::optional<StructuredClocked> clocked;
  llvm::SmallVector<StructuredChildState> childStates;
  llvm::SmallVector<StructuredIncomingState> incomingStates;
  llvm::SmallVector<StructuredClockedCall> clockedCalls;
  llvm::SmallVector<unsigned> preparationInputs;
  uint64_t sourceCheckCount = 0;

  bool hasChecks() const { return sourceCheckCount != 0; }

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
/// Recover a proven local row write in an already verified package.
/// On change the old analysis no longer applies. Reverify before emission.
bool recoverStructuredRowWrite(mlir::ModuleOp package, const StructuredProgram &program);
void registerStructuredGFIRPasses();

} // namespace acir::gfir

#endif // PYCIRCUIT_DIALECT_GFIR_STRUCTUREDPROGRAM_H

#ifndef PYCIRCUIT_CONCRETE_DESIGN_PLAN_H
#define PYCIRCUIT_CONCRETE_DESIGN_PLAN_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

#include <cstdint>
#include <optional>
#include <string>

namespace acir::compiler {

struct ConcreteModuleDefinition {
  mlir::Operation *operation = nullptr;
  ConcreteModuleDefinition() = default;
  ConcreteModuleDefinition(mlir::Operation *operation) : operation(operation) {}
  ConcreteModuleDefinition(ac::ModuleOp module)
      : operation(module ? module.getOperation() : nullptr) {}
  operator mlir::Operation *() const { return operation; }
  mlir::Operation *operator->() const { return operation; }
  mlir::StringRef getSymName() const {
    auto name = operation
                    ? operation->getAttrOfType<mlir::StringAttr>("sym_name")
                    : mlir::StringAttr();
    return name ? name.getValue() : mlir::StringRef();
  }
  bool operator==(const ConcreteModuleDefinition &) const = default;
};

struct ConcreteIntegerBinding {
  mlir::StringAttr name;
  std::string canonicalValue;

  bool operator==(const ConcreteIntegerBinding &) const = default;
};

enum class ConcreteTypeKind { Bits, Table, Struct, Enum };

struct ConcreteTypeBinding {
  mlir::StringAttr name;
  ConcreteTypeKind kind = ConcreteTypeKind::Bits;
  mlir::Type resolvedType;
  uint64_t packedWidth = 0;
  llvm::SmallVector<uint64_t, 2> shape;
  std::string canonicalForm;

  bool operator==(const ConcreteTypeBinding &other) const {
    return name == other.name && canonicalForm == other.canonicalForm;
  }
};

struct ConcreteModuleKey {
  ConcreteModuleDefinition definition;
  llvm::SmallVector<ConcreteIntegerBinding, 2> integers;
  llvm::SmallVector<ConcreteTypeBinding, 2> types;

  bool operator==(const ConcreteModuleKey &other) const {
    return definition == other.definition && integers == other.integers &&
           types == other.types;
  }
};

struct ConcreteTypePlan {
  struct PackedField {
    llvm::SmallVector<std::string, 2> path;
    uint64_t low = 0;
    uint64_t width = 0;
  };
  mlir::Type type;
  uint64_t packedWidth = 0;
  enum class RuntimeKind {
    Bits,
    Table,
    Struct,
    Enum
  } runtimeKind = RuntimeKind::Bits;
  std::string cppTypeName;
  llvm::SmallVector<uint64_t, 2> shape;
  llvm::SmallVector<PackedField, 2> fields;
};

using ConcreteValueId = unsigned;

enum class ConcreteValueKind { ModuleInput, OperationResult, TableElement };

// One concrete SSA value owned by a module specialization. The numeric ID and
// C++ name are stable within the specialization and are the only value
// identity the emitter needs. `source` is retained for diagnostics and plan
// construction; it is not an invitation to re-resolve the value's semantics.
struct ConcreteValuePlan {
  ConcreteValueId id = 0;
  ConcreteValueKind kind = ConcreteValueKind::ModuleInput;
  ConcreteTypePlan type;
  std::string cppName;
  // A TableElement is an aliasing view of one lane of a whole-table value.
  // Once the parent is available, this view is available without emitted
  // work. The reverse edge is represented by an AvailabilityBarrier after
  // every lane writer has completed.
  std::optional<ConcreteValueId> availabilityParent;
  std::optional<uint64_t> tableLane;
};

struct ConcretePortPlan {
  mlir::StringAttr name;
  ConcreteTypePlan type;
  ConcreteValueId value = 0;
  std::string cppName;
};

struct ConcretePortBinding {
  unsigned port = 0;
  ConcreteValueId value = 0;
  std::string cppName;
};

enum class ConcreteChildKind { Module, Primitive, Queue };

struct ConcretePrimitiveConfig {
  mlir::StringAttr kind;
  llvm::SmallVector<ConcreteIntegerBinding, 2> integers;
  llvm::SmallVector<ConcreteTypeBinding, 2> types;
  std::optional<ac::ResolvedQueueConfig> queue;
  // Fully selected runtime surface.  The emitter must not recover a runtime
  // class or pin schema from the primitive kind or the originating IR.
  std::string runtimeCppType;
  std::string runtimeHeader;
  llvm::SmallVector<std::string, 8> inputMembers;
  llvm::SmallVector<std::string, 2> outputMembers;
  // One flag per output member. A temporal output is a view of committed
  // state and can be consumed before this primitive samples its next-state
  // inputs in Work().
  llvm::SmallVector<uint8_t, 2> outputIsTemporal;
  // For each non-temporal output, input pin ordinals that must be driven
  // before the runtime primitive can refresh that pure output. Temporal
  // outputs ignore this list because they are committed-state views.
  llvm::SmallVector<llvm::SmallVector<unsigned, 1>, 2>
      outputRefreshInputs;
  bool hasPureOutputRefresh = false;
  bool hasWork = false;
};

struct ConcreteChildPlan {
  ConcreteChildKind kind = ConcreteChildKind::Module;
  mlir::Operation *occurrence = nullptr;
  mlir::StringAttr instanceName;
  std::string cppName;
  llvm::SmallVector<ConcretePortBinding, 4> inputs;
  llvm::SmallVector<ConcretePortBinding, 2> outputs;
  std::optional<ConcreteModuleKey> calleeKey;
  std::optional<unsigned> moduleIndex;
  mlir::StringAttr primitiveKind;
  std::optional<ac::ResolvedQueueConfig> queue;
  std::optional<ConcretePrimitiveConfig> primitive;
  // When present, this child is one lane of an ac.collection. Its input and
  // output bindings reference whole table values; the renderer accesses the
  // per-lane element via wire<table<...>>::element(lane).
  std::optional<uint64_t> laneIndex;
};

enum class ConcreteScheduleStepKind {
  ComputeValue,
  AvailabilityBarrier,
  DriveChildInput,
  InvokeChildWork,
  ReadChildOutput,
  ReadChildOutputField,
  DrivePrimitiveInput,
  InvokePrimitiveWork,
  ReadPrimitiveOutput,
  MaterializeOutput
};

enum class ConcreteComputeKind {
  BitsConstant,
  BitsConstant4,
  BitsUnary,
  BitsBinary,
  BitsCompare,
  BitsSelect,
  BitsConcat,
  BitsExtract,
  BitsResize,
  PackedIdentity,
  EnumFromBits,
  TableCreate,
  TableSplat,
  TableView,
  TableIndex,
  TableGet
  ,TableMatch,
  TableChoose,
  TableFold,
  ValueMerge
};

struct ConcreteComputePlan {
  ConcreteComputeKind kind = ConcreteComputeKind::BitsConstant;
  std::string opcode;
  llvm::SmallVector<uint64_t, 2> constantWords;
  llvm::SmallVector<llvm::SmallVector<uint64_t, 2>, 4> enumCodes;
  std::optional<uint64_t> immediate;
  llvm::SmallVector<uint64_t, 0> laneMap;
  // Exact four-state planes. For BitsConstant4, constantWords is the value
  // plane and both masks have the same word count. A known mask bit selects
  // 0/1 from constantWords; otherwise zWords distinguishes Z from X.
  llvm::SmallVector<uint64_t, 2> knownWords;
  llvm::SmallVector<uint64_t, 2> zWords;
};

struct ConcreteScheduleStep {
  ConcreteScheduleStepKind kind = ConcreteScheduleStepKind::ComputeValue;
  // Retained for opcode/configuration lookup and diagnostics. Value wiring is
  // fully represented by inputs/outputs and must not be rediscovered from it.
  mlir::Operation *operation = nullptr;
  std::optional<unsigned> childIndex;
  std::optional<unsigned> portIndex;
  // A field-level child read extracts this packed slice after all listed
  // child input ports have been driven.  These records are produced from the
  // verified HWIR output dependency contract, not rediscovered by rendering.
  std::optional<uint64_t> fieldLow;
  llvm::SmallVector<unsigned, 2> refreshInputPorts;
  llvm::SmallVector<ConcreteValueId, 4> inputs;
  llvm::SmallVector<ConcreteValueId, 2> outputs;
  std::optional<ConcreteComputePlan> compute;
};

struct ConcreteModuleSchedule {
  llvm::SmallVector<ConcreteScheduleStep, 0> steps;
};

enum class ConcreteObservationKind { Event, Gauge };

// One definition-local staging slot. A source observation with several values
// owns one slot per value; literal-only events retain one path-backed slot.
struct ConcreteObservationSitePlan {
  ConcreteValueId path = 0;
  ConcreteValueId value = 0;
  bool carriesValue = false;
  ConcreteObservationKind kind = ConcreteObservationKind::Event;
  uint32_t groupIndex = 0;
  uint32_t groupSize = 1;
  uint64_t payloadWidth = 0;
  uint64_t registrationKey = 0;
  uint64_t siteKey = 0;
  std::string kindName;
  std::string registrationJson;
  std::string siteJson;
  std::string specJson;
  std::string reportName;
  llvm::SmallVector<std::string, 2> logicalOwnerSuffix;
};

enum class ConcreteLifecycleStepKind {
  InvokeRootWork,
  WholeSystemCheck,
  InvokeRootXfer,
  InvokeRootDiscard,
  InvokeRootReset,
  InvokeChildXfer,
  InvokePrimitiveXfer,
  InvokeChildDiscard,
  InvokePrimitiveDiscard,
  InvokeChildReset,
  InvokePrimitiveReset
};

// Lifecycle calls are separated by phase because Check chooses exactly one of
// Xfer and Discard. moduleIndex identifies the module whose method contains a
// child call; childIndex is present only for module-local calls.
struct ConcreteLifecycleStep {
  ConcreteLifecycleStepKind kind = ConcreteLifecycleStepKind::WholeSystemCheck;
  unsigned moduleIndex = 0;
  std::optional<unsigned> childIndex;
};

struct ConcreteLifecyclePlan {
  llvm::SmallVector<ConcreteLifecycleStep, 1> work;
  llvm::SmallVector<ConcreteLifecycleStep, 1> check;
  llvm::SmallVector<ConcreteLifecycleStep, 0> xfer;
  llvm::SmallVector<ConcreteLifecycleStep, 0> discard;
  llvm::SmallVector<ConcreteLifecycleStep, 0> reset;
};

struct ConcreteModulePlan {
  ConcreteModuleKey key;
  ac::HardwareBindings bindings;
  std::string cppName;
  llvm::SmallVector<ConcretePortPlan, 0> inputs;
  llvm::SmallVector<ConcretePortPlan, 0> outputs;
  llvm::SmallVector<ConcreteValuePlan, 0> values;
  // Child plans retain bindings and queue configuration, so they are large
  // enough that SmallVector must not guess an inline capacity.
  llvm::SmallVector<ConcreteChildPlan, 2> children;
  ac::ModuleAnalysis schedule;
  ConcreteModuleSchedule concreteSchedule;
  // Pure, non-sampling preparation run at Reset: constants and module inputs
  // are propagated to child reset/initializer pins without invoking Work or
  // staging retained effects.
  ConcreteModuleSchedule resetPreparation;
  // State-only projection run after child Xfer. It keeps generated module
  // output pins that are temporal cuts synchronized with committed state, so
  // a parent can safely read them before invoking this module's next Work().
  ConcreteModuleSchedule temporalRefresh;
  // Pure combinational projection of the current module inputs and committed
  // child state. It never invokes Work and therefore cannot stage next state,
  // checks, clocks or observations.
  ConcreteModuleSchedule pureOutputRefresh;
  llvm::SmallVector<uint8_t, 2> outputIsTemporal;
  ConcreteLifecyclePlan lifecycle;
  llvm::SmallVector<ConcreteObservationSitePlan, 0> observationSites;
  uint64_t observationSubtreeWidth = 0;
  // Fully materialized HWIR RTL boundary. Renderers must not re-read module
  // attributes to decide public ports or clock/reset ownership.
  unsigned publicInputCount = 0;
  unsigned publicOutputCount = 0;
  llvm::SmallVector<uint64_t, 2> rootInputDefaults;
  std::optional<unsigned> clockInput;
  std::optional<unsigned> resetInput;
  bool closedSystem = false;
};

// One name/value record in the specialization manifest. Names follow the
// declared parameter/type-parameter schema order; values are the canonical
// concrete spellings the specialization key is built from.
struct ConcreteManifestArgument {
  std::string name;
  std::string value;
};

// Reconstruction record tying one concrete specialization (its stable C++
// class name) back to the HWIR definition and the concrete arguments that
// selected it. This is the specialization source map: the plan owns the data
// and the emitter only serializes it. `definition` is the module symbol, e.g.
// "@pkg.Adder" when qualified, or "@top" for an unqualified fixture.
struct ConcreteModuleManifest {
  std::string cppName;
  std::string definition;
  llvm::SmallVector<ConcreteManifestArgument, 2> integerArguments;
  llvm::SmallVector<ConcreteManifestArgument, 2> typeArguments;
};

struct ConcreteNamedTypePlan {
  ConcreteTypeKind kind = ConcreteTypeKind::Struct;
  std::string cppQualifiedName;
  uint64_t packedWidth = 0;
};

struct ConcreteCheckPlan {
  unsigned moduleIndex = 0;
  ConcreteValueId condition = 0;
  ConcreteValueId path = 0;
  std::string kind;
  llvm::SmallVector<ConcreteValueId, 2> sampling;
  std::string message;
  std::string locationJson;
  std::string checkIdJson;
};

// One globally addressed Runtime descriptor for one concrete module
// occurrence. It points back to a definition-local staging site without
// retaining the originating MLIR operation.
struct ConcreteObservationPlan {
  uint32_t stableOrdinal = 0;
  uint64_t ownerKey = 0;
  uint64_t registrationKey = 0;
  uint64_t siteKey = 0;
  ConcreteObservationKind kind = ConcreteObservationKind::Event;
  uint32_t groupIndex = 0;
  uint32_t groupSize = 1;
  uint64_t payloadWidth = 0;
  unsigned moduleIndex = 0;
  unsigned localSiteIndex = 0;
  // Physical module occurrence that owns the definition-local staging slot.
  // This can differ from `instancePath` when analysis assigns the observation
  // to a logical owner below or outside that staging occurrence.
  std::string physicalInstancePath;
  // Logical owner path published in observation metadata.
  std::string instancePath;
  std::string kindName;
  std::string registrationJson;
  std::string siteJson;
  std::string specJson;
  bool carriesValue = false;
  std::string reportName;
};

struct ConcreteDependencyNode {
  unsigned moduleIndex = 0;
  mlir::Operation *occurrence = nullptr;
};

struct ConcreteDependencyEdge {
  unsigned fromNode = 0;
  unsigned toNode = 0;
};

// Closure-level SSA dependency graph. Nodes are operations in concrete module
// schedules; moduleIndex preserves which specialization owns each operation.
// The graph is a codegen validation artifact, not a second semantic IR.
struct ConcreteDependencyGraph {
  llvm::SmallVector<ConcreteDependencyNode, 0> nodes;
  llvm::SmallVector<ConcreteDependencyEdge, 0> edges;
};

struct ConcreteSourceUnitPlan {
  mlir::DictionaryAttr sourceOwner;
  llvm::SmallVector<unsigned, 0> moduleIndices;
  llvm::SmallVector<unsigned, 0> dependencies;
};

// A source @system is externally closed except for its compiler-owned clock
// and reset pins. The plan records their concrete root-port ordinals so the
// runner renderer never re-queries HWIR attributes.
struct ConcreteSystemPlan {
  unsigned clockInput = 0;
  unsigned resetInput = 0;
};

// Backend-owned, immutable-after-construction view of one fully resolved root
// specialization closure. It references verified HWIR operations and is not a
// serializable IR or an independent source of hardware semantics.
struct ConcreteDesignPlan {
  mlir::ModuleOp package;
  unsigned rootModule = 0;
  ConcreteModuleKey root;
  // Module plans include the analyzed schedule and should always live out of
  // line; an explicit zero avoids embedding a very large first element.
  llvm::SmallVector<ConcreteModulePlan, 0> modules;
  ConcreteDependencyGraph dependencies;
  // Root-level protocol: Work, one whole-system Check, then either Xfer or
  // Discard. Per-module plans contain the recursive child lifecycle calls.
  ConcreteLifecyclePlan lifecycle;
  // Specialization source map: one entry per `modules` element in the same
  // order, reconstructing each emitted class from its concrete arguments.
  llvm::SmallVector<ConcreteModuleManifest, 0> manifest;
  llvm::SmallVector<ConcreteNamedTypePlan, 0> namedTypes;
  llvm::SmallVector<ConcreteSourceUnitPlan, 0> sourceUnits;
  llvm::SmallVector<ConcreteCheckPlan, 0> checks;
  llvm::SmallVector<ConcreteObservationPlan, 0> observations;
  std::optional<ConcreteSystemPlan> system;
  uint64_t observationEventCapacity = 0;
  // Number of concrete check occurrences in the rooted instance hierarchy.
  // This can exceed checks.size() when one specialization is instantiated
  // more than once; bundle verification must report the occurrence count.
  uint64_t sourceCheckCount = 0;
};

// Build the concrete specialization closure after structural-C++ admission.
// Every type, width, extent and occurrence binding retained by the result is
// concrete. Unsupported or unresolved input fails at its originating HWIR op.
mlir::FailureOr<ConcreteDesignPlan>
buildConcreteDesignPlan(mlir::ModuleOp package, ac::HardwareAnalysis &analysis);

// Checks only structural integrity of an already materialized codegen plan;
// it performs no HWIR analysis or semantic inference.
mlir::LogicalResult verifyConcreteDesignPlan(const ConcreteDesignPlan &plan);

} // namespace acir::compiler

#endif // PYCIRCUIT_CONCRETE_DESIGN_PLAN_H

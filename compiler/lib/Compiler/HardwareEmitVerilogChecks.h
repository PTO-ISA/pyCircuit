#ifndef PYCIRCUIT_HARDWARE_EMIT_VERILOG_CHECKS_H
#define PYCIRCUIT_HARDWARE_EMIT_VERILOG_CHECKS_H

#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace acir::compiler {
class HardwareEmitContext;
class RtlRenderSymbols;

// These indices are emission-local, never substitutes for original IR identity.
template <typename Tag> struct RtlId {
  uint64_t index;
  explicit constexpr RtlId(uint64_t index = 0) : index(index) {}
  constexpr bool operator==(RtlId other) const { return index == other.index; }
  constexpr bool operator!=(RtlId other) const { return !(*this == other); }
};
struct RtlNodeTag;
struct RtlFamilyTag;
struct RtlInstanceTag;
struct RtlSlotTag;
using RtlNodeId = RtlId<RtlNodeTag>;
using RtlFamilyId = RtlId<RtlFamilyTag>;
using RtlInstanceId = RtlId<RtlInstanceTag>;
using RtlSlotId = RtlId<RtlSlotTag>;

enum class RtlScopeKind { Family, HardwareRoot, SimulationRoot };
struct RtlScope {
  RtlScopeKind kind;
  ac::ModuleOp definition;
  bool operator==(const RtlScope &other) const {
    return kind == other.kind && definition == other.definition;
  }
  bool operator!=(const RtlScope &other) const { return !(*this == other); }
};
enum class RtlSort { Payload, FourStateBit, Phase3, StaticBit };
struct RtlType {
  RtlSort sort;
  // Present exactly for Payload; graph verification checks this invariant.
  std::optional<mlir::Type> payloadType;
};
enum class RtlPrivatePort {
  Phase,
  Permit,
  InheritedReset,
  SubtreeSourceError,
  SubtreePrimitiveError
};
enum class RtlPrivateParameter { Managed, ContextBound };
enum class RtlPhase : uint8_t {
  Idle0 = 0,
  Prepare1 = 1,
  Commit2 = 2,
  Discard3 = 3,
  ResetPrepare4 = 4
};
enum class RtlDirection { Input, Output };
struct RtlSourcePort {
  RtlDirection direction;
  uint64_t ordinal;
};
using RtlConnectionDestination = std::variant<RtlSourcePort, RtlPrivatePort>;
struct RtlConnection {
  RtlConnectionDestination destination;
  RtlNodeId source;
};
struct RtlPrivateParameterConnection {
  RtlPrivateParameter destination;
  RtlNodeId value;
};
struct RtlSourceOutput {
  uint64_t ordinal;
};
using RtlOutput = std::variant<RtlSourceOutput, RtlPrivatePort>;

// The original shape controls row-major coordinates and reverse-packed slices.
// There is no source elaboration or expression text in this lane binder.
struct RtlLaneBinder {
  ac::CollectionOp allocation;
  mlir::ArrayAttr shape;
};
struct RtlSourceValue {
  mlir::Value value;
};
struct RtlRootInput {
  uint64_t inputOrdinal;
};
struct RtlPrivateInput {
  RtlPrivatePort role;
};
struct RtlPrivateParameterValue {
  RtlPrivateParameter role;
};
struct RtlConstantBit {
  bool value;
};
struct RtlConstantPhase {
  RtlPhase value;
};
struct RtlStaticBitConstant {
  bool value;
};
struct RtlIdentity {
  RtlNodeId value;
};
struct RtlBitNot {
  RtlNodeId value;
};
struct RtlBitAnd {
  llvm::SmallVector<RtlNodeId> operands;
};
struct RtlBitOr {
  llvm::SmallVector<RtlNodeId> operands;
};
struct RtlKnownZero {
  RtlNodeId value;
};
struct RtlStaticSelect {
  RtlNodeId condition;
  RtlNodeId trueValue;
  RtlNodeId falseValue;
};
struct RtlPackedLane {
  RtlNodeId source;
  RtlLaneBinder lane;
};
struct RtlChildOutput {
  RtlInstanceId instance;
  RtlOutput output;
};
struct RtlSnapshotValue {
  RtlSlotId slot;
};
struct RtlOccurrenceValue {
  uint64_t ownerOccurrence;
  mlir::Value value;
};
struct RtlOccurrenceInput {
  uint64_t ownerOccurrence;
  uint64_t inputOrdinal;
};
using RtlNodeValue =
    std::variant<RtlSourceValue, RtlRootInput, RtlPrivateInput,
                 RtlPrivateParameterValue, RtlConstantBit, RtlConstantPhase,
                 RtlStaticBitConstant, RtlIdentity, RtlBitNot, RtlBitAnd,
                 RtlBitOr, RtlKnownZero, RtlStaticSelect, RtlPackedLane,
                 RtlChildOutput, RtlSnapshotValue, RtlOccurrenceValue,
                 RtlOccurrenceInput>;
struct RtlNode {
  RtlScope scope;
  RtlType type;
  RtlNodeValue value;
};

struct RtlPrivatePortDescriptor {
  RtlPrivatePort role;
  RtlDirection direction;
  RtlType type;
  std::optional<RtlNodeId> defaultValue;
};
struct RtlPrivateParameterDescriptor {
  RtlPrivateParameter role;
  RtlNodeId defaultValue;
};
struct RtlContextGuard {
  RtlNodeId contextBound;
  // Legalized unresolved diagnostic-module spelling, not a semantic edge.
  std::string diagnosticModule;
};
struct RtlLocalCheck {
  uint64_t bindingIndex;
  RtlNodeId condition;
  RtlNodeId path;
  RtlNodeId reset;
  RtlNodeId error;
};
struct RtlFamilyDescriptor {
  ac::ModuleOp definition;
  std::string sourceOutputGroup;
  mlir::FunctionType signature;
  mlir::ArrayAttr inputNames;
  mlir::ArrayAttr outputNames;
  llvm::SmallVector<RtlLocalCheck> localChecks;
  // Only source instances may occur here, never synthetic root entries.
  llvm::SmallVector<RtlInstanceId> allocations;
  llvm::SmallVector<RtlPrivatePortDescriptor> privatePorts;
  llvm::SmallVector<RtlPrivateParameterDescriptor> privateParameters;
  RtlNodeId effectiveReset;
  RtlNodeId effectivePermit;
  RtlNodeId localSourceError;
  RtlNodeId subtreeSourceError;
  RtlNodeId subtreePrimitiveError;
  std::optional<RtlContextGuard> contextGuard;
};
enum class RtlPrimitiveKind { Dff, Dffe, Fifo, ByteMem, SyncMem, SyncMemDp };
struct RtlFamilyCallee {
  RtlFamilyId family;
};
struct RtlPrimitiveCallee {
  RtlPrimitiveKind kind;
  // QueueOp owns a fixed FIFO ABI directly and has no import declaration.
  std::optional<ac::ModuleImportOp> declaration;
};
using RtlCallee = std::variant<RtlFamilyCallee, RtlPrimitiveCallee>;
struct RtlOrdinaryInstance {};
using RtlInstanceForm = std::variant<RtlOrdinaryInstance, RtlLaneBinder>;
struct RtlSourceInstanceDescriptor {
  RtlFamilyId parent;
  mlir::Operation *allocation;
  RtlCallee callee;
  mlir::ArrayAttr parameters;
  mlir::ArrayAttr typeArguments;
  // Retains all original queue attributes as well as instance metadata.
  mlir::DictionaryAttr allocationAttributes;
  RtlInstanceForm form;
  llvm::SmallVector<RtlConnection> sourceConnections;
  llvm::SmallVector<RtlConnection> privateConnections;
  llvm::SmallVector<RtlPrivateParameterConnection> privateParameters;
};
enum class RtlRootKind { Hardware, Simulation };
struct RtlRootOutputConnection {
  uint64_t outputOrdinal;
  RtlNodeId value;
};
struct RtlRootEntryDescriptor {
  RtlRootKind kind;
  ac::SystemOp system;
  mlir::DictionaryAttr entry;
  RtlFamilyId callee;
  ac::HardwareBindings bindings;
  mlir::FunctionType resolvedSignature;
  llvm::SmallVector<RtlConnection> sourceConnections;
  llvm::SmallVector<RtlConnection> privateConnections;
  llvm::SmallVector<RtlPrivateParameterConnection> privateParameters;
  llvm::SmallVector<RtlRootOutputConnection> wrapperOutputs;
};
using RtlInstanceDescriptor =
    std::variant<RtlSourceInstanceDescriptor, RtlRootEntryDescriptor>;

// Metadata storage has no node sort and cannot be read by SnapshotValue.
// Its encoding belongs to the closed lifecycle actions, not source payloads.
enum class RtlSlotStorage {
  Lifecycle,
  Epoch,
  FailurePhase,
  FailureKind,
  FailureCheckIndex
};
enum class RtlValueSlotRole {
  Frame,
  PendingOutput,
  PublishedOutput,
  Condition,
  Path,
  Reset,
  Completion,
  Phase,
  FrozenPermission,
  SampleAvailable
};
struct RtlValueSlot {
  RtlType type;
  RtlValueSlotRole role;
};
struct RtlMetadataSlot {
  RtlSlotStorage storage;
};
struct RtlSlot {
  RtlScope scope;
  std::variant<RtlValueSlot, RtlMetadataSlot> storage;
};
struct RtlFailureSlots {
  RtlSlotId phase;
  RtlSlotId kind;
  RtlSlotId checkIndex;
};
struct RtlLifecycleSlots {
  RtlSlotId lifecycle;
  RtlSlotId epoch;
  RtlSlotId sampleAvailable;
  RtlFailureSlots failure;
};
struct RtlFrameCapture {
  uint64_t inputOrdinal;
  RtlNodeId externalInput;
  RtlSlotId frame;
};
struct RtlResetCapture {
  uint64_t ownerOccurrence;
  uint64_t inputOrdinal;
  RtlNodeId input;
  RtlSlotId value;
  RtlSlotId completion;
};
struct RtlCheckCapture {
  uint64_t resolvedCheckIndex;
  RtlNodeId condition;
  RtlNodeId path;
  RtlSlotId conditionSlot;
  RtlSlotId pathSlot;
  RtlSlotId completion;
  std::optional<uint64_t> resetCaptureIndex;
};
struct RtlOutputCapture {
  uint64_t outputOrdinal;
  RtlNodeId source;
  RtlSlotId pending;
  RtlSlotId published;
  RtlSlotId completion;
};

// Capture actions copy the entire referenced tuple before writing completion.
// Indices refer to the corresponding capture vector and are independently
// bounds/provenance checked; no action supplies an asserted coverage flag.
struct RtlStepEntry {
  RtlLifecycleSlots slots;
};
struct RtlFreezeInputs {
  llvm::SmallVector<uint64_t> frameCaptureIndices;
};
struct RtlClearPending {
  llvm::SmallVector<RtlSlotId> completions;
  RtlFailureSlots failure;
};
struct RtlDelay1 {};
struct RtlSetPhase {
  RtlSlotId slot;
  RtlPhase phase;
};
struct RtlCaptureReset {
  uint64_t captureIndex;
};
struct RtlCaptureCheck {
  uint64_t captureIndex;
};
struct RtlCaptureOutput {
  uint64_t captureIndex;
};
struct RtlValidatePrimitivePreparation {
  RtlNodeId primitiveError;
  RtlFailureSlots failure;
};
struct RtlValidateSnapshotCompletion {
  llvm::SmallVector<RtlSlotId> completions;
  RtlFailureSlots failure;
};
struct RtlSelectFirstSourceFailure {
  llvm::SmallVector<uint64_t> checkCaptureIndices;
  RtlFailureSlots failure;
};
enum class RtlPermissionKind { ValidatedAttempt, HostReset };
struct RtlFreezePermission {
  RtlSlotId permission;
  RtlPermissionKind kind;
  RtlFailureSlots failure;
};
struct RtlCommitOrDiscard {
  RtlSlotId phase;
  RtlSlotId permission;
};
struct RtlFinishStep {
  RtlLifecycleSlots slots;
  llvm::SmallVector<uint64_t> outputCaptureIndices;
};
struct RtlFinishReset {
  RtlLifecycleSlots slots;
};
using RtlAction =
    std::variant<RtlStepEntry, RtlFreezeInputs, RtlClearPending, RtlDelay1,
                 RtlSetPhase, RtlCaptureReset, RtlCaptureCheck,
                 RtlCaptureOutput, RtlValidatePrimitivePreparation,
                 RtlValidateSnapshotCompletion, RtlSelectFirstSourceFailure,
                 RtlFreezePermission, RtlCommitOrDiscard, RtlFinishStep,
                 RtlFinishReset>;
enum class RtlSimulationAdapter { AutonomousRoot, CheckedSimulationRoot };

struct HardwareVerilogProjection {
  llvm::SmallVector<RtlNode> nodes;
  llvm::SmallVector<RtlFamilyDescriptor> families;
  llvm::SmallVector<RtlInstanceDescriptor> instances;
  llvm::SmallVector<RtlSlot> slots;
  RtlInstanceId hardwareRoot;
  std::optional<RtlInstanceId> simulationRoot;
  llvm::SmallVector<RtlFrameCapture> frameCaptures;
  llvm::SmallVector<RtlResetCapture> resetCaptures;
  llvm::SmallVector<RtlCheckCapture> checkCaptures;
  llvm::SmallVector<RtlOutputCapture> outputCaptures;
  llvm::SmallVector<RtlAction> stepActions;
  llvm::SmallVector<RtlAction> resetActions;
  RtlSimulationAdapter adapter;

  mlir::FailureOr<const RtlNode *> node(RtlNodeId id) const {
    if (id.index >= nodes.size())
      return mlir::failure();
    return &nodes[static_cast<size_t>(id.index)];
  }
  mlir::FailureOr<const RtlFamilyDescriptor *> family(RtlFamilyId id) const {
    if (id.index >= families.size())
      return mlir::failure();
    return &families[static_cast<size_t>(id.index)];
  }
  mlir::FailureOr<const RtlInstanceDescriptor *>
  instance(RtlInstanceId id) const {
    if (id.index >= instances.size())
      return mlir::failure();
    return &instances[static_cast<size_t>(id.index)];
  }
  mlir::FailureOr<const RtlSlot *> slot(RtlSlotId id) const {
    if (id.index >= slots.size())
      return mlir::failure();
    return &slots[static_cast<size_t>(id.index)];
  }
};

mlir::FailureOr<HardwareVerilogProjection>
buildHardwareVerilogProjection(HardwareEmitContext &context);
mlir::LogicalResult
verifyHardwareCheckProjection(const ac::HardwareAnalysis &analysis,
                              const ac::HardwareSourceCheckPlan &plan,
                              const HardwareVerilogProjection &projection);
mlir::LogicalResult emitHardwareVerilogDefinition(
    HardwareEmitContext &context, ac::ModuleOp definition,
    const HardwareVerilogProjection &projection, RtlRenderSymbols &symbols,
    llvm::raw_ostream &out);
mlir::LogicalResult emitHardwareVerilogRoots(
    HardwareEmitContext &context, const HardwareVerilogProjection &projection,
    const RtlRenderSymbols &symbols, llvm::raw_ostream &out);
mlir::LogicalResult emitHardwareVerilogSimulationAdapter(
    const HardwareVerilogProjection &projection, llvm::raw_ostream &out);
} // namespace acir::compiler
#endif

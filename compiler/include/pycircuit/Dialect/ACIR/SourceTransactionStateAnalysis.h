#ifndef PYCIRCUIT_DIALECT_ACIR_SOURCETRANSACTIONSTATEANALYSIS_H
#define PYCIRCUIT_DIALECT_ACIR_SOURCETRANSACTIONSTATEANALYSIS_H

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>
#include <optional>
#include <variant>

namespace acir::ac {

// These indices refer only to vectors in one TransactionStatePlan. They are
// analysis identities, not hardware values or published source identities.
using LexicalInstanceId = uint64_t;
using InvocationFrameId = uint64_t;
using BoundValueId = uint64_t;
using StateOwnerId = uint64_t;
using StateVersionId = uint64_t;
using StateReadId = uint64_t;
using StateWriteId = uint64_t;
using GuardBoundaryId = uint64_t;
using AdmissionControlId = uint64_t;
using SelectionBoundaryId = uint64_t;
using CallBindingId = uint64_t;
using BodyCheckId = uint64_t;
using ObservationId = uint64_t;
using BranchId = uint64_t;
using BranchResultId = uint64_t;
using StateJoinId = uint64_t;
/// Number of completed admission controls; zero is unconditional.
using AdmissionPrefix = uint64_t;

/// Full source-region ancestry, including branches with no admission control.
struct TransactionSourcePath {
  std::optional<BranchId> branch;
  bool whenTrue = false;
};
struct TransactionPoint {
  AdmissionPrefix prefix = 0;
  TransactionSourcePath path;
};
struct CallableGuardControl {
  GuardBoundaryId guard = 0;
};
struct BranchSelectControl {
  SelectionBoundaryId selection = 0;
};
using AdmissionControlBoundary =
    std::variant<CallableGuardControl, BranchSelectControl>;
struct AdmissionControl {
  AdmissionControlBoundary boundary;
  TransactionSourcePath path;
};
struct SelectionBoundary {
  BranchId branch = 0;
  AdmissionControlId control = 0;
};

/// A module instance relative to the transaction's lexical root. The root has
/// no parent/allocation and an empty occurrence path. Child paths contain the
/// canonical declaration occurrences, never source instance-name spellings.
struct TransactionLexicalInstance {
  ModuleOp definition;
  std::optional<LexicalInstanceId> parent;
  InstanceOp allocation;
  mlir::ArrayAttr occurrencePath;
  HardwareBindings bindings;
};

/// An original SSA value in a particular instance and invocation. Pure payload
/// computations remain actual IR, not evaluated or copied into this plan.
/// Module-body values use the plan's rootFrame as their canonical frame, even
/// in child instances: anchor selects that module instance's bindings. Callable
/// region values use their owning invocation frame. A capture's module-body
/// value never takes the current caller/Method frame as a dynamic lexical
/// scope.
struct BoundValueRef {
  mlir::Value value;
  LexicalInstanceId anchor = 0;
  InvocationFrameId frame = 0;
  TransactionPoint validity;
};

/// Callable block arguments bind separately in guard and body regions. Entries
/// are in formal ordinal order: explicit inputs followed by lexical captures.
/// Actuals refer to values in their original instance/frame context.
struct TransactionValueBinding {
  mlir::BlockArgument formal;
  BoundValueId actual = 0;
};

struct InvocationFrame {
  SourceTransactionOp transaction;
  std::optional<InvocationFrameId> parent;
  SourceMethodCallOp call;
  SourceMethodOp method;
  mlir::DictionaryAttr callOccurrence;
  mlir::DictionaryAttr receiver;
  LexicalInstanceId anchor = 0;
  HardwareBindings bindings;
  llvm::SmallVector<TransactionValueBinding> guardArguments;
  llvm::SmallVector<TransactionValueBinding> bodyArguments;
  GuardBoundaryId guard = 0;
  TransactionPoint entry, exit;
};

/// Only callable formal/capture binding and verified Instance input ordinals
/// normalize these endpoints. Equality includes the lexical instance anchor;
/// equal constants or expressions at different SSA sites are not equivalent.
struct DomainEndpoint {
  mlir::Value value;
  LexicalInstanceId anchor = 0;

  bool operator==(const DomainEndpoint &other) const {
    return value == other.value && anchor == other.anchor;
  }
};

/// Clock and reset identity are paired and ordered. This records a sufficient
/// same-domain proof, not runtime knownness or a sampled-edge proof.
struct TransactionDomain {
  DomainEndpoint clock;
  DomainEndpoint reset;

  bool operator==(const TransactionDomain &other) const {
    return clock == other.clock && reset == other.reset;
  }
};

/// Owner identity includes the instance's canonical occurrence path and the
/// state declaration occurrence. HardwareBindings.owner alone is insufficient.
struct TransactionStateOwner {
  LexicalInstanceId anchor = 0;
  mlir::ArrayAttr instancePath;
  SourceStateOp declaration;
  mlir::DictionaryAttr occurrence;
  mlir::Type payloadType;
  TransactionDomain domain;
  BoundValueId oldQ = 0;
};

enum class StateVersionKind { OldQ, Write, Join };

/// Each owner's ordinal zero is its Q0 version, with no predecessor/write and
/// value equal to the owner's explicit old-Q value. A write version retains
/// its statement-time RHS. A join has its entry predecessor and join reference,
/// with no source RHS; its data and write intent select the two arm versions.
struct StateVersion {
  SourceTransactionOp transaction;
  StateOwnerId owner = 0;
  uint64_t ordinal = 0;
  StateVersionKind kind = StateVersionKind::OldQ;
  std::optional<StateVersionId> predecessor;
  std::optional<StateWriteId> write;
  std::optional<StateJoinId> join;
  std::optional<BoundValueId> value;
  mlir::Type payloadType;
  TransactionPoint validity;

  std::optional<BoundValueId> getSourceValue() const { return value; }
  std::optional<StateJoinId> getJoinId() const { return join; }
};

struct StateReadBinding {
  SourceStateReadOp operation;
  InvocationFrameId frame = 0;
  StateOwnerId owner = 0;
  StateVersionId version = 0;
  BoundValueId result = 0;
  BoundValueId authoredPath = 0;
  mlir::ArrayAttr selectors;
  llvm::SmallVector<BoundValueId> indices;
  mlir::DictionaryAttr occurrence;
};

/// Identity writes are still intents with a distinct successor version.
struct StateWriteIntent {
  SourceStateWriteOp operation;
  InvocationFrameId frame = 0;
  StateOwnerId owner = 0;
  StateVersionId predecessor = 0;
  StateVersionId version = 0;
  BoundValueId value = 0;
  BoundValueId authoredPath = 0;
  mlir::ArrayAttr selectors;
  llvm::SmallVector<BoundValueId> indices;
  mlir::DictionaryAttr occurrence;
};

struct OwnerVersionBinding {
  StateOwnerId owner = 0;
  StateVersionId version = 0;
};

/// Structured source result, independent of either arm's original yield SSA.
struct TransactionBranchResult {
  BranchId branch;
  InvocationFrameId frame;
  uint64_t ordinal;
  BoundValueId result, whenTrue, whenFalse;
  mlir::Type payloadType;
  TransactionPoint validity;
};

struct TransactionStateJoin {
  BranchId branch;
  StateOwnerId owner;
  StateVersionId entry, whenTrue, whenFalse, result;
};

/// Both arms restore the same candidates while control prefixes advance.
struct TransactionBranch {
  SourceBranchOp operation;
  InvocationFrameId frame;
  BoundValueId condition;
  TransactionPoint entry;
  std::optional<SelectionBoundaryId> selection;
  AdmissionPrefix thenExitPrefix = 0, elseExitPrefix = 0;
  mlir::BoolAttr conditionFact;
  llvm::SmallVector<OwnerVersionBinding> entryVersions, thenVersions,
      elseVersions;
  llvm::SmallVector<StateJoinId> joins;
  llvm::SmallVector<BranchResultId> results;
  /// Frozen source provenance; borrowed operation handles grant no authority.
  mlir::DictionaryAttr occurrence, conditionLocation;
};

/// Ordered admission obligations retain known-false and unknown guards alike.
/// Entry versions are frozen before evaluating this guard. Its control index
/// records reachability order; later writes cannot change these entry versions.
/// A null predicateFact means unknown, not a successful admission proof.
/// entryVersions lists the owners read by this guard, not all known state. Each
/// listed version is its candidate at guard entry. An owner discovered lazily
/// after entry uses Q0 if it was absent from the entry candidate environment.
struct GuardBoundary {
  mlir::Operation *callable = nullptr;
  InvocationFrameId frame = 0;
  AdmissionControlId control = 0;
  TransactionDomain domain;
  llvm::SmallVector<OwnerVersionBinding> entryVersions;
  llvm::SmallVector<StateReadId> reads;
  BoundValueId predicate = 0;
  mlir::BoolAttr predicateFact;
  /// Exact declaring callable guard span, retained from verified source IR.
  mlir::DictionaryAttr location;
};

/// A call result keeps its original SSA identity and the ordinary body yield it
/// binds. This preserves saved values across subsequent candidate writes.
struct TransactionResultBinding {
  BoundValueId result = 0;
  BoundValueId yielded = 0;
};

/// Calls share the outer transaction's candidates and introduce no commit.
/// Captures come from the callee declaration's lexical instance, not the
/// caller.
/// Entry/exit versions list owners touched by this invocation, including nested
/// calls. They do not inventory all known state. Entry candidates are frozen at
/// CallEnter; a lazily discovered owner absent at entry uses Q0, even if its
/// first access later writes it. Exit versions reflect the completed body.
struct TransactionCallBinding {
  SourceMethodCallOp operation;
  InvocationFrameId caller = 0;
  InvocationFrameId callee = 0;
  BoundValueId authoredPath = 0;
  llvm::SmallVector<BoundValueId> arguments;
  llvm::SmallVector<BoundValueId> captures;
  llvm::SmallVector<OwnerVersionBinding> entryVersions;
  llvm::SmallVector<OwnerVersionBinding> exitVersions;
  llvm::SmallVector<TransactionResultBinding> results;
};

/// Statement-time check inputs in one invocation. The condition is retained
/// even when false or unknown; only the authored path requires a true proof.
/// A null message is absent, while a nonnull empty string is authored empty.
struct TransactionBodyCheck {
  SourceExpectOp operation;
  InvocationFrameId frame = 0;
  BoundValueId condition = 0;
  BoundValueId authoredPath = 0;
  mlir::DictionaryAttr occurrence;
  mlir::DictionaryAttr location;
  mlir::StringAttr message;
  TransactionPoint point;
};

/// Ordered statement-time payloads. The frame's anchor identifies the logical
/// receiver; its parent chain retains invocation history. Repeated expansions
/// keep separate records even when they borrow the same source operation.
/// Literal-only logs have no values; no emission sentinel belongs in this plan.
struct TransactionObservation {
  SourceObserveOp operation;
  InvocationFrameId frame = 0;
  BoundValueId authoredPath = 0;
  llvm::SmallVector<BoundValueId, 0> values;
  mlir::DictionaryAttr occurrence;
  mlir::DictionaryAttr location;
  mlir::StringAttr kind;
  mlir::DictionaryAttr spec;
  TransactionPoint point;
};

enum class TransactionStateEventKind {
  TransactionEnter,
  GuardEnter,
  Read,
  GuardExit,
  SelectionEnter,
  SelectionExit,
  Write,
  BodyCheck,
  Observe,
  CallEnter,
  CallExit,
  BranchEnter,
  ThenEnter,
  ThenExit,
  ElseEnter,
  ElseExit,
  StateJoin,
  BranchResult,
  BranchExit,
  TransactionExit
};

/// Vector order is semantic order, including repeated visits to the same op in
/// different invocation frames. Only the matching kind's record reference is
/// set; transaction boundaries carry a frame and no record reference.
/// CallEnter/CallExit use the caller frame; the call record identifies the
/// callee. Guard/read/write/body-effect events use the invocation that owns
/// their operation, and transaction boundaries use rootFrame.
struct TransactionStateEvent {
  TransactionStateEventKind kind = TransactionStateEventKind::TransactionEnter;
  InvocationFrameId frame = 0;
  std::optional<StateReadId> read;
  std::optional<StateWriteId> write;
  std::optional<GuardBoundaryId> guard;
  std::optional<CallBindingId> call;
  TransactionPoint point;
  std::optional<SelectionBoundaryId> selection;
  std::optional<BodyCheckId> bodyCheck;
  std::optional<ObservationId> observation;
  std::optional<BranchId> branch;
  std::optional<StateJoinId> join;
  std::optional<BranchResultId> branchResult;
};

/// A borrowed, transaction-relative version/use-def plan. Each transaction
/// starts from its own Q0 candidates. Indices always address this plan's
/// vectors. The analysis exposes this object only through a const view after
/// all package transactions succeed; its IR handles require reconstruction
/// after mutation.
struct TransactionStatePlan {
  SourceTransactionOp transaction;
  ModuleOp lexicalModule;
  TransactionDomain domain;
  InvocationFrameId rootFrame = 0;
  GuardBoundaryId guard = 0;
  llvm::SmallVector<TransactionLexicalInstance, 0> instances;
  llvm::SmallVector<InvocationFrame, 0> frames;
  llvm::SmallVector<BoundValueRef, 0> values;
  llvm::SmallVector<TransactionStateOwner, 0> owners;
  llvm::SmallVector<StateVersion, 0> versions;
  llvm::SmallVector<StateReadBinding, 0> reads;
  llvm::SmallVector<StateWriteIntent, 0> writes;
  llvm::SmallVector<GuardBoundary, 0> guards;
  llvm::SmallVector<AdmissionControl, 0> controls;
  llvm::SmallVector<SelectionBoundary, 0> selections;
  llvm::SmallVector<TransactionCallBinding, 0> calls;
  llvm::SmallVector<TransactionBodyCheck, 0> bodyChecks;
  llvm::SmallVector<TransactionObservation, 0> observations;
  llvm::SmallVector<TransactionBranch, 0> branches;
  llvm::SmallVector<TransactionStateJoin, 0> joins;
  llvm::SmallVector<TransactionBranchResult, 0> branchResults;
  llvm::SmallVector<TransactionStateEvent, 0> events;
  llvm::SmallVector<OwnerVersionBinding> finalVersions;
  llvm::SmallVector<TransactionResultBinding, 0> results;
  BoundValueId clockValue = 0;
  BoundValueId resetValue = 0;
};

/// Read-only analysis of every definition transaction in a builtin ModuleOp
/// package owning its closure. Native verification and Method summaries remain
/// authoritative. Whole-Bits effects retain source paths, ordered admission
/// controls and candidate joins. False and unknown obligations remain explicit.
/// Missing executable bodies, partial state and unproved domains fail closed.
/// Construction neither mutates IR nor executes hardware values. Publication is
/// atomic: any failure makes every lookup null and print fail without output.
class SourceTransactionStateAnalysis {
public:
  explicit SourceTransactionStateAnalysis(mlir::Operation *package);
  bool isValid() const { return valid; }
  const TransactionStatePlan *lookup(SourceTransactionOp transaction) const;

  /// Print previously charged plan records with stable source/frame/value
  /// identities, without pointer addresses, resolution or renewed proof work.
  mlir::LogicalResult print(llvm::raw_ostream &output) const;

private:
  llvm::DenseMap<mlir::Operation *, TransactionStatePlan> plans;
  llvm::SmallVector<mlir::Operation *> transactionOrder;
  bool valid = false;
};

} // namespace acir::ac

#endif

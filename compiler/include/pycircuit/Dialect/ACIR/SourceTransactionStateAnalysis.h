#ifndef PYCIRCUIT_DIALECT_ACIR_SOURCETRANSACTIONSTATEANALYSIS_H
#define PYCIRCUIT_DIALECT_ACIR_SOURCETRANSACTIONSTATEANALYSIS_H

#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>
#include <optional>

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
using CallBindingId = uint64_t;
/// Number of ordered guards required to admit: zero is unconditional.
using AdmissionPrefix = uint64_t;

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
  AdmissionPrefix requiredPrefix = 0;
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

enum class StateVersionKind { OldQ, Write };

/// Each owner's ordinal zero is its Q0 version, with no predecessor/write and
/// value equal to the owner's explicit old-Q value. A write version has both
/// predecessor and write references, and retains the bound assignment RHS.
struct StateVersion {
  SourceTransactionOp transaction;
  StateOwnerId owner = 0;
  uint64_t ordinal = 0;
  StateVersionKind kind = StateVersionKind::OldQ;
  std::optional<StateVersionId> predecessor;
  std::optional<StateWriteId> write;
  BoundValueId value = 0;
  mlir::Type payloadType;
  AdmissionPrefix requiredPrefix = 0;
};

struct StateReadBinding {
  SourceStateReadOp operation;
  InvocationFrameId frame = 0;
  StateOwnerId owner = 0;
  StateVersionId version = 0;
  BoundValueId result = 0;
  BoundValueId path = 0;
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
  BoundValueId path = 0;
  mlir::ArrayAttr selectors;
  llvm::SmallVector<BoundValueId> indices;
  mlir::DictionaryAttr occurrence;
};

struct OwnerVersionBinding {
  StateOwnerId owner = 0;
  StateVersionId version = 0;
};

/// Ordered admission obligations retain known-false and unknown guards alike.
/// Entry versions are frozen before evaluating this guard. predecessor records
/// reachability order; later body writes cannot affect these entry versions.
/// A null predicateFact means unknown, not a successful admission proof.
/// entryVersions lists the owners read by this guard, not all known state. Each
/// listed version is its candidate at guard entry. An owner discovered lazily
/// after entry uses Q0 if it was absent from the entry candidate environment.
struct GuardBoundary {
  mlir::Operation *callable = nullptr;
  InvocationFrameId frame = 0;
  std::optional<GuardBoundaryId> predecessor;
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
  BoundValueId path = 0;
  GuardBoundaryId guard = 0;
  llvm::SmallVector<BoundValueId> arguments;
  llvm::SmallVector<BoundValueId> captures;
  llvm::SmallVector<OwnerVersionBinding> entryVersions;
  llvm::SmallVector<OwnerVersionBinding> exitVersions;
  llvm::SmallVector<TransactionResultBinding> results;
};

enum class TransactionStateEventKind {
  TransactionEnter,
  GuardEnter,
  Read,
  GuardExit,
  Write,
  CallEnter,
  CallExit,
  TransactionExit
};

/// Vector order is semantic order, including repeated visits to the same op in
/// different invocation frames. Only the matching kind's record reference is
/// set; transaction boundaries carry a frame and no record reference.
/// CallEnter/CallExit use the caller frame; the call record identifies the
/// callee. Guard/read/write events use the invocation that owns their
/// operation, and transaction boundaries use rootFrame.
struct TransactionStateEvent {
  TransactionStateEventKind kind = TransactionStateEventKind::TransactionEnter;
  InvocationFrameId frame = 0;
  std::optional<StateReadId> read;
  std::optional<StateWriteId> write;
  std::optional<GuardBoundaryId> guard;
  std::optional<CallBindingId> call;
  AdmissionPrefix prefix = 0;
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
  llvm::SmallVector<TransactionCallBinding, 0> calls;
  llvm::SmallVector<TransactionStateEvent, 0> events;
  llvm::SmallVector<OwnerVersionBinding> finalVersions;
  llvm::SmallVector<TransactionResultBinding, 0> results;
  BoundValueId clockValue = 0;
  BoundValueId resetValue = 0;
};

/// Read-only analysis of every definition transaction in a builtin ModuleOp
/// package owning its closure. Native verification and Method summaries remain
/// authoritative. Only straight-line whole-owner effects on proven-true paths
/// are admitted; guard obligations may be false or unknown. Missing executable
/// Method bodies, selectors and unproved domains fail this analysis entrypoint.
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

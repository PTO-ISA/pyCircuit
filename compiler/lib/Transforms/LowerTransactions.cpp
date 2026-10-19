#include "../Dialect/ACIR/SourceFormalProposalProof.h"
#include "Dialect/ACIR/HardwareGrantProof.h"
#include "../Dialect/ACIR/SourceFormalCandidateSelection.h"
#include "LowerTransactions.h"
#include "TransactionValueAvailability.h"
#include "Dialect/ACIR/HardwareValueAvailability.h"
#include "Dialect/ACIR/HardwareQueueResources.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
#include "Dialect/ACIR/SourceTransactionSummary.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Transforms/Passes.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"
#include <functional>
#include <limits>
#include <map>

using namespace mlir;

namespace acir {
namespace {

using ac::detail::SourceCheckBudget;
using detail::DefinitionRouting;
using detail::MaterializedBranchResult;
using detail::MaterializedTransaction;
using detail::MaterializedTransactionLowering;
using detail::MaterializedVersion;
using detail::PreparedTransactionLowering;
using detail::Route;
using detail::RoutingKey;
using detail::QueueRoutingKey;
using detail::TransactionQueueProposal;
using detail::TransactionPreparation;
using detail::resourcePoint;
using detail::resourceOperation;
using detail::resourceFrame;
using detail::resourceOccurrence;
using detail::resourceLocation;

bool charge(SourceCheckBudget &budget, Operation *site, uint64_t units = 1) {
  return succeeded(budget.debit(units, site));
}
bool product(SourceCheckBudget &budget, Operation *site, uint64_t count,
             uint64_t units) {
  return charge(budget, site, SourceCheckBudget::multiply(count, units));
}

template <typename Vector>
bool prepayAppend(SourceCheckBudget &budget, Operation *site, Vector &vector) {
  if (!charge(budget, site))
    return false;
  if (vector.size() < vector.capacity())
    return true;
  uint64_t capacity = SourceCheckBudget::add(
      SourceCheckBudget::multiply(vector.capacity(), 2), 1);
  if (capacity > vector.max_size() || capacity <= vector.capacity())
    return charge(budget, site, std::numeric_limits<uint64_t>::max());
  auto bytes = SourceCheckBudget::multiply(capacity - vector.capacity(),
                                           sizeof(typename Vector::value_type));
  if (!charge(budget, site,
              SourceCheckBudget::add(SourceCheckBudget::bytes(bytes),
                                     vector.size())))
    return false;
  vector.reserve(capacity);
  return true;
}

// Metadata for materialization only. The original-source proof below derives
// its own traversal directly from original declarations and yields.
struct BranchJoinNode {
  Type type;
  StringAttr field;
  uint64_t parent = UINT64_MAX;
  SmallVector<uint64_t, 0> children;
  SmallVector<Value, 0> fields;
  Value whenTrue, whenFalse, joined;
  uint64_t enumWidth = 0;
  uint64_t ordinal = 0;
};
struct BranchJoinLayout {
  SmallVector<BranchJoinNode, 0> nodes;
  uint64_t operations = 0;
};

FailureOr<Type> branchResultType(Type type,
                                 const ac::HardwareAnalysis &hardware,
                                 const ac::HardwareBindings &bindings,
                                 Operation *site, SourceCheckBudget &budget) {
  SourceCheckBudget::RawDiagnosticScope rawDiagnostics(budget);
  return ac::detail::resolveSourceBranchResultType(
      type, hardware, bindings, site, [&](uint64_t work, uint64_t bytes) {
        return budget.debit(
            SourceCheckBudget::add(work,
                                   bytes == std::numeric_limits<uint64_t>::max()
                                       ? bytes
                                       : SourceCheckBudget::bytes(bytes)),
            site);
      });
}

// One owner/phase, immutable original IR and immutable actual bindings. Cache
// only the expensive source type admission; every generated SSA edge, arm and
// candidate operation is still reconstructed and checked at each occurrence.
class BranchTypeAdmissionCache {
public:
  FailureOr<Type> admit(Type original, const ac::HardwareAnalysis &hardware,
                         const ac::HardwareBindings &bindings, Operation *site,
                         SourceCheckBudget &budget) {
    if (!charge(budget, site, 2)) return failure();
    const ac::HardwareBindings *context = &bindings;
    if (bindings.integers.empty() && bindings.types.empty()) {
      // Separate Rule frames of one unparameterized Module carry equivalent
      // immutable bindings. Reuse type admission across those frames while
      // keeping their value histories and generated graph proofs independent.
      for (uint64_t count = emptyContexts.size() + 1; count; count >>= 1)
        if (!charge(budget, site, 2)) return failure();
      auto found = emptyContexts.find(bindings.owner);
      if (found != emptyContexts.end()) {
        context = found->second;
      } else {
        if (!charge(budget, site, 8 + SourceCheckBudget::bytes(
                sizeof(decltype(emptyContexts)::value_type) + 4 * sizeof(void *))))
          return failure();
        emptyContexts.try_emplace(bindings.owner, context);
      }
    }
    auto key = std::make_pair(context, original);
    if (auto found = admitted.find(key); found != admitted.end()) return found->second;
    auto resolved = branchResultType(original, hardware, bindings, site, budget);
    if (failed(resolved)) return failure();
    using Bucket = decltype(admitted)::value_type;
    uint64_t oldCapacity = admitted.getMemorySize() / sizeof(Bucket);
    uint64_t capacity = std::max(uint64_t(64), oldCapacity);
    uint64_t count = admitted.size() + 1;
    while (SourceCheckBudget::multiply(count, 4) >=
           SourceCheckBudget::multiply(capacity, 3)) {
      if (!charge(budget, site)) return failure();
      capacity = SourceCheckBudget::multiply(capacity, 2);
    }
    if (capacity > oldCapacity &&
        !charge(budget, site, SourceCheckBudget::add(admitted.size(),
            SourceCheckBudget::bytes(SourceCheckBudget::multiply(capacity, sizeof(Bucket))))))
      return failure();
    admitted.try_emplace(key, *resolved);
    return *resolved;
  }
private:
  llvm::DenseMap<std::pair<const ac::HardwareBindings *, Type>, Type> admitted;
  std::map<Operation *, const ac::HardwareBindings *> emptyContexts;
};

// Only called after this traversal independently admits the complete root
// closure under these bindings. Canonical literal widths resolve to the exact
// same Type, including origin/location; computed and parameter widths still use
// the complete existing admission path.
FailureOr<Type> admittedBranchField(Type declared,
                                    const ac::HardwareAnalysis &hardware,
                                    const ac::HardwareBindings &bindings,
                                    Operation *site, SourceCheckBudget &budget,
                                    BranchTypeAdmissionCache *cache) {
  if (!charge(budget, site, 16)) return failure();
  auto bits = dyn_cast<ac::BitsType>(declared);
  auto tree = bits ? bits.getWidth().getTree() : DictionaryAttr();
  auto kind = tree ? tree.getAs<StringAttr>("kind") : StringAttr();
  auto value = tree ? tree.getAs<DictionaryAttr>("value") : DictionaryAttr();
  auto valueKind = value ? value.getAs<StringAttr>("kind") : StringAttr();
  if (tree && tree.size() == 4 && kind && kind.getValue() == "literal" &&
      tree.get("origin") && tree.get("location") && value && value.size() == 2 &&
      valueKind && valueKind.getValue() == "integer" &&
      value.getAs<ac::MathIntAttr>("value"))
    return declared;
  return cache ? cache->admit(declared, hardware, bindings, site, budget)
               : branchResultType(declared, hardware, bindings, site, budget);
}

FailureOr<BranchJoinLayout>
branchJoinLayout(Type root, const ac::HardwareAnalysis &hardware,
                 const ac::HardwareBindings &bindings, Operation *site,
                 SourceCheckBudget &budget, BranchTypeAdmissionCache *cache = nullptr) {
  auto admitted = cache ? cache->admit(root, hardware, bindings, site, budget)
                        : branchResultType(root, hardware, bindings, site, budget);
  if (failed(admitted))
    return failure();
  struct Pending {
    Type type;
    StringAttr field;
    uint64_t parent;
    unsigned depth;
  };
  SmallVector<Pending, 0> pending;
  BranchJoinLayout result;
  if (!prepayAppend(budget, site, pending))
    return failure();
  pending.push_back({*admitted, {}, UINT64_MAX, 0});
  while (!pending.empty()) {
    auto next = pending.pop_back_val();
    if (next.depth > 128 || !prepayAppend(budget, site, result.nodes))
      return failure();
    uint64_t id = result.nodes.size();
    result.nodes.emplace_back();
    result.nodes.back().type = next.type;
    result.nodes.back().field = next.field;
    result.nodes.back().parent = next.parent;
    if (next.parent != UINT64_MAX) {
      result.nodes[id].ordinal = result.nodes[next.parent].children.size();
      result.nodes[next.parent].children.push_back(id);
    }
    if (isa<ac::BitsType>(next.type)) {
      result.operations = SourceCheckBudget::add(result.operations, 1);
    } else if (auto enumeration = dyn_cast<ac::EnumType>(next.type)) {
      // The admission helper has resolved and validated this declaration.
      // Read its already-proven finite width without revalidating members.
      auto declaration = hardware.lookupEnum(enumeration);
      auto width = declaration.getWidthAttr().getCanonicalValue();
      if (!charge(budget, site,
                  SourceCheckBudget::add(
                      16, SourceCheckBudget::bytes(width.size()))))
        return failure();
      result.nodes[id].enumWidth = llvm::APSInt(width).getZExtValue();
      result.operations = SourceCheckBudget::add(result.operations, 4);
    } else if (auto table = dyn_cast<ac::TableType>(next.type)) {
      auto count = hardware.getTableSize(table, bindings, site);
      if (failed(count) || !product(budget, site, *count,
                                   sizeof(BranchJoinNode) + sizeof(uint64_t)))
        return failure();
      if (*count > result.nodes[id].children.max_size() ||
          *count > result.nodes[id].fields.max_size()) return failure();
      result.nodes[id].children.reserve(*count);
      result.nodes[id].fields.reserve(*count);
      result.operations = SourceCheckBudget::add(
          result.operations, SourceCheckBudget::add(1, 3 * *count));
      // Element order is the established flattened Table order.
      for (uint64_t i = 0; i < *count; ++i) {
        if (!prepayAppend(budget, site, pending)) return failure();
        pending.push_back({table.getElementType(), {}, id, next.depth + 1});
      }
    } else if (auto structure = dyn_cast<ac::StructType>(next.type)) {
      auto fields = hardware.lookupStruct(structure).getFields();
      uint64_t count = fields.size();
      uint64_t bytes =
          SourceCheckBudget::multiply(count, sizeof(uint64_t) + sizeof(Value));
      if (bytes == std::numeric_limits<uint64_t>::max() ||
          count > result.nodes[id].children.max_size() ||
          count > result.nodes[id].fields.max_size() ||
          !charge(
              budget, site,
              SourceCheckBudget::add(count, SourceCheckBudget::bytes(bytes))))
        return failure();
      result.nodes[id].children.reserve(count);
      result.nodes[id].fields.reserve(count);
      result.operations = SourceCheckBudget::add(
          result.operations,
          SourceCheckBudget::add(1, SourceCheckBudget::multiply(count, 2)));
      for (Attribute raw : llvm::reverse(fields)) {
        if (!charge(budget, site, 2)) return failure();
        auto field = cast<DictionaryAttr>(raw);
        Type declared = field.getAs<TypeAttr>("type").getValue();
        // This traversal's admitted root proves the complete nominal closure.
        // Nominal children retain identity; Bits still resolve their width.
        FailureOr<Type> resolved = failure();
        if (isa<ac::StructType, ac::EnumType>(declared))
          resolved = declared;
        else if (isa<ac::BitsType>(declared))
          resolved = admittedBranchField(declared, hardware, bindings, site, budget, cache);
        else
          return site->emitOpError() << "unsupported local branch field type";
        if (failed(resolved) || !prepayAppend(budget, site, pending))
          return failure();
        pending.push_back(
            {*resolved, field.getAs<StringAttr>("name"), id, next.depth + 1});
      }
    } else {
      return site->emitOpError() << "unsupported local branch result type";
    }
  }
  return result;
}

// Width provenance belongs to this original branch occurrence. Enum width
// authority has already been established from its nominal declaration.
DictionaryAttr selectionSourceLocation(Operation *source) {
  auto span = source->getAttrOfType<DictionaryAttr>("ac.condition_location");
  if (!span) span = source->getAttrOfType<DictionaryAttr>("ac.guard_location");
  if (!span)
    if (auto transaction = source->getParentOfType<ac::SourceTransactionOp>())
      span = transaction->getAttrOfType<DictionaryAttr>("ac.guard_location");
  if (!span)
    if (auto method = source->getParentOfType<ac::SourceMethodOp>())
      span = method->getAttrOfType<DictionaryAttr>("ac.guard_location");
  // A resource pin join belongs to its original primitive allocation. Keep
  // carrier provenance at that authored allocation's exact source span.
  if (!span && isa<ac::InstanceOp>(source))
    span = source->getAttrOfType<DictionaryAttr>("ac.location");
  return span;
}

Type branchCarrierType(uint64_t width, Operation *branch,
                       ac::BitsType predicateType = {}) {
  Builder builder(branch->getContext());
  auto location = selectionSourceLocation(branch);
  // Incoming commits are owned by a state declaration, outside a branch or
  // transaction region. Its predicate type retains verified width provenance,
  // just as the generated literal values below retain their carrier's span.
  if (!location && predicateType)
    location = predicateType.getWidth().getTree().getAs<DictionaryAttr>("location");
  auto literal = ac::MathIntAttr::get(
      branch->getContext(), llvm::APSInt(llvm::APInt(64, width), true));
  auto tree = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
       builder.getNamedAttr(
           "value",
           builder.getDictionaryAttr(
               {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
                builder.getNamedAttr("value", literal)})),
       builder.getNamedAttr("origin", branch->getAttr("occurrence")),
       builder.getNamedAttr("location", location)});
  return ac::BitsType::get(branch->getContext(),
                           ac::StaticExprAttr::get(branch->getContext(), tree));
}

// Prepay every generated operation and its transient operand/type/attribute
// storage before creating any join operation, including Table element access.
bool prepayBranchJoin(SourceCheckBudget &budget, Operation *site,
                      const BranchJoinLayout &layout) {
  uint64_t bytes = SourceCheckBudget::multiply(
      layout.operations, sizeof(Operation *) + sizeof(Operation) +
                             sizeof(OperationState) + 4 * sizeof(Value) +
                             2 * sizeof(Type) + 16 * sizeof(NamedAttribute) +
                             sizeof(ac::StructGetOp::Properties));
  bytes = SourceCheckBudget::add(
      bytes, SourceCheckBudget::multiply(
                 layout.nodes.size(), 2 * sizeof(std::pair<uint64_t, bool>)));
  if (bytes == std::numeric_limits<uint64_t>::max())
    return charge(budget, site, bytes);
  return charge(
      budget, site,
      SourceCheckBudget::add(SourceCheckBudget::multiply(layout.operations, 64),
                             SourceCheckBudget::bytes(bytes)));
}

struct SavedMaterializedObservation {
  Value path;
  ac::TransactionPoint point;
  SmallVector<Value, 0> values;
};

// IRMapping does not expose reservation. Prepay the actual value-map growth
// before adding a branch result; arm SSA shares this one invocation map.
bool prepayValueMapping(SourceCheckBudget &budget, Operation *site,
                        const IRMapping &mapping, Value key) {
  const auto &values = mapping.getValueMap();
  if (values.contains(key))
    return charge(budget, site);
  using Entry = llvm::DenseMap<Value, Value>::value_type;
  uint64_t memory = values.getMemorySize();
  uint64_t buckets = memory / sizeof(Entry);
  uint64_t entries = SourceCheckBudget::add(values.size(), 1);
  uint64_t fee = 1 + SourceCheckBudget::bytes(sizeof(Entry));
  if (SourceCheckBudget::multiply(entries, 4) >=
      SourceCheckBudget::multiply(buckets, 3)) {
    uint64_t grown =
        std::max<uint64_t>(64, SourceCheckBudget::multiply(buckets, 2));
    uint64_t bytes = SourceCheckBudget::multiply(grown, sizeof(Entry));
    if (bytes < memory || bytes == std::numeric_limits<uint64_t>::max())
      return charge(budget, site, std::numeric_limits<uint64_t>::max());
    fee = SourceCheckBudget::add(
        fee, SourceCheckBudget::add(SourceCheckBudget::bytes(bytes - memory),
                                    values.size()));
  }
  return charge(budget, site, fee);
}

bool sameSourcePath(ac::TransactionSourcePath lhs,
                    ac::TransactionSourcePath rhs) {
  return lhs.branch == rhs.branch && lhs.whenTrue == rhs.whenTrue;
}
bool sameSourcePoint(ac::TransactionPoint lhs, ac::TransactionPoint rhs) {
  return lhs.prefix == rhs.prefix && sameSourcePath(lhs.path, rhs.path);
}

// Projection discards pure source branches, preserving the polarity of the
// nearest selection ancestor rather than that of its pure descendant.
FailureOr<DictionaryAttr> sampledPath(const ac::TransactionStatePlan &plan,
                                      ac::TransactionSourcePath path,
                                      SourceCheckBudget &budget) {
  Builder b(plan.transaction->getContext());
  for (unsigned depth = 0; depth <= 128; ++depth) {
    if (!charge(budget, plan.transaction))
      return failure();
    if (!path.branch) {
      if (path.whenTrue)
        return plan.transaction->emitOpError() << "invalid root source path";
      return b.getDictionaryAttr({});
    }
    if (*path.branch >= plan.branches.size())
      return plan.transaction->emitOpError() << "invalid branch source path";
    const auto &branch = plan.branches[*path.branch];
    if (branch.selection) {
      if (*branch.selection >= plan.selections.size())
        return plan.transaction->emitOpError()
               << "invalid selection source path";
      return b.getDictionaryAttr(
          {b.getNamedAttr(
               "selection",
               b.getI64IntegerAttr(plan.selections[*branch.selection].control)),
           b.getNamedAttr("when_true", b.getBoolAttr(path.whenTrue))});
    }
    if (branch.entry.path.branch && *branch.entry.path.branch >= *path.branch)
      break;
    path = branch.entry.path;
  }
  return plan.transaction->emitOpError() << "source path depth exceeded";
}

FailureOr<bool> staticTotality(const ac::TransactionStatePlan &plan,
                              SourceCheckBudget &budget) {
  if (!plan.transaction->getNumResults())
    return true;
  if (!plan.queueAccesses.empty() || !plan.memoryAccesses.empty())
    return false;
  enum class Fact { False, True, Unknown };
  auto conjunction = [](Fact lhs, Fact rhs) {
    if (lhs == Fact::False || rhs == Fact::False)
      return Fact::False;
    return lhs == Fact::True && rhs == Fact::True ? Fact::True : Fact::Unknown;
  };
  Fact prefix = Fact::True;
  for (const auto &control : plan.controls) {
    if (!charge(budget, plan.transaction))
      return failure();
    Fact path = Fact::True;
    auto ancestry = control.path;
    for (unsigned depth = 0; ancestry.branch; ++depth) {
      if (!charge(budget, plan.transaction) || depth > 128 ||
          *ancestry.branch >= plan.branches.size())
        return failure();
      const auto &branch = plan.branches[*ancestry.branch];
      Fact selected = !branch.conditionFact ? Fact::Unknown
                      : branch.conditionFact.getValue() == ancestry.whenTrue
                          ? Fact::True
                          : Fact::False;
      path = conjunction(path, selected);
      ancestry = branch.entry.path;
    }
    Fact accepted = Fact::Unknown;
    if (auto *guard =
            std::get_if<ac::CallableGuardControl>(&control.boundary)) {
      if (guard->guard >= plan.guards.size())
        return failure();
      auto fact = plan.guards[guard->guard].predicateFact;
      if (fact)
        accepted = fact.getValue() ? Fact::True : Fact::False;
    } else {
      auto id = std::get<ac::BranchSelectControl>(control.boundary).selection;
      if (id >= plan.selections.size() ||
          plan.selections[id].branch >= plan.branches.size())
        return failure();
      if (plan.branches[plan.selections[id].branch].conditionFact)
        accepted = Fact::True;
    }
    Fact bypass = path == Fact::False || accepted == Fact::True ? Fact::True
                  : path == Fact::True                          ? accepted
                                                                : Fact::Unknown;
    prefix = conjunction(prefix, bypass);
  }
  return prefix == Fact::True;
}
LogicalResult requireStaticTotality(const ac::TransactionStatePlan &plan,
                                    SourceCheckBudget &budget,
                                    bool managed = false) {
  auto total = staticTotality(plan, budget);
  if (failed(total)) return failure();
  if (!*total && !managed)
    return plan.transaction->emitOpError()
           << "ordinary transaction results require proven total admission";
  return success();
}

bool branchHasStateWrites(const ac::TransactionStatePlan &plan,
                          ac::BranchId wanted) {
  for (const auto &write : plan.writes) {
    auto path = plan.versions[write.version].validity.path;
    for (unsigned depth = 0; path.branch && depth < 128; ++depth) {
      if (*path.branch == wanted) return true;
      path = plan.branches[*path.branch].entry.path;
    }
  }
  return false;
}

LogicalResult verifyClosedPurePayload(Operation *root,
                                      SourceCheckBudget &budget,
                                      StringRef description) {
  SmallVector<Operation *> pending{root};
  while (!pending.empty()) {
    Operation *operation = pending.pop_back_val();
    if (!charge(budget, operation,
                3 + uint64_t(operation->getNumOperands()) * 2 +
                    uint64_t(operation->getNumResults()) * 2 +
                    operation->getNumRegions() +
                    operation->getRawDictionaryAttrs().size() +
                    operation->getPropertiesStorageSize()))
      return failure();
    if (isa<ac::SourceExpectOp, ac::SourceObserveOp>(operation))
      return operation->emitOpError()
             << "transaction body checks/observations require sampled effect "
                "lowering";
    if (!ac::detail::isCombinationalHardwareOperation(operation) &&
        !(operation != root && isa<ac::YieldOp>(operation)))
      return operation->emitOpError() << description;
    for (Region &region : operation->getRegions())
      for (Block &block : region) {
        if (!charge(budget, operation,
                    1 + uint64_t(block.getNumArguments()) * 2))
          return failure();
        for (Operation &nested : block) {
          if (!charge(budget, operation))
            return failure();
          pending.push_back(&nested);
        }
      }
  }
  return success();
}

LogicalResult
verifyPreparedEffectInventory(const PreparedTransactionLowering &prepared,
                              SourceCheckBudget &budget) {
  SmallVector<uint64_t> rank(prepared.transactions.size(), UINT64_MAX);
  if (prepared.transactionSchedule.size() != prepared.transactions.size())
    return prepared.original->emitError() << "transaction schedule inventory is inconsistent";
  for (auto [ordinal, index] : llvm::enumerate(prepared.transactionSchedule)) {
    if (index >= rank.size() || rank[index] != UINT64_MAX)
      return prepared.original->emitError() << "transaction schedule contains an invalid registration";
    rank[index] = ordinal;
  }
  for (auto [itemIndex, item] : llvm::enumerate(prepared.transactions)) {
    if (!item.plan)
      return prepared.original->emitError()
             << "transaction lowering requires an intact prepared state plan";
    const auto &plan = *item.plan;
    auto site = plan.transaction;
    if (!charge(budget, site,
                8 + plan.bodyChecks.size() + plan.observations.size() +
                    plan.events.size()))
      return failure();
    auto checks = item.expectedAdmission.getAs<ArrayAttr>("body_checks");
    auto observations = item.expectedAdmission.getAs<ArrayAttr>("observations");
    auto guards = item.expectedAdmission.getAs<ArrayAttr>("controls");
    auto entries = item.expectedAdmission.getAs<ArrayAttr>("ehr_entries");
    auto requirements = item.expectedAdmission.getAs<ArrayAttr>("requirements");
    if (item.sourceControls.size() != plan.controls.size() ||
        (item.requirements.empty() ? bool(requirements)
                                  : (!requirements || requirements.size() != item.requirements.size())))
      return site.emitOpError() << "transaction required-use inventory is inconsistent";
    for (auto [index, control] : llvm::enumerate(item.controls)) {
      if (unsigned(bool(control.source)) + unsigned(bool(control.requirement)) +
              unsigned(bool(control.resource)) != 1 ||
          (control.resource && *control.resource >= item.resources.size()) ||
          (control.source && (*control.source >= plan.controls.size() ||
                             item.sourceControls[*control.source] != index)) ||
          (control.requirement && *control.requirement >= item.requirements.size()))
        return site.emitOpError() << "transaction control interleave is inconsistent";
    }
    for (const auto &requirement : item.requirements)
      if (!requirement.site || requirement.frame >= plan.frames.size() ||
          requirement.payload >= plan.values.size() ||
          (requirement.rawPath && *requirement.rawPath >= plan.values.size()) ||
          requirement.control >= item.controls.size() || !requirement.kind ||
          !requirement.occurrence || !requirement.location ||
          requirement.point.prefix > plan.controls.size() ||
          (requirement.point.path.branch && *requirement.point.path.branch >= plan.branches.size()))
        return site.emitOpError() << "transaction required-use bounds are inconsistent";
    if (item.expectedAdmission.size() != (6 + !item.entryOwners.empty() + !item.requirements.empty() +
            !item.resources.empty() + !item.queueProposals.empty() + !item.memoryProposals.empty() + !item.writeDomains.empty()) ||
        (item.entryOwners.empty() ? bool(entries) : (!entries || entries.size() != item.entryOwners.size())) ||
        !guards || !checks ||
        !observations || guards.size() != item.controls.size() ||
        checks.size() != plan.bodyChecks.size() ||
        observations.size() != plan.observations.size() ||
        item.observationPathResults.size() != plan.observations.size() ||
        item.expectedRequiredChecks.size() !=
            item.controls.size() + plan.bodyChecks.size())
      return site.emitOpError()
             << "transaction prepared body-effect inventory is inconsistent";
    uint64_t fire = SourceCheckBudget::add(
        plan.transaction->getNumResults(),
        SourceCheckBudget::multiply(2, item.outputs.size()));
    uint64_t next = SourceCheckBudget::add(fire, 1);
    for (auto [id, observation] : llvm::enumerate(plan.observations)) {
      if (item.observationPathResults[id] != next)
        return site.emitOpError()
               << "transaction prepared observation layout is inconsistent";
      next = SourceCheckBudget::add(
          next, SourceCheckBudget::add(1, observation.values.size()));
    }
    uint64_t total = SourceCheckBudget::add(
        next, SourceCheckBudget::multiply(
                  2, SourceCheckBudget::add(item.controls.size(),
                                            plan.bodyChecks.size())));
    total = SourceCheckBudget::add(total, item.entryOwners.size());
    for (const auto &proposal : item.queueProposals) {
      if (proposal.access >= plan.queueAccesses.size() ||
          proposal.rawResult != total || proposal.pathResult != total + 1 ||
          (proposal.dataResult && *proposal.dataResult != total + 2))
        return site.emitOpError() << "queue proposal result layout is inconsistent";
      total += proposal.dataResult ? 3 : 2;
    }
    for (const auto &proposal : item.memoryProposals) {
      if (proposal.access >= plan.memoryAccesses.size() || proposal.rawResult != total ||
          proposal.pathResult != total + 1 || proposal.argumentResults.size() != plan.memoryAccesses[proposal.access].arguments.size())
        return site.emitOpError() << "Memory proposal result layout is inconsistent";
      if (proposal.reachedControlEnd > item.controls.size() ||
          (plan.memoryAccesses[proposal.access].kind != ac::TransactionMemoryAccessKind::Read &&
           (proposal.reachedControlEnd || !proposal.excludedControls.empty())))
        return site.emitOpError() << "Memory reached control boundary is inconsistent";
      uint64_t previousExcluded = UINT64_MAX;
      for (uint64_t excluded : proposal.excludedControls) {
        if (excluded >= proposal.reachedControlEnd ||
            (previousExcluded != UINT64_MAX && excluded <= previousExcluded))
          return site.emitOpError() << "Memory excluded control layout is inconsistent";
        previousExcluded = excluded;
      }
      if (proposal.physicalReuse) {
        const auto &access = plan.memoryAccesses[proposal.access];
        if (*proposal.physicalReuse >= proposal.access ||
            access.kind != ac::TransactionMemoryAccessKind::Read ||
            plan.memoryAccesses[*proposal.physicalReuse].kind != ac::TransactionMemoryAccessKind::Read)
          return site.emitOpError() << "Memory physical read reuse layout is inconsistent";
      }
      total += 2;
      for (uint64_t result : proposal.argumentResults)
        if (result != total++) return site.emitOpError() << "Memory argument result layout is inconsistent";
    }
    for (const auto &domain : item.writeDomains) {
      if (domain.write >= plan.writes.size() || domain.candidate >= item.outputs.size() ||
          domain.result != total || !domain.selectors || !domain.maskType ||
          (domain.index && *domain.index >= plan.values.size()))
        return site.emitOpError() << "write domain result layout is inconsistent";
      total = SourceCheckBudget::add(total, 1);
    }
    llvm::DenseSet<ac::StateOwnerId> ownerSet;
    for (auto owner : item.entryOwners)
      if (owner >= plan.owners.size() || !ownerSet.insert(owner).second)
        return site.emitOpError() << "transaction entry owner inventory is inconsistent";
    for (auto [ordinal, forward] : llvm::enumerate(item.forwarding))
      if (forward.owner >= plan.owners.size() || forward.predecessor >= prepared.transactions.size() ||
          rank[forward.predecessor] >= rank[itemIndex] ||
          forward.output >= prepared.transactions[forward.predecessor].outputs.size() ||
          forward.dataCapture != item.liveIns.size() + 2 * ordinal ||
          forward.enableCapture != forward.dataCapture + 1)
        return site.emitOpError() << "transaction entry capture inventory is inconsistent";
    if (item.clockCapture >= item.liveIns.size() ||
        item.resetCapture >= item.liveIns.size() ||
        fire == std::numeric_limits<uint64_t>::max() ||
        next == std::numeric_limits<uint64_t>::max() ||
        total == std::numeric_limits<uint64_t>::max() ||
        item.fireResult != fire || item.checkBase != next ||
        item.totalResults != total ||
        total > std::numeric_limits<size_t>::max())
      return site.emitOpError()
             << "transaction prepared result layout is inconsistent";
    for (auto [ordinal, output] : llvm::enumerate(item.outputs))
      if (output.binding.owner >= plan.owners.size() ||
          output.binding.version >= plan.versions.size() ||
          output.dataResult !=
              plan.transaction->getNumResults() + 2 * ordinal ||
          output.enableResult != output.dataResult + 1)
        return site.emitOpError()
               << "transaction prepared candidate layout is inconsistent";
    auto index = [](uint64_t id, size_t size) { return id < size; };
    auto validPath = [&](ac::TransactionSourcePath path) {
      return path.branch ? index(*path.branch, plan.branches.size())
                         : !path.whenTrue;
    };
    auto validPoint = [&](ac::TransactionPoint point) {
      return point.prefix <= plan.controls.size() && validPath(point.path);
    };
    if (plan.rootFrame >= plan.frames.size() ||
        plan.guard >= plan.guards.size() ||
        !index(plan.clockValue, plan.values.size()) ||
        !index(plan.resetValue, plan.values.size()) ||
        plan.controls.size() != plan.guards.size() + plan.selections.size() ||
        plan.guards.size() != plan.frames.size())
      return site.emitOpError()
             << "transaction prepared control/frame inventory is inconsistent";
    for (auto [frameId, frame] : llvm::enumerate(plan.frames)) {
      if (!charge(budget, site,
                  8 + frame.guardArguments.size() +
                      frame.bodyArguments.size()) ||
          !index(frame.anchor, plan.instances.size()) ||
          !index(frame.guard, plan.guards.size()) ||
          (frame.parent && !index(*frame.parent, plan.frames.size())) ||
          (frame.registrationEntry && frameId != plan.rootFrame) ||
          !validPoint(frame.entry) || !validPoint(frame.exit))
        return failure();
      for (ArrayRef<ac::TransactionValueBinding> arguments :
           {ArrayRef<ac::TransactionValueBinding>(frame.guardArguments),
            ArrayRef<ac::TransactionValueBinding>(frame.bodyArguments)})
        for (const auto &argument : arguments)
          if (!index(argument.actual, plan.values.size()) || !argument.formal)
            return failure();
    }
    for (const auto &ref : plan.values)
      if (!charge(budget, site, 4) || !ref.value ||
          !index(ref.anchor, plan.instances.size()) ||
          !index(ref.frame, plan.frames.size()) || !validPoint(ref.validity))
        return failure();
    for (const auto &guard : plan.guards)
      if (!charge(budget, site, 4) || !guard.callable ||
          !index(guard.frame, plan.frames.size()) ||
          !index(guard.control, plan.controls.size()) ||
          !index(guard.predicate, plan.values.size()))
        return failure();
    for (const auto &control : plan.controls) {
      if (!charge(budget, site, 4) || !validPath(control.path))
        return failure();
      if (auto *tag =
              std::get_if<ac::CallableGuardControl>(&control.boundary)) {
        if (!index(tag->guard, plan.guards.size()))
          return failure();
      } else if (!index(std::get<ac::BranchSelectControl>(control.boundary)
                            .selection,
                        plan.selections.size()))
        return failure();
    }
    for (const auto &selection : plan.selections)
      if (!charge(budget, site, 2) ||
          !index(selection.branch, plan.branches.size()) ||
          !index(selection.control, plan.controls.size()))
        return failure();
    for (const auto &branch : plan.branches)
      if (!charge(budget, site, 4) || !branch.operation ||
          !index(branch.frame, plan.frames.size()) ||
          !index(branch.condition, plan.values.size()) ||
          !validPoint(branch.entry) ||
          (branch.selection &&
           !index(*branch.selection, plan.selections.size())) ||
          branch.thenExitPrefix > plan.controls.size() ||
          branch.elseExitPrefix > plan.controls.size())
        return failure();
    for (const auto &version : plan.versions)
      if (!charge(budget, site, 4) ||
          !index(version.owner, plan.owners.size()) ||
          !validPoint(version.validity) ||
          (version.value && !index(*version.value, plan.values.size())) ||
          (version.predecessor &&
           !index(*version.predecessor, plan.versions.size())))
        return failure();
    for (const auto &read : plan.reads)
      if (!charge(budget, site, 4) || !read.operation ||
          !index(read.frame, plan.frames.size()) ||
          !index(read.owner, plan.owners.size()) ||
          !index(read.version, plan.versions.size()) ||
          !index(read.result, plan.values.size()) ||
          !index(read.authoredPath, plan.values.size()))
        return failure();
    for (const auto &write : plan.writes)
      if (!charge(budget, site, 4) || !write.operation ||
          !index(write.frame, plan.frames.size()) ||
          !index(write.owner, plan.owners.size()) ||
          !index(write.predecessor, plan.versions.size()) ||
          !index(write.version, plan.versions.size()) ||
          !index(write.value, plan.values.size()) ||
          !index(write.authoredPath, plan.values.size()))
        return failure();
    for (const auto &call : plan.calls)
      if (!charge(budget, site, 4) || !call.operation ||
          !index(call.caller, plan.frames.size()) ||
          !index(call.callee, plan.frames.size()) ||
          !index(call.authoredPath, plan.values.size()))
        return failure();
    for (const auto &check : plan.bodyChecks)
      if (!check.operation || !index(check.frame, plan.frames.size()) ||
          !index(check.condition, plan.values.size()) ||
          !index(check.authoredPath, plan.values.size()) ||
          !validPoint(check.point))
        return site.emitOpError()
               << "transaction prepared body-check locator is absent";
    for (const auto &observation : plan.observations)
      if (!observation.operation ||
          !index(observation.frame, plan.frames.size()) ||
          !index(observation.authoredPath, plan.values.size()) ||
          !validPoint(observation.point))
        return site.emitOpError()
               << "transaction prepared observation locator is absent";
    for (const auto &event : plan.events) {
      if (event.kind == ac::TransactionStateEventKind::BodyCheck &&
          (!event.bodyCheck || *event.bodyCheck >= plan.bodyChecks.size()))
        return site.emitOpError()
               << "transaction prepared body-check event is inconsistent";
      if (event.kind == ac::TransactionStateEventKind::Observe &&
          (!event.observation ||
           *event.observation >= plan.observations.size()))
        return site.emitOpError()
               << "transaction prepared observation event is inconsistent";
    }
  }
  return success();
}

LogicalResult mergedEnvelope(mlir::ModuleOp package) {
  if (package->hasAttr("ac.stage"))
    return package.emitError()
           << "transaction lowering requires a merged package without ac.stage";
  for (StringRef name : {"ac.source_owner", "ac.unit_kind", "ac.interfaces",
                         "ac.exports", "ac.import_bindings"})
    if (package->hasAttr(name))
      return package.emitError()
             << "transaction lowering cannot consume a single-source envelope: "
             << name;
  if (!package->getAttrOfType<ArrayAttr>("ac.source_units"))
    return package.emitError()
           << "transaction lowering requires merged ac.source_units";
  if (failed(ac::collectFinalSourceUnits(package,
                                         [&] { return package.emitError(); })))
    return failure();
  unsigned systems = 0;
  for (Operation &op : package.getBody()->getOperations()) {
    if (!isa<ac::ModuleOp, ac::ModuleImportOp, ac::StructOp, ac::EnumOp,
             ac::SystemOp>(op))
      return op.emitOpError() << "transaction lowering requires closed merged "
                                 "hardware declarations";
    systems += isa<ac::SystemOp>(op);
    if (auto imported = dyn_cast<ac::ModuleImportOp>(op))
      if (imported.getSemanticKind() != "hardware" ||
          !imported->getAttrOfType<StringAttr>("primitive_kind"))
        return imported.emitOpError()
               << "transaction lowering requires resolved provider definitions";
  }
  if (systems != 1)
    return package.emitError()
           << "transaction lowering requires exactly one merged ac.system";
  return success();
}

bool prepayOperationHeader(Operation *op, SourceCheckBudget &budget) {
  return charge(budget, op, 3 + uint64_t(op->getNumOperands()) * 2 +
                               uint64_t(op->getNumResults()) * 2 + op->getNumRegions() +
                               op->getRawDictionaryAttrs().size() +
                               op->getPropertiesStorageSize());
}

LogicalResult prepayClone(Operation *root, SourceCheckBudget &budget) {
  SmallVector<Operation *> pending;
  if (!charge(budget, root))
    return failure();
  pending.push_back(root);
  while (!pending.empty()) {
    Operation *op = pending.pop_back_val();
    if (!prepayOperationHeader(op, budget))
      return failure();
    for (Region &region : op->getRegions())
      for (Block &block : region) {
        if (!charge(budget, op, 1 + uint64_t(block.getNumArguments()) * 2))
          return failure();
        for (Operation &nested : block) {
          if (!charge(budget, op))
            return failure();
          pending.push_back(&nested);
        }
      }
  }
  return success();
}

bool moduleValue(Value value, ac::ModuleOp module) {
  return value.getParentBlock() == &module.getBody().front();
}

ArrayAttr routePath(MLIRContext *context, ArrayRef<ac::InstanceOp> path) {
  SmallVector<Attribute> occurrences;
  for (auto allocation : path)
    occurrences.push_back(allocation.getOccurrence());
  return ArrayAttr::get(context, occurrences);
}

RoutingKey routingKey(ArrayRef<ac::InstanceOp> path, Value value) {
  return {routePath(value.getContext(), path), value};
}

SmallVector<ac::InstanceOp> anchorPath(const ac::TransactionStatePlan &plan,
                                       ac::LexicalInstanceId anchor) {
  SmallVector<ac::InstanceOp> result;
  while (plan.instances[anchor].parent) {
    result.push_back(plan.instances[anchor].allocation);
    anchor = *plan.instances[anchor].parent;
  }
  std::reverse(result.begin(), result.end());
  return result;
}

// Physical Memory issue operands in canonical primitive order. Result snapshots
// claim no port; their data and live state are ordinary routed captures.
StringRef memoryKind(ac::TransactionMemoryAccessKind kind) {
  using K = ac::TransactionMemoryAccessKind;
  switch (kind) {
  case K::Request: return "request";
  case K::Result: return "result";
  case K::Read: return "read";
  case K::Write: return "write";
  }
  llvm_unreachable("invalid Memory access kind");
}
SmallVector<unsigned, 4> memoryOperands(const ac::TransactionMemoryAccess &access,
                                       const ac::MemoryPrimitiveLayout &layout) {
  using K = ac::TransactionMemoryAccessKind;
  if (access.kind == K::Request)
    return {layout.readEnableOperands[access.port], layout.readAddressOperands[access.port]};
  if (access.kind == K::Read) return {layout.readAddressOperands[0]};
  if (access.kind == K::Write)
    return {layout.writeEnableOperand, layout.writeAddressOperand,
            layout.writeDataOperand, layout.writeMaskOperand};
  return {};
}

#include "TransactionSourceGrantProof.inc"

/// Admission and growth accounting finish on the intact input before cloning.
/// Routing only summarizes the existing analysis identities; it does not
/// resolve Methods, evaluate values or construct a second state plan.
class LoweringPreparation : public PreparedTransactionLowering {
public:
  LoweringPreparation(mlir::ModuleOp package,
                      const ac::SourceTransactionStateAnalysis &analysis,
                      SourceCheckBudget &budget)
      : hardware(package), package(package), analysis(analysis),
        budget(budget) {
    original = package;
  }

  LogicalResult run() {
    if (failed(mergedEnvelope(package)) || failed(prepayClone(package, budget)))
      return failure();
    for (auto imported : package.getOps<ac::ModuleImportOp>()) {
      if (!charge(budget, imported) || failed(hardware.verifyImport(imported)))
        return failure();
      StringRef kind = hardware.getPrimitiveKind(imported);
      if (kind == "dffe") {
        ++dffeCount;
        if (!dffe)
          dffe = imported;
      } else if (kind == "dff") {
        if (!dff)
          dff = imported;
      }
    }
    if (!states.empty() && (!dffe || dffeCount != 1))
      return package.emitError() << "transaction state requires exactly one "
                                    "verified DFFE primitive";
    budget.setPhase("prep: inventory");
    for (auto module : package.getOps<ac::ModuleOp>()) {
      auto signature = module.getFunctionType();
      definitionIndex.try_emplace(module, definitions.size());
      DefinitionRouting definition;
      definition.module = module;
      definition.sourceInputs = signature.getNumInputs();
      definition.sourceOutputs = signature.getNumResults();
      definitions.push_back(std::move(definition));
      ac::HardwareBindings bindings;
      bindings.owner = module;
      for (Operation &op : module.getBody().front()) {
        if (!charge(budget, &op))
          return failure();
        if (auto state = dyn_cast<ac::SourceStateOp>(op)) {
          auto width =
              hardware.getPackedWidth(state.getQ().getType(), bindings, state);
          if (failed(width))
            return failure();
          if (!*width)
            return state.emitOpError()
                   << "transaction state width must be positive";
          if (!charge(budget, state, 32))
            return failure();
          definitions.back().stateIndices.push_back(states.size());
          states.push_back(state);
        } else if (auto queue = dyn_cast<ac::InstanceOp>(op);
                   queue && ac::isQueueInstance(queue)) {
          if (!charge(budget, queue, 8)) return failure();
          definitions.back().queueIndices.push_back(queues.size());
          queues.push_back(queue);
        } else if (auto transaction = dyn_cast<ac::SourceTransactionOp>(op)) {
          auto *plan = analysis.lookup(transaction);
          if (!plan)
            return transaction.emitOpError()
                   << "transaction lowering requires the current complete "
                      "state plan";
          if (failed(admit(*plan)) || !charge(budget, transaction, 2))
            return failure();
          definitions.back().transactionIndices.push_back(transactions.size());
          transactions.push_back({plan, {}, {}});
        }
      }
    }
    for (const auto &prepared : transactions) {
      auto total = staticTotality(*prepared.plan, budget);
      if (failed(total)) return failure();
      if (!prepared.plan->queueAccesses.empty() || !prepared.plan->memoryAccesses.empty()) valueAvailability = true;
      if (!*total) {
        if (!prepared.plan->lexicalModule->getAttrOfType<ArrayAttr>("ac.result_availability"))
          return prepared.plan->transaction->emitOpError()
                 << "ordinary transaction results require compiler-managed availability";
        valueAvailability = true;
      }
    }
    budget.setPhase("prep: live-ins/outputs");
    for (auto &prepared : transactions)
      if (failed(liveIns(prepared)) || failed(outputs(prepared))) return failure();
    budget.setPhase("prep: ordered inputs");
    if (failed(orderedInputs())) return failure();
    budget.setPhase("prep: write domains");
    for (auto &prepared : transactions)
      if (failed(writeDomains(prepared))) return failure();
    budget.setPhase("prep: requirements/expansion/queues/admission");
    for (auto &prepared : transactions)
      if (failed(valueRequirements(prepared)) || failed(expansion(prepared)) ||
          failed(resourceSpecializations(prepared)) || failed(queueRoutes(prepared)) ||
          failed(memoryRoutes(prepared)) ||
          failed(admissionMetadata(prepared)))
        return failure();
    budget.setPhase("prep: prospective queue routes");
    if (failed(queueProspectiveRoutes())) return failure();
    // Every definition participates, including definitions not selected by the
    // System. Summaries are relative to definitions, never root occurrences.
    budget.setPhase("prep: writer summary");
    for (auto &definition : definitions)
      if (failed(writerSummary(definition.module, 0)))
        return failure();
    for (auto module : package.getOps<ac::ModuleOp>())
      for (auto collection :
           module.getBody().front().getOps<ac::CollectionOp>()) {
        auto callee =
            dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(collection));
        if (callee && (!routing(callee).upward.empty() ||
                       !routing(callee).downward.empty() ||
                       !routing(callee).queueIncoming.empty() ||
                       !routing(callee).queuePopSummaries.empty() ||
                       !routing(callee).memoryIncoming.empty()))
          return collection.emitOpError()
                 << "transaction routing cannot extend a definition shared "
                    "with Collection";
      }
    // Prepay full arities, signature growth, route lookup and generated zeros.
    budget.setPhase("prep: route arities/wiring");
    for (auto &definition : definitions) {
      uint64_t ports =
          definition.upward.size() + 2 * definition.downward.size() +
          queueInputCount(definition) + 2 * definition.memoryIncoming.size() + definition.queuePopSummaries.size();
      if (!charge(budget, definition.module, 16 * ports + 1))
        return failure();
      for (auto allocation :
           definition.module.getBody().front().getOps<ac::InstanceOp>()) {
        auto callee =
            dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(allocation));
        if (!callee)
          continue;
        const auto &child = routing(callee);
        if (!charge(budget, allocation,
                    allocation.getNumOperands() + allocation.getNumResults() +
                        32 * (child.upward.size() + 2 * child.downward.size() +
                              queueInputCount(child) + 2 * child.memoryIncoming.size() + child.queuePopSummaries.size()) +
                        1))
          return failure();
        // Each actual occurrence prepends itself to every descendant proposal
        // path, then builds/hashes that path for candidate lookup. Definition
        // union and empty writer summaries do not pay this occurrence cost.
        for (const auto &route : child.downward)
          if (!product(budget, allocation, route.path.size() + 1, 8))
            return failure();
      }
    }
    for (auto &definition : definitions)
      if (failed(wiringMetadata(definition)) || failed(queueWiringMetadata(definition)))
        return failure();
    return success();
  }

  DefinitionRouting &routing(ac::ModuleOp module) {
    return definitions[definitionIndex.lookup(module)];
  }
  RoutingKey key(ArrayRef<ac::InstanceOp> path, Value value) const {
    return {routePath(package->getContext(), path), value};
  }
  ac::HardwareAnalysis hardware;

private:
#include "TransactionQueuePreparation.inc"
#include "TransactionMemoryPreparation.inc"

#include "TransactionSelectedWritePreparation.inc"

  LogicalResult valueRequirements(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    prepared.sourceControls.resize(plan.controls.size(), UINT64_MAX);
    auto control = [&](uint64_t source) {
      prepared.sourceControls[source] = prepared.controls.size();
      prepared.controls.push_back({source, std::nullopt});
    };
    if (!valueAvailability && plan.queueAccesses.empty() && plan.memoryAccesses.empty()) {
      for (uint64_t i = 0; i < plan.controls.size(); ++i) control(i);
      return success();
    }
    Builder b(package.getContext());
    auto append = [&](Operation *site, ac::InvocationFrameId frame,
                      ac::BoundValueId value, StringRef kind, uint64_t operand,
                      ac::TransactionPoint point,
                      std::optional<ac::BoundValueId> path = std::nullopt) -> LogicalResult {
      if (!valueAvailability) return success();
      if (!charge(budget, site, 32)) return failure();
      Operation *function = plan.frames[frame].method
          ? plan.frames[frame].method.operator->() : plan.transaction.operator->();
      auto occurrence = site->getAttrOfType<DictionaryAttr>("occurrence");
      if (!occurrence) occurrence = function->getAttrOfType<DictionaryAttr>(
          isa<ac::SourceTransactionOp>(function) ? "declaration" : "occurrence");
      auto location = site->getAttrOfType<DictionaryAttr>("ac.location");
      if (!location) location = site->getAttrOfType<DictionaryAttr>("ac.condition_location");
      if (!location) location = function->getAttrOfType<DictionaryAttr>("ac.guard_location");
      uint64_t index = prepared.requirements.size();
      prepared.requirements.push_back({site, frame, value, path, point,
          b.getStringAttr(kind), operand, prepared.controls.size(), occurrence, location});
      prepared.controls.push_back({std::nullopt, index});
      return success();
    };
    for (const auto &event : plan.events) {
      using K = ac::TransactionStateEventKind;
      if (event.kind == K::GuardExit) {
        const auto &guard = plan.guards[*event.guard];
        auto site = guard.callable->getRegion(0).front().getTerminator();
        if (failed(append(site, guard.frame, guard.predicate, "guard", 0,
                          {guard.control, plan.controls[guard.control].path})))
          return failure();
        control(guard.control);
      } else if (event.kind == K::BranchEnter &&
                 !plan.branches[*event.branch].selection &&
                 branchHasStateWrites(plan, *event.branch)) {
        const auto &branch = plan.branches[*event.branch];
        if (failed(append(branch.operation, branch.frame, branch.condition,
                          "branch", 0, branch.entry))) return failure();
      } else if (event.kind == K::SelectionEnter) {
        const auto &selection = plan.selections[*event.selection];
        const auto &branch = plan.branches[selection.branch];
        if (failed(append(branch.operation, branch.frame, branch.condition,
                          "branch", 0, branch.entry))) return failure();
        control(selection.control);
      } else if (event.kind == K::Write) {
        const auto &write = plan.writes[*event.write];
        if ((!write.pathFact || !write.pathFact.getValue()) &&
            failed(append(write.operation, write.frame, write.authoredPath,
                          "write_path", 0, event.point))) return failure();
        if (failed(append(write.operation, write.frame, write.value,
                          "state_write", 1, event.point,
                          (!write.pathFact || !write.pathFact.getValue())
                              ? std::optional<ac::BoundValueId>(write.authoredPath)
                              : std::nullopt))) return failure();
        for (auto [ordinal, index] : llvm::enumerate(write.indices))
          if (failed(append(write.operation, write.frame, index, "state_write", 2 + ordinal,
                            event.point, write.authoredPath))) return failure();
      } else if (event.kind == K::QueueAccess) {
        const auto &access = plan.queueAccesses[*event.queueAccess];
        if (access.payload && failed(append(access.operation, access.frame,
              *access.payload, "queue_push", 1, event.point,
              access.authoredPath))) return failure();
        if (failed(queueControl(prepared, *event.queueAccess))) return failure();
      } else if (event.kind == K::MemoryAccess) {
        const auto &access = plan.memoryAccesses[*event.memoryAccess];
        for (auto [ordinal, argument] : llvm::enumerate(access.arguments))
          if (failed(append(access.operation, access.frame, argument,
                            "memory_argument", ordinal, event.point,
                            access.authoredPath))) return failure();
        if (failed(memoryControl(prepared, *event.memoryAccess))) return failure();
      } else if (event.kind == K::FormalWrite) {
        const auto &formal = plan.formalWrites[*event.formalWrite];
        if (!formal.owner) continue;
        if (failed(append(formal.operation, formal.frame, formal.authoredPath,
                          "formal_path", 0, event.point)) ||
            failed(append(formal.operation, formal.frame, formal.payload,
                          "formal_write", 1, event.point, formal.authoredPath)) ||
            failed(append(formal.operation, formal.frame, formal.candidate,
                          "formal_candidate", 2, event.point, formal.authoredPath)))
          return failure();
        for (auto [ordinal, index] : llvm::enumerate(formal.indices))
          if (failed(append(formal.operation, formal.frame, index, "formal_write", 3 + ordinal,
                            event.point, formal.authoredPath))) return failure();
      } else if (event.kind == K::CallEnter) {
        const auto &call = plan.calls[*event.call];
        for (auto [ordinal, argument] : llvm::enumerate(call.arguments))
          if (failed(append(call.operation, call.caller, argument,
                            "call_argument", ordinal, event.point))) return failure();
      } else if (event.kind == K::BodyCheck) {
        const auto &check = plan.bodyChecks[*event.bodyCheck];
        if (failed(append(check.operation, check.frame, check.condition,
                          "assert", 0, event.point))) return failure();
      } else if (event.kind == K::Observe) {
        const auto &observation = plan.observations[*event.observation];
        for (auto [ordinal, value] : llvm::enumerate(observation.values))
          if (failed(append(observation.operation, observation.frame, value,
                            "log", ordinal, event.point))) return failure();
      } else if (event.kind == K::CallExit) {
        const auto &call = plan.calls[*event.call];
        const auto &frame = plan.frames[call.callee];
        ac::SourceMethodView method{plan.instances[frame.anchor].definition,
                                    frame.method, {}};
        auto site = frame.method->getRegion(1).front().getTerminator();
        for (uint64_t ordinal = 0; ordinal < method.getNumPublicResults(); ++ordinal)
          if (failed(append(site, call.callee, call.results[ordinal].yielded,
                            "return", ordinal, frame.exit))) return failure();
      } else if (event.kind == K::TransactionExit) {
        auto rootMethod = plan.frames[plan.rootFrame].method;
        Operation *root = rootMethod ? rootMethod.operator->() : plan.transaction.operator->();
        auto site = root->getRegion(1).front().getTerminator();
        for (auto [ordinal, result] : llvm::enumerate(plan.results))
          if (failed(append(site, plan.rootFrame, result.yielded, "return",
                            ordinal, event.point))) return failure();
      }
    }
    if (llvm::is_contained(prepared.sourceControls, UINT64_MAX))
      return plan.transaction->emitOpError() << "required-use control schedule is incomplete";
    return success();
  }

  DictionaryAttr stateID(ArrayRef<ac::InstanceOp> path,
                         ac::SourceStateOp state) {
    Builder b(package.getContext());
    return b.getDictionaryAttr(
        {b.getNamedAttr("owner", b.getDictionaryAttr({b.getNamedAttr(
                                     "instance_path",
                                     routePath(package.getContext(), path))})),
         b.getNamedAttr("declaration", state.getOccurrence()),
         b.getNamedAttr("element", b.getArrayAttr({}))});
  }

  LogicalResult admissionMetadata(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    auto transaction = plan.transaction;
    Builder b(package.getContext());
    prepared.clockCapture = llvm::find(prepared.liveIns, plan.clockValue) -
                            prepared.liveIns.begin();
    prepared.resetCapture = llvm::find(prepared.liveIns, plan.resetValue) -
                            prepared.liveIns.begin();
    auto add = [&](uint64_t lhs, uint64_t rhs,
                   StringRef description) -> FailureOr<uint64_t> {
      if (rhs > std::numeric_limits<uint64_t>::max() - lhs) {
        transaction.emitOpError() << description << " count overflows";
        return failure();
      }
      return lhs + rhs;
    };
    auto invocation = [&](ac::InvocationFrameId frame) -> FailureOr<ArrayAttr> {
      SmallVector<ac::InvocationFrameId> chain;
      while (plan.frames[frame].parent) {
        if (!charge(budget, transaction, 16))
          return failure();
        chain.push_back(frame);
        frame = *plan.frames[frame].parent;
      }
      SmallVector<Attribute> result;
      for (auto id : llvm::reverse(chain)) {
        const auto &binding = plan.frames[id];
        result.push_back(b.getDictionaryAttr(
            {b.getNamedAttr("call", binding.callOccurrence),
             b.getNamedAttr("receiver", binding.receiver)}));
      }
      return b.getArrayAttr(result);
    };
    auto owner = [&](ac::InvocationFrameId frame) {
      if (plan.frames[frame].parent)
        return plan.frames[frame].receiver;
      return b.getDictionaryAttr(
          {b.getNamedAttr("instance_path", b.getArrayAttr({}))});
    };
    auto fire = add(transaction->getNumResults(),
                    SourceCheckBudget::multiply(2, prepared.outputs.size()),
                    "transaction fire result");
    if (failed(fire))
      return failure();
    prepared.fireResult = *fire;
    auto next = add(prepared.fireResult, 1, "transaction observation layout");
    if (failed(next))
      return failure();
    prepared.observationPathResults.clear();
    for (const auto &observation : plan.observations) {
      prepared.observationPathResults.push_back(*next);
      auto width =
          add(1, observation.values.size(), "transaction observation payload");
      if (failed(width))
        return failure();
      next = add(*next, *width, "transaction observation layout");
      if (failed(next))
        return failure();
    }
    prepared.checkBase = *next;
    auto checkCount = add(prepared.controls.size(), plan.bodyChecks.size(),
                          "transaction check inventory");
    if (failed(checkCount) ||
        *checkCount > std::numeric_limits<uint64_t>::max() / 2)
      return transaction.emitOpError()
             << "transaction check result count overflows";
    auto total =
        add(prepared.checkBase, 2 * *checkCount, "transaction result layout");
    if (failed(total) || *total > SmallVector<Value>().max_size() ||
        *total > SmallVector<Type>().max_size())
      return transaction.emitOpError()
             << "transaction result layout exceeds host capacity";
    prepared.totalResults = SourceCheckBudget::add(*total, prepared.entryOwners.size());
    for (auto &proposal : prepared.queueProposals) {
      proposal.rawResult = prepared.totalResults++;
      proposal.pathResult = prepared.totalResults++;
      if (plan.queueAccesses[proposal.access].payload)
        proposal.dataResult = prepared.totalResults++;
    }
    for (auto &proposal : prepared.memoryProposals) {
      proposal.rawResult = prepared.totalResults++;
      proposal.pathResult = prepared.totalResults++;
      for (auto argument : plan.memoryAccesses[proposal.access].arguments) {
        (void)argument;
        proposal.argumentResults.push_back(prepared.totalResults++);
      }
    }
    for (auto &domain : prepared.writeDomains) {
      if (!charge(budget, transaction)) return failure();
      domain.result = prepared.totalResults;
      prepared.totalResults = SourceCheckBudget::add(prepared.totalResults, 1);
      if (prepared.totalResults == std::numeric_limits<uint64_t>::max())
        return transaction.emitOpError() << "write domain result count overflows";
    }
    SmallVector<Attribute> guards, bodyChecks, observations, checks, requirements;
    auto projectedPath = [&](ac::TransactionSourcePath source) -> FailureOr<DictionaryAttr> {
      auto path = sampledPath(plan, source, budget);
      if (failed(path) || path->empty()) return path;
      NamedAttrList mapped(*path);
      uint64_t index = path->getAs<IntegerAttr>("selection").getInt();
      mapped.set("selection", b.getI64IntegerAttr(prepared.sourceControls[index]));
      return mapped.getDictionary(b.getContext());
    };
    for (auto [ordinal, scheduled] : llvm::enumerate(prepared.controls)) {
      const ac::AdmissionControl *original = scheduled.source
          ? &plan.controls[*scheduled.source] : nullptr;
      const auto *requirement = scheduled.requirement
          ? &prepared.requirements[*scheduled.requirement] : nullptr;
      const auto *resource = scheduled.resource ? &prepared.resources[*scheduled.resource] : nullptr;
      ac::TransactionSourcePath sourcePath = original ? original->path : requirement
          ? requirement->point.path : resourcePoint(plan, *resource).path;
      ac::InvocationFrameId frame;
      DictionaryAttr occurrence, location;
      StringRef kind;
      if (resource) {
        frame = resourceFrame(plan, *resource);
        occurrence = resourceOccurrence(plan, *resource);
        location = resourceLocation(plan, *resource);
        kind = "resource_available";
      } else if (requirement) {
        frame = requirement->frame;
        occurrence = requirement->occurrence;
        location = requirement->location;
        kind = "value_available";
        requirements.push_back(b.getDictionaryAttr({
            b.getNamedAttr("control", b.getI64IntegerAttr(ordinal)),
            b.getNamedAttr("kind", requirement->kind),
            b.getNamedAttr("operand", b.getI64IntegerAttr(requirement->operand)),
            b.getNamedAttr("occurrence", occurrence),
            b.getNamedAttr("location", location)}));
      } else if (auto *binding =
              std::get_if<ac::CallableGuardControl>(&original->boundary)) {
        const auto &guard = plan.guards[binding->guard];
        frame = guard.frame;
        occurrence = guard.callable->getAttrOfType<DictionaryAttr>(
            isa<ac::SourceTransactionOp>(guard.callable) ? "declaration"
                                                         : "occurrence");
        location = guard.location;
        kind = "callable_guard";
      } else {
        auto selection =
            std::get<ac::BranchSelectControl>(original->boundary).selection;
        const auto &branch = plan.branches[plan.selections[selection].branch];
        frame = branch.frame;
        occurrence = branch.occurrence;
        location = branch.conditionLocation;
        kind = "branch_select";
      }
      auto frames = invocation(frame);
      auto path = projectedPath(sourcePath);
      if (failed(frames) || failed(path) ||
          !charge(budget, transaction, 64 + 2 * frames->size()))
        return failure();
      auto id = b.getDictionaryAttr(
          {b.getNamedAttr("registration", transaction.getOccurrence()),
           b.getNamedAttr("check", occurrence),
           b.getNamedAttr("obligation", b.getI64IntegerAttr(ordinal))});
      guards.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("id", id), b.getNamedAttr("invocation", *frames),
           b.getNamedAttr("kind", b.getStringAttr(kind)),
           b.getNamedAttr("path", *path),
           b.getNamedAttr("predecessor",
                          b.getI64IntegerAttr(int64_t(ordinal) - 1))}));
      checks.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("id", id),
           b.getNamedAttr("kind", b.getStringAttr("guard_known")),
           b.getNamedAttr("location", location)}));
    }
    for (auto [ordinal, check] : llvm::enumerate(plan.bodyChecks)) {
      auto frames = invocation(check.frame);
      if (failed(frames) || !charge(budget, transaction, 40 + frames->size()))
        return failure();
      auto id = b.getDictionaryAttr(
          {b.getNamedAttr("registration", transaction.getOccurrence()),
           b.getNamedAttr("check", check.occurrence),
           b.getNamedAttr("obligation", b.getI64IntegerAttr(
                                            prepared.controls.size() + ordinal))});
      auto path = projectedPath(check.point.path);
      if (failed(path))
        return failure();
      bodyChecks.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("id", id), b.getNamedAttr("invocation", *frames),
           b.getNamedAttr("path", *path)}));
      checks.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("id", id),
           b.getNamedAttr("kind", b.getStringAttr("assert")),
           b.getNamedAttr("location", check.location)}));
    }
    for (auto [ordinal, observation] : llvm::enumerate(plan.observations)) {
      auto frames = invocation(observation.frame);
      if (failed(frames) || !charge(budget, transaction, 48 + frames->size()))
        return failure();
      auto id = b.getDictionaryAttr(
          {b.getNamedAttr("registration", transaction.getOccurrence()),
           b.getNamedAttr("site", observation.occurrence),
           b.getNamedAttr("ordinal", b.getI64IntegerAttr(ordinal)),
           b.getNamedAttr("invocation", *frames),
           b.getNamedAttr("owner", owner(observation.frame))});
      auto path = projectedPath(observation.point.path);
      if (failed(path))
        return failure();
      observations.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("id", id), b.getNamedAttr("path", *path),
           b.getNamedAttr("value_count",
                          b.getI64IntegerAttr(observation.values.size()))}));
    }
    prepared.expectedAdmission = b.getDictionaryAttr(
        {b.getNamedAttr("clock_capture",
                        b.getI64IntegerAttr(prepared.clockCapture)),
         b.getNamedAttr("reset_capture",
                        b.getI64IntegerAttr(prepared.resetCapture)),
         b.getNamedAttr("fire_result",
                        b.getI64IntegerAttr(prepared.fireResult)),
         b.getNamedAttr("controls", b.getArrayAttr(guards)),
         b.getNamedAttr("body_checks", b.getArrayAttr(bodyChecks)),
         b.getNamedAttr("observations", b.getArrayAttr(observations))});
    if (!requirements.empty()) {
      NamedAttrList admission(prepared.expectedAdmission);
      admission.append("requirements", b.getArrayAttr(requirements));
      prepared.expectedAdmission = admission.getDictionary(b.getContext());
    }
    if (!prepared.entryOwners.empty()) {
      SmallVector<Attribute> entries;
      for (auto [ordinal, ownerIndex] : llvm::enumerate(prepared.entryOwners)) {
        const auto &state = plan.owners[ownerIndex];
        SmallVector<Attribute> predecessors;
        for (const auto &forward : prepared.forwarding) {
          if (forward.owner != ownerIndex) continue;
          predecessors.push_back(b.getDictionaryAttr({
              b.getNamedAttr("registration", transactions[forward.predecessor].plan->transaction->getAttrOfType<DictionaryAttr>("occurrence")),
              b.getNamedAttr("data_capture", b.getI64IntegerAttr(forward.dataCapture)),
              b.getNamedAttr("enable_capture", b.getI64IntegerAttr(forward.enableCapture))}));
        }
        auto oldQ = llvm::find(prepared.liveIns, state.oldQ) - prepared.liveIns.begin();
        entries.push_back(b.getDictionaryAttr({
            b.getNamedAttr("state", stateID(anchorPath(plan, state.anchor), state.declaration)),
            b.getNamedAttr("old_q_capture", b.getI64IntegerAttr(oldQ)),
            b.getNamedAttr("predecessors", b.getArrayAttr(predecessors)),
            b.getNamedAttr("result", b.getI64IntegerAttr(*total + ordinal))}));
      }
      NamedAttrList admission(prepared.expectedAdmission);
      admission.append("ehr_entries", b.getArrayAttr(entries));
      prepared.expectedAdmission = admission.getDictionary(b.getContext());
    }
    if (failed(queueAdmissionMetadata(prepared)) || failed(memoryAdmissionMetadata(prepared))) return failure();
    if (!prepared.writeDomains.empty()) {
      SmallVector<Attribute> domains;
      for (const auto &domain : prepared.writeDomains) {
        if (!charge(budget, transaction, 16)) return failure();
        domains.push_back(b.getDictionaryAttr({
            b.getNamedAttr("candidate", b.getI64IntegerAttr(domain.candidate)),
            b.getNamedAttr("selectors", domain.selectors),
            b.getNamedAttr("mask", b.getI64IntegerAttr(domain.result))}));
      }
      NamedAttrList admission(prepared.expectedAdmission);
      admission.append("write_domains", b.getArrayAttr(domains));
      prepared.expectedAdmission = admission.getDictionary(b.getContext());
    }
    prepared.expectedRequiredChecks = b.getArrayAttr(checks);
    return success();
  }

  LogicalResult wiringMetadata(DefinitionRouting &definition) {
    if (definition.stateIndices.empty() &&
        definition.transactionIndices.empty() && definition.upward.empty() &&
        definition.downward.empty() && definition.queueIncoming.empty() &&
        definition.queuePopSummaries.empty())
      return success();
    Builder b(package.getContext());
    SmallVector<Attribute> inventory, incoming, writers;
    for (auto index : definition.stateIndices) {
      if (!charge(budget, definition.module, 4))
        return failure();
      inventory.push_back(states[index].getOccurrence());
    }
    for (auto [ordinal, route] : llvm::enumerate(definition.downward)) {
      if (!charge(budget, definition.module, 32 + route.path.size() * 8))
        return failure();
      incoming.push_back(b.getDictionaryAttr(
          {b.getNamedAttr(
               "state",
               stateID(route.path,
                       route.value.getDefiningOp<ac::SourceStateOp>())),
           b.getNamedAttr("data", b.getI64IntegerAttr(definition.sourceInputs +
                                                      2 * ordinal)),
           b.getNamedAttr("enable",
                          b.getI64IntegerAttr(definition.sourceInputs +
                                              2 * ordinal + 1))}));
    }
    for (auto index : definition.transactionIndices) {
      const auto &prepared = transactions[index];
      const auto &plan = *prepared.plan;
      SmallVector<Attribute> candidates;
      for (auto [ordinal, output] : llvm::enumerate(prepared.outputs)) {
        const auto &owner = plan.owners[output.binding.owner];
        auto path = anchorPath(plan, owner.anchor);
        if (!charge(budget, definition.module, 32 + path.size() * 8))
          return failure();
        candidates.push_back(b.getDictionaryAttr(
            {b.getNamedAttr("state", stateID(path, owner.declaration)),
             b.getNamedAttr("data", b.getI64IntegerAttr(output.dataResult)),
             b.getNamedAttr("enable",
                            b.getI64IntegerAttr(output.enableResult))}));
      }
      if (!charge(budget, definition.module, 16))
        return failure();
      writers.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("registration",
                          plan.transaction->getAttr("occurrence")),
           b.getNamedAttr(
               "ordinary_results",
               b.getI64IntegerAttr(plan.transaction->getNumResults())),
           b.getNamedAttr("candidates", b.getArrayAttr(candidates))}));
    }
    definition.expectedWiring = b.getDictionaryAttr(
        {b.getNamedAttr("states", b.getArrayAttr(inventory)),
         b.getNamedAttr("incoming", b.getArrayAttr(incoming)),
         b.getNamedAttr("writers", b.getArrayAttr(writers))});
    return success();
  }

  LogicalResult admit(const ac::TransactionStatePlan &plan) {
    auto transaction = plan.transaction;
    if (failed(requireStaticTotality(plan, budget,
          bool(plan.lexicalModule->getAttrOfType<ArrayAttr>("ac.result_availability")))))
      return failure();
    for (const auto &owner : plan.owners) {
      if (!charge(budget, transaction, 1 + owner.instancePath.size()))
        return failure();
      auto width = hardware.getPackedWidth(owner.payloadType,
                                           plan.instances[owner.anchor].bindings,
                                           owner.declaration);
      if (failed(width) || !*width)
        return transaction.emitOpError()
               << "transaction state requires a finite storage representation";
    }
    return success();
  }

  LogicalResult demand(ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                       Value value, bool downward) {
    if (!charge(budget, module, 8 + path.size()))
      return failure();
    auto &definition = routing(module);
    auto routeKey = key(path, value);
    auto &index = downward ? definition.downwardIndex : definition.upwardIndex;
    if (index.contains(routeKey))
      return success();
    ac::HardwareBindings bindings;
    bindings.owner = module;
    for (auto allocation : path) {
      auto bound = hardware.bindInstance(allocation, bindings);
      if (failed(bound))
        return failure();
      bindings = *bound;
    }
    auto type = hardware.resolveType(value.getType(), bindings, module);
    if (failed(type))
      return failure();
    Type enableType;
    if (downward) {
      auto state = value.getDefiningOp<ac::SourceStateOp>();
      auto enable =
          hardware.resolveType(state.getClk().getType(), bindings, module);
      if (failed(enable))
        return failure();
      enableType = *enable;
    }
    auto &routes = downward ? definition.downward : definition.upward;
    index.try_emplace(routeKey, routes.size());
    routes.push_back(
        {SmallVector<ac::InstanceOp>(path), value, *type, enableType});
    return success();
  }

  LogicalResult propagate(const ac::TransactionStatePlan &plan,
                          ac::LexicalInstanceId anchor, Value value,
                          bool downward) {
    auto path = anchorPath(plan, anchor);
    // The transaction root consumes/emits the route; it does not need a port
    // for its own transaction. Each descendant exports/imports its suffix.
    for (unsigned offset = 0; offset < path.size(); ++offset) {
      auto child = dyn_cast<ac::ModuleOp>(hardware.resolveCallee(path[offset]));
      if (!child)
        return path[offset].emitOpError()
               << "transaction route requires an ordinary Module";
      if (failed(demand(child,
                        ArrayRef<ac::InstanceOp>(path).drop_front(offset + 1),
                        value, downward)))
        return failure();
    }
    return success();
  }

  LogicalResult liveIns(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    // Bound module SSA records already include lexical captures and old Q.
    // The anchor distinguishes same-definition instances; the frame does not
    // turn a callee lexical capture into a caller variable.
    for (auto [id, ref] : llvm::enumerate(plan.values)) {
      if (!charge(budget, plan.transaction))
        return failure();
      auto module = plan.instances[ref.anchor].definition;
      if (!moduleValue(ref.value, module) ||
          ref.value.getDefiningOp() == plan.transaction.operator->())
        continue;
      prepared.liveIns.push_back(id);
      if (failed(propagate(plan, ref.anchor, ref.value, false)))
        return failure();
    }
    for (auto pin : {plan.clockValue, plan.resetValue}) {
      if (!llvm::is_contained(prepared.liveIns, pin)) {
        if (!charge(budget, plan.transaction, 4))
          return failure();
        prepared.liveIns.push_back(pin);
        const auto &ref = plan.values[pin];
        if (failed(propagate(plan, ref.anchor, ref.value, false)))
          return failure();
      }
    }
    return success();
  }

  LogicalResult outputs(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    for (auto binding : plan.finalVersions) {
      if (!charge(budget, plan.transaction))
        return failure();
      bool written = false;
      for (const auto &write : plan.writes) {
        if (!charge(budget, plan.transaction))
          return failure();
        written |= write.owner == binding.owner;
      }
      if (!written)
        continue;
      if (!charge(budget, plan.transaction,
                  1 + SourceCheckBudget::bytes(
                          sizeof(detail::TransactionCandidatePreparation))))
        return failure();
      uint64_t data = SourceCheckBudget::add(
          plan.transaction->getNumResults(),
          SourceCheckBudget::multiply(2, prepared.outputs.size()));
      uint64_t enable = SourceCheckBudget::add(data, 1);
      if (enable == std::numeric_limits<uint64_t>::max())
        return plan.transaction->emitOpError()
               << "transaction candidate result count overflows";
      prepared.outputs.push_back({binding, data, enable});
      const auto &owner = plan.owners[binding.owner];
      if (!charge(budget, plan.transaction, prepared.liveIns.size()) ||
          !llvm::is_contained(prepared.liveIns, owner.oldQ))
        return plan.transaction->emitOpError()
               << "transaction state old-Q capture is missing";
      auto declaration = owner.declaration;
      if (failed(propagate(plan, owner.anchor, declaration.getQ(), true)))
        return failure();
    }
    return success();
  }

  LogicalResult orderedInputs() {
    llvm::DenseMap<Operation *, uint64_t> indices;
    for (auto [index, prepared] : llvm::enumerate(transactions))
      indices.try_emplace(prepared.plan->transaction, index);
    llvm::DenseSet<uint64_t> visited;
    std::function<LogicalResult(uint64_t)> schedule = [&](uint64_t index) {
      if (!charge(budget, transactions[index].plan->transaction)) return failure();
      if (!visited.insert(index).second) return success();
      for (auto predecessor : transactions[index].plan->predecessors) {
        auto found = indices.find(predecessor);
        if (found == indices.end() || failed(schedule(found->second))) return failure();
      }
      transactionSchedule.push_back(index);
      return success();
    };
    for (uint64_t index = 0; index < transactions.size(); ++index)
      if (failed(schedule(index))) return failure();
    for (auto &prepared : transactions) {
      const auto &plan = *prepared.plan;
      for (auto [ownerIndex, owner] : llvm::enumerate(plan.owners)) {
        auto ownerKey = key(anchorPath(plan, owner.anchor), owner.declaration->getResult(0));
        for (auto predecessor : plan.predecessors) {
          uint64_t index = indices.lookup(predecessor);
          const auto &prior = transactions[index];
          for (auto [ordinal, output] : llvm::enumerate(prior.outputs)) {
            const auto &priorOwner = prior.plan->owners[output.binding.owner];
            if (ownerKey != key(anchorPath(*prior.plan, priorOwner.anchor),
                                priorOwner.declaration->getResult(0))) continue;
            if (!prepayAppend(budget, plan.transaction, prepared.forwarding)) return failure();
            uint64_t capture = prepared.liveIns.size() + 2 * prepared.forwarding.size();
            prepared.forwarding.push_back({ownerIndex, index, ordinal, capture, capture + 1});
          }
        }
        if (!prepared.forwarding.empty() && prepared.forwarding.back().owner == ownerIndex)
          prepared.entryOwners.push_back(ownerIndex);
      }
    }
    return success();
  }

  bool ordered(ac::SourceTransactionOp first, ac::SourceTransactionOp second) {
    auto *plan = analysis.lookup(second);
    return plan && llvm::is_contained(plan->predecessors, first);
  }

  struct Writer {
    SmallVector<ac::InstanceOp> ownerPath;
    Value state;
    SmallVector<ac::InstanceOp> transactionPath;
    ac::SourceTransactionOp transaction;
    const ac::TransactionStatePlan *plan = nullptr;
    ac::StateOwnerId owner = 0;
  };

  LogicalResult writerSummary(ac::ModuleOp module, unsigned depth) {
    if (depth > 256)
      return module.emitOpError()
             << "transaction routing path depth budget exceeded";
    auto status = summaryStatus.lookup(module);
    if (status == 2)
      return success();
    if (status == 1)
      return module.emitOpError() << "transaction routing requires "
                                     "nonrecursive instance definitions";
    summaryStatus[module] = 1;
    auto &summary = summaries[module];
    std::unique_ptr<SourceWriteGrantProof> grant;
    SmallVector<std::pair<RoutingKey, unsigned>, 0> groupModes;
    auto add = [&](Writer writer) -> LogicalResult {
      if (!charge(budget, module, 8 + writer.ownerPath.size() + writer.transactionPath.size()))
        return failure();
      auto ownerKey = key(writer.ownerPath, writer.state);
      for (const auto &prior : summary) {
        if (!charge(budget, module)) return failure();
        if (ownerKey != key(prior.ownerPath, prior.state)) continue;
        if (prior.transaction == writer.transaction &&
            prior.transactionPath == writer.transactionPath) return success();
        bool sameScope = prior.transactionPath == writer.transactionPath;
        bool pairOrdered = sameScope &&
            (ordered(prior.transaction, writer.transaction) ||
             ordered(writer.transaction, prior.transaction));
        // Partial selector domains cross scopes are admitted only through the
        // same independent grant proof as whole-owner writers. A different
        // field is disjoint; same-field/index pairs must prove mutually
        // exclusive enables below. Cross-scope after composition has no
        // accepted visibility contract and fails at this earlier owner.
        if (!sameScope &&
            ((prior.plan && !prior.plan->predecessors.empty()) ||
             (writer.plan && !writer.plan->predecessors.empty())))
          return module.emitOpError()
                 << "cross-scope writers cannot participate in after composition";
        auto mode = llvm::find_if(groupModes, [&](const auto &entry) { return entry.first == ownerKey; });
        unsigned expectedMode = pairOrdered ? 1 : 2;
        if (mode == groupModes.end()) {
          if (!prepayAppend(budget, module, groupModes)) return failure();
          groupModes.push_back({ownerKey, expectedMode});
        } else if (mode->second != expectedMode)
          return module.emitOpError() << "mixed ordered and unordered owner writer groups require an explicit supported composition contract";
        if (!pairOrdered) {
          if (!prior.plan || !writer.plan) return failure();
          if (!grant) {
            if (!charge(budget, module, 1 + SourceCheckBudget::bytes(sizeof(SourceWriteGrantProof)))) return failure();
            grant = std::make_unique<SourceWriteGrantProof>(module, hardware, budget);
          }
          for (auto [left, first] : llvm::enumerate(prior.plan->writes)) {
            if (!charge(budget, module)) return failure();
            if (first.owner != prior.owner || first.forwardedProposal) continue;
            for (auto [right, second] : llvm::enumerate(writer.plan->writes)) {
              if (!charge(budget, module)) return failure();
              if (second.owner != writer.owner || second.forwardedProposal) continue;
              auto disjoint = grant->disjoint(*prior.plan, prior.transactionPath, left,
                                               *writer.plan, writer.transactionPath, right);
              if (failed(disjoint)) return module.emitOpError()
                  << "source write grant proof could not resolve scoped SSA: " << grant->failureReason();
              if (!*disjoint) {
                auto diagnostic = module.emitOpError()
                    << "multiple outer writers require a declared transaction order or proved disjoint write domains";
                diagnostic.attachNote(prior.transaction->getLoc())
                    << "previous rule registration is here";
                diagnostic.attachNote(writer.transaction.getLoc())
                    << "conflicting rule registration is here";
                return failure();
              }
            }
          }
        }
      }
      summary.push_back(std::move(writer));
      return success();
    };
    for (unsigned index : routing(module).transactionIndices) {
      if (!charge(budget, module))
        return failure();
      const auto &prepared = transactions[index];
      const auto &plan = *prepared.plan;
      for (const auto &write : plan.writes) {
        const auto &owner = plan.owners[write.owner];
        if (!charge(budget, module, 1 + owner.instancePath.size()))
          return failure();
        auto declaration = owner.declaration;
        if (failed(add({anchorPath(plan, owner.anchor),
                        declaration.getQ(),
                        {},
                        plan.transaction, &plan, write.owner})))
          return failure();
      }
    }
    for (Operation &op : module.getBody().front()) {
      if (!charge(budget, &op))
        return failure();
      ac::ModuleOp child;
      ac::InstanceOp allocation = dyn_cast<ac::InstanceOp>(op);
      auto collection = dyn_cast<ac::CollectionOp>(op);
      if (allocation)
        child =
            dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(allocation));
      else if (collection)
        child =
            dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(collection));
      if (!child)
        continue;
      if (failed(writerSummary(child, depth + 1)))
        return failure();
      if (collection) {
        if (!summaries[child].empty())
          return collection.emitOpError()
                 << "transaction writer summaries cannot cross Collection";
        continue;
      }
      for (const auto &childWriter : summaries[child]) {
        if (!charge(budget, module,
                    3 + childWriter.ownerPath.size() +
                        childWriter.transactionPath.size()))
          return failure();
        Writer writer = childWriter;
        writer.ownerPath.insert(writer.ownerPath.begin(), allocation);
        writer.transactionPath.insert(writer.transactionPath.begin(),
                                      allocation);
        if (failed(add(std::move(writer))))
          return failure();
      }
    }
    summaryStatus[module] = 2;
    return success();
  }

  LogicalResult expansion(const TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    auto transaction = plan.transaction;
    if (plan.versions.size() >
            SmallVector<MaterializedVersion, 0>().max_size() ||
        plan.branchResults.size() >
            SmallVector<MaterializedBranchResult, 0>().max_size() ||
        plan.frames.size() > SmallVector<IRMapping, 0>().max_size())
      return transaction.emitOpError()
             << "transaction materialization storage exceeds host capacity";
    uint64_t storage = SourceCheckBudget::add(
        SourceCheckBudget::multiply(plan.frames.size(), sizeof(IRMapping)),
        SourceCheckBudget::add(
            SourceCheckBudget::multiply(plan.values.size(), sizeof(Value)),
            SourceCheckBudget::add(
                SourceCheckBudget::multiply(plan.versions.size(),
                                            sizeof(MaterializedVersion)),
                SourceCheckBudget::multiply(
                    plan.branchResults.size() + plan.joins.size() + plan.writes.size(),
                    sizeof(MaterializedBranchResult)))));
    storage = SourceCheckBudget::add(
        storage, SourceCheckBudget::add(
                     SourceCheckBudget::multiply(prepared.controls.size() + 1,
                                                 sizeof(Value)),
                     SourceCheckBudget::add(
                         SourceCheckBudget::multiply(
                             prepared.controls.size(),
                             sizeof(detail::MaterializedAdmissionControl)),
                         SourceCheckBudget::multiply(
                             plan.selections.size(),
                             sizeof(detail::MaterializedSelection)))));
    storage = SourceCheckBudget::add(storage,
        SourceCheckBudget::multiply(prepared.writeDomains.empty() ? 0 : plan.writes.size(),
                                    sizeof(detail::MaterializedSelectedWrite)));
    uint64_t effectValues = 0;
    for (const auto &observation : plan.observations)
      effectValues =
          SourceCheckBudget::add(effectValues, observation.values.size());
    storage = SourceCheckBudget::add(
        storage, SourceCheckBudget::multiply(
                     plan.bodyChecks.size() + plan.observations.size(),
                     sizeof(const ac::TransactionStateEvent *)));
    if (!charge(budget, transaction, SourceCheckBudget::bytes(storage)) ||
        !product(budget, transaction, prepared.controls.size(), 64) ||
        !charge(
            budget, transaction,
            SourceCheckBudget::add(
                SourceCheckBudget::multiply(
                    plan.bodyChecks.size(),
                    SourceCheckBudget::bytes(sizeof(std::pair<Value, Value>) +
                                             sizeof(ac::SourceExpectOp))),
                SourceCheckBudget::add(
                    SourceCheckBudget::multiply(
                        plan.observations.size(),
                        SourceCheckBudget::bytes(
                            sizeof(SavedMaterializedObservation) +
                            sizeof(ac::SourceObserveOp))),
                    SourceCheckBudget::multiply(
                        effectValues,
                        SourceCheckBudget::bytes(sizeof(Value)))))))
      return failure();
    if (!charge(budget, transaction,
                plan.values.size() + plan.instances.size() +
                    plan.versions.size() + plan.reads.size() +
                    plan.writes.size() + plan.calls.size() +
                    plan.bodyChecks.size() + plan.observations.size() +
                    plan.frames.size() + plan.results.size() +
                    plan.branchResults.size() + prepared.outputs.size()))
      return failure();
    for (const auto &result : plan.branchResults) {
      auto branch = plan.branches[result.branch].operation;
      auto layout =
          branchJoinLayout(result.payloadType, hardware,
                           plan.frames[result.frame].bindings, branch, budget, &branchTypes);
      if (failed(layout) || !prepayBranchJoin(budget, branch, *layout))
        return failure();
    }
    for (const auto &join : plan.joins) {
      const auto &branch = plan.branches[join.branch];
      auto layout = branchJoinLayout(plan.owners[join.owner].payloadType,
                                     hardware, plan.frames[branch.frame].bindings,
                                     branch.operation, budget, &branchTypes);
      if (failed(layout) || !prepayBranchJoin(budget, branch.operation, *layout))
        return failure();
    }
    for (const auto &write : plan.writes) {
      if (write.pathFact && write.pathFact.getValue())
        continue;
      auto layout = branchJoinLayout(plan.owners[write.owner].payloadType,
          hardware, plan.frames[write.frame].bindings, write.operation, budget, &branchTypes);
      if (failed(layout) || !prepayBranchJoin(budget, write.operation, *layout))
        return failure();
    }
    for (const auto &frame : plan.frames) {
      auto method = frame.method;
      Operation *function =
          method ? method.operator->() : transaction.operator->();
      if (!charge(budget, function,
                  frame.guardArguments.size() + frame.bodyArguments.size()))
        return failure();
      SmallVector<Operation *> pending;
      for (Region &region : function->getRegions())
        for (Operation &op : region.front()) {
          if (!charge(budget, function))
            return failure();
          pending.push_back(&op);
        }
      while (!pending.empty()) {
        Operation *op = pending.pop_back_val();
        if (!charge(budget, op,
                    3 + uint64_t(op->getNumOperands()) * 2 +
                        uint64_t(op->getNumResults()) * 2 +
                        op->getNumRegions() +
                        op->getRawDictionaryAttrs().size() +
                        op->getPropertiesStorageSize()))
          return failure();
        if (!isa<ac::SourceStateReadOp, ac::SourceStateWriteOp,
                 ac::SourceFormalWriteOp, ac::SourceMethodCallOp, ac::SourceExpectOp,
                 ac::SourceObserveOp, ac::SourceBranchOp, ac::SourceQueuePopOp,
                 ac::SourceQueuePeekOp, ac::SourceQueuePushOp, ac::YieldOp>(op) &&
            !ac::detail::isCombinationalHardwareOperation(op))
          return op->emitOpError()
                 << "transaction lowering cannot materialize this operation";
        for (Region &region : op->getRegions())
          for (Block &block : region) {
            if (!charge(budget, op, 1 + uint64_t(block.getNumArguments()) * 2))
              return failure();
            for (Operation &nested : block) {
              if (!charge(budget, op))
                return failure();
              pending.push_back(&nested);
            }
          }
      }
    }
    return success();
  }

  mlir::ModuleOp package;
  const ac::SourceTransactionStateAnalysis &analysis;
  SourceCheckBudget &budget;
  llvm::DenseMap<Operation *, unsigned> summaryStatus;
  std::map<Operation *, SmallVector<Writer>> summaries;
  BranchTypeAdmissionCache branchTypes;
  unsigned dffeCount = 0;
};

struct StateStorage {
  ac::SourceStateOp declaration;
  Operation *instance;
  bool dff = false;
};

/// Translate a definition-relative demand through the rebuilt physical
/// signature. Original Value and Instance identities remain the lookup keys.
class MaterializationRoutes {
public:
  MaterializationRoutes(const PreparedTransactionLowering &prepared,
                        IRMapping &mapping)
      : prepared(prepared), mapping(mapping), hardware(prepared.original) {}

  FailureOr<Value> value(ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                         Value original) {
    if (path.empty()) {
      auto mapped = mapping.lookupOrNull(original);
      if (mapped)
        return mapped;
      return module.emitOpError()
             << "transaction lexical SSA mapping is missing";
    }
    auto child = cast<ac::ModuleOp>(hardware.resolveCallee(path.front()));
    auto &definition =
        prepared
            .definitions[prepared.definitionIndex.lookup(child.operator->())];
    auto found =
        definition.upwardIndex.find(routingKey(path.drop_front(), original));
    if (found == definition.upwardIndex.end())
      return module.emitOpError() << "transaction upward route is missing";
    auto originalAllocation = path.front();
    auto allocation =
        cast<ac::InstanceOp>(mapping.lookup(originalAllocation.operator->()));
    return allocation.getResult(definition.sourceOutputs + found->second);
  }

  FailureOr<Value> bound(const ac::TransactionStatePlan &plan,
                         ac::BoundValueId id) {
    const auto &ref = plan.values[id];
    auto path = anchorPath(plan, ref.anchor);
    return value(plan.lexicalModule, path, ref.value);
  }

private:
  const PreparedTransactionLowering &prepared;
  IRMapping &mapping;
  ac::HardwareAnalysis hardware;
};

Value bitsLiteral(Type payload, DictionaryAttr occurrence, Location location,
                  bool one, OpBuilder &builder);

FailureOr<MaterializedBranchResult> materializeSelection(
  Operation *branch, const ac::HardwareBindings &bindings,
  Value condition, Value whenTrue, Value whenFalse, Type type,
  OpBuilder &builder, SourceCheckBudget &budget,
  BranchTypeAdmissionCache *cache = nullptr) {
  ac::HardwareAnalysis hardware(branch->getParentOfType<mlir::ModuleOp>());
  auto layout = branchJoinLayout(type, hardware, bindings,
                                 branch, budget, cache);
  if (failed(layout) || !condition || !whenTrue || !whenFalse ||
      !ac::areEquivalentHardwareTypes(whenTrue.getType(), type) ||
      !ac::areEquivalentHardwareTypes(whenFalse.getType(), type) ||
      layout->operations > SmallVector<Operation *, 0>().max_size() ||
      layout->nodes.size() >
          SmallVector<std::pair<uint64_t, bool>, 0>().max_size() / 2 ||
      !prepayBranchJoin(budget, branch, *layout))
    return failure();
  MaterializedBranchResult result;
  result.generated.reserve(layout->operations);
  SmallVector<std::pair<uint64_t, bool>, 0> pending;
  pending.reserve(2 * layout->nodes.size());
  layout->nodes[0].whenTrue = whenTrue;
  layout->nodes[0].whenFalse = whenFalse;
  pending.push_back({0, false});
  auto create = [&](StringRef name, ArrayRef<Value> operands,
                    ArrayRef<Type> types,
                    ArrayRef<NamedAttribute> attributes = {}) {
    OperationState state(branch->getLoc(), name);
    state.addOperands(operands);
    state.addTypes(types);
    state.addAttributes(attributes);
    Operation *op = builder.create(state);
    result.generated.push_back(op);
    return op;
  };
  while (!pending.empty()) {
    auto [index, exiting] = pending.pop_back_val();
    auto &node = layout->nodes[index];
    if (!exiting && node.parent != UINT64_MAX) {
      auto &parent = layout->nodes[node.parent];
      if (isa<ac::TableType>(parent.type)) {
        auto integer = ac::MathIntAttr::get(branch->getContext(),
            llvm::APSInt(llvm::APInt(64, node.ordinal), true));
        auto expression = builder.getDictionaryAttr({
            builder.getNamedAttr("kind", builder.getStringAttr("literal")),
            builder.getNamedAttr("value", builder.getDictionaryAttr({
                builder.getNamedAttr("kind", builder.getStringAttr("integer")),
                builder.getNamedAttr("value", integer)})),
            builder.getNamedAttr("origin", branch->getAttrOfType<DictionaryAttr>("occurrence")),
            builder.getNamedAttr("location", selectionSourceLocation(branch))});
        Value index = create(ac::BitsConstantOp::getOperationName(), {},
                             {branchCarrierType(64, branch)},
                             {builder.getNamedAttr("value", ac::StaticExprAttr::get(
                                 branch->getContext(), expression))})->getResult(0);
        Type membership = branchCarrierType(1, branch);
        node.whenTrue = create(ac::TableGetOp::getOperationName(),
                               {parent.whenTrue, index}, {node.type, membership})->getResult(0);
        node.whenFalse = create(ac::TableGetOp::getOperationName(),
                                {parent.whenFalse, index}, {node.type, membership})->getResult(0);
      } else {
        auto field = builder.getNamedAttr("field", node.field);
        node.whenTrue = create(ac::StructGetOp::getOperationName(),
                               {parent.whenTrue}, {node.type}, {field})->getResult(0);
        node.whenFalse = create(ac::StructGetOp::getOperationName(),
                                {parent.whenFalse}, {node.type}, {field})->getResult(0);
      }
    }
    if (isa<ac::StructType, ac::TableType>(node.type) && !exiting) {
      pending.push_back({index, true});
      for (auto child : llvm::reverse(node.children))
        pending.push_back({child, false});
      continue;
    }
    if (isa<ac::BitsType>(node.type)) {
      node.joined =
          create(ac::BitsSelectOp::getOperationName(),
                 {condition, node.whenTrue, node.whenFalse}, {node.type})
              ->getResult(0);
    } else if (isa<ac::EnumType>(node.type)) {
      Type carrier = branchCarrierType(node.enumWidth, branch);
      Type membership = branchCarrierType(1, branch);
      Value t = create(ac::EnumToBitsOp::getOperationName(), {node.whenTrue},
                       {carrier})
                    ->getResult(0);
      Value f = create(ac::EnumToBitsOp::getOperationName(), {node.whenFalse},
                       {carrier})
                    ->getResult(0);
      Value selected = create(ac::BitsSelectOp::getOperationName(),
                              {condition, t, f}, {carrier})
                           ->getResult(0);
      node.joined = create(ac::EnumFromBitsOp::getOperationName(), {selected},
                           {node.type, membership})
                        ->getResult(0);
    } else {
      node.joined = create(isa<ac::TableType>(node.type)
                               ? ac::TableCreateOp::getOperationName()
                               : ac::StructCreateOp::getOperationName(),
                           node.fields, {node.type})
                        ->getResult(0);
    }
    if (node.parent != UINT64_MAX)
      layout->nodes[node.parent].fields.push_back(node.joined);
  }
  if (result.generated.size() != layout->operations)
    return branch->emitOpError()
           << "local branch join operation count differs";
  result.value = layout->nodes[0].joined;
  return result;
}


ac::StaticExprAttr bitsLiteralAttribute(Type payload, DictionaryAttr occurrence,
                                        MLIRContext *context, bool one);

class RuleMaterializer {
public:
  RuleMaterializer(const TransactionPreparation &prepared,
                   IRMapping &packageMap, MaterializationRoutes &routes,
                   ArrayRef<MaterializedTransaction> predecessors,
                   const PreparedTransactionLowering &lowering,
                   SourceCheckBudget &budget)
      : prepared(prepared), plan(*prepared.plan), packageMap(packageMap),
        routes(routes), predecessors(predecessors), lowering(lowering), budget(budget), frames(plan.frames.size()),
        versions(plan.versions.size()),
        branchResults(plan.branchResults.size()), stateJoins(plan.joins.size()),
        conditionalWrites(plan.writes.size()),
        controls(prepared.controls.size()), selections(plan.selections.size()),
        requirements(prepared.requirements.size()), resources(prepared.resources.size()),
        queuePaths(prepared.plan->queueAccesses.size()),
        memoryPaths(prepared.plan->memoryAccesses.size()),
        memoryReadPrefixes(prepared.plan->memoryAccesses.size()),
        selectedWrites(prepared.writeDomains.empty() ? 0 : prepared.plan->writes.size()) {
    prefixes.reserve(prepared.controls.size() + 1);
    for (auto [id, requirement] : llvm::enumerate(prepared.requirements))
      requirementSites[{requirement.frame, requirement.site}].push_back(id);
  }

  FailureOr<MaterializedTransaction> run() {
    auto transaction = plan.transaction;
    auto staged = cast<ac::SourceTransactionOp>(
        packageMap.lookup(transaction.operator->()));
    if (!charge(
            budget, transaction,
            SourceCheckBudget::add(
                SourceCheckBudget::multiply(
                    prepared.totalResults,
                    1 + SourceCheckBudget::bytes(sizeof(Value) + sizeof(Type))),
                SourceCheckBudget::add(plan.bodyChecks.size(),
                                       plan.observations.size()))))
      return failure();
    SmallVector<Value> captures;
    SmallVector<Type> types(transaction->getResultTypes());
    for (auto liveIn : prepared.liveIns) {
      auto capture = routes.bound(plan, liveIn);
      if (failed(capture))
        return failure();
      captures.push_back(*capture);
    }
    for (const auto &forward : prepared.forwarding) {
      auto rule = predecessors[forward.predecessor].rule;
      const auto &output = lowering.transactions[forward.predecessor].outputs[forward.output];
      if (!rule) return transaction.emitOpError() << "ordered predecessor is not materialized";
      captures.push_back(rule->getResult(output.dataResult));
      captures.push_back(rule->getResult(output.enableResult));
    }
    // Existing typed endpoints are detached placeholders only. Queue routing
    // replaces every external operand with its independently checked union
    // before candidate verification or publication.
    for (const auto &external : prepared.externalPops) {
      auto placeholder = routes.bound(plan, plan.queueOwners[external.owner].valid);
      if (failed(placeholder)) return failure();
      captures.push_back(*placeholder);
    }
    Type predicateType = captures[prepared.clockCapture].getType();
    for (auto output : prepared.outputs) {
      types.push_back(plan.owners[output.binding.owner].payloadType);
      types.push_back(predicateType);
    }
    types.push_back(predicateType);
    for (const auto &observation : plan.observations) {
      types.push_back(predicateType);
      for (auto value : observation.values)
        types.push_back(plan.values[value].value.getType());
    }
    for (const auto &control : prepared.controls) {
      types.push_back(predicateType);
      types.push_back(predicateType);
    }
    for (const auto &check : plan.bodyChecks) {
      types.push_back(plan.values[check.condition].value.getType());
      types.push_back(predicateType);
    }
    for (auto owner : prepared.entryOwners)
      types.push_back(plan.owners[owner].payloadType);
    for (const auto &proposal : prepared.queueProposals) {
      types.push_back(predicateType); types.push_back(predicateType);
      if (proposal.dataResult)
        types.push_back(plan.queueOwners[plan.queueAccesses[proposal.access].owner].payloadType);
    }
    for (const auto &proposal : prepared.memoryProposals) {
      types.push_back(predicateType); types.push_back(predicateType);
      for (auto argument : plan.memoryAccesses[proposal.access].arguments)
        types.push_back(plan.values[argument].value.getType());
    }
    for (const auto &domain : prepared.writeDomains)
      types.push_back(domain.maskType);
    if (types.size() != prepared.totalResults)
      return transaction.emitOpError()
             << "transaction prepared result layout is inconsistent";
    OperationState state(staged.getLoc(), ac::RuleOp::getOperationName());
    state.addOperands(captures);
    state.addTypes(types);
    NamedAttrList attributes(transaction->getAttrs());
    attributes.erase("ac.guard_location");
    attributes.erase("ac.registration_entry");
    attributes.set("ac.sampled_admission", prepared.expectedAdmission);
    attributes.set("ac.required_checks", prepared.expectedRequiredChecks);
    state.addAttributes(attributes);
    state.addRegion();
    auto rule = cast<ac::RuleOp>(Operation::create(state));
    staged->getBlock()->getOperations().insert(staged->getIterator(), rule);
    auto *block = new Block();
    rule.getBody().push_back(block);
    for (auto [ordinal, liveIn] : llvm::enumerate(prepared.liveIns)) {
      const auto &ref = plan.values[liveIn];
      auto argument =
          block->addArgument(captures[ordinal].getType(), ref.value.getLoc());
      lexical.try_emplace(LexicalKey{ref.anchor, ref.value}, argument);
    }
    for (uint64_t ordinal = prepared.liveIns.size(); ordinal < captures.size(); ++ordinal)
      block->addArgument(captures[ordinal].getType(), transaction.getLoc());
    // Isolated callable regions receive lexical values through explicit formal
    // bindings. Keep raw Module values only in the shared lexical map.
    for (const auto &read : plan.reads)
      reads.try_emplace(Key{read.frame, read.operation}, &read);
    for (const auto &write : plan.writes)
      writes.try_emplace(Key{write.frame, write.operation}, &write);
    for (const auto &call : plan.calls)
      calls.try_emplace(Key{call.caller, call.operation}, &call);
    for (auto [id, check] : llvm::enumerate(plan.bodyChecks))
      bodyCheckPlans.try_emplace(Key{check.frame, check.operation}, id);
    for (auto [id, observation] : llvm::enumerate(plan.observations))
      observationPlans.try_emplace(
          Key{observation.frame, observation.operation}, id);
    effectEvents.reserve(plan.bodyChecks.size() + plan.observations.size());
    for (const auto &event : plan.events)
      if (event.kind == ac::TransactionStateEventKind::BodyCheck ||
          event.kind == ac::TransactionStateEventKind::Observe)
        effectEvents.push_back(&event);
    savedChecks.resize(plan.bodyChecks.size());
    savedObservations.resize(plan.observations.size());
    OpBuilder builder(rule.getContext());
    builder.setInsertionPointToEnd(block);
    entryValues.resize(plan.owners.size());
    for (auto [owner, declaration] : llvm::enumerate(plan.owners)) {
      auto value = bound(declaration.oldQ);
      if (failed(value)) return failure();
      entryValues[owner] = *value;
    }
    for (const auto &forward : prepared.forwarding) {
      auto joined = materializeSelection(transaction, plan.frames[0].bindings,
          block->getArgument(forward.enableCapture), block->getArgument(forward.dataCapture),
          entryValues[forward.owner], plan.owners[forward.owner].payloadType, builder, budget, &branchTypes);
      if (failed(joined)) return failure();
      entryValues[forward.owner] = joined->value;
      forwarded.push_back(std::move(*joined));
    }
    for (auto [id, version] : llvm::enumerate(plan.versions))
      if (version.kind == ac::StateVersionKind::OldQ)
        if (failed(ensureVersion(id, predicateType, builder)))
          return failure();
    prefix = bitsLiteral(captures[prepared.clockCapture].getType(),
                         transaction.getOccurrence(), transaction.getLoc(),
                         true, builder);
    prefixes.push_back(prefix);
    if (failed(invocation(plan.rootFrame, builder)))
      return failure();
    if (nextEffectEvent != effectEvents.size())
      return transaction.emitOpError()
             << "transaction body-effect event inventory is incomplete";
    detail::MaterializedResourceDemand resourceDemand;
    if (!prepared.resources.empty()) {
      auto materialized = materializeResourceDemand(builder);
      if (failed(materialized))
        return failure();
      resourceDemand = std::move(*materialized);
    }
    SmallVector<Value> results;
    for (const auto &result : plan.results) {
      auto value = bound(result.yielded);
      if (failed(value))
        return failure();
      results.push_back(*value);
    }
    for (auto output : prepared.outputs) {
      if (!versions[output.binding.version].data ||
          !versions[output.binding.version].rawIntent)
        return transaction.emitOpError()
               << "transaction final candidate is unmapped";
      results.push_back(versions[output.binding.version].data);
      if (!charge(budget, transaction,
                  3 + 2 * 2 + 2 + 1 + sizeof(ac::BitsBinaryOp::Properties)))
        return failure();
      OperationState enable(transaction.getLoc(),
                            ac::BitsBinaryOp::getOperationName());
      enable.addOperands({prefix, versions[output.binding.version].rawIntent});
      enable.addTypes(predicateType);
      enable.addAttribute("opcode", builder.getStringAttr("and"));
      results.push_back(builder.create(enable)->getResult(0));
    }
    results.push_back(prefix);
    for (const auto &observation : savedObservations) {
      if (!observation.path)
        return transaction.emitOpError()
               << "transaction observation payload is unmapped";
      auto path = effectPath(observation.point.path, builder);
      if (failed(path))
        return failure();
      results.push_back(*path);
      llvm::append_range(results, observation.values);
    }
    for (auto [id, control] : llvm::enumerate(controls)) {
      results.push_back(control.raw);
      results.push_back(prepared.resources.empty()
                            ? control.reached
                            : resourceDemand.reached[id]);
    }
    for (auto [id, check] : llvm::enumerate(savedChecks)) {
      if (!check.first || !check.second)
        return transaction.emitOpError()
               << "transaction body-check payload is unmapped";
      auto path = effectPath(plan.bodyChecks[id].point.path, builder);
      if (failed(path))
        return failure();
      results.push_back(check.first);
      results.push_back(*path);
    }
    for (auto owner : prepared.entryOwners)
      results.push_back(entryValues[owner]);
    for (const auto &proposal : prepared.queueProposals) {
      auto path = queueProposalPath(proposal, builder);
      if (failed(path)) return failure();
      results.push_back(queuePaths[proposal.access]); results.push_back(*path);
      if (proposal.dataResult) {
        auto data = bound(*plan.queueAccesses[proposal.access].payload);
        if (failed(data)) return failure();
        results.push_back(*data);
      }
    }
    for (const auto &proposal : prepared.memoryProposals) {
      const auto &access = plan.memoryAccesses[proposal.access];
      Value reached = prefix;
      if (access.kind == ac::TransactionMemoryAccessKind::Read) {
        auto retained = retainedPrefix(access.operation, proposal.excludedControls,
                                       proposal.reachedControlEnd, builder);
        if (failed(retained)) return failure();
        reached = *retained;
      }
      auto path = binary(reached, memoryPaths[proposal.access], "and", access.operation->getLoc(), builder);
      if (failed(path)) return failure();
      results.push_back(memoryPaths[proposal.access]); results.push_back(*path);
      for (auto argument : access.arguments) {
        auto value = bound(argument);
        if (failed(value)) return failure();
        results.push_back(*value);
      }
    }
    for (const auto &domain : prepared.writeDomains) {
      if (!selectedWrites[domain.write].mask)
        return transaction.emitOpError() << "transaction write domain mask is unmapped";
      results.push_back(selectedWrites[domain.write].mask);
    }
    if (results.size() != prepared.totalResults)
      return transaction.emitOpError()
             << "transaction materialized result layout is inconsistent";
    // Effect values are captured after formal/actual and receiver bindings have
    // resolved their static type expressions.  Keep the Rule result boundary
    // identical to those authenticated statement-time SSA values rather than
    // retaining a definition-relative spelling from the source plan.
    for (auto [id, observation] : llvm::enumerate(savedObservations)) {
      uint64_t result = prepared.observationPathResults[id];
      rule->getResult(result).setType(prefix.getType());
      for (uint64_t ordinal = 0; ordinal < observation.values.size(); ++ordinal)
        rule->getResult(result + 1 + ordinal)
            .setType(observation.values[ordinal].getType());
    }
    for (auto [id, control] : llvm::enumerate(controls)) {
      uint64_t result = prepared.checkBase + 2 * id;
      rule->getResult(result).setType(control.raw.getType());
      rule->getResult(result + 1).setType(control.reached.getType());
    }
    for (auto [id, check] : llvm::enumerate(savedChecks)) {
      uint64_t result = prepared.checkBase + 2 * (prepared.controls.size() + id);
      rule->getResult(result).setType(check.first.getType());
      rule->getResult(result + 1).setType(prefix.getType());
    }
    OperationState yield(staged.getLoc(), ac::YieldOp::getOperationName());
    yield.addOperands(results);
    builder.create(yield);
    MaterializedTransaction record;
    record.rule = rule;
    record.boundValues.resize(plan.values.size());
    for (auto [id, ref] : llvm::enumerate(plan.values)) {
      if (ref.value.getDefiningOp() == transaction.operator->()) {
        auto ordinal = cast<OpResult>(ref.value).getResultNumber();
        record.boundValues[id] = rule->getResult(ordinal);
        continue;
      }
      // Pure path/reference nodes used only by eliminated effects need no final
      // wire.
      if (moduleValue(ref.value, plan.instances[ref.anchor].definition))
        record.boundValues[id] = lexical.lookup({ref.anchor, ref.value});
      else
        record.boundValues[id] = frames[ref.frame].lookupOrNull(ref.value);
    }
    record.prefixes = std::move(prefixes);
    record.controls = std::move(controls);
    record.selections = std::move(selections);
    record.versions = std::move(versions);
    record.branchResults = std::move(branchResults);
    record.stateJoins = std::move(stateJoins);
    record.conditionalWrites = std::move(conditionalWrites);
    record.forwarding = std::move(forwarded);
    record.frames = std::move(frames);
    record.requirements = std::move(requirements);
    record.resources = std::move(resources);
    record.queuePaths = std::move(queuePaths);
    record.memoryPaths = std::move(memoryPaths);
    record.memoryReadPrefixes = std::move(memoryReadPrefixes);
    record.selectedWrites = std::move(selectedWrites);
    record.resourceDemand = std::move(resourceDemand);
    return record;
  }

private:
  using Key = std::pair<ac::InvocationFrameId, Operation *>;

  FailureOr<Value> bound(ac::BoundValueId id) {
    const auto &ref = plan.values[id];
    Value result;
    if (moduleValue(ref.value, plan.instances[ref.anchor].definition))
      result = lexical.lookup({ref.anchor, ref.value});
    else
      result = frames[ref.frame].lookupOrNull(ref.value);
    if (!result)
      return plan.transaction->emitOpError()
             << "transaction invocation SSA mapping is incomplete";
    return result;
  }

  FailureOr<Value> intentLiteral(Type type, bool one, Location location,
                                 OpBuilder &builder) {
    if (!charge(budget, plan.transaction, 3 + 2 + 1 + 16))
      return failure();
    return bitsLiteral(
        type, plan.transaction->getAttrOfType<DictionaryAttr>("occurrence"),
        location, one, builder);
  }

  LogicalResult ensureVersion(ac::StateVersionId id, Type intentType,
                              OpBuilder &builder) {
    if (id >= plan.versions.size() || id >= versions.size())
      return plan.transaction->emitOpError()
             << "transaction state version is out of range";
    if (versions[id].data && versions[id].rawIntent)
      return success();
    if (versions[id].data || versions[id].rawIntent)
      return plan.transaction->emitOpError()
             << "transaction state version is partially mapped";
    const auto &version = plan.versions[id];
    if (version.kind != ac::StateVersionKind::OldQ || !version.value ||
        version.owner >= plan.owners.size() ||
        *version.value != plan.owners[version.owner].oldQ)
      return plan.transaction->emitOpError()
             << "transaction state version is unmapped";
    auto value = bound(*version.value);
    auto intent =
        intentLiteral(intentType, false, plan.transaction->getLoc(), builder);
    if (failed(value) || failed(intent))
      return failure();
    versions[id] = {entryValues[version.owner] ? entryValues[version.owner] : *value, *intent};
    return success();
  }

  FailureOr<ac::BitsSelectOp> select(Value condition, Value whenTrue,
                                     Value whenFalse, Type resultType,
                                     Location location, OpBuilder &builder) {
    if (!condition || !whenTrue || !whenFalse || !resultType ||
        !ac::areEquivalentHardwareTypes(whenTrue.getType(), resultType) ||
        !ac::areEquivalentHardwareTypes(whenFalse.getType(), resultType))
      return plan.transaction->emitOpError()
             << "transaction branch selection inputs are unmapped";
    if (!charge(budget, plan.transaction, 3 + 2 * 3 + 2))
      return failure();
    OperationState state(location, ac::BitsSelectOp::getOperationName());
    state.addOperands({condition, whenTrue, whenFalse});
    state.addTypes(resultType);
    return cast<ac::BitsSelectOp>(builder.create(state));
  }

  FailureOr<MaterializedBranchResult> localJoin(ac::SourceBranchOp branch,
                                                ac::InvocationFrameId frame,
                                                Value condition, Value whenTrue,
                                                Value whenFalse, Type type,
                                                OpBuilder &builder) {
    return materializeSelection(branch, plan.frames[frame].bindings, condition,
                                whenTrue, whenFalse, type, builder, budget, &branchTypes);
  }

  FailureOr<Value> binary(Value lhs, Value rhs, StringRef opcode,
                          Location location, OpBuilder &builder) {
    if (!charge(budget, plan.transaction,
                3 + 4 + 2 + 1 + sizeof(ac::BitsBinaryOp::Properties)))
      return failure();
    OperationState state(location, ac::BitsBinaryOp::getOperationName());
    state.addOperands({lhs, rhs});
    state.addTypes(lhs.getType());
    state.addAttribute("opcode", builder.getStringAttr(opcode));
    return builder.create(state)->getResult(0);
  }
  FailureOr<Value> known(Value raw, bool one, Location location,
                         OpBuilder &builder) {
    auto literal = intentLiteral(prefix.getType(), one, location, builder);
    if (failed(literal) ||
        !charge(budget, plan.transaction,
                3 + 4 + 2 + 1 + sizeof(ac::BitsCompareOp::Properties)))
      return failure();
    OperationState state(location, ac::BitsCompareOp::getOperationName());
    state.addOperands({raw, *literal});
    state.addTypes(prefix.getType());
    state.addAttribute("predicate", builder.getStringAttr("case_eq"));
    return builder.create(state)->getResult(0);
  }
  FailureOr<Value> selectedPath(ac::TransactionSourcePath path) {
    auto projected = sampledPath(plan, path, budget);
    if (failed(projected))
      return failure();
    if (projected->empty())
      return Value();
    auto control = projected->getAs<IntegerAttr>("selection").getInt();
    auto *selection =
        std::get_if<ac::BranchSelectControl>(&plan.controls[control].boundary);
    if (!selection || selection->selection >= selections.size())
      return failure();
    const auto &paths = selections[selection->selection];
    Value result = projected->getAs<BoolAttr>("when_true").getValue()
                       ? paths.whenTrue
                       : paths.whenFalse;
    if (!result)
      return failure();
    return result;
  }
#include "TransactionRawStatementPath.inc"
#include "TransactionQueueMaterialization.inc"
#include "TransactionMemoryMaterialization.inc"
#include "TransactionResourceDemandMaterialization.inc"

  LogicalResult admitControl(ac::AdmissionControlId id, Value raw,
                             Location location, OpBuilder &builder, bool generated = false) {
    uint64_t source = id;
    if (!generated) id = prepared.sourceControls[id];
    if (id != prefixes.size() - 1 || id >= controls.size())
      return plan.transaction->emitOpError() << "control order differs";
    auto sourcePath = !generated ? plan.controls[source].path
        : prepared.controls[id].requirement
            ? prepared.requirements[*prepared.controls[id].requirement].point.path
            : resourcePoint(plan, prepared.resources[*prepared.controls[id].resource]).path;
    auto path = selectedPath(sourcePath);
    if (failed(path))
      return failure();
    auto k1 = known(raw, true, location, builder);
    if (failed(k1))
      return failure();
    Value accepted = *k1;
    auto *selection = generated ? nullptr :
        std::get_if<ac::BranchSelectControl>(&plan.controls[source].boundary);
    Value k0;
    if (selection) {
      auto zero = known(raw, false, location, builder);
      if (failed(zero))
        return failure();
      k0 = *zero;
      auto either = binary(k0, *k1, "or", location, builder);
      if (failed(either))
        return failure();
      accepted = *either;
    }
    Value reached = prefix, bypass = accepted;
    if (*path) {
      auto reach = binary(prefix, *path, "and", location, builder);
      auto one = intentLiteral(prefix.getType(), true, location, builder);
      if (failed(reach) || failed(one))
        return failure();
      auto selected =
          select(*path, accepted, *one, prefix.getType(), location, builder);
      if (failed(selected))
        return failure();
      reached = *reach;
      bypass = selected->getResult();
    }
    auto next = binary(prefix, bypass, "and", location, builder);
    if (failed(next))
      return failure();
    controls[id] = {raw, *path, reached, accepted, bypass, {}};
    prefix = *next;
    prefixes.push_back(prefix);
    if (selection) {
      Value truePath = *k1, falsePath = k0;
      if (*path) {
        auto t = binary(*path, *k1, "and", location, builder);
        auto f = binary(*path, k0, "and", location, builder);
        if (failed(t) || failed(f))
          return failure();
        truePath = *t;
        falsePath = *f;
      }
      selections[selection->selection] = {truePath, falsePath};
    }
    return success();
  }
  FailureOr<Value> effectPath(ac::TransactionSourcePath path,
                              OpBuilder &builder) {
    auto selected = selectedPath(path);
    if (failed(selected))
      return failure();
    if (!*selected)
      return prefix;
    return binary(prefix, *selected, "and", plan.transaction->getLoc(),
                  builder);
  }

  LogicalResult emitRequirements(Operation *site, ac::InvocationFrameId frame,
                                 OpBuilder &builder) {
    auto found = requirementSites.find({frame, site});
    if (found == requirementSites.end()) return success();
    for (uint64_t id : found->second) {
      const auto &requirement = prepared.requirements[id];
      if (requirements[id])
        return site->emitOpError() << "required use materialized twice";
      auto payload = bound(requirement.payload);
      auto one = intentLiteral(prefix.getType(), true, site->getLoc(), builder);
      if (failed(payload) || failed(one)) return failure();
      Value path = *one;
      auto ancestry = requirement.point.path;
      for (unsigned depth = 0; ancestry.branch; ++depth) {
        if (depth > 128 || !charge(budget, site)) return failure();
        const auto &branch = plan.branches[*ancestry.branch];
        auto condition = bound(branch.condition);
        if (failed(condition)) return failure();
        Value selected = *condition;
        if (!ancestry.whenTrue) {
          OperationState invert(site->getLoc(), ac::BitsUnaryOp::getOperationName());
          invert.addOperands(selected);
          invert.addTypes(selected.getType());
          invert.addAttribute("opcode", builder.getStringAttr("not"));
          selected = builder.create(invert)->getResult(0);
        }
        auto joined = binary(path, selected, "and", site->getLoc(), builder);
        if (failed(joined)) return failure();
        path = *joined;
        ancestry = branch.entry.path;
      }
      if (requirement.rawPath) {
        auto raw = bound(*requirement.rawPath);
        if (failed(raw)) return failure();
        auto joined = binary(path, *raw, "and", site->getLoc(), builder);
        if (failed(joined)) return failure();
        path = *joined;
      }
      OperationState state(site->getLoc(), ac::ValueRequireOp::getOperationName());
      // Availability is filled from actual producer SSA after every Rule and
      // physical Module route exists. No speculative zero/one wire is seeded.
      state.addOperands({*payload, path});
      state.addTypes(prefix.getType());
      state.addAttribute("occurrence", requirement.occurrence);
      state.addAttribute("source_location", requirement.location);
      auto operation = cast<ac::ValueRequireOp>(builder.create(state));
      requirements[id] = operation;
      if (failed(admitControl(requirement.control, operation.getResult(),
                              site->getLoc(), builder, true))) return failure();
    }
    return success();
  }

#include "TransactionSelectedWriteMaterialization.inc"

  LogicalResult materializeWrite(const ac::StateWriteIntent &write,
                                 OpBuilder &builder) {
        auto rhs = bound(write.value);
        if (failed(rhs))
          return failure();
        uint64_t writeId = *plan.versions[write.version].write;
        auto domain = llvm::find_if(prepared.writeDomains, [&](const auto &entry) {
          return entry.write == writeId;
        });
        if (domain != prepared.writeDomains.end()) {
          if (failed(ensureVersion(write.predecessor, prefix.getType(), builder)))
            return failure();
          auto payload = write.selectedPayload ? bound(*write.selectedPayload) : rhs;
          if (failed(payload)) return failure();
          auto selected = selectedWrite(write, *domain, versions[write.predecessor].data, *payload, builder);
          if (failed(selected)) return failure();
          selectedWrites[writeId] = std::move(*selected);
          if (isa<ac::SourceStateWriteOp>(write.operation) && !write.selectors.empty())
            rhs = selectedWrites[writeId].data;
        }
        if (write.pathFact && write.pathFact.getValue()) {
          auto rawIntent =
              intentLiteral(prefix.getType(), true, write.operation->getLoc(), builder);
          if (failed(rawIntent)) return failure();
          versions[write.version] = {*rhs, *rawIntent};
        } else {
          auto path = bound(write.authoredPath);
          if (failed(path) ||
              failed(ensureVersion(write.predecessor, prefix.getType(), builder)))
            return failure();
          auto selected = materializeSelection(write.operation, plan.frames[write.frame].bindings,
              *path, *rhs, versions[write.predecessor].data,
              plan.owners[write.owner].payloadType, builder, budget, &branchTypes);
          auto rawIntent = binary(versions[write.predecessor].rawIntent, *path,
                                  "or", write.operation->getLoc(), builder);
          if (failed(selected) || failed(rawIntent)) return failure();
          versions[write.version] = {selected->value, *rawIntent};
          conditionalWrites[*plan.versions[write.version].write] =
              std::move(*selected);
        }
    return success();
  }

  LogicalResult statements(Block &source, ac::InvocationFrameId frame,
                           OpBuilder &builder, unsigned depth = 0) {
    if (depth > 128)
      return plan.transaction->emitOpError()
             << "transaction branch depth budget exceeded";
    if (!charge(budget, plan.transaction,
                1 + SourceCheckBudget::bytes(sizeof(Block *) + sizeof(frame) +
                                             sizeof(depth))))
      return failure();
    for (Operation &op : source.without_terminator()) {
      if (!isa<ac::SourceStateWriteOp, ac::SourceBranchOp, ac::SourceMethodCallOp>(op) &&
          failed(emitRequirements(&op, frame, builder))) return failure();
      if (isa<ac::SourceQueuePopOp, ac::SourceQueuePeekOp, ac::SourceQueuePushOp>(op)) {
        return op.emitOpError()
               << "retired dedicated queue effects are not admitted";
      } else if (auto access = dyn_cast<ac::SourceStateReadOp>(op)) {
        auto found = reads.find({frame, access});
        if (found == reads.end() || !versions[found->second->version].data)
          return access.emitOpError() << "transaction read version is unmapped";
        if (!prepayValueMapping(budget, access, frames[frame],
                                access.getValue()))
          return failure();
        frames[frame].map(access.getValue(),
                          versions[found->second->version].data);
      } else if (auto intent = dyn_cast<ac::SourceStateWriteOp>(op)) {
        if (failed(emitRequirements(intent, frame, builder))) return failure();
        auto found = writes.find({frame, intent});
        if (found == writes.end())
          return intent.emitOpError() << "transaction write plan is missing";
        if (failed(materializeWrite(*found->second, builder))) return failure();
      } else if (auto formal = dyn_cast<ac::SourceFormalWriteOp>(op)) {
        auto found = llvm::find_if(plan.formalWrites, [&](const auto &write) {
          return write.frame == frame && write.operation == formal;
        });
        if (found == plan.formalWrites.end())
          return formal.emitOpError() << "transaction formal write plan is missing";
        if (found->write && failed(materializeWrite(plan.writes[*found->write], builder)))
          return failure();
        if (found->snapshot) {
          auto selected = bound(found->owner ? found->candidate : *found->previous);
          if (failed(selected) || !prepayValueMapping(budget, formal, frames[frame],
                                                     formal.getSnapshot())) return failure();
          frames[frame].map(formal.getSnapshot(), *selected);
        }
      } else if (auto split = dyn_cast<ac::SourceBranchOp>(op)) {
        if (failed(emitRequirements(split, frame, builder))) return failure();
        if (!charge(budget, split, plan.branches.size()))
          return failure();
        auto found = llvm::find_if(plan.branches, [&](const auto &branch) {
          return branch.frame == frame && branch.operation == split;
        });
        if (found == plan.branches.end())
          return split.emitOpError() << "transaction branch plan is missing";
        auto condition = bound(found->condition);
        if (failed(condition))
          return failure();
        if (found->selection &&
            failed(admitControl(plan.selections[*found->selection].control,
                                *condition, split.getLoc(), builder)))
          return failure();
        if (failed(statements(split.getThenRegion().front(), frame, builder,
                              depth + 1)) ||
            failed(statements(split.getElseRegion().front(), frame, builder,
                              depth + 1)))
          return failure();
        for (auto id : found->joins) {
          const auto &join = plan.joins[id];
          if (failed(ensureVersion(join.whenTrue, prefix.getType(), builder)) ||
              failed(ensureVersion(join.whenFalse, prefix.getType(), builder)))
            return failure();
          auto data =
              localJoin(split, frame, *condition, versions[join.whenTrue].data,
                        versions[join.whenFalse].data,
                        plan.owners[join.owner].payloadType, builder);
          auto intent = select(*condition, versions[join.whenTrue].rawIntent,
                               versions[join.whenFalse].rawIntent,
                               prefix.getType(), op.getLoc(), builder);
          if (failed(data) || failed(intent))
            return failure();
          versions[join.result] = {data->value, intent->getResult()};
          stateJoins[id] = std::move(*data);
        }
        for (auto id : found->results) {
          const auto &result = plan.branchResults[id];
          auto whenTrue = bound(result.whenTrue);
          auto whenFalse = bound(result.whenFalse);
          if (failed(whenTrue) || failed(whenFalse))
            return failure();
          auto joined = localJoin(split, frame, *condition, *whenTrue,
                                  *whenFalse, result.payloadType, builder);
          if (failed(joined))
            return failure();
          Value original = plan.values[result.result].value;
          if (!prepayValueMapping(budget, split, frames[frame], original))
            return failure();
          frames[frame].map(original, joined->value);
          branchResults[id] = std::move(*joined);
        }
      } else if (auto call = dyn_cast<ac::SourceMethodCallOp>(op)) {
        if (failed(emitRequirements(call, frame, builder))) return failure();
        auto memory = llvm::find_if(plan.memoryAccesses, [&](const auto &access) {
          return access.operation == call && access.frame == frame;
        });
        if (memory != plan.memoryAccesses.end()) {
          if (failed(emitMemoryAccess(call, frame, builder))) return failure();
          continue;
        }
        auto queue = llvm::find_if(plan.queueAccesses, [&](const auto &access) {
          return access.operation == call && access.frame == frame;
        });
        if (queue != plan.queueAccesses.end()) {
          if (failed(emitQueueAccess(call, frame, builder))) return failure();
          continue;
        }
        auto found = calls.find({frame, call});
        if (found == calls.end())
          return call.emitOpError() << "transaction call plan is missing";
        const auto &callee = *found->second;
        if (failed(invocation(callee.callee, builder)))
          return failure();
        for (const auto &result : callee.results) {
          auto value = bound(result.yielded);
          if (failed(value))
            return failure();
          if (!prepayValueMapping(budget, call, frames[frame],
                                  plan.values[result.result].value))
            return failure();
          frames[frame].map(plan.values[result.result].value, *value);
        }
      } else if (auto check = dyn_cast<ac::SourceExpectOp>(op)) {
        auto found = bodyCheckPlans.find({frame, check});
        if (found == bodyCheckPlans.end() ||
            nextEffectEvent == effectEvents.size() ||
            effectEvents[nextEffectEvent]->kind !=
                ac::TransactionStateEventKind::BodyCheck ||
            effectEvents[nextEffectEvent]->frame != frame ||
            effectEvents[nextEffectEvent]->bodyCheck != found->second)
          return op.emitOpError()
                 << "transaction body-check event order is inconsistent";
        const auto &record = plan.bodyChecks[found->second];
        auto condition = bound(record.condition);
        auto path = bound(record.authoredPath);
        if (failed(condition) || failed(path))
          return failure();
        savedChecks[found->second] = {*condition, *path};
        ++nextEffectEvent;
      } else if (auto observation = dyn_cast<ac::SourceObserveOp>(op)) {
        auto found = observationPlans.find({frame, observation});
        if (found == observationPlans.end() ||
            nextEffectEvent == effectEvents.size() ||
            effectEvents[nextEffectEvent]->kind !=
                ac::TransactionStateEventKind::Observe ||
            effectEvents[nextEffectEvent]->frame != frame ||
            effectEvents[nextEffectEvent]->observation != found->second)
          return op.emitOpError()
                 << "transaction observation event order is inconsistent";
        const auto &record = plan.observations[found->second];
        auto path = bound(record.authoredPath);
        if (failed(path))
          return failure();
        savedObservations[found->second].path = *path;
        savedObservations[found->second].point = record.point;
        savedObservations[found->second].values.reserve(record.values.size());
        for (auto value : record.values) {
          auto mapped = bound(value);
          if (failed(mapped))
            return failure();
          savedObservations[found->second].values.push_back(*mapped);
        }
        ++nextEffectEvent;
      } else {
        if (failed(verifyClosedPurePayload(
                &op, budget,
                "transaction invocation requires closed pure payload "
                "operations")))
          return failure();
        for (Value operand : op.getOperands())
          if (!frames[frame].contains(operand))
            return op.emitOpError() << "transaction pure operand is unmapped";
        builder.clone(op, frames[frame]);
      }
    }
    return success();
  }

  LogicalResult invocation(ac::InvocationFrameId frame, OpBuilder &builder) {
    const auto &binding = plan.frames[frame];
    for (ArrayRef<ac::TransactionValueBinding> arguments :
         {ArrayRef<ac::TransactionValueBinding>(binding.guardArguments),
          ArrayRef<ac::TransactionValueBinding>(binding.bodyArguments)})
      for (const auto &argument : arguments) {
        auto actual = bound(argument.actual);
        if (failed(actual))
          return failure();
        if (!prepayValueMapping(budget, plan.transaction, frames[frame],
                                argument.formal))
          return failure();
        frames[frame].map(argument.formal, *actual);
      }
    auto method = binding.method;
    auto transaction = plan.transaction;
    Operation *function =
        method ? method.operator->() : transaction.operator->();
    for (auto [regionOrdinal, region] :
         llvm::enumerate(function->getRegions())) {
      if (failed(statements(region.front(), frame, builder)))
        return failure();
      if (failed(emitRequirements(region.front().getTerminator(), frame, builder)))
        return failure();
      if (regionOrdinal == 0) {
        auto found = llvm::find_if(plan.guards, [&](const auto &guard) {
          return guard.frame == frame;
        });
        if (found == plan.guards.end())
          return function->emitOpError() << "transaction guard plan is missing";
        auto raw = bound(found->predicate);
        if (failed(raw))
          return failure();
        if (failed(admitControl(found->control, *raw, function->getLoc(),
                                builder)))
          return failure();
      }
    }
    return success();
  }

  SmallVector<ac::ValueRequireOp, 0> requirements;
  SmallVector<ac::BitsSelectOp, 0> resources;
  SmallVector<Value, 0> queuePaths, memoryPaths, memoryReadPrefixes;
  llvm::DenseMap<Key, SmallVector<uint64_t, 2>> requirementSites;
  const TransactionPreparation &prepared;
  const ac::TransactionStatePlan &plan;
  IRMapping &packageMap;
  using LexicalKey = std::pair<ac::LexicalInstanceId, Value>;
  MaterializationRoutes &routes;
  ArrayRef<MaterializedTransaction> predecessors;
  const PreparedTransactionLowering &lowering;
  SourceCheckBudget &budget;
  BranchTypeAdmissionCache branchTypes;
  llvm::DenseMap<LexicalKey, Value> lexical;
  SmallVector<IRMapping, 0> frames;
  SmallVector<MaterializedVersion, 0> versions;
  SmallVector<MaterializedBranchResult, 0> branchResults, stateJoins, forwarded,
      conditionalWrites;
  SmallVector<Value, 0> entryValues;
  Value prefix;
  SmallVector<Value, 0> prefixes;
  SmallVector<detail::MaterializedAdmissionControl, 0> controls;
  SmallVector<detail::MaterializedSelection, 0> selections;
  llvm::DenseMap<Key, const ac::StateReadBinding *> reads;
  llvm::DenseMap<Key, const ac::StateWriteIntent *> writes;
  llvm::DenseMap<Key, const ac::TransactionCallBinding *> calls;
  llvm::DenseMap<Key, ac::BodyCheckId> bodyCheckPlans;
  llvm::DenseMap<Key, ac::ObservationId> observationPlans;
  SmallVector<detail::MaterializedSelectedWrite, 0> selectedWrites;
  SmallVector<const ac::TransactionStateEvent *, 0> effectEvents;
  size_t nextEffectEvent = 0;
  SmallVector<std::pair<Value, Value>, 0> savedChecks;
  SmallVector<SavedMaterializedObservation, 0> savedObservations;
};

ac::StaticExprAttr bitsLiteralAttribute(Type payload, DictionaryAttr occurrence,
                                        MLIRContext *context, bool one) {
  Builder builder(context);
  auto type = cast<ac::BitsType>(payload);
  auto literal = ac::MathIntAttr::get(
      context, llvm::APSInt(llvm::APInt(1, one ? 1 : 0), true));
  auto tree = builder.getDictionaryAttr(
      {builder.getNamedAttr("kind", builder.getStringAttr("literal")),
       builder.getNamedAttr(
           "value",
           builder.getDictionaryAttr(
               {builder.getNamedAttr("kind", builder.getStringAttr("integer")),
                builder.getNamedAttr("value", literal)})),
       builder.getNamedAttr("origin", occurrence),
       builder.getNamedAttr("location",
                            type.getWidth().getTree().get("location"))});
  return ac::StaticExprAttr::get(context, tree);
}

Value bitsLiteral(Type payload, DictionaryAttr occurrence, Location location,
                  bool one, OpBuilder &builder) {
  OperationState constant(location, ac::BitsConstantOp::getOperationName());
  constant.addTypes(payload);
  constant.addAttribute(
      "value",
      bitsLiteralAttribute(payload, occurrence, builder.getContext(), one));
  return builder.create(constant)->getResult(0);
}

#include "TransactionDomainCommitMaterialization.inc"
#include "TransactionQueuePins.inc"
#include "TransactionMemoryPins.inc"

/// Remove every clone-map entry whose target lies in one of the retired
/// subtrees. The map is package-wide and immutable during cleanup, so one
/// attributed scan replaces the previous scan-per-retired-root. Lookups still
/// pay their own comparisons through IRMapping::erase.
LogicalResult eraseTargetMappings(IRMapping &mapping,
                                  ArrayRef<Operation *> removed,
                                  SourceCheckBudget &budget) {
  llvm::DenseSet<Operation *> roots(removed.begin(), removed.end());
  uint64_t entries = SourceCheckBudget::add(
      mapping.getOperationMap().size(),
      SourceCheckBudget::add(mapping.getBlockMap().size(),
                             mapping.getValueMap().size()));
  if (!charge(budget, removed.front(), SourceCheckBudget::multiply(entries, 4)))
    return failure();
  auto belongs = [&](Operation *op) {
    for (Operation *at = op; at; at = at->getParentOp())
      if (roots.contains(at))
        return true;
    return false;
  };
  SmallVector<Operation *> opKeys;
  SmallVector<Block *> blockKeys;
  SmallVector<Value> valueKeys;
  for (const auto &entry : mapping.getOperationMap())
    if (belongs(entry.second))
      opKeys.push_back(entry.first);
  for (const auto &entry : mapping.getBlockMap())
    if (belongs(entry.second->getParentOp()))
      blockKeys.push_back(entry.first);
  for (const auto &entry : mapping.getValueMap()) {
    Value target = entry.second;
    if (belongs(target.getParentRegion()->getParentOp()))
      valueKeys.push_back(entry.first);
  }
  for (auto key : opKeys)
    mapping.erase(key);
  for (auto key : blockKeys)
    mapping.erase(key);
  for (auto key : valueKeys)
    mapping.erase(key);
  return success();
}

/// Prove an admitted Bits1 SSA value is exactly constant one. Only closed
/// constants and bitwise/compare/select combinations are folded; any unknown
/// operand or unsupported operation is false, never a guess. This decides the
/// plain DFF leaf from the actually materialized commit enable.
bool knownTrue(Value value, const ac::HardwareAnalysis &hardware,
               SourceCheckBudget &budget, unsigned depth = 0) {
  Operation *site = value.getDefiningOp();
  if (!site || depth > 64 || !charge(budget, site)) return false;
  Type type = value.getType();
  if (!isa<ac::BitsType>(type)) return false;
  auto width = hardware.getPackedWidth(type, {}, site);
  if (failed(width) || *width != 1) return false;
  if (auto rule = value.getDefiningOp<ac::RuleOp>()) {
    auto *terminator = rule.getBody().front().getTerminator();
    auto yielded = dyn_cast_or_null<ac::YieldOp>(terminator);
    auto result = dyn_cast<OpResult>(value);
    if (!yielded || !result ||
        result.getResultNumber() >= yielded.getNumOperands())
      return false;
    return knownTrue(yielded.getOperand(result.getResultNumber()), hardware,
                     budget, depth + 1);
  }
  if (auto constant = value.getDefiningOp<ac::BitsConstantOp>()) {
    auto evaluated =
        hardware.evaluateStatic(constant.getValue(), {}, constant);
    if (failed(evaluated)) return false;
    auto integer = dyn_cast<ac::MathIntAttr>(*evaluated);
    if (!integer) return false;
    llvm::APSInt number(integer.getCanonicalValue());
    return number.getBitWidth() == 1 && number.getBoolValue();
  }
  if (auto binary = value.getDefiningOp<ac::BitsBinaryOp>()) {
    StringRef opcode = binary.getOpcode();
    if (opcode == "and")
      return knownTrue(binary.getLhs(), hardware, budget, depth + 1) &&
             knownTrue(binary.getRhs(), hardware, budget, depth + 1);
    if (opcode == "or") {
      bool lhs = knownTrue(binary.getLhs(), hardware, budget, depth + 1);
      if (lhs) return true;
      return knownTrue(binary.getRhs(), hardware, budget, depth + 1);
    }
    return false;
  }
  if (auto compare = value.getDefiningOp<ac::BitsCompareOp>()) {
    auto constant = [&](Value operand) -> std::optional<llvm::APSInt> {
      auto op = operand.getDefiningOp<ac::BitsConstantOp>();
      if (!op) return std::nullopt;
      auto evaluated = hardware.evaluateStatic(op.getValue(), {}, op);
      if (failed(evaluated)) return std::nullopt;
      auto integer = dyn_cast<ac::MathIntAttr>(*evaluated);
      if (!integer) return std::nullopt;
      return llvm::APSInt(integer.getCanonicalValue());
    };
    auto lhs = constant(compare.getLhs());
    auto rhs = constant(compare.getRhs());
    if (!lhs || !rhs) return false;
    StringRef predicate = compare.getPredicate();
    if (predicate == "eq" || predicate == "case_eq") return *lhs == *rhs;
    if (predicate == "ne") return *lhs != *rhs;
    if (predicate == "ult") return lhs->ult(*rhs);
    if (predicate == "ule") return lhs->ule(*rhs);
    if (predicate == "ugt") return lhs->ugt(*rhs);
    if (predicate == "uge") return lhs->uge(*rhs);
    if (predicate == "slt") return lhs->slt(*rhs);
    if (predicate == "sle") return lhs->sle(*rhs);
    if (predicate == "sgt") return lhs->sgt(*rhs);
    if (predicate == "sge") return lhs->sge(*rhs);
    return false;
  }
  if (auto select = value.getDefiningOp<ac::BitsSelectOp>()) {
    if (!knownTrue(select.getCondition(), hardware, budget, depth + 1))
      return false;
    return knownTrue(select.getTrueValue(), hardware, budget, depth + 1);
  }
  return false;
}

LogicalResult materialize(PreparedTransactionLowering &prepared,
                          mlir::ModuleOp staged, IRMapping &mapping,
                          SmallVectorImpl<MaterializedTransaction> &records,
                          SmallVectorImpl<detail::MaterializedIncomingCommit> &incomingCommits,
                          SourceCheckBudget &budget) {
  ac::HardwareAnalysis hardware(prepared.original);
  Builder attributes(staged.getContext());
  // The schema is final-only, including on this detached candidate. No source
  // envelope or input object is changed by admission or late verification.
  staged->setAttr("ac.stage", attributes.getStringAttr("final"));
  using CandidateMap = llvm::DenseMap<RoutingKey, std::pair<Value, Value>>;
  llvm::DenseMap<Operation *, CandidateMap> nextValues;
  llvm::DenseMap<Operation *, SmallVector<std::pair<Value, Value>>> incoming;
  MaterializationRoutes routes(prepared, mapping);
  SmallVector<StateStorage> storage;

  // Definitions receive the union of demands from every caller occurrence.
  // Source types/names/domain ordinals are preserved as an exact prefix.
  for (auto &definition : prepared.definitions) {
    if (definition.upward.empty() && definition.downward.empty() &&
        definition.queuePopSummaries.empty())
      continue;
    auto module =
        cast<ac::ModuleOp>(mapping.lookup(definition.module.operator->()));
    auto old = module.getFunctionType();
    SmallVector<Type> inputs(old.getInputs()), outputs(old.getResults());
    SmallVector<Attribute> inputNames(module.getInputNames().begin(),
                                      module.getInputNames().end());
    SmallVector<Attribute> outputNames(module.getOutputNames().begin(),
                                       module.getOutputNames().end());
    llvm::StringSet<> names;
    for (auto name : inputNames)
      names.insert(cast<StringAttr>(name).getValue());
    for (auto name : outputNames)
      names.insert(cast<StringAttr>(name).getValue());
    auto portName = [&](StringRef direction, unsigned ordinal) {
      std::string stem =
          (Twine("__ac_transaction_") + direction + "_" + Twine(ordinal)).str();
      std::string name = stem;
      unsigned collision = 0;
      while (!names.insert(name).second)
        name = (Twine(stem) + "_" + Twine(++collision)).str();
      return attributes.getStringAttr(name);
    };
    auto &arguments = incoming[definition.module];
    for (auto [ordinal, route] : llvm::enumerate(definition.downward)) {
      inputs.push_back(route.type);
      inputs.push_back(route.enableType);
      inputNames.push_back(portName("d", ordinal));
      inputNames.push_back(portName("e", ordinal));
      auto &body = module.getBody().front();
      Value d = body.addArgument(route.type, module.getLoc());
      Value e = body.addArgument(route.enableType, module.getLoc());
      arguments.emplace_back(d, e);
    }
    for (auto [ordinal, route] : llvm::enumerate(definition.upward)) {
      outputs.push_back(route.type);
      outputNames.push_back(portName("q_or_capture", ordinal));
    }
    for (auto [ordinal, route] : llvm::enumerate(definition.queuePopSummaries)) {
      outputs.push_back(route.predicateType);
      outputNames.push_back(portName("pop_summary", ordinal));
    }
    module->setAttr(
        "function_type",
        TypeAttr::get(FunctionType::get(staged.getContext(), inputs, outputs)));
    module->setAttr("input_names", attributes.getArrayAttr(inputNames));
    module->setAttr("output_names", attributes.getArrayAttr(outputNames));
    SmallVector<Attribute> defaults(inputs.size() - definition.sourceInputs,
                                    attributes.getI64IntegerAttr(0));
    module->setAttr(
        "ac.internal_ports",
        attributes.getDictionaryAttr(
            {attributes.getNamedAttr(
                 "source_inputs",
                 attributes.getI64IntegerAttr(definition.sourceInputs)),
             attributes.getNamedAttr(
                 "source_outputs",
                 attributes.getI64IntegerAttr(definition.sourceOutputs)),
             attributes.getNamedAttr("root_inputs",
                                     attributes.getArrayAttr(defaults))}));
    if (failed(ac::getModulePortLayout(module)))
      return failure();
  }

  // Rebuild each affected original Instance once, preserving its identity and
  // complete source arity. Even unrelated occurrences bind every suffix port.
  // Candidate suffix operands are bound after every physical allocation exists.
  for (auto &parent : prepared.definitions) {
    for (auto original :
         parent.module->getRegion(0).front().getOps<ac::InstanceOp>()) {
      auto child =
          dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(original));
      if (!child)
        continue;
      auto &definition =
          prepared
              .definitions[prepared.definitionIndex.lookup(child.operator->())];
      if (definition.upward.empty() && definition.downward.empty() &&
        definition.queuePopSummaries.empty())
        continue;
      auto old = cast<ac::InstanceOp>(mapping.lookup(original.operator->()));
      auto bindings = hardware.bindInstance(original);
      if (failed(bindings))
        return failure();
      SmallVector<Type> resultTypes(old.getResultTypes());
      for (const auto &route : definition.upward) {
        auto type = hardware.resolveType(route.type, *bindings, original);
        if (failed(type))
          return failure();
        resultTypes.push_back(*type);
      }
      for (const auto &route : definition.queuePopSummaries) {
        auto type = hardware.resolveType(route.predicateType, *bindings, original);
        if (failed(type)) return failure();
        resultTypes.push_back(*type);
      }
      SmallVector<Value> operands(old.getOperands());
      OpBuilder builder(old);
      // Detached reconstruction binds complete candidate pairs after every
      // allocation exists. Avoid inventing an untyped zero for nominal data.
      OperationState replacement(old.getLoc(),
                                 ac::InstanceOp::getOperationName());
      replacement.addAttributes(old->getAttrs());
      replacement.addTypes(resultTypes);
      replacement.addOperands(operands);
      auto allocation = cast<ac::InstanceOp>(builder.create(replacement));
      for (auto [source, prior, result] :
           llvm::zip(original.getResults(), old.getResults(),
                     allocation.getResults().take_front(old.getNumResults()))) {
        prior.replaceAllUsesWith(result);
        mapping.map(source, result);
      }
      mapping.map(original.operator->(), allocation.operator->());
      old.erase();
    }
  }

  // State remains at its lexical declaration: scalar DFFE or element Collection.
  for (auto original : prepared.states) {
    auto state = cast<ac::SourceStateOp>(mapping.lookup(original.operator->()));
    auto table = dyn_cast<ac::TableType>(original.getQ().getType());
    bool dff = false;
    Type element = table ? table.getElementType() : original.getQ().getType();
    OperationState leaf(state.getLoc(), table ? ac::CollectionOp::getOperationName()
                                              : ac::InstanceOp::getOperationName());
    if (table)
      leaf.addAttribute("shape", table.getShape());
    leaf.addTypes(original.getQ().getType());
    leaf.addAttribute("instance_name", original.getInstanceNameAttr());
    leaf.addAttribute("occurrence", original.getOccurrence());
    auto primitive = dff ? prepared.dff : prepared.dffe;
    if (!primitive)
      return state.emitOpError()
             << "state storage requires its verified "
             << (dff ? "DFF" : "DFFE") << " primitive import";
    leaf.addAttribute(
        "callee",
        FlatSymbolRefAttr::get(
            staged.getContext(),
            primitive->getAttrOfType<StringAttr>("sym_name").getValue()));
    leaf.addAttribute("parameters", attributes.getArrayAttr({}));
    leaf.addAttribute(
        "type_arguments",
        attributes.getArrayAttr({TypeAttr::get(element)}));
    auto *instance = Operation::create(leaf);
    state->getBlock()->getOperations().insert(state->getIterator(), instance);
    state.getQ().replaceAllUsesWith(instance->getResult(0));
    mapping.map(original.getQ(), instance->getResult(0));
    storage.push_back({original, instance, dff});
    mapping.map(original.operator->(), instance);
    state.erase();
  }

  // Close every incoming definition-relative prefix against the actual Q0.
  // Disabled occurrences may bind arbitrary typed D: only E gives write intent.
  for (const auto &definition : prepared.definitions) {
    auto module = cast<ac::ModuleOp>(mapping.lookup(definition.module.operator->()));
    OpBuilder builder(module.getBody().front().getTerminator());
    ac::HardwareBindings bindings;
    bindings.owner = definition.module;
    for (auto [ordinal, route] : llvm::enumerate(definition.downward)) {
      auto oldQ = routes.value(definition.module, route.path, route.value);
      if (failed(oldQ) || !prepayAppend(budget, definition.module, incomingCommits)) return failure();
      auto [data, enable] = incoming[definition.module][ordinal];
      auto state = route.value.getDefiningOp<ac::SourceStateOp>();
      auto closed = materializeDomainCommit(state, state.getOccurrence(), route.type,
          bindings, *oldQ, data, enable, {}, hardware, builder, budget);
      if (failed(closed)) return failure();
      auto key = routingKey(route.path, route.value);
      nextValues[definition.module][key] = {closed->value, enable};
      incomingCommits.push_back({definition.module, key, std::move(*closed)});
    }
  }

  struct Replacement {
    ac::SourceTransactionOp original;
    ac::RuleOp rule;
  };
  SmallVector<Replacement> replacements;
  records.resize(prepared.transactions.size());
  for (uint64_t transactionIndex : prepared.transactionSchedule) {
    const auto &transaction = prepared.transactions[transactionIndex];
    RuleMaterializer builder(transaction, mapping, routes, records, prepared, budget);
    auto rule = builder.run();
    if (failed(rule))
      return failure();
    auto original = transaction.plan->transaction;
    auto materializedRule = rule->rule;
    records[transactionIndex] = std::move(*rule);
    auto &materialized = records[transactionIndex];
    materialized.commits.resize(transaction.outputs.size());
    materialized.bodyChecks.resize(transaction.plan->bodyChecks.size());
    materialized.observations.resize(transaction.plan->observations.size());
    replacements.push_back({original, materializedRule});
    OpBuilder expectBuilder(materializedRule);
    expectBuilder.setInsertionPointAfter(materializedRule);
    for (auto [ordinal, control] :
         llvm::enumerate(transaction.controls)) {
      auto check =
          cast<DictionaryAttr>(transaction.expectedRequiredChecks[ordinal]);
      OperationState expect(original.getLoc(),
                            ac::SourceExpectOp::getOperationName());
      expect.addOperands(
          {materializedRule->getResult(transaction.checkBase + 2 * ordinal),
           materializedRule->getResult(transaction.checkBase + 2 * ordinal + 1),
           materializedRule->getOperand(transaction.clockCapture),
           materializedRule->getOperand(transaction.resetCapture)});
      expect.addAttribute("kind", expectBuilder.getStringAttr("guard_known"));
      expect.addAttribute("location", check.get("location"));
      expect.addAttribute("ac.check_id", check.get("id"));
      materialized.controls[ordinal].terminal =
          cast<ac::SourceExpectOp>(expectBuilder.create(expect));
    }
    auto bodyRows =
        transaction.expectedAdmission.getAs<ArrayAttr>("body_checks");
    auto observationRows =
        transaction.expectedAdmission.getAs<ArrayAttr>("observations");
    for (const auto &event : transaction.plan->events) {
      if (!charge(budget, original))
        return failure();
      if (event.kind == ac::TransactionStateEventKind::BodyCheck) {
        auto id = *event.bodyCheck;
        const auto &source = transaction.plan->bodyChecks[id];
        auto metadata = cast<DictionaryAttr>(bodyRows[id]);
        uint64_t result = transaction.checkBase +
                          2 * (transaction.controls.size() + id);
        OperationState expect(source.operation->getLoc(),
                              ac::SourceExpectOp::getOperationName());
        expect.addOperands({materializedRule->getResult(result),
                            materializedRule->getResult(result + 1)});
        expect.addAttribute("kind", expectBuilder.getStringAttr("assert"));
        expect.addAttribute("location", source.location);
        expect.addAttribute("ac.check_id", metadata.get("id"));
        if (source.message)
          expect.addAttribute("ac.message", source.message);
        materialized.bodyChecks[id] =
            cast<ac::SourceExpectOp>(expectBuilder.create(expect));
      } else if (event.kind == ac::TransactionStateEventKind::Observe) {
        auto id = *event.observation;
        const auto &source = transaction.plan->observations[id];
        auto metadata = cast<DictionaryAttr>(observationRows[id]);
        uint64_t pathResult = transaction.observationPathResults[id];
        SmallVector<Value> operands{materializedRule->getResult(pathResult)};
        for (uint64_t ordinal = 0; ordinal < source.values.size(); ++ordinal)
          operands.push_back(
              materializedRule->getResult(pathResult + 1 + ordinal));
        OperationState observe(source.operation->getLoc(),
                               ac::SourceObserveOp::getOperationName());
        observe.addOperands(operands);
        observe.addAttribute("kind", source.kind);
        observe.addAttribute("spec", source.spec);
        observe.addAttribute("ac.location", source.location);
        observe.addAttribute("ac.observation_id", metadata.get("id"));
        materialized.observations[id] =
            cast<ac::SourceObserveOp>(expectBuilder.create(observe));
      }
    }
    for (auto [ordinal, binding] : llvm::enumerate(transaction.outputs)) {
      const auto &owner = transaction.plan->owners[binding.binding.owner];
      auto path = anchorPath(*transaction.plan, owner.anchor);
      auto declaration = owner.declaration;
      auto &candidates = nextValues[transaction.plan->lexicalModule];
      auto key = routingKey(path, declaration.getQ());
      Value data = materializedRule->getResult(binding.dataResult);
      Value enable = materializedRule->getResult(binding.enableResult);
      auto prior = candidates.find(key);
      auto unordered = unorderedOwnerGroup(prepared, transaction, binding.binding.owner, budget);
      if (failed(unordered)) return failure();
      const auto &definition = prepared.definitions[prepared.definitionIndex.lookup(
          transaction.plan->lexicalModule.operator->())];
      bool crossScope = !path.empty();
      bool candidateDomains = llvm::any_of(
          transaction.writeDomains,
          [&](const auto &domain) { return domain.candidate == ordinal; });
      // An ancestor's partial writer is routed as a complete lane. Pre-compose
      // that lane with its own write domains so the receiving definition and
      // the final proof can recover exactly which leaves it owns; masks never
      // cross a hierarchy port.
      if (*unordered || definition.downwardIndex.contains(key) ||
          (prior == candidates.end() && crossScope && candidateDomains)) {
        Value previous;
        if (prior != candidates.end()) previous = prior->second.first;
        else {
          auto oldQ = routes.value(transaction.plan->lexicalModule, path,
                                   declaration.getQ());
          if (failed(oldQ)) return failure();
          previous = *oldQ;
        }
        OpBuilder commitBuilder(materializedRule);
        commitBuilder.setInsertionPointAfter(materializedRule);
        SmallVector<DomainCommitUse, 0> domains;
        for (const auto &domain : transaction.writeDomains) {
          if (!charge(budget, original)) return failure();
          if (domain.candidate != ordinal) continue;
          if (!prepayAppend(budget, original, domains)) return failure();
          domains.push_back({domain.selectors.getValue(), materializedRule->getResult(domain.result)});
        }
        auto composed = materializeDomainCommit(original, original.getOccurrence(), owner.payloadType,
            transaction.plan->frames[transaction.plan->rootFrame].bindings, previous,
            data, enable, domains, hardware, commitBuilder, budget);
        if (failed(composed)) return failure();
        auto &commit = materialized.commits[ordinal];
        commit.data = std::move(*composed); commit.domainMerge = true; commit.enable = enable;
        if (prior != candidates.end()) {
          auto gate = [&](Value first, Value second, StringRef opcode) -> FailureOr<Value> {
            if (!charge(budget, original, 32) || !prepayAppend(budget, original, commit.enableGenerated)) return failure();
            OperationState state(original->getLoc(), ac::BitsBinaryOp::getOperationName());
            state.addOperands({first, second}); state.addTypes(first.getType());
            state.addAttribute("opcode", commitBuilder.getStringAttr(opcode));
            Operation *operation = commitBuilder.create(state); commit.enableGenerated.push_back(operation);
            return operation->getResult(0);
          };
          auto active = gate(prior->second.second, enable, "or");
          auto leftPoison = gate(prior->second.second, prior->second.second, "xor");
          auto rightPoison = gate(enable, enable, "xor");
          if (failed(active) || failed(leftPoison) || failed(rightPoison)) return failure();
          auto poison = gate(*leftPoison, *rightPoison, "or");
          if (failed(poison)) return failure();
          auto combined = gate(*active, *poison, "xor");
          if (failed(combined)) return failure(); commit.enable = *combined;
        }
        candidates[key] = {commit.data.value, commit.enable};
        continue;
      }
      if (prior == candidates.end()) {
        candidates.try_emplace(key, std::make_pair(data, enable));
      } else {
        OpBuilder commitBuilder(materializedRule);
        commitBuilder.setInsertionPointAfter(materializedRule);
        auto selected = materializeSelection(original, transaction.plan->frames[0].bindings,
            enable, data, prior->second.first, owner.payloadType, commitBuilder, budget);
        if (failed(selected) || !charge(budget, original, 32)) return failure();
        OperationState either(original.getLoc(), ac::BitsBinaryOp::getOperationName());
        either.addOperands({prior->second.second, enable});
        either.addTypes(enable.getType());
        either.addAttribute("opcode", commitBuilder.getStringAttr("or"));
        Value combined = commitBuilder.create(either)->getResult(0);
        materialized.commits[ordinal] = {std::move(*selected), combined};
        prior->second = {materialized.commits[ordinal].data.value, combined};
      }
    }
  }
  // All Rules exist before ordinary transaction results disappear. Captures
  // from another Rule retain actual graph dependencies and saved SSA values.
  for (auto replacement : replacements) {
    auto original = replacement.original;
    auto transaction =
        cast<ac::SourceTransactionOp>(mapping.lookup(original.operator->()));
    for (auto [source, old, result] :
         llvm::zip(original.getResults(), transaction.getResults(),
                   replacement.rule.getResults().take_front(
                       original.getNumResults()))) {
      old.replaceAllUsesWith(result);
      mapping.map(source, result);
    }
  }

  // The candidate map contains the closed incoming prefix followed by local
  // contributors. Absent either, E is zero for this actual occurrence.
  auto candidate = [&](ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                       Value state, Type dType, Type eType,
                       DictionaryAttr occurrence, Location location,
                       OpBuilder &builder) -> std::pair<Value, Value> {
    auto key = routingKey(path, state);
    auto local = nextValues[module].find(key);
    if (local != nextValues[module].end())
      return local->second;
    // Disabled proposals retain the owner's typed old-Q. This has no
    // candidate write intent and needs no invented nominal zero value.
    if (path.empty())
      return {mapping.lookup(state),
              bitsLiteral(eType, occurrence, location, false, builder)};
    if (isa<ac::BitsType>(dType))
      return {bitsLiteral(dType, occurrence, location, false, builder),
              bitsLiteral(eType, occurrence, location, false, builder)};
    auto oldQ = routes.value(module, path, state);
    assert(succeeded(oldQ) && "prepared old-Q route must exist");
    return {*oldQ, bitsLiteral(eType, occurrence, location, false, builder)};
  };

  for (auto &parent : prepared.definitions) {
    for (auto original :
         parent.module->getRegion(0).front().getOps<ac::InstanceOp>()) {
      auto child =
          dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(original));
      if (!child)
        continue;
      auto &definition =
          prepared
              .definitions[prepared.definitionIndex.lookup(child.operator->())];
      if (definition.downward.empty())
        continue;
      auto allocation =
          cast<ac::InstanceOp>(mapping.lookup(original.operator->()));
      SmallVector<Value> operands(
          allocation.getOperands().take_front(definition.sourceInputs));
      OpBuilder builder(allocation);
      for (auto [ordinal, route] : llvm::enumerate(definition.downward)) {
        SmallVector<ac::InstanceOp> path{original};
        llvm::append_range(path, route.path);
        auto bindings = hardware.bindInstance(original);
        if (failed(bindings)) return failure();
        auto dataType = hardware.resolveType(route.type, *bindings, original);
        auto enableType = hardware.resolveType(route.enableType, *bindings, original);
        if (failed(dataType) || failed(enableType)) return failure();
        auto [d, e] =
            candidate(parent.module, path, route.value, *dataType, *enableType,
                      original.getOccurrence(), original.getLoc(), builder);
        operands.push_back(d);
        operands.push_back(e);
      }
      allocation->setOperands(operands);
    }
    if (!parent.upward.empty()) {
      auto module =
          cast<ac::ModuleOp>(mapping.lookup(parent.module.operator->()));
      auto yield = cast<ac::YieldOp>(module.getBody().front().getTerminator());
      SmallVector<Value> outputs(yield.getValues());
      for (const auto &route : parent.upward) {
        auto value = routes.value(parent.module, route.path, route.value);
        if (failed(value))
          return failure();
        outputs.push_back(*value);
      }
      yield->setOperands(outputs);
    }
  }
  for (auto row : storage) {
    auto original = row.declaration;
    auto module = original->getParentOfType<ac::ModuleOp>();
    OpBuilder builder(row.instance);
    auto [d, enable] =
        candidate(module, {}, original.getQ(), original.getQ().getType(),
                  original.getClk().getType(), original.getOccurrence(),
                  original.getLoc(), builder);
    auto table = dyn_cast<ac::TableType>(original.getQ().getType());
    // Final DFF admission authenticates one local candidate. A composed
    // enable has no physical pin after DFF selection, so retain DFFE for
    // ordered/unordered or incoming composition until its full independent
    // enable proof is supported. Single-candidate enables are direct Rule
    // results; composition above produces module-level operations/arguments.
    auto localWriter = enable.getDefiningOp<ac::RuleOp>();
    bool dff = !table && localWriter &&
               localWriter->getParentOfType<ac::ModuleOp>() ==
                   row.instance->getParentOfType<ac::ModuleOp>() &&
               knownTrue(enable, hardware, budget);
    SmallVector<Value> inputs{mapping.lookup(original.getClk()),
                              mapping.lookup(original.getRst())};
    SmallVector<Value> controls{enable, d, mapping.lookup(original.getInitial())};
    if (dff)
      controls.erase(controls.begin());
    llvm::append_range(inputs, controls);
    if (table)
      for (unsigned i : {0u, 1u, 2u}) {
        OperationState splat(original.getLoc(), ac::TableSplatOp::getOperationName());
        splat.addOperands(inputs[i]);
        splat.addTypes(ac::TableType::get(staged.getContext(), table.getShape(),
                                          inputs[i].getType()));
        splat.addAttribute("shape", table.getShape());
        inputs[i] = builder.create(splat)->getResult(0);
      }
    if (dff) {
      if (!prepared.dff)
        return row.instance->emitOpError()
               << "unconditional state requires the verified DFF primitive";
      row.instance->setAttr(
          "callee",
          FlatSymbolRefAttr::get(
              staged.getContext(),
              cast<StringAttr>(prepared.dff->getAttr(
                                   SymbolTable::getSymbolAttrName()))
                  .getValue()));
      row.dff = true;
      prepared.dffStates.insert(original.getOperation());
    }
    row.instance->setOperands(inputs);
  }
  SmallVector<Operation *> retired;
  for (auto replacement : replacements) {
    auto old = mapping.lookupOrNull(replacement.original.operator->());
    if (!old)
      return replacement.original->emitOpError()
             << "original replacement clone is missing from the package map";
    mapping.map(replacement.original.operator->(),
                replacement.rule.operator->());
    retired.push_back(old);
  }
  for (auto module : staged.getOps<ac::ModuleOp>())
    for (auto method : llvm::make_early_inc_range(
             module.getBody().front().getOps<ac::SourceMethodOp>()))
      retired.push_back(method);
  if (!retired.empty() &&
      failed(eraseTargetMappings(mapping, retired, budget)))
    return failure();
  for (Operation *old : retired) {
    if (!old)
      return staged.emitError() << "retired clone mapping is null";
    old->erase();
  }
  for (const auto &definition : prepared.definitions) {
    auto module =
        cast<ac::ModuleOp>(mapping.lookup(definition.module.operator->()));
    if (definition.expectedWiring)
      module->setAttr("ac.transaction_wiring", definition.expectedWiring);
  }
  return success();
}

/// Authenticate locators against the live detached graph before dereferencing.
/// Erased source objects are never consulted through clone mappings.
bool sameBindings(const ac::HardwareBindings &lhs,
                  const ac::HardwareBindings &rhs) {
  if (lhs.owner != rhs.owner || lhs.integers.size() != rhs.integers.size() ||
      lhs.types.size() != rhs.types.size())
    return false;
  for (const auto &entry : lhs.integers)
    if (rhs.integers.lookup(entry.getKey()) != entry.second)
      return false;
  for (const auto &entry : lhs.types)
    if (rhs.types.lookup(entry.getKey()) != entry.second)
      return false;
  return true;
}

// Reconstruct original value identities and availability independently of the
// producer's aliases, frame maps and supplied validity records. Build one
// independent identity index; only its storage is shared by replay lookups.
class SourceReplayValues {
public:
  SourceReplayValues(const ac::TransactionStatePlan &plan,
                     SourceCheckBudget &budget)
      : plan(plan), budget(budget),
        hardware(plan.transaction->getParentOfType<mlir::ModuleOp>()) {}

  LogicalResult initialize() {
    uint64_t n = plan.values.size();
    if (n > std::numeric_limits<unsigned>::max())
      return error("source replay capacity exceeded");
    // Reserve once for the complete immutable index. Use the pinned DenseMap
    // load/growth rule, as in StateAnalysisBudget::reserveMapInsert. There are
    // no retained buckets to move because each replay starts with an empty map.
    uint64_t buckets = n ? 64 : 0;
    while (n && SourceCheckBudget::multiply(n, 4) >=
                    SourceCheckBudget::multiply(buckets, 3)) {
      if (!charge(budget, plan.transaction))
        return failure();
      buckets = SourceCheckBudget::multiply(buckets, 2);
    }
    if (!charge(budget, plan.transaction,
                SourceCheckBudget::bytes(SourceCheckBudget::multiply(
                    buckets, sizeof(decltype(index)::value_type)))) ||
        !product(budget, plan.transaction, n,
                 3 + SourceCheckBudget::bytes(
                         sizeof(ac::TransactionPoint) + sizeof(bool) +
                         2 * sizeof(std::optional<ac::BoundValueId>))) ||
        !product(
            budget, plan.transaction, plan.branches.size(),
            1 + SourceCheckBudget::bytes(sizeof(ac::TransactionSourcePath))))
      return failure();
    if (n > validity.max_size() || n > assigned.max_size() ||
        n > edges.max_size() || n > resultEdges.max_size() ||
        plan.branches.size() > parents.max_size())
      return error("source replay capacity exceeded");
    index.reserve(n);
    validity.resize(n);
    assigned.resize(n, false);
    edges.resize(n);
    resultEdges.resize(n);
    parents.reserve(plan.branches.size());
    for (auto [id, ref] : llvm::enumerate(plan.values)) {
      if (!ref.value || ref.anchor >= plan.instances.size() ||
          ref.frame >= plan.frames.size())
        return error("bound value identity is out of range");
      bool lexical =
          moduleValue(ref.value, plan.instances[ref.anchor].definition);
      if ((lexical && ref.frame != plan.rootFrame) ||
          (!lexical && ref.anchor != plan.frames[ref.frame].anchor))
        return error("bound value frame/anchor differs");
      if (!index.try_emplace(Key{ref.value, {ref.anchor, ref.frame}}, id)
               .second)
        return error("duplicate bound value identity");
      if (lexical &&
          ref.value.getDefiningOp() != plan.transaction.operator->()) {
        if (failed(assignValidity(id, {})))
          return failure();
      }
    }
    return success();
  }

  FailureOr<ac::BoundValueId> find(Value value, ac::LexicalInstanceId anchor,
                                   ac::InvocationFrameId frame) {
    if (anchor >= plan.instances.size())
      return failure();
    if (moduleValue(value, plan.instances[anchor].definition))
      frame = plan.rootFrame;
    if (!charge(budget, plan.transaction))
      return failure();
    auto found = index.find(Key{value, {anchor, frame}});
    if (found == index.end()) {
      (void)error("original SSA is absent from bound values");
      return failure();
    }
    return found->second;
  }
  LogicalResult identify(ac::BoundValueId id, Value original,
                         ac::LexicalInstanceId anchor,
                         ac::InvocationFrameId frame) {
    auto expected = find(original, anchor, frame);
    if (failed(expected) || *expected != id)
      return error("original bound value differs");
    return success();
  }
  LogicalResult branch(ac::BranchId id, ac::TransactionSourcePath parent) {
    if (!charge(budget, plan.transaction) || id != parents.size())
      return failure();
    parents.push_back(parent);
    return success();
  }
  FailureOr<bool> sourcePathDominates(ac::TransactionSourcePath definition,
                                      ac::TransactionSourcePath use) {
    for (unsigned depth = 0; depth <= 128; ++depth) {
      if (!charge(budget, plan.transaction))
        return failure();
      if ((!definition.branch && definition.whenTrue) ||
          (!use.branch && use.whenTrue) ||
          (definition.branch && *definition.branch >= parents.size()) ||
          (use.branch && *use.branch >= parents.size()))
        return failure();
      if (sameSourcePath(definition, use))
        return true;
      if (!use.branch)
        return false;
      use = parents[*use.branch];
    }
    (void)error("source path depth exceeded");
    return failure();
  }
  LogicalResult assignValidity(ac::BoundValueId id,
                               ac::TransactionPoint point) {
    if (!charge(budget, plan.transaction) || id >= validity.size() ||
        !sameSourcePoint(plan.values[id].validity, point) ||
        (assigned[id] && !sameSourcePoint(validity[id], point)))
      return error("original value validity differs");
    validity[id] = point;
    assigned[id] = true;
    return success();
  }
  LogicalResult requireAvailable(ac::BoundValueId id,
                                 ac::TransactionPoint use) {
    if (!charge(budget, plan.transaction) || id >= assigned.size() ||
        !assigned[id])
      return error("original value has no definition");
    auto dominates = sourcePathDominates(validity[id].path, use.path);
    if (failed(dominates) || !*dominates || validity[id].prefix > use.prefix)
      return error("original value is unavailable");
    return success();
  }
  LogicalResult requireVersionAvailable(ac::StateVersionId id,
                                        ac::TransactionPoint validityPoint,
                                        ac::TransactionPoint use) {
    if (!charge(budget, plan.transaction) || id >= plan.versions.size() ||
        !sameSourcePoint(plan.versions[id].validity, validityPoint))
      return error("original version validity differs");
    auto dominates = sourcePathDominates(validityPoint.path, use.path);
    if (failed(dominates) || !*dominates || validityPoint.prefix > use.prefix)
      return error("original version is unavailable");
    return success();
  }
  LogicalResult binding(ac::BoundValueId formal, ac::BoundValueId actual,
                        ac::TransactionPoint entry) {
    if (failed(requireAvailable(actual, entry)) || formal >= edges.size() ||
        edges[formal] || failed(assignValidity(formal, validity[actual])))
      return failure();
    edges[formal] = actual;
    return success();
  }
  LogicalResult snapshot(ac::BoundValueId output, ac::BoundValueId selected,
                          ac::TransactionPoint point) {
    if (output >= edges.size() || edges[output] || resultEdges[output] ||
        failed(requireAvailable(selected, point)) || failed(assignValidity(output, point)))
      return error("original formal snapshot binding differs");
    edges[output] = selected;
    return success();
  }
  FailureOr<ac::BoundValueId> identity(ac::BoundValueId id,
                                       ac::TransactionPoint point) {
    for (unsigned depth = 0; depth <= 258; ++depth) {
      if (failed(requireAvailable(id, point))) return failure();
      if (edges[id]) { id = *edges[id]; continue; }
      const auto &ref = plan.values[id];
      auto argument = dyn_cast<BlockArgument>(ref.value);
      if (!argument || argument.getOwner()->getParentOp() !=
              plan.instances[ref.anchor].definition || !plan.instances[ref.anchor].parent)
        return id;
      const auto &instance = plan.instances[ref.anchor];
      if (argument.getArgNumber() >= instance.allocation->getNumOperands()) return failure();
      auto actual = find(instance.allocation->getOperand(argument.getArgNumber()),
                         *instance.parent, plan.rootFrame);
      if (failed(actual)) return failure();
      id = *actual;
    }
    return failure();
  }
  LogicalResult result(ac::BoundValueId output, ac::BoundValueId yielded,
                       ac::TransactionPoint exit) {
    if (output == yielded || output >= resultEdges.size() || edges[output] ||
        resultEdges[output] || failed(requireAvailable(yielded, exit)) ||
        failed(assignValidity(output, exit)))
      return error("original call result binding differs");
    resultEdges[output] = yielded;
    return success();
  }
  FailureOr<BoolAttr> fact(ac::BoundValueId id, ac::TransactionPoint use) {
    for (unsigned depth = 0; depth <= 128; ++depth) {
      if (failed(requireAvailable(id, use)))
        return failure();
      if (edges[id]) {
        id = *edges[id];
        continue;
      }
      if (resultEdges[id]) {
        id = *resultEdges[id];
        continue;
      }
      const auto &ref = plan.values[id];
      if (auto arg = dyn_cast<BlockArgument>(ref.value)) {
        if (arg.getOwner()->getParentOp() ==
                plan.instances[ref.anchor].definition &&
            plan.instances[ref.anchor].parent) {
          const auto &instance = plan.instances[ref.anchor];
          if (arg.getArgNumber() >= instance.allocation->getNumOperands())
            return failure();
          auto actual =
              find(instance.allocation->getOperand(arg.getArgNumber()),
                   *instance.parent, plan.rootFrame);
          if (failed(actual))
            return failure();
          id = *actual;
          continue;
        }
      }
      auto forwarded =
          ac::detail::getSourcePredicateIdentityInput(ref.value, hardware);
      if (failed(forwarded))
        return failure();
      if (!*forwarded)
        return ac::detail::getSourceLiteralPredicate(ref.value, hardware);
      auto next = find(*forwarded, ref.anchor, ref.frame);
      if (failed(next))
        return failure();
      id = *next;
    }
    (void)error("predicate identity depth exceeded");
    return failure();
  }
  // Domain identity deliberately does not follow ordinary call results.
  FailureOr<ac::DomainEndpoint> endpoint(ac::BoundValueId id,
                                         ac::TransactionPoint use) {
    for (unsigned depth = 0; depth <= 258; ++depth) {
      if (failed(requireAvailable(id, use)))
        return failure();
      if (edges[id]) {
        id = *edges[id];
        continue;
      }
      const auto &ref = plan.values[id];
      auto argument = dyn_cast<BlockArgument>(ref.value);
      if (!argument ||
          argument.getOwner()->getParentOp() !=
              plan.instances[ref.anchor].definition ||
          !plan.instances[ref.anchor].parent)
        return ac::DomainEndpoint{ref.value, ref.anchor};
      const auto &instance = plan.instances[ref.anchor];
      if (argument.getArgNumber() >= instance.allocation->getNumOperands())
        return failure();
      auto actual =
          find(instance.allocation->getOperand(argument.getArgNumber()),
               *instance.parent, plan.rootFrame);
      if (failed(actual))
        return failure();
      id = *actual;
    }
    (void)error("domain identity depth exceeded");
    return failure();
  }
  LogicalResult authoredTrue(ac::BoundValueId id, ac::TransactionPoint use) {
    auto known = fact(id, use);
    if (failed(known) || !*known || !known->getValue())
      return error("authored effect path is not true");
    return success();
  }
  ac::TransactionPoint validityOf(ac::BoundValueId id) const {
    return validity[id];
  }
  LogicalResult complete() {
    for (bool present : assigned)
      if (!charge(budget, plan.transaction) || !present)
        return error("unreplayed original bound value");
    return success();
  }

private:
  using Key = std::pair<Value, std::pair<ac::LexicalInstanceId,
                                        ac::InvocationFrameId>>;
  LogicalResult error(StringRef message) {
    return plan.transaction->emitOpError()
           << "transaction source replay: " << message;
  }
  const ac::TransactionStatePlan &plan;
  SourceCheckBudget &budget;
  ac::HardwareAnalysis hardware;
  llvm::DenseMap<Key, ac::BoundValueId> index;
  SmallVector<ac::TransactionPoint, 0> validity;
  SmallVector<bool, 0> assigned;
  SmallVector<std::optional<ac::BoundValueId>, 0> edges, resultEdges;
  SmallVector<ac::TransactionSourcePath, 0> parents;
};

class OriginalSourceProof {
public:
  OriginalSourceProof(const PreparedTransactionLowering &prepared,
                      const MaterializedTransactionLowering &candidate,
                      SourceCheckBudget &budget)
      : prepared(prepared), candidate(candidate), budget(budget),
        hardware(prepared.original) {}

  LogicalResult run() {
    auto staged = *candidate.module;
    if (!charge(budget, prepared.original,
                SourceCheckBudget::bytes(sizeof(*this))))
      return failure();
    if (staged->getName() != prepared.original->getName() ||
        staged->getNumRegions() != prepared.original->getNumRegions() ||
        staged->getLoc() != prepared.original->getLoc())
      return reject("original package shape differs");
    NamedAttrList expectedEnvelope(prepared.original->getAttrs());
    expectedEnvelope.set("ac.stage",
                         StringAttr::get(staged.getContext(), "final"));
    if (staged->getAttrs() != expectedEnvelope.getAttrs() ||
        staged->getPropertiesAsAttribute() !=
            prepared.original->getPropertiesAsAttribute())
      return reject("original package envelope differs");
    uint64_t locatorEntries = SourceCheckBudget::add(
        candidate.packageMap.getOperationMap().size(),
        SourceCheckBudget::add(candidate.packageMap.getBlockMap().size(),
                               candidate.packageMap.getValueMap().size()));
    if (!charge(budget, prepared.original,
                SourceCheckBudget::multiply(locatorEntries, 4)))
      return failure();
    // The clone map already enumerates every operation and block copied from
    // the original package. Seed the live-inventory sets from it once instead
    // of re-allocating the same entries during the staged walk. The walk still
    // visits every staged operation, charges that traversal, and pays full
    // inventory cost only for generated operations absent from the map.
    for (const auto &entry : candidate.packageMap.getOperationMap())
      if (entry.second) {
        operations.insert(entry.second);
        for (auto result : entry.second->getResults())
          values.insert(result);
      }
    for (const auto &entry : candidate.packageMap.getBlockMap())
      if (entry.second) {
        blocks.insert(entry.second);
        for (auto argument : entry.second->getArguments())
          values.insert(argument);
      }
    for (const auto &entry : candidate.packageMap.getValueMap())
      if (entry.second)
        values.insert(entry.second);
    llvm::DenseSet<Operation *> visited;
    bool paid = true;
    staged.walk([&](Operation *op) {
      if (!paid)
        return;
      if (!charge(budget, prepared.original, 1)) {
        paid = false;
        return;
      }
      bool cloned = operations.contains(op);
      visited.insert(op);
      if (!cloned &&
          !charge(budget, prepared.original,
                  8 + 4 * op->getNumResults() + op->getNumRegions())) {
        paid = false;
        return;
      }
      operations.insert(op);
      for (auto result : op->getResults())
        values.insert(result);
      for (Region &region : op->getRegions())
        for (Block &block : region) {
          if (!blocks.contains(&block) &&
              !charge(budget, prepared.original,
                      4 + 4 * block.getNumArguments())) {
            paid = false;
            return;
          }
          blocks.insert(&block);
          for (auto argument : block.getArguments())
            values.insert(argument);
        }
    });
    if (!paid)
      return failure();
    for (const auto &entry : candidate.packageMap.getOperationMap())
      if (!entry.second || !visited.contains(entry.second))
        return reject("package mapping contains an erased operation");
    if (prepared.valueAvailability) {
      auto proved = ac::detail::getVerifiedValueAvailabilityUses(ac::HardwareAnalysis(staged));
      if (failed(proved)) return failure();
      availabilityUses = std::move(*proved);
    }
    for (const auto &entry : candidate.packageMap.getOperationMap())
      if (!operations.contains(entry.second))
        return reject("package mapping contains an erased operation");
    for (const auto &entry : candidate.packageMap.getBlockMap())
      if (!blocks.contains(entry.second))
        return reject("package mapping contains an erased block");
    for (const auto &entry : candidate.packageMap.getValueMap())
      if (!values.contains(entry.second))
        return reject("package mapping contains an erased value");
    if (candidate.transactions.size() != prepared.transactions.size() ||
        staged.getBody()->getOperations().size() !=
            prepared.original->getRegion(0).front().getOperations().size())
      return reject("original transaction/declaration inventory differs");
    if (failed(orderProof())) return failure();
    for (const auto &definition : prepared.definitions)
      if (failed(definitionProof(definition)))
        return failure();
    for (auto [index, transaction] : llvm::enumerate(prepared.transactions))
      if (failed(transactionProof(transaction, candidate.transactions[index])))
        return failure();
    // Package declarations other than module bodies retain their entire shape.
    for (Operation &op :
         prepared.original->getRegion(0).front().getOperations()) {
      if (isa<ac::ModuleOp>(op))
        continue;
      auto target = candidate.packageMap.lookupOrNull(&op);
      if (!target || !operations.contains(target) ||
          target->getParentOp() != staged.operator->())
        return reject("original package declaration locator differs");
      IRMapping authenticated;
      for (const auto &entry : candidate.packageMap.getValueMap())
        authenticated.map(entry.first, entry.second);
      if (failed(exactClone(&op, target, authenticated, candidate.packageMap)))
        return failure();
    }
    if (failed(routingProof()) || failed(queueWiringProof()) || failed(memoryPinsProof()))
      return failure();
    if (prepared.valueAvailability) {
      SmallVector<Value> pending;
      DenseSet<Value> seen;
      DenseSet<Operation *> reachable;
      ac::HardwareAnalysis finalHardware(staged);
      for (auto module : finalHardware.getDefinitions()) {
        auto layout = ac::getModulePortLayout(module);
        if (failed(layout)) return failure();
        auto yield = cast<ac::YieldOp>(module.getBody().front().getTerminator());
        for (int64_t ordinal : layout->outputValidity)
          if (ordinal >= 0) pending.push_back(yield->getOperand(ordinal));
        for (auto [payload, validity] : layout->routedOutputValidity)
          pending.push_back(yield->getOperand(validity));
        for (auto rule : module.getBody().front().getOps<ac::RuleOp>())
          for (auto requirement : rule.getBody().front().getOps<ac::ValueRequireOp>())
            pending.push_back(requirement.getAvailability());
        for (auto instance : module.getBody().front().getOps<ac::InstanceOp>()) {
          auto child = dyn_cast_or_null<ac::ModuleOp>(finalHardware.resolveCallee(instance));
          if (!child) continue;
          auto inputs = ac::getModulePortLayout(child);
          if (failed(inputs)) return failure();
          for (int64_t ordinal : inputs->inputValidity)
            if (ordinal >= 0) pending.push_back(instance->getOperand(ordinal));
        }
      }
      while (!pending.empty()) {
        Value value = pending.pop_back_val();
        if (!charge(budget, prepared.original)) return failure();
        if (!seen.insert(value).second) continue;
        if (auto argument = dyn_cast<BlockArgument>(value)) {
          if (auto rule = dyn_cast<ac::RuleOp>(argument.getOwner()->getParentOp()))
            pending.push_back(rule->getOperand(argument.getArgNumber()));
          continue;
        }
        auto *operation = value.getDefiningOp();
        if (!operation) return failure();
        reachable.insert(operation);
        if (auto rule = dyn_cast<ac::RuleOp>(operation)) {
          pending.push_back(rule.getBody().front().getTerminator()->getOperand(
              cast<OpResult>(value).getResultNumber()));
          continue;
        }
        if (isa<ac::InstanceOp, ac::CollectionOp>(operation)) continue;
        if (auto require = dyn_cast<ac::ValueRequireOp>(operation)) {
          pending.push_back(require.getAvailability());
          pending.push_back(require.getPath());
          continue;
        }
        llvm::append_range(pending, operation->getOperands());
      }
      for (auto *operation : candidate.availabilityOperations) {
        if (!operations.contains(operation) || !reachable.contains(operation) ||
            !isa<ac::BitsConstantOp, ac::BitsConstant4Op, ac::BitsBinaryOp,
                 ac::BitsCompareOp, ac::BitsSelectOp>(operation) ||
            failed(cover(operation)))
          return reject("original generated availability graph has extra operations");
      }
    } else if (!candidate.availabilityOperations.empty()) {
      return reject("original total package has invented availability operations");
    }
    if (failed(cover(staged)))
      return failure();
    for (Operation *op : operations) {
      if (!charge(budget, prepared.original, 1))
        return failure();
      if (!clones.contains(op))
        return reject(isa<ac::ModuleOp>(op->getParentOp())
                          ? "original module-body operation inventory differs"
                          : "original nested operation inventory differs");
    }
    return success();
  }

private:
  LogicalResult reject(StringRef message) {
    return prepared.original->emitError()
           << "transaction original-source proof failed: " << message;
  }

  struct OriginalRegistration {
    uint64_t index;
    SmallVector<ac::SourceTransactionOp, 0> predecessors;
    bool proven = false;
  };

  // This index and the successful closures belong to this immutable source
  // proof only. They never import the preparer's schedule as evidence.
  FailureOr<OriginalRegistration *> originalRegistration(
      ac::SourceTransactionOp transaction) {
    auto lookupWork = [](size_t count) {
      uint64_t work = 1;
      for (uint64_t nodes = SourceCheckBudget::add(count, 1); nodes;
           nodes >>= 1)
        work = SourceCheckBudget::add(work, 2);
      return work;
    };
    if (!registrationsIndexed) {
      for (auto [index, entry] : llvm::enumerate(prepared.transactions)) {
        if (!charge(budget, entry.plan->transaction,
                    SourceCheckBudget::add(
                        lookupWork(originalRegistrations.size()),
                        SourceCheckBudget::bytes(
                            sizeof(decltype(originalRegistrations)::value_type) +
                            4 * sizeof(void *)))))
          return failure();
        if (!originalRegistrations.try_emplace(
                 entry.plan->transaction.operator->(),
                 OriginalRegistration{index, {}, false}).second)
          return reject("original registration appears in multiple plans");
      }
      registrationsIndexed = true;
    }
    if (!charge(budget, transaction, lookupWork(originalRegistrations.size())))
      return failure();
    auto entry = originalRegistrations.find(transaction.getOperation());
    if (entry == originalRegistrations.end())
      return reject("original predecessor has no complete plan");
    return &entry->second;
  }

  FailureOr<ArrayRef<ac::SourceTransactionOp>> originalPredecessors(
      ac::SourceTransactionOp transaction) {
    auto after = transaction->getAttrOfType<ArrayAttr>("ac.after");
    if (!after || after.empty()) {
      if (!charge(budget, transaction)) return failure();
      return ArrayRef<ac::SourceTransactionOp>();
    }
    auto cached = originalRegistration(transaction);
    if (failed(cached)) return failure();
    if ((*cached)->proven)
      return ArrayRef<ac::SourceTransactionOp>((*cached)->predecessors);
    SmallVector<ac::SourceTransactionOp, 0> result;
    llvm::DenseSet<Operation *> active, seen;
    std::function<LogicalResult(ac::SourceTransactionOp, unsigned)> visit =
        [&](ac::SourceTransactionOp current, unsigned depth) -> LogicalResult {
      if (depth > 128 || !charge(budget, current)) return failure();
      if (!active.insert(current).second) return reject("original after graph is cyclic");
      auto module = current->getParentOfType<ac::ModuleOp>();
      if (auto after = current->getAttrOfType<ArrayAttr>("ac.after"))
        for (Attribute identity : after) {
          ac::SourceTransactionOp predecessor;
          unsigned count = 0;
          for (auto possible : module.getBody().front().getOps<ac::SourceTransactionOp>()) {
            if (!charge(budget, current)) return failure();
            if (ac::detail::compareClosedSourceStructure(possible.getOccurrence(), identity) == 0) {
              predecessor = possible;
              ++count;
            }
          }
          if (count != 1 || predecessor == current)
            return reject("original after registration is absent or ambiguous");
          if (!seen.contains(predecessor)) {
            if (failed(visit(predecessor, depth + 1))) return failure();
            seen.insert(predecessor);
            if (!prepayAppend(budget, current, result)) return failure();
            result.push_back(predecessor);
          }
        }
      active.erase(current);
      return success();
    };
    if (failed(visit(transaction, 0))) return failure();
    // Only cache a complete top-level DFS. Recursive reuse could hide the
    // depth bound or change active-cycle checks and postorder de-duplication.
    (*cached)->predecessors = std::move(result);
    (*cached)->proven = true;
    return ArrayRef<ac::SourceTransactionOp>((*cached)->predecessors);
  }

  FailureOr<uint64_t> originalTransactionIndex(ac::SourceTransactionOp transaction) {
    auto entry = originalRegistration(transaction);
    if (failed(entry)) return failure();
    return (*entry)->index;
  }

  LogicalResult orderProof() {
    SmallVector<uint64_t> order;
    llvm::DenseSet<uint64_t> seen;
    for (auto [index, preparation] : llvm::enumerate(prepared.transactions)) {
      auto ancestors = originalPredecessors(preparation.plan->transaction);
      if (failed(ancestors) || !llvm::equal(*ancestors, preparation.plan->predecessors))
        return reject("original predecessor closure differs from prepared plan");
      for (auto transaction : *ancestors) {
        auto id = originalTransactionIndex(transaction);
        if (failed(id)) return failure();
        if (seen.insert(*id).second) order.push_back(*id);
      }
      if (seen.insert(index).second) order.push_back(index);
    }
    if (!llvm::equal(order, prepared.transactionSchedule))
      return reject("original dependency traversal differs from prepared schedule");
    return success();
  }

#include "TransactionQueueWiringProof.inc"
#include "TransactionMemoryPinsProof.inc"

  LogicalResult cover(Operation *op) {
    if (!charge(budget, prepared.original, 4))
      return failure();
    if (!op || !operations.contains(op) || !clones.insert(op).second)
      return reject(
          "original generated operation identity is absent or duplicated");
    return success();
  }

  /// These operation shapes are derived from the lowering recipe on the intact
  /// plan. Candidate metadata or an arbitrary same-valued wire is not evidence.
  LogicalResult generatedProof(Operation *op, StringRef name,
                               ArrayRef<Value> operands, ArrayRef<Type> types,
                               DictionaryAttr attributes, Location location,
                               Block *parent, unsigned regions = 0) {
    if (!charge(budget, prepared.original,
                16 + 4 * operands.size() + 4 * types.size() +
                    attributes.size()))
      return failure();
    if (!op || !operations.contains(op) ||
        op->getName().getStringRef() != name || op->getBlock() != parent ||
        op->getLoc() != location || op->getNumRegions() != regions ||
        !llvm::equal(op->getOperands(), operands) ||
        op->getResultTypes() != types ||
        op->getAttrs() != attributes.getValue())
      return reject(
          "original generated operation shape/operands/attributes differ");
    if (Attribute raw = op->getPropertiesAsAttribute()) {
      auto properties = dyn_cast<DictionaryAttr>(raw);
      if (!properties)
        return reject("original generated operation properties differ");
      for (auto property : properties)
        if (attributes.get(property.getName()) != property.getValue())
          return reject("original generated operation properties differ");
    }
    return cover(op);
  }

#include "TransactionSelectedWriteProof.inc"

  LogicalResult localBranchProof(Operation *branch, Type resolvedRoot,
                                 const ac::HardwareBindings &bindings,
                                 Value condition, Value whenTrue,
                                 Value whenFalse,
                                 const MaterializedBranchResult &record,
                                 Block *parent, Operation *&last,
                                 BranchTypeAdmissionCache *typeCache = nullptr) {
    auto &admissionCache = typeCache ? *typeCache : branchTypes;
    if (!record.value || !values.contains(record.value) ||
        !values.contains(condition) || !values.contains(whenTrue) ||
        !values.contains(whenFalse))
      return reject("original local branch locator is not a live value");
    // The immediately preceding original-result check admitted this exact root
    // with the same original hardware and lexical bindings. Reuse only its
    // resolved Type here; descendant checks and graph proof remain independent.
    if (!resolvedRoot ||
        !ac::areEquivalentHardwareTypes(whenTrue.getType(), resolvedRoot) ||
        !ac::areEquivalentHardwareTypes(whenFalse.getType(), resolvedRoot))
      return reject("original local branch input type differs");

    if (isa<ac::StructType, ac::TableType>(resolvedRoot)) {
      auto admitted = admissionCache.admit(resolvedRoot, hardware, bindings, branch, budget);
      if (failed(admitted) || *admitted != resolvedRoot)
        return reject("original aggregate branch root admission differs");
    }

    // Count from the original type tree, independently of materializer
    // metadata. Repeated sibling declarations are distinct visits, not nominal
    // cycles.
    struct CountFrame {
      Type type;
      unsigned depth;
    };
    SmallVector<CountFrame, 0> counting;
    if (!prepayAppend(budget, branch, counting))
      return failure();
    counting.push_back({resolvedRoot, 0});
    uint64_t expectedCount = 0;
    while (!counting.empty()) {
      auto next = counting.pop_back_val();
      if (next.depth > 128 || !charge(budget, branch))
        return failure();
      if (isa<ac::BitsType>(next.type)) {
        expectedCount = SourceCheckBudget::add(expectedCount, 1);
      } else if (isa<ac::EnumType>(next.type)) {
        expectedCount = SourceCheckBudget::add(expectedCount, 4);
      } else if (auto table = dyn_cast<ac::TableType>(next.type)) {
        auto count = hardware.getTableSize(table, bindings, branch);
        if (failed(count) || !product(budget, branch, *count, sizeof(CountFrame)))
          return failure();
        expectedCount = SourceCheckBudget::add(expectedCount,
                                               SourceCheckBudget::add(1, 3 * *count));
        for (uint64_t i = 0; i < *count; ++i) {
          if (!prepayAppend(budget, branch, counting)) return failure();
          counting.push_back({table.getElementType(), next.depth + 1});
        }
      } else if (auto structure = dyn_cast<ac::StructType>(next.type)) {
        auto fields = hardware.lookupStruct(structure).getFields();
        expectedCount = SourceCheckBudget::add(
            expectedCount,
            SourceCheckBudget::add(
                1, SourceCheckBudget::multiply(fields.size(), 2)));
        for (Attribute raw : fields) {
          if (!charge(budget, branch, 2)) return failure();
          auto field = cast<DictionaryAttr>(raw);
          Type declared = field.getAs<TypeAttr>("type").getValue();
          // Reuse only this proof's independently admitted nominal closure.
          FailureOr<Type> resolved = failure();
          if (isa<ac::StructType, ac::EnumType>(declared))
            resolved = declared;
          else if (isa<ac::BitsType>(declared))
            resolved = admittedBranchField(declared, hardware, bindings, branch, budget, &admissionCache);
          else
            return reject("original local branch field type is unsupported");
          if (failed(resolved) || !prepayAppend(budget, branch, counting))
            return failure();
          counting.push_back({*resolved, next.depth + 1});
        }
      } else {
        return reject("original local branch type is unsupported");
      }
    }
    if (record.generated.size() != expectedCount ||
        !charge(budget, branch, SourceCheckBudget::multiply(expectedCount, 8)))
      return reject("original local branch operation inventory differs");

    struct Frame {
      Type type;
      Value whenTrue, whenFalse;
      ArrayAttr fields;
      uint64_t next = 0;
      uint64_t tableSize = 0;
      SmallVector<Value, 0> joinedFields;
    };
    SmallVector<Frame, 0> stack;
    if (!charge(budget, branch, SourceCheckBudget::bytes(129 * sizeof(Frame))))
      return failure();
    stack.reserve(129);
    auto push = [&](Type type, Value t, Value f) -> LogicalResult {
      if (stack.size() >= 129 || !charge(budget, branch, 4))
        return failure();
      Frame frame{type, t, f, {}, 0, 0, {}};
      if (auto structure = dyn_cast<ac::StructType>(type)) {
        frame.fields = hardware.lookupStruct(structure).getFields();
        uint64_t size = frame.fields.size();
        if (size > frame.joinedFields.max_size() ||
            !charge(budget, branch,
                    SourceCheckBudget::add(
                        size,
                        SourceCheckBudget::bytes(
                            SourceCheckBudget::multiply(size, sizeof(Value))))))
          return failure();
        frame.joinedFields.reserve(size);
      }
      if (auto table = dyn_cast<ac::TableType>(type)) {
        auto count = hardware.getTableSize(table, bindings, branch);
        if (failed(count) || !product(budget, branch, *count, sizeof(Value)))
          return failure();
        frame.tableSize = *count;
        frame.joinedFields.reserve(*count);
      }
      stack.push_back(std::move(frame));
      return success();
    };
    Builder b(branch->getContext());
    uint64_t cursor = 0;
    auto take = [&](StringRef name, ArrayRef<Value> operands,
                    ArrayRef<Type> types,
                    DictionaryAttr attributes) -> FailureOr<Value> {
      if (cursor == record.generated.size() || !charge(budget, branch, 4))
        return failure();
      Operation *op = record.generated[cursor++];
      // A locator can be erased or stale. Membership precedes every access.
      if (!op || !operations.contains(op))
        return reject("original local branch operation locator is not live");
      for (Value operand : operands)
        if (!values.contains(operand))
          return reject("original local branch operand locator is not live");
      if (failed(generatedProof(op, name, operands, types, attributes,
                                branch->getLoc(), parent)) ||
          (last && !last->isBeforeInBlock(op)))
        return reject("original local branch operation order differs");
      last = op;
      Value result = op->getResult(0);
      if (!values.contains(result))
        return reject("original local branch result locator is not live");
      return result;
    };
    // Construct expected literal widths directly from the original declaration
    // and source span; candidate width metadata never supplies the authority.
    auto widthType = [&](uint64_t width) -> Type {
      auto number = ac::MathIntAttr::get(
          branch->getContext(), llvm::APSInt(llvm::APInt(64, width), true));
      auto expression = b.getDictionaryAttr(
          {b.getNamedAttr("kind", b.getStringAttr("literal")),
           b.getNamedAttr(
               "value", b.getDictionaryAttr(
                            {b.getNamedAttr("kind", b.getStringAttr("integer")),
                             b.getNamedAttr("value", number)})),
           b.getNamedAttr("origin", branch->getAttr("occurrence")),
           b.getNamedAttr("location", selectionSourceLocation(branch))});
      return ac::BitsType::get(
          branch->getContext(),
          ac::StaticExprAttr::get(branch->getContext(), expression));
    };
    if (failed(push(resolvedRoot, whenTrue, whenFalse)))
      return failure();
    Value final;
    while (!stack.empty()) {
      Frame &frame = stack.back();
      if (auto table = dyn_cast<ac::TableType>(frame.type);
          table && frame.next < frame.tableSize) {
        auto number = ac::MathIntAttr::get(branch->getContext(),
            llvm::APSInt(llvm::APInt(64, frame.next++), true));
        auto expression = b.getDictionaryAttr({
            b.getNamedAttr("kind", b.getStringAttr("literal")),
            b.getNamedAttr("value", b.getDictionaryAttr({
                b.getNamedAttr("kind", b.getStringAttr("integer")),
                b.getNamedAttr("value", number)})),
            b.getNamedAttr("origin", branch->getAttrOfType<DictionaryAttr>("occurrence")),
            b.getNamedAttr("location", selectionSourceLocation(branch))});
        auto index = take(ac::BitsConstantOp::getOperationName(), {}, {widthType(64)},
            b.getDictionaryAttr({b.getNamedAttr("value", ac::StaticExprAttr::get(
                branch->getContext(), expression))}));
        if (failed(index)) return failure();
        Type element = table.getElementType();
        auto trueElement = take(ac::TableGetOp::getOperationName(),
            {frame.whenTrue, *index}, {element, widthType(1)}, b.getDictionaryAttr({}));
        auto falseElement = take(ac::TableGetOp::getOperationName(),
            {frame.whenFalse, *index}, {element, widthType(1)}, b.getDictionaryAttr({}));
        if (failed(trueElement) || failed(falseElement) ||
            !record.generated[cursor - 1]->getResult(1).use_empty() ||
            !record.generated[cursor - 2]->getResult(1).use_empty() ||
            failed(push(element, *trueElement, *falseElement))) return failure();
        continue;
      }
      if (frame.fields && frame.next != frame.fields.size()) {
        if (!charge(budget, branch, 2)) return failure();
        auto field = cast<DictionaryAttr>(frame.fields[frame.next++]);
        Type declared = field.getAs<TypeAttr>("type").getValue();
        // The original declarations and bindings have not changed since root
        // admission. Authenticate every generated field operation below.
        FailureOr<Type> fieldType = failure();
        if (isa<ac::StructType, ac::EnumType>(declared))
          fieldType = declared;
        else if (isa<ac::BitsType>(declared))
          fieldType = admittedBranchField(declared, hardware, bindings, branch, budget, &admissionCache);
        else
          return reject("original local branch field type is unsupported");
        if (failed(fieldType))
          return failure();
        auto attributes = b.getDictionaryAttr(
            {b.getNamedAttr("field", field.getAs<StringAttr>("name"))});
        auto t = take(ac::StructGetOp::getOperationName(), {frame.whenTrue},
                      {*fieldType}, attributes);
        auto f = take(ac::StructGetOp::getOperationName(), {frame.whenFalse},
                      {*fieldType}, attributes);
        if (failed(t) || failed(f) || failed(push(*fieldType, *t, *f)))
          return failure();
        continue;
      }
      FailureOr<Value> joined = failure();
      if (isa<ac::BitsType>(frame.type)) {
        joined = take(ac::BitsSelectOp::getOperationName(),
                      {condition, frame.whenTrue, frame.whenFalse},
                      {frame.type}, b.getDictionaryAttr({}));
      } else if (auto enumeration = dyn_cast<ac::EnumType>(frame.type)) {
        auto declaration = hardware.lookupEnum(enumeration);
        auto width = declaration.getWidthAttr().getCanonicalValue();
        if (!charge(budget, branch,
                    SourceCheckBudget::add(
                        64, SourceCheckBudget::bytes(width.size()))))
          return failure();
        Type carrier = widthType(llvm::APSInt(width).getZExtValue());
        Type membership = widthType(1);
        auto t = take(ac::EnumToBitsOp::getOperationName(), {frame.whenTrue},
                      {carrier}, b.getDictionaryAttr({}));
        auto f = take(ac::EnumToBitsOp::getOperationName(), {frame.whenFalse},
                      {carrier}, b.getDictionaryAttr({}));
        if (failed(t) || failed(f))
          return failure();
        auto selected =
            take(ac::BitsSelectOp::getOperationName(), {condition, *t, *f},
                 {carrier}, b.getDictionaryAttr({}));
        if (failed(selected))
          return failure();
        joined = take(ac::EnumFromBitsOp::getOperationName(), {*selected},
                      {frame.type, membership}, b.getDictionaryAttr({}));
        if (failed(joined))
          return failure();
        // Both results are inventoried, even for a dead aggregate join. A dead
        // consumer still violates the deliberately unused membership result.
        Operation *conversion = record.generated[cursor - 1];
        Value member = conversion->getResult(1);
        if (!values.contains(member) || !member.use_empty())
          return reject("original local Enum join membership result is used");
      } else {
        joined =
            take(isa<ac::TableType>(frame.type) ? ac::TableCreateOp::getOperationName()
                                                   : ac::StructCreateOp::getOperationName(), frame.joinedFields,
                 {frame.type}, b.getDictionaryAttr({}));
      }
      if (failed(joined))
        return failure();
      final = *joined;
      stack.pop_back();
      if (!stack.empty())
        stack.back().joinedFields.push_back(final);
    }
    if (cursor != record.generated.size() || final != record.value ||
        !values.contains(final) || final.getType() != resolvedRoot)
      return reject("original local branch final result differs");
    return success();
  }

  LogicalResult literalProof(Value value, Type type, DictionaryAttr occurrence,
                             Location location, bool one, Block *parent) {
    if (!charge(budget, prepared.original, 16))
      return failure();
    auto result = dyn_cast<OpResult>(value);
    if (!result || result.getResultNumber() != 0)
      return reject("original generated literal result is absent");
    Builder b(prepared.original->getContext());
    return generatedProof(
        result.getOwner(), ac::BitsConstantOp::getOperationName(), {}, {type},
        b.getDictionaryAttr({b.getNamedAttr(
            "value",
            bitsLiteralAttribute(type, occurrence, b.getContext(), one))}),
        location, parent);
  }

#include "TransactionDomainCommitProof.inc"

  LogicalResult candidatePairProof(ac::ModuleOp originalModule,
                                   ArrayRef<ac::InstanceOp> path, Value state,
                                   Value data, std::optional<Value> enable,
                                   Type dataType, Type enableType,
                                   DictionaryAttr occurrence, Location location) {
    if (!charge(budget, originalModule, 16 + 8 * path.size()))
      return failure();
    const auto &definition =
        prepared.definitions[prepared.definitionIndex.lookup(
            originalModule.operator->())];
    auto module = cast<ac::ModuleOp>(
        candidate.packageMap.lookup(originalModule.operator->()));
    auto key = routingKey(path, state);
    using WriterSlot = std::pair<uint64_t, uint64_t>;
    SmallVector<WriterSlot, 0> matching;
    for (uint64_t index : prepared.transactionSchedule) {
      const auto &transaction = prepared.transactions[index];
      const auto &plan = *transaction.plan;
      if (plan.lexicalModule != originalModule) continue;
      for (auto [ordinal, output] : llvm::enumerate(transaction.outputs)) {
        if (!charge(budget, originalModule)) return failure();
        const auto &owner = plan.owners[output.binding.owner];
        if (routingKey(anchorPath(plan, owner.anchor), owner.declaration->getResult(0)) != key) continue;
        if (!prepayAppend(budget, originalModule, matching)) return failure();
        matching.push_back({index, ordinal});
      }
    }
    // Reconstruct contributors from actual source allocations along this owner
    // route. Definition pointers alone do not identify registration scope.
    struct ScopedWriter {
      const TransactionPreparation *transaction;
      ac::StateOwnerId owner;
      SmallVector<ac::InstanceOp> scope;
    };
    SmallVector<ScopedWriter, 0> scoped;
    SmallVector<ac::InstanceOp> prefix;
    ac::ModuleOp lexical = originalModule;
    for (uint64_t depth = 0; depth <= path.size(); ++depth) {
      if (!charge(budget, originalModule, 4 + prefix.size())) return failure();
      for (const auto &transaction : prepared.transactions) {
        const auto &plan = *transaction.plan;
        if (!charge(budget, originalModule)) return failure();
        if (plan.lexicalModule != lexical) continue;
        for (const auto &output : transaction.outputs) {
          const auto &owner = plan.owners[output.binding.owner];
          auto suffix = anchorPath(plan, owner.anchor);
          if (!product(budget, originalModule, prefix.size() + suffix.size(),
                       1 + SourceCheckBudget::bytes(sizeof(ac::InstanceOp)))) return failure();
          SmallVector<ac::InstanceOp> ownerPath(prefix);
          llvm::append_range(ownerPath, suffix);
          if (routingKey(ownerPath, owner.declaration->getResult(0)) != key) continue;
          if (!prepayAppend(budget, originalModule, scoped)) return failure();
          if (!product(budget, originalModule, prefix.size(),
                       1 + SourceCheckBudget::bytes(sizeof(ac::InstanceOp)))) return failure();
          scoped.push_back({&transaction, output.binding.owner, prefix});
        }
      }
      if (depth == path.size()) break;
      auto child = dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(path[depth]));
      if (!child || path[depth]->getParentOfType<ac::ModuleOp>() != lexical)
        return reject("original contributor owner path is not an actual allocation");
      if (!prepayAppend(budget, originalModule, prefix)) return failure();
      prefix.push_back(path[depth]);
      lexical = child;
    }
    bool scopedOrdered = false, scopedUnordered = false;
    std::unique_ptr<SourceWriteGrantProof> scopedGrant;
    for (size_t i = 0; i < scoped.size(); ++i)
      for (size_t j = 0; j < i; ++j) {
        const auto &first = scoped[i], &second = scoped[j];
        const auto &a = *first.transaction->plan, &b = *second.transaction->plan;
        auto ap = originalPredecessors(a.transaction), bp = originalPredecessors(b.transaction);
        if (failed(ap) || failed(bp)) return failure();
        bool sameScope = first.scope == second.scope;
        bool pairOrdered = sameScope &&
            (llvm::is_contained(*ap, b.transaction) || llvm::is_contained(*bp, a.transaction));
        scopedOrdered |= pairOrdered; scopedUnordered |= !pairOrdered;
        if (pairOrdered) continue;
        if (!scopedGrant) {
          if (!charge(budget, originalModule, 1 + SourceCheckBudget::bytes(sizeof(SourceWriteGrantProof)))) return failure();
          scopedGrant = std::make_unique<SourceWriteGrantProof>(originalModule, hardware, budget);
        }
        for (auto [wa, left] : llvm::enumerate(a.writes)) {
          if (!charge(budget, originalModule)) return failure();
          if (left.owner != first.owner || left.forwardedProposal) continue;
          for (auto [wb, right] : llvm::enumerate(b.writes)) {
            if (!charge(budget, originalModule)) return failure();
            if (right.owner != second.owner || right.forwardedProposal) continue;
            auto disjoint = scopedGrant->disjoint(a, first.scope, wa, b, second.scope, wb);
            if (failed(disjoint) || !*disjoint)
              return reject("original scoped writers are not independently disjoint");
          }
        }
      }
    if (scopedOrdered && scopedUnordered)
      return reject("original mixed scoped ordered/unordered writer group is unsupported");
    bool unordered = false, ordered = false;
    // The scoped proof above includes every local matching writer at depth
    // zero with an empty prefix. Reuse that proof, retaining the separate local
    // ordering classification needed to validate the actual commit below.
    for (size_t i = 0; i < matching.size(); ++i)
      for (size_t j = 0; j < i; ++j) {
        if (!charge(budget, originalModule, 4)) return failure();
        auto [ai, ao] = matching[i]; auto [bi, bo] = matching[j];
        const auto &a = *prepared.transactions[ai].plan, &b = *prepared.transactions[bi].plan;
        auto ap = originalPredecessors(a.transaction), bp = originalPredecessors(b.transaction);
        if (failed(ap) || failed(bp)) return failure();
        bool pairOrdered = llvm::is_contained(*ap, b.transaction) || llvm::is_contained(*bp, a.transaction);
        ordered |= pairOrdered; unordered |= !pairOrdered;
      }
    if (ordered && unordered) return reject("original mixed ordered/unordered writer group is unsupported");
    Value expectedData, expectedEnable;
    auto incoming = definition.downwardIndex.find(key);
    if (incoming != definition.downwardIndex.end()) {
      const detail::MaterializedIncomingCommit *closed = nullptr;
      for (const auto &entry : candidate.incomingCommits) {
        if (!charge(budget, originalModule)) return failure();
        if (entry.module != originalModule || entry.key != key) continue;
        if (closed) return reject("original incoming closure is duplicated");
        closed = &entry;
      }
      if (!closed) return reject("original incoming closure is missing");
      unsigned input = definition.sourceInputs + 2 * incoming->second;
      Value rawData = module.getBody().front().getArgument(input);
      expectedEnable = module.getBody().front().getArgument(input + 1);
      auto oldQ = routed(originalModule, path, state);
      if (failed(oldQ)) return failure();
      ac::HardwareBindings bindings;
      bindings.owner = originalModule;
      auto declaration = state.getDefiningOp<ac::SourceStateOp>();
      Operation *last = nullptr;
      if (failed(domainCommitProof(declaration, declaration.getOccurrence(), dataType,
          bindings, rawData, expectedEnable, {}, closed->data, *oldQ,
          &module.getBody().front(), last))) return failure();
      expectedData = closed->data.value;
    }
    for (auto [index, ordinal] : matching) {
      const auto &transaction = prepared.transactions[index];
      const auto &plan = *transaction.plan;
      const auto &output = transaction.outputs[ordinal];
      const auto &owner = plan.owners[output.binding.owner];
      const auto &record = candidate.transactions[index];
      auto rule = record.rule;
      Value currentData = rule->getResult(output.dataResult);
      Value currentEnable = rule->getResult(output.enableResult);
      const auto &commit = record.commits[ordinal];
      bool crossScopeDomains = !path.empty() && llvm::any_of(
          transaction.writeDomains,
          [&](const auto &domain) { return domain.candidate == ordinal; });
      if (unordered || incoming != definition.downwardIndex.end() ||
          (!expectedData && crossScopeDomains)) {
        Value base = expectedData;
        if (!base) { auto oldQ = routed(originalModule, path, state); if (failed(oldQ)) return failure(); base = *oldQ; }
        Operation *last = rule;
        if (failed(domainCommitProof(transaction, ordinal, record, base, &module.getBody().front(), last))) return failure();
        if (!expectedEnable) {
          if (commit.enable != currentEnable || !commit.enableGenerated.empty())
            return reject("original first unordered enable differs from its actual writer");
          expectedEnable = currentEnable;
        } else {
          if (commit.enableGenerated.size() != 5) return reject("original unordered poison enable inventory differs");
          unsigned cursor = 0;
          Builder b(originalModule.getContext());
          auto gate = [&](Value first, Value second, StringRef opcode) -> FailureOr<Value> {
            Operation *operation = commit.enableGenerated[cursor++];
            if (failed(generatedProof(operation, ac::BitsBinaryOp::getOperationName(), {first, second},
                {first.getType()}, b.getDictionaryAttr({b.getNamedAttr("opcode", b.getStringAttr(opcode))}),
                plan.transaction->getLoc(), &module.getBody().front())) ||
                (last && !last->isBeforeInBlock(operation))) return failure();
            last = operation; return operation->getResult(0);
          };
          auto active = gate(expectedEnable, currentEnable, "or");
          auto priorPoison = gate(expectedEnable, expectedEnable, "xor");
          auto currentPoison = gate(currentEnable, currentEnable, "xor");
          if (failed(active) || failed(priorPoison) || failed(currentPoison)) return failure();
          auto poison = gate(*priorPoison, *currentPoison, "or"); if (failed(poison)) return failure();
          auto combined = gate(*active, *poison, "xor");
          if (failed(combined) || commit.enable != *combined) return reject("original unordered poison enable differs");
          expectedEnable = *combined;
        }
        expectedData = commit.data.value;
        continue;
      }
      if (commit.domainMerge || !commit.enableGenerated.empty()) return reject("original ordered owner has an invented domain merge");
      if (!expectedData) {
        if (commit.data.value || commit.enable || !commit.data.generated.empty())
          return reject("original first candidate has an invented commit chain");
        expectedData = currentData; expectedEnable = currentEnable; continue;
      }
      Operation *last = nullptr;
      if (failed(localBranchProof(plan.transaction, owner.payloadType, plan.frames[0].bindings,
            currentEnable, currentData, expectedData, commit.data, &module.getBody().front(), last))) return failure();
      Builder b(originalModule.getContext());
      if (!commit.enable || !values.contains(commit.enable) ||
          failed(generatedProof(commit.enable.getDefiningOp(), ac::BitsBinaryOp::getOperationName(),
              {expectedEnable, currentEnable}, {currentEnable.getType()},
              b.getDictionaryAttr({b.getNamedAttr("opcode", b.getStringAttr("or"))}),
              plan.transaction->getLoc(), &module.getBody().front())))
        return reject("original ordered commit enable differs");
      expectedData = commit.data.value; expectedEnable = commit.enable;
    }
    if (expectedData) {
      if (data != expectedData)
        return reject("original ordered final candidate data differs");
      if (!enable) {
        // A plain DFF has no physical enable pin. Recompute the composed
        // commit enable independently and require an exact constant-one truth,
        // not the materializer's own record.
        if (!expectedEnable || !knownTrue(expectedEnable, hardware, budget))
          return reject("original DFF storage lacks a constant-true commit enable");
        return success();
      }
      if (*enable != expectedEnable)
        return reject("original ordered final candidate routing enable differs");
      return success();
    }
    if (path.empty()) {
      if (data != candidate.packageMap.lookupOrNull(state))
        return reject("original unwritten state hold data differs");
    } else if (isa<ac::BitsType>(dataType)) {
      if (failed(literalProof(data, dataType, occurrence, location, false,
                              &module.getBody().front()))) return failure();
    } else {
      auto oldQ = routed(originalModule, path, state);
      if (failed(oldQ) || data != *oldQ)
        return reject("original unused external route must preserve typed Q");
    }
    if (!enable)
      return reject("original unwritten DFF state has no local writer");
    return literalProof(*enable, enableType, occurrence, location, false,
                        &module.getBody().front());
  }

  LogicalResult routingProof() {
    uint64_t incomingCount = 0;
    for (const auto &definition : prepared.definitions) {
      if (!charge(budget, definition.module)) return failure();
      incomingCount = SourceCheckBudget::add(incomingCount, definition.downward.size());
    }
    if (candidate.incomingCommits.size() != incomingCount)
      return reject("original incoming closure inventory differs");
    for (const auto &entry : candidate.incomingCommits) {
      if (!charge(budget, prepared.original)) return failure();
      auto found = prepared.definitionIndex.find(entry.module.operator->());
      if (found == prepared.definitionIndex.end() ||
          !prepared.definitions[found->second].downwardIndex.contains(entry.key))
        return reject("original incoming closure has no source owner route");
    }
    for (auto state : prepared.states) {
      auto module = state->getParentOfType<ac::ModuleOp>();
      auto *storage = candidate.packageMap.lookup(state.operator->());
      bool dff = prepared.dffStates.contains(state.operator->());
      std::optional<Value> enable;
      if (dff)
        enable = storage->getOperand(2);
      else
        enable = isa<ac::CollectionOp>(storage)
                      ? cast<ac::TableSplatOp>(
                            storage->getOperand(2).getDefiningOp())
                            .getInput()
                      : storage->getOperand(2);
      Value data = dff ? storage->getOperand(2) : storage->getOperand(3);
      if (dff)
        enable = std::nullopt;
      if (failed(candidatePairProof(
              module, {}, state.getQ(), data, enable, state.getQ().getType(),
              state.getClk().getType(), state.getOccurrence(), state.getLoc())))
        return failure();
    }
    for (const auto &parent : prepared.definitions) {
      auto originalModule = parent.module;
      for (auto original :
           originalModule.getBody().front().getOps<ac::InstanceOp>()) {
        auto child =
            dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(original));
        if (!child)
          continue;
        const auto &definition =
            prepared.definitions[prepared.definitionIndex.lookup(
                child.operator->())];
        if (definition.downward.empty())
          continue;
        auto bindings = hardware.bindInstance(original);
        if (failed(bindings))
          return failure();
        auto actual = cast<ac::InstanceOp>(
            candidate.packageMap.lookup(original.operator->()));
        for (auto [ordinal, route] : llvm::enumerate(definition.downward)) {
          auto dType = hardware.resolveType(route.type, *bindings, original);
          auto eType =
              hardware.resolveType(route.enableType, *bindings, original);
          if (failed(dType) || failed(eType))
            return failure();
          if (!charge(budget, original, 8 + 8 * route.path.size()))
            return failure();
          SmallVector<ac::InstanceOp> path{original};
          llvm::append_range(path, route.path);
          unsigned input = definition.sourceInputs + 2 * ordinal;
          if (failed(candidatePairProof(
                  originalModule, path, route.value, actual->getOperand(input),
                  actual->getOperand(input + 1), *dType, *eType,
                  original.getOccurrence(), original.getLoc())))
            return failure();
        }
      }
    }
    return success();
  }

  /// Names, properties, all attributes, result ordinals, operands and complete
  /// nested block/region shape are compared. A map is only an object locator.
  LogicalResult exactClone(Operation *source, Operation *target,
                           IRMapping &authenticated,
                           const IRMapping &locators) {
    if (!charge(
            budget, source,
            16 + 4 * source->getNumOperands() + 4 * source->getNumResults() +
                source->getRawDictionaryAttrs().size() +
                SourceCheckBudget::bytes(source->getPropertiesStorageSize())))
      return failure();
    if (!target || !operations.contains(target) ||
        source->getName() != target->getName() ||
        source->getAttrs() != target->getAttrs() ||
        source->getPropertiesAsAttribute() !=
            target->getPropertiesAsAttribute() ||
        source->getLoc() != target->getLoc() ||
        source->getNumOperands() != target->getNumOperands() ||
        source->getResultTypes() != target->getResultTypes() ||
        source->getNumRegions() != target->getNumRegions() ||
        !clones.insert(target).second)
      return reject(
          "actual clone opcode/attributes/properties/type/identity differs");
    for (auto [original, actual] :
         llvm::zip(source->getOperands(), target->getOperands())) {
      auto expected = authenticated.lookupOrNull(original);
      if (!expected || actual != expected)
        return reject(
            "actual clone operand differs from authenticated original SSA");
    }
    for (auto [original, actual] :
         llvm::zip(source->getResults(), target->getResults())) {
      if (locators.lookupOrNull(original) != actual)
        return reject("clone result mapping has the wrong original ordinal");
      authenticated.map(original, actual);
    }
    for (auto [sourceRegion, targetRegion] :
         llvm::zip(source->getRegions(), target->getRegions())) {
      if (sourceRegion.getBlocks().size() != targetRegion.getBlocks().size())
        return reject("clone nested region block shape differs");
      for (auto [sourceBlock, targetBlock] :
           llvm::zip(sourceRegion, targetRegion)) {
        if (!charge(budget, source, 8 + 4 * sourceBlock.getNumArguments()))
          return failure();
        if (locators.lookupOrNull(&sourceBlock) != &targetBlock ||
            sourceBlock.getArgumentTypes() != targetBlock.getArgumentTypes() ||
            sourceBlock.getOperations().size() !=
                targetBlock.getOperations().size())
          return reject("clone block/argument/operation shape differs");
        for (auto [original, actual] : llvm::zip(sourceBlock.getArguments(),
                                                 targetBlock.getArguments())) {
          if (locators.lookupOrNull(original) != actual)
            return reject("clone nested block argument ordinal differs");
          authenticated.map(original, actual);
        }
        for (auto [original, actual] : llvm::zip(sourceBlock.getOperations(),
                                                 targetBlock.getOperations())) {
          if (locators.lookupOrNull(&original) != &actual ||
              failed(exactClone(&original, &actual, authenticated, locators)))
            return failure();
        }
      }
    }
    return success();
  }

  FailureOr<Value> routed(ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                          Value source) {
    if (path.empty()) {
      auto value = candidate.packageMap.lookupOrNull(source);
      if (!value || !values.contains(value))
        return reject("original lexical value mapping is absent");
      return value;
    }
    if (!product(budget, module, path.size(), 8))
      return failure();
    auto child =
        dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(path.front()));
    if (!child)
      return reject("original lexical route is not an ordinary instance");
    const auto &definition =
        prepared
            .definitions[prepared.definitionIndex.lookup(child.operator->())];
    auto found =
        definition.upwardIndex.find(routingKey(path.drop_front(), source));
    auto allocation = dyn_cast_or_null<ac::InstanceOp>(
        candidate.packageMap.lookupOrNull(path.front().operator->()));
    if (found == definition.upwardIndex.end() || !allocation ||
        !operations.contains(allocation.operator->()) ||
        definition.sourceOutputs + found->second >= allocation->getNumResults())
      return reject("original lexical upward route is absent");
    return allocation->getResult(definition.sourceOutputs + found->second);
  }

  uint64_t queueInputCount(const DefinitionRouting &definition) const {
    uint64_t count = 0;
    for (const auto &route : definition.queueIncoming) count += route.push ? 2 : 1;
    return count;
  }

  LogicalResult definitionProof(const DefinitionRouting &definition) {
    auto original = definition.module;
    auto module = dyn_cast_or_null<ac::ModuleOp>(
        candidate.packageMap.lookupOrNull(original.operator->()));
    if (!module || !operations.contains(module.operator->()) ||
        module->getParentOp() != (*candidate.module).operator->() ||
        module->getNumRegions() != 1 || !module.getBody().hasOneBlock())
      return reject("original definition owner differs");
    if (failed(cover(module)))
      return failure();
    if (module->getAttr("ac.transaction_wiring") != definition.expectedWiring)
      return reject(
          "exact original affected-definition companion inventory differs");
    auto sourceType = original.getFunctionType(),
         targetType = module.getFunctionType();
    auto availability = ac::getModulePortLayout(module);
    if (failed(availability) || availability->sourceInputCount != definition.sourceInputs ||
        availability->sourceOutputCount != definition.sourceOutputs) return failure();
    uint64_t validityInputs = llvm::count_if(availability->inputValidity,
                                           [](int64_t i) { return i >= 0; });
    uint64_t validityOutputs = llvm::count_if(availability->outputValidity,
                                            [](int64_t i) { return i >= 0; });
    validityOutputs += availability->routedOutputValidity.size();
    if (!prepared.valueAvailability && (validityInputs || validityOutputs))
      return reject("original total package has invented validity ports");
    uint64_t routedBase = definition.sourceOutputs + definition.upward.size();
    uint64_t availabilityBase = routedBase + definition.queuePopSummaries.size();
    uint64_t publicValidityCount = llvm::count_if(availability->outputValidity,
                                                 [](int64_t i) { return i >= 0; });
    for (auto [ordinal, pair] : llvm::enumerate(availability->routedOutputValidity))
      if (pair.first < definition.sourceOutputs || pair.first >= routedBase ||
          pair.second != availabilityBase + publicValidityCount + ordinal)
        return reject("original routed capture validity does not follow its exported payload");
    if (targetType.getNumInputs() !=
            definition.sourceInputs + 2 * definition.downward.size() + queueInputCount(definition) + 2 * definition.memoryIncoming.size() + validityInputs ||
        targetType.getNumResults() !=
            definition.sourceOutputs + definition.upward.size() + definition.queuePopSummaries.size() + validityOutputs ||
        targetType.getInputs().take_front(definition.sourceInputs) !=
            sourceType.getInputs() ||
        targetType.getResults().take_front(definition.sourceOutputs) !=
            sourceType.getResults() ||
        module.getBody().front().getNumArguments() !=
            targetType.getNumInputs() ||
        module.getInputNames().getValue().take_front(definition.sourceInputs) !=
            original.getInputNames().getValue() ||
        module.getOutputNames().getValue().take_front(
            definition.sourceOutputs) != original.getOutputNames().getValue())
      return reject("original module port prefix/route arity differs");
    for (auto [ordinal, route] : llvm::enumerate(definition.downward))
      if (targetType.getInput(definition.sourceInputs + 2 * ordinal) !=
              route.type ||
          targetType.getInput(definition.sourceInputs + 2 * ordinal + 1) !=
              route.enableType)
        return reject("original appended incoming route types differ");
    for (auto [ordinal, route] : llvm::enumerate(definition.memoryIncoming)) {
      uint64_t input = definition.sourceInputs + 2 * definition.downward.size() + queueInputCount(definition) + 2 * ordinal;
      if (targetType.getInput(input) != route.predicateType || targetType.getInput(input + 1) != route.type)
        return reject("original appended Memory pin route types differ");
    }
    for (auto [ordinal, route] : llvm::enumerate(definition.upward))
      if (targetType.getResult(definition.sourceOutputs + ordinal) !=
          route.type)
        return reject("original appended upward route type differs");
    for (auto [ordinal, route] : llvm::enumerate(definition.queuePopSummaries))
      if (targetType.getResult(definition.sourceOutputs + definition.upward.size() + ordinal) !=
          route.predicateType)
        return reject("original appended queue summary type differs");
    if (definition.upward.empty() && definition.downward.empty() && definition.queueIncoming.empty() &&
        definition.memoryIncoming.empty() && definition.queuePopSummaries.empty() &&
        !validityInputs && !validityOutputs) {
      if (module->getAttr("ac.internal_ports") !=
          original->getAttr("ac.internal_ports"))
        return reject("original unextended internal port metadata differs");
    } else {
      Builder b(original.getContext());
      SmallVector<Attribute> defaults(2 * definition.downward.size() + queueInputCount(definition) + 2 * definition.memoryIncoming.size(),
                                      b.getI64IntegerAttr(0));
      defaults.append(validityInputs, b.getI64IntegerAttr(1));
      auto internal = b.getDictionaryAttr(
          {b.getNamedAttr("source_inputs",
                          b.getI64IntegerAttr(definition.sourceInputs)),
           b.getNamedAttr("source_outputs",
                          b.getI64IntegerAttr(definition.sourceOutputs)),
           b.getNamedAttr("root_inputs", b.getArrayAttr(defaults))});
      if (module->getAttr("ac.internal_ports") != internal)
        return reject("original appended route/internal port metadata differs");
    }
    NamedAttrList expected(original->getAttrs()), actual(module->getAttrs());
    for (StringRef name : {"function_type", "input_names", "output_names",
                           "ac.internal_ports", "ac.transaction_wiring", "ac.queue_wiring",
                           "ac.input_validity", "ac.output_validity",
                           "ac.routed_output_validity"}) {
      expected.erase(name);
      actual.erase(name);
    }
    if (expected.getDictionary(original.getContext()) !=
        actual.getDictionary(original.getContext()))
      return reject("original module identity/domain/attributes differ");
    for (auto [source, target] :
         llvm::zip(original.getBody().front().getArguments(),
                   module.getBody().front().getArguments().take_front(
                       definition.sourceInputs)))
      if (candidate.packageMap.lookupOrNull(source) != target)
        return reject("original module argument ordinal differs");
    IRMapping authenticated;
    for (auto argument : original.getBody().front().getArguments())
      authenticated.map(argument, candidate.packageMap.lookup(argument));
    // Module graph regions permit forward dependencies. Authenticate every
    // direct result locator before checking operands; this does not certify
    // operation semantics, which the complete second pass still proves.
    for (Operation &op : original.getBody().front()) {
      if (!charge(budget, &op, 1))
        return failure();
      if (isa<ac::SourceMethodOp>(op))
        continue;
      auto target = candidate.packageMap.lookupOrNull(&op);
      if (!target || !operations.contains(target) ||
          target->getBlock() != &module.getBody().front() ||
          target->getNumResults() < op.getNumResults())
        return reject("original module graph operation/result locator differs");
      // The package inventory already charged for every live result entry and
      // inserted it in `values`. This pass adds one authenticated comparison
      // and map link per result, not a second allocation of the same entry.
      if (!product(budget, &op, op.getNumResults(), 1))
        return failure();
      for (auto [ordinal, result] : llvm::enumerate(op.getResults())) {
        Value actualResult = target->getResult(ordinal);
        Value prior = authenticated.lookupOrNull(result);
        if (actualResult.getType() != result.getType() ||
            candidate.packageMap.lookupOrNull(result) != actualResult ||
            (prior && prior != actualResult))
          return reject("original module graph result ordinal/type differs");
        authenticated.map(result, actualResult);
      }
    }
    // All module-level computations and ordinary instance prefixes are checked,
    // including lexical roots subsequently captured by invoked Methods.
    for (Operation &op : original.getBody().front()) {
      if (isa<ac::SourceMethodOp, ac::SourceTransactionOp>(op)) {
        if (auto transaction = dyn_cast<ac::SourceTransactionOp>(op))
          for (Value result : transaction->getResults())
            authenticated.map(result, candidate.packageMap.lookup(result));
        continue;
      }
      auto target = candidate.packageMap.lookupOrNull(&op);
      if (!target || !operations.contains(target) ||
          target->getBlock() != &module.getBody().front())
        return reject("original module-body operation owner differs");
      if (auto state = dyn_cast<ac::SourceStateOp>(op)) {
        auto *storage = target;
        auto table = dyn_cast<ac::TableType>(state.getQ().getType());
        Type element = table ? table.getElementType() : state.getQ().getType();
        if ((table && !isa<ac::CollectionOp>(storage)) ||
            (!table && !isa<ac::InstanceOp>(storage)))
          return reject("original typed state allocation kind differs");
        bool dff = prepared.dffStates.contains(state.getOperation());
        unsigned arity = dff ? 4 : 5;
        if (storage->getNumOperands() != arity || storage->getNumResults() != 1)
          return reject("original typed state storage arity differs");
        SmallVector<Value> scalarControls;
        if (table) {
          for (unsigned i : {0u, 1u, 2u}) {
            auto splat = storage->getOperand(i).getDefiningOp<ac::TableSplatOp>();
            if (!splat || !operations.contains(splat) ||
                splat.getShape() != table.getShape())
              return reject("original Table state requires replicated controls");
            scalarControls.push_back(splat.getInput());
            Builder b(original.getContext());
            Type expected = ac::TableType::get(original.getContext(), table.getShape(),
                                               splat.getInput().getType());
            if (failed(generatedProof(splat, ac::TableSplatOp::getOperationName(),
                       {splat.getInput()}, {expected}, b.getDictionaryAttr({
                           b.getNamedAttr("shape", table.getShape())}),
                       state.getLoc(), &module.getBody().front())))
              return failure();
          }
        } else if (dff) {
          scalarControls.push_back(storage->getOperand(0));
          scalarControls.push_back(storage->getOperand(1));
        } else {
          for (unsigned i : {0u, 1u, 2u})
            scalarControls.push_back(storage->getOperand(i));
        }
        if (!storage || !arity || storage->getNumOperands() != arity ||
            storage->getNumResults() != 1 ||
            storage->getAttr("occurrence") != state.getOccurrence() ||
            storage->getAttr("instance_name") != state.getInstanceNameAttr() ||
            storage->getAttr("callee") !=
                FlatSymbolRefAttr::get(
                    original.getContext(),
                    cast<StringAttr>((dff ? prepared.dff : prepared.dffe)
                                         ->getAttr(SymbolTable::getSymbolAttrName()))
                        .getValue()) ||
            storage->getAttr("parameters") !=
                ArrayAttr::get(original.getContext(), {}) ||
            storage->getAttr("type_arguments") !=
                ArrayAttr::get(original.getContext(),
                               {TypeAttr::get(element)}) ||
            storage->getResult(0).getType() != state.getQ().getType() ||
            candidate.packageMap.lookupOrNull(state.getQ()) !=
                storage->getResult(0) ||
            scalarControls[0] != authenticated.lookupOrNull(state.getClk()) ||
            scalarControls[1] != authenticated.lookupOrNull(state.getRst()) ||
            storage->getOperand(dff ? 3 : 4) !=
                authenticated.lookupOrNull(state.getInitial()))
          return reject("original state owner/Q/type/domain/initial differs");
        Builder b(original.getContext());
        auto primitive = dff ? prepared.dff : prepared.dffe;
        NamedAttrList storageAttributes(b.getDictionaryAttr({
            b.getNamedAttr("instance_name", state.getInstanceNameAttr()),
            b.getNamedAttr("occurrence", state.getOccurrence()),
            b.getNamedAttr(
                "callee",
                FlatSymbolRefAttr::get(
                    original.getContext(),
                    cast<StringAttr>(primitive->getAttr(
                                         SymbolTable::getSymbolAttrName()))
                        .getValue())),
            b.getNamedAttr("parameters", b.getArrayAttr({})),
            b.getNamedAttr("type_arguments", b.getArrayAttr({TypeAttr::get(element)}))}));
        if (table)
          storageAttributes.append("shape", table.getShape());
        SmallVector<Value> storageOperands;
        for (unsigned i = 0; i < arity - 1; ++i)
          storageOperands.push_back(storage->getOperand(i));
        storageOperands.push_back(authenticated.lookup(state.getInitial()));
        if (failed(generatedProof(
                storage, table ? ac::CollectionOp::getOperationName()
                               : ac::InstanceOp::getOperationName(),
                storageOperands, {state.getQ().getType()},
                storageAttributes.getDictionary(original.getContext()),
                state.getLoc(), &module.getBody().front())))
          return failure();
        authenticated.map(state.getQ(), storage->getResult(0));
      } else if (auto memory = dyn_cast<ac::InstanceOp>(op);
                 memory && memory->hasAttr("ac.source_memory")) {
        NamedAttrList expected(memory->getAttrs());
        expected.erase("ac.source_memory"); expected.erase("ac.payload_constraint");
        if (!isa<ac::InstanceOp>(target) || target->getAttrs() != expected.getAttrs() ||
            target->getPropertiesAsAttribute() != memory->getPropertiesAsAttribute() ||
            target->getLoc() != memory.getLoc() || target->getNumOperands() != memory->getNumOperands() ||
            target->getNumResults() != memory->getNumResults() ||
            target->getResultTypes() != memory->getResultTypes() || failed(cover(target)))
          return reject("original Memory allocation identity differs");
        for (unsigned operand = 0; operand < 2; ++operand)
          if (target->getOperand(operand) != authenticated.lookupOrNull(memory.getInputs()[operand]))
            return reject("original Memory clock/reset domain binding differs");
        for (auto [source, result] : llvm::zip(memory.getOutputs(), target->getResults())) {
          if (candidate.packageMap.lookupOrNull(source) != result)
            return reject("original Memory data/live result ordinal differs");
          authenticated.map(source, result);
        }
      } else if (auto queue = dyn_cast<ac::InstanceOp>(op);
                 queue && ac::isQueueInstance(queue)) {
        NamedAttrList expected(queue->getAttrs());
        for (StringRef attr : {"ac.source_unbound_push", "ac.source_unbound_pop", "ac.payload_constraint"})
          expected.erase(attr);
        if (!isa<ac::InstanceOp>(target) ||
            target->getAttrs() != expected.getAttrs() ||
            target->getPropertiesAsAttribute() != queue->getPropertiesAsAttribute() ||
            target->getLoc() != queue->getLoc() || target->getNumOperands() != queue->getNumOperands() ||
            target->getNumResults() != queue->getNumResults() || failed(cover(target)))
          return reject("original queue allocation identity differs");
        for (auto [ordinal, operand] : llvm::enumerate(queue->getOperands())) {
          bool controlled = queue->hasAttr("ac.source_unbound_push") && (ordinal == 2 || ordinal == 3);
          controlled |= queue->hasAttr("ac.source_unbound_pop") && ordinal == 4;
          if (!controlled && target->getOperand(ordinal) != authenticated.lookupOrNull(operand))
            return reject("original raw queue binding differs");
        }
      } else if (auto allocation = dyn_cast<ac::InstanceOp>(op)) {
        auto child =
            dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(allocation));
        unsigned extraInputs = 0, extraOutputs = 0;
        if (child) {
          const auto &routing =
              prepared.definitions[prepared.definitionIndex.lookup(
                  child.operator->())];
          extraInputs = 2 * routing.downward.size() + queueInputCount(routing) + 2 * routing.memoryIncoming.size();
          extraOutputs = routing.upward.size() + routing.queuePopSummaries.size();
          auto rebuilt = cast<ac::ModuleOp>(candidate.packageMap.lookup(child.operator->()));
          auto layout = ac::getModulePortLayout(rebuilt);
          if (failed(layout)) return failure();
          extraInputs += llvm::count_if(layout->inputValidity, [](int64_t i) { return i >= 0; });
          extraOutputs += llvm::count_if(layout->outputValidity, [](int64_t i) { return i >= 0; });
          extraOutputs += layout->routedOutputValidity.size();
        }
        if (!isa<ac::InstanceOp>(target) ||
            target->getAttrs() != op.getAttrs() ||
            target->getPropertiesAsAttribute() !=
                op.getPropertiesAsAttribute() ||
            target->getLoc() != op.getLoc() ||
            target->getNumRegions() != op.getNumRegions() ||
            target->getNumOperands() != op.getNumOperands() + extraInputs ||
            target->getNumResults() != op.getNumResults() + extraOutputs ||
            target->getResults().take_front(op.getNumResults()).getTypes() !=
                op.getResultTypes())
          return reject("original instance identity/port prefix differs");
        if (failed(cover(target)))
          return failure();
        for (auto [source, value] :
             llvm::zip(op.getOperands(),
                       target->getOperands().take_front(op.getNumOperands())))
          if (value != authenticated.lookupOrNull(source))
            return reject("original instance input prefix differs");
        for (auto [source, value] :
             llvm::zip(op.getResults(),
                       target->getResults().take_front(op.getNumResults()))) {
          if (candidate.packageMap.lookupOrNull(source) != value)
            return reject("original instance result ordinal differs");
          authenticated.map(source, value);
        }
      } else if (isa<ac::YieldOp>(op)) {
        if (!isa<ac::YieldOp>(target) || target->getAttrs() != op.getAttrs() ||
            target->getPropertiesAsAttribute() !=
                op.getPropertiesAsAttribute() ||
            target->getLoc() != op.getLoc() || target->getNumRegions() ||
            target->getNumResults() ||
            target->getNumOperands() !=
                definition.sourceOutputs + definition.upward.size() + definition.queuePopSummaries.size() + validityOutputs)
          return reject("original module yield prefix/route layout differs");
        if (failed(cover(target)))
          return failure();
        for (auto [source, value] : llvm::zip(
                 op.getOperands(),
                 target->getOperands().take_front(definition.sourceOutputs)))
          if (value != authenticated.lookupOrNull(source))
            return reject("original module yield operand differs");
        for (auto [ordinal, route] : llvm::enumerate(definition.upward)) {
          auto value = routed(original, route.path, route.value);
          if (failed(value) ||
              target->getOperand(definition.sourceOutputs + ordinal) != *value)
            return reject("original upward route endpoint differs");
        }
      } else if (failed(exactClone(&op, target, authenticated,
                                   candidate.packageMap))) {
        return failure();
      }
    }
    return success();
  }

#include "TransactionResourceDemandSourceProof.inc"

  LogicalResult transactionProof(const TransactionPreparation &preparation,
                                 const MaterializedTransaction &record) {
    const auto &plan = *preparation.plan;
    auto transaction = plan.transaction;
    auto rule = record.rule;
    auto registration = ac::getSourceRegistrationEntry(transaction);
    if (failed(registration)) return failure();
    auto entryMethod = *registration ? (**registration).method : ac::SourceMethodOp();
    auto entryCall = *registration ? (**registration).call : ac::SourceMethodCallOp();
    if (plan.rootFrame != 0 || plan.frames.empty() || plan.instances.empty() ||
        plan.guard != plan.frames[0].guard || plan.frames[0].parent ||
        plan.frames[0].method != entryMethod || plan.frames[0].call ||
        plan.frames[0].registrationEntry != entryCall ||
        plan.frames[0].anchor != 0 ||
        plan.frames[0].transaction != transaction ||
        plan.instances[0].definition !=
            transaction->getParentOfType<ac::ModuleOp>() ||
        plan.instances[0].parent || plan.instances[0].allocation ||
        !plan.instances[0].occurrencePath.empty() ||
        plan.frames[0].callOccurrence || plan.frames[0].receiver)
      return reject("original root frame/instance differs");
    ac::HardwareBindings rootBindings;
    rootBindings.owner = plan.lexicalModule;
    if (!sameBindings(plan.instances[0].bindings, rootBindings) ||
        !sameBindings(plan.frames[0].bindings, rootBindings))
      return reject("original root bindings differ");
    for (auto [id, instance] : llvm::enumerate(plan.instances)) {
      if (!charge(budget, transaction,
                  16 + instance.occurrencePath.size() +
                      instance.bindings.integers.size() +
                      instance.bindings.types.size()))
        return failure();
      if (!id)
        continue;
      if (!instance.parent || *instance.parent >= id || !instance.allocation ||
          instance.allocation->getParentOp() !=
              plan.instances[*instance.parent].definition.operator->() ||
          hardware.resolveCallee(instance.allocation) !=
              instance.definition.operator->())
        return reject("original lexical instance edge differs");
      auto bindings = hardware.bindInstance(
          instance.allocation, plan.instances[*instance.parent].bindings);
      if (failed(bindings) || !sameBindings(instance.bindings, *bindings))
        return reject("original lexical instance bindings differ");
      SmallVector<Attribute, 0> expectedPath;
      const auto &parent = plan.instances[*instance.parent];
      uint64_t expectedPathSize =
          SourceCheckBudget::add(parent.occurrencePath.size(), 1);
      if (expectedPathSize > expectedPath.max_size() ||
          !product(budget, transaction, expectedPathSize,
                   1 + SourceCheckBudget::bytes(sizeof(Attribute))))
        return failure();
      expectedPath.reserve(expectedPathSize);
      expectedPath.append(parent.occurrencePath.begin(),
                          parent.occurrencePath.end());
      expectedPath.push_back(
          instance.allocation->getAttrOfType<DictionaryAttr>("occurrence"));
      if (instance.occurrencePath !=
          ArrayAttr::get(transaction.getContext(), expectedPath))
        return reject("original lexical occurrence path differs");
    }
    SmallVector<detail::TransactionForwardingPreparation> expectedForwarding;
    auto ancestors = originalPredecessors(transaction);
    if (failed(ancestors)) return failure();
    for (auto [ownerIndex, owner] : llvm::enumerate(plan.owners)) {
      auto key = routingKey(anchorPath(plan, owner.anchor), owner.declaration->getResult(0));
      for (auto predecessor : *ancestors) {
        auto index = originalTransactionIndex(predecessor);
        if (failed(index)) return failure();
        const auto &prior = prepared.transactions[*index];
        if (!(prior.plan->domain == plan.domain))
          return reject("original predecessor clock/reset identity differs");
        for (auto [ordinal, output] : llvm::enumerate(prior.outputs)) {
          const auto &state = prior.plan->owners[output.binding.owner];
          if (key != routingKey(anchorPath(*prior.plan, state.anchor), state.declaration->getResult(0)))
            continue;
          if (!prepayAppend(budget, transaction, expectedForwarding)) return failure();
          uint64_t capture = preparation.liveIns.size() + 2 * expectedForwarding.size();
          expectedForwarding.push_back({ownerIndex, *index, ordinal, capture, capture + 1});
        }
      }
    }
    if (expectedForwarding.size() != preparation.forwarding.size() ||
        expectedForwarding.size() != record.forwarding.size())
      return reject("original forwarding capture inventory differs");
    for (auto [expected, actual] : llvm::zip(expectedForwarding, preparation.forwarding))
      if (expected.owner != actual.owner || expected.predecessor != actual.predecessor ||
          expected.output != actual.output || expected.dataCapture != actual.dataCapture ||
          expected.enableCapture != actual.enableCapture)
        return reject("original forwarding predecessor/owner/capture differs");
    uint64_t expectedCaptures = preparation.liveIns.size() + 2 * expectedForwarding.size();
    auto expectedExternalPops = originalQueueExternalPops(preparation, expectedCaptures);
    if (failed(expectedExternalPops)) return failure();
    expectedCaptures += expectedExternalPops->size();
    uint64_t expectedFire = SourceCheckBudget::add(
        transaction->getNumResults(),
        SourceCheckBudget::multiply(2, preparation.outputs.size()));
    uint64_t cursor = SourceCheckBudget::add(expectedFire, 1);
    if (preparation.observationPathResults.size() != plan.observations.size())
      return reject("original observation result inventory differs");
    for (auto [id, observation] : llvm::enumerate(plan.observations)) {
      if (preparation.observationPathResults[id] != cursor)
        return reject("original observation result layout differs");
      cursor = SourceCheckBudget::add(
          cursor, SourceCheckBudget::add(1, observation.values.size()));
    }
    uint64_t legacyTotal = SourceCheckBudget::add(
        cursor, SourceCheckBudget::multiply(
                    2, SourceCheckBudget::add(preparation.controls.size(),
                                              plan.bodyChecks.size())));
    SmallVector<ac::StateOwnerId> expectedEntryOwners;
    for (const auto &forward : expectedForwarding)
      if (expectedEntryOwners.empty() || expectedEntryOwners.back() != forward.owner)
        expectedEntryOwners.push_back(forward.owner);
    if (!llvm::equal(expectedEntryOwners, preparation.entryOwners))
      return reject("original forwarded owner result inventory differs");
    uint64_t expectedTotal = SourceCheckBudget::add(legacyTotal, expectedEntryOwners.size());
    for (const auto &proposal : preparation.queueProposals) {
      if (proposal.rawResult != expectedTotal || proposal.pathResult != expectedTotal + 1 ||
          (proposal.dataResult && *proposal.dataResult != expectedTotal + 2))
        return reject("original queue proposal layout differs");
      expectedTotal += proposal.dataResult ? 3 : 2;
    }
    for (const auto &proposal : preparation.memoryProposals) {
      if (proposal.rawResult != expectedTotal || proposal.pathResult != expectedTotal + 1)
        return reject("original Memory proposal layout differs");
      expectedTotal += 2;
      for (uint64_t argument : proposal.argumentResults)
        if (argument != expectedTotal++) return reject("original Memory argument layout differs");
    }
    SmallVector<Attribute> expectedDomains;
    uint64_t domainCursor = 0;
    bool partialWrites = llvm::any_of(plan.writes, [](const auto &write) {
      return !write.selectors.empty();
    });
    for (auto [id, write] : llvm::enumerate(plan.writes)) {
      if (!charge(budget, transaction)) return failure();
      if (!partialWrites || write.forwardedProposal) continue;
      if (domainCursor == preparation.writeDomains.size())
        return reject("original write domain inventory is missing");
      const auto &domain = preparation.writeDomains[domainCursor++];
      auto candidate = llvm::find_if(preparation.outputs, [&](const auto &output) {
        return output.binding.owner == write.owner;
      });
      if (candidate == preparation.outputs.end() || domain.write != id ||
          domain.candidate != uint64_t(candidate - preparation.outputs.begin()) ||
          domain.selectors != write.selectors || domain.result != expectedTotal)
        return reject("original write domain owner/selector/result differs");
      Builder b(transaction.getContext());
      expectedDomains.push_back(b.getDictionaryAttr({
          b.getNamedAttr("candidate", b.getI64IntegerAttr(domain.candidate)),
          b.getNamedAttr("selectors", write.selectors),
          b.getNamedAttr("mask", b.getI64IntegerAttr(expectedTotal))}));
      ++expectedTotal;
    }
    if (domainCursor != preparation.writeDomains.size() ||
        (expectedDomains.empty() ? bool(preparation.expectedAdmission.get("write_domains"))
          : preparation.expectedAdmission.get("write_domains") !=
                Builder(transaction.getContext()).getArrayAttr(expectedDomains)))
      return reject("original write domain metadata differs");
    if (expectedEntryOwners.empty()) {
      if (preparation.expectedAdmission.get("ehr_entries"))
        return reject("original unordered transaction has invented entry metadata");
    } else {
      Builder b(transaction.getContext());
      SmallVector<Attribute> entries;
      for (auto [ordinal, ownerIndex] : llvm::enumerate(expectedEntryOwners)) {
        const auto &owner = plan.owners[ownerIndex];
        SmallVector<Attribute> ancestors;
        for (const auto &forward : expectedForwarding) {
          if (forward.owner != ownerIndex) continue;
          ancestors.push_back(b.getDictionaryAttr({
              b.getNamedAttr("registration", prepared.transactions[forward.predecessor].plan->transaction->getAttrOfType<DictionaryAttr>("occurrence")),
              b.getNamedAttr("data_capture", b.getI64IntegerAttr(forward.dataCapture)),
              b.getNamedAttr("enable_capture", b.getI64IntegerAttr(forward.enableCapture))}));
        }
        auto path = anchorPath(plan, owner.anchor);
        auto identity = b.getDictionaryAttr({
            b.getNamedAttr("owner", b.getDictionaryAttr({b.getNamedAttr("instance_path", routePath(transaction.getContext(), path))})),
            b.getNamedAttr("declaration", owner.occurrence),
            b.getNamedAttr("element", b.getArrayAttr({}))});
        auto oldQ = llvm::find(preparation.liveIns, owner.oldQ) - preparation.liveIns.begin();
        entries.push_back(b.getDictionaryAttr({
            b.getNamedAttr("state", identity),
            b.getNamedAttr("old_q_capture", b.getI64IntegerAttr(oldQ)),
            b.getNamedAttr("predecessors", b.getArrayAttr(ancestors)),
            b.getNamedAttr("result", b.getI64IntegerAttr(legacyTotal + ordinal))}));
      }
      if (preparation.expectedAdmission.get("ehr_entries") != b.getArrayAttr(entries))
        return reject("original entry owner/predecessor/result metadata differs");
    }
    auto expectedClock = llvm::find(preparation.liveIns, plan.clockValue) -
                         preparation.liveIns.begin();
    auto expectedReset = llvm::find(preparation.liveIns, plan.resetValue) -
                         preparation.liveIns.begin();
    Builder captureAttributes(transaction.getContext());
    SmallVector<Attribute> captureRows;
    uint64_t baseCaptures = expectedCaptures;
    uint64_t availabilityPayloadCaptures = preparation.liveIns.size() + 2 * expectedForwarding.size();
    for (auto [ordinal, pair] : llvm::enumerate(record.validityCaptures)) {
      if (!prepared.valueAvailability || pair.first >= availabilityPayloadCaptures ||
          pair.second != baseCaptures + ordinal ||
          pair.second >= rule->getNumOperands() ||
          rule.getBody().front().getArgument(pair.second).use_empty())
        return reject("original validity capture inventory differs");
      captureRows.push_back(captureAttributes.getDictionaryAttr({
          captureAttributes.getNamedAttr("payload", captureAttributes.getI64IntegerAttr(pair.first)),
          captureAttributes.getNamedAttr("validity", captureAttributes.getI64IntegerAttr(pair.second))}));
    }
    expectedCaptures += record.validityCaptures.size();
    if ((captureRows.empty() && rule->hasAttr("ac.value_availability_captures")) ||
        (!captureRows.empty() && rule->getAttr("ac.value_availability_captures") !=
             captureAttributes.getArrayAttr(captureRows)))
      return reject("original validity capture metadata differs");
    if (preparation.clockCapture != expectedClock ||
        preparation.resetCapture != expectedReset ||
        preparation.fireResult != expectedFire ||
        preparation.checkBase != cursor ||
        preparation.totalResults != expectedTotal ||
        preparation.expectedAdmission.size() !=
            (6 + !expectedEntryOwners.empty() + !preparation.requirements.empty() +
             !preparation.resources.empty() + !preparation.queueProposals.empty() + !preparation.memoryProposals.empty() +
             !expectedDomains.empty()) ||
        preparation.expectedAdmission.getAs<IntegerAttr>("clock_capture") !=
            Builder(transaction.getContext())
                .getI64IntegerAttr(expectedClock) ||
        preparation.expectedAdmission.getAs<IntegerAttr>("reset_capture") !=
            Builder(transaction.getContext())
                .getI64IntegerAttr(expectedReset) ||
        preparation.expectedAdmission.getAs<IntegerAttr>("fire_result") !=
            Builder(transaction.getContext()).getI64IntegerAttr(expectedFire))
      return reject("original transaction prepared layout differs");
    if (!operations.contains(rule.operator->()) ||
        !isa<ac::RuleOp>(rule.operator->()) ||
        candidate.packageMap.lookupOrNull(transaction.operator->()) !=
            rule.operator->() ||
        rule->getParentOp() != candidate.packageMap.lookupOrNull(
                                   plan.lexicalModule.operator->()) ||
        rule->getNumRegions() != 1 || !rule.getBody().hasOneBlock() ||
        rule.getBody().front().empty() ||
        !isa<ac::YieldOp>(rule.getBody().front().getTerminator()) ||
        rule->getNumOperands() != expectedCaptures ||
        rule.getBody().front().getNumArguments() != expectedCaptures ||
        rule->getNumResults() != preparation.totalResults ||
        rule->getAttr("ac.sampled_admission") !=
            preparation.expectedAdmission ||
        rule->getAttr("ac.required_checks") !=
            preparation.expectedRequiredChecks ||
        rule->hasAttr("ac.guard_location") || rule->hasAttr("ac.registration_entry") ||
        record.boundValues.size() != plan.values.size() ||
        record.versions.size() != plan.versions.size() ||
        record.branchResults.size() != plan.branchResults.size() ||
        record.stateJoins.size() != plan.joins.size() ||
        record.conditionalWrites.size() != plan.writes.size() ||
        record.selectedWrites.size() != (preparation.writeDomains.empty() ? 0 : plan.writes.size()) ||
        record.frames.size() != plan.frames.size() ||
        record.prefixes.size() != preparation.controls.size() + 1 ||
        record.controls.size() != preparation.controls.size() ||
        record.selections.size() != plan.selections.size() ||
        record.bodyChecks.size() != plan.bodyChecks.size() ||
        record.observations.size() != plan.observations.size() ||
        record.requirements.size() != preparation.requirements.size() ||
        record.resources.size() != preparation.resources.size() ||
        record.queuePaths.size() != plan.queueAccesses.size() ||
        record.memoryPaths.size() != plan.memoryAccesses.size() ||
        record.memoryReadPrefixes.size() != plan.memoryAccesses.size() ||
        record.commits.size() != preparation.outputs.size())
      return reject(
          "original transaction layout/domain/check metadata differs");
    if (failed(cover(rule)))
      return failure();
    NamedAttrList expected(transaction->getAttrs()), actual(rule->getAttrs());
    expected.erase("ac.guard_location");
    expected.erase("ac.registration_entry");
    actual.erase("ac.required_checks");
    actual.erase("ac.sampled_admission");
    actual.erase("ac.value_availability_captures");
    if (expected.getDictionary(transaction.getContext()) !=
        actual.getDictionary(transaction.getContext()))
      return reject("original transaction registration/attributes differ");
    for (const auto &map : record.frames) {
      uint64_t entries = SourceCheckBudget::add(
          map.getOperationMap().size(),
          SourceCheckBudget::add(map.getBlockMap().size(),
                                 map.getValueMap().size()));
      if (!charge(budget, transaction, SourceCheckBudget::multiply(entries, 4)))
        return failure();
      for (const auto &entry : map.getOperationMap())
        if (!operations.contains(entry.second))
          return reject("invocation map contains erased clone");
      for (const auto &entry : map.getValueMap())
        if (!values.contains(entry.second))
          return reject("invocation map contains erased wire");
      for (const auto &entry : map.getBlockMap())
        if (!blocks.contains(entry.second))
          return reject("invocation map contains erased block");
    }
    if (!charge(budget, transaction,
                SourceCheckBudget::add(
                    SourceCheckBudget::multiply(
                        plan.frames.size(),
                        SourceCheckBudget::bytes(sizeof(IRMapping))),
                    SourceCheckBudget::multiply(
                        plan.values.size() + plan.versions.size(), 4))))
      return failure();
    SmallVector<IRMapping, 0> authenticated(plan.frames.size());
    DenseMap<std::pair<ac::LexicalInstanceId, Value>, Value> lexical;
    for (auto [ordinal, id] : llvm::enumerate(preparation.liveIns)) {
      const auto &ref = plan.values[id];
      auto path = anchorPath(plan, ref.anchor);
      auto capture = routed(plan.lexicalModule, path, ref.value);
      if (failed(capture) || rule->getOperand(ordinal) != *capture ||
          rule.getBody().front().getArgument(ordinal).getType() !=
              capture->getType())
        return reject("original lexical capture/pin route differs");
      Value argument = rule.getBody().front().getArgument(ordinal);
      lexical.try_emplace({ref.anchor, ref.value}, argument);
    }
    SmallVector<Value> visibleEntries(plan.owners.size());
    for (auto [ownerIndex, owner] : llvm::enumerate(plan.owners)) {
      const auto &ref = plan.values[owner.oldQ];
      visibleEntries[ownerIndex] = lexical.lookup({ref.anchor, ref.value});
    }
    Operation *last = nullptr;
    for (auto [ordinal, forward] : llvm::enumerate(expectedForwarding)) {
      const auto &prior = prepared.transactions[forward.predecessor];
      const auto &output = prior.outputs[forward.output];
      auto producer = candidate.transactions[forward.predecessor].rule;
      Value data = producer->getResult(output.dataResult);
      Value enable = producer->getResult(output.enableResult);
      if (rule->getOperand(forward.dataCapture) != data ||
          rule->getOperand(forward.enableCapture) != enable ||
          rule.getBody().front().getArgument(forward.dataCapture).getType() != data.getType() ||
          rule.getBody().front().getArgument(forward.enableCapture).getType() != enable.getType())
        return reject("original predecessor proposal capture is disconnected");
      if (failed(localBranchProof(transaction, plan.owners[forward.owner].payloadType,
              plan.frames[0].bindings,
              rule.getBody().front().getArgument(forward.enableCapture),
              rule.getBody().front().getArgument(forward.dataCapture),
              visibleEntries[forward.owner], record.forwarding[ordinal],
              &rule.getBody().front(), last))) return failure();
      visibleEntries[forward.owner] = record.forwarding[ordinal].value;
    }
    if (!product(budget, transaction, plan.versions.size(),
                 1 + SourceCheckBudget::bytes(sizeof(MaterializedVersion))) ||
        !product(
            budget, transaction, plan.owners.size(),
            3 * (1 + SourceCheckBudget::bytes(sizeof(ac::StateVersionId)))))
      return failure();
    SmallVector<MaterializedVersion, 0> versions(plan.versions.size());
    SmallVector<ac::StateVersionId, 0> environment(plan.owners.size());
    SmallVector<uint64_t, 0> ordinals(plan.owners.size(), 0);
    SmallVector<bool, 0> originalWriters(plan.owners.size(), false);
    auto bound = [&](ac::BoundValueId id) -> Value {
      const auto &ref = plan.values[id];
      if (ref.value.getDefiningOp() == transaction.operator->())
        return rule->getResult(cast<OpResult>(ref.value).getResultNumber());
      if (moduleValue(ref.value, plan.instances[ref.anchor].definition))
        return lexical.lookup({ref.anchor, ref.value});
      return authenticated[ref.frame].lookupOrNull(ref.value);
    };
    SourceReplayValues replay(plan, budget);
    if (failed(replay.initialize()) ||
        failed(replay.identify(plan.clockValue, transaction.getClk(), 0, 0)) ||
        failed(replay.identify(plan.resetValue, transaction.getRst(), 0, 0)))
      return failure();
    auto originalClock = replay.endpoint(plan.clockValue, {});
    auto originalReset = replay.endpoint(plan.resetValue, {});
    if (failed(originalClock) || failed(originalReset) ||
        !(plan.domain == ac::TransactionDomain{*originalClock, *originalReset}))
      return reject("original transaction domain identity differs");
    if (!product(budget, transaction, plan.versions.size(),
                 1 + SourceCheckBudget::bytes(sizeof(ac::TransactionPoint))))
      return failure();
    SmallVector<ac::TransactionPoint, 0> versionValidity(plan.versions.size());
    for (auto [id, version] : llvm::enumerate(plan.versions))
      if (version.kind == ac::StateVersionKind::OldQ) {
        if (version.transaction != transaction || !version.value ||
            version.predecessor || version.write || version.join ||
            version.ordinal != 0 || version.owner >= plan.owners.size() ||
            *version.value != plan.owners[version.owner].oldQ)
          return reject("original old-Q version shape differs");
        if (failed(replay.requireVersionAvailable(id, {}, {})))
          return failure();
        versions[id] = {visibleEntries[version.owner], record.versions[id].rawIntent};
        environment[version.owner] = id;
        if (!versions[id].data ||
            failed(literalProof(
                versions[id].rawIntent,
                rule->getOperand(preparation.clockCapture).getType(),
                transaction.getOccurrence(), transaction.getLoc(), false,
                &rule.getBody().front())))
          return reject("original old-Q version is absent");
      }
    struct SourceObservation {
      Value path;
      SmallVector<Value, 0> values;
    };
    uint64_t payloadValues = 0;
    for (const auto &observation : plan.observations)
      payloadValues =
          SourceCheckBudget::add(payloadValues, observation.values.size());
    uint64_t effectEventsCount = SourceCheckBudget::add(
        plan.bodyChecks.size(), plan.observations.size());
    uint64_t sourceStorage = SourceCheckBudget::add(
        SourceCheckBudget::multiply(plan.guards.size(),
                                    SourceCheckBudget::bytes(sizeof(Value))),
        SourceCheckBudget::add(
            SourceCheckBudget::multiply(
                plan.bodyChecks.size(),
                SourceCheckBudget::bytes(sizeof(std::pair<Value, Value>))),
            SourceCheckBudget::add(
                SourceCheckBudget::multiply(
                    plan.observations.size(),
                    SourceCheckBudget::bytes(sizeof(SourceObservation))),
                SourceCheckBudget::add(
                    SourceCheckBudget::multiply(
                        payloadValues, SourceCheckBudget::bytes(sizeof(Value))),
                    SourceCheckBudget::multiply(
                        effectEventsCount,
                        SourceCheckBudget::bytes(
                            sizeof(const ac::TransactionStateEvent *)))))));
    sourceStorage = SourceCheckBudget::add(
        sourceStorage,
        SourceCheckBudget::add(
            SourceCheckBudget::multiply(
                plan.controls.size(), SourceCheckBudget::bytes(sizeof(Value))),
            SourceCheckBudget::multiply(plan.selections.size(),
                                        SourceCheckBudget::bytes(sizeof(
                                            detail::MaterializedSelection)))));
    if (!charge(budget, transaction, sourceStorage))
      return failure();
    SmallVector<Value> predicates;
    SmallVector<std::pair<Value, Value>, 0> sourceChecks(
        plan.bodyChecks.size());
    SmallVector<SourceObservation, 0> sourceObservations(
        plan.observations.size());
    SmallVector<const ac::TransactionStateEvent *, 0> effectEvents;
    effectEvents.reserve(effectEventsCount);
    for (const auto &event : plan.events)
      if (event.kind == ac::TransactionStateEventKind::BodyCheck ||
          event.kind == ac::TransactionStateEventKind::Observe)
        effectEvents.push_back(&event);
    size_t nextEffectEvent = 0;
    std::function<LogicalResult(ac::InvocationFrameId,
                                SmallVectorImpl<ac::StateOwnerId> &)>
        invocation;
    Type bit = rule->getOperand(preparation.clockCapture).getType();
    uint64_t formalIndex = 0;
    std::unique_ptr<ac::detail::SourceFormalProposalProof> replayFormalProof;
    if (!product(budget, transaction, plan.frames.size(),
                 2 + SourceCheckBudget::bytes(sizeof(SmallVector<std::optional<ac::StateOwnerId>, 0>) +
                                              sizeof(SmallVector<ac::BoundValueId, 0>)))) return failure();
    SmallVector<SmallVector<std::optional<ac::StateOwnerId>, 0>, 0>
        replayFormalOwners(plan.frames.size());
    SmallVector<SmallVector<ac::BoundValueId, 0>, 0> replayFormalValues(plan.frames.size());
    if (!product(budget, transaction, plan.frames.size(),
                 1 + SourceCheckBudget::bytes(sizeof(SmallVector<uint64_t, 0>)))) return failure();
    SmallVector<SmallVector<uint64_t, 0>, 0> replayFormalIntents(plan.frames.size());
    uint64_t eventIndex = 0, readIndex = 0, writeIndex = 0, callIndex = 0,
             branchIndex = 0, joinIndex = 0, resultIndex = 0, guardIndex = 0, queueIndex = 0, memoryIndex = 0;
    ac::AdmissionPrefix admission = 0;
    ac::TransactionSourcePath sourcePath;
    auto point = [&] { return ac::TransactionPoint{admission, sourcePath}; };
    uint64_t selectionIndex = 0, nextFrame = 1;
    SmallVector<Value, 0> sourceRaw(plan.controls.size());
    auto event =
        [&](ac::TransactionStateEventKind kind, ac::InvocationFrameId frame,
            std::optional<uint64_t> id = std::nullopt) -> LogicalResult {
      if (!charge(budget, transaction, 12) || eventIndex == plan.events.size())
        return reject("original event inventory is incomplete");
      const auto &actual = plan.events[eventIndex++];
      ac::TransactionStateEvent expected;
      expected.kind = kind;
      expected.frame = frame;
      expected.point = point();
      using K = ac::TransactionStateEventKind;
      switch (kind) {
      case K::QueueAccess:
        expected.queueAccess = id;
        break;
      case K::MemoryAccess:
        expected.memoryAccess = id;
        break;
      case K::Read:
        expected.read = id;
        break;
      case K::Write:
        expected.write = id;
        break;
      case K::FormalWrite:
        expected.formalWrite = id;
        break;
      case K::GuardEnter:
      case K::GuardExit:
        expected.guard = id;
        break;
      case K::SelectionEnter:
      case K::SelectionExit:
        expected.selection = id;
        break;
      case K::CallEnter:
      case K::CallExit:
        expected.call = id;
        break;
      case K::BodyCheck:
        expected.bodyCheck = id;
        break;
      case K::Observe:
        expected.observation = id;
        break;
      case K::StateJoin:
        expected.join = id;
        break;
      case K::BranchResult:
        expected.branchResult = id;
        break;
      case K::BranchEnter:
      case K::ThenEnter:
      case K::ThenExit:
      case K::ElseEnter:
      case K::ElseExit:
      case K::BranchExit:
        expected.branch = id;
        break;
      case K::TransactionEnter:
      case K::TransactionExit:
        break;
      }
      if (actual.kind != expected.kind || actual.frame != expected.frame ||
          !sameSourcePoint(actual.point, expected.point) ||
          actual.read != expected.read || actual.queueAccess != expected.queueAccess ||
          actual.memoryAccess != expected.memoryAccess ||
          actual.selection != expected.selection ||
          actual.write != expected.write || actual.formalWrite != expected.formalWrite ||
          actual.guard != expected.guard ||
          actual.call != expected.call ||
          actual.bodyCheck != expected.bodyCheck ||
          actual.observation != expected.observation ||
          actual.branch != expected.branch || actual.join != expected.join ||
          actual.branchResult != expected.branchResult)
        return reject("original event order/reference/prefix differs");
      return success();
    };
    auto ownerProof = [&](Operation *effect, ac::InvocationFrameId frame,
                          ac::StateOwnerId id, Value path, Value payload,
                          ArrayAttr selectors, ValueRange indices,
                          DictionaryAttr occurrence) -> LogicalResult {
      if (id >= plan.owners.size() || !path || !payload)
        return reject("original state access shape differs");
      const auto &owner = plan.owners[id];
      auto anchor = plan.frames[frame].anchor;
      auto module = plan.instances[anchor].definition;
      ac::SourceStateOp declaration;
      for (Operation &op : module.getBody().front()) {
        if (!charge(budget, effect))
          return failure();
        auto state = dyn_cast<ac::SourceStateOp>(op);
        if (state && state.getInstanceNameAttr() == effect->getAttr("state")) {
          if (declaration)
            return reject("original state access owner is ambiguous");
          declaration = state;
        }
      }
      if (!declaration || owner.anchor != anchor ||
          owner.declaration != declaration ||
          effect->getAttr("occurrence") != occurrence)
        return reject("original state access declaration/anchor differs");
      if (owner.instancePath != plan.instances[anchor].occurrencePath ||
          owner.occurrence != declaration.getOccurrence() ||
          owner.domain != plan.domain ||
          failed(replay.identify(owner.oldQ, declaration.getQ(), anchor,
                                 plan.rootFrame)))
        return reject("original owner old-Q/occurrence/domain differs");
      auto clock = replay.find(declaration.getClk(), anchor, plan.rootFrame);
      auto reset = replay.find(declaration.getRst(), anchor, plan.rootFrame);
      if (failed(clock) || failed(reset))
        return failure();
      auto domainClock = replay.endpoint(*clock, point());
      auto domainReset = replay.endpoint(*reset, point());
      if (failed(domainClock) || failed(domainReset) ||
          !(plan.domain == ac::TransactionDomain{*domainClock, *domainReset}))
        return reject("original owner physical domain differs");
      auto type = hardware.resolveType(payload.getType(),
                                       plan.frames[frame].bindings, effect);
      auto expected =
          hardware.resolveType(declaration.getQ().getType(),
                               plan.instances[owner.anchor].bindings, effect);
      if (failed(type) || failed(expected) ||
          !ac::areEquivalentHardwareTypes(*expected, owner.payloadType))
        return reject("original state access owner payload type differs");
      Type selected = *expected;
      unsigned indexOrdinal = 0;
      for (Attribute raw : selectors) {
        if (!charge(budget, effect, 4)) return failure();
        auto selector = cast<DictionaryAttr>(raw);
        if (auto field = selector.getAs<StringAttr>("field")) {
          auto structure = dyn_cast<ac::StructType>(selected);
          auto declaration = structure ? hardware.lookupStruct(structure) : ac::StructOp();
          Type next;
          if (declaration) for (Attribute rawField : declaration.getFields()) {
            if (!charge(budget, effect)) return failure();
            auto item = cast<DictionaryAttr>(rawField);
            if (item.getAs<StringAttr>("name") == field) {
              auto resolved = hardware.resolveType(item.getAs<TypeAttr>("type").getValue(),
                                                    plan.frames[frame].bindings, effect);
              if (failed(resolved)) return failure();
              next = *resolved; break;
            }
          }
          if (!next) return reject("original state field selector differs");
          selected = next;
        } else {
          auto ordinal = selector.getAs<IntegerAttr>("index");
          auto table = dyn_cast<ac::TableType>(selected);
          if (!ordinal || ordinal.getInt() != indexOrdinal || indexOrdinal >= indices.size() ||
              !table || table.getShape().empty())
            return reject("original state index selector differs");
          ++indexOrdinal;
          selected = table.getShape().size() == 1 ? table.getElementType()
              : Type(ac::TableType::get(effect->getContext(),
                  ArrayAttr::get(effect->getContext(), table.getShape().getValue().drop_front()),
                  table.getElementType()));
        }
      }
      if (indexOrdinal != indices.size() || !ac::areEquivalentHardwareTypes(*type, selected))
        return reject("original selected state payload/index count differs");
      return success();
    };
    auto selectProof = [&](Value selected, Value condition, Value whenTrue,
                           Value whenFalse, Type resultType,
                           Location location) -> LogicalResult {
      Builder b(transaction.getContext());
      if (!selected || !condition || !whenTrue || !whenFalse || !resultType ||
          !values.contains(selected) || !values.contains(condition) ||
          !values.contains(whenTrue) || !values.contains(whenFalse) ||
          !ac::areEquivalentHardwareTypes(whenTrue.getType(), resultType) ||
          !ac::areEquivalentHardwareTypes(whenFalse.getType(), resultType) ||
          failed(generatedProof(
              selected.getDefiningOp(), ac::BitsSelectOp::getOperationName(),
              {condition, whenTrue, whenFalse}, {resultType},
              b.getDictionaryAttr({}), location, &rule.getBody().front())))
        return reject("original branch select operands/type differ");
      auto actual = selected.getDefiningOp();
      if (actual->getResult(0) != selected ||
          (last && !last->isBeforeInBlock(actual)))
        return reject("original branch select result/order differs");
      last = actual;
      return success();
    };
    auto physicalWrite = [&](Operation *operation, ac::InvocationFrameId frame,
                             Value sourcePath, Value sourceValue,
                             ArrayAttr selectors, ValueRange indices, bool formal,
                             llvm::function_ref<LogicalResult(ac::StateOwnerId)> markTouched)
                             -> LogicalResult {
          if (writeIndex == plan.writes.size())
            return reject("original write inventory differs");
          auto found = plan.writes.begin() + writeIndex++;
          if (found->frame != frame || found->operation != operation)
            return reject("original write intent is absent");
          Value rhs = bound(found->value);
          Value selectedSource = formal ? cast<ac::SourceFormalWriteOp>(operation).getPayload() : sourceValue;
          if (!found->selectedPayload || found->indices.size() != indices.size() ||
              failed(replay.identify(*found->selectedPayload, selectedSource,
                                     plan.frames[frame].anchor, frame)))
            return reject("original selected RHS/index inventory differs");
          for (auto [ordinal, index] : llvm::enumerate(indices))
            if (failed(replay.identify(found->indices[ordinal], index,
                                       plan.frames[frame].anchor, frame)) ||
                failed(replay.requireAvailable(found->indices[ordinal], point())))
              return reject("original statement-time selected index differs");
          bool forwarded = formal && bool(cast<ac::SourceFormalWriteOp>(operation).getPrevious());
          if (!formal) {
            auto data = dyn_cast<OpResult>(sourceValue);
            auto call = data ? dyn_cast<ac::SourceMethodCallOp>(data.getOwner()) : ac::SourceMethodCallOp();
            if (call) for (uint64_t prior = 0; prior < callIndex; ++prior) {
              if (!charge(budget, operation)) return failure();
              const auto &binding = plan.calls[prior];
              if (binding.operation != call || binding.caller != frame) continue;
              const auto &callee = plan.frames[binding.callee];
              ac::SourceMethodView view{plan.instances[callee.anchor].definition, callee.method, {}};
              auto proposals = view.getFormalProposals();
              auto mappings = call->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
              if (proposals && mappings) for (auto [ordinal, argument] : llvm::enumerate(proposals)) {
                unsigned base = view.getNumPublicResults() + 2 * ordinal;
                if (!charge(budget, operation, 4)) return failure();
                if (data.getResultNumber() != base || call->getResult(base + 1) != sourcePath) continue;
                for (Attribute raw : mappings) {
                  if (!charge(budget, operation)) return failure();
                  auto mapping = cast<DictionaryAttr>(raw);
                  if (mapping.get("formal") == argument && mapping.get("state") == operation->getAttr("state"))
                    forwarded = true;
                }
              }
            }
          }
          if (found->forwardedProposal != forwarded)
            return reject("original forwarded physical proposal mode differs");
          const auto &version = plan.versions[found->version];
          if (!rhs || found->owner >= environment.size() ||
              version.transaction != transaction ||
              version.kind != ac::StateVersionKind::Write || version.join ||
              version.write != writeIndex - 1 ||
              version.owner != found->owner ||
              found->predecessor != environment[found->owner] ||
              version.predecessor != found->predecessor ||
              version.value != found->value ||
              version.ordinal != ++ordinals[found->owner] ||
              found->selectors != selectors ||
              bound(found->authoredPath) !=
                  authenticated[frame].lookupOrNull(sourcePath) ||
              (!formal && failed(ownerProof(operation, frame, found->owner, sourcePath,
                                sourceValue, selectors,
                                indices, found->occurrence))) ||
              rhs != authenticated[frame].lookupOrNull(sourceValue) ||
              (!formal && failed(event(ac::TransactionStateEventKind::Write, frame,
                           writeIndex - 1))))
            return reject("original write owner/predecessor/RHS differs");
          if (failed(replay.identify(found->authoredPath, sourcePath,
                                     plan.frames[frame].anchor, frame)) ||
              failed(replay.requireAvailable(found->authoredPath, point())) ||
              failed(replay.identify(found->value, sourceValue,
                                     plan.frames[frame].anchor, frame)) ||
              failed(replay.requireAvailable(found->value, point())) ||
              failed(replay.requireVersionAvailable(
                  found->predecessor, versionValidity[found->predecessor],
                  point())) ||
              !sameSourcePoint(version.validity, point()))
            return failure();
          versionValidity[found->version] = point();
          Value intent = record.versions[found->version].rawIntent;
          auto pathFact = replay.fact(found->authoredPath, point());
          if (failed(pathFact) || *pathFact != found->pathFact)
            return reject("original write path fact differs");
          Value selectedRhs = rhs;
          if (failed(selectedWriteProof(preparation, record, writeIndex - 1,
                                        versions[found->predecessor].data,
                                        bound(*found->selectedPayload), bound,
                                        &rule.getBody().front(), last))) return failure();
          if (!formal && !selectors.empty()) selectedRhs = record.selectedWrites[writeIndex - 1].data;
          Value data = selectedRhs;
          if (*pathFact && pathFact->getValue()) {
            if (record.conditionalWrites[writeIndex - 1].value ||
                !record.conditionalWrites[writeIndex - 1].generated.empty() ||
                record.versions[found->version].data != selectedRhs ||
                failed(literalProof(intent, bit, transaction.getOccurrence(),
                                    operation->getLoc(), true, &rule.getBody().front())))
              return failure();
          } else {
            Value path = bound(found->authoredPath);
            const auto &prior = versions[found->predecessor];
            const auto &selection = record.conditionalWrites[writeIndex - 1];
            if (record.versions[found->version].data != selection.value ||
                failed(localBranchProof(operation, plan.owners[found->owner].payloadType,
                    plan.frames[frame].bindings, path, selectedRhs, prior.data,
                    selection, &rule.getBody().front(), last)))
              return failure();
            auto result = dyn_cast_or_null<OpResult>(intent);
            Builder b(operation->getContext());
            if (!result || result.getResultNumber() != 0 ||
                failed(generatedProof(result.getOwner(),
                    ac::BitsBinaryOp::getOperationName(),
                    {prior.rawIntent, path}, {bit},
                    b.getDictionaryAttr(
                        {b.getNamedAttr("opcode", b.getStringAttr("or"))}),
                    operation->getLoc(), &rule.getBody().front())))
              return failure();
            data = selection.value;
          }
          versions[found->version] = {data, intent};
          environment[found->owner] = found->version;
          originalWriters[found->owner] = true;
          if (failed(markTouched(found->owner))) return failure();
          return success();
    };
    std::function<LogicalResult(Block &, ac::InvocationFrameId,
                                SmallVectorImpl<bool> &,
                                SmallVectorImpl<ac::StateOwnerId> &, unsigned)>
        statements;
    statements = [&](Block &source, ac::InvocationFrameId frame,
                     SmallVectorImpl<bool> &touched,
                     SmallVectorImpl<ac::StateOwnerId> &touchOrder,
                     unsigned depth) -> LogicalResult {
      if (depth > 128 ||
          !charge(budget, transaction,
                  1 + SourceCheckBudget::bytes(sizeof(frame) + sizeof(depth) +
                                               sizeof(Block *))))
        return reject("original branch proof depth/budget exceeded");
      auto touch = [&](ac::StateOwnerId owner) -> LogicalResult {
        if (!charge(budget, transaction))
          return failure();
        if (!touched[owner]) {
          if (!prepayAppend(budget, transaction, touchOrder))
            return failure();
          touchOrder.push_back(owner);
          touched[owner] = true;
        }
        return success();
      };
      auto isNativeQueueCallee = [](ac::SourceMethodCallOp call) {
        auto symbol = call.getCalleeAttr();
        if (!symbol || symbol.getNestedReferences().size() != 1)
          return false;
        auto root = symbol.getRootReference().getValue();
        auto leaf = symbol.getLeafReference().getValue();
        return (root == "pycircuit.__builtins__.fifo_local" ||
                root == "pycircuit.__builtins__.fifo_downstream_pop") &&
               (leaf == "push" || leaf == "pop" || leaf == "peek");
      };
      auto isNativeMemoryCallee = [](ac::SourceMethodCallOp call) {
        auto symbol = call.getCalleeAttr();
        if (!symbol || symbol.getNestedReferences().size() != 1) return false;
        auto root = symbol.getRootReference().getValue();
        return root == "pycircuit.__builtins__.sync_mem" ||
               root == "pycircuit.__builtins__.sync_mem_dp" ||
               root == "pycircuit.__builtins__.byte_mem";
      };
      for (Operation &op : source.without_terminator()) {
        if (!charge(budget, &op, 8))
          return failure();
        if (isa<ac::SourceQueuePopOp, ac::SourceQueuePeekOp,
                ac::SourceQueuePushOp>(op)) {
          return op.emitOpError()
                 << "retired dedicated queue effects are not admitted";
        } else if (auto call = dyn_cast<ac::SourceMethodCallOp>(op);
                   call && (isNativeQueueCallee(call) || isNativeMemoryCallee(call))) {
          auto payModule = [&](Operation *module, Operation *) {
            return prepayClone(module, budget);
          };
          auto payMethod = [&](ac::SourceMethodOp method, Operation *) {
            return prepayClone(method, budget);
          };
          auto payWorkStorage = [&](Operation *site, uint64_t work, uint64_t bytes) {
            return success(charge(budget, site, work) &&
                           charge(budget, site, SourceCheckBudget::bytes(bytes)));
          };
          auto payInstanceBinding = [&](Operation *owner, Operation *allocation,
                                        Operation *target, Operation *) {
            return success(succeeded(prepayClone(allocation ? allocation : owner, budget)) &&
                           prepayOperationHeader(target, budget));
          };
          ac::SourceReceiverProofFees fees{payModule, payMethod, payWorkStorage,
                                          payInstanceBinding};
          auto resolved = ac::resolveSourceMethodCall(
              call, plan.frames[frame].bindings,
              [&](FlatSymbolRefAttr symbol) -> Operation * {
                return hardware.lookupDefinition(symbol.getValue());
              }, &receiverProof, &fees);
          if (failed(resolved) ||
              !charge(budget, call,
                      32 + resolved->bindings.integers.size() +
                          resolved->bindings.types.size()) ||
              !resolved->method.isNativeTerminal())
            return reject("original native terminal resolution differs");
          if (resolved->method.isNativeMemoryTerminal()) {
#include "TransactionMemorySourceReplay.inc"
          } else {
#include "TransactionQueueSourceReplay.inc"
          }
        } else if (auto read = dyn_cast<ac::SourceStateReadOp>(op)) {
          if (readIndex == plan.reads.size())
            return reject("original read inventory differs");
          auto found = plan.reads.begin() + readIndex++;
          if (found->frame != frame || found->operation != read ||
              found->owner >= environment.size() ||
              found->version != environment[found->owner] ||
              found->selectors != read.getSelectors() ||
              bound(found->authoredPath) !=
                  authenticated[frame].lookupOrNull(read.getPath()) ||
              failed(ownerProof(read, frame, found->owner, read.getPath(),
                                read.getValue(), read.getSelectors(),
                                read.getIndices(), found->occurrence)) ||
              !versions[found->version].data ||
              record.frames[frame].lookupOrNull(read.getValue()) !=
                  versions[found->version].data ||
              failed(event(ac::TransactionStateEventKind::Read, frame,
                           readIndex - 1)))
            return reject("original read selected version differs");
          if (failed(replay.identify(found->authoredPath, read.getPath(),
                                     plan.frames[frame].anchor, frame)) ||
              failed(replay.authoredTrue(found->authoredPath, point())) ||
              failed(replay.identify(found->result, read.getValue(),
                                     plan.frames[frame].anchor, frame)) ||
              failed(replay.requireVersionAvailable(
                  found->version, versionValidity[found->version], point())) ||
              failed(replay.assignValidity(found->result, point())))
            return failure();
          if (failed(touch(found->owner)))
            return failure();
          authenticated[frame].map(read.getValue(),
                                   versions[found->version].data);
        } else if (auto write = dyn_cast<ac::SourceStateWriteOp>(op)) {
          if (failed(physicalWrite(write, frame, write.getPath(), write.getValue(),
                                   write.getSelectors(), write.getIndices(), false, touch)))
            return failure();
        } else if (auto formal = dyn_cast<ac::SourceFormalWriteOp>(op)) {
          if (formalIndex == plan.formalWrites.size()) return reject("original formal marker inventory differs");
          const auto &recorded = plan.formalWrites[formalIndex];
          unsigned ordinal = formal.getFormalAttr().getInt();
          auto anchor = plan.frames[frame].anchor;
          if (ordinal >= replayFormalValues[frame].size()) return failure();
          if (failed(ac::detail::verifySourceFormalCandidateSelection(formal, hardware,
                plan.frames[frame].bindings,
                [&](Value previous) -> FailureOr<bool> {
                  auto bound = replay.find(previous, anchor, frame);
                  if (failed(bound)) return failure();
                  return replayFormalProof->equal(*bound, replayFormalValues[frame][ordinal], point());
                }, [&](uint64_t work) { return success(charge(budget, formal, work)); }))) return failure();
          if (recorded.operation != formal || recorded.frame != frame || recorded.formal != ordinal ||
              ordinal >= replayFormalOwners[frame].size() ||
              recorded.owner != replayFormalOwners[frame][ordinal] ||
              !sameSourcePoint(recorded.point, point()) ||
              recorded.occurrence != formal.getOccurrence() ||
              recorded.location != formal.getSourceLocation() ||
              recorded.selectors != formal.getSelectors() ||
              recorded.indices.size() != formal.getIndices().size() ||
              failed(replay.identify(recorded.payload, formal.getPayload(), anchor, frame)) ||
              failed(replay.identify(recorded.authoredPath, formal.getPath(), anchor, frame)) ||
              failed(replay.identify(recorded.candidate, formal.getCandidate(), anchor, frame)) ||
              failed(replay.requireAvailable(recorded.payload, point())) ||
              failed(replay.requireAvailable(recorded.authoredPath, point())) ||
              failed(replay.requireAvailable(recorded.candidate, point())))
            return reject("original formal payload/mode/provenance differs");
          for (auto [index, actual] : llvm::enumerate(formal.getIndices()))
            if (failed(replay.identify(recorded.indices[index], actual, anchor, frame)) ||
                failed(replay.requireAvailable(recorded.indices[index], point())))
              return reject("original formal selected index differs");
          if (recorded.owner) {
            if (!recorded.write || *recorded.write != writeIndex ||
                plan.writes[*recorded.write].owner != *recorded.owner ||
                failed(physicalWrite(formal, frame, formal.getPath(), formal.getCandidate(),
                                     formal.getSelectors(), formal.getIndices(), true, touch)))
              return reject("original formal physical candidate advance differs");
          } else if (recorded.write) return reject("by-value formal acquired a physical candidate");
          if (Value previous = formal.getPrevious()) {
            auto proposal = formal.getPayload().getDefiningOp<ac::SourceMethodCallOp>();
            bool matchedProposal = false;
            if (proposal && proposal->getBlock() == formal->getBlock()) {
              const ac::TransactionCallBinding *priorCall = nullptr;
              for (uint64_t call = 0; call < callIndex; ++call) {
                if (!charge(budget, formal)) return failure();
                if (plan.calls[call].operation == proposal && plan.calls[call].caller == frame)
                  priorCall = &plan.calls[call];
              }
              if (priorCall) {
                const auto &callee = plan.frames[priorCall->callee];
                ac::SourceMethodView view{plan.instances[callee.anchor].definition, callee.method, {}};
                auto mappings = proposal->getAttrOfType<ArrayAttr>("ac.formal_owner_bindings");
                auto proposals = view.getFormalProposals();
                if (mappings && proposals) for (Attribute raw : mappings) {
                  if (!charge(budget, formal)) return failure();
                  auto mapping = cast<DictionaryAttr>(raw);
                  if (mapping.get("caller_formal") != formal.getFormalAttr()) continue;
                  auto found = llvm::find(proposals, mapping.get("formal"));
                  if (found == proposals.end()) return failure();
                  unsigned base = view.getNumPublicResults() + 2 * std::distance(proposals.begin(), found);
                  matchedProposal = proposal->getResult(base) == formal.getPayload() &&
                                    proposal->getResult(base + 1) == formal.getPath();
                }
              }
            }
            if (!matchedProposal) return reject("original forwarded formal is not its bound callee proposal");
            if (!recorded.previous || !recorded.snapshot ||
                failed(replay.identify(*recorded.previous, previous, anchor, frame)) ||
                failed(replay.identify(*recorded.snapshot, formal.getSnapshot(), anchor, frame)))
              return reject("original forwarded formal snapshot identity differs");
            auto samePrevious = replayFormalProof->equal(*recorded.previous,
                replayFormalValues[frame][ordinal], point());
            if (failed(samePrevious) || !*samePrevious)
              return reject("original forwarded formal previous candidate differs");
            auto selected = recorded.owner ? recorded.candidate : *recorded.previous;
            Value mapped = bound(selected);
            if (!mapped || record.frames[frame].lookupOrNull(formal.getSnapshot()) != mapped ||
                failed(replay.snapshot(*recorded.snapshot, selected, point())))
              return reject("original formal snapshot static mode differs");
            authenticated[frame].map(formal.getSnapshot(), mapped);
            replayFormalValues[frame][ordinal] = *recorded.snapshot;
          } else {
            if (recorded.previous || recorded.snapshot) return reject("original direct formal acquired a snapshot");
            replayFormalValues[frame][ordinal] = recorded.candidate;
          }
          auto pathFact = replay.fact(recorded.authoredPath, point());
          if (failed(pathFact) || !replayFormalProof || ordinal >= replayFormalIntents[frame].size())
            return failure();
          auto intent = replayFormalProof->write(replayFormalIntents[frame][ordinal],
              recorded.authoredPath, *pathFact, bool(formal.getPrevious()));
          if (failed(intent)) return failure();
          replayFormalIntents[frame][ordinal] = *intent;
          if (failed(event(ac::TransactionStateEventKind::FormalWrite, frame, formalIndex++))) return failure();
        } else if (auto split = dyn_cast<ac::SourceBranchOp>(op)) {
          if (branchIndex == plan.branches.size() ||
              split->getNumRegions() != 2 ||
              !split.getThenRegion().hasOneBlock() ||
              !split.getElseRegion().hasOneBlock())
            return reject("original branch inventory/regions differ");
          ac::BranchId id = branchIndex++;
          const auto &branch = plan.branches[id];
          const auto &conditionRef = plan.values[branch.condition];
          Value condition =
              authenticated[frame].lookupOrNull(split.getCondition());
          if (!condition || branch.operation != split ||
              split.getOccurrence() != branch.occurrence ||
              split.getConditionLocation() != branch.conditionLocation)
            return reject("original branch source metadata differs");
          if (branch.frame != frame ||
              !sameSourcePoint(branch.entry, point()) ||
              conditionRef.value != split.getCondition() ||
              bound(branch.condition) != condition ||
              failed(
                  event(ac::TransactionStateEventKind::BranchEnter, frame, id)))
            return reject("original branch selector/frame/entry differs");
          auto resolvedCondition = hardware.resolveType(
              split.getCondition().getType(),
              plan.instances[conditionRef.anchor].bindings, split);
          if (failed(resolvedCondition) ||
              !ac::areEquivalentHardwareTypes(*resolvedCondition,
                                              condition.getType()))
            return reject("original branch selector type differs");
          auto width = hardware.getPackedWidth(
              *resolvedCondition, plan.instances[conditionRef.anchor].bindings,
              split);
          if (failed(width) || *width != 1)
            return reject("original branch selector width differs");
          if (failed(replay.identify(branch.condition, split.getCondition(),
                                     plan.frames[frame].anchor, frame)) ||
              failed(replay.requireAvailable(branch.condition, point())) ||
              failed(replay.branch(id, sourcePath)))
            return failure();
          auto conditionFact = replay.fact(branch.condition, point());
          if (failed(conditionFact) || *conditionFact != branch.conditionFact)
            return reject("original branch predicate fact differs");
          bool effectful = false;
          SmallVector<Operation *, 0> pending;
          if (!prepayAppend(budget, split, pending))
            return failure();
          pending.push_back(split);
          while (!pending.empty()) {
            Operation *nested = pending.pop_back_val();
            if (!charge(budget, split))
              return failure();
            effectful |= isa<ac::SourceMethodCallOp, ac::SourceExpectOp,
                             ac::SourceObserveOp, ac::SourceQueuePopOp,
                             ac::SourceQueuePeekOp, ac::SourceQueuePushOp>(nested) ||
                         nested->hasAttr("ac.loop_continuation");
            for (Region &region : nested->getRegions())
              for (Block &block : region)
                for (Operation &child : block) {
                  if (!prepayAppend(budget, split, pending))
                    return failure();
                  pending.push_back(&child);
                }
          }
          if (bool(branch.selection) != effectful)
            return reject("original branch effect classification differs");
          if (effectful) {
            if (*branch.selection != selectionIndex ||
                selectionIndex >= plan.selections.size())
              return reject("original selection inventory differs");
            const auto &selection = plan.selections[selectionIndex];
            if (selection.branch != id || selection.control != admission ||
                admission >= plan.controls.size())
              return reject("original selection boundary differs");
            const auto &control = plan.controls[admission];
            auto *tag = std::get_if<ac::BranchSelectControl>(&control.boundary);
            if (!tag || tag->selection != selectionIndex ||
                !sameSourcePath(control.path, sourcePath) ||
                failed(event(ac::TransactionStateEventKind::SelectionEnter,
                             frame, selectionIndex)))
              return reject("original selection control differs");
            sourceRaw[admission++] = condition;
            if (failed(event(ac::TransactionStateEventKind::SelectionExit,
                             frame, selectionIndex)))
              return failure();
            ++selectionIndex;
          }
          auto parentPath = sourcePath;
          sourcePath = {id, true};
          if (failed(
                  event(ac::TransactionStateEventKind::ThenEnter, frame, id)))
            return failure();
          if (!product(budget, split, environment.size(),
                       3 * (1 + SourceCheckBudget::bytes(
                                    sizeof(ac::StateVersionId))) +
                           1 + SourceCheckBudget::bytes(sizeof(bool))))
            return failure();
          SmallVector<ac::StateVersionId, 0> entry(environment);
          if (!product(budget, split, replayFormalValues[frame].size(),
                       3 * (1 + SourceCheckBudget::bytes(sizeof(ac::BoundValueId))))) return failure();
          SmallVector<ac::BoundValueId, 0> formalEntry(replayFormalValues[frame]);
          if (!product(budget, split, replayFormalIntents[frame].size(),
                       3 * (1 + SourceCheckBudget::bytes(sizeof(uint64_t))))) return failure();
          SmallVector<uint64_t, 0> intentEntry(replayFormalIntents[frame]);
          SmallVector<bool, 0> armTouched(environment.size(), false);
          SmallVector<ac::StateOwnerId, 0> armOrder;
          if (failed(statements(split.getThenRegion().front(), frame,
                                armTouched, armOrder, depth + 1)) ||
              failed(event(ac::TransactionStateEventKind::ThenExit, frame, id)))
            return failure();
          if (branch.thenExitPrefix != admission)
            return reject("original then exit prefix differs");
          SmallVector<ac::StateVersionId, 0> whenTrue(environment);
          SmallVector<ac::BoundValueId, 0> formalTrue(replayFormalValues[frame]);
          SmallVector<uint64_t, 0> intentTrue(replayFormalIntents[frame]);
          replayFormalValues[frame] = formalEntry;
          replayFormalIntents[frame] = intentEntry;
          if (!charge(budget, split, environment.size()))
            return failure();
          llvm::copy(entry, environment.begin());
          sourcePath = {id, false};
          if (failed(
                  event(ac::TransactionStateEventKind::ElseEnter, frame, id)) ||
              failed(statements(split.getElseRegion().front(), frame,
                                armTouched, armOrder, depth + 1)) ||
              failed(event(ac::TransactionStateEventKind::ElseExit, frame, id)))
            return failure();
          if (branch.elseExitPrefix != admission)
            return reject("original else exit prefix differs");
          sourcePath = parentPath;
          uint64_t touchedIndex = 0, branchJoinIndex = 0;
          for (auto [owner, wasTouched] : llvm::enumerate(armTouched)) {
            if (!charge(budget, split))
              return failure();
            if (!wasTouched)
              continue;
            if (failed(touch(owner)))
              return failure();
            if (touchedIndex >= branch.entryVersions.size() ||
                touchedIndex >= branch.thenVersions.size() ||
                touchedIndex >= branch.elseVersions.size())
              return reject("original branch candidate inventory differs");
            auto expectedBinding = [&](const auto &binding,
                                       ac::StateVersionId value) {
              return binding.owner == owner && binding.version == value;
            };
            if (!expectedBinding(branch.entryVersions[touchedIndex],
                                 entry[owner]) ||
                !expectedBinding(branch.thenVersions[touchedIndex],
                                 whenTrue[owner]) ||
                !expectedBinding(branch.elseVersions[touchedIndex],
                                 environment[owner]))
              return reject("original branch arm entry/exit versions differ");
            ++touchedIndex;
            if (whenTrue[owner] == entry[owner] &&
                environment[owner] == entry[owner])
              continue;
            if (branchJoinIndex == branch.joins.size() ||
                joinIndex == plan.joins.size() ||
                branch.joins[branchJoinIndex++] != joinIndex)
              return reject("original state join inventory differs");
            const auto &join = plan.joins[joinIndex];
            if (join.branch != id || join.owner != owner ||
                join.entry != entry[owner] ||
                join.whenTrue != whenTrue[owner] ||
                join.whenFalse != environment[owner] ||
                join.result >= plan.versions.size())
              return reject("original state join inputs differ");
            const auto &version = plan.versions[join.result];
            if (version.transaction != transaction ||
                version.kind != ac::StateVersionKind::Join || version.value ||
                version.write || version.join != joinIndex ||
                version.owner != owner || version.predecessor != entry[owner] ||
                version.ordinal != ++ordinals[owner] ||
                join.whenTrue >= join.result || join.whenFalse >= join.result ||
                !ac::areEquivalentHardwareTypes(version.payloadType,
                                                plan.owners[owner].payloadType))
              return reject("original state join version differs");
            if (failed(replay.requireVersionAvailable(
                    join.entry, versionValidity[join.entry], branch.entry)) ||
                failed(replay.requireVersionAvailable(
                    join.whenTrue, versionValidity[join.whenTrue],
                    {branch.thenExitPrefix, {id, true}})) ||
                failed(replay.requireVersionAvailable(
                    join.whenFalse, versionValidity[join.whenFalse],
                    {branch.elseExitPrefix, {id, false}})) ||
                !sameSourcePoint(version.validity, point()))
              return failure();
            versionValidity[join.result] = point();
            const auto &actual = record.versions[join.result];
            if (record.stateJoins[joinIndex].value != actual.data ||
                failed(localBranchProof(split, plan.owners[owner].payloadType,
                    plan.frames[frame].bindings, condition,
                    versions[join.whenTrue].data, versions[join.whenFalse].data,
                    record.stateJoins[joinIndex], &rule.getBody().front(), last)) ||
                failed(selectProof(actual.rawIntent, condition,
                                   versions[join.whenTrue].rawIntent,
                                   versions[join.whenFalse].rawIntent, bit,
                                   split.getLoc())) ||
                failed(event(ac::TransactionStateEventKind::StateJoin, frame,
                             joinIndex)))
              return failure();
            versions[join.result] = actual;
            environment[owner] = join.result;
            ++joinIndex;
          }
          if (touchedIndex != branch.entryVersions.size() ||
              touchedIndex != branch.thenVersions.size() ||
              touchedIndex != branch.elseVersions.size() ||
              branchJoinIndex != branch.joins.size() ||
              branch.results.size() != split.getNumResults())
            return reject("original branch arm/join/result inventory differs");
          auto trueYield =
              cast<ac::YieldOp>(split.getThenRegion().front().back());
          auto falseYield =
              cast<ac::YieldOp>(split.getElseRegion().front().back());
          if (trueYield.getNumOperands() != split.getNumResults() ||
              falseYield.getNumOperands() != split.getNumResults())
            return reject("original branch yield arity differs");
          for (auto [ordinal, original] : llvm::enumerate(split.getResults())) {
            if (!charge(budget, split, 8) ||
                resultIndex == plan.branchResults.size() ||
                branch.results[ordinal] != resultIndex)
              return reject("original branch result inventory differs");
            const auto &result = plan.branchResults[resultIndex];
            Value trueValue = authenticated[frame].lookupOrNull(
                trueYield.getOperand(ordinal));
            Value falseValue = authenticated[frame].lookupOrNull(
                falseYield.getOperand(ordinal));
            const auto &actual = record.branchResults[resultIndex];
            const auto &resultRef = plan.values[result.result];
            auto type = branchTypes.admit(
                original.getType(), hardware,
                plan.instances[resultRef.anchor].bindings, split, budget);
            if (result.branch != id || result.frame != frame ||
                result.ordinal != ordinal ||
                plan.values[result.result].value != original ||
                !sameSourcePoint(result.validity, point()) || failed(type) ||
                !ac::areEquivalentHardwareTypes(result.payloadType, *type) ||
                !actual.value || !values.contains(actual.value) ||
                trueValue != bound(result.whenTrue) ||
                falseValue != bound(result.whenFalse) ||
                actual.value.getType() != result.payloadType ||
                record.frames[frame].lookupOrNull(original) != actual.value ||
                failed(
                    localBranchProof(split, *type,
                                     plan.instances[resultRef.anchor].bindings,
                                     condition, trueValue, falseValue, actual,
                                     &rule.getBody().front(), last)) ||
                failed(event(ac::TransactionStateEventKind::BranchResult, frame,
                             resultIndex)))
              return reject("original branch yielded-result binding differs");
            if (failed(replay.identify(result.result, original,
                                       plan.frames[frame].anchor, frame)) ||
                failed(replay.identify(result.whenTrue,
                                       trueYield.getOperand(ordinal),
                                       plan.frames[frame].anchor, frame)) ||
                failed(replay.identify(result.whenFalse,
                                       falseYield.getOperand(ordinal),
                                       plan.frames[frame].anchor, frame)) ||
                failed(replay.requireAvailable(
                    result.whenTrue, {branch.thenExitPrefix, {id, true}})) ||
                failed(replay.requireAvailable(
                    result.whenFalse, {branch.elseExitPrefix, {id, false}})) ||
                failed(replay.assignValidity(result.result, point())))
              return failure();
            if (!prepayValueMapping(budget, split, authenticated[frame],
                                    original))
              return failure();
            authenticated[frame].map(original, actual.value);
            ++resultIndex;
          }
          for (unsigned formal = 0; formal < intentEntry.size(); ++formal) {
            auto joined = replayFormalProof->join(id, intentTrue[formal], replayFormalIntents[frame][formal]);
            if (failed(joined)) return failure();
            replayFormalIntents[frame][formal] = *joined;
          }
          for (unsigned formal = 0; formal < formalEntry.size(); ++formal) {
            auto trueId = formalTrue[formal], falseId = replayFormalValues[frame][formal];
            if (trueId == falseId) { replayFormalValues[frame][formal] = trueId; continue; }
            std::optional<ac::BoundValueId> joined;
            for (auto resultId : branch.results) {
              if (!charge(budget, split)) return failure();
              const auto &result = plan.branchResults[resultId];
              auto trueArm = replayFormalProof->equal(result.whenTrue, trueId,
                  {branch.thenExitPrefix, {id, true}});
              auto falseArm = replayFormalProof->equal(result.whenFalse, falseId,
                  {branch.elseExitPrefix, {id, false}});
              if (failed(trueArm) || failed(falseArm)) return failure();
              if (*trueArm && *falseArm && !joined) joined = result.result;
            }
            if (!joined) return reject("original current formal branch join is absent");
            replayFormalValues[frame][formal] = *joined;
          }
          if (failed(
                  event(ac::TransactionStateEventKind::BranchExit, frame, id)))
            return failure();
        } else if (auto call = dyn_cast<ac::SourceMethodCallOp>(op)) {
          if (callIndex == plan.calls.size())
            return reject("original call inventory differs");
          auto found = plan.calls.begin() + callIndex++;
          if (found->caller != frame || found->operation != call ||
              found->callee != nextFrame++ ||
              found->callee >= plan.frames.size())
            return reject("original call frame order differs");
          const auto &calleeFrame = plan.frames[found->callee];
          auto calleeMethod = calleeFrame.method;
          auto callerAnchor = plan.frames[frame].anchor;
          auto payModule = [&](Operation *module, Operation *) {
            return prepayClone(module, budget);
          };
          auto payMethod = [&](ac::SourceMethodOp method, Operation *) {
            return prepayClone(method, budget);
          };
          auto payWorkStorage = [&](Operation *site, uint64_t work, uint64_t bytes) {
            return success(charge(budget, site, work) &&
                           charge(budget, site, SourceCheckBudget::bytes(bytes)));
          };
          auto payInstanceBinding = [&](Operation *owner, Operation *allocation,
                                        Operation *target, Operation *) {
            return success(succeeded(prepayClone(allocation ? allocation : owner, budget)) &&
                           prepayOperationHeader(target, budget));
          };
          ac::SourceReceiverProofFees fees{payModule, payMethod, payWorkStorage,
                                          payInstanceBinding};
          auto resolved = ac::resolveSourceMethodCall(
              call, plan.frames[frame].bindings,
              [&](FlatSymbolRefAttr symbol) -> Operation * {
                return hardware.lookupDefinition(symbol.getValue());
              }, &receiverProof, &fees);
          if (failed(resolved) ||
              !charge(budget, call,
                      32 + resolved->bindings.integers.size() +
                          resolved->bindings.types.size()) ||
              !resolved->method.definition ||
              calleeFrame.method != resolved->method.definition ||
              calleeFrame.transaction != transaction ||
              calleeFrame.parent != frame || calleeFrame.call != call ||
              calleeFrame.registrationEntry ||
              calleeFrame.callOccurrence != call.getOccurrence() ||
              calleeFrame.anchor >= plan.instances.size() ||
              !sameBindings(calleeFrame.bindings, resolved->bindings) ||
              plan.instances[calleeFrame.anchor].definition.operator->() !=
                  resolved->method.module)
            return reject("original resolved Method frame differs");
          auto receiverPath =
              resolved->receiver.getAs<ArrayAttr>("instance_path");
          if (!receiverPath)
            return reject("original Method receiver path is absent");
          auto expectedAnchorPath = plan.instances[callerAnchor].occurrencePath;
          SmallVector<Attribute, 0> fullPath;
          uint64_t fullPathSize = SourceCheckBudget::add(
              expectedAnchorPath.size(), receiverPath.size());
          if (fullPathSize > fullPath.max_size() ||
              !product(budget, call, fullPathSize,
                       1 + SourceCheckBudget::bytes(sizeof(Attribute))))
            return failure();
          fullPath.reserve(fullPathSize);
          fullPath.append(expectedAnchorPath.begin(), expectedAnchorPath.end());
          fullPath.append(receiverPath.begin(), receiverPath.end());
          auto receiver =
              Builder(transaction.getContext())
                  .getDictionaryAttr({NamedAttribute(
                      StringAttr::get(transaction.getContext(),
                                      "instance_path"),
                      ArrayAttr::get(transaction.getContext(), fullPath))});
          if (calleeFrame.receiver != receiver ||
              plan.instances[calleeFrame.anchor].occurrencePath !=
                  receiver.getAs<ArrayAttr>("instance_path"))
            return reject("original Method receiver identity differs");
          if (failed(replay.identify(found->authoredPath, call.getPath(),
                                     callerAnchor, frame)) ||
              failed(replay.authoredTrue(found->authoredPath, point())) ||
              found->arguments.size() != call.getArguments().size() ||
              found->captures.size() != calleeMethod.getCaptures().size())
            return failure();
          for (auto [ordinal, argument] : llvm::enumerate(found->arguments))
            if (failed(replay.identify(argument, call.getArguments()[ordinal],
                                       callerAnchor, frame)) ||
                failed(replay.requireAvailable(argument, point())))
              return failure();
          for (auto [ordinal, capture] : llvm::enumerate(found->captures))
            if (failed(replay.identify(capture,
                                       calleeMethod.getCaptures()[ordinal],
                                       calleeFrame.anchor, plan.rootFrame)) ||
                failed(replay.requireAvailable(capture, point())))
              return failure();
          if (!product(
                  budget, call, environment.size(),
                  1 + SourceCheckBudget::bytes(sizeof(ac::StateVersionId))))
            return failure();
          SmallVector<ac::StateVersionId, 0> entry(environment);
          SmallVector<ac::StateOwnerId, 0> calleeTouched;
          if (failed(event(ac::TransactionStateEventKind::CallEnter, frame,
                           callIndex - 1)) ||
              failed(invocation(found->callee, calleeTouched)))
            return failure();
          if (found->entryVersions.size() != calleeTouched.size() ||
              found->exitVersions.size() != calleeTouched.size())
            return reject("original call candidate snapshot inventory differs");
          for (auto [ordinal, owner] : llvm::enumerate(calleeTouched)) {
            if (!charge(budget, call, 4) ||
                found->entryVersions[ordinal].owner != owner ||
                found->entryVersions[ordinal].version != entry[owner] ||
                found->exitVersions[ordinal].owner != owner ||
                found->exitVersions[ordinal].version != environment[owner] ||
                failed(touch(owner)))
              return reject("original call entry/exit candidates differ");
          }
          auto yielded =
              cast<ac::YieldOp>(calleeMethod.getBody().front().getTerminator());
          if (found->results.size() != yielded.getNumOperands() ||
              found->results.size() != call.getNumResults())
            return reject("original call result arity differs");
          for (auto [ordinal, result] : llvm::enumerate(found->results)) {
            Value value = bound(result.yielded),
                  source = call.getResult(ordinal);
            if (failed(replay.identify(result.result, source, callerAnchor,
                                       frame)) ||
                failed(replay.identify(result.yielded,
                                       yielded.getOperand(ordinal),
                                       calleeFrame.anchor, found->callee)) ||
                failed(replay.result(result.result, result.yielded,
                                     calleeFrame.exit)) ||
                value != authenticated[found->callee].lookupOrNull(
                             yielded.getOperand(ordinal)) ||
                !value || record.frames[frame].lookupOrNull(source) != value)
              return reject(
                  "original call yielded-result identity/validity differs");
            authenticated[frame].map(source, value);
          }
          if (failed(event(ac::TransactionStateEventKind::CallExit, frame,
                           std::distance(plan.calls.begin(), found))))
            return failure();
        } else if (auto check = dyn_cast<ac::SourceExpectOp>(op)) {
          if (!charge(budget, check, plan.bodyChecks.size()))
            return failure();
          auto found = llvm::find_if(plan.bodyChecks, [&](const auto &item) {
            return item.frame == frame && item.operation.operator->() == &op;
          });
          if (found == plan.bodyChecks.end() ||
              nextEffectEvent == effectEvents.size())
            return reject("original body-check plan/event is absent");
          auto id = std::distance(plan.bodyChecks.begin(), found);
          const auto *effectEvent = effectEvents[nextEffectEvent++];
          Value condition = bound(found->condition),
                path = bound(found->authoredPath);
          if (effectEvent->kind != ac::TransactionStateEventKind::BodyCheck ||
              effectEvent->frame != frame || effectEvent->bodyCheck != id ||
              !condition || !path || check.getKind() != "assert" ||
              !check.getSampling().empty() ||
              check->getAttr("occurrence") != found->occurrence ||
              check.getLocationAttr() != found->location ||
              check->getAttrOfType<StringAttr>("ac.message") !=
                  found->message ||
              check->hasAttr("ac.check_id") ||
              check->hasAttr("ac.observation_id") ||
              check.getCondition() != plan.values[found->condition].value ||
              check.getPath() != plan.values[found->authoredPath].value)
            return reject("original body-check operation/frame/inputs differ");
          if (!sameSourcePoint(found->point, point()) ||
              failed(
                  event(ac::TransactionStateEventKind::BodyCheck, frame, id)))
            return reject("original body-check placement/event differs");
          if (failed(replay.identify(found->condition, check.getCondition(),
                                     plan.frames[frame].anchor, frame)) ||
              failed(replay.requireAvailable(found->condition, point())) ||
              failed(replay.identify(found->authoredPath, check.getPath(),
                                     plan.frames[frame].anchor, frame)) ||
              failed(replay.authoredTrue(found->authoredPath, point())))
            return failure();
          sourceChecks[id] = {condition, path};
        } else if (auto observation = dyn_cast<ac::SourceObserveOp>(op)) {
          if (!charge(budget, observation, plan.observations.size()))
            return failure();
          auto found = llvm::find_if(plan.observations, [&](const auto &item) {
            return item.frame == frame && item.operation.operator->() == &op;
          });
          if (found == plan.observations.end() ||
              nextEffectEvent == effectEvents.size())
            return reject("original observation plan/event is absent");
          auto id = std::distance(plan.observations.begin(), found);
          const auto *effectEvent = effectEvents[nextEffectEvent++];
          Value path = bound(found->authoredPath);
          if (effectEvent->kind != ac::TransactionStateEventKind::Observe ||
              effectEvent->frame != frame || effectEvent->observation != id ||
              !path ||
              observation->getAttr("occurrence") != found->occurrence ||
              observation->getAttr("ac.location") != found->location ||
              observation.getKindAttr() != found->kind ||
              observation.getSpec() != found->spec ||
              observation->hasAttr("ac.check_id") ||
              observation->hasAttr("ac.observation_id") ||
              observation.getPath() != plan.values[found->authoredPath].value ||
              observation.getValues().size() != found->values.size())
            return reject("original observation operation/frame/path differ");
          if (!sameSourcePoint(found->point, point()) ||
              failed(event(ac::TransactionStateEventKind::Observe, frame, id)))
            return reject("original observation placement/event differs");
          if (failed(replay.identify(found->authoredPath, observation.getPath(),
                                     plan.frames[frame].anchor, frame)) ||
              failed(replay.authoredTrue(found->authoredPath, point())))
            return failure();
          sourceObservations[id].path = path;
          sourceObservations[id].values.reserve(found->values.size());
          for (auto [ordinal, value] : llvm::enumerate(found->values)) {
            Value mapped = bound(value);
            if (!mapped ||
                observation.getValues()[ordinal] != plan.values[value].value)
              return reject("original observation payload/order differs");
            if (failed(replay.identify(value, observation.getValues()[ordinal],
                                       plan.frames[frame].anchor, frame)) ||
                failed(replay.requireAvailable(value, point())))
              return failure();
            sourceObservations[id].values.push_back(mapped);
          }
        } else {
          auto actual = record.frames[frame].lookupOrNull(&op);
          if (!actual || !operations.contains(actual) ||
              actual->getBlock() != &rule.getBody().front() ||
              (last && !last->isBeforeInBlock(actual)))
            return reject("original invocation clone parent/order differs");
          SmallVector<Operation *, 0> originalPayload;
          if (!prepayAppend(budget, &op, originalPayload))
            return failure();
          originalPayload.push_back(&op);
          while (!originalPayload.empty()) {
            auto *nested = originalPayload.pop_back_val();
            if (!charge(budget, nested,
                        3 + nested->getNumOperands() + nested->getNumResults()))
              return failure();
            // Validate the closed payload during the independent SSA replay,
            // without a second traversal of the same original regions.
            if (isa<ac::SourceExpectOp, ac::SourceObserveOp>(nested))
              return nested->emitOpError()
                     << "transaction body checks/observations require sampled effect lowering";
            if (!ac::detail::isCombinationalHardwareOperation(nested) &&
                !(nested != &op && isa<ac::YieldOp>(nested)))
              return nested->emitOpError()
                     << "transaction source proof requires closed pure payload operations";
            for (Value operand : nested->getOperands()) {
              auto id = replay.find(operand, plan.frames[frame].anchor, frame);
              if (failed(id) || failed(replay.requireAvailable(*id, point())))
                return failure();
            }
            for (Value value : nested->getResults()) {
              auto id = replay.find(value, plan.frames[frame].anchor, frame);
              if (failed(id) || failed(replay.assignValidity(*id, point())))
                return failure();
            }
            for (Region &region : llvm::reverse(nested->getRegions()))
              for (Block &block : llvm::reverse(region)) {
                for (BlockArgument argument : block.getArguments()) {
                  auto id =
                      replay.find(argument, plan.frames[frame].anchor, frame);
                  if (failed(id) || failed(replay.assignValidity(*id, point())))
                    return failure();
                }
                for (Operation &child : llvm::reverse(block)) {
                  if (!prepayAppend(budget, nested, originalPayload))
                    return failure();
                  originalPayload.push_back(&child);
                }
              }
          }
          if (failed(exactClone(&op, actual, authenticated[frame],
                                record.frames[frame])))
            return failure();
          last = actual;
        }
      }
      return success();
    };
    invocation =
        [&](ac::InvocationFrameId frame,
            SmallVectorImpl<ac::StateOwnerId> &touchOrder) -> LogicalResult {
      const auto &binding = plan.frames[frame];
      auto method = binding.method;
      Operation *function =
          method ? method.operator->() : transaction.operator->();
      const ac::TransactionCallBinding *callBinding = nullptr;
      ac::TransactionCallBinding registrationBinding;
      bool registrationRoot = frame == plan.rootFrame && bool(entryMethod);
      if (registrationRoot) {
        registrationBinding.operation = entryCall;
        registrationBinding.caller = plan.rootFrame;
        registrationBinding.callee = plan.rootFrame;
        if (!product(budget, transaction, (**registration).argumentCaptureOrdinals.size(),
                     2 + SourceCheckBudget::bytes(sizeof(ac::BoundValueId)))) return failure();
        for (unsigned ordinal : (**registration).argumentCaptureOrdinals) {
          auto actual = replay.find(transaction.getCaptures()[ordinal], 0, plan.rootFrame);
          if (failed(actual) || failed(replay.requireAvailable(*actual, {}))) return failure();
          registrationBinding.arguments.push_back(*actual);
        }
        callBinding = &registrationBinding;
      } else if (frame) {
        if (!charge(budget, transaction, plan.calls.size()))
          return failure();
        auto found = llvm::find_if(
            plan.calls, [&](const auto &call) { return call.callee == frame; });
        if (found == plan.calls.end())
          return reject("original frame has no call");
        callBinding = &*found;
      }
      auto proposalOrdinals = function->getAttrOfType<ArrayAttr>("ac.formal_proposals");
      if (proposalOrdinals && !proposalOrdinals.empty()) {
        if (!replayFormalProof) {
          if (!charge(budget, function, 1 + SourceCheckBudget::bytes(
                  sizeof(ac::detail::SourceFormalProposalProof)))) return failure();
          replayFormalProof = std::make_unique<ac::detail::SourceFormalProposalProof>(plan, hardware,
              [&](ac::BoundValueId id, ac::TransactionPoint context) { return replay.identity(id, context); },
              [&](Value source, ac::LexicalInstanceId anchor, ac::InvocationFrameId invocation) {
                return replay.find(source, anchor, invocation);
              }, [&](uint64_t work) { return charge(budget, transaction, work); });
        }
      }
#include "TransactionFormalBindingProof.inc"
      if (proposalOrdinals && !proposalOrdinals.empty()) {
        auto zero = replayFormalProof->zero();
        if (failed(zero) || !product(budget, function, explicitCount,
              1 + SourceCheckBudget::bytes(sizeof(uint64_t)))) return failure();
        replayFormalIntents[frame].assign(explicitCount, *zero);
      }
      for (unsigned region : {0u, 1u}) {
        ArrayRef<ac::TransactionValueBinding> arguments =
            region == 0
                ? ArrayRef<ac::TransactionValueBinding>(binding.guardArguments)
                : ArrayRef<ac::TransactionValueBinding>(binding.bodyArguments);
        auto formals = function->getRegion(region).front().getArguments();
        size_t explicitCount = callBinding ? callBinding->arguments.size() : 0;
        ValueRange captures =
            method ? method.getCaptures() : transaction.getCaptures();
        if (arguments.size() != formals.size() ||
            arguments.size() != explicitCount + captures.size())
          return reject("original formal binding arity differs");
        for (auto [ordinal, argument] : llvm::enumerate(arguments)) {
          auto formal = formals[ordinal];
          Value originalActual;
          ac::InvocationFrameId actualFrame = plan.rootFrame;
          auto actualAnchor = binding.anchor;
          if (ordinal < explicitCount) {
            auto originalCall = callBinding->operation;
            originalActual = registrationRoot
                ? transaction.getCaptures()[(**registration).argumentCaptureOrdinals[ordinal]]
                : originalCall.getArguments()[ordinal];
            actualFrame = callBinding->caller;
            actualAnchor = plan.frames[actualFrame].anchor;
          } else
            originalActual = captures[ordinal - explicitCount];
          auto expectedActual =
              replay.find(originalActual, actualAnchor, actualFrame);
          auto formalId = replay.find(formal, binding.anchor, frame);
          Value value = bound(argument.actual);
          if (failed(expectedActual) || failed(formalId) ||
              argument.formal != formal || *expectedActual != argument.actual ||
              !value ||
              failed(
                  replay.binding(*formalId, argument.actual, binding.entry)) ||
              record.frames[frame].lookupOrNull(formal) != value)
            return reject(
                "original formal actual ordinal/frame binding differs");
          authenticated[frame].map(formal, value);
        }
      }
      if (guardIndex == plan.guards.size())
        return reject("original guard inventory differs");
      uint64_t guardId = guardIndex++;
      const auto &guardPlan = plan.guards[guardId];
      ac::TransactionDomain invocationDomain = plan.domain;
      if (frame || registrationRoot) {
        auto definition = plan.instances[binding.anchor].definition;
        auto inputs =
            definition->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
        auto clk = inputs ? inputs.getAs<IntegerAttr>("clock") : IntegerAttr();
        auto rst = inputs ? inputs.getAs<IntegerAttr>("reset") : IntegerAttr();
        if (!inputs || inputs.size() != 2 || !clk || !rst || clk.getInt() < 0 ||
            rst.getInt() < 0 ||
            uint64_t(clk.getInt()) >=
                definition.getBody().front().getNumArguments() ||
            uint64_t(rst.getInt()) >=
                definition.getBody().front().getNumArguments())
          return reject("original callable domain inputs differ");
        auto clock =
            replay.find(definition.getBody().front().getArgument(clk.getInt()),
                        binding.anchor, 0);
        auto reset =
            replay.find(definition.getBody().front().getArgument(rst.getInt()),
                        binding.anchor, 0);
        if (failed(clock) || failed(reset))
          return failure();
        auto c = replay.endpoint(*clock, point()),
             r = replay.endpoint(*reset, point());
        if (failed(c) || failed(r))
          return failure();
        invocationDomain = {*c, *r};
      }
      if (!(invocationDomain == plan.domain) ||
          !(guardPlan.domain == invocationDomain))
        return reject("original callable domain binding differs");
      if (guardPlan.frame != frame || guardPlan.callable != function ||
          binding.guard != guardId || guardPlan.control != admission ||
          !sameSourcePoint(binding.entry, point()) ||
          admission >= plan.controls.size() ||
          !std::holds_alternative<ac::CallableGuardControl>(
              plan.controls[admission].boundary) ||
          std::get<ac::CallableGuardControl>(plan.controls[admission].boundary)
                  .guard != guardId ||
          !sameSourcePath(plan.controls[admission].path, sourcePath) ||
          failed(
              event(ac::TransactionStateEventKind::GuardEnter, frame, guardId)))
        return reject("original guard frame/event differs");
      if (!product(budget, transaction, plan.owners.size(),
                   1 + SourceCheckBudget::bytes(sizeof(bool))))
        return failure();
      SmallVector<bool, 0> touched(plan.owners.size(), false);
      uint64_t guardReadBegin = readIndex;
      for (auto [regionOrdinal, region] :
           llvm::enumerate(function->getRegions())) {
        if (failed(statements(region.front(), frame, touched, touchOrder, 0)))
          return failure();
        if (!regionOrdinal) {
          auto yielded = cast<ac::YieldOp>(region.front().getTerminator());
          Value predicate =
              authenticated[frame].lookupOrNull(yielded.getOperand(0));
          if (!predicate || predicate != bound(guardPlan.predicate))
            return reject("original guard predicate is absent");
          if (failed(replay.identify(guardPlan.predicate, yielded.getOperand(0),
                                     binding.anchor, frame)) ||
              failed(replay.requireAvailable(guardPlan.predicate, point())))
            return failure();
          auto guardFact = replay.fact(guardPlan.predicate, point());
          if (failed(guardFact) || *guardFact != guardPlan.predicateFact)
            return reject("original callable predicate fact differs");
          auto location =
              function->getAttrOfType<DictionaryAttr>("ac.guard_location");
          if (guardPlan.location != location)
            return reject("original guard span differs");
          for (auto binding : guardPlan.entryVersions) {
            if (!charge(budget, transaction) ||
                binding.owner >= environment.size() ||
                !touched[binding.owner] ||
                binding.version != environment[binding.owner])
              return reject("original guard entry candidate differs");
          }
          if (llvm::count(touched, true) != guardPlan.entryVersions.size())
            return reject("original guard entry candidate inventory differs");
          if (guardPlan.reads.size() != readIndex - guardReadBegin)
            return reject("original guard read inventory differs");
          for (auto [ordinal, read] : llvm::enumerate(guardPlan.reads))
            if (!charge(budget, transaction) ||
                read != guardReadBegin + ordinal)
              return reject("original guard read ordering differs");
          predicates.push_back(predicate);
          sourceRaw[admission++] = predicate;
          if (failed(event(ac::TransactionStateEventKind::GuardExit, frame,
                           guardId)))
            return failure();
        }
      }
      if (proposalOrdinals) {
        ac::SourceMethodView view{plan.instances[binding.anchor].definition, method, {}};
        auto yield = cast<ac::YieldOp>(function->getRegion(1).front().getTerminator());
        for (auto [ordinal, raw] : llvm::enumerate(proposalOrdinals)) {
          unsigned formal = cast<IntegerAttr>(raw).getInt();
          unsigned suffix = view.getNumPublicResults() + 2 * ordinal;
          if (!replayFormalProof || formal >= replayFormalIntents[frame].size() ||
              suffix + 1 >= yield.getNumOperands()) return failure();
          auto data = replay.find(yield.getOperand(suffix), binding.anchor, frame);
          auto intent = replay.find(yield.getOperand(suffix + 1), binding.anchor, frame);
          if (failed(data) || failed(intent)) return failure();
          auto same = replayFormalProof->equal(*data, replayFormalValues[frame][formal], point());
          if (failed(same) || !*same ||
              failed(replayFormalProof->check(*intent, replayFormalIntents[frame][formal], point())))
            return reject("original private formal proposal suffix differs from statement history");
        }
      }
      if (!sameSourcePoint(binding.exit, point()))
        return reject("original invocation exit differs");
      return success();
    };
    SmallVector<ac::StateOwnerId, 0> rootTouched;
    if (failed(event(ac::TransactionStateEventKind::TransactionEnter,
                     plan.rootFrame)) ||
        failed(invocation(plan.rootFrame, rootTouched)))
      return failure();
    Operation *rootCallable = entryMethod ? entryMethod.operator->() : transaction.operator->();
    auto originalYield = cast<ac::YieldOp>(rootCallable->getRegion(1).front().getTerminator());
    if (plan.results.size() != transaction->getNumResults() ||
        (entryMethod ? plan.results.size() > originalYield.getNumOperands()
                     : plan.results.size() != originalYield.getNumOperands()))
      return reject("original result inventory differs");
    for (auto [ordinal, result] : llvm::enumerate(plan.results))
      if (failed(replay.identify(result.result, transaction->getResult(ordinal),
                                 0, 0)) ||
          failed(replay.identify(result.yielded,
                                 originalYield.getOperand(ordinal), 0, 0)) ||
          failed(replay.result(result.result, result.yielded, point())))
        return failure();
    if (failed(event(ac::TransactionStateEventKind::TransactionExit,
                     plan.rootFrame)) ||
        failed(replay.complete()) ||
        failed(requireStaticTotality(plan, budget, prepared.valueAvailability)))
      return failure();
    if (eventIndex != plan.events.size() || readIndex != plan.reads.size() ||
        writeIndex != plan.writes.size() || formalIndex != plan.formalWrites.size() ||
        callIndex != plan.calls.size() || queueIndex != plan.queueAccesses.size() ||
        memoryIndex != plan.memoryAccesses.size() ||
        branchIndex != plan.branches.size() || joinIndex != plan.joins.size() ||
        resultIndex != plan.branchResults.size() ||
        guardIndex != plan.guards.size() ||
        selectionIndex != plan.selections.size() ||
        admission != plan.controls.size() || nextFrame != plan.frames.size())
      return reject("original transaction event/record inventory differs");
    if (plan.finalVersions.size() != environment.size())
      return reject("original final owner inventory differs");
    for (auto [owner, binding] : llvm::enumerate(plan.finalVersions))
      if (!charge(budget, transaction) || binding.owner != owner ||
          binding.version != environment[owner])
        return reject("original final owner environment differs");
    uint64_t outputIndex = 0;
    for (auto [owner, written] : llvm::enumerate(originalWriters)) {
      if (!charge(budget, transaction))
        return failure();
      if (!written)
        continue;
      if (outputIndex == preparation.outputs.size() ||
          preparation.outputs[outputIndex++].binding.owner != owner)
        return reject("original potential-writer output owner order differs");
    }
    if (outputIndex != preparation.outputs.size())
      return reject("original potential-writer output inventory differs");
    if (nextEffectEvent != effectEvents.size())
      return reject("original body-effect event inventory is incomplete");
    for (auto [id, original] : llvm::enumerate(plan.values)) {
      Value expectedValue = bound(id);
      if (record.boundValues[id] != expectedValue)
        return reject("saved bound-value map differs from original SSA");
    }
    for (auto [id, original] : llvm::enumerate(plan.versions))
      if (!versions[id].data || !versions[id].rawIntent ||
          record.versions[id].data != versions[id].data ||
          record.versions[id].rawIntent != versions[id].rawIntent ||
          (original.kind == ac::StateVersionKind::Write && original.value &&
           original.write && plan.writes[*original.write].pathFact &&
           plan.writes[*original.write].pathFact.getValue() &&
           versions[id].data !=
             (isa<ac::SourceStateWriteOp>(plan.writes[*original.write].operation) &&
                      !plan.writes[*original.write].selectors.empty()
                  ? record.selectedWrites[*original.write].data : bound(*original.value))))
        return reject("saved state-version map differs from original RHS");
    auto yield = cast<ac::YieldOp>(rule.getBody().front().getTerminator());
    Builder yieldAttributes(transaction.getContext());
    if (!product(budget, transaction, yield->getNumOperands(),
                 1 + SourceCheckBudget::bytes(sizeof(Value))))
      return failure();
    SmallVector<Value> yieldOperands(yield->getOperands());
    if (failed(generatedProof(yield, ac::YieldOp::getOperationName(),
                              yieldOperands, {},
                              yieldAttributes.getDictionaryAttr({}),
                              transaction.getLoc(), &rule.getBody().front())))
      return failure();
    if (yield->getNumOperands() != rule->getNumResults())
      return reject("original Rule yield arity differs");
    for (auto [ordinal, owner] : llvm::enumerate(expectedEntryOwners))
      if (yield->getOperand(legacyTotal + ordinal) != visibleEntries[owner])
        return reject("original entry visibility result differs from proven chain");
    for (const auto &domain : preparation.writeDomains)
      if (domain.write >= record.selectedWrites.size() ||
          yield->getOperand(domain.result) != record.selectedWrites[domain.write].mask ||
          yield->getOperand(domain.result).getType() != domain.maskType)
        return reject("original private write domain mask yield differs");
    auto sourceYield = originalYield;
    if (entryMethod ? plan.results.size() > sourceYield.getNumOperands()
                    : plan.results.size() != sourceYield.getNumOperands())
      return reject("original ordinary result inventory differs");
    for (auto [ordinal, result] : llvm::enumerate(plan.results))
      if (bound(result.yielded) != authenticated[plan.rootFrame].lookupOrNull(
                                       sourceYield.getOperand(ordinal)) ||
          yield->getOperand(ordinal) != bound(result.yielded) ||
          candidate.packageMap.lookupOrNull(plan.values[result.result].value) !=
              rule->getResult(ordinal))
        return reject("original ordinary result yield differs");
    for (auto [ordinal, output] : llvm::enumerate(preparation.outputs))
      if (output.dataResult != transaction->getNumResults() + 2 * ordinal ||
          output.enableResult != output.dataResult + 1 ||
          output.binding.owner >= environment.size() ||
          output.binding.version != environment[output.binding.owner] ||
          !originalWriters[output.binding.owner] ||
          yield->getOperand(output.dataResult) !=
              versions[output.binding.version].data)
        return reject("original final candidate RHS differs");
    Builder b(transaction.getContext());
    SmallVector<detail::MaterializedSelection, 0> sourceSelections(
        plan.selections.size());
    auto sourceSelected =
        [&](ac::TransactionSourcePath path) -> FailureOr<Value> {
      auto projection = sampledPath(plan, path, budget);
      if (failed(projection))
        return failure();
      if (projection->empty())
        return Value();
      uint64_t control = projection->getAs<IntegerAttr>("selection").getInt();
      if (control >= plan.controls.size())
        return failure();
      auto *selection = std::get_if<ac::BranchSelectControl>(
          &plan.controls[control].boundary);
      if (!selection || selection->selection >= sourceSelections.size())
        return failure();
      auto &paths = sourceSelections[selection->selection];
      Value value = projection->getAs<BoolAttr>("when_true").getValue()
                        ? paths.whenTrue
                        : paths.whenFalse;
      if (!value)
        return failure();
      return value;
    };
    auto binaryProof = [&](Value output, Value lhs, Value rhs, StringRef opcode,
                           Location location) -> LogicalResult {
      auto result = dyn_cast_or_null<OpResult>(output);
      if (!result || result.getResultNumber() != 0)
        return reject("original control binary result differs");
      return generatedProof(
          result.getOwner(), ac::BitsBinaryOp::getOperationName(), {lhs, rhs},
          {bit},
          b.getDictionaryAttr(
              {b.getNamedAttr("opcode", b.getStringAttr(opcode))}),
          location, &rule.getBody().front());
    };
    auto knownProof = [&](Value output, Value raw, bool one,
                          Location location) -> LogicalResult {
      auto compare = output ? output.getDefiningOp<ac::BitsCompareOp>()
                            : ac::BitsCompareOp();
      if (!compare || compare->getNumOperands() != 2 ||
          output != compare->getResult(0))
        return reject("original control knownness is absent");
      auto literal = compare->getOperand(1);
      if (failed(literalProof(literal, bit, transaction.getOccurrence(),
                              location, one, &rule.getBody().front())))
        return failure();
      return generatedProof(
          compare, ac::BitsCompareOp::getOperationName(), {raw, literal}, {bit},
          b.getDictionaryAttr(
              {b.getNamedAttr("predicate", b.getStringAttr("case_eq"))}),
          location, &rule.getBody().front());
    };
#include "TransactionQueueSourceProof.inc"
    uint64_t requiredIndex = 0, scheduledControl = 0;
    auto requireUse = [&](Operation *site, ac::InvocationFrameId frame,
                          ac::BoundValueId payload, StringRef kind,
                          uint64_t operand, ac::TransactionPoint point,
                          std::optional<ac::BoundValueId> rawPath = std::nullopt) -> LogicalResult {
      if (!prepared.valueAvailability) return success();
      if (requiredIndex >= preparation.requirements.size() ||
          requiredIndex >= record.requirements.size() ||
          scheduledControl >= preparation.controls.size())
        return reject("original required-use inventory is incomplete");
      const auto &requirement = preparation.requirements[requiredIndex];
      const auto &scheduled = preparation.controls[scheduledControl];
      if (requirement.site != site || requirement.frame != frame ||
          requirement.payload != payload || requirement.rawPath != rawPath ||
          requirement.kind.getValue() != kind || requirement.operand != operand ||
          !sameSourcePoint(requirement.point, point) ||
          requirement.control != scheduledControl || scheduled.source ||
          scheduled.requirement != requiredIndex)
        return reject("original required-use source root/order differs");
      Operation *function = plan.frames[frame].method
          ? plan.frames[frame].method.operator->() : transaction.operator->();
      auto occurrence = site->getAttrOfType<DictionaryAttr>("occurrence");
      if (!occurrence) occurrence = function->getAttrOfType<DictionaryAttr>(
          isa<ac::SourceTransactionOp>(function) ? "declaration" : "occurrence");
      auto location = site->getAttrOfType<DictionaryAttr>("ac.location");
      if (!location) location = site->getAttrOfType<DictionaryAttr>("ac.condition_location");
      if (!location) location = function->getAttrOfType<DictionaryAttr>("ac.guard_location");
      if (requirement.occurrence != occurrence || requirement.location != location)
        return reject("original required-use provenance differs");
      auto operation = record.requirements[requiredIndex];
      if (!operation || !operations.contains(operation) ||
          operation->getNumOperands() != 3 || operation.getPayload() != bound(payload))
        return reject("original required payload is disconnected");
      Value path = operation.getPath();
      auto peel = [&](Value expected) -> LogicalResult {
        auto conjunction = path.getDefiningOp<ac::BitsBinaryOp>();
        if (!conjunction || conjunction.getOpcode() != "and" ||
            conjunction.getRhs() != expected ||
            failed(binaryProof(path, conjunction.getLhs(), expected, "and", site->getLoc())))
          return reject("original required-use path differs");
        path = conjunction.getLhs();
        return success();
      };
      if (rawPath && failed(peel(bound(*rawPath)))) return failure();
      SmallVector<std::pair<ac::BranchId, bool>> ancestry;
      auto sourcePath = point.path;
      while (sourcePath.branch) {
        if (!charge(budget, site) || ancestry.size() > 128) return failure();
        ancestry.push_back({*sourcePath.branch, sourcePath.whenTrue});
        sourcePath = plan.branches[*sourcePath.branch].entry.path;
      }
      for (auto [branchId, whenTrue] : llvm::reverse(ancestry)) {
        Value condition = bound(plan.branches[branchId].condition);
        Value expected = condition;
        if (!whenTrue) {
          auto conjunction = path.getDefiningOp<ac::BitsBinaryOp>();
          auto inverted = conjunction ? conjunction.getRhs().getDefiningOp<ac::BitsUnaryOp>()
                                      : ac::BitsUnaryOp();
          if (!inverted || inverted.getInput() != condition ||
              failed(generatedProof(inverted, ac::BitsUnaryOp::getOperationName(),
                  {condition}, {condition.getType()}, b.getDictionaryAttr({
                      b.getNamedAttr("opcode", b.getStringAttr("not"))}),
                  site->getLoc(), &rule.getBody().front()))) return failure();
          expected = inverted.getResult();
        }
        if (failed(peel(expected))) return failure();
      }
      if (failed(literalProof(path, bit, transaction.getOccurrence(), site->getLoc(),
                              true, &rule.getBody().front())) ||
          failed(generatedProof(operation, ac::ValueRequireOp::getOperationName(),
              {bound(payload), operation.getAvailability(), operation.getPath()}, {bit},
              b.getDictionaryAttr({b.getNamedAttr("occurrence", occurrence),
                  b.getNamedAttr("source_location", location)}),
              site->getLoc(), &rule.getBody().front()))) return failure();
      ++requiredIndex;
      ++scheduledControl;
      return success();
    };
    auto sourceControl = [&](uint64_t source) -> LogicalResult {
      if (scheduledControl >= preparation.controls.size() ||
          preparation.sourceControls[source] != scheduledControl ||
          preparation.controls[scheduledControl].source != source ||
          preparation.controls[scheduledControl].requirement)
        return reject("original source/required control interleave differs");
      ++scheduledControl;
      return success();
    };
    for (const auto &event : plan.events) {
      using K = ac::TransactionStateEventKind;
      if (event.kind == K::GuardExit) {
        const auto &guard = plan.guards[*event.guard];
        if (failed(requireUse(guard.callable->getRegion(0).front().getTerminator(),
              guard.frame, guard.predicate, "guard", 0,
              {guard.control, plan.controls[guard.control].path})) ||
            failed(sourceControl(guard.control))) return failure();
      } else if (event.kind == K::BranchEnter &&
                 !plan.branches[*event.branch].selection &&
                 branchHasStateWrites(plan, *event.branch)) {
        const auto &branch = plan.branches[*event.branch];
        if (failed(requireUse(branch.operation, branch.frame, branch.condition,
                              "branch", 0, branch.entry))) return failure();
      } else if (event.kind == K::SelectionEnter) {
        const auto &selection = plan.selections[*event.selection];
        const auto &branch = plan.branches[selection.branch];
        if (failed(requireUse(branch.operation, branch.frame, branch.condition,
                              "branch", 0, branch.entry)) ||
            failed(sourceControl(selection.control))) return failure();
      } else if (event.kind == K::Write) {
        const auto &write = plan.writes[*event.write];
        if ((!write.pathFact || !write.pathFact.getValue()) &&
            failed(requireUse(write.operation, write.frame, write.authoredPath,
                              "write_path", 0, event.point))) return failure();
        if (failed(requireUse(write.operation, write.frame, write.value, "state_write",
            1, event.point, (!write.pathFact || !write.pathFact.getValue())
                ? std::optional<ac::BoundValueId>(write.authoredPath) : std::nullopt)))
          return failure();
        for (auto [ordinal, index] : llvm::enumerate(write.indices))
          if (failed(requireUse(write.operation, write.frame, index, "state_write", 2 + ordinal,
                                event.point, write.authoredPath))) return failure();
      } else if (event.kind == K::QueueAccess) {
        const auto &access = plan.queueAccesses[*event.queueAccess];
        if (access.payload && failed(requireUse(access.operation, access.frame,
              *access.payload, "queue_push", 1, event.point, access.authoredPath))) return failure();
        for (auto [index, resource] : llvm::enumerate(preparation.resources)) {
          if (resource.memoryAccess || resource.access != *event.queueAccess) continue;
          if (resource.control != scheduledControl || scheduledControl >= preparation.controls.size() ||
              preparation.controls[scheduledControl].resource != index)
            return reject("original queue resource schedule differs");
          ++scheduledControl;
        }
      } else if (event.kind == K::MemoryAccess) {
        const auto &access = plan.memoryAccesses[*event.memoryAccess];
        for (auto [ordinal, argument] : llvm::enumerate(access.arguments))
          if (failed(requireUse(access.operation, access.frame, argument,
                                "memory_argument", ordinal, event.point,
                                access.authoredPath))) return failure();
        if (*event.memoryAccess >= record.memoryReadPrefixes.size() ||
            (access.kind == ac::TransactionMemoryAccessKind::Read
                 ? record.memoryReadPrefixes[*event.memoryAccess] !=
                       record.prefixes[scheduledControl]
                 : bool(record.memoryReadPrefixes[*event.memoryAccess])))
          return reject("original Memory reached prefix differs");
        if (access.kind == ac::TransactionMemoryAccessKind::Read) {
          auto proposal = llvm::find_if(preparation.memoryProposals, [&](const auto &item) {
            return item.access == *event.memoryAccess;
          });
          if (proposal == preparation.memoryProposals.end() ||
              proposal->reachedControlEnd != scheduledControl)
            return reject("original Memory read prefix endpoint differs");
        }
        for (auto [index, resource] : llvm::enumerate(preparation.resources)) {
          if (resource.memoryAccess != event.memoryAccess) continue;
          if (resource.control != scheduledControl ||
              scheduledControl >= preparation.controls.size() ||
              preparation.controls[scheduledControl].resource != index)
            return reject("original Memory resource schedule differs");
          ++scheduledControl;
        }
      } else if (event.kind == K::FormalWrite) {
        const auto &formal = plan.formalWrites[*event.formalWrite];
        if (!formal.owner) continue;
        if (failed(requireUse(formal.operation, formal.frame, formal.authoredPath,
                              "formal_path", 0, event.point)) ||
            failed(requireUse(formal.operation, formal.frame, formal.payload,
                              "formal_write", 1, event.point, formal.authoredPath)) ||
            failed(requireUse(formal.operation, formal.frame, formal.candidate,
                              "formal_candidate", 2, event.point, formal.authoredPath)))
          return failure();
        for (auto [ordinal, index] : llvm::enumerate(formal.indices))
          if (failed(requireUse(formal.operation, formal.frame, index, "formal_write", 3 + ordinal,
                                event.point, formal.authoredPath))) return failure();
      } else if (event.kind == K::CallEnter) {
        const auto &call = plan.calls[*event.call];
        for (auto [ordinal, argument] : llvm::enumerate(call.arguments))
          if (failed(requireUse(call.operation, call.caller, argument, "call_argument",
                                ordinal, event.point))) return failure();
      } else if (event.kind == K::BodyCheck) {
        const auto &check = plan.bodyChecks[*event.bodyCheck];
        if (failed(requireUse(check.operation, check.frame, check.condition,
                              "assert", 0, event.point))) return failure();
      } else if (event.kind == K::Observe) {
        const auto &observation = plan.observations[*event.observation];
        for (auto [ordinal, value] : llvm::enumerate(observation.values))
          if (failed(requireUse(observation.operation, observation.frame, value,
                                "log", ordinal, event.point))) return failure();
      } else if (event.kind == K::CallExit) {
        const auto &call = plan.calls[*event.call];
        const auto &frame = plan.frames[call.callee];
        ac::SourceMethodView method{plan.instances[frame.anchor].definition, frame.method, {}};
        for (uint64_t ordinal = 0; ordinal < method.getNumPublicResults(); ++ordinal)
          if (failed(requireUse(frame.method->getRegion(1).front().getTerminator(),
              call.callee, call.results[ordinal].yielded, "return", ordinal, frame.exit)))
            return failure();
      } else if (event.kind == K::TransactionExit) {
        for (auto [ordinal, result] : llvm::enumerate(plan.results))
          if (failed(requireUse((plan.frames[plan.rootFrame].method
                  ? plan.frames[plan.rootFrame].method.operator->() : transaction.operator->())
                  ->getRegion(1).front().getTerminator(),
              plan.rootFrame, result.yielded, "return", ordinal, event.point))) return failure();
      }
    }
    if (requiredIndex != preparation.requirements.size() ||
        requiredIndex != record.requirements.size() ||
        scheduledControl != preparation.controls.size())
      return reject("original required-use inventory has extra rows");
    Value prefix = record.prefixes[0];
    if (failed(literalProof(prefix, bit, transaction.getOccurrence(),
                            transaction.getLoc(), true,
                            &rule.getBody().front())))
      return failure();
    for (auto [id, scheduled] : llvm::enumerate(preparation.controls)) {
      const auto *control = scheduled.source ? &plan.controls[*scheduled.source] : nullptr;
      const auto *requirement = scheduled.requirement
          ? &preparation.requirements[*scheduled.requirement] : nullptr;
      const auto *resource = scheduled.resource ? &preparation.resources[*scheduled.resource] : nullptr;
      Value raw = resource ? record.resources[*scheduled.resource]->getResult(0)
          : requirement ? record.requirements[*scheduled.requirement]->getResult(0)
                        : sourceRaw[*scheduled.source];
      const auto &actual = record.controls[id];
      bool selection =
          control && std::holds_alternative<ac::BranchSelectControl>(control->boundary);
      Location location = transaction.getLoc();
      if (resource) location = detail::resourceOperation(plan, *resource)->getLoc();
      else if (requirement) location = requirement->site->getLoc();
      else if (auto *guard =
              std::get_if<ac::CallableGuardControl>(&control->boundary))
        location = plan.guards[guard->guard].callable->getLoc();
      else
        location =
            plan.branches[plan.selections[std::get<ac::BranchSelectControl>(
                                              control->boundary)
                                              .selection]
                              .branch]
                .operation->getLoc();
      auto selected = sourceSelected(control ? control->path : requirement ? requirement->point.path
          : detail::resourcePoint(plan, *resource).path);
      Value yieldedReached = yield->getOperand(preparation.checkBase + 2 * id + 1);
      if (failed(selected) || actual.raw != raw ||
          actual.selectedPath != *selected ||
          yield->getOperand(preparation.checkBase + 2 * id) != raw ||
          (preparation.resources.empty()
               ? yieldedReached != actual.reached
               : id >= record.resourceDemand.reached.size() ||
                     yieldedReached != record.resourceDemand.reached[id]))
        return reject("original control raw/path/suffix differs");
      Value k0, k1 = actual.accepted;
      if (selection) {
        auto either = actual.accepted.getDefiningOp<ac::BitsBinaryOp>();
        if (!either || either->getNumOperands() != 2)
          return reject("original selector acceptance differs");
        k0 = either->getOperand(0);
        k1 = either->getOperand(1);
        if (failed(knownProof(k0, raw, false, location)) ||
            failed(binaryProof(actual.accepted, k0, k1, "or", location)))
          return failure();
      }
      if (failed(knownProof(k1, raw, true, location)))
        return failure();
      if (*selected) {
        auto bypass = actual.bypass.getDefiningOp<ac::BitsSelectOp>();
        if (!bypass || bypass->getNumOperands() != 3 ||
            failed(binaryProof(actual.reached, prefix, *selected, "and",
                               location)) ||
            failed(literalProof(bypass->getOperand(2), bit,
                                transaction.getOccurrence(), location, true,
                                &rule.getBody().front())) ||
            failed(generatedProof(
                bypass, ac::BitsSelectOp::getOperationName(),
                {*selected, actual.accepted, bypass->getOperand(2)}, {bit},
                b.getDictionaryAttr({}), location, &rule.getBody().front())) ||
            bypass->getResult(0) != actual.bypass)
          return reject("original control reached/bypass differs");
      } else if (actual.reached != prefix || actual.bypass != actual.accepted)
        return reject("original root control specialization differs");
      Value next = record.prefixes[id + 1];
      if (failed(binaryProof(next, prefix, actual.bypass, "and", location)))
        return failure();
      prefix = next;
      if (selection) {
        auto index =
            std::get<ac::BranchSelectControl>(control->boundary).selection;
        auto paths = record.selections[index];
        if (*selected) {
          if (failed(binaryProof(paths.whenTrue, *selected, k1, "and",
                                 location)) ||
              failed(
                  binaryProof(paths.whenFalse, *selected, k0, "and", location)))
            return failure();
        } else if (paths.whenTrue != k1 || paths.whenFalse != k0)
          return reject("original root selection paths differ");
        sourceSelections[index] = paths;
      }
    }
    if (preparation.resources.empty()) {
      if (record.resourceDemand.demand || record.resourceDemand.mayExecute ||
          !record.resourceDemand.reached.empty() ||
          !record.resourceDemand.generated.empty())
        return reject("original resource-free transaction invented demand");
    } else if (failed(resourceDemandProof(preparation, record, sourceRaw,
                                          bound))) {
      return failure();
    }
#include "TransactionQueuePrefixSourceProof.inc"
#include "TransactionMemoryPrefixSourceProof.inc"
    if (yield->getOperand(preparation.fireResult) != prefix)
      return reject("original final admission prefix differs");
    auto effectPathProof =
        [&](Value actual, ac::TransactionSourcePath path) -> LogicalResult {
      auto selected = sourceSelected(path);
      if (failed(selected))
        return failure();
      if (!*selected)
        return success(actual == prefix);
      return binaryProof(actual, prefix, *selected, "and",
                         transaction.getLoc());
    };
    for (OpOperand &use : rule->getResult(preparation.fireResult).getUses())
      if (!availabilityUses.contains(&use))
        return reject("original internal fire result escapes availability routing");
    if (llvm::count(originalWriters, true) != preparation.outputs.size())
      return reject("original potential-writer candidate inventory differs");
    for (const auto &output : preparation.outputs) {
      Value enable = yield->getOperand(output.enableResult);
      if (!enable ||
          failed(generatedProof(
              enable.getDefiningOp(), ac::BitsBinaryOp::getOperationName(),
              {prefix, versions[output.binding.version].rawIntent}, {bit},
              b.getDictionaryAttr(
                  {b.getNamedAttr("opcode", b.getStringAttr("and"))}),
              transaction.getLoc(), &rule.getBody().front())) ||
          enable.getDefiningOp()->getResult(0) != enable)
        return reject("original effective enable fire/intent differs");
    }
    for (auto [id, observation] : llvm::enumerate(plan.observations)) {
      uint64_t pathResult = preparation.observationPathResults[id];
      if (failed(effectPathProof(yield->getOperand(pathResult),
                                 observation.point.path)) ||
          rule->getResult(pathResult).getType() != prefix.getType() ||
          sourceObservations[id].path != bound(observation.authoredPath) ||
          sourceObservations[id].values.size() != observation.values.size())
        return reject("original observation yield/path inventory differs");
      for (uint64_t ordinal = 0; ordinal < observation.values.size(); ++ordinal)
        if (yield->getOperand(pathResult + 1 + ordinal) !=
                sourceObservations[id].values[ordinal] ||
            yield->getOperand(pathResult + 1 + ordinal).getType() !=
                sourceObservations[id].values[ordinal].getType())
          return reject("original observation payload yield differs");
    }
    for (auto [id, check] : llvm::enumerate(plan.bodyChecks)) {
      uint64_t result = preparation.checkBase + 2 * (preparation.controls.size() + id);
      if (yield->getOperand(result) != sourceChecks[id].first ||
          failed(effectPathProof(yield->getOperand(result + 1),
                                 check.point.path)) ||
          sourceChecks[id].second != bound(check.authoredPath) ||
          rule->getResult(result).getType() !=
              sourceChecks[id].first.getType() ||
          rule->getResult(result + 1).getType() != prefix.getType())
        return reject("original body-check yield/path inventory differs");
    }
    // Independently match every final check against original identity/span and
    // actual Rule suffix/pins; shared proof then establishes prefix topology.
    auto module = rule->getParentOfType<ac::ModuleOp>();
    auto invocationIdentity =
        [&](ac::InvocationFrameId frame) -> FailureOr<ArrayAttr> {
      uint64_t count = 0;
      for (auto cursor = frame; plan.frames[cursor].parent;
           cursor = *plan.frames[cursor].parent) {
        if (!charge(budget, transaction))
          return failure();
        count = SourceCheckBudget::add(count, 1);
      }
      uint64_t storage = SourceCheckBudget::multiply(
          count, SourceCheckBudget::add(
                     SourceCheckBudget::bytes(sizeof(ac::InvocationFrameId)),
                     SourceCheckBudget::bytes(sizeof(Attribute))));
      if (!charge(budget, transaction, storage) ||
          count > std::numeric_limits<size_t>::max())
        return failure();
      SmallVector<ac::InvocationFrameId> chain;
      chain.reserve(count);
      while (plan.frames[frame].parent) {
        chain.push_back(frame);
        frame = *plan.frames[frame].parent;
      }
      SmallVector<Attribute> rows;
      rows.reserve(count);
      for (auto id : llvm::reverse(chain))
        rows.push_back(b.getDictionaryAttr(
            {b.getNamedAttr("call", plan.frames[id].callOccurrence),
             b.getNamedAttr("receiver", plan.frames[id].receiver)}));
      return b.getArrayAttr(rows);
    };
    auto ownerIdentity = [&](ac::InvocationFrameId frame) {
      return plan.frames[frame].parent
                 ? plan.frames[frame].receiver
                 : b.getDictionaryAttr(
                       {b.getNamedAttr("instance_path", b.getArrayAttr({}))});
    };
    auto runtimeMetadataPath = [&](ac::TransactionSourcePath source) -> FailureOr<DictionaryAttr> {
      auto path = sampledPath(plan, source, budget);
      if (failed(path) || path->empty()) return path;
      NamedAttrList mapped(*path);
      uint64_t index = path->getAs<IntegerAttr>("selection").getInt();
      mapped.set("selection", b.getI64IntegerAttr(preparation.sourceControls[index]));
      return mapped.getDictionary(b.getContext());
    };
    Operation *lastTerminal = rule.operator->();
    for (auto [ordinal, scheduled] : llvm::enumerate(preparation.controls)) {
      const auto *control = scheduled.source ? &plan.controls[*scheduled.source] : nullptr;
      const auto *requirement = scheduled.requirement
          ? &preparation.requirements[*scheduled.requirement] : nullptr;
      const auto *resource = scheduled.resource ? &preparation.resources[*scheduled.resource] : nullptr;
      auto sourcePath = control ? control->path : requirement ? requirement->point.path
          : detail::resourcePoint(plan, *resource).path;
      ac::InvocationFrameId frame;
      DictionaryAttr occurrence, location;
      StringRef kind;
      if (resource) {
        frame = detail::resourceFrame(plan, *resource);
        occurrence = detail::resourceOccurrence(plan, *resource);
        location = detail::resourceLocation(plan, *resource);
        kind = "resource_available";
      } else if (requirement) {
        frame = requirement->frame;
        occurrence = requirement->occurrence;
        location = requirement->location;
        kind = "value_available";
      } else if (auto *tag =
              std::get_if<ac::CallableGuardControl>(&control->boundary)) {
        const auto &guard = plan.guards[tag->guard];
        frame = guard.frame;
        location = guard.location;
        kind = "callable_guard";
        occurrence = guard.callable->getAttrOfType<DictionaryAttr>(
            isa<ac::SourceTransactionOp>(guard.callable) ? "declaration"
                                                         : "occurrence");
      } else {
        const auto &branch =
            plan.branches[plan.selections[std::get<ac::BranchSelectControl>(
                                              control->boundary)
                                              .selection]
                              .branch];
        frame = branch.frame;
        location = branch.conditionLocation;
        kind = "branch_select";
        occurrence = branch.occurrence;
      }
      auto check =
          cast<DictionaryAttr>(preparation.expectedRequiredChecks[ordinal]);
      auto expectedID = b.getDictionaryAttr(
          {b.getNamedAttr("registration", transaction.getOccurrence()),
           b.getNamedAttr("check", occurrence),
           b.getNamedAttr("obligation", b.getI64IntegerAttr(ordinal))});
      if (check != b.getDictionaryAttr(
                       {b.getNamedAttr("id", expectedID),
                        b.getNamedAttr("kind", b.getStringAttr("guard_known")),
                        b.getNamedAttr("location", location)}))
        return reject("original guard required metadata differs");
      auto guardRows =
          preparation.expectedAdmission.getAs<ArrayAttr>("controls");
      auto invocation = invocationIdentity(frame);
      if (failed(invocation))
        return failure();
      auto path = sampledPath(plan, sourcePath, budget);
      if (failed(path)) return failure();
      if (!path->empty()) {
        NamedAttrList mapped(*path);
        uint64_t source = path->getAs<IntegerAttr>("selection").getInt();
        mapped.set("selection", b.getI64IntegerAttr(preparation.sourceControls[source]));
        path = mapped.getDictionary(b.getContext());
      }
      auto expectedGuard = b.getDictionaryAttr(
          {b.getNamedAttr("id", expectedID),
           b.getNamedAttr("invocation", *invocation),
           b.getNamedAttr("kind", b.getStringAttr(kind)),
           b.getNamedAttr("path", *path),
           b.getNamedAttr("predecessor",
                          b.getI64IntegerAttr(int64_t(ordinal) - 1))});
      if (guardRows[ordinal] != expectedGuard)
        return reject("original guard admission metadata differs");
      unsigned matches = 0;
      for (auto expect :
           module.getBody().front().getOps<ac::SourceExpectOp>()) {
        if (!charge(budget, expect, 8))
          return failure();
        if (expect->getAttr("ac.check_id") != check.get("id"))
          continue;
        ++matches;
        if (expect.getKind() != "guard_known" ||
            expect->getAttr("location") != location ||
            expect->getNumOperands() != 4 ||
            expect->getOperand(0) !=
                rule->getResult(preparation.checkBase + 2 * ordinal) ||
            expect->getOperand(1) !=
                rule->getResult(preparation.checkBase + 2 * ordinal + 1) ||
            expect->getOperand(2) !=
                rule->getOperand(preparation.clockCapture) ||
            expect->getOperand(3) !=
                rule->getOperand(preparation.resetCapture) ||
            !lastTerminal->isBeforeInBlock(expect))
          return reject("original guard expect span/suffix/domain differs");
        if (failed(generatedProof(
                expect, ac::SourceExpectOp::getOperationName(),
                {rule->getResult(preparation.checkBase + 2 * ordinal),
                 rule->getResult(preparation.checkBase + 2 * ordinal + 1),
                 rule->getOperand(preparation.clockCapture),
                 rule->getOperand(preparation.resetCapture)},
                {},
                b.getDictionaryAttr(
                    {b.getNamedAttr("kind", b.getStringAttr("guard_known")),
                     b.getNamedAttr("location", location),
                     b.getNamedAttr("ac.check_id", check.get("id"))}),
                transaction.getLoc(), &module.getBody().front())))
          return failure();
        if (record.controls[ordinal].terminal != expect)
          return reject("original control terminal locator differs");
        lastTerminal = expect;
      }
      if (matches != 1)
        return reject("original guard expect inventory differs");
    }
    for (const auto &event : plan.events) {
      if (event.kind == ac::TransactionStateEventKind::BodyCheck) {
        auto id = *event.bodyCheck;
        const auto &source = plan.bodyChecks[id];
        auto identity = b.getDictionaryAttr(
            {b.getNamedAttr("registration", transaction.getOccurrence()),
             b.getNamedAttr("check", source.occurrence),
             b.getNamedAttr("obligation",
                            b.getI64IntegerAttr(preparation.controls.size() + id))});
        auto bodyRows =
            preparation.expectedAdmission.getAs<ArrayAttr>("body_checks");
        auto invocation = invocationIdentity(source.frame);
        if (failed(invocation))
          return failure();
        auto path = runtimeMetadataPath(source.point.path);
        if (failed(path))
          return failure();
        if (bodyRows[id] !=
            b.getDictionaryAttr({b.getNamedAttr("id", identity),
                                 b.getNamedAttr("invocation", *invocation),
                                 b.getNamedAttr("path", *path)}))
          return reject("original body-check admission metadata differs");
        if (preparation.expectedRequiredChecks[preparation.controls.size() + id] !=
            b.getDictionaryAttr(
                {b.getNamedAttr("id", identity),
                 b.getNamedAttr("kind", b.getStringAttr("assert")),
                 b.getNamedAttr("location", source.location)}))
          return reject("original body-check required metadata differs");
        uint64_t result =
            preparation.checkBase + 2 * (preparation.controls.size() + id);
        ac::SourceExpectOp found;
        unsigned matches = 0;
        for (auto expect :
             module.getBody().front().getOps<ac::SourceExpectOp>()) {
          if (!charge(budget, expect, 8))
            return failure();
          if (expect->getAttr("ac.check_id") != identity)
            continue;
          ++matches;
          found = expect;
          NamedAttrList attrs;
          attrs.append("kind", b.getStringAttr("assert"));
          attrs.append("location", source.location);
          attrs.append("ac.check_id", identity);
          if (source.message)
            attrs.append("ac.message", source.message);
          if (expect.getKind() != "assert" || !expect.getSampling().empty() ||
              expect->getNumOperands() != 2 ||
              expect.getCondition() != rule->getResult(result) ||
              expect.getPath() != rule->getResult(result + 1) ||
              !lastTerminal->isBeforeInBlock(expect))
            return reject("original body-check terminal shape/order differs");
          if (failed(generatedProof(
                  expect, ac::SourceExpectOp::getOperationName(),
                  {rule->getResult(result), rule->getResult(result + 1)}, {},
                  attrs.getDictionary(transaction.getContext()),
                  source.operation->getLoc(), &module.getBody().front())))
            return failure();
          lastTerminal = expect;
        }
        if (matches != 1 || record.bodyChecks[id] != found)
          return reject("original body-check terminal inventory differs");
      } else if (event.kind == ac::TransactionStateEventKind::Observe) {
        auto id = *event.observation;
        const auto &source = plan.observations[id];
        auto invocation = invocationIdentity(source.frame);
        if (failed(invocation))
          return failure();
        auto identity = b.getDictionaryAttr(
            {b.getNamedAttr("registration", transaction.getOccurrence()),
             b.getNamedAttr("site", source.occurrence),
             b.getNamedAttr("ordinal", b.getI64IntegerAttr(id)),
             b.getNamedAttr("invocation", *invocation),
             b.getNamedAttr("owner", ownerIdentity(source.frame))});
        auto observationRows =
            preparation.expectedAdmission.getAs<ArrayAttr>("observations");
        auto path = runtimeMetadataPath(source.point.path);
        if (failed(path))
          return failure();
        if (observationRows[id] !=
            b.getDictionaryAttr(
                {b.getNamedAttr("id", identity), b.getNamedAttr("path", *path),
                 b.getNamedAttr("value_count",
                                b.getI64IntegerAttr(source.values.size()))}))
          return reject("original observation admission metadata differs");
        uint64_t pathResult = preparation.observationPathResults[id];
        if (!charge(budget, transaction,
                    SourceCheckBudget::multiply(
                        SourceCheckBudget::add(1, source.values.size()),
                        SourceCheckBudget::bytes(sizeof(Value)))))
          return failure();
        SmallVector<Value> operands{rule->getResult(pathResult)};
        for (uint64_t ordinal = 0; ordinal < source.values.size(); ++ordinal)
          operands.push_back(rule->getResult(pathResult + 1 + ordinal));
        ac::SourceObserveOp found;
        unsigned matches = 0;
        for (auto observe :
             module.getBody().front().getOps<ac::SourceObserveOp>()) {
          if (!charge(budget, observe, 8))
            return failure();
          if (observe->getAttr("ac.observation_id") != identity)
            continue;
          ++matches;
          found = observe;
          if (observe.getKindAttr() != source.kind ||
              observe.getSpec() != source.spec ||
              observe->getAttr("ac.location") != source.location ||
              !lastTerminal->isBeforeInBlock(observe))
            return reject("original observation terminal shape/order differs");
          if (failed(generatedProof(
                  observe, ac::SourceObserveOp::getOperationName(), operands,
                  {},
                  b.getDictionaryAttr(
                      {b.getNamedAttr("kind", source.kind),
                       b.getNamedAttr("spec", source.spec),
                       b.getNamedAttr("ac.location", source.location),
                       b.getNamedAttr("ac.observation_id", identity)}),
                  source.operation->getLoc(), &module.getBody().front())))
            return failure();
          lastTerminal = observe;
        }
        if (matches != 1 || record.observations[id] != found)
          return reject("original observation terminal inventory differs");
      }
    }
    return success();
  }

  const PreparedTransactionLowering &prepared;
  const MaterializedTransactionLowering &candidate;
  SourceCheckBudget &budget;
  ac::HardwareAnalysis hardware;
  ac::SourceReceiverProofContext receiverProof;
  BranchTypeAdmissionCache branchTypes;
  // Node-stable storage keeps simultaneous ArrayRef results valid when another
  // transaction's independent closure is first queried.
  std::map<Operation *, OriginalRegistration> originalRegistrations;
  bool registrationsIndexed = false;
  DenseSet<Operation *> operations, clones;
  DenseSet<OpOperand *> availabilityUses;
  DenseSet<Block *> blocks;
  DenseSet<Value> values;
};

} // namespace

namespace detail {
FailureOr<PreparedTransactionLowering>
prepareTransactionLowering(mlir::ModuleOp original,
                           const ac::SourceTransactionStateAnalysis &analysis,
                           SourceCheckBudget &budget) {
  budget.setPhase("transaction preparation");
  if (!analysis.isValid())
    return original.emitError()
           << "transaction lowering requires a valid original plan";
  LoweringPreparation builder(original, analysis, budget);
  if (failed(builder.run()))
    return failure();
  return std::move(static_cast<PreparedTransactionLowering &>(builder));
}

FailureOr<MaterializedTransactionLowering>
materializeTransactionLowering(PreparedTransactionLowering &prepared,
                               SourceCheckBudget &budget) {
  budget.setPhase("transaction clone/materialization");
  if (failed(verifyPreparedEffectInventory(prepared, budget)) ||
      !charge(budget, prepared.original,
              SourceCheckBudget::multiply(
                  prepared.transactions.size(),
                  SourceCheckBudget::bytes(sizeof(MaterializedTransaction)))))
    return failure();
  MaterializedTransactionLowering result;
  result.module =
      cast<mlir::ModuleOp>(prepared.original->clone(result.packageMap));
  if (failed(materialize(prepared, *result.module, result.packageMap,
                         result.transactions, result.incomingCommits, budget)))
    return failure();
  if (failed(QueuePinMaterializer(prepared, result, budget).run())) return failure();
  if (failed(MemoryPinMaterializer(prepared, result, budget).run())) return failure();
  if (failed(materializeTransactionValueAvailability(prepared, result, budget)))
    return failure();
  return std::move(result);
}

LogicalResult verifyTransactionMaterialization(
    const PreparedTransactionLowering &prepared,
    const MaterializedTransactionLowering &candidate,
    SourceCheckBudget &budget) {
  budget.setPhase("transaction original-source proof");
  if (failed(verifyPreparedEffectInventory(prepared, budget)) ||
      !candidate.module ||
      failed(OriginalSourceProof(prepared, candidate, budget).run()))
    return failure();
  // The canonical verifier owns final IR admission and its independent budget.
  return ac::verifyHardwarePackage(*candidate.module);
}
} // namespace detail

namespace {

class LowerTransactionsPass
    : public PassWrapper<LowerTransactionsPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerTransactionsPass)
  StringRef getArgument() const final { return "ac-lower-transactions"; }
  StringRef getDescription() const final {
    return "Lower sampled transactions through ordinary instance ports and "
           "original DFFE storage";
  }
  void runOnOperation() final {
    auto input = getOperation();
    if (!ac::detail::findUnloweredTransaction(input)) {
      // Existing ordinary hardware retains its own admission and budgets. No
      // transaction analysis, invocation accounting or clone is introduced.
      if (failed(getAnalysis<ac::HardwareAnalysis>().verify()) ||
          failed(mergedEnvelope(input))) {
        signalPassFailure();
        return;
      }
      input->setAttr("ac.stage", StringAttr::get(input.getContext(), "final"));
      return;
    }
    const auto &analysis = getAnalysis<ac::SourceTransactionStateAnalysis>();
    if (!analysis.isValid()) {
      signalPassFailure();
      return;
    }
    ac::HardwareSourceCheckLimits limits;
    limits.maxWorkUnits = 1024 * 1024;
    SourceCheckBudget budget(limits);
    auto prepared = detail::prepareTransactionLowering(input, analysis, budget);
    if (failed(prepared)) {
      signalPassFailure();
      return;
    }
    auto candidate = detail::materializeTransactionLowering(*prepared, budget);
    if (failed(candidate) || failed(detail::verifyTransactionMaterialization(
                                 *prepared, *candidate, budget))) {
      signalPassFailure();
      return;
    }
    auto &staged = candidate->module;
    // Verification is complete. Preserve the pass root and replace contents
    // atomically, with no fallible step after this first original mutation.
    input->setAttrs((*staged)->getAttrs());
    input->getRegion(0).takeBody((*staged)->getRegion(0));
  }
};

} // namespace

std::unique_ptr<Pass> createLowerTransactionsPass() {
  return std::make_unique<LowerTransactionsPass>();
}

} // namespace acir

#include "LowerTransactions.h"
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
using detail::TransactionPreparation;

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

LogicalResult requireStaticTotality(const ac::TransactionStatePlan &plan,
                                    SourceCheckBudget &budget) {
  if (!plan.transaction->getNumResults())
    return success();
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
  if (prefix != Fact::True)
    return plan.transaction->emitOpError()
           << "ordinary transaction results require proven total admission";
  return success();
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
  for (const auto &item : prepared.transactions) {
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
    if (item.expectedAdmission.size() != 6 || !guards || !checks ||
        !observations || guards.size() != plan.controls.size() ||
        checks.size() != plan.bodyChecks.size() ||
        observations.size() != plan.observations.size() ||
        item.observationPathResults.size() != plan.observations.size() ||
        item.expectedRequiredChecks.size() !=
            plan.controls.size() + plan.bodyChecks.size())
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
                  2, SourceCheckBudget::add(plan.controls.size(),
                                            plan.bodyChecks.size())));
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
    for (const auto &frame : plan.frames) {
      if (!charge(budget, site,
                  8 + frame.guardArguments.size() +
                      frame.bodyArguments.size()) ||
          !index(frame.anchor, plan.instances.size()) ||
          !index(frame.guard, plan.guards.size()) ||
          (frame.parent && !index(*frame.parent, plan.frames.size())) ||
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

LogicalResult prepayClone(Operation *root, SourceCheckBudget &budget) {
  SmallVector<Operation *> pending;
  if (!charge(budget, root))
    return failure();
  pending.push_back(root);
  while (!pending.empty()) {
    Operation *op = pending.pop_back_val();
    if (!charge(budget, op,
                3 + uint64_t(op->getNumOperands()) * 2 +
                    uint64_t(op->getNumResults()) * 2 + op->getNumRegions() +
                    op->getRawDictionaryAttrs().size() +
                    op->getPropertiesStorageSize()))
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
      if (hardware.getPrimitiveKind(imported) == "dffe") {
        ++dffeCount;
        if (!dffe)
          dffe = imported;
      }
    }
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
          if (!isa<ac::BitsType>(state.getQ().getType()))
            return state.emitOpError()
                   << "transaction lowering supports fixed Bits state only";
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
    if (!states.empty() && (!dffe || dffeCount != 1))
      return package.emitError() << "transaction state requires exactly one "
                                    "verified DFFE primitive";
    for (auto &prepared : transactions)
      if (failed(liveIns(prepared)) || failed(outputs(prepared)) ||
          failed(expansion(prepared)) || failed(admissionMetadata(prepared)))
        return failure();
    // Every definition participates, including definitions not selected by the
    // System. Summaries are relative to definitions, never root occurrences.
    for (auto &definition : definitions)
      if (failed(writerSummary(definition.module, 0)))
        return failure();
    for (auto module : package.getOps<ac::ModuleOp>())
      for (auto collection :
           module.getBody().front().getOps<ac::CollectionOp>()) {
        auto callee =
            dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(collection));
        if (callee && (!routing(callee).upward.empty() ||
                       !routing(callee).downward.empty()))
          return collection.emitOpError()
                 << "transaction routing cannot extend a definition shared "
                    "with Collection";
      }
    // Prepay full arities, signature growth, route lookup and generated zeros.
    for (auto &definition : definitions) {
      uint64_t ports =
          definition.upward.size() + 2 * definition.downward.size();
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
                        32 * (child.upward.size() + 2 * child.downward.size()) +
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
      if (failed(wiringMetadata(definition)))
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
    auto checkCount = add(plan.controls.size(), plan.bodyChecks.size(),
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
    prepared.totalResults = *total;
    SmallVector<Attribute> guards, bodyChecks, observations, checks;
    for (auto [ordinal, control] : llvm::enumerate(plan.controls)) {
      ac::InvocationFrameId frame;
      DictionaryAttr occurrence, location;
      StringRef kind;
      if (auto *binding =
              std::get_if<ac::CallableGuardControl>(&control.boundary)) {
        const auto &guard = plan.guards[binding->guard];
        frame = guard.frame;
        occurrence = guard.callable->getAttrOfType<DictionaryAttr>(
            isa<ac::SourceTransactionOp>(guard.callable) ? "declaration"
                                                         : "occurrence");
        location = guard.location;
        kind = "callable_guard";
      } else {
        auto selection =
            std::get<ac::BranchSelectControl>(control.boundary).selection;
        const auto &branch = plan.branches[plan.selections[selection].branch];
        frame = branch.frame;
        occurrence = branch.occurrence;
        location = branch.conditionLocation;
        kind = "branch_select";
      }
      auto frames = invocation(frame);
      auto path = sampledPath(plan, control.path, budget);
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
                                            plan.controls.size() + ordinal))});
      auto path = sampledPath(plan, check.point.path, budget);
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
      auto path = sampledPath(plan, observation.point.path, budget);
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
    prepared.expectedRequiredChecks = b.getArrayAttr(checks);
    return success();
  }

  LogicalResult wiringMetadata(DefinitionRouting &definition) {
    if (definition.stateIndices.empty() &&
        definition.transactionIndices.empty() && definition.upward.empty() &&
        definition.downward.empty())
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
    if (failed(requireStaticTotality(plan, budget)))
      return failure();
    for (const auto &owner : plan.owners) {
      if (!charge(budget, transaction, 1 + owner.instancePath.size()))
        return failure();
      if (!isa<ac::BitsType>(owner.payloadType))
        return transaction.emitOpError()
               << "transaction lowering requires whole fixed Bits state";
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

  struct Writer {
    SmallVector<ac::InstanceOp> ownerPath;
    Value state;
    SmallVector<ac::InstanceOp> transactionPath;
    ac::SourceTransactionOp transaction;
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
    llvm::DenseMap<RoutingKey, unsigned> seen;
    auto add = [&](Writer writer) -> LogicalResult {
      if (!charge(budget, module,
                  8 + writer.ownerPath.size() + writer.transactionPath.size()))
        return failure();
      auto ownerKey = key(writer.ownerPath, writer.state);
      auto found = seen.find(ownerKey);
      if (found != seen.end()) {
        const auto &prior = summary[found->second];
        if (prior.transaction != writer.transaction ||
            prior.transactionPath != writer.transactionPath)
          return module.emitOpError()
                 << "transaction lowering cannot admit multiple outer writers "
                    "for one physical owner";
        return success();
      }
      seen.try_emplace(ownerKey, summary.size());
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
                        plan.transaction})))
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
                    plan.branchResults.size(),
                    sizeof(MaterializedBranchResult)))));
    storage = SourceCheckBudget::add(
        storage, SourceCheckBudget::add(
                     SourceCheckBudget::multiply(plan.controls.size() + 1,
                                                 sizeof(Value)),
                     SourceCheckBudget::add(
                         SourceCheckBudget::multiply(
                             plan.controls.size(),
                             sizeof(detail::MaterializedAdmissionControl)),
                         SourceCheckBudget::multiply(
                             plan.selections.size(),
                             sizeof(detail::MaterializedSelection)))));
    uint64_t effectValues = 0;
    for (const auto &observation : plan.observations)
      effectValues =
          SourceCheckBudget::add(effectValues, observation.values.size());
    storage = SourceCheckBudget::add(
        storage, SourceCheckBudget::multiply(
                     plan.bodyChecks.size() + plan.observations.size(),
                     sizeof(const ac::TransactionStateEvent *)));
    if (!charge(budget, transaction, SourceCheckBudget::bytes(storage)) ||
        !product(budget, transaction, plan.controls.size(), 64) ||
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
                    plan.branchResults.size() + prepared.outputs.size()) ||
        !product(budget, transaction, plan.frames.size(),
                 2 * prepared.liveIns.size() + 1))
      return failure();
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
                 ac::SourceMethodCallOp, ac::SourceExpectOp,
                 ac::SourceObserveOp, ac::SourceBranchOp, ac::YieldOp>(op) &&
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
  unsigned dffeCount = 0;
};

struct StateStorage {
  ac::SourceStateOp declaration;
  ac::InstanceOp instance;
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

class RuleMaterializer {
public:
  RuleMaterializer(const TransactionPreparation &prepared,
                   IRMapping &packageMap, MaterializationRoutes &routes,
                   SourceCheckBudget &budget)
      : prepared(prepared), plan(*prepared.plan), packageMap(packageMap),
        routes(routes), budget(budget), frames(plan.frames.size()),
        versions(plan.versions.size()),
        branchResults(plan.branchResults.size()),
        controls(plan.controls.size()), selections(plan.selections.size()) {
    prefixes.reserve(plan.controls.size() + 1);
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
    for (const auto &control : plan.controls) {
      ac::BoundValueId raw;
      if (auto *guard =
              std::get_if<ac::CallableGuardControl>(&control.boundary))
        raw = plan.guards[guard->guard].predicate;
      else
        raw = plan.branches[plan.selections[std::get<ac::BranchSelectControl>(
                                                control.boundary)
                                                .selection]
                                .branch]
                  .condition;
      types.push_back(plan.values[raw].value.getType());
      types.push_back(predicateType);
    }
    for (const auto &check : plan.bodyChecks) {
      types.push_back(plan.values[check.condition].value.getType());
      types.push_back(predicateType);
    }
    if (types.size() != prepared.totalResults)
      return transaction.emitOpError()
             << "transaction prepared result layout is inconsistent";
    OperationState state(staged.getLoc(), ac::RuleOp::getOperationName());
    state.addOperands(captures);
    state.addTypes(types);
    NamedAttrList attributes(transaction->getAttrs());
    attributes.erase("ac.guard_location");
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
    for (auto [frame, binding] : llvm::enumerate(plan.frames))
      for (auto liveIn : prepared.liveIns) {
        const auto &ref = plan.values[liveIn];
        if (ref.anchor == binding.anchor) {
          if (!prepayValueMapping(budget, transaction, frames[frame],
                                  ref.value))
            return failure();
          frames[frame].map(ref.value, lexical.lookup({ref.anchor, ref.value}));
        }
      }
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
    for (const auto &control : controls) {
      results.push_back(control.raw);
      results.push_back(control.reached);
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
      uint64_t result = prepared.checkBase + 2 * (plan.controls.size() + id);
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
    record.frames = std::move(frames);
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
    versions[id] = {*value, *intent};
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
  LogicalResult admitControl(ac::AdmissionControlId id, Value raw,
                             Location location, OpBuilder &builder) {
    if (id != prefixes.size() - 1 || id >= controls.size())
      return plan.transaction->emitOpError() << "control order differs";
    auto path = selectedPath(plan.controls[id].path);
    if (failed(path))
      return failure();
    auto k1 = known(raw, true, location, builder);
    if (failed(k1))
      return failure();
    Value accepted = *k1;
    auto *selection =
        std::get_if<ac::BranchSelectControl>(&plan.controls[id].boundary);
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
      if (auto access = dyn_cast<ac::SourceStateReadOp>(op)) {
        auto found = reads.find({frame, access});
        if (found == reads.end() || !versions[found->second->version].data)
          return access.emitOpError() << "transaction read version is unmapped";
        if (!prepayValueMapping(budget, access, frames[frame],
                                access.getValue()))
          return failure();
        frames[frame].map(access.getValue(),
                          versions[found->second->version].data);
      } else if (auto intent = dyn_cast<ac::SourceStateWriteOp>(op)) {
        auto found = writes.find({frame, intent});
        if (found == writes.end())
          return intent.emitOpError() << "transaction write plan is missing";
        auto rhs = bound(found->second->value);
        if (failed(rhs))
          return failure();
        auto rawIntent =
            intentLiteral(prefix.getType(), true, op.getLoc(), builder);
        if (failed(rawIntent))
          return failure();
        versions[found->second->version] = {*rhs, *rawIntent};
      } else if (auto split = dyn_cast<ac::SourceBranchOp>(op)) {
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
              select(*condition, versions[join.whenTrue].data,
                     versions[join.whenFalse].data,
                     plan.owners[join.owner].payloadType, op.getLoc(), builder);
          auto intent = select(*condition, versions[join.whenTrue].rawIntent,
                               versions[join.whenFalse].rawIntent,
                               prefix.getType(), op.getLoc(), builder);
          if (failed(data) || failed(intent))
            return failure();
          versions[join.result] = {data->getResult(), intent->getResult()};
        }
        for (auto id : found->results) {
          const auto &result = plan.branchResults[id];
          auto whenTrue = bound(result.whenTrue);
          auto whenFalse = bound(result.whenFalse);
          if (failed(whenTrue) || failed(whenFalse))
            return failure();
          auto joined = select(*condition, *whenTrue, *whenFalse,
                               result.payloadType, op.getLoc(), builder);
          if (failed(joined))
            return failure();
          Value original = plan.values[result.result].value;
          if (!prepayValueMapping(budget, split, frames[frame], original))
            return failure();
          frames[frame].map(original, joined->getResult());
          branchResults[id].select = *joined;
        }
      } else if (auto call = dyn_cast<ac::SourceMethodCallOp>(op)) {
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

  const TransactionPreparation &prepared;
  const ac::TransactionStatePlan &plan;
  IRMapping &packageMap;
  using LexicalKey = std::pair<ac::LexicalInstanceId, Value>;
  MaterializationRoutes &routes;
  SourceCheckBudget &budget;
  llvm::DenseMap<LexicalKey, Value> lexical;
  SmallVector<IRMapping, 0> frames;
  SmallVector<MaterializedVersion, 0> versions;
  SmallVector<MaterializedBranchResult, 0> branchResults;
  Value prefix;
  SmallVector<Value, 0> prefixes;
  SmallVector<detail::MaterializedAdmissionControl, 0> controls;
  SmallVector<detail::MaterializedSelection, 0> selections;
  llvm::DenseMap<Key, const ac::StateReadBinding *> reads;
  llvm::DenseMap<Key, const ac::StateWriteIntent *> writes;
  llvm::DenseMap<Key, const ac::TransactionCallBinding *> calls;
  llvm::DenseMap<Key, ac::BodyCheckId> bodyCheckPlans;
  llvm::DenseMap<Key, ac::ObservationId> observationPlans;
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

LogicalResult eraseTargetMappings(IRMapping &mapping, Operation *removed,
                                  SourceCheckBudget &budget) {
  uint64_t entries = SourceCheckBudget::add(
      mapping.getOperationMap().size(),
      SourceCheckBudget::add(mapping.getBlockMap().size(),
                             mapping.getValueMap().size()));
  if (!charge(budget, removed, SourceCheckBudget::multiply(entries, 4)))
    return failure();
  auto belongs = [&](Operation *op) {
    return op == removed || removed->isAncestor(op);
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

LogicalResult materialize(const PreparedTransactionLowering &prepared,
                          mlir::ModuleOp staged, IRMapping &mapping,
                          SmallVectorImpl<MaterializedTransaction> &records,
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
  SmallVector<Value> stagingZeros;

  // Definitions receive the union of demands from every caller occurrence.
  // Source types/names/domain ordinals are preserved as an exact prefix.
  for (auto &definition : prepared.definitions) {
    if (definition.upward.empty() && definition.downward.empty())
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
  // Initial zero operands are replaced below wherever an actual writer routes.
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
      if (definition.upward.empty() && definition.downward.empty())
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
      SmallVector<Value> operands(old.getOperands());
      OpBuilder builder(old);
      for (const auto &route : definition.downward) {
        auto dType = hardware.resolveType(route.type, *bindings, original);
        auto eType =
            hardware.resolveType(route.enableType, *bindings, original);
        if (failed(dType) || failed(eType))
          return failure();
        auto d = bitsLiteral(*dType, original.getOccurrence(),
                             original.getLoc(), false, builder);
        auto e = bitsLiteral(*eType, original.getOccurrence(),
                             original.getLoc(), false, builder);
        operands.append({d, e});
        stagingZeros.append({d, e});
      }
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

  // State remains at its original lexical declaration, one DFFE per owner.
  for (auto original : prepared.states) {
    auto state = cast<ac::SourceStateOp>(mapping.lookup(original.operator->()));
    OperationState leaf(state.getLoc(), ac::InstanceOp::getOperationName());
    leaf.addTypes(original.getQ().getType());
    leaf.addAttribute("instance_name", original.getInstanceNameAttr());
    leaf.addAttribute("occurrence", original.getOccurrence());
    leaf.addAttribute(
        "callee",
        FlatSymbolRefAttr::get(
            staged.getContext(),
            prepared.dffe->getAttrOfType<StringAttr>("sym_name").getValue()));
    leaf.addAttribute("parameters", attributes.getArrayAttr({}));
    leaf.addAttribute(
        "type_arguments",
        attributes.getArrayAttr({TypeAttr::get(original.getQ().getType())}));
    auto instance = cast<ac::InstanceOp>(Operation::create(leaf));
    state->getBlock()->getOperations().insert(state->getIterator(), instance);
    state.getQ().replaceAllUsesWith(instance.getResult(0));
    mapping.map(original.getQ(), instance.getResult(0));
    storage.push_back({original, instance});
    mapping.map(original.operator->(), instance.operator->());
    state.erase();
  }

  struct Replacement {
    ac::SourceTransactionOp original;
    ac::RuleOp rule;
  };
  SmallVector<Replacement> replacements;
  for (const auto &transaction : prepared.transactions) {
    RuleMaterializer builder(transaction, mapping, routes, budget);
    auto rule = builder.run();
    if (failed(rule))
      return failure();
    auto original = transaction.plan->transaction;
    auto materializedRule = rule->rule;
    records.push_back(std::move(*rule));
    auto &materialized = records.back();
    materialized.bodyChecks.resize(transaction.plan->bodyChecks.size());
    materialized.observations.resize(transaction.plan->observations.size());
    replacements.push_back({original, materializedRule});
    OpBuilder expectBuilder(materializedRule);
    expectBuilder.setInsertionPointAfter(materializedRule);
    for (auto [ordinal, control] :
         llvm::enumerate(transaction.plan->controls)) {
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
                          2 * (transaction.plan->controls.size() + id);
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
    for (auto binding : transaction.outputs) {
      const auto &owner = transaction.plan->owners[binding.binding.owner];
      auto path = anchorPath(*transaction.plan, owner.anchor);
      auto declaration = owner.declaration;
      nextValues[transaction.plan->lexicalModule].try_emplace(
          routingKey(path, declaration.getQ()),
          std::make_pair(materializedRule->getResult(binding.dataResult),
                         materializedRule->getResult(binding.enableResult)));
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

  // A candidate originates in the module's outer Rule or an incoming parent
  // route. Absent either, E is explicitly zero for this actual occurrence.
  auto candidate = [&](ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                       Value state, Type dType, Type eType,
                       DictionaryAttr occurrence, Location location,
                       OpBuilder &builder) -> std::pair<Value, Value> {
    auto key = routingKey(path, state);
    auto local = nextValues[module].find(key);
    if (local != nextValues[module].end())
      return local->second;
    auto &definition =
        prepared
            .definitions[prepared.definitionIndex.lookup(module.operator->())];
    auto found = definition.downwardIndex.find(key);
    if (found != definition.downwardIndex.end())
      return incoming[module][found->second];
    // Unrouted local storage retains the established D=Q hold wiring. Only
    // unused external proposal ports require a known-zero data input.
    if (path.empty())
      return {mapping.lookup(state),
              bitsLiteral(eType, occurrence, location, false, builder)};
    return {bitsLiteral(dType, occurrence, location, false, builder),
            bitsLiteral(eType, occurrence, location, false, builder)};
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
        unsigned input = definition.sourceInputs + 2 * ordinal;
        auto [d, e] =
            candidate(parent.module, path, route.value,
                      allocation.getOperand(input).getType(),
                      allocation.getOperand(input + 1).getType(),
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
    row.instance->setOperands({mapping.lookup(original.getClk()),
                               mapping.lookup(original.getRst()), enable, d,
                               mapping.lookup(original.getInitial())});
  }
  // These temporary operands made detached instance reconstruction possible.
  // All final pairs were independently supplied above; retain no unused nodes.
  for (Value zero : stagingZeros) {
    if (!charge(budget, staged, 1))
      return failure();
    if (zero.use_empty())
      zero.getDefiningOp()->erase();
  }
  for (auto replacement : replacements) {
    auto old = mapping.lookup(replacement.original.operator->());
    if (failed(eraseTargetMappings(mapping, old, budget)))
      return failure();
    mapping.map(replacement.original.operator->(),
                replacement.rule.operator->());
    old->erase();
  }
  for (auto module : staged.getOps<ac::ModuleOp>())
    for (auto method : llvm::make_early_inc_range(
             module.getBody().front().getOps<ac::SourceMethodOp>())) {
      if (failed(eraseTargetMappings(mapping, method, budget)))
        return failure();
      method.erase();
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
// producer's aliases, frame maps and supplied validity records. The sorted
// index is bounded once; lookups pay their actual comparisons.
class SourceReplayValues {
public:
  SourceReplayValues(const ac::TransactionStatePlan &plan,
                     SourceCheckBudget &budget)
      : plan(plan), budget(budget),
        hardware(plan.transaction->getParentOfType<mlir::ModuleOp>()) {}

  LogicalResult initialize() {
    uint64_t n = plan.values.size(), levels = 0;
    for (uint64_t size = n; size > 1; size = (size + 1) / 2)
      ++levels;
    if (!product(budget, plan.transaction, n,
                 3 + 2 * levels +
                     SourceCheckBudget::bytes(
                         sizeof(Index) + sizeof(ac::TransactionPoint) +
                         sizeof(bool) +
                         2 * sizeof(std::optional<ac::BoundValueId>))) ||
        !product(
            budget, plan.transaction, plan.branches.size(),
            1 + SourceCheckBudget::bytes(sizeof(ac::TransactionSourcePath))))
      return failure();
    if (n > index.max_size() || n > validity.max_size() ||
        n > assigned.max_size() || n > edges.max_size() ||
        n > resultEdges.max_size() || plan.branches.size() > parents.max_size())
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
      index.push_back({ref.anchor, ref.frame, ref.value, id});
      if (lexical &&
          ref.value.getDefiningOp() != plan.transaction.operator->()) {
        if (failed(assignValidity(id, {})))
          return failure();
      }
    }
    llvm::sort(index, less);
    for (uint64_t i = 1; i < index.size(); ++i)
      if (!less(index[i - 1], index[i]))
        return error("duplicate bound value identity");
    return success();
  }

  FailureOr<ac::BoundValueId> find(Value value, ac::LexicalInstanceId anchor,
                                   ac::InvocationFrameId frame) {
    if (anchor >= plan.instances.size())
      return failure();
    if (moduleValue(value, plan.instances[anchor].definition))
      frame = plan.rootFrame;
    Index key{anchor, frame, value, 0};
    size_t low = 0, high = index.size();
    while (low < high) {
      if (!charge(budget, plan.transaction))
        return failure();
      size_t mid = low + (high - low) / 2;
      if (less(index[mid], key))
        low = mid + 1;
      else
        high = mid;
    }
    if (!charge(budget, plan.transaction) || low == index.size() ||
        less(key, index[low]) || less(index[low], key)) {
      (void)error("original SSA is absent from bound values");
      return failure();
    }
    return index[low].id;
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
  struct Index {
    ac::LexicalInstanceId anchor;
    ac::InvocationFrameId frame;
    Value value;
    ac::BoundValueId id;
  };
  static bool less(const Index &lhs, const Index &rhs) {
    if (lhs.anchor != rhs.anchor)
      return lhs.anchor < rhs.anchor;
    if (lhs.frame != rhs.frame)
      return lhs.frame < rhs.frame;
    return std::less<const void *>{}(lhs.value.getAsOpaquePointer(),
                                     rhs.value.getAsOpaquePointer());
  }
  LogicalResult error(StringRef message) {
    return plan.transaction->emitOpError()
           << "transaction source replay: " << message;
  }
  const ac::TransactionStatePlan &plan;
  SourceCheckBudget &budget;
  ac::HardwareAnalysis hardware;
  SmallVector<Index, 0> index;
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
    bool paid = true;
    staged.walk([&](Operation *op) {
      if (!paid)
        return;
      paid = charge(budget, prepared.original,
                    8 + 4 * op->getNumResults() + op->getNumRegions());
      if (!paid)
        return;
      operations.insert(op);
      for (auto result : op->getResults())
        values.insert(result);
      for (Region &region : op->getRegions())
        for (Block &block : region) {
          if (!charge(budget, prepared.original,
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
    if (failed(routingProof()))
      return failure();
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
                               Block *parent) {
    if (!charge(budget, prepared.original,
                16 + 4 * operands.size() + 4 * types.size() +
                    attributes.size()))
      return failure();
    if (!op || !operations.contains(op) ||
        op->getName().getStringRef() != name || op->getBlock() != parent ||
        op->getLoc() != location || op->getNumRegions() ||
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

  LogicalResult candidatePairProof(ac::ModuleOp originalModule,
                                   ArrayRef<ac::InstanceOp> path, Value state,
                                   Value data, Value enable, Type dataType,
                                   Type enableType, DictionaryAttr occurrence,
                                   Location location) {
    if (!charge(budget, originalModule, 16 + 8 * path.size()))
      return failure();
    const auto &definition =
        prepared.definitions[prepared.definitionIndex.lookup(
            originalModule.operator->())];
    auto module = cast<ac::ModuleOp>(
        candidate.packageMap.lookup(originalModule.operator->()));
    auto key = routingKey(path, state);
    // The writer is selected from original final owner versions, never final
    // companion attributes or a self-authored candidate allowlist.
    for (auto index : definition.transactionIndices) {
      const auto &transaction = prepared.transactions[index];
      const auto &plan = *transaction.plan;
      for (auto [ordinal, output] : llvm::enumerate(transaction.outputs)) {
        if (!charge(budget, originalModule, 8))
          return failure();
        const auto &owner = plan.owners[output.binding.owner];
        auto declaration = owner.declaration;
        if (!product(budget, originalModule, owner.instancePath.size(), 8))
          return failure();
        auto ownerPath = anchorPath(plan, owner.anchor);
        if (routingKey(ownerPath, declaration.getQ()) != key)
          continue;
        auto rule = candidate.transactions[index].rule;
        if (data != rule->getResult(output.dataResult) ||
            enable != rule->getResult(output.enableResult))
          return reject("original local candidate/fire routing pair differs");
        return success();
      }
    }
    auto incoming = definition.downwardIndex.find(key);
    if (incoming != definition.downwardIndex.end()) {
      auto &body = module.getBody().front();
      unsigned input = definition.sourceInputs + 2 * incoming->second;
      if (data != body.getArgument(input) ||
          enable != body.getArgument(input + 1))
        return reject("original incoming candidate routing pair differs");
      return success();
    }
    if (path.empty()) {
      if (data != candidate.packageMap.lookupOrNull(state))
        return reject("original unwritten state hold data differs");
    } else if (failed(literalProof(data, dataType, occurrence, location, false,
                                   &module.getBody().front()))) {
      return failure();
    }
    return literalProof(enable, enableType, occurrence, location, false,
                        &module.getBody().front());
  }

  LogicalResult routingProof() {
    for (auto state : prepared.states) {
      auto module = state->getParentOfType<ac::ModuleOp>();
      auto storage =
          cast<ac::InstanceOp>(candidate.packageMap.lookup(state.operator->()));
      if (failed(candidatePairProof(
              module, {}, state.getQ(), storage->getOperand(3),
              storage->getOperand(2), state.getQ().getType(),
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
    if (targetType.getNumInputs() !=
            definition.sourceInputs + 2 * definition.downward.size() ||
        targetType.getNumResults() !=
            definition.sourceOutputs + definition.upward.size() ||
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
    for (auto [ordinal, route] : llvm::enumerate(definition.upward))
      if (targetType.getResult(definition.sourceOutputs + ordinal) !=
          route.type)
        return reject("original appended upward route type differs");
    if (definition.upward.empty() && definition.downward.empty()) {
      if (module->getAttr("ac.internal_ports") !=
          original->getAttr("ac.internal_ports"))
        return reject("original unextended internal port metadata differs");
    } else {
      Builder b(original.getContext());
      SmallVector<Attribute> defaults(2 * definition.downward.size(),
                                      b.getI64IntegerAttr(0));
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
                           "ac.internal_ports", "ac.transaction_wiring"}) {
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
      if (!product(
              budget, &op, op.getNumResults(),
              SourceCheckBudget::bytes(sizeof(std::pair<Value, Value>) + 64)))
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
        auto storage = dyn_cast<ac::InstanceOp>(target);
        auto dffe = prepared.dffe;
        if (!storage || storage->getNumOperands() != 5 ||
            storage->getNumResults() != 1 ||
            storage.getOccurrence() != state.getOccurrence() ||
            storage.getInstanceNameAttr() != state.getInstanceNameAttr() ||
            storage.getCalleeAttr() !=
                FlatSymbolRefAttr::get(original.getContext(),
                                       dffe.getSymName()) ||
            storage.getParameters() !=
                ArrayAttr::get(original.getContext(), {}) ||
            storage.getTypeArguments() !=
                ArrayAttr::get(original.getContext(),
                               {TypeAttr::get(state.getQ().getType())}) ||
            storage->getResult(0).getType() != state.getQ().getType() ||
            candidate.packageMap.lookupOrNull(state.getQ()) !=
                storage->getResult(0) ||
            storage->getOperand(0) !=
                authenticated.lookupOrNull(state.getClk()) ||
            storage->getOperand(1) !=
                authenticated.lookupOrNull(state.getRst()) ||
            storage->getOperand(4) !=
                authenticated.lookupOrNull(state.getInitial()))
          return reject("original state owner/Q/type/domain/initial differs");
        Builder b(original.getContext());
        if (failed(generatedProof(
                storage, ac::InstanceOp::getOperationName(),
                {authenticated.lookup(state.getClk()),
                 authenticated.lookup(state.getRst()), storage->getOperand(2),
                 storage->getOperand(3),
                 authenticated.lookup(state.getInitial())},
                {state.getQ().getType()},
                b.getDictionaryAttr(
                    {b.getNamedAttr("instance_name",
                                    state.getInstanceNameAttr()),
                     b.getNamedAttr("occurrence", state.getOccurrence()),
                     b.getNamedAttr(
                         "callee", FlatSymbolRefAttr::get(original.getContext(),
                                                          dffe.getSymName())),
                     b.getNamedAttr("parameters", b.getArrayAttr({})),
                     b.getNamedAttr("type_arguments",
                                    b.getArrayAttr({TypeAttr::get(
                                        state.getQ().getType())}))}),
                state.getLoc(), &module.getBody().front())))
          return failure();
        authenticated.map(state.getQ(), storage->getResult(0));
      } else if (auto allocation = dyn_cast<ac::InstanceOp>(op)) {
        auto child =
            dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(allocation));
        unsigned extraInputs = 0, extraOutputs = 0;
        if (child) {
          const auto &routing =
              prepared.definitions[prepared.definitionIndex.lookup(
                  child.operator->())];
          extraInputs = 2 * routing.downward.size();
          extraOutputs = routing.upward.size();
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
                definition.sourceOutputs + definition.upward.size())
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

  LogicalResult transactionProof(const TransactionPreparation &preparation,
                                 const MaterializedTransaction &record) {
    const auto &plan = *preparation.plan;
    auto transaction = plan.transaction;
    auto rule = record.rule;
    if (plan.rootFrame != 0 || plan.frames.empty() || plan.instances.empty() ||
        plan.guard != plan.frames[0].guard || plan.frames[0].parent ||
        plan.frames[0].method || plan.frames[0].call ||
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
    uint64_t expectedTotal = SourceCheckBudget::add(
        cursor, SourceCheckBudget::multiply(
                    2, SourceCheckBudget::add(plan.controls.size(),
                                              plan.bodyChecks.size())));
    auto expectedClock = llvm::find(preparation.liveIns, plan.clockValue) -
                         preparation.liveIns.begin();
    auto expectedReset = llvm::find(preparation.liveIns, plan.resetValue) -
                         preparation.liveIns.begin();
    if (preparation.clockCapture != expectedClock ||
        preparation.resetCapture != expectedReset ||
        preparation.fireResult != expectedFire ||
        preparation.checkBase != cursor ||
        preparation.totalResults != expectedTotal ||
        preparation.expectedAdmission.size() != 6 ||
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
        rule->getNumOperands() != preparation.liveIns.size() ||
        rule.getBody().front().getNumArguments() !=
            preparation.liveIns.size() ||
        rule->getNumResults() != preparation.totalResults ||
        rule->getAttr("ac.sampled_admission") !=
            preparation.expectedAdmission ||
        rule->getAttr("ac.required_checks") !=
            preparation.expectedRequiredChecks ||
        rule->hasAttr("ac.guard_location") ||
        record.boundValues.size() != plan.values.size() ||
        record.versions.size() != plan.versions.size() ||
        record.branchResults.size() != plan.branchResults.size() ||
        record.frames.size() != plan.frames.size() ||
        record.prefixes.size() != plan.controls.size() + 1 ||
        record.controls.size() != plan.controls.size() ||
        record.selections.size() != plan.selections.size() ||
        record.bodyChecks.size() != plan.bodyChecks.size() ||
        record.observations.size() != plan.observations.size())
      return reject(
          "original transaction layout/domain/check metadata differs");
    if (failed(cover(rule)))
      return failure();
    NamedAttrList expected(transaction->getAttrs()), actual(rule->getAttrs());
    expected.erase("ac.guard_location");
    actual.erase("ac.required_checks");
    actual.erase("ac.sampled_admission");
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
      for (auto [frame, binding] : llvm::enumerate(plan.frames))
        if (binding.anchor == ref.anchor)
          authenticated[frame].map(ref.value, argument);
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
        versions[id] = {bound(*version.value), record.versions[id].rawIntent};
        environment[version.owner] = id;
        if (!versions[id].data ||
            failed(literalProof(
                versions[id].rawIntent,
                rule->getOperand(preparation.clockCapture).getType(),
                transaction.getOccurrence(), transaction.getLoc(), false,
                &rule.getBody().front())))
          return reject("original old-Q version is absent");
      }
    Operation *last = nullptr;
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
    uint64_t eventIndex = 0, readIndex = 0, writeIndex = 0, callIndex = 0,
             branchIndex = 0, joinIndex = 0, resultIndex = 0, guardIndex = 0;
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
      case K::Read:
        expected.read = id;
        break;
      case K::Write:
        expected.write = id;
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
          actual.read != expected.read ||
          actual.selection != expected.selection ||
          actual.write != expected.write || actual.guard != expected.guard ||
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
      if (id >= plan.owners.size() || !selectors.empty() || !indices.empty() ||
          !path || !payload)
        return reject("original whole-state access shape differs");
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
          !ac::areEquivalentHardwareTypes(*expected, owner.payloadType) ||
          !ac::areEquivalentHardwareTypes(*type, *expected))
        return reject("original state access payload type differs");
      return success();
    };
    auto selectProof = [&](Value selected, Value condition, Value whenTrue,
                           Value whenFalse, Type resultType,
                           Location location) -> LogicalResult {
      Builder b(transaction.getContext());
      if (!selected || !condition || !whenTrue || !whenFalse || !resultType ||
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
      for (Operation &op : source.without_terminator()) {
        if (!charge(budget, &op, 8))
          return failure();
        if (auto read = dyn_cast<ac::SourceStateReadOp>(op)) {
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
          if (writeIndex == plan.writes.size())
            return reject("original write inventory differs");
          auto found = plan.writes.begin() + writeIndex++;
          if (found->frame != frame || found->operation != write)
            return reject("original write intent is absent");
          Value rhs = bound(found->value);
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
              found->selectors != write.getSelectors() ||
              bound(found->authoredPath) !=
                  authenticated[frame].lookupOrNull(write.getPath()) ||
              failed(ownerProof(write, frame, found->owner, write.getPath(),
                                write.getValue(), write.getSelectors(),
                                write.getIndices(), found->occurrence)) ||
              rhs != authenticated[frame].lookupOrNull(write.getValue()) ||
              failed(event(ac::TransactionStateEventKind::Write, frame,
                           writeIndex - 1)))
            return reject("original write owner/predecessor/RHS differs");
          if (failed(replay.identify(found->authoredPath, write.getPath(),
                                     plan.frames[frame].anchor, frame)) ||
              failed(replay.authoredTrue(found->authoredPath, point())) ||
              failed(replay.identify(found->value, write.getValue(),
                                     plan.frames[frame].anchor, frame)) ||
              failed(replay.requireAvailable(found->value, point())) ||
              failed(replay.requireVersionAvailable(
                  found->predecessor, versionValidity[found->predecessor],
                  point())) ||
              !sameSourcePoint(version.validity, point()))
            return failure();
          versionValidity[found->version] = point();
          Value intent = record.versions[found->version].rawIntent;
          if (failed(literalProof(intent, bit, transaction.getOccurrence(),
                                  op.getLoc(), true, &rule.getBody().front())))
            return failure();
          versions[found->version] = {rhs, intent};
          environment[found->owner] = found->version;
          originalWriters[found->owner] = true;
          if (failed(touch(found->owner)))
            return failure();
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
                             ac::SourceObserveOp>(nested);
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
          SmallVector<bool, 0> armTouched(environment.size(), false);
          SmallVector<ac::StateOwnerId, 0> armOrder;
          if (failed(statements(split.getThenRegion().front(), frame,
                                armTouched, armOrder, depth + 1)) ||
              failed(event(ac::TransactionStateEventKind::ThenExit, frame, id)))
            return failure();
          if (branch.thenExitPrefix != admission)
            return reject("original then exit prefix differs");
          SmallVector<ac::StateVersionId, 0> whenTrue(environment);
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
            if (failed(selectProof(
                    actual.data, condition, versions[join.whenTrue].data,
                    versions[join.whenFalse].data,
                    plan.owners[owner].payloadType, split.getLoc())) ||
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
            auto actual = record.branchResults[resultIndex].select;
            const auto &resultRef = plan.values[result.result];
            auto type = hardware.resolveType(
                original.getType(), plan.instances[resultRef.anchor].bindings,
                split);
            if (result.branch != id || result.frame != frame ||
                result.ordinal != ordinal ||
                plan.values[result.result].value != original ||
                !sameSourcePoint(result.validity, point()) || failed(type) ||
                !ac::areEquivalentHardwareTypes(result.payloadType, *type) ||
                !actual || trueValue != bound(result.whenTrue) ||
                falseValue != bound(result.whenFalse) ||
                actual.getResult().getType() != result.payloadType ||
                record.frames[frame].lookupOrNull(original) !=
                    actual.getResult() ||
                failed(selectProof(actual.getResult(), condition, trueValue,
                                   falseValue, result.payloadType,
                                   split.getLoc())) ||
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
            authenticated[frame].map(original, actual.getResult());
            ++resultIndex;
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
          bool resolverPaid = succeeded(
              prepayClone(call->getParentOfType<ac::ModuleOp>(), budget));
          if (!resolverPaid)
            return failure();
          auto resolved = ac::resolveSourceMethodCall(
              call, plan.frames[frame].bindings,
              [&](FlatSymbolRefAttr symbol) -> Operation * {
                auto *target = hardware.lookupDefinition(symbol.getValue());
                if (target && failed(prepayClone(target, budget))) {
                  resolverPaid = false;
                  return nullptr;
                }
                return target;
              });
          if (!resolverPaid || failed(resolved) ||
              !charge(budget, call,
                      32 + resolved->bindings.integers.size() +
                          resolved->bindings.types.size()) ||
              !resolved->method.definition ||
              calleeFrame.method != resolved->method.definition ||
              calleeFrame.transaction != transaction ||
              calleeFrame.parent != frame || calleeFrame.call != call ||
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
          if (failed(verifyClosedPurePayload(
                  &op, budget,
                  "transaction source proof requires closed pure payload "
                  "operations")))
            return failure();
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
                        1 + nested->getNumOperands() + nested->getNumResults()))
              return failure();
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
      if (frame) {
        if (!charge(budget, transaction, plan.calls.size()))
          return failure();
        auto found = llvm::find_if(
            plan.calls, [&](const auto &call) { return call.callee == frame; });
        if (found == plan.calls.end())
          return reject("original frame has no call");
        callBinding = &*found;
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
            originalActual = originalCall.getArguments()[ordinal];
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
      if (frame) {
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
      if (!sameSourcePoint(binding.exit, point()))
        return reject("original invocation exit differs");
      return success();
    };
    SmallVector<ac::StateOwnerId, 0> rootTouched;
    if (failed(event(ac::TransactionStateEventKind::TransactionEnter,
                     plan.rootFrame)) ||
        failed(invocation(plan.rootFrame, rootTouched)))
      return failure();
    auto originalYield =
        cast<ac::YieldOp>(transaction.getBody().front().getTerminator());
    if (plan.results.size() != transaction->getNumResults() ||
        plan.results.size() != originalYield.getNumOperands())
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
        failed(requireStaticTotality(plan, budget)))
      return failure();
    if (eventIndex != plan.events.size() || readIndex != plan.reads.size() ||
        writeIndex != plan.writes.size() || callIndex != plan.calls.size() ||
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
          (original.value && versions[id].data != bound(*original.value)))
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
    auto sourceYield =
        cast<ac::YieldOp>(transaction.getBody().front().getTerminator());
    if (plan.results.size() != sourceYield.getNumOperands())
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
    Value prefix = record.prefixes[0];
    if (failed(literalProof(prefix, bit, transaction.getOccurrence(),
                            transaction.getLoc(), true,
                            &rule.getBody().front())))
      return failure();
    for (auto [id, control] : llvm::enumerate(plan.controls)) {
      const auto &actual = record.controls[id];
      bool selection =
          std::holds_alternative<ac::BranchSelectControl>(control.boundary);
      Location location = transaction.getLoc();
      if (auto *guard =
              std::get_if<ac::CallableGuardControl>(&control.boundary))
        location = plan.guards[guard->guard].callable->getLoc();
      else
        location =
            plan.branches[plan.selections[std::get<ac::BranchSelectControl>(
                                              control.boundary)
                                              .selection]
                              .branch]
                .operation->getLoc();
      auto selected = sourceSelected(control.path);
      if (failed(selected) || actual.raw != sourceRaw[id] ||
          actual.selectedPath != *selected ||
          yield->getOperand(preparation.checkBase + 2 * id) != sourceRaw[id] ||
          yield->getOperand(preparation.checkBase + 2 * id + 1) !=
              actual.reached)
        return reject("original control raw/path/suffix differs");
      Value k0, k1 = actual.accepted;
      if (selection) {
        auto either = actual.accepted.getDefiningOp<ac::BitsBinaryOp>();
        if (!either || either->getNumOperands() != 2)
          return reject("original selector acceptance differs");
        k0 = either->getOperand(0);
        k1 = either->getOperand(1);
        if (failed(knownProof(k0, sourceRaw[id], false, location)) ||
            failed(binaryProof(actual.accepted, k0, k1, "or", location)))
          return failure();
      }
      if (failed(knownProof(k1, sourceRaw[id], true, location)))
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
            std::get<ac::BranchSelectControl>(control.boundary).selection;
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
    if (!rule->getResult(preparation.fireResult).use_empty())
      return reject("original internal fire result has external users");
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
      uint64_t result = preparation.checkBase + 2 * (plan.controls.size() + id);
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
    Operation *lastTerminal = rule.operator->();
    for (auto [ordinal, control] : llvm::enumerate(plan.controls)) {
      ac::InvocationFrameId frame;
      DictionaryAttr occurrence, location;
      StringRef kind;
      if (auto *tag =
              std::get_if<ac::CallableGuardControl>(&control.boundary)) {
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
                                              control.boundary)
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
      auto path = sampledPath(plan, control.path, budget);
      if (failed(path))
        return failure();
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
                            b.getI64IntegerAttr(plan.controls.size() + id))});
        auto bodyRows =
            preparation.expectedAdmission.getAs<ArrayAttr>("body_checks");
        auto invocation = invocationIdentity(source.frame);
        if (failed(invocation))
          return failure();
        auto path = sampledPath(plan, source.point.path, budget);
        if (failed(path))
          return failure();
        if (bodyRows[id] !=
            b.getDictionaryAttr({b.getNamedAttr("id", identity),
                                 b.getNamedAttr("invocation", *invocation),
                                 b.getNamedAttr("path", *path)}))
          return reject("original body-check admission metadata differs");
        if (preparation.expectedRequiredChecks[plan.controls.size() + id] !=
            b.getDictionaryAttr(
                {b.getNamedAttr("id", identity),
                 b.getNamedAttr("kind", b.getStringAttr("assert")),
                 b.getNamedAttr("location", source.location)}))
          return reject("original body-check required metadata differs");
        uint64_t result =
            preparation.checkBase + 2 * (plan.controls.size() + id);
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
        auto path = sampledPath(plan, source.point.path, budget);
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
  DenseSet<Operation *> operations, clones;
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
materializeTransactionLowering(const PreparedTransactionLowering &prepared,
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
                         result.transactions, budget)))
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

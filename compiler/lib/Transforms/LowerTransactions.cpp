#include "LowerTransactions.h"
#include "Dialect/ACIR/HardwareSourceCheckPreflight.h"
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
using detail::MaterializedTransaction;
using detail::MaterializedTransactionLowering;
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
    auto guards = item.expectedAdmission.getAs<ArrayAttr>("guards");
    if (item.expectedAdmission.size() != 6 || !guards || !checks ||
        !observations || guards.size() != plan.guards.size() ||
        checks.size() != plan.bodyChecks.size() ||
        observations.size() != plan.observations.size() ||
        item.observationPathResults.size() != plan.observations.size() ||
        item.expectedRequiredChecks.size() !=
            plan.guards.size() + plan.bodyChecks.size())
      return site.emitOpError()
             << "transaction prepared body-effect inventory is inconsistent";
    uint64_t fire = SourceCheckBudget::add(plan.transaction->getNumResults(),
                                           item.outputs.size());
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
                  2, SourceCheckBudget::add(plan.guards.size(),
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
    for (const auto &check : plan.bodyChecks)
      if (!check.operation)
        return site.emitOpError()
               << "transaction prepared body-check locator is absent";
    for (const auto &observation : plan.observations)
      if (!observation.operation)
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
    auto fire = add(transaction->getNumResults(), prepared.outputs.size(),
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
    auto checkCount = add(plan.guards.size(), plan.bodyChecks.size(),
                          "transaction check inventory");
    if (failed(checkCount) ||
        *checkCount > std::numeric_limits<uint64_t>::max() / 2)
      return transaction.emitOpError()
             << "transaction check result count overflows";
    auto total =
        add(prepared.checkBase, 2 * *checkCount, "transaction result layout");
    if (failed(total) || *total > std::numeric_limits<size_t>::max())
      return transaction.emitOpError()
             << "transaction result layout exceeds host capacity";
    prepared.totalResults = *total;
    SmallVector<Attribute> guards, bodyChecks, observations, checks;
    for (auto [ordinal, guard] : llvm::enumerate(plan.guards)) {
      auto frames = invocation(guard.frame);
      if (failed(frames) ||
          !charge(budget, transaction, 48 + 2 * frames->size()))
        return failure();
      auto id = b.getDictionaryAttr(
          {b.getNamedAttr("registration", transaction.getOccurrence()),
           b.getNamedAttr("check",
                          guard.callable->getAttr(
                              isa<ac::SourceTransactionOp>(guard.callable)
                                  ? "declaration"
                                  : "occurrence")),
           b.getNamedAttr("obligation", b.getI64IntegerAttr(ordinal))});
      guards.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("id", id), b.getNamedAttr("invocation", *frames),
           b.getNamedAttr("predecessor",
                          b.getI64IntegerAttr(int64_t(ordinal) - 1))}));
      checks.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("id", id),
           b.getNamedAttr("kind", b.getStringAttr("guard_known")),
           b.getNamedAttr("location", guard.location)}));
    }
    for (auto [ordinal, check] : llvm::enumerate(plan.bodyChecks)) {
      auto frames = invocation(check.frame);
      if (failed(frames) || !charge(budget, transaction, 40 + frames->size()))
        return failure();
      auto id = b.getDictionaryAttr(
          {b.getNamedAttr("registration", transaction.getOccurrence()),
           b.getNamedAttr("check", check.occurrence),
           b.getNamedAttr("obligation",
                          b.getI64IntegerAttr(plan.guards.size() + ordinal))});
      bodyChecks.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("id", id), b.getNamedAttr("invocation", *frames)}));
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
      observations.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("id", id),
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
         b.getNamedAttr("guards", b.getArrayAttr(guards)),
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
        const auto &owner = plan.owners[output.owner];
        auto path = anchorPath(plan, owner.anchor);
        if (!charge(budget, definition.module, 32 + path.size() * 8))
          return failure();
        candidates.push_back(b.getDictionaryAttr(
            {b.getNamedAttr("state", stateID(path, owner.declaration)),
             b.getNamedAttr(
                 "result", b.getI64IntegerAttr(
                               plan.transaction->getNumResults() + ordinal))}));
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
    for (const auto &guard : plan.guards) {
      if (!charge(budget, transaction))
        return failure();
      if (transaction->getNumResults() &&
          (!guard.predicateFact || !guard.predicateFact.getValue()))
        return guard.callable->emitOpError()
               << "transaction lowering requires proven-true active guards";
    }
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
      if (plan.versions[binding.version].kind != ac::StateVersionKind::Write)
        continue;
      prepared.outputs.push_back(binding);
      const auto &owner = plan.owners[binding.owner];
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
    uint64_t storage = SourceCheckBudget::add(
        SourceCheckBudget::multiply(plan.frames.size(), sizeof(IRMapping)),
        SourceCheckBudget::multiply(plan.values.size() + plan.versions.size(),
                                    sizeof(Value)));
    uint64_t effectValues = 0;
    for (const auto &observation : plan.observations)
      effectValues =
          SourceCheckBudget::add(effectValues, observation.values.size());
    if (!charge(budget, transaction, SourceCheckBudget::bytes(storage)) ||
        !product(budget, transaction, plan.guards.size(), 64) ||
        !charge(budget, transaction,
                SourceCheckBudget::add(
                    SourceCheckBudget::multiply(plan.bodyChecks.size(),
                                                SourceCheckBudget::bytes(sizeof(
                                                    ac::TransactionBodyCheck))),
                    SourceCheckBudget::add(
                        SourceCheckBudget::multiply(
                            plan.observations.size(),
                            SourceCheckBudget::bytes(
                                sizeof(ac::TransactionObservation))),
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
                    prepared.outputs.size()) ||
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
                 ac::SourceObserveOp, ac::YieldOp>(op) &&
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
        versions(plan.versions.size()) {}

  FailureOr<MaterializedTransaction> run() {
    auto transaction = plan.transaction;
    auto staged = cast<ac::SourceTransactionOp>(
        packageMap.lookup(transaction.operator->()));
    if (!charge(budget, transaction,
                SourceCheckBudget::add(
                    prepared.totalResults,
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
    for (auto output : prepared.outputs)
      types.push_back(plan.owners[output.owner].payloadType);
    Type predicateType = captures[prepared.clockCapture].getType();
    types.push_back(predicateType);
    for (const auto &observation : plan.observations) {
      types.push_back(predicateType);
      for (auto value : observation.values)
        types.push_back(plan.values[value].value.getType());
    }
    for (const auto &guard : plan.guards) {
      types.push_back(plan.values[guard.predicate].value.getType());
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
        if (ref.anchor == binding.anchor)
          frames[frame].map(ref.value, lexical.lookup({ref.anchor, ref.value}));
      }
    for (auto [id, version] : llvm::enumerate(plan.versions))
      if (version.kind == ac::StateVersionKind::OldQ) {
        auto value = bound(version.value);
        if (failed(value))
          return failure();
        versions[id] = *value;
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
    for (const auto &event : plan.events)
      if (event.kind == ac::TransactionStateEventKind::BodyCheck ||
          event.kind == ac::TransactionStateEventKind::Observe)
        effectEvents.push_back(&event);
    savedChecks.resize(plan.bodyChecks.size());
    savedObservations.resize(plan.observations.size());
    OpBuilder builder(rule.getContext());
    builder.setInsertionPointToEnd(block);
    prefix = bitsLiteral(captures[prepared.clockCapture].getType(),
                         transaction.getOccurrence(), transaction.getLoc(),
                         true, builder);
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
      if (!versions[output.version])
        return transaction.emitOpError()
               << "transaction final candidate is unmapped";
      results.push_back(versions[output.version]);
    }
    results.push_back(prefix);
    for (const auto &observation : savedObservations) {
      if (!observation.path)
        return transaction.emitOpError()
               << "transaction observation payload is unmapped";
      results.push_back(prefix);
      llvm::append_range(results, observation.values);
    }
    for (auto pair : guards) {
      results.push_back(pair.first);
      results.push_back(pair.second);
    }
    for (const auto &check : savedChecks) {
      if (!check.first || !check.second)
        return transaction.emitOpError()
               << "transaction body-check payload is unmapped";
      results.push_back(check.first);
      results.push_back(prefix);
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
    for (auto [id, guard] : llvm::enumerate(guards)) {
      uint64_t result = prepared.checkBase + 2 * id;
      rule->getResult(result).setType(guard.first.getType());
      rule->getResult(result + 1).setType(guard.second.getType());
    }
    for (auto [id, check] : llvm::enumerate(savedChecks)) {
      uint64_t result = prepared.checkBase + 2 * (plan.guards.size() + id);
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
    record.versions = std::move(versions);
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

  LogicalResult invocation(ac::InvocationFrameId frame, OpBuilder &builder) {
    const auto &binding = plan.frames[frame];
    for (ArrayRef<ac::TransactionValueBinding> arguments :
         {ArrayRef<ac::TransactionValueBinding>(binding.guardArguments),
          ArrayRef<ac::TransactionValueBinding>(binding.bodyArguments)})
      for (const auto &argument : arguments) {
        auto actual = bound(argument.actual);
        if (failed(actual))
          return failure();
        frames[frame].map(argument.formal, *actual);
      }
    auto method = binding.method;
    auto transaction = plan.transaction;
    Operation *function =
        method ? method.operator->() : transaction.operator->();
    for (auto [regionOrdinal, region] :
         llvm::enumerate(function->getRegions())) {
      for (Operation &op : region.front().without_terminator()) {
        if (auto access = dyn_cast<ac::SourceStateReadOp>(op)) {
          auto found = reads.find({frame, access});
          if (found == reads.end() || !versions[found->second->version])
            return access.emitOpError()
                   << "transaction read version is unmapped";
          frames[frame].map(access.getValue(),
                            versions[found->second->version]);
        } else if (auto intent = dyn_cast<ac::SourceStateWriteOp>(op)) {
          auto found = writes.find({frame, intent});
          if (found == writes.end())
            return intent.emitOpError() << "transaction write plan is missing";
          auto rhs = bound(found->second->value);
          if (failed(rhs))
            return failure();
          versions[found->second->version] = *rhs;
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
          auto path = bound(record.path);
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
          auto path = bound(record.path);
          if (failed(path))
            return failure();
          savedObservations[found->second].path = *path;
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
      if (regionOrdinal == 0) {
        auto found = llvm::find_if(plan.guards, [&](const auto &guard) {
          return guard.frame == frame;
        });
        if (found == plan.guards.end())
          return function->emitOpError() << "transaction guard plan is missing";
        auto raw = bound(found->predicate);
        if (failed(raw))
          return failure();
        guards.emplace_back(*raw, prefix);
        auto one = bitsLiteral(prefix.getType(), transaction.getOccurrence(),
                               function->getLoc(), true, builder);
        OperationState known(function->getLoc(),
                             ac::BitsCompareOp::getOperationName());
        known.addOperands({*raw, one});
        known.addTypes(prefix.getType());
        known.addAttribute("predicate", builder.getStringAttr("case_eq"));
        auto k = builder.create(known)->getResult(0);
        OperationState next(function->getLoc(),
                            ac::BitsBinaryOp::getOperationName());
        next.addOperands({prefix, k});
        next.addTypes(prefix.getType());
        next.addAttribute("opcode", builder.getStringAttr("and"));
        prefix = builder.create(next)->getResult(0);
      }
    }
    return success();
  }

  const TransactionPreparation &prepared;
  const ac::TransactionStatePlan &plan;
  IRMapping &packageMap;
  using LexicalKey = std::pair<ac::LexicalInstanceId, Value>;
  struct SavedObservation {
    Value path;
    SmallVector<Value, 0> values;
  };
  MaterializationRoutes &routes;
  SourceCheckBudget &budget;
  llvm::DenseMap<LexicalKey, Value> lexical;
  SmallVector<IRMapping, 0> frames;
  SmallVector<Value, 0> versions;
  Value prefix;
  SmallVector<std::pair<Value, Value>> guards;
  llvm::DenseMap<Key, const ac::StateReadBinding *> reads;
  llvm::DenseMap<Key, const ac::StateWriteIntent *> writes;
  llvm::DenseMap<Key, const ac::TransactionCallBinding *> calls;
  llvm::DenseMap<Key, ac::BodyCheckId> bodyCheckPlans;
  llvm::DenseMap<Key, ac::ObservationId> observationPlans;
  SmallVector<const ac::TransactionStateEvent *, 0> effectEvents;
  size_t nextEffectEvent = 0;
  SmallVector<std::pair<Value, Value>, 0> savedChecks;
  SmallVector<SavedObservation, 0> savedObservations;
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
    for (auto [ordinal, guard] : llvm::enumerate(transaction.plan->guards)) {
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
      expect.addAttribute("location", guard.location);
      expect.addAttribute("ac.check_id", check.get("id"));
      expectBuilder.create(expect);
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
        uint64_t result =
            transaction.checkBase + 2 * (transaction.plan->guards.size() + id);
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
    unsigned ordinary = original.getNumResults();
    for (auto [ordinal, binding] : llvm::enumerate(transaction.outputs)) {
      const auto &owner = transaction.plan->owners[binding.owner];
      auto path = anchorPath(*transaction.plan, owner.anchor);
      auto declaration = owner.declaration;
      nextValues[transaction.plan->lexicalModule].try_emplace(
          routingKey(path, declaration.getQ()),
          std::make_pair(materializedRule->getResult(ordinary + ordinal),
                         materializedRule->getResult(transaction.fireResult)));
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
        const auto &owner = plan.owners[output.owner];
        auto declaration = owner.declaration;
        if (!product(budget, originalModule, owner.instancePath.size(), 8))
          return failure();
        auto ownerPath = anchorPath(plan, owner.anchor);
        if (routingKey(ownerPath, declaration.getQ()) != key)
          continue;
        auto rule = candidate.transactions[index].rule;
        if (data !=
                rule->getResult(plan.transaction->getNumResults() + ordinal) ||
            enable != rule->getResult(transaction.fireResult))
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
    uint64_t expectedFire = SourceCheckBudget::add(transaction->getNumResults(),
                                                   preparation.outputs.size());
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
                    2, SourceCheckBudget::add(plan.guards.size(),
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
        record.frames.size() != plan.frames.size() ||
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
    SmallVector<Value, 0> versions(plan.versions.size());
    auto bound = [&](ac::BoundValueId id) -> Value {
      const auto &ref = plan.values[id];
      if (ref.value.getDefiningOp() == transaction.operator->())
        return rule->getResult(cast<OpResult>(ref.value).getResultNumber());
      if (moduleValue(ref.value, plan.instances[ref.anchor].definition))
        return lexical.lookup({ref.anchor, ref.value});
      return authenticated[ref.frame].lookupOrNull(ref.value);
    };
    for (auto [id, version] : llvm::enumerate(plan.versions))
      if (version.kind == ac::StateVersionKind::OldQ) {
        versions[id] = bound(version.value);
        if (!versions[id])
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
    std::function<LogicalResult(ac::InvocationFrameId)> invocation;
    invocation = [&](ac::InvocationFrameId frame) -> LogicalResult {
      const auto &binding = plan.frames[frame];
      for (ArrayRef<ac::TransactionValueBinding> arguments :
           {ArrayRef<ac::TransactionValueBinding>(binding.guardArguments),
            ArrayRef<ac::TransactionValueBinding>(binding.bodyArguments)})
        for (const auto &argument : arguments) {
          Value value = bound(argument.actual);
          if (!value ||
              record.frames[frame].lookupOrNull(argument.formal) != value)
            return reject("original formal actual binding differs");
          authenticated[frame].map(argument.formal, value);
        }
      auto method = binding.method;
      Operation *function =
          method ? method.operator->() : transaction.operator->();
      for (auto [regionOrdinal, region] :
           llvm::enumerate(function->getRegions())) {
        for (Operation &op : region.front().without_terminator()) {
          if (!charge(budget, &op, 8))
            return failure();
          if (auto read = dyn_cast<ac::SourceStateReadOp>(op)) {
            auto found = llvm::find_if(plan.reads, [&](const auto &r) {
              return r.frame == frame && r.operation.operator->() == &op;
            });
            if (found == plan.reads.end() || !versions[found->version] ||
                record.frames[frame].lookupOrNull(read.getValue()) !=
                    versions[found->version])
              return reject("original read selected version differs");
            authenticated[frame].map(read.getValue(), versions[found->version]);
          } else if (isa<ac::SourceStateWriteOp>(op)) {
            auto found = llvm::find_if(plan.writes, [&](const auto &w) {
              return w.frame == frame && w.operation.operator->() == &op;
            });
            if (found == plan.writes.end())
              return reject("original write intent is absent");
            Value rhs = bound(found->value);
            const auto &version = plan.versions[found->version];
            if (!rhs || version.owner != found->owner ||
                version.predecessor != found->predecessor ||
                version.value != found->value)
              return reject("original write owner/predecessor/RHS differs");
            versions[found->version] = rhs;
          } else if (isa<ac::SourceMethodCallOp>(op)) {
            auto found = llvm::find_if(plan.calls, [&](const auto &c) {
              return c.caller == frame && c.operation.operator->() == &op;
            });
            if (found == plan.calls.end() || failed(invocation(found->callee)))
              return failure();
            for (const auto &result : found->results) {
              Value value = bound(result.yielded);
              Value source = plan.values[result.result].value;
              if (!value || record.frames[frame].lookupOrNull(source) != value)
                return reject("original call yielded-result binding differs");
              authenticated[frame].map(source, value);
            }
          } else if (auto check = dyn_cast<ac::SourceExpectOp>(op)) {
            auto found = llvm::find_if(plan.bodyChecks, [&](const auto &item) {
              return item.frame == frame && item.operation.operator->() == &op;
            });
            if (found == plan.bodyChecks.end() ||
                nextEffectEvent == effectEvents.size())
              return reject("original body-check plan/event is absent");
            auto id = std::distance(plan.bodyChecks.begin(), found);
            const auto *event = effectEvents[nextEffectEvent++];
            Value condition = bound(found->condition),
                  path = bound(found->path);
            if (event->kind != ac::TransactionStateEventKind::BodyCheck ||
                event->frame != frame || event->bodyCheck != id || !condition ||
                !path || check.getKind() != "assert" ||
                !check.getSampling().empty() ||
                check->getAttr("occurrence") != found->occurrence ||
                check.getLocationAttr() != found->location ||
                check->getAttrOfType<StringAttr>("ac.message") !=
                    found->message ||
                check->hasAttr("ac.check_id") ||
                check->hasAttr("ac.observation_id") ||
                check.getCondition() != plan.values[found->condition].value ||
                check.getPath() != plan.values[found->path].value)
              return reject(
                  "original body-check operation/frame/inputs differ");
            sourceChecks[id] = {condition, path};
          } else if (auto observation = dyn_cast<ac::SourceObserveOp>(op)) {
            auto found =
                llvm::find_if(plan.observations, [&](const auto &item) {
                  return item.frame == frame &&
                         item.operation.operator->() == &op;
                });
            if (found == plan.observations.end() ||
                nextEffectEvent == effectEvents.size())
              return reject("original observation plan/event is absent");
            auto id = std::distance(plan.observations.begin(), found);
            const auto *event = effectEvents[nextEffectEvent++];
            Value path = bound(found->path);
            if (event->kind != ac::TransactionStateEventKind::Observe ||
                event->frame != frame || event->observation != id || !path ||
                observation->getAttr("occurrence") != found->occurrence ||
                observation->getAttr("ac.location") != found->location ||
                observation.getKindAttr() != found->kind ||
                observation.getSpec() != found->spec ||
                observation->hasAttr("ac.check_id") ||
                observation->hasAttr("ac.observation_id") ||
                observation.getPath() != plan.values[found->path].value ||
                observation.getValues().size() != found->values.size())
              return reject("original observation operation/frame/path differ");
            sourceObservations[id].path = path;
            for (auto [ordinal, value] : llvm::enumerate(found->values)) {
              Value mapped = bound(value);
              if (!mapped ||
                  observation.getValues()[ordinal] != plan.values[value].value)
                return reject("original observation payload/order differs");
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
            if (failed(exactClone(&op, actual, authenticated[frame],
                                  record.frames[frame])))
              return failure();
            last = actual;
          }
        }
        if (!regionOrdinal) {
          auto guard = llvm::find_if(
              plan.guards, [&](const auto &g) { return g.frame == frame; });
          if (guard == plan.guards.end() || !bound(guard->predicate))
            return reject("original guard predicate is absent");
          predicates.push_back(bound(guard->predicate));
        }
      }
      return success();
    };
    if (failed(invocation(plan.rootFrame)))
      return failure();
    if (nextEffectEvent != effectEvents.size())
      return reject("original body-effect event inventory is incomplete");
    for (auto [id, original] : llvm::enumerate(plan.values)) {
      Value expectedValue = bound(id);
      if (record.boundValues[id] != expectedValue)
        return reject("saved bound-value map differs from original SSA");
    }
    for (auto [id, original] : llvm::enumerate(plan.versions))
      if (!versions[id] || record.versions[id] != versions[id] ||
          versions[id] != bound(original.value))
        return reject("saved state-version map differs from original RHS");
    auto yield = cast<ac::YieldOp>(rule.getBody().front().getTerminator());
    Builder yieldAttributes(transaction.getContext());
    SmallVector<Value> yieldOperands(yield->getOperands());
    if (failed(generatedProof(yield, ac::YieldOp::getOperationName(),
                              yieldOperands, {},
                              yieldAttributes.getDictionaryAttr({}),
                              transaction.getLoc(), &rule.getBody().front())))
      return failure();
    if (yield->getNumOperands() != rule->getNumResults())
      return reject("original Rule yield arity differs");
    for (auto [ordinal, result] : llvm::enumerate(plan.results))
      if (yield->getOperand(ordinal) != bound(result.yielded) ||
          candidate.packageMap.lookupOrNull(plan.values[result.result].value) !=
              rule->getResult(ordinal))
        return reject("original ordinary result yield differs");
    for (auto [ordinal, output] : llvm::enumerate(preparation.outputs))
      if (yield->getOperand(transaction->getNumResults() + ordinal) !=
          versions[output.version])
        return reject("original final candidate RHS differs");
    if (predicates.size() != plan.guards.size())
      return reject("original guard invocation order differs");
    for (auto [ordinal, predicate] : llvm::enumerate(predicates))
      if (yield->getOperand(preparation.checkBase + 2 * ordinal) != predicate)
        return reject("original raw guard predicate differs");
    Builder b(transaction.getContext());
    Type bit = rule->getOperand(preparation.clockCapture).getType();
    Value prefix = yield->getOperand(preparation.checkBase + 1);
    if (failed(literalProof(prefix, bit, transaction.getOccurrence(),
                            transaction.getLoc(), true,
                            &rule.getBody().front())))
      return failure();
    for (auto [ordinal, guard] : llvm::enumerate(plan.guards)) {
      if (yield->getOperand(preparation.checkBase + 2 * ordinal + 1) != prefix)
        return reject("original generated guard reached prefix differs");
      Value next =
          ordinal + 1 == plan.guards.size()
              ? yield->getOperand(preparation.fireResult)
              : yield->getOperand(preparation.checkBase + 2 * ordinal + 3);
      Operation *conjunction = next.getDefiningOp();
      if (!conjunction || conjunction->getNumOperands() != 2 ||
          conjunction->getNumResults() != 1 ||
          next != conjunction->getResult(0))
        return reject("original generated guard conjunction is absent");
      Value known = conjunction->getOperand(1);
      Operation *comparison = known.getDefiningOp();
      if (!comparison || comparison->getNumOperands() != 2 ||
          comparison->getNumResults() != 1 || known != comparison->getResult(0))
        return reject("original generated guard knownness is absent");
      Value one = comparison->getOperand(1);
      if (failed(literalProof(one, bit, transaction.getOccurrence(),
                              guard.callable->getLoc(), true,
                              &rule.getBody().front())) ||
          failed(generatedProof(
              comparison, ac::BitsCompareOp::getOperationName(),
              {predicates[ordinal], one}, {bit},
              b.getDictionaryAttr(
                  {b.getNamedAttr("predicate", b.getStringAttr("case_eq"))}),
              guard.callable->getLoc(), &rule.getBody().front())) ||
          failed(generatedProof(
              conjunction, ac::BitsBinaryOp::getOperationName(),
              {prefix, known}, {bit},
              b.getDictionaryAttr(
                  {b.getNamedAttr("opcode", b.getStringAttr("and"))}),
              guard.callable->getLoc(), &rule.getBody().front())))
        return failure();
      prefix = next;
    }
    for (auto [id, observation] : llvm::enumerate(plan.observations)) {
      uint64_t pathResult = preparation.observationPathResults[id];
      if (yield->getOperand(pathResult) != prefix ||
          rule->getResult(pathResult).getType() != prefix.getType() ||
          sourceObservations[id].path != bound(observation.path) ||
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
      uint64_t result = preparation.checkBase + 2 * (plan.guards.size() + id);
      if (yield->getOperand(result) != sourceChecks[id].first ||
          yield->getOperand(result + 1) != prefix ||
          sourceChecks[id].second != bound(check.path) ||
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
    for (auto [ordinal, guard] : llvm::enumerate(plan.guards)) {
      auto check =
          cast<DictionaryAttr>(preparation.expectedRequiredChecks[ordinal]);
      auto expectedID = b.getDictionaryAttr(
          {b.getNamedAttr("registration", transaction.getOccurrence()),
           b.getNamedAttr("check",
                          guard.callable->getAttr(
                              isa<ac::SourceTransactionOp>(guard.callable)
                                  ? "declaration"
                                  : "occurrence")),
           b.getNamedAttr("obligation", b.getI64IntegerAttr(ordinal))});
      if (check != b.getDictionaryAttr(
                       {b.getNamedAttr("id", expectedID),
                        b.getNamedAttr("kind", b.getStringAttr("guard_known")),
                        b.getNamedAttr("location", guard.location)}))
        return reject("original guard required metadata differs");
      auto guardRows = preparation.expectedAdmission.getAs<ArrayAttr>("guards");
      auto invocation = invocationIdentity(guard.frame);
      if (failed(invocation))
        return failure();
      auto expectedGuard = b.getDictionaryAttr(
          {b.getNamedAttr("id", expectedID),
           b.getNamedAttr("invocation", *invocation),
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
            expect->getAttr("location") != guard.location ||
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
                     b.getNamedAttr("location", guard.location),
                     b.getNamedAttr("ac.check_id", check.get("id"))}),
                transaction.getLoc(), &module.getBody().front())))
          return failure();
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
                            b.getI64IntegerAttr(plan.guards.size() + id))});
        auto bodyRows =
            preparation.expectedAdmission.getAs<ArrayAttr>("body_checks");
        auto invocation = invocationIdentity(source.frame);
        if (failed(invocation))
          return failure();
        if (bodyRows[id] !=
            b.getDictionaryAttr({b.getNamedAttr("id", identity),
                                 b.getNamedAttr("invocation", *invocation)}))
          return reject("original body-check admission metadata differs");
        if (preparation.expectedRequiredChecks[plan.guards.size() + id] !=
            b.getDictionaryAttr(
                {b.getNamedAttr("id", identity),
                 b.getNamedAttr("kind", b.getStringAttr("assert")),
                 b.getNamedAttr("location", source.location)}))
          return reject("original body-check required metadata differs");
        uint64_t result = preparation.checkBase + 2 * (plan.guards.size() + id);
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
        if (observationRows[id] !=
            b.getDictionaryAttr(
                {b.getNamedAttr("id", identity),
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

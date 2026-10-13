#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "pycircuit/Transforms/Passes.h"

#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSet.h"
#include <map>

using namespace mlir;

namespace acir {
namespace {

class MaterializationBudget {
public:
  bool charge(Operation *site, uint64_t units = 1) {
    if (units > limit - used) {
      mlir::emitError(UnknownLoc::get(site->getContext()))
          << "transaction materialization work budget exceeded";
      return false;
    }
    used += units;
    return true;
  }

  bool product(Operation *site, uint64_t count, uint64_t units) {
    if (units && count > (limit - used) / units) {
      mlir::emitError(UnknownLoc::get(site->getContext()))
          << "transaction materialization expansion budget exceeded";
      return false;
    }
    return charge(site, count * units);
  }

private:
  static constexpr uint64_t limit = 1024 * 1024;
  uint64_t used = 0;
};

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

LogicalResult prepayClone(Operation *root, MaterializationBudget &budget) {
  SmallVector<Operation *> pending;
  if (!budget.charge(root))
    return failure();
  pending.push_back(root);
  while (!pending.empty()) {
    Operation *op = pending.pop_back_val();
    if (!budget.charge(op, 3 + uint64_t(op->getNumOperands()) * 2 +
                               uint64_t(op->getNumResults()) * 2 +
                               op->getNumRegions() +
                               op->getRawDictionaryAttrs().size() +
                               op->getPropertiesStorageSize()))
      return failure();
    for (Region &region : op->getRegions())
      for (Block &block : region) {
        if (!budget.charge(op, 1 + uint64_t(block.getNumArguments()) * 2))
          return failure();
        for (Operation &nested : block) {
          if (!budget.charge(op))
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

using RoutingKey = std::pair<ArrayAttr, Value>;

struct Route {
  SmallVector<ac::InstanceOp> path;
  Value value;
  Type type;
  // Downward routes carry a whole state candidate and its one-bit enable.
  Type enableType;
};

struct DefinitionRouting {
  ac::ModuleOp module;
  SmallVector<Route> upward;
  SmallVector<Route> downward;
  llvm::DenseMap<RoutingKey, unsigned> upwardIndex, downwardIndex;
  SmallVector<unsigned> transactionIndices;
  unsigned sourceInputs = 0, sourceOutputs = 0;
};

struct TransactionPreparation {
  const ac::TransactionStatePlan *plan = nullptr;
  SmallVector<ac::BoundValueId> liveIns;
  SmallVector<ac::OwnerVersionBinding> outputs;
};

ArrayAttr routePath(MLIRContext *context, ArrayRef<ac::InstanceOp> path) {
  SmallVector<Attribute> occurrences;
  for (auto allocation : path)
    occurrences.push_back(allocation.getOccurrence());
  return ArrayAttr::get(context, occurrences);
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
class LoweringPreparation {
public:
  LoweringPreparation(mlir::ModuleOp package,
                      const ac::SourceTransactionStateAnalysis &analysis,
                      MaterializationBudget &budget)
      : hardware(package), package(package), analysis(analysis),
        budget(budget) {}

  LogicalResult run() {
    if (failed(mergedEnvelope(package)) || failed(prepayClone(package, budget)))
      return failure();
    for (auto imported : package.getOps<ac::ModuleImportOp>()) {
      if (!budget.charge(imported) || failed(hardware.verifyImport(imported)))
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
        if (!budget.charge(&op))
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
          if (!budget.charge(state, 32))
            return failure();
          states.push_back(state);
        } else if (auto transaction = dyn_cast<ac::SourceTransactionOp>(op)) {
          auto *plan = analysis.lookup(transaction);
          if (!plan)
            return transaction.emitOpError()
                   << "transaction lowering requires the current complete "
                      "state plan";
          if (failed(admit(*plan)) || !budget.charge(transaction, 2))
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
          failed(expansion(prepared)))
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
      if (!budget.charge(definition.module, 16 * ports + 1))
        return failure();
      for (auto allocation :
           definition.module.getBody().front().getOps<ac::InstanceOp>()) {
        auto callee =
            dyn_cast_or_null<ac::ModuleOp>(hardware.resolveCallee(allocation));
        if (!callee)
          continue;
        const auto &child = routing(callee);
        if (!budget.charge(
                allocation,
                allocation.getNumOperands() + allocation.getNumResults() +
                    32 * (child.upward.size() + 2 * child.downward.size()) + 1))
          return failure();
        // Each actual occurrence prepends itself to every descendant proposal
        // path, then builds/hashes that path for candidate lookup. Definition
        // union and empty writer summaries do not pay this occurrence cost.
        for (const auto &route : child.downward)
          if (!budget.product(allocation, route.path.size() + 1, 8))
            return failure();
      }
    }
    return success();
  }

  DefinitionRouting &routing(ac::ModuleOp module) {
    return definitions[definitionIndex.lookup(module)];
  }
  RoutingKey key(ArrayRef<ac::InstanceOp> path, Value value) const {
    return {routePath(package->getContext(), path), value};
  }
  SmallVector<TransactionPreparation, 0> transactions;
  SmallVector<ac::SourceStateOp> states;
  SmallVector<DefinitionRouting, 0> definitions;
  ac::ModuleImportOp dffe;
  ac::HardwareAnalysis hardware;

private:
  LogicalResult admit(const ac::TransactionStatePlan &plan) {
    auto transaction = plan.transaction;
    for (const auto &guard : plan.guards) {
      if (!budget.charge(transaction))
        return failure();
      if (!guard.predicateFact || !guard.predicateFact.getValue())
        return guard.callable->emitOpError()
               << "transaction lowering requires proven-true active guards";
    }
    for (const auto &owner : plan.owners) {
      if (!budget.charge(transaction, 1 + owner.instancePath.size()))
        return failure();
      if (!isa<ac::BitsType>(owner.payloadType))
        return transaction.emitOpError()
               << "transaction lowering requires whole fixed Bits state";
    }
    return success();
  }

  LogicalResult demand(ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                       Value value, bool downward) {
    if (!budget.charge(module, 8 + path.size()))
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
      if (!budget.charge(plan.transaction))
        return failure();
      auto module = plan.instances[ref.anchor].definition;
      if (!moduleValue(ref.value, module) ||
          ref.value.getDefiningOp() == plan.transaction.operator->())
        continue;
      prepared.liveIns.push_back(id);
      if (failed(propagate(plan, ref.anchor, ref.value, false)))
        return failure();
    }
    return success();
  }

  LogicalResult outputs(TransactionPreparation &prepared) {
    const auto &plan = *prepared.plan;
    for (auto binding : plan.finalVersions) {
      if (!budget.charge(plan.transaction))
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
      if (!budget.charge(module, 8 + writer.ownerPath.size() +
                                     writer.transactionPath.size()))
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
      if (!budget.charge(module))
        return failure();
      const auto &prepared = transactions[index];
      const auto &plan = *prepared.plan;
      for (const auto &write : plan.writes) {
        const auto &owner = plan.owners[write.owner];
        if (!budget.charge(module, 1 + owner.instancePath.size()))
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
      if (!budget.charge(&op))
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
        if (!budget.charge(module, 3 + childWriter.ownerPath.size() +
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
    if (!budget.charge(transaction, plan.values.size() + plan.instances.size() +
                                        plan.versions.size() +
                                        plan.reads.size() + plan.writes.size() +
                                        plan.calls.size() + plan.frames.size() +
                                        plan.results.size() +
                                        prepared.outputs.size()) ||
        !budget.product(transaction, plan.frames.size(),
                        2 * prepared.liveIns.size() + 1))
      return failure();
    for (const auto &frame : plan.frames) {
      auto method = frame.method;
      Operation *function =
          method ? method.getOperation() : transaction.getOperation();
      if (!budget.charge(function, frame.guardArguments.size() +
                                       frame.bodyArguments.size()))
        return failure();
      SmallVector<Operation *> pending;
      for (Region &region : function->getRegions())
        for (Operation &op : region.front()) {
          if (!budget.charge(function))
            return failure();
          pending.push_back(&op);
        }
      while (!pending.empty()) {
        Operation *op = pending.pop_back_val();
        if (!budget.charge(op, 3 + uint64_t(op->getNumOperands()) * 2 +
                                   uint64_t(op->getNumResults()) * 2 +
                                   op->getNumRegions() +
                                   op->getRawDictionaryAttrs().size() +
                                   op->getPropertiesStorageSize()))
          return failure();
        if (!isa<ac::SourceStateReadOp, ac::SourceStateWriteOp,
                 ac::SourceMethodCallOp, ac::YieldOp>(op) &&
            !ac::detail::isCombinationalHardwareOperation(op))
          return op->emitOpError()
                 << "transaction lowering cannot materialize this operation";
        for (Region &region : op->getRegions())
          for (Block &block : region) {
            if (!budget.charge(op, 1 + uint64_t(block.getNumArguments()) * 2))
              return failure();
            for (Operation &nested : block) {
              if (!budget.charge(op))
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
  MaterializationBudget &budget;
  llvm::DenseMap<Operation *, unsigned> definitionIndex, summaryStatus;
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
  MaterializationRoutes(LoweringPreparation &prepared, IRMapping &mapping)
      : prepared(prepared), mapping(mapping) {}

  FailureOr<Value> value(ac::ModuleOp module, ArrayRef<ac::InstanceOp> path,
                         Value original) {
    if (path.empty()) {
      auto mapped = mapping.lookupOrNull(original);
      if (mapped)
        return mapped;
      return module.emitOpError()
             << "transaction lexical SSA mapping is missing";
    }
    auto child =
        cast<ac::ModuleOp>(prepared.hardware.resolveCallee(path.front()));
    auto &definition = prepared.routing(child);
    auto found =
        definition.upwardIndex.find(prepared.key(path.drop_front(), original));
    if (found == definition.upwardIndex.end())
      return module.emitOpError() << "transaction upward route is missing";
    auto originalAllocation = path.front();
    auto allocation =
        cast<ac::InstanceOp>(mapping.lookup(originalAllocation.getOperation()));
    return allocation.getResult(definition.sourceOutputs + found->second);
  }

  FailureOr<Value> bound(const ac::TransactionStatePlan &plan,
                         ac::BoundValueId id) {
    const auto &ref = plan.values[id];
    auto path = anchorPath(plan, ref.anchor);
    return value(plan.lexicalModule, path, ref.value);
  }

private:
  LoweringPreparation &prepared;
  IRMapping &mapping;
};

class RuleMaterializer {
public:
  RuleMaterializer(const TransactionPreparation &prepared,
                   IRMapping &packageMap, MaterializationRoutes &routes)
      : prepared(prepared), plan(*prepared.plan), packageMap(packageMap),
        routes(routes), frames(plan.frames.size()),
        versions(plan.versions.size()) {}

  FailureOr<ac::RuleOp> run() {
    auto transaction = plan.transaction;
    auto staged = cast<ac::SourceTransactionOp>(
        packageMap.lookup(transaction.getOperation()));
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
    OperationState state(staged.getLoc(), ac::RuleOp::getOperationName());
    state.addOperands(captures);
    state.addTypes(types);
    NamedAttrList attributes(transaction->getAttrs());
    attributes.erase("ac.guard_location");
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
    OpBuilder builder(rule.getContext());
    builder.setInsertionPointToEnd(block);
    if (failed(invocation(plan.rootFrame, builder)))
      return failure();
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
    OperationState yield(staged.getLoc(), ac::YieldOp::getOperationName());
    yield.addOperands(results);
    builder.create(yield);
    return rule;
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
        method ? method.getOperation() : transaction.getOperation();
    for (Region &region : function->getRegions())
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
        } else {
          for (Value operand : op.getOperands())
            if (!frames[frame].contains(operand))
              return op.emitOpError() << "transaction pure operand is unmapped";
          builder.clone(op, frames[frame]);
        }
      }
    return success();
  }

  const TransactionPreparation &prepared;
  const ac::TransactionStatePlan &plan;
  IRMapping &packageMap;
  using LexicalKey = std::pair<ac::LexicalInstanceId, Value>;
  MaterializationRoutes &routes;
  llvm::DenseMap<LexicalKey, Value> lexical;
  SmallVector<IRMapping, 0> frames;
  SmallVector<Value, 0> versions;
  llvm::DenseMap<Key, const ac::StateReadBinding *> reads;
  llvm::DenseMap<Key, const ac::StateWriteIntent *> writes;
  llvm::DenseMap<Key, const ac::TransactionCallBinding *> calls;
};

Value bitsLiteral(Type payload, DictionaryAttr occurrence, Location location,
                  bool one, OpBuilder &builder) {
  auto type = cast<ac::BitsType>(payload);
  auto literal = ac::MathIntAttr::get(
      builder.getContext(), llvm::APSInt(llvm::APInt(1, one ? 1 : 0), true));
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
  OperationState constant(location, ac::BitsConstantOp::getOperationName());
  constant.addTypes(type);
  constant.addAttribute("value",
                        ac::StaticExprAttr::get(builder.getContext(), tree));
  return builder.create(constant)->getResult(0);
}

LogicalResult materialize(LoweringPreparation &prepared, mlir::ModuleOp staged,
                          IRMapping &mapping) {
  Builder attributes(staged.getContext());
  // The schema is final-only, including on this detached candidate. No source
  // envelope or input object is changed by admission or late verification.
  staged->setAttr("ac.stage", attributes.getStringAttr("final"));
  using CandidateMap = llvm::DenseMap<RoutingKey, Value>;
  llvm::DenseMap<Operation *, CandidateMap> nextValues;
  llvm::DenseMap<Operation *, SmallVector<std::pair<Value, Value>>> incoming;
  MaterializationRoutes routes(prepared, mapping);
  SmallVector<StateStorage> storage;

  // Definitions receive the union of demands from every caller occurrence.
  // Source types/names/domain ordinals are preserved as an exact prefix.
  for (auto &definition : prepared.definitions) {
    if (definition.upward.empty() && definition.downward.empty())
      continue;
    auto module =
        cast<ac::ModuleOp>(mapping.lookup(definition.module.getOperation()));
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
         parent.module.getBody().front().getOps<ac::InstanceOp>()) {
      auto child = dyn_cast_or_null<ac::ModuleOp>(
          prepared.hardware.resolveCallee(original));
      if (!child)
        continue;
      auto &definition = prepared.routing(child);
      if (definition.upward.empty() && definition.downward.empty())
        continue;
      auto old = cast<ac::InstanceOp>(mapping.lookup(original.getOperation()));
      auto bindings = prepared.hardware.bindInstance(original);
      if (failed(bindings))
        return failure();
      SmallVector<Type> resultTypes(old.getResultTypes());
      for (const auto &route : definition.upward) {
        auto type =
            prepared.hardware.resolveType(route.type, *bindings, original);
        if (failed(type))
          return failure();
        resultTypes.push_back(*type);
      }
      SmallVector<Value> operands(old.getOperands());
      OpBuilder builder(old);
      for (const auto &route : definition.downward) {
        auto dType =
            prepared.hardware.resolveType(route.type, *bindings, original);
        auto eType = prepared.hardware.resolveType(route.enableType, *bindings,
                                                   original);
        if (failed(dType) || failed(eType))
          return failure();
        operands.push_back(bitsLiteral(*dType, original.getOccurrence(),
                                       original.getLoc(), false, builder));
        operands.push_back(bitsLiteral(*eType, original.getOccurrence(),
                                       original.getLoc(), false, builder));
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
      mapping.map(original.getOperation(), allocation.getOperation());
      old.erase();
    }
  }

  // State remains at its original lexical declaration, one DFFE per owner.
  for (auto original : prepared.states) {
    auto state =
        cast<ac::SourceStateOp>(mapping.lookup(original.getOperation()));
    OperationState leaf(state.getLoc(), ac::InstanceOp::getOperationName());
    leaf.addTypes(original.getQ().getType());
    leaf.addAttribute("instance_name", original.getInstanceNameAttr());
    leaf.addAttribute("occurrence", original.getOccurrence());
    leaf.addAttribute("callee",
                      FlatSymbolRefAttr::get(staged.getContext(),
                                             prepared.dffe.getSymName()));
    leaf.addAttribute("parameters", attributes.getArrayAttr({}));
    leaf.addAttribute(
        "type_arguments",
        attributes.getArrayAttr({TypeAttr::get(original.getQ().getType())}));
    auto instance = cast<ac::InstanceOp>(Operation::create(leaf));
    state->getBlock()->getOperations().insert(state->getIterator(), instance);
    state.getQ().replaceAllUsesWith(instance.getResult(0));
    mapping.map(original.getQ(), instance.getResult(0));
    storage.push_back({original, instance});
    state.erase();
  }

  struct Replacement {
    ac::SourceTransactionOp original;
    ac::RuleOp rule;
  };
  SmallVector<Replacement> replacements;
  for (const auto &transaction : prepared.transactions) {
    RuleMaterializer builder(transaction, mapping, routes);
    auto rule = builder.run();
    if (failed(rule))
      return failure();
    auto original = transaction.plan->transaction;
    replacements.push_back({original, *rule});
    unsigned ordinary = original.getNumResults();
    for (auto [ordinal, binding] : llvm::enumerate(transaction.outputs)) {
      const auto &owner = transaction.plan->owners[binding.owner];
      auto path = anchorPath(*transaction.plan, owner.anchor);
      auto declaration = owner.declaration;
      nextValues[transaction.plan->lexicalModule].try_emplace(
          prepared.key(path, declaration.getQ()),
          rule->getResult(ordinary + ordinal));
    }
  }
  // All Rules exist before ordinary transaction results disappear. Captures
  // from another Rule retain actual graph dependencies and saved SSA values.
  for (auto replacement : replacements) {
    auto original = replacement.original;
    auto transaction =
        cast<ac::SourceTransactionOp>(mapping.lookup(original.getOperation()));
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
    auto key = prepared.key(path, state);
    Value local = nextValues[module].lookup(key);
    if (local)
      return {local, bitsLiteral(eType, occurrence, location, true, builder)};
    auto &definition = prepared.routing(module);
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
         parent.module.getBody().front().getOps<ac::InstanceOp>()) {
      auto child = dyn_cast_or_null<ac::ModuleOp>(
          prepared.hardware.resolveCallee(original));
      if (!child)
        continue;
      auto &definition = prepared.routing(child);
      if (definition.downward.empty())
        continue;
      auto allocation =
          cast<ac::InstanceOp>(mapping.lookup(original.getOperation()));
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
          cast<ac::ModuleOp>(mapping.lookup(parent.module.getOperation()));
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
  for (auto replacement : replacements)
    cast<ac::SourceTransactionOp>(
        mapping.lookup(replacement.original.getOperation()))
        .erase();
  for (auto module : staged.getOps<ac::ModuleOp>())
    for (auto method : llvm::make_early_inc_range(
             module.getBody().front().getOps<ac::SourceMethodOp>()))
      method.erase();
  if (failed(mlir::verify(staged)) ||
      failed(ac::detail::verifyNoUnloweredTransactions(staged)) ||
      failed(ac::verifyHardwarePackage(staged)))
    return failure();
  return success();
}

class LowerTransactionsPass
    : public PassWrapper<LowerTransactionsPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerTransactionsPass)
  StringRef getArgument() const final { return "ac-lower-transactions"; }
  StringRef getDescription() const final {
    return "Lower total transactions through ordinary instance ports and "
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
    MaterializationBudget budget;
    LoweringPreparation prepared(input, analysis, budget);
    if (failed(prepared.run())) {
      signalPassFailure();
      return;
    }
    IRMapping mapping;
    OwningOpRef<mlir::ModuleOp> staged(
        cast<mlir::ModuleOp>(input->clone(mapping)));
    if (failed(materialize(prepared, *staged, mapping))) {
      signalPassFailure();
      return;
    }
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

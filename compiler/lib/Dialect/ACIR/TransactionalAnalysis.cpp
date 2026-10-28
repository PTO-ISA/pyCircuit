#include "pycircuit/Dialect/ACIR/TransactionalAnalysis.h"
#include "ACIRSourceContracts.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/ADT/SmallPtrSet.h"
#include <limits>
#include <map>

using namespace mlir;
namespace acir::ac {
namespace {
struct Binding {
  enum Kind { Payload, Resource, Instance } kind;
  unsigned id;
};
using Environment = llvm::DenseMap<Value, Binding>;
using Predicate = std::pair<std::string, bool>;
Operation *named(Operation *owner, StringRef name) {
  for (Operation &op : owner->getRegion(0).front())
    if (auto symbol = op.getAttrOfType<StringAttr>("sym_name");
        symbol && symbol.getValue() == name)
      return &op;
  return nullptr;
}
} // namespace

FailureOr<unsigned> TransactionalAnalysis::getPayloadWidth(Type type) const {
  if (!isa<BitsType, EnumType, StructType>(type))
    return failure();
  HardwareBindings bindings;
  bindings.owner = getTop();
  HardwareAnalysis hardware(package_);
  auto leaves = hardware.getLeaves(type, bindings, getTop());
  if (failed(leaves) || llvm::any_of(*leaves, [](const PackedLeaf &leaf) {
        return !isa<BitsType, EnumType>(leaf.type);
      }))
    return failure();
  auto width = hardware.getPackedWidth(type, bindings, getTop());
  if (failed(width) || !*width || *width > std::numeric_limits<unsigned>::max())
    return failure();
  return unsigned(*width);
}

class TransactionalPlanBuilder {
public:
  TransactionalPlanBuilder(TransactionalAnalysis &plan,
                           TransactionalDebit debit)
      : p(plan), debit(std::move(debit)), hardware(plan.package_) {}
  LogicalResult run(StringRef top) {
    // Charge source traversal before native verification and allocation. The
    // caller owns the same budget used for provenance parsing and graph proof.
    auto scan = p.package_.walk([&](Operation *op) {
      return failed(charge(16 + op->getNumOperands() + op->getNumResults(), op))
                 ? WalkResult::interrupt()
                 : WalkResult::advance();
    });
    if (scan.wasInterrupted() || failed(verify(p.package_)) ||
        failed(verifyTransactionalPackage(p.package_)))
      return failure();
    auto stage = p.package_->getAttrOfType<StringAttr>("ac.stage");
    if (!stage || stage.getValue() != "linked")
      return p.package_.emitError(
          "transactional analysis requires linked ACIR");
    auto module = dyn_cast_or_null<ModuleOp>(named(p.package_, top));
    if (!module)
      return p.package_.emitError(
          "transactional top must name a defined Module");
    auto systems = p.package_.getOps<SystemOp>();
    if (!systems.empty()) {
      if (!llvm::hasSingleElement(systems))
        return p.package_.emitError(
            "transactional analysis requires one System entry");
      auto system = *systems.begin();
      auto entry = system.getEntry();
      if (entry.getAs<FlatSymbolRefAttr>("callee").getValue() != top ||
          !entry.getAs<ArrayAttr>("parameters").empty() ||
          !entry.getAs<ArrayAttr>("type_arguments").empty())
        return p.package_.emitError(
            "transactional System entry must match the selected closed root");
    }
    if (!module.getBody().front().getArguments().empty() ||
        !module.getParameters().empty() || !module.getTypeParameters().empty())
      return module.emitError(
          "transactional execution requires a closed concrete root Module");
    bindings.owner = module;
    if (failed(declareInstance(module, {}, TransactionalNoID)) ||
        failed(prepareInstance(0, {})))
      return failure();
    for (unsigned id = 0; id < p.instances_.size(); ++id)
      if (failed(registerRules(id)))
        return failure();
    auto yield = cast<YieldOp>(module.getBody().front().getTerminator());
    for (Value value : yield.getValues()) {
      auto item = lookup(value, environments.front(), yield);
      if (failed(item) || item->kind != Binding::Resource)
        return yield.emitError(
            "transactional outputs must export allocated resource identities");
      if (p.resources_[item->id].isQueue())
        return yield.emitError(
            "root Queue exports require an availability-bearing external "
            "interface; consume the Queue inside a System");
      p.exports_.push_back(item->id);
    }
    if (failed(availability()) || failed(conflicts()))
      return failure();
    return knownControls();
  }

private:
  TransactionalAnalysis &p;
  TransactionalDebit debit;
  HardwareAnalysis hardware;
  HardwareBindings bindings;
  std::vector<Environment> environments;
  unsigned instance = 0;
  SmallVector<TransactionalInvocationPlan> invocation;
  unsigned nextStep = 0;
  llvm::DenseMap<unsigned, TransactionalStep *> producers;
  llvm::DenseMap<unsigned, bool> closedCache;
  unsigned nextClosedValue = 0;
  llvm::DenseMap<unsigned, Predicate> predicates;
  unsigned predicateDepth = 0;
  llvm::SmallPtrSet<Operation *, 16> methodStack;

  LogicalResult charge(uint64_t units, Operation *site) {
    return debit(units, site);
  }
  LogicalResult declareInstance(ModuleOp module, InstanceOp declaration,
                                unsigned parent) {
    if (!module.getParameters().empty() || !module.getTypeParameters().empty())
      return module.emitError("transactional hierarchy requires concrete Modules");
    if (failed(charge(64, module)))
      return failure();
    unsigned id = p.instances_.size();
    p.instances_.push_back({module, declaration, parent, {}, {}, {}, {}});
    environments.emplace_back();
    if (parent != TransactionalNoID)
      p.instances_[parent].children.push_back(id);
    for (auto allocation : module.getBody().front().getOps<ResourceAllocOp>()) {
      Type type = allocation.getResource().getType();
      Type payload;
      if (auto state = dyn_cast<StateType>(type))
        payload = state.getPayload();
      else if (auto queue = dyn_cast<QueueType>(type))
        payload = queue.getPayload();
      if (!payload || failed(p.getPayloadWidth(payload)))
        return allocation.emitError(
            "transactional execution requires finite Bits, Enum or Struct "
            "State or Queue");
      if (failed(charge(32, allocation)))
        return failure();
      unsigned resource = p.resources_.size();
      p.resources_.push_back({allocation, id, type});
      if (isa<QueueType>(type))
        p.resources_.back().depth = allocation.getDepthAttr().getInt();
      p.instances_[id].resources.push_back(resource);
      environments[id][allocation.getResource()] = {Binding::Resource, resource};
    }
    for (auto child : module.getBody().front().getOps<InstanceOp>()) {
      auto target = dyn_cast_or_null<ModuleOp>(named(p.package_, child.getCallee()));
      if (!target)
        return child.emitError("instance needs a linked Module definition");
      if (failed(declareInstance(target, child, id)))
        return failure();
    }
    return success();
  }
  LogicalResult prepareInstance(unsigned id, ArrayRef<Binding> args) {
    unsigned caller = instance;
    instance = id;
    auto restore = llvm::scope_exit([&] { instance = caller; });
    auto module = p.instances_[id].definition;
    auto &environment = environments[id];
    auto &body = module.getBody().front();
    if (body.getNumArguments() != args.size())
      return module.emitError("Module instance binding arity differs");
    for (auto [argument, item] : llvm::zip(body.getArguments(), args))
      environment[argument] = item;
    SmallVector<Operation *> pending;
    module.getBody().walk<WalkOrder::PreOrder>([&](Operation *op) {
      if (isa<TransactionRuleOp, TransactionMethodOp>(op))
        return WalkResult::skip();
      if (!isa<ResourceAllocOp, TransactionRuleCallOp, YieldOp,
               SourcePredicateOp, scf::IfOp, scf::YieldOp>(op))
        pending.push_back(op);
      return WalkResult::advance();
    });
    while (!pending.empty()) {
      bool progress = false;
      for (auto &op : pending) {
        if (!op)
          continue;
        if (failed(charge(4 + op->getNumOperands(), op)))
          return failure();
        if (!llvm::all_of(op->getOperands(), [&](Value value) {
              return environment.contains(value);
            }))
          continue;
        if (failed(operation(op, environment, p.instances_[id].comb,
                             TransactionalNoID, true)))
          return failure();
        op = nullptr;
        progress = true;
      }
      if (!progress)
        return module.emitError("transactional Module computation has "
                                "unresolved or cyclic dependencies");
      llvm::erase(pending, nullptr);
    }
    for (unsigned resourceID : p.instances_[id].resources) {
      auto &resource = p.resources_[resourceID];
      if (resource.isQueue())
        continue;
      if (resource.allocation.getInitial().empty())
        return resource.allocation.emitError(
            "transactional execution currently requires an explicit State "
            "initial value");
      auto initial = payload(resource.allocation.getInitial()[0], environment,
                             resource.allocation);
      if (failed(initial) || !closed(*initial))
        return resource.allocation.emitError(
            "State initial value must be a closed pure expression");
      resource.initial = *initial;
    }
    return success();
  }
  LogicalResult registerRules(unsigned id) {
    instance = id;
    auto module = p.instances_[id].definition;
    auto &environment = environments[id];
    SmallVector<TransactionRuleCallOp> registrations;
    module.walk([&](TransactionRuleCallOp call) { registrations.push_back(call); });
    for (auto call : registrations) {
      auto *target = named(module, call.getCallee());
      if (!target)
        target = named(p.package_, call.getCallee());
      auto definition = dyn_cast_or_null<TransactionRuleOp>(target);
      if (!definition)
        return call.emitError("Rule registration has no definition");
      if (failed(charge(32, call)))
        return failure();
      SmallVector<Binding> args;
      for (Value value : call.getInterfaces()) {
        auto item = lookup(value, environment, call);
        if (failed(item) || item->kind != Binding::Resource)
          return call.emitError("Rule interfaces must bind allocated resources");
        args.push_back(*item);
      }
      for (Value value : definition.getCaptures()) {
        auto item = lookup(value, environment, call);
        if (failed(item))
          return failure();
        args.push_back(*item);
      }
      unsigned rule = p.rules_.size();
      p.rules_.push_back({call, definition, id, {}, {}, {}, {}});
      SmallVector<TransactionalPathTerm> path;
      for (Operation *at = call; at->getParentOp() != module;
           at = at->getParentOp()) {
        auto branch = dyn_cast<scf::IfOp>(at->getParentOp());
        if (!branch)
          return call.emitError("registration requires Module SCF scope");
        auto bridge = branch.getCondition().getDefiningOp<SourcePredicateOp>();
        if (!bridge)
          return branch.emitError("Module condition requires source predicate");
        auto condition = payload(bridge->getOperand(0), environment, branch);
        if (failed(condition))
          return failure();
        path.push_back({*condition, at->getParentRegion() == &branch.getThenRegion()});
      }
      p.rules_[rule].path.assign(path.rbegin(), path.rend());
      p.instances_[id].rules.push_back(rule);
      invocation.clear();
      if (id != 0)
        invocation.push_back({call, id});
      if (failed(region(definition.getGuard(), args, p.rules_[rule].guard, rule,
                        true)) ||
          failed(region(definition.getBody(), args, p.rules_[rule].body, rule,
                        false)))
        return failure();
    }
    return success();
  }
  FailureOr<Binding> lookup(Value value, Environment &env, Operation *site) {
    auto found = env.find(value);
    if (found != env.end())
      return found->second;
    site->emitError(
        "value is outside the closed transactional evaluation scope");
    return failure();
  }
  FailureOr<unsigned> payload(Value value, Environment &env, Operation *site) {
    auto item = lookup(value, env, site);
    if (failed(item))
      return failure();
    if (item->kind != Binding::Payload) {
      site->emitError("expected payload SSA value");
      return failure();
    }
    return item->id;
  }
  bool closed(unsigned id) {
    if (auto found = closedCache.find(id); found != closedCache.end())
      return found->second;
    // Sequence construction is topological; compute without recursive
    // expansion.
    for (; nextClosedValue <= id; ++nextClosedValue) {
      unsigned current = nextClosedValue;
      auto *step = producers.lookup(current);
      bool value = step && step->kind == TransactionalStepKind::Pure;
      if (step)
        for (unsigned operand : step->operands)
          value &= closedCache.lookup(operand);
      closedCache[current] = value;
    }
    return closedCache.lookup(id);
  }
  LogicalResult region(Region &region, ArrayRef<Binding> args,
                       TransactionalSequence &sequence, unsigned rule,
                       bool pure) {
    if (region.empty())
      return success();
    if (!region.hasOneBlock() ||
        region.front().getNumArguments() != args.size())
      return region.getParentOp()->emitError(
          "callable region binding arity differs");
    Environment env;
    for (auto [arg, item] : llvm::zip(region.front().getArguments(), args))
      env[arg] = item;
    for (Operation &op : region.front()) {
      if (auto yield = dyn_cast<YieldOp>(op)) {
        for (Value value : yield.getValues()) {
          auto id = payload(value, env, yield);
          if (failed(id))
            return failure();
          sequence.yields.push_back(*id);
        }
      } else if (failed(operation(&op, env, sequence, rule, pure)))
        return failure();
    }
    return success();
  }
  LogicalResult operation(Operation *op, Environment &env,
                          TransactionalSequence &sequence, unsigned rule,
                          bool pure) {
    if (failed(charge(32 + op->getNumOperands() * 4 + op->getNumResults() * 8,
                      op)))
      return failure();
    if (auto reference = dyn_cast<InstanceRefOp>(op)) {
      unsigned target = instance;
      for (Attribute segment : reference.getInstancePath()) {
        bool found = false;
        for (unsigned child : p.instances_[target].children)
          if (p.instances_[child].declaration.getInstanceName() ==
              cast<StringAttr>(segment).getValue()) {
            target = child;
            found = true;
            break;
          }
        if (!found)
          return reference.emitError("instance reference has no actual binding");
      }
      env[reference.getReference()] = {Binding::Instance, target};
      return success();
    }
    if (auto child = dyn_cast<InstanceOp>(op)) {
      unsigned target = TransactionalNoID;
      for (unsigned id : p.instances_[instance].children)
        if (p.instances_[id].declaration == child)
          target = id;
      if (target == TransactionalNoID)
        return child.emitError("instance has no declared occurrence");
      SmallVector<Binding> args;
      for (Value value : child.getOperands()) {
        auto item = lookup(value, env, child);
        if (failed(item))
          return failure();
        args.push_back(*item);
      }
      if (failed(prepareInstance(target, args)))
        return failure();
      auto module = p.instances_[target].definition;
      auto yield = cast<YieldOp>(module.getBody().front().getTerminator());
      for (auto [result, value] : llvm::zip(child.getResults(), yield.getValues())) {
        auto item = lookup(value, environments[target], child);
        if (failed(item))
          return failure();
        env[result] = *item;
      }
      return success();
    }
    if (auto call = dyn_cast<TransactionMethodCallOp>(op)) {
      auto receiver = lookup(call.getReceiver(), env, op);
      if (failed(receiver))
        return failure();
      if (receiver->kind == Binding::Instance)
        return method(call, *receiver, env, sequence, rule, pure);
    }
    auto owned = std::make_unique<TransactionalStep>();
    auto &step = *owned;
    step.operation = op;
    step.rule = rule;
    step.instance = instance;
    step.invocation = invocation;
    step.id = nextStep++;
    if (auto call = dyn_cast<TransactionMethodCallOp>(op)) {
      auto receiver = lookup(call.getReceiver(), env, op);
      if (failed(receiver) || receiver->kind != Binding::Resource)
        return op->emitError(
            "transactional resource call requires an allocated resource");
      step.resource = receiver->id;
      auto method = call.getCalleeAttr().getRootReference().getValue();
      if (p.resources_[step.resource].isQueue()) {
        if (method == "head" || method == "peek")
          step.kind = TransactionalStepKind::QueueHead;
        else if (method == "pop" && !pure)
          step.kind = TransactionalStepKind::QueuePop;
        else if (method == "push" && !pure)
          step.kind = TransactionalStepKind::QueuePush;
        else
          return op->emitError("Queue requires head/peek observation or "
                               "Rule-body pop/push");
      } else if (method == "read")
        step.kind = TransactionalStepKind::StateRead;
      else if (method == "write" && !pure)
        step.kind = TransactionalStepKind::StateWrite;
      else
        return op->emitError("transactional execution currently admits State "
                             "read and body write only");
      for (Value value : call.getArguments()) {
        auto id = payload(value, env, op);
        if (failed(id))
          return failure();
        step.operands.push_back(*id);
      }
    } else if (auto observe = dyn_cast<SourceObserveOp>(op)) {
      if (pure)
        return op->emitError("observations require a Rule body");
      step.kind = TransactionalStepKind::Observe;
      auto path = payload(observe.getPath(), env, op);
      if (failed(path))
        return failure();
      step.activation = *path;
      for (Value value : observe.getValues()) {
        auto id = payload(value, env, op);
        if (failed(id))
          return failure();
        step.operands.push_back(*id);
      }
    } else if (auto expect = dyn_cast<SourceExpectOp>(op)) {
      if (pure || expect.getKind() != "assert" || !expect.getSampling().empty())
        return op->emitError(
            "transactional checks require unsampled Rule-body assert");
      step.kind = TransactionalStepKind::Assert;
      auto condition = payload(expect.getCondition(), env, op);
      auto path = payload(expect.getPath(), env, op);
      if (failed(condition) || failed(path))
        return failure();
      step.activation = *path;
      step.operands.push_back(*condition);
    } else if (detail::isCombinationalHardwareOperation(op) &&
               !isa<ValueMergeOp, ValueRequireOp>(op)) {
      if (op->getNumRegions() ||
          failed(hardware.verifyResolvedOperation(
              op, HardwareBindings{p.instances_[instance].definition})))
        return failure();
      for (Value value : op->getOperands()) {
        auto id = payload(value, env, op);
        if (failed(id))
          return failure();
        step.operands.push_back(*id);
      }
    } else
      return op->emitError(
          "operation is outside closed State/Rule execution support");
    // Verification-only observers are not a new functional expression route.
    if (op->hasAttr("ac.verification_observer"))
      return op->emitError("verification-observer expressions are outside this "
                           "transactional execution slice");
    for (Value result : op->getResults()) {
      if (failed(p.getPayloadWidth(result.getType())))
        return op->emitError(
            "transactional execution requires finite Bits, Enum or Struct "
            "payloads");
      unsigned id = p.values_.size();
      p.values_.push_back({result.getType(), instance, rule});
      step.results.push_back(id);
      producers[id] = &step;
      env[result] = {Binding::Payload, id};
    }
    sequence.steps.push_back(std::move(owned));
    return success();
  }

  LogicalResult method(TransactionMethodCallOp call, Binding receiver,
                       Environment &env, TransactionalSequence &sequence,
                       unsigned rule, bool pure) {
    if (rule == TransactionalNoID)
      return call.emitError("Module Method calls require a Rule transaction");
    auto module = p.instances_[receiver.id].definition;
    auto definition = dyn_cast_or_null<TransactionMethodOp>(
        named(module, call.getCalleeAttr().getRootReference().getValue()));
    if (!definition)
      return call.emitError("Method has no definition on its actual instance");
    if (methodStack.size() >= 64 || !methodStack.insert(definition).second)
      return call.emitError("recursive or over-depth Method closure");
    auto cleanup = llvm::scope_exit([&] { methodStack.erase(definition); });
    SmallVector<Binding> args;
    for (Value value : call.getArguments()) {
      auto id = payload(value, env, call);
      if (failed(id))
        return failure();
      args.push_back({Binding::Payload, *id});
    }
    // Captures belong to the callee Module, never a prior invocation's locals.
    for (Value value : definition.getCaptures()) {
      auto item = lookup(value, environments[receiver.id], call);
      if (failed(item))
        return failure();
      args.push_back(*item);
    }
    unsigned caller = instance;
    instance = receiver.id;
    invocation.push_back({call, receiver.id});
    auto restore = llvm::scope_exit([&] {
      instance = caller;
      invocation.pop_back();
    });
    TransactionalSequence guard, body;
    if (failed(region(definition.getGuard(), args, guard, rule, true)) ||
        failed(region(definition.getBody(), args, body, rule, pure)))
      return failure();
    llvm::append_range(p.rules_[rule].requirements, guard.yields);
    for (auto &step : guard.steps)
      sequence.steps.push_back(std::move(step));
    for (auto &step : body.steps)
      sequence.steps.push_back(std::move(step));
    for (auto [result, id] : llvm::zip(call.getResults(), body.yields))
      env[result] = {Binding::Payload, id};
    return success();
  }

  LogicalResult availability() {
    // Follow actual SSA, including Module computations captured by a Rule.
    // A cached head is a nonconsuming observation, but every transaction that
    // uses it must still wait for the old Queue to be readable.
    std::vector<SmallVector<unsigned>> dependencies(p.values_.size());
    auto append = [](SmallVector<unsigned> &to, ArrayRef<unsigned> from) {
      llvm::DenseSet<unsigned> seen(to.begin(), to.end());
      for (unsigned resource : from)
        if (seen.insert(resource).second)
          to.push_back(resource);
    };
    auto sequence = [&](const TransactionalSequence &seq,
                        TransactionalRulePlan *rule) -> LogicalResult {
      for (const auto &step : seq.steps) {
        if (failed(charge(8 + p.resources_.size() *
                                  (step->operands.size() + step->results.size() + 1),
                          step->operation)))
          return failure();
        SmallVector<unsigned> reads;
        for (unsigned operand : step->operands)
          append(reads, dependencies[operand]);
        if (step->activation != TransactionalNoID)
          append(reads, dependencies[step->activation]);
        if (step->kind == TransactionalStepKind::QueueHead ||
            step->kind == TransactionalStepKind::QueuePop)
          append(reads, {step->resource});
        for (unsigned result : step->results)
          dependencies[result] = reads;
        if (rule) {
          append(rule->queueReads, reads);
          if (step->kind == TransactionalStepKind::QueuePush)
            append(rule->queueWrites, {step->resource});
        }
      }
      return success();
    };
    for (const auto &owner : p.instances_)
      if (failed(sequence(owner.comb, nullptr)))
        return failure();
    for (auto &rule : p.rules_) {
      for (auto term : rule.path)
        append(rule.queueReads, dependencies[term.value]);
      if (failed(sequence(rule.guard, &rule)) ||
          failed(sequence(rule.body, &rule)))
        return failure();
      for (unsigned requirement : rule.requirements)
        append(rule.queueReads, dependencies[requirement]);
    }
    return success();
  }

  LogicalResult knownControls() {
    // Greatest fixed point: an initially known State remains known only if
    // every possible proposal is known. Queue payloads are known only under
    // the nonempty admission requirement and only if all pushes are known;
    // an empty head itself is never assumed known. Reads refer to old storage.
    // Track each packed leaf separately: an unknown data field must not poison
    // its known sibling control. Bits (including wide Bits) and Enum are leaves.
    using KnownLeaves = std::vector<bool>;
    llvm::DenseMap<Type, SmallVector<PackedLeaf>> layouts;
    auto layout = [&](Type type) -> LogicalResult {
      if (layouts.contains(type))
        return success();
      auto leaves = hardware.getLeaves(type, bindings, p.getTop());
      if (failed(leaves) || failed(charge(leaves->size(), p.getTop())))
        return failure();
      layouts[type] = std::move(*leaves);
      return success();
    };
    std::vector<KnownLeaves> resources, values;
    for (auto &resource : p.resources_) {
      if (failed(layout(resource.getPayload())))
        return failure();
      auto count = layouts[resource.getPayload()].size();
      if (failed(charge(count, resource.allocation)))
        return failure();
      resources.emplace_back(count, true);
    }
    for (auto &value : p.values_) {
      if (failed(layout(value.type)))
        return failure();
      auto count = layouts[value.type].size();
      if (failed(charge(count, p.getTop())))
        return failure();
      values.emplace_back(count, false);
    }
    auto allKnown = [](const KnownLeaves &leaves) {
      return llvm::all_of(leaves, [](bool known) { return known; });
    };
    auto evaluate =
        [&](const TransactionalSequence &sequence) -> LogicalResult {
      for (const auto &owned : sequence.steps) {
        const auto &step = *owned;
        if (failed(charge(8 + step.operands.size() + step.results.size(),
                          step.operation)))
          return failure();
        KnownLeaves fields;
        bool known = false;
        if (step.kind == TransactionalStepKind::StateRead ||
            step.kind == TransactionalStepKind::QueueHead ||
            step.kind == TransactionalStepKind::QueuePop)
          fields = resources[step.resource];
        if (step.kind == TransactionalStepKind::Pure) {
          known = true;
          for (unsigned operand : step.operands)
            known = known && allKnown(values[operand]);
          if (isa<StructCreateOp>(step.operation)) {
            // The common layout visits leaves in constructor field order.
            for (unsigned operand : step.operands)
              fields.insert(fields.end(), values[operand].begin(),
                            values[operand].end());
          } else if (auto get = dyn_cast<StructGetOp>(step.operation)) {
            unsigned operand = step.operands.front();
            for (auto [index, leaf] :
                 llvm::enumerate(layouts[p.values_[operand].type]))
              if (leaf.path.front() == get.getFieldAttr())
                fields.push_back(values[operand][index]);
          }
          if (auto literal = dyn_cast<BitsConstant4Op>(step.operation))
            known =
                literal.getPattern().find_first_of("xzXZ") == StringRef::npos;
          if (auto binary = dyn_cast<BitsBinaryOp>(step.operation))
            if (binary.getOpcode() == "udiv" || binary.getOpcode() == "urem")
              known = false;
          if (auto compare = dyn_cast<BitsCompareOp>(step.operation))
            if (compare.getPredicate() == "case_eq")
              known = true;
        }
        for (unsigned result : step.results) {
          if (failed(charge(values[result].size(), step.operation)))
            return failure();
          if (!fields.empty())
            values[result] = fields;
          else
            std::fill(values[result].begin(), values[result].end(), known);
        }
      }
      return success();
    };
    bool changed;
    do {
      changed = false;
      for (auto &owner : p.instances_)
        if (failed(evaluate(owner.comb)))
          return failure();
      for (auto &rule : p.rules_)
        if (failed(evaluate(rule.guard)) || failed(evaluate(rule.body)))
          return failure();
      auto constrain = [&](unsigned id, const KnownLeaves &known) {
        for (unsigned i = 0; i < known.size(); ++i)
          if (resources[id][i] && !known[i]) {
            resources[id][i] = false;
            changed = true;
          }
      };
      for (auto [id, resource] : llvm::enumerate(p.resources_))
        if (!resource.isQueue())
          constrain(id, values[resource.initial]);
      for (auto &rule : p.rules_)
        for (auto &step : rule.body.steps)
          if (step->kind == TransactionalStepKind::StateWrite ||
              step->kind == TransactionalStepKind::QueuePush)
            constrain(step->resource, values[step->operands.front()]);
    } while (changed);
    for (auto &rule : p.rules_) {
      for (auto term : rule.path)
        if (!allKnown(values[term.value]))
          return rule.registration.emitError(
              "transactional backend admission cannot resolve X/Z Module control; "
              "four-state payloads remain supported");
      if (!rule.guard.yields.empty() &&
          !allKnown(values[rule.guard.yields.front()]))
        return rule.registration.emitError(
            "transactional backend admission cannot resolve X/Z Rule control; "
            "four-state payloads remain supported");
      for (unsigned requirement : rule.requirements)
        if (!allKnown(values[requirement]))
          return rule.registration.emitError(
              "transactional backend admission cannot resolve X/Z Method "
              "control; four-state payloads remain supported");
    }
    return success();
  }

  // Prototype necessary-fact proof, restricted to State reads and complement
  // predicates. The keys are analysis-local and never serialized as proof IR.
  FailureOr<Predicate> predicate(unsigned id) {
    if (auto found = predicates.find(id); found != predicates.end())
      return found->second;
    auto *step = producers.lookup(id);
    if (failed(charge(8, step ? step->operation : p.package_)))
      return failure();
    Predicate result{"v" + std::to_string(id), true};
    if (++predicateDepth > 128) {
      --predicateDepth;
      return result;
    }
    auto save = [&](Predicate value) -> FailureOr<Predicate> {
      --predicateDepth;
      if (value.first.size() > 4096)
        value = result;
      if (failed(charge(value.first.size(), step->operation)))
        return failure();
      predicates[id] = value;
      return value;
    };
    if (step->kind == TransactionalStepKind::StateRead)
      return save({"old_resource_" + std::to_string(step->resource), true});
    Operation *op = step->operation;
    if (auto constant = dyn_cast<BitsConstantOp>(op)) {
      auto value = hardware.evaluateStatic(constant.getValue(), bindings, op);
      if (succeeded(value))
        if (auto integer = dyn_cast<MathIntAttr>(*value)) {
          auto width = p.getPayloadWidth(p.values_[id].type);
          if (succeeded(width)) {
            StringRef spelling = integer.getCanonicalValue();
            bool negative = spelling.consume_front("-");
            llvm::APInt number(*width, spelling, 10);
            if (negative)
              number = -number;
            llvm::SmallString<32> text;
            number.toStringUnsigned(text, 10);
            return save({"constant_" + text.str().str(), true});
          }
        }
    }
    if (auto constant = dyn_cast<BitsConstant4Op>(op)) {
      StringRef pattern = constant.getPattern();
      if (llvm::all_of(pattern,
                       [](char bit) { return bit == '0' || bit == '1'; })) {
        llvm::APInt number(pattern.size(), pattern, 2);
        llvm::SmallString<32> text;
        number.toStringUnsigned(text, 10);
        return save({"constant_" + text.str().str(), true});
      }
    }
    if (auto unary = dyn_cast<BitsUnaryOp>(op)) {
      auto width = p.getPayloadWidth(unary.getResult().getType());
      if (unary.getOpcode() == "not" && succeeded(width) && *width == 1) {
        auto operand = predicate(step->operands[0]);
        if (failed(operand)) {
          --predicateDepth;
          return failure();
        }
        return save({operand->first, !operand->second});
      }
    }
    if (auto compare = dyn_cast<BitsCompareOp>(op)) {
      auto lhs = predicate(step->operands[0]),
           rhs = predicate(step->operands[1]);
      if (failed(lhs) || failed(rhs)) {
        --predicateDepth;
        return failure();
      }
      std::string kind = compare.getPredicate().str();
      bool positive = true;
      if (kind == "ne") {
        kind = "eq";
        positive = false;
      } else if (kind == "uge") {
        kind = "ult";
        positive = false;
      } else if (kind == "ugt") {
        kind = "ule";
        positive = false;
      } else if (kind == "sge") {
        kind = "slt";
        positive = false;
      } else if (kind == "sgt") {
        kind = "sle";
        positive = false;
      }
      if (kind == "eq" && *rhs < *lhs)
        std::swap(lhs, rhs);
      auto width = p.getPayloadWidth(p.values_[step->operands[0]].type);
      if (failed(width)) {
        --predicateDepth;
        return failure();
      }
      return save({kind + std::to_string(*width) + "(" + lhs->first +
                       (lhs->second ? "+" : "-") + "," + rhs->first +
                       (rhs->second ? "+" : "-") + ")",
                   positive});
    }
    return save(result);
  }
  struct Fact {
    Predicate predicate;
    unsigned resource = TransactionalNoID;
    std::string constant;
  };
  LogicalResult facts(unsigned id, bool positive, SmallVectorImpl<Fact> &out,
                      unsigned depth = 0) {
    auto *step = producers.lookup(id);
    if (failed(charge(8, step ? step->operation : p.package_)))
      return failure();
    // Bounded necessary facts only; unsupported expressions stay opaque.
    if (step && depth < 128) {
      if (auto unary = dyn_cast<BitsUnaryOp>(step->operation))
        if (unary.getOpcode() == "not")
          return facts(step->operands[0], !positive, out, depth + 1);
    }
    auto key = predicate(id);
    if (failed(key))
      return failure();
    Fact fact{*key, TransactionalNoID, {}};
    fact.predicate.second = (fact.predicate.second == positive);
    if (step)
      if (auto compare = dyn_cast<BitsCompareOp>(step->operation))
        if ((positive && compare.getPredicate() == "eq") ||
            (!positive && compare.getPredicate() == "ne"))
          for (unsigned side = 0; side < 2; ++side) {
            auto *read = producers.lookup(step->operands[side]);
            auto constant = predicate(step->operands[1 - side]);
            if (failed(constant))
              return failure();
            if (read && read->kind == TransactionalStepKind::StateRead &&
                constant->second &&
                StringRef(constant->first).starts_with("constant_")) {
              fact.resource = read->resource;
              fact.constant = constant->first;
            }
          }
    out.push_back(std::move(fact));
    // Retain the whole condition as well as its necessary constituent facts.
    // Otherwise an OR path and its else path lose their shared identity when
    // only the negative OR is decomposed (likewise a positive AND).
    if (step && depth < 128)
      if (auto binary = dyn_cast<BitsBinaryOp>(step->operation))
        if ((positive && binary.getOpcode() == "and") ||
            (!positive && binary.getOpcode() == "or")) {
          if (failed(facts(step->operands[0], positive, out, depth + 1)))
            return failure();
          return facts(step->operands[1], positive, out, depth + 1);
        }
    return success();
  }
  static bool disjoint(const Fact &a, const Fact &b) {
    return (a.predicate.first == b.predicate.first &&
            a.predicate.second != b.predicate.second) ||
           (a.resource != TransactionalNoID && a.resource == b.resource &&
            a.constant != b.constant);
  }
  LogicalResult conflicts() {
    struct Write {
      TransactionalStep *step;
      SmallVector<Fact> conditions;
    };
    SmallVector<Write> writes;
    for (auto &rule : p.rules_) {
      SmallVector<Fact> conditions;
      for (auto term : rule.path)
        if (failed(facts(term.value, term.positive, conditions)))
          return failure();
      if (!rule.guard.yields.empty() &&
          failed(facts(rule.guard.yields.front(), true, conditions)))
        return failure();
      for (unsigned requirement : rule.requirements)
        if (failed(facts(requirement, true, conditions)))
          return failure();
      bool impossible = false;
      for (const auto &a : conditions) {
        impossible |= (a.predicate.first == "constant_0" && a.predicate.second) ||
                      (a.predicate.first == "constant_1" && !a.predicate.second);
        for (const auto &b : conditions) {
          if (failed(charge(4, rule.registration)))
            return failure();
          impossible |= disjoint(a, b);
        }
      }
      if (impossible)
        continue;
      for (auto &step : rule.body.steps)
        if (step->kind == TransactionalStepKind::StateWrite ||
            step->kind == TransactionalStepKind::QueuePush ||
            step->kind == TransactionalStepKind::QueuePop)
          writes.push_back({step.get(), conditions});
    }
    for (unsigned i = 0; i < writes.size(); ++i)
      for (unsigned j = i + 1; j < writes.size(); ++j) {
        auto &a = writes[i];
        auto &b = writes[j];
        if (failed(charge(8 + a.conditions.size() * b.conditions.size(),
                          b.step->operation)))
          return failure();
        bool exclusive = a.step->resource != b.step->resource ||
                         a.step->kind != b.step->kind;
        for (const auto &x : a.conditions)
          for (const auto &y : b.conditions)
            exclusive |= disjoint(x, y);
        if (exclusive)
          continue;
        auto diagnostic = b.step->operation->emitError(
            "transactional resource accesses may overlap; no implicit "
            "arbitration or overwrite");
        diagnostic.attachNote(a.step->operation->getLoc())
            << "other potentially simultaneous access";
        return failure();
      }
    return success();
  }
};

FailureOr<std::unique_ptr<TransactionalAnalysis>>
TransactionalAnalysis::create(mlir::ModuleOp package, StringRef top,
                              TransactionalDebit debit) {
  if (!package || !debit)
    return failure();
  auto plan = std::unique_ptr<TransactionalAnalysis>(
      new TransactionalAnalysis(package));
  if (failed(TransactionalPlanBuilder(*plan, std::move(debit)).run(top)))
    return failure();
  return std::move(plan);
}
} // namespace acir::ac

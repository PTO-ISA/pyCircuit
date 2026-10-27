#include "pycircuit/Dialect/ACIR/TransactionalAnalysis.h"
#include "ACIRSourceContracts.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include <limits>
#include <map>

using namespace mlir;
namespace acir::ac {
namespace {
struct Binding {
  bool resource;
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
  if (!isa<BitsType>(type))
    return failure();
  HardwareBindings bindings;
  bindings.owner = getTop();
  auto width =
      HardwareAnalysis(package_).getPackedWidth(type, bindings, getTop());
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
          "transactional State execution requires one closed concrete Module");
    p.instances_.push_back({module, {}, {}, {}});
    bindings.owner = module;
    for (auto allocation : module.getBody().front().getOps<ResourceAllocOp>()) {
      auto type = dyn_cast<StateType>(allocation.getResource().getType());
      if (!type || failed(p.getPayloadWidth(type.getPayload())))
        return allocation.emitError(
            "transactional execution currently requires finite Bits State");
      if (failed(charge(32, allocation)))
        return failure();
      unsigned id = p.resources_.size();
      p.resources_.push_back({allocation, 0, type});
      p.instances_[0].resources.push_back(id);
      environment[allocation.getResource()] = {true, id};
    }
    SmallVector<Operation *> pending;
    for (Operation &op : module.getBody().front()) {
      if (isa<ResourceAllocOp, TransactionRuleOp, TransactionRuleCallOp,
              TransactionMethodOp, YieldOp>(op))
        continue;
      pending.push_back(&op);
    }
    // Module regions are hardware graphs. Order pure evaluation by SSA
    // dependencies, without imposing source statement order on definitions.
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
        if (failed(operation(op, environment, p.instances_[0].comb,
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
    for (auto &resource : p.resources_) {
      if (resource.allocation.getInitial().empty())
        return resource.allocation.emitError(
            "transactional execution currently requires an explicit State "
            "initial value");
      auto id = payload(resource.allocation.getInitial()[0], environment,
                        resource.allocation);
      if (failed(id) || !closed(*id))
        return resource.allocation.emitError(
            "State initial value must be a closed pure expression");
      resource.initial = *id;
    }
    for (auto call : module.getBody().front().getOps<TransactionRuleCallOp>()) {
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
        if (failed(item) || !item->resource)
          return call.emitError("Rule interfaces must bind allocated State");
        args.push_back(*item);
      }
      for (Value value : definition.getCaptures()) {
        auto item = lookup(value, environment, call);
        if (failed(item))
          return failure();
        args.push_back(*item);
      }
      unsigned rule = p.rules_.size();
      p.rules_.push_back({call, definition, 0, {}, {}});
      p.instances_[0].rules.push_back(rule);
      if (failed(region(definition.getGuard(), args, p.rules_[rule].guard, rule,
                        true)) ||
          failed(region(definition.getBody(), args, p.rules_[rule].body, rule,
                        false)))
        return failure();
    }
    auto yield = cast<YieldOp>(module.getBody().front().getTerminator());
    for (Value value : yield.getValues()) {
      auto item = lookup(value, environment, yield);
      if (failed(item) || !item->resource)
        return yield.emitError(
            "transactional outputs must export allocated State identities");
      p.exports_.push_back(item->id);
    }
    if (failed(conflicts()))
      return failure();
    return knownControls();
  }

private:
  TransactionalAnalysis &p;
  TransactionalDebit debit;
  HardwareAnalysis hardware;
  HardwareBindings bindings;
  Environment environment;
  unsigned nextStep = 0;
  llvm::DenseMap<unsigned, TransactionalStep *> producers;
  llvm::DenseMap<unsigned, bool> closedCache;
  unsigned nextClosedValue = 0;
  llvm::DenseMap<unsigned, Predicate> predicates;
  unsigned predicateDepth = 0;

  LogicalResult charge(uint64_t units, Operation *site) {
    return debit(units, site);
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
    if (item->resource) {
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
          "Rule region binding arity differs");
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
    auto owned = std::make_unique<TransactionalStep>();
    auto &step = *owned;
    step.operation = op;
    step.rule = rule;
    step.id = nextStep++;
    if (auto call = dyn_cast<TransactionMethodCallOp>(op)) {
      auto receiver = lookup(call.getReceiver(), env, op);
      if (failed(receiver) || !receiver->resource)
        return op->emitError(
            "transactional execution currently admits State read/write only");
      step.resource = receiver->id;
      auto method = call.getCalleeAttr().getRootReference().getValue();
      if (method == "read")
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
          failed(hardware.verifyResolvedOperation(op, bindings)))
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
            "transactional execution requires finite Bits payloads");
      unsigned id = p.values_.size();
      p.values_.push_back({result.getType(), 0, rule});
      step.results.push_back(id);
      producers[id] = &step;
      env[result] = {false, id};
    }
    sequence.steps.push_back(std::move(owned));
    return success();
  }

  LogicalResult knownControls() {
    // Greatest fixed point: an initially known State remains known only if
    // every possible proposal is known. Reads always refer to that old Q.
    std::vector<bool> resources(p.resources_.size(), true);
    std::vector<bool> values(p.values_.size(), false);
    auto evaluate =
        [&](const TransactionalSequence &sequence) -> LogicalResult {
      for (const auto &owned : sequence.steps) {
        const auto &step = *owned;
        if (failed(charge(8 + step.operands.size() + step.results.size(),
                          step.operation)))
          return failure();
        bool known = false;
        if (step.kind == TransactionalStepKind::StateRead)
          known = resources[step.resource];
        if (step.kind == TransactionalStepKind::Pure) {
          known = true;
          for (unsigned operand : step.operands)
            known = known && values[operand];
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
        for (unsigned result : step.results)
          values[result] = known;
      }
      return success();
    };
    bool changed;
    do {
      changed = false;
      if (failed(evaluate(p.instances_[0].comb)))
        return failure();
      for (auto &rule : p.rules_)
        if (failed(evaluate(rule.guard)) || failed(evaluate(rule.body)))
          return failure();
      auto constrain = [&](unsigned id, bool known) {
        if (resources[id] && !known) {
          resources[id] = false;
          changed = true;
        }
      };
      for (auto [id, resource] : llvm::enumerate(p.resources_))
        constrain(id, values[resource.initial]);
      for (auto &rule : p.rules_)
        for (auto &step : rule.body.steps)
          if (step->kind == TransactionalStepKind::StateWrite)
            constrain(step->resource, values[step->operands.front()]);
    } while (changed);
    for (auto &rule : p.rules_)
      if (!rule.guard.yields.empty() && !values[rule.guard.yields.front()])
        return rule.registration.emitError(
            "transactional backend admission cannot resolve X/Z Rule control; "
            "four-state payloads remain supported");
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
        if (auto integer = dyn_cast<MathIntAttr>(*value))
          return save({"constant_" + integer.getCanonicalValue().str(), true});
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
  LogicalResult conflicts() {
    struct Write {
      TransactionalStep *step;
      Predicate guard;
    };
    SmallVector<Write> writes;
    for (auto &rule : p.rules_) {
      Predicate guard{"constant_1", true};
      if (!rule.guard.yields.empty()) {
        auto value = predicate(rule.guard.yields.front());
        if (failed(value))
          return failure();
        guard = *value;
      }
      if ((guard.first == "constant_0" && guard.second) ||
          (guard.first == "constant_1" && !guard.second))
        continue;
      for (auto &step : rule.body.steps)
        if (step->kind == TransactionalStepKind::StateWrite)
          writes.push_back({step.get(), guard});
    }
    for (unsigned i = 0; i < writes.size(); ++i)
      for (unsigned j = i + 1; j < writes.size(); ++j) {
        auto &a = writes[i];
        auto &b = writes[j];
        if (failed(charge(8 + a.guard.first.size() + b.guard.first.size(),
                          b.step->operation)))
          return failure();
        if (a.step->resource != b.step->resource ||
            (a.guard.first == b.guard.first &&
             a.guard.second != b.guard.second))
          continue;
        auto diagnostic = b.step->operation->emitError(
            "transactional State writes may overlap; no implicit arbitration "
            "or overwrite");
        diagnostic.attachNote(a.step->operation->getLoc())
            << "other potentially simultaneous write";
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

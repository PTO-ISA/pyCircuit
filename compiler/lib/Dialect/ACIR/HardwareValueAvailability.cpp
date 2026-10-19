#include "HardwareValueAvailability.h"
#include "HardwareQueueResources.h"
#include "llvm/ADT/ScopeExit.h"
#include <map>
#include <set>

using namespace mlir;
namespace acir::ac::detail {
namespace {
// Canonicalize only identities that hold for four-state bits. In particular,
// neither a | ~a nor an unknown condition is converted to a known Boolean.
class AvailabilityProof {
  const HardwareAnalysis &analysis;
  using Expr = unsigned;
  struct Node {
    std::string kind;
    SmallVector<Expr> children;
  };
  SmallVector<Node> nodes{{"zero", {}}, {"one", {}}};
  std::map<std::string, Expr> interned;
  DenseMap<Value, Expr> formulaCache;
  DenseSet<Value> formulaActive;
  std::map<std::string, Expr> availabilityCache;
  std::set<std::string> active;
  bool invalid = false;

  Expr node(StringRef kind, ArrayRef<Expr> children = {}) {
    std::string key = kind.str();
    for (auto child : children)
      key += ":" + std::to_string(child);
    auto [entry, inserted] = interned.emplace(key, nodes.size());
    if (inserted)
      nodes.push_back({kind.str(), SmallVector<Expr>(children)});
    return entry->second;
  }
  Expr all(ArrayRef<Expr> inputs) {
    SmallVector<Expr> terms;
    for (auto input : inputs) {
      if (input == 0)
        return 0;
      if (input == 1)
        continue;
      if (nodes[input].kind == "and")
        llvm::append_range(terms, nodes[input].children);
      else
        terms.push_back(input);
    }
    llvm::sort(terms);
    terms.erase(std::unique(terms.begin(), terms.end()), terms.end());
    if (terms.empty())
      return 1;
    if (terms.size() == 1)
      return terms.front();
    return node("and", terms);
  }
  Expr select(Expr condition, Expr yes, Expr no) {
    if (condition == 1 || yes == no)
      return yes;
    if (condition == 0)
      return no;
    if (yes == 1 && no == 0)
      return condition;
    return node("select", {condition, yes, no});
  }
  Value wire(Value value) {
    DenseSet<Value> seen;
    while (seen.insert(value).second) {
      if (auto arg = dyn_cast<BlockArgument>(value)) {
        if (auto rule = dyn_cast<RuleOp>(arg.getOwner()->getParentOp())) {
          value = rule.getCaptures()[arg.getArgNumber()];
          continue;
        }
      } else if (auto rule = value.getDefiningOp<RuleOp>()) {
        value = cast<YieldOp>(rule.getBody().front().back())
                    .getValues()[cast<OpResult>(value).getResultNumber()];
        continue;
      }
      return value;
    }
    invalid = true;
    return value;
  }
  std::optional<std::string> constant(Value value) {
    auto op = wire(value).getDefiningOp<BitsConstantOp>();
    if (!op)
      return std::nullopt;
    auto tree = op.getValue().getTree();
    auto literal = tree.getAs<DictionaryAttr>("value");
    auto integer =
        literal ? literal.getAs<MathIntAttr>("value") : MathIntAttr();
    if (!integer)
      return std::nullopt;
    return integer.getCanonicalValue().str();
  }
  Expr scalar(Value value) {
    value = wire(value);
    if (auto number = constant(value))
      return node("constant:" + *number);
    return node("wire:" + std::to_string(reinterpret_cast<uintptr_t>(
                              value.getAsOpaquePointer())));
  }
  Expr equal(Expr left, Expr right) {
    if (left > right)
      std::swap(left, right);
    return node("eq", {left, right});
  }
  Expr formula(Value value) {
    value = wire(value);
    if (auto found = formulaCache.find(value); found != formulaCache.end())
      return found->second;
    if (!formulaActive.insert(value).second) {
      invalid = true;
      return 0;
    }
    llvm::scope_exit cleanup([&] { formulaActive.erase(value); });
    Expr result;
    if (auto number = constant(value);
        number && (*number == "0" || *number == "1"))
      result = *number == "1" ? 1 : 0;
    else if (auto op = value.getDefiningOp<BitsBinaryOp>();
             op && op.getOpcode() == "and")
      result = all({formula(op->getOperand(0)), formula(op->getOperand(1))});
    else if (auto op = value.getDefiningOp<BitsSelectOp>())
      result = select(formula(op.getCondition()), formula(op.getTrueValue()),
                      formula(op.getFalseValue()));
    else if (auto op = value.getDefiningOp<ValueRequireOp>())
      result = select(formula(op.getPath()), formula(op.getAvailability()), 1);
    else if (auto op = value.getDefiningOp<BitsCompareOp>()) {
      auto left = constant(op->getOperand(0));
      auto right = constant(op->getOperand(1));
      StringRef predicate = op.getPredicate();
      if (left && right &&
          (predicate == "eq" || predicate == "case_eq" || predicate == "ne"))
        result = ((*left == *right) != (predicate == "ne")) ? 1 : 0;
      else if (predicate == "eq")
        result = equal(scalar(op->getOperand(0)), scalar(op->getOperand(1)));
      else
        result = scalar(value);
    } else
      result = scalar(value);
    formulaCache.try_emplace(value, result);
    return result;
  }
  Expr reject(Operation *site, StringRef message) {
    site->emitOpError() << message;
    invalid = true;
    return 0;
  }
  Expr tableAt(Value table, uint64_t ordinal, ArrayRef<StringAttr> path) {
    if (auto arg = dyn_cast<BlockArgument>(table))
      if (auto rule = dyn_cast<RuleOp>(arg.getOwner()->getParentOp()))
        return tableAt(rule.getCaptures()[arg.getArgNumber()], ordinal, path);
    if (auto splat = table.getDefiningOp<TableSplatOp>())
      return availability(splat.getInput(), path);
    if (auto view = table.getDefiningOp<TableViewOp>()) {
      auto source = analysis.getViewSourceOrdinal(view, ordinal);
      if (failed(source)) {
        invalid = true;
        return 0;
      }
      return tableAt(view.getInput(), *source, path);
    }
    if (auto create = table.getDefiningOp<TableCreateOp>())
      return ordinal < create.getInputs().size()
                 ? availability(create.getInputs()[ordinal], path)
                 : 1;
    return availability(table, path);
  }
  Expr tableElement(Value table, Value index, ArrayRef<StringAttr> path) {
    auto size = analysis.getTableSize(cast<TableType>(table.getType()), {},
                                      table.getParentRegion()->getParentOp());
    if (failed(size)) {
      invalid = true;
      return 0;
    }
    auto indexWidth = analysis.getPackedWidth(
        index.getType(), {}, index.getParentRegion()->getParentOp());
    if (failed(indexWidth)) {
      invalid = true;
      return 0;
    }
    // Ordinals outside the index carrier cannot be selected. In particular,
    // a Bits1 index into three rows never compares against a truncated two.
    if (*indexWidth < 64)
      *size = std::min(*size, uint64_t(1) << *indexWidth);
    if (auto number = constant(index)) {
      uint64_t ordinal;
      if (!StringRef(*number).getAsInteger(10, ordinal))
        return ordinal < *size ? tableAt(table, ordinal, path) : 1;
    }
    // A known out-of-range index preserves TableGet's existing payload and
    // membership semantics; availability does not introduce a new range guard.
    Expr selected = 1;
    for (uint64_t i = *size; i-- > 0;)
      selected =
          select(equal(scalar(index), node("constant:" + std::to_string(i))),
                 tableAt(table, i, path), selected);
    return selected;
  }
  Expr derive(Value value, ArrayRef<StringAttr> path) {
    if (auto arg = dyn_cast<BlockArgument>(value)) {
      auto *owner = arg.getOwner()->getParentOp();
      if (auto rule = dyn_cast<RuleOp>(owner))
        return availability(rule.getCaptures()[arg.getArgNumber()], path);
      if (auto module = dyn_cast<ModuleOp>(owner)) {
        auto ports = getModulePortLayout(module);
        if (failed(ports)) {
          invalid = true;
          return 0;
        }
        unsigned ordinal = arg.getArgNumber();
        if (ordinal < ports->inputValidity.size() &&
            ports->inputValidity[ordinal] >= 0)
          return formula(
              arg.getOwner()->getArgument(ports->inputValidity[ordinal]));
        return 1;
      }
      return reject(owner,
                    "availability has unsupported region argument provenance");
    }
    Operation *op = value.getDefiningOp();
    if (isa<BitsConstantOp, BitsConstant4Op, EnumCreateOp>(op))
      return 1;
    if (auto rule = dyn_cast<RuleOp>(op)) {
      auto yields = cast<YieldOp>(rule.getBody().front().back()).getValues();
      auto sampled =
          rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      if (sampled) {
        auto fire = sampled.getAs<IntegerAttr>("fire_result");
        if (!fire || fire.getInt() < 0 ||
            uint64_t(fire.getInt()) >= yields.size())
          return reject(rule,
                        "availability requires a valid sampled fire result");
        uint64_t ordinary = fire.getInt();
        auto module = rule->getParentOfType<ModuleOp>();
        auto wiring =
            module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
        auto writers =
            wiring ? wiring.getAs<ArrayAttr>("writers") : ArrayAttr();
        if (writers)
          for (auto raw : writers) {
            auto writer = dyn_cast<DictionaryAttr>(raw);
            if (writer &&
                writer.get("registration") == rule->getAttr("occurrence")) {
              auto count = writer.getAs<IntegerAttr>("ordinary_results");
              if (!count || count.getInt() < 0)
                return reject(
                    rule, "availability requires valid ordinary result count");
              ordinary = count.getInt();
            }
          }
        // Candidate D/E, check witnesses and EHR entries have private routing
        // proofs. They are not authored payload outputs and must not acquire a
        // predecessor-fire prerequisite merely by being transported internally.
        if (cast<OpResult>(value).getResultNumber() >= ordinary)
          return 1;
        return formula(yields[fire.getInt()]);
      }
      return availability(yields[cast<OpResult>(value).getResultNumber()],
                          path);
    }
    if (auto instance = dyn_cast<InstanceOp>(op)) {
      Operation *target = analysis.lookupDefinition(instance.getCallee());
      if (!target)
        return reject(instance, "availability has an unresolved instance");
      if (!analysis.getPrimitiveKind(target).empty())
        return 1;
      auto ports = getModulePortLayout(target);
      if (failed(ports)) {
        invalid = true;
        return 0;
      }
      auto ordinal = cast<OpResult>(value).getResultNumber();
      if (ordinal < ports->outputValidity.size() &&
          ports->outputValidity[ordinal] >= 0)
        return formula(instance->getResult(ports->outputValidity[ordinal]));
      for (auto [payload, companion] : ports->routedOutputValidity)
        if (payload == ordinal)
          return formula(instance->getResult(companion));
      return 1;
    }
    if (auto collection = dyn_cast<CollectionOp>(op)) {
      auto target = analysis.lookupDefinition(collection.getCallee());
      if (target && !analysis.getPrimitiveKind(target).empty())
        return 1;
      if (target) {
        auto ports = getModulePortLayout(target);
        if (succeeded(ports) &&
            llvm::all_of(ports->inputValidity,
                         [](int64_t i) { return i < 0; }) &&
            llvm::all_of(ports->outputValidity,
                         [](int64_t i) { return i < 0; }) &&
            llvm::all_of(collection->getOperands(),
                         [&](Value input) { return availability(input) == 1; }))
          return 1;
      }
      return reject(collection, "availability requires scalar instance routing "
                                "for ordinary Collections");
    }
    if (auto get = dyn_cast<StructGetOp>(op)) {
      SmallVector<StringAttr> nested{get.getFieldAttr()};
      llvm::append_range(nested, path);
      return availability(get.getValue(), nested);
    }
    if (auto create = dyn_cast<StructCreateOp>(op)) {
      if (!path.empty()) {
        auto declaration = analysis.lookupStruct(create.getResult().getType());
        for (auto [i, raw] : llvm::enumerate(declaration.getFields()))
          if (cast<DictionaryAttr>(raw).getAs<StringAttr>("name") ==
              path.front())
            return availability(create.getValues()[i], path.drop_front());
        return reject(create,
                      "availability projection names an unknown Struct field");
      }
    }
    if (auto selectOp = dyn_cast<BitsSelectOp>(op))
      return all({availability(selectOp.getCondition()),
                  select(formula(selectOp.getCondition()),
                         availability(selectOp.getTrueValue(), path),
                         availability(selectOp.getFalseValue(), path))});
    if (auto get = dyn_cast<TableGetOp>(op)) {
      Expr index = availability(get.getIndex());
      return cast<OpResult>(value).getResultNumber() == 1
                 ? index
                 : all({index,
                        tableElement(get.getInput(), get.getIndex(), path)});
    }
    if (auto view = dyn_cast<TableViewOp>(op))
      return availability(view.getInput(), path);
    if (auto require = dyn_cast<ValueRequireOp>(op))
      return all({availability(require.getAvailability()),
                  availability(require.getPath())});
    if (op->getNumRegions()) {
      bool total = llvm::all_of(op->getOperands(), [&](Value input) {
        return availability(input) == 1;
      });
      if (total && isa<TableMapOp, TableMatchOp>(op))
        return 1;
      return reject(op, "availability requires normalized region provenance");
    }
    SmallVector<Expr> operands;
    for (Value operand : op->getOperands())
      operands.push_back(availability(operand, path));
    return all(operands);
  }
  Expr availability(Value value, ArrayRef<StringAttr> path = {}) {
    std::string key =
        std::to_string(reinterpret_cast<uintptr_t>(value.getAsOpaquePointer()));
    for (auto field : path)
      key += "." + field.getValue().str();
    if (auto found = availabilityCache.find(key);
        found != availabilityCache.end())
      return found->second;
    if (!active.insert(key).second) {
      invalid = true;
      return 0;
    }
    llvm::scope_exit cleanup([&] { active.erase(key); });
    Expr result = derive(value, path);
    availabilityCache.emplace(key, result);
    return result;
  }
  LogicalResult matches(Value payload, Value supplied, Operation *site) {
    if (availability(payload) != (supplied ? formula(supplied) : 1) || invalid)
      return site->emitOpError()
             << "availability differs from actual payload SSA";
    return success();
  }

public:
  explicit AvailabilityProof(const HardwareAnalysis &analysis)
      : analysis(analysis) {}
  LogicalResult run() {
    std::optional<DenseSet<OpOperand *>> queueUses;
    for (auto module : analysis.getDefinitions()) {
      auto ports = getModulePortLayout(module);
      if (failed(ports))
        return failure();
      auto yield = cast<YieldOp>(module.getBody().front().back());
      auto yielded = yield.getValues();
      DenseMap<unsigned, unsigned> companions;
      DenseSet<unsigned> controls;
      for (auto [payload, companion] : llvm::enumerate(ports->outputValidity))
        if (companion >= 0) {
          companions.try_emplace(payload, companion);
          controls.insert(companion);
        }
      for (auto [payload, companion] : ports->routedOutputValidity) {
        companions.try_emplace(payload, companion);
        controls.insert(companion);
      }
      DenseSet<unsigned> queueSummaries;
      auto queueWiring = module->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
      if (queueWiring && queueWiring.get("pop_summaries")) {
        // Queue summaries are private control outputs, even when their exact
        // subtree union is an authored public input. Authenticate the actual
        // yield edge, not the shared SSA value. This structural collector never
        // calls sampled-admission or availability proof, so there is no proof
        // recursion through Rule fire/private-route validation.
        if (!queueUses) {
          auto verified = getVerifiedQueueResourceUses(analysis);
          if (failed(verified)) return failure();
          queueUses = std::move(*verified);
        }
        auto rows = queueWiring.getAs<ArrayAttr>("pop_summaries");
        if (!rows || rows.empty()) return failure();
        for (Attribute raw : rows) {
          auto row = dyn_cast<DictionaryAttr>(raw);
          auto output = row ? row.getAs<IntegerAttr>("output") : IntegerAttr();
          if (!row || row.size() != 2 || !row.getAs<DictionaryAttr>("queue") ||
              !output || !output.getType().isSignlessInteger(64) || output.getInt() < 0 ||
              uint64_t(output.getInt()) < ports->sourceOutputCount ||
              uint64_t(output.getInt()) >= yielded.size())
            return module->emitOpError() << "private Queue summary requires its authenticated output ordinal";
          unsigned ordinal = output.getInt();
          if (controls.contains(ordinal) || companions.count(ordinal) ||
              !queueSummaries.insert(ordinal).second ||
              !queueUses->contains(&yield->getOpOperand(ordinal)))
            return module->emitOpError() << "private Queue summary cannot replace public or routed payload availability";
        }
      }
      // Private lexical capture exports also require actual yielded-SSA proof.
      // An unmapped payload is permitted only when that proof establishes
      // totality. Only separately authenticated private Queue summary edges
      // bypass this payload check; other uses of the same value remain checked.
      for (unsigned i = 0; i < yielded.size(); ++i) {
        if (controls.contains(i) || queueSummaries.contains(i))
          continue;
        auto companion = companions.find(i);
        Value supplied = companion == companions.end()
                             ? Value()
                             : yielded[companion->second];
        if (failed(matches(yielded[i], supplied, module)))
          return failure();
      }
      auto result = module.walk([&](Operation *operation) {
        if (auto require = dyn_cast<ValueRequireOp>(operation))
          return failed(matches(require.getPayload(), require.getAvailability(),
                                require))
                     ? WalkResult::interrupt()
                     : WalkResult::advance();
        auto instance = dyn_cast<InstanceOp>(operation);
        if (!instance)
          return WalkResult::advance();
        auto *target = analysis.lookupDefinition(instance.getCallee());
        if (!target)
          return WalkResult::interrupt();
        if (!analysis.getPrimitiveKind(target).empty())
          return WalkResult::advance();
        auto layout = getModulePortLayout(target);
        if (failed(layout))
          return WalkResult::interrupt();
        for (unsigned i = 0; i < layout->sourceInputCount; ++i) {
          int64_t companion = layout->inputValidity[i];
          if (failed(matches(instance->getOperand(i),
                             companion < 0 ? Value()
                                           : instance->getOperand(companion),
                             instance)))
            return WalkResult::interrupt();
        }
        return WalkResult::advance();
      });
      if (result.wasInterrupted())
        return failure();
    }
    return failure(invalid);
  }
};
// Only companion destinations and required availability operands authorize
// transport of a private fire. Shared ordinary-data consumers invalidate it.
class AvailabilityUses {
  const HardwareAnalysis &analysis;
  DenseSet<OpOperand *> roots, backward;
  DenseMap<OpOperand *, bool> closed;
  DenseSet<OpOperand *> active;
  SmallVector<OpOperand *> pending;
  void add(OpOperand &use, bool root = false) {
    if (root)
      roots.insert(&use);
    if (backward.insert(&use).second)
      pending.push_back(&use);
  }
  bool allUsers(Value value) {
    return llvm::all_of(value.getUses(),
                        [&](OpOperand &use) { return permits(use); });
  }
  bool permits(OpOperand &use) {
    if (roots.contains(&use))
      return true;
    if (!backward.contains(&use)) {
      auto *owner = use.getOwner();
      // A retained, dead availability select/AND has no consumer to expose
      // the control to. This does not authorize an ordinary live data route.
      bool transfer = isa<BitsSelectOp>(owner);
      if (auto binary = dyn_cast<BitsBinaryOp>(owner))
        transfer |= binary.getOpcode() == "and";
      return transfer && llvm::all_of(owner->getResults(), [](Value value) {
               return value.use_empty();
             });
    }
    if (auto known = closed.find(&use); known != closed.end())
      return known->second;
    if (!active.insert(&use).second)
      return false;
    llvm::scope_exit cleanup([&] { active.erase(&use); });
    Operation *owner = use.getOwner();
    bool allowed = false;
    if (auto rule = dyn_cast<RuleOp>(owner))
      allowed =
          allUsers(rule.getBody().front().getArgument(use.getOperandNumber()));
    else if (auto yield = dyn_cast<YieldOp>(owner)) {
      if (auto rule = dyn_cast<RuleOp>(yield->getParentOp()))
        allowed = allUsers(rule->getResult(use.getOperandNumber()));
    } else if (isa<BitsBinaryOp, BitsSelectOp>(owner))
      allowed = llvm::all_of(owner->getResults(),
                             [&](Value value) { return allUsers(value); });
    closed.try_emplace(&use, allowed);
    return allowed;
  }

public:
  explicit AvailabilityUses(const HardwareAnalysis &analysis)
      : analysis(analysis) {}
  FailureOr<DenseSet<OpOperand *>> collect() {
    for (auto module : analysis.getDefinitions()) {
      auto layout = getModulePortLayout(module);
      if (failed(layout))
        return failure();
      auto yield = cast<YieldOp>(module.getBody().front().back());
      for (int64_t companion : layout->outputValidity)
        if (companion >= 0)
          add(yield->getOpOperand(companion), true);
      for (auto [payload, companion] : layout->routedOutputValidity)
        add(yield->getOpOperand(companion), true);
      auto walked = module.walk([&](Operation *operation) {
        if (auto require = dyn_cast<ValueRequireOp>(operation))
          add(require->getOpOperand(1), true);
        if (auto instance = dyn_cast<InstanceOp>(operation)) {
          auto *callee = analysis.resolveCallee(instance);
          if (!callee)
            return WalkResult::interrupt();
          auto ports = getModulePortLayout(callee);
          if (failed(ports))
            return WalkResult::interrupt();
          for (int64_t companion : ports->inputValidity)
            if (companion >= 0)
              add(instance->getOpOperand(companion), true);
        }
        return WalkResult::advance();
      });
      if (walked.wasInterrupted())
        return failure();
    }
    while (!pending.empty()) {
      Value value = pending.pop_back_val()->get();
      if (auto argument = dyn_cast<BlockArgument>(value)) {
        if (auto rule = dyn_cast<RuleOp>(argument.getOwner()->getParentOp()))
          add(rule->getOpOperand(argument.getArgNumber()));
        continue;
      }
      Operation *producer = value.getDefiningOp();
      if (auto op = dyn_cast<BitsBinaryOp>(producer);
          op && op.getOpcode() == "and") {
        add(op->getOpOperand(0));
        add(op->getOpOperand(1));
      } else if (auto op = dyn_cast<BitsSelectOp>(producer)) {
        // The selector is a raw predicate reference, not an availability route.
        add(op->getOpOperand(1));
        add(op->getOpOperand(2));
      } else if (auto rule = dyn_cast<RuleOp>(producer)) {
        auto sampled =
            rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
        auto fire =
            sampled ? sampled.getAs<IntegerAttr>("fire_result") : IntegerAttr();
        unsigned index = cast<OpResult>(value).getResultNumber();
        if (!fire || fire.getInt() != index)
          add(rule.getBody().front().back().getOpOperand(index));
      }
    }
    DenseSet<OpOperand *> result;
    for (OpOperand *use : backward)
      if (permits(*use))
        result.insert(use);
    // A physical companion cannot be repurposed as public payload or a raw
    // branch predicate inside another definition.
    for (auto module : analysis.getDefinitions()) {
      auto layout = getModulePortLayout(module);
      if (failed(layout))
        return failure();
      for (int64_t companion : layout->inputValidity)
        if (companion >= 0 &&
            !allUsers(module.getBody().front().getArgument(companion)))
          return module.emitOpError()
                 << "availability companion has a data use";
      auto walked = module.walk([&](InstanceOp instance) {
        auto *callee = analysis.resolveCallee(instance);
        if (!callee)
          return WalkResult::interrupt();
        auto ports = getModulePortLayout(callee);
        if (failed(ports))
          return WalkResult::interrupt();
        for (int64_t companion : ports->outputValidity)
          if (companion >= 0 && !allUsers(instance->getResult(companion))) {
            instance.emitOpError() << "availability companion has a data use";
            return WalkResult::interrupt();
          }
        for (auto [payload, companion] : ports->routedOutputValidity)
          if (!allUsers(instance->getResult(companion))) {
            instance.emitOpError()
                << "routed availability companion has a data use";
            return WalkResult::interrupt();
          }
        return WalkResult::advance();
      });
      if (walked.wasInterrupted())
        return failure();
    }
    return result;
  }
};
bool hasAvailabilityTransport(const HardwareAnalysis &analysis) {
  bool found = false;
  analysis.getPackage().walk([&](Operation *operation) {
    auto sampled =
        operation->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
    found |= isa<ValueRequireOp>(operation) ||
             operation->hasAttr("ac.input_validity") ||
             operation->hasAttr("ac.output_validity") ||
             operation->hasAttr("ac.routed_output_validity") ||
             (sampled && sampled.get("requirements"));
  });
  return found;
}
} // namespace
LogicalResult
verifyHardwareValueAvailability(const HardwareAnalysis &analysis) {
  // Transport-free packages retain independently checked legacy totality,
  // including literal predicates routed through ordinary instances.
  if (!hasAvailabilityTransport(analysis))
    return success();
  if (failed(AvailabilityProof(analysis).run()))
    return failure();
  return success(succeeded(AvailabilityUses(analysis).collect()));
}
FailureOr<DenseSet<OpOperand *>>
getVerifiedValueAvailabilityUses(const HardwareAnalysis &analysis) {
  if (!hasAvailabilityTransport(analysis))
    return DenseSet<OpOperand *>{};
  if (failed(AvailabilityProof(analysis).run()))
    return failure();
  return AvailabilityUses(analysis).collect();
}
} // namespace acir::ac::detail

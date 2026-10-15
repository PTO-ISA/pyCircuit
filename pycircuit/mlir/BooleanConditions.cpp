#include "Dialect.h"
#include "Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include <functional>
#include <map>
#include <stdexcept>
#include <vector>
using namespace mlir;
namespace {
// Compile-time Boolean normalization shared by the C++ and RTL paths. In
// addition to truth equivalence, check the demand of every partial atom: a
// tautology must not silently discard an assertion or change a read snapshot.
struct Logic {
  struct Node {
    unsigned atom, low, high;
  };
  struct Limit {};
  std::vector<Node> nodes{{0, 0, 0}, {0, 1, 1}};
  std::vector<Value> atoms;
  DenseMap<Value, unsigned> formulas, atomIds;
  DenseMap<Value, unsigned> requested;
  bool checked = false;
  std::map<std::tuple<unsigned, unsigned, unsigned>, unsigned> unique, memo;
  DenseMap<unsigned, unsigned> inverses;
  std::map<std::tuple<unsigned, const void *, const void *>, unsigned>
      comparisons;
  std::map<std::string, unsigned> reads;
  unsigned node(unsigned atom, unsigned low, unsigned high) {
    if (low == high)
      return low;
    auto key = std::make_tuple(atom, low, high);
    if (auto it = unique.find(key); it != unique.end())
      return it->second;
    if (nodes.size() >= 100000)
      throw Limit();
    unsigned id = nodes.size();
    nodes.push_back({atom, low, high});
    unique[key] = id;
    return id;
  }
  unsigned combine(unsigned a, unsigned b, bool isOr) {
    if (a > b)
      std::swap(a, b);
    if (a == b)
      return a;
    if (isOr) {
      if (a == 1 || b == 1)
        return 1;
      if (a == 0)
        return b;
    } else {
      if (a == 0)
        return 0;
      if (a == 1)
        return b;
    }
    auto key = std::make_tuple(a, b, unsigned(isOr));
    if (auto it = memo.find(key); it != memo.end())
      return it->second;
    auto atom = std::min(nodes[a].atom, nodes[b].atom);
    auto l = combine(nodes[a].atom == atom ? nodes[a].low : a,
                     nodes[b].atom == atom ? nodes[b].low : b, isOr);
    auto h = combine(nodes[a].atom == atom ? nodes[a].high : a,
                     nodes[b].atom == atom ? nodes[b].high : b, isOr);
    return memo[key] = node(atom, l, h);
  }
  unsigned both(unsigned a, unsigned b) { return combine(a, b, false); }
  unsigned either(unsigned a, unsigned b) { return combine(a, b, true); }
  unsigned negate(unsigned id) {
    if (id < 2)
      return 1 - id;
    if (auto it = inverses.find(id); it != inverses.end())
      return it->second;
    auto n = nodes[id];
    auto result = node(n.atom, negate(n.low), negate(n.high));
    inverses[id] = result;
    inverses[result] = id;
    return result;
  }
  unsigned formula(Value value) {
    if (auto it = formulas.find(value); it != formulas.end())
      return it->second;
    auto *op = value.getDefiningOp();
    auto n = op->getName().getStringRef();
    unsigned result;
    auto read = dyn_cast<acir::ReadOp>(op);
    bool scalar = read && !op->hasAttr("resources");
    auto method =
        read ? op->getAttrOfType<StringAttr>("method").getValue() : StringRef();
    bool totalReady =
        scalar && value == read.getReady() &&
        (method == "read" || method == "empty" || method == "size" ||
         (op->getAttrOfType<IntegerAttr>("txn").getInt() >= 0 &&
          method != "peek" && method != "get"));
    if (totalReady || (checked && n == "acir.assert"))
      result = 1;
    else if (auto constant = dyn_cast<arith::ConstantOp>(op))
      result = !cast<IntegerAttr>(constant.getValue()).getValue().isZero();
    else if (n == "arith.andi")
      result = both(formula(op->getOperand(0)), formula(op->getOperand(1)));
    else if (n == "arith.ori")
      result = either(formula(op->getOperand(0)), formula(op->getOperand(1)));
    else if (n == "arith.xori") {
      auto a = formula(op->getOperand(0)), b = formula(op->getOperand(1));
      result = either(both(a, negate(b)), both(negate(a), b));
    } else if (n == "arith.select") {
      auto c = formula(op->getOperand(0));
      result = either(both(c, formula(op->getOperand(1))),
                      both(negate(c), formula(op->getOperand(2))));
    } else if (n == "acir.bind" || n == "acir.export")
      result = formula(op->getOperand(0));
    else {
      unsigned atom = atoms.size();
      result = node(atom, 0, 1);
      if (auto cmp = dyn_cast<arith::CmpIOp>(op)) {
        auto p = unsigned(cmp.getPredicate());
        auto inverse = unsigned(arith::invertPredicate(cmp.getPredicate()));
        auto key = std::make_tuple(std::min(p, inverse),
                                   cmp.getLhs().getAsOpaquePointer(),
                                   cmp.getRhs().getAsOpaquePointer());
        if (auto it = comparisons.find(key); it != comparisons.end())
          result = p < inverse ? it->second : negate(it->second);
        else
          comparisons[key] = p < inverse ? result : negate(result);
      }
      if (scalar && ((method == "empty" && value == read.getValue()) ||
                     (method == "peek" && value == read.getReady()))) {
        auto key = op->getAttrOfType<StringAttr>("resource").getValue().str();
        if (auto it = reads.find(key); it != reads.end())
          result = method == "empty" ? it->second : negate(it->second);
        else
          reads[key] = method == "empty" ? result : negate(result);
      }
      if (result == node(atom, 0, 1)) {
        atoms.push_back(value);
        atomIds[value] = atom;
      }
    }
    return formulas[value] = result;
  }
  DenseMap<Value, bool> hazards;
  bool partial(Value v) {
    if (auto it = hazards.find(v); it != hazards.end())
      return it->second;
    auto *op = v.getDefiningOp();
    auto n = op->getName().getStringRef();
    bool result = n == "acir.assert" || n == "acir.array.index" ||
                  n == "acir.update" || n.starts_with("arith.div") ||
                  n.starts_with("arith.rem") || n == "arith.floordivsi";
    if (n.starts_with("arith.sh")) {
      auto amount = op->getOperand(1).getDefiningOp<arith::ConstantOp>();
      result = !amount || cast<IntegerAttr>(amount.getValue())
                              .getValue()
                              .uge(cast<IntegerType>(v.getType()).getWidth());
    }
    if (auto r = dyn_cast<acir::ReadOp>(op)) {
      auto method = r->getAttrOfType<StringAttr>("method").getValue();
      return hazards[v] =
                 r->getAttrOfType<IntegerAttr>("txn").getInt() < 0 &&
                 (method == "get" || method == "full" || method == "ehr.read");
    }
    if (n == "acir.extract" && !op->hasAttr("field"))
      result = true;
    for (auto input : op->getOperands())
      result |= partial(input);
    return hazards[v] = result;
  }
  bool preservesDemand(Value root, unsigned canonical) {
    DenseMap<Value, unsigned> original;
    std::vector<Value> pending;
    auto request = [&](Value v, unsigned path) {
      auto updated = either(requested.lookup(v), path);
      if (updated != requested.lookup(v)) {
        requested[v] = updated;
        pending.push_back(v);
      }
    };
    request(root, 1);
    while (!pending.empty()) {
      auto v = pending.back();
      pending.pop_back();
      auto path = requested.lookup(v);
      auto *op = v.getDefiningOp();
      auto n = op->getName().getStringRef();
      if (n == "arith.andi" || n == "arith.ori") {
        auto lhs = formula(op->getOperand(0));
        request(op->getOperand(0), path);
        request(op->getOperand(1),
                both(path, n == "arith.andi" ? lhs : negate(lhs)));
      } else if (n == "arith.select") {
        auto c = formula(op->getOperand(0));
        request(op->getOperand(0), path);
        request(op->getOperand(1), both(path, c));
        request(op->getOperand(2), both(path, negate(c)));
      } else if (n == "arith.xori" || n == "acir.bind" || n == "acir.export") {
        for (auto x : op->getOperands())
          request(x, path);
      } else if (!isa<arith::ConstantOp>(op))
        original[v] = either(original.lookup(v), path);
    }
    DenseMap<unsigned, unsigned> normalized, paths;
    std::vector<unsigned> queue;
    auto demand = [&](unsigned id, unsigned path) {
      if (id < 2)
        return;
      auto updated = either(paths.lookup(id), path);
      if (updated != paths.lookup(id)) {
        paths[id] = updated;
        queue.push_back(id);
      }
    };
    demand(canonical, 1);
    while (!queue.empty()) {
      auto id = queue.back();
      queue.pop_back();
      auto path = paths.lookup(id);
      auto n = nodes[id];
      normalized[n.atom] = either(normalized.lookup(n.atom), path);
      auto atom = node(n.atom, 0, 1);
      demand(n.low, both(path, negate(atom)));
      demand(n.high, both(path, atom));
    }
    // Comparisons and their inverses share a truth atom, but retain separate
    // demand obligations if their operands can fail.
    for (auto [v, path] : original)
      if (partial(v) &&
          !(checked &&
            v.getDefiningOp()->getName().getStringRef() == "acir.assert")) {
        auto f = formula(v);
        if (f < 2 || normalized.lookup(nodes[f].atom) != path)
          return false;
      }
    for (auto [atom, path] : normalized)
      if (partial(atoms[atom]) && !original.count(atoms[atom]))
        return false;
    return true;
  }
  Value emit(unsigned id, OpBuilder &b, Location loc,
             DenseMap<unsigned, Value> &values) {
    if (auto it = values.find(id); it != values.end())
      return it->second;
    if (id < 2)
      return values[id] = arith::ConstantIntOp::create(b, loc, id, 1);
    auto n = nodes[id];
    auto atom = atoms[n.atom];
    if (n.low == 0 && n.high == 1)
      return values[id] = atom;
    auto inverse = [&]() -> Value {
      return arith::XOrIOp::create(b, loc, atom, emit(1, b, loc, values));
    };
    Value result;
    if (n.low == 1 && n.high == 0)
      result = inverse();
    else if (n.low == 0)
      result =
          arith::AndIOp::create(b, loc, atom, emit(n.high, b, loc, values));
    else if (n.high == 1)
      result = arith::OrIOp::create(b, loc, atom, emit(n.low, b, loc, values));
    else if (n.high == 0)
      result =
          arith::AndIOp::create(b, loc, inverse(), emit(n.low, b, loc, values));
    else if (n.low == 1)
      result =
          arith::OrIOp::create(b, loc, inverse(), emit(n.high, b, loc, values));
    else
      result =
          arith::SelectOp::create(b, loc, atom, emit(n.high, b, loc, values),
                                  emit(n.low, b, loc, values));
    return values[id] = result;
  }
};
struct Conditions : PassWrapper<Conditions, OperationPass<mlir::ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(Conditions)
  StringRef getArgument() const final { return "acir-simplify-conditions"; }
  StringRef getDescription() const final {
    return "Normalize Boolean conditions while preserving partial evaluation "
           "paths";
  }
  void runOnOperation() override {
    getOperation().walk([&](Operation *op) {
      if (!isa<acir::FireOp, acir::EffectOp>(op))
        return;
      auto root = op->getOperand(0);
      try {
        Logic logic;
        auto formula = logic.formula(root);
        bool preserves = logic.preservesDemand(root, formula);
        SmallVector<std::pair<Value, unsigned>> checks;
        if (isa<acir::FireOp>(op)) {
          // A successful assertion contributes true to readiness. Carry its
          // original lazy evaluation obligation explicitly on the transaction
          // instead of duplicating the checked token throughout every phi.
          for (auto atom : logic.atoms)
            if (atom.getDefiningOp()->getName().getStringRef() == "acir.assert")
              checks.push_back({atom, logic.requested.lookup(atom)});
        }
        if (!checks.empty()) {
          // Rebuild formulas with checked atoms fixed to their successful
          // value. Demand predicates for later checks use the same fact: a
          // failed earlier check throws before any later evaluation.
          Logic reduced;
          reduced.checked = true;
          formula = reduced.formula(root);
          if (!reduced.preservesDemand(root, formula))
            return;
          OpBuilder b(op);
          DenseMap<unsigned, Value> originalValues, reducedValues;
          SmallVector<Value> obligations;
          for (auto [assertion, demand] : checks) {
            auto enabled = logic.emit(demand, b, root.getLoc(), originalValues);
            auto guard = reduced.formula(enabled);
            reduced.requested.clear();
            if (!reduced.preservesDemand(enabled, guard))
              return;
            auto condition =
                reduced.emit(guard, b, root.getLoc(), reducedValues);
            auto yes = arith::ConstantIntOp::create(b, root.getLoc(), 1, 1);
            obligations.push_back(arith::SelectOp::create(
                b, assertion.getLoc(), condition, assertion, yes));
          }
          op->setOperand(
              0, reduced.emit(formula, b, root.getLoc(), reducedValues));
          op->insertOperands(op->getNumOperands(), obligations);
          return;
        }
        if (!preserves)
          return;
        // Bound representation growth as well as decision graph construction.
        llvm::DenseSet<unsigned> reachable;
        std::function<void(unsigned)> visit = [&](unsigned id) {
          if (id < 2 || !reachable.insert(id).second)
            return;
          visit(logic.nodes[id].low);
          visit(logic.nodes[id].high);
        };
        visit(formula);
        if (reachable.size() > logic.formulas.size())
          return;
        OpBuilder b(op);
        DenseMap<unsigned, Value> values;
        op->setOperand(0, logic.emit(formula, b, root.getLoc(), values));
      } catch (const Logic::Limit &) {
        // A bounded optimizer must leave the original correct graph usable.
      }
    });
  }
};
} // namespace
std::unique_ptr<Pass> acir::createConditionPass() {
  return std::make_unique<Conditions>();
}

#include "CppLowering.h"
#include "Dialect.h"
#include "Execution.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinAttributes.h"
#include <cctype>
using namespace mlir;
std::string acir::cppIdentifier(llvm::StringRef source) {
  static const std::set<std::string> reserved = {
      "alignas",       "alignof",     "and",
      "and_eq",        "asm",         "auto",
      "bitand",        "bitor",       "bool",
      "break",         "case",        "catch",
      "char",          "char8_t",     "char16_t",
      "char32_t",      "class",       "compl",
      "concept",       "const",       "consteval",
      "constexpr",     "constinit",   "const_cast",
      "continue",      "co_await",    "co_return",
      "co_yield",      "decltype",    "default",
      "delete",        "do",          "double",
      "dynamic_cast",  "else",        "enum",
      "explicit",      "export",      "extern",
      "false",         "float",       "for",
      "friend",        "goto",        "if",
      "inline",        "int",         "long",
      "mutable",       "namespace",   "new",
      "noexcept",      "not",         "not_eq",
      "nullptr",       "operator",    "or",
      "or_eq",         "private",     "protected",
      "public",        "register",    "reinterpret_cast",
      "requires",      "return",      "short",
      "signed",        "sizeof",      "static",
      "static_assert", "static_cast", "struct",
      "switch",        "template",    "this",
      "thread_local",  "throw",       "true",
      "try",           "typedef",     "typeid",
      "typename",      "union",       "unsigned",
      "using",         "virtual",     "void",
      "volatile",      "wchar_t",     "while",
      "xor",           "xor_eq"};
  std::string out;
  for (auto c : source) {
    auto next =
        std::isalnum(static_cast<unsigned char>(c)) || c == '_' ? c : '_';
    if (next != '_' || out.empty() || out.back() != '_')
      out += next;
  }
  if (out.empty())
    out = "value";
  if (std::isdigit(static_cast<unsigned char>(out[0])) || out[0] == '_')
    out = "value" + out;
  if (reserved.count(out))
    out += "_value";
  return out;
}
namespace {
std::string locationName(Location loc) {
  if (auto n = dyn_cast<NameLoc>(loc)) {
    auto name = n.getName().getValue();
    if (name.starts_with("acir.binding:"))
      return name.rsplit(':').second.str();
    return locationName(n.getChildLoc());
  }
  if (auto c = dyn_cast<CallSiteLoc>(loc))
    return locationName(c.getCallee());
  if (auto f = dyn_cast<FusedLoc>(loc))
    for (auto l : f.getLocations()) {
      auto n = locationName(l);
      if (!n.empty())
        return n;
    }
  return "";
}
} // namespace
std::string acir::cppSourceName(Value v) { return locationName(v.getLoc()); }
acir::CppPlan::CppPlan(mlir::ModuleOp module) {
  std::function<void(Value, int64_t)> visit = [&](Value v, int64_t txn) {
    if (!transactions[v].insert(txn).second)
      return;
    auto *op = v.getDefiningOp();
    // Read path conditions are schedule evidence; they are not data operands.
    unsigned first = isa<ReadOp>(op) ? 1 : 0;
    for (auto x : op->getOperands().drop_front(first))
      visit(x, txn);
    if (isa<ReadOp>(op))
      for (auto x : op->getResults())
        transactions[x].insert(txn);
  };
  module.walk([&](Operation *op) {
    if (isa<FireOp, EffectOp>(op)) {
      auto txn = op->getAttrOfType<IntegerAttr>("txn").getInt();
      for (auto v : op->getOperands())
        visit(v, txn);
    }
  });
  module.walk([&](Operation *op) {
    // Schedule evidence and unused IR definitions do not constitute runtime
    // expression uses. Count the graph reachable from transaction statements.
    bool live = isa<FireOp, EffectOp>(op);
    for (auto value : op->getResults())
      live |= !transactions[value].empty();
    if (!live)
      return;
    unsigned first = isa<ReadOp>(op) ? 1 : 0;
    for (auto v : op->getOperands().drop_front(first))
      ++uses[v];
  });
  module.walk([&](Operation *op) {
    if (isa<FireOp>(op))
      return;
    std::vector<Value> outputs;
    for (auto v : op->getResults()) {
      if (transactions[v].size() < 2 ||
          op->getName().getStringRef() == "arith.constant")
        continue;
      bool crosses = isa<ReadOp>(op);
      for (auto *user : v.getUsers()) {
        if (isa<FireOp, EffectOp>(user))
          crosses = true;
        for (auto result : user->getResults())
          if (transactions[result].size() < 2)
            crosses = true;
      }
      if (crosses)
        outputs.push_back(v);
    }
    if (outputs.empty())
      return;
    // Both read results share one snapshot, including when readiness is first.
    if (isa<ReadOp>(op))
      outputs.assign(op->getResults().begin(), op->getResults().end());
    for (auto v : outputs)
      boundary[v] = blocks.size();
    blocks.push_back(outputs);
  });
}
acir::CppStatements::CppStatements(
    const CppPlan &p, std::function<std::string(Type)> t,
    std::function<std::string(Value, llvm::ArrayRef<std::string>)> e,
    std::function<std::string(Value)> s)
    : plan(p), type(t), expression(e), shared(s) {
  names["ctx"] = 1;
}
std::string acir::CppStatements::name(std::string base) {
  base = cppIdentifier(base);
  auto &serial = names[base];
  for (;;) {
    auto n = serial++;
    auto result = base + (n ? "_" + std::to_string(n + 1) : "");
    if (assigned.insert(result).second)
      return result;
  }
}
void acir::CppStatements::line(const std::string &text) {
  code << std::string(depth * 2, ' ') << text << '\n';
}
void acir::CppStatements::conditional(const std::string &condition,
                                      const std::function<void()> &body) {
  auto outer = available;
  auto outerComputed = computed;
  line("if (" + condition + ") {");
  ++depth;
  body();
  --depth;
  line("}");
  available = outer;
  computed = outerComputed;
}
namespace {
// Reduced ordered decision graph for demand predicates. Each value is emitted
// once per statement region, under the union of the paths that actually use it.
struct Demand {
  struct Node {
    unsigned atom, low, high;
  };
  std::vector<Node> nodes{{0, 0, 0}, {0, 1, 1}};
  std::map<std::tuple<unsigned, unsigned, unsigned>, unsigned> unique, memo;
  std::map<std::tuple<unsigned, const void *, const void *>, unsigned>
      comparisons;
  std::map<std::string, unsigned> reads;
  DenseMap<Value, unsigned> atoms, formulas;
  DenseMap<unsigned, unsigned> negations;
  std::vector<Value> values;
  DenseSet<Value> frontier;
  DenseMap<Value, bool> known;
  std::function<Attribute(Value)> literal;
  unsigned node(unsigned atom, unsigned low, unsigned high) {
    if (low == high)
      return low;
    auto key = std::make_tuple(atom, low, high);
    if (auto it = unique.find(key); it != unique.end())
      return it->second;
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
      if (b == 1 || a == 1)
        return 1;
      if (a == 0)
        return b;
    } else {
      if (a == 0)
        return 0;
      if (a == 1)
        return b;
      if (b == 1)
        return a;
    }
    auto key = std::make_tuple(a, b, unsigned(isOr));
    if (auto it = memo.find(key); it != memo.end())
      return it->second;
    auto atom = std::min(nodes[a].atom, nodes[b].atom);
    auto low = combine(nodes[a].atom == atom ? nodes[a].low : a,
                       nodes[b].atom == atom ? nodes[b].low : b, isOr);
    auto high = combine(nodes[a].atom == atom ? nodes[a].high : a,
                        nodes[b].atom == atom ? nodes[b].high : b, isOr);
    return memo[key] = node(atom, low, high);
  }
  unsigned negate(unsigned id) {
    if (id < 2)
      return 1 - id;
    if (auto it = negations.find(id); it != negations.end())
      return it->second;
    auto n = nodes[id];
    auto result = node(n.atom, negate(n.low), negate(n.high));
    negations[id] = result;
    negations[result] = id;
    return result;
  }
  unsigned boolean(Value v) {
    if (auto it = formulas.find(v); it != formulas.end())
      return it->second;
    auto *op = v.getDefiningOp();
    auto n = op->getName().getStringRef();
    unsigned result;
    auto constant = dyn_cast_or_null<IntegerAttr>(literal(v));
    if (known.count(v))
      result = known.at(v);
    else if (constant && !frontier.count(v))
      result = !constant.getValue().isZero();
    else if (frontier.count(v)) {
      unsigned atom;
      if (auto it = atoms.find(v); it != atoms.end())
        atom = it->second;
      else {
        atom = values.size();
        atoms[v] = atom;
        values.push_back(v);
      }
      result = node(atom, 0, 1);
    } else if (n == "arith.constant")
      result = !op->getAttrOfType<IntegerAttr>("value").getValue().isZero();
    else if (n == "acir.bind" || n == "acir.export" || n == "acir.fire")
      result = boolean(op->getOperand(0));
    else if (n == "acir.unary" &&
             op->getAttrOfType<StringAttr>("operator_name").getValue() == "not")
      result = negate(boolean(op->getOperand(0)));
    else if (n == "arith.andi" || n == "arith.ori")
      result = combine(boolean(op->getOperand(0)), boolean(op->getOperand(1)),
                       n == "arith.ori");
    else if (n == "arith.xori") {
      auto a = boolean(op->getOperand(0)), b = boolean(op->getOperand(1));
      result = combine(combine(a, negate(b), false),
                       combine(negate(a), b, false), true);
    } else if (n == "arith.select") {
      auto c = boolean(op->getOperand(0));
      result =
          combine(combine(c, boolean(op->getOperand(1)), false),
                  combine(negate(c), boolean(op->getOperand(2)), false), true);
    } else {
      unsigned atom;
      if (auto it = atoms.find(v); it != atoms.end())
        atom = it->second;
      else {
        atom = values.size();
        atoms[v] = atom;
        values.push_back(v);
      }
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
      if (auto read = dyn_cast<acir::ReadOp>(op))
        if (!op->hasAttr("resources")) {
          auto method = op->getAttrOfType<StringAttr>("method").getValue();
          if ((method == "empty" && v == read.getValue()) ||
              (method == "peek" && v == read.getReady())) {
            auto key =
                op->getAttrOfType<StringAttr>("resource").getValue().str();
            if (auto it = reads.find(key); it != reads.end())
              result = method == "empty" ? it->second : negate(it->second);
            else
              reads[key] = method == "empty" ? result : negate(result);
          }
        }
    }
    return formulas[v] = result;
  }
  void dependencies(unsigned id, DenseSet<Value> &out) {
    DenseSet<unsigned> visited;
    std::function<void(unsigned)> walk = [&](unsigned n) {
      if (n < 2 || !visited.insert(n).second)
        return;
      out.insert(values[nodes[n].atom]);
      walk(nodes[n].low);
      walk(nodes[n].high);
    };
    walk(id);
  }
};
} // namespace
void acir::CppStatements::assume(Value condition) {
  Demand logic;
  logic.literal = [](Value value) -> Attribute {
    if (auto c = value.getDefiningOp<arith::ConstantOp>())
      return c.getValue();
    return {};
  };
  for (auto &[value, block] : plan.boundary)
    if (!ignored.count(value))
      logic.frontier.insert(value);
  facts.push_back(condition);
  unsigned path = 1;
  for (auto fact : facts)
    path = logic.combine(path, logic.boolean(fact), false);
  // Reuse predicates already evaluated for the successful fire, including
  // conditional assertion results, without a second evaluation or SSA cache.
  for (auto &[value, local] : computed)
    if (value.getType().isInteger(1)) {
      auto predicate = logic.boolean(value);
      if (!logic.combine(path, logic.negate(predicate), false))
        available[value] = "true";
      else if (!logic.combine(path, predicate, false))
        available[value] = "false";
    }
  available[condition] = "true";
}
std::string acir::CppStatements::value(Value root) {
  if (auto it = available.find(root); it != available.end())
    return it->second;
  if (plan.boundary.count(root) && !ignored.count(root))
    return shared(root);
  if (root.getType().isInteger(1) && !facts.empty()) {
    Demand proof;
    proof.literal = [](Value value) -> Attribute {
      if (auto c = value.getDefiningOp<arith::ConstantOp>())
        return c.getValue();
      return {};
    };
    for (auto &[value, block] : plan.boundary)
      if (!ignored.count(value))
        proof.frontier.insert(value);
    unsigned path = 1;
    for (auto fact : facts)
      path = proof.combine(path, proof.boolean(fact), false);
    auto predicate = proof.boolean(root);
    if (!proof.combine(path, proof.negate(predicate), false))
      return available[root] = "true";
    if (!proof.combine(path, predicate, false))
      return available[root] = "false";
  }
  Demand guards;
  DenseMap<Value, Attribute> constants;
  DenseSet<Value> tested;
  std::function<Attribute(Value)> literal = [&](Value v) -> Attribute {
    if (!tested.insert(v).second)
      return constants.lookup(v);
    auto *op = v.getDefiningOp();
    auto n = op->getName().getStringRef();
    if (isa<ReadOp, FireOp>(op) || n == "acir.assert")
      return {};
    SmallVector<Attribute> inputs;
    for (auto operand : op->getOperands()) {
      auto value = literal(operand);
      if (!value)
        return {};
      inputs.push_back(value);
    }
    try {
      return constants[v] = acir::calculate(
                 op, inputs, op->getParentOfType<mlir::ModuleOp>());
    } catch (const std::exception &) {
      return {};
    }
  };
  guards.literal = literal;
  for (auto &[v, name] : available) {
    if (name == "true" || name == "false")
      guards.known[v] = name == "true";
    // Keep Boolean identities visible: an available p and !p are correlated.
    // Treating every earlier SSA name as an independent atom invents paths.
  }
  for (auto &[v, id] : plan.boundary)
    if (!ignored.count(v))
      guards.frontier.insert(v);
  DenseMap<Value, unsigned> demands;
  std::vector<Value> pending, order;
  auto external = [&](Value v) {
    return plan.boundary.count(v) && !ignored.count(v);
  };
  auto bounded = [&](Value coordinate, Type container) {
    if (auto s = dyn_cast<SelectedType>(container))
      container = s.getElementType();
    int64_t size = 0;
    if (auto a = dyn_cast<ArrayType>(container))
      size = a.getShape().front();
    if (auto m = dyn_cast<MaskType>(container))
      size = m.getShape().front();
    if (auto t = dyn_cast<TupleType>(container))
      size = t.size();
    if (auto folded = dyn_cast_or_null<IntegerAttr>(literal(coordinate)))
      return folded.getValue().getLimitedValue() < uint64_t(size);
    auto *index = coordinate.getDefiningOp();
    while (index->getName().getStringRef() == "acir.bind") {
      coordinate = index->getOperand(0);
      index = coordinate.getDefiningOp();
    }
    if (auto c = index->getAttrOfType<IntegerAttr>("value"))
      return c.getValue().getLimitedValue() < uint64_t(size);
    if (index->getName().getStringRef() != "arith.select")
      return false;
    auto *compare = index->getOperand(0).getDefiningOp();
    auto predicate = compare->getAttrOfType<IntegerAttr>("predicate");
    if (!predicate || predicate.getInt() != 6 ||
        compare->getOperand(0) != index->getOperand(1))
      return false;
    auto limit =
        compare->getOperand(1).getDefiningOp()->getAttrOfType<IntegerAttr>(
            "value");
    auto fallback =
        index->getOperand(2).getDefiningOp()->getAttrOfType<IntegerAttr>(
            "value");
    return limit && fallback &&
           limit.getValue().getLimitedValue() <= uint64_t(size) &&
           fallback.getValue().getLimitedValue() < uint64_t(size);
  };
  auto primitiveGuarded = [&](Value v) {
    auto *op = v.getDefiningOp();
    auto n = op->getName().getStringRef();
    if (n == "acir.assert")
      return !literal(v);
    if (n.contains("div") || n.contains("rem")) {
      auto denominator =
          dyn_cast_or_null<IntegerAttr>(literal(op->getOperand(1)));
      return !denominator || denominator.getValue().isZero();
    }
    if (n == "acir.array.index")
      return !bounded(op->getOperand(1), op->getOperand(0).getType());
    if (n == "acir.update") {
      Type current = op->getOperand(0).getType();
      if (auto selected = dyn_cast<SelectedType>(current))
        current = selected.getElementType();
      unsigned coordinate = 2;
      for (auto step : op->getAttrOfType<ArrayAttr>("path")) {
        if (auto field = dyn_cast<StringAttr>(step)) {
          auto structure = dyn_cast<StructType>(current);
          if (!structure)
            return true;
          auto model = op->getParentOfType<mlir::ModuleOp>()
                           ->getAttrOfType<DictionaryAttr>("acir.model");
          auto fields = model.getAs<DictionaryAttr>("types").getAs<ArrayAttr>(
              structure.getName());
          Type next;
          for (auto f : fields) {
            auto d = cast<DictionaryAttr>(f);
            if (d.getAs<StringAttr>("name") == field)
              next = d.getAs<TypeAttr>("payload_type").getValue();
          }
          if (!next)
            return true;
          current = next;
        } else {
          if (!bounded(op->getOperand(coordinate++), current))
            return true;
          if (auto array = dyn_cast<ArrayType>(current))
            current = array.getShape().size() == 1
                          ? array.getElementType()
                          : Type(ArrayType::get(op->getContext(),
                                                array.getElementType(),
                                                array.getShape().drop_front()));
          else
            return true;
        }
      }
      return false;
    }
    if (n == "acir.extract" && !op->hasAttr("field"))
      return !bounded(op->getOperand(1), op->getOperand(0).getType());
    if (n == "arith.shli" || n == "arith.shrui" || n == "arith.shrsi") {
      auto amount = op->getOperand(1);
      while (amount.getDefiningOp()->getName().getStringRef() == "acir.bind")
        amount = amount.getDefiningOp()->getOperand(0);
      auto literal =
          amount.getDefiningOp()->getAttrOfType<IntegerAttr>("value");
      return !literal || literal.getValue().getLimitedValue() >=
                             cast<IntegerType>(v.getType()).getWidth();
    }
    if (external(v) && isa<ReadOp>(op)) {
      auto method = op->getAttrOfType<StringAttr>("method").getValue();
      return method == "full" || method == "get" || method == "ehr.read";
    }
    return false;
  };
  DenseMap<Value, bool> hazards;
  std::function<bool(Value)> hazardous = [&](Value v) {
    if (auto it = hazards.find(v); it != hazards.end())
      return it->second;
    bool result = primitiveGuarded(v);
    auto *op = v.getDefiningOp();
    unsigned first = isa<ReadOp>(op) ? 1 : 0;
    for (auto operand : op->getOperands().drop_front(first))
      result = hazardous(operand) || result;
    return hazards[v] = result;
  };
  auto guarded = [&](Value v) {
    return external(v) ? hazardous(v) : primitiveGuarded(v);
  };
  auto request = [&](Value v, unsigned path) {
    if (!path || available.count(v))
      return;
    if (!hazardous(v))
      path = 1;
    auto joined = guards.combine(demands.lookup(v), path, true);
    if (joined != demands.lookup(v)) {
      if (external(v)) {
        for (auto output : plan.blocks[plan.boundary.at(v)]) {
          auto combined = guards.combine(demands.lookup(output), joined, true);
          demands[output] = combined;
          pending.push_back(output);
        }
      } else {
        demands[v] = joined;
        pending.push_back(v);
      }
    }
  };
  request(root, 1);
  while (!pending.empty()) {
    auto v = pending.back();
    pending.pop_back();
    auto path = demands.at(v);
    if (external(v))
      continue;
    auto *op = v.getDefiningOp();
    auto n = op->getName().getStringRef();
    bool boolean =
        (n == "arith.andi" || n == "arith.ori") && v.getType().isInteger(1);
    if (hazardous(v) && (n == "arith.select" || boolean)) {
      auto c = op->getOperand(0);
      request(c, path);
      auto condition = guards.boolean(c);
      request(op->getOperand(1),
              guards.combine(
                  path, n == "arith.ori" ? guards.negate(condition) : condition,
                  false));
      if (!boolean)
        request(op->getOperand(2),
                guards.combine(path, guards.negate(condition), false));
    } else {
      unsigned first = isa<ReadOp>(op) ? 1 : 0;
      for (auto x : op->getOperands().drop_front(first))
        request(x, path);
    }
  }
  DenseMap<Value, unsigned> marks;
  bool conditionalCycle = false;
  std::function<void(Value)> visit = [&](Value v) {
    if (!demands.count(v) || marks.lookup(v) == 2)
      return;
    if (marks.lookup(v) == 1) {
      // Unioning demand paths across opaque shared boundaries can introduce
      // ordering edges absent from the SSA DAG. Keep the original conditional
      // evaluation order rather than rejecting an acyclic computation.
      conditionalCycle = true;
      return;
    }
    marks[v] = 1;
    DenseSet<Value> controls;
    if (guarded(v))
      guards.dependencies(demands.at(v), controls);
    // Visit controls in stable atom order, never by pointer/hash order.
    for (auto atom : guards.values)
      if (controls.count(atom))
        visit(atom);
    auto *op = v.getDefiningOp();
    unsigned first = isa<ReadOp>(op) ? 1 : 0;
    if (!external(v))
      for (auto x : op->getOperands().drop_front(first))
        visit(x);
    marks[v] = 2;
    order.push_back(v);
  };
  visit(root);
  if (conditionalCycle) {
    std::function<std::string(Value)> emit = [&](Value v) -> std::string {
      if (auto it = available.find(v); it != available.end())
        return it->second;
      auto *op = v.getDefiningOp();
      auto n = op->getName().getStringRef();
      if (!external(v) && n == "arith.constant")
        return available[v] = expression(v, {});
      auto base = cppSourceName(v);
      if (base.empty() && n == "acir.extract" && op->hasAttr("field"))
        base = op->getAttrOfType<StringAttr>("field").getValue().str();
      auto local = name(base.empty() ? "branch_value" : base);
      bool boolean = v.getType().isInteger(1) &&
                     (n == "arith.andi" || n == "arith.ori");
      if (!external(v) && hazardous(v) && (n == "arith.select" || boolean)) {
        auto condition = emit(op->getOperand(0));
        line(type(v.getType()) + " " + local + "{};");
        auto outer = available;
        auto outerComputed = computed;
        line("if (" + condition + ") {");
        ++depth;
        auto yes = boolean && n == "arith.ori" ? std::string("true")
                                               : emit(op->getOperand(1));
        line(local + " = " + yes + ";");
        --depth;
        available = outer;
        computed = outerComputed;
        line("} else {");
        ++depth;
        auto no = boolean && n == "arith.andi"
                      ? std::string("false")
                      : emit(op->getOperand(boolean ? 1 : 2));
        line(local + " = " + no + ";");
        --depth;
        line("}");
        available = outer;
        computed = outerComputed;
      } else {
        SmallVector<std::string> args;
        if (!external(v)) {
          unsigned first = isa<ReadOp>(op) ? 1 : 0;
          for (auto operand : op->getOperands().drop_front(first))
            args.push_back(emit(operand));
        }
        auto expr = external(v) ? shared(v) : expression(v, args);
        if (!external(v) && cppSourceName(v).empty() &&
            (expr == "true" || expr == "false" || llvm::is_contained(args, expr)))
          return available[v] = expr;
        line("const " + type(v.getType()) + " " + local + " = " + expr + ";");
      }
      computed[v] = local;
      return available[v] = local;
    };
    return emit(root);
  }
  DenseMap<Value, std::string> locals;
  for (auto v : order) {
    auto *op = v.getDefiningOp();
    auto n = op->getName().getStringRef();
    if (!external(v) && n == "arith.constant") {
      locals[v] = expression(v, {});
      continue;
    }
    auto base = cppSourceName(v);
    if (auto read = dyn_cast<ReadOp>(op))
      if (v == read.getReady())
        base.clear();
    if (base.empty()) {
      if (auto r = dyn_cast<ReadOp>(op)) {
        auto resource = op->getAttrOfType<StringAttr>("array");
        if (!resource)
          resource = op->getAttrOfType<StringAttr>("resource");
        auto method = op->getAttrOfType<StringAttr>("method").getValue();
        auto suffix = v == r.getReady() ? "_ready"
                      : method == "full" ? "_full"
                      : method == "empty" ? "_empty"
                      : method == "size" ? "_size" : "_value";
        base = resource.getValue().rsplit('.').second.str() + suffix;
      } else if (n == "arith.cmpi")
        base = "condition";
      else if (n == "arith.select")
        base = "selected_value";
      else if (v.getType().isInteger(1))
        base = "ready";
      else if (n == "acir.extract" && op->hasAttr("field"))
        base = op->getAttrOfType<StringAttr>("field").getValue().str();
      else if (n == "acir.aggregate")
        base = "payload";
      else
        base = "computed_value";
    }
    auto local = name(base);
    locals[v] = local;
    auto ref = [&](Value x) -> std::string {
      if (auto it = locals.find(x); it != locals.end())
        return it->second;
      if (auto it = available.find(x); it != available.end())
        return it->second;
      if (plan.boundary.count(x) && !ignored.count(x))
        return shared(x);
      // A branch proven unreachable by the demand decision graph.
      return type(x.getType()) + "{}";
    };
    SmallVector<std::string> args;
    unsigned first = isa<ReadOp>(op) ? 1 : 0;
    if (!external(v))
      for (auto x : op->getOperands().drop_front(first))
        args.push_back(ref(x));
    auto expr = external(v) ? shared(v) : expression(v, args);
    auto demand = demands.at(v);
    if (demand == 1 || !guarded(v)) {
      bool anonymous = cppSourceName(v).empty();
      if (auto read = dyn_cast<ReadOp>(op))
        anonymous |= v == read.getReady();
      if (anonymous && (expr == "true" || expr == "false" ||
                        llvm::is_contained(args, expr))) {
        locals[v] = expr;
        continue;
      }
      bool inlineValue = !external(v) && cppSourceName(v).empty() &&
                         !isa<ReadOp>(op) && plan.uses.lookup(v) <= 1 &&
                         expr.size() < 100;
      if (inlineValue)
        locals[v] = expr;
      else
        line("const " + type(v.getType()) + " " + local + " = " + expr + ";");
    } else {
      DenseMap<unsigned, std::string> predicates;
      std::function<std::string(unsigned)> predicate =
          [&](unsigned id) -> std::string {
        if (id < 2)
          return id ? "true" : "false";
        if (auto it = predicates.find(id); it != predicates.end())
          return it->second;
        auto node = guards.nodes[id];
        auto atom = guards.values[node.atom];
        auto c = computed.count(atom) && !locals.count(atom) &&
                         !available.count(atom)
                     ? computed.at(atom)
                     : ref(atom);
        std::string result;
        if (node.low == 0 && node.high == 1)
          result = c;
        else if (node.low == 1 && node.high == 0)
          result = "(!" + c + ")";
        else if (node.low == 0)
          result = "(" + c + " && " + predicate(node.high) + ")";
        else if (node.high == 1)
          result = "(" + c + " || " + predicate(node.low) + ")";
        else
          result = "(" + c + " ? " + predicate(node.high) + " : " +
                   predicate(node.low) + ")";
        if (result.size() > 100) {
          auto path = name("condition_path");
          line("const bool " + path + " = " + result + ";");
          result = path;
        }
        return predicates[id] = result;
      };
      auto condition = predicate(demand);
      line(type(v.getType()) + " " + local + "{};");
      line("if (" + condition + ") {");
      ++depth;
      line(local + " = " + expr + ";");
      --depth;
      line("}");
    }
  }
  for (auto v : order)
    computed[v] = locals.at(v);
  for (auto v : order)
    if (demands.at(v) == 1)
      available[v] = locals.at(v);
  return available[root];
}

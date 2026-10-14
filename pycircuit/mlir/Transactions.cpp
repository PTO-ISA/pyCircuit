#include "Dialect.h"
#include "Execution.h"
#include "Passes.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/SmallPtrSet.h"
#include <map>
#include <set>
#include <stdexcept>
using namespace mlir;
namespace {
Operation *make(OpBuilder &b, Location loc, StringRef name,
                ValueRange args = {}, TypeRange results = {},
                ArrayRef<NamedAttribute> attrs = {}) {
  OperationState state(loc, name);
  state.addOperands(args);
  state.addTypes(results);
  state.addAttributes(attrs);
  return b.create(state);
}
std::string text(Operation *op, StringRef key) {
  auto a = op->getAttrOfType<StringAttr>(key);
  return a ? a.getValue().str() : "";
}
struct Pair {
  Value value, ready;
  std::string resource;
  int producer = -1;
  Value index;
  std::vector<Value> selected;
};
struct Transaction {
  std::string name;
  Value condition, fire;
  std::vector<Operation *> effects;
};
struct Extractor {
  mlir::ModuleOp source, target;
  OpBuilder b;
  acir::CircuitOp circuit;
  Location loc;
  Value yes, no;
  std::vector<Transaction> txns;
  std::vector<std::string> frames;
  std::map<std::string, Operation *> symbols, states;
  std::map<std::string, std::vector<std::string>> arrays;
  std::map<std::string, std::vector<Operation *>> accesses;
  std::map<std::pair<std::string, std::string>, std::string> relations;
  std::map<std::pair<std::string, std::string>, Pair> exports;
  std::map<Operation *, bool> changes;
  std::set<Operation *> checking;
  int active = -1;
  std::vector<Operation *> callSites;
  std::map<std::pair<int, const void *>, int> branches;
  int nextBranch = 0;
  Extractor(mlir::ModuleOp source)
      : source(source), target(mlir::ModuleOp::create(source.getLoc())),
        b(source.getContext()), loc(source.getLoc()) {
    target->setAttrs(source->getAttrs());
    target->setAttr("acir.stage", b.getStringAttr("transaction"));
    source.walk([&](Operation *op) {
      if (auto a = op->getAttrOfType<StringAttr>("sym_name"))
        if (!symbols.count(a.getValue().str()) || !isa<func::FuncOp>(op) ||
            !cast<func::FuncOp>(op).isDeclaration())
          symbols[a.getValue().str()] = op;
    });
    b.setInsertionPointToEnd(target.getBody());
    OperationState st(loc, "acir.circuit");
    st.addRegion();
    circuit = cast<acir::CircuitOp>(b.create(st));
    circuit.getBody().push_back(new Block());
    b.setInsertionPointToEnd(&circuit.getBody().front());
    yes = constant(1, 1);
    no = constant(0, 1);
    circuit->setAttr("stage", b.getStringAttr("transaction"));
    for (auto scope : source.getOps<acir::ModuleOp>())
      for (auto contract : scope.getDeclarations().getOps<acir::ContractOp>())
        for (auto r : contract.getRelations()) {
          auto d = cast<DictionaryAttr>(r);
          auto prefix = scope.getSymName().str() + ":";
          auto a = prefix + d.getAs<StringAttr>("before").getValue().str(),
               z = prefix + d.getAs<StringAttr>("after").getValue().str(),
               kind = d.getAs<StringAttr>("relation").getValue().str();
          auto key = std::make_pair(a, z);
          if (relations.count(key) && relations[key] != kind)
            throw std::runtime_error("contradictory Rule relations: " + a +
                                     " / " + z);
          relations[key] = kind;
          if (kind == "C" || kind == "CF")
            relations[{z, a}] = kind;
        }
    // Transitive declared order, used for visibility only within the
    // declaration graph.
    bool changed = true;
    while (changed) {
      changed = false;
      auto copy = relations;
      for (auto &[ab, k] : copy)
        if (k == "SB" || k == "SBR")
          for (auto &[cd, l] : copy)
            if ((l == "SB" || l == "SBR") && ab.second == cd.first) {
              auto key = std::make_pair(ab.first, cd.second);
              if (key.first == key.second)
                throw std::runtime_error("cycle in declared Rule contract: " +
                                         key.first);
              if (!relations.count(key)) {
                relations[key] = "SB";
                changed = true;
              }
            }
    }
  }
  Value constant(uint64_t n, unsigned width = 32) {
    return arith::ConstantIntOp::create(b, loc, n, width);
  }
  Value conjunction(Value a, Value c) {
    if (a == yes)
      return c;
    if (c == yes)
      return a;
    if (a == no || c == no)
      return no;
    return arith::AndIOp::create(b, loc, a, c);
  }
  Value disjunction(Value a, Value c) {
    if (a == no)
      return c;
    if (c == no)
      return a;
    if (a == yes || c == yes)
      return yes;
    return arith::OrIOp::create(b, loc, a, c);
  }
  Value negate(Value a) {
    if (a == yes)
      return no;
    if (a == no)
      return yes;
    return arith::XOrIOp::create(b, loc, a, yes);
  }
  Value select(Value c, Value a, Value z) {
    if (a == z)
      return a;
    if (c == yes)
      return a;
    if (c == no)
      return z;
    return arith::SelectOp::create(b, loc, c, a, z);
  }
  Pair pathAnd(Pair path, Pair condition, bool invert = false) {
    return {conjunction(path.value,
                        invert ? negate(condition.value) : condition.value),
            conjunction(path.ready, condition.ready)};
  }
  void require(Pair path, Value ready) {
    if (active >= 0)
      txns[active].condition = conjunction(
          txns[active].condition,
          disjunction(negate(path.value), conjunction(path.ready, ready)));
  }
  SmallVector<NamedAttribute> metadata(std::string res, std::string method,
                                       int txn) {
    SmallVector<Attribute> context;
    for (auto &f : frames)
      context.push_back(b.getStringAttr(f));
    return {b.getNamedAttr("resource", b.getStringAttr(res)),
            b.getNamedAttr("method", b.getStringAttr(method)),
            b.getNamedAttr("txn", b.getI64IntegerAttr(txn)),
            b.getNamedAttr("port", b.getI64IntegerAttr(0)),
            b.getNamedAttr("frames", b.getArrayAttr(context))};
  }
  int branch(Value path, int txn) {
    auto key = std::make_pair(txn, path.getAsOpaquePointer());
    auto found = branches.find(key);
    if (found != branches.end())
      return found->second;
    int id = nextBranch++;
    branches[key] = id;
    make(b, loc, "acir.path", path, {},
         {b.getNamedAttr("txn", b.getI64IntegerAttr(txn)),
          b.getNamedAttr("branch", b.getI64IntegerAttr(id)),
          b.getNamedAttr("kind", b.getStringAttr("branch"))});
    return id;
  }
  Pair read(std::string res, std::string method, Type type, Pair path) {
    auto attrs = metadata(res, method, active);
    attrs.push_back(b.getNamedAttr(
        "branch", b.getI64IntegerAttr(branch(path.value, active))));
    auto *op =
        make(b, loc, "acir.read", path.value, {type, b.getI1Type()}, attrs);
    accesses[res].push_back(op);
    require(path, op->getResult(1));
    return {op->getResult(0), op->getResult(1)};
  }
  void effect(std::string res, std::string method, Value value, Pair path,
              Value ready) {
    if (active < 0)
      throw std::runtime_error("state effect outside a transaction: " + res);
    require(path, ready);
    if (method == "pop")
      for (auto *request : txns[active].effects)
        if (text(request, "resource") == res &&
            text(request, "method") == "pop") {
          auto joined = disjunction(request->getOperand(0), path.value);
          request->setOperand(0, joined);
          request->setAttr("branch",
                           b.getI64IntegerAttr(branch(joined, active)));
          auto available = read(res, "empty", b.getI1Type(), path);
          require(path, negate(available.value));
          return;
        }
    auto attrs = metadata(res, method, active);
    attrs.push_back(b.getNamedAttr(
        "branch", b.getI64IntegerAttr(branch(path.value, active))));
    SmallVector<Value> args{path.value};
    if (value)
      args.push_back(value);
    auto *op = make(b, loc, "acir.effect", args, {}, attrs);
    txns[active].effects.push_back(op);
    if (method != "check")
      accesses[res].push_back(op);
    if (method == "push") {
      auto full = read(res, "full", b.getI1Type(), path);
      require(path, negate(full.value));
    }
    if (method == "pop") {
      auto available = read(res, "empty", b.getI1Type(), path);
      require(path, negate(available.value));
    }
  }
  Type memberType(Type type, Attribute field) {
    if (auto array = dyn_cast<acir::ArrayType>(type))
      return array.getShape().size() == 1 ? array.getElementType() :
        acir::ArrayType::get(b.getContext(), array.getElementType(), array.getShape().drop_front());
    if (auto structure = dyn_cast<acir::StructType>(type)) {
      auto fields = source->getAttrOfType<DictionaryAttr>("acir.model")
                      .getAs<DictionaryAttr>("types").getAs<ArrayAttr>(structure.getName());
      for (auto f : fields) {
        auto entry = cast<DictionaryAttr>(f);
        if (entry.getAs<StringAttr>("name") == field) return entry.getAs<TypeAttr>("payload_type").getValue();
      }
    }
    throw std::runtime_error("invalid aggregate update path");
  }
  Value cmpIndex(Value index, int64_t coordinate) {
    return arith::CmpIOp::create(b, loc, arith::CmpIPredicate::eq, index,
      constant(coordinate, cast<IntegerType>(index.getType()).getWidth()));
  }
  Value maskBit(Value mask, ArrayRef<int64_t> shape, int64_t flat) {
    for (unsigned axis = 0; axis < shape.size(); ++axis) {
      int64_t stride = 1;
      for (auto d : shape.drop_front(axis + 1)) stride *= d;
      auto coord = flat / stride; flat %= stride;
      Type result = axis + 1 == shape.size() ? b.getI1Type() :
        Type(acir::MaskType::get(b.getContext(), shape.drop_front(axis + 1)));
      mask = make(b, loc, "acir.extract", {mask, constant(coord)}, result)->getResult(0);
    }
    return mask;
  }
  Value fullMask(ArrayRef<int64_t> shape) {
    SmallVector<Value> values;
    for (int64_t i = 0; i < shape.front(); ++i)
      values.push_back(shape.size() == 1 ? yes : fullMask(shape.drop_front()));
    return make(b, loc, "acir.aggregate", values, acir::MaskType::get(b.getContext(), shape))->getResult(0);
  }
  Pair readReference(Pair reference, std::string method, Type type, Pair path) {
    if (!arrays.count(reference.resource))
      return read(reference.resource, method, type, path);
    if (!reference.index)
      throw std::runtime_error("index a Queue array before reading");
    Pair result{make(b, loc, "acir.aggregate", {}, type)->getResult(0), no};
    for (auto [i, res] : llvm::enumerate(arrays[reference.resource])) {
      auto chosen = arith::CmpIOp::create(b, loc, arith::CmpIPredicate::eq,
                                          reference.index, constant(i, cast<IntegerType>(reference.index.getType()).getWidth()));
      Value selected = chosen;
      if (!reference.selected.empty()) selected = conjunction(selected, reference.selected[i]);
      auto branchPath = pathAnd(path, {selected, reference.ready});
      auto value = read(res, method, type, branchPath);
      result.value = select(selected, value.value, result.value);
      result.ready =
          disjunction(result.ready, conjunction(selected, value.ready));
    }
    result.ready = conjunction(result.ready, reference.ready);
    require(path, result.ready);
    return result;
  }
  void effectReference(Pair reference, std::string method, Value value,
                       Pair path, Value ready) {
    if (!arrays.count(reference.resource)) {
      effect(reference.resource, method, value, path, ready);
      return;
    }
    if (!reference.index)
      throw std::runtime_error("index a Queue array before writing");
    auto valid = arith::CmpIOp::create(
        b, loc, arith::CmpIPredicate::ult, reference.index,
        constant(arrays[reference.resource].size(), cast<IntegerType>(reference.index.getType()).getWidth()));
    Value member = reference.selected.empty() ? yes : no;
    for (auto [i, bit] : llvm::enumerate(reference.selected))
      member = disjunction(member, conjunction(bit, cmpIndex(reference.index, i)));
    require(path, conjunction(reference.ready, conjunction(valid, member)));
    for (auto [i, res] : llvm::enumerate(arrays[reference.resource])) {
      auto chosen = arith::CmpIOp::create(b, loc, arith::CmpIPredicate::eq,
                                          reference.index, constant(i, cast<IntegerType>(reference.index.getType()).getWidth()));
      Value selected = chosen;
      if (!reference.selected.empty()) selected = conjunction(selected, reference.selected[i]);
      effect(res, method, value, pathAnd(path, {selected, reference.ready}),
             ready);
    }
  }
  bool modifying(Operation *op) {
    if (auto i = changes.find(op); i != changes.end())
      return i->second;
    if (!checking.insert(op).second)
      throw std::runtime_error("recursive Rule calls");
    bool result = false;
    op->walk([&](Operation *nested) {
      auto name = nested->getName().getStringRef();
      if (name.ends_with(".write") || name == "acir.signal.set" ||
          name == "acir.queue.push" || name == "acir.queue.pop")
        result = true;
      if (name == "acir.queue.peek" && text(nested, "role") == "consume")
        result = true;
      if (auto call = dyn_cast<func::CallOp>(nested)) {
        auto *callee = symbols.at(call.getCallee().str());
        if (!cast<func::FuncOp>(callee).isDeclaration() && modifying(callee))
          result = true;
      }
      if (auto call = dyn_cast<acir::RuleCallOp>(nested)) {
        auto *callee =
            symbols.at(call.getTarget().getLeafReference().getValue().str());
        if (modifying(callee) || (call->hasAttr("output_binding") ||
                                  call->hasAttr("output_bindings")))
          result = true;
      }
    });
    checking.erase(op);
    changes[op] = result;
    return result;
  }
  Pair merge(std::vector<std::pair<Pair, Pair>> values, Type type) {
    if (values.empty())
      return {make(b, loc, "acir.aggregate", {}, type)->getResult(0), no};
    auto result = values.back().second;
    Value ready = no;
    for (auto &[path, v] : values)
      ready = disjunction(
          ready, conjunction(path.value, conjunction(path.ready, v.ready)));
    for (auto it = values.rbegin() + 1; it != values.rend(); ++it) {
      if (result.resource != it->second.resource)
        throw std::runtime_error("runtime resource selection is not supported; "
                                 "branch around operations");
      result.value = select(it->first.value, it->second.value, result.value);
      if (result.index && it->second.index)
        result.index = select(it->first.value, it->second.index, result.index);
      if (result.producer != it->second.producer && result.producer >= 0)
        throw std::runtime_error(
            "independent transaction values cannot be merged");
    }
    result.ready = ready;
    return result;
  }
  Pair run(Region &region, ArrayRef<Pair> arguments, Pair path) {
    struct Incoming {
      Pair path;
      SmallVector<Pair> args;
    };
    std::map<Block *, std::vector<Incoming>> incoming;
    std::map<Block *, unsigned> indegrees;
    for (auto &block : region) {
      indegrees[&block] = 0;
    }
    for (auto &block : region)
      for (auto *succ : block.getSuccessors())
        ++indegrees[succ];
    std::vector<Block *> order;
    std::vector<Block *> pending;
    for (auto &[block, n] : indegrees)
      if (n == 0)
        pending.push_back(block);
    while (!pending.empty()) {
      auto *block = pending.back();
      pending.pop_back();
      order.push_back(block);
      for (auto *succ : block->getSuccessors())
        if (--indegrees[succ] == 0)
          pending.push_back(succ);
    }
    if (order.size() != region.getBlocks().size())
      throw std::runtime_error("runtime loop requires a static bound; cyclic "
                               "CFG cannot lower to finite RTL");
    // Structured control dependence ends at the immediate postdominator.
    // A failed Module condition disables its branches, while independent code
    // after their join must remain evaluable. Phi values retain their validity.
    std::map<Block *, std::set<Block *>> postdom;
    std::set<Block *> all;
    for (auto &block : region)
      all.insert(&block);
    for (auto *block : all)
      postdom[block] =
          block->getNumSuccessors() ? all : std::set<Block *>{block};
    bool update = true;
    while (update) {
      update = false;
      for (auto *block : all)
        if (block->getNumSuccessors()) {
          auto successors = block->getSuccessors();
          auto common = postdom[*successors.begin()];
          for (auto *s : successors) {
            std::set<Block *> intersection;
            std::set_intersection(
                common.begin(), common.end(), postdom[s].begin(),
                postdom[s].end(),
                std::inserter(intersection, intersection.end()));
            common = std::move(intersection);
          }
          common.insert(block);
          if (common != postdom[block]) {
            postdom[block] = common;
            update = true;
          }
        }
    }
    std::map<Block *, Pair> resumed;
    incoming[&region.front()].push_back({path, SmallVector<Pair>(arguments)});
    Value checkReady = yes;
    DenseMap<Value, Pair> env;
    std::vector<std::pair<Pair, Pair>> returns;
    for (auto *block : order) {
      auto &ins = incoming[block];
      if (ins.empty())
        continue;
      Pair current{no, yes};
      for (auto &in : ins) {
        current.value = disjunction(current.value, in.path.value);
        current.ready = conjunction(
            current.ready, disjunction(negate(in.path.value), in.path.ready));
      }
      Pair operationPath = current;
      if (resumed.count(block))
        operationPath = resumed[block];
      for (auto [i, arg] : llvm::enumerate(block->getArguments())) {
        std::vector<std::pair<Pair, Pair>> vals;
        for (auto &in : ins)
          vals.push_back({in.path, in.args[i]});
        env[arg] = merge(vals, arg.getType());
      }
      current = operationPath;
      for (auto &operation : *block) {
        auto *op = &operation;
        loc = op->getLoc();
        auto name = op->getName().getStringRef();
        SmallVector<Pair> a;
        for (auto v : op->getOperands())
          a.push_back(env.lookup(v));
        auto allReady = yes;
        for (auto v : a)
          allReady = conjunction(allReady, v.ready);
        Pair result;
        if (auto br = dyn_cast<cf::BranchOp>(op)) {
          incoming[br.getDest()].push_back({current, a});
          continue;
        }
        if (auto br = dyn_cast<cf::CondBranchOp>(op)) {
          require(current, a[0].ready);
          auto candidates = postdom[block];
          candidates.erase(block);
          Block *join = nullptr;
          for (auto *candidate : candidates)
            if (!join || postdom[candidate].size() > postdom[join].size())
              join = candidate;
          if (join)
            resumed[join] = current;
          SmallVector<Pair> t, z;
          for (auto v : br.getTrueDestOperands())
            t.push_back(env.lookup(v));
          for (auto v : br.getFalseDestOperands())
            z.push_back(env.lookup(v));
          incoming[br.getTrueDest()].push_back({pathAnd(current, a[0]), t});
          incoming[br.getFalseDest()].push_back(
              {pathAnd(current, a[0], true), z});
          continue;
        }
        if (isa<acir::ReturnOp, func::ReturnOp, acir::GuardYieldOp,
                acir::BodyEndOp>(op)) {
          if (!a.empty()) {
            auto ret = a[0];
            if (op->hasAttr("omitted"))
              ret.ready = no;
            returns.push_back({current, ret});
          } else
            returns.push_back({current, {no, yes}});
          continue;
        }
        if (auto ref = dyn_cast<acir::GetOp>(op)) {
          result = {no, yes,
                    ref.getTarget().getLeafReference().getValue().str()};
        } else if (auto external = dyn_cast<acir::ExternalOp>(op)) {
          auto key = std::make_pair(external.getSource().str(),
                                    external.getIdentity().str());
          if (!exports.count(key))
            throw std::runtime_error("shared computation must dominate its use: " + external.getSource().str());
          result = exports.at(key);
        } else if (name == "acir.config") {
          auto config = source->getAttrOfType<DictionaryAttr>("acir.config");
          auto value = config ? config.get(text(op, "name")) : Attribute();
          if (!value)
            throw std::runtime_error("missing constructor configuration: " +
                                     text(op, "name"));
          result = {arith::ConstantOp::create(b, loc,
                                              op->getResult(0).getType(),
                                              cast<TypedAttr>(value)),
                    yes};
        } else if (auto call = dyn_cast<acir::ModuleCallOp>(op)) {
          auto instance = call.getTarget().getLeafReference().getValue();
          runBody(instance.str() + "_body", current);
          continue;
        } else if (auto call = dyn_cast<acir::RuleCallOp>(op)) {
          auto *callee =
              symbols.at(call.getTarget().getLeafReference().getValue().str());
          bool boundary = active < 0 &&
                          (modifying(callee) || op->hasAttr("output_binding") ||
                           op->hasAttr("output_bindings"));
          int previous = active;
          if (boundary) {
            for (auto arg : a)
              if (arg.producer >= 0)
                throw std::runtime_error(
                    "ordinary return value crosses independent transactions");
            active = txns.size();
            auto meta = callee->getAttrOfType<DictionaryAttr>("acir.function");
            auto name = meta.getAs<StringAttr>("instance").getValue().str() +
                        "." + meta.getAs<StringAttr>("rule").getValue().str() +
                        "#" + std::to_string(active);
            txns.push_back({name,
                            conjunction(current.value,
                                        conjunction(current.ready, allReady)),
                            {},
                            {}});
          }
          result = callRule(callee, a, current);
          if (auto binding =
                  op->getAttrOfType<SymbolRefAttr>("output_binding")) {
            auto res = binding.getLeafReference().getValue().str();
            auto kind = text(states.at(res), "kind");
            effect(res,
                   kind == "queue"    ? "push"
                   : kind == "signal" ? "set"
                   : kind == "ehr"    ? "ehr.write"
                                      : "write",
                   result.value,
                   {conjunction(current.value, result.ready), current.ready},
                   yes);
          }
          if (auto bindings = op->getAttrOfType<ArrayAttr>("output_bindings")) {
            auto tuple = cast<TupleType>(result.value.getType());
            for (auto [i, binding] : llvm::enumerate(bindings))
              if (auto symbol = dyn_cast<SymbolRefAttr>(binding)) {
                auto res = symbol.getLeafReference().getValue().str(),
                     kind = text(states.at(res), "kind");
                auto value = make(b, loc, "acir.extract",
                                  {result.value, constant(i)}, tuple.getType(i))
                                 ->getResult(0);
                auto present =
                    make(b, loc, "acir.extract",
                         {result.value, constant(i + bindings.size())},
                         b.getI1Type())
                        ->getResult(0);
                effect(res,
                       kind == "queue"    ? "push"
                       : kind == "signal" ? "set"
                                          : "write",
                       value,
                       {conjunction(current.value, present), current.ready},
                       result.ready);
              }
          }
          if (boundary) {
            auto &txn = txns[active];
            auto *fire =
                make(b, loc, "acir.fire", txn.condition, b.getI1Type(),
                     {b.getNamedAttr("name", b.getStringAttr(txn.name)),
                      b.getNamedAttr("txn", b.getI64IntegerAttr(active))});
            txn.fire = fire->getResult(0);
            for (auto *effect : txn.effects) {
              b.setInsertionPointToEnd(&circuit.getBody().front());
              auto enable = conjunction(txn.fire, effect->getOperand(0));
              effect->setOperand(0, enable);
              effect->moveBefore(&circuit.getBody().front(),
                                 circuit.getBody().front().end());
            }
            result.producer = active;
            active = previous;
          }
        } else if (auto call = dyn_cast<func::CallOp>(op)) {
          auto *callee = symbols.at(call.getCallee().str());
          auto fn = cast<func::FuncOp>(callee);
          if (fn.isDeclaration()) {
            auto meta = fn->getAttrOfType<DictionaryAttr>("acir.function");
            auto intrinsic = meta.getAs<StringAttr>("intrinsic").getValue();
            bool sign =
                meta.getAs<StringAttr>("result").getValue().starts_with("i");
            StringRef opcode = intrinsic == "shl" ? "arith.shli"
                               : intrinsic == "shr"
                                   ? (sign ? "arith.shrsi" : "arith.shrui")
                               : intrinsic == "div"
                                   ? (sign ? "arith.floordivsi" : "arith.divui")
                                   : (sign ? "arith.remsi" : "arith.remui");
            SmallVector<Value> values;
            for (auto arg : a)
              values.push_back(arg.value);
            result = {make(b, loc, opcode, values, op->getResultTypes())
                          ->getResult(0),
                      allReady};
          } else
            result = run(fn.getBody(), a, current);
        } else if (name == "acir.array.domain" && !a[0].resource.empty()) {
          auto shape = cast<acir::MaskType>(op->getResult(0).getType()).getShape();
          if (!a[0].selected.empty()) throw std::runtime_error("match requires payload, not resource array");
          result = {fullMask(shape), a[0].ready};
        } else if (name == "acir.array.select" && !a[0].resource.empty()) {
          result = a[0]; result.ready = allReady;
          auto shape = cast<acir::MaskType>(a[1].value.getType()).getShape();
          int64_t count = 1; for (auto d : shape) count *= d;
          result.selected.clear();
          for (auto [i, res] : llvm::enumerate(arrays.at(result.resource))) {
            Value enabled = no;
            for (int64_t j = 0; j < count; ++j) {
              auto chosen = a[0].index ? cmpIndex(a[0].index, int64_t(i) - j) : (i == size_t(j) ? yes : no);
              enabled = disjunction(enabled, conjunction(chosen, maskBit(a[1].value, shape, j)));
            }
            if (!a[0].selected.empty()) enabled = conjunction(enabled, a[0].selected[i]);
            result.selected.push_back(enabled);
          }
        } else if (name == "acir.array.write") {
          auto type = op->getOperand(0).getType();
          if (auto selected = dyn_cast<acir::SelectedType>(type)) type = selected.getElementType();
          auto array = cast<acir::ArrayType>(type);
          require(current, a[0].ready);
          for (auto [i, res] : llvm::enumerate(arrays.at(a[0].resource))) {
            Value enabled = a[0].selected.empty() ? yes : a[0].selected[i];
            if (a[0].index) {
              Value inside = no;
              for (int64_t j = 0; j < array.getSize(); ++j)
                inside = disjunction(inside, cmpIndex(a[0].index, int64_t(i) - j));
              enabled = conjunction(enabled, inside);
            }
            auto kind = text(states.at(res), "kind");
            effect(res, kind == "queue" ? "push" : kind == "ehr" ? "ehr.write" : kind == "signal" ? "set" : "write",
                   a[1].value, pathAnd(current, {enabled, a[0].ready}), a[1].ready);
          }
          continue;
        } else if (name == "acir.array.index") {
          auto inputType = op->getOperand(0).getType();
          bool selected = isa<acir::SelectedType>(inputType);
          if (selected) inputType = cast<acir::SelectedType>(inputType).getElementType();
          auto array = dyn_cast<acir::ArrayType>(inputType);
          auto mask = dyn_cast<acir::MaskType>(inputType);
          auto shape = array ? array.getShape() : mask.getShape();
          auto index = a[1].value;
          auto width = cast<IntegerType>(index.getType()).getWidth();
          auto valid = arith::CmpIOp::create(b, loc, arith::CmpIPredicate::ult,
                                            index, constant(shape.front(), width));
          if (!a[0].resource.empty()) {
            if (width < 64) index = arith::ExtUIOp::create(b, loc, b.getI64Type(), index);
            int64_t stride = 1;
            for (auto d : shape.drop_front()) stride *= d;
            Value offset = arith::MulIOp::create(b, loc, index, constant(stride, 64));
            if (a[0].index) offset = arith::AddIOp::create(b, loc, a[0].index, offset);
            result = {no, conjunction(allReady, valid), a[0].resource, -1, offset, a[0].selected};
          } else {
            auto safe = select(valid, index, constant(0, width));
            Value ready = conjunction(allReady, valid);
            if (selected && shape.size() == 1) {
              auto domain = make(b, loc, "acir.array.domain", a[0].value, acir::MaskType::get(b.getContext(), shape))->getResult(0);
              auto enabled = make(b, loc, "acir.extract", {domain, safe}, b.getI1Type())->getResult(0);
              ready = conjunction(ready, enabled);
            }
            result = {make(b, loc, "acir.array.index", {a[0].value, safe}, op->getResultTypes())->getResult(0), ready};
          }
        } else if (name == "acir.update") {
          auto rootType = op->getOperand(0).getType();
          auto originalType = rootType;
          Value root = a[0].value, domain;
          if (auto selected = dyn_cast<acir::SelectedType>(rootType)) {
            rootType = selected.getElementType();
            domain = make(b, loc, "acir.array.domain", root,
              acir::MaskType::get(b.getContext(), cast<acir::ArrayType>(rootType).getShape()))->getResult(0);
            root = make(b, loc, "acir.extract", {root, constant(0)}, rootType)->getResult(0);
          }
          SmallVector<Value> inputs{root, a[1].value};
          auto type = rootType;
          Value valid = allReady;
          unsigned dynamic = 2;
          for (auto step : op->getAttrOfType<ArrayAttr>("path")) {
            if (isa<UnitAttr>(step)) {
              auto array = cast<acir::ArrayType>(type);
              auto index = a[dynamic++].value;
              auto width = cast<IntegerType>(index.getType()).getWidth();
              auto inRange = arith::CmpIOp::create(b, loc, arith::CmpIPredicate::ult,
                index, constant(array.getShape().front(), width));
              valid = conjunction(valid, inRange);
              auto safe = select(inRange, index, constant(0, width));
              inputs.push_back(safe);
              if (domain) {
                Type maskType = array.getShape().size() == 1 ? b.getI1Type() :
                  Type(acir::MaskType::get(b.getContext(), array.getShape().drop_front()));
                domain = make(b, loc, "acir.extract", {domain, safe}, maskType)->getResult(0);
                if (maskType.isInteger(1)) { valid = conjunction(valid, domain); domain = {}; }
              }
            }
            type = memberType(type, step);
          }
          if (a[1].value.getType() != type) throw std::runtime_error("aggregate update payload type mismatch");
          auto updated = make(b, loc, "acir.update", inputs, rootType, op->getAttrs())->getResult(0);
          if (isa<acir::SelectedType>(originalType)) {
            auto mask = make(b, loc, "acir.array.domain", a[0].value,
              acir::MaskType::get(b.getContext(), cast<acir::ArrayType>(rootType).getShape()))->getResult(0);
            updated = make(b, loc, "acir.array.select", {updated, mask}, originalType)->getResult(0);
          }
          result = {updated, valid};
        } else if (name == "acir.aggregate" &&
                   isa<acir::ArrayType>(op->getResult(0).getType()) && !a.empty() && !a[0].resource.empty()) {
          auto key = "array." + std::to_string(arrays.size());
          for (auto arg : a) {
            if (arrays.count(arg.resource)) {
              auto &nested = arrays.at(arg.resource);
              arrays[key].insert(arrays[key].end(), nested.begin(), nested.end());
            } else arrays[key].push_back(arg.resource);
          }
          result = {no, allReady, key};
        } else if (name == "acir.guard") {
          require(current, conjunction(a[0].ready, a[0].value));
          continue;
        } else if (name == "acir.check") {
          if (active >= 0)
            effect("", "check", a[0].value, current, allReady);
          else {
            auto checked =
                make(b, loc, "acir.assert", a[0].value, b.getI1Type())
                    ->getResult(0);
            checkReady = conjunction(
                checkReady, disjunction(negate(current.value),
                                        conjunction(allReady, checked)));
          }
          continue;
        } else if (name == "acir.unreachable") {
          effect("", "check", no, current, yes);
          continue;
        } else if (name == "acir.queue.peek" || name == "acir.cell.read" ||
                   name == "acir.ehr.read" || name == "acir.signal.get" ||
                   name == "acir.queue.empty" || name == "acir.queue.full" ||
                   name == "acir.queue.size") {
          auto method = name == "acir.queue.peek"    ? "peek"
                        : name == "acir.cell.read"   ? "read"
                        : name == "acir.ehr.read"    ? "ehr.read"
                        : name == "acir.signal.get"  ? "get"
                        : name == "acir.queue.empty" ? "empty"
                        : name == "acir.queue.full"  ? "full"
                                                     : "size";
          result =
              readReference(a[0], method, op->getResult(0).getType(), current);
          if (name == "acir.queue.peek" && text(op, "role") == "consume")
            effectReference(a[0], "pop", {}, current, result.ready);
        } else if (name == "acir.queue.push" || name == "acir.cell.write" ||
                   name == "acir.ehr.write" || name == "acir.signal.set" ||
                   name == "acir.queue.pop") {
          auto method = name == "acir.queue.push"   ? "push"
                        : name == "acir.cell.write" ? "write"
                        : name == "acir.ehr.write"  ? "ehr.write"
                        : name == "acir.signal.set" ? "set"
                                                    : "pop";
          effectReference(a[0], method, a.size() > 1 ? a[1].value : Value(),
                          current, allReady);
          continue;
        } else {
          SmallVector<Value> operands;
          for (auto arg : a) {
            if (!arg.resource.empty())
              throw std::runtime_error(
                  "component reference in ordinary computation");
            operands.push_back(arg.value);
          }
          auto *copy = make(b, loc, name, operands, op->getResultTypes(),
                            op->getAttrs());
          result = {op->getNumResults() ? copy->getResult(0) : no, allReady};
          for (auto arg : a)
            if (arg.producer >= 0)
              result.producer = arg.producer;
          if (name == "acir.export") {
            auto owner = op->getParentOfType<acir::BodyOp>()
                             ->getAttrOfType<DictionaryAttr>("acir.function")
                             .getAs<StringAttr>("name")
                             .getValue();
            exports[{owner.str(), text(op, "acir.id")}] = result;
          }
        }
        if (op->getNumResults())
          env[op->getResult(0)] = result;
      }
    }
    Type resultType =
        returns.empty() ? b.getI1Type() : returns[0].second.value.getType();
    auto result = merge(returns, resultType);
    result.ready = conjunction(result.ready, checkReady);
    return result;
  }
  Pair callRule(Operation *rule, ArrayRef<Pair> args, Pair path) {
    auto meta = rule->getAttrOfType<DictionaryAttr>("acir.function");
    auto key = meta.getAs<StringAttr>("instance").getValue().str() + ":" +
               meta.getAs<StringAttr>("rule").getValue().str();
    frames.push_back(key);
    auto attrs = metadata("Module contract", "callsite", active);
    attrs.push_back(b.getNamedAttr(
        "branch", b.getI64IntegerAttr(branch(path.value, active))));
    auto *site = make(b, loc, "acir.path", path.value, {}, attrs);
    callSites.push_back(site);
    auto ready = yes;
    if (!rule->getRegion(0).empty()) {
      bool invalid = false;
      rule->getRegion(0).walk([&](acir::RuleCallOp call) {
        if (modifying(symbols.at(
                call.getTarget().getLeafReference().getValue().str())))
          invalid = true;
      });
      rule->getRegion(0).walk([&](func::CallOp call) {
        auto *callee = symbols.at(call.getCallee().str());
        if (!cast<func::FuncOp>(callee).isDeclaration() && modifying(callee))
          invalid = true;
      });
      if (invalid)
        throw std::runtime_error("guard calls a modifying Rule: " + key);
      auto guard = run(rule->getRegion(0), args, path);
      ready = conjunction(guard.ready, guard.value);
      require(path, ready);
    }
    auto result = run(rule->getRegion(1), args,
                      {path.value, conjunction(path.ready, ready)});
    result.ready = conjunction(result.ready, ready);
    frames.pop_back();
    return result;
  }
  std::set<Operation *> doneBodies;
  void runBody(StringRef name, Pair path) {
    for (auto scope : source.getOps<acir::ModuleOp>())
      for (auto body : scope.getDeclarations().getOps<acir::BodyOp>())
        if (body->getAttrOfType<DictionaryAttr>("acir.function")
                .getAs<StringAttr>("name")
                .getValue() == name) {
          if (!doneBodies.insert(body).second)
            throw std::runtime_error("repeated or recursive Module call: " + name.str());
          run(body.getBody(), {}, path);
          return;
        }
    throw std::runtime_error("shared body not found");
  }
  mlir::ModuleOp extract() {
    // Initializers are pure standard functions and evaluated from MLIR.
    auto initialModule = cast<mlir::ModuleOp>(source->clone());
    SmallVector<Operation *> nested;
    initialModule.walk([&](func::FuncOp fn) {
      if (fn->getParentOp() != initialModule)
        nested.push_back(fn);
    });
    for (auto *fn : nested) {
      auto name = text(fn, "sym_name");
      if (initialModule.lookupSymbol(name)) {
        fn->erase();
        continue;
      }
      fn->moveBefore(initialModule.getBody(), initialModule.getBody()->end());
    }
    SmallVector<Attribute> config;
    auto model = source->getAttrOfType<DictionaryAttr>("acir.model");
    auto cfg = source->getAttrOfType<DictionaryAttr>("acir.config");
    for (auto c : model.getAs<ArrayAttr>("config")) {
      auto d = cast<DictionaryAttr>(c);
      auto name = d.getAs<StringAttr>("name").getValue();
      auto value = cfg ? cfg.get(name) : Attribute();
      if (!value)
        throw std::runtime_error("missing constructor configuration: " +
                                 name.str());
      config.push_back(value);
    }
    for (auto &[name, op] : symbols) {
      if (!op->hasAttr("payload_type"))
        continue;
      auto kind = op->getName().getStringRef().drop_front(5);
      auto type = op->getAttrOfType<TypeAttr>("payload_type").getValue();
      Attribute initial;
      if (kind == "signal")
        initial = acir::zero(type, source);
      else
        initial = acir::evaluateFunction(initialModule, text(op, "initial_fn"),
                                         b.getArrayAttr(config));
      SmallVector<NamedAttribute> attrs{
          b.getNamedAttr("sym_name", b.getStringAttr(name)),
          b.getNamedAttr("payload_type", TypeAttr::get(type)),
          b.getNamedAttr("kind", b.getStringAttr(kind)),
          b.getNamedAttr("initial", initial)};
      if (kind == "queue") {
        auto cap = acir::evaluateFunction(
            initialModule, text(op, "capacity_fn"), b.getArrayAttr(config));
        auto n = cast<IntegerAttr>(cap).getInt();
        if (n < 1 || cast<ArrayAttr>(initial).size() > (uint64_t)n)
          throw std::runtime_error("invalid Queue capacity/initial contents: " +
                                   name);
        attrs.push_back(b.getNamedAttr("capacity", b.getI64IntegerAttr(n)));
      }
      auto *state = make(b, op->getLoc(), "acir.state", {}, {}, attrs);
      states[name] = state;
    }
    initialModule.erase();
    for (auto scope : source.getOps<acir::ModuleOp>())
      if (scope.getSymName() == "top")
        for (auto body : scope.getDeclarations().getOps<acir::BodyOp>())
          runBody(body->getAttrOfType<DictionaryAttr>("acir.function")
                      .getAs<StringAttr>("name").getValue(), {yes, yes});
    schedule();
    return target;
  }
  std::string relation(Operation *a, Operation *z) {
    for (auto f : a->getAttrOfType<ArrayAttr>("frames"))
      for (auto g : z->getAttrOfType<ArrayAttr>("frames")) {
        auto key = std::make_pair(cast<StringAttr>(f).getValue().str(),
                                  cast<StringAttr>(g).getValue().str());
        auto i = relations.find(key);
        if (i != relations.end() && i->second != "CF")
          return i->second;
      }
    return "";
  }
  void schedule() {
    // EHR phases are constrained only by declared order, independently of
    // transaction order.
    for (auto &[res, ops] : accesses)
      if (text(states.at(res), "kind") == "ehr") {
        std::map<std::string, int> ports;
        std::map<Operation *, std::string> keys;
        for (auto *op : ops) {
          auto context = op->getAttrOfType<ArrayAttr>("frames");
          auto key = context.empty()
                         ? "shared:" + res
                         : cast<StringAttr>(context[context.size() - 1])
                               .getValue()
                               .str();
          keys[op] = key;
          ports[key] = 0;
        }
        for (unsigned pass = 0; pass < ports.size(); ++pass)
          for (auto *a : ops)
            for (auto *z : ops)
              if (keys[a] != keys[z] &&
                  (relation(a, z) == "SB" || relation(a, z) == "SBR"))
                ports[keys[z]] = std::max(ports[keys[z]], ports[keys[a]] + 1);
        for (auto &[key, p] : ports)
          if (p >= (int)ports.size())
            throw std::runtime_error("unsatisfiable EHR port order: " + res +
                                     " " + key);
        for (auto *op : ops)
          op->setAttr("port", b.getI64IntegerAttr(ports[keys[op]]));
        // An incomparable writer must not become visible through rank
        // assignment.
        for (auto *a : ops)
          for (auto *z : ops)
            if (text(a, "method") == "ehr.write" &&
                a->getAttrOfType<IntegerAttr>("port").getInt() <
                    z->getAttrOfType<IntegerAttr>("port").getInt() &&
                relation(a, z) != "SB" && relation(a, z) != "SBR")
              throw std::runtime_error(
                  "EHR port would introduce undeclared visibility: " + res +
                  " " + keys[a] + " -> " + keys[z]);
      }
    std::vector<std::set<int>> successors(txns.size());
    SmallVector<Attribute> edges, conflicts;
    auto edge = [&](int a, int z, std::string res, std::string kind,
                    Operation *from = nullptr, Operation *to = nullptr) {
      if (a < 0 || z < 0 || a == z)
        return;
      successors[a].insert(z);
      edges.push_back(b.getDictionaryAttr(
          {b.getNamedAttr("from", b.getI64IntegerAttr(a)),
           b.getNamedAttr("to", b.getI64IntegerAttr(z)),
           b.getNamedAttr("resource", b.getStringAttr(res)),
           b.getNamedAttr("relation", b.getStringAttr(kind)),
           b.getNamedAttr("source_branch", from ? from->getAttr("branch")
                                                : b.getI64IntegerAttr(-1)),
           b.getNamedAttr("target_branches",
                          b.getArrayAttr({to ? to->getAttr("branch")
                                             : b.getI64IntegerAttr(-1)}))}));
    };
    auto users = [&](Operation *op) {
      std::set<int> result;
      auto id = op->getAttrOfType<IntegerAttr>("txn").getInt();
      if (id >= 0)
        result.insert(id);
      else {
        llvm::SmallPtrSet<Operation *, 32> seen;
        std::vector<Operation *> pending{op};
        while (!pending.empty()) {
          auto *v = pending.back();
          pending.pop_back();
          if (!seen.insert(v).second)
            continue;
          if (auto fire = dyn_cast<acir::FireOp>(v))
            result.insert(fire->getAttrOfType<IntegerAttr>("txn").getInt());
          for (auto *user : v->getUsers())
            pending.push_back(user);
        }
      }
      return result;
    };
    auto mutuallyExclusive = [&](Operation *a, Operation *z) {
      // Prove path disjointness through the actual boolean SSA graph. Unknown
      // comparisons remain independent atoms, so a failed proof is
      // conservative.
      DenseMap<Value, unsigned> atoms;
      std::function<void(Value)> collect = [&](Value v) {
        auto *op = v.getDefiningOp();
        auto n = op->getName().getStringRef();
        if (n == "arith.constant")
          return;
        if (n == "acir.fire" || n == "arith.andi" || n == "arith.ori" ||
            n == "arith.xori" || n == "arith.select" ||
            (n == "acir.unary" && text(op, "operator_name") == "not")) {
          for (auto x : op->getOperands())
            if (x.getType().isInteger(1))
              collect(x);
          return;
        }
        if (!atoms.count(v))
          atoms[v] = atoms.size();
      };
      auto left = a->getOperand(0), right = z->getOperand(0);
      collect(left);
      collect(right);
      if (atoms.size() > 18)
        return false;
      std::function<bool(Value, uint64_t)> eval = [&](Value v, uint64_t bits) {
        auto *op = v.getDefiningOp();
        auto n = op->getName().getStringRef();
        if (n == "arith.constant")
          return !op->getAttrOfType<IntegerAttr>("value").getValue().isZero();
        if (n == "acir.fire")
          return eval(op->getOperand(0), bits);
        if (n == "arith.andi")
          return eval(op->getOperand(0), bits) && eval(op->getOperand(1), bits);
        if (n == "arith.ori")
          return eval(op->getOperand(0), bits) || eval(op->getOperand(1), bits);
        if (n == "arith.xori")
          return eval(op->getOperand(0), bits) != eval(op->getOperand(1), bits);
        if (n == "arith.select")
          return eval(op->getOperand(eval(op->getOperand(0), bits) ? 1 : 2),
                      bits);
        if (n == "acir.unary" && text(op, "operator_name") == "not")
          return !eval(op->getOperand(0), bits);
        return bool((bits >> atoms[v]) & 1);
      };
      for (uint64_t bits = 0; bits < (1ULL << atoms.size()); ++bits)
        if (eval(left, bits) && eval(right, bits))
          return false;
      return true;
    };
    for (auto &[res, ops] : accesses)
      for (auto *a : ops)
        for (auto *z : ops)
          if (a != z) {
            auto x = text(a, "method"), y = text(z, "method"),
                 kind = relation(a, z);
            auto component = text(states.at(res), "kind");
            bool ordered = kind == "SB" || kind == "SBR",
                 strict = kind == "SBR", conflict = kind == "C";
            if (component == "queue" && x == "peek" && y == "pop") {
              ordered = true;
              kind = "Queue.peek → pop";
            }
            if (component == "queue" && x == "pop" &&
                (y == "full" || y == "push")) {
              ordered = true;
              strict = true;
              kind = "Queue.pop → push";
            }
            if (component == "signal" && x == "set" && y == "get") {
              ordered = true;
              strict = true;
              kind = "Signal.set → get";
            }
            if (component == "signal" && x == "set" && y == "set")
              conflict = true;
            if (component == "ehr") {
              auto p = a->getAttrOfType<IntegerAttr>("port").getInt(),
                   q = z->getAttrOfType<IntegerAttr>("port").getInt();
              if (p < q) {
                ordered = true;
                strict = true;
                kind = "EHR.port";
              }
              if (p == q && x == "ehr.read" && y == "ehr.write") {
                ordered = true;
                kind = "EHR.read → write";
              }
              if (p == q && x == "ehr.write" && y == "ehr.write")
                conflict = true;
            }
            for (auto ta : users(a))
              for (auto tz : users(z)) {
                if (ta == tz && (strict || conflict) &&
                    !mutuallyExclusive(
                        a, z)) { // Same-transaction exclusions require a path
                                 // satisfiability proof.
                  if (component == "queue" && x == "peek" && y == "pop") {
                    ordered = true;
                    kind = "Queue.peek → pop";
                  }
                  if (component == "queue" && x == "pop" && y == "full")
                    throw std::runtime_error(
                        "same transaction uses Queue pop and capacity phase: " +
                        res);
                  if (component == "signal" || component == "ehr" ||
                      kind == "C" || kind == "SBR")
                    throw std::runtime_error(
                        "incompatible operations in transaction " +
                        txns[ta].name + " on " + res + ": " + x + " / " + y);
                }
                if (ordered)
                  edge(ta, tz, res, kind, a, z);
                if (conflict && ta != tz)
                  conflicts.push_back(b.getDictionaryAttr(
                      {b.getNamedAttr("from", b.getI64IntegerAttr(ta)),
                       b.getNamedAttr("to", b.getI64IntegerAttr(tz)),
                       b.getNamedAttr("resource", b.getStringAttr(res))}));
              }
          }
    // Rule declarations propagate through the actual nested call stack, even
    // when their bodies access separate resources or have no component effects.
    for (auto *a : callSites)
      for (auto *z : callSites)
        if (a != z) {
          auto kind = relation(a, z);
          auto ta = a->getAttrOfType<IntegerAttr>("txn").getInt(),
               tz = z->getAttrOfType<IntegerAttr>("txn").getInt();
          if (ta == tz && ta >= 0 && (kind == "C" || kind == "SBR") &&
              !mutuallyExclusive(a, z))
            throw std::runtime_error(
                "Rule contract forbids nested calls in transaction " +
                txns[ta].name);
          if (kind == "SB" || kind == "SBR")
            edge(ta, tz, "Module contract", kind, a, z);
          if (kind == "C" && ta >= 0 && tz >= 0 && ta != tz)
            conflicts.push_back(b.getDictionaryAttr(
                {b.getNamedAttr("from", b.getI64IntegerAttr(ta)),
                 b.getNamedAttr("to", b.getI64IntegerAttr(tz)),
                 b.getNamedAttr("resource",
                                b.getStringAttr("Module contract"))}));
        }
    std::vector<int> degree(txns.size());
    for (auto &ss : successors)
      for (auto z : ss)
        ++degree[z];
    SmallVector<Attribute> order;
    std::set<int> pending;
    for (unsigned i = 0; i < degree.size(); ++i)
      if (!degree[i])
        pending.insert(i);
    while (!pending.empty()) {
      int a = *pending.begin();
      pending.erase(a);
      order.push_back(b.getI64IntegerAttr(a));
      for (int z : successors[a])
        if (--degree[z] == 0)
          pending.insert(z);
    }
    if (order.size() != txns.size()) {
      std::string message =
          "branch-union transaction schedule contains a cycle:";
      for (unsigned i = 0; i < degree.size(); ++i)
        if (degree[i])
          message += " " + txns[i].name;
      throw std::runtime_error(message);
    }
    for (unsigned i = 0; i < txns.size(); ++i) {
      SmallVector<Attribute> next;
      for (auto z : successors[i])
        next.push_back(b.getI64IntegerAttr(z));
      auto *fire = txns[i].fire.getDefiningOp();
      fire->setAttr("successors", b.getArrayAttr(next));
    }
    circuit->setAttr("schedule", b.getArrayAttr(order));
    circuit->setAttr("edges", b.getArrayAttr(edges));
    circuit->setAttr("conflicts", b.getArrayAttr(conflicts));
  }
};
struct Transactions : PassWrapper<Transactions, OperationPass<mlir::ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(Transactions)
  StringRef getArgument() const final { return "acir-extract-transactions"; }
  StringRef getDescription() const final {
    return "Extract atomic transactions, conditions, component ports and "
           "schedule from source MLIR";
  }
  void runOnOperation() override {
    auto module = getOperation();
    if (!module.getOps<acir::CircuitOp>().empty())
      return;
    try {
      PassManager pm(module.getContext());
      pm.addPass(createSCFToControlFlowPass());
      if (failed(pm.run(module))) {
        signalPassFailure();
        return;
      }
      Extractor ex(module);
      auto result = ex.extract();
      module.getBodyRegion().takeBody(result.getBodyRegion());
      module->setAttrs(result->getAttrs());
      result.erase();
    } catch (const std::exception &e) {
      module.emitError(e.what());
      signalPassFailure();
    }
  }
};

} // namespace
std::unique_ptr<Pass> acir::createTransactionPass() {
  return std::make_unique<Transactions>();
}

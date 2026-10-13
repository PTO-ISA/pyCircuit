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
struct RTLBuilder {
  mlir::ModuleOp source, target;
  acir::CircuitOp input, output;
  OpBuilder b;
  Location loc;
  DenseMap<Value, Value> values;
  llvm::SmallPtrSet<Operation *, 32> busy;
  std::map<std::string, Operation *> states;
  std::map<std::pair<std::string, std::string>, Value> currents;
  Value yes, no;
  RTLBuilder(mlir::ModuleOp module)
      : source(module), target(mlir::ModuleOp::create(module.getLoc())),
        input(*module.getOps<acir::CircuitOp>().begin()),
        b(module.getContext()), loc(module.getLoc()) {
    target->setAttrs(source->getAttrs());
    target->setAttr("acir.stage", b.getStringAttr("rtl"));
    b.setInsertionPointToEnd(target.getBody());
    OperationState st(loc, "acir.circuit");
    st.addAttributes(input->getAttrs());
    st.addRegion();
    output = cast<acir::CircuitOp>(b.create(st));
    output.getBody().push_back(new Block());
    output->setAttr("stage", b.getStringAttr("rtl"));
    b.setInsertionPointToEnd(&output.getBody().front());
    for (auto state : input.getBody().getOps<acir::StateOp>()) {
      if (text(state, "kind") == "signal")
        continue;
      auto *copy = b.clone(*state);
      states[state.getSymName().str()] = copy;
    }
    yes = constant(1, 1);
    no = constant(0, 1);
  }
  Value constant(uint64_t n, unsigned w = 32) {
    return arith::ConstantIntOp::create(b, loc, n, w);
  }
  Value and_(Value a, Value z) { return arith::AndIOp::create(b, loc, a, z); }
  Value or_(Value a, Value z) { return arith::OrIOp::create(b, loc, a, z); }
  Value select(Value c, Value a, Value z) {
    return arith::SelectOp::create(b, loc, c, a, z);
  }
  Value cmp(arith::CmpIPredicate p, Value a, Value z) {
    return arith::CmpIOp::create(b, loc, p, a, z);
  }
  Value current(std::string res, std::string aspect = "value") {
    auto key = std::make_pair(res, aspect);
    if (currents.count(key))
      return currents[key];
    auto *state = states.at(res);
    auto type = state->getAttrOfType<TypeAttr>("payload_type").getValue();
    if (aspect == "count")
      type = b.getI32Type();
    if (aspect == "storage")
      type = acir::ArrayType::get(
          b.getContext(), type,
          state->getAttrOfType<IntegerAttr>("capacity").getInt());
    return currents[key] =
               make(b, loc, "acir.current", {}, type,
                    {b.getNamedAttr("resource", b.getStringAttr(res)),
                     b.getNamedAttr("aspect", b.getStringAttr(aspect))})
                   ->getResult(0);
  }
  std::vector<Operation *> effects(std::string res, std::string method) {
    std::vector<Operation *> result;
    for (auto op : input.getBody().getOps<acir::EffectOp>())
      if (text(op, "resource") == res && text(op, "method") == method)
        result.push_back(op);
    auto schedule = input->getAttrOfType<ArrayAttr>("schedule");
    std::map<int, int> rank;
    for (auto [i, a] : llvm::enumerate(schedule))
      rank[cast<IntegerAttr>(a).getInt()] = i;
    std::stable_sort(
        result.begin(), result.end(), [&](Operation *a, Operation *z) {
          if (method == "ehr.write")
            return a->getAttrOfType<IntegerAttr>("port").getInt() <
                   z->getAttrOfType<IntegerAttr>("port").getInt();
          return rank[a->getAttrOfType<IntegerAttr>("txn").getInt()] <
                 rank[z->getAttrOfType<IntegerAttr>("txn").getInt()];
        });
    return result;
  }
  Value popEnabled(std::string res) {
    std::map<int, Value> txns;
    for (auto *op : effects(res, "pop")) {
      auto id = op->getAttrOfType<IntegerAttr>("txn").getInt();
      auto enable = map(op->getOperand(0));
      txns[id] = txns.count(id) ? or_(txns[id], enable) : enable;
    }
    auto count = constant(0);
    for (auto &[id, v] : txns)
      count = arith::AddIOp::create(b, loc, count,
                                    select(v, constant(1), constant(0)));
    return count;
  }
  Value map(Value v) {
    if (values.count(v))
      return values[v];
    auto *op = v.getDefiningOp();
    loc = op->getLoc();
    if (!busy.insert(op).second)
      throw std::runtime_error("RTL combinational dependency cycle");
    if (auto read = dyn_cast<acir::ReadOp>(op)) {
      auto res = text(op, "resource"), method = text(op, "method");
      Value data, ready = yes;
      if (method == "read")
        data = current(res);
      else if (method == "ehr.read") {
        data = current(res);
        int port = op->getAttrOfType<IntegerAttr>("port").getInt();
        for (auto *w : effects(res, "ehr.write"))
          if (w->getAttrOfType<IntegerAttr>("port").getInt() < port)
            data = select(map(w->getOperand(0)), map(w->getOperand(1)), data);
      } else if (method == "get") {
        data = make(b, loc, "acir.aggregate", {}, read.getValue().getType())
                   ->getResult(0);
        ready = no;
        for (auto *w : effects(res, "set")) {
          if (w->getAttrOfType<IntegerAttr>("txn") ==
              op->getAttrOfType<IntegerAttr>("txn"))
            continue;
          auto en = map(w->getOperand(0));
          data = select(en, map(w->getOperand(1)), data);
          ready = or_(ready, en);
        }
      } else if (method == "peek") {
        auto storage = current(res, "storage");
        auto index = constant(0);
        data = make(b, loc, "acir.extract", {storage, index},
                    read.getValue().getType())
                   ->getResult(0);
        ready =
            cmp(arith::CmpIPredicate::ne, current(res, "count"), constant(0));
      } else if (method == "empty")
        data =
            cmp(arith::CmpIPredicate::eq, current(res, "count"), constant(0));
      else if (method == "size")
        data = current(res, "count");
      else if (method == "full") {
        auto free = arith::SubIOp::create(b, loc, current(res, "count"),
                                          popEnabled(res));
        data = cmp(
            arith::CmpIPredicate::uge, free,
            constant(
                states[res]->getAttrOfType<IntegerAttr>("capacity").getInt()));
      } else
        throw std::runtime_error("unknown transaction read: " + method);
      values[read.getValue()] = data;
      values[read.getReady()] = ready;
    } else {
      SmallVector<Value> a;
      for (auto x : op->getOperands())
        a.push_back(map(x));
      auto *copy = make(b, loc, op->getName().getStringRef(), a,
                        op->getResultTypes(), op->getAttrs());
      for (auto [x, z] : llvm::zip(op->getResults(), copy->getResults()))
        values[x] = z;
    }
    busy.erase(op);
    return values.at(v);
  }
  void next(std::string res, std::string aspect, Value value) {
    make(b, loc, "acir.next", value, {},
         {b.getNamedAttr("resource", b.getStringAttr(res)),
          b.getNamedAttr("aspect", b.getStringAttr(aspect))});
  }
  mlir::ModuleOp lower() {
    for (auto fire : input.getBody().getOps<acir::FireOp>())
      map(fire.getFire());
    for (auto &[res, state] : states) {
      auto kind = text(state, "kind");
      if (kind == "signal")
        continue;
      if (kind == "queue") {
        int capacity = state->getAttrOfType<IntegerAttr>("capacity").getInt();
        auto type = state->getAttrOfType<TypeAttr>("payload_type").getValue();
        auto old = current(res, "storage"), pops = popEnabled(res);
        Value storage = old,
              count =
                  arith::SubIOp::create(b, loc, current(res, "count"), pops);
        // Fixed storage shift by the number of granted consumers, padded with
        // zero.
        SmallVector<Value> elements;
        for (int i = 0; i < capacity; ++i) {
          Value data = make(b, loc, "acir.aggregate", {}, type)->getResult(0);
          for (int j = capacity - 1; j >= i; --j) {
            auto value = make(b, loc, "acir.extract", {old, constant(j)}, type)
                             ->getResult(0);
            data = select(cmp(arith::CmpIPredicate::eq, pops, constant(j - i)),
                          value, data);
          }
          elements.push_back(data);
        }
        storage = make(b, loc, "acir.aggregate", elements, old.getType())
                      ->getResult(0);
        for (auto *push : effects(res, "push")) {
          auto enable = map(push->getOperand(0));
          auto value = map(push->getOperand(1));
          auto safeIndex = select(enable, count, constant(0));
          auto updated =
              make(b, loc, "acir.update", {storage, value, safeIndex},
                   storage.getType(),
                   {b.getNamedAttr("path", b.getArrayAttr({b.getUnitAttr()}))})
                  ->getResult(0);
          storage = select(enable, updated, storage);
          count = arith::AddIOp::create(
              b, loc, count, select(enable, constant(1), constant(0)));
        }
        next(res, "storage", storage);
        next(res, "count", count);
      } else {
        auto value = current(res);
        for (auto *write : effects(res, kind == "ehr" ? "ehr.write" : "write"))
          value = select(map(write->getOperand(0)), map(write->getOperand(1)),
                         value);
        next(res, "value", value);
      }
    }
    for (auto op : input.getBody().getOps<acir::EffectOp>())
      if (text(op, "method") == "check") {
        SmallVector<Value> a;
        for (auto v : op->getOperands())
          a.push_back(map(v));
        make(b, op.getLoc(), "acir.effect", a, {}, op->getAttrs());
      }
    return target;
  }
};
struct RTL : PassWrapper<RTL, OperationPass<mlir::ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(RTL)
  StringRef getArgument() const final { return "acir-lower-rtl"; }
  StringRef getDescription() const final {
    return "Lower resolved transactions to combinational logic and explicit "
           "next state";
  }
  void runOnOperation() override {
    auto module = getOperation();
    try {
      RTLBuilder lower(module);
      auto result = lower.lower();
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
std::unique_ptr<Pass> acir::createRTLPass() { return std::make_unique<RTL>(); }

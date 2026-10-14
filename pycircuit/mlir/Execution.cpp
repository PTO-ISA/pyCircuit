#include "Execution.h"
#include "Dialect.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/StringMap.h"
#include <functional>
#include <set>
#include <stdexcept>
using namespace mlir;
namespace {
std::string str(Operation *op, StringRef name) {
  auto a = op->getAttrOfType<StringAttr>(name);
  return a ? a.getValue().str() : "";
}
uint64_t number(Attribute a) {
  return cast<IntegerAttr>(a).getValue().getZExtValue();
}
bool truth(Attribute a) { return number(a) != 0; }
ArrayAttr fields(Type type, mlir::ModuleOp module) {
  if (auto s = dyn_cast<acir::StructType>(type))
    return module->getAttrOfType<DictionaryAttr>("acir.model")
        .getAs<DictionaryAttr>("types")
        .getAs<ArrayAttr>(s.getName());
  return {};
}
unsigned fieldIndex(Type type, StringRef field, mlir::ModuleOp module) {
  auto fs = fields(type, module);
  if (!fs)
    throw std::runtime_error("field on nonstruct");
  for (auto [i, f] : llvm::enumerate(fs))
    if (cast<DictionaryAttr>(f).getAs<StringAttr>("name").getValue() == field)
      return i;
  throw std::runtime_error("unknown field");
}
Type fieldType(Type type, unsigned index, mlir::ModuleOp module) {
  if (auto a = dyn_cast<acir::ArrayType>(type))
    return a.getShape().size() == 1 ? a.getElementType() :
      acir::ArrayType::get(type.getContext(), a.getElementType(), a.getShape().drop_front());
  if (auto a = dyn_cast<acir::MaskType>(type))
    return a.getShape().size() == 1 ? IntegerType::get(type.getContext(), 1) :
      Type(acir::MaskType::get(type.getContext(), a.getShape().drop_front()));
  if (auto t = dyn_cast<TupleType>(type))
    return t.getType(index);
  auto f = cast<DictionaryAttr>(fields(type, module)[index]);
  if (auto payload = f.getAs<TypeAttr>("payload_type")) return payload.getValue();
  auto s = f.getAs<StringAttr>("type").getValue();
  if (s == "bool")
    return IntegerType::get(module.getContext(), 1);
  if (s.starts_with("u") || s.starts_with("i")) {
    unsigned width;
    if (!s.drop_front().getAsInteger(10, width))
      return IntegerType::get(module.getContext(), width);
  }
  return acir::StructType::get(module.getContext(), s);
}
} // namespace
Attribute acir::zero(Type type, mlir::ModuleOp module) {
  auto *ctx = module.getContext();
  if (auto i = dyn_cast<IntegerType>(type))
    return IntegerAttr::get(i, 0);
  SmallVector<Attribute> values;
  if (auto a = dyn_cast<ArrayType>(type))
    values.assign(a.getShape().front(), zero(fieldType(type, 0, module), module));
  else if (auto a = dyn_cast<SelectedType>(type)) {
    auto array = cast<ArrayType>(a.getElementType());
    values.push_back(zero(array, module));
    values.push_back(zero(MaskType::get(ctx, array.getShape()), module));
  }
  else if (auto a = dyn_cast<MaskType>(type))
    values.assign(a.getShape().front(), zero(fieldType(type, 0, module), module));
  else if (auto t = dyn_cast<TupleType>(type))
    for (auto v : t.getTypes())
      values.push_back(zero(v, module));
  else if (auto f = fields(type, module))
    for (unsigned i = 0; i < f.size(); ++i)
      values.push_back(zero(fieldType(type, i, module), module));
  return ArrayAttr::get(ctx, values);
}
Attribute acir::calculate(Operation *op, ArrayRef<Attribute> a,
                          mlir::ModuleOp module) {
  Builder b(module.getContext());
  auto name = op->getName().getStringRef();
  auto type = op->getNumResults() ? op->getResult(0).getType() : Type();
  auto integer = [&](uint64_t v) -> Attribute {
    return IntegerAttr::get(type, v);
  };
  if (name == "arith.constant")
    return op->getAttr("value");
  if (name == "arith.select")
    return truth(a[0]) ? a[1] : a[2];
  if (name == "acir.export" || name == "acir.fire")
    return a[0];
  if (name == "acir.aggregate")
    return a.empty() ? zero(type, module) : b.getArrayAttr(a);
  if (name == "acir.length")
    return integer(cast<ArrayAttr>(a[0]).size());
  if (name == "acir.array.domain") {
    if (isa<acir::SelectedType>(op->getOperand(0).getType())) return cast<ArrayAttr>(a[0])[1];
    auto shape = cast<acir::MaskType>(type).getShape();
    std::function<Attribute(ArrayRef<int64_t>)> full = [&](ArrayRef<int64_t> dims) -> Attribute {
      SmallVector<Attribute> bits;
      for (int64_t i = 0; i < dims.front(); ++i)
        bits.push_back(dims.size() == 1 ? b.getBoolAttr(true) : full(dims.drop_front()));
      return b.getArrayAttr(bits);
    };
    return full(shape);
  }
  if (name == "acir.array.select") {
    Attribute data = a[0], mask = a[1];
    if (isa<acir::SelectedType>(op->getOperand(0).getType())) {
      auto selected = cast<ArrayAttr>(data); data = selected[0];
      std::function<Attribute(Attribute, Attribute)> intersect = [&](Attribute x, Attribute y) -> Attribute {
        if (isa<IntegerAttr>(x)) return b.getBoolAttr(truth(x) && truth(y));
        SmallVector<Attribute> result;
        for (auto [l, r] : llvm::zip(cast<ArrayAttr>(x), cast<ArrayAttr>(y))) result.push_back(intersect(l, r));
        return b.getArrayAttr(result);
      };
      mask = intersect(selected[1], mask);
    }
    return b.getArrayAttr({data, mask});
  }
  if (name == "acir.array.index") {
    Attribute data = a[0];
    auto index = number(a[1]);
    if (isa<acir::SelectedType>(op->getOperand(0).getType())) {
      auto selected = cast<ArrayAttr>(data);
      data = cast<ArrayAttr>(selected[0])[index];
      if (isa<acir::SelectedType>(type)) return b.getArrayAttr({data, cast<ArrayAttr>(selected[1])[index]});
      return data;
    }
    return cast<ArrayAttr>(data)[index];
  }
  if (name == "acir.extract") {
    auto array = cast<ArrayAttr>(a[0]);
    unsigned index =
        op->hasAttr("field")
            ? fieldIndex(op->getOperand(0).getType(), str(op, "field"), module)
            : number(a[1]);
    if (index >= array.size())
      throw std::runtime_error("aggregate index out of bounds");
    return array[index];
  }
  if (name == "acir.update") {
    auto path = op->getAttrOfType<ArrayAttr>("path");
    unsigned dynamic = 2;
    std::function<Attribute(Attribute, Type, unsigned)> update =
        [&](Attribute root, Type t, unsigned depth) -> Attribute {
      if (depth == path.size())
        return a[1];
      auto arr = cast<ArrayAttr>(root);
      auto step = path[depth];
      unsigned index =
          isa<UnitAttr>(step)
              ? number(a[dynamic++])
              : fieldIndex(t, cast<StringAttr>(step).getValue(), module);
      if (index >= arr.size())
        throw std::runtime_error("aggregate update out of bounds");
      SmallVector<Attribute> vals(arr.getValue());
      vals[index] = update(vals[index], fieldType(t, index, module), depth + 1);
      return b.getArrayAttr(vals);
    };
    return update(a[0], op->getOperand(0).getType(), 0);
  }
  if (name == "acir.compare") {
    auto cmp = str(op, "operator_name");
    if (cmp != "==" && cmp != "!=")
      throw std::runtime_error("aggregate comparison requires == or !=");
    return b.getBoolAttr((a[0] == a[1]) == (cmp == "=="));
  }
  if (name == "acir.cast" || name == "arith.extui" || name == "arith.trunci")
    return integer(number(a[0]));
  if (name == "arith.extsi")
    return IntegerAttr::get(type, cast<IntegerAttr>(a[0]).getValue().sext(
                                      cast<IntegerType>(type).getWidth()));
  if (name == "acir.unary") {
    auto oper = str(op, "operator_name");
    auto x = number(a[0]);
    return integer(oper == "not" ? !x
                   : oper == "~" ? ~x
                   : oper == "-" ? -x
                                 : x);
  }
  if (name == "arith.cmpi") {
    auto x = cast<IntegerAttr>(a[0]).getValue(),
         y = cast<IntegerAttr>(a[1]).getValue();
    bool r = false;
    switch (op->getAttrOfType<IntegerAttr>("predicate").getInt()) {
    case 0:
      r = x == y;
      break;
    case 1:
      r = x != y;
      break;
    case 2:
      r = x.slt(y);
      break;
    case 3:
      r = x.sle(y);
      break;
    case 4:
      r = x.sgt(y);
      break;
    case 5:
      r = x.sge(y);
      break;
    case 6:
      r = x.ult(y);
      break;
    case 7:
      r = x.ule(y);
      break;
    case 8:
      r = x.ugt(y);
      break;
    case 9:
      r = x.uge(y);
      break;
    }
    return b.getBoolAttr(r);
  }
  if (name.starts_with("arith.")) {
    auto x = cast<IntegerAttr>(a[0]).getValue(),
         y = cast<IntegerAttr>(a[1]).getValue();
    llvm::APInt r = x;
    if (name == "arith.addi")
      r = x + y;
    else if (name == "arith.subi")
      r = x - y;
    else if (name == "arith.muli")
      r = x * y;
    else if (name == "arith.andi")
      r = x & y;
    else if (name == "arith.ori")
      r = x | y;
    else if (name == "arith.xori")
      r = x ^ y;
    else if (name == "arith.shli")
      r = x.shl(y);
    else if (name == "arith.shrui")
      r = x.lshr(y);
    else if (name == "arith.shrsi")
      r = x.ashr(y);
    else if (name == "arith.divui" || name == "arith.divsi" ||
             name == "arith.floordivsi") {
      if (y.isZero())
        throw std::runtime_error("division by zero");
      r = name == "arith.divui" ? x.udiv(y) : x.sdiv(y);
      if (name == "arith.floordivsi" && !x.srem(y).isZero() &&
          x.isNegative() != y.isNegative())
        r -= 1;
    } else if (name == "arith.remui" || name == "arith.remsi") {
      if (y.isZero())
        throw std::runtime_error("division by zero");
      r = name == "arith.remui" ? x.urem(y) : x.srem(y);
    } else
      throw std::runtime_error("unsupported arithmetic: " + name.str());
    return IntegerAttr::get(type, r);
  }
  if (name == "acir.assert") {
    if (!truth(a[0]))
      throw std::runtime_error("model assertion failed in shared computation");
    return b.getBoolAttr(true);
  }
  if (name == "acir.check") {
    if (!truth(a[0]))
      throw std::runtime_error("model assertion failed");
    return {};
  }
  if (name == "acir.unreachable")
    throw std::runtime_error("missing return");
  throw std::runtime_error("unsupported computation: " + name.str());
}
Attribute acir::evaluateFunction(mlir::ModuleOp module, StringRef name,
                                 ArrayAttr args) {
  auto fn = module.lookupSymbol<func::FuncOp>(name);
  if (!fn)
    throw std::runtime_error("initializer not found: " + name.str());
  auto meta = fn->getAttrOfType<DictionaryAttr>("acir.function");
  if (fn.isDeclaration()) {
    auto intrinsic = meta.getAs<StringAttr>("intrinsic").getValue();
    auto x = cast<IntegerAttr>(args[0]).getValue(),
         y = cast<IntegerAttr>(args[1]).getValue();
    bool sign = meta.getAs<StringAttr>("result").getValue().starts_with("i");
    if ((intrinsic == "div" || intrinsic == "mod") && y.isZero())
      throw std::runtime_error("division by zero");
    auto r = intrinsic == "shl"   ? x.shl(y)
             : intrinsic == "shr" ? (sign ? x.ashr(y) : x.lshr(y))
             : intrinsic == "div" ? (sign ? x.sdiv(y) : x.udiv(y))
                                  : (sign ? x.srem(y) : x.urem(y));
    if (intrinsic == "div" && sign && !x.srem(y).isZero() &&
        x.isNegative() != y.isNegative())
      r -= 1;
    return IntegerAttr::get(fn.getResultTypes()[0], r);
  }
  DenseMap<Value, Attribute> env;
  Block *block = &fn.getBody().front();
  SmallVector<Attribute> incoming(args.getValue());
  unsigned steps = 0;
  while (block) {
    if (++steps > 100000)
      throw std::runtime_error("initializer loop limit exceeded");
    for (auto [arg, value] : llvm::zip(block->getArguments(), incoming))
      env[arg] = value;
    auto *current = block;
    block = nullptr;
    for (auto &op : *current) {
      SmallVector<Attribute> a;
      for (auto v : op.getOperands())
        a.push_back(env.lookup(v));
      if (auto ret = dyn_cast<func::ReturnOp>(op))
        return a.empty() ? Attribute() : a[0];
      if (auto br = dyn_cast<cf::BranchOp>(op)) {
        block = br.getDest();
        incoming = a;
        break;
      }
      if (auto br = dyn_cast<cf::CondBranchOp>(op)) {
        auto dest = truth(a[0]) ? br.getTrueDest() : br.getFalseDest();
        auto values =
            truth(a[0]) ? br.getTrueDestOperands() : br.getFalseDestOperands();
        incoming.clear();
        for (auto v : values)
          incoming.push_back(env.lookup(v));
        block = dest;
        break;
      }
      auto value =
          isa<func::CallOp>(op)
              ? evaluateFunction(module, cast<func::CallOp>(op).getCallee(),
                                 ArrayAttr::get(module.getContext(), a))
              : calculate(&op, a, module);
      if (op.getNumResults())
        env[op.getResult(0)] = value;
    }
  }
  return {};
}
llvm::json::Value acir::jsonAttribute(Attribute a) {
  if (auto i = dyn_cast_or_null<IntegerAttr>(a))
    return i.getType().isInteger(1)
               ? llvm::json::Value(!i.getValue().isZero())
               : llvm::json::Value(i.getValue().getZExtValue());
  if (auto s = dyn_cast_or_null<StringAttr>(a))
    return s.getValue().str();
  if (auto array = dyn_cast_or_null<ArrayAttr>(a)) {
    llvm::json::Array v;
    for (auto item : array)
      v.push_back(jsonAttribute(item));
    return v;
  }
  if (auto d = dyn_cast_or_null<DictionaryAttr>(a)) {
    llvm::json::Object v;
    for (auto x : d)
      v[x.getName().str()] = jsonAttribute(x.getValue());
    return v;
  }
  return nullptr;
}
llvm::json::Object acir::execute(mlir::ModuleOp module, unsigned cycles) {
  Builder b(module.getContext());
  auto circuit = *module.getOps<CircuitOp>().begin();
  std::map<std::string, Attribute> state;
  std::map<std::string, Operation *> declarations;
  for (auto s : circuit.getBody().getOps<StateOp>()) {
    auto name = s.getSymName().str();
    state[name] = s->getAttr("initial");
    declarations[name] = s;
  }
  auto schedule = circuit->getAttrOfType<ArrayAttr>("schedule");
  llvm::json::Array trace;
  for (unsigned tick = 0; tick < cycles; ++tick) {
    DenseMap<Value, Attribute> cache;
    llvm::SmallDenseSet<Value> evaluating;
    struct Write {
      std::string resource, method;
      unsigned port;
      Attribute value;
    };
    std::vector<Write> writes;
    std::map<std::string, Attribute> next = state;
    std::function<Attribute(Value)> eval = [&](Value v) -> Attribute {
      if (auto a = cache.lookup(v))
        return a;
      if (!evaluating.insert(v).second)
        throw std::runtime_error("combinational cycle while executing MLIR");
      auto *op = v.getDefiningOp();
      auto name = op->getName().getStringRef();
      Attribute result;
      if (auto read = dyn_cast<ReadOp>(op)) {
        auto res = str(op, "resource"), method = str(op, "method");
        auto *decl = declarations.at(res);
        auto old = state.at(res);
        bool ready = true;
        Attribute payload = old;
        unsigned port = op->getAttrOfType<IntegerAttr>("port").getInt();
        if (method == "get") {
          ready = false;
          payload = zero(read.getValue().getType(), module);
          for (auto &w : writes)
            if (w.resource == res && w.method == "set") {
              ready = true;
              payload = w.value;
            }
        } else if (method == "ehr.read") {
          for (auto &w : writes)
            if (w.resource == res && w.port < port)
              payload = w.value;
        } else if (method == "peek") {
          auto arr = cast<ArrayAttr>(old);
          ready = !arr.empty();
          payload = ready ? arr[0] : zero(read.getValue().getType(), module);
        } else if (method == "empty")
          payload = b.getBoolAttr(cast<ArrayAttr>(old).empty());
        else if (method == "size")
          payload = b.getI32IntegerAttr(cast<ArrayAttr>(old).size());
        else if (method == "full") {
          int count = cast<ArrayAttr>(old).size();
          for (auto &w : writes)
            if (w.resource == res && w.method == "pop")
              --count;
          payload = b.getBoolAttr(
              count >= decl->getAttrOfType<IntegerAttr>("capacity").getInt());
        }
        cache[read.getValue()] = payload;
        cache[read.getReady()] = b.getBoolAttr(ready);
        result = cache.lookup(v);
      } else if (name == "acir.current") {
        auto res = str(op, "resource"), aspect = str(op, "aspect");
        auto old = state.at(res);
        if (aspect == "count")
          result = b.getI32IntegerAttr(cast<ArrayAttr>(old).size());
        else if (aspect == "storage") {
          auto arr = cast<ArrayAttr>(old);
          SmallVector<Attribute> vals(arr.getValue());
          auto cap = declarations.at(res)
                         ->getAttrOfType<IntegerAttr>("capacity")
                         .getInt();
          while ((int64_t)vals.size() < cap)
            vals.push_back(zero(
                cast<TupleType>(v.getType()).getType(0), module));
          result = b.getArrayAttr(vals);
        } else
          result = old;
      } else if (auto select = dyn_cast<arith::SelectOp>(op))
        result =
            eval(truth(eval(select.getCondition())) ? select.getTrueValue()
                                                    : select.getFalseValue());
      else if (name == "arith.andi" && v.getType().isInteger(1))
        result = truth(eval(op->getOperand(0))) ? eval(op->getOperand(1))
                                                : b.getBoolAttr(false);
      else if (name == "arith.ori" && v.getType().isInteger(1))
        result = truth(eval(op->getOperand(0))) ? b.getBoolAttr(true)
                                                : eval(op->getOperand(1));
      else {
        SmallVector<Attribute> a;
        for (auto x : op->getOperands())
          a.push_back(eval(x));
        result = calculate(op, a, module);
      }
      evaluating.erase(v);
      cache[v] = result;
      return result;
    };
    llvm::json::Object fires;
    for (auto id : schedule) {
      auto txn = cast<IntegerAttr>(id).getInt();
      for (auto fire : circuit.getBody().getOps<FireOp>())
        if (fire->getAttrOfType<IntegerAttr>("txn").getInt() == txn)
          fires[fire.getName().str()] = truth(eval(fire.getFire()));
      std::set<std::string> popped;
      for (auto effect : circuit.getBody().getOps<EffectOp>())
        if (effect->getAttrOfType<IntegerAttr>("txn").getInt() == txn &&
            truth(eval(effect.getEnable()))) {
          auto res = str(effect, "resource"), method = str(effect, "method");
          if (method == "pop" && !popped.insert(res).second)
            continue;
          if (method == "check") {
            if (!truth(eval(effect.getInputs()[0])))
              throw std::runtime_error("model assertion failed at tick " +
                                       std::to_string(tick));
            continue;
          }
          Attribute value = effect.getInputs().empty()
                                ? Attribute()
                                : eval(effect.getInputs()[0]);
          unsigned port = effect->getAttrOfType<IntegerAttr>("port").getInt();
          writes.push_back({res, method, port, value});
        }
    }
    if (circuit->getAttrOfType<StringAttr>("stage").getValue() ==
        "transaction") {
      for (auto &[name, decl] : declarations) {
        auto kind = str(decl, "kind");
        if (kind == "signal") {
          next[name] = state[name];
          continue;
        }
        if (kind == "queue") {
          SmallVector<Attribute> vals(cast<ArrayAttr>(state[name]).getValue());
          for (auto &w : writes)
            if (w.resource == name && w.method == "pop" && !vals.empty())
              vals.erase(vals.begin());
          for (auto &w : writes)
            if (w.resource == name && w.method == "push")
              vals.push_back(w.value);
          next[name] = b.getArrayAttr(vals);
        } else {
          int highest = -1;
          for (auto &w : writes)
            if (w.resource == name &&
                (kind != "ehr" || (int)w.port >= highest)) {
              next[name] = w.value;
              highest = w.port;
            }
        }
      }
    } else {
      std::map<std::string, Attribute> storage;
      std::map<std::string, unsigned> counts;
      for (auto n : circuit.getBody().getOps<NextOp>()) {
        auto res = str(n, "resource"), aspect = str(n, "aspect");
        auto value = eval(n.getValue());
        if (aspect == "count")
          counts[res] = number(value);
        else if (aspect == "storage")
          storage[res] = value;
        else
          next[res] = value;
      }
      for (auto &[res, arr] : storage) {
        auto vals = cast<ArrayAttr>(arr);
        if (counts[res] > vals.size())
          throw std::runtime_error("Queue overflow");
        next[res] = b.getArrayAttr(vals.getValue().take_front(counts[res]));
      }
    }
    state = next;
    llvm::json::Object states;
    for (auto &[name, value] : state)
      if (str(declarations[name], "kind") != "signal")
        states[name] = jsonAttribute(value);
    trace.push_back(llvm::json::Object{{"tick", tick},
                                       {"fire", std::move(fires)},
                                       {"state", std::move(states)}});
  }
  return llvm::json::Object{{"trace", std::move(trace)}};
}

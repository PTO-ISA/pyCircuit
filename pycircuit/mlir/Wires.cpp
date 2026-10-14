// Expand aggregate data and logical component state only at the RTL boundary.
#include "Dialect.h"
#include "Execution.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include <map>
#include <stdexcept>
using namespace mlir;
namespace {
std::string text(Operation *op, StringRef key) {
  auto a = op->getAttrOfType<StringAttr>(key);
  return a ? a.getValue().str() : "";
}
struct Wires {
  mlir::ModuleOp source, target;
  acir::CircuitOp input, output;
  OpBuilder b;
  Location loc;
  DenseMap<Value, Value> values;
  std::map<std::pair<std::string, std::string>, Type> aspects;
  std::map<std::string, Value> currents;
  Wires(mlir::ModuleOp source)
      : source(source), target(mlir::ModuleOp::create(source.getLoc())),
        input(*source.getOps<acir::CircuitOp>().begin()), b(source.getContext()), loc(source.getLoc()) {
    target->setAttrs(source->getAttrs());
    b.setInsertionPointToEnd(target.getBody());
    OperationState st(loc, "acir.circuit"); st.addAttributes(input->getAttrs()); st.addRegion();
    output = cast<acir::CircuitOp>(b.create(st)); output.getBody().push_back(new Block());
    b.setInsertionPointToEnd(&output.getBody().front());
  }
  Operation *make(StringRef name, ValueRange args = {}, TypeRange types = {}, ArrayRef<NamedAttribute> attrs = {}) {
    OperationState st(loc, name); st.addOperands(args); st.addTypes(types); st.addAttributes(attrs);
    return b.create(st);
  }
  Value constant(uint64_t value, unsigned width = 32) { return arith::ConstantIntOp::create(b, loc, value, width); }
  ArrayAttr fields(Type type) {
    auto s = cast<acir::StructType>(type);
    return source->getAttrOfType<DictionaryAttr>("acir.model").getAs<DictionaryAttr>("types").getAs<ArrayAttr>(s.getName());
  }
  SmallVector<Type> children(Type type) {
    if (auto a = dyn_cast<acir::ArrayType>(type)) {
      Type child = a.getShape().size() == 1 ? a.getElementType() :
        acir::ArrayType::get(b.getContext(), a.getElementType(), a.getShape().drop_front());
      return SmallVector<Type>(a.getShape().front(), child);
    }
    if (auto a = dyn_cast<acir::MaskType>(type)) {
      Type child = a.getShape().size() == 1 ? b.getI1Type() : Type(acir::MaskType::get(b.getContext(), a.getShape().drop_front()));
      return SmallVector<Type>(a.getShape().front(), child);
    }
    if (auto s = dyn_cast<acir::SelectedType>(type)) {
      auto a = cast<acir::ArrayType>(s.getElementType());
      return {a, acir::MaskType::get(b.getContext(), a.getShape())};
    }
    if (auto t = dyn_cast<TupleType>(type)) return SmallVector<Type>(t.getTypes());
    SmallVector<Type> result;
    if (isa<acir::StructType>(type))
      for (auto field : fields(type)) result.push_back(cast<DictionaryAttr>(field).getAs<TypeAttr>("payload_type").getValue());
    return result;
  }
  Type wireType(Type type) {
    if (isa<IntegerType>(type)) return type;
    SmallVector<Type> elements;
    for (auto t : children(type)) elements.push_back(wireType(t));
    return TupleType::get(b.getContext(), elements);
  }
  Value pack(Type type, ValueRange args) { return make("acir.aggregate", args, wireType(type))->getResult(0); }
  Value zero(Type type, bool ones = false) {
    if (auto i = dyn_cast<IntegerType>(type)) return constant(ones ? 1 : 0, i.getWidth());
    SmallVector<Value> elements;
    for (auto t : children(type)) elements.push_back(zero(t, ones));
    return pack(type, elements);
  }
  Value at(Value aggregate, unsigned index) {
    auto types = cast<TupleType>(aggregate.getType()).getTypes();
    return make("acir.extract", {aggregate, constant(index)}, types[index])->getResult(0);
  }
  Value choose(Value cond, Value yes, Value no) {
    if (isa<IntegerType>(yes.getType())) return arith::SelectOp::create(b, loc, cond, yes, no);
    SmallVector<Value> elements;
    for (unsigned i = 0; i < cast<TupleType>(yes.getType()).size(); ++i) elements.push_back(choose(cond, at(yes,i), at(no,i)));
    return make("acir.aggregate", elements, yes.getType())->getResult(0);
  }
  Value mux(Value array, Value index, Type result) {
    if (auto c = index.getDefiningOp<arith::ConstantOp>())
      if (auto n = dyn_cast<IntegerAttr>(c.getValue())) {
        auto i = n.getValue().getZExtValue();
        if (i < cast<TupleType>(array.getType()).size()) return at(array,i);
      }
    Value value = zero(result);
    auto width = cast<IntegerType>(index.getType()).getWidth();
    for (unsigned i = 0; i < cast<TupleType>(array.getType()).size(); ++i) {
      auto selected = arith::CmpIOp::create(b, loc, arith::CmpIPredicate::eq, index, constant(i,width));
      value = choose(selected, at(array,i), value);
    }
    return value;
  }
  Value intersect(Value a, Value c) {
    if (a.getType().isInteger(1)) return arith::AndIOp::create(b,loc,a,c);
    SmallVector<Value> bits;
    for (unsigned i = 0; i < cast<TupleType>(a.getType()).size(); ++i) bits.push_back(intersect(at(a,i),at(c,i)));
    return make("acir.aggregate",bits,a.getType())->getResult(0);
  }
  unsigned fieldIndex(Type type, StringRef name) {
    for (auto [i, field] : llvm::enumerate(fields(type)))
      if (cast<DictionaryAttr>(field).getAs<StringAttr>("name").getValue() == name) return i;
    throw std::runtime_error("unknown field in RTL aggregate");
  }
  Value update(Value base, Type type, Value replacement, ArrayRef<Attribute> path, ArrayRef<Value> indices) {
    if (path.empty()) return replacement;
    auto types = children(type);
    SmallVector<Value> elements;
    bool indexed = isa<UnitAttr>(path.front());
    unsigned field = indexed ? 0 : fieldIndex(type, cast<StringAttr>(path.front()).getValue());
    for (auto [i, child] : llvm::enumerate(types)) {
      auto old = at(base,i);
      if (!indexed && i != field) { elements.push_back(old); continue; }
      auto fresh = update(old,child,replacement,path.drop_front(),indexed ? indices.drop_front() : indices);
      if (indexed) {
        auto index = indices.front();
        auto equal = arith::CmpIOp::create(b,loc,arith::CmpIPredicate::eq,index,constant(i,cast<IntegerType>(index.getType()).getWidth()));
        fresh = choose(equal,fresh,old);
      }
      elements.push_back(fresh);
    }
    return pack(type,elements);
  }
  std::string physical(StringRef root, StringRef aspect, ArrayRef<int64_t> path) {
    std::string name = root.str() + "$" + aspect.str();
    for (auto i : path) name += "$" + std::to_string(i);
    return name;
  }
  void declare(StringRef root, StringRef aspect, Type type, Attribute initial, SmallVector<int64_t> path, SmallVector<Attribute> &parts) {
    if (!isa<IntegerType>(type)) {
      auto data = cast<ArrayAttr>(initial);
      for (auto [i,t] : llvm::enumerate(children(type))) {
        auto next = path; next.push_back(i); declare(root,aspect,t,data[i],next,parts);
      }
      return;
    }
    auto name = physical(root,aspect,path);
    make("acir.state",{}, {}, {b.getNamedAttr("sym_name",b.getStringAttr(name)),b.getNamedAttr("payload_type",TypeAttr::get(type)),
         b.getNamedAttr("kind",b.getStringAttr("cell")),b.getNamedAttr("initial",initial)});
    SmallVector<Attribute> coordinates; for (auto i:path) coordinates.push_back(b.getI64IntegerAttr(i));
    parts.push_back(b.getDictionaryAttr({b.getNamedAttr("resource",b.getStringAttr(name)),b.getNamedAttr("path",b.getArrayAttr(coordinates))}));
  }
  Value current(StringRef root, StringRef aspect, Type type, SmallVector<int64_t> path = {}) {
    if (!isa<IntegerType>(type)) {
      SmallVector<Value> elements;
      for (auto [i,t]:llvm::enumerate(children(type))) { auto next=path;next.push_back(i); elements.push_back(current(root,aspect,t,next)); }
      return pack(type,elements);
    }
    auto name = physical(root,aspect,path);
    if (!currents.count(name)) currents[name]=make("acir.current",{},type,{b.getNamedAttr("resource",b.getStringAttr(name)),b.getNamedAttr("aspect",b.getStringAttr("value"))})->getResult(0);
    return currents[name];
  }
  void next(StringRef root,StringRef aspect,Type type,Value value,SmallVector<int64_t> path={}) {
    if (!isa<IntegerType>(type)) {
      for (auto [i,t]:llvm::enumerate(children(type))) { auto child=path;child.push_back(i);next(root,aspect,t,at(value,i),child); }
      return;
    }
    make("acir.next",value,{}, {b.getNamedAttr("resource",b.getStringAttr(physical(root,aspect,path))),b.getNamedAttr("aspect",b.getStringAttr("value"))});
  }
  Value map(Value v) {
    if (values.count(v)) return values.at(v);
    auto *op=v.getDefiningOp();loc=op->getLoc();
    auto name=op->getName().getStringRef();
    SmallVector<Value> args;for(auto x:op->getOperands()) args.push_back(map(x));
    Value result;
    if(name=="acir.current") result=current(text(op,"resource"),text(op,"aspect"),v.getType());
    else if(name=="acir.aggregate") result=args.empty()?zero(v.getType()):pack(v.getType(),args);
    else if(name=="arith.select") result=choose(args[0],args[1],args[2]);
    else if(name=="acir.array.domain") result=isa<acir::SelectedType>(op->getOperand(0).getType())?at(args[0],1):zero(v.getType(),true);
    else if(name=="acir.array.select") {
      auto data=args[0],mask=args[1];
      if(isa<acir::SelectedType>(op->getOperand(0).getType())) {mask=intersect(at(data,1),mask);data=at(data,0);}
      result=pack(v.getType(),{data,mask});
    } else if(name=="acir.array.index" && isa<acir::SelectedType>(op->getOperand(0).getType())) {
      auto resultType=v.getType();
      if(auto selected=dyn_cast<acir::SelectedType>(resultType)) {
        auto subarray=cast<acir::ArrayType>(selected.getElementType());
        result=pack(resultType,{mux(at(args[0],0),args[1],subarray),mux(at(args[0],1),args[1],acir::MaskType::get(b.getContext(),subarray.getShape()))});
      } else result=mux(at(args[0],0),args[1],resultType);
    } else if(name=="acir.array.index" || name=="acir.extract") {
      if(op->hasAttr("field")) result=at(args[0],fieldIndex(op->getOperand(0).getType(),text(op,"field")));
      else result=mux(args[0],args[1],v.getType());
    } else if(name=="acir.update") result=update(args[0],op->getOperand(0).getType(),args[1],op->getAttrOfType<ArrayAttr>("path").getValue(),ArrayRef<Value>(args).drop_front(2));
    else if(name=="acir.length") result=constant(cast<TupleType>(args[0].getType()).size());
    else result=make(name,args,wireType(v.getType()),op->getAttrs())->getResult(0);
    values[v]=result;return result;
  }
  mlir::ModuleOp lower() {
    SmallVector<Attribute> logical;
    for(auto state:input.getBody().getOps<acir::StateOp>()) {
      auto name=state.getSymName().str(),kind=text(state,"kind");
      Type type=state.getPayloadType();
      Attribute initial=state->getAttr("initial");
      SmallVector<Attribute> parts;
      SmallVector<NamedAttribute> meta{b.getNamedAttr("name",b.getStringAttr(name)),b.getNamedAttr("kind",b.getStringAttr(kind))};
      if(kind=="queue") {
        auto entries=cast<ArrayAttr>(initial);auto capacity=state->getAttrOfType<IntegerAttr>("capacity").getInt();
        SmallVector<Attribute> slots(entries.getValue());while(int64_t(slots.size())<capacity)slots.push_back(acir::zero(type,source));
        type=TupleType::get(b.getContext(),SmallVector<Type>(capacity,type));initial=b.getArrayAttr(slots);
        SmallVector<Attribute> countParts;declare(name,"count",b.getI32Type(),b.getI32IntegerAttr(entries.size()),{},countParts);
        meta.push_back(b.getNamedAttr("count",b.getStringAttr(physical(name,"count",{}))));
        aspects[{name,"count"}]=b.getI32Type();aspects[{name,"storage"}]=type;
        declare(name,"storage",type,initial,{},parts);
      } else {aspects[{name,"value"}]=type;declare(name,"value",type,initial,{},parts);}
      meta.push_back(b.getNamedAttr("prototype",initial));meta.push_back(b.getNamedAttr("parts",b.getArrayAttr(parts)));logical.push_back(b.getDictionaryAttr(meta));
    }
    output->setAttr("logical_states",b.getArrayAttr(logical));
    for(auto fire:input.getBody().getOps<acir::FireOp>())map(fire.getFire());
    for(auto op:input.getBody().getOps<acir::NextOp>()) {
      auto root=text(op,"resource"),aspect=text(op,"aspect");next(root,aspect,aspects.at({root,aspect}),map(op.getValue()));
    }
    for(auto op:input.getBody().getOps<acir::EffectOp>()) {SmallVector<Value> args;for(auto v:op->getOperands())args.push_back(map(v));make("acir.effect",args,{},op->getAttrs());}
    return target;
  }
};
}
mlir::ModuleOp acirLowerWires(mlir::ModuleOp source) { return Wires(source).lower(); }

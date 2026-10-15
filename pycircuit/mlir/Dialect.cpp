#include "Dialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include <functional>
#include <limits>
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/DialectImplementation.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/TypeSwitch.h"
using namespace mlir;
#include "ACIRDialect.cpp.inc"
#define GET_TYPEDEF_CLASSES
#include "ACIRTypes.cpp.inc"
#define GET_OP_CLASSES
#include "ACIROps.cpp.inc"
void acir::ACIRDialect::initialize() {
  addTypes<
#define GET_TYPEDEF_LIST
#include "ACIRTypes.cpp.inc"
      >();
  addOperations<
#define GET_OP_LIST
#include "ACIROps.cpp.inc"
      >();
}
LogicalResult acir::ArrayType::verify(function_ref<InFlightDiagnostic()> error,
                                      Type element, ArrayRef<int64_t> shape) {
  if (shape.empty() || llvm::any_of(shape, [](int64_t d) { return d <= 0; }))
    return error() << "array shape requires positive dimensions";
  int64_t size = 1;
  for (auto d : shape) {
    if (size > std::numeric_limits<int64_t>::max() / d) return error() << "array shape product overflows";
    size *= d;
  }
  if (isa<acir::ArrayType>(element))
    return error() << "nested array element type is unsupported; use shape";
  return success();
}
namespace {
Operation *resolve(Operation *op, SymbolRefAttr target) {
  auto root = op->getParentOfType<mlir::ModuleOp>();
  return root ? SymbolTable::lookupSymbolIn(root, target) : nullptr;
}
Type payload(Type type) {
  if (auto t = dyn_cast<acir::QueueType>(type))
    return t.getElementType();
  if (auto t = dyn_cast<acir::CellType>(type))
    return t.getElementType();
  if (auto t = dyn_cast<acir::SignalType>(type))
    return t.getElementType();
  if (auto t = dyn_cast<acir::EHRType>(type))
    return t.getElementType();
  return {};
}
LogicalResult definition(Operation *op) {
  if (!isa<acir::ModuleOp>(op->getParentOp()))
    return op->emitOpError("definition must belong to a Module scope");
  auto signature = cast<FunctionType>(
      cast<TypeAttr>(op->getAttr("function_type")).getValue());
  for (auto &region : op->getRegions()) {
    if (region.empty() && &region == &op->getRegion(0))
      continue;
    if (region.empty())
      return op->emitOpError("requires body entry block");
    if (region.front().getArgumentTypes() != signature.getInputs())
      return op->emitOpError("guard and body arguments must match signature");
  }
  for (auto [index, region] : llvm::enumerate(op->getRegions())) {
    for (auto &block : region) {
      if (block.empty())
        return op->emitOpError("region requires a terminator");
      auto &end = block.back();
      if (end.getNumSuccessors() == 0 &&
          (index == 0 ? !isa<acir::GuardYieldOp>(end)
                      : !isa<acir::ReturnOp>(end)))
        return op->emitOpError("region has an incorrect exit terminator");
    }
  }
  bool invalid = false;
  op->getRegion(0).walk([&](Operation *nested) {
    auto name = nested->getName().getStringRef();
    if (name.ends_with(".write") || name == "acir.signal.set" ||
        name == "acir.queue.pop" || name == "acir.queue.push") {
      nested->emitOpError("guard must only observe state");
      invalid = true;
    }
    if (auto yield = dyn_cast<acir::GuardYieldOp>(nested))
      if (!yield.getCondition().getType().isInteger(1)) {
        yield.emitOpError("guard yields one bool");
        invalid = true;
      }
  });
  return failure(invalid);
}
LogicalResult call(Operation *op) {
  if (op->getParentOfType<func::FuncOp>()) return op->emitOpError("payload helper cannot call a Rule");
  auto *def = resolve(op, cast<SymbolRefAttr>(op->getAttr("target")));
  if (!isa_and_nonnull<acir::RuleOp>(def))
    return op->emitOpError("unknown Rule target");
  auto metadata = def->getAttrOfType<DictionaryAttr>("acir.function");
  if (auto publicFlag = metadata.getAs<BoolAttr>("public"))
    if (!publicFlag.getValue() && op->getParentOfType<acir::ModuleOp>() !=
                                      def->getParentOfType<acir::ModuleOp>())
      return op->emitOpError("cannot call a private Rule from another Module");
  auto signature = cast<FunctionType>(
      cast<TypeAttr>(def->getAttr("function_type")).getValue());
  if (op->getOperandTypes() != signature.getInputs() ||
      op->getResultTypes() != signature.getResults())
    return op->emitOpError("call signature mismatch");
  return success();
}
} // namespace
namespace {
LogicalResult declaration(Operation *op) {
  auto name = op->getName().getStringRef();
  if (name == "acir.cell" || name == "acir.ehr") {
    if (!op->getAttrOfType<StringAttr>("initial_fn"))
      return op->emitOpError("requires initialization");
    if (op->hasAttr("capacity_fn"))
      return op->emitOpError("has no capacity");
  }
  return success();
}
LogicalResult accessMetadata(Operation *op) {
  if (op->getParentOfType<func::FuncOp>()) return op->emitOpError("payload helper cannot access state");
  if (op->hasAttr("port") || op->hasAttr("ports") || op->hasAttr("aspect") ||
      op->hasAttr("view"))
    return op->emitOpError(
        "component views and ports are derived; remove manual metadata");
  return success();
}
} // namespace
#define DECL_VERIFY(CLASS)                                                     \
  LogicalResult acir::CLASS::verify() { return declaration(*this); }
DECL_VERIFY(QueueOp)
DECL_VERIFY(CellOp)
DECL_VERIFY(SignalOp)
DECL_VERIFY(EHROp)
LogicalResult acir::InstanceOp::verify() {
  if (!isa_and_nonnull<ModuleOp>(resolve(*this, getTarget())))
    return emitOpError("requires a Module target");
  return success();
}
LogicalResult acir::ModuleCallOp::verify() {
  auto target = dyn_cast_or_null<ModuleOp>(resolve(*this, getTarget()));
  if (!target || target.getDeclarations().getOps<BodyOp>().empty())
    return emitOpError("requires a Module with a body");
  if (!(*this)->getParentOfType<BodyOp>())
    return emitOpError("Module calls belong to Module bodies");
  return success();
}
LogicalResult acir::BodyOp::verify() {
  if (!isa<ModuleOp>((*this)->getParentOp()) || getBody().empty() ||
      getBody().front().getNumArguments())
    return emitOpError("Module body needs an entry without arguments");
  return success();
}
LogicalResult acir::BodyEndOp::verify() { return success(); }
LogicalResult acir::GuardYieldOp::verify() {
  auto *parent = (*this)->getParentOp();
  return success(isa<RuleOp>(parent) &&
                 (*this)->getParentRegion() == &parent->getRegion(0));
}
LogicalResult acir::RuleOp::verify() { return definition(*this); }
LogicalResult acir::RuleCallOp::verify() { return call(*this); }
LogicalResult acir::GetOp::verify() {
  if (failed(accessMetadata(*this))) return failure();
  auto *target = resolve(*this, getTarget());
  if (!target)
    return emitOpError("unknown resource symbol ") << getTarget();
  auto kind = target->getName().getStringRef();
  auto type = getReference().getType();
  auto declared = target->getAttrOfType<TypeAttr>("payload_type");
  if (!declared || declared.getValue() != payload(type))
    return emitOpError("reference payload must match declaration");
  if ((isa<QueueType>(type) && kind == "acir.queue") ||
      (isa<CellType>(type) && kind == "acir.cell") ||
      (isa<SignalType>(type) && kind == "acir.signal") ||
      (isa<EHRType>(type) && kind == "acir.ehr"))
    return success();
  return emitOpError("reference type must match declaration kind");
}
LogicalResult acir::ReturnOp::verify() {
  auto rule = (*this)->getParentOfType<RuleOp>();
  if (!rule)
    return emitOpError("must return from a Rule");
  return success(getValues().getTypes() ==
                 cast<FunctionType>(rule.getFunctionType()).getResults());
}
#define READ_VERIFY(CLASS)                                                     \
  LogicalResult acir::CLASS::verify() {                                        \
    if (failed(accessMetadata(*this)))                                         \
      return failure();                                                        \
    if (payload(getResource().getType()) != getValue().getType())              \
      return emitOpError("result must match component payload");               \
    return success();                                                          \
  }
READ_VERIFY(QueuePeekOp)
READ_VERIFY(CellReadOp)
READ_VERIFY(EHRReadOp)
READ_VERIFY(SignalGetOp)
#define QUERY_VERIFY(CLASS)                                                    \
  LogicalResult acir::CLASS::verify() { return accessMetadata(*this); }
QUERY_VERIFY(QueueEmptyOp)
QUERY_VERIFY(QueueFullOp)
QUERY_VERIFY(QueueSizeOp)
#define WRITE_VERIFY(CLASS)                                                    \
  LogicalResult acir::CLASS::verify() {                                        \
    if (failed(accessMetadata(*this)))                                         \
      return failure();                                                        \
    if (payload(getResource().getType()) != getValue().getType())              \
      return emitOpError("value must match component payload");                \
    return success();                                                          \
  }
WRITE_VERIFY(PushOp)
WRITE_VERIFY(CellWriteOp)
WRITE_VERIFY(EHRWriteOp)
WRITE_VERIFY(SignalSetOp)
LogicalResult acir::ExportOp::verify() {
  if (getInput().getType() != getValue().getType())
    return emitOpError("export must preserve its input type");
  return success();
}
namespace {
acir::StateOp declaredState(Operation *op) {
  auto circuit = op->getParentOfType<acir::CircuitOp>();
  auto name = op->getAttrOfType<StringAttr>("resource");
  if (!circuit || !name)
    return {};
  return dyn_cast_or_null<acir::StateOp>(
      SymbolTable::lookupSymbolIn(circuit, name.getValue()));
}
LogicalResult executableAccess(Operation *op) {
  auto target = declaredState(op);
  if (!target)
    return op->emitOpError("requires a declared state resource");
  if (auto resources = op->getAttrOfType<ArrayAttr>("resources")) {
    auto ports = op->getAttrOfType<ArrayAttr>("ports");
    if (resources.empty() || !ports || ports.size() != resources.size())
      return op->emitOpError("logical access requires matching resources and ports");
    auto circuit = op->getParentOfType<acir::CircuitOp>();
    for (auto [res, port] : llvm::zip(resources, ports)) {
      auto name = dyn_cast<StringAttr>(res); auto p = dyn_cast<IntegerAttr>(port);
      auto state = name ? dyn_cast_or_null<acir::StateOp>(SymbolTable::lookupSymbolIn(circuit, name.getValue())) : acir::StateOp();
      if (!state || state.getPayloadType() != target.getPayloadType() || state->getAttr("kind") != target->getAttr("kind") || !p || p.getInt() < 0)
        return op->emitOpError("incompatible logical resource or port");
    }
    bool read = isa<acir::ReadOp>(op);
    auto method = op->getAttrOfType<StringAttr>("method");
    unsigned index = read || (method && method.getValue() == "pop") ? 1 : 2;
    if ((op->getNumOperands() != index+1 && op->getNumOperands() != index+2) || !isa<IntegerType>(op->getOperand(index).getType()))
      return op->emitOpError("logical access requires integer index and optional flat domain");
    if (op->getNumOperands() == index+2) {
      auto mask = dyn_cast<acir::MaskType>(op->getOperand(index+1).getType());
      if (!mask || mask.getShape().size() != 1 || mask.getShape()[0] != int64_t(resources.size()))
        return op->emitOpError("logical selection domain shape mismatch");
    }
  }
  return success();
}
} // namespace
LogicalResult acir::CircuitOp::verify() {
  auto stage = (*this)->getAttrOfType<StringAttr>("stage");
  if (!stage ||
      (stage.getValue() != "transaction" && stage.getValue() != "rtl"))
    return emitOpError("requires transaction or rtl stage");
  if (!getBody().hasOneBlock() || getBody().front().getNumArguments())
    return emitOpError("requires one dataflow block without arguments");
  auto schedule = (*this)->getAttrOfType<ArrayAttr>("schedule");
  if (!schedule)
    return emitOpError("requires resolved transaction schedule");
  llvm::SmallDenseSet<int64_t> ids;
  for (auto fire : getBody().getOps<FireOp>()) {
    auto id = fire->getAttrOfType<IntegerAttr>("txn");
    if (!id || id.getInt() < 0 || !ids.insert(id.getInt()).second)
      return emitOpError("fire IDs must be unique and nonnegative");
  }
  if (schedule.size() != ids.size())
    return emitOpError("schedule must contain every fire exactly once");
  llvm::SmallDenseSet<int64_t> scheduled;
  llvm::DenseMap<int64_t, unsigned> rank;
  for (auto [i, value] : llvm::enumerate(schedule)) {
    auto id = dyn_cast<IntegerAttr>(value);
    if (!id || !ids.count(id.getInt()) || !scheduled.insert(id.getInt()).second)
      return emitOpError("invalid or repeated schedule entry");
    rank[id.getInt()] = i;
  }
  if (auto edges = (*this)->getAttrOfType<ArrayAttr>("edges"))
    for (auto edge : edges) {
      auto d = dyn_cast<DictionaryAttr>(edge);
      auto from = d ? d.getAs<IntegerAttr>("from") : IntegerAttr();
      auto to = d ? d.getAs<IntegerAttr>("to") : IntegerAttr();
      if (!from || !to || !ids.count(from.getInt()) ||
          !ids.count(to.getInt()) || rank[from.getInt()] >= rank[to.getInt()])
        return emitOpError("schedule violates a same-tick order edge");
    }
  bool invalid = false;
  if (stage.getValue() == "rtl")
    getBody().walk([&](Operation *op) {
      auto name = op->getName().getStringRef();
      if (isa<ReadOp, RuleCallOp, ModuleCallOp>(op) || name.starts_with("acir.array.") || name == "acir.update" || name.starts_with("scf.") ||
          name.starts_with("cf.") || name.starts_with("func.")) {
        op->emitOpError("unresolved operation in RTL stage");
        invalid = true;
      }
      if (auto effect = dyn_cast<EffectOp>(op))
        if (effect->getAttrOfType<StringAttr>("method").getValue() != "check") {
          effect.emitOpError("state effects must lower to next-state SSA");
          invalid = true;
        }
    });
  return failure(invalid);
}
namespace {
SmallVector<Type> aggregateTypes(Type type, Operation *op) {
  auto *ctx = op->getContext();
  if (auto a = dyn_cast<acir::ArrayType>(type)) {
    Type t = a.getShape().size() == 1 ? a.getElementType() : acir::ArrayType::get(ctx,a.getElementType(),a.getShape().drop_front());
    return SmallVector<Type>(a.getShape().front(),t);
  }
  if (auto m = dyn_cast<acir::MaskType>(type)) {
    Type t = m.getShape().size() == 1 ? IntegerType::get(ctx,1) : Type(acir::MaskType::get(ctx,m.getShape().drop_front()));
    return SmallVector<Type>(m.getShape().front(),t);
  }
  if (auto a = dyn_cast<acir::SelectedType>(type)) {
    auto base = dyn_cast<acir::ArrayType>(a.getElementType());
    if (!base) return {};
    return {base,acir::MaskType::get(ctx,base.getShape())};
  }
  if (auto tuple = dyn_cast<TupleType>(type)) return SmallVector<Type>(tuple.getTypes());
  SmallVector<Type> result;
  if (auto structure = dyn_cast<acir::StructType>(type)) {
    auto root=op->getParentOfType<mlir::ModuleOp>();
    auto model=root->getAttrOfType<DictionaryAttr>("acir.model");
    auto types=model?model.getAs<DictionaryAttr>("types"):DictionaryAttr();
    auto fields=types?types.getAs<ArrayAttr>(structure.getName()):ArrayAttr();
    if (fields) for (auto f:fields) {
      auto d=dyn_cast<DictionaryAttr>(f);auto t=d?d.getAs<TypeAttr>("payload_type"):TypeAttr();
      if (!t) return {};
      result.push_back(t.getValue());
    }
  }
  return result;
}
bool stateValue(Type type, Attribute value, Operation *op) {
  if (auto i=dyn_cast<IntegerType>(type)) { auto a=dyn_cast<IntegerAttr>(value); return a && a.getType()==i; }
  if (!isa<acir::ArrayType,acir::StructType,TupleType>(type)) return false;
  auto array=dyn_cast<ArrayAttr>(value); auto members=aggregateTypes(type,op);
  if (!array || array.size()!=members.size()) return false;
  for (auto [t,v]:llvm::zip(members,array)) if (!stateValue(t,v,op)) return false;
  return true;
}
bool completePayload(Type type, Operation *op) {
  if (isa<IntegerType>(type)) return true;
  if (!isa<acir::ArrayType, acir::StructType, TupleType>(type)) return false;
  if (auto array = dyn_cast<acir::ArrayType>(type))
    return completePayload(array.getElementType(), op);
  return llvm::all_of(aggregateTypes(type, op),
                     [&](Type member) { return completePayload(member, op); });
}
}
LogicalResult acir::StateOp::verify() {
  if (!completePayload(getPayloadType(), *this))
    return emitOpError("state requires complete payload types");
  auto kind = (*this)->getAttrOfType<StringAttr>("kind");
  if (!kind || !(kind.getValue() == "cell" || kind.getValue() == "ehr" ||
                 kind.getValue() == "queue" || kind.getValue() == "signal"))
    return emitOpError("invalid state kind");
  if (!(*this)->getAttr("initial"))
    return emitOpError("requires complete initial state");
  if (kind.getValue() == "queue") {
    auto cap = (*this)->getAttrOfType<IntegerAttr>("capacity");
    auto init = (*this)->getAttrOfType<ArrayAttr>("initial");
    if (!cap || cap.getInt() < 1 || !init ||
        init.size() > (uint64_t)cap.getInt())
      return emitOpError("invalid fixed Queue storage");
  }
  auto initial = (*this)->getAttr("initial");
  if (kind.getValue() == "queue") {
    for (auto value : cast<ArrayAttr>(initial))
      if (!stateValue(getPayloadType(),value,*this)) return emitOpError("Queue message must match complete payload type");
  } else if (!stateValue(getPayloadType(),initial,*this))
    return emitOpError("initial state must match complete payload type and shape");
  auto circuit = (*this)->getParentOfType<CircuitOp>();
  if (circuit &&
      circuit->getAttrOfType<StringAttr>("stage").getValue() == "rtl" &&
      kind.getValue() == "signal")
    return emitOpError("Signal must lower to combinational data/valid");
  return success();
}
LogicalResult acir::BindOp::verify() {
  if (getInput().getType() != getValue().getType())
    return emitOpError("binding cannot change value type");
  return success();
}
LogicalResult acir::ReadOp::verify() {
  if (failed(executableAccess(*this)))
    return failure();
  auto method = (*this)->getAttrOfType<StringAttr>("method");
  auto port = (*this)->getAttrOfType<IntegerAttr>("port");
  if (!method || !port || port.getInt() < 0)
    return emitOpError("requires resolved method and nonnegative port");
  if (getInputs().empty() || (!(*this)->hasAttr("resources") && getInputs().size() != 1) || !getInputs()[0].getType().isInteger(1))
    return emitOpError("requires one path condition");
  return success();
}
LogicalResult acir::FireOp::verify() {
  if (!(*this)->getAttrOfType<IntegerAttr>("txn"))
    return emitOpError("requires transaction identity");
  // Backends may use successful checks as facts. A dependency must return
  // true or throw; an arbitrary false-valued Boolean is not a check token.
  std::function<bool(Value)> checked = [&](Value value) {
    auto *op = value.getDefiningOp();
    if (!op) return false;
    if (isa<AssertOp>(op)) return true;
    if (auto c = dyn_cast<arith::ConstantOp>(op))
      return cast<IntegerAttr>(c.getValue()).getValue().isOne();
    if (isa<BindOp, ExportOp>(op)) return checked(op->getOperand(0));
    if (auto select = dyn_cast<arith::SelectOp>(op))
      return checked(select.getTrueValue()) && checked(select.getFalseValue());
    if (isa<arith::AndIOp>(op))
      return checked(op->getOperand(0)) && checked(op->getOperand(1));
    if (isa<arith::OrIOp>(op))
      return checked(op->getOperand(0)) || checked(op->getOperand(1));
    return false;
  };
  for (auto check : getChecks())
    if (!checked(check))
      return emitOpError("check dependency must return true or assert");
  return success();
}
LogicalResult acir::EffectOp::verify() {
  auto method = (*this)->getAttrOfType<StringAttr>("method");
  if (!method)
    return emitOpError("requires resolved effect method");
  if (method.getValue() == "check")
    return success(getInputs().size() == 1 &&
                   getInputs()[0].getType().isInteger(1));
  if (failed(executableAccess(*this)))
    return failure();
  auto target = declaredState(*this);
  auto port = (*this)->getAttrOfType<IntegerAttr>("port");
  if (!port || port.getInt() < 0)
    return emitOpError("requires assigned port");
  if (method.getValue() == "pop")
    return success((*this)->hasAttr("resources") || getInputs().empty());
  if (getInputs().empty() || (!(*this)->hasAttr("resources") && getInputs().size() != 1) ||
      getInputs()[0].getType() != target.getPayloadType())
    return emitOpError("effect payload type mismatch");
  return success();
}
LogicalResult acir::CurrentOp::verify() {
  if (failed(executableAccess(*this)))
    return failure();
  auto target = declaredState(*this);
  auto aspect = (*this)->getAttrOfType<StringAttr>("aspect");
  if (!aspect)
    return emitOpError("requires resolved state aspect");
  Type expected = target.getPayloadType();
  if (aspect.getValue() != "value" && aspect.getValue() != "count" &&
      aspect.getValue() != "storage")
    return emitOpError("unknown state aspect");
  if (aspect.getValue() != "value" &&
      target->getAttrOfType<StringAttr>("kind").getValue() != "queue")
    return emitOpError("count/storage require a Queue state");
  if (aspect.getValue() == "count")
    expected = IntegerType::get(getContext(), 32);
  if (aspect.getValue() == "storage")
    expected =
        TupleType::get(getContext(), SmallVector<Type>(
                       target->getAttrOfType<IntegerAttr>("capacity").getInt(), expected));
  if (getValue().getType() != expected)
    return emitOpError("current state type mismatch");
  return success();
}
LogicalResult acir::NextOp::verify() {
  if (failed(executableAccess(*this)))
    return failure();
  auto target = declaredState(*this);
  auto aspect = (*this)->getAttrOfType<StringAttr>("aspect");
  if (!aspect)
    return emitOpError("requires next-state aspect");
  if (aspect.getValue() != "value" && aspect.getValue() != "count" &&
      aspect.getValue() != "storage")
    return emitOpError("unknown state aspect");
  Type expected = target.getPayloadType();
  if (aspect.getValue() != "value" &&
      target->getAttrOfType<StringAttr>("kind").getValue() != "queue")
    return emitOpError("count/storage require a Queue state");
  if (aspect.getValue() == "count")
    expected = IntegerType::get(getContext(), 32);
  if (aspect.getValue() == "storage")
    expected =
        TupleType::get(getContext(), SmallVector<Type>(
                       target->getAttrOfType<IntegerAttr>("capacity").getInt(), expected));
  if (getValue().getType() != expected)
    return emitOpError("next-state type mismatch");
  return success();
}

LogicalResult acir::MaskType::verify(function_ref<InFlightDiagnostic()> error, ArrayRef<int64_t> shape) {
  if (shape.empty() || llvm::any_of(shape, [](int64_t d) { return d <= 0; }))
    return error() << "mask shape requires positive dimensions";
  return success();
}
LogicalResult acir::ArraySelectOp::verify() {
  Type base = getArray().getType();
  if (auto selected = dyn_cast<SelectedType>(base)) base = selected.getElementType();
  auto array = dyn_cast<ArrayType>(base);
  if (!array || array.getShape() != getMask().getType().getShape() ||
      getSelected().getType().getElementType() != base)
    return emitOpError("selection must preserve array type and shape");
  return success();
}
LogicalResult acir::ArrayDomainOp::verify() {
  Type base = getArray().getType();
  if (auto selected = dyn_cast<SelectedType>(base)) base = selected.getElementType();
  auto array = dyn_cast<ArrayType>(base);
  if (!array || array.getShape() != getMask().getType().getShape())
    return emitOpError("selection domain shape mismatch");
  return success();
}

LogicalResult acir::ArrayIndexOp::verify() {
  Type base = getArray().getType();
  bool selected = isa<SelectedType>(base);
  if (selected) base = cast<SelectedType>(base).getElementType();
  Type expected;
  ArrayRef<int64_t> shape;
  if (auto array = dyn_cast<ArrayType>(base)) {
    shape = array.getShape();
    expected = shape.size() == 1 ? array.getElementType() :
      ArrayType::get(getContext(), array.getElementType(), shape.drop_front());
    if (selected && shape.size() > 1) expected = SelectedType::get(getContext(), expected);
  } else if (auto mask = dyn_cast<MaskType>(base)) {
    shape = mask.getShape();
    expected = shape.size() == 1 ? IntegerType::get(getContext(), 1) :
      Type(MaskType::get(getContext(), shape.drop_front()));
  } else return emitOpError("requires array or mask");
  if (getValue().getType() != expected) return emitOpError("index result type mismatch");
  if (auto constant = getIndex().getDefiningOp<arith::ConstantOp>()) {
    auto integer = dyn_cast<IntegerAttr>(constant.getValue());
    if (!integer || integer.getValue().getLimitedValue() >= uint64_t(shape.front()))
      return emitOpError("static array index out of bounds");
  }
  return success();
}

LogicalResult acir::AggregateOp::verify() {
  if (getElements().empty()) return success(); // Explicit internal zero value.
  auto members=aggregateTypes(getValue().getType(),*this);
  if (getElements().getTypes()!=ArrayRef<Type>(members)) return emitOpError("aggregate elements must exactly match shape and types");
  return success();
}
LogicalResult acir::ExtractOp::verify() {
  auto members=aggregateTypes(getAggregate().getType(),*this);
  if (getField()) {
    auto type=dyn_cast<StructType>(getAggregate().getType());
    if (!type) return emitOpError("field requires struct");
    auto root=(*this)->getParentOfType<mlir::ModuleOp>();
    auto fields=root->getAttrOfType<DictionaryAttr>("acir.model").getAs<DictionaryAttr>("types").getAs<ArrayAttr>(type.getName());
    for (auto [i,f]:llvm::enumerate(fields))
      if (cast<DictionaryAttr>(f).getAs<StringAttr>("name").getValue()==*getField())
        return success(getValue().getType()==members[i]);
    return emitOpError("unknown struct field");
  }
  if (getIndices().size()!=1 || members.empty()) return emitOpError("requires one aggregate index");
  if (auto constant=getIndices()[0].getDefiningOp<arith::ConstantOp>()) {
    auto index=cast<IntegerAttr>(constant.getValue()).getValue().getZExtValue();
    if (index>=members.size()) return emitOpError("static aggregate index out of bounds");
    if (getValue().getType()!=members[index]) return emitOpError("extract result type mismatch");
  } else if (llvm::any_of(members,[&](Type t){return t!=getValue().getType();}))
    return emitOpError("dynamic indexing requires homogeneous element types");
  return success();
}
LogicalResult acir::UpdateOp::verify() {
  if (getValue().getType()!=getAggregate().getType()) return emitOpError("update must preserve aggregate type");
  Type type = getAggregate().getType();
  if (auto selected = dyn_cast<SelectedType>(type)) type = selected.getElementType();
  unsigned index = 0;
  if (getPath().empty()) return emitOpError("update requires a nonempty path");
  for (Attribute step : getPath()) {
    auto members = aggregateTypes(type, *this);
    if (isa<UnitAttr>(step)) {
      auto array = dyn_cast<ArrayType>(type);
      if (!array || index >= getIndices().size())
        return emitOpError("update coordinate requires an array and an integer index");
      Value coordinate = getIndices()[index++];
      if (!isa<IntegerType>(coordinate.getType()) || coordinate.getType().isInteger(1))
        return emitOpError("update coordinate must be an integer");
      if (auto constant = coordinate.getDefiningOp<arith::ConstantOp>())
        if (cast<IntegerAttr>(constant.getValue()).getValue().getLimitedValue() >= uint64_t(array.getShape().front()))
          return emitOpError("static update index out of bounds");
      type = members.front();
    } else if (auto field = dyn_cast<StringAttr>(step)) {
      auto structure = dyn_cast<StructType>(type);
      if (!structure) return emitOpError("update field requires a struct");
      auto model = (*this)->getParentOfType<mlir::ModuleOp>()->getAttrOfType<DictionaryAttr>("acir.model");
      auto fields = model.getAs<DictionaryAttr>("types").getAs<ArrayAttr>(structure.getName());
      bool found = false;
      for (auto [i, item] : llvm::enumerate(fields)) {
        if (cast<DictionaryAttr>(item).getAs<StringAttr>("name") == field) {
          type = members[i]; found = true; break;
        }
      }
      if (!found) return emitOpError("unknown update field");
    } else return emitOpError("invalid update path step");
  }
  if (index != getIndices().size()) return emitOpError("update index count does not match path");
  if (getOperand(1).getType() != type) return emitOpError("update payload type mismatch");
  return success();
}
LogicalResult acir::OptionalOp::verify() {
  if(getValue().getType()!=getResult().getType())return emitOpError("optional result type mismatch");
  return success();
}
LogicalResult acir::ArrayWriteOp::verify() {
  Type type=getArray().getType();if(auto selected=dyn_cast<SelectedType>(type))type=selected.getElementType();
  auto array=dyn_cast<ArrayType>(type);
  if(!array || payload(array.getElementType())!=getValue().getType())return emitOpError("array write needs component references and matching payload");
  return accessMetadata(*this);
}

namespace {
int64_t shapeSize(ArrayAttr shape) {
  if (shape.empty()) return -1;
  int64_t count=1;
  for(auto dimension:shape) {
    auto d=dyn_cast<IntegerAttr>(dimension);
    if(!d || d.getInt()<=0 || count>std::numeric_limits<int64_t>::max()/d.getInt())return -1;
    count*=d.getInt();
  }
  return count;
}
}
LogicalResult acir::InstanceArrayOp::verify() {
  if(shapeSize(getShape())!=int64_t(getTargets().size()))return emitOpError("instance array shape mismatch");
  for(auto target:getTargets()) {
    auto ref=dyn_cast<SymbolRefAttr>(target);
    if(!ref || !isa_and_nonnull<ModuleOp>(resolve(*this,ref)))return emitOpError("invalid Module array instance");
  }
  return success();
}
LogicalResult acir::ResourceArrayOp::verify() {
  if(shapeSize(getShape())!=int64_t(getResources().size()))return emitOpError("resource array shape mismatch");
  auto circuit=(*this)->getParentOfType<CircuitOp>();
  for(auto resource:getResources()) {
    auto ref=dyn_cast<StringAttr>(resource);
    if(!ref || !circuit || !isa_and_nonnull<StateOp>(SymbolTable::lookupSymbolIn(circuit,ref.getValue())))return emitOpError("unknown array resource");
  }
  return success();
}

#include "Dialect.h"
#include "mlir/IR/Builders.h"
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
                                      Type element, int64_t size) {
  if (size < 0)
    return error() << "array extent must be nonnegative";
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
  if (auto t = dyn_cast<acir::QueueArrayType>(type))
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
DECL_VERIFY(QueueArrayOp)
LogicalResult acir::InstanceOp::verify() {
  if (!isa_and_nonnull<ModuleOp>(resolve(*this, getTarget())))
    return emitOpError("requires a Module target");
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
      (isa<EHRType>(type) && kind == "acir.ehr") ||
      (isa<QueueArrayType>(type) && kind == "acir.queue.array"))
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
  LogicalResult acir::CLASS::verify() { return success(); }
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
      if (isa<ReadOp, RuleCallOp>(op) || name.starts_with("scf.") ||
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
LogicalResult acir::StateOp::verify() {
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
  auto circuit = (*this)->getParentOfType<CircuitOp>();
  if (circuit &&
      circuit->getAttrOfType<StringAttr>("stage").getValue() == "rtl" &&
      kind.getValue() == "signal")
    return emitOpError("Signal must lower to combinational data/valid");
  return success();
}
LogicalResult acir::ReadOp::verify() {
  if (failed(executableAccess(*this)))
    return failure();
  auto method = (*this)->getAttrOfType<StringAttr>("method");
  auto port = (*this)->getAttrOfType<IntegerAttr>("port");
  if (!method || !port || port.getInt() < 0)
    return emitOpError("requires resolved method and nonnegative port");
  if (getInputs().size() != 1 || !getInputs()[0].getType().isInteger(1))
    return emitOpError("requires one path condition");
  return success();
}
LogicalResult acir::FireOp::verify() {
  if (!(*this)->getAttrOfType<IntegerAttr>("txn"))
    return emitOpError("requires transaction identity");
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
    return success(getInputs().empty());
  if (getInputs().size() != 1 ||
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
        ArrayType::get(getContext(), expected,
                       target->getAttrOfType<IntegerAttr>("capacity").getInt());
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
        ArrayType::get(getContext(), expected,
                       target->getAttrOfType<IntegerAttr>("capacity").getInt());
  if (getValue().getType() != expected)
    return emitOpError("next-state type mismatch");
  return success();
}

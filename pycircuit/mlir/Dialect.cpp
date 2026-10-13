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
LogicalResult acir::ArrayType::verify(function_ref<InFlightDiagnostic()> error, Type element, int64_t size) {
  if (size < 0) return error() << "array extent must be nonnegative";
  return success();
}
namespace {
Operation *resolve(Operation *op, SymbolRefAttr target) {
  auto root = op->getParentOfType<mlir::ModuleOp>();
  return root ? SymbolTable::lookupSymbolIn(root, target) : nullptr;
}
Type payload(Type type) {
  if (auto t = dyn_cast<acir::QueueType>(type)) return t.getElementType();
  if (auto t = dyn_cast<acir::CellType>(type)) return t.getElementType();
  if (auto t = dyn_cast<acir::SignalType>(type)) return t.getElementType();
  if (auto t = dyn_cast<acir::EHRType>(type)) return t.getElementType();
  if (auto t = dyn_cast<acir::QueueArrayType>(type)) return t.getElementType();
  return {};
}
LogicalResult definition(Operation *op) {
  if (!isa<acir::ModuleOp>(op->getParentOp())) return op->emitOpError("definition must belong to a Module scope");
  auto signature = cast<FunctionType>(cast<TypeAttr>(op->getAttr("function_type")).getValue());
  for (auto &region : op->getRegions()) {
    if (region.empty() && &region == &op->getRegion(0)) continue;
    if (region.empty()) return op->emitOpError("requires body entry block");
    if (region.front().getArgumentTypes() != signature.getInputs()) return op->emitOpError("guard and body arguments must match signature");
  }
  for (auto [index, region] : llvm::enumerate(op->getRegions())) {
    for (auto &block : region) {
      if (block.empty()) return op->emitOpError("region requires a terminator");
      auto &end = block.back();
      if (end.getNumSuccessors() == 0 && (index == 0 ? !isa<acir::GuardYieldOp>(end) : !isa<acir::ReturnOp>(end))) return op->emitOpError("region has an incorrect exit terminator");
    }
  }
  bool invalid = false;
  op->getRegion(0).walk([&](Operation *nested) {
    auto name = nested->getName().getStringRef();
    if (name.ends_with(".write") || name == "acir.signal.set" || name == "acir.queue.pop" || name == "acir.queue.push" || name == "acir.rule.call") {
      nested->emitOpError("guard must only observe state"); invalid = true;
    }
    if (auto yield = dyn_cast<acir::GuardYieldOp>(nested))
      if (!yield.getCondition().getType().isInteger(1)) {
        yield.emitOpError("guard yields one bool"); invalid = true;
      }
  });
  return failure(invalid);
}
LogicalResult call(Operation *op, bool rule) {
  auto target = cast<SymbolRefAttr>(op->getAttr("target"));
  auto *def = resolve(op, target);
  if (!def || (rule ? !isa<acir::RuleOp>(def) : !isa<acir::MethodOp>(def))) return op->emitOpError("unknown or incorrect target symbol ") << target;
  auto signature = cast<FunctionType>(cast<TypeAttr>(def->getAttr("function_type")).getValue());
  if (op->getOperandTypes() != signature.getInputs()) return op->emitOpError("arguments must match target signature");
  if (!rule && op->getResultTypes() != signature.getResults()) return op->emitOpError("results must match target signature");
  if (rule) {
    auto bindings = cast<ArrayAttr>(op->getAttr("output_bindings"));
    auto outputs = def->getAttrOfType<ArrayAttr>("output_types");
    if (!outputs || bindings.size() != outputs.size()) return op->emitOpError("output binding count must match slots");
    for (auto [index, binding] : llvm::enumerate(bindings)) {
      if (isa<UnitAttr>(binding) && isa<UnitAttr>(outputs[index])) continue;
      auto reference = dyn_cast<SymbolRefAttr>(binding);
      auto *resource = reference ? resolve(op, reference) : nullptr;
      if (!resource) return op->emitOpError("output binding must reference a resource");
      auto expected = dyn_cast<TypeAttr>(outputs[index]);
      auto kind = resource->getName().getStringRef().drop_front(5);
      Type ref = expected ? expected.getValue() : Type();
      if (!ref || (isa<acir::QueueType>(ref) && kind != "queue") || (isa<acir::CellType>(ref) && kind != "cell") || (isa<acir::SignalType>(ref) && kind != "signal")) return op->emitOpError("output binding component kind mismatch");
      auto source = resource->getAttrOfType<TypeAttr>("payload_type");
      if (!source || source.getValue() != payload(ref)) return op->emitOpError("output binding payload type mismatch");
    }
  }
  return success();
}
}
namespace {
LogicalResult declaration(Operation *op) {
  auto name = op->getName().getStringRef();
  if (name == "acir.cell" || name == "acir.ehr" || name == "acir.signal") {
    if (!op->getAttrOfType<StringAttr>("initial_fn")) return op->emitOpError("requires initialization");
    if (op->hasAttr("capacity_fn")) return op->emitOpError("has no capacity");
  }
  return success();
}
LogicalResult accessMetadata(Operation *op) {
  if (op->hasAttr("port") || op->hasAttr("ports") || op->hasAttr("aspect") || op->hasAttr("view")) return op->emitOpError("component views and ports are derived; remove manual metadata");
  return success();
}
}
#define DECL_VERIFY(CLASS) LogicalResult acir::CLASS::verify() { return declaration(*this); }
DECL_VERIFY(QueueOp)
DECL_VERIFY(CellOp)
DECL_VERIFY(SignalOp)
DECL_VERIFY(EHROp)
DECL_VERIFY(QueueArrayOp)
LogicalResult acir::InstanceOp::verify() {
  if (!isa_and_nonnull<ModuleOp>(resolve(*this, getTarget()))) return emitOpError("requires a Module target");
  return success();
}
LogicalResult acir::InterfaceOp::verify() {
  for (auto target : getImplementations()) {
    auto symbol = dyn_cast<SymbolRefAttr>(target);
    if (!symbol || !isa_and_nonnull<MethodOp>(resolve(*this, symbol))) return emitOpError("implementation must reference a Method");
  }
  for (auto order : getOrders()) {
    auto data = dyn_cast<DictionaryAttr>(order);
    auto before = data ? data.getAs<SymbolRefAttr>("before") : SymbolRefAttr();
    auto *target = before ? resolve(*this, before) : nullptr;
    if (!isa_and_nonnull<InterfaceOp>(target) || target->getParentOp() != (*this)->getParentOp()) return emitOpError("order must reference an interface of the same Module");
  }
  return success();
}
LogicalResult acir::WorkOp::verify() {
  if (!isa<ModuleOp>((*this)->getParentOp())) return emitOpError("Work must belong to a Module scope");
  if (getBody().empty() || getBody().front().getNumArguments()) return emitOpError("Work requires an entry block without arguments");
  return success();
}
LogicalResult acir::WorkEndOp::verify() {
  if (!(*this)->getParentOfType<WorkOp>()) return emitOpError("must terminate Work");
  return success();
}
LogicalResult acir::GuardYieldOp::verify() {
  auto *parent = (*this)->getParentOp();
  if (!isa<RuleOp, MethodOp>(parent) || (*this)->getParentRegion() != &parent->getRegion(0)) return emitOpError("must terminate a Rule or Method guard");
  return success();
}
LogicalResult acir::RuleOp::verify() { return definition(*this); }
LogicalResult acir::MethodOp::verify() { return definition(*this); }
LogicalResult acir::RuleCallOp::verify() { return call(*this, true); }
LogicalResult acir::MethodCallOp::verify() { return call(*this, false); }
LogicalResult acir::GetOp::verify() {
  auto *target = resolve(*this, getTarget());
  if (!target) return emitOpError("unknown resource symbol ") << getTarget();
  auto kind = target->getName().getStringRef();
  auto type = getReference().getType();
  auto declared = target->getAttrOfType<TypeAttr>("payload_type");
  if (!declared || declared.getValue() != payload(type)) return emitOpError("reference payload must match declaration");
  if ((isa<QueueType>(type) && kind == "acir.queue") || (isa<CellType>(type) && kind == "acir.cell") || (isa<SignalType>(type) && kind == "acir.signal") || (isa<EHRType>(type) && kind == "acir.ehr") || (isa<QueueArrayType>(type) && kind == "acir.queue.array")) return success();
  return emitOpError("reference type must match declaration kind");
}
LogicalResult acir::ReturnOp::verify() {
  if (auto rule = (*this)->getParentOfType<RuleOp>()) {
    auto slots = (*this)->getAttrOfType<ArrayAttr>("slots");
    auto types = rule->getAttrOfType<ArrayAttr>("output_types");
    if (!slots || slots.size() != getValues().size() || !types) return emitOpError("requires one output slot per payload");
    llvm::SmallDenseSet<int64_t> seen;
    for (auto [index, slot] : llvm::enumerate(slots)) {
      auto number = dyn_cast<IntegerAttr>(slot);
      if (!number || number.getInt() < 0 || number.getInt() >= (int64_t)types.size() || !seen.insert(number.getInt()).second) return emitOpError("invalid or duplicate return slot");
      auto ref = dyn_cast<TypeAttr>(types[number.getInt()]);
      if (!ref || payload(ref.getValue()) != getValues()[index].getType()) return emitOpError("payload must match output slot type");
    }
    return success();
  }
  if (auto method = (*this)->getParentOfType<MethodOp>()) {
    auto signature = cast<FunctionType>(method.getFunctionType());
    if (getValues().getTypes() != signature.getResults()) return emitOpError("return must match method result types");
    return success();
  }
  return emitOpError("must terminate a Rule or Method body");
}
#define READ_VERIFY(CLASS) LogicalResult acir::CLASS::verify() { if (failed(accessMetadata(*this))) return failure(); if (payload(getResource().getType()) != getValue().getType()) return emitOpError("result must match component payload"); return success(); }
READ_VERIFY(QueuePeekOp)
READ_VERIFY(CellReadOp)
READ_VERIFY(EHRReadOp)
READ_VERIFY(SignalGetOp)
#define QUERY_VERIFY(CLASS) LogicalResult acir::CLASS::verify() { return success(); }
QUERY_VERIFY(QueueEmptyOp)
QUERY_VERIFY(QueueFullOp)
QUERY_VERIFY(QueueSizeOp)
QUERY_VERIFY(CapacityReadOp)
#define WRITE_VERIFY(CLASS) LogicalResult acir::CLASS::verify() { if (failed(accessMetadata(*this))) return failure(); if (payload(getResource().getType()) != getValue().getType()) return emitOpError("value must match component payload"); return success(); }
WRITE_VERIFY(PushOp)
WRITE_VERIFY(CellWriteOp)
WRITE_VERIFY(EHRWriteOp)
WRITE_VERIFY(SignalSetOp)
LogicalResult acir::CapacityWriteOp::verify() { return success(getValue().getType().isInteger(1)); }
LogicalResult acir::ExportOp::verify() { if (getInput().getType() != getValue().getType()) return emitOpError("export must preserve its input type"); return success(); }

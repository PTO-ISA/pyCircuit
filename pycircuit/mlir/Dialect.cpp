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
    if (name.ends_with(".write") || name == "acir.signal.set" || name == "acir.queue.pop" || name == "acir.queue.push") {
      nested->emitOpError("guard must only observe state"); invalid = true;
    }
    if (auto yield = dyn_cast<acir::GuardYieldOp>(nested))
      if (!yield.getCondition().getType().isInteger(1)) {
        yield.emitOpError("guard yields one bool"); invalid = true;
      }
  });
  return failure(invalid);
}
LogicalResult call(Operation *op) {
  auto *def = resolve(op, cast<SymbolRefAttr>(op->getAttr("target")));
  if (!isa_and_nonnull<acir::RuleOp>(def)) return op->emitOpError("unknown Rule target");
  auto metadata=def->getAttrOfType<DictionaryAttr>("acir.function");
  if(auto publicFlag=metadata.getAs<BoolAttr>("public"))if(!publicFlag.getValue()&&op->getParentOfType<acir::ModuleOp>()!=def->getParentOfType<acir::ModuleOp>())return op->emitOpError("cannot call a private Rule from another Module");
  auto signature = cast<FunctionType>(cast<TypeAttr>(def->getAttr("function_type")).getValue());
  if (op->getOperandTypes() != signature.getInputs() || op->getResultTypes() != signature.getResults()) return op->emitOpError("call signature mismatch");
  return success();
}
}
namespace {
LogicalResult declaration(Operation *op) {
  auto name = op->getName().getStringRef();
  if (name == "acir.cell" || name == "acir.ehr") {
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
LogicalResult acir::BodyOp::verify() {
  if (!isa<ModuleOp>((*this)->getParentOp()) || getBody().empty() || getBody().front().getNumArguments()) return emitOpError("Module body needs an entry without arguments");
  return success();
}
LogicalResult acir::BodyEndOp::verify() { return success(); }
LogicalResult acir::GuardYieldOp::verify() {
  auto *parent = (*this)->getParentOp();
  return success(isa<RuleOp>(parent) && (*this)->getParentRegion() == &parent->getRegion(0));
}
LogicalResult acir::RuleOp::verify() { return definition(*this); }
LogicalResult acir::RuleCallOp::verify() { return call(*this); }
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
  auto rule = (*this)->getParentOfType<RuleOp>();
  if (!rule) return emitOpError("must return from a Rule");
  return success(getValues().getTypes() == cast<FunctionType>(rule.getFunctionType()).getResults());
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
#define WRITE_VERIFY(CLASS) LogicalResult acir::CLASS::verify() { if (failed(accessMetadata(*this))) return failure(); if (payload(getResource().getType()) != getValue().getType()) return emitOpError("value must match component payload"); return success(); }
WRITE_VERIFY(PushOp)
WRITE_VERIFY(CellWriteOp)
WRITE_VERIFY(EHRWriteOp)
WRITE_VERIFY(SignalSetOp)
LogicalResult acir::ExportOp::verify() { if (getInput().getType() != getValue().getType()) return emitOpError("export must preserve its input type"); return success(); }

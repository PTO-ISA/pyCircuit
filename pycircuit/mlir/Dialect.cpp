#include "Dialect.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/DialectImplementation.h"
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
LogicalResult acir::ResourceOp::verify() {
  if (getKind() != "queue" && getKind() != "qarray" && getKind() != "cell" && getKind() != "signal")
    return emitOpError("kind must be queue, qarray, cell or signal");
  if (auto visibility = (*this)->getAttrOfType<StringAttr>("visibility")) {
    if (visibility.getValue() != "old" && visibility.getValue() != "ehr" && visibility.getValue() != "current")
      return emitOpError("visibility must be old, ehr or current");
    if (visibility.getValue() == "ehr" && getKind() != "cell")
      return emitOpError("EHR declarations use Cell references");
    if (visibility.getValue() == "current" && getKind() != "signal")
      return emitOpError("current visibility requires a Signal");
  }
  if (getKind() == "signal") {
    auto view = (*this)->getAttrOfType<StringAttr>("visibility");
    if (!view || view.getValue() != "current" || !(*this)->getAttrOfType<StringAttr>("initial_fn") || (*this)->hasAttr("capacity_fn"))
      return emitOpError("Signal requires current visibility, initialization and no capacity");
  }
  return success();
}
LogicalResult acir::ReadOp::verify() {
  if (isa<SignalType>(getResource().getType())) {
    auto view = (*this)->getAttrOfType<StringAttr>("view");
    auto interface = (*this)->getAttrOfType<StringAttr>("interface");
    if (!view || view.getValue() != "current" || !interface || interface.getValue() != "get" || (*this)->hasAttr("aspect") || (*this)->hasAttr("port") || (*this)->hasAttr("ports"))
      return emitOpError("Signal requires current view, get interface and no EHR ports");
  }

  if (auto view = (*this)->getAttrOfType<StringAttr>("view")) {
    if (view.getValue() == "current" && !isa<SignalType>(getResource().getType()))
      return emitOpError("current view requires a Signal");
    if (view.getValue() != "old" && view.getValue() != "ehr" && view.getValue() != "current")
      return emitOpError("reads require old state view, ehr or current; recompile obsolete Queue bypass artifacts");
  }
  if (auto aspect = (*this)->getAttrOfType<StringAttr>("aspect")) {
    if (aspect.getValue() != "full" || !isa<QueueType>(getResource().getType()) ||
        !getValue().getType().isInteger(1) || !(*this)->getAttrOfType<StringAttr>("view") || (*this)->getAttrOfType<StringAttr>("view").getValue() != "ehr")
      return emitOpError("Queue capacity view requires ehr full and bool payload");
    return success();
  }
  if (isa<QueueType>(getResource().getType()))
    if (auto view = (*this)->getAttrOfType<StringAttr>("view"))
      if (view.getValue() != "old")
        return emitOpError("Queue payload reads require the old state view");
  Type payload =
      isa<QueueType>(getResource().getType())
          ? cast<QueueType>(getResource().getType()).getElementType()
          : isa<SignalType>(getResource().getType()) ? cast<SignalType>(getResource().getType()).getElementType() : cast<CellType>(getResource().getType()).getElementType();
  if (getValue().getType() != payload)
    return emitOpError("result must match resource payload type");
  return success();
}
LogicalResult acir::WriteOp::verify() {
  if (auto view = (*this)->getAttrOfType<StringAttr>("view"))
    if (view.getValue() == "current" && !isa<SignalType>(getResource().getType()))
      return emitOpError("current view requires a Signal");
  if (isa<SignalType>(getResource().getType())) {
    auto view = (*this)->getAttrOfType<StringAttr>("view");
    auto interface = (*this)->getAttrOfType<StringAttr>("interface");
    if (!view || view.getValue() != "current" || !interface || interface.getValue() != "set" || (*this)->hasAttr("aspect") || (*this)->hasAttr("port") || (*this)->hasAttr("ports"))
      return emitOpError("Signal requires current view, set interface and no EHR ports");
  }

  if (isa<QueueType>(getResource().getType())) {
    auto aspect = (*this)->getAttrOfType<StringAttr>("aspect");
    auto view = (*this)->getAttrOfType<StringAttr>("view");
    if (!aspect || aspect.getValue() != "full" || !view || view.getValue() != "ehr" || !getValue().getType().isInteger(1))
      return emitOpError("Queue capacity write requires ehr full and bool payload");
    return success();
  }
  Type payload = isa<SignalType>(getResource().getType()) ? cast<SignalType>(getResource().getType()).getElementType() : cast<CellType>(getResource().getType()).getElementType();
  if (getValue().getType() != payload)
    return emitOpError("written value must match resource payload type");
  return success();
}
LogicalResult acir::QueryOp::verify() {
  if (getKind() == "full")
    return emitOpError("full requires an EHR capacity read; recompile obsolete full query artifacts");
  if (getKind() != "empty" && getKind() != "size")
    return emitOpError("unknown Queue query");
  if (!getValue().getType().isInteger(getKind() == "size" ? 32 : 1))
    return emitOpError("query has incorrect result width");
  return success();
}
LogicalResult acir::PushOp::verify() {
  if (getValue().getType() !=
      cast<QueueType>(getResource().getType()).getElementType())
    return emitOpError("pushed value must match Queue payload type");
  return success();
}
LogicalResult acir::ReviseOp::verify() {
  unsigned indices = 0;
  for (auto p : getPath()) {
    if (isa<UnitAttr>(p))
      ++indices;
    else if (!isa<StringAttr>(p))
      return emitOpError("path components must be fields or dynamic indices");
  }
  if (indices != getIndices().size())
    return emitOpError("path/index count mismatch");
  if (getPath().empty() &&
      getValue().getType() !=
          cast<QueueType>(getResource().getType()).getElementType())
    return emitOpError("whole-value revise must match Queue payload type");
  return success();
}

LogicalResult acir::ExportOp::verify() {
  if (getInput().getType() != getValue().getType())
    return emitOpError("export must preserve its input type");
  return success();
}

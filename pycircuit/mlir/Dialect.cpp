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
  if (getKind() != "queue" && getKind() != "qarray" && getKind() != "signal")
    return emitOpError("kind must be queue, qarray or signal");
  return success();
}
LogicalResult acir::ReadOp::verify() {
  Type payload =
      isa<QueueType>(getResource().getType())
          ? cast<QueueType>(getResource().getType()).getElementType()
          : cast<SignalType>(getResource().getType()).getElementType();
  if (getValue().getType() != payload)
    return emitOpError("result must match resource payload type");
  return success();
}
LogicalResult acir::QueryOp::verify() {
  if (getKind() != "empty" && getKind() != "full" && getKind() != "size")
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

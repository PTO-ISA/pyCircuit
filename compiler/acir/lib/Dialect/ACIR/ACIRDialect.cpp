#include "acir/Dialect/ACIR/ACIRDialect.h"

#include "ACIRFinalRecordUses.h"
#include "acir/Dialect/ACIR/ACIRDialect.cpp.inc"

#include "mlir/IR/BuiltinOps.h"

using namespace mlir;

namespace acir::ac {

LogicalResult ACIRDialect::verifyOperationAttribute(Operation *operation,
                                                    NamedAttribute attribute) {
  if (attribute.getName().getValue() != "ac.required_records")
    return success();
  auto rule = dyn_cast_or_null<RuleOp>(operation);
  if (!operation)
    return failure();
  if (!rule)
    return operation->emitError()
           << "ac.required_records is only valid on ac.rule";
  auto unit = operation->getParentOfType<mlir::ModuleOp>();
  auto stage =
      unit ? unit->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  if (!unit || !stage ||
      (stage.getValue() != "linked" && stage.getValue() != "final"))
    return operation->emitError()
           << "ac.required_records requires a linked or final semantic unit";
  if (stage.getValue() == "linked")
    return operation->emitError()
           << "linked record verification is not admitted in this phase";
  if (!hasFinalRecordUses(rule))
    return operation->emitError()
           << "final record verification requires ac.required_records";
  return verifyFinalRecordUses(rule);
}

} // namespace acir::ac

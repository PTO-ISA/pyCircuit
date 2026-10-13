#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

#include "pycircuit/Dialect/ACIR/ACIRDialect.cpp.inc"

using namespace mlir;

namespace acir::ac {

LogicalResult ACIRDialect::verifyOperationAttribute(Operation *operation,
                                                    NamedAttribute attribute) {
  StringRef name = attribute.getName().getValue();
  if (name != "ac.sampled_admission" && name != "ac.transaction_wiring")
    return success();
  auto package = operation->getParentOfType<mlir::ModuleOp>();
  auto stage =
      package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  bool placement = name == "ac.sampled_admission" ? isa<RuleOp>(operation)
                                                  : isa<ModuleOp>(operation);
  if (!placement || !stage || stage.getValue() != "final")
    return operation->emitOpError()
           << name << " is permitted only on its owning "
           << (name == "ac.sampled_admission" ? "rule" : "module")
           << " in a final package";
  if (!isa<DictionaryAttr>(attribute.getValue()))
    return operation->emitOpError() << name << " must be a dictionary";
  return success();
}

} // namespace acir::ac

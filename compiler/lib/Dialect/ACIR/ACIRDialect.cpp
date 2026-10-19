#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

#include "pycircuit/Dialect/ACIR/ACIRDialect.cpp.inc"

using namespace mlir;

namespace acir::ac {

LogicalResult ACIRDialect::verifyOperationAttribute(Operation *operation,
                                                    NamedAttribute attribute) {
  StringRef name = attribute.getName().getValue();
  if (name == "ac.source_memory") {
    if (!isa<InstanceOp>(operation) ||
        !isa_and_nonnull<ModuleOp>(operation->getParentOp()) ||
        !isa<UnitAttr>(attribute.getValue()))
      return operation->emitOpError()
             << "ac.source_memory is a source Memory allocation unit marker";
    return success();
  }
  if (name == "ac.system_samples") {
    auto role = operation->getAttrOfType<StringAttr>("ac.root_kind");
    if (!isa<ModuleOp, ModuleImportOp>(operation) || !role ||
        role.getValue() != "system" ||
        !isa_and_nonnull<mlir::ModuleOp>(operation->getParentOp()))
      return operation->emitOpError()
             << "ac.system_samples belongs only to a System declaration";
    return success();
  }
  if (name == "ac.execution_policy") {
    auto package = dyn_cast_or_null<mlir::ModuleOp>(operation->getParentOp());
    auto stage =
        package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
    if (!isa<SystemOp>(operation) || !package ||
        (stage && stage.getValue() != "final") ||
        package->hasAttr("ac.unit_kind"))
      return operation->emitOpError()
             << "ac.execution_policy is permitted only on the root ac.system "
                "in an executable package";
    return success();
  }
  if (name == "ac.registration_entry") {
    auto package = operation->getParentOfType<mlir::ModuleOp>();
    auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage")
                         : StringAttr();
    if (!isa<SourceTransactionOp>(operation) ||
        !isa<UnitAttr>(attribute.getValue()) ||
        (stage && stage.getValue() == "final"))
      return operation->emitOpError()
             << "ac.registration_entry is a source transaction unit marker";
    return success();
  }
  if (name != "ac.sampled_admission" && name != "ac.transaction_wiring" && name != "ac.queue_wiring")
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

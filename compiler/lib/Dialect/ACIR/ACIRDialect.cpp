#include "pycircuit/Dialect/ACIR/ACIRDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"

#include "pycircuit/Dialect/ACIR/ACIRDialect.cpp.inc"

using namespace mlir;

namespace acir::ac {

LogicalResult ACIRDialect::verifyOperationAttribute(Operation *operation,
                                                    NamedAttribute attribute) {
  StringRef name = attribute.getName().getValue();
  if (name == "ac.anchor_provenance") {
    auto package = dyn_cast<mlir::ModuleOp>(operation);
    auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage")
                         : StringAttr();
    auto record = dyn_cast<DictionaryAttr>(attribute.getValue());
    auto isDigest = [](Attribute value) {
      auto text = dyn_cast_or_null<StringAttr>(value);
      if (!text || text.getValue().size() != 64)
        return false;
      for (char byte : text.getValue())
        if (!((byte >= '0' && byte <= '9') || (byte >= 'a' && byte <= 'f')))
          return false;
      return true;
    };
    if (!package || package->hasAttr("ac.unit_kind") || !stage ||
        stage.getValue() != "final" || !record || record.size() != 2 ||
        !isDigest(record.get("content_identity")) ||
        !isDigest(record.get("configuration_identity")))
      return operation->emitOpError()
             << "ac.anchor_provenance requires exactly content_identity and "
                "configuration_identity lowercase SHA256 strings on the final "
                "package";
    // Like the routed marker, root placement is checked after parser wrappers
    // have been removed. These identities are provenance, never proof authority.
    return success();
  }
  if (name == "ac.routed_contract") {
    auto package = dyn_cast<mlir::ModuleOp>(operation);
    auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage")
                         : StringAttr();
    auto version = dyn_cast<IntegerAttr>(attribute.getValue());
    // Assembly parsing verifies while the parsed package still has a temporary
    // container. Root placement belongs to the independent package verifier's
    // closed walk, which rejects markers on every retained non-root operation.
    if (!package ||
        package->hasAttr("ac.unit_kind") || !stage ||
        (stage.getValue() != "routed" && stage.getValue() != "final") ||
        !version || !version.getType().isSignlessInteger(64) ||
        version.getValue() != 2)
      return operation->emitOpError()
             << "ac.routed_contract requires version 2 : i64 on the top-level "
                "routed or final package; regenerate unsupported artifacts";
    return success();
  }
  if (name == "ac.external_contract") {
    auto imported = dyn_cast<ModuleImportOp>(operation);
    if (!imported || imported->hasAttr("primitive_kind") ||
        imported.getSymName().starts_with("pycircuit.__builtins__."))
      return operation->emitOpError()
             << "ac.external_contract belongs only to an ordinary module import";
    return success();
  }
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
        (stage && stage.getValue() != "final" && stage.getValue() != "routed") ||
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
        (stage && (stage.getValue() == "final" || stage.getValue() == "routed")))
      return operation->emitOpError()
             << "ac.registration_entry is a source transaction unit marker";
    return success();
  }
  bool arrayLayout = name == "ac.input_validity" ||
                     name == "ac.output_validity" ||
                     name == "ac.routed_output_validity";
  if (name != "ac.sampled_admission" && name != "ac.transaction_wiring" &&
      name != "ac.queue_wiring" && name != "ac.internal_ports" && !arrayLayout)
    return success();
  auto package = operation->getParentOfType<mlir::ModuleOp>();
  auto stage =
      package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  bool placement = name == "ac.sampled_admission" ? isa<RuleOp>(operation)
                                                  : isa<ModuleOp>(operation);
  if (!placement || !stage ||
      (stage.getValue() != "routed" && stage.getValue() != "final") ||
      package->hasAttr("ac.unit_kind"))
    return operation->emitOpError()
           << name << " is permitted only on its owning "
           << (name == "ac.sampled_admission" ? "rule" : "module")
           << " in a routed or final package";
  if (arrayLayout ? !isa<ArrayAttr>(attribute.getValue())
                  : !isa<DictionaryAttr>(attribute.getValue()))
    return operation->emitOpError()
           << name << (arrayLayout ? " must be an array" : " must be a dictionary");
  return success();
}

} // namespace acir::ac

#include "ACIRHardwareClosure.h"

#include "ACIRHardwareClosureDetail.h"

#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::ac::hardware_detail {

FailureOr<DictionaryAttr> specKey(Attribute raw, Operation *owner) {
  auto value = dyn_cast<DictionaryAttr>(raw);
  if (!value || failed(detail::verifySpecKey(
                    value, [&] { return owner->emitOpError(); })))
    return owner->emitOpError() << "final hardware SpecKey is malformed";
  return value;
}

DictionaryAttr ownedRef(Builder &builder, RegOp reg) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("owned")),
      builder.getNamedAttr("declaration", reg->getAttr("ac.declaration")),
      builder.getNamedAttr("element", reg->getAttr("ac.element")),
  });
}

DictionaryAttr formalRef(Builder &builder, DictionaryAttr port) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("formal")),
      builder.getNamedAttr("parameter", port.get("parameter")),
      builder.getNamedAttr("ordinal", port.get("ordinal")),
  });
}

DictionaryAttr stateID(Builder &builder, DictionaryAttr owner,
                       DictionaryAttr state) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("owner", owner),
      builder.getNamedAttr("declaration", state.get("declaration")),
      builder.getNamedAttr("element", state.get("element")),
  });
}

LogicalResult inspectEnvelope(Closure &closure,
                              ac::detail::EmitError emitError) {
  mlir::ModuleOp package = closure.package;
  auto stage = package->getAttrOfType<StringAttr>("ac.stage");
  auto entry = specKey(package->getAttr("ac.entry"), package);
  auto rows = package->getAttrOfType<ArrayAttr>("ac.instance_bindings");
  if (!stage || stage.getValue() != "final" || failed(entry) || !rows ||
      package->getAttrs().size() != 3 || package->hasAttr("ac.unit_kind") ||
      package.getBody()->empty())
    return emitError() << "final hardware package envelope is not canonical";
  closure.entry = *entry;
  closure.rows = rows;

  Builder builder(package.getContext());
  DictionaryAttr previousOwner;
  for (Operation &nested : package.getBody()->getOperations()) {
    if (nested.getName().getStringRef() == "ac.system") {
      if (closure.system)
        return emitError() << "final hardware package repeats ac.system";
      closure.system = &nested;
      continue;
    }
    auto unit = dyn_cast<mlir::ModuleOp>(nested);
    auto unitStage =
        unit ? unit->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
    auto kind =
        unit ? unit->getAttrOfType<StringAttr>("ac.unit_kind") : StringAttr();
    auto owner = unit ? unit->getAttrOfType<DictionaryAttr>("ac.source_owner")
                      : DictionaryAttr();
    if (!unit || !unitStage || unitStage.getValue() != "final" || !kind ||
        kind.getValue() != "implementation" ||
        failed(detail::verifySourceOwner(owner, emitError)) ||
        unit->getAttrs().size() != 3 ||
        (previousOwner &&
         detail::compareClosedSourceStructure(previousOwner, owner) >= 0))
      return emitError()
             << "final package child is not a source-owned implementation";
    previousOwner = owner;
    SmallVector<ModuleOp> modules(unit.getBody()->getOps<ModuleOp>());
    if (modules.size() != 1)
      return emitError()
             << "final implementation unit must own one module definition";
    ModuleOp module = modules.front();
    auto definition =
        FlatSymbolRefAttr::get(package.getContext(), module.getSymName());
    auto key = builder.getDictionaryAttr({
        builder.getNamedAttr("definition", definition),
        builder.getNamedAttr("arguments", builder.getArrayAttr({})),
    });
    if (!closure.definitions.try_emplace(key, Definition{module, key}).second)
      return emitError() << "final package repeats a module definition";
  }
  if (!closure.system || closure.definitions.empty())
    return emitError() << "final package lacks system or definitions";
  Operation *system = closure.system;
  auto systemEntry = specKey(system->getAttr("entry"), system);
  auto owner = system->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto origin = system->getAttrOfType<DictionaryAttr>("ac.origin");
  auto domain = system->getAttrOfType<StringAttr>("domain");
  if (system->getNumOperands() || system->getNumResults() ||
      system->getNumRegions() || system->getAttrs().size() != 4 ||
      failed(systemEntry) || *systemEntry != closure.entry ||
      failed(detail::verifySourceOwner(owner, emitError)) ||
      failed(detail::verifyOccurrence(origin, emitError)) || !domain ||
      domain.getValue() != "default")
    return emitError() << "ac.system descriptor is not canonical";
  return success();
}

} // namespace acir::ac::hardware_detail

namespace acir::ac {

LogicalResult verifyFinalHardware(mlir::ModuleOp package) {
  if (!package)
    return failure();
  auto emit = [&] { return package.emitError(); };
  hardware_detail::Closure closure;
  closure.package = package;
  if (failed(hardware_detail::inspectEnvelope(closure, emit)) ||
      failed(hardware_detail::rebuildInstances(closure, emit)) ||
      failed(hardware_detail::verifyInstanceRows(closure, emit)) ||
      failed(hardware_detail::verifyCommits(closure, emit)))
    return failure();
  return success();
}

} // namespace acir::ac

#include "EntryBoundary.h"

#include "llvm/ADT/STLExtras.h"

using namespace mlir;
namespace acir::ac::detail {
namespace {
bool entryShape(DictionaryAttr entry) {
  if (!entry || entry.size() != 3 || !entry.getAs<FlatSymbolRefAttr>("callee") ||
      !entry.getAs<ArrayAttr>("parameters") || !entry.getAs<ArrayAttr>("type_arguments") ||
      entry.getAs<ArrayAttr>("parameters").size() > HardwareSourceCheckLimits{}.maxWorkUnits ||
      entry.getAs<ArrayAttr>("type_arguments").size() > HardwareSourceCheckLimits{}.maxWorkUnits) return false;
  return llvm::all_of(entry.getAs<ArrayAttr>("parameters"), [](Attribute a) { return isa<StaticExprAttr>(a); }) &&
      llvm::all_of(entry.getAs<ArrayAttr>("type_arguments"), [](Attribute a) { auto t = dyn_cast<TypeAttr>(a); return t && isa<BitsType, StructType>(t.getValue()); });
}
}
LogicalResult verifyEntryBoundaryAttribute(Operation *operation, Attribute attribute) {
  auto system = dyn_cast<SystemOp>(operation);
  auto package = dyn_cast_or_null<mlir::ModuleOp>(operation->getParentOp());
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto record = dyn_cast<DictionaryAttr>(attribute);
  auto version = record ? record.getAs<IntegerAttr>("version") : IntegerAttr();
  if (!system || !package || package->hasAttr("ac.unit_kind") ||
      (stage ? stage.getValue() != "routed" && stage.getValue() != "final"
             : !package->getAttrOfType<ArrayAttr>("ac.source_units")) ||
      !record || record.size() != 2 || !version || !version.getType().isSignlessInteger(64) ||
      version.getInt() != 1 || !entryShape(record.getAs<DictionaryAttr>("logical_entry")))
    return operation->emitOpError("ac.entry_boundary requires exactly version=1 : i64 and logical_entry on the executable System");
  if (stage) {
    auto contract = package->getAttrOfType<IntegerAttr>("ac.routed_contract");
    if (!contract || !contract.getType().isSignlessInteger(64) ||
        contract.getInt() != 5)
      return operation->emitOpError("ac.entry_boundary requires ac.routed_contract = 5 : i64 in routed and final packages");
  }
  return success();
}

} // namespace acir::ac::detail

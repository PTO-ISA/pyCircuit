#include "ACIRSourceContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"

#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::ac {

FailureOr<SmallVector<FinalSourceUnitView>>
collectFinalSourceUnits(mlir::ModuleOp package, detail::EmitError emitError) {
  if (!package)
    return emitError() << "final source-unit collection requires a package";

  Attribute rawInventory = package->getAttr("ac.source_units");
  ArrayAttr inventory;
  if (rawInventory) {
    inventory = dyn_cast<ArrayAttr>(rawInventory);
    if (!inventory)
      return emitError() << "ac.source_units must be an ArrayAttr";
  }

  SmallVector<FinalSourceUnitView> units;
  llvm::DenseMap<Attribute, unsigned> ownerIndices;
  DictionaryAttr previous;
  if (inventory) {
    for (auto [index, rawOwner] : llvm::enumerate(inventory)) {
      auto owner = dyn_cast<DictionaryAttr>(rawOwner);
      if (!owner || failed(detail::verifySourceOwner(owner, emitError)))
        return emitError() << "ac.source_units[" << index
                           << "] must be a valid SourceOwner dictionary";
      if (!ownerIndices.try_emplace(owner, units.size()).second)
        return emitError()
               << "ac.source_units contains a duplicate SourceOwner";
      if (previous &&
          detail::compareClosedSourceStructure(previous, owner) >= 0)
        return emitError() << "ac.source_units must be sorted by UTF-8 "
                              "(package, path) bytes";
      units.push_back({owner, {}});
      previous = owner;
    }
  }

  // Final provenance does not establish source role, qualified identity or
  // historical provider authority. Those belong to admitted body/header pairs.
  // Module imports, including canonical primitives, are never owner
  // definitions.
  for (Operation &operation : package.getBody()->getOperations()) {
    bool module = isa<ac::ModuleOp>(operation);
    if (!module && !isa<StructOp, EnumOp>(operation))
      continue;
    Attribute rawOwner =
        operation.getAttr(module ? "source_owner" : "ac.source_owner");
    if (!rawOwner && !module)
      continue;
    auto owner = dyn_cast_or_null<DictionaryAttr>(rawOwner);
    if (!owner || failed(detail::verifySourceOwner(owner, emitError)))
      return emitError() << "final declaration " << operation.getName()
                         << " requires a valid SourceOwner dictionary in "
                         << (module ? "source_owner" : "ac.source_owner");
    auto found = ownerIndices.find(owner);
    if (found == ownerIndices.end()) {
      if (inventory)
        return emitError() << "final declaration SourceOwner is absent from "
                              "ac.source_units";
      unsigned index = units.size();
      ownerIndices.try_emplace(owner, index);
      units.push_back({owner, {}});
      found = ownerIndices.find(owner);
    }
    units[found->second].declarations.push_back(&operation);
  }

  llvm::sort(units, [](const FinalSourceUnitView &left,
                       const FinalSourceUnitView &right) {
    return detail::compareClosedSourceStructure(left.owner, right.owner) < 0;
  });
  for (FinalSourceUnitView &unit : units)
    llvm::stable_sort(unit.declarations, [](Operation *left, Operation *right) {
      auto leftName = SymbolTable::getSymbolName(left);
      auto rightName = SymbolTable::getSymbolName(right);
      return (leftName ? leftName.getValue() : StringRef()) <
             (rightName ? rightName.getValue() : StringRef());
    });
  return units;
}

} // namespace acir::ac

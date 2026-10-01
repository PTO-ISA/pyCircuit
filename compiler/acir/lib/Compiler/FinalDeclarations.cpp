#include "FinalDeclarations.h"

#include "Dialect/ACIR/ACIRFinalDeclarations.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"

using namespace mlir;

namespace acir::compiler {
namespace {

int compareBytes(StringRef left, StringRef right) {
  size_t count = std::min(left.size(), right.size());
  for (size_t index = 0; index < count; ++index) {
    auto lhs = static_cast<unsigned char>(left[index]);
    auto rhs = static_cast<unsigned char>(right[index]);
    if (lhs != rhs)
      return lhs < rhs ? -1 : 1;
  }
  return left.size() == right.size() ? 0 : left.size() < right.size() ? -1 : 1;
}

StringRef symbolName(Operation *operation) {
  auto name = operation ? operation->getAttrOfType<StringAttr>(
                              SymbolTable::getSymbolAttrName())
                        : StringAttr();
  return name ? name.getValue() : StringRef();
}

} // namespace

FailureOr<SmallVector<FinalDeclarationProjection, 0>>
projectFinalDeclarations(ArrayRef<SourceLinkUnit> units,
                         const SourceHeaderRegistry &registry,
                         ac::detail::EmitError emitError) {
  DenseSet<Attribute> owners;
  DenseSet<Attribute> caseFoldedOwners;
  llvm::StringMap<DictionaryAttr> ownerByImportModule;
  SmallVector<FinalDeclarationProjection, 0> result;
  result.reserve(units.size());

  for (SourceLinkUnit pair : units) {
    if (!pair.body || !pair.header)
      return emitError()
             << "final declaration projection requires complete source units";
    auto bodyOwner =
        pair.body->getAttrOfType<DictionaryAttr>("ac.source_owner");
    auto headerOwner =
        pair.header->getAttrOfType<DictionaryAttr>("ac.source_owner");
    auto bodyStage = pair.body->getAttrOfType<StringAttr>("ac.stage");
    auto headerStage = pair.header->getAttrOfType<StringAttr>("ac.stage");
    auto bodyKind = pair.body->getAttrOfType<StringAttr>("ac.unit_kind");
    auto headerKind = pair.header->getAttrOfType<StringAttr>("ac.unit_kind");
    if (failed(ac::detail::verifySourceOwner(bodyOwner, emitError)) ||
        failed(ac::detail::verifySourceOwner(headerOwner, emitError)) ||
        bodyOwner != headerOwner || !bodyStage ||
        bodyStage.getValue() != "source" || !headerStage ||
        headerStage.getValue() != "source" || !bodyKind ||
        (bodyKind.getValue() != "implementation" &&
         bodyKind.getValue() != "declarations") ||
        !headerKind || headerKind.getValue() != "interface")
      return emitError() << "source unit envelope changed before declaration "
                            "projection";
    if (!owners.insert(bodyOwner).second)
      return emitError() << "declaration projection repeats a SourceOwner";
    auto caseFoldedOwner =
        ac::final_detail::sourceOwnerCaseFoldIdentity(bodyOwner, emitError);
    if (failed(caseFoldedOwner))
      return failure();
    if (!caseFoldedOwners.insert(*caseFoldedOwner).second)
      return emitError()
             << "declaration projection repeats a case-folded SourceOwner";

    auto importModule =
        ac::final_detail::sourceImportModuleIdentity(bodyOwner, emitError);
    if (failed(importModule))
      return failure();
    if (!ownerByImportModule.try_emplace(*importModule, bodyOwner).second)
      return emitError()
             << "source units repeat a case-folded import-module identity";
    if (!llvm::is_contained(registry.suppliedHeaders(), pair.header))
      return emitError()
             << "declaration projection header is not in the admitted registry";

    SmallVector<Operation *> selected;
    for (Operation &operation : pair.header.getBody()->getOperations()) {
      auto role = operation.getAttrOfType<StringAttr>("ac.declaration_role");
      auto owner = operation.getAttrOfType<DictionaryAttr>("ac.source_owner");
      if (!role || role.getValue() != "definition")
        continue;
      if (owner != bodyOwner)
        return emitError()
               << "owning interface definition has a foreign SourceOwner";
      if (!isa<ac::TypeAliasOp, ac::ConstantOp, ac::StructOp>(operation))
        continue;

      auto canonical = isa<ac::StructOp>(operation)
                           ? ac::final_detail::verifyFinalRecordDeclaration(
                                 &operation, bodyOwner, emitError)
                           : ac::final_detail::verifyFinalScalarDeclaration(
                                 &operation, bodyOwner, emitError);
      if (failed(canonical))
        return emitError()
               << "owning interface contains an unsupported final declaration";
      if (registry.lookupDeclaration(*canonical) != &operation)
        return emitError()
               << "owning interface declaration is not canonical authority";
      selected.push_back(&operation);
    }
    llvm::sort(selected, [](Operation *left, Operation *right) {
      return compareBytes(symbolName(left), symbolName(right)) < 0;
    });

    auto staging =
        OwningOpRef<ModuleOp>(ModuleOp::create(pair.header.getLoc()));
    for (Operation *declaration : selected)
      staging->getBody()->push_back(declaration->clone());
    result.push_back({bodyOwner, bodyKind, std::move(staging)});
  }
  llvm::sort(result, [](const FinalDeclarationProjection &left,
                        const FinalDeclarationProjection &right) {
    return ac::detail::compareClosedSourceStructure(left.sourceOwner,
                                                    right.sourceOwner) < 0;
  });
  return result;
}

} // namespace acir::compiler

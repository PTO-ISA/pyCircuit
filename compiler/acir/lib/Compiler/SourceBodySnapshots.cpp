#include "SourceHeaderHelpers.h"
#include "SourceUnit.h"

#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler {

LogicalResult SourceHeaderRegistry::verifyBodySnapshots(
    ModuleOp body, ModuleOp owningHeader,
    ac::detail::EmitError emitError) const {
  if (!body || !owningHeader)
    return emitError() << "source body and owning header are both required";
  SmallVector<ModuleOp> completeHeaders(headers_.begin(), headers_.end());
  completeHeaders.push_back(owningHeader);
  auto authority = SourceHeaderRegistry::create(completeHeaders, emitError);
  if (failed(authority))
    return failure();

  auto owner = owningHeader->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto bodyOwner = body->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto bodyKind = body->getAttrOfType<StringAttr>("ac.unit_kind");
  auto stage = body->getAttrOfType<StringAttr>("ac.stage");
  if (!owner || bodyOwner != owner || !stage || stage.getValue() != "source" ||
      !bodyKind ||
      (bodyKind.getValue() != "implementation" &&
       bodyKind.getValue() != "declarations"))
    return emitError()
           << "source body envelope does not match its owning header";

  for (StringRef name : {"ac.interfaces", "ac.exports", "ac.import_bindings"})
    if (!body->getAttr(name) ||
        body->getAttr(name) != owningHeader->getAttr(name))
      return emitError() << "source body/header metadata differs for " << name;

  bool hasExecutableModule = false;
  for (Operation &operation : body.getBody()->getOperations()) {
    if (auto module = dyn_cast<ac::ModuleOp>(operation)) {
      if (hasExecutableModule || bodyKind.getValue() != "implementation")
        return emitError() << "implementation body must own exactly one module";
      hasExecutableModule = true;
      auto symbol =
          module->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
      auto canonical =
          symbol ? FlatSymbolRefAttr::get(body.getContext(), symbol.getValue())
                 : FlatSymbolRefAttr();
      Operation *declaration =
          canonical ? authority->lookupDeclaration(canonical) : nullptr;
      if (!declaration || !isa<ac::ModuleImportOp>(declaration) ||
          declaration->getAttrOfType<DictionaryAttr>("ac.source_owner") !=
              owner)
        return emitError()
               << "source module has no matching owning header declaration";
      continue;
    }

    auto role = operation.getAttrOfType<StringAttr>("ac.declaration_role");
    auto symbol =
        operation.getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
    auto snapshotOwner =
        operation.getAttrOfType<DictionaryAttr>("ac.source_owner");
    if (!role || role.getValue() != "import_snapshot" || !symbol ||
        !snapshotOwner)
      return emitError() << "source body contains an unverified declaration";
    auto canonical =
        FlatSymbolRefAttr::get(body.getContext(), symbol.getValue());
    Operation *definition = authority->lookupDeclaration(canonical);
    if (!definition ||
        definition->getAttrOfType<DictionaryAttr>("ac.source_owner") !=
            snapshotOwner ||
        !detail::sameDeclaration(definition, &operation))
      return emitError()
             << "source body import snapshot differs from its owning header: "
             << canonical;
  }
  if (hasExecutableModule != (bodyKind.getValue() == "implementation"))
    return emitError() << "source body kind disagrees with executable module";
  return success();
}

} // namespace acir::compiler

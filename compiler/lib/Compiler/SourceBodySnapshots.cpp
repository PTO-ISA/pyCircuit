#include "SourceHeaderHelpers.h"
#include "SourceUnit.h"
#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"

using namespace mlir;

namespace acir::compiler {
namespace {
LogicalResult verifyPublishedModule(ac::ModuleOp module,
                                    ac::ModuleImportOp declaration,
                                    ac::detail::EmitError emitError) {
  if (!declaration || module.getSourceOwner() != declaration.getSourceOwner() ||
      module.getParameters() != declaration.getParameters() ||
      module.getFunctionTypeAttr() != declaration.getFunctionTypeAttr() ||
      module.getTypeParameters() != declaration.getTypeParameters() ||
      module.getInputNames() != declaration.getInputNames() ||
      module.getOutputNames() != declaration.getOutputNames())
    return emitError()
           << "source module signature differs from its published interface: @"
           << module.getSymName();
  for (StringRef name : {"ac.root_kind", "ac.return_form", "ac.parameters",
                         "ac.result_constraints", "ac.domain_inputs"})
    if (module->getAttr(name) != declaration->getAttr(name))
      return emitError()
             << "source module call contract differs from its published "
                "interface: @"
             << module.getSymName();
  return verifyPublishedDependencies(module, declaration, emitError);
}

template <typename LookupDeclaration>
LogicalResult
verifyBodySnapshotsAgainstAuthority(ModuleOp body, ModuleOp owningHeader,
                                    LookupDeclaration lookupDeclaration,
                                    ac::detail::EmitError emitError) {
  if (!body || !owningHeader)
    return emitError() << "source body and owning header are both required";

  auto structure = ac::verifySourceBodyStructure(body, emitError);
  if (failed(structure))
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

  unsigned executableModules = 0;
  for (Operation &operation : body.getBody()->getOperations()) {
    if (auto module = dyn_cast<ac::ModuleOp>(operation)) {
      if (bodyKind.getValue() != "implementation")
        return emitError() << "declarations body cannot own ac.module";
      ++executableModules;
      auto symbol =
          module->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
      auto canonical =
          symbol ? FlatSymbolRefAttr::get(body.getContext(), symbol.getValue())
                 : FlatSymbolRefAttr();
      Operation *declaration =
          canonical ? lookupDeclaration(canonical) : nullptr;
      auto moduleDeclaration =
          dyn_cast_or_null<ac::ModuleImportOp>(declaration);
      auto declarationRole = moduleDeclaration
                                 ? moduleDeclaration->getAttrOfType<StringAttr>(
                                       "ac.declaration_role")
                                 : StringAttr();
      if (!moduleDeclaration ||
          ac::detail::declarationSourceOwner(declaration) != owner ||
          !declarationRole || declarationRole.getValue() != "definition")
        return emitError()
               << "source module has no matching owning header declaration";
      if (failed(verifyPublishedModule(module, moduleDeclaration, emitError)))
        return failure();
      continue;
    }

    auto role = operation.getAttrOfType<StringAttr>("ac.declaration_role");
    auto symbol =
        operation.getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
    auto snapshotOwner = ac::detail::declarationSourceOwner(&operation);
    bool localNominal = isa<ac::StructOp, ac::EnumOp>(operation) && role &&
                        role.getValue() == "definition" &&
                        snapshotOwner == owner;
    if (!role || (role.getValue() != "import_snapshot" && !localNominal) ||
        !symbol || !snapshotOwner)
      return emitError() << "source body contains an unverified declaration";
    auto canonical =
        FlatSymbolRefAttr::get(body.getContext(), symbol.getValue());
    Operation *definition = lookupDeclaration(canonical);
    if (!definition ||
        ac::detail::declarationSourceOwner(definition) != snapshotOwner ||
        !detail::sameDeclaration(definition, &operation))
      return emitError()
             << "source body declaration differs from its owning header: "
             << canonical;
  }
  if (bodyKind.getValue() == "implementation" && executableModules == 0)
    return emitError() << "implementation body must own at least one module";

  // Authority is bidirectional: a consumer's retained snapshot cannot stand
  // in for a nominal definition deleted from the actual owner's body.
  for (Operation &declaration : owningHeader.getBody()->getOperations()) {
    auto role = declaration.getAttrOfType<StringAttr>("ac.declaration_role");
    if (!isa<ac::StructOp, ac::EnumOp>(declaration) || !role ||
        role.getValue() != "definition" ||
        ac::detail::declarationSourceOwner(&declaration) != owner)
      continue;
    auto symbol = SymbolTable::getSymbolName(&declaration);
    Operation *definition = structure->declarations.lookup(symbol.getValue());
    auto bodyRole =
        definition
            ? definition->getAttrOfType<StringAttr>("ac.declaration_role")
            : StringAttr();
    if (!definition || !bodyRole || bodyRole.getValue() != "definition" ||
        ac::detail::declarationSourceOwner(definition) != owner ||
        !detail::sameDeclaration(&declaration, definition))
      return emitError()
             << "owning header nominal declaration has no matching body "
                "definition: "
             << symbol;
  }
  return success();
}

} // namespace

LogicalResult verifyIntrinsicSourceUnitPair(ModuleOp body, ModuleOp interface,
                                            ac::detail::EmitError emitError) {
  if (!body || !interface)
    return emitError() << "source body and interface are both required";

  auto owner = interface->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto bodyOwner = body->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto interfaceKind = interface->getAttrOfType<StringAttr>("ac.unit_kind");
  auto interfaceStage = interface->getAttrOfType<StringAttr>("ac.stage");
  auto bodyKind = body->getAttrOfType<StringAttr>("ac.unit_kind");
  auto bodyStage = body->getAttrOfType<StringAttr>("ac.stage");
  if (failed(ac::detail::verifySourceOwner(owner, emitError)) ||
      failed(ac::detail::verifySourceOwner(bodyOwner, emitError)) ||
      owner != bodyOwner)
    return emitError() << "source body/interface SourceOwner mismatch";
  if (!interfaceKind || interfaceKind.getValue() != "interface" ||
      !interfaceStage || interfaceStage.getValue() != "source")
    return emitError() << "source interface stage/unit_kind is invalid";
  if (!bodyKind ||
      (bodyKind.getValue() != "implementation" &&
       bodyKind.getValue() != "declarations") ||
      !bodyStage || bodyStage.getValue() != "source")
    return emitError() << "source body stage/unit_kind is invalid";
  for (StringRef name : {"ac.interfaces", "ac.exports", "ac.import_bindings"})
    if (!interface->getAttr(name) ||
        interface->getAttr(name) != body->getAttr(name))
      return emitError() << "source body/interface metadata differs for "
                         << name;

  auto structure = ac::verifySourceInterfaceStructure(interface, emitError);
  if (failed(structure))
    return failure();
  auto lookup = [&declarations =
                     structure->declarations](FlatSymbolRefAttr canonical) {
    if (!canonical)
      return static_cast<Operation *>(nullptr);
    auto found = declarations.find(canonical.getValue());
    return found == declarations.end() ? static_cast<Operation *>(nullptr)
                                       : found->second;
  };
  return verifyBodySnapshotsAgainstAuthority(body, interface, lookup,
                                             emitError);
}

LogicalResult
verifyIntrinsicSourceUnitOwnerPair(ModuleOp body, ModuleOp interface,
                                   bool &ownerMismatch,
                                   ac::detail::EmitError emitError) {
  ownerMismatch = false;
  if (!body || !interface)
    return emitError() << "source body and interface are both required";

  auto interfaceKind = interface->getAttrOfType<StringAttr>("ac.unit_kind");
  auto interfaceStage = interface->getAttrOfType<StringAttr>("ac.stage");
  auto bodyKind = body->getAttrOfType<StringAttr>("ac.unit_kind");
  auto bodyStage = body->getAttrOfType<StringAttr>("ac.stage");
  if (!interfaceKind || interfaceKind.getValue() != "interface" ||
      !interfaceStage || interfaceStage.getValue() != "source")
    return emitError() << "source interface stage/unit_kind is invalid";
  if (!bodyKind ||
      (bodyKind.getValue() != "implementation" &&
       bodyKind.getValue() != "declarations") ||
      !bodyStage || bodyStage.getValue() != "source")
    return emitError() << "source body stage/unit_kind is invalid";

  auto owner = interface->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto bodyOwner = body->getAttrOfType<DictionaryAttr>("ac.source_owner");
  if (failed(ac::detail::verifySourceOwner(owner, emitError)) ||
      failed(ac::detail::verifySourceOwner(bodyOwner, emitError)))
    return failure();
  ownerMismatch = owner != bodyOwner;
  if (ownerMismatch)
    return emitError() << "source body/interface SourceOwner mismatch";
  return success();
}

LogicalResult SourceHeaderRegistry::verifyBodySnapshots(
    ModuleOp body, ModuleOp owningHeader,
    ac::detail::EmitError emitError) const {
  if (!body || !owningHeader)
    return emitError() << "source body and owning header are both required";

  auto owner = owningHeader->getAttrOfType<DictionaryAttr>("ac.source_owner");
  ModuleOp suppliedOwningHeader;
  for (ModuleOp header : headers_)
    if (header->getAttrOfType<DictionaryAttr>("ac.source_owner") == owner) {
      suppliedOwningHeader = header;
      break;
    }

  if (suppliedOwningHeader) {
    if (suppliedOwningHeader != owningHeader)
      return emitError()
             << "owning header does not match the header admitted for its "
                "SourceOwner";
    auto lookup = [this](FlatSymbolRefAttr canonical) {
      return lookupDeclaration(canonical);
    };
    return verifyBodySnapshotsAgainstAuthority(body, owningHeader, lookup,
                                               emitError);
  }

  SmallVector<ModuleOp> completeHeaders(headers_.begin(), headers_.end());
  completeHeaders.push_back(owningHeader);
  auto authority = SourceHeaderRegistry::create(completeHeaders, emitError);
  if (failed(authority))
    return failure();
  auto lookup = [&authority](FlatSymbolRefAttr canonical) {
    return authority->lookupDeclaration(canonical);
  };
  return verifyBodySnapshotsAgainstAuthority(body, owningHeader, lookup,
                                             emitError);
}

} // namespace acir::compiler

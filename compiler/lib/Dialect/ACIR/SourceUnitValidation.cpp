#include "pycircuit/Dialect/ACIR/SourceUnitValidation.h"
#include "ACIRSourceContracts.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

#include <string>
#include <tuple>

using namespace mlir;

namespace acir::ac {
namespace {
std::string moduleName(DictionaryAttr owner) {
  StringRef package = owner.getAs<StringAttr>("package").getValue();
  StringRef path = owner.getAs<StringAttr>("path").getValue();
  SmallVector<StringRef> parts;
  path.drop_back(3).split(parts, '/');
  if (!parts.empty() && parts.back() == "__init__")
    parts.pop_back();
  std::string result = package.str();
  for (StringRef part : parts) {
    if (!result.empty())
      result.push_back('.');
    result.append(part);
  }
  return result;
}

bool ownerLess(DictionaryAttr left, DictionaryAttr right) {
  return detail::compareClosedSourceStructure(left, right) < 0;
}

FailureOr<DictionaryAttr> verifyEnvelope(mlir::ModuleOp unit,
                                         StringRef expectedKind,
                                         detail::EmitError emitError) {
  if (!unit)
    return emitError() << "source unit is required";
  auto owner = unit->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto kind = unit->getAttrOfType<StringAttr>("ac.unit_kind");
  auto stage = unit->getAttrOfType<StringAttr>("ac.stage");
  auto interfaces = unit->getAttrOfType<ArrayAttr>("ac.interfaces");
  if (failed(detail::verifySourceOwner(owner, emitError)) || !kind ||
      kind.getValue() != expectedKind || !stage ||
      stage.getValue() != "source" || !interfaces || interfaces.empty())
    return emitError() << "source " << expectedKind
                       << " unit envelope is incomplete";
  if (!unit->getAttrOfType<ArrayAttr>("ac.exports"))
    return emitError() << "source unit requires ArrayAttr ac.exports";
  if (!unit->getAttrOfType<ArrayAttr>("ac.import_bindings"))
    return emitError() << "source unit requires ArrayAttr ac.import_bindings";
  llvm::DenseSet<Attribute> unique;
  DictionaryAttr previous;
  for (auto [index, raw] : llvm::enumerate(interfaces)) {
    auto dependency = dyn_cast<DictionaryAttr>(raw);
    if (!dependency || failed(detail::verifySourceOwner(dependency, emitError)))
      return emitError() << "ac.interfaces[" << index
                         << "] is not a valid SourceOwner";
    if (!unique.insert(dependency).second)
      return emitError() << "ac.interfaces contains a duplicate SourceOwner";
    if (index == 0) {
      if (dependency != owner)
        return emitError()
               << "ac.interfaces must begin with the source unit owner";
      continue;
    }
    if (dependency == owner || (previous && !ownerLess(previous, dependency)))
      return emitError()
             << "ac.interfaces dependencies must be structurally ordered";
    previous = dependency;
  }
  return owner;
}

bool retainedDeclaration(Operation *operation) {
  return isa<TypeAliasOp, ConstantOp, StructOp, EnumOp, ModuleImportOp,
             func::FuncOp>(operation);
}

LogicalResult verifyDeclaration(Operation *operation, DictionaryAttr owner,
                                ArrayAttr interfaces, bool body,
                                detail::EmitError emitError) {
  auto declarationOwner = detail::declarationSourceOwner(operation);
  auto origin = operation->getAttrOfType<DictionaryAttr>("ac.origin");
  auto role = operation->getAttrOfType<StringAttr>("ac.declaration_role");
  auto name =
      operation->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
  if (!isa<SymbolOpInterface>(operation) || !name || name.getValue().empty() ||
      failed(detail::verifySourceOwner(declarationOwner, emitError)) ||
      failed(detail::verifyOccurrence(origin, emitError)) ||
      failed(detail::verifyOriginDefinition(
          origin,
          FlatSymbolRefAttr::get(operation->getContext(), name.getValue()),
          "source declaration", emitError)) ||
      !role)
    return emitError() << "source declaration envelope is malformed";
  if (role.getValue() == "definition") {
    if (declarationOwner != owner)
      return emitError() << "source definition has a foreign SourceOwner";
    if (body ? !isa<ModuleOp, StructOp, EnumOp>(operation)
             : !retainedDeclaration(operation))
      return emitError() << "unsupported source definition category";
  } else if (role.getValue() == "import_snapshot") {
    if (!retainedDeclaration(operation))
      return emitError() << "unsupported retained import snapshot category";
    if (declarationOwner == owner)
      return emitError() << "source import snapshot has its owning SourceOwner";
    auto imported = dyn_cast<ModuleImportOp>(operation);
    bool builtinSnapshot = imported && imported.getPrimitiveKindAttr();
    if (!builtinSnapshot && !llvm::is_contained(interfaces, declarationOwner))
      return emitError()
             << "source snapshot owner is absent from ac.interfaces";
  } else {
    return emitError() << "source declaration role is invalid";
  }
  if (!detail::hasQualifiedDeclarationIdentityForOwner(operation,
                                                       declarationOwner))
    return emitError()
           << "source declaration symbol does not match its SourceOwner";
  return success();
}

FailureOr<SourceUnitStructure> verifyStructure(mlir::ModuleOp unit, bool body,
                                               detail::EmitError emitError) {
  if (!unit)
    return emitError() << "source unit is required";
  if (failed(mlir::verify(unit)))
    return failure();
  auto kind = unit->getAttrOfType<StringAttr>("ac.unit_kind");
  if (body && (!kind || (kind.getValue() != "implementation" &&
                         kind.getValue() != "declarations")))
    return emitError()
           << "source body unit_kind must be implementation or declarations";
  bool implementation = body && kind.getValue() == "implementation";
  auto owner =
      verifyEnvelope(unit, body ? kind.getValue() : "interface", emitError);
  if (failed(owner) ||
      failed(detail::verifyNamespaceRecordShapes(unit, emitError)))
    return failure();
  SourceUnitStructure result;
  result.owner = *owner;
  auto interfaces = unit->getAttrOfType<ArrayAttr>("ac.interfaces");
  unsigned modules = 0;
  for (Operation &operation : unit.getBody()->getOperations()) {
    if (failed(
            verifyDeclaration(&operation, *owner, interfaces, body, emitError)))
      return failure();
    if (isa<ModuleOp>(operation))
      ++modules;
    auto name =
        operation.getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
    if (!result.declarations.try_emplace(name.getValue(), &operation).second)
      return emitError() << "duplicate source declaration symbol @"
                         << name.getValue();
  }
  if (implementation && modules == 0)
    return emitError() << "implementation body must own at least one module";
  if (body && !implementation && modules != 0)
    return emitError() << "declarations body cannot own ac.module";
  if (failed(detail::verifyRetainedNamespaceTargets(unit, result.declarations,
                                                    emitError)))
    return failure();
  return std::move(result);
}
} // namespace

namespace detail {
DictionaryAttr declarationSourceOwner(Operation *operation) {
  if (auto module = dyn_cast_or_null<ModuleOp>(operation))
    return module.getSourceOwner();
  if (auto imported = dyn_cast_or_null<ModuleImportOp>(operation))
    return imported.getSourceOwner();
  return operation ? operation->getAttrOfType<DictionaryAttr>("ac.source_owner")
                   : DictionaryAttr();
}

bool hasQualifiedDeclarationIdentityForOwner(Operation *operation,
                                             DictionaryAttr owner) {
  auto name =
      operation->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
  if (!owner || !name || name.getValue().empty())
    return false;
  std::string module = moduleName(owner);
  StringRef symbol = name.getValue();
  if (!module.empty() && !symbol.starts_with((Twine(module) + ".").str()))
    return false;
  StringRef local =
      module.empty() ? symbol : symbol.drop_front(module.size() + 1);
  if (isa<TypeAliasOp, ConstantOp, StructOp, EnumOp, ModuleOp, ModuleImportOp>(
          operation))
    return !local.empty() && !local.contains('.');
  if (isa<func::FuncOp>(operation)) {
    if (!local.contains('.'))
      return !local.empty();
    StringRef parent, leaf;
    std::tie(parent, leaf) = local.rsplit('.');
    return !parent.empty() && !parent.contains('.') && leaf == "__init__";
  }
  return false;
}

LogicalResult verifyOriginDefinition(DictionaryAttr origin,
                                     FlatSymbolRefAttr expected,
                                     StringRef description,
                                     EmitError emitError) {
  auto site = origin ? origin.getAs<DictionaryAttr>("site") : DictionaryAttr();
  auto definition =
      site ? site.getAs<FlatSymbolRefAttr>("definition") : FlatSymbolRefAttr();
  if (!expected || !definition || definition != expected)
    return emitError()
           << description
           << " occurrence definition does not match its canonical symbol";
  return success();
}
} // namespace detail

FailureOr<SourceUnitStructure>
verifySourceBodyStructure(mlir::ModuleOp unit, detail::EmitError emitError) {
  return verifyStructure(unit, true, emitError);
}
FailureOr<SourceUnitStructure>
verifySourceInterfaceStructure(mlir::ModuleOp unit,
                               detail::EmitError emitError) {
  return verifyStructure(unit, false, emitError);
}
} // namespace acir::ac

#include "RuleEffectView.h"
#include "SourceHeaderHelpers.h"
#include "SourceNamespace.h"
#include "SourceUnit.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"

using namespace mlir;

namespace acir::compiler {
namespace {
LogicalResult verifyModulePortsAgainstHeader(ac::ModuleOp module,
                                             ac::ModuleImportOp declaration,
                                             ac::detail::EmitError emitError) {
  auto contract = declaration->getAttrOfType<DictionaryAttr>("ac.contract");
  auto parameters =
      contract ? contract.getAs<ArrayAttr>("parameters") : ArrayAttr();
  auto connections =
      contract ? contract.getAs<ArrayAttr>("connections") : ArrayAttr();
  auto actual = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!parameters || !connections || !actual)
    return emitError() << "source module/header port authority is incomplete";

  llvm::StringMap<DictionaryAttr> parameterByName;
  for (Attribute raw : parameters) {
    auto parameter = dyn_cast<DictionaryAttr>(raw);
    auto name = parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
    auto category =
        parameter ? parameter.getAs<StringAttr>("category") : StringAttr();
    if (name && category && category.getValue() == "connection")
      parameterByName.try_emplace(name.getValue(), parameter);
  }

  SmallVector<Attribute> currentPorts;
  SmallVector<Attribute> nextPorts;
  Builder builder(module.getContext());
  for (Attribute raw : connections) {
    auto connection = dyn_cast<DictionaryAttr>(raw);
    auto name =
        connection ? connection.getAs<StringAttr>("parameter") : StringAttr();
    auto elements =
        connection ? connection.getAs<ArrayAttr>("elements") : ArrayAttr();
    auto found =
        name ? parameterByName.find(name.getValue()) : parameterByName.end();
    if (!name || !elements || found == parameterByName.end())
      return emitError()
             << "source module header has an unresolved connection contract";
    DictionaryAttr parameter = found->second;
    for (Attribute rawElement : elements) {
      auto element = dyn_cast<DictionaryAttr>(rawElement);
      auto read = element ? element.getAs<BoolAttr>("read") : BoolAttr();
      auto write = element ? element.getAs<BoolAttr>("write") : BoolAttr();
      if (!element || !read || !write)
        return emitError()
               << "source module header has a malformed connection effect";
      auto makePort = [&](StringRef role) -> Attribute {
        return builder.getDictionaryAttr({
            builder.getNamedAttr("parameter", name),
            builder.getNamedAttr("ordinal", element.get("ordinal")),
            builder.getNamedAttr("role", builder.getStringAttr(role)),
            builder.getNamedAttr("type", parameter.get("type")),
            builder.getNamedAttr("origin", parameter.get("origin")),
            builder.getNamedAttr("location", parameter.get("location")),
        });
      };
      if (read.getValue())
        currentPorts.push_back(makePort("current"));
      if (write.getValue())
        nextPorts.push_back(makePort("next"));
    }
  }
  SmallVector<Attribute> expected;
  llvm::append_range(expected, currentPorts);
  llvm::append_range(expected, nextPorts);
  if (actual != builder.getArrayAttr(expected))
    return emitError()
           << "source module ports differ from the owning header contract";
  return success();
}

DictionaryAttr formalState(Builder &builder, StringAttr parameter,
                           Attribute ordinal) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("formal")),
      builder.getNamedAttr("parameter", parameter),
      builder.getNamedAttr("ordinal", ordinal),
  });
}

LogicalResult
verifyModuleEffectsAgainstHeader(ac::ModuleOp module,
                                 ac::ModuleImportOp declaration,
                                 ac::detail::EmitError emitError) {
  auto inferred = detail::inferSourceModuleEffects(module, emitError);
  if (failed(inferred))
    return failure();

  auto contract = declaration->getAttrOfType<DictionaryAttr>("ac.contract");
  auto parameters =
      contract ? contract.getAs<ArrayAttr>("parameters") : ArrayAttr();
  auto connections =
      contract ? contract.getAs<ArrayAttr>("connections") : ArrayAttr();
  if (!parameters || !connections)
    return emitError() << "source module header effect authority is incomplete";

  llvm::StringMap<DictionaryAttr> logicalTypeByParameter;
  for (Attribute raw : parameters) {
    auto parameter = dyn_cast<DictionaryAttr>(raw);
    auto name = parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
    auto category =
        parameter ? parameter.getAs<StringAttr>("category") : StringAttr();
    auto logical =
        parameter ? parameter.getAs<DictionaryAttr>("type") : DictionaryAttr();
    if (name && category && category.getValue() == "connection" && logical)
      logicalTypeByParameter.try_emplace(name.getValue(), logical);
  }

  Builder builder(module.getContext());
  llvm::DenseSet<Attribute> consumed;
  for (Attribute raw : connections) {
    auto connection = dyn_cast<DictionaryAttr>(raw);
    auto parameter =
        connection ? connection.getAs<StringAttr>("parameter") : StringAttr();
    auto elements =
        connection ? connection.getAs<ArrayAttr>("elements") : ArrayAttr();
    auto type = parameter ? logicalTypeByParameter.find(parameter.getValue())
                          : logicalTypeByParameter.end();
    if (!connection || !parameter || !elements ||
        type == logicalTypeByParameter.end())
      return emitError()
             << "source module header connection authority is malformed";
    for (Attribute rawElement : elements) {
      auto element = dyn_cast<DictionaryAttr>(rawElement);
      Attribute ordinal = element ? element.get("ordinal") : Attribute();
      if (!element || !ordinal)
        return emitError()
               << "source module header element authority is malformed";
      DictionaryAttr state = formalState(builder, parameter, ordinal);
      auto effect = llvm::find_if(inferred->effects,
                                  [&](const detail::RuleEffect &candidate) {
                                    return candidate.state == state;
                                  });
      if (effect == inferred->effects.end() || !consumed.insert(state).second)
        return emitError()
               << "source module header effect has no unique body authority";
      SmallVector<Attribute> origins(effect->origins.begin(),
                                     effect->origins.end());
      if (effect->logicalType != type->second ||
          element.getAs<BoolAttr>("read") !=
              builder.getBoolAttr(effect->read) ||
          element.getAs<BoolAttr>("write") !=
              builder.getBoolAttr(effect->write) ||
          element.getAs<StringAttr>("precision") !=
              builder.getStringAttr(effect->precision) ||
          element.getAs<ArrayAttr>("origins") != builder.getArrayAttr(origins))
        return emitError()
               << "source module header effect differs from recomputed body "
                  "authority";
    }
  }

  for (const detail::RuleEffect &effect : inferred->effects) {
    auto kind = effect.state.getAs<StringAttr>("kind");
    if (!kind)
      return emitError() << "recomputed source effect has no StateRef kind";
    if (kind.getValue() == "owned")
      continue;
    if (kind.getValue() != "formal" || !consumed.contains(effect.state))
      return emitError()
             << "recomputed formal effect is absent from its owning header";
  }
  return success();
}

template <typename LookupDeclaration>
LogicalResult
verifyBodySnapshotsAgainstAuthority(ModuleOp body, ModuleOp owningHeader,
                                    LookupDeclaration lookupDeclaration,
                                    ac::detail::EmitError emitError) {
  if (!body || !owningHeader)
    return emitError() << "source body and owning header are both required";

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

  ac::ModuleOp executableModule;
  ac::ModuleImportOp executableDeclaration;
  for (Operation &operation : body.getBody()->getOperations()) {
    if (auto module = dyn_cast<ac::ModuleOp>(operation)) {
      if (executableModule || bodyKind.getValue() != "implementation")
        return emitError() << "implementation body must own exactly one module";
      executableModule = module;
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
          declaration->getAttrOfType<DictionaryAttr>("ac.source_owner") !=
              owner ||
          !declarationRole || declarationRole.getValue() != "definition")
        return emitError()
               << "source module has no matching owning header declaration";
      if (module->getAttr("ac.root_kind") !=
              moduleDeclaration->getAttr("ac.root_kind") ||
          module->getAttr("ac.control_ports") !=
              moduleDeclaration->getAttr("ac.control_ports"))
        return emitError()
               << "source module root/control contract differs from its "
                  "owning header";
      if (failed(verifyModulePortsAgainstHeader(module, moduleDeclaration,
                                                emitError)))
        return failure();
      executableDeclaration = moduleDeclaration;
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
    Operation *definition = lookupDeclaration(canonical);
    if (!definition ||
        definition->getAttrOfType<DictionaryAttr>("ac.source_owner") !=
            snapshotOwner ||
        !detail::sameDeclaration(definition, &operation))
      return emitError()
             << "source body import snapshot differs from its owning header: "
             << canonical;
  }
  if (static_cast<bool>(executableModule) !=
      (bodyKind.getValue() == "implementation"))
    return emitError() << "source body kind disagrees with executable module";
  if (executableModule &&
      failed(verifyModuleEffectsAgainstHeader(
          executableModule, executableDeclaration, emitError)))
    return failure();
  return success();
}

LogicalResult
validateIntrinsicHeader(ModuleOp header, DictionaryAttr owner,
                        llvm::StringMap<Operation *> &declarations,
                        ac::detail::EmitError emitError) {
  auto interfaces = header->getAttrOfType<ArrayAttr>("ac.interfaces");
  auto exports = header->getAttrOfType<ArrayAttr>("ac.exports");
  auto importBindings = header->getAttrOfType<ArrayAttr>("ac.import_bindings");
  if (!interfaces || interfaces.empty() || !exports || !importBindings)
    return emitError() << "source interface metadata is incomplete";
  llvm::DenseSet<Attribute> interfaceOwners;
  DictionaryAttr previousDependency;
  for (auto [index, raw] : llvm::enumerate(interfaces)) {
    auto interfaceOwner = dyn_cast<DictionaryAttr>(raw);
    auto package = interfaceOwner ? interfaceOwner.getAs<StringAttr>("package")
                                  : StringAttr();
    auto path = interfaceOwner ? interfaceOwner.getAs<StringAttr>("path")
                               : StringAttr();
    auto previousPackage = previousDependency
                               ? previousDependency.getAs<StringAttr>("package")
                               : StringAttr();
    auto previousPath = previousDependency
                            ? previousDependency.getAs<StringAttr>("path")
                            : StringAttr();
    if (!interfaceOwner ||
        failed(ac::detail::verifySourceOwner(interfaceOwner, emitError)) ||
        !interfaceOwners.insert(interfaceOwner).second ||
        (index == 0 && interfaceOwner != owner) ||
        (index != 0 && (interfaceOwner == owner || !package || !path ||
                        (previousDependency &&
                         (package.getValue() < previousPackage.getValue() ||
                          (package == previousPackage &&
                           path.getValue() <= previousPath.getValue()))))))
      return emitError() << "source interface owner list is malformed";
    if (index != 0)
      previousDependency = interfaceOwner;
  }

  if (failed(detail::verifyNamespaceRecordShapes(header, emitError)))
    return failure();

  for (Operation &operation : header.getBody()->getOperations()) {
    auto symbol = dyn_cast<SymbolOpInterface>(&operation);
    auto name =
        operation.getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
    auto role = operation.getAttrOfType<StringAttr>("ac.declaration_role");
    auto declarationOwner =
        operation.getAttrOfType<DictionaryAttr>("ac.source_owner");
    auto origin = operation.getAttrOfType<DictionaryAttr>("ac.origin");
    if (!symbol || !name || name.getValue().empty() || !role ||
        !declarationOwner ||
        failed(ac::detail::verifySourceOwner(declarationOwner, emitError)) ||
        !origin || failed(ac::detail::verifyOccurrence(origin, emitError)) ||
        failed(detail::verifyOriginDefinition(
            origin,
            FlatSymbolRefAttr::get(header.getContext(), name.getValue()),
            "interface declaration", emitError)) ||
        !isa<ac::TypeAliasOp, ac::ConstantOp, ac::StructOp, ac::ModuleImportOp,
             func::FuncOp>(operation))
      return emitError()
             << "source interface declaration envelope is malformed";
    if (role.getValue() == "definition") {
      if (declarationOwner != owner)
        return emitError()
               << "source interface definition has a foreign SourceOwner";
    } else if (role.getValue() == "import_snapshot") {
      if (declarationOwner == owner)
        return emitError()
               << "source interface import snapshot has its owning SourceOwner";
      if (!interfaceOwners.contains(declarationOwner))
        return emitError() << "source interface snapshot owner is absent from "
                              "ac.interfaces";
    } else {
      return emitError() << "source interface declaration role is invalid";
    }
    if (!detail::hasQualifiedDeclarationIdentityForOwner(&operation,
                                                         declarationOwner))
      return emitError()
             << "source interface declaration symbol does not match its "
                "SourceOwner";
    if (!declarations.try_emplace(name.getValue(), &operation).second)
      return emitError() << "duplicate source interface declaration symbol @"
                         << name.getValue();
  }
  if (failed(detail::verifyRetainedNamespaceTargets(header, declarations,
                                                    emitError)))
    return failure();
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

  llvm::StringMap<Operation *> declarations;
  if (failed(
          validateIntrinsicHeader(interface, owner, declarations, emitError)))
    return failure();
  auto lookup = [&declarations](FlatSymbolRefAttr canonical) {
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

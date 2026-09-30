#include "FinalCppEmission.h"
#include "FinalCppDeclarations.h"
#include "FinalCppNames.h"

#include "SourceUnit.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"

#include <algorithm>
#include <cstdint>

using namespace mlir;

namespace acir::compiler {
namespace {

FailureOr<std::string> carrierName(const FinalProgram &program,
                                   Attribute stateID,
                                   ac::detail::EmitError emitError) {
  for (const auto &carrier : program.stateCarriers()) {
    if (carrier.stateID != stateID)
      continue;
    auto name = carrier.declaration
                    ? carrier.declaration->getAttrOfType<StringAttr>("name")
                    : StringAttr();
    if (!name || name.getValue().empty())
      return emitError() << "C++ source state has no frozen source name";
    return name.getValue().str();
  }
  return emitError() << "C++ source state has no frozen carrier";
}

LogicalResult verifyGlobalSourceFamily(StringRef nameSpace,
                                       StringRef familyName,
                                       ac::detail::EmitError emitError) {
  if (!nameSpace.empty())
    return success();
  if (familyName == "std" || familyName == "gfsim")
    return emitError() << "C++ source module occupies reserved global scope ::"
                       << familyName;
  if (familyName == "FinalSystem" || familyName == "FinalModel" ||
      familyName == "ReadView" || familyName == "ProposalSlot" ||
      familyName == "SignExtend" || familyName == "kObservationDescriptors")
    return emitError() << "C++ source module collides with generated global "
                          "glue: "
                       << familyName;
  return success();
}

} // namespace

StringRef CppEmissionNames::methodType(size_t definition) const {
  return specs[definition].methodTypeName;
}

StringRef CppEmissionNames::qualifiedType(size_t definition) const {
  return specs[definition].qualifiedTypeName;
}

StringRef CppEmissionNames::family(size_t definition) const {
  return specs[definition].familyName;
}

std::string CppEmissionNames::input(size_t definition,
                                    unsigned portIndex) const {
  if (!sourceOwned)
    return (Twine("input_") + Twine(portIndex) + "_").str();
  auto found = specs[definition].inputNames.find(portIndex);
  return found == specs[definition].inputNames.end() ? std::string()
                                                     : found->second;
}

std::string CppEmissionNames::inputParameter(size_t definition,
                                             unsigned portIndex) const {
  std::string name = input(definition, portIndex);
  if (!name.empty() && name.back() == '_')
    name.pop_back();
  return name;
}

std::string CppEmissionNames::child(size_t definition,
                                    size_t childPosition) const {
  if (!sourceOwned)
    return (Twine("child_") + Twine(childPosition) + "_").str();
  return childPosition < specs[definition].childNames.size()
             ? specs[definition].childNames[childPosition]
             : std::string();
}

std::string CppEmissionNames::state(size_t definition, StringRef role,
                                    size_t localState) const {
  if (!sourceOwned)
    return (Twine(role) + Twine(localState) + "_").str();
  StringRef sourceName = specs[definition].stateNames[localState];
  return (Twine(role) + "_" + sourceName + "_").str();
}

FailureOr<CppEmissionNames>
buildCppEmissionNames(const FinalProgram &program, ArrayRef<SpecGroup> groups,
                      ArrayRef<size_t> defByInstance, bool sourceOwned,
                      ac::detail::EmitError emitError) {
  auto instances = program.instances();
  if (defByInstance.size() != instances.size())
    return emitError() << "C++ naming requires a complete SpecGroup map";

  CppEmissionNames result;
  result.sourceOwned = sourceOwned;
  result.defByInstance.append(defByInstance.begin(), defByInstance.end());
  result.specs.resize(groups.size());
  DenseMap<Attribute, size_t> sourceGroupByOwner;
  llvm::StringMap<DictionaryAttr> ownerByPath;
  llvm::StringMap<FlatSymbolRefAttr> definitionByName;
  llvm::StringMap<FlatSymbolRefAttr> moduleByCanonicalName;
  llvm::StringMap<std::string> rawNamespaceByCppPrefix;

  for (auto [definitionIndex, group] : llvm::enumerate(groups)) {
    if (group.instances.empty() || group.instances.front() >= instances.size())
      return emitError() << "C++ SpecGroup has no valid representative";
    const auto &representative = instances[group.instances.front()];
    CppSpecNames &names = result.specs[definitionIndex];
    if (!sourceOwned) {
      names.familyName = "FinalModuleDef" + std::to_string(definitionIndex);
      names.methodTypeName = names.familyName;
      names.qualifiedTypeName = names.familyName;
      definitionByName.try_emplace(names.familyName, group.definition);
      auto owner = representative.module->getAttrOfType<DictionaryAttr>(
          "ac.source_owner");
      if (!owner)
        return emitError() << "C++ source module has no ac.source_owner";
      for (size_t instanceOrdinal : group.instances)
        if (instanceOrdinal >= instances.size() ||
            instances[instanceOrdinal].module->getAttr("ac.source_owner") !=
                owner)
          return emitError()
                 << "SpecGroup instances disagree on their source owner";
      auto components = sourceOwnerComponents(owner, emitError);
      if (failed(components))
        return failure();
      auto namespaceName = namespaceCppName(components->namespaces, emitError);
      auto symbolName = sourceDefinitionName(group.definition,
                                             components->moduleName, emitError);
      if (failed(namespaceName) || failed(symbolName))
        return failure();
      auto family = legalizeIdentifier(*symbolName, emitError);
      if (failed(family))
        return failure();
      if (failed(verifyGlobalSourceFamily(*namespaceName, *family, emitError)))
        return failure();
      for (auto [index, component] : llvm::enumerate(components->namespaces)) {
        SmallVector<std::string> prefixParts;
        for (StringRef part : ArrayRef<std::string>(components->namespaces)
                                  .take_front(index + 1))
          prefixParts.push_back(part.str());
        std::string prefix = join(prefixParts, "::");
        std::string rawPrefix = join(prefixParts, ".");
        auto existingPrefix = rawNamespaceByCppPrefix.find(prefix);
        if (existingPrefix != rawNamespaceByCppPrefix.end() &&
            existingPrefix->second != rawPrefix)
          return emitError() << "C++ namespace prefix collision: " << prefix;
        rawNamespaceByCppPrefix.try_emplace(prefix, rawPrefix);
      }
      std::string canonicalClass =
          namespaceName->empty() ? *family : (*namespaceName + "::" + *family);
      auto priorClass = moduleByCanonicalName.find(canonicalClass);
      if (priorClass != moduleByCanonicalName.end() &&
          priorClass->second != group.definition)
        return emitError() << "C++ source class name collision: "
                           << canonicalClass;
      moduleByCanonicalName.try_emplace(canonicalClass, group.definition);
    } else {
      if (!group.staticArguments || !group.staticArguments.empty())
        return emitError()
               << "source-owned C++ emission supports empty static arguments";
      auto owner = representative.module->getAttrOfType<DictionaryAttr>(
          "ac.source_owner");
      if (!owner)
        return emitError() << "C++ source module has no ac.source_owner";
      for (size_t instanceOrdinal : group.instances) {
        if (instanceOrdinal >= instances.size() ||
            instances[instanceOrdinal].module->getAttr("ac.source_owner") !=
                owner)
          return emitError()
                 << "SpecGroup instances disagree on their source owner";
      }
      auto components = sourceOwnerComponents(owner, emitError);
      if (failed(components))
        return failure();
      SmallVector<std::string> cppNamespaceParts;
      for (auto [index, component] : llvm::enumerate(components->namespaces)) {
        auto legalizedComponent = legalizeIdentifier(component, emitError);
        if (failed(legalizedComponent))
          return failure();
        cppNamespaceParts.push_back(std::move(*legalizedComponent));
        std::string cppPrefix = join(cppNamespaceParts, "::");
        std::string rawPrefix = join(
            ArrayRef<std::string>(components->namespaces).take_front(index + 1),
            ".");
        auto existingPrefix = rawNamespaceByCppPrefix.find(cppPrefix);
        if (existingPrefix != rawNamespaceByCppPrefix.end() &&
            existingPrefix->second != rawPrefix)
          return emitError() << "C++ namespace prefix collision: " << cppPrefix;
        rawNamespaceByCppPrefix.try_emplace(cppPrefix, std::move(rawPrefix));
      }
      auto symbolName = sourceDefinitionName(group.definition,
                                             components->moduleName, emitError);
      if (failed(symbolName))
        return failure();
      auto family = legalizeIdentifier(*symbolName, emitError);
      auto nameSpace = namespaceCppName(components->namespaces, emitError);
      auto header = sourcePathName(components->filePath, ".hpp", emitError);
      auto source = sourcePathName(components->filePath, ".cpp", emitError);
      if (failed(family) || failed(nameSpace) || failed(header) ||
          failed(source))
        return failure();
      if (failed(verifyGlobalSourceFamily(*nameSpace, *family, emitError)))
        return failure();
      names.sourceOwner = owner;
      names.familyName = std::move(*family);
      names.nameSpace = std::move(*nameSpace);
      names.headerPath = std::move(*header);
      names.sourcePath = std::move(*source);
      names.methodTypeName = names.familyName + "<>";
      names.rawNameSpace = join(components->namespaces, ".");
      names.qualifiedTypeName =
          names.nameSpace.empty()
              ? "::" + names.methodTypeName
              : "::" + names.nameSpace + "::" + names.methodTypeName;

      std::string classKey = names.nameSpace.empty()
                                 ? names.familyName
                                 : names.nameSpace + "::" + names.familyName;
      auto existingClass = definitionByName.find(classKey);
      if (existingClass != definitionByName.end() &&
          existingClass->second != group.definition)
        return emitError() << "C++ source class name collision: " << classKey;
      definitionByName.try_emplace(classKey, group.definition);
      auto priorCanonical = moduleByCanonicalName.find(classKey);
      if (priorCanonical != moduleByCanonicalName.end() &&
          priorCanonical->second != group.definition)
        return emitError() << "C++ source class name collision: " << classKey;
      moduleByCanonicalName.try_emplace(classKey, group.definition);

      auto sourceGroup = sourceGroupByOwner.find(owner);
      if (sourceGroup == sourceGroupByOwner.end()) {
        std::string pathKey = names.headerPath;
        auto existingOwner = ownerByPath.find(pathKey);
        if (existingOwner != ownerByPath.end() &&
            existingOwner->second != owner)
          return emitError() << "C++ source path collision: " << pathKey;
        ownerByPath.try_emplace(pathKey, owner);
        size_t index = result.sourceGroups.size();
        CppSourceOwnerNames ownerNames;
        ownerNames.sourceOwner = owner;
        ownerNames.rawNameSpace = names.rawNameSpace;
        ownerNames.nameSpace = names.nameSpace;
        ownerNames.headerPath = names.headerPath;
        ownerNames.sourcePath = names.sourcePath;
        result.sourceGroups.push_back(std::move(ownerNames));
        sourceGroupByOwner.try_emplace(owner, index);
        sourceGroup = sourceGroupByOwner.find(owner);
      } else if (result.sourceGroups[sourceGroup->second].headerPath !=
                     names.headerPath ||
                 result.sourceGroups[sourceGroup->second].sourcePath !=
                     names.sourcePath) {
        return emitError()
               << "one SourceOwner maps to inconsistent C++ source paths";
      }
      result.sourceGroups[sourceGroup->second].definitions.push_back(
          definitionIndex);

      llvm::StringSet<> memberNames;
      for (size_t stateOrdinal : representative.ownedStateOrdinals) {
        if (stateOrdinal >= program.proposals().states.size())
          return emitError() << "C++ source state ordinal is invalid";
        auto rawName = carrierName(
            program, program.proposals().states[stateOrdinal].stateID,
            emitError);
        if (failed(rawName))
          return failure();
        auto stateName = legalizeIdentifier(*rawName, emitError);
        if (failed(stateName))
          return failure();
        if (!memberNames.insert("q_" + *stateName + "_").second)
          return emitError()
                 << "C++ source state name collision in " << names.familyName;
        names.stateNames.push_back(std::move(*stateName));
      }

      auto ports = representative.module->getAttrOfType<ArrayAttr>("ac.ports");
      if (!ports)
        return emitError() << "C++ source module has no frozen ac.ports";
      llvm::StringSet<> inputNames;
      for (auto [portIndex, rawPort] : llvm::enumerate(ports)) {
        auto port = dyn_cast<DictionaryAttr>(rawPort);
        auto role = port ? port.getAs<StringAttr>("role") : StringAttr();
        if (!role)
          return emitError() << "C++ source module port has no role";
        if (role.getValue() != "current")
          continue;
        auto parameter =
            port ? port.getAs<StringAttr>("parameter") : StringAttr();
        Attribute ordinal = port.get("ordinal");
        if (!parameter || !ordinal)
          return emitError() << "C++ source input port identity is malformed";
        auto parameterName =
            legalizeIdentifier(parameter.getValue(), emitError);
        if (failed(parameterName))
          return failure();
        std::string inputName = "input_" + *parameterName + "_";
        if (!isa<UnitAttr>(ordinal)) {
          auto element = ac::detail::decodeU64(dyn_cast<IntegerAttr>(ordinal),
                                               "C++ input ordinal", emitError);
          if (failed(element))
            return failure();
          inputName += std::to_string(*element) + "_";
        }
        if (!inputNames.insert(inputName).second)
          return emitError()
                 << "C++ source input name collision in " << names.familyName;
        names.inputNames.try_emplace(portIndex, std::move(inputName));
      }

      for (size_t childOrdinal : representative.childOrdinals) {
        if (childOrdinal >= instances.size())
          return emitError() << "C++ source child ordinal is invalid";
        auto rawChildName =
            frozenPlacementName(instances[childOrdinal], emitError);
        if (failed(rawChildName))
          return failure();
        auto childName = legalizeIdentifier(*rawChildName, emitError);
        if (failed(childName))
          return failure();
        std::string memberName = (Twine("child_") + *childName + "_").str();
        if (!memberNames.insert(memberName).second)
          return emitError()
                 << "C++ source child name collision in " << names.familyName;
        names.childNames.push_back(std::move(memberName));
      }
    }
  }

  // Declaration-only and empty source units are first-class C++ owners too.
  // Read only the already-verified final unit tree; source/header inputs are
  // deliberately outside this renderer's authority.
  ModuleOp package = program.hardware();
  if (!package || package.getBodyRegion().empty())
    return emitError() << "C++ declaration projection requires verified final "
                          "units";
  llvm::StringMap<std::string> declarationNames;
  for (Operation &operation : package.getBody()->getOperations()) {
    auto unit = dyn_cast<ModuleOp>(&operation);
    if (!unit)
      continue;
    auto owner = unit->getAttrOfType<DictionaryAttr>("ac.source_owner");
    if (!owner)
      return emitError() << "C++ final source unit has no SourceOwner";
    auto components = sourceOwnerComponents(owner, emitError);
    if (failed(components))
      return failure();
    auto nameSpace = namespaceCppName(components->namespaces, emitError);
    auto header = sourcePathName(components->filePath, ".hpp", emitError);
    auto source = sourcePathName(components->filePath, ".cpp", emitError);
    if (failed(nameSpace) || failed(header) || failed(source))
      return failure();
    auto sourceGroup = sourceGroupByOwner.find(owner);
    size_t groupIndex;
    if (sourceGroup == sourceGroupByOwner.end()) {
      auto existingOwner = ownerByPath.find(*header);
      if (existingOwner != ownerByPath.end() && existingOwner->second != owner)
        return emitError() << "C++ source path collision: " << *header;
      ownerByPath.try_emplace(*header, owner);
      CppSourceOwnerNames ownerNames;
      ownerNames.sourceOwner = owner;
      ownerNames.rawNameSpace = join(components->namespaces, ".");
      ownerNames.nameSpace = *nameSpace;
      ownerNames.headerPath = *header;
      // Empty until an implementation module is mapped to this owner.
      ownerNames.sourcePath.clear();
      groupIndex = result.sourceGroups.size();
      result.sourceGroups.push_back(std::move(ownerNames));
      sourceGroupByOwner.try_emplace(owner, groupIndex);
    } else {
      groupIndex = sourceGroup->second;
      CppSourceOwnerNames &ownerNames = result.sourceGroups[groupIndex];
      if (ownerNames.headerPath != *header ||
          ownerNames.nameSpace != *nameSpace)
        return emitError()
               << "one SourceOwner maps to inconsistent C++ source paths";
      // `sourceOwned=false` has no module-owned groups yet; a mapped
      // implementation owner retains its real source path.
      if (!ownerNames.sourcePath.empty() && ownerNames.sourcePath != *source)
        return emitError()
               << "one SourceOwner maps to inconsistent C++ source paths";
    }
    const std::string rawModuleName = components->moduleName;
    for (auto [index, rawComponent] : llvm::enumerate(components->namespaces)) {
      SmallVector<std::string> prefixParts;
      for (StringRef part :
           ArrayRef<std::string>(components->namespaces).take_front(index + 1))
        prefixParts.push_back(part.str());
      auto legalizedPrefix = namespaceCppName(prefixParts, emitError);
      if (failed(legalizedPrefix))
        return failure();
      std::string rawPrefix = join(prefixParts, ".");
      auto priorPrefix = rawNamespaceByCppPrefix.find(*legalizedPrefix);
      if (priorPrefix != rawNamespaceByCppPrefix.end() &&
          priorPrefix->second != rawPrefix)
        return emitError() << "C++ namespace prefix collision: "
                           << *legalizedPrefix;
      rawNamespaceByCppPrefix.try_emplace(*legalizedPrefix, rawPrefix);
      (void)rawComponent;
    }
    StringRef legalizedNamespace = *nameSpace;
    if (legalizedNamespace.starts_with("std") &&
        (legalizedNamespace.size() == 3 ||
         legalizedNamespace.drop_front(3).starts_with("::")))
      return emitError() << "C++ source namespace occupies reserved global "
                            "scope ::std";
    if (legalizedNamespace.starts_with("gfsim") &&
        (legalizedNamespace.size() == 5 ||
         legalizedNamespace.drop_front(5).starts_with("::")))
      return emitError() << "C++ source namespace occupies reserved global "
                            "scope ::gfsim";

    ModuleOp declarationUnit = unit;
    for (Operation &declaration : declarationUnit.getBody()->getOperations()) {
      StringRef opName = declaration.getName().getStringRef();
      if (opName != "ac.type_alias" && opName != "ac.constant")
        continue;
      auto symbol = declaration.getAttrOfType<StringAttr>("sym_name");
      if (!symbol)
        return emitError() << "C++ final declaration has no symbol name";
      StringRef rawSymbol = symbol.getValue();
      StringRef leaf = rawSymbol;
      if (!rawModuleName.empty()) {
        std::string prefix = (Twine(rawModuleName) + ".").str();
        if (!rawSymbol.starts_with(prefix))
          return emitError() << "C++ declaration symbol is outside its "
                                "SourceOwner: "
                             << rawSymbol;
        leaf = rawSymbol.drop_front(prefix.size());
      } else if (rawSymbol.contains('.')) {
        return emitError() << "C++ declaration symbol has an unexpected "
                              "module prefix: "
                           << rawSymbol;
      }
      if (leaf.empty() || leaf.contains('.'))
        return emitError() << "C++ declaration has an invalid canonical "
                              "symbol: "
                           << rawSymbol;
      auto cppName = legalizeIdentifier(leaf, emitError);
      if (failed(cppName))
        return failure();
      std::string qualified =
          nameSpace->empty() ? *cppName : (*nameSpace + "::" + *cppName);
      if (!declarationNames.try_emplace(qualified, rawSymbol.str()).second ||
          definitionByName.contains(qualified) ||
          moduleByCanonicalName.contains(qualified))
        return emitError() << "C++ declaration/module name collision: "
                           << qualified;
      if (nameSpace->empty()) {
        if (qualified == "std" || qualified == "gfsim")
          return emitError() << "C++ declaration occupies reserved global "
                                "scope ::"
                             << qualified;
        if (qualified == "FinalSystem" || qualified == "FinalModel" ||
            qualified == "ReadView" || qualified == "ProposalSlot" ||
            qualified == "SignExtend" || qualified == "kObservationDescriptors")
          return emitError()
                 << "C++ declaration collides with generated global glue: "
                 << qualified;
      }
      std::string ownerText =
          owner.getAs<StringAttr>("package").getValue().str() + ":" +
          owner.getAs<StringAttr>("path").getValue().str() + ":" +
          rawSymbol.str();
      auto text = emitCppScalarDeclaration(&declaration, *cppName, ownerText,
                                           emitError);
      if (failed(text))
        return failure();
      result.sourceGroups[groupIndex].declarations.push_back(
          {rawSymbol.str(), std::move(*text)});
    }
  }
  for (CppSourceOwnerNames &ownerGroup : result.sourceGroups)
    llvm::sort(ownerGroup.declarations, [](const CppDeclarationNames &left,
                                           const CppDeclarationNames &right) {
      return left.symbolName < right.symbolName;
    });
  for (const auto &entry : declarationNames)
    if (rawNamespaceByCppPrefix.contains(entry.getKey()))
      return emitError() << "C++ namespace and declaration names collide: "
                         << entry.getKey();

  for (const auto &namespacePrefix : rawNamespaceByCppPrefix)
    if (moduleByCanonicalName.contains(namespacePrefix.getKey()))
      return emitError() << "C++ namespace and class names collide: "
                         << namespacePrefix.getKey();

  if (sourceOwned) {
    for (auto [definitionIndex, group] : llvm::enumerate(groups)) {
      const auto &representative = instances[group.instances.front()];
      size_t ownerGroupIndex =
          sourceGroupByOwner.lookup(result.specs[definitionIndex].sourceOwner);
      CppSourceOwnerNames &ownerGroup = result.sourceGroups[ownerGroupIndex];
      llvm::StringSet<> included;
      for (size_t childOrdinal : representative.childOrdinals) {
        size_t childDefinition = defByInstance[childOrdinal];
        const CppSpecNames &childNames = result.specs[childDefinition];
        if (childNames.sourceOwner == ownerGroup.sourceOwner ||
            !included.insert(childNames.headerPath).second)
          continue;
        ownerGroup.childHeaders.push_back(childNames.headerPath);
      }
      llvm::sort(ownerGroup.childHeaders);
    }
  }

  llvm::sort(result.sourceGroups, [](const CppSourceOwnerNames &left,
                                     const CppSourceOwnerNames &right) {
    return left.headerPath < right.headerPath;
  });
  return result;
}

} // namespace acir::compiler

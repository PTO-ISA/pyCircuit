#include "FinalCppEmission.h"

#include "SourceUnit.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"

#include <algorithm>
#include <cstdint>

using namespace mlir;

namespace acir::compiler {
namespace {

struct OwnerComponents {
  std::string moduleName;
  SmallVector<std::string> namespaces;
  SmallVector<std::string> filePath;
};

bool isAsciiAlpha(char value) {
  return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z');
}

bool isAsciiDigit(char value) { return value >= '0' && value <= '9'; }

bool isCppKeyword(StringRef value) {
  static const llvm::StringSet<> keywords = [] {
    llvm::StringSet<> result;
    SmallVector<StringRef> spellings;
    StringRef(
        "alignas alignof and and_eq asm atomic_cancel atomic_commit "
        "atomic_noexcept auto bitand bitor bool break case catch char char8_t "
        "char16_t char32_t class compl concept const consteval constexpr "
        "constinit const_cast continue co_await co_return co_yield decltype "
        "default delete do double dynamic_cast else enum explicit export "
        "extern false float for friend goto if import inline int long module "
        "mutable namespace new noexcept not not_eq nullptr operator or or_eq "
        "private protected public reflexpr register reinterpret_cast requires "
        "return short signed sizeof static static_assert static_cast struct "
        "switch synchronized template this thread_local throw true try "
        "typedef typeid typename union unsigned using virtual void volatile "
        "wchar_t while xor xor_eq")
        .split(spellings, ' ');
    for (StringRef keyword : spellings)
      result.insert(keyword);
    return result;
  }();
  return keywords.contains(value);
}

FailureOr<uint32_t> readCodePoint(StringRef text, size_t &offset,
                                  ac::detail::EmitError emitError) {
  const auto *bytes = reinterpret_cast<const uint8_t *>(text.data());
  uint8_t first = bytes[offset];
  unsigned length = first < 0x80             ? 1
                    : (first & 0xe0) == 0xc0 ? 2
                    : (first & 0xf0) == 0xe0 ? 3
                    : (first & 0xf8) == 0xf0 ? 4
                                             : 0;
  if (!length || offset + length > text.size())
    return emitError() << "source name contains invalid UTF-8";
  uint32_t value = first & (length == 1   ? 0x7f
                            : length == 2 ? 0x1f
                            : length == 3 ? 0x0f
                                          : 0x07);
  for (unsigned index = 1; index < length; ++index) {
    uint8_t byte = bytes[offset + index];
    if ((byte & 0xc0) != 0x80)
      return emitError() << "source name contains invalid UTF-8";
    value = (value << 6) | (byte & 0x3f);
  }
  if ((length == 2 && value < 0x80) || (length == 3 && value < 0x800) ||
      (length == 4 && value < 0x10000) || value > 0x10ffff ||
      (value >= 0xd800 && value <= 0xdfff))
    return emitError() << "source name contains invalid UTF-8";
  offset += length;
  return value;
}

FailureOr<std::string> legalizeIdentifier(StringRef source,
                                          ac::detail::EmitError emitError) {
  if (source.empty())
    return emitError() << "C++ source identifier is empty";
  static constexpr char hex[] = "0123456789abcdef";
  std::string result;
  bool pendingSeparator = false;
  bool previousLowerOrDigit = false;
  size_t offset = 0;
  while (offset < source.size()) {
    unsigned char byte = static_cast<unsigned char>(source[offset]);
    if (byte >= 0x80) {
      auto codePoint = readCodePoint(source, offset, emitError);
      if (failed(codePoint))
        return failure();
      if (!result.empty())
        result.push_back('_');
      std::string token = "u";
      uint32_t value = *codePoint;
      SmallVector<char, 8> digits;
      do {
        digits.push_back(hex[value & 0xf]);
        value >>= 4;
      } while (value);
      for (auto it = digits.rbegin(); it != digits.rend(); ++it)
        token.push_back(*it);
      result.append(token);
      pendingSeparator = true;
      previousLowerOrDigit = false;
      continue;
    }

    char value = static_cast<char>(byte);
    ++offset;
    if (!isAsciiAlpha(value) && !isAsciiDigit(value)) {
      pendingSeparator = true;
      previousLowerOrDigit = false;
      continue;
    }
    bool uppercase = value >= 'A' && value <= 'Z';
    bool originalLowerOrDigit =
        (value >= 'a' && value <= 'z') || isAsciiDigit(value);
    if (uppercase && previousLowerOrDigit)
      pendingSeparator = true;
    if (pendingSeparator && !result.empty() && result.back() != '_')
      result.push_back('_');
    pendingSeparator = false;
    if (uppercase)
      value = static_cast<char>(value - 'A' + 'a');
    result.push_back(value);
    previousLowerOrDigit = originalLowerOrDigit;
  }
  while (!result.empty() && result.back() == '_')
    result.pop_back();
  if (result.empty())
    return emitError() << "source name does not form a C++ identifier";
  if (isAsciiDigit(result.front()))
    result.insert(0, "pyc_");
  if (isCppKeyword(result))
    result.insert(0, "pyc_");
  return result;
}

FailureOr<SmallVector<std::string>>
splitComponents(StringRef value, char separator,
                ac::detail::EmitError emitError) {
  SmallVector<std::string> result;
  if (value.empty())
    return result;
  SmallVector<StringRef> parts;
  value.split(parts, separator, /*MaxSplit=*/-1, /*KeepEmpty=*/true);
  for (StringRef part : parts) {
    if (part.empty())
      return emitError() << "SourceOwner contains an empty qualified component";
    result.push_back(part.str());
  }
  return result;
}

std::string join(ArrayRef<std::string> parts, StringRef separator) {
  std::string result;
  for (StringRef part : parts) {
    if (!result.empty())
      result.append(separator);
    result.append(part);
  }
  return result;
}

FailureOr<OwnerComponents>
sourceOwnerComponents(DictionaryAttr owner, ac::detail::EmitError emitError) {
  if (failed(ac::detail::verifySourceOwner(owner, emitError)))
    return failure();
  auto packageAttr = owner.getAs<StringAttr>("package");
  auto pathAttr = owner.getAs<StringAttr>("path");
  StringRef path = pathAttr.getValue();
  if (!path.ends_with(".py"))
    return emitError() << "C++ source group owner path must end in .py";

  OwnerComponents result;
  auto packageParts = splitComponents(packageAttr.getValue(), '.', emitError);
  if (failed(packageParts))
    return failure();
  auto fileParts = splitComponents(path.drop_back(3), '/', emitError);
  if (failed(fileParts))
    return failure();
  SmallVector<std::string> moduleParts = *fileParts;
  if (!moduleParts.empty() && moduleParts.back() == "__init__")
    moduleParts.pop_back();

  result.namespaces.append(packageParts->begin(), packageParts->end());
  result.namespaces.append(moduleParts.begin(), moduleParts.end());
  result.filePath.append(packageParts->begin(), packageParts->end());
  result.filePath.append(fileParts->begin(), fileParts->end());
  if (result.filePath.empty())
    result.filePath.push_back("__init__");
  result.moduleName = join(result.namespaces, ".");
  return result;
}

FailureOr<std::string> sourceDefinitionName(FlatSymbolRefAttr definition,
                                            StringRef moduleName,
                                            ac::detail::EmitError emitError) {
  if (!definition)
    return emitError() << "C++ source group has no qualified definition";
  StringRef symbol = definition.getValue();
  StringRef leaf = symbol;
  if (!moduleName.empty()) {
    std::string prefix = (Twine(moduleName) + ".").str();
    if (!symbol.starts_with(prefix))
      return emitError()
             << "C++ definition does not belong to its module SourceOwner";
    leaf = symbol.drop_front(prefix.size());
  } else if (symbol.contains('.')) {
    return emitError()
           << "unpackaged C++ definition has a qualified module prefix";
  }
  if (leaf.empty() || leaf.contains('.'))
    return emitError() << "C++ definition has an invalid source class name";
  return leaf.str();
}

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

FailureOr<std::string> namespaceCppName(ArrayRef<std::string> components,
                                        ac::detail::EmitError emitError) {
  SmallVector<std::string> legalized;
  for (StringRef component : components) {
    auto name = legalizeIdentifier(component, emitError);
    if (failed(name))
      return failure();
    legalized.push_back(std::move(*name));
  }
  return join(legalized, "::");
}

FailureOr<std::string> sourcePathName(ArrayRef<std::string> components,
                                      StringRef extension,
                                      ac::detail::EmitError emitError) {
  SmallVector<std::string> legalized;
  for (StringRef component : components) {
    auto name = legalizeIdentifier(component, emitError);
    if (failed(name))
      return failure();
    legalized.push_back(std::move(*name));
  }
  return (Twine("sources/") + join(legalized, "/") + extension).str();
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

      auto sourceGroup = sourceGroupByOwner.find(owner);
      if (sourceGroup == sourceGroupByOwner.end()) {
        std::string pathKey = names.headerPath;
        auto existingOwner = ownerByPath.find(pathKey);
        if (existingOwner != ownerByPath.end() &&
            existingOwner->second != owner)
          return emitError() << "C++ source path collision: " << pathKey;
        ownerByPath.try_emplace(pathKey, owner);
        size_t index = result.sourceGroups.size();
        result.sourceGroups.push_back(
            {owner, names.headerPath, names.sourcePath, {}});
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

  if (sourceOwned) {
    for (const auto &namespacePrefix : rawNamespaceByCppPrefix)
      if (definitionByName.contains(namespacePrefix.getKey()))
        return emitError() << "C++ namespace and class names collide: "
                           << namespacePrefix.getKey();
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

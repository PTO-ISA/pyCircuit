#include "SourceNamespace.h"
#include "SourceIdentifier.h"

#include "mlir/IR/OperationSupport.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/STLExtras.h"

#include <algorithm>
#include <cstdint>
#include <string>

using namespace mlir;

namespace acir::compiler::detail {
namespace {

struct ImportBinding {
  DictionaryAttr source;
  StringAttr name;
  FlatSymbolRefAttr target;
  DictionaryAttr site;
};

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

int compareOwner(DictionaryAttr left, DictionaryAttr right) {
  int package = compareBytes(left.getAs<StringAttr>("package").getValue(),
                             right.getAs<StringAttr>("package").getValue());
  if (package)
    return package;
  return compareBytes(left.getAs<StringAttr>("path").getValue(),
                      right.getAs<StringAttr>("path").getValue());
}

uint64_t integerValue(DictionaryAttr record, StringRef name) {
  auto value = record.getAs<IntegerAttr>(name);
  return value ? value.getValue().getZExtValue() : 0;
}

int comparePathComponent(DictionaryAttr left, DictionaryAttr right) {
  auto leftKind = left.getAs<StringAttr>("kind").getValue();
  auto rightKind = right.getAs<StringAttr>("kind").getValue();
  int kind = compareBytes(leftKind, rightKind);
  if (kind || leftKind != rightKind)
    return kind;
  if (leftKind == "field")
    return compareBytes(left.getAs<StringAttr>("name").getValue(),
                        right.getAs<StringAttr>("name").getValue());
  uint64_t lhs = integerValue(left, "value");
  uint64_t rhs = integerValue(right, "value");
  return lhs == rhs ? 0 : lhs < rhs ? -1 : 1;
}

int compareSite(DictionaryAttr left, DictionaryAttr right) {
  ArrayAttr leftPath = left.getAs<ArrayAttr>("ast_path");
  ArrayAttr rightPath = right.getAs<ArrayAttr>("ast_path");
  size_t count = std::min(leftPath.size(), rightPath.size());
  for (size_t index = 0; index < count; ++index) {
    int component =
        comparePathComponent(cast<DictionaryAttr>(leftPath[index]),
                             cast<DictionaryAttr>(rightPath[index]));
    if (component)
      return component;
  }
  if (leftPath.size() != rightPath.size())
    return leftPath.size() < rightPath.size() ? -1 : 1;

  DictionaryAttr leftLocation = left.getAs<DictionaryAttr>("location");
  DictionaryAttr rightLocation = right.getAs<DictionaryAttr>("location");
  int path = compareBytes(leftLocation.getAs<StringAttr>("path").getValue(),
                          rightLocation.getAs<StringAttr>("path").getValue());
  if (path)
    return path;
  for (StringRef coordinate : {"line", "column", "end_line", "end_column"}) {
    uint64_t lhs = integerValue(leftLocation, coordinate);
    uint64_t rhs = integerValue(rightLocation, coordinate);
    if (lhs != rhs)
      return lhs < rhs ? -1 : 1;
  }
  return 0;
}

int compareImportBinding(const ImportBinding &left,
                         const ImportBinding &right) {
  int source = compareOwner(left.source, right.source);
  if (source)
    return source;
  int name = compareBytes(left.name.getValue(), right.name.getValue());
  if (name)
    return name;
  int site = compareSite(left.site, right.site);
  if (site)
    return site;
  return compareBytes(left.target.getValue(), right.target.getValue());
}

LogicalResult verifyNamespaceSite(DictionaryAttr site, DictionaryAttr owner,
                                  ac::detail::EmitError emitError) {
  if (!site || site.size() != 2)
    return emitError() << "NamespaceSite must contain exactly two fields";
  auto path = site.getAs<ArrayAttr>("ast_path");
  auto location = site.getAs<DictionaryAttr>("location");
  if (!path || !location)
    return emitError() << "NamespaceSite ast_path/location has the wrong type";
  for (auto [index, raw] : llvm::enumerate(path)) {
    auto component = dyn_cast<DictionaryAttr>(raw);
    if (!component ||
        failed(ac::detail::verifyPathComponent(component, emitError)))
      return emitError() << "NamespaceSite ast_path[" << index
                         << "] is not a valid PathComponent";
  }
  if (failed(ac::detail::verifySourceSpan(location, emitError)))
    return failure();
  auto sitePath = location.getAs<StringAttr>("path");
  auto ownerPath = owner ? owner.getAs<StringAttr>("path") : StringAttr();
  if (!sitePath || !ownerPath || sitePath != ownerPath)
    return emitError()
           << "NamespaceSite SourceSpan.path does not match unit owner path";
  return success();
}

LogicalResult verifyTargetCategory(FlatSymbolRefAttr target,
                                   const SourceHeaderRegistry &registry,
                                   ac::detail::EmitError emitError) {
  Operation *declaration = registry.lookupDeclaration(target);
  if (!declaration)
    return emitError()
           << "namespace target has no canonical declaration authority: "
           << target;
  return verifyNamespaceTargetCategory(declaration, target, emitError);
}

} // namespace

LogicalResult verifyNamespaceTargetCategory(Operation *declaration,
                                            FlatSymbolRefAttr target,
                                            ac::detail::EmitError emitError) {
  if (!declaration || !target)
    return emitError() << "namespace target has no declaration: " << target;
  if (isa<ac::TypeAliasOp, ac::ConstantOp, ac::StructOp, ac::ModuleImportOp>(
          declaration))
    return success();
  if (auto helper = dyn_cast<func::FuncOp>(declaration)) {
    auto kind = helper->getAttrOfType<StringAttr>("ac.helper_kind");
    if (kind && kind.getValue() == "value")
      return success();
    if (kind && kind.getValue() == "record_constructor")
      return emitError()
             << "record constructor helpers are not direct namespace exports";
  }
  return emitError()
         << "namespace target category is not supported by this U02 slice: "
         << target;
}

LogicalResult verifyNamespaceRecordShapes(ModuleOp header,
                                          ac::detail::EmitError emitError) {
  auto owner = header ? header->getAttrOfType<DictionaryAttr>("ac.source_owner")
                      : DictionaryAttr();
  auto interfaces =
      header ? header->getAttrOfType<ArrayAttr>("ac.interfaces") : ArrayAttr();
  auto exports =
      header ? header->getAttrOfType<ArrayAttr>("ac.exports") : ArrayAttr();
  auto importBindings =
      header ? header->getAttrOfType<ArrayAttr>("ac.import_bindings")
             : ArrayAttr();
  if (!owner || !interfaces || !exports || !importBindings)
    return emitError() << "interface namespace metadata is incomplete";

  StringAttr previousExport;
  for (auto [index, raw] : llvm::enumerate(exports)) {
    auto entry = dyn_cast<DictionaryAttr>(raw);
    auto name = entry ? entry.getAs<StringAttr>("name") : StringAttr();
    auto target =
        entry ? entry.getAs<FlatSymbolRefAttr>("target") : FlatSymbolRefAttr();
    auto site = entry ? entry.getAs<DictionaryAttr>("site") : DictionaryAttr();
    if (!entry || entry.size() != 3 || !name || !target || !site ||
        failed(verifyPythonAstIdentifier(name.getValue(), true, emitError)) ||
        failed(verifyNamespaceSite(site, owner, emitError)))
      return emitError() << "ac.exports[" << index
                         << "] must be a closed ExportBinding with valid "
                            "name, target, and site";
    if (previousExport &&
        compareBytes(previousExport.getValue(), name.getValue()) >= 0)
      return emitError()
             << "ac.exports must be unique and sorted by UTF-8 name bytes";
    previousExport = name;
  }

  SmallVector<ImportBinding> bindings;
  llvm::StringMap<FlatSymbolRefAttr> targetsByProviderName;
  for (auto [index, raw] : llvm::enumerate(importBindings)) {
    auto entry = dyn_cast<DictionaryAttr>(raw);
    auto source =
        entry ? entry.getAs<DictionaryAttr>("source") : DictionaryAttr();
    auto name = entry ? entry.getAs<StringAttr>("name") : StringAttr();
    auto target =
        entry ? entry.getAs<FlatSymbolRefAttr>("target") : FlatSymbolRefAttr();
    auto site = entry ? entry.getAs<DictionaryAttr>("site") : DictionaryAttr();
    if (!entry || entry.size() != 4 || !source || !name || !target || !site ||
        failed(ac::detail::verifySourceOwner(source, emitError)) ||
        !llvm::is_contained(interfaces, source) ||
        failed(verifyPythonAstIdentifier(name.getValue(), false, emitError)) ||
        failed(verifyNamespaceSite(site, owner, emitError)))
      return emitError() << "ac.import_bindings[" << index
                         << "] must be a closed ImportBindingUse with valid "
                            "source/name/target/site";
    std::string key = (Twine(source.getAs<StringAttr>("package").getValue()) +
                       "::" + source.getAs<StringAttr>("path").getValue() +
                       "::" + name.getValue())
                          .str();
    auto existing = targetsByProviderName.find(key);
    if (existing != targetsByProviderName.end() && existing->second != target)
      return emitError() << "one producer records different targets for the "
                            "same provider/name";
    targetsByProviderName.try_emplace(key, target);
    bindings.push_back({source, name, target, site});
  }
  for (size_t index = 1; index < bindings.size(); ++index)
    if (compareImportBinding(bindings[index - 1], bindings[index]) >= 0)
      return emitError()
             << "ac.import_bindings must be unique and structurally ordered";
  return success();
}

LogicalResult
verifyRetainedNamespaceTargets(ModuleOp header,
                               const llvm::StringMap<Operation *> &declarations,
                               ac::detail::EmitError emitError) {
  auto owner = header ? header->getAttrOfType<DictionaryAttr>("ac.source_owner")
                      : DictionaryAttr();
  auto exports =
      header ? header->getAttrOfType<ArrayAttr>("ac.exports") : ArrayAttr();
  auto importBindings =
      header ? header->getAttrOfType<ArrayAttr>("ac.import_bindings")
             : ArrayAttr();
  if (!owner || !exports || !importBindings)
    return emitError() << "interface retained namespace targets are incomplete";

  for (auto [index, raw] : llvm::enumerate(exports)) {
    auto entry = dyn_cast<DictionaryAttr>(raw);
    auto target =
        entry ? entry.getAs<FlatSymbolRefAttr>("target") : FlatSymbolRefAttr();
    Operation *declaration =
        target ? declarations.lookup(target.getValue()) : nullptr;
    if (!declaration)
      return emitError() << "namespace target has no retained declaration: "
                         << target;
    if (failed(verifyNamespaceTargetCategory(declaration, target, emitError)))
      return failure();
  }
  for (auto [index, raw] : llvm::enumerate(importBindings)) {
    auto entry = dyn_cast<DictionaryAttr>(raw);
    auto target =
        entry ? entry.getAs<FlatSymbolRefAttr>("target") : FlatSymbolRefAttr();
    Operation *declaration =
        target ? declarations.lookup(target.getValue()) : nullptr;
    auto role =
        declaration
            ? declaration->getAttrOfType<StringAttr>("ac.declaration_role")
            : StringAttr();
    auto targetOwner =
        declaration
            ? declaration->getAttrOfType<DictionaryAttr>("ac.source_owner")
            : DictionaryAttr();
    if (!declaration)
      return emitError()
             << "import binding target has no retained declaration: " << target;
    if (!role || role.getValue() != "import_snapshot" || !targetOwner ||
        targetOwner == owner)
      return emitError()
             << "import binding target must be an external retained snapshot: "
             << target;
    if (failed(verifyNamespaceTargetCategory(declaration, target, emitError)))
      return failure();
  }
  return success();
}

FailureOr<SmallVector<NamespaceExportBinding>>
readNamespaceExports(ModuleOp header, const SourceHeaderRegistry &registry,
                     ac::detail::EmitError emitError) {
  auto entries = header->getAttrOfType<ArrayAttr>("ac.exports");
  if (!entries)
    return emitError() << "interface requires ArrayAttr ac.exports";
  if (failed(verifyNamespaceRecordShapes(header, emitError)))
    return failure();
  SmallVector<NamespaceExportBinding> result;
  for (Attribute raw : entries) {
    auto entry = dyn_cast<DictionaryAttr>(raw);
    auto name = entry ? entry.getAs<StringAttr>("name") : StringAttr();
    auto target =
        entry ? entry.getAs<FlatSymbolRefAttr>("target") : FlatSymbolRefAttr();
    if (failed(verifyTargetCategory(target, registry, emitError)))
      return failure();
    result.push_back({name, target});
  }
  return result;
}

LogicalResult verifyNamespaceImports(ModuleOp header,
                                     const SourceHeaderRegistry &registry,
                                     ac::detail::EmitError emitError) {
  auto entries = header->getAttrOfType<ArrayAttr>("ac.import_bindings");
  auto owner = header->getAttrOfType<DictionaryAttr>("ac.source_owner");
  if (!entries)
    return emitError() << "interface requires ArrayAttr ac.import_bindings";
  if (failed(verifyNamespaceRecordShapes(header, emitError)))
    return failure();
  auto localInterfaces = registry.interfacesForModule(moduleName(owner));
  for (Attribute raw : entries) {
    auto entry = dyn_cast<DictionaryAttr>(raw);
    auto source =
        entry ? entry.getAs<DictionaryAttr>("source") : DictionaryAttr();
    auto name = entry ? entry.getAs<StringAttr>("name") : StringAttr();
    auto target =
        entry ? entry.getAs<FlatSymbolRefAttr>("target") : FlatSymbolRefAttr();
    auto site = entry ? entry.getAs<DictionaryAttr>("site") : DictionaryAttr();
    std::string providerModule = moduleName(source);
    auto explicitProvider = registry.ownerForModule(providerModule);
    if (!explicitProvider || explicitProvider != source ||
        !llvm::is_contained(localInterfaces, source))
      return emitError() << "import binding provider has no explicit header in "
                            "the unit dependency closure";
    FlatSymbolRefAttr published =
        registry.lookupExport(providerModule, name.getValue());
    auto location = site.getAs<DictionaryAttr>("location");
    if (!published) {
      emitError() << "import at "
                  << location.getAs<StringAttr>("path").getValue() << ':'
                  << integerValue(location, "line") << ':'
                  << integerValue(location, "column")
                  << " requests unpublished name '" << name.getValue()
                  << "' from provider '" << providerModule << "'";
      return failure();
    }
    if (published != target) {
      emitError() << "stale import binding at "
                  << location.getAs<StringAttr>("path").getValue() << ':'
                  << integerValue(location, "line") << ':'
                  << integerValue(location, "column") << " for provider '"
                  << providerModule << "', name '" << name.getValue()
                  << "': expected " << target << ", published " << published;
      return failure();
    }
  }
  return success();
}

} // namespace acir::compiler::detail

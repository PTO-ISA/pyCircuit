#include "SourceNamespace.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "llvm/ADT/STLExtras.h"
#include <cstdint>
#include <string>

using namespace mlir;

namespace acir::compiler::detail {
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

uint64_t integerValue(DictionaryAttr record, StringRef name) {
  auto value = record.getAs<IntegerAttr>(name);
  return value ? value.getValue().getZExtValue() : 0;
}

LogicalResult verifyTargetCategory(FlatSymbolRefAttr target,
                                   const SourceHeaderRegistry &registry,
                                   ac::detail::EmitError emitError) {
  Operation *declaration = registry.lookupDeclaration(target);
  if (!declaration)
    return emitError()
           << "namespace target has no canonical declaration authority: "
           << target;
  return ac::detail::verifyNamespaceTargetCategory(declaration, target,
                                                   emitError);
}

} // namespace

FailureOr<SmallVector<NamespaceExportBinding>>
readNamespaceExports(ModuleOp header, const SourceHeaderRegistry &registry,
                     ac::detail::EmitError emitError) {
  auto entries = header->getAttrOfType<ArrayAttr>("ac.exports");
  if (!entries)
    return emitError() << "interface requires ArrayAttr ac.exports";
  if (failed(ac::detail::verifyNamespaceRecordShapes(header, emitError)))
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
  if (failed(ac::detail::verifyNamespaceRecordShapes(header, emitError)))
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

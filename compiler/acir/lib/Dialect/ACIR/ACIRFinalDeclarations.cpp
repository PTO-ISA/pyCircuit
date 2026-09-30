#include "ACIRFinalDeclarations.h"
#include "ACIRSourceOwnerCaseFoldData.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <iterator>

using namespace mlir;

namespace acir::ac::final_detail {
namespace {

StringRef declarationSymbol(Operation *operation) {
  auto name = operation ? operation->getAttrOfType<StringAttr>(
                              SymbolTable::getSymbolAttrName())
                        : StringAttr();
  return name ? name.getValue() : StringRef();
}

FailureOr<uint32_t> decodeUTF8(StringRef text, size_t &offset,
                               detail::EmitError emitError) {
  const auto *bytes = reinterpret_cast<const uint8_t *>(text.data());
  uint8_t first = bytes[offset];
  unsigned length = first < 0x80             ? 1
                    : (first & 0xe0) == 0xc0 ? 2
                    : (first & 0xf0) == 0xe0 ? 3
                    : (first & 0xf8) == 0xf0 ? 4
                                             : 0;
  if (!length || offset + length > text.size())
    return emitError() << "SourceOwner import module name is not valid UTF-8";
  uint32_t value = first & (length == 1   ? 0x7f
                            : length == 2 ? 0x1f
                            : length == 3 ? 0x0f
                                          : 0x07);
  for (unsigned index = 1; index < length; ++index) {
    uint8_t byte = bytes[offset + index];
    if ((byte & 0xc0) != 0x80)
      return emitError() << "SourceOwner import module name is not valid UTF-8";
    value = (value << 6) | (byte & 0x3f);
  }
  if ((length == 2 && value < 0x80) || (length == 3 && value < 0x800) ||
      (length == 4 && value < 0x10000) || value > 0x10ffff ||
      (value >= 0xd800 && value <= 0xdfff))
    return emitError() << "SourceOwner import module name is not valid UTF-8";
  offset += length;
  return value;
}

void appendUTF8(std::string &result, uint32_t codePoint) {
  if (codePoint <= 0x7f) {
    result.push_back(static_cast<char>(codePoint));
  } else if (codePoint <= 0x7ff) {
    result.push_back(static_cast<char>(0xc0 | (codePoint >> 6)));
    result.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
  } else if (codePoint <= 0xffff) {
    result.push_back(static_cast<char>(0xe0 | (codePoint >> 12)));
    result.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
    result.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
  } else {
    result.push_back(static_cast<char>(0xf0 | (codePoint >> 18)));
    result.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3f)));
    result.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
    result.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
  }
}

FailureOr<std::string> unicodeCaseFold(StringRef text,
                                       detail::EmitError emitError) {
  std::string result;
  for (size_t offset = 0; offset < text.size();) {
    auto codePoint = decodeUTF8(text, offset, emitError);
    if (failed(codePoint))
      return failure();
    auto found = std::lower_bound(
        std::begin(kSourceOwnerCaseFoldEntries),
        std::end(kSourceOwnerCaseFoldEntries), *codePoint,
        [](const SourceOwnerCaseFoldEntry &entry, uint32_t value) {
          return entry.codePoint < value;
        });
    if (found == std::end(kSourceOwnerCaseFoldEntries) ||
        found->codePoint != *codePoint) {
      appendUTF8(result, *codePoint);
      continue;
    }
    for (uint8_t index = 0; index < found->valueLength; ++index)
      appendUTF8(result,
                 kSourceOwnerCaseFoldValues[found->valueOffset + index]);
  }
  return result;
}

} // namespace

FailureOr<std::string> sourceImportModuleName(DictionaryAttr owner,
                                              detail::EmitError emitError) {
  if (failed(detail::verifySourceOwner(owner, emitError)))
    return failure();
  auto package = owner.getAs<StringAttr>("package");
  auto path = owner.getAs<StringAttr>("path");
  if (!package || !path || !path.getValue().ends_with(".py"))
    return emitError() << "SourceOwner cannot be mapped to an import module";
  SmallVector<StringRef> components;
  path.getValue().drop_back(3).split(components, '/');
  if (!components.empty() && components.back() == "__init__")
    components.pop_back();

  std::string result = package.getValue().str();
  for (StringRef component : components) {
    if (component.empty())
      return emitError() << "SourceOwner has an empty module path component";
    if (!result.empty())
      result.push_back('.');
    result.append(component);
  }
  return result;
}

FailureOr<std::string> sourceImportModuleIdentity(DictionaryAttr owner,
                                                  detail::EmitError emitError) {
  auto module = sourceImportModuleName(owner, emitError);
  if (failed(module))
    return failure();
  return unicodeCaseFold(*module, emitError);
}

FailureOr<DictionaryAttr>
sourceOwnerCaseFoldIdentity(DictionaryAttr owner, detail::EmitError emitError) {
  if (failed(detail::verifySourceOwner(owner, emitError)))
    return failure();
  auto package = owner.getAs<StringAttr>("package");
  auto path = owner.getAs<StringAttr>("path");
  auto foldedPackage = unicodeCaseFold(package.getValue(), emitError);
  auto foldedPath = unicodeCaseFold(path.getValue(), emitError);
  if (failed(foldedPackage) || failed(foldedPath))
    return failure();
  Builder builder(owner.getContext());
  return builder.getDictionaryAttr({
      builder.getNamedAttr("package", builder.getStringAttr(*foldedPackage)),
      builder.getNamedAttr("path", builder.getStringAttr(*foldedPath)),
  });
}

LogicalResult verifyFinalQualifiedSymbol(DictionaryAttr owner, StringRef symbol,
                                         detail::EmitError emitError) {
  auto module = sourceImportModuleName(owner, emitError);
  if (failed(module))
    return failure();
  StringRef local = symbol;
  if (!module->empty()) {
    std::string prefix = (Twine(*module) + ".").str();
    if (!symbol.starts_with(prefix))
      return emitError()
             << "final symbol is outside its SourceOwner module namespace";
    local = symbol.drop_front(prefix.size());
  }
  if (local.empty() || local.contains('.'))
    return emitError()
           << "final declaration requires one canonical local symbol name";
  return success();
}

FailureOr<FlatSymbolRefAttr>
verifyFinalScalarDeclaration(Operation *operation,
                             DictionaryAttr enclosingOwner,
                             detail::EmitError emitError) {
  if (!operation || !isa<TypeAliasOp, ConstantOp>(operation))
    return emitError() << "final declaration unit contains an unsupported op";

  auto name =
      operation->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
  auto owner = operation->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto origin = operation->getAttrOfType<DictionaryAttr>("ac.origin");
  auto role = operation->getAttrOfType<StringAttr>("ac.declaration_role");
  if (!owner || failed(detail::verifySourceOwner(owner, emitError)))
    return emitError() << "final scalar declaration has an invalid SourceOwner";
  if (owner != enclosingOwner)
    return emitError()
           << "final scalar declaration SourceOwner does not match its unit";
  if (operation->getNumOperands() || operation->getNumResults() ||
      operation->getNumRegions() || !name || name.getValue().empty() ||
      !origin || failed(detail::verifyOccurrence(origin, emitError)) || !role ||
      role.getValue() != "definition" ||
      operation->getDiscardableAttrDictionary().size() != 3 ||
      failed(verifyFinalQualifiedSymbol(owner, name.getValue(), emitError)))
    return emitError() << "final scalar declaration envelope is not canonical";

  auto site = origin.getAs<DictionaryAttr>("site");
  auto originDefinition =
      site ? site.getAs<FlatSymbolRefAttr>("definition") : FlatSymbolRefAttr();
  FlatSymbolRefAttr canonical =
      FlatSymbolRefAttr::get(operation->getContext(), name.getValue());
  if (!originDefinition || originDefinition != canonical)
    return emitError()
           << "final declaration origin does not name its canonical symbol";

  auto properties =
      dyn_cast<DictionaryAttr>(operation->getPropertiesAsAttribute());
  if (auto alias = dyn_cast<TypeAliasOp>(operation)) {
    auto kind = alias.getTarget().getAs<StringAttr>("kind");
    if (!properties || properties.size() != 2 ||
        properties.getAs<StringAttr>("sym_name") != name ||
        properties.getAs<DictionaryAttr>("target") != alias.getTarget() ||
        !kind || (kind.getValue() != "bool" && kind.getValue() != "integer") ||
        failed(detail::verifyLogicalTypeStructure(
            alias.getTarget(), [&] { return operation->emitOpError(); })))
      return emitError() << "final type alias must carry a scalar LogicalType";
  } else {
    auto constant = cast<ConstantOp>(operation);
    auto type = constant->getAttrOfType<DictionaryAttr>("type");
    auto value = constant->getAttrOfType<DictionaryAttr>("value");
    auto kind = type ? type.getAs<StringAttr>("kind") : StringAttr();
    if (!properties || properties.size() != 3 ||
        properties.getAs<StringAttr>("sym_name") != name ||
        properties.getAs<DictionaryAttr>("type") != type ||
        properties.getAs<DictionaryAttr>("value") != value || !type || !value ||
        !kind || (kind.getValue() != "bool" && kind.getValue() != "integer") ||
        failed(detail::verifyStaticTypeStructure(
            type, [&] { return operation->emitOpError(); })) ||
        failed(detail::verifyStaticValueStructure(
            value, [&] { return operation->emitOpError(); })))
      return emitError()
             << "final constant must carry a scalar StaticType/StaticValue";
    auto noRecords =
        [&](FlatSymbolRefAttr symbol) -> FailureOr<detail::ResolvedRecordView> {
      return operation->emitOpError()
             << "scalar final constant unexpectedly references record "
             << symbol;
    };
    if (failed(detail::verifyStaticValueMatchesType(
            value, type, detail::ExpectedTypeKind::Static, noRecords,
            [&] { return operation->emitOpError(); })))
      return emitError() << "final scalar constant value is invalid";
  }
  return canonical;
}

} // namespace acir::ac::final_detail

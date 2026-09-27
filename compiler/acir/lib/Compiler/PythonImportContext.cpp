#include "PythonImportContext.h"

#include "acir/Dialect/ACIR/ACIRDialect.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "llvm/ADT/STLExtras.h"

#include <algorithm>

using namespace mlir;

namespace acir::compiler::detail {
namespace {

std::string sourceModuleName(DictionaryAttr owner) {
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

bool pathLess(const AstNode &left, const AstNode &right) {
  size_t count = std::min(left.path.size(), right.path.size());
  for (size_t index = 0; index < count; ++index) {
    const AstStep &leftStep = left.path[index];
    const AstStep &rightStep = right.path[index];
    if (leftStep.index && rightStep.index) {
      if (*leftStep.index != *rightStep.index)
        return *leftStep.index < *rightStep.index;
      continue;
    }
    if (leftStep.index.has_value() != rightStep.index.has_value())
      return !leftStep.index;
    int field = compareBytes(leftStep.field, rightStep.field);
    if (field)
      return field < 0;
  }
  return left.path.size() < right.path.size();
}

bool siteLess(const AstNode &left, const AstNode &right) {
  if (pathLess(left, right))
    return true;
  if (pathLess(right, left))
    return false;
  DictionaryAttr leftSpan = left.value.getAs<DictionaryAttr>("span");
  DictionaryAttr rightSpan = right.value.getAs<DictionaryAttr>("span");
  for (StringRef coordinate : {"start_line", "start_codepoint_column",
                               "end_line", "end_codepoint_column"}) {
    uint64_t leftValue =
        leftSpan.getAs<IntegerAttr>(coordinate).getValue().getZExtValue();
    uint64_t rightValue =
        rightSpan.getAs<IntegerAttr>(coordinate).getValue().getZExtValue();
    if (leftValue != rightValue)
      return leftValue < rightValue;
  }
  return false;
}

bool ownerLess(DictionaryAttr left, DictionaryAttr right) {
  StringRef leftPackage = left.getAs<StringAttr>("package").getValue();
  StringRef rightPackage = right.getAs<StringAttr>("package").getValue();
  int package = compareBytes(leftPackage, rightPackage);
  if (package)
    return package < 0;
  return compareBytes(left.getAs<StringAttr>("path").getValue(),
                      right.getAs<StringAttr>("path").getValue()) < 0;
}

bool isExportableDeclaration(Operation *declaration) {
  if (isa<ac::StructOp, ac::TypeAliasOp>(declaration))
    return true;
  auto function = dyn_cast<func::FuncOp>(declaration);
  auto kind = function ? function->getAttrOfType<StringAttr>("ac.helper_kind")
                       : StringAttr();
  return kind && kind.getValue() == "value";
}

DictionaryAttr namespaceSite(OpBuilder &builder, StringRef sourcePath,
                             const AstNode &node) {
  SmallVector<Attribute> path;
  for (const AstStep &step : node.path) {
    if (step.index)
      path.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("index")),
          builder.getNamedAttr("value", builder.getI64IntegerAttr(*step.index)),
      }));
    else
      path.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("kind", builder.getStringAttr("field")),
          builder.getNamedAttr("name", builder.getStringAttr(step.field)),
      }));
  }
  return builder.getDictionaryAttr({
      builder.getNamedAttr("ast_path", builder.getArrayAttr(path)),
      builder.getNamedAttr("location", sourceSpan(builder, sourcePath, node)),
  });
}

} // namespace

PythonImportContext::PythonImportContext(const CapturedSource &source,
                                         DictionaryAttr owner,
                                         const SourceHeaderRegistry &headers,
                                         ac::detail::EmitError emitError)
    : source(source), owner(owner), headers(headers), emitError(emitError),
      builder(owner.getContext()), module(sourceModuleName(owner)) {}

std::string PythonImportContext::qualifiedName(StringRef name) const {
  return module.empty() ? name.str() : (Twine(module) + "." + name).str();
}

void PythonImportContext::bindNamespaceName(StringRef name,
                                            FlatSymbolRefAttr target,
                                            const AstNode &site) {
  namespaceBindings[name] = {target, site};
}

void PythonImportContext::recordNamespaceImport(DictionaryAttr provider,
                                                StringRef remoteName,
                                                FlatSymbolRefAttr target,
                                                const AstNode &site) {
  namespaceImportUses.push_back({provider, remoteName.str(), target, site});
}

void PythonImportContext::registerLocalDeclaration(FlatSymbolRefAttr symbol,
                                                   Operation *declaration) {
  if (symbol && declaration)
    localDeclarations[symbol.getValue()] = declaration;
}

Operation *PythonImportContext::lookupCanonicalDeclaration(
    FlatSymbolRefAttr symbol) const {
  if (!symbol)
    return nullptr;
  if (Operation *authority = headers.lookupDeclaration(symbol))
    return authority;
  auto local = localDeclarations.find(symbol.getValue());
  return local == localDeclarations.end() ? nullptr : local->second;
}

LogicalResult PythonImportContext::attachNamespaceMetadata() {
  SmallVector<std::pair<StringRef, NamespaceBinding>> orderedExports;
  for (const auto &entry : namespaceBindings) {
    Operation *declaration = lookupCanonicalDeclaration(entry.second.target);
    if (isExportableDeclaration(declaration))
      orderedExports.push_back({entry.first(), entry.second});
  }
  llvm::sort(orderedExports, [](const auto &left, const auto &right) {
    return compareBytes(left.first, right.first) < 0;
  });

  SmallVector<Attribute> exports;
  for (const auto &[name, binding] : orderedExports)
    exports.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("name", builder.getStringAttr(name)),
        builder.getNamedAttr("target", binding.target),
        builder.getNamedAttr("site",
                             namespaceSite(builder, source.path, binding.site)),
    }));

  llvm::sort(namespaceImportUses, [](const NamespaceImportUse &left,
                                     const NamespaceImportUse &right) {
    if (left.source != right.source)
      return ownerLess(left.source, right.source);
    int name = compareBytes(left.name, right.name);
    if (name)
      return name < 0;
    if (siteLess(left.site, right.site))
      return true;
    if (siteLess(right.site, left.site))
      return false;
    return compareBytes(left.target.getValue(), right.target.getValue()) < 0;
  });
  SmallVector<Attribute> importBindings;
  for (const NamespaceImportUse &use : namespaceImportUses)
    importBindings.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("source", use.source),
        builder.getNamedAttr("name", builder.getStringAttr(use.name)),
        builder.getNamedAttr("target", use.target),
        builder.getNamedAttr("site",
                             namespaceSite(builder, source.path, use.site)),
    }));

  ArrayAttr exportArray = builder.getArrayAttr(exports);
  ArrayAttr importArray = builder.getArrayAttr(importBindings);
  body->getOperation()->setAttr("ac.exports", exportArray);
  body->getOperation()->setAttr("ac.import_bindings", importArray);
  interface->getOperation()->setAttr("ac.exports", exportArray);
  interface->getOperation()->setAttr("ac.import_bindings", importArray);
  return success();
}

FailureOr<ac::MathIntAttr> parseStaticInteger(OpBuilder &builder,
                                              StringRef spelling,
                                              ac::detail::EmitError emitError) {
  return ac::detail::parseMathIntAttr(builder.getContext(), spelling,
                                      emitError);
}

DictionaryAttr staticValue(OpBuilder &builder, Attribute raw,
                           ac::detail::EmitError emitError) {
  if (auto boolean = dyn_cast_or_null<BoolAttr>(raw))
    return builder.getDictionaryAttr({
        builder.getNamedAttr("kind", builder.getStringAttr("bool")),
        builder.getNamedAttr("value", boolean),
    });
  auto encoded = dyn_cast_or_null<DictionaryAttr>(raw);
  auto spelling = encoded ? encoded.getAs<StringAttr>("integer") : StringAttr();
  if (!spelling)
    return {};
  auto value = parseStaticInteger(builder, spelling.getValue(), emitError);
  if (failed(value))
    return {};
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("integer")),
      builder.getNamedAttr("value", *value),
  });
}

Type physicalType(DictionaryAttr logical, MLIRContext *context) {
  StringRef kind = logical.getAs<StringAttr>("kind").getValue();
  if (kind == "bool")
    return IntegerType::get(context, 1);
  if (kind == "integer")
    return logical.getAs<TypeAttr>("storage").getValue();
  auto symbol = logical.getAs<FlatSymbolRefAttr>("symbol");
  return symbol ? Type(ac::StructType::get(
                      context, StringAttr::get(context, symbol.getValue())))
                : Type();
}

DictionaryAttr valueConstraint(OpBuilder &builder, DictionaryAttr type) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("logical")),
      builder.getNamedAttr("type", type),
  });
}

DictionaryAttr absentDefault(OpBuilder &builder) {
  return builder.getDictionaryAttr(
      {builder.getNamedAttr("present", builder.getBoolAttr(false))});
}

} // namespace acir::compiler::detail

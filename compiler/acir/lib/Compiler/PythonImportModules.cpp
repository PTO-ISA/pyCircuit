#include "PythonImportModules.h"
#include "PythonImportRules.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {
bool pathLess(const AstNode &left, const AstNode &right) {
  size_t count = std::min(left.path.size(), right.path.size());
  for (size_t index = 0; index < count; ++index) {
    const AstStep &lhs = left.path[index];
    const AstStep &rhs = right.path[index];
    if (lhs.index && rhs.index) {
      if (*lhs.index != *rhs.index)
        return *lhs.index < *rhs.index;
      continue;
    }
    if (lhs.index.has_value() != rhs.index.has_value())
      return !lhs.index;
    if (lhs.field != rhs.field)
      return lhs.field < rhs.field;
  }
  return left.path.size() < right.path.size();
}

DictionaryAttr parameterAttribute(OpBuilder &builder, const ModuleModel &module,
                                  const ModuleParameter &parameter,
                                  StringRef sourcePath) {
  AstNode formal = parameter.syntax.parameter;
  AstNode relative = relativeToModule(module.declaration, formal);
  return builder.getDictionaryAttr({
      builder.getNamedAttr("name", builder.getStringAttr(parameter.name)),
      builder.getNamedAttr("binding",
                           builder.getStringAttr(parameter.syntax.binding)),
      builder.getNamedAttr("category",
                           builder.getStringAttr(parameter.category)),
      builder.getNamedAttr("type", parameter.type),
      builder.getNamedAttr("default", parameter.defaultValue),
      builder.getNamedAttr("origin",
                           occurrence(builder, module.symbol, relative)),
      builder.getNamedAttr("location", sourceSpan(builder, sourcePath, formal)),
  });
}

DictionaryAttr elementEffect(OpBuilder &builder, bool read, bool write,
                             ArrayRef<AstNode> readSites,
                             ArrayRef<AstNode> writeSites, StringRef precision,
                             const ModuleModel &module, StringRef sourcePath) {
  SmallVector<AstNode> sites;
  llvm::append_range(sites, readSites);
  llvm::append_range(sites, writeSites);
  llvm::sort(sites, pathLess);
  SmallVector<Attribute> origins;
  for (auto [index, site] : llvm::enumerate(sites)) {
    if (index && !pathLess(sites[index - 1], site) &&
        !pathLess(site, sites[index - 1]))
      continue;
    origins.push_back(occurrence(builder, module.symbol,
                                 relativeToModule(module.declaration, site)));
  }
  return builder.getDictionaryAttr({
      builder.getNamedAttr("ordinal", builder.getUnitAttr()),
      builder.getNamedAttr("read", builder.getBoolAttr(read)),
      builder.getNamedAttr("write", builder.getBoolAttr(write)),
      builder.getNamedAttr("precision", builder.getStringAttr(precision)),
      builder.getNamedAttr("origins", builder.getArrayAttr(origins)),
  });
}

} // namespace

AstNode relativeToModule(const AstNode &anchor, const AstNode &node) {
  AstNode relative = node;
  if (anchor.path.size() <= relative.path.size()) {
    bool prefix = true;
    for (size_t index = 0; index < anchor.path.size(); ++index) {
      const AstStep &left = anchor.path[index];
      const AstStep &right = relative.path[index];
      if (left.field != right.field || left.index != right.index) {
        prefix = false;
        break;
      }
    }
    if (prefix)
      relative.path.erase(relative.path.begin(),
                          relative.path.begin() + anchor.path.size());
  }
  return relative;
}

LogicalResult
validateChildParameterCategories(ArrayAttr parameters,
                                 ac::detail::EmitError emitError) {
  for (Attribute raw : parameters) {
    auto parameter = dyn_cast<DictionaryAttr>(raw);
    auto category =
        parameter ? parameter.getAs<StringAttr>("category") : StringAttr();
    if (!category)
      return emitError() << "module instance parameter category is malformed";
    if (category.getValue() == "static")
      return emitError() << "U02-A module child static parameters are not "
                            "supported";
    if (category.getValue() != "connection")
      return emitError() << "module instance parameter category is unsupported";
  }
  return success();
}

LogicalResult ModuleCompiler::run() {
  auto module = classify();
  if (failed(module))
    return failure();
  RuleCompiler rules(sourceCompiler, *module);
  if (failed(rules.analyze()) || failed(rules.validateInactiveMethods()) ||
      failed(emitBody(*module, rules)))
    return failure();
  return emitHeader(*module);
}

LogicalResult ModuleCompiler::emitHeader(ModuleModel &model) {
  OpBuilder &builder = sourceCompiler.builder;
  SmallVector<Attribute> parameterAttributes;
  SmallVector<Attribute> connectionAttributes;
  for (const ModuleParameter &parameter : model.parameters) {
    parameterAttributes.push_back(parameterAttribute(
        builder, model, parameter, sourceCompiler.source.path));
    auto member = llvm::find_if(model.members, [&](const ModuleMember &entry) {
      return entry.kind == ModuleMember::Kind::Connection &&
             entry.parameter == parameter.name;
    });
    if (member == model.members.end())
      return sourceCompiler.emitError()
             << "module import contract has an unbound connection parameter";
    auto kind = member->logicalType.getAs<StringAttr>("kind");
    if (!kind || kind.getValue() == "list")
      return sourceCompiler.emitError()
             << "U02-A module connections support scalar and record values";
    DictionaryAttr effect =
        elementEffect(builder, member->read, member->write, member->readSites,
                      member->writeSites, member->precision, model,
                      sourceCompiler.source.path);
    connectionAttributes.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("parameter",
                             builder.getStringAttr(parameter.name)),
        builder.getNamedAttr("elements", builder.getArrayAttr({effect})),
    }));
  }
  DictionaryAttr contract = builder.getDictionaryAttr({
      builder.getNamedAttr("parameters",
                           builder.getArrayAttr(parameterAttributes)),
      builder.getNamedAttr("connections",
                           builder.getArrayAttr(connectionAttributes)),
  });
  SmallVector<NamedAttribute> attributes{
      builder.getNamedAttr(SymbolTable::getSymbolAttrName(),
                           builder.getStringAttr(model.symbol.getValue())),
      builder.getNamedAttr("ac.source_owner", sourceCompiler.owner),
      builder.getNamedAttr(
          "ac.origin",
          occurrence(builder, model.symbol,
                     relativeToModule(model.declaration, model.declaration))),
      builder.getNamedAttr("ac.declaration_role",
                           builder.getStringAttr("definition")),
      builder.getNamedAttr("ac.contract", contract),
  };
  builder.setInsertionPointToEnd(sourceCompiler.interface->getBody());
  Operation *header = createSourceOperation(
      builder,
      model.declaration.location(builder.getContext(),
                                 sourceCompiler.source.path),
      ac::ModuleImportOp::getOperationName(), {}, {}, attributes);
  sourceCompiler.registerLocalDeclaration(model.symbol, header);
  return success();
}

} // namespace acir::compiler::detail

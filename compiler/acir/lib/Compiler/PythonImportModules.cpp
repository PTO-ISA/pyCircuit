#include "PythonImportModules.h"
#include "PythonImportRules.h"
#include "RuleEffectView.h"

#include "acir/Dialect/ACIR/ACIROps.h"
#include "mlir/IR/Builders.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {
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

DictionaryAttr elementEffect(OpBuilder &builder, const RuleEffect &effect) {
  SmallVector<Attribute> origins;
  llvm::append_range(origins, effect.origins);
  return builder.getDictionaryAttr({
      builder.getNamedAttr("ordinal", builder.getUnitAttr()),
      builder.getNamedAttr("read", builder.getBoolAttr(effect.read)),
      builder.getNamedAttr("write", builder.getBoolAttr(effect.write)),
      builder.getNamedAttr("precision",
                           builder.getStringAttr(effect.precision)),
      builder.getNamedAttr("origins", builder.getArrayAttr(origins)),
  });
}

DictionaryAttr portAttribute(OpBuilder &builder, const ModuleModel &module,
                             const ModuleParameter &parameter, StringRef role,
                             StringRef sourcePath) {
  AstNode formal = parameter.syntax.parameter;
  return builder.getDictionaryAttr({
      builder.getNamedAttr("parameter", builder.getStringAttr(parameter.name)),
      builder.getNamedAttr("ordinal", builder.getUnitAttr()),
      builder.getNamedAttr("role", builder.getStringAttr(role)),
      builder.getNamedAttr("type", parameter.type),
      builder.getNamedAttr(
          "origin", occurrence(builder, module.symbol,
                               relativeToModule(module.declaration, formal))),
      builder.getNamedAttr("location", sourceSpan(builder, sourcePath, formal)),
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
  ac::ModuleOp bodyModule;
  for (ac::ModuleOp candidate :
       sourceCompiler.body->getBody()->getOps<ac::ModuleOp>())
    if (candidate.getSymName() == model.symbol.getValue()) {
      bodyModule = candidate;
      break;
    }
  if (!bodyModule)
    return sourceCompiler.emitError()
           << "module body is absent before effect inference";
  auto inferred =
      inferSourceModuleEffects(bodyModule, sourceCompiler.emitError);
  if (failed(inferred))
    return failure();

  SmallVector<Attribute> parameterAttributes;
  SmallVector<Attribute> connectionAttributes;
  SmallVector<Attribute> currentPorts;
  SmallVector<Attribute> nextPorts;
  llvm::DenseSet<Attribute> consumedEffects;
  for (const ModuleParameter &parameter : model.parameters) {
    parameterAttributes.push_back(parameterAttribute(
        builder, model, parameter, sourceCompiler.source.path));
    auto effect =
        llvm::find_if(inferred->effects, [&](const RuleEffect &entry) {
          auto kind = entry.state.getAs<StringAttr>("kind");
          auto name = entry.state.getAs<StringAttr>("parameter");
          return kind && kind.getValue() == "formal" && name &&
                 name.getValue() == parameter.name &&
                 isa<UnitAttr>(entry.state.get("ordinal"));
        });
    if (effect == inferred->effects.end())
      return sourceCompiler.emitError()
             << "module connection has no effect inferred from its body";
    if (!consumedEffects.insert(effect->state).second)
      return sourceCompiler.emitError()
             << "module connection effect is not uniquely inferred";
    if (effect->logicalType != parameter.type)
      return sourceCompiler.emitError()
             << "module connection effect LogicalType disagrees with its "
                "declaration";
    auto kind = parameter.type.getAs<StringAttr>("kind");
    if (!kind || kind.getValue() == "list")
      return sourceCompiler.emitError()
             << "U02-A module connections support scalar and record values";
    if (effect->read)
      currentPorts.push_back(portAttribute(builder, model, parameter, "current",
                                           sourceCompiler.source.path));
    if (effect->write)
      nextPorts.push_back(portAttribute(builder, model, parameter, "next",
                                        sourceCompiler.source.path));
    connectionAttributes.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("parameter",
                             builder.getStringAttr(parameter.name)),
        builder.getNamedAttr("elements", builder.getArrayAttr({elementEffect(
                                             builder, *effect)})),
    }));
  }
  for (const RuleEffect &effect : inferred->effects) {
    auto kind = effect.state.getAs<StringAttr>("kind");
    if (!kind)
      return sourceCompiler.emitError()
             << "inferred rule effect has no StateRef kind";
    if (kind.getValue() == "owned")
      continue;
    if (kind.getValue() != "formal" || !consumedEffects.contains(effect.state))
      return sourceCompiler.emitError()
             << "inferred formal effect has no declared module parameter";
  }
  SmallVector<Attribute> expectedPorts;
  llvm::append_range(expectedPorts, currentPorts);
  llvm::append_range(expectedPorts, nextPorts);
  if (bodyModule->getAttrOfType<ArrayAttr>("ac.ports") !=
      builder.getArrayAttr(expectedPorts))
    return sourceCompiler.emitError()
           << "module body ports disagree with inferred source effects";

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
      builder.getNamedAttr(
          "ac.control_ports",
          builder.getDictionaryAttr({
              builder.getNamedAttr("clock", builder.getI32IntegerAttr(0)),
              builder.getNamedAttr("reset", builder.getI32IntegerAttr(1)),
          })),
      builder.getNamedAttr("ac.contract", contract),
  };
  if (model.isSystem)
    attributes.push_back(
        builder.getNamedAttr("ac.root_kind", builder.getStringAttr("system")));
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

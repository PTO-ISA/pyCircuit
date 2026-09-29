#include "ModuleGraph.h"

#include "PythonImportContext.h"
#include "RuleEffectView.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"

#include <functional>

using namespace mlir;

namespace acir::compiler {
namespace {

struct DefinitionView {
  ac::ModuleOp module;
  ac::ModuleImportOp header;
};

DictionaryAttr ownerRef(Builder &builder, ArrayRef<Attribute> path) {
  return builder.getDictionaryAttr(
      {builder.getNamedAttr("instance_path", builder.getArrayAttr(path))});
}

DictionaryAttr stateID(Builder &builder, DictionaryAttr owner,
                       DictionaryAttr declaration, ArrayAttr element) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("owner", owner),
      builder.getNamedAttr("declaration", declaration),
      builder.getNamedAttr("element", element),
  });
}

DictionaryAttr formalState(Builder &builder, DictionaryAttr port) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("formal")),
      builder.getNamedAttr("parameter", port.get("parameter")),
      builder.getNamedAttr("ordinal", port.get("ordinal")),
  });
}

DictionaryAttr ownedState(Builder &builder, ac::RegOp reg) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("owned")),
      builder.getNamedAttr("declaration", reg->getAttr("ac.declaration")),
      builder.getNamedAttr("element", reg->getAttr("ac.element")),
  });
}

ArrayAttr stateElement(Builder &builder, Attribute ordinal) {
  if (isa<UnitAttr>(ordinal))
    return builder.getArrayAttr({});
  return builder.getArrayAttr({ordinal});
}

} // namespace

FailureOr<DictionaryAttr>
ModuleGraph::resolveState(const InstanceView &owner, DictionaryAttr state,
                          ac::detail::EmitError emitError) const {
  if (failed(ac::detail::verifyStateRef(state, emitError)))
    return failure();
  auto kind = state.getAs<StringAttr>("kind");
  const auto &aliases =
      kind.getValue() == "owned" ? owner.ownedStates : owner.formalAliases;
  auto found = aliases.find(state);
  if (found == aliases.end())
    return emitError() << "module-relative StateRef has no instance StateID";
  return found->second;
}

FailureOr<ModuleGraph>
buildSourceModuleGraph(ArrayRef<SourceLinkUnit> units,
                       const SourceHeaderRegistry &registry,
                       ac::detail::EmitError emitError) {
  if (units.empty())
    return emitError() << "module graph requires complete source units";
  ModuleGraph graph;
  ModuleOp firstBody = units.front().body;
  Builder builder(firstBody.getContext());
  DenseMap<Attribute, DefinitionView> definitions;
  DenseMap<Attribute, unsigned> incoming;

  for (SourceLinkUnit unit : units) {
    auto bodyKind = unit.body->getAttrOfType<StringAttr>("ac.unit_kind");
    if (!bodyKind || bodyKind.getValue() == "declarations")
      continue;
    SmallVector<ac::ModuleOp> modules(
        unit.body.getBody()->getOps<ac::ModuleOp>());
    if (modules.size() != 1)
      return emitError()
             << "module graph implementation unit must own one module";
    ac::ModuleOp module = modules.front();
    auto symbol =
        FlatSymbolRefAttr::get(module.getContext(), module.getSymName());
    auto declaration = dyn_cast_or_null<ac::ModuleImportOp>(
        registry.lookupDeclaration(symbol));
    if (!declaration ||
        declaration->getParentOfType<mlir::ModuleOp>() != unit.header)
      return emitError()
             << "module graph definition lacks its owning header authority";
    if (!definitions.try_emplace(symbol, DefinitionView{module, declaration})
             .second)
      return emitError() << "module graph repeats one module definition";
    incoming.try_emplace(symbol, 0);
  }
  if (definitions.empty())
    return emitError() << "module graph has no executable definition";

  for (auto &[symbol, definition] : definitions)
    for (ac::InstanceOp instance :
         definition.module.getBody().front().getOps<ac::InstanceOp>()) {
      auto callee = dyn_cast<FlatSymbolRefAttr>(instance.getCalleeAttr());
      if (!callee || !definitions.contains(callee))
        return emitError()
               << "module graph instance has no complete callee body";
      ++incoming[callee];
    }

  SmallVector<Attribute> systemRoots;
  SmallVector<Attribute> zeroIncoming;
  for (const auto &[symbol, definition] : definitions) {
    if (definition.module->getAttrOfType<StringAttr>("ac.root_kind") ||
        definition.header->getAttrOfType<StringAttr>("ac.root_kind"))
      systemRoots.push_back(symbol);
    if (incoming.lookup(symbol) == 0)
      zeroIncoming.push_back(symbol);
  }
  Attribute rootDefinition;
  if (!systemRoots.empty()) {
    if (systemRoots.size() != 1 || incoming.lookup(systemRoots.front()) != 0)
      return emitError() << "module graph requires one unowned system root";
    rootDefinition = systemRoots.front();
  } else {
    if (zeroIncoming.size() != 1)
      return emitError() << "module graph requires one zero-parent root";
    rootDefinition = zeroIncoming.front();
  }

  DenseMap<Attribute, InstanceView *> owners;
  DenseSet<Attribute> activeDefinitions;
  DenseSet<Attribute> reachableDefinitions;
  auto recordState = [&](DictionaryAttr id,
                         DictionaryAttr logical) -> LogicalResult {
    auto inserted = graph.stateTypes_.try_emplace(id, logical);
    if (!inserted.second && inserted.first->second != logical)
      return emitError() << "one StateID has inconsistent LogicalTypes";
    return success();
  };

  std::function<FailureOr<InstanceView *>(Attribute, InstanceView *,
                                          ac::InstanceOp, ArrayRef<Attribute>,
                                          DenseMap<Attribute, DictionaryAttr>)>
      buildView;
  buildView = [&](Attribute definitionKey, InstanceView *parent,
                  ac::InstanceOp placement, ArrayRef<Attribute> ownerPath,
                  DenseMap<Attribute, DictionaryAttr> formalAliases)
      -> FailureOr<InstanceView *> {
    auto definition = definitions.find(definitionKey);
    if (definition == definitions.end())
      return emitError() << "module graph view has no definition";
    if (!activeDefinitions.insert(definitionKey).second)
      return emitError() << "recursive module graph is unsupported";
    reachableDefinitions.insert(definitionKey);

    auto viewStorage = std::make_unique<InstanceView>();
    InstanceView *view = viewStorage.get();
    view->owner = ownerRef(builder, ownerPath);
    view->definition = cast<FlatSymbolRefAttr>(definitionKey);
    view->staticArguments =
        placement ? placement->getAttrOfType<ArrayAttr>("ac.static_args")
                  : builder.getArrayAttr({});
    view->module = definition->second.module;
    view->header = definition->second.header;
    view->placement = placement;
    view->parent = parent;
    view->formalAliases = std::move(formalAliases);
    auto ownerPosition = owners.try_emplace(view->owner, view);
    if (!ownerPosition.second)
      return emitError() << (ownerPosition.first->second->parent == parent
                                 ? "module graph has a duplicate owner"
                                 : "module graph child has two parents");
    graph.storage_.push_back(std::move(viewStorage));
    graph.views.push_back(view);
    if (parent)
      parent->children.push_back(view);

    Block &body = view->module.getBody().front();
    for (ac::RegOp reg : body.getOps<ac::RegOp>()) {
      DictionaryAttr relative = ownedState(builder, reg);
      DictionaryAttr id =
          stateID(builder, view->owner,
                  reg->getAttrOfType<DictionaryAttr>("ac.declaration"),
                  reg->getAttrOfType<ArrayAttr>("ac.element"));
      if (!view->ownedStates.try_emplace(relative, id).second)
        return emitError() << "module graph repeats an owned StateRef";
      view->ownedStateIDs.push_back(id);
      if (failed(recordState(
              id, reg->getAttrOfType<DictionaryAttr>("ac.logical_element"))))
        return failure();
    }

    auto ports = view->module->getAttrOfType<ArrayAttr>("ac.ports");
    if (!ports || body.getNumArguments() != ports.size() + 2)
      return emitError() << "module graph module ports are not closed";
    for (Attribute rawPort : ports) {
      auto port = dyn_cast<DictionaryAttr>(rawPort);
      auto origin =
          port ? port.getAs<DictionaryAttr>("origin") : DictionaryAttr();
      auto logical =
          port ? port.getAs<DictionaryAttr>("type") : DictionaryAttr();
      Attribute ordinal = port ? port.get("ordinal") : Attribute();
      if (!port || !origin || !logical || !ordinal)
        return emitError() << "module graph has a malformed PortSlot";
      DictionaryAttr relative = formalState(builder, port);
      if (!parent) {
        DictionaryAttr id = stateID(builder, view->owner, origin,
                                    stateElement(builder, ordinal));
        auto inserted = view->formalAliases.try_emplace(relative, id);
        if (!inserted.second && inserted.first->second != id)
          return emitError() << "root port identity is inconsistent";
        if (failed(recordState(id, logical)))
          return failure();
      } else {
        auto alias = view->formalAliases.find(relative);
        if (alias == view->formalAliases.end())
          return emitError() << "child PortSlot has no actual StateID alias";
        auto type = graph.stateTypes_.find(alias->second);
        if (type == graph.stateTypes_.end() || type->second != logical)
          return emitError()
                 << "child PortSlot alias has the wrong LogicalType";
      }
    }

    auto resolveActual =
        [&](Value handle,
            DictionaryAttr expected) -> FailureOr<DictionaryAttr> {
      auto physical = dyn_cast<ac::RegType>(handle.getType());
      if (!physical ||
          physical.getElementType() !=
              detail::physicalType(expected, view->module.getContext()))
        return emitError() << "instance actual has the wrong physical type";
      if (auto reg = handle.getDefiningOp<ac::RegOp>()) {
        if (reg->getParentOfType<ac::ModuleOp>() != view->module ||
            reg->getBlock() != &body)
          return emitError()
                 << "instance actual leaks private or foreign state";
        return graph.resolveState(*view, ownedState(builder, reg), emitError);
      }
      auto argument = dyn_cast<BlockArgument>(handle);
      if (!argument || argument.getOwner() != &body ||
          argument.getArgNumber() < 2 ||
          argument.getArgNumber() - 2 >= ports.size())
        return emitError()
               << "instance actual is not parent-owned or parent-formal state";
      auto port = cast<DictionaryAttr>(ports[argument.getArgNumber() - 2]);
      return graph.resolveState(*view, formalState(builder, port), emitError);
    };

    for (ac::InstanceOp instance : body.getOps<ac::InstanceOp>()) {
      if (instance.getClock() != body.getArgument(0) ||
          instance.getReset() != body.getArgument(1))
        return emitError()
               << "instance controls do not identity-forward parent controls";
      auto callee = dyn_cast<FlatSymbolRefAttr>(instance.getCalleeAttr());
      auto childDefinition =
          callee ? definitions.find(callee) : definitions.end();
      if (childDefinition == definitions.end() ||
          childDefinition->second.header->getAttr("ac.root_kind"))
        return emitError() << "instance callee is missing or is a system root";
      auto contract =
          childDefinition->second.header->getAttrOfType<DictionaryAttr>(
              "ac.contract");
      auto parameters =
          contract ? contract.getAs<ArrayAttr>("parameters") : ArrayAttr();
      auto connections =
          contract ? contract.getAs<ArrayAttr>("connections") : ArrayAttr();
      if (!parameters || !connections)
        return emitError() << "instance callee contract is incomplete";
      llvm::StringMap<DictionaryAttr> parameterTypes;
      for (Attribute raw : parameters) {
        auto parameter = dyn_cast<DictionaryAttr>(raw);
        auto name =
            parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
        auto category =
            parameter ? parameter.getAs<StringAttr>("category") : StringAttr();
        auto type = parameter ? parameter.getAs<DictionaryAttr>("type")
                              : DictionaryAttr();
        if (name && category && category.getValue() == "connection" && type)
          parameterTypes.try_emplace(name.getValue(), type);
      }
      size_t inputIndex = 0;
      size_t targetIndex = 0;
      DenseSet<Attribute> writableActuals;
      DenseMap<Attribute, DictionaryAttr> childAliases;
      for (Attribute raw : connections) {
        auto connection = dyn_cast<DictionaryAttr>(raw);
        auto parameter = connection ? connection.getAs<StringAttr>("parameter")
                                    : StringAttr();
        auto elements =
            connection ? connection.getAs<ArrayAttr>("elements") : ArrayAttr();
        auto type = parameter ? parameterTypes.find(parameter.getValue())
                              : parameterTypes.end();
        if (!connection || !parameter || !elements || elements.size() != 1 ||
            type == parameterTypes.end())
          return emitError()
                 << "module graph supports closed scalar child connections";
        auto effect = dyn_cast<DictionaryAttr>(elements[0]);
        auto read = effect ? effect.getAs<BoolAttr>("read") : BoolAttr();
        auto write = effect ? effect.getAs<BoolAttr>("write") : BoolAttr();
        if (!effect || !read || !write)
          return emitError() << "child connection effect is malformed";
        DictionaryAttr readID;
        DictionaryAttr writeID;
        if (read.getValue()) {
          if (inputIndex >= instance.getInputs().size())
            return emitError() << "child current port has no actual";
          auto resolved =
              resolveActual(instance.getInputs()[inputIndex++], type->second);
          if (failed(resolved))
            return failure();
          readID = *resolved;
        }
        if (write.getValue()) {
          if (targetIndex >= instance.getTargets().size())
            return emitError() << "child next port has no actual";
          auto resolved =
              resolveActual(instance.getTargets()[targetIndex++], type->second);
          if (failed(resolved))
            return failure();
          writeID = *resolved;
          if (!writableActuals.insert(writeID).second)
            return emitError()
                   << "two write-capable child ports alias one StateID";
        }
        if (readID && writeID && readID != writeID)
          return emitError()
                 << "child current/next endpoints disagree on actual StateID";
        DictionaryAttr actual = readID ? readID : writeID;
        DictionaryAttr relative = builder.getDictionaryAttr({
            builder.getNamedAttr("kind", builder.getStringAttr("formal")),
            builder.getNamedAttr("parameter", parameter),
            builder.getNamedAttr("ordinal", effect.get("ordinal")),
        });
        if (!actual || !childAliases.try_emplace(relative, actual).second)
          return emitError() << "child formal alias is missing or repeated";
      }
      if (inputIndex != instance.getInputs().size() ||
          targetIndex != instance.getTargets().size())
        return emitError()
               << "instance actual arity exceeds its child contract";

      SmallVector<Attribute> childPath(ownerPath.begin(), ownerPath.end());
      auto instanceOrigin =
          instance->getAttrOfType<DictionaryAttr>("ac.origin");
      if (!instanceOrigin)
        return emitError() << "instance owner occurrence is missing";
      childPath.push_back(instanceOrigin);
      auto child =
          buildView(callee, view, instance, childPath, std::move(childAliases));
      if (failed(child))
        return failure();
    }

    activeDefinitions.erase(definitionKey);
    graph.postOrder_.push_back(view);
    return view;
  };

  SmallVector<Attribute> rootPath;
  auto root = buildView(rootDefinition, nullptr, {}, rootPath,
                        DenseMap<Attribute, DictionaryAttr>());
  if (failed(root))
    return failure();
  graph.root = *root;
  if (reachableDefinitions.size() != definitions.size())
    return emitError() << "module graph contains an unreachable definition";

  for (InstanceView *view : graph.postOrder_) {
    auto effects = detail::inferSourceModuleEffects(view->module, emitError);
    if (failed(effects))
      return failure();
    DenseMap<Attribute, Attribute> writerByState;
    for (const detail::RuleEffect &effect : effects->effects) {
      auto id = graph.resolveState(*view, effect.state, emitError);
      if (failed(id))
        return failure();
      auto type = graph.stateTypes_.find(*id);
      if (type == graph.stateTypes_.end() || type->second != effect.logicalType)
        return emitError()
               << "bottom-up effect type disagrees with resolved StateID";
      if (!effect.write)
        continue;
      auto inserted = writerByState.try_emplace(*id, effect.state);
      if (!inserted.second && inserted.first->second != effect.state)
        return emitError() << "write-capable aliases resolve to one StateID";
    }
  }
  return graph;
}

} // namespace acir::compiler

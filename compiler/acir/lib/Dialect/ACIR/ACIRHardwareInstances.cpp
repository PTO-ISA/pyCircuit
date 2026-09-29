#include "ACIRHardwareClosureDetail.h"

#include "llvm/ADT/STLExtras.h"

#include <functional>

using namespace mlir;

namespace acir::ac::hardware_detail {
namespace {

DictionaryAttr ownerRef(Builder &builder, ArrayRef<Attribute> path) {
  return builder.getDictionaryAttr(
      {builder.getNamedAttr("instance_path", builder.getArrayAttr(path))});
}

FailureOr<DictionaryAttr> relativeHandle(View &view, Value handle,
                                         ac::detail::EmitError emitError) {
  Builder builder(view.module.getContext());
  Block &body = view.module.getBody().front();
  if (auto reg = handle.getDefiningOp<RegOp>()) {
    if (reg->getParentOfType<ModuleOp>() != view.module ||
        reg->getBlock() != &body)
      return emitError() << "final instance leaks foreign owned state";
    return ownedRef(builder, reg);
  }
  auto argument = dyn_cast<BlockArgument>(handle);
  auto ports = view.module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!argument || argument.getOwner() != &body ||
      argument.getArgNumber() < 2 || !ports ||
      argument.getArgNumber() - 2 >= ports.size())
    return emitError() << "final instance actual is not a module state handle";
  return formalRef(builder,
                   cast<DictionaryAttr>(ports[argument.getArgNumber() - 2]));
}

LogicalResult verifyPlacement(View &parent, InstanceOp placement,
                              Definition childDefinition,
                              DenseMap<Attribute, DictionaryAttr> &aliases,
                              ac::detail::EmitError emitError) {
  Builder builder(parent.module.getContext());
  Block &parentBody = parent.module.getBody().front();
  Block &childBody = childDefinition.module.getBody().front();
  auto ports = childDefinition.module->getAttrOfType<ArrayAttr>("ac.ports");
  auto bindings = placement->getAttrOfType<ArrayAttr>("ac.port_bindings");
  auto origin = placement->getAttrOfType<DictionaryAttr>("ac.origin");
  if (placement.getName().empty() ||
      failed(detail::verifyOccurrence(origin, emitError)) ||
      placement.getClock() != parentBody.getArgument(0) ||
      placement.getReset() != parentBody.getArgument(1) || !ports ||
      !bindings || bindings.size() != ports.size() ||
      childBody.getNumArguments() != ports.size() + 2 ||
      placement->getAttrs().size() != 5 || placement->hasAttr("ac.static_args"))
    return emitError() << "final instance envelope/port arity is invalid";
  size_t current = 0, next = 0;
  for (auto [index, rawPort] : llvm::enumerate(ports)) {
    auto port = cast<DictionaryAttr>(rawPort);
    auto role = port.getAs<StringAttr>("role");
    Value actual;
    if (role && role.getValue() == "current") {
      if (current >= placement.getInputs().size())
        return emitError() << "final instance lacks a current actual";
      actual = placement.getInputs()[current++];
    } else if (role && role.getValue() == "next") {
      if (next >= placement.getTargets().size())
        return emitError() << "final instance lacks a next actual";
      actual = placement.getTargets()[next++];
    } else {
      return emitError() << "final child PortSlot role is invalid";
    }
    auto binding = dyn_cast<DictionaryAttr>(bindings[index]);
    auto portIndex =
        binding ? binding.getAs<IntegerAttr>("port") : IntegerAttr();
    auto decoded =
        detail::decodeU32(portIndex, "final PortBinding index", emitError);
    auto target =
        binding ? binding.getAs<DictionaryAttr>("target") : DictionaryAttr();
    auto expectedTarget = relativeHandle(parent, actual, emitError);
    auto state = resolveHandle(parent, actual, emitError);
    if (!binding || binding.size() != 2 || failed(decoded) ||
        *decoded != index ||
        failed(detail::verifyStateRef(target, emitError)) ||
        failed(expectedTarget) || target != *expectedTarget || failed(state))
      return emitError() << "final PortBinding differs from its actual operand";
    if (actual.getType() != childBody.getArgument(index + 2).getType())
      return emitError()
             << "final instance actual type differs from child port";
    auto inserted = aliases.try_emplace(formalRef(builder, port), *state);
    if (!inserted.second && inserted.first->second != *state)
      return emitError() << "final child current/next formal aliases disagree";
  }
  if (current != placement.getInputs().size() ||
      next != placement.getTargets().size() ||
      placement.getNextValues().size() != 2 * next)
    return emitError() << "final instance actual/result inventory is stale";
  size_t result = 0;
  for (auto [portIndex, rawPort] : llvm::enumerate(ports)) {
    auto port = cast<DictionaryAttr>(rawPort);
    if (port.getAs<StringAttr>("role").getValue() != "next")
      continue;
    Type payload = cast<RegType>(childBody.getArgument(2 + portIndex).getType())
                       .getElementType();
    if (placement.getNextValues()[result].getType() != payload ||
        !placement.getNextValues()[result + 1].getType().isInteger(1))
      return emitError() << "final child next result type is invalid";
    result += 2;
  }
  return success();
}

} // namespace

FailureOr<DictionaryAttr> resolveHandle(View &view, Value handle,
                                        ac::detail::EmitError emitError) {
  auto relative = relativeHandle(view, handle, emitError);
  if (failed(relative))
    return failure();
  auto found = view.states.find(*relative);
  if (found == view.states.end())
    return emitError() << "final state handle has no reconstructed StateID";
  return found->second;
}

LogicalResult rebuildInstances(Closure &closure,
                               ac::detail::EmitError emitError) {
  Builder builder(closure.package.getContext());
  DenseSet<Attribute> owners;
  std::function<FailureOr<View *>(DictionaryAttr, View *, InstanceOp,
                                  ArrayRef<Attribute>,
                                  DenseMap<Attribute, DictionaryAttr>)>
      build;
  build =
      [&](DictionaryAttr key, View *parent, InstanceOp placement,
          ArrayRef<Attribute> path,
          DenseMap<Attribute, DictionaryAttr> aliases) -> FailureOr<View *> {
    auto definition = closure.definitions.find(key);
    if (definition == closure.definitions.end())
      return emitError() << "final instance key has no module definition";
    if (!closure.activeDefinitions.insert(key).second)
      return emitError() << "final hardware instance tree is recursive";
    closure.reachableDefinitions.insert(key);
    auto storage = std::make_unique<View>();
    View *view = storage.get();
    view->owner = ownerRef(builder, path);
    view->key = key;
    view->module = definition->second.module;
    view->placement = placement;
    view->parent = parent;
    view->states = std::move(aliases);
    if (!owners.insert(view->owner).second)
      return emitError() << "final hardware repeats one OwnerRef";
    closure.storage.push_back(std::move(storage));
    closure.views.push_back(view);
    if (parent)
      parent->children.push_back(view);
    Block &body = view->module.getBody().front();
    for (RegOp reg : body.getOps<RegOp>()) {
      DictionaryAttr relative = ownedRef(builder, reg);
      DictionaryAttr id = stateID(builder, view->owner, relative);
      if (!view->states.try_emplace(relative, id).second)
        return emitError() << "final owner repeats one state identity";
    }
    auto ports = view->module->getAttrOfType<ArrayAttr>("ac.ports");
    if (!ports || body.getNumArguments() != ports.size() + 2)
      return emitError() << "final module ports do not match its block";
    if (!parent && !ports.empty())
      return emitError() << "final hardware root must be portless";
    view->portStates.reserve(ports.size());
    for (Attribute rawPort : ports) {
      DictionaryAttr relative =
          formalRef(builder, cast<DictionaryAttr>(rawPort));
      auto found = view->states.find(relative);
      if (found == view->states.end())
        return emitError() << "final child port has no StateID alias";
      view->portStates.push_back(found->second);
    }
    for (InstanceOp child : body.getOps<InstanceOp>()) {
      auto childKey = specKey(child.getCalleeAttr(), child);
      if (failed(childKey))
        return failure();
      auto childDefinition = closure.definitions.find(*childKey);
      if (childDefinition == closure.definitions.end())
        return emitError() << "final child has no executable definition";
      DenseMap<Attribute, DictionaryAttr> childAliases;
      if (failed(verifyPlacement(*view, child, childDefinition->second,
                                 childAliases, emitError)))
        return failure();
      SmallVector<Attribute> childPath(path.begin(), path.end());
      childPath.push_back(child->getAttrOfType<DictionaryAttr>("ac.origin"));
      auto childView =
          build(*childKey, view, child, childPath, std::move(childAliases));
      if (failed(childView))
        return failure();
    }
    closure.activeDefinitions.erase(key);
    return view;
  };

  DenseMap<Attribute, DictionaryAttr> noAliases;
  auto root = build(closure.entry, nullptr, {}, ArrayRef<Attribute>{},
                    std::move(noAliases));
  if (failed(root) ||
      closure.reachableDefinitions.size() != closure.definitions.size())
    return emitError()
           << "final package has missing root or unreachable definitions";
  auto systemOwner =
      closure.system->getAttrOfType<DictionaryAttr>("ac.source_owner");
  auto systemOrigin =
      closure.system->getAttrOfType<DictionaryAttr>("ac.origin");
  if (systemOwner !=
          (*root)->module->getAttrOfType<DictionaryAttr>("ac.source_owner") ||
      systemOrigin !=
          (*root)->module->getAttrOfType<DictionaryAttr>("ac.origin"))
    return emitError() << "ac.system owner/origin differs from selected root";
  return success();
}

} // namespace acir::ac::hardware_detail

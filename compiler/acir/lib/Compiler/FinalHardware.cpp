#include "FinalHardware.h"

#include "Dialect/ACIR/ACIRHardwareClosure.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::compiler {
namespace {

DictionaryAttr specKey(Builder &builder, FlatSymbolRefAttr definition,
                       ArrayAttr arguments) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("definition", definition),
      builder.getNamedAttr("arguments", arguments),
  });
}

DictionaryAttr formalState(Builder &builder, DictionaryAttr port) {
  auto ordinal = port.get("ordinal");
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("formal")),
      builder.getNamedAttr("parameter", port.get("parameter")),
      builder.getNamedAttr("ordinal", ordinal),
  });
}

DictionaryAttr ownedState(Builder &builder, ac::RegOp reg) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("owned")),
      builder.getNamedAttr("declaration", reg->getAttr("ac.declaration")),
      builder.getNamedAttr("element", reg->getAttr("ac.element")),
  });
}

FailureOr<DictionaryAttr> stateRef(ac::ModuleOp module, Value handle,
                                   ac::detail::EmitError emitError) {
  Builder builder(module.getContext());
  for (ac::RegOp reg : module.getBody().front().getOps<ac::RegOp>())
    if (reg.getState() == handle)
      return ownedState(builder, reg);
  auto argument = dyn_cast<BlockArgument>(handle);
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!argument || argument.getOwner() != &module.getBody().front() ||
      argument.getArgNumber() < 2 || !ports ||
      argument.getArgNumber() - 2 >= ports.size())
    return emitError()
           << "final hardware handle is neither owned nor a formal port";
  auto port = dyn_cast<DictionaryAttr>(ports[argument.getArgNumber() - 2]);
  if (!port)
    return emitError() << "final hardware formal has a malformed PortSlot";
  return formalState(builder, port);
}

struct Driver {
  Value data;
  Value enable;
};

LogicalResult addDriver(DenseMap<Attribute, Driver> &drivers,
                        DictionaryAttr target, Value data, Value enable,
                        ac::detail::EmitError emitError) {
  if (!target || !data || !enable ||
      !drivers.try_emplace(target, Driver{data, enable}).second)
    return emitError()
           << "final hardware direct packet has repeated or malformed driver";
  return success();
}

FailureOr<ac::ModuleOp> findDefinition(ArrayRef<ac::ModuleOp> definitions,
                                       FlatSymbolRefAttr symbol,
                                       ac::detail::EmitError emitError) {
  if (!symbol)
    return emitError() << "final hardware has an empty module definition";
  ac::ModuleOp found;
  for (ac::ModuleOp candidate : definitions) {
    if (candidate.getSymName() != symbol.getValue())
      continue;
    if (found)
      return emitError()
             << "final hardware package repeats a module definition";
    found = candidate;
  }
  if (!found)
    return emitError() << "final hardware instance has no module definition";
  return found;
}

LogicalResult bindInstance(ac::ModuleOp parent, ac::InstanceOp instance,
                           ArrayRef<ac::ModuleOp> definitions,
                           const ModuleGraph &modules,
                           ac::detail::EmitError emitError) {
  auto sourceDefinition = dyn_cast<FlatSymbolRefAttr>(instance.getCalleeAttr());
  FlatSymbolRefAttr definition;
  ArrayAttr arguments;
  for (InstanceView *view : modules.views) {
    if (!view || !view->placement || !view->parent ||
        view->placement.getName() != instance.getName() ||
        view->parent->module.getSymName() != parent.getSymName())
      continue;
    if ((definition && definition != view->definition) ||
        (arguments && arguments != view->staticArguments))
      return emitError()
             << "one module definition has inconsistent child SpecKeys";
    definition = view->definition;
    arguments = view->staticArguments;
  }
  if (!sourceDefinition || !definition || sourceDefinition != definition ||
      !arguments)
    return emitError()
           << "final hardware instance has no exact source specialization";
  auto child = findDefinition(definitions, definition, emitError);
  if (failed(child))
    return failure();
  auto ports = (*child)->getAttrOfType<ArrayAttr>("ac.ports");
  if (!ports)
    return emitError() << "final hardware child has no PortSlot contract";
  Builder builder(parent.getContext());
  SmallVector<Attribute> bindings;
  size_t input = 0;
  size_t target = 0;
  for (auto [portIndex, raw] : llvm::enumerate(ports)) {
    auto port = dyn_cast<DictionaryAttr>(raw);
    auto role = port ? port.getAs<StringAttr>("role") : StringAttr();
    Value actual;
    if (role && role.getValue() == "current") {
      if (input >= instance.getInputs().size())
        return emitError() << "final child current actual is missing";
      actual = instance.getInputs()[input++];
    } else if (role && role.getValue() == "next") {
      if (target >= instance.getTargets().size())
        return emitError() << "final child next actual is missing";
      actual = instance.getTargets()[target++];
    } else {
      return emitError() << "final child PortSlot role is invalid";
    }
    auto relative = stateRef(parent, actual, emitError);
    if (failed(relative))
      return failure();
    bindings.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("port", builder.getI32IntegerAttr(portIndex)),
        builder.getNamedAttr("target", *relative),
    }));
  }
  if (input != instance.getInputs().size() ||
      target != instance.getTargets().size())
    return emitError() << "final child actual arity differs from its ports";
  instance->setAttr("callee", specKey(builder, definition, arguments));
  instance->setAttr("ac.port_bindings", builder.getArrayAttr(bindings));
  instance->removeAttr("ac.static_args");
  return success();
}

LogicalResult collectRuleDrivers(ac::ModuleOp module,
                                 DenseMap<Attribute, Driver> &drivers,
                                 ac::detail::EmitError emitError) {
  for (ac::RuleOp rule : module.getBody().front().getOps<ac::RuleOp>()) {
    auto targets = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
    if (!targets || rule.getNextValues().size() != 2 * targets.size())
      return emitError() << "final rule output/result closure is incomplete";
    for (auto [index, raw] : llvm::enumerate(targets)) {
      auto target = dyn_cast<DictionaryAttr>(raw);
      if (failed(addDriver(drivers, target, rule.getNextValues()[2 * index],
                           rule.getNextValues()[2 * index + 1], emitError)))
        return failure();
    }
  }
  return success();
}

LogicalResult collectChildDrivers(ac::ModuleOp module,
                                  ArrayRef<ac::ModuleOp> definitions,
                                  DenseMap<Attribute, Driver> &drivers,
                                  ac::detail::EmitError emitError) {
  for (ac::InstanceOp instance :
       module.getBody().front().getOps<ac::InstanceOp>()) {
    auto key = dyn_cast<DictionaryAttr>(instance.getCalleeAttr());
    auto definition =
        key ? key.getAs<FlatSymbolRefAttr>("definition") : FlatSymbolRefAttr();
    auto child = findDefinition(definitions, definition, emitError);
    auto bindings = instance->getAttrOfType<ArrayAttr>("ac.port_bindings");
    auto ports = succeeded(child)
                     ? (*child)->getAttrOfType<ArrayAttr>("ac.ports")
                     : ArrayAttr();
    if (failed(child) || !bindings || !ports || bindings.size() != ports.size())
      return emitError() << "final child binding closure is incomplete";
    size_t result = 0;
    for (auto [index, rawPort] : llvm::enumerate(ports)) {
      auto port = cast<DictionaryAttr>(rawPort);
      if (port.getAs<StringAttr>("role").getValue() != "next")
        continue;
      auto binding = dyn_cast<DictionaryAttr>(bindings[index]);
      auto target =
          binding ? binding.getAs<DictionaryAttr>("target") : DictionaryAttr();
      if (result + 1 >= instance.getNextValues().size() ||
          failed(addDriver(drivers, target, instance.getNextValues()[result],
                           instance.getNextValues()[result + 1], emitError)))
        return failure();
      result += 2;
    }
    if (result != instance.getNextValues().size())
      return emitError() << "final child next result arity is incomplete";
  }
  return success();
}

LogicalResult closeModule(ac::ModuleOp module,
                          ArrayRef<ac::ModuleOp> definitions,
                          ac::detail::EmitError emitError) {
  DenseMap<Attribute, Driver> drivers;
  if (failed(collectRuleDrivers(module, drivers, emitError)) ||
      failed(collectChildDrivers(module, definitions, drivers, emitError)))
    return failure();
  Builder builder(module.getContext());
  SmallVector<DictionaryAttr> targets;
  SmallVector<ac::RegOp> owned;
  for (ac::RegOp reg : module.getBody().front().getOps<ac::RegOp>()) {
    targets.push_back(ownedState(builder, reg));
    owned.push_back(reg);
  }
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!ports)
    return emitError() << "final module has no PortSlot contract";
  for (Attribute raw : ports) {
    auto port = cast<DictionaryAttr>(raw);
    if (port.getAs<StringAttr>("role").getValue() == "next")
      targets.push_back(formalState(builder, port));
  }

  Block &body = module.getBody().front();
  auto oldYield = dyn_cast<ac::YieldOp>(body.getTerminator());
  if (!oldYield || !oldYield.getValues().empty())
    return emitError() << "final module does not have an empty local yield";
  OpBuilder ops(oldYield);
  SmallVector<Value> yielded;
  for (auto [index, target] : llvm::enumerate(targets)) {
    auto found = drivers.find(target);
    if (found != drivers.end()) {
      yielded.push_back(found->second.data);
      yielded.push_back(found->second.enable);
      continue;
    }
    if (index >= owned.size())
      return emitError() << "final borrowed next state has no direct driver";
    auto initial =
        dyn_cast<TypedAttr>(owned[index]->getAttr("ac.initial_value"));
    if (!initial)
      return emitError() << "final hold state has no typed reset image";
    auto data = arith::ConstantOp::create(ops, oldYield.getLoc(), initial);
    auto disabled = arith::ConstantOp::create(ops, oldYield.getLoc(),
                                              ops.getBoolAttr(false));
    yielded.push_back(data.getResult());
    yielded.push_back(disabled.getResult());
  }
  DenseSet<Attribute> commitSet;
  for (DictionaryAttr target : targets)
    commitSet.insert(target);
  for (const auto &driver : drivers)
    if (!commitSet.contains(driver.first))
      return emitError() << "final direct driver targets a foreign state";
  ac::YieldOp::create(ops, oldYield.getLoc(), yielded);
  oldYield.erase();
  SmallVector<Attribute> rawTargets(targets.begin(), targets.end());
  module->setAttr("ac.commit_targets", builder.getArrayAttr(rawTargets));
  return success();
}

FailureOr<ArrayAttr> instanceBindings(const ModuleGraph &modules,
                                      ac::detail::EmitError emitError) {
  Builder builder(modules.root->module.getContext());
  SmallVector<DictionaryAttr> rows;
  for (InstanceView *view : modules.views) {
    auto ports = view->module->getAttrOfType<ArrayAttr>("ac.ports");
    if (!ports ||
        view->module.getBody().front().getNumArguments() != ports.size() + 2)
      return emitError() << "global instance ports are incomplete";
    SmallVector<Attribute> bindings;
    for (auto [index, raw] : llvm::enumerate(ports)) {
      auto port = dyn_cast<DictionaryAttr>(raw);
      auto relative = port ? formalState(builder, port) : DictionaryAttr();
      auto resolved = relative
                          ? modules.resolveState(*view, relative, emitError)
                          : FailureOr<DictionaryAttr>(failure());
      if (failed(resolved))
        return failure();
      bindings.push_back(builder.getDictionaryAttr({
          builder.getNamedAttr("port", builder.getI32IntegerAttr(index)),
          builder.getNamedAttr("state", *resolved),
      }));
    }
    rows.push_back(builder.getDictionaryAttr({
        builder.getNamedAttr("owner", view->owner),
        builder.getNamedAttr(
            "key", specKey(builder, view->definition, view->staticArguments)),
        builder.getNamedAttr("ports", builder.getArrayAttr(bindings)),
    }));
  }
  llvm::sort(rows, [](DictionaryAttr left, DictionaryAttr right) {
    return ac::detail::compareClosedSourceStructure(left.get("owner"),
                                                    right.get("owner")) < 0;
  });
  SmallVector<Attribute> raw(rows.begin(), rows.end());
  return builder.getArrayAttr(raw);
}

} // namespace

FailureOr<OwningOpRef<ModuleOp>> materializeFinalHardwarePackage(
    SmallVectorImpl<OwningOpRef<ModuleOp>> &ownedUnits,
    ArrayRef<SourceLinkUnit> units, const ModuleGraph &modules,
    SmallVectorImpl<FinalDeclarationProjection> &declarations,
    ac::detail::EmitError emitError) {
  if (!modules.root || modules.views.empty())
    return emitError() << "final hardware package requires one rooted graph";
  auto rootPorts = modules.root->module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!rootPorts || !rootPorts.empty())
    return emitError() << "final hardware entry module must be portless";
  auto bindings = instanceBindings(modules, emitError);
  if (failed(bindings))
    return failure();

  SmallVector<ModuleOp> implementations;
  SmallVector<ModuleOp> declarationUnits;
  DenseMap<Operation *, OwningOpRef<ModuleOp> *> owners;
  DenseMap<Attribute, FinalDeclarationProjection *> projectionByOwner;
  DenseMap<Operation *, FinalDeclarationProjection *> projectionByUnit;
  for (OwningOpRef<ModuleOp> &owned : ownedUnits)
    if (owned)
      owners.try_emplace((*owned).getOperation(), &owned);
  for (FinalDeclarationProjection &projection : declarations) {
    if (!projection.sourceOwner || !projection.sourceUnitKind ||
        !projection.declarations ||
        !projectionByOwner.try_emplace(projection.sourceOwner, &projection)
             .second)
      return emitError() << "final declaration projection inventory is invalid";
    projectionByUnit.try_emplace((*projection.declarations).getOperation(),
                                 &projection);
    if (projection.sourceUnitKind.getValue() == "declarations") {
      declarationUnits.push_back(*projection.declarations);
      Builder unitBuilder(projection.declarations->getContext());
      (*projection.declarations)
          ->setAttrs(unitBuilder.getDictionaryAttr({
              unitBuilder.getNamedAttr("ac.source_owner",
                                       projection.sourceOwner),
              unitBuilder.getNamedAttr("ac.stage",
                                       unitBuilder.getStringAttr("final")),
              unitBuilder.getNamedAttr(
                  "ac.unit_kind", unitBuilder.getStringAttr("declarations")),
          }));
      continue;
    }
    if (projection.sourceUnitKind.getValue() != "implementation")
      return emitError()
             << "final declaration projection has an unsupported unit kind";
  }
  DenseSet<Operation *> seen;
  for (SourceLinkUnit unit : units) {
    auto kind = unit.body ? unit.body->getAttrOfType<StringAttr>("ac.unit_kind")
                          : StringAttr();
    if (!kind || kind.getValue() != "implementation")
      continue;
    if (!owners.contains(unit.body))
      return emitError()
             << "final implementation unit has no private owned lifetime";
    if (seen.insert(unit.body).second)
      implementations.push_back(unit.body);
  }
  llvm::sort(implementations, [](const auto &left, const auto &right) {
    return ac::detail::compareClosedSourceStructure(
               left->getAttr("ac.source_owner"),
               right->getAttr("ac.source_owner")) < 0;
  });
  for (size_t index = 1; index < implementations.size(); ++index)
    if (ac::detail::compareClosedSourceStructure(
            implementations[index - 1]->getAttr("ac.source_owner"),
            implementations[index]->getAttr("ac.source_owner")) == 0)
      return emitError() << "final hardware package repeats one source owner";

  SmallVector<ac::ModuleOp> definitions;
  for (ModuleOp unit : implementations) {
    auto owner = unit->getAttrOfType<DictionaryAttr>("ac.source_owner");
    auto projection = projectionByOwner.lookup(owner);
    if (!projection ||
        projection->sourceUnitKind.getValue() != "implementation")
      return emitError()
             << "final implementation lacks its declaration projection";

    ac::ModuleOp module;
    for (Operation &child : unit.getBody()->getOperations()) {
      auto candidate = dyn_cast<ac::ModuleOp>(child);
      if (!candidate)
        return emitError()
               << "source implementation contains unexpected post-cleanup op";
      if (module)
        return emitError() << "source implementation repeats its direct module";
      module = candidate;
    }
    if (!module)
      return emitError() << "source implementation has no direct module";

    SmallVector<Operation *> selectedDeclarations;
    for (Operation &declaration :
         projection->declarations->getBody()->getOperations()) {
      if (!isa<ac::TypeAliasOp, ac::ConstantOp, ac::StructOp>(declaration))
        return emitError()
               << "declaration projection contains an unsupported op";
      selectedDeclarations.push_back(&declaration);
    }
    for (Operation *declaration : selectedDeclarations)
      declaration->moveBefore(module);
    SmallVector<Operation *> projectedChildren;
    for (Operation &child : unit.getBody()->getOperations())
      projectedChildren.push_back(&child);
    if (projectedChildren.size() != selectedDeclarations.size() + 1 ||
        !llvm::equal(ArrayRef<Operation *>(projectedChildren)
                         .take_front(selectedDeclarations.size()),
                     selectedDeclarations) ||
        projectedChildren.back() != module)
      return emitError()
             << "final implementation does not match its scalar declaration "
                "projection";

    Builder unitBuilder(unit.getContext());
    unit->setAttrs(unitBuilder.getDictionaryAttr({
        unitBuilder.getNamedAttr("ac.source_owner",
                                 unit->getAttr("ac.source_owner")),
        unitBuilder.getNamedAttr("ac.stage",
                                 unitBuilder.getStringAttr("final")),
        unitBuilder.getNamedAttr("ac.unit_kind",
                                 unitBuilder.getStringAttr("implementation")),
    }));
    definitions.push_back(module);
  }
  for (InstanceView *view : modules.views)
    if (!view->staticArguments || !view->staticArguments.empty())
      return emitError()
             << "global final foundation supports empty SpecKey arguments";
    else if (failed(findDefinition(definitions, view->definition, emitError)))
      return emitError()
             << "final package omitted one executable module definition";
  for (ac::ModuleOp module : definitions)
    for (ac::InstanceOp instance :
         module.getBody().front().getOps<ac::InstanceOp>())
      if (failed(
              bindInstance(module, instance, definitions, modules, emitError)))
        return failure();
  for (ac::ModuleOp module : definitions)
    if (failed(closeModule(module, definitions, emitError)))
      return failure();

  SmallVector<ModuleOp> finalUnits(implementations.begin(),
                                   implementations.end());
  llvm::append_range(finalUnits, declarationUnits);
  if (finalUnits.size() != declarations.size())
    return emitError()
           << "final unit inventory does not match declaration projections";
  llvm::sort(finalUnits, [](ModuleOp left, ModuleOp right) {
    return ac::detail::compareClosedSourceStructure(
               left->getAttr("ac.source_owner"),
               right->getAttr("ac.source_owner")) < 0;
  });
  for (size_t index = 1; index < finalUnits.size(); ++index)
    if (ac::detail::compareClosedSourceStructure(
            finalUnits[index - 1]->getAttr("ac.source_owner"),
            finalUnits[index]->getAttr("ac.source_owner")) == 0)
      return emitError() << "final package repeats one source owner";

  MLIRContext *context = modules.root->module.getContext();
  auto package =
      OwningOpRef<ModuleOp>(ModuleOp::create(UnknownLoc::get(context)));
  Builder builder(context);
  DictionaryAttr entry =
      specKey(builder, modules.root->definition, modules.root->staticArguments);
  (*package)->setAttr("ac.stage", builder.getStringAttr("final"));
  (*package)->setAttr("ac.entry", entry);
  (*package)->setAttr("ac.instance_bindings", *bindings);

  OperationState systemState(modules.root->module.getLoc(), "ac.system");
  systemState.addAttribute("entry", entry);
  systemState.addAttribute("ac.source_owner",
                           modules.root->module->getAttr("ac.source_owner"));
  systemState.addAttribute("ac.origin",
                           modules.root->module->getAttr("ac.origin"));
  systemState.addAttribute("domain", builder.getStringAttr("default"));
  OpBuilder packageBuilder = OpBuilder::atBlockBegin(package->getBody());
  packageBuilder.create(systemState);
  for (ModuleOp unit : finalUnits) {
    auto kind = unit->getAttrOfType<StringAttr>("ac.unit_kind");
    if (kind && kind.getValue() == "implementation") {
      auto owner = owners.find(unit);
      if (owner == owners.end() || !*owner->second)
        return emitError() << "final implementation ownership changed";
      package->getBody()->push_back(owner->second->release());
      continue;
    }
    auto projection = projectionByUnit.find(unit.getOperation());
    if (projection == projectionByUnit.end() ||
        !projection->second->declarations)
      return emitError() << "final declarations unit lost its projection owner";
    package->getBody()->push_back(projection->second->declarations.release());
  }

  if (failed(ac::verifyFinalHardware(*package)))
    return emitError() << "materialized global final hardware failed verify";
  return package;
}

} // namespace acir::compiler

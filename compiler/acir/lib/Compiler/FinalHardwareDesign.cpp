#include "FinalHardwareDesign.h"

#include "Dialect/ACIR/ACIRHardwareClosure.h"
#include "Dialect/ACIR/ACIRHardwareClosureDetail.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

#include <functional>

using namespace mlir;

namespace acir::compiler {
namespace {

FailureOr<DictionaryAttr> logicalType(ac::hardware_detail::View &view,
                                      DictionaryAttr relative,
                                      ac::detail::EmitError emitError) {
  auto kind = relative.getAs<StringAttr>("kind");
  if (!kind)
    return emitError() << "final hardware state alias has no kind";
  if (kind.getValue() == "owned") {
    for (ac::RegOp reg : view.module.getBody().front().getOps<ac::RegOp>())
      if (reg->getAttr("ac.declaration") == relative.get("declaration") &&
          reg->getAttr("ac.element") == relative.get("element")) {
        auto logical = reg->getAttrOfType<DictionaryAttr>("ac.logical_type");
        return logical ? FailureOr<DictionaryAttr>(logical)
                       : FailureOr<DictionaryAttr>(
                             emitError() << "final reg has no logical type");
      }
    return emitError() << "final hardware owned alias has no ac.reg";
  }
  auto ports = view.module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!ports)
    return emitError() << "final hardware formal alias has no ports";
  for (Attribute raw : ports) {
    auto port = cast<DictionaryAttr>(raw);
    if (port.get("parameter") == relative.get("parameter") &&
        port.get("ordinal") == relative.get("ordinal")) {
      auto logical = port.getAs<DictionaryAttr>("type");
      return logical ? FailureOr<DictionaryAttr>(logical)
                     : FailureOr<DictionaryAttr>(
                           emitError() << "final port has no logical type");
    }
  }
  return emitError() << "final hardware formal alias has no PortSlot";
}

} // namespace

FailureOr<ModuleGraph>
rebuildFinalModuleGraphFromHardware(ModuleOp package,
                                    ac::detail::EmitError emitError) {
  if (!package || failed(ac::verifyFinalHardware(package)))
    return emitError() << "final hardware reconstruction requires verified IR";
  ac::hardware_detail::Closure closure;
  closure.package = package;
  if (failed(ac::hardware_detail::inspectEnvelope(closure, emitError)) ||
      failed(ac::hardware_detail::rebuildInstances(closure, emitError)) ||
      failed(ac::hardware_detail::verifyInstanceRows(closure, emitError)) ||
      failed(ac::hardware_detail::verifyCommits(closure, emitError)))
    return failure();

  ModuleGraph graph;
  DenseMap<ac::hardware_detail::View *, InstanceView *> translated;
  DenseSet<Attribute> owners;
  for (ac::hardware_detail::View *source : closure.views) {
    if (!source || !source->key || !source->module ||
        !owners.insert(source->owner).second)
      return emitError() << "final hardware view inventory is incomplete";
    auto storage = std::make_unique<InstanceView>();
    InstanceView *view = storage.get();
    view->owner = source->owner;
    view->definition = source->key.getAs<FlatSymbolRefAttr>("definition");
    view->staticArguments = source->key.getAs<ArrayAttr>("arguments");
    view->module = source->module;
    view->placement = source->placement;
    if (!view->definition || !view->staticArguments ||
        !translated.try_emplace(source, view).second)
      return emitError() << "final hardware view SpecKey is incomplete";
    graph.storage_.push_back(std::move(storage));
    graph.views.push_back(view);
  }

  ac::hardware_detail::View *root = nullptr;
  for (ac::hardware_detail::View *source : closure.views) {
    InstanceView *view = translated.lookup(source);
    if (source->parent) {
      view->parent = translated.lookup(source->parent);
      if (!view->parent)
        return emitError() << "final hardware view has a foreign parent";
      view->parent->children.push_back(view);
    } else if (root) {
      return emitError() << "final hardware has multiple reconstructed roots";
    } else {
      root = source;
      graph.root = view;
    }
    for (const auto &[relativeRaw, state] : source->states) {
      auto relative = dyn_cast<DictionaryAttr>(relativeRaw);
      auto kind = relative ? relative.getAs<StringAttr>("kind") : StringAttr();
      auto logical = relative ? logicalType(*source, relative, emitError)
                              : FailureOr<DictionaryAttr>(failure());
      if (!relative || !kind || failed(logical))
        return failure();
      auto &aliases =
          kind.getValue() == "owned" ? view->ownedStates : view->formalAliases;
      if (!aliases.try_emplace(relative, state).second)
        return emitError() << "final hardware repeats a relative state alias";
      auto inserted = graph.stateTypes_.try_emplace(state, *logical);
      if (!inserted.second && inserted.first->second != *logical)
        return emitError() << "final hardware StateID logical type changed";
    }
    Builder builder(package.getContext());
    for (ac::RegOp reg : view->module.getBody().front().getOps<ac::RegOp>()) {
      DictionaryAttr relative = ac::hardware_detail::ownedRef(builder, reg);
      auto found = view->ownedStates.find(relative);
      if (found == view->ownedStates.end())
        return emitError()
               << "final hardware owned state ordering is incomplete";
      view->ownedStateIDs.push_back(found->second);
    }
  }
  if (!root || !graph.root)
    return emitError() << "final hardware reconstruction has no root";

  DenseSet<InstanceView *> visited;
  std::function<LogicalResult(InstanceView *)> postOrder =
      [&](InstanceView *view) -> LogicalResult {
    if (!view || !visited.insert(view).second)
      return emitError() << "final hardware reconstructed tree is cyclic";
    for (InstanceView *child : view->children)
      if (failed(postOrder(child)))
        return failure();
    graph.postOrder_.push_back(view);
    return success();
  };
  if (failed(postOrder(graph.root)) || visited.size() != graph.views.size())
    return emitError() << "final hardware reconstructed tree is disconnected";
  return graph;
}

FailureOr<FinalHardwareDesignView>
rebuildFinalHardwareDesign(ModuleOp package, ac::detail::EmitError emitError) {
  auto modules = rebuildFinalModuleGraphFromHardware(package, emitError);
  if (failed(modules))
    return failure();
  FinalHardwareDesignView result;
  result.modules = std::make_unique<ModuleGraph>(std::move(*modules));

  auto checks = buildSourceCheckGraph(*result.modules, emitError);
  if (failed(checks) || failed(verifySourceChecks(*checks, emitError)))
    return emitError() << "final hardware CheckGraph reconstruction failed";
  result.checks = std::make_unique<CheckGraph>(std::move(*checks));

  auto proposals =
      buildSourceProposalGraph(*result.modules, result.checks.get(), emitError);
  if (failed(proposals) || failed(verifySourceProposals(*proposals, emitError)))
    return emitError() << "final hardware ProposalGraph reconstruction failed";
  result.proposals = std::make_unique<ProposalGraph>(std::move(*proposals));

  auto observations = buildSourceObservationGraph(*result.modules, emitError);
  if (failed(observations) ||
      failed(verifySourceObservations(*observations, emitError)))
    return emitError()
           << "final hardware ObservationGraph reconstruction failed";
  result.observations =
      std::make_unique<ObservationGraph>(std::move(*observations));
  return result;
}

} // namespace acir::compiler

#include "Compiler/RuleEffectView.h"
#include "Compiler/RuleInventory.h"
#include "Dialect/ACIR/ACIRNumericComposition.h"
#include "Dialect/ACIR/ACIRNumericNextUse.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringMap.h"

using namespace mlir;

namespace acir::compiler::detail {
namespace {

struct ResolvedState {
  DictionaryAttr state;
  DictionaryAttr logicalType;
};

DictionaryAttr formalState(Builder &builder, DictionaryAttr port) {
  return builder.getDictionaryAttr({
      builder.getNamedAttr("kind", builder.getStringAttr("formal")),
      builder.getNamedAttr("parameter", port.get("parameter")),
      builder.getNamedAttr("ordinal", port.get("ordinal")),
  });
}

FailureOr<ResolvedState> resolveHandle(ac::ModuleOp module, Value handle,
                                       StringRef role,
                                       ac::detail::EmitError emitError) {
  Builder builder(module.getContext());
  if (auto state = handle.getDefiningOp<ac::RegOp>()) {
    if (state->getParentOfType<ac::ModuleOp>() != module)
      return emitError() << "rule effect references a reg from another module";
    auto declaration = state->getAttrOfType<DictionaryAttr>("ac.declaration");
    auto element = state->getAttrOfType<ArrayAttr>("ac.element");
    auto logical = state->getAttrOfType<DictionaryAttr>("ac.logical_element");
    if (!declaration || !element || !logical)
      return emitError() << "rule effect owned reg identity is incomplete";
    return ResolvedState{
        builder.getDictionaryAttr({
            builder.getNamedAttr("kind", builder.getStringAttr("owned")),
            builder.getNamedAttr("declaration", declaration),
            builder.getNamedAttr("element", element),
        }),
        logical};
  }

  auto argument = dyn_cast<BlockArgument>(handle);
  auto ports = module->getAttrOfType<ArrayAttr>("ac.ports");
  if (!argument || argument.getOwner() != &module.getBody().front() || !ports ||
      argument.getArgNumber() < 2 ||
      argument.getArgNumber() - 2 >= ports.size())
    return emitError() << "rule effect formal handle is not a module port";
  auto port = dyn_cast<DictionaryAttr>(ports[argument.getArgNumber() - 2]);
  auto actualRole = port ? port.getAs<StringAttr>("role") : StringAttr();
  auto logical = port ? port.getAs<DictionaryAttr>("type") : DictionaryAttr();
  if (!port || !actualRole || actualRole.getValue() != role || !logical)
    return emitError() << "rule effect formal handle has the wrong port role";
  return ResolvedState{formalState(builder, port), logical};
}

LogicalResult addEffect(RuleEffectView &view,
                        DenseMap<Attribute, size_t> &indexByState,
                        DictionaryAttr state, DictionaryAttr logicalType,
                        Value currentHandle, Value nextHandle, bool read,
                        bool write, DictionaryAttr origin, StringRef precision,
                        ac::detail::EmitError emitError,
                        bool coalesceOrigin = false) {
  if (failed(ac::detail::verifyStateRef(state, emitError)) ||
      failed(ac::detail::verifyOccurrence(origin, emitError)))
    return failure();
  auto [position, inserted] =
      indexByState.try_emplace(state, view.effects.size());
  if (inserted) {
    RuleEffect effect;
    effect.state = state;
    effect.logicalType = logicalType;
    view.effects.push_back(std::move(effect));
  }
  RuleEffect &effect = view.effects[position->second];
  if (effect.logicalType != logicalType)
    return emitError() << "one StateRef has inconsistent LogicalTypes";
  if (read) {
    if (!currentHandle)
      return emitError() << "read effect has no current handle";
    if (effect.currentHandle && effect.currentHandle != currentHandle)
      return emitError() << "one StateRef has inconsistent current handles";
    effect.currentHandle = currentHandle;
    effect.read = true;
  }
  if (write) {
    if (!nextHandle)
      return emitError() << "write effect has no next handle";
    if (effect.nextHandle && effect.nextHandle != nextHandle)
      return emitError() << "one StateRef has inconsistent next handles";
    effect.nextHandle = nextHandle;
    effect.write = true;
  }
  if (!origin)
    return emitError() << "rule effect origin is missing";
  if (!coalesceOrigin || !llvm::is_contained(effect.origins, origin))
    effect.origins.push_back(origin);
  if (precision == "conservative")
    effect.precision = "conservative";
  else if (precision != "exact")
    return emitError() << "rule effect precision is invalid";
  return success();
}

ac::ModuleImportOp findChildHeader(ac::ModuleOp module,
                                   FlatSymbolRefAttr callee) {
  auto file = module->getParentOfType<mlir::ModuleOp>();
  if (!file)
    return {};
  for (ac::ModuleImportOp header : file.getOps<ac::ModuleImportOp>())
    if (header.getSymName() == callee.getValue())
      return header;
  return {};
}

LogicalResult finishEffects(RuleEffectView &view,
                            ac::detail::EmitError emitError) {
  for (RuleEffect &effect : view.effects) {
    llvm::sort(effect.origins, [](DictionaryAttr left, DictionaryAttr right) {
      return ac::detail::compareClosedSourceStructure(left, right) < 0;
    });
    for (size_t index = 1; index < effect.origins.size(); ++index)
      if (effect.origins[index - 1] == effect.origins[index])
        return emitError() << "one StateRef repeats a source effect occurrence";
  }
  llvm::sort(view.effects, [](const RuleEffect &left, const RuleEffect &right) {
    return ac::detail::compareClosedSourceStructure(left.state, right.state) <
           0;
  });
  return success();
}

LogicalResult mergeEffects(RuleEffectView &destination,
                           DenseMap<Attribute, size_t> &indexByState,
                           const RuleEffectView &source,
                           ac::detail::EmitError emitError) {
  for (const RuleEffect &incoming : source.effects) {
    auto [position, inserted] =
        indexByState.try_emplace(incoming.state, destination.effects.size());
    if (inserted) {
      destination.effects.push_back(incoming);
      continue;
    }
    RuleEffect &effect = destination.effects[position->second];
    if (effect.logicalType != incoming.logicalType)
      return emitError() << "one StateRef has inconsistent LogicalTypes";
    if (incoming.read) {
      if (effect.currentHandle &&
          effect.currentHandle != incoming.currentHandle)
        return emitError() << "one StateRef has inconsistent current handles";
      effect.currentHandle = incoming.currentHandle;
      effect.read = true;
    }
    if (incoming.write) {
      if (effect.nextHandle && effect.nextHandle != incoming.nextHandle)
        return emitError() << "one StateRef has inconsistent next handles";
      effect.nextHandle = incoming.nextHandle;
      effect.write = true;
    }
    if (incoming.precision == "conservative")
      effect.precision = "conservative";
    for (DictionaryAttr origin : incoming.origins)
      if (!llvm::is_contained(effect.origins, origin))
        effect.origins.push_back(origin);
  }
  return success();
}

DictionaryAttr requiredUseTarget(ac::RuleOp rule, ac::ValueUseOp use) {
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_uses");
  if (!required)
    return {};
  DictionaryAttr target;
  for (Attribute raw : required) {
    auto entry = dyn_cast<DictionaryAttr>(raw);
    if (!entry || entry.getAs<DictionaryAttr>("id") != use.getIdAttr())
      continue;
    if (target || entry.size() != 3 ||
        entry.getAs<DictionaryAttr>("value") != use.getSourceAttr())
      return {};
    target = entry.getAs<DictionaryAttr>("target");
  }
  return target;
}

} // namespace

FailureOr<RuleEffectView>
inferSourceRuleEffects(ac::RuleOp rule, ac::detail::EmitError emitError) {
  auto module = rule ? rule->getParentOfType<ac::ModuleOp>() : ac::ModuleOp();
  if (!rule || !module || module.getBody().getBlocks().size() != 1 ||
      rule->getBlock() != &module.getBody().front() ||
      rule.getBody().getBlocks().size() != 1)
    return emitError()
           << "source rule effects require a direct single-block module rule";
  RuleEffectView view;
  DenseMap<Attribute, size_t> indexByState;
  Block &body = rule.getBody().front();
  auto inputBindings = rule->getAttrOfType<ArrayAttr>("ac.input_bindings");
  auto inputTypes = rule->getAttrOfType<ArrayAttr>("ac.input_types");
  auto outputBindings = rule->getAttrOfType<ArrayAttr>("ac.output_bindings");
  auto outputTypes = rule->getAttrOfType<ArrayAttr>("ac.output_types");
  if (!inputBindings || !inputTypes || !outputBindings || !outputTypes ||
      inputBindings.size() != rule.getInputs().size() ||
      inputTypes.size() != rule.getInputs().size() ||
      outputBindings.size() != rule.getTargets().size() ||
      outputTypes.size() != rule.getTargets().size())
    return emitError() << "source rule effects require closed rule bindings";

  if (ruleHasNumericObligation(rule) && !rule.getTargets().empty()) {
    bool composition = ac::hasNumericCompositionContract(rule);
    bool closed = composition
                      ? succeeded(ac::verifyNumericCompositionClosure(rule))
                      : ac::hasNumericNextUseContract(rule) &&
                            succeeded(ac::verifyNumericNextUseClosure(rule));
    if (!closed)
      return emitError()
             << "source rule effects require verified numeric next-state "
                "closure";
  }

  LogicalResult result = success();
  rule.walk([&](ac::SourceReadOp read) {
    if (failed(result))
      return WalkResult::interrupt();
    if (read->getBlock() != &body) {
      result = emitError() << "nested source.read is not supported";
      return WalkResult::interrupt();
    }
    auto argument = dyn_cast<BlockArgument>(read.getCurrent());
    if (!argument || argument.getOwner() != &body ||
        argument.getArgNumber() >= rule.getInputs().size()) {
      result = emitError() << "source.read is not bound to a rule input";
      return WalkResult::interrupt();
    }
    size_t index = argument.getArgNumber();
    auto state = dyn_cast<DictionaryAttr>(inputBindings[index]);
    auto logical = dyn_cast<DictionaryAttr>(inputTypes[index]);
    auto origin = read->getAttrOfType<DictionaryAttr>("ac.origin");
    if (!state || !logical ||
        failed(addEffect(view, indexByState, state, logical,
                         rule.getInputs()[index], {}, true, false, origin,
                         "exact", emitError)))
      result = failure();
    return failed(result) ? WalkResult::interrupt() : WalkResult::advance();
  });
  if (failed(result))
    return failure();

  rule.walk([&](ac::SourceUseOp use) {
    if (failed(result))
      return WalkResult::interrupt();
    if (use->getBlock() != &body) {
      result = emitError() << "nested source.use is not supported";
      return WalkResult::interrupt();
    }
    auto target = use.getTarget();
    auto kind = target.getAs<StringAttr>("kind");
    auto state = target.getAs<DictionaryAttr>("state");
    if (!kind || kind.getValue() != "next_scalar" || !state) {
      result =
          emitError() << "rule effects support only next_scalar source.use";
      return WalkResult::interrupt();
    }
    std::optional<size_t> outputIndex;
    for (auto [index, binding] : llvm::enumerate(outputBindings))
      if (binding == state) {
        if (outputIndex) {
          result = emitError()
                   << "source.use target matches more than one rule output";
          return WalkResult::interrupt();
        }
        outputIndex = index;
      }
    if (!outputIndex) {
      result = emitError() << "source.use target has no rule output binding";
      return WalkResult::interrupt();
    }
    auto logical = dyn_cast<DictionaryAttr>(outputTypes[*outputIndex]);
    auto id = use.getId();
    auto origin = id.getAs<DictionaryAttr>("origin");
    if (!logical || failed(addEffect(view, indexByState, state, logical, {},
                                     rule.getTargets()[*outputIndex], false,
                                     true, origin, "exact", emitError)))
      result = failure();
    return failed(result) ? WalkResult::interrupt() : WalkResult::advance();
  });
  rule.walk(
      [&](ac::ValueUseOp use) {
        if (failed(result))
          return WalkResult::interrupt();
        if (use->getBlock() != &body) {
          result = emitError() << "nested value.use is not supported";
          return WalkResult::interrupt();
        }
        auto target = requiredUseTarget(rule, use);
        auto kind = target ? target.getAs<StringAttr>("kind") : StringAttr();
        auto state =
            target ? target.getAs<DictionaryAttr>("state") : DictionaryAttr();
        if (!kind || kind.getValue() != "next_scalar" || !state) {
          result = emitError()
                   << "verified ValueUse has no exact next_scalar RequiredUse";
          return WalkResult::interrupt();
        }
        std::optional<size_t> outputIndex;
        for (auto [index, binding] : llvm::enumerate(outputBindings))
          if (binding == state) {
            if (outputIndex) {
              result = emitError()
                       << "ValueUse target matches more than one rule output";
              return WalkResult::interrupt();
            }
            outputIndex = index;
          }
        if (!outputIndex) {
          result = emitError() << "ValueUse target has no rule output binding";
          return WalkResult::interrupt();
        }
        auto logical = dyn_cast<DictionaryAttr>(outputTypes[*outputIndex]);
        auto origin = use.getIdAttr().getAs<DictionaryAttr>("origin");
        if (!logical || failed(addEffect(view, indexByState, state, logical, {},
                                         rule.getTargets()[*outputIndex], false,
                                         true, origin, "exact", emitError)))
          result = failure();
        return failed(result) ? WalkResult::interrupt() : WalkResult::advance();
      });
  if (failed(result) || failed(finishEffects(view, emitError)))
    return failure();
  return view;
}

FailureOr<RuleEffectView>
inferSourceModuleEffects(ac::ModuleOp module, ac::detail::EmitError emitError) {
  if (!module || module.getBody().getBlocks().size() != 1)
    return emitError() << "source effects require one verified source module";

  RuleEffectView view;
  DenseMap<Attribute, size_t> indexByState;
  for (ac::RuleOp rule : module.getBody().front().getOps<ac::RuleOp>()) {
    auto ruleEffects = inferSourceRuleEffects(rule, emitError);
    if (failed(ruleEffects) ||
        failed(mergeEffects(view, indexByState, *ruleEffects, emitError)))
      return failure();
  }

  bool nestedRuleOrInstance = false;
  module.walk([&](Operation *operation) {
    if ((isa<ac::RuleOp, ac::InstanceOp>(operation)) &&
        operation->getBlock() != &module.getBody().front())
      nestedRuleOrInstance = true;
  });
  if (nestedRuleOrInstance)
    return emitError()
           << "nested source rule or instance effects are unsupported";

  for (ac::InstanceOp instance : module.getBody().getOps<ac::InstanceOp>()) {
    auto callee = dyn_cast<FlatSymbolRefAttr>(instance.getCalleeAttr());
    ac::ModuleImportOp header =
        callee ? findChildHeader(module, callee) : ac::ModuleImportOp();
    auto contract = header
                        ? header->getAttrOfType<DictionaryAttr>("ac.contract")
                        : DictionaryAttr();
    auto parameters =
        contract ? contract.getAs<ArrayAttr>("parameters") : ArrayAttr();
    auto connections =
        contract ? contract.getAs<ArrayAttr>("connections") : ArrayAttr();
    auto origin = instance->getAttrOfType<DictionaryAttr>("ac.origin");
    if (!header || !parameters || !connections || !origin)
      return emitError() << "child rule effects lack a verified ModuleImport";
    llvm::StringMap<DictionaryAttr> types;
    for (Attribute raw : parameters) {
      auto parameter = dyn_cast<DictionaryAttr>(raw);
      auto name =
          parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
      auto category =
          parameter ? parameter.getAs<StringAttr>("category") : StringAttr();
      auto type = parameter ? parameter.getAs<DictionaryAttr>("type")
                            : DictionaryAttr();
      if (name && category && category.getValue() == "connection" && type)
        types.try_emplace(name.getValue(), type);
    }
    size_t inputIndex = 0;
    size_t targetIndex = 0;
    for (Attribute raw : connections) {
      auto connection = dyn_cast<DictionaryAttr>(raw);
      auto parameter =
          connection ? connection.getAs<StringAttr>("parameter") : StringAttr();
      auto elements =
          connection ? connection.getAs<ArrayAttr>("elements") : ArrayAttr();
      auto found = parameter ? types.find(parameter.getValue()) : types.end();
      if (!connection || !parameter || !elements || elements.size() != 1 ||
          found == types.end())
        return emitError()
               << "child rule effects require one closed scalar element";
      auto effect = dyn_cast<DictionaryAttr>(elements[0]);
      auto read = effect ? effect.getAs<BoolAttr>("read") : BoolAttr();
      auto write = effect ? effect.getAs<BoolAttr>("write") : BoolAttr();
      auto precision =
          effect ? effect.getAs<StringAttr>("precision") : StringAttr();
      if (!effect || !read || !write || !precision)
        return emitError() << "child ElementEffect is malformed";
      DictionaryAttr actualState;
      DictionaryAttr actualLogical;
      Value currentHandle;
      Value nextHandle;
      if (read.getValue()) {
        if (inputIndex >= instance.getInputs().size())
          return emitError() << "child read effect has no actual input handle";
        currentHandle = instance.getInputs()[inputIndex++];
        auto resolved =
            resolveHandle(module, currentHandle, "current", emitError);
        if (failed(resolved) || resolved->logicalType != found->second)
          return failure();
        actualState = resolved->state;
        actualLogical = resolved->logicalType;
      }
      if (write.getValue()) {
        if (targetIndex >= instance.getTargets().size())
          return emitError()
                 << "child write effect has no actual target handle";
        nextHandle = instance.getTargets()[targetIndex++];
        auto resolved = resolveHandle(module, nextHandle, "next", emitError);
        if (failed(resolved) || resolved->logicalType != found->second)
          return failure();
        if (actualState && (actualState != resolved->state ||
                            actualLogical != resolved->logicalType))
          return emitError()
                 << "child read/write actuals do not identify one parent state";
        actualState = resolved->state;
        actualLogical = resolved->logicalType;
      }
      if ((read.getValue() || write.getValue()) &&
          failed(addEffect(view, indexByState, actualState, actualLogical,
                           currentHandle, nextHandle, read.getValue(),
                           write.getValue(), origin, precision.getValue(),
                           emitError, /*coalesceOrigin=*/true)))
        return failure();
    }
    if (inputIndex != instance.getInputs().size() ||
        targetIndex != instance.getTargets().size())
      return emitError()
             << "child actual handles do not match its verified effects";
  }

  if (failed(finishEffects(view, emitError)))
    return failure();
  return view;
}

} // namespace acir::compiler::detail

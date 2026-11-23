#include "pycircuit/Dialect/ACIR/HardwareCDC.h"
#include "HardwareCDCLocalVerification.h"
#include "ACIRSourceContracts.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/MathExtras.h"
using namespace mlir;
namespace acir::ac {
namespace {
LogicalResult verifyFIFOIntents(const HardwareAnalysis &analysis, Operation *resource, ArrayRef<Value> intents, const HardwareBindings &bindings) {
  for (auto intent : intents) {
    if (auto constant = intent.getDefiningOp<BitsConstantOp>()) {
      auto value = analysis.evaluateStatic(constant.getValue(), bindings, constant);
      if (succeeded(value))
        if (auto number = dyn_cast<MathIntAttr>(*value);
            number && number.getCanonicalValue() == "0")
          continue;
    }
    auto rule = intent.getDefiningOp<RuleOp>();
    if (!rule)
      return resource->emitOpError(
          "nonzero endpoint intent requires a sampled transaction fire");
    auto effects = getAsyncFIFOEndpointEffects(rule);
    if (failed(effects))
      return failure();
    if (llvm::none_of(*effects, [&](auto &effect) {
          return effect.resource == resource && effect.intent == intent;
        }))
      return resource->emitOpError(
          "endpoint intent lacks authenticated admission");
  }
  return success();
}
} // namespace

LogicalResult verifyCDCRefinementIntents(
    const HardwareAnalysis &analysis, CDCRefinementOp witness,
    const HardwareBindings &bindings) {
  if (failed(verifyCDCRefinementContract(witness)))
    return failure();
  if (witness.getLogicalKind() != "async_fifo")
    return success();
  if (failed(verifyCDCDepth(analysis,
          witness->getAttrOfType<StaticExprAttr>("depth"), bindings, witness)))
    return failure();
  return verifyFIFOIntents(analysis, witness,
                          {witness.getOperand(4), witness.getOperand(5)},
                          bindings);
}

LogicalResult verifyCDCDepth(const HardwareAnalysis &analysis,
    StaticExprAttr depth, const HardwareBindings &bindings, Operation *site) {
  if (!depth) return site->emitOpError("requires typed FIFO DEPTH");
  if (!analysis.isStaticEvaluable(depth, bindings)) return success();
  auto evaluated = analysis.evaluateStatic(depth, bindings, site);
  if (failed(evaluated)) return failure();
  auto number = dyn_cast<MathIntAttr>(*evaluated);
  if (!number) return site->emitOpError("FIFO DEPTH must be an integer");
  llvm::APSInt value(number.getCanonicalValue());
  if (value.isNegative() || value.getActiveBits() > 64 ||
      value.getZExtValue() < 2 || !llvm::isPowerOf2_64(value.getZExtValue()))
    return site->emitOpError("selected CDC FIFO implementation supports power-of-two depth >= 2 fitting u64");
  return success();
}
bool isCDCPrimitive(Operation *declaration) {
  auto imported = dyn_cast_or_null<ModuleImportOp>(declaration);
  auto kind = imported ? imported->getAttrOfType<StringAttr>("primitive_kind")
                       : StringAttr();
  return kind && (kind.getValue() == "async_reg" || kind.getValue() == "async_fifo");
}
bool isCDCInstance(InstanceOp instance) {
  // Package classification also runs before native structural verification.
  // A missing or ill-typed callee is rejected by the ordinary instance verifier.
  auto callee = instance ? instance->getAttrOfType<FlatSymbolRefAttr>("callee")
                         : FlatSymbolRefAttr();
  return callee && (callee.getValue() == "pycircuit.__builtins__.async_reg" ||
                    callee.getValue() == "pycircuit.__builtins__.async_fifo");
}
LogicalResult verifyCDCPrimitiveDeclaration(const HardwareAnalysis &analysis,
                                            ModuleImportOp declaration) {
  auto reject = [&]() -> LogicalResult { return declaration.emitOpError("invalid canonical CDC primitive declaration"); };
  if (!isCDCPrimitive(declaration) || declaration.getSemanticKind() != "hardware") return reject();
  StringRef kind = declaration.getPrimitiveKindAttr().getValue();
  bool fifo = kind == "async_fifo";
  if (declaration.getSymName() != (Twine("pycircuit.__builtins__.") + kind).str()) return reject();
  auto signature = declaration.getFunctionType();
  auto typeParameters = declaration.getTypeParameters();
  auto parameters = declaration.getParameters();
  if (signature.getNumInputs() != (fifo ? 6u : 4u) ||
      signature.getNumResults() != (fifo ? 3u : 1u) ||
      typeParameters.size() != (fifo ? 1u : 0u) || parameters.size() != (fifo ? 1u : 0u)) return reject();
  if (fifo) {
    auto typeName = dyn_cast<StringAttr>(typeParameters[0]);
    auto parameter = dyn_cast<DictionaryAttr>(parameters[0]);
    auto name = parameter ? parameter.getAs<StringAttr>("name") : StringAttr();
    auto type = parameter ? parameter.getAs<TypeAttr>("type") : TypeAttr();
    if (!typeName || typeName.getValue() != "T" || !name ||
        name.getValue() != "DEPTH" || !type ||
        !isa<MathIntType>(type.getValue()) || parameter.size() != 2)
      return reject();
  }
  SmallVector<StringRef> inputNames = fifo
      ? SmallVector<StringRef>{"write_clock", "read_clock", "whole_reset", "push_payload", "push_intent", "pop_intent"}
      : SmallVector<StringRef>{"source_clock", "target_clock", "whole_reset", "input"};
  SmallVector<StringRef> outputNames = fifo
      ? SmallVector<StringRef>{"head", "write_available", "read_available"}
      : SmallVector<StringRef>{"output"};
  if (declaration.getInputNames().size() != inputNames.size() ||
      declaration.getOutputNames().size() != outputNames.size()) return reject();
  for (auto [raw, name] : llvm::zip(declaration.getInputNames(), inputNames))
    if (dyn_cast<StringAttr>(raw) != StringAttr::get(declaration.getContext(), name)) return reject();
  for (auto [raw, name] : llvm::zip(declaration.getOutputNames(), outputNames))
    if (dyn_cast<StringAttr>(raw) != StringAttr::get(declaration.getContext(), name)) return reject();
  auto bit = [&](Type type) -> LogicalResult {
    auto bits = dyn_cast<BitsType>(type);
    if (!bits) return reject();
    auto width = analysis.evaluateStatic(bits.getWidth(), {}, declaration);
    auto number = succeeded(width) ? dyn_cast<MathIntAttr>(*width) : MathIntAttr();
    return number && number.getCanonicalValue() == "1" ? success() : reject();
  };
  auto payload = TypeParamType::get(declaration.getContext(),
      FlatSymbolRefAttr::get(declaration.getContext(), declaration.getSymName()),
      StringAttr::get(declaration.getContext(), "T"));
  for (auto [index, type] : llvm::enumerate(signature.getInputs())) {
    if (fifo && index == 3) { if (type != payload) return reject(); }
    else if (failed(bit(type))) return failure();
  }
  for (auto [index, type] : llvm::enumerate(signature.getResults())) {
    if (fifo && index == 0) { if (type != payload) return reject(); }
    else if (failed(bit(type))) return failure();
  }
  auto dependencies = analysis.getImportDependencies(declaration, {});
  if (failed(dependencies)) return failure();
  for (const auto &dependency : *dependencies)
    if (!dependency.inputs.empty()) return reject();
  return success();
}
FailureOr<CDCPrimitiveLayout> getCDCPrimitiveLayout(
    InstanceOp instance, const HardwareBindings &parent) {
  if (!isCDCInstance(instance)) return instance.emitOpError("requires a canonical CDC instance");
  auto owner = dyn_cast_or_null<ModuleOp>(instance->getParentOp());
  auto package = instance->getParentOfType<mlir::ModuleOp>();
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage") : StringAttr();
  auto contract = package ? package->getAttrOfType<IntegerAttr>("ac.routed_contract") : IntegerAttr();
  if (!owner || !stage || stage.getValue() != "routed" || !contract || contract.getInt() != 7)
    return instance.emitOpError("logical CDC requires a contract-7 routed module graph");
  HardwareAnalysis analysis(package);
  auto bindings = analysis.bindInstance(instance, parent);
  if (failed(bindings)) return failure();
  auto primitive = dyn_cast_or_null<ModuleImportOp>(bindings->owner);
  if (!primitive || failed(verifyCDCPrimitiveDeclaration(analysis, primitive))) return failure();
  CDCPrimitiveLayout result;
  result.allocation = instance;
  result.primitive = primitive;
  result.bindings = *bindings;
  result.kind = primitive.getPrimitiveKindAttr().getValue();
  bool fifo = result.isFIFO();
  if (instance.getNumOperands() != (fifo ? 6u : 4u) || instance.getNumResults() != (fifo ? 3u : 1u))
    return instance.emitOpError("CDC instance differs from its canonical signature");
  result.payloadType = instance.getOperand(3).getType();
  auto signature = primitive.getFunctionType();
  auto checkType = [&](Type actual, Type expected) -> LogicalResult {
    auto a = analysis.resolveType(actual, parent, instance);
    auto e = analysis.resolveType(expected, *bindings, instance);
    if (failed(a) || failed(e)) return failure();
    if (!areEquivalentHardwareTypes(*a, *e))
      return instance.emitOpError("CDC pin type differs from canonical signature");
    return success();
  };
  for (auto [actual, expected] : llvm::zip(instance.getOperandTypes(), signature.getInputs()))
    if (failed(checkType(actual, expected))) return failure();
  for (auto [actual, expected] : llvm::zip(instance.getResultTypes(), signature.getResults()))
    if (failed(checkType(actual, expected))) return failure();
  auto version = instance->getAttrOfType<IntegerAttr>("resource_contract");
  if (!version || !version.getType().isSignlessInteger(64) || version.getInt() != 1 || instance->hasAttr("depth"))
    return instance.emitOpError("requires resource_contract = 1 and canonical parameter authority");
  if (instance.getInstanceName().empty())
    return instance.emitOpError("CDC resource requires a nonempty instance name");
  auto domains = owner->getAttrOfType<ArrayAttr>("ac.domain_params");
  SmallVector<StringRef> domainNames =
      fifo ? SmallVector<StringRef>{"write_domain", "read_domain"}
           : SmallVector<StringRef>{"source_domain", "target_domain"};
  for (StringRef name : domainNames) {
    auto value = instance->getAttrOfType<StringAttr>(name);
    if (!value || !domains || !llvm::is_contained(domains, value))
      return instance.emitOpError("CDC endpoint must name a declared logical domain");
  }
  SmallVector<StringRef> edgeNames =
      fifo ? SmallVector<StringRef>{"write_edge", "read_edge"}
           : SmallVector<StringRef>{"target_edge"};
  for (StringRef name : edgeNames) {
    auto value = instance->getAttrOfType<StringAttr>(name);
    if (!value || (value.getValue() != "posedge" && value.getValue() != "negedge"))
      return instance.emitOpError("CDC endpoint requires an explicit edge");
  }
  auto polarity = instance->getAttrOfType<StringAttr>("reset_polarity");
  if (!polarity || (polarity.getValue() != "high" && polarity.getValue() != "low"))
    return instance.emitOpError("CDC resource requires whole-reset polarity");
  if (failed(detail::verifyOccurrence(instance.getOccurrence(), [&] { return instance.emitOpError(); }))) return failure();
  for (auto &other : owner.getBody().front())
    if (&other != instance.getOperation() && (other.getAttr("instance_name") == instance.getInstanceNameAttr() ||
                              other.getAttr("occurrence") == instance.getOccurrence()))
      return instance.emitOpError("CDC resource identity must be unique within its owner");
  if (fifo) {
    result.depthExpression = cast<StaticExprAttr>(instance.getParameters()[0]);
    Attribute depth = bindings->integers.lookup("DEPTH");
    if (auto expression = dyn_cast_or_null<StaticExprAttr>(depth)) {
      if (!analysis.isStaticEvaluable(expression, *bindings)) return result;
      auto value = analysis.evaluateStatic(expression, *bindings, instance);
      if (failed(value)) return failure();
      depth = *value;
    }
    auto number = dyn_cast_or_null<MathIntAttr>(depth);
    if (!number) return instance.emitOpError("CDC FIFO requires an integer DEPTH");
    llvm::APSInt value(number.getCanonicalValue());
    if (value.isNegative() || value.isZero() || value.getActiveBits() > 64)
      return instance.emitOpError("CDC FIFO DEPTH must be positive and fit u64");
    result.depth = value.getZExtValue();
  }
  return result;
}
bool hasLogicalCDC(Operation *root) {
  bool found = false;
  root->walk([&](Operation *operation) {
    found |= isCDCPrimitive(operation) ||
             isCDCInstance(dyn_cast<InstanceOp>(operation));
  });
  return found;
}
LogicalResult verifyBoundHardwareCDCOperation(
    const HardwareAnalysis &analysis, Operation *op,
    const HardwareBindings &bindings) {
  SmallVector<Value> controls;
  if (auto instance = dyn_cast<InstanceOp>(op); isCDCInstance(instance)) {
    auto layout = getCDCPrimitiveLayout(instance, bindings);
    if (failed(layout)) return failure();
    controls.append({layout->sourceClock(), layout->targetClock(), layout->reset()});
    if (layout->isFIFO()) {
      controls.append({layout->pushIntent(), layout->popIntent(), layout->writeAvailable(), layout->readAvailable()});
      if (failed(verifyFIFOIntents(analysis, op, {layout->pushIntent(), layout->popIntent()}, bindings))) return failure();
    } else controls.append({layout->input(), layout->output()});
  } else if (auto witness = dyn_cast<CDCRefinementOp>(op)) {
    if (failed(verifyCDCRefinementContract(op))) return failure();
    bool reg = witness.getLogicalKind() == "async_reg";
    for (unsigned ordinal : reg ? SmallVector<unsigned>{0, 1, 2, 3, 4}
                                : SmallVector<unsigned>{0, 1, 2, 4, 5, 7, 8})
      controls.push_back(witness.getOperand(ordinal));
    if (failed(verifyCDCRefinementIntents(analysis, witness, bindings)))
      return failure();
    if (failed(verifyHardwareCDCComponent(op))) return failure();
  }
  for (Value control : controls) {
    auto width = analysis.getPackedWidth(control.getType(), bindings, op);
    if (!isa<BitsType>(control.getType()) || failed(width) || *width != 1)
      return op->emitOpError("CDC controls must be one bit");
  }
  return success();
}
} // namespace acir::ac
namespace acir::ac {
namespace {
bool containsAdmission(Value value, Value admission, RuleOp rule,
                       DenseSet<Value> &visited, unsigned depth = 0) {
  if (depth > 128 || !visited.insert(value).second)
    return false;
  if (value == admission)
    return true;
  if (auto argument = dyn_cast<BlockArgument>(value)) {
    if (argument.getOwner()->getParentOp() == rule)
      return containsAdmission(rule.getCaptures()[argument.getArgNumber()],
                               admission, rule, visited, depth + 1);
  }
  if (auto op = value.getDefiningOp<BitsBinaryOp>();
      op && op.getOpcode() == "and")
    return containsAdmission(op.getLhs(), admission, rule, visited,
                             depth + 1) ||
           containsAdmission(op.getRhs(), admission, rule, visited, depth + 1);
  return false;
}
} // namespace
FailureOr<SmallVector<HardwareCDCEndpointEffect>>
getAsyncFIFOEndpointEffects(RuleOp rule) {
  SmallVector<HardwareCDCEndpointEffect> effects;
  auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
  auto fire = row ? row.getAs<IntegerAttr>("fire_result") : IntegerAttr();
  auto clockCapture =
      row ? row.getAs<IntegerAttr>("clock_capture") : IntegerAttr();
  for (auto result : rule.getResults())
    for (auto &use : result.getUses()) {
      auto instance = dyn_cast<InstanceOp>(use.getOwner());
      auto witness = dyn_cast<CDCRefinementOp>(use.getOwner());
      bool logical = isCDCInstance(instance) && (use.getOperandNumber() == 4 || use.getOperandNumber() == 5);
      bool physical = witness && witness.getLogicalKind() == "async_fifo" &&
                      (use.getOperandNumber() == 4 || use.getOperandNumber() == 5);
      if (!logical && !physical) continue;
      bool write = use.getOperandNumber() == 4;
      Operation *resource = use.getOwner();
      std::optional<CDCPrimitiveLayout> layout;
      if (logical) {
        auto checked = getCDCPrimitiveLayout(instance);
        if (failed(checked)) return failure();
        if (!checked->isFIFO()) return rule.emitOpError("invalid CDC endpoint resource");
        layout = *checked;
      } else if (failed(verifyCDCRefinementContract(witness))) return failure();
      if (!fire || fire.getInt() < 0 ||
          uint64_t(fire.getInt()) >= rule.getNumResults() ||
          result != rule.getResult(fire.getInt()) || !clockCapture ||
          clockCapture.getInt() < 0 ||
          uint64_t(clockCapture.getInt()) >= rule.getCaptures().size())
        return rule.emitOpError("CDC transfer intent must be its endpoint "
                                "transaction's authenticated final fire");
      auto expectedClock = layout
          ? (write ? layout->sourceClock() : layout->targetClock())
          : resource->getOperand(write ? 0 : 1);
      auto wholeReset = layout ? layout->reset() : resource->getOperand(2);
      auto edge = resource->getAttrOfType<StringAttr>(write ? "write_edge"
                                                            : "read_edge");
      auto polarity = resource->getAttrOfType<StringAttr>("reset_polarity");
      if (!edge || !polarity ||
          resource->getNumOperands() < (logical ? 6u : 9u))
        return rule.emitOpError("invalid CDC endpoint contract");
      // Actual clock identity is proved for every instantiated writer by the
      // common effect-footprint proof; distinct formal pins may alias there.
      if (row.getAs<StringAttr>("edge") != edge)
        return rule.emitOpError("CDC endpoint transaction clock/edge differs");
      bool resetPresent = false;
      if (auto resets = row.getAs<ArrayAttr>("resets"))
        for (auto raw : resets) {
          auto reset = dyn_cast<DictionaryAttr>(raw);
          auto capture =
              reset ? reset.getAs<IntegerAttr>("capture") : IntegerAttr();
          if (capture && capture.getInt() >= 0 &&
              uint64_t(capture.getInt()) < rule.getCaptures().size() &&
              rule.getCaptures()[capture.getInt()] == wholeReset &&
              reset.getAs<StringAttr>("kind") ==
                  StringAttr::get(rule.getContext(), "async") &&
              reset.getAs<StringAttr>("polarity") == polarity)
            resetPresent = true;
        }
      if (!resetPresent)
        return rule.emitOpError("CDC endpoint transaction must include its "
                                "actual whole asynchronous reset");
      auto yielded = cast<YieldOp>(rule.getBody().front().back());
      auto available =
          logical ? (write ? layout->writeAvailable() : layout->readAvailable())
                  : resource->getOperand(write ? 7 : 8);
      DenseSet<Value> admissionVisits;
      if (!containsAdmission(yielded.getOperand(fire.getInt()), available, rule,
                             admissionVisits))
        return rule.emitOpError(
            "CDC endpoint fire must include its actual local availability");
      effects.push_back(
          {resource, expectedClock, wholeReset, result, available,
           edge.getValue() == "posedge" ? RegisterEdge::Posedge
                                        : RegisterEdge::Negedge,
           polarity.getValue() == "high" ? RegisterResetPolarity::High
                                         : RegisterResetPolarity::Low,
           write});
    }
  return effects;
}
} // namespace acir::ac

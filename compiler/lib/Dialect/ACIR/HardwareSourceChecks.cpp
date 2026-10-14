#include "HardwareSourceChecks.h"
#include "ACIRSourceContracts.h"
#include "HardwareSampledAdmission.h"
#include "HardwareSourceCheckPreflight.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <map>

using namespace mlir;
namespace acir::ac {
namespace {
LogicalResult predicate(const HardwareAnalysis &analysis, Value value,
                        const HardwareBindings &bindings, Operation *site) {
  if (!isa<BitsType>(value.getType()))
    return site->emitOpError() << "source check operand requires one-bit Bits";
  auto width = analysis.getPackedWidth(value.getType(), bindings, site);
  if (failed(width))
    return failure();
  if (*width != 1)
    return site->emitOpError() << "source check operand must have width one";
  return success();
}
FailureOr<Value> carrier(const HardwareAnalysis &analysis, Value value,
                         const HardwareBindings &bindings,
                         detail::SourceCheckBudget &budget) {
  llvm::DenseSet<Value> seen;
  while (auto extract = value.getDefiningOp<BitsExtractOp>()) {
    if (failed(budget.debit(1 + detail::SourceCheckBudget::bytes(sizeof(Value)),
                            extract)))
      return failure();
    if (!seen.insert(value).second)
      return extract.emitOpError() << "cyclic source check carrier association";
    if (failed(analysis.verifyResolvedOperation(extract, bindings)))
      return failure();
    auto low = analysis.evaluateStatic(extract.getLow(), bindings, extract);
    auto input = analysis.getPackedWidth(extract.getInput().getType(), bindings,
                                         extract);
    auto output = analysis.getPackedWidth(value.getType(), bindings, extract);
    if (failed(low) || failed(input) || failed(output))
      return failure();
    auto integer = dyn_cast<MathIntAttr>(*low);
    if (!integer || !llvm::APSInt(integer.getCanonicalValue()).isZero() ||
        *input != *output)
      break;
    value = extract.getInput();
  }
  return value;
}
// Debit every metadata node and string traversal. The iterative work stack
// retains capacity while visiting siblings, and prepays only actual growth.
LogicalResult payEffectMetadata(
    Attribute attribute, Operation *site,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  using Budget = detail::SourceCheckBudget;
  auto pay = [&](uint64_t units) {
    return debit ? debit(units, site) : success();
  };
  if (failed(pay(1)))
    return failure();
  if (!attribute)
    return success();
  if (auto text = dyn_cast<StringAttr>(attribute))
    return pay(Budget::bytes(text.getValue().size()));
  if (!isa<ArrayAttr, DictionaryAttr>(attribute))
    return success();
  SmallVector<Attribute, 0> pending;
  auto push = [&](Attribute value) -> LogicalResult {
    if (failed(pay(1)))
      return failure();
    if (pending.size() == pending.capacity()) {
      uint64_t capacity = Budget::multiply(
          std::max(uint64_t(1), uint64_t(pending.capacity())), 2);
      uint64_t storage =
          Budget::multiply(capacity - pending.capacity(), sizeof(Attribute));
      if (capacity == std::numeric_limits<uint64_t>::max() ||
          storage == std::numeric_limits<uint64_t>::max() ||
          capacity > pending.max_size())
        return site->emitOpError()
               << "source effect metadata exceeds host stack capacity";
      if (failed(pay(Budget::bytes(storage))))
        return failure();
      pending.reserve(capacity);
    }
    pending.push_back(value);
    return success();
  };
  auto children = [&](Attribute value) -> LogicalResult {
    if (auto array = dyn_cast<ArrayAttr>(value)) {
      for (Attribute item : array)
        if (failed(push(item)))
          return failure();
    } else if (auto dictionary = dyn_cast<DictionaryAttr>(value)) {
      for (auto item : dictionary)
        if (failed(push(item.getName())) || failed(push(item.getValue())))
          return failure();
    }
    return success();
  };
  if (failed(children(attribute)))
    return failure();
  while (!pending.empty()) {
    Attribute value = pending.pop_back_val();
    if (auto text = dyn_cast_or_null<StringAttr>(value)) {
      if (failed(pay(Budget::bytes(text.getValue().size()))))
        return failure();
    } else if (failed(children(value)))
      return failure();
  }
  return success();
}
int compareCheck(DictionaryAttr lhs, DictionaryAttr rhs) {
  for (StringRef key : {"registration", "check", "obligation"})
    if (int order =
            detail::compareClosedSourceStructure(lhs.get(key), rhs.get(key)))
      return order;
  return 0;
}
} // namespace
LogicalResult
detail::verifySourceObservationStructure(SourceObserveOp observe) {
  auto error = [&] { return observe.emitOpError(); };
  auto kind = observe->getAttrOfType<StringAttr>("kind");
  auto spec = observe->getAttrOfType<DictionaryAttr>("spec");
  auto *parent = observe->getParentOp();
  bool source = isa_and_nonnull<SourceMethodOp, SourceTransactionOp>(parent);
  if (source) {
    if (observe->getParentRegion() != &parent->getRegion(1) || !kind ||
        !llvm::is_contained(ArrayRef<StringRef>{"log", "report"},
                            kind.getValue()) ||
        observe->hasAttr("ac.observation_id") ||
        observe->hasAttr("ac.check_id"))
      return error() << "source observation requires direct Method/transaction "
                        "body placement, log/report kind and no final identity";
    if (failed(verifySourceEffectMetadata(
            observe->getAttrOfType<DictionaryAttr>("occurrence"),
            observe->getAttrOfType<DictionaryAttr>("ac.location"),
            parent->getParentOp(), error)))
      return failure();
  } else {
    if (!isa<ModuleOp>(parent) || !kind ||
        !llvm::is_contained(ArrayRef<StringRef>{"print", "log", "report"},
                            kind.getValue()))
      return error() << "observation requires module placement and known kind";
    if (failed(verifyObservationID(
            observe->getAttrOfType<DictionaryAttr>("ac.observation_id"),
            error)) ||
        failed(verifySourceSpan(
            observe->getAttrOfType<DictionaryAttr>("ac.location"), error)))
      return failure();
  }
  if (observe->getNumOperands() == 0 ||
      !isa<BitsType, TypeParamType>(observe->getOperand(0).getType()))
    return error() << "observation path requires scalar Bits";
  HardwareBindings bindings;
  bindings.owner = source ? parent->getParentOp() : parent;
  if (source &&
      failed(predicate(
          HardwareAnalysis(observe->getParentOfType<mlir::ModuleOp>()),
          observe->getOperand(0), bindings, observe)))
    return failure();
  for (auto operand : observe->getOperands().drop_front())
    if (!isa<BitsType, EnumType, TypeParamType>(operand.getType()))
      return error() << "observation value requires a scalar hardware payload";
  return verifySourceObservationSpec(kind, spec, observe->getNumOperands() - 1,
                                     error);
}

LogicalResult detail::verifySourceObservationSpec(StringAttr kind,
                                                  DictionaryAttr spec,
                                                  uint64_t valueCount,
                                                  EmitError error) {
  if (!kind || !spec)
    return error() << "observation requires a known kind and closed spec";
  auto text = [&](StringRef key, bool nonempty = false) {
    auto value = spec.getAs<StringAttr>(key);
    return value && (!nonempty || !value.getValue().empty());
  };
  if (kind.getValue() == "report") {
    if (spec.size() != 1 || !text("name", true) || valueCount != 1)
      return error() << "report requires a nonempty name and one scalar value";
    return success();
  }
  if (spec.size() != 3 ||
      (kind.getValue() == "print" && (!text("sep") || !text("end"))) ||
      (kind.getValue() == "log" &&
       (!text("event", true) || !text("level") ||
        !llvm::is_contained(
            ArrayRef<StringRef>{"debug", "info", "warning", "error"},
            spec.getAs<StringAttr>("level").getValue()))))
    return error() << "observation spec has invalid closed fields";
  auto items = spec.getAs<ArrayAttr>("items");
  if (!items)
    return error() << "observation items must be an array";
  uint64_t next = 0;
  for (auto raw : items) {
    auto item = dyn_cast<DictionaryAttr>(raw);
    auto itemKind = item ? item.getAs<StringAttr>("kind") : StringAttr();
    if (!item || item.size() != 2 || !itemKind)
      return error() << "observation item requires exact kind and payload";
    if (itemKind.getValue() == "literal") {
      if (!item.getAs<StringAttr>("text"))
        return error() << "literal observation item requires text";
    } else if (itemKind.getValue() == "value") {
      auto ordinal = decodeU32(item.getAs<IntegerAttr>("ordinal"),
                               "observation item ordinal", error);
      if (failed(ordinal) || *ordinal != next++)
        return error()
               << "observation items must cover values exactly in order";
    } else {
      return error() << "observation item kind must be literal or value";
    }
  }
  if (next != valueCount)
    return error() << "observation items must cover values exactly in order";
  return success();
}

LogicalResult detail::verifyModuleObservationIdentities(ModuleOp definition) {
  SmallVector<ObservationLocalKey, 0> keys;
  for (auto observe : definition.getBody().front().getOps<SourceObserveOp>()) {
    if (failed(verifySourceObservationStructure(observe)))
      return failure();
    auto identity = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
    ObservationLocalKey key{identity, identity.getAs<DictionaryAttr>("owner"),
                            observe.getKind() == "report"
                                ? observe.getSpec().getAs<StringAttr>("name")
                                : StringAttr()};
    for (const auto &prior : keys) {
      if (prior.identity == key.identity)
        return observe.emitOpError() << "duplicate full ObservationID";
      if (key.reportName && prior.reportName == key.reportName &&
          prior.owner == key.owner)
        return observe.emitOpError()
               << "report name must be unique within logical owner: "
               << key.reportName.getValue();
    }
    keys.push_back(key);
  }
  return success();
}

FailureOr<detail::ResolvedSourceEffectOwner> detail::resolveSourceEffectOwner(
    const HardwareAnalysis &analysis, ModuleOp physical,
    const HardwareBindings &physicalBindings, DictionaryAttr registration,
    DictionaryAttr site, ArrayAttr invocation, DictionaryAttr location,
    Operation *diagnosticSite,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  using Budget = SourceCheckBudget;
  auto pay = [&](uint64_t cost, Operation *operation) {
    return debit ? debit(cost, operation) : success();
  };
  auto metadata = [&](Attribute attr) {
    return payEffectMetadata(attr, diagnosticSite, debit);
  };
  auto error = [&] { return diagnosticSite->emitOpError(); };
  for (Attribute attr : {Attribute(registration), Attribute(site),
                         Attribute(invocation), Attribute(location)})
    if (failed(metadata(attr)))
      return failure();
  if (!physical || physicalBindings.owner != physical ||
      physical->getParentOp() != analysis.getPackage().getOperation() ||
      !physical.getBody().hasOneBlock())
    return error()
           << "source effect bindings require its analyzed physical Module";
  if (failed(verifyOccurrence(registration, error)) ||
      failed(verifyOccurrence(site, error)) ||
      failed(verifySourceSpan(location, error)) || !invocation)
    return failure();
  for (Attribute raw : invocation) {
    auto frame = dyn_cast<DictionaryAttr>(raw);
    if (!frame || frame.size() != 2 ||
        failed(verifyOccurrence(frame.getAs<DictionaryAttr>("call"), error)) ||
        failed(verifyOwnerRef(frame.getAs<DictionaryAttr>("receiver"), error)))
      return error()
             << "source effect invocation requires exact call/receiver frames";
  }
  if (failed(pay(Budget::add(Budget::bytes(sizeof(ResolvedSourceEffectOwner)),
                             Budget::bindings(physicalBindings)),
                 diagnosticSite)))
    return failure();
  ResolvedSourceEffectOwner result{physical, physicalBindings, {}};
  auto origin = [&](DictionaryAttr occurrence, ModuleOp module,
                    StringRef description) -> LogicalResult {
    if (failed(metadata(occurrence)))
      return failure();
    return verifyOriginDefinition(
        occurrence, FlatSymbolRefAttr::get(module.getSymNameAttr()),
        description, [&] { return diagnosticSite->emitOpError(); });
  };
  if (failed(origin(registration, physical, "source effect registration")))
    return failure();
  // Every receiver is root-relative and must extend the preceding receiver.
  auto descend = [&](ArrayAttr path) -> LogicalResult {
    if (path.size() < result.path.size())
      return diagnosticSite->emitOpError()
             << "source effect receiver cannot ascend";
    for (size_t index = 0; index < result.path.size(); ++index) {
      if (failed(pay(1, diagnosticSite)) ||
          failed(metadata(result.path[index]->getAttr("occurrence"))))
        return failure();
      if (result.path[index]->getAttr("occurrence") != path[index])
        return diagnosticSite->emitOpError()
               << "source effect receiver must descend";
    }
    for (size_t index = result.path.size(); index < path.size(); ++index) {
      InstanceOp found;
      for (auto &op : result.terminal.getBody().front()) {
        if (!isa<InstanceOp, CollectionOp>(op))
          continue;
        if (failed(pay(1, &op)) || failed(metadata(op.getAttr("occurrence"))))
          return failure();
        if (failed(
                verifyOccurrence(op.getAttrOfType<DictionaryAttr>("occurrence"),
                                 [&] { return op.emitOpError(); })))
          return failure();
        if (op.getAttr("occurrence") != path[index])
          continue;
        if (isa<CollectionOp>(op) || found)
          return diagnosticSite->emitOpError()
                 << "source effect owner requires unique ordinary instances";
        found = cast<InstanceOp>(op);
      }
      auto child =
          found ? dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(found))
                : ModuleOp();
      if (!child || !child.getBody().hasOneBlock() ||
          !analysis.getPrimitiveKind(child).empty())
        return diagnosticSite->emitOpError()
               << "source effect owner path must resolve ordinary definitions";
      if (failed(origin(found->getAttrOfType<DictionaryAttr>("occurrence"),
                        result.terminal, "source effect owner allocation")))
        return failure();
      if (failed(pay(Budget::add(Budget::bytes(sizeof(InstanceOp)),
                                 Budget::bindings(result.bindings)),
                     diagnosticSite)))
        return failure();
      auto bindings = analysis.bindInstance(found, result.bindings);
      if (failed(bindings))
        return failure();
      if (failed(pay(Budget::bindings(*bindings), diagnosticSite)))
        return failure();
      result.path.push_back(found);
      result.terminal = child;
      result.bindings = std::move(*bindings);
    }
    return success();
  };
  for (auto raw : invocation) {
    auto frame = cast<DictionaryAttr>(raw);
    if (failed(origin(frame.getAs<DictionaryAttr>("call"), result.terminal,
                      "source effect invocation call")) ||
        failed(descend(frame.getAs<DictionaryAttr>("receiver")
                           .getAs<ArrayAttr>("instance_path"))))
      return failure();
  }
  if (failed(origin(site, result.terminal, "source effect site")))
    return failure();
  auto sourceOwner =
      result.terminal->getAttrOfType<DictionaryAttr>("source_owner");
  if (failed(metadata(sourceOwner)) ||
      failed(verifySourceOwner(sourceOwner,
                               [&] { return diagnosticSite->emitOpError(); })))
    return failure();
  if (location.getAs<StringAttr>("path") !=
      sourceOwner.getAs<StringAttr>("path"))
    return diagnosticSite->emitOpError()
           << "source effect span must match logical owner SourceOwner";
  return std::move(result);
}

FailureOr<detail::ResolvedSourceEffectOwner> detail::resolveObservationOwner(
    const HardwareAnalysis &analysis, SourceObserveOp observe,
    const HardwareBindings &physicalBindings,
    const std::function<LogicalResult(uint64_t, Operation *)> &debit) {
  using Budget = SourceCheckBudget;
  auto pay = [&](uint64_t cost) {
    return debit ? debit(cost, observe) : success();
  };
  // Pay actual terminal shape/spec validation before using its typed identity.
  for (StringRef key : {"ac.observation_id", "ac.location", "spec", "kind"})
    if (failed(payEffectMetadata(observe->getAttr(key), observe, debit)))
      return failure();
  if (failed(pay(
          Budget::add(8, Budget::multiply(2, observe->getNumOperands())))) ||
      failed(verifySourceObservationStructure(observe)) ||
      failed(predicate(analysis, observe->getOperand(0), physicalBindings,
                       observe)))
    return failure();
  auto id = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
  return resolveSourceEffectOwner(
      analysis, dyn_cast<ModuleOp>(observe->getParentOp()), physicalBindings,
      id.getAs<DictionaryAttr>("registration"),
      id.getAs<DictionaryAttr>("site"), id.getAs<ArrayAttr>("invocation"),
      observe->getAttrOfType<DictionaryAttr>("ac.location"), observe, debit);
}

FailureOr<SmallVector<HardwareCheckBinding>>
detail::definitionSourceChecks(const HardwareAnalysis &analysis,
                               ModuleOp definition,
                               const HardwareBindings &bindings,
                               detail::SampledAdmissionValidation &validation,
                               detail::SourceCheckBudget &budget) {
  if (failed(validation.verifyInvocation(analysis, budget)))
    return failure();
  if (!definition)
    return analysis.getPackage().emitOpError()
           << "source checks require a Module definition";
  if (definition->getNumRegions() != 1 || !definition.getBody().hasOneBlock())
    return definition.emitOpError()
           << "source checks require one definition graph block";
  SmallVector<HardwareCheckBinding> result;
  std::map<DictionaryAttr, size_t, bool (*)(DictionaryAttr, DictionaryAttr)>
      identities([](DictionaryAttr lhs, DictionaryAttr rhs) {
        return compareCheck(lhs, rhs) < 0;
      });
  llvm::DenseMap<Operation *, detail::SampledAdmissionView> sampledRules;
  auto prepared = validation.prepare(definition, bindings);
  if (failed(prepared))
    return failure();
  for (auto rule : definition.getBody().front().getOps<RuleOp>()) {
    auto error = [&] { return rule.emitOpError(); };
    if (failed(mlir::verify(rule)) ||
        failed(detail::verifyOccurrence(rule.getOccurrence(), error)))
      return failure();
    auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    if (rule->hasAttr("ac.required_checks") && !required)
      return error() << "ac.required_checks must be an array";
    if (rule->hasAttr("ac.sampled_admission")) {
      auto sampled = validation.verify(rule, bindings);
      if (failed(sampled))
        return failure();
      if (!required || required.size() != detail::SourceCheckBudget::add(
                                              sampled->guards.size(),
                                              sampled->bodyChecks.size()))
        return error() << "sampled Rule requires exactly its ordered "
                          "guard/assert obligations";
      if (failed(budget.debit(
              detail::SourceCheckBudget::bytes(
                  sizeof(std::pair<Operation *, detail::SampledAdmissionView>)),
              rule)))
        return failure();
      sampledRules.try_emplace(rule, std::move(*sampled));
    }
    if (!required)
      continue;
    if (required.size() > rule.getNumResults() / 2)
      return error()
             << "source check suffix requires two results per obligation";
    uint64_t first = rule.getNumResults() - 2 * required.size();
    for (auto [index, raw] : llvm::enumerate(required)) {
      auto record = dyn_cast<DictionaryAttr>(raw);
      auto id = record ? record.getAs<DictionaryAttr>("id") : DictionaryAttr();
      auto kind = record ? record.getAs<StringAttr>("kind") : StringAttr();
      auto location =
          record ? record.getAs<DictionaryAttr>("location") : DictionaryAttr();
      if (!record || record.size() != 3 || !kind ||
          !llvm::is_contained(
              ArrayRef<StringRef>{"assert", "range", "guard_known"},
              kind.getValue()))
        return error() << "RequiredCheck requires exact id, kind and location";
      if (failed(detail::verifyCheckID(id, error)) ||
          failed(detail::verifySourceSpan(location, error)))
        return failure();
      if (detail::compareClosedSourceStructure(id.get("registration"),
                                               rule.getOccurrence()) != 0)
        return error() << "RequiredCheck registration must equal owning rule "
                          "occurrence";
      uint64_t conditionResult = first + 2 * index;
      uint64_t pathResult = conditionResult + 1;
      auto sampled = sampledRules.find(rule);
      if (kind.getValue() == "guard_known") {
        if (sampled == sampledRules.end())
          return error() << "guard_known requires sampled admission";
        if (index >= sampled->second.guards.size())
          return error() << "guard_known is outside sampled guard inventory";
        const auto &guard = sampled->second.guards[index];
        if (id != guard.id || conditionResult != guard.predicateResult ||
            pathResult != guard.reachedResult)
          return error() << "guard_known differs from sampled guard suffix";
      } else if (sampled != sampledRules.end()) {
        uint64_t guardCount = sampled->second.guards.size();
        if (kind.getValue() != "assert" || index < guardCount ||
            index - guardCount >= sampled->second.bodyChecks.size())
          return error() << "sampled admission only permits ordered "
                            "guard_known/assert checks";
        const auto &body = sampled->second.bodyChecks[index - guardCount];
        if (id != body.id || conditionResult != body.conditionResult ||
            pathResult != body.pathResult)
          return error() << "assert differs from sampled body check suffix";
      }
      if (failed(budget.debit(detail::SourceCheckBudget::bytes(
                                  sizeof(HardwareCheckBinding) +
                                  sizeof(std::pair<DictionaryAttr, size_t>) +
                                  3 * sizeof(void *)),
                              rule)))
        return failure();
      if (!identities.emplace(id, result.size()).second)
        return error() << "duplicate full source CheckID";
      if (failed(predicate(analysis, rule.getResult(conditionResult), bindings,
                           rule)) ||
          failed(
              predicate(analysis, rule.getResult(pathResult), bindings, rule)))
        return failure();
      result.push_back({definition,
                        rule,
                        {},
                        id,
                        kind,
                        location,
                        {},
                        {},
                        {},
                        index,
                        conditionResult,
                        pathResult});
    }
  }
  for (auto expect : definition.getBody().front().getOps<SourceExpectOp>()) {
    if (failed(detail::verifySourceExpectOperation(analysis, expect, bindings)))
      return failure();
    auto id = expect->getAttrOfType<DictionaryAttr>("ac.check_id");
    auto position = identities.find(id);
    if (position == identities.end())
      return expect.emitOpError()
             << "orphan source check without RequiredCheck owner";
    auto &found = result[position->second];
    if (found.expect)
      return expect.emitOpError() << "duplicate expect for source CheckID";
    if (found.kind != expect.getKindAttr() ||
        found.location != expect.getLocationAttr())
      return expect.emitOpError()
             << "expect kind/location differs from RequiredCheck";
    auto condition = carrier(analysis, expect.getCondition(), bindings, budget);
    auto path = carrier(analysis, expect.getPath(), bindings, budget);
    if (failed(condition) || failed(path))
      return failure();
    if (*condition != found.rule.getResult(found.conditionResult) ||
        *path != found.rule.getResult(found.pathResult))
      return expect.emitOpError()
             << "expect operands must match owning rule check suffix";
    if (found.kind.getValue() == "guard_known") {
      auto sampled = sampledRules.find(found.rule);
      if (sampled == sampledRules.end())
        return expect.emitOpError() << "guard_known has no sampled Rule";
      detail::HardwareIdentityStopPolicy stops{
          detail::HardwareIdentityTraversal::DefinitionLocal,
          (*prepared)->getBoundaries()};
      const Value captures[] = {sampled->second.clock, sampled->second.reset};
      for (auto [actual, capture] : llvm::zip(expect.getSampling(), captures)) {
        auto lhs = detail::makeIdentityEndpoint(actual, (*prepared)->getScope(),
                                                analysis, budget);
        auto rhs = detail::makeIdentityEndpoint(
            capture, (*prepared)->getScope(), analysis, budget);
        if (failed(lhs) || failed(rhs))
          return failure();
        auto normalizedActual =
            detail::normalizeIdentityEndpoint(*lhs, stops, analysis, budget);
        auto normalizedCapture =
            detail::normalizeIdentityEndpoint(*rhs, stops, analysis, budget);
        if (failed(normalizedActual) || failed(normalizedCapture))
          return failure();
        if (normalizedActual->value != normalizedCapture->value ||
            normalizedActual->resolvedType != normalizedCapture->resolvedType)
          return expect.emitOpError()
                 << "expect sampling pins differ from owning Rule captures";
      }
    }
    found.expect = expect;
    found.condition = expect.getCondition();
    found.path = expect.getPath();
    found.message = expect->getAttrOfType<StringAttr>("ac.message");
  }
  for (const auto &binding : result)
    if (!binding.expect)
      return definition.emitOpError() << "RequiredCheck has no matching expect";
  // Reuse the structural map order instead of sorting/recomparing the IDs.
  SmallVector<HardwareCheckBinding> ordered;
  if (failed(budget.debit(detail::SourceCheckBudget::multiply(
                              result.size(), detail::SourceCheckBudget::bytes(
                                                 sizeof(HardwareCheckBinding))),
                          definition)))
    return failure();
  ordered.reserve(result.size());
  // Structural order establishes registration ordering and uniqueness. Within
  // each sampled registration replay the authenticated RequiredCheck order.
  llvm::DenseSet<Operation *> emitted;
  if (failed(budget.debit(
          detail::SourceCheckBudget::multiply(
              sampledRules.size(),
              detail::SourceCheckBudget::bytes(sizeof(Operation *))),
          definition)))
    return failure();
  for (const auto &[id, index] : identities) {
    const auto &binding = result[index];
    if (!sampledRules.count(binding.rule)) {
      ordered.push_back(binding);
      continue;
    }
    if (!emitted.insert(binding.rule).second)
      continue;
    const auto &sampled = sampledRules.find(binding.rule)->second;
    uint64_t count = detail::SourceCheckBudget::add(sampled.guards.size(),
                                                    sampled.bodyChecks.size());
    if (index < binding.requiredIndex)
      return definition.emitOpError()
             << "sampled check ordering lost its first obligation";
    uint64_t first = index - binding.requiredIndex;
    if (first > result.size() || count > result.size() - first)
      return definition.emitOpError()
             << "sampled check ordering exceeds catalog";
    for (uint64_t ordinal = 0; ordinal < count; ++ordinal) {
      if (failed(budget.debit(1, definition)))
        return failure();
      const auto &found = result[first + ordinal];
      if (found.rule != binding.rule || found.requiredIndex != ordinal)
        return definition.emitOpError()
               << "sampled check ordering lost an obligation";
      ordered.push_back(found);
    }
  }
  return ordered;
}
bool detail::sameHardwareBindings(const HardwareBindings &lhs,
                                  const HardwareBindings &rhs) {
  if (lhs.owner != rhs.owner || lhs.integers.size() != rhs.integers.size() ||
      lhs.types.size() != rhs.types.size())
    return false;
  for (const auto &entry : lhs.integers) {
    auto found = rhs.integers.find(entry.first());
    if (found == rhs.integers.end() || found->second != entry.second)
      return false;
  }
  for (const auto &entry : lhs.types) {
    auto found = rhs.types.find(entry.first());
    if (found == rhs.types.end() || found->second != entry.second)
      return false;
  }
  return true;
}
namespace detail {
LogicalResult verifySourceExpectOperation(const HardwareAnalysis &analysis,
                                          SourceExpectOp expect,
                                          const HardwareBindings &bindings) {
  auto error = [&] { return expect.emitOpError(); };
  auto *parent = expect->getParentOp();
  bool source = isa_and_nonnull<SourceMethodOp, SourceTransactionOp>(parent);
  if (source) {
    if (expect->getParentRegion() != &parent->getRegion(1) ||
        expect.getKind() != "assert" || expect->hasAttr("ac.check_id") ||
        expect->hasAttr("ac.observation_id") || !expect.getSampling().empty())
      return error() << "source assertion requires direct Method/transaction "
                        "body placement, assert kind, no sampling and no final "
                        "identity";
    if (failed(verifySourceEffectMetadata(
            expect->getAttrOfType<DictionaryAttr>("occurrence"),
            expect.getLocationAttr(), parent->getParentOp(), error)))
      return failure();
  } else {
    if (!isa<ModuleOp>(parent) ||
        !llvm::is_contained(
            ArrayRef<StringRef>{"assert", "range", "guard_known"},
            expect.getKind()))
      return error() << "check requires module placement and known kind";
    if (failed(verifyCheckID(
            expect->getAttrOfType<DictionaryAttr>("ac.check_id"), error)) ||
        failed(verifySourceSpan(expect.getLocationAttr(), error)))
      return failure();
  }
  if (expect->hasAttr("ac.message") &&
      !expect->getAttrOfType<StringAttr>("ac.message"))
    return error() << "source check message must be a static string";
  if (expect.getSampling().size() !=
      (expect.getKind() == "guard_known" ? 2u : 0u))
    return error() << "guard_known requires clock/reset sampling operands; "
                      "assert/range require no sampling operands";
  for (Value pin : expect.getSampling())
    if (failed(predicate(analysis, pin, bindings, expect)))
      return failure();
  if (failed(predicate(analysis, expect.getCondition(), bindings, expect)) ||
      failed(predicate(analysis, expect.getPath(), bindings, expect)))
    return failure();
  return success();
}
FailureOr<HardwareBindings>
resolveHardwareRootBindings(const HardwareAnalysis &analysis) {
  auto package = analysis.getPackage();
  auto systems = package.getOps<SystemOp>();
  if (!llvm::hasSingleElement(systems))
    return package.emitOpError()
           << "executable hardware package requires exactly one ac.system";
  auto system = *systems.begin();
  auto entry = system.getEntry();
  auto callee = entry.getAs<FlatSymbolRefAttr>("callee");
  auto root = callee ? dyn_cast_or_null<ModuleOp>(
                           analysis.lookupDefinition(callee.getValue()))
                     : ModuleOp();
  if (!root)
    return system.emitOpError()
           << "system entry must resolve to module definition";
  HardwareBindings bindings;
  bindings.owner = root;
  auto parameters = entry.getAs<ArrayAttr>("parameters");
  auto types = entry.getAs<ArrayAttr>("type_arguments");
  if (!parameters)
    parameters = ArrayAttr::get(package.getContext(), {});
  if (!types)
    types = ArrayAttr::get(package.getContext(), {});
  if (types.size() != root.getTypeParameters().size() ||
      parameters.size() > root.getParameters().size())
    return system.emitOpError() << "system entry parameter arity mismatch";
  for (auto [name, type] : llvm::zip(root.getTypeParameters(), types)) {
    auto attr = dyn_cast<TypeAttr>(type);
    if (!attr)
      return system.emitOpError() << "system type argument must be TypeAttr";
    bindings.types[cast<StringAttr>(name).getValue()] = attr.getValue();
    if (failed(analysis.getPackedWidth(attr.getValue(), {}, system)))
      return failure();
  }
  for (auto [index, raw] : llvm::enumerate(root.getParameters())) {
    auto formal = cast<DictionaryAttr>(raw);
    auto expr = index < parameters.size()
                    ? dyn_cast<StaticExprAttr>(parameters[index])
                    : formal.getAs<StaticExprAttr>("default");
    if (!expr)
      return system.emitOpError() << "missing root integer parameter";
    auto value = analysis.evaluateStatic(expr, bindings, system);
    if (failed(value))
      return failure();
    if (!isa<MathIntAttr>(*value))
      return system.emitOpError()
             << "root integer parameter requires math integer";
    bindings.integers[formal.getAs<StringAttr>("name").getValue()] = *value;
  }
  return bindings;
}
} // namespace detail

LogicalResult
HardwareAnalysis::verifyDefinitionSourceChecks(ModuleOp definition) const {
  if (!definition || definition->getParentOp() != package.getOperation())
    return package.emitOpError()
           << "source check definition must belong to analyzed package";
  HardwareBindings bindings;
  bindings.owner = definition;
  detail::SourceCheckBudget budget(HardwareSourceCheckLimits{});
  budget.setPhase("definition source checks");
  detail::SampledAdmissionValidation validation(*this, budget);
  return success(succeeded(detail::definitionSourceChecks(
      *this, definition, bindings, validation, budget)));
}

FailureOr<HardwareSourceCheckPlan>
HardwareAnalysis::getSourceCheckPlan(HardwareSourceCheckLimits limits) const {
  return detail::buildSourceCheckPlan(*this, limits);
}

LogicalResult HardwareAnalysis::verifySourceCheckPlan(
    const HardwareSourceCheckPlan &plan,
    HardwareSourceCheckLimits limits) const {
  if (!package)
    return failure();
  // Recompute from current SSA, attributes and bindings; no saved plan grants
  // authority to an edited package or a removed check/commit endpoint.
  HardwareAnalysis current(package);
  auto fresh = current.getSourceCheckPlan(limits);
  if (failed(fresh))
    return failure();
  auto mismatch = [&] {
    return package.emitOpError()
           << "source check plan differs from current hardware package";
  };
  if (plan.bindings.size() != fresh->bindings.size() ||
      plan.owners.size() != fresh->owners.size() ||
      plan.checks.size() != fresh->checks.size() ||
      plan.commits.size() != fresh->commits.size() ||
      plan.samplingBindings.size() != fresh->samplingBindings.size() ||
      plan.samplingOccurrences.size() != fresh->samplingOccurrences.size())
    return mismatch();
  for (auto [lhs, rhs] : llvm::zip(plan.bindings, fresh->bindings))
    if (lhs.definition != rhs.definition || lhs.rule != rhs.rule ||
        lhs.expect != rhs.expect || lhs.checkID != rhs.checkID ||
        lhs.kind != rhs.kind || lhs.location != rhs.location ||
        lhs.message != rhs.message || lhs.condition != rhs.condition ||
        lhs.path != rhs.path || lhs.requiredIndex != rhs.requiredIndex ||
        lhs.conditionResult != rhs.conditionResult ||
        lhs.pathResult != rhs.pathResult ||
        lhs.samplingBindingIndex != rhs.samplingBindingIndex)
      return mismatch();
  for (auto [lhs, rhs] : llvm::zip(plan.owners, fresh->owners)) {
    if (lhs.definition != rhs.definition || lhs.parent != rhs.parent ||
        lhs.ordinal != rhs.ordinal ||
        !detail::sameHardwareBindings(lhs.bindings, rhs.bindings) ||
        lhs.path.size() != rhs.path.size())
      return mismatch();
    for (auto [left, right] : llvm::zip(lhs.path, rhs.path))
      if (left.allocation != right.allocation ||
          left.coordinates != right.coordinates)
        return mismatch();
  }
  for (auto [lhs, rhs] : llvm::zip(plan.checks, fresh->checks))
    if (lhs.bindingIndex != rhs.bindingIndex ||
        lhs.ownerOccurrence != rhs.ownerOccurrence ||
        lhs.ordinal != rhs.ordinal || lhs.reset.kind != rhs.reset.kind ||
        lhs.reset.ownerOccurrence != rhs.reset.ownerOccurrence ||
        lhs.reset.inputOrdinal != rhs.reset.inputOrdinal ||
        lhs.samplingOccurrenceIndex != rhs.samplingOccurrenceIndex)
      return mismatch();
  for (auto [lhs, rhs] : llvm::zip(plan.commits, fresh->commits))
    if (lhs.ownerOccurrence != rhs.ownerOccurrence ||
        lhs.allocation != rhs.allocation ||
        lhs.coordinates != rhs.coordinates ||
        lhs.primitiveKind != rhs.primitiveKind)
      return mismatch();
  for (auto [lhs, rhs] :
       llvm::zip(plan.samplingBindings, fresh->samplingBindings))
    if (lhs.definition != rhs.definition || lhs.clock != rhs.clock ||
        lhs.reset != rhs.reset || lhs.localOrdinal != rhs.localOrdinal)
      return mismatch();
  for (auto [lhs, rhs] :
       llvm::zip(plan.samplingOccurrences, fresh->samplingOccurrences))
    if (lhs.bindingIndex != rhs.bindingIndex ||
        lhs.ownerOccurrence != rhs.ownerOccurrence ||
        lhs.ordinal != rhs.ordinal)
      return mismatch();
  return success();
}
} // namespace acir::ac

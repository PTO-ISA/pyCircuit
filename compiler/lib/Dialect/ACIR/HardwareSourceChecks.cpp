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
int compareCheck(DictionaryAttr lhs, DictionaryAttr rhs) {
  for (StringRef key : {"registration", "check", "obligation"})
    if (int order =
            detail::compareClosedSourceStructure(lhs.get(key), rhs.get(key)))
      return order;
  return 0;
}
} // namespace
FailureOr<SmallVector<HardwareCheckBinding>> detail::definitionSourceChecks(
    const HardwareAnalysis &analysis, ModuleOp definition,
    const HardwareBindings &bindings, detail::SourceCheckBudget &budget) {
  if (!definition.getBody().hasOneBlock())
    return definition.emitOpError()
           << "source checks require one definition graph block";
  SmallVector<HardwareCheckBinding> result;
  std::map<DictionaryAttr, size_t, bool (*)(DictionaryAttr, DictionaryAttr)>
      identities([](DictionaryAttr lhs, DictionaryAttr rhs) {
        return compareCheck(lhs, rhs) < 0;
      });
  llvm::DenseMap<Operation *, detail::SampledAdmissionView> sampledRules;
  bool sampledDefinition =
      definition->hasAttr("ac.transaction_wiring") ||
      llvm::any_of(
          definition.getBody().front().getOps<RuleOp>(),
          [](RuleOp rule) { return rule->hasAttr("ac.sampled_admission"); });
  detail::HardwareIdentityScope identityScope;
  if (sampledDefinition) {
    if (failed(budget.debit(detail::SourceCheckBudget::add(
                                detail::SourceCheckBudget::bindings(bindings),
                                detail::SourceCheckBudget::bytes(
                                    sizeof(detail::HardwareIdentityScope))),
                            definition)))
      return failure();
    identityScope = {definition, bindings, {}};
  }
  SmallVector<detail::HardwareIdentityEndpoint> stopEndpoints;
  auto addStop = [&](Value value) -> LogicalResult {
    if (failed(budget.debit(detail::SourceCheckBudget::bytes(
                                sizeof(detail::HardwareIdentityEndpoint)),
                            definition)))
      return failure();
    auto endpoint =
        detail::makeIdentityEndpoint(value, identityScope, analysis, budget);
    if (failed(endpoint))
      return failure();
    stopEndpoints.push_back(std::move(*endpoint));
    return success();
  };
  if (definition->hasAttr("ac.transaction_wiring")) {
    auto wiring =
        detail::parseTransactionWiring(definition, analysis, bindings, budget);
    if (failed(wiring))
      return failure();
    for (auto &writer : wiring->writers)
      for (uint64_t index = writer.ordinaryResults;
           index < writer.rule.getNumResults(); ++index)
        if (failed(addStop(writer.rule.getResult(index))))
          return failure();
  }
  if (sampledDefinition) {
    for (auto rule : definition.getBody().front().getOps<RuleOp>()) {
      auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
      if (!required || required.size() > rule.getNumResults() / 2)
        continue;
      for (uint64_t index = rule.getNumResults() - 2 * required.size();
           index < rule.getNumResults(); ++index)
        if (failed(addStop(rule.getResult(index))))
          return failure();
    }
  }
  for (auto rule : definition.getBody().front().getOps<RuleOp>()) {
    auto error = [&] { return rule.emitOpError(); };
    if (failed(mlir::verify(rule)) ||
        failed(detail::verifyOccurrence(rule.getOccurrence(), error)))
      return failure();
    auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
    if (rule->hasAttr("ac.required_checks") && !required)
      return error() << "ac.required_checks must be an array";
    if (rule->hasAttr("ac.sampled_admission")) {
      auto sampled =
          detail::parseSampledAdmission(rule, analysis, bindings, budget);
      if (failed(sampled) || failed(detail::proveSampledAdmission(
                                 *sampled, analysis, bindings, budget)))
        return failure();
      if (!required || required.size() != sampled->guards.size())
        return error() << "sampled Rule requires exactly its guard obligations";
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
        const auto &guard = sampled->second.guards[index];
        if (id != guard.id || conditionResult != guard.predicateResult ||
            pathResult != guard.reachedResult)
          return error() << "guard_known differs from sampled guard suffix";
      } else if (sampled != sampledRules.end()) {
        return error() << "sampled admission only permits guard_known checks";
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
          detail::HardwareIdentityTraversal::DefinitionLocal, stopEndpoints};
      const Value captures[] = {sampled->second.clock, sampled->second.reset};
      for (auto [actual, capture] : llvm::zip(expect.getSampling(), captures)) {
        auto lhs = detail::makeIdentityEndpoint(actual, identityScope, analysis,
                                                budget);
        auto rhs = detail::makeIdentityEndpoint(capture, identityScope,
                                                analysis, budget);
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
  for (const auto &[id, index] : identities)
    ordered.push_back(result[index]);
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
  if (!isa<ModuleOp>(expect->getParentOp()) ||
      !llvm::is_contained(ArrayRef<StringRef>{"assert", "range", "guard_known"},
                          expect.getKind()))
    return error() << "check requires module placement and known kind";
  if (failed(verifyCheckID(expect->getAttrOfType<DictionaryAttr>("ac.check_id"),
                           error)) ||
      failed(verifySourceSpan(expect.getLocationAttr(), error)))
    return failure();
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
  return success(succeeded(
      detail::definitionSourceChecks(*this, definition, bindings, budget)));
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

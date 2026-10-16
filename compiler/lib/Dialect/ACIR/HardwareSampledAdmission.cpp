#include "HardwareSampledAdmission.h"
#include "ACIRSourceContracts.h"
#include "HardwareSourceCheckPreflight.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <tuple>

using namespace mlir;
namespace acir::ac::detail {
namespace {
LogicalResult charge(SourceCheckBudget &budget, Operation *site, uint64_t count,
                     uint64_t size) {
  return budget.debit(
      SourceCheckBudget::multiply(
          count, SourceCheckBudget::add(1, SourceCheckBudget::bytes(size))),
      site);
}
FailureOr<int64_t> index(DictionaryAttr row, StringRef key, Operation *site,
                         bool predecessor = false) {
  auto attr = row ? row.getAs<IntegerAttr>(key) : IntegerAttr();
  auto type = attr ? dyn_cast<IntegerType>(attr.getType()) : IntegerType();
  if (!type || !type.isSignless() || type.getWidth() != 64 ||
      attr.getInt() < (predecessor ? -1 : 0))
    return site->emitOpError() << key << " must be a nonnegative signless i64"
                               << (predecessor ? " (or predecessor -1)" : "");
  return attr.getInt();
}
LogicalResult exact(DictionaryAttr row, ArrayRef<StringRef> fields,
                    Operation *site, StringRef name) {
  if (!row || row.size() != fields.size())
    return site->emitOpError() << name << " requires its exact closed fields";
  for (auto field : fields)
    if (!row.get(field))
      return site->emitOpError() << name << " is missing " << field;
  return success();
}
// Closed identity validators and structural comparators walk nested metadata.
// Prepay every visited node before invoking those existing owners; there is no
// alternate identity parser or unaccounted comparison traversal here.
LogicalResult payAttribute(Attribute attr, SourceCheckBudget &budget,
                           Operation *site) {
  if (failed(budget.debit(1, site)))
    return failure();
  if (!attr)
    return success();
  if (auto text = dyn_cast<StringAttr>(attr))
    return budget.debit(SourceCheckBudget::bytes(text.getValue().size()), site);
  if (!isa<ArrayAttr, DictionaryAttr>(attr))
    return success();
  SmallVector<Attribute, 0> pending;
  auto push = [&](Attribute value) {
    if (failed(budget.debit(1, site)))
      return failure();
    if (pending.size() == pending.capacity()) {
      uint64_t capacity = std::max(
          uint64_t(2), SourceCheckBudget::add(
                           SourceCheckBudget::multiply(pending.capacity(), 2), 1));
      uint64_t storage = SourceCheckBudget::multiply(
          capacity - pending.capacity(), sizeof(Attribute));
      if (capacity == std::numeric_limits<uint64_t>::max() ||
          storage == std::numeric_limits<uint64_t>::max() ||
          capacity > pending.max_size())
        return budget.debit(std::numeric_limits<uint64_t>::max(), site);
      if (failed(budget.debit(SourceCheckBudget::add(SourceCheckBudget::bytes(storage),
                                                pending.size()), site)))
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
        if (failed(budget.debit(
                SourceCheckBudget::bytes(item.getName().getValue().size()),
                site)) ||
            failed(push(item.getValue())))
          return failure();
    }
    return success();
  };
  if (failed(children(attr)))
    return failure();
  while (!pending.empty()) {
    Attribute value = pending.pop_back_val();
    if (!value)
      continue;
    if (auto text = dyn_cast<StringAttr>(value)) {
      if (failed(budget.debit(SourceCheckBudget::bytes(text.getValue().size()),
                              site)))
        return failure();
    } else if (failed(children(value)))
      return failure();
  }
  return success();
}
LogicalResult payComparison(Attribute lhs, Attribute rhs,
                            SourceCheckBudget &budget, Operation *site) {
  if (lhs == rhs)
    return budget.debit(1, site);
  if (failed(payAttribute(lhs, budget, site)) ||
      failed(payAttribute(rhs, budget, site)))
    return failure();
  return success();
}
bool identical(Attribute lhs, Attribute rhs) {
  // Missing metadata never establishes logical identity. The owning closed
  // validators below reject malformed non-null attributes before comparison.
  return lhs && rhs &&
         (lhs == rhs || compareClosedSourceStructure(lhs, rhs) == 0);
}
Operation *valueSite(Value value) {
  if (auto result = dyn_cast<OpResult>(value))
    return result.getOwner();
  return cast<BlockArgument>(value).getOwner()->getParentOp();
}
ModuleOp valueModule(Value value) {
  Operation *site = valueSite(value);
  return isa<ModuleOp>(site) ? cast<ModuleOp>(site)
                             : site->getParentOfType<ModuleOp>();
}
LogicalResult bitOne(Value value, const HardwareAnalysis &analysis,
                     const HardwareBindings &bindings, Operation *site,
                     SourceCheckBudget &budget) {
  if (failed(budget.debit(1, site)))
    return failure();
  auto type = analysis.resolveType(value.getType(), bindings, site);
  if (failed(type))
    return failure();
  if (!isa<BitsType>(*type))
    return site->emitOpError() << "sampled admission requires Bits1 operands";
  auto width = analysis.getPackedWidth(*type, bindings, site);
  if (failed(width))
    return failure();
  if (*width != 1)
    return site->emitOpError() << "sampled admission requires width one";
  return success();
}
FailureOr<HardwareBindings> scopeBindings(const HardwareIdentityScope &scope,
                                          const HardwareAnalysis &analysis,
                                          SourceCheckBudget &budget,
                                          Operation *site) {
  if (!scope.root || scope.rootBindings.owner != scope.root.operator->())
    return site->emitOpError() << "identity root bindings have the wrong owner";
  if (failed(
          budget.debit(SourceCheckBudget::bindings(scope.rootBindings), site)))
    return failure();
  auto root = scope.root;
  if (!root.getBody().hasOneBlock())
    return site->emitOpError() << "identity root requires one graph block";
  HardwareBindings bound = scope.rootBindings;
  bound.owner = scope.root;
  ModuleOp owner = scope.root;
  for (auto instance : scope.path) {
    if (failed(budget.debit(
            SourceCheckBudget::add(1, SourceCheckBudget::bindings(bound)),
            site)))
      return failure();
    if (instance->getParentOp() != owner.operator->())
      return site->emitOpError()
             << "identity path is not an actual instance path";
    auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
    if (!child || !child.getBody().hasOneBlock() ||
        !analysis.getPrimitiveKind(child).empty())
      return site->emitOpError()
             << "identity path requires ordinary module instances";
    auto next = analysis.bindInstance(instance, bound);
    if (failed(next))
      return failure();
    owner = child;
    bound = std::move(*next);
  }
  return bound;
}
ModuleOp scopeOwner(const HardwareIdentityScope &scope,
                    const HardwareAnalysis &analysis) {
  return scope.path.empty() ? scope.root
                            : dyn_cast_or_null<ModuleOp>(
                                  analysis.resolveCallee(scope.path.back()));
}
bool sameScope(const HardwareIdentityScope &a, const HardwareIdentityScope &b) {
  return a.root == b.root && a.path == b.path &&
         sameHardwareBindings(a.rootBindings, b.rootBindings);
}
bool sameEndpoint(const HardwareIdentityEndpoint &a,
                  const HardwareIdentityEndpoint &b) {
  return a.value == b.value && sameScope(a.scope, b.scope) &&
         areEquivalentHardwareTypes(a.resolvedType, b.resolvedType);
}
LogicalResult payScope(const HardwareIdentityScope &scope,
                       SourceCheckBudget &budget, Operation *site) {
  return budget.debit(
      SourceCheckBudget::add(
          SourceCheckBudget::bindings(scope.rootBindings),
          SourceCheckBudget::add(
              SourceCheckBudget::bytes(sizeof(HardwareIdentityScope)),
              SourceCheckBudget::multiply(scope.path.size(),
                                          sizeof(InstanceOp) / 8 + 1))),
      site);
}
FailureOr<HardwareIdentityScope> copyScope(const HardwareIdentityScope &scope,
                                           SourceCheckBudget &budget,
                                           Operation *site) {
  if (failed(payScope(scope, budget, site)))
    return failure();
  return scope;
}
FailureOr<HardwareIdentityScope> childScope(const HardwareIdentityScope &scope,
                                            InstanceOp instance,
                                            SourceCheckBudget &budget) {
  auto copied = copyScope(scope, budget, instance);
  if (failed(copied) || failed(charge(budget, instance, 1, sizeof(InstanceOp))))
    return failure();
  copied->path.push_back(instance);
  return std::move(*copied);
}
FailureOr<std::optional<bool>> literal(const HardwareIdentityEndpoint &endpoint,
                                       const HardwareAnalysis &analysis,
                                       SourceCheckBudget &budget) {
  auto constant = endpoint.value.getDefiningOp<BitsConstantOp>();
  if (!constant)
    return std::optional<bool>();
  auto bound = scopeBindings(endpoint.scope, analysis, budget, constant);
  if (failed(bound) || failed(budget.debit(1, constant)))
    return failure();
  auto value = analysis.evaluateStatic(constant.getValue(), *bound, constant);
  if (failed(value))
    return failure();
  auto integer = dyn_cast<MathIntAttr>(*value);
  if (!integer)
    return constant->emitOpError()
           << "sampled literal requires a known integer";
  llvm::APSInt number(integer.getCanonicalValue());
  if (number.isZero())
    return std::optional<bool>(false);
  if (number == 1)
    return std::optional<bool>(true);
  return std::optional<bool>();
}
FailureOr<HardwareIdentityEndpoint>
normalized(Value value, const HardwareIdentityScope &scope,
           const HardwareIdentityStopPolicy &policy,
           const HardwareAnalysis &analysis, SourceCheckBudget &budget) {
  auto endpoint = makeIdentityEndpoint(value, scope, analysis, budget);
  if (failed(endpoint))
    return failure();
  return normalizeIdentityEndpoint(*endpoint, policy, analysis, budget);
}
LogicalResult wholeStateID(DictionaryAttr state, Operation *site) {
  if (failed(verifyStateID(state, [&] { return site->emitOpError(); })))
    return failure();
  if (!state.getAs<ArrayAttr>("element").empty())
    return site->emitOpError()
           << "sampled transaction state requires whole Bits ownership";
  return success();
}
FailureOr<InstanceOp> directState(ModuleOp module, DictionaryAttr declaration,
                                  const HardwareAnalysis &analysis,
                                  SourceCheckBudget &budget) {
  if (!module.getBody().hasOneBlock())
    return module->emitOpError()
           << "transaction state owner requires one graph block";
  InstanceOp found;
  for (auto instance : module.getBody().front().getOps<InstanceOp>()) {
    if (failed(budget.debit(1, instance)))
      return failure();
    auto actual = instance->getAttrOfType<DictionaryAttr>("occurrence");
    if (failed(payAttribute(actual, budget, instance)) ||
        failed(verifyOccurrence(actual,
                                [&] { return instance->emitOpError(); })) ||
        failed(payComparison(actual, declaration, budget, instance)))
      return failure();
    if (!identical(instance->getAttrOfType<DictionaryAttr>("occurrence"),
                   declaration))
      continue;
    if (found)
      return module->emitOpError()
             << "state declaration resolves to multiple direct instances";
    if (analysis.getPrimitiveKind(analysis.resolveCallee(instance)) != "dffe" ||
        instance->getNumOperands() != 5 || instance->getNumResults() != 1)
      return instance->emitOpError()
             << "transaction inventory requires a direct verified DFFE";
    found = instance;
  }
  if (!found)
    return module->emitOpError()
           << "transaction inventory has no matching direct DFFE";
  return found;
}
} // namespace

FailureOr<SampledAdmissionView>
parseSampledAdmission(RuleOp rule, const HardwareAnalysis &analysis,
                      const HardwareBindings &bindings,
                      SourceCheckBudget &budget) {
  auto lexical = dyn_cast<ModuleOp>(rule->getParentOp());
  if (bindings.owner != lexical)
    return rule->emitOpError()
           << "sampled admission bindings have the wrong owner";
  if (failed(charge(budget, rule, 1, sizeof(SampledAdmissionView))))
    return failure();
  auto module = dyn_cast<ModuleOp>(rule->getParentOp());
  auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
  if (!module || rule->getNumRegions() != 1 || !rule.getBody().hasOneBlock() ||
      failed(exact(row,
                   {"clock_capture", "reset_capture", "fire_result", "controls",
                    "body_checks", "observations"},
                   rule, "ac.sampled_admission")))
    return failure();
  if (failed(payAttribute(row, budget, rule)))
    return failure();
  auto clock = index(row, "clock_capture", rule);
  auto reset = index(row, "reset_capture", rule);
  auto fire = index(row, "fire_result", rule);
  auto controls = row.getAs<ArrayAttr>("controls");
  auto bodyChecks = row.getAs<ArrayAttr>("body_checks");
  auto observations = row.getAs<ArrayAttr>("observations");
  if (rule.getBody().front().empty())
    return rule->emitOpError() << "sampled Rule requires a yield terminator";
  auto yielded = dyn_cast<YieldOp>(rule.getBody().front().getTerminator());
  if (failed(clock) || failed(reset) || failed(fire))
    return failure();
  if (!controls || controls.empty() || !bodyChecks || !observations ||
      !yielded || *clock >= rule->getNumOperands() ||
      *reset >= rule->getNumOperands() || *fire >= rule->getNumResults())
    return rule->emitOpError()
           << "sampled admission captures/inventories are outside Rule layout";
  uint64_t checkBase = SourceCheckBudget::add(*fire, 1);
  for (Attribute raw : observations) {
    auto observation = dyn_cast<DictionaryAttr>(raw);
    if (failed(exact(observation, {"id", "value_count", "path"}, rule,
                     "sampled observation")))
      return failure();
    auto count = index(observation, "value_count", rule);
    if (failed(count))
      return failure();
    checkBase =
        SourceCheckBudget::add(checkBase, SourceCheckBudget::add(1, *count));
  }
  uint64_t checkCount =
      SourceCheckBudget::add(controls.size(), bodyChecks.size());
  uint64_t total = SourceCheckBudget::add(
      checkBase, SourceCheckBudget::multiply(2, checkCount));
  if (total == std::numeric_limits<uint64_t>::max() ||
      total != rule->getNumResults() || yielded->getNumOperands() != total)
    return rule->emitOpError() << "sampled admission requires exact "
                                  "observation and check suffix layout";
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  if (!required || required.size() != checkCount)
    return rule->emitOpError() << "sampled admission requires exactly its "
                                  "ordered control/assert obligations";
  if (failed(payAttribute(required, budget, rule)) ||
      failed(payAttribute(rule->getAttr("occurrence"), budget, rule)))
    return failure();
  if (failed(
          bitOne(rule->getOperand(*clock), analysis, bindings, rule, budget)) ||
      failed(
          bitOne(rule->getOperand(*reset), analysis, bindings, rule, budget)) ||
      failed(bitOne(yielded->getOperand(*fire), analysis, bindings, rule,
                    budget)) ||
      failed(verifyOccurrence(rule->getAttrOfType<DictionaryAttr>("occurrence"),
                              [&] { return rule->emitOpError(); })) ||
      failed(charge(budget, rule, controls.size(), sizeof(SampledControlView))))
    return failure();
  SampledAdmissionView result;
  result.definition = module;
  result.rule = rule;
  result.clockCapture = *clock;
  result.resetCapture = *reset;
  result.fireResult = *fire;
  result.checkBase = checkBase;
  result.totalResults = total;
  result.clock = rule->getOperand(*clock);
  result.reset = rule->getOperand(*reset);
  result.fire = yielded->getOperand(*fire);
  if (!rule->getResult(*fire).use_empty())
    return rule->emitOpError() << "sampled external fire must have zero users";
  auto parsePath = [&](DictionaryAttr row,
                       uint64_t before) -> FailureOr<SampledPathRef> {
    auto path = row.getAs<DictionaryAttr>("path");
    if (!path)
      return rule->emitOpError() << "sampled row requires a closed path";
    if (path.empty())
      return SampledPathRef{};
    if (failed(exact(path, {"selection", "when_true"}, rule, "sampled path")))
      return failure();
    auto selection = index(path, "selection", rule);
    auto arm = path.getAs<BoolAttr>("when_true");
    if (failed(selection) || !arm ||
        static_cast<uint64_t>(*selection) >= before ||
        static_cast<uint64_t>(*selection) >= result.controls.size() ||
        result.controls[*selection].kind != SampledControlKind::BranchSelect)
      return rule->emitOpError()
             << "sampled path requires an earlier branch_select and Bool arm";
    if (failed(budget.debit(1, rule)))
      return failure();
    return SampledPathRef{static_cast<uint64_t>(*selection), arm.getValue()};
  };
  auto invocationPrefix = [&](ArrayAttr prefix,
                              ArrayAttr invocation) -> FailureOr<bool> {
    if (failed(budget.debit(1, rule)))
      return failure();
    if (!prefix || !invocation || prefix.size() > invocation.size())
      return false;
    for (uint64_t i = 0; i < prefix.size(); ++i) {
      if (failed(budget.debit(1, rule)) ||
          failed(payComparison(prefix[i], invocation[i], budget, rule)))
        return failure();
      if (!identical(prefix[i], invocation[i]))
        return false;
    }
    return true;
  };
  auto validatePath = [&](SampledPathRef path, ArrayAttr invocation,
                          const SampledControlView *owner) -> LogicalResult {
    bool extendsOwner = !owner || !owner->executionPath.selectionControl;
    while (path.selectionControl) {
      if (failed(budget.debit(1, rule)))
        return failure();
      if (owner &&
          path.selectionControl == owner->executionPath.selectionControl &&
          path.whenTrue == owner->executionPath.whenTrue)
        extendsOwner = true;
      const auto &selection = result.controls[*path.selectionControl];
      auto prefix = invocationPrefix(selection.invocation, invocation);
      if (failed(prefix))
        return failure();
      if (!*prefix)
        return rule->emitOpError()
               << "sampled path references a sibling invocation";
      path = selection.executionPath;
    }
    if (!extendsOwner)
      return rule->emitOpError()
             << "sampled path escapes its callable execution path";
    return success();
  };
  auto callable = [&](ArrayAttr invocation,
                      bool parent) -> FailureOr<const SampledControlView *> {
    if (!invocation || (parent && invocation.empty()))
      return rule->emitOpError() << "sampled invocation has no callable owner";
    for (const auto &prior : result.controls) {
      if (failed(budget.debit(1, rule)))
        return failure();
      if (prior.kind != SampledControlKind::CallableGuard)
        continue;
      uint64_t requiredLength =
          parent ? SourceCheckBudget::add(prior.invocation.size(), 1)
                 : prior.invocation.size();
      if (requiredLength != invocation.size())
        continue;
      if (parent) {
        auto prefix = invocationPrefix(prior.invocation, invocation);
        if (failed(prefix))
          return failure();
        if (*prefix)
          return &prior;
      } else {
        if (failed(payComparison(prior.invocation, invocation, budget, rule)))
          return failure();
        if (identical(prior.invocation, invocation))
          return &prior;
      }
    }
    return rule->emitOpError()
           << "sampled invocation is absent from callable guard inventory";
  };
  auto validateInvocationFrame = [&](Attribute raw) -> LogicalResult {
    auto frame = dyn_cast<DictionaryAttr>(raw);
    if (failed(exact(frame, {"call", "receiver"}, rule,
                     "sampled invocation frame")) ||
        failed(verifyOccurrence(frame.getAs<DictionaryAttr>("call"),
                                [&] { return rule->emitOpError(); })) ||
        failed(verifyOwnerRef(frame.getAs<DictionaryAttr>("receiver"),
                              [&] { return rule->emitOpError(); })))
      return failure();
    return success();
  };
  result.controls.reserve(controls.size());
  for (auto [ordinal, raw] : llvm::enumerate(controls)) {
    auto guard = dyn_cast<DictionaryAttr>(raw);
    if (failed(exact(guard, {"kind", "id", "invocation", "predecessor", "path"},
                     rule, "sampled control")))
      return failure();
    auto kindAttr = guard.getAs<StringAttr>("kind");
    if (!kindAttr || !llvm::is_contained(
                         ArrayRef<StringRef>{"callable_guard", "branch_select"},
                         kindAttr.getValue()))
      return rule->emitOpError() << "sampled control has an invalid role";
    auto role = kindAttr.getValue() == "callable_guard"
                    ? SampledControlKind::CallableGuard
                    : SampledControlKind::BranchSelect;
    auto id = guard.getAs<DictionaryAttr>("id");
    auto invocation = guard.getAs<ArrayAttr>("invocation");
    auto predecessor = index(guard, "predecessor", rule, true);
    auto path = parsePath(guard, ordinal);
    if (failed(predecessor) || failed(path) ||
        failed(verifyCheckID(id, [&] { return rule->emitOpError(); })))
      return failure();
    if (failed(payComparison(id.get("registration"),
                             rule->getAttr("occurrence"), budget, rule)))
      return failure();
    auto obligation = index(id, "obligation", rule);
    if (failed(obligation))
      return failure();
    if (*predecessor != static_cast<int64_t>(ordinal) - 1 ||
        *obligation != static_cast<int64_t>(ordinal) || !invocation ||
        (ordinal == 0 && (!invocation.empty() || path->selectionControl ||
                          role != SampledControlKind::CallableGuard)) ||
        !identical(id.get("registration"), rule->getAttr("occurrence")))
      return rule->emitOpError() << "sampled control has inconsistent ordinal, "
                                    "registration or invocation";
    if (failed(charge(budget, rule, invocation.size(), sizeof(DictionaryAttr))))
      return failure();
    for (auto frameRaw : invocation)
      if (failed(validateInvocationFrame(frameRaw)))
        return failure();
    // Closed CheckIDs already have distinct obligations equal to their row
    // ordinals and the same verified registration. Only role-specific
    // invocation/site uniqueness still needs a pairwise comparison here.
    for (const auto &prior : result.controls) {
      if (failed(budget.debit(1, rule)))
        return failure();
      if (prior.kind != role || prior.invocation.size() != invocation.size())
        continue;
      if (failed(payComparison(prior.invocation, invocation, budget, rule)))
        return failure();
      bool sameInvocation = identical(prior.invocation, invocation);
      if (role == SampledControlKind::CallableGuard && sameInvocation)
        return rule->emitOpError() << "duplicate sampled callable identity";
      if (role == SampledControlKind::BranchSelect && sameInvocation) {
        if (failed(payComparison(prior.id.get("check"), id.get("check"), budget,
                                 rule)))
          return failure();
        if (identical(prior.id.get("check"), id.get("check")))
          return rule->emitOpError() << "duplicate sampled selection site";
      }
    }
    const SampledControlView *owner = nullptr;
    if (ordinal) {
      auto found =
          callable(invocation, role == SampledControlKind::CallableGuard);
      if (failed(found))
        return failure();
      owner = *found;
    }
    if (failed(validatePath(*path, invocation, owner)))
      return failure();
    auto check = dyn_cast<DictionaryAttr>(required[ordinal]);
    if (failed(exact(check, {"id", "kind", "location"}, rule,
                     "control RequiredCheck")))
      return failure();
    if (failed(payComparison(check.get("id"), id, budget, rule)))
      return failure();
    auto kind = check.getAs<StringAttr>("kind");
    if (!kind || kind.getValue() != "guard_known" ||
        !identical(check.get("id"), id) ||
        failed(verifySourceSpan(check.getAs<DictionaryAttr>("location"),
                                [&] { return rule->emitOpError(); })))
      return rule->emitOpError() << "sampled control differs from its ordered "
                                    "guard_known obligation";
    uint64_t predicate = checkBase + 2 * ordinal, reached = predicate + 1;
    if (failed(bitOne(yielded->getOperand(predicate), analysis, bindings, rule,
                      budget)) ||
        failed(bitOne(yielded->getOperand(reached), analysis, bindings, rule,
                      budget)))
      return failure();
    result.controls.push_back(
        {role, id, invocation, *predecessor, *path, predicate, reached,
         yielded->getOperand(predicate), yielded->getOperand(reached)});
  }
  auto effectPath = [&](ArrayAttr invocation,
                        SampledPathRef path) -> LogicalResult {
    auto owner = callable(invocation, false);
    if (failed(owner))
      return failure();
    return validatePath(path, invocation, *owner);
  };
  if (failed(charge(budget, rule, bodyChecks.size(),
                    sizeof(SampledBodyCheckView))) ||
      failed(charge(budget, rule, observations.size(),
                    sizeof(SampledObservationView))))
    return failure();
  result.bodyChecks.reserve(bodyChecks.size());
  result.observations.reserve(observations.size());
  for (auto [ordinal, raw] : llvm::enumerate(bodyChecks)) {
    auto body = dyn_cast<DictionaryAttr>(raw);
    if (failed(exact(body, {"id", "invocation", "path"}, rule,
                     "sampled body check")))
      return failure();
    auto id = body.getAs<DictionaryAttr>("id");
    auto invocation = body.getAs<ArrayAttr>("invocation");
    auto executionPath = parsePath(body, result.controls.size());
    if (failed(executionPath) ||
        failed(verifyCheckID(id, [&] { return rule->emitOpError(); })) ||
        !invocation || failed(payAttribute(invocation, budget, rule)))
      return failure();
    for (Attribute frame : invocation)
      if (failed(budget.debit(1, rule)) ||
          failed(validateInvocationFrame(frame)))
        return failure();
    if (failed(effectPath(invocation, *executionPath)))
      return failure();
    if (failed(payComparison(id.get("registration"),
                             rule->getAttr("occurrence"), budget, rule)))
      return failure();
    uint64_t obligation = controls.size() + ordinal;
    auto actual = index(id, "obligation", rule);
    auto record = dyn_cast<DictionaryAttr>(required[obligation]);
    if (failed(actual) || *actual != static_cast<int64_t>(obligation) ||
        !identical(id.get("registration"), rule->getAttr("occurrence")) ||
        failed(exact(record, {"id", "kind", "location"}, rule,
                     "assert RequiredCheck")))
      return rule->emitOpError()
             << "sampled body assertion has inconsistent identity/obligation";
    if (failed(payComparison(record.get("id"), id, budget, rule)))
      return failure();
    auto kind = record.getAs<StringAttr>("kind");
    if (!kind || kind.getValue() != "assert" ||
        !identical(record.get("id"), id) ||
        failed(verifySourceSpan(record.getAs<DictionaryAttr>("location"),
                                [&] { return rule->emitOpError(); })))
      return rule->emitOpError()
             << "sampled body assertion differs from its ordered obligation";
    uint64_t condition = checkBase + 2 * obligation, path = condition + 1;
    if (failed(bitOne(yielded->getOperand(condition), analysis, bindings, rule,
                      budget)) ||
        failed(bitOne(yielded->getOperand(path), analysis, bindings, rule,
                      budget)))
      return failure();
    result.bodyChecks.push_back({*executionPath, id, invocation, condition,
                                 path, yielded->getOperand(condition),
                                 yielded->getOperand(path)});
  }
  uint64_t path = *fire + 1;
  for (auto [ordinal, raw] : llvm::enumerate(observations)) {
    auto row = cast<DictionaryAttr>(raw);
    auto id = row.getAs<DictionaryAttr>("id");
    auto count = index(row, "value_count", rule);
    if (failed(count) ||
        failed(verifyObservationID(id, [&] { return rule->emitOpError(); })))
      return failure();
    auto invocation = id.getAs<ArrayAttr>("invocation");
    if (failed(payAttribute(invocation, budget, rule)))
      return failure();
    auto executionPath = parsePath(row, result.controls.size());
    if (failed(executionPath))
      return failure();
    if (failed(payComparison(id.get("registration"),
                             rule->getAttr("occurrence"), budget, rule)))
      return failure();
    auto actual = index(id, "ordinal", rule);
    if (failed(actual) || *actual != static_cast<int64_t>(ordinal) ||
        !identical(id.get("registration"), rule->getAttr("occurrence")) ||
        failed(effectPath(invocation, *executionPath)))
      return rule->emitOpError() << "sampled observation has inconsistent "
                                    "ordinal/registration/invocation";
    if (failed(bitOne(yielded->getOperand(path), analysis, bindings, rule,
                      budget)) ||
        failed(charge(budget, rule, *count, sizeof(Value))))
      return failure();
    SampledObservationView observation{*executionPath,
                                       id,
                                       path,
                                       path + 1,
                                       static_cast<uint64_t>(*count),
                                       yielded->getOperand(path),
                                       {}};
    observation.values.reserve(*count);
    for (uint64_t k = 0; k < static_cast<uint64_t>(*count); ++k) {
      Value value = yielded->getOperand(path + 1 + k);
      auto type = analysis.resolveType(value.getType(), bindings, rule);
      if (failed(budget.debit(1, rule)) || failed(type))
        return failure();
      if (!isa<BitsType, EnumType>(*type))
        return rule->emitOpError()
               << "sampled observation requires existing scalar payloads";
      auto width = analysis.getPackedWidth(*type, bindings, rule);
      if (failed(width))
        return failure();
      if (*width > 64)
        return rule->emitOpError()
               << "sampled observation exceeds 64-bit scalar slot limit";
      observation.values.push_back(value);
    }
    result.observations.push_back(std::move(observation));
    path += 1 + *count;
  }
  return result;
}

FailureOr<TransactionWiringView>
parseTransactionWiring(ModuleOp module, const HardwareAnalysis &analysis,
                       const HardwareBindings &bindings,
                       SourceCheckBudget &budget) {
  if (bindings.owner != module)
    return module->emitOpError()
           << "transaction wiring bindings have the wrong owner";
  if (failed(charge(budget, module, 1, sizeof(TransactionWiringView))))
    return failure();
  auto row = module->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
  if (!module.getBody().hasOneBlock() ||
      failed(exact(row, {"states", "incoming", "writers"}, module,
                   "ac.transaction_wiring")))
    return failure();
  if (failed(payAttribute(row, budget, module)))
    return failure();
  auto states = row.getAs<ArrayAttr>("states");
  auto incoming = row.getAs<ArrayAttr>("incoming");
  auto writers = row.getAs<ArrayAttr>("writers");
  if (!states || !incoming || !writers)
    return module->emitOpError()
           << "transaction wiring requires array inventories";
  auto ports = getModulePortLayout(module);
  if (failed(ports))
    return failure();
  if (SourceCheckBudget::add(ports->sourceInputCount,
                             SourceCheckBudget::multiply(2, incoming.size())) !=
      module.getFunctionType().getNumInputs())
    return module->emitOpError()
           << "transaction incoming must exactly cover appended D/E inputs";
  if (failed(charge(budget, module, states.size(),
                    sizeof(TransactionStateInventoryEntry))) ||
      failed(charge(budget, module, incoming.size(),
                    sizeof(TransactionIncomingView))) ||
      failed(charge(budget, module, writers.size(),
                    sizeof(TransactionWriterView))))
    return failure();
  TransactionWiringView result;
  result.definition = module;
  result.ports = *ports;
  if (states.size() > result.states.max_size() ||
      incoming.size() > result.incoming.max_size() ||
      writers.size() > result.writers.max_size())
    return budget.debit(std::numeric_limits<uint64_t>::max(), module);
  result.states.reserve(states.size());
  result.incoming.reserve(incoming.size());
  result.writers.reserve(writers.size());
  for (auto raw : states) {
    auto declaration = dyn_cast<DictionaryAttr>(raw);
    if (failed(verifyOccurrence(declaration,
                                [&] { return module->emitOpError(); })))
      return failure();
    for (const auto &prior : result.states) {
      if (failed(payComparison(prior.declaration, declaration, budget, module)))
        return failure();
      if (identical(prior.declaration, declaration))
        return module->emitOpError() << "duplicate transaction state inventory";
    }
    auto storage = directState(module, declaration, analysis, budget);
    if (failed(storage))
      return failure();
    auto type =
        analysis.resolveType(storage->getResult(0).getType(), bindings, module);
    if (failed(type))
      return failure();
    if (!isa<BitsType>(*type))
      return module->emitOpError()
             << "transaction inventory requires whole Bits payloads";
    result.states.push_back({declaration, *storage});
  }
  auto &body = module.getBody().front();
  for (auto [ordinal, raw] : llvm::enumerate(incoming)) {
    auto entry = dyn_cast<DictionaryAttr>(raw);
    if (failed(exact(entry, {"state", "data", "enable"}, module,
                     "transaction incoming")))
      return failure();
    auto state = entry.getAs<DictionaryAttr>("state");
    auto data = index(entry, "data", module);
    auto enable = index(entry, "enable", module);
    if (failed(data) || failed(enable) || failed(wholeStateID(state, module)))
      return failure();
    uint64_t expected = SourceCheckBudget::add(
        ports->sourceInputCount, SourceCheckBudget::multiply(2, ordinal));
    if (expected == std::numeric_limits<uint64_t>::max() ||
        static_cast<uint64_t>(*data) != expected ||
        static_cast<uint64_t>(*enable) != SourceCheckBudget::add(expected, 1) ||
        static_cast<uint64_t>(*enable) >= body.getNumArguments())
      return module->emitOpError()
             << "transaction incoming D/E must occupy its exact appended pair";
    for (const auto &prior : result.incoming) {
      if (failed(payComparison(prior.stateID, state, budget, module)))
        return failure();
      if (identical(prior.stateID, state))
        return module->emitOpError() << "duplicate transaction incoming owner";
    }
    if (failed(bitOne(body.getArgument(*enable), analysis, bindings, module,
                      budget)))
      return failure();
    result.incoming.push_back(
        {state, static_cast<uint64_t>(*data), static_cast<uint64_t>(*enable),
         body.getArgument(*data), body.getArgument(*enable)});
  }
  for (auto raw : writers) {
    auto entry = dyn_cast<DictionaryAttr>(raw);
    if (failed(exact(entry, {"registration", "ordinary_results", "candidates"},
                     module, "transaction writer")))
      return failure();
    auto registration = entry.getAs<DictionaryAttr>("registration");
    auto ordinary = index(entry, "ordinary_results", module);
    auto candidates = entry.getAs<ArrayAttr>("candidates");
    if (failed(ordinary) || !candidates ||
        failed(verifyOccurrence(registration,
                                [&] { return module->emitOpError(); })))
      return failure();
    RuleOp rule;
    for (auto possible : body.getOps<RuleOp>()) {
      if (failed(budget.debit(1, possible)))
        return failure();
      auto actual = possible->getAttrOfType<DictionaryAttr>("occurrence");
      if (failed(payAttribute(actual, budget, possible)) ||
          failed(verifyOccurrence(actual,
                                  [&] { return possible->emitOpError(); })) ||
          failed(payComparison(actual, registration, budget, possible)))
        return failure();
      if (!identical(possible->getAttrOfType<DictionaryAttr>("occurrence"),
                     registration))
        continue;
      if (rule)
        return module->emitOpError()
               << "transaction registration resolves to multiple Rules";
      rule = possible;
    }
    if (!rule || !rule->hasAttr("ac.sampled_admission"))
      return module->emitOpError()
             << "transaction writer must resolve to a sampled Rule";
    for (const auto &prior : result.writers) {
      if (failed(budget.debit(1, module)))
        return failure();
      if (prior.rule == rule)
        return module->emitOpError()
               << "duplicate transaction writer registration";
    }
    auto sampled = parseSampledAdmission(rule, analysis, bindings, budget);
    if (failed(sampled))
      return failure();
    if (SourceCheckBudget::add(
            *ordinary, SourceCheckBudget::multiply(2, candidates.size())) !=
        sampled->fireResult)
      return rule->emitOpError() << "transaction candidate slots must precede "
                                    "fire in exact result layout";
    if (failed(charge(budget, rule, candidates.size(),
                      sizeof(TransactionCandidateView))))
      return failure();
    TransactionWriterView writer;
    writer.registration = registration;
    writer.rule = rule;
    writer.ordinaryResults = *ordinary;
    if (candidates.size() > writer.candidates.max_size())
      return budget.debit(std::numeric_limits<uint64_t>::max(), rule);
    writer.candidates.reserve(candidates.size());
    for (auto [ordinal, candidateRaw] : llvm::enumerate(candidates)) {
      auto candidate = dyn_cast<DictionaryAttr>(candidateRaw);
      if (failed(exact(candidate, {"state", "data", "enable"}, rule,
                       "transaction candidate")))
        return failure();
      auto state = candidate.getAs<DictionaryAttr>("state");
      auto data = index(candidate, "data", rule);
      auto enable = index(candidate, "enable", rule);
      if (failed(data) || failed(enable) || failed(wholeStateID(state, rule)))
        return failure();
      uint64_t expected = SourceCheckBudget::add(
          *ordinary, SourceCheckBudget::multiply(2, ordinal));
      if (expected == std::numeric_limits<uint64_t>::max() ||
          static_cast<uint64_t>(*data) != expected ||
          static_cast<uint64_t>(*enable) !=
              SourceCheckBudget::add(expected, 1) ||
          static_cast<uint64_t>(*enable) >= sampled->fireResult)
        return rule->emitOpError()
               << "candidate D/E must cover its exact continuous pair";
      for (const auto &prior : writer.candidates) {
        if (failed(payComparison(prior.stateID, state, budget, rule)))
          return failure();
        if (identical(prior.stateID, state))
          return rule->emitOpError()
                 << "duplicate candidate owner within transaction";
      }
      if (failed(bitOne(rule->getResult(*enable), analysis, bindings, rule,
                        budget)))
        return failure();
      writer.candidates.push_back({state, static_cast<uint64_t>(*data),
                                   static_cast<uint64_t>(*enable),
                                   cast<OpResult>(rule->getResult(*data)),
                                   cast<OpResult>(rule->getResult(*enable))});
    }
    result.writers.push_back(std::move(writer));
  }
  for (auto rule : body.getOps<RuleOp>())
    if (rule->hasAttr("ac.sampled_admission") &&
        !llvm::any_of(result.writers,
                      [&](const auto &writer) { return writer.rule == rule; }))
      return rule->emitOpError()
             << "sampled Rule is missing its transaction writer row";
  return result;
}

FailureOr<PhysicalStateEndpoint>
resolveTransactionState(ModuleOp relativeTo, DictionaryAttr stateID,
                        const HardwareAnalysis &analysis,
                        const HardwareBindings &bindings,
                        SourceCheckBudget &budget) {
  if (!relativeTo)
    return analysis.getPackage().emitOpError()
           << "StateID requires a relative module owner";
  if (!relativeTo.getBody().hasOneBlock())
    return relativeTo->emitOpError()
           << "StateID relative owner requires one graph block";
  if (bindings.owner != relativeTo)
    return relativeTo->emitOpError()
           << "StateID bindings have the wrong relative owner";
  if (failed(payAttribute(stateID, budget, relativeTo)) ||
      failed(wholeStateID(stateID, relativeTo)) ||
      failed(charge(budget, relativeTo, 1, sizeof(PhysicalStateEndpoint))) ||
      failed(budget.debit(SourceCheckBudget::bindings(bindings), relativeTo)))
    return failure();
  PhysicalStateEndpoint result;
  result.ownerScope.root = relativeTo;
  result.ownerScope.rootBindings = bindings;
  result.ownerScope.rootBindings.owner = relativeTo;
  HardwareBindings bound = result.ownerScope.rootBindings;
  ModuleOp owner = relativeTo;
  auto path =
      stateID.getAs<DictionaryAttr>("owner").getAs<ArrayAttr>("instance_path");
  for (auto raw : path) {
    auto occurrence = cast<DictionaryAttr>(raw);
    InstanceOp allocation;
    for (auto instance : owner.getBody().front().getOps<InstanceOp>()) {
      if (failed(budget.debit(1, instance)))
        return failure();
      auto actual = instance->getAttrOfType<DictionaryAttr>("occurrence");
      if (failed(payAttribute(actual, budget, instance)) ||
          failed(verifyOccurrence(actual,
                                  [&] { return instance->emitOpError(); })) ||
          failed(payComparison(actual, occurrence, budget, instance)))
        return failure();
      if (!identical(instance->getAttrOfType<DictionaryAttr>("occurrence"),
                     occurrence))
        continue;
      if (allocation)
        return owner->emitOpError()
               << "StateID owner occurrence resolves ambiguously";
      allocation = instance;
    }
    auto child =
        allocation
            ? dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(allocation))
            : ModuleOp();
    if (!child || !child.getBody().hasOneBlock())
      return owner->emitOpError()
             << "StateID owner path must resolve actual ordinary instances";
    if (failed(charge(budget, allocation, 1, sizeof(InstanceOp))) ||
        failed(budget.debit(SourceCheckBudget::bindings(bound), allocation)))
      return failure();
    auto next = analysis.bindInstance(allocation, bound);
    if (failed(next))
      return failure();
    result.ownerScope.path.push_back(allocation);
    bound = std::move(*next);
    owner = child;
  }
  auto declaration = stateID.getAs<DictionaryAttr>("declaration");
  auto inventory =
      owner->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
  auto states = inventory ? inventory.getAs<ArrayAttr>("states") : ArrayAttr();
  uint64_t matching = 0;
  if (states)
    for (Attribute raw : states) {
      if (failed(payAttribute(raw, budget, owner)) ||
          failed(verifyOccurrence(dyn_cast<DictionaryAttr>(raw),
                                  [&] { return owner->emitOpError(); })) ||
          failed(payComparison(raw, declaration, budget, owner)))
        return failure();
      matching += identical(raw, declaration);
    }
  if (!states || matching != 1)
    return owner->emitOpError()
           << "StateID terminal owner is missing its matching inventory";
  auto storage = directState(owner, declaration, analysis, budget);
  if (failed(storage) ||
      failed(budget.debit(SourceCheckBudget::bindings(bound), owner)))
    return failure();
  auto leaf = analysis.bindInstance(*storage, bound);
  auto payload =
      analysis.resolveType(storage->getResult(0).getType(), bound, owner);
  if (failed(leaf) || failed(payload))
    return failure();
  if (!isa<BitsType>(*payload))
    return owner->emitOpError()
           << "StateID terminal payload must be whole Bits";
  result.owner = owner;
  result.storage = *storage;
  result.ownerBindings = std::move(bound);
  result.storageBindings = std::move(*leaf);
  result.payloadType = *payload;
  result.clock = storage->getOperand(0);
  result.reset = storage->getOperand(1);
  result.enable = storage->getOperand(2);
  result.data = storage->getOperand(3);
  result.initial = storage->getOperand(4);
  result.q = storage->getResult(0);
  return result;
}

FailureOr<HardwareIdentityEndpoint>
makeIdentityEndpoint(Value value, const HardwareIdentityScope &scope,
                     const HardwareAnalysis &analysis,
                     SourceCheckBudget &budget) {
  Operation *site =
      value ? valueSite(value) : analysis.getPackage().getOperation();
  if (!value ||
      failed(charge(budget, site, 1, sizeof(HardwareIdentityEndpoint))))
    return failure();
  auto bound = scopeBindings(scope, analysis, budget, site);
  if (failed(bound))
    return failure();
  if (valueModule(value) != scopeOwner(scope, analysis))
    return site->emitOpError()
           << "identity SSA value does not belong to its actual owner scope";
  if (failed(budget.debit(1, site)))
    return failure();
  auto type = analysis.resolveType(value.getType(), *bound, site);
  auto copied = copyScope(scope, budget, site);
  if (failed(type) || failed(copied))
    return failure();
  return HardwareIdentityEndpoint{value, std::move(*copied), *type};
}

FailureOr<HardwareIdentityEndpoint>
normalizeIdentityEndpoint(const HardwareIdentityEndpoint &endpoint,
                          const HardwareIdentityStopPolicy &stops,
                          const HardwareAnalysis &analysis,
                          SourceCheckBudget &budget) {
  auto checked =
      makeIdentityEndpoint(endpoint.value, endpoint.scope, analysis, budget);
  if (failed(checked))
    return failure();
  if (!areEquivalentHardwareTypes(checked->resolvedType, endpoint.resolvedType))
    return valueSite(endpoint.value)->emitOpError()
           << "stale identity endpoint resolved type";
  SmallVector<HardwareIdentityEndpoint> seen;
  HardwareIdentityEndpoint current = std::move(*checked);
  for (;;) {
    Operation *site = valueSite(current.value);
    if (failed(budget.debit(SourceCheckBudget::add(1, stops.boundaries.size()),
                            site)))
      return failure();
    for (const auto &stop : stops.boundaries) {
      if (stop.value != current.value)
        continue;
      if (failed(payScope(stop.scope, budget, site)) ||
          failed(payScope(current.scope, budget, site)))
        return failure();
      if (sameEndpoint(stop, current))
        return current;
    }
    for (const auto &prior : seen) {
      if (failed(budget.debit(1, site)))
        return failure();
      if (prior.value != current.value)
        continue;
      if (failed(payScope(prior.scope, budget, site)) ||
          failed(payScope(current.scope, budget, site)))
        return failure();
      if (sameEndpoint(prior, current))
        return site->emitOpError() << "cyclic sampled hardware identity route";
    }
    if (failed(charge(budget, site, 1, sizeof(HardwareIdentityEndpoint))) ||
        failed(payScope(current.scope, budget, site)))
      return failure();
    seen.push_back(current);
    auto bound = scopeBindings(current.scope, analysis, budget, site);
    if (failed(bound))
      return failure();
    Value next;
    auto nextScope = copyScope(current.scope, budget, site);
    if (failed(nextScope))
      return failure();
    if (auto extract = current.value.getDefiningOp<BitsExtractOp>()) {
      if (failed(analysis.verifyResolvedOperation(extract, *bound)))
        return failure();
      auto low = analysis.evaluateStatic(extract.getLow(), *bound, extract);
      auto input = analysis.getPackedWidth(extract.getInput().getType(), *bound,
                                           extract);
      auto output =
          analysis.getPackedWidth(current.resolvedType, *bound, extract);
      if (failed(low) || failed(input) || failed(output))
        return failure();
      auto integer = dyn_cast<MathIntAttr>(*low);
      if (integer && llvm::APSInt(integer.getCanonicalValue()).isZero() &&
          *input == *output)
        next = extract.getInput();
    } else if (auto argument = dyn_cast<BlockArgument>(current.value)) {
      if (auto rule = dyn_cast<RuleOp>(argument.getOwner()->getParentOp())) {
        if (argument.getArgNumber() >= rule->getNumOperands())
          return rule->emitOpError()
                 << "identity Rule capture ordinal is out of bounds";
        next = rule->getOperand(argument.getArgNumber());
      } else if (stops.traversal ==
                     HardwareIdentityTraversal::OrdinaryInstances &&
                 isa<ModuleOp>(argument.getOwner()->getParentOp()) &&
                 !nextScope->path.empty()) {
        auto instance = nextScope->path.back();
        if (argument.getArgNumber() >= instance->getNumOperands())
          return instance->emitOpError()
                 << "identity actual input ordinal is out of bounds";
        next = instance->getOperand(argument.getArgNumber());
        nextScope->path.pop_back();
      }
    } else if (auto result = dyn_cast<OpResult>(current.value)) {
      if (auto rule = dyn_cast<RuleOp>(result.getOwner())) {
        if (!rule.getBody().hasOneBlock() || rule.getBody().front().empty())
          return rule->emitOpError()
                 << "identity Rule requires one graph block and yield";
        auto yielded =
            dyn_cast<YieldOp>(rule.getBody().front().getTerminator());
        if (!yielded || result.getResultNumber() >= yielded->getNumOperands())
          return rule->emitOpError()
                 << "identity Rule result ordinal is out of bounds";
        next = yielded->getOperand(result.getResultNumber());
      } else if (stops.traversal ==
                 HardwareIdentityTraversal::OrdinaryInstances) {
        if (auto instance = dyn_cast<InstanceOp>(result.getOwner())) {
          auto child =
              dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
          if (child && analysis.getPrimitiveKind(child).empty()) {
            if (!child.getBody().hasOneBlock() ||
                child.getBody().front().empty())
              return instance->emitOpError()
                     << "identity callee requires one graph block and yield";
            auto yielded =
                dyn_cast<YieldOp>(child.getBody().front().getTerminator());
            if (!yielded ||
                result.getResultNumber() >= yielded->getNumOperands())
              return instance->emitOpError()
                     << "identity Module result ordinal is out of bounds";
            auto nested = childScope(current.scope, instance, budget);
            if (failed(nested))
              return failure();
            nextScope = std::move(nested);
            next = yielded->getOperand(result.getResultNumber());
          }
        }
      }
    }
    if (!next)
      return current;
    auto resolved = makeIdentityEndpoint(next, *nextScope, analysis, budget);
    if (failed(resolved))
      return failure();
    if (!areEquivalentHardwareTypes(current.resolvedType,
                                    resolved->resolvedType))
      return site->emitOpError()
             << "hardware identity route changes resolved payload type";
    current = std::move(*resolved);
  }
}

namespace {
struct IdentityMemoEntry {
  HardwareIdentityEndpoint input, output;
};
class IdentityMemo {
public:
  IdentityMemo(const HardwareAnalysis &analysis, SourceCheckBudget &budget,
               HardwareIdentityStopPolicy policy)
      : analysis(analysis), budget(budget), policy(policy) {}
  FailureOr<HardwareIdentityEndpoint>
  normalize(Value value, const HardwareIdentityScope &scope) {
    if (!value)
      return failure();
    Operation *site = valueSite(value);
    for (const auto &entry : cache) {
      if (failed(budget.debit(1, site)))
        return failure();
      if (entry.input.value != value)
        continue;
      if (failed(payScope(scope, budget, site)) ||
          failed(payScope(entry.input.scope, budget, site)))
        return failure();
      // Immutable IR and this memo's fixed complete stop policy make the same
      // SSA value in the full same scope resolve to the same payload type.
      if (!sameScope(entry.input.scope, scope))
        continue;
      if (failed(charge(budget, site, 1, sizeof(HardwareIdentityEndpoint))) ||
          failed(payScope(entry.output.scope, budget, site)))
        return failure();
      return entry.output;
    }
    auto input = makeIdentityEndpoint(value, scope, analysis, budget);
    if (failed(input))
      return failure();
    auto output = normalizeIdentityEndpoint(*input, policy, analysis, budget);
    if (failed(output) ||
        failed(
            charge(budget, valueSite(value), 1, sizeof(IdentityMemoEntry))) ||
        failed(payScope(input->scope, budget, valueSite(value))) ||
        failed(payScope(output->scope, budget, valueSite(value))))
      return failure();
    cache.push_back({std::move(*input), *output});
    return std::move(*output);
  }

private:
  const HardwareAnalysis &analysis;
  SourceCheckBudget &budget;
  // Immutable for this object's lifetime: traversal and complete stop contents
  // are part of every entry's implicit key, independent of ArrayRef address.
  HardwareIdentityStopPolicy policy;
  SmallVector<IdentityMemoEntry, 0> cache;
};
enum class KnownTest : uint8_t { Zero, One, Either };
struct KnownAtom {
  KnownTest test;
  HardwareIdentityEndpoint endpoint;
};
struct Prefix {
  bool impossible = false;
  SmallVector<uint64_t, 0> atoms;
};
enum class PrefixMode : uint8_t { Prefix, SelectionPath, Acceptance };
struct PrefixCache {
  HardwareIdentityEndpoint endpoint;
  PrefixMode mode;
  Prefix prefix;
  bool complete = false;
};
struct ConditionalAtom {
  Prefix selectionPath, acceptance;
};
struct PrefixParseFrame {
  PrefixMode mode;
  HardwareIdentityEndpoint input;
  uint64_t memoIndex;
  unsigned stage = 0;
  std::optional<Prefix> intermediate;
};
struct ExpectedSelectionPaths {
  uint64_t control;
  Prefix whenTrue, whenFalse;
};
// All three grammar modes share one immutable identity context. Only composite
// nodes create DFS frames; raw operands and direct Known-OR children never do.
class OrderedPrefixProof {
public:
  OrderedPrefixProof(const HardwareAnalysis &analysis,
                     SourceCheckBudget &budget,
                     HardwareIdentityStopPolicy policy,
                     const HardwareIdentityScope &scope)
      : analysis(analysis), budget(budget), fixedScope(&scope),
        identity(analysis, budget, policy) {}
  FailureOr<Prefix> parse(Value value,
                          PrefixMode mode = PrefixMode::Prefix) {
    SmallVector<PrefixParseFrame, 0> frames;
    std::optional<Prefix> completed;
    auto finish = [&](uint64_t position, Prefix result) -> LogicalResult {
      auto saved = copy(result, valueSite(cache[position].endpoint.value));
      if (failed(saved))
        return failure();
      cache[position].prefix = std::move(*saved);
      cache[position].complete = true;
      completed = std::move(result);
      return success();
    };
    auto start = [&](Value input, PrefixMode inputMode) -> LogicalResult {
      auto endpoint = identity.normalize(input, *fixedScope);
      if (failed(endpoint))
        return failure();
      Operation *site = valueSite(input);
      for (auto [i, entry] : llvm::enumerate(cache)) {
        if (failed(budget.debit(1, site)))
          return failure();
        if (entry.mode != inputMode || entry.endpoint.value != endpoint->value)
          continue;
        if (failed(payScope(entry.endpoint.scope, budget, site)) ||
            failed(payScope(endpoint->scope, budget, site)))
          return failure();
        if (!sameEndpoint(entry.endpoint, *endpoint))
          return site->emitOpError()
                 << "sampled prefix cache differs from its fixed scope or type";
        if (!entry.complete)
          return site->emitOpError() << "cyclic sampled admission prefix";
        auto result = copy(entry.prefix, site);
        if (failed(result))
          return failure();
        completed = std::move(*result);
        return success();
      }
      auto bound = scopeBindings(endpoint->scope, analysis, budget, site);
      if (failed(bound) ||
          failed(bitOne(endpoint->value, analysis, *bound, site, budget)) ||
          failed(grow(cache, site)) || failed(budget.debit(1, site)) ||
          failed(payScope(endpoint->scope, budget, site)))
        return failure();
      uint64_t position = cache.size();
      cache.push_back({*endpoint, inputMode, {}, false});
      auto known = literal(*endpoint, analysis, budget);
      if (failed(known))
        return failure();
      if (known->has_value()) {
        Prefix result;
        result.impossible = !**known;
        return finish(position, std::move(result));
      }
      Operation *operation = endpoint->value.getDefiningOp();
      auto binary = dyn_cast_or_null<BitsBinaryOp>(operation);
      auto compare = dyn_cast_or_null<BitsCompareOp>(operation);
      auto select = dyn_cast_or_null<BitsSelectOp>(operation);
      if (!operation)
        return site->emitOpError()
               << "sampled prefix cannot use an unproved raw operand";
      if (failed(analysis.verifyResolvedOperation(operation, *bound)))
        return failure();
      if (compare && compare.getPredicate() == "case_eq") {
        auto lhs = identity.normalize(compare.getLhs(), *fixedScope);
        auto rhs = identity.normalize(compare.getRhs(), *fixedScope);
        if (failed(lhs) || failed(rhs))
          return failure();
        auto left = literal(*lhs, analysis, budget);
        auto right = literal(*rhs, analysis, budget);
        if (failed(left) || failed(right))
          return failure();
        const HardwareIdentityEndpoint *raw = nullptr;
        std::optional<bool> test;
        if (left->has_value() &&
            (inputMode != PrefixMode::Acceptance || **left)) {
          raw = &*rhs;
          test = **left;
        } else if (right->has_value() &&
                   (inputMode != PrefixMode::Acceptance || **right)) {
          raw = &*lhs;
          test = **right;
        }
        if (!raw || (inputMode == PrefixMode::Acceptance && !*test))
          return compare->emitOpError()
                 << "sampled comparison requires its admitted zero/one test";
        auto result =
            knownValue(*raw, *test ? KnownTest::One : KnownTest::Zero, compare);
        if (failed(result))
          return failure();
        return finish(position, std::move(*result));
      }
      if (binary && binary.getOpcode() == "or" &&
          inputMode != PrefixMode::SelectionPath) {
        auto result = knownEither(binary, *bound);
        if (failed(result))
          return failure();
        return finish(position, std::move(*result));
      }
      bool isAnd = binary && binary.getOpcode() == "and" &&
                   inputMode != PrefixMode::Acceptance;
      bool isSelect = select && inputMode == PrefixMode::Prefix;
      if (!isAnd && !isSelect)
        return operation->emitOpError()
               << "sampled control is outside its admitted prefix grammar";
      if (isSelect) {
        auto fallback =
            identity.normalize(select.getFalseValue(), *fixedScope);
        if (failed(fallback))
          return failure();
        auto one = literal(*fallback, analysis, budget);
        if (failed(one) || failed(budget.debit(1, site)))
          return failure();
        if (!one->has_value() || !**one)
          return select->emitOpError()
                 << "sampled bypass requires literal one fallback";
      }
      if (failed(grow(frames, site)) || failed(budget.debit(1, site)) ||
          failed(payScope(endpoint->scope, budget, site)))
        return failure();
      frames.push_back(
          {inputMode, std::move(*endpoint), position, 0, std::nullopt});
      return success();
    };
    if (failed(start(value, mode)))
      return failure();
    while (!frames.empty()) {
      // Copy only the stable scalar scheduling data before a possible growth.
      auto &frame = frames.back();
      Operation *site = valueSite(frame.input.value);
      if (failed(budget.debit(1, site)))
        return failure();
      auto binary = frame.input.value.getDefiningOp<BitsBinaryOp>();
      auto select = frame.input.value.getDefiningOp<BitsSelectOp>();
      if (frame.stage == 0) {
        frame.stage = 1;
        Value child = select ? select.getCondition() : binary.getLhs();
        PrefixMode childMode = select ? PrefixMode::SelectionPath : frame.mode;
        if (failed(start(child, childMode)))
          return failure();
      } else if (frame.stage == 1) {
        if (!completed)
          return site->emitOpError() << "incomplete sampled prefix child";
        frame.intermediate = std::move(*completed);
        completed.reset();
        frame.stage = 2;
        Value child = select ? select.getTrueValue() : binary.getRhs();
        PrefixMode childMode = select ? PrefixMode::Acceptance : frame.mode;
        if (failed(start(child, childMode)))
          return failure();
      } else {
        if (!completed || !frame.intermediate)
          return site->emitOpError() << "incomplete sampled prefix children";
        auto result = select
                          ? conditional(*frame.intermediate, *completed, site)
                          : conjunction(*frame.intermediate, *completed, site);
        if (failed(result) || failed(budget.debit(1, site)))
          return failure();
        uint64_t position = frame.memoIndex;
        frames.pop_back();
        completed.reset();
        if (failed(finish(position, std::move(*result))))
          return failure();
      }
    }
    if (!completed)
      return failure();
    return std::move(*completed);
  }
  FailureOr<HardwareIdentityEndpoint>
  normalize(Value value) {
    return identity.normalize(value, *fixedScope);
  }
  FailureOr<Prefix> knownValue(const HardwareIdentityEndpoint &raw,
                               KnownTest test, Operation *site) {
    auto bindings = scopeBindings(raw.scope, analysis, budget, site);
    if (failed(bindings) ||
        failed(bitOne(raw.value, analysis, *bindings, site, budget)))
      return failure();
    auto known = literal(raw, analysis, budget);
    if (failed(known))
      return failure();
    Prefix result;
    if (known->has_value())
      result.impossible =
          test != KnownTest::Either && (**known != (test == KnownTest::One));
    else {
      uint64_t index = knownAtoms.size();
      for (auto [i, prior] : llvm::enumerate(knownAtoms)) {
        if (failed(budget.debit(1, site)) ||
            failed(payScope(prior.endpoint.scope, budget, site)) ||
            failed(payScope(raw.scope, budget, site)))
          return failure();
        if (prior.test == test && sameEndpoint(prior.endpoint, raw)) {
          index = i;
          break;
        }
      }
      uint64_t id = SourceCheckBudget::multiply(index, 2);
      if (id == std::numeric_limits<uint64_t>::max())
        return budget.debit(id, site);
      if (index == knownAtoms.size()) {
        if (failed(grow(knownAtoms, site)) || failed(budget.debit(1, site)) ||
            failed(payScope(raw.scope, budget, site)))
          return failure();
        knownAtoms.push_back({test, raw});
      }
      if (failed(reserveAtoms(result, 1, site)) ||
          failed(budget.debit(1, site)))
        return failure();
      result.atoms.push_back(id);
    }
    return result;
  }
  FailureOr<Prefix> conjunction(const Prefix &lhs, const Prefix &rhs,
                                Operation *site) {
    Prefix result;
    result.impossible = lhs.impossible || rhs.impossible;
    if (result.impossible)
      return result;
    uint64_t capacity =
        SourceCheckBudget::add(lhs.atoms.size(), rhs.atoms.size());
    // Full capacity, comparisons, copies and cursor progress are prepaid.
    if (failed(charge(budget, site, 1, sizeof(Prefix))) ||
        failed(budget.debit(SourceCheckBudget::add(
                   1, SourceCheckBudget::multiply(3, capacity)), site)) ||
        failed(reserveAtoms(result, capacity, site)))
      return failure();
    size_t left = 0, right = 0;
    while (left < lhs.atoms.size() || right < rhs.atoms.size()) {
      if (right == rhs.atoms.size() ||
          (left < lhs.atoms.size() && lhs.atoms[left] < rhs.atoms[right]))
        result.atoms.push_back(lhs.atoms[left++]);
      else if (left == lhs.atoms.size() || rhs.atoms[right] < lhs.atoms[left])
        result.atoms.push_back(rhs.atoms[right++]);
      else {
        result.atoms.push_back(lhs.atoms[left++]);
        ++right;
      }
    }
    return result;
  }
  FailureOr<Prefix> conditional(const Prefix &path, const Prefix &acceptance,
                                Operation *site) {
    // Actual children have completed SelectionPath/Acceptance parsing before
    // this call, including pool hits. Expected children use the same restricted
    // known factories. Neither route can supply nested conditional references.
    for (auto [i, entry] : llvm::enumerate(pool)) {
      if (failed(budget.debit(1, site)))
        return failure();
      auto samePath = equivalent(path, entry.selectionPath, site);
      auto sameAcceptance = equivalent(acceptance, entry.acceptance, site);
      if (failed(samePath) || failed(sameAcceptance))
        return failure();
      if (*samePath && *sameAcceptance)
        return reference(i, site);
    }
    if (SourceCheckBudget::add(SourceCheckBudget::multiply(pool.size(), 2), 1) ==
        std::numeric_limits<uint64_t>::max())
      return budget.debit(std::numeric_limits<uint64_t>::max(), site);
    auto savedPath = copy(path, site), savedAcceptance = copy(acceptance, site);
    if (failed(savedPath) || failed(savedAcceptance) ||
        failed(grow(pool, site)) || failed(budget.debit(1, site)))
      return failure();
    uint64_t i = pool.size();
    pool.push_back({std::move(*savedPath), std::move(*savedAcceptance)});
    return reference(i, site);
  }
  LogicalResult equal(const Prefix &lhs, const Prefix &rhs, Operation *site) {
    auto result = equivalent(lhs, rhs, site);
    if (failed(result))
      return failure();
    if (!*result)
      return site->emitOpError()
             << "sampled reached/fire differs from its ordered control prefix";
    return success();
  }

private:
  template <typename T>
  LogicalResult grow(SmallVector<T, 0> &storage, Operation *site) {
    if (storage.size() < storage.capacity())
      return success();
    uint64_t capacity = SourceCheckBudget::add(
        SourceCheckBudget::multiply(storage.capacity(), 2), 1);
    uint64_t bytes =
        SourceCheckBudget::multiply(capacity - storage.capacity(), sizeof(T));
    if (capacity == std::numeric_limits<uint64_t>::max() ||
        bytes == std::numeric_limits<uint64_t>::max() ||
        capacity > storage.max_size())
      return budget.debit(std::numeric_limits<uint64_t>::max(), site);
    if (failed(
            budget.debit(SourceCheckBudget::add(SourceCheckBudget::bytes(bytes),
                                                storage.size()),
                         site)))
      return failure();
    storage.reserve(capacity);
    return success();
  }
  LogicalResult reserveAtoms(Prefix &prefix, uint64_t capacity,
                             Operation *site) {
    if (capacity == std::numeric_limits<uint64_t>::max() ||
        capacity > prefix.atoms.max_size())
      return budget.debit(std::numeric_limits<uint64_t>::max(), site);
    if (failed(budget.debit(
            SourceCheckBudget::multiply(
                capacity, SourceCheckBudget::bytes(sizeof(uint64_t))),
            site)))
      return failure();
    if (capacity)
      prefix.atoms.reserve(capacity);
    return success();
  }
  FailureOr<Prefix> copy(const Prefix &prefix, Operation *site) {
    if (failed(charge(budget, site, 1, sizeof(Prefix))))
      return failure();
    Prefix result;
    result.impossible = prefix.impossible;
    if (failed(reserveAtoms(result, prefix.atoms.size(), site)) ||
        failed(budget.debit(prefix.atoms.size(), site)))
      return failure();
    result.atoms.append(prefix.atoms.begin(), prefix.atoms.end());
    return result;
  }
  FailureOr<bool> equivalent(const Prefix &lhs, const Prefix &rhs,
                             Operation *site) {
    if (failed(budget.debit(SourceCheckBudget::add(
                   1, std::min(lhs.atoms.size(), rhs.atoms.size())), site)))
      return failure();
    return lhs.impossible == rhs.impossible && lhs.atoms == rhs.atoms;
  }
  FailureOr<Prefix> reference(uint64_t i, Operation *site) {
    uint64_t id = SourceCheckBudget::add(SourceCheckBudget::multiply(i, 2), 1);
    if (id == std::numeric_limits<uint64_t>::max())
      return budget.debit(id, site);
    Prefix result;
    if (failed(reserveAtoms(result, 1, site)) || failed(budget.debit(1, site)))
      return failure();
    result.atoms.push_back(id);
    return result;
  }
  FailureOr<Prefix> knownEither(BitsBinaryOp binary,
                                const HardwareBindings &bound) {
    // Six requests: two comparison operations and their four raw operands.
    auto left = identity.normalize(binary.getLhs(), *fixedScope);
    auto right = identity.normalize(binary.getRhs(), *fixedScope);
    if (failed(left) || failed(right) || failed(budget.debit(2, binary)))
      return failure();
    auto a = left->value.getDefiningOp<BitsCompareOp>();
    auto b = right->value.getDefiningOp<BitsCompareOp>();
    if (!a || !b || a.getPredicate() != "case_eq" ||
        b.getPredicate() != "case_eq")
      return binary->emitOpError()
             << "sampled Known requires two case_eq comparisons";
    if (failed(analysis.verifyResolvedOperation(a, bound)) ||
        failed(analysis.verifyResolvedOperation(b, bound)))
      return failure();
    auto al = identity.normalize(a.getLhs(), *fixedScope);
    auto ar = identity.normalize(a.getRhs(), *fixedScope);
    auto bl = identity.normalize(b.getLhs(), *fixedScope);
    auto br = identity.normalize(b.getRhs(), *fixedScope);
    if (failed(al) || failed(ar) || failed(bl) || failed(br))
      return failure();
    auto avl = literal(*al, analysis, budget),
         avr = literal(*ar, analysis, budget);
    auto bvl = literal(*bl, analysis, budget),
         bvr = literal(*br, analysis, budget);
    if (failed(avl) || failed(avr) || failed(bvl) || failed(bvr))
      return failure();
    const HardwareIdentityEndpoint *raw = nullptr;
    for (bool reverse : {false, true}) {
      if (failed(budget.debit(1, binary)))
        return failure();
      const HardwareIdentityEndpoint *as[] = {&*ar, &*al};
      const HardwareIdentityEndpoint *bs[] = {&*br, &*bl};
      std::optional<bool> av[] = {*avl, *avr}, bv[] = {*bvl, *bvr};
      for (unsigned i = 0; i < 2; ++i)
        for (unsigned j = 0; j < 2; ++j) {
          if (!av[i].has_value() || *av[i] != reverse || !bv[j].has_value() ||
              *bv[j] == reverse)
            continue;
          if (failed(budget.debit(1, binary)) ||
              failed(payScope(as[i]->scope, budget, binary)) ||
              failed(payScope(bs[j]->scope, budget, binary)))
            return failure();
          if (sameEndpoint(*as[i], *bs[j]))
            raw = as[i];
        }
    }
    if (!raw)
      return binary->emitOpError()
             << "sampled Known comparisons require the same raw endpoint";
    return knownValue(*raw, KnownTest::Either, binary);
  }
  const HardwareAnalysis &analysis;
  SourceCheckBudget &budget;
  const HardwareIdentityScope *const fixedScope;
  IdentityMemo identity;
  SmallVector<KnownAtom, 0> knownAtoms;
  SmallVector<PrefixCache, 0> cache;
  SmallVector<ConditionalAtom, 0> pool;
};
// One context fixes the immutable package, analysis, budget, definition, full
// bindings, complete stop policy and Rule. Normalized SSA plus resolved type
// supplies the remaining node key; actual occurrence routing is never cached.
class IntentProof {
  using Key = std::pair<const void *, const void *>;
  struct Less {
    bool operator()(const Key &a, const Key &b) const {
      std::less<const void *> less;
      return less(a.first, b.first) ||
             (a.first == b.first && less(a.second, b.second));
    }
  };
  enum class State { Active, Complete };
  struct Frame {
    HardwareIdentityEndpoint endpoint;
    BitsSelectOp select;
    unsigned child = 0;
  };

public:
  IntentProof(const HardwareAnalysis &analysis, SourceCheckBudget &budget,
              const PreparedSampledBoundaries &prepared, RuleOp rule)
      : analysis(analysis), budget(budget), prepared(prepared), rule(rule),
        policy{HardwareIdentityTraversal::DefinitionLocal,
               prepared.getBoundaries()},
        identity(analysis, budget, policy) {}

  static uint64_t nodeStorageBytes() {
    return sizeof(std::pair<const Key, State>) + 4 * sizeof(void *);
  }
  static uint64_t rootStorageBytes() {
    return sizeof(Key) + 4 * sizeof(void *);
  }
  static uint64_t frameStorageBytes() { return sizeof(Frame); }
  FailureOr<HardwareIdentityEndpoint> normalize(Value value) {
    return identity.normalize(value, prepared.getScope());
  }
  LogicalResult prove(Value value) {
    auto root = normalize(value);
    if (failed(root))
      return failure();
    Key rootKey = key(*root);
    if (failed(lookup(roots.size(), valueSite(value))))
      return failure();
    if (roots.count(rootKey))
      return success();
    SmallVector<Frame, 0> pending;
    auto push = [&](HardwareIdentityEndpoint endpoint) -> LogicalResult {
      Operation *site = valueSite(endpoint.value);
      if (failed(bitOne(endpoint.value, analysis,
                        prepared.getScope().rootBindings, site, budget)) ||
          failed(lookup(nodes.size(), site)))
        return failure();
      auto found = nodes.find(key(endpoint));
      if (found != nodes.end()) {
        if (found->second == State::Active)
          return site->emitOpError() << "cyclic transaction write intent";
        return success();
      }
      if (failed(
              charge(budget, site, 1,
                     sizeof(std::pair<const Key, State>) + 4 * sizeof(void *))))
        return failure();
      if (failed(lookup(nodes.size(), site)))
        return failure();
      nodes.emplace(key(endpoint), State::Active);
      auto constant = endpoint.value.getDefiningOp<BitsConstantOp>();
      if (constant) {
        if (failed(analysis.verifyResolvedOperation(
                constant, prepared.getScope().rootBindings)))
          return failure();
        auto known = literal(endpoint, analysis, budget);
        if (failed(known))
          return failure();
        if (!known->has_value())
          return constant->emitOpError() << "transaction intent literal must "
                                            "be known Bits1 zero or one";
        if (failed(lookup(nodes.size(), site)))
          return failure();
        nodes.find(key(endpoint))->second = State::Complete;
        return success();
      }
      auto select = endpoint.value.getDefiningOp<BitsSelectOp>();
      if (!select)
        return site->emitOpError() << "transaction intent admits only known "
                                      "Bits1 zero/one and select";
      if (failed(analysis.verifyResolvedOperation(
              select, prepared.getScope().rootBindings)) ||
          failed(budget.debit(1, select)))
        return failure();
      auto selector = normalize(select.getCondition());
      if (failed(selector) ||
          failed(bitOne(selector->value, analysis,
                        prepared.getScope().rootBindings, select, budget)))
        return failure();
      // The selector is Bits1 data and may be unknown. It is not an intent or
      // known-one guard, and contributes no admission obligation.
      if (pending.size() == pending.capacity()) {
        uint64_t capacity = SourceCheckBudget::multiply(
            std::max(uint64_t(1), uint64_t(pending.capacity())), 2);
        uint64_t bytes = SourceCheckBudget::multiply(
            capacity - pending.capacity(), sizeof(Frame));
        if (capacity == std::numeric_limits<uint64_t>::max() ||
            bytes == std::numeric_limits<uint64_t>::max() ||
            capacity > pending.max_size())
          return budget.debit(std::numeric_limits<uint64_t>::max(), site);
        if (failed(budget.debit(
                SourceCheckBudget::add(SourceCheckBudget::bytes(bytes),
                                       pending.size()),
                site)))
          return failure();
        pending.reserve(capacity);
      }
      if (failed(budget.debit(1, site)) ||
          failed(payScope(endpoint.scope, budget, site)))
        return failure();
      pending.push_back({std::move(endpoint), select, 0});
      return success();
    };
    if (failed(push(std::move(*root))))
      return failure();
    while (!pending.empty()) {
      auto &frame = pending.back();
      if (failed(budget.debit(1, frame.select)))
        return failure();
      if (frame.child < 2) {
        Value child = frame.child++ == 0 ? frame.select.getTrueValue()
                                         : frame.select.getFalseValue();
        if (failed(budget.debit(1, frame.select)))
          return failure();
        auto normalized = normalize(child);
        if (failed(normalized) || failed(push(std::move(*normalized))))
          return failure();
      } else {
        if (failed(lookup(nodes.size(), frame.select)))
          return failure();
        nodes.find(key(frame.endpoint))->second = State::Complete;
        pending.pop_back();
      }
    }
    if (failed(lookup(roots.size(), valueSite(value))) ||
        failed(charge(budget, valueSite(value), 1,
                      sizeof(Key) + 4 * sizeof(void *))))
      return failure();
    roots.insert(rootKey); // Only successful roots are reusable.
    return success();
  }

private:
  static Key key(const HardwareIdentityEndpoint &endpoint) {
    return {endpoint.value.getAsOpaquePointer(),
            endpoint.resolvedType.getAsOpaquePointer()};
  }
  LogicalResult lookup(uint64_t count, Operation *site) {
    uint64_t comparisons = 1;
    for (uint64_t n = SourceCheckBudget::add(count, 1); n > 1;
         n = n / 2 + n % 2)
      comparisons = SourceCheckBudget::add(comparisons, 2);
    return budget.debit(comparisons, site);
  }
  const HardwareAnalysis &analysis;
  SourceCheckBudget &budget;
  const PreparedSampledBoundaries &prepared;
  RuleOp rule;
  HardwareIdentityStopPolicy policy;
  IdentityMemo identity;
  std::map<Key, State, Less> nodes;
  std::set<Key, Less> roots;
};
FailureOr<SmallVector<HardwareIdentityEndpoint>>
localBoundaries(ModuleOp module, const HardwareIdentityScope &scope,
                const HardwareAnalysis &analysis, SourceCheckBudget &budget) {
  SmallVector<HardwareIdentityEndpoint> result;
  std::optional<TransactionWiringView> wiring;
  auto bindings = scopeBindings(scope, analysis, budget, module);
  if (failed(bindings))
    return failure();
  if (module->hasAttr("ac.transaction_wiring")) {
    auto parsed = parseTransactionWiring(module, analysis, *bindings, budget);
    if (failed(parsed))
      return failure();
    wiring = std::move(*parsed);
  }
  for (auto rule : module.getBody().front().getOps<RuleOp>()) {
    uint64_t first = rule->getNumResults();
    if (rule->hasAttr("ac.sampled_admission")) {
      if (wiring) {
        const TransactionWriterView *writer = nullptr;
        for (const auto &candidate : wiring->writers) {
          if (failed(budget.debit(1, rule)))
            return failure();
          if (candidate.rule == rule)
            writer = &candidate;
        }
        if (!writer)
          return rule->emitOpError()
                 << "sampled Rule lacks its transaction writer";
        first = writer->ordinaryResults;
      } else {
        auto sampled = parseSampledAdmission(rule, analysis, *bindings, budget);
        if (failed(sampled))
          return failure();
        first = sampled->fireResult;
      }
    } else if (auto required =
                   rule->getAttrOfType<ArrayAttr>("ac.required_checks")) {
      if (required.size() > rule->getNumResults() / 2)
        return rule->emitOpError()
               << "identity check boundary is outside Rule results";
      first = rule->getNumResults() - 2 * required.size();
    }
    for (uint64_t ordinal = first; ordinal < rule->getNumResults(); ++ordinal) {
      if (failed(charge(budget, rule, 1, sizeof(HardwareIdentityEndpoint))))
        return failure();
      auto endpoint = makeIdentityEndpoint(rule->getResult(ordinal), scope,
                                           analysis, budget);
      if (failed(endpoint))
        return failure();
      result.push_back(std::move(*endpoint));
    }
  }
  return result;
}
} // namespace

PreparedSampledBoundaries::PreparedSampledBoundaries(
    HardwareIdentityScope &&scope,
    SmallVector<HardwareIdentityEndpoint> &&boundaries)
    : scope(std::move(scope)), boundaries(std::move(boundaries)) {}

LogicalResult
PreparedSampledBoundaries::verifyFor(ModuleOp definition,
                                     const HardwareBindings &bindings,
                                     SourceCheckBudget &budget) const {
  Operation *site = scope.root.operator->();
  if (failed(budget.debit(
          SourceCheckBudget::add(
              1, SourceCheckBudget::add(
                     SourceCheckBudget::bindings(bindings),
                     SourceCheckBudget::bindings(scope.rootBindings))),
          site)))
    return failure();
  if (!definition || scope.root != definition || !scope.path.empty() ||
      !sameHardwareBindings(bindings, scope.rootBindings))
    return site->emitOpError()
           << "prepared sampled boundaries differ from definition or bindings";
  return success();
}

FailureOr<std::optional<PreparedSampledBoundaries>>
prepareSampledBoundaries(ModuleOp definition, const HardwareAnalysis &analysis,
                         const HardwareBindings &bindings,
                         SourceCheckBudget &budget) {
  if (!definition || !isa<ModuleOp>(definition.operator->()))
    return analysis.getPackage().emitOpError()
           << "sampled boundary preparation requires a Module definition";
  if (bindings.owner != definition || definition->getNumRegions() != 1 ||
      !definition.getBody().hasOneBlock())
    return definition->emitOpError() << "sampled boundary preparation requires "
                                        "its owner and one graph block";
  bool sampledDefinition =
      definition->hasAttr("ac.transaction_wiring") ||
      llvm::any_of(
          definition.getBody().front().getOps<RuleOp>(),
          [](RuleOp rule) { return rule->hasAttr("ac.sampled_admission"); });
  if (!sampledDefinition)
    return std::optional<PreparedSampledBoundaries>();
  if (failed(budget.debit(
          SourceCheckBudget::add(
              SourceCheckBudget::add(1, SourceCheckBudget::bytes(
                                            sizeof(PreparedSampledBoundaries))),
              SourceCheckBudget::bindings(bindings)),
          definition)))
    return failure();
  HardwareIdentityScope scope{definition, bindings, {}};
  auto boundaries = localBoundaries(definition, scope, analysis, budget);
  if (failed(boundaries))
    return failure();
  PreparedSampledBoundaries prepared(std::move(scope), std::move(*boundaries));
  return std::optional<PreparedSampledBoundaries>(std::move(prepared));
}

LogicalResult proveSampledAdmission(const SampledAdmissionView &view,
                                    const PreparedSampledBoundaries &prepared,
                                    const HardwareAnalysis &analysis,
                                    const HardwareBindings &bindings,
                                    SourceCheckBudget &budget) {
  if (bindings.owner != view.definition)
    return analysis.getPackage().emitOpError()
           << "sampled proof bindings have the wrong owner";
  if (!view.definition || !view.rule || view.controls.empty())
    return analysis.getPackage().emitOpError()
           << "sampled proof requires a parsed nonempty control inventory";
  if (failed(prepared.verifyFor(view.definition, bindings, budget)))
    return failure();
  const auto &scope = prepared.getScope();
  HardwareIdentityStopPolicy policy{HardwareIdentityTraversal::DefinitionLocal,
                                    prepared.getBoundaries()};
  if (failed(charge(budget, view.rule, 1,
                    sizeof(const HardwareIdentityScope *))))
    return failure();
  OrderedPrefixProof proof(analysis, budget, policy, scope);
  Prefix expected, root;
  SmallVector<ExpectedSelectionPaths, 0> selections;
  uint64_t count = llvm::count_if(view.controls, [](const auto &control) {
    return control.kind == SampledControlKind::BranchSelect;
  });
  if (count > selections.max_size() ||
      failed(charge(budget, view.rule, count, sizeof(ExpectedSelectionPaths))))
    return failure();
  if (count)
    selections.reserve(count);
  auto selectedPath = [&](SampledPathRef path) -> FailureOr<const Prefix *> {
    if (!path.selectionControl)
      return &root;
    if (failed(budget.debit(1, view.rule)))
      return failure();
    for (const auto &selection : selections) {
      if (failed(budget.debit(1, view.rule)))
        return failure();
      if (selection.control == *path.selectionControl)
        return path.whenTrue ? &selection.whenTrue : &selection.whenFalse;
    }
    return view.rule->emitOpError()
           << "sampled path has no established selection";
  };
  for (auto [i, control] : llvm::enumerate(view.controls)) {
    auto path = selectedPath(control.executionPath);
    if (failed(path))
      return failure();
    auto reached = proof.parse(control.reached);
    if (failed(reached))
      return failure();
    if (control.executionPath.selectionControl) {
      auto expectedReached = proof.conjunction(expected, **path, view.rule);
      if (failed(expectedReached) ||
          failed(proof.equal(*reached, *expectedReached, view.rule)))
        return failure();
    } else if (failed(proof.equal(*reached, expected, view.rule)))
      return failure();
    auto raw = proof.normalize(control.raw);
    if (failed(raw))
      return failure();
    auto accepted = proof.knownValue(
        *raw,
        control.kind == SampledControlKind::CallableGuard ? KnownTest::One
                                                          : KnownTest::Either,
        view.rule);
    if (failed(accepted))
      return failure();
    if (control.kind == SampledControlKind::BranchSelect) {
      auto zero = proof.knownValue(*raw, KnownTest::Zero, view.rule);
      auto one = proof.knownValue(*raw, KnownTest::One, view.rule);
      if (failed(zero) || failed(one))
        return failure();
      auto whenTrue = proof.conjunction(**path, *one, view.rule);
      auto whenFalse = proof.conjunction(**path, *zero, view.rule);
      if (failed(whenTrue) || failed(whenFalse))
        return failure();
      selections.push_back({i, std::move(*whenTrue), std::move(*whenFalse)});
    }
    if (control.executionPath.selectionControl) {
      auto bypass = proof.conditional(**path, *accepted, view.rule);
      if (failed(bypass))
        return failure();
      accepted = std::move(*bypass);
    }
    auto next = proof.conjunction(expected, *accepted, view.rule);
    if (failed(next))
      return failure();
    expected = std::move(*next);
  }
  auto fire = proof.parse(view.fire);
  if (failed(fire) || failed(proof.equal(*fire, expected, view.rule)))
    return failure();
  auto exactFire = normalized(view.fire, scope, policy, analysis, budget);
  if (failed(exactFire))
    return failure();
  auto effect = [&](Value value,
                    SampledPathRef executionPath) -> LogicalResult {
    auto endpoint = normalized(value, scope, policy, analysis, budget);
    if (failed(endpoint))
      return failure();
    if (executionPath.selectionControl) {
      auto binary = endpoint->value.getDefiningOp<BitsBinaryOp>();
      if (!binary || binary.getOpcode() != "and")
        return view.rule->emitOpError()
               << "sampled arm effect requires canonical final-fire AND path";
      endpoint = normalized(binary.getLhs(), endpoint->scope, policy, analysis,
                            budget);
      if (failed(endpoint))
        return failure();
    }
    if (failed(payScope(endpoint->scope, budget, view.rule)) ||
        failed(payScope(exactFire->scope, budget, view.rule)))
      return failure();
    if (!sameEndpoint(*endpoint, *exactFire))
      return view.rule->emitOpError()
             << "sampled effect requires exact internal final fire";
    auto actual = proof.parse(value);
    if (failed(actual))
      return failure();
    if (!executionPath.selectionControl)
      return proof.equal(*actual, expected, view.rule);
    auto path = selectedPath(executionPath);
    if (failed(path))
      return failure();
    auto expectedEffect = proof.conjunction(expected, **path, view.rule);
    if (failed(expectedEffect))
      return failure();
    return proof.equal(*actual, *expectedEffect, view.rule);
  };
  for (const auto &check : view.bodyChecks)
    if (failed(effect(check.path, check.executionPath)))
      return failure();
  for (const auto &observation : view.observations)
    if (failed(effect(observation.path, observation.executionPath)))
      return failure();
  return success();
}

struct SampledAdmissionValidation::Entry {
  explicit Entry(PreparedSampledBoundaries &&prepared)
      : prepared(std::move(prepared)) {}
  PreparedSampledBoundaries prepared;
  std::optional<TransactionWiringView> wiring;
  SmallVector<RuleOp, 0> provenRules;
  SmallVector<std::pair<RuleOp, std::unique_ptr<IntentProof>>, 0> intentProofs;
};

SampledAdmissionValidation::SampledAdmissionValidation(
    const HardwareAnalysis &analysis, SourceCheckBudget &budget)
    : analysis(analysis), budget(budget) {}

SampledAdmissionValidation::~SampledAdmissionValidation() = default;

uint64_t SampledAdmissionValidation::prefixKnownAtomStorageBytes() {
  return sizeof(KnownAtom);
}
uint64_t SampledAdmissionValidation::prefixValueStorageBytes() {
  return sizeof(Prefix);
}
uint64_t SampledAdmissionValidation::prefixCacheStorageBytes() {
  return sizeof(PrefixCache);
}
uint64_t SampledAdmissionValidation::prefixConditionalStorageBytes() {
  return sizeof(ConditionalAtom);
}
uint64_t SampledAdmissionValidation::prefixFrameStorageBytes() {
  return sizeof(PrefixParseFrame);
}
uint64_t SampledAdmissionValidation::prefixSelectionPathsStorageBytes() {
  return sizeof(ExpectedSelectionPaths);
}

uint64_t SampledAdmissionValidation::entryStorageBytes() {
  return sizeof(Entry) + sizeof(std::unique_ptr<Entry>);
}

uint64_t SampledAdmissionValidation::intentProofStorageBytes() {
  return sizeof(IntentProof) +
         sizeof(std::pair<RuleOp, std::unique_ptr<IntentProof>>);
}
uint64_t SampledAdmissionValidation::intentNodeStorageBytes() {
  return IntentProof::nodeStorageBytes();
}
uint64_t SampledAdmissionValidation::intentRootStorageBytes() {
  return IntentProof::rootStorageBytes();
}
uint64_t SampledAdmissionValidation::intentFrameStorageBytes() {
  return IntentProof::frameStorageBytes();
}

LogicalResult
SampledAdmissionValidation::verifyInvocation(const HardwareAnalysis &analysis,
                                             SourceCheckBudget &budget) const {
  auto package = this->analysis.getPackage();
  if (failed(this->budget.debit(1, package)))
    return failure();
  if (&analysis != &this->analysis || &budget != &this->budget)
    return package.emitOpError()
           << "sampled validation differs from analysis or budget invocation";
  return success();
}

FailureOr<const PreparedSampledBoundaries *>
SampledAdmissionValidation::prepare(ModuleOp definition,
                                    const HardwareBindings &bindings) {
  Operation *site = definition ? definition.getOperation()
                               : analysis.getPackage().getOperation();
  if (failed(budget.debit(1, site)))
    return failure();
  if (!definition ||
      definition->getParentOp() != analysis.getPackage().getOperation())
    return site->emitOpError()
           << "sampled validation definition must belong to analyzed package";
  for (const auto &entry : entries) {
    if (failed(budget.debit(1, site)))
      return failure();
    const auto &scope = entry->prepared.getScope();
    if (scope.root != definition)
      continue;
    if (failed(
            budget.debit(SourceCheckBudget::add(
                             SourceCheckBudget::bindings(bindings),
                             SourceCheckBudget::bindings(scope.rootBindings)),
                         site)))
      return failure();
    if (sameHardwareBindings(bindings, scope.rootBindings))
      return &entry->prepared;
  }
  auto prepared =
      prepareSampledBoundaries(definition, analysis, bindings, budget);
  if (failed(prepared))
    return failure();
  if (!*prepared)
    return static_cast<const PreparedSampledBoundaries *>(nullptr);
  if (failed(charge(budget, site, 1, entryStorageBytes())))
    return failure();
  entries.push_back(std::make_unique<Entry>(std::move(**prepared)));
  return &entries.back()->prepared;
}

FailureOr<const TransactionWiringView *>
SampledAdmissionValidation::wiring(ModuleOp definition,
                                   const HardwareBindings &bindings) {
  auto prepared = prepare(definition, bindings);
  if (failed(prepared))
    return failure();
  if (!*prepared)
    return definition->emitOpError()
           << "wiring validation lacks complete definition boundaries";
  for (auto &entry : entries) {
    if (failed(budget.debit(1, definition)))
      return failure();
    if (&entry->prepared != *prepared)
      continue;
    if (!entry->wiring) {
      auto parsed =
          parseTransactionWiring(definition, analysis, bindings, budget);
      if (failed(parsed))
        return failure();
      uint64_t moveWork = SourceCheckBudget::add(
          1, SourceCheckBudget::add(
                 parsed->states.size(),
                 SourceCheckBudget::add(parsed->incoming.size(),
                                        parsed->writers.size())));
      for (const auto &writer : parsed->writers)
        moveWork = SourceCheckBudget::add(moveWork, writer.candidates.size());
      if (failed(budget.debit(moveWork, definition)))
        return failure();
      entry->wiring = std::move(*parsed);
    }
    // Definition, complete bindings and private boundaries are fixed by Entry.
    // This is immutable schema preparation, never an occurrence/root verdict.
    return &*entry->wiring;
  }
  return definition->emitOpError()
         << "wiring validation has no exact definition entry";
}

FailureOr<SampledAdmissionView>
SampledAdmissionValidation::verifySampledAdmissionImpl(
    RuleOp rule, const HardwareBindings &bindings) {
  Operation *site =
      rule ? rule.getOperation() : analysis.getPackage().getOperation();
  if (failed(budget.debit(1, site)))
    return failure();
  auto definition =
      rule ? dyn_cast_or_null<ModuleOp>(rule->getParentOp()) : ModuleOp();
  if (!definition)
    return site->emitOpError()
           << "sampled validation requires a Rule directly owned by a Module";
  auto prepared = prepare(definition, bindings);
  if (failed(prepared))
    return failure();
  if (!*prepared)
    return rule->emitOpError()
           << "sampled validation lacks prepared definition boundaries";
  // Parse current IR on every call. Only a completed prefix proof is reused,
  // under immutable IR, exact bindings and the prepared complete stop policy.
  auto sampled = parseSampledAdmission(rule, analysis, bindings, budget);
  if (failed(sampled))
    return failure();
  for (const auto &entry : entries) {
    if (failed(budget.debit(1, rule)))
      return failure();
    if (&entry->prepared != *prepared)
      continue;
    for (auto proven : entry->provenRules) {
      if (failed(budget.debit(1, rule)))
        return failure();
      if (proven == rule)
        return std::move(*sampled);
    }
    if (failed(proveSampledAdmission(*sampled, entry->prepared, analysis,
                                     bindings, budget)) ||
        failed(charge(budget, rule, 1, sizeof(RuleOp))))
      return failure();
    entry->provenRules.push_back(rule);
    return std::move(*sampled);
  }
  return rule->emitOpError()
         << "sampled validation has no entry for prepared boundaries";
}

FailureOr<SampledAdmissionView>
SampledAdmissionValidation::verify(RuleOp rule,
                                   const HardwareBindings &bindings) {
  auto sampled = verifySampledAdmissionImpl(rule, bindings);
  if (failed(sampled))
    return failure();
  if (sampled->definition->hasAttr("ac.transaction_wiring")) {
    auto view = wiring(sampled->definition, bindings);
    if (failed(view))
      return failure();
    const TransactionWriterView *writer = nullptr;
    for (const auto &candidate : (*view)->writers) {
      if (failed(budget.debit(1, rule)))
        return failure();
      if (candidate.rule == rule)
        writer = &candidate;
    }
    if (!writer)
      return rule->emitOpError() << "sampled Rule lacks its transaction writer";
    auto enables = verifyCandidateEnablesImpl(*writer, *sampled, bindings);
    if (failed(enables))
      return failure();
  }
  return std::move(*sampled);
}

FailureOr<SmallVector<CheckedCandidateEnable, 0>>
SampledAdmissionValidation::verifyCandidateEnables(
    const TransactionWriterView &writer, const HardwareBindings &bindings) {
  auto sampled = verifySampledAdmissionImpl(writer.rule, bindings);
  if (failed(sampled))
    return failure();
  return verifyCandidateEnablesImpl(writer, *sampled, bindings);
}

FailureOr<SmallVector<CheckedCandidateEnable, 0>>
SampledAdmissionValidation::verifyCandidateEnablesImpl(
    const TransactionWriterView &writer,
    const SampledAdmissionView &freshSampled,
    const HardwareBindings &bindings) {
  const SampledAdmissionView *sampled = &freshSampled;
  // Reparse this exact writer, not every unrelated inventory and writer in
  // the definition. Full wiring admission remains the caller's independent
  // owner; this typed query authenticates its current closed row and slots.
  auto wiring = sampled->definition->getAttrOfType<DictionaryAttr>(
      "ac.transaction_wiring");
  if (failed(exact(wiring, {"states", "incoming", "writers"}, writer.rule,
                   "ac.transaction_wiring")))
    return failure();
  auto rows = wiring.getAs<ArrayAttr>("writers");
  if (!rows || !wiring.getAs<ArrayAttr>("states") ||
      !wiring.getAs<ArrayAttr>("incoming"))
    return writer.rule->emitOpError()
           << "transaction wiring requires array inventories";
  DictionaryAttr actual;
  auto registration = writer.rule->getAttrOfType<DictionaryAttr>("occurrence");
  for (Attribute raw : rows) {
    if (failed(budget.debit(1, writer.rule)))
      return failure();
    auto row = dyn_cast<DictionaryAttr>(raw);
    if (!row || row.get("registration") != registration)
      continue;
    if (actual)
      return writer.rule->emitOpError()
             << "duplicate transaction writer registration";
    actual = row;
  }
  if (!actual || failed(payAttribute(actual, budget, writer.rule)) ||
      failed(exact(actual, {"registration", "ordinary_results", "candidates"},
                   writer.rule, "transaction writer")))
    return failure();
  auto ordinary = index(actual, "ordinary_results", writer.rule);
  auto candidates = actual.getAs<ArrayAttr>("candidates");
  if (failed(ordinary) || !candidates ||
      actual.get("registration") != writer.registration ||
      static_cast<uint64_t>(*ordinary) != writer.ordinaryResults ||
      candidates.size() != writer.candidates.size() ||
      SourceCheckBudget::add(
          *ordinary, SourceCheckBudget::multiply(2, candidates.size())) !=
          sampled->fireResult)
    return writer.rule->emitOpError()
           << "candidate enable writer differs from actual wiring";
  for (auto [i, raw] : llvm::enumerate(candidates)) {
    if (failed(budget.debit(1, writer.rule)))
      return failure();
    auto row = dyn_cast<DictionaryAttr>(raw);
    if (failed(exact(row, {"state", "data", "enable"}, writer.rule,
                     "transaction candidate")))
      return failure();
    auto state = row.getAs<DictionaryAttr>("state");
    auto data = index(row, "data", writer.rule);
    auto enable = index(row, "enable", writer.rule);
    uint64_t expected =
        SourceCheckBudget::add(*ordinary, SourceCheckBudget::multiply(2, i));
    if (failed(data) || failed(enable) ||
        failed(wholeStateID(state, writer.rule)))
      return failure();
    const auto &view = writer.candidates[i];
    if (expected == std::numeric_limits<uint64_t>::max() ||
        static_cast<uint64_t>(*data) != expected ||
        static_cast<uint64_t>(*enable) != SourceCheckBudget::add(expected, 1) ||
        static_cast<uint64_t>(*enable) >= sampled->fireResult ||
        state != view.stateID ||
        static_cast<uint64_t>(*data) != view.dataIndex ||
        static_cast<uint64_t>(*enable) != view.enableIndex ||
        writer.rule->getResult(*data) != view.data ||
        writer.rule->getResult(*enable) != view.enable)
      return writer.rule->emitOpError()
             << "candidate enable query differs from actual slots";
    if (failed(bitOne(view.enable, analysis, bindings, writer.rule, budget)))
      return failure();
    for (uint64_t j = 0; j < i; ++j) {
      if (failed(payComparison(writer.candidates[j].stateID, state, budget,
                               writer.rule)))
        return failure();
      if (identical(writer.candidates[j].stateID, state))
        return writer.rule->emitOpError()
               << "duplicate candidate owner within transaction";
    }
  }
  auto prepared = prepare(sampled->definition, bindings);
  if (failed(prepared) || !*prepared)
    return failure();
  IntentProof *proof = nullptr;
  for (auto &entry : entries) {
    if (failed(budget.debit(1, writer.rule)))
      return failure();
    if (&entry->prepared != *prepared)
      continue;
    for (auto &existing : entry->intentProofs) {
      if (failed(budget.debit(1, writer.rule)))
        return failure();
      if (existing.first == writer.rule)
        proof = existing.second.get();
    }
    if (!proof) {
      if (failed(charge(budget, writer.rule, 1, sizeof(IntentProof))))
        return failure();
      if (entry->intentProofs.size() == entry->intentProofs.capacity()) {
        uint64_t capacity = SourceCheckBudget::multiply(
            std::max(uint64_t(1), uint64_t(entry->intentProofs.capacity())), 2);
        uint64_t bytes = SourceCheckBudget::multiply(
            capacity - entry->intentProofs.capacity(),
            sizeof(std::pair<RuleOp, std::unique_ptr<IntentProof>>));
        if (capacity == std::numeric_limits<uint64_t>::max() ||
            bytes == std::numeric_limits<uint64_t>::max() ||
            capacity > entry->intentProofs.max_size())
          return budget.debit(std::numeric_limits<uint64_t>::max(),
                              writer.rule);
        if (failed(budget.debit(
                SourceCheckBudget::add(SourceCheckBudget::bytes(bytes),
                                       entry->intentProofs.size()),
                writer.rule)))
          return failure();
        entry->intentProofs.reserve(capacity);
      }
      if (failed(budget.debit(1, writer.rule)))
        return failure();
      entry->intentProofs.emplace_back(
          writer.rule, std::make_unique<IntentProof>(
                           analysis, budget, entry->prepared, writer.rule));
      proof = entry->intentProofs.back().second.get();
    }
    break;
  }
  if (!proof)
    return writer.rule->emitOpError()
           << "candidate intent lacks exact proof context";
  SmallVector<CheckedCandidateEnable, 0> result;
  if (writer.candidates.size() > result.max_size() ||
      failed(charge(budget, writer.rule, writer.candidates.size(),
                    sizeof(CheckedCandidateEnable))))
    return failure();
  result.reserve(writer.candidates.size());
  auto yielded =
      cast<YieldOp>(writer.rule->getRegion(0).front().getTerminator());
  auto fire = proof->normalize(sampled->fire);
  if (failed(fire))
    return failure();
  for (const auto &candidate : writer.candidates) {
    // Enter through the exact validated internal yield slot, before identity
    // normalization. Complete private D/E/fire boundaries remain unchanged.
    auto enabled = proof->normalize(yielded.getOperand(candidate.enableIndex));
    if (failed(enabled))
      return failure();
    auto binary = enabled->value.getDefiningOp<BitsBinaryOp>();
    if (!binary || binary.getOpcode() != "and")
      return writer.rule->emitOpError()
             << "candidate enable requires exact AND(final fire, intent)";
    if (failed(budget.debit(1, binary)) ||
        failed(analysis.verifyResolvedOperation(binary, bindings)))
      return failure();
    auto lhs = proof->normalize(binary.getLhs());
    if (failed(lhs) || failed(payScope(lhs->scope, budget, binary)) ||
        failed(payScope(fire->scope, budget, binary)))
      return failure();
    if (!sameEndpoint(*lhs, *fire))
      return binary->emitOpError()
             << "candidate enable left operand must be internal final fire";
    if (failed(budget.debit(1, binary)) ||
        failed(proof->prove(binary.getRhs())))
      return failure();
    result.push_back({enabled->value, binary.getRhs()});
  }
  return result;
}

namespace {
struct Occurrence {
  HardwareIdentityScope scope;
  HardwareBindings bindings;
  ModuleOp module;
  const TransactionWiringView *wiring = nullptr;
  std::optional<size_t> parent;
  InstanceOp allocation;
};
struct State {
  HardwareIdentityScope scope;
  InstanceOp storage;
  Type payload;
  std::optional<HardwareIdentityEndpoint> data, enable;
  std::optional<HardwareIdentityEndpoint> clock, reset;
  RuleOp writer;
  bool external = false;
};
struct PrivateRoute {
  HardwareIdentityEndpoint endpoint;
  size_t state = 0;
  enum Kind { Data, Enable, EffectOperand } kind;
  Operation *terminal = nullptr;
  unsigned operand = 0;
};
// The surrounding route invocation fixes state, role, terminal and operand;
// this index therefore stores only the exact endpoint identity.
using RouteVisitKey = std::tuple<const void *, const void *, size_t>;
struct EffectTerminalIndex {
  ModuleOp definition;
  HardwareBindings bindings;
  SmallVector<std::pair<DictionaryAttr, SourceExpectOp>, 0> checks;
  SmallVector<std::pair<DictionaryAttr, SourceObserveOp>, 0> observations;
};
struct EffectOwnerProof {
  ModuleOp definition;
  HardwareBindings bindings;
  RuleOp rule;
  Operation *terminal;
  ArrayAttr invocation;
  ResolvedSourceEffectOwner owner;
};
struct EffectProofCache {
  SmallVector<EffectTerminalIndex, 0> terminals;
  SmallVector<EffectOwnerProof, 0> owners;
};
// A proof invocation owns one actual root. The catalog caller supplies every
// definition/binding root, including unselected definitions; no package closure
// or manifest parser is constructed here.
class WiringProof {
public:
  WiringProof(const HardwareAnalysis &analysis, SourceCheckBudget &budget,
              const llvm::DenseSet<Operation *> &relevant,
              SampledAdmissionValidation &validation,
              EffectProofCache &effectProofCache)
      : analysis(analysis), budget(budget), validation(validation),
        relevant(relevant), effectProofCache(effectProofCache) {}
  LogicalResult run(ModuleOp root, const HardwareBindings &bindings) {
    if (bindings.owner != root)
      return root->emitOpError()
             << "transaction proof bindings have the wrong owner";
    budget.setPhase("sampled wiring occurrence expansion");
    if (failed(budget.debit(SourceCheckBudget::bindings(bindings), root)))
      return failure();
    HardwareIdentityScope scope{root, bindings, {}};
    scope.rootBindings.owner = root;
    if (failed(expand(scope, std::nullopt, {})) ||
        failed(buildOccurrenceIndex()))
      return failure();
    budget.setPhase("sampled wiring state inventory");
    if (failed(inventory()))
      return failure();
    budget.setPhase("sampled wiring writers");
    if (failed(writers()))
      return failure();
    budget.setPhase("sampled wiring external inputs");
    if (failed(externalInputs()))
      return failure();
    budget.setPhase("sampled wiring connections");
    if (failed(connections()))
      return failure();
    budget.setPhase("sampled wiring private uses");
    if (failed(privateUses()))
      return failure();
    budget.setPhase("sampled transaction wiring proof");
    return success();
  }

private:
  static uint64_t occurrenceHash(const HardwareIdentityScope &scope) {
    llvm::hash_code hash = llvm::hash_value(scope.root.operator->());
    for (InstanceOp instance : scope.path)
      hash = llvm::hash_combine(hash, instance.operator->());
    return static_cast<uint64_t>(static_cast<size_t>(hash));
  }
  LogicalResult buildOccurrenceIndex() {
    for (auto [ordinal, entry] : llvm::enumerate(occurrences)) {
      if (failed(
              budget.debit(SourceCheckBudget::add(1, entry.scope.path.size()),
                           entry.module)))
        return failure();
      uint64_t hash = occurrenceHash(entry.scope);
      auto found = occurrenceIndex.find(hash);
      if (found == occurrenceIndex.end()) {
        if (failed(
                charge(budget, entry.module, 1,
                       sizeof(std::pair<uint64_t, SmallVector<size_t>>) + 64)))
          return failure();
        found = occurrenceIndex.try_emplace(hash).first;
      }
      for (size_t prior : found->second) {
        const auto &priorPath = occurrences[prior].scope.path;
        if (failed(budget.debit(
                SourceCheckBudget::add(
                    1, std::min(priorPath.size(), entry.scope.path.size())),
                entry.module)))
          return failure();
        if (priorPath == entry.scope.path)
          return entry.module->emitOpError()
                 << "duplicate actual module occurrence scope";
      }
      if (failed(charge(budget, entry.module, 1, sizeof(size_t))))
        return failure();
      found->second.push_back(ordinal);
    }
    return success();
  }
  LogicalResult expand(const HardwareIdentityScope &scope,
                       std::optional<size_t> parent, InstanceOp allocation) {
    ModuleOp module = scopeOwner(scope, analysis);
    if (!module || !module.getBody().hasOneBlock())
      return analysis.getPackage().emitOpError()
             << "transaction proof requires ordinary graph modules";
    for (auto ancestor = parent; ancestor;
         ancestor = occurrences[*ancestor].parent)
      if (occurrences[*ancestor].module == module)
        return module->emitOpError()
               << "cyclic ordinary instance transaction wiring";
    auto bound = scopeBindings(scope, analysis, budget, module);
    auto copied = copyScope(scope, budget, module);
    if (failed(bound) || failed(copied) ||
        failed(charge(budget, module, 1, sizeof(Occurrence))))
      return failure();
    Occurrence entry{std::move(*copied), std::move(*bound), module, {}, parent,
                     allocation};
    bool witness = module->hasAttr("ac.internal_ports");
    for (auto &operation : module.getBody().front()) {
      if (failed(budget.debit(1, &operation)))
        return failure();
      if (auto rule = dyn_cast<RuleOp>(operation)) {
        witness |= rule->hasAttr("ac.sampled_admission");
        if (auto required =
                rule->getAttrOfType<ArrayAttr>("ac.required_checks"))
          for (auto raw : required) {
            auto row = dyn_cast<DictionaryAttr>(raw);
            auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
            witness |= kind && kind.getValue() == "guard_known";
          }
      }
      if (auto expect = dyn_cast<SourceExpectOp>(operation))
        witness |= expect.getKind() == "guard_known";
    }
    if (witness && !module->hasAttr("ac.transaction_wiring"))
      return module->emitOpError() << "sampled/check/internal-port witness "
                                      "requires transaction_wiring companion";
    if (module->hasAttr("ac.transaction_wiring")) {
      auto wiring = validation.wiring(module, entry.bindings);
      if (failed(wiring))
        return failure();
      if (!witness && (*wiring)->states.empty() &&
          (*wiring)->incoming.empty() && (*wiring)->writers.empty())
        return module->emitOpError() << "empty transaction_wiring has no "
                                        "sampled/check/internal-port witness";
      entry.wiring = *wiring;
    }
    size_t position = occurrences.size();
    occurrences.push_back(std::move(entry));
    for (auto instance : module.getBody().front().getOps<InstanceOp>()) {
      auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
      if (failed(budget.debit(1, instance)))
        return failure();
      if (!child || !relevant.contains(child.operator->()))
        continue;
      auto nested = childScope(scope, instance, budget);
      if (failed(nested) || failed(expand(*nested, position, instance)))
        return failure();
    }
    return success();
  }
  FailureOr<HardwareIdentityScope>
  actualStateScope(const Occurrence &entry,
                   const PhysicalStateEndpoint &relative) {
    auto scope = copyScope(entry.scope, budget, entry.module);
    if (failed(scope) ||
        failed(charge(budget, entry.module, relative.ownerScope.path.size(),
                      sizeof(InstanceOp))))
      return failure();
    scope->path.append(relative.ownerScope.path);
    return std::move(*scope);
  }
  FailureOr<size_t> target(const Occurrence &entry, DictionaryAttr id) {
    size_t entryOrdinal = &entry - occurrences.data();
    if (failed(budget.debit(1, entry.module)))
      return failure();
    std::pair<size_t, Attribute> key{entryOrdinal, id};
    auto proven = stateTargetProofs.find(key);
    if (proven != stateTargetProofs.end())
      return proven->second;
    auto relative = resolveTransactionState(entry.module, id, analysis,
                                            entry.bindings, budget);
    if (failed(relative))
      return failure();
    auto scope = actualStateScope(entry, *relative);
    if (failed(scope))
      return failure();
    auto occurrence = occurrenceFor(*scope);
    if (failed(occurrence) || failed(budget.debit(1, entry.module)))
      return failure();
    auto found =
        stateIndex.find({*occurrence, relative->storage.getOperation()});
    if (found != stateIndex.end()) {
      const auto &state = states[found->second];
      if (failed(payScope(state.scope, budget, entry.module)) ||
          failed(payScope(*scope, budget, entry.module)))
        return failure();
      if (state.storage == relative->storage &&
          sameScope(state.scope, *scope)) {
        if (failed(charge(budget, entry.module, 1,
                          sizeof(std::pair<decltype(key), size_t>) + 64)))
          return failure();
        stateTargetProofs.try_emplace(key, found->second);
        return found->second;
      }
    }
    return entry.module->emitOpError()
           << "transaction route target has no actual inventoried owner";
  }
  LogicalResult boundary(Value value, const HardwareIdentityScope &scope,
                         SmallVectorImpl<HardwareIdentityEndpoint> &out) {
    if (failed(charge(budget, valueSite(value), 1,
                      sizeof(HardwareIdentityEndpoint))))
      return failure();
    auto endpoint = makeIdentityEndpoint(value, scope, analysis, budget);
    if (failed(endpoint))
      return failure();
    out.push_back(std::move(*endpoint));
    return success();
  }
  LogicalResult inventory() {
    for (auto [occurrenceOrdinal, entry] : llvm::enumerate(occurrences)) {
      auto prepared = validation.prepare(entry.module, entry.bindings);
      if (failed(prepared))
        return failure();
      if (entry.wiring) {
        if (!*prepared)
          return entry.module->emitOpError()
                 << "transaction inventory lacks prepared boundaries";
        for (const auto &record : entry.wiring->states) {
          if (failed(charge(budget, entry.module, 1, sizeof(State))))
            return failure();
          auto scope = copyScope(entry.scope, budget, entry.module);
          if (failed(scope) || failed(budget.debit(1, entry.module)))
            return failure();
          auto payload =
              analysis.resolveType(record.storage->getResult(0).getType(),
                                   entry.bindings, entry.module);
          if (failed(payload))
            return failure();
          states.push_back({std::move(*scope),
                            record.storage,
                            *payload,
                            {},
                            {},
                            {},
                            {},
                            {},
                            false});
          std::pair<size_t, Operation *> key{occurrenceOrdinal, record.storage};
          if (stateIndex.contains(key))
            return record.storage->emitOpError()
                   << "duplicate state terminal in actual occurrence";
          if (failed(charge(budget, record.storage, 1,
                            sizeof(std::pair<decltype(key), size_t>) + 64)))
            return failure();
          stateIndex.try_emplace(key, states.size() - 1);
        }
        for (const auto &writer : entry.wiring->writers) {
          for (uint64_t ordinal = writer.ordinaryResults;
               ordinal < writer.rule->getNumResults(); ++ordinal)
            if (failed(boundary(writer.rule->getResult(ordinal), entry.scope,
                                stops)))
              return failure();
        }
      }
      // Non-sampled check suffixes are also immutable identity boundaries.
      for (auto rule : entry.module->getRegion(0).front().getOps<RuleOp>()) {
        if (rule->hasAttr("ac.sampled_admission"))
          continue;
        auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
        if (!required)
          continue;
        if (required.size() > rule->getNumResults() / 2)
          return rule->emitOpError() << "check suffix is outside Rule results";
        uint64_t first = rule->getNumResults() - 2 * required.size();
        for (uint64_t ordinal = first; ordinal < rule->getNumResults();
             ++ordinal)
          if (failed(boundary(rule->getResult(ordinal), entry.scope, stops)))
            return failure();
      }
    }
    return success();
  }
  HardwareIdentityStopPolicy policy() const {
    return {HardwareIdentityTraversal::OrdinaryInstances, stops};
  }
  FailureOr<HardwareIdentityEndpoint>
  normal(Value value, const HardwareIdentityScope &scope) {
    if (!identity) {
      if (failed(charge(budget, valueSite(value), 1, sizeof(IdentityMemo))))
        return failure();
      for (const auto &stop : stops)
        if (failed(payScope(stop.scope, budget, valueSite(value))))
          return failure();
      identity.emplace(analysis, budget, policy());
    }
    return identity->normalize(value, scope);
  }
  LogicalResult typeMatches(const HardwareIdentityEndpoint &endpoint, Type type,
                            Operation *site) {
    if (!areEquivalentHardwareTypes(endpoint.resolvedType, type))
      return site->emitOpError()
             << "transaction candidate/route payload differs from actual state";
    return success();
  }
  LogicalResult privateRoute(Value value, const HardwareIdentityScope &scope,
                             size_t state, PrivateRoute::Kind kind,
                             Operation *terminal = nullptr,
                             unsigned operand = 0) {
    if (failed(charge(budget, valueSite(value), 1, sizeof(PrivateRoute))))
      return failure();
    auto endpoint = makeIdentityEndpoint(value, scope, analysis, budget);
    if (failed(endpoint))
      return failure();
    privateRoutes.push_back(
        {std::move(*endpoint), state, kind, terminal, operand});
    return success();
  }
  LogicalResult effectDomain(const Occurrence &entry,
                             const ResolvedSourceEffectOwner &logical,
                             const HardwareIdentityEndpoint &clock,
                             const HardwareIdentityEndpoint &reset,
                             Operation *terminal) {
    auto receiver = copyScope(entry.scope, budget, terminal);
    if (failed(receiver) || failed(charge(budget, terminal, logical.path.size(),
                                          sizeof(InstanceOp))))
      return failure();
    receiver->path.append(logical.path.begin(), logical.path.end());
    auto domain =
        logical.terminal->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
    auto clk = domain ? index(domain, "clock", terminal)
                      : FailureOr<int64_t>(failure());
    auto rst = domain ? index(domain, "reset", terminal)
                      : FailureOr<int64_t>(failure());
    if (failed(clk) || failed(rst) ||
        !logical.terminal->getRegion(0).hasOneBlock() ||
        *clk >= logical.terminal->getRegion(0).front().getNumArguments() ||
        *rst >= logical.terminal->getRegion(0).front().getNumArguments())
      return terminal->emitOpError() << "sampled effect logical owner requires "
                                        "declared clock/reset domain";
    auto receiverClock = normal(
        logical.terminal->getRegion(0).front().getArgument(*clk), *receiver);
    auto receiverReset = normal(
        logical.terminal->getRegion(0).front().getArgument(*rst), *receiver);
    if (failed(receiverClock) || failed(receiverReset))
      return failure();
    if (!sameEndpoint(*receiverClock, clock) ||
        !sameEndpoint(*receiverReset, reset))
      return terminal->emitOpError()
             << "sampled effect receiver domain differs from actual writer "
                "physical clock/reset";
    return success();
  }
  FailureOr<EffectTerminalIndex *> effectTerminals(const Occurrence &entry) {
    for (auto &index : effectProofCache.terminals) {
      if (failed(budget.debit(1, entry.module)))
        return failure();
      if (index.definition != entry.module)
        continue;
      if (failed(budget.debit(SourceCheckBudget::add(
                                  SourceCheckBudget::bindings(index.bindings),
                                  SourceCheckBudget::bindings(entry.bindings)),
                              entry.module)))
        return failure();
      if (sameHardwareBindings(index.bindings, entry.bindings))
        return &index;
    }
    if (failed(charge(budget, entry.module, 1, sizeof(EffectTerminalIndex))) ||
        failed(budget.debit(SourceCheckBudget::bindings(entry.bindings),
                            entry.module)))
      return failure();
    EffectTerminalIndex index{entry.module, entry.bindings, {}, {}};
    for (auto &operation : entry.module->getRegion(0).front()) {
      if (failed(budget.debit(1, &operation)))
        return failure();
      if (auto expect = dyn_cast<SourceExpectOp>(operation)) {
        auto id = expect->getAttrOfType<DictionaryAttr>("ac.check_id");
        if (!id)
          continue;
        if (failed(payAttribute(id, budget, expect)) ||
            failed(verifyCheckID(id, [&] { return expect->emitOpError(); })) ||
            failed(
                verifySourceExpectOperation(analysis, expect, entry.bindings)))
          return failure();
        for (const auto &[prior, terminal] : index.checks) {
          if (failed(budget.debit(1, expect)))
            return failure();
          if (prior == id)
            return expect->emitOpError()
                   << "duplicate expect for source CheckID";
        }
        if (failed(charge(budget, expect, 1,
                          sizeof(std::pair<DictionaryAttr, SourceExpectOp>))))
          return failure();
        index.checks.push_back({id, expect});
        continue;
      }
      auto observe = dyn_cast<SourceObserveOp>(operation);
      if (!observe)
        continue;
      auto id = observe->getAttrOfType<DictionaryAttr>("ac.observation_id");
      if (!id)
        continue;
      if (failed(payAttribute(id, budget, observe)) ||
          failed(
              verifyObservationID(id, [&] { return observe->emitOpError(); })))
        return failure();
      for (const auto &[prior, terminal] : index.observations) {
        if (failed(budget.debit(1, observe)))
          return failure();
        if (prior == id)
          return observe->emitOpError()
                 << "duplicate terminal for source ObservationID";
      }
      if (failed(charge(budget, observe, 1,
                        sizeof(std::pair<DictionaryAttr, SourceObserveOp>))))
        return failure();
      index.observations.push_back({id, observe});
    }
    effectProofCache.terminals.push_back(std::move(index));
    return &effectProofCache.terminals.back();
  }
  FailureOr<SourceExpectOp> checkTerminal(EffectTerminalIndex &index,
                                          DictionaryAttr id, Operation *site) {
    for (const auto &[candidate, terminal] : index.checks) {
      if (failed(budget.debit(1, site)))
        return failure();
      if (candidate == id)
        return terminal;
    }
    return site->emitOpError() << "sampled check has no matching expect";
  }
  FailureOr<SourceObserveOp> observationTerminal(EffectTerminalIndex &index,
                                                 DictionaryAttr id,
                                                 Operation *site) {
    for (const auto &[candidate, terminal] : index.observations) {
      if (failed(budget.debit(1, site)))
        return failure();
      if (candidate == id)
        return terminal;
    }
    return site->emitOpError()
           << "sampled observation has no matching terminal";
  }
  FailureOr<const ResolvedSourceEffectOwner *> effectOwner(
      const Occurrence &entry, RuleOp rule, Operation *terminal,
      ArrayAttr invocation,
      llvm::function_ref<FailureOr<ResolvedSourceEffectOwner>()> resolve) {
    for (const auto &proof : effectProofCache.owners) {
      if (failed(budget.debit(1, terminal)))
        return failure();
      if (proof.definition != entry.module || proof.rule != rule ||
          proof.terminal != terminal || proof.invocation != invocation)
        continue;
      if (failed(budget.debit(SourceCheckBudget::add(
                                  SourceCheckBudget::bindings(proof.bindings),
                                  SourceCheckBudget::bindings(entry.bindings)),
                              terminal)))
        return failure();
      if (sameHardwareBindings(proof.bindings, entry.bindings))
        return &proof.owner;
    }
    auto owner = resolve();
    if (failed(owner) ||
        failed(charge(budget, terminal, 1, sizeof(EffectOwnerProof))) ||
        failed(budget.debit(SourceCheckBudget::bindings(entry.bindings),
                            terminal)))
      return failure();
    effectProofCache.owners.push_back({entry.module, entry.bindings, rule,
                                       terminal, invocation,
                                       std::move(*owner)});
    return &effectProofCache.owners.back().owner;
  }
  LogicalResult checkOwner(const Occurrence &entry, DictionaryAttr id,
                           ArrayAttr invocation, DictionaryAttr location,
                           RuleOp rule, Operation *terminal,
                           const HardwareIdentityEndpoint &clock,
                           const HardwareIdentityEndpoint &reset) {
    auto logical = effectOwner(entry, rule, terminal, invocation, [&] {
      return resolveSourceEffectOwner(analysis, entry.module, entry.bindings,
                                      id.getAs<DictionaryAttr>("registration"),
                                      id.getAs<DictionaryAttr>("check"),
                                      invocation, location, terminal,
                                      [&](uint64_t cost, Operation *site) {
                                        return budget.debit(cost, site);
                                      });
    });
    if (failed(logical))
      return failure();
    return effectDomain(entry, **logical, clock, reset, terminal);
  }
  LogicalResult writers() {
    for (const auto &entry : occurrences) {
      if (!entry.wiring)
        continue;
      auto terminals = effectTerminals(entry);
      if (failed(terminals))
        return failure();
      for (const auto &writer : entry.wiring->writers) {
        auto sampled = validation.verify(writer.rule, entry.bindings);
        if (failed(sampled))
          return failure();
        auto clock = normal(sampled->clock, entry.scope);
        auto reset = normal(sampled->reset, entry.scope);
        if (failed(clock) || failed(reset))
          return failure();
        if (writer.ordinaryResults) {
          using StaticPathFact =
              std::pair<std::optional<bool>, std::optional<bool>>;
          SmallVector<StaticPathFact, 0> facts;
          if (sampled->controls.size() > facts.max_size() ||
              failed(charge(budget, writer.rule, sampled->controls.size(),
                            sizeof(StaticPathFact))))
            return failure();
          facts.reserve(sampled->controls.size());
          auto conjunction =
              [](std::optional<bool> lhs,
                 std::optional<bool> rhs) -> std::optional<bool> {
            if ((lhs && !*lhs) || (rhs && !*rhs))
              return false;
            if (lhs && rhs)
              return true;
            return std::nullopt;
          };
          std::optional<bool> prefix = true;
          for (const auto &control : sampled->controls) {
            auto raw = normal(control.raw, entry.scope);
            if (failed(raw))
              return failure();
            auto known = literal(*raw, analysis, budget);
            if (failed(known))
              return failure();
            std::optional<bool> path = true;
            if (auto selection = control.executionPath.selectionControl) {
              const auto &parent = facts[*selection];
              std::optional<bool> arm;
              if (parent.first)
                arm = *parent.first == control.executionPath.whenTrue;
              path = conjunction(parent.second, arm);
            }
            std::optional<bool> accepted = *known;
            if (control.kind == SampledControlKind::BranchSelect)
              accepted =
                  known->has_value() ? std::optional<bool>(true) : std::nullopt;
            std::optional<bool> bypass;
            if ((path && !*path) || (accepted && *accepted))
              bypass = true;
            else if (path && *path)
              bypass = accepted;
            prefix = conjunction(prefix, bypass);
            facts.push_back({*known, path});
          }
          if (!prefix || !*prefix)
            return writer.rule->emitOpError()
                   << "ordinary transaction results require structured final "
                      "admission proven true";
        }
        if (writer.candidates.empty()) {
          auto domain =
              entry.module->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
          auto clk = domain ? index(domain, "clock", entry.module)
                            : FailureOr<int64_t>(failure());
          auto rst = domain ? index(domain, "reset", entry.module)
                            : FailureOr<int64_t>(failure());
          if (failed(clk) || failed(rst) ||
              *clk >= entry.module->getRegion(0).front().getNumArguments() ||
              *rst >= entry.module->getRegion(0).front().getNumArguments())
            return entry.module->emitOpError()
                   << "statefree transaction requires its module domain_inputs "
                      "clock/reset";
          auto physicalClock =
              normal(entry.module->getRegion(0).front().getArgument(*clk),
                     entry.scope);
          auto physicalReset =
              normal(entry.module->getRegion(0).front().getArgument(*rst),
                     entry.scope);
          if (failed(physicalClock) || failed(physicalReset))
            return failure();
          if (!sameEndpoint(*clock, *physicalClock) ||
              !sameEndpoint(*reset, *physicalReset))
            return writer.rule->emitOpError()
                   << "statefree sampled pins differ from its owning module "
                      "domain";
        }
        for (const auto &candidate : writer.candidates) {
          auto stateIndex = target(entry, candidate.stateID);
          auto data = normal(candidate.data, entry.scope);
          auto enable = normal(candidate.enable, entry.scope);
          if (failed(stateIndex) || failed(data) || failed(enable))
            return failure();
          auto &state = states[*stateIndex];
          if (state.writer)
            return writer.rule->emitOpError()
                   << "multiple outer transaction writers for one actual owner";
          if (failed(typeMatches(*data, state.payload, writer.rule)))
            return failure();
          if (failed(payScope(data->scope, budget, writer.rule)) ||
              failed(payScope(enable->scope, budget, writer.rule)) ||
              failed(payScope(clock->scope, budget, writer.rule)) ||
              failed(payScope(reset->scope, budget, writer.rule)))
            return failure();
          state.writer = writer.rule;
          state.data = *data;
          state.enable = *enable;
          state.clock = *clock;
          state.reset = *reset;
          if (failed(privateRoute(candidate.data, entry.scope, *stateIndex,
                                  PrivateRoute::Data)) ||
              failed(privateRoute(candidate.enable, entry.scope, *stateIndex,
                                  PrivateRoute::Enable)))
            return failure();
        }
        for (const auto &guard : sampled->controls) {
          auto found = checkTerminal(**terminals, guard.id, writer.rule);
          if (failed(found))
            return failure();
          SourceExpectOp match = *found;
          auto condition = normal(match.getCondition(), entry.scope);
          auto reached = normal(match.getPath(), entry.scope);
          auto expectedCondition =
              normal(writer.rule->getResult(guard.rawResult), entry.scope);
          auto expectedReached =
              normal(writer.rule->getResult(guard.reachedResult), entry.scope);
          auto required =
              writer.rule->getAttrOfType<ArrayAttr>("ac.required_checks");
          auto record = cast<DictionaryAttr>(
              required[(guard.rawResult - sampled->checkBase) / 2]);
          if (failed(condition) || failed(reached) ||
              failed(expectedCondition) || failed(expectedReached))
            return failure();
          if (match.getKind() != "guard_known" ||
              match.getLocationAttr() !=
                  record.getAs<DictionaryAttr>("location") ||
              !sameEndpoint(*condition, *expectedCondition) ||
              !sameEndpoint(*reached, *expectedReached) ||
              match.getSampling().size() != 2)
            return match->emitOpError()
                   << "sampled expect differs from corresponding guard "
                      "suffix/kind/span";
          auto sampleClock = normal(match.getSampling()[0], entry.scope);
          auto sampleReset = normal(match.getSampling()[1], entry.scope);
          if (failed(sampleClock) || failed(sampleReset))
            return failure();
          if (!sameEndpoint(*sampleClock, *clock) ||
              !sameEndpoint(*sampleReset, *reset))
            return match->emitOpError()
                   << "sampled expect pins differ from owning Rule captures";
          if (failed(checkOwner(entry, guard.id, guard.invocation,
                                match.getLocationAttr(), writer.rule, match,
                                *clock, *reset)))
            return failure();
          if (failed(privateRoute(writer.rule->getResult(guard.rawResult),
                                  entry.scope, 0, PrivateRoute::EffectOperand,
                                  match, 0)) ||
              failed(privateRoute(writer.rule->getResult(guard.reachedResult),
                                  entry.scope, 0, PrivateRoute::EffectOperand,
                                  match, 1)))
            return failure();
        }
        for (const auto &body : sampled->bodyChecks) {
          auto found = checkTerminal(**terminals, body.id, writer.rule);
          if (failed(found))
            return failure();
          SourceExpectOp match = *found;
          auto condition = normal(match.getCondition(), entry.scope);
          auto path = normal(match.getPath(), entry.scope);
          auto expectedCondition =
              normal(writer.rule->getResult(body.conditionResult), entry.scope);
          auto expectedPath =
              normal(writer.rule->getResult(body.pathResult), entry.scope);
          auto required =
              writer.rule->getAttrOfType<ArrayAttr>("ac.required_checks");
          auto record = cast<DictionaryAttr>(
              required[(body.conditionResult - sampled->checkBase) / 2]);
          if (failed(condition) || failed(path) || failed(expectedCondition) ||
              failed(expectedPath))
            return failure();
          if (match.getKind() != "assert" || !match.getSampling().empty() ||
              match.getLocationAttr() !=
                  record.getAs<DictionaryAttr>("location") ||
              !sameEndpoint(*condition, *expectedCondition) ||
              !sameEndpoint(*path, *expectedPath))
            return match->emitOpError() << "sampled body assertion differs "
                                           "from designated suffix/kind/span";
          if (failed(checkOwner(entry, body.id, body.invocation,
                                match.getLocationAttr(), writer.rule, match,
                                *clock, *reset)) ||
              failed(privateRoute(writer.rule->getResult(body.conditionResult),
                                  entry.scope, 0, PrivateRoute::EffectOperand,
                                  match, 0)) ||
              failed(privateRoute(writer.rule->getResult(body.pathResult),
                                  entry.scope, 0, PrivateRoute::EffectOperand,
                                  match, 1)))
            return failure();
        }
        for (const auto &observation : sampled->observations) {
          auto found =
              observationTerminal(**terminals, observation.id, writer.rule);
          if (failed(found))
            return failure();
          SourceObserveOp match = *found;
          auto logical =
              effectOwner(entry, writer.rule, match,
                          observation.id.getAs<ArrayAttr>("invocation"), [&] {
                            return resolveObservationOwner(
                                analysis, match, entry.bindings,
                                [&](uint64_t cost, Operation *site) {
                                  return budget.debit(cost, site);
                                });
                          });
          if (failed(logical) ||
              failed(effectDomain(entry, **logical, *clock, *reset, match)))
            return failure();
          if (match->getNumOperands() != observation.valueCount + 1)
            return match->emitOpError()
                   << "sampled observation value count differs from inventory";
          for (uint64_t ordinal = 0; ordinal <= observation.valueCount;
               ++ordinal) {
            uint64_t slot = observation.pathResult + ordinal;
            auto actual = normal(match->getOperand(ordinal), entry.scope);
            auto expected = normal(writer.rule->getResult(slot), entry.scope);
            if (failed(actual) || failed(expected))
              return failure();
            if (!sameEndpoint(*actual, *expected))
              return match->emitOpError() << "sampled observation must consume "
                                             "its designated operand slot";
            if (failed(privateRoute(writer.rule->getResult(slot), entry.scope,
                                    0, PrivateRoute::EffectOperand, match,
                                    ordinal)))
              return failure();
          }
        }
        for (const auto &[id, observe] : (**terminals).observations) {
          if (failed(payComparison(id.get("registration"), writer.registration,
                                   budget, observe)))
            return failure();
          if (!identical(id.get("registration"), writer.registration))
            continue;
          bool present = false;
          for (const auto &observation : sampled->observations) {
            if (failed(payComparison(id, observation.id, budget, observe)))
              return failure();
            present |= identical(id, observation.id);
          }
          if (!present)
            return observe->emitOpError()
                   << "observation claims sampled registration absent from "
                      "inventory";
        }
      }
    }
    return success();
  }
  LogicalResult externalInputs() {
    if (occurrences.empty() || !occurrences.front().wiring)
      return success();
    const auto &entry = occurrences.front();
    for (const auto &incoming : entry.wiring->incoming) {
      auto stateIndex = target(entry, incoming.stateID);
      auto data = normal(incoming.data, entry.scope);
      auto enable = normal(incoming.enable, entry.scope);
      if (failed(stateIndex) || failed(data) || failed(enable))
        return failure();
      auto &state = states[*stateIndex];
      if (state.writer || state.external)
        return entry.module->emitOpError()
               << "external incoming and local/descendant writer conflict for "
                  "actual owner";
      if (failed(typeMatches(*data, state.payload, entry.module)) ||
          failed(payScope(data->scope, budget, entry.module)) ||
          failed(payScope(enable->scope, budget, entry.module)))
        return failure();
      state.external = true;
      state.data = *data;
      state.enable = *enable;
    }
    return success();
  }
  FailureOr<bool> declaredIncomingHold(Value dataValue, Value enableValue,
                                       const HardwareIdentityScope &scope,
                                       size_t stateIndex) {
    auto ownerIndex = occurrenceFor(scope);
    if (failed(ownerIndex))
      return failure();
    const auto &owner = occurrences[*ownerIndex];
    if (!owner.wiring)
      return false;
    HardwareIdentityStopPolicy local{HardwareIdentityTraversal::DefinitionLocal,
                                     stops};
    auto data = normalized(dataValue, scope, local, analysis, budget);
    auto enable = normalized(enableValue, scope, local, analysis, budget);
    if (failed(data) || failed(enable))
      return failure();
    for (const auto &incoming : owner.wiring->incoming) {
      auto targetIndex = target(owner, incoming.stateID);
      if (failed(targetIndex))
        return failure();
      if (*targetIndex != stateIndex)
        continue;
      auto expectedData =
          normalized(incoming.data, scope, local, analysis, budget);
      auto expectedEnable =
          normalized(incoming.enable, scope, local, analysis, budget);
      if (failed(expectedData) || failed(expectedEnable))
        return failure();
      if (sameEndpoint(*data, *expectedData) &&
          sameEndpoint(*enable, *expectedEnable))
        return true;
    }
    return false;
  }
  LogicalResult samePair(Value dataValue, Value enableValue,
                         const HardwareIdentityScope &scope, size_t stateIndex,
                         bool storage, Operation *site) {
    auto data = normal(dataValue, scope);
    auto enable = normal(enableValue, scope);
    if (failed(data) || failed(enable))
      return failure();
    auto &state = states[stateIndex];
    if (failed(typeMatches(*data, state.payload, site)))
      return failure();
    // Appended child ports may remain an unused zero pair while a descendant
    // Rule owns its state. Such a pair is never counted as a writer.
    auto enLiteral = literal(*enable, analysis, budget);
    if (failed(enLiteral))
      return failure();
    if (enLiteral->has_value() && !**enLiteral) {
      if (storage && (state.writer || state.external))
        return site->emitOpError()
               << "transaction writer has a missing live D/E route";
      if (storage) {
        auto q = normal(state.storage->getResult(0), state.scope);
        if (failed(q))
          return failure();
        if (!sameEndpoint(*data, *q)) {
          auto dataLiteral = literal(*data, analysis, budget);
          if (failed(dataLiteral))
            return failure();
          auto incomingHold =
              declaredIncomingHold(dataValue, enableValue, scope, stateIndex);
          if (failed(incomingHold))
            return failure();
          if (!dataLiteral->has_value() || **dataLiteral || !*incomingHold)
            return site->emitOpError()
                   << "unwritten hold requires local D=Q or declared unused "
                      "incoming D=zero, with E=zero";
        }
      } else {
        auto dataLiteral = literal(*data, analysis, budget);
        if (failed(dataLiteral))
          return failure();
        if (!dataLiteral->has_value() || **dataLiteral)
          return site->emitOpError()
                 << "unused external route requires D=zero and E=zero";
      }
      return success();
    }
    if (!state.data || !state.enable || !sameEndpoint(*data, *state.data) ||
        !sameEndpoint(*enable, *state.enable))
      return site->emitOpError()
             << "transaction D/E origin differs from corresponding "
                "candidate D/E or incoming pair";
    return success();
  }
  LogicalResult connections() {
    for (const auto &entry : occurrences) {
      if (!entry.wiring)
        continue;
      for (const auto &incoming : entry.wiring->incoming) {
        auto stateIndex = target(entry, incoming.stateID);
        auto endpoint = normal(incoming.data, entry.scope);
        if (failed(stateIndex) || failed(endpoint) ||
            failed(typeMatches(*endpoint, states[*stateIndex].payload,
                               entry.module)))
          return failure();
        if (entry.parent) {
          const auto &parent = occurrences[*entry.parent];
          if (incoming.enableInput >= entry.allocation->getNumOperands())
            return entry.allocation->emitOpError()
                   << "child incoming ordinal is outside actual operands";
          if (failed(
                  samePair(entry.allocation->getOperand(incoming.dataInput),
                           entry.allocation->getOperand(incoming.enableInput),
                           parent.scope, *stateIndex, false, entry.allocation)))
            return failure();
        }
        if (failed(privateRoute(incoming.data, entry.scope, *stateIndex,
                                PrivateRoute::Data)) ||
            failed(privateRoute(incoming.enable, entry.scope, *stateIndex,
                                PrivateRoute::Enable)))
          return failure();
      }
    }
    for (auto [ordinal, state] : llvm::enumerate(states)) {
      if (failed(samePair(state.storage->getOperand(3),
                          state.storage->getOperand(2), state.scope, ordinal,
                          true, state.storage)))
        return failure();
      if (state.writer) {
        auto clock = normal(state.storage->getOperand(0), state.scope);
        auto reset = normal(state.storage->getOperand(1), state.scope);
        if (failed(clock) || failed(reset))
          return failure();
        if (!sameEndpoint(*clock, *state.clock) ||
            !sameEndpoint(*reset, *state.reset))
          return state.storage->emitOpError()
                 << "transaction sampled domain differs from target DFFE "
                    "physical pins";
      }
    }
    return success();
  }
  bool stateTerminal(const HardwareIdentityScope &scope, InstanceOp instance,
                     size_t expected) const {
    return expected < states.size() && states[expected].storage == instance &&
           sameScope(states[expected].scope, scope);
  }
  FailureOr<bool> enableTarget(const PrivateRoute &route, size_t candidate) {
    if (route.state == candidate)
      return true;
    if (candidate >= states.size() || !states[candidate].enable)
      return false;
    auto origin = normal(route.endpoint.value, route.endpoint.scope);
    if (failed(origin))
      return failure();
    return sameEndpoint(*origin, *states[candidate].enable);
  }
  FailureOr<size_t> occurrenceFor(const HardwareIdentityScope &scope) {
    if (occurrences.empty())
      return scope.root->emitOpError()
             << "private route has no actual module occurrences";
    const auto &root = occurrences.front().scope;
    // Every scope reaching this private helper was created from this
    // WiringProof's already-paid exact root bindings (copyScope/childScope).
    // Recheck the root handle and exact path here without rescanning the same
    // immutable bindings for every SSA use.
    if (failed(budget.debit(1, scope.root)))
      return failure();
    if (root.root != scope.root)
      return scope.root->emitOpError()
             << "private route scope differs from its proof root";
    if (failed(budget.debit(SourceCheckBudget::add(1, scope.path.size()),
                            scope.root)))
      return failure();
    auto found = occurrenceIndex.find(occurrenceHash(scope));
    if (found != occurrenceIndex.end())
      for (size_t ordinal : found->second) {
        const auto &path = occurrences[ordinal].scope.path;
        if (failed(
                budget.debit(SourceCheckBudget::add(
                                 1, std::min(path.size(), scope.path.size())),
                             scope.root)))
          return failure();
        if (path == scope.path)
          return ordinal;
      }
    return scope.root->emitOpError()
           << "private route has no actual module occurrence";
  }
  LogicalResult visitPrivate(const HardwareIdentityEndpoint &endpoint,
                             const PrivateRoute &route,
                             std::set<RouteVisitKey> &seen,
                             std::optional<size_t> knownOccurrence = {}) {
    Operation *site = valueSite(endpoint.value);
    if (failed(budget.debit(1, site)))
      return failure();
    if (endpoint.value.use_empty()) {
      return site->emitOpError()
             << "private transaction/check route has a dead end before its "
                "corresponding D/E or expect";
    }
    FailureOr<size_t> resolvedOccurrence =
        knownOccurrence ? FailureOr<size_t>(*knownOccurrence)
                        : occurrenceFor(endpoint.scope);
    if (failed(resolvedOccurrence))
      return failure();
    size_t currentOccurrence = *resolvedOccurrence;
    RouteVisitKey key{endpoint.value.getAsOpaquePointer(),
                      endpoint.resolvedType.getAsOpaquePointer(),
                      currentOccurrence};
    uint64_t comparisons = 1;
    for (uint64_t nodes = seen.size() + 1; nodes > 1; nodes = (nodes + 1) / 2)
      comparisons = SourceCheckBudget::add(comparisons, 2);
    if (failed(budget.debit(comparisons, site)))
      return failure();
    auto insertion = seen.lower_bound(key);
    if (insertion != seen.end() && *insertion == key)
      return success();
    if (failed(charge(budget, site, 1,
                      sizeof(RouteVisitKey) + 4 * sizeof(void *))))
      return failure();
    seen.emplace_hint(insertion, std::move(key));
    const auto &owner = occurrences[currentOccurrence];
    for (auto &use : endpoint.value.getUses()) {
      if (failed(budget.debit(1, use.getOwner())))
        return failure();
      Operation *consumer = use.getOwner();
      unsigned operand = use.getOperandNumber();
      if (auto extract = dyn_cast<BitsExtractOp>(consumer)) {
        auto next = makeIdentityEndpoint(extract->getResult(0), endpoint.scope,
                                         analysis, budget);
        if (failed(next))
          return failure();
        auto canonical = normal(extract->getResult(0), endpoint.scope);
        auto original = normal(endpoint.value, endpoint.scope);
        if (failed(canonical) || failed(original))
          return failure();
        if (!sameEndpoint(*canonical, *original))
          return extract->emitOpError() << "private transaction value escapes "
                                           "through nonidentity extract";
        if (failed(visitPrivate(*next, route, seen, currentOccurrence)))
          return failure();
        continue;
      }
      if (isa<SourceExpectOp, SourceObserveOp>(consumer)) {
        if (route.kind == PrivateRoute::EffectOperand &&
            consumer == route.terminal && operand == route.operand)
          continue;
        return consumer->emitOpError()
               << "private transaction/effect value escapes to unrelated "
                  "terminal operand";
      }
      if (auto instance = dyn_cast<InstanceOp>(consumer)) {
        if (route.kind == PrivateRoute::EffectOperand)
          return instance->emitOpError()
                 << "check suffix escapes its corresponding expect";
        if (analysis.getPrimitiveKind(analysis.resolveCallee(instance)) ==
            "dffe") {
          if (route.kind == PrivateRoute::Data && operand == 3 &&
              stateTerminal(endpoint.scope, instance, route.state))
            continue;
          if (route.kind == PrivateRoute::Enable && operand == 2) {
            if (failed(budget.debit(1, instance)))
              return failure();
            auto found = stateIndex.find({currentOccurrence, instance});
            if (found != stateIndex.end()) {
              size_t ordinal = found->second;
              if (failed(payScope(states[ordinal].scope, budget, instance)) ||
                  failed(payScope(endpoint.scope, budget, instance)))
                return failure();
              if (!stateTerminal(endpoint.scope, instance, ordinal))
                return instance->emitOpError()
                       << "state terminal index differs from actual scope";
              auto allowed = enableTarget(route, ordinal);
              if (failed(allowed))
                return failure();
              if (*allowed)
                continue;
            }
          }
          return instance->emitOpError() << "private candidate D/E reaches "
                                            "unrelated primitive pin or owner";
        }
        auto child =
            dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
        if (!child)
          return instance->emitOpError() << "private transaction value reaches "
                                            "undeclared primitive/import pin";
        auto nested = childScope(endpoint.scope, instance, budget);
        if (failed(nested))
          return failure();
        auto childIndex = occurrenceFor(*nested);
        if (failed(childIndex))
          return failure();
        const auto &childEntry = occurrences[*childIndex];
        if (!childEntry.wiring)
          return instance->emitOpError()
                 << "private transaction route requires declared child "
                    "incoming pair";
        const TransactionIncomingView *matched = nullptr;
        for (const auto &incoming : childEntry.wiring->incoming)
          if ((route.kind == PrivateRoute::Data &&
               incoming.dataInput == operand) ||
              (route.kind == PrivateRoute::Enable &&
               incoming.enableInput == operand)) {
            matched = &incoming;
            break;
          }
        if (!matched)
          return instance->emitOpError()
                 << "private transaction value escapes through "
                    "public/unrelated child input";
        auto targetIndex = target(childEntry, matched->stateID);
        if (failed(targetIndex))
          return failure();
        bool allowed = *targetIndex == route.state;
        if (route.kind == PrivateRoute::Enable) {
          auto admitted = enableTarget(route, *targetIndex);
          if (failed(admitted))
            return failure();
          allowed = *admitted;
        }
        if (!allowed)
          return instance->emitOpError() << "private transaction route is "
                                            "connected to wrong state owner";
        Value next = route.kind == PrivateRoute::Data ? Value(matched->data)
                                                      : Value(matched->enable);
        auto nextEndpoint =
            makeIdentityEndpoint(next, *nested, analysis, budget);
        if (failed(nextEndpoint) ||
            failed(visitPrivate(*nextEndpoint, route, seen, *childIndex)))
          return failure();
        continue;
      }
      if (auto yielded = dyn_cast<YieldOp>(consumer)) {
        if (route.kind == PrivateRoute::EffectOperand ||
            yielded->getParentOp() != owner.module.operator->() ||
            !owner.wiring || operand < owner.wiring->ports.sourceOutputCount ||
            !owner.parent || operand >= owner.allocation->getNumResults())
          return yielded->emitOpError()
                 << "private transaction value escapes through "
                    "public/unterminated Module or Rule output";
        const auto &parent = occurrences[*owner.parent];
        auto next = makeIdentityEndpoint(owner.allocation->getResult(operand),
                                         parent.scope, analysis, budget);
        if (failed(next) ||
            failed(visitPrivate(*next, route, seen, *owner.parent)))
          return failure();
        continue;
      }
      return consumer->emitOpError()
             << "private transaction value escapes through unrelated Rule "
                "capture or nonidentity operation";
    }
    return success();
  }
  LogicalResult privateUses() {
    for (const auto &route : privateRoutes) {
      std::set<RouteVisitKey> seen;
      if (failed(visitPrivate(route.endpoint, route, seen)))
        return failure();
    }
    return success();
  }
  const HardwareAnalysis &analysis;
  SourceCheckBudget &budget;
  SampledAdmissionValidation &validation;
  const llvm::DenseSet<Operation *> &relevant;
  SmallVector<Occurrence, 0> occurrences;
  SmallVector<State, 0> states;
  SmallVector<HardwareIdentityEndpoint> stops;
  SmallVector<PrivateRoute> privateRoutes;
  llvm::DenseMap<uint64_t, SmallVector<size_t>> occurrenceIndex;
  llvm::DenseMap<std::pair<size_t, Operation *>, size_t> stateIndex;
  llvm::DenseMap<std::pair<size_t, Attribute>, size_t> stateTargetProofs;
  EffectProofCache &effectProofCache;
  std::optional<IdentityMemo> identity;
};
bool localWitness(ModuleOp module) {
  if (module->hasAttr("ac.sampled_admission") ||
      module->hasAttr("ac.transaction_wiring") ||
      module->hasAttr("ac.internal_ports"))
    return true;
  for (auto &operation : module.getBody().front()) {
    if (operation.hasAttr("ac.sampled_admission") ||
        operation.hasAttr("ac.transaction_wiring"))
      return true;
    if (auto expect = dyn_cast<SourceExpectOp>(operation))
      if (expect.getKind() == "guard_known")
        return true;
    if (auto required =
            operation.getAttrOfType<ArrayAttr>("ac.required_checks"))
      for (auto raw : required) {
        auto row = dyn_cast<DictionaryAttr>(raw);
        auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
        if (kind && kind.getValue() == "guard_known")
          return true;
      }
  }
  return false;
}
} // namespace
LogicalResult
verifyTransactionWiring(const HardwareAnalysis &analysis,
                        ArrayRef<std::pair<ModuleOp, HardwareBindings>> scopes,
                        SampledAdmissionValidation &validation,
                        SourceCheckBudget &budget) {
  if (failed(validation.verifyInvocation(analysis, budget)))
    return failure();
  // Unrelated ordinary hardware retains its existing analysis limits. This
  // inexpensive witness scan deliberately precedes transaction allocations.
  bool hasWitness = llvm::any_of(scopes, [](const auto &entry) {
    auto module = entry.first;
    return module.getBody().hasOneBlock() && localWitness(module);
  });
  if (!hasWitness)
    return success();
  budget.setPhase("sampled transaction wiring proof");
  llvm::DenseSet<Operation *> relevant, writerRelevant, scanned;
  llvm::DenseMap<Operation *, SmallVector<ModuleOp>> parents;
  SmallVector<ModuleOp> pending, writerPending;
  SmallVector<std::pair<CollectionOp, Operation *>> collections;
  auto mark = [&](ModuleOp module, llvm::DenseSet<Operation *> &selected,
                  SmallVectorImpl<ModuleOp> &worklist) -> LogicalResult {
    if (failed(budget.debit(1, module)))
      return failure();
    if (selected.contains(module.operator->()))
      return success();
    if (failed(charge(budget, module, 1, sizeof(Operation *) + 64)) ||
        failed(charge(budget, module, 1, sizeof(ModuleOp))))
      return failure();
    selected.insert(module.operator->());
    worklist.push_back(module);
    return success();
  };
  // Build reverse ordinary-instance adjacency once from the supplied catalog.
  // Bound roots of one definition share this relation; actual bindings remain
  // distinct in WiringProof. Every scan, set/map operation and allocation pays
  // the same invocation budget before work or mutation.
  for (const auto &entry : scopes) {
    auto module = entry.first;
    if (failed(budget.debit(1, module)))
      return failure();
    if (scanned.contains(module.operator->()))
      continue;
    if (failed(charge(budget, module, 1, sizeof(Operation *) + 64)))
      return failure();
    scanned.insert(module.operator->());
    if (!module.getBody().hasOneBlock())
      return module->emitOpError()
             << "transaction catalog requires one graph block";
    // The initial witness scan performed no allocations; account this catalog
    // scan before using its result to seed the worklist.
    bool hasCheckBoundary = false;
    for (auto &operation : module.getBody().front()) {
      if (failed(budget.debit(1, &operation)) ||
          failed(payAttribute(operation.getAttr("ac.required_checks"), budget,
                              &operation)))
        return failure();
      if (isa<RuleOp>(operation))
        if (auto required =
                operation.getAttrOfType<ArrayAttr>("ac.required_checks"))
          hasCheckBoundary |= !required.empty();
    }
    // Totality identity follows ordinary instances. Their check-only suffixes
    // remain private boundaries without requiring transaction companions.
    if ((localWitness(module) || hasCheckBoundary) &&
        failed(mark(module, relevant, pending)))
      return failure();
    if (module->hasAttr("ac.transaction_wiring")) {
      auto wiring = validation.wiring(module, entry.second);
      if (failed(wiring))
        return failure();
      for (const auto &writer : (*wiring)->writers) {
        if (failed(budget.debit(1, writer.rule)))
          return failure();
        // Ownership records preserve write intent even when fire is known
        // false or the candidate is an identity write. Do not infer absence of
        // a writer from unused results or from lowered data expressions.
        if (!writer.candidates.empty() &&
            failed(mark(module, writerRelevant, writerPending)))
          return failure();
      }
    }
    for (auto collection : module.getBody().front().getOps<CollectionOp>()) {
      if (failed(budget.debit(1, collection)))
        return failure();
      auto *callee = analysis.resolveCallee(collection);
      if (!callee)
        return collection->emitOpError()
               << "transaction collection callee must resolve in the catalog";
      if (failed(charge(budget, collection, 1,
                        sizeof(std::pair<CollectionOp, Operation *>))))
        return failure();
      collections.push_back({collection, callee});
    }
    for (auto instance : module.getBody().front().getOps<InstanceOp>()) {
      if (failed(budget.debit(1, instance)))
        return failure();
      auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
      if (!child)
        continue;
      if (failed(budget.debit(1, instance)))
        return failure();
      if (!parents.contains(child.operator->())) {
        if (failed(charge(budget, instance, 1,
                          sizeof(Operation *) + sizeof(SmallVector<ModuleOp>) +
                              64)))
          return failure();
        parents.try_emplace(child.operator->());
      }
      if (failed(charge(budget, instance, 1, sizeof(ModuleOp))))
        return failure();
      parents.find(child.operator->())->second.push_back(module);
    }
  }
  auto propagate = [&](llvm::DenseSet<Operation *> &selected,
                       SmallVectorImpl<ModuleOp> &worklist) -> LogicalResult {
    while (!worklist.empty()) {
      auto module = worklist.pop_back_val();
      if (failed(budget.debit(1, module)))
        return failure();
      auto found = parents.find(module.operator->());
      if (found == parents.end())
        continue;
      for (auto parent : found->second)
        if (failed(mark(parent, selected, worklist)))
          return failure();
    }
    return success();
  };
  if (failed(propagate(relevant, pending)) ||
      failed(propagate(writerRelevant, writerPending)))
    return failure();
  for (const auto &[collection, callee] : collections) {
    if (failed(budget.debit(2, collection)))
      return failure();
    if (callee->hasAttr("ac.internal_ports"))
      return collection->emitOpError() << "transaction Collection callee "
                                          "cannot have internal routing ports";
    if (writerRelevant.contains(callee))
      return collection->emitOpError()
             << "transaction Collection callee cannot contain a local or "
                "descendant writer";
  }
  SmallVector<const std::pair<ModuleOp, HardwareBindings> *, 0> verifiedRoots;
  EffectProofCache effectProofCache;
  for (const auto &entry : scopes) {
    const auto &[module, bindings] = entry;
    if (failed(budget.debit(1, module)))
      return failure();
    if (!relevant.contains(module.operator->()))
      continue;
    bool verified = false;
    for (const auto *prior : verifiedRoots) {
      if (failed(budget.debit(1, module)))
        return failure();
      if (prior->first != module)
        continue;
      if (failed(budget.debit(SourceCheckBudget::add(
                                  SourceCheckBudget::bindings(bindings),
                                  SourceCheckBudget::bindings(prior->second)),
                              module)))
        return failure();
      if (sameHardwareBindings(prior->second, bindings)) {
        verified = true;
        break;
      }
    }
    if (verified)
      continue;
    WiringProof proof(analysis, budget, relevant, validation, effectProofCache);
    if (failed(proof.run(module, bindings)))
      return failure();
    // Record only completed root proofs; child visits do not prove that child
    // as a root with its own complete occurrence and private-use checks.
    if (failed(charge(budget, module, 1, sizeof(&entry))))
      return failure();
    verifiedRoots.push_back(&entry);
  }
  return success();
}
} // namespace acir::ac::detail

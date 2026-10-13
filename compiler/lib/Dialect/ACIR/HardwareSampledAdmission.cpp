#include "HardwareSampledAdmission.h"
#include "ACIRSourceContracts.h"
#include "HardwareSourceCheckPreflight.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include <algorithm>
#include <limits>
#include <optional>

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
  if (auto array = dyn_cast<ArrayAttr>(attr)) {
    for (auto item : array)
      if (failed(payAttribute(item, budget, site)))
        return failure();
  } else if (auto dictionary = dyn_cast<DictionaryAttr>(attr)) {
    for (auto item : dictionary)
      if (failed(budget.debit(
              SourceCheckBudget::bytes(item.getName().getValue().size()),
              site)) ||
          failed(payAttribute(item.getValue(), budget, site)))
        return failure();
  }
  return success();
}
LogicalResult payComparison(Attribute lhs, Attribute rhs,
                            SourceCheckBudget &budget, Operation *site) {
  if (failed(payAttribute(lhs, budget, site)) ||
      failed(payAttribute(rhs, budget, site)))
    return failure();
  return success();
}
bool identical(Attribute lhs, Attribute rhs) {
  // Missing metadata never establishes logical identity. The owning closed
  // validators below reject malformed non-null attributes before comparison.
  return lhs && rhs && compareClosedSourceStructure(lhs, rhs) == 0;
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
  if (!module || !rule.getBody().hasOneBlock() ||
      failed(exact(row,
                   {"clock_capture", "reset_capture", "fire_result", "guards"},
                   rule, "ac.sampled_admission")))
    return failure();
  if (failed(payAttribute(row, budget, rule)))
    return failure();
  auto clock = index(row, "clock_capture", rule);
  auto reset = index(row, "reset_capture", rule);
  auto fire = index(row, "fire_result", rule);
  auto guards = row.getAs<ArrayAttr>("guards");
  if (rule.getBody().front().empty())
    return rule->emitOpError() << "sampled Rule requires a yield terminator";
  auto yielded = dyn_cast<YieldOp>(rule.getBody().front().getTerminator());
  if (failed(clock) || failed(reset) || failed(fire))
    return failure();
  if (!guards || guards.empty() || !yielded ||
      *clock >= rule->getNumOperands() || *reset >= rule->getNumOperands() ||
      *fire >= rule->getNumResults() ||
      SourceCheckBudget::add(*fire + 1,
                             SourceCheckBudget::multiply(2, guards.size())) !=
          rule->getNumResults() ||
      yielded->getNumOperands() != rule->getNumResults())
    return rule->emitOpError() << "sampled admission captures/guard suffix are "
                                  "outside the exact Rule layout";
  auto required = rule->getAttrOfType<ArrayAttr>("ac.required_checks");
  if (!required || required.size() != guards.size())
    return rule->emitOpError()
           << "sampled admission requires exactly its guard_known obligations";
  if (failed(
          bitOne(rule->getOperand(*clock), analysis, bindings, rule, budget)) ||
      failed(
          bitOne(rule->getOperand(*reset), analysis, bindings, rule, budget)) ||
      failed(bitOne(yielded->getOperand(*fire), analysis, bindings, rule,
                    budget)) ||
      failed(verifyOccurrence(rule->getAttrOfType<DictionaryAttr>("occurrence"),
                              [&] { return rule->emitOpError(); })) ||
      failed(charge(budget, rule, guards.size(), sizeof(SampledGuardView))))
    return failure();
  SampledAdmissionView result;
  result.definition = module;
  result.rule = rule;
  result.clockCapture = *clock;
  result.resetCapture = *reset;
  result.fireResult = *fire;
  result.clock = rule->getOperand(*clock);
  result.reset = rule->getOperand(*reset);
  result.fire = yielded->getOperand(*fire);
  result.guards.reserve(guards.size());
  for (auto [ordinal, raw] : llvm::enumerate(guards)) {
    auto guard = dyn_cast<DictionaryAttr>(raw);
    if (failed(exact(guard, {"id", "invocation", "predecessor"}, rule,
                     "sampled guard")))
      return failure();
    auto id = guard.getAs<DictionaryAttr>("id");
    auto invocation = guard.getAs<ArrayAttr>("invocation");
    auto predecessor = index(guard, "predecessor", rule, true);
    if (failed(predecessor) ||
        failed(verifyCheckID(id, [&] { return rule->emitOpError(); })))
      return failure();
    auto obligation = index(id, "obligation", rule);
    if (failed(obligation))
      return failure();
    if (*predecessor != static_cast<int64_t>(ordinal) - 1 ||
        *obligation != static_cast<int64_t>(ordinal) || !invocation ||
        (ordinal == 0 && !invocation.empty()) ||
        !identical(id.get("registration"),
                   rule->getAttrOfType<DictionaryAttr>("occurrence")))
      return rule->emitOpError() << "sampled guard has inconsistent ordinal, "
                                    "registration or invocation";
    if (failed(charge(budget, rule, invocation.size(), sizeof(DictionaryAttr))))
      return failure();
    for (auto frameRaw : invocation) {
      auto frame = dyn_cast<DictionaryAttr>(frameRaw);
      if (failed(exact(frame, {"call", "receiver"}, rule,
                       "sampled invocation frame")) ||
          failed(verifyOccurrence(frame.getAs<DictionaryAttr>("call"),
                                  [&] { return rule->emitOpError(); })) ||
          failed(verifyOwnerRef(frame.getAs<DictionaryAttr>("receiver"),
                                [&] { return rule->emitOpError(); })))
        return failure();
    }
    for (const auto &prior : result.guards) {
      if (failed(payComparison(prior.id, id, budget, rule)) ||
          failed(payComparison(prior.invocation, invocation, budget, rule)))
        return failure();
      if (identical(prior.id, id) || identical(prior.invocation, invocation))
        return rule->emitOpError()
               << "duplicate sampled guard identity or invocation";
    }
    auto check = dyn_cast<DictionaryAttr>(required[ordinal]);
    if (failed(exact(check, {"id", "kind", "location"}, rule,
                     "guard RequiredCheck")))
      return failure();
    auto kind = check.getAs<StringAttr>("kind");
    if (!kind || kind.getValue() != "guard_known" ||
        !identical(check.get("id"), id) ||
        failed(verifySourceSpan(check.getAs<DictionaryAttr>("location"),
                                [&] { return rule->emitOpError(); })))
      return rule->emitOpError()
             << "sampled guard differs from its ordered guard_known obligation";
    uint64_t predicate = *fire + 1 + 2 * ordinal;
    uint64_t reached = predicate + 1;
    if (failed(bitOne(yielded->getOperand(predicate), analysis, bindings, rule,
                      budget)) ||
        failed(bitOne(yielded->getOperand(reached), analysis, bindings, rule,
                      budget)))
      return failure();
    result.guards.push_back({id, invocation, *predecessor, predicate, reached,
                             yielded->getOperand(predicate),
                             yielded->getOperand(reached)});
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
    if (*data != ports->sourceInputCount + 2 * ordinal ||
        *enable != *data + 1 || *enable >= body.getNumArguments())
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
    if (SourceCheckBudget::add(*ordinary, candidates.size()) !=
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
    writer.candidates.reserve(candidates.size());
    for (auto [ordinal, candidateRaw] : llvm::enumerate(candidates)) {
      auto candidate = dyn_cast<DictionaryAttr>(candidateRaw);
      if (failed(exact(candidate, {"state", "result"}, rule,
                       "transaction candidate")))
        return failure();
      auto state = candidate.getAs<DictionaryAttr>("state");
      auto slot = index(candidate, "result", rule);
      if (failed(slot) || failed(wholeStateID(state, rule)))
        return failure();
      if (*slot != *ordinary + ordinal || *slot >= rule->getNumResults())
        return rule->emitOpError()
               << "candidate result must cover its exact continuous slot";
      for (const auto &prior : writer.candidates) {
        if (failed(payComparison(prior.stateID, state, budget, rule)))
          return failure();
        if (identical(prior.stateID, state))
          return rule->emitOpError()
                 << "duplicate candidate owner within transaction";
      }
      writer.candidates.push_back({state, static_cast<uint64_t>(*slot),
                                   cast<OpResult>(rule->getResult(*slot))});
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
    for (const auto &stop : stops.boundaries)
      if (failed(payScope(stop.scope, budget, site)) ||
          failed(payScope(current.scope, budget, site)))
        return failure();
    if (llvm::any_of(stops.boundaries, [&](const auto &stop) {
          return sameEndpoint(stop, current);
        }))
      return current;
    for (const auto &prior : seen)
      if (failed(payScope(prior.scope, budget, site)) ||
          failed(payScope(current.scope, budget, site)))
        return failure();
    if (llvm::any_of(seen, [&](const auto &prior) {
          return sameEndpoint(prior, current);
        }))
      return site->emitOpError() << "cyclic sampled hardware identity route";
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
    auto input = makeIdentityEndpoint(value, scope, analysis, budget);
    if (failed(input))
      return failure();
    for (const auto &entry : cache) {
      if (failed(payScope(input->scope, budget, valueSite(value))) ||
          failed(payScope(entry.input.scope, budget, valueSite(value))))
        return failure();
      if (!sameEndpoint(entry.input, *input))
        continue;
      if (failed(charge(budget, valueSite(value), 1,
                        sizeof(HardwareIdentityEndpoint))) ||
          failed(payScope(entry.output.scope, budget, valueSite(value))))
        return failure();
      return entry.output;
    }
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
struct Prefix {
  bool impossible = false;
  SmallVector<HardwareIdentityEndpoint> atoms;
};
struct PrefixCache {
  HardwareIdentityEndpoint endpoint;
  Prefix prefix;
  bool complete = false;
};
// This cache belongs to exactly one normalizer, whose complete stop policy is
// immutable. Entries compare actual root/path/bindings/value/resolved type;
// the borrowed boundary array's address is never an identity key.
class PrefixProof {
public:
  PrefixProof(const HardwareAnalysis &analysis, SourceCheckBudget &budget,
              HardwareIdentityStopPolicy policy)
      : analysis(analysis), budget(budget), policy(policy),
        identity(analysis, budget, policy) {}
  FailureOr<Prefix> parse(Value value, const HardwareIdentityScope &scope) {
    auto endpoint = identity.normalize(value, scope);
    if (failed(endpoint))
      return failure();
    for (const auto &entry : cache) {
      if (failed(payScope(entry.endpoint.scope, budget, valueSite(value))) ||
          failed(payScope(endpoint->scope, budget, valueSite(value))))
        return failure();
      if (!sameEndpoint(entry.endpoint, *endpoint))
        continue;
      if (!entry.complete)
        return valueSite(value)->emitOpError()
               << "cyclic sampled admission prefix";
      return copy(entry.prefix, valueSite(value));
    }
    auto bound =
        scopeBindings(endpoint->scope, analysis, budget, valueSite(value));
    if (failed(bound) ||
        failed(bitOne(endpoint->value, analysis, *bound, valueSite(value),
                      budget)) ||
        failed(charge(budget, valueSite(value), 1, sizeof(PrefixCache))) ||
        failed(payScope(endpoint->scope, budget, valueSite(value))))
      return failure();
    size_t position = cache.size();
    cache.push_back({*endpoint, {}, false});
    Prefix result;
    auto known = literal(*endpoint, analysis, budget);
    if (failed(known))
      return failure();
    if (known->has_value()) {
      result.impossible = !**known;
    } else if (auto binary = endpoint->value.getDefiningOp<BitsBinaryOp>();
               binary && binary.getOpcode() == "and") {
      if (failed(analysis.verifyResolvedOperation(binary, *bound)))
        return failure();
      auto lhs = parse(binary.getLhs(), endpoint->scope);
      auto rhs = parse(binary.getRhs(), endpoint->scope);
      if (failed(lhs) || failed(rhs))
        return failure();
      auto combined = conjunction(*lhs, *rhs, binary);
      if (failed(combined))
        return failure();
      result = std::move(*combined);
    } else if (auto compare = endpoint->value.getDefiningOp<BitsCompareOp>();
               compare && compare.getPredicate() == "case_eq") {
      if (failed(analysis.verifyResolvedOperation(compare, *bound)))
        return failure();
      auto left = identity.normalize(compare.getLhs(), endpoint->scope);
      auto right = identity.normalize(compare.getRhs(), endpoint->scope);
      if (failed(left) || failed(right))
        return failure();
      auto leftLiteral = literal(*left, analysis, budget);
      auto rightLiteral = literal(*right, analysis, budget);
      if (failed(leftLiteral) || failed(rightLiteral))
        return failure();
      HardwareIdentityEndpoint *raw = nullptr;
      if (leftLiteral->has_value() && **leftLiteral)
        raw = &*right;
      else if (rightLiteral->has_value() && **rightLiteral)
        raw = &*left;
      if (!raw)
        return compare->emitOpError()
               << "sampled prefix requires case_eq(raw guard, one)";
      auto atom = knownOne(*raw, compare);
      if (failed(atom))
        return failure();
      result = std::move(*atom);
    } else {
      return valueSite(endpoint->value)->emitOpError()
             << "sampled prefix admits only known literals, identity, case_eq "
                "and Bits1 AND";
    }
    auto saved = copy(result, valueSite(value));
    if (failed(saved))
      return failure();
    cache[position].prefix = std::move(*saved);
    cache[position].complete = true;
    return result;
  }
  FailureOr<Prefix> knownOne(const HardwareIdentityEndpoint &raw,
                             Operation *site) {
    auto bindings = scopeBindings(raw.scope, analysis, budget, site);
    if (failed(bindings) ||
        failed(bitOne(raw.value, analysis, *bindings, site, budget)))
      return failure();
    auto known = literal(raw, analysis, budget);
    if (failed(known))
      return failure();
    Prefix result;
    if (known->has_value()) {
      result.impossible = !**known;
    } else {
      if (failed(charge(budget, site, 1, sizeof(HardwareIdentityEndpoint))) ||
          failed(payScope(raw.scope, budget, site)))
        return failure();
      result.atoms.push_back(raw);
    }
    return result;
  }
  FailureOr<Prefix> conjunction(const Prefix &lhs, const Prefix &rhs,
                                Operation *site) {
    Prefix result;
    result.impossible = lhs.impossible || rhs.impossible;
    if (result.impossible)
      return result;
    auto copied = copy(lhs, site);
    if (failed(copied))
      return failure();
    result = std::move(*copied);
    for (const auto &atom : rhs.atoms) {
      if (failed(budget.debit(result.atoms.size() + 1, site)))
        return failure();
      for (const auto &prior : result.atoms)
        if (failed(payScope(prior.scope, budget, site)) ||
            failed(payScope(atom.scope, budget, site)))
          return failure();
      if (llvm::any_of(result.atoms, [&](const auto &prior) {
            return sameEndpoint(prior, atom);
          }))
        continue;
      if (failed(charge(budget, site, 1, sizeof(HardwareIdentityEndpoint))) ||
          failed(payScope(atom.scope, budget, site)))
        return failure();
      result.atoms.push_back(atom);
    }
    // Prefix proofs are definition-local: all atoms share the fixed physical
    // scope. Sort by canonical SSA identity after budgeted deduplication.
    if (failed(
            budget.debit(SourceCheckBudget::multiply(result.atoms.size() + 1,
                                                     result.atoms.size() + 1),
                         site)))
      return failure();
    llvm::sort(result.atoms, [](const auto &a, const auto &b) {
      return std::less<const void *>()(a.value.getAsOpaquePointer(),
                                       b.value.getAsOpaquePointer());
    });
    return result;
  }
  LogicalResult equal(const Prefix &lhs, const Prefix &rhs, Operation *site) {
    if (failed(budget.debit(SourceCheckBudget::multiply(lhs.atoms.size() + 1,
                                                        rhs.atoms.size() + 1),
                            site)))
      return failure();
    if (lhs.impossible != rhs.impossible ||
        lhs.atoms.size() != rhs.atoms.size())
      return site->emitOpError() << "sampled reached/fire differs from its "
                                    "ordered known-one prefix";
    for (const auto &atom : lhs.atoms) {
      for (const auto &other : rhs.atoms)
        if (failed(payScope(atom.scope, budget, site)) ||
            failed(payScope(other.scope, budget, site)))
          return failure();
      if (!llvm::any_of(rhs.atoms, [&](const auto &other) {
            return sameEndpoint(atom, other);
          }))
        return site->emitOpError() << "sampled reached/fire differs from its "
                                      "ordered known-one prefix";
    }
    return success();
  }

private:
  FailureOr<Prefix> copy(const Prefix &prefix, Operation *site) {
    if (failed(charge(budget, site, 1, sizeof(Prefix))) ||
        failed(charge(budget, site, prefix.atoms.size(),
                      sizeof(HardwareIdentityEndpoint))))
      return failure();
    for (const auto &atom : prefix.atoms)
      if (failed(payScope(atom.scope, budget, site)))
        return failure();
    return prefix;
  }
  const HardwareAnalysis &analysis;
  SourceCheckBudget &budget;
  HardwareIdentityStopPolicy policy;
  IdentityMemo identity;
  SmallVector<PrefixCache, 0> cache;
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
      auto sampled = parseSampledAdmission(rule, analysis, *bindings, budget);
      if (failed(sampled))
        return failure();
      first = sampled->fireResult;
      if (wiring)
        for (const auto &writer : wiring->writers)
          if (writer.rule == rule)
            first = writer.ordinaryResults;
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

LogicalResult proveSampledAdmission(const SampledAdmissionView &view,
                                    const HardwareAnalysis &analysis,
                                    const HardwareBindings &bindings,
                                    SourceCheckBudget &budget) {
  if (bindings.owner != view.definition)
    return analysis.getPackage().emitOpError()
           << "sampled proof bindings have the wrong owner";
  if (!view.definition || !view.rule || view.guards.empty())
    return analysis.getPackage().emitOpError()
           << "sampled proof requires a parsed nonempty guard inventory";
  if (failed(budget.debit(SourceCheckBudget::bindings(bindings), view.rule)))
    return failure();
  HardwareIdentityScope scope{view.definition, bindings, {}};
  scope.rootBindings.owner = view.definition;
  auto boundaries = localBoundaries(view.definition, scope, analysis, budget);
  if (failed(boundaries))
    return failure();
  HardwareIdentityStopPolicy policy{HardwareIdentityTraversal::DefinitionLocal,
                                    *boundaries};
  PrefixProof proof(analysis, budget, policy);
  Prefix expected;
  for (const auto &guard : view.guards) {
    auto reached = proof.parse(guard.reached, scope);
    if (failed(reached) || failed(proof.equal(*reached, expected, view.rule)))
      return failure();
    auto raw = normalized(guard.predicate, scope, policy, analysis, budget);
    if (failed(raw))
      return failure();
    auto atom = proof.knownOne(*raw, view.rule);
    if (failed(atom))
      return failure();
    auto next = proof.conjunction(expected, *atom, view.rule);
    if (failed(next))
      return failure();
    expected = std::move(*next);
  }
  auto fire = proof.parse(view.fire, scope);
  if (failed(fire))
    return failure();
  return proof.equal(*fire, expected, view.rule);
}

namespace {
struct Occurrence {
  HardwareIdentityScope scope;
  HardwareBindings bindings;
  ModuleOp module;
  std::optional<TransactionWiringView> wiring;
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
  enum Kind { Data, Enable, Condition, Reached } kind;
  SourceExpectOp expect;
};
struct RouteVisit {
  HardwareIdentityEndpoint endpoint;
  size_t state;
  PrivateRoute::Kind kind;
  SourceExpectOp expect;
};
// A proof invocation owns one actual root. The catalog caller supplies every
// definition/binding root, including unselected definitions; no package closure
// or manifest parser is constructed here.
class WiringProof {
public:
  WiringProof(const HardwareAnalysis &analysis, SourceCheckBudget &budget,
              const llvm::DenseSet<Operation *> &relevant)
      : analysis(analysis), budget(budget), relevant(relevant) {}
  LogicalResult run(ModuleOp root, const HardwareBindings &bindings) {
    if (bindings.owner != root)
      return root->emitOpError()
             << "transaction proof bindings have the wrong owner";
    if (failed(budget.debit(SourceCheckBudget::bindings(bindings), root)))
      return failure();
    HardwareIdentityScope scope{root, bindings, {}};
    scope.rootBindings.owner = root;
    if (failed(expand(scope, std::nullopt, {})) || failed(inventory()) ||
        failed(writers()) || failed(externalInputs()) ||
        failed(connections()) || failed(privateUses()))
      return failure();
    return success();
  }

private:
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
      auto wiring =
          parseTransactionWiring(module, analysis, entry.bindings, budget);
      if (failed(wiring))
        return failure();
      if (!witness && wiring->states.empty() && wiring->incoming.empty() &&
          wiring->writers.empty())
        return module->emitOpError() << "empty transaction_wiring has no "
                                        "sampled/check/internal-port witness";
      entry.wiring = std::move(*wiring);
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
    auto relative = resolveTransactionState(entry.module, id, analysis,
                                            entry.bindings, budget);
    if (failed(relative))
      return failure();
    auto scope = actualStateScope(entry, *relative);
    if (failed(scope))
      return failure();
    for (auto [ordinal, state] : llvm::enumerate(states)) {
      if (failed(payScope(state.scope, budget, entry.module)) ||
          failed(payScope(*scope, budget, entry.module)))
        return failure();
      if (state.storage == relative->storage && sameScope(state.scope, *scope))
        return ordinal;
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
    for (const auto &entry : occurrences) {
      if (!entry.wiring)
        continue;
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
      }
      for (const auto &writer : entry.wiring->writers) {
        auto sampled = parseSampledAdmission(writer.rule, analysis,
                                             entry.bindings, budget);
        if (failed(sampled) || failed(proveSampledAdmission(
                                   *sampled, analysis, entry.bindings, budget)))
          return failure();
        for (uint64_t ordinal = writer.ordinaryResults;
             ordinal < writer.rule->getNumResults(); ++ordinal)
          if (failed(boundary(writer.rule->getResult(ordinal), entry.scope,
                              stops)))
            return failure();
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
                             SourceExpectOp expect = {}) {
    if (failed(charge(budget, valueSite(value), 1, sizeof(PrivateRoute))))
      return failure();
    auto endpoint = makeIdentityEndpoint(value, scope, analysis, budget);
    if (failed(endpoint))
      return failure();
    privateRoutes.push_back({std::move(*endpoint), state, kind, expect});
    return success();
  }
  LogicalResult writers() {
    for (const auto &entry : occurrences) {
      if (!entry.wiring)
        continue;
      for (const auto &writer : entry.wiring->writers) {
        auto sampled = parseSampledAdmission(writer.rule, analysis,
                                             entry.bindings, budget);
        if (failed(sampled))
          return failure();
        auto clock = normal(sampled->clock, entry.scope);
        auto reset = normal(sampled->reset, entry.scope);
        auto fire =
            normal(writer.rule->getResult(sampled->fireResult), entry.scope);
        if (failed(clock) || failed(reset) || failed(fire))
          return failure();
        if (writer.ordinaryResults) {
          auto localStops =
              localBoundaries(entry.module, entry.scope, analysis, budget);
          if (failed(localStops))
            return failure();
          HardwareIdentityStopPolicy local{
              HardwareIdentityTraversal::DefinitionLocal, *localStops};
          for (const auto &guard : sampled->guards) {
            auto raw = normalized(guard.predicate, entry.scope, local, analysis,
                                  budget);
            if (failed(raw))
              return failure();
            auto known = literal(*raw, analysis, budget);
            if (failed(known))
              return failure();
            if (!known->has_value() || !**known)
              return writer.rule->emitOpError()
                     << "ordinary transaction results require all guards "
                        "proven literal true";
          }
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
        if (writer.candidates.empty() &&
            failed(privateRoute(writer.rule->getResult(sampled->fireResult),
                                entry.scope, std::numeric_limits<size_t>::max(),
                                PrivateRoute::Enable)))
          return failure();
        for (const auto &candidate : writer.candidates) {
          auto stateIndex = target(entry, candidate.stateID);
          auto data = normal(candidate.result, entry.scope);
          if (failed(stateIndex) || failed(data))
            return failure();
          auto &state = states[*stateIndex];
          if (state.writer)
            return writer.rule->emitOpError()
                   << "multiple outer transaction writers for one actual owner";
          if (failed(typeMatches(*data, state.payload, writer.rule)))
            return failure();
          if (failed(payScope(data->scope, budget, writer.rule)) ||
              failed(payScope(fire->scope, budget, writer.rule)) ||
              failed(payScope(clock->scope, budget, writer.rule)) ||
              failed(payScope(reset->scope, budget, writer.rule)))
            return failure();
          state.writer = writer.rule;
          state.data = *data;
          state.enable = *fire;
          state.clock = *clock;
          state.reset = *reset;
          if (failed(privateRoute(candidate.result, entry.scope, *stateIndex,
                                  PrivateRoute::Data)) ||
              failed(privateRoute(writer.rule->getResult(sampled->fireResult),
                                  entry.scope, *stateIndex,
                                  PrivateRoute::Enable)))
            return failure();
        }
        for (const auto &guard : sampled->guards) {
          SourceExpectOp match;
          for (auto expect :
               entry.module->getRegion(0).front().getOps<SourceExpectOp>()) {
            if (failed(budget.debit(1, expect)))
              return failure();
            auto checkID = expect->getAttrOfType<DictionaryAttr>("ac.check_id");
            if (failed(payAttribute(checkID, budget, expect)) ||
                failed(verifyCheckID(checkID,
                                     [&] { return expect->emitOpError(); })) ||
                failed(payComparison(checkID, guard.id, budget, expect)))
              return failure();
            if (!identical(expect->getAttrOfType<DictionaryAttr>("ac.check_id"),
                           guard.id))
              continue;
            if (match)
              return expect->emitOpError()
                     << "duplicate expect for sampled guard";
            match = expect;
          }
          if (!match)
            return writer.rule->emitOpError()
                   << "sampled guard has no matching expect";
          if (failed(
                  verifySourceExpectOperation(analysis, match, entry.bindings)))
            return failure();
          auto condition = normal(match.getCondition(), entry.scope);
          auto reached = normal(match.getPath(), entry.scope);
          auto expectedCondition = normal(
              writer.rule->getResult(guard.predicateResult), entry.scope);
          auto expectedReached =
              normal(writer.rule->getResult(guard.reachedResult), entry.scope);
          auto required =
              writer.rule->getAttrOfType<ArrayAttr>("ac.required_checks");
          auto record = cast<DictionaryAttr>(
              required[(guard.predicateResult - sampled->fireResult - 1) / 2]);
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
          if (failed(privateRoute(writer.rule->getResult(guard.predicateResult),
                                  entry.scope, 0, PrivateRoute::Condition,
                                  match)) ||
              failed(privateRoute(writer.rule->getResult(guard.reachedResult),
                                  entry.scope, 0, PrivateRoute::Reached,
                                  match)))
            return failure();
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
                "candidate/fire or incoming pair";
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
    for (auto [ordinal, entry] : llvm::enumerate(occurrences)) {
      if (failed(payScope(entry.scope, budget, scope.root)) ||
          failed(payScope(scope, budget, scope.root)))
        return failure();
      if (sameScope(entry.scope, scope))
        return ordinal;
    }
    return scope.root->emitOpError()
           << "private route has no actual module occurrence";
  }
  LogicalResult visitPrivate(const HardwareIdentityEndpoint &endpoint,
                             const PrivateRoute &route,
                             SmallVectorImpl<RouteVisit> &seen) {
    Operation *site = valueSite(endpoint.value);
    if (failed(budget.debit(seen.size() + 1, site)))
      return failure();
    if (endpoint.value.use_empty()) {
      // A transaction with no candidates still publishes its verified fire
      // suffix, even though it has no physical enable terminal. Only that
      // original root is intentionally unused; identity forks and internal
      // output carriers must reach their prescribed terminal on every branch.
      if (route.kind == PrivateRoute::Enable &&
          route.state == std::numeric_limits<size_t>::max() &&
          sameEndpoint(endpoint, route.endpoint))
        return success();
      return site->emitOpError()
             << "private transaction/check route has a dead end before its "
                "corresponding D/E or expect";
    }
    for (const auto &entry : seen)
      if (failed(payScope(entry.endpoint.scope, budget, site)) ||
          failed(payScope(endpoint.scope, budget, site)))
        return failure();
    if (llvm::any_of(seen, [&](const auto &entry) {
          return entry.state == route.state && entry.kind == route.kind &&
                 entry.expect == route.expect &&
                 sameEndpoint(entry.endpoint, endpoint);
        }))
      return success();
    if (failed(charge(budget, site, 1, sizeof(RouteVisit))) ||
        failed(payScope(endpoint.scope, budget, site)))
      return failure();
    seen.push_back({endpoint, route.state, route.kind, route.expect});
    auto currentOccurrence = occurrenceFor(endpoint.scope);
    if (failed(currentOccurrence))
      return failure();
    const auto &owner = occurrences[*currentOccurrence];
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
        if (failed(visitPrivate(*next, route, seen)))
          return failure();
        continue;
      }
      if (auto expect = dyn_cast<SourceExpectOp>(consumer)) {
        if (expect == route.expect &&
            ((route.kind == PrivateRoute::Condition && operand == 0) ||
             (route.kind == PrivateRoute::Reached && operand == 1)))
          continue;
        return expect->emitOpError() << "private transaction/check value "
                                        "escapes to unrelated expect operand";
      }
      if (auto instance = dyn_cast<InstanceOp>(consumer)) {
        if (route.kind == PrivateRoute::Condition ||
            route.kind == PrivateRoute::Reached)
          return instance->emitOpError()
                 << "check suffix escapes its corresponding expect";
        if (analysis.getPrimitiveKind(analysis.resolveCallee(instance)) ==
            "dffe") {
          if (route.kind == PrivateRoute::Data && operand == 3 &&
              stateTerminal(endpoint.scope, instance, route.state))
            continue;
          if (route.kind == PrivateRoute::Enable && operand == 2) {
            bool allowed = false;
            for (auto [ordinal, state] : llvm::enumerate(states)) {
              if (failed(payScope(state.scope, budget, instance)) ||
                  failed(payScope(endpoint.scope, budget, instance)))
                return failure();
              if (!stateTerminal(endpoint.scope, instance, ordinal))
                continue;
              auto targetAllowed = enableTarget(route, ordinal);
              if (failed(targetAllowed))
                return failure();
              allowed = *targetAllowed;
            }
            if (allowed)
              continue;
          }
          return instance->emitOpError() << "private candidate/fire reaches "
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
            failed(visitPrivate(*nextEndpoint, route, seen)))
          return failure();
        continue;
      }
      if (auto yielded = dyn_cast<YieldOp>(consumer)) {
        if (route.kind == PrivateRoute::Condition ||
            route.kind == PrivateRoute::Reached ||
            yielded->getParentOp() != owner.module.operator->() ||
            !owner.wiring || operand < owner.wiring->ports.sourceOutputCount ||
            !owner.parent || operand >= owner.allocation->getNumResults())
          return yielded->emitOpError()
                 << "private transaction value escapes through "
                    "public/unterminated Module or Rule output";
        const auto &parent = occurrences[*owner.parent];
        auto next = makeIdentityEndpoint(owner.allocation->getResult(operand),
                                         parent.scope, analysis, budget);
        if (failed(next) || failed(visitPrivate(*next, route, seen)))
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
    SmallVector<RouteVisit> seen;
    for (const auto &route : privateRoutes)
      if (failed(visitPrivate(route.endpoint, route, seen)))
        return failure();
    return success();
  }
  const HardwareAnalysis &analysis;
  SourceCheckBudget &budget;
  const llvm::DenseSet<Operation *> &relevant;
  SmallVector<Occurrence, 0> occurrences;
  SmallVector<State, 0> states;
  SmallVector<HardwareIdentityEndpoint> stops;
  SmallVector<PrivateRoute> privateRoutes;
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
                        SourceCheckBudget &budget) {
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
    for (auto &operation : module.getBody().front()) {
      if (failed(budget.debit(1, &operation)) ||
          failed(payAttribute(operation.getAttr("ac.required_checks"), budget,
                              &operation)))
        return failure();
    }
    if (localWitness(module) && failed(mark(module, relevant, pending)))
      return failure();
    if (module->hasAttr("ac.transaction_wiring")) {
      auto wiring =
          parseTransactionWiring(module, analysis, entry.second, budget);
      if (failed(wiring))
        return failure();
      for (const auto &writer : wiring->writers) {
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
  for (const auto &[module, bindings] : scopes) {
    if (failed(budget.debit(1, module)))
      return failure();
    if (!relevant.contains(module.operator->()))
      continue;
    WiringProof proof(analysis, budget, relevant);
    if (failed(proof.run(module, bindings)))
      return failure();
  }
  return success();
}
} // namespace acir::ac::detail

#include "HardwareQueueResources.h"
#include "HardwareSourceCheckPreflight.h"
#include "ACIRSourceContracts.h"
#include "HardwareSampledAdmission.h"
#include "pycircuit/Dialect/ACIR/TransactionalRouting.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include <algorithm>
#include <memory>
#include <map>

using namespace mlir;
namespace acir::ac::detail {
namespace {
using Budget = SourceCheckBudget;
struct Expansion {
  uint64_t owners = 1, checks = 0, commits = 0, sampling = 0, cost = 0;
};
struct Allocation {
  Operation *operation;
  SmallVector<uint64_t> shape;
  std::optional<size_t> child;
  StringAttr primitive;
  uint64_t count = 1;
  HardwareCheckCommitEndpointForm form = HardwareCheckCommitEndpointForm::Physical;
  bool combinationalExternal = false;
};
struct BoundDefinition {
  ModuleOp definition;
  HardwareBindings bindings;
  SmallVector<uint64_t> checkIndices;
  SmallVector<uint64_t> samplingIndices;
  SmallVector<Allocation> allocations;
  std::optional<unsigned> reset;
  bool hasChecks = false;
  std::optional<Expansion> expansion;
};
class Builder {
public:
  Builder(const HardwareAnalysis &analysis, HardwareSourceCheckLimits limits,
          PackageVerificationProfile profile)
      : analysis(analysis), profile(profile), budget(limits), validation(analysis, budget) {}
  FailureOr<HardwareSourceCheckPlan> build() {
    auto package = analysis.getPackage();
    budget.setPhase("raw preflight");
    if (!package)
      return failure();
    {
      Budget::RawDiagnosticScope rawDiagnostics(budget);
      if (failed(rawPreflight()))
        return failure();
    }
    if (failed(verifyHardwarePackageStage(analysis, profile))) return failure();
    {
      NativeSourceCheckScope native(analysis, validation, budget, nativeChecks);
      // Establish the builtin package's own ODS/trait/custom shape before any
      // child verifier assumes its parent is well formed. Full nonrecursive
      // verification here would run dominance on unchecked child operands.
      if (failed(budget.debit(1, package)) ||
          failed(package->getName().verifyInvariants(package))) return failure();
      // MLIR parallelizes nested isolated operations. Verify direct package
      // children as explicit roots so each Module's own region verifier runs
      // on this thread after its nested parallel checks have joined. Each
      // subtree retains the complete native structural/dominance verification.
      for (Region &region : package->getRegions())
        for (Block &block : region)
          for (Operation &operation : block) {
            if (failed(budget.debit(1, &operation)) ||
                failed(mlir::verify(&operation))) return failure();
          }
      // All children are verified; retain the package's own invariants and
      // cross-child region/dominance checks without scheduling them again.
      if (failed(mlir::verify(package, false))) return failure();
    }
    if (failed(verifyHardwarePackageEnvelope(
            analysis, profile, [&](uint64_t units, Operation *site) {
              return budget.debit(units, site);
            }))) return failure();
    auto resetSemantics = getHardwareResetSemantics(package);
    if (failed(resetSemantics))
      return failure();
    plan.resetSemantics = *resetSemantics;
    budget.setPhase("definition catalog");
    auto root = resolveHardwareRootBindings(analysis);
    if (failed(root) || failed(catalog()))
      return failure();
    budget.setPhase("bound definitions");
    auto rootIndex = visit(cast<ModuleOp>(root->owner), *root);
    if (failed(rootIndex))
      return failure();
    auto scopes = std::move(catalogScopes);
    if (failed(budget.debit(
            Budget::multiply(
                Budget::add(nodes.size(), scopes.size()),
                Budget::bytes(sizeof(std::pair<ModuleOp, HardwareBindings>))),
            package)))
      return failure();
    for (const auto &node : nodes) {
      if (failed(
              budget.debit(Budget::bindings(node->bindings), node->definition)))
        return failure();
      scopes.emplace_back(node->definition, node->bindings);
    }
    budget.setPhase("compact dependencies");
    if (failed(verifySourceCheckDependencies(
            analysis, scopes, [&](uint64_t units, Operation *site) {
              return budget.debit(units, site);
            })))
      return failure();
    budget.setPhase("queue resource wiring");
    if (failed(verifyQueueResourceWiring(analysis, scopes, budget)))
      return failure();
    budget.setPhase("transaction wiring");
    auto contract = package->getAttrOfType<IntegerAttr>("ac.routed_contract");
    if (failed(contract && contract.getInt() == 4
                   ? verifyTransactionalRouting(
                         package, [&](uint64_t units, Operation *site) {
                           return budget.debit(units, site);
                         })
                   : verifyTransactionWiring(analysis, scopes, validation, budget)))
      return failure();
    // A closed system needs the complete managed commit tree even without
    // source assertions: generated reset/prepare/commit must reach every leaf.
    // Reusable modules without checks still avoid hypothetical expansion.
    auto role = root->owner->getAttrOfType<StringAttr>("ac.root_kind");
    if (!nodes[*rootIndex]->hasChecks && (!role || role.getValue() != "system") &&
        (!contract || contract.getInt() != 4))
      return std::move(plan);
    budget.setPhase("occurrence summary");
    auto expansion = summarize(*rootIndex);
    if (failed(expansion) || failed(budget.admit(expansion->cost, package)))
      return failure();
    if (expansion->owners > plan.owners.max_size() ||
        expansion->checks > plan.checks.max_size() ||
        expansion->commits > plan.commits.max_size() ||
        expansion->sampling > plan.samplingOccurrences.max_size())
      return package.emitOpError()
             << "source-check plan exceeds analysis work budget before "
                "occurrence expansion";
    plan.owners.reserve(expansion->owners);
    plan.checks.reserve(expansion->checks);
    plan.commits.reserve(expansion->commits);
    plan.samplingOccurrences.reserve(expansion->sampling);
    budget.setPhase("materialization");
    if (failed(materialize(*rootIndex, {}, std::nullopt, {})))
      return failure();
    return std::move(plan);
  }

private:
  FailureOr<uint64_t> structureCost(Attribute attribute, Operation *site) {
    if (!attribute)
      return uint64_t(0);
    uint64_t cost = 1;
    if (auto text = dyn_cast<StringAttr>(attribute))
      cost = Budget::add(cost, Budget::bytes(text.getValue().size()));
    if (failed(budget.debit(cost, site)))
      return failure();
    if (!isa<ArrayAttr, DictionaryAttr>(attribute))
      return cost;
    SmallVector<Attribute, 0> pending;
    auto push = [&](Attribute value) -> LogicalResult {
      if (pending.size() == pending.capacity()) {
        uint64_t capacity = Budget::multiply(
            std::max(uint64_t(1), uint64_t(pending.capacity())), 2);
        uint64_t storage =
            Budget::multiply(capacity - pending.capacity(), sizeof(Attribute));
        if (capacity == std::numeric_limits<uint64_t>::max() ||
            storage == std::numeric_limits<uint64_t>::max() ||
            capacity > pending.max_size())
          return budget.debit(std::numeric_limits<uint64_t>::max(), site);
        if (failed(budget.debit(Budget::bytes(storage), site)))
          return failure();
        pending.reserve(capacity);
      }
      pending.push_back(value);
      return success();
    };
    auto children = [&](Attribute value) -> LogicalResult {
      if (auto array = dyn_cast<ArrayAttr>(value)) {
        for (Attribute child : array)
          if (failed(push(child)))
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
      Attribute current = pending.pop_back_val();
      uint64_t node = 1;
      if (auto text = dyn_cast<StringAttr>(current))
        node = Budget::add(node, Budget::bytes(text.getValue().size()));
      if (failed(budget.debit(node, site)))
        return failure();
      cost = Budget::add(cost, node);
      if (failed(children(current)))
        return failure();
    }
    return cost;
  }
  struct RawDefinitionWork {
    uint64_t operations = 0, obligations = 0, metadata = 1, carriers = 0;
    uint64_t operands = 0, rules = 0, sampledRules = 0, instances = 0;
    uint64_t ruleResults = 0, instanceMetadata = 0, ruleMetadata = 0;
    uint64_t wiringMetadata = 0, states = 0, incoming = 0, writers = 0;
    uint64_t candidates = 0, maxCandidates = 0, sampledSchema = 0;
    uint64_t observations = 0, observationOperands = 0, observationMetadata = 0;
    uint64_t checkKeys = 0, keyMax = 0, keyInsertion = 0;
    uint64_t registrationGroups = 0, registrationMax = 0,
             registrationInsertion = 0, terminalRegistrationCost = 0;
    uint64_t terminalKeys = 0, terminalKeyCost = 0, validationOperands = 0;
    struct Registration {
      DictionaryAttr occurrence;
      uint64_t cost;
    };
    SmallVector<Registration, 0> registrations;
    bool distinctRegistrations = true;
    bool hasSamplingMetadata = false, hasWiring = false;
  };
  FailureOr<uint64_t> checkComparisonAllowance(uint64_t count,
                                               Operation *site) {
    if (!count)
      return uint64_t(0);
    uint64_t height = 0;
    for (; count; count /= 2) {
      if (failed(budget.debit(1, site)))
        return failure();
      ++height;
    }
    // Covers insertion and lookup in the inspected standard-library tree
    // implementations, including semantic-hardening comparator calls.
    return Budget::add(Budget::multiply(6, height), 1);
  }
  LogicalResult countAssociationOperands(Operation *op,
                                         RawDefinitionWork &local,
                                         bool directRule, bool directExpect) {
    auto fail = [&]() -> LogicalResult {
      return emitError(UnknownLoc::get(op->getContext()))
             << "source-check raw association requires closed required rows "
                "and dictionary CheckIDs";
    };
    if (directRule) {
      if (failed(budget.debit(1, op)))
        return failure();
      Attribute occurrence = op->getAttr("occurrence");
      Attribute rawRequired = op->getAttr("ac.required_checks");
      auto required = dyn_cast_or_null<ArrayAttr>(rawRequired);
      if (rawRequired && !required)
        return fail();
      auto occurrenceCost = structureCost(occurrence, op);
      auto requiredCost = structureCost(rawRequired, op);
      if (failed(occurrenceCost) || failed(requiredCost))
        return failure();
      local.validationOperands = Budget::add(
          local.validationOperands,
          Budget::add(Budget::multiply(3, *occurrenceCost), *requiredCost));
      if (required) {
        if (!required.empty()) {
          // Prove the raw registration groups are distinct before using the
          // direct sampled-row allowance. Structural equality includes numeric
          // attributes with different widths; pointer inequality is not proof.
          auto registration = dyn_cast_or_null<DictionaryAttr>(occurrence);
          if (failed(budget.debit(*occurrenceCost, op)) ||
              failed(verifyOccurrence(registration, [&] {
                return emitError(UnknownLoc::get(op->getContext()));
              })))
            return failure();
          for (const auto &prior : local.registrations) {
            if (failed(budget.debit(
                    Budget::add(1, Budget::add(*occurrenceCost, prior.cost)), op)))
              return failure();
            if (compareClosedSourceStructure(registration, prior.occurrence) == 0)
              local.distinctRegistrations = false;
          }
          if (local.registrations.size() == local.registrations.capacity()) {
            uint64_t capacity = Budget::add(
                Budget::multiply(local.registrations.capacity(), 2), 1);
            uint64_t bytes = Budget::multiply(
                capacity - local.registrations.capacity(),
                sizeof(RawDefinitionWork::Registration));
            if (capacity == std::numeric_limits<uint64_t>::max() ||
                bytes == std::numeric_limits<uint64_t>::max() ||
                capacity > local.registrations.max_size())
              return budget.debit(std::numeric_limits<uint64_t>::max(), op);
            if (failed(budget.debit(
                    Budget::add(Budget::bytes(bytes), local.registrations.size()), op)))
              return failure();
            local.registrations.reserve(capacity);
          }
          local.registrations.push_back({registration, *occurrenceCost});
          auto comparisons = checkComparisonAllowance(local.registrationGroups, op);
          if (failed(comparisons))
            return failure();
          local.registrationInsertion = Budget::add(
              local.registrationInsertion,
              Budget::multiply(*comparisons,
                               Budget::add(1, Budget::add(*occurrenceCost,
                                                        local.registrationMax))));
          local.registrationMax = std::max(local.registrationMax, *occurrenceCost);
          local.registrationGroups = Budget::add(local.registrationGroups, 1);
        }
        for (Attribute raw : required) {
          if (failed(budget.debit(1, op)))
            return failure();
          auto row = dyn_cast<DictionaryAttr>(raw);
          auto id = row ? row.getAs<DictionaryAttr>("id") : DictionaryAttr();
          auto registration =
              id ? id.getAs<DictionaryAttr>("registration") : DictionaryAttr();
          if (!row || row.size() != 3 || !id || id.size() != 3 ||
              !registration || !row.getAs<StringAttr>("kind") ||
              !row.getAs<DictionaryAttr>("location"))
            return fail();
          auto checkCost = structureCost(id.get("check"), op);
          auto obligationCost = structureCost(id.get("obligation"), op);
          auto comparisons = checkComparisonAllowance(local.checkKeys, op);
          if (failed(checkCost) || failed(obligationCost) || failed(comparisons))
            return failure();
          uint64_t idCost = Budget::add(*checkCost, *obligationCost);
          local.keyInsertion = Budget::add(
              local.keyInsertion,
              Budget::multiply(
                  *comparisons,
                  Budget::add(1, Budget::add(idCost, local.keyMax))));
          local.keyMax = std::max(local.keyMax, idCost);
          local.checkKeys = Budget::add(local.checkKeys, 1);
          // The owning Rule/registration comparison is a separate traversal,
          // not an association lookup or a cached structural verdict.
          if (failed(budget.debit(1, op)))
            return failure();
          uint64_t registrationCost = 1;
          if (registration != occurrence) {
            auto lhs = structureCost(registration, op);
            auto rhs = structureCost(occurrence, op);
            if (failed(lhs) || failed(rhs))
              return failure();
            registrationCost = Budget::add(*lhs, *rhs);
          }
          local.validationOperands =
              Budget::add(local.validationOperands, registrationCost);
        }
      }
    }
    if (directExpect) {
      if (failed(budget.debit(1, op)))
        return failure();
      uint64_t metadata = 0;
      for (StringRef key : {"ac.check_id", "location", "kind", "ac.message"}) {
        Attribute value = op->getAttr(key);
        if (key == "ac.check_id" && !isa_and_nonnull<DictionaryAttr>(value))
          return fail();
        auto cost = structureCost(value, op);
        if (failed(cost))
          return failure();
        metadata = Budget::add(metadata, *cost);
        if (key == "ac.check_id") {
          auto id = cast<DictionaryAttr>(value);
          auto registrationCost = structureCost(id.get("registration"), op);
          auto checkCost = structureCost(id.get("check"), op);
          auto obligationCost = structureCost(id.get("obligation"), op);
          if (failed(registrationCost) || failed(checkCost) || failed(obligationCost))
            return failure();
          local.terminalRegistrationCost = Budget::add(
              local.terminalRegistrationCost, *registrationCost);
          local.terminalKeyCost = Budget::add(
              local.terminalKeyCost, Budget::add(*checkCost, *obligationCost));
        }
      }
      local.terminalKeys = Budget::add(local.terminalKeys, 1);
      local.validationOperands =
          Budget::add(local.validationOperands, Budget::multiply(2, metadata));
    }
    return success();
  }
  FailureOr<uint64_t> rawCarrierCost(Value value, Operation *site) {
    llvm::DenseSet<Value> seen;
    uint64_t cost = 0;
    while (value) {
      Operation *producer = value.getDefiningOp();
      if (!producer)
        break;
      if (!isa<BitsExtractOp>(producer))
        break;
      uint64_t step = Budget::add(1, Budget::bytes(sizeof(Value)));
      if (failed(budget.debit(step, site)))
        return failure();
      cost = Budget::add(cost, step);
      // Generic operand access is safe even when an extract is malformed.
      if (!seen.insert(value).second || producer->getNumOperands() != 1)
        break;
      value = producer->getOperand(0);
    }
    return cost;
  }
  template <typename T> static uint64_t schemaRecordCost() {
    return Budget::add(1, Budget::bytes(sizeof(T)));
  }
  // Exact native payAttribute mirror. Raw traversal pays its own work before
  // visiting metadata or allocating its explicit handle stack.
  FailureOr<uint64_t> sampledAttributeCost(Attribute attribute, Operation *site) {
    // Attributes are immutable for this build. Cache only the completed
    // measurement; callers still receive its full downstream reservation.
    bool retain = isa_and_nonnull<ArrayAttr, DictionaryAttr>(attribute);
    uint64_t lookupCost = 0;
    const void *key = attribute.getAsOpaquePointer();
    if (retain) {
      auto allowance = checkComparisonAllowance(
          Budget::add(sampledAttributeCosts.size(), 1), site);
      if (failed(allowance) || failed(budget.debit(*allowance, site)))
        return failure();
      lookupCost = *allowance;
      auto found = sampledAttributeCosts.find(key);
      if (found != sampledAttributeCosts.end()) return found->second;
    }
    uint64_t cost = 0;
    auto pay = [&](uint64_t units) -> LogicalResult {
      if (failed(budget.debit(units, site)))
        return failure();
      cost = Budget::add(cost, units);
      return success();
    };
    if (failed(pay(1)))
      return failure();
    SmallVector<Attribute, 0> pending;
    auto push = [&](Attribute value) -> LogicalResult {
      if (failed(pay(1)))
        return failure();
      if (pending.size() == pending.capacity()) {
        uint64_t capacity = std::max(
            uint64_t(2), Budget::add(Budget::multiply(pending.capacity(), 2), 1));
        uint64_t bytes =
            Budget::multiply(capacity - pending.capacity(), sizeof(Attribute));
        if (capacity == std::numeric_limits<uint64_t>::max() ||
            bytes == std::numeric_limits<uint64_t>::max() ||
            capacity > pending.max_size())
          return budget.debit(std::numeric_limits<uint64_t>::max(), site);
        if (failed(pay(Budget::add(Budget::bytes(bytes), pending.size()))))
          return failure();
        pending.reserve(capacity);
      }
      pending.push_back(value);
      return success();
    };
    auto children = [&](Attribute value) -> LogicalResult {
      if (auto text = dyn_cast_or_null<StringAttr>(value))
        return pay(Budget::bytes(text.getValue().size()));
      if (auto array = dyn_cast_or_null<ArrayAttr>(value)) {
        for (Attribute child : array)
          if (failed(push(child)))
            return failure();
      } else if (auto dictionary = dyn_cast_or_null<DictionaryAttr>(value)) {
        for (auto field : dictionary)
          if (failed(pay(Budget::bytes(field.getName().getValue().size()))) ||
              failed(push(field.getValue())))
            return failure();
      }
      return success();
    };
    if (failed(children(attribute)))
      return failure();
    while (!pending.empty())
      if (failed(children(pending.pop_back_val())))
        return failure();
    if (retain) {
      if (failed(budget.debit(Budget::add(lookupCost,
              Budget::bytes(sizeof(decltype(sampledAttributeCosts)::value_type) +
                            4 * sizeof(void *))), site)))
        return failure();
      sampledAttributeCosts.emplace(key, cost);
    }
    return cost;
  }
  FailureOr<uint64_t> sampledSchemaCost(Attribute attribute, Operation *site) {
    auto sampled = dyn_cast_or_null<DictionaryAttr>(attribute);
    auto controls = sampled ? sampled.getAs<ArrayAttr>("controls") : ArrayAttr();
    auto checks = sampled ? sampled.getAs<ArrayAttr>("body_checks") : ArrayAttr();
    auto observations =
        sampled ? sampled.getAs<ArrayAttr>("observations") : ArrayAttr();
    if ((sampled && sampled.get("ehr_entries")) || site->hasAttr("ac.after") ||
        site->hasAttr("ac.callable_after"))
      return site->emitOpError() << "after ordering and EHR entry forwarding are retired";
    auto requirements = sampled ? sampled.getAs<ArrayAttr>("requirements") : ArrayAttr();
    auto resources = sampled ? sampled.getAs<ArrayAttr>("resources") : ArrayAttr();
    auto proposals = sampled ? sampled.getAs<ArrayAttr>("queue_proposals") : ArrayAttr();
    auto memoryProposals = sampled ? sampled.getAs<ArrayAttr>("memory_proposals") : ArrayAttr();
    auto domains = sampled ? sampled.getAs<ArrayAttr>("write_domains") : ArrayAttr();
    auto fail = [&]() -> LogicalResult {
      emitError(UnknownLoc::get(site->getContext()))
          << "sampled admission raw layout requires closed schema and "
             "checked result counts, roles and decreasing path indices";
      return failure();
    };
    auto rawIndex = [&](DictionaryAttr row,
                        StringRef key) -> std::optional<uint64_t> {
      auto value = row ? row.getAs<IntegerAttr>(key) : IntegerAttr();
      auto type = value ? dyn_cast<IntegerType>(value.getType()) : IntegerType();
      if (!type || !type.isSignless() || type.getWidth() != 64 ||
          value.getInt() < 0)
        return std::nullopt;
      return static_cast<uint64_t>(value.getInt());
    };
    if (failed(budget.debit(1, site)))
      return failure();
    auto clock = rawIndex(sampled, "clock_capture"),
         reset = rawIndex(sampled, "reset_capture"),
         fire = rawIndex(sampled, "fire_result");
    if (!sampled || sampled.size() != (6 + bool(requirements) + bool(resources) + bool(proposals) + bool(memoryProposals) + bool(domains)) || !controls || controls.empty() ||
        !checks || !observations || !clock || !reset || !fire ||
        *clock >= site->getNumOperands() || *reset >= site->getNumOperands())
      return fail();
    uint64_t g = controls.size(), a = checks.size(), n = observations.size();
    auto required = site->getAttrOfType<ArrayAttr>("ac.required_checks");
    if (!required || required.size() != Budget::add(g, a))
      return fail();
    auto add = Budget::add;
    auto multiply = Budget::multiply;
    uint64_t cost = schemaRecordCost<SampledAdmissionView>();
    auto attributeCost = [&](Attribute value) -> LogicalResult {
      auto units = sampledAttributeCost(value, site);
      if (failed(units))
        return failure();
      cost = add(cost, *units);
      return success();
    };
    auto comparison = [&](Attribute lhs, Attribute rhs) -> LogicalResult {
      if (failed(budget.debit(1, site)))
        return failure();
      if (lhs == rhs) {
        cost = add(cost, 1);
        return success();
      }
      return failure(failed(attributeCost(lhs)) || failed(attributeCost(rhs)));
    };
    auto prefixRelation = [&](ArrayAttr prefix,
                              ArrayAttr invocation) -> LogicalResult {
      if (failed(budget.debit(1, site)))
        return failure();
      cost = add(cost, 1);
      if (!prefix || !invocation)
        return fail();
      if (prefix.size() > invocation.size())
        return success();
      for (uint64_t i = 0; i < prefix.size(); ++i) {
        if (failed(budget.debit(1, site)))
          return failure();
        cost = add(cost, 1);
        if (failed(comparison(prefix[i], invocation[i])))
          return failure();
      }
      return success();
    };
    auto kindOf = [](DictionaryAttr row) {
      return row ? row.getAs<StringAttr>("kind") : StringAttr();
    };
    auto validControl = [&](DictionaryAttr row) -> bool {
      auto kind = kindOf(row);
      auto predecessor = row ? row.getAs<IntegerAttr>("predecessor") : IntegerAttr();
      auto type = predecessor ? dyn_cast<IntegerType>(predecessor.getType())
                              : IntegerType();
      return row && row.size() == 5 && kind &&
             (kind.getValue() == "callable_guard" ||
              kind.getValue() == "branch_select" ||
              kind.getValue() == "value_available" ||
              kind.getValue() == "resource_available") &&
             row.getAs<DictionaryAttr>("id") && row.getAs<ArrayAttr>("invocation") &&
             row.getAs<DictionaryAttr>("path") && type && type.isSignless() &&
             type.getWidth() == 64 && predecessor.getInt() >= -1;
    };
    auto pathCost = [&](DictionaryAttr row, ArrayAttr invocation,
                        uint64_t before) -> LogicalResult {
      auto path = row ? row.getAs<DictionaryAttr>("path") : DictionaryAttr();
      if (!path)
        return fail();
      if (!path.empty()) {
        if (failed(budget.debit(1, site)))
          return failure();
        cost = add(cost, 1);
      }
      while (!path.empty()) {
        if (failed(budget.debit(1, site)))
          return failure();
        auto selection = rawIndex(path, "selection");
        if (path.size() != 2 || !path.getAs<BoolAttr>("when_true") ||
            !selection || *selection >= before || *selection >= g)
          return fail();
        auto parent = dyn_cast<DictionaryAttr>(controls[*selection]);
        if (!validControl(parent) || kindOf(parent).getValue() != "branch_select")
          return fail();
        cost = add(cost, 1);
        if (failed(prefixRelation(parent.getAs<ArrayAttr>("invocation"),
                                  invocation)))
          return failure();
        before = *selection;
        path = parent.getAs<DictionaryAttr>("path");
      }
      return success();
    };
    auto callableCost = [&](ArrayAttr invocation, uint64_t before,
                            bool parent) -> LogicalResult {
      for (uint64_t j = 0; j < before; ++j) {
        if (failed(budget.debit(1, site)))
          return failure();
        cost = add(cost, 1);
        auto candidate = dyn_cast<DictionaryAttr>(controls[j]);
        if (!validControl(candidate))
          return fail();
        if (kindOf(candidate).getValue() != "callable_guard")
          continue;
        auto prior = candidate.getAs<ArrayAttr>("invocation");
        uint64_t requiredLength = parent ? add(prior.size(), 1) : prior.size();
        if (requiredLength != invocation.size())
          continue;
        if (parent ? failed(prefixRelation(prior, invocation))
                   : failed(comparison(prior, invocation)))
          return failure();
      }
      return success();
    };
    if (failed(attributeCost(sampled)) || failed(attributeCost(required)) ||
        failed(attributeCost(site->getAttr("occurrence"))))
      return failure();
    uint64_t frames = 0, k = 0, checkBase = add(*fire, 1);
    for (auto [ordinal, raw] : llvm::enumerate(controls)) {
      if (failed(budget.debit(1, site)))
        return failure();
      auto row = dyn_cast<DictionaryAttr>(raw);
      if (!validControl(row))
        return fail();
      auto id = row.getAs<DictionaryAttr>("id");
      auto invocation = row.getAs<ArrayAttr>("invocation");
      frames = add(frames, invocation.size());
      auto record = dyn_cast<DictionaryAttr>(required[ordinal]);
      if (!record || record.size() != 3 || !record.getAs<DictionaryAttr>("id"))
        return fail();
      if (failed(comparison(id.get("registration"), site->getAttr("occurrence"))) ||
          failed(comparison(record.get("id"), id)) ||
          failed(pathCost(row, invocation, ordinal)))
        return failure();
      for (uint64_t j = 0; j < ordinal; ++j) {
        if (failed(budget.debit(1, site)))
          return failure();
        cost = add(cost, 1);
        auto prior = dyn_cast<DictionaryAttr>(controls[j]);
        if (!validControl(prior))
          return fail();
        auto priorInvocation = prior.getAs<ArrayAttr>("invocation");
        if (kindOf(prior) != kindOf(row) ||
            priorInvocation.size() != invocation.size())
          continue;
        if (failed(comparison(priorInvocation, invocation)))
          return failure();
        // Census all eligible selection-site comparisons without retaining
        // structural identity verdicts or stopping at a successful owner.
        if (kindOf(row).getValue() == "branch_select" &&
            failed(comparison(prior.getAs<DictionaryAttr>("id").get("check"),
                              id.get("check"))))
          return failure();
      }
      if (ordinal && failed(callableCost(
                         invocation, ordinal,
                         kindOf(row).getValue() == "callable_guard")))
        return failure();
    }
    for (StringRef key : {"body_checks", "observations"}) {
      auto rows = sampled.getAs<ArrayAttr>(key);
      for (auto [ordinal, raw] : llvm::enumerate(rows)) {
        if (failed(budget.debit(1, site)))
          return failure();
        auto row = dyn_cast<DictionaryAttr>(raw);
        auto id = row ? row.getAs<DictionaryAttr>("id") : DictionaryAttr();
        if (!row || row.size() != 3 || !id)
          return fail();
        auto invocation = key == "observations" ? id.getAs<ArrayAttr>("invocation")
                                                : row.getAs<ArrayAttr>("invocation");
        if (!invocation)
          return fail();
        frames = add(frames, invocation.size());
        if (failed(attributeCost(invocation)) ||
            failed(comparison(id.get("registration"), site->getAttr("occurrence"))) ||
            failed(callableCost(invocation, g, false)) ||
            failed(pathCost(row, invocation, g)))
          return failure();
        if (key == "body_checks") {
          // Native body-frame validation visits frames already covered by
          // the raw invocation traversal above; reserve only its native fee.
          cost = add(cost, invocation.size());
          auto record = dyn_cast<DictionaryAttr>(required[g + ordinal]);
          if (!record || record.size() != 3 || !record.getAs<DictionaryAttr>("id"))
            return fail();
          if (failed(comparison(record.get("id"), id)))
            return failure();
        } else {
          auto count = rawIndex(row, "value_count");
          if (!count)
            return fail();
          k = add(k, *count);
          checkBase = add(checkBase, add(1, *count));
        }
      }
    }
    uint64_t legacyTotal = add(checkBase, multiply(2, add(g, a)));
    uint64_t total = legacyTotal;
    if (requirements) {
      if (requirements.empty()) return fail();
      cost = add(cost, multiply(requirements.size(), schemaRecordCost<SampledValueRequirement>()));
      for (Attribute raw : requirements) {
        auto row = dyn_cast<DictionaryAttr>(raw);
        auto control = rawIndex(row, "control"), operand = rawIndex(row, "operand");
        if (!row || row.size() != 5 || !control || *control >= g || !operand ||
            !row.getAs<StringAttr>("kind") || !row.getAs<DictionaryAttr>("occurrence") ||
            !row.getAs<DictionaryAttr>("location")) return fail();
      }
    }
    if (resources) {
      if (resources.empty()) return fail();
      cost = add(cost, multiply(resources.size(), schemaRecordCost<SampledResourceView>()));
      uint64_t nextResource = 0;
      for (auto [ordinal, rawControl] : llvm::enumerate(controls)) {
        auto control = cast<DictionaryAttr>(rawControl);
        if (kindOf(control).getValue() != "resource_available") continue;
        if (nextResource == resources.size()) return fail();
        auto row = dyn_cast<DictionaryAttr>(resources[nextResource++]);
        auto controlIndex = rawIndex(row, "control");
        auto kind = kindOf(row);
        if (!row || row.size() != 6 + unsigned(bool(row.get("prospective"))) + unsigned(kind && kind.getValue() == "memory_live") || !controlIndex || *controlIndex != ordinal ||
            !kind || (kind.getValue() != "queue_valid" && kind.getValue() != "queue_ready" && kind.getValue() != "memory_live") ||
            !row.getAs<DictionaryAttr>("owner") || !row.getAs<DictionaryAttr>("declaration") ||
            !row.getAs<DictionaryAttr>("occurrence") || !row.getAs<DictionaryAttr>("location"))
          return fail();
        if (kind.getValue() == "memory_live") {
          if (!rawIndex(row, "port")) return fail();
        } else if (row.get("port")) return fail();
        auto prospective = row.getAs<DictionaryAttr>("prospective");
        if (kind.getValue() != "queue_ready") {
          if (row.get("prospective")) return fail();
        } else {
          if (!prospective || (prospective.size() != 1 && prospective.size() != 4)) return fail();
          auto capacity = rawIndex(prospective, "capacity");
          if (!capacity || *capacity >= site->getNumOperands()) return fail();
          cost = add(cost, schemaRecordCost<SampledQueueProspectiveView>());
          if (prospective.size() == 4) {
            auto valid = rawIndex(prospective, "valid"), external = rawIndex(prospective, "external_pop");
            auto prior = prospective.getAs<ArrayAttr>("prior_pops");
            if (!valid || !external || *valid >= site->getNumOperands() ||
                *external >= site->getNumOperands() || !prior) return fail();
            cost = add(cost, multiply(prior.size(), schemaRecordCost<uint64_t>()));
            std::optional<uint64_t> previous;
            for (Attribute item : prior) {
              auto index = dyn_cast<IntegerAttr>(item);
              if (!index || !index.getType().isSignlessInteger(64) || index.getInt() < 0 ||
                  (previous && uint64_t(index.getInt()) <= *previous)) return fail();
              previous = index.getInt();
            }
          }
        }
      }
      if (nextResource != resources.size()) return fail();
    } else {
      for (Attribute rawControl : controls)
        if (kindOf(cast<DictionaryAttr>(rawControl)).getValue() == "resource_available")
          return fail();
    }
    if (proposals) {
      if (proposals.empty()) return fail();
      cost = add(cost, multiply(proposals.size(), schemaRecordCost<SampledQueueProposalView>()));
      for (auto [ordinal, raw] : llvm::enumerate(proposals)) {
        auto row = dyn_cast<DictionaryAttr>(raw);
        auto kind = kindOf(row);
        auto queue = row ? row.getAs<DictionaryAttr>("queue") : DictionaryAttr();
        auto rawPath = rawIndex(row, "raw_path"), path = rawIndex(row, "path");
        auto resource = rawIndex(row, "resource_control");
        bool push = kind && kind.getValue() == "push";
        auto excluded = row ? row.getAs<ArrayAttr>("excluded_controls") : ArrayAttr();
        if (!row || !kind || (!push && kind.getValue() != "pop") ||
            row.size() != 7 + unsigned(push) + unsigned(bool(excluded)) ||
            !queue || queue.size() != 2 || !queue.getAs<DictionaryAttr>("owner") ||
            !queue.getAs<DictionaryAttr>("declaration") ||
            !row.getAs<DictionaryAttr>("occurrence") || !row.getAs<DictionaryAttr>("location") ||
            !rawPath || !path || *rawPath != total || *path != add(total, 1) ||
            !resource || *resource >= g ||
            kindOf(cast<DictionaryAttr>(controls[*resource])).getValue() != "resource_available" ||
            row.get("ready_from_pop")) return fail();
        if (push) {
          auto data = rawIndex(row, "data");
          if (!data || *data != add(total, 2)) return fail();
        }
        if (excluded) {
          if (excluded.empty()) return fail();
          cost = add(cost, multiply(excluded.size(), schemaRecordCost<uint64_t>()));
          std::optional<uint64_t> previous;
          for (Attribute rawControl : excluded) {
            auto control = dyn_cast<IntegerAttr>(rawControl);
            if (!control || !control.getType().isSignlessInteger(64) || control.getInt() < 0 ||
                uint64_t(control.getInt()) >= g ||
                (previous && uint64_t(control.getInt()) <= *previous)) return fail();
            previous = control.getInt();
          }
        }
        total = add(total, push ? 3 : 2);
        // Each private suffix endpoint is visited by the parser and physical
        // resource verifier. Attribute traversal was prepaid above.
        cost = add(cost, multiply(push ? 3 : 2, schemaRecordCost<Value>()));
      }
    }
    if (memoryProposals) {
      if (memoryProposals.empty()) return fail();
      cost = add(cost, multiply(memoryProposals.size(), schemaRecordCost<SampledMemoryProposalView>()));
      for (Attribute raw : memoryProposals) {
        auto row = dyn_cast<DictionaryAttr>(raw);
        auto kind = kindOf(row);
        auto memory = row ? row.getAs<DictionaryAttr>("memory") : DictionaryAttr();
        auto port = rawIndex(row, "port"), rawPath = rawIndex(row, "raw_path"), path = rawIndex(row, "path");
        auto arguments = row ? row.getAs<ArrayAttr>("arguments") : ArrayAttr();
        if (!row || row.size() != 8 || !kind || !memory || memory.size() != 2 ||
            !memory.getAs<DictionaryAttr>("owner") || !memory.getAs<DictionaryAttr>("declaration") ||
            !row.getAs<DictionaryAttr>("occurrence") || !row.getAs<DictionaryAttr>("location") ||
            !port || !rawPath || !path || *rawPath != total || *path != add(total, 1) || !arguments ||
            (kind.getValue() != "request" && kind.getValue() != "read" && kind.getValue() != "write") ||
            arguments.size() != (kind.getValue() == "write" ? 3 : 1)) return fail();
        for (auto [ordinal, rawArgument] : llvm::enumerate(arguments)) {
          auto argument = dyn_cast<IntegerAttr>(rawArgument);
          if (!argument || !argument.getType().isSignlessInteger(64) || argument.getInt() < 0 ||
              uint64_t(argument.getInt()) != add(total, add(2, ordinal))) return fail();
        }
        cost = add(cost, multiply(arguments.size(), add(schemaRecordCost<uint64_t>(), schemaRecordCost<Value>())));
        cost = add(cost, multiply(add(2, arguments.size()), schemaRecordCost<Value>()));
        total = add(total, add(2, arguments.size()));
      }
    }
    if (domains) {
      if (domains.empty()) return fail();
      cost = add(cost, multiply(domains.size(), schemaRecordCost<SampledWriteDomainView>()));
      for (Attribute raw : domains) {
        if (failed(budget.debit(1, site))) return failure();
        auto row = dyn_cast<DictionaryAttr>(raw);
        auto candidate = rawIndex(row, "candidate"), mask = rawIndex(row, "mask");
        auto selectors = row ? row.getAs<ArrayAttr>("selectors") : ArrayAttr();
        if (!row || row.size() != 3 || !candidate || !mask || !selectors || *mask != total)
          return fail();
        unsigned indices = 0;
        for (Attribute rawSelector : selectors) {
          if (failed(budget.debit(1, site))) return failure();
          cost = add(cost, schemaRecordCost<Attribute>());
          auto selector = dyn_cast<DictionaryAttr>(rawSelector);
          if (!selector || selector.size() != 1) return fail();
          if (selector.getAs<StringAttr>("field")) continue;
          auto index = rawIndex(selector, "index");
          if (!index || *index != indices++ || indices > 1) return fail();
        }
        total = add(total, 1);
      }
    }
    if (total == std::numeric_limits<uint64_t>::max() ||
        total != site->getNumResults() || site->getNumRegions() != 1 ||
        !site->getRegion(0).hasOneBlock() || site->getRegion(0).front().empty() ||
        site->getRegion(0).front().back().getNumOperands() != total)
      return fail();
    cost = add(cost, multiply(g, schemaRecordCost<SampledControlView>()));
    cost = add(cost, multiply(a, schemaRecordCost<SampledBodyCheckView>()));
    cost = add(cost, multiply(n, schemaRecordCost<SampledObservationView>()));
    cost = add(cost, multiply(k, schemaRecordCost<Value>()));
    cost = add(cost, multiply(frames, schemaRecordCost<DictionaryAttr>()));
    using StaticPathFact = std::pair<std::optional<bool>, std::optional<bool>>;
    cost = add(cost, multiply(g, schemaRecordCost<StaticPathFact>()));
    return add(cost, add(add(add(3, multiply(2, g)), multiply(2, a)),
                         add(n, multiply(2, k))));
  }
  FailureOr<uint64_t> queueWiringSchemaCost(Attribute attribute, Operation *site) {
    auto row = dyn_cast_or_null<DictionaryAttr>(attribute);
    auto fail = [&]() -> LogicalResult {
      return site->emitOpError() << "queue wiring raw layout requires closed queue/incoming/writer rows";
    };
    auto exactFields = [](DictionaryAttr row, ArrayRef<StringRef> fields) {
      return row && row.size() == fields.size() &&
             llvm::all_of(fields, [&](StringRef key) { return bool(row.get(key)); });
    };
    auto index = [](DictionaryAttr row, StringRef key) -> std::optional<uint64_t> {
      auto value = row ? row.getAs<IntegerAttr>(key) : IntegerAttr();
      if (!value || !value.getType().isSignlessInteger(64) || value.getInt() < 0)
        return std::nullopt;
      return value.getInt();
    };
    auto queueID = [&](DictionaryAttr queue) {
      auto owner = queue ? queue.getAs<DictionaryAttr>("owner") : DictionaryAttr();
      return exactFields(queue, {"owner", "declaration"}) &&
             exactFields(owner, {"instance_path"}) &&
             owner.getAs<ArrayAttr>("instance_path") &&
             queue.getAs<DictionaryAttr>("declaration");
    };
    auto queues = row ? row.getAs<ArrayAttr>("queues") : ArrayAttr();
    auto incoming = row ? row.getAs<ArrayAttr>("incoming") : ArrayAttr();
    auto writers = row ? row.getAs<ArrayAttr>("writers") : ArrayAttr();
    auto summaries = row ? row.getAs<ArrayAttr>("pop_summaries") : ArrayAttr();
    auto external = row ? row.getAs<ArrayAttr>("external_pops") : ArrayAttr();
    if (!row || row.size() != 3 + unsigned(bool(summaries)) + unsigned(bool(external)) ||
        !queues || !incoming || !writers ||
        (row.get("pop_summaries") && (!summaries || summaries.empty())) ||
        (row.get("external_pops") && (!external || external.empty()))) return fail();
    uint64_t cost = 0;
    auto chargeRow = [&](uint64_t fields) -> LogicalResult {
      cost = Budget::add(cost, Budget::multiply(fields,
          schemaRecordCost<Attribute>()));
      return budget.debit(1, site);
    };
    for (Attribute raw : queues) {
      if (failed(chargeRow(3))) return failure();
      auto queue = dyn_cast<DictionaryAttr>(raw);
      if (!exactFields(queue, {"declaration", "unbound_push", "unbound_pop"}) ||
          !queue.getAs<DictionaryAttr>("declaration") ||
          !queue.getAs<BoolAttr>("unbound_push") ||
          !queue.getAs<BoolAttr>("unbound_pop")) return fail();
    }
    auto route = [&](DictionaryAttr route, uint64_t limit) -> LogicalResult {
      if (failed(chargeRow(4))) return failure();
      auto kind = route ? route.getAs<StringAttr>("kind") : StringAttr();
      bool push = kind && kind.getValue() == "push";
      auto path = index(route, "path");
      if (!route || !kind || (!push && kind.getValue() != "pop") ||
          !queueID(route.getAs<DictionaryAttr>("queue")) || !path || *path >= limit ||
          (push ? !exactFields(route, {"queue", "kind", "path", "data"})
                : !exactFields(route, {"queue", "kind", "path"}))) return fail();
      if (push) {
        auto data = index(route, "data");
        if (!data || *data >= limit) return fail();
      }
      return success();
    };
    auto type = site->getAttrOfType<TypeAttr>("function_type");
    auto signature = type ? dyn_cast<FunctionType>(type.getValue()) : FunctionType();
    if (!signature) return fail();
    for (Attribute raw : incoming)
      if (failed(route(dyn_cast<DictionaryAttr>(raw), signature.getNumInputs())))
        return failure();
    if (summaries) for (Attribute raw : summaries) {
      if (failed(chargeRow(2))) return failure();
      auto entry = dyn_cast<DictionaryAttr>(raw);
      auto output = index(entry, "output");
      if (!exactFields(entry, {"queue", "output"}) ||
          !queueID(entry.getAs<DictionaryAttr>("queue")) || !output ||
          *output >= signature.getNumResults()) return fail();
    }
    if (external) for (Attribute raw : external) {
      if (failed(chargeRow(3))) return failure();
      auto entry = dyn_cast<DictionaryAttr>(raw);
      if (!exactFields(entry, {"registration", "queue", "capture"}) ||
          !entry.getAs<DictionaryAttr>("registration") ||
          !queueID(entry.getAs<DictionaryAttr>("queue")) || !index(entry, "capture")) return fail();
    }
    for (Attribute raw : writers) {
      if (failed(chargeRow(2))) return failure();
      auto writer = dyn_cast<DictionaryAttr>(raw);
      auto proposals = writer ? writer.getAs<ArrayAttr>("proposals") : ArrayAttr();
      if (!exactFields(writer, {"registration", "proposals"}) ||
          !writer.getAs<DictionaryAttr>("registration") || !proposals || proposals.empty())
        return fail();
      // Rule-specific result membership belongs to the typed queue-wiring
      // verifier; this raw pass only checks bounded nonnegative route syntax.
      for (Attribute rawProposal : proposals)
        if (failed(route(dyn_cast<DictionaryAttr>(rawProposal),
                         std::numeric_limits<uint64_t>::max()))) return failure();
    }
    return cost;
  }
  LogicalResult countWiringSchema(Attribute attribute, RawDefinitionWork &local,
                                  Operation *site) {
    auto wiring = dyn_cast_or_null<DictionaryAttr>(attribute);
    if (!wiring)
      return success();
    if (auto states = wiring.getAs<ArrayAttr>("states"))
      local.states = states.size();
    if (auto incoming = wiring.getAs<ArrayAttr>("incoming"))
      local.incoming = incoming.size();
    if (auto writers = wiring.getAs<ArrayAttr>("writers")) {
      local.writers = writers.size();
      for (Attribute raw : writers) {
        if (failed(budget.debit(1, site)))
          return failure();
        auto writer = dyn_cast<DictionaryAttr>(raw);
        if (auto candidates =
                writer ? writer.getAs<ArrayAttr>("candidates") : ArrayAttr()) {
          local.candidates = Budget::add(local.candidates, candidates.size());
          local.maxCandidates =
              std::max(local.maxCandidates, uint64_t(candidates.size()));
        }
      }
    }
    return success();
  }
  static uint64_t wiringSchemaCost(const RawDefinitionWork &local,
                                   bool includeSampled = true) {
    if (!local.hasWiring)
      return 0;
    auto add = Budget::add;
    auto multiply = Budget::multiply;
    uint64_t rows = std::max(
        {uint64_t(1), local.states, local.incoming, local.maxCandidates});
    uint64_t cost = schemaRecordCost<TransactionWiringView>();
    cost =
        add(cost, multiply(local.states,
                           schemaRecordCost<TransactionStateInventoryEntry>()));
    cost = add(cost, multiply(local.incoming,
                              schemaRecordCost<TransactionIncomingView>()));
    cost = add(cost, multiply(local.writers,
                              schemaRecordCost<TransactionWriterView>()));
    cost = add(cost, multiply(local.candidates,
                              schemaRecordCost<TransactionCandidateView>()));
    cost = add(cost, multiply(rows, local.wiringMetadata));
    cost = add(
        cost, multiply(local.states, add(local.operations,
                                         multiply(2, local.instanceMetadata))));
    cost = add(cost, multiply(local.instances, local.wiringMetadata));
    cost = add(
        cost, multiply(local.writers,
                       add(local.operations, multiply(2, local.ruleMetadata))));
    cost = add(cost, multiply(local.rules, local.wiringMetadata));
    if (local.writers) {
      // Divide the even factor first: saturation before division would lose
      // the conservative triangle bound for very large raw inventories.
      uint64_t left = local.writers, right = local.writers - 1;
      if (left % 2 == 0)
        left /= 2;
      else
        right /= 2;
      cost = add(cost, multiply(left, right));
    }
    cost = add(cost,
               add(add(local.incoming, local.candidates),
                   includeSampled ? local.sampledSchema : 0));
    return add(cost, multiply(local.rules, add(local.writers, 1)));
  }
  LogicalResult rawPreflight() {
    // Do not invoke typed attribute/signature accessors or any op verifier.
    // ModuleOp's region verifier constructs the local association catalog, so
    // its record payload and comparison work must be prepaid first.
    llvm::DenseMap<Operation *, RawDefinitionWork> work;
    if (failed(budget.debit(
            Budget::bytes(sizeof(Operation *) + sizeof(RawDefinitionWork *)),
            analysis.getPackage())))
      return failure();
    Operation *lastDefinition = nullptr;
    RawDefinitionWork *lastWork = nullptr;
    auto counted = analysis.getPackage().walk([&](Operation *op) {
      if (failed(budget.debit(1, op)))
        return WalkResult::interrupt();
      Operation *definition = isa<ModuleOp>(op) ? op : nullptr;
      for (Operation *parent = op->getParentOp(); !definition && parent;
           parent = parent->getParentOp()) {
        if (failed(budget.debit(1, op)))
          return WalkResult::interrupt();
        if (isa<ModuleOp>(parent))
          definition = parent;
      }
      if (!definition) {
        lastDefinition = nullptr;
        lastWork = nullptr;
        return WalkResult::advance();
      }
      if (failed(budget.debit(1, op)))
        return WalkResult::interrupt();
      if (definition != lastDefinition) {
        // Reuse only the immediately preceding owner's accumulator. A change
        // of owner invalidates both pointers before any possible map growth.
        lastDefinition = nullptr;
        lastWork = nullptr;
        using Bucket = decltype(work)::value_type;
        uint64_t oldCapacity = work.getMemorySize() / sizeof(Bucket);
        // Insert-only DenseMap probes are bounded by the complete bucket array.
        // Pay before even the existing-key lookup can inspect that array.
        if (failed(budget.debit(Budget::add(oldCapacity, 1), op)))
          return WalkResult::interrupt();
        auto found = work.find(definition);
        if (found == work.end()) {
          if (failed(budget.debit(1, op)))
            return WalkResult::interrupt();
          uint64_t next = Budget::add(work.size(), 1);
          uint64_t capacity = std::max(uint64_t(64), oldCapacity);
          while (true) {
            if (failed(budget.debit(1, op)))
              return WalkResult::interrupt();
            // LLVM's reserve and load calculations use unsigned arithmetic.
            if (next > std::numeric_limits<unsigned>::max() / 4 ||
                capacity > std::numeric_limits<unsigned>::max() / 3) {
              (void)budget.debit(std::numeric_limits<uint64_t>::max(), op);
              return WalkResult::interrupt();
            }
            if (Budget::multiply(4, next) < Budget::multiply(3, capacity))
              break;
            capacity = Budget::multiply(2, capacity);
          }
          uint64_t growth = 1;
          if (capacity > oldCapacity) {
            growth = Budget::add(
                growth,
                Budget::add(
                    Budget::bytes(Budget::multiply(capacity, sizeof(Bucket))),
                    Budget::add(capacity,
                                Budget::add(oldCapacity,
                                            Budget::multiply(
                                                work.size(),
                                                Budget::add(capacity, 1))))));
          }
          if (failed(budget.debit(growth, op)))
            return WalkResult::interrupt();
          work.reserve(next);
          if (failed(budget.debit(Budget::add(capacity, 1), op)))
            return WalkResult::interrupt();
          found = work.try_emplace(definition).first;
        }
        lastDefinition = definition;
        lastWork = &found->second;
      }
      auto &local = *lastWork;
      local.operations = Budget::add(local.operations, 1);
      bool directRule = isa<RuleOp>(op) && op->getParentOp() == definition;
      bool directInstance =
          isa<InstanceOp>(op) && op->getParentOp() == definition;
      if (directRule) {
        local.rules = Budget::add(local.rules, 1);
        local.ruleResults = Budget::add(local.ruleResults, op->getNumResults());
        if (op->hasAttr("ac.sampled_admission"))
          local.sampledRules = Budget::add(local.sampledRules, 1);
      }
      if (directInstance)
        local.instances = Budget::add(local.instances, 1);
      local.operands = Budget::add(local.operands, op->getNumOperands());
      if (isa<SourceObserveOp>(op)) {
        local.observations = Budget::add(local.observations, 1);
        local.observationOperands =
            Budget::add(local.observationOperands, op->getNumOperands());
        for (StringRef key :
             {"ac.observation_id", "ac.location", "spec", "kind"}) {
          auto cost = structureCost(op->getAttr(key), op);
          if (failed(cost))
            return WalkResult::interrupt();
          local.observationMetadata =
              Budget::add(local.observationMetadata, *cost);
        }
      }
      for (StringRef key :
           {"occurrence", "ac.required_checks", "ac.check_id", "location",
            "ac.message", "ac.sampled_admission", "ac.transaction_wiring",
            "ac.queue_wiring", "ac.system_samples", "ac.registration_path",
            "ac.lexical_registration", "ac.registration_owner",
            "ac.transaction_returns"}) {
        Attribute attribute = op->getAttr(key);
        auto cost = structureCost(attribute, op);
        if (failed(cost))
          return WalkResult::interrupt();
        local.metadata = Budget::add(local.metadata, *cost);
        uint64_t metadata = std::max(uint64_t(1), *cost);
        if (key == "occurrence") {
          if (directInstance)
            local.instanceMetadata =
                Budget::add(local.instanceMetadata, metadata);
          if (directRule)
            local.ruleMetadata = Budget::add(local.ruleMetadata, metadata);
        }
        if (attribute &&
            (key == "ac.sampled_admission" || key == "ac.transaction_wiring" ||
             key == "ac.queue_wiring"))
          local.hasSamplingMetadata = true;
        if (directRule && key == "ac.sampled_admission" && attribute) {
          auto schema = sampledSchemaCost(attribute, op);
          if (failed(schema))
            return WalkResult::interrupt();
          local.sampledSchema = Budget::add(local.sampledSchema, *schema);
        }
        if (key == "ac.queue_wiring" && attribute) {
          auto package = analysis.getPackage();
          auto stage = package->getAttrOfType<StringAttr>("ac.stage");
          if (op != definition || !stage ||
              (stage.getValue() != "final" && stage.getValue() != "routed")) {
            op->emitOpError() << "ac.queue_wiring requires its owning module in a routed or final package";
            return WalkResult::interrupt();
          }
          auto schema = queueWiringSchemaCost(attribute, op);
          if (failed(schema)) return WalkResult::interrupt();
          local.sampledSchema = Budget::add(local.sampledSchema, *schema);
        }
        if (op == definition && key == "ac.transaction_wiring" && attribute) {
          local.hasWiring = true;
          local.wiringMetadata = metadata;
          if (failed(countWiringSchema(attribute, local, op)))
            return WalkResult::interrupt();
        }
      }
      if (failed(countAssociationOperands(
              op, local, directRule,
              isa<SourceExpectOp>(op) && op->getParentOp() == definition)))
        return WalkResult::interrupt();
      if (auto required = op->getAttrOfType<ArrayAttr>("ac.required_checks"))
        local.obligations = Budget::add(local.obligations, required.size());
      if (isa<SourceExpectOp>(op)) {
        for (unsigned index = 0; index < op->getNumOperands(); ++index) {
          Value operand = op->getOperand(index);
          auto cost = rawCarrierCost(operand, op);
          if (failed(cost))
            return WalkResult::interrupt();
          local.carriers = Budget::add(local.carriers, *cost);
        }
      }
      return WalkResult::advance();
    });
    if (counted.wasInterrupted())
      return failure();
    // DenseMap iteration scans spare buckets as well as occupied records.
    if (failed(budget.debit(
            Budget::add(1, work.getMemorySize() / sizeof(decltype(work)::value_type)),
            analysis.getPackage())))
      return failure();
    for (const auto &[definition, local] : work) {
      if (failed(budget.debit(1, definition)))
        return failure();
      auto comparisons = checkComparisonAllowance(local.checkKeys, definition);
      if (failed(comparisons))
        return failure();
      uint64_t mapCost = Budget::add(
          local.keyInsertion,
          Budget::multiply(
              *comparisons,
              Budget::add(local.terminalKeys,
                          Budget::add(local.terminalKeyCost,
                                      Budget::multiply(local.terminalKeys,
                                                       local.keyMax)))));
      if (local.rules == local.sampledRules && local.distinctRegistrations) {
        // Distinct authenticated sampled registrations have no CheckID tree: each
        // RequiredCheck occupies its proved row ordinal. Terminal lookup pays
        // one complete semantic ID comparison, including width-equivalent
        // integer attributes, instead of repeated tree comparisons.
        mapCost = Budget::add(
            Budget::multiply(3, local.checkKeys),
            Budget::add(Budget::multiply(3, local.terminalKeys),
                        Budget::add(local.terminalKeyCost,
                                    Budget::multiply(local.terminalKeys,
                                                     local.keyMax))));
      }
      auto registrationComparisons =
          checkComparisonAllowance(local.registrationGroups, definition);
      if (failed(registrationComparisons))
        return failure();
      mapCost = Budget::add(
          mapCost,
          Budget::add(local.registrationInsertion,
                      Budget::multiply(
                          *registrationComparisons,
                          Budget::add(local.terminalKeys,
                                      Budget::add(local.terminalRegistrationCost,
                                                  Budget::multiply(local.terminalKeys,
                                                                   local.registrationMax))))));
      uint64_t validationMetadata =
          Budget::add(local.metadata, local.validationOperands);
      uint64_t records = Budget::multiply(
          local.obligations,
          Budget::bytes(2 * sizeof(HardwareCheckBinding) +
                        sizeof(std::pair<DictionaryAttr, size_t>) +
                        3 * sizeof(void *)));
      records = Budget::add(
          records,
          Budget::multiply(local.registrationGroups,
                           Budget::bytes(sizeof(std::pair<DictionaryAttr, void *>) +
                                         16 * sizeof(void *))));
      records = Budget::add(
          records,
          Budget::add(local.obligations,
                      Budget::multiply(local.sampledRules,
                                       schemaRecordCost<Operation *>())));
      uint64_t cost = Budget::add(
          local.operations,
          Budget::add(records,
                      Budget::add(validationMetadata,
                                  Budget::add(mapCost, local.carriers))));
      const uint64_t n = local.observations;
      const uint64_t j = local.observationMetadata;
      const uint64_t s = Budget::add(
          j, Budget::add(Budget::multiply(2, local.observationOperands),
                         Budget::multiply(8, n)));
      uint64_t left = n, right = n ? n - 1 : 0;
      if (left % 2 == 0)
        left /= 2;
      else
        right /= 2;
      const uint64_t pairs = Budget::multiply(left, right);
      const uint64_t observationReserve = Budget::add(
          Budget::add(Budget::multiply(2, s), j),
          Budget::add(
              Budget::multiply(n, schemaRecordCost<ObservationLocalKey>()),
              Budget::add(Budget::multiply(6, pairs), Budget::multiply(2, n))));
      cost = Budget::add(cost, observationReserve);
      // Opaque native association/metadata scans are prepaid here. Sampled
      // schema, identity and prefix proofs debit their actual work through the
      // synchronous NativeSourceCheckScope; no speculative duplicate reserve.
      // Native operation verifiers still traverse the raw operand inventories.
      if (local.hasSamplingMetadata) cost = Budget::add(cost, local.operands);
      if (failed(budget.debit(cost, definition))) return failure();
      if (failed(budget.debit(Budget::bytes(sizeof(std::pair<Operation *, uint64_t>)),
                              definition))) return failure();
      // Different bound invocations still repeat opaque local validation;
      // retain that allowance without replaying the default-binding catalog.
      localValidationCost[definition] = Budget::add(local.operations,
          Budget::add(records, Budget::add(validationMetadata,
                                          Budget::add(mapCost, local.carriers))));
    }
    return success();
  }
  LogicalResult catalogSampling(ModuleOp definition,
                                const HardwareBindings &bindings,
                                SmallVector<HardwareCheckBinding> &checks) {
    // Body assertions are checked on every Work evaluation with ordinary reset
    // masking. Only guard-known checks acquire an edge sampling binding.
    if (!llvm::any_of(checks, [](const auto &check) {
          return check.kind.getValue() == "guard_known";
        }))
      return success();
    if (failed(budget.debit(Budget::bytes(sizeof(
                                std::pair<Operation *, SmallVector<uint64_t>>)),
                            definition)))
      return failure();
    auto &indices = localSamplingIndices[definition];
    auto prepared = validation.prepare(definition, bindings);
    if (failed(prepared))
      return failure();
    if (!*prepared)
      return definition->emitOpError()
             << "sampling catalog lacks prepared boundaries";
    const auto &scope = (*prepared)->getScope();
    HardwareIdentityStopPolicy stops{HardwareIdentityTraversal::DefinitionLocal,
                                     (*prepared)->getBoundaries()};
    // Repeated controls of one Rule share a binding. Distinct Rules share only
    // exactly equal normalized definition-local SSA clock/reset pairs.
    llvm::DenseMap<Operation *, uint64_t> ruleIndices;
    for (auto &check : checks) {
      if (check.kind.getValue() != "guard_known")
        continue;
      auto known = ruleIndices.find(check.rule);
      if (known != ruleIndices.end()) {
        check.samplingBindingIndex = known->second;
        continue;
      }
      auto sampled =
          parseSampledAdmission(check.rule, analysis, bindings, budget);
      if (failed(sampled))
        return failure();
      auto clock =
          makeIdentityEndpoint(sampled->clock, scope, analysis, budget);
      auto reset =
          makeIdentityEndpoint(sampled->reset, scope, analysis, budget);
      if (failed(clock) || failed(reset))
        return failure();
      auto normalizedClock =
          normalizeIdentityEndpoint(*clock, stops, analysis, budget);
      auto normalizedReset =
          normalizeIdentityEndpoint(*reset, stops, analysis, budget);
      if (failed(normalizedClock) || failed(normalizedReset))
        return failure();
      std::optional<uint64_t> found;
      for (uint64_t index : indices) {
        if (failed(budget.debit(1, definition)))
          return failure();
        const auto &binding = plan.samplingBindings[index];
        if (binding.clock == normalizedClock->value &&
            binding.reset == normalizedReset->value) {
          found = index;
          break;
        }
      }
      if (!found) {
        if (failed(budget.debit(
                Budget::add(Budget::bytes(sizeof(HardwareCheckSamplingBinding)),
                            Budget::bytes(sizeof(uint64_t))),
                definition)))
          return failure();
        found = plan.samplingBindings.size();
        plan.samplingBindings.push_back({definition, normalizedClock->value,
                                         normalizedReset->value,
                                         indices.size()});
        indices.push_back(*found);
      }
      if (failed(budget.debit(
              Budget::bytes(sizeof(std::pair<Operation *, uint64_t>)),
              check.rule)))
        return failure();
      ruleIndices.try_emplace(check.rule, *found);
      check.samplingBindingIndex = *found;
    }
    return success();
  }
  LogicalResult catalog() {
    for (auto definition : analysis.getPackage().getOps<ModuleOp>()) {
      HardwareBindings local;
      local.owner = definition;
      if (failed(budget.debit(
              Budget::add(
                  Budget::bindings(local),
                  Budget::bytes(sizeof(std::pair<ModuleOp, HardwareBindings>) +
                                sizeof(uint64_t))),
              definition)))
        return failure();
      catalogScopes.emplace_back(definition, local);
      if (failed(budget.debit(1, definition))) return failure();
      auto native = nativeChecks.find(definition);
      if (native == nativeChecks.end())
        return definition.emitOpError() << "native verification omitted the current definition";
      auto bindings = std::move(native->second);
      nativeChecks.erase(native);
      if (failed(
              budget.debit(Budget::bytes(sizeof(
                               std::pair<Operation *, SmallVector<uint64_t>>)),
                           definition)))
        return failure();
      auto &indices = localIndices[definition];
      auto sampling = catalogSampling(definition, local, bindings);
      if (failed(sampling))
        return failure();
      if (failed(budget.debit(Budget::multiply(bindings.size(),
                                               Budget::bytes(sizeof(uint64_t))),
                              definition)))
        return failure();
      for (const auto &binding : bindings) {
        indices.push_back(plan.bindings.size());
        plan.bindings.push_back(binding);
      }
    }
    return success();
  }
  FailureOr<size_t> visit(ModuleOp definition, const HardwareBindings &scope) {
    if (!active.insert(definition).second)
      return definition.emitOpError()
             << "recursive source check owner hierarchy";
    llvm::scope_exit cleanup([&] { active.erase(definition); });
    for (size_t index = 0; index < nodes.size(); ++index) {
      if (failed(budget.debit(Budget::add(1, Budget::bindings(scope)),
                              definition)))
        return failure();
      if (nodes[index]->definition == definition &&
          sameHardwareBindings(nodes[index]->bindings, scope))
        return index;
    }
    if (failed(budget.debit(Budget::add(Budget::bytes(sizeof(BoundDefinition)),
                                        Budget::bindings(scope)),
                            definition)))
      return failure();
    auto node = std::make_unique<BoundDefinition>();
    node->definition = definition;
    node->bindings = scope;
    if (failed(budget.debit(localIndices[definition].size(), definition)))
      return failure();
    node->checkIndices = localIndices[definition];
    auto sampling = localSamplingIndices.find(definition);
    if (sampling != localSamplingIndices.end()) {
      if (failed(budget.debit(Budget::multiply(sampling->second.size(),
                                               Budget::bytes(sizeof(uint64_t))),
                              definition)))
        return failure();
      node->samplingIndices = sampling->second;
    }
    node->hasChecks = !node->checkIndices.empty();
    size_t index = nodes.size();
    nodes.push_back(std::move(node));
    auto &bound = *nodes[index];
    for (Type type : definition.getFunctionType().getInputs())
      if (failed(analysis.getPackedWidth(type, scope, definition)))
        return failure();
    for (Type type : definition.getFunctionType().getResults())
      if (failed(analysis.getPackedWidth(type, scope, definition)))
        return failure();
    if (failed(verifyModuleSourceCallContract(definition)))
      return failure();
    auto domain = definition->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
    if (domain && !domain.empty()) {
      auto ordinal =
          decodeU64(domain.getAs<IntegerAttr>("reset"), "reset input ordinal",
                    [&] { return definition.emitOpError(); });
      if (failed(ordinal) ||
          *ordinal >= definition.getFunctionType().getNumInputs() ||
          *ordinal > std::numeric_limits<unsigned>::max())
        return definition.emitOpError()
               << "source check reset input is outside owner signature";
      auto width = analysis.getPackedWidth(
          definition.getFunctionType().getInput(*ordinal), scope, definition);
      if (failed(width) || *width != 1)
        return definition.emitOpError()
               << "source check reset input must have width one";
      bound.reset = static_cast<unsigned>(*ordinal);
    }
    auto resolved = definition.walk([&](Operation *op) {
      if (failed(budget.debit(1, op)))
        return WalkResult::interrupt();
      if (op == definition.getOperation() || isa<YieldOp>(op))
        return WalkResult::advance();
      return failed(analysis.verifyResolvedOperation(op, scope))
                 ? WalkResult::interrupt()
                 : WalkResult::advance();
    });
    if (resolved.wasInterrupted())
      return failure();
    // Catalog proofs are complete before visit, and the package stays
    // immutable. Reuse only the same definition with exactly equal bindings.
    bool catalogProved = false;
    for (const auto &[catalogDefinition, catalogBindings] : catalogScopes) {
      if (failed(budget.debit(1, definition)))
        return failure();
      if (catalogDefinition != definition)
        continue;
      if (failed(budget.debit(Budget::add(Budget::bindings(scope),
                                          Budget::bindings(catalogBindings)),
                              definition)))
        return failure();
      catalogProved = sameHardwareBindings(scope, catalogBindings);
      break;
    }
    if (!catalogProved) {
      if (failed(budget.debit(localValidationCost[definition], definition)))
        return failure();
      auto local = definitionSourceChecks(analysis, definition, scope,
                                          validation, budget);
      if (failed(local))
        return failure();
    }
    SmallVector<Operation *> allocations;
    for (Operation &op : definition.getBody().front()) {
      if (!isa<InstanceOp, CollectionOp, SourceStateOp>(op))
        continue;
      if (failed(budget.debit(Budget::bytes(sizeof(Allocation)), &op)) ||
          failed(
              verifyOccurrence(op.getAttrOfType<DictionaryAttr>("occurrence"),
                               [&] { return op.emitOpError(); })))
        return failure();
      allocations.push_back(&op);
    }
    // Sorting depends on definition-local identities, never lane coordinates.
    if (failed(budget.debit(
            Budget::multiply(allocations.size(), allocations.size()),
            definition)))
      return failure();
    llvm::sort(allocations, [](Operation *lhs, Operation *rhs) {
      return compareClosedSourceStructure(lhs->getAttr("occurrence"),
                                          rhs->getAttr("occurrence")) < 0;
    });
    for (size_t i = 1; i < allocations.size(); ++i)
      if (compareClosedSourceStructure(
              allocations[i - 1]->getAttr("occurrence"),
              allocations[i]->getAttr("occurrence")) == 0)
        return allocations[i]->emitOpError()
               << "duplicate allocation occurrence identity";
    for (Operation *allocation : allocations) {
      Allocation edge{allocation, {}, {}, {}};
      if (isa<SourceStateOp>(allocation)) {
        edge.form = HardwareCheckCommitEndpointForm::Logical;
      } else if (auto queue = dyn_cast<InstanceOp>(allocation);
          queue && isQueueInstance(queue)) {
        if (failed(analysis.resolveQueue(queue, scope)))
          return failure();
        edge.primitive = StringAttr::get(definition.getContext(), "fifo");
      } else {
        if (failed(isa<InstanceOp>(allocation)
                       ? analysis.verifyInstance(cast<InstanceOp>(allocation),
                                                 scope)
                       : analysis.verifyInstance(cast<CollectionOp>(allocation),
                                                 scope)))
          return failure();
        auto child =
            isa<InstanceOp>(allocation)
                ? analysis.bindInstance(cast<InstanceOp>(allocation), scope)
                : analysis.bindInstance(cast<CollectionOp>(allocation), scope);
        if (failed(child))
          return failure();
        if (failed(budget.debit(Budget::bindings(*child), allocation)))
          return failure();
        for (const auto &type : child->types)
          if (failed(analysis.getPackedWidth(type.second, *child, allocation)))
            return failure();
        for (Type type : allocation->getOperandTypes())
          if (failed(analysis.getPackedWidth(type, scope, allocation)))
            return failure();
        for (Type type : allocation->getResultTypes())
          if (failed(analysis.getPackedWidth(type, scope, allocation)))
            return failure();
        if (auto collection = dyn_cast<CollectionOp>(allocation)) {
          if (failed(budget.debit(collection.getShape().size(), allocation)))
            return failure();
          auto shape =
              analysis.resolveShape(collection.getShape(), scope, collection);
          if (failed(shape))
            return failure();
          edge.shape = std::move(*shape);
        }
        auto primitive = analysis.getPrimitiveKind(child->owner);
        if (!primitive.empty()) {
          if (failed(analysis.verifyImport(cast<ModuleImportOp>(child->owner))))
            return failure();
          edge.primitive = StringAttr::get(definition.getContext(), primitive);
        } else {
          auto childDefinition = dyn_cast<ModuleOp>(child->owner);
          if (!childDefinition) {
            if (profile != PackageVerificationProfile::Routed ||
                !child->owner->hasAttr("ac.external_contract"))
              return allocation->emitOpError()
                     << "source check plan requires explicit provider body";
            edge.combinationalExternal = true;
            bound.allocations.push_back(std::move(edge));
            continue;
          }
          auto childIndex = visit(childDefinition, *child);
          if (failed(childIndex))
            return failure();
          edge.child = *childIndex;
          bound.hasChecks |= nodes[*childIndex]->hasChecks;
        }
      }
      bound.allocations.push_back(std::move(edge));
    }
    return index;
  }
  static uint64_t stepCost(const Allocation &edge) {
    return Budget::add(Budget::bytes(sizeof(HardwareCheckOccurrenceStep)),
                       edge.shape.size());
  }
  static uint64_t ownerCost(const BoundDefinition &node) {
    return Budget::add(
        1, Budget::add(Budget::bytes(sizeof(HardwareCheckOwnerOccurrence)),
                       Budget::bindings(node.bindings)));
  }
  FailureOr<Expansion> summarize(size_t index) {
    auto &node = *nodes[index];
    if (failed(budget.debit(1, node.definition)))
      return failure();
    if (node.expansion)
      return *node.expansion;
    Expansion result;
    result.checks = node.checkIndices.size();
    result.sampling = node.samplingIndices.size();
    result.cost = Budget::add(
        ownerCost(node),
        Budget::multiply(result.checks,
                         Budget::add(1, Budget::bytes(sizeof(
                                            HardwareResolvedSourceCheck)))));
    result.cost = Budget::add(result.cost,
                              Budget::multiply(result.checks, result.sampling));
    result.cost = Budget::add(
        result.cost,
        Budget::multiply(
            result.sampling,
            Budget::add(
                1, Budget::bytes(sizeof(HardwareCheckSamplingOccurrence)))));
    for (auto &edge : node.allocations) {
      if (edge.combinationalExternal) continue;
      for (uint64_t extent : edge.shape)
        edge.count = Budget::multiply(edge.count, extent);
      if (edge.child) {
        auto child = summarize(*edge.child);
        if (failed(child))
          return failure();
        result.owners = Budget::add(
            result.owners, Budget::multiply(edge.count, child->owners));
        result.checks = Budget::add(
            result.checks, Budget::multiply(edge.count, child->checks));
        result.commits = Budget::add(
            result.commits, Budget::multiply(edge.count, child->commits));
        result.sampling = Budget::add(
            result.sampling, Budget::multiply(edge.count, child->sampling));
        uint64_t cost = Budget::add(
            edge.shape.size(),
            Budget::add(child->cost,
                        Budget::multiply(Budget::multiply(2, child->owners),
                                         stepCost(edge))));
        result.cost =
            Budget::add(result.cost, Budget::multiply(edge.count, cost));
      } else {
        result.commits = Budget::add(result.commits, edge.count);
        uint64_t cost = Budget::add(
            1, Budget::add(Budget::bytes(sizeof(HardwareCheckCommitEndpoint)),
                           edge.shape.size()));
        result.cost =
            Budget::add(result.cost, Budget::multiply(edge.count, cost));
      }
    }
    if (failed(budget.admit(result.cost, node.definition)))
      return failure();
    node.expansion = result;
    return result;
  }
  LogicalResult materialize(size_t index,
                            ArrayRef<HardwareCheckOccurrenceStep> path,
                            std::optional<uint64_t> parent,
                            HardwareCheckResetContext reset) {
    const auto &node = *nodes[index];
    uint64_t cost = ownerCost(node);
    for (const auto &step : path)
      cost = Budget::add(
          cost, Budget::add(Budget::bytes(sizeof(HardwareCheckOccurrenceStep)),
                            step.coordinates.size()));
    if (failed(budget.debit(cost, node.definition)))
      return failure();
    uint64_t owner = plan.owners.size();
    plan.owners.push_back(
        {node.definition,
         SmallVector<HardwareCheckOccurrenceStep>(path.begin(), path.end()),
         node.bindings, parent, owner});
    if (node.reset)
      reset = {HardwareCheckResetKind::PhysicalReset, owner, *node.reset};
    uint64_t firstSamplingOccurrence = plan.samplingOccurrences.size();
    for (uint64_t bindingIndex : node.samplingIndices) {
      if (failed(budget.debit(
              Budget::add(
                  1, Budget::bytes(sizeof(HardwareCheckSamplingOccurrence))),
              node.definition)))
        return failure();
      plan.samplingOccurrences.push_back(
          {bindingIndex, owner, plan.samplingOccurrences.size()});
    }
    for (uint64_t bindingIndex : node.checkIndices) {
      if (failed(budget.debit(Budget::add(1, Budget::bytes(sizeof(
                                                 HardwareResolvedSourceCheck))),
                              node.definition)))
        return failure();
      std::optional<uint64_t> samplingOccurrence;
      if (auto samplingBinding =
              plan.bindings[bindingIndex].samplingBindingIndex) {
        for (auto [localOrdinal, index] :
             llvm::enumerate(node.samplingIndices)) {
          if (failed(budget.debit(1, node.definition)))
            return failure();
          if (index == *samplingBinding) {
            samplingOccurrence = firstSamplingOccurrence + localOrdinal;
            break;
          }
        }
        if (!samplingOccurrence)
          return node.definition->emitOpError()
                 << "sampled check lacks its owner sampling occurrence";
      }
      plan.checks.push_back(
          {bindingIndex, owner, reset, plan.checks.size(), samplingOccurrence});
    }
    for (const auto &edge : node.allocations) {
      if (edge.combinationalExternal) continue;
      for (uint64_t lane = 0; lane < edge.count; ++lane) {
        // Debit before creating either coordinate or copied path payload.
        if (failed(budget.debit(edge.shape.size(), edge.operation)))
          return failure();
        SmallVector<uint64_t> coordinates(edge.shape.size());
        uint64_t remainder = lane;
        for (size_t dimension = edge.shape.size(); dimension > 0; --dimension) {
          coordinates[dimension - 1] = remainder % edge.shape[dimension - 1];
          remainder /= edge.shape[dimension - 1];
        }
        if (edge.child) {
          uint64_t pathCost = stepCost(edge);
          for (const auto &step : path)
            pathCost = Budget::add(
                pathCost,
                Budget::add(Budget::bytes(sizeof(HardwareCheckOccurrenceStep)),
                            step.coordinates.size()));
          if (failed(budget.debit(pathCost, edge.operation)))
            return failure();
          SmallVector<HardwareCheckOccurrenceStep> childPath(path.begin(),
                                                             path.end());
          childPath.push_back({edge.operation, std::move(coordinates)});
          if (failed(materialize(*edge.child, childPath, owner, reset)))
            return failure();
        } else {
          if (failed(budget.debit(
                  Budget::add(
                      1, Budget::bytes(sizeof(HardwareCheckCommitEndpoint))),
                  edge.operation)))
            return failure();
          plan.commits.push_back(
              {owner, edge.operation, std::move(coordinates), edge.primitive, edge.form});
        }
      }
    }
    return success();
  }
  const HardwareAnalysis &analysis;
  PackageVerificationProfile profile;
  SourceCheckBudget budget;
  SampledAdmissionValidation validation;
  HardwareSourceCheckPlan plan;
  llvm::DenseMap<Operation *, SmallVector<uint64_t>> localIndices;
  llvm::DenseMap<Operation *, SmallVector<uint64_t>> localSamplingIndices;
  NativeSourceCheckScope::Definitions nativeChecks;
  llvm::DenseMap<Operation *, uint64_t> localValidationCost;
  std::map<const void *, uint64_t> sampledAttributeCosts;
  llvm::DenseSet<Operation *> active;
  SmallVector<std::unique_ptr<BoundDefinition>> nodes;
  SmallVector<std::pair<ModuleOp, HardwareBindings>> catalogScopes;
};
} // namespace
FailureOr<HardwareSourceCheckPlan>
buildSourceCheckPlan(const HardwareAnalysis &analysis,
                     HardwareSourceCheckLimits limits,
                     PackageVerificationProfile profile) {
  return Builder(analysis, limits, profile).build();
}
} // namespace acir::ac::detail

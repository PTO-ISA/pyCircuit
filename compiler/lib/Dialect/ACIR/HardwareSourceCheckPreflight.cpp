#include "HardwareSourceCheckPreflight.h"
#include "ACIRSourceContracts.h"
#include "HardwareSampledAdmission.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/ScopeExit.h"
#include <algorithm>
#include <memory>

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
  Builder(const HardwareAnalysis &analysis, HardwareSourceCheckLimits limits)
      : analysis(analysis), budget(limits) {}
  FailureOr<HardwareSourceCheckPlan> build() {
    auto package = analysis.getPackage();
    budget.setPhase("raw preflight");
    if (!package || failed(rawPreflight()) || failed(mlir::verify(package)) ||
        failed(verifyHardwarePackageEnvelope(analysis)))
      return failure();
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
    budget.setPhase("transaction wiring");
    if (failed(verifyTransactionWiring(analysis, scopes, budget)))
      return failure();
    // A closed system needs the complete managed commit tree even without
    // source assertions: generated reset/prepare/commit must reach every leaf.
    // Reusable modules without checks still avoid hypothetical expansion.
    auto role = root->owner->getAttrOfType<StringAttr>("ac.root_kind");
    if (!nodes[*rootIndex]->hasChecks && (!role || role.getValue() != "system"))
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
    if (auto string = dyn_cast<StringAttr>(attribute))
      cost = Budget::add(cost, Budget::bytes(string.getValue().size()));
    if (failed(budget.debit(cost, site)))
      return failure();
    if (auto array = dyn_cast<ArrayAttr>(attribute)) {
      for (Attribute child : array) {
        auto nested = structureCost(child, site);
        if (failed(nested))
          return failure();
        cost = Budget::add(cost, *nested);
      }
    } else if (auto dictionary = dyn_cast<DictionaryAttr>(attribute)) {
      for (auto item : dictionary) {
        auto key = structureCost(item.getName(), site);
        auto nested = structureCost(item.getValue(), site);
        if (failed(key) || failed(nested))
          return failure();
        cost = Budget::add(cost, Budget::add(*key, *nested));
      }
    }
    return cost;
  }
  struct RawDefinitionWork {
    uint64_t operations = 0, obligations = 0, metadata = 1, carriers = 0;
    uint64_t sampledGuards = 0, operands = 0, samplingMetadata = 0;
  };
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
  FailureOr<uint64_t> sampledReachableValues(Operation *definition) {
    llvm::DenseSet<Value> boundaries;
    llvm::DenseMap<Value, unsigned> visited;
    SmallVector<std::pair<Value, bool>> pending;
    auto seed = [&](Value value, bool prefix) -> LogicalResult {
      if (!value)
        return success();
      if (failed(budget.debit(Budget::bytes(sizeof(std::pair<Value, bool>)),
                              definition)))
        return failure();
      pending.emplace_back(value, prefix);
      return success();
    };
    auto boundary = [&](Value value) -> LogicalResult {
      if (failed(budget.debit(Budget::bytes(sizeof(Value)), definition)))
        return failure();
      boundaries.insert(value);
      return success();
    };
    auto ordinal = [](DictionaryAttr row, StringRef name,
                      uint64_t count) -> std::optional<uint64_t> {
      auto value = row ? row.getAs<IntegerAttr>(name) : IntegerAttr();
      auto type =
          value ? dyn_cast<IntegerType>(value.getType()) : IntegerType();
      if (!type || !type.isSignless() || type.getWidth() != 64 ||
          value.getInt() < 0 || static_cast<uint64_t>(value.getInt()) >= count)
        return std::nullopt;
      return static_cast<uint64_t>(value.getInt());
    };
    // Use raw region/operand access: this runs before native ODS verification.
    // Private slots are counted as endpoints, never as candidate/data roots.
    for (Region &region : definition->getRegions()) {
      for (Block &block : region) {
        for (Operation &operation : block) {
          Operation *op = &operation;
          auto sampled =
              op->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
          if (isa<RuleOp>(op)) {
            uint64_t firstPrivate = op->getNumResults();
            if (auto required =
                    op->getAttrOfType<ArrayAttr>("ac.required_checks"))
              if (required.size() <= op->getNumResults() / 2)
                firstPrivate = op->getNumResults() - 2 * required.size();
            if (auto fire =
                    ordinal(sampled, "fire_result", op->getNumResults()))
              firstPrivate = std::min(firstPrivate, *fire);
            auto wiring = definition->getAttrOfType<DictionaryAttr>(
                "ac.transaction_wiring");
            auto writers =
                wiring ? wiring.getAs<ArrayAttr>("writers") : ArrayAttr();
            if (writers) {
              for (Attribute raw : writers) {
                if (failed(budget.debit(1, definition)))
                  return failure();
                auto writer = dyn_cast<DictionaryAttr>(raw);
                if (!writer || !writer.get("registration") ||
                    writer.get("registration") != op->getAttr("occurrence"))
                  continue;
                if (auto ordinary = ordinal(writer, "ordinary_results",
                                            op->getNumResults()))
                  firstPrivate = std::min(firstPrivate, *ordinary);
              }
            }
            for (uint64_t index = firstPrivate; index < op->getNumResults();
                 ++index)
              if (failed(boundary(op->getResult(index))))
                return failure();
          }
          if (sampled) {
            for (StringRef name : {"clock_capture", "reset_capture"})
              if (auto index = ordinal(sampled, name, op->getNumOperands()))
                if (failed(seed(op->getOperand(*index), false)))
                  return failure();
            auto fire = ordinal(sampled, "fire_result", op->getNumResults());
            auto guards = sampled.getAs<ArrayAttr>("guards");
            if (fire && guards && op->getNumRegions() == 1 &&
                op->getRegion(0).hasOneBlock() &&
                !op->getRegion(0).front().empty()) {
              Operation &yielded = op->getRegion(0).front().back();
              if (*fire < yielded.getNumOperands() &&
                  failed(seed(yielded.getOperand(*fire), true)))
                return failure();
              for (uint64_t index = 0; index < guards.size(); ++index) {
                uint64_t predicate = Budget::add(Budget::add(*fire, 1),
                                                 Budget::multiply(2, index));
                uint64_t reached = Budget::add(predicate, 1);
                if (predicate < yielded.getNumOperands() &&
                    failed(seed(yielded.getOperand(predicate), false)))
                  return failure();
                if (reached < yielded.getNumOperands() &&
                    failed(seed(yielded.getOperand(reached), true)))
                  return failure();
              }
            }
          }
          if (isa<SourceExpectOp>(op)) {
            auto kind = op->getAttrOfType<StringAttr>("kind");
            if (kind && kind.getValue() == "guard_known")
              for (unsigned index = 2; index < op->getNumOperands(); ++index)
                if (failed(seed(op->getOperand(index), false)))
                  return failure();
          }
        }
      }
    }
    while (!pending.empty()) {
      auto [value, prefix] = pending.pop_back_val();
      if (failed(budget.debit(1, definition)))
        return failure();
      unsigned mode = prefix ? 2 : 1;
      auto known = visited.find(value);
      if (known != visited.end() && (known->second & mode))
        continue;
      if (known == visited.end()) {
        if (failed(budget.debit(
                Budget::bytes(sizeof(std::pair<Value, unsigned>)), definition)))
          return failure();
        known = visited.try_emplace(value, 0).first;
      }
      known->second |= mode;
      if (boundaries.contains(value))
        continue;
      if (auto argument = dyn_cast<BlockArgument>(value)) {
        Operation *owner = argument.getOwner()->getParentOp();
        if (isa<RuleOp>(owner) &&
            argument.getArgNumber() < owner->getNumOperands())
          if (failed(seed(owner->getOperand(argument.getArgNumber()), prefix)))
            return failure();
        continue;
      }
      auto result = dyn_cast<OpResult>(value);
      if (!result)
        continue;
      Operation *owner = result.getOwner();
      if (isa<RuleOp>(owner)) {
        if (owner->getNumRegions() == 1 && owner->getRegion(0).hasOneBlock() &&
            !owner->getRegion(0).front().empty()) {
          Operation &yielded = owner->getRegion(0).front().back();
          if (result.getResultNumber() < yielded.getNumOperands() &&
              failed(
                  seed(yielded.getOperand(result.getResultNumber()), prefix)))
            return failure();
        }
      } else if (isa<BitsExtractOp>(owner)) {
        if (owner->getNumOperands() &&
            failed(seed(owner->getOperand(0), prefix)))
          return failure();
      } else if (prefix && isa<BitsBinaryOp>(owner)) {
        auto opcode = owner->getAttrOfType<StringAttr>("opcode");
        if (opcode && opcode.getValue() == "and")
          for (Value operand : owner->getOperands())
            if (failed(seed(operand, true)))
              return failure();
      } else if (prefix && isa<BitsCompareOp>(owner)) {
        auto predicate = owner->getAttrOfType<StringAttr>("predicate");
        if (predicate && predicate.getValue() == "case_eq")
          for (Value operand : owner->getOperands())
            if (failed(seed(operand, false)))
              return failure();
      }
      // Opaque raw guard computations and ordinary Instance results terminate
      // definition-local identity; their unrelated data DAGs are not visited.
    }
    return Budget::add(visited.size(), boundaries.size());
  }
  LogicalResult rawPreflight() {
    // Do not invoke typed attribute/signature accessors or any op verifier.
    // ModuleOp's region verifier constructs the local association catalog, so
    // its record payload and comparison work must be prepaid first.
    llvm::DenseMap<Operation *, RawDefinitionWork> work;
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
      if (!definition)
        return WalkResult::advance();
      if (!work.count(definition) &&
          failed(budget.debit(
              Budget::bytes(sizeof(std::pair<Operation *, RawDefinitionWork>) +
                            sizeof(uint64_t)),
              op)))
        return WalkResult::interrupt();
      auto &local = work[definition];
      ++local.operations;
      local.operands = Budget::add(local.operands, op->getNumOperands());
      for (StringRef key :
           {"occurrence", "ac.required_checks", "ac.check_id", "location",
            "ac.message", "ac.sampled_admission", "ac.transaction_wiring"}) {
        auto cost = structureCost(op->getAttr(key), op);
        if (failed(cost))
          return WalkResult::interrupt();
        local.metadata = Budget::add(local.metadata, *cost);
        if (key == "ac.sampled_admission" || key == "ac.transaction_wiring")
          local.samplingMetadata = Budget::add(local.samplingMetadata, *cost);
      }
      if (auto required = op->getAttrOfType<ArrayAttr>("ac.required_checks"))
        local.obligations = Budget::add(local.obligations, required.size());
      if (auto sampled =
              op->getAttrOfType<DictionaryAttr>("ac.sampled_admission"))
        if (auto guards = sampled.getAs<ArrayAttr>("guards"))
          local.sampledGuards = Budget::add(local.sampledGuards, guards.size());
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
    for (const auto &[definition, local] : work) {
      uint64_t height = 0;
      for (uint64_t count = local.obligations; count; count /= 2)
        ++height;
      // A red-black tree has height <= 2*ceil(log2(N+1)). Both insertion and
      // lookup compare full structural IDs. The second binding vector copies
      // the map's sorted order without another ID comparison/sort pass.
      uint64_t comparisons = Budget::add(Budget::multiply(4, height), 2);
      uint64_t records = Budget::multiply(
          local.obligations,
          Budget::bytes(2 * sizeof(HardwareCheckBinding) +
                        sizeof(std::pair<DictionaryAttr, size_t>) +
                        3 * sizeof(void *)));
      uint64_t cost = Budget::add(
          local.operations,
          Budget::add(records,
                      Budget::add(Budget::multiply(comparisons, local.metadata),
                                  local.carriers)));
      // Native Module verification runs before the builder can share its
      // budget with the local verifier. Prepay a conservative bound for the
      // sampled schema, stop endpoints, identity walks and prefix atom sets.
      if (local.samplingMetadata) {
        auto reachable = sampledReachableValues(definition);
        if (failed(reachable))
          return failure();
        uint64_t values =
            Budget::add(Budget::add(*reachable, local.sampledGuards), 1);
        uint64_t square = Budget::multiply(values, values);
        uint64_t scope = Budget::bytes(sizeof(HardwareBindings) +
                                       sizeof(HardwareIdentityScope));
        uint64_t payload = Budget::bytes(
            4 * sizeof(HardwareIdentityEndpoint) + sizeof(SampledGuardView) +
            sizeof(TransactionWriterView) + sizeof(TransactionCandidateView));
        // Memoized prefix nodes retain at most V atoms, and local identity
        // walks visit at most V endpoints with at most V context comparisons.
        // V counts only guard-prefix/identity-reachable SSA and private stop
        // endpoints; unrelated datapaths retain only ordinary graph charges.
        uint64_t proof = Budget::add(
            Budget::multiply(Budget::multiply(square, values), scope),
            Budget::multiply(square, Budget::add(payload, local.metadata)));
        uint64_t schema = Budget::multiply(local.samplingMetadata,
                                           Budget::add(local.operations, 1));
        cost = Budget::add(
            cost, Budget::add(proof, Budget::add(schema, local.operands)));
      }
      if (failed(budget.debit(cost, definition)))
        return failure();
      if (failed(budget.debit(
              Budget::bytes(sizeof(std::pair<Operation *, uint64_t>)),
              definition)))
        return failure();
      localValidationCost[definition] = Budget::add(
          local.operations,
          Budget::add(records,
                      Budget::add(Budget::multiply(comparisons, local.metadata),
                                  local.carriers)));
    }
    return success();
  }
  LogicalResult catalogSampling(ModuleOp definition,
                                const HardwareBindings &bindings,
                                SmallVector<HardwareCheckBinding> &checks) {
    if (!llvm::any_of(checks, [](const auto &check) {
          return check.kind.getValue() == "guard_known";
        }))
      return success();
    if (failed(budget.debit(Budget::bytes(sizeof(
                                std::pair<Operation *, SmallVector<uint64_t>>)),
                            definition)))
      return failure();
    auto &indices = localSamplingIndices[definition];
    if (failed(budget.debit(
            Budget::add(Budget::bindings(bindings),
                        Budget::bytes(sizeof(HardwareIdentityScope))),
            definition)))
      return failure();
    HardwareIdentityScope scope{definition, bindings, {}};
    SmallVector<HardwareIdentityEndpoint> boundaries;
    auto addBoundary = [&](Value value) -> LogicalResult {
      if (failed(budget.debit(Budget::bytes(sizeof(HardwareIdentityEndpoint)),
                              definition)))
        return failure();
      auto endpoint = makeIdentityEndpoint(value, scope, analysis, budget);
      if (failed(endpoint))
        return failure();
      boundaries.push_back(std::move(*endpoint));
      return success();
    };
    auto wiring =
        parseTransactionWiring(definition, analysis, bindings, budget);
    if (failed(wiring))
      return failure();
    for (auto &writer : wiring->writers)
      for (uint64_t index = writer.ordinaryResults;
           index < writer.rule.getNumResults(); ++index)
        if (failed(addBoundary(writer.rule.getResult(index))))
          return failure();
    for (auto &check : checks)
      if (failed(addBoundary(check.rule.getResult(check.conditionResult))) ||
          failed(addBoundary(check.rule.getResult(check.pathResult))))
        return failure();
    HardwareIdentityStopPolicy stops{HardwareIdentityTraversal::DefinitionLocal,
                                     boundaries};
    // Repeated guards of one Rule share a binding. Distinct Rules share only
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
      uint64_t cost = localValidationCost[definition];
      if (failed(budget.debit(cost, definition)))
        return failure();
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
      auto bindings =
          definitionSourceChecks(analysis, definition, local, budget);
      if (failed(bindings))
        return failure();
      if (failed(
              budget.debit(Budget::bytes(sizeof(
                               std::pair<Operation *, SmallVector<uint64_t>>)),
                           definition)))
        return failure();
      auto &indices = localIndices[definition];
      auto sampling = catalogSampling(definition, local, *bindings);
      if (failed(sampling))
        return failure();
      if (failed(budget.debit(Budget::multiply(bindings->size(),
                                               Budget::bytes(sizeof(uint64_t))),
                              definition)))
        return failure();
      for (const auto &binding : *bindings) {
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
    if (failed(budget.debit(localValidationCost[definition], definition)))
      return failure();
    auto local = definitionSourceChecks(analysis, definition, scope, budget);
    if (failed(local))
      return failure();
    SmallVector<Operation *> allocations;
    for (Operation &op : definition.getBody().front()) {
      if (!isa<InstanceOp, CollectionOp, QueueOp>(op))
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
      if (auto queue = dyn_cast<QueueOp>(allocation)) {
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
          if (!childDefinition)
            return allocation->emitOpError()
                   << "source check plan requires explicit provider body";
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
              {owner, edge.operation, std::move(coordinates), edge.primitive});
        }
      }
    }
    return success();
  }
  const HardwareAnalysis &analysis;
  SourceCheckBudget budget;
  HardwareSourceCheckPlan plan;
  llvm::DenseMap<Operation *, SmallVector<uint64_t>> localIndices;
  llvm::DenseMap<Operation *, SmallVector<uint64_t>> localSamplingIndices;
  llvm::DenseMap<Operation *, uint64_t> localValidationCost;
  llvm::DenseSet<Operation *> active;
  SmallVector<std::unique_ptr<BoundDefinition>> nodes;
  SmallVector<std::pair<ModuleOp, HardwareBindings>> catalogScopes;
};
} // namespace
FailureOr<HardwareSourceCheckPlan>
buildSourceCheckPlan(const HardwareAnalysis &analysis,
                     HardwareSourceCheckLimits limits) {
  return Builder(analysis, limits).build();
}
} // namespace acir::ac::detail

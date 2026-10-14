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
      : analysis(analysis), budget(limits), validation(analysis, budget) {}
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
    if (failed(mlir::verify(package)) ||
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
    if (failed(verifyTransactionWiring(analysis, scopes, validation, budget)))
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
    bool hasSamplingMetadata = false, hasWiring = false;
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
  struct RawSampledRuleWork {
    uint64_t prefixValues = 0, andNodes = 0, comparisons = 0;
    uint64_t andOperands = 0, guards = 0, effects = 0, identityLength = 0;
    uint64_t intentValues = 0, intentSelects = 0, intentEdges = 0, enables = 0;
    uint64_t intentIdentityInputs = 0, effectiveValues = 0,
             intentIdentityLength = 0;
  };
  static uint64_t sampledRuleProofCost(const RawSampledRuleWork &work,
                                       uint64_t stops) {
    auto add = Budget::add;
    auto multiply = Budget::multiply;
    uint64_t b = Budget::bytes(sizeof(HardwareBindings));
    uint64_t s = add(b, Budget::bytes(sizeof(HardwareIdentityScope)));
    uint64_t e = add(1, Budget::bytes(sizeof(HardwareIdentityEndpoint)));
    uint64_t f = add(add(add(e, b), 1), s);
    uint64_t m = add(1, Budget::bytes(2 * sizeof(HardwareIdentityEndpoint)));
    uint64_t h =
        add(1, Budget::bytes(sizeof(SmallVector<HardwareIdentityEndpoint>) +
                             sizeof(uint64_t)));
    uint64_t hc =
        add(1, Budget::bytes(sizeof(HardwareIdentityEndpoint) +
                             sizeof(SmallVector<HardwareIdentityEndpoint>) +
                             2 * sizeof(uint64_t)));
    uint64_t atoms = std::max(work.guards, work.comparisons);
    uint64_t prefixQueries =
        add(add(add(work.guards, 1), work.effects), work.andOperands);
    uint64_t intentQueries =
        add(1, add(work.intentEdges, multiply(3, work.enables)));
    // Dedicated body-effect paths have their own non-memoized exact-fire
    // identity checks, separate from both prefix and intent memo contexts.
    uint64_t exactPathQueries = add(1, work.effects);
    uint64_t queries = add(prefixQueries, add(multiply(2, work.comparisons),
                                              multiply(5, work.guards)));
    // Each memo context pays only chains it actually normalizes. Intent
    // selectors and D/E carriers cannot lengthen ordered-prefix walks.
    auto normalizationCost = [&](uint64_t length) {
      uint64_t normalStep =
          add(add(1, stops), add(add(e, b), add(f, multiply(6, s))));
      uint64_t normal = add(f, multiply(length, normalStep));
      if (length) {
        uint64_t left = length, right = length - 1;
        if (left % 2 == 0)
          left /= 2;
        else
          right /= 2;
        normal = add(normal, multiply(left, right));
      }
      return normal;
    };
    uint64_t normal = normalizationCost(work.identityLength);
    uint64_t intentNormal = normalizationCost(work.intentIdentityLength);
    uint64_t identity = multiply(
        queries, add(add(add(f, queries), normal), add(m, multiply(4, s))));
    // IntentProof has a different immutable IdentityMemo from the prefix
    // prover. Repeated fire and shared literal-intent queries are memo hits;
    // only distinct effective enables, intent nodes and selectors can miss.
    uint64_t intentEntries =
        add(work.effectiveValues,
            add(work.intentValues, work.intentIdentityInputs));
    uint64_t intentMisses = std::min(intentQueries, intentEntries);
    uint64_t intentIdentity =
        multiply(intentQueries, add(intentEntries, add(e, multiply(3, s))));
    intentIdentity = add(intentIdentity,
                         multiply(intentMisses, add(add(f, intentNormal),
                                                    add(m, multiply(2, s)))));
    identity = add(identity, intentIdentity);
    identity = add(identity, multiply(exactPathQueries, add(f, normal)));
    identity = add(identity, multiply(work.effects, multiply(2, s)));
    uint64_t copy = add(h, multiply(atoms, add(e, s)));
    uint64_t atomStep =
        add(add(atoms, 1), add(multiply(multiply(2, s), atoms), add(e, s)));
    uint64_t sort = multiply(add(atoms, 1), add(atoms, 1));
    uint64_t joinGeneral = add(copy, add(multiply(atoms, atomStep), sort));
    uint64_t joinSingleton = add(copy, add(atomStep, sort));
    uint64_t equal =
        add(sort, multiply(multiply(multiply(2, s), atoms), atoms));
    uint64_t cost =
        add(identity,
            multiply(prefixQueries,
                     add(multiply(multiply(2, s), work.prefixValues), copy)));
    cost = add(cost, multiply(work.prefixValues, add(add(multiply(2, b), 2),
                                                     add(hc, add(s, copy)))));
    cost = add(cost, multiply(multiply(2, work.comparisons), add(b, 1)));
    cost = add(cost, multiply(add(work.comparisons, work.guards),
                              add(add(multiply(2, b), 2), add(e, s))));
    cost = add(cost, multiply(work.andNodes, joinGeneral));
    cost = add(cost, multiply(work.guards, joinSingleton));
    cost = add(cost, multiply(add(add(work.guards, 1), work.effects), equal));
    // The separate intent grammar owns completed-root and Active/Complete node
    // memo storage plus explicit DFS frames. Reserve actual raw nodes/edges;
    // selectors remain ordinary Bits1 identity queries, never guard atoms.
    uint64_t intentLookup = 1;
    for (uint64_t n = add(work.intentValues, work.enables); n > 1;
         n = n / 2 + n % 2)
      intentLookup = add(intentLookup, 2);
    cost = add(
        cost,
        multiply(
            work.intentValues,
            add(1, Budget::bytes(
                       SampledAdmissionValidation::intentNodeStorageBytes()))));
    cost = add(
        cost,
        multiply(
            work.enables,
            add(1, Budget::bytes(
                       SampledAdmissionValidation::intentRootStorageBytes()))));
    // Geometric frame growth reserves less than twice the visited select count;
    // old-element move work is separately prepaid.
    cost = add(
        cost,
        multiply(
            multiply(2, work.intentSelects),
            add(2,
                Budget::bytes(
                    SampledAdmissionValidation::intentFrameStorageBytes()))));
    cost = add(cost,
               multiply(add(work.intentEdges,
                            multiply(4, add(work.intentValues, work.enables))),
                        intentLookup));
    cost =
        add(cost, multiply(work.intentValues, add(add(multiply(2, b), 3), s)));
    cost =
        add(cost, multiply(work.intentSelects, add(add(multiply(2, b), 4), s)));
    return add(cost, multiply(work.enables, add(3, multiply(2, s))));
  }
  FailureOr<uint64_t> sampledProofCost(Operation *definition, uint64_t stops) {
    llvm::DenseSet<Value> boundaries;
    SmallVector<Operation *> rules;
    SmallVector<std::pair<Value, unsigned>> pending;
    auto seed = [&](Value value, unsigned mode) -> LogicalResult {
      if (!value)
        return success();
      if (failed(budget.debit(1, definition)))
        return failure();
      if (pending.size() == pending.capacity()) {
        uint64_t capacity = Budget::multiply(
            std::max(uint64_t(1), uint64_t(pending.capacity())), 2);
        uint64_t bytes = Budget::multiply(capacity - pending.capacity(),
                                          sizeof(std::pair<Value, unsigned>));
        if (capacity == std::numeric_limits<uint64_t>::max() ||
            bytes == std::numeric_limits<uint64_t>::max() ||
            capacity > pending.max_size())
          return budget.debit(std::numeric_limits<uint64_t>::max(), definition);
        if (failed(budget.debit(
                Budget::add(Budget::bytes(bytes), pending.size()), definition)))
          return failure();
        pending.reserve(capacity);
      }
      pending.emplace_back(value, mode);
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
    auto seedSampled = [&](Operation *op) -> LogicalResult {
      auto sampled = op->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      for (StringRef name : {"clock_capture", "reset_capture"})
        if (auto index = ordinal(sampled, name, op->getNumOperands()))
          if (failed(seed(op->getOperand(*index), false)))
            return failure();
      auto fire = ordinal(sampled, "fire_result", op->getNumResults());
      auto guards = sampled.getAs<ArrayAttr>("guards");
      if (!fire || !guards || op->getNumRegions() != 1 ||
          !op->getRegion(0).hasOneBlock() || op->getRegion(0).front().empty())
        return success();
      Operation &yielded = op->getRegion(0).front().back();
      if (*fire < yielded.getNumOperands()) {
        if (failed(seed(yielded.getOperand(*fire), true)) ||
            failed(seed(yielded.getOperand(*fire), 4)))
          return failure();
      }
      auto wiring =
          definition->getAttrOfType<DictionaryAttr>("ac.transaction_wiring");
      auto writers = wiring ? wiring.getAs<ArrayAttr>("writers") : ArrayAttr();
      if (writers) {
        for (Attribute raw : writers) {
          if (failed(budget.debit(1, definition)))
            return failure();
          auto writer = dyn_cast<DictionaryAttr>(raw);
          if (!writer ||
              writer.get("registration") != op->getAttr("occurrence"))
            continue;
          auto candidates = writer.getAs<ArrayAttr>("candidates");
          if (!candidates)
            continue;
          for (Attribute candidateRaw : candidates) {
            if (failed(budget.debit(1, definition)))
              return failure();
            auto candidate = dyn_cast<DictionaryAttr>(candidateRaw);
            if (auto enable =
                    ordinal(candidate, "enable", yielded.getNumOperands()))
              if (failed(seed(yielded.getOperand(*enable), 3)))
                return failure();
          }
        }
      }
      uint64_t checkBase = Budget::add(*fire, 1);
      if (auto observations = sampled.getAs<ArrayAttr>("observations")) {
        for (Attribute raw : observations) {
          if (failed(budget.debit(1, definition)))
            return failure();
          if (checkBase < yielded.getNumOperands() &&
              failed(seed(yielded.getOperand(checkBase), true)))
            return failure();
          auto row = dyn_cast<DictionaryAttr>(raw);
          auto count =
              row ? row.getAs<IntegerAttr>("value_count") : IntegerAttr();
          if (count && count.getInt() >= 0)
            checkBase = Budget::add(checkBase, Budget::add(1, count.getInt()));
        }
      }
      if (auto checks = sampled.getAs<ArrayAttr>("body_checks")) {
        for (uint64_t i = 0; i < checks.size(); ++i) {
          if (failed(budget.debit(1, definition)))
            return failure();
          uint64_t path = Budget::add(
              checkBase,
              Budget::add(Budget::multiply(2, Budget::add(guards.size(), i)),
                          1));
          if (path < yielded.getNumOperands() &&
              failed(seed(yielded.getOperand(path), true)))
            return failure();
        }
      }
      for (uint64_t index = 0; index < guards.size(); ++index) {
        if (failed(budget.debit(1, definition)))
          return failure();
        uint64_t predicate = Budget::add(checkBase, Budget::multiply(2, index));
        uint64_t reached = Budget::add(predicate, 1);
        if (predicate < yielded.getNumOperands() &&
            failed(seed(yielded.getOperand(predicate), false)))
          return failure();
        if (reached < yielded.getNumOperands() &&
            failed(seed(yielded.getOperand(reached), true)))
          return failure();
      }
      return success();
    };
    // Use raw region/operand access before native ODS verification. Private
    // slots are endpoints; unrelated data DAGs and ordinary Instances stop.
    for (Region &region : definition->getRegions()) {
      for (Block &block : region) {
        for (Operation &operation : block) {
          if (failed(budget.debit(1, definition)))
            return failure();
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
          if (sampled && isa<RuleOp>(op)) {
            if (failed(budget.debit(Budget::bytes(sizeof(Operation *)),
                                    definition)))
              return failure();
            rules.push_back(op);
          }
        }
      }
    }
    auto identityNext = [&](Value value) -> Value {
      if (boundaries.contains(value))
        return {};
      if (auto argument = dyn_cast<BlockArgument>(value)) {
        Operation *owner = argument.getOwner()->getParentOp();
        if (isa<RuleOp>(owner) &&
            argument.getArgNumber() < owner->getNumOperands())
          return owner->getOperand(argument.getArgNumber());
      } else if (auto result = dyn_cast<OpResult>(value)) {
        Operation *owner = result.getOwner();
        if (isa<RuleOp>(owner)) {
          if (owner->getNumRegions() == 1 &&
              owner->getRegion(0).hasOneBlock() &&
              !owner->getRegion(0).front().empty()) {
            Operation &yielded = owner->getRegion(0).front().back();
            if (result.getResultNumber() < yielded.getNumOperands())
              return yielded.getOperand(result.getResultNumber());
          }
        } else if (isa<BitsExtractOp>(owner) && owner->getNumOperands()) {
          return owner->getOperand(0);
        }
      }
      return {};
    };
    auto walk = [&]() -> FailureOr<RawSampledRuleWork> {
      llvm::DenseMap<Value, unsigned> visited;
      RawSampledRuleWork result;
      while (!pending.empty()) {
        auto [value, modeIndex] = pending.pop_back_val();
        bool prefix = modeIndex == 1;
        bool intent = modeIndex == 2;
        bool effective = modeIndex == 3;
        if (failed(budget.debit(1, definition)))
          return failure();
        unsigned mode = 1u << modeIndex;
        auto known = visited.find(value);
        if (known != visited.end() && (known->second & mode))
          continue;
        if (known == visited.end()) {
          if (failed(budget.debit(
                  Budget::bytes(sizeof(std::pair<Value, unsigned>)),
                  definition)))
            return failure();
          known = visited.try_emplace(value, 0).first;
        }
        known->second |= mode;
        if (prefix)
          result.prefixValues = Budget::add(result.prefixValues, 1);
        if (intent)
          result.intentValues = Budget::add(result.intentValues, 1);
        if (effective)
          result.effectiveValues = Budget::add(result.effectiveValues, 1);
        if (modeIndex == 4)
          result.intentIdentityInputs =
              Budget::add(result.intentIdentityInputs, 1);
        if (Value next = identityNext(value)) {
          if (failed(seed(next, modeIndex)))
            return failure();
          continue;
        }
        if (boundaries.contains(value))
          continue;
        Operation *owner = value.getDefiningOp();
        if (effective && owner && isa<BitsBinaryOp>(owner)) {
          result.enables = Budget::add(result.enables, 1);
          if (owner->getNumOperands() > 0 &&
              failed(seed(owner->getOperand(0), 4)))
            return failure();
          if (owner->getNumOperands() > 1 &&
              failed(seed(owner->getOperand(1), 2)))
            return failure();
        } else if (intent && owner && isa<BitsSelectOp>(owner)) {
          result.intentSelects = Budget::add(result.intentSelects, 1);
          result.intentEdges =
              Budget::add(result.intentEdges, owner->getNumOperands());
          for (auto [i, operand] : llvm::enumerate(owner->getOperands()))
            if (failed(seed(operand, i == 0 ? 4 : 2)))
              return failure();
        } else if (prefix && owner && isa<BitsBinaryOp>(owner)) {
          auto opcode = owner->getAttrOfType<StringAttr>("opcode");
          if (opcode && opcode.getValue() == "and") {
            result.andNodes = Budget::add(result.andNodes, 1);
            result.andOperands =
                Budget::add(result.andOperands, owner->getNumOperands());
            for (Value operand : owner->getOperands())
              if (failed(seed(operand, true)))
                return failure();
          }
        } else if (prefix && owner && isa<BitsCompareOp>(owner)) {
          auto predicate = owner->getAttrOfType<StringAttr>("predicate");
          if (predicate && predicate.getValue() == "case_eq") {
            result.comparisons = Budget::add(result.comparisons, 1);
            for (Value operand : owner->getOperands())
              if (failed(seed(operand, false)))
                return failure();
          }
        }
      }
      struct IdentityLength {
        uint64_t length = 0;
        bool complete = false, cyclic = false;
      };
      llvm::DenseMap<Value, IdentityLength> lengths;
      SmallVector<Value> chain;
      // The raw graph has at most one identity successor. Memoize suffixes
      // iteratively, so long captures/extracts never consume the host stack.
      for (const auto &entry : visited) {
        if (failed(budget.debit(1, definition)))
          return failure();
        Value current = entry.first;
        uint64_t length = 0;
        bool cycle = false;
        while (current) {
          if (failed(budget.debit(1, definition)))
            return failure();
          auto known = lengths.find(current);
          if (known != lengths.end()) {
            cycle = !known->second.complete || known->second.cyclic;
            length =
                cycle ? Budget::add(visited.size(), 1) : known->second.length;
            break;
          }
          if (failed(budget.debit(
                  Budget::add(
                      Budget::bytes(sizeof(std::pair<Value, IdentityLength>)),
                      Budget::bytes(sizeof(Value))),
                  definition)))
            return failure();
          lengths.try_emplace(current);
          chain.push_back(current);
          current = identityNext(current);
        }
        while (!chain.empty()) {
          if (failed(budget.debit(1, definition)))
            return failure();
          if (!cycle)
            length = Budget::add(length, 1);
          lengths.find(chain.pop_back_val())->second = {length, true, cycle};
        }
        if (entry.second & 3u)
          result.identityLength = std::max(result.identityLength, length);
        if (entry.second & 28u)
          result.intentIdentityLength =
              std::max(result.intentIdentityLength, length);
      }
      return result;
    };
    uint64_t proof = 0;
    for (Operation *rule : rules) {
      if (failed(budget.debit(1, definition)) || failed(seedSampled(rule)))
        return failure();
      auto local = walk();
      if (failed(local))
        return failure();
      auto sampled =
          rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
      if (auto guards = sampled.getAs<ArrayAttr>("guards"))
        local->guards = guards.size();
      if (auto checks = sampled.getAs<ArrayAttr>("body_checks"))
        local->effects = checks.size();
      if (auto observations = sampled.getAs<ArrayAttr>("observations"))
        local->effects = Budget::add(local->effects, observations.size());
      proof = Budget::add(proof, sampledRuleProofCost(*local, stops));
    }
    return proof;
  }
  template <typename T> static uint64_t schemaRecordCost() {
    return Budget::add(1, Budget::bytes(sizeof(T)));
  }
  FailureOr<uint64_t> sampledSchemaCost(Attribute attribute, uint64_t metadata,
                                        Operation *site) {
    auto sampled = dyn_cast_or_null<DictionaryAttr>(attribute);
    auto guards = sampled ? sampled.getAs<ArrayAttr>("guards") : ArrayAttr();
    auto checks =
        sampled ? sampled.getAs<ArrayAttr>("body_checks") : ArrayAttr();
    auto observations =
        sampled ? sampled.getAs<ArrayAttr>("observations") : ArrayAttr();
    auto fail = [&]() -> FailureOr<uint64_t> {
      emitError(UnknownLoc::get(site->getContext()))
          << "sampled admission raw layout requires exact six-field schema and "
             "checked result counts";
      return failure();
    };
    auto rawIndex = [&](DictionaryAttr row,
                        StringRef key) -> std::optional<uint64_t> {
      auto value = row ? row.getAs<IntegerAttr>(key) : IntegerAttr();
      auto type =
          value ? dyn_cast<IntegerType>(value.getType()) : IntegerType();
      if (!type || !type.isSignless() || type.getWidth() != 64 ||
          value.getInt() < 0)
        return std::nullopt;
      return static_cast<uint64_t>(value.getInt());
    };
    auto clock = rawIndex(sampled, "clock_capture"),
         reset = rawIndex(sampled, "reset_capture"),
         fire = rawIndex(sampled, "fire_result");
    if (!sampled || sampled.size() != 6 || !guards || guards.empty() ||
        !checks || !observations || !clock || !reset || !fire ||
        *clock >= site->getNumOperands() || *reset >= site->getNumOperands())
      return fail();
    uint64_t g = guards.size(), a = checks.size(), n = observations.size(),
             k = 0, frames = 0;
    uint64_t checkBase = Budget::add(*fire, 1);
    for (Attribute raw : observations) {
      if (failed(budget.debit(1, site)))
        return failure();
      auto row = dyn_cast<DictionaryAttr>(raw);
      auto count = rawIndex(row, "value_count");
      if (!row || row.size() != 2 || !row.getAs<DictionaryAttr>("id") || !count)
        return fail();
      k = Budget::add(k, *count);
      checkBase = Budget::add(checkBase, Budget::add(1, *count));
    }
    uint64_t total =
        Budget::add(checkBase, Budget::multiply(2, Budget::add(g, a)));
    if (total == std::numeric_limits<uint64_t>::max() ||
        total != site->getNumResults() || site->getNumRegions() != 1 ||
        !site->getRegion(0).hasOneBlock() ||
        site->getRegion(0).front().empty() ||
        site->getRegion(0).front().back().getNumOperands() != total)
      return fail();
    auto required = site->getAttrOfType<ArrayAttr>("ac.required_checks");
    if (!required || required.size() != Budget::add(g, a))
      return fail();
    auto requiredCost = structureCost(required, site);
    if (failed(requiredCost))
      return failure();
    metadata = Budget::add(metadata, *requiredCost);
    auto countFrames = [&](ArrayAttr rows, bool observation) -> LogicalResult {
      for (Attribute raw : rows) {
        if (failed(budget.debit(1, site)))
          return failure();
        auto row = dyn_cast<DictionaryAttr>(raw);
        if (observation)
          row = row ? row.getAs<DictionaryAttr>("id") : DictionaryAttr();
        if (auto invocation =
                row ? row.getAs<ArrayAttr>("invocation") : ArrayAttr())
          frames = Budget::add(frames, invocation.size());
      }
      return success();
    };
    if (failed(countFrames(guards, false)) ||
        failed(countFrames(checks, false)) ||
        failed(countFrames(observations, true)))
      return failure();
    auto add = Budget::add;
    auto multiply = Budget::multiply;
    uint64_t q = add(add(g, a), n);
    uint64_t cost = schemaRecordCost<SampledAdmissionView>();
    cost = add(cost, multiply(g, schemaRecordCost<SampledGuardView>()));
    cost = add(cost, multiply(a, schemaRecordCost<SampledBodyCheckView>()));
    cost = add(cost, multiply(n, schemaRecordCost<SampledObservationView>()));
    cost = add(cost, multiply(k, schemaRecordCost<Value>()));
    cost = add(cost, multiply(frames, schemaRecordCost<DictionaryAttr>()));
    cost = add(cost, multiply(add(multiply(2, q), 3), metadata));
    return add(cost, add(add(add(3, multiply(2, g)), multiply(2, a)),
                         add(n, multiply(2, k))));
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
  static uint64_t wiringSchemaCost(const RawDefinitionWork &local) {
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
               add(add(local.incoming, local.candidates), local.sampledSchema));
    return add(cost, multiply(local.rules, add(local.writers, 1)));
  }
  static uint64_t nativeSampledSchemaCost(const RawDefinitionWork &local) {
    auto add = Budget::add;
    auto multiply = Budget::multiply;
    uint64_t b = Budget::bytes(sizeof(HardwareBindings));
    uint64_t s = add(b, Budget::bytes(sizeof(HardwareIdentityScope)));
    uint64_t e = schemaRecordCost<HardwareIdentityEndpoint>();
    uint64_t f = add(add(add(e, b), 1), s);
    uint64_t prepared =
        add(1, Budget::bytes(sizeof(PreparedSampledBoundaries)));
    uint64_t validate = add(1, multiply(2, b));
    // Without wiring there is no parsed writer from which boundary slots can
    // be obtained, so both boundary preparation and the actual Rule query
    // parse.
    uint64_t cost = add(wiringSchemaCost(local),
                        multiply(local.hasWiring ? 1 : 2, local.sampledSchema));
    cost = add(cost, add(prepared, multiply(2, b)));
    cost = add(cost, multiply(local.ruleResults, add(e, f)));
    cost = add(cost, multiply(add(local.sampledRules, 1), validate));
    uint64_t map =
        Budget::bytes(sizeof(std::pair<Operation *, SampledAdmissionView>));
    cost = add(cost, multiply(local.sampledRules, map));
    cost = add(cost, multiply(2, local.operations));
    cost = add(cost, multiply(local.rules, local.writers));
    // Complete boundary and immutable wiring preparation each parse once;
    // the single actual Rule query above always reparses its six closed fields.
    // One additional full wiring parse serves all typed writer queries. Their
    // row revalidation visits only the actual row metadata and candidate slots;
    // it never rebuilds unrelated state/Rule inventories per writer.
    cost = add(cost, wiringSchemaCost(local));
    cost = add(cost, add(1, add(local.states,
                                add(local.incoming,
                                    add(local.writers, local.candidates)))));
    uint64_t writerRows = std::max(uint64_t(1), local.maxCandidates);
    cost = add(cost, multiply(writerRows, local.wiringMetadata));
    cost = add(
        cost,
        multiply(
            multiply(2, local.writers),
            add(2,
                Budget::bytes(
                    SampledAdmissionValidation::intentProofStorageBytes()))));
    cost = add(cost, multiply(local.candidates,
                              schemaRecordCost<CheckedCandidateEnable>()));
    cost = add(cost, multiply(local.candidates, add(2, f)));
    cost = add(cost, multiply(3, multiply(local.writers, local.writers)));
    cost = add(cost, multiply(local.writers, add(4, multiply(4, b))));
    cost = add(cost, multiply(local.candidates, add(2, local.maxCandidates)));
    // The native definition verifier owns a fresh validation context: one
    // initial prepare, then one prepare and entry lookup per sampled Rule.
    // Keep the prior schema/proof reserve and add all context bookkeeping.
    uint64_t r = local.sampledRules;
    uint64_t queries = add(r, 1);
    cost = add(cost, schemaRecordCost<SampledAdmissionValidation>());
    cost = add(
        cost,
        add(1, Budget::bytes(SampledAdmissionValidation::entryStorageBytes())));
    cost = add(cost, queries);
    cost = add(cost, multiply(r, add(1, multiply(2, b))));
    // verifyInvocation, actual-Rule queries, prepared-entry selection and the
    // successful Rule list all compare identities before further work.
    cost = add(cost, add(1, multiply(2, r)));
    cost = add(cost, multiply(r, r));
    return add(cost, multiply(r, schemaRecordCost<RuleOp>()));
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
            "ac.message", "ac.sampled_admission", "ac.transaction_wiring"}) {
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
            (key == "ac.sampled_admission" || key == "ac.transaction_wiring"))
          local.hasSamplingMetadata = true;
        if (directRule && key == "ac.sampled_admission" && attribute) {
          auto schema = sampledSchemaCost(attribute, metadata, op);
          if (failed(schema))
            return WalkResult::interrupt();
          local.sampledSchema = Budget::add(local.sampledSchema, *schema);
        }
        if (op == definition && key == "ac.transaction_wiring" && attribute) {
          local.hasWiring = true;
          local.wiringMetadata = metadata;
          if (failed(countWiringSchema(attribute, local, op)))
            return WalkResult::interrupt();
        }
      }
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
      records = Budget::add(
          records,
          Budget::add(local.obligations,
                      Budget::multiply(local.sampledRules,
                                       schemaRecordCost<Operation *>())));
      uint64_t cost = Budget::add(
          local.operations,
          Budget::add(records,
                      Budget::add(Budget::multiply(comparisons, local.metadata),
                                  local.carriers)));
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
      // Native Module verification runs before the builder can share its
      // budget with the local verifier. Prepay a conservative bound for the
      // sampled schema, stop endpoints, identity walks and prefix atom sets.
      if (local.hasSamplingMetadata) {
        // Definition stops may retain duplicate check endpoints; each prefix
        // proof and native schema parse have their own bounded inventories.
        uint64_t stops = Budget::add(local.ruleResults,
                                     Budget::multiply(2, local.obligations));
        auto proof = sampledProofCost(definition, stops);
        if (failed(proof))
          return failure();
        uint64_t schema = nativeSampledSchemaCost(local);
        cost = Budget::add(
            cost, Budget::add(*proof, Budget::add(schema, local.operands)));
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
    auto prepared = validation.prepare(definition, bindings);
    if (failed(prepared))
      return failure();
    if (!*prepared)
      return definition->emitOpError()
             << "sampling catalog lacks prepared boundaries";
    const auto &scope = (*prepared)->getScope();
    HardwareIdentityStopPolicy stops{HardwareIdentityTraversal::DefinitionLocal,
                                     (*prepared)->getBoundaries()};
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
      auto bindings = definitionSourceChecks(analysis, definition, local,
                                             validation, budget);
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
  SampledAdmissionValidation validation;
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

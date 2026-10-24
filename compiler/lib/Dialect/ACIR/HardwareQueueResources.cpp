#include "HardwareQueueResources.h"
#include "HardwareGrantProof.h"
#include "ACIRSourceContracts.h"
#include "HardwareSourceCheckPreflight.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/DenseMap.h"
#include <functional>
#include <memory>
#include <optional>

using namespace mlir;
namespace acir::ac::detail {
namespace {
struct QueueEndpoint {
  HardwareIdentityScope scope;
  HardwareBindings bindings;
  InstanceOp queue;
  bool unboundPush = false, unboundPop = false;
};
struct QueuePins {
  DictionaryAttr id;
  bool push;
  Value path, data;
  RuleOp rule;
  uint64_t ordinal = 0;
};
using IncomingDemand = std::pair<DictionaryAttr, bool>;
using QueueDemands =
    std::pair<SmallVector<IncomingDemand>, SmallVector<DictionaryAttr>>;
struct QueueDemandDefinition {
  ModuleOp module;
  QueueDemands demands;
};
// One complete lexical demand closure for a synchronous, read-only public
// invocation. Root/binding and physical occurrence proofs remain separate.
struct QueueSampleEntry {
  HardwareBindings bindings;
  std::unique_ptr<SampledAdmissionView> view;
};
struct MemoryClosureFacts {
  bool allocation = false, effect = false;
};
struct MemoryClosureEntry {
  HardwareBindings bindings;
  MemoryClosureFacts facts;
  bool complete = false;
};
struct QueueDemandInventory {
  SmallVector<QueueDemandDefinition, 0> definitions;
  llvm::DenseMap<Operation *, size_t> definitionIndex;
  bool complete = false;
  // Parsed schema belongs to this immutable analysis/budget invocation. Actual
  // occurrence resource, domain and physical routing proofs remain separate.
  llvm::DenseMap<Operation *, SmallVector<QueueSampleEntry, 0>> sampled;
  llvm::DenseMap<Operation *, SmallVector<std::unique_ptr<MemoryClosureEntry>, 0>> memoryClosure;
};
bool same(const HardwareIdentityEndpoint &a, const HardwareIdentityEndpoint &b) {
  return a.value == b.value && a.scope.root == b.scope.root &&
         a.scope.path == b.scope.path &&
         sameHardwareBindings(a.scope.rootBindings, b.scope.rootBindings) &&
         areEquivalentHardwareTypes(a.resolvedType, b.resolvedType);
}
LogicalResult exact(DictionaryAttr row, ArrayRef<StringRef> fields,
                    Operation *site) {
  if (!row || row.size() != fields.size())
    return site->emitOpError() << "queue wiring requires its exact closed fields";
  for (StringRef field : fields)
    if (!row.get(field))
      return site->emitOpError() << "queue wiring is missing " << field;
  return success();
}
FailureOr<uint64_t> ordinal(DictionaryAttr row, StringRef key, Operation *site) {
  auto value = row ? row.getAs<IntegerAttr>(key) : IntegerAttr();
  if (!value || !value.getType().isSignlessInteger(64) || value.getInt() < 0)
    return site->emitOpError() << "queue wiring " << key << " requires nonnegative i64";
  return uint64_t(value.getInt());
}
class QueueWiringProof {
public:
  QueueWiringProof(const HardwareAnalysis &analysis, SourceCheckBudget &budget,
                   ModuleOp module, const HardwareBindings &bindings,
                   llvm::DenseSet<OpOperand *> &verifiedUses,
                   QueueDemandInventory &demandInventory,
                   bool authenticate = true)
      : analysis(analysis), budget(budget), module(module), bindings(bindings),
        scope{module, bindings, {}}, verifiedUses(verifiedUses),
        demandInventory(demandInventory), authenticate(authenticate) {}

  FailureOr<uint64_t> incomingMemoryPorts() { return memoryIncomingCount(); }

  LogicalResult run() {
    if (authenticate && failed(memoryWiring())) return failure();
    if (failed(childSummaryRoots())) return failure();
    auto raw = module->getAttr("ac.queue_wiring");
    if (!raw) {
      if (module.getBody().hasOneBlock())
        for (auto instance : module.getBody().front().getOps<InstanceOp>()) {
          if (failed(step(instance))) return failure();
          auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
          auto childWiring = child ? child->getAttrOfType<DictionaryAttr>("ac.queue_wiring") : DictionaryAttr();
          auto incoming = childWiring ? childWiring.getAs<ArrayAttr>("incoming") : ArrayAttr();
          if (childWiring && !incoming) return failure();
          if (incoming && failed(disabledChildPushes(instance, incoming))) return failure();
        }
      if (authenticate && module.getBody().hasOneBlock()) for (auto rule : module.getBody().front().getOps<RuleOp>()) {
        auto row = rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
        auto resources = row ? row.getAs<ArrayAttr>("resources") : ArrayAttr();
        bool queueResource = resources && llvm::any_of(resources, [](Attribute raw) {
          auto resource = dyn_cast<DictionaryAttr>(raw);
          auto kind = resource ? resource.getAs<StringAttr>("kind") : StringAttr();
          return !kind || kind.getValue() != "memory_live";
        });
        if (row && (queueResource || row.get("queue_proposals")))
          return rule->emitOpError() << "sampled queue metadata requires owning module queue wiring";
      }
      return privateUses();
    }
    auto wiring = dyn_cast<DictionaryAttr>(raw);
    if (!wiring) return module->emitOpError() << "queue wiring requires a dictionary";
    NamedAttrList core(wiring);
    core.erase("pop_summaries"); core.erase("external_pops");
    if (failed(exact(core.getDictionary(module.getContext()), {"queues", "incoming", "writers"}, module)))
      return failure();
    for (StringRef key : {"pop_summaries", "external_pops"})
      if (wiring.get(key) && (!wiring.getAs<ArrayAttr>(key) || wiring.getAs<ArrayAttr>(key).empty()))
        return module->emitOpError() << "optional queue wiring inventories must be nonempty arrays";
    auto queues = wiring.getAs<ArrayAttr>("queues");
    auto incoming = wiring.getAs<ArrayAttr>("incoming");
    auto writers = wiring.getAs<ArrayAttr>("writers");
    if (!queues || !incoming || !writers ||
        (queues.empty() && incoming.empty() && writers.empty() &&
         !wiring.get("pop_summaries") && !wiring.get("external_pops")))
      return module->emitOpError() << "queue wiring requires nonempty inventories";
    auto ports = getModulePortLayout(module);
    if (failed(ports)) return failure();
    // Every declaration occurs once, including bound legacy directions. The
    // source replay separately authenticates which pins were authored.
    llvm::DenseSet<Operation *> allocations;
    for (Attribute item : queues) {
      auto row = dyn_cast<DictionaryAttr>(item);
      if (failed(exact(row, {"declaration", "unbound_push", "unbound_pop"}, module)))
        return failure();
      auto declaration = row.getAs<DictionaryAttr>("declaration");
      auto push = row.getAs<BoolAttr>("unbound_push");
      auto pop = row.getAs<BoolAttr>("unbound_pop");
      if (!push || !pop || failed(verifyOccurrence(declaration, error()))) return failure();
      InstanceOp queue;
      for (auto candidate : module.getBody().front().getOps<InstanceOp>()) {
        if (failed(step(candidate))) return failure();
        if (isQueueInstance(candidate) &&
            candidate.getOccurrence() == declaration) {
          if (queue) return module->emitOpError() << "ambiguous queue declaration";
          queue = candidate;
        }
      }
      if (!queue || !allocations.insert(queue).second)
        return module->emitOpError() << "queue inventory must name each actual allocation once";
    }
    for (auto queue : module.getBody().front().getOps<InstanceOp>())
      if (isQueueInstance(queue) && !allocations.contains(queue))
        return queue->emitOpError() << "queue allocation missing from wiring inventory";

    uint64_t nextInput = ports->sourceInputCount;
    for (StringRef name : {"ac.transaction_wiring", "ac.transaction_returns"}) {
      Attribute raw = module->getAttr(name);
      if (!raw) continue;
      auto wiring = dyn_cast<DictionaryAttr>(raw);
      auto routes = wiring ? wiring.getAs<ArrayAttr>("incoming") : ArrayAttr();
      if (!routes)
        return module->emitOpError() << name << " requires an incoming route array";
      if (failed(step(module, SourceCheckBudget::add(1, routes.size())))) return failure();
      nextInput = SourceCheckBudget::add(nextInput,
          SourceCheckBudget::multiply(2, routes.size()));
    }
    for (Attribute item : incoming) {
      auto row = dyn_cast<DictionaryAttr>(item);
      auto parsed = route(row, true);
      if (failed(parsed)) return failure();
      auto path = ordinal(row, "path", module);
      if (failed(path) || *path != nextInput++)
        return module->emitOpError()
               << "queue inputs must follow the state proposal suffix and "
                  "public-result suffix";
      if (parsed->push) {
        auto data = ordinal(row, "data", module);
        if (failed(data) || *data != nextInput++) return failure();
      }
      for (const auto &prior : incomingPins)
        if (prior.id == parsed->id && prior.push == parsed->push)
          return module->emitOpError() << "duplicate incoming queue direction";
      if (failed(privateRoot(parsed->path, RouteEnable)) ||
          (parsed->push && failed(privateRoot(parsed->data, RouteData))) ||
          failed(pay(sizeof(QueuePins)))) return failure();
      incomingPins.push_back(*parsed);
    }
    llvm::DenseSet<Operation *> writtenRules;
    for (Attribute item : writers) {
      auto row = dyn_cast<DictionaryAttr>(item);
      if (failed(exact(row, {"registration", "proposals"}, module))) return failure();
      auto registration = row.getAs<DictionaryAttr>("registration");
      auto proposals = row.getAs<ArrayAttr>("proposals");
      if (!proposals || proposals.empty() || failed(verifyOccurrence(registration, error())))
        return failure();
      RuleOp rule;
      for (auto candidate : module.getBody().front().getOps<RuleOp>()) {
        if (failed(step(candidate))) return failure();
        if (candidate.getOccurrence() == registration) {
          if (rule) return module->emitOpError() << "ambiguous queue writer registration";
          rule = candidate;
        }
      }
      if (!rule || !writtenRules.insert(rule).second)
        return module->emitOpError() << "queue writer requires unique actual Rule";
      auto sampled = authenticate ? sample(rule)
          : FailureOr<const SampledAdmissionView *>(static_cast<const SampledAdmissionView *>(nullptr));
      if (failed(sampled) || (authenticate && proposals.size() != (*sampled)->queueProposals.size()))
        return module->emitOpError() << "queue writer differs from sampled proposals";
      for (auto [index, item] : llvm::enumerate(proposals)) {
        auto row = dyn_cast<DictionaryAttr>(item);
        auto parsed = route(row, false, rule);
        if (failed(parsed)) return failure();
        if (authenticate) {
        const auto &proposal = (*sampled)->queueProposals[index];
        if (!rule->getResult(proposal.rawPathResult).use_empty())
          return rule->emitOpError() << "private queue raw path witness escapes its proof suffix";
        if (parsed->id != proposal.queue ||
            parsed->push != (proposal.kind.getValue() == "push") ||
            parsed->path != rule->getResult(proposal.pathResult) ||
            (parsed->push && parsed->data != rule->getResult(*proposal.dataResult)))
          return rule->emitOpError() << "queue writer route differs from authenticated Rule suffix";
        }
        if (failed(privateRoot(parsed->path, RouteEnable)) ||
            (parsed->push && failed(privateRoot(parsed->data, RouteData))) ||
            failed(pay(sizeof(QueuePins)))) return failure();
        parsed->rule = rule; parsed->ordinal = index;
        localPins.push_back(*parsed);
      }
      if (authenticate && failed(resources(**sampled))) return failure();
    }
    if (authenticate) for (auto rule : module.getBody().front().getOps<RuleOp>()) {
      if (!rule->hasAttr("ac.sampled_admission")) continue;
      auto sampled = sample(rule);
      if (failed(sampled)) return failure();
      if (!(*sampled)->queueProposals.empty() && !writtenRules.contains(rule))
        return rule->emitOpError() << "queue proposals have no writer inventory";
      if (!writtenRules.contains(rule) && failed(resources(**sampled))) return failure();
    }
    if (!authenticate) for (auto rule : module.getBody().front().getOps<RuleOp>())
      if (failed(structuralResourceUses(rule))) return failure();
    if (authenticate && failed(disjointProposals())) return failure();
    if (failed(prospectiveRoutes(wiring, *ports))) return failure();
    // Compare every compiler-driven physical pin with its exact local union or
    // incoming route, including forwarded child directions with no local writer.
    for (Attribute item : queues) {
      auto row = cast<DictionaryAttr>(item);
      mlir::Builder b(module.getContext());
      auto id = b.getDictionaryAttr({b.getNamedAttr("owner", b.getDictionaryAttr({
          b.getNamedAttr("instance_path", b.getArrayAttr({}))})),
          b.getNamedAttr("declaration", row.get("declaration"))});
      auto target = resolve(id);
      if (failed(target)) return failure();
      for (bool push : {false, true}) {
        bool unbound = push ? target->unboundPush : target->unboundPop;
        auto candidates = proposals(id, push);
        if (failed(candidates)) return failure();
        if (!unbound && !candidates->empty())
          return module->emitOpError() << "transaction drives an authored queue direction";
        if (unbound) {
          if (failed(pins(id, push,
                  push ? target->queue.getInputs()[2]
                       : target->queue.getInputs()[4],
                  push ? target->queue.getInputs()[3] : Value(), *candidates)) ||
              failed(allow(target->queue, push ? 2 : 4, true, RouteEnable)) ||
              (push && failed(allow(target->queue, 3, true)))) return failure();
        }
      }
    }
    for (auto instance : module.getBody().front().getOps<InstanceOp>()) {
      auto child = dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(instance));
      if (!child) continue;
      auto wiring = child->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
      auto rows = wiring ? wiring.getAs<ArrayAttr>("incoming") : ArrayAttr();
      if (!rows) continue;
      for (Attribute item : rows) {
        auto row = dyn_cast<DictionaryAttr>(item);
        auto id = row ? row.getAs<DictionaryAttr>("queue") : DictionaryAttr();
        auto owner = id ? id.getAs<DictionaryAttr>("owner") : DictionaryAttr();
        auto path = owner ? owner.getAs<ArrayAttr>("instance_path") : ArrayAttr();
        auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
        auto pin = ordinal(row, "path", instance);
        if (!path || !kind || (kind.getValue() != "pop" && kind.getValue() != "push") ||
            failed(pin) || *pin >= instance.getNumOperands()) return failure();
        mlir::Builder b(module.getContext());
        SmallVector<Attribute> routePath{instance.getOccurrence()};
        llvm::append_range(routePath, path);
        auto relative = b.getDictionaryAttr({b.getNamedAttr("owner", b.getDictionaryAttr({
            b.getNamedAttr("instance_path", b.getArrayAttr(routePath))})),
            b.getNamedAttr("declaration", id.get("declaration"))});
        bool push = kind.getValue() == "push";
        Value data;
        if (push) {
          auto index = ordinal(row, "data", instance);
          if (failed(index) || *index >= instance.getNumOperands()) return failure();
          data = instance->getOperand(*index);
        }
        auto candidates = proposals(relative, push);
        if (failed(candidates) || failed(pins(relative, push,
            instance->getOperand(*pin), data, *candidates)) ||
            failed(allow(instance, *pin, true, RouteEnable))) return failure();
        if (push) {
          auto dataIndex = ordinal(row, "data", instance);
          if (failed(dataIndex) || failed(allow(instance, *dataIndex, true))) return failure();
        }
      }
    }
    return privateUses();
  }

private:
  LogicalResult pay(uint64_t bytes) {
    return budget.debit(SourceCheckBudget::add(1, SourceCheckBudget::bytes(bytes)), module);
  }
  enum RouteKind : unsigned { RouteEnable = 1, RouteData = 2 };
  LogicalResult allow(Operation *operation, unsigned operand, bool terminal = false,
                      unsigned kind = RouteData) {
    if (operand >= operation->getNumOperands() || failed(pay(sizeof(OpOperand *) * 3)))
      return failure();
    auto *edge = &operation->getOpOperand(operand);
    allowed[edge] |= kind;
    if (terminal) terminals.insert(edge);
    return success();
  }
  LogicalResult privateRoot(Value value, unsigned kind) {
    if (failed(pay(sizeof(Value) * 3))) return failure();
    privateRoots[value] |= kind;
    return success();
  }
  FailureOr<const SampledAdmissionView *> sampleFor(RuleOp rule,
                                                   const HardwareBindings &bound) {
    if (failed(step(rule))) return failure();
    auto found = demandInventory.sampled.find(rule);
    if (found != demandInventory.sampled.end())
      for (const auto &entry : found->second) {
        if (failed(step(rule, SourceCheckBudget::add(SourceCheckBudget::bindings(bound),
                                                    SourceCheckBudget::bindings(entry.bindings))))) return failure();
        if (sameHardwareBindings(bound, entry.bindings)) return entry.view.get();
      }
    auto view = parseSampledAdmission(rule, analysis, bound, budget);
    if (failed(view) || failed(pay(sizeof(QueueSampleEntry) + sizeof(SampledAdmissionView) +
                                  SourceCheckBudget::multiply(8, SourceCheckBudget::bindings(bound)) +
                                  (found == demandInventory.sampled.end() ? sizeof(Operation *) + 64 : 0))))
      return failure();
    auto owned = std::make_unique<SampledAdmissionView>(std::move(*view));
    auto *result = owned.get();
    // Parsing never reenters this sample cache. Publish only the complete view.
    if (found == demandInventory.sampled.end())
      found = demandInventory.sampled.try_emplace(rule).first;
    found->second.push_back({bound, std::move(owned)});
    return result;
  }
  FailureOr<const SampledAdmissionView *> sample(RuleOp rule) {
    return sampleFor(rule, bindings);
  }
  LogicalResult privateUses() {
    // Rule yields and identity routes may expose an enable through another
    // payload slot, so compare SSA origins. Child summaries are actual private
    // control boundaries: their underlying authored out_ready can also be used
    // independently as public payload without exposing the summary result.
    SmallVector<HardwareIdentityEndpoint, 0> summaryBoundaries;
    for (Value summary : unusedSummaryRoots) {
      if (failed(pay(sizeof(HardwareIdentityEndpoint)))) return failure();
      auto endpoint = makeIdentityEndpoint(summary, scope, analysis, budget);
      if (failed(endpoint)) return failure();
      summaryBoundaries.push_back(std::move(*endpoint));
    }
    SmallVector<std::pair<HardwareIdentityEndpoint, unsigned>> roots;
    for (auto [value, kind] : privateRoots) {
      auto endpoint = makeIdentityEndpoint(value, scope, analysis, budget);
      auto actual = succeeded(endpoint)
          ? normalizeIdentityEndpoint(*endpoint,
                {HardwareIdentityTraversal::OrdinaryInstances, summaryBoundaries},
                analysis, budget)
          : FailureOr<HardwareIdentityEndpoint>(failure());
      if (failed(actual) || failed(pay(sizeof(HardwareIdentityEndpoint) + sizeof(unsigned))))
        return failure();
      for (const auto &[prior, priorKind] : roots) {
        if (failed(step(module))) return failure();
        if (same(prior, *actual) && ((kind | priorKind) == (RouteEnable | RouteData)))
          return module->emitOpError() << "private queue enable cannot alias its payload route";
      }
      if (kind == (RouteEnable | RouteData))
        return module->emitOpError() << "private queue enable cannot alias its payload route";
      roots.push_back({*actual, kind});
    }
    llvm::DenseSet<std::pair<Value, unsigned>> seen;
    SmallVector<std::pair<Value, unsigned>> pending;
    for (auto [value, kinds] : privateRoots)
      for (unsigned kind : {unsigned(RouteEnable), unsigned(RouteData)})
        if (kinds & kind) {
          if (failed(pay(sizeof(Value) + sizeof(unsigned)))) return failure();
          pending.push_back({value, kind});
        }
    while (!pending.empty()) {
      auto [value, kind] = pending.pop_back_val();
      if (failed(step(module)) || failed(pay((sizeof(Value) + sizeof(unsigned)) * 3))) return failure();
      if (!seen.insert({value, kind}).second) continue;
      if (value.use_empty()) {
        if (unusedSummaryRoots.contains(value)) continue;
        return module->emitOpError() << "private queue route ends before its declared queue pin";
      }
      for (OpOperand &use : value.getUses()) {
        if (failed(step(use.getOwner()))) return failure();
        auto *edge = &use;
        auto found = allowed.find(edge);
        if (found == allowed.end() || !(found->second & kind)) {
          auto extract = dyn_cast<BitsExtractOp>(use.getOwner());
          if (!extract || use.getOperandNumber() != 0)
            return use.getOwner()->emitOpError() << "private queue route escapes its canonical payload/enable transport";
          auto before = normal(value, scope), after = normal(extract.getResult(), scope);
          if (failed(before) || failed(after) || !same(*before, *after) ||
              failed(allow(extract, 0, false, kind)))
            return use.getOwner()->emitOpError() << "private queue route permits only identity extracts";
        }
        if (failed(pay(sizeof(OpOperand *) * 3))) return failure();
        verifiedUses.insert(edge);
        if (terminals.contains(edge)) continue;
        Operation *consumer = use.getOwner();
        if (!consumer->getNumResults()) return failure();
        // Aggregate projection/conversion membership flags are not payload
        // transport. A secondary-result use would expose private information.
        for (Value extra : consumer->getResults().drop_front())
          if (!extra.use_empty())
            return consumer->emitOpError() << "private queue projection membership escapes";
        if (failed(pay(sizeof(Value) + sizeof(unsigned)))) return failure();
        // Select conditions influence a selected payload, but the payload is
        // never an enable route. Its remaining users still need full closure.
        unsigned nextKind = isa<BitsSelectOp>(consumer) ? RouteData : kind;
        pending.push_back({consumer->getResult(0), nextKind});
      }
    }
    return success();
  }

  std::function<InFlightDiagnostic()> error() { return [&] { return module->emitOpError(); }; }
  LogicalResult step(Operation *site, uint64_t count = 1) {
    return budget.debit(count, site);
  }
  FailureOr<HardwareIdentityEndpoint> normal(Value value, const HardwareIdentityScope &at) {
    if (failed(step(module))) return failure();
    auto found = normalized.find(value);
    if (found != normalized.end())
      for (const auto &entry : found->second) {
        if (failed(step(module, 1 + std::min(entry.input.path.size(), at.path.size())))) return failure();
        if (entry.input.root != at.root || entry.input.path != at.path) continue;
        if (failed(step(module, SourceCheckBudget::add(SourceCheckBudget::bindings(entry.input.rootBindings),
                                                      SourceCheckBudget::bindings(at.rootBindings))))) return failure();
        if (!sameHardwareBindings(entry.input.rootBindings, at.rootBindings)) continue;
        // The graph, complete bindings and ordinary-instance stop policy are
        // immutable for this proof object. Reuse only route normalization;
        // actual occurrence/domain/resource and physical pin proofs still run.
        if (failed(step(module, SourceCheckBudget::add(1 + entry.output.scope.path.size(),
                                                      SourceCheckBudget::bindings(entry.output.scope.rootBindings))))) return failure();
        return entry.output;
      }
    auto endpoint = makeIdentityEndpoint(value, at, analysis, budget);
    if (failed(endpoint)) return failure();
    auto output = normalizeIdentityEndpoint(*endpoint,
        {HardwareIdentityTraversal::OrdinaryInstances, {}}, analysis, budget);
    if (failed(output) || failed(pay(sizeof(NormalizedEntry) +
          SourceCheckBudget::multiply(sizeof(InstanceOp), at.path.size() + output->scope.path.size()) +
          SourceCheckBudget::multiply(8, SourceCheckBudget::add(SourceCheckBudget::bindings(at.rootBindings),
                                                              SourceCheckBudget::bindings(output->scope.rootBindings))) +
          (found == normalized.end() ? sizeof(Value) + 64 : 0)))) return failure();
    if (found == normalized.end()) found = normalized.try_emplace(value).first;
    found->second.push_back({at, *output});
    return *output;
  }
  // The caller may already have authenticated the complete normalized scope.
  // Read that endpoint once instead of repeating normalization for both bits.
  FailureOr<std::optional<bool>> literal(const HardwareIdentityEndpoint &endpoint) {
    auto constant = endpoint.value.getDefiningOp<BitsConstantOp>();
    if (!constant) return std::optional<bool>();
    HardwareBindings bound = endpoint.scope.rootBindings;
    for (auto instance : endpoint.scope.path) {
      auto next = analysis.bindInstance(instance, bound);
      if (failed(next)) return failure();
      bound = *next;
    }
    auto evaluated = analysis.evaluateStatic(constant.getValue(), bound, constant);
    if (failed(evaluated)) return failure();
    auto integer = dyn_cast<MathIntAttr>(*evaluated);
    if (!integer) return std::optional<bool>();
    llvm::APSInt value(integer.getCanonicalValue());
    if (value.isZero()) return std::optional<bool>(false);
    if (value == 1) return std::optional<bool>(true);
    return std::optional<bool>();
  }
  FailureOr<bool> literal(Value value, const HardwareIdentityScope &at, bool expected) {
    auto endpoint = normal(value, at);
    if (failed(endpoint)) return failure();
    auto known = literal(*endpoint);
    if (failed(known)) return failure();
    return *known && **known == expected;
  }
  FailureOr<QueueEndpoint> resolve(DictionaryAttr id) {
    if (failed(exact(id, {"owner", "declaration"}, module))) return failure();
    auto owner = id.getAs<DictionaryAttr>("owner");
    auto declaration = id.getAs<DictionaryAttr>("declaration");
    if (failed(verifyOwnerRef(owner, error())) ||
        failed(verifyOccurrence(declaration, error()))) return failure();
    QueueEndpoint result{scope, bindings, {}, false, false};
    ModuleOp current = module;
    for (Attribute occurrence : owner.getAs<ArrayAttr>("instance_path")) {
      InstanceOp selected;
      for (auto instance : current.getBody().front().getOps<InstanceOp>()) {
        if (failed(step(instance))) return failure();
        if (instance.getOccurrence() == occurrence) {
          if (selected) return module->emitOpError() << "ambiguous queue owner occurrence";
          selected = instance;
        }
      }
      auto child = selected ? dyn_cast_or_null<ModuleOp>(analysis.resolveCallee(selected)) : ModuleOp();
      if (!child || !child.getBody().hasOneBlock() || !analysis.getPrimitiveKind(child).empty())
        return module->emitOpError() << "queue owner must resolve actual ordinary instances";
      auto bound = analysis.bindInstance(selected, result.bindings);
      if (failed(bound)) return failure();
      result.bindings = *bound;
      result.scope.path.push_back(selected);
      current = child;
    }
    for (auto queue : current.getBody().front().getOps<InstanceOp>()) {
      if (failed(step(queue))) return failure();
      if (isQueueInstance(queue) && queue.getOccurrence() == declaration) {
        if (result.queue) return module->emitOpError() << "ambiguous queue allocation";
        result.queue = queue;
      }
    }
    if (!result.queue) return module->emitOpError() << "queue declaration has no physical allocation";
    auto wiring = current->getAttrOfType<DictionaryAttr>("ac.queue_wiring");
    auto rows = wiring ? wiring.getAs<ArrayAttr>("queues") : ArrayAttr();
    unsigned matches = 0;
    if (rows) for (Attribute item : rows) {
      auto row = dyn_cast<DictionaryAttr>(item);
      if (!row || row.get("declaration") != declaration) continue;
      auto push = row.getAs<BoolAttr>("unbound_push"), pop = row.getAs<BoolAttr>("unbound_pop");
      if (!push || !pop) return failure();
      result.unboundPush = push.getValue(); result.unboundPop = pop.getValue(); ++matches;
    }
    if (matches != 1) return module->emitOpError() << "queue target requires one inventory entry";
    return result;
  }
  FailureOr<QueuePins> route(DictionaryAttr row, bool input, RuleOp rule = {}) {
    auto kind = row ? row.getAs<StringAttr>("kind") : StringAttr();
    if (!kind || (kind.getValue() != "push" && kind.getValue() != "pop"))
      return module->emitOpError() << "queue route kind must be push or pop";
    bool push = kind.getValue() == "push";
    if (failed(push ? exact(row, {"queue", "kind", "path", "data"}, module)
                    : exact(row, {"queue", "kind", "path"}, module))) return failure();
    auto target = resolve(row.getAs<DictionaryAttr>("queue"));
    auto index = ordinal(row, "path", module);
    if (failed(target) || failed(index)) return failure();
    auto count = input ? module.getFunctionType().getNumInputs() : rule.getNumResults();
    if (*index >= count) return module->emitOpError() << "queue route outside physical ports";
    auto get = [&](unsigned index) -> Value {
      return input ? Value(module.getBody().front().getArgument(index)) : Value(rule->getResult(index));
    };
    QueuePins result{row.getAs<DictionaryAttr>("queue"), push, get(*index), {}};
    auto type = analysis.resolveType(result.path.getType(), bindings, module);
    auto width = succeeded(type) ? analysis.getPackedWidth(*type, bindings, module) : FailureOr<uint64_t>(failure());
    if (failed(type) || !isa<BitsType>(*type) || failed(width) || *width != 1)
      return module->emitOpError() << "queue proposal enable requires Bits1";
    if (push) {
      auto data = ordinal(row, "data", module);
      if (failed(data) || *data >= count || *data != *index + 1) return failure();
      result.data = get(*data);
      auto actual = analysis.resolveType(result.data.getType(), bindings, module);
      auto expected = analysis.resolveType(target->queue.getInputs()[3].getType(),
                                           target->bindings, module);
      if (failed(actual) || failed(expected) || !areEquivalentHardwareTypes(*actual, *expected))
        return module->emitOpError() << "queue proposal payload type differs from actual allocation";
    }
    if (!(push ? target->unboundPush : target->unboundPop))
      return module->emitOpError() << "queue proposal targets an authored direction";
    return result;
  }
  FailureOr<SmallVector<QueuePins>> proposals(DictionaryAttr id, bool push) {
    SmallVector<QueuePins> result;
    if (!push) for (const auto &pin : incomingPins)
      if (pin.id == id && !pin.push) result.push_back(pin);
    for (const auto &pin : localPins) if (pin.id == id && pin.push == push) result.push_back(pin);
    if (push) for (const auto &pin : incomingPins) if (pin.id == id && pin.push) {
      if (!result.empty()) return module->emitOpError() << "local and incoming queue writers conflict";
      result.push_back(pin);
    }
    return result;
  }
  LogicalResult pins(DictionaryAttr id, bool push, Value enable, Value data,
                     ArrayRef<QueuePins> candidates) {
    auto actual = normal(enable, scope);
    if (failed(actual)) return failure();
    if (candidates.empty()) {
      auto zero = literal(enable, scope, false);
      if (failed(zero) || !*zero)
        return module->emitOpError() << "undriven transactional queue direction requires zero enable";
      return success();
    }
    // Each additional proposal uses the existing OR / select merge. Exact
    // expression structure retains four-state behavior, including Z payloads.
    auto payload = push ? normal(data, scope) : FailureOr<HardwareIdentityEndpoint>(failure());
    if (push && failed(payload)) return failure();
    for (size_t i = candidates.size(); i > 1; --i) {
      const auto &candidate = candidates[i - 1];
      auto binary = actual->value.getDefiningOp<BitsBinaryOp>();
      if (!binary || binary.getOpcode() != "or")
        return module->emitOpError() << "queue enable union differs from proposal inventory";
      auto rhs = normal(binary.getRhs(), actual->scope), expected = normal(candidate.path, scope);
      if (failed(rhs) || failed(expected) || !same(*rhs, *expected)) return failure();
      if (failed(allow(binary, 0, false, RouteEnable)) || failed(allow(binary, 1, false, RouteEnable))) return failure();
      actual = normal(binary.getLhs(), actual->scope);
      if (failed(actual)) return failure();
      if (push) {
        auto expectedData = normal(candidate.data, scope);
        if (failed(expectedData)) return failure();
        payload = peelSelection(*payload, *expected, *expectedData);
        if (failed(payload)) return failure();
      }
    }
    auto expected = normal(candidates.front().path, scope);
    if (failed(expected) || !same(*actual, *expected))
      return module->emitOpError() << "queue physical enable differs from proposal route";
    if (push) {
      auto expectedData = normal(candidates.front().data, scope);
      if (failed(expectedData) || !same(*payload, *expectedData))
        return module->emitOpError() << "queue physical payload differs from proposal route";
    }
    return success();
  }
  FailureOr<HardwareBindings> scopeBindings(const HardwareIdentityScope &at) {
    HardwareBindings result = at.rootBindings;
    for (InstanceOp instance : at.path) {
      if (failed(step(instance))) return failure();
      auto next = analysis.bindInstance(instance, result);
      if (failed(next)) return failure();
      result = std::move(*next);
    }
    return result;
  }
  FailureOr<HardwareIdentityEndpoint> projectionRoot(
      Value value, const HardwareIdentityScope &at, ArrayRef<Attribute> path) {
    auto current = normal(value, at);
    if (failed(current)) return failure();
    for (Attribute item : llvm::reverse(path)) {
      if (auto field = dyn_cast<StringAttr>(item)) {
        auto get = current->value.getDefiningOp<StructGetOp>();
        if (!get || get.getFieldAttr() != field || failed(allow(get, 0)))
          return module->emitOpError() << "queue payload field projection differs";
        current = normal(get.getValue(), current->scope);
      } else {
        auto get = current->value.getDefiningOp<TableGetOp>();
        if (!get) return module->emitOpError() << "queue payload Table projection differs";
        auto index = normal(get.getIndex(), current->scope);
        if (failed(index)) return failure();
        auto constant = index->value.getDefiningOp<BitsConstantOp>();
        auto bound = scopeBindings(index->scope);
        if (!constant || failed(bound)) return failure();
        auto evaluated = analysis.evaluateStatic(constant.getValue(), *bound, get);
        if (failed(evaluated)) return failure();
        auto integer = dyn_cast<MathIntAttr>(*evaluated);
        auto ordinal = cast<IntegerAttr>(item).getInt();
        if (!integer || llvm::APSInt(integer.getCanonicalValue()) != ordinal ||
            failed(allow(get, 0)))
          return module->emitOpError() << "queue payload Table ordinal differs";
        current = normal(get.getInput(), current->scope);
      }
      if (failed(current)) return failure();
    }
    return *current;
  }
  FailureOr<HardwareIdentityEndpoint> peelSelection(
      const HardwareIdentityEndpoint &selected,
      const HardwareIdentityEndpoint &condition,
      const HardwareIdentityEndpoint &whenTrue) {
    std::optional<HardwareIdentityEndpoint> whenFalse;
    SmallVector<Attribute> path;
    std::function<LogicalResult(Value, Type, const HardwareIdentityScope &, unsigned)> visit =
        [&](Value value, Type type, const HardwareIdentityScope &at, unsigned depth) -> LogicalResult {
      if (depth > 128 || failed(step(module))) return failure();
      auto actual = normal(value, at);
      if (failed(actual)) return failure();
      if (auto structure = dyn_cast<StructType>(type)) {
        auto create = actual->value.getDefiningOp<StructCreateOp>();
        auto declaration = analysis.lookupStruct(structure);
        if (!create || !declaration || create->getNumOperands() != declaration.getFields().size())
          return module->emitOpError() << "queue Struct selection reconstruction differs";
        for (auto [ordinal, raw] : llvm::enumerate(declaration.getFields())) {
          auto field = cast<DictionaryAttr>(raw);
          if (failed(pay(sizeof(Attribute)))) return failure();
          path.push_back(field.getAs<StringAttr>("name"));
          auto bound = scopeBindings(actual->scope);
          if (failed(bound)) return failure();
          auto fieldType = analysis.resolveType(field.getAs<TypeAttr>("type").getValue(), *bound, create);
          if (failed(fieldType) || failed(allow(create, ordinal)) ||
              failed(visit(create->getOperand(ordinal), *fieldType, actual->scope, depth + 1))) return failure();
          path.pop_back();
        }
        return success();
      }
      if (auto table = dyn_cast<TableType>(type)) {
        auto create = actual->value.getDefiningOp<TableCreateOp>();
        auto bound = scopeBindings(actual->scope);
        if (!create || failed(bound)) return failure();
        auto count = analysis.getTableSize(table, *bound, create);
        if (failed(count) || create->getNumOperands() != *count) return failure();
        Builder builder(create.getContext());
        for (uint64_t ordinal = 0; ordinal < *count; ++ordinal) {
          if (failed(pay(sizeof(Attribute)))) return failure();
          path.push_back(builder.getI64IntegerAttr(ordinal));
          if (failed(allow(create, ordinal)) ||
              failed(visit(create->getOperand(ordinal), table.getElementType(), actual->scope, depth + 1)))
            return failure();
          path.pop_back();
        }
        return success();
      }
      Value carrier = actual->value;
      if (isa<EnumType>(type)) {
        auto conversion = carrier.getDefiningOp<EnumFromBitsOp>();
        if (!conversion || failed(allow(conversion, 0))) return failure();
        carrier = conversion.getInput();
      }
      auto chosen = normal(carrier, actual->scope);
      if (failed(chosen)) return failure();
      auto select = chosen->value.getDefiningOp<BitsSelectOp>();
      if (!select) return module->emitOpError() << "queue payload requires four-state selection";
      auto predicate = normal(select.getCondition(), chosen->scope);
      if (failed(predicate) || !same(*predicate, condition))
        return module->emitOpError() << "queue payload selection differs from proposal enable";
      for (unsigned operand = 0; operand != 3; ++operand)
        if (failed(allow(select, operand, false, operand == 0 ? RouteEnable : RouteData))) return failure();
      Value trueValue = select.getTrueValue(), falseValue = select.getFalseValue();
      if (isa<EnumType>(type)) {
        auto t = trueValue.getDefiningOp<EnumToBitsOp>();
        auto f = falseValue.getDefiningOp<EnumToBitsOp>();
        if (!t || !f || failed(allow(t, 0)) || failed(allow(f, 0))) return failure();
        trueValue = t.getInput(); falseValue = f.getInput();
      }
      auto t = projectionRoot(trueValue, chosen->scope, path);
      auto f = projectionRoot(falseValue, chosen->scope, path);
      if (failed(t) || failed(f) || !same(*t, whenTrue) ||
          (whenFalse && !same(*whenFalse, *f)))
        return module->emitOpError() << "queue aggregate selection mixes payload origins";
      whenFalse = *f;
      return success();
    };
    if (failed(visit(selected.value, selected.resolvedType, selected.scope, 0)) || !whenFalse)
      return failure();
    return *whenFalse;
  }
  struct PathAtom {
    HardwareIdentityEndpoint value;
    bool positive;
  };
  struct PathFactors {
    SmallVector<PathAtom> atoms;
    bool falsePath = false;
  };
  FailureOr<PathFactors> factors(Value value, const HardwareIdentityScope &at,
                                 bool retainExpandedSelectors = false, bool positive = true) {
    PathFactors result;
    struct Pending { Value value; HardwareIdentityScope scope; bool positive; };
    SmallVector<Pending> pending;
    SmallVector<PathAtom> seen;
    if (failed(pay(sizeof(Pending)))) return failure();
    pending.push_back({value, at, positive});
    while (!pending.empty()) {
      auto item = pending.pop_back_val();
      auto endpoint = normal(item.value, item.scope);
      if (failed(endpoint)) return failure();
      bool visited = false;
      for (const auto &prior : seen) {
        if (failed(step(module))) return failure();
        if (prior.positive == item.positive && same(prior.value, *endpoint)) { visited = true; break; }
      }
      if (visited) continue;
      if (failed(pay(sizeof(PathAtom)))) return failure();
      seen.push_back({*endpoint, item.positive});
      auto known = literal(*endpoint);
      if (failed(known)) return failure();
      if (*known) {
        result.falsePath |= item.positive != **known;
        continue;
      }
      auto push = [&](Value next, bool positive) -> LogicalResult {
        if (failed(pay(sizeof(Pending)))) return failure();
        pending.push_back({next, endpoint->scope, positive});
        return success();
      };
      if (auto binary = endpoint->value.getDefiningOp<BitsBinaryOp>();
          item.positive && binary && binary.getOpcode() == "and") {
        if (retainExpandedSelectors) {
          if (failed(pay(sizeof(PathAtom)))) return failure();
          result.atoms.push_back({*endpoint, true});
        }
        if (failed(push(binary.getLhs(), true)) || failed(push(binary.getRhs(), true))) return failure();
      } else if (auto invert = endpoint->value.getDefiningOp<BitsUnaryOp>();
                 invert && invert.getOpcode() == "not") {
        if (retainExpandedSelectors) {
          if (failed(pay(sizeof(PathAtom)))) return failure();
          result.atoms.push_back({*endpoint, item.positive});
        }
        if (failed(push(invert.getInput(), !item.positive))) return failure();
      } else {
        if (failed(pay(sizeof(PathAtom)))) return failure();
        result.atoms.push_back({*endpoint, item.positive});
      }
    }
    return result;
  }
  FailureOr<bool> retainedSelector(const SampledAdmissionView &view,
                                   const SampledQueueProposalView &proposal,
                                   const HardwareIdentityEndpoint &selector,
                                   const HardwareIdentityScope &at) {
    for (auto [ordinal, control] : llvm::enumerate(view.controls)) {
      if (failed(step(view.rule))) return failure();
      if (control.kind != SampledControlKind::BranchSelect ||
          llvm::is_contained(proposal.excludedControls, ordinal)) continue;
      auto actual = normal(control.raw, at);
      if (failed(actual)) return failure();
      // A retained branch proves either-known for its scalar predicate. For
      // Bits1, four-state NOT is known exactly when its input is known, so the
      // proof also authenticates the base selector after polarity expansion.
      // No such implication is valid for AND (zero can mask an unknown input).
      for (unsigned depth = 0;; ++depth) {
        if (failed(step(view.rule))) return failure();
        if (same(*actual, selector)) return true;
        auto inverse = actual->value.getDefiningOp<BitsUnaryOp>();
        if (!inverse || inverse.getOpcode() != "not") break;
        if (depth >= 128) return module->emitOpError() << "retained Queue selector inversion chain is too deep";
        auto bound = scopeBindings(actual->scope);
        auto width = succeeded(bound)
            ? analysis.getPackedWidth(actual->resolvedType, *bound, inverse)
            : FailureOr<uint64_t>(failure());
        if (failed(bound) || !isa<BitsType>(actual->resolvedType) || failed(width) || *width != 1) return failure();
        auto input = normal(inverse.getInput(), actual->scope);
        if (failed(input)) return failure();
        auto inputBound = scopeBindings(input->scope);
        auto inputWidth = succeeded(inputBound)
            ? analysis.getPackedWidth(input->resolvedType, *inputBound, inverse)
            : FailureOr<uint64_t>(failure());
        if (failed(inputBound) || !isa<BitsType>(input->resolvedType) || failed(inputWidth) || *inputWidth != 1) return failure();
        actual = std::move(input);
      }
    }
    return false;
  }
#include "HardwareQueueProspectiveRoutingProof.inc"
  LogicalResult resourcePath(const SampledAdmissionView &view,
                             const SampledControlView &control, Value condition,
                             const HardwareIdentityScope &at) {
    auto full = normal(condition, at);
    if (failed(full)) return failure();
    auto root = full->value.getDefiningOp<BitsBinaryOp>();
    if (!root || root.getOpcode() != "and")
      return module->emitOpError() << "queue resource path requires ancestry AND local predicate";
    // The final RHS is the authored local predicate, not an extra structural
    // ancestor. Its source identity is authenticated by source replay; final H
    // independently consumes this same operand. Prove every structural factor.
    auto ancestry = normal(root.getLhs(), full->scope);
    if (failed(ancestry)) return failure();
    SmallVector<SampledPathRef, 0> parents;
    auto path = control.executionPath;
    while (path.selectionControl) {
      if (parents.size() >= 128 || failed(pay(sizeof(SampledPathRef))) ||
          failed(step(module))) return failure();
      parents.push_back(path);
      path = view.controls[*path.selectionControl].executionPath;
    }
    for (const auto &parent : llvm::reverse(parents)) {
      if (failed(step(module))) return failure();
      auto part = ancestry->value.getDefiningOp<BitsBinaryOp>();
      if (!part || part.getOpcode() != "and")
        return module->emitOpError() << "queue resource ancestry is incomplete";
      auto actual = normal(part.getRhs(), ancestry->scope);
      auto expected = normal(view.controls[*parent.selectionControl].raw, at);
      if (failed(actual) || failed(expected)) return failure();
      if (!parent.whenTrue) {
        auto inverse = actual->value.getDefiningOp<BitsUnaryOp>();
        if (!inverse || inverse.getOpcode() != "not")
          return module->emitOpError() << "queue false-arm ancestry requires raw selector complement";
        actual = normal(inverse.getInput(), actual->scope);
        if (failed(actual)) return failure();
      }
      if (!same(*actual, *expected))
        return module->emitOpError() << "queue resource ancestry differs from actual selector";
      ancestry = normal(part.getLhs(), ancestry->scope);
      if (failed(ancestry)) return failure();
    }
    auto one = literal(ancestry->value, ancestry->scope, true);
    if (failed(one) || !*one)
      return module->emitOpError() << "queue resource path has extra or repeated ancestry";
    return success();
  }
#include "HardwareQueueProspectivePredicateProof.inc"
  LogicalResult resources(const SampledAdmissionView &sampled) {
    mlir::Builder b(module.getContext());
    for (const auto &resource : sampled.resources) {
      if (resource.kind.getValue() == "memory_live") continue;
      auto id = b.getDictionaryAttr({b.getNamedAttr("owner", resource.owner),
                                    b.getNamedAttr("declaration", resource.declaration)});
      auto target = resolve(id);
      if (failed(target)) return failure();
      const auto &control = sampled.controls[resource.control];
      auto raw = normal(control.raw, scope);
      auto select = succeeded(raw) ? raw->value.getDefiningOp<BitsSelectOp>() : BitsSelectOp();
      if (!select) return module->emitOpError() << "queue resource requires path-selected raw queue predicate";
      auto predicate = normal(select.getTrueValue(), raw->scope);
      auto expected = normal(target->queue.getOutputs()[1], target->scope);
      if (resource.kind.getValue() == "queue_ready" &&
          failed(prospectivePredicate(sampled, resource, *target, select.getTrueValue()))) return failure();
      auto bypass = literal(select.getFalseValue(), raw->scope, true);
      if (failed(resourcePath(sampled, control, select.getCondition(), scope))) return failure();
      auto clock = normal(sampled.clock, scope), reset = normal(sampled.reset, scope);
      auto queueClock = normal(target->queue.getInputs()[0], target->scope),
           queueReset = normal(target->queue.getInputs()[1], target->scope);
      if (failed(predicate) || failed(expected) || failed(bypass) || !*bypass ||
          (resource.kind.getValue() == "queue_valid" && !same(*predicate, *expected)) || failed(clock) || failed(reset) ||
          failed(queueClock) || failed(queueReset) || !same(*clock, *queueClock) || !same(*reset, *queueReset))
        return module->emitOpError() << "queue resource predicate or domain differs from actual allocation";
    }
    return llvm::any_of(sampled.resources, [](const auto &resource) {
      return resource.kind.getValue() == "queue_ready";
    }) ? prospectiveUseClosure(sampled) : success();
  }
#include "HardwareMemoryResources.inc"
#include "HardwareQueueProspectiveStructuralUses.inc"
  const HardwareAnalysis &analysis;
  SourceCheckBudget &budget;
  ModuleOp module;
  const HardwareBindings &bindings;
  HardwareIdentityScope scope;
  struct NormalizedEntry {
    HardwareIdentityScope input;
    HardwareIdentityEndpoint output;
  };
  llvm::DenseMap<Value, SmallVector<NormalizedEntry, 0>> normalized;
  SmallVector<QueuePins> incomingPins, localPins;
  llvm::DenseMap<Value, unsigned> privateRoots;
  llvm::DenseMap<OpOperand *, unsigned> allowed;
  llvm::DenseSet<OpOperand *> terminals;
  llvm::DenseSet<OpOperand *> prospectiveAllowed;
  llvm::DenseSet<Value> prospectivePrivate, prospectiveLeaves, unusedSummaryRoots;
  llvm::DenseSet<OpOperand *> &verifiedUses;
  QueueDemandInventory &demandInventory;
  bool authenticate;

};
} // namespace
FailureOr<uint64_t> getMemoryIncomingPortCount(ModuleOp module,
    const HardwareAnalysis &analysis, const HardwareBindings &bindings,
    SourceCheckBudget &budget) {
  llvm::DenseSet<OpOperand *> uses;
  QueueDemandInventory demands;
  return QueueWiringProof(analysis, budget, module, bindings, uses, demands, false).incomingMemoryPorts();
}
LogicalResult verifyQueueResourceWiring(const HardwareAnalysis &analysis,
    ArrayRef<std::pair<ModuleOp, HardwareBindings>> scopes, SourceCheckBudget &budget) {
  QueueDemandInventory demands;
  SmallVector<const std::pair<ModuleOp, HardwareBindings> *, 0> verifiedRoots;
  for (const auto &entry : scopes) {
    const auto &[module, bindings] = entry;
    if (failed(budget.debit(1, module))) return failure();
    bool verified = false;
    for (const auto *prior : verifiedRoots) {
      if (failed(budget.debit(1, module))) return failure();
      if (prior->first != module) continue;
      if (failed(budget.debit(SourceCheckBudget::add(
              SourceCheckBudget::bindings(bindings),
              SourceCheckBudget::bindings(prior->second)), module)))
        return failure();
      if (sameHardwareBindings(prior->second, bindings)) {
        verified = true;
        break;
      }
    }
    if (verified) continue;
    llvm::DenseSet<OpOperand *> uses;
    if (failed(QueueWiringProof(analysis, budget, module, bindings, uses,
                                demands).run())) return failure();
    // Only a complete proof establishes this exact module/binding root.
    // Descendant visits do not establish an independently checked child root.
    if (failed(budget.debit(SourceCheckBudget::add(
            1, SourceCheckBudget::bytes(sizeof(&entry))), module)))
      return failure();
    verifiedRoots.push_back(&entry);
  }
  return success();
}
FailureOr<llvm::DenseSet<OpOperand *>> getVerifiedQueueResourceUses(
    const HardwareAnalysis &analysis, SourceCheckBudget &budget) {
  llvm::DenseSet<OpOperand *> result;
  QueueDemandInventory demands;
  for (auto module : analysis.getPackage().getBody()->getOps<ModuleOp>()) {
    HardwareBindings bindings;
    bindings.owner = module;
    if (failed(QueueWiringProof(analysis, budget, module, bindings, result,
                                demands, false).run()))
      return failure();
  }
  return result;
}
FailureOr<llvm::DenseSet<OpOperand *>>
getVerifiedQueueResourceUses(const HardwareAnalysis &analysis) {
  SourceCheckBudget budget(HardwareSourceCheckLimits{});
  budget.setPhase("queue private route closure");
  return getVerifiedQueueResourceUses(analysis, budget);
}
} // namespace acir::ac::detail

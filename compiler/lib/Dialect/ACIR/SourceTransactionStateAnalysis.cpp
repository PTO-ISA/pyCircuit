#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"

#include "SourceTransactionSummary.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "pycircuit/Dialect/ACIR/SourceMethodAnalysis.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::ac {
namespace {

constexpr unsigned maxDepth = 128;

// A rejected raw graph must not be printed as an operation note, and its
// location has not necessarily passed preflight either. Keep these diagnostics
// independent of both, without changing any MLIRContext diagnostic settings.
InFlightDiagnostic boundedError(Operation *site) {
  return mlir::emitError(UnknownLoc::get(site->getContext()));
}

class StateAnalysisBudget {
public:
  uint64_t usedWork() const { return work; }

  bool charge(Operation *site, uint64_t units = 1) {
    if (exhausted)
      return false;
    if (units > maxWork - work) {
      boundedError(site) << "transaction state analysis work budget exceeded";
      exhausted = true;
      return false;
    }
    work += units;
    return true;
  }

  bool chargeBindings(Operation *site, const HardwareBindings &bindings) {
    if (!charge(site, 1 + bindings.integers.size() + bindings.types.size()))
      return false;
    for (const auto &entry : bindings.integers)
      if (!charge(site, entry.getKey().size()))
        return false;
    for (const auto &entry : bindings.types)
      if (!charge(site, entry.getKey().size()))
        return false;
    return true;
  }

  // Occurrence dictionaries are printed in the stable plan dump. Prepay their
  // generic traversal and spelling before copying them into expanded records.
  bool chargeOccurrence(Operation *site, DictionaryAttr occurrence) {
    SmallVector<Attribute> pending;
    if (!charge(site))
      return false;
    pending.push_back(occurrence);
    while (!pending.empty()) {
      Attribute attr = pending.pop_back_val();
      if (auto array = dyn_cast<ArrayAttr>(attr)) {
        if (!charge(site, 1 + array.size()))
          return false;
        pending.append(array.begin(), array.end());
      } else if (auto dictionary = dyn_cast<DictionaryAttr>(attr)) {
        if (!charge(site, 1 + dictionary.size()))
          return false;
        for (NamedAttribute field : dictionary) {
          if (!charge(site, field.getName().getValue().size()))
            return false;
          pending.push_back(field.getValue());
        }
      } else if (auto text = dyn_cast<StringAttr>(attr)) {
        if (!charge(site, 1 + text.getValue().size()))
          return false;
      } else if (auto symbol = dyn_cast<SymbolRefAttr>(attr)) {
        if (!charge(site, 1 + symbol.getRootReference().getValue().size() +
                              symbol.getNestedReferences().size()))
          return false;
        for (FlatSymbolRefAttr nested : symbol.getNestedReferences())
          if (!charge(site, nested.getValue().size()))
            return false;
      } else if (!charge(site))
        return false;
    }
    return true;
  }

private:
  static constexpr uint64_t maxWork = 1024 * 1024;
  uint64_t work = 0;
  bool exhausted = false;
};

/// Check only immediate subelements; never invoke recursive walks or evaluate
/// an expression. Shared nodes are traversed once, but their cached height and
/// expanded cost participate at every edge/root. A shallow exponentially shared
/// graph therefore cannot bypass the bound of a later recursive verifier.
class AttributeTypePreflight {
public:
  explicit AttributeTypePreflight(StateAnalysisBudget &budget)
      : budget(budget) {}

  bool check(Operation *site, Attribute attribute) {
    return !attribute || check(site, Node(attribute));
  }
  bool check(Operation *site, Type type) {
    return !type || check(site, Node(type));
  }

private:
  using Node = llvm::PointerUnion<Attribute, Type>;
  struct Summary {
    uint64_t expandedWork = 0;
    unsigned height = 0;
    bool active = true;
  };
  struct Frame {
    Node node;
    SmallVector<Node, 0> children;
    size_t next = 0;
    uint64_t expandedWork = 1;
    unsigned height = 0;
    bool initialized = false;
  };

  bool addWork(Operation *site, uint64_t &total, uint64_t units) {
    if (units > maxExpandedWork - total) {
      boundedError(site)
          << "transaction attribute/type graph expansion budget exceeded";
      return false;
    }
    total += units;
    return true;
  }

  bool check(Operation *site, Node root) {
    auto known = summaries.find(root);
    if (known != summaries.end())
      return budget.charge(site, known->second.expandedWork);
    SmallVector<Frame, 0> stack;
    if (!budget.charge(site, 2))
      return false;
    summaries.try_emplace(root);
    stack.push_back({root, {}, 0, 1, 0, false});
    while (!stack.empty()) {
      auto &frame = stack.back();
      if (!frame.initialized) {
        frame.initialized = true;
        bool valid = true;
        auto child = [&](Node node) {
          if (!valid || !node)
            return;
          if (!budget.charge(site, 2)) {
            valid = false;
            return;
          }
          frame.children.push_back(node);
        };
        auto attribute = [&](Attribute attr) { child(Node(attr)); };
        auto type = [&](Type ty) { child(Node(ty)); };
        uint64_t storage = 0;
        if (auto attr = frame.node.dyn_cast<Attribute>()) {
          if (auto text = dyn_cast<StringAttr>(attr))
            storage = text.getValue().size();
          else if (auto opaque = dyn_cast<OpaqueAttr>(attr))
            storage = opaque.getAttrData().size();
          else if (auto integer = dyn_cast<IntegerAttr>(attr))
            storage = (uint64_t(integer.getValue().getBitWidth()) + 7) / 8;
          else if (auto dense = dyn_cast<DenseElementsAttr>(attr))
            storage = dense.getRawData().size();
          else if (auto array = dyn_cast<DenseArrayAttr>(attr))
            storage = array.getRawData().size();
          if (!budget.charge(site, storage) ||
              !addWork(site, frame.expandedWork, storage))
            return false;
          attr.walkImmediateSubElements(attribute, type);
        } else {
          auto ty = frame.node.get<Type>();
          if (auto opaque = dyn_cast<OpaqueType>(ty))
            storage = opaque.getTypeData().size();
          if (!budget.charge(site, storage) ||
              !addWork(site, frame.expandedWork, storage))
            return false;
          ty.walkImmediateSubElements(attribute, type);
        }
        if (!valid)
          return false;
      }
      if (frame.next == frame.children.size()) {
        summaries.find(frame.node)->second = {frame.expandedWork, frame.height,
                                              false};
        stack.pop_back();
        continue;
      }
      Node child = frame.children[frame.next];
      auto found = summaries.find(child);
      if (found != summaries.end()) {
        if (found->second.active) {
          boundedError(site) << "cyclic transaction attribute/type graph";
          return false;
        }
        unsigned height = found->second.height + 1;
        if (height > maxDepth) {
          boundedError(site)
              << "transaction attribute/type graph depth budget exceeded";
          return false;
        }
        frame.height = std::max(frame.height, height);
        if (!addWork(site, frame.expandedWork, found->second.expandedWork))
          return false;
        ++frame.next;
        continue;
      }
      if (stack.size() > maxDepth) {
        boundedError(site)
            << "transaction attribute/type graph depth budget exceeded";
        return false;
      }
      if (!budget.charge(site, 2))
        return false;
      summaries.try_emplace(child);
      stack.push_back({child, {}, 0, 1, 0, false});
    }
    return budget.charge(site, summaries.find(root)->second.expandedWork);
  }

  static constexpr uint64_t maxExpandedWork = 1024 * 1024;
  StateAnalysisBudget &budget;
  llvm::DenseMap<Node, Summary> summaries;
};

struct PackageInventory {
  SmallVector<SourceTransactionOp> transactions;
  llvm::DenseMap<Operation *, uint64_t> operationCosts;
  llvm::DenseMap<Operation *, uint64_t> moduleCosts;
  llvm::DenseMap<Attribute, uint64_t> methodCosts;
};

LogicalResult preflight(mlir::ModuleOp package, StateAnalysisBudget &budget,
                        PackageInventory &inventory) {
  // Bound nesting and growth before invoking native recursive verifiers.
  struct Pending {
    Operation *operation;
    Operation *module;
    unsigned depth;
  };
  SmallVector<Pending> pending;
  AttributeTypePreflight subelements(budget);
  if (!budget.charge(package))
    return failure();
  pending.push_back({package, nullptr, 0});
  while (!pending.empty()) {
    auto [op, module, depth] = pending.pop_back_val();
    if (depth > maxDepth)
      return boundedError(op)
             << "transaction state analysis structural depth exceeded";
    if (isa<ModuleOp>(op))
      module = op;
    uint64_t units = 1 + uint64_t(op->getNumOperands()) + op->getNumResults();
    if (!budget.charge(op, units))
      return failure();
    uint64_t before = budget.usedWork();
    if (!subelements.check(op, op->getRawDictionaryAttrs()) ||
        !subelements.check(op, op->getLoc()))
      return failure();
    // Property conversion is shallow for the owning operation representation;
    // prepay its storage before requesting the attribute view. Inherent attrs
    // stored as properties are absent from the raw discardable dictionary.
    if (op->getPropertiesStorageSize()) {
      if (!budget.charge(op, op->getPropertiesStorageSize()) ||
          !subelements.check(op, op->getPropertiesAsAttribute()))
        return failure();
    }
    for (Type type : op->getOperandTypes())
      if (!subelements.check(op, type))
        return failure();
    for (Type type : op->getResultTypes())
      if (!subelements.check(op, type))
        return failure();
    for (Region &region : op->getRegions())
      for (Block &block : region)
        for (Type type : block.getArgumentTypes())
          if (!subelements.check(op, type))
            return failure();
    inventory.operationCosts[op] = units + budget.usedWork() - before;
    if (auto transaction = dyn_cast<SourceTransactionOp>(op)) {
      if (!module || module->getParentOp() != package)
        return boundedError(transaction)
               << "transaction state analysis requires package-owned "
                  "definitions";
      if (!budget.charge(op))
        return failure();
      inventory.transactions.push_back(transaction);
    }
    // Reverse insertion preserves source order without recursive IR walking.
    for (Region &region : llvm::reverse(op->getRegions()))
      for (Block &block : llvm::reverse(region)) {
        if (!budget.charge(op, 1 + block.getNumArguments()))
          return failure();
        inventory.operationCosts[op] += 1 + block.getNumArguments();
        for (Operation &child : llvm::reverse(block)) {
          if (!budget.charge(op))
            return failure();
          pending.push_back({&child, module, depth + 1});
        }
      }
  }
  return success();
}

LogicalResult priceResolver(mlir::ModuleOp package, StateAnalysisBudget &budget,
                            PackageInventory &inventory) {
  // The resolver verifies module signatures, direct Instance declarations and
  // Method headers. It does not recursively verify every callable's payload
  // body. Keep those costs separate from per-invocation body expansion below.
  constexpr uint64_t limit = 1024 * 1024;
  auto add = [&](uint64_t &total, uint64_t units) {
    total = total > limit || units > limit - total ? limit + 1 : total + units;
  };
  for (Operation &op : package.getBody()->getOperations()) {
    if (!budget.charge(&op))
      return failure();
    auto module = dyn_cast<ModuleOp>(op);
    if (!module)
      continue;
    uint64_t cost = 0;
    // Each visited module has at most two receiverHeader checks: caller/target
    // entry followed by receiver-path traversal or the final Method lookup.
    add(cost, inventory.operationCosts.lookup(module));
    add(cost, inventory.operationCosts.lookup(module));
    uint64_t bodySize = module.getBody().front().getOperations().size();
    add(cost, bodySize);
    for (Operation &declaration : module.getBody().front()) {
      if (!budget.charge(&declaration))
        return failure();
      if (auto instance = dyn_cast<InstanceOp>(declaration)) {
        // Instance name uniqueness scans the direct body. Its occurrence is
        // also checked again when deriving the receiver occurrence suffix.
        add(cost, bodySize);
        add(cost, inventory.operationCosts.lookup(instance));
        add(cost, inventory.operationCosts.lookup(instance));
      } else if (auto method = dyn_cast<SourceMethodOp>(declaration)) {
        // Non-selected candidates receive invariant/region-shape checks only.
        add(cost,
            1 + method.getCaptures().size() + method.getArgumentNames().size());
        SourceMethodView view{module, method, {}};
        if (!budget.charge(&declaration))
          return failure();
        inventory.methodCosts.try_emplace(
            view.getSymbol(), inventory.operationCosts.lookup(method));
      }
    }
    if (!budget.charge(module))
      return failure();
    inventory.moduleCosts.try_emplace(module, cost);
  }
  return success();
}

struct InvocationResult {
  SmallVector<StateOwnerId> touched;
  SmallVector<BoundValueId> yielded;
  GuardBoundaryId guard = 0;
};

class TransactionPlanBuilder {
public:
  TransactionPlanBuilder(mlir::ModuleOp package, StateAnalysisBudget &budget,
                         const PackageInventory &inventory)
      : package(package), hardware(package), budget(budget),
        inventory(inventory) {}

  FailureOr<TransactionStatePlan> run(SourceTransactionOp transaction) {
    plan.transaction = transaction;
    plan.lexicalModule = transaction->getParentOfType<ModuleOp>();
    HardwareBindings rootBindings;
    rootBindings.owner = plan.lexicalModule;
    if (!budget.chargeBindings(transaction, rootBindings) ||
        !budget.charge(transaction, 2))
      return failure();
    plan.instances.push_back({plan.lexicalModule,
                              std::nullopt,
                              {},
                              ArrayAttr::get(package.getContext(), {}),
                              rootBindings});
    InvocationFrame root;
    root.transaction = transaction;
    root.anchor = 0;
    root.bindings = rootBindings;
    plan.frames.push_back(std::move(root));
    plan.rootFrame = 0;
    auto clock = value(transaction.getClk(), 0, 0);
    auto reset = value(transaction.getRst(), 0, 0);
    if (failed(clock) || failed(reset))
      return failure();
    auto domain = getDomain(*clock, *reset, transaction);
    if (failed(domain))
      return failure();
    plan.domain = *domain;
    SmallVector<BoundValueId> captures;
    if (!budget.charge(transaction, transaction.getCaptures().size()))
      return failure();
    for (Value capture : transaction.getCaptures()) {
      auto bound = value(capture, 0, 0);
      if (failed(bound))
        return failure();
      captures.push_back(*bound);
    }
    if (failed(bindArguments(0, transaction, {}, captures)) ||
        failed(event(TransactionStateEventKind::TransactionEnter, 0)))
      return failure();
    auto body = invocation(0, transaction, plan.domain, 0);
    if (failed(body))
      return failure();
    plan.guard = body->guard;
    if (failed(bindResults(transaction->getResults(), 0, 0, body->yielded,
                           plan.results)) ||
        failed(event(TransactionStateEventKind::TransactionExit, 0)) ||
        !budget.charge(transaction, candidates.size()))
      return failure();
    for (auto [owner, version] : llvm::enumerate(candidates))
      plan.finalVersions.push_back({owner, version});
    if (failed(chargeDump()))
      return failure();
    return std::move(plan);
  }

private:
  using ValueKey =
      std::pair<Value, std::pair<LexicalInstanceId, InvocationFrameId>>;
  using OwnerKey = std::pair<LexicalInstanceId, Operation *>;

  Operation *callable(InvocationFrameId frame) {
    auto method = plan.frames[frame].method;
    return method ? method.getOperation() : plan.transaction.getOperation();
  }

  FailureOr<BoundValueId> value(Value original, LexicalInstanceId anchor,
                                InvocationFrameId frame) {
    if (!budget.charge(plan.transaction))
      return failure();
    Operation *scope = original.getParentBlock()->getParentOp();
    Operation *function = nullptr;
    while (scope && !isa<ModuleOp>(scope)) {
      if (!budget.charge(plan.transaction))
        return failure();
      if (isa<SourceTransactionOp, SourceMethodOp>(scope))
        function = scope;
      scope = scope->getParentOp();
    }
    if (scope != plan.instances[anchor].definition.getOperation() ||
        (function && function != callable(frame)))
      return plan.transaction.emitOpError()
             << "transaction value has an unresolved lexical owner or "
                "invocation";
    if (!function)
      frame = plan.rootFrame;
    ValueKey key{original, {anchor, frame}};
    auto found = values.find(key);
    if (found != values.end())
      return found->second;
    if (!budget.charge(plan.transaction, 2))
      return failure();
    BoundValueId id = plan.values.size();
    plan.values.push_back({original, anchor, frame});
    values.try_emplace(key, id);
    return id;
  }

  LogicalResult alias(BoundValueId formal, BoundValueId actual,
                      Operation *site) {
    if (!budget.charge(site))
      return failure();
    auto [found, inserted] = aliases.try_emplace(formal, actual);
    if (!inserted && found->second != actual)
      return site->emitOpError() << "transaction SSA binding is ambiguous";
    return success();
  }

  FailureOr<BoundValueId> normalize(BoundValueId id, Operation *site) {
    llvm::DenseSet<BoundValueId> seen;
    for (unsigned depth = 0; depth <= 2 * maxDepth + 2; ++depth) {
      if (!budget.charge(site, 2))
        return failure();
      if (!seen.insert(id).second)
        return boundedError(site) << "cyclic transaction value binding";
      if (auto found = aliases.find(id); found != aliases.end()) {
        id = found->second;
        continue;
      }
      // Original module inputs are bound only by the actual Instance operand
      // ordinal. Instance outputs and ordinary computations remain opaque.
      BoundValueRef ref = plan.values[id];
      auto argument = dyn_cast<BlockArgument>(ref.value);
      if (!argument || argument.getOwner()->getParentOp() !=
                           plan.instances[ref.anchor].definition)
        return id;
      auto parent = plan.instances[ref.anchor].parent;
      if (!parent)
        return id;
      auto allocation = plan.instances[ref.anchor].allocation;
      unsigned ordinal = argument.getArgNumber();
      if (ordinal >= allocation.getNumOperands())
        return site->emitOpError()
               << "transaction instance input binding ordinal is invalid";
      auto actual =
          value(allocation.getOperand(ordinal), *parent, plan.rootFrame);
      if (failed(actual) || failed(equivalent(id, *actual, site)) ||
          failed(alias(id, *actual, site)))
        return failure();
      id = *actual;
    }
    return boundedError(site)
           << "transaction value binding depth budget exceeded";
  }

  FailureOr<Type> payloadType(BoundValueId id, Operation *site) {
    if (!budget.charge(site))
      return failure();
    auto ref = plan.values[id];
    return hardware.resolveType(ref.value.getType(),
                                plan.instances[ref.anchor].bindings, site);
  }

  LogicalResult equivalent(BoundValueId left, BoundValueId right,
                           Operation *site) {
    auto lhs = payloadType(left, site), rhs = payloadType(right, site);
    if (failed(lhs) || failed(rhs))
      return failure();
    if (!areEquivalentHardwareTypes(*lhs, *rhs))
      return site->emitOpError()
             << "transaction binding payload types disagree";
    return success();
  }

  FailureOr<BoolAttr> predicate(BoundValueId id, Operation *site) {
    llvm::DenseSet<BoundValueId> seen;
    for (unsigned depth = 0; depth <= maxDepth; ++depth) {
      auto bound = normalize(id, site);
      if (failed(bound) || !budget.charge(site, 2))
        return failure();
      if (!seen.insert(*bound).second)
        return boundedError(site) << "cyclic transaction predicate identity";
      auto ref = plan.values[*bound];
      auto input = detail::getSourcePredicateIdentityInput(ref.value, hardware);
      if (failed(input))
        return failure();
      if (!*input)
        // This helper has lexical bindings only, with no parameter-specialized
        // proof or expression evaluation. Domain normalize remains unchanged.
        return detail::getSourceLiteralPredicate(ref.value, hardware);
      auto forwarded = value(*input, ref.anchor, ref.frame);
      if (failed(forwarded))
        return failure();
      id = *forwarded;
    }
    return boundedError(site)
           << "transaction predicate identity depth budget exceeded";
  }

  LogicalResult truePath(BoundValueId id, Operation *site) {
    auto fact = predicate(id, site);
    if (failed(fact))
      return failure();
    if (!*fact || !(*fact).getValue())
      return site->emitOpError()
             << "transaction state analysis requires a proven-true effect path";
    return success();
  }

  FailureOr<TransactionDomain> getDomain(BoundValueId clock, BoundValueId reset,
                                         Operation *site) {
    auto clk = normalize(clock, site), rst = normalize(reset, site);
    if (failed(clk) || failed(rst))
      return failure();
    auto c = plan.values[*clk], r = plan.values[*rst];
    return TransactionDomain{{c.value, c.anchor}, {r.value, r.anchor}};
  }

  LogicalResult sameDomain(const TransactionDomain &domain, Operation *site) {
    if (!budget.charge(site))
      return failure();
    if (!(domain == plan.domain))
      return site->emitOpError() << "transaction state analysis cannot prove "
                                    "the same clock/reset domain";
    return success();
  }

  FailureOr<TransactionDomain> receiverDomain(LexicalInstanceId anchor,
                                              Operation *site) {
    auto definition = plan.instances[anchor].definition;
    auto domain = definition->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
    auto clock = domain ? domain.getAs<IntegerAttr>("clock") : IntegerAttr();
    auto reset = domain ? domain.getAs<IntegerAttr>("reset") : IntegerAttr();
    auto &block = definition.getBody().front();
    if (!domain || domain.size() != 2 || !clock || !reset ||
        !clock.getType().isSignlessInteger(64) ||
        !reset.getType().isSignlessInteger(64) ||
        clock.getValue().isNegative() || reset.getValue().isNegative() ||
        clock.getValue().getZExtValue() >= block.getNumArguments() ||
        reset.getValue().getZExtValue() >= block.getNumArguments())
      return site->emitOpError() << "transaction Method receiver requires "
                                    "validated ac.domain_inputs";
    auto clk = value(block.getArgument(clock.getInt()), anchor, plan.rootFrame);
    auto rst = value(block.getArgument(reset.getInt()), anchor, plan.rootFrame);
    if (failed(clk) || failed(rst))
      return failure();
    auto bound = getDomain(*clk, *rst, site);
    if (failed(bound) || failed(sameDomain(*bound, site)))
      return failure();
    return *bound;
  }

  LogicalResult bindArguments(InvocationFrameId frame, Operation *function,
                              ArrayRef<BoundValueId> explicitInputs,
                              ArrayRef<BoundValueId> captures) {
    for (unsigned region : {0u, 1u}) {
      auto arguments = function->getRegion(region).front().getArguments();
      if (arguments.size() != explicitInputs.size() + captures.size())
        return function->emitOpError()
               << "transaction callable binding arity is invalid";
      if (!budget.charge(function, arguments.size()))
        return failure();
      for (auto [ordinal, formal] : llvm::enumerate(arguments)) {
        BoundValueId actual = ordinal < explicitInputs.size()
                                  ? explicitInputs[ordinal]
                                  : captures[ordinal - explicitInputs.size()];
        auto bound = value(formal, plan.frames[frame].anchor, frame);
        if (failed(bound) || failed(equivalent(*bound, actual, function)) ||
            failed(alias(*bound, actual, function)))
          return failure();
        auto &bindings = region == 0 ? plan.frames[frame].guardArguments
                                     : plan.frames[frame].bodyArguments;
        bindings.push_back({formal, actual});
      }
    }
    return success();
  }

  template <typename Vector>
  LogicalResult bindResults(ValueRange originalResults,
                            LexicalInstanceId anchor, InvocationFrameId frame,
                            ArrayRef<BoundValueId> yielded, Vector &bindings) {
    if (originalResults.size() != yielded.size())
      return callable(frame)->emitOpError()
             << "transaction ordinary result binding arity is invalid";
    if (!budget.charge(callable(frame), originalResults.size()))
      return failure();
    for (auto [original, actual] : llvm::zip(originalResults, yielded)) {
      auto bound = value(original, anchor, frame);
      if (failed(bound) ||
          failed(equivalent(*bound, actual, callable(frame))) ||
          failed(alias(*bound, actual, callable(frame))))
        return failure();
      bindings.push_back({*bound, actual});
    }
    return success();
  }

  LogicalResult event(TransactionStateEventKind kind, InvocationFrameId frame,
                      std::optional<uint64_t> record = std::nullopt) {
    if (!budget.charge(callable(frame)))
      return failure();
    TransactionStateEvent next;
    next.kind = kind;
    next.frame = frame;
    switch (kind) {
    case TransactionStateEventKind::Read:
      next.read = record;
      break;
    case TransactionStateEventKind::Write:
      next.write = record;
      break;
    case TransactionStateEventKind::GuardEnter:
    case TransactionStateEventKind::GuardExit:
      next.guard = record;
      break;
    case TransactionStateEventKind::CallEnter:
    case TransactionStateEventKind::CallExit:
      next.call = record;
      break;
    case TransactionStateEventKind::TransactionEnter:
    case TransactionStateEventKind::TransactionExit:
      break;
    }
    plan.events.push_back(next);
    return success();
  }

  FailureOr<StateOwnerId> owner(Operation *effect, LexicalInstanceId anchor) {
    auto module = plan.instances[anchor].definition;
    auto name = effect->getAttrOfType<StringAttr>("state");
    SourceStateOp declaration;
    for (Operation &op : module.getBody().front()) {
      if (!budget.charge(effect))
        return failure();
      auto state = dyn_cast<SourceStateOp>(op);
      if (state && state->getAttrOfType<StringAttr>("instance_name") == name) {
        if (declaration)
          return effect->emitOpError()
                 << "transaction state owner is ambiguous";
        declaration = state;
      }
    }
    if (!declaration)
      return effect->emitOpError() << "transaction state owner is unresolved";
    auto key = OwnerKey{anchor, declaration};
    auto found = owners.find(key);
    if (found != owners.end())
      return found->second;
    auto q = value(declaration.getQ(), anchor, plan.rootFrame);
    auto clock = value(declaration.getClk(), anchor, plan.rootFrame);
    auto reset = value(declaration.getRst(), anchor, plan.rootFrame);
    if (failed(q) || failed(clock) || failed(reset))
      return failure();
    auto domain = getDomain(*clock, *reset, effect);
    auto type = payloadType(*q, effect);
    if (failed(domain) || failed(type) || failed(sameDomain(*domain, effect)) ||
        !budget.chargeOccurrence(effect, declaration.getOccurrence()) ||
        !budget.charge(effect, 6))
      return failure();
    StateOwnerId id = plan.owners.size();
    StateVersionId q0 = plan.versions.size();
    plan.owners.push_back({anchor, plan.instances[anchor].occurrencePath,
                           declaration, declaration.getOccurrence(), *type,
                           *domain, *q});
    plan.versions.push_back({plan.transaction, id, 0, StateVersionKind::OldQ,
                             std::nullopt, std::nullopt, *q, *type});
    owners.try_emplace(key, id);
    q0Versions.push_back(q0);
    candidates.push_back(q0);
    return id;
  }

  LogicalResult wholeOwner(Operation *operation, ArrayAttr selectors,
                           ValueRange indices) {
    if (!selectors.empty() || !indices.empty())
      return operation->emitOpError()
             << "transaction state analysis supports only whole-owner accesses";
    return success();
  }

  LogicalResult touch(SmallVectorImpl<StateOwnerId> &touched,
                      llvm::DenseSet<StateOwnerId> &seen, StateOwnerId id,
                      Operation *site) {
    if (!budget.charge(site, 2))
      return failure();
    if (seen.insert(id).second)
      touched.push_back(id);
    return success();
  }

  LogicalResult read(SourceStateReadOp operation, InvocationFrameId frame,
                     SmallVectorImpl<StateOwnerId> &touched,
                     llvm::DenseSet<StateOwnerId> &seen) {
    if (failed(wholeOwner(operation, operation.getSelectors(),
                          operation.getIndices())))
      return failure();
    auto anchor = plan.frames[frame].anchor;
    auto path = value(operation.getPath(), anchor, frame);
    if (failed(path) || failed(truePath(*path, operation)))
      return failure();
    auto state = owner(operation, anchor);
    auto result = value(operation.getValue(), anchor, frame);
    if (failed(state) || failed(result))
      return failure();
    auto type = payloadType(*result, operation);
    if (failed(type))
      return failure();
    if (!areEquivalentHardwareTypes(*type, plan.owners[*state].payloadType))
      return operation.emitOpError()
             << "whole-owner read payload type mismatch";
    if (!budget.chargeOccurrence(operation, operation.getOccurrence()) ||
        !budget.charge(operation))
      return failure();
    StateReadId id = plan.reads.size();
    plan.reads.push_back({operation,
                          frame,
                          *state,
                          candidates[*state],
                          *result,
                          *path,
                          operation.getSelectors(),
                          {},
                          operation.getOccurrence()});
    if (failed(touch(touched, seen, *state, operation)))
      return failure();
    return event(TransactionStateEventKind::Read, frame, id);
  }

  LogicalResult write(SourceStateWriteOp operation, InvocationFrameId frame,
                      SmallVectorImpl<StateOwnerId> &touched,
                      llvm::DenseSet<StateOwnerId> &seen) {
    if (failed(wholeOwner(operation, operation.getSelectors(),
                          operation.getIndices())))
      return failure();
    auto anchor = plan.frames[frame].anchor;
    auto path = value(operation.getPath(), anchor, frame);
    if (failed(path) || failed(truePath(*path, operation)))
      return failure();
    auto state = owner(operation, anchor);
    auto rhs = value(operation.getValue(), anchor, frame);
    if (failed(state) || failed(rhs))
      return failure();
    auto type = payloadType(*rhs, operation);
    if (failed(type))
      return failure();
    if (!areEquivalentHardwareTypes(*type, plan.owners[*state].payloadType))
      return operation.emitOpError()
             << "whole-owner write payload type mismatch";
    if (!budget.chargeOccurrence(operation, operation.getOccurrence()) ||
        !budget.charge(operation, 2))
      return failure();
    StateWriteId id = plan.writes.size();
    StateVersionId version = plan.versions.size();
    StateVersionId predecessor = candidates[*state];
    plan.writes.push_back({operation,
                           frame,
                           *state,
                           predecessor,
                           version,
                           *rhs,
                           *path,
                           operation.getSelectors(),
                           {},
                           operation.getOccurrence()});
    plan.versions.push_back({plan.transaction, *state,
                             plan.versions[predecessor].ordinal + 1,
                             StateVersionKind::Write, predecessor, id, *rhs,
                             plan.owners[*state].payloadType});
    candidates[*state] = version;
    if (failed(touch(touched, seen, *state, operation)))
      return failure();
    return event(TransactionStateEventKind::Write, frame, id);
  }

  FailureOr<LexicalInstanceId> receiver(InvocationFrameId frame,
                                        const ResolvedSourceMethod &resolved,
                                        SourceMethodCallOp call) {
    LexicalInstanceId anchor = plan.frames[frame].anchor;
    auto suffix = resolved.receiver.getAs<ArrayAttr>("instance_path");
    if (!budget.charge(call, suffix.size()))
      return failure();
    for (Attribute occurrence : suffix) {
      auto definition = plan.instances[anchor].definition;
      InstanceOp allocation;
      for (Operation &op : definition.getBody().front()) {
        if (!budget.charge(call))
          return failure();
        auto instance = dyn_cast<InstanceOp>(op);
        if (instance && instance.getOccurrence() == occurrence)
          allocation = instance;
      }
      if (!allocation)
        return call.emitOpError() << "transaction receiver requires an actual "
                                     "static Instance body";
      auto key = OwnerKey{anchor, allocation};
      auto found = instances.find(key);
      if (found != instances.end()) {
        anchor = found->second;
        continue;
      }
      Operation *target = hardware.lookupDefinition(allocation.getCallee());
      auto body = dyn_cast_or_null<ModuleOp>(target);
      if (!body)
        return call.emitOpError()
               << "transaction receiver requires an executable provider body";
      auto bound =
          hardware.bindInstance(allocation, plan.instances[anchor].bindings);
      if (failed(bound))
        return failure();
      auto parentPath = plan.instances[anchor].occurrencePath;
      if (parentPath.size() >= maxDepth)
        return boundedError(call)
               << "transaction lexical instance depth budget exceeded";
      if (!budget.charge(call, parentPath.size() + 3) ||
          !budget.chargeBindings(call, *bound) ||
          !budget.chargeOccurrence(call, allocation.getOccurrence()))
        return failure();
      SmallVector<Attribute> path(parentPath.begin(), parentPath.end());
      path.push_back(occurrence);
      LexicalInstanceId child = plan.instances.size();
      plan.instances.push_back({body, anchor, allocation,
                                ArrayAttr::get(package.getContext(), path),
                                *bound});
      instances.try_emplace(key, child);
      anchor = child;
    }
    if (plan.instances[anchor].definition != resolved.method.module)
      return call.emitOpError()
             << "transaction receiver body identity mismatch";
    return anchor;
  }

  LogicalResult snapshot(Operation *site,
                         SmallVectorImpl<StateVersionId> &saved) {
    if (!budget.charge(site, candidates.size()))
      return failure();
    saved.assign(candidates.begin(), candidates.end());
    return success();
  }

  LogicalResult entryVersions(Operation *site, ArrayRef<StateOwnerId> touched,
                              ArrayRef<StateVersionId> saved,
                              SmallVectorImpl<OwnerVersionBinding> &result) {
    if (!budget.charge(site, touched.size()))
      return failure();
    for (StateOwnerId id : touched)
      result.push_back({id, id < saved.size() ? saved[id] : q0Versions[id]});
    return success();
  }

  LogicalResult call(SourceMethodCallOp operation, InvocationFrameId parent,
                     unsigned depth, SmallVectorImpl<StateOwnerId> &touched,
                     llvm::DenseSet<StateOwnerId> &seen) {
    if (depth >= maxDepth)
      return boundedError(operation)
             << "transaction invocation depth budget exceeded";
    auto callerAnchor = plan.frames[parent].anchor;
    auto path = value(operation.getPath(), callerAnchor, parent);
    if (failed(path) || failed(truePath(*path, operation)))
      return failure();
    SmallVector<BoundValueId> actuals;
    if (!budget.charge(operation, operation.getArguments().size()))
      return failure();
    for (Value argument : operation.getArguments()) {
      auto actual = value(argument, callerAnchor, parent);
      if (failed(actual))
        return failure();
      actuals.push_back(*actual);
    }
    // Receiver/signature resolution depends on the original call and its
    // static instance bindings, not invocation payload values. Reuse only this
    // existing resolver result; frames, actuals, guards and effects stay fresh.
    auto resolutionKey = OwnerKey{callerAnchor, operation};
    auto cached = resolutions.find(resolutionKey);
    FailureOr<ResolvedSourceMethod> resolved = failure();
    if (cached != resolutions.end()) {
      if (!budget.chargeBindings(operation, cached->second.bindings))
        return failure();
      resolved = cached->second;
    } else {
      auto payModule = [&](Operation *module) {
        auto found = inventory.moduleCosts.find(module);
        return budget.charge(operation, found == inventory.moduleCosts.end()
                                            ? 1
                                            : found->second);
      };
      if (!payModule(plan.instances[callerAnchor].definition) ||
          !budget.charge(operation, inventory.methodCosts.lookup(
                                        operation.getCalleeAttr())))
        return failure();
      bool resolverBudgetValid = true;
      auto lookup = [&](FlatSymbolRefAttr symbol) -> Operation * {
        Operation *target = hardware.lookupDefinition(symbol.getValue());
        if (target && !payModule(target)) {
          resolverBudgetValid = false;
          // The primary bounded budget diagnostic has already been emitted.
          // Null stops the existing resolver before provider verification,
          // binding or descent; its missing-provider diagnostic is secondary.
          return nullptr;
        }
        return target;
      };
      resolved = resolveSourceMethodCall(operation,
                                         plan.frames[parent].bindings, lookup);
      if (!resolverBudgetValid || failed(resolved) ||
          !budget.chargeBindings(operation, resolved->bindings) ||
          !budget.charge(operation))
        return failure();
      resolutions.try_emplace(resolutionKey, *resolved);
    }
    auto method = resolved->method.definition;
    if (!method)
      return operation.emitOpError()
             << "transaction Method call requires an executable provider body";
    auto anchor = receiver(parent, *resolved, operation);
    if (failed(anchor))
      return failure();
    auto domain = receiverDomain(*anchor, operation);
    if (failed(domain))
      return failure();
    SmallVector<StateVersionId> saved;
    if (failed(snapshot(operation, saved)) ||
        !budget.chargeBindings(operation, resolved->bindings) ||
        !budget.chargeOccurrence(operation, operation.getOccurrence()) ||
        !budget.charge(operation,
                       2 + actuals.size() + method.getCaptures().size()))
      return failure();
    InvocationFrameId child = plan.frames.size();
    InvocationFrame next;
    next.transaction = plan.transaction;
    next.parent = parent;
    next.call = operation;
    next.method = method;
    next.callOccurrence = operation.getOccurrence();
    next.receiver = DictionaryAttr::get(
        package.getContext(),
        {NamedAttribute(StringAttr::get(package.getContext(), "instance_path"),
                        plan.instances[*anchor].occurrencePath)});
    next.anchor = *anchor;
    next.bindings = resolved->bindings;
    plan.frames.push_back(std::move(next));
    TransactionCallBinding binding;
    binding.operation = operation;
    binding.caller = parent;
    binding.callee = child;
    binding.path = *path;
    binding.arguments.assign(actuals.begin(), actuals.end());
    for (Value capture : method.getCaptures()) {
      auto bound = value(capture, *anchor, plan.rootFrame);
      if (failed(bound))
        return failure();
      binding.captures.push_back(*bound);
    }
    if (failed(bindArguments(child, method, actuals, binding.captures)))
      return failure();
    CallBindingId id = plan.calls.size();
    plan.calls.push_back(std::move(binding));
    if (failed(event(TransactionStateEventKind::CallEnter, parent, id)))
      return failure();
    auto body = invocation(child, method, *domain, depth + 1);
    if (failed(body))
      return failure();
    plan.calls[id].guard = body->guard;
    if (failed(entryVersions(operation, body->touched, saved,
                             plan.calls[id].entryVersions)) ||
        !budget.charge(operation, body->touched.size()))
      return failure();
    for (StateOwnerId owner : body->touched) {
      plan.calls[id].exitVersions.push_back({owner, candidates[owner]});
      if (failed(touch(touched, seen, owner, operation)))
        return failure();
    }
    if (failed(bindResults(operation.getResults(), callerAnchor, parent,
                           body->yielded, plan.calls[id].results)))
      return failure();
    return event(TransactionStateEventKind::CallExit, parent, id);
  }

  // Retain every ordinary payload edge, including unused results and nested
  // pure collection-region values. No payload result is executed here.
  LogicalResult payload(Operation *operation, InvocationFrameId frame) {
    SmallVector<Operation *> pending;
    if (!budget.charge(operation))
      return failure();
    pending.push_back(operation);
    while (!pending.empty()) {
      Operation *op = pending.pop_back_val();
      if (!budget.charge(op, 1 + op->getNumOperands() + op->getNumResults()))
        return failure();
      for (Value operand : op->getOperands())
        if (failed(value(operand, plan.frames[frame].anchor, frame)))
          return failure();
      for (Value result : op->getResults())
        if (failed(value(result, plan.frames[frame].anchor, frame)))
          return failure();
      for (Region &region : llvm::reverse(op->getRegions()))
        for (Block &block : llvm::reverse(region)) {
          if (!budget.charge(op, block.getNumArguments()))
            return failure();
          for (BlockArgument argument : block.getArguments())
            if (failed(value(argument, plan.frames[frame].anchor, frame)))
              return failure();
          for (Operation &nested : llvm::reverse(block)) {
            if (!budget.charge(op))
              return failure();
            pending.push_back(&nested);
          }
        }
    }
    return success();
  }

  LogicalResult region(Block &block, InvocationFrameId frame, unsigned depth,
                       SmallVectorImpl<StateOwnerId> &touched,
                       llvm::DenseSet<StateOwnerId> &seen) {
    for (Operation &op : block.without_terminator()) {
      if (!budget.charge(&op))
        return failure();
      if (auto access = dyn_cast<SourceStateReadOp>(op)) {
        if (failed(read(access, frame, touched, seen)))
          return failure();
      } else if (auto intent = dyn_cast<SourceStateWriteOp>(op)) {
        if (failed(write(intent, frame, touched, seen)))
          return failure();
      } else if (auto method = dyn_cast<SourceMethodCallOp>(op)) {
        if (failed(call(method, frame, depth, touched, seen)))
          return failure();
      } else if (failed(payload(&op, frame)))
        return failure();
    }
    return success();
  }

  FailureOr<InvocationResult> invocation(InvocationFrameId frame,
                                         Operation *function,
                                         const TransactionDomain &domain,
                                         unsigned depth) {
    if (!budget.charge(function) || failed(sameDomain(domain, function)))
      return failure();
    SmallVector<StateVersionId> saved;
    if (failed(snapshot(function, saved)) || !budget.charge(function))
      return failure();
    GuardBoundaryId guard = plan.guards.size();
    GuardBoundary boundary;
    boundary.callable = function;
    boundary.frame = frame;
    boundary.predecessor = previousGuard;
    boundary.domain = domain;
    plan.guards.push_back(std::move(boundary));
    previousGuard = guard;
    if (failed(event(TransactionStateEventKind::GuardEnter, frame, guard)))
      return failure();
    InvocationResult result;
    result.guard = guard;
    llvm::DenseSet<StateOwnerId> seen;
    StateReadId firstRead = plan.reads.size();
    auto &guardBlock = function->getRegion(0).front();
    if (failed(region(guardBlock, frame, depth, result.touched, seen)) ||
        failed(entryVersions(function, result.touched, saved,
                             plan.guards[guard].entryVersions)) ||
        !budget.charge(function, plan.reads.size() - firstRead))
      return failure();
    for (StateReadId read = firstRead; read != plan.reads.size(); ++read)
      plan.guards[guard].reads.push_back(read);
    auto guardYield = cast<YieldOp>(guardBlock.back());
    auto condition =
        value(guardYield.getValues().front(), plan.frames[frame].anchor, frame);
    if (failed(condition))
      return failure();
    auto fact = predicate(*condition, function);
    if (failed(fact))
      return failure();
    plan.guards[guard].predicate = *condition;
    plan.guards[guard].predicateFact = *fact;
    if (failed(event(TransactionStateEventKind::GuardExit, frame, guard)))
      return failure();
    auto &bodyBlock = function->getRegion(1).front();
    if (failed(region(bodyBlock, frame, depth, result.touched, seen)))
      return failure();
    auto yielded = cast<YieldOp>(bodyBlock.back()).getValues();
    if (!budget.charge(function, yielded.size()))
      return failure();
    for (Value original : yielded) {
      auto bound = value(original, plan.frames[frame].anchor, frame);
      if (failed(bound))
        return failure();
      result.yielded.push_back(*bound);
    }
    return result;
  }

  LogicalResult chargeDump() {
    // Dump traverses only these records. Charge every variable-length view now,
    // so printing a valid analysis cannot discover an exhaustion halfway
    // through.
    auto site = plan.transaction.getOperation();
    for (uint64_t count :
         {plan.instances.size(), plan.frames.size(), plan.values.size(),
          plan.owners.size(), plan.versions.size(), plan.reads.size(),
          plan.writes.size(), plan.guards.size(), plan.calls.size(),
          plan.events.size(), plan.finalVersions.size(), plan.results.size()})
      if (!budget.charge(site, count))
        return failure();
    if (!budget.chargeOccurrence(site, plan.transaction.getOccurrence()) ||
        !budget.charge(site, plan.lexicalModule.getSymName().size() +
                                 plan.transaction.getName().size()))
      return failure();
    for (const auto &instance : plan.instances) {
      auto definition = instance.definition;
      if (!budget.charge(site, instance.occurrencePath.size() +
                                   definition.getSymName().size()))
        return failure();
      for (Attribute occurrence : instance.occurrencePath)
        if (!budget.chargeOccurrence(site, cast<DictionaryAttr>(occurrence)))
          return failure();
    }
    for (const auto &frame : plan.frames) {
      auto method = frame.method;
      if (!budget.charge(site, frame.guardArguments.size() +
                                   frame.bodyArguments.size() +
                                   (method ? method.getSymName().size() : 0)))
        return failure();
    }
    for (const auto &guard : plan.guards)
      if (!budget.charge(site, guard.entryVersions.size() + guard.reads.size()))
        return failure();
    for (const auto &call : plan.calls)
      if (!budget.charge(site, call.arguments.size() + call.captures.size() +
                                   call.entryVersions.size() +
                                   call.exitVersions.size() +
                                   call.results.size()))
        return failure();
    return success();
  }

  mlir::ModuleOp package;
  HardwareAnalysis hardware;
  StateAnalysisBudget &budget;
  const PackageInventory &inventory;
  TransactionStatePlan plan;
  llvm::DenseMap<ValueKey, BoundValueId> values;
  llvm::DenseMap<BoundValueId, BoundValueId> aliases;
  llvm::DenseMap<OwnerKey, StateOwnerId> owners;
  llvm::DenseMap<OwnerKey, LexicalInstanceId> instances;
  llvm::DenseMap<OwnerKey, ResolvedSourceMethod> resolutions;
  SmallVector<StateVersionId, 0> candidates;
  SmallVector<StateVersionId, 0> q0Versions;
  std::optional<GuardBoundaryId> previousGuard;
};

StringRef eventName(TransactionStateEventKind kind) {
  switch (kind) {
  case TransactionStateEventKind::TransactionEnter:
    return "transaction-enter";
  case TransactionStateEventKind::GuardEnter:
    return "guard-enter";
  case TransactionStateEventKind::Read:
    return "read";
  case TransactionStateEventKind::GuardExit:
    return "guard-exit";
  case TransactionStateEventKind::Write:
    return "write";
  case TransactionStateEventKind::CallEnter:
    return "call-enter";
  case TransactionStateEventKind::CallExit:
    return "call-exit";
  case TransactionStateEventKind::TransactionExit:
    return "transaction-exit";
  }
  llvm_unreachable("unknown transaction state event kind");
}

void printVersions(llvm::raw_ostream &out,
                   ArrayRef<OwnerVersionBinding> versions) {
  out << '[';
  llvm::interleaveComma(versions, out, [&](const OwnerVersionBinding &binding) {
    out << 'o' << binding.owner << "=q" << binding.version;
  });
  out << ']';
}

void printResults(llvm::raw_ostream &out,
                  ArrayRef<TransactionResultBinding> results) {
  out << '[';
  llvm::interleaveComma(
      results, out, [&](const TransactionResultBinding &binding) {
        out << 'v' << binding.result << "=v" << binding.yielded;
      });
  out << ']';
}

void printPlan(llvm::raw_ostream &out, const TransactionStatePlan &plan) {
  auto transaction = plan.transaction;
  auto module = plan.lexicalModule;
  out << "transaction @" << module.getSymName() << "::" << transaction.getName()
      << " occurrence=" << transaction.getOccurrence() << " clock-anchor=i"
      << plan.domain.clock.anchor << " reset-anchor=i"
      << plan.domain.reset.anchor << '\n';
  for (auto [id, instance] : llvm::enumerate(plan.instances)) {
    auto definition = instance.definition;
    out << "  instance i" << id << " @" << definition.getSymName();
    if (instance.parent)
      out << " parent=i" << *instance.parent;
    out << " path=" << instance.occurrencePath << '\n';
  }
  for (auto [id, frame] : llvm::enumerate(plan.frames)) {
    auto method = frame.method;
    out << "  frame f" << id << " instance=i" << frame.anchor;
    if (frame.parent)
      out << " parent=f" << *frame.parent << " method=@" << method.getSymName()
          << " occurrence=" << frame.callOccurrence;
    else
      out << " root";
    for (auto [name, bindings] :
         {std::pair<StringRef, ArrayRef<TransactionValueBinding>>{
              "guard", frame.guardArguments},
          {"body", frame.bodyArguments}}) {
      out << ' ' << name << "-args=[";
      llvm::interleaveComma(
          bindings, out, [&](const TransactionValueBinding &b) {
            out << "arg" << b.formal.getArgNumber() << "=v" << b.actual;
          });
      out << ']';
    }
    out << '\n';
  }
  for (auto [id, ref] : llvm::enumerate(plan.values)) {
    out << "  value v" << id << " instance=i" << ref.anchor << " frame=f"
        << ref.frame;
    if (auto arg = dyn_cast<BlockArgument>(ref.value))
      out << " argument=" << arg.getArgNumber();
    else {
      auto result = cast<OpResult>(ref.value);
      out << " operation=" << result.getOwner()->getName().getStringRef()
          << " result=" << result.getResultNumber();
    }
    if (ref.value == plan.domain.clock.value &&
        ref.anchor == plan.domain.clock.anchor)
      out << " clock-endpoint";
    if (ref.value == plan.domain.reset.value &&
        ref.anchor == plan.domain.reset.anchor)
      out << " reset-endpoint";
    out << '\n';
  }
  for (auto [id, owner] : llvm::enumerate(plan.owners))
    out << "  owner o" << id << " instance=i" << owner.anchor << " old-Q=v"
        << owner.oldQ << " domain=transaction occurrence=" << owner.occurrence
        << '\n';
  for (auto [id, version] : llvm::enumerate(plan.versions)) {
    out << "  version q" << id << " owner=o" << version.owner
        << " ordinal=" << version.ordinal << " value=v" << version.value;
    if (version.kind == StateVersionKind::OldQ)
      out << " Q0";
    else
      out << " predecessor=q" << *version.predecessor << " write=w"
          << *version.write;
    out << '\n';
  }
  for (auto [id, read] : llvm::enumerate(plan.reads))
    out << "  read r" << id << " frame=f" << read.frame << " owner=o"
        << read.owner << " version=q" << read.version << " result=v"
        << read.result << " path=v" << read.path << '\n';
  for (auto [id, write] : llvm::enumerate(plan.writes))
    out << "  write w" << id << " frame=f" << write.frame << " owner=o"
        << write.owner << " predecessor=q" << write.predecessor << " version=q"
        << write.version << " value=v" << write.value << " path=v" << write.path
        << '\n';
  for (auto [id, guard] : llvm::enumerate(plan.guards)) {
    out << "  guard g" << id << " frame=f" << guard.frame
        << " domain=transaction predicate=v" << guard.predicate << " fact=";
    if (guard.predicateFact)
      out << (guard.predicateFact.getValue() ? "true" : "false");
    else
      out << "unknown";
    if (guard.predecessor)
      out << " predecessor=g" << *guard.predecessor;
    out << " entry=";
    printVersions(out, guard.entryVersions);
    out << " reads=[";
    llvm::interleaveComma(guard.reads, out,
                          [&](StateReadId read) { out << 'r' << read; });
    out << "]\n";
  }
  for (auto [id, call] : llvm::enumerate(plan.calls)) {
    out << "  call c" << id << " caller=f" << call.caller << " callee=f"
        << call.callee << " guard=g" << call.guard << " path=v" << call.path;
    for (auto [name, inputs] : {std::pair<StringRef, ArrayRef<BoundValueId>>{
                                    "arguments", call.arguments},
                                {"captures", call.captures}}) {
      out << ' ' << name << "=[";
      llvm::interleaveComma(inputs, out,
                            [&](BoundValueId input) { out << 'v' << input; });
      out << ']';
    }
    out << " entry=";
    printVersions(out, call.entryVersions);
    out << " exit=";
    printVersions(out, call.exitVersions);
    out << " results=";
    printResults(out, call.results);
    out << '\n';
  }
  for (auto [id, event] : llvm::enumerate(plan.events)) {
    out << "  event " << id << ' ' << eventName(event.kind) << " frame=f"
        << event.frame;
    if (event.read)
      out << " read=r" << *event.read;
    if (event.write)
      out << " write=w" << *event.write;
    if (event.guard)
      out << " guard=g" << *event.guard;
    if (event.call)
      out << " call=c" << *event.call;
    out << '\n';
  }
  out << "  final=";
  printVersions(out, plan.finalVersions);
  out << " results=";
  printResults(out, plan.results);
  out << '\n';
}

} // namespace

SourceTransactionStateAnalysis::SourceTransactionStateAnalysis(Operation *op) {
  auto package = dyn_cast_or_null<mlir::ModuleOp>(op);
  if (!package) {
    if (op)
      boundedError(op)
          << "transaction state analysis requires a builtin ModuleOp package";
    return;
  }
  StateAnalysisBudget budget;
  PackageInventory inventory;
  if (failed(preflight(package, budget, inventory)) ||
      failed(mlir::verify(package)))
    return;
  if (failed(priceResolver(package, budget, inventory)))
    return;
  SourceMethodSummariesAnalysis summaries(package);
  if (!summaries.isValid())
    return;
  llvm::DenseMap<Operation *, TransactionStatePlan> completed;
  SmallVector<Operation *> order;
  for (SourceTransactionOp transaction : inventory.transactions) {
    if (!budget.charge(transaction, 2))
      return;
    TransactionPlanBuilder builder(package, budget, inventory);
    auto plan = builder.run(transaction);
    if (failed(plan))
      return;
    completed.try_emplace(transaction, std::move(*plan));
    order.push_back(transaction);
  }
  plans = std::move(completed);
  transactionOrder = std::move(order);
  valid = true;
}

const TransactionStatePlan *
SourceTransactionStateAnalysis::lookup(SourceTransactionOp transaction) const {
  if (!valid || !transaction)
    return nullptr;
  auto found = plans.find(transaction);
  return found == plans.end() ? nullptr : &found->second;
}

LogicalResult
SourceTransactionStateAnalysis::print(llvm::raw_ostream &out) const {
  if (!valid)
    return failure();
  for (Operation *transaction : transactionOrder)
    printPlan(out, plans.find(transaction)->second);
  return success();
}

} // namespace acir::ac

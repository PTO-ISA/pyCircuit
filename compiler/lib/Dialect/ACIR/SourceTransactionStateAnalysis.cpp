#include "pycircuit/Dialect/ACIR/SourceTransactionStateAnalysis.h"

#include "ACIRSourceContracts.h"
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
#include <limits>

using namespace mlir;

namespace acir::ac {
namespace {

constexpr unsigned maxDepth = 128;

uint64_t saturatedMultiply(uint64_t left, uint64_t right) {
  constexpr uint64_t maximum = std::numeric_limits<uint64_t>::max();
  return right && left > maximum / right ? maximum : left * right;
}

uint64_t saturatedAdd(uint64_t left, uint64_t right) {
  constexpr uint64_t maximum = std::numeric_limits<uint64_t>::max();
  return right > maximum - left ? maximum : left + right;
}

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

  template <typename T> bool chargeStorage(Operation *site, uint64_t count) {
    // A(n,T) = n * (1 + ceil(sizeof(T)/8)), saturating before allocation.
    constexpr uint64_t units = 1 + (sizeof(T) + 7) / 8;
    return charge(site, saturatedMultiply(count, units));
  }

  template <typename Vector>
  bool reserve(Operation *site, Vector &storage, uint64_t additional) {
    if (additional > storage.max_size() - storage.size())
      return charge(site, std::numeric_limits<uint64_t>::max());
    uint64_t count = storage.size() + additional;
    if (count <= storage.capacity())
      return true;
    // Reserve the full growth explicitly. Per-record A fees already paid the
    // logical new slots; charge only spare capacity bytes and retained moves.
    uint64_t capacity = std::max(
        count,
        std::min(uint64_t(storage.max_size()),
                 saturatedAdd(saturatedMultiply(storage.capacity(), 2), 1)));
    uint64_t spare = capacity - storage.capacity();
    spare = spare > additional ? spare - additional : 0;
    uint64_t bytes =
        saturatedMultiply(spare, sizeof(typename Vector::value_type));
    uint64_t fee = saturatedAdd(storage.size(), bytes / 8 + (bytes % 8 != 0));
    if (!charge(site, fee))
      return false;
    storage.reserve(capacity);
    return true;
  }

  template <typename Map> bool reserveMapInsert(Operation *site, Map &storage) {
    // DenseMap in the pinned LLVM toolchain reserves power-of-two bucket
    // arrays, at least 64, keeping load strictly below 3/4. Pay bucket storage
    // and every retained bucket visited by growth before requesting it.
    using Bucket = typename Map::value_type;
    uint64_t count = uint64_t(storage.size()) + 1;
    uint64_t oldCapacity = storage.getMemorySize() / sizeof(Bucket);
    uint64_t capacity = std::max(uint64_t(64), oldCapacity);
    while (saturatedMultiply(count, 4) >= saturatedMultiply(capacity, 3)) {
      if (!charge(site))
        return false;
      capacity = saturatedMultiply(capacity, 2);
    }
    if (count > std::numeric_limits<unsigned>::max() || !charge(site) ||
        (capacity > oldCapacity &&
         (!chargeStorage<Bucket>(site, capacity - oldCapacity) ||
          !charge(site, oldCapacity))))
      return false;
    storage.reserve(count);
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

  // Borrowed attributes are printed in the stable plan dump. Prepay their
  // generic traversal and spelling before copying them into expanded records.
  bool chargeAttribute(Operation *site, Attribute root) {
    SmallVector<Attribute> pending;
    if (!root || !charge(site))
      return false;
    pending.push_back(root);
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

  bool chargeOccurrence(Operation *site, DictionaryAttr occurrence) {
    return chargeAttribute(site, occurrence);
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
        inventory(inventory), subelements(budget) {}

  FailureOr<TransactionStatePlan> run(SourceTransactionOp transaction) {
    plan.transaction = transaction;
    plan.lexicalModule = transaction->getParentOfType<ModuleOp>();
    HardwareBindings rootBindings;
    rootBindings.owner = plan.lexicalModule;
    if (!budget.chargeBindings(transaction, rootBindings) ||
        !budget.chargeStorage<TransactionLexicalInstance>(transaction, 1) ||
        !budget.reserve(transaction, plan.instances, 1) ||
        !budget.chargeStorage<InvocationFrame>(transaction, 1) ||
        !budget.reserve(transaction, plan.frames, 1))
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
    auto clock = useValue(transaction.getClk(), 0, 0, transaction);
    auto reset = useValue(transaction.getRst(), 0, 0, transaction);
    if (failed(clock) || failed(reset))
      return failure();
    auto domain = getDomain(*clock, *reset, transaction);
    if (failed(domain))
      return failure();
    plan.domain = *domain;
    plan.clockValue = *clock;
    plan.resetValue = *reset;
    SmallVector<BoundValueId> captures;
    if (!budget.chargeStorage<BoundValueId>(transaction,
                                            transaction.getCaptures().size()) ||
        !budget.reserve(transaction, captures,
                        transaction.getCaptures().size()))
      return failure();
    for (Value capture : transaction.getCaptures()) {
      auto bound = useValue(capture, 0, 0, transaction);
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
        !budget.chargeStorage<OwnerVersionBinding>(transaction,
                                                   candidates.size()) ||
        !budget.reserve(transaction, plan.finalVersions, candidates.size()))
      return failure();
    for (auto [owner, version] : llvm::enumerate(candidates))
      plan.finalVersions.push_back({owner, version});
    if (failed(verifyPlan()) || failed(chargeDump()))
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
    if (!budget.chargeStorage<BoundValueRef>(plan.transaction, 1) ||
        !budget.reserve(plan.transaction, plan.values, 1) ||
        !budget.chargeStorage<uint8_t>(plan.transaction, 2) ||
        !budget.reserve(plan.transaction, validityAssigned, 1) ||
        !budget.reserve(plan.transaction, lexicalValues, 1) ||
        !budget.reserveMapInsert(plan.transaction, values))
      return failure();
    BoundValueId id = plan.values.size();
    plan.values.push_back({original, anchor, frame});
    validityAssigned.push_back(false);
    lexicalValues.push_back(!function && original.getDefiningOp() !=
                                             plan.transaction.getOperation());
    values.try_emplace(key, id);
    return id;
  }

  static bool samePath(TransactionSourcePath left,
                       TransactionSourcePath right) {
    return left.branch == right.branch && left.whenTrue == right.whenTrue;
  }

  static bool samePoint(TransactionPoint left, TransactionPoint right) {
    return left.prefix == right.prefix && samePath(left.path, right.path);
  }

  TransactionPoint point() const { return {currentPrefix, currentPath}; }

  FailureOr<bool> sourcePathDominates(TransactionSourcePath definition,
                                      TransactionSourcePath use,
                                      Operation *site) {
    for (unsigned depth = 0; depth <= maxDepth; ++depth) {
      if (!budget.charge(site))
        return failure();
      if ((!definition.branch && definition.whenTrue) ||
          (!use.branch && use.whenTrue) ||
          (definition.branch && *definition.branch >= plan.branches.size()) ||
          (use.branch && *use.branch >= plan.branches.size()))
        return site->emitOpError() << "invalid transaction source path";
      if (samePath(definition, use))
        return true;
      if (!use.branch)
        return false;
      const auto &branch = plan.branches[*use.branch];
      if (branch.entry.path.branch && *branch.entry.path.branch >= *use.branch)
        return site->emitOpError() << "cyclic transaction source path";
      use = branch.entry.path;
    }
    return boundedError(site) << "transaction source path depth exceeded";
  }

  // Interning establishes identity. Only the actual definition or an
  // authenticated binding establishes availability.
  LogicalResult assignValidity(BoundValueId id, TransactionPoint validity,
                               Operation *site) {
    if (!budget.charge(site) || id >= plan.values.size())
      return failure();
    auto legal = sourcePathDominates(validity.path, validity.path, site);
    if (failed(legal))
      return failure();
    if (validity.prefix > currentPrefix)
      return site->emitOpError()
             << "transaction value requires a future control";
    if (validityAssigned[id] && !samePoint(plan.values[id].validity, validity))
      return site->emitOpError() << "transaction value validity conflicts";
    plan.values[id].validity = validity;
    validityAssigned[id] = true;
    return success();
  }

  LogicalResult requireAvailable(BoundValueId id, TransactionPoint use,
                                 Operation *site) {
    if (!budget.charge(site) || id >= plan.values.size())
      return failure();
    if (!validityAssigned[id])
      return site->emitOpError() << "transaction value has no validity point";
    const auto &ref = plan.values[id];
    auto dominates = sourcePathDominates(ref.validity.path, use.path, site);
    if (failed(dominates))
      return failure();
    if (!*dominates || ref.validity.prefix > use.prefix)
      return site->emitOpError()
             << "transaction operand is unavailable on this path";
    return success();
  }

  LogicalResult requireVersionAvailable(StateVersionId id, TransactionPoint use,
                                        Operation *site) {
    if (!budget.charge(site) || id >= plan.versions.size())
      return failure();
    auto validity = plan.versions[id].validity;
    auto dominates = sourcePathDominates(validity.path, use.path, site);
    if (failed(dominates))
      return failure();
    if (!*dominates || validity.prefix > use.prefix)
      return site->emitOpError()
             << "state candidate is unavailable on this path";
    return success();
  }

  LogicalResult available(BoundValueId id, Operation *site) {
    return requireAvailable(id, point(), site);
  }

  FailureOr<BoundValueId> useValue(Value original, LexicalInstanceId anchor,
                                   InvocationFrameId frame, Operation *site) {
    auto id = value(original, anchor, frame);
    if (failed(id))
      return failure();
    // Other verified transaction outputs are lawful lexical live-ins. This
    // transaction's own outputs acquire its exit prefix in bindResults.
    if (lexicalValues[*id] && failed(assignValidity(*id, {}, site)))
      return failure();
    if (failed(available(*id, site)))
      return failure();
    return *id;
  }

  LogicalResult alias(BoundValueId formal, BoundValueId actual,
                      Operation *site) {
    if (!budget.reserveMapInsert(site, aliases))
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
      const BoundValueRef &ref = plan.values[id];
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
      auto actual = useValue(allocation.getOperand(ordinal), *parent,
                             plan.rootFrame, site);
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
    const auto &ref = plan.values[id];
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
      if (failed(available(*bound, site)))
        return failure();
      if (auto result = resultBindings.find(*bound);
          result != resultBindings.end()) {
        id = result->second;
        continue;
      }
      const auto &ref = plan.values[*bound];
      auto input = detail::getSourcePredicateIdentityInput(ref.value, hardware);
      if (failed(input))
        return failure();
      if (!*input)
        // This helper has lexical bindings only, with no parameter-specialized
        // proof or expression evaluation. Domain normalize remains unchanged.
        return detail::getSourceLiteralPredicate(ref.value, hardware);
      auto forwarded = useValue(*input, ref.anchor, ref.frame, site);
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
    const auto &c = plan.values[*clk], &r = plan.values[*rst];
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
    auto clk = useValue(block.getArgument(clock.getInt()), anchor,
                        plan.rootFrame, site);
    auto rst = useValue(block.getArgument(reset.getInt()), anchor,
                        plan.rootFrame, site);
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
      auto &bindings = region == 0 ? plan.frames[frame].guardArguments
                                   : plan.frames[frame].bodyArguments;
      if (!budget.chargeStorage<TransactionValueBinding>(function,
                                                         arguments.size()) ||
          !budget.reserve(function, bindings, arguments.size()))
        return failure();
      for (auto [ordinal, formal] : llvm::enumerate(arguments)) {
        BoundValueId actual = ordinal < explicitInputs.size()
                                  ? explicitInputs[ordinal]
                                  : captures[ordinal - explicitInputs.size()];
        auto bound = value(formal, plan.frames[frame].anchor, frame);
        if (failed(bound) ||
            failed(
                requireAvailable(actual, plan.frames[frame].entry, function)) ||
            failed(equivalent(*bound, actual, function)) ||
            failed(alias(*bound, actual, function)) ||
            failed(
                assignValidity(*bound, plan.values[actual].validity, function)))
          return failure();
        if (formal.getOwner()->getParentOp() != function ||
            plan.values[*bound].frame != frame ||
            plan.values[*bound].anchor != plan.frames[frame].anchor)
          return function->emitOpError()
                 << "callable formal belongs to another frame";
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
    if (!budget.chargeStorage<TransactionResultBinding>(
            callable(frame), originalResults.size()) ||
        !budget.reserve(callable(frame), bindings, originalResults.size()))
      return failure();
    for (auto [original, actual] : llvm::zip(originalResults, yielded)) {
      auto bound = value(original, anchor, frame);
      if (failed(bound) || failed(available(actual, callable(frame))) ||
          failed(equivalent(*bound, actual, callable(frame))) ||
          failed(assignValidity(*bound, point(), callable(frame))) ||
          !budget.reserveMapInsert(callable(frame), resultBindings))
        return failure();
      if (!resultBindings.try_emplace(*bound, actual).second)
        return callable(frame)->emitOpError()
               << "duplicate transaction result binding";
      bindings.push_back({*bound, actual});
    }
    return success();
  }

  LogicalResult event(TransactionStateEventKind kind, InvocationFrameId frame,
                      std::optional<uint64_t> record = std::nullopt) {
    if (!budget.charge(callable(frame), 2) ||
        !budget.chargeStorage<TransactionStateEvent>(callable(frame), 1) ||
        !budget.reserve(callable(frame), plan.events, 1))
      return failure();
    TransactionStateEvent next;
    next.kind = kind;
    next.frame = frame;
    next.point = point();
    switch (kind) {
    case TransactionStateEventKind::Read:
      next.read = record;
      break;
    case TransactionStateEventKind::Write:
      next.write = record;
      break;
    case TransactionStateEventKind::BodyCheck:
      next.bodyCheck = record;
      break;
    case TransactionStateEventKind::Observe:
      next.observation = record;
      break;
    case TransactionStateEventKind::GuardEnter:
    case TransactionStateEventKind::GuardExit:
      next.guard = record;
      break;
    case TransactionStateEventKind::SelectionEnter:
    case TransactionStateEventKind::SelectionExit:
      next.selection = record;
      break;
    case TransactionStateEventKind::CallEnter:
    case TransactionStateEventKind::CallExit:
      next.call = record;
      break;
    case TransactionStateEventKind::BranchEnter:
    case TransactionStateEventKind::ThenEnter:
    case TransactionStateEventKind::ThenExit:
    case TransactionStateEventKind::ElseEnter:
    case TransactionStateEventKind::ElseExit:
    case TransactionStateEventKind::BranchExit:
      next.branch = record;
      break;
    case TransactionStateEventKind::StateJoin:
      next.join = record;
      break;
    case TransactionStateEventKind::BranchResult:
      next.branchResult = record;
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
    auto q = useValue(declaration.getQ(), anchor, plan.rootFrame, effect);
    auto clock = useValue(declaration.getClk(), anchor, plan.rootFrame, effect);
    auto reset = useValue(declaration.getRst(), anchor, plan.rootFrame, effect);
    if (failed(q) || failed(clock) || failed(reset))
      return failure();
    auto domain = getDomain(*clock, *reset, effect);
    auto type = payloadType(*q, effect);
    if (failed(domain) || failed(type) || failed(sameDomain(*domain, effect)) ||
        !budget.chargeOccurrence(effect, declaration.getOccurrence()) ||
        !budget.chargeStorage<TransactionStateOwner>(effect, 1) ||
        !budget.reserve(effect, plan.owners, 1) ||
        !budget.chargeStorage<StateVersion>(effect, 1) ||
        !budget.reserve(effect, plan.versions, 1) ||
        !budget.reserveMapInsert(effect, owners) ||
        !budget.chargeStorage<StateVersionId>(effect, 2) ||
        !budget.reserve(effect, q0Versions, 1) ||
        !budget.reserve(effect, candidates, 1) ||
        !budget.chargeStorage<uint64_t>(effect, 1) ||
        !budget.reserve(effect, ownerOrdinals, 1))
      return failure();
    StateOwnerId id = plan.owners.size();
    StateVersionId q0 = plan.versions.size();
    plan.owners.push_back({anchor, plan.instances[anchor].occurrencePath,
                           declaration, declaration.getOccurrence(), *type,
                           *domain, *q});
    plan.versions.push_back({plan.transaction,
                             id,
                             0,
                             StateVersionKind::OldQ,
                             std::nullopt,
                             std::nullopt,
                             std::nullopt,
                             *q,
                             *type,
                             {}});
    owners.try_emplace(key, id);
    q0Versions.push_back(q0);
    candidates.push_back(q0);
    ownerOrdinals.push_back(0);
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
    if (!seen.contains(id)) {
      if (!budget.chargeStorage<StateOwnerId>(site, 2) ||
          !budget.reserve(site, touched, 1))
        return failure();
      seen.insert(id);
      touched.push_back(id);
    }
    return success();
  }

  LogicalResult read(SourceStateReadOp operation, InvocationFrameId frame,
                     SmallVectorImpl<StateOwnerId> &touched,
                     llvm::DenseSet<StateOwnerId> &seen) {
    if (failed(wholeOwner(operation, operation.getSelectors(),
                          operation.getIndices())))
      return failure();
    auto anchor = plan.frames[frame].anchor;
    auto path = useValue(operation.getPath(), anchor, frame, operation);
    if (failed(path) || failed(truePath(*path, operation)))
      return failure();
    auto state = owner(operation, anchor);
    auto result = value(operation.getValue(), anchor, frame);
    if (failed(state) || failed(result))
      return failure();
    auto selected = candidates[*state];
    if (failed(requireVersionAvailable(selected, point(), operation)) ||
        failed(assignValidity(*result, point(), operation)))
      return failure();
    auto type = payloadType(*result, operation);
    if (failed(type))
      return failure();
    if (!areEquivalentHardwareTypes(*type, plan.owners[*state].payloadType))
      return operation.emitOpError()
             << "whole-owner read payload type mismatch";
    if (!budget.chargeOccurrence(operation, operation.getOccurrence()) ||
        !budget.chargeStorage<StateReadBinding>(operation, 1) ||
        !budget.reserve(operation, plan.reads, 1))
      return failure();
    StateReadId id = plan.reads.size();
    plan.reads.push_back({operation,
                          frame,
                          *state,
                          selected,
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
    auto path = useValue(operation.getPath(), anchor, frame, operation);
    if (failed(path) || failed(truePath(*path, operation)))
      return failure();
    auto state = owner(operation, anchor);
    auto rhs = useValue(operation.getValue(), anchor, frame, operation);
    if (failed(state) || failed(rhs))
      return failure();
    auto type = payloadType(*rhs, operation);
    if (failed(type))
      return failure();
    if (!areEquivalentHardwareTypes(*type, plan.owners[*state].payloadType))
      return operation.emitOpError()
             << "whole-owner write payload type mismatch";
    if (!budget.chargeOccurrence(operation, operation.getOccurrence()) ||
        !budget.chargeStorage<StateWriteIntent>(operation, 1) ||
        !budget.reserve(operation, plan.writes, 1) ||
        !budget.chargeStorage<StateVersion>(operation, 1) ||
        !budget.reserve(operation, plan.versions, 1))
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
    plan.versions.push_back({plan.transaction, *state, ++ownerOrdinals[*state],
                             StateVersionKind::Write, predecessor, id,
                             std::nullopt, *rhs,
                             plan.owners[*state].payloadType, point()});
    candidates[*state] = version;
    if (failed(touch(touched, seen, *state, operation)))
      return failure();
    return event(TransactionStateEventKind::Write, frame, id);
  }

  LogicalResult bodyCheck(SourceExpectOp operation, InvocationFrameId frame) {
    auto anchor = plan.frames[frame].anchor;
    auto condition =
        useValue(operation.getCondition(), anchor, frame, operation);
    auto path = useValue(operation.getPath(), anchor, frame, operation);
    if (failed(condition) || failed(path) || failed(truePath(*path, operation)))
      return failure();
    if (!subelements.check(operation, operation.getCondition().getType()) ||
        !subelements.check(operation, operation.getPath().getType()))
      return failure();
    auto occurrence = operation->getAttrOfType<DictionaryAttr>("occurrence");
    if (!budget.charge(operation, 8) ||
        !budget.chargeOccurrence(operation, occurrence) ||
        !budget.chargeAttribute(operation, operation.getLocationAttr()) ||
        (operation->hasAttr("ac.message") &&
         !budget.chargeAttribute(operation, operation->getAttr("ac.message"))))
      return failure();
    if (!budget.chargeStorage<TransactionBodyCheck>(operation, 1) ||
        !budget.reserve(operation, plan.bodyChecks, 1))
      return failure();
    BodyCheckId id = plan.bodyChecks.size();
    plan.bodyChecks.push_back(
        {operation, frame, *condition, *path, occurrence,
         operation.getLocationAttr(),
         operation->getAttrOfType<StringAttr>("ac.message"), point()});
    return event(TransactionStateEventKind::BodyCheck, frame, id);
  }

  LogicalResult observe(SourceObserveOp operation, InvocationFrameId frame) {
    auto anchor = plan.frames[frame].anchor;
    auto path = useValue(operation.getPath(), anchor, frame, operation);
    if (failed(path) || failed(truePath(*path, operation)))
      return failure();
    if (!subelements.check(operation, operation.getPath().getType()))
      return failure();
    SmallVector<BoundValueId, 0> values;
    if (!budget.charge(operation, 8) ||
        !budget.chargeStorage<BoundValueId>(operation,
                                            operation.getValues().size()) ||
        !budget.reserve(operation, values, operation.getValues().size()))
      return failure();
    for (Value original : operation.getValues()) {
      auto bound = useValue(original, anchor, frame, operation);
      if (failed(bound) || !subelements.check(operation, original.getType()))
        return failure();
      values.push_back(*bound);
    }
    auto occurrence = operation->getAttrOfType<DictionaryAttr>("occurrence");
    if (!budget.chargeOccurrence(operation, occurrence) ||
        !budget.chargeAttribute(operation, operation->getAttr("ac.location")) ||
        !budget.chargeAttribute(operation, operation.getKindAttr()) ||
        !budget.chargeAttribute(operation, operation.getSpec()))
      return failure();
    if (!budget.charge(operation))
      return failure();
    if (!budget.chargeStorage<TransactionObservation>(operation, 1) ||
        !budget.reserve(operation, plan.observations, 1))
      return failure();
    ObservationId id = plan.observations.size();
    plan.observations.push_back(
        {operation, frame, *path, std::move(values), occurrence,
         operation->getAttrOfType<DictionaryAttr>("ac.location"),
         operation.getKindAttr(), operation.getSpec(), point()});
    return event(TransactionStateEventKind::Observe, frame, id);
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
          !budget.chargeOccurrence(call, allocation.getOccurrence()) ||
          !budget.chargeStorage<TransactionLexicalInstance>(call, 1) ||
          !budget.reserve(call, plan.instances, 1) ||
          !budget.reserveMapInsert(call, instances))
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
    if (!budget.chargeStorage<StateVersionId>(site, candidates.size()) ||
        !budget.reserve(site, saved, candidates.size()))
      return failure();
    saved.assign(candidates.begin(), candidates.end());
    return success();
  }

  LogicalResult entryVersions(Operation *site, ArrayRef<StateOwnerId> touched,
                              ArrayRef<StateVersionId> saved,
                              SmallVectorImpl<OwnerVersionBinding> &result) {
    if (!budget.chargeStorage<OwnerVersionBinding>(site, touched.size()) ||
        !budget.reserve(site, result, touched.size()))
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
    auto path = useValue(operation.getPath(), callerAnchor, parent, operation);
    if (failed(path) || failed(truePath(*path, operation)))
      return failure();
    SmallVector<BoundValueId> actuals;
    if (!budget.chargeStorage<BoundValueId>(operation,
                                            operation.getArguments().size()) ||
        !budget.reserve(operation, actuals, operation.getArguments().size()))
      return failure();
    for (Value argument : operation.getArguments()) {
      auto actual = useValue(argument, callerAnchor, parent, operation);
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
          !budget.reserveMapInsert(operation, resolutions))
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
        !budget.chargeStorage<InvocationFrame>(operation, 1) ||
        !budget.reserve(operation, plan.frames, 1) ||
        !budget.chargeStorage<TransactionCallBinding>(operation, 1) ||
        !budget.reserve(operation, plan.calls, 1) ||
        !budget.chargeStorage<BoundValueId>(
            operation, actuals.size() + method.getCaptures().size()))
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
    next.entry = point();
    plan.frames.push_back(std::move(next));
    TransactionCallBinding binding;
    binding.operation = operation;
    binding.caller = parent;
    binding.callee = child;
    binding.authoredPath = *path;
    if (!budget.reserve(operation, binding.arguments, actuals.size()) ||
        !budget.reserve(operation, binding.captures,
                        method.getCaptures().size()))
      return failure();
    binding.arguments.assign(actuals.begin(), actuals.end());
    for (Value capture : method.getCaptures()) {
      auto bound = useValue(capture, *anchor, plan.rootFrame, operation);
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
    if (failed(entryVersions(operation, body->touched, saved,
                             plan.calls[id].entryVersions)) ||
        !budget.chargeStorage<OwnerVersionBinding>(operation,
                                                   body->touched.size()) ||
        !budget.reserve(operation, plan.calls[id].exitVersions,
                        body->touched.size()))
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
      if (!isa<YieldOp>(op) && !detail::isCombinationalHardwareOperation(op))
        return op->emitOpError()
               << "transaction state plan cannot omit an unrepresented "
                  "payload effect";

      for (Value operand : op->getOperands()) {
        auto bound = useValue(operand, plan.frames[frame].anchor, frame, op);
        if (failed(bound))
          return failure();
      }
      for (Value result : op->getResults()) {
        auto bound = value(result, plan.frames[frame].anchor, frame);
        if (failed(bound) || failed(assignValidity(*bound, point(), op)))
          return failure();
      }
      for (Region &region : llvm::reverse(op->getRegions()))
        for (Block &block : llvm::reverse(region)) {
          if (!budget.charge(op, block.getNumArguments()))
            return failure();
          // Pure region arguments inherit the operation's entry and inputs,
          // including region-generated index arguments with no actual operand.
          for (BlockArgument argument : block.getArguments()) {
            auto bound = value(argument, plan.frames[frame].anchor, frame);
            if (failed(bound) || failed(assignValidity(*bound, point(), op)))
              return failure();
          }
          for (Operation &nested : llvm::reverse(block)) {
            if (!budget.charge(op))
              return failure();
            pending.push_back(&nested);
          }
        }
    }
    return success();
  }

  LogicalResult restore(Operation *site, ArrayRef<StateVersionId> entry) {
    // Owners discovered in either arm have one transaction-local Q0. A missing
    // entry is never another arm's final candidate.
    for (auto [id, candidate] : llvm::enumerate(candidates)) {
      if (!budget.charge(site))
        return failure();
      candidate = id < entry.size() ? entry[id] : q0Versions[id];
    }
    return success();
  }

  LogicalResult armResults(SourceBranchOp operation, Block &block,
                           InvocationFrameId frame,
                           SmallVectorImpl<BoundValueId> &result) {
    auto yielded = cast<YieldOp>(block.back()).getValues();
    if (yielded.size() != operation.getNumResults())
      return operation.emitOpError() << "branch result arity is invalid";
    if (!budget.chargeStorage<BoundValueId>(operation, yielded.size()) ||
        !budget.reserve(operation, result, yielded.size()))
      return failure();
    for (Value original : yielded) {
      if (!budget.charge(operation))
        return failure();
      auto bound = useValue(original, plan.frames[frame].anchor, frame,
                            block.getTerminator());
      if (failed(bound))
        return failure();
      result.push_back(*bound);
    }
    return success();
  }

  FailureOr<bool> needsSelection(SourceBranchOp branch) {
    SmallVector<Operation *, 0> pending;
    auto append = [&](Operation *op) {
      if (!budget.chargeStorage<Operation *>(branch, 1) ||
          !budget.reserve(branch, pending, 1))
        return false;
      pending.push_back(op);
      return true;
    };
    if (!append(branch))
      return failure();
    bool effectful = false;
    while (!pending.empty()) {
      Operation *op = pending.pop_back_val();
      if (!budget.charge(branch))
        return failure();
      effectful |= isa<SourceMethodCallOp, SourceExpectOp, SourceObserveOp>(op);
      for (Region &region : op->getRegions())
        for (Block &block : region)
          for (Operation &child : block)
            if (!append(&child))
              return failure();
    }
    return effectful;
  }

  LogicalResult branch(SourceBranchOp operation, InvocationFrameId frame,
                       unsigned depth, SmallVectorImpl<StateOwnerId> &touched,
                       llvm::DenseSet<StateOwnerId> &seen) {
    if (depth >= maxDepth)
      return boundedError(operation)
             << "transaction branch depth budget exceeded";
    auto anchor = plan.frames[frame].anchor;
    auto condition =
        useValue(operation.getCondition(), anchor, frame, operation);
    if (failed(condition))
      return failure();
    auto conditionType = payloadType(*condition, operation);
    if (failed(conditionType) || !isa<BitsType>(*conditionType))
      return operation.emitOpError()
             << "branch condition requires width-one Bits";
    auto width = hardware.getPackedWidth(
        *conditionType, plan.frames[frame].bindings, operation);
    if (failed(width) || *width != 1)
      return operation.emitOpError()
             << "branch condition requires width-one Bits";
    if (!budget.chargeOccurrence(operation, operation.getOccurrence()) ||
        !budget.chargeAttribute(operation, operation.getConditionLocation()) ||
        !budget.chargeStorage<TransactionBranch>(operation, 1) ||
        !budget.reserve(operation, plan.branches, 1))
      return failure();
    SmallVector<StateVersionId, 0> entry, thenEnd;
    if (failed(snapshot(operation, entry)))
      return failure();
    TransactionPoint entryPoint = point();
    BranchId id = plan.branches.size();
    TransactionBranch branchRecord;
    branchRecord.operation = operation;
    branchRecord.frame = frame;
    branchRecord.condition = *condition;
    branchRecord.entry = entryPoint;
    branchRecord.occurrence = operation.getOccurrence();
    branchRecord.conditionLocation = operation.getConditionLocation();
    auto fact = predicate(*condition, operation);
    auto effectful = needsSelection(operation);
    if (failed(fact) || failed(effectful))
      return failure();
    branchRecord.conditionFact = *fact;
    plan.branches.push_back(std::move(branchRecord));
    if (failed(event(TransactionStateEventKind::BranchEnter, frame, id)))
      return failure();
    if (*effectful) {
      if (!budget.chargeStorage<SelectionBoundary>(operation, 1) ||
          !budget.reserve(operation, plan.selections, 1) ||
          !budget.chargeStorage<AdmissionControl>(operation, 1) ||
          !budget.reserve(operation, plan.controls, 1))
        return failure();
      SelectionBoundaryId selection = plan.selections.size();
      plan.branches[id].selection = selection;
      plan.selections.push_back({id, currentPrefix});
      plan.controls.push_back({BranchSelectControl{selection}, currentPath});
      if (failed(event(TransactionStateEventKind::SelectionEnter, frame,
                       selection)))
        return failure();
      ++currentPrefix;
      if (failed(event(TransactionStateEventKind::SelectionExit, frame,
                       selection)))
        return failure();
    }
    currentPath = {id, true};
    if (failed(event(TransactionStateEventKind::ThenEnter, frame, id)))
      return failure();
    SmallVector<StateOwnerId> armTouched;
    llvm::DenseSet<StateOwnerId> armSeen;
    SmallVector<BoundValueId, 0> whenTrue, whenFalse;
    if (failed(region(operation.getThenRegion().front(), frame, depth + 1,
                      armTouched, armSeen)) ||
        failed(armResults(operation, operation.getThenRegion().front(), frame,
                          whenTrue)) ||
        failed(snapshot(operation, thenEnd)))
      return failure();
    plan.branches[id].thenExitPrefix = currentPrefix;
    if (failed(event(TransactionStateEventKind::ThenExit, frame, id)) ||
        failed(restore(operation, entry)))
      return failure();
    currentPath = {id, false};
    if (failed(event(TransactionStateEventKind::ElseEnter, frame, id)) ||
        failed(region(operation.getElseRegion().front(), frame, depth + 1,
                      armTouched, armSeen)) ||
        failed(armResults(operation, operation.getElseRegion().front(), frame,
                          whenFalse)))
      return failure();
    plan.branches[id].elseExitPrefix = currentPrefix;
    if (failed(event(TransactionStateEventKind::ElseExit, frame, id)))
      return failure();
    currentPath = entryPoint.path;
    // Owner IDs already encode deterministic first discovery. Sorting this
    // union keeps join ordering independent of which arm first touched a state.
    uint64_t sortDepth = 0;
    for (uint64_t count = armTouched.size(); count > 1; count = (count + 1) / 2)
      ++sortDepth;
    if (!budget.charge(operation,
                       saturatedMultiply(armTouched.size(), 2 * sortDepth + 1)))
      return failure();
    llvm::sort(armTouched);
    uint64_t joinCount = 0;
    for (StateOwnerId owner : armTouched) {
      if (!budget.charge(operation))
        return failure();
      StateVersionId initial =
          owner < entry.size() ? entry[owner] : q0Versions[owner];
      StateVersionId trueVersion =
          owner < thenEnd.size() ? thenEnd[owner] : q0Versions[owner];
      if (trueVersion != initial || candidates[owner] != initial)
        ++joinCount;
    }
    uint64_t resultCount = operation.getNumResults();
    // Exact persistent record fees, in addition to ordinary value/type/edge
    // work. Events pay their own fee immediately before each append.
    if (!budget.chargeStorage<OwnerVersionBinding>(
            operation, saturatedMultiply(3, armTouched.size())) ||
        !budget.chargeStorage<TransactionStateJoin>(operation, joinCount) ||
        !budget.chargeStorage<StateVersion>(operation, joinCount) ||
        !budget.chargeStorage<StateJoinId>(operation, joinCount) ||
        !budget.chargeStorage<TransactionBranchResult>(operation,
                                                       resultCount) ||
        !budget.chargeStorage<BranchResultId>(operation, resultCount) ||
        !budget.reserve(operation, plan.joins, joinCount) ||
        !budget.reserve(operation, plan.versions, joinCount) ||
        !budget.reserve(operation, plan.branchResults, resultCount))
      return failure();
    auto &record = plan.branches[id];
    if (!budget.reserve(operation, record.entryVersions, armTouched.size()) ||
        !budget.reserve(operation, record.thenVersions, armTouched.size()) ||
        !budget.reserve(operation, record.elseVersions, armTouched.size()) ||
        !budget.reserve(operation, record.joins, joinCount) ||
        !budget.reserve(operation, record.results, resultCount))
      return failure();
    for (StateOwnerId owner : armTouched) {
      if (!budget.charge(operation) ||
          failed(touch(touched, seen, owner, operation)))
        return failure();
      StateVersionId initial =
          owner < entry.size() ? entry[owner] : q0Versions[owner];
      StateVersionId trueVersion =
          owner < thenEnd.size() ? thenEnd[owner] : q0Versions[owner];
      StateVersionId falseVersion = candidates[owner];
      record.entryVersions.push_back({owner, initial});
      record.thenVersions.push_back({owner, trueVersion});
      record.elseVersions.push_back({owner, falseVersion});
      if (trueVersion == initial && falseVersion == initial)
        continue;
      StateJoinId join = plan.joins.size();
      StateVersionId version = plan.versions.size();
      if (failed(requireVersionAvailable(initial, entryPoint, operation)) ||
          failed(requireVersionAvailable(
              trueVersion, {record.thenExitPrefix, {id, true}}, operation)) ||
          failed(requireVersionAvailable(
              falseVersion, {record.elseExitPrefix, {id, false}}, operation)))
        return failure();
      plan.joins.push_back(
          {id, owner, initial, trueVersion, falseVersion, version});
      plan.versions.push_back({plan.transaction, owner, ++ownerOrdinals[owner],
                               StateVersionKind::Join, initial, std::nullopt,
                               join, std::nullopt,
                               plan.owners[owner].payloadType, point()});
      record.joins.push_back(join);
      candidates[owner] = version;
      if (failed(event(TransactionStateEventKind::StateJoin, frame, join)))
        return failure();
    }
    for (auto [ordinal, original] : llvm::enumerate(operation.getResults())) {
      if (!budget.charge(operation))
        return failure();
      auto bound = value(original, anchor, frame);
      if (failed(bound) ||
          failed(equivalent(*bound, whenTrue[ordinal], operation)) ||
          failed(equivalent(*bound, whenFalse[ordinal], operation)))
        return failure();
      auto type = payloadType(*bound, operation);
      if (failed(type) || !isa<BitsType>(*type))
        return operation.emitOpError() << "branch results require scalar Bits";
      if (failed(requireAvailable(whenTrue[ordinal],
                                  {record.thenExitPrefix, {id, true}},
                                  operation)) ||
          failed(requireAvailable(whenFalse[ordinal],
                                  {record.elseExitPrefix, {id, false}},
                                  operation)) ||
          failed(assignValidity(*bound, point(), operation)))
        return failure();
      BranchResultId result = plan.branchResults.size();
      plan.branchResults.push_back({id, frame, ordinal, *bound,
                                    whenTrue[ordinal], whenFalse[ordinal],
                                    *type, point()});
      record.results.push_back(result);
      // No alias: the result denotes a selection, never either arm's yield.
      if (failed(event(TransactionStateEventKind::BranchResult, frame, result)))
        return failure();
    }
    return event(TransactionStateEventKind::BranchExit, frame, id);
  }

  LogicalResult region(Block &block, InvocationFrameId frame, unsigned depth,
                       SmallVectorImpl<StateOwnerId> &touched,
                       llvm::DenseSet<StateOwnerId> &seen, bool guard = false) {
    if (!budget.charge(block.getParentOp()))
      return failure();
    for (Operation &op : block.without_terminator()) {
      if (!budget.charge(&op))
        return failure();
      if (auto access = dyn_cast<SourceStateReadOp>(op)) {
        if (failed(read(access, frame, touched, seen)))
          return failure();
      } else if (auto intent = dyn_cast<SourceStateWriteOp>(op)) {
        if (failed(write(intent, frame, touched, seen)))
          return failure();
      } else if (auto split = dyn_cast<SourceBranchOp>(op)) {
        if (guard)
          return split.emitOpError() << "branches are not admitted in guards";
        if (failed(branch(split, frame, depth, touched, seen)))
          return failure();
      } else if (auto method = dyn_cast<SourceMethodCallOp>(op)) {
        if (failed(call(method, frame, depth, touched, seen)))
          return failure();
      } else if (auto check = dyn_cast<SourceExpectOp>(op)) {
        if (failed(bodyCheck(check, frame)))
          return failure();
      } else if (auto observation = dyn_cast<SourceObserveOp>(op)) {
        if (failed(observe(observation, frame)))
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
    if (!budget.chargeStorage<GuardBoundary>(function, 1) ||
        !budget.reserve(function, plan.guards, 1) ||
        !budget.chargeStorage<AdmissionControl>(function, 1) ||
        !budget.reserve(function, plan.controls, 1))
      return failure();
    GuardBoundary boundary;
    boundary.callable = function;
    boundary.frame = frame;
    boundary.control = currentPrefix;
    boundary.domain = domain;
    boundary.location =
        function->getAttrOfType<DictionaryAttr>("ac.guard_location");
    plan.guards.push_back(std::move(boundary));
    plan.controls.push_back({CallableGuardControl{guard}, currentPath});
    plan.frames[frame].guard = guard;
    if (failed(event(TransactionStateEventKind::GuardEnter, frame, guard)))
      return failure();
    InvocationResult result;
    result.guard = guard;
    llvm::DenseSet<StateOwnerId> seen;
    StateReadId firstRead = plan.reads.size();
    auto &guardBlock = function->getRegion(0).front();
    if (failed(region(guardBlock, frame, depth, result.touched, seen, true)) ||
        failed(entryVersions(function, result.touched, saved,
                             plan.guards[guard].entryVersions)) ||
        !budget.chargeStorage<StateReadId>(function,
                                           plan.reads.size() - firstRead) ||
        !budget.reserve(function, plan.guards[guard].reads,
                        plan.reads.size() - firstRead))
      return failure();
    for (StateReadId read = firstRead; read != plan.reads.size(); ++read)
      plan.guards[guard].reads.push_back(read);
    auto guardYield = cast<YieldOp>(guardBlock.back());
    auto condition = useValue(guardYield.getValues().front(),
                              plan.frames[frame].anchor, frame, guardYield);
    if (failed(condition))
      return failure();
    auto fact = predicate(*condition, function);
    if (failed(fact))
      return failure();
    plan.guards[guard].predicate = *condition;
    plan.guards[guard].predicateFact = *fact;
    ++currentPrefix;
    if (failed(event(TransactionStateEventKind::GuardExit, frame, guard)))
      return failure();
    auto &bodyBlock = function->getRegion(1).front();
    if (failed(region(bodyBlock, frame, depth, result.touched, seen)))
      return failure();
    auto yielded = cast<YieldOp>(bodyBlock.back()).getValues();
    if (!budget.chargeStorage<BoundValueId>(function, yielded.size()) ||
        !budget.reserve(function, result.yielded, yielded.size()))
      return failure();
    for (Value original : yielded) {
      auto bound = useValue(original, plan.frames[frame].anchor, frame,
                            bodyBlock.getTerminator());
      if (failed(bound))
        return failure();
      result.yielded.push_back(*bound);
    }
    plan.frames[frame].exit = point();
    return result;
  }

  LogicalResult verifyPlan() {
    auto site = plan.transaction.getOperation();
    auto invalid = [&]() -> LogicalResult {
      return site->emitOpError()
             << "transaction control, path or validity inventory is invalid";
    };
    if (currentPrefix != plan.controls.size() ||
        plan.controls.size() != plan.guards.size() + plan.selections.size() ||
        plan.guards.size() != plan.frames.size() || currentPath.branch ||
        plan.guard != plan.frames[plan.rootFrame].guard)
      return invalid();
    for (auto [id, control] : llvm::enumerate(plan.controls)) {
      if (!budget.charge(site))
        return failure();
      if (auto *guard = std::get_if<CallableGuardControl>(&control.boundary)) {
        if (guard->guard >= plan.guards.size() ||
            plan.guards[guard->guard].control != id)
          return invalid();
      } else {
        auto selection =
            std::get<BranchSelectControl>(control.boundary).selection;
        if (selection >= plan.selections.size() ||
            plan.selections[selection].control != id)
          return invalid();
      }
      auto legal = sourcePathDominates(control.path, control.path, site);
      if (failed(legal))
        return failure();
    }
    for (auto [id, frame] : llvm::enumerate(plan.frames)) {
      if (!budget.charge(site) || frame.guard >= plan.guards.size() ||
          plan.guards[frame.guard].frame != id ||
          frame.entry.prefix != plan.guards[frame.guard].control ||
          !samePath(frame.entry.path, frame.exit.path) ||
          frame.entry.prefix >= frame.exit.prefix ||
          frame.exit.prefix > currentPrefix)
        return invalid();
    }
    for (auto [id, branch] : llvm::enumerate(plan.branches)) {
      if (!budget.charge(site) || branch.frame >= plan.frames.size() ||
          branch.condition >= plan.values.size() ||
          (branch.entry.path.branch && *branch.entry.path.branch >= id) ||
          branch.thenExitPrefix <
              branch.entry.prefix + unsigned(branch.selection.has_value()) ||
          branch.elseExitPrefix < branch.thenExitPrefix ||
          branch.elseExitPrefix > currentPrefix ||
          failed(requireAvailable(branch.condition, branch.entry, site)))
        return invalid();
      if (branch.selection) {
        if (*branch.selection >= plan.selections.size())
          return invalid();
        const auto &selection = plan.selections[*branch.selection];
        if (selection.branch != id ||
            selection.control != branch.entry.prefix ||
            selection.control >= plan.controls.size() ||
            !samePath(plan.controls[selection.control].path, branch.entry.path))
          return invalid();
        auto *control = std::get_if<BranchSelectControl>(
            &plan.controls[selection.control].boundary);
        if (!control || control->selection != *branch.selection)
          return invalid();
      } else if (branch.thenExitPrefix != branch.entry.prefix ||
                 branch.elseExitPrefix != branch.entry.prefix)
        return invalid();
    }
    for (auto [id, ref] : llvm::enumerate(plan.values)) {
      if (!budget.charge(site) || !validityAssigned[id] ||
          ref.anchor >= plan.instances.size() ||
          ref.frame >= plan.frames.size() ||
          ref.validity.prefix > currentPrefix)
        return invalid();
      auto legal =
          sourcePathDominates(ref.validity.path, ref.validity.path, site);
      if (failed(legal))
        return failure();
    }
    for (auto [id, version] : llvm::enumerate(plan.versions)) {
      if (!budget.charge(site) || version.owner >= plan.owners.size() ||
          version.validity.prefix > currentPrefix)
        return invalid();
      switch (version.kind) {
      case StateVersionKind::OldQ:
        if (!version.value || version.predecessor || version.write ||
            version.join || version.ordinal || !samePoint(version.validity, {}))
          return invalid();
        break;
      case StateVersionKind::Write:
        if (!version.value || !version.predecessor || !version.write ||
            version.join || !version.ordinal ||
            *version.write >= plan.writes.size() ||
            plan.writes[*version.write].version != id)
          return invalid();
        break;
      case StateVersionKind::Join:
        if (version.value || !version.predecessor || version.write ||
            !version.join || !version.ordinal ||
            *version.join >= plan.joins.size() ||
            plan.joins[*version.join].result != id)
          return invalid();
        break;
      }
      if (version.value &&
          failed(requireAvailable(*version.value, version.validity, site)))
        return failure();
      if (version.predecessor &&
          (*version.predecessor >= id ||
           plan.versions[*version.predecessor].owner != version.owner))
        return invalid();
    }
    for (auto [id, join] : llvm::enumerate(plan.joins)) {
      if (!budget.charge(site, 5) || join.branch >= plan.branches.size() ||
          join.owner >= plan.owners.size() ||
          join.result >= plan.versions.size())
        return invalid();
      const auto &branch = plan.branches[join.branch];
      const auto &result = plan.versions[join.result];
      if (result.kind != StateVersionKind::Join || result.join != id ||
          result.owner != join.owner || result.predecessor != join.entry ||
          !samePoint(result.validity,
                     {branch.elseExitPrefix, branch.entry.path}) ||
          failed(requireVersionAvailable(join.entry, branch.entry, site)) ||
          failed(requireVersionAvailable(
              join.whenTrue, {branch.thenExitPrefix, {join.branch, true}},
              site)) ||
          failed(requireVersionAvailable(
              join.whenFalse, {branch.elseExitPrefix, {join.branch, false}},
              site)))
        return invalid();
      for (StateVersionId input : {join.entry, join.whenTrue, join.whenFalse})
        if (!budget.charge(site) || input >= join.result ||
            plan.versions[input].owner != join.owner ||
            !areEquivalentHardwareTypes(plan.versions[input].payloadType,
                                        result.payloadType))
          return invalid();
    }
    for (const auto &result : plan.branchResults) {
      if (!budget.charge(site, 5) || result.branch >= plan.branches.size() ||
          result.frame >= plan.frames.size() ||
          result.result >= plan.values.size())
        return invalid();
      const auto &branch = plan.branches[result.branch];
      auto operation = branch.operation;
      const auto &bound = plan.values[result.result];
      TransactionPoint join{branch.elseExitPrefix, branch.entry.path};
      if (result.frame != branch.frame || bound.frame != result.frame ||
          bound.anchor != plan.frames[result.frame].anchor ||
          result.ordinal >= operation.getNumResults() ||
          bound.value != operation.getResult(result.ordinal) ||
          aliases.contains(result.result) ||
          !samePoint(result.validity, join) ||
          !samePoint(bound.validity, join) ||
          failed(requireAvailable(
              result.whenTrue, {branch.thenExitPrefix, {result.branch, true}},
              site)) ||
          failed(requireAvailable(
              result.whenFalse, {branch.elseExitPrefix, {result.branch, false}},
              site)))
        return invalid();
    }
    for (const auto &call : plan.calls) {
      if (!budget.charge(site) || call.callee >= plan.frames.size() ||
          call.caller >= plan.frames.size())
        return invalid();
      const auto &frame = plan.frames[call.callee];
      if (frame.parent != call.caller || frame.call != call.operation)
        return invalid();
      for (const auto &result : call.results) {
        if (!budget.charge(site) || result.result >= plan.values.size() ||
            result.yielded >= plan.values.size() ||
            !samePoint(plan.values[result.result].validity, frame.exit) ||
            aliases.contains(result.result) ||
            failed(requireAvailable(result.yielded, frame.exit, site)))
          return invalid();
      }
    }
    for (const auto &check : plan.bodyChecks)
      if (!budget.charge(site) || check.frame >= plan.frames.size() ||
          failed(requireAvailable(check.condition, check.point, site)) ||
          failed(requireAvailable(check.authoredPath, check.point, site)))
        return invalid();
    for (const auto &observation : plan.observations) {
      if (!budget.charge(site) || observation.frame >= plan.frames.size() ||
          failed(requireAvailable(observation.authoredPath, observation.point,
                                  site)))
        return invalid();
      for (BoundValueId value : observation.values)
        if (failed(requireAvailable(value, observation.point, site)))
          return failure();
    }
    // Replay boundaries using separate frame/branch stacks. Prefixes never
    // rewind when candidate environments restore at an alternate arm.
    struct BranchCursor {
      BranchId branch;
      unsigned stage;
    };
    SmallVector<InvocationFrameId, 0> frames;
    SmallVector<BranchCursor, 0> branches;
    if (!budget.chargeStorage<InvocationFrameId>(site, plan.frames.size()) ||
        !budget.chargeStorage<BranchCursor>(site, plan.branches.size()) ||
        !budget.reserve(site, frames, plan.frames.size()) ||
        !budget.reserve(site, branches, plan.branches.size()))
      return failure();
    frames.push_back(plan.rootFrame);
    TransactionPoint cursor;
    BodyCheckId nextCheck = 0;
    ObservationId nextObservation = 0;
    AdmissionControlId nextControl = 0;
    std::optional<AdmissionControlId> openControl;
    for (const auto &event : plan.events) {
      if (!budget.charge(site) || frames.empty() ||
          event.frame >= plan.frames.size())
        return invalid();
      unsigned references = unsigned(event.read.has_value()) +
                            unsigned(event.write.has_value()) +
                            unsigned(event.guard.has_value()) +
                            unsigned(event.selection.has_value()) +
                            unsigned(event.call.has_value()) +
                            unsigned(event.bodyCheck.has_value()) +
                            unsigned(event.observation.has_value()) +
                            unsigned(event.branch.has_value()) +
                            unsigned(event.join.has_value()) +
                            unsigned(event.branchResult.has_value());
      bool boundary =
          event.kind == TransactionStateEventKind::TransactionEnter ||
          event.kind == TransactionStateEventKind::TransactionExit;
      if (references != (boundary ? 0u : 1u))
        return invalid();
      if (event.kind != TransactionStateEventKind::CallExit &&
          event.frame != frames.back())
        return invalid();
      switch (event.kind) {
      case TransactionStateEventKind::TransactionEnter:
        if (event.frame != plan.rootFrame || cursor.prefix ||
            cursor.path.branch)
          return invalid();
        break;
      case TransactionStateEventKind::TransactionExit:
        if (frames.size() != 1 || !branches.empty() || cursor.path.branch ||
            cursor.prefix != currentPrefix)
          return invalid();
        break;
      case TransactionStateEventKind::GuardEnter:
      case TransactionStateEventKind::GuardExit: {
        if (!event.guard || *event.guard >= plan.guards.size())
          return invalid();
        const auto &guard = plan.guards[*event.guard];
        if (guard.frame != event.frame ||
            guard.control >= plan.controls.size() ||
            !samePath(plan.controls[guard.control].path, cursor.path))
          return invalid();
        if (event.kind == TransactionStateEventKind::GuardEnter) {
          if (openControl || guard.control != nextControl ||
              cursor.prefix != nextControl)
            return invalid();
          openControl = nextControl;
        } else {
          if (openControl != nextControl || guard.control != nextControl ||
              cursor.prefix != nextControl)
            return invalid();
          openControl.reset();
          ++cursor.prefix;
          ++nextControl;
        }
        break;
      }
      case TransactionStateEventKind::SelectionEnter:
      case TransactionStateEventKind::SelectionExit: {
        if (!event.selection || *event.selection >= plan.selections.size())
          return invalid();
        const auto &selection = plan.selections[*event.selection];
        if (branches.empty() || branches.back().branch != selection.branch ||
            branches.back().stage != 0 || selection.control != nextControl ||
            cursor.prefix != nextControl)
          return invalid();
        if (event.kind == TransactionStateEventKind::SelectionEnter) {
          if (openControl)
            return invalid();
          openControl = nextControl;
        } else {
          if (openControl != nextControl)
            return invalid();
          openControl.reset();
          ++cursor.prefix;
          ++nextControl;
        }
        break;
      }
      case TransactionStateEventKind::CallEnter: {
        if (!event.call || *event.call >= plan.calls.size())
          return invalid();
        const auto &call = plan.calls[*event.call];
        if (call.caller != event.frame ||
            !samePoint(plan.frames[call.callee].entry, cursor))
          return invalid();
        frames.push_back(call.callee);
        break;
      }
      case TransactionStateEventKind::CallExit: {
        if (!event.call || *event.call >= plan.calls.size() ||
            frames.size() < 2)
          return invalid();
        const auto &call = plan.calls[*event.call];
        if (call.callee != frames.back() || call.caller != event.frame ||
            !samePoint(plan.frames[call.callee].exit, cursor))
          return invalid();
        frames.pop_back();
        break;
      }
      case TransactionStateEventKind::BranchEnter:
        if (!event.branch || *event.branch >= plan.branches.size() ||
            !samePoint(plan.branches[*event.branch].entry, cursor))
          return invalid();
        branches.push_back({*event.branch, 0});
        break;
      case TransactionStateEventKind::ThenEnter:
      case TransactionStateEventKind::ThenExit:
      case TransactionStateEventKind::ElseEnter:
      case TransactionStateEventKind::ElseExit:
      case TransactionStateEventKind::BranchExit: {
        if (!event.branch || branches.empty() ||
            branches.back().branch != *event.branch)
          return invalid();
        auto &active = branches.back();
        const auto &branch = plan.branches[*event.branch];
        if (branch.frame != event.frame)
          return invalid();
        if (event.kind == TransactionStateEventKind::ThenEnter) {
          if (active.stage != 0 ||
              cursor.prefix !=
                  branch.entry.prefix + unsigned(branch.selection.has_value()))
            return invalid();
          active.stage = 1;
          cursor.path = {*event.branch, true};
        } else if (event.kind == TransactionStateEventKind::ThenExit) {
          if (active.stage != 1 || cursor.prefix != branch.thenExitPrefix)
            return invalid();
          active.stage = 2;
        } else if (event.kind == TransactionStateEventKind::ElseEnter) {
          if (active.stage != 2)
            return invalid();
          active.stage = 3;
          cursor.path = {*event.branch, false};
        } else if (event.kind == TransactionStateEventKind::ElseExit) {
          if (active.stage != 3 || cursor.prefix != branch.elseExitPrefix)
            return invalid();
          active.stage = 4;
          // The exit event still belongs to the arm; lifting starts afterwards.
        } else {
          if (active.stage != 4)
            return invalid();
          cursor.path = branch.entry.path;
          branches.pop_back();
        }
        break;
      }
      case TransactionStateEventKind::StateJoin:
        if (!event.join || *event.join >= plan.joins.size())
          return invalid();
        cursor.path = plan.branches[plan.joins[*event.join].branch].entry.path;
        break;
      case TransactionStateEventKind::BranchResult:
        if (!event.branchResult ||
            *event.branchResult >= plan.branchResults.size())
          return invalid();
        cursor.path =
            plan.branches[plan.branchResults[*event.branchResult].branch]
                .entry.path;
        break;
      case TransactionStateEventKind::Read:
        if (!event.read || *event.read >= plan.reads.size() ||
            plan.reads[*event.read].frame != event.frame ||
            failed(requireVersionAvailable(plan.reads[*event.read].version,
                                           cursor, site)) ||
            !samePoint(plan.values[plan.reads[*event.read].result].validity,
                       cursor))
          return invalid();
        break;
      case TransactionStateEventKind::Write:
        if (!event.write || *event.write >= plan.writes.size() ||
            plan.writes[*event.write].frame != event.frame ||
            !samePoint(
                plan.versions[plan.writes[*event.write].version].validity,
                cursor))
          return invalid();
        break;
      case TransactionStateEventKind::BodyCheck:
        if (!event.bodyCheck || *event.bodyCheck != nextCheck ||
            nextCheck >= plan.bodyChecks.size() ||
            plan.bodyChecks[nextCheck].frame != event.frame ||
            !samePoint(plan.bodyChecks[nextCheck].point, cursor))
          return invalid();
        ++nextCheck;
        break;
      case TransactionStateEventKind::Observe:
        if (!event.observation || *event.observation != nextObservation ||
            nextObservation >= plan.observations.size() ||
            plan.observations[nextObservation].frame != event.frame ||
            !samePoint(plan.observations[nextObservation].point, cursor))
          return invalid();
        ++nextObservation;
        break;
      }
      if (!samePoint(event.point, cursor))
        return invalid();
    }
    if (openControl || nextControl != plan.controls.size() ||
        nextCheck != plan.bodyChecks.size() ||
        nextObservation != plan.observations.size() || frames.size() != 1 ||
        !branches.empty())
      return invalid();
    // Static totality follows structural bypass, retaining unreachable rows.
    enum class Fact { False, True, Unknown };
    auto conjunction = [](Fact left, Fact right) {
      if (left == Fact::False || right == Fact::False)
        return Fact::False;
      if (left == Fact::True && right == Fact::True)
        return Fact::True;
      return Fact::Unknown;
    };
    Fact final = Fact::True;
    for (const auto &control : plan.controls) {
      Fact path = Fact::True;
      auto ancestry = control.path;
      unsigned depth = 0;
      while (ancestry.branch) {
        if (!budget.charge(site) || ++depth > maxDepth)
          return failure();
        const auto &branch = plan.branches[*ancestry.branch];
        Fact selected = !branch.conditionFact ? Fact::Unknown
                        : branch.conditionFact.getValue() == ancestry.whenTrue
                            ? Fact::True
                            : Fact::False;
        path = conjunction(path, selected);
        ancestry = branch.entry.path;
      }
      Fact acceptance = Fact::Unknown;
      if (auto *guard = std::get_if<CallableGuardControl>(&control.boundary)) {
        auto raw = plan.guards[guard->guard].predicateFact;
        if (raw)
          acceptance = raw.getValue() ? Fact::True : Fact::False;
      } else {
        auto selection =
            std::get<BranchSelectControl>(control.boundary).selection;
        if (plan.branches[plan.selections[selection].branch].conditionFact)
          acceptance = Fact::True;
      }
      Fact bypass = path == Fact::False || acceptance == Fact::True ? Fact::True
                    : path == Fact::True ? acceptance
                                         : Fact::Unknown;
      final = conjunction(final, bypass);
    }
    if (!plan.results.empty() && final != Fact::True)
      return site->emitOpError()
             << "ordinary transaction results require proven total admission";
    return success();
  }

  LogicalResult chargeDump() {
    // Dump traverses only these records. Charge every variable-length view now,
    // so printing a valid analysis cannot discover an exhaustion halfway
    // through.
    auto site = plan.transaction.getOperation();
    // Prepay the added decimal prefix fields and exact clock/reset identities.
    if (!budget.charge(site, 64))
      return failure();
    for (uint64_t count :
         {plan.values.size(), plan.versions.size(), plan.bodyChecks.size(),
          plan.observations.size(), plan.events.size()})
      if (!budget.charge(site, saturatedMultiply(96, count)))
        return failure();
    for (uint64_t count :
         {plan.instances.size(), plan.frames.size(), plan.values.size(),
          plan.owners.size(), plan.versions.size(), plan.reads.size(),
          plan.writes.size(), plan.guards.size(), plan.calls.size(),
          plan.bodyChecks.size(), plan.observations.size(), plan.events.size(),
          plan.finalVersions.size(), plan.results.size(), plan.branches.size(),
          plan.joins.size(), plan.branchResults.size(), plan.controls.size(),
          plan.selections.size()})
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
      if (!budget.charge(site, 160 + frame.guardArguments.size() +
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
    for (const auto &branch : plan.branches) {
      auto operation = branch.operation;
      if (!budget.charge(site, 160) ||
          !budget.charge(site,
                         saturatedMultiply(28, branch.entryVersions.size() +
                                                   branch.thenVersions.size() +
                                                   branch.elseVersions.size() +
                                                   branch.joins.size() +
                                                   branch.results.size())) ||
          !budget.chargeOccurrence(site, branch.occurrence) ||
          !budget.chargeAttribute(site, branch.conditionLocation))
        return failure();
    }
    if (!budget.charge(site,
                       saturatedMultiply(96, plan.controls.size() +
                                                 plan.selections.size())) ||
        !budget.charge(site, saturatedMultiply(160, plan.joins.size())) ||
        !budget.charge(site, saturatedMultiply(160, plan.branchResults.size())))
      return failure();
    for (const auto &check : plan.bodyChecks)
      if (!budget.charge(site, 8) ||
          !budget.chargeOccurrence(site, check.occurrence) ||
          !budget.chargeAttribute(site, check.location) ||
          (check.message && !budget.chargeAttribute(site, check.message)))
        return failure();
    for (const auto &observation : plan.observations)
      if (!budget.charge(site, 8 + observation.values.size()) ||
          !budget.chargeOccurrence(site, observation.occurrence) ||
          !budget.chargeAttribute(site, observation.location) ||
          !budget.chargeAttribute(site, observation.kind) ||
          !budget.chargeAttribute(site, observation.spec))
        return failure();
    return success();
  }

  mlir::ModuleOp package;
  HardwareAnalysis hardware;
  StateAnalysisBudget &budget;
  const PackageInventory &inventory;
  AttributeTypePreflight subelements;
  TransactionStatePlan plan;
  llvm::DenseMap<ValueKey, BoundValueId> values;
  SmallVector<uint8_t, 0> validityAssigned;
  SmallVector<uint8_t, 0> lexicalValues;
  AdmissionPrefix currentPrefix = 0;
  llvm::DenseMap<BoundValueId, BoundValueId> aliases;
  llvm::DenseMap<OwnerKey, StateOwnerId> owners;
  llvm::DenseMap<OwnerKey, LexicalInstanceId> instances;
  llvm::DenseMap<OwnerKey, ResolvedSourceMethod> resolutions;
  SmallVector<StateVersionId, 0> candidates;
  SmallVector<StateVersionId, 0> q0Versions;
  SmallVector<uint64_t, 0> ownerOrdinals;
  TransactionSourcePath currentPath;
  llvm::DenseMap<BoundValueId, BoundValueId> resultBindings;
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
  case TransactionStateEventKind::SelectionEnter:
    return "selection-enter";
  case TransactionStateEventKind::SelectionExit:
    return "selection-exit";
  case TransactionStateEventKind::Write:
    return "write";
  case TransactionStateEventKind::BodyCheck:
    return "body-check";
  case TransactionStateEventKind::Observe:
    return "observe";
  case TransactionStateEventKind::CallEnter:
    return "call-enter";
  case TransactionStateEventKind::CallExit:
    return "call-exit";
  case TransactionStateEventKind::BranchEnter:
    return "branch-enter";
  case TransactionStateEventKind::ThenEnter:
    return "then-enter";
  case TransactionStateEventKind::ThenExit:
    return "then-exit";
  case TransactionStateEventKind::ElseEnter:
    return "else-enter";
  case TransactionStateEventKind::ElseExit:
    return "else-exit";
  case TransactionStateEventKind::StateJoin:
    return "state-join";
  case TransactionStateEventKind::BranchResult:
    return "branch-result";
  case TransactionStateEventKind::BranchExit:
    return "branch-exit";
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

void printPath(llvm::raw_ostream &out, TransactionSourcePath path) {
  if (path.branch)
    out << 'h' << *path.branch << (path.whenTrue ? ":then" : ":else");
  else
    out << "root";
}

void printPoint(llvm::raw_ostream &out, TransactionPoint point) {
  out << point.prefix << '@';
  printPath(out, point.path);
}

void printPlan(llvm::raw_ostream &out, const TransactionStatePlan &plan) {
  auto transaction = plan.transaction;
  auto module = plan.lexicalModule;
  out << "transaction @" << module.getSymName() << "::" << transaction.getName()
      << " occurrence=" << transaction.getOccurrence() << " clock-anchor=i"
      << plan.domain.clock.anchor << " reset-anchor=i"
      << plan.domain.reset.anchor << " clock=v" << plan.clockValue << " reset=v"
      << plan.resetValue << '\n';
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
    out << " guard=g" << frame.guard << " entry=";
    printPoint(out, frame.entry);
    out << " exit=";
    printPoint(out, frame.exit);
    out << '\n';
  }
  for (auto [id, control] : llvm::enumerate(plan.controls)) {
    out << "  control a" << id;
    if (auto *guard = std::get_if<CallableGuardControl>(&control.boundary))
      out << " guard=g" << guard->guard;
    else
      out << " selection=s"
          << std::get<BranchSelectControl>(control.boundary).selection;
    out << " path=";
    printPath(out, control.path);
    out << '\n';
  }
  for (auto [id, selection] : llvm::enumerate(plan.selections))
    out << "  selection s" << id << " branch=h" << selection.branch
        << " control=a" << selection.control << '\n';
  for (auto [id, ref] : llvm::enumerate(plan.values)) {
    out << "  value v" << id << " instance=i" << ref.anchor << " frame=f"
        << ref.frame << " validity=";
    printPoint(out, ref.validity);
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
        << " ordinal=" << version.ordinal << " validity=";
    printPoint(out, version.validity);
    switch (version.kind) {
    case StateVersionKind::OldQ:
      out << " value=v" << *version.value << " Q0";
      break;
    case StateVersionKind::Write:
      out << " value=v" << *version.value << " predecessor=q"
          << *version.predecessor << " write=w" << *version.write;
      break;
    case StateVersionKind::Join: {
      const auto &join = plan.joins[*version.join];
      out << " predecessor=q" << *version.predecessor << " join=j"
          << *version.join << " branch=h" << join.branch << " true=q"
          << join.whenTrue << " false=q" << join.whenFalse;
      break;
    }
    }
    out << '\n';
  }
  for (auto [id, read] : llvm::enumerate(plan.reads))
    out << "  read r" << id << " frame=f" << read.frame << " owner=o"
        << read.owner << " version=q" << read.version << " result=v"
        << read.result << " path=v" << read.authoredPath << '\n';
  for (auto [id, write] : llvm::enumerate(plan.writes))
    out << "  write w" << id << " frame=f" << write.frame << " owner=o"
        << write.owner << " predecessor=q" << write.predecessor << " version=q"
        << write.version << " value=v" << write.value << " path=v"
        << write.authoredPath << '\n';
  for (auto [id, guard] : llvm::enumerate(plan.guards)) {
    out << "  guard g" << id << " frame=f" << guard.frame
        << " domain=transaction predicate=v" << guard.predicate << " fact=";
    if (guard.predicateFact)
      out << (guard.predicateFact.getValue() ? "true" : "false");
    else
      out << "unknown";
    out << " control=a" << guard.control;
    out << " entry=";
    printVersions(out, guard.entryVersions);
    out << " reads=[";
    llvm::interleaveComma(guard.reads, out,
                          [&](StateReadId read) { out << 'r' << read; });
    out << "]\n";
  }
  for (auto [id, call] : llvm::enumerate(plan.calls)) {
    out << "  call c" << id << " caller=f" << call.caller << " callee=f"
        << call.callee << " guard=g" << plan.frames[call.callee].guard
        << " path=v" << call.authoredPath;
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
  for (auto [id, branch] : llvm::enumerate(plan.branches)) {
    auto operation = branch.operation;
    out << "  branch h" << id << " frame=f" << branch.frame << " condition=v"
        << branch.condition << " point=";
    printPoint(out, branch.entry);
    out << " then-exit=" << branch.thenExitPrefix
        << " else-exit=" << branch.elseExitPrefix;
    if (branch.selection)
      out << " selection=s" << *branch.selection;
    out << " fact="
        << (!branch.conditionFact             ? "unknown"
            : branch.conditionFact.getValue() ? "true"
                                              : "false");
    out << " entry=";
    printVersions(out, branch.entryVersions);
    out << " then=";
    printVersions(out, branch.thenVersions);
    out << " else=";
    printVersions(out, branch.elseVersions);
    out << " joins=[";
    llvm::interleaveComma(branch.joins, out,
                          [&](StateJoinId join) { out << 'j' << join; });
    out << "] results=[";
    llvm::interleaveComma(branch.results, out,
                          [&](BranchResultId result) { out << 'b' << result; });
    out << "] occurrence=" << branch.occurrence
        << " location=" << branch.conditionLocation << '\n';
  }
  for (auto [id, join] : llvm::enumerate(plan.joins))
    out << "  join j" << id << " branch=h" << join.branch << " owner=o"
        << join.owner << " entry=q" << join.entry << " true=q" << join.whenTrue
        << " false=q" << join.whenFalse << " result=q" << join.result << '\n';
  for (auto [id, result] : llvm::enumerate(plan.branchResults)) {
    out << "  branch-result b" << id << " branch=h" << result.branch
        << " frame=f" << result.frame << " ordinal=" << result.ordinal
        << " result=v" << result.result << " true=v" << result.whenTrue
        << " false=v" << result.whenFalse << " validity=";
    printPoint(out, result.validity);
    out << '\n';
  }
  for (auto [id, check] : llvm::enumerate(plan.bodyChecks)) {
    out << "  body-check b" << id << " frame=f" << check.frame << " condition=v"
        << check.condition << " path=v" << check.authoredPath << " point=";
    printPoint(out, check.point);
    out << " occurrence=" << check.occurrence << " location=" << check.location
        << " message=";
    if (check.message)
      out << check.message;
    else
      out << "<absent>";
    out << '\n';
  }
  for (auto [id, observation] : llvm::enumerate(plan.observations)) {
    out << "  observe o" << id << " frame=f" << observation.frame << " path=v"
        << observation.authoredPath << " point=";
    printPoint(out, observation.point);
    out << " values=[";
    llvm::interleaveComma(observation.values, out,
                          [&](BoundValueId value) { out << 'v' << value; });
    out << "] occurrence=" << observation.occurrence
        << " location=" << observation.location << " kind=" << observation.kind
        << " spec=" << observation.spec << '\n';
  }
  for (auto [id, event] : llvm::enumerate(plan.events)) {
    out << "  event " << id << ' ' << eventName(event.kind) << " frame=f"
        << event.frame << " point=";
    printPoint(out, event.point);
    if (event.selection)
      out << " selection=s" << *event.selection;
    if (event.read)
      out << " read=r" << *event.read;
    if (event.write)
      out << " write=w" << *event.write;
    if (event.guard)
      out << " guard=g" << *event.guard;
    if (event.call)
      out << " call=c" << *event.call;
    if (event.bodyCheck)
      out << " body-check=b" << *event.bodyCheck;
    if (event.observation)
      out << " observation=o" << *event.observation;
    if (event.branch)
      out << " branch=h" << *event.branch;
    if (event.join)
      out << " join=j" << *event.join;
    if (event.branchResult)
      out << " branch-result=b" << *event.branchResult;
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

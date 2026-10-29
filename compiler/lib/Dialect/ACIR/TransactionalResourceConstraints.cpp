#include "pycircuit/Dialect/ACIR/TransactionalContracts.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

using namespace mlir;

namespace acir::ac {

// Close resource identity constraints after every instance occurrence is bound.
// Allocation is deliberately later than both alias and payload validation.
LogicalResult
resolveTransactionalResourceConnections(mlir::ModuleOp package,
                                        ArrayRef<ac::ModuleOp> modules) {
  // Each registration specializes its template independently, including
  // source-global Rules shared by differently bound instances.
  unsigned ruleIndex = 0;
  for (auto module : modules) {
    llvm::DenseSet<Operation *> specialized;
    SmallVector<ac::TransactionRuleCallOp> calls;
    module.walk([&](ac::TransactionRuleCallOp call) { calls.push_back(call); });
    for (auto call : calls) {
      Operation *target = SymbolTable::lookupSymbolIn(module, call.getCallee());
      if (!target)
        target = SymbolTable::lookupSymbolIn(package, call.getCallee());
      auto rule = dyn_cast_or_null<ac::TransactionRuleOp>(target);
      if (!rule)
        return call.emitOpError("Rule registration requires its provider body");
      auto clone = cast<ac::TransactionRuleOp>(rule->clone());
      std::string name;
      do {
        name = "__bound_rule_" + std::to_string(ruleIndex++);
      } while (SymbolTable::lookupSymbolIn(module, name));
      specialized.insert(clone);
      clone->setAttr("sym_name", StringAttr::get(package.getContext(), name));
      module.getBody().front().getOperations().insert(
          module.getBody().front().getTerminator()->getIterator(), clone);
      call->setAttr("callee",
                    FlatSymbolRefAttr::get(package.getContext(), name));
    }
    for (auto rule : llvm::make_early_inc_range(
             module.getBody().front().getOps<ac::TransactionRuleOp>()))
      if (!specialized.contains(rule))
        rule.erase();
  }
  for (auto rule :
       llvm::make_early_inc_range(package.getOps<ac::TransactionRuleOp>()))
    rule.erase();

  auto lookup = [&](StringRef name) {
    return SymbolTable::lookupSymbolIn(package, name);
  };
  llvm::DenseMap<Value, unsigned> nodes;
  SmallVector<Value> values;
  SmallVector<unsigned> parents;
  auto resource = [](Type type) {
    return isa<ac::PrimitiveType, ac::StateType, ac::QueueType>(type);
  };
  auto node = [&](Value value) {
    auto found = nodes.find(value);
    if (found != nodes.end())
      return found->second;
    unsigned id = values.size();
    nodes[value] = id;
    values.push_back(value);
    parents.push_back(id);
    return id;
  };
  auto root = [&](unsigned id) {
    while (parents[id] != id) {
      parents[id] = parents[parents[id]];
      id = parents[id];
    }
    return id;
  };
  auto unify = [&](Value a, Value b) {
    if (resource(a.getType()) && resource(b.getType()))
      parents[root(node(a))] = root(node(b));
  };
  for (auto module : modules) {
    module.walk([&](Operation *op) {
      for (Value value : op->getResults())
        if (resource(value.getType()))
          node(value);
      for (Region &region : op->getRegions())
        for (Block &block : region)
          for (BlockArgument arg : block.getArguments())
            if (resource(arg.getType()))
              node(arg);
      if (auto connection = dyn_cast<ac::ResourceConnectionOp>(op))
        for (Value operand : connection.getConnections())
          unify(connection.getResource(), operand);
      if (auto child = dyn_cast<ac::InstanceOp>(op)) {
        auto target = cast<ac::ModuleOp>(lookup(child.getCallee()));
        for (auto [a, b] : llvm::zip(child.getOperands(),
                                     target.getBody().front().getArguments()))
          unify(a, b);
        auto yield =
            cast<ac::YieldOp>(target.getBody().front().getTerminator());
        for (auto [a, b] : llvm::zip(child.getResults(), yield.getValues()))
          unify(a, b);
      }
      if (auto call = dyn_cast<ac::TransactionRuleCallOp>(op)) {
        auto rule = cast<ac::TransactionRuleOp>(
            SymbolTable::lookupSymbolIn(module, call.getCallee()));
        SmallVector<Value> actual(call.getInterfaces());
        actual.append(rule.getCaptures().begin(), rule.getCaptures().end());
        for (Region &region : rule->getRegions())
          if (!region.empty())
            for (auto [a, b] : llvm::zip(actual, region.front().getArguments()))
              unify(a, b);
      }
      if (auto method = dyn_cast<ac::TransactionMethodOp>(op)) {
        auto sig = method.getFunctionType();
        for (Region &region : method->getRegions())
          if (!region.empty())
            for (auto [a, b] :
                 llvm::zip(method.getCaptures(),
                           region.front().getArguments().drop_front(
                               sig.getNumInputs())))
              unify(a, b);
      }
    });
  }
  llvm::DenseMap<unsigned, Type> payloads;
  for (auto [id, value] : llvm::enumerate(values)) {
    Type data = value.getType();
    if (auto type = dyn_cast<ac::PrimitiveType>(data))
      data = type.getPayload();
    else if (auto type = dyn_cast<ac::StateType>(data))
      data = type.getPayload();
    else if (auto type = dyn_cast<ac::QueueType>(data))
      data = type.getPayload();
    unsigned group = root(id);
    auto prior = payloads.find(group);
    if (prior != payloads.end() &&
        !ac::transactionalTypesEqual(prior->second, data))
      return emitError(value.getLoc(),
                       "resource connection payload types differ");
    payloads[group] = data;
  }
  // Identity is stricter than type compatibility. A connection component may
  // have many formal aliases, but at most one actual allocation occurrence.
  llvm::DenseMap<unsigned, ac::ResourceAllocOp> allocations;
  llvm::DenseMap<Operation *, unsigned> owners;
  for (auto [id, module] : llvm::enumerate(modules))
    owners[module] = id;
  for (auto [id, value] : llvm::enumerate(values)) {
    auto allocation = value.getDefiningOp<ac::ResourceAllocOp>();
    if (!allocation)
      continue;
    unsigned group = root(id);
    auto prior = allocations.find(group);
    if (prior != allocations.end() && prior->second != allocation)
      return allocation.emitOpError(
          "resource connection joins distinct allocation identities");
    allocations[group] = allocation;
  }
  llvm::DenseMap<unsigned, Value> defaults;
  auto preferDefault = [&](unsigned group, Value value) {
    auto owner = value.getParentRegion()->getParentOfType<ac::ModuleOp>();
    auto prior = defaults.find(group);
    if (prior == defaults.end() ||
        owners.lookup(owner) <
            owners.lookup(prior->second.getParentRegion()
                              ->getParentOfType<ac::ModuleOp>()))
      defaults[group] = value;
  };
  for (auto [id, value] : llvm::enumerate(values))
    if (value.getDefiningOp<ac::ResourceConnectionOp>())
      preferDefault(root(id), value);
  // A closed root has no external owner for its bound output formals.
  auto rootModule = modules.front();
  unsigned rootInputs = rootModule.getFunctionType().getNumInputs();
  for (BlockArgument arg : rootModule.getBody().front().getArguments())
    if (arg.getArgNumber() >= rootInputs && resource(arg.getType()))
      preferDefault(root(node(arg)), arg);
  for (auto [group, endpoint] : defaults) {
    if (allocations.contains(group))
      continue;
    auto module = endpoint.getParentRegion()->getParentOfType<ac::ModuleOp>();
    OpBuilder builder(module.getContext());
    builder.setInsertionPointToStart(&module.getBody().front());
    Type data = endpoint.getType();
    if (auto type = dyn_cast<ac::PrimitiveType>(data))
      data = type.getPayload();
    else if (auto type = dyn_cast<ac::StateType>(data))
      data = type.getPayload();
    else if (auto type = dyn_cast<ac::QueueType>(data))
      data = type.getPayload();
    auto type = ac::QueueType::get(package.getContext(), data);
    std::string name = "__connection_" + std::to_string(group);
    if (auto connection = endpoint.getDefiningOp<ac::ResourceConnectionOp>())
      name = connection.getInstanceName().str();
    OperationState state(endpoint.getLoc(), "ac.resource.alloc");
    state.addTypes(type);
    state.addAttribute("instance_name", builder.getStringAttr(name));
    state.addAttribute("depth", builder.getI64IntegerAttr(2));
    auto allocation = cast<ac::ResourceAllocOp>(builder.create(state));
    allocations[group] = allocation;
    // Keep the new allocation in the same identity component.
    unify(endpoint, allocation.getResource());
  }
  llvm::DenseMap<unsigned, Type> concrete;
  for (auto [id, value] : llvm::enumerate(values)) {
    Type type = value.getType();
    if (isa<ac::PrimitiveType>(type))
      continue;
    unsigned group = root(id);
    auto found = concrete.find(group);
    if (found != concrete.end() &&
        !ac::transactionalTypesEqual(found->second, type))
      return emitError(value.getLoc(), "incompatible resource bindings");
    concrete[group] = type;
  }
  for (auto [id, value] : llvm::enumerate(values)) {
    auto found = concrete.find(root(id));
    if (found == concrete.end())
      return emitError(value.getLoc(),
                       "resource interface has no concrete binding");
    value.setType(found->second);
  }
  // Remove deferred endpoints using a representative in the same Module.
  // Cross-Module values remain connected through instance operands/results.
  SmallVector<ac::ResourceConnectionOp> retiredConnections;
  for (auto module : modules) {
    SmallVector<ac::ResourceConnectionOp> connections(
        module.getBody().front().getOps<ac::ResourceConnectionOp>());
    for (auto connection : connections) {
      Value endpoint = connection.getResource();
      unsigned group = root(nodes.lookup(endpoint));
      Value replacement;
      for (Value candidate : values) {
        if (root(nodes.lookup(candidate)) != group ||
            candidate.getDefiningOp<ac::ResourceConnectionOp>())
          continue;
        if (candidate.getParentRegion() != &module.getBody())
          continue;
        // An instance result can depend on the endpoint via a bound input.
        // Prefer its Module formal or actual allocation instead.
        if (isa<BlockArgument>(candidate) ||
            candidate.getDefiningOp<ac::ResourceAllocOp>()) {
          replacement = candidate;
          break;
        }
        if (!replacement)
          replacement = candidate;
      }
      if (!replacement)
        return connection.emitOpError("resource connection has no local owner");
      endpoint.replaceAllUsesWith(replacement);
    }
    retiredConnections.append(connections);
  }
  for (auto connection : retiredConnections)
    connection.erase();
  // Root bound outputs become exports of their now-resolved local allocation.
  for (unsigned index = rootModule.getBody().front().getNumArguments();
       index > rootInputs; --index) {
    auto argument = rootModule.getBody().front().getArgument(index - 1);
    unsigned group = root(nodes.lookup(argument));
    Value replacement;
    for (auto allocation :
         rootModule.getBody().front().getOps<ac::ResourceAllocOp>())
      if (root(nodes.lookup(allocation.getResource())) == group)
        replacement = allocation.getResource();
    if (!replacement)
      return rootModule.emitOpError(
          "closed root output has no local allocation");
    argument.replaceAllUsesWith(replacement);
    rootModule.getBody().front().eraseArgument(index - 1);
  }
  SmallVector<Attribute> rootKinds(
      rootModule.getFunctionType().getNumResults(),
      StringAttr::get(package.getContext(), "export"));
  rootModule->setAttr("output_kinds",
                      ArrayAttr::get(package.getContext(), rootKinds));
  for (auto module : modules) {
    module.walk([&](Operation *op) {
      if (auto owner = dyn_cast<ac::ModuleOp>(op)) {
        auto original = owner.getFunctionType();
        auto yield = cast<ac::YieldOp>(owner.getBody().front().getTerminator());
        SmallVector<Type> args(owner.getBody().front().getArgumentTypes());
        owner->setAttr(
            "function_type",
            TypeAttr::get(FunctionType::get(
                package.getContext(),
                ArrayRef<Type>(args).take_front(original.getNumInputs()),
                yield.getOperandTypes())));
      } else if (auto rule = dyn_cast<ac::TransactionRuleOp>(op)) {
        auto original = rule.getFunctionType();
        SmallVector<Type> args(rule.getBody().front().getArgumentTypes());
        rule->setAttr(
            "function_type",
            TypeAttr::get(FunctionType::get(
                package.getContext(),
                ArrayRef<Type>(args).take_front(original.getNumInputs()),
                ArrayRef<Type>(args)
                    .drop_front(original.getNumInputs())
                    .take_front(original.getNumResults()))));
      }
      if (auto call = dyn_cast<ac::TransactionMethodCallOp>(op)) {
        if (isa<ac::QueueType>(call.getReceiver().getType())) {
          auto name = call.getCalleeAttr().getRootReference().getValue();
          if (name == "read" || name == "write") {
            StringRef resolved = name == "read" ? "pop" : "push";
            if (name == "read" && call->hasAttr("rule_parameter")) {
              auto rule = call->getParentOfType<ac::TransactionRuleOp>();
              Region *region = call->getParentRegion();
              while (region->getParentOp() != rule)
                region = region->getParentRegion();
              if (region == &rule.getGuard() || call->hasAttr("ac.parameter_join"))
                resolved = "head";
            }
            call->setAttr("callee", FlatSymbolRefAttr::get(
                                        package.getContext(),
                                        resolved));
          }
        }
      }
    });
  }
  return success();
}

} // namespace acir::ac

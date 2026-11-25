#include "pycircuit/Dialect/GFIR/StructuredProgram.h"
#include "Dialect/ACIR/HardwareValueAvailability.h"
#include "Dialect/GFIR/StructuredClocked.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringSet.h"
#include <algorithm>
#include <iterator>
#include <limits>

using namespace mlir;

namespace acir::gfir {
namespace detail {

LogicalResult verifyAttributes(Operation *op, ArrayRef<StringRef> allowed) {
  for (NamedAttribute attr : op->getAttrs())
    if (!llvm::is_contained(allowed, attr.getName().getValue()))
      return op->emitOpError() << "unsupported structured GFIR attribute "
                               << attr.getName();
  return success();
}

bool isCombinational(Operation *op) {
  return ac::detail::isStrictBitsOperation(op) ||
         isa<ac::BitsConstantOp, ac::BitsConstant4Op, ac::BitsSelectOp,
             ac::StructCreateOp, ac::StructGetOp>(op);
}

bool isStructuredLeaf(Type type) {
  if (auto table = dyn_cast<ac::TableType>(type))
    return table.getShape().size() == 1 && isa<ac::BitsType>(table.getElementType());
  return isa<ac::BitsType, ac::EnumType>(type);
}

bool matchesSignature(CallOp call, FunctionType type) {
  auto equivalent = [](TypeRange actual, TypeRange formal) {
    return actual.size() == formal.size() &&
           llvm::all_of(llvm::zip(actual, formal), [](auto pair) {
             return ac::areEquivalentHardwareTypes(std::get<0>(pair), std::get<1>(pair));
           });
  };
  if (auto signature = call.getBindingTypeAttr())
    return equivalent(cast<FunctionType>(signature.getValue()).getInputs(), type.getInputs()) &&
           equivalent(call.getOutputs().getTypes(), type.getResults());
  return equivalent(call.getInputs().getTypes(), type.getInputs()) &&
         equivalent(call.getOutputs().getTypes(), type.getResults());
}

} // namespace detail
namespace {

using detail::isCombinational;
using detail::verifyAttributes;

struct InstancePath {
  SmallVector<InstanceOp, 2> instances;
  SmallVector<unsigned, 2> definitions;
};

SmallVector<StringAttr> symbolComponents(SymbolRefAttr symbol) {
  SmallVector<StringAttr> path{symbol.getRootReference()};
  for (auto nested : symbol.getNestedReferences())
    path.push_back(nested.getAttr());
  return path;
}

LogicalResult verifyRoot(RootOp root) {
  if (failed(verifyAttributes(root, {"callee", "kind", "domains", "execution_policy",
                                     "system_samples"})))
    return failure();
  if (root.getKind() == "system" && !detail::hasGeneratedDomain(root))
    return root.emitOpError("System requires the generated default domain");
  if (root.getKind() != "system" && root.getSystemSamplesAttr())
    return root.emitOpError("only a System can declare sample rows");
  if (Attribute raw = root->getAttr("domains")) {
    auto domains = dyn_cast<ArrayAttr>(raw);
    if (!domains || (!domains.empty() && !detail::hasGeneratedDomain(root) && root.getKind() != "module"))
      return root.emitOpError("structured roots support a single generated rising default domain");
  }
  if (Attribute raw = root->getAttr("execution_policy")) {
    auto policy = dyn_cast<DictionaryAttr>(raw);
    auto reset = policy ? policy.getAs<StringAttr>("reset") : StringAttr();
    if (!policy || policy.size() != 1 || !reset ||
        (reset.getValue() != "strict" && reset.getValue() != "if_one"))
      return root.emitOpError("execution policy requires reset = strict or if_one");
  }
  return success();
}

/// Topological ordering over an explicit dependency graph. Include unused
/// definitions and wires so an invalid feedback edge cannot disappear at emit.
FailureOr<SmallVector<unsigned>>
getOrder(ArrayRef<SmallVector<unsigned>> dependents,
         SmallVector<unsigned> remaining, Operation *site, StringRef cycle) {
  SmallVector<unsigned> order;
  for (auto [i, count] : llvm::enumerate(remaining))
    if (count == 0)
      order.push_back(i);
  for (unsigned head = 0; head < order.size(); ++head)
    for (unsigned next : dependents[order[head]])
      if (--remaining[next] == 0)
        order.push_back(next);
  if (order.size() != remaining.size())
    return site->emitOpError() << cycle;
  return order;
}

class Analyzer {
public:
  explicit Analyzer(mlir::ModuleOp package) : package(package), hardware(package) {}
  FailureOr<StructuredProgram> run();

private:
  llvm::StringMap<std::pair<unsigned, unsigned>> explicitInputs;
  LogicalResult verifyDefinition(ModuleOp module, bool clocked);
  FailureOr<StructuredDefinition> analyzeDefinition(ModuleOp module);
  FailureOr<llvm::DenseMap<Operation *, StructuredChildMethod>>
  bindChildMethods(StructuredDefinition &definition);
  FailureOr<InstancePath> resolveInstancePath(ModuleOp owner, ArrayRef<StringAttr> names,
                                               CallOp call);
  FailureOr<unsigned> bindStateRoute(StructuredDefinition &caller, const InstancePath &path,
                                     unsigned state, bool writes, unsigned rule, CallOp call);
  LogicalResult bindMethodFormals(StructuredDefinition &caller, StructuredChildMethod &binding,
                                 MethodOp method, const StructuredMethodEffects &effect,
                                 unsigned rule, CallOp call);
  LogicalResult analyzeAvailability(
      StructuredDefinition &definition,
      const llvm::DenseMap<Operation *, unsigned> &clockedCallNodes);
  FailureOr<SmallVector<unsigned>> orderDefinitions(ArrayRef<ModuleOp> modules);
  FailureOr<unsigned> findWire(const StructuredDefinition &definition,
                               const ac::WireEndpoint &endpoint, Operation *site);
  LogicalResult addValue(StructuredDefinition &definition, Value value);
  FailureOr<SmallVector<ac::WireEndpoint>> dependencies(ac::WireEndpoint endpoint);
  LogicalResult bindCall(CallOp call, InstanceOp receiver,
                         ModuleOp callee);
  LogicalResult describeFifoQuery(StructuredDefinition &definition,
                                  StructuredWire &wire, CallOp call,
                                  StructuredFifoUse use);
  FailureOr<std::optional<unsigned>>
  readyDependency(const StructuredDefinition &definition, const StructuredFifo &fifo,
                  std::optional<unsigned> rule);

  mlir::ModuleOp package;
  ac::HardwareAnalysis hardware;
  ac::detail::SourceCheckBudget budget{{}};
  llvm::DenseMap<Operation *, unsigned> done;
  llvm::DenseMap<Operation *, InstanceOp> receivers;
  llvm::DenseMap<Operation *, llvm::BitVector> usedResultPorts;
  llvm::DenseSet<Operation *> clockedModules;
  llvm::DenseMap<Operation *, llvm::SmallDenseSet<unsigned>> inputDemands;
  bool inputDemandsChanged = false;
  StructuredProgram result;
};

LogicalResult Analyzer::verifyDefinition(ModuleOp module, bool clocked) {
  auto body = &module.getBody().front();
  if (failed(verifyAttributes(module, {"sym_name", "function_type", "source_owner",
                                       "input_names", "output_names", "domains"})))
    return failure();
  Attribute rawDomains = module->getAttr("domains");
  auto domains = dyn_cast_if_present<ArrayAttr>(rawDomains);
  bool explicitRoot = !explicitInputs.empty() && module.getSymName() == result.root.getCalleeAttr().getValue();
  if (explicitRoot) {
    if (!domains || domains.size() != explicitInputs.size())
      return module.emitOpError("formal domains must match explicit root domains");
    llvm::StringSet<> seen;
    for (Attribute raw : domains) {
      auto name = dyn_cast<StringAttr>(raw);
      if (!name || !explicitInputs.count(name.getValue()) || !seen.insert(name.getValue()).second)
        return module.emitOpError("unknown or repeated explicit domain");
    }
    for (Operation &op : *body)
      if (isa<VarOp, MethodOp, RuleOp>(op))
        return op.emitOpError("explicit domain wrapper cannot own local transactions");
  } else if (clocked) {
    if (!domains || domains.size() != 1 ||
        domains[0] != StringAttr::get(module.getContext(), "default"))
      return module.emitOpError("clocked module requires the default formal domain");
    for (ArrayAttr names : {module.getInputNames(), module.getOutputNames()})
      if (llvm::any_of(names, [](Attribute attr) {
            auto name = cast<StringAttr>(attr).getValue();
            return name == "pyc_clk" || name == "pyc_rst";
          }))
        return module.emitOpError("root port conflicts with a generated domain port");
  } else if (rawDomains && (!domains || !domains.empty())) {
    return module.emitOpError("structured combinational module requires empty domains");
  }
  llvm::StringMap<InstanceOp> instances;
  llvm::SmallDenseSet<Operation *> bound;
  for (auto instance : body->getOps<InstanceOp>()) {
    if (failed(verifyAttributes(instance, {"sym_name", "callee", "domains",
                                          "ac.payload_constraint"})))
      return failure();
    auto *callee = hardware.lookupDefinition(instance.getCalleeAttr().getValue());
    if (result.root.getKind() == "system" &&
        instance.getCalleeAttr() == result.root.getCalleeAttr())
      return instance.emitOpError("System cannot be used as a child module");
    if (!explicitInputs.empty() && instance.getCalleeAttr() == result.root.getCalleeAttr())
      return instance.emitOpError("explicit domain wrapper cannot be a child");
    const bool native = isa_and_nonnull<PrimitiveOp>(callee);
    if (explicitRoot && native)
      return instance.emitOpError("explicit domain wrapper cannot own native resources");
    if (native) {
      bound.insert(instance);
    } else if (!isa_and_nonnull<ModuleOp>(callee)) {
      return instance.emitOpError("callee must be a defined Module or native primitive");
    } else {
      if (instance->hasAttr("ac.payload_constraint"))
        return instance.emitOpError("payload constraint requires a native resource instance");
    }
    const bool inheritsClock = native || clockedModules.contains(callee);
    if (inheritsClock && !clocked && !explicitRoot)
      return instance.emitOpError("clocked child requires the parent's default domain");
    if (explicitRoot && inheritsClock) {
      auto binding = instance->getAttrOfType<DictionaryAttr>("domains");
      auto name = binding ? binding.getAs<StringAttr>("default") : StringAttr();
      if (!binding || binding.size() != 1 || !name || !explicitInputs.count(name.getValue()))
        return instance.emitOpError("clocked child requires an explicit named domain binding");
    } else if (auto raw = instance->getAttr("domains")) {
      auto binding = dyn_cast<DictionaryAttr>(raw);
      if (!binding ||
          (inheritsClock
               ? (binding.size() != 1 ||
                  binding.getAs<StringAttr>("default") !=
                      StringAttr::get(module.getContext(), "default"))
               : !binding.empty()))
        return instance.emitOpError("unsupported structured instance domain binding");
    }
    instances.try_emplace(instance.getSymName(), instance);
  }
  for (Operation &op : *body) {
    if (isa<InstanceOp>(op) ||
        (clocked && isa<VarOp, MethodOp, RuleOp, ReadOp>(op)))
      continue;
    if (clocked && isa<ResultOp>(op)) {
      if (failed(verifyAttributes(&op, {"rule", "ordinal"})))
        return failure();
      continue;
    }
    if (isa<YieldOp>(op)) {
      if (failed(verifyAttributes(&op, {})))
        return failure();
      continue;
    }
    if (auto call = dyn_cast<CallOp>(op)) {
      if (clocked && call.getInterfaceAttr())
        continue;
      if (failed(verifyAttributes(call, {"receiver"})))
        return failure();
      auto receiverAttr = call.getReceiverAttr();
      auto receiver = receiverAttr ? instances.lookup(receiverAttr.getRootReference().getValue())
                                   : InstanceOp();
      if (!receiver)
        return call.emitOpError("receiver must be a declared local instance");
      auto callee = dyn_cast_or_null<ModuleOp>(
          hardware.lookupDefinition(receiver.getCalleeAttr().getValue()));
      if (!callee)
        return receiver.emitOpError("callee must be a defined structured module");
      if (!bound.insert(receiver).second)
        return call.emitOpError("instance already has a call binding");
      if (failed(bindCall(call, receiver, callee)))
        return failure();
      continue;
    }
    if (!isCombinational(&op) &&
        !isa<ac::TableGetOp, ac::TableSplatOp, ac::TableMapOp>(op))
      return op.emitOpError("operation is unavailable in the structured GFIR graph");
    if (failed(hardware.verifyResolvedOperation(&op, {})))
      return failure();
    if (auto map = dyn_cast<ac::TableMapOp>(op))
      for (Operation &row : map.getBody().front().without_terminator()) {
        if (!isCombinational(&row))
          return row.emitOpError("GFIR Table map rows require pure scalar operations");
        if (failed(hardware.verifyResolvedOperation(&row, {})))
          return failure();
      }
  }
  for (auto instance : body->getOps<InstanceOp>())
    if (!bound.contains(instance))
      return instance.emitOpError("instance requires a call binding");
  return success();
}

LogicalResult Analyzer::bindCall(CallOp call,
                                 InstanceOp receiver, ModuleOp callee) {
  if (!detail::matchesSignature(call, callee.getFunctionType()))
    return call.emitOpError("instance call type does not match callee signature");
  receivers.try_emplace(call, receiver);
  if (failed(budget.debit(call.getNumResults(), call)))
    return failure();
  auto &used = usedResultPorts.try_emplace(callee, call.getNumResults()).first->second;
  for (auto [ordinal, output] : llvm::enumerate(call.getOutputs()))
    if (!output.use_empty())
      used.set(ordinal);
  return success();
}

FailureOr<InstancePath>
Analyzer::resolveInstancePath(ModuleOp owner, ArrayRef<StringAttr> names, CallOp call) {
  if (failed(budget.debit(names.size(), call)))
    return failure();
  InstancePath path;
  for (auto name : names) {
    auto instance = dyn_cast_or_null<InstanceOp>(SymbolTable::lookupSymbolIn(owner, name));
    auto *callee = instance ? hardware.lookupDefinition(instance.getCalleeAttr().getValue())
                            : nullptr;
    if (!isa_and_nonnull<ModuleOp>(callee))
      return call.emitOpError("receiver or owner path requires declared ordinary instances");
    unsigned ordinal = done.lookup(callee);
    if (!result.definitions[ordinal].clocked)
      return call.emitOpError("Method state path must share the caller's default domain");
    path.instances.push_back(instance);
    path.definitions.push_back(ordinal);
    owner = cast<ModuleOp>(callee);
  }
  return path;
}

FailureOr<unsigned> Analyzer::bindStateRoute(StructuredDefinition &caller,
                                              const InstancePath &path,
                                              unsigned state, bool writes,
                                              unsigned rule, CallOp call) {
  unsigned leaf = path.definitions.back();
  StructuredStateRoute route{path.instances, leaf, state};
  if (failed(budget.debit(caller.childStates.size() + path.instances.size(), call)))
    return failure();
  auto existing = llvm::find_if(caller.childStates, [&](const auto &entry) {
    return entry.route == route;
  });
  if (existing == caller.childStates.end()) {
    caller.childStates.push_back({route, {}});
    existing = std::prev(caller.childStates.end());
  }
  if (writes) {
    if (existing->writer && *existing->writer != rule)
      return call.emitOpError("child state write overlaps another Rule");
    const auto &rules = result.definitions[leaf].clocked->rules;
    if (llvm::any_of(rules, [&](const StructuredRule &entry) {
          return llvm::is_contained(entry.writes, state);
        }))
      return call.emitOpError("child Method state overlaps a local Rule writer");
    existing->writer = rule;
  }
  for (unsigned ordinal : path.definitions) {
    auto &owner = result.definitions[ordinal];
    route.path.erase(route.path.begin());
    if (writes && llvm::any_of(owner.childStates, [&](const auto &entry) {
          return entry.route == route && entry.writer;
        }))
      return call.emitOpError("child Method state overlaps a descendant Rule writer");
    auto incoming = llvm::find_if(owner.incomingStates, [&](const auto &entry) {
      return entry.route == route;
    });
    if (incoming == owner.incomingStates.end())
      owner.incomingStates.push_back({route, writes});
    else
      incoming->writable |= writes;
  }
  return existing - caller.childStates.begin();
}

LogicalResult Analyzer::bindMethodFormals(StructuredDefinition &caller,
                                           StructuredChildMethod &binding,
                                           MethodOp method,
                                           const StructuredMethodEffects &effect,
                                           unsigned rule, CallOp call) {
  auto writable = method.getWritableAttr();
  auto bindings = call.getBindingsAttr();
  if ((writable ? writable.size() : 0) != (bindings ? bindings.size() : 0))
    return call.emitOpError("all and only writable Method formals require bindings");
  if (!bindings)
    return success();
  llvm::SmallDenseSet<unsigned> formalOrdinals;
  for (auto row : writable)
    formalOrdinals.insert(cast<IntegerAttr>(row.getValue()).getValue().getZExtValue());
  struct Place {
    StructuredStateRoute route;
    uint64_t low, high;
  };
  SmallVector<Place> places;
  for (unsigned state : effect.states)
    places.push_back({{binding.path, binding.definition, state},
                      0, std::numeric_limits<uint64_t>::max()});
  for (auto raw : bindings) {
    auto row = cast<DictionaryAttr>(raw);
    unsigned formal = row.getAs<IntegerAttr>("ordinal").getValue().getZExtValue();
    if (!formalOrdinals.contains(formal))
      return call.emitOpError("binding ordinal must refer to a writable Method formal");
    auto names = symbolComponents(row.getAs<SymbolRefAttr>("owner"));
    auto varName = names.pop_back_val();
    auto path = resolveInstancePath(caller.module, names, call);
    if (failed(path))
      return failure();
    bool local = path->instances.empty();
    unsigned owner = local ? result.definitions.size() : path->definitions.back();
    auto ownerModule = local ? caller.module : result.definitions[owner].module;
    auto declared = ownerModule.getBody().front().getOps<VarOp>();
    auto var = llvm::find_if(declared, [&](VarOp var) { return var.getSymNameAttr() == varName; });
    if (var == declared.end())
      return call.emitOpError("bound owner must resolve to an actual gfir.var");
    auto variable = *var;
    unsigned state = std::distance(declared.begin(), var);
    Type actual = variable.getInitial().getType();
    StructuredFormalBinding bound{formal, state, local,
                                  llvm::is_contained(effect.formalWrites, formal)};
    if (auto fields = row.getAs<ArrayAttr>("fields")) {
      auto projection = detail::describeStateFields(hardware, budget, actual, fields, call);
      if (failed(projection))
        return failure();
      actual = projection->types.back();
      bound.projection = std::move(*projection);
    }
    auto width = hardware.getPackedWidth(actual, {}, call);
    if (failed(width))
      return failure();
    uint64_t low = bound.projection ? bound.projection->low : 0;
    if (auto rowIndex = row.getAs<IntegerAttr>("index_operand")) {
      auto table = dyn_cast<ac::TableType>(actual);
      if (!table || table.getShape().size() != 1 ||
          !isa<ac::BitsType>(table.getElementType()))
        return call.emitOpError("indexed writable binding requires a rank-one Bits Table");
      bound.indexOperand = rowIndex.getValue().getZExtValue();
      if (failed(hardware.verifySourceIndex(call.getInputs()[*bound.indexOperand],
                                            table, {}, call)))
        return failure();
      bound.table = table;
      actual = table.getElementType();
    }
    if (!ac::areEquivalentHardwareTypes(actual,
                                        method.getFunctionType().getInput(formal)))
      return call.emitOpError("writable binding payload must match the nominal formal type");
    StructuredStateRoute place{path->instances, owner, state};
    // An unknown or dynamic index may touch every row of the selected table.
    if (llvm::any_of(places, [&](const auto &other) {
          return other.route == place && low < other.high && other.low < low + *width;
        }))
      return call.emitOpError("writable formals overlap another actual owner of this invocation");
    places.push_back({std::move(place), low, low + *width});
    if (!local) {
      auto slot = bindStateRoute(caller, *path, state, bound.writes, rule, call);
      if (failed(slot))
        return failure();
      bound.slot = *slot;
    }
    binding.formals.push_back(std::move(bound));
  }
  return success();
}

FailureOr<llvm::DenseMap<Operation *, StructuredChildMethod>>
Analyzer::bindChildMethods(StructuredDefinition &definition) {
  llvm::DenseMap<Operation *, StructuredChildMethod> calls;
  auto module = definition.module;
  auto &body = module.getBody().front();
  for (auto [index, rule] : llvm::enumerate(body.getOps<RuleOp>())) {
    auto &ordered = rule.getBody().front();
    bool usesChild = false;
    for (auto call : ordered.getOps<CallOp>()) {
      auto receiver = call.getReceiverAttr();
      if (!receiver)
        continue;
      auto first = dyn_cast_or_null<InstanceOp>(
          SymbolTable::lookupSymbolIn(module, receiver.getRootReference()));
      if (!first)
        continue;
      auto *firstDefinition = hardware.lookupDefinition(first.getCalleeAttr().getValue());
      if (receiver.getNestedReferences().empty() && isa_and_nonnull<PrimitiveOp>(firstDefinition))
        continue;
      if (!call.getInterfaceAttr())
        return call.emitOpError("ordered instance call requires a Method name");
      auto path = resolveInstancePath(module, symbolComponents(receiver), call);
      if (failed(path))
        return failure();
      unsigned childIndex = path->definitions.back();
      const auto &child = result.definitions[childIndex].clocked;
      if (failed(budget.debit(child->methods.size(), call)))
        return failure();
      auto found = llvm::find_if(child->methods, [&](MethodOp method) {
        return method.getSymName() == call.getInterfaceAttr().getValue();
      });
      if (found == child->methods.end())
        return call.emitOpError("interface must resolve to a Method on the actual receiver");
      MethodOp method = *found;
      if (!detail::matchesSignature(call, method.getFunctionType()))
        return call.emitOpError("child Method operands and results must match its signature");
      unsigned methodIndex = found - child->methods.begin();
      auto effects = child->directMethodEffects.find(methodIndex);
      if (effects == child->directMethodEffects.end())
        return call.emitOpError("child Method requires a bounded guard-free chain of state and value operations");
      const auto &effect = effects->second;
      if (failed(budget.debit(effect.expandedWork, call)))
        return failure();
      StructuredChildMethod binding{path->instances, childIndex, methodIndex, {}, {}};
      for (unsigned state : effect.states) {
        const bool writes = llvm::is_contained(effect.writes, state);
        auto slot = bindStateRoute(definition, *path, state, writes, index, call);
        if (failed(slot))
          return failure();
        binding.states.emplace_back(state, *slot);
      }
      if (failed(bindMethodFormals(definition, binding, method, effect, index, call)))
        return failure();
      calls.try_emplace(call, std::move(binding));
      usesChild = true;
    }
    if (usesChild)
      for (Operation &op : ordered)
        if (isa<IfOp, LoopOp>(op) || (isa<CallOp>(op) && !calls.contains(&op)))
          return op.emitOpError("child Method caller requires a direct ordered body");
  }
  return calls;
}

FailureOr<SmallVector<unsigned>>
Analyzer::orderDefinitions(ArrayRef<ModuleOp> modules) {
  llvm::DenseMap<Operation *, unsigned> indices;
  SmallVector<SmallVector<unsigned>> dependents(modules.size());
  SmallVector<unsigned> remaining(modules.size(), 0);
  for (auto [index, module] : llvm::enumerate(modules))
    indices.try_emplace(module, index);
  for (unsigned index = 0; index < modules.size(); ++index) {
    ModuleOp module = modules[index];
    for (auto instance : module.getBody().front().getOps<InstanceOp>()) {
      auto found = indices.find(hardware.lookupDefinition(instance.getCalleeAttr().getValue()));
      if (found == indices.end())
        continue;
      unsigned dependency = found->second;
      dependents[dependency].push_back(index);
      ++remaining[index];
    }
  }
  return getOrder(dependents, std::move(remaining), package,
                  "recursive structured GFIR hierarchy");
}

LogicalResult Analyzer::addValue(StructuredDefinition &definition, Value value) {
  auto *site = value.getParentRegion()->getParentOp();
  if (!isa<ac::BitsType, ac::StructType, ac::EnumType, ac::TableType>(value.getType()))
    return site->emitOpError("structured combinational value requires a closed hardware type");
  auto leaves = hardware.getLeaves(value.getType(), {}, site);
  if (failed(leaves) || leaves->empty())
    return failure();
  auto &ids = definition.valueWires[value];
  for (const auto &leaf : *leaves) {
    if (!detail::isStructuredLeaf(leaf.type) || leaf.width == 0 ||
        leaf.width > std::numeric_limits<unsigned>::max() - leaf.low)
      return site->emitOpError("structured value has an unsupported packed leaf");
    if (failed(budget.debit(1 + leaf.path.size(), site)))
      return failure();
    ids.push_back(definition.wires.size());
    definition.wires.push_back(
        {{value, leaf.path}, unsigned(leaf.low), unsigned(leaf.width), leaf.type,
         {}, {}, {}, {}, {}, {}});
  }
  return success();
}

FailureOr<SmallVector<ac::WireEndpoint>>
Analyzer::dependencies(ac::WireEndpoint endpoint) {
  auto debit = [&](uint64_t units, Operation *op) { return budget.debit(units, op); };
  auto edges = hardware.getDependencies(endpoint.value, endpoint.path, {}, debit);
  auto map = endpoint.value.getDefiningOp<ac::TableMapOp>();
  if (failed(edges) || !map)
    return edges;
  // The shared dependency query follows TableMap results to the row SSA.
  // Walk those endpoints until reaching the enclosing module graph again.
  SmallVector<ac::WireEndpoint> result;
  llvm::DenseMap<Value, SmallVector<ac::FieldPath>> visited;
  while (!edges->empty()) {
    auto edge = edges->pop_back_val();
    auto &paths = visited[edge.value];
    if (failed(budget.debit(1 + paths.size() + edge.path.size(), map)))
      return failure();
    if (llvm::is_contained(paths, edge.path))
      continue;
    paths.push_back(edge.path);
    if (edge.value.getParentBlock() == map->getBlock()) {
      result.push_back(std::move(edge));
      continue;
    }
    auto nested = hardware.getDependencies(edge.value, edge.path, {}, debit);
    if (failed(nested))
      return failure();
    edges->append(nested->begin(), nested->end());
  }
  return result;
}

FailureOr<unsigned> Analyzer::findWire(const StructuredDefinition &definition,
                                        const ac::WireEndpoint &endpoint,
                                        Operation *site) {
  auto where = definition.valueWires.find(endpoint.value);
  if (where != definition.valueWires.end()) {
    if (failed(budget.debit(where->second.size(), site)))
      return failure();
    for (unsigned index : where->second)
      if (definition.wires[index].endpoint.path == endpoint.path)
        return index;
  }
  return site->emitOpError("unresolved field dependency in structured GFIR");
}

FailureOr<std::optional<unsigned>>
Analyzer::readyDependency(const StructuredDefinition &definition,
                           const StructuredFifo &fifo,
                           std::optional<unsigned> rule) {
  if (!fifo.downstreamPop)
    return std::optional<unsigned>();
  if (fifo.rawOutput) {
    auto call = fifo.rawOutput;
    auto wire = findWire(definition, {call.getInputs().front(), {}}, call);
    if (failed(wire))
      return failure();
    return std::optional<unsigned>(*wire);
  }
  if (fifo.consumer && fifo.consumer != rule)
    return std::optional<unsigned>(definition.wires.size() + *fifo.consumer);
  return std::optional<unsigned>();
}

LogicalResult Analyzer::describeFifoQuery(StructuredDefinition &definition,
                                           StructuredWire &wire, CallOp call,
                                           StructuredFifoUse use) {
  auto result = cast<OpResult>(wire.endpoint.value).getResultNumber();
  auto query = StructuredFifoQuery::Ready;
  if (use.access == StructuredFifoAccess::RawOut)
    query = result == 0 ? StructuredFifoQuery::Valid : StructuredFifoQuery::Data;
  else if (use.access == StructuredFifoAccess::Status && result != 0)
    query = result == 1 ? StructuredFifoQuery::Valid : StructuredFifoQuery::Data;
  if (query == StructuredFifoQuery::Ready) {
    auto echo = readyDependency(definition, definition.clocked->fifos[use.fifo], {});
    if (failed(echo))
      return failure();
    if (*echo)
      wire.dependencies.push_back(**echo);
  }
  wire.fifoQuery = std::pair(use.fifo, query);
  return success();
}

LogicalResult Analyzer::analyzeAvailability(
    StructuredDefinition &definition,
    const llvm::DenseMap<Operation *, unsigned> &clockedCallNodes) {
  auto *clocked = definition.clocked ? &*definition.clocked : nullptr;
  const bool hasLocalResults = clocked && llvm::any_of(clocked->rules, [](const StructuredRule &rule) {
    return rule.sampled && !rule.resultUses.empty();
  });
  auto childFor = [&](CallOp call) -> const StructuredDefinition * {
    auto receiver = receivers.lookup(call);
    if (!receiver || call.getInterfaceAttr())
      return nullptr;
    auto found = done.find(hardware.lookupDefinition(receiver.getCalleeAttr().getValue()));
    return found == done.end() ? nullptr : &result.definitions[found->second];
  };
  const bool hasChildResults = llvm::any_of(definition.module.getBody().front().getOps<CallOp>(), [&](auto call) {
    auto *child = childFor(call);
    return child && !child->outputAvailability.empty();
  });
  auto demands = inputDemands.find(definition.module);
  if (demands != inputDemands.end()) {
    llvm::StringSet<> names;
    for (auto list : {definition.module.getInputNames(), definition.module.getOutputNames()})
      for (auto name : list)
        names.insert(cast<StringAttr>(name).getValue());
    for (unsigned port = 0; port < definition.module.getFunctionType().getNumInputs(); ++port) {
      if (!demands->second.contains(port))
        continue;
      std::string name = "__pyc_valid_in_" + std::to_string(port);
      while (!names.insert(name).second)
        name += "_";
      definition.inputAvailability.push_back({port, StringAttr::get(package.getContext(), name)});
    }
  }
  if (!hasLocalResults && !hasChildResults && definition.inputAvailability.empty())
    return success();
  definition.fourStateAvailability = llvm::any_of(definition.module.getBody().front().getOps<CallOp>(), [&](auto call) {
    auto *child = childFor(call);
    return child && child->fourStateAvailability;
  });
  definition.fourStateAvailability |= !definition.inputAvailability.empty();

  using Source = StructuredAvailabilitySource;
  SmallVector<StructuredAvailability> wireSources(definition.wires.size());
  auto merge = [&](StructuredAvailability &into, const StructuredAvailability &from,
                   Operation *site) -> LogicalResult {
    if (from.empty())
      return success();
    if (failed(budget.debit(into.size() + from.size(), site)))
      return failure();
    StructuredAvailability combined;
    std::set_union(into.begin(), into.end(), from.begin(), from.end(),
                   std::back_inserter(combined));
    into = std::move(combined);
    return success();
  };
  for (unsigned index : definition.order) {
    if (index >= definition.wires.size()) {
      if (index < definition.firstClockedCallNode()) {
        auto &rule = clocked->rules[index - definition.wires.size()];
        for (Value capture : rule.operation.getCaptures())
          if (llvm::any_of(definition.valueWires.find(capture)->second, [&](unsigned leaf) {
                return !wireSources[leaf].empty();
              })) {
            rule.sampled = true;
            rule.afterSnapshot = nullptr;
          }
      }
      continue;
    }
    const auto &wire = definition.wires[index];
    if (auto argument = dyn_cast<BlockArgument>(wire.endpoint.value)) {
      if (llvm::any_of(definition.inputAvailability, [&](const auto &input) {
            return input.port == argument.getArgNumber();
          }))
        wireSources[index].push_back({Source::Kind::Input, argument.getArgNumber()});
      continue;
    }
    if (wire.ruleResult) {
      unsigned rule = wire.ruleResult->first;
      if (clocked->rules[rule].sampled)
        wireSources[index].push_back({Source::Kind::Rule, rule});
      continue;
    }
    auto *site = wire.endpoint.value.getDefiningOp();
    if (auto select = dyn_cast_or_null<ac::BitsSelectOp>(site)) {
      auto condition = findWire(definition, {select.getCondition(), {}}, select);
      auto yes = findWire(definition, {select.getTrueValue(), {}}, select);
      auto no = findWire(definition, {select.getFalseValue(), {}}, select);
      if (failed(condition) || failed(yes) || failed(no))
        return failure();
      if (failed(merge(wireSources[index], wireSources[*condition], select)))
        return failure();
      if (wireSources[*yes] == wireSources[*no]) {
        if (failed(merge(wireSources[index], wireSources[*yes], select)))
          return failure();
      } else {
        if (failed(budget.debit(1 + wireSources[*yes].size() + wireSources[*no].size(), select)))
          return failure();
        unsigned expression = definition.availabilitySelects.size();
        definition.availabilitySelects.push_back({index, *condition, wireSources[*yes], wireSources[*no]});
        definition.fourStateAvailability = true;
        StructuredAvailability selected{{Source::Kind::Select, expression}};
        if (failed(merge(wireSources[index], selected, select)))
          return failure();
      }
      continue;
    }
    if (wire.call) {
      const auto &child = result.definitions[wire.call->definition];
      const auto &output = child.outputs[wire.call->output];
      auto addChild = [&](Source::Kind kind, unsigned resultPort) {
        if (!child.clocked) {
          if (llvm::any_of(wire.dependencies, [&](unsigned input) {
                return !wireSources[input].empty();
              }))
            wireSources[index].push_back({Source::Kind::PureOutput, index});
          return;
        }
        unsigned call = clockedCallNodes.lookup(site) - definition.firstClockedCallNode();
        wireSources[index].push_back({kind, call, resultPort});
      };
      if (auto field = output.fieldOutput) {
        if (!child.fieldOutputs[*field].sources.empty())
          addChild(Source::Kind::ChildField, *field);
      } else {
        if (failed(budget.debit(child.outputAvailability.size(), site)))
          return failure();
        if (llvm::any_of(child.outputAvailability, [&](const auto &available) {
              return available.port == output.port.port;
            }))
          addChild(Source::Kind::ChildOutput, output.port.port);
      }
    }
    for (unsigned dep : wire.dependencies) {
      if (dep >= wireSources.size() || wireSources[dep].empty())
        continue;
      if (wire.call)
        continue;
      if (!isa<ac::StructCreateOp, ac::StructGetOp>(site) &&
          !ac::detail::isStrictBitsOperation(site))
        return site->emitOpError("gated Rule results require a supported graph operation");
      if (failed(merge(wireSources[index], wireSources[dep], site)))
        return failure();
    }
  }

  auto module = definition.module;
  auto &body = module.getBody().front();
  auto checkInputs = [&](Operation *site, ValueRange inputs) -> LogicalResult {
    for (Value input : inputs) {
      const auto &leaves = definition.valueWires.find(input)->second;
      if (llvm::any_of(leaves, [&](unsigned wire) { return !wireSources[wire].empty(); }))
        return site->emitOpError("gated Rule results require consumer admission here");
    }
    return success();
  };
  for (auto call : body.getOps<CallOp>()) {
    if (call.getInterfaceAttr()) {
      if (failed(checkInputs(call, call.getInputs())))
        return failure();
      continue;
    }
    auto receiver = receivers.lookup(call);
    auto callee = receiver ? dyn_cast_or_null<ModuleOp>(hardware.lookupDefinition(
                                 receiver.getCalleeAttr().getValue())) : ModuleOp();
    for (auto [port, actual] : llvm::enumerate(call.getInputs())) {
      const auto &leaves = definition.valueWires.find(actual)->second;
      if (llvm::none_of(leaves, [&](unsigned leaf) { return !wireSources[leaf].empty(); }))
        continue;
      if (!callee || leaves.size() != 1 ||
          !isa<ac::BitsType>(actual.getType()))
        return call.emitOpError("available Module input currently requires a scalar receiver");
      if (!clockedModules.contains(callee) &&
          llvm::any_of(callee.getFunctionType().getResults(), [](Type type) {
            return !isa<ac::BitsType>(type);
          }))
        return call.emitOpError("available pure Module currently requires scalar results");
      inputDemandsChanged |= inputDemands[callee].insert(port).second;
    }
  }
  if (clocked) for (auto &consumer : clocked->rules) {
    auto rule = consumer.operation;
    auto &ordered = rule.getBody().front();
    for (auto [index, capture] : llvm::enumerate(rule.getCaptures())) {
      const auto &leaves = definition.valueWires.find(capture)->second;
      if (llvm::none_of(leaves, [&](unsigned wire) { return !wireSources[wire].empty(); }))
        continue;
      if (leaves.size() != 1 || !isa<ac::BitsType>(capture.getType()))
        return rule.emitOpError("available capture currently requires scalar Bits");
      auto argument = ordered.getArgument(index);
      if (argument.use_empty())
        return rule.emitOpError("gated capture requires a direct guard use");
      for (Operation *user : argument.getUsers()) {
        if (failed(budget.debit(1, user)))
          return failure();
        if (isa<GuardOp>(user) && user->getParentOp() == rule &&
            definition.wires[leaves.front()].width == 1) {
          clocked->guardedCaptureUses.try_emplace(user, index);
        } else if (auto write = dyn_cast<WriteOp>(user);
                   write && write.getValue() == argument && !write.getIndex() &&
                   write->getParentOp() == rule && !clocked->writePaths.contains(write)) {
          // Demand is registered at the consuming statement below.
        } else if (auto yield = dyn_cast<YieldOp>(user); yield && yield->getParentOp() == rule) {
          // A pure computation before this return does not demand the value.
        } else if (ac::detail::isStrictBitsOperation(user) && user->getParentOp() == rule) {
        } else {
          return user->emitOpError("available capture requires a direct guard, state write or return");
        }
      }
      const auto &sources = wireSources[leaves.front()];
      if (failed(budget.debit(sources.size(), rule)))
        return failure();
      consumer.guardCaptures.push_back({unsigned(index), sources});
    }
    llvm::DenseMap<Value, SmallVector<unsigned>> required;
    llvm::DenseMap<unsigned, SmallVector<unsigned>> candidateRequired;
    for (const auto &capture : consumer.guardCaptures)
      required[ordered.getArgument(capture.capture)].push_back(capture.capture);
    for (Operation &op : ordered) {
      if (auto read = dyn_cast<ReadOp>(op)) {
        required[read.getValue()] = candidateRequired.lookup(clocked->stateAccesses.lookup(read));
        continue;
      }
      SmallVector<unsigned> inputs;
      for (Value operand : op.getOperands()) {
        const auto &sources = required[operand];
        if (failed(budget.debit(inputs.size() + sources.size(), &op))) return failure();
        inputs.append(sources);
      }
      llvm::sort(inputs);
      inputs.erase(std::unique(inputs.begin(), inputs.end()), inputs.end());
      if (ac::detail::isStrictBitsOperation(&op)) {
        for (Value value : op.getResults()) required[value] = inputs;
      } else if (auto write = dyn_cast<WriteOp>(op)) {
        if (!inputs.empty() && (write.getIndex() || clocked->writePaths.contains(write)))
          return write.emitOpError("available writes currently require a whole scalar state");
        candidateRequired[clocked->stateAccesses.lookup(write)] = inputs;
        if (!inputs.empty())
          clocked->dataCaptureUses[write].push_back({inputs, 1, false});
      } else if (auto yield = dyn_cast<YieldOp>(op)) {
        for (auto [ordinal, value] : llvm::enumerate(yield.getValues()))
          if (!required[value].empty())
            clocked->dataCaptureUses[yield].push_back({required[value], unsigned(ordinal), true});
      } else if (!inputs.empty() && !clocked->guardedCaptureUses.contains(&op)) {
        return op.emitOpError("available local values require a strict computation, whole state write or return");
      }
    }
    if (!consumer.guardCaptures.empty())
      for (Operation &op : ordered)
        if (isa<CallOp, IfOp, LoopOp>(op))
          return op.emitOpError("gated guard consumer requires a direct ordered body");
  }

  SmallVector<StructuredAvailability> ports(module.getFunctionType().getNumResults());
  SmallVector<std::optional<unsigned>> firstLeaves(ports.size());
  llvm::BitVector mixedPorts(ports.size());
  for (const auto &output : definition.outputs) {
    unsigned port = output.port.port;
    const auto &sources = wireSources[output.wire];
    if (!firstLeaves[port]) {
      firstLeaves[port] = output.wire;
    } else {
      const auto &first = wireSources[*firstLeaves[port]];
      if (failed(budget.debit(first.size() + sources.size(), module)))
        return failure();
      if (first != sources)
        mixedPorts.set(port);
    }
    if (failed(merge(ports[port], sources, module)))
      return failure();
  }
  llvm::StringSet<> names;
  for (auto list : {module.getInputNames(), module.getOutputNames()})
    for (auto name : list)
      names.insert(cast<StringAttr>(name).getValue());
  auto uniqueName = [&](std::string name) {
    while (!names.insert(name).second)
      name += "_";
    return StringAttr::get(module.getContext(), name);
  };
  for (auto [port, sources] : llvm::enumerate(ports)) {
    if (sources.empty())
      continue;
    definition.outputAvailability.push_back(
        {unsigned(port), uniqueName("__pyc_valid_out_" + std::to_string(port)),
         std::move(sources)});
  }
  definition.wireAvailability = wireSources;
  auto used = usedResultPorts.find(module);
  if (used == usedResultPorts.end() || !mixedPorts.any())
    return success();
  for (auto [index, output] : llvm::enumerate(definition.outputs)) {
    unsigned port = output.port.port;
    if (!mixedPorts.test(port) || !used->second.test(port))
      continue;
    const auto &sources = wireSources[output.wire];
    if (failed(budget.debit(1 + output.port.path.size() + sources.size(), module)))
      return failure();
    output.fieldOutput = definition.fieldOutputs.size();
    StringAttr available;
    if (!sources.empty())
      available = uniqueName("__pyc_valid_field_" + std::to_string(index));
    definition.fieldOutputs.push_back(
        {unsigned(index), uniqueName("__pyc_field_out_" + std::to_string(index)),
         available, sources});
  }
  return success();
}

FailureOr<StructuredDefinition> Analyzer::analyzeDefinition(ModuleOp module) {
  StructuredDefinition definition;
  definition.module = module;
  definition.explicitDomains = !explicitInputs.empty() && module.getSymName() == result.root.getCalleeAttr().getValue();
  auto &body = module.getBody().front();
  llvm::StringMap<unsigned> rules;
  if (clockedModules.contains(module)) {
    auto childMethods = bindChildMethods(definition);
    if (failed(childMethods))
      return failure();
    auto plan = detail::analyzeClocked(module, hardware, budget, std::move(*childMethods),
                                       definition.childStates);
    if (failed(plan))
      return failure();
    definition.clocked = std::move(*plan);
    for (const auto &rule : definition.clocked->rules) {
      if (!rule.assertions.empty() &&
          (module.getSymName() != result.root.getCalleeAttr().getValue() ||
           !detail::hasGeneratedDomain(result.root)))
        return rule.operation->emitOpError("assertions require the selected generated-default-domain root");
      if (!rule.observations.empty() &&
          (module.getSymName() != result.root.getCalleeAttr().getValue() ||
           result.root.getKind() != "system"))
        return rule.operation->emitOpError("observations require the selected closed System root");
    }
    for (auto [index, rule] : llvm::enumerate(definition.clocked->rules))
      rules.try_emplace(rule.operation.getSymName(), index);
    for (auto &rule : definition.clocked->rules) {
      if (rule.resultUses.empty() || rule.writes.empty() || rule.sampled ||
          !rule.requirements.empty())
        continue;
      // Captured values and saved SSA results remain immutable. Work can
      // freeze the yield operands before evaluating later proposals.
      auto &ordered = rule.operation.getBody().front();
      auto yield = cast<YieldOp>(ordered.getTerminator());
      llvm::SmallDenseSet<Operation *> producers;
      for (Value result : yield.getValues())
        if (auto *producer = result.getDefiningOp())
          producers.insert(producer);
      Operation *cut = &ordered.front();
      for (Operation &op : llvm::reverse(ordered.without_terminator()))
        if (producers.contains(&op)) {
          cut = op.getNextNode();
          break;
        }
      if (cut != yield.getOperation())
        rule.afterSnapshot = cut;
    }
  }
  for (Value value : body.getArguments())
    if (failed(addValue(definition, value)))
      return failure();
  for (Operation &op : body) {
    if (isa<ac::TableSplatOp>(op) &&
        llvm::all_of(op.getResult(0).getUsers(),
                     [](Operation *use) { return isa<VarOp>(use); }))
      continue;
    for (Value value : op.getResults())
      if (failed(addValue(definition, value)))
        return failure();
  }

  auto childDefinition = [&](InstanceOp instance) -> unsigned {
    return done.lookup(hardware.lookupDefinition(instance.getCalleeAttr().getValue()));
  };
  auto parentInput = [&](CallOp call, const StructuredDefinition &child,
                         unsigned input) -> ac::WireEndpoint {
    const auto &source = child.wires[input].endpoint;
    auto arg = cast<BlockArgument>(source.value).getArgNumber();
    return {call.getInputs()[arg], source.path};
  };
  llvm::DenseMap<Operation *, unsigned> clockedCallNodes;
  for (auto call : body.getOps<CallOp>()) {
    if (call.getInterfaceAttr())
      continue;
    auto receiver = receivers.lookup(call);
    unsigned target = childDefinition(receiver);
    const auto &child = result.definitions[target];
    if (!child.outputAvailability.empty() && child.clocked && !definition.clocked)
      return call.emitOpError("gated child outputs require a clocked caller");
    if (!child.clocked)
      continue;
    definition.sourceCheckCount = ac::detail::SourceCheckBudget::add(
        definition.sourceCheckCount, child.sourceCheckCount);
    auto &prepare = definition.clockedCalls.emplace_back(
        StructuredClockedCall{call, receiver, target, {}});
    clockedCallNodes.try_emplace(
        call, definition.firstClockedCallNode() + definition.clockedCalls.size() - 1);
    if (definition.explicitDomains) {
      auto name = receiver->getAttrOfType<DictionaryAttr>("domains").getAs<StringAttr>("default");
      auto [clock, reset] = explicitInputs.lookup(name.getValue());
      auto clockWire = findWire(definition, {body.getArgument(clock), {}}, call);
      auto resetWire = findWire(definition, {body.getArgument(reset), {}}, call);
      if (failed(clockWire) || failed(resetWire))
        return failure();
      prepare.domainInputs = std::pair<unsigned, unsigned>{*clockWire, *resetWire};
      prepare.dependencies.push_back(*clockWire);
      prepare.dependencies.push_back(*resetWire);
    }
    for (unsigned input : child.preparationInputs) {
      auto parent = findWire(definition, parentInput(call, child, input), call);
      if (failed(parent))
        return failure();
      prepare.dependencies.push_back(*parent);
    }
    for (const auto &input : child.inputAvailability) {
      auto parent = findWire(definition, {call.getInputs()[input.port], {}}, call);
      if (failed(parent))
        return failure();
      prepare.dependencies.push_back(*parent);
    }
    for (const auto &state : definition.childStates)
      if (state.route.path.front() == receiver && state.writer)
        prepare.dependencies.push_back(definition.wires.size() + *state.writer);
  }
  size_t count = definition.firstClockedCallNode() + definition.clockedCalls.size();
  SmallVector<SmallVector<unsigned>> dependents(count);
  SmallVector<unsigned> remaining(count);
  for (auto [index, wire] : llvm::enumerate(definition.wires)) {
    auto endpoint = wire.endpoint;
    auto *op = endpoint.value.getDefiningOp();
    SmallVector<ac::WireEndpoint> edges;
    if (auto call = dyn_cast_or_null<CallOp>(op)) {
      if (call.getInterfaceAttr()) {
        const auto &plan = *definition.clocked;
        if (auto memory = plan.memoryCalls.find(call); memory != plan.memoryCalls.end()) {
          wire.memory = memory->second;
          if (plan.memories[*wire.memory].asynchronous())
            edges.push_back({call.getInputs().front(), {}});
        } else if (failed(describeFifoQuery(definition, wire, call,
                                            plan.fifoCalls.lookup(call)))) {
          return failure();
        }
      } else {
        auto receiver = receivers.lookup(call);
        unsigned target = childDefinition(receiver);
        const auto &child = result.definitions[target];
        unsigned port = cast<OpResult>(endpoint.value).getResultNumber();
        if (failed(budget.debit(child.outputs.size(), op)))
          return failure();
        auto match = llvm::find_if(child.outputs, [&](const StructuredOutput &output) {
          return output.port.port == port && output.port.path == endpoint.path;
        });
        if (match == child.outputs.end())
          return call.emitOpError("callee output field is unresolved");
        wire.call = StructuredCall{receiver, target, unsigned(match - child.outputs.begin())};
        if (match->requiresPreparation)
          wire.dependencies.push_back(clockedCallNodes.lookup(call));
        else
          for (unsigned input : match->inputs)
            edges.push_back(parentInput(call, child, input));
      }
    } else if (auto read = dyn_cast_or_null<ReadOp>(op)) {
      wire.state = definition.clocked->stateAccesses.lookup(read);
    } else if (auto output = dyn_cast_or_null<ResultOp>(op)) {
      unsigned rule = rules.lookup(output.getRuleAttr().getValue());
      wire.ruleResult = std::pair(rule, unsigned(output.getOrdinalAttr().getInt()));
      wire.dependencies.push_back(definition.wires.size() + rule);
    } else if (op) {
      auto computed = dependencies(endpoint);
      if (failed(computed))
        return failure();
      edges = std::move(*computed);
    }
    for (const auto &edge : edges) {
      auto dep = findWire(definition, edge, module);
      if (failed(dep))
        return failure();
      wire.dependencies.push_back(*dep);
    }
    for (unsigned dependency : wire.dependencies)
      dependents[dependency].push_back(index);
    remaining[index] = wire.dependencies.size();
  }
  if (definition.clocked) {
    for (auto [index, rule] : llvm::enumerate(definition.clocked->rules)) {
      unsigned grant = definition.wires.size() + index;
      for (Value capture : rule.operation.getCaptures()) {
        auto fields = definition.valueWires.find(capture);
        if (fields == definition.valueWires.end())
          return rule.operation.emitOpError("capture must have graph field dependencies");
        if (failed(budget.debit(fields->second.size(), rule.operation)))
          return failure();
        rule.dependencies.append(fields->second.begin(), fields->second.end());
      }
      for (const auto &requirement : rule.requirements) {
        if (!requirement.requiresReady || requirement.ownPop)
          continue;
        auto dependency = readyDependency(
            definition, definition.clocked->fifos[requirement.fifo], unsigned(index));
        if (failed(dependency))
          return failure();
        if (*dependency)
          rule.dependencies.push_back(**dependency);
      }
      llvm::sort(rule.dependencies);
      rule.dependencies.erase(std::unique(rule.dependencies.begin(),
                                           rule.dependencies.end()),
                              rule.dependencies.end());
      for (unsigned dependency : rule.dependencies)
        dependents[dependency].push_back(grant);
      remaining[grant] = rule.dependencies.size();
    }
  }
  for (auto [index, call] : llvm::enumerate(definition.clockedCalls)) {
    unsigned node = definition.firstClockedCallNode() + index;
    for (unsigned dependency : call.dependencies)
      dependents[dependency].push_back(node);
    remaining[node] = call.dependencies.size();
  }
  auto order = getOrder(dependents, std::move(remaining), module,
                        definition.clockedCalls.empty()
                            ? "structured GFIR combinational cycle in field dependencies"
                            : "structured GFIR field and child preparation dependencies cannot be scheduled");
  if (failed(order))
    return failure();
  definition.order = std::move(*order);

  // Collapse immediate dependencies to the input leaves each exported value
  // requires. Keep argument order stable across printing and reloading.
  SmallVector<SmallVector<unsigned>> inputs(count);
  llvm::BitVector prepared(count);
  for (unsigned index : definition.order) {
    const bool isWire = index < definition.wires.size();
    if (isWire && isa<BlockArgument>(definition.wires[index].endpoint.value))
      inputs[index].push_back(index);
    if (!isWire)
      prepared.set(index);
    for (unsigned dep : definition.dependencies(index)) {
      if (prepared.test(dep))
        prepared.set(index);
      SmallVector<unsigned> combined;
      if (failed(budget.debit(inputs[index].size() + inputs[dep].size(), module)))
        return failure();
      std::set_union(inputs[index].begin(), inputs[index].end(), inputs[dep].begin(),
                     inputs[dep].end(), std::back_inserter(combined));
      inputs[index] = std::move(combined);
    }
  }
  for (auto [port, value] : llvm::enumerate(cast<YieldOp>(body.getTerminator()).getValues()))
    for (unsigned index : definition.valueWires.find(value)->second) {
      if (failed(budget.debit(1 + inputs[index].size(), module)))
        return failure();
      definition.outputs.push_back(
          {{unsigned(port), definition.wires[index].endpoint.path}, index,
           inputs[index], prepared.test(index)});
    }
  if (failed(analyzeAvailability(definition, clockedCallNodes)))
    return failure();
  if (definition.clocked) {
    if (failed(detail::buildClockedChecks(module, *definition.clocked, budget)))
      return failure();
    for (const auto &rule : definition.clocked->rules)
      definition.sourceCheckCount = ac::detail::SourceCheckBudget::add(
          definition.sourceCheckCount, rule.checks.size());
  }
  if (failed(budget.debit(definition.sourceCheckCount, module)))
    return failure();
  if (definition.clocked || !definition.clockedCalls.empty()) {
    llvm::BitVector preparationInputs(definition.wires.size());
    auto include = [&](unsigned node) -> LogicalResult {
      if (failed(budget.debit(inputs[node].size(), module)))
        return failure();
      for (unsigned input : inputs[node])
        preparationInputs.set(input);
      return success();
    };
    for (unsigned node = definition.wires.size(); node < count; ++node)
      if (failed(include(node)))
        return failure();
    for (const auto &output : definition.outputs)
      if (output.requiresPreparation && failed(include(output.wire)))
        return failure();
    auto includeRaw = [&](CallOp binding) -> LogicalResult {
      if (!binding)
        return success();
      for (Value value : binding.getInputs()) {
        auto wires = definition.valueWires.find(value);
        if (wires == definition.valueWires.end())
          return binding.emitOpError("native input must have resolved graph fields");
        for (unsigned wire : wires->second)
          if (failed(include(wire)))
            return failure();
      }
      return success();
    };
    if (definition.clocked) {
      for (const auto &fifo : definition.clocked->fifos)
        if (failed(includeRaw(fifo.rawInput)) || failed(includeRaw(fifo.rawOutput)))
          return failure();
      for (const auto &memory : definition.clocked->memories)
        if (failed(includeRaw(memory.raw)))
          return failure();
    }
    for (unsigned input : preparationInputs.set_bits())
      definition.preparationInputs.push_back(input);
  }
  return definition;
}

FailureOr<StructuredProgram> Analyzer::run() {
  if (failed(mlir::verify(package)) || failed(verifyAttributes(package, {"gfir.contract"})))
    return failure();
  auto contract = package->getAttrOfType<IntegerAttr>("gfir.contract");
  if (!contract || !contract.getType().isSignlessInteger(64) || contract.getInt() != 1)
    return package.emitOpError("structured GFIR requires gfir.contract = 1 : i64");
  auto roots = package.getOps<RootOp>();
  if (!llvm::hasSingleElement(roots))
    return package.emitOpError("structured GFIR requires one gfir.root");
  result.root = *roots.begin();
  if (failed(verifyRoot(result.root)))
    return failure();
  auto top = dyn_cast_or_null<ModuleOp>(
      hardware.lookupDefinition(result.root.getCalleeAttr().getValue()));
  if (!top)
    return result.root.emitOpError("root must select a defined structured module");
  if (result.root.getKind() == "system" &&
      (top.getFunctionType().getNumInputs() || top.getFunctionType().getNumResults()))
    return result.root.emitOpError("System requires a closed module interface");
  result.samples = ArrayAttr::get(package.getContext(), {});
  if (auto metadata = result.root.getSystemSamplesAttr()) {
    auto samples = ac::getSystemSampleRows(metadata, top);
    if (failed(samples))
      return failure();
    result.samples = *samples;
  }
  if (auto domains = result.root->getAttrOfType<ArrayAttr>("domains");
      domains && !domains.empty() && !detail::hasGeneratedDomain(result.root)) {
    llvm::DenseSet<std::pair<unsigned, unsigned>> pairs;
    for (Attribute raw : domains) {
      if (failed(budget.debit(1, result.root)))
        return failure();
      auto domain = dyn_cast<DictionaryAttr>(raw);
      auto name = domain ? domain.getAs<StringAttr>("name") : StringAttr();
      auto edge = domain ? domain.getAs<StringAttr>("edge") : StringAttr();
      if (!domain || domain.size() != 4 || !name || name.getValue().empty() ||
          !edge || edge.getValue() != "rising" || explicitInputs.count(name.getValue()))
        return result.root.emitOpError("expected unique named rising input domain");
      SmallVector<unsigned> pair;
      for (StringRef key : {"clock", "reset"}) {
        auto endpoint = domain.getAs<DictionaryAttr>(key);
        auto input = endpoint ? endpoint.getAs<IntegerAttr>("input") : IntegerAttr();
        if (!endpoint || endpoint.size() != 1 || !input || !input.getType().isSignlessInteger(64) ||
            input.getInt() < 0 || uint64_t(input.getInt()) >= top.getFunctionType().getNumInputs())
          return result.root.emitOpError("domain requires a scalar input ordinal");
        unsigned ordinal = input.getInt();
        Type type = top.getFunctionType().getInput(ordinal);
        auto width = hardware.getPackedWidth(type, {}, result.root);
        if (!isa<ac::BitsType>(type) || failed(width) || *width != 1)
          return result.root.emitOpError("domain requires scalar Bits1 inputs");
        pair.push_back(ordinal);
      }
      auto endpoints = std::make_pair(pair[0], pair[1]);
      if (pair[0] == pair[1] || !pairs.insert(endpoints).second)
        return result.root.emitOpError("named domains require distinct clock/reset pairs");
      explicitInputs.try_emplace(name.getValue(), endpoints);
    }
  }
  SmallVector<ModuleOp> modules;
  budget.setPhase("structured GFIR field dependency verification");
  for (Operation &op : *package.getBody()) {
    if (failed(budget.debit(1, &op)))
      return failure();
    if (auto module = dyn_cast<ModuleOp>(op)) {
      modules.push_back(module);
      if (auto domains = module->getAttrOfType<ArrayAttr>("domains");
          domains && domains.size() == 1 &&
          domains[0] == StringAttr::get(module.getContext(), "default"))
        clockedModules.insert(module);
    } else if (isa<ac::StructOp, ac::EnumOp>(op)) {
      if (failed(hardware.verifyResolvedOperation(&op, {})))
        return failure();
    } else if (auto primitive = dyn_cast<PrimitiveOp>(op)) {
      if (!detail::hasGeneratedDomain(result.root) && explicitInputs.empty())
        return primitive.emitOpError("native declarations require a clocked root");
      if (primitive.getNativeKind() == "memory") {
        if (failed(detail::describeRawMemory(primitive, hardware)))
          return failure();
      } else if (failed(detail::verifyFifoPrimitive(primitive, hardware))) {
        return failure();
      }
    } else if (!isa<RootOp>(op)) {
      return op.emitOpError("unexpected declaration in structured GFIR package");
    }
  }
  if (!explicitInputs.empty())
    clockedModules.erase(top);
  if (clockedModules.contains(top) != detail::hasGeneratedDomain(result.root))
    return result.root.emitOpError("generated domain must match the selected Module's domains");
  for (auto module : modules)
    if (failed(verifyDefinition(module, clockedModules.contains(module))))
      return failure();
  auto order = orderDefinitions(modules);
  if (failed(order))
    return failure();
  do {
    inputDemandsChanged = false;
    done.clear();
    result.definitions.clear();
    for (unsigned index : *order) {
      auto module = modules[index];
      auto definition = analyzeDefinition(module);
      if (failed(definition))
        return failure();
      done.try_emplace(module, result.definitions.size());
      result.definitions.push_back(std::move(*definition));
    }
  } while (inputDemandsChanged);
  result.rootIndex = done.lookup(top);
  for (auto [index, definition] : llvm::enumerate(result.definitions)) {
    if (!definition.clocked)
      continue;
    auto &fifos = definition.clocked->fifos;
    auto credit = llvm::find_if(fifos, [](const auto &fifo) { return fifo.localPopCredit; });
    if (credit == fifos.end())
      continue;
    // This local route closes the incoming proposal at the ABI boundary.
    // Child routes need corresponding private actual operands.
    if (index != result.rootIndex ||
        llvm::any_of(result.definitions, [&](const auto &parent) {
          return llvm::any_of(parent.clockedCalls, [&](const auto &call) {
            return call.definition == index;
          });
        }))
      return credit->instance->emitOpError(
          "ordered local FIFO credit requires a selected root without parent instances");
  }
  return std::move(result);
}

class VerifyStructuredPass
    : public PassWrapper<VerifyStructuredPass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(VerifyStructuredPass)
  StringRef getArgument() const final { return "gfir-verify"; }
  StringRef getDescription() const final { return "Verify a closed structured GFIR package"; }
  void runOnOperation() final {
    if (failed(analyzeStructured(getOperation())))
      signalPassFailure();
    else
      markAllAnalysesPreserved();
  }
};

class RecoverStructuredRowWritePass
    : public PassWrapper<RecoverStructuredRowWritePass, OperationPass<mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(RecoverStructuredRowWritePass)
  StringRef getArgument() const final { return "gfir-recover-row-write"; }
  StringRef getDescription() const final {
    return "Recover a proven indexed state write from a whole record update";
  }
  void runOnOperation() final {
    auto package = getOperation();
    auto program = analyzeStructured(package);
    if (failed(program)) {
      signalPassFailure();
      return;
    }
    if (!recoverStructuredRowWrite(package, *program)) {
      markAllAnalysesPreserved();
      return;
    }
    if (failed(mlir::verify(package)) || failed(analyzeStructured(package)))
      signalPassFailure();
  }
};

} // namespace

FailureOr<StructuredProgram> analyzeStructured(mlir::ModuleOp package) {
  return Analyzer(package).run();
}

void registerStructuredGFIRPasses() {
  PassRegistration<VerifyStructuredPass>();
  PassRegistration<RecoverStructuredRowWritePass>();
}

} // namespace acir::gfir

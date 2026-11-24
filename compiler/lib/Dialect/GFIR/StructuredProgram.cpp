#include "pycircuit/Dialect/GFIR/StructuredProgram.h"
#include "Dialect/GFIR/StructuredClocked.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseSet.h"
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
  return isa<ac::BitsConstantOp, ac::BitsConstant4Op, ac::BitsUnaryOp,
             ac::BitsBinaryOp, ac::BitsCompareOp, ac::BitsSelectOp,
             ac::BitsConcatOp, ac::BitsExtractOp, ac::BitsResizeOp,
             ac::StructCreateOp, ac::StructGetOp>(op);
}

bool isStructuredLeaf(Type type) {
  if (auto table = dyn_cast<ac::TableType>(type))
    return table.getShape().size() == 1 && isa<ac::BitsType>(table.getElementType());
  return isa<ac::BitsType, ac::EnumType>(type);
}

} // namespace detail
namespace {

using detail::isCombinational;
using detail::verifyAttributes;

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
    if (!domains || (!domains.empty() && !detail::hasGeneratedDomain(root)))
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
  LogicalResult verifyDefinition(ModuleOp module, bool clocked);
  FailureOr<StructuredDefinition> analyzeDefinition(ModuleOp module);
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
  llvm::DenseSet<Operation *> clockedModules;
  StructuredProgram result;
};

LogicalResult Analyzer::verifyDefinition(ModuleOp module, bool clocked) {
  auto body = &module.getBody().front();
  if (failed(verifyAttributes(module, {"sym_name", "function_type", "source_owner",
                                       "input_names", "output_names", "domains"})))
    return failure();
  Attribute rawDomains = module->getAttr("domains");
  auto domains = dyn_cast_if_present<ArrayAttr>(rawDomains);
  if (clocked) {
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
    if (failed(verifyAttributes(instance, {"sym_name", "callee", "domains"})))
      return failure();
    auto *callee = hardware.lookupDefinition(instance.getCalleeAttr().getValue());
    if (result.root.getKind() == "system" &&
        instance.getCalleeAttr() == result.root.getCalleeAttr())
      return instance.emitOpError("System cannot be used as a child module");
    const bool native = isa_and_nonnull<PrimitiveOp>(callee);
    if (native) {
      bound.insert(instance);
    } else if (!isa_and_nonnull<ModuleOp>(callee)) {
      return instance.emitOpError("callee must be a defined Module or native primitive");
    }
    const bool inheritsClock = native || clockedModules.contains(callee);
    if (inheritsClock && !clocked)
      return instance.emitOpError("clocked child requires the parent's default domain");
    if (auto raw = instance->getAttr("domains")) {
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
      auto receiver = receiverAttr ? instances.lookup(receiverAttr.getValue())
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
  auto type = callee.getFunctionType();
  auto equivalent = [](TypeRange actual, TypeRange formal) {
    return actual.size() == formal.size() &&
           llvm::all_of(llvm::zip(actual, formal), [](auto pair) {
             return ac::areEquivalentHardwareTypes(std::get<0>(pair), std::get<1>(pair));
           });
  };
  if (!equivalent(call.getInputs().getTypes(), type.getInputs()) ||
      !equivalent(call.getOutputs().getTypes(), type.getResults()))
    return call.emitOpError("instance call type does not match callee signature");
  receivers.try_emplace(call, receiver);
  return success();
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
        {{value, leaf.path}, unsigned(leaf.low), unsigned(leaf.width), {}, {}, {}, {}, {}, {}});
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

FailureOr<StructuredDefinition> Analyzer::analyzeDefinition(ModuleOp module) {
  StructuredDefinition definition;
  definition.module = module;
  auto &body = module.getBody().front();
  llvm::StringMap<unsigned> rules;
  if (clockedModules.contains(module)) {
    auto plan = detail::analyzeClocked(module, hardware, budget);
    if (failed(plan))
      return failure();
    definition.clocked = std::move(*plan);
    for (auto [index, rule] : llvm::enumerate(definition.clocked->rules))
      rules.try_emplace(rule.operation.getSymName(), index);
    definition.hasChecks = llvm::any_of(definition.clocked->rules,
        [](const StructuredRule &rule) { return rule.sampled; });
    for (auto output : body.getOps<ResultOp>()) {
      auto &rule = definition.clocked->rules[rules.lookup(output.getRuleAttr().getValue())];
      if (rule.sampled || !rule.requirements.empty())
        return output.emitOpError("gated Rule results require availability lowering");
      rule.resultUses.push_back(output);
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
    if (!child.clocked)
      continue;
    definition.hasChecks |= child.hasChecks;
    auto &prepare = definition.clockedCalls.emplace_back(
        StructuredClockedCall{call, receiver, target, {}});
    clockedCallNodes.try_emplace(
        call, definition.firstClockedCallNode() + definition.clockedCalls.size() - 1);
    for (unsigned input : child.preparationInputs) {
      auto parent = findWire(definition, parentInput(call, child, input), call);
      if (failed(parent))
        return failure();
      prepare.dependencies.push_back(*parent);
    }
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
  if (definition.clocked) {
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
    for (const auto &fifo : definition.clocked->fifos)
      if (failed(includeRaw(fifo.rawInput)) || failed(includeRaw(fifo.rawOutput)))
        return failure();
    for (const auto &memory : definition.clocked->memories)
      if (failed(includeRaw(memory.raw)))
        return failure();
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
      if (!detail::hasGeneratedDomain(result.root))
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
  if (clockedModules.contains(top) != detail::hasGeneratedDomain(result.root))
    return result.root.emitOpError("generated domain must match the selected Module's domains");
  for (auto module : modules)
    if (failed(verifyDefinition(module, clockedModules.contains(module))))
      return failure();
  auto order = orderDefinitions(modules);
  if (failed(order))
    return failure();
  for (unsigned index : *order) {
    auto module = modules[index];
    auto definition = analyzeDefinition(module);
    if (failed(definition))
      return failure();
    done.try_emplace(module, result.definitions.size());
    result.definitions.push_back(std::move(*definition));
  }
  result.rootIndex = done.lookup(top);
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

} // namespace

FailureOr<StructuredProgram> analyzeStructured(mlir::ModuleOp package) {
  return Analyzer(package).run();
}

void registerStructuredGFIRPasses() {
  PassRegistration<VerifyStructuredPass>();
}

} // namespace acir::gfir

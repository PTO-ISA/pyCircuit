#include "pycircuit/Dialect/GFIR/StructuredProgram.h"
#include "Dialect/ACIR/SourceCheckBudget.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassRegistry.h"
#include "llvm/ADT/DenseSet.h"
#include <algorithm>
#include <iterator>
#include <limits>

using namespace mlir;

namespace acir::gfir {
namespace {

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

LogicalResult verifyRoot(RootOp root) {
  if (root.getKind() != "module")
    return root.emitOpError("structured GFIR currently supports ordinary Module roots");
  if (failed(verifyAttributes(root, {"callee", "kind", "domains", "ac.execution_policy"})))
    return failure();
  if (Attribute raw = root->getAttr("domains")) {
    auto domains = dyn_cast<ArrayAttr>(raw);
    if (!domains || !domains.empty())
      return root.emitOpError("structured combinational root requires empty domains");
  }
  if (Attribute raw = root->getAttr("ac.execution_policy")) {
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
  LogicalResult verifyDefinition(ModuleOp module);
  FailureOr<StructuredDefinition> analyzeDefinition(ModuleOp module);
  FailureOr<SmallVector<unsigned>> orderDefinitions(ArrayRef<ModuleOp> modules);
  FailureOr<unsigned> findWire(const StructuredDefinition &definition,
                               const ac::WireEndpoint &endpoint, Operation *site);
  LogicalResult addValue(StructuredDefinition &definition, Value value);
  LogicalResult bindCall(CallOp call, InstanceOp receiver,
                         ModuleOp callee);

  mlir::ModuleOp package;
  ac::HardwareAnalysis hardware;
  ac::detail::SourceCheckBudget budget{{}};
  llvm::DenseMap<Operation *, unsigned> done;
  llvm::DenseMap<Operation *, InstanceOp> receivers;
  StructuredProgram result;
};

LogicalResult Analyzer::verifyDefinition(ModuleOp module) {
  auto body = &module.getBody().front();
  if (failed(verifyAttributes(module, {"sym_name", "function_type", "source_owner",
                                       "input_names", "output_names", "domains"})))
    return failure();
  if (Attribute raw = module->getAttr("domains")) {
    auto domains = dyn_cast<ArrayAttr>(raw);
    if (!domains || !domains.empty())
      return module.emitOpError("structured combinational module requires empty domains");
  }
  llvm::StringMap<InstanceOp> instances;
  llvm::SmallDenseSet<Operation *> bound;
  for (auto instance : body->getOps<InstanceOp>()) {
    if (failed(verifyAttributes(instance, {"sym_name", "callee"})))
      return failure();
    instances.try_emplace(instance.getSymName(), instance);
  }
  for (Operation &op : *body) {
    if (isa<InstanceOp>(op))
      continue;
    if (isa<YieldOp>(op)) {
      if (failed(verifyAttributes(&op, {})))
        return failure();
      continue;
    }
    if (auto call = dyn_cast<CallOp>(op)) {
      if (failed(verifyAttributes(call, {"receiver"})))
        return failure();
      auto receiver = instances.lookup(call.getReceiverAttr().getValue());
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
    if (!isCombinational(&op))
      return op.emitOpError("operation is unavailable in structured combinational GFIR");
    if (failed(hardware.verifyResolvedOperation(&op, {})))
      return failure();
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
      unsigned dependency = indices.lookup(hardware.lookupDefinition(
          instance.getCalleeAttr().getValue()));
      dependents[dependency].push_back(index);
      ++remaining[index];
    }
  }
  return getOrder(dependents, std::move(remaining), package,
                  "recursive structured GFIR hierarchy");
}

LogicalResult Analyzer::addValue(StructuredDefinition &definition, Value value) {
  auto *site = value.getParentRegion()->getParentOp();
  if (!isa<ac::BitsType, ac::StructType>(value.getType()))
    return site->emitOpError("structured combinational value must be Bits or a Struct of Bits");
  auto leaves = hardware.getLeaves(value.getType(), {}, site);
  if (failed(leaves) || leaves->empty())
    return failure();
  auto &ids = definition.valueWires[value];
  for (const auto &leaf : *leaves) {
    if (!isa<ac::BitsType>(leaf.type) || leaf.width == 0 ||
        leaf.width > std::numeric_limits<unsigned>::max() - leaf.low)
      return site->emitOpError("structured value has an unsupported packed leaf");
    if (failed(budget.debit(1 + leaf.path.size(), site)))
      return failure();
    ids.push_back(definition.wires.size());
    definition.wires.push_back(
        {{value, leaf.path}, unsigned(leaf.low), unsigned(leaf.width), {}, {}});
  }
  return success();
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

FailureOr<StructuredDefinition> Analyzer::analyzeDefinition(ModuleOp module) {
  StructuredDefinition definition;
  definition.module = module;
  auto &body = module.getBody().front();
  for (Value value : body.getArguments())
    if (failed(addValue(definition, value)))
      return failure();
  for (Operation &op : body)
    for (Value value : op.getResults())
      if (failed(addValue(definition, value)))
        return failure();

  auto debit = [&](uint64_t units, Operation *op) { return budget.debit(units, op); };
  SmallVector<SmallVector<unsigned>> dependents(definition.wires.size());
  SmallVector<unsigned> remaining;
  remaining.reserve(definition.wires.size());
  for (auto [index, wire] : llvm::enumerate(definition.wires)) {
    auto endpoint = wire.endpoint;
    auto *op = endpoint.value.getDefiningOp();
    SmallVector<ac::WireEndpoint> edges;
    if (auto call = dyn_cast_or_null<CallOp>(op)) {
      auto receiver = receivers.lookup(call);
      auto target = done.lookup(hardware.lookupDefinition(receiver.getCalleeAttr().getValue()));
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
      for (unsigned input : match->inputs) {
        const auto &source = child.wires[input].endpoint;
        auto arg = cast<BlockArgument>(source.value).getArgNumber();
        edges.push_back({call.getInputs()[arg], source.path});
      }
    } else if (op) {
      auto computed = hardware.getDependencies(endpoint.value, endpoint.path, {}, debit);
      if (failed(computed))
        return failure();
      edges = std::move(*computed);
    }
    for (const auto &edge : edges) {
      auto dep = findWire(definition, edge, module);
      if (failed(dep))
        return failure();
      wire.dependencies.push_back(*dep);
      dependents[*dep].push_back(index);
    }
    remaining.push_back(wire.dependencies.size());
  }
  auto order = getOrder(dependents, std::move(remaining), module,
                        "structured GFIR combinational cycle in field dependencies");
  if (failed(order))
    return failure();
  definition.order = std::move(*order);

  // Collapse immediate dependencies to the input leaves each exported value
  // requires. Keep argument order stable across printing and reloading.
  SmallVector<SmallVector<unsigned>> inputs(definition.wires.size());
  for (unsigned index : definition.order) {
    const auto &wire = definition.wires[index];
    if (isa<BlockArgument>(wire.endpoint.value))
      inputs[index].push_back(index);
    for (unsigned dep : wire.dependencies) {
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
          {{unsigned(port), definition.wires[index].endpoint.path}, index, inputs[index]});
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

  SmallVector<ModuleOp> modules;
  budget.setPhase("structured GFIR field dependency verification");
  for (Operation &op : *package.getBody()) {
    if (failed(budget.debit(1, &op)))
      return failure();
    if (auto module = dyn_cast<ModuleOp>(op)) {
      if (failed(verifyDefinition(module)))
        return failure();
      modules.push_back(module);
    } else if (auto structure = dyn_cast<ac::StructOp>(op)) {
      if (failed(hardware.verifyResolvedOperation(structure, {})))
        return failure();
    } else if (!isa<RootOp>(op)) {
      return op.emitOpError("unexpected declaration in structured GFIR package");
    }
  }
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

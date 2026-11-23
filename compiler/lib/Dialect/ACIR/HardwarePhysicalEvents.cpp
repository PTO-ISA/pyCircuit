#include "HardwarePhysicalEvents.h"
#include "HardwareSourceCheckPreflight.h"
#include "pycircuit/Dialect/ACIR/HardwareDomains.h"
#include "llvm/ADT/DenseSet.h"
#include <algorithm>
#include <functional>

using namespace mlir;
namespace acir::ac::detail {
FailureOr<HardwarePhysicalProjection> getBatchedPhysicalProjection(
    const HardwareAnalysis &analysis, const HardwareSourceCheckPlan &plan,
    HardwareSourceCheckLimits limits) {
  if (plan.owners.empty())
    return analysis.getPackage().emitOpError() << "physical RTL requires a root owner";
  auto root = plan.owners.front().definition;
  auto kind = root->getAttrOfType<StringAttr>("ac.root_kind");
  if (kind && kind.getValue() == "system")
    return root.emitOpError() << "physical RTL requires an ordinary Module";
  HardwarePhysicalProjection projection;
  projection.root = root;
  auto layout = getModulePortLayout(root);
  if (failed(layout)) return failure();
  auto domains = getHardwareDomainPlan(analysis.getPackage(), limits);
  if (failed(domains)) return failure();
  projection.clockInputs = domains->clockInputs;
  // Trace identities through actual occurrence wiring. Reset expressions may
  // use only total feedforward operations: state/resource and Rule-result
  // availability feedback cannot create another physical batch.
  SourceCheckBudget budget(limits);
  budget.setPhase("physical event projection");
  std::function<LogicalResult(Value, uint64_t, bool, unsigned,
                             SmallVectorImpl<unsigned> &)> trace;
  trace = [&](Value value, uint64_t owner, bool clock, unsigned depth,
              SmallVectorImpl<unsigned> &inputs) -> LogicalResult {
    auto &scope = plan.owners[owner];
    Operation *op = value.getDefiningOp();
    Operation *site = op ? op : value.getParentBlock()->getParentOp();
    if (depth > 256)
      return site->emitOpError() << "physical event projection exceeds bounded traversal";
    if (failed(budget.debit(1, site))) return failure();
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      auto *parent = argument.getOwner()->getParentOp();
      if (auto rule = dyn_cast<RuleOp>(parent))
        return trace(rule.getCaptures()[argument.getArgNumber()], owner,
                     clock, depth + 1, inputs);
      if (parent == scope.definition) {
        if (scope.parent)
          return trace(scope.path.back().allocation->getOperand(argument.getArgNumber()),
                       *scope.parent, clock, depth + 1, inputs);
        if (argument.getArgNumber() >= layout->sourceInputCount)
          return site->emitOpError() << "physical event source is not a public input";
        inputs.push_back(argument.getArgNumber());
        return success();
      }
    }
    if (auto rule = dyn_cast_or_null<RuleOp>(op)) {
      // Even a pure Rule result can be unavailable when its transaction does
      // not commit. Captures are identity wiring; results are not event roots.
      return rule.emitOpError() << "physical reset cannot depend on a transaction result";
    }
    if (isa_and_nonnull<InstanceOp, CollectionOp>(op)) {
      bool found = false;
      for (auto [child, occurrence] : llvm::enumerate(plan.owners)) {
        if (occurrence.parent != owner || occurrence.path.empty() ||
            occurrence.path.back().allocation != op) continue;
        found = true;
        auto &terminator = occurrence.definition->getRegion(0).front().back();
        auto result = cast<OpResult>(value).getResultNumber();
        if (result >= terminator.getNumOperands() ||
            failed(trace(terminator.getOperand(result), child, clock,
                         depth + 1, inputs))) return failure();
      }
      if (found) return success();
      return op->emitOpError() << "physical reset cannot depend on storage or CDC state";
    }
    bool transport = isa_and_nonnull<TableSplatOp, TableCreateOp>(op);
    if (auto extract = dyn_cast_or_null<BitsExtractOp>(op)) {
      auto inWidth = analysis.getPackedWidth(extract.getInput().getType(), scope.bindings, op);
      auto outWidth = analysis.getPackedWidth(value.getType(), scope.bindings, op);
      auto low = analysis.evaluateStatic(extract.getLow(), scope.bindings, op);
      if (failed(inWidth) || failed(outWidth) || failed(low)) return failure();
      auto integer = dyn_cast<MathIntAttr>(*low);
      transport = integer && integer.getCanonicalValue() == "0" && *inWidth == *outWidth;
    }
    bool feedforward = isa_and_nonnull<BitsConstantOp, BitsConstant4Op,
        BitsUnaryOp, BitsBinaryOp, BitsCompareOp, BitsSelectOp,
        BitsConcatOp, BitsExtractOp, BitsResizeOp>(op);
    if (!op || !(transport || (!clock && feedforward)))
      return site->emitOpError() << (clock ? "physical clock requires root identity wiring"
          : "physical reset requires a total feedforward root-control expression");
    for (Value operand : op->getOperands())
      if (failed(trace(operand, owner, clock, depth + 1, inputs))) return failure();
    return success();
  };
  auto clock = [&](Value value, uint64_t owner) -> LogicalResult {
    SmallVector<unsigned> inputs;
    if (failed(trace(value, owner, true, 0, inputs))) return failure();
    llvm::sort(inputs); inputs.erase(std::unique(inputs.begin(), inputs.end()), inputs.end());
    if (inputs.size() != 1 || !llvm::is_contained(projection.clockInputs, inputs.front()))
      return root.emitOpError() << "physical clock does not resolve to a declared root clock";
    return success();
  };
  for (const auto &sampling : plan.samplingOccurrences) {
    const auto &binding = plan.samplingBindings[sampling.bindingIndex];
    if (failed(clock(binding.clock, sampling.ownerOccurrence))) return failure();
    for (const auto &reset : binding.resets)
      if (failed(trace(reset.value, sampling.ownerOccurrence, false, 0,
                       projection.resetInputs))) return failure();
  }
  for (const auto &endpoint : plan.commits) {
    Operation *op = endpoint.allocation;
    auto primitive = endpoint.primitiveKind.getValue();
    if (primitive != "dff" && primitive != "dffe")
      return op->emitOpError() << "physical event adapter requires publicly expanded storage";
    auto policy = getRegisterPolicy(op);
    if (failed(policy) || failed(clock(op->getOperand(0), endpoint.ownerOccurrence)))
      return failure();
    if (policy->resetKind != RegisterResetKind::None &&
        failed(trace(op->getOperand(1), endpoint.ownerOccurrence, false, 0,
                     projection.resetInputs))) return failure();
  }
  for (auto &inputs : {&projection.clockInputs, &projection.resetInputs}) {
    llvm::sort(*inputs);
    inputs->erase(std::unique(inputs->begin(), inputs->end()), inputs->end());
  }
  if (!projection.clockInputs.empty()) projection.clockInput = projection.clockInputs.front();
  if (!projection.resetInputs.empty()) projection.resetInput = projection.resetInputs.front();
  return projection;
}
} // namespace acir::ac::detail

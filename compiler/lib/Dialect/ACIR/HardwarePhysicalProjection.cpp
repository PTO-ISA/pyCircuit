#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"
#include "HardwarePhysicalEvents.h"
#include "ACIRSourceContracts.h"
#include "HardwareSampledAdmission.h"
#include "SourceCheckBudget.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include <functional>

using namespace mlir;
namespace acir::ac {
FailureOr<HardwarePhysicalProjection> HardwareAnalysis::getPhysicalProjection(
    const HardwareSourceCheckPlan &plan,
    HardwareSourceCheckLimits limits) const {
  if (failed(verifySourceCheckPlan(plan, limits)))
    return failure();
  auto contract = package->getAttrOfType<IntegerAttr>("ac.routed_contract");
  if (contract && contract.getInt() == 7)
    return detail::getBatchedPhysicalProjection(*this, plan, limits);
  if (plan.owners.empty())
    return package.emitOpError() << "physical RTL requires a root owner";
  auto root = plan.owners.front().definition;
  auto kind = root->getAttrOfType<StringAttr>("ac.root_kind");
  if (kind && kind.getValue() == "system")
    return root.emitOpError()
           << "physical RTL requires an ordinary Module, not a closed System";
  auto domain = root->getAttrOfType<DictionaryAttr>("ac.domain_inputs");
  auto clock = domain ? domain.getAs<IntegerAttr>("clock") : IntegerAttr();
  auto reset = domain ? domain.getAs<IntegerAttr>("reset") : IntegerAttr();
  auto layout = getModulePortLayout(root);
  if (failed(layout))
    return failure();
  if (!clock || !reset || clock.getValue().getActiveBits() > 32 ||
      reset.getValue().getActiveBits() > 32 || clock == reset ||
      clock.getValue().getZExtValue() >= layout->sourceInputCount ||
      reset.getValue().getZExtValue() >= layout->sourceInputCount)
    return root.emitOpError()
           << "physical RTL requires one actual root clock/reset pair";
  unsigned clk = clock.getValue().getZExtValue(),
           rst = reset.getValue().getZExtValue();
  detail::SourceCheckBudget budget(limits);
  budget.setPhase("physical RTL projection");
  // Identity-only transport is sufficient for domain proof. Uniform Tables
  // preserve identity through Collection slicing and layout permutations.
  // Derived expressions deliberately have no domain identity.
  std::function<FailureOr<unsigned>(Value, uint64_t, unsigned)> identity;
  identity = [&](Value value, uint64_t owner,
                 unsigned depth) -> FailureOr<unsigned> {
    auto &occurrence = plan.owners[owner];
    Operation *site = value.getDefiningOp();
    if (!site)
      site = value.getParentBlock()->getParentOp();
    if (depth > 256 || failed(budget.debit(1, site)))
      return site->emitOpError()
             << "physical domain identity exceeds bounded traversal";
    if (auto argument = dyn_cast<BlockArgument>(value)) {
      auto *parent = argument.getOwner()->getParentOp();
      if (auto rule = dyn_cast<RuleOp>(parent))
        return identity(rule->getOperand(argument.getArgNumber()), owner,
                        depth + 1);
      if (parent == occurrence.definition) {
        if (!occurrence.parent)
          return argument.getArgNumber() == clk   ? 1u
                 : argument.getArgNumber() == rst ? 2u
                                                  : 0u;
        auto *allocation = occurrence.path.back().allocation;
        return identity(allocation->getOperand(argument.getArgNumber()),
                        *occurrence.parent, depth + 1);
      }
      return 0u;
    }
    if (auto result = dyn_cast<OpResult>(value)) {
      if (auto rule = dyn_cast<RuleOp>(result.getOwner()))
        return identity(cast<YieldOp>(rule.getBody().front().back())
                            .getValues()[result.getResultNumber()],
                        owner, depth + 1);
    }
    if (auto extract = value.getDefiningOp<BitsExtractOp>()) {
      auto low = evaluateStatic(extract.getLow(), occurrence.bindings, extract);
      auto inWidth = getPackedWidth(extract.getInput().getType(),
                                    occurrence.bindings, extract);
      auto outWidth =
          getPackedWidth(value.getType(), occurrence.bindings, extract);
      if (failed(low) || failed(inWidth) || failed(outWidth))
        return failure();
      auto integer = dyn_cast<MathIntAttr>(*low);
      if (integer && llvm::APSInt(integer.getCanonicalValue()).isZero() &&
          *inWidth == *outWidth)
        return identity(extract.getInput(), owner, depth + 1);
    }
    if (auto splat = value.getDefiningOp<TableSplatOp>())
      return identity(splat.getInput(), owner, depth + 1);
    if (auto create = value.getDefiningOp<TableCreateOp>()) {
      unsigned uniform = 0;
      for (Value element : create->getOperands()) {
        auto part = identity(element, owner, depth + 1);
        if (failed(part))
          return failure();
        if (!*part || (uniform && uniform != *part))
          return 0u;
        uniform = *part;
      }
      return uniform;
    }
    return 0u;
  };
  auto pair = [&](Value clockValue, Value resetValue, uint64_t owner,
                  Operation *site) -> LogicalResult {
    auto c = identity(clockValue, owner, 0), r = identity(resetValue, owner, 0);
    if (failed(c) || failed(r))
      return failure();
    if (*c != 1 || *r != 2)
      return site->emitOpError()
             << "physical RTL requires all sampling and storage to use the "
                "actual root clock/reset pair";
    return success();
  };
  llvm::DenseSet<Operation *> reachable;
  for (const auto &owner : plan.owners)
    reachable.insert(owner.definition);
  for (const auto &check : plan.bindings)
    if (reachable.contains(check.definition) &&
        check.kind.getValue() != "guard_known")
      return check.expect->emitOpError()
             << "physical RTL requires managed execution for residual "
             << check.kind.getValue() << " checks";
  for (const auto &sampling : plan.samplingOccurrences) {
    const auto &binding = plan.samplingBindings[sampling.bindingIndex];
    if (failed(pair(binding.clock, binding.reset, sampling.ownerOccurrence,
                    binding.definition)))
      return failure();
  }
  for (auto [owner, occurrence] : llvm::enumerate(plan.owners)) {
    auto definition = occurrence.definition;
    auto observations = definition.getBody().front().getOps<SourceObserveOp>();
    if (!observations.empty())
      return definition.emitOpError()
             << "physical RTL does not support observations";
    // Include stateless and readonly transactions even if no storage/check
    // occurrence happens to retain their domain in the managed plan.
    for (auto rule : definition.getBody().front().getOps<RuleOp>()) {
      if (!rule->hasAttr("ac.sampled_admission"))
        continue;
      auto view = detail::parseSampledAdmission(rule, *this,
                                                occurrence.bindings, budget);
      if (failed(view) || failed(pair(view->clock, view->reset, owner, rule)))
        return failure();
      for (const auto &control : view->controls)
        if (control.kind != detail::SampledControlKind::CallableGuard &&
            control.kind != detail::SampledControlKind::BranchSelect)
          return rule.emitOpError() << "physical RTL requires managed "
                                       "execution for non-guard controls";
    }
    for (Operation &op : definition.getBody().front()) {
      if (!isa<InstanceOp, CollectionOp>(op))
        continue;
      auto *callee = isa<InstanceOp>(op)
                         ? resolveCallee(cast<InstanceOp>(op))
                         : resolveCallee(cast<CollectionOp>(op));
      auto primitive = getPrimitiveKind(callee);
      if (primitive.empty())
        continue;
      if (primitive != "dff" && primitive != "dffe")
        return op.emitOpError()
               << "physical RTL supports only DFF/DFFE storage primitives";
      if (op.getNumOperands() < 2 ||
          failed(pair(op.getOperand(0), op.getOperand(1), owner, &op)))
        return failure();
      auto version = getRegisterContract(callee);
      if (failed(version)) return failure();
      unsigned initialIndex = primitive == "dff" ? 3 : 4;
      auto policy = getRegisterPolicy(&op);
      if (failed(policy)) return failure();
      if (*version == 3 && policy->resetKind != RegisterResetKind::None &&
          (op.getNumOperands() <= initialIndex + 1 ||
           failed(detail::verifyClosedStateInitializer(
               *this, op.getOperand(initialIndex + 1), occurrence.bindings, &op, &budget))))
        return failure();
      if (policy->hasInitial && (op.getNumOperands() <= initialIndex ||
          failed(detail::verifyClosedStateInitializer(
              *this, op.getOperand(initialIndex), occurrence.bindings, &op, &budget))))
        return failure();
    }
    auto uses = definition.walk([&](Operation *op) {
      for (auto [index, operand] : llvm::enumerate(op->getOperands())) {
        auto id = identity(operand, owner, 0);
        if (failed(id))
          return WalkResult::interrupt();
        if (*id != 1)
          continue;
        bool transport = isa<TableSplatOp>(op);
        if (auto create = dyn_cast<TableCreateOp>(op)) {
          auto result = identity(create.getResult(), owner, 0);
          if (failed(result))
            return WalkResult::interrupt();
          transport = *result == 1;
        }
        if (auto extract = dyn_cast<BitsExtractOp>(op)) {
          auto result = identity(extract.getResult(), owner, 0);
          if (failed(result))
            return WalkResult::interrupt();
          transport = *result == 1;
        }
        if (auto rule = dyn_cast<RuleOp>(op)) {
          auto metadata =
              rule->getAttrOfType<DictionaryAttr>("ac.sampled_admission");
          auto capture = metadata ? metadata.getAs<IntegerAttr>("clock_capture")
                                  : IntegerAttr();
          transport = capture && index == capture.getValue().getZExtValue();
        }
        if (isa<InstanceOp, CollectionOp>(op)) {
          auto *callee = isa<InstanceOp>(op)
                             ? resolveCallee(cast<InstanceOp>(op))
                             : resolveCallee(cast<CollectionOp>(op));
          auto primitive = getPrimitiveKind(callee);
          // Ordinary child arguments are checked in their actual occurrence.
          transport =
              primitive.empty() ||
              ((primitive == "dff" || primitive == "dffe") && index == 0);
        }
        if (auto expect = dyn_cast<SourceExpectOp>(op))
          transport = expect.getKind() == "guard_known" && index == 2;
        if (!transport) {
          op->emitOpError() << "physical RTL forbids clock use as data or "
                               "combinational control";
          return WalkResult::interrupt();
        }
      }
      return WalkResult::advance();
    });
    if (uses.wasInterrupted())
      return failure();
  }
  return HardwarePhysicalProjection{root, clk, rst};
}
} // namespace acir::ac

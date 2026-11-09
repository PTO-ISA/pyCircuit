#include "pycircuit/Analysis/LogicalStateValidation.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Verifier.h"
#include "Dialect/ACIR/SourceCheckBudget.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "IR/StateDeclarationContracts.h"
#include "pycircuit/Dialect/ACIR/ACIROps.h"
#include "pycircuit/Dialect/ACIR/HardwareAnalysis.h"

#include <functional>

using namespace mlir;

namespace acir::ac {
namespace {

LogicalResult staticInitializer(const HardwareAnalysis &analysis, Value initial,
                                const HardwareBindings &bindings,
                                Operation *state,
                                detail::SourceCheckBudget *budget) {
  // Prove closure of the existing literal/constructor graph. This performs
  // no SSA arithmetic evaluation and never obtains values from runtime wires.
  // Inside the bounded source-check plan each visited DAG operation charges
  // one unit; native verification passes no budget.
  struct Frame {
    Operation *operation;
    unsigned nextOperand = 0;
  };
  SmallVector<Frame> stack;
  llvm::DenseSet<Operation *> active, complete;
  auto push = [&](Value value) -> LogicalResult {
    auto *operation = value.getDefiningOp();
    if (!operation || operation->getParentOp() != bindings.owner ||
        !isa<BitsConstantOp, BitsConstant4Op, EnumCreateOp, StructCreateOp,
             TableCreateOp, TableSplatOp, StructGetOp, EnumToBitsOp,
             BitsExtractOp, BitsResizeOp>(operation))
      return state->emitOpError()
             << "state initializer requires a closed static "
                "literal/constructor "
                "DAG; runtime values and unsupported computations are rejected";
    // The complete set deduplicates operations, so each charges once; the
    // charge covers the operation's own checks plus its operand-edge
    // traversal steps, operand/result type checks and attribute checks.
    // Shared initializer DAGs therefore pay per state and per operand edge
    // (S*M growth), not once per distinct node.
    if (budget &&
        failed(budget->debit(
            detail::SourceCheckBudget::add(
                uint64_t(1),
                detail::SourceCheckBudget::add(
                    detail::SourceCheckBudget::add(
                        operation->getNumOperands(),
                        operation->getNumResults()),
                    operation->getAttrs().size())),
            state)))
      return failure();
    if (!active.insert(operation).second)
      return state->emitOpError()
             << "state initializer constructor DAG is cyclic";
    if (failed(mlir::verify(operation))) return failure();
    bool concrete = true;
    for (TypeRange types : {TypeRange(operation->getOperandTypes()), TypeRange(operation->getResultTypes())})
      for (Type type : types) {
        bool closed = false;
        if (failed(detail::verifySourceTemplateType(analysis, type, bindings, operation, &closed)))
          return failure();
        concrete &= closed;
      }
    for (NamedAttribute attr : operation->getAttrs())
      if (failed(detail::verifyHardwareAttributeScope(attr.getValue(), bindings.owner)))
        return failure();
    // Independently closed attributes remain checked even when some operand
    // or result width depends on a formal. Only width-relative fit may defer.
    std::function<LogicalResult(Attribute, unsigned)> checkAttribute =
        [&](Attribute attribute, unsigned depth) -> LogicalResult {
      if (depth > 128) return state->emitOpError() << "initializer expression depth exceeded";
      if (auto expression = dyn_cast<StaticExprAttr>(attribute)) {
        if (analysis.isStaticEvaluable(expression, bindings))
          return success(succeeded(analysis.evaluateStatic(expression, bindings, operation)));
        return checkAttribute(expression.getTree(), depth + 1);
      }
      if (auto array = dyn_cast<ArrayAttr>(attribute))
        for (Attribute child : array)
          if (failed(checkAttribute(child, depth + 1))) return failure();
      if (auto dictionary = dyn_cast<DictionaryAttr>(attribute))
        for (NamedAttribute field : dictionary)
          if (field.getName() != "origin" && field.getName() != "location" &&
              failed(checkAttribute(field.getValue(), depth + 1))) return failure();
      return success();
    };
    for (NamedAttribute attr : operation->getAttrs())
      if (failed(checkAttribute(attr.getValue(), 0))) return failure();
    auto nonnegative = [&](StaticExprAttr expression, StringRef description) -> LogicalResult {
      if (!analysis.isStaticEvaluable(expression, bindings)) return success();
      auto value = analysis.evaluateStatic(expression, bindings, operation);
      auto integer = succeeded(value) ? dyn_cast<MathIntAttr>(*value) : MathIntAttr();
      if (!integer || llvm::APSInt(integer.getCanonicalValue()).isNegative())
        return operation->emitOpError() << description << " requires a nonnegative Integer";
      return success();
    };
    if (auto constant = dyn_cast<BitsConstantOp>(operation))
      if (failed(nonnegative(constant.getValue(), "state initializer constant"))) return failure();
    if (auto extract = dyn_cast<BitsExtractOp>(operation))
      if (failed(nonnegative(extract.getLow(), "state initializer extract low"))) return failure();
    if (concrete && failed(analysis.verifyResolvedOperation(operation, bindings)))
      return failure();
    stack.push_back({operation});
    return success();
  };
  if (failed(push(initial)))
    return failure();
  while (!stack.empty()) {
    auto &frame = stack.back();
    if (frame.nextOperand == frame.operation->getNumOperands()) {
      complete.insert(frame.operation);
      active.erase(frame.operation);
      stack.pop_back();
      continue;
    }
    Value input = frame.operation->getOperand(frame.nextOperand++);
    if (complete.contains(input.getDefiningOp()))
      continue;
    if (failed(push(input)))
      return failure();
  }
  return success();
}

} // namespace

LogicalResult detail::verifyClosedStateInitializer(
    const HardwareAnalysis &analysis, Value initial,
    const HardwareBindings &bindings, Operation *site,
    SourceCheckBudget *budget) {
  return staticInitializer(analysis, initial, bindings, site, budget);
}

HardwareBindings stateOwnerScope(Operation *operation) {
  HardwareBindings bindings;
  bindings.owner = operation->getParentOfType<ModuleOp>();
  return bindings;
}

LogicalResult stateControlPredicate(const HardwareAnalysis &analysis,
                                    Value value,
                                    const HardwareBindings &bindings,
                                    Operation *site, llvm::StringRef description) {
  if (!isa<BitsType>(value.getType()))
    return site->emitOpError() << description << " requires width-one Bits";
  auto width = analysis.getPackedWidth(value.getType(), bindings, site);
  if (failed(width))
    return failure();
  if (*width != 1)
    return site->emitOpError() << description << " requires width-one Bits";
  return success();
}

FailureOr<LogicalStateUpdateOpInterface>
admissionPairLogicalStateUpdate(LogicalStateOpInterface state,
                                detail::SourceCheckBudget &budget) {
  if (failed(verifyLogicalStateStructure(state.getOperation())))
    return failure();
  LogicalStateUpdateOpInterface result;
  for (auto *user : state.getStateQ().getUsers()) {
    // Same single-unit admission charge as the previous admission helper.
    if (failed(budget.debit(1, state.getOperation())))
      return failure();
    auto update = dyn_cast<LogicalStateUpdateOpInterface>(user);
    if (!update || update.getUpdatedStateQ() != state.getStateQ())
      continue;
    // One operation may use Q in both the state and candidate positions.
    if (result && result != update)
      return state.getOperation()->emitOpError()
             << "logical state has multiple update bindings";
    result = update;
  }
  if (!result)
    return state.getOperation()->emitOpError()
           << "logical state has no update binding";
  return result;
}

FailureOr<LogicalStateUpdateOpInterface>
verifySameBlockStateUpdatePairing(LogicalStateOpInterface state,
                                  detail::SourceCheckBudget *budget) {
  Operation *operation = state.getOperation();
  LogicalStateUpdateOpInterface pairedUpdate;
  for (Operation *user : state.getStateQ().getUsers()) {
    // One unit per examined user, matching the admission helper's charge.
    if (budget && failed(budget->debit(1, operation)))
      return failure();
    auto update = dyn_cast<LogicalStateUpdateOpInterface>(user);
    if (!update || update->getBlock() != operation->getBlock() ||
        update->getNumOperands() != 3 ||
        update.getUpdatedStateQ() != state.getStateQ())
      continue;
    if (pairedUpdate && pairedUpdate != update)
      return operation->emitOpError()
             << "routed logical state requires exactly one same-block update";
    // Q may also be this update's candidate, so its users can repeat it.
    pairedUpdate = update;
  }
  if (!pairedUpdate)
    return operation->emitOpError()
           << "routed logical state requires exactly one same-block update";
  return pairedUpdate;
}

namespace {

// Shared declaration-bound sequence used by the routed interface binding and
// the source declaration binding: width-one controls under resolved
// bindings, initial/Q payload equivalence, and the raw payload-constraint
// attribute check. The staged pairing and initializer closure are owned by
// the stage-specific callers.
LogicalResult declarationBoundChecks(Operation *operation, Value clock,
                                     Value reset, Value initial, Value q,
                                     ModuleOp owner,
                                     const HardwareAnalysis &analysis,
                                     const HardwareBindings &bindings) {
  if (failed(stateControlPredicate(analysis, clock, bindings, operation,
                                   "state clock")) ||
      failed(stateControlPredicate(analysis, reset, bindings, operation,
                                   "state reset")))
    return failure();
  if (!areEquivalentHardwareTypes(initial.getType(), q.getType()))
    return operation->emitOpError()
           << "initial value and old-Q payload types must match";
  // Test the raw attribute so a present-but-malformed constraint is still
  // rejected downstream, matching the previous untyped branch; the
  // interface getter alone would silently skip a non-Dictionary attribute.
  if (Attribute constraint = operation->getAttr("ac.payload_constraint"))
    if (failed(detail::verifySourceCallHardwareConstraint(
            constraint, q.getType(), analysis, owner, operation,
            "state payload", detail::SourceCallWidthMode::SymbolicFixedBits)))
      return failure();
  return success();
}

} // namespace

LogicalResult verifyLogicalStateDeclarationBinding(
    LogicalStateOpInterface state, const HardwareAnalysis *shared,
    const HardwareBindings *sharedBindings,
    detail::SourceCheckBudget *budget) {
  Operation *operation = state.getOperation();
  if (failed(verifyLogicalStateStructure(operation)))
    return failure();
  auto owner = cast<ModuleOp>(operation->getParentOp());
  HardwareAnalysis owned(operation->getParentOfType<mlir::ModuleOp>());
  const HardwareAnalysis &analysis = shared ? *shared : owned;
  HardwareBindings ownedBindings = stateOwnerScope(operation);
  const HardwareBindings &bindings =
      sharedBindings ? *sharedBindings : ownedBindings;
  if (failed(declarationBoundChecks(operation, state.getStateClock(),
                                    state.getStateReset(),
                                    state.getStateInitial(), state.getStateQ(),
                                    owner, analysis, bindings)))
    return failure();
  auto package = operation->getParentOfType<mlir::ModuleOp>();
  auto stage = package ? package->getAttrOfType<StringAttr>("ac.stage")
                       : StringAttr();
  if (stage && stage.getValue() == "routed") {
    auto pairedUpdate = verifySameBlockStateUpdatePairing(state, budget);
    if (failed(pairedUpdate))
      return failure();
    // The single enumeration also carries the paired update's bound check
    // (resolved intent width under the same bindings); its fixed work
    // charges one unit inside the bounded plan.
    if (budget && failed(budget->debit(1, operation)))
      return failure();
    if (failed(verifyLogicalStateUpdateBinding(*pairedUpdate, &analysis,
                                               &bindings)))
      return failure();
  }
  return detail::verifyClosedStateInitializer(analysis, state.getStateInitial(),
                                              bindings, operation, budget);
}

LogicalResult verifyLogicalStateSourceBinding(SourceStateOp state) {
  // The source ac.state declaration keeps its bound checks; its routed
  // pairing branch is gone with the routed class (B1-boundary.md).
  Operation *operation = state.getOperation();
  auto owner = cast<ModuleOp>(operation->getParentOp());
  HardwareAnalysis owned(operation->getParentOfType<mlir::ModuleOp>());
  HardwareBindings ownedBindings = stateOwnerScope(operation);
  if (failed(declarationBoundChecks(operation, state.getClk(), state.getRst(),
                                    state.getInitial(), state.getQ(), owner,
                                    owned, ownedBindings)))
    return failure();
  return detail::verifyClosedStateInitializer(owned, state.getInitial(),
                                               ownedBindings, operation,
                                               /*budget=*/nullptr);
}

LogicalResult verifyLogicalStateUpdateBinding(
    LogicalStateUpdateOpInterface update, const HardwareAnalysis *shared,
    const HardwareBindings *sharedBindings) {
  Operation *operation = update.getOperation();
  if (failed(verifyLogicalStateUpdateStructure(operation)))
    return failure();
  auto package = operation->getParentOfType<mlir::ModuleOp>();
  if (failed(verifyRoutedLogicalStatePlacement(operation)))
    return failure();
  if (!operation->getAttrs().empty())
    return operation->emitOpError()
           << "state update does not accept attributes";
  // The concrete gfir.state pairing is proven by the GFIR local verifier;
  // the bound check independently requires the defining routed declaration
  // to expose the common state contract in the same block.
  auto state =
      update.getUpdatedStateQ().getDefiningOp();
  auto stateContract =
      state ? dyn_cast<LogicalStateOpInterface>(state) : nullptr;
  if (!stateContract || state->getBlock() != operation->getBlock() ||
      failed(verifyLogicalStateStructure(state)))
    return operation->emitOpError()
           << "state must be the direct result of a same-module routed "
              "logical state declaration";
  if (!areEquivalentHardwareTypes(update.getUpdatedStateQ().getType(),
                                  update.getStateCandidate().getType()))
    return operation->emitOpError()
           << "candidate must exactly match the state payload type";
  HardwareAnalysis owned(package);
  const HardwareAnalysis &analysis = shared ? *shared : owned;
  HardwareBindings ownedBindings = stateOwnerScope(operation);
  const HardwareBindings &bindings =
      sharedBindings ? *sharedBindings : ownedBindings;
  return stateControlPredicate(analysis, update.getStateWriteIntent(),
                               bindings, operation, "state update intent");
}

} // namespace acir::ac

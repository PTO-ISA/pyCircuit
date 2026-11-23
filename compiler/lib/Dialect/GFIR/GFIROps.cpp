#include "pycircuit/Dialect/GFIR/GFIROps.h"
#include "pycircuit/Dialect/GFIR/GFIRDialect.h"
#include "Dialect/ACIR/ACIRSourceContracts.h"
#include "pycircuit/Interfaces/ModuleChildInterfaces.h"
#include "pycircuit/Dialect/ACIR/SourceEffects.h"
#include "IR/StateDeclarationContracts.h"
#include "mlir/IR/BuiltinOps.h"

using namespace mlir;

namespace acir::gfir {

// GFIR owns concrete state local verification (B1-boundary.md). These native
// verifiers prove local structure, placement and literal-shape facts only.
// The staged package pipeline's bound checks (resolved control widths,
// constraint semantics, initializer closure, pairing inventory) remain
// separate obligations; parsing or mlir::verify alone never authorizes
// publication, refinement or execution.

void StateOp::getEffects(
    llvm::SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  // Same storage resource identity as the source declaration it replaces.
  effects.emplace_back(MemoryEffects::Allocate::get(),
                       ac::SourceStateResource::get());
  effects.emplace_back(MemoryEffects::Read::get(),
                       ac::SourceStateResource::get());
}

LogicalResult StateOp::verify() {
  if (failed(ac::verifyRoutedLogicalStatePlacement(getOperation())))
    return failure();
  if ((*this)->hasAttr("sym_name"))
    return emitOpError() << "state uses instance_name; sym_name is unsupported";
  if (failed(ac::verifyNamedLogicalDeclaration(getOperation(),
                                                 getInstanceNameAttr())))
    return failure();
  auto owner = (*this)->getParentOp();
  for (Operation &other : owner->getRegion(0).front())
    if (&other != getOperation() &&
        (isa<ModuleChildOpInterface, LogicalStateOpInterface>(other)) &&
        other.getAttrOfType<StringAttr>("instance_name") ==
            getInstanceNameAttr())
      return emitOpError() << "state instance_name must be unique across state "
                              "and child instance declarations";
  for (Operation &other : owner->getRegion(0).front()) {
    if (&other == getOperation())
      continue;
    auto peer = dyn_cast<LogicalStateOpInterface>(other);
    if (!peer)
      continue;
    if (ac::detail::compareClosedSourceStructure(
            getOccurrence(), peer.getStateOccurrence()) == 0)
      return emitOpError() << "state declarations require distinct occurrence "
                              "identities within their module";
  }
  if (failed(ac::verifyLiteralControlWidth(getOperation(), getClk(),
                                             "state clock")) ||
      (getRst() && failed(ac::verifyLiteralControlWidth(getOperation(), getRst(),
                                             "state reset"))))
    return failure();
  if (getInitial() && !ac::areEquivalentHardwareTypes(getInitial().getType(), getQ().getType()))
    return emitOpError()
           << "initial value and old-Q payload types must match";
  if (getResetValue() && !ac::areEquivalentHardwareTypes(getResetValue().getType(), getQ().getType()))
    return emitOpError("reset value must match the state payload type");
  // Test the raw attribute so a present-but-malformed constraint is still
  // rejected downstream; the interface getter alone would silently skip a
  // non-Dictionary attribute. The semantic SymbolicFixedBits and scope
  // checks stay with the bound stage verifier.
  if (Attribute constraint = (*this)->getAttr("ac.payload_constraint"))
    if (failed(ac::verifyPayloadConstraintShape(getOperation(), constraint,
                                                  getQ().getType())))
      return failure();
  // Local same-block pairing: exactly one update binds this Q, deduplicating
  // a Q that also appears as the update's candidate. The package declaration
  // binding independently re-establishes the complete inventory.
  LogicalStateUpdateOpInterface paired;
  for (Operation *user : getQ().getUsers()) {
    auto update = dyn_cast<LogicalStateUpdateOpInterface>(user);
    if (!update || update->getBlock() != (*this)->getBlock() ||
        update->getNumOperands() != 3 ||
        update.getUpdatedStateQ() != getQ())
      continue;
    if (paired && paired != update)
      return emitOpError()
             << "routed logical state requires exactly one same-block update";
    paired = update;
  }
  if (!paired)
    return emitOpError()
           << "routed logical state requires exactly one same-block update";
  return success();
}

void StateUpdateOp::getEffects(
    llvm::SmallVectorImpl<MemoryEffects::EffectInstance> &effects) {
  effects.emplace_back(MemoryEffects::Write::get(),
                       ac::SourceStateResource::get());
}

LogicalResult StateUpdateOp::verify() {
  if (failed(ac::verifyRoutedLogicalStatePlacement(getOperation())))
    return failure();
  if (!(*this)->getAttrs().empty())
    return emitOpError() << "state update does not accept attributes";
  // The bound Q must be the direct result of one same-block gfir.state;
  // a source ac.state never satisfies this routed pairing.
  auto state = getState().getDefiningOp<StateOp>();
  if (!state || state->getBlock() != (*this)->getBlock())
    return emitOpError()
           << "state must be the direct result of a same-module gfir.state";
  if (failed(verifyLogicalStateStructure(state.getOperation())))
    return failure();
  if (!ac::areEquivalentHardwareTypes(getState().getType(),
                                      getCandidate().getType()))
    return emitOpError() << "candidate must exactly match the state payload type";
  return ac::verifyLiteralControlWidth(getOperation(), getIntent(),
                                         "state update intent");
}

} // namespace acir::gfir

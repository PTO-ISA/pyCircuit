#ifndef PYCIRCUIT_INTERFACES_LOGICALSTATEINTERFACES_TD
#define PYCIRCUIT_INTERFACES_LOGICALSTATEINTERFACES_TD

include "mlir/IR/OpBase.td"

// Common routed-logical-state contracts shared by staged analysis. These
// expose actual SSA operands, results and declaration attributes of the
// implementing operation; no method performs lookup, caching, allocation,
// stage queries or verification. Implementations must keep the fixed
// operand/result ordinals verified by verifyLogicalStateStructure and
// verifyLogicalStateUpdateStructure: state clock/reset/initial = operands
// 0/1/2 with Q = result 0, update Q/candidate/intent = operands 0/1/2 with
// no results. The interface is an access contract, never an admission
// decision: presence does not authorize publication, refinement or
// execution (see B1-boundary.md).

def LogicalStateOpInterface : OpInterface<"LogicalStateOpInterface"> {
  let description = [{
    A declared logical state with committed old-Q semantics in a routed
    package. Source-stage declarations do not carry this contract after
    the GFIR migration.
  }];
  let methods = [
    InterfaceMethod<"Clock operand at fixed slot 0.", "mlir::Value",
                    "getStateClock">,
    InterfaceMethod<"Reset operand at fixed slot 1.", "mlir::Value",
                    "getStateReset">,
    InterfaceMethod<"Initial value operand at fixed slot 2.", "mlir::Value",
                    "getStateInitial">,
    InterfaceMethod<"Committed old-Q result at fixed slot 0.",
                    "mlir::Value", "getStateQ">,
    InterfaceMethod<"Declaration instance name attribute.",
                    "mlir::StringAttr", "getStateInstanceName">,
    InterfaceMethod<"Declaration occurrence attribute.",
                    "mlir::DictionaryAttr", "getStateOccurrence">,
    InterfaceMethod<"Optional payload constraint dictionary, or null.",
                    "mlir::DictionaryAttr", "getStatePayloadConstraint">,
  ];
}

def LogicalStateUpdateOpInterface : OpInterface<"LogicalStateUpdateOpInterface"> {
  let description = [{
    The single declarative candidate/intent binding of one logical state.
    It is not a Work-time mutation; false intent holds old Q.
  }];
  let methods = [
    InterfaceMethod<"Bound state Q operand at fixed slot 0.", "mlir::Value",
                    "getUpdatedStateQ">,
    InterfaceMethod<"Candidate value operand at fixed slot 1.", "mlir::Value",
                    "getStateCandidate">,
    InterfaceMethod<"Write intent operand at fixed slot 2.", "mlir::Value",
                    "getStateWriteIntent">,
  ];
}

#endif // PYCIRCUIT_INTERFACES_LOGICALSTATEINTERFACES_TD

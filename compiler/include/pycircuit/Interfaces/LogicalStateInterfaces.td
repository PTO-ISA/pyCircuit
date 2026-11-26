#ifndef PYCIRCUIT_INTERFACES_LOGICALSTATEINTERFACES_TD
#define PYCIRCUIT_INTERFACES_LOGICALSTATEINTERFACES_TD

include "mlir/IR/OpBase.td"

// Common routed-logical-state contracts shared by staged analysis. These
// Accessors expose actual SSA operands, results and declaration attributes.
// Contract 4/5 uses clock/reset/initial at 0/1/2; contract 7 uses the exact
// four clock/reset/initial/reset-value segments, with optional reset and init.
// Structural decoding validates each getter before staged admission.

def LogicalStateOpInterface : OpInterface<"LogicalStateOpInterface"> {
  let description = [{
    A declared logical state with committed old-Q semantics in a routed
    package. Source-stage declarations do not carry this contract after
    the GFIR migration.
  }];
  let methods = [
    InterfaceMethod<"Clock operand at fixed slot 0.", "mlir::Value",
                    "getStateClock">,
    InterfaceMethod<"Optional reset operand.", "mlir::Value",
                    "getStateReset">,
    InterfaceMethod<"Optional initial value operand.", "mlir::Value",
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

#ifndef ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD
#define ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD

include "acir/Dialect/ACIR/ACIRDialect.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/EnumAttr.td"

def ACIR_Unit_Ticks : I32EnumAttrCase<"Ticks", 0, "ticks">;
def ACIR_Unit_Cycles : I32EnumAttrCase<"Cycles", 1, "cycles">;
def ACIR_Unit_Seconds : I32EnumAttrCase<"Seconds", 2, "seconds">;
def ACIR_Unit_Milliseconds
    : I32EnumAttrCase<"Milliseconds", 3, "milliseconds">;
def ACIR_Unit_Microseconds
    : I32EnumAttrCase<"Microseconds", 4, "microseconds">;
def ACIR_Unit_Nanoseconds
    : I32EnumAttrCase<"Nanoseconds", 5, "nanoseconds">;
def ACIR_Unit_Picoseconds
    : I32EnumAttrCase<"Picoseconds", 6, "picoseconds">;
def ACIR_Unit_Bytes : I32EnumAttrCase<"Bytes", 7, "bytes">;
def ACIR_Unit_Bits : I32EnumAttrCase<"Bits", 8, "bits">;
def ACIR_Unit_Entries : I32EnumAttrCase<"Entries", 9, "entries">;
def ACIR_Unit_Packets : I32EnumAttrCase<"Packets", 10, "packets">;
def ACIR_Unit_Transactions
    : I32EnumAttrCase<"Transactions", 11, "transactions">;

def ACIR_Unit : I32EnumAttr<"Unit", "ACIR exact measurement unit", [
  ACIR_Unit_Ticks,
  ACIR_Unit_Cycles,
  ACIR_Unit_Seconds,
  ACIR_Unit_Milliseconds,
  ACIR_Unit_Microseconds,
  ACIR_Unit_Nanoseconds,
  ACIR_Unit_Picoseconds,
  ACIR_Unit_Bytes,
  ACIR_Unit_Bits,
  ACIR_Unit_Entries,
  ACIR_Unit_Packets,
  ACIR_Unit_Transactions
]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}

def ACIR_TypeConstraintState_Unknown
    : I32EnumAttrCase<"Unknown", 0, "unknown">;
def ACIR_TypeConstraintState_Constrained
    : I32EnumAttrCase<"Constrained", 1, "constrained">;
def ACIR_TypeConstraintState_Exact
    : I32EnumAttrCase<"Exact", 2, "exact">;
def ACIR_TypeConstraintState
    : I32EnumAttr<"TypeConstraintState", "rule type-constraint lattice", [
        ACIR_TypeConstraintState_Unknown,
        ACIR_TypeConstraintState_Constrained,
        ACIR_TypeConstraintState_Exact
      ]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_TypeConstraintStateAttr
    : EnumAttr<ACIR_Dialect, ACIR_TypeConstraintState,
               "type_constraint_state">;

def ACIR_TypeConstraintKind_QueuePayload
    : I32EnumAttrCase<"QueuePayload", 0, "queue_payload">;
def ACIR_TypeConstraintKind
    : I32EnumAttr<"TypeConstraintKind", "rule type-constraint kind", [
        ACIR_TypeConstraintKind_QueuePayload
      ]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_TypeConstraintKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_TypeConstraintKind,
               "type_constraint_kind">;

def ACIR_ValueFactKind_CommittedInput
    : I32EnumAttrCase<"CommittedInput", 0, "committed_input">;
def ACIR_ValueFactKind
    : I32EnumAttr<"ValueFactKind", "rule value-fact kind", [
        ACIR_ValueFactKind_CommittedInput
      ]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ValueFactKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_ValueFactKind, "value_fact_kind">;

def ACIR_ObligationState_Pending
    : I32EnumAttrCase<"Pending", 0, "pending">;
def ACIR_ObligationState_Materialized
    : I32EnumAttrCase<"Materialized", 1, "materialized">;
def ACIR_ObligationState_Discharged
    : I32EnumAttrCase<"Discharged", 2, "discharged">;
def ACIR_ObligationState
    : I32EnumAttr<"ObligationState", "rule obligation lifecycle", [
        ACIR_ObligationState_Pending,
        ACIR_ObligationState_Materialized,
        ACIR_ObligationState_Discharged
      ]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ObligationStateAttr
    : EnumAttr<ACIR_Dialect, ACIR_ObligationState, "obligation_state">;

def ACIR_ObligationResolver_Handshake
    : I32EnumAttrCase<"Handshake", 0, "handshake">;
def ACIR_ObligationResolver_Checks
    : I32EnumAttrCase<"Checks", 1, "checks">;
def ACIR_ObligationResolver_Schedule
    : I32EnumAttrCase<"Schedule", 2, "schedule">;
def ACIR_ObligationResolver
    : I32EnumAttr<"ObligationResolver", "named rule obligation resolver", [
        ACIR_ObligationResolver_Handshake,
        ACIR_ObligationResolver_Checks,
        ACIR_ObligationResolver_Schedule
      ]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ObligationResolverAttr
    : EnumAttr<ACIR_Dialect, ACIR_ObligationResolver,
               "obligation_resolver">;

#endif // ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD

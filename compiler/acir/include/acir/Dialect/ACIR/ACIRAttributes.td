#ifndef ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD
#define ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD

include "acir/Dialect/ACIR/ACIRDialect.td"
include "mlir/IR/AttrTypeBase.td"
include "mlir/IR/EnumAttr.td"

def ACIR_Unit_Ticks : I32EnumAttrCase<"Ticks", 0, "ticks">;
def ACIR_Unit_Cycles : I32EnumAttrCase<"Cycles", 1, "cycles">;
def ACIR_Unit_Seconds : I32EnumAttrCase<"Seconds", 2, "seconds">;
def ACIR_Unit_Milliseconds : I32EnumAttrCase<"Milliseconds", 3, "milliseconds">;
def ACIR_Unit_Microseconds : I32EnumAttrCase<"Microseconds", 4, "microseconds">;
def ACIR_Unit_Nanoseconds : I32EnumAttrCase<"Nanoseconds", 5, "nanoseconds">;
def ACIR_Unit_Picoseconds : I32EnumAttrCase<"Picoseconds", 6, "picoseconds">;
def ACIR_Unit_Bytes : I32EnumAttrCase<"Bytes", 7, "bytes">;
def ACIR_Unit_Bits : I32EnumAttrCase<"Bits", 8, "bits">;
def ACIR_Unit_Entries : I32EnumAttrCase<"Entries", 9, "entries">;
def ACIR_Unit_Packets : I32EnumAttrCase<"Packets", 10, "packets">;
def ACIR_Unit_Transactions
    : I32EnumAttrCase<"Transactions", 11, "transactions">;

def ACIR_Unit
    : I32EnumAttr<"Unit", "ACIR exact measurement unit",
                  [ACIR_Unit_Ticks, ACIR_Unit_Cycles, ACIR_Unit_Seconds,
                   ACIR_Unit_Milliseconds, ACIR_Unit_Microseconds,
                   ACIR_Unit_Nanoseconds, ACIR_Unit_Picoseconds,
                   ACIR_Unit_Bytes, ACIR_Unit_Bits, ACIR_Unit_Entries,
                   ACIR_Unit_Packets, ACIR_Unit_Transactions]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}

def ACIR_TypeConstraintState_Unknown : I32EnumAttrCase<"Unknown", 0, "unknown">;
def ACIR_TypeConstraintState_Constrained
    : I32EnumAttrCase<"Constrained", 1, "constrained">;
def ACIR_TypeConstraintState_Exact : I32EnumAttrCase<"Exact", 2, "exact">;
def ACIR_TypeConstraintState
    : I32EnumAttr<"TypeConstraintState", "rule type-constraint lattice",
                  [ACIR_TypeConstraintState_Unknown,
                   ACIR_TypeConstraintState_Constrained,
                   ACIR_TypeConstraintState_Exact]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_TypeConstraintStateAttr
    : EnumAttr<ACIR_Dialect, ACIR_TypeConstraintState, "type_constraint_state">;

def ACIR_ObligationState_Pending : I32EnumAttrCase<"Pending", 0, "pending">;
def ACIR_ObligationState_Materialized
    : I32EnumAttrCase<"Materialized", 1, "materialized">;
def ACIR_ObligationState_Discharged
    : I32EnumAttrCase<"Discharged", 2, "discharged">;
def ACIR_ObligationState
    : I32EnumAttr<"ObligationState", "rule obligation lifecycle",
                  [ACIR_ObligationState_Pending,
                   ACIR_ObligationState_Materialized,
                   ACIR_ObligationState_Discharged]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ObligationStateAttr
    : EnumAttr<ACIR_Dialect, ACIR_ObligationState, "obligation_state">;

def ACIR_ObligationResolver_Handshake
    : I32EnumAttrCase<"Handshake", 0, "handshake">;
def ACIR_ObligationResolver_Checks : I32EnumAttrCase<"Checks", 1, "checks">;
def ACIR_ObligationResolver_Schedule
    : I32EnumAttrCase<"Schedule", 2, "schedule">;
def ACIR_ObligationResolver
    : I32EnumAttr<"ObligationResolver", "named rule obligation resolver",
                  [ACIR_ObligationResolver_Handshake,
                   ACIR_ObligationResolver_Checks,
                   ACIR_ObligationResolver_Schedule]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ObligationResolverAttr
    : EnumAttr<ACIR_Dialect, ACIR_ObligationResolver, "obligation_resolver">;

// Decision 0272 architecture obligations are deliberately separate from the
// transient rule-handshake ObligationState/ObligationResolver vocabulary.
def ACIR_ArchitectureObligationKind_MutualExclusion
    : I32EnumAttrCase<"MutualExclusion", 0, "mutual_exclusion">;
def ACIR_ArchitectureObligationKind_SingleWriter
    : I32EnumAttrCase<"SingleWriter", 1, "single_writer">;
def ACIR_ArchitectureObligationKind_ResourceCapacity
    : I32EnumAttrCase<"ResourceCapacity", 2, "resource_capacity">;
def ACIR_ArchitectureObligationKind_ReadyValidIntegrity
    : I32EnumAttrCase<"ReadyValidIntegrity", 3, "ready_valid_integrity">;
def ACIR_ArchitectureObligationKind_TransactionAtomicity
    : I32EnumAttrCase<"TransactionAtomicity", 4, "transaction_atomicity">;
def ACIR_ArchitectureObligationKind_GenerationMatch
    : I32EnumAttrCase<"GenerationMatch", 5, "generation_match">;
def ACIR_ArchitectureObligationKind_EpochMatch
    : I32EnumAttrCase<"EpochMatch", 6, "epoch_match">;
def ACIR_ArchitectureObligationKind_Ordering
    : I32EnumAttrCase<"Ordering", 7, "ordering">;
def ACIR_ArchitectureObligationKind_Range
    : I32EnumAttrCase<"Range", 8, "range">;
def ACIR_ArchitectureObligationKind_Onehot0
    : I32EnumAttrCase<"Onehot0", 9, "onehot0">;
def ACIR_ArchitectureObligationKind_NoPartialCommit
    : I32EnumAttrCase<"NoPartialCommit", 10, "no_partial_commit">;
def ACIR_ArchitectureObligationKind_NoStaleUpdate
    : I32EnumAttrCase<"NoStaleUpdate", 11, "no_stale_update">;
def ACIR_ArchitectureObligationKind_CreditBalance
    : I32EnumAttrCase<"CreditBalance", 12, "credit_balance">;
def ACIR_ArchitectureObligationKind_PipelineAlignment
    : I32EnumAttrCase<"PipelineAlignment", 13, "pipeline_alignment">;
def ACIR_ArchitectureObligationKind
    : I32EnumAttr<"ArchitectureObligationKind",
                  "closed Decision 0272 phase-one safety kind",
                  [ACIR_ArchitectureObligationKind_MutualExclusion,
                   ACIR_ArchitectureObligationKind_SingleWriter,
                   ACIR_ArchitectureObligationKind_ResourceCapacity,
                   ACIR_ArchitectureObligationKind_ReadyValidIntegrity,
                   ACIR_ArchitectureObligationKind_TransactionAtomicity,
                   ACIR_ArchitectureObligationKind_GenerationMatch,
                   ACIR_ArchitectureObligationKind_EpochMatch,
                   ACIR_ArchitectureObligationKind_Ordering,
                   ACIR_ArchitectureObligationKind_Range,
                   ACIR_ArchitectureObligationKind_Onehot0,
                   ACIR_ArchitectureObligationKind_NoPartialCommit,
                   ACIR_ArchitectureObligationKind_NoStaleUpdate,
                   ACIR_ArchitectureObligationKind_CreditBalance,
                   ACIR_ArchitectureObligationKind_PipelineAlignment]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ArchitectureObligationKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_ArchitectureObligationKind,
               "architecture_obligation_kind">;

def ACIR_ArchitectureObligationSeverity_Error
    : I32EnumAttrCase<"Error", 0, "error">;
def ACIR_ArchitectureObligationSeverity_Fatal
    : I32EnumAttrCase<"Fatal", 1, "fatal">;
def ACIR_ArchitectureObligationSeverity
    : I32EnumAttr<"ArchitectureObligationSeverity",
                  "closed architecture-obligation severity",
                  [ACIR_ArchitectureObligationSeverity_Error,
                   ACIR_ArchitectureObligationSeverity_Fatal]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ArchitectureObligationSeverityAttr
    : EnumAttr<ACIR_Dialect, ACIR_ArchitectureObligationSeverity,
               "architecture_obligation_severity">;

def ACIR_ArchitectureObligationStatus_Pending
    : I32EnumAttrCase<"Pending", 0, "pending">;
def ACIR_ArchitectureObligationStatus_Proved
    : I32EnumAttrCase<"Proved", 1, "proved">;
def ACIR_ArchitectureObligationStatus_RuntimeChecked
    : I32EnumAttrCase<"RuntimeChecked", 2, "runtime_checked">;
def ACIR_ArchitectureObligationStatus_Rejected
    : I32EnumAttrCase<"Rejected", 3, "rejected">;
def ACIR_ArchitectureObligationStatus
    : I32EnumAttr<"ArchitectureObligationStatus",
                  "architecture-obligation proof/admission status",
                  [ACIR_ArchitectureObligationStatus_Pending,
                   ACIR_ArchitectureObligationStatus_Proved,
                   ACIR_ArchitectureObligationStatus_RuntimeChecked,
                   ACIR_ArchitectureObligationStatus_Rejected]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ArchitectureObligationStatusAttr
    : EnumAttr<ACIR_Dialect, ACIR_ArchitectureObligationStatus,
               "architecture_obligation_status">;

def ACIR_ArchitectureRuntimeTarget_Cpp
    : I32EnumAttrCase<"Cpp", 0, "cpp">;
def ACIR_ArchitectureRuntimeTarget_Gfsim
    : I32EnumAttrCase<"Gfsim", 1, "gfsim">;
def ACIR_ArchitectureRuntimeTarget_Sva
    : I32EnumAttrCase<"Sva", 2, "sva">;
def ACIR_ArchitectureRuntimeTarget
    : I32EnumAttr<"ArchitectureRuntimeTarget",
                  "closed architecture-obligation runtime target",
                  [ACIR_ArchitectureRuntimeTarget_Cpp,
                   ACIR_ArchitectureRuntimeTarget_Gfsim,
                   ACIR_ArchitectureRuntimeTarget_Sva]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ArchitectureRuntimeTargetAttr
    : EnumAttr<ACIR_Dialect, ACIR_ArchitectureRuntimeTarget,
               "architecture_runtime_target">;

def ACIR_ArchitectureSamplingKind_TickObservation
    : I32EnumAttrCase<"TickObservation", 0, "tick_observation">;
def ACIR_ArchitectureSamplingKind_XferObservation
    : I32EnumAttrCase<"XferObservation", 1, "xfer_observation">;
def ACIR_ArchitectureSamplingKind_PrePublish
    : I32EnumAttrCase<"PrePublish", 2, "pre_publish">;
def ACIR_ArchitectureSamplingKind_ProducerEvent
    : I32EnumAttrCase<"ProducerEvent", 3, "producer_event">;
def ACIR_ArchitectureSamplingKind
    : I32EnumAttr<"ArchitectureSamplingKind",
                  "closed architecture-obligation sampling union tag",
                  [ACIR_ArchitectureSamplingKind_TickObservation,
                   ACIR_ArchitectureSamplingKind_XferObservation,
                   ACIR_ArchitectureSamplingKind_PrePublish,
                   ACIR_ArchitectureSamplingKind_ProducerEvent]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ArchitectureSamplingKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_ArchitectureSamplingKind,
               "architecture_sampling_kind">;

def ACIR_ArchitectureSamplingEdge_None
    : I32EnumAttrCase<"None", 0, "none">;
def ACIR_ArchitectureSamplingEdge_Posedge
    : I32EnumAttrCase<"Posedge", 1, "posedge">;
def ACIR_ArchitectureSamplingEdge_Negedge
    : I32EnumAttrCase<"Negedge", 2, "negedge">;
def ACIR_ArchitectureSamplingEdge
    : I32EnumAttr<"ArchitectureSamplingEdge",
                  "closed architecture-obligation sampling edge",
                  [ACIR_ArchitectureSamplingEdge_None,
                   ACIR_ArchitectureSamplingEdge_Posedge,
                   ACIR_ArchitectureSamplingEdge_Negedge]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ArchitectureSamplingEdgeAttr
    : EnumAttr<ACIR_Dialect, ACIR_ArchitectureSamplingEdge,
               "architecture_sampling_edge">;

def ACIR_ActivationResourceKind_InputQueue
    : I32EnumAttrCase<"InputQueue", 0, "input_queue">;
def ACIR_ActivationResourceKind_OutputQueue
    : I32EnumAttrCase<"OutputQueue", 1, "output_queue">;
def ACIR_ActivationResourceKind_State : I32EnumAttrCase<"State", 2, "state">;
def ACIR_ActivationResourceKind_Slot : I32EnumAttrCase<"Slot", 3, "slot">;
def ACIR_ActivationResourceKind
    : I32EnumAttr<"ActivationResourceKind",
                  "typed rule activation/transaction resource kind",
                  [ACIR_ActivationResourceKind_InputQueue,
                   ACIR_ActivationResourceKind_OutputQueue,
                   ACIR_ActivationResourceKind_State,
                   ACIR_ActivationResourceKind_Slot]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_ActivationResourceKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_ActivationResourceKind,
               "activation_resource_kind">;

def ACIR_RuleGuardKind_Always : I32EnumAttrCase<"Always", 0, "always">;
def ACIR_RuleGuardKind_Predicate
    : I32EnumAttrCase<"Predicate", 1, "predicate">;
def ACIR_RuleGuardKind
    : I32EnumAttr<"RuleGuardKind", "typed rule guard kind",
                  [ACIR_RuleGuardKind_Always,
                   ACIR_RuleGuardKind_Predicate]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_RuleGuardKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_RuleGuardKind, "rule_guard_kind">;

def ACIR_RuleScheduleKind_Independent
    : I32EnumAttrCase<"Independent", 0, "independent">;
def ACIR_RuleScheduleKind_LexicalPriority
    : I32EnumAttrCase<"LexicalPriority", 1, "lexical_priority">;
def ACIR_RuleScheduleKind
    : I32EnumAttr<"RuleScheduleKind", "typed rule schedule kind",
                  [ACIR_RuleScheduleKind_Independent,
                   ACIR_RuleScheduleKind_LexicalPriority]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_RuleScheduleKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_RuleScheduleKind, "rule_schedule_kind">;

def ACIR_WriterArbitrationPolicy_Priority
    : I32EnumAttrCase<"Priority", 0, "priority">;
def ACIR_WriterArbitrationPolicy
    : I32EnumAttr<"WriterArbitrationPolicy",
                  "typed same-owner writer arbitration policy",
                  [ACIR_WriterArbitrationPolicy_Priority]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_WriterArbitrationPolicyAttr
    : EnumAttr<ACIR_Dialect, ACIR_WriterArbitrationPolicy,
               "writer_arbitration_policy">;

def ACIR_WriterArbitrationResolution_WinnerTakesTransaction
    : I32EnumAttrCase<"WinnerTakesTransaction", 0,
                      "winner_takes_transaction">;
def ACIR_WriterArbitrationResolution
    : I32EnumAttr<"WriterArbitrationResolution",
                  "typed writer conflict resolution",
                  [ACIR_WriterArbitrationResolution_WinnerTakesTransaction]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_WriterArbitrationResolutionAttr
    : EnumAttr<ACIR_Dialect, ACIR_WriterArbitrationResolution,
               "writer_arbitration_resolution">;

def ACIR_TableSelectionPolicy_First
    : I32EnumAttrCase<"First", 0, "first">;
def ACIR_TableSelectionPolicy_Min
    : I32EnumAttrCase<"Min", 1, "min">;
def ACIR_TableSelectionPolicy_Max
    : I32EnumAttrCase<"Max", 2, "max">;
def ACIR_TableSelectionPolicy_RoundRobin
    : I32EnumAttrCase<"RoundRobin", 3, "round_robin">;
def ACIR_TableSelectionPolicy
    : I32EnumAttr<"TableSelectionPolicy", "typed Table selection policy",
                  [ACIR_TableSelectionPolicy_First,
                   ACIR_TableSelectionPolicy_Min,
                   ACIR_TableSelectionPolicy_Max,
                   ACIR_TableSelectionPolicy_RoundRobin]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_TableSelectionPolicyAttr
    : EnumAttr<ACIR_Dialect, ACIR_TableSelectionPolicy,
               "table_selection_policy">;

def ACIR_TableKeyOrdering_Unsigned
    : I32EnumAttrCase<"Unsigned", 0, "unsigned">;
def ACIR_TableKeyOrdering_Signed
    : I32EnumAttrCase<"Signed", 1, "signed">;
def ACIR_TableKeyOrdering
    : I32EnumAttr<"TableKeyOrdering", "typed Table key ordering intent",
                  [ACIR_TableKeyOrdering_Unsigned,
                   ACIR_TableKeyOrdering_Signed]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_TableKeyOrderingAttr
    : EnumAttr<ACIR_Dialect, ACIR_TableKeyOrdering,
               "table_key_ordering">;

def ACIR_RuleCheckKind_InputAvailable
    : I32EnumAttrCase<"InputAvailable", 0, "input_available">;
def ACIR_RuleCheckKind_OutputCapacity
    : I32EnumAttrCase<"OutputCapacity", 1, "output_capacity">;
def ACIR_RuleCheckKind
    : I32EnumAttr<"RuleCheckKind", "typed inferred rule check kind",
                  [ACIR_RuleCheckKind_InputAvailable,
                   ACIR_RuleCheckKind_OutputCapacity]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_RuleCheckKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_RuleCheckKind, "rule_check_kind">;

def ACIR_RuleEffectKind_InputConsume
    : I32EnumAttrCase<"InputConsume", 0, "input_consume">;
def ACIR_RuleEffectKind_OutputProduce
    : I32EnumAttrCase<"OutputProduce", 1, "output_produce">;
def ACIR_RuleEffectKind_StateRead
    : I32EnumAttrCase<"StateRead", 2, "state_read">;
def ACIR_RuleEffectKind_StateWrite
    : I32EnumAttrCase<"StateWrite", 3, "state_write">;
def ACIR_RuleEffectKind
    : I32EnumAttr<"RuleEffectKind", "typed inferred rule effect kind",
                  [ACIR_RuleEffectKind_InputConsume,
                   ACIR_RuleEffectKind_OutputProduce,
                   ACIR_RuleEffectKind_StateRead,
                   ACIR_RuleEffectKind_StateWrite]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_RuleEffectKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_RuleEffectKind, "rule_effect_kind">;

def ACIR_RuleOutputPresenceKind_Always
    : I32EnumAttrCase<"Always", 0, "always">;
def ACIR_RuleOutputPresenceKind_Predicate
    : I32EnumAttrCase<"Predicate", 1, "predicate">;
def ACIR_RuleOutputPresenceKind
    : I32EnumAttr<"RuleOutputPresenceKind", "typed selected output presence",
                  [ACIR_RuleOutputPresenceKind_Always,
                   ACIR_RuleOutputPresenceKind_Predicate]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_RuleOutputPresenceKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_RuleOutputPresenceKind,
               "rule_output_presence_kind">;

def ACIR_RuleStateAccessKind_Read
    : I32EnumAttrCase<"Read", 0, "read">;
def ACIR_RuleStateAccessKind_Replace
    : I32EnumAttrCase<"Replace", 1, "replace">;
def ACIR_RuleStateAccessKind_FieldWrite
    : I32EnumAttrCase<"FieldWrite", 2, "field_write">;
def ACIR_RuleStateAccessKind
    : I32EnumAttr<"RuleStateAccessKind", "typed rule state access kind",
                  [ACIR_RuleStateAccessKind_Read,
                   ACIR_RuleStateAccessKind_Replace,
                   ACIR_RuleStateAccessKind_FieldWrite]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_RuleStateAccessKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_RuleStateAccessKind,
               "rule_state_access_kind">;

def ACIR_RuleIndexKind_Static
    : I32EnumAttrCase<"Static", 0, "static">;
def ACIR_RuleIndexKind_Dynamic
    : I32EnumAttrCase<"Dynamic", 1, "dynamic">;
def ACIR_RuleIndexKind_All : I32EnumAttrCase<"All", 2, "all">;
def ACIR_RuleIndexKind
    : I32EnumAttr<"RuleIndexKind", "typed state footprint index kind",
                  [ACIR_RuleIndexKind_Static,
                   ACIR_RuleIndexKind_Dynamic,
                   ACIR_RuleIndexKind_All]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_RuleIndexKindAttr
    : EnumAttr<ACIR_Dialect, ACIR_RuleIndexKind, "rule_index_kind">;

// Closed node vocabulary for Decision 0271's persisted rule-expression DAG.
// Node ordinals are serialization-local handles only; semantic equality is the
// recursively typed structure represented by these opcodes and attributes.
def ACIR_RuleExpressionOpcode_RuleInput
    : I32EnumAttrCase<"RuleInput", 0, "rule_input">;
def ACIR_RuleExpressionOpcode_Constant
    : I32EnumAttrCase<"Constant", 1, "constant">;
def ACIR_RuleExpressionOpcode_CommittedState
    : I32EnumAttrCase<"CommittedState", 2, "committed_state">;
def ACIR_RuleExpressionOpcode_Operation
    : I32EnumAttrCase<"Operation", 3, "operation">;
def ACIR_RuleExpressionOpcode_Lane
    : I32EnumAttrCase<"Lane", 4, "lane">;
def ACIR_RuleExpressionOpcode
    : I32EnumAttr<"RuleExpressionOpcode",
                  "closed persisted rule-expression node opcode",
                  [ACIR_RuleExpressionOpcode_RuleInput,
                   ACIR_RuleExpressionOpcode_Constant,
                   ACIR_RuleExpressionOpcode_CommittedState,
                   ACIR_RuleExpressionOpcode_Operation,
                   ACIR_RuleExpressionOpcode_Lane]> {
  let genSpecializedAttr = 0;
  let cppNamespace = "::acir::ac";
}
def ACIR_RuleExpressionOpcodeAttr
    : EnumAttr<ACIR_Dialect, ACIR_RuleExpressionOpcode,
               "rule_expression_opcode">;

def ACIR_WriterPriorityAttr : AttrDef<ACIR_Dialect, "WriterPriority"> {
  let mnemonic = "writer_priority";
  let summary = "explicit owner-local Table writer priority";
  let parameters = (ins "int64_t":$rank);
  let assemblyFormat = "`<` $rank `>`";
  let genVerifyDecl = 1;
}

#endif // ACIR_DIALECT_ACIR_ACIRATTRIBUTES_TD

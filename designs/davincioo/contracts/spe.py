"""Shared typed payloads for the in-tree DavinciOO contributor designs.

Ported from hengliao1972/DavinciOO at b81ecfc2b634d41886b01fd8724905eeb5bb6551.
"""

from __future__ import annotations

from enum import Enum

import agentic_circuit as ac

HANDOFF_OWNER_MPQ = 0b01
HANDOFF_OWNER_BROB = 0b10

WRITEBACK_EFFECT_GPR = 0b0000001
WRITEBACK_EFFECT_WAKEUP = 0b0000010
WRITEBACK_EFFECT_ROB = 0b0000100
WRITEBACK_EFFECT_BROB = 0b0001000
WRITEBACK_EFFECT_FAULT = 0b0010000
WRITEBACK_EFFECT_STORE = 0b0100000
WRITEBACK_EFFECT_BRANCH = 0b1000000

RENAME_OWNER_GPR = 0b00000001
RENAME_OWNER_HISTORY = 0b00000010
RENAME_OWNER_ROB = 0b00000100
RENAME_OWNER_DISPATCH = 0b00001000
RENAME_OWNER_ISQ = 0b00010000
RENAME_OWNER_CMDIQ = 0b00100000
RENAME_OWNER_LOCAL = 0b01000000


class RobEventKind(Enum):
    ALLOCATE = 0
    ALLOCATED = 1
    COMPLETE = 2
    FLUSH = 3
    MICROCOMMIT = 4


class IntAluOperation(Enum):
    ADD = 0
    SUB = 1
    AND = 2
    OR = 3
    XOR = 4


class ExecutionClass(Enum):
    ALU = 0
    BRU = 1
    AGU = 2
    STD = 3
    FSU = 4
    DIV = 5
    SYS = 6
    CMD = 7


class OperandSourceKind(Enum):
    NONE = 0
    REGISTER_FILE = 1
    BYPASS = 2
    LOAD_FORWARD = 3


class IssueCancelReason(Enum):
    RECOVERY = 0
    READ_PORT_DENIED = 1
    LOAD_MISS = 2
    PIPE_CONFLICT = 3
    STALE_ATTEMPT = 4
    LOAD_REPLAY = 5


class TerminalSource(Enum):
    ALU = 0
    BRU = 1
    AGU = 2
    LSU = 3
    STD = 4
    FSU = 5
    DIV = 6
    SYS = 7
    CMD = 8


class TerminalStatus(Enum):
    VALUE = 0
    NO_DESTINATION = 1
    FAULT = 2
    STORE = 3
    BRANCH = 4
    WAKEUP_ONLY = 5
    CANCELLED = 6


class MapEventKind(Enum):
    RENAME = 0
    HISTORY_ACK = 1
    HANDOFF_ACK = 2
    ARCHITECTURAL_COMMIT = 3
    COMMIT_ACK = 4
    RECLAIM = 5


class GprMaintenanceKind(Enum):
    INITIALIZE = 0
    ALLOCATE = 1
    RECLAIM = 2


class W2ApplyPhase(Enum):
    IDLE = 0
    WAIT_GPR = 1
    READY_FINAL = 2


class ScalarRegisterClass(Enum):
    NONE = 0
    GPR = 1
    T = 2
    U = 3
    CONSTANT_ZERO = 4


class RenameDestinationKind(Enum):
    NONE = 0
    GPR = 1
    T_PUSH = 2
    U_PUSH = 3


class RenameTxnPhase(Enum):
    PREVIEWED = 0
    PREPARE_SENT = 1
    READY_TO_PUBLISH = 2
    PUBLISHED = 3
    CANCELED = 4


class RenameMaintenanceKind(Enum):
    INITIALIZE = 0
    RECLAIM = 1
    DRAIN = 2


class SourceLeaseOwner(Enum):
    REN = 0
    MPQ = 1
    ISQ = 2
    I1 = 3
    I2 = 4
    CMDIQ = 5
    CMDP = 6
    CBRG = 7
    BISQ = 8
    EXECUTION = 9
    DRAINED = 10


class RenameReclaimEvidenceKind(Enum):
    CMAP_DISPLACEMENT = 0
    LOCAL_MAPPING_DISPLACEMENT = 1
    MPQ_ZERO_LEASE = 2
    DATA_OWNER_DRAIN = 3
    WBA_DRAIN = 4
    W2_DRAIN = 5
    LATE_PRODUCER_DRAIN = 6


class MpqLeaseUpdateKind(Enum):
    TRANSFER = 0
    RELEASE = 1


@ac.struct
class FlowKey:
    core_id: ac.bits[4]
    pe_id: ac.bits[2]
    stid: ac.bits[4]
    launch_generation: ac.bits[16]


@ac.struct
class EpochKey:
    flow: FlowKey
    recovery_epoch: ac.bits[16]


@ac.struct
class InstKey:
    flow: FlowKey
    instruction_sequence: ac.bits[32]
    original_pc: ac.bits[64]


@ac.struct
class BlockKey:
    flow: FlowKey
    block_sequence: ac.bits[32]
    slot: ac.bits[8]
    generation: ac.bits[16]


@ac.struct
class RobKey:
    flow: FlowKey
    slot: ac.bits[4]
    generation: ac.bits[16]


@ac.struct
class PhysRef:
    tag: ac.bits[7]
    generation: ac.bits[16]
    valid: bool


@ac.struct
class MapRecord:
    kind: MapEventKind
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    arch_index: ac.bits[5]
    old_phys: PhysRef
    new_phys: PhysRef
    history_sequence: ac.bits[32]
    commit_sequence: ac.bits[32]
    source_lease_count: ac.bits[8]
    valid: bool
    durable: bool


@ac.struct
class MapLookupRequest:
    flow: FlowKey
    arch_index: ac.bits[5]
    query_sequence: ac.bits[32]
    valid: bool


@ac.struct
class MapLookupResponse:
    flow: FlowKey
    arch_index: ac.bits[5]
    query_sequence: ac.bits[32]
    phys: PhysRef
    found: bool


@ac.struct
class RobEvent:
    kind: RobEventKind
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    uop_index: ac.bits[4]
    destination_tag: ac.bits[7]
    destination_generation: ac.bits[16]
    handoff_required_mask: ac.bits[2]
    mpq_history_record_count: ac.bits[4]
    result: ac.bits[64]
    result_valid: bool
    status: TerminalStatus
    fault_code: ac.bits[32]
    fault_arg0: ac.bits[64]
    fault_bi: bool
    valid: bool
    done: bool
    handoff_pending: bool
    fault_valid: bool


@ac.struct
class RobHandoff:
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    required_owner_mask: ac.bits[2]
    received_owner_mask: ac.bits[2]
    mpq_histories_required: ac.bits[4]
    mpq_histories_acked: ac.bits[4]
    valid: bool
    durable: bool


@ac.struct
class BrobHandoffAck:
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    valid: bool
    durable: bool


@ac.struct
class MpqHistoryAckSlot:
    history_sequence: ac.bits[32]
    valid: bool


@ac.struct
class HandoffDiagnostic:
    owner_mask: ac.bits[2]
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    history_sequence: ac.bits[32]
    duplicate: bool
    unsolicited: bool
    identity_mismatch: bool
    kind_mismatch: bool
    invalid_response: bool
    durability_missing: bool
    capacity_error: bool
    coalesced_count: ac.bits[16]
    valid: bool


@ac.struct
class DispatchReservation:
    flow: FlowKey
    execution_class: ExecutionClass
    bank: ac.bits[4]
    slot: ac.bits[6]
    generation: ac.bits[16]
    valid: bool


@ac.struct
class LoadProducerToken:
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    destination_tag: ac.bits[7]
    destination_generation: ac.bits[16]
    load_attempt_generation: ac.bits[16]
    valid: bool


@ac.struct
class OperandSourceDescriptor:
    architectural_index: ac.u5
    constant_zero: bool
    tag: ac.bits[7]
    generation: ac.bits[16]
    valid: bool
    speculative: bool
    load_stage_mask: ac.bits[4]
    load_producer: LoadProducerToken


@ac.invariant
def valid_producer_identity(value: LoadProducerToken) -> bool:
    return (
        value.epoch.flow == value.inst.flow
        and value.epoch.flow == value.block.flow
        and value.epoch.flow == value.rob.flow
    )


@ac.invariant
def valid_operand_source(value: OperandSourceDescriptor) -> bool:
    return (
        (
            value.constant_zero
            and value.architectural_index == 0
            and not value.valid
            and value.tag == 0
            and value.generation == 0
        )
        or (
            not value.constant_zero
            and (
                (
                    value.valid
                    and value.architectural_index != 0
                    and value.architectural_index < 24
                )
                or (
                    not value.valid
                    and value.architectural_index == 0
                    and value.tag == 0
                    and value.generation == 0
                )
            )
        )
    ) and (
        (
            value.valid
            and value.speculative
            and value.load_producer.valid
            and value.load_producer.destination_tag == value.tag
            and value.load_producer.destination_generation == value.generation
            and value.load_stage_mask != 0
            and valid_producer_identity(value.load_producer)
        )
        or (
            not value.speculative
            and not value.load_producer.valid
            and value.load_stage_mask == 0
        )
    )


@ac.struct
class IssueIdentity:
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    dispatch: DispatchReservation
    isq_index: ac.bits[4]


@ac.struct
class IssueEntry:
    identity: IssueIdentity
    age: ac.bits[16]
    operation: IntAluOperation
    src0: OperandSourceDescriptor
    src0_ready: bool
    src0_value: ac.bits[64]
    src1: OperandSourceDescriptor
    src1_ready: bool
    src1_value: ac.bits[64]
    destination: PhysRef
    valid: bool


@ac.struct
class IssueAttemptKey:
    identity: IssueIdentity
    attempt_generation: ac.bits[16]


@ac.struct
class IssueAttempt:
    key: IssueAttemptKey
    entry: IssueEntry
    valid: bool


@ac.struct
class OperandReadRequest:
    attempt: IssueAttempt
    src0: OperandSourceDescriptor
    src1: OperandSourceDescriptor
    rf_read_mask: ac.bits[2]
    forward_mask: ac.bits[2]
    valid: bool


@ac.struct
class OperandReadResponse:
    request: OperandReadRequest
    src0_value: ac.bits[64]
    src1_value: ac.bits[64]
    src0_source: OperandSourceKind
    src1_source: OperandSourceKind
    granted_mask: ac.bits[2]
    denied_mask: ac.bits[2]
    valid: bool


@ac.struct
class GprEntry:
    flow: FlowKey
    generation: ac.u16
    value: ac.u64
    value_valid: bool
    allocated: bool


@ac.struct
class GprMaintenanceRequest:
    kind: GprMaintenanceKind
    flow: FlowKey
    phys: PhysRef
    value: ac.u64
    value_valid: bool
    valid: bool


@ac.struct
class GprReadySeed:
    flow: FlowKey
    phys: PhysRef
    ready: bool
    valid: bool


@ac.struct
class GprMaintenanceAck:
    request: GprMaintenanceRequest
    ready_seed: GprReadySeed
    accepted: bool
    initialized: bool
    allocated: bool
    reclaimed: bool
    stale_generation: bool
    ownership_mismatch: bool
    protocol_error: bool
    valid: bool


@ac.struct
class GprReadResult:
    request: OperandReadRequest
    src0_value: ac.u64
    src1_value: ac.u64
    granted_mask: ac.u2
    denied_mask: ac.u2
    stale_generation_mask: ac.u2
    ownership_mismatch_mask: ac.u2
    unallocated_mask: ac.u2
    unwritten_mask: ac.u2
    malformed: bool
    valid: bool


@ac.struct
class GprProjectionRequest:
    flow: FlowKey
    architectural_index: ac.u5
    phys: PhysRef
    query_sequence: ac.u32
    valid: bool


@ac.struct
class GprProjectionResult:
    request: GprProjectionRequest
    value: ac.u64
    found: bool
    zero_register: bool
    stale_generation: bool
    ownership_mismatch: bool
    unallocated: bool
    protocol_error: bool
    valid: bool


@ac.struct
class OperandResponseAck:
    response: OperandReadResponse
    accepted: bool
    valid: bool


@ac.struct
class OperandReadDecision:
    key: IssueAttemptKey
    granted: bool
    rf_granted_mask: ac.bits[2]
    forward_accepted_mask: ac.bits[2]
    valid: bool


@ac.struct
class OperandReadDecisionAck:
    request: OperandReadDecision
    accepted: bool
    transferred: bool
    retry: bool
    valid: bool


@ac.struct
class IssueCancel:
    key: IssueAttemptKey
    reason: IssueCancelReason
    valid: bool


@ac.struct
class IssueCancelAck:
    request: IssueCancel
    accepted: bool
    valid: bool


@ac.struct
class LoadDependencyResolution:
    consumer: IssueAttemptKey
    operand_index: ac.bits[1]
    producer: LoadProducerToken
    value: ac.bits[64]
    value_valid: bool
    hit: bool
    miss: bool
    replay: bool
    valid: bool


@ac.struct
class LoadDependencyAck:
    request: LoadDependencyResolution
    accepted: bool
    resolved: bool
    canceled: bool
    valid: bool


@ac.struct
class ExecutePacket:
    attempt: IssueAttempt
    lhs: ac.bits[64]
    rhs: ac.bits[64]
    lhs_source: OperandSourceKind
    rhs_source: OperandSourceKind
    valid: bool


@ac.struct
class ExecutionSinkDecision:
    key: IssueAttemptKey
    accepted: bool
    retry: bool
    valid: bool


@ac.struct
class ExecutionSinkDecisionAck:
    request: ExecutionSinkDecision
    matched: bool
    transferred: bool
    retry: bool
    valid: bool


@ac.struct
class IssueRelease:
    key: IssueAttemptKey
    dispatch: DispatchReservation
    valid: bool


@ac.struct
class AttemptTombstone:
    key: IssueAttemptKey
    valid: bool


@ac.struct
class StoreResolveData:
    lsid: ac.bits[64]
    stq_slot: ac.bits[16]
    stq_generation: ac.bits[16]
    address: ac.bits[64]
    address_ready: bool
    data_ready: bool
    valid: bool


@ac.struct
class BranchResolveData:
    predicted_taken: bool
    actual_taken: bool
    target_pc: ac.bits[64]
    target_valid: bool
    fallthrough_pc: ac.bits[64]
    checkpoint_id: ac.bits[16]
    branch_epoch: ac.bits[16]
    recovery_required: bool
    valid: bool


@ac.struct
class TerminalResult:
    source: TerminalSource
    status: TerminalStatus
    attempt: IssueAttemptKey
    destination: PhysRef
    destination_arch_index: ac.u5
    result: ac.bits[64]
    result_valid: bool
    fault_code: ac.bits[32]
    fault_arg0: ac.bits[64]
    fault_bi: bool
    fault_valid: bool
    store: StoreResolveData
    branch: BranchResolveData
    age_order: ac.bits[48]
    required_effect_mask: ac.bits[7]
    valid: bool


@ac.struct
class WbaEntry:
    result: TerminalResult
    slot: ac.bits[3]
    generation: ac.bits[16]
    published: bool
    canceled: bool
    completed: bool
    valid: bool


@ac.struct
class WritebackCommit:
    entry: WbaEntry
    effect_mask: ac.bits[7]
    valid: bool


@ac.struct
class WritebackApplyAck:
    attempt: IssueAttemptKey
    slot: ac.bits[3]
    generation: ac.bits[16]
    effect_mask: ac.bits[7]
    applied: bool
    retry: bool
    valid: bool


@ac.struct
class WritebackAckResult:
    request: WritebackApplyAck
    matched: bool
    cancel_completed: bool
    valid: bool


@ac.struct
class WritebackCancelAck:
    request: IssueCancel
    accepted: bool
    tombstoned: bool
    unpublished_cleared: bool
    apply_owned: bool
    already_completed: bool
    valid: bool


@ac.struct
class WbaDrainRequest:
    attempt: IssueAttemptKey
    wba_identity_valid: bool
    wba_slot: ac.bits[3]
    wba_generation: ac.bits[16]
    effect_mask: ac.bits[7]
    producer_drained: bool
    valid: bool


@ac.struct
class WbaDrainAck:
    request: WbaDrainRequest
    accepted: bool
    entry_reclaimed: bool
    cancel_tombstone_reclaimed: bool
    wba_released: bool
    valid: bool


@ac.struct
class GprWriteRequest:
    attempt: IssueAttemptKey
    destination: PhysRef
    architectural_index: ac.u5
    value: ac.u64
    valid: bool


@ac.struct
class GprWriteAck:
    request: GprWriteRequest
    accepted: bool
    applied: bool
    stale_generation: bool
    ownership_mismatch: bool
    unallocated: bool
    duplicate_write: bool
    protocol_error: bool
    valid: bool


@ac.struct
class W2PendingApply:
    commit: WritebackCommit
    phase: W2ApplyPhase
    history_slot: ac.u4
    history_reserved: bool
    gpr_request: GprWriteRequest
    gpr_ack: GprWriteAck
    valid: bool


@ac.struct
class ScalarPhysRef:
    register_class: ScalarRegisterClass
    phys: PhysRef
    valid: bool


@ac.struct
class RenameMapEntry:
    flow: FlowKey
    selector: ac.u5
    register: ScalarPhysRef
    producer: InstKey
    valid: bool


@ac.struct
class RenamePhysState:
    flow: FlowKey
    generation: ac.u16
    transaction_id: ac.u32
    free: bool
    reserved: bool
    valid: bool


@ac.struct
class RenameEpochState:
    flow: FlowKey
    recovery_epoch: ac.u16
    valid: bool


@ac.struct
class SourceVersionLease:
    flow: FlowKey
    register: ScalarPhysRef
    producer: InstKey
    lease_slot: ac.u32
    lease_generation: ac.u16
    owner: SourceLeaseOwner
    owner_generation: ac.u16
    valid: bool


@ac.struct
class ScalarLocalReadySeed:
    flow: FlowKey
    register: ScalarPhysRef
    ready: bool
    valid: bool


@ac.struct
class ScalarLocalMaintenanceRequest:
    flow: FlowKey
    register: ScalarPhysRef
    allocate: bool
    valid: bool


@ac.struct
class ScalarLocalMaintenanceAck:
    request: ScalarLocalMaintenanceRequest
    ready_seed: ScalarLocalReadySeed
    accepted: bool
    allocated: bool
    valid: bool


@ac.struct
class RenameUopRequest:
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    uop_index: ac.u4
    execution_class: ExecutionClass
    opcode_id: ac.u12
    src0_selector: ac.u5
    src0_valid: bool
    src1_selector: ac.u5
    src1_valid: bool
    src2_selector: ac.u5
    src2_valid: bool
    destination_selector: ac.u5
    destination_valid: bool
    preview_sequence: ac.u32
    valid: bool


@ac.struct
class RenameSourceBinding:
    selector: ac.u5
    register_class: ScalarRegisterClass
    register: ScalarPhysRef
    producer: InstKey
    constant_zero: bool
    lease: SourceVersionLease
    valid: bool


@ac.struct
class RenameDestinationBinding:
    selector: ac.u5
    kind: RenameDestinationKind
    old_register: ScalarPhysRef
    old_producer: InstKey
    new_register: ScalarPhysRef
    valid: bool


@ac.struct
class RenamePreview:
    request: RenameUopRequest
    transaction_id: ac.u32
    rename_epoch: ac.u16
    src0: RenameSourceBinding
    src1: RenameSourceBinding
    src2: RenameSourceBinding
    destination: RenameDestinationBinding
    valid: bool


@ac.struct
class RenamePreviewResult:
    request: RenameUopRequest
    preview: RenamePreview
    accepted: bool
    duplicate: bool
    source_unmapped: bool
    no_transaction_slot: bool
    no_physical_destination: bool
    identity_exhausted: bool
    ordered_frontier_busy: bool
    ownership_mismatch: bool
    malformed: bool
    valid: bool


@ac.struct
class RenameTxn:
    preview: RenamePreview
    phase: RenameTxnPhase
    rob: RobKey
    dispatch: DispatchReservation
    required_owner_mask: ac.u8
    received_owner_mask: ac.u8
    slot: ac.u4
    valid: bool


@ac.struct
class RenamePrepareRequest:
    preview: RenamePreview
    rob: RobKey
    dispatch: DispatchReservation
    required_owner_mask: ac.u8
    valid: bool


@ac.struct
class ScalarRenameHistory:
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    execution_class: ExecutionClass
    register_class: ScalarRegisterClass
    selector: ac.u5
    old_register: ScalarPhysRef
    old_producer: InstKey
    new_register: ScalarPhysRef
    rename_transaction_id: ac.u32
    source_lease_count: ac.u2
    src0_lease: SourceVersionLease
    src1_lease: SourceVersionLease
    src2_lease: SourceVersionLease
    lease_holder: SourceLeaseOwner
    history_sequence: ac.u32
    durable: bool
    valid: bool


@ac.struct
class RenameHolderDeliveryAck:
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    rename_transaction_id: ac.u32
    history_sequence: ac.u32
    holder: SourceLeaseOwner
    accepted: bool
    durable: bool
    valid: bool


@ac.struct
class MpqHolderAcquireRequest:
    flow: FlowKey
    recovery_epoch: ac.u16
    rename_transaction_id: ac.u32
    history_sequence: ac.u32
    requested_holder: SourceLeaseOwner
    delivery_ack: RenameHolderDeliveryAck
    durable: bool
    valid: bool


@ac.struct
class MpqHolderAcquireAck:
    request: MpqHolderAcquireRequest
    history: ScalarRenameHistory
    accepted: bool
    stale: bool
    holder_mismatch: bool
    delivery_mismatch: bool
    lease_owner_mismatch: bool
    generation_exhausted: bool
    valid: bool


@ac.struct
class RenamePrepareBundle:
    request: RenamePrepareRequest
    gpr_allocate: GprMaintenanceRequest
    local_allocate: ScalarLocalMaintenanceRequest
    history: ScalarRenameHistory
    src0_lease: SourceVersionLease
    src1_lease: SourceVersionLease
    src2_lease: SourceVersionLease
    accepted: bool
    stale: bool
    owner_mask_error: bool
    valid: bool


@ac.struct
class RenamePublishRequest:
    transaction_id: ac.u32
    flow: FlowKey
    rename_epoch: ac.u16
    gpr_ack: GprMaintenanceAck
    local_ack: ScalarLocalMaintenanceAck
    history_ack: ScalarRenameHistory
    holder_ack: MpqHolderAcquireAck
    received_owner_mask: ac.u8
    valid: bool


@ac.struct
class RenamedUop:
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    dispatch: DispatchReservation
    uop_index: ac.u4
    execution_class: ExecutionClass
    opcode_id: ac.u12
    src0: RenameSourceBinding
    src1: RenameSourceBinding
    src2: RenameSourceBinding
    destination: RenameDestinationBinding
    valid: bool


@ac.struct
class RenamePublication:
    request: RenamePublishRequest
    renamed_uop: RenamedUop
    history: ScalarRenameHistory
    src0_lease: SourceVersionLease
    src1_lease: SourceVersionLease
    src2_lease: SourceVersionLease
    accepted: bool
    stale: bool
    owner_incomplete: bool
    gpr_ack_error: bool
    local_ack_error: bool
    history_ack_error: bool
    holder_ack_error: bool
    valid: bool


@ac.struct
class RenameCancelRequest:
    transaction_id: ac.u32
    flow: FlowKey
    recovery_epoch: ac.u16
    valid: bool


@ac.struct
class RenameCancelAck:
    request: RenameCancelRequest
    accepted: bool
    reservation_released: bool
    compensation_required: bool
    already_published: bool
    already_canceled: bool
    stale: bool
    valid: bool


@ac.struct
class RenameReclaimEvidence:
    kind: RenameReclaimEvidenceKind
    flow: FlowKey
    register: ScalarPhysRef
    producer: InstKey
    proof_generation: ac.u16
    accepted: bool
    durable: bool
    valid: bool


@ac.struct
class RenameReclaimProof:
    flow: FlowKey
    register: ScalarPhysRef
    producer: InstKey
    proof_generation: ac.u16
    mapping_displacement: RenameReclaimEvidence
    mpq_zero_lease: RenameReclaimEvidence
    data_owner_drain: RenameReclaimEvidence
    wba_drain: RenameReclaimEvidence
    w2_drain: RenameReclaimEvidence
    late_producer_drain: RenameReclaimEvidence
    valid: bool


@ac.struct
class RenameMaintenanceRequest:
    kind: RenameMaintenanceKind
    flow: FlowKey
    selector: ac.u5
    register: ScalarPhysRef
    producer: InstKey
    transaction_id: ac.u32
    recovery_epoch: ac.u16
    owner_mask: ac.u8
    mapping_valid: bool
    physical_free: bool
    reclaim_proof: RenameReclaimProof
    durable: bool
    valid: bool


@ac.struct
class RenameMaintenanceAck:
    request: RenameMaintenanceRequest
    accepted: bool
    initialized: bool
    reclaimed: bool
    drained: bool
    stale: bool
    ownership_mismatch: bool
    valid: bool


@ac.struct
class MpqHistoryEntry:
    history: ScalarRenameHistory
    mapping_evidence: RenameReclaimEvidence
    holder_acquired: bool
    rob_handoff_durable: bool
    mapping_released: bool
    valid: bool


@ac.struct
class MpqHandoffAck:
    request: RobEvent
    history: ScalarRenameHistory
    accepted: bool
    stale: bool
    valid: bool


@ac.struct
class MpqLeaseUpdateRequest:
    kind: MpqLeaseUpdateKind
    history_sequence: ac.u32
    lease: SourceVersionLease
    next_owner: SourceLeaseOwner
    durable: bool
    valid: bool


@ac.struct
class MpqLeaseUpdateAck:
    request: MpqLeaseUpdateRequest
    updated_lease: SourceVersionLease
    accepted: bool
    stale: bool
    owner_mismatch: bool
    generation_exhausted: bool
    valid: bool


@ac.struct
class MpqMappingRelease:
    history: ScalarRenameHistory
    mapping_evidence: RenameReclaimEvidence
    durable: bool
    valid: bool


@ac.struct
class MpqMappingReleaseAck:
    request: MpqMappingRelease
    accepted: bool
    stale: bool
    evidence_error: bool
    valid: bool


@ac.struct
class MpqAbortRequest:
    flow: FlowKey
    recovery_epoch: ac.u16
    rename_transaction_id: ac.u32
    history_sequence: ac.u32
    durable: bool
    valid: bool


@ac.struct
class MpqAbortAck:
    request: MpqAbortRequest
    history: ScalarRenameHistory
    accepted: bool
    stale: bool
    holder_leases_live: bool
    ren_private_leases_dropped: bool
    already_handed_off: bool
    mapping_state_conflict: bool
    valid: bool


@ac.struct
class MpqReclaimCandidate:
    history: ScalarRenameHistory
    mapping_evidence: RenameReclaimEvidence
    mpq_zero_lease: RenameReclaimEvidence
    physical_reclaim_required: bool
    valid: bool


@ac.struct
class WakeupPublication:
    attempt: IssueAttemptKey
    destination: PhysRef
    ready: bool
    valid: bool


@ac.struct
class RobCompletion:
    attempt: IssueAttemptKey
    status: TerminalStatus
    result: ac.bits[64]
    result_valid: bool
    fault_code: ac.bits[32]
    fault_arg0: ac.bits[64]
    fault_bi: bool
    fault_valid: bool
    valid: bool


@ac.struct
class BrobResolve:
    attempt: IssueAttemptKey
    status: TerminalStatus
    store: StoreResolveData
    branch: BranchResolveData
    fault_code: ac.bits[32]
    fault_arg0: ac.bits[64]
    fault_bi: bool
    fault_valid: bool
    valid: bool


@ac.struct
class StoreResolve:
    attempt: IssueAttemptKey
    store: StoreResolveData
    valid: bool


@ac.struct
class BranchResolve:
    attempt: IssueAttemptKey
    branch: BranchResolveData
    valid: bool


@ac.struct
class FaultPublication:
    attempt: IssueAttemptKey
    fault_code: ac.bits[32]
    fault_arg0: ac.bits[64]
    fault_bi: bool
    valid: bool


@ac.struct
class W2CancelAck:
    request: IssueCancel
    accepted: bool
    tombstoned: bool
    late_after_apply: bool
    capacity_error: bool
    valid: bool


@ac.struct
class W2AppliedRecord:
    attempt: IssueAttemptKey
    wba_slot: ac.bits[3]
    wba_generation: ac.bits[16]
    effect_mask: ac.bits[7]
    valid: bool


@ac.struct
class W2DrainRequest:
    attempt: IssueAttemptKey
    wba_identity_valid: bool
    wba_slot: ac.bits[3]
    wba_generation: ac.bits[16]
    effect_mask: ac.bits[7]
    producer_drained: bool
    wba_released: bool
    valid: bool


@ac.struct
class W2DrainAck:
    request: W2DrainRequest
    accepted: bool
    cancel_tombstone_reclaimed: bool
    applied_history_reclaimed: bool
    valid: bool


@ac.struct
class WakeupEvent:
    flow: FlowKey
    tag: ac.bits[7]
    generation: ac.bits[16]
    ready: bool


@ac.struct
class ReadyState:
    generation: ac.bits[16]
    ready: bool


@ac.struct
class RecoveryEvent:
    epoch: EpochKey
    valid: bool


@ac.struct
class AluPacket:
    epoch: EpochKey
    inst: InstKey
    block: BlockKey
    rob: RobKey
    operation: IntAluOperation
    lhs: ac.bits[64]
    rhs: ac.bits[64]
    destination_tag: ac.bits[7]
    destination_generation: ac.bits[16]
    result: ac.bits[64]
    completed: bool

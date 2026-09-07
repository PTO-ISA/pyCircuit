# ndf: node=ndf://davincioo/DAV-SPE-IEX-I2-0002
"""I2 operand join and non-cancellable execution-transfer owner."""

# ruff: noqa: F841

from __future__ import annotations

import agentic_circuit as ac

from designs.davincioo.contracts.spe import (
    AttemptTombstone,
    ExecutePacket,
    ExecutionSinkDecision,
    ExecutionSinkDecisionAck,
    IssueAttempt,
    IssueCancel,
    IssueCancelAck,
    IssueCancelReason,
    IssueRelease,
    LoadDependencyAck,
    LoadDependencyResolution,
    OperandReadResponse,
    OperandResponseAck,
    OperandSourceKind,
    valid_operand_source,
)


@ac.rule
def capture_operands(
    active,
    pending,
    lhs,
    rhs,
    lhs_source,
    rhs_source,
    src0_resolved,
    src1_resolved,
    cancel_tombstones,
    response,
):
    request = response.request
    attempt = request.attempt
    entry = attempt.entry
    key = attempt.key
    flow = key.identity.epoch.flow
    src0 = request.src0
    src1 = request.src1
    all_required = request.rf_read_mask | request.forward_mask
    expected_rf_read_mask = 0
    expected_forward_mask = 0
    if entry.src0.valid:
        if entry.src0.speculative:
            expected_forward_mask = expected_forward_mask | 0b01
        else:
            expected_rf_read_mask = expected_rf_read_mask | 0b01
    if entry.src1.valid:
        if entry.src1.speculative:
            expected_forward_mask = expected_forward_mask | 0b10
        else:
            expected_rf_read_mask = expected_rf_read_mask | 0b10

    tombstone = ac.find(
        cancel_tombstones,
        where=lambda row: row.valid & (row.key == key),
    )
    identity = entry.identity
    identity_coherent = (
        (identity.epoch.flow == identity.inst.flow)
        & (identity.epoch.flow == identity.block.flow)
        & (identity.epoch.flow == identity.rob.flow)
        & (identity.epoch.flow == identity.dispatch.flow)
    )
    descriptor_matches = (src0 == entry.src0) & (src1 == entry.src1)
    producer_flows_match = (
        (not src0.speculative) | (flow == src0.load_producer.epoch.flow)
    ) & ((not src1.speculative) | (flow == src1.load_producer.epoch.flow))
    descriptors_valid = valid_operand_source(entry.src0) & valid_operand_source(
        entry.src1
    )
    src0_kind_valid = (
        (
            ((all_required & 0b01) == 0)
            & (response.src0_source == OperandSourceKind.NONE)
        )
        | (
            ((request.rf_read_mask & 0b01) != 0)
            & (
                (response.src0_source == OperandSourceKind.REGISTER_FILE)
                | (response.src0_source == OperandSourceKind.BYPASS)
            )
        )
        | (
            ((request.forward_mask & 0b01) != 0)
            & (response.src0_source == OperandSourceKind.LOAD_FORWARD)
        )
    )
    src1_kind_valid = (
        (
            ((all_required & 0b10) == 0)
            & (response.src1_source == OperandSourceKind.NONE)
        )
        | (
            ((request.rf_read_mask & 0b10) != 0)
            & (
                (response.src1_source == OperandSourceKind.REGISTER_FILE)
                | (response.src1_source == OperandSourceKind.BYPASS)
            )
        )
        | (
            ((request.forward_mask & 0b10) != 0)
            & (response.src1_source == OperandSourceKind.LOAD_FORWARD)
        )
    )
    identity_matches = (
        (key.identity == identity)
        & identity_coherent
        & descriptor_matches
        & producer_flows_match
        & descriptors_valid
    )
    response_shape_valid = (
        response.valid
        & request.valid
        & attempt.valid
        & entry.valid
        & identity.dispatch.valid
        & (response.granted_mask == all_required)
        & (response.denied_mask == 0)
        & (request.rf_read_mask == expected_rf_read_mask)
        & (request.forward_mask == expected_forward_mask)
        & ((request.rf_read_mask & request.forward_mask) == 0)
        & src0_kind_valid
        & src1_kind_valid
    )
    was_inactive = not active
    accepted = (
        was_inactive & (not tombstone.valid) & identity_matches & response_shape_valid
    )
    if accepted:
        pending = attempt
        lhs = response.src0_value
        rhs = response.src1_value
        lhs_source = response.src0_source
        rhs_source = response.src1_source
        src0_resolved = (request.forward_mask & 0b01) == 0
        src1_resolved = (request.forward_mask & 0b10) == 0
        if not entry.src0.valid:
            lhs = 0
        if not entry.src1.valid:
            rhs = 0
        active = True
    if was_inactive:
        return OperandResponseAck(response=response, accepted=accepted, valid=True)


@ac.rule
def accept_load_dependency(
    active,
    pending,
    lhs,
    rhs,
    src0_resolved,
    src1_resolved,
    cancel_pending,
    cancel_payload,
    request,
):
    source = pending.entry.src0.load_producer
    source_unresolved = not src0_resolved
    if request.operand_index != 0:
        source = pending.entry.src1.load_producer
        source_unresolved = not src1_resolved
    key_matches = active & request.valid & (pending.key == request.consumer)
    producer_matches = (
        source.valid & request.producer.valid & (source == request.producer)
    )
    hit_shape = request.hit & (not request.miss) & (not request.replay)
    cancel_shape = (not request.hit) & (request.miss != request.replay)
    accepted = (
        key_matches
        & producer_matches
        & source_unresolved
        & (hit_shape | cancel_shape)
        & ((not request.hit) | request.value_valid)
        & (not cancel_pending)
    )
    if accepted:
        if request.hit:
            if request.operand_index == 0:
                lhs = request.value
                src0_resolved = True
            else:
                rhs = request.value
                src1_resolved = True
        else:
            reason = IssueCancelReason.LOAD_MISS
            if request.replay:
                reason = IssueCancelReason.LOAD_REPLAY
            cancel_payload = IssueCancel(key=pending.key, reason=reason, valid=True)
            cancel_pending = True
    return LoadDependencyAck(
        request=request,
        accepted=accepted,
        resolved=accepted & request.hit,
        canceled=accepted & (not request.hit),
        valid=True,
    )


@ac.rule
def publish_execute(
    active,
    pending,
    lhs,
    rhs,
    lhs_source,
    rhs_source,
    src0_resolved,
    src1_resolved,
    execute_outstanding,
    release_pending,
    cancel_pending,
):
    if (
        active
        & src0_resolved
        & src1_resolved
        & (not execute_outstanding)
        & (not release_pending)
        & (not cancel_pending)
    ):
        execute_outstanding = True
        return ExecutePacket(
            attempt=pending,
            lhs=lhs,
            rhs=rhs,
            lhs_source=lhs_source,
            rhs_source=rhs_source,
            valid=True,
        )


@ac.rule
def accept_sink_decision(
    active, pending, execute_outstanding, release_pending, request
):
    key_matches = (
        active
        & execute_outstanding
        & request.valid
        & (request.accepted != request.retry)
        & (pending.key == request.key)
    )
    if key_matches:
        execute_outstanding = False
        if request.accepted:
            release_pending = True
    return ExecutionSinkDecisionAck(
        request=request,
        matched=key_matches,
        transferred=key_matches & request.accepted,
        retry=key_matches & request.retry,
        valid=True,
    )


@ac.rule
def publish_release(active, pending, release_pending):
    if active & release_pending:
        released = IssueRelease(
            key=pending.key,
            dispatch=pending.entry.identity.dispatch,
            valid=True,
        )
        active = False
        pending = pending.with_fields(valid=False)
        release_pending = False
        return released


@ac.rule
def publish_generated_cancel(
    active,
    pending,
    execute_outstanding,
    release_pending,
    cancel_pending,
    cancel_payload,
    cancel_tombstones,
):
    key = cancel_payload.key
    tombstone = ac.find(
        cancel_tombstones,
        where=lambda row: row.valid & (row.key == key),
    )
    free = ac.find(cancel_tombstones, where=lambda row: not row.valid)
    if active & cancel_pending & (tombstone.valid | free.valid):
        tombstone_index = free.index
        tombstone_value = AttemptTombstone(
            key=cancel_payload.key,
            valid=True,
        )
        if tombstone.valid:
            tombstone_index = tombstone.index
            tombstone_value = tombstone.value
        cancel_tombstones[tombstone_index] = tombstone_value
        active = False
        pending = pending.with_fields(valid=False)
        execute_outstanding = False
        release_pending = False
        cancel_pending = False
        return cancel_payload


@ac.rule
def accept_external_cancel(
    active,
    pending,
    execute_outstanding,
    release_pending,
    cancel_pending,
    cancel_tombstones,
    request,
):
    active_key_matches = active & (pending.key == request.key)
    tombstone = ac.find(
        cancel_tombstones,
        where=lambda row: row.valid & (row.key == request.key),
    )
    free = ac.find(cancel_tombstones, where=lambda row: not row.valid)
    post_accept_cancel = active_key_matches & release_pending
    matched = request.valid & (not post_accept_cancel) & (tombstone.valid | free.valid)
    if matched & (not tombstone.valid):
        cancel_tombstones[free.index] = AttemptTombstone(
            key=request.key,
            valid=True,
        )
    if matched & active_key_matches:
        active = False
        pending = pending.with_fields(valid=False)
        execute_outstanding = False
        release_pending = False
        cancel_pending = False
    return IssueCancelAck(request=request, accepted=matched, valid=True)


@ac.module
def i2(
    operand_response: OperandReadResponse,
    load_dependency: LoadDependencyResolution,
    sink_decision: ExecutionSinkDecision,
    cancel: IssueCancel,
) -> tuple[
    ExecutePacket,
    OperandResponseAck,
    LoadDependencyAck,
    ExecutionSinkDecisionAck,
    IssueRelease,
    IssueCancel,
    IssueCancelAck,
]:
    """Join operands and release ISQ only after durable execution acceptance."""

    active: bool = False
    pending: IssueAttempt = 0
    lhs: ac.bits[64] = 0
    rhs: ac.bits[64] = 0
    lhs_source: OperandSourceKind = OperandSourceKind.NONE
    rhs_source: OperandSourceKind = OperandSourceKind.NONE
    src0_resolved: bool = False
    src1_resolved: bool = False
    execute_outstanding: bool = False
    release_pending: bool = False
    cancel_pending: bool = False
    cancel_payload: IssueCancel = 0
    cancel_tombstones: list[AttemptTombstone] = [0] * 16

    operand_ack = capture_operands(
        active,
        pending,
        lhs,
        rhs,
        lhs_source,
        rhs_source,
        src0_resolved,
        src1_resolved,
        cancel_tombstones,
        operand_response,
    )
    dependency_ack = accept_load_dependency(
        active,
        pending,
        lhs,
        rhs,
        src0_resolved,
        src1_resolved,
        cancel_pending,
        cancel_payload,
        load_dependency,
    )
    execute_request = publish_execute(
        active,
        pending,
        lhs,
        rhs,
        lhs_source,
        rhs_source,
        src0_resolved,
        src1_resolved,
        execute_outstanding,
        release_pending,
        cancel_pending,
    )
    sink_decision_ack = accept_sink_decision(
        active,
        pending,
        execute_outstanding,
        release_pending,
        sink_decision,
    )
    release = publish_release(active, pending, release_pending)
    generated_cancel = publish_generated_cancel(
        active,
        pending,
        execute_outstanding,
        release_pending,
        cancel_pending,
        cancel_payload,
        cancel_tombstones,
    )
    cancel_ack = accept_external_cancel(
        active,
        pending,
        execute_outstanding,
        release_pending,
        cancel_pending,
        cancel_tombstones,
        cancel,
    )
    return (
        execute_request,
        operand_ack,
        dependency_ack,
        sink_decision_ack,
        release,
        generated_cancel,
        cancel_ack,
    )


@ac.system
def i2_system(
    operand_response: OperandReadResponse,
    load_dependency: LoadDependencyResolution,
    sink_decision: ExecutionSinkDecision,
    cancel: IssueCancel,
) -> tuple[
    ExecutePacket,
    OperandResponseAck,
    LoadDependencyAck,
    ExecutionSinkDecisionAck,
    IssueRelease,
    IssueCancel,
    IssueCancelAck,
]:
    """Executable boundary used by design-local compile and simulation gates."""
    return i2(operand_response, load_dependency, sink_decision, cancel)

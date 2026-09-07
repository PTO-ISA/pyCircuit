# ndf-draft: node=ndf://davincioo/DAV-SPE-IEX-I1-0002
"""Retryable I1 compound operand-read owner."""

# ruff: noqa: F841

from __future__ import annotations

import agentic_circuit as ac

from designs.davincioo.contracts.spe import (
    IssueAttempt,
    IssueCancel,
    IssueCancelAck,
    OperandReadDecision,
    OperandReadDecisionAck,
    OperandReadRequest,
    valid_operand_source,
)


@ac.rule
def capture_attempt(active, pending, rf_read_mask, forward_mask, selected):
    flow = selected.key.identity.epoch.flow
    src0 = selected.entry.src0
    src1 = selected.entry.src1
    src0_flow_matches = (not src0.speculative) | (flow == src0.load_producer.epoch.flow)
    src1_flow_matches = (not src1.speculative) | (flow == src1.load_producer.epoch.flow)
    identity_matches = (
        (selected.key.identity == selected.entry.identity)
        & valid_operand_source(src0)
        & valid_operand_source(src1)
    )
    identity_matches = identity_matches & src0_flow_matches & src1_flow_matches
    if (not active) & selected.valid & selected.entry.valid & identity_matches:
        next_rf_read_mask = ac.concat(
            src1.valid & (not src1.speculative),
            src0.valid & (not src0.speculative),
        )
        next_forward_mask = ac.concat(
            src1.valid & src1.speculative,
            src0.valid & src0.speculative,
        )
        pending = selected
        rf_read_mask = next_rf_read_mask
        forward_mask = next_forward_mask
        active = True


@ac.rule
def publish_read_request(
    active,
    pending,
    rf_read_mask,
    forward_mask,
    read_request_outstanding,
):
    if active & (not read_request_outstanding):
        request = OperandReadRequest(
            attempt=pending,
            src0=pending.entry.src0,
            src1=pending.entry.src1,
            rf_read_mask=rf_read_mask,
            forward_mask=forward_mask,
            valid=True,
        )
        read_request_outstanding = True
        return request


@ac.rule
def accept_read_decision(
    active,
    pending,
    rf_read_mask,
    forward_mask,
    read_request_outstanding,
    request,
):
    identity_matches = (
        active & read_request_outstanding & request.valid & (pending.key == request.key)
    )
    grant_shape = (
        request.granted
        & (request.rf_granted_mask == rf_read_mask)
        & (request.forward_accepted_mask == forward_mask)
    )
    denial_shape = (
        (not request.granted)
        & (request.rf_granted_mask == 0)
        & (request.forward_accepted_mask == 0)
    )
    accepted = identity_matches & (grant_shape | denial_shape)
    if accepted:
        read_request_outstanding = False
        if request.granted:
            active = False
            pending = pending.with_fields(valid=False)
    return OperandReadDecisionAck(
        request=request,
        accepted=accepted,
        transferred=accepted & request.granted,
        retry=accepted & (not request.granted),
        valid=True,
    )


@ac.rule
def cancel_attempt(active, pending, read_request_outstanding, request):
    identity_matches = active & request.valid & (pending.key == request.key)
    if identity_matches:
        active = False
        pending = pending.with_fields(valid=False)
        read_request_outstanding = False
    return IssueCancelAck(
        request=request,
        accepted=identity_matches,
        valid=True,
    )


@ac.module
def i1(
    selected: IssueAttempt,
    cancel: IssueCancel,
    read_decision: OperandReadDecision,
) -> tuple[OperandReadRequest, OperandReadDecisionAck, IssueCancelAck]:
    """Hold one P1 attempt until exact read transfer or cancellation."""

    active: bool = False
    pending: IssueAttempt = 0
    rf_read_mask: ac.bits[2] = 0
    forward_mask: ac.bits[2] = 0
    read_request_outstanding: bool = False

    capture_attempt(active, pending, rf_read_mask, forward_mask, selected)
    read_request = publish_read_request(
        active,
        pending,
        rf_read_mask,
        forward_mask,
        read_request_outstanding,
    )
    read_decision_ack = accept_read_decision(
        active,
        pending,
        rf_read_mask,
        forward_mask,
        read_request_outstanding,
        read_decision,
    )
    cancel_ack = cancel_attempt(
        active,
        pending,
        read_request_outstanding,
        cancel,
    )
    return read_request, read_decision_ack, cancel_ack

# Source behavior: hengliao1972/DavinciOO@b81ecfc2b634d41886b01fd8724905eeb5bb6551
"""Retained terminal-result arbiter producing composite writeback commits."""

import agentic_circuit as ac

from designs.davincioo.contracts.spe import (
    WRITEBACK_EFFECT_BRANCH,
    WRITEBACK_EFFECT_BROB,
    WRITEBACK_EFFECT_FAULT,
    WRITEBACK_EFFECT_GPR,
    WRITEBACK_EFFECT_ROB,
    WRITEBACK_EFFECT_STORE,
    WRITEBACK_EFFECT_WAKEUP,
    AttemptTombstone,
    ExecutionClass,
    IssueCancel,
    TerminalResult,
    TerminalSource,
    TerminalStatus,
    WbaDrainAck,
    WbaDrainRequest,
    WbaEntry,
    WritebackAckResult,
    WritebackApplyAck,
    WritebackCancelAck,
    WritebackCommit,
)


@ac.rule
def ingest_terminal(entries, cancel_tombstones, lane, request):
    free = ac.find(entries, where=lambda entry: not entry.valid)
    old = entries[free.index]
    duplicate = ac.find(
        entries,
        where=lambda entry: entry.valid & (entry.result.attempt == request.attempt),
    )
    tombstone = ac.find(
        cancel_tombstones,
        where=lambda row: row.valid & (row.key == request.attempt),
    )
    effects = request.required_effect_mask
    has_gpr = (effects & WRITEBACK_EFFECT_GPR) != 0
    has_wakeup = (effects & WRITEBACK_EFFECT_WAKEUP) != 0
    has_rob = (effects & WRITEBACK_EFFECT_ROB) != 0
    has_brob = (effects & WRITEBACK_EFFECT_BROB) != 0
    has_fault = (effects & WRITEBACK_EFFECT_FAULT) != 0
    has_store = (effects & WRITEBACK_EFFECT_STORE) != 0
    has_branch = (effects & WRITEBACK_EFFECT_BRANCH) != 0
    base = (
        request.valid
        & (request.status != TerminalStatus.CANCELLED)
        & (effects != 0)
        & has_rob
        & ((not (has_gpr | has_wakeup)) | request.destination.valid)
        & ((not has_gpr) | request.result_valid)
    )
    destination_shape = (
        has_gpr
        & (request.destination_arch_index != 0)
        & (request.destination_arch_index < 24)
    ) | ((not has_gpr) & (request.destination_arch_index == 0))
    status_shape = (
        (
            (request.status == TerminalStatus.VALUE)
            & request.result_valid
            & request.destination.valid
            & has_gpr
            & has_wakeup
            & (not request.fault_valid)
        )
        | (
            (request.status == TerminalStatus.NO_DESTINATION)
            & (not request.result_valid)
            & (not request.destination.valid)
            & (not has_gpr)
            & (not has_wakeup)
            & (not request.fault_valid)
        )
        | (
            (request.status == TerminalStatus.FAULT)
            & request.fault_valid
            & (not request.result_valid)
            & has_fault
            & (not has_gpr)
            & (not has_wakeup)
        )
        | (
            (request.status == TerminalStatus.STORE)
            & (not request.result_valid)
            & has_store
            & (not has_gpr)
            & (not has_wakeup)
            & (not request.fault_valid)
        )
        | (
            (request.status == TerminalStatus.BRANCH)
            & (not request.result_valid)
            & has_branch
            & (not has_gpr)
            & (not has_wakeup)
            & (not request.fault_valid)
        )
    )
    allowed_effects = WRITEBACK_EFFECT_ROB | WRITEBACK_EFFECT_BROB
    if request.status == TerminalStatus.VALUE:
        allowed_effects = (
            WRITEBACK_EFFECT_GPR
            | WRITEBACK_EFFECT_WAKEUP
            | WRITEBACK_EFFECT_ROB
            | WRITEBACK_EFFECT_BROB
        )
    elif request.status == TerminalStatus.FAULT:
        allowed_effects = (
            WRITEBACK_EFFECT_ROB | WRITEBACK_EFFECT_BROB | WRITEBACK_EFFECT_FAULT
        )
    elif request.status == TerminalStatus.STORE:
        allowed_effects = (
            WRITEBACK_EFFECT_ROB | WRITEBACK_EFFECT_BROB | WRITEBACK_EFFECT_STORE
        )
    elif request.status == TerminalStatus.BRANCH:
        allowed_effects = (
            WRITEBACK_EFFECT_ROB | WRITEBACK_EFFECT_BROB | WRITEBACK_EFFECT_BRANCH
        )
    exact_effect_shape = (effects & allowed_effects) == effects
    resolve_shape = (
        ((not has_store) | has_brob)
        & ((not has_branch) | has_brob)
        & ((not has_fault) | has_brob)
    )
    source_valid = (
        (request.source == TerminalSource.ALU)
        | (request.source == TerminalSource.BRU)
        | (request.source == TerminalSource.AGU)
        | (request.source == TerminalSource.LSU)
        | (request.source == TerminalSource.STD)
        | (request.source == TerminalSource.FSU)
        | (request.source == TerminalSource.DIV)
        | (request.source == TerminalSource.SYS)
        | (request.source == TerminalSource.CMD)
    )
    source_status_shape = (
        (request.status != TerminalStatus.BRANCH)
        | (request.source == TerminalSource.BRU)
    ) & (
        (request.status != TerminalStatus.STORE)
        | (request.source == TerminalSource.AGU)
        | (request.source == TerminalSource.LSU)
        | (request.source == TerminalSource.STD)
    )
    sidecar_shape = (
        (
            (request.status == TerminalStatus.STORE)
            & request.store.valid
            & request.store.address_ready
            & request.store.data_ready
            & (not request.branch.valid)
        )
        | (
            (request.status == TerminalStatus.BRANCH)
            & request.branch.valid
            & request.branch.target_valid
            & (not request.store.valid)
        )
        | (
            (request.status != TerminalStatus.STORE)
            & (request.status != TerminalStatus.BRANCH)
            & (not request.store.valid)
            & (not request.branch.valid)
        )
    )
    source_dispatch_shape = (
        (
            (request.source == TerminalSource.ALU)
            & (request.attempt.identity.dispatch.execution_class == ExecutionClass.ALU)
        )
        | (
            (request.source == TerminalSource.BRU)
            & (request.attempt.identity.dispatch.execution_class == ExecutionClass.BRU)
        )
        | (
            (
                (request.source == TerminalSource.AGU)
                | (request.source == TerminalSource.LSU)
            )
            & (request.attempt.identity.dispatch.execution_class == ExecutionClass.AGU)
        )
        | (
            (request.source == TerminalSource.STD)
            & (request.attempt.identity.dispatch.execution_class == ExecutionClass.STD)
        )
        | (
            (request.source == TerminalSource.FSU)
            & (request.attempt.identity.dispatch.execution_class == ExecutionClass.FSU)
        )
        | (
            (request.source == TerminalSource.DIV)
            & (request.attempt.identity.dispatch.execution_class == ExecutionClass.DIV)
        )
        | (
            (request.source == TerminalSource.SYS)
            & (request.attempt.identity.dispatch.execution_class == ExecutionClass.SYS)
        )
        | (
            (request.source == TerminalSource.CMD)
            & (request.attempt.identity.dispatch.execution_class == ExecutionClass.CMD)
        )
    )
    lane_source_shape = (
        ((lane == 0) & (request.source == TerminalSource.ALU))
        | ((lane == 1) & (request.source == TerminalSource.BRU))
        | (
            (lane == 2)
            & (
                (request.source == TerminalSource.AGU)
                | (request.source == TerminalSource.LSU)
                | (request.source == TerminalSource.STD)
            )
        )
        | (
            (lane == 3)
            & (
                (request.source == TerminalSource.FSU)
                | (request.source == TerminalSource.DIV)
                | (request.source == TerminalSource.SYS)
                | (request.source == TerminalSource.CMD)
            )
        )
    )
    if (
        free.valid
        & (not duplicate.valid)
        & (not tombstone.valid)
        & source_valid
        & source_status_shape
        & destination_shape
        & sidecar_shape
        & source_dispatch_shape
        & lane_source_shape
        & base
        & status_shape
        & exact_effect_shape
        & resolve_shape
    ):
        entries[free.index] = WbaEntry(
            result=request,
            slot=free.index,
            generation=old.generation + 1,
            published=False,
            canceled=False,
            completed=False,
            valid=True,
        )


@ac.rule
def publish_oldest(entries):
    selected = ac.find(
        entries,
        where=lambda entry: (
            entry.valid
            & (not entry.published)
            & (not entry.canceled)
            & (not entry.completed)
        ),
        key=lambda entry: entry.result.age_order,
    )
    if selected.valid:
        published = selected.value.with_fields(published=True)
        entries[selected.index] = published
        return WritebackCommit(
            entry=published,
            effect_mask=selected.value.result.required_effect_mask,
            valid=True,
        )


@ac.rule
def accept_apply_ack(entries, request):
    selected = ac.find(
        entries,
        where=lambda entry: (
            entry.valid
            & entry.published
            & (entry.slot == request.slot)
            & (entry.generation == request.generation)
            & (entry.result.required_effect_mask == request.effect_mask)
            & (entry.result.attempt == request.attempt)
        ),
    )
    shape_valid = (
        request.valid
        & (request.applied != request.retry)
        & ((not request.applied) | (not selected.value.canceled))
    )
    matched = shape_valid & selected.valid
    cancel_completed = matched & request.retry & selected.value.canceled
    if matched:
        if request.applied:
            entries[selected.index] = selected.value.with_fields(
                published=False, completed=True
            )
        elif selected.value.canceled:
            entries[selected.index] = selected.value.with_fields(
                published=False, completed=True
            )
        else:
            entries[selected.index] = selected.value.with_fields(published=False)
    return WritebackAckResult(
        request=request,
        matched=matched,
        cancel_completed=cancel_completed,
        valid=True,
    )


@ac.rule
def cancel_unpublished(entries, cancel_tombstones, request):
    target = ac.find(
        entries,
        where=lambda entry: (
            entry.valid & (not entry.completed) & (entry.result.attempt == request.key)
        ),
    )
    completed = ac.find(
        entries,
        where=lambda entry: (
            entry.valid & entry.completed & (entry.result.attempt == request.key)
        ),
    )
    tombstone = ac.find(
        cancel_tombstones,
        where=lambda row: row.valid & (row.key == request.key),
    )
    free = ac.find(cancel_tombstones, where=lambda row: not row.valid)
    accepted = request.valid & (not completed.valid) & (tombstone.valid | free.valid)
    if accepted & (not tombstone.valid):
        cancel_tombstones[free.index] = AttemptTombstone(key=request.key, valid=True)
    if accepted & target.valid:
        entries[target.index] = target.value.with_fields(
            valid=target.value.published,
            published=target.value.published,
            canceled=target.value.published,
        )
    return WritebackCancelAck(
        request=request,
        accepted=accepted,
        tombstoned=accepted,
        unpublished_cleared=accepted & target.valid & (not target.value.published),
        apply_owned=accepted & target.valid & target.value.published,
        already_completed=completed.valid,
        valid=True,
    )


@ac.rule
def drain_completed(entries, cancel_tombstones, request):
    completed = ac.find(
        entries,
        where=lambda entry: (
            entry.valid
            & entry.completed
            & request.wba_identity_valid
            & (entry.slot == request.wba_slot)
            & (entry.generation == request.wba_generation)
            & (entry.result.required_effect_mask == request.effect_mask)
            & (entry.result.attempt == request.attempt)
        ),
    )
    tombstone = ac.find(
        cancel_tombstones,
        where=lambda row: row.valid & (row.key == request.attempt),
    )
    identity_shape = (request.wba_identity_valid & (request.effect_mask != 0)) | (
        (not request.wba_identity_valid)
        & (request.wba_slot == 0)
        & (request.wba_generation == 0)
        & (request.effect_mask == 0)
    )
    accepted = (
        request.valid
        & request.producer_drained
        & identity_shape
        & (
            (request.wba_identity_valid & completed.valid)
            | ((not request.wba_identity_valid) & tombstone.valid)
        )
    )
    entry_reclaimed = accepted & completed.valid
    tombstone_reclaimed = accepted & tombstone.valid
    if entry_reclaimed:
        entries[completed.index] = completed.value.with_fields(
            valid=False, published=False, canceled=False, completed=False
        )
    if tombstone_reclaimed:
        cancel_tombstones[tombstone.index] = tombstone.value.with_fields(valid=False)
    return WbaDrainAck(
        request=request,
        accepted=accepted,
        entry_reclaimed=entry_reclaimed,
        cancel_tombstone_reclaimed=tombstone_reclaimed,
        wba_released=accepted,
        valid=True,
    )


@ac.module
def wba(
    alu_result: TerminalResult,
    bru_result: TerminalResult,
    lsu_result: TerminalResult,
    other_result: TerminalResult,
    apply_ack: WritebackApplyAck,
    cancel: IssueCancel,
    drain_request: WbaDrainRequest,
) -> tuple[WritebackCommit, WritebackAckResult, WritebackCancelAck, WbaDrainAck]:
    """Retain, arbitrate, retry, cancel, and drain one flow's results."""

    entries: list[WbaEntry] = [0] * 8
    cancel_tombstones: list[AttemptTombstone] = [0] * 16
    ingest_terminal(entries, cancel_tombstones, 0, alu_result)
    ingest_terminal(entries, cancel_tombstones, 1, bru_result)
    ingest_terminal(entries, cancel_tombstones, 2, lsu_result)
    ingest_terminal(entries, cancel_tombstones, 3, other_result)
    commit = publish_oldest(entries)
    ack_result = accept_apply_ack(entries, apply_ack)
    cancel_ack = cancel_unpublished(entries, cancel_tombstones, cancel)
    drain_ack = drain_completed(entries, cancel_tombstones, drain_request)
    return commit, ack_result, cancel_ack, drain_ack


@ac.system
def wba_system(
    alu_result: TerminalResult,
    bru_result: TerminalResult,
    lsu_result: TerminalResult,
    other_result: TerminalResult,
    apply_ack: WritebackApplyAck,
    cancel: IssueCancel,
    drain_request: WbaDrainRequest,
) -> tuple[WritebackCommit, WritebackAckResult, WritebackCancelAck, WbaDrainAck]:
    """Compilation and gfsim boundary for the WBA H3 leaf."""

    commit, ack_result, cancel_ack, drain_ack = wba(
        alu_result,
        bru_result,
        lsu_result,
        other_result,
        apply_ack,
        cancel,
        drain_request,
    )
    return commit, ack_result, cancel_ack, drain_ack

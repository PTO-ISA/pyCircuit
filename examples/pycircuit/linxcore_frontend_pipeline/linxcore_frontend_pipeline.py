"""Domain-aware LinxCore F0-F4 and D1-D3 transport prototype.

This module intentionally models the *ownership boundaries* of the production
Chisel frontend/OOO path, rather than trying to duplicate the complete ITLB,
L1I, B-SIDE predictor, opcode catalog, BROB, or rename implementations.

The important authoring rule is visible in the implementation:

* ``domain.next()`` names the temporal coordinate of each stage;
* every ready/valid residency boundary is an explicit state register;
* F4 is also the one-entry Instruction Buffer boundary in this prototype;
* D2 previews a virtual plan and D3 captures a provisional reservation token;
* no physical resource is mutated from D2 preview alone.

The resulting IR is useful for evaluating optimizer transforms because stage
residency, data age, and transaction boundaries are all explicit and machine
checkable.  It is not a replacement for ``rtl/LinxCore``.
"""

from __future__ import annotations

from collections.abc import Mapping

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    ForwardSignal,
    Wire,
    const,
    function,
    u,
    wire_of,
)

STAGE_ORDER = ("f0", "f1", "f2", "f3", "f4_ib", "d1", "d2", "d3")
FETCH_WIDTH = 4
LINE_BYTES = 16
LINE_BITS = LINE_BYTES * 8


@const
def _payload_fields(
    m: CycleAwareCircuit, *, stid_width: int, pe_width: int, rid_width: int
) -> tuple[tuple[str, int, bool], ...]:
    _ = m
    fields = [
        ("pc", 64, True),
        ("stid", stid_width, True),
        ("pe_id", pe_width, True),
        ("packet_uid", 16, True),
        ("fetch_seq", 16, True),
        ("checkpoint_id", 16, True),
        ("epoch", 8, True),
        ("line_data", LINE_BITS, True),
        ("line_bytes_valid", 5, True),
        ("fetch_fault", 1, True),
        ("block_start_mask", FETCH_WIDTH, True),
        ("block_stop_mask", FETCH_WIDTH, True),
        ("pred_taken_mask", FETCH_WIDTH, True),
        ("pred_target", 64, True),
        ("valid_mask", FETCH_WIDTH, False),
        ("uop_count", 4, False),
        ("group_count", 3, False),
        ("resource_demand", 8, False),
        ("virtual_rid_base", rid_width, False),
        ("tail_epoch", 8, False),
        ("reservation_token", rid_width + 8, False),
    ]
    for lane in range(FETCH_WIDTH):
        fields.extend(
            (
                (f"inst_{lane}", 64, False),
                (f"len_{lane}", 4, False),
                (f"lane_pc_{lane}", 64, False),
                (f"opcode_id_{lane}", 12, True),
            )
        )
    return tuple(fields)


def _const(m: CycleAwareCircuit, value: int, width: int) -> Wire:
    _ = m
    return u(width, value)


@function
def _stage_state(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    stage: str,
    fields: tuple[tuple[str, int, bool], ...],
) -> tuple[ForwardSignal, dict[str, ForwardSignal]]:
    _ = m
    valid = domain.signal(width=1, reset_value=0, name=f"{stage}__valid")
    payload: dict[str, ForwardSignal] = {}
    for field in fields:
        field_name = field[0]
        field_width = field[1]
        payload[field_name] = domain.signal(
            width=field_width,
            reset_value=0,
            name=f"{stage}__{field_name}",
        )
    return valid, payload


@function
def _popcount4(m: CycleAwareCircuit, mask: Wire, *, width: int) -> Wire:
    total = _const(m, 0, width)
    for bit in range(FETCH_WIDTH):
        total = (total + mask[bit] + u(width, 0))[0:width]
    return total


@function
def _instruction_length(m: CycleAwareCircuit, window: Wire) -> Wire:
    """Decode the Linx 2/4/6/8-byte length from the low header nibble."""
    long_header = window[1:4] == _const(m, 0b111, 3)
    low_bit_set = window[0]
    even_len = _const(m, 6, 4) if long_header else _const(m, 2, 4)
    odd_len = _const(m, 8, 4) if long_header else _const(m, 4, 4)
    return odd_len if low_bit_set else even_len


@function
def _window_at_byte(m: CycleAwareCircuit, line: Wire, offset: Wire) -> Wire:
    """Select a zero-extended 64-bit window at a dynamic byte offset."""
    selected = _const(m, 0, 64)
    for byte in range(LINE_BYTES):
        lo = byte * 8
        available = LINE_BITS - lo
        take = min(64, available)
        low = line[lo : lo + take]
        candidate = low if take == 64 else m.cat(_const(m, 0, 64 - take), low)
        selected = candidate if offset == _const(m, byte, 5) else selected
    return selected


@function
def _assemble_f3(
    m: CycleAwareCircuit,
    src: Mapping[str, ForwardSignal],
) -> dict[str, Wire]:
    """Create a dense four-lane variable-length instruction group."""
    line = wire_of(src["line_data"])
    reported_valid_bytes = wire_of(src["line_bytes_valid"])
    line_limit = _const(m, LINE_BYTES, 5)
    valid_bytes = (
        line_limit if reported_valid_bytes > line_limit else reported_valid_bytes
    )
    base_pc = wire_of(src["pc"])
    offset = _const(m, 0, 5)
    prefix_valid = _const(m, 1, 1)
    lane_valids: list[Wire] = []
    overrides: dict[str, Wire] = {}

    for lane in range(FETCH_WIDTH):
        window = _window_at_byte(m, line, offset)
        length = _instruction_length(m, window)
        end_offset = (offset + length + u(5, 0))[0:5]
        fits = end_offset <= valid_bytes
        lane_valid = prefix_valid & fits
        lane_pc = (base_pc + offset + u(64, 0))[0:64]

        lane_valids.append(lane_valid)
        overrides[f"inst_{lane}"] = window
        overrides[f"len_{lane}"] = length if lane_valid else _const(m, 0, 4)
        overrides[f"lane_pc_{lane}"] = lane_pc

        offset = end_offset if lane_valid else offset
        prefix_valid = lane_valid

    overrides["valid_mask"] = m.cat(
        lane_valids[3], lane_valids[2], lane_valids[1], lane_valids[0]
    )
    return overrides


@function
def _copy_stage(
    m: CycleAwareCircuit,
    *,
    dst_valid: ForwardSignal,
    dst: Mapping[str, ForwardSignal],
    src_valid: Wire,
    src: Mapping[str, Wire],
    ready: Wire,
    flush: Wire,
) -> None:
    """Update one elastic stage without creating an implicit feedback boundary."""
    zero = _const(m, 0, 1)
    capture = ready & src_valid
    held_or_new_valid = src_valid if ready else wire_of(dst_valid)
    dst_valid <<= zero if flush else held_or_new_valid

    for name, state in dst.items():
        incoming = src[name]
        state <<= incoming if capture else wire_of(state)


@function
def _payload_wires(
    m: CycleAwareCircuit, src: Mapping[str, ForwardSignal]
) -> dict[str, Wire]:
    _ = m
    result: dict[str, Wire] = {}
    for name, state in src.items():
        result[name] = wire_of(state)
    return result


@function
def _input_payload(
    m: CycleAwareCircuit,
    fields: tuple[tuple[str, int, bool], ...],
) -> dict[str, Wire]:
    values: dict[str, Wire] = {}
    for field in fields:
        field_name = field[0]
        field_width = field[1]
        is_external = field[2]
        values[field_name] = (
            m.input(f"in_{field_name}", width=field_width)
            if is_external
            else _const(m, 0, field_width)
        )
    return values


def build(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    stid_width: int = 2,
    pe_width: int = 2,
    rid_width: int = 8,
) -> None:
    """Build the elastic F0-F4/IB/D1-D3 prototype."""
    if stid_width < 1 or pe_width < 1 or rid_width < 1:
        raise ValueError("stid_width, pe_width, and rid_width must be positive")

    fields = _payload_fields(
        m,
        stid_width=stid_width,
        pe_width=pe_width,
        rid_width=rid_width,
    )
    # Keep this literal rather than a comprehension: the pyCircuit JIT accepts
    # static containers but deliberately rejects residual dynamic constructs.
    stages = {
        "f0": _stage_state(m, domain, "f0", fields),
        "f1": _stage_state(m, domain, "f1", fields),
        "f2": _stage_state(m, domain, "f2", fields),
        "f3": _stage_state(m, domain, "f3", fields),
        "f4_ib": _stage_state(m, domain, "f4_ib", fields),
        "d1": _stage_state(m, domain, "d1", fields),
        "d2": _stage_state(m, domain, "d2", fields),
        "d3": _stage_state(m, domain, "d3", fields),
    }

    flush = m.input("flush", width=1)
    input_valid = m.input("in_valid", width=1)
    output_ready = m.input("out_ready", width=1)
    lookup_ready = m.input("lookup_ready", width=1)
    fetch_result_ready = m.input("fetch_result_ready", width=1)
    ib_enqueue_ready = m.input("ib_enqueue_ready", width=1)
    d2_resource_ready = m.input("d2_resource_ready", width=1)
    d3_reservation_ready = m.input("d3_reservation_ready", width=1)
    rob_tail_epoch = m.input("rob_tail_epoch", width=8)
    virtual_rid_base = m.input("virtual_rid_base", width=rid_width)
    input_payload = _input_payload(m, fields)

    # Gates qualify transfers *out* of the corresponding stage.  Keeping them
    # separate from valid is what makes payload hold under backpressure.
    out_gate = {
        "f0": lookup_ready,
        "f1": _const(m, 1, 1),
        "f2": fetch_result_ready,
        "f3": ib_enqueue_ready,
        "f4_ib": _const(m, 1, 1),
        "d1": d2_resource_ready,
        "d2": d3_reservation_ready,
    }

    ready: dict[str, Wire] = {}
    last = STAGE_ORDER[-1]
    last_valid, _ = stages[last]
    ready[last] = (~wire_of(last_valid)) | output_ready
    d2_stage_valid, _ = stages["d2"]
    ready["d2"] = (~wire_of(d2_stage_valid)) | (out_gate["d2"] & ready["d3"])
    d1_stage_valid, _ = stages["d1"]
    ready["d1"] = (~wire_of(d1_stage_valid)) | (out_gate["d1"] & ready["d2"])
    f4_stage_valid, _ = stages["f4_ib"]
    ready["f4_ib"] = (~wire_of(f4_stage_valid)) | (out_gate["f4_ib"] & ready["d1"])
    f3_stage_valid, _ = stages["f3"]
    ready["f3"] = (~wire_of(f3_stage_valid)) | (out_gate["f3"] & ready["f4_ib"])
    f2_stage_valid, _ = stages["f2"]
    ready["f2"] = (~wire_of(f2_stage_valid)) | (out_gate["f2"] & ready["f3"])
    f1_stage_valid, _ = stages["f1"]
    ready["f1"] = (~wire_of(f1_stage_valid)) | (out_gate["f1"] & ready["f2"])
    f0_stage_valid, _ = stages["f0"]
    ready["f0"] = (~wire_of(f0_stage_valid)) | (out_gate["f0"] & ready["f1"])

    # F0 — canonical PC/thread/fetch identity capture.
    f0_valid, f0 = stages["f0"]
    _copy_stage(
        m,
        dst_valid=f0_valid,
        dst=f0,
        src_valid=input_valid,
        src=input_payload,
        ready=ready["f0"],
        flush=flush,
    )
    domain.next()

    # F1 — atomic ITLB/L1I launch boundary (represented by lookup_ready).
    f1_valid, f1 = stages["f1"]
    _copy_stage(
        m,
        dst_valid=f1_valid,
        dst=f1,
        src_valid=wire_of(f0_valid) & out_gate["f0"],
        src=_payload_wires(m, f0),
        ready=ready["f1"],
        flush=flush,
    )
    domain.next()

    # F2 — exact lookup-result qualification boundary.
    f2_valid, f2 = stages["f2"]
    _copy_stage(
        m,
        dst_valid=f2_valid,
        dst=f2,
        src_valid=wire_of(f1_valid) & out_gate["f1"],
        src=_payload_wires(m, f1),
        ready=ready["f2"],
        flush=flush,
    )
    domain.next()

    # F3 — byte-stream alignment and 2/4/6/8-byte instruction assembly.
    f3_valid, f3 = stages["f3"]
    f2_to_f3 = _payload_wires(m, f2)
    for name, value in _assemble_f3(m, f2).items():
        f2_to_f3[name] = value
    _copy_stage(
        m,
        dst_valid=f3_valid,
        dst=f3,
        src_valid=wire_of(f2_valid) & out_gate["f2"],
        src=f2_to_f3,
        ready=ready["f3"],
        flush=flush,
    )
    domain.next()

    # F4/IB — final lightweight boundary metadata and retained IB residency.
    f4_valid, f4 = stages["f4_ib"]
    _copy_stage(
        m,
        dst_valid=f4_valid,
        dst=f4,
        src_valid=wire_of(f3_valid) & out_gate["f3"],
        src=_payload_wires(m, f3),
        ready=ready["f4_ib"],
        flush=flush,
    )
    domain.next()

    # D1 — canonical decode/expansion demand. Opcode IDs are catalog sidebands;
    # this prototype deliberately does not recreate an ad-hoc opcode table.
    d1_valid, d1 = stages["d1"]
    f4_mask = wire_of(f4["valid_mask"])
    decoded_mask = _const(m, 0, FETCH_WIDTH) if wire_of(f4["fetch_fault"]) else f4_mask
    uop_count = _popcount4(m, decoded_mask, width=4)
    f4_to_d1 = _payload_wires(m, f4)
    f4_to_d1["valid_mask"] = decoded_mask
    f4_to_d1["uop_count"] = uop_count
    f4_to_d1["resource_demand"] = (uop_count + u(8, 0))[0:8]
    _copy_stage(
        m,
        dst_valid=d1_valid,
        dst=d1,
        src_valid=wire_of(f4_valid) & out_gate["f4_ib"],
        src=f4_to_d1,
        ready=ready["d1"],
        flush=flush,
    )
    domain.next()

    # D2 — virtual RID/group plan and immutable tail-epoch snapshot.  There is
    # no public physical allocator mutation at this stage.
    d2_valid, d2 = stages["d2"]
    d1_mask = wire_of(d1["valid_mask"])
    group_count = _popcount4(m, d1_mask, width=3)
    d1_to_d2 = _payload_wires(m, d1)
    d1_to_d2["group_count"] = group_count
    d1_to_d2["virtual_rid_base"] = virtual_rid_base
    d1_to_d2["tail_epoch"] = rob_tail_epoch
    _copy_stage(
        m,
        dst_valid=d2_valid,
        dst=d2,
        src_valid=wire_of(d1_valid) & out_gate["d1"],
        src=d1_to_d2,
        ready=ready["d2"],
        flush=flush,
    )
    domain.next()

    # D3 — provisional reservation capture.  The consumer-facing handshake is
    # the only publication event exposed by this bounded prototype.
    d3_valid, d3 = stages["d3"]
    reservation_token = m.cat(
        wire_of(d2["tail_epoch"]), wire_of(d2["virtual_rid_base"])
    )
    d2_to_d3 = _payload_wires(m, d2)
    d2_to_d3["reservation_token"] = reservation_token
    _copy_stage(
        m,
        dst_valid=d3_valid,
        dst=d3,
        src_valid=wire_of(d2_valid) & out_gate["d2"],
        src=d2_to_d3,
        ready=ready["d3"],
        flush=flush,
    )

    m.output("in_ready", ready["f0"] & (~flush))
    for stage in STAGE_ORDER:
        stage_valid, stage_payload = stages[stage]
        m.output(f"stage__{stage}__valid", wire_of(stage_valid))
        m.output(f"stage__{stage}__pc", wire_of(stage_payload["pc"]))
        m.output(f"stage__{stage}__valid_mask", wire_of(stage_payload["valid_mask"]))

    out_valid = wire_of(d3_valid)
    m.output("out_valid", out_valid)
    m.output("out_fire", out_valid & output_ready)
    m.output("out_stid", wire_of(d3["stid"]))
    m.output("out_pe_id", wire_of(d3["pe_id"]))
    m.output("out_packet_uid", wire_of(d3["packet_uid"]))
    m.output("out_checkpoint_id", wire_of(d3["checkpoint_id"]))
    m.output("out_epoch", wire_of(d3["epoch"]))
    m.output("out_valid_mask", wire_of(d3["valid_mask"]))
    m.output("out_uop_count", wire_of(d3["uop_count"]))
    m.output("out_group_count", wire_of(d3["group_count"]))
    m.output("out_virtual_rid_base", wire_of(d3["virtual_rid_base"]))
    m.output("out_tail_epoch", wire_of(d3["tail_epoch"]))
    m.output("out_reservation_token", wire_of(d3["reservation_token"]))
    for lane in range(FETCH_WIDTH):
        m.output(f"out_inst_{lane}", wire_of(d3[f"inst_{lane}"]))
        m.output(f"out_len_{lane}", wire_of(d3[f"len_{lane}"]))
        m.output(f"out_pc_{lane}", wire_of(d3[f"lane_pc_{lane}"]))
        m.output(f"out_opcode_id_{lane}", wire_of(d3[f"opcode_id_{lane}"]))


build.__pycircuit_name__ = "linxcore_frontend_pipeline"


def reference_split_window(
    data: bytes, *, valid_bytes: int | None = None
) -> list[tuple[int, int]]:
    """Pure-Python oracle for the bounded F3 split used by unit tests."""
    limit = len(data) if valid_bytes is None else min(len(data), valid_bytes)
    offset = 0
    result: list[tuple[int, int]] = []
    while len(result) < FETCH_WIDTH and offset < limit:
        header = data[offset] & 0xF
        long_header = ((header >> 1) & 0x7) == 0x7
        length = (
            (8 if long_header else 4) if (header & 1) else (6 if long_header else 2)
        )
        if offset + length > limit:
            break
        raw = int.from_bytes(data[offset : offset + length], "little")
        result.append((raw, length))
        offset += length
    return result

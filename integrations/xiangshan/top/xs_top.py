"""XSTop — SoC Top-Level Wrapper for XiangShan-pyc.

Top-level SoC integrating multiple XSTile instances with a shared memory
bus and peripheral interface.  This is the outermost module that maps to
the chip boundary.

Reference: XiangShan/src/main/scala/top/XSTop.scala

Ports:
  - AXI4 memory port (simplified TileLink-to-AXI bridge)
  - Per-tile interrupt inputs (meip, seip, mtip, msip)
  - Debug port / JTAG interface
  - Peripheral access port

Internal:
  - Replicate XSTile per core
  - Shared downstream bus arbiter (round-robin simplified)
  - Interrupt distributor

Key features:
  S-XT-001  Multi-core tile instantiation
  S-XT-002  Shared memory bus arbitration
  S-XT-003  Per-core interrupt routing
  S-XT-004  AXI4 memory port output
  S-XT-005  Debug / JTAG interface
"""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    cas,
    mux,
    wire_of,
)

from top.parameters import (
    CACHE_LINE_SIZE,
    PC_WIDTH,
    XLEN,
)
from top.peripherals import clint, plic
from top.xs_tile import xs_tile

BLOCK_BITS = CACHE_LINE_SIZE
HART_ID_WIDTH = 4
AXI_ID_WIDTH = 8
AXI_ADDR_WIDTH = PC_WIDTH
AXI_DATA_WIDTH = XLEN


def xs_top(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "soc",
    num_cores: int = 2,
    data_width: int = AXI_DATA_WIDTH,
    addr_width: int = AXI_ADDR_WIDTH,
    block_bits: int = BLOCK_BITS,
    hart_id_w: int = HART_ID_WIDTH,
    axi_id_w: int = AXI_ID_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """XSTop: top-level SoC with multiple tiles + peripherals."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    core_sel_w = max(1, (num_cores - 1).bit_length())

    ZERO_1 = cas(domain, m.const(0, width=1), cycle=0)
    ONE_1 = cas(domain, m.const(1, width=1), cycle=0)

    # ── Sub-module calls ──
    for _i in range(num_cores):
        domain.call(
            xs_tile,
            inputs={},
            prefix=f"{prefix}_s_tile{_i}",
            data_width=data_width,
            pc_width=addr_width,
        )

    domain.call(plic, inputs={}, prefix=f"{prefix}_s_plic")

    domain.call(clint, inputs={}, prefix=f"{prefix}_s_clint")

    # ================================================================
    # External inputs
    # ================================================================

    # Per-core interrupt inputs
    core_meip = [
        cas(domain, m.input(f"{prefix}_core{i}_meip", width=1), cycle=0)
        for i in range(num_cores)
    ]
    core_seip = [
        cas(domain, m.input(f"{prefix}_core{i}_seip", width=1), cycle=0)
        for i in range(num_cores)
    ]
    core_mtip = [
        cas(domain, m.input(f"{prefix}_core{i}_mtip", width=1), cycle=0)
        for i in range(num_cores)
    ]
    core_msip = [
        cas(domain, m.input(f"{prefix}_core{i}_msip", width=1), cycle=0)
        for i in range(num_cores)
    ]

    # Debug / JTAG
    debug_req_valid = (
        _in["debug_req_valid"]
        if "debug_req_valid" in _in
        else cas(domain, m.input(f"{prefix}_debug_req_valid", width=1), cycle=0)
    )
    (
        _in["debug_req_addr"]
        if "debug_req_addr" in _in
        else cas(domain, m.input(f"{prefix}_debug_req_addr", width=addr_width), cycle=0)
    )
    debug_req_data = (
        _in["debug_req_data"]
        if "debug_req_data" in _in
        else cas(domain, m.input(f"{prefix}_debug_req_data", width=data_width), cycle=0)
    )
    (
        _in["debug_req_op"]
        if "debug_req_op" in _in
        else cas(domain, m.input(f"{prefix}_debug_req_op", width=2), cycle=0)
    )

    # AXI4 memory port — read response from external memory
    axi_r_valid = (
        _in["axi_r_valid"]
        if "axi_r_valid" in _in
        else cas(domain, m.input(f"{prefix}_axi_r_valid", width=1), cycle=0)
    )
    axi_r_data = (
        _in["axi_r_data"]
        if "axi_r_data" in _in
        else cas(domain, m.input(f"{prefix}_axi_r_data", width=data_width), cycle=0)
    )
    axi_r_id = (
        _in["axi_r_id"]
        if "axi_r_id" in _in
        else cas(domain, m.input(f"{prefix}_axi_r_id", width=axi_id_w), cycle=0)
    )
    (
        _in["axi_r_last"]
        if "axi_r_last" in _in
        else cas(domain, m.input(f"{prefix}_axi_r_last", width=1), cycle=0)
    )

    # AXI4 write response
    (
        _in["axi_b_valid"]
        if "axi_b_valid" in _in
        else cas(domain, m.input(f"{prefix}_axi_b_valid", width=1), cycle=0)
    )
    (
        _in["axi_b_id"]
        if "axi_b_id" in _in
        else cas(domain, m.input(f"{prefix}_axi_b_id", width=axi_id_w), cycle=0)
    )

    # Per-tile downstream request (from tiles → to shared bus)
    tile_ds_req_valid = [
        cas(domain, m.input(f"{prefix}_tile{i}_ds_req_valid", width=1), cycle=0)
        for i in range(num_cores)
    ]
    tile_ds_req_addr = [
        cas(domain, m.input(f"{prefix}_tile{i}_ds_req_addr", width=addr_width), cycle=0)
        for i in range(num_cores)
    ]
    tile_ds_req_source = [
        cas(domain, m.input(f"{prefix}_tile{i}_ds_req_source", width=1), cycle=0)
        for i in range(num_cores)
    ]

    # ================================================================
    # Shared bus arbiter (round-robin simplified)
    # ================================================================

    arb_sel = domain.signal(
        width=core_sel_w, reset_value=0, name=f"{prefix}_xs_arb_sel"
    )

    # Priority select: check current arb_sel first, then wrap
    bus_req_valid = ZERO_1
    bus_req_addr = cas(domain, m.const(0, width=addr_width), cycle=0)
    bus_req_source = ZERO_1
    bus_req_core = cas(domain, m.const(0, width=core_sel_w), cycle=0)

    for i in range(num_cores):
        i_c = cas(domain, m.const(i, width=core_sel_w), cycle=0)
        is_selected = (arb_sel == i_c) & tile_ds_req_valid[i]
        bus_req_valid = mux(is_selected, ONE_1, bus_req_valid)
        bus_req_addr = mux(is_selected, tile_ds_req_addr[i], bus_req_addr)
        bus_req_source = mux(is_selected, tile_ds_req_source[i], bus_req_source)
        bus_req_core = mux(is_selected, i_c, bus_req_core)

    # Fallback: if current selection has no request, try others
    for i in range(num_cores):
        i_c = cas(domain, m.const(i, width=core_sel_w), cycle=0)
        fallback = tile_ds_req_valid[i] & (~bus_req_valid)
        bus_req_valid = mux(fallback, ONE_1, bus_req_valid)
        bus_req_addr = mux(fallback, tile_ds_req_addr[i], bus_req_addr)
        bus_req_source = mux(fallback, tile_ds_req_source[i], bus_req_source)
        bus_req_core = mux(fallback, i_c, bus_req_core)

    # ================================================================
    # AXI4 read request output (TileLink → AXI bridge, simplified)
    # ================================================================

    m.output(f"{prefix}_axi_ar_valid", wire_of(bus_req_valid))
    _out["axi_ar_valid"] = bus_req_valid
    m.output(f"{prefix}_axi_ar_addr", wire_of(bus_req_addr))
    _out["axi_ar_addr"] = bus_req_addr
    # Encode {core_id, source} into AXI ID
    axi_out_id = cas(
        domain,
        m.cat(
            wire_of(bus_req_core),
            wire_of(bus_req_source),
            m.const(0, width=max(1, axi_id_w - core_sel_w - 1)),
        ),
        cycle=0,
    )
    m.output(f"{prefix}_axi_ar_id", wire_of(axi_out_id))
    _out["axi_ar_id"] = axi_out_id
    m.output(f"{prefix}_axi_ar_len", m.const(0, width=8))  # single beat
    m.output(f"{prefix}_axi_ar_size", m.const(3, width=3))  # 8 bytes

    # ================================================================
    # AXI4 read response → route to correct tile
    # ================================================================

    # Decode core_id from AXI R ID
    resp_core = axi_r_id[1 : 1 + core_sel_w]
    resp_source = axi_r_id[0:1]

    for i in range(num_cores):
        i_c = cas(domain, m.const(i, width=core_sel_w), cycle=0)
        is_this_core = resp_core == i_c
        m.output(f"{prefix}_tile{i}_ds_resp_valid", wire_of(axi_r_valid & is_this_core))
        m.output(f"{prefix}_tile{i}_ds_resp_data", wire_of(axi_r_data))
        m.output(f"{prefix}_tile{i}_ds_resp_source", wire_of(resp_source))

    # ================================================================
    # Per-core interrupt routing
    # ================================================================

    for i in range(num_cores):
        m.output(f"{prefix}_tile{i}_meip", wire_of(core_meip[i]))
        m.output(f"{prefix}_tile{i}_seip", wire_of(core_seip[i]))
        m.output(f"{prefix}_tile{i}_mtip", wire_of(core_mtip[i]))
        m.output(f"{prefix}_tile{i}_msip", wire_of(core_msip[i]))
        m.output(f"{prefix}_tile{i}_hart_id", m.const(i, width=hart_id_w))

    # ================================================================
    # Debug port output
    # ================================================================

    m.output(f"{prefix}_debug_resp_valid", wire_of(debug_req_valid))
    _out["debug_resp_valid"] = debug_req_valid
    m.output(f"{prefix}_debug_resp_data", wire_of(debug_req_data))
    _out["debug_resp_data"] = debug_req_data

    # ================================================================
    # Pipeline register + arbiter state update
    # ================================================================

    domain.cycle(wire_of(bus_req_valid), name=f"{prefix}_xs_s1_bv")
    domain.cycle(wire_of(bus_req_core), name=f"{prefix}_xs_s1_bc")

    domain.next()

    # Round-robin: advance selector when a request fires
    one_sel = cas(domain, m.const(1, width=core_sel_w), cycle=0)
    max_sel = cas(domain, m.const(num_cores - 1, width=core_sel_w), cycle=0)
    next_sel = cas(domain, (wire_of(arb_sel) + wire_of(one_sel))[0:core_sel_w], cycle=0)
    wrap_sel = mux(
        arb_sel == max_sel, cas(domain, m.const(0, width=core_sel_w), cycle=0), next_sel
    )
    arb_sel <<= mux(bus_req_valid, wrap_sel, arb_sel)

    # AXI write channel (simplified: no write support yet)
    m.output(f"{prefix}_axi_aw_valid", wire_of(ZERO_1))
    _out["axi_aw_valid"] = ZERO_1
    m.output(f"{prefix}_axi_w_valid", wire_of(ZERO_1))
    _out["axi_w_valid"] = ZERO_1
    m.output(f"{prefix}_axi_b_ready", wire_of(ONE_1))
    _out["axi_b_ready"] = ONE_1
    return _out


xs_top.__pycircuit_name__ = "xs_top"


if __name__ == "__main__":
    pass

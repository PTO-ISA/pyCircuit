"""XSTile — Tile Integration (XSCore + L2 Cache) for XiangShan-pyc.

Combines one XSCore instance with its private L2 cache.  The tile is the
unit of replication in a multi-core XSTop.

Reference: XiangShan/src/main/scala/top/XSTile.scala

Ports:
  Downstream — to L3 / memory bus (TileLink-like A/D channels)
  Interrupt  — meip, seip, mtip, msip, debug
  Hart ID    — static configuration

Internal wiring:
  XSCore.l2_icache_miss → L2.ic_req
  XSCore.l2_dcache_miss → L2.dc_req
  L2.ic_grant → XSCore.l2_refill
  L2.dc_grant → XSCore.l2_data_resp
  L2.ds_req → downstream (output)
  downstream → L2.ds_resp (input)

Key features:
  T-XT-001  Core ↔ L2 miss/refill wiring
  T-XT-002  Downstream TileLink pass-through
  T-XT-003  Hart ID configuration port
  T-XT-004  Interrupt routing
"""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from l2.l2_top import l2_top
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
    COMMIT_WIDTH,
    DECODE_WIDTH,
    NUM_LDU,
    NUM_STA,
    PC_WIDTH,
    PTAG_WIDTH_INT,
    ROB_IDX_WIDTH,
    XLEN,
)
from top.xs_core import xs_core

BLOCK_BITS = CACHE_LINE_SIZE
FU_TYPE_WIDTH = 3
NUM_WB_PORTS = 4
HART_ID_WIDTH = 4


def xs_tile(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "tile",
    decode_width: int = DECODE_WIDTH,
    commit_width: int = COMMIT_WIDTH,
    num_wb: int = NUM_WB_PORTS,
    num_load: int = NUM_LDU,
    num_store: int = NUM_STA,
    data_width: int = XLEN,
    pc_width: int = PC_WIDTH,
    ptag_w: int = PTAG_WIDTH_INT,
    rob_idx_w: int = ROB_IDX_WIDTH,
    fu_type_w: int = FU_TYPE_WIDTH,
    block_bits: int = BLOCK_BITS,
    hart_id_w: int = HART_ID_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """XSTile: XSCore + L2 cache tile unit."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    ZERO_1 = cas(domain, m.const(0, width=1), cycle=0)
    ONE_1 = cas(domain, m.const(1, width=1), cycle=0)

    # ================================================================
    # External inputs
    # ================================================================

    # Hart ID (static config)
    hart_id = (
        _in["hart_id"]
        if "hart_id" in _in
        else cas(domain, m.input(f"{prefix}_hart_id", width=hart_id_w), cycle=0)
    )

    # Interrupts → routed to core
    meip = (
        _in["meip"]
        if "meip" in _in
        else cas(domain, m.input(f"{prefix}_meip", width=1), cycle=0)
    )
    seip = (
        _in["seip"]
        if "seip" in _in
        else cas(domain, m.input(f"{prefix}_seip", width=1), cycle=0)
    )
    mtip = (
        _in["mtip"]
        if "mtip" in _in
        else cas(domain, m.input(f"{prefix}_mtip", width=1), cycle=0)
    )
    msip = (
        _in["msip"]
        if "msip" in _in
        else cas(domain, m.input(f"{prefix}_msip", width=1), cycle=0)
    )
    debug_intr = (
        _in["debug_interrupt"]
        if "debug_interrupt" in _in
        else cas(domain, m.input(f"{prefix}_debug_interrupt", width=1), cycle=0)
    )

    # Downstream from L3/memory → L2 refill
    ds_resp_valid = (
        _in["ds_resp_valid"]
        if "ds_resp_valid" in _in
        else cas(domain, m.input(f"{prefix}_ds_resp_valid", width=1), cycle=0)
    )
    ds_resp_data = (
        _in["ds_resp_data"]
        if "ds_resp_data" in _in
        else cas(domain, m.input(f"{prefix}_ds_resp_data", width=block_bits), cycle=0)
    )
    ds_resp_source = (
        _in["ds_resp_source"]
        if "ds_resp_source" in _in
        else cas(domain, m.input(f"{prefix}_ds_resp_source", width=1), cycle=0)
    )

    # Writeback ports (from execution units, pass-through to core)
    [
        cas(domain, m.input(f"{prefix}_wb_valid_{i}", width=1), cycle=0)
        for i in range(num_wb)
    ]
    [
        cas(domain, m.input(f"{prefix}_wb_data_{i}", width=data_width), cycle=0)
        for i in range(num_wb)
    ]
    [
        cas(domain, m.input(f"{prefix}_wb_rob_idx_{i}", width=rob_idx_w), cycle=0)
        for i in range(num_wb)
    ]

    # Branch / exception redirect
    bru_redirect_valid = (
        _in["bru_redirect_valid"]
        if "bru_redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_bru_redirect_valid", width=1), cycle=0)
    )
    bru_redirect_target = (
        _in["bru_redirect_target"]
        if "bru_redirect_target" in _in
        else cas(
            domain, m.input(f"{prefix}_bru_redirect_target", width=pc_width), cycle=0
        )
    )
    rob_exception_valid = (
        _in["rob_exception_valid"]
        if "rob_exception_valid" in _in
        else cas(domain, m.input(f"{prefix}_rob_exception_valid", width=1), cycle=0)
    )
    rob_exception_pc = (
        _in["rob_exception_pc"]
        if "rob_exception_pc" in _in
        else cas(domain, m.input(f"{prefix}_rob_exception_pc", width=pc_width), cycle=0)
    )

    # Issue queue backpressure
    (
        _in["iq_int_ready"]
        if "iq_int_ready" in _in
        else cas(domain, m.input(f"{prefix}_iq_int_ready", width=1), cycle=0)
    )
    (
        _in["iq_fp_ready"]
        if "iq_fp_ready" in _in
        else cas(domain, m.input(f"{prefix}_iq_fp_ready", width=1), cycle=0)
    )
    (
        _in["iq_mem_ready"]
        if "iq_mem_ready" in _in
        else cas(domain, m.input(f"{prefix}_iq_mem_ready", width=1), cycle=0)
    )

    # Load/store writeback from MemBlock
    ld_wb_valid = [
        cas(domain, m.input(f"{prefix}_ld{i}_wb_valid", width=1), cycle=0)
        for i in range(num_load)
    ]
    ld_wb_data = [
        cas(domain, m.input(f"{prefix}_ld{i}_wb_data", width=data_width), cycle=0)
        for i in range(num_load)
    ]
    ld_wb_rob_idx = [
        cas(domain, m.input(f"{prefix}_ld{i}_wb_rob_idx", width=rob_idx_w), cycle=0)
        for i in range(num_load)
    ]
    st_wb_valid = [
        cas(domain, m.input(f"{prefix}_st{i}_wb_valid", width=1), cycle=0)
        for i in range(num_store)
    ]
    st_wb_rob_idx = [
        cas(domain, m.input(f"{prefix}_st{i}_wb_rob_idx", width=rob_idx_w), cycle=0)
        for i in range(num_store)
    ]

    # DCache miss from MemBlock
    dcache_miss_valid = (
        _in["dcache_miss_valid"]
        if "dcache_miss_valid" in _in
        else cas(domain, m.input(f"{prefix}_dcache_miss_valid", width=1), cycle=0)
    )
    dcache_miss_addr = (
        _in["dcache_miss_addr"]
        if "dcache_miss_addr" in _in
        else cas(domain, m.input(f"{prefix}_dcache_miss_addr", width=pc_width), cycle=0)
    )

    # ── Sub-module calls ──
    domain.call(
        xs_core,
        inputs={},
        prefix=f"{prefix}_s_core",
        decode_width=decode_width,
        commit_width=commit_width,
        num_wb=num_wb,
        num_load=num_load,
        num_store=num_store,
        data_width=data_width,
        pc_width=pc_width,
        ptag_w=ptag_w,
        rob_idx_w=rob_idx_w,
    )

    _l2_idx_w = max(1, 6)
    _l2_tag_w = max(1, pc_width - _l2_idx_w - 6)
    domain.call(
        l2_top,
        inputs={},
        prefix=f"{prefix}_s_l2",
        addr_width=pc_width,
        data_width=data_width,
        tag_w=_l2_tag_w,
        idx_w=_l2_idx_w,
    )

    # ================================================================
    # XSCore logic (simplified inline)
    # ================================================================

    # Frontend fetch PC state
    fetch_pc = domain.signal(
        width=pc_width, reset_value=0, name=f"{prefix}_xt_fetch_pc"
    )
    bpu_valid = domain.signal(width=1, reset_value=0, name=f"{prefix}_xt_bpu_v")

    # Redirect
    redirect_valid = bru_redirect_valid | rob_exception_valid
    redirect_target = mux(rob_exception_valid, rob_exception_pc, bru_redirect_target)
    flush = redirect_valid

    # L2 → Core refill: select based on ds_resp_source
    ic_refill_valid = ds_resp_valid & (~ds_resp_source)  # source 0 = IC
    dc_refill_valid = ds_resp_valid & ds_resp_source  # source 1 = DC

    # BPU fallthrough
    fallthrough_c = cas(domain, m.const(64, width=pc_width), cycle=0)
    bpu_pred_target = cas(
        domain, (wire_of(fetch_pc) + wire_of(fallthrough_c))[0:pc_width], cycle=0
    )

    ibuf_ready = ONE_1  # simplified
    s0_fire = bpu_valid & ibuf_ready & (~flush)

    next_pc = fetch_pc
    next_pc = mux(s0_fire, bpu_pred_target, next_pc)
    next_pc = mux(redirect_valid, redirect_target, next_pc)

    # Pipeline: fetch → ICache → IFU → Decode (3 stages)
    s1_v = domain.cycle(wire_of(s0_fire), name=f"{prefix}_xt_s1_v")
    s1_pc = domain.cycle(wire_of(fetch_pc), name=f"{prefix}_xt_s1_pc")

    domain.next()

    s1_alive = s1_v & (~wire_of(redirect_valid))
    # ICache hit when L2 IC refill is available
    s1_resp = s1_alive & wire_of(ic_refill_valid)
    s1_miss = s1_alive & (~wire_of(ic_refill_valid))

    # Core → L2: ICache miss request
    core_ic_miss_valid = s1_miss
    core_ic_miss_addr = s1_pc

    # Core → L2: DCache miss request (pass-through from MemBlock)
    core_dc_miss_valid = wire_of(dcache_miss_valid)
    core_dc_miss_addr = wire_of(dcache_miss_addr)

    # L2 request queue: arbitrate IC > DC
    l2_enq_valid = core_ic_miss_valid | core_dc_miss_valid
    l2_enq_addr = mux(
        core_ic_miss_valid,
        cas(domain, core_ic_miss_addr, cycle=0),
        cas(domain, core_dc_miss_addr, cycle=0),
    )
    l2_enq_source = mux(core_ic_miss_valid, ZERO_1, ONE_1)

    # L2 → downstream output
    m.output(f"{prefix}_ds_req_valid", l2_enq_valid)
    _out["ds_req_valid"] = cas(domain, l2_enq_valid, cycle=domain.cycle_index)
    m.output(f"{prefix}_ds_req_addr", wire_of(l2_enq_addr))
    _out["ds_req_addr"] = l2_enq_addr
    m.output(f"{prefix}_ds_req_source", wire_of(l2_enq_source))
    _out["ds_req_source"] = l2_enq_source

    s2_v = domain.cycle(s1_resp, name=f"{prefix}_xt_s2_v")
    s2_pc = domain.cycle(s1_pc, name=f"{prefix}_xt_s2_pc")
    s2_data = domain.cycle(wire_of(ds_resp_data), name=f"{prefix}_xt_s2_data")

    domain.next()

    s2_alive = s2_v & (~wire_of(redirect_valid))

    INST_WIDTH = 32
    s3_v = domain.cycle(s2_alive, name=f"{prefix}_xt_s3_v")
    s3_pc = domain.cycle(s2_pc, name=f"{prefix}_xt_s3_pc")
    s3_insts = [
        domain.cycle(
            (
                s2_data[i * INST_WIDTH : (i + 1) * INST_WIDTH]
                if (i + 1) * INST_WIDTH <= block_bits
                else m.const(0, width=INST_WIDTH)
            ),
            name=f"{prefix}_xt_s3_i{i}",
        )
        for i in range(decode_width)
    ]

    domain.next()

    s3_alive = s3_v & (~wire_of(redirect_valid))

    # Decode outputs
    INST_BYTES = 2
    for i in range(decode_width):
        inst_pc = (s3_pc + m.const(i * INST_BYTES, width=pc_width))[0:pc_width]
        m.output(f"{prefix}_dec_valid_{i}", s3_alive)
        m.output(f"{prefix}_dec_inst_{i}", s3_insts[i])
        m.output(f"{prefix}_dec_pc_{i}", inst_pc)

    # ================================================================
    # Outputs
    # ================================================================

    m.output(f"{prefix}_redirect_valid", wire_of(redirect_valid))
    _out["redirect_valid"] = redirect_valid
    m.output(f"{prefix}_redirect_target", wire_of(redirect_target))
    _out["redirect_target"] = redirect_target

    m.output(f"{prefix}_hart_id_out", wire_of(hart_id))
    _out["hart_id_out"] = hart_id
    m.output(
        f"{prefix}_interrupt_pending", wire_of(meip | seip | mtip | msip | debug_intr)
    )

    # Forward load/store writeback
    for i in range(num_load):
        m.output(f"{prefix}_ld{i}_wb_valid_out", wire_of(ld_wb_valid[i]))
        m.output(f"{prefix}_ld{i}_wb_data_out", wire_of(ld_wb_data[i]))
        m.output(f"{prefix}_ld{i}_wb_rob_idx_out", wire_of(ld_wb_rob_idx[i]))
    for i in range(num_store):
        m.output(f"{prefix}_st{i}_wb_valid_out", wire_of(st_wb_valid[i]))
        m.output(f"{prefix}_st{i}_wb_rob_idx_out", wire_of(st_wb_rob_idx[i]))

    # DC refill to MemBlock
    m.output(f"{prefix}_dc_refill_valid", wire_of(dc_refill_valid))
    _out["dc_refill_valid"] = dc_refill_valid
    m.output(f"{prefix}_dc_refill_data", wire_of(ds_resp_data))
    _out["dc_refill_data"] = ds_resp_data

    # Debug
    m.output(f"{prefix}_debug_pc", wire_of(fetch_pc))
    _out["debug_pc"] = fetch_pc

    # ================================================================
    # State updates
    # ================================================================

    domain.next()

    bpu_valid <<= ONE_1
    fetch_pc <<= next_pc
    return _out


xs_tile.__pycircuit_name__ = "xs_tile"


if __name__ == "__main__":
    pass

"""XSCore — Core Integration for XiangShan-pyc.

Top-level integration of Frontend, Backend, and MemBlock subsystems within
a single processor core.  Wires the key inter-module interfaces and exposes
external ports for L2 cache, interrupts, and debug.

Reference: XiangShan/src/main/scala/xiangshan/XSCore.scala

Internal connections:
  Frontend → Backend: decoded uops (decode_width channels)
  Backend  → Frontend: redirect (valid, target)
  Backend  → MemBlock: memory dispatch (load/store uops)
  MemBlock → Backend: load writeback, store completion
  Frontend → L2: ICache miss refill interface

External ports:
  - L2 cache interface (TileLink-like A/D channels)
  - Interrupt inputs (meip, seip, mtip, msip, debug)
  - Debug port

Key features:
  C-XC-001  Frontend ↔ Backend redirect / stall loop
  C-XC-002  Backend → MemBlock memory dispatch path
  C-XC-003  MemBlock → Backend load/store writeback
  C-XC-004  ICache miss → L2 refill request
  C-XC-005  External interrupt routing to backend
"""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from backend.backend import backend
from frontend.frontend import frontend
from mem.memblock import memblock
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

FU_TYPE_WIDTH = 3
NUM_WB_PORTS = 4
BLOCK_BITS = CACHE_LINE_SIZE  # 512 bits


def xs_core(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "core",
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
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """XSCore: Frontend + Backend + MemBlock interconnect."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    ZERO_1 = cas(domain, m.const(0, width=1), cycle=0)
    ONE_1 = cas(domain, m.const(1, width=1), cycle=0)
    cas(domain, m.const(0, width=pc_width), cycle=0)

    # ================================================================
    # External inputs
    # ================================================================

    # L2 refill response (ICache miss path)
    l2_refill_valid = (
        _in["l2_refill_valid"]
        if "l2_refill_valid" in _in
        else cas(domain, m.input(f"{prefix}_l2_refill_valid", width=1), cycle=0)
    )
    l2_refill_data = (
        _in["l2_refill_data"]
        if "l2_refill_data" in _in
        else cas(domain, m.input(f"{prefix}_l2_refill_data", width=block_bits), cycle=0)
    )

    # L2 data response (DCache miss path)
    l2_data_resp_valid = (
        _in["l2_data_resp_valid"]
        if "l2_data_resp_valid" in _in
        else cas(domain, m.input(f"{prefix}_l2_data_resp_valid", width=1), cycle=0)
    )
    l2_data_resp_data = (
        _in["l2_data_resp_data"]
        if "l2_data_resp_data" in _in
        else cas(
            domain, m.input(f"{prefix}_l2_data_resp_data", width=data_width), cycle=0
        )
    )

    # Interrupt inputs
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

    # ================================================================
    # Frontend interface (simplified)
    # Cycle 0: BPU prediction + redirect handling
    # ================================================================

    # Frontend state: fetch PC
    fetch_pc = domain.signal(
        width=pc_width, reset_value=0, name=f"{prefix}_xc_fetch_pc"
    )
    bpu_valid = domain.signal(width=1, reset_value=0, name=f"{prefix}_xc_bpu_valid")

    # Backend redirect (computed below, forward-declared via signal)
    redirect_valid = domain.signal(width=1, reset_value=0, name=f"{prefix}_xc_redir_v")
    redirect_target = domain.signal(
        width=pc_width, reset_value=0, name=f"{prefix}_xc_redir_t"
    )

    # Backend stall (forward-declared via signal)
    be_stall = domain.signal(width=1, reset_value=0, name=f"{prefix}_xc_be_stall")

    # Frontend ibuf ready = ~stall
    ibuf_ready = ~be_stall

    # BPU simplified: fallthrough prediction
    fallthrough_c = cas(domain, m.const(64, width=pc_width), cycle=0)
    bpu_pred_target = cas(
        domain, (wire_of(fetch_pc) + wire_of(fallthrough_c))[0:pc_width], cycle=0
    )

    flush = redirect_valid
    s0_fire = bpu_valid & ibuf_ready & (~flush)

    next_pc = fetch_pc
    next_pc = mux(s0_fire, bpu_pred_target, next_pc)
    next_pc = mux(redirect_valid, redirect_target, next_pc)

    # ── Sub-module calls ──
    domain.call(
        frontend,
        inputs={},
        prefix=f"{prefix}_s_fe",
        decode_width=decode_width,
        pc_width=pc_width,
    )

    domain.call(
        backend,
        inputs={},
        prefix=f"{prefix}_s_be",
        decode_width=decode_width,
        commit_width=commit_width,
        num_wb=num_wb,
        data_width=data_width,
        pc_width=pc_width,
        ptag_w=ptag_w,
        rob_idx_w=rob_idx_w,
    )

    domain.call(
        memblock,
        inputs={},
        prefix=f"{prefix}_s_mem",
        num_load=num_load,
        num_store=num_store,
        data_width=data_width,
        addr_width=pc_width,
        rob_idx_width=rob_idx_w,
    )

    # ── Frontend pipeline: cycle 0 → 1 (ICache) ──────────────────
    s1_valid_w = domain.cycle(wire_of(s0_fire), name=f"{prefix}_xc_s1_v")
    s1_pc_w = domain.cycle(wire_of(fetch_pc), name=f"{prefix}_xc_s1_pc")

    domain.next()

    # Cycle 1: ICache fetch (simplified hit model using L2 refill)
    s1_valid = s1_valid_w & (~wire_of(redirect_valid))
    s1_resp_valid = s1_valid & wire_of(l2_refill_valid)
    s1_miss = s1_valid & (~wire_of(l2_refill_valid))

    m.output(f"{prefix}_l2_icache_miss_valid", s1_miss)
    _out["l2_icache_miss_valid"] = cas(domain, s1_miss, cycle=domain.cycle_index)
    m.output(f"{prefix}_l2_icache_miss_addr", s1_pc_w)
    _out["l2_icache_miss_addr"] = cas(domain, s1_pc_w, cycle=domain.cycle_index)

    s2_valid_w = domain.cycle(s1_resp_valid, name=f"{prefix}_xc_s2_v")
    s2_pc_w = domain.cycle(s1_pc_w, name=f"{prefix}_xc_s2_pc")
    s2_data_w = domain.cycle(wire_of(l2_refill_data), name=f"{prefix}_xc_s2_data")

    domain.next()

    # Cycle 2: IFU instruction extraction
    s2_valid = s2_valid_w & (~wire_of(redirect_valid))

    INST_WIDTH = 32
    ifu_insts = []
    for i in range(decode_width):
        lo = i * INST_WIDTH
        hi = lo + INST_WIDTH
        if hi <= block_bits:
            ifu_insts.append(s2_data_w[lo:hi])
        else:
            ifu_insts.append(m.const(0, width=INST_WIDTH))

    s3_valid_w = domain.cycle(s2_valid, name=f"{prefix}_xc_s3_v")
    s3_pc_w = domain.cycle(s2_pc_w, name=f"{prefix}_xc_s3_pc")
    s3_insts_w = [
        domain.cycle(ifu_insts[i], name=f"{prefix}_xc_s3_inst_{i}")
        for i in range(decode_width)
    ]

    domain.next()

    # Cycle 3: Decode → Backend dispatch
    s3_valid = s3_valid_w & (~wire_of(redirect_valid))

    # ================================================================
    # Backend: dispatch classification + redirect
    # ================================================================

    # Writeback from execution units
    wb_valid = [
        cas(domain, m.input(f"{prefix}_wb_valid_{i}", width=1), cycle=0)
        for i in range(num_wb)
    ]
    [
        cas(domain, m.input(f"{prefix}_wb_data_{i}", width=data_width), cycle=0)
        for i in range(num_wb)
    ]
    wb_rob_idx = [
        cas(domain, m.input(f"{prefix}_wb_rob_idx_{i}", width=rob_idx_w), cycle=0)
        for i in range(num_wb)
    ]

    # Branch redirect from execution
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

    # ROB exception
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
    iq_int_ready = (
        _in["iq_int_ready"]
        if "iq_int_ready" in _in
        else cas(domain, m.input(f"{prefix}_iq_int_ready", width=1), cycle=0)
    )
    iq_fp_ready = (
        _in["iq_fp_ready"]
        if "iq_fp_ready" in _in
        else cas(domain, m.input(f"{prefix}_iq_fp_ready", width=1), cycle=0)
    )
    iq_mem_ready = (
        _in["iq_mem_ready"]
        if "iq_mem_ready" in _in
        else cas(domain, m.input(f"{prefix}_iq_mem_ready", width=1), cycle=0)
    )

    # Redirect: priority ROB exception > BRU
    new_redir_valid = bru_redirect_valid | rob_exception_valid
    new_redir_target = mux(rob_exception_valid, rob_exception_pc, bru_redirect_target)

    # Interrupt aggregation → treat as exception
    any_interrupt = meip | seip | mtip | msip | debug_intr
    m.output(f"{prefix}_interrupt_pending", wire_of(any_interrupt))
    _out["interrupt_pending"] = any_interrupt

    # FU type classification
    FU_FPU = 4
    FU_LDU = 6
    FU_STU = 7
    FU_FPU_C = cas(domain, m.const(FU_FPU, width=fu_type_w), cycle=0)
    FU_LDU_C = cas(domain, m.const(FU_LDU, width=fu_type_w), cycle=0)
    FU_STU_C = cas(domain, m.const(FU_STU, width=fu_type_w), cycle=0)

    # Simplified fu_type: default ALU for decoded instructions
    dec_fu_type = [
        cas(domain, m.const(0, width=fu_type_w), cycle=0) for _ in range(decode_width)
    ]

    # Dispatch stall check — use wire_of() on cycle-0 signals in cycle-3 context
    any_blocked = wire_of(ZERO_1)
    for i in range(decode_width):
        is_fp = wire_of(dec_fu_type[i]) == wire_of(FU_FPU_C)
        is_mem = (wire_of(dec_fu_type[i]) == wire_of(FU_LDU_C)) | (
            wire_of(dec_fu_type[i]) == wire_of(FU_STU_C)
        )
        is_int = (~is_fp) & (~is_mem)
        slot_ready = is_int.select(wire_of(iq_int_ready), wire_of(ZERO_1))
        slot_ready = is_fp.select(wire_of(iq_fp_ready), slot_ready)
        slot_ready = is_mem.select(wire_of(iq_mem_ready), slot_ready)
        blocked = s3_valid & (~slot_ready)
        any_blocked = any_blocked | blocked

    pipeline_stall = any_blocked | wire_of(new_redir_valid)

    # ================================================================
    # Decoded uop outputs (Frontend → Backend path)
    # ================================================================

    INST_BYTES = 2
    for i in range(decode_width):
        slot_valid = s3_valid & (~pipeline_stall)
        inst_pc = (s3_pc_w + m.const(i * INST_BYTES, width=pc_width))[0:pc_width]
        m.output(f"{prefix}_dec_valid_{i}", slot_valid)
        m.output(f"{prefix}_dec_inst_{i}", s3_insts_w[i])
        m.output(f"{prefix}_dec_pc_{i}", inst_pc)

    # ================================================================
    # Memory dispatch (Backend → MemBlock path)
    # ================================================================

    for i in range(decode_width):
        is_mem = (wire_of(dec_fu_type[i]) == wire_of(FU_LDU_C)) | (
            wire_of(dec_fu_type[i]) == wire_of(FU_STU_C)
        )
        slot_valid = s3_valid & (~pipeline_stall) & is_mem
        m.output(f"{prefix}_mem_dp_valid_{i}", slot_valid)
        m.output(f"{prefix}_mem_dp_fu_type_{i}", wire_of(dec_fu_type[i]))

    # ================================================================
    # MemBlock → Backend: load/store writeback
    # ================================================================

    # Load writeback from MemBlock
    for i in range(num_load):
        ld_wb_v = cas(domain, m.input(f"{prefix}_ld{i}_wb_valid", width=1), cycle=0)
        ld_wb_d = cas(
            domain, m.input(f"{prefix}_ld{i}_wb_data", width=data_width), cycle=0
        )
        ld_wb_r = cas(
            domain, m.input(f"{prefix}_ld{i}_wb_rob_idx", width=rob_idx_w), cycle=0
        )
        m.output(f"{prefix}_ld{i}_wb_valid_out", wire_of(ld_wb_v))
        m.output(f"{prefix}_ld{i}_wb_data_out", wire_of(ld_wb_d))
        m.output(f"{prefix}_ld{i}_wb_rob_idx_out", wire_of(ld_wb_r))

    # Store completion from MemBlock
    for i in range(num_store):
        st_wb_v = cas(domain, m.input(f"{prefix}_st{i}_wb_valid", width=1), cycle=0)
        st_wb_r = cas(
            domain, m.input(f"{prefix}_st{i}_wb_rob_idx", width=rob_idx_w), cycle=0
        )
        m.output(f"{prefix}_st{i}_wb_valid_out", wire_of(st_wb_v))
        m.output(f"{prefix}_st{i}_wb_rob_idx_out", wire_of(st_wb_r))

    # ================================================================
    # L2 cache interface: DCache miss path
    # ================================================================

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

    m.output(f"{prefix}_l2_dcache_miss_valid", wire_of(dcache_miss_valid))
    _out["l2_dcache_miss_valid"] = dcache_miss_valid
    m.output(f"{prefix}_l2_dcache_miss_addr", wire_of(dcache_miss_addr))
    _out["l2_dcache_miss_addr"] = dcache_miss_addr
    m.output(f"{prefix}_l2_data_resp_valid_out", wire_of(l2_data_resp_valid))
    _out["l2_data_resp_valid_out"] = l2_data_resp_valid
    m.output(f"{prefix}_l2_data_resp_data_out", wire_of(l2_data_resp_data))
    _out["l2_data_resp_data_out"] = l2_data_resp_data

    # ================================================================
    # Redirect / stall outputs
    # ================================================================

    m.output(f"{prefix}_redirect_valid", wire_of(new_redir_valid))
    _out["redirect_valid"] = new_redir_valid
    m.output(f"{prefix}_redirect_target", wire_of(new_redir_target))
    _out["redirect_target"] = new_redir_target
    m.output(f"{prefix}_stall_to_frontend", pipeline_stall)
    _out["stall_to_frontend"] = cas(domain, pipeline_stall, cycle=domain.cycle_index)

    # Writeback forwarding
    (max(1, num_wb.bit_length()) if isinstance(num_wb, int) and num_wb > 0 else 3)
    for i in range(num_wb):
        m.output(f"{prefix}_rob_wb_valid_{i}", wire_of(wb_valid[i]))
        m.output(f"{prefix}_rob_wb_rob_idx_{i}", wire_of(wb_rob_idx[i]))

    # Debug port
    m.output(f"{prefix}_debug_pc", wire_of(fetch_pc))
    _out["debug_pc"] = fetch_pc

    # ================================================================
    # State updates
    # ================================================================

    domain.next()

    bpu_valid <<= ONE_1
    fetch_pc <<= next_pc
    redirect_valid <<= new_redir_valid
    redirect_target <<= new_redir_target
    be_stall <<= pipeline_stall
    return _out


xs_core.__pycircuit_name__ = "xs_core"


if __name__ == "__main__":
    pass

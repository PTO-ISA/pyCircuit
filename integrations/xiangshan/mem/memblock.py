"""MemBlock — Memory Block top-level for XiangShan-pyc.

Integrates all memory-subsystem components:
  - Load units (3 pipelines)
  - Store units (2 pipelines)
  - Load queue + Store queue (LS queue)
  - Store buffer (SBuffer)
  - Prefetcher
  - DCache interface
  - TLB interface

Provides pass-through wiring of key interfaces between sub-modules
and the external backend / cache hierarchy.

Reference: XiangShan/src/main/scala/xiangshan/mem/MemBlock.scala

Key features:
  M-MB-001  Multi-load-unit, multi-store-unit pipeline integration
  M-MB-002  Load queue + store queue coordination
  M-MB-003  Store-to-load forwarding path wiring
  M-MB-004  SBuffer drain to DCache
  M-MB-005  Prefetch request generation from load activity
  M-MB-006  Redirect/flush propagation to all sub-units
"""

from __future__ import annotations

import math
import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from cache.dcache.dcache import dcache
from cache.mmu.tlb import tlb
from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    CycleAwareSignal,
    cas,
    wire_of,
)
from top.parameters import *

from mem.lsqueue.load_queue import load_queue
from mem.lsqueue.store_queue import store_queue
from mem.pipeline.load_unit import load_unit
from mem.pipeline.store_unit import store_unit
from mem.prefetch.prefetcher import prefetcher
from mem.sbuffer.sbuffer import sbuffer


def memblock(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "mem",
    num_load: int = NUM_LDU,
    num_store: int = NUM_STA,
    lq_size: int = VIRTUAL_LOAD_QUEUE_SIZE,
    sq_size: int = STORE_QUEUE_SIZE,
    sbuf_size: int = STORE_BUFFER_SIZE,
    data_width: int = XLEN,
    addr_width: int = PC_WIDTH,
    rob_idx_width: int = ROB_IDX_WIDTH,
    lq_idx_width: int = LQ_IDX_WIDTH,
    sq_idx_width: int = SQ_IDX_WIDTH,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """MemBlock: top-level integration of load/store units, LSQ, SBuffer."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    # ── Derived widths for sub-module port matching ──
    vpn_width = 27
    ppn_width = 24
    asid_width = ASID_LENGTH

    dc_paddr_width = 36
    dc_block_bytes = DCACHE_BLOCK_BYTES
    dc_block_bits = dc_block_bytes * 8
    dc_offset_bits = int(math.log2(dc_block_bytes))
    dc_index_bits = int(math.log2(DCACHE_SETS))
    dc_tag_bits = dc_paddr_width - dc_index_bits - dc_offset_bits
    dc_way_bits = max(1, int(math.log2(DCACHE_WAYS)))

    ldq_default_size = 72
    ldq_idx_w = max(1, math.ceil(math.log2(ldq_default_size)))

    stq_default_size = 56
    stq_idx_w = max(1, math.ceil(math.log2(stq_default_size)))

    sbuf_mask_w = data_width // 8

    # ================================================================
    # Global signals
    # ================================================================

    flush = (
        _in["flush"]
        if "flush" in _in
        else cas(domain, m.input(f"{prefix}_flush", width=1), cycle=0)
    )
    cas(domain, m.const(0, width=1), cycle=0)
    cas(domain, m.const(1, width=1), cycle=0)

    # ================================================================
    # Load pipeline interfaces (N load units)
    # ================================================================

    ld_issue_valid = []
    ld_issue_addr = []
    ld_issue_rob_idx = []
    ld_issue_lq_idx = []
    for i in range(num_load):
        ld_issue_valid.append(
            cas(domain, m.input(f"{prefix}_ld{i}_issue_valid", width=1), cycle=0)
        )
        ld_issue_addr.append(
            cas(
                domain, m.input(f"{prefix}_ld{i}_issue_addr", width=addr_width), cycle=0
            )
        )
        ld_issue_rob_idx.append(
            cas(
                domain,
                m.input(f"{prefix}_ld{i}_issue_rob_idx", width=rob_idx_width),
                cycle=0,
            )
        )
        ld_issue_lq_idx.append(
            cas(
                domain,
                m.input(f"{prefix}_ld{i}_issue_lq_idx", width=lq_idx_width),
                cycle=0,
            )
        )

    ld_fwd_valid = []
    ld_fwd_data = []
    ld_tlb_resp_hit = []
    ld_tlb_resp_ppn = []
    ld_dcache_resp_valid = []
    ld_dcache_resp_data = []
    for i in range(num_load):
        ld_fwd_valid.append(
            cas(domain, m.input(f"{prefix}_ld{i}_fwd_valid", width=1), cycle=0)
        )
        ld_fwd_data.append(
            cas(domain, m.input(f"{prefix}_ld{i}_fwd_data", width=data_width), cycle=0)
        )
        ld_tlb_resp_hit.append(
            cas(domain, m.input(f"{prefix}_ld{i}_tlb_resp_hit", width=1), cycle=0)
        )
        ld_tlb_resp_ppn.append(
            cas(
                domain,
                m.input(f"{prefix}_ld{i}_tlb_resp_ppn", width=ppn_width),
                cycle=0,
            )
        )
        ld_dcache_resp_valid.append(
            cas(domain, m.input(f"{prefix}_ld{i}_dcache_resp_valid", width=1), cycle=0)
        )
        ld_dcache_resp_data.append(
            cas(
                domain,
                m.input(f"{prefix}_ld{i}_dcache_resp_data", width=data_width),
                cycle=0,
            )
        )

    # ================================================================
    # Store pipeline interfaces (N store units)
    # ================================================================

    st_issue_valid = []
    st_issue_addr = []
    st_issue_data = []
    st_issue_rob_idx = []
    st_issue_sq_idx = []
    for i in range(num_store):
        st_issue_valid.append(
            cas(domain, m.input(f"{prefix}_st{i}_issue_valid", width=1), cycle=0)
        )
        st_issue_addr.append(
            cas(
                domain, m.input(f"{prefix}_st{i}_issue_addr", width=addr_width), cycle=0
            )
        )
        st_issue_data.append(
            cas(
                domain, m.input(f"{prefix}_st{i}_issue_data", width=data_width), cycle=0
            )
        )
        st_issue_rob_idx.append(
            cas(
                domain,
                m.input(f"{prefix}_st{i}_issue_rob_idx", width=rob_idx_width),
                cycle=0,
            )
        )
        st_issue_sq_idx.append(
            cas(
                domain,
                m.input(f"{prefix}_st{i}_issue_sq_idx", width=sq_idx_width),
                cycle=0,
            )
        )

    st_tlb_resp_hit = []
    st_tlb_resp_ppn = []
    for i in range(num_store):
        st_tlb_resp_hit.append(
            cas(domain, m.input(f"{prefix}_st{i}_tlb_resp_hit", width=1), cycle=0)
        )
        st_tlb_resp_ppn.append(
            cas(
                domain,
                m.input(f"{prefix}_st{i}_tlb_resp_ppn", width=ppn_width),
                cycle=0,
            )
        )

    # ================================================================
    # External cache/TLB interface
    # ================================================================

    dcache_resp_valid = (
        _in["dcache_resp_valid"]
        if "dcache_resp_valid" in _in
        else cas(domain, m.input(f"{prefix}_dcache_resp_valid", width=1), cycle=0)
    )
    dcache_resp_data = (
        _in["dcache_resp_data"]
        if "dcache_resp_data" in _in
        else cas(
            domain, m.input(f"{prefix}_dcache_resp_data", width=data_width), cycle=0
        )
    )
    (
        _in["dcache_ready"]
        if "dcache_ready" in _in
        else cas(domain, m.input(f"{prefix}_dcache_ready", width=1), cycle=0)
    )

    tlb_resp_hit = (
        _in["tlb_resp_hit"]
        if "tlb_resp_hit" in _in
        else cas(domain, m.input(f"{prefix}_tlb_resp_hit", width=1), cycle=0)
    )
    tlb_resp_ppn = (
        _in["tlb_resp_ppn"]
        if "tlb_resp_ppn" in _in
        else cas(domain, m.input(f"{prefix}_tlb_resp_ppn", width=ppn_width), cycle=0)
    )

    # ================================================================
    # Commit interface (from ROB)
    # ================================================================

    commit_ld_valid = (
        _in["commit_ld_valid"]
        if "commit_ld_valid" in _in
        else cas(domain, m.input(f"{prefix}_commit_ld_valid", width=1), cycle=0)
    )
    commit_st_valid = (
        _in["commit_st_valid"]
        if "commit_st_valid" in _in
        else cas(domain, m.input(f"{prefix}_commit_st_valid", width=1), cycle=0)
    )

    # ================================================================
    # TLB (DTLB) sub-module inputs
    # ================================================================

    dtlb_flush_asid_valid = (
        _in["dtlb_flush_asid_valid"]
        if "dtlb_flush_asid_valid" in _in
        else cas(domain, m.input(f"{prefix}_dtlb_flush_asid_valid", width=1), cycle=0)
    )
    dtlb_flush_asid = (
        _in["dtlb_flush_asid"]
        if "dtlb_flush_asid" in _in
        else cas(
            domain, m.input(f"{prefix}_dtlb_flush_asid", width=asid_width), cycle=0
        )
    )
    dtlb_lookup_valid = (
        _in["dtlb_lookup_valid"]
        if "dtlb_lookup_valid" in _in
        else cas(domain, m.input(f"{prefix}_dtlb_lookup_valid", width=1), cycle=0)
    )
    dtlb_lookup_vpn = (
        _in["dtlb_lookup_vpn"]
        if "dtlb_lookup_vpn" in _in
        else cas(domain, m.input(f"{prefix}_dtlb_lookup_vpn", width=vpn_width), cycle=0)
    )
    dtlb_lookup_asid = (
        _in["dtlb_lookup_asid"]
        if "dtlb_lookup_asid" in _in
        else cas(
            domain, m.input(f"{prefix}_dtlb_lookup_asid", width=asid_width), cycle=0
        )
    )
    dtlb_refill_valid = (
        _in["dtlb_refill_valid"]
        if "dtlb_refill_valid" in _in
        else cas(domain, m.input(f"{prefix}_dtlb_refill_valid", width=1), cycle=0)
    )
    dtlb_refill_vpn = (
        _in["dtlb_refill_vpn"]
        if "dtlb_refill_vpn" in _in
        else cas(domain, m.input(f"{prefix}_dtlb_refill_vpn", width=vpn_width), cycle=0)
    )
    dtlb_refill_ppn = (
        _in["dtlb_refill_ppn"]
        if "dtlb_refill_ppn" in _in
        else cas(domain, m.input(f"{prefix}_dtlb_refill_ppn", width=ppn_width), cycle=0)
    )
    dtlb_refill_asid = (
        _in["dtlb_refill_asid"]
        if "dtlb_refill_asid" in _in
        else cas(
            domain, m.input(f"{prefix}_dtlb_refill_asid", width=asid_width), cycle=0
        )
    )

    # ================================================================
    # DCache sub-module inputs
    # ================================================================

    dc_load_valid = (
        _in["dc_load_valid"]
        if "dc_load_valid" in _in
        else cas(domain, m.input(f"{prefix}_dc_load_valid", width=1), cycle=0)
    )
    dc_load_vaddr = (
        _in["dc_load_vaddr"]
        if "dc_load_vaddr" in _in
        else cas(
            domain, m.input(f"{prefix}_dc_load_vaddr", width=dc_paddr_width), cycle=0
        )
    )
    dc_load_ptag = (
        _in["dc_load_ptag"]
        if "dc_load_ptag" in _in
        else cas(domain, m.input(f"{prefix}_dc_load_ptag", width=dc_tag_bits), cycle=0)
    )
    dc_store_valid = (
        _in["dc_store_valid"]
        if "dc_store_valid" in _in
        else cas(domain, m.input(f"{prefix}_dc_store_valid", width=1), cycle=0)
    )
    dc_store_vaddr = (
        _in["dc_store_vaddr"]
        if "dc_store_vaddr" in _in
        else cas(
            domain, m.input(f"{prefix}_dc_store_vaddr", width=dc_paddr_width), cycle=0
        )
    )
    dc_store_ptag = (
        _in["dc_store_ptag"]
        if "dc_store_ptag" in _in
        else cas(domain, m.input(f"{prefix}_dc_store_ptag", width=dc_tag_bits), cycle=0)
    )
    dc_store_wdata = (
        _in["dc_store_wdata"]
        if "dc_store_wdata" in _in
        else cas(
            domain, m.input(f"{prefix}_dc_store_wdata", width=dc_block_bits), cycle=0
        )
    )
    dc_store_wmask = (
        _in["dc_store_wmask"]
        if "dc_store_wmask" in _in
        else cas(
            domain, m.input(f"{prefix}_dc_store_wmask", width=dc_block_bytes), cycle=0
        )
    )
    dc_refill_valid = (
        _in["dc_refill_valid"]
        if "dc_refill_valid" in _in
        else cas(domain, m.input(f"{prefix}_dc_refill_valid", width=1), cycle=0)
    )
    dc_refill_set = (
        _in["dc_refill_set"]
        if "dc_refill_set" in _in
        else cas(
            domain, m.input(f"{prefix}_dc_refill_set", width=dc_index_bits), cycle=0
        )
    )
    dc_refill_tag = (
        _in["dc_refill_tag"]
        if "dc_refill_tag" in _in
        else cas(domain, m.input(f"{prefix}_dc_refill_tag", width=dc_tag_bits), cycle=0)
    )
    dc_refill_way = (
        _in["dc_refill_way"]
        if "dc_refill_way" in _in
        else cas(domain, m.input(f"{prefix}_dc_refill_way", width=dc_way_bits), cycle=0)
    )
    dc_refill_data = (
        _in["dc_refill_data"]
        if "dc_refill_data" in _in
        else cas(
            domain, m.input(f"{prefix}_dc_refill_data", width=dc_block_bits), cycle=0
        )
    )

    # ================================================================
    # Load Queue sub-module inputs
    # ================================================================

    ldq_enq_valid = (
        _in["ldq_enq_valid"]
        if "ldq_enq_valid" in _in
        else cas(domain, m.input(f"{prefix}_ldq_enq_valid", width=1), cycle=0)
    )
    ldq_enq_rob_idx = (
        _in["ldq_enq_rob_idx"]
        if "ldq_enq_rob_idx" in _in
        else cas(
            domain, m.input(f"{prefix}_ldq_enq_rob_idx", width=rob_idx_width), cycle=0
        )
    )
    ldq_addr_update_valid = (
        _in["ldq_addr_update_valid"]
        if "ldq_addr_update_valid" in _in
        else cas(domain, m.input(f"{prefix}_ldq_addr_update_valid", width=1), cycle=0)
    )
    ldq_addr_update_idx = (
        _in["ldq_addr_update_idx"]
        if "ldq_addr_update_idx" in _in
        else cas(
            domain, m.input(f"{prefix}_ldq_addr_update_idx", width=ldq_idx_w), cycle=0
        )
    )
    ldq_addr_update_addr = (
        _in["ldq_addr_update_addr"]
        if "ldq_addr_update_addr" in _in
        else cas(
            domain, m.input(f"{prefix}_ldq_addr_update_addr", width=addr_width), cycle=0
        )
    )
    ldq_lookup_valid = (
        _in["ldq_lookup_valid"]
        if "ldq_lookup_valid" in _in
        else cas(domain, m.input(f"{prefix}_ldq_lookup_valid", width=1), cycle=0)
    )
    ldq_lookup_addr = (
        _in["ldq_lookup_addr"]
        if "ldq_lookup_addr" in _in
        else cas(
            domain, m.input(f"{prefix}_ldq_lookup_addr", width=addr_width), cycle=0
        )
    )
    ldq_redirect_valid = (
        _in["ldq_redirect_valid"]
        if "ldq_redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_ldq_redirect_valid", width=1), cycle=0)
    )
    ldq_redirect_rob_idx = (
        _in["ldq_redirect_rob_idx"]
        if "ldq_redirect_rob_idx" in _in
        else cas(
            domain,
            m.input(f"{prefix}_ldq_redirect_rob_idx", width=rob_idx_width),
            cycle=0,
        )
    )

    # ================================================================
    # Store Queue sub-module inputs
    # ================================================================

    stq_enq_valid = (
        _in["stq_enq_valid"]
        if "stq_enq_valid" in _in
        else cas(domain, m.input(f"{prefix}_stq_enq_valid", width=1), cycle=0)
    )
    stq_enq_rob_idx = (
        _in["stq_enq_rob_idx"]
        if "stq_enq_rob_idx" in _in
        else cas(
            domain, m.input(f"{prefix}_stq_enq_rob_idx", width=rob_idx_width), cycle=0
        )
    )
    stq_write_valid = (
        _in["stq_write_valid"]
        if "stq_write_valid" in _in
        else cas(domain, m.input(f"{prefix}_stq_write_valid", width=1), cycle=0)
    )
    stq_write_idx = (
        _in["stq_write_idx"]
        if "stq_write_idx" in _in
        else cas(domain, m.input(f"{prefix}_stq_write_idx", width=stq_idx_w), cycle=0)
    )
    stq_write_addr = (
        _in["stq_write_addr"]
        if "stq_write_addr" in _in
        else cas(domain, m.input(f"{prefix}_stq_write_addr", width=addr_width), cycle=0)
    )
    stq_write_data = (
        _in["stq_write_data"]
        if "stq_write_data" in _in
        else cas(domain, m.input(f"{prefix}_stq_write_data", width=data_width), cycle=0)
    )
    stq_fwd_valid = (
        _in["stq_fwd_valid"]
        if "stq_fwd_valid" in _in
        else cas(domain, m.input(f"{prefix}_stq_fwd_valid", width=1), cycle=0)
    )
    stq_fwd_addr = (
        _in["stq_fwd_addr"]
        if "stq_fwd_addr" in _in
        else cas(domain, m.input(f"{prefix}_stq_fwd_addr", width=addr_width), cycle=0)
    )
    stq_sbuf_ready = (
        _in["stq_sbuf_ready"]
        if "stq_sbuf_ready" in _in
        else cas(domain, m.input(f"{prefix}_stq_sbuf_ready", width=1), cycle=0)
    )
    stq_redirect_valid = (
        _in["stq_redirect_valid"]
        if "stq_redirect_valid" in _in
        else cas(domain, m.input(f"{prefix}_stq_redirect_valid", width=1), cycle=0)
    )

    # ================================================================
    # SBuffer sub-module inputs
    # ================================================================

    sbuf_enq_valid = (
        _in["sbuf_enq_valid"]
        if "sbuf_enq_valid" in _in
        else cas(domain, m.input(f"{prefix}_sbuf_enq_valid", width=1), cycle=0)
    )
    sbuf_enq_addr = (
        _in["sbuf_enq_addr"]
        if "sbuf_enq_addr" in _in
        else cas(domain, m.input(f"{prefix}_sbuf_enq_addr", width=addr_width), cycle=0)
    )
    sbuf_enq_data = (
        _in["sbuf_enq_data"]
        if "sbuf_enq_data" in _in
        else cas(domain, m.input(f"{prefix}_sbuf_enq_data", width=data_width), cycle=0)
    )
    sbuf_enq_mask = (
        _in["sbuf_enq_mask"]
        if "sbuf_enq_mask" in _in
        else cas(domain, m.input(f"{prefix}_sbuf_enq_mask", width=sbuf_mask_w), cycle=0)
    )
    sbuf_dcache_ready = (
        _in["sbuf_dcache_ready"]
        if "sbuf_dcache_ready" in _in
        else cas(domain, m.input(f"{prefix}_sbuf_dcache_ready", width=1), cycle=0)
    )

    # ================================================================
    # Prefetcher sub-module inputs
    # ================================================================

    pf_train_valid = (
        _in["pf_train_valid"]
        if "pf_train_valid" in _in
        else cas(domain, m.input(f"{prefix}_pf_train_valid", width=1), cycle=0)
    )
    pf_train_pc = (
        _in["pf_train_pc"]
        if "pf_train_pc" in _in
        else cas(domain, m.input(f"{prefix}_pf_train_pc", width=PC_WIDTH), cycle=0)
    )
    pf_train_addr = (
        _in["pf_train_addr"]
        if "pf_train_addr" in _in
        else cas(domain, m.input(f"{prefix}_pf_train_addr", width=addr_width), cycle=0)
    )

    # ================================================================
    # Sub-module calls (all inputs threaded through)
    # ================================================================

    domain.call(
        tlb,
        inputs={
            "flush": flush,
            "flush_asid_valid": dtlb_flush_asid_valid,
            "flush_asid": dtlb_flush_asid,
            "lookup_valid": dtlb_lookup_valid,
            "lookup_vpn": dtlb_lookup_vpn,
            "lookup_asid": dtlb_lookup_asid,
            "refill_valid": dtlb_refill_valid,
            "refill_vpn": dtlb_refill_vpn,
            "refill_ppn": dtlb_refill_ppn,
            "refill_asid": dtlb_refill_asid,
        },
        prefix=f"{prefix}_s_dtlb",
    )

    domain.call(
        dcache,
        inputs={
            "flush": flush,
            "load_valid": dc_load_valid,
            "load_vaddr": dc_load_vaddr,
            "load_ptag": dc_load_ptag,
            "store_valid": dc_store_valid,
            "store_vaddr": dc_store_vaddr,
            "store_ptag": dc_store_ptag,
            "store_wdata": dc_store_wdata,
            "store_wmask": dc_store_wmask,
            "refill_valid": dc_refill_valid,
            "refill_set": dc_refill_set,
            "refill_tag": dc_refill_tag,
            "refill_way": dc_refill_way,
            "refill_data": dc_refill_data,
        },
        prefix=f"{prefix}_s_dc",
    )

    for _i in range(num_load):
        domain.call(
            load_unit,
            inputs={
                "flush": flush,
                "issue_valid": ld_issue_valid[_i],
                "issue_addr": ld_issue_addr[_i],
                "issue_rob_idx": ld_issue_rob_idx[_i],
                "issue_lq_idx": ld_issue_lq_idx[_i],
                "fwd_valid": ld_fwd_valid[_i],
                "fwd_data": ld_fwd_data[_i],
                "tlb_resp_hit": ld_tlb_resp_hit[_i],
                "tlb_resp_ppn": ld_tlb_resp_ppn[_i],
                "dcache_resp_valid": ld_dcache_resp_valid[_i],
                "dcache_resp_data": ld_dcache_resp_data[_i],
            },
            prefix=f"{prefix}_s_ldu{_i}",
            data_width=data_width,
            addr_width=addr_width,
            rob_idx_width=rob_idx_width,
        )

    for _i in range(num_store):
        domain.call(
            store_unit,
            inputs={
                "flush": flush,
                "issue_valid": st_issue_valid[_i],
                "issue_addr": st_issue_addr[_i],
                "issue_data": st_issue_data[_i],
                "issue_rob_idx": st_issue_rob_idx[_i],
                "issue_sq_idx": st_issue_sq_idx[_i],
                "tlb_resp_hit": st_tlb_resp_hit[_i],
                "tlb_resp_ppn": st_tlb_resp_ppn[_i],
            },
            prefix=f"{prefix}_s_stu{_i}",
            data_width=data_width,
            addr_width=addr_width,
            rob_idx_width=rob_idx_width,
        )

    domain.call(
        load_queue,
        inputs={
            "flush": flush,
            "enq_valid": ldq_enq_valid,
            "enq_rob_idx": ldq_enq_rob_idx,
            "addr_update_valid": ldq_addr_update_valid,
            "addr_update_idx": ldq_addr_update_idx,
            "addr_update_addr": ldq_addr_update_addr,
            "commit_valid": commit_ld_valid,
            "lookup_valid": ldq_lookup_valid,
            "lookup_addr": ldq_lookup_addr,
            "redirect_valid": ldq_redirect_valid,
            "redirect_rob_idx": ldq_redirect_rob_idx,
        },
        prefix=f"{prefix}_s_ldq",
        addr_width=addr_width,
    )

    domain.call(
        store_queue,
        inputs={
            "flush": flush,
            "enq_valid": stq_enq_valid,
            "enq_rob_idx": stq_enq_rob_idx,
            "write_valid": stq_write_valid,
            "write_idx": stq_write_idx,
            "write_addr": stq_write_addr,
            "write_data": stq_write_data,
            "commit_valid": commit_st_valid,
            "fwd_valid": stq_fwd_valid,
            "fwd_addr": stq_fwd_addr,
            "sbuf_ready": stq_sbuf_ready,
            "redirect_valid": stq_redirect_valid,
        },
        prefix=f"{prefix}_s_stq",
        addr_width=addr_width,
    )

    domain.call(
        sbuffer,
        inputs={
            "flush": flush,
            "enq_valid": sbuf_enq_valid,
            "enq_addr": sbuf_enq_addr,
            "enq_data": sbuf_enq_data,
            "enq_mask": sbuf_enq_mask,
            "dcache_ready": sbuf_dcache_ready,
        },
        prefix=f"{prefix}_s_sbuf",
        addr_width=addr_width,
    )

    domain.call(
        prefetcher,
        inputs={
            "train_valid": pf_train_valid,
            "train_pc": pf_train_pc,
            "train_addr": pf_train_addr,
        },
        prefix=f"{prefix}_s_pf",
        addr_width=addr_width,
    )

    # ================================================================
    # Pipeline registers: simplified single-load-unit model
    # ================================================================

    # We model the first load unit's pipeline as representative
    ld0_s0_fire = ld_issue_valid[0] & (~flush)
    ld0_s1_valid_w = domain.cycle(wire_of(ld0_s0_fire), name=f"{prefix}_ld0_s1_v")
    ld0_s1_addr_w = domain.cycle(wire_of(ld_issue_addr[0]), name=f"{prefix}_ld0_s1_a")
    ld0_s1_rob_w = domain.cycle(wire_of(ld_issue_rob_idx[0]), name=f"{prefix}_ld0_s1_r")

    domain.next()  # ── s0 → s1 ──

    ld0_s1_alive = ld0_s1_valid_w & (~wire_of(flush))
    ld0_s2_valid_w = domain.cycle(ld0_s1_alive, name=f"{prefix}_ld0_s2_v")
    ld0_s2_rob_w = domain.cycle(ld0_s1_rob_w, name=f"{prefix}_ld0_s2_r")

    domain.next()  # ── s1 → s2 ──

    ld0_s2_alive = ld0_s2_valid_w & (~wire_of(flush))
    ld0_wb_valid = ld0_s2_alive & wire_of(dcache_resp_valid)

    # Store unit pipeline: s0 → s1
    st0_s0_fire = st_issue_valid[0] & (~flush)
    st0_s1_valid_w = domain.cycle(wire_of(st0_s0_fire), name=f"{prefix}_st0_s1_v")
    st0_s1_rob_w = domain.cycle(wire_of(st_issue_rob_idx[0]), name=f"{prefix}_st0_s1_r")
    domain.cycle(wire_of(st_issue_sq_idx[0]), name=f"{prefix}_st0_s1_sq")
    domain.cycle(wire_of(st_issue_addr[0]), name=f"{prefix}_st0_s1_a")
    domain.cycle(wire_of(st_issue_data[0]), name=f"{prefix}_st0_s1_d")

    domain.next()  # ── st s0 → s1 ──

    st0_s1_alive = st0_s1_valid_w & (~wire_of(flush)) & wire_of(tlb_resp_hit)
    st0_wb_valid = st0_s1_alive

    domain.next()  # ── final stage → outputs ──

    # ================================================================
    # Output ports
    # ================================================================

    # Load writeback
    m.output(f"{prefix}_ld0_wb_valid", ld0_wb_valid)
    _out["ld0_wb_valid"] = cas(domain, ld0_wb_valid, cycle=domain.cycle_index)
    m.output(f"{prefix}_ld0_wb_data", wire_of(dcache_resp_data))
    _out["ld0_wb_data"] = dcache_resp_data
    m.output(f"{prefix}_ld0_wb_rob_idx", ld0_s2_rob_w)
    _out["ld0_wb_rob_idx"] = cas(domain, ld0_s2_rob_w, cycle=domain.cycle_index)

    # Store writeback (address phase complete)
    m.output(f"{prefix}_st0_wb_valid", st0_wb_valid)
    _out["st0_wb_valid"] = cas(domain, st0_wb_valid, cycle=domain.cycle_index)
    m.output(f"{prefix}_st0_wb_rob_idx", st0_s1_rob_w)
    _out["st0_wb_rob_idx"] = cas(domain, st0_s1_rob_w, cycle=domain.cycle_index)

    # TLB request (from load unit 0 s0)
    m.output(f"{prefix}_tlb_req_valid", wire_of(ld0_s0_fire))
    _out["tlb_req_valid"] = ld0_s0_fire
    m.output(f"{prefix}_tlb_req_vpn", wire_of(ld_issue_addr[0][12:addr_width]))

    # DCache request (from load unit 0 s1)
    m.output(f"{prefix}_dcache_req_valid", ld0_s1_alive)
    _out["dcache_req_valid"] = cas(domain, ld0_s1_alive, cycle=domain.cycle_index)
    paddr = m.cat(wire_of(tlb_resp_ppn), ld0_s1_addr_w[0:12])
    m.output(f"{prefix}_dcache_req_addr", paddr)
    _out["dcache_req_addr"] = cas(domain, paddr, cycle=domain.cycle_index)

    # SBuffer → DCache drain (from commit flow)
    m.output(f"{prefix}_sbuf_drain_valid", wire_of(commit_st_valid))
    _out["sbuf_drain_valid"] = commit_st_valid

    # Flush propagation
    m.output(f"{prefix}_flush_out", wire_of(flush))
    _out["flush_out"] = flush
    return _out


memblock.__pycircuit_name__ = "memblock"


if __name__ == "__main__":
    pass

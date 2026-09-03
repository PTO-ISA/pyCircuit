"""RegFile — Physical Integer Register File for XiangShan-pyc backend.

Multi-read, multi-write register file with:
  - Combinational read (cycle 0): mux over all entries for each read port
  - Synchronous write (cycle 1): conditional update per entry

Reference: XiangShan/src/main/scala/xiangshan/backend/regfile/

Key features:
  B-RF-001  224 physical integer registers (64-bit each)
  B-RF-002  14 read ports (6 ALU + 3 BRU + 2 MUL + 3 LDU)
  B-RF-003  8 write ports (from writeback buses)
  B-RF-004  Register 0 is hardwired to zero
  B-RF-005  Combinational read, synchronous write
"""

from __future__ import annotations

import sys
from pathlib import Path

_XS_ROOT = Path(__file__).resolve().parent.parent.parent
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
    INT_PHYS_REGS,
    PTAG_WIDTH_INT,
    XLEN,
)

NUM_READ_PORTS = 14
NUM_WRITE_PORTS = 8


def regfile(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "rf",
    num_entries: int = INT_PHYS_REGS,
    num_read: int = NUM_READ_PORTS,
    num_write: int = NUM_WRITE_PORTS,
    data_width: int = XLEN,
    addr_width: int = PTAG_WIDTH_INT,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """Physical register file: multi-read, multi-write with zero-reg."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    ZERO_DATA = cas(domain, m.const(0, width=data_width), cycle=0)

    # ── Cycle 0: Inputs ──────────────────────────────────────────
    rd_addr = [
        cas(domain, m.input(f"{prefix}_rd_addr_{i}", width=addr_width), cycle=0)
        for i in range(num_read)
    ]

    wr_en = [
        cas(domain, m.input(f"{prefix}_wr_en_{i}", width=1), cycle=0)
        for i in range(num_write)
    ]
    wr_addr = [
        cas(domain, m.input(f"{prefix}_wr_addr_{i}", width=addr_width), cycle=0)
        for i in range(num_write)
    ]
    wr_data = [
        cas(domain, m.input(f"{prefix}_wr_data_{i}", width=data_width), cycle=0)
        for i in range(num_write)
    ]

    # ── State: register entries ──────────────────────────────────
    regs = [
        domain.signal(width=data_width, reset_value=0, name=f"{prefix}_r{i}")
        for i in range(num_entries)
    ]

    # ── Cycle 0: Combinational read ──────────────────────────────
    for p in range(num_read):
        rd_val = ZERO_DATA
        for j in range(num_entries):
            hit = rd_addr[p] == cas(domain, m.const(j, width=addr_width), cycle=0)
            rd_val = mux(hit, regs[j], rd_val)
        # r0 always reads zero
        is_r0 = rd_addr[p] == cas(domain, m.const(0, width=addr_width), cycle=0)
        rd_val = mux(is_r0, ZERO_DATA, rd_val)
        m.output(f"{prefix}_rd_data_{p}", wire_of(rd_val))
        _out[f"rd_data_{p}"] = rd_val

    # ── Cycle 1: Synchronous write ───────────────────────────────
    domain.next()

    cas(domain, m.const(0, width=1), cycle=0)
    ZERO_ADDR = cas(domain, m.const(0, width=addr_width), cycle=0)

    for w in range(num_write):
        not_r0 = ~(wr_addr[w] == ZERO_ADDR)
        do_write = wr_en[w] & not_r0
        for j in range(num_entries):
            hit = wr_addr[w] == cas(domain, m.const(j, width=addr_width), cycle=0)
            we = do_write & hit
            regs[j].assign(wr_data[w], when=we)
    return _out


regfile.__pycircuit_name__ = "regfile"


if __name__ == "__main__":
    pass

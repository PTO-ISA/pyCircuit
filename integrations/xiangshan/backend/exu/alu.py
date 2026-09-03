"""ALU — Arithmetic Logic Unit for XiangShan-pyc backend.

Single-cycle, pure combinational ALU supporting RISC-V integer operations.

Reference: XiangShan/src/main/scala/xiangshan/backend/fu/Alu.scala

Operations (4-bit alu_op encoding):
  0000  ADD       1000  SLT
  0001  SUB       1001  SLTU
  0010  AND       1010  SLL
  0011  OR        1011  SRL
  0100  XOR       1100  SRA

Key features:
  B-ALU-001  Single-cycle latency (pure combinational)
  B-ALU-002  64-bit data path
  B-ALU-003  10 operations selected by 4-bit opcode
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
    u,
    wire_of,
)
from top.parameters import XLEN

ALU_OP_WIDTH = 4

OP_ADD = 0b0000
OP_SUB = 0b0001
OP_AND = 0b0010
OP_OR = 0b0011
OP_XOR = 0b0100
OP_SLT = 0b1000
OP_SLTU = 0b1001
OP_SLL = 0b1010
OP_SRL = 0b1011
OP_SRA = 0b1100


def alu(
    m: CycleAwareCircuit,
    domain: CycleAwareDomain,
    *,
    prefix: str = "alu",
    data_width: int = XLEN,
    inputs: dict[str, CycleAwareSignal] | None = None,
) -> dict[str, CycleAwareSignal]:
    """ALU: single-cycle combinational arithmetic/logic unit."""
    _in = inputs or {}
    _out: dict[str, CycleAwareSignal] = {}

    op_w = ALU_OP_WIDTH
    shamt_w = max(1, (data_width - 1).bit_length())  # 6 for 64-bit
    ext_w = data_width + 1  # for subtraction / comparison overflow

    # ── Cycle 0: Inputs ──────────────────────────────────────────
    src1 = (
        _in["src1"]
        if "src1" in _in
        else cas(domain, m.input(f"{prefix}_src1", width=data_width), cycle=0)
    )
    src2 = (
        _in["src2"]
        if "src2" in _in
        else cas(domain, m.input(f"{prefix}_src2", width=data_width), cycle=0)
    )
    alu_op = (
        _in["alu_op"]
        if "alu_op" in _in
        else cas(domain, m.input(f"{prefix}_alu_op", width=op_w), cycle=0)
    )

    def _const(val, w=data_width):
        return cas(domain, m.const(val, width=w), cycle=0)

    def _op(val):
        return cas(domain, m.const(val, width=op_w), cycle=0)

    ZERO = _const(0)
    ONE = _const(1)

    # ── Compute each operation ───────────────────────────────────

    # ADD / SUB
    add_result = cas(domain, (wire_of(src1) + wire_of(src2))[0:data_width], cycle=0)
    sub_result = cas(domain, (wire_of(src1) - wire_of(src2))[0:data_width], cycle=0)

    # Bitwise
    and_result = src1 & src2
    or_result = src1 | src2
    xor_result = src1 ^ src2

    # SLT: signed comparison via sign bit of subtraction
    # For signed: if signs differ, negative one is less; if same, check sub result
    sign1 = src1[data_width - 1 : data_width]
    sign2 = src2[data_width - 1 : data_width]
    sub_sign = sub_result[data_width - 1 : data_width]
    signs_differ = sign1 ^ sign2
    slt_result = mux(signs_differ, sign1, sub_sign)  # 1-bit
    slt_extended = mux(slt_result, ONE, ZERO)

    # SLTU: unsigned comparison
    # src1 < src2 unsigned: if sub borrows (carry out) — use extended subtraction
    ext_sub = cas(
        domain,
        (wire_of(src1) + u(ext_w, 0) - wire_of(src2) - u(ext_w, 0))[0:ext_w],
        cycle=0,
    )
    sltu_bit = ext_sub[data_width : data_width + 1]
    sltu_result = mux(sltu_bit, ONE, ZERO)

    # Shifts
    shamt = src2[0:shamt_w]
    sll_result = cas(
        domain, wire_of(src1).shl(amount=wire_of(shamt))[0:data_width], cycle=0
    )
    srl_result = cas(
        domain, wire_of(src1).lshr(amount=wire_of(shamt))[0:data_width], cycle=0
    )
    sra_result = cas(
        domain, wire_of(src1).ashr(amount=wire_of(shamt))[0:data_width], cycle=0
    )

    # ── Result mux ───────────────────────────────────────────────
    result = add_result  # default: ADD
    result = mux(alu_op == _op(OP_SUB), sub_result, result)
    result = mux(alu_op == _op(OP_AND), and_result, result)
    result = mux(alu_op == _op(OP_OR), or_result, result)
    result = mux(alu_op == _op(OP_XOR), xor_result, result)
    result = mux(alu_op == _op(OP_SLT), slt_extended, result)
    result = mux(alu_op == _op(OP_SLTU), sltu_result, result)
    result = mux(alu_op == _op(OP_SLL), sll_result, result)
    result = mux(alu_op == _op(OP_SRL), srl_result, result)
    result = mux(alu_op == _op(OP_SRA), sra_result, result)

    # ── Outputs ──────────────────────────────────────────────────
    m.output(f"{prefix}_result", wire_of(result))
    _out["result"] = result

    zero_flag = result == ZERO
    m.output(f"{prefix}_zero", wire_of(zero_flag))
    _out["zero"] = zero_flag
    return _out


alu.__pycircuit_name__ = "alu"


if __name__ == "__main__":
    pass

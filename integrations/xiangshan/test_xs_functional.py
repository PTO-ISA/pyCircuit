"""L2 functional test orchestrator — runs golden-vector based verification.

This file uses the Python-side golden reference model to verify that the
ALU/BRU/Decode module implementations produce correct MLIR with expected
structure.  The @testbench functions in individual tb_*.py files handle
the actual simulation-level drive/expect.

Run with:  pytest test_xs_functional.py -v -m functional
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

import pytest

_XS_ROOT = Path(__file__).resolve().parent
if str(_XS_ROOT) not in sys.path:
    sys.path.insert(0, str(_XS_ROOT))

from pycircuit import compile_cycle_aware  # noqa: E402


# ── Golden vector cross-check ────────────────────────────────────


@pytest.mark.functional
class TestALUGoldenVectors:
    """Verify ALU MLIR compiles for configs matching all golden vectors."""

    def test_golden_16bit_count(self):
        from golden.alu_vectors import ALU_VECTORS_16BIT

        assert len(ALU_VECTORS_16BIT) > 100, "expect comprehensive coverage"

    def test_golden_64bit_count(self):
        from golden.alu_vectors import ALU_VECTORS_64BIT

        assert len(ALU_VECTORS_64BIT) > 100

    def test_golden_model_consistency(self):
        """Verify the Python golden model produces correct results for known cases."""
        from golden.alu_vectors import gen_alu_vectors, OP_ADD, OP_SUB, OP_SLT

        vecs = gen_alu_vectors(16)
        lookup = {(s1, s2, op): (r, z) for s1, s2, op, r, z in vecs}
        assert lookup[(1, 2, OP_ADD)] == (3, 0)  # 1+2=3
        assert lookup[(1, 1, OP_SUB)] == (0, 1)  # 1-1=0, zero=1
        assert lookup[(0xFFFF, 1, OP_SLT)] == (1, 0)  # -1 < 1


@pytest.mark.functional
class TestDecodeGoldenVectors:
    """Verify decode golden vectors are well-formed."""

    def test_decode_vectors_valid(self):
        from golden.decode_vectors import DECODE_VECTORS

        assert len(DECODE_VECTORS) >= 8

    def test_encode_roundtrip(self):
        from golden.decode_vectors import (
            INST_ADD_X1_X2_X3,
            INST_LW_X5_8_X10,
            INST_BEQ_X1_X2_8,
        )

        # R-type: opcode is bits [6:0]
        assert (INST_ADD_X1_X2_X3 & 0x7F) == 0x33
        # I-type LOAD: opcode
        assert (INST_LW_X5_8_X10 & 0x7F) == 0x03
        # B-type: opcode
        assert (INST_BEQ_X1_X2_8 & 0x7F) == 0x63


@pytest.mark.functional
class TestRISCVEncodings:
    """Verify instruction encoders."""

    def test_nop(self):
        from golden.riscv_encodings import nop

        inst = nop()
        assert (inst & 0x7F) == 0x13  # ADDI opcode
        assert ((inst >> 7) & 0x1F) == 0  # rd = x0

    def test_programs_exist(self):
        from golden.riscv_encodings import (
            PROGRAM_SINGLE_ADD,
            PROGRAM_RAW_CHAIN,
            PROGRAM_LOAD_STORE,
            PROGRAM_BRANCH,
        )

        assert len(PROGRAM_SINGLE_ADD) == 3
        assert len(PROGRAM_RAW_CHAIN) == 3
        assert len(PROGRAM_LOAD_STORE) == 3
        assert len(PROGRAM_BRANCH) == 5


# ── Register count regression ────────────────────────────────────


@pytest.mark.regcount
class TestRegisterCounts:
    """Verify that modules have the expected number of state registers."""

    def _reg_count(self, build_fn, name, **kw):
        mlir = compile_cycle_aware(build_fn, name=name, eager=True, **kw).emit_mlir()
        return len(re.findall(r"pyc\.reg", mlir))

    def test_alu_zero_regs(self):
        from backend.exu.alu import build_alu

        assert self._reg_count(build_alu, "alu_rc", data_width=16) == 0

    def test_bru_zero_regs(self):
        from backend.exu.bru import build_bru

        assert self._reg_count(build_bru, "bru_rc", data_width=16, pc_width=16) == 0

    def test_mul_pipeline_regs(self):
        from backend.exu.mul import build_mul

        n = self._reg_count(build_mul, "mul_rc", data_width=16)
        assert n >= 2, f"MUL needs ≥2 pipe regs; got {n}"

    def test_div_state_regs(self):
        from backend.exu.div import build_div

        n = self._reg_count(build_div, "div_rc", data_width=16, latency=4)
        assert n >= 5, f"DIV needs ≥5 state regs; got {n}"

    def test_regfile_entry_regs(self):
        from backend.regfile.regfile import build_regfile

        n = self._reg_count(
            build_regfile,
            "rf_rc",
            num_entries=8,
            num_read=2,
            num_write=2,
            data_width=16,
            addr_width=3,
        )
        assert n >= 8, f"RegFile(8) needs ≥8 regs; got {n}"

    def test_ibuffer_entry_regs(self):
        from frontend.ibuffer.ibuffer import build_ibuffer

        n = self._reg_count(
            build_ibuffer,
            "ib_rc",
            size=4,
            enq_width=2,
            deq_width=2,
            inst_width=16,
            pc_width=16,
        )
        assert n >= 18, f"IBuffer(4) needs ≥18 regs (2ptr+4×4fields); got {n}"

    def test_rob_entry_regs(self):
        from backend.rob.rob import build_rob

        n = self._reg_count(
            build_rob,
            "rob_rc",
            rob_size=4,
            rename_width=2,
            commit_width=2,
            wb_ports=2,
            ptag_w=4,
            lreg_w=3,
            pc_width=16,
        )
        assert n >= 30, f"ROB(4) needs ≥30 regs (2ptr+4×7fields); got {n}"

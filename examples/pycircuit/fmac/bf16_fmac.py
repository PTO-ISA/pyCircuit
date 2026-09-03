"""BF16 Fused Multiply-Accumulate (FMAC) — 4-stage pipeline, pyCircuit v6.

Computes:  acc += a * b
  where a, b are BF16 (1-8-7 format), acc is FP32 (1-8-23 format).

BF16 format:  sign(1) | exponent(8) | mantissa(7)   bias=127
FP32 format:  sign(1) | exponent(8) | mantissa(23)  bias=127

Pipeline stages:
  Stage 1 (cycle 0→1): Unpack BF16 operands, compute product sign/exponent
                        depth ≈ 8 (exponent add via RCA)
  Stage 2 (cycle 1→2): 8×8 mantissa multiply (partial product + reduction)
                        depth ≈ 12 (Wallace tree + final RCA)
  Stage 3 (cycle 2→3): Align product to accumulator (barrel shift), add mantissas
                        depth ≈ 14 (shift + 26-bit RCA)
  Stage 4 (cycle 3→4): Normalize result (LZC + shift + exponent adjust), pack FP32
                        depth ≈ 14 (LZC + barrel shift + RCA)

All arithmetic built from primitive standard cells (HA, FA, RCA, MUX).
"""

from __future__ import annotations

import sys
from pathlib import Path

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    u,
)

try:
    from .primitive_standard_cells import (
        barrel_shift_left,
        barrel_shift_right,
        leading_zero_count,
        multiplier_complete_reduce,
        multiplier_pp_and_partial_reduce,
        ripple_carry_adder_packed,
        unsigned_multiplier,
    )
except ImportError:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from primitive_standard_cells import (
        barrel_shift_left,
        barrel_shift_right,
        leading_zero_count,
        multiplier_complete_reduce,
        multiplier_pp_and_partial_reduce,
    )


# ── Format constants ─────────────────────────────────────────
BF16_W = 16
BF16_EXP = 8
BF16_MAN = 7
BF16_BIAS = 127
FP32_W = 32
FP32_EXP = 8
FP32_MAN = 23
FP32_BIAS = 127

# Internal mantissa with implicit 1: 8 bits for BF16 (1.7), 24 for FP32 (1.23)
BF16_MANT_FULL = BF16_MAN + 1  # 8
FP32_MANT_FULL = FP32_MAN + 1  # 24

# Product mantissa: 8 × 8 = 16 bits (1.7 × 1.7 = 2.14, normalized to 1.15 → 16 bits)
PROD_MANT_W = BF16_MANT_FULL * 2  # 16

# Accumulator mantissa with guard bits for alignment: 26 bits
ACC_MANT_W = FP32_MANT_FULL + 2  # 26 (24 + 2 guard bits)

_pipeline_depths: dict = {}


def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    pipeline_depths = {}
    cd = domain.clock_domain
    clk = cd.clk
    rst = cd.rst

    # ════════════════════════════════════════════════════════════
    # Inputs
    # ════════════════════════════════════════════════════════════
    a_in = m.input("a_in", width=BF16_W)
    b_in = m.input("b_in", width=BF16_W)
    acc_in = m.input("acc_in", width=FP32_W)
    valid_in = m.input("valid_in", width=1)

    # ════════════════════════════════════════════════════════════
    # Pipeline registers (all declared at top level)
    # ════════════════════════════════════════════════════════════
    MAX_INTER_ROWS = 6

    # Stage 1→2 registers
    s1_prod_sign = m.out("s1_prod_sign", domain=cd, width=1, init=u(1, 0))
    s1_prod_exp = m.out("s1_prod_exp", domain=cd, width=10, init=u(10, 0))
    s1_acc_sign = m.out("s1_acc_sign", domain=cd, width=1, init=u(1, 0))
    s1_acc_exp = m.out("s1_acc_exp", domain=cd, width=8, init=u(8, 0))
    s1_acc_mant = m.out(
        "s1_acc_mant", domain=cd, width=FP32_MANT_FULL, init=u(FP32_MANT_FULL, 0)
    )
    s1_prod_zero = m.out("s1_prod_zero", domain=cd, width=1, init=u(1, 0))
    s1_acc_zero = m.out("s1_acc_zero", domain=cd, width=1, init=u(1, 0))
    s1_valid = m.out("s1_valid", domain=cd, width=1, init=u(1, 0))
    s1_mul_rows = [
        m.out(f"s1_mul_row{i}", domain=cd, width=PROD_MANT_W, init=u(PROD_MANT_W, 0))
        for i in range(MAX_INTER_ROWS)
    ]
    s1_mul_nrows = m.out("s1_mul_nrows", domain=cd, width=4, init=u(4, 0))

    # Stage 2→3 registers
    s2_prod_mant = m.out(
        "s2_prod_mant", domain=cd, width=PROD_MANT_W, init=u(PROD_MANT_W, 0)
    )
    s2_prod_sign = m.out("s2_prod_sign", domain=cd, width=1, init=u(1, 0))
    s2_prod_exp = m.out("s2_prod_exp", domain=cd, width=10, init=u(10, 0))
    s2_acc_sign = m.out("s2_acc_sign", domain=cd, width=1, init=u(1, 0))
    s2_acc_exp = m.out("s2_acc_exp", domain=cd, width=8, init=u(8, 0))
    s2_acc_mant = m.out(
        "s2_acc_mant", domain=cd, width=FP32_MANT_FULL, init=u(FP32_MANT_FULL, 0)
    )
    s2_prod_zero = m.out("s2_prod_zero", domain=cd, width=1, init=u(1, 0))
    s2_acc_zero = m.out("s2_acc_zero", domain=cd, width=1, init=u(1, 0))
    s2_valid = m.out("s2_valid", domain=cd, width=1, init=u(1, 0))

    # Stage 3→4 registers
    s3_result_sign = m.out("s3_result_sign", domain=cd, width=1, init=u(1, 0))
    s3_result_exp = m.out("s3_result_exp", domain=cd, width=10, init=u(10, 0))
    s3_result_mant = m.out(
        "s3_result_mant", domain=cd, width=ACC_MANT_W, init=u(ACC_MANT_W, 0)
    )
    s3_valid = m.out("s3_valid", domain=cd, width=1, init=u(1, 0))

    # Output registers
    result_r = m.out("result", domain=cd, width=FP32_W, init=u(FP32_W, 0))
    valid_r = m.out("result_valid", domain=cd, width=1, init=u(1, 0))

    # ════════════════════════════════════════════════════════════
    # STAGE 1 (cycle 0): Unpack + exponent add
    # ════════════════════════════════════════════════════════════
    s1_depth = 0

    # Unpack BF16 a
    a_sign = a_in[15]
    a_exp = a_in[7:15]  # 8 bits
    a_mant_raw = a_in[0:7]  # 7 bits
    a_is_zero = a_exp == u(8, 0)
    a_mant = (
        u(BF16_MANT_FULL, 0)
        if a_is_zero
        else (
            (u(1, 1) | u(BF16_MANT_FULL, 0)) << BF16_MAN
            | (a_mant_raw | u(BF16_MANT_FULL, 0))
        )
    )
    s1_depth = max(s1_depth, 3)  # mux + or

    # Unpack BF16 b
    b_sign = b_in[15]
    b_exp = b_in[7:15]
    b_mant_raw = b_in[0:7]
    b_is_zero = b_exp == u(8, 0)
    b_mant = (
        u(BF16_MANT_FULL, 0)
        if b_is_zero
        else (
            (u(1, 1) | u(BF16_MANT_FULL, 0)) << BF16_MAN
            | (b_mant_raw | u(BF16_MANT_FULL, 0))
        )
    )

    # Unpack FP32 accumulator
    acc_sign = acc_in[31]
    acc_exp = acc_in[23:31]  # 8 bits
    acc_mant_raw = acc_in[0:23]  # 23 bits
    acc_is_zero = acc_exp == u(8, 0)
    acc_mant = (
        u(FP32_MANT_FULL, 0)
        if acc_is_zero
        else (
            (u(1, 1) | u(FP32_MANT_FULL, 0)) << FP32_MAN
            | (acc_mant_raw | u(FP32_MANT_FULL, 0))
        )
    )

    # Product sign = a_sign XOR b_sign
    prod_sign = a_sign ^ b_sign
    s1_depth = max(s1_depth, 1)

    # Product exponent = a_exp + b_exp - bias (10-bit to handle overflow)
    prod_exp_sum = (a_exp | u(10, 0)) + (b_exp | u(10, 0))
    prod_exp = prod_exp_sum - u(10, BF16_BIAS)
    s1_depth = max(s1_depth, 8)

    # Product is zero if either input is zero
    prod_zero = a_is_zero | b_is_zero

    # ── Partial product generation + 2 CSA rounds (still in Stage 1) ──
    CSA_ROUNDS_IN_S1 = 2
    mul_inter_rows, pp_csa_depth = multiplier_pp_and_partial_reduce(
        m,
        a_mant,
        b_mant,
        BF16_MANT_FULL,
        BF16_MANT_FULL,
        csa_rounds=CSA_ROUNDS_IN_S1,
        name="mantmul",
    )
    n_inter_rows = len(mul_inter_rows)
    s1_depth = max(s1_depth, 8 + pp_csa_depth)

    pipeline_depths["Stage 1: Unpack + PP + 2×CSA"] = s1_depth

    # ──── Pipeline register write (stage 1) ────
    s1_prod_sign.set(prod_sign)
    s1_prod_exp.set(prod_exp)
    s1_acc_sign.set(acc_sign)
    s1_acc_exp.set(acc_exp)
    s1_acc_mant.set(acc_mant)
    s1_prod_zero.set(prod_zero)
    s1_acc_zero.set(acc_is_zero)
    s1_valid.set(valid_in)
    for i in range(MAX_INTER_ROWS):
        if i < n_inter_rows:
            s1_mul_rows[i].set(mul_inter_rows[i])
        else:
            s1_mul_rows[i].set(u(PROD_MANT_W, 0))
    s1_mul_nrows.set(u(4, n_inter_rows))

    # ════════════════════════════════════════════════════════════
    # STAGE 2 (cycle 1): Complete multiply (remaining CSA + carry-select)
    # ════════════════════════════════════════════════════════════
    prod_mant, mul_depth = multiplier_complete_reduce(
        m,
        [s1_mul_rows[i].out() for i in range(n_inter_rows)],
        PROD_MANT_W,
        name="mantmul",
    )
    pipeline_depths["Stage 2: Complete Multiply"] = mul_depth

    # ──── Pipeline register write (stage 2) ────
    s2_prod_mant.set(prod_mant)
    s2_prod_sign.set(s1_prod_sign.out())
    s2_prod_exp.set(s1_prod_exp.out())
    s2_acc_sign.set(s1_acc_sign.out())
    s2_acc_exp.set(s1_acc_exp.out())
    s2_acc_mant.set(s1_acc_mant.out())
    s2_prod_zero.set(s1_prod_zero.out())
    s2_acc_zero.set(s1_acc_zero.out())
    s2_valid.set(s1_valid.out())

    # ════════════════════════════════════════════════════════════
    # STAGE 3 (cycle 2): Align + Add
    # ════════════════════════════════════════════════════════════
    s3_depth = 0

    s2_pm = s2_prod_mant.out()
    s2_pe = s2_prod_exp.out()
    s2_ps = s2_prod_sign.out()
    s2_as = s2_acc_sign.out()
    s2_ae = s2_acc_exp.out()
    s2_am = s2_acc_mant.out()
    s2_pz = s2_prod_zero.out()

    # Normalize product mantissa: 8×8 product is in 2.14 format (16 bits).
    prod_msb = s2_pm[PROD_MANT_W - 1]
    prod_mant_norm = (s2_pm >> 1) if prod_msb else s2_pm
    prod_exp_norm = (s2_pe + 1) if prod_msb else s2_pe
    s3_depth = s3_depth + 3

    # Extend product mantissa to ACC_MANT_W (26 bits)
    prod_mant_ext = (prod_mant_norm | u(ACC_MANT_W, 0)) << 9

    # Extend accumulator mantissa to ACC_MANT_W
    acc_mant_ext = s2_am | u(ACC_MANT_W, 0)

    # Determine exponent difference and align
    prod_exp_8 = prod_exp_norm[0:8]
    exp_diff_raw = prod_exp_8.as_signed() - s2_ae.as_signed()
    exp_diff_pos = exp_diff_raw[0:8]

    prod_bigger = prod_exp_8 > s2_ae
    exp_diff_abs = (
        (prod_exp_8 - s2_ae)[0:8] if prod_bigger else (s2_ae - prod_exp_8)[0:8]
    )
    s3_depth = s3_depth + 2

    # Shift the smaller operand right to align
    shift_5 = exp_diff_abs[0:5]
    shift_capped = u(5, ACC_MANT_W) if (exp_diff_abs > u(8, ACC_MANT_W)) else shift_5

    prod_aligned = (
        prod_mant_ext
        if prod_bigger
        else barrel_shift_right(prod_mant_ext, shift_capped, ACC_MANT_W, 5, "prod_bsr")[
            0
        ]
    )
    acc_aligned = (
        barrel_shift_right(acc_mant_ext, shift_capped, ACC_MANT_W, 5, "acc_bsr")[0]
        if prod_bigger
        else acc_mant_ext
    )
    s3_depth = s3_depth + 12

    result_exp = prod_exp_8 if prod_bigger else s2_ae

    # Add or subtract mantissas based on signs
    same_sign = ~(s2_ps ^ s2_as)
    sum_mant = (
        (prod_aligned | u(ACC_MANT_W + 1, 0)) + (acc_aligned | u(ACC_MANT_W + 1, 0))
    )[0:ACC_MANT_W]

    mag_prod_ge = prod_aligned >= acc_aligned
    diff_mant = (
        (prod_aligned - acc_aligned) if mag_prod_ge else (acc_aligned - prod_aligned)
    )

    result_mant = sum_mant if same_sign else diff_mant
    result_sign = s2_ps if same_sign else (s2_ps if mag_prod_ge else s2_as)
    s3_depth = s3_depth + 4

    # Handle zeros
    result_mant_final = acc_mant_ext if s2_pz else result_mant
    result_exp_final = s2_ae if s2_pz else result_exp
    result_sign_final = s2_as if s2_pz else result_sign

    pipeline_depths["Stage 3: Align + Add"] = s3_depth

    # ──── Pipeline register write (stage 3) ────
    s3_result_sign.set(result_sign_final)
    s3_result_exp.set(result_exp_final | u(10, 0))
    s3_result_mant.set(result_mant_final)
    s3_valid.set(s2_valid.out())

    # ════════════════════════════════════════════════════════════
    # STAGE 4 (cycle 3): Normalize + Pack FP32
    # ════════════════════════════════════════════════════════════
    s4_depth = 0

    s3_rm = s3_result_mant.out()
    s3_re = s3_result_exp.out()
    s3_rs = s3_result_sign.out()
    s3_v = s3_valid.out()

    # Leading-zero count for normalization
    lzc, lzc_depth = leading_zero_count(s3_rm, ACC_MANT_W, "norm_lzc")
    s4_depth = s4_depth + lzc_depth

    GUARD_BITS = 2
    lzc_5 = lzc[0:5]

    need_left = lzc_5 > u(5, GUARD_BITS)
    need_right = lzc_5 < u(5, GUARD_BITS)

    left_amt = (lzc_5 - u(5, GUARD_BITS))[0:5]
    right_amt = (u(5, GUARD_BITS) - lzc_5)[0:5]

    left_shifted, bsl_depth = barrel_shift_left(
        s3_rm, left_amt, ACC_MANT_W, 5, "norm_bsl"
    )
    right_shifted, _ = barrel_shift_right(s3_rm, right_amt, ACC_MANT_W, 5, "norm_bsr")

    norm_mant = left_shifted if need_left else (right_shifted if need_right else s3_rm)
    s4_depth = s4_depth + bsl_depth + 4

    # Adjust exponent: exp = exp + GUARD_BITS - lzc
    norm_exp = s3_re + u(10, GUARD_BITS) - (lzc | u(10, 0))
    s4_depth = s4_depth + 4

    # Extract FP32 mantissa: implicit 1 now at bit 23.
    fp32_mant = norm_mant[0:23]  # 23 fractional bits

    # Pack FP32: sign(1) | exp(8) | mantissa(23)
    fp32_exp = norm_exp[0:8]

    # Handle zero result
    result_is_zero = s3_rm == u(ACC_MANT_W, 0)
    fp32_packed = (
        u(FP32_W, 0)
        if result_is_zero
        else (
            ((s3_rs | u(FP32_W, 0)) << 31)
            | ((fp32_exp | u(FP32_W, 0)) << 23)
            | (fp32_mant | u(FP32_W, 0))
        )
    )
    s4_depth = s4_depth + 3

    pipeline_depths["Stage 4: Normalize + Pack"] = s4_depth

    # ──── Output register write ────
    result_r.set(fp32_packed, when=s3_v)
    valid_r.set(s3_v)

    # ════════════════════════════════════════════════════════════
    # Outputs
    # ════════════════════════════════════════════════════════════
    m.output("result", result_r)
    m.output("result_valid", valid_r)

    _pipeline_depths.update(pipeline_depths)


build.__pycircuit_name__ = "bf16_fmac"

if __name__ == "__main__":
    _pipeline_depths.clear()
    circuit = compile_cycle_aware(build, name="bf16_fmac")

    print("\n" + "=" * 60)
    print("  BF16 FMAC — Pipeline Critical Path Analysis")
    print("=" * 60)
    total = 0
    for stage, depth in _pipeline_depths.items():
        print(f"  {stage:<35s}  depth = {depth:>3d}")
        total += depth
    print(f"  {'─' * 50}")
    print(f"  {'Total combinational depth':<35s}  depth = {total:>3d}")
    print(
        f"  {'Max stage depth (critical path)':<35s}  depth = {max(_pipeline_depths.values()):>3d}"
    )
    print("=" * 60 + "\n")

    mlir = circuit.emit_mlir()
    print(f"MLIR: {len(mlir)} chars")

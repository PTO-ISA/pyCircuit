# ruff: noqa: F821, F823, F841, N802 -- compiler-owned hardware state and System names.
"""Independent exact boundary expectations for the finite numeric profile."""
import pycircuit as ac
from fa2.fa2_numeric import decode16, encode16, fp_add, fp_div, fp_exp_neg, fp_mul


@ac.system
def Edges():
    gap = fp_add(1065353216, 2541748224)  # 1 - 2**-80
    cancel = fp_add(3212836864, 1065353216)
    zero = fp_add(0, 0)
    negative_zero = fp_mul(2147483648, 1065353216)
    ftz = fp_mul(8388608, 1056964608)  # smallest normal /2
    dftz = decode16(1, 0)
    bftz = decode16(1, 1)
    hmax = encode16(1199570944, 0)  # 65536 saturates FP16
    clamp = fp_exp_neg(3246391296)  # -16
    unit = fp_exp_neg(2147483648)
    quotient = fp_div(1077936128, 1073741824)  # 3/2

    @ac.rule
    def check():
        assert gap == 1065353215, "RTZ opposite-sign exponent gap"
        assert (cancel & 2147483647) == 0, "exact cancellation"
        assert zero == 0, "positive zero"
        assert negative_zero == 2147483648, "signed zero product"
        assert ftz == 0, "result FTZ"
        assert dftz == 0, "FP16 input FTZ"
        assert bftz == 0, "BF16 input FTZ"
        assert hmax == 31743, "finite half saturation"
        assert clamp == 0, "exp clamp -16"
        assert unit == 1065353216, "exp negative zero"
        assert quotient == 1069547520, "exact normalization divide"

    check()

"""The historical finite-bit BF16 FMAC with four register boundaries."""

import pycircuit as ac


@ac.struct
class Metadata:
    prod_sign: ac.u1
    prod_exp: ac.u10
    acc_sign: ac.u1
    acc_exp: ac.u8
    acc_mant: ac.u24
    prod_zero: ac.u1
    acc_zero: ac.u1
    valid: ac.u1


@ac.struct
class Stage1:
    metadata: Metadata
    row0: ac.u16
    row1: ac.u16
    row2: ac.u16
    row3: ac.u16
    row4: ac.u16
    row5: ac.u16
    nrows: ac.u4


@ac.struct
class Stage2:
    metadata: Metadata
    prod_mant: ac.u16


@ac.struct
class Stage3:
    sign: ac.u1
    exp: ac.u10
    mant: ac.u26
    valid: ac.u1


@ac.struct
class FmacResult:
    result: ac.u32
    result_valid: ac.u1


@ac.struct
class PartialProducts:
    metadata: Metadata
    pp0: ac.u16
    pp1: ac.u16
    pp2: ac.u16
    pp3: ac.u16
    pp4: ac.u16
    pp5: ac.u16
    pp6: ac.u16
    pp7: ac.u16


@ac.struct
class CarrySave:
    sum: ac.u16
    carry: ac.u16


@ac.struct
class RippleResult:
    word: ac.u8
    carry: ac.u1


@ac.struct
class Alignment:
    prod_mant: ac.u26
    acc_mant: ac.u26
    shift: ac.u5
    prod_bigger: ac.u1
    exp: ac.u8


@ac.struct
class Mantissa:
    value: ac.u26


@ac.struct
class Normalization:
    lzc: ac.u6
    left_amt: ac.u5
    right_amt: ac.u5
    need_left: ac.u1
    need_right: ac.u1


@ac.struct
class PackedResult:
    value: ac.u32


@ac.rule
def partial_products(a_in, b_in, acc_in, valid_in) -> PartialProducts:
    # Flush exponent-zero mantissas; retain the ten-bit modular exponent.
    a_exp = a_in[7:15]
    b_exp = b_in[7:15]
    acc_exp = acc_in[23:31]
    a_zero: ac.u1 = a_exp == 0
    b_zero: ac.u1 = b_exp == 0
    acc_zero: ac.u1 = acc_exp == 0
    a_fraction: ac.u8 = a_in[:7]
    b_fraction: ac.u8 = b_in[:7]
    acc_fraction: ac.u24 = acc_in[:23]
    a_mant: ac.u8 = 0 if a_zero else a_fraction | 128
    b_mant: ac.u8 = 0 if b_zero else b_fraction | 128
    acc_mant: ac.u24 = 0 if acc_zero else acc_fraction | 0x800000
    a_exp_wide: ac.u10 = a_exp
    b_exp_wide: ac.u10 = b_exp
    metadata = Metadata(
        prod_sign=a_in[15:16] ^ b_in[15:16],
        prod_exp=(a_exp_wide | 0) + (b_exp_wide | 0) - 127,
        acc_sign=acc_in[31:32],
        acc_exp=acc_exp,
        acc_mant=acc_mant,
        prod_zero=a_zero | b_zero,
        acc_zero=acc_zero,
        valid=valid_in,
    )
    # AND before widening/shifting preserves the original X/Z gate network.
    mask0: ac.u8 = 255 if a_mant[0:1] else 0
    mask1: ac.u8 = 255 if a_mant[1:2] else 0
    mask2: ac.u8 = 255 if a_mant[2:3] else 0
    mask3: ac.u8 = 255 if a_mant[3:4] else 0
    mask4: ac.u8 = 255 if a_mant[4:5] else 0
    mask5: ac.u8 = 255 if a_mant[5:6] else 0
    mask6: ac.u8 = 255 if a_mant[6:7] else 0
    mask7: ac.u8 = 255 if a_mant[7:8] else 0
    pp0: ac.u16 = b_mant & mask0
    pp1: ac.u16 = b_mant & mask1
    pp2: ac.u16 = b_mant & mask2
    pp3: ac.u16 = b_mant & mask3
    pp4: ac.u16 = b_mant & mask4
    pp5: ac.u16 = b_mant & mask5
    pp6: ac.u16 = b_mant & mask6
    pp7: ac.u16 = b_mant & mask7
    return PartialProducts(
        metadata=metadata,
        pp0=pp0 << 0,
        pp1=pp1 << 1,
        pp2=pp2 << 2,
        pp3=pp3 << 3,
        pp4=pp4 << 4,
        pp5=pp5 << 5,
        pp6=pp6 << 6,
        pp7=pp7 << 7,
    )


@ac.rule
def carry_save(a, b, c) -> CarrySave:
    return CarrySave(sum=a ^ b ^ c, carry=((a & b) | (c & (a ^ b))) << 1)


@ac.rule
def ripple8(a, b, cin) -> RippleResult:
    # Full-adder gates, not bits addition: carries remain locally resolvable.
    carry: ac.u1 = cin
    ab0 = a[0:1] ^ b[0:1]
    sum0 = ab0 ^ carry
    carry = (a[0:1] & b[0:1]) | (carry & ab0)
    ab1 = a[1:2] ^ b[1:2]
    sum1 = ab1 ^ carry
    carry = (a[1:2] & b[1:2]) | (carry & ab1)
    ab2 = a[2:3] ^ b[2:3]
    sum2 = ab2 ^ carry
    carry = (a[2:3] & b[2:3]) | (carry & ab2)
    ab3 = a[3:4] ^ b[3:4]
    sum3 = ab3 ^ carry
    carry = (a[3:4] & b[3:4]) | (carry & ab3)
    ab4 = a[4:5] ^ b[4:5]
    sum4 = ab4 ^ carry
    carry = (a[4:5] & b[4:5]) | (carry & ab4)
    ab5 = a[5:6] ^ b[5:6]
    sum5 = ab5 ^ carry
    carry = (a[5:6] & b[5:6]) | (carry & ab5)
    ab6 = a[6:7] ^ b[6:7]
    sum6 = ab6 ^ carry
    carry = (a[6:7] & b[6:7]) | (carry & ab6)
    ab7 = a[7:8] ^ b[7:8]
    sum7 = ab7 ^ carry
    carry = (a[7:8] & b[7:8]) | (carry & ab7)
    return RippleResult(
        word=ac.concat(sum7, sum6, sum5, sum4, sum3, sum2, sum1, sum0),
        carry=carry,
    )


@ac.rule
def prepare_alignment(stage) -> Alignment:
    prod_msb = stage.prod_mant[15:16]
    prod_mant = stage.prod_mant >> 1 if prod_msb else stage.prod_mant
    prod_exp = stage.metadata.prod_exp + 1 if prod_msb else stage.metadata.prod_exp
    prod_wide: ac.u26 = prod_mant
    acc_wide: ac.u26 = stage.metadata.acc_mant
    prod_exp8 = prod_exp[:8]
    prod_bigger = prod_exp8 > stage.metadata.acc_exp
    diff = (
        prod_exp8 - stage.metadata.acc_exp
        if prod_bigger
        else stage.metadata.acc_exp - prod_exp8
    )
    shift: ac.u5 = 26 if diff > 26 else diff[:5]
    return Alignment(
        prod_mant=(prod_wide | 0) << 9,
        acc_mant=acc_wide | 0,
        shift=shift,
        prod_bigger=prod_bigger,
        exp=prod_exp8 if prod_bigger else stage.metadata.acc_exp,
    )


@ac.rule
def right_barrel26(value, amount) -> Mantissa:
    shifted = value >> 1 if amount[0:1] else value
    shifted = shifted >> 2 if amount[1:2] else shifted
    shifted = shifted >> 4 if amount[2:3] else shifted
    shifted = shifted >> 8 if amount[3:4] else shifted
    shifted = shifted >> 16 if amount[4:5] else shifted
    return Mantissa(value=shifted)


@ac.rule
def combine(stage, alignment, prod_shift, acc_shift) -> Stage3:
    prod = alignment.prod_mant if alignment.prod_bigger else prod_shift.value
    acc = acc_shift.value if alignment.prod_bigger else alignment.acc_mant
    same_sign = ~(stage.metadata.prod_sign ^ stage.metadata.acc_sign)
    prod_wide: ac.u27 = prod
    acc_wide: ac.u27 = acc
    sum_mant = ((prod_wide | 0) + (acc_wide | 0))[:26]
    prod_ge = prod >= acc
    diff_mant = prod - acc if prod_ge else acc - prod
    mant = sum_mant if same_sign else diff_mant
    sign = (
        stage.metadata.prod_sign
        if same_sign
        else (stage.metadata.prod_sign if prod_ge else stage.metadata.acc_sign)
    )
    exp: ac.u10 = stage.metadata.acc_exp if stage.metadata.prod_zero else alignment.exp
    return Stage3(
        sign=stage.metadata.acc_sign if stage.metadata.prod_zero else sign,
        exp=exp | 0,
        mant=alignment.acc_mant if stage.metadata.prod_zero else mant,
        valid=stage.metadata.valid,
    )


@ac.rule
def priority_normalization(mant) -> Normalization:
    # The historical low-to-high priority mux chain preserves equal-bit merges.
    lzc: ac.u6 = 26
    lzc = 25 if mant[0:1] else lzc
    lzc = 24 if mant[1:2] else lzc
    lzc = 23 if mant[2:3] else lzc
    lzc = 22 if mant[3:4] else lzc
    lzc = 21 if mant[4:5] else lzc
    lzc = 20 if mant[5:6] else lzc
    lzc = 19 if mant[6:7] else lzc
    lzc = 18 if mant[7:8] else lzc
    lzc = 17 if mant[8:9] else lzc
    lzc = 16 if mant[9:10] else lzc
    lzc = 15 if mant[10:11] else lzc
    lzc = 14 if mant[11:12] else lzc
    lzc = 13 if mant[12:13] else lzc
    lzc = 12 if mant[13:14] else lzc
    lzc = 11 if mant[14:15] else lzc
    lzc = 10 if mant[15:16] else lzc
    lzc = 9 if mant[16:17] else lzc
    lzc = 8 if mant[17:18] else lzc
    lzc = 7 if mant[18:19] else lzc
    lzc = 6 if mant[19:20] else lzc
    lzc = 5 if mant[20:21] else lzc
    lzc = 4 if mant[21:22] else lzc
    lzc = 3 if mant[22:23] else lzc
    lzc = 2 if mant[23:24] else lzc
    lzc = 1 if mant[24:25] else lzc
    lzc = 0 if mant[25:26] else lzc
    lzc5 = lzc[:5]
    two: ac.u5 = 2
    return Normalization(
        lzc=lzc,
        left_amt=lzc5 - 2,
        right_amt=two - lzc5,
        need_left=lzc5 > 2,
        need_right=lzc5 < 2,
    )


@ac.rule
def left_barrel26(value, amount) -> Mantissa:
    shifted = value << 1 if amount[0:1] else value
    shifted = shifted << 2 if amount[1:2] else shifted
    shifted = shifted << 4 if amount[2:3] else shifted
    shifted = shifted << 8 if amount[3:4] else shifted
    shifted = shifted << 16 if amount[4:5] else shifted
    return Mantissa(value=shifted)


@ac.rule
def pack_result(stage, normalization, left, right) -> PackedResult:
    mant = (
        left.value
        if normalization.need_left
        else (right.value if normalization.need_right else stage.mant)
    )
    lzc_wide: ac.u10 = normalization.lzc
    exp = stage.exp + 2 - (lzc_wide | 0)
    sign_packed: ac.u32 = stage.sign
    exp_packed: ac.u32 = exp[:8]
    fraction_packed: ac.u32 = mant[:23]
    packed = (
        ((sign_packed | 0) << 31) | ((exp_packed | 0) << 23) | (fraction_packed | 0)
    )
    return PackedResult(value=0 if stage.mant == 0 else packed)


@ac.rule
def advance_fmac(
    s1, s2, s3, result_r, valid_r, stage1_data, stage2_data, stage3_data, packed
) -> FmacResult:
    result = FmacResult(result=result_r, result_valid=valid_r)
    output_enable = s3.valid
    s1 = stage1_data
    s2 = stage2_data
    s3 = stage3_data
    if output_enable:
        result_r = packed.value
    valid_r = output_enable
    return result


@ac.module
def BF16Fmac(  # noqa: N802
    a_in: ac.u16, b_in: ac.u16, acc_in: ac.u32, valid_in: ac.u1
) -> FmacResult:
    s1: Stage1 = Stage1()
    s2: Stage2 = Stage2()
    s3: Stage3 = Stage3()
    result_r: ac.u32 = 0
    valid_r: ac.u1 = 0

    # S1: eight AND partial products, then shared CSA rounds 8 -> 6 -> 4.
    pp = partial_products(a_in, b_in, acc_in, valid_in)
    r1a = carry_save(pp.pp0, pp.pp1, pp.pp2)
    r1b = carry_save(pp.pp3, pp.pp4, pp.pp5)
    r2a = carry_save(r1a.sum, r1a.carry, r1b.sum)
    r2b = carry_save(r1b.carry, pp.pp6, pp.pp7)
    stage1_data = Stage1(
        metadata=pp.metadata,
        row0=r2a.sum,
        row1=r2a.carry,
        row2=r2b.sum,
        row3=r2b.carry,
        nrows=4,
    )

    # S2: old S1 only; 4 -> 3 -> 2 and three shared eight-bit ripple chains.
    r3 = carry_save(s1.row0, s1.row1, s1.row2)
    r4 = carry_save(r3.sum, r3.carry, s1.row3)
    lo = ripple8(r4.sum[:8], r4.carry[:8], False)
    hi0 = ripple8(r4.sum[8:16], r4.carry[8:16], False)
    hi1 = ripple8(r4.sum[8:16], r4.carry[8:16], True)
    hi_word = hi1.word if lo.carry else hi0.word
    stage2_data = Stage2(metadata=s1.metadata, prod_mant=ac.concat(hi_word, lo.word))

    # S3: old S2 only; reuse the right barrel for both alignment paths.
    alignment = prepare_alignment(s2)
    prod_shift = right_barrel26(alignment.prod_mant, alignment.shift)
    acc_shift = right_barrel26(alignment.acc_mant, alignment.shift)
    stage3_data = combine(s2, alignment, prod_shift, acc_shift)

    # Output: old S3 only; the same right barrel is reused for normalization.
    normalization = priority_normalization(s3.mant)
    left = left_barrel26(s3.mant, normalization.left_amt)
    right = right_barrel26(s3.mant, normalization.right_amt)
    packed = pack_result(s3, normalization, left, right)
    return advance_fmac(
        s1, s2, s3, result_r, valid_r, stage1_data, stage2_data, stage3_data, packed
    )

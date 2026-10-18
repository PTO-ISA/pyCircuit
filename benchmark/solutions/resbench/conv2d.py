"""ResBench conv2d: authored from the problem and port declaration. Row-major valid cross-correlation convention used for ML convolution; the problem does not specify kernel reversal."""

import pycircuit as ac


@ac.struct
class Output:
    out_0: ac.u32
    out_1: ac.u32
    out_2: ac.u32
    out_3: ac.u32


def compute(
    in_0_0: ac.u16,
    in_0_1: ac.u16,
    in_0_2: ac.u16,
    in_0_3: ac.u16,
    in_0_4: ac.u16,
    in_0_5: ac.u16,
    in_0_6: ac.u16,
    in_0_7: ac.u16,
    in_0_8: ac.u16,
    in_1_0: ac.u16,
    in_1_1: ac.u16,
    in_1_2: ac.u16,
    in_1_3: ac.u16,
) -> Output:
    in_0_0_wide: ac.u32 = in_0_0
    in_0_0_wide = in_0_0_wide | 4294901760 if in_0_0 >= 32768 else in_0_0_wide
    in_0_1_wide: ac.u32 = in_0_1
    in_0_1_wide = in_0_1_wide | 4294901760 if in_0_1 >= 32768 else in_0_1_wide
    in_0_2_wide: ac.u32 = in_0_2
    in_0_2_wide = in_0_2_wide | 4294901760 if in_0_2 >= 32768 else in_0_2_wide
    in_0_3_wide: ac.u32 = in_0_3
    in_0_3_wide = in_0_3_wide | 4294901760 if in_0_3 >= 32768 else in_0_3_wide
    in_0_4_wide: ac.u32 = in_0_4
    in_0_4_wide = in_0_4_wide | 4294901760 if in_0_4 >= 32768 else in_0_4_wide
    in_0_5_wide: ac.u32 = in_0_5
    in_0_5_wide = in_0_5_wide | 4294901760 if in_0_5 >= 32768 else in_0_5_wide
    in_0_6_wide: ac.u32 = in_0_6
    in_0_6_wide = in_0_6_wide | 4294901760 if in_0_6 >= 32768 else in_0_6_wide
    in_0_7_wide: ac.u32 = in_0_7
    in_0_7_wide = in_0_7_wide | 4294901760 if in_0_7 >= 32768 else in_0_7_wide
    in_0_8_wide: ac.u32 = in_0_8
    in_0_8_wide = in_0_8_wide | 4294901760 if in_0_8 >= 32768 else in_0_8_wide
    in_1_0_wide: ac.u32 = in_1_0
    in_1_0_wide = in_1_0_wide | 4294901760 if in_1_0 >= 32768 else in_1_0_wide
    in_1_1_wide: ac.u32 = in_1_1
    in_1_1_wide = in_1_1_wide | 4294901760 if in_1_1 >= 32768 else in_1_1_wide
    in_1_2_wide: ac.u32 = in_1_2
    in_1_2_wide = in_1_2_wide | 4294901760 if in_1_2 >= 32768 else in_1_2_wide
    in_1_3_wide: ac.u32 = in_1_3
    in_1_3_wide = in_1_3_wide | 4294901760 if in_1_3 >= 32768 else in_1_3_wide
    return Output(
        out_0=in_0_0_wide * in_1_0_wide
        + in_0_1_wide * in_1_1_wide
        + in_0_3_wide * in_1_2_wide
        + in_0_4_wide * in_1_3_wide,
        out_1=in_0_1_wide * in_1_0_wide
        + in_0_2_wide * in_1_1_wide
        + in_0_4_wide * in_1_2_wide
        + in_0_5_wide * in_1_3_wide,
        out_2=in_0_3_wide * in_1_0_wide
        + in_0_4_wide * in_1_1_wide
        + in_0_6_wide * in_1_2_wide
        + in_0_7_wide * in_1_3_wide,
        out_3=in_0_4_wide * in_1_0_wide
        + in_0_5_wide * in_1_1_wide
        + in_0_7_wide * in_1_2_wide
        + in_0_8_wide * in_1_3_wide,
    )


@ac.module
def Top(
    in_0_0: ac.u16,
    in_0_1: ac.u16,
    in_0_2: ac.u16,
    in_0_3: ac.u16,
    in_0_4: ac.u16,
    in_0_5: ac.u16,
    in_0_6: ac.u16,
    in_0_7: ac.u16,
    in_0_8: ac.u16,
    in_1_0: ac.u16,
    in_1_1: ac.u16,
    in_1_2: ac.u16,
    in_1_3: ac.u16,
) -> Output:
    return compute(
        in_0_0,
        in_0_1,
        in_0_2,
        in_0_3,
        in_0_4,
        in_0_5,
        in_0_6,
        in_0_7,
        in_0_8,
        in_1_0,
        in_1_1,
        in_1_2,
        in_1_3,
    )

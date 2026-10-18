"""ResBench mse_loss: authored from the problem and port declaration. Computes integer floor of the exact four-square mean, then wraps to 32 bits."""

import pycircuit as ac


def compute(
    in_0_0: ac.u16,
    in_0_1: ac.u16,
    in_0_2: ac.u16,
    in_0_3: ac.u16,
    in_1_0: ac.u16,
    in_1_1: ac.u16,
    in_1_2: ac.u16,
    in_1_3: ac.u16,
) -> ac.u32:
    in_0_0_wide: ac.u34 = in_0_0
    in_0_0_wide = in_0_0_wide | 17179803648 if in_0_0 >= 32768 else in_0_0_wide
    in_0_1_wide: ac.u34 = in_0_1
    in_0_1_wide = in_0_1_wide | 17179803648 if in_0_1 >= 32768 else in_0_1_wide
    in_0_2_wide: ac.u34 = in_0_2
    in_0_2_wide = in_0_2_wide | 17179803648 if in_0_2 >= 32768 else in_0_2_wide
    in_0_3_wide: ac.u34 = in_0_3
    in_0_3_wide = in_0_3_wide | 17179803648 if in_0_3 >= 32768 else in_0_3_wide
    in_1_0_wide: ac.u34 = in_1_0
    in_1_0_wide = in_1_0_wide | 17179803648 if in_1_0 >= 32768 else in_1_0_wide
    in_1_1_wide: ac.u34 = in_1_1
    in_1_1_wide = in_1_1_wide | 17179803648 if in_1_1 >= 32768 else in_1_1_wide
    in_1_2_wide: ac.u34 = in_1_2
    in_1_2_wide = in_1_2_wide | 17179803648 if in_1_2 >= 32768 else in_1_2_wide
    in_1_3_wide: ac.u34 = in_1_3
    in_1_3_wide = in_1_3_wide | 17179803648 if in_1_3 >= 32768 else in_1_3_wide
    d0 = in_0_0_wide - in_1_0_wide
    d1 = in_0_1_wide - in_1_1_wide
    d2 = in_0_2_wide - in_1_2_wide
    d3 = in_0_3_wide - in_1_3_wide
    total = d0 * d0 + d1 * d1 + d2 * d2 + d3 * d3
    average = total // 4
    return average[:32]


@ac.module
def Top(
    in_0_0: ac.u16,
    in_0_1: ac.u16,
    in_0_2: ac.u16,
    in_0_3: ac.u16,
    in_1_0: ac.u16,
    in_1_1: ac.u16,
    in_1_2: ac.u16,
    in_1_3: ac.u16,
) -> ac.u32:
    return compute(in_0_0, in_0_1, in_0_2, in_0_3, in_1_0, in_1_1, in_1_2, in_1_3)

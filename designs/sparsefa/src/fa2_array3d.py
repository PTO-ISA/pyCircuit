# ruff: noqa: F823, F841, N802 -- compiler-owned state and hardware Module names.
"""4096 spatial products, four registered rounded tree levels, and accumulation.

Each cell owns 32 binary32 words: 16 products, 8+4+2+1 tree nodes,
and its accumulator. Five participation bits track these real data stages.
All arithmetic uses the existing finite RTZ, flush-to-zero FA2 profile.
"""
import pycircuit as ac
from fa2.fa2_array3d_interfaces import (
    Array3DAdvance,
    Array3DBegin,
    Array3DResult,
    Cell3DStep,
    Row3DStep,
)
from fa2.fa2_array_interfaces import Row16
from fa2.fa2_numeric import fp_add_value, fp_mul_value


def group_tail_mask(length: ac.u7, index: ac.u3) -> ac.u16:
    base: ac.u7 = index
    remaining = length - ((base | 0) << 4)
    mask: ac.u16 = 65535
    if remaining < 16:
        one: ac.u16 = 1
        mask = ((one | 0) << remaining) - 1
    return mask


@ac.module
def Array3DProduct():
    product: ac.u32 = 0

    @ac.method
    def load(left: ac.u32, right: ac.u32, enabled: ac.u1) -> None:
        product = 0
        if enabled:
            product = fp_mul_value(left, right)

    @ac.method
    def read() -> ac.u32:
        return product


@ac.module
def Array3DCell():
    total: ac.u32 = 0
    participation: ac.u5 = 0
    product0 = Array3DProduct()
    product1 = Array3DProduct()
    product2 = Array3DProduct()
    product3 = Array3DProduct()
    product4 = Array3DProduct()
    product5 = Array3DProduct()
    product6 = Array3DProduct()
    product7 = Array3DProduct()
    product8 = Array3DProduct()
    product9 = Array3DProduct()
    product10 = Array3DProduct()
    product11 = Array3DProduct()
    product12 = Array3DProduct()
    product13 = Array3DProduct()
    product14 = Array3DProduct()
    product15 = Array3DProduct()
    pair0: ac.u32 = 0
    pair1: ac.u32 = 0
    pair2: ac.u32 = 0
    pair3: ac.u32 = 0
    pair4: ac.u32 = 0
    pair5: ac.u32 = 0
    pair6: ac.u32 = 0
    pair7: ac.u32 = 0
    quad0: ac.u32 = 0
    quad1: ac.u32 = 0
    quad2: ac.u32 = 0
    quad3: ac.u32 = 0
    octet0: ac.u32 = 0
    octet1: ac.u32 = 0
    sum0: ac.u32 = 0

    @ac.method
    def clear() -> None:
        total = 0
        participation = 0

    @ac.method
    def advance(beat: Cell3DStep) -> None:
        # Downstream first: each level consumes the preceding committed stage.
        if participation[4]:
            total = fp_add_value(total, sum0)
        if participation[3]:
            sum0 = fp_add_value(octet0, octet1)
        if participation[2]:
            octet0 = fp_add_value(quad0, quad1)
            octet1 = fp_add_value(quad2, quad3)
        if participation[1]:
            quad0 = fp_add_value(pair0, pair1)
            quad1 = fp_add_value(pair2, pair3)
            quad2 = fp_add_value(pair4, pair5)
            quad3 = fp_add_value(pair6, pair7)
        if participation[0]:
            pair0 = fp_add_value(product0.read(), product1.read())
            pair1 = fp_add_value(product2.read(), product3.read())
            pair2 = fp_add_value(product4.read(), product5.read())
            pair3 = fp_add_value(product6.read(), product7.read())
            pair4 = fp_add_value(product8.read(), product9.read())
            pair5 = fp_add_value(product10.read(), product11.read())
            pair6 = fp_add_value(product12.read(), product13.read())
            pair7 = fp_add_value(product14.read(), product15.read())
        participation = participation << 1
        if beat.enabled and beat.k_mask != 0:
            left = beat.left
            right = beat.right
            # Masked leaves are exact +0 before any reduction, including tails.
            product0.load(left.c0, right.c0, beat.k_mask[0])
            product1.load(left.c1, right.c1, beat.k_mask[1])
            product2.load(left.c2, right.c2, beat.k_mask[2])
            product3.load(left.c3, right.c3, beat.k_mask[3])
            product4.load(left.c4, right.c4, beat.k_mask[4])
            product5.load(left.c5, right.c5, beat.k_mask[5])
            product6.load(left.c6, right.c6, beat.k_mask[6])
            product7.load(left.c7, right.c7, beat.k_mask[7])
            product8.load(left.c8, right.c8, beat.k_mask[8])
            product9.load(left.c9, right.c9, beat.k_mask[9])
            product10.load(left.c10, right.c10, beat.k_mask[10])
            product11.load(left.c11, right.c11, beat.k_mask[11])
            product12.load(left.c12, right.c12, beat.k_mask[12])
            product13.load(left.c13, right.c13, beat.k_mask[13])
            product14.load(left.c14, right.c14, beat.k_mask[14])
            product15.load(left.c15, right.c15, beat.k_mask[15])
            participation = participation | 1

    @ac.method
    def read() -> ac.u32:
        return total


@ac.module
def Array3DRow():
    cell0 = Array3DCell()
    cell1 = Array3DCell()
    cell2 = Array3DCell()
    cell3 = Array3DCell()
    cell4 = Array3DCell()
    cell5 = Array3DCell()
    cell6 = Array3DCell()
    cell7 = Array3DCell()
    cell8 = Array3DCell()
    cell9 = Array3DCell()
    cell10 = Array3DCell()
    cell11 = Array3DCell()
    cell12 = Array3DCell()
    cell13 = Array3DCell()
    cell14 = Array3DCell()
    cell15 = Array3DCell()

    @ac.method
    def clear() -> None:
        cell0.clear()
        cell1.clear()
        cell2.clear()
        cell3.clear()
        cell4.clear()
        cell5.clear()
        cell6.clear()
        cell7.clear()
        cell8.clear()
        cell9.clear()
        cell10.clear()
        cell11.clear()
        cell12.clear()
        cell13.clear()
        cell14.clear()
        cell15.clear()

    @ac.method
    def advance(beat: Row3DStep) -> ac.u32:
        cell0.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[0],
                k_mask=beat.k_mask,
                enabled=beat.mask[0],
            )
        )
        cell1.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[1],
                k_mask=beat.k_mask,
                enabled=beat.mask[1],
            )
        )
        cell2.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[2],
                k_mask=beat.k_mask,
                enabled=beat.mask[2],
            )
        )
        cell3.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[3],
                k_mask=beat.k_mask,
                enabled=beat.mask[3],
            )
        )
        cell4.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[4],
                k_mask=beat.k_mask,
                enabled=beat.mask[4],
            )
        )
        cell5.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[5],
                k_mask=beat.k_mask,
                enabled=beat.mask[5],
            )
        )
        cell6.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[6],
                k_mask=beat.k_mask,
                enabled=beat.mask[6],
            )
        )
        cell7.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[7],
                k_mask=beat.k_mask,
                enabled=beat.mask[7],
            )
        )
        cell8.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[8],
                k_mask=beat.k_mask,
                enabled=beat.mask[8],
            )
        )
        cell9.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[9],
                k_mask=beat.k_mask,
                enabled=beat.mask[9],
            )
        )
        cell10.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[10],
                k_mask=beat.k_mask,
                enabled=beat.mask[10],
            )
        )
        cell11.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[11],
                k_mask=beat.k_mask,
                enabled=beat.mask[11],
            )
        )
        cell12.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[12],
                k_mask=beat.k_mask,
                enabled=beat.mask[12],
            )
        )
        cell13.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[13],
                k_mask=beat.k_mask,
                enabled=beat.mask[13],
            )
        )
        cell14.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[14],
                k_mask=beat.k_mask,
                enabled=beat.mask[14],
            )
        )
        cell15.advance(
            Cell3DStep(
                left=beat.left,
                right=beat.right[15],
                k_mask=beat.k_mask,
                enabled=beat.mask[15],
            )
        )
        cells: ac.u32 = ac.popcount(beat.mask)
        lanes: ac.u32 = ac.popcount(beat.k_mask)
        return (cells | 0) * (lanes | 0)

    @ac.method
    def read() -> Row16:
        return Row16(
            c0=cell0.read(),
            c1=cell1.read(),
            c2=cell2.read(),
            c3=cell3.read(),
            c4=cell4.read(),
            c5=cell5.read(),
            c6=cell6.read(),
            c7=cell7.read(),
            c8=cell8.read(),
            c9=cell9.read(),
            c10=cell10.read(),
            c11=cell11.read(),
            c12=cell12.read(),
            c13=cell13.read(),
            c14=cell14.read(),
            c15=cell15.read(),
        )


@ac.module
def Array16x16x16():
    # 0 IDLE, 1 ACTIVE, 2 DRAIN, 3 RESULT. One public call per edge.
    phase: ac.u2 = 0
    saved_tag: ac.u32 = 0
    length: ac.u7 = 0
    groups: ac.u3 = 0
    cell_mask: ac.bits[256] = 0
    accepted: ac.u3 = 0
    retired: ac.u3 = 0
    occupancy: ac.u5 = 0
    useful_macs: ac.u32 = 0
    row0 = Array3DRow()
    row1 = Array3DRow()
    row2 = Array3DRow()
    row3 = Array3DRow()
    row4 = Array3DRow()
    row5 = Array3DRow()
    row6 = Array3DRow()
    row7 = Array3DRow()
    row8 = Array3DRow()
    row9 = Array3DRow()
    row10 = Array3DRow()
    row11 = Array3DRow()
    row12 = Array3DRow()
    row13 = Array3DRow()
    row14 = Array3DRow()
    row15 = Array3DRow()

    @ac.method(
        guard=lambda command: (phase == 0)
        & (command.length > 0)
        & (command.length <= 64)
    )
    def begin(command: Array3DBegin) -> None:
        saved_tag = command.tag
        length = command.length
        groups = ((command.length + 15) >> 4)[:3]
        cell_mask = command.cell_mask
        accepted = 0
        retired = 0
        occupancy = 0
        useful_macs = 0
        row0.clear()
        row1.clear()
        row2.clear()
        row3.clear()
        row4.clear()
        row5.clear()
        row6.clear()
        row7.clear()
        row8.clear()
        row9.clear()
        row10.clear()
        row11.clear()
        row12.clear()
        row13.clear()
        row14.clear()
        row15.clear()
        phase = 1

    @ac.method
    def advance(command: Array3DAdvance) -> ac.u1:
        action_accepted: ac.u1 = 0
        if (phase != 0) & (command.action == 3) & (command.tag == saved_tag):
            # Matching cancellation dominates progress, including final retirement.
            phase = 0
            occupancy = 0
            accepted = 0
            retired = 0
            useful_macs = 0
            row0.clear()
            row1.clear()
            row2.clear()
            row3.clear()
            row4.clear()
            row5.clear()
            row6.clear()
            row7.clear()
            row8.clear()
            row9.clear()
            row10.clear()
            row11.clear()
            row12.clear()
            row13.clear()
            row14.clear()
            row15.clear()
            action_accepted = 1
        elif (phase == 1) | (phase == 2):
            group_accepted: ac.u1 = 0
            if command.tag == saved_tag:
                if command.action == 0:
                    action_accepted = 1
                elif (
                    (command.action == 1)
                    & (phase == 1)
                    & (command.index == accepted)
                    & (accepted < groups)
                ):
                    group_accepted = 1
                    action_accepted = 1
                elif (command.action == 2) & (phase == 1) & (accepted == groups):
                    phase = 2
                    action_accepted = 1

            # Occupancy counts accepted groups even when every cell is masked.
            if occupancy[4]:
                retired = retired + 1
            occupancy = occupancy << 1
            effective_k: ac.u16 = 0
            if group_accepted:
                effective_k = command.k_mask & group_tail_mask(length, accepted)
                occupancy = occupancy | 1
                accepted = accepted + 1
            mask0 = (
                cell_mask[0:16] & command.col_mask
                if group_accepted & command.row_mask[0]
                else 0
            )
            count0 = row0.advance(
                Row3DStep(
                    left=command.left[0],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask0,
                )
            )
            mask1 = (
                cell_mask[16:32] & command.col_mask
                if group_accepted & command.row_mask[1]
                else 0
            )
            count1 = row1.advance(
                Row3DStep(
                    left=command.left[1],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask1,
                )
            )
            mask2 = (
                cell_mask[32:48] & command.col_mask
                if group_accepted & command.row_mask[2]
                else 0
            )
            count2 = row2.advance(
                Row3DStep(
                    left=command.left[2],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask2,
                )
            )
            mask3 = (
                cell_mask[48:64] & command.col_mask
                if group_accepted & command.row_mask[3]
                else 0
            )
            count3 = row3.advance(
                Row3DStep(
                    left=command.left[3],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask3,
                )
            )
            mask4 = (
                cell_mask[64:80] & command.col_mask
                if group_accepted & command.row_mask[4]
                else 0
            )
            count4 = row4.advance(
                Row3DStep(
                    left=command.left[4],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask4,
                )
            )
            mask5 = (
                cell_mask[80:96] & command.col_mask
                if group_accepted & command.row_mask[5]
                else 0
            )
            count5 = row5.advance(
                Row3DStep(
                    left=command.left[5],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask5,
                )
            )
            mask6 = (
                cell_mask[96:112] & command.col_mask
                if group_accepted & command.row_mask[6]
                else 0
            )
            count6 = row6.advance(
                Row3DStep(
                    left=command.left[6],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask6,
                )
            )
            mask7 = (
                cell_mask[112:128] & command.col_mask
                if group_accepted & command.row_mask[7]
                else 0
            )
            count7 = row7.advance(
                Row3DStep(
                    left=command.left[7],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask7,
                )
            )
            mask8 = (
                cell_mask[128:144] & command.col_mask
                if group_accepted & command.row_mask[8]
                else 0
            )
            count8 = row8.advance(
                Row3DStep(
                    left=command.left[8],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask8,
                )
            )
            mask9 = (
                cell_mask[144:160] & command.col_mask
                if group_accepted & command.row_mask[9]
                else 0
            )
            count9 = row9.advance(
                Row3DStep(
                    left=command.left[9],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask9,
                )
            )
            mask10 = (
                cell_mask[160:176] & command.col_mask
                if group_accepted & command.row_mask[10]
                else 0
            )
            count10 = row10.advance(
                Row3DStep(
                    left=command.left[10],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask10,
                )
            )
            mask11 = (
                cell_mask[176:192] & command.col_mask
                if group_accepted & command.row_mask[11]
                else 0
            )
            count11 = row11.advance(
                Row3DStep(
                    left=command.left[11],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask11,
                )
            )
            mask12 = (
                cell_mask[192:208] & command.col_mask
                if group_accepted & command.row_mask[12]
                else 0
            )
            count12 = row12.advance(
                Row3DStep(
                    left=command.left[12],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask12,
                )
            )
            mask13 = (
                cell_mask[208:224] & command.col_mask
                if group_accepted & command.row_mask[13]
                else 0
            )
            count13 = row13.advance(
                Row3DStep(
                    left=command.left[13],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask13,
                )
            )
            mask14 = (
                cell_mask[224:240] & command.col_mask
                if group_accepted & command.row_mask[14]
                else 0
            )
            count14 = row14.advance(
                Row3DStep(
                    left=command.left[14],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask14,
                )
            )
            mask15 = (
                cell_mask[240:256] & command.col_mask
                if group_accepted & command.row_mask[15]
                else 0
            )
            count15 = row15.advance(
                Row3DStep(
                    left=command.left[15],
                    right=command.right,
                    k_mask=effective_k,
                    mask=mask15,
                )
            )
            useful_macs = (
                useful_macs
                + count0
                + count1
                + count2
                + count3
                + count4
                + count5
                + count6
                + count7
                + count8
                + count9
                + count10
                + count11
                + count12
                + count13
                + count14
                + count15
            )
            if (phase == 2) & (retired == groups):
                phase = 3
        return action_accepted

    @ac.method(guard=lambda: phase == 3)
    def take() -> Array3DResult:
        result = Array3DResult(
            tag=saved_tag,
            groups=accepted,
            useful_macs=useful_macs,
            rows=(
                row0.read(),
                row1.read(),
                row2.read(),
                row3.read(),
                row4.read(),
                row5.read(),
                row6.read(),
                row7.read(),
                row8.read(),
                row9.read(),
                row10.read(),
                row11.read(),
                row12.read(),
                row13.read(),
                row14.read(),
                row15.read(),
            ),
        )
        phase = 0
        return result

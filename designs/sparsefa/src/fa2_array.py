# ruff: noqa: F823, F841, N802 -- compiler-owned state and hardware Module names.
"""Standalone 256-PE nonfused outer-product array; see ARRAY-1002."""
import pycircuit as ac
from fa2.fa2_array_interfaces import ArrayBegin, ArrayResult, ArrayStep, Row16, RowStep
from fa2.fa2_numeric import fp_add_value, fp_mul_value


@ac.module
def ArrayPE():
    total: ac.u32 = 0

    @ac.method
    def clear() -> None:
        total = 0

    @ac.method
    def accumulate(left: ac.u32, right: ac.u32, enabled: ac.u1) -> None:
        if enabled:
            total = fp_add_value(total, fp_mul_value(left, right))

    @ac.method
    def read() -> ac.u32:
        return total


@ac.module
def ArrayRow():
    pe0 = ArrayPE()
    pe1 = ArrayPE()
    pe2 = ArrayPE()
    pe3 = ArrayPE()
    pe4 = ArrayPE()
    pe5 = ArrayPE()
    pe6 = ArrayPE()
    pe7 = ArrayPE()
    pe8 = ArrayPE()
    pe9 = ArrayPE()
    pe10 = ArrayPE()
    pe11 = ArrayPE()
    pe12 = ArrayPE()
    pe13 = ArrayPE()
    pe14 = ArrayPE()
    pe15 = ArrayPE()

    @ac.method
    def clear() -> None:
        pe0.clear()
        pe1.clear()
        pe2.clear()
        pe3.clear()
        pe4.clear()
        pe5.clear()
        pe6.clear()
        pe7.clear()
        pe8.clear()
        pe9.clear()
        pe10.clear()
        pe11.clear()
        pe12.clear()
        pe13.clear()
        pe14.clear()
        pe15.clear()

    @ac.method
    def accumulate(beat: RowStep) -> ac.u32:
        mask = beat.mask
        if beat.enabled == 0:
            mask = 0
        pe0.accumulate(beat.left, beat.right[0], mask[0])
        pe1.accumulate(beat.left, beat.right[1], mask[1])
        pe2.accumulate(beat.left, beat.right[2], mask[2])
        pe3.accumulate(beat.left, beat.right[3], mask[3])
        pe4.accumulate(beat.left, beat.right[4], mask[4])
        pe5.accumulate(beat.left, beat.right[5], mask[5])
        pe6.accumulate(beat.left, beat.right[6], mask[6])
        pe7.accumulate(beat.left, beat.right[7], mask[7])
        pe8.accumulate(beat.left, beat.right[8], mask[8])
        pe9.accumulate(beat.left, beat.right[9], mask[9])
        pe10.accumulate(beat.left, beat.right[10], mask[10])
        pe11.accumulate(beat.left, beat.right[11], mask[11])
        pe12.accumulate(beat.left, beat.right[12], mask[12])
        pe13.accumulate(beat.left, beat.right[13], mask[13])
        pe14.accumulate(beat.left, beat.right[14], mask[14])
        pe15.accumulate(beat.left, beat.right[15], mask[15])
        return ac.popcount(mask)

    @ac.method
    def read() -> Row16:
        return Row16(
            c0=pe0.read(),
            c1=pe1.read(),
            c2=pe2.read(),
            c3=pe3.read(),
            c4=pe4.read(),
            c5=pe5.read(),
            c6=pe6.read(),
            c7=pe7.read(),
            c8=pe8.read(),
            c9=pe9.read(),
            c10=pe10.read(),
            c11=pe11.read(),
            c12=pe12.read(),
            c13=pe13.read(),
            c14=pe14.read(),
            c15=pe15.read(),
        )


@ac.module
def Array16x16():
    phase: ac.u2 = 0
    saved_tag: ac.u32 = 0
    length: ac.u7 = 0
    cell_mask: ac.bits[256] = 0
    accepted_steps: ac.u7 = 0
    useful_macs: ac.u32 = 0
    row0 = ArrayRow()
    row1 = ArrayRow()
    row2 = ArrayRow()
    row3 = ArrayRow()
    row4 = ArrayRow()
    row5 = ArrayRow()
    row6 = ArrayRow()
    row7 = ArrayRow()
    row8 = ArrayRow()
    row9 = ArrayRow()
    row10 = ArrayRow()
    row11 = ArrayRow()
    row12 = ArrayRow()
    row13 = ArrayRow()
    row14 = ArrayRow()
    row15 = ArrayRow()

    @ac.method(
        guard=lambda command: (phase == 0)
        & (command.length > 0)
        & (command.length <= 64)
    )
    def begin(command: ArrayBegin) -> None:
        saved_tag = command.tag
        length = command.length
        cell_mask = command.cell_mask
        accepted_steps = 0
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

    @ac.method(
        guard=lambda beat: (phase == 1)
        & (beat.tag == saved_tag)
        & (beat.index == accepted_steps)
        & (accepted_steps < length)
    )
    def step(beat: ArrayStep) -> None:
        count0 = row0.accumulate(
            RowStep(
                left=beat.left[0],
                right=beat.right,
                mask=cell_mask[0:16] & beat.col_mask,
                enabled=beat.row_mask[0],
            )
        )
        count1 = row1.accumulate(
            RowStep(
                left=beat.left[1],
                right=beat.right,
                mask=cell_mask[16:32] & beat.col_mask,
                enabled=beat.row_mask[1],
            )
        )
        count2 = row2.accumulate(
            RowStep(
                left=beat.left[2],
                right=beat.right,
                mask=cell_mask[32:48] & beat.col_mask,
                enabled=beat.row_mask[2],
            )
        )
        count3 = row3.accumulate(
            RowStep(
                left=beat.left[3],
                right=beat.right,
                mask=cell_mask[48:64] & beat.col_mask,
                enabled=beat.row_mask[3],
            )
        )
        count4 = row4.accumulate(
            RowStep(
                left=beat.left[4],
                right=beat.right,
                mask=cell_mask[64:80] & beat.col_mask,
                enabled=beat.row_mask[4],
            )
        )
        count5 = row5.accumulate(
            RowStep(
                left=beat.left[5],
                right=beat.right,
                mask=cell_mask[80:96] & beat.col_mask,
                enabled=beat.row_mask[5],
            )
        )
        count6 = row6.accumulate(
            RowStep(
                left=beat.left[6],
                right=beat.right,
                mask=cell_mask[96:112] & beat.col_mask,
                enabled=beat.row_mask[6],
            )
        )
        count7 = row7.accumulate(
            RowStep(
                left=beat.left[7],
                right=beat.right,
                mask=cell_mask[112:128] & beat.col_mask,
                enabled=beat.row_mask[7],
            )
        )
        count8 = row8.accumulate(
            RowStep(
                left=beat.left[8],
                right=beat.right,
                mask=cell_mask[128:144] & beat.col_mask,
                enabled=beat.row_mask[8],
            )
        )
        count9 = row9.accumulate(
            RowStep(
                left=beat.left[9],
                right=beat.right,
                mask=cell_mask[144:160] & beat.col_mask,
                enabled=beat.row_mask[9],
            )
        )
        count10 = row10.accumulate(
            RowStep(
                left=beat.left[10],
                right=beat.right,
                mask=cell_mask[160:176] & beat.col_mask,
                enabled=beat.row_mask[10],
            )
        )
        count11 = row11.accumulate(
            RowStep(
                left=beat.left[11],
                right=beat.right,
                mask=cell_mask[176:192] & beat.col_mask,
                enabled=beat.row_mask[11],
            )
        )
        count12 = row12.accumulate(
            RowStep(
                left=beat.left[12],
                right=beat.right,
                mask=cell_mask[192:208] & beat.col_mask,
                enabled=beat.row_mask[12],
            )
        )
        count13 = row13.accumulate(
            RowStep(
                left=beat.left[13],
                right=beat.right,
                mask=cell_mask[208:224] & beat.col_mask,
                enabled=beat.row_mask[13],
            )
        )
        count14 = row14.accumulate(
            RowStep(
                left=beat.left[14],
                right=beat.right,
                mask=cell_mask[224:240] & beat.col_mask,
                enabled=beat.row_mask[14],
            )
        )
        count15 = row15.accumulate(
            RowStep(
                left=beat.left[15],
                right=beat.right,
                mask=cell_mask[240:256] & beat.col_mask,
                enabled=beat.row_mask[15],
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
        accepted_steps = accepted_steps + 1

    @ac.method(
        guard=lambda tag: (phase == 1) & (tag == saved_tag) & (accepted_steps == length)
    )
    def end(tag: ac.u32) -> None:
        phase = 2

    @ac.method(guard=lambda: phase == 2)
    def take() -> ArrayResult:
        result = ArrayResult(
            tag=saved_tag,
            steps=accepted_steps,
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

    @ac.method(guard=lambda tag: (phase != 0) & (tag == saved_tag))
    def abort(tag: ac.u32) -> None:
        phase = 0

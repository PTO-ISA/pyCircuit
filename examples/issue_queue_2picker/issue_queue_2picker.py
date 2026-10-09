"""Four ordered slots with one input and two dependent output pickers."""

import pycircuit as ac


@ac.struct
class Slot:
    valid: ac.u1
    data: ac.u8


@ac.struct
class PickerResult:
    in_ready: ac.u1
    out0_valid: ac.u1
    out0_data: ac.u8
    out1_valid: ac.u1
    out1_data: ac.u8


@ac.rule
def transact(s0, s1, s2, s3, in_valid, in_data, out0_ready, out1_ready) -> PickerResult:
    pop0 = s0.valid & out0_ready
    pop1 = s1.valid & out1_ready & pop0
    in_ready = ~s3.valid | pop0
    push = in_valid & in_ready

    # Outputs observe the pre-commit slots, including data from invalid slots.
    result = PickerResult(
        in_ready=in_ready,
        out0_valid=s0.valid,
        out0_data=s0.data,
        out1_valid=s1.valid,
        out1_data=s1.data,
    )

    # Each shift clears only the last valid bit; its data continues to hold.
    a1_0 = s1 if pop0 else s0
    a1_1 = s2 if pop0 else s1
    a1_2 = s3 if pop0 else s2
    a1_3 = Slot(valid=0, data=s3.data) if pop0 else s3

    a2_0 = a1_1 if pop1 else a1_0
    a2_1 = a1_2 if pop1 else a1_1
    a2_2 = a1_3 if pop1 else a1_2
    a2_3 = Slot(valid=0, data=a1_3.data) if pop1 else a1_3

    # Insert after both shifts, in the first slot whose valid bit is clear.
    en0 = push & ~a2_0.valid
    pref1 = push & a2_0.valid
    en1 = pref1 & ~a2_1.valid
    pref2 = pref1 & a2_1.valid
    en2 = pref2 & ~a2_2.valid
    pref3 = pref2 & a2_2.valid
    en3 = pref3 & ~a2_3.valid

    # Unconditional proposals preserve value merging under unknown controls.
    s0 = Slot(valid=a2_0.valid | en0, data=in_data if en0 else a2_0.data)
    s1 = Slot(valid=a2_1.valid | en1, data=in_data if en1 else a2_1.data)
    s2 = Slot(valid=a2_2.valid | en2, data=in_data if en2 else a2_2.data)
    s3 = Slot(valid=a2_3.valid | en3, data=in_data if en3 else a2_3.data)
    return result


@ac.module
def IssueQueue2Picker(  # noqa: N802 - hardware module definition
    in_valid: ac.u1, in_data: ac.u8, out0_ready: ac.u1, out1_ready: ac.u1  # noqa: N802
) -> PickerResult:
    s0: Slot = Slot()
    s1: Slot = Slot()
    s2: Slot = Slot()
    s3: Slot = Slot()
    return transact(s0, s1, s2, s3, in_valid, in_data, out0_ready, out1_ready)

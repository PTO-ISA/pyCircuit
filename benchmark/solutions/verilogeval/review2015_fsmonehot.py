"""VerilogEval review2015_fsmonehot."""

# ruff: noqa: N802 -- benchmark top name is part of the mapping contract.
import pycircuit as ac


@ac.struct
class Outputs:
    B3_next: ac.u1
    S_next: ac.u1
    S1_next: ac.u1
    Count_next: ac.u1
    Wait_next: ac.u1
    done: ac.u1
    counting: ac.u1
    shift_ena: ac.u1


@ac.module
def Top(
    d: ac.u1, done_counting: ac.u1, ack: ac.u1, state: ac.u10
) -> Outputs:  # noqa: N802
    return Outputs(
        B3_next=state[6:7],
        S_next=(~d & (state[:1] | state[1:2] | state[3:4])) | (state[9:10] & ack),
        S1_next=state[:1] & d,
        Count_next=state[7:8] | (state[8:9] & ~done_counting),
        Wait_next=(state[8:9] & done_counting) | (state[9:10] & ~ack),
        done=state[9:10],
        counting=state[8:9],
        shift_ena=state[4:5] | state[5:6] | state[6:7] | state[7:8],
    )

# ruff: noqa: N802, F823, F841 -- captured System state.
"""Early arrival, commit retry, source-aware dedup and epoch reuse."""

import pycircuit as ac
from wse_calendar.receive import ReceiveEngine, Request, Segment


@ac.system
def ReceiveSystem():
    phase: ac.u4 = 0
    segment = Segment(
        opcode=1,
        epoch=2 if phase >= 8 else 1,
        address=100,
        nbytes=4,
        source=2 if phase >= 3 and phase <= 5 else 1,
        seq=0,
        committed=phase != 3,
        self_sourced=phase == 6,
    )
    kind = (
        1
        if phase == 1 or phase == 8
        else 3 if phase == 7 or phase == 10 else 0 if phase == 2 or phase == 11 else 2
    )
    request = Request(
        kind=kind,
        segment=segment,
        capacity=16,
        expected=4 if phase >= 8 else 8,
        compiled_expected=4 if phase >= 8 else 8,
    )
    result = ReceiveEngine(request, EARLY_SLOTS=2, SEEN_SLOTS=2)

    # ndf: verifies=WSEMIG-RX-VER-001
    @ac.rule
    def check() -> None:
        expected_status: ac.u4 = (
            6
            if phase == 0
            else (
                1
                if phase == 1 or phase == 8
                else (
                    2
                    if phase == 2 or phase == 4 or phase == 9
                    else (
                        5
                        if phase == 3
                        else (
                            3
                            if phase == 5
                            else 4 if phase == 6 else 7 if phase == 7 or phase == 10 else 0
                        )
                    )
                )
            )
        )
        expected_count: ac.u32 = (
            0 if phase <= 1 or phase == 8 else 4 if phase <= 3 or phase >= 9 else 8
        )
        assert not result.fault, "legal receive trace"
        assert result.status == expected_status, "receive disposition"
        assert result.counted == expected_count, "receive byte accounting"
        assert result.accepted == (phase != 2 and phase != 11), "external admission"
        assert result.pending == (1 if phase <= 1 else 0), "early occupancy"
        assert result.complete == (phase >= 4 and phase <= 6 or phase == 9), "completion"
        ac.log("info", "receive_checked", phase, result.status, result.counted)
        phase = phase + 1

# ruff: noqa: F823 -- captured module-owned state proposals.
"""Normal generated System scenario; detailed timing oracle lives in tests."""

import pycircuit as ac

from timed_word_pipeline.timed_word_pipeline import (
    Request,
    TimedWordPipeline,
    TimedWordPipelineAlt,
)


@ac.struct
class Scenario:
    tag: ac.u4
    enable: ac.u1
    lhs: ac.u64
    rhs: ac.u64
    operation: ac.u4


def scenarios() -> ac.table[8, Scenario]:
    return (
        Scenario(tag=0, enable=1, lhs=1, rhs=2, operation=0),
        Scenario(tag=1, enable=1, lhs=3, rhs=4, operation=2),
        Scenario(tag=2, enable=0, lhs=99, rhs=101, operation=0),
        Scenario(tag=3, enable=1, lhs=7, rhs=8, operation=15),
        Scenario(tag=4, enable=1, lhs=5, rhs=6, operation=0),
        Scenario(tag=5, enable=1, lhs=7, rhs=8, operation=0),
        Scenario(tag=6, enable=1, lhs=2, rhs=8, operation=2),
        Scenario(tag=7, enable=1, lhs=9, rhs=1, operation=1),
    )


@ac.system
def TimedWordPipelineSystem():  # noqa: N802
    phase: ac.u5 = 0
    fast_index: ac.u3 = 0
    alt_index: ac.u3 = 0
    fast_done: ac.u1 = 0
    alt_done: ac.u1 = 0
    fast_received: ac.u4 = 0
    alt_received: ac.u4 = 0
    fast = TimedWordPipeline()
    alt = TimedWordPipelineAlt()

    @ac.rule
    def feed_fast() -> None:
        if phase[0:1] & ~fast_done:
            row = scenarios()[fast_index]
            fast.submit(
                Request(
                    tag=row.tag,
                    enable=row.enable,
                    lhs=row.lhs,
                    rhs=row.rhs,
                    operation=row.operation,
                )
            )
            ac.log(
                "info",
                "fast request",
                row.tag,
                row.enable,
                row.lhs,
                row.rhs,
                row.operation,
            )
            if fast_index == 7:
                fast_done = 1
            else:
                fast_index = fast_index + 1

    @ac.rule
    def feed_alt() -> None:
        if phase[0:1] & ~alt_done:
            row = scenarios()[alt_index]
            alt.submit(
                Request(
                    tag=row.tag,
                    enable=row.enable,
                    lhs=row.lhs,
                    rhs=row.rhs,
                    operation=row.operation,
                )
            )
            ac.log(
                "info",
                "alt request",
                row.tag,
                row.enable,
                row.lhs,
                row.rhs,
                row.operation,
            )
            if alt_index == 7:
                alt_done = 1
            else:
                alt_index = alt_index + 1

    @ac.rule
    def drain_fast() -> None:
        response = fast.take()
        fast_received = fast_received + 1
        expected: ac.u64 = (
            3
            if response.tag == 0
            else (
                12
                if response.tag == 1
                else (
                    11
                    if response.tag == 4
                    else 15 if response.tag == 5 else 16 if response.tag == 6 else 8
                )
            )
        )
        assert response.valid, "timed word fast response validity"
        assert response.value == expected, "timed word fast raw result"
        ac.log(
            "info",
            "fast commit",
            response.tag,
            response.valid,
            response.invalid,
            response.value,
            fast_received,
        )

    @ac.rule(guard=lambda: phase >= 8)
    def drain_alt() -> None:
        response = alt.take()
        alt_received = alt_received + 1
        expected: ac.u64 = (
            3
            if response.tag == 0
            else (
                12
                if response.tag == 1
                else (
                    11
                    if response.tag == 4
                    else 15 if response.tag == 5 else 16 if response.tag == 6 else 8
                )
            )
        )
        assert response.valid, "timed word alt response validity"
        assert response.value == expected, "timed word alt raw result"
        ac.log(
            "info",
            "alt commit",
            response.tag,
            response.valid,
            response.invalid,
            response.value,
            alt_received,
        )

    @ac.rule
    def inspect() -> None:
        fast_accepted, fast_total, fast_completed, fast_rejected = fast.snapshot()
        alt_accepted, alt_total, alt_completed, alt_rejected = alt.snapshot()
        ac.log(
            "info",
            "timing state",
            phase,
            fast_accepted,
            fast_total,
            fast_completed,
            fast_rejected,
            alt_accepted,
            alt_total,
            alt_completed,
            alt_rejected,
        )
        if phase == 23:
            assert fast_accepted == 7, "timed word fast accepted count"
            assert fast_total == 65, "timed word fast final total"
            assert fast_completed == 7, "timed word fast completion count"
            assert fast_rejected == 1, "timed word fast rejection count"
            assert alt_accepted == 7, "timed word alt accepted count"
            assert alt_total == 65, "timed word alt final total"
            assert alt_completed == 7, "timed word alt completion count"
            assert alt_rejected == 1, "timed word alt rejection count"

    @ac.rule
    def tick() -> None:
        if phase < 23:
            phase = phase + 1

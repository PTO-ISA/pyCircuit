"""Finite system stimulus with literal independent old-state expectations."""

from example_aggregate_payload_pipeline.aggregate_payload_pipeline import (
    AggregatePacket,
    AggregatePayloadPipeline,
    Pair,
)
from pycircuit import bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_pair_first: bits[3]
    input_pair_second: bits[5]
    input_lane_0: bits[4]
    input_lane_1: bits[4]
    input_lane_2: bits[4]
    input_lane_3: bits[4]
    input_selected: bits[4]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_pair_first: bits[3]
    expected_pair_second: bits[5]
    expected_lane_0: bits[4]
    expected_lane_1: bits[4]
    expected_lane_2: bits[4]
    expected_lane_3: bits[4]
    expected_selected: bits[4]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    take: bits[1] = 0
    input_pair_first: bits[3] = 0
    input_pair_second: bits[5] = 0
    input_lane_0: bits[4] = 0
    input_lane_1: bits[4] = 0
    input_lane_2: bits[4] = 0
    input_lane_3: bits[4] = 0
    input_selected: bits[4] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_pair_first: bits[3] = 0
    expected_pair_second: bits[5] = 0
    expected_lane_0: bits[4] = 0
    expected_lane_1: bits[4] = 0
    expected_lane_2: bits[4] = 0
    expected_lane_3: bits[4] = 0
    expected_selected: bits[4] = 0
    if phase == 0:
        valid = 1
        take = 1
        input_pair_first = 5
        input_pair_second = 17
        input_lane_0 = 1
        input_lane_1 = 2
        input_lane_2 = 3
        input_lane_3 = 4
        expected_ready = 1
    if phase == 1:
        take = 1
        input_pair_first = 1
        input_pair_second = 7
        input_lane_0 = 1
        input_lane_1 = 4
        input_lane_2 = 8
        input_lane_3 = 12
        input_selected = 5
        expected_ready = 1
    if phase == 2:
        take = 1
        input_pair_first = 2
        input_pair_second = 14
        input_lane_0 = 2
        input_lane_1 = 5
        input_lane_2 = 9
        input_lane_3 = 13
        input_selected = 10
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 6
        expected_pair_second = 18
        expected_lane_0 = 2
        expected_lane_1 = 3
        expected_lane_2 = 4
        expected_lane_3 = 1
        expected_selected = 3
    if phase == 3:
        take = 1
        input_pair_first = 3
        input_pair_second = 21
        input_lane_0 = 3
        input_lane_1 = 6
        input_lane_2 = 10
        input_lane_3 = 14
        input_selected = 15
        expected_ready = 1
    if phase == 4:
        take = 1
        input_pair_first = 4
        input_pair_second = 28
        input_lane_0 = 4
        input_lane_1 = 7
        input_lane_2 = 11
        input_lane_3 = 15
        input_selected = 4
        expected_ready = 1
    if phase == 5:
        take = 1
        input_pair_first = 5
        input_pair_second = 3
        input_lane_0 = 5
        input_lane_1 = 8
        input_lane_2 = 12
        input_selected = 9
        expected_ready = 1
    if phase == 6:
        take = 1
        input_pair_first = 6
        input_pair_second = 10
        input_lane_0 = 6
        input_lane_1 = 9
        input_lane_2 = 13
        input_lane_3 = 1
        input_selected = 14
        expected_ready = 1
    if phase == 7:
        take = 1
        input_pair_first = 7
        input_pair_second = 17
        input_lane_0 = 7
        input_lane_1 = 10
        input_lane_2 = 14
        input_lane_3 = 2
        input_selected = 3
        expected_ready = 1
    if phase == 8:
        take = 1
        input_pair_second = 24
        input_lane_0 = 8
        input_lane_1 = 11
        input_lane_2 = 15
        input_lane_3 = 3
        input_selected = 8
        expected_ready = 1
    if phase == 9:
        take = 1
        input_pair_first = 1
        input_pair_second = 31
        input_lane_0 = 9
        input_lane_1 = 12
        input_lane_3 = 4
        input_selected = 13
        expected_ready = 1
    if phase == 10:
        valid = 1
        take = 1
        input_pair_first = 5
        input_pair_second = 29
        input_lane_0 = 1
        input_lane_1 = 2
        input_lane_2 = 3
        input_lane_3 = 4
        input_selected = 15
        expected_ready = 1
    if phase == 11:
        take = 1
        input_pair_first = 3
        input_pair_second = 13
        input_lane_0 = 11
        input_lane_1 = 14
        input_lane_2 = 2
        input_lane_3 = 6
        input_selected = 7
        expected_ready = 1
    if phase == 12:
        take = 1
        input_pair_first = 4
        input_pair_second = 20
        input_lane_0 = 12
        input_lane_1 = 15
        input_lane_2 = 3
        input_lane_3 = 7
        input_selected = 12
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 6
        expected_pair_second = 30
        expected_lane_0 = 2
        expected_lane_1 = 3
        expected_lane_2 = 4
        expected_lane_3 = 1
        expected_selected = 3
    if phase == 13:
        take = 1
        input_pair_first = 5
        input_pair_second = 27
        input_lane_0 = 13
        input_lane_2 = 4
        input_lane_3 = 8
        input_selected = 1
        expected_ready = 1
    if phase == 14:
        valid = 1
        input_pair_first = 6
        input_pair_second = 2
        input_lane_0 = 14
        input_lane_1 = 1
        input_lane_2 = 5
        input_lane_3 = 9
        input_selected = 6
        expected_ready = 1
    if phase == 15:
        valid = 1
        input_pair_first = 7
        input_pair_second = 9
        input_lane_0 = 15
        input_lane_1 = 2
        input_lane_2 = 6
        input_lane_3 = 10
        input_selected = 11
        expected_ready = 1
    if phase == 16:
        valid = 1
        input_pair_second = 16
        input_lane_1 = 3
        input_lane_2 = 7
        input_lane_3 = 11
        expected_valid = 1
        expected_pair_first = 7
        expected_pair_second = 3
        expected_lane_0 = 1
        expected_lane_1 = 5
        expected_lane_2 = 9
        expected_lane_3 = 14
        expected_selected = 5
    if phase == 17:
        valid = 1
        input_pair_first = 1
        input_pair_second = 23
        input_lane_0 = 1
        input_lane_1 = 4
        input_lane_2 = 8
        input_lane_3 = 12
        input_selected = 5
        expected_valid = 1
        expected_pair_first = 7
        expected_pair_second = 3
        expected_lane_0 = 1
        expected_lane_1 = 5
        expected_lane_2 = 9
        expected_lane_3 = 14
        expected_selected = 5
    if phase == 18:
        valid = 1
        input_pair_first = 2
        input_pair_second = 30
        input_lane_0 = 2
        input_lane_1 = 5
        input_lane_2 = 9
        input_lane_3 = 13
        input_selected = 10
        expected_valid = 1
        expected_pair_first = 7
        expected_pair_second = 3
        expected_lane_0 = 1
        expected_lane_1 = 5
        expected_lane_2 = 9
        expected_lane_3 = 14
        expected_selected = 5
    if phase == 19:
        valid = 1
        input_pair_first = 3
        input_pair_second = 5
        input_lane_0 = 3
        input_lane_1 = 6
        input_lane_2 = 10
        input_lane_3 = 14
        input_selected = 15
        expected_valid = 1
        expected_pair_first = 7
        expected_pair_second = 3
        expected_lane_0 = 1
        expected_lane_1 = 5
        expected_lane_2 = 9
        expected_lane_3 = 14
        expected_selected = 5
    if phase == 20:
        valid = 1
        take = 1
        input_pair_first = 4
        input_pair_second = 12
        input_lane_0 = 4
        input_lane_1 = 7
        input_lane_2 = 11
        input_lane_3 = 15
        input_selected = 4
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 7
        expected_pair_second = 3
        expected_lane_0 = 1
        expected_lane_1 = 5
        expected_lane_2 = 9
        expected_lane_3 = 14
        expected_selected = 5
    if phase == 21:
        valid = 1
        take = 1
        input_pair_first = 5
        input_pair_second = 19
        input_lane_0 = 5
        input_lane_1 = 8
        input_lane_2 = 12
        input_selected = 9
        expected_ready = 1
        expected_valid = 1
        expected_pair_second = 10
        expected_lane_0 = 2
        expected_lane_1 = 6
        expected_lane_2 = 10
        expected_lane_3 = 15
        expected_selected = 6
    if phase == 22:
        valid = 1
        take = 1
        input_pair_first = 6
        input_pair_second = 26
        input_lane_0 = 6
        input_lane_1 = 9
        input_lane_2 = 13
        input_lane_3 = 1
        input_selected = 14
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 5
        expected_pair_second = 13
        expected_lane_0 = 7
        expected_lane_1 = 11
        expected_lane_2 = 15
        expected_lane_3 = 4
        expected_selected = 11
    if phase == 23:
        valid = 1
        take = 1
        input_pair_first = 7
        input_pair_second = 1
        input_lane_0 = 7
        input_lane_1 = 10
        input_lane_2 = 14
        input_lane_3 = 2
        input_selected = 3
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 6
        expected_pair_second = 20
        expected_lane_0 = 8
        expected_lane_1 = 12
        expected_lane_3 = 5
        expected_selected = 12
    if phase == 24:
        valid = 1
        take = 1
        input_pair_second = 8
        input_lane_0 = 8
        input_lane_1 = 11
        input_lane_2 = 15
        input_lane_3 = 3
        input_selected = 8
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 7
        expected_pair_second = 27
        expected_lane_0 = 9
        expected_lane_1 = 13
        expected_lane_2 = 1
        expected_lane_3 = 6
        expected_selected = 13
    if phase == 25:
        valid = 1
        take = 1
        input_pair_first = 1
        input_pair_second = 15
        input_lane_0 = 9
        input_lane_1 = 12
        input_lane_3 = 4
        input_selected = 13
        expected_ready = 1
        expected_valid = 1
        expected_pair_second = 2
        expected_lane_0 = 10
        expected_lane_1 = 14
        expected_lane_2 = 2
        expected_lane_3 = 7
        expected_selected = 14
    if phase == 26:
        valid = 1
        take = 1
        input_pair_first = 2
        input_pair_second = 22
        input_lane_0 = 10
        input_lane_1 = 13
        input_lane_2 = 1
        input_lane_3 = 5
        input_selected = 2
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 1
        expected_pair_second = 9
        expected_lane_0 = 11
        expected_lane_1 = 15
        expected_lane_2 = 3
        expected_lane_3 = 8
        expected_selected = 15
    if phase == 27:
        valid = 1
        take = 1
        input_pair_first = 3
        input_pair_second = 29
        input_lane_0 = 11
        input_lane_1 = 14
        input_lane_2 = 2
        input_lane_3 = 6
        input_selected = 7
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 2
        expected_pair_second = 16
        expected_lane_0 = 12
        expected_lane_2 = 4
        expected_lane_3 = 9
    if phase == 28:
        valid = 1
        take = 1
        input_pair_first = 4
        input_pair_second = 4
        input_lane_0 = 12
        input_lane_1 = 15
        input_lane_2 = 3
        input_lane_3 = 7
        input_selected = 12
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 3
        expected_pair_second = 23
        expected_lane_0 = 13
        expected_lane_1 = 1
        expected_lane_2 = 5
        expected_lane_3 = 10
        expected_selected = 1
    if phase == 29:
        valid = 1
        take = 1
        input_pair_first = 5
        input_pair_second = 11
        input_lane_0 = 13
        input_lane_2 = 4
        input_lane_3 = 8
        input_selected = 1
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 4
        expected_pair_second = 30
        expected_lane_0 = 14
        expected_lane_1 = 2
        expected_lane_2 = 6
        expected_lane_3 = 11
        expected_selected = 2
    if phase == 30:
        valid = 1
        take = 1
        input_pair_first = 6
        input_pair_second = 18
        input_lane_0 = 14
        input_lane_1 = 1
        input_lane_2 = 5
        input_lane_3 = 9
        input_selected = 6
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 5
        expected_pair_second = 5
        expected_lane_0 = 15
        expected_lane_1 = 3
        expected_lane_2 = 7
        expected_lane_3 = 12
        expected_selected = 3
    if phase == 31:
        valid = 1
        take = 1
        input_pair_first = 7
        input_pair_second = 25
        input_lane_0 = 15
        input_lane_1 = 2
        input_lane_2 = 6
        input_lane_3 = 10
        input_selected = 11
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 6
        expected_pair_second = 12
        expected_lane_1 = 4
        expected_lane_2 = 8
        expected_lane_3 = 13
        expected_selected = 4
    if phase == 32:
        valid = 1
        take = 1
        input_lane_1 = 3
        input_lane_2 = 7
        input_lane_3 = 11
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 7
        expected_pair_second = 19
        expected_lane_0 = 1
        expected_lane_1 = 5
        expected_lane_2 = 9
        expected_lane_3 = 14
        expected_selected = 5
    if phase == 33:
        valid = 1
        take = 1
        input_pair_first = 1
        input_pair_second = 7
        input_lane_0 = 1
        input_lane_1 = 4
        input_lane_2 = 8
        input_lane_3 = 12
        input_selected = 5
        expected_ready = 1
        expected_valid = 1
        expected_pair_second = 26
        expected_lane_0 = 2
        expected_lane_1 = 6
        expected_lane_2 = 10
        expected_lane_3 = 15
        expected_selected = 6
    if phase == 34:
        valid = 1
        take = 1
        input_pair_first = 2
        input_pair_second = 14
        input_lane_0 = 2
        input_lane_1 = 5
        input_lane_2 = 9
        input_lane_3 = 13
        input_selected = 10
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 1
        expected_pair_second = 1
        expected_lane_0 = 3
        expected_lane_1 = 7
        expected_lane_2 = 11
        expected_selected = 7
    if phase == 35:
        valid = 1
        take = 1
        input_pair_first = 3
        input_pair_second = 21
        input_lane_0 = 3
        input_lane_1 = 6
        input_lane_2 = 10
        input_lane_3 = 14
        input_selected = 15
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 2
        expected_pair_second = 8
        expected_lane_0 = 4
        expected_lane_1 = 8
        expected_lane_2 = 12
        expected_lane_3 = 1
        expected_selected = 8
    if phase == 36:
        valid = 1
        take = 1
        input_pair_first = 4
        input_pair_second = 28
        input_lane_0 = 4
        input_lane_1 = 7
        input_lane_2 = 11
        input_lane_3 = 15
        input_selected = 4
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 3
        expected_pair_second = 15
        expected_lane_0 = 5
        expected_lane_1 = 9
        expected_lane_2 = 13
        expected_lane_3 = 2
        expected_selected = 9
    if phase == 37:
        valid = 1
        take = 1
        input_pair_first = 5
        input_pair_second = 3
        input_lane_0 = 5
        input_lane_1 = 8
        input_lane_2 = 12
        input_selected = 9
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 4
        expected_pair_second = 22
        expected_lane_0 = 6
        expected_lane_1 = 10
        expected_lane_2 = 14
        expected_lane_3 = 3
        expected_selected = 10
    if phase == 38:
        valid = 1
        take = 1
        input_pair_first = 6
        input_pair_second = 10
        input_lane_0 = 6
        input_lane_1 = 9
        input_lane_2 = 13
        input_lane_3 = 1
        input_selected = 14
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 5
        expected_pair_second = 29
        expected_lane_0 = 7
        expected_lane_1 = 11
        expected_lane_2 = 15
        expected_lane_3 = 4
        expected_selected = 11
    if phase == 39:
        valid = 1
        take = 1
        input_pair_first = 7
        input_pair_second = 17
        input_lane_0 = 7
        input_lane_1 = 10
        input_lane_2 = 14
        input_lane_3 = 2
        input_selected = 3
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 6
        expected_pair_second = 4
        expected_lane_0 = 8
        expected_lane_1 = 12
        expected_lane_3 = 5
        expected_selected = 12
    if phase == 40:
        valid = 1
        take = 1
        input_pair_second = 24
        input_lane_0 = 8
        input_lane_1 = 11
        input_lane_2 = 15
        input_lane_3 = 3
        input_selected = 8
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 7
        expected_pair_second = 11
        expected_lane_0 = 9
        expected_lane_1 = 13
        expected_lane_2 = 1
        expected_lane_3 = 6
        expected_selected = 13
    if phase == 41:
        valid = 1
        take = 1
        input_pair_first = 1
        input_pair_second = 31
        input_lane_0 = 9
        input_lane_1 = 12
        input_lane_3 = 4
        input_selected = 13
        expected_ready = 1
        expected_valid = 1
        expected_pair_second = 18
        expected_lane_0 = 10
        expected_lane_1 = 14
        expected_lane_2 = 2
        expected_lane_3 = 7
        expected_selected = 14
    if phase == 42:
        take = 1
        input_pair_first = 2
        input_pair_second = 6
        input_lane_0 = 10
        input_lane_1 = 13
        input_lane_2 = 1
        input_lane_3 = 5
        input_selected = 2
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 1
        expected_pair_second = 25
        expected_lane_0 = 11
        expected_lane_1 = 15
        expected_lane_2 = 3
        expected_lane_3 = 8
        expected_selected = 15
    if phase == 43:
        take = 1
        input_pair_first = 3
        input_pair_second = 13
        input_lane_0 = 11
        input_lane_1 = 14
        input_lane_2 = 2
        input_lane_3 = 6
        input_selected = 7
        expected_ready = 1
        expected_valid = 1
        expected_pair_first = 2
        expected_lane_0 = 12
        expected_lane_2 = 4
        expected_lane_3 = 9
    if phase == 44:
        take = 1
        input_pair_first = 4
        input_pair_second = 20
        input_lane_0 = 12
        input_lane_1 = 15
        input_lane_2 = 3
        input_lane_3 = 7
        input_selected = 12
        expected_ready = 1
    if phase == 45:
        take = 1
        input_pair_first = 5
        input_pair_second = 27
        input_lane_0 = 13
        input_lane_2 = 4
        input_lane_3 = 8
        input_selected = 1
        expected_ready = 1
    if phase == 46:
        take = 1
        input_pair_first = 6
        input_pair_second = 2
        input_lane_0 = 14
        input_lane_1 = 1
        input_lane_2 = 5
        input_lane_3 = 9
        input_selected = 6
        expected_ready = 1
    if phase == 47:
        take = 1
        input_pair_first = 7
        input_pair_second = 9
        input_lane_0 = 15
        input_lane_1 = 2
        input_lane_2 = 6
        input_lane_3 = 10
        input_selected = 11
        expected_ready = 1
    return Stimulus(
        valid=valid,
        take=take,
        input_pair_first=input_pair_first,
        input_pair_second=input_pair_second,
        input_lane_0=input_lane_0,
        input_lane_1=input_lane_1,
        input_lane_2=input_lane_2,
        input_lane_3=input_lane_3,
        input_selected=input_selected,
        expected_ready=expected_ready,
        expected_valid=expected_valid,
        expected_pair_first=expected_pair_first,
        expected_pair_second=expected_pair_second,
        expected_lane_0=expected_lane_0,
        expected_lane_1=expected_lane_1,
        expected_lane_2=expected_lane_2,
        expected_lane_3=expected_lane_3,
        expected_selected=expected_selected,
    )


@rule
def advance(phase):
    if phase < 47:
        phase = phase + 1


@system
def AggregatePayloadPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = AggregatePacket(
        pair=Pair(first=frame.input_pair_first, second=frame.input_pair_second),
        lanes=(
            frame.input_lane_0,
            frame.input_lane_1,
            frame.input_lane_2,
            frame.input_lane_3,
        ),
        selected=frame.input_selected,
    )
    dut = AggregatePayloadPipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert (
            dut.ready == frame.expected_ready
        ), "aggregate_payload_pipeline ready old-state check"
        assert (
            dut.valid == frame.expected_valid
        ), "aggregate_payload_pipeline valid old-state check"
        assert (
            dut.data.pair.first == frame.expected_pair_first
        ), "aggregate_payload_pipeline pair_first old-state check"
        assert (
            dut.data.pair.second == frame.expected_pair_second
        ), "aggregate_payload_pipeline pair_second old-state check"
        assert (
            dut.data.lanes[0] == frame.expected_lane_0
        ), "aggregate_payload_pipeline lane_0 old-state check"
        assert (
            dut.data.lanes[1] == frame.expected_lane_1
        ), "aggregate_payload_pipeline lane_1 old-state check"
        assert (
            dut.data.lanes[2] == frame.expected_lane_2
        ), "aggregate_payload_pipeline lane_2 old-state check"
        assert (
            dut.data.lanes[3] == frame.expected_lane_3
        ), "aggregate_payload_pipeline lane_3 old-state check"
        assert (
            dut.data.selected == frame.expected_selected
        ), "aggregate_payload_pipeline selected old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "pair_first", dut.data.pair.first)
        log("info", "pair_second", dut.data.pair.second)
        log("info", "lane_0", dut.data.lanes[0])
        log("info", "lane_1", dut.data.lanes[1])
        log("info", "lane_2", dut.data.lanes[2])
        log("info", "lane_3", dut.data.lanes[3])
        log("info", "selected", dut.data.selected)

    advance(phase)
    check()

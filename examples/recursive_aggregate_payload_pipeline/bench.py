"""Literal independent history for nested nominal records and an Enum Table."""

from example_recursive_aggregate_payload_pipeline.recursive_aggregate_payload_pipeline import (
    Header,
    Mode,
    Nested,
    Packet,
    RecursiveAggregatePayloadPipeline,
    Tagged,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_tagged_mode: bits[1]
    input_tagged_value: bits[3]
    input_header_code: bits[3]
    input_nested_value: bits[3]
    input_mode_0: bits[1]
    input_mode_1: bits[1]
    input_flag: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_tagged_mode: bits[1]
    expected_tagged_value: bits[3]
    expected_header_code: bits[3]
    expected_nested_value: bits[3]
    expected_mode_0: bits[1]
    expected_mode_1: bits[1]
    expected_flag: bits[1]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    take: bits[1] = 0
    input_tagged_mode: bits[1] = 0
    input_tagged_value: bits[3] = 0
    input_header_code: bits[3] = 0
    input_nested_value: bits[3] = 0
    input_mode_0: bits[1] = 0
    input_mode_1: bits[1] = 0
    input_flag: bits[1] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_tagged_mode: bits[1] = 0
    expected_tagged_value: bits[3] = 0
    expected_header_code: bits[3] = 0
    expected_nested_value: bits[3] = 0
    expected_mode_0: bits[1] = 0
    expected_mode_1: bits[1] = 0
    expected_flag: bits[1] = 0
    if phase == 0:
        valid = 1
        take = 1
        input_tagged_value = 5
        input_header_code = 6
        input_nested_value = 2
        input_mode_1 = 1
        expected_ready = 1
    if phase == 1:
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 3
        input_header_code = 1
        input_nested_value = 5
        input_mode_1 = 1
        expected_ready = 1
    if phase == 2:
        take = 1
        input_tagged_value = 6
        input_header_code = 2
        input_nested_value = 2
        input_mode_0 = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 5
        expected_header_code = 7
        expected_nested_value = 2
        expected_mode_0 = 1
        expected_flag = 1
    if phase == 3:
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 1
        input_header_code = 3
        input_nested_value = 7
        input_mode_0 = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
    if phase == 4:
        take = 1
        input_tagged_value = 4
        input_header_code = 4
        input_nested_value = 4
        input_flag = 1
        expected_ready = 1
    if phase == 5:
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 7
        input_header_code = 5
        input_nested_value = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
    if phase == 6:
        take = 1
        input_tagged_value = 2
        input_header_code = 6
        input_nested_value = 6
        input_mode_0 = 1
        expected_ready = 1
    if phase == 7:
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 5
        input_header_code = 7
        input_nested_value = 3
        input_mode_0 = 1
        input_mode_1 = 1
        expected_ready = 1
    if phase == 8:
        take = 1
        expected_ready = 1
    if phase == 9:
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 3
        input_header_code = 1
        input_nested_value = 5
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
    if phase == 10:
        valid = 1
        take = 1
        input_tagged_value = 5
        input_header_code = 6
        input_nested_value = 2
        input_mode_1 = 1
        expected_ready = 1
    if phase == 11:
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 1
        input_header_code = 3
        input_nested_value = 7
        input_mode_0 = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
    if phase == 12:
        take = 1
        input_tagged_value = 4
        input_header_code = 4
        input_nested_value = 4
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 5
        expected_header_code = 7
        expected_nested_value = 2
        expected_mode_0 = 1
        expected_flag = 1
    if phase == 13:
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 7
        input_header_code = 5
        input_nested_value = 1
        input_mode_1 = 1
        expected_ready = 1
    if phase == 14:
        valid = 1
        input_tagged_value = 2
        input_header_code = 6
        input_nested_value = 6
        input_mode_0 = 1
        expected_ready = 1
    if phase == 15:
        valid = 1
        input_tagged_mode = 1
        input_tagged_value = 5
        input_header_code = 7
        input_nested_value = 3
        input_mode_0 = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
    if phase == 16:
        valid = 1
        input_flag = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 2
        expected_header_code = 7
        expected_nested_value = 6
        expected_mode_1 = 1
    if phase == 17:
        valid = 1
        input_tagged_mode = 1
        input_tagged_value = 3
        input_header_code = 1
        input_nested_value = 5
        input_mode_1 = 1
        input_flag = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 2
        expected_header_code = 7
        expected_nested_value = 6
        expected_mode_1 = 1
    if phase == 18:
        valid = 1
        input_tagged_value = 6
        input_header_code = 2
        input_nested_value = 2
        input_mode_0 = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 2
        expected_header_code = 7
        expected_nested_value = 6
        expected_mode_1 = 1
    if phase == 19:
        valid = 1
        input_tagged_mode = 1
        input_tagged_value = 1
        input_header_code = 3
        input_nested_value = 7
        input_mode_0 = 1
        input_mode_1 = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 2
        expected_header_code = 7
        expected_nested_value = 6
        expected_mode_1 = 1
    if phase == 20:
        valid = 1
        take = 1
        input_tagged_value = 4
        input_header_code = 4
        input_nested_value = 4
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 2
        expected_header_code = 7
        expected_nested_value = 6
        expected_mode_1 = 1
    if phase == 21:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 7
        input_header_code = 5
        input_nested_value = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 5
        expected_nested_value = 3
        expected_mode_0 = 1
        expected_mode_1 = 1
    if phase == 22:
        valid = 1
        take = 1
        input_tagged_value = 2
        input_header_code = 6
        input_nested_value = 6
        input_mode_0 = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 4
        expected_header_code = 5
        expected_nested_value = 4
        expected_flag = 1
    if phase == 23:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 5
        input_header_code = 7
        input_nested_value = 3
        input_mode_0 = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 7
        expected_header_code = 6
        expected_nested_value = 1
        expected_mode_0 = 1
        expected_flag = 1
    if phase == 24:
        valid = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 2
        expected_header_code = 7
        expected_nested_value = 6
        expected_mode_1 = 1
    if phase == 25:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 3
        input_header_code = 1
        input_nested_value = 5
        input_mode_1 = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 5
        expected_nested_value = 3
        expected_mode_0 = 1
        expected_mode_1 = 1
    if phase == 26:
        valid = 1
        take = 1
        input_tagged_value = 6
        input_header_code = 2
        input_nested_value = 2
        input_mode_0 = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_header_code = 1
        expected_flag = 1
    if phase == 27:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 1
        input_header_code = 3
        input_nested_value = 7
        input_mode_0 = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 3
        expected_header_code = 2
        expected_nested_value = 5
        expected_mode_0 = 1
        expected_flag = 1
    if phase == 28:
        valid = 1
        take = 1
        input_tagged_value = 4
        input_header_code = 4
        input_nested_value = 4
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 6
        expected_header_code = 3
        expected_nested_value = 2
        expected_mode_1 = 1
    if phase == 29:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 7
        input_header_code = 5
        input_nested_value = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 1
        expected_header_code = 4
        expected_nested_value = 7
        expected_mode_0 = 1
        expected_mode_1 = 1
    if phase == 30:
        valid = 1
        take = 1
        input_tagged_value = 2
        input_header_code = 6
        input_nested_value = 6
        input_mode_0 = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 4
        expected_header_code = 5
        expected_nested_value = 4
        expected_flag = 1
    if phase == 31:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 5
        input_header_code = 7
        input_nested_value = 3
        input_mode_0 = 1
        input_mode_1 = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 7
        expected_header_code = 6
        expected_nested_value = 1
        expected_mode_0 = 1
        expected_flag = 1
    if phase == 32:
        valid = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 2
        expected_header_code = 7
        expected_nested_value = 6
        expected_mode_1 = 1
    if phase == 33:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 3
        input_header_code = 1
        input_nested_value = 5
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 5
        expected_nested_value = 3
        expected_mode_0 = 1
        expected_mode_1 = 1
    if phase == 34:
        valid = 1
        take = 1
        input_tagged_value = 6
        input_header_code = 2
        input_nested_value = 2
        input_mode_0 = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_header_code = 1
        expected_flag = 1
    if phase == 35:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 1
        input_header_code = 3
        input_nested_value = 7
        input_mode_0 = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 3
        expected_header_code = 2
        expected_nested_value = 5
        expected_mode_0 = 1
        expected_flag = 1
    if phase == 36:
        valid = 1
        take = 1
        input_tagged_value = 4
        input_header_code = 4
        input_nested_value = 4
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 6
        expected_header_code = 3
        expected_nested_value = 2
        expected_mode_1 = 1
    if phase == 37:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 7
        input_header_code = 5
        input_nested_value = 1
        input_mode_1 = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 1
        expected_header_code = 4
        expected_nested_value = 7
        expected_mode_0 = 1
        expected_mode_1 = 1
    if phase == 38:
        valid = 1
        take = 1
        input_tagged_value = 2
        input_header_code = 6
        input_nested_value = 6
        input_mode_0 = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 4
        expected_header_code = 5
        expected_nested_value = 4
        expected_flag = 1
    if phase == 39:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 5
        input_header_code = 7
        input_nested_value = 3
        input_mode_0 = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 7
        expected_header_code = 6
        expected_nested_value = 1
        expected_mode_0 = 1
        expected_flag = 1
    if phase == 40:
        valid = 1
        take = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 2
        expected_header_code = 7
        expected_nested_value = 6
        expected_mode_1 = 1
    if phase == 41:
        valid = 1
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 3
        input_header_code = 1
        input_nested_value = 5
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 5
        expected_nested_value = 3
        expected_mode_0 = 1
        expected_mode_1 = 1
    if phase == 42:
        take = 1
        input_tagged_value = 6
        input_header_code = 2
        input_nested_value = 2
        input_mode_0 = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_header_code = 1
        expected_flag = 1
    if phase == 43:
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 1
        input_header_code = 3
        input_nested_value = 7
        input_mode_0 = 1
        input_mode_1 = 1
        expected_ready = 1
        expected_valid = 1
        expected_tagged_mode = 1
        expected_tagged_value = 3
        expected_header_code = 2
        expected_nested_value = 5
        expected_mode_0 = 1
        expected_flag = 1
    if phase == 44:
        take = 1
        input_tagged_value = 4
        input_header_code = 4
        input_nested_value = 4
        expected_ready = 1
    if phase == 45:
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 7
        input_header_code = 5
        input_nested_value = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
    if phase == 46:
        take = 1
        input_tagged_value = 2
        input_header_code = 6
        input_nested_value = 6
        input_mode_0 = 1
        input_flag = 1
        expected_ready = 1
    if phase == 47:
        take = 1
        input_tagged_mode = 1
        input_tagged_value = 5
        input_header_code = 7
        input_nested_value = 3
        input_mode_0 = 1
        input_mode_1 = 1
        input_flag = 1
        expected_ready = 1
    return Stimulus(
        valid=valid,
        take=take,
        input_tagged_mode=input_tagged_mode,
        input_tagged_value=input_tagged_value,
        input_header_code=input_header_code,
        input_nested_value=input_nested_value,
        input_mode_0=input_mode_0,
        input_mode_1=input_mode_1,
        input_flag=input_flag,
        expected_ready=expected_ready,
        expected_valid=expected_valid,
        expected_tagged_mode=expected_tagged_mode,
        expected_tagged_value=expected_tagged_value,
        expected_header_code=expected_header_code,
        expected_nested_value=expected_nested_value,
        expected_mode_0=expected_mode_0,
        expected_mode_1=expected_mode_1,
        expected_flag=expected_flag,
    )


@rule
def advance(phase):
    if phase < 47:
        phase = phase + 1


@system
def RecursiveAggregatePayloadPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Packet(
        tagged=Tagged(
            mode=Mode.RUN if frame.input_tagged_mode == 1 else Mode.IDLE,
            value=frame.input_tagged_value,
        ),
        nested=Nested(
            header=Header(code=frame.input_header_code),
            value=frame.input_nested_value,
        ),
        modes=(
            Mode.RUN if frame.input_mode_0 == 1 else Mode.IDLE,
            Mode.RUN if frame.input_mode_1 == 1 else Mode.IDLE,
        ),
        flag=frame.input_flag,
    )
    dut = RecursiveAggregatePayloadPipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert (
            dut.ready == frame.expected_ready
        ), "recursive_aggregate_payload_pipeline ready old-state check"
        assert (
            dut.valid == frame.expected_valid
        ), "recursive_aggregate_payload_pipeline valid old-state check"
        assert (
            enum_to_bits(dut.data.tagged.mode) == frame.expected_tagged_mode
        ), "recursive_aggregate_payload_pipeline tagged_mode old-state check"
        assert (
            dut.data.tagged.value == frame.expected_tagged_value
        ), "recursive_aggregate_payload_pipeline tagged_value old-state check"
        assert (
            dut.data.nested.header.code == frame.expected_header_code
        ), "recursive_aggregate_payload_pipeline header_code old-state check"
        assert (
            dut.data.nested.value == frame.expected_nested_value
        ), "recursive_aggregate_payload_pipeline nested_value old-state check"
        assert (
            enum_to_bits(dut.data.modes[0]) == frame.expected_mode_0
        ), "recursive_aggregate_payload_pipeline mode_0 old-state check"
        assert (
            enum_to_bits(dut.data.modes[1]) == frame.expected_mode_1
        ), "recursive_aggregate_payload_pipeline mode_1 old-state check"
        assert (
            dut.data.flag == frame.expected_flag
        ), "recursive_aggregate_payload_pipeline flag old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "tagged_mode", enum_to_bits(dut.data.tagged.mode))
        log("info", "tagged_value", dut.data.tagged.value)
        log("info", "header_code", dut.data.nested.header.code)
        log("info", "nested_value", dut.data.nested.value)
        log("info", "mode_0", enum_to_bits(dut.data.modes[0]))
        log("info", "mode_1", enum_to_bits(dut.data.modes[1]))
        log("info", "flag", dut.data.flag)

    advance(phase)
    check()

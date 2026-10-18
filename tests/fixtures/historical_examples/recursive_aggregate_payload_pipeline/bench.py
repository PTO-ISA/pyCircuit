"""Literal independent history for nested nominal records and an Enum Table."""

from example_recursive_aggregate_payload_pipeline.recursive_aggregate_payload_pipeline import (
    Header,
    Mode,
    Nested,
    Packet,
    RecursiveAggregatePayloadPipeline,
    Tagged,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system, table


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


@struct
class Stimuli:
    rows: table[49, Stimulus]


def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, take=1, input_tagged_value=5, input_header_code=6, input_nested_value=2, input_mode_1=1, expected_ready=1),
            Stimulus(take=1, input_tagged_mode=1, input_tagged_value=3, input_header_code=1, input_nested_value=5, input_mode_1=1, expected_ready=1),
            Stimulus(take=1, input_tagged_value=6, input_header_code=2, input_nested_value=2, input_mode_0=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=5, expected_header_code=7, expected_nested_value=2, expected_mode_0=1, expected_flag=1),
            Stimulus(take=1, input_tagged_mode=1, input_tagged_value=1, input_header_code=3, input_nested_value=7, input_mode_0=1, input_mode_1=1, input_flag=1, expected_ready=1),
            Stimulus(take=1, input_tagged_value=4, input_header_code=4, input_nested_value=4, input_flag=1, expected_ready=1),
            Stimulus(take=1, input_tagged_mode=1, input_tagged_value=7, input_header_code=5, input_nested_value=1, input_mode_1=1, input_flag=1, expected_ready=1),
            Stimulus(take=1, input_tagged_value=2, input_header_code=6, input_nested_value=6, input_mode_0=1, expected_ready=1),
            Stimulus(take=1, input_tagged_mode=1, input_tagged_value=5, input_header_code=7, input_nested_value=3, input_mode_0=1, input_mode_1=1, expected_ready=1),
            Stimulus(take=1, expected_ready=1),
            Stimulus(take=1, input_tagged_mode=1, input_tagged_value=3, input_header_code=1, input_nested_value=5, input_mode_1=1, input_flag=1, expected_ready=1),
            Stimulus(valid=1, take=1, input_tagged_value=5, input_header_code=6, input_nested_value=2, input_mode_1=1, expected_ready=1),
            Stimulus(take=1, input_tagged_mode=1, input_tagged_value=1, input_header_code=3, input_nested_value=7, input_mode_0=1, input_mode_1=1, input_flag=1, expected_ready=1),
            Stimulus(take=1, input_tagged_value=4, input_header_code=4, input_nested_value=4, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=5, expected_header_code=7, expected_nested_value=2, expected_mode_0=1, expected_flag=1),
            Stimulus(take=1, input_tagged_mode=1, input_tagged_value=7, input_header_code=5, input_nested_value=1, input_mode_1=1, expected_ready=1),
            Stimulus(valid=1, input_tagged_value=2, input_header_code=6, input_nested_value=6, input_mode_0=1, expected_ready=1),
            Stimulus(valid=1, input_tagged_mode=1, input_tagged_value=5, input_header_code=7, input_nested_value=3, input_mode_0=1, input_mode_1=1, input_flag=1, expected_ready=1),
            Stimulus(valid=1, input_flag=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=2, expected_header_code=7, expected_nested_value=6, expected_mode_1=1),
            Stimulus(valid=1, input_tagged_mode=1, input_tagged_value=3, input_header_code=1, input_nested_value=5, input_mode_1=1, input_flag=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=2, expected_header_code=7, expected_nested_value=6, expected_mode_1=1),
            Stimulus(valid=1, input_tagged_value=6, input_header_code=2, input_nested_value=2, input_mode_0=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=2, expected_header_code=7, expected_nested_value=6, expected_mode_1=1),
            Stimulus(valid=1, input_tagged_mode=1, input_tagged_value=1, input_header_code=3, input_nested_value=7, input_mode_0=1, input_mode_1=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=2, expected_header_code=7, expected_nested_value=6, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_value=4, input_header_code=4, input_nested_value=4, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=2, expected_header_code=7, expected_nested_value=6, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=7, input_header_code=5, input_nested_value=1, input_mode_1=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=5, expected_nested_value=3, expected_mode_0=1, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_value=2, input_header_code=6, input_nested_value=6, input_mode_0=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=4, expected_header_code=5, expected_nested_value=4, expected_flag=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=5, input_header_code=7, input_nested_value=3, input_mode_0=1, input_mode_1=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=7, expected_header_code=6, expected_nested_value=1, expected_mode_0=1, expected_flag=1),
            Stimulus(valid=1, take=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=2, expected_header_code=7, expected_nested_value=6, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=3, input_header_code=1, input_nested_value=5, input_mode_1=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=5, expected_nested_value=3, expected_mode_0=1, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_value=6, input_header_code=2, input_nested_value=2, input_mode_0=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_header_code=1, expected_flag=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=1, input_header_code=3, input_nested_value=7, input_mode_0=1, input_mode_1=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=3, expected_header_code=2, expected_nested_value=5, expected_mode_0=1, expected_flag=1),
            Stimulus(valid=1, take=1, input_tagged_value=4, input_header_code=4, input_nested_value=4, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=6, expected_header_code=3, expected_nested_value=2, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=7, input_header_code=5, input_nested_value=1, input_mode_1=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=1, expected_header_code=4, expected_nested_value=7, expected_mode_0=1, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_value=2, input_header_code=6, input_nested_value=6, input_mode_0=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=4, expected_header_code=5, expected_nested_value=4, expected_flag=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=5, input_header_code=7, input_nested_value=3, input_mode_0=1, input_mode_1=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=7, expected_header_code=6, expected_nested_value=1, expected_mode_0=1, expected_flag=1),
            Stimulus(valid=1, take=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=2, expected_header_code=7, expected_nested_value=6, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=3, input_header_code=1, input_nested_value=5, input_mode_1=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=5, expected_nested_value=3, expected_mode_0=1, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_value=6, input_header_code=2, input_nested_value=2, input_mode_0=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_header_code=1, expected_flag=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=1, input_header_code=3, input_nested_value=7, input_mode_0=1, input_mode_1=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=3, expected_header_code=2, expected_nested_value=5, expected_mode_0=1, expected_flag=1),
            Stimulus(valid=1, take=1, input_tagged_value=4, input_header_code=4, input_nested_value=4, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=6, expected_header_code=3, expected_nested_value=2, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=7, input_header_code=5, input_nested_value=1, input_mode_1=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=1, expected_header_code=4, expected_nested_value=7, expected_mode_0=1, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_value=2, input_header_code=6, input_nested_value=6, input_mode_0=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=4, expected_header_code=5, expected_nested_value=4, expected_flag=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=5, input_header_code=7, input_nested_value=3, input_mode_0=1, input_mode_1=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=7, expected_header_code=6, expected_nested_value=1, expected_mode_0=1, expected_flag=1),
            Stimulus(valid=1, take=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=2, expected_header_code=7, expected_nested_value=6, expected_mode_1=1),
            Stimulus(valid=1, take=1, input_tagged_mode=1, input_tagged_value=3, input_header_code=1, input_nested_value=5, input_mode_1=1, input_flag=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=5, expected_nested_value=3, expected_mode_0=1, expected_mode_1=1),
            Stimulus(take=1, input_tagged_value=6, input_header_code=2, input_nested_value=2, input_mode_0=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_header_code=1, expected_flag=1),
            Stimulus(take=1, input_tagged_mode=1, input_tagged_value=1, input_header_code=3, input_nested_value=7, input_mode_0=1, input_mode_1=1, expected_ready=1, expected_valid=1, expected_tagged_mode=1, expected_tagged_value=3, expected_header_code=2, expected_nested_value=5, expected_mode_0=1, expected_flag=1),
            Stimulus(take=1, input_tagged_value=4, input_header_code=4, input_nested_value=4, expected_ready=1),
            Stimulus(take=1, input_tagged_mode=1, input_tagged_value=7, input_header_code=5, input_nested_value=1, input_mode_1=1, input_flag=1, expected_ready=1),
            Stimulus(take=1, input_tagged_value=2, input_header_code=6, input_nested_value=6, input_mode_0=1, input_flag=1, expected_ready=1),
            Stimulus(take=1, input_tagged_mode=1, input_tagged_value=5, input_header_code=7, input_nested_value=3, input_mode_0=1, input_mode_1=1, input_flag=1, expected_ready=1),
            Stimulus(),
        ),
    )
    # fmt: on
    position = phase if phase < 48 else 48
    return stimuli.rows[position]


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
        assert dut.ready == frame.expected_ready, (
            "recursive_aggregate_payload_pipeline ready old-state check"
        )
        assert dut.valid == frame.expected_valid, (
            "recursive_aggregate_payload_pipeline valid old-state check"
        )
        assert enum_to_bits(dut.data.tagged.mode) == frame.expected_tagged_mode, (
            "recursive_aggregate_payload_pipeline tagged_mode old-state check"
        )
        assert dut.data.tagged.value == frame.expected_tagged_value, (
            "recursive_aggregate_payload_pipeline tagged_value old-state check"
        )
        assert dut.data.nested.header.code == frame.expected_header_code, (
            "recursive_aggregate_payload_pipeline header_code old-state check"
        )
        assert dut.data.nested.value == frame.expected_nested_value, (
            "recursive_aggregate_payload_pipeline nested_value old-state check"
        )
        assert enum_to_bits(dut.data.modes[0]) == frame.expected_mode_0, (
            "recursive_aggregate_payload_pipeline mode_0 old-state check"
        )
        assert enum_to_bits(dut.data.modes[1]) == frame.expected_mode_1, (
            "recursive_aggregate_payload_pipeline mode_1 old-state check"
        )
        assert dut.data.flag == frame.expected_flag, (
            "recursive_aggregate_payload_pipeline flag old-state check"
        )
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

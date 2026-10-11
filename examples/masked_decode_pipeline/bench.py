"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_masked_decode_pipeline.masked_decode_pipeline import (
    MaskedDecodePipeline,
    Instruction,
    Result,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_opcode: bits[4]
    input_is_compute: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_opcode: bits[4]
    expected_data_is_compute: bits[1]


@struct
class Stimuli:
    rows: table[185, Stimulus]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, input_is_compute=1, expected_ready=1),
            Stimulus(valid=1, input_opcode=1, expected_ready=1),
            Stimulus(valid=1, input_opcode=2, input_is_compute=1, expected_valid=1),
            Stimulus(valid=1, input_opcode=3, expected_valid=1),
            Stimulus(valid=1, input_opcode=4, input_is_compute=1, expected_valid=1),
            Stimulus(valid=1, input_opcode=5, expected_valid=1),
            Stimulus(valid=1, input_opcode=6, input_is_compute=1, expected_valid=1),
            Stimulus(valid=1, input_opcode=7, expected_valid=1),
            Stimulus(valid=1, input_opcode=8, input_is_compute=1, expected_valid=1),
            Stimulus(valid=1, input_opcode=9, expected_valid=1),
            Stimulus(valid=1, input_opcode=10, input_is_compute=1, expected_valid=1),
            Stimulus(valid=1, input_opcode=11, expected_valid=1),
            Stimulus(valid=1, input_opcode=12, input_is_compute=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_opcode=13, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_opcode=14, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=1),
            Stimulus(valid=1, take=1, input_opcode=15, expected_ready=1, expected_valid=1, expected_data_opcode=13),
            Stimulus(valid=1, input_is_compute=1, expected_valid=1, expected_data_opcode=14, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=1, expected_ready=1, expected_valid=1, expected_data_opcode=14, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=2, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, take=1, input_opcode=3, expected_ready=1, expected_valid=1, expected_data_opcode=1),
            Stimulus(valid=1, input_opcode=4, input_is_compute=1, expected_valid=1, expected_data_opcode=2),
            Stimulus(valid=1, take=1, input_opcode=5, expected_ready=1, expected_valid=1, expected_data_opcode=2),
            Stimulus(valid=1, take=1, input_opcode=6, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=3),
            Stimulus(valid=1, take=1, input_opcode=7, expected_ready=1, expected_valid=1, expected_data_opcode=5),
            Stimulus(valid=1, input_opcode=8, input_is_compute=1, expected_valid=1, expected_data_opcode=6),
            Stimulus(valid=1, take=1, input_opcode=9, expected_ready=1, expected_valid=1, expected_data_opcode=6),
            Stimulus(valid=1, take=1, input_opcode=10, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=7),
            Stimulus(valid=1, take=1, input_opcode=11, expected_ready=1, expected_valid=1, expected_data_opcode=9),
            Stimulus(valid=1, input_opcode=12, input_is_compute=1, expected_valid=1, expected_data_opcode=10, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=13, expected_ready=1, expected_valid=1, expected_data_opcode=10, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=14, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=11),
            Stimulus(take=1, input_opcode=15, expected_ready=1, expected_valid=1, expected_data_opcode=13),
            Stimulus(valid=1, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=14, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=1, expected_ready=1, expected_valid=1, expected_data_opcode=14, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=2, input_is_compute=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_opcode=3, expected_ready=1, expected_valid=1, expected_data_opcode=1),
            Stimulus(input_opcode=4, input_is_compute=1, expected_valid=1, expected_data_opcode=2),
            Stimulus(valid=1, take=1, input_opcode=5, expected_ready=1, expected_valid=1, expected_data_opcode=2),
            Stimulus(valid=1, take=1, input_opcode=6, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=3),
            Stimulus(valid=1, take=1, input_opcode=7, expected_ready=1, expected_valid=1, expected_data_opcode=5),
            Stimulus(valid=1, input_opcode=8, input_is_compute=1, expected_valid=1, expected_data_opcode=6),
            Stimulus(take=1, input_opcode=9, expected_ready=1, expected_valid=1, expected_data_opcode=6),
            Stimulus(valid=1, take=1, input_opcode=10, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=7),
            Stimulus(valid=1, take=1, input_opcode=11, expected_ready=1),
            Stimulus(valid=1, input_opcode=12, input_is_compute=1, expected_valid=1, expected_data_opcode=10, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=13, expected_ready=1, expected_valid=1, expected_data_opcode=10, expected_data_is_compute=1),
            Stimulus(take=1, input_opcode=14, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=11),
            Stimulus(valid=1, take=1, input_opcode=15, expected_ready=1, expected_valid=1, expected_data_opcode=13),
            Stimulus(valid=1, input_is_compute=1, expected_ready=1),
            Stimulus(valid=1, input_opcode=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=2, input_is_compute=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(input_opcode=3, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=4, input_is_compute=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=5, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=6, input_is_compute=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=7, expected_valid=1, expected_data_opcode=15),
            Stimulus(input_opcode=8, input_is_compute=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=9, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=10, input_is_compute=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=11, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=12, input_is_compute=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(input_opcode=13, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=14, input_is_compute=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, input_opcode=15, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, take=1, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, take=1, input_opcode=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_opcode=2, input_is_compute=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_opcode=3, expected_ready=1, expected_valid=1, expected_data_opcode=1),
            Stimulus(valid=1, take=1, input_opcode=4, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=2),
            Stimulus(valid=1, take=1, input_opcode=5, expected_ready=1, expected_valid=1, expected_data_opcode=3),
            Stimulus(valid=1, take=1, input_opcode=6, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=4),
            Stimulus(valid=1, take=1, input_opcode=7, expected_ready=1, expected_valid=1, expected_data_opcode=5),
            Stimulus(valid=1, take=1, input_opcode=8, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=6),
            Stimulus(valid=1, take=1, input_opcode=9, expected_ready=1, expected_valid=1, expected_data_opcode=7),
            Stimulus(valid=1, take=1, input_opcode=10, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=11, expected_ready=1, expected_valid=1, expected_data_opcode=9),
            Stimulus(valid=1, take=1, input_opcode=12, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=10, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=13, expected_ready=1, expected_valid=1, expected_data_opcode=11),
            Stimulus(valid=1, take=1, input_opcode=14, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=12, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=15, expected_ready=1, expected_valid=1, expected_data_opcode=13),
            Stimulus(valid=1, take=1, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=14, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=1, expected_ready=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, take=1, input_opcode=2, input_is_compute=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_opcode=3, expected_ready=1, expected_valid=1, expected_data_opcode=1),
            Stimulus(valid=1, take=1, input_opcode=4, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=2),
            Stimulus(valid=1, take=1, input_opcode=5, expected_ready=1, expected_valid=1, expected_data_opcode=3),
            Stimulus(valid=1, take=1, input_opcode=6, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=4),
            Stimulus(valid=1, take=1, input_opcode=7, expected_ready=1, expected_valid=1, expected_data_opcode=5),
            Stimulus(valid=1, take=1, input_opcode=8, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=6),
            Stimulus(valid=1, take=1, input_opcode=9, expected_ready=1, expected_valid=1, expected_data_opcode=7),
            Stimulus(valid=1, take=1, input_opcode=10, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=11, expected_ready=1, expected_valid=1, expected_data_opcode=9),
            Stimulus(valid=1, take=1, input_opcode=12, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=10, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=13, expected_ready=1, expected_valid=1, expected_data_opcode=11),
            Stimulus(valid=1, take=1, input_opcode=14, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=12, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=15, expected_ready=1, expected_valid=1, expected_data_opcode=13),
            Stimulus(valid=1, take=1, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=14, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=1, expected_ready=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, take=1, input_opcode=2, input_is_compute=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_opcode=3, expected_ready=1, expected_valid=1, expected_data_opcode=1),
            Stimulus(valid=1, take=1, input_opcode=4, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=2),
            Stimulus(valid=1, take=1, input_opcode=5, expected_ready=1, expected_valid=1, expected_data_opcode=3),
            Stimulus(valid=1, take=1, input_opcode=6, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=4),
            Stimulus(valid=1, take=1, input_opcode=7, expected_ready=1, expected_valid=1, expected_data_opcode=5),
            Stimulus(valid=1, take=1, input_opcode=8, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=6),
            Stimulus(valid=1, take=1, input_opcode=9, expected_ready=1, expected_valid=1, expected_data_opcode=7),
            Stimulus(valid=1, take=1, input_opcode=10, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=11, expected_ready=1, expected_valid=1, expected_data_opcode=9),
            Stimulus(valid=1, take=1, input_opcode=12, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=10, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=13, expected_ready=1, expected_valid=1, expected_data_opcode=11),
            Stimulus(valid=1, take=1, input_opcode=14, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=12, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=15, expected_ready=1, expected_valid=1, expected_data_opcode=13),
            Stimulus(valid=1, take=1, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=14, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=1, expected_ready=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, take=1, input_opcode=2, input_is_compute=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_opcode=3, expected_ready=1, expected_valid=1, expected_data_opcode=1),
            Stimulus(valid=1, take=1, input_opcode=4, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=2),
            Stimulus(valid=1, take=1, input_opcode=5, expected_ready=1, expected_valid=1, expected_data_opcode=3),
            Stimulus(valid=1, take=1, input_opcode=6, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=4),
            Stimulus(valid=1, take=1, input_opcode=7, expected_ready=1, expected_valid=1, expected_data_opcode=5),
            Stimulus(valid=1, take=1, input_opcode=8, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=6),
            Stimulus(valid=1, take=1, input_opcode=9, expected_ready=1, expected_valid=1, expected_data_opcode=7),
            Stimulus(valid=1, take=1, input_opcode=10, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=11, expected_ready=1, expected_valid=1, expected_data_opcode=9),
            Stimulus(valid=1, take=1, input_opcode=12, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=10, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=13, expected_ready=1, expected_valid=1, expected_data_opcode=11),
            Stimulus(valid=1, take=1, input_opcode=14, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=12, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=15, expected_ready=1, expected_valid=1, expected_data_opcode=13),
            Stimulus(valid=1, take=1, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=14, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=1, expected_ready=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, take=1, input_opcode=2, input_is_compute=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_opcode=3, expected_ready=1, expected_valid=1, expected_data_opcode=1),
            Stimulus(valid=1, take=1, input_opcode=4, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=2),
            Stimulus(valid=1, take=1, input_opcode=5, expected_ready=1, expected_valid=1, expected_data_opcode=3),
            Stimulus(valid=1, take=1, input_opcode=6, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=4),
            Stimulus(valid=1, take=1, input_opcode=7, expected_ready=1, expected_valid=1, expected_data_opcode=5),
            Stimulus(valid=1, take=1, input_opcode=8, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=6),
            Stimulus(valid=1, take=1, input_opcode=9, expected_ready=1, expected_valid=1, expected_data_opcode=7),
            Stimulus(valid=1, take=1, input_opcode=10, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=11, expected_ready=1, expected_valid=1, expected_data_opcode=9),
            Stimulus(valid=1, take=1, input_opcode=12, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=10, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=13, expected_ready=1, expected_valid=1, expected_data_opcode=11),
            Stimulus(valid=1, take=1, input_opcode=14, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=12, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=15, expected_ready=1, expected_valid=1, expected_data_opcode=13),
            Stimulus(valid=1, take=1, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=14, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=1, expected_ready=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(valid=1, take=1, input_opcode=2, input_is_compute=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_opcode=3, expected_ready=1, expected_valid=1, expected_data_opcode=1),
            Stimulus(valid=1, take=1, input_opcode=4, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=2),
            Stimulus(valid=1, take=1, input_opcode=5, expected_ready=1, expected_valid=1, expected_data_opcode=3),
            Stimulus(valid=1, take=1, input_opcode=6, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=4),
            Stimulus(valid=1, take=1, input_opcode=7, expected_ready=1, expected_valid=1, expected_data_opcode=5),
            Stimulus(valid=1, take=1, input_opcode=8, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=6),
            Stimulus(valid=1, take=1, input_opcode=9, expected_ready=1, expected_valid=1, expected_data_opcode=7),
            Stimulus(valid=1, take=1, input_opcode=10, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=11, expected_ready=1, expected_valid=1, expected_data_opcode=9),
            Stimulus(valid=1, take=1, input_opcode=12, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=10, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=13, expected_ready=1, expected_valid=1, expected_data_opcode=11),
            Stimulus(valid=1, take=1, input_opcode=14, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=12, expected_data_is_compute=1),
            Stimulus(valid=1, take=1, input_opcode=15, expected_ready=1, expected_valid=1, expected_data_opcode=13),
            Stimulus(take=1, input_is_compute=1, expected_ready=1, expected_valid=1, expected_data_opcode=14, expected_data_is_compute=1),
            Stimulus(take=1, input_opcode=1, expected_ready=1, expected_valid=1, expected_data_opcode=15),
            Stimulus(take=1, input_opcode=2, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=3, expected_ready=1),
            Stimulus(take=1, input_opcode=4, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=5, expected_ready=1),
            Stimulus(take=1, input_opcode=6, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=7, expected_ready=1),
            Stimulus(take=1, input_opcode=8, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=9, expected_ready=1),
            Stimulus(take=1, input_opcode=10, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=11, expected_ready=1),
            Stimulus(take=1, input_opcode=12, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=13, expected_ready=1),
            Stimulus(take=1, input_opcode=14, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=15, expected_ready=1),
            Stimulus(take=1, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=1, expected_ready=1),
            Stimulus(take=1, input_opcode=2, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=3, expected_ready=1),
            Stimulus(take=1, input_opcode=4, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=5, expected_ready=1),
            Stimulus(take=1, input_opcode=6, input_is_compute=1, expected_ready=1),
            Stimulus(take=1, input_opcode=7, expected_ready=1),
            Stimulus(),
        ),
    )
    # fmt: on
    position = phase if phase < 184 else 184
    return stimuli.rows[position]


@rule
def advance(phase):
    if phase < 183:
        phase = phase + 1


@system
def MaskedDecodePipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Instruction(opcode=frame.input_opcode, is_compute=frame.input_is_compute)
    dut = MaskedDecodePipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert (
            dut.ready == frame.expected_ready
        ), "masked_decode_pipeline input capacity"
        assert (
            dut.valid == frame.expected_valid
        ), "masked_decode_pipeline result availability"
        assert (
            dut.data.opcode == frame.expected_data_opcode
        ), "masked_decode_pipeline opcode old-state check"
        assert (
            dut.data.is_compute == frame.expected_data_is_compute
        ), "masked_decode_pipeline is_compute old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "opcode", dut.data.opcode)
        log("info", "is_compute", dut.data.is_compute)

    advance(phase)
    check()

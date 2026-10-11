"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_bitfield_scalar_pipeline.bitfield_scalar_pipeline import (
    BitfieldScalarPipeline,
)
from pycircuit import bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    data: bits[32]
    take: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data: bits[32]


@struct
class Stimuli:
    rows: table[159, Stimulus]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, expected_ready=1),
            Stimulus(valid=1, data=4294967295, expected_ready=1),
            Stimulus(valid=1, data=4294967294, expected_valid=1),
            Stimulus(valid=1, data=2147483648, expected_valid=1),
            Stimulus(valid=1, data=2147483647, expected_valid=1),
            Stimulus(valid=1, data=2863311530, expected_valid=1),
            Stimulus(valid=1, data=1431655765, expected_valid=1),
            Stimulus(valid=1, data=1, expected_valid=1),
            Stimulus(valid=1, data=2, expected_valid=1),
            Stimulus(valid=1, data=4, expected_valid=1),
            Stimulus(valid=1, data=8, expected_valid=1),
            Stimulus(valid=1, data=16, expected_valid=1),
            Stimulus(valid=1, data=32, expected_valid=1),
            Stimulus(valid=1, data=64, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=128, take=1, expected_ready=1, expected_valid=1, expected_data=4294967295),
            Stimulus(valid=1, data=256, take=1, expected_ready=1, expected_valid=1, expected_data=2097152),
            Stimulus(valid=1, data=512, expected_valid=1, expected_data=4194304),
            Stimulus(valid=1, data=1024, take=1, expected_ready=1, expected_valid=1, expected_data=4194304),
            Stimulus(valid=1, data=2048, take=1, expected_ready=1, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=4096, take=1, expected_ready=1, expected_valid=1, expected_data=33554432),
            Stimulus(valid=1, data=8192, expected_valid=1, expected_data=67108864),
            Stimulus(valid=1, data=16384, take=1, expected_ready=1, expected_valid=1, expected_data=67108864),
            Stimulus(valid=1, data=32768, take=1, expected_ready=1, expected_valid=1, expected_data=134217728),
            Stimulus(valid=1, data=65536, take=1, expected_ready=1, expected_valid=1, expected_data=536870912),
            Stimulus(valid=1, data=131072, expected_valid=1, expected_data=1073741824),
            Stimulus(valid=1, data=262144, take=1, expected_ready=1, expected_valid=1, expected_data=1073741824),
            Stimulus(valid=1, data=524288, take=1, expected_ready=1, expected_valid=1, expected_data=2147483648),
            Stimulus(valid=1, data=1048576, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=2097152, expected_valid=1),
            Stimulus(valid=1, data=4194304, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=8388608, take=1, expected_ready=1, expected_valid=1, expected_data=8),
            Stimulus(data=16777216, take=1, expected_ready=1, expected_valid=1, expected_data=32),
            Stimulus(valid=1, data=33554432, expected_ready=1, expected_valid=1, expected_data=64),
            Stimulus(valid=1, data=67108864, take=1, expected_ready=1, expected_valid=1, expected_data=64),
            Stimulus(valid=1, data=134217728, take=1, expected_ready=1, expected_valid=1, expected_data=256),
            Stimulus(valid=1, data=268435456, take=1, expected_ready=1, expected_valid=1, expected_data=512),
            Stimulus(data=536870912, expected_valid=1, expected_data=1024),
            Stimulus(valid=1, data=1073741824, take=1, expected_ready=1, expected_valid=1, expected_data=1024),
            Stimulus(valid=1, data=2147483648, take=1, expected_ready=1, expected_valid=1, expected_data=2048),
            Stimulus(valid=1, data=1, take=1, expected_ready=1, expected_valid=1, expected_data=8192),
            Stimulus(valid=1, data=2, expected_valid=1, expected_data=16384),
            Stimulus(data=4, take=1, expected_ready=1, expected_valid=1, expected_data=16384),
            Stimulus(valid=1, data=8, take=1, expected_ready=1, expected_valid=1, expected_data=32769),
            Stimulus(valid=1, data=16, take=1, expected_ready=1),
            Stimulus(valid=1, data=32, expected_valid=1, expected_data=262144),
            Stimulus(valid=1, data=64, take=1, expected_ready=1, expected_valid=1, expected_data=262144),
            Stimulus(data=128, take=1, expected_ready=1, expected_valid=1, expected_data=524288),
            Stimulus(valid=1, data=256, take=1, expected_ready=1, expected_valid=1, expected_data=2097152),
            Stimulus(valid=1, data=512, expected_ready=1),
            Stimulus(valid=1, data=1024, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=2048, expected_valid=1, expected_data=8388608),
            Stimulus(data=4096, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=8192, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=16384, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=32768, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=65536, expected_valid=1, expected_data=8388608),
            Stimulus(data=131072, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=262144, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=524288, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=1048576, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=2097152, expected_valid=1, expected_data=8388608),
            Stimulus(data=4194304, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=8388608, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=16777216, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, take=1, expected_ready=1, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=4294967295, take=1, expected_ready=1, expected_valid=1, expected_data=16777216),
            Stimulus(valid=1, data=4294967294, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=2147483648, take=1, expected_ready=1, expected_valid=1, expected_data=4294967295),
            Stimulus(valid=1, data=2147483647, take=1, expected_ready=1, expected_valid=1, expected_data=4294934526),
            Stimulus(valid=1, data=2863311530, take=1, expected_ready=1, expected_valid=1, expected_data=16384),
            Stimulus(valid=1, data=1431655765, take=1, expected_ready=1, expected_valid=1, expected_data=4294950911),
            Stimulus(valid=1, data=1, take=1, expected_ready=1, expected_valid=1, expected_data=1431655762),
            Stimulus(valid=1, data=2, take=1, expected_ready=1, expected_valid=1, expected_data=2863311533),
            Stimulus(valid=1, data=4, take=1, expected_ready=1, expected_valid=1, expected_data=32769),
            Stimulus(valid=1, data=8, take=1, expected_ready=1, expected_valid=1, expected_data=65538),
            Stimulus(valid=1, data=16, take=1, expected_ready=1, expected_valid=1, expected_data=131076),
            Stimulus(valid=1, data=32, take=1, expected_ready=1, expected_valid=1, expected_data=262144),
            Stimulus(valid=1, data=64, take=1, expected_ready=1, expected_valid=1, expected_data=524288),
            Stimulus(valid=1, data=128, take=1, expected_ready=1, expected_valid=1, expected_data=1048576),
            Stimulus(valid=1, data=256, take=1, expected_ready=1, expected_valid=1, expected_data=2097152),
            Stimulus(valid=1, data=512, take=1, expected_ready=1, expected_valid=1, expected_data=4194304),
            Stimulus(valid=1, data=1024, take=1, expected_ready=1, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=2048, take=1, expected_ready=1, expected_valid=1, expected_data=16777216),
            Stimulus(valid=1, data=4096, take=1, expected_ready=1, expected_valid=1, expected_data=33554432),
            Stimulus(valid=1, data=8192, take=1, expected_ready=1, expected_valid=1, expected_data=67108864),
            Stimulus(valid=1, data=16384, take=1, expected_ready=1, expected_valid=1, expected_data=134217728),
            Stimulus(valid=1, data=32768, take=1, expected_ready=1, expected_valid=1, expected_data=268435456),
            Stimulus(valid=1, data=65536, take=1, expected_ready=1, expected_valid=1, expected_data=536870912),
            Stimulus(valid=1, data=131072, take=1, expected_ready=1, expected_valid=1, expected_data=1073741824),
            Stimulus(valid=1, data=262144, take=1, expected_ready=1, expected_valid=1, expected_data=2147483648),
            Stimulus(valid=1, data=524288, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=1048576, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=2097152, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=4194304, take=1, expected_ready=1, expected_valid=1, expected_data=8),
            Stimulus(valid=1, data=8388608, take=1, expected_ready=1, expected_valid=1, expected_data=16),
            Stimulus(valid=1, data=16777216, take=1, expected_ready=1, expected_valid=1, expected_data=32),
            Stimulus(valid=1, data=33554432, take=1, expected_ready=1, expected_valid=1, expected_data=64),
            Stimulus(valid=1, data=67108864, take=1, expected_ready=1, expected_valid=1, expected_data=128),
            Stimulus(valid=1, data=134217728, take=1, expected_ready=1, expected_valid=1, expected_data=256),
            Stimulus(valid=1, data=268435456, take=1, expected_ready=1, expected_valid=1, expected_data=512),
            Stimulus(valid=1, data=536870912, take=1, expected_ready=1, expected_valid=1, expected_data=1024),
            Stimulus(valid=1, data=1073741824, take=1, expected_ready=1, expected_valid=1, expected_data=2048),
            Stimulus(valid=1, data=2147483648, take=1, expected_ready=1, expected_valid=1, expected_data=4096),
            Stimulus(valid=1, data=1, take=1, expected_ready=1, expected_valid=1, expected_data=8192),
            Stimulus(valid=1, data=2, take=1, expected_ready=1, expected_valid=1, expected_data=16384),
            Stimulus(valid=1, data=4, take=1, expected_ready=1, expected_valid=1, expected_data=32769),
            Stimulus(valid=1, data=8, take=1, expected_ready=1, expected_valid=1, expected_data=65538),
            Stimulus(valid=1, data=16, take=1, expected_ready=1, expected_valid=1, expected_data=131076),
            Stimulus(valid=1, data=32, take=1, expected_ready=1, expected_valid=1, expected_data=262144),
            Stimulus(valid=1, data=64, take=1, expected_ready=1, expected_valid=1, expected_data=524288),
            Stimulus(valid=1, data=128, take=1, expected_ready=1, expected_valid=1, expected_data=1048576),
            Stimulus(valid=1, data=256, take=1, expected_ready=1, expected_valid=1, expected_data=2097152),
            Stimulus(valid=1, data=512, take=1, expected_ready=1, expected_valid=1, expected_data=4194304),
            Stimulus(valid=1, data=1024, take=1, expected_ready=1, expected_valid=1, expected_data=8388608),
            Stimulus(valid=1, data=2048, take=1, expected_ready=1, expected_valid=1, expected_data=16777216),
            Stimulus(valid=1, data=4096, take=1, expected_ready=1, expected_valid=1, expected_data=33554432),
            Stimulus(valid=1, data=8192, take=1, expected_ready=1, expected_valid=1, expected_data=67108864),
            Stimulus(valid=1, data=16384, take=1, expected_ready=1, expected_valid=1, expected_data=134217728),
            Stimulus(valid=1, data=32768, take=1, expected_ready=1, expected_valid=1, expected_data=268435456),
            Stimulus(valid=1, data=65536, take=1, expected_ready=1, expected_valid=1, expected_data=536870912),
            Stimulus(valid=1, data=131072, take=1, expected_ready=1, expected_valid=1, expected_data=1073741824),
            Stimulus(valid=1, data=262144, take=1, expected_ready=1, expected_valid=1, expected_data=2147483648),
            Stimulus(valid=1, data=524288, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=1048576, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=2097152, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=4194304, take=1, expected_ready=1, expected_valid=1, expected_data=8),
            Stimulus(valid=1, data=8388608, take=1, expected_ready=1, expected_valid=1, expected_data=16),
            Stimulus(valid=1, data=16777216, take=1, expected_ready=1, expected_valid=1, expected_data=32),
            Stimulus(valid=1, data=33554432, take=1, expected_ready=1, expected_valid=1, expected_data=64),
            Stimulus(valid=1, data=67108864, take=1, expected_ready=1, expected_valid=1, expected_data=128),
            Stimulus(valid=1, data=134217728, take=1, expected_ready=1, expected_valid=1, expected_data=256),
            Stimulus(valid=1, data=268435456, take=1, expected_ready=1, expected_valid=1, expected_data=512),
            Stimulus(valid=1, data=536870912, take=1, expected_ready=1, expected_valid=1, expected_data=1024),
            Stimulus(valid=1, data=1073741824, take=1, expected_ready=1, expected_valid=1, expected_data=2048),
            Stimulus(data=2147483648, take=1, expected_ready=1, expected_valid=1, expected_data=4096),
            Stimulus(data=1, take=1, expected_ready=1, expected_valid=1, expected_data=8192),
            Stimulus(data=2, take=1, expected_ready=1),
            Stimulus(data=4, take=1, expected_ready=1),
            Stimulus(data=8, take=1, expected_ready=1),
            Stimulus(data=16, take=1, expected_ready=1),
            Stimulus(data=32, take=1, expected_ready=1),
            Stimulus(data=64, take=1, expected_ready=1),
            Stimulus(data=128, take=1, expected_ready=1),
            Stimulus(data=256, take=1, expected_ready=1),
            Stimulus(data=512, take=1, expected_ready=1),
            Stimulus(data=1024, take=1, expected_ready=1),
            Stimulus(data=2048, take=1, expected_ready=1),
            Stimulus(data=4096, take=1, expected_ready=1),
            Stimulus(data=8192, take=1, expected_ready=1),
            Stimulus(data=16384, take=1, expected_ready=1),
            Stimulus(data=32768, take=1, expected_ready=1),
            Stimulus(data=65536, take=1, expected_ready=1),
            Stimulus(data=131072, take=1, expected_ready=1),
            Stimulus(data=262144, take=1, expected_ready=1),
            Stimulus(data=524288, take=1, expected_ready=1),
            Stimulus(data=1048576, take=1, expected_ready=1),
            Stimulus(data=2097152, take=1, expected_ready=1),
            Stimulus(data=4194304, take=1, expected_ready=1),
            Stimulus(),
        ),
    )
    # fmt: on
    position = phase if phase < 158 else 158
    return stimuli.rows[position]


@rule
def advance(phase):
    if phase < 157:
        phase = phase + 1


@system
def BitfieldScalarPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = BitfieldScalarPipeline(frame.valid, frame.data, frame.take)

    @rule
    def exercise():
        assert (
            dut.ready == frame.expected_ready
        ), "bitfield_scalar_pipeline ready old-state check"
        assert (
            dut.valid == frame.expected_valid
        ), "bitfield_scalar_pipeline valid old-state check"
        assert (
            dut.data == frame.expected_data
        ), "bitfield_scalar_pipeline data old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "data", dut.data)

    advance(phase)
    exercise()

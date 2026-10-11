"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_recursive_pipeline.recursive_pipeline import RecursivePipeline
from pycircuit import bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    data: bits[64]
    take: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data: bits[64]


@struct
class Stimuli:
    rows: table[159, Stimulus]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, expected_ready=1),
            Stimulus(valid=1, data=18446744073709551615, expected_ready=1),
            Stimulus(valid=1, data=18446744073709551614, expected_ready=1),
            Stimulus(valid=1, data=9223372036854775808, expected_ready=1),
            Stimulus(valid=1, data=9223372036854775807, expected_ready=1, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=12297829382473034410, expected_ready=1, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=6148914691236517205, expected_ready=1, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=1, expected_ready=1, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=2, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=4, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=8, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=16, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=32, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=64, take=1, expected_ready=1, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=128, take=1, expected_ready=1, expected_valid=1, expected_data=2),
            Stimulus(valid=1, data=256, take=1, expected_ready=1, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=512, expected_valid=1, expected_data=9223372036854775811),
            Stimulus(valid=1, data=1024, take=1, expected_ready=1, expected_valid=1, expected_data=9223372036854775811),
            Stimulus(valid=1, data=2048, take=1, expected_ready=1, expected_valid=1, expected_data=9223372036854775810),
            Stimulus(valid=1, data=4096, take=1, expected_ready=1, expected_valid=1, expected_data=12297829382473034413),
            Stimulus(valid=1, data=8192, expected_valid=1, expected_data=6148914691236517208),
            Stimulus(valid=1, data=16384, take=1, expected_ready=1, expected_valid=1, expected_data=6148914691236517208),
            Stimulus(valid=1, data=32768, take=1, expected_ready=1, expected_valid=1, expected_data=4),
            Stimulus(valid=1, data=65536, take=1, expected_ready=1, expected_valid=1, expected_data=67),
            Stimulus(valid=1, data=131072, expected_valid=1, expected_data=131),
            Stimulus(valid=1, data=262144, take=1, expected_ready=1, expected_valid=1, expected_data=131),
            Stimulus(valid=1, data=524288, take=1, expected_ready=1, expected_valid=1, expected_data=259),
            Stimulus(valid=1, data=1048576, take=1, expected_ready=1, expected_valid=1, expected_data=1027),
            Stimulus(valid=1, data=2097152, expected_valid=1, expected_data=2051),
            Stimulus(valid=1, data=4194304, take=1, expected_ready=1, expected_valid=1, expected_data=2051),
            Stimulus(valid=1, data=8388608, take=1, expected_ready=1, expected_valid=1, expected_data=4099),
            Stimulus(data=16777216, take=1, expected_ready=1, expected_valid=1, expected_data=16387),
            Stimulus(valid=1, data=33554432, expected_ready=1, expected_valid=1, expected_data=32771),
            Stimulus(valid=1, data=67108864, take=1, expected_ready=1, expected_valid=1, expected_data=32771),
            Stimulus(valid=1, data=134217728, take=1, expected_ready=1, expected_valid=1, expected_data=65539),
            Stimulus(valid=1, data=268435456, take=1, expected_ready=1, expected_valid=1, expected_data=262147),
            Stimulus(data=536870912, expected_valid=1, expected_data=524291),
            Stimulus(valid=1, data=1073741824, take=1, expected_ready=1, expected_valid=1, expected_data=524291),
            Stimulus(valid=1, data=2147483648, take=1, expected_ready=1, expected_valid=1, expected_data=1048579),
            Stimulus(valid=1, data=4294967296, take=1, expected_ready=1, expected_valid=1, expected_data=4194307),
            Stimulus(valid=1, data=8589934592, expected_valid=1, expected_data=8388611),
            Stimulus(data=17179869184, take=1, expected_ready=1, expected_valid=1, expected_data=8388611),
            Stimulus(valid=1, data=34359738368, take=1, expected_ready=1, expected_valid=1, expected_data=33554435),
            Stimulus(valid=1, data=68719476736, take=1, expected_ready=1, expected_valid=1, expected_data=67108867),
            Stimulus(valid=1, data=137438953472, expected_ready=1, expected_valid=1, expected_data=134217731),
            Stimulus(valid=1, data=274877906944, take=1, expected_ready=1, expected_valid=1, expected_data=134217731),
            Stimulus(data=549755813888, take=1, expected_ready=1, expected_valid=1, expected_data=268435459),
            Stimulus(valid=1, data=1099511627776, take=1, expected_ready=1, expected_valid=1, expected_data=1073741827),
            Stimulus(valid=1, data=2199023255552, expected_ready=1, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=4398046511104, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=8796093022208, expected_valid=1, expected_data=2147483651),
            Stimulus(data=17592186044416, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=35184372088832, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=70368744177664, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=140737488355328, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=281474976710656, expected_valid=1, expected_data=2147483651),
            Stimulus(data=562949953421312, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=1125899906842624, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=2251799813685248, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=4503599627370496, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=9007199254740992, expected_valid=1, expected_data=2147483651),
            Stimulus(data=18014398509481984, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=36028797018963968, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=72057594037927936, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, take=1, expected_ready=1, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=18446744073709551615, take=1, expected_ready=1, expected_valid=1, expected_data=4294967299),
            Stimulus(valid=1, data=18446744073709551614, take=1, expected_ready=1, expected_valid=1, expected_data=34359738371),
            Stimulus(valid=1, data=9223372036854775808, take=1, expected_ready=1, expected_valid=1, expected_data=68719476739),
            Stimulus(valid=1, data=9223372036854775807, take=1, expected_ready=1, expected_valid=1, expected_data=137438953475),
            Stimulus(valid=1, data=12297829382473034410, take=1, expected_ready=1, expected_valid=1, expected_data=274877906947),
            Stimulus(valid=1, data=6148914691236517205, take=1, expected_ready=1, expected_valid=1, expected_data=1099511627779),
            Stimulus(valid=1, data=1, take=1, expected_ready=1, expected_valid=1, expected_data=2199023255555),
            Stimulus(valid=1, data=2, take=1, expected_ready=1, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=4, take=1, expected_ready=1, expected_valid=1, expected_data=2),
            Stimulus(valid=1, data=8, take=1, expected_ready=1, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=16, take=1, expected_ready=1, expected_valid=1, expected_data=9223372036854775811),
            Stimulus(valid=1, data=32, take=1, expected_ready=1, expected_valid=1, expected_data=9223372036854775810),
            Stimulus(valid=1, data=64, take=1, expected_ready=1, expected_valid=1, expected_data=12297829382473034413),
            Stimulus(valid=1, data=128, take=1, expected_ready=1, expected_valid=1, expected_data=6148914691236517208),
            Stimulus(valid=1, data=256, take=1, expected_ready=1, expected_valid=1, expected_data=4),
            Stimulus(valid=1, data=512, take=1, expected_ready=1, expected_valid=1, expected_data=5),
            Stimulus(valid=1, data=1024, take=1, expected_ready=1, expected_valid=1, expected_data=7),
            Stimulus(valid=1, data=2048, take=1, expected_ready=1, expected_valid=1, expected_data=11),
            Stimulus(valid=1, data=4096, take=1, expected_ready=1, expected_valid=1, expected_data=19),
            Stimulus(valid=1, data=8192, take=1, expected_ready=1, expected_valid=1, expected_data=35),
            Stimulus(valid=1, data=16384, take=1, expected_ready=1, expected_valid=1, expected_data=67),
            Stimulus(valid=1, data=32768, take=1, expected_ready=1, expected_valid=1, expected_data=131),
            Stimulus(valid=1, data=65536, take=1, expected_ready=1, expected_valid=1, expected_data=259),
            Stimulus(valid=1, data=131072, take=1, expected_ready=1, expected_valid=1, expected_data=515),
            Stimulus(valid=1, data=262144, take=1, expected_ready=1, expected_valid=1, expected_data=1027),
            Stimulus(valid=1, data=524288, take=1, expected_ready=1, expected_valid=1, expected_data=2051),
            Stimulus(valid=1, data=1048576, take=1, expected_ready=1, expected_valid=1, expected_data=4099),
            Stimulus(valid=1, data=2097152, take=1, expected_ready=1, expected_valid=1, expected_data=8195),
            Stimulus(valid=1, data=4194304, take=1, expected_ready=1, expected_valid=1, expected_data=16387),
            Stimulus(valid=1, data=8388608, take=1, expected_ready=1, expected_valid=1, expected_data=32771),
            Stimulus(valid=1, data=16777216, take=1, expected_ready=1, expected_valid=1, expected_data=65539),
            Stimulus(valid=1, data=33554432, take=1, expected_ready=1, expected_valid=1, expected_data=131075),
            Stimulus(valid=1, data=67108864, take=1, expected_ready=1, expected_valid=1, expected_data=262147),
            Stimulus(valid=1, data=134217728, take=1, expected_ready=1, expected_valid=1, expected_data=524291),
            Stimulus(valid=1, data=268435456, take=1, expected_ready=1, expected_valid=1, expected_data=1048579),
            Stimulus(valid=1, data=536870912, take=1, expected_ready=1, expected_valid=1, expected_data=2097155),
            Stimulus(valid=1, data=1073741824, take=1, expected_ready=1, expected_valid=1, expected_data=4194307),
            Stimulus(valid=1, data=2147483648, take=1, expected_ready=1, expected_valid=1, expected_data=8388611),
            Stimulus(valid=1, data=4294967296, take=1, expected_ready=1, expected_valid=1, expected_data=16777219),
            Stimulus(valid=1, data=8589934592, take=1, expected_ready=1, expected_valid=1, expected_data=33554435),
            Stimulus(valid=1, data=17179869184, take=1, expected_ready=1, expected_valid=1, expected_data=67108867),
            Stimulus(valid=1, data=34359738368, take=1, expected_ready=1, expected_valid=1, expected_data=134217731),
            Stimulus(valid=1, data=68719476736, take=1, expected_ready=1, expected_valid=1, expected_data=268435459),
            Stimulus(valid=1, data=137438953472, take=1, expected_ready=1, expected_valid=1, expected_data=536870915),
            Stimulus(valid=1, data=274877906944, take=1, expected_ready=1, expected_valid=1, expected_data=1073741827),
            Stimulus(valid=1, data=549755813888, take=1, expected_ready=1, expected_valid=1, expected_data=2147483651),
            Stimulus(valid=1, data=1099511627776, take=1, expected_ready=1, expected_valid=1, expected_data=4294967299),
            Stimulus(valid=1, data=2199023255552, take=1, expected_ready=1, expected_valid=1, expected_data=8589934595),
            Stimulus(valid=1, data=4398046511104, take=1, expected_ready=1, expected_valid=1, expected_data=17179869187),
            Stimulus(valid=1, data=8796093022208, take=1, expected_ready=1, expected_valid=1, expected_data=34359738371),
            Stimulus(valid=1, data=17592186044416, take=1, expected_ready=1, expected_valid=1, expected_data=68719476739),
            Stimulus(valid=1, data=35184372088832, take=1, expected_ready=1, expected_valid=1, expected_data=137438953475),
            Stimulus(valid=1, data=70368744177664, take=1, expected_ready=1, expected_valid=1, expected_data=274877906947),
            Stimulus(valid=1, data=140737488355328, take=1, expected_ready=1, expected_valid=1, expected_data=549755813891),
            Stimulus(valid=1, data=281474976710656, take=1, expected_ready=1, expected_valid=1, expected_data=1099511627779),
            Stimulus(valid=1, data=562949953421312, take=1, expected_ready=1, expected_valid=1, expected_data=2199023255555),
            Stimulus(valid=1, data=1125899906842624, take=1, expected_ready=1, expected_valid=1, expected_data=4398046511107),
            Stimulus(valid=1, data=2251799813685248, take=1, expected_ready=1, expected_valid=1, expected_data=8796093022211),
            Stimulus(valid=1, data=4503599627370496, take=1, expected_ready=1, expected_valid=1, expected_data=17592186044419),
            Stimulus(valid=1, data=9007199254740992, take=1, expected_ready=1, expected_valid=1, expected_data=35184372088835),
            Stimulus(valid=1, data=18014398509481984, take=1, expected_ready=1, expected_valid=1, expected_data=70368744177667),
            Stimulus(valid=1, data=36028797018963968, take=1, expected_ready=1, expected_valid=1, expected_data=140737488355331),
            Stimulus(valid=1, data=72057594037927936, take=1, expected_ready=1, expected_valid=1, expected_data=281474976710659),
            Stimulus(valid=1, data=144115188075855872, take=1, expected_ready=1, expected_valid=1, expected_data=562949953421315),
            Stimulus(valid=1, data=288230376151711744, take=1, expected_ready=1, expected_valid=1, expected_data=1125899906842627),
            Stimulus(valid=1, data=576460752303423488, take=1, expected_ready=1, expected_valid=1, expected_data=2251799813685251),
            Stimulus(valid=1, data=1152921504606846976, take=1, expected_ready=1, expected_valid=1, expected_data=4503599627370499),
            Stimulus(valid=1, data=2305843009213693952, take=1, expected_ready=1, expected_valid=1, expected_data=9007199254740995),
            Stimulus(valid=1, data=4611686018427387904, take=1, expected_ready=1, expected_valid=1, expected_data=18014398509481987),
            Stimulus(data=9223372036854775808, take=1, expected_ready=1, expected_valid=1, expected_data=36028797018963971),
            Stimulus(data=1, take=1, expected_ready=1, expected_valid=1, expected_data=72057594037927939),
            Stimulus(data=2, take=1, expected_ready=1, expected_valid=1, expected_data=144115188075855875),
            Stimulus(data=4, take=1, expected_ready=1, expected_valid=1, expected_data=288230376151711747),
            Stimulus(data=8, take=1, expected_ready=1, expected_valid=1, expected_data=576460752303423491),
            Stimulus(data=16, take=1, expected_ready=1, expected_valid=1, expected_data=1152921504606846979),
            Stimulus(data=32, take=1, expected_ready=1, expected_valid=1, expected_data=2305843009213693955),
            Stimulus(data=64, take=1, expected_ready=1, expected_valid=1, expected_data=4611686018427387907),
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
def RecursivePipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = RecursivePipeline(frame.valid, frame.data, frame.take)

    @rule
    def exercise():
        assert (
            dut.ready == frame.expected_ready
        ), "recursive_pipeline ready old-state check"
        assert (
            dut.valid == frame.expected_valid
        ), "recursive_pipeline valid old-state check"
        assert (
            dut.data == frame.expected_data
        ), "recursive_pipeline data old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "data", dut.data)

    advance(phase)
    exercise()

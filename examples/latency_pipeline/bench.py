"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_latency_pipeline.latency_pipeline import LatencyPipeline
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
            Stimulus(valid=1, data=9223372036854775807, expected_ready=1, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=12297829382473034410, expected_ready=1, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=6148914691236517205, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=1, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=2, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=4, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=8, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=16, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=32, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=64, take=1, expected_ready=1, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=128, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=256, take=1, expected_ready=1, expected_valid=1, expected_data=18446744073709551615),
            Stimulus(valid=1, data=512, expected_valid=1, expected_data=9223372036854775809),
            Stimulus(valid=1, data=1024, take=1, expected_ready=1, expected_valid=1, expected_data=9223372036854775809),
            Stimulus(valid=1, data=2048, take=1, expected_ready=1, expected_valid=1, expected_data=9223372036854775808),
            Stimulus(valid=1, data=4096, take=1, expected_ready=1, expected_valid=1, expected_data=12297829382473034411),
            Stimulus(valid=1, data=8192, expected_valid=1, expected_data=65),
            Stimulus(valid=1, data=16384, take=1, expected_ready=1, expected_valid=1, expected_data=65),
            Stimulus(valid=1, data=32768, take=1, expected_ready=1, expected_valid=1, expected_data=129),
            Stimulus(valid=1, data=65536, take=1, expected_ready=1, expected_valid=1, expected_data=257),
            Stimulus(valid=1, data=131072, expected_valid=1, expected_data=1025),
            Stimulus(valid=1, data=262144, take=1, expected_ready=1, expected_valid=1, expected_data=1025),
            Stimulus(valid=1, data=524288, take=1, expected_ready=1, expected_valid=1, expected_data=2049),
            Stimulus(valid=1, data=1048576, take=1, expected_ready=1, expected_valid=1, expected_data=4097),
            Stimulus(valid=1, data=2097152, expected_valid=1, expected_data=16385),
            Stimulus(valid=1, data=4194304, take=1, expected_ready=1, expected_valid=1, expected_data=16385),
            Stimulus(valid=1, data=8388608, take=1, expected_ready=1, expected_valid=1, expected_data=32769),
            Stimulus(data=16777216, take=1, expected_ready=1, expected_valid=1, expected_data=65537),
            Stimulus(valid=1, data=33554432, expected_ready=1, expected_valid=1, expected_data=262145),
            Stimulus(valid=1, data=67108864, take=1, expected_ready=1, expected_valid=1, expected_data=262145),
            Stimulus(valid=1, data=134217728, take=1, expected_ready=1, expected_valid=1, expected_data=524289),
            Stimulus(valid=1, data=268435456, take=1, expected_ready=1, expected_valid=1, expected_data=1048577),
            Stimulus(data=536870912, expected_valid=1, expected_data=4194305),
            Stimulus(valid=1, data=1073741824, take=1, expected_ready=1, expected_valid=1, expected_data=4194305),
            Stimulus(valid=1, data=2147483648, take=1, expected_ready=1, expected_valid=1, expected_data=8388609),
            Stimulus(valid=1, data=4294967296, take=1, expected_ready=1, expected_valid=1, expected_data=33554433),
            Stimulus(valid=1, data=8589934592, expected_valid=1, expected_data=67108865),
            Stimulus(data=17179869184, take=1, expected_ready=1, expected_valid=1, expected_data=67108865),
            Stimulus(valid=1, data=34359738368, take=1, expected_ready=1, expected_valid=1, expected_data=134217729),
            Stimulus(valid=1, data=68719476736, take=1, expected_ready=1, expected_valid=1, expected_data=268435457),
            Stimulus(valid=1, data=137438953472, expected_ready=1, expected_valid=1, expected_data=1073741825),
            Stimulus(valid=1, data=274877906944, take=1, expected_ready=1, expected_valid=1, expected_data=1073741825),
            Stimulus(data=549755813888, take=1, expected_ready=1, expected_valid=1, expected_data=2147483649),
            Stimulus(valid=1, data=1099511627776, take=1, expected_ready=1, expected_valid=1, expected_data=4294967297),
            Stimulus(valid=1, data=2199023255552, expected_ready=1, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=4398046511104, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=8796093022208, expected_valid=1, expected_data=34359738369),
            Stimulus(data=17592186044416, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=35184372088832, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=70368744177664, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=140737488355328, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=281474976710656, expected_valid=1, expected_data=34359738369),
            Stimulus(data=562949953421312, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=1125899906842624, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=2251799813685248, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=4503599627370496, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=9007199254740992, expected_valid=1, expected_data=34359738369),
            Stimulus(data=18014398509481984, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=36028797018963968, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=72057594037927936, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, take=1, expected_ready=1, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=18446744073709551615, take=1, expected_ready=1, expected_valid=1, expected_data=68719476737),
            Stimulus(valid=1, data=18446744073709551614, take=1, expected_ready=1, expected_valid=1, expected_data=137438953473),
            Stimulus(valid=1, data=9223372036854775808, take=1, expected_ready=1, expected_valid=1, expected_data=274877906945),
            Stimulus(valid=1, data=9223372036854775807, take=1, expected_ready=1, expected_valid=1, expected_data=1099511627777),
            Stimulus(valid=1, data=12297829382473034410, take=1, expected_ready=1, expected_valid=1, expected_data=2199023255553),
            Stimulus(valid=1, data=6148914691236517205, take=1, expected_ready=1, expected_valid=1, expected_data=1),
            Stimulus(valid=1, data=1, take=1, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, data=2, take=1, expected_ready=1, expected_valid=1, expected_data=18446744073709551615),
            Stimulus(valid=1, data=4, take=1, expected_ready=1, expected_valid=1, expected_data=9223372036854775809),
            Stimulus(valid=1, data=8, take=1, expected_ready=1, expected_valid=1, expected_data=9223372036854775808),
            Stimulus(valid=1, data=16, take=1, expected_ready=1, expected_valid=1, expected_data=12297829382473034411),
            Stimulus(valid=1, data=32, take=1, expected_ready=1, expected_valid=1, expected_data=6148914691236517206),
            Stimulus(valid=1, data=64, take=1, expected_ready=1, expected_valid=1, expected_data=2),
            Stimulus(valid=1, data=128, take=1, expected_ready=1, expected_valid=1, expected_data=3),
            Stimulus(valid=1, data=256, take=1, expected_ready=1, expected_valid=1, expected_data=5),
            Stimulus(valid=1, data=512, take=1, expected_ready=1, expected_valid=1, expected_data=9),
            Stimulus(valid=1, data=1024, take=1, expected_ready=1, expected_valid=1, expected_data=17),
            Stimulus(valid=1, data=2048, take=1, expected_ready=1, expected_valid=1, expected_data=33),
            Stimulus(valid=1, data=4096, take=1, expected_ready=1, expected_valid=1, expected_data=65),
            Stimulus(valid=1, data=8192, take=1, expected_ready=1, expected_valid=1, expected_data=129),
            Stimulus(valid=1, data=16384, take=1, expected_ready=1, expected_valid=1, expected_data=257),
            Stimulus(valid=1, data=32768, take=1, expected_ready=1, expected_valid=1, expected_data=513),
            Stimulus(valid=1, data=65536, take=1, expected_ready=1, expected_valid=1, expected_data=1025),
            Stimulus(valid=1, data=131072, take=1, expected_ready=1, expected_valid=1, expected_data=2049),
            Stimulus(valid=1, data=262144, take=1, expected_ready=1, expected_valid=1, expected_data=4097),
            Stimulus(valid=1, data=524288, take=1, expected_ready=1, expected_valid=1, expected_data=8193),
            Stimulus(valid=1, data=1048576, take=1, expected_ready=1, expected_valid=1, expected_data=16385),
            Stimulus(valid=1, data=2097152, take=1, expected_ready=1, expected_valid=1, expected_data=32769),
            Stimulus(valid=1, data=4194304, take=1, expected_ready=1, expected_valid=1, expected_data=65537),
            Stimulus(valid=1, data=8388608, take=1, expected_ready=1, expected_valid=1, expected_data=131073),
            Stimulus(valid=1, data=16777216, take=1, expected_ready=1, expected_valid=1, expected_data=262145),
            Stimulus(valid=1, data=33554432, take=1, expected_ready=1, expected_valid=1, expected_data=524289),
            Stimulus(valid=1, data=67108864, take=1, expected_ready=1, expected_valid=1, expected_data=1048577),
            Stimulus(valid=1, data=134217728, take=1, expected_ready=1, expected_valid=1, expected_data=2097153),
            Stimulus(valid=1, data=268435456, take=1, expected_ready=1, expected_valid=1, expected_data=4194305),
            Stimulus(valid=1, data=536870912, take=1, expected_ready=1, expected_valid=1, expected_data=8388609),
            Stimulus(valid=1, data=1073741824, take=1, expected_ready=1, expected_valid=1, expected_data=16777217),
            Stimulus(valid=1, data=2147483648, take=1, expected_ready=1, expected_valid=1, expected_data=33554433),
            Stimulus(valid=1, data=4294967296, take=1, expected_ready=1, expected_valid=1, expected_data=67108865),
            Stimulus(valid=1, data=8589934592, take=1, expected_ready=1, expected_valid=1, expected_data=134217729),
            Stimulus(valid=1, data=17179869184, take=1, expected_ready=1, expected_valid=1, expected_data=268435457),
            Stimulus(valid=1, data=34359738368, take=1, expected_ready=1, expected_valid=1, expected_data=536870913),
            Stimulus(valid=1, data=68719476736, take=1, expected_ready=1, expected_valid=1, expected_data=1073741825),
            Stimulus(valid=1, data=137438953472, take=1, expected_ready=1, expected_valid=1, expected_data=2147483649),
            Stimulus(valid=1, data=274877906944, take=1, expected_ready=1, expected_valid=1, expected_data=4294967297),
            Stimulus(valid=1, data=549755813888, take=1, expected_ready=1, expected_valid=1, expected_data=8589934593),
            Stimulus(valid=1, data=1099511627776, take=1, expected_ready=1, expected_valid=1, expected_data=17179869185),
            Stimulus(valid=1, data=2199023255552, take=1, expected_ready=1, expected_valid=1, expected_data=34359738369),
            Stimulus(valid=1, data=4398046511104, take=1, expected_ready=1, expected_valid=1, expected_data=68719476737),
            Stimulus(valid=1, data=8796093022208, take=1, expected_ready=1, expected_valid=1, expected_data=137438953473),
            Stimulus(valid=1, data=17592186044416, take=1, expected_ready=1, expected_valid=1, expected_data=274877906945),
            Stimulus(valid=1, data=35184372088832, take=1, expected_ready=1, expected_valid=1, expected_data=549755813889),
            Stimulus(valid=1, data=70368744177664, take=1, expected_ready=1, expected_valid=1, expected_data=1099511627777),
            Stimulus(valid=1, data=140737488355328, take=1, expected_ready=1, expected_valid=1, expected_data=2199023255553),
            Stimulus(valid=1, data=281474976710656, take=1, expected_ready=1, expected_valid=1, expected_data=4398046511105),
            Stimulus(valid=1, data=562949953421312, take=1, expected_ready=1, expected_valid=1, expected_data=8796093022209),
            Stimulus(valid=1, data=1125899906842624, take=1, expected_ready=1, expected_valid=1, expected_data=17592186044417),
            Stimulus(valid=1, data=2251799813685248, take=1, expected_ready=1, expected_valid=1, expected_data=35184372088833),
            Stimulus(valid=1, data=4503599627370496, take=1, expected_ready=1, expected_valid=1, expected_data=70368744177665),
            Stimulus(valid=1, data=9007199254740992, take=1, expected_ready=1, expected_valid=1, expected_data=140737488355329),
            Stimulus(valid=1, data=18014398509481984, take=1, expected_ready=1, expected_valid=1, expected_data=281474976710657),
            Stimulus(valid=1, data=36028797018963968, take=1, expected_ready=1, expected_valid=1, expected_data=562949953421313),
            Stimulus(valid=1, data=72057594037927936, take=1, expected_ready=1, expected_valid=1, expected_data=1125899906842625),
            Stimulus(valid=1, data=144115188075855872, take=1, expected_ready=1, expected_valid=1, expected_data=2251799813685249),
            Stimulus(valid=1, data=288230376151711744, take=1, expected_ready=1, expected_valid=1, expected_data=4503599627370497),
            Stimulus(valid=1, data=576460752303423488, take=1, expected_ready=1, expected_valid=1, expected_data=9007199254740993),
            Stimulus(valid=1, data=1152921504606846976, take=1, expected_ready=1, expected_valid=1, expected_data=18014398509481985),
            Stimulus(valid=1, data=2305843009213693952, take=1, expected_ready=1, expected_valid=1, expected_data=36028797018963969),
            Stimulus(valid=1, data=4611686018427387904, take=1, expected_ready=1, expected_valid=1, expected_data=72057594037927937),
            Stimulus(data=9223372036854775808, take=1, expected_ready=1, expected_valid=1, expected_data=144115188075855873),
            Stimulus(data=1, take=1, expected_ready=1, expected_valid=1, expected_data=288230376151711745),
            Stimulus(data=2, take=1, expected_ready=1, expected_valid=1, expected_data=576460752303423489),
            Stimulus(data=4, take=1, expected_ready=1, expected_valid=1, expected_data=1152921504606846977),
            Stimulus(data=8, take=1, expected_ready=1, expected_valid=1, expected_data=2305843009213693953),
            Stimulus(data=16, take=1, expected_ready=1, expected_valid=1, expected_data=4611686018427387905),
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
def LatencyPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = LatencyPipeline(frame.valid, frame.data, frame.take)

    @rule
    def exercise():
        assert (
            dut.ready == frame.expected_ready
        ), "latency_pipeline ready old-state check"
        assert (
            dut.valid == frame.expected_valid
        ), "latency_pipeline valid old-state check"
        assert dut.data == frame.expected_data, "latency_pipeline data old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "data", dut.data)

    advance(phase)
    exercise()

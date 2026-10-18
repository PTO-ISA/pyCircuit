"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_rule_pair_pipeline.rule_pair_pipeline import RulePairPipeline
from pycircuit import bits, log, rule, struct, system, table


@struct
class Stimulus:
    left_valid: bits[1]
    left_data: bits[64]
    left_take: bits[1]
    right_valid: bits[1]
    right_data: bits[64]
    right_take: bits[1]
    expected_left_ready: bits[1]
    expected_left_valid: bits[1]
    expected_left_data: bits[64]
    expected_right_ready: bits[1]
    expected_right_valid: bits[1]
    expected_right_data: bits[64]


@struct
class Stimuli:
    rows: table[159, Stimulus]


def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(left_valid=1, right_data=9223372036854775808, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=18446744073709551615, right_valid=1, right_data=9223372036854775807, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=18446744073709551614, right_valid=1, right_data=12297829382473034410, expected_left_valid=1, expected_left_data=1, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=9223372036854775808, right_data=6148914691236517205, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=9223372036854775807, right_valid=1, right_data=1, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=12297829382473034410, right_valid=1, right_data=2, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=6148914691236517205, right_data=4, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=1, right_valid=1, right_data=8, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=2, right_valid=1, right_data=16, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=4, right_data=32, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=8, right_valid=1, right_data=64, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=16, right_valid=1, right_data=128, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=32, right_data=256, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=64, left_take=1, right_valid=1, right_data=512, expected_left_ready=1, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=128, left_take=1, right_valid=1, right_data=1024, expected_left_ready=1, expected_left_valid=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=256, left_take=1, right_data=2048, expected_left_ready=1, expected_left_valid=1, expected_left_data=65, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=512, right_valid=1, right_data=4096, expected_left_valid=1, expected_left_data=129, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=1024, left_take=1, right_valid=1, right_data=8192, expected_left_ready=1, expected_left_valid=1, expected_left_data=129, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=2048, left_take=1, right_data=16384, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=257, expected_right_ready=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(left_valid=1, left_data=4096, left_take=1, right_valid=1, right_data=32768, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=1025, expected_right_ready=1, expected_right_valid=1, expected_right_data=6148914691236517204),
            Stimulus(left_valid=1, left_data=8192, right_valid=1, right_data=65536, expected_left_valid=1, expected_left_data=2049, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=16384, left_take=1, right_data=131072, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=2049, expected_right_ready=1, expected_right_valid=1, expected_right_data=65536),
            Stimulus(left_valid=1, left_data=32768, left_take=1, right_valid=1, right_data=262144, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=4097, expected_right_ready=1, expected_right_valid=1, expected_right_data=131072),
            Stimulus(left_valid=1, left_data=65536, left_take=1, right_valid=1, right_data=524288, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=16385, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=131072, right_data=1048576, right_take=1, expected_left_valid=1, expected_left_data=32769, expected_right_ready=1, expected_right_valid=1, expected_right_data=524288),
            Stimulus(left_valid=1, left_data=262144, left_take=1, right_valid=1, right_data=2097152, expected_left_ready=1, expected_left_valid=1, expected_left_data=32769, expected_right_ready=1, expected_right_valid=1, expected_right_data=1048576),
            Stimulus(left_valid=1, left_data=524288, left_take=1, right_valid=1, right_data=4194304, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=65537, expected_right_ready=1, expected_right_valid=1, expected_right_data=1048576),
            Stimulus(left_valid=1, left_data=1048576, left_take=1, right_data=8388608, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=262145, expected_right_ready=1, expected_right_valid=1, expected_right_data=4194304),
            Stimulus(left_valid=1, left_data=2097152, right_valid=1, right_data=16777216, right_take=1, expected_left_valid=1, expected_left_data=524289, expected_right_ready=1, expected_right_valid=1, expected_right_data=8388608),
            Stimulus(left_valid=1, left_data=4194304, left_take=1, right_valid=1, right_data=33554432, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=524289, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=8388608, left_take=1, right_data=67108864, expected_left_ready=1, expected_left_valid=1, expected_left_data=1048577, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(left_data=16777216, left_take=1, right_valid=1, right_data=134217728, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=4194305, expected_right_ready=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(left_valid=1, left_data=33554432, right_valid=1, right_data=268435456, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=8388609, expected_right_ready=1, expected_right_valid=1, expected_right_data=67108864),
            Stimulus(left_valid=1, left_data=67108864, left_take=1, right_data=536870912, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=8388609, expected_right_ready=1, expected_right_valid=1, expected_right_data=268435456),
            Stimulus(left_valid=1, left_data=134217728, left_take=1, right_valid=1, right_data=1073741824, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=33554433, expected_right_ready=1, expected_right_valid=1, expected_right_data=536870912),
            Stimulus(left_valid=1, left_data=268435456, left_take=1, right_valid=1, right_data=2147483648, expected_left_ready=1, expected_left_valid=1, expected_left_data=67108865, expected_right_ready=1),
            Stimulus(left_data=536870912, right_data=4294967296, right_take=1, expected_left_valid=1, expected_left_data=134217729, expected_right_ready=1, expected_right_valid=1, expected_right_data=2147483648),
            Stimulus(left_valid=1, left_data=1073741824, left_take=1, right_valid=1, right_data=8589934592, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=134217729, expected_right_ready=1, expected_right_valid=1, expected_right_data=4294967296),
            Stimulus(left_valid=1, left_data=2147483648, left_take=1, right_valid=1, right_data=17179869184, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=268435457, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=4294967296, left_take=1, right_data=34359738368, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=1073741825, expected_right_ready=1, expected_right_valid=1, expected_right_data=17179869184),
            Stimulus(left_valid=1, left_data=8589934592, right_valid=1, right_data=68719476736, expected_left_valid=1, expected_left_data=2147483649, expected_right_ready=1, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_data=17179869184, left_take=1, right_valid=1, right_data=137438953472, expected_left_ready=1, expected_left_valid=1, expected_left_data=2147483649, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=34359738368, left_take=1, right_data=274877906944, expected_left_ready=1, expected_left_valid=1, expected_left_data=4294967297, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=68719476736, left_take=1, right_valid=1, right_data=549755813888, expected_left_ready=1, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=137438953472, right_valid=1, right_data=1099511627776, expected_left_valid=1, expected_left_data=34359738369, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=274877906944, left_take=1, right_data=2199023255552, expected_left_ready=1, expected_left_valid=1, expected_left_data=34359738369, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_data=549755813888, left_take=1, right_valid=1, right_data=4398046511104, expected_left_ready=1, expected_left_valid=1, expected_left_data=68719476737, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=1099511627776, left_take=1, right_valid=1, right_data=8796093022208, expected_left_ready=1, expected_left_valid=1, expected_left_data=274877906945, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=2199023255552, right_data=17592186044416, expected_left_ready=1, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=4398046511104, right_valid=1, right_data=35184372088832, expected_left_valid=1, expected_left_data=1099511627777, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=8796093022208, right_valid=1, right_data=70368744177664, expected_left_valid=1, expected_left_data=1099511627777, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_data=17592186044416, right_data=140737488355328, expected_left_valid=1, expected_left_data=1099511627777, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=35184372088832, right_valid=1, right_data=281474976710656, expected_left_valid=1, expected_left_data=1099511627777, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=70368744177664, right_valid=1, right_data=562949953421312, expected_left_valid=1, expected_left_data=1099511627777, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=140737488355328, right_data=1125899906842624, expected_left_valid=1, expected_left_data=1099511627777, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=281474976710656, right_valid=1, right_data=2251799813685248, expected_left_valid=1, expected_left_data=1099511627777, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_data=562949953421312, right_valid=1, right_data=4503599627370496, right_take=1, expected_left_valid=1, expected_left_data=1099511627777, expected_right_ready=1, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=1125899906842624, right_data=9007199254740992, right_take=1, expected_left_valid=1, expected_left_data=1099511627777, expected_right_ready=1, expected_right_valid=1, expected_right_data=137438953472),
            Stimulus(left_valid=1, left_data=2251799813685248, right_valid=1, right_data=18014398509481984, right_take=1, expected_left_valid=1, expected_left_data=1099511627777, expected_right_ready=1, expected_right_valid=1, expected_right_data=9007199254740992),
            Stimulus(left_valid=1, left_data=4503599627370496, right_valid=1, right_data=36028797018963968, right_take=1, expected_left_valid=1, expected_left_data=1099511627777, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=9007199254740992, right_data=72057594037927936, expected_left_valid=1, expected_left_data=1099511627777, expected_right_valid=1, expected_right_data=36028797018963968),
            Stimulus(left_data=18014398509481984, right_valid=1, right_data=144115188075855872, right_take=1, expected_left_valid=1, expected_left_data=1099511627777, expected_right_ready=1, expected_right_valid=1, expected_right_data=36028797018963968),
            Stimulus(left_valid=1, left_data=36028797018963968, right_valid=1, right_data=288230376151711744, right_take=1, expected_left_valid=1, expected_left_data=1099511627777, expected_right_ready=1, expected_right_valid=1, expected_right_data=72057594037927936),
            Stimulus(left_valid=1, left_data=72057594037927936, right_data=576460752303423488, right_take=1, expected_left_valid=1, expected_left_data=1099511627777, expected_right_ready=1, expected_right_valid=1, expected_right_data=288230376151711744),
            Stimulus(left_valid=1, left_take=1, right_valid=1, right_data=1152921504606846976, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=1099511627777, expected_right_ready=1, expected_right_valid=1, expected_right_data=576460752303423488),
            Stimulus(left_valid=1, left_data=18446744073709551615, left_take=1, right_valid=1, right_data=2305843009213693952, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=2199023255553, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=18446744073709551614, left_take=1, right_valid=1, right_data=4611686018427387904, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=1, expected_right_ready=1, expected_right_valid=1, expected_right_data=2305843009213693952),
            Stimulus(left_valid=1, left_data=9223372036854775808, left_take=1, right_valid=1, right_data=9223372036854775808, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_right_ready=1, expected_right_valid=1, expected_right_data=4611686018427387904),
            Stimulus(left_valid=1, left_data=9223372036854775807, left_take=1, right_valid=1, right_data=1, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=18446744073709551615, expected_right_ready=1, expected_right_valid=1, expected_right_data=9223372036854775808),
            Stimulus(left_valid=1, left_data=12297829382473034410, left_take=1, right_valid=1, right_data=2, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=9223372036854775809, expected_right_ready=1, expected_right_valid=1),
            Stimulus(left_valid=1, left_data=6148914691236517205, left_take=1, right_valid=1, right_data=4, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=9223372036854775808, expected_right_ready=1, expected_right_valid=1, expected_right_data=2),
            Stimulus(left_valid=1, left_data=1, left_take=1, right_valid=1, right_data=8, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=12297829382473034411, expected_right_ready=1, expected_right_valid=1, expected_right_data=4),
            Stimulus(left_valid=1, left_data=2, left_take=1, right_valid=1, right_data=16, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=6148914691236517206, expected_right_ready=1, expected_right_valid=1, expected_right_data=8),
            Stimulus(left_valid=1, left_data=4, left_take=1, right_valid=1, right_data=32, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=2, expected_right_ready=1, expected_right_valid=1, expected_right_data=16),
            Stimulus(left_valid=1, left_data=8, left_take=1, right_valid=1, right_data=64, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=3, expected_right_ready=1, expected_right_valid=1, expected_right_data=32),
            Stimulus(left_valid=1, left_data=16, left_take=1, right_valid=1, right_data=128, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=5, expected_right_ready=1, expected_right_valid=1, expected_right_data=64),
            Stimulus(left_valid=1, left_data=32, left_take=1, right_valid=1, right_data=256, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=9, expected_right_ready=1, expected_right_valid=1, expected_right_data=128),
            Stimulus(left_valid=1, left_data=64, left_take=1, right_valid=1, right_data=512, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=17, expected_right_ready=1, expected_right_valid=1, expected_right_data=256),
            Stimulus(left_valid=1, left_data=128, left_take=1, right_valid=1, right_data=1024, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=33, expected_right_ready=1, expected_right_valid=1, expected_right_data=512),
            Stimulus(left_valid=1, left_data=256, left_take=1, right_valid=1, right_data=2048, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=65, expected_right_ready=1, expected_right_valid=1, expected_right_data=1024),
            Stimulus(left_valid=1, left_data=512, left_take=1, right_valid=1, right_data=4096, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=129, expected_right_ready=1, expected_right_valid=1, expected_right_data=2048),
            Stimulus(left_valid=1, left_data=1024, left_take=1, right_valid=1, right_data=8192, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=257, expected_right_ready=1, expected_right_valid=1, expected_right_data=4096),
            Stimulus(left_valid=1, left_data=2048, left_take=1, right_valid=1, right_data=16384, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=513, expected_right_ready=1, expected_right_valid=1, expected_right_data=8192),
            Stimulus(left_valid=1, left_data=4096, left_take=1, right_valid=1, right_data=32768, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=1025, expected_right_ready=1, expected_right_valid=1, expected_right_data=16384),
            Stimulus(left_valid=1, left_data=8192, left_take=1, right_valid=1, right_data=65536, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=2049, expected_right_ready=1, expected_right_valid=1, expected_right_data=32768),
            Stimulus(left_valid=1, left_data=16384, left_take=1, right_valid=1, right_data=131072, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=4097, expected_right_ready=1, expected_right_valid=1, expected_right_data=65536),
            Stimulus(left_valid=1, left_data=32768, left_take=1, right_valid=1, right_data=262144, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=8193, expected_right_ready=1, expected_right_valid=1, expected_right_data=131072),
            Stimulus(left_valid=1, left_data=65536, left_take=1, right_valid=1, right_data=524288, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=16385, expected_right_ready=1, expected_right_valid=1, expected_right_data=262144),
            Stimulus(left_valid=1, left_data=131072, left_take=1, right_valid=1, right_data=1048576, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=32769, expected_right_ready=1, expected_right_valid=1, expected_right_data=524288),
            Stimulus(left_valid=1, left_data=262144, left_take=1, right_valid=1, right_data=2097152, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=65537, expected_right_ready=1, expected_right_valid=1, expected_right_data=1048576),
            Stimulus(left_valid=1, left_data=524288, left_take=1, right_valid=1, right_data=4194304, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=131073, expected_right_ready=1, expected_right_valid=1, expected_right_data=2097152),
            Stimulus(left_valid=1, left_data=1048576, left_take=1, right_valid=1, right_data=8388608, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=262145, expected_right_ready=1, expected_right_valid=1, expected_right_data=4194304),
            Stimulus(left_valid=1, left_data=2097152, left_take=1, right_valid=1, right_data=16777216, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=524289, expected_right_ready=1, expected_right_valid=1, expected_right_data=8388608),
            Stimulus(left_valid=1, left_data=4194304, left_take=1, right_valid=1, right_data=33554432, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=1048577, expected_right_ready=1, expected_right_valid=1, expected_right_data=16777216),
            Stimulus(left_valid=1, left_data=8388608, left_take=1, right_valid=1, right_data=67108864, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=2097153, expected_right_ready=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(left_valid=1, left_data=16777216, left_take=1, right_valid=1, right_data=134217728, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=4194305, expected_right_ready=1, expected_right_valid=1, expected_right_data=67108864),
            Stimulus(left_valid=1, left_data=33554432, left_take=1, right_valid=1, right_data=268435456, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=8388609, expected_right_ready=1, expected_right_valid=1, expected_right_data=134217728),
            Stimulus(left_valid=1, left_data=67108864, left_take=1, right_valid=1, right_data=536870912, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=16777217, expected_right_ready=1, expected_right_valid=1, expected_right_data=268435456),
            Stimulus(left_valid=1, left_data=134217728, left_take=1, right_valid=1, right_data=1073741824, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=33554433, expected_right_ready=1, expected_right_valid=1, expected_right_data=536870912),
            Stimulus(left_valid=1, left_data=268435456, left_take=1, right_valid=1, right_data=2147483648, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=67108865, expected_right_ready=1, expected_right_valid=1, expected_right_data=1073741824),
            Stimulus(left_valid=1, left_data=536870912, left_take=1, right_valid=1, right_data=4294967296, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=134217729, expected_right_ready=1, expected_right_valid=1, expected_right_data=2147483648),
            Stimulus(left_valid=1, left_data=1073741824, left_take=1, right_valid=1, right_data=8589934592, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=268435457, expected_right_ready=1, expected_right_valid=1, expected_right_data=4294967296),
            Stimulus(left_valid=1, left_data=2147483648, left_take=1, right_valid=1, right_data=17179869184, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=536870913, expected_right_ready=1, expected_right_valid=1, expected_right_data=8589934592),
            Stimulus(left_valid=1, left_data=4294967296, left_take=1, right_valid=1, right_data=34359738368, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=1073741825, expected_right_ready=1, expected_right_valid=1, expected_right_data=17179869184),
            Stimulus(left_valid=1, left_data=8589934592, left_take=1, right_valid=1, right_data=68719476736, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=2147483649, expected_right_ready=1, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(left_valid=1, left_data=17179869184, left_take=1, right_valid=1, right_data=137438953472, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=4294967297, expected_right_ready=1, expected_right_valid=1, expected_right_data=68719476736),
            Stimulus(left_valid=1, left_data=34359738368, left_take=1, right_valid=1, right_data=274877906944, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=8589934593, expected_right_ready=1, expected_right_valid=1, expected_right_data=137438953472),
            Stimulus(left_valid=1, left_data=68719476736, left_take=1, right_valid=1, right_data=549755813888, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=17179869185, expected_right_ready=1, expected_right_valid=1, expected_right_data=274877906944),
            Stimulus(left_valid=1, left_data=137438953472, left_take=1, right_valid=1, right_data=1099511627776, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=34359738369, expected_right_ready=1, expected_right_valid=1, expected_right_data=549755813888),
            Stimulus(left_valid=1, left_data=274877906944, left_take=1, right_valid=1, right_data=2199023255552, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=68719476737, expected_right_ready=1, expected_right_valid=1, expected_right_data=1099511627776),
            Stimulus(left_valid=1, left_data=549755813888, left_take=1, right_valid=1, right_data=4398046511104, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=137438953473, expected_right_ready=1, expected_right_valid=1, expected_right_data=2199023255552),
            Stimulus(left_valid=1, left_data=1099511627776, left_take=1, right_valid=1, right_data=8796093022208, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=274877906945, expected_right_ready=1, expected_right_valid=1, expected_right_data=4398046511104),
            Stimulus(left_valid=1, left_data=2199023255552, left_take=1, right_valid=1, right_data=17592186044416, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=549755813889, expected_right_ready=1, expected_right_valid=1, expected_right_data=8796093022208),
            Stimulus(left_valid=1, left_data=4398046511104, left_take=1, right_valid=1, right_data=35184372088832, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=1099511627777, expected_right_ready=1, expected_right_valid=1, expected_right_data=17592186044416),
            Stimulus(left_valid=1, left_data=8796093022208, left_take=1, right_valid=1, right_data=70368744177664, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=2199023255553, expected_right_ready=1, expected_right_valid=1, expected_right_data=35184372088832),
            Stimulus(left_valid=1, left_data=17592186044416, left_take=1, right_valid=1, right_data=140737488355328, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=4398046511105, expected_right_ready=1, expected_right_valid=1, expected_right_data=70368744177664),
            Stimulus(left_valid=1, left_data=35184372088832, left_take=1, right_valid=1, right_data=281474976710656, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=8796093022209, expected_right_ready=1, expected_right_valid=1, expected_right_data=140737488355328),
            Stimulus(left_valid=1, left_data=70368744177664, left_take=1, right_valid=1, right_data=562949953421312, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=17592186044417, expected_right_ready=1, expected_right_valid=1, expected_right_data=281474976710656),
            Stimulus(left_valid=1, left_data=140737488355328, left_take=1, right_valid=1, right_data=1125899906842624, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=35184372088833, expected_right_ready=1, expected_right_valid=1, expected_right_data=562949953421312),
            Stimulus(left_valid=1, left_data=281474976710656, left_take=1, right_valid=1, right_data=2251799813685248, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=70368744177665, expected_right_ready=1, expected_right_valid=1, expected_right_data=1125899906842624),
            Stimulus(left_valid=1, left_data=562949953421312, left_take=1, right_valid=1, right_data=4503599627370496, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=140737488355329, expected_right_ready=1, expected_right_valid=1, expected_right_data=2251799813685248),
            Stimulus(left_valid=1, left_data=1125899906842624, left_take=1, right_valid=1, right_data=9007199254740992, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=281474976710657, expected_right_ready=1, expected_right_valid=1, expected_right_data=4503599627370496),
            Stimulus(left_valid=1, left_data=2251799813685248, left_take=1, right_valid=1, right_data=18014398509481984, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=562949953421313, expected_right_ready=1, expected_right_valid=1, expected_right_data=9007199254740992),
            Stimulus(left_valid=1, left_data=4503599627370496, left_take=1, right_valid=1, right_data=36028797018963968, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=1125899906842625, expected_right_ready=1, expected_right_valid=1, expected_right_data=18014398509481984),
            Stimulus(left_valid=1, left_data=9007199254740992, left_take=1, right_valid=1, right_data=72057594037927936, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=2251799813685249, expected_right_ready=1, expected_right_valid=1, expected_right_data=36028797018963968),
            Stimulus(left_valid=1, left_data=18014398509481984, left_take=1, right_valid=1, right_data=144115188075855872, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=4503599627370497, expected_right_ready=1, expected_right_valid=1, expected_right_data=72057594037927936),
            Stimulus(left_valid=1, left_data=36028797018963968, left_take=1, right_valid=1, right_data=288230376151711744, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=9007199254740993, expected_right_ready=1, expected_right_valid=1, expected_right_data=144115188075855872),
            Stimulus(left_valid=1, left_data=72057594037927936, left_take=1, right_valid=1, right_data=576460752303423488, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=18014398509481985, expected_right_ready=1, expected_right_valid=1, expected_right_data=288230376151711744),
            Stimulus(left_valid=1, left_data=144115188075855872, left_take=1, right_valid=1, right_data=1152921504606846976, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=36028797018963969, expected_right_ready=1, expected_right_valid=1, expected_right_data=576460752303423488),
            Stimulus(left_valid=1, left_data=288230376151711744, left_take=1, right_valid=1, right_data=2305843009213693952, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=72057594037927937, expected_right_ready=1, expected_right_valid=1, expected_right_data=1152921504606846976),
            Stimulus(left_valid=1, left_data=576460752303423488, left_take=1, right_valid=1, right_data=4611686018427387904, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=144115188075855873, expected_right_ready=1, expected_right_valid=1, expected_right_data=2305843009213693952),
            Stimulus(left_valid=1, left_data=1152921504606846976, left_take=1, right_valid=1, right_data=9223372036854775808, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=288230376151711745, expected_right_ready=1, expected_right_valid=1, expected_right_data=4611686018427387904),
            Stimulus(left_valid=1, left_data=2305843009213693952, left_take=1, right_valid=1, right_data=1, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=576460752303423489, expected_right_ready=1, expected_right_valid=1, expected_right_data=9223372036854775808),
            Stimulus(left_valid=1, left_data=4611686018427387904, left_take=1, right_valid=1, right_data=2, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=1152921504606846977, expected_right_ready=1, expected_right_valid=1),
            Stimulus(left_data=9223372036854775808, left_take=1, right_data=4, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=2305843009213693953, expected_right_ready=1, expected_right_valid=1, expected_right_data=2),
            Stimulus(left_data=1, left_take=1, right_data=8, right_take=1, expected_left_ready=1, expected_left_valid=1, expected_left_data=4611686018427387905, expected_right_ready=1, expected_right_valid=1, expected_right_data=4),
            Stimulus(left_data=2, left_take=1, right_data=16, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=4, left_take=1, right_data=32, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=8, left_take=1, right_data=64, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=16, left_take=1, right_data=128, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=32, left_take=1, right_data=256, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=64, left_take=1, right_data=512, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=128, left_take=1, right_data=1024, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=256, left_take=1, right_data=2048, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=512, left_take=1, right_data=4096, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=1024, left_take=1, right_data=8192, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=2048, left_take=1, right_data=16384, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=4096, left_take=1, right_data=32768, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=8192, left_take=1, right_data=65536, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=16384, left_take=1, right_data=131072, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=32768, left_take=1, right_data=262144, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=65536, left_take=1, right_data=524288, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=131072, left_take=1, right_data=1048576, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=262144, left_take=1, right_data=2097152, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=524288, left_take=1, right_data=4194304, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=1048576, left_take=1, right_data=8388608, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=2097152, left_take=1, right_data=16777216, right_take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=4194304, left_take=1, right_data=33554432, right_take=1, expected_left_ready=1, expected_right_ready=1),
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
def RulePairPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    left, right = RulePairPipeline(
        frame.left_valid,
        frame.left_data,
        frame.left_take,
        frame.right_valid,
        frame.right_data,
        frame.right_take,
    )

    @rule
    def exercise():
        assert left.ready == frame.expected_left_ready, (
            "rule_pair_pipeline left_ready old-state check"
        )
        assert left.valid == frame.expected_left_valid, (
            "rule_pair_pipeline left_valid old-state check"
        )
        assert left.data == frame.expected_left_data, (
            "rule_pair_pipeline left_data old-state check"
        )
        assert right.ready == frame.expected_right_ready, (
            "rule_pair_pipeline right_ready old-state check"
        )
        assert right.valid == frame.expected_right_valid, (
            "rule_pair_pipeline right_valid old-state check"
        )
        assert right.data == frame.expected_right_data, (
            "rule_pair_pipeline right_data old-state check"
        )
        log("info", "phase", phase)
        log("info", "left_ready", left.ready)
        log("info", "left_valid", left.valid)
        log("info", "left_data", left.data)
        log("info", "right_ready", right.ready)
        log("info", "right_valid", right.valid)
        log("info", "right_data", right.data)

    advance(phase)
    exercise()

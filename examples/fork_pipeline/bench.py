"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_fork_pipeline.fork_pipeline import ForkPipeline
from pycircuit import bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    data: bits[64]
    left_take: bits[1]
    right_take: bits[1]
    expected_ready: bits[1]
    expected_left_valid: bits[1]
    expected_left_data: bits[64]
    expected_right_valid: bits[1]
    expected_right_data: bits[64]


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
            Stimulus(valid=1, data=18446744073709551614, expected_ready=1, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=9223372036854775808, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=9223372036854775807, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=12297829382473034410, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=6148914691236517205, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=1, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=2, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=4, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=8, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=16, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=32, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=64, left_take=1, expected_left_valid=1, expected_right_valid=1),
            Stimulus(valid=1, data=128, left_take=1, expected_left_valid=1, expected_left_data=18446744073709551615, expected_right_valid=1),
            Stimulus(valid=1, data=256, left_take=1, expected_left_valid=1, expected_left_data=18446744073709551614, expected_right_valid=1),
            Stimulus(valid=1, data=512, expected_right_valid=1),
            Stimulus(valid=1, data=1024, left_take=1, expected_right_valid=1),
            Stimulus(valid=1, data=2048, left_take=1, expected_right_valid=1),
            Stimulus(valid=1, data=4096, left_take=1, expected_right_valid=1),
            Stimulus(valid=1, data=8192, expected_right_valid=1),
            Stimulus(valid=1, data=16384, left_take=1, expected_right_valid=1),
            Stimulus(valid=1, data=32768, left_take=1, expected_right_valid=1),
            Stimulus(valid=1, data=65536, left_take=1, expected_right_valid=1),
            Stimulus(valid=1, data=131072, right_take=1, expected_ready=1, expected_right_valid=1),
            Stimulus(valid=1, data=262144, left_take=1, expected_right_valid=1, expected_right_data=18446744073709551615),
            Stimulus(valid=1, data=524288, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=131072, expected_right_valid=1, expected_right_data=18446744073709551615),
            Stimulus(valid=1, data=1048576, left_take=1, right_take=1, expected_ready=1, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(valid=1, data=2097152, expected_left_valid=1, expected_left_data=524288, expected_right_valid=1, expected_right_data=131072),
            Stimulus(valid=1, data=4194304, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=524288, expected_right_valid=1, expected_right_data=131072),
            Stimulus(valid=1, data=8388608, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=1048576, expected_right_valid=1, expected_right_data=524288),
            Stimulus(data=16777216, left_take=1, expected_left_valid=1, expected_left_data=4194304, expected_right_valid=1, expected_right_data=1048576),
            Stimulus(valid=1, data=33554432, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=8388608, expected_right_valid=1, expected_right_data=1048576),
            Stimulus(valid=1, data=67108864, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=8388608, expected_right_valid=1, expected_right_data=4194304),
            Stimulus(valid=1, data=134217728, left_take=1, expected_left_valid=1, expected_left_data=33554432, expected_right_valid=1, expected_right_data=8388608),
            Stimulus(valid=1, data=268435456, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=67108864, expected_right_valid=1, expected_right_data=8388608),
            Stimulus(data=536870912, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=1073741824, left_take=1, expected_left_valid=1, expected_left_data=268435456, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=2147483648, left_take=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=4294967296, left_take=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=8589934592, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(data=17179869184, left_take=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=34359738368, left_take=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=68719476736, left_take=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=137438953472, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=274877906944, left_take=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(data=549755813888, left_take=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=1099511627776, left_take=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=2199023255552, right_take=1, expected_ready=1, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=4398046511104, expected_right_valid=1, expected_right_data=67108864),
            Stimulus(valid=1, data=8796093022208, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=2199023255552, expected_right_valid=1, expected_right_data=67108864),
            Stimulus(data=17592186044416, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=2199023255552, expected_right_valid=1, expected_right_data=268435456),
            Stimulus(valid=1, data=35184372088832, expected_ready=1, expected_left_valid=1, expected_left_data=2199023255552, expected_right_valid=1, expected_right_data=2199023255552),
            Stimulus(valid=1, data=70368744177664, right_take=1, expected_left_valid=1, expected_left_data=2199023255552, expected_right_valid=1, expected_right_data=2199023255552),
            Stimulus(valid=1, data=140737488355328, right_take=1, expected_left_valid=1, expected_left_data=2199023255552, expected_right_valid=1, expected_right_data=8796093022208),
            Stimulus(valid=1, data=281474976710656, expected_left_valid=1, expected_left_data=2199023255552, expected_right_valid=1, expected_right_data=35184372088832),
            Stimulus(data=562949953421312, right_take=1, expected_left_valid=1, expected_left_data=2199023255552, expected_right_valid=1, expected_right_data=35184372088832),
            Stimulus(valid=1, data=1125899906842624, right_take=1, expected_left_valid=1, expected_left_data=2199023255552),
            Stimulus(valid=1, data=2251799813685248, expected_left_valid=1, expected_left_data=2199023255552),
            Stimulus(valid=1, data=4503599627370496, right_take=1, expected_left_valid=1, expected_left_data=2199023255552),
            Stimulus(valid=1, data=9007199254740992, right_take=1, expected_left_valid=1, expected_left_data=2199023255552),
            Stimulus(data=18014398509481984, expected_left_valid=1, expected_left_data=2199023255552),
            Stimulus(valid=1, data=36028797018963968, right_take=1, expected_left_valid=1, expected_left_data=2199023255552),
            Stimulus(valid=1, data=72057594037927936, right_take=1, expected_left_valid=1, expected_left_data=2199023255552),
            Stimulus(valid=1, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=2199023255552),
            Stimulus(valid=1, data=18446744073709551615, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=8796093022208),
            Stimulus(valid=1, data=18446744073709551614, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=35184372088832, expected_right_valid=1),
            Stimulus(valid=1, data=9223372036854775808, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_right_valid=1, expected_right_data=18446744073709551615),
            Stimulus(valid=1, data=9223372036854775807, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=18446744073709551615, expected_right_valid=1, expected_right_data=18446744073709551614),
            Stimulus(valid=1, data=12297829382473034410, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=18446744073709551614, expected_right_valid=1, expected_right_data=9223372036854775808),
            Stimulus(valid=1, data=6148914691236517205, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=9223372036854775808, expected_right_valid=1, expected_right_data=9223372036854775807),
            Stimulus(valid=1, data=1, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=9223372036854775807, expected_right_valid=1, expected_right_data=12297829382473034410),
            Stimulus(valid=1, data=2, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=12297829382473034410, expected_right_valid=1, expected_right_data=6148914691236517205),
            Stimulus(valid=1, data=4, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=6148914691236517205, expected_right_valid=1, expected_right_data=1),
            Stimulus(valid=1, data=8, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=1, expected_right_valid=1, expected_right_data=2),
            Stimulus(valid=1, data=16, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=2, expected_right_valid=1, expected_right_data=4),
            Stimulus(valid=1, data=32, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=4, expected_right_valid=1, expected_right_data=8),
            Stimulus(valid=1, data=64, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=8, expected_right_valid=1, expected_right_data=16),
            Stimulus(valid=1, data=128, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=16, expected_right_valid=1, expected_right_data=32),
            Stimulus(valid=1, data=256, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=32, expected_right_valid=1, expected_right_data=64),
            Stimulus(valid=1, data=512, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=64, expected_right_valid=1, expected_right_data=128),
            Stimulus(valid=1, data=1024, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=128, expected_right_valid=1, expected_right_data=256),
            Stimulus(valid=1, data=2048, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=256, expected_right_valid=1, expected_right_data=512),
            Stimulus(valid=1, data=4096, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=512, expected_right_valid=1, expected_right_data=1024),
            Stimulus(valid=1, data=8192, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=1024, expected_right_valid=1, expected_right_data=2048),
            Stimulus(valid=1, data=16384, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=2048, expected_right_valid=1, expected_right_data=4096),
            Stimulus(valid=1, data=32768, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=4096, expected_right_valid=1, expected_right_data=8192),
            Stimulus(valid=1, data=65536, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=8192, expected_right_valid=1, expected_right_data=16384),
            Stimulus(valid=1, data=131072, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=16384, expected_right_valid=1, expected_right_data=32768),
            Stimulus(valid=1, data=262144, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=32768, expected_right_valid=1, expected_right_data=65536),
            Stimulus(valid=1, data=524288, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=65536, expected_right_valid=1, expected_right_data=131072),
            Stimulus(valid=1, data=1048576, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=131072, expected_right_valid=1, expected_right_data=262144),
            Stimulus(valid=1, data=2097152, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=262144, expected_right_valid=1, expected_right_data=524288),
            Stimulus(valid=1, data=4194304, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=524288, expected_right_valid=1, expected_right_data=1048576),
            Stimulus(valid=1, data=8388608, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=1048576, expected_right_valid=1, expected_right_data=2097152),
            Stimulus(valid=1, data=16777216, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=2097152, expected_right_valid=1, expected_right_data=4194304),
            Stimulus(valid=1, data=33554432, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=4194304, expected_right_valid=1, expected_right_data=8388608),
            Stimulus(valid=1, data=67108864, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=8388608, expected_right_valid=1, expected_right_data=16777216),
            Stimulus(valid=1, data=134217728, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=16777216, expected_right_valid=1, expected_right_data=33554432),
            Stimulus(valid=1, data=268435456, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=33554432, expected_right_valid=1, expected_right_data=67108864),
            Stimulus(valid=1, data=536870912, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=67108864, expected_right_valid=1, expected_right_data=134217728),
            Stimulus(valid=1, data=1073741824, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=134217728, expected_right_valid=1, expected_right_data=268435456),
            Stimulus(valid=1, data=2147483648, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=268435456, expected_right_valid=1, expected_right_data=536870912),
            Stimulus(valid=1, data=4294967296, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=536870912, expected_right_valid=1, expected_right_data=1073741824),
            Stimulus(valid=1, data=8589934592, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=1073741824, expected_right_valid=1, expected_right_data=2147483648),
            Stimulus(valid=1, data=17179869184, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=2147483648, expected_right_valid=1, expected_right_data=4294967296),
            Stimulus(valid=1, data=34359738368, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=4294967296, expected_right_valid=1, expected_right_data=8589934592),
            Stimulus(valid=1, data=68719476736, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=8589934592, expected_right_valid=1, expected_right_data=17179869184),
            Stimulus(valid=1, data=137438953472, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=17179869184, expected_right_valid=1, expected_right_data=34359738368),
            Stimulus(valid=1, data=274877906944, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=34359738368, expected_right_valid=1, expected_right_data=68719476736),
            Stimulus(valid=1, data=549755813888, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=68719476736, expected_right_valid=1, expected_right_data=137438953472),
            Stimulus(valid=1, data=1099511627776, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=137438953472, expected_right_valid=1, expected_right_data=274877906944),
            Stimulus(valid=1, data=2199023255552, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=274877906944, expected_right_valid=1, expected_right_data=549755813888),
            Stimulus(valid=1, data=4398046511104, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=549755813888, expected_right_valid=1, expected_right_data=1099511627776),
            Stimulus(valid=1, data=8796093022208, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=1099511627776, expected_right_valid=1, expected_right_data=2199023255552),
            Stimulus(valid=1, data=17592186044416, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=2199023255552, expected_right_valid=1, expected_right_data=4398046511104),
            Stimulus(valid=1, data=35184372088832, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=4398046511104, expected_right_valid=1, expected_right_data=8796093022208),
            Stimulus(valid=1, data=70368744177664, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=8796093022208, expected_right_valid=1, expected_right_data=17592186044416),
            Stimulus(valid=1, data=140737488355328, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=17592186044416, expected_right_valid=1, expected_right_data=35184372088832),
            Stimulus(valid=1, data=281474976710656, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=35184372088832, expected_right_valid=1, expected_right_data=70368744177664),
            Stimulus(valid=1, data=562949953421312, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=70368744177664, expected_right_valid=1, expected_right_data=140737488355328),
            Stimulus(valid=1, data=1125899906842624, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=140737488355328, expected_right_valid=1, expected_right_data=281474976710656),
            Stimulus(valid=1, data=2251799813685248, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=281474976710656, expected_right_valid=1, expected_right_data=562949953421312),
            Stimulus(valid=1, data=4503599627370496, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=562949953421312, expected_right_valid=1, expected_right_data=1125899906842624),
            Stimulus(valid=1, data=9007199254740992, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=1125899906842624, expected_right_valid=1, expected_right_data=2251799813685248),
            Stimulus(valid=1, data=18014398509481984, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=2251799813685248, expected_right_valid=1, expected_right_data=4503599627370496),
            Stimulus(valid=1, data=36028797018963968, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=4503599627370496, expected_right_valid=1, expected_right_data=9007199254740992),
            Stimulus(valid=1, data=72057594037927936, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=9007199254740992, expected_right_valid=1, expected_right_data=18014398509481984),
            Stimulus(valid=1, data=144115188075855872, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=18014398509481984, expected_right_valid=1, expected_right_data=36028797018963968),
            Stimulus(valid=1, data=288230376151711744, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=36028797018963968, expected_right_valid=1, expected_right_data=72057594037927936),
            Stimulus(valid=1, data=576460752303423488, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=72057594037927936, expected_right_valid=1, expected_right_data=144115188075855872),
            Stimulus(valid=1, data=1152921504606846976, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=144115188075855872, expected_right_valid=1, expected_right_data=288230376151711744),
            Stimulus(valid=1, data=2305843009213693952, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=288230376151711744, expected_right_valid=1, expected_right_data=576460752303423488),
            Stimulus(valid=1, data=4611686018427387904, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=576460752303423488, expected_right_valid=1, expected_right_data=1152921504606846976),
            Stimulus(data=9223372036854775808, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=1152921504606846976, expected_right_valid=1, expected_right_data=2305843009213693952),
            Stimulus(data=1, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=2305843009213693952, expected_right_valid=1, expected_right_data=4611686018427387904),
            Stimulus(data=2, left_take=1, right_take=1, expected_ready=1, expected_left_valid=1, expected_left_data=4611686018427387904),
            Stimulus(data=4, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=8, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=16, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=32, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=64, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=128, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=256, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=512, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=1024, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=2048, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=4096, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=8192, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=16384, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=32768, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=65536, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=131072, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=262144, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=524288, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=1048576, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=2097152, left_take=1, right_take=1, expected_ready=1),
            Stimulus(data=4194304, left_take=1, right_take=1, expected_ready=1),
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
def ForkPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = ForkPipeline(frame.valid, frame.data, frame.left_take, frame.right_take)

    @rule
    def exercise():
        assert dut.ready == frame.expected_ready, "fork_pipeline ready old-state check"
        assert (
            dut.left_valid == frame.expected_left_valid
        ), "fork_pipeline left_valid old-state check"
        assert (
            dut.left_data == frame.expected_left_data
        ), "fork_pipeline left_data old-state check"
        assert (
            dut.right_valid == frame.expected_right_valid
        ), "fork_pipeline right_valid old-state check"
        assert (
            dut.right_data == frame.expected_right_data
        ), "fork_pipeline right_data old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "left_valid", dut.left_valid)
        log("info", "left_data", dut.left_data)
        log("info", "right_valid", dut.right_valid)
        log("info", "right_data", dut.right_data)

    advance(phase)
    exercise()

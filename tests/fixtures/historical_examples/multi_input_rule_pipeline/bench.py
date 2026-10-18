"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_multi_input_rule_pipeline.multi_input_rule_pipeline import (
    MultiInputRulePipeline,
)
from pycircuit import bits, log, rule, struct, system, table


@struct
class Stimulus:
    left_valid: bits[1]
    left_data: bits[64]
    right_valid: bits[1]
    right_data: bits[64]
    take: bits[1]
    expected_left_ready: bits[1]
    expected_right_ready: bits[1]
    expected_valid: bits[1]
    expected_data: bits[64]


@struct
class Stimuli:
    rows: table[159, Stimulus]


def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(left_valid=1, right_data=9223372036854775808, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=18446744073709551615, right_valid=1, right_data=9223372036854775807, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=18446744073709551614, right_valid=1, right_data=12297829382473034410, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=9223372036854775808, right_data=6148914691236517205, expected_right_ready=1, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=9223372036854775807, right_valid=1, right_data=1, expected_right_ready=1, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=12297829382473034410, right_valid=1, right_data=2, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=6148914691236517205, right_data=4, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=1, right_valid=1, right_data=8, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=2, right_valid=1, right_data=16, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=4, right_data=32, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=8, right_valid=1, right_data=64, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=16, right_valid=1, right_data=128, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=32, right_data=256, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=64, right_valid=1, right_data=512, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=9223372036854775807),
            Stimulus(left_valid=1, left_data=128, right_valid=1, right_data=1024, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=12297829382473034409),
            Stimulus(left_valid=1, left_data=256, right_data=2048, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=18446744073709551615),
            Stimulus(left_valid=1, left_data=512, right_valid=1, right_data=4096, expected_right_ready=1, expected_valid=1, expected_data=576),
            Stimulus(left_valid=1, left_data=1024, right_valid=1, right_data=8192, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=576),
            Stimulus(left_valid=1, left_data=2048, right_data=16384, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=1152),
            Stimulus(left_valid=1, left_data=4096, right_valid=1, right_data=32768, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=4352),
            Stimulus(left_valid=1, left_data=8192, right_valid=1, right_data=65536, expected_right_ready=1, expected_valid=1, expected_data=9216),
            Stimulus(left_valid=1, left_data=16384, right_data=131072, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=9216),
            Stimulus(left_valid=1, left_data=32768, right_valid=1, right_data=262144, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=34816),
            Stimulus(left_valid=1, left_data=65536, right_valid=1, right_data=524288, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=69632),
            Stimulus(left_valid=1, left_data=131072, right_data=1048576, expected_right_ready=1, expected_valid=1, expected_data=278528),
            Stimulus(left_valid=1, left_data=262144, right_valid=1, right_data=2097152, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=278528),
            Stimulus(left_valid=1, left_data=524288, right_valid=1, right_data=4194304, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=557056),
            Stimulus(left_valid=1, left_data=1048576, right_data=8388608, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=2162688),
            Stimulus(left_valid=1, left_data=2097152, right_valid=1, right_data=16777216, expected_right_ready=1, expected_valid=1, expected_data=4456448),
            Stimulus(left_valid=1, left_data=4194304, right_valid=1, right_data=33554432, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=4456448),
            Stimulus(left_valid=1, left_data=8388608, right_data=67108864, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=17301504),
            Stimulus(left_data=16777216, right_valid=1, right_data=134217728, take=1, expected_right_ready=1, expected_valid=1, expected_data=34603008),
            Stimulus(left_valid=1, left_data=33554432, right_valid=1, right_data=268435456, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=67108864, right_data=536870912, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=138412032),
            Stimulus(left_valid=1, left_data=134217728, right_valid=1, right_data=1073741824, take=1, expected_right_ready=1, expected_valid=1, expected_data=276824064),
            Stimulus(left_valid=1, left_data=268435456, right_valid=1, right_data=2147483648, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=536870912, right_data=4294967296, expected_right_ready=1, expected_valid=1, expected_data=1107296256),
            Stimulus(left_valid=1, left_data=1073741824, right_valid=1, right_data=8589934592, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=1107296256),
            Stimulus(left_valid=1, left_data=2147483648, right_valid=1, right_data=17179869184, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=2214592512),
            Stimulus(left_valid=1, left_data=4294967296, right_data=34359738368, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=8858370048),
            Stimulus(left_valid=1, left_data=8589934592, right_valid=1, right_data=68719476736, expected_right_ready=1, expected_valid=1, expected_data=18253611008),
            Stimulus(left_data=17179869184, right_valid=1, right_data=137438953472, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=18253611008),
            Stimulus(left_valid=1, left_data=34359738368, right_data=274877906944, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=70866960384),
            Stimulus(left_valid=1, left_data=68719476736, right_valid=1, right_data=549755813888, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=141733920768),
            Stimulus(left_valid=1, left_data=137438953472, right_valid=1, right_data=1099511627776, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=274877906944, right_data=2199023255552, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=584115552256),
            Stimulus(left_data=549755813888, right_valid=1, right_data=4398046511104, take=1, expected_right_ready=1, expected_valid=1, expected_data=1168231104512),
            Stimulus(left_valid=1, left_data=1099511627776, right_valid=1, right_data=8796093022208, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_valid=1, left_data=2199023255552, right_data=17592186044416, expected_right_ready=1, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=4398046511104, right_valid=1, right_data=35184372088832, expected_right_ready=1, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=8796093022208, right_valid=1, right_data=70368744177664, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_data=17592186044416, right_data=140737488355328, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=35184372088832, right_valid=1, right_data=281474976710656, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=70368744177664, right_valid=1, right_data=562949953421312, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=140737488355328, right_data=1125899906842624, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=281474976710656, right_valid=1, right_data=2251799813685248, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_data=562949953421312, right_valid=1, right_data=4503599627370496, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=1125899906842624, right_data=9007199254740992, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=2251799813685248, right_valid=1, right_data=18014398509481984, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=4503599627370496, right_valid=1, right_data=36028797018963968, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=9007199254740992, right_data=72057594037927936, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_data=18014398509481984, right_valid=1, right_data=144115188075855872, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=36028797018963968, right_valid=1, right_data=288230376151711744, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=72057594037927936, right_data=576460752303423488, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, right_valid=1, right_data=1152921504606846976, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=4535485464576),
            Stimulus(left_valid=1, left_data=18446744073709551615, right_valid=1, right_data=2305843009213693952, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=9070970929152),
            Stimulus(left_valid=1, left_data=18446744073709551614, right_valid=1, right_data=4611686018427387904, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=36283883716608),
            Stimulus(left_valid=1, left_data=9223372036854775808, right_valid=1, right_data=9223372036854775808, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=1152921504606846976),
            Stimulus(left_valid=1, left_data=9223372036854775807, right_valid=1, right_data=1, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=2305843009213693951),
            Stimulus(left_valid=1, left_data=12297829382473034410, right_valid=1, right_data=2, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=4611686018427387902),
            Stimulus(left_valid=1, left_data=6148914691236517205, right_valid=1, right_data=4, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1),
            Stimulus(left_valid=1, left_data=1, right_valid=1, right_data=8, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=9223372036854775808),
            Stimulus(left_valid=1, left_data=2, right_valid=1, right_data=16, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=12297829382473034412),
            Stimulus(left_valid=1, left_data=4, right_valid=1, right_data=32, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=6148914691236517209),
            Stimulus(left_valid=1, left_data=8, right_valid=1, right_data=64, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=9),
            Stimulus(left_valid=1, left_data=16, right_valid=1, right_data=128, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=18),
            Stimulus(left_valid=1, left_data=32, right_valid=1, right_data=256, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=36),
            Stimulus(left_valid=1, left_data=64, right_valid=1, right_data=512, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=72),
            Stimulus(left_valid=1, left_data=128, right_valid=1, right_data=1024, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=144),
            Stimulus(left_valid=1, left_data=256, right_valid=1, right_data=2048, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=288),
            Stimulus(left_valid=1, left_data=512, right_valid=1, right_data=4096, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=576),
            Stimulus(left_valid=1, left_data=1024, right_valid=1, right_data=8192, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=1152),
            Stimulus(left_valid=1, left_data=2048, right_valid=1, right_data=16384, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=2304),
            Stimulus(left_valid=1, left_data=4096, right_valid=1, right_data=32768, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=4608),
            Stimulus(left_valid=1, left_data=8192, right_valid=1, right_data=65536, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=9216),
            Stimulus(left_valid=1, left_data=16384, right_valid=1, right_data=131072, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=18432),
            Stimulus(left_valid=1, left_data=32768, right_valid=1, right_data=262144, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=36864),
            Stimulus(left_valid=1, left_data=65536, right_valid=1, right_data=524288, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=73728),
            Stimulus(left_valid=1, left_data=131072, right_valid=1, right_data=1048576, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=147456),
            Stimulus(left_valid=1, left_data=262144, right_valid=1, right_data=2097152, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=294912),
            Stimulus(left_valid=1, left_data=524288, right_valid=1, right_data=4194304, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=589824),
            Stimulus(left_valid=1, left_data=1048576, right_valid=1, right_data=8388608, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=1179648),
            Stimulus(left_valid=1, left_data=2097152, right_valid=1, right_data=16777216, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=2359296),
            Stimulus(left_valid=1, left_data=4194304, right_valid=1, right_data=33554432, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=4718592),
            Stimulus(left_valid=1, left_data=8388608, right_valid=1, right_data=67108864, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=9437184),
            Stimulus(left_valid=1, left_data=16777216, right_valid=1, right_data=134217728, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=18874368),
            Stimulus(left_valid=1, left_data=33554432, right_valid=1, right_data=268435456, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=37748736),
            Stimulus(left_valid=1, left_data=67108864, right_valid=1, right_data=536870912, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=75497472),
            Stimulus(left_valid=1, left_data=134217728, right_valid=1, right_data=1073741824, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=150994944),
            Stimulus(left_valid=1, left_data=268435456, right_valid=1, right_data=2147483648, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=301989888),
            Stimulus(left_valid=1, left_data=536870912, right_valid=1, right_data=4294967296, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=603979776),
            Stimulus(left_valid=1, left_data=1073741824, right_valid=1, right_data=8589934592, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=1207959552),
            Stimulus(left_valid=1, left_data=2147483648, right_valid=1, right_data=17179869184, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=2415919104),
            Stimulus(left_valid=1, left_data=4294967296, right_valid=1, right_data=34359738368, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=4831838208),
            Stimulus(left_valid=1, left_data=8589934592, right_valid=1, right_data=68719476736, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=9663676416),
            Stimulus(left_valid=1, left_data=17179869184, right_valid=1, right_data=137438953472, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=19327352832),
            Stimulus(left_valid=1, left_data=34359738368, right_valid=1, right_data=274877906944, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=38654705664),
            Stimulus(left_valid=1, left_data=68719476736, right_valid=1, right_data=549755813888, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=77309411328),
            Stimulus(left_valid=1, left_data=137438953472, right_valid=1, right_data=1099511627776, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=154618822656),
            Stimulus(left_valid=1, left_data=274877906944, right_valid=1, right_data=2199023255552, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=309237645312),
            Stimulus(left_valid=1, left_data=549755813888, right_valid=1, right_data=4398046511104, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=618475290624),
            Stimulus(left_valid=1, left_data=1099511627776, right_valid=1, right_data=8796093022208, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=1236950581248),
            Stimulus(left_valid=1, left_data=2199023255552, right_valid=1, right_data=17592186044416, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=2473901162496),
            Stimulus(left_valid=1, left_data=4398046511104, right_valid=1, right_data=35184372088832, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=4947802324992),
            Stimulus(left_valid=1, left_data=8796093022208, right_valid=1, right_data=70368744177664, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=9895604649984),
            Stimulus(left_valid=1, left_data=17592186044416, right_valid=1, right_data=140737488355328, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=19791209299968),
            Stimulus(left_valid=1, left_data=35184372088832, right_valid=1, right_data=281474976710656, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=39582418599936),
            Stimulus(left_valid=1, left_data=70368744177664, right_valid=1, right_data=562949953421312, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=79164837199872),
            Stimulus(left_valid=1, left_data=140737488355328, right_valid=1, right_data=1125899906842624, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=158329674399744),
            Stimulus(left_valid=1, left_data=281474976710656, right_valid=1, right_data=2251799813685248, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=316659348799488),
            Stimulus(left_valid=1, left_data=562949953421312, right_valid=1, right_data=4503599627370496, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=633318697598976),
            Stimulus(left_valid=1, left_data=1125899906842624, right_valid=1, right_data=9007199254740992, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=1266637395197952),
            Stimulus(left_valid=1, left_data=2251799813685248, right_valid=1, right_data=18014398509481984, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=2533274790395904),
            Stimulus(left_valid=1, left_data=4503599627370496, right_valid=1, right_data=36028797018963968, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=5066549580791808),
            Stimulus(left_valid=1, left_data=9007199254740992, right_valid=1, right_data=72057594037927936, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=10133099161583616),
            Stimulus(left_valid=1, left_data=18014398509481984, right_valid=1, right_data=144115188075855872, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=20266198323167232),
            Stimulus(left_valid=1, left_data=36028797018963968, right_valid=1, right_data=288230376151711744, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=40532396646334464),
            Stimulus(left_valid=1, left_data=72057594037927936, right_valid=1, right_data=576460752303423488, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=81064793292668928),
            Stimulus(left_valid=1, left_data=144115188075855872, right_valid=1, right_data=1152921504606846976, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=162129586585337856),
            Stimulus(left_valid=1, left_data=288230376151711744, right_valid=1, right_data=2305843009213693952, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=324259173170675712),
            Stimulus(left_valid=1, left_data=576460752303423488, right_valid=1, right_data=4611686018427387904, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=648518346341351424),
            Stimulus(left_valid=1, left_data=1152921504606846976, right_valid=1, right_data=9223372036854775808, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=1297036692682702848),
            Stimulus(left_valid=1, left_data=2305843009213693952, right_valid=1, right_data=1, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=2594073385365405696),
            Stimulus(left_valid=1, left_data=4611686018427387904, right_valid=1, right_data=2, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=5188146770730811392),
            Stimulus(left_data=9223372036854775808, right_data=4, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=10376293541461622784),
            Stimulus(left_data=1, right_data=8, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=2305843009213693953),
            Stimulus(left_data=2, right_data=16, take=1, expected_left_ready=1, expected_right_ready=1, expected_valid=1, expected_data=4611686018427387906),
            Stimulus(left_data=4, right_data=32, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=8, right_data=64, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=16, right_data=128, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=32, right_data=256, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=64, right_data=512, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=128, right_data=1024, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=256, right_data=2048, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=512, right_data=4096, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=1024, right_data=8192, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=2048, right_data=16384, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=4096, right_data=32768, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=8192, right_data=65536, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=16384, right_data=131072, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=32768, right_data=262144, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=65536, right_data=524288, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=131072, right_data=1048576, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=262144, right_data=2097152, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=524288, right_data=4194304, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=1048576, right_data=8388608, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=2097152, right_data=16777216, take=1, expected_left_ready=1, expected_right_ready=1),
            Stimulus(left_data=4194304, right_data=33554432, take=1, expected_left_ready=1, expected_right_ready=1),
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
def MultiInputRulePipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    left_ready, right_ready, output = MultiInputRulePipeline(
        frame.left_valid,
        frame.left_data,
        frame.right_valid,
        frame.right_data,
        frame.take,
    )

    @rule
    def exercise():
        assert left_ready == frame.expected_left_ready, (
            "multi_input_rule_pipeline left_ready old-state check"
        )
        assert right_ready == frame.expected_right_ready, (
            "multi_input_rule_pipeline right_ready old-state check"
        )
        assert output.valid == frame.expected_valid, (
            "multi_input_rule_pipeline valid old-state check"
        )
        assert output.data == frame.expected_data, (
            "multi_input_rule_pipeline data old-state check"
        )
        log("info", "phase", phase)
        log("info", "left_ready", left_ready)
        log("info", "right_ready", right_ready)
        log("info", "valid", output.valid)
        log("info", "data", output.data)

    advance(phase)
    exercise()

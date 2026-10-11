"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_struct_pipeline.struct_pipeline import StructPipeline, Item, StructResult
from pycircuit import bits, enum_to_bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_value: bits[64]
    input_remaining: bits[64]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_value: bits[64]
    expected_remaining: bits[64]


@struct
class Stimuli:
    rows: table[185, Stimulus]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, input_remaining=9223372036854775808, expected_ready=1),
            Stimulus(valid=1, input_value=18446744073709551615, input_remaining=9223372036854775807, expected_ready=1),
            Stimulus(valid=1, input_value=18446744073709551614, input_remaining=12297829382473034410, expected_ready=1, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, input_value=9223372036854775808, input_remaining=6148914691236517205, expected_ready=1, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, input_value=9223372036854775807, input_remaining=1, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, input_value=12297829382473034410, input_remaining=2, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, input_value=6148914691236517205, input_remaining=4, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, input_value=1, input_remaining=8, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, input_value=2, input_remaining=16, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, input_value=4, input_remaining=32, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, input_value=8, input_remaining=64, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, input_value=16, input_remaining=128, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, input_value=32, input_remaining=256, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, take=1, input_value=64, input_remaining=512, expected_ready=1, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, take=1, input_value=128, input_remaining=1024, expected_ready=1, expected_valid=1, expected_remaining=9223372036854775806),
            Stimulus(valid=1, take=1, input_value=256, input_remaining=2048, expected_ready=1, expected_valid=1, expected_value=18446744073709551615, expected_remaining=12297829382473034409),
            Stimulus(valid=1, input_value=512, input_remaining=4096, expected_valid=1, expected_value=9223372036854775809, expected_remaining=6148914691236517204),
            Stimulus(valid=1, take=1, input_value=1024, input_remaining=8192, expected_ready=1, expected_valid=1, expected_value=9223372036854775809, expected_remaining=6148914691236517204),
            Stimulus(valid=1, take=1, input_value=2048, input_remaining=16384, expected_ready=1, expected_valid=1, expected_value=65, expected_remaining=511),
            Stimulus(valid=1, take=1, input_value=4096, input_remaining=32768, expected_ready=1, expected_valid=1, expected_value=129, expected_remaining=1023),
            Stimulus(valid=1, input_value=8192, input_remaining=65536, expected_valid=1, expected_value=257, expected_remaining=2047),
            Stimulus(valid=1, take=1, input_value=16384, input_remaining=131072, expected_ready=1, expected_valid=1, expected_value=257, expected_remaining=2047),
            Stimulus(valid=1, take=1, input_value=32768, input_remaining=262144, expected_ready=1, expected_valid=1, expected_value=1025, expected_remaining=8191),
            Stimulus(valid=1, take=1, input_value=65536, input_remaining=524288, expected_ready=1, expected_valid=1, expected_value=2049, expected_remaining=16383),
            Stimulus(valid=1, input_value=131072, input_remaining=1048576, expected_valid=1, expected_value=4097, expected_remaining=32767),
            Stimulus(valid=1, take=1, input_value=262144, input_remaining=2097152, expected_ready=1, expected_valid=1, expected_value=4097, expected_remaining=32767),
            Stimulus(valid=1, take=1, input_value=524288, input_remaining=4194304, expected_ready=1, expected_valid=1, expected_value=16385, expected_remaining=131071),
            Stimulus(valid=1, take=1, input_value=1048576, input_remaining=8388608, expected_ready=1, expected_valid=1, expected_value=32769, expected_remaining=262143),
            Stimulus(valid=1, input_value=2097152, input_remaining=16777216, expected_valid=1, expected_value=65537, expected_remaining=524287),
            Stimulus(valid=1, take=1, input_value=4194304, input_remaining=33554432, expected_ready=1, expected_valid=1, expected_value=65537, expected_remaining=524287),
            Stimulus(valid=1, take=1, input_value=8388608, input_remaining=67108864, expected_ready=1, expected_valid=1, expected_value=262145, expected_remaining=2097151),
            Stimulus(take=1, input_value=16777216, input_remaining=134217728, expected_ready=1, expected_valid=1, expected_value=524289, expected_remaining=4194303),
            Stimulus(valid=1, input_value=33554432, input_remaining=268435456, expected_ready=1, expected_valid=1, expected_value=1048577, expected_remaining=8388607),
            Stimulus(valid=1, take=1, input_value=67108864, input_remaining=536870912, expected_ready=1, expected_valid=1, expected_value=1048577, expected_remaining=8388607),
            Stimulus(valid=1, take=1, input_value=134217728, input_remaining=1073741824, expected_ready=1, expected_valid=1, expected_value=4194305, expected_remaining=33554431),
            Stimulus(valid=1, take=1, input_value=268435456, input_remaining=2147483648, expected_ready=1, expected_valid=1, expected_value=8388609, expected_remaining=67108863),
            Stimulus(input_value=536870912, input_remaining=4294967296, expected_valid=1, expected_value=33554433, expected_remaining=268435455),
            Stimulus(valid=1, take=1, input_value=1073741824, input_remaining=8589934592, expected_ready=1, expected_valid=1, expected_value=33554433, expected_remaining=268435455),
            Stimulus(valid=1, take=1, input_value=2147483648, input_remaining=17179869184, expected_ready=1, expected_valid=1, expected_value=67108865, expected_remaining=536870911),
            Stimulus(valid=1, take=1, input_value=4294967296, input_remaining=34359738368, expected_ready=1, expected_valid=1, expected_value=134217729, expected_remaining=1073741823),
            Stimulus(valid=1, input_value=8589934592, input_remaining=68719476736, expected_valid=1, expected_value=268435457, expected_remaining=2147483647),
            Stimulus(take=1, input_value=17179869184, input_remaining=137438953472, expected_ready=1, expected_valid=1, expected_value=268435457, expected_remaining=2147483647),
            Stimulus(valid=1, take=1, input_value=34359738368, input_remaining=274877906944, expected_ready=1, expected_valid=1, expected_value=1073741825, expected_remaining=8589934591),
            Stimulus(valid=1, take=1, input_value=68719476736, input_remaining=549755813888, expected_ready=1, expected_valid=1, expected_value=2147483649, expected_remaining=17179869183),
            Stimulus(valid=1, input_value=137438953472, input_remaining=1099511627776, expected_ready=1, expected_valid=1, expected_value=4294967297, expected_remaining=34359738367),
            Stimulus(valid=1, take=1, input_value=274877906944, input_remaining=2199023255552, expected_ready=1, expected_valid=1, expected_value=4294967297, expected_remaining=34359738367),
            Stimulus(take=1, input_value=549755813888, input_remaining=4398046511104, expected_ready=1, expected_valid=1, expected_value=34359738369, expected_remaining=274877906943),
            Stimulus(valid=1, take=1, input_value=1099511627776, input_remaining=8796093022208, expected_ready=1, expected_valid=1, expected_value=68719476737, expected_remaining=549755813887),
            Stimulus(valid=1, input_value=2199023255552, input_remaining=17592186044416, expected_ready=1, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=4398046511104, input_remaining=35184372088832, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=8796093022208, input_remaining=70368744177664, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(input_value=17592186044416, input_remaining=140737488355328, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=35184372088832, input_remaining=281474976710656, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=70368744177664, input_remaining=562949953421312, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=140737488355328, input_remaining=1125899906842624, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=281474976710656, input_remaining=2251799813685248, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(input_value=562949953421312, input_remaining=4503599627370496, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=1125899906842624, input_remaining=9007199254740992, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=2251799813685248, input_remaining=18014398509481984, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=4503599627370496, input_remaining=36028797018963968, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=9007199254740992, input_remaining=72057594037927936, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(input_value=18014398509481984, input_remaining=144115188075855872, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=36028797018963968, input_remaining=288230376151711744, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, input_value=72057594037927936, input_remaining=576460752303423488, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, take=1, input_remaining=9223372036854775808, expected_ready=1, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, take=1, input_value=18446744073709551615, input_remaining=9223372036854775807, expected_ready=1, expected_valid=1, expected_value=274877906945, expected_remaining=2199023255551),
            Stimulus(valid=1, take=1, input_value=18446744073709551614, input_remaining=12297829382473034410, expected_ready=1, expected_valid=1, expected_value=1099511627777, expected_remaining=8796093022207),
            Stimulus(valid=1, take=1, input_value=9223372036854775808, input_remaining=6148914691236517205, expected_ready=1, expected_valid=1, expected_value=2199023255553, expected_remaining=17592186044415),
            Stimulus(valid=1, take=1, input_value=9223372036854775807, input_remaining=1, expected_ready=1, expected_valid=1, expected_value=1, expected_remaining=9223372036854775807),
            Stimulus(valid=1, take=1, input_value=12297829382473034410, input_remaining=2, expected_ready=1, expected_valid=1, expected_remaining=9223372036854775806),
            Stimulus(valid=1, take=1, input_value=6148914691236517205, input_remaining=4, expected_ready=1, expected_valid=1, expected_value=18446744073709551615, expected_remaining=12297829382473034409),
            Stimulus(valid=1, take=1, input_value=1, input_remaining=8, expected_ready=1, expected_valid=1, expected_value=9223372036854775809, expected_remaining=6148914691236517204),
            Stimulus(valid=1, take=1, input_value=2, input_remaining=16, expected_ready=1, expected_valid=1, expected_value=9223372036854775808),
            Stimulus(valid=1, take=1, input_value=4, input_remaining=32, expected_ready=1, expected_valid=1, expected_value=12297829382473034411, expected_remaining=1),
            Stimulus(valid=1, take=1, input_value=8, input_remaining=64, expected_ready=1, expected_valid=1, expected_value=6148914691236517206, expected_remaining=3),
            Stimulus(valid=1, take=1, input_value=16, input_remaining=128, expected_ready=1, expected_valid=1, expected_value=2, expected_remaining=7),
            Stimulus(valid=1, take=1, input_value=32, input_remaining=256, expected_ready=1, expected_valid=1, expected_value=3, expected_remaining=15),
            Stimulus(valid=1, take=1, input_value=64, input_remaining=512, expected_ready=1, expected_valid=1, expected_value=5, expected_remaining=31),
            Stimulus(valid=1, take=1, input_value=128, input_remaining=1024, expected_ready=1, expected_valid=1, expected_value=9, expected_remaining=63),
            Stimulus(valid=1, take=1, input_value=256, input_remaining=2048, expected_ready=1, expected_valid=1, expected_value=17, expected_remaining=127),
            Stimulus(valid=1, take=1, input_value=512, input_remaining=4096, expected_ready=1, expected_valid=1, expected_value=33, expected_remaining=255),
            Stimulus(valid=1, take=1, input_value=1024, input_remaining=8192, expected_ready=1, expected_valid=1, expected_value=65, expected_remaining=511),
            Stimulus(valid=1, take=1, input_value=2048, input_remaining=16384, expected_ready=1, expected_valid=1, expected_value=129, expected_remaining=1023),
            Stimulus(valid=1, take=1, input_value=4096, input_remaining=32768, expected_ready=1, expected_valid=1, expected_value=257, expected_remaining=2047),
            Stimulus(valid=1, take=1, input_value=8192, input_remaining=65536, expected_ready=1, expected_valid=1, expected_value=513, expected_remaining=4095),
            Stimulus(valid=1, take=1, input_value=16384, input_remaining=131072, expected_ready=1, expected_valid=1, expected_value=1025, expected_remaining=8191),
            Stimulus(valid=1, take=1, input_value=32768, input_remaining=262144, expected_ready=1, expected_valid=1, expected_value=2049, expected_remaining=16383),
            Stimulus(valid=1, take=1, input_value=65536, input_remaining=524288, expected_ready=1, expected_valid=1, expected_value=4097, expected_remaining=32767),
            Stimulus(valid=1, take=1, input_value=131072, input_remaining=1048576, expected_ready=1, expected_valid=1, expected_value=8193, expected_remaining=65535),
            Stimulus(valid=1, take=1, input_value=262144, input_remaining=2097152, expected_ready=1, expected_valid=1, expected_value=16385, expected_remaining=131071),
            Stimulus(valid=1, take=1, input_value=524288, input_remaining=4194304, expected_ready=1, expected_valid=1, expected_value=32769, expected_remaining=262143),
            Stimulus(valid=1, take=1, input_value=1048576, input_remaining=8388608, expected_ready=1, expected_valid=1, expected_value=65537, expected_remaining=524287),
            Stimulus(valid=1, take=1, input_value=2097152, input_remaining=16777216, expected_ready=1, expected_valid=1, expected_value=131073, expected_remaining=1048575),
            Stimulus(valid=1, take=1, input_value=4194304, input_remaining=33554432, expected_ready=1, expected_valid=1, expected_value=262145, expected_remaining=2097151),
            Stimulus(valid=1, take=1, input_value=8388608, input_remaining=67108864, expected_ready=1, expected_valid=1, expected_value=524289, expected_remaining=4194303),
            Stimulus(valid=1, take=1, input_value=16777216, input_remaining=134217728, expected_ready=1, expected_valid=1, expected_value=1048577, expected_remaining=8388607),
            Stimulus(valid=1, take=1, input_value=33554432, input_remaining=268435456, expected_ready=1, expected_valid=1, expected_value=2097153, expected_remaining=16777215),
            Stimulus(valid=1, take=1, input_value=67108864, input_remaining=536870912, expected_ready=1, expected_valid=1, expected_value=4194305, expected_remaining=33554431),
            Stimulus(valid=1, take=1, input_value=134217728, input_remaining=1073741824, expected_ready=1, expected_valid=1, expected_value=8388609, expected_remaining=67108863),
            Stimulus(valid=1, take=1, input_value=268435456, input_remaining=2147483648, expected_ready=1, expected_valid=1, expected_value=16777217, expected_remaining=134217727),
            Stimulus(valid=1, take=1, input_value=536870912, input_remaining=4294967296, expected_ready=1, expected_valid=1, expected_value=33554433, expected_remaining=268435455),
            Stimulus(valid=1, take=1, input_value=1073741824, input_remaining=8589934592, expected_ready=1, expected_valid=1, expected_value=67108865, expected_remaining=536870911),
            Stimulus(valid=1, take=1, input_value=2147483648, input_remaining=17179869184, expected_ready=1, expected_valid=1, expected_value=134217729, expected_remaining=1073741823),
            Stimulus(valid=1, take=1, input_value=4294967296, input_remaining=34359738368, expected_ready=1, expected_valid=1, expected_value=268435457, expected_remaining=2147483647),
            Stimulus(valid=1, take=1, input_value=8589934592, input_remaining=68719476736, expected_ready=1, expected_valid=1, expected_value=536870913, expected_remaining=4294967295),
            Stimulus(valid=1, take=1, input_value=17179869184, input_remaining=137438953472, expected_ready=1, expected_valid=1, expected_value=1073741825, expected_remaining=8589934591),
            Stimulus(valid=1, take=1, input_value=34359738368, input_remaining=274877906944, expected_ready=1, expected_valid=1, expected_value=2147483649, expected_remaining=17179869183),
            Stimulus(valid=1, take=1, input_value=68719476736, input_remaining=549755813888, expected_ready=1, expected_valid=1, expected_value=4294967297, expected_remaining=34359738367),
            Stimulus(valid=1, take=1, input_value=137438953472, input_remaining=1099511627776, expected_ready=1, expected_valid=1, expected_value=8589934593, expected_remaining=68719476735),
            Stimulus(valid=1, take=1, input_value=274877906944, input_remaining=2199023255552, expected_ready=1, expected_valid=1, expected_value=17179869185, expected_remaining=137438953471),
            Stimulus(valid=1, take=1, input_value=549755813888, input_remaining=4398046511104, expected_ready=1, expected_valid=1, expected_value=34359738369, expected_remaining=274877906943),
            Stimulus(valid=1, take=1, input_value=1099511627776, input_remaining=8796093022208, expected_ready=1, expected_valid=1, expected_value=68719476737, expected_remaining=549755813887),
            Stimulus(valid=1, take=1, input_value=2199023255552, input_remaining=17592186044416, expected_ready=1, expected_valid=1, expected_value=137438953473, expected_remaining=1099511627775),
            Stimulus(valid=1, take=1, input_value=4398046511104, input_remaining=35184372088832, expected_ready=1, expected_valid=1, expected_value=274877906945, expected_remaining=2199023255551),
            Stimulus(valid=1, take=1, input_value=8796093022208, input_remaining=70368744177664, expected_ready=1, expected_valid=1, expected_value=549755813889, expected_remaining=4398046511103),
            Stimulus(valid=1, take=1, input_value=17592186044416, input_remaining=140737488355328, expected_ready=1, expected_valid=1, expected_value=1099511627777, expected_remaining=8796093022207),
            Stimulus(valid=1, take=1, input_value=35184372088832, input_remaining=281474976710656, expected_ready=1, expected_valid=1, expected_value=2199023255553, expected_remaining=17592186044415),
            Stimulus(valid=1, take=1, input_value=70368744177664, input_remaining=562949953421312, expected_ready=1, expected_valid=1, expected_value=4398046511105, expected_remaining=35184372088831),
            Stimulus(valid=1, take=1, input_value=140737488355328, input_remaining=1125899906842624, expected_ready=1, expected_valid=1, expected_value=8796093022209, expected_remaining=70368744177663),
            Stimulus(valid=1, take=1, input_value=281474976710656, input_remaining=2251799813685248, expected_ready=1, expected_valid=1, expected_value=17592186044417, expected_remaining=140737488355327),
            Stimulus(valid=1, take=1, input_value=562949953421312, input_remaining=4503599627370496, expected_ready=1, expected_valid=1, expected_value=35184372088833, expected_remaining=281474976710655),
            Stimulus(valid=1, take=1, input_value=1125899906842624, input_remaining=9007199254740992, expected_ready=1, expected_valid=1, expected_value=70368744177665, expected_remaining=562949953421311),
            Stimulus(valid=1, take=1, input_value=2251799813685248, input_remaining=18014398509481984, expected_ready=1, expected_valid=1, expected_value=140737488355329, expected_remaining=1125899906842623),
            Stimulus(valid=1, take=1, input_value=4503599627370496, input_remaining=36028797018963968, expected_ready=1, expected_valid=1, expected_value=281474976710657, expected_remaining=2251799813685247),
            Stimulus(valid=1, take=1, input_value=9007199254740992, input_remaining=72057594037927936, expected_ready=1, expected_valid=1, expected_value=562949953421313, expected_remaining=4503599627370495),
            Stimulus(valid=1, take=1, input_value=18014398509481984, input_remaining=144115188075855872, expected_ready=1, expected_valid=1, expected_value=1125899906842625, expected_remaining=9007199254740991),
            Stimulus(valid=1, take=1, input_value=36028797018963968, input_remaining=288230376151711744, expected_ready=1, expected_valid=1, expected_value=2251799813685249, expected_remaining=18014398509481983),
            Stimulus(valid=1, take=1, input_value=72057594037927936, input_remaining=576460752303423488, expected_ready=1, expected_valid=1, expected_value=4503599627370497, expected_remaining=36028797018963967),
            Stimulus(valid=1, take=1, input_value=144115188075855872, input_remaining=1152921504606846976, expected_ready=1, expected_valid=1, expected_value=9007199254740993, expected_remaining=72057594037927935),
            Stimulus(valid=1, take=1, input_value=288230376151711744, input_remaining=2305843009213693952, expected_ready=1, expected_valid=1, expected_value=18014398509481985, expected_remaining=144115188075855871),
            Stimulus(valid=1, take=1, input_value=576460752303423488, input_remaining=4611686018427387904, expected_ready=1, expected_valid=1, expected_value=36028797018963969, expected_remaining=288230376151711743),
            Stimulus(valid=1, take=1, input_value=1152921504606846976, input_remaining=9223372036854775808, expected_ready=1, expected_valid=1, expected_value=72057594037927937, expected_remaining=576460752303423487),
            Stimulus(valid=1, take=1, input_value=2305843009213693952, input_remaining=1, expected_ready=1, expected_valid=1, expected_value=144115188075855873, expected_remaining=1152921504606846975),
            Stimulus(valid=1, take=1, input_value=4611686018427387904, input_remaining=3, expected_ready=1, expected_valid=1, expected_value=288230376151711745, expected_remaining=2305843009213693951),
            Stimulus(valid=1, take=1, input_value=9223372036854775808, input_remaining=7, expected_ready=1, expected_valid=1, expected_value=576460752303423489, expected_remaining=4611686018427387903),
            Stimulus(valid=1, take=1, input_value=1, input_remaining=15, expected_ready=1, expected_valid=1, expected_value=1152921504606846977, expected_remaining=9223372036854775807),
            Stimulus(valid=1, take=1, input_value=3, input_remaining=31, expected_ready=1, expected_valid=1, expected_value=2305843009213693953),
            Stimulus(valid=1, take=1, input_value=7, input_remaining=63, expected_ready=1, expected_valid=1, expected_value=4611686018427387905, expected_remaining=2),
            Stimulus(valid=1, take=1, input_value=15, input_remaining=127, expected_ready=1, expected_valid=1, expected_value=9223372036854775809, expected_remaining=6),
            Stimulus(valid=1, take=1, input_value=31, input_remaining=255, expected_ready=1, expected_valid=1, expected_value=2, expected_remaining=14),
            Stimulus(valid=1, take=1, input_value=63, input_remaining=511, expected_ready=1, expected_valid=1, expected_value=4, expected_remaining=30),
            Stimulus(valid=1, take=1, input_value=127, input_remaining=1023, expected_ready=1, expected_valid=1, expected_value=8, expected_remaining=62),
            Stimulus(valid=1, take=1, input_value=255, input_remaining=2047, expected_ready=1, expected_valid=1, expected_value=16, expected_remaining=126),
            Stimulus(valid=1, take=1, input_value=511, input_remaining=4095, expected_ready=1, expected_valid=1, expected_value=32, expected_remaining=254),
            Stimulus(valid=1, take=1, input_value=1023, input_remaining=8191, expected_ready=1, expected_valid=1, expected_value=64, expected_remaining=510),
            Stimulus(valid=1, take=1, input_value=2047, input_remaining=16383, expected_ready=1, expected_valid=1, expected_value=128, expected_remaining=1022),
            Stimulus(valid=1, take=1, input_value=4095, input_remaining=32767, expected_ready=1, expected_valid=1, expected_value=256, expected_remaining=2046),
            Stimulus(valid=1, take=1, input_value=8191, input_remaining=65535, expected_ready=1, expected_valid=1, expected_value=512, expected_remaining=4094),
            Stimulus(valid=1, take=1, input_value=16383, input_remaining=131071, expected_ready=1, expected_valid=1, expected_value=1024, expected_remaining=8190),
            Stimulus(valid=1, take=1, input_value=32767, input_remaining=262143, expected_ready=1, expected_valid=1, expected_value=2048, expected_remaining=16382),
            Stimulus(valid=1, take=1, input_value=65535, input_remaining=524287, expected_ready=1, expected_valid=1, expected_value=4096, expected_remaining=32766),
            Stimulus(valid=1, take=1, input_value=131071, input_remaining=1048575, expected_ready=1, expected_valid=1, expected_value=8192, expected_remaining=65534),
            Stimulus(valid=1, take=1, input_value=262143, input_remaining=2097151, expected_ready=1, expected_valid=1, expected_value=16384, expected_remaining=131070),
            Stimulus(valid=1, take=1, input_value=524287, input_remaining=4194303, expected_ready=1, expected_valid=1, expected_value=32768, expected_remaining=262142),
            Stimulus(valid=1, take=1, input_value=1048575, input_remaining=8388607, expected_ready=1, expected_valid=1, expected_value=65536, expected_remaining=524286),
            Stimulus(valid=1, take=1, input_value=2097151, input_remaining=16777215, expected_ready=1, expected_valid=1, expected_value=131072, expected_remaining=1048574),
            Stimulus(valid=1, take=1, input_value=4194303, input_remaining=33554431, expected_ready=1, expected_valid=1, expected_value=262144, expected_remaining=2097150),
            Stimulus(valid=1, take=1, input_value=8388607, input_remaining=67108863, expected_ready=1, expected_valid=1, expected_value=524288, expected_remaining=4194302),
            Stimulus(valid=1, take=1, input_value=16777215, input_remaining=134217727, expected_ready=1, expected_valid=1, expected_value=1048576, expected_remaining=8388606),
            Stimulus(valid=1, take=1, input_value=33554431, input_remaining=268435455, expected_ready=1, expected_valid=1, expected_value=2097152, expected_remaining=16777214),
            Stimulus(take=1, input_value=67108863, input_remaining=536870911, expected_ready=1, expected_valid=1, expected_value=4194304, expected_remaining=33554430),
            Stimulus(take=1, input_value=134217727, input_remaining=1073741823, expected_ready=1, expected_valid=1, expected_value=8388608, expected_remaining=67108862),
            Stimulus(take=1, input_value=268435455, input_remaining=2147483647, expected_ready=1, expected_valid=1, expected_value=16777216, expected_remaining=134217726),
            Stimulus(take=1, input_value=536870911, input_remaining=4294967295, expected_ready=1, expected_valid=1, expected_value=33554432, expected_remaining=268435454),
            Stimulus(take=1, input_value=1073741823, input_remaining=8589934591, expected_ready=1),
            Stimulus(take=1, input_value=2147483647, input_remaining=17179869183, expected_ready=1),
            Stimulus(take=1, input_value=4294967295, input_remaining=34359738367, expected_ready=1),
            Stimulus(take=1, input_value=8589934591, input_remaining=68719476735, expected_ready=1),
            Stimulus(take=1, input_value=17179869183, input_remaining=137438953471, expected_ready=1),
            Stimulus(take=1, input_value=34359738367, input_remaining=274877906943, expected_ready=1),
            Stimulus(take=1, input_value=68719476735, input_remaining=549755813887, expected_ready=1),
            Stimulus(take=1, input_value=137438953471, input_remaining=1099511627775, expected_ready=1),
            Stimulus(take=1, input_value=274877906943, input_remaining=2199023255551, expected_ready=1),
            Stimulus(take=1, input_value=549755813887, input_remaining=4398046511103, expected_ready=1),
            Stimulus(take=1, input_value=1099511627775, input_remaining=8796093022207, expected_ready=1),
            Stimulus(take=1, input_value=2199023255551, input_remaining=17592186044415, expected_ready=1),
            Stimulus(take=1, input_value=4398046511103, input_remaining=35184372088831, expected_ready=1),
            Stimulus(take=1, input_value=8796093022207, input_remaining=70368744177663, expected_ready=1),
            Stimulus(take=1, input_value=17592186044415, input_remaining=140737488355327, expected_ready=1),
            Stimulus(take=1, input_value=35184372088831, input_remaining=281474976710655, expected_ready=1),
            Stimulus(take=1, input_value=70368744177663, input_remaining=562949953421311, expected_ready=1),
            Stimulus(take=1, input_value=140737488355327, input_remaining=1125899906842623, expected_ready=1),
            Stimulus(take=1, input_value=281474976710655, input_remaining=2251799813685247, expected_ready=1),
            Stimulus(take=1, input_value=562949953421311, input_remaining=4503599627370495, expected_ready=1),
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
def StructPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Item(value=frame.input_value, remaining=frame.input_remaining)
    dut = StructPipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "struct_pipeline input capacity"
        assert dut.valid == frame.expected_valid, "struct_pipeline result availability"
        assert (
            dut.data.value == frame.expected_value
        ), "struct_pipeline value old-state check"
        assert (
            dut.data.remaining == frame.expected_remaining
        ), "struct_pipeline remaining old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "value", dut.data.value)
        log("info", "remaining", dut.data.remaining)

    advance(phase)
    check()

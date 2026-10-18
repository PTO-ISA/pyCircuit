"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_conditional_pipeline.conditional_pipeline import (
    ConditionalPipeline,
    Item,
    ConditionalResult,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_value: bits[32]
    input_route: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_value: bits[32]
    expected_data_route: bits[1]


@struct
class Stimuli:
    rows: table[185, Stimulus]


def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, input_route=1, expected_ready=1),
            Stimulus(valid=1, input_value=4294967295, expected_ready=1),
            Stimulus(valid=1, input_value=4294967294, input_route=1, expected_ready=1),
            Stimulus(valid=1, input_value=2147483648, expected_ready=1),
            Stimulus(valid=1, input_value=2147483647, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, input_value=2863311530, expected_ready=1, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, input_value=1431655765, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, input_value=1, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, input_value=2, input_route=1, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, input_value=4, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, input_value=8, input_route=1, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, input_value=16, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, input_value=32, input_route=1, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=64, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=128, input_route=1, expected_valid=1, expected_data_value=18, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=256, expected_ready=1, expected_valid=1, expected_data_value=2147483667, expected_data_route=1),
            Stimulus(valid=1, input_value=512, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=9),
            Stimulus(valid=1, take=1, input_value=1024, expected_ready=1, expected_valid=1, expected_data_value=9),
            Stimulus(valid=1, take=1, input_value=2048, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=2147483658),
            Stimulus(valid=1, take=1, input_value=4096, expected_ready=1, expected_valid=1, expected_data_value=1431655785, expected_data_route=1),
            Stimulus(valid=1, input_value=8192, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=2863311540),
            Stimulus(valid=1, take=1, input_value=16384, expected_valid=1, expected_data_value=2863311540),
            Stimulus(valid=1, take=1, input_value=32768, input_route=1, expected_valid=1, expected_data_value=532, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=65536, expected_ready=1, expected_valid=1, expected_data_value=2068, expected_data_route=1),
            Stimulus(valid=1, input_value=131072, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=266),
            Stimulus(valid=1, take=1, input_value=262144, expected_ready=1, expected_valid=1, expected_data_value=266),
            Stimulus(valid=1, take=1, input_value=524288, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=1034),
            Stimulus(valid=1, take=1, input_value=1048576, expected_ready=1, expected_valid=1, expected_data_value=8212, expected_data_route=1),
            Stimulus(valid=1, input_value=2097152, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=4106),
            Stimulus(valid=1, take=1, input_value=4194304, expected_valid=1, expected_data_value=4106),
            Stimulus(valid=1, take=1, input_value=8388608, input_route=1, expected_valid=1, expected_data_value=131092, expected_data_route=1),
            Stimulus(take=1, input_value=16777216, expected_ready=1, expected_valid=1, expected_data_value=524308, expected_data_route=1),
            Stimulus(valid=1, input_value=33554432, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=65546),
            Stimulus(valid=1, take=1, input_value=67108864, expected_ready=1, expected_valid=1, expected_data_value=65546),
            Stimulus(valid=1, take=1, input_value=134217728, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=262154),
            Stimulus(valid=1, take=1, input_value=268435456, expected_ready=1, expected_valid=1, expected_data_value=2097172, expected_data_route=1),
            Stimulus(input_value=536870912, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=33554452, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1073741824, expected_ready=1, expected_valid=1, expected_data_value=33554452, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=2147483648, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=134217748, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1, expected_ready=1, expected_valid=1, expected_data_value=1048586),
            Stimulus(valid=1, input_value=3, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=67108874),
            Stimulus(take=1, input_value=7, expected_ready=1, expected_valid=1, expected_data_value=67108874),
            Stimulus(valid=1, take=1, input_value=15, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=268435466),
            Stimulus(valid=1, take=1, input_value=31, expected_ready=1, expected_valid=1, expected_data_value=2147483668, expected_data_route=1),
            Stimulus(valid=1, input_value=63, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=1073741834),
            Stimulus(valid=1, take=1, input_value=127, expected_ready=1, expected_valid=1, expected_data_value=1073741834),
            Stimulus(take=1, input_value=255, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=23, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=511, expected_ready=1, expected_valid=1, expected_data_value=35, expected_data_route=1),
            Stimulus(valid=1, input_value=1023, input_route=1, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=2047, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=4095, input_route=1, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(input_value=8191, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=16383, input_route=1, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=32767, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=65535, input_route=1, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=131071, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(input_value=262143, input_route=1, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=524287, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=1048575, input_route=1, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=2097151, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=4194303, input_route=1, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(input_value=8388607, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=16777215, input_route=1, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, input_value=33554431, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, take=1, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=4294967295, expected_ready=1, expected_valid=1, expected_data_value=11),
            Stimulus(valid=1, take=1, input_value=4294967294, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=41),
            Stimulus(valid=1, take=1, input_value=2147483648, expected_ready=1, expected_valid=1, expected_data_value=137),
            Stimulus(valid=1, take=1, input_value=2147483647, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=521),
            Stimulus(valid=1, take=1, input_value=2863311530, expected_ready=1, expected_valid=1, expected_data_value=20, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1431655765, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=9),
            Stimulus(valid=1, take=1, input_value=1, expected_ready=1, expected_valid=1, expected_data_value=18, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=2, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=2147483658),
            Stimulus(valid=1, take=1, input_value=4, expected_ready=1, expected_valid=1, expected_data_value=2147483667, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=8, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=2863311540),
            Stimulus(valid=1, take=1, input_value=16, expected_ready=1, expected_valid=1, expected_data_value=1431655785, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=32, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=11),
            Stimulus(valid=1, take=1, input_value=64, expected_ready=1, expected_valid=1, expected_data_value=22, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=128, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=14),
            Stimulus(valid=1, take=1, input_value=256, expected_ready=1, expected_valid=1, expected_data_value=28, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=512, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=26),
            Stimulus(valid=1, take=1, input_value=1024, expected_ready=1, expected_valid=1, expected_data_value=52, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=2048, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=74),
            Stimulus(valid=1, take=1, input_value=4096, expected_ready=1, expected_valid=1, expected_data_value=148, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=8192, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=266),
            Stimulus(valid=1, take=1, input_value=16384, expected_ready=1, expected_valid=1, expected_data_value=532, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=32768, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=1034),
            Stimulus(valid=1, take=1, input_value=65536, expected_ready=1, expected_valid=1, expected_data_value=2068, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=131072, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=4106),
            Stimulus(valid=1, take=1, input_value=262144, expected_ready=1, expected_valid=1, expected_data_value=8212, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=524288, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=16394),
            Stimulus(valid=1, take=1, input_value=1048576, expected_ready=1, expected_valid=1, expected_data_value=32788, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=2097152, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=65546),
            Stimulus(valid=1, take=1, input_value=4194304, expected_ready=1, expected_valid=1, expected_data_value=131092, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=8388608, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=262154),
            Stimulus(valid=1, take=1, input_value=16777216, expected_ready=1, expected_valid=1, expected_data_value=524308, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=33554432, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=1048586),
            Stimulus(valid=1, take=1, input_value=67108864, expected_ready=1, expected_valid=1, expected_data_value=2097172, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=134217728, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=4194314),
            Stimulus(valid=1, take=1, input_value=268435456, expected_ready=1, expected_valid=1, expected_data_value=8388628, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=536870912, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=16777226),
            Stimulus(valid=1, take=1, input_value=1073741824, expected_ready=1, expected_valid=1, expected_data_value=33554452, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=2147483648, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=67108874),
            Stimulus(valid=1, take=1, input_value=1, expected_ready=1, expected_valid=1, expected_data_value=134217748, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=3, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=268435466),
            Stimulus(valid=1, take=1, input_value=7, expected_ready=1, expected_valid=1, expected_data_value=536870932, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=15, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=1073741834),
            Stimulus(valid=1, take=1, input_value=31, expected_ready=1, expected_valid=1, expected_data_value=2147483668, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=63, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=11),
            Stimulus(valid=1, take=1, input_value=127, expected_ready=1, expected_valid=1, expected_data_value=23, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=255, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=17),
            Stimulus(valid=1, take=1, input_value=511, expected_ready=1, expected_valid=1, expected_data_value=35, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1023, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=41),
            Stimulus(valid=1, take=1, input_value=2047, expected_ready=1, expected_valid=1, expected_data_value=83, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=4095, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=137),
            Stimulus(valid=1, take=1, input_value=8191, expected_ready=1, expected_valid=1, expected_data_value=275, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=16383, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=521),
            Stimulus(valid=1, take=1, input_value=32767, expected_ready=1, expected_valid=1, expected_data_value=1043, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=65535, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=2057),
            Stimulus(valid=1, take=1, input_value=131071, expected_ready=1, expected_valid=1, expected_data_value=4115, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=262143, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=8201),
            Stimulus(valid=1, take=1, input_value=524287, expected_ready=1, expected_valid=1, expected_data_value=16403, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1048575, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=32777),
            Stimulus(valid=1, take=1, input_value=2097151, expected_ready=1, expected_valid=1, expected_data_value=65555, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=4194303, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=131081),
            Stimulus(valid=1, take=1, input_value=8388607, expected_ready=1, expected_valid=1, expected_data_value=262163, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=16777215, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=524297),
            Stimulus(valid=1, take=1, input_value=33554431, expected_ready=1, expected_valid=1, expected_data_value=1048595, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=67108863, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=2097161),
            Stimulus(valid=1, take=1, input_value=134217727, expected_ready=1, expected_valid=1, expected_data_value=4194323, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=268435455, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=8388617),
            Stimulus(valid=1, take=1, input_value=536870911, expected_ready=1, expected_valid=1, expected_data_value=16777235, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1073741823, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=33554441),
            Stimulus(valid=1, take=1, input_value=2147483647, expected_ready=1, expected_valid=1, expected_data_value=67108883, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=4294967295, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=134217737),
            Stimulus(valid=1, take=1, input_value=2991959596, expected_ready=1, expected_valid=1, expected_data_value=268435475, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=856644119, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=536870921),
            Stimulus(valid=1, take=1, input_value=3015759362, expected_ready=1, expected_valid=1, expected_data_value=1073741843, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=880439789, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=2147483657),
            Stimulus(valid=1, take=1, input_value=3039821272, expected_ready=1, expected_valid=1, expected_data_value=19, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=903956931, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=2991959606),
            Stimulus(valid=1, take=1, input_value=3063596462, expected_ready=1, expected_valid=1, expected_data_value=856644139, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=927744409, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3015759372),
            Stimulus(valid=1, take=1, input_value=3087125892, expected_ready=1, expected_valid=1, expected_data_value=880439809, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=951826799, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3039821282),
            Stimulus(valid=1, take=1, input_value=3110876506, expected_ready=1, expected_valid=1, expected_data_value=903956951, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=975622469, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3063596472),
            Stimulus(valid=1, take=1, input_value=3134938416, expected_ready=1, expected_valid=1, expected_data_value=927744429, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=999155995, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3087125902),
            Stimulus(valid=1, take=1, input_value=3158729990, expected_ready=1, expected_valid=1, expected_data_value=951826819, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1022943473, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3110876516),
            Stimulus(valid=1, take=1, input_value=3182259420, expected_ready=1, expected_valid=1, expected_data_value=975622489, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1046976711, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3134938426),
            Stimulus(valid=1, take=1, input_value=3206026418, expected_ready=1, expected_valid=1, expected_data_value=999156015, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1070772381, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3158730000),
            Stimulus(valid=1, take=1, input_value=3230088328, expected_ready=1, expected_valid=1, expected_data_value=1022943493, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1094289523, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3182259430),
            Stimulus(valid=1, take=1, input_value=3253863518, expected_ready=1, expected_valid=1, expected_data_value=1046976731, expected_data_route=1),
            Stimulus(valid=1, take=1, input_value=1118077001, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3206026428),
            Stimulus(valid=1, take=1, input_value=3277392948, expected_ready=1, expected_valid=1, expected_data_value=1070772401, expected_data_route=1),
            Stimulus(take=1, input_value=1142061087, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3230088338),
            Stimulus(take=1, input_value=3301307402, expected_ready=1, expected_valid=1, expected_data_value=1094289543, expected_data_route=1),
            Stimulus(take=1, input_value=1165979637, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3253863528),
            Stimulus(take=1, input_value=3325107168, expected_ready=1, expected_valid=1, expected_data_value=1118077021, expected_data_route=1),
            Stimulus(take=1, input_value=1189390283, input_route=1, expected_ready=1, expected_valid=1, expected_data_value=3277392958),
            Stimulus(take=1, input_value=3349029814, expected_ready=1),
            Stimulus(take=1, input_value=1213317025, input_route=1, expected_ready=1),
            Stimulus(take=1, input_value=3372952460, expected_ready=1),
            Stimulus(take=1, input_value=1237243767, input_route=1, expected_ready=1),
            Stimulus(take=1, input_value=3396359010, expected_ready=1),
            Stimulus(take=1, input_value=1261162317, input_route=1, expected_ready=1),
            Stimulus(take=1, input_value=3420289848, expected_ready=1),
            Stimulus(take=1, input_value=1284556579, input_route=1, expected_ready=1),
            Stimulus(take=1, input_value=3444196110, expected_ready=1),
            Stimulus(take=1, input_value=1308483321, input_route=1, expected_ready=1),
            Stimulus(take=1, input_value=3468118756, expected_ready=1),
            Stimulus(take=1, input_value=1332360911, input_route=1, expected_ready=1),
            Stimulus(take=1, input_value=3491541690, expected_ready=1),
            Stimulus(take=1, input_value=1356279461, input_route=1, expected_ready=1),
            Stimulus(take=1, input_value=3515472528, expected_ready=1),
            Stimulus(take=1, input_value=1379690107, input_route=1, expected_ready=1),
            Stimulus(take=1, input_value=3539395174, expected_ready=1),
            Stimulus(take=1, input_value=1403616849, input_route=1, expected_ready=1),
            Stimulus(take=1, input_value=3563317820, expected_ready=1),
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
def ConditionalPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Item(value=frame.input_value, route=frame.input_route)
    dut = ConditionalPipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "conditional_pipeline input capacity"
        assert dut.valid == frame.expected_valid, (
            "conditional_pipeline result availability"
        )
        assert dut.data.value == frame.expected_data_value, (
            "conditional_pipeline value old-state check"
        )
        assert dut.data.route == frame.expected_data_route, (
            "conditional_pipeline route old-state check"
        )
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "value", dut.data.value)
        log("info", "route", dut.data.route)

    advance(phase)
    check()

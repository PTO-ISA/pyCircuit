"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_bit_widths.bit_widths import BitWidths, MaskedTag, Result
from pycircuit import bits, enum_to_bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_value: bits[13]
    input_mask: bits[13]
    input_rotated: bits[13]
    input_sequence: bits[37]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_value: bits[13]
    expected_data_mask: bits[13]
    expected_data_rotated: bits[13]
    expected_data_sequence: bits[37]


@struct
class Stimuli:
    rows: table[185, Stimulus]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, input_mask=4096, input_rotated=2730, input_sequence=4, expected_ready=1),
            Stimulus(valid=1, input_value=8191, input_mask=4095, input_rotated=1, input_sequence=8, expected_ready=1),
            Stimulus(valid=1, input_value=8190, input_mask=5461, input_rotated=2, input_sequence=16, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, input_value=4096, input_mask=2730, input_rotated=4, input_sequence=32, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, input_value=4095, input_mask=1, input_rotated=8, input_sequence=64, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, input_value=5461, input_mask=2, input_rotated=16, input_sequence=128, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, input_value=2730, input_mask=4, input_rotated=32, input_sequence=256, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, input_value=1, input_mask=8, input_rotated=64, input_sequence=512, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, input_value=2, input_mask=16, input_rotated=128, input_sequence=1024, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, input_value=4, input_mask=32, input_rotated=256, input_sequence=2048, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, input_value=8, input_mask=64, input_rotated=512, input_sequence=4096, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, input_value=16, input_mask=128, input_rotated=1024, input_sequence=8192, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, input_value=32, input_mask=256, input_rotated=2048, input_sequence=16384, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, take=1, input_value=64, input_mask=512, input_rotated=4096, input_sequence=32768, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, take=1, input_value=128, input_mask=1024, input_rotated=1, input_sequence=65536, expected_ready=1, expected_valid=1, expected_data_value=4094, expected_data_mask=4095, expected_data_rotated=8191, expected_data_sequence=8),
            Stimulus(valid=1, take=1, input_value=256, input_mask=2048, input_rotated=3, input_sequence=131072, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=512, expected_data_rotated=128, expected_data_sequence=32768),
            Stimulus(valid=1, input_value=512, input_mask=4096, input_rotated=7, input_sequence=262144, expected_valid=1, expected_data_value=1, expected_data_mask=1024, expected_data_rotated=256, expected_data_sequence=65536),
            Stimulus(valid=1, take=1, input_value=1024, input_mask=1, input_rotated=15, input_sequence=524288, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=1024, expected_data_rotated=256, expected_data_sequence=65536),
            Stimulus(valid=1, take=1, input_value=2048, input_mask=3, input_rotated=31, input_sequence=1048576, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=2048, expected_data_rotated=512, expected_data_sequence=131072),
            Stimulus(valid=1, take=1, input_value=4096, input_mask=7, input_rotated=63, input_sequence=2097152, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=1, expected_data_rotated=2048, expected_data_sequence=524288),
            Stimulus(valid=1, input_value=1, input_mask=15, input_rotated=127, input_sequence=4194304, expected_valid=1, expected_data_value=1, expected_data_mask=3, expected_data_rotated=4096, expected_data_sequence=1048576),
            Stimulus(valid=1, take=1, input_value=3, input_mask=31, input_rotated=255, input_sequence=8388608, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=3, expected_data_rotated=4096, expected_data_sequence=1048576),
            Stimulus(valid=1, take=1, input_value=7, input_mask=63, input_rotated=511, input_sequence=16777216, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=7, expected_data_rotated=1, expected_data_sequence=2097152),
            Stimulus(valid=1, take=1, input_value=15, input_mask=127, input_rotated=1023, input_sequence=33554432, expected_ready=1, expected_valid=1, expected_data_value=2, expected_data_mask=31, expected_data_rotated=6, expected_data_sequence=8388608),
            Stimulus(valid=1, input_value=31, input_mask=255, input_rotated=2047, input_sequence=67108864, expected_valid=1, expected_data_value=6, expected_data_mask=63, expected_data_rotated=14, expected_data_sequence=16777216),
            Stimulus(valid=1, take=1, input_value=63, input_mask=511, input_rotated=4095, input_sequence=134217728, expected_ready=1, expected_valid=1, expected_data_value=6, expected_data_mask=63, expected_data_rotated=14, expected_data_sequence=16777216),
            Stimulus(valid=1, take=1, input_value=127, input_mask=1023, input_rotated=8191, input_sequence=268435456, expected_ready=1, expected_valid=1, expected_data_value=14, expected_data_mask=127, expected_data_rotated=30, expected_data_sequence=33554432),
            Stimulus(valid=1, take=1, input_value=255, input_mask=2047, input_rotated=2378, input_sequence=536870912, expected_ready=1, expected_valid=1, expected_data_value=62, expected_data_mask=511, expected_data_rotated=126, expected_data_sequence=134217728),
            Stimulus(valid=1, input_value=511, input_mask=4095, input_rotated=5429, input_sequence=1073741824, expected_valid=1, expected_data_value=126, expected_data_mask=1023, expected_data_rotated=254, expected_data_sequence=268435456),
            Stimulus(valid=1, take=1, input_value=1023, input_mask=8191, input_rotated=4384, input_sequence=2147483648, expected_ready=1, expected_valid=1, expected_data_value=126, expected_data_mask=1023, expected_data_rotated=254, expected_data_sequence=268435456),
            Stimulus(valid=1, take=1, input_value=2047, input_mask=2378, input_rotated=3339, input_sequence=4294967296, expected_ready=1, expected_valid=1, expected_data_value=254, expected_data_mask=2047, expected_data_rotated=510, expected_data_sequence=536870912),
            Stimulus(take=1, input_value=4095, input_mask=5429, input_rotated=6390, input_sequence=8589934592, expected_ready=1, expected_valid=1, expected_data_value=1022, expected_data_mask=8191, expected_data_rotated=2046, expected_data_sequence=2147483648),
            Stimulus(valid=1, input_value=8191, input_mask=4384, input_rotated=1249, input_sequence=17179869184, expected_ready=1, expected_valid=1, expected_data_value=331, expected_data_mask=2378, expected_data_rotated=4094, expected_data_sequence=4294967296),
            Stimulus(valid=1, take=1, input_value=2378, input_mask=3339, input_rotated=204, input_sequence=34359738368, expected_ready=1, expected_valid=1, expected_data_value=331, expected_data_mask=2378, expected_data_rotated=4094, expected_data_sequence=4294967296),
            Stimulus(valid=1, take=1, input_value=5429, input_mask=6390, input_rotated=7351, input_sequence=68719476736, expected_ready=1, expected_valid=1, expected_data_value=4385, expected_data_mask=4384, expected_data_rotated=8191, expected_data_sequence=17179869184),
            Stimulus(valid=1, take=1, input_value=4384, input_mask=1249, input_rotated=2210, input_sequence=1, expected_ready=1, expected_valid=1, expected_data_value=2315, expected_data_mask=3339, expected_data_rotated=4756, expected_data_sequence=34359738368),
            Stimulus(input_value=3339, input_mask=204, input_rotated=5261, input_sequence=3, expected_valid=1, expected_data_value=4149, expected_data_mask=6390, expected_data_rotated=2667, expected_data_sequence=68719476736),
            Stimulus(valid=1, take=1, input_value=6390, input_mask=7351, input_rotated=4216, input_sequence=7, expected_ready=1, expected_valid=1, expected_data_value=4149, expected_data_mask=6390, expected_data_rotated=2667, expected_data_sequence=68719476736),
            Stimulus(valid=1, take=1, input_value=1249, input_mask=2210, input_rotated=3171, input_sequence=15, expected_ready=1, expected_valid=1, expected_data_value=33, expected_data_mask=1249, expected_data_rotated=577, expected_data_sequence=1),
            Stimulus(valid=1, take=1, input_value=204, input_mask=5261, input_rotated=6222, input_sequence=31, expected_ready=1, expected_valid=1, expected_data_value=6327, expected_data_mask=7351, expected_data_rotated=4589, expected_data_sequence=7),
            Stimulus(valid=1, input_value=7351, input_mask=4216, input_rotated=1081, input_sequence=63, expected_valid=1, expected_data_value=161, expected_data_mask=2210, expected_data_rotated=2498, expected_data_sequence=15),
            Stimulus(take=1, input_value=2210, input_mask=3171, input_rotated=36, input_sequence=127, expected_ready=1, expected_valid=1, expected_data_value=161, expected_data_mask=2210, expected_data_rotated=2498, expected_data_sequence=15),
            Stimulus(valid=1, take=1, input_value=5261, input_mask=6222, input_rotated=7183, input_sequence=255, expected_ready=1, expected_valid=1, expected_data_value=141, expected_data_mask=5261, expected_data_rotated=408, expected_data_sequence=31),
            Stimulus(valid=1, take=1, input_value=4216, input_mask=1081, input_rotated=6138, input_sequence=511, expected_ready=1),
            Stimulus(valid=1, input_value=3171, input_mask=36, input_rotated=5093, input_sequence=1023, expected_valid=1, expected_data_value=4109, expected_data_mask=6222, expected_data_rotated=2331, expected_data_sequence=255),
            Stimulus(valid=1, take=1, input_value=6222, input_mask=7183, input_rotated=8144, input_sequence=2047, expected_ready=1, expected_valid=1, expected_data_value=4109, expected_data_mask=6222, expected_data_rotated=2331, expected_data_sequence=255),
            Stimulus(take=1, input_value=1081, input_mask=6138, input_rotated=3003, input_sequence=4095, expected_ready=1, expected_valid=1, expected_data_value=57, expected_data_mask=1081, expected_data_rotated=241, expected_data_sequence=511),
            Stimulus(valid=1, take=1, input_value=36, input_mask=5093, input_rotated=1958, input_sequence=8191, expected_ready=1, expected_valid=1, expected_data_value=6159, expected_data_mask=7183, expected_data_rotated=4253, expected_data_sequence=2047),
            Stimulus(valid=1, input_value=7183, input_mask=8144, input_rotated=913, input_sequence=16383, expected_ready=1),
            Stimulus(valid=1, input_value=6138, input_mask=3003, input_rotated=3964, input_sequence=32767, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=5093, input_mask=1958, input_rotated=7015, input_sequence=65535, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(input_value=8144, input_mask=913, input_rotated=5970, input_sequence=131071, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=3003, input_mask=3964, input_rotated=4925, input_sequence=262143, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=1958, input_mask=7015, input_rotated=7976, input_sequence=524287, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=913, input_mask=5970, input_rotated=2835, input_sequence=1048575, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=3964, input_mask=4925, input_rotated=1790, input_sequence=2097151, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(input_value=7015, input_mask=7976, input_rotated=745, input_sequence=4194303, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=5970, input_mask=2835, input_rotated=3796, input_sequence=8388607, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=4925, input_mask=1790, input_rotated=6847, input_sequence=16777215, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=7976, input_mask=745, input_rotated=5802, input_sequence=33554431, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=2835, input_mask=3796, input_rotated=4757, input_sequence=67108863, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(input_value=1790, input_mask=6847, input_rotated=7808, input_sequence=134217727, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=745, input_mask=5802, input_rotated=2667, input_sequence=268435455, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, input_value=3796, input_mask=4757, input_rotated=1622, input_sequence=536870911, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, take=1, input_mask=4096, input_rotated=2730, input_sequence=4, expected_ready=1, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, take=1, input_value=8191, input_mask=4095, input_rotated=1, input_sequence=8, expected_ready=1, expected_valid=1, expected_data_value=7169, expected_data_mask=8144, expected_data_rotated=6175, expected_data_sequence=16383),
            Stimulus(valid=1, take=1, input_value=8190, input_mask=5461, input_rotated=2, input_sequence=16, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_sequence=4),
            Stimulus(valid=1, take=1, input_value=4096, input_mask=2730, input_rotated=4, input_sequence=32, expected_ready=1, expected_valid=1, expected_data_value=4094, expected_data_mask=4095, expected_data_rotated=8191, expected_data_sequence=8),
            Stimulus(valid=1, take=1, input_value=4095, input_mask=1, input_rotated=8, input_sequence=64, expected_ready=1, expected_valid=1, expected_data_value=5461, expected_data_mask=5461, expected_data_rotated=8189, expected_data_sequence=16),
            Stimulus(valid=1, take=1, input_value=5461, input_mask=2, input_rotated=16, input_sequence=128, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=2730, expected_data_rotated=1, expected_data_sequence=32),
            Stimulus(valid=1, take=1, input_value=2730, input_mask=4, input_rotated=32, input_sequence=256, expected_ready=1, expected_valid=1, expected_data_mask=1, expected_data_rotated=8190, expected_data_sequence=64),
            Stimulus(valid=1, take=1, input_value=1, input_mask=8, input_rotated=64, input_sequence=512, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=2, expected_data_rotated=2731, expected_data_sequence=128),
            Stimulus(valid=1, take=1, input_value=2, input_mask=16, input_rotated=128, input_sequence=1024, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=4, expected_data_rotated=5460, expected_data_sequence=256),
            Stimulus(valid=1, take=1, input_value=4, input_mask=32, input_rotated=256, input_sequence=2048, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=8, expected_data_rotated=2, expected_data_sequence=512),
            Stimulus(valid=1, take=1, input_value=8, input_mask=64, input_rotated=512, input_sequence=4096, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=16, expected_data_rotated=4, expected_data_sequence=1024),
            Stimulus(valid=1, take=1, input_value=16, input_mask=128, input_rotated=1024, input_sequence=8192, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=32, expected_data_rotated=8, expected_data_sequence=2048),
            Stimulus(valid=1, take=1, input_value=32, input_mask=256, input_rotated=2048, input_sequence=16384, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=64, expected_data_rotated=16, expected_data_sequence=4096),
            Stimulus(valid=1, take=1, input_value=64, input_mask=512, input_rotated=4096, input_sequence=32768, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=128, expected_data_rotated=32, expected_data_sequence=8192),
            Stimulus(valid=1, take=1, input_value=128, input_mask=1024, input_rotated=1, input_sequence=65536, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=256, expected_data_rotated=64, expected_data_sequence=16384),
            Stimulus(valid=1, take=1, input_value=256, input_mask=2048, input_rotated=3, input_sequence=131072, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=512, expected_data_rotated=128, expected_data_sequence=32768),
            Stimulus(valid=1, take=1, input_value=512, input_mask=4096, input_rotated=7, input_sequence=262144, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=1024, expected_data_rotated=256, expected_data_sequence=65536),
            Stimulus(valid=1, take=1, input_value=1024, input_mask=1, input_rotated=15, input_sequence=524288, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=2048, expected_data_rotated=512, expected_data_sequence=131072),
            Stimulus(valid=1, take=1, input_value=2048, input_mask=3, input_rotated=31, input_sequence=1048576, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=4096, expected_data_rotated=1024, expected_data_sequence=262144),
            Stimulus(valid=1, take=1, input_value=4096, input_mask=7, input_rotated=63, input_sequence=2097152, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=1, expected_data_rotated=2048, expected_data_sequence=524288),
            Stimulus(valid=1, take=1, input_value=1, input_mask=15, input_rotated=127, input_sequence=4194304, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=3, expected_data_rotated=4096, expected_data_sequence=1048576),
            Stimulus(valid=1, take=1, input_value=3, input_mask=31, input_rotated=255, input_sequence=8388608, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_mask=7, expected_data_rotated=1, expected_data_sequence=2097152),
            Stimulus(valid=1, take=1, input_value=7, input_mask=63, input_rotated=511, input_sequence=16777216, expected_ready=1, expected_valid=1, expected_data_mask=15, expected_data_rotated=2, expected_data_sequence=4194304),
            Stimulus(valid=1, take=1, input_value=15, input_mask=127, input_rotated=1023, input_sequence=33554432, expected_ready=1, expected_valid=1, expected_data_value=2, expected_data_mask=31, expected_data_rotated=6, expected_data_sequence=8388608),
            Stimulus(valid=1, take=1, input_value=31, input_mask=255, input_rotated=2047, input_sequence=67108864, expected_ready=1, expected_valid=1, expected_data_value=6, expected_data_mask=63, expected_data_rotated=14, expected_data_sequence=16777216),
            Stimulus(valid=1, take=1, input_value=63, input_mask=511, input_rotated=4095, input_sequence=134217728, expected_ready=1, expected_valid=1, expected_data_value=14, expected_data_mask=127, expected_data_rotated=30, expected_data_sequence=33554432),
            Stimulus(valid=1, take=1, input_value=127, input_mask=1023, input_rotated=8191, input_sequence=268435456, expected_ready=1, expected_valid=1, expected_data_value=30, expected_data_mask=255, expected_data_rotated=62, expected_data_sequence=67108864),
            Stimulus(valid=1, take=1, input_value=255, input_mask=2047, input_rotated=2378, input_sequence=536870912, expected_ready=1, expected_valid=1, expected_data_value=62, expected_data_mask=511, expected_data_rotated=126, expected_data_sequence=134217728),
            Stimulus(valid=1, take=1, input_value=511, input_mask=4095, input_rotated=5429, input_sequence=1073741824, expected_ready=1, expected_valid=1, expected_data_value=126, expected_data_mask=1023, expected_data_rotated=254, expected_data_sequence=268435456),
            Stimulus(valid=1, take=1, input_value=1023, input_mask=8191, input_rotated=4384, input_sequence=2147483648, expected_ready=1, expected_valid=1, expected_data_value=254, expected_data_mask=2047, expected_data_rotated=510, expected_data_sequence=536870912),
            Stimulus(valid=1, take=1, input_value=2047, input_mask=2378, input_rotated=3339, input_sequence=4294967296, expected_ready=1, expected_valid=1, expected_data_value=510, expected_data_mask=4095, expected_data_rotated=1022, expected_data_sequence=1073741824),
            Stimulus(valid=1, take=1, input_value=4095, input_mask=5429, input_rotated=6390, input_sequence=8589934592, expected_ready=1, expected_valid=1, expected_data_value=1022, expected_data_mask=8191, expected_data_rotated=2046, expected_data_sequence=2147483648),
            Stimulus(valid=1, take=1, input_value=8191, input_mask=4384, input_rotated=1249, input_sequence=17179869184, expected_ready=1, expected_valid=1, expected_data_value=331, expected_data_mask=2378, expected_data_rotated=4094, expected_data_sequence=4294967296),
            Stimulus(valid=1, take=1, input_value=2378, input_mask=3339, input_rotated=204, input_sequence=34359738368, expected_ready=1, expected_valid=1, expected_data_value=1332, expected_data_mask=5429, expected_data_rotated=8190, expected_data_sequence=8589934592),
            Stimulus(valid=1, take=1, input_value=5429, input_mask=6390, input_rotated=7351, input_sequence=68719476736, expected_ready=1, expected_valid=1, expected_data_value=4385, expected_data_mask=4384, expected_data_rotated=8191, expected_data_sequence=17179869184),
            Stimulus(valid=1, take=1, input_value=4384, input_mask=1249, input_rotated=2210, input_sequence=1, expected_ready=1, expected_valid=1, expected_data_value=2315, expected_data_mask=3339, expected_data_rotated=4756, expected_data_sequence=34359738368),
            Stimulus(valid=1, take=1, input_value=3339, input_mask=204, input_rotated=5261, input_sequence=3, expected_ready=1, expected_valid=1, expected_data_value=4149, expected_data_mask=6390, expected_data_rotated=2667, expected_data_sequence=68719476736),
            Stimulus(valid=1, take=1, input_value=6390, input_mask=7351, input_rotated=4216, input_sequence=7, expected_ready=1, expected_valid=1, expected_data_value=33, expected_data_mask=1249, expected_data_rotated=577, expected_data_sequence=1),
            Stimulus(valid=1, take=1, input_value=1249, input_mask=2210, input_rotated=3171, input_sequence=15, expected_ready=1, expected_valid=1, expected_data_value=9, expected_data_mask=204, expected_data_rotated=6678, expected_data_sequence=3),
            Stimulus(valid=1, take=1, input_value=204, input_mask=5261, input_rotated=6222, input_sequence=31, expected_ready=1, expected_valid=1, expected_data_value=6327, expected_data_mask=7351, expected_data_rotated=4589, expected_data_sequence=7),
            Stimulus(valid=1, take=1, input_value=7351, input_mask=4216, input_rotated=1081, input_sequence=63, expected_ready=1, expected_valid=1, expected_data_value=161, expected_data_mask=2210, expected_data_rotated=2498, expected_data_sequence=15),
            Stimulus(valid=1, take=1, input_value=2210, input_mask=3171, input_rotated=36, input_sequence=127, expected_ready=1, expected_valid=1, expected_data_value=141, expected_data_mask=5261, expected_data_rotated=408, expected_data_sequence=31),
            Stimulus(valid=1, take=1, input_value=5261, input_mask=6222, input_rotated=7183, input_sequence=255, expected_ready=1, expected_valid=1, expected_data_value=4145, expected_data_mask=4216, expected_data_rotated=6511, expected_data_sequence=63),
            Stimulus(valid=1, take=1, input_value=4216, input_mask=1081, input_rotated=6138, input_sequence=511, expected_ready=1, expected_valid=1, expected_data_value=2083, expected_data_mask=3171, expected_data_rotated=4420, expected_data_sequence=127),
            Stimulus(valid=1, take=1, input_value=3171, input_mask=36, input_rotated=5093, input_sequence=1023, expected_ready=1, expected_valid=1, expected_data_value=4109, expected_data_mask=6222, expected_data_rotated=2331, expected_data_sequence=255),
            Stimulus(valid=1, take=1, input_value=6222, input_mask=7183, input_rotated=8144, input_sequence=2047, expected_ready=1, expected_valid=1, expected_data_value=57, expected_data_mask=1081, expected_data_rotated=241, expected_data_sequence=511),
            Stimulus(valid=1, take=1, input_value=1081, input_mask=6138, input_rotated=3003, input_sequence=4095, expected_ready=1, expected_valid=1, expected_data_value=33, expected_data_mask=36, expected_data_rotated=6342, expected_data_sequence=1023),
            Stimulus(valid=1, take=1, input_value=36, input_mask=5093, input_rotated=1958, input_sequence=8191, expected_ready=1, expected_valid=1, expected_data_value=6159, expected_data_mask=7183, expected_data_rotated=4253, expected_data_sequence=2047),
            Stimulus(valid=1, take=1, input_value=7183, input_mask=8144, input_rotated=913, input_sequence=16383, expected_ready=1, expected_valid=1, expected_data_value=1081, expected_data_mask=6138, expected_data_rotated=2162, expected_data_sequence=4095),
            Stimulus(valid=1, take=1, input_value=6138, input_mask=3003, input_rotated=3964, input_sequence=32767, expected_ready=1, expected_valid=1, expected_data_value=37, expected_data_mask=5093, expected_data_rotated=72, expected_data_sequence=8191),
            Stimulus(valid=1, take=1, input_value=5093, input_mask=1958, input_rotated=7015, input_sequence=65535, expected_ready=1, expected_valid=1, expected_data_value=7169, expected_data_mask=8144, expected_data_rotated=6175, expected_data_sequence=16383),
            Stimulus(valid=1, take=1, input_value=8144, input_mask=913, input_rotated=5970, input_sequence=131071, expected_ready=1, expected_valid=1, expected_data_value=955, expected_data_mask=3003, expected_data_rotated=4085, expected_data_sequence=32767),
            Stimulus(valid=1, take=1, input_value=3003, input_mask=3964, input_rotated=4925, input_sequence=262143, expected_ready=1, expected_valid=1, expected_data_value=933, expected_data_mask=1958, expected_data_rotated=1995, expected_data_sequence=65535),
            Stimulus(valid=1, take=1, input_value=1958, input_mask=7015, input_rotated=7976, input_sequence=524287, expected_ready=1, expected_valid=1, expected_data_value=913, expected_data_mask=913, expected_data_rotated=8097, expected_data_sequence=131071),
            Stimulus(valid=1, take=1, input_value=913, input_mask=5970, input_rotated=2835, input_sequence=1048575, expected_ready=1, expected_valid=1, expected_data_value=2873, expected_data_mask=3964, expected_data_rotated=6006, expected_data_sequence=262143),
            Stimulus(valid=1, take=1, input_value=3964, input_mask=4925, input_rotated=1790, input_sequence=2097151, expected_ready=1, expected_valid=1, expected_data_value=807, expected_data_mask=7015, expected_data_rotated=3916, expected_data_sequence=524287),
            Stimulus(valid=1, take=1, input_value=7015, input_mask=7976, input_rotated=745, input_sequence=4194303, expected_ready=1, expected_valid=1, expected_data_value=785, expected_data_mask=5970, expected_data_rotated=1826, expected_data_sequence=1048575),
            Stimulus(valid=1, take=1, input_value=5970, input_mask=2835, input_rotated=3796, input_sequence=8388607, expected_ready=1, expected_valid=1, expected_data_value=829, expected_data_mask=4925, expected_data_rotated=7928, expected_data_sequence=2097151),
            Stimulus(valid=1, take=1, input_value=4925, input_mask=1790, input_rotated=6847, input_sequence=16777215, expected_ready=1, expected_valid=1, expected_data_value=6945, expected_data_mask=7976, expected_data_rotated=5839, expected_data_sequence=4194303),
            Stimulus(valid=1, take=1, input_value=7976, input_mask=745, input_rotated=5802, input_sequence=33554431, expected_ready=1, expected_valid=1, expected_data_value=787, expected_data_mask=2835, expected_data_rotated=3749, expected_data_sequence=8388607),
            Stimulus(valid=1, take=1, input_value=2835, input_mask=3796, input_rotated=4757, input_sequence=67108863, expected_ready=1, expected_valid=1, expected_data_value=573, expected_data_mask=1790, expected_data_rotated=1659, expected_data_sequence=16777215),
            Stimulus(valid=1, take=1, input_value=1790, input_mask=6847, input_rotated=7808, input_sequence=134217727, expected_ready=1, expected_valid=1, expected_data_value=553, expected_data_mask=745, expected_data_rotated=7761, expected_data_sequence=33554431),
            Stimulus(valid=1, take=1, input_value=745, input_mask=5802, input_rotated=2667, input_sequence=268435455, expected_ready=1, expected_valid=1, expected_data_value=2577, expected_data_mask=3796, expected_data_rotated=5670, expected_data_sequence=67108863),
            Stimulus(valid=1, take=1, input_value=3796, input_mask=4757, input_rotated=1622, input_sequence=536870911, expected_ready=1, expected_valid=1, expected_data_value=703, expected_data_mask=6847, expected_data_rotated=3580, expected_data_sequence=134217727),
            Stimulus(valid=1, take=1, input_value=6847, input_mask=7808, input_rotated=577, input_sequence=1073741823, expected_ready=1, expected_valid=1, expected_data_value=681, expected_data_mask=5802, expected_data_rotated=1490, expected_data_sequence=268435455),
            Stimulus(valid=1, take=1, input_value=5802, input_mask=2667, input_rotated=3628, input_sequence=2147483647, expected_ready=1, expected_valid=1, expected_data_value=661, expected_data_mask=4757, expected_data_rotated=7592, expected_data_sequence=536870911),
            Stimulus(valid=1, take=1, input_value=4757, input_mask=1622, input_rotated=6679, input_sequence=4294967295, expected_ready=1, expected_valid=1, expected_data_value=6785, expected_data_mask=7808, expected_data_rotated=5503, expected_data_sequence=1073741823),
            Stimulus(valid=1, take=1, input_value=7808, input_mask=577, input_rotated=5634, input_sequence=8589934591, expected_ready=1, expected_valid=1, expected_data_value=555, expected_data_mask=2667, expected_data_rotated=3413, expected_data_sequence=2147483647),
            Stimulus(valid=1, take=1, input_value=2667, input_mask=3628, input_rotated=4589, input_sequence=17179869183, expected_ready=1, expected_valid=1, expected_data_value=533, expected_data_mask=1622, expected_data_rotated=1323, expected_data_sequence=4294967295),
            Stimulus(valid=1, take=1, input_value=1622, input_mask=6679, input_rotated=7640, input_sequence=34359738367, expected_ready=1, expected_valid=1, expected_data_value=513, expected_data_mask=577, expected_data_rotated=7425, expected_data_sequence=8589934591),
            Stimulus(valid=1, take=1, input_value=577, input_mask=5634, input_rotated=2499, input_sequence=68719476735, expected_ready=1, expected_valid=1, expected_data_value=2601, expected_data_mask=3628, expected_data_rotated=5334, expected_data_sequence=17179869183),
            Stimulus(valid=1, take=1, input_value=3628, input_mask=4589, input_rotated=1454, input_sequence=137438953471, expected_ready=1, expected_valid=1, expected_data_value=535, expected_data_mask=6679, expected_data_rotated=3244, expected_data_sequence=34359738367),
            Stimulus(valid=1, take=1, input_value=6679, input_mask=7640, input_rotated=409, input_sequence=63240418650, expected_ready=1, expected_valid=1, expected_data_value=513, expected_data_mask=5634, expected_data_rotated=1154, expected_data_sequence=68719476735),
            Stimulus(valid=1, take=1, input_value=5634, input_mask=2499, input_rotated=3460, input_sequence=91169935685, expected_ready=1, expected_valid=1, expected_data_value=45, expected_data_mask=4589, expected_data_rotated=7256, expected_data_sequence=137438953471),
            Stimulus(valid=1, take=1, input_value=4589, input_mask=1454, input_rotated=6511, input_sequence=119099055408, expected_ready=1, expected_valid=1, expected_data_value=6161, expected_data_mask=7640, expected_data_rotated=5167, expected_data_sequence=63240418650),
            Stimulus(valid=1, take=1, input_value=7640, input_mask=409, input_rotated=5466, input_sequence=9589090587, expected_ready=1, expected_valid=1, expected_data_value=3, expected_data_mask=2499, expected_data_rotated=3077, expected_data_sequence=91169935685),
            Stimulus(valid=1, take=1, input_value=2499, input_mask=3460, input_rotated=4421, input_sequence=37518468358, expected_ready=1, expected_valid=1, expected_data_value=429, expected_data_mask=1454, expected_data_rotated=987, expected_data_sequence=119099055408),
            Stimulus(valid=1, take=1, input_value=1454, input_mask=6511, input_rotated=7472, input_sequence=65447452913, expected_ready=1, expected_valid=1, expected_data_value=409, expected_data_mask=409, expected_data_rotated=7089, expected_data_sequence=9589090587),
            Stimulus(valid=1, take=1, input_value=409, input_mask=5466, input_rotated=2331, input_sequence=93376572636, expected_ready=1, expected_valid=1, expected_data_value=2433, expected_data_mask=3460, expected_data_rotated=4998, expected_data_sequence=37518468358),
            Stimulus(valid=1, take=1, input_value=3460, input_mask=4421, input_rotated=1286, input_sequence=121306060999, expected_ready=1, expected_valid=1, expected_data_value=303, expected_data_mask=6511, expected_data_rotated=2908, expected_data_sequence=65447452913),
            Stimulus(valid=1, take=1, input_value=6511, input_mask=7472, input_rotated=241, input_sequence=11795961010, expected_ready=1, expected_valid=1, expected_data_value=281, expected_data_mask=5466, expected_data_rotated=818, expected_data_sequence=93376572636),
            Stimulus(valid=1, take=1, input_value=5466, input_mask=2331, input_rotated=3292, input_sequence=39725478045, expected_ready=1, expected_valid=1, expected_data_value=261, expected_data_mask=4421, expected_data_rotated=6920, expected_data_sequence=121306060999),
            Stimulus(valid=1, take=1, input_value=4421, input_mask=1286, input_rotated=6343, input_sequence=67654597768, expected_ready=1, expected_valid=1, expected_data_value=6433, expected_data_mask=7472, expected_data_rotated=4831, expected_data_sequence=11795961010),
            Stimulus(valid=1, take=1, input_value=7472, input_mask=241, input_rotated=5298, input_sequence=95583570035, expected_ready=1, expected_valid=1, expected_data_value=283, expected_data_mask=2331, expected_data_rotated=2741, expected_data_sequence=39725478045),
            Stimulus(valid=1, take=1, input_value=2331, input_mask=3292, input_rotated=4253, input_sequence=123512947806, expected_ready=1, expected_valid=1, expected_data_value=261, expected_data_mask=1286, expected_data_rotated=651, expected_data_sequence=67654597768),
            Stimulus(valid=1, take=1, input_value=1286, input_mask=6343, input_rotated=7304, input_sequence=14002978889, expected_ready=1, expected_valid=1, expected_data_value=49, expected_data_mask=241, expected_data_rotated=6753, expected_data_sequence=95583570035),
            Stimulus(valid=1, take=1, input_value=241, input_mask=5298, input_rotated=2163, input_sequence=41932098612, expected_ready=1, expected_valid=1, expected_data_value=2073, expected_data_mask=3292, expected_data_rotated=4662, expected_data_sequence=123512947806),
            Stimulus(valid=1, take=1, input_value=3292, input_mask=4253, input_rotated=1118, input_sequence=69861537823, expected_ready=1, expected_valid=1, expected_data_value=7, expected_data_mask=6343, expected_data_rotated=2572, expected_data_sequence=14002978889),
            Stimulus(valid=1, take=1, input_value=6343, input_mask=7304, input_rotated=73, input_sequence=97790587914, expected_ready=1, expected_valid=1, expected_data_value=177, expected_data_mask=5298, expected_data_rotated=482, expected_data_sequence=41932098612),
            Stimulus(valid=1, take=1, input_value=5298, input_mask=2163, input_rotated=3124, input_sequence=125720031221, expected_ready=1, expected_valid=1, expected_data_value=157, expected_data_mask=4253, expected_data_rotated=6584, expected_data_sequence=69861537823),
            Stimulus(valid=1, take=1, input_value=4253, input_mask=1118, input_rotated=6175, input_sequence=16210009056, expected_ready=1, expected_valid=1, expected_data_value=6273, expected_data_mask=7304, expected_data_rotated=4495, expected_data_sequence=97790587914),
            Stimulus(valid=1, take=1, input_value=7304, input_mask=73, input_rotated=5130, input_sequence=44139063243, expected_ready=1, expected_valid=1, expected_data_value=51, expected_data_mask=2163, expected_data_rotated=2405, expected_data_sequence=125720031221),
            Stimulus(valid=1, take=1, input_value=2163, input_mask=3124, input_rotated=4085, input_sequence=72068506550, expected_ready=1, expected_valid=1, expected_data_value=29, expected_data_mask=1118, expected_data_rotated=315, expected_data_sequence=16210009056),
            Stimulus(valid=1, take=1, input_value=1118, input_mask=6175, input_rotated=7136, input_sequence=99997564833, expected_ready=1, expected_valid=1, expected_data_value=9, expected_data_mask=73, expected_data_rotated=6417, expected_data_sequence=44139063243),
            Stimulus(valid=1, take=1, input_value=73, input_mask=5130, input_rotated=1995, input_sequence=127927004044, expected_ready=1, expected_valid=1, expected_data_value=2097, expected_data_mask=3124, expected_data_rotated=4326, expected_data_sequence=72068506550),
            Stimulus(valid=1, take=1, input_value=3124, input_mask=4085, input_rotated=950, input_sequence=18417112951, expected_ready=1, expected_valid=1, expected_data_value=31, expected_data_mask=6175, expected_data_rotated=2236, expected_data_sequence=99997564833),
            Stimulus(take=1, input_value=6175, input_mask=7136, input_rotated=8097, input_sequence=46346031970, expected_ready=1, expected_valid=1, expected_data_value=9, expected_data_mask=5130, expected_data_rotated=146, expected_data_sequence=127927004044),
            Stimulus(take=1, input_value=5130, input_mask=1995, input_rotated=2956, input_sequence=74275606349, expected_ready=1, expected_valid=1, expected_data_value=3125, expected_data_mask=4085, expected_data_rotated=6248, expected_data_sequence=18417112951),
            Stimulus(take=1, input_value=4085, input_mask=950, input_rotated=6007, input_sequence=102204537656, expected_ready=1),
            Stimulus(take=1, input_value=7136, input_mask=8097, input_rotated=4962, input_sequence=130133575459, expected_ready=1),
            Stimulus(take=1, input_value=1995, input_mask=2956, input_rotated=3917, input_sequence=20624065294, expected_ready=1),
            Stimulus(take=1, input_value=950, input_mask=6007, input_rotated=6968, input_sequence=48553123577, expected_ready=1),
            Stimulus(take=1, input_value=8097, input_mask=4962, input_rotated=1827, input_sequence=76482562788, expected_ready=1),
            Stimulus(take=1, input_value=2956, input_mask=3917, input_rotated=782, input_sequence=104411576015, expected_ready=1),
            Stimulus(take=1, input_value=6007, input_mask=6968, input_rotated=7929, input_sequence=132340560570, expected_ready=1),
            Stimulus(take=1, input_value=4962, input_mask=1827, input_rotated=2788, input_sequence=22831115941, expected_ready=1),
            Stimulus(take=1, input_value=3917, input_mask=782, input_rotated=5839, input_sequence=50760112784, expected_ready=1),
            Stimulus(take=1, input_value=6968, input_mask=7929, input_rotated=4794, input_sequence=78689101435, expected_ready=1),
            Stimulus(take=1, input_value=1827, input_mask=2788, input_rotated=3749, input_sequence=106618610278, expected_ready=1),
            Stimulus(take=1, input_value=782, input_mask=5839, input_rotated=6800, input_sequence=134547603025, expected_ready=1),
            Stimulus(take=1, input_value=7929, input_mask=4794, input_rotated=1659, input_sequence=25038154300, expected_ready=1),
            Stimulus(take=1, input_value=2788, input_mask=3749, input_rotated=614, input_sequence=52967118375, expected_ready=1),
            Stimulus(take=1, input_value=5839, input_mask=6800, input_rotated=7761, input_sequence=80896102930, expected_ready=1),
            Stimulus(take=1, input_value=4794, input_mask=1659, input_rotated=2620, input_sequence=108825611773, expected_ready=1),
            Stimulus(take=1, input_value=3749, input_mask=614, input_rotated=5671, input_sequence=136754608616, expected_ready=1),
            Stimulus(take=1, input_value=6800, input_mask=7761, input_rotated=4626, input_sequence=27244627411, expected_ready=1),
            Stimulus(take=1, input_value=1659, input_mask=2620, input_rotated=3581, input_sequence=55174136254, expected_ready=1),
            Stimulus(take=1, input_value=614, input_mask=5671, input_rotated=6632, input_sequence=83103129001, expected_ready=1),
            Stimulus(take=1, input_value=7761, input_mask=4626, input_rotated=1491, input_sequence=111032633748, expected_ready=1),
            Stimulus(take=1, input_value=2620, input_mask=3581, input_rotated=446, input_sequence=1522922879, expected_ready=1),
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
def BitWidthsSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = MaskedTag(
        value=frame.input_value,
        mask=frame.input_mask,
        rotated=frame.input_rotated,
        sequence=frame.input_sequence,
    )
    dut = BitWidths(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "bit_widths input capacity"
        assert dut.valid == frame.expected_valid, "bit_widths result availability"
        assert (
            dut.data.value == frame.expected_data_value
        ), "bit_widths value old-state check"
        assert (
            dut.data.mask == frame.expected_data_mask
        ), "bit_widths mask old-state check"
        assert (
            dut.data.rotated == frame.expected_data_rotated
        ), "bit_widths rotated old-state check"
        assert (
            dut.data.sequence == frame.expected_data_sequence
        ), "bit_widths sequence old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "value", dut.data.value)
        log("info", "mask", dut.data.mask)
        log("info", "rotated", dut.data.rotated)
        log("info", "sequence", dut.data.sequence)

    advance(phase)
    check()

"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_count_zeros_pipeline.count_zeros_pipeline import (
    CountZerosPipeline,
    Item,
    CountZerosResult,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_value: bits[13]
    input_leading: bits[4]
    input_trailing: bits[4]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_value: bits[13]
    expected_data_leading: bits[4]
    expected_data_trailing: bits[4]


@struct
class Stimuli:
    rows: table[185, Stimulus]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, input_leading=3, input_trailing=6, expected_ready=1),
            Stimulus(valid=1, input_value=8191, input_leading=4, input_trailing=7, expected_ready=1),
            Stimulus(valid=1, input_value=8190, input_leading=5, input_trailing=8, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, input_value=4096, input_leading=6, input_trailing=9, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, input_value=4095, input_leading=7, input_trailing=10, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, input_value=5461, input_leading=8, input_trailing=11, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, input_value=2730, input_leading=9, input_trailing=12, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, input_value=1, input_leading=10, input_trailing=13, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, input_value=2, input_leading=11, input_trailing=14, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, input_value=4, input_leading=12, input_trailing=15, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, input_value=8, input_leading=13, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, input_value=16, input_leading=14, input_trailing=1, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, input_value=32, input_leading=15, input_trailing=2, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, take=1, input_value=64, input_trailing=3, expected_ready=1, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, take=1, input_value=128, input_leading=1, input_trailing=4, expected_ready=1, expected_valid=1, expected_data_value=8191),
            Stimulus(valid=1, take=1, input_value=256, input_leading=2, input_trailing=5, expected_ready=1, expected_valid=1, expected_data_value=64, expected_data_leading=6, expected_data_trailing=6),
            Stimulus(valid=1, input_value=512, input_leading=3, input_trailing=6, expected_valid=1, expected_data_value=128, expected_data_leading=5, expected_data_trailing=7),
            Stimulus(valid=1, take=1, input_value=1024, input_leading=4, input_trailing=7, expected_ready=1, expected_valid=1, expected_data_value=128, expected_data_leading=5, expected_data_trailing=7),
            Stimulus(valid=1, take=1, input_value=2048, input_leading=5, input_trailing=8, expected_ready=1, expected_valid=1, expected_data_value=256, expected_data_leading=4, expected_data_trailing=8),
            Stimulus(valid=1, take=1, input_value=4096, input_leading=6, input_trailing=9, expected_ready=1, expected_valid=1, expected_data_value=1024, expected_data_leading=2, expected_data_trailing=10),
            Stimulus(valid=1, input_value=1, input_leading=7, input_trailing=10, expected_valid=1, expected_data_value=2048, expected_data_leading=1, expected_data_trailing=11),
            Stimulus(valid=1, take=1, input_value=3, input_leading=8, input_trailing=11, expected_ready=1, expected_valid=1, expected_data_value=2048, expected_data_leading=1, expected_data_trailing=11),
            Stimulus(valid=1, take=1, input_value=7, input_leading=9, input_trailing=12, expected_ready=1, expected_valid=1, expected_data_value=4096, expected_data_trailing=12),
            Stimulus(valid=1, take=1, input_value=15, input_leading=10, input_trailing=13, expected_ready=1, expected_valid=1, expected_data_value=3, expected_data_leading=11),
            Stimulus(valid=1, input_value=31, input_leading=11, input_trailing=14, expected_valid=1, expected_data_value=7, expected_data_leading=10),
            Stimulus(valid=1, take=1, input_value=63, input_leading=12, input_trailing=15, expected_ready=1, expected_valid=1, expected_data_value=7, expected_data_leading=10),
            Stimulus(valid=1, take=1, input_value=127, input_leading=13, expected_ready=1, expected_valid=1, expected_data_value=15, expected_data_leading=9),
            Stimulus(valid=1, take=1, input_value=255, input_leading=14, input_trailing=1, expected_ready=1, expected_valid=1, expected_data_value=63, expected_data_leading=7),
            Stimulus(valid=1, input_value=511, input_leading=15, input_trailing=2, expected_valid=1, expected_data_value=127, expected_data_leading=6),
            Stimulus(valid=1, take=1, input_value=1023, input_trailing=3, expected_ready=1, expected_valid=1, expected_data_value=127, expected_data_leading=6),
            Stimulus(valid=1, take=1, input_value=2047, input_leading=1, input_trailing=4, expected_ready=1, expected_valid=1, expected_data_value=255, expected_data_leading=5),
            Stimulus(take=1, input_value=4095, input_leading=2, input_trailing=5, expected_ready=1, expected_valid=1, expected_data_value=1023, expected_data_leading=3),
            Stimulus(valid=1, input_value=8191, input_leading=3, input_trailing=6, expected_ready=1, expected_valid=1, expected_data_value=2047, expected_data_leading=2),
            Stimulus(valid=1, take=1, input_value=2378, input_leading=4, input_trailing=7, expected_ready=1, expected_valid=1, expected_data_value=2047, expected_data_leading=2),
            Stimulus(valid=1, take=1, input_value=5429, input_leading=5, input_trailing=8, expected_ready=1, expected_valid=1, expected_data_value=8191),
            Stimulus(valid=1, take=1, input_value=4384, input_leading=6, input_trailing=9, expected_ready=1, expected_valid=1, expected_data_value=2378, expected_data_leading=1, expected_data_trailing=1),
            Stimulus(input_value=3339, input_leading=7, input_trailing=10, expected_valid=1, expected_data_value=5429),
            Stimulus(valid=1, take=1, input_value=6390, input_leading=8, input_trailing=11, expected_ready=1, expected_valid=1, expected_data_value=5429),
            Stimulus(valid=1, take=1, input_value=1249, input_leading=9, input_trailing=12, expected_ready=1, expected_valid=1, expected_data_value=4384, expected_data_trailing=5),
            Stimulus(valid=1, take=1, input_value=204, input_leading=10, input_trailing=13, expected_ready=1, expected_valid=1, expected_data_value=6390, expected_data_trailing=1),
            Stimulus(valid=1, input_value=7351, input_leading=11, input_trailing=14, expected_valid=1, expected_data_value=1249, expected_data_leading=2),
            Stimulus(take=1, input_value=2210, input_leading=12, input_trailing=15, expected_ready=1, expected_valid=1, expected_data_value=1249, expected_data_leading=2),
            Stimulus(valid=1, take=1, input_value=5261, input_leading=13, expected_ready=1, expected_valid=1, expected_data_value=204, expected_data_leading=5, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_value=4216, input_leading=14, input_trailing=1, expected_ready=1),
            Stimulus(valid=1, input_value=3171, input_leading=15, input_trailing=2, expected_valid=1, expected_data_value=5261),
            Stimulus(valid=1, take=1, input_value=6222, input_trailing=3, expected_ready=1, expected_valid=1, expected_data_value=5261),
            Stimulus(take=1, input_value=1081, input_leading=1, input_trailing=4, expected_ready=1, expected_valid=1, expected_data_value=4216, expected_data_trailing=3),
            Stimulus(valid=1, take=1, input_value=36, input_leading=2, input_trailing=5, expected_ready=1, expected_valid=1, expected_data_value=6222, expected_data_trailing=1),
            Stimulus(valid=1, input_value=7183, input_leading=3, input_trailing=6, expected_ready=1),
            Stimulus(valid=1, input_value=6138, input_leading=4, input_trailing=7, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=5093, input_leading=5, input_trailing=8, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(input_value=8144, input_leading=6, input_trailing=9, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=3003, input_leading=7, input_trailing=10, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=1958, input_leading=8, input_trailing=11, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=913, input_leading=9, input_trailing=12, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=3964, input_leading=10, input_trailing=13, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(input_value=7015, input_leading=11, input_trailing=14, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=5970, input_leading=12, input_trailing=15, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=4925, input_leading=13, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=7976, input_leading=14, input_trailing=1, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=2835, input_leading=15, input_trailing=2, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(input_value=1790, input_trailing=3, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=745, input_leading=1, input_trailing=4, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, input_value=3796, input_leading=2, input_trailing=5, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_leading=3, input_trailing=6, expected_ready=1, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_value=8191, input_leading=4, input_trailing=7, expected_ready=1, expected_valid=1, expected_data_value=7183),
            Stimulus(valid=1, take=1, input_value=8190, input_leading=5, input_trailing=8, expected_ready=1, expected_valid=1, expected_data_leading=13, expected_data_trailing=13),
            Stimulus(valid=1, take=1, input_value=4096, input_leading=6, input_trailing=9, expected_ready=1, expected_valid=1, expected_data_value=8191),
            Stimulus(valid=1, take=1, input_value=4095, input_leading=7, input_trailing=10, expected_ready=1, expected_valid=1, expected_data_value=8190, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=5461, input_leading=8, input_trailing=11, expected_ready=1, expected_valid=1, expected_data_value=4096, expected_data_trailing=12),
            Stimulus(valid=1, take=1, input_value=2730, input_leading=9, input_trailing=12, expected_ready=1, expected_valid=1, expected_data_value=4095, expected_data_leading=1),
            Stimulus(valid=1, take=1, input_value=1, input_leading=10, input_trailing=13, expected_ready=1, expected_valid=1, expected_data_value=5461),
            Stimulus(valid=1, take=1, input_value=2, input_leading=11, input_trailing=14, expected_ready=1, expected_valid=1, expected_data_value=2730, expected_data_leading=1, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=4, input_leading=12, input_trailing=15, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_leading=12),
            Stimulus(valid=1, take=1, input_value=8, input_leading=13, expected_ready=1, expected_valid=1, expected_data_value=2, expected_data_leading=11, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=16, input_leading=14, input_trailing=1, expected_ready=1, expected_valid=1, expected_data_value=4, expected_data_leading=10, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_value=32, input_leading=15, input_trailing=2, expected_ready=1, expected_valid=1, expected_data_value=8, expected_data_leading=9, expected_data_trailing=3),
            Stimulus(valid=1, take=1, input_value=64, input_trailing=3, expected_ready=1, expected_valid=1, expected_data_value=16, expected_data_leading=8, expected_data_trailing=4),
            Stimulus(valid=1, take=1, input_value=128, input_leading=1, input_trailing=4, expected_ready=1, expected_valid=1, expected_data_value=32, expected_data_leading=7, expected_data_trailing=5),
            Stimulus(valid=1, take=1, input_value=256, input_leading=2, input_trailing=5, expected_ready=1, expected_valid=1, expected_data_value=64, expected_data_leading=6, expected_data_trailing=6),
            Stimulus(valid=1, take=1, input_value=512, input_leading=3, input_trailing=6, expected_ready=1, expected_valid=1, expected_data_value=128, expected_data_leading=5, expected_data_trailing=7),
            Stimulus(valid=1, take=1, input_value=1024, input_leading=4, input_trailing=7, expected_ready=1, expected_valid=1, expected_data_value=256, expected_data_leading=4, expected_data_trailing=8),
            Stimulus(valid=1, take=1, input_value=2048, input_leading=5, input_trailing=8, expected_ready=1, expected_valid=1, expected_data_value=512, expected_data_leading=3, expected_data_trailing=9),
            Stimulus(valid=1, take=1, input_value=4096, input_leading=6, input_trailing=9, expected_ready=1, expected_valid=1, expected_data_value=1024, expected_data_leading=2, expected_data_trailing=10),
            Stimulus(valid=1, take=1, input_value=1, input_leading=7, input_trailing=10, expected_ready=1, expected_valid=1, expected_data_value=2048, expected_data_leading=1, expected_data_trailing=11),
            Stimulus(valid=1, take=1, input_value=3, input_leading=8, input_trailing=11, expected_ready=1, expected_valid=1, expected_data_value=4096, expected_data_trailing=12),
            Stimulus(valid=1, take=1, input_value=7, input_leading=9, input_trailing=12, expected_ready=1, expected_valid=1, expected_data_value=1, expected_data_leading=12),
            Stimulus(valid=1, take=1, input_value=15, input_leading=10, input_trailing=13, expected_ready=1, expected_valid=1, expected_data_value=3, expected_data_leading=11),
            Stimulus(valid=1, take=1, input_value=31, input_leading=11, input_trailing=14, expected_ready=1, expected_valid=1, expected_data_value=7, expected_data_leading=10),
            Stimulus(valid=1, take=1, input_value=63, input_leading=12, input_trailing=15, expected_ready=1, expected_valid=1, expected_data_value=15, expected_data_leading=9),
            Stimulus(valid=1, take=1, input_value=127, input_leading=13, expected_ready=1, expected_valid=1, expected_data_value=31, expected_data_leading=8),
            Stimulus(valid=1, take=1, input_value=255, input_leading=14, input_trailing=1, expected_ready=1, expected_valid=1, expected_data_value=63, expected_data_leading=7),
            Stimulus(valid=1, take=1, input_value=511, input_leading=15, input_trailing=2, expected_ready=1, expected_valid=1, expected_data_value=127, expected_data_leading=6),
            Stimulus(valid=1, take=1, input_value=1023, input_trailing=3, expected_ready=1, expected_valid=1, expected_data_value=255, expected_data_leading=5),
            Stimulus(valid=1, take=1, input_value=2047, input_leading=1, input_trailing=4, expected_ready=1, expected_valid=1, expected_data_value=511, expected_data_leading=4),
            Stimulus(valid=1, take=1, input_value=4095, input_leading=2, input_trailing=5, expected_ready=1, expected_valid=1, expected_data_value=1023, expected_data_leading=3),
            Stimulus(valid=1, take=1, input_value=8191, input_leading=3, input_trailing=6, expected_ready=1, expected_valid=1, expected_data_value=2047, expected_data_leading=2),
            Stimulus(valid=1, take=1, input_value=2378, input_leading=4, input_trailing=7, expected_ready=1, expected_valid=1, expected_data_value=4095, expected_data_leading=1),
            Stimulus(valid=1, take=1, input_value=5429, input_leading=5, input_trailing=8, expected_ready=1, expected_valid=1, expected_data_value=8191),
            Stimulus(valid=1, take=1, input_value=4384, input_leading=6, input_trailing=9, expected_ready=1, expected_valid=1, expected_data_value=2378, expected_data_leading=1, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=3339, input_leading=7, input_trailing=10, expected_ready=1, expected_valid=1, expected_data_value=5429),
            Stimulus(valid=1, take=1, input_value=6390, input_leading=8, input_trailing=11, expected_ready=1, expected_valid=1, expected_data_value=4384, expected_data_trailing=5),
            Stimulus(valid=1, take=1, input_value=1249, input_leading=9, input_trailing=12, expected_ready=1, expected_valid=1, expected_data_value=3339, expected_data_leading=1),
            Stimulus(valid=1, take=1, input_value=204, input_leading=10, input_trailing=13, expected_ready=1, expected_valid=1, expected_data_value=6390, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=7351, input_leading=11, input_trailing=14, expected_ready=1, expected_valid=1, expected_data_value=1249, expected_data_leading=2),
            Stimulus(valid=1, take=1, input_value=2210, input_leading=12, input_trailing=15, expected_ready=1, expected_valid=1, expected_data_value=204, expected_data_leading=5, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_value=5261, input_leading=13, expected_ready=1, expected_valid=1, expected_data_value=7351),
            Stimulus(valid=1, take=1, input_value=4216, input_leading=14, input_trailing=1, expected_ready=1, expected_valid=1, expected_data_value=2210, expected_data_leading=1, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=3171, input_leading=15, input_trailing=2, expected_ready=1, expected_valid=1, expected_data_value=5261),
            Stimulus(valid=1, take=1, input_value=6222, input_trailing=3, expected_ready=1, expected_valid=1, expected_data_value=4216, expected_data_trailing=3),
            Stimulus(valid=1, take=1, input_value=1081, input_leading=1, input_trailing=4, expected_ready=1, expected_valid=1, expected_data_value=3171, expected_data_leading=1),
            Stimulus(valid=1, take=1, input_value=36, input_leading=2, input_trailing=5, expected_ready=1, expected_valid=1, expected_data_value=6222, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=7183, input_leading=3, input_trailing=6, expected_ready=1, expected_valid=1, expected_data_value=1081, expected_data_leading=2),
            Stimulus(valid=1, take=1, input_value=6138, input_leading=4, input_trailing=7, expected_ready=1, expected_valid=1, expected_data_value=36, expected_data_leading=7, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_value=5093, input_leading=5, input_trailing=8, expected_ready=1, expected_valid=1, expected_data_value=7183),
            Stimulus(valid=1, take=1, input_value=8144, input_leading=6, input_trailing=9, expected_ready=1, expected_valid=1, expected_data_value=6138, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=3003, input_leading=7, input_trailing=10, expected_ready=1, expected_valid=1, expected_data_value=5093),
            Stimulus(valid=1, take=1, input_value=1958, input_leading=8, input_trailing=11, expected_ready=1, expected_valid=1, expected_data_value=8144, expected_data_trailing=4),
            Stimulus(valid=1, take=1, input_value=913, input_leading=9, input_trailing=12, expected_ready=1, expected_valid=1, expected_data_value=3003, expected_data_leading=1),
            Stimulus(valid=1, take=1, input_value=3964, input_leading=10, input_trailing=13, expected_ready=1, expected_valid=1, expected_data_value=1958, expected_data_leading=2, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=7015, input_leading=11, input_trailing=14, expected_ready=1, expected_valid=1, expected_data_value=913, expected_data_leading=3),
            Stimulus(valid=1, take=1, input_value=5970, input_leading=12, input_trailing=15, expected_ready=1, expected_valid=1, expected_data_value=3964, expected_data_leading=1, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_value=4925, input_leading=13, expected_ready=1, expected_valid=1, expected_data_value=7015),
            Stimulus(valid=1, take=1, input_value=7976, input_leading=14, input_trailing=1, expected_ready=1, expected_valid=1, expected_data_value=5970, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=2835, input_leading=15, input_trailing=2, expected_ready=1, expected_valid=1, expected_data_value=4925),
            Stimulus(valid=1, take=1, input_value=1790, input_trailing=3, expected_ready=1, expected_valid=1, expected_data_value=7976, expected_data_trailing=3),
            Stimulus(valid=1, take=1, input_value=745, input_leading=1, input_trailing=4, expected_ready=1, expected_valid=1, expected_data_value=2835, expected_data_leading=1),
            Stimulus(valid=1, take=1, input_value=3796, input_leading=2, input_trailing=5, expected_ready=1, expected_valid=1, expected_data_value=1790, expected_data_leading=2, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=6847, input_leading=3, input_trailing=6, expected_ready=1, expected_valid=1, expected_data_value=745, expected_data_leading=3),
            Stimulus(valid=1, take=1, input_value=5802, input_leading=4, input_trailing=7, expected_ready=1, expected_valid=1, expected_data_value=3796, expected_data_leading=1, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_value=4757, input_leading=5, input_trailing=8, expected_ready=1, expected_valid=1, expected_data_value=6847),
            Stimulus(valid=1, take=1, input_value=7808, input_leading=6, input_trailing=9, expected_ready=1, expected_valid=1, expected_data_value=5802, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=2667, input_leading=7, input_trailing=10, expected_ready=1, expected_valid=1, expected_data_value=4757),
            Stimulus(valid=1, take=1, input_value=1622, input_leading=8, input_trailing=11, expected_ready=1, expected_valid=1, expected_data_value=7808, expected_data_trailing=7),
            Stimulus(valid=1, take=1, input_value=577, input_leading=9, input_trailing=12, expected_ready=1, expected_valid=1, expected_data_value=2667, expected_data_leading=1),
            Stimulus(valid=1, take=1, input_value=3628, input_leading=10, input_trailing=13, expected_ready=1, expected_valid=1, expected_data_value=1622, expected_data_leading=2, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=6679, input_leading=11, input_trailing=14, expected_ready=1, expected_valid=1, expected_data_value=577, expected_data_leading=3),
            Stimulus(valid=1, take=1, input_value=5634, input_leading=12, input_trailing=15, expected_ready=1, expected_valid=1, expected_data_value=3628, expected_data_leading=1, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_value=4589, input_leading=13, expected_ready=1, expected_valid=1, expected_data_value=6679),
            Stimulus(valid=1, take=1, input_value=7640, input_leading=14, input_trailing=1, expected_ready=1, expected_valid=1, expected_data_value=5634, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=2499, input_leading=15, input_trailing=2, expected_ready=1, expected_valid=1, expected_data_value=4589),
            Stimulus(valid=1, take=1, input_value=1454, input_trailing=3, expected_ready=1, expected_valid=1, expected_data_value=7640, expected_data_trailing=3),
            Stimulus(valid=1, take=1, input_value=409, input_leading=1, input_trailing=4, expected_ready=1, expected_valid=1, expected_data_value=2499, expected_data_leading=1),
            Stimulus(valid=1, take=1, input_value=3460, input_leading=2, input_trailing=5, expected_ready=1, expected_valid=1, expected_data_value=1454, expected_data_leading=2, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=6511, input_leading=3, input_trailing=6, expected_ready=1, expected_valid=1, expected_data_value=409, expected_data_leading=4),
            Stimulus(valid=1, take=1, input_value=5466, input_leading=4, input_trailing=7, expected_ready=1, expected_valid=1, expected_data_value=3460, expected_data_leading=1, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_value=4421, input_leading=5, input_trailing=8, expected_ready=1, expected_valid=1, expected_data_value=6511),
            Stimulus(valid=1, take=1, input_value=7472, input_leading=6, input_trailing=9, expected_ready=1, expected_valid=1, expected_data_value=5466, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=2331, input_leading=7, input_trailing=10, expected_ready=1, expected_valid=1, expected_data_value=4421),
            Stimulus(valid=1, take=1, input_value=1286, input_leading=8, input_trailing=11, expected_ready=1, expected_valid=1, expected_data_value=7472, expected_data_trailing=4),
            Stimulus(valid=1, take=1, input_value=241, input_leading=9, input_trailing=12, expected_ready=1, expected_valid=1, expected_data_value=2331, expected_data_leading=1),
            Stimulus(valid=1, take=1, input_value=3292, input_leading=10, input_trailing=13, expected_ready=1, expected_valid=1, expected_data_value=1286, expected_data_leading=2, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=6343, input_leading=11, input_trailing=14, expected_ready=1, expected_valid=1, expected_data_value=241, expected_data_leading=5),
            Stimulus(valid=1, take=1, input_value=5298, input_leading=12, input_trailing=15, expected_ready=1, expected_valid=1, expected_data_value=3292, expected_data_leading=1, expected_data_trailing=2),
            Stimulus(valid=1, take=1, input_value=4253, input_leading=13, expected_ready=1, expected_valid=1, expected_data_value=6343),
            Stimulus(valid=1, take=1, input_value=7304, input_leading=14, input_trailing=1, expected_ready=1, expected_valid=1, expected_data_value=5298, expected_data_trailing=1),
            Stimulus(valid=1, take=1, input_value=2163, input_leading=15, input_trailing=2, expected_ready=1, expected_valid=1, expected_data_value=4253),
            Stimulus(valid=1, take=1, input_value=1118, input_trailing=3, expected_ready=1, expected_valid=1, expected_data_value=7304, expected_data_trailing=3),
            Stimulus(valid=1, take=1, input_value=73, input_leading=1, input_trailing=4, expected_ready=1, expected_valid=1, expected_data_value=2163, expected_data_leading=1),
            Stimulus(valid=1, take=1, input_value=3124, input_leading=2, input_trailing=5, expected_ready=1, expected_valid=1, expected_data_value=1118, expected_data_leading=2, expected_data_trailing=1),
            Stimulus(take=1, input_value=6175, input_leading=3, input_trailing=6, expected_ready=1, expected_valid=1, expected_data_value=73, expected_data_leading=6),
            Stimulus(take=1, input_value=5130, input_leading=4, input_trailing=7, expected_ready=1, expected_valid=1, expected_data_value=3124, expected_data_leading=1, expected_data_trailing=2),
            Stimulus(take=1, input_value=4085, input_leading=5, input_trailing=8, expected_ready=1),
            Stimulus(take=1, input_value=7136, input_leading=6, input_trailing=9, expected_ready=1),
            Stimulus(take=1, input_value=1995, input_leading=7, input_trailing=10, expected_ready=1),
            Stimulus(take=1, input_value=950, input_leading=8, input_trailing=11, expected_ready=1),
            Stimulus(take=1, input_value=8097, input_leading=9, input_trailing=12, expected_ready=1),
            Stimulus(take=1, input_value=2956, input_leading=10, input_trailing=13, expected_ready=1),
            Stimulus(take=1, input_value=6007, input_leading=11, input_trailing=14, expected_ready=1),
            Stimulus(take=1, input_value=4962, input_leading=12, input_trailing=15, expected_ready=1),
            Stimulus(take=1, input_value=3917, input_leading=13, expected_ready=1),
            Stimulus(take=1, input_value=6968, input_leading=14, input_trailing=1, expected_ready=1),
            Stimulus(take=1, input_value=1827, input_leading=15, input_trailing=2, expected_ready=1),
            Stimulus(take=1, input_value=782, input_trailing=3, expected_ready=1),
            Stimulus(take=1, input_value=7929, input_leading=1, input_trailing=4, expected_ready=1),
            Stimulus(take=1, input_value=2788, input_leading=2, input_trailing=5, expected_ready=1),
            Stimulus(take=1, input_value=5839, input_leading=3, input_trailing=6, expected_ready=1),
            Stimulus(take=1, input_value=4794, input_leading=4, input_trailing=7, expected_ready=1),
            Stimulus(take=1, input_value=3749, input_leading=5, input_trailing=8, expected_ready=1),
            Stimulus(take=1, input_value=6800, input_leading=6, input_trailing=9, expected_ready=1),
            Stimulus(take=1, input_value=1659, input_leading=7, input_trailing=10, expected_ready=1),
            Stimulus(take=1, input_value=614, input_leading=8, input_trailing=11, expected_ready=1),
            Stimulus(take=1, input_value=7761, input_leading=9, input_trailing=12, expected_ready=1),
            Stimulus(take=1, input_value=2620, input_leading=10, input_trailing=13, expected_ready=1),
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
def CountZerosPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Item(
        value=frame.input_value,
        leading=frame.input_leading,
        trailing=frame.input_trailing,
    )
    dut = CountZerosPipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "count_zeros_pipeline input capacity"
        assert (
            dut.valid == frame.expected_valid
        ), "count_zeros_pipeline result availability"
        assert (
            dut.data.value == frame.expected_data_value
        ), "count_zeros_pipeline value old-state check"
        assert (
            dut.data.leading == frame.expected_data_leading
        ), "count_zeros_pipeline leading old-state check"
        assert (
            dut.data.trailing == frame.expected_data_trailing
        ), "count_zeros_pipeline trailing old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "value", dut.data.value)
        log("info", "leading", dut.data.leading)
        log("info", "trailing", dut.data.trailing)

    advance(phase)
    check()

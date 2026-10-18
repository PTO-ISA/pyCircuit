"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_nested_payload_pipeline.nested_payload_pipeline import (
    NestedPayloadPipeline,
    Header,
    Packet,
    AdvanceResult,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_header_opcode: bits[6]
    input_header_mode: bits[3]
    input_payload: bits[17]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_header_opcode: bits[6]
    expected_data_header_mode: bits[3]
    expected_data_payload: bits[17]


@struct
class Stimuli:
    rows: table[185, Stimulus]


def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, input_header_mode=3, input_payload=43690, expected_ready=1),
            Stimulus(valid=1, input_header_opcode=63, input_header_mode=4, input_payload=1, expected_ready=1),
            Stimulus(valid=1, input_header_opcode=62, input_header_mode=5, input_payload=2, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, input_header_opcode=32, input_header_mode=6, input_payload=4, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, input_header_opcode=31, input_header_mode=7, input_payload=8, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, input_header_opcode=42, input_payload=16, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, input_header_opcode=21, input_header_mode=1, input_payload=32, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, input_header_opcode=1, input_header_mode=2, input_payload=64, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, input_header_opcode=2, input_header_mode=3, input_payload=128, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, input_header_opcode=4, input_header_mode=4, input_payload=256, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, input_header_opcode=8, input_header_mode=5, input_payload=512, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, input_header_opcode=16, input_header_mode=6, input_payload=1024, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, input_header_opcode=32, input_header_mode=7, input_payload=2048, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, take=1, input_header_opcode=1, input_payload=4096, expected_ready=1, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, take=1, input_header_opcode=3, input_header_mode=1, input_payload=8192, expected_ready=1, expected_valid=1, expected_data_header_opcode=63, expected_data_header_mode=5, expected_data_payload=1),
            Stimulus(valid=1, take=1, input_header_opcode=7, input_header_mode=2, input_payload=16384, expected_ready=1, expected_valid=1, expected_data_header_opcode=1, expected_data_header_mode=1, expected_data_payload=4096),
            Stimulus(valid=1, input_header_opcode=15, input_header_mode=3, input_payload=32768, expected_valid=1, expected_data_header_opcode=3, expected_data_header_mode=2, expected_data_payload=8192),
            Stimulus(valid=1, take=1, input_header_opcode=31, input_header_mode=4, input_payload=65536, expected_ready=1, expected_valid=1, expected_data_header_opcode=3, expected_data_header_mode=2, expected_data_payload=8192),
            Stimulus(valid=1, take=1, input_header_opcode=63, input_header_mode=5, input_payload=1, expected_ready=1, expected_valid=1, expected_data_header_opcode=7, expected_data_header_mode=3, expected_data_payload=16384),
            Stimulus(valid=1, take=1, input_header_opcode=48, input_header_mode=6, input_payload=3, expected_ready=1, expected_valid=1, expected_data_header_opcode=31, expected_data_header_mode=5, expected_data_payload=65536),
            Stimulus(valid=1, input_header_opcode=27, input_header_mode=7, input_payload=7, expected_valid=1, expected_data_header_opcode=63, expected_data_header_mode=6, expected_data_payload=1),
            Stimulus(valid=1, take=1, input_header_opcode=6, input_payload=15, expected_ready=1, expected_valid=1, expected_data_header_opcode=63, expected_data_header_mode=6, expected_data_payload=1),
            Stimulus(valid=1, take=1, input_header_opcode=49, input_header_mode=1, input_payload=31, expected_ready=1, expected_valid=1, expected_data_header_opcode=48, expected_data_header_mode=7, expected_data_payload=3),
            Stimulus(valid=1, take=1, input_header_opcode=28, input_header_mode=2, input_payload=63, expected_ready=1, expected_valid=1, expected_data_header_opcode=6, expected_data_header_mode=1, expected_data_payload=15),
            Stimulus(valid=1, input_header_opcode=7, input_header_mode=3, input_payload=127, expected_valid=1, expected_data_header_opcode=49, expected_data_header_mode=2, expected_data_payload=31),
            Stimulus(valid=1, take=1, input_header_opcode=50, input_header_mode=4, input_payload=255, expected_ready=1, expected_valid=1, expected_data_header_opcode=49, expected_data_header_mode=2, expected_data_payload=31),
            Stimulus(valid=1, take=1, input_header_opcode=29, input_header_mode=5, input_payload=511, expected_ready=1, expected_valid=1, expected_data_header_opcode=28, expected_data_header_mode=3, expected_data_payload=63),
            Stimulus(valid=1, take=1, input_header_opcode=8, input_header_mode=6, input_payload=1023, expected_ready=1, expected_valid=1, expected_data_header_opcode=50, expected_data_header_mode=5, expected_data_payload=255),
            Stimulus(valid=1, input_header_opcode=51, input_header_mode=7, input_payload=2047, expected_valid=1, expected_data_header_opcode=29, expected_data_header_mode=6, expected_data_payload=511),
            Stimulus(valid=1, take=1, input_header_opcode=30, input_payload=4095, expected_ready=1, expected_valid=1, expected_data_header_opcode=29, expected_data_header_mode=6, expected_data_payload=511),
            Stimulus(valid=1, take=1, input_header_opcode=9, input_header_mode=1, input_payload=8191, expected_ready=1, expected_valid=1, expected_data_header_opcode=8, expected_data_header_mode=7, expected_data_payload=1023),
            Stimulus(take=1, input_header_opcode=52, input_header_mode=2, input_payload=16383, expected_ready=1, expected_valid=1, expected_data_header_opcode=30, expected_data_header_mode=1, expected_data_payload=4095),
            Stimulus(valid=1, input_header_opcode=31, input_header_mode=3, input_payload=32767, expected_ready=1, expected_valid=1, expected_data_header_opcode=9, expected_data_header_mode=2, expected_data_payload=8191),
            Stimulus(valid=1, take=1, input_header_opcode=10, input_header_mode=4, input_payload=65535, expected_ready=1, expected_valid=1, expected_data_header_opcode=9, expected_data_header_mode=2, expected_data_payload=8191),
            Stimulus(valid=1, take=1, input_header_opcode=53, input_header_mode=5, input_payload=131071, expected_ready=1, expected_valid=1, expected_data_header_opcode=31, expected_data_header_mode=4, expected_data_payload=32767),
            Stimulus(valid=1, take=1, input_header_opcode=32, input_header_mode=6, input_payload=92322, expected_ready=1, expected_valid=1, expected_data_header_opcode=10, expected_data_header_mode=5, expected_data_payload=65535),
            Stimulus(input_header_opcode=11, input_header_mode=7, input_payload=62605, expected_valid=1, expected_data_header_opcode=53, expected_data_header_mode=6, expected_data_payload=131071),
            Stimulus(valid=1, take=1, input_header_opcode=54, input_payload=28792, expected_ready=1, expected_valid=1, expected_data_header_opcode=53, expected_data_header_mode=6, expected_data_payload=131071),
            Stimulus(valid=1, take=1, input_header_opcode=33, input_header_mode=1, input_payload=117859, expected_ready=1, expected_valid=1, expected_data_header_opcode=32, expected_data_header_mode=7, expected_data_payload=92322),
            Stimulus(valid=1, take=1, input_header_opcode=12, input_header_mode=2, input_payload=88142, expected_ready=1, expected_valid=1, expected_data_header_opcode=54, expected_data_header_mode=1, expected_data_payload=28792),
            Stimulus(valid=1, input_header_opcode=55, input_header_mode=3, input_payload=50233, expected_valid=1, expected_data_header_opcode=33, expected_data_header_mode=2, expected_data_payload=117859),
            Stimulus(take=1, input_header_opcode=34, input_header_mode=4, input_payload=16420, expected_ready=1, expected_valid=1, expected_data_header_opcode=33, expected_data_header_mode=2, expected_data_payload=117859),
            Stimulus(valid=1, take=1, input_header_opcode=13, input_header_mode=5, input_payload=80911, expected_ready=1, expected_valid=1, expected_data_header_opcode=12, expected_data_header_mode=3, expected_data_payload=88142),
            Stimulus(valid=1, take=1, input_header_opcode=56, input_header_mode=6, input_payload=112634, expected_ready=1),
            Stimulus(valid=1, input_header_opcode=35, input_header_mode=7, input_payload=21477, expected_valid=1, expected_data_header_opcode=13, expected_data_header_mode=6, expected_data_payload=80911),
            Stimulus(valid=1, take=1, input_header_opcode=14, input_payload=57296, expected_ready=1, expected_valid=1, expected_data_header_opcode=13, expected_data_header_mode=6, expected_data_payload=80911),
            Stimulus(take=1, input_header_opcode=57, input_header_mode=1, input_payload=93115, expected_ready=1, expected_valid=1, expected_data_header_opcode=56, expected_data_header_mode=7, expected_data_payload=112634),
            Stimulus(valid=1, take=1, input_header_opcode=36, input_header_mode=2, input_payload=124838, expected_ready=1, expected_valid=1, expected_data_header_opcode=14, expected_data_header_mode=1, expected_data_payload=57296),
            Stimulus(valid=1, input_header_opcode=15, input_header_mode=3, input_payload=25489, expected_ready=1),
            Stimulus(valid=1, input_header_opcode=58, input_header_mode=4, input_payload=61308, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=37, input_header_mode=5, input_payload=72551, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(input_header_opcode=16, input_header_mode=6, input_payload=104274, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=59, input_header_mode=7, input_payload=13117, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=38, input_payload=48936, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=17, input_header_mode=1, input_payload=68371, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=60, input_header_mode=2, input_payload=100094, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(input_header_opcode=39, input_header_mode=3, input_payload=745, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=18, input_header_mode=4, input_payload=36564, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=61, input_header_mode=5, input_payload=64191, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=40, input_header_mode=6, input_payload=30378, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=19, input_header_mode=7, input_payload=4757, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(input_header_opcode=62, input_payload=106112, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=41, input_header_mode=1, input_payload=76395, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, input_header_opcode=20, input_header_mode=2, input_payload=42582, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, take=1, input_header_mode=3, input_payload=43690, expected_ready=1, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, take=1, input_header_opcode=63, input_header_mode=4, input_payload=1, expected_ready=1, expected_valid=1, expected_data_header_opcode=15, expected_data_header_mode=4, expected_data_payload=25489),
            Stimulus(valid=1, take=1, input_header_opcode=62, input_header_mode=5, input_payload=2, expected_ready=1, expected_valid=1, expected_data_header_mode=4, expected_data_payload=43690),
            Stimulus(valid=1, take=1, input_header_opcode=32, input_header_mode=6, input_payload=4, expected_ready=1, expected_valid=1, expected_data_header_opcode=63, expected_data_header_mode=5, expected_data_payload=1),
            Stimulus(valid=1, take=1, input_header_opcode=31, input_header_mode=7, input_payload=8, expected_ready=1, expected_valid=1, expected_data_header_opcode=62, expected_data_header_mode=6, expected_data_payload=2),
            Stimulus(valid=1, take=1, input_header_opcode=42, input_payload=16, expected_ready=1, expected_valid=1, expected_data_header_opcode=32, expected_data_header_mode=7, expected_data_payload=4),
            Stimulus(valid=1, take=1, input_header_opcode=21, input_header_mode=1, input_payload=32, expected_ready=1, expected_valid=1, expected_data_header_opcode=31, expected_data_payload=8),
            Stimulus(valid=1, take=1, input_header_opcode=1, input_header_mode=2, input_payload=64, expected_ready=1, expected_valid=1, expected_data_header_opcode=42, expected_data_header_mode=1, expected_data_payload=16),
            Stimulus(valid=1, take=1, input_header_opcode=2, input_header_mode=3, input_payload=128, expected_ready=1, expected_valid=1, expected_data_header_opcode=21, expected_data_header_mode=2, expected_data_payload=32),
            Stimulus(valid=1, take=1, input_header_opcode=4, input_header_mode=4, input_payload=256, expected_ready=1, expected_valid=1, expected_data_header_opcode=1, expected_data_header_mode=3, expected_data_payload=64),
            Stimulus(valid=1, take=1, input_header_opcode=8, input_header_mode=5, input_payload=512, expected_ready=1, expected_valid=1, expected_data_header_opcode=2, expected_data_header_mode=4, expected_data_payload=128),
            Stimulus(valid=1, take=1, input_header_opcode=16, input_header_mode=6, input_payload=1024, expected_ready=1, expected_valid=1, expected_data_header_opcode=4, expected_data_header_mode=5, expected_data_payload=256),
            Stimulus(valid=1, take=1, input_header_opcode=32, input_header_mode=7, input_payload=2048, expected_ready=1, expected_valid=1, expected_data_header_opcode=8, expected_data_header_mode=6, expected_data_payload=512),
            Stimulus(valid=1, take=1, input_header_opcode=1, input_payload=4096, expected_ready=1, expected_valid=1, expected_data_header_opcode=16, expected_data_header_mode=7, expected_data_payload=1024),
            Stimulus(valid=1, take=1, input_header_opcode=3, input_header_mode=1, input_payload=8192, expected_ready=1, expected_valid=1, expected_data_header_opcode=32, expected_data_payload=2048),
            Stimulus(valid=1, take=1, input_header_opcode=7, input_header_mode=2, input_payload=16384, expected_ready=1, expected_valid=1, expected_data_header_opcode=1, expected_data_header_mode=1, expected_data_payload=4096),
            Stimulus(valid=1, take=1, input_header_opcode=15, input_header_mode=3, input_payload=32768, expected_ready=1, expected_valid=1, expected_data_header_opcode=3, expected_data_header_mode=2, expected_data_payload=8192),
            Stimulus(valid=1, take=1, input_header_opcode=31, input_header_mode=4, input_payload=65536, expected_ready=1, expected_valid=1, expected_data_header_opcode=7, expected_data_header_mode=3, expected_data_payload=16384),
            Stimulus(valid=1, take=1, input_header_opcode=63, input_header_mode=5, input_payload=1, expected_ready=1, expected_valid=1, expected_data_header_opcode=15, expected_data_header_mode=4, expected_data_payload=32768),
            Stimulus(valid=1, take=1, input_header_opcode=48, input_header_mode=6, input_payload=3, expected_ready=1, expected_valid=1, expected_data_header_opcode=31, expected_data_header_mode=5, expected_data_payload=65536),
            Stimulus(valid=1, take=1, input_header_opcode=27, input_header_mode=7, input_payload=7, expected_ready=1, expected_valid=1, expected_data_header_opcode=63, expected_data_header_mode=6, expected_data_payload=1),
            Stimulus(valid=1, take=1, input_header_opcode=6, input_payload=15, expected_ready=1, expected_valid=1, expected_data_header_opcode=48, expected_data_header_mode=7, expected_data_payload=3),
            Stimulus(valid=1, take=1, input_header_opcode=49, input_header_mode=1, input_payload=31, expected_ready=1, expected_valid=1, expected_data_header_opcode=27, expected_data_payload=7),
            Stimulus(valid=1, take=1, input_header_opcode=28, input_header_mode=2, input_payload=63, expected_ready=1, expected_valid=1, expected_data_header_opcode=6, expected_data_header_mode=1, expected_data_payload=15),
            Stimulus(valid=1, take=1, input_header_opcode=7, input_header_mode=3, input_payload=127, expected_ready=1, expected_valid=1, expected_data_header_opcode=49, expected_data_header_mode=2, expected_data_payload=31),
            Stimulus(valid=1, take=1, input_header_opcode=50, input_header_mode=4, input_payload=255, expected_ready=1, expected_valid=1, expected_data_header_opcode=28, expected_data_header_mode=3, expected_data_payload=63),
            Stimulus(valid=1, take=1, input_header_opcode=29, input_header_mode=5, input_payload=511, expected_ready=1, expected_valid=1, expected_data_header_opcode=7, expected_data_header_mode=4, expected_data_payload=127),
            Stimulus(valid=1, take=1, input_header_opcode=8, input_header_mode=6, input_payload=1023, expected_ready=1, expected_valid=1, expected_data_header_opcode=50, expected_data_header_mode=5, expected_data_payload=255),
            Stimulus(valid=1, take=1, input_header_opcode=51, input_header_mode=7, input_payload=2047, expected_ready=1, expected_valid=1, expected_data_header_opcode=29, expected_data_header_mode=6, expected_data_payload=511),
            Stimulus(valid=1, take=1, input_header_opcode=30, input_payload=4095, expected_ready=1, expected_valid=1, expected_data_header_opcode=8, expected_data_header_mode=7, expected_data_payload=1023),
            Stimulus(valid=1, take=1, input_header_opcode=9, input_header_mode=1, input_payload=8191, expected_ready=1, expected_valid=1, expected_data_header_opcode=51, expected_data_payload=2047),
            Stimulus(valid=1, take=1, input_header_opcode=52, input_header_mode=2, input_payload=16383, expected_ready=1, expected_valid=1, expected_data_header_opcode=30, expected_data_header_mode=1, expected_data_payload=4095),
            Stimulus(valid=1, take=1, input_header_opcode=31, input_header_mode=3, input_payload=32767, expected_ready=1, expected_valid=1, expected_data_header_opcode=9, expected_data_header_mode=2, expected_data_payload=8191),
            Stimulus(valid=1, take=1, input_header_opcode=10, input_header_mode=4, input_payload=65535, expected_ready=1, expected_valid=1, expected_data_header_opcode=52, expected_data_header_mode=3, expected_data_payload=16383),
            Stimulus(valid=1, take=1, input_header_opcode=53, input_header_mode=5, input_payload=131071, expected_ready=1, expected_valid=1, expected_data_header_opcode=31, expected_data_header_mode=4, expected_data_payload=32767),
            Stimulus(valid=1, take=1, input_header_opcode=32, input_header_mode=6, input_payload=92322, expected_ready=1, expected_valid=1, expected_data_header_opcode=10, expected_data_header_mode=5, expected_data_payload=65535),
            Stimulus(valid=1, take=1, input_header_opcode=11, input_header_mode=7, input_payload=62605, expected_ready=1, expected_valid=1, expected_data_header_opcode=53, expected_data_header_mode=6, expected_data_payload=131071),
            Stimulus(valid=1, take=1, input_header_opcode=54, input_payload=28792, expected_ready=1, expected_valid=1, expected_data_header_opcode=32, expected_data_header_mode=7, expected_data_payload=92322),
            Stimulus(valid=1, take=1, input_header_opcode=33, input_header_mode=1, input_payload=117859, expected_ready=1, expected_valid=1, expected_data_header_opcode=11, expected_data_payload=62605),
            Stimulus(valid=1, take=1, input_header_opcode=12, input_header_mode=2, input_payload=88142, expected_ready=1, expected_valid=1, expected_data_header_opcode=54, expected_data_header_mode=1, expected_data_payload=28792),
            Stimulus(valid=1, take=1, input_header_opcode=55, input_header_mode=3, input_payload=50233, expected_ready=1, expected_valid=1, expected_data_header_opcode=33, expected_data_header_mode=2, expected_data_payload=117859),
            Stimulus(valid=1, take=1, input_header_opcode=34, input_header_mode=4, input_payload=16420, expected_ready=1, expected_valid=1, expected_data_header_opcode=12, expected_data_header_mode=3, expected_data_payload=88142),
            Stimulus(valid=1, take=1, input_header_opcode=13, input_header_mode=5, input_payload=80911, expected_ready=1, expected_valid=1, expected_data_header_opcode=55, expected_data_header_mode=4, expected_data_payload=50233),
            Stimulus(valid=1, take=1, input_header_opcode=56, input_header_mode=6, input_payload=112634, expected_ready=1, expected_valid=1, expected_data_header_opcode=34, expected_data_header_mode=5, expected_data_payload=16420),
            Stimulus(valid=1, take=1, input_header_opcode=35, input_header_mode=7, input_payload=21477, expected_ready=1, expected_valid=1, expected_data_header_opcode=13, expected_data_header_mode=6, expected_data_payload=80911),
            Stimulus(valid=1, take=1, input_header_opcode=14, input_payload=57296, expected_ready=1, expected_valid=1, expected_data_header_opcode=56, expected_data_header_mode=7, expected_data_payload=112634),
            Stimulus(valid=1, take=1, input_header_opcode=57, input_header_mode=1, input_payload=93115, expected_ready=1, expected_valid=1, expected_data_header_opcode=35, expected_data_payload=21477),
            Stimulus(valid=1, take=1, input_header_opcode=36, input_header_mode=2, input_payload=124838, expected_ready=1, expected_valid=1, expected_data_header_opcode=14, expected_data_header_mode=1, expected_data_payload=57296),
            Stimulus(valid=1, take=1, input_header_opcode=15, input_header_mode=3, input_payload=25489, expected_ready=1, expected_valid=1, expected_data_header_opcode=57, expected_data_header_mode=2, expected_data_payload=93115),
            Stimulus(valid=1, take=1, input_header_opcode=58, input_header_mode=4, input_payload=61308, expected_ready=1, expected_valid=1, expected_data_header_opcode=36, expected_data_header_mode=3, expected_data_payload=124838),
            Stimulus(valid=1, take=1, input_header_opcode=37, input_header_mode=5, input_payload=72551, expected_ready=1, expected_valid=1, expected_data_header_opcode=15, expected_data_header_mode=4, expected_data_payload=25489),
            Stimulus(valid=1, take=1, input_header_opcode=16, input_header_mode=6, input_payload=104274, expected_ready=1, expected_valid=1, expected_data_header_opcode=58, expected_data_header_mode=5, expected_data_payload=61308),
            Stimulus(valid=1, take=1, input_header_opcode=59, input_header_mode=7, input_payload=13117, expected_ready=1, expected_valid=1, expected_data_header_opcode=37, expected_data_header_mode=6, expected_data_payload=72551),
            Stimulus(valid=1, take=1, input_header_opcode=38, input_payload=48936, expected_ready=1, expected_valid=1, expected_data_header_opcode=16, expected_data_header_mode=7, expected_data_payload=104274),
            Stimulus(valid=1, take=1, input_header_opcode=17, input_header_mode=1, input_payload=68371, expected_ready=1, expected_valid=1, expected_data_header_opcode=59, expected_data_payload=13117),
            Stimulus(valid=1, take=1, input_header_opcode=60, input_header_mode=2, input_payload=100094, expected_ready=1, expected_valid=1, expected_data_header_opcode=38, expected_data_header_mode=1, expected_data_payload=48936),
            Stimulus(valid=1, take=1, input_header_opcode=39, input_header_mode=3, input_payload=745, expected_ready=1, expected_valid=1, expected_data_header_opcode=17, expected_data_header_mode=2, expected_data_payload=68371),
            Stimulus(valid=1, take=1, input_header_opcode=18, input_header_mode=4, input_payload=36564, expected_ready=1, expected_valid=1, expected_data_header_opcode=60, expected_data_header_mode=3, expected_data_payload=100094),
            Stimulus(valid=1, take=1, input_header_opcode=61, input_header_mode=5, input_payload=64191, expected_ready=1, expected_valid=1, expected_data_header_opcode=39, expected_data_header_mode=4, expected_data_payload=745),
            Stimulus(valid=1, take=1, input_header_opcode=40, input_header_mode=6, input_payload=30378, expected_ready=1, expected_valid=1, expected_data_header_opcode=18, expected_data_header_mode=5, expected_data_payload=36564),
            Stimulus(valid=1, take=1, input_header_opcode=19, input_header_mode=7, input_payload=4757, expected_ready=1, expected_valid=1, expected_data_header_opcode=61, expected_data_header_mode=6, expected_data_payload=64191),
            Stimulus(valid=1, take=1, input_header_opcode=62, input_payload=106112, expected_ready=1, expected_valid=1, expected_data_header_opcode=40, expected_data_header_mode=7, expected_data_payload=30378),
            Stimulus(valid=1, take=1, input_header_opcode=41, input_header_mode=1, input_payload=76395, expected_ready=1, expected_valid=1, expected_data_header_opcode=19, expected_data_payload=4757),
            Stimulus(valid=1, take=1, input_header_opcode=20, input_header_mode=2, input_payload=42582, expected_ready=1, expected_valid=1, expected_data_header_opcode=62, expected_data_header_mode=1, expected_data_payload=106112),
            Stimulus(valid=1, take=1, input_header_opcode=63, input_header_mode=3, input_payload=8769, expected_ready=1, expected_valid=1, expected_data_header_opcode=41, expected_data_header_mode=2, expected_data_payload=76395),
            Stimulus(valid=1, take=1, input_header_opcode=42, input_header_mode=4, input_payload=110124, expected_ready=1, expected_valid=1, expected_data_header_opcode=20, expected_data_header_mode=3, expected_data_payload=42582),
            Stimulus(valid=1, take=1, input_header_opcode=21, input_header_mode=5, input_payload=88599, expected_ready=1, expected_valid=1, expected_data_header_opcode=63, expected_data_header_mode=4, expected_data_payload=8769),
            Stimulus(valid=1, take=1, input_header_mode=6, input_payload=54786, expected_ready=1, expected_valid=1, expected_data_header_opcode=42, expected_data_header_mode=5, expected_data_payload=110124),
            Stimulus(valid=1, take=1, input_header_opcode=43, input_header_mode=7, input_payload=29165, expected_ready=1, expected_valid=1, expected_data_header_opcode=21, expected_data_header_mode=6, expected_data_payload=88599),
            Stimulus(valid=1, take=1, input_header_opcode=22, input_payload=130520, expected_ready=1, expected_valid=1, expected_data_header_mode=7, expected_data_payload=54786),
            Stimulus(valid=1, take=1, input_header_opcode=1, input_header_mode=1, input_payload=84419, expected_ready=1, expected_valid=1, expected_data_header_opcode=43, expected_data_payload=29165),
            Stimulus(valid=1, take=1, input_header_opcode=44, input_header_mode=2, input_payload=50606, expected_ready=1, expected_valid=1, expected_data_header_opcode=22, expected_data_header_mode=1, expected_data_payload=130520),
            Stimulus(valid=1, take=1, input_header_opcode=23, input_header_mode=3, input_payload=16793, expected_ready=1, expected_valid=1, expected_data_header_opcode=1, expected_data_header_mode=2, expected_data_payload=84419),
            Stimulus(valid=1, take=1, input_header_opcode=2, input_header_mode=4, input_payload=118148, expected_ready=1, expected_valid=1, expected_data_header_opcode=44, expected_data_header_mode=3, expected_data_payload=50606),
            Stimulus(valid=1, take=1, input_header_opcode=45, input_header_mode=5, input_payload=113007, expected_ready=1, expected_valid=1, expected_data_header_opcode=23, expected_data_header_mode=4, expected_data_payload=16793),
            Stimulus(valid=1, take=1, input_header_opcode=24, input_header_mode=6, input_payload=13658, expected_ready=1, expected_valid=1, expected_data_header_opcode=2, expected_data_header_mode=5, expected_data_payload=118148),
            Stimulus(valid=1, take=1, input_header_opcode=3, input_header_mode=7, input_payload=53573, expected_ready=1, expected_valid=1, expected_data_header_opcode=45, expected_data_header_mode=6, expected_data_payload=113007),
            Stimulus(valid=1, take=1, input_header_opcode=46, input_payload=89392, expected_ready=1, expected_valid=1, expected_data_header_opcode=24, expected_data_header_mode=7, expected_data_payload=13658),
            Stimulus(valid=1, take=1, input_header_opcode=25, input_header_mode=1, input_payload=125211, expected_ready=1, expected_valid=1, expected_data_header_opcode=3, expected_data_payload=53573),
            Stimulus(valid=1, take=1, input_header_opcode=4, input_header_mode=2, input_payload=25862, expected_ready=1, expected_valid=1, expected_data_header_opcode=46, expected_data_header_mode=1, expected_data_payload=89392),
            Stimulus(valid=1, take=1, input_header_opcode=47, input_header_mode=3, input_payload=57585, expected_ready=1, expected_valid=1, expected_data_header_opcode=25, expected_data_header_mode=2, expected_data_payload=125211),
            Stimulus(valid=1, take=1, input_header_opcode=26, input_header_mode=4, input_payload=93404, expected_ready=1, expected_valid=1, expected_data_header_opcode=4, expected_data_header_mode=3, expected_data_payload=25862),
            Stimulus(valid=1, take=1, input_header_opcode=5, input_header_mode=5, input_payload=104647, expected_ready=1, expected_valid=1, expected_data_header_opcode=47, expected_data_header_mode=4, expected_data_payload=57585),
            Stimulus(valid=1, take=1, input_header_opcode=48, input_header_mode=6, input_payload=5298, expected_ready=1, expected_valid=1, expected_data_header_opcode=26, expected_data_header_mode=5, expected_data_payload=93404),
            Stimulus(valid=1, take=1, input_header_opcode=27, input_header_mode=7, input_payload=45213, expected_ready=1, expected_valid=1, expected_data_header_opcode=5, expected_data_header_mode=6, expected_data_payload=104647),
            Stimulus(valid=1, take=1, input_header_opcode=6, input_payload=81032, expected_ready=1, expected_valid=1, expected_data_header_opcode=48, expected_data_header_mode=7, expected_data_payload=5298),
            Stimulus(valid=1, take=1, input_header_opcode=49, input_header_mode=1, input_payload=100467, expected_ready=1, expected_valid=1, expected_data_header_opcode=27, expected_data_payload=45213),
            Stimulus(valid=1, take=1, input_header_opcode=28, input_header_mode=2, input_payload=1118, expected_ready=1, expected_valid=1, expected_data_header_opcode=6, expected_data_header_mode=1, expected_data_payload=81032),
            Stimulus(valid=1, take=1, input_header_opcode=7, input_header_mode=3, input_payload=32841, expected_ready=1, expected_valid=1, expected_data_header_opcode=49, expected_data_header_mode=2, expected_data_payload=100467),
            Stimulus(valid=1, take=1, input_header_opcode=50, input_header_mode=4, input_payload=68660, expected_ready=1, expected_valid=1, expected_data_header_opcode=28, expected_data_header_mode=3, expected_data_payload=1118),
            Stimulus(valid=1, take=1, input_header_opcode=29, input_header_mode=5, input_payload=30751, expected_ready=1, expected_valid=1, expected_data_header_opcode=7, expected_data_header_mode=4, expected_data_payload=32841),
            Stimulus(valid=1, take=1, input_header_opcode=8, input_header_mode=6, input_payload=128010, expected_ready=1, expected_valid=1, expected_data_header_opcode=50, expected_data_header_mode=5, expected_data_payload=68660),
            Stimulus(valid=1, take=1, input_header_opcode=51, input_header_mode=7, input_payload=94197, expected_ready=1, expected_valid=1, expected_data_header_opcode=29, expected_data_header_mode=6, expected_data_payload=30751),
            Stimulus(valid=1, take=1, input_header_opcode=30, input_payload=72672, expected_ready=1, expected_valid=1, expected_data_header_opcode=8, expected_data_header_mode=7, expected_data_payload=128010),
            Stimulus(valid=1, take=1, input_header_opcode=9, input_header_mode=1, input_payload=42955, expected_ready=1, expected_valid=1, expected_data_header_opcode=51, expected_data_payload=94197),
            Stimulus(valid=1, take=1, input_header_opcode=52, input_header_mode=2, input_payload=9142, expected_ready=1, expected_valid=1, expected_data_header_opcode=30, expected_data_header_mode=1, expected_data_payload=72672),
            Stimulus(take=1, input_header_opcode=31, input_header_mode=3, input_payload=114593, expected_ready=1, expected_valid=1, expected_data_header_opcode=9, expected_data_header_mode=2, expected_data_payload=42955),
            Stimulus(take=1, input_header_opcode=10, input_header_mode=4, input_payload=76684, expected_ready=1, expected_valid=1, expected_data_header_opcode=52, expected_data_header_mode=3, expected_data_payload=9142),
            Stimulus(take=1, input_header_opcode=53, input_header_mode=5, input_payload=55159, expected_ready=1),
            Stimulus(take=1, input_header_opcode=32, input_header_mode=6, input_payload=21346, expected_ready=1),
            Stimulus(take=1, input_header_opcode=11, input_header_mode=7, input_payload=118605, expected_ready=1),
            Stimulus(take=1, input_header_opcode=54, input_payload=97080, expected_ready=1),
            Stimulus(take=1, input_header_opcode=33, input_header_mode=1, input_payload=50979, expected_ready=1),
            Stimulus(take=1, input_header_opcode=12, input_header_mode=2, input_payload=17166, expected_ready=1),
            Stimulus(take=1, input_header_opcode=55, input_header_mode=3, input_payload=122617, expected_ready=1),
            Stimulus(take=1, input_header_opcode=34, input_header_mode=4, input_payload=84708, expected_ready=1),
            Stimulus(take=1, input_header_opcode=13, input_header_mode=5, input_payload=14031, expected_ready=1),
            Stimulus(take=1, input_header_opcode=56, input_header_mode=6, input_payload=45754, expected_ready=1),
            Stimulus(take=1, input_header_opcode=35, input_header_mode=7, input_payload=77477, expected_ready=1),
            Stimulus(take=1, input_header_opcode=14, input_payload=121488, expected_ready=1),
            Stimulus(take=1, input_header_opcode=57, input_header_mode=1, input_payload=26235, expected_ready=1),
            Stimulus(take=1, input_header_opcode=36, input_header_mode=2, input_payload=57958, expected_ready=1),
            Stimulus(take=1, input_header_opcode=15, input_header_mode=3, input_payload=97873, expected_ready=1),
            Stimulus(take=1, input_header_opcode=58, input_header_mode=4, input_payload=125500, expected_ready=1),
            Stimulus(take=1, input_header_opcode=37, input_header_mode=5, input_payload=5671, expected_ready=1),
            Stimulus(take=1, input_header_opcode=16, input_header_mode=6, input_payload=37394, expected_ready=1),
            Stimulus(take=1, input_header_opcode=59, input_header_mode=7, input_payload=69117, expected_ready=1),
            Stimulus(take=1, input_header_opcode=38, input_payload=113128, expected_ready=1),
            Stimulus(take=1, input_header_opcode=17, input_header_mode=1, input_payload=1491, expected_ready=1),
            Stimulus(take=1, input_header_opcode=60, input_header_mode=2, input_payload=33214, expected_ready=1),
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
def NestedPayloadPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Packet(
        header=Header(opcode=frame.input_header_opcode, mode=frame.input_header_mode),
        payload=frame.input_payload,
    )
    dut = NestedPayloadPipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, (
            "nested_payload_pipeline input capacity"
        )
        assert dut.valid == frame.expected_valid, (
            "nested_payload_pipeline result availability"
        )
        assert dut.data.header.opcode == frame.expected_data_header_opcode, (
            "nested_payload_pipeline header.opcode old-state check"
        )
        assert dut.data.header.mode == frame.expected_data_header_mode, (
            "nested_payload_pipeline header.mode old-state check"
        )
        assert dut.data.payload == frame.expected_data_payload, (
            "nested_payload_pipeline payload old-state check"
        )
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "header_opcode", dut.data.header.opcode)
        log("info", "header_mode", dut.data.header.mode)
        log("info", "payload", dut.data.payload)

    advance(phase)
    check()

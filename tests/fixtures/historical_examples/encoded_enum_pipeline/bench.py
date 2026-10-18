"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_encoded_enum_pipeline.encoded_enum_pipeline import (
    EncodedEnumPipeline,
    Command,
    Result,
    Opcode,
    WideOpcode,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_opcode: bits[4]
    input_selected: bits[4]
    input_wide: bits[64]
    input_matched: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_opcode: bits[4]
    expected_data_selected: bits[4]
    expected_data_wide: bits[64]
    expected_data_matched: bits[1]


@struct
class Stimuli:
    rows: table[185, Stimulus]


def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_wide=9223372036854775807, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_wide=9223372036854775807, input_matched=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_wide=9223372036854775807, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_wide=9223372036854775807, input_matched=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_wide=9223372036854775807, input_matched=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(input_wide=9223372036854775807, input_matched=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(input_wide=9223372036854775807, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_wide=9223372036854775807, input_matched=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_wide=9223372036854775807, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_wide=9223372036854775807, input_matched=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, input_wide=9223372036854775807, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_wide=9223372036854775807, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(valid=1, take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(valid=1, take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1, expected_valid=1, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1, expected_valid=1, expected_data_opcode=3, expected_data_selected=9, expected_data_wide=18446744073709551615, expected_data_matched=1),
            Stimulus(take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1, expected_valid=1, expected_data_opcode=9, expected_data_selected=9, expected_data_wide=18446744073709551615),
            Stimulus(take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_wide=9223372036854775807, expected_ready=1),
            Stimulus(take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1),
            Stimulus(take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1),
            Stimulus(take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_wide=9223372036854775807, expected_ready=1),
            Stimulus(take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1),
            Stimulus(take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1),
            Stimulus(take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_wide=9223372036854775807, expected_ready=1),
            Stimulus(take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1),
            Stimulus(take=1, input_wide=9223372036854775807, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, expected_ready=1),
            Stimulus(take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_wide=9223372036854775807, expected_ready=1),
            Stimulus(take=1, input_opcode=3, input_selected=3, input_wide=9223372036854775808, input_matched=1, expected_ready=1),
            Stimulus(take=1, input_opcode=9, input_selected=9, input_wide=18446744073709551615, expected_ready=1),
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
def EncodedEnumPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Command(
        opcode=(
            Opcode.NONE
            if frame.input_opcode == 0
            else (Opcode.READ if frame.input_opcode == 3 else Opcode.WRITE)
        ),
        selected=(
            Opcode.NONE
            if frame.input_selected == 0
            else (Opcode.READ if frame.input_selected == 3 else Opcode.WRITE)
        ),
        wide=(
            WideOpcode.LOW
            if frame.input_wide == 9223372036854775807
            else (
                WideOpcode.HIGH
                if frame.input_wide == 9223372036854775808
                else WideOpcode.MAX
            )
        ),
        matched=frame.input_matched,
    )
    dut = EncodedEnumPipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "encoded_enum_pipeline input capacity"
        assert dut.valid == frame.expected_valid, (
            "encoded_enum_pipeline result availability"
        )
        assert enum_to_bits(dut.data.opcode) == frame.expected_data_opcode, (
            "encoded_enum_pipeline opcode old-state check"
        )
        assert enum_to_bits(dut.data.selected) == frame.expected_data_selected, (
            "encoded_enum_pipeline selected old-state check"
        )
        assert enum_to_bits(dut.data.wide) == frame.expected_data_wide, (
            "encoded_enum_pipeline wide old-state check"
        )
        assert dut.data.matched == frame.expected_data_matched, (
            "encoded_enum_pipeline matched old-state check"
        )
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "opcode", enum_to_bits(dut.data.opcode))
        log("info", "selected", enum_to_bits(dut.data.selected))
        log("info", "wide", enum_to_bits(dut.data.wide))
        log("info", "matched", dut.data.matched)

    advance(phase)
    check()

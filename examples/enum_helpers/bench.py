"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_enum_helpers.enum_helpers import (
    EnumHelpers,
    Command,
    EnumResult,
    Result,
    Opcode,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_raw_opcode: bits[4]
    input_onehot_mask: bits[2]
    input_selector: bits[4]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_decoded: bits[4]
    expected_data_decoded_valid: bits[1]
    expected_data_onehot: bits[4]
    expected_data_onehot_present: bits[1]
    expected_data_onehot_conflict: bits[1]
    expected_data_selected: bits[1]
    expected_data_classification: bits[8]


@struct
class Stimuli:
    rows: table[185, Stimulus]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, input_onehot_mask=3, input_selector=9, expected_ready=1),
            Stimulus(valid=1, input_raw_opcode=1, input_selector=15, expected_ready=1),
            Stimulus(valid=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, input_raw_opcode=5, input_selector=15, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, input_raw_opcode=9, input_selector=15, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=13, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=1, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=5, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=3, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=9, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=9, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=13, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(take=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=1, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=5, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=3, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(take=1, input_raw_opcode=9, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_ready=1),
            Stimulus(valid=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=13, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(take=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, input_onehot_mask=3, input_selector=9, expected_ready=1),
            Stimulus(valid=1, input_raw_opcode=1, input_selector=15, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=5, input_selector=15, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=9, input_selector=15, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(input_raw_opcode=13, input_selector=15, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=1, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=5, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=3, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=9, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=9, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=13, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=1, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=5, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=3, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=9, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=9, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=13, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=1, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=5, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=3, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=9, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=9, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=13, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=1, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=5, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=3, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=9, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=9, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=13, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=1, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=5, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=3, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=9, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=9, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=13, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=1, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=5, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=3, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=9, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=9, expected_data_decoded_valid=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(valid=1, take=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(valid=1, take=1, input_raw_opcode=13, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(valid=1, take=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=15, expected_data_onehot_present=1, expected_data_onehot_conflict=1, expected_data_selected=1, expected_data_classification=12),
            Stimulus(valid=1, take=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=1, expected_data_classification=13),
            Stimulus(take=1, input_onehot_mask=3, input_selector=9, expected_ready=1, expected_valid=1, expected_data_decoded=1, expected_data_onehot=3, expected_data_onehot_present=1, expected_data_classification=10),
            Stimulus(take=1, input_raw_opcode=1, input_selector=15, expected_ready=1, expected_valid=1, expected_data_decoded=15, expected_data_decoded_valid=1, expected_data_onehot=9, expected_data_onehot_present=1, expected_data_selected=1, expected_data_classification=11),
            Stimulus(take=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=5, input_selector=15, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=8, input_onehot_mask=3, input_selector=9, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=9, input_selector=15, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=10, input_onehot_mask=1, input_selector=1, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=11, input_onehot_mask=2, input_selector=3, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=12, input_onehot_mask=3, input_selector=9, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=13, input_selector=15, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=14, input_onehot_mask=1, input_selector=1, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=15, input_onehot_mask=2, input_selector=3, expected_ready=1),
            Stimulus(take=1, input_onehot_mask=3, input_selector=9, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=1, input_selector=15, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=2, input_onehot_mask=1, input_selector=1, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=3, input_onehot_mask=2, input_selector=3, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=4, input_onehot_mask=3, input_selector=9, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=5, input_selector=15, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=6, input_onehot_mask=1, input_selector=1, expected_ready=1),
            Stimulus(take=1, input_raw_opcode=7, input_onehot_mask=2, input_selector=3, expected_ready=1),
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
def EnumHelpersSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Command(
        raw_opcode=frame.input_raw_opcode,
        onehot_mask=frame.input_onehot_mask,
        selector=(
            Opcode.NONE
            if frame.input_selector == 1
            else (
                Opcode.READ
                if frame.input_selector == 3
                else (Opcode.WRITE if frame.input_selector == 9 else Opcode.ERROR)
            )
        ),
    )
    dut = EnumHelpers(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "enum_helpers input capacity"
        assert dut.valid == frame.expected_valid, "enum_helpers result availability"
        assert (
            enum_to_bits(dut.data.decoded) == frame.expected_data_decoded
        ), "enum_helpers decoded old-state check"
        assert (
            dut.data.decoded_valid == frame.expected_data_decoded_valid
        ), "enum_helpers decoded_valid old-state check"
        assert (
            enum_to_bits(dut.data.onehot) == frame.expected_data_onehot
        ), "enum_helpers onehot old-state check"
        assert (
            dut.data.onehot_present == frame.expected_data_onehot_present
        ), "enum_helpers onehot_present old-state check"
        assert (
            dut.data.onehot_conflict == frame.expected_data_onehot_conflict
        ), "enum_helpers onehot_conflict old-state check"
        assert (
            dut.data.selected == frame.expected_data_selected
        ), "enum_helpers selected old-state check"
        assert (
            dut.data.classification == frame.expected_data_classification
        ), "enum_helpers classification old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "decoded", enum_to_bits(dut.data.decoded))
        log("info", "decoded_valid", dut.data.decoded_valid)
        log("info", "onehot", enum_to_bits(dut.data.onehot))
        log("info", "onehot_present", dut.data.onehot_present)
        log("info", "onehot_conflict", dut.data.onehot_conflict)
        log("info", "selected", dut.data.selected)
        log("info", "classification", dut.data.classification)

    advance(phase)
    check()

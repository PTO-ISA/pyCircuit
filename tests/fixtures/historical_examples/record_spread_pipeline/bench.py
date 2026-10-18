"""Regular-clock full known-input stream from the retained record_spread_pipeline oracle."""

from example_record_spread_pipeline.record_spread_pipeline import (
    RecordSpreadPipeline,
    Packet,
    Header,
    Payload,
    Patch,
)
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    base_valid: bits[1]
    base_opcode: bits[4]
    base_tag: bits[4]
    base_data: bits[16]
    base_present: bits[1]
    header_valid: bits[1]
    header_opcode: bits[4]
    header_tag: bits[4]
    payload_valid: bits[1]
    payload_data: bits[16]
    patch_valid: bits[1]
    patch_tag: bits[4]
    patch_present: bits[1]
    take: bits[1]
    expected_base_ready: bits[1]
    expected_header_ready: bits[1]
    expected_payload_ready: bits[1]
    expected_patch_ready: bits[1]
    expected_valid: bits[1]
    expected_data_opcode: bits[4]
    expected_data_tag: bits[4]
    expected_data_data: bits[16]
    expected_data_valid: bits[1]


@struct
class Scenarios:
    rows: table[158, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseRecordSpreadPipeline():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(header_valid=1, payload_valid=1, patch_valid=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_opcode=15, base_tag=15, base_data=65535, base_present=1, header_valid=1, header_opcode=15, header_tag=15, payload_valid=1, payload_data=65535, patch_valid=1, patch_tag=15, patch_present=1, take=1, expected_base_ready=1),
            Scenario(base_valid=1, take=1, expected_base_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1),
            Scenario(expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=15, base_tag=15, base_data=65535, base_present=1, header_opcode=15, header_tag=15, payload_valid=1, payload_data=65535, patch_valid=1, patch_tag=15, patch_present=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=15, base_tag=8, base_data=32768, header_opcode=15, payload_valid=1, payload_data=32768, patch_valid=1, patch_tag=15, take=1, expected_header_ready=1),
            Scenario(base_opcode=15, base_tag=15, base_data=65535, base_present=1, header_valid=1, header_opcode=15, header_tag=15, payload_data=65535, patch_tag=15, patch_present=1, take=1, expected_header_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=15, expected_data_tag=15, expected_data_data=65535, expected_data_valid=1),
            Scenario(expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=15, base_tag=8, base_data=32768, header_valid=1, header_opcode=15, payload_data=32768, patch_valid=1, patch_tag=15, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=5, base_tag=10, base_data=23205, base_present=1, header_valid=1, header_opcode=5, header_tag=10, payload_data=23205, patch_valid=1, patch_tag=5, patch_present=1, take=1, expected_payload_ready=1),
            Scenario(base_opcode=15, base_tag=8, base_data=32768, header_opcode=15, payload_valid=1, payload_data=32768, patch_tag=15, take=1, expected_payload_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=15, expected_data_tag=15, expected_data_data=32768),
            Scenario(expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=5, base_tag=10, base_data=23205, base_present=1, header_valid=1, header_opcode=5, header_tag=10, payload_valid=1, payload_data=23205, patch_tag=5, patch_present=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=8, base_tag=1, base_data=1, header_valid=1, header_opcode=8, header_tag=1, payload_valid=1, payload_data=1, patch_tag=8, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_opcode=5, base_tag=10, base_data=23205, base_present=1, header_opcode=5, header_tag=10, payload_data=23205, patch_valid=1, patch_tag=5, patch_present=1, take=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=5, expected_data_tag=5, expected_data_data=23205, expected_data_valid=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, header_valid=1, payload_valid=1, patch_valid=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=15, base_tag=15, base_data=65535, base_present=1, header_valid=1, header_opcode=15, header_tag=15, payload_valid=1, payload_data=65535, patch_valid=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=15, base_tag=8, base_data=32768, header_valid=1, header_opcode=15, payload_valid=1, payload_data=32768, patch_valid=1, patch_tag=15, patch_present=1, expected_valid=1, expected_data_opcode=8, expected_data_data=1),
            Scenario(base_valid=1, base_opcode=5, base_tag=10, base_data=23205, base_present=1, header_valid=1, header_opcode=5, header_tag=10, payload_valid=1, payload_data=23205, patch_valid=1, patch_tag=5, patch_present=1, expected_valid=1, expected_data_opcode=8, expected_data_data=1),
            Scenario(base_valid=1, base_opcode=5, base_tag=10, base_data=23205, base_present=1, header_valid=1, header_opcode=5, header_tag=10, payload_valid=1, payload_data=23205, patch_valid=1, patch_tag=15, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_data=1),
            Scenario(base_valid=1, base_opcode=8, base_tag=1, base_data=1, header_valid=1, header_opcode=8, header_tag=1, payload_valid=1, payload_data=1, patch_valid=1, patch_tag=5, patch_present=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=15, expected_data_tag=15, expected_data_data=65535),
            Scenario(base_valid=1, header_valid=1, payload_valid=1, patch_valid=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=5, expected_data_tag=5, expected_data_data=23205, expected_data_valid=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_data=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=15, base_tag=15, base_data=65535, base_present=1, header_valid=1, header_opcode=15, header_tag=15, payload_valid=1, payload_data=65535, patch_valid=1, patch_tag=15, patch_present=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=15, expected_data_valid=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=15, base_tag=8, base_data=32768, header_valid=1, header_opcode=15, payload_valid=1, payload_data=32768, patch_valid=1, patch_tag=15, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=15, expected_data_tag=15, expected_data_data=65535),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=5, base_tag=10, base_data=23205, base_present=1, header_valid=1, header_opcode=5, header_tag=10, payload_valid=1, payload_data=23205, patch_valid=1, patch_tag=5, patch_present=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=15, expected_data_tag=5, expected_data_data=32768, expected_data_valid=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=8, base_tag=1, base_data=1, header_valid=1, header_opcode=8, header_tag=1, payload_valid=1, payload_data=1, patch_valid=1, patch_tag=8, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=5, expected_data_tag=8, expected_data_data=23205),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_present=1, header_valid=1, header_tag=1, payload_valid=1, payload_data=1, patch_valid=1, patch_present=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_data=1, expected_data_valid=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=1, header_valid=1, header_tag=2, payload_valid=1, payload_data=2, patch_valid=1, patch_tag=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=1, expected_data_data=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=2, header_valid=1, header_tag=4, payload_valid=1, payload_data=4, patch_valid=1, patch_tag=2, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=2, expected_data_data=2),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=4, header_valid=1, header_tag=8, payload_valid=1, payload_data=8, patch_valid=1, patch_tag=4, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=4, expected_data_data=4),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=8, header_valid=1, header_opcode=1, payload_valid=1, payload_data=16, patch_valid=1, patch_tag=8, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=8, expected_data_data=8),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=16, header_valid=1, header_opcode=2, payload_valid=1, payload_data=32, patch_valid=1, patch_present=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=1, expected_data_data=16, expected_data_valid=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=32, header_valid=1, header_opcode=4, payload_valid=1, payload_data=64, patch_valid=1, patch_tag=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=2, expected_data_tag=1, expected_data_data=32),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=64, header_valid=1, header_opcode=8, payload_valid=1, payload_data=128, patch_valid=1, patch_tag=2, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=4, expected_data_tag=2, expected_data_data=64),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=128, header_valid=1, header_tag=1, payload_valid=1, payload_data=256, patch_valid=1, patch_tag=4, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_tag=4, expected_data_data=128),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=256, header_valid=1, header_tag=2, payload_valid=1, payload_data=512, patch_valid=1, patch_tag=8, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=8, expected_data_data=256),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=512, header_valid=1, header_tag=4, payload_valid=1, payload_data=1024, patch_valid=1, patch_present=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_data=512, expected_data_valid=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=1024, header_valid=1, header_tag=8, payload_valid=1, payload_data=2048, patch_valid=1, patch_tag=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=1, expected_data_data=1024),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=2048, header_valid=1, header_opcode=1, payload_valid=1, payload_data=4096, patch_valid=1, patch_tag=2, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=2, expected_data_data=2048),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=4096, header_valid=1, header_opcode=2, payload_valid=1, payload_data=8192, patch_valid=1, patch_tag=4, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=1, expected_data_tag=4, expected_data_data=4096),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=8192, header_valid=1, header_opcode=4, payload_valid=1, payload_data=16384, patch_valid=1, patch_tag=8, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=2, expected_data_tag=8, expected_data_data=8192),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=16384, header_valid=1, header_opcode=8, payload_valid=1, payload_data=32768, patch_valid=1, patch_present=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=4, expected_data_data=16384, expected_data_valid=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_data=32768, header_valid=1, header_tag=1, payload_valid=1, payload_data=1, patch_valid=1, patch_tag=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_tag=1, expected_data_data=32768),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_tag=1, header_valid=1, header_tag=2, payload_valid=1, payload_data=2, patch_valid=1, patch_tag=2, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=2, expected_data_data=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_tag=2, header_valid=1, header_tag=4, payload_valid=1, payload_data=4, patch_valid=1, patch_tag=4, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=4, expected_data_data=2),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_tag=4, header_valid=1, header_tag=8, payload_valid=1, payload_data=8, patch_valid=1, patch_tag=8, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_tag=8, expected_data_data=4),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_tag=8, header_valid=1, header_opcode=1, payload_valid=1, payload_data=16, patch_valid=1, patch_present=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_data=8, expected_data_valid=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=1, header_valid=1, header_opcode=2, payload_valid=1, payload_data=32, patch_valid=1, patch_tag=1, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=1, expected_data_tag=1, expected_data_data=16),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=2, header_valid=1, header_opcode=4, payload_valid=1, payload_data=64, patch_valid=1, patch_tag=2, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=2, expected_data_tag=2, expected_data_data=32),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=4, header_valid=1, header_opcode=8, payload_valid=1, payload_data=128, patch_valid=1, patch_tag=4, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=4, expected_data_tag=4, expected_data_data=64),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(base_valid=1, base_opcode=8, header_valid=1, header_tag=1, payload_valid=1, payload_data=256, patch_valid=1, patch_tag=8, take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1, expected_valid=1, expected_data_opcode=8, expected_data_tag=8, expected_data_data=128),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
            Scenario(take=1, expected_base_ready=1, expected_header_ready=1, expected_payload_ready=1, expected_patch_ready=1),
        ),
    )
    # fmt: on
    position = phase if phase < 158 else 157
    scenario = scenarios.rows[position]
    base = Packet(
        opcode=scenario.base_opcode,
        tag=scenario.base_tag,
        data=scenario.base_data,
        valid=scenario.base_present,
    )
    header = Header(opcode=scenario.header_opcode, tag=scenario.header_tag)
    payload = Payload(data=scenario.payload_data)
    patch = Patch(tag=scenario.patch_tag, valid=scenario.patch_present)
    base_ready, header_ready, payload_ready, patch_ready, output = RecordSpreadPipeline(
        scenario.base_valid,
        base,
        scenario.header_valid,
        header,
        scenario.payload_valid,
        payload,
        scenario.patch_valid,
        patch,
        scenario.take,
    )

    @rule
    def check():
        if phase < 158:
            assert base_ready == scenario.expected_base_ready, (
                "record_spread_pipeline: base_ready"
            )
            assert header_ready == scenario.expected_header_ready, (
                "record_spread_pipeline: header_ready"
            )
            assert payload_ready == scenario.expected_payload_ready, (
                "record_spread_pipeline: payload_ready"
            )
            assert patch_ready == scenario.expected_patch_ready, (
                "record_spread_pipeline: patch_ready"
            )
            assert output.valid == scenario.expected_valid, (
                "record_spread_pipeline: valid"
            )
            assert output.data.opcode == scenario.expected_data_opcode, (
                "record_spread_pipeline: data.opcode"
            )
            assert output.data.tag == scenario.expected_data_tag, (
                "record_spread_pipeline: data.tag"
            )
            assert output.data.data == scenario.expected_data_data, (
                "record_spread_pipeline: data.data"
            )
            assert output.data.valid == scenario.expected_data_valid, (
                "record_spread_pipeline: data.valid"
            )
        log("info", "record_spread_pipeline.base_ready", base_ready)
        log("info", "record_spread_pipeline.header_ready", header_ready)
        log("info", "record_spread_pipeline.payload_ready", payload_ready)
        log("info", "record_spread_pipeline.patch_ready", patch_ready)
        log("info", "record_spread_pipeline.valid", output.valid)
        log("info", "record_spread_pipeline.data.opcode", output.data.opcode)
        log("info", "record_spread_pipeline.data.tag", output.data.tag)
        log("info", "record_spread_pipeline.data.data", output.data.data)
        log("info", "record_spread_pipeline.data.valid", output.data.valid)

    check()
    advance(phase)

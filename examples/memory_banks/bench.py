"""Two writes, four-bank reads and a stalled fixed-priority merge."""

# ruff: noqa: F823 -- the System Rule advances Module-owned phase state.
import pycircuit as ac
from example_memory_banks.memory_banks import BankRequest, MemoryBanks


@ac.struct
class Scenario:
    valid: ac.u1
    bank: ac.u2
    write: ac.u1
    data: ac.u16
    tag: ac.u8
    take: ac.u1 = 1
    output_valid: ac.u1
    response_bank: ac.u2
    response_write: ac.u1
    response_data: ac.u16
    response_tag: ac.u8


def scenario_rows() -> ac.table[26, Scenario]:
    # fmt: off
    rows: ac.table[26, Scenario] = (
        Scenario(),
        Scenario(valid=1, write=1, data=41, tag=1),
        Scenario(valid=1, bank=1, write=1, data=91, tag=2),
        Scenario(valid=1, tag=3),
        Scenario(valid=1, bank=1, tag=4),
        Scenario(valid=1, bank=2, tag=5),
        Scenario(),
        Scenario(output_valid=1, response_write=1, response_tag=1),
        Scenario(output_valid=1, response_bank=1, response_write=1, response_tag=2),
        Scenario(),
        Scenario(output_valid=1, response_data=41, response_tag=3),
        Scenario(output_valid=1, response_bank=1, response_data=91, response_tag=4),
        Scenario(output_valid=1, response_bank=2, response_tag=5),
        Scenario(valid=1, bank=3, write=1, data=0x3333, tag=6),
        Scenario(valid=1, bank=3, tag=7),
        Scenario(valid=1, tag=8),
        Scenario(valid=1, bank=1, tag=9),
        Scenario(valid=1, bank=2, tag=10),
        Scenario(),
        Scenario(
            take=0, output_valid=1, response_bank=3, response_write=1, response_tag=6
        ),
        Scenario(
            take=0, output_valid=1, response_bank=3, response_write=1, response_tag=6
        ),
        Scenario(output_valid=1, response_bank=3, response_write=1, response_tag=6),
        Scenario(output_valid=1, response_data=41, response_tag=8),
        Scenario(output_valid=1, response_bank=1, response_data=91, response_tag=9),
        Scenario(output_valid=1, response_bank=2, response_tag=10),
        Scenario(output_valid=1, response_bank=3, response_data=0x3333, response_tag=7),
    )
    # fmt: on
    return rows


@ac.system
def ExerciseMemoryBanks():  # noqa: N802
    phase: ac.u5 = 0
    scenarios = scenario_rows()
    position = phase if phase < 26 else 25
    scenario = scenarios[position]
    request = BankRequest(
        bank=scenario.bank,
        offset=3,
        write=scenario.write,
        data=scenario.data,
        tag=scenario.tag,
    )
    dut = MemoryBanks(scenario.valid, request, scenario.take)

    @ac.rule
    def check() -> None:
        assert dut.input_ready == 1, "memory banks: input capacity"
        assert dut.output_valid == scenario.output_valid, "memory banks: output valid"
        assert (scenario.output_valid == 0) | (
            dut.response.bank == scenario.response_bank
        ), "memory banks: response bank"
        assert (scenario.output_valid == 0) | (
            dut.response.offset == 3
        ), "memory banks: response offset"
        assert (scenario.output_valid == 0) | (
            dut.response.write == scenario.response_write
        ), "memory banks: response kind"
        assert (scenario.output_valid == 0) | (
            dut.response.data == scenario.response_data
        ), "memory banks: response data"
        assert (scenario.output_valid == 0) | (
            dut.response.tag == scenario.response_tag
        ), "memory banks: response tag"
        ac.log("info", "memory bank scenario", phase, dut.output_valid)
        phase = phase + 1

    check()

"""Two readable seed/copy/readback transactions with response stalls."""

# ruff: noqa: F823 -- the System Rule advances Module-owned phase state.
import pycircuit as ac
from example_dma.dma import Dma, DmaRequest


@ac.struct
class Scenario:
    seed_valid: ac.u1
    seed_dram: ac.u4
    seed_sram: ac.u4
    seed_data: ac.u16
    seed_tag: ac.u8
    copy_valid: ac.u1
    copy_dram: ac.u4
    copy_sram: ac.u4
    copy_tag: ac.u8
    check_valid: ac.u1
    check_dram: ac.u4
    check_sram: ac.u4
    check_tag: ac.u8
    take_copy: ac.u1 = 1
    take_check: ac.u1 = 1
    dram_accepted: ac.u2
    dram_enqueued: ac.u2
    sram_accepted: ac.u2
    sram_enqueued: ac.u2
    seed_response_valid: ac.u1
    seed_response_data: ac.u16
    seed_response_tag: ac.u8
    copy_response_valid: ac.u1
    copy_response_data: ac.u16
    copy_response_tag: ac.u8
    check_response_valid: ac.u1
    check_response_data: ac.u16
    check_response_tag: ac.u8


def scenario_rows() -> ac.table[42, Scenario]:
    # fmt: off
    rows: ac.table[42, Scenario] = (
        Scenario(),
        Scenario(seed_valid=1, seed_dram=5, seed_sram=3, seed_data=0x1234, seed_tag=1),
        Scenario(dram_accepted=2),
        Scenario(),
        Scenario(),
        Scenario(dram_enqueued=2),
        Scenario(seed_response_valid=1, seed_response_tag=1),
        Scenario(copy_valid=1, copy_dram=5, copy_sram=3, copy_tag=2),
        Scenario(dram_accepted=1),
        Scenario(),
        Scenario(),
        Scenario(dram_enqueued=1),
        Scenario(sram_accepted=2),
        Scenario(),
        Scenario(sram_enqueued=2),
        Scenario(take_copy=0, copy_response_valid=1, copy_response_tag=2),
        Scenario(take_copy=0, copy_response_valid=1, copy_response_tag=2),
        Scenario(take_copy=0, copy_response_valid=1, copy_response_tag=2),
        Scenario(copy_response_valid=1, copy_response_tag=2),
        Scenario(check_valid=1, check_sram=3, check_tag=3),
        Scenario(sram_accepted=1),
        Scenario(seed_valid=1, seed_dram=9, seed_sram=1, seed_data=0xBEEF, seed_tag=4),
        Scenario(dram_accepted=2, sram_enqueued=1),
        Scenario(
            take_check=0,
            check_response_valid=1,
            check_response_data=0x1234,
            check_response_tag=3,
        ),
        Scenario(
            take_check=0,
            check_response_valid=1,
            check_response_data=0x1234,
            check_response_tag=3,
        ),
        Scenario(
            dram_enqueued=2,
            check_response_valid=1,
            check_response_data=0x1234,
            check_response_tag=3,
        ),
        Scenario(seed_response_valid=1, seed_response_tag=4),
        Scenario(copy_valid=1, copy_dram=9, copy_sram=3, copy_tag=5),
        Scenario(dram_accepted=1),
        Scenario(),
        Scenario(),
        Scenario(dram_enqueued=1),
        Scenario(sram_accepted=2),
        Scenario(),
        Scenario(sram_enqueued=2),
        Scenario(copy_response_valid=1, copy_response_data=0x1234, copy_response_tag=5),
        Scenario(check_valid=1, check_dram=12, check_sram=3, check_tag=6),
        Scenario(sram_accepted=1),
        Scenario(),
        Scenario(sram_enqueued=1),
        Scenario(
            check_response_valid=1, check_response_data=0xBEEF, check_response_tag=6
        ),
        Scenario(),
    )
    # fmt: on
    return rows


@ac.system
def ExerciseDma():  # noqa: N802
    phase: ac.u6 = 0
    scenarios = scenario_rows()
    position = phase if phase < 42 else 41
    scenario = scenarios[position]
    seed = DmaRequest(
        dram_address=scenario.seed_dram,
        sram_address=scenario.seed_sram,
        data=scenario.seed_data,
        tag=scenario.seed_tag,
    )
    copy = DmaRequest(
        dram_address=scenario.copy_dram,
        sram_address=scenario.copy_sram,
        data=0,
        tag=scenario.copy_tag,
    )
    readback = DmaRequest(
        dram_address=scenario.check_dram,
        sram_address=scenario.check_sram,
        data=0,
        tag=scenario.check_tag,
    )
    dut = Dma(
        scenario.seed_valid,
        seed,
        scenario.copy_valid,
        copy,
        scenario.check_valid,
        readback,
        1,
        scenario.take_copy,
        scenario.take_check,
    )

    @ac.rule
    def check() -> None:
        assert dut.seed_ready == 1, "dma: seed request capacity"
        assert dut.copy_ready == 1, "dma: copy request capacity"
        assert dut.check_ready == 1, "dma: readback request capacity"
        assert dut.dram_accepted == scenario.dram_accepted, "dma: DRAM accepted"
        assert dut.dram_enqueued == scenario.dram_enqueued, "dma: DRAM enqueued"
        assert dut.sram_accepted == scenario.sram_accepted, "dma: SRAM accepted"
        assert dut.sram_enqueued == scenario.sram_enqueued, "dma: SRAM enqueued"
        assert (
            dut.seed_valid == scenario.seed_response_valid
        ), "dma: seed response valid"
        assert (
            dut.copy_valid == scenario.copy_response_valid
        ), "dma: copy response valid"
        assert (
            dut.check_valid == scenario.check_response_valid
        ), "dma: check response valid"
        if scenario.seed_response_valid:
            assert (
                dut.seed_response.data == scenario.seed_response_data
            ), "dma: seed response data"
            assert (
                dut.seed_response.tag == scenario.seed_response_tag
            ), "dma: seed response tag"
        if scenario.copy_response_valid:
            assert (
                dut.copy_response.data == scenario.copy_response_data
            ), "dma: copy response data"
            assert (
                dut.copy_response.tag == scenario.copy_response_tag
            ), "dma: copy response tag"
        if scenario.check_response_valid:
            assert (
                dut.check_response.data == scenario.check_response_data
            ), "dma: check response data"
            assert (
                dut.check_response.tag == scenario.check_response_tag
            ), "dma: check response tag"
        ac.log(
            "info",
            "dma scenario",
            phase,
            dut.seed_valid,
            dut.copy_valid,
            dut.check_valid,
        )
        phase = phase + 1

    check()

"""Regular-clock full known-input stream from the retained dma oracle."""

from example_dma.dma import Dma, DmaRequest
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    end: bits[64]
    seed_valid: bits[1]
    seed_dram: bits[4]
    seed_sram: bits[4]
    seed_data: bits[16]
    seed_tag: bits[8]
    copy_valid: bits[1]
    copy_dram: bits[4]
    copy_sram: bits[4]
    copy_data: bits[16]
    copy_tag: bits[8]
    check_valid: bits[1]
    check_dram: bits[4]
    check_sram: bits[4]
    check_data: bits[16]
    check_tag: bits[8]
    take_seed: bits[1]
    take_copy: bits[1]
    take_check: bits[1]
    expected_seed_ready: bits[1]
    expected_copy_ready: bits[1]
    expected_check_ready: bits[1]
    expected_seed_valid: bits[1]
    expected_copy_valid: bits[1]
    expected_check_valid: bits[1]
    expected_dram_accepted: bits[2]
    expected_dram_enqueued: bits[2]
    expected_sram_accepted: bits[2]
    expected_sram_enqueued: bits[2]
    expected_seed_response_dram_address: bits[4]
    expected_seed_response_sram_address: bits[4]
    expected_seed_response_data: bits[16]
    expected_seed_response_tag: bits[8]
    expected_copy_response_dram_address: bits[4]
    expected_copy_response_sram_address: bits[4]
    expected_copy_response_data: bits[16]
    expected_copy_response_tag: bits[8]
    expected_check_response_dram_address: bits[4]
    expected_check_response_sram_address: bits[4]
    expected_check_response_data: bits[16]
    expected_check_response_tag: bits[8]


@struct
class Scenarios:
    rows: table[294, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseDma():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(end=0, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=1, seed_valid=1, seed_dram=5, seed_sram=3, seed_data=4660, seed_tag=1, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=2, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=2),
            Scenario(end=4, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=5, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=6, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_seed_response_dram_address=5, expected_seed_response_sram_address=3, expected_seed_response_tag=1),
            Scenario(end=7, copy_valid=1, copy_dram=5, copy_sram=3, copy_tag=2, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=8, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=1),
            Scenario(end=10, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=11, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=1),
            Scenario(end=12, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_accepted=2),
            Scenario(end=13, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=14, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=15, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_copy_response_dram_address=5, expected_copy_response_sram_address=3, expected_copy_response_tag=2),
            Scenario(end=16, check_valid=1, check_sram=3, check_tag=3, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=17, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_accepted=1),
            Scenario(end=18, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=19, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=1),
            Scenario(end=20, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_check_valid=1, expected_check_response_sram_address=3, expected_check_response_data=4660, expected_check_response_tag=3),
            Scenario(end=21, seed_valid=1, seed_dram=9, seed_sram=1, seed_data=48879, seed_tag=4, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=22, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=2),
            Scenario(end=24, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=25, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=26, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_seed_response_dram_address=9, expected_seed_response_sram_address=1, expected_seed_response_tag=4),
            Scenario(end=27, copy_valid=1, copy_dram=9, copy_sram=3, copy_data=30583, copy_tag=5, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=28, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=1),
            Scenario(end=30, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=31, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=1),
            Scenario(end=32, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_accepted=2),
            Scenario(end=33, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=34, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=35, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_copy_response_dram_address=9, expected_copy_response_sram_address=3, expected_copy_response_data=4660, expected_copy_response_tag=5),
            Scenario(end=36, check_valid=1, check_dram=12, check_sram=3, check_data=21845, check_tag=6, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=37, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_accepted=1),
            Scenario(end=38, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=39, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=1),
            Scenario(end=40, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_check_valid=1, expected_check_response_dram_address=12, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=6),
            Scenario(end=41, seed_valid=1, seed_sram=5, seed_data=16384, seed_tag=10, copy_valid=1, copy_sram=3, copy_data=40960, copy_tag=24, check_valid=1, check_dram=7, check_sram=3, check_data=36864, check_tag=46, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=42, seed_valid=1, seed_dram=1, seed_sram=6, seed_data=16385, seed_tag=11, copy_valid=1, copy_dram=3, copy_sram=8, copy_data=40961, copy_tag=25, check_valid=1, check_dram=8, check_sram=8, check_data=36865, check_tag=47, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=1),
            Scenario(end=43, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=6, copy_sram=13, copy_data=40962, copy_tag=26, check_valid=1, check_dram=9, check_sram=13, check_data=36866, check_tag=48, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=44, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=9, copy_sram=2, copy_data=40963, copy_tag=27, check_valid=1, check_dram=10, check_sram=2, check_data=36867, check_tag=49, expected_copy_ready=1, expected_sram_enqueued=1),
            Scenario(end=45, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=12, copy_sram=7, copy_data=40964, copy_tag=28, check_valid=1, check_dram=10, check_sram=2, check_data=36867, check_tag=49, expected_copy_ready=1, expected_check_valid=1, expected_dram_enqueued=1, expected_sram_accepted=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=46, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=15, copy_sram=12, copy_data=40965, copy_tag=29, check_valid=1, check_dram=10, check_sram=2, check_data=36867, check_tag=49, expected_check_ready=1, expected_check_valid=1, expected_dram_accepted=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=47, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=15, copy_sram=12, copy_data=40965, copy_tag=29, check_valid=1, check_dram=11, check_sram=7, check_data=36868, check_tag=50, expected_copy_ready=1, expected_check_valid=1, expected_sram_enqueued=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=48, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=2, copy_sram=1, copy_data=40966, copy_tag=30, check_valid=1, check_dram=11, check_sram=7, check_data=36868, check_tag=50, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=49, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=2, copy_sram=1, copy_data=40966, copy_tag=30, check_valid=1, check_dram=11, check_sram=7, check_data=36868, check_tag=50, expected_check_ready=1, expected_check_valid=1, expected_dram_enqueued=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=50, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=2, copy_sram=1, copy_data=40966, copy_tag=30, check_valid=1, check_dram=12, check_sram=12, check_data=36869, check_tag=51, expected_check_valid=1, expected_dram_accepted=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=51, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=2, copy_sram=1, copy_data=40966, copy_tag=30, check_valid=1, check_dram=12, check_sram=12, check_data=36869, check_tag=51, expected_copy_ready=1, expected_check_valid=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=52, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_dram=2, copy_sram=1, copy_data=40966, copy_tag=30, check_dram=12, check_sram=12, check_data=36869, check_tag=51, expected_check_valid=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=132, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=12, check_sram=12, check_data=36869, check_tag=51, expected_check_valid=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=133, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_dram=12, check_sram=12, check_data=36869, check_tag=51, expected_check_valid=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=152, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=12, check_sram=12, check_data=36869, check_tag=51, expected_check_valid=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=153, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=12, check_sram=12, check_data=36869, check_tag=51, take_check=1, expected_check_valid=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=46),
            Scenario(end=154, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=12, check_sram=12, check_data=36869, check_tag=51, take_check=1, expected_check_valid=1, expected_sram_enqueued=1, expected_check_response_dram_address=8, expected_check_response_sram_address=8, expected_check_response_tag=47),
            Scenario(end=155, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=12, check_sram=12, check_data=36869, check_tag=51, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=9, expected_check_response_sram_address=13, expected_check_response_tag=48),
            Scenario(end=156, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=12, check_sram=12, check_data=36869, check_tag=51, take_check=1, expected_check_ready=1),
            Scenario(end=157, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=13, check_sram=1, check_data=36870, check_tag=52, take_check=1, expected_sram_enqueued=1),
            Scenario(end=158, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=13, check_sram=1, check_data=36870, check_tag=52, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=10, expected_check_response_sram_address=2, expected_check_response_tag=49),
            Scenario(end=159, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=13, check_sram=1, check_data=36870, check_tag=52, take_check=1, expected_check_ready=1),
            Scenario(end=160, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=14, check_sram=6, check_data=36871, check_tag=53, take_check=1, expected_sram_enqueued=1),
            Scenario(end=161, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=14, check_sram=6, check_data=36871, check_tag=53, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=11, expected_check_response_sram_address=7, expected_check_response_tag=50),
            Scenario(end=162, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=14, check_sram=6, check_data=36871, check_tag=53, take_check=1, expected_check_ready=1),
            Scenario(end=163, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=15, check_sram=11, check_data=36872, check_tag=54, take_check=1, expected_sram_enqueued=1),
            Scenario(end=164, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=15, check_sram=11, check_data=36872, check_tag=54, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=12, expected_check_response_sram_address=12, expected_check_response_tag=51),
            Scenario(end=165, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=15, check_sram=11, check_data=36872, check_tag=54, take_check=1, expected_check_ready=1),
            Scenario(end=166, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_data=36873, check_tag=55, take_check=1, expected_sram_enqueued=1),
            Scenario(end=167, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_data=36873, check_tag=55, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=13, expected_check_response_sram_address=1, expected_check_response_tag=52),
            Scenario(end=168, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_data=36873, check_tag=55, take_check=1, expected_check_ready=1),
            Scenario(end=169, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=1, check_sram=5, check_data=36874, check_tag=56, take_check=1, expected_sram_enqueued=1),
            Scenario(end=170, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=1, check_sram=5, check_data=36874, check_tag=56, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=14, expected_check_response_sram_address=6, expected_check_response_tag=53),
            Scenario(end=171, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=1, check_sram=5, check_data=36874, check_tag=56, take_check=1, expected_check_ready=1),
            Scenario(end=172, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=2, check_sram=10, check_data=36875, check_tag=57, take_check=1, expected_sram_enqueued=1),
            Scenario(end=173, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=2, check_sram=10, check_data=36875, check_tag=57, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=15, expected_check_response_sram_address=11, expected_check_response_tag=54),
            Scenario(end=174, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=2, check_sram=10, check_data=36875, check_tag=57, take_check=1, expected_check_ready=1),
            Scenario(end=175, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=3, check_sram=15, check_data=36876, check_tag=58, take_check=1, expected_sram_enqueued=1),
            Scenario(end=176, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=3, check_sram=15, check_data=36876, check_tag=58, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_tag=55),
            Scenario(end=177, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=3, check_sram=15, check_data=36876, check_tag=58, take_check=1, expected_check_ready=1),
            Scenario(end=178, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=4, check_sram=4, check_data=36877, check_tag=59, take_check=1, expected_sram_enqueued=1),
            Scenario(end=179, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=4, check_sram=4, check_data=36877, check_tag=59, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=1, expected_check_response_sram_address=5, expected_check_response_tag=56),
            Scenario(end=180, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=4, check_sram=4, check_data=36877, check_tag=59, take_check=1, expected_check_ready=1),
            Scenario(end=181, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=5, check_sram=9, check_data=36878, check_tag=60, take_check=1, expected_sram_enqueued=1),
            Scenario(end=182, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=5, check_sram=9, check_data=36878, check_tag=60, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=2, expected_check_response_sram_address=10, expected_check_response_tag=57),
            Scenario(end=183, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=5, check_sram=9, check_data=36878, check_tag=60, take_check=1, expected_check_ready=1),
            Scenario(end=184, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=6, check_sram=14, check_data=36879, check_tag=61, take_check=1, expected_sram_enqueued=1),
            Scenario(end=185, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=6, check_sram=14, check_data=36879, check_tag=61, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=3, expected_check_response_sram_address=15, expected_check_response_tag=58),
            Scenario(end=186, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=6, check_sram=14, check_data=36879, check_tag=61, take_check=1, expected_check_ready=1),
            Scenario(end=187, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=7, check_sram=3, check_data=36880, check_tag=62, take_check=1, expected_sram_enqueued=1),
            Scenario(end=188, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=7, check_sram=3, check_data=36880, check_tag=62, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=4, expected_check_response_sram_address=4, expected_check_response_tag=59),
            Scenario(end=189, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=7, check_sram=3, check_data=36880, check_tag=62, take_check=1, expected_check_ready=1),
            Scenario(end=190, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=8, check_sram=8, check_data=36881, check_tag=63, take_check=1, expected_sram_enqueued=1),
            Scenario(end=191, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=8, check_sram=8, check_data=36881, check_tag=63, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=5, expected_check_response_sram_address=9, expected_check_response_tag=60),
            Scenario(end=192, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=8, check_sram=8, check_data=36881, check_tag=63, take_check=1, expected_check_ready=1),
            Scenario(end=193, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=9, check_sram=13, check_data=36882, check_tag=64, take_check=1, expected_sram_enqueued=1),
            Scenario(end=194, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=9, check_sram=13, check_data=36882, check_tag=64, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=6, expected_check_response_sram_address=14, expected_check_response_tag=61),
            Scenario(end=195, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=9, check_sram=13, check_data=36882, check_tag=64, take_check=1, expected_check_ready=1),
            Scenario(end=196, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=10, check_sram=2, check_data=36883, check_tag=65, take_check=1, expected_sram_enqueued=1),
            Scenario(end=197, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=10, check_sram=2, check_data=36883, check_tag=65, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=7, expected_check_response_sram_address=3, expected_check_response_data=48879, expected_check_response_tag=62),
            Scenario(end=198, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=10, check_sram=2, check_data=36883, check_tag=65, take_check=1, expected_check_ready=1),
            Scenario(end=199, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=11, check_sram=7, check_data=36884, check_tag=66, take_check=1, expected_sram_enqueued=1),
            Scenario(end=200, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=11, check_sram=7, check_data=36884, check_tag=66, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=8, expected_check_response_sram_address=8, expected_check_response_tag=63),
            Scenario(end=201, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=11, check_sram=7, check_data=36884, check_tag=66, take_check=1, expected_check_ready=1),
            Scenario(end=202, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=12, check_sram=12, check_data=36885, check_tag=67, take_check=1, expected_sram_enqueued=1),
            Scenario(end=203, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=12, check_sram=12, check_data=36885, check_tag=67, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=9, expected_check_response_sram_address=13, expected_check_response_tag=64),
            Scenario(end=204, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=12, check_sram=12, check_data=36885, check_tag=67, take_check=1, expected_check_ready=1),
            Scenario(end=205, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=13, check_sram=1, check_data=36886, check_tag=68, take_check=1, expected_sram_enqueued=1),
            Scenario(end=206, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=13, check_sram=1, check_data=36886, check_tag=68, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=10, expected_check_response_sram_address=2, expected_check_response_tag=65),
            Scenario(end=207, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=13, check_sram=1, check_data=36886, check_tag=68, take_check=1, expected_check_ready=1),
            Scenario(end=208, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=14, check_sram=6, check_data=36887, check_tag=69, take_check=1, expected_sram_enqueued=1),
            Scenario(end=209, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=14, check_sram=6, check_data=36887, check_tag=69, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=11, expected_check_response_sram_address=7, expected_check_response_tag=66),
            Scenario(end=210, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, check_valid=1, check_dram=14, check_sram=6, check_data=36887, check_tag=69, take_check=1, expected_check_ready=1),
            Scenario(end=211, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_sram_enqueued=1),
            Scenario(end=212, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=12, expected_check_response_sram_address=12, expected_check_response_tag=67),
            Scenario(end=213, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_check_ready=1),
            Scenario(end=214, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_check_ready=1, expected_sram_enqueued=1),
            Scenario(end=215, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_check_ready=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_dram_address=13, expected_check_response_sram_address=1, expected_check_response_tag=68),
            Scenario(end=216, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_check_ready=1),
            Scenario(end=217, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_check_ready=1, expected_sram_enqueued=1),
            Scenario(end=218, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_check_ready=1, expected_check_valid=1, expected_sram_accepted=2, expected_check_response_dram_address=14, expected_check_response_sram_address=6, expected_check_response_tag=69),
            Scenario(end=219, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_check_ready=1, expected_dram_enqueued=1),
            Scenario(end=220, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_enqueued=2),
            Scenario(end=221, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=5, copy_sram=6, copy_data=40967, copy_tag=31, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_sram_accepted=2, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=222, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=8, copy_sram=11, copy_data=40968, copy_tag=32, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=223, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=8, copy_sram=11, copy_data=40968, copy_tag=32, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_sram_enqueued=2, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=224, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=8, copy_sram=11, copy_data=40968, copy_tag=32, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_accepted=1, expected_sram_accepted=2, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=225, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=8, copy_sram=11, copy_data=40968, copy_tag=32, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=226, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=11, copy_data=40969, copy_tag=33, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=227, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=11, copy_data=40969, copy_tag=33, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=228, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=11, copy_data=40969, copy_tag=33, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_accepted=1, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=229, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=11, copy_data=40969, copy_tag=33, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=412, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=14, copy_sram=5, copy_data=40970, copy_tag=34, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=413, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=14, copy_sram=5, copy_data=40970, copy_tag=34, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_copy_response_sram_address=3, expected_copy_response_data=48879, expected_copy_response_tag=24),
            Scenario(end=414, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=14, copy_sram=5, copy_data=40970, copy_tag=34, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_sram_enqueued=2, expected_copy_response_dram_address=3, expected_copy_response_sram_address=8, expected_copy_response_tag=25),
            Scenario(end=415, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=14, copy_sram=5, copy_data=40970, copy_tag=34, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_sram_accepted=2, expected_copy_response_dram_address=6, expected_copy_response_sram_address=13, expected_copy_response_tag=26),
            Scenario(end=416, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=14, copy_sram=5, copy_data=40970, copy_tag=34, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_enqueued=1),
            Scenario(end=417, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=14, copy_sram=5, copy_data=40970, copy_tag=34, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_enqueued=2),
            Scenario(end=418, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=14, copy_sram=5, copy_data=40970, copy_tag=34, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_sram_accepted=2, expected_copy_response_dram_address=9, expected_copy_response_sram_address=2, expected_copy_response_tag=27),
            Scenario(end=419, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=1, copy_sram=10, copy_data=40971, copy_tag=35, take_copy=1, take_check=1, expected_check_ready=1),
            Scenario(end=420, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=1, copy_sram=10, copy_data=40971, copy_tag=35, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_enqueued=1, expected_sram_enqueued=2),
            Scenario(end=421, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=1, copy_sram=10, copy_data=40971, copy_tag=35, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_accepted=1, expected_sram_accepted=2, expected_copy_response_dram_address=12, expected_copy_response_sram_address=7, expected_copy_response_tag=28),
            Scenario(end=422, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=1, copy_sram=10, copy_data=40971, copy_tag=35, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=423, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=4, copy_sram=15, copy_data=40972, copy_tag=36, take_copy=1, take_check=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=424, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=4, copy_sram=15, copy_data=40972, copy_tag=36, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_sram_accepted=2, expected_copy_response_dram_address=15, expected_copy_response_sram_address=12, expected_copy_response_tag=29),
            Scenario(end=425, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=4, copy_sram=15, copy_data=40972, copy_tag=36, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1),
            Scenario(end=426, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=4, copy_sram=15, copy_data=40972, copy_tag=36, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=427, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=7, copy_sram=4, copy_data=40973, copy_tag=37, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_sram_accepted=2, expected_copy_response_dram_address=2, expected_copy_response_sram_address=1, expected_copy_response_tag=30),
            Scenario(end=428, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=7, copy_sram=4, copy_data=40973, copy_tag=37, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_enqueued=1),
            Scenario(end=429, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=7, copy_sram=4, copy_data=40973, copy_tag=37, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_enqueued=2),
            Scenario(end=430, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=7, copy_sram=4, copy_data=40973, copy_tag=37, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_sram_accepted=2, expected_copy_response_dram_address=5, expected_copy_response_sram_address=6, expected_copy_response_tag=31),
            Scenario(end=431, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=10, copy_sram=9, copy_data=40974, copy_tag=38, take_copy=1, take_check=1, expected_check_ready=1),
            Scenario(end=432, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=10, copy_sram=9, copy_data=40974, copy_tag=38, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_enqueued=1, expected_sram_enqueued=2),
            Scenario(end=433, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=10, copy_sram=9, copy_data=40974, copy_tag=38, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_accepted=1, expected_sram_accepted=2, expected_copy_response_dram_address=8, expected_copy_response_sram_address=11, expected_copy_response_tag=32),
            Scenario(end=434, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=10, copy_sram=9, copy_data=40974, copy_tag=38, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=435, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=13, copy_sram=14, copy_data=40975, copy_tag=39, take_copy=1, take_check=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=436, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=13, copy_sram=14, copy_data=40975, copy_tag=39, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=11, expected_copy_response_tag=33),
            Scenario(end=437, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=13, copy_sram=14, copy_data=40975, copy_tag=39, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=438, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=13, copy_sram=14, copy_data=40975, copy_tag=39, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=439, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_sram=3, copy_data=40976, copy_tag=40, take_copy=1, take_check=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=440, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_sram=3, copy_data=40976, copy_tag=40, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=14, expected_copy_response_sram_address=5, expected_copy_response_tag=34),
            Scenario(end=441, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_sram=3, copy_data=40976, copy_tag=40, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=442, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_sram=3, copy_data=40976, copy_tag=40, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=443, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=3, copy_sram=8, copy_data=40977, copy_tag=41, take_copy=1, take_check=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=444, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=3, copy_sram=8, copy_data=40977, copy_tag=41, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=1, expected_copy_response_sram_address=10, expected_copy_response_tag=35),
            Scenario(end=445, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=3, copy_sram=8, copy_data=40977, copy_tag=41, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=446, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=3, copy_sram=8, copy_data=40977, copy_tag=41, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=447, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=6, copy_sram=13, copy_data=40978, copy_tag=42, take_copy=1, take_check=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=448, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=6, copy_sram=13, copy_data=40978, copy_tag=42, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=4, expected_copy_response_sram_address=15, expected_copy_response_tag=36),
            Scenario(end=449, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=6, copy_sram=13, copy_data=40978, copy_tag=42, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=450, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=6, copy_sram=13, copy_data=40978, copy_tag=42, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=451, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=9, copy_sram=2, copy_data=40979, copy_tag=43, take_copy=1, take_check=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=452, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=9, copy_sram=2, copy_data=40979, copy_tag=43, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=7, expected_copy_response_sram_address=4, expected_copy_response_tag=37),
            Scenario(end=453, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=9, copy_sram=2, copy_data=40979, copy_tag=43, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=454, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=9, copy_sram=2, copy_data=40979, copy_tag=43, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=455, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=12, copy_sram=7, copy_data=40980, copy_tag=44, take_copy=1, take_check=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=456, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=12, copy_sram=7, copy_data=40980, copy_tag=44, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=10, expected_copy_response_sram_address=9, expected_copy_response_tag=38),
            Scenario(end=457, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=12, copy_sram=7, copy_data=40980, copy_tag=44, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=458, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=12, copy_sram=7, copy_data=40980, copy_tag=44, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=459, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=15, copy_sram=12, copy_data=40981, copy_tag=45, take_copy=1, take_check=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=460, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=15, copy_sram=12, copy_data=40981, copy_tag=45, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=13, expected_copy_response_sram_address=14, expected_copy_response_tag=39),
            Scenario(end=461, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=15, copy_sram=12, copy_data=40981, copy_tag=45, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=462, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, copy_valid=1, copy_dram=15, copy_sram=12, copy_data=40981, copy_tag=45, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=463, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=464, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_sram_address=3, expected_copy_response_tag=40),
            Scenario(end=465, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=466, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=467, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=468, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=3, expected_copy_response_sram_address=8, expected_copy_response_tag=41),
            Scenario(end=469, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=470, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=471, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=472, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=6, expected_copy_response_sram_address=13, expected_copy_response_tag=42),
            Scenario(end=473, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=474, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=475, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=476, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=9, expected_copy_response_sram_address=2, expected_copy_response_data=48879, expected_copy_response_tag=43),
            Scenario(end=477, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=2),
            Scenario(end=478, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=479, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=480, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=1, expected_copy_response_dram_address=12, expected_copy_response_sram_address=7, expected_copy_response_tag=44),
            Scenario(end=481, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=2, expected_sram_accepted=2),
            Scenario(end=482, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=16386, seed_tag=12, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=483, seed_valid=1, seed_dram=3, seed_sram=8, seed_data=16387, seed_tag=13, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=484, seed_valid=1, seed_dram=3, seed_sram=8, seed_data=16387, seed_tag=13, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_enqueued=2, expected_copy_response_dram_address=15, expected_copy_response_sram_address=12, expected_copy_response_tag=45),
            Scenario(end=485, seed_valid=1, seed_dram=3, seed_sram=8, seed_data=16387, seed_tag=13, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_sram_address=5, expected_seed_response_tag=10),
            Scenario(end=486, seed_valid=1, seed_dram=3, seed_sram=8, seed_data=16387, seed_tag=13, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_seed_response_sram_address=5, expected_seed_response_tag=10),
            Scenario(end=487, seed_valid=1, seed_dram=4, seed_sram=9, seed_data=16388, seed_tag=14, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_seed_response_sram_address=5, expected_seed_response_tag=10),
            Scenario(end=488, seed_valid=1, seed_dram=4, seed_sram=9, seed_data=16388, seed_tag=14, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_enqueued=2, expected_seed_response_sram_address=5, expected_seed_response_tag=10),
            Scenario(end=489, seed_valid=1, seed_dram=4, seed_sram=9, seed_data=16388, seed_tag=14, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_sram_address=5, expected_seed_response_tag=10),
            Scenario(end=490, seed_valid=1, seed_dram=4, seed_sram=9, seed_data=16388, seed_tag=14, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_seed_response_sram_address=5, expected_seed_response_tag=10),
            Scenario(end=492, seed_valid=1, seed_dram=5, seed_sram=10, seed_data=16389, seed_tag=15, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_seed_response_sram_address=5, expected_seed_response_tag=10),
            Scenario(end=493, seed_valid=1, seed_dram=5, seed_sram=10, seed_data=16389, seed_tag=15, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_seed_response_sram_address=5, expected_seed_response_tag=10),
            Scenario(end=494, seed_valid=1, seed_dram=5, seed_sram=10, seed_data=16389, seed_tag=15, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_enqueued=2, expected_seed_response_dram_address=1, expected_seed_response_sram_address=6, expected_seed_response_tag=11),
            Scenario(end=495, seed_valid=1, seed_dram=5, seed_sram=10, seed_data=16389, seed_tag=15, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=2, expected_seed_response_sram_address=7, expected_seed_response_tag=12),
            Scenario(end=496, seed_valid=1, seed_dram=5, seed_sram=10, seed_data=16389, seed_tag=15, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=497, seed_valid=1, seed_dram=6, seed_sram=11, seed_data=16390, seed_tag=16, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=498, seed_valid=1, seed_dram=6, seed_sram=11, seed_data=16390, seed_tag=16, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=499, seed_valid=1, seed_dram=6, seed_sram=11, seed_data=16390, seed_tag=16, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=3, expected_seed_response_sram_address=8, expected_seed_response_tag=13),
            Scenario(end=500, seed_valid=1, seed_dram=6, seed_sram=11, seed_data=16390, seed_tag=16, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=501, seed_valid=1, seed_dram=7, seed_sram=12, seed_data=16391, seed_tag=17, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=502, seed_valid=1, seed_dram=7, seed_sram=12, seed_data=16391, seed_tag=17, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=503, seed_valid=1, seed_dram=7, seed_sram=12, seed_data=16391, seed_tag=17, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=4, expected_seed_response_sram_address=9, expected_seed_response_tag=14),
            Scenario(end=504, seed_valid=1, seed_dram=7, seed_sram=12, seed_data=16391, seed_tag=17, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=505, seed_valid=1, seed_dram=8, seed_sram=13, seed_data=16392, seed_tag=18, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=506, seed_valid=1, seed_dram=8, seed_sram=13, seed_data=16392, seed_tag=18, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=507, seed_valid=1, seed_dram=8, seed_sram=13, seed_data=16392, seed_tag=18, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=5, expected_seed_response_sram_address=10, expected_seed_response_data=4660, expected_seed_response_tag=15),
            Scenario(end=508, seed_valid=1, seed_dram=8, seed_sram=13, seed_data=16392, seed_tag=18, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=509, seed_valid=1, seed_dram=9, seed_sram=14, seed_data=16393, seed_tag=19, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=510, seed_valid=1, seed_dram=9, seed_sram=14, seed_data=16393, seed_tag=19, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=511, seed_valid=1, seed_dram=9, seed_sram=14, seed_data=16393, seed_tag=19, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=6, expected_seed_response_sram_address=11, expected_seed_response_tag=16),
            Scenario(end=512, seed_valid=1, seed_dram=9, seed_sram=14, seed_data=16393, seed_tag=19, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=513, seed_valid=1, seed_dram=10, seed_sram=15, seed_data=16394, seed_tag=20, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=514, seed_valid=1, seed_dram=10, seed_sram=15, seed_data=16394, seed_tag=20, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=515, seed_valid=1, seed_dram=10, seed_sram=15, seed_data=16394, seed_tag=20, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=7, expected_seed_response_sram_address=12, expected_seed_response_tag=17),
            Scenario(end=516, seed_valid=1, seed_dram=10, seed_sram=15, seed_data=16394, seed_tag=20, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=517, seed_valid=1, seed_dram=11, seed_data=16395, seed_tag=21, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=518, seed_valid=1, seed_dram=11, seed_data=16395, seed_tag=21, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=519, seed_valid=1, seed_dram=11, seed_data=16395, seed_tag=21, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=8, expected_seed_response_sram_address=13, expected_seed_response_tag=18),
            Scenario(end=520, seed_valid=1, seed_dram=11, seed_data=16395, seed_tag=21, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=521, seed_valid=1, seed_dram=12, seed_sram=1, seed_data=16396, seed_tag=22, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=522, seed_valid=1, seed_dram=12, seed_sram=1, seed_data=16396, seed_tag=22, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=523, seed_valid=1, seed_dram=12, seed_sram=1, seed_data=16396, seed_tag=22, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=9, expected_seed_response_sram_address=14, expected_seed_response_data=48879, expected_seed_response_tag=19),
            Scenario(end=524, seed_valid=1, seed_dram=12, seed_sram=1, seed_data=16396, seed_tag=22, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=525, seed_valid=1, seed_dram=13, seed_sram=2, seed_data=16397, seed_tag=23, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=526, seed_valid=1, seed_dram=13, seed_sram=2, seed_data=16397, seed_tag=23, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=527, seed_valid=1, seed_dram=13, seed_sram=2, seed_data=16397, seed_tag=23, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=10, expected_seed_response_sram_address=15, expected_seed_response_tag=20),
            Scenario(end=528, seed_valid=1, seed_dram=13, seed_sram=2, seed_data=16397, seed_tag=23, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=529, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=530, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=531, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=11, expected_seed_response_tag=21),
            Scenario(end=533, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=534, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=535, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_dram_accepted=2, expected_seed_response_dram_address=12, expected_seed_response_sram_address=1, expected_seed_response_tag=22),
            Scenario(end=537, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=538, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=539, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_seed_response_dram_address=13, expected_seed_response_sram_address=2, expected_seed_response_tag=23),
            Scenario(end=540, seed_valid=1, seed_dram=14, seed_sram=11, seed_data=30292, seed_tag=100, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=541, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=2),
            Scenario(end=543, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=544, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=2),
            Scenario(end=545, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_seed_valid=1, expected_seed_response_dram_address=14, expected_seed_response_sram_address=11, expected_seed_response_tag=100),
            Scenario(end=546, copy_valid=1, copy_dram=14, copy_sram=11, copy_tag=101, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=547, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=1),
            Scenario(end=549, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=550, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=1),
            Scenario(end=551, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_accepted=2),
            Scenario(end=552, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=553, check_valid=1, check_dram=6, check_sram=11, check_tag=102, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=2),
            Scenario(end=554, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_sram_accepted=1, expected_copy_response_dram_address=14, expected_copy_response_sram_address=11, expected_copy_response_tag=101),
            Scenario(end=555, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=556, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=1),
            Scenario(end=557, take_seed=1, take_copy=1, take_check=1, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_check_valid=1, expected_check_response_dram_address=6, expected_check_response_sram_address=11, expected_check_response_data=30292, expected_check_response_tag=102),
            Scenario(end=558, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=17767, seed_tag=110, copy_valid=1, copy_dram=9, copy_sram=4, copy_tag=130, check_valid=1, check_sram=3, check_tag=150, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=559, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=17767, seed_tag=111, copy_valid=1, copy_dram=9, copy_sram=4, copy_tag=131, check_valid=1, check_sram=3, check_tag=151, expected_seed_ready=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_accepted=1),
            Scenario(end=560, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=17767, seed_tag=112, copy_valid=1, copy_dram=9, copy_sram=4, copy_tag=132, check_valid=1, check_sram=3, check_tag=152, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=561, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=17767, seed_tag=113, copy_valid=1, copy_dram=9, copy_sram=4, copy_tag=133, check_valid=1, check_sram=3, check_tag=153, expected_copy_ready=1, expected_sram_enqueued=1),
            Scenario(end=562, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=17767, seed_tag=114, copy_valid=1, copy_dram=9, copy_sram=4, copy_tag=134, check_valid=1, check_sram=3, check_tag=154, expected_copy_ready=1, expected_check_valid=1, expected_dram_enqueued=1, expected_sram_accepted=1, expected_check_response_sram_address=3, expected_check_response_tag=150),
            Scenario(end=563, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=17767, seed_tag=115, copy_valid=1, copy_dram=9, copy_sram=4, copy_tag=135, check_valid=1, check_sram=3, check_tag=155, expected_check_ready=1, expected_check_valid=1, expected_dram_accepted=1, expected_check_response_sram_address=3, expected_check_response_tag=150),
            Scenario(end=564, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=17767, seed_tag=116, copy_valid=1, copy_dram=9, copy_sram=4, copy_tag=136, check_valid=1, check_sram=3, check_tag=156, expected_copy_ready=1, expected_check_valid=1, expected_sram_enqueued=1, expected_check_response_sram_address=3, expected_check_response_tag=150),
            Scenario(end=565, seed_valid=1, seed_dram=2, seed_sram=7, seed_data=17767, seed_tag=117, copy_valid=1, copy_dram=9, copy_sram=4, copy_tag=137, check_valid=1, check_sram=3, check_tag=157, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_sram_address=3, expected_check_response_tag=150),
            Scenario(end=566, take_seed=1, take_copy=1, take_check=1, expected_check_ready=1, expected_check_valid=1, expected_dram_enqueued=1, expected_check_response_sram_address=3, expected_check_response_tag=150),
            Scenario(end=567, copy_valid=1, copy_dram=14, copy_sram=11, copy_tag=180, take_seed=1, take_copy=1, take_check=1, expected_check_ready=1, expected_check_valid=1, expected_dram_accepted=1, expected_sram_enqueued=1, expected_check_response_sram_address=3, expected_check_response_tag=151),
            Scenario(end=568, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_check_valid=1, expected_sram_accepted=1, expected_check_response_sram_address=3, expected_check_response_tag=152),
            Scenario(end=569, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=570, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=1),
            Scenario(end=571, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_check_valid=1, expected_sram_accepted=2, expected_check_response_sram_address=3, expected_check_response_tag=155),
            Scenario(end=572, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=1),
            Scenario(end=573, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_accepted=1, expected_sram_enqueued=2),
            Scenario(end=574, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_sram_accepted=2, expected_copy_response_dram_address=9, expected_copy_response_sram_address=4, expected_copy_response_tag=130),
            Scenario(end=575, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=576, check_valid=1, check_sram=11, check_tag=181, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=1, expected_sram_enqueued=2),
            Scenario(end=577, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_copy_valid=1, expected_dram_accepted=1, expected_sram_accepted=1, expected_copy_response_dram_address=9, expected_copy_response_sram_address=4, expected_copy_response_data=16393, expected_copy_response_tag=131),
            Scenario(end=578, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1),
            Scenario(end=579, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_sram_enqueued=1),
            Scenario(end=580, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_check_valid=1, expected_sram_accepted=2, expected_check_response_sram_address=11, expected_check_response_data=30292, expected_check_response_tag=181),
            Scenario(end=18446744073709551615, take_seed=1, take_copy=1, take_check=1, expected_copy_ready=1, expected_check_ready=1, expected_dram_enqueued=1),
        ),
    )
    # fmt: on
    position, found = scenarios.rows.first(where=lambda row: phase <= row.end)
    scenario = scenarios.rows[position]
    seed = DmaRequest(
        dram_address=scenario.seed_dram,
        sram_address=scenario.seed_sram,
        data=scenario.seed_data,
        tag=scenario.seed_tag,
    )
    copy = DmaRequest(
        dram_address=scenario.copy_dram,
        sram_address=scenario.copy_sram,
        data=scenario.copy_data,
        tag=scenario.copy_tag,
    )
    check = DmaRequest(
        dram_address=scenario.check_dram,
        sram_address=scenario.check_sram,
        data=scenario.check_data,
        tag=scenario.check_tag,
    )
    dut = Dma(
        scenario.seed_valid,
        seed,
        scenario.copy_valid,
        copy,
        scenario.check_valid,
        check,
        scenario.take_seed,
        scenario.take_copy,
        scenario.take_check,
    )

    @rule
    def check():
        if phase < 582:
            assert dut.seed_ready == scenario.expected_seed_ready, "dma: seed_ready"
            assert dut.copy_ready == scenario.expected_copy_ready, "dma: copy_ready"
            assert dut.check_ready == scenario.expected_check_ready, "dma: check_ready"
            assert dut.seed_valid == scenario.expected_seed_valid, "dma: seed_valid"
            assert dut.copy_valid == scenario.expected_copy_valid, "dma: copy_valid"
            assert dut.check_valid == scenario.expected_check_valid, "dma: check_valid"
            assert (
                dut.dram_accepted == scenario.expected_dram_accepted
            ), "dma: dram_accepted"
            assert (
                dut.dram_enqueued == scenario.expected_dram_enqueued
            ), "dma: dram_enqueued"
            assert (
                dut.sram_accepted == scenario.expected_sram_accepted
            ), "dma: sram_accepted"
            assert (
                dut.sram_enqueued == scenario.expected_sram_enqueued
            ), "dma: sram_enqueued"
            assert (scenario.expected_seed_valid == 0) | (
                dut.seed_response.dram_address
                == scenario.expected_seed_response_dram_address
            ), "dma: seed_response.dram_address"
            assert (scenario.expected_seed_valid == 0) | (
                dut.seed_response.sram_address
                == scenario.expected_seed_response_sram_address
            ), "dma: seed_response.sram_address"
            assert (scenario.expected_seed_valid == 0) | (
                dut.seed_response.data == scenario.expected_seed_response_data
            ), "dma: seed_response.data"
            assert (scenario.expected_seed_valid == 0) | (
                dut.seed_response.tag == scenario.expected_seed_response_tag
            ), "dma: seed_response.tag"
            assert (scenario.expected_copy_valid == 0) | (
                dut.copy_response.dram_address
                == scenario.expected_copy_response_dram_address
            ), "dma: copy_response.dram_address"
            assert (scenario.expected_copy_valid == 0) | (
                dut.copy_response.sram_address
                == scenario.expected_copy_response_sram_address
            ), "dma: copy_response.sram_address"
            assert (scenario.expected_copy_valid == 0) | (
                dut.copy_response.data == scenario.expected_copy_response_data
            ), "dma: copy_response.data"
            assert (scenario.expected_copy_valid == 0) | (
                dut.copy_response.tag == scenario.expected_copy_response_tag
            ), "dma: copy_response.tag"
            assert (scenario.expected_check_valid == 0) | (
                dut.check_response.dram_address
                == scenario.expected_check_response_dram_address
            ), "dma: check_response.dram_address"
            assert (scenario.expected_check_valid == 0) | (
                dut.check_response.sram_address
                == scenario.expected_check_response_sram_address
            ), "dma: check_response.sram_address"
            assert (scenario.expected_check_valid == 0) | (
                dut.check_response.data == scenario.expected_check_response_data
            ), "dma: check_response.data"
            assert (scenario.expected_check_valid == 0) | (
                dut.check_response.tag == scenario.expected_check_response_tag
            ), "dma: check_response.tag"
        log("info", "dma.seed_ready", dut.seed_ready)
        log("info", "dma.copy_ready", dut.copy_ready)
        log("info", "dma.check_ready", dut.check_ready)
        log("info", "dma.seed_valid", dut.seed_valid)
        log("info", "dma.copy_valid", dut.copy_valid)
        log("info", "dma.check_valid", dut.check_valid)
        log("info", "dma.dram_accepted", dut.dram_accepted)
        log("info", "dma.dram_enqueued", dut.dram_enqueued)
        log("info", "dma.sram_accepted", dut.sram_accepted)
        log("info", "dma.sram_enqueued", dut.sram_enqueued)
        log("info", "dma.seed_response.dram_address", dut.seed_response.dram_address)
        log("info", "dma.seed_response.sram_address", dut.seed_response.sram_address)
        log("info", "dma.seed_response.data", dut.seed_response.data)
        log("info", "dma.seed_response.tag", dut.seed_response.tag)
        log("info", "dma.copy_response.dram_address", dut.copy_response.dram_address)
        log("info", "dma.copy_response.sram_address", dut.copy_response.sram_address)
        log("info", "dma.copy_response.data", dut.copy_response.data)
        log("info", "dma.copy_response.tag", dut.copy_response.tag)
        log("info", "dma.check_response.dram_address", dut.check_response.dram_address)
        log("info", "dma.check_response.sram_address", dut.check_response.sram_address)
        log("info", "dma.check_response.data", dut.check_response.data)
        log("info", "dma.check_response.tag", dut.check_response.tag)

    check()
    advance(phase)

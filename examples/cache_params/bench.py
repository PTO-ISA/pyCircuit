"""Complete known-input table from the retained independent cache_params oracle."""

from example_cache_params.cache_params import CacheParams
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    addr: bits[40]
    expected_tag: bits[28]


@struct
class Scenarios:
    rows: table[14, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseCacheParams():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(),
            Scenario(addr=2048),
            Scenario(addr=4095),
            Scenario(addr=4096, expected_tag=1),
            Scenario(addr=4097, expected_tag=1),
            Scenario(addr=8191, expected_tag=1),
            Scenario(addr=8192, expected_tag=2),
            Scenario(addr=549755813888, expected_tag=134217728),
            Scenario(addr=1099511627775, expected_tag=268435455),
            Scenario(addr=78187493530, expected_tag=19088743),
            Scenario(addr=737894400291, expected_tag=180150000),
            Scenario(addr=178954240, expected_tag=43690),
            Scenario(addr=89477120, expected_tag=21845),
            Scenario(),
        ),
    )
    # fmt: on
    position = phase if phase < 14 else 13
    scenario = scenarios.rows[position]
    addr = scenario.addr
    dut = CacheParams(addr)

    expected_tag = scenario.expected_tag

    @rule
    def check_and_advance():
        if phase < 14:
            assert dut.tag == expected_tag, "cache_params: tag"
            assert dut.line_words == 8, "cache_params: line words"
            assert dut.tag_bits == 28, "cache_params: tag width"

        log("info", "cache_params.tag", dut.tag)

    check_and_advance()
    advance(phase)

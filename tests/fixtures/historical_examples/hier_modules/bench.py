"""Complete known-input table from the retained independent hier_modules oracle."""

from example_hier_modules.hier_modules import HierModules
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    x: bits[8]
    expected_y: bits[8]


@struct
class Scenarios:
    rows: table[13, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseHierModules():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(x=1, expected_y=4),
            Scenario(expected_y=3),
            Scenario(x=252, expected_y=255),
            Scenario(x=253),
            Scenario(x=254, expected_y=1),
            Scenario(x=255, expected_y=2),
            Scenario(x=2, expected_y=5),
            Scenario(x=127, expected_y=130),
            Scenario(x=128, expected_y=131),
            Scenario(x=250, expected_y=253),
            Scenario(x=5, expected_y=8),
            Scenario(x=17, expected_y=20),
            Scenario(),
        ),
    )
    # fmt: on
    position = phase if phase < 12 else 12
    scenario = scenarios.rows[position]
    x = scenario.x
    dut = HierModules(x)

    expected_y = scenario.expected_y

    @rule
    def check_and_advance():
        if phase < 12:
            assert dut.y == expected_y, "hier_modules: y"

        log("info", "hier_modules.y", dut.y)

    check_and_advance()
    advance(phase)

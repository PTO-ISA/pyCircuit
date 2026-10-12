"""Complete known-input table from the retained independent boundary_value_ports oracle."""

from example_boundary_value_ports.boundary_value_ports import BoundaryValuePorts
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    seed: bits[32]
    expected_acc: bits[32]


@struct
class Scenarios:
    rows: table[17, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseBoundaryValuePorts():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(seed=10, expected_acc=48),
            Scenario(expected_acc=18),
            Scenario(seed=1, expected_acc=21),
            Scenario(seed=4294967283, expected_acc=4294967275),
            Scenario(seed=4294967284, expected_acc=4294967278),
            Scenario(seed=4294967289, expected_acc=4294967293),
            Scenario(seed=4294967290),
            Scenario(seed=4294967291, expected_acc=3),
            Scenario(seed=4294967294, expected_acc=12),
            Scenario(seed=4294967295, expected_acc=15),
            Scenario(seed=2147483648, expected_acc=2147483666),
            Scenario(seed=2147483647, expected_acc=2147483663),
            Scenario(seed=1073741824, expected_acc=3221225490),
            Scenario(seed=1431655765, expected_acc=17),
            Scenario(seed=2863311530, expected_acc=16),
            Scenario(seed=17, expected_acc=69),
            Scenario(),
        ),
    )
    # fmt: on
    position = phase if phase < 16 else 16
    scenario = scenarios.rows[position]
    seed = scenario.seed
    dut = BoundaryValuePorts(seed)

    expected_acc = scenario.expected_acc

    @rule
    def check_and_advance():
        if phase < 16:
            assert dut.acc == expected_acc, "boundary_value_ports: acc"

        log("info", "boundary_value_ports.acc", dut.acc)

    check_and_advance()
    advance(phase)

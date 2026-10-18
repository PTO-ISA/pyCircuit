"""Complete known-input table from the retained independent arith oracle."""

from example_arith.arith import Arith
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    a: bits[19]
    b: bits[19]
    expected_sum: bits[19]


@struct
class Scenarios:
    rows: table[11, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseArith():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(a=1, b=2, expected_sum=3),
            Scenario(),
            Scenario(a=524287, expected_sum=524287),
            Scenario(a=524287, b=1),
            Scenario(a=524287, b=524287, expected_sum=524286),
            Scenario(a=262144, b=262144),
            Scenario(a=262143, b=1, expected_sum=262144),
            Scenario(a=524286, b=3, expected_sum=1),
            Scenario(a=273067, b=174762, expected_sum=447829),
            Scenario(a=12345, b=54321, expected_sum=66666),
            Scenario(),
        ),
    )
    # fmt: on
    position = phase if phase < 10 else 10
    scenario = scenarios.rows[position]
    a = scenario.a
    b = scenario.b
    dut = Arith(a, b)

    expected_sum = scenario.expected_sum

    @rule
    def check_and_advance():
        if phase < 10:
            assert dut.sum == expected_sum, "arith: sum"
            assert dut.lane_mask == 65535, "arith: lane mask"
            assert dut.acc_width == 19, "arith: accumulator width"

        log("info", "arith.sum", dut.sum)

    check_and_advance()
    advance(phase)

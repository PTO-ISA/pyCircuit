"""Complete known-input table from the retained independent jit_control_flow oracle."""

from example_jit_control_flow.jit_control_flow import JitControlFlow
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    a: bits[8]
    b: bits[8]
    op: bits[2]
    expected_result: bits[8]


@struct
class Scenarios:
    rows: table[66, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseJitControlFlow():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(a=1, b=2, expected_result=7),
            Scenario(expected_result=4),
            Scenario(op=1, expected_result=4),
            Scenario(op=2, expected_result=4),
            Scenario(op=3, expected_result=4),
            Scenario(a=1, b=2, expected_result=7),
            Scenario(a=1, b=2, op=1, expected_result=3),
            Scenario(a=1, b=2, op=2, expected_result=7),
            Scenario(a=1, b=2, op=3, expected_result=4),
            Scenario(a=255, b=1, expected_result=4),
            Scenario(a=255, b=1, op=1, expected_result=2),
            Scenario(a=255, b=1, op=2, expected_result=2),
            Scenario(a=255, b=1, op=3, expected_result=5),
            Scenario(b=255, expected_result=3),
            Scenario(b=255, op=1, expected_result=5),
            Scenario(b=255, op=2, expected_result=3),
            Scenario(b=255, op=3, expected_result=4),
            Scenario(a=254, b=255, expected_result=1),
            Scenario(a=254, b=255, op=1, expected_result=3),
            Scenario(a=254, b=255, op=2, expected_result=5),
            Scenario(a=254, b=255, op=3, expected_result=2),
            Scenario(a=128, b=127, expected_result=3),
            Scenario(a=128, b=127, op=1, expected_result=5),
            Scenario(a=128, b=127, op=2, expected_result=3),
            Scenario(a=128, b=127, op=3, expected_result=4),
            Scenario(a=128, b=128, expected_result=4),
            Scenario(a=128, b=128, op=1, expected_result=4),
            Scenario(a=128, b=128, op=2, expected_result=4),
            Scenario(a=128, b=128, op=3, expected_result=132),
            Scenario(a=255, b=255, expected_result=2),
            Scenario(a=255, b=255, op=1, expected_result=4),
            Scenario(a=255, b=255, op=2, expected_result=4),
            Scenario(a=255, b=255, op=3, expected_result=3),
            Scenario(a=85, b=170, expected_result=3),
            Scenario(a=85, b=170, op=1, expected_result=175),
            Scenario(a=85, b=170, op=2, expected_result=3),
            Scenario(a=85, b=170, op=3, expected_result=4),
            Scenario(a=170, b=85, expected_result=3),
            Scenario(a=170, b=85, op=1, expected_result=89),
            Scenario(a=170, b=85, op=2, expected_result=3),
            Scenario(a=170, b=85, op=3, expected_result=4),
            Scenario(a=252, b=4, expected_result=4),
            Scenario(a=252, b=4, op=1, expected_result=252),
            Scenario(a=252, b=4, op=2, expected_result=252),
            Scenario(a=252, b=4, op=3, expected_result=8),
            Scenario(a=253, b=3, expected_result=4),
            Scenario(a=253, b=3, op=1, expected_result=254),
            Scenario(a=253, b=3, op=2, expected_result=2),
            Scenario(a=253, b=3, op=3, expected_result=5),
            Scenario(a=250, b=9, expected_result=7),
            Scenario(a=250, b=9, op=1, expected_result=245),
            Scenario(a=250, b=9, op=2, expected_result=247),
            Scenario(a=250, b=9, op=3, expected_result=12),
            Scenario(a=7, b=200, expected_result=211),
            Scenario(a=7, b=200, op=1, expected_result=67),
            Scenario(a=7, b=200, op=2, expected_result=211),
            Scenario(a=7, b=200, op=3, expected_result=4),
            Scenario(a=17, b=31, expected_result=52),
            Scenario(a=17, b=31, op=1, expected_result=246),
            Scenario(a=17, b=31, op=2, expected_result=18),
            Scenario(a=17, b=31, op=3, expected_result=21),
            Scenario(a=254, expected_result=2),
            Scenario(a=254, op=1, expected_result=2),
            Scenario(a=254, op=2, expected_result=2),
            Scenario(a=254, op=3, expected_result=4),
            Scenario(),
        ),
    )
    # fmt: on
    position = phase if phase < 65 else 65
    scenario = scenarios.rows[position]
    a = scenario.a
    b = scenario.b
    op = scenario.op
    dut = JitControlFlow(a, b, op)

    expected_result = scenario.expected_result

    @rule
    def check_and_advance():
        if phase < 65:
            assert dut.result == expected_result, "jit_control_flow: result"

        log("info", "jit_control_flow.result", dut.result)

    check_and_advance()
    advance(phase)

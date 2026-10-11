"""Regular-clock jit_pipeline_vec scenario; original physical-control oracles remain."""

from example_jit_pipeline_vec.jit_pipeline_vec import JitPipelineVec
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    a: bits[16]
    b: bits[16]
    sel: bits[1]
    expected_tag: bits[1]
    expected_data: bits[16]
    expected_lo8: bits[8]


@struct
class Scenarios:
    rows: table[75, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseJitPipelineVec():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(a=1, b=1, sel=1),
            Scenario(a=65535, b=1, sel=1),
            Scenario(a=85, b=170),
            Scenario(a=3, b=1, expected_tag=1, expected_data=2, expected_lo8=2),
            Scenario(a=65535, b=1, sel=1),
            Scenario(a=1, b=1, sel=1, expected_data=255, expected_lo8=255),
            Scenario(a=65535, b=1, sel=1, expected_data=2, expected_lo8=2),
            Scenario(a=65535, b=1, sel=1),
            Scenario(a=65535, b=65535, sel=1, expected_tag=1, expected_data=2, expected_lo8=2),
            Scenario(b=65535),
            Scenario(a=32768, b=32768, sel=1),
            Scenario(a=85, b=170, expected_tag=1, expected_data=65534, expected_lo8=254),
            Scenario(a=85, b=170, sel=1, expected_data=65535, expected_lo8=255),
            Scenario(a=3, b=1, expected_tag=1),
            Scenario(a=1, b=1, sel=1, expected_data=255, expected_lo8=255),
            Scenario(a=65109, b=2716, expected_data=255, expected_lo8=255),
            Scenario(a=4662, b=42315, sel=1, expected_data=2, expected_lo8=2),
            Scenario(a=4964, b=14894, expected_tag=1, expected_data=2, expected_lo8=2),
            Scenario(a=54834, b=15285, sel=1, expected_data=62665, expected_lo8=201),
            Scenario(a=62835, b=6544, expected_data=46977, expected_lo8=129),
            Scenario(a=3718, b=40879, sel=1, expected_data=10570, expected_lo8=74),
            Scenario(a=13563, b=51522, expected_data=4583, expected_lo8=231),
            Scenario(a=59098, b=30649, sel=1, expected_data=60643, expected_lo8=227),
            Scenario(a=45643, b=48580, expected_data=44597, expected_lo8=53),
            Scenario(a=7, b=44627, sel=1, expected_data=64953, expected_lo8=185),
            Scenario(a=21090, b=57238, expected_data=24211, expected_lo8=147),
            Scenario(a=42544, b=4605, sel=1, expected_data=3983, expected_lo8=143),
            Scenario(a=38335, b=43832, expected_data=44634, expected_lo8=90),
            Scenario(a=60890, b=62775, sel=1, expected_data=36340, expected_lo8=244),
            Scenario(a=32928, b=20778, expected_data=47149, expected_lo8=45),
            Scenario(a=37379, b=52865, sel=1, expected_data=16007, expected_lo8=135),
            Scenario(a=21888, b=54764, expected_data=58129, expected_lo8=17),
            Scenario(a=52545, b=55387, sel=1, expected_data=53642, expected_lo8=138),
            Scenario(a=1502, b=12798, expected_data=24708, expected_lo8=132),
            Scenario(a=62064, b=45381, sel=1, expected_data=32876, expected_lo8=108),
            Scenario(a=49798, b=29152, expected_data=42396, expected_lo8=156),
            Scenario(a=52609, b=64447, sel=1, expected_data=13344, expected_lo8=32),
            Scenario(a=3069, b=54802, expected_data=41909, expected_lo8=181),
            Scenario(a=58207, b=65097, sel=1, expected_data=45926, expected_lo8=102),
            Scenario(a=56512, b=62228, expected_data=51520, expected_lo8=64),
            Scenario(a=4105, b=17251, sel=1, expected_data=56815, expected_lo8=239),
            Scenario(a=21099, b=53606, expected_data=57768, expected_lo8=168),
            Scenario(a=54349, b=14733, sel=1, expected_data=12244, expected_lo8=212),
            Scenario(a=26638, b=3464, expected_data=21356, expected_lo8=108),
            Scenario(a=28881, b=54087, sel=1, expected_data=33549, expected_lo8=13),
            Scenario(a=13107, b=63482, expected_data=3546, expected_lo8=218),
            Scenario(a=49738, b=10001, sel=1, expected_data=25990, expected_lo8=134),
            Scenario(a=30316, b=46396, expected_data=17432, expected_lo8=24),
            Scenario(a=17158, b=3947, sel=1, expected_data=50377, expected_lo8=201),
            Scenario(a=36693, b=24014, expected_data=59739, expected_lo8=91),
            Scenario(a=45122, b=51925, sel=1, expected_data=50000, expected_lo8=80),
            Scenario(a=39793, b=7728, expected_data=21105, expected_lo8=113),
            Scenario(a=9700, b=39887, sel=1, expected_data=53915, expected_lo8=155),
            Scenario(a=30294, b=22242, expected_data=31511, expected_lo8=23),
            Scenario(a=8577, b=26841, sel=1, expected_data=34113, expected_lo8=65),
            Scenario(a=48376, b=48228, expected_data=49587, expected_lo8=179),
            Scenario(a=21988, b=23667, sel=1, expected_data=8372, expected_lo8=180),
            Scenario(a=40216, b=30518, expected_data=35418, expected_lo8=90),
            Scenario(a=39538, b=34077, sel=1, expected_data=156, expected_lo8=156),
            Scenario(a=46366, b=17368, expected_data=45655, expected_lo8=87),
            Scenario(a=7704, b=30039, sel=1, expected_data=59950, expected_lo8=46),
            Scenario(a=63468, b=37578, expected_data=8079, expected_lo8=143),
            Scenario(a=27560, b=58273, sel=1, expected_data=63174, expected_lo8=198),
            Scenario(a=14701, b=43148, expected_data=37743, expected_lo8=111),
            Scenario(a=19924, b=19067, sel=1, expected_data=25894, expected_lo8=38),
            Scenario(a=3046, b=48542, expected_data=20297, expected_lo8=73),
            Scenario(a=19233, b=34917, sel=1, expected_data=37345, expected_lo8=225),
            Scenario(a=47960, b=7808, expected_data=38991, expected_lo8=79),
            Scenario(a=54927, b=32735, sel=1, expected_data=46712, expected_lo8=120),
            Scenario(a=52082, b=19378, expected_data=54150, expected_lo8=134),
            Scenario(a=32711, b=46953, sel=1, expected_data=42456, expected_lo8=216),
            Scenario(a=1, b=1, sel=1, expected_data=22126, expected_lo8=110),
            Scenario(a=1, b=1, sel=1, expected_data=32960, expected_lo8=192),
            Scenario(a=1, b=1, sel=1, expected_data=14128, expected_lo8=48),
            Scenario(a=1, b=1, sel=1, expected_tag=1, expected_data=2, expected_lo8=2),
        ),
    )
    # fmt: on
    position = phase if phase < 75 else 74
    scenario = scenarios.rows[position]
    a = scenario.a
    b = scenario.b
    sel = scenario.sel
    dut = JitPipelineVec(a, b, sel)

    expected_tag = scenario.expected_tag
    expected_data = scenario.expected_data
    expected_lo8 = scenario.expected_lo8

    @rule
    def check_and_advance():
        if phase < 75:
            assert dut.tag == expected_tag, "jit_pipeline_vec: tag"
            assert dut.data == expected_data, "jit_pipeline_vec: data"
            assert dut.lo8 == expected_lo8, "jit_pipeline_vec: lo8"

        log("info", "jit_pipeline_vec.tag", dut.tag)
        log("info", "jit_pipeline_vec.data", dut.data)
        log("info", "jit_pipeline_vec.lo8", dut.lo8)

    check_and_advance()
    advance(phase)

"""Regular-clock bf16_fmac scenario; original physical-control oracles remain."""

from example_bf16_fmac.bf16_fmac import BF16Fmac
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    a_in: bits[16]
    b_in: bits[16]
    acc_in: bits[32]
    valid_in: bits[1]
    expected_result: bits[32]
    expected_result_valid: bits[1]


def scenario_rows() -> table[86, Scenario]:
    # fmt: off
    rows: table[86, Scenario] = (
        Scenario(),
        Scenario(a_in=16256, b_in=16256, valid_in=1),
        Scenario(a_in=49024, b_in=16256),
        Scenario(a_in=49024, b_in=16256),
        Scenario(a_in=49024, b_in=16256),
        Scenario(a_in=49024, b_in=16256, expected_result=1065353216, expected_result_valid=1),
        Scenario(a_in=49024, b_in=16256, expected_result=1065353216),
        Scenario(valid_in=1, expected_result=1065353216),
        Scenario(b_in=16256, acc_in=1065427781, valid_in=1, expected_result=1065353216),
        Scenario(a_in=32768, b_in=16256, acc_in=3221225472, valid_in=1, expected_result=1065353216),
        Scenario(a_in=16256, b_in=16256, acc_in=1065353216, valid_in=1, expected_result=1065353216),
        Scenario(valid_in=1, expected_result_valid=1),
        Scenario(b_in=16256, acc_in=1065427781, valid_in=1, expected_result=1065427781, expected_result_valid=1),
        Scenario(a_in=32768, b_in=16256, acc_in=3221225472, valid_in=1, expected_result=3221225472, expected_result_valid=1),
        Scenario(a_in=127, b_in=16383, acc_in=3212836864, valid_in=1, expected_result=1073741824, expected_result_valid=1),
        Scenario(a_in=16320, b_in=16320, expected_result_valid=1),
        Scenario(a_in=16256, b_in=16256, valid_in=1, expected_result=1065427781, expected_result_valid=1),
        Scenario(a_in=16256, b_in=16256, acc_in=1065353216, valid_in=1, expected_result=3221225472, expected_result_valid=1),
        Scenario(a_in=49024, b_in=16256, valid_in=1, expected_result=3212836864, expected_result_valid=1),
        Scenario(a_in=16256, b_in=49024, acc_in=1065353216, valid_in=1, expected_result=3212836864),
        Scenario(a_in=16256, b_in=16256, acc_in=1283457024, expected_result=1065353216, expected_result_valid=1),
        Scenario(a_in=16256, b_in=16256, acc_in=3221225472, valid_in=1, expected_result=1073741824, expected_result_valid=1),
        Scenario(a_in=49024, b_in=49024, acc_in=3212836864, valid_in=1, expected_result=3212836864, expected_result_valid=1),
        Scenario(a_in=16320, b_in=16320, valid_in=1, expected_result_valid=1),
        Scenario(a_in=16383, b_in=16383, valid_in=1),
        Scenario(a_in=16256, b_in=16256, acc_in=8388607, expected_result=3212836864, expected_result_valid=1),
        Scenario(a_in=16257, b_in=16257, valid_in=1, expected_result_valid=1),
        Scenario(a_in=16256, b_in=16256, acc_in=1275068416, valid_in=1, expected_result=1074790400, expected_result_valid=1),
        Scenario(a_in=16256, b_in=16256, acc_in=1283457024, valid_in=1, expected_result=1081999360, expected_result_valid=1),
        Scenario(a_in=16256, b_in=16256, acc_in=1291845632, valid_in=1, expected_result=1081999360),
        Scenario(a_in=13350, b_in=47178, acc_in=980349794, expected_result=1065484800, expected_result_valid=1),
        Scenario(a_in=128, b_in=128, valid_in=1, expected_result=1275068416, expected_result_valid=1),
        Scenario(a_in=32512, b_in=32512, valid_in=1, expected_result=1283457024, expected_result_valid=1),
        Scenario(a_in=16256, b_in=16256, acc_in=8388607, valid_in=1, expected_result=1291845632, expected_result_valid=1),
        Scenario(a_in=16256, b_in=16256, acc_in=2147483648, valid_in=1, expected_result=1291845632),
        Scenario(a_in=17010, b_in=52830, acc_in=2966215206, expected_result=1098907648, expected_result_valid=1),
        Scenario(a_in=11520, b_in=11520, acc_in=754974720, valid_in=1, expected_result=1048576000, expected_result_valid=1),
        Scenario(a_in=45203, b_in=12965, acc_in=867662257, valid_in=1, expected_result=1065353216, expected_result_valid=1),
        Scenario(a_in=13350, b_in=47178, acc_in=980349794, valid_in=1, expected_result=1065353216, expected_result_valid=1),
        Scenario(a_in=47033, b_in=48623, acc_in=1084648723, valid_in=1, expected_result=1065353216),
        Scenario(a_in=20542, b_in=49394, acc_in=1252704490, expected_result=754974720, expected_result_valid=1),
        Scenario(a_in=15180, b_in=17172, acc_in=3344819908, valid_in=1, expected_result=867662257, expected_result_valid=1),
        Scenario(a_in=48863, b_in=18617, acc_in=3449118837, valid_in=1, expected_result=980349794, expected_result_valid=1),
        Scenario(a_in=17010, b_in=52830, acc_in=2966215206, valid_in=1, expected_result=1084648728, expected_result_valid=1),
        Scenario(a_in=50565, b_in=45059, acc_in=3070514135, valid_in=1, expected_result=1084648728),
        Scenario(a_in=14986, b_in=45830, acc_in=3238569902, expected_result=3344819791, expected_result_valid=1),
        Scenario(a_in=18712, b_in=13736, acc_in=1035718024, valid_in=1, expected_result=3449123993, expected_result_valid=1),
        Scenario(a_in=52395, b_in=15181, acc_in=1148405561, valid_in=1, expected_result=3511802880, expected_result_valid=1),
        Scenario(a_in=20542, b_in=49394, acc_in=1252704490, valid_in=1, expected_result=3066337454, expected_result_valid=1),
        Scenario(a_in=45137, b_in=50711, acc_in=769800859, valid_in=1, expected_result=3066337454),
        Scenario(a_in=18646, b_in=51482, acc_in=937856626, expected_result=1063188913, expected_result_valid=1),
        Scenario(a_in=13284, b_in=19388, acc_in=3021583436, valid_in=1, expected_result=3364385885, expected_result_valid=1),
        Scenario(a_in=46967, b_in=11745, acc_in=3134270973, valid_in=1, expected_result=3518208341, expected_result_valid=1),
        Scenario(a_in=14986, b_in=45830, acc_in=3238569902, valid_in=1, expected_result=922127928, expected_result_valid=1),
        Scenario(a_in=48669, b_in=47275, acc_in=3351257439, valid_in=1, expected_result=922127928),
        Scenario(a_in=13090, b_in=48046, acc_in=2923722038, expected_result=1076326400, expected_result_valid=1),
        Scenario(a_in=16816, b_in=15952, acc_in=1316461328, valid_in=1, expected_result=3134270973, expected_result_valid=1),
        Scenario(a_in=50499, b_in=17397, acc_in=825169089, valid_in=1, expected_result=3238569902, expected_result_valid=1),
        Scenario(a_in=18646, b_in=51482, acc_in=937856626, valid_in=1, expected_result=3351257439, expected_result_valid=1),
        Scenario(a_in=52329, b_in=52927, acc_in=1042155555, valid_in=1, expected_result=3351257439),
        Scenario(a_in=16750, b_in=44610, acc_in=1210211322, expected_result=1316461328, expected_result_valid=1),
        Scenario(a_in=20476, b_in=12516, acc_in=3302326740, valid_in=1, expected_result=3384450560, expected_result_valid=1),
        Scenario(a_in=44943, b_in=13833, acc_in=3406625669, valid_in=1, expected_result=3531652096, expected_result_valid=1),
        Scenario(a_in=13090, b_in=48046, acc_in=2923722038, valid_in=1, expected_result=1538119168, expected_result_valid=1),
        Scenario(a_in=46773, b_in=49491, acc_in=3036409575, valid_in=1, expected_result=1538119168),
        Scenario(a_in=20282, b_in=50262, acc_in=3196076734, expected_result=3302211828, expected_result_valid=1),
        Scenario(a_in=14920, b_in=18168, acc_in=993224856, valid_in=1, expected_result=3406625669, expected_result_valid=1),
        Scenario(a_in=48603, b_in=19485, acc_in=1105912393, valid_in=1, expected_result=2944838310, expected_result_valid=1),
        Scenario(a_in=16750, b_in=44610, acc_in=1210211322, valid_in=1, expected_result=949236242, expected_result_valid=1),
        Scenario(a_in=50305, b_in=46055, acc_in=1322898859, valid_in=1, expected_result=949236242),
        Scenario(a_in=32768, b_in=16256, acc_in=3221225472, expected_result=1103218075, expected_result_valid=1),
        Scenario(a_in=18452, b_in=14604, acc_in=2979090268, valid_in=1, expected_result=3397799366, expected_result_valid=1),
        Scenario(a_in=52135, b_in=16049, acc_in=3091777805, valid_in=1, expected_result=1210211322, expected_result_valid=1),
        Scenario(a_in=20282, b_in=50262, acc_in=3196076734, valid_in=1, expected_result=1322898859, expected_result_valid=1),
        Scenario(a_in=44877, b_in=51707, acc_in=3308764271, valid_in=1, expected_result=1322898859),
        Scenario(a_in=49024, b_in=16256, expected_result=1101127680, expected_result_valid=1),
        Scenario(expected_result=3404131840, expected_result_valid=1),
        Scenario(expected_result=3558571008, expected_result_valid=1),
        Scenario(expected_result=3308764270, expected_result_valid=1),
        Scenario(expected_result=3308764270),
        Scenario(expected_result=3308764270),
        Scenario(expected_result=3308764270),
        Scenario(expected_result=3308764270),
        Scenario(expected_result=3308764270),
        Scenario(expected_result=3308764270),
        Scenario(expected_result=3308764270),
    )
    # fmt: on
    return rows


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseBF16Fmac():  # noqa: N802
    phase: bits[64] = 0

    scenarios = scenario_rows()
    position = phase if phase < 86 else 85
    scenario = scenarios[position]
    a_in = scenario.a_in
    b_in = scenario.b_in
    acc_in = scenario.acc_in
    valid_in = scenario.valid_in
    dut = BF16Fmac(a_in, b_in, acc_in, valid_in)

    expected_result = scenario.expected_result
    expected_result_valid = scenario.expected_result_valid

    @rule
    def check_and_advance():
        if phase < 86:
            assert dut.result == expected_result, "bf16_fmac: result"
            assert dut.result_valid == expected_result_valid, "bf16_fmac: result_valid"

        log("info", "bf16_fmac.result", dut.result)
        log("info", "bf16_fmac.result_valid", dut.result_valid)

    advance(phase)

"""Regular-clock digital_filter scenario; original physical-control oracles remain."""

from example_digital_filter.digital_filter import DigitalFilter
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    x_in: bits[16]
    x_valid: bits[1]
    expected_y_out: bits[34]
    expected_y_valid: bits[1]


@struct
class Scenarios:
    rows: table[177, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseDigitalFilter():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(x_in=999, x_valid=1),
            Scenario(x_in=1, x_valid=1, expected_y_out=999, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=1999, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=2999, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=3999, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=4, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_valid=1),
            Scenario(x_in=999, x_valid=1, expected_y_valid=1),
            Scenario(x_in=65535, x_valid=1, expected_y_out=999, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=1997, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=2995, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=3993, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=17179869180, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_valid=1),
            Scenario(x_in=999, x_valid=1, expected_y_valid=1),
            Scenario(x_in=1, x_valid=1, expected_y_out=999, expected_y_valid=1),
            Scenario(x_in=1, x_valid=1, expected_y_out=1999, expected_y_valid=1),
            Scenario(x_in=1, x_valid=1, expected_y_out=3000, expected_y_valid=1),
            Scenario(x_in=1, x_valid=1, expected_y_out=4002, expected_y_valid=1),
            Scenario(x_in=1, x_valid=1, expected_y_out=10, expected_y_valid=1),
            Scenario(x_in=1, x_valid=1, expected_y_out=10, expected_y_valid=1),
            Scenario(x_in=1, x_valid=1, expected_y_out=10, expected_y_valid=1),
            Scenario(x_in=1, x_valid=1, expected_y_out=10, expected_y_valid=1),
            Scenario(x_in=999, x_valid=1, expected_y_out=10, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=1008, expected_y_valid=1),
            Scenario(x_in=1, x_valid=1, expected_y_out=2005, expected_y_valid=1),
            Scenario(x_in=2, x_valid=1, expected_y_out=3002, expected_y_valid=1),
            Scenario(x_in=3, x_valid=1, expected_y_out=4000, expected_y_valid=1),
            Scenario(x_in=4, x_valid=1, expected_y_out=10, expected_y_valid=1),
            Scenario(x_in=5, x_valid=1, expected_y_out=20, expected_y_valid=1),
            Scenario(x_in=6, x_valid=1, expected_y_out=30, expected_y_valid=1),
            Scenario(x_in=7, x_valid=1, expected_y_out=40, expected_y_valid=1),
            Scenario(x_in=999, x_valid=1, expected_y_out=50, expected_y_valid=1),
            Scenario(x_in=100, x_valid=1, expected_y_out=1051, expected_y_valid=1),
            Scenario(x_in=65436, x_valid=1, expected_y_out=2143, expected_y_valid=1),
            Scenario(x_in=100, x_valid=1, expected_y_out=3125, expected_y_valid=1),
            Scenario(x_in=65436, x_valid=1, expected_y_out=4196, expected_y_valid=1),
            Scenario(x_in=100, x_valid=1, expected_y_out=200, expected_y_valid=1),
            Scenario(x_in=65436, x_valid=1, expected_y_out=17179868984, expected_y_valid=1),
            Scenario(x_in=100, x_valid=1, expected_y_out=200, expected_y_valid=1),
            Scenario(x_in=65436, x_valid=1, expected_y_out=17179868984, expected_y_valid=1),
            Scenario(x_in=999, x_valid=1, expected_y_out=200, expected_y_valid=1),
            Scenario(x_in=10000, x_valid=1, expected_y_out=699, expected_y_valid=1),
            Scenario(x_in=10000, x_valid=1, expected_y_out=12098, expected_y_valid=1),
            Scenario(x_in=10000, x_valid=1, expected_y_out=32597, expected_y_valid=1),
            Scenario(x_in=10000, x_valid=1, expected_y_out=63996, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=100000, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=90000, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=70000, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=40000, expected_y_valid=1),
            Scenario(x_in=999, x_valid=1, expected_y_valid=1),
            Scenario(x_in=32767, x_valid=1, expected_y_out=999, expected_y_valid=1),
            Scenario(x_in=32767, x_valid=1, expected_y_out=34765, expected_y_valid=1),
            Scenario(x_in=32767, x_valid=1, expected_y_out=101298, expected_y_valid=1),
            Scenario(x_in=32767, x_valid=1, expected_y_out=200598, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=327670, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=294903, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=229369, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=131068, expected_y_valid=1),
            Scenario(x_in=999, x_valid=1, expected_y_valid=1),
            Scenario(x_in=32768, x_valid=1, expected_y_out=999, expected_y_valid=1),
            Scenario(x_in=32768, x_valid=1, expected_y_out=17179838414, expected_y_valid=1),
            Scenario(x_in=32768, x_valid=1, expected_y_out=17179773877, expected_y_valid=1),
            Scenario(x_in=32768, x_valid=1, expected_y_out=17179676572, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=17179541504, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=17179574272, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=17179639808, expected_y_valid=1),
            Scenario(x_valid=1, expected_y_out=17179738112, expected_y_valid=1),
            Scenario(x_in=999, x_valid=1, expected_y_valid=1),
            Scenario(x_in=3, x_valid=1, expected_y_out=999, expected_y_valid=1),
            Scenario(x_in=65529, x_valid=1, expected_y_out=2001, expected_y_valid=1),
            Scenario(x_in=11, x_valid=1, expected_y_out=2996, expected_y_valid=1),
            Scenario(x_in=65535, expected_y_out=4002, expected_y_valid=1),
            Scenario(x_in=61964, expected_y_out=4002),
            Scenario(x_in=58393, expected_y_out=4002),
            Scenario(x_in=54822, expected_y_out=4002),
            Scenario(x_in=51251, expected_y_out=4002),
            Scenario(x_in=47680, expected_y_out=4002),
            Scenario(x_in=44109, expected_y_out=4002),
            Scenario(x_in=40538, expected_y_out=4002),
            Scenario(x_in=36967, expected_y_out=4002),
            Scenario(x_in=33396, expected_y_out=4002),
            Scenario(x_in=29825, expected_y_out=4002),
            Scenario(x_in=26254, expected_y_out=4002),
            Scenario(x_in=22683, expected_y_out=4002),
            Scenario(x_in=19112, expected_y_out=4002),
            Scenario(x_in=15541, expected_y_out=4002),
            Scenario(x_in=11970, expected_y_out=4002),
            Scenario(x_in=8399, expected_y_out=4002),
            Scenario(x_in=4828, expected_y_out=4002),
            Scenario(x_in=1257, expected_y_out=4002),
            Scenario(x_in=63222, expected_y_out=4002),
            Scenario(x_in=59651, expected_y_out=4002),
            Scenario(x_in=56080, expected_y_out=4002),
            Scenario(x_in=52509, expected_y_out=4002),
            Scenario(x_in=48938, expected_y_out=4002),
            Scenario(x_in=45367, expected_y_out=4002),
            Scenario(x_in=41796, expected_y_out=4002),
            Scenario(x_in=38225, expected_y_out=4002),
            Scenario(x_in=34654, expected_y_out=4002),
            Scenario(x_in=31083, expected_y_out=4002),
            Scenario(x_in=27512, expected_y_out=4002),
            Scenario(x_in=23941, expected_y_out=4002),
            Scenario(x_in=20370, expected_y_out=4002),
            Scenario(x_in=65519, x_valid=1, expected_y_out=4002),
            Scenario(x_in=4567, x_valid=1, expected_y_out=17179869180, expected_y_valid=1),
            Scenario(x_in=65413, x_valid=1, expected_y_out=4538, expected_y_valid=1),
            Scenario(x_in=999, expected_y_out=9004, expected_y_valid=1),
            Scenario(x_in=63826, expected_y_out=9004),
            Scenario(x_in=30387, x_valid=1, expected_y_out=9004),
            Scenario(x_in=50398, x_valid=1, expected_y_out=43774, expected_y_valid=1),
            Scenario(x_in=33299, x_valid=1, expected_y_out=63535, expected_y_valid=1),
            Scenario(x_in=10334, x_valid=1, expected_y_out=28156, expected_y_valid=1),
            Scenario(x_in=10719, x_valid=1, expected_y_out=21994, expected_y_valid=1),
            Scenario(x_in=48105, x_valid=1, expected_y_out=17179743308, expected_y_valid=1),
            Scenario(x_in=60486, expected_y_out=17179775245, expected_y_valid=1),
            Scenario(x_in=6946, x_valid=1, expected_y_out=17179775245),
            Scenario(x_in=54147, x_valid=1, expected_y_out=45577, expected_y_valid=1),
            Scenario(x_in=37803, x_valid=1, expected_y_out=17179862270, expected_y_valid=1),
            Scenario(x_in=7304, x_valid=1, expected_y_out=17179769787, expected_y_valid=1),
            Scenario(x_in=26616, x_valid=1, expected_y_out=17179814639, expected_y_valid=1),
            Scenario(x_in=29807, x_valid=1, expected_y_out=17179781653, expected_y_valid=1),
            Scenario(x_in=28553, expected_y_out=17179863203, expected_y_valid=1),
            Scenario(x_in=12806, x_valid=1, expected_y_out=17179863203),
            Scenario(x_in=43721, x_valid=1, expected_y_out=181484, expected_y_valid=1),
            Scenario(x_in=56608, x_valid=1, expected_y_out=199682, expected_y_valid=1),
            Scenario(x_in=57386, x_valid=1, expected_y_out=105088, expected_y_valid=1),
            Scenario(x_in=19363, x_valid=1, expected_y_out=17179828957, expected_y_valid=1),
            Scenario(x_in=22844, x_valid=1, expected_y_out=17179758203, expected_y_valid=1),
            Scenario(x_in=11362, expected_y_out=1408, expected_y_valid=1),
            Scenario(x_in=43463, x_valid=1, expected_y_out=1408),
            Scenario(x_in=4671, x_valid=1, expected_y_out=49104, expected_y_valid=1),
            Scenario(x_in=57393, x_valid=1, expected_y_out=106509, expected_y_valid=1),
            Scenario(x_in=34357, x_valid=1, expected_y_out=26356, expected_y_valid=1),
            Scenario(x_in=58816, x_valid=1, expected_y_out=17179747440, expected_y_valid=1),
            Scenario(x_in=25908, x_valid=1, expected_y_out=17179794361, expected_y_valid=1),
            Scenario(x_in=65135, expected_y_out=17179755543, expected_y_valid=1),
            Scenario(x_in=26355, x_valid=1, expected_y_out=17179755543),
            Scenario(x_in=37745, x_valid=1, expected_y_out=17179802479, expected_y_valid=1),
            Scenario(x_in=11555, x_valid=1, expected_y_out=75763, expected_y_valid=1),
            Scenario(x_in=37019, x_valid=1, expected_y_out=138670, expected_y_valid=1),
            Scenario(x_in=14000, x_valid=1, expected_y_out=16640, expected_y_valid=1),
            Scenario(x_in=60485, x_valid=1, expected_y_out=17179749651, expected_y_valid=1),
            Scenario(x_in=39150, expected_y_out=17179852802, expected_y_valid=1),
            Scenario(x_in=17002, x_valid=1, expected_y_out=17179852802),
            Scenario(x_in=226, x_valid=1, expected_y_out=17179804016, expected_y_valid=1),
            Scenario(x_in=3849, x_valid=1, expected_y_out=75077, expected_y_valid=1),
            Scenario(x_in=18694, x_valid=1, expected_y_out=35103, expected_y_valid=1),
            Scenario(x_in=55575, x_valid=1, expected_y_out=95078, expected_y_valid=1),
            Scenario(x_in=29762, x_valid=1, expected_y_out=39878, expected_y_valid=1),
            Scenario(x_in=54915, expected_y_out=81318, expected_y_valid=1),
            Scenario(x_in=11496, x_valid=1, expected_y_out=81318),
            Scenario(x_in=2840, x_valid=1, expected_y_out=115913, expected_y_valid=1),
            Scenario(x_in=36595, x_valid=1, expected_y_out=75274, expected_y_valid=1),
            Scenario(x_in=61252, x_valid=1, expected_y_out=130275, expected_y_valid=1),
            Scenario(x_in=5859, x_valid=1, expected_y_out=17179861522, expected_y_valid=1),
            Scenario(x_in=6414, x_valid=1, expected_y_out=17179791012, expected_y_valid=1),
            Scenario(x_in=39650, expected_y_out=17179758700, expected_y_valid=1),
            Scenario(x_in=32956, x_valid=1, expected_y_out=17179758700),
            Scenario(x_in=18972, x_valid=1, expected_y_out=17179849873, expected_y_valid=1),
            Scenario(x_in=39931, x_valid=1, expected_y_out=17179865674, expected_y_valid=1),
            Scenario(x_in=24174, x_valid=1, expected_y_out=17179809439, expected_y_valid=1),
            Scenario(x_in=64908, x_valid=1, expected_y_out=17179768744, expected_y_valid=1),
            Scenario(x_in=34517, x_valid=1, expected_y_out=46793, expected_y_valid=1),
            Scenario(x_in=15545, expected_y_out=17179807011, expected_y_valid=1),
            Scenario(x_in=42314, x_valid=1, expected_y_out=17179807011),
            Scenario(x_in=61217, x_valid=1, expected_y_out=9552, expected_y_valid=1),
            Scenario(x_in=22763, x_valid=1, expected_y_out=17179722852, expected_y_valid=1),
            Scenario(x_in=18600, x_valid=1, expected_y_out=17179689567, expected_y_valid=1),
            Scenario(x_in=34208, x_valid=1, expected_y_out=17179827465, expected_y_valid=1),
            Scenario(x_in=53375, x_valid=1, expected_y_out=56885, expected_y_valid=1),
            Scenario(x_in=44191, expected_y_out=72035, expected_y_valid=1),
            Scenario(x_in=44191, expected_y_out=72035),
        ),
    )
    # fmt: on
    position = phase if phase < 177 else 176
    scenario = scenarios.rows[position]
    x_in = scenario.x_in
    x_valid = scenario.x_valid
    dut = DigitalFilter(x_in, x_valid)

    expected_y_out = scenario.expected_y_out
    expected_y_valid = scenario.expected_y_valid

    @rule
    def check_and_advance():
        if phase < 177:
            assert dut.y_out == expected_y_out, "digital_filter: y_out"
            assert dut.y_valid == expected_y_valid, "digital_filter: y_valid"

        log("info", "digital_filter.y_out", dut.y_out)
        log("info", "digital_filter.y_valid", dut.y_valid)

    check_and_advance()
    advance(phase)

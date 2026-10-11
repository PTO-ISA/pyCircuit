"""Regular-clock struct_transform scenario; original physical-control oracles remain."""

from example_struct_transform.struct_transform import StructTransform
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    op: bits[4]
    dst: bits[6]
    word: bits[32]
    valid: bits[1]
    expected_op: bits[4]
    expected_dst: bits[6]
    expected_word: bits[32]
    expected_valid: bits[1]


@struct
class Scenarios:
    rows: table[72, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseStructTransform():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(op=1, dst=2, word=3, valid=1, expected_word=1),
            Scenario(op=15, dst=63, word=4294967295, expected_op=1, expected_dst=2, expected_word=5, expected_valid=1),
            Scenario(op=1, dst=2, word=3, valid=1, expected_op=15, expected_dst=63, expected_word=15),
            Scenario(op=15, dst=63, word=4294967295, expected_op=1, expected_dst=2, expected_word=5, expected_valid=1),
            Scenario(op=15, dst=63, word=4294967295, expected_op=15, expected_dst=63, expected_word=15),
            Scenario(op=1, dst=2, word=3, valid=1, expected_op=15, expected_dst=63, expected_word=15),
            Scenario(op=9, dst=31, word=4294967280, expected_op=1, expected_dst=2, expected_word=5, expected_valid=1),
            Scenario(op=13, dst=13, word=4294967295, valid=1, expected_op=9, expected_dst=31, expected_word=4294967290),
            Scenario(op=13, dst=4, expected_op=13, expected_dst=13, expected_word=13, expected_valid=1),
            Scenario(op=13, dst=62, word=3754036845, valid=1, expected_op=13, expected_dst=4, expected_word=14),
            Scenario(op=5, dst=29, word=1403919592, expected_op=13, expected_dst=62, expected_word=3754036859, expected_valid=1),
            Scenario(op=10, dst=51, word=2926762791, valid=1, expected_op=5, expected_dst=29, expected_word=1403919598),
            Scenario(op=4, dst=19, word=1113890394, expected_op=10, expected_dst=51, expected_word=2926762802, expected_valid=1),
            Scenario(op=2, dst=11, word=694999537, valid=1, expected_op=4, expected_dst=19, expected_word=1113890399),
            Scenario(op=15, dst=50, word=4266986140, expected_op=2, expected_dst=11, expected_word=694999540, expected_valid=1),
            Scenario(op=1, dst=17, word=305571147, valid=1, expected_op=15, expected_dst=50, expected_word=4266986156),
            Scenario(op=1, dst=27, word=325335598, expected_op=1, expected_dst=17, expected_word=305571149, expected_valid=1),
            Scenario(op=13, dst=49, word=3593616309, valid=1, expected_op=1, expected_dst=27, expected_word=325335600),
            Scenario(op=15, dst=43, word=4117961104, expected_op=13, expected_dst=49, expected_word=3593616323, expected_valid=1),
            Scenario(dst=52, word=243703727, valid=1, expected_op=15, expected_dst=43, expected_word=4117961120),
            Scenario(op=3, dst=39, word=888916290, expected_dst=52, expected_word=243703728, expected_valid=1),
            Scenario(op=14, dst=54, word=3873077177, valid=1, expected_op=3, expected_dst=39, expected_word=888916294),
            Scenario(op=11, dst=18, word=2991308228, expected_op=14, expected_dst=54, expected_word=3873077192, expected_valid=1),
            Scenario(word=503379, valid=1, expected_op=11, expected_dst=18, expected_word=2991308240),
            Scenario(op=5, dst=19, word=1382211478, expected_word=503380, expected_valid=1),
            Scenario(op=10, dst=49, word=2788168189, valid=1, expected_op=5, expected_dst=19, expected_word=1382211484),
            Scenario(op=9, dst=45, word=2512366392, expected_op=10, expected_dst=49, expected_word=2788168200, expected_valid=1),
            Scenario(op=14, dst=46, word=3990549815, valid=1, expected_op=9, expected_dst=45, expected_word=2512366402),
            Scenario(op=8, dst=5, word=2157990186, expected_op=14, expected_dst=46, expected_word=3990549830, expected_valid=1),
            Scenario(op=9, dst=16, word=2449723009, valid=1, expected_op=8, expected_dst=5, expected_word=2157990195),
            Scenario(op=5, dst=44, word=1434506732, expected_op=9, expected_dst=16, expected_word=2449723019, expected_valid=1),
            Scenario(op=12, dst=42, word=3443644507, valid=1, expected_op=5, expected_dst=44, expected_word=1434506738),
            Scenario(dst=46, word=98447870, expected_op=12, expected_dst=42, expected_word=3443644520, expected_valid=1),
            Scenario(op=15, dst=19, word=4067471685, valid=1, expected_dst=46, expected_word=98447871),
            Scenario(op=12, dst=20, word=3263590880, expected_op=15, expected_dst=19, expected_word=4067471701, expected_valid=1),
            Scenario(op=12, dst=44, word=3447847871, valid=1, expected_op=12, expected_dst=20, expected_word=3263590893),
            Scenario(dst=31, word=201184786, expected_op=12, expected_dst=44, expected_word=3447847884, expected_valid=1),
            Scenario(op=14, dst=26, word=3814719049, valid=1, expected_dst=31, expected_word=201184787),
            Scenario(op=13, dst=38, word=3703632660, expected_op=14, expected_dst=26, expected_word=3814719064, expected_valid=1),
            Scenario(op=1, word=269042531, valid=1, expected_op=13, expected_dst=38, expected_word=3703632674),
            Scenario(op=5, dst=19, word=1382797670, expected_op=1, expected_word=269042533, expected_valid=1),
            Scenario(op=13, dst=34, word=3561830797, valid=1, expected_op=5, expected_dst=19, expected_word=1382797676),
            Scenario(op=6, word=1745751432, expected_op=13, expected_dst=34, expected_word=3561830811, expected_valid=1),
            Scenario(op=7, dst=6, word=1892799303, valid=1, expected_op=6, expected_word=1745751439),
            Scenario(op=3, dst=25, word=859043834, expected_op=7, expected_dst=6, expected_word=1892799311, expected_valid=1),
            Scenario(op=12, dst=18, word=3259639569, valid=1, expected_op=3, expected_dst=25, expected_word=859043838),
            Scenario(op=7, dst=51, word=1986835772, expected_op=12, expected_dst=18, expected_word=3259639582, expected_valid=1),
            Scenario(op=4, dst=24, word=1124470635, valid=1, expected_op=7, expected_dst=51, expected_word=1986835780),
            Scenario(op=8, dst=58, word=2404736462, expected_op=4, expected_dst=24, expected_word=1124470640, expected_valid=1),
            Scenario(op=11, dst=2, word=2957167317, valid=1, expected_op=8, expected_dst=58, expected_word=2404736471),
            Scenario(op=9, dst=27, word=2607881776, expected_op=11, expected_dst=2, expected_word=2957167329, expected_valid=1),
            Scenario(op=2, dst=47, word=635739087, valid=1, expected_op=9, expected_dst=27, expected_word=2607881786),
            Scenario(op=7, dst=50, word=1985369826, expected_op=2, expected_dst=47, expected_word=635739090, expected_valid=1),
            Scenario(op=2, dst=12, word=562129113, valid=1, expected_op=7, expected_dst=50, expected_word=1985369834),
            Scenario(op=11, dst=39, word=3170417764, expected_op=2, expected_dst=12, expected_word=562129116, expected_valid=1),
            Scenario(op=5, dst=47, word=1441029235, valid=1, expected_op=11, expected_dst=39, expected_word=3170417776),
            Scenario(op=9, dst=40, word=2635626294, expected_op=5, expected_dst=47, expected_word=1441029241, expected_valid=1),
            Scenario(op=9, dst=19, word=2591196445, valid=1, expected_op=9, expected_dst=40, expected_word=2635626304),
            Scenario(op=11, dst=40, word=3038659544, expected_op=9, expected_dst=19, expected_word=2591196455, expected_valid=1),
            Scenario(op=1, dst=48, word=504919383, valid=1, expected_op=11, expected_dst=40, expected_word=3038659556),
            Scenario(op=15, dst=63, word=4159476426, expected_op=1, expected_dst=48, expected_word=504919385, expected_valid=1),
            Scenario(op=6, dst=29, word=1806230433, valid=1, expected_op=15, expected_dst=63, expected_word=4159476442),
            Scenario(op=3, dst=11, word=963487884, expected_op=6, expected_dst=29, expected_word=1806230440, expected_valid=1),
            Scenario(op=4, dst=46, word=1305758331, valid=1, expected_op=3, expected_dst=11, expected_word=963487888),
            Scenario(dst=31, word=199671198, expected_op=4, expected_dst=46, expected_word=1305758336, expected_valid=1),
            Scenario(op=4, dst=25, word=1260488805, valid=1, expected_dst=31, expected_word=199671199),
            Scenario(op=11, dst=26, word=3143114368, expected_op=4, expected_dst=25, expected_word=1260488810, expected_valid=1),
            Scenario(op=13, dst=52, word=3599728607, valid=1, expected_op=11, expected_dst=26, expected_word=3143114380),
            Scenario(op=12, dst=27, word=3413265330, expected_op=13, expected_dst=52, expected_word=3599728621, expected_valid=1),
            Scenario(op=7, dst=62, word=2143795049, valid=1, expected_op=12, expected_dst=27, expected_word=3413265343),
            Scenario(op=1, dst=2, word=3, valid=1, expected_op=7, expected_dst=62, expected_word=2143795057, expected_valid=1),
            Scenario(op=1, dst=2, word=3, valid=1, expected_op=1, expected_dst=2, expected_word=5, expected_valid=1),
        ),
    )
    # fmt: on
    position = phase if phase < 72 else 71
    scenario = scenarios.rows[position]
    dut = StructTransform(scenario.op, scenario.dst, scenario.word, scenario.valid)

    @rule
    def check_and_advance():
        if phase < 72:
            assert dut.op == scenario.expected_op, "struct_transform: op"
            assert dut.dst == scenario.expected_dst, "struct_transform: dst"
            assert dut.word == scenario.expected_word, "struct_transform: word"
            assert dut.valid == scenario.expected_valid, "struct_transform: valid"

        log("info", "struct_transform.op", dut.op)
        log("info", "struct_transform.dst", dut.dst)
        log("info", "struct_transform.word", dut.word)
        log("info", "struct_transform.valid", dut.valid)

    check_and_advance()
    advance(phase)

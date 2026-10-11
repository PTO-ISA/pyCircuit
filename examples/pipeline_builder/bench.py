"""Regular-clock pipeline_builder scenario; original physical-control oracles remain."""

from example_pipeline_builder.pipeline_builder import PipelineBuilder
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    word: bits[32]
    valid: bits[1]
    expected_word: bits[32]
    expected_valid: bits[1]


@struct
class Scenarios:
    rows: table[72, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExercisePipelineBuilder():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(word=5, valid=1),
            Scenario(word=4294967295, expected_word=1),
            Scenario(word=5, valid=1, expected_word=6, expected_valid=1),
            Scenario(word=4294967295),
            Scenario(word=4294967295, expected_word=6, expected_valid=1),
            Scenario(word=5, valid=1),
            Scenario(word=4294967280),
            Scenario(word=4294967295, valid=1, expected_word=6, expected_valid=1),
            Scenario(expected_word=4294967281),
            Scenario(word=3754036845, valid=1, expected_valid=1),
            Scenario(word=1403919592, expected_word=1),
            Scenario(word=2926762791, valid=1, expected_word=3754036846, expected_valid=1),
            Scenario(word=1113890394, expected_word=1403919593),
            Scenario(word=694999537, valid=1, expected_word=2926762792, expected_valid=1),
            Scenario(word=4266986140, expected_word=1113890395),
            Scenario(word=305571147, valid=1, expected_word=694999538, expected_valid=1),
            Scenario(word=325335598, expected_word=4266986141),
            Scenario(word=3593616309, valid=1, expected_word=305571148, expected_valid=1),
            Scenario(word=4117961104, expected_word=325335599),
            Scenario(word=243703727, valid=1, expected_word=3593616310, expected_valid=1),
            Scenario(word=888916290, expected_word=4117961105),
            Scenario(word=3873077177, valid=1, expected_word=243703728, expected_valid=1),
            Scenario(word=2991308228, expected_word=888916291),
            Scenario(word=503379, valid=1, expected_word=3873077178, expected_valid=1),
            Scenario(word=1382211478, expected_word=2991308229),
            Scenario(word=2788168189, valid=1, expected_word=503380, expected_valid=1),
            Scenario(word=2512366392, expected_word=1382211479),
            Scenario(word=3990549815, valid=1, expected_word=2788168190, expected_valid=1),
            Scenario(word=2157990186, expected_word=2512366393),
            Scenario(word=2449723009, valid=1, expected_word=3990549816, expected_valid=1),
            Scenario(word=1434506732, expected_word=2157990187),
            Scenario(word=3443644507, valid=1, expected_word=2449723010, expected_valid=1),
            Scenario(word=98447870, expected_word=1434506733),
            Scenario(word=4067471685, valid=1, expected_word=3443644508, expected_valid=1),
            Scenario(word=3263590880, expected_word=98447871),
            Scenario(word=3447847871, valid=1, expected_word=4067471686, expected_valid=1),
            Scenario(word=201184786, expected_word=3263590881),
            Scenario(word=3814719049, valid=1, expected_word=3447847872, expected_valid=1),
            Scenario(word=3703632660, expected_word=201184787),
            Scenario(word=269042531, valid=1, expected_word=3814719050, expected_valid=1),
            Scenario(word=1382797670, expected_word=3703632661),
            Scenario(word=3561830797, valid=1, expected_word=269042532, expected_valid=1),
            Scenario(word=1745751432, expected_word=1382797671),
            Scenario(word=1892799303, valid=1, expected_word=3561830798, expected_valid=1),
            Scenario(word=859043834, expected_word=1745751433),
            Scenario(word=3259639569, valid=1, expected_word=1892799304, expected_valid=1),
            Scenario(word=1986835772, expected_word=859043835),
            Scenario(word=1124470635, valid=1, expected_word=3259639570, expected_valid=1),
            Scenario(word=2404736462, expected_word=1986835773),
            Scenario(word=2957167317, valid=1, expected_word=1124470636, expected_valid=1),
            Scenario(word=2607881776, expected_word=2404736463),
            Scenario(word=635739087, valid=1, expected_word=2957167318, expected_valid=1),
            Scenario(word=1985369826, expected_word=2607881777),
            Scenario(word=562129113, valid=1, expected_word=635739088, expected_valid=1),
            Scenario(word=3170417764, expected_word=1985369827),
            Scenario(word=1441029235, valid=1, expected_word=562129114, expected_valid=1),
            Scenario(word=2635626294, expected_word=3170417765),
            Scenario(word=2591196445, valid=1, expected_word=1441029236, expected_valid=1),
            Scenario(word=3038659544, expected_word=2635626295),
            Scenario(word=504919383, valid=1, expected_word=2591196446, expected_valid=1),
            Scenario(word=4159476426, expected_word=3038659545),
            Scenario(word=1806230433, valid=1, expected_word=504919384, expected_valid=1),
            Scenario(word=963487884, expected_word=4159476427),
            Scenario(word=1305758331, valid=1, expected_word=1806230434, expected_valid=1),
            Scenario(word=199671198, expected_word=963487885),
            Scenario(word=1260488805, valid=1, expected_word=1305758332, expected_valid=1),
            Scenario(word=3143114368, expected_word=199671199),
            Scenario(word=3599728607, valid=1, expected_word=1260488806, expected_valid=1),
            Scenario(word=3413265330, expected_word=3143114369),
            Scenario(word=2143795049, valid=1, expected_word=3599728608, expected_valid=1),
            Scenario(word=5, valid=1, expected_word=3413265331),
            Scenario(word=5, valid=1, expected_word=2143795050, expected_valid=1),
        ),
    )
    # fmt: on
    position = phase if phase < 72 else 71
    scenario = scenarios.rows[position]
    word = scenario.word
    valid = scenario.valid
    dut = PipelineBuilder(word, valid)

    expected_word = scenario.expected_word
    expected_valid = scenario.expected_valid

    @rule
    def check_and_advance():
        if phase < 72:
            assert dut.word == expected_word, "pipeline_builder: word"
            assert dut.valid == expected_valid, "pipeline_builder: valid"

        log("info", "pipeline_builder.word", dut.word)
        log("info", "pipeline_builder.valid", dut.valid)

    check_and_advance()
    advance(phase)

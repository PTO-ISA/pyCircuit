"""Eight literal input/output cases for the generated multi-output System."""

import pycircuit as ac
from example_multi_outputs.multi_outputs import GroupedOutputs, MultiOutputs


@ac.struct
class Scenario:
    word: ac.u8
    enable: ac.u1
    expected_tag: ac.u4
    expected_data: ac.u8
    expected_valid: ac.u1


@ac.struct
class Scenarios:
    rows: ac.table[8, Scenario] = (
        Scenario(word=0, enable=0, expected_tag=1, expected_data=255, expected_valid=0),
        Scenario(word=1, enable=1, expected_tag=2, expected_data=254, expected_valid=1),
        Scenario(
            word=15, enable=1, expected_tag=0, expected_data=240, expected_valid=1
        ),
        Scenario(
            word=16, enable=0, expected_tag=1, expected_data=239, expected_valid=0
        ),
        Scenario(
            word=127, enable=1, expected_tag=0, expected_data=128, expected_valid=1
        ),
        Scenario(
            word=128, enable=0, expected_tag=1, expected_data=127, expected_valid=0
        ),
        Scenario(
            word=254, enable=1, expected_tag=15, expected_data=1, expected_valid=1
        ),
        Scenario(word=255, enable=0, expected_tag=0, expected_data=0, expected_valid=0),
    )


@ac.rule
def advance(phase):
    phase = phase + 1


@ac.system
def MultiOutputsSystem():  # noqa: N802
    phase: ac.u3 = 0
    scenarios = Scenarios()
    row = scenarios.rows[phase]
    tag, data, valid = MultiOutputs(row.word, row.enable)
    grouped_tag, grouped_data, grouped_valid = GroupedOutputs(row.word, row.enable)

    @ac.rule
    def check():
        assert tag == row.expected_tag, "multi_outputs: header consumer"
        assert data == row.expected_data, "multi_outputs: payload consumer"
        assert valid == row.expected_valid, "multi_outputs: independent valid output"
        assert grouped_tag == row.expected_tag, "multi_outputs: grouped header"
        assert grouped_data == row.expected_data, "multi_outputs: grouped payload"
        assert grouped_valid == row.expected_valid, "multi_outputs: grouped valid"
        ac.log("info", "multi_outputs", tag, data, valid)

    check()
    advance(phase)

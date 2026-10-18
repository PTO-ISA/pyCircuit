"""Replace six entries, stall one response, and check the returned old values."""

# ruff: noqa: F823 -- captured System-owned state proposals.
import pycircuit as ac
from example_table_rule.table_rule import Entry, TableRule


@ac.system
def ExerciseTableRule():  # noqa: N802
    phase: ac.u7 = 0
    completed: ac.u4 = 0
    sending = (phase >= 1 and phase <= 4) or phase == 9 or phase == 10
    request = Entry(
        index=1 if phase == 3 or phase == 10 else 0,
        value=50 if phase == 9 else 60 if phase == 10 else phase * 10,
    )
    take = phase != 4
    result = TableRule(1 if sending else 0, request, 1 if take else 0)

    @ac.rule
    def check_and_advance() -> None:
        expected_response = (phase >= 3 and phase <= 7) or phase == 11 or phase == 12
        # The first access to each slot returns its zero-initialized Entry.
        # At phase 4 the second response stalls and must remain visible at 5.
        expected = Entry(index=0, value=0)
        if phase == 4 or phase == 5:
            expected.value = 10
        elif phase == 7:
            expected.value = 20
        elif phase == 11:
            expected.value = 40
        elif phase == 12:
            expected.index = 1
            expected.value = 30
        if expected_response:
            assert result.valid == 1, "table response must arrive"
            assert result.data.index == expected.index, "old entry index"
            assert result.data.value == expected.value, "old entry value"
        else:
            assert result.valid == 0, "unexpected table response"
        if sending:
            assert result.ready == 1, "scheduled request must be accepted"
        if result.valid and take:
            completed = completed + 1
        if phase == 15:
            assert completed == 6, "all six replacements must complete"
        ac.log("info", "table replacement", phase, result.valid, result.data.value)
        phase = phase + 1

    check_and_advance()

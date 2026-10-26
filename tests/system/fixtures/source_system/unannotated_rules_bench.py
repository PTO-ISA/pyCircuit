import pycircuit as ac
from checks.unannotated_rules_provider import (
    GlobalCounter,
    Iterations,
    LocalCounter,
    Widths,
)


@ac.system
def ExerciseUnannotatedRules():
    local = LocalCounter()
    global_value = GlobalCounter()
    widths = Widths(3, 250)
    iteration = Iterations(1)

    @ac.rule
    def check():
        assert widths.small == 4
        assert widths.large == 251
        assert iteration.value == 7
        ac.log(
            "info",
            "unannotated values",
            local.value,
            global_value.value,
            widths.small,
            widths.large,
            iteration.value,
        )

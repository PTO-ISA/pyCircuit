import pycircuit as ac
from checks.method_conditional_provider import ConditionalCounter


@ac.system
def ExerciseConditionalMethods():
    left = ConditionalCounter()
    right = ConditionalCounter()

    @ac.rule
    def observe_before() -> None:
        ac.log("info", "conditional old before", left.read(), right.read())

    @ac.rule
    def drive() -> None:
        left_result = left.sequence(1)
        right_result = right.sequence(0)
        ac.log("info", "conditional results", left_result, right_result)

    @ac.rule
    def observe_after() -> None:
        ac.log("info", "conditional old after", left.read(), right.read())

    observe_before()
    drive()
    observe_after()

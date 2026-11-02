import pycircuit as ac
from checks.method_conditional_provider import ConditionalCounter


@ac.system
def ExerciseConditionalMethods():
    left = ConditionalCounter()
    right = ConditionalCounter()

    @ac.autorule
    def observe_before() -> None:
        ac.log("info", "conditional old before", left.read(), right.read())

    @ac.autorule
    def drive() -> None:
        left_result = left.sequence(1)
        right_result = right.sequence(0)
        ac.log("info", "conditional results", left_result, right_result)

    @ac.autorule
    def observe_after() -> None:
        ac.log("info", "conditional old after", left.read(), right.read())

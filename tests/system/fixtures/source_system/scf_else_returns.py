# ruff: noqa: B007, F821, F823, F841 -- captured dead names and state proposals.
import pycircuit as ac


@ac.rule
def zero_return(value: ac.u8):
    for index in range(0):
        value = missing_dead_value
    else:
        return value + 3


@ac.rule
def zero_break_return(value: ac.u8):
    for index in range(0):
        break
    else:
        return value + 5


@ac.rule
def natural_return(value: ac.u8):
    for index in range(2):
        value = value + 2
    else:
        return value + 7


@ac.rule
def continue_return(value: ac.u8):
    for index in range(3):
        value = value + 3
        continue
        value = value + 100
    else:
        return value + 9


@ac.rule
def while_return(value: ac.u8, limit: ac.u2):
    cursor: ac.u2 = 0
    while cursor < limit:
        cursor = cursor + 1
        increment: ac.u8 = cursor
        value = value + increment
    else:
        return value + 13


@ac.rule
def tuple_return(value: ac.u8):
    for index in range(1):
        pass
    else:
        return value + 17, value + 19


@ac.rule
def annotated_return(value: ac.u8) -> ac.u8:
    for index in range(0):
        pass
    else:
        return value + 23


@ac.rule
def direct_return(value: ac.u8):
    return value + 29


@ac.rule
def bare_return(value: ac.u8):
    for index in range(0):
        pass
    else:
        ac.log("info", "bare else", value)
        return


@ac.rule
def none_return(value: ac.u8):
    for index in range(1):
        pass
    else:
        ac.log("info", "none else", value)
        return None


@ac.system
def ExerciseElseReturns():
    seed: ac.u8 = 250
    phase: ac.u2 = 0

    @ac.rule
    def advance() -> None:
        incoming = seed
        zero = zero_return(incoming)
        dead_break = zero_break_return(incoming)
        natural = natural_return(incoming)
        continued = continue_return(incoming)
        bounded = while_return(incoming, phase)
        left, right = tuple_return(incoming)
        annotated = annotated_return(incoming)
        direct = direct_return(incoming)
        bare_return(incoming)
        none_return(incoming)
        ac.log(
            "info",
            "else returns",
            incoming,
            zero,
            dead_break,
            natural,
            continued,
            bounded,
            left,
            right,
            annotated,
            direct,
        )
        seed = seed + 3
        phase = phase + 1

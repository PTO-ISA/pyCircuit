# ruff: noqa: F821, F823, F841, F842, N802 -- Compiler-owned bindings and state.
import pycircuit as ac


@ac.module
def Export(source: ac.u8) -> ac.u8:
    alias = source
    return alias


@ac.module
def Relay(source: ac.u8) -> ac.u8:
    result = Export(source)
    return result


@ac.module
def Pair(source: ac.u8) -> tuple[ac.u8, ac.u8]:
    @ac.rule
    def split(value: ac.u8) -> tuple[ac.u8, ac.u8]:
        return value + 10, value + 20

    first, second = split(source)
    return first, second


@ac.system
def Mixed():
    cycle: ac.u8 = 0
    source: ac.u8 = 1
    stored: ac.u8 = 100
    first = ac.queue[ac.u8](depth=2)
    second = ac.queue[ac.u8](depth=2)
    stored, first = Pair(source)
    other, second = Pair(source)
    alias = Relay(stored)
    another = Relay(alias)

    @ac.rule(guard=lambda: cycle >= 6)
    def consume_first(value: ac.u8) -> None:
        ac.log("info", "first", value)

    @ac.rule
    def consume_other(value: ac.u8) -> None:
        ac.log("info", "other", value)

    @ac.rule
    def consume_second(value: ac.u8) -> None:
        ac.log("info", "second", value)

    consume_first(first)
    consume_other(other)
    consume_second(second)

    @ac.autorule
    def tick() -> None:
        ac.log("info", "mixed", cycle, source, stored, alias, another)
        cycle = cycle + 1
        source = source + 1


@ac.module
def Independent(left: ac.u8, right: ac.u8) -> tuple[ac.u8, ac.u8]:
    @ac.rule
    def first(value: ac.u8) -> ac.u8:
        ac.log("info", "first_fire", value)
        return value + 10

    @ac.rule
    def second(value: ac.u8) -> ac.u8:
        ac.log("info", "second_fire", value)
        return value + 20

    out_left = first(left)
    out_right = second(right)
    return out_left, out_right


@ac.system
def Separate():
    cycle: ac.u8 = 0
    sent_left: ac.u8 = 0
    sent_right: ac.u8 = 0

    @ac.rule(guard=lambda: sent_left < 4)
    def produce_left() -> ac.u8:
        value = sent_left + 1
        sent_left = sent_left + 1
        return value

    @ac.rule(guard=lambda: sent_right < 4)
    def produce_right() -> ac.u8:
        value = sent_right + 1
        sent_right = sent_right + 1
        return value

    input_left = produce_left()
    input_right = produce_right()
    left, right = Independent(input_left, input_right)

    @ac.rule(guard=lambda: cycle >= 10)
    def consume_left(value: ac.u8) -> None:
        ac.log("info", "left", value)

    @ac.rule
    def consume_right(value: ac.u8) -> None:
        ac.log("info", "right", value)

    consume_left(left)
    consume_right(right)

    @ac.autorule
    def tick() -> None:
        ac.log("info", "progress", cycle, sent_left, sent_right)
        cycle = cycle + 1


@ac.system
def Conflict():
    source: ac.u8 = 1
    queue = ac.queue[ac.u8](depth=2)
    alias = Relay(queue)
    queue, alias = Pair(source)

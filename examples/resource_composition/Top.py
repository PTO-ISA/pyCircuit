# ruff: noqa: F821, F823, F842 -- Module bindings and state are compiler-owned.
import pycircuit as ac
from example_resource_composition.counter import Counter
from example_resource_composition.observer import Observer
from example_resource_composition.relay import Relay


@ac.system
def Top():  # noqa: N802
    shared: Counter
    independent: Counter
    observer: Observer
    sent: ac.u8 = 0
    left_received: ac.u8 = 0
    right_received: ac.u8 = 0
    cycle: ac.u8 = 0

    @ac.rule(guard=lambda: sent < 4)
    def produce() -> ac.u8:
        value = sent + 1
        sent = sent + 1
        ac.log("info", "send", value)
        return value

    incoming = produce()
    left, right = Relay(incoming, shared)
    observer(shared, independent)

    @ac.rule
    def consume_left(value: ac.u8) -> None:
        assert left_received < 4
        assert value == left_received + 11
        left_received = left_received + 1
        ac.log("info", "left", value)

    # A full right output blocks the entire SplitStage transaction, including
    # the shared Counter update and the left output enqueue.
    @ac.rule(guard=lambda: cycle >= 10)
    def consume_right(value: ac.u8) -> None:
        assert right_received < 4
        assert value == right_received + 21
        right_received = right_received + 1
        ac.log("info", "right", value)

    consume_left(left)
    consume_right(right)

    @ac.autorule
    def tick() -> None:
        cycle = cycle + 1
        ac.log("info", "progress", cycle, sent, left_received, right_received)

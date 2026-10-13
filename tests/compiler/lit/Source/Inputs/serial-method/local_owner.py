# ruff: noqa: F823, F841 -- hardware owner writes and retained snapshot/capture probes.
from pycircuit import method, module, rule, u7


@module
def LocalOwner():
    count: u7 = 5
    before: u7 = 0
    middle: u7 = 0
    saved: u7 = 0
    after: u7 = 0
    old_seen: u7 = 0
    payload: u7 = 9
    copied: u7 = 0
    held: u7 = 9
    identity: u7 = 11
    bias = 3

    @method
    def read() -> u7:
        return count

    @method(guard=lambda delta: True)
    def add(delta: u7) -> u7:
        count = count + delta
        return count

    @method
    def chain(delta: u7) -> u7:
        first = add(delta)
        ignored = add(bias)
        return first

    @method
    def copy_payload() -> u7:
        return payload

    @method(guard=lambda allowed: allowed)
    def uncalled(allowed: bool) -> None:
        count = count

    @rule
    def step() -> None:
        bias = 11
        first = read()
        next_value = add(1)
        nested_saved = chain(2)
        final_value = add(4)
        before = first
        middle = next_value
        saved = nested_saved
        after = final_value
        copied = copy_payload()
        identity = identity

    @rule
    def inspect() -> None:
        old_seen = read()

    step()
    inspect()

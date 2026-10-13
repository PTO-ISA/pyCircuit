# ruff: noqa: F823, F841 -- hardware owner writes and lexical snapshot probes.
from pycircuit import method, module, rule, u7


@module
def Counter():
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
    old_snapshot = count + 0

    @method
    def read() -> u7:
        return count

    @method
    def snapshot() -> u7:
        return old_snapshot

    @method
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

    @method
    def record(a: u7, b: u7, c: u7, d: u7, p: u7) -> None:
        before = a
        middle = b
        saved = c
        after = d
        copied = p
        identity = identity

    @rule
    def inspect() -> None:
        old_seen = read()

    inspect()

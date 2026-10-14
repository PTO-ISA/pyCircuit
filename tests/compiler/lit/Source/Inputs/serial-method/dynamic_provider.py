# ruff: noqa: F823, F841 -- hardware state writes and retained probe owners.
from pycircuit import method, module, rule, u1, u7


@module
def AutonomousOwner():
    count: u7 = 5
    before: u7 = 0
    after: u7 = 0
    old_seen: u7 = 0
    first_gate: u1 = 1
    second_gate: u1 = 1

    @method(guard=lambda: second_gate)
    def commit() -> None:
        count = count + 1

    @method(guard=lambda: first_gate)
    def advance() -> None:
        save = count
        commit()
        before = save
        after = count

    @rule
    def observe() -> None:
        old_seen = count

    @rule
    def step() -> None:
        advance()

    observe()
    step()


@module
def ManualOwner():
    count: u7 = 5
    before: u7 = 0
    after: u7 = 0
    old_seen: u7 = 0
    first_gate: u1 = 1
    second_gate: u1 = 1

    @method(guard=lambda: second_gate)
    def commit() -> None:
        count = count + 1

    @method(guard=lambda: first_gate)
    def advance() -> None:
        save = count
        commit()
        before = save
        after = count

    @rule
    def observe() -> None:
        old_seen = count

    observe()


@module
def CandidateEntryOwner():
    count: u7 = 5
    before: u7 = 0
    after: u7 = 0
    old_seen: u7 = 0
    first_gate: u1 = 1
    second_gate: u1 = 1

    @method(guard=lambda: first_gate)
    def advance() -> None:
        save = count
        count = count + 1
        before = save
        after = count

    @rule
    def observe() -> None:
        old_seen = count

    @rule
    def step() -> None:
        first_gate = second_gate
        advance()
        first_gate = 0

    observe()
    step()

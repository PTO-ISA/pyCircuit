# ruff: noqa: F823, F841 -- captured state and formal proposals.
import pycircuit as ac


@ac.module
def AliasOwner():
    count: ac.u8 = 9

    @ac.rule
    def read() -> ac.u8:
        return count

    @ac.rule
    def revise(value: ac.u8) -> tuple[ac.u8, ac.u8, ac.u8]:
        saved = value
        value = value + 2
        middle = read()
        value = value + 5
        return saved, middle, count

    @ac.rule
    def forward(value: ac.u8) -> tuple[ac.u8, ac.u8, ac.u8]:
        saved, middle, final = revise(value)
        return saved, middle, final

    @ac.rule(guard=lambda enabled: enabled)
    def advance(enabled: ac.u1) -> None:
        saved, middle, final = forward(count)
        local: ac.u8 = 13
        value_saved, value_middle, value_final = forward(local)
        ac.log("info", "formal alias", saved, middle, final, count)
        ac.log("info", "formal value", local, value_saved, value_middle, value_final)

    @ac.rule
    def inspect() -> None:
        ac.log("info", "formal snapshot", count)


@ac.module
def FormalCalculator():
    @ac.rule
    def revise(value: ac.u8, enabled: ac.u1, identity: ac.u1) -> ac.u8:
        saved = value
        if enabled:
            if identity:
                value = value
            else:
                value = value + 3
        return saved

    @ac.rule
    def increment(value: ac.u8) -> None:
        value = value + 1

    @ac.rule
    def forward(value: ac.u8) -> ac.u8:
        increment(value)
        return value

    @ac.rule
    def direct_then_forward(value: ac.u8) -> ac.u8:
        value = value + 2
        increment(value)
        return value

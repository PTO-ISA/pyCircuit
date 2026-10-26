# ruff: noqa: F823, F841 -- captured state and formal proposals.
import pycircuit as ac


@ac.module
def AliasOwner():
    count: ac.u8 = 9

    @ac.method
    def read() -> ac.u8:
        return count

    @ac.method(guard=lambda enabled: enabled)
    def advance(enabled: ac.u1) -> None:
        saved, middle, final = forward_alias(count)
        local: ac.u8 = 13
        value_saved, value_local_middle, value_local_final = forward_alias(local)
        value_middle = read()
        value_final = read()
        ac.log("info", "formal alias", saved, middle, final, count)
        ac.log("info", "formal value", local, value_saved, value_middle, value_final)

    @ac.method
    def inspect() -> None:
        ac.log("info", "formal snapshot", count)


@ac.rule
def revise_alias(value: ac.u8) -> tuple[ac.u8, ac.u8, ac.u8]:
    saved = value
    value = value + 2
    middle = value
    value = value + 5
    return saved, middle, value


@ac.rule
def forward_alias(value: ac.u8) -> tuple[ac.u8, ac.u8, ac.u8]:
    saved, middle, final = revise_alias(value)
    return saved, middle, final

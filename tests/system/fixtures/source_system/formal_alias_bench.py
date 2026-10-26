# ruff: noqa: F823, F841 -- captured module state proposals.
import pycircuit as ac
from checks.formal_alias_provider import AliasOwner


# Writable templates bind actual owners within this source unit. The imported
# AliasOwner still exercises captured state through its published Methods.
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


@ac.system
def ExerciseFormalAliases():
    phase: ac.u3 = 0
    count: ac.u8 = 19
    forwarded: ac.u8 = 41
    owner = AliasOwner()

    @ac.rule
    def update_alias() -> None:
        owner.advance(phase != 1)

    @ac.rule
    def inspect_alias() -> None:
        owner.inspect()

    @ac.rule
    def update_formal() -> None:
        before = revise(count, phase != 1, phase == 2)
        ac.log("info", "formal conditional", before, count)

    @ac.rule
    def exercise_forwarding() -> None:
        snapshot = forwarded
        simple_value = forward(snapshot)
        direct_value = direct_then_forward(snapshot)
        simple_owner = forward(forwarded)
        direct_owner = direct_then_forward(forwarded)
        ac.log(
            "info",
            "formal forwarded",
            snapshot,
            simple_value,
            direct_value,
            simple_owner,
            direct_owner,
            forwarded,
        )

    @ac.rule
    def tick() -> None:
        phase = phase + 1

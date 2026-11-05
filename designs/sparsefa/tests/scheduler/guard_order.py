# ruff: noqa: F823, F841, N802 -- compiler-owned captured Module state.
"""Focused original-branch and nested-guard transaction equivalence fixtures."""
import pycircuit as ac


@ac.module
def Original(outer: ac.u1, inner: ac.u1, data: ac.u8) -> tuple[ac.u8, ac.u8]:
    saved: ac.u8 = 0
    heartbeat: ac.u8 = 0

    @ac.rule(guard=lambda: outer)
    def transfer() -> None:
        if inner:
            saved = data

    @ac.rule
    def tick() -> None:
        heartbeat = heartbeat + 1

    transfer()
    tick()
    return saved, heartbeat


@ac.module
def Nested(outer: ac.u1, inner: ac.u1, data: ac.u8) -> tuple[ac.u8, ac.u8]:
    saved: ac.u8 = 0
    heartbeat: ac.u8 = 0

    @ac.rule(guard=lambda: inner)
    def accept() -> None:
        saved = data

    @ac.rule(guard=lambda: outer)
    def transfer() -> None:
        accept()

    @ac.rule
    def tick() -> None:
        heartbeat = heartbeat + 1

    transfer()
    tick()
    return saved, heartbeat

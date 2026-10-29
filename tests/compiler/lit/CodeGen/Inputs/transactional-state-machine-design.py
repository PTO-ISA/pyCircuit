# ruff: noqa: F823, F841, N802 -- Captured Module state and Rule declarations.
"""A paused three-stage machine with independent wrapping counters."""

import pycircuit as ac


@ac.module
def Machine():
    stage: ac.u2 = 0
    permit: ac.u1 = 0
    completed: ac.u8 = 0
    saved: ac.u2 = 3
    narrow: ac.u3 = 6
    wide: ac.u8 = 250

    @ac.rule
    def unregistered() -> None:
        stage = 3

    @ac.rule(guard=lambda: permit == 1)
    def receive() -> None:
        previous = stage
        stage = 1
        saved = stage
        ac.log("info", "receive", previous, stage)

    @ac.rule
    def compute() -> None:
        stage = 2
        ac.log("info", "compute", stage)

    @ac.rule
    def publish() -> None:
        stage = 0
        completed = completed + 1
        ac.log("info", "publish", stage)

    if stage == 0:
        receive()
    elif stage == 1:
        compute()
    else:
        publish()

    @ac.autorule
    def tick() -> None:
        permit = permit ^ 1
        narrow = narrow + 1
        wide = wide + 10
        ac.log("info", "state", stage, permit, completed, saved, narrow, wide)

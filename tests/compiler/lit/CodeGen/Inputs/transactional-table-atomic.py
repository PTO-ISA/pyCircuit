# ruff: noqa: F821, F823, F841, N802 -- Captured Module state and instances.
"""Selected guards and late cancellation share one atomic transaction."""

import pycircuit as ac
from table_case.provider import Gate, GuardedWorker


@ac.module
def Dispatch():
    workers: ac.table[4, GuardedWorker]
    gate: Gate
    phase: ac.u8 = 0
    index: ac.u2 = 0
    accepted: ac.u8 = 0

    @ac.rule(guard=lambda: phase < 7)
    def attempt() -> None:
        accepted += 1
        updated = ac.u8(0)
        # Phase 0 skips the call entirely. Phase 1 reaches a false selected
        # guard; phase 2 reaches the later false guard after making proposals.
        if phase != 0:
            updated = workers[index].add(phase)
        gate.finish(phase)
        ac.log("info", "commit", phase, accepted, updated)

    attempt()

    @ac.autorule
    def observe_and_tick() -> None:
        ac.log("info", "progress", phase, accepted, gate.read(), workers[index].read())
        for lane, worker in enumerate(workers):
            ac.log("info", "worker", ac.u8(lane), worker.read())
        phase += 1
        index += 1


@ac.system
def Top():
    Dispatch()

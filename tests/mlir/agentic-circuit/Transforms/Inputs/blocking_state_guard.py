"""Blocking conditions observe branch-entry state; proposals remain serial."""

import agentic_circuit as ac


@ac.rule
def increment(count, mirror, request):
    if count != 4:
        count = count + 1
        mirror = count
        return request


@ac.rule
def decrement(count, mirror, request):
    if count != 0:
        count = count - 1
        mirror = count
        return request


@ac.system
def blocking_state_guard(up: ac.u3, down: ac.u3) -> tuple[ac.u3, ac.u3]:
    count: ac.u3 = 0
    mirror: ac.u3 = 0
    increased = increment(count, mirror, up)
    decreased = decrement(count, mirror, down)
    return increased, decreased

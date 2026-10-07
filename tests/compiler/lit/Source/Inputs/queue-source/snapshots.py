# ruff: noqa: F821, N802 -- forward hardware wires and module names are intentional.
import pycircuit as ac


@ac.struct
class SnapshotResult:
    a_ready: ac.u1
    a_valid: ac.u1
    a_data: ac.u13
    b_ready: ac.u1
    b_valid: ac.u1
    b_data: ac.u13
    observed: ac.u13


@ac.module
def Top(valid: ac.u1, data: ac.u13, take: ac.u1) -> SnapshotResult:
    sample = data
    a_ready, a_valid, a_data = ac.queue[ac.u13](valid, sample, b_ready)
    sample = sample + 1
    b_ready, b_valid, b_data = ac.queue[ac.u13](valid, sample, take)
    return SnapshotResult(
        a_ready=a_ready,
        a_valid=a_valid,
        a_data=a_data,
        b_ready=b_ready,
        b_valid=b_valid,
        b_data=b_data,
        observed=sample,
    )

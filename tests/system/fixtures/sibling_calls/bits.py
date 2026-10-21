# ruff: noqa: F823, F841 -- captured Module state is compiler-owned.
import pycircuit as ac


@ac.module
def BitsOwner():  # noqa: N802
    stored: ac.u4 = 0

    @ac.method(guard=lambda payload, enabled: enabled)
    def write(payload: ac.u4, enabled: ac.u1) -> None:
        stored = payload

    @ac.method(after=(write,))
    def read() -> ac.u4:
        return stored

    @ac.method
    def old() -> ac.u4:
        return stored

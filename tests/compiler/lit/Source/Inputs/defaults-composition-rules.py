"""An independently compiled autonomous state owner shared by two instances."""

import pycircuit as ac


@ac.module
def Guarded(enable: ac.u1, value: ac.u5) -> ac.u5:  # noqa: N802
    prior: ac.u5 = 0

    @ac.autorule(guard=lambda: enable)
    def update() -> None:
        prior = value  # noqa: F841 -- persistent Module state update.

    return prior

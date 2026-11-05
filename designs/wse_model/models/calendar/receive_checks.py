"""Model-resource diagnostics, distinct from WSE protocol faults."""

import pycircuit as ac


# ndf: implements=WSEMIG-RX-CAPACITY-001
@ac.module
def ReceiveChecks():  # noqa: N802 -- pyCircuit module name.
    @ac.method
    def require_identity_capacity(available: bool) -> None:
        assert available, "WSE_MODEL_CAPACITY: dedup identity slots exhausted"

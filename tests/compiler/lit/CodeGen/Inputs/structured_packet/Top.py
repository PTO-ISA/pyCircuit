"""Run the finite packet stream with the generated clock driver."""

import pycircuit as ac
from packet_case.design import PacketStream


@ac.system
def Top():  # noqa: N802
    PacketStream()

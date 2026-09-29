"""Root composition for the Queue adder gfsim example."""

import agentic_circuit as ac

from queue_adder import queue_adder


@ac.system
def queue_adder_system(left: ac.u32, right: ac.u32) -> ac.u32:
    return queue_adder(left, right)

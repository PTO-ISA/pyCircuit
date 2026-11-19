# Captured hardware source: Rules update Module-owned state.
# ruff: noqa: N802, F823, F841
import pycircuit as ac
from checks.fifo_endpoints import Consumer, Producer


@ac.system(samples=[])
def Bench(reset: ac.u1, permit: ac.u1, enable: ac.u1):
    writer = ac.domain(clock="clk_write")
    reader = ac.domain(clock="clk_read")
    fifo = ac.AsyncFIFO[ac.u9, 4](reset=reset)
    alias = fifo
    Producer(alias, reset, permit, enable, domain=writer)
    Consumer(fifo, domain=reader)

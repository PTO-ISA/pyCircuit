# ruff: noqa: N802 -- published hardware symbols.
import pycircuit as ac
from memory_suite.facade import Read, Store


@ac.module
def Middle(ren: ac.u1, raddr: ac.u2, wvalid: ac.u1,
           waddr: ac.u2, wdata: ac.u13, wstrb: ac.u2) -> Read:
    result = Store(ren, raddr, wvalid, waddr, wdata, wstrb)
    return result


@ac.module
def Top(ren: ac.u1, raddr: ac.u2, wvalid: ac.u1,
        waddr: ac.u2, wdata: ac.u13, wstrb: ac.u2) -> Read:
    result = Middle(ren, raddr, wvalid, waddr, wdata, wstrb)
    return result

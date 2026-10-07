# ruff: noqa: N802 -- published hardware symbols.
import pycircuit as ac


@ac.struct
class Read:
    rdata: ac.u13


@ac.module
def Memory(ren: ac.u1, raddr: ac.u2, wvalid: ac.u1,
           waddr: ac.u2, wdata: ac.u13, wstrb: ac.u2) -> Read:
    data = ac.sync_mem[ac.u13](ren, raddr, wvalid, waddr, wdata, wstrb, depth=3)
    return Read(rdata=data)

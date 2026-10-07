# Hardware symbols and the forward-read fixture deliberately use these names.
# ruff: noqa: N802, F821
import pycircuit as ac
from pycircuit import sync_mem as ram


@ac.struct
class Pair13:
    high: ac.u5
    low: ac.u8


@ac.struct
class BitsResult:
    r0: ac.u13
    r1: ac.u13


@ac.struct
class NominalResult:
    r0: Pair13
    r1: Pair13


@ac.struct
class ByteResult:
    r0: ac.u16
    r1: ac.u16


@ac.struct
class WordResult:
    r0: ac.u32
    r1: ac.u32


@ac.module
def Sync32(ren0: ac.u1, raddr0: ac.u2, ren1: ac.u1, raddr1: ac.u2,
           wvalid: ac.u1, waddr: ac.u2, wdata: ac.u32, wstrb: ac.u4) -> WordResult:
    # Both historical single-port 4x32 scenarios share this exact memory.
    value = ac.sync_mem[ac.u32](ren0, raddr0, wvalid, waddr, wdata, wstrb, depth=4)
    return WordResult(r0=value, r1=0)


@ac.module
def Sync(ren0: ac.u1, raddr0: ac.u2, ren1: ac.u1, raddr1: ac.u2,
         wvalid: ac.u1, waddr: ac.u2, wdata: ac.u13, wstrb: ac.u2) -> BitsResult:
    first = ram[ac.u13](ren0, raddr0, wvalid, waddr, wdata, wstrb, depth=3)
    second = ac.sync_mem[ac.u13](ren=ren1, raddr=raddr1, wvalid=False,
                                waddr=waddr, wdata=wdata, wstrb=wstrb, depth=3)
    return BitsResult(r0=first, r1=second)


@ac.module
def Dual(ren0: ac.u1, raddr0: ac.u2, ren1: ac.u1, raddr1: ac.u2,
         wvalid: ac.u1, waddr: ac.u2, wdata: Pair13, wstrb: ac.u2) -> NominalResult:
    first, second = ac.sync_mem_dp[Pair13](ren0, raddr0, ren1, raddr1,
                                         wvalid, waddr, wdata, wstrb, depth=3)
    return NominalResult(r0=first, r1=second)


@ac.module
def Byte(ren0: ac.u1, raddr0: ac.u3, ren1: ac.u1, raddr1: ac.u3,
         wvalid: ac.u1, waddr: ac.u3, wdata: ac.u16, wstrb: ac.u2) -> ByteResult:
    value = ac.byte_mem[ac.u16](raddr0, wvalid, waddr, wdata, wstrb, depth=5)
    return ByteResult(r0=value, r1=0)


@ac.module
def DepthOne(ren0: ac.u1, raddr0: ac.u1, ren1: ac.u1, raddr1: ac.u1,
             wvalid: ac.u1, waddr: ac.u1, wdata: ac.u13, wstrb: ac.u2) -> BitsResult:
    value = ram[ac.u13](ren0, raddr0, wvalid, waddr, wdata, wstrb, depth=1)
    return BitsResult(r0=value, r1=0)


@ac.module
def Forward(ren: ac.u1, write: ac.u1, address: ac.u2, data: ac.u13,
            strobe: ac.u2) -> BitsResult:
    first = ram[ac.u13](ren, second[0:2], write, address, data, strobe, depth=3)
    second = ram[ac.u13](ren, address, write, address, data, strobe, depth=3)
    return BitsResult(r0=first, r1=second)

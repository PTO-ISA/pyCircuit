from __future__ import annotations

from pycircuit.data import Bits
from pycircuit.dsl import Signal
from pycircuit.hw import Circuit, ClockDomain, Wire


class SRAMError(ValueError):
    pass


def SRAM(
    m: Circuit,
    cd: ClockDomain,
    ren: Wire,
    raddr: Wire,
    wvalid: Wire,
    waddr: Wire,
    wdata: Wire,
    wstrb: Wire,
    *,
    depth: int,
):
    clk_v = cd.clk
    rst_v = cd.rst
    if not isinstance(clk_v, Signal) or clk_v.ty != "!pyc.clock":
        raise SRAMError("SRAM domain clk must be !pyc.clock")
    if not isinstance(rst_v, Signal) or rst_v.ty != "!pyc.reset":
        raise SRAMError("SRAM domain rst must be !pyc.reset")

    ren_w = ren
    wvalid_w = wvalid
    raddr_w = raddr
    waddr_w = waddr
    wdata_w = wdata
    wstrb_w = wstrb
    if ren_w.ty != Bits(1) or wvalid_w.ty != Bits(1):
        raise SRAMError("SRAM ren/wvalid must be i1")

    rdata = m.sync_mem(
        clk_v,
        rst_v,
        ren=ren_w,
        raddr=raddr_w,
        wvalid=wvalid_w,
        waddr=waddr_w,
        wdata=wdata_w,
        wstrb=wstrb_w,
        depth=int(depth),
        name="mem",
    )
    always = m.const(1, width=1)
    captured = m.reg(
        clk_v,
        rst_v,
        always,
        rdata,
        m.const(0, width=wdata_w.width),
    )
    nba_safe = ren_w.select(rdata, captured.q)

    return m.bundle_connector(
        rdata=nba_safe,
        rdata_live=ren_w,
        rdata_raw=rdata,
        rdata_captured=captured.q,
    )

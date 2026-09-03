from __future__ import annotations

from pycircuit.data import Bits
from pycircuit.dsl import Signal
from pycircuit.hw import Circuit, ClockDomain, Wire


class FIFOError(ValueError):
    pass


def FIFO(
    m: Circuit,
    cd: ClockDomain,
    in_valid: Wire,
    in_data: Wire,
    out_ready: Wire,
    *,
    depth: int = 2,
):
    clk_v = cd.clk
    rst_v = cd.rst
    if not isinstance(clk_v, Signal) or clk_v.ty != "!pyc.clock":
        raise FIFOError("FIFO domain clk must be !pyc.clock")
    if not isinstance(rst_v, Signal) or rst_v.ty != "!pyc.reset":
        raise FIFOError("FIFO domain rst must be !pyc.reset")

    in_valid_w = in_valid
    in_data_w = in_data
    out_ready_w = out_ready

    if not isinstance(in_valid_w, Wire) or in_valid_w.ty != Bits(1):
        raise FIFOError("FIFO.in_valid must be i1")
    if not isinstance(in_data_w, Wire):
        raise FIFOError("FIFO.in_data must be integer wire")
    if not isinstance(out_ready_w, Wire) or out_ready_w.ty != Bits(1):
        raise FIFOError("FIFO.out_ready must be i1")

    in_ready, out_valid, out_data = m.fifo(
        clk_v,
        rst_v,
        in_valid=in_valid_w,
        in_data=in_data_w,
        out_ready=out_ready_w,
        depth=int(depth),
    )

    return m.bundle_connector(
        in_ready=in_ready,
        out_valid=out_valid,
        out_data=out_data,
    )

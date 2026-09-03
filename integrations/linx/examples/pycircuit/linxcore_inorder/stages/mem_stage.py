from __future__ import annotations

from pycircuit import Circuit, Wire, function, u, unsigned

from ..isa import (
    OP_HL_LBU_PCR,
    OP_HL_LHU_PCR,
    OP_HL_LWU_PCR,
    OP_LBU,
    OP_LBUI,
    OP_LHU,
    OP_LHUI,
    OP_LWU,
    OP_LWUI,
)
from ..pipeline import ExMemRegs, MemWbRegs


@function
def _bytes_with_wb_store_forwarding(
    m: Circuit,
    *,
    addr: Wire,
    mem_rdata: Wire,
    fwd_en: Wire,
    wb_store_addr: Wire,
    wb_store_size: Wire,
    wb_store_wdata: Wire,
) -> list[Wire]:
    _ = m
    bs = [mem_rdata[8 * i : 8 * (i + 1)] for i in range(8)]
    sbs = [wb_store_wdata[8 * i : 8 * (i + 1)] for i in range(8)]
    for i in range(8):
        ai = addr + i
        bi = bs[i]
        for k in range(8):
            cond = fwd_en & (ai == wb_store_addr + k) & wb_store_size.ugt(k)
            bi = sbs[k] if cond else bi
        bs[i] = bi
    return bs


@function
def build_mem_stage(
    m: Circuit,
    *,
    do_mem: Wire,
    exmem: ExMemRegs,
    memwb: MemWbRegs,
    mem_rdata: Wire,
    wb_store_valid: Wire,
    wb_store_addr: Wire,
    wb_store_size: Wire,
    wb_store_wdata: Wire,
) -> Wire:
    mem_val = u(64, 0)
    with m.scope("MEM"):
        pc = exmem.pc.out()
        window = exmem.window.out()
        pred_next_pc = exmem.pred_next_pc.out()
        op = exmem.op.out()
        len_bytes = exmem.len_bytes.out()
        regdst = exmem.regdst.out()
        srcl = exmem.srcl.out()
        srcr = exmem.srcr.out()
        imm = exmem.imm.out()
        alu = exmem.alu.out()
        is_load = exmem.is_load.out()
        is_store = exmem.is_store.out()
        addr = exmem.addr.out()
        size = exmem.size.out()
        wdata = exmem.wdata.out()
        fwd_en = wb_store_valid & is_load
        bs = _bytes_with_wb_store_forwarding(
            m,
            addr=addr,
            mem_rdata=mem_rdata,
            fwd_en=fwd_en,
            wb_store_addr=wb_store_addr,
            wb_store_size=wb_store_size,
            wb_store_wdata=wb_store_wdata,
        )
        b0 = bs[0]
        b1 = bs[1]
        b2 = bs[2]
        b3 = bs[3]
        b4 = bs[4]
        b5 = bs[5]
        b6 = bs[6]
        b7 = bs[7]
        word32 = m.vec(b3, b2, b1, b0).pack()
        half16 = m.vec(b1, b0).pack()
        dword64 = m.vec(b7, b6, b5, b4, b3, b2, b1, b0).pack()
        load_val = u(64, 0)
        if size == 1:
            if (op == OP_LBUI) | (op == OP_LBU) | (op == OP_HL_LBU_PCR):
                load_val = unsigned(b0)
            else:
                load_val = b0.as_signed()
        if size == 2:
            if (op == OP_LHUI) | (op == OP_LHU) | (op == OP_HL_LHU_PCR):
                load_val = unsigned(half16)
            else:
                load_val = half16.as_signed()
        if size == 4:
            if (op == OP_LWUI) | (op == OP_LWU) | (op == OP_HL_LWU_PCR):
                load_val = unsigned(word32)
            else:
                load_val = word32.as_signed()
        if size == 8:
            load_val = dword64
        mem_val = alu
        if is_load:
            mem_val = load_val
        if is_store:
            mem_val = 0
        memwb.pc.set(pc, when=do_mem)
        memwb.window.set(window, when=do_mem)
        memwb.pred_next_pc.set(pred_next_pc, when=do_mem)
        memwb.op.set(op, when=do_mem)
        memwb.len_bytes.set(len_bytes, when=do_mem)
        memwb.regdst.set(regdst, when=do_mem)
        memwb.srcl.set(srcl, when=do_mem)
        memwb.srcr.set(srcr, when=do_mem)
        memwb.imm.set(imm, when=do_mem)
        memwb.value.set(mem_val, when=do_mem)
        memwb.is_load.set(is_load, when=do_mem)
        memwb.is_store.set(is_store, when=do_mem)
        memwb.size.set(size, when=do_mem)
        memwb.addr.set(addr, when=do_mem)
        memwb.wdata.set(wdata, when=do_mem)
    return mem_val

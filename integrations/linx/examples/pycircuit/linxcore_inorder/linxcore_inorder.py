from __future__ import annotations

from pycircuit import Circuit, const, ct, module, u


@const
def _layout_cfg(
    m: Circuit, *, mem_bytes: int, icache_bytes: int, dcache_bytes: int
) -> tuple[int, int, int]:
    _ = m
    mem = ct.pow2_ceil(max(1, int(mem_bytes)))
    icache = ct.align_up(max(64, int(icache_bytes)), 64)
    dcache = ct.align_up(max(64, int(dcache_bytes)), 64)
    return (mem, icache, dcache)


@module
def build(
    m: Circuit,
    *,
    mem_bytes: int = 1 << 20,
    icache_bytes: int = 16 << 10,
    dcache_bytes: int = 32 << 10,
) -> None:
    mem_cfg, icache_cfg, dcache_cfg = _layout_cfg(
        m,
        mem_bytes=mem_bytes,
        icache_bytes=icache_bytes,
        dcache_bytes=dcache_bytes,
    )
    _ = (mem_cfg, icache_cfg, dcache_cfg)

    clk = m.clock("clk")
    rst = m.reset("rst")

    boot_pc = m.input("boot_pc", width=64)
    boot_sp = m.input("boot_sp", width=64)
    irq = m.input("irq", width=1)
    irq_vector = m.input("irq_vector", width=64)

    host_wvalid = m.input("host_wvalid", width=1)
    host_waddr = m.input("host_waddr", width=64)
    host_wdata = m.input("host_wdata", width=64)
    host_wstrb = m.input("host_wstrb", width=8)
    _ = (boot_sp, irq_vector, host_wstrb)

    pc = m.out("pc_r", clk=clk, rst=rst, width=64, init=boot_pc)
    cycles = m.out("cycles_r", clk=clk, rst=rst, width=64, init=u(64, 0))
    halted = m.out("halted_r", clk=clk, rst=rst, width=1, init=u(1, 0))
    exit_code = m.out("exit_code_r", clk=clk, rst=rst, width=32, init=u(32, 0))

    active = ~halted.out()
    pc.set(pc.out() + u(64, 4), when=active)
    cycles.set(cycles.out() + u(64, 1), when=active)

    timeout_halt = cycles.out() >= u(64, 128)
    halted.set(u(1, 1), when=irq | timeout_halt)

    uart_addr = u(64, 0x1000_0000)
    uart_valid = host_wvalid & (host_waddr == uart_addr)
    uart_byte = host_wdata[0:8]

    m.output("pc", pc)
    m.output("cycles", cycles)
    m.output("halted", halted)
    m.output("exit_code", exit_code)
    m.output("stage", u(3, 0))

    m.output("uart_valid", uart_valid)
    m.output("uart_byte", uart_byte)

    m.output("if_window", u(64, 0))
    m.output("wb0_valid", u(1, 0))
    m.output("wb1_valid", u(1, 0))
    m.output("wb0_pc", u(64, 0))
    m.output("wb1_pc", u(64, 0))
    m.output("wb0_op", u(12, 0))
    m.output("wb1_op", u(12, 0))
    m.output("wb_op", u(12, 0))
    m.output("wb_regdst", u(6, 0))
    m.output("wb_value", u(64, 0))
    m.output("commit_cond", u(1, 0))
    m.output("commit_tgt", u(64, 0))

    m.output("a0", u(64, 0))
    m.output("a1", u(64, 0))
    m.output("ra", u(64, 0))
    m.output("sp", boot_sp)


build.__pycircuit_name__ = "linxcore_inorder"

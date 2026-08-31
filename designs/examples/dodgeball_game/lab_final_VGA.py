"""VGA timing generator — pyCircuit v6 rewrite of lab_final_VGA.v.

Implements the same 640x480@60Hz timing logic with 800x524 total counts.
"""

from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    u,
)

# VGA timing constants (same as reference Verilog)
HS_STA = 16
HS_END = 16 + 96
HA_STA = 16 + 96 + 48
VS_STA = 480 + 11
VS_END = 480 + 11 + 2
VA_END = 480
LINE = 800
SCREEN = 524


def build(m: CycleAwareCircuit, domain: CycleAwareDomain) -> None:
    """Standalone VGA module (ports mirror the reference Verilog)."""
    cd = domain.clock_domain
    clk = cd.clk
    rst = cd.rst

    i_pix_stb = m.input("i_pix_stb", width=1)

    h_count = m.out("vga_h_count", domain=cd, width=10, init=u(10, 0))
    v_count = m.out("vga_v_count", domain=cd, width=10, init=u(10, 0))

    h = h_count.out()
    v = v_count.out()

    h_end = h == u(10, LINE)
    v_end = v == u(10, SCREEN)

    h_inc = h + u(10, 1)
    v_inc = v + u(10, 1)

    h_after = u(10, 0) if h_end else h_inc
    v_after = v_inc if h_end else v
    v_after = u(10, 0) if v_end else v_after

    h_next = h_after if i_pix_stb else h
    v_next = v_after if i_pix_stb else v

    o_hs = ~((h >= u(10, HS_STA)) & (h < u(10, HS_END)))
    o_vs = ~((v >= u(10, VS_STA)) & (v < u(10, VS_END)))

    o_x = u(10, 0) if (h < u(10, HA_STA)) else (h - u(10, HA_STA))
    y_full = u(10, VA_END - 1) if (v >= u(10, VA_END)) else v
    o_y = y_full[0:9]

    o_blanking = (h < u(10, HA_STA)) | (v > u(10, VA_END - 1))
    o_animate = (v == u(10, VA_END - 1)) & (h == u(10, LINE))

    h_count.set(h_next)
    v_count.set(v_next)

    m.output("o_hs", o_hs)
    m.output("o_vs", o_vs)
    m.output("o_blanking", o_blanking)
    m.output("o_animate", o_animate)
    m.output("o_x", o_x)
    m.output("o_y", o_y)


build.__pycircuit_name__ = "lab_final_vga"

if __name__ == "__main__":
    print(compile_cycle_aware(build, name="lab_final_vga").emit_mlir())

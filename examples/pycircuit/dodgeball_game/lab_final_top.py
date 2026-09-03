"""Dodgeball top — pyCircuit v6 rewrite of lab_final_top.v.

Notes:
- `clk` corresponds to the original `CLK_in`.
- A synchronous `rst` port is introduced for deterministic initialization.
- The internal game logic still uses `RST_BTN` exactly like the reference.
"""

from __future__ import annotations

from pycircuit import (
    CycleAwareCircuit,
    CycleAwareDomain,
    compile_cycle_aware,
    u,
)

# VGA timing constants (same as lab_final_VGA)
HS_STA = 16
HS_END = 16 + 96
HA_STA = 16 + 96 + 48
VS_STA = 480 + 11
VS_END = 480 + 11 + 2
VA_END = 480
LINE = 800
SCREEN = 524


def build(
    m: CycleAwareCircuit, domain: CycleAwareDomain, *, MAIN_CLK_BIT: int = 20
) -> None:
    if MAIN_CLK_BIT < 0 or MAIN_CLK_BIT > 24:
        raise ValueError("MAIN_CLK_BIT must be in [0, 24]")
    cd = domain.clock_domain
    clk = cd.clk
    rst = cd.rst

    # ================================================================
    # Inputs
    # ================================================================
    rst_btn = m.input("RST_BTN", width=1)
    start = m.input("START", width=1)
    left = m.input("left", width=1)
    right = m.input("right", width=1)

    # ================================================================
    # Registers
    # ================================================================
    cnt = m.out("pix_cnt", domain=cd, width=16, init=u(16, 0))
    pix_stb = m.out("pix_stb", domain=cd, width=1, init=u(1, 0))
    main_clk = m.out("main_clk", domain=cd, width=25, init=u(25, 0))

    player_x = m.out("player_x", domain=cd, width=4, init=u(4, 8))
    j = m.out("j", domain=cd, width=5, init=u(5, 0))

    ob1_x = m.out("ob1_x", domain=cd, width=4, init=u(4, 1))
    ob2_x = m.out("ob2_x", domain=cd, width=4, init=u(4, 4))
    ob3_x = m.out("ob3_x", domain=cd, width=4, init=u(4, 7))

    ob1_y = m.out("ob1_y", domain=cd, width=4, init=u(4, 0))
    ob2_y = m.out("ob2_y", domain=cd, width=4, init=u(4, 0))
    ob3_y = m.out("ob3_y", domain=cd, width=4, init=u(4, 0))

    fsm_state = m.out("fsm_state", domain=cd, width=3, init=u(3, 0))

    # ================================================================
    # Combinational logic
    # ================================================================

    # --- Pixel strobe divider ---
    cnt_ext = cnt.out() | u(17, 0)
    sum17 = cnt_ext + u(17, 0x4000)
    cnt_next = sum17[0:16]
    pix_stb_next = sum17[16]

    # --- Main clock divider bit (for game logic tick) ---
    main_clk_next = main_clk.out() + u(25, 1)
    main_bit = main_clk.out()[MAIN_CLK_BIT]
    main_next_bit = main_clk_next[MAIN_CLK_BIT]
    game_tick = (~main_bit) & main_next_bit

    # --- VGA timing (inlined from lab_final_VGA) ---
    vga_h_count = m.out("vga_h_count", domain=cd, width=10, init=u(10, 0))
    vga_v_count = m.out("vga_v_count", domain=cd, width=10, init=u(10, 0))

    vh = vga_h_count.out()
    vv = vga_v_count.out()

    vh_end = vh == u(10, LINE)
    vv_end = vv == u(10, SCREEN)

    vh_inc = vh + u(10, 1)
    vv_inc = vv + u(10, 1)

    vh_after = u(10, 0) if vh_end else vh_inc
    vv_after = vv_inc if vh_end else vv
    vv_after = u(10, 0) if vv_end else vv_after

    i_pix_stb = pix_stb.out()
    vh_next = vh_after if i_pix_stb else vh
    vv_next = vv_after if i_pix_stb else vv

    vga_hs = ~((vh >= u(10, HS_STA)) & (vh < u(10, HS_END)))
    vga_vs = ~((vv >= u(10, VS_STA)) & (vv < u(10, VS_END)))

    vga_x_raw = u(10, 0) if (vh < u(10, HA_STA)) else (vh - u(10, HA_STA))
    vga_y_full = u(10, VA_END - 1) if (vv >= u(10, VA_END)) else vv
    vga_y_raw = vga_y_full[0:9]

    vga_h_count.set(vh_next)
    vga_v_count.set(vv_next)

    x = vga_x_raw
    y = vga_y_raw

    # --- Read register Q outputs for combinational logic ---
    px = player_x.out()
    jv = j.out()
    o1x = ob1_x.out()
    o1y = ob1_y.out()
    o2x = ob2_x.out()
    o2y = ob2_y.out()
    o3x = ob3_x.out()
    o3y = ob3_y.out()
    fsm = fsm_state.out()

    # --- Collision detection ---
    collision = (
        ((o1x == px) & (o1y == u(4, 10)))
        | ((o2x == px) & (o2y == u(4, 10)))
        | ((o3x == px) & (o3y == u(4, 10)))
    )

    # --- Object motion increments (boolean -> 4-bit) ---
    inc1 = ((jv > u(5, 0)) & (jv < u(5, 13))) | u(4, 0)
    inc2 = ((jv > u(5, 3)) & (jv < u(5, 16))) | u(4, 0)
    inc3 = ((jv > u(5, 7)) & (jv < u(5, 20))) | u(4, 0)

    # --- FSM state flags ---
    st0 = fsm == u(3, 0)
    st1 = fsm == u(3, 1)
    st2 = fsm == u(3, 2)

    cond_state0 = game_tick & st0
    cond_state1 = game_tick & st1
    cond_state2 = game_tick & st2

    cond_start = cond_state0 & start
    cond_rst_s1 = cond_state1 & rst_btn
    cond_rst_s2 = cond_state2 & rst_btn
    cond_collision = cond_state1 & collision
    cond_j20 = cond_state1 & (jv == u(5, 20))

    # --- Player movement (left/right) ---
    left_only = left & ~right
    right_only = right & ~left
    can_left = px > u(4, 0)
    can_right = px < u(4, 15)
    move_left = cond_state1 & left_only & can_left
    move_right = cond_state1 & right_only & can_right

    # --- VGA draw logic ---
    x10 = x
    y10 = y | u(10, 0)

    player_x0 = (px | u(10, 0)) * u(10, 40)
    player_x1 = ((px + u(4, 1)) | u(10, 0)) * u(10, 40)

    ob1_x0 = (o1x | u(10, 0)) * u(10, 40)
    ob1_x1 = ((o1x + u(4, 1)) | u(10, 0)) * u(10, 40)
    ob1_y0 = (o1y | u(10, 0)) * u(10, 40)
    ob1_y1 = ((o1y + u(4, 1)) | u(10, 0)) * u(10, 40)

    ob2_x0 = (o2x | u(10, 0)) * u(10, 40)
    ob2_x1 = ((o2x + u(4, 1)) | u(10, 0)) * u(10, 40)
    ob2_y0 = (o2y | u(10, 0)) * u(10, 40)
    ob2_y1 = ((o2y + u(4, 1)) | u(10, 0)) * u(10, 40)

    ob3_x0 = (o3x | u(10, 0)) * u(10, 40)
    ob3_x1 = ((o3x + u(4, 1)) | u(10, 0)) * u(10, 40)
    ob3_y0 = (o3y | u(10, 0)) * u(10, 40)
    ob3_y1 = ((o3y + u(4, 1)) | u(10, 0)) * u(10, 40)

    sq_player = (
        (x10 > player_x0) & (y10 > u(10, 400)) & (x10 < player_x1) & (y10 < u(10, 440))
    )

    sq_object1 = (x10 > ob1_x0) & (y10 > ob1_y0) & (x10 < ob1_x1) & (y10 < ob1_y1)
    sq_object2 = (x10 > ob2_x0) & (y10 > ob2_y0) & (x10 < ob2_x1) & (y10 < ob2_y1)
    sq_object3 = (x10 > ob3_x0) & (y10 > ob3_y0) & (x10 < ob3_x1) & (y10 < ob3_y1)

    over_wire = (
        (x10 > u(10, 0)) & (y10 > u(10, 0)) & (x10 < u(10, 640)) & (y10 < u(10, 480))
    )
    down = (
        (x10 > u(10, 0)) & (y10 > u(10, 440)) & (x10 < u(10, 640)) & (y10 < u(10, 480))
    )
    up = (x10 > u(10, 0)) & (y10 > u(10, 0)) & (x10 < u(10, 640)) & (y10 < u(10, 40))

    fsm_over = fsm == u(3, 2)
    not_over = ~fsm_over

    circle = u(1, 0)

    vga_r_bit = sq_player & not_over
    vga_b_bit = (sq_object1 | sq_object2 | sq_object3 | down | up) & not_over
    vga_g_bit = circle | (over_wire & fsm_over)

    vga_r = m.cat(vga_r_bit, u(3, 0))
    vga_g = m.cat(vga_g_bit, u(3, 0))
    vga_b = m.cat(vga_b_bit, u(3, 0))

    # ================================================================
    # Register updates (last-write-wins order mirrors Verilog)
    # ================================================================

    # Clock divider flops
    cnt.set(cnt_next)
    pix_stb.set(pix_stb_next)
    main_clk.set(main_clk_next)

    # FSM state
    fsm_state.set(u(3, 1), when=cond_start)
    fsm_state.set(u(3, 0), when=cond_rst_s1)
    fsm_state.set(u(3, 2), when=cond_collision)
    fsm_state.set(u(3, 0), when=cond_rst_s2)

    # j counter
    j.set(u(5, 0), when=cond_rst_s1)
    j.set(u(5, 0), when=cond_j20)
    j.set(jv + u(5, 1), when=cond_state1)
    j.set(u(5, 0), when=cond_rst_s2)

    # player movement
    player_x.set(px - u(4, 1), when=move_left)
    player_x.set(px + u(4, 1), when=move_right)

    # object Y updates
    ob1_y.set(u(4, 0), when=cond_rst_s1)
    ob1_y.set(u(4, 0), when=cond_j20)
    ob1_y.set(o1y + inc1, when=cond_state1)
    ob1_y.set(u(4, 0), when=cond_rst_s2)

    ob2_y.set(u(4, 0), when=cond_rst_s1)
    ob2_y.set(u(4, 0), when=cond_j20)
    ob2_y.set(o2y + inc2, when=cond_state1)
    ob2_y.set(u(4, 0), when=cond_rst_s2)

    ob3_y.set(u(4, 0), when=cond_rst_s1)
    ob3_y.set(u(4, 0), when=cond_j20)
    ob3_y.set(o3y + inc3, when=cond_state1)
    ob3_y.set(u(4, 0), when=cond_rst_s2)

    # ================================================================
    # Outputs
    # ================================================================
    m.output("VGA_HS_O", vga_hs)
    m.output("VGA_VS_O", vga_vs)
    m.output("VGA_R", vga_r)
    m.output("VGA_G", vga_g)
    m.output("VGA_B", vga_b)

    # Debug / visualization taps
    m.output("dbg_state", fsm_state)
    m.output("dbg_j", j)
    m.output("dbg_player_x", player_x)
    m.output("dbg_ob1_x", ob1_x)
    m.output("dbg_ob1_y", ob1_y)
    m.output("dbg_ob2_x", ob2_x)
    m.output("dbg_ob2_y", ob2_y)
    m.output("dbg_ob3_x", ob3_x)
    m.output("dbg_ob3_y", ob3_y)


build.__pycircuit_name__ = "dodgeball_game"

if __name__ == "__main__":
    print(
        compile_cycle_aware(build, name="dodgeball_game", MAIN_CLK_BIT=20).emit_mlir()
    )

"""The fixed three-object dodgeball game with its original VGA timing."""

import pycircuit as ac


@ac.struct
class GameResult:
    VGA_HS_O: ac.u1
    VGA_VS_O: ac.u1
    VGA_R: ac.u4
    VGA_G: ac.u4
    VGA_B: ac.u4
    dbg_state: ac.u3
    dbg_j: ac.u5
    dbg_player_x: ac.u4
    dbg_ob1_x: ac.u4
    dbg_ob1_y: ac.u4
    dbg_ob2_x: ac.u4
    dbg_ob2_y: ac.u4
    dbg_ob3_x: ac.u4
    dbg_ob3_y: ac.u4


@ac.rule
def advance_game(
    pix_cnt,
    pix_stb,
    main_clk,
    player_x,
    j,
    ob1_x,
    ob2_x,
    ob3_x,
    ob1_y,
    ob2_y,
    ob3_y,
    fsm_state,
    vga_h_count,
    vga_v_count,
    RST_BTN,
    START,
    left,
    right,
) -> GameResult:  # noqa: N803
    # Explicit snapshots retain old-Q RHS values after later owner assignments.
    cnt = pix_cnt
    old_pix_stb = pix_stb
    old_main_clk = main_clk
    px = player_x
    jv = j
    o1x = ob1_x
    o2x = ob2_x
    o3x = ob3_x
    o1y = ob1_y
    o2y = ob2_y
    o3y = ob3_y
    fsm = fsm_state
    vh = vga_h_count
    vv = vga_v_count

    cnt_ext: ac.u17 = cnt
    sum17 = cnt_ext + 0x4000
    cnt_value = sum17[:16]
    pix_stb_value = sum17[16:17]
    main_value = old_main_clk + 1
    game_tick = ~old_main_clk[4:5] & main_value[4:5]

    # Pixel timing consumes the previous strobe, not the newly computed carry.
    vh_end = vh == 800
    vv_end = vv == 524
    vh_after: ac.u10 = 0 if vh_end else vh + 1
    vv_after = vv + 1 if vh_end else vv
    vv_value: ac.u10 = 0 if vv_end else vv_after
    vh_value = vh_after if old_pix_stb else vh
    vv_value = vv_value if old_pix_stb else vv

    hs: ac.u1 = ~((vh >= 16) & (vh < 112))
    vs: ac.u1 = ~((vv >= 491) & (vv < 493))
    x: ac.u10 = 0 if vh < 160 else vh - 160
    y_full: ac.u10 = 479 if vv >= 480 else vv
    y: ac.u10 = y_full[:9]

    collision = (
        ((o1x == px) & (o1y == 10))
        | ((o2x == px) & (o2y == 10))
        | ((o3x == px) & (o3y == 10))
    )
    inc1: ac.u4 = 1 if (jv > 0) & (jv < 13) else 0
    inc2: ac.u4 = 1 if (jv > 3) & (jv < 16) else 0
    inc3: ac.u4 = 1 if (jv > 7) & (jv < 20) else 0

    cond_state0 = game_tick & (fsm == 0)
    cond_state1 = game_tick & (fsm == 1)
    cond_state2 = game_tick & (fsm == 2)
    cond_start = cond_state0 & START
    cond_rst_s1 = cond_state1 & RST_BTN
    cond_rst_s2 = cond_state2 & RST_BTN
    cond_collision = cond_state1 & collision
    cond_j20 = cond_state1 & (jv == 20)
    move_left = cond_state1 & left & ~right & (px > 0)
    move_right = cond_state1 & right & ~left & (px < 15)

    # Upper bounds add at four bits first, exactly as in the original design.
    px_plus: ac.u4 = px + 1
    o1x_plus: ac.u4 = o1x + 1
    o2x_plus: ac.u4 = o2x + 1
    o3x_plus: ac.u4 = o3x + 1
    o1y_plus: ac.u4 = o1y + 1
    o2y_plus: ac.u4 = o2y + 1
    o3y_plus: ac.u4 = o3y + 1
    px_wide: ac.u10 = px
    px_plus_wide: ac.u10 = px_plus
    o1x_wide: ac.u10 = o1x
    o1x_plus_wide: ac.u10 = o1x_plus
    o2x_wide: ac.u10 = o2x
    o2x_plus_wide: ac.u10 = o2x_plus
    o3x_wide: ac.u10 = o3x
    o3x_plus_wide: ac.u10 = o3x_plus
    o1y_wide: ac.u10 = o1y
    o1y_plus_wide: ac.u10 = o1y_plus
    o2y_wide: ac.u10 = o2y
    o2y_plus_wide: ac.u10 = o2y_plus
    o3y_wide: ac.u10 = o3y
    o3y_plus_wide: ac.u10 = o3y_plus

    sq_player = (x > px_wide * 40) & (y > 400) & (x < px_plus_wide * 40) & (y < 440)
    sq_object1 = (
        (x > o1x_wide * 40)
        & (y > o1y_wide * 40)
        & (x < o1x_plus_wide * 40)
        & (y < o1y_plus_wide * 40)
    )
    sq_object2 = (
        (x > o2x_wide * 40)
        & (y > o2y_wide * 40)
        & (x < o2x_plus_wide * 40)
        & (y < o2y_plus_wide * 40)
    )
    sq_object3 = (
        (x > o3x_wide * 40)
        & (y > o3y_wide * 40)
        & (x < o3x_plus_wide * 40)
        & (y < o3y_plus_wide * 40)
    )
    over_wire = (x > 0) & (y > 0) & (x < 640) & (y < 480)
    down = (x > 0) & (y > 440) & (x < 640) & (y < 480)
    up = (x > 0) & (y > 0) & (x < 640) & (y < 40)
    fsm_over: ac.u1 = fsm == 2
    red: ac.u1 = sq_player & ~fsm_over
    blue: ac.u1 = (sq_object1 | sq_object2 | sq_object3 | down | up) & ~fsm_over
    green: ac.u1 = over_wire & fsm_over
    low_color: ac.u3 = 0

    # The complete output record is a pre-commit snapshot.
    result = GameResult(
        VGA_HS_O=hs,
        VGA_VS_O=vs,
        VGA_R=ac.concat(red, low_color),
        VGA_G=ac.concat(green, low_color),
        VGA_B=ac.concat(blue, low_color),
        dbg_state=fsm,
        dbg_j=jv,
        dbg_player_x=px,
        dbg_ob1_x=o1x,
        dbg_ob1_y=o1y,
        dbg_ob2_x=o2x,
        dbg_ob2_y=o2y,
        dbg_ob3_x=o3x,
        dbg_ob3_y=o3y,
    )

    vga_h_count = vh_value
    vga_v_count = vv_value
    pix_cnt = cnt_value
    pix_stb = pix_stb_value
    main_clk = main_value

    # Preserve the original guarded-write ordering, including overwritten resets.
    if cond_start:
        fsm_state = 1
    if cond_rst_s1:
        fsm_state = 0
    if cond_collision:
        fsm_state = 2
    if cond_rst_s2:
        fsm_state = 0

    if cond_rst_s1:
        j = 0
    if cond_j20:
        j = 0
    if cond_state1:
        j = jv + 1
    if cond_rst_s2:
        j = 0

    if move_left:
        player_x = px - 1
    if move_right:
        player_x = px + 1

    if cond_rst_s1:
        ob1_y = 0
    if cond_j20:
        ob1_y = 0
    if cond_state1:
        ob1_y = o1y + inc1
    if cond_rst_s2:
        ob1_y = 0

    if cond_rst_s1:
        ob2_y = 0
    if cond_j20:
        ob2_y = 0
    if cond_state1:
        ob2_y = o2y + inc2
    if cond_rst_s2:
        ob2_y = 0

    if cond_rst_s1:
        ob3_y = 0
    if cond_j20:
        ob3_y = 0
    if cond_state1:
        ob3_y = o3y + inc3
    if cond_rst_s2:
        ob3_y = 0
    return result


@ac.module
def DodgeballGame(  # noqa: N802
    RST_BTN: ac.u1, START: ac.u1, left: ac.u1, right: ac.u1  # noqa: N803
) -> GameResult:
    pix_cnt: ac.u16 = 0
    pix_stb: ac.u1 = 0
    main_clk: ac.u25 = 0
    player_x: ac.u4 = 8
    j: ac.u5 = 0
    ob1_x: ac.u4 = 1
    ob2_x: ac.u4 = 4
    ob3_x: ac.u4 = 7
    ob1_y: ac.u4 = 0
    ob2_y: ac.u4 = 0
    ob3_y: ac.u4 = 0
    fsm_state: ac.u3 = 0
    vga_h_count: ac.u10 = 0
    vga_v_count: ac.u10 = 0
    return advance_game(
        pix_cnt,
        pix_stb,
        main_clk,
        player_x,
        j,
        ob1_x,
        ob2_x,
        ob3_x,
        ob1_y,
        ob2_y,
        ob3_y,
        fsm_state,
        vga_h_count,
        vga_v_count,
        RST_BTN,
        START,
        left,
        right,
    )

"""Regular-clock dodgeball_game scenario; original physical-control oracles remain."""

from example_dodgeball_game.dodgeball_game import DodgeballGame
from pycircuit import bits, log, rule, system, struct, table


@struct
class Scenario:
    end: bits[64]
    expected_dbg_j: bits[5]
    expected_dbg_player_x: bits[4]
    expected_dbg_ob1_y: bits[4]
    expected_dbg_ob2_y: bits[4]
    expected_dbg_ob3_y: bits[4]


@struct
class Scenarios:
    rows: table[123, Scenario]


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseDodgeballGame():  # noqa: N802
    phase: bits[64] = 0

    # fmt: off
    scenarios = Scenarios(
        rows=(
            Scenario(end=47, expected_dbg_player_x=8),
            Scenario(end=79, expected_dbg_j=1, expected_dbg_player_x=7),
            Scenario(end=111, expected_dbg_j=2, expected_dbg_player_x=6, expected_dbg_ob1_y=1),
            Scenario(end=143, expected_dbg_j=3, expected_dbg_player_x=5, expected_dbg_ob1_y=2),
            Scenario(end=175, expected_dbg_j=4, expected_dbg_player_x=4, expected_dbg_ob1_y=3),
            Scenario(end=207, expected_dbg_j=5, expected_dbg_player_x=3, expected_dbg_ob1_y=4, expected_dbg_ob2_y=1),
            Scenario(end=239, expected_dbg_j=6, expected_dbg_player_x=2, expected_dbg_ob1_y=5, expected_dbg_ob2_y=2),
            Scenario(end=271, expected_dbg_j=7, expected_dbg_player_x=1, expected_dbg_ob1_y=6, expected_dbg_ob2_y=3),
            Scenario(end=303, expected_dbg_j=8, expected_dbg_ob1_y=7, expected_dbg_ob2_y=4),
            Scenario(end=335, expected_dbg_j=9, expected_dbg_ob1_y=8, expected_dbg_ob2_y=5, expected_dbg_ob3_y=1),
            Scenario(end=367, expected_dbg_j=10, expected_dbg_player_x=1, expected_dbg_ob1_y=9, expected_dbg_ob2_y=6, expected_dbg_ob3_y=2),
            Scenario(end=399, expected_dbg_j=11, expected_dbg_player_x=2, expected_dbg_ob1_y=10, expected_dbg_ob2_y=7, expected_dbg_ob3_y=3),
            Scenario(end=431, expected_dbg_j=12, expected_dbg_player_x=3, expected_dbg_ob1_y=11, expected_dbg_ob2_y=8, expected_dbg_ob3_y=4),
            Scenario(end=463, expected_dbg_j=13, expected_dbg_player_x=4, expected_dbg_ob1_y=12, expected_dbg_ob2_y=9, expected_dbg_ob3_y=5),
            Scenario(end=495, expected_dbg_j=14, expected_dbg_player_x=5, expected_dbg_ob1_y=12, expected_dbg_ob2_y=10, expected_dbg_ob3_y=6),
            Scenario(end=527, expected_dbg_j=15, expected_dbg_player_x=6, expected_dbg_ob1_y=12, expected_dbg_ob2_y=11, expected_dbg_ob3_y=7),
            Scenario(end=559, expected_dbg_j=16, expected_dbg_player_x=7, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=8),
            Scenario(end=591, expected_dbg_j=17, expected_dbg_player_x=8, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=9),
            Scenario(end=623, expected_dbg_j=18, expected_dbg_player_x=9, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=10),
            Scenario(end=655, expected_dbg_j=19, expected_dbg_player_x=10, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=11),
            Scenario(end=687, expected_dbg_j=20, expected_dbg_player_x=11, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=719, expected_dbg_j=21, expected_dbg_player_x=12, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=751, expected_dbg_j=22, expected_dbg_player_x=13, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=783, expected_dbg_j=23, expected_dbg_player_x=14, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=815, expected_dbg_j=24, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=847, expected_dbg_j=25, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=879, expected_dbg_j=26, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=911, expected_dbg_j=27, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=975, expected_dbg_j=28, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=1039, expected_dbg_j=29, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=1071, expected_dbg_j=30, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=1103, expected_dbg_j=31, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=1135, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=1167, expected_dbg_j=1, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=1199, expected_dbg_j=2, expected_dbg_player_x=15, expected_dbg_ob1_y=13, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=1231, expected_dbg_j=3, expected_dbg_player_x=15, expected_dbg_ob1_y=14, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=1263, expected_dbg_j=4, expected_dbg_player_x=15, expected_dbg_ob1_y=15, expected_dbg_ob2_y=12, expected_dbg_ob3_y=12),
            Scenario(end=1295, expected_dbg_j=5, expected_dbg_player_x=15, expected_dbg_ob2_y=13, expected_dbg_ob3_y=12),
            Scenario(end=1327, expected_dbg_j=6, expected_dbg_player_x=15, expected_dbg_ob1_y=1, expected_dbg_ob2_y=14, expected_dbg_ob3_y=12),
            Scenario(end=1359, expected_dbg_j=7, expected_dbg_player_x=15, expected_dbg_ob1_y=2, expected_dbg_ob2_y=15, expected_dbg_ob3_y=12),
            Scenario(end=1391, expected_dbg_j=8, expected_dbg_player_x=15, expected_dbg_ob1_y=3, expected_dbg_ob3_y=12),
            Scenario(end=1423, expected_dbg_j=9, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=1, expected_dbg_ob3_y=13),
            Scenario(end=1455, expected_dbg_j=10, expected_dbg_player_x=15, expected_dbg_ob1_y=5, expected_dbg_ob2_y=2, expected_dbg_ob3_y=14),
            Scenario(end=1487, expected_dbg_j=11, expected_dbg_player_x=15, expected_dbg_ob1_y=6, expected_dbg_ob2_y=3, expected_dbg_ob3_y=15),
            Scenario(end=1519, expected_dbg_j=12, expected_dbg_player_x=15, expected_dbg_ob1_y=7, expected_dbg_ob2_y=4),
            Scenario(end=1551, expected_dbg_j=13, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=5, expected_dbg_ob3_y=1),
            Scenario(end=1583, expected_dbg_j=14, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=6, expected_dbg_ob3_y=2),
            Scenario(end=1615, expected_dbg_j=15, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=7, expected_dbg_ob3_y=3),
            Scenario(end=1647, expected_dbg_j=16, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=4),
            Scenario(end=1679, expected_dbg_j=17, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=5),
            Scenario(end=1711, expected_dbg_j=18, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=6),
            Scenario(end=1743, expected_dbg_j=19, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=7),
            Scenario(end=1775, expected_dbg_j=20, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=1807, expected_dbg_j=21, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=1839, expected_dbg_j=22, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=1871, expected_dbg_j=23, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=1903, expected_dbg_j=24, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=1935, expected_dbg_j=25, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=1967, expected_dbg_j=26, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=1999, expected_dbg_j=27, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=2031, expected_dbg_j=28, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=2063, expected_dbg_j=29, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=2095, expected_dbg_j=30, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=2127, expected_dbg_j=31, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=2159, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=2191, expected_dbg_j=1, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=2223, expected_dbg_j=2, expected_dbg_player_x=15, expected_dbg_ob1_y=9, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=2255, expected_dbg_j=3, expected_dbg_player_x=15, expected_dbg_ob1_y=10, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=2287, expected_dbg_j=4, expected_dbg_player_x=15, expected_dbg_ob1_y=11, expected_dbg_ob2_y=8, expected_dbg_ob3_y=8),
            Scenario(end=2319, expected_dbg_j=5, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=9, expected_dbg_ob3_y=8),
            Scenario(end=2351, expected_dbg_j=6, expected_dbg_player_x=15, expected_dbg_ob1_y=13, expected_dbg_ob2_y=10, expected_dbg_ob3_y=8),
            Scenario(end=2383, expected_dbg_j=7, expected_dbg_player_x=15, expected_dbg_ob1_y=14, expected_dbg_ob2_y=11, expected_dbg_ob3_y=8),
            Scenario(end=2415, expected_dbg_j=8, expected_dbg_player_x=15, expected_dbg_ob1_y=15, expected_dbg_ob2_y=12, expected_dbg_ob3_y=8),
            Scenario(end=2447, expected_dbg_j=9, expected_dbg_player_x=15, expected_dbg_ob2_y=13, expected_dbg_ob3_y=9),
            Scenario(end=2479, expected_dbg_j=10, expected_dbg_player_x=15, expected_dbg_ob1_y=1, expected_dbg_ob2_y=14, expected_dbg_ob3_y=10),
            Scenario(end=2511, expected_dbg_j=11, expected_dbg_player_x=15, expected_dbg_ob1_y=2, expected_dbg_ob2_y=15, expected_dbg_ob3_y=11),
            Scenario(end=2543, expected_dbg_j=12, expected_dbg_player_x=15, expected_dbg_ob1_y=3, expected_dbg_ob3_y=12),
            Scenario(end=2575, expected_dbg_j=13, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=1, expected_dbg_ob3_y=13),
            Scenario(end=2607, expected_dbg_j=14, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=2, expected_dbg_ob3_y=14),
            Scenario(end=2639, expected_dbg_j=15, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=3, expected_dbg_ob3_y=15),
            Scenario(end=2671, expected_dbg_j=16, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4),
            Scenario(end=2703, expected_dbg_j=17, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=1),
            Scenario(end=2735, expected_dbg_j=18, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=2),
            Scenario(end=2767, expected_dbg_j=19, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=3),
            Scenario(end=2799, expected_dbg_j=20, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=2831, expected_dbg_j=21, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=2863, expected_dbg_j=22, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=2895, expected_dbg_j=23, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=2927, expected_dbg_j=24, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=2959, expected_dbg_j=25, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=2991, expected_dbg_j=26, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3023, expected_dbg_j=27, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3055, expected_dbg_j=28, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3087, expected_dbg_j=29, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3119, expected_dbg_j=30, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3151, expected_dbg_j=31, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3183, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3215, expected_dbg_j=1, expected_dbg_player_x=15, expected_dbg_ob1_y=4, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3247, expected_dbg_j=2, expected_dbg_player_x=15, expected_dbg_ob1_y=5, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3279, expected_dbg_j=3, expected_dbg_player_x=15, expected_dbg_ob1_y=6, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3311, expected_dbg_j=4, expected_dbg_player_x=15, expected_dbg_ob1_y=7, expected_dbg_ob2_y=4, expected_dbg_ob3_y=4),
            Scenario(end=3343, expected_dbg_j=5, expected_dbg_player_x=15, expected_dbg_ob1_y=8, expected_dbg_ob2_y=5, expected_dbg_ob3_y=4),
            Scenario(end=3375, expected_dbg_j=6, expected_dbg_player_x=15, expected_dbg_ob1_y=9, expected_dbg_ob2_y=6, expected_dbg_ob3_y=4),
            Scenario(end=3407, expected_dbg_j=7, expected_dbg_player_x=15, expected_dbg_ob1_y=10, expected_dbg_ob2_y=7, expected_dbg_ob3_y=4),
            Scenario(end=3439, expected_dbg_j=8, expected_dbg_player_x=15, expected_dbg_ob1_y=11, expected_dbg_ob2_y=8, expected_dbg_ob3_y=4),
            Scenario(end=3471, expected_dbg_j=9, expected_dbg_player_x=15, expected_dbg_ob1_y=12, expected_dbg_ob2_y=9, expected_dbg_ob3_y=5),
            Scenario(end=3503, expected_dbg_j=10, expected_dbg_player_x=15, expected_dbg_ob1_y=13, expected_dbg_ob2_y=10, expected_dbg_ob3_y=6),
            Scenario(end=3535, expected_dbg_j=11, expected_dbg_player_x=15, expected_dbg_ob1_y=14, expected_dbg_ob2_y=11, expected_dbg_ob3_y=7),
            Scenario(end=3567, expected_dbg_j=12, expected_dbg_player_x=15, expected_dbg_ob1_y=15, expected_dbg_ob2_y=12, expected_dbg_ob3_y=8),
            Scenario(end=3599, expected_dbg_j=13, expected_dbg_player_x=15, expected_dbg_ob2_y=13, expected_dbg_ob3_y=9),
            Scenario(end=3631, expected_dbg_j=14, expected_dbg_player_x=15, expected_dbg_ob2_y=14, expected_dbg_ob3_y=10),
            Scenario(end=3663, expected_dbg_j=15, expected_dbg_player_x=15, expected_dbg_ob2_y=15, expected_dbg_ob3_y=11),
            Scenario(end=3695, expected_dbg_j=16, expected_dbg_player_x=15, expected_dbg_ob3_y=12),
            Scenario(end=3727, expected_dbg_j=17, expected_dbg_player_x=15, expected_dbg_ob3_y=13),
            Scenario(end=3759, expected_dbg_j=18, expected_dbg_player_x=15, expected_dbg_ob3_y=14),
            Scenario(end=3791, expected_dbg_j=19, expected_dbg_player_x=15, expected_dbg_ob3_y=15),
            Scenario(end=3823, expected_dbg_j=20, expected_dbg_player_x=15),
            Scenario(end=3855, expected_dbg_j=21, expected_dbg_player_x=15),
            Scenario(end=3887, expected_dbg_j=22, expected_dbg_player_x=15),
            Scenario(end=3919, expected_dbg_j=23, expected_dbg_player_x=15),
            Scenario(end=3951, expected_dbg_j=24, expected_dbg_player_x=15),
            Scenario(end=3983, expected_dbg_j=25, expected_dbg_player_x=15),
            Scenario(end=18446744073709551615, expected_dbg_j=26, expected_dbg_player_x=15),
        ),
    )
    # fmt: on
    position, found = scenarios.rows.first(where=lambda row: phase <= row.end)
    scenario = scenarios.rows[position]
    RST_BTN = (
        (((phase[:1] & 0) | 0) if phase < 896 else ((phase[:1] & 0) | 1))
        if phase < 928
        else (
            ((phase[:1] & 0) | 0)
            if phase < 960
            else (((phase[:1] & 0) | 1) if phase < 1024 else ((phase[:1] & 0) | 0))
        )
    )
    START = (
        (
            ((phase[:1] & 0) | 1)
            if phase < 16
            else (((phase[:1] & 0) | 0) if phase < 928 else ((phase[:1] & 0) | 1))
        )
        if phase < 960
        else (
            ((phase[:1] & 0) | 0)
            if phase < 992
            else (((phase[:1] & 0) | 1) if phase < 1024 else ((phase[:1] & 0) | 0))
        )
    )
    left = (
        (((phase[:1] & 0) | 0) if phase < 16 else ((phase[:1] & 0) | 1))
        if phase < 320
        else (
            ((phase[:1] & 0) | 0)
            if phase < 832
            else (((phase[:1] & 0) | 1) if phase < 896 else ((phase[:1] & 0) | 0))
        )
    )
    right = (
        ((phase[:1] & 0) | 0)
        if phase < 320
        else (((phase[:1] & 0) | 1) if phase < 896 else ((phase[:1] & 0) | 0))
    )
    dut = DodgeballGame(RST_BTN, START, left, right)

    expected_VGA_HS_O = (
        (((phase[:1] & 0) | 1) if phase < 65 else ((phase[:1] & 0) | 0))
        if phase < 449
        else (
            ((phase[:1] & 0) | 1)
            if phase < 3269
            else (((phase[:1] & 0) | 0) if phase < 3653 else ((phase[:1] & 0) | 1))
        )
    )
    expected_VGA_VS_O = (phase[:1] & 0) | 1
    expected_VGA_R = (phase[:4] & 0) | 0
    expected_VGA_G = (phase[:4] & 0) | 0
    expected_VGA_B = ((phase[:4] & 0) | 0) if phase < 3849 else ((phase[:4] & 0) | 8)
    expected_dbg_state = (
        (
            ((phase[:3] & 0) | 0)
            if phase < 16
            else (((phase[:3] & 0) | 1) if phase < 912 else ((phase[:3] & 0) | 0))
        )
        if phase < 944
        else (
            ((phase[:3] & 0) | 1)
            if phase < 976
            else (((phase[:3] & 0) | 0) if phase < 1008 else ((phase[:3] & 0) | 1))
        )
    )
    expected_dbg_j = scenario.expected_dbg_j
    expected_dbg_player_x = scenario.expected_dbg_player_x
    expected_dbg_ob1_x = (phase[:4] & 0) | 1
    expected_dbg_ob1_y = scenario.expected_dbg_ob1_y
    expected_dbg_ob2_x = (phase[:4] & 0) | 4
    expected_dbg_ob2_y = scenario.expected_dbg_ob2_y
    expected_dbg_ob3_x = (phase[:4] & 0) | 7
    expected_dbg_ob3_y = scenario.expected_dbg_ob3_y

    @rule
    def check_and_advance():
        if phase < 4001:
            assert dut.VGA_HS_O == expected_VGA_HS_O, "dodgeball_game: VGA_HS_O"
            assert dut.VGA_VS_O == expected_VGA_VS_O, "dodgeball_game: VGA_VS_O"
            assert dut.VGA_R == expected_VGA_R, "dodgeball_game: VGA_R"
            assert dut.VGA_G == expected_VGA_G, "dodgeball_game: VGA_G"
            assert dut.VGA_B == expected_VGA_B, "dodgeball_game: VGA_B"
            assert dut.dbg_state == expected_dbg_state, "dodgeball_game: dbg_state"
            assert dut.dbg_j == expected_dbg_j, "dodgeball_game: dbg_j"
            assert (
                dut.dbg_player_x == expected_dbg_player_x
            ), "dodgeball_game: dbg_player_x"
            assert dut.dbg_ob1_x == expected_dbg_ob1_x, "dodgeball_game: dbg_ob1_x"
            assert dut.dbg_ob1_y == expected_dbg_ob1_y, "dodgeball_game: dbg_ob1_y"
            assert dut.dbg_ob2_x == expected_dbg_ob2_x, "dodgeball_game: dbg_ob2_x"
            assert dut.dbg_ob2_y == expected_dbg_ob2_y, "dodgeball_game: dbg_ob2_y"
            assert dut.dbg_ob3_x == expected_dbg_ob3_x, "dodgeball_game: dbg_ob3_x"
            assert dut.dbg_ob3_y == expected_dbg_ob3_y, "dodgeball_game: dbg_ob3_y"

        log("info", "dodgeball_game.VGA_HS_O", dut.VGA_HS_O)
        log("info", "dodgeball_game.VGA_VS_O", dut.VGA_VS_O)
        log("info", "dodgeball_game.VGA_R", dut.VGA_R)
        log("info", "dodgeball_game.VGA_G", dut.VGA_G)
        log("info", "dodgeball_game.VGA_B", dut.VGA_B)
        log("info", "dodgeball_game.dbg_state", dut.dbg_state)
        log("info", "dodgeball_game.dbg_j", dut.dbg_j)
        log("info", "dodgeball_game.dbg_player_x", dut.dbg_player_x)
        log("info", "dodgeball_game.dbg_ob1_x", dut.dbg_ob1_x)
        log("info", "dodgeball_game.dbg_ob1_y", dut.dbg_ob1_y)
        log("info", "dodgeball_game.dbg_ob2_x", dut.dbg_ob2_x)
        log("info", "dodgeball_game.dbg_ob2_y", dut.dbg_ob2_y)
        log("info", "dodgeball_game.dbg_ob3_x", dut.dbg_ob3_x)
        log("info", "dodgeball_game.dbg_ob3_y", dut.dbg_ob3_y)

    check_and_advance()
    advance(phase)

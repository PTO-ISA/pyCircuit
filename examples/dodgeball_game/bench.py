"""Regular-clock dodgeball_game scenario; original physical-control oracles remain."""

from example_dodgeball_game.dodgeball_game import DodgeballGame
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseDodgeballGame():  # noqa: N802
    phase: bits[64] = 0
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
    expected_dbg_j = (
        (
            (
                (
                    (
                        (
                            ((phase[:5] & 0) | 0)
                            if phase < 48
                            else (
                                ((phase[:5] & 0) | 1)
                                if phase < 80
                                else ((phase[:5] & 0) | 2)
                            )
                        )
                        if phase < 112
                        else (
                            (
                                ((phase[:5] & 0) | 3)
                                if phase < 144
                                else ((phase[:5] & 0) | 4)
                            )
                            if phase < 176
                            else (
                                ((phase[:5] & 0) | 5)
                                if phase < 208
                                else ((phase[:5] & 0) | 6)
                            )
                        )
                    )
                    if phase < 240
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 7)
                                if phase < 272
                                else ((phase[:5] & 0) | 8)
                            )
                            if phase < 304
                            else (
                                ((phase[:5] & 0) | 9)
                                if phase < 336
                                else ((phase[:5] & 0) | 10)
                            )
                        )
                        if phase < 368
                        else (
                            (
                                ((phase[:5] & 0) | 11)
                                if phase < 400
                                else ((phase[:5] & 0) | 12)
                            )
                            if phase < 432
                            else (
                                ((phase[:5] & 0) | 13)
                                if phase < 464
                                else ((phase[:5] & 0) | 14)
                            )
                        )
                    )
                )
                if phase < 496
                else (
                    (
                        (
                            ((phase[:5] & 0) | 15)
                            if phase < 528
                            else (
                                ((phase[:5] & 0) | 16)
                                if phase < 560
                                else ((phase[:5] & 0) | 17)
                            )
                        )
                        if phase < 592
                        else (
                            (
                                ((phase[:5] & 0) | 18)
                                if phase < 624
                                else ((phase[:5] & 0) | 19)
                            )
                            if phase < 656
                            else (
                                ((phase[:5] & 0) | 20)
                                if phase < 688
                                else ((phase[:5] & 0) | 21)
                            )
                        )
                    )
                    if phase < 720
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 22)
                                if phase < 752
                                else ((phase[:5] & 0) | 23)
                            )
                            if phase < 784
                            else (
                                ((phase[:5] & 0) | 24)
                                if phase < 816
                                else ((phase[:5] & 0) | 25)
                            )
                        )
                        if phase < 848
                        else (
                            (
                                ((phase[:5] & 0) | 26)
                                if phase < 880
                                else ((phase[:5] & 0) | 27)
                            )
                            if phase < 912
                            else (
                                ((phase[:5] & 0) | 28)
                                if phase < 976
                                else ((phase[:5] & 0) | 29)
                            )
                        )
                    )
                )
            )
            if phase < 1040
            else (
                (
                    (
                        (
                            ((phase[:5] & 0) | 30)
                            if phase < 1072
                            else (
                                ((phase[:5] & 0) | 31)
                                if phase < 1104
                                else ((phase[:5] & 0) | 0)
                            )
                        )
                        if phase < 1136
                        else (
                            (
                                ((phase[:5] & 0) | 1)
                                if phase < 1168
                                else ((phase[:5] & 0) | 2)
                            )
                            if phase < 1200
                            else (
                                ((phase[:5] & 0) | 3)
                                if phase < 1232
                                else ((phase[:5] & 0) | 4)
                            )
                        )
                    )
                    if phase < 1264
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 5)
                                if phase < 1296
                                else ((phase[:5] & 0) | 6)
                            )
                            if phase < 1328
                            else (
                                ((phase[:5] & 0) | 7)
                                if phase < 1360
                                else ((phase[:5] & 0) | 8)
                            )
                        )
                        if phase < 1392
                        else (
                            (
                                ((phase[:5] & 0) | 9)
                                if phase < 1424
                                else ((phase[:5] & 0) | 10)
                            )
                            if phase < 1456
                            else (
                                ((phase[:5] & 0) | 11)
                                if phase < 1488
                                else ((phase[:5] & 0) | 12)
                            )
                        )
                    )
                )
                if phase < 1520
                else (
                    (
                        (
                            (
                                ((phase[:5] & 0) | 13)
                                if phase < 1552
                                else ((phase[:5] & 0) | 14)
                            )
                            if phase < 1584
                            else (
                                ((phase[:5] & 0) | 15)
                                if phase < 1616
                                else ((phase[:5] & 0) | 16)
                            )
                        )
                        if phase < 1648
                        else (
                            (
                                ((phase[:5] & 0) | 17)
                                if phase < 1680
                                else ((phase[:5] & 0) | 18)
                            )
                            if phase < 1712
                            else (
                                ((phase[:5] & 0) | 19)
                                if phase < 1744
                                else ((phase[:5] & 0) | 20)
                            )
                        )
                    )
                    if phase < 1776
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 21)
                                if phase < 1808
                                else ((phase[:5] & 0) | 22)
                            )
                            if phase < 1840
                            else (
                                ((phase[:5] & 0) | 23)
                                if phase < 1872
                                else ((phase[:5] & 0) | 24)
                            )
                        )
                        if phase < 1904
                        else (
                            (
                                ((phase[:5] & 0) | 25)
                                if phase < 1936
                                else ((phase[:5] & 0) | 26)
                            )
                            if phase < 1968
                            else (
                                ((phase[:5] & 0) | 27)
                                if phase < 2000
                                else ((phase[:5] & 0) | 28)
                            )
                        )
                    )
                )
            )
        )
        if phase < 2032
        else (
            (
                (
                    (
                        (
                            ((phase[:5] & 0) | 29)
                            if phase < 2064
                            else (
                                ((phase[:5] & 0) | 30)
                                if phase < 2096
                                else ((phase[:5] & 0) | 31)
                            )
                        )
                        if phase < 2128
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 2160
                                else ((phase[:5] & 0) | 1)
                            )
                            if phase < 2192
                            else (
                                ((phase[:5] & 0) | 2)
                                if phase < 2224
                                else ((phase[:5] & 0) | 3)
                            )
                        )
                    )
                    if phase < 2256
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 4)
                                if phase < 2288
                                else ((phase[:5] & 0) | 5)
                            )
                            if phase < 2320
                            else (
                                ((phase[:5] & 0) | 6)
                                if phase < 2352
                                else ((phase[:5] & 0) | 7)
                            )
                        )
                        if phase < 2384
                        else (
                            (
                                ((phase[:5] & 0) | 8)
                                if phase < 2416
                                else ((phase[:5] & 0) | 9)
                            )
                            if phase < 2448
                            else (
                                ((phase[:5] & 0) | 10)
                                if phase < 2480
                                else ((phase[:5] & 0) | 11)
                            )
                        )
                    )
                )
                if phase < 2512
                else (
                    (
                        (
                            (
                                ((phase[:5] & 0) | 12)
                                if phase < 2544
                                else ((phase[:5] & 0) | 13)
                            )
                            if phase < 2576
                            else (
                                ((phase[:5] & 0) | 14)
                                if phase < 2608
                                else ((phase[:5] & 0) | 15)
                            )
                        )
                        if phase < 2640
                        else (
                            (
                                ((phase[:5] & 0) | 16)
                                if phase < 2672
                                else ((phase[:5] & 0) | 17)
                            )
                            if phase < 2704
                            else (
                                ((phase[:5] & 0) | 18)
                                if phase < 2736
                                else ((phase[:5] & 0) | 19)
                            )
                        )
                    )
                    if phase < 2768
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 20)
                                if phase < 2800
                                else ((phase[:5] & 0) | 21)
                            )
                            if phase < 2832
                            else (
                                ((phase[:5] & 0) | 22)
                                if phase < 2864
                                else ((phase[:5] & 0) | 23)
                            )
                        )
                        if phase < 2896
                        else (
                            (
                                ((phase[:5] & 0) | 24)
                                if phase < 2928
                                else ((phase[:5] & 0) | 25)
                            )
                            if phase < 2960
                            else (
                                ((phase[:5] & 0) | 26)
                                if phase < 2992
                                else ((phase[:5] & 0) | 27)
                            )
                        )
                    )
                )
            )
            if phase < 3024
            else (
                (
                    (
                        (
                            ((phase[:5] & 0) | 28)
                            if phase < 3056
                            else (
                                ((phase[:5] & 0) | 29)
                                if phase < 3088
                                else ((phase[:5] & 0) | 30)
                            )
                        )
                        if phase < 3120
                        else (
                            (
                                ((phase[:5] & 0) | 31)
                                if phase < 3152
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 3184
                            else (
                                ((phase[:5] & 0) | 1)
                                if phase < 3216
                                else ((phase[:5] & 0) | 2)
                            )
                        )
                    )
                    if phase < 3248
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 3)
                                if phase < 3280
                                else ((phase[:5] & 0) | 4)
                            )
                            if phase < 3312
                            else (
                                ((phase[:5] & 0) | 5)
                                if phase < 3344
                                else ((phase[:5] & 0) | 6)
                            )
                        )
                        if phase < 3376
                        else (
                            (
                                ((phase[:5] & 0) | 7)
                                if phase < 3408
                                else ((phase[:5] & 0) | 8)
                            )
                            if phase < 3440
                            else (
                                ((phase[:5] & 0) | 9)
                                if phase < 3472
                                else ((phase[:5] & 0) | 10)
                            )
                        )
                    )
                )
                if phase < 3504
                else (
                    (
                        (
                            (
                                ((phase[:5] & 0) | 11)
                                if phase < 3536
                                else ((phase[:5] & 0) | 12)
                            )
                            if phase < 3568
                            else (
                                ((phase[:5] & 0) | 13)
                                if phase < 3600
                                else ((phase[:5] & 0) | 14)
                            )
                        )
                        if phase < 3632
                        else (
                            (
                                ((phase[:5] & 0) | 15)
                                if phase < 3664
                                else ((phase[:5] & 0) | 16)
                            )
                            if phase < 3696
                            else (
                                ((phase[:5] & 0) | 17)
                                if phase < 3728
                                else ((phase[:5] & 0) | 18)
                            )
                        )
                    )
                    if phase < 3760
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 19)
                                if phase < 3792
                                else ((phase[:5] & 0) | 20)
                            )
                            if phase < 3824
                            else (
                                ((phase[:5] & 0) | 21)
                                if phase < 3856
                                else ((phase[:5] & 0) | 22)
                            )
                        )
                        if phase < 3888
                        else (
                            (
                                ((phase[:5] & 0) | 23)
                                if phase < 3920
                                else ((phase[:5] & 0) | 24)
                            )
                            if phase < 3952
                            else (
                                ((phase[:5] & 0) | 25)
                                if phase < 3984
                                else ((phase[:5] & 0) | 26)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_dbg_player_x = (
        (
            (
                (
                    ((phase[:4] & 0) | 8)
                    if phase < 48
                    else (
                        ((phase[:4] & 0) | 7) if phase < 80 else ((phase[:4] & 0) | 6)
                    )
                )
                if phase < 112
                else (
                    ((phase[:4] & 0) | 5)
                    if phase < 144
                    else (
                        ((phase[:4] & 0) | 4) if phase < 176 else ((phase[:4] & 0) | 3)
                    )
                )
            )
            if phase < 208
            else (
                (
                    ((phase[:4] & 0) | 2)
                    if phase < 240
                    else (
                        ((phase[:4] & 0) | 1) if phase < 272 else ((phase[:4] & 0) | 0)
                    )
                )
                if phase < 336
                else (
                    ((phase[:4] & 0) | 1)
                    if phase < 368
                    else (
                        ((phase[:4] & 0) | 2) if phase < 400 else ((phase[:4] & 0) | 3)
                    )
                )
            )
        )
        if phase < 432
        else (
            (
                (
                    ((phase[:4] & 0) | 4)
                    if phase < 464
                    else (
                        ((phase[:4] & 0) | 5) if phase < 496 else ((phase[:4] & 0) | 6)
                    )
                )
                if phase < 528
                else (
                    ((phase[:4] & 0) | 7)
                    if phase < 560
                    else (
                        ((phase[:4] & 0) | 8) if phase < 592 else ((phase[:4] & 0) | 9)
                    )
                )
            )
            if phase < 624
            else (
                (
                    ((phase[:4] & 0) | 10)
                    if phase < 656
                    else (
                        ((phase[:4] & 0) | 11)
                        if phase < 688
                        else ((phase[:4] & 0) | 12)
                    )
                )
                if phase < 720
                else (
                    ((phase[:4] & 0) | 13)
                    if phase < 752
                    else (
                        ((phase[:4] & 0) | 14)
                        if phase < 784
                        else ((phase[:4] & 0) | 15)
                    )
                )
            )
        )
    )
    expected_dbg_ob1_x = (phase[:4] & 0) | 1
    expected_dbg_ob1_y = (
        (
            (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 80
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 112
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                    if phase < 144
                    else (
                        ((phase[:4] & 0) | 3)
                        if phase < 176
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 208
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                )
                if phase < 240
                else (
                    (
                        ((phase[:4] & 0) | 6)
                        if phase < 272
                        else (
                            ((phase[:4] & 0) | 7)
                            if phase < 304
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                    if phase < 336
                    else (
                        ((phase[:4] & 0) | 9)
                        if phase < 368
                        else (
                            ((phase[:4] & 0) | 10)
                            if phase < 400
                            else ((phase[:4] & 0) | 11)
                        )
                    )
                )
            )
            if phase < 432
            else (
                (
                    (
                        ((phase[:4] & 0) | 12)
                        if phase < 1168
                        else (
                            ((phase[:4] & 0) | 13)
                            if phase < 1200
                            else ((phase[:4] & 0) | 14)
                        )
                    )
                    if phase < 1232
                    else (
                        ((phase[:4] & 0) | 15)
                        if phase < 1264
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 1296
                            else ((phase[:4] & 0) | 1)
                        )
                    )
                )
                if phase < 1328
                else (
                    (
                        ((phase[:4] & 0) | 2)
                        if phase < 1360
                        else (
                            ((phase[:4] & 0) | 3)
                            if phase < 1392
                            else ((phase[:4] & 0) | 4)
                        )
                    )
                    if phase < 1424
                    else (
                        ((phase[:4] & 0) | 5)
                        if phase < 1456
                        else (
                            ((phase[:4] & 0) | 6)
                            if phase < 1488
                            else ((phase[:4] & 0) | 7)
                        )
                    )
                )
            )
        )
        if phase < 1520
        else (
            (
                (
                    (
                        ((phase[:4] & 0) | 8)
                        if phase < 2192
                        else (
                            ((phase[:4] & 0) | 9)
                            if phase < 2224
                            else ((phase[:4] & 0) | 10)
                        )
                    )
                    if phase < 2256
                    else (
                        ((phase[:4] & 0) | 11)
                        if phase < 2288
                        else (
                            ((phase[:4] & 0) | 12)
                            if phase < 2320
                            else ((phase[:4] & 0) | 13)
                        )
                    )
                )
                if phase < 2352
                else (
                    (
                        ((phase[:4] & 0) | 14)
                        if phase < 2384
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 2416
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 2448
                    else (
                        ((phase[:4] & 0) | 1)
                        if phase < 2480
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 2512
                            else ((phase[:4] & 0) | 3)
                        )
                    )
                )
            )
            if phase < 2544
            else (
                (
                    (
                        ((phase[:4] & 0) | 4)
                        if phase < 3216
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 3248
                            else ((phase[:4] & 0) | 6)
                        )
                    )
                    if phase < 3280
                    else (
                        ((phase[:4] & 0) | 7)
                        if phase < 3312
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 3344
                            else ((phase[:4] & 0) | 9)
                        )
                    )
                )
                if phase < 3376
                else (
                    (
                        ((phase[:4] & 0) | 10)
                        if phase < 3408
                        else (
                            ((phase[:4] & 0) | 11)
                            if phase < 3440
                            else ((phase[:4] & 0) | 12)
                        )
                    )
                    if phase < 3472
                    else (
                        (
                            ((phase[:4] & 0) | 13)
                            if phase < 3504
                            else ((phase[:4] & 0) | 14)
                        )
                        if phase < 3536
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 3568
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
        )
    )
    expected_dbg_ob2_x = (phase[:4] & 0) | 4
    expected_dbg_ob2_y = (
        (
            (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 176
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 208
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                    if phase < 240
                    else (
                        ((phase[:4] & 0) | 3)
                        if phase < 272
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 304
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                )
                if phase < 336
                else (
                    (
                        ((phase[:4] & 0) | 6)
                        if phase < 368
                        else (
                            ((phase[:4] & 0) | 7)
                            if phase < 400
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                    if phase < 432
                    else (
                        ((phase[:4] & 0) | 9)
                        if phase < 464
                        else (
                            ((phase[:4] & 0) | 10)
                            if phase < 496
                            else ((phase[:4] & 0) | 11)
                        )
                    )
                )
            )
            if phase < 528
            else (
                (
                    (
                        ((phase[:4] & 0) | 12)
                        if phase < 1264
                        else (
                            ((phase[:4] & 0) | 13)
                            if phase < 1296
                            else ((phase[:4] & 0) | 14)
                        )
                    )
                    if phase < 1328
                    else (
                        ((phase[:4] & 0) | 15)
                        if phase < 1360
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 1392
                            else ((phase[:4] & 0) | 1)
                        )
                    )
                )
                if phase < 1424
                else (
                    (
                        ((phase[:4] & 0) | 2)
                        if phase < 1456
                        else (
                            ((phase[:4] & 0) | 3)
                            if phase < 1488
                            else ((phase[:4] & 0) | 4)
                        )
                    )
                    if phase < 1520
                    else (
                        ((phase[:4] & 0) | 5)
                        if phase < 1552
                        else (
                            ((phase[:4] & 0) | 6)
                            if phase < 1584
                            else ((phase[:4] & 0) | 7)
                        )
                    )
                )
            )
        )
        if phase < 1616
        else (
            (
                (
                    (
                        ((phase[:4] & 0) | 8)
                        if phase < 2288
                        else (
                            ((phase[:4] & 0) | 9)
                            if phase < 2320
                            else ((phase[:4] & 0) | 10)
                        )
                    )
                    if phase < 2352
                    else (
                        ((phase[:4] & 0) | 11)
                        if phase < 2384
                        else (
                            ((phase[:4] & 0) | 12)
                            if phase < 2416
                            else ((phase[:4] & 0) | 13)
                        )
                    )
                )
                if phase < 2448
                else (
                    (
                        ((phase[:4] & 0) | 14)
                        if phase < 2480
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 2512
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 2544
                    else (
                        ((phase[:4] & 0) | 1)
                        if phase < 2576
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 2608
                            else ((phase[:4] & 0) | 3)
                        )
                    )
                )
            )
            if phase < 2640
            else (
                (
                    (
                        ((phase[:4] & 0) | 4)
                        if phase < 3312
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 3344
                            else ((phase[:4] & 0) | 6)
                        )
                    )
                    if phase < 3376
                    else (
                        ((phase[:4] & 0) | 7)
                        if phase < 3408
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 3440
                            else ((phase[:4] & 0) | 9)
                        )
                    )
                )
                if phase < 3472
                else (
                    (
                        ((phase[:4] & 0) | 10)
                        if phase < 3504
                        else (
                            ((phase[:4] & 0) | 11)
                            if phase < 3536
                            else ((phase[:4] & 0) | 12)
                        )
                    )
                    if phase < 3568
                    else (
                        (
                            ((phase[:4] & 0) | 13)
                            if phase < 3600
                            else ((phase[:4] & 0) | 14)
                        )
                        if phase < 3632
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 3664
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
        )
    )
    expected_dbg_ob3_x = (phase[:4] & 0) | 7
    expected_dbg_ob3_y = (
        (
            (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 304
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 336
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                    if phase < 368
                    else (
                        ((phase[:4] & 0) | 3)
                        if phase < 400
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 432
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                )
                if phase < 464
                else (
                    (
                        ((phase[:4] & 0) | 6)
                        if phase < 496
                        else (
                            ((phase[:4] & 0) | 7)
                            if phase < 528
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                    if phase < 560
                    else (
                        ((phase[:4] & 0) | 9)
                        if phase < 592
                        else (
                            ((phase[:4] & 0) | 10)
                            if phase < 624
                            else ((phase[:4] & 0) | 11)
                        )
                    )
                )
            )
            if phase < 656
            else (
                (
                    (
                        ((phase[:4] & 0) | 12)
                        if phase < 1392
                        else (
                            ((phase[:4] & 0) | 13)
                            if phase < 1424
                            else ((phase[:4] & 0) | 14)
                        )
                    )
                    if phase < 1456
                    else (
                        ((phase[:4] & 0) | 15)
                        if phase < 1488
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 1520
                            else ((phase[:4] & 0) | 1)
                        )
                    )
                )
                if phase < 1552
                else (
                    (
                        ((phase[:4] & 0) | 2)
                        if phase < 1584
                        else (
                            ((phase[:4] & 0) | 3)
                            if phase < 1616
                            else ((phase[:4] & 0) | 4)
                        )
                    )
                    if phase < 1648
                    else (
                        ((phase[:4] & 0) | 5)
                        if phase < 1680
                        else (
                            ((phase[:4] & 0) | 6)
                            if phase < 1712
                            else ((phase[:4] & 0) | 7)
                        )
                    )
                )
            )
        )
        if phase < 1744
        else (
            (
                (
                    (
                        ((phase[:4] & 0) | 8)
                        if phase < 2416
                        else (
                            ((phase[:4] & 0) | 9)
                            if phase < 2448
                            else ((phase[:4] & 0) | 10)
                        )
                    )
                    if phase < 2480
                    else (
                        ((phase[:4] & 0) | 11)
                        if phase < 2512
                        else (
                            ((phase[:4] & 0) | 12)
                            if phase < 2544
                            else ((phase[:4] & 0) | 13)
                        )
                    )
                )
                if phase < 2576
                else (
                    (
                        ((phase[:4] & 0) | 14)
                        if phase < 2608
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 2640
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 2672
                    else (
                        ((phase[:4] & 0) | 1)
                        if phase < 2704
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 2736
                            else ((phase[:4] & 0) | 3)
                        )
                    )
                )
            )
            if phase < 2768
            else (
                (
                    (
                        ((phase[:4] & 0) | 4)
                        if phase < 3440
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 3472
                            else ((phase[:4] & 0) | 6)
                        )
                    )
                    if phase < 3504
                    else (
                        ((phase[:4] & 0) | 7)
                        if phase < 3536
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 3568
                            else ((phase[:4] & 0) | 9)
                        )
                    )
                )
                if phase < 3600
                else (
                    (
                        ((phase[:4] & 0) | 10)
                        if phase < 3632
                        else (
                            ((phase[:4] & 0) | 11)
                            if phase < 3664
                            else ((phase[:4] & 0) | 12)
                        )
                    )
                    if phase < 3696
                    else (
                        (
                            ((phase[:4] & 0) | 13)
                            if phase < 3728
                            else ((phase[:4] & 0) | 14)
                        )
                        if phase < 3760
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 3792
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
        )
    )

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

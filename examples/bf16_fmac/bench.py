"""Regular-clock bf16_fmac scenario; original physical-control oracles remain."""

from example_bf16_fmac.bf16_fmac import BF16Fmac
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseBF16Fmac():  # noqa: N802
    phase: bits[64] = 0
    a_in = (
        (
            (
                (
                    (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 1
                            else ((phase[:16] & 0) | 16256)
                        )
                        if phase < 2
                        else (
                            ((phase[:16] & 0) | 49024)
                            if phase < 7
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                    if phase < 9
                    else (
                        (
                            ((phase[:16] & 0) | 32768)
                            if phase < 10
                            else ((phase[:16] & 0) | 16256)
                        )
                        if phase < 11
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 13
                            else ((phase[:16] & 0) | 32768)
                        )
                    )
                )
                if phase < 14
                else (
                    (
                        (
                            ((phase[:16] & 0) | 127)
                            if phase < 15
                            else ((phase[:16] & 0) | 16320)
                        )
                        if phase < 16
                        else (
                            ((phase[:16] & 0) | 16256)
                            if phase < 18
                            else ((phase[:16] & 0) | 49024)
                        )
                    )
                    if phase < 19
                    else (
                        (
                            ((phase[:16] & 0) | 16256)
                            if phase < 22
                            else ((phase[:16] & 0) | 49024)
                        )
                        if phase < 23
                        else (
                            ((phase[:16] & 0) | 16320)
                            if phase < 24
                            else ((phase[:16] & 0) | 16383)
                        )
                    )
                )
            )
            if phase < 25
            else (
                (
                    (
                        (
                            ((phase[:16] & 0) | 16256)
                            if phase < 26
                            else ((phase[:16] & 0) | 16257)
                        )
                        if phase < 27
                        else (
                            ((phase[:16] & 0) | 16256)
                            if phase < 30
                            else ((phase[:16] & 0) | 13350)
                        )
                    )
                    if phase < 31
                    else (
                        (
                            ((phase[:16] & 0) | 128)
                            if phase < 32
                            else ((phase[:16] & 0) | 32512)
                        )
                        if phase < 33
                        else (
                            ((phase[:16] & 0) | 16256)
                            if phase < 35
                            else ((phase[:16] & 0) | 17010)
                        )
                    )
                )
                if phase < 36
                else (
                    (
                        (
                            ((phase[:16] & 0) | 11520)
                            if phase < 37
                            else ((phase[:16] & 0) | 45203)
                        )
                        if phase < 38
                        else (
                            ((phase[:16] & 0) | 13350)
                            if phase < 39
                            else ((phase[:16] & 0) | 47033)
                        )
                    )
                    if phase < 40
                    else (
                        (
                            ((phase[:16] & 0) | 20542)
                            if phase < 41
                            else ((phase[:16] & 0) | 15180)
                        )
                        if phase < 42
                        else (
                            ((phase[:16] & 0) | 48863)
                            if phase < 43
                            else ((phase[:16] & 0) | 17010)
                        )
                    )
                )
            )
        )
        if phase < 44
        else (
            (
                (
                    (
                        (
                            ((phase[:16] & 0) | 50565)
                            if phase < 45
                            else ((phase[:16] & 0) | 14986)
                        )
                        if phase < 46
                        else (
                            ((phase[:16] & 0) | 18712)
                            if phase < 47
                            else ((phase[:16] & 0) | 52395)
                        )
                    )
                    if phase < 48
                    else (
                        (
                            ((phase[:16] & 0) | 20542)
                            if phase < 49
                            else ((phase[:16] & 0) | 45137)
                        )
                        if phase < 50
                        else (
                            ((phase[:16] & 0) | 18646)
                            if phase < 51
                            else ((phase[:16] & 0) | 13284)
                        )
                    )
                )
                if phase < 52
                else (
                    (
                        (
                            ((phase[:16] & 0) | 46967)
                            if phase < 53
                            else ((phase[:16] & 0) | 14986)
                        )
                        if phase < 54
                        else (
                            ((phase[:16] & 0) | 48669)
                            if phase < 55
                            else ((phase[:16] & 0) | 13090)
                        )
                    )
                    if phase < 56
                    else (
                        (
                            ((phase[:16] & 0) | 16816)
                            if phase < 57
                            else ((phase[:16] & 0) | 50499)
                        )
                        if phase < 58
                        else (
                            ((phase[:16] & 0) | 18646)
                            if phase < 59
                            else ((phase[:16] & 0) | 52329)
                        )
                    )
                )
            )
            if phase < 60
            else (
                (
                    (
                        (
                            ((phase[:16] & 0) | 16750)
                            if phase < 61
                            else ((phase[:16] & 0) | 20476)
                        )
                        if phase < 62
                        else (
                            ((phase[:16] & 0) | 44943)
                            if phase < 63
                            else ((phase[:16] & 0) | 13090)
                        )
                    )
                    if phase < 64
                    else (
                        (
                            ((phase[:16] & 0) | 46773)
                            if phase < 65
                            else ((phase[:16] & 0) | 20282)
                        )
                        if phase < 66
                        else (
                            ((phase[:16] & 0) | 14920)
                            if phase < 67
                            else ((phase[:16] & 0) | 48603)
                        )
                    )
                )
                if phase < 68
                else (
                    (
                        (
                            ((phase[:16] & 0) | 16750)
                            if phase < 69
                            else ((phase[:16] & 0) | 50305)
                        )
                        if phase < 70
                        else (
                            ((phase[:16] & 0) | 32768)
                            if phase < 71
                            else ((phase[:16] & 0) | 18452)
                        )
                    )
                    if phase < 72
                    else (
                        (
                            ((phase[:16] & 0) | 52135)
                            if phase < 73
                            else ((phase[:16] & 0) | 20282)
                        )
                        if phase < 74
                        else (
                            ((phase[:16] & 0) | 44877)
                            if phase < 75
                            else (
                                ((phase[:16] & 0) | 49024)
                                if phase < 76
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    b_in = (
        (
            (
                (
                    (
                        ((phase[:16] & 0) | 0)
                        if phase < 1
                        else (
                            ((phase[:16] & 0) | 16256)
                            if phase < 7
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                    if phase < 8
                    else (
                        (
                            ((phase[:16] & 0) | 16256)
                            if phase < 11
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 12
                        else (
                            ((phase[:16] & 0) | 16256)
                            if phase < 14
                            else ((phase[:16] & 0) | 16383)
                        )
                    )
                )
                if phase < 15
                else (
                    (
                        (
                            ((phase[:16] & 0) | 16320)
                            if phase < 16
                            else ((phase[:16] & 0) | 16256)
                        )
                        if phase < 19
                        else (
                            ((phase[:16] & 0) | 49024)
                            if phase < 20
                            else ((phase[:16] & 0) | 16256)
                        )
                    )
                    if phase < 22
                    else (
                        (
                            ((phase[:16] & 0) | 49024)
                            if phase < 23
                            else ((phase[:16] & 0) | 16320)
                        )
                        if phase < 24
                        else (
                            ((phase[:16] & 0) | 16383)
                            if phase < 25
                            else ((phase[:16] & 0) | 16256)
                        )
                    )
                )
            )
            if phase < 26
            else (
                (
                    (
                        (
                            ((phase[:16] & 0) | 16257)
                            if phase < 27
                            else ((phase[:16] & 0) | 16256)
                        )
                        if phase < 30
                        else (
                            ((phase[:16] & 0) | 47178)
                            if phase < 31
                            else ((phase[:16] & 0) | 128)
                        )
                    )
                    if phase < 32
                    else (
                        (
                            ((phase[:16] & 0) | 32512)
                            if phase < 33
                            else ((phase[:16] & 0) | 16256)
                        )
                        if phase < 35
                        else (
                            ((phase[:16] & 0) | 52830)
                            if phase < 36
                            else ((phase[:16] & 0) | 11520)
                        )
                    )
                )
                if phase < 37
                else (
                    (
                        (
                            ((phase[:16] & 0) | 12965)
                            if phase < 38
                            else ((phase[:16] & 0) | 47178)
                        )
                        if phase < 39
                        else (
                            ((phase[:16] & 0) | 48623)
                            if phase < 40
                            else ((phase[:16] & 0) | 49394)
                        )
                    )
                    if phase < 41
                    else (
                        (
                            ((phase[:16] & 0) | 17172)
                            if phase < 42
                            else ((phase[:16] & 0) | 18617)
                        )
                        if phase < 43
                        else (
                            ((phase[:16] & 0) | 52830)
                            if phase < 44
                            else ((phase[:16] & 0) | 45059)
                        )
                    )
                )
            )
        )
        if phase < 45
        else (
            (
                (
                    (
                        (
                            ((phase[:16] & 0) | 45830)
                            if phase < 46
                            else ((phase[:16] & 0) | 13736)
                        )
                        if phase < 47
                        else (
                            ((phase[:16] & 0) | 15181)
                            if phase < 48
                            else ((phase[:16] & 0) | 49394)
                        )
                    )
                    if phase < 49
                    else (
                        (
                            ((phase[:16] & 0) | 50711)
                            if phase < 50
                            else ((phase[:16] & 0) | 51482)
                        )
                        if phase < 51
                        else (
                            ((phase[:16] & 0) | 19388)
                            if phase < 52
                            else ((phase[:16] & 0) | 11745)
                        )
                    )
                )
                if phase < 53
                else (
                    (
                        (
                            ((phase[:16] & 0) | 45830)
                            if phase < 54
                            else ((phase[:16] & 0) | 47275)
                        )
                        if phase < 55
                        else (
                            ((phase[:16] & 0) | 48046)
                            if phase < 56
                            else ((phase[:16] & 0) | 15952)
                        )
                    )
                    if phase < 57
                    else (
                        (
                            ((phase[:16] & 0) | 17397)
                            if phase < 58
                            else ((phase[:16] & 0) | 51482)
                        )
                        if phase < 59
                        else (
                            ((phase[:16] & 0) | 52927)
                            if phase < 60
                            else ((phase[:16] & 0) | 44610)
                        )
                    )
                )
            )
            if phase < 61
            else (
                (
                    (
                        (
                            ((phase[:16] & 0) | 12516)
                            if phase < 62
                            else ((phase[:16] & 0) | 13833)
                        )
                        if phase < 63
                        else (
                            ((phase[:16] & 0) | 48046)
                            if phase < 64
                            else ((phase[:16] & 0) | 49491)
                        )
                    )
                    if phase < 65
                    else (
                        (
                            ((phase[:16] & 0) | 50262)
                            if phase < 66
                            else ((phase[:16] & 0) | 18168)
                        )
                        if phase < 67
                        else (
                            ((phase[:16] & 0) | 19485)
                            if phase < 68
                            else ((phase[:16] & 0) | 44610)
                        )
                    )
                )
                if phase < 69
                else (
                    (
                        (
                            ((phase[:16] & 0) | 46055)
                            if phase < 70
                            else ((phase[:16] & 0) | 16256)
                        )
                        if phase < 71
                        else (
                            ((phase[:16] & 0) | 14604)
                            if phase < 72
                            else ((phase[:16] & 0) | 16049)
                        )
                    )
                    if phase < 73
                    else (
                        (
                            ((phase[:16] & 0) | 50262)
                            if phase < 74
                            else ((phase[:16] & 0) | 51707)
                        )
                        if phase < 75
                        else (
                            ((phase[:16] & 0) | 16256)
                            if phase < 76
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                )
            )
        )
    )
    acc_in = (
        (
            (
                (
                    (
                        (
                            ((phase[:32] & 0) | 0)
                            if phase < 8
                            else ((phase[:32] & 0) | 1065427781)
                        )
                        if phase < 9
                        else (
                            ((phase[:32] & 0) | 3221225472)
                            if phase < 10
                            else ((phase[:32] & 0) | 1065353216)
                        )
                    )
                    if phase < 11
                    else (
                        (
                            ((phase[:32] & 0) | 0)
                            if phase < 12
                            else ((phase[:32] & 0) | 1065427781)
                        )
                        if phase < 13
                        else (
                            ((phase[:32] & 0) | 3221225472)
                            if phase < 14
                            else ((phase[:32] & 0) | 3212836864)
                        )
                    )
                )
                if phase < 15
                else (
                    (
                        (
                            ((phase[:32] & 0) | 0)
                            if phase < 17
                            else ((phase[:32] & 0) | 1065353216)
                        )
                        if phase < 18
                        else (
                            ((phase[:32] & 0) | 0)
                            if phase < 19
                            else ((phase[:32] & 0) | 1065353216)
                        )
                    )
                    if phase < 20
                    else (
                        (
                            ((phase[:32] & 0) | 1283457024)
                            if phase < 21
                            else ((phase[:32] & 0) | 3221225472)
                        )
                        if phase < 22
                        else (
                            ((phase[:32] & 0) | 3212836864)
                            if phase < 23
                            else ((phase[:32] & 0) | 0)
                        )
                    )
                )
            )
            if phase < 25
            else (
                (
                    (
                        (
                            ((phase[:32] & 0) | 8388607)
                            if phase < 26
                            else ((phase[:32] & 0) | 0)
                        )
                        if phase < 27
                        else (
                            ((phase[:32] & 0) | 1275068416)
                            if phase < 28
                            else ((phase[:32] & 0) | 1283457024)
                        )
                    )
                    if phase < 29
                    else (
                        (
                            ((phase[:32] & 0) | 1291845632)
                            if phase < 30
                            else ((phase[:32] & 0) | 980349794)
                        )
                        if phase < 31
                        else (
                            ((phase[:32] & 0) | 0)
                            if phase < 33
                            else ((phase[:32] & 0) | 8388607)
                        )
                    )
                )
                if phase < 34
                else (
                    (
                        (
                            ((phase[:32] & 0) | 2147483648)
                            if phase < 35
                            else ((phase[:32] & 0) | 2966215206)
                        )
                        if phase < 36
                        else (
                            ((phase[:32] & 0) | 754974720)
                            if phase < 37
                            else ((phase[:32] & 0) | 867662257)
                        )
                    )
                    if phase < 38
                    else (
                        (
                            ((phase[:32] & 0) | 980349794)
                            if phase < 39
                            else ((phase[:32] & 0) | 1084648723)
                        )
                        if phase < 40
                        else (
                            ((phase[:32] & 0) | 1252704490)
                            if phase < 41
                            else (
                                ((phase[:32] & 0) | 3344819908)
                                if phase < 42
                                else ((phase[:32] & 0) | 3449118837)
                            )
                        )
                    )
                )
            )
        )
        if phase < 43
        else (
            (
                (
                    (
                        (
                            ((phase[:32] & 0) | 2966215206)
                            if phase < 44
                            else ((phase[:32] & 0) | 3070514135)
                        )
                        if phase < 45
                        else (
                            ((phase[:32] & 0) | 3238569902)
                            if phase < 46
                            else ((phase[:32] & 0) | 1035718024)
                        )
                    )
                    if phase < 47
                    else (
                        (
                            ((phase[:32] & 0) | 1148405561)
                            if phase < 48
                            else ((phase[:32] & 0) | 1252704490)
                        )
                        if phase < 49
                        else (
                            ((phase[:32] & 0) | 769800859)
                            if phase < 50
                            else ((phase[:32] & 0) | 937856626)
                        )
                    )
                )
                if phase < 51
                else (
                    (
                        (
                            ((phase[:32] & 0) | 3021583436)
                            if phase < 52
                            else ((phase[:32] & 0) | 3134270973)
                        )
                        if phase < 53
                        else (
                            ((phase[:32] & 0) | 3238569902)
                            if phase < 54
                            else ((phase[:32] & 0) | 3351257439)
                        )
                    )
                    if phase < 55
                    else (
                        (
                            ((phase[:32] & 0) | 2923722038)
                            if phase < 56
                            else ((phase[:32] & 0) | 1316461328)
                        )
                        if phase < 57
                        else (
                            ((phase[:32] & 0) | 825169089)
                            if phase < 58
                            else ((phase[:32] & 0) | 937856626)
                        )
                    )
                )
            )
            if phase < 59
            else (
                (
                    (
                        (
                            ((phase[:32] & 0) | 1042155555)
                            if phase < 60
                            else ((phase[:32] & 0) | 1210211322)
                        )
                        if phase < 61
                        else (
                            ((phase[:32] & 0) | 3302326740)
                            if phase < 62
                            else ((phase[:32] & 0) | 3406625669)
                        )
                    )
                    if phase < 63
                    else (
                        (
                            ((phase[:32] & 0) | 2923722038)
                            if phase < 64
                            else ((phase[:32] & 0) | 3036409575)
                        )
                        if phase < 65
                        else (
                            ((phase[:32] & 0) | 3196076734)
                            if phase < 66
                            else ((phase[:32] & 0) | 993224856)
                        )
                    )
                )
                if phase < 67
                else (
                    (
                        (
                            ((phase[:32] & 0) | 1105912393)
                            if phase < 68
                            else ((phase[:32] & 0) | 1210211322)
                        )
                        if phase < 69
                        else (
                            ((phase[:32] & 0) | 1322898859)
                            if phase < 70
                            else ((phase[:32] & 0) | 3221225472)
                        )
                    )
                    if phase < 71
                    else (
                        (
                            ((phase[:32] & 0) | 2979090268)
                            if phase < 72
                            else ((phase[:32] & 0) | 3091777805)
                        )
                        if phase < 73
                        else (
                            ((phase[:32] & 0) | 3196076734)
                            if phase < 74
                            else (
                                ((phase[:32] & 0) | 3308764271)
                                if phase < 75
                                else ((phase[:32] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    valid_in = (
        (
            (
                (
                    ((phase[:1] & 0) | 0)
                    if phase < 1
                    else (((phase[:1] & 0) | 1) if phase < 2 else ((phase[:1] & 0) | 0))
                )
                if phase < 7
                else (
                    (((phase[:1] & 0) | 1) if phase < 15 else ((phase[:1] & 0) | 0))
                    if phase < 16
                    else (
                        ((phase[:1] & 0) | 1) if phase < 20 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 21
            else (
                (
                    ((phase[:1] & 0) | 1)
                    if phase < 25
                    else (
                        ((phase[:1] & 0) | 0) if phase < 26 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 30
                else (
                    (((phase[:1] & 0) | 0) if phase < 31 else ((phase[:1] & 0) | 1))
                    if phase < 35
                    else (
                        ((phase[:1] & 0) | 0) if phase < 36 else ((phase[:1] & 0) | 1)
                    )
                )
            )
        )
        if phase < 40
        else (
            (
                (
                    ((phase[:1] & 0) | 0)
                    if phase < 41
                    else (
                        ((phase[:1] & 0) | 1) if phase < 45 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 46
                else (
                    (((phase[:1] & 0) | 1) if phase < 50 else ((phase[:1] & 0) | 0))
                    if phase < 51
                    else (
                        ((phase[:1] & 0) | 1) if phase < 55 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 56
            else (
                (
                    (((phase[:1] & 0) | 1) if phase < 60 else ((phase[:1] & 0) | 0))
                    if phase < 61
                    else (
                        ((phase[:1] & 0) | 1) if phase < 65 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 66
                else (
                    (((phase[:1] & 0) | 1) if phase < 70 else ((phase[:1] & 0) | 0))
                    if phase < 71
                    else (
                        ((phase[:1] & 0) | 1) if phase < 75 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
    )
    dut = BF16Fmac(a_in, b_in, acc_in, valid_in)

    expected_result = (
        (
            (
                (
                    (
                        ((phase[:32] & 0) | 0)
                        if phase < 5
                        else (
                            ((phase[:32] & 0) | 1065353216)
                            if phase < 11
                            else ((phase[:32] & 0) | 0)
                        )
                    )
                    if phase < 12
                    else (
                        (
                            ((phase[:32] & 0) | 1065427781)
                            if phase < 13
                            else ((phase[:32] & 0) | 3221225472)
                        )
                        if phase < 14
                        else (
                            ((phase[:32] & 0) | 1073741824)
                            if phase < 15
                            else ((phase[:32] & 0) | 0)
                        )
                    )
                )
                if phase < 16
                else (
                    (
                        ((phase[:32] & 0) | 1065427781)
                        if phase < 17
                        else (
                            ((phase[:32] & 0) | 3221225472)
                            if phase < 18
                            else ((phase[:32] & 0) | 3212836864)
                        )
                    )
                    if phase < 20
                    else (
                        (
                            ((phase[:32] & 0) | 1065353216)
                            if phase < 21
                            else ((phase[:32] & 0) | 1073741824)
                        )
                        if phase < 22
                        else (
                            ((phase[:32] & 0) | 3212836864)
                            if phase < 23
                            else ((phase[:32] & 0) | 0)
                        )
                    )
                )
            )
            if phase < 25
            else (
                (
                    (
                        ((phase[:32] & 0) | 3212836864)
                        if phase < 26
                        else (
                            ((phase[:32] & 0) | 0)
                            if phase < 27
                            else ((phase[:32] & 0) | 1074790400)
                        )
                    )
                    if phase < 28
                    else (
                        (
                            ((phase[:32] & 0) | 1081999360)
                            if phase < 30
                            else ((phase[:32] & 0) | 1065484800)
                        )
                        if phase < 31
                        else (
                            ((phase[:32] & 0) | 1275068416)
                            if phase < 32
                            else ((phase[:32] & 0) | 1283457024)
                        )
                    )
                )
                if phase < 33
                else (
                    (
                        ((phase[:32] & 0) | 1291845632)
                        if phase < 35
                        else (
                            ((phase[:32] & 0) | 1098907648)
                            if phase < 36
                            else ((phase[:32] & 0) | 1048576000)
                        )
                    )
                    if phase < 37
                    else (
                        (
                            ((phase[:32] & 0) | 1065353216)
                            if phase < 40
                            else ((phase[:32] & 0) | 754974720)
                        )
                        if phase < 41
                        else (
                            ((phase[:32] & 0) | 867662257)
                            if phase < 42
                            else ((phase[:32] & 0) | 980349794)
                        )
                    )
                )
            )
        )
        if phase < 43
        else (
            (
                (
                    (
                        ((phase[:32] & 0) | 1084648728)
                        if phase < 45
                        else (
                            ((phase[:32] & 0) | 3344819791)
                            if phase < 46
                            else ((phase[:32] & 0) | 3449123993)
                        )
                    )
                    if phase < 47
                    else (
                        (
                            ((phase[:32] & 0) | 3511802880)
                            if phase < 48
                            else ((phase[:32] & 0) | 3066337454)
                        )
                        if phase < 50
                        else (
                            ((phase[:32] & 0) | 1063188913)
                            if phase < 51
                            else ((phase[:32] & 0) | 3364385885)
                        )
                    )
                )
                if phase < 52
                else (
                    (
                        ((phase[:32] & 0) | 3518208341)
                        if phase < 53
                        else (
                            ((phase[:32] & 0) | 922127928)
                            if phase < 55
                            else ((phase[:32] & 0) | 1076326400)
                        )
                    )
                    if phase < 56
                    else (
                        (
                            ((phase[:32] & 0) | 3134270973)
                            if phase < 57
                            else ((phase[:32] & 0) | 3238569902)
                        )
                        if phase < 58
                        else (
                            ((phase[:32] & 0) | 3351257439)
                            if phase < 60
                            else ((phase[:32] & 0) | 1316461328)
                        )
                    )
                )
            )
            if phase < 61
            else (
                (
                    (
                        ((phase[:32] & 0) | 3384450560)
                        if phase < 62
                        else (
                            ((phase[:32] & 0) | 3531652096)
                            if phase < 63
                            else ((phase[:32] & 0) | 1538119168)
                        )
                    )
                    if phase < 65
                    else (
                        (
                            ((phase[:32] & 0) | 3302211828)
                            if phase < 66
                            else ((phase[:32] & 0) | 3406625669)
                        )
                        if phase < 67
                        else (
                            ((phase[:32] & 0) | 2944838310)
                            if phase < 68
                            else ((phase[:32] & 0) | 949236242)
                        )
                    )
                )
                if phase < 70
                else (
                    (
                        (
                            ((phase[:32] & 0) | 1103218075)
                            if phase < 71
                            else ((phase[:32] & 0) | 3397799366)
                        )
                        if phase < 72
                        else (
                            ((phase[:32] & 0) | 1210211322)
                            if phase < 73
                            else ((phase[:32] & 0) | 1322898859)
                        )
                    )
                    if phase < 75
                    else (
                        (
                            ((phase[:32] & 0) | 1101127680)
                            if phase < 76
                            else ((phase[:32] & 0) | 3404131840)
                        )
                        if phase < 77
                        else (
                            ((phase[:32] & 0) | 3558571008)
                            if phase < 78
                            else ((phase[:32] & 0) | 3308764270)
                        )
                    )
                )
            )
        )
    )
    expected_result_valid = (
        (
            (
                (
                    ((phase[:1] & 0) | 0)
                    if phase < 5
                    else (((phase[:1] & 0) | 1) if phase < 6 else ((phase[:1] & 0) | 0))
                )
                if phase < 11
                else (
                    (((phase[:1] & 0) | 1) if phase < 19 else ((phase[:1] & 0) | 0))
                    if phase < 20
                    else (
                        ((phase[:1] & 0) | 1) if phase < 24 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 25
            else (
                (
                    ((phase[:1] & 0) | 1)
                    if phase < 29
                    else (
                        ((phase[:1] & 0) | 0) if phase < 30 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 34
                else (
                    (((phase[:1] & 0) | 0) if phase < 35 else ((phase[:1] & 0) | 1))
                    if phase < 39
                    else (
                        ((phase[:1] & 0) | 0) if phase < 40 else ((phase[:1] & 0) | 1)
                    )
                )
            )
        )
        if phase < 44
        else (
            (
                (
                    ((phase[:1] & 0) | 0)
                    if phase < 45
                    else (
                        ((phase[:1] & 0) | 1) if phase < 49 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 50
                else (
                    (((phase[:1] & 0) | 1) if phase < 54 else ((phase[:1] & 0) | 0))
                    if phase < 55
                    else (
                        ((phase[:1] & 0) | 1) if phase < 59 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 60
            else (
                (
                    (((phase[:1] & 0) | 1) if phase < 64 else ((phase[:1] & 0) | 0))
                    if phase < 65
                    else (
                        ((phase[:1] & 0) | 1) if phase < 69 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 70
                else (
                    (((phase[:1] & 0) | 1) if phase < 74 else ((phase[:1] & 0) | 0))
                    if phase < 75
                    else (
                        ((phase[:1] & 0) | 1) if phase < 79 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 86:
            assert dut.result == expected_result, "bf16_fmac: result"
            assert dut.result_valid == expected_result_valid, "bf16_fmac: result_valid"

        log("info", "bf16_fmac.result", dut.result)
        log("info", "bf16_fmac.result_valid", dut.result_valid)

    check_and_advance()
    advance(phase)

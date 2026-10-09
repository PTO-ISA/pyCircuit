"""Regular-clock struct_transform scenario; original physical-control oracles remain."""

from example_struct_transform.struct_transform import StructTransform
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseStructTransform():  # noqa: N802
    phase: bits[64] = 0
    op = (
        (
            (
                (
                    (
                        (((phase[:4] & 0) | 1) if phase < 1 else ((phase[:4] & 0) | 15))
                        if phase < 2
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 3
                            else ((phase[:4] & 0) | 15)
                        )
                    )
                    if phase < 5
                    else (
                        (((phase[:4] & 0) | 1) if phase < 6 else ((phase[:4] & 0) | 9))
                        if phase < 7
                        else (
                            ((phase[:4] & 0) | 13)
                            if phase < 10
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                )
                if phase < 11
                else (
                    (
                        (
                            ((phase[:4] & 0) | 10)
                            if phase < 12
                            else ((phase[:4] & 0) | 4)
                        )
                        if phase < 13
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 14
                            else ((phase[:4] & 0) | 15)
                        )
                    )
                    if phase < 15
                    else (
                        (
                            ((phase[:4] & 0) | 1)
                            if phase < 17
                            else ((phase[:4] & 0) | 13)
                        )
                        if phase < 18
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 19
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
            if phase < 20
            else (
                (
                    (
                        (
                            ((phase[:4] & 0) | 3)
                            if phase < 21
                            else ((phase[:4] & 0) | 14)
                        )
                        if phase < 22
                        else (
                            ((phase[:4] & 0) | 11)
                            if phase < 23
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 24
                    else (
                        (
                            ((phase[:4] & 0) | 5)
                            if phase < 25
                            else ((phase[:4] & 0) | 10)
                        )
                        if phase < 26
                        else (
                            ((phase[:4] & 0) | 9)
                            if phase < 27
                            else ((phase[:4] & 0) | 14)
                        )
                    )
                )
                if phase < 28
                else (
                    (
                        (((phase[:4] & 0) | 8) if phase < 29 else ((phase[:4] & 0) | 9))
                        if phase < 30
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 31
                            else ((phase[:4] & 0) | 12)
                        )
                    )
                    if phase < 32
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 33
                            else ((phase[:4] & 0) | 15)
                        )
                        if phase < 34
                        else (
                            ((phase[:4] & 0) | 12)
                            if phase < 36
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
        )
        if phase < 37
        else (
            (
                (
                    (
                        (
                            ((phase[:4] & 0) | 14)
                            if phase < 38
                            else ((phase[:4] & 0) | 13)
                        )
                        if phase < 39
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 40
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                    if phase < 41
                    else (
                        (
                            ((phase[:4] & 0) | 13)
                            if phase < 42
                            else ((phase[:4] & 0) | 6)
                        )
                        if phase < 43
                        else (
                            ((phase[:4] & 0) | 7)
                            if phase < 44
                            else ((phase[:4] & 0) | 3)
                        )
                    )
                )
                if phase < 45
                else (
                    (
                        (
                            ((phase[:4] & 0) | 12)
                            if phase < 46
                            else ((phase[:4] & 0) | 7)
                        )
                        if phase < 47
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 48
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                    if phase < 49
                    else (
                        (
                            ((phase[:4] & 0) | 11)
                            if phase < 50
                            else ((phase[:4] & 0) | 9)
                        )
                        if phase < 51
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 52
                            else ((phase[:4] & 0) | 7)
                        )
                    )
                )
            )
            if phase < 53
            else (
                (
                    (
                        (
                            ((phase[:4] & 0) | 2)
                            if phase < 54
                            else ((phase[:4] & 0) | 11)
                        )
                        if phase < 55
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 56
                            else ((phase[:4] & 0) | 9)
                        )
                    )
                    if phase < 58
                    else (
                        (
                            ((phase[:4] & 0) | 11)
                            if phase < 59
                            else ((phase[:4] & 0) | 1)
                        )
                        if phase < 60
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 61
                            else ((phase[:4] & 0) | 6)
                        )
                    )
                )
                if phase < 62
                else (
                    (
                        (((phase[:4] & 0) | 3) if phase < 63 else ((phase[:4] & 0) | 4))
                        if phase < 64
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 65
                            else ((phase[:4] & 0) | 4)
                        )
                    )
                    if phase < 66
                    else (
                        (
                            ((phase[:4] & 0) | 11)
                            if phase < 67
                            else ((phase[:4] & 0) | 13)
                        )
                        if phase < 68
                        else (
                            ((phase[:4] & 0) | 12)
                            if phase < 69
                            else (
                                ((phase[:4] & 0) | 7)
                                if phase < 70
                                else ((phase[:4] & 0) | 1)
                            )
                        )
                    )
                )
            )
        )
    )
    dst = (
        (
            (
                (
                    (
                        (((phase[:6] & 0) | 2) if phase < 1 else ((phase[:6] & 0) | 63))
                        if phase < 2
                        else (
                            ((phase[:6] & 0) | 2)
                            if phase < 3
                            else ((phase[:6] & 0) | 63)
                        )
                    )
                    if phase < 5
                    else (
                        (((phase[:6] & 0) | 2) if phase < 6 else ((phase[:6] & 0) | 31))
                        if phase < 7
                        else (
                            ((phase[:6] & 0) | 13)
                            if phase < 8
                            else ((phase[:6] & 0) | 4)
                        )
                    )
                )
                if phase < 9
                else (
                    (
                        (
                            ((phase[:6] & 0) | 62)
                            if phase < 10
                            else ((phase[:6] & 0) | 29)
                        )
                        if phase < 11
                        else (
                            ((phase[:6] & 0) | 51)
                            if phase < 12
                            else ((phase[:6] & 0) | 19)
                        )
                    )
                    if phase < 13
                    else (
                        (
                            ((phase[:6] & 0) | 11)
                            if phase < 14
                            else ((phase[:6] & 0) | 50)
                        )
                        if phase < 15
                        else (
                            ((phase[:6] & 0) | 17)
                            if phase < 16
                            else (
                                ((phase[:6] & 0) | 27)
                                if phase < 17
                                else ((phase[:6] & 0) | 49)
                            )
                        )
                    )
                )
            )
            if phase < 18
            else (
                (
                    (
                        (
                            ((phase[:6] & 0) | 43)
                            if phase < 19
                            else ((phase[:6] & 0) | 52)
                        )
                        if phase < 20
                        else (
                            ((phase[:6] & 0) | 39)
                            if phase < 21
                            else ((phase[:6] & 0) | 54)
                        )
                    )
                    if phase < 22
                    else (
                        (
                            ((phase[:6] & 0) | 18)
                            if phase < 23
                            else ((phase[:6] & 0) | 0)
                        )
                        if phase < 24
                        else (
                            ((phase[:6] & 0) | 19)
                            if phase < 25
                            else (
                                ((phase[:6] & 0) | 49)
                                if phase < 26
                                else ((phase[:6] & 0) | 45)
                            )
                        )
                    )
                )
                if phase < 27
                else (
                    (
                        (
                            ((phase[:6] & 0) | 46)
                            if phase < 28
                            else ((phase[:6] & 0) | 5)
                        )
                        if phase < 29
                        else (
                            ((phase[:6] & 0) | 16)
                            if phase < 30
                            else ((phase[:6] & 0) | 44)
                        )
                    )
                    if phase < 31
                    else (
                        (
                            ((phase[:6] & 0) | 42)
                            if phase < 32
                            else ((phase[:6] & 0) | 46)
                        )
                        if phase < 33
                        else (
                            ((phase[:6] & 0) | 19)
                            if phase < 34
                            else (
                                ((phase[:6] & 0) | 20)
                                if phase < 35
                                else ((phase[:6] & 0) | 44)
                            )
                        )
                    )
                )
            )
        )
        if phase < 36
        else (
            (
                (
                    (
                        (
                            ((phase[:6] & 0) | 31)
                            if phase < 37
                            else ((phase[:6] & 0) | 26)
                        )
                        if phase < 38
                        else (
                            ((phase[:6] & 0) | 38)
                            if phase < 39
                            else ((phase[:6] & 0) | 0)
                        )
                    )
                    if phase < 40
                    else (
                        (
                            ((phase[:6] & 0) | 19)
                            if phase < 41
                            else ((phase[:6] & 0) | 34)
                        )
                        if phase < 42
                        else (
                            ((phase[:6] & 0) | 0)
                            if phase < 43
                            else ((phase[:6] & 0) | 6)
                        )
                    )
                )
                if phase < 44
                else (
                    (
                        (
                            ((phase[:6] & 0) | 25)
                            if phase < 45
                            else ((phase[:6] & 0) | 18)
                        )
                        if phase < 46
                        else (
                            ((phase[:6] & 0) | 51)
                            if phase < 47
                            else ((phase[:6] & 0) | 24)
                        )
                    )
                    if phase < 48
                    else (
                        (
                            ((phase[:6] & 0) | 58)
                            if phase < 49
                            else ((phase[:6] & 0) | 2)
                        )
                        if phase < 50
                        else (
                            ((phase[:6] & 0) | 27)
                            if phase < 51
                            else (
                                ((phase[:6] & 0) | 47)
                                if phase < 52
                                else ((phase[:6] & 0) | 50)
                            )
                        )
                    )
                )
            )
            if phase < 53
            else (
                (
                    (
                        (
                            ((phase[:6] & 0) | 12)
                            if phase < 54
                            else ((phase[:6] & 0) | 39)
                        )
                        if phase < 55
                        else (
                            ((phase[:6] & 0) | 47)
                            if phase < 56
                            else ((phase[:6] & 0) | 40)
                        )
                    )
                    if phase < 57
                    else (
                        (
                            ((phase[:6] & 0) | 19)
                            if phase < 58
                            else ((phase[:6] & 0) | 40)
                        )
                        if phase < 59
                        else (
                            ((phase[:6] & 0) | 48)
                            if phase < 60
                            else (
                                ((phase[:6] & 0) | 63)
                                if phase < 61
                                else ((phase[:6] & 0) | 29)
                            )
                        )
                    )
                )
                if phase < 62
                else (
                    (
                        (
                            ((phase[:6] & 0) | 11)
                            if phase < 63
                            else ((phase[:6] & 0) | 46)
                        )
                        if phase < 64
                        else (
                            ((phase[:6] & 0) | 31)
                            if phase < 65
                            else ((phase[:6] & 0) | 25)
                        )
                    )
                    if phase < 66
                    else (
                        (
                            ((phase[:6] & 0) | 26)
                            if phase < 67
                            else ((phase[:6] & 0) | 52)
                        )
                        if phase < 68
                        else (
                            ((phase[:6] & 0) | 27)
                            if phase < 69
                            else (
                                ((phase[:6] & 0) | 62)
                                if phase < 70
                                else ((phase[:6] & 0) | 2)
                            )
                        )
                    )
                )
            )
        )
    )
    word = (
        (
            (
                (
                    (
                        (
                            ((phase[:32] & 0) | 3)
                            if phase < 1
                            else ((phase[:32] & 0) | 4294967295)
                        )
                        if phase < 2
                        else (
                            ((phase[:32] & 0) | 3)
                            if phase < 3
                            else ((phase[:32] & 0) | 4294967295)
                        )
                    )
                    if phase < 5
                    else (
                        (
                            ((phase[:32] & 0) | 3)
                            if phase < 6
                            else ((phase[:32] & 0) | 4294967280)
                        )
                        if phase < 7
                        else (
                            ((phase[:32] & 0) | 4294967295)
                            if phase < 8
                            else ((phase[:32] & 0) | 0)
                        )
                    )
                )
                if phase < 9
                else (
                    (
                        (
                            ((phase[:32] & 0) | 3754036845)
                            if phase < 10
                            else ((phase[:32] & 0) | 1403919592)
                        )
                        if phase < 11
                        else (
                            ((phase[:32] & 0) | 2926762791)
                            if phase < 12
                            else ((phase[:32] & 0) | 1113890394)
                        )
                    )
                    if phase < 13
                    else (
                        (
                            ((phase[:32] & 0) | 694999537)
                            if phase < 14
                            else ((phase[:32] & 0) | 4266986140)
                        )
                        if phase < 15
                        else (
                            ((phase[:32] & 0) | 305571147)
                            if phase < 16
                            else (
                                ((phase[:32] & 0) | 325335598)
                                if phase < 17
                                else ((phase[:32] & 0) | 3593616309)
                            )
                        )
                    )
                )
            )
            if phase < 18
            else (
                (
                    (
                        (
                            ((phase[:32] & 0) | 4117961104)
                            if phase < 19
                            else ((phase[:32] & 0) | 243703727)
                        )
                        if phase < 20
                        else (
                            ((phase[:32] & 0) | 888916290)
                            if phase < 21
                            else ((phase[:32] & 0) | 3873077177)
                        )
                    )
                    if phase < 22
                    else (
                        (
                            ((phase[:32] & 0) | 2991308228)
                            if phase < 23
                            else ((phase[:32] & 0) | 503379)
                        )
                        if phase < 24
                        else (
                            ((phase[:32] & 0) | 1382211478)
                            if phase < 25
                            else (
                                ((phase[:32] & 0) | 2788168189)
                                if phase < 26
                                else ((phase[:32] & 0) | 2512366392)
                            )
                        )
                    )
                )
                if phase < 27
                else (
                    (
                        (
                            ((phase[:32] & 0) | 3990549815)
                            if phase < 28
                            else ((phase[:32] & 0) | 2157990186)
                        )
                        if phase < 29
                        else (
                            ((phase[:32] & 0) | 2449723009)
                            if phase < 30
                            else ((phase[:32] & 0) | 1434506732)
                        )
                    )
                    if phase < 31
                    else (
                        (
                            ((phase[:32] & 0) | 3443644507)
                            if phase < 32
                            else ((phase[:32] & 0) | 98447870)
                        )
                        if phase < 33
                        else (
                            ((phase[:32] & 0) | 4067471685)
                            if phase < 34
                            else (
                                ((phase[:32] & 0) | 3263590880)
                                if phase < 35
                                else ((phase[:32] & 0) | 3447847871)
                            )
                        )
                    )
                )
            )
        )
        if phase < 36
        else (
            (
                (
                    (
                        (
                            ((phase[:32] & 0) | 201184786)
                            if phase < 37
                            else ((phase[:32] & 0) | 3814719049)
                        )
                        if phase < 38
                        else (
                            ((phase[:32] & 0) | 3703632660)
                            if phase < 39
                            else ((phase[:32] & 0) | 269042531)
                        )
                    )
                    if phase < 40
                    else (
                        (
                            ((phase[:32] & 0) | 1382797670)
                            if phase < 41
                            else ((phase[:32] & 0) | 3561830797)
                        )
                        if phase < 42
                        else (
                            ((phase[:32] & 0) | 1745751432)
                            if phase < 43
                            else ((phase[:32] & 0) | 1892799303)
                        )
                    )
                )
                if phase < 44
                else (
                    (
                        (
                            ((phase[:32] & 0) | 859043834)
                            if phase < 45
                            else ((phase[:32] & 0) | 3259639569)
                        )
                        if phase < 46
                        else (
                            ((phase[:32] & 0) | 1986835772)
                            if phase < 47
                            else ((phase[:32] & 0) | 1124470635)
                        )
                    )
                    if phase < 48
                    else (
                        (
                            ((phase[:32] & 0) | 2404736462)
                            if phase < 49
                            else ((phase[:32] & 0) | 2957167317)
                        )
                        if phase < 50
                        else (
                            ((phase[:32] & 0) | 2607881776)
                            if phase < 51
                            else (
                                ((phase[:32] & 0) | 635739087)
                                if phase < 52
                                else ((phase[:32] & 0) | 1985369826)
                            )
                        )
                    )
                )
            )
            if phase < 53
            else (
                (
                    (
                        (
                            ((phase[:32] & 0) | 562129113)
                            if phase < 54
                            else ((phase[:32] & 0) | 3170417764)
                        )
                        if phase < 55
                        else (
                            ((phase[:32] & 0) | 1441029235)
                            if phase < 56
                            else ((phase[:32] & 0) | 2635626294)
                        )
                    )
                    if phase < 57
                    else (
                        (
                            ((phase[:32] & 0) | 2591196445)
                            if phase < 58
                            else ((phase[:32] & 0) | 3038659544)
                        )
                        if phase < 59
                        else (
                            ((phase[:32] & 0) | 504919383)
                            if phase < 60
                            else (
                                ((phase[:32] & 0) | 4159476426)
                                if phase < 61
                                else ((phase[:32] & 0) | 1806230433)
                            )
                        )
                    )
                )
                if phase < 62
                else (
                    (
                        (
                            ((phase[:32] & 0) | 963487884)
                            if phase < 63
                            else ((phase[:32] & 0) | 1305758331)
                        )
                        if phase < 64
                        else (
                            ((phase[:32] & 0) | 199671198)
                            if phase < 65
                            else ((phase[:32] & 0) | 1260488805)
                        )
                    )
                    if phase < 66
                    else (
                        (
                            ((phase[:32] & 0) | 3143114368)
                            if phase < 67
                            else ((phase[:32] & 0) | 3599728607)
                        )
                        if phase < 68
                        else (
                            ((phase[:32] & 0) | 3413265330)
                            if phase < 69
                            else (
                                ((phase[:32] & 0) | 2143795049)
                                if phase < 70
                                else ((phase[:32] & 0) | 3)
                            )
                        )
                    )
                )
            )
        )
    )
    valid = (
        (
            (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 1 else ((phase[:1] & 0) | 0))
                        if phase < 2
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 3
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 5
                    else (
                        (((phase[:1] & 0) | 1) if phase < 6 else ((phase[:1] & 0) | 0))
                        if phase < 7
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 8
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 9
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 10 else ((phase[:1] & 0) | 0))
                        if phase < 11
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 12
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 13
                    else (
                        (((phase[:1] & 0) | 1) if phase < 14 else ((phase[:1] & 0) | 0))
                        if phase < 15
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 16
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 17
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
            if phase < 18
            else (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 19 else ((phase[:1] & 0) | 1))
                        if phase < 20
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 21
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 22
                    else (
                        (((phase[:1] & 0) | 0) if phase < 23 else ((phase[:1] & 0) | 1))
                        if phase < 24
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 25
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                )
                if phase < 26
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 27 else ((phase[:1] & 0) | 1))
                        if phase < 28
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 29
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 30
                    else (
                        (((phase[:1] & 0) | 0) if phase < 31 else ((phase[:1] & 0) | 1))
                        if phase < 32
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 33
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 34
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 35
        else (
            (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 36 else ((phase[:1] & 0) | 0))
                        if phase < 37
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 38
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 39
                    else (
                        (((phase[:1] & 0) | 1) if phase < 40 else ((phase[:1] & 0) | 0))
                        if phase < 41
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 42
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 43
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 44 else ((phase[:1] & 0) | 0))
                        if phase < 45
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 46
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 47
                    else (
                        (((phase[:1] & 0) | 1) if phase < 48 else ((phase[:1] & 0) | 0))
                        if phase < 49
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 50
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 51
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
            if phase < 52
            else (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 53 else ((phase[:1] & 0) | 1))
                        if phase < 54
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 55
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 56
                    else (
                        (((phase[:1] & 0) | 0) if phase < 57 else ((phase[:1] & 0) | 1))
                        if phase < 58
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 59
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 60
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 61
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 62 else ((phase[:1] & 0) | 0))
                        if phase < 63
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 64
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 65
                    else (
                        (((phase[:1] & 0) | 1) if phase < 66 else ((phase[:1] & 0) | 0))
                        if phase < 67
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 68
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 69
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
        )
    )
    dut = StructTransform(op, dst, word, valid)

    expected_op = (
        (
            (
                (
                    (
                        (((phase[:4] & 0) | 0) if phase < 1 else ((phase[:4] & 0) | 1))
                        if phase < 2
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 3
                            else ((phase[:4] & 0) | 1)
                        )
                    )
                    if phase < 4
                    else (
                        (((phase[:4] & 0) | 15) if phase < 6 else ((phase[:4] & 0) | 1))
                        if phase < 7
                        else (
                            ((phase[:4] & 0) | 9)
                            if phase < 8
                            else ((phase[:4] & 0) | 13)
                        )
                    )
                )
                if phase < 11
                else (
                    (
                        (
                            ((phase[:4] & 0) | 5)
                            if phase < 12
                            else ((phase[:4] & 0) | 10)
                        )
                        if phase < 13
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 14
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                    if phase < 15
                    else (
                        (
                            ((phase[:4] & 0) | 15)
                            if phase < 16
                            else ((phase[:4] & 0) | 1)
                        )
                        if phase < 18
                        else (
                            ((phase[:4] & 0) | 13)
                            if phase < 19
                            else ((phase[:4] & 0) | 15)
                        )
                    )
                )
            )
            if phase < 20
            else (
                (
                    (
                        (((phase[:4] & 0) | 0) if phase < 21 else ((phase[:4] & 0) | 3))
                        if phase < 22
                        else (
                            ((phase[:4] & 0) | 14)
                            if phase < 23
                            else ((phase[:4] & 0) | 11)
                        )
                    )
                    if phase < 24
                    else (
                        (((phase[:4] & 0) | 0) if phase < 25 else ((phase[:4] & 0) | 5))
                        if phase < 26
                        else (
                            ((phase[:4] & 0) | 10)
                            if phase < 27
                            else ((phase[:4] & 0) | 9)
                        )
                    )
                )
                if phase < 28
                else (
                    (
                        (
                            ((phase[:4] & 0) | 14)
                            if phase < 29
                            else ((phase[:4] & 0) | 8)
                        )
                        if phase < 30
                        else (
                            ((phase[:4] & 0) | 9)
                            if phase < 31
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                    if phase < 32
                    else (
                        (
                            ((phase[:4] & 0) | 12)
                            if phase < 33
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 34
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 35
                            else (
                                ((phase[:4] & 0) | 12)
                                if phase < 37
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 38
        else (
            (
                (
                    (
                        (
                            ((phase[:4] & 0) | 14)
                            if phase < 39
                            else ((phase[:4] & 0) | 13)
                        )
                        if phase < 40
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 41
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                    if phase < 42
                    else (
                        (
                            ((phase[:4] & 0) | 13)
                            if phase < 43
                            else ((phase[:4] & 0) | 6)
                        )
                        if phase < 44
                        else (
                            ((phase[:4] & 0) | 7)
                            if phase < 45
                            else ((phase[:4] & 0) | 3)
                        )
                    )
                )
                if phase < 46
                else (
                    (
                        (
                            ((phase[:4] & 0) | 12)
                            if phase < 47
                            else ((phase[:4] & 0) | 7)
                        )
                        if phase < 48
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 49
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                    if phase < 50
                    else (
                        (
                            ((phase[:4] & 0) | 11)
                            if phase < 51
                            else ((phase[:4] & 0) | 9)
                        )
                        if phase < 52
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 53
                            else ((phase[:4] & 0) | 7)
                        )
                    )
                )
            )
            if phase < 54
            else (
                (
                    (
                        (
                            ((phase[:4] & 0) | 2)
                            if phase < 55
                            else ((phase[:4] & 0) | 11)
                        )
                        if phase < 56
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 57
                            else ((phase[:4] & 0) | 9)
                        )
                    )
                    if phase < 59
                    else (
                        (
                            ((phase[:4] & 0) | 11)
                            if phase < 60
                            else ((phase[:4] & 0) | 1)
                        )
                        if phase < 61
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 62
                            else ((phase[:4] & 0) | 6)
                        )
                    )
                )
                if phase < 63
                else (
                    (
                        (((phase[:4] & 0) | 3) if phase < 64 else ((phase[:4] & 0) | 4))
                        if phase < 65
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 66
                            else ((phase[:4] & 0) | 4)
                        )
                    )
                    if phase < 67
                    else (
                        (
                            ((phase[:4] & 0) | 11)
                            if phase < 68
                            else ((phase[:4] & 0) | 13)
                        )
                        if phase < 69
                        else (
                            ((phase[:4] & 0) | 12)
                            if phase < 70
                            else (
                                ((phase[:4] & 0) | 7)
                                if phase < 71
                                else ((phase[:4] & 0) | 1)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_dst = (
        (
            (
                (
                    (
                        (((phase[:6] & 0) | 0) if phase < 1 else ((phase[:6] & 0) | 2))
                        if phase < 2
                        else (
                            ((phase[:6] & 0) | 63)
                            if phase < 3
                            else ((phase[:6] & 0) | 2)
                        )
                    )
                    if phase < 4
                    else (
                        (((phase[:6] & 0) | 63) if phase < 6 else ((phase[:6] & 0) | 2))
                        if phase < 7
                        else (
                            ((phase[:6] & 0) | 31)
                            if phase < 8
                            else ((phase[:6] & 0) | 13)
                        )
                    )
                )
                if phase < 9
                else (
                    (
                        (
                            ((phase[:6] & 0) | 4)
                            if phase < 10
                            else ((phase[:6] & 0) | 62)
                        )
                        if phase < 11
                        else (
                            ((phase[:6] & 0) | 29)
                            if phase < 12
                            else ((phase[:6] & 0) | 51)
                        )
                    )
                    if phase < 13
                    else (
                        (
                            ((phase[:6] & 0) | 19)
                            if phase < 14
                            else ((phase[:6] & 0) | 11)
                        )
                        if phase < 15
                        else (
                            ((phase[:6] & 0) | 50)
                            if phase < 16
                            else (
                                ((phase[:6] & 0) | 17)
                                if phase < 17
                                else ((phase[:6] & 0) | 27)
                            )
                        )
                    )
                )
            )
            if phase < 18
            else (
                (
                    (
                        (
                            ((phase[:6] & 0) | 49)
                            if phase < 19
                            else ((phase[:6] & 0) | 43)
                        )
                        if phase < 20
                        else (
                            ((phase[:6] & 0) | 52)
                            if phase < 21
                            else ((phase[:6] & 0) | 39)
                        )
                    )
                    if phase < 22
                    else (
                        (
                            ((phase[:6] & 0) | 54)
                            if phase < 23
                            else ((phase[:6] & 0) | 18)
                        )
                        if phase < 24
                        else (
                            ((phase[:6] & 0) | 0)
                            if phase < 25
                            else (
                                ((phase[:6] & 0) | 19)
                                if phase < 26
                                else ((phase[:6] & 0) | 49)
                            )
                        )
                    )
                )
                if phase < 27
                else (
                    (
                        (
                            ((phase[:6] & 0) | 45)
                            if phase < 28
                            else ((phase[:6] & 0) | 46)
                        )
                        if phase < 29
                        else (
                            ((phase[:6] & 0) | 5)
                            if phase < 30
                            else ((phase[:6] & 0) | 16)
                        )
                    )
                    if phase < 31
                    else (
                        (
                            ((phase[:6] & 0) | 44)
                            if phase < 32
                            else ((phase[:6] & 0) | 42)
                        )
                        if phase < 33
                        else (
                            ((phase[:6] & 0) | 46)
                            if phase < 34
                            else (
                                ((phase[:6] & 0) | 19)
                                if phase < 35
                                else ((phase[:6] & 0) | 20)
                            )
                        )
                    )
                )
            )
        )
        if phase < 36
        else (
            (
                (
                    (
                        (
                            ((phase[:6] & 0) | 44)
                            if phase < 37
                            else ((phase[:6] & 0) | 31)
                        )
                        if phase < 38
                        else (
                            ((phase[:6] & 0) | 26)
                            if phase < 39
                            else ((phase[:6] & 0) | 38)
                        )
                    )
                    if phase < 40
                    else (
                        (
                            ((phase[:6] & 0) | 0)
                            if phase < 41
                            else ((phase[:6] & 0) | 19)
                        )
                        if phase < 42
                        else (
                            ((phase[:6] & 0) | 34)
                            if phase < 43
                            else (
                                ((phase[:6] & 0) | 0)
                                if phase < 44
                                else ((phase[:6] & 0) | 6)
                            )
                        )
                    )
                )
                if phase < 45
                else (
                    (
                        (
                            ((phase[:6] & 0) | 25)
                            if phase < 46
                            else ((phase[:6] & 0) | 18)
                        )
                        if phase < 47
                        else (
                            ((phase[:6] & 0) | 51)
                            if phase < 48
                            else ((phase[:6] & 0) | 24)
                        )
                    )
                    if phase < 49
                    else (
                        (
                            ((phase[:6] & 0) | 58)
                            if phase < 50
                            else ((phase[:6] & 0) | 2)
                        )
                        if phase < 51
                        else (
                            ((phase[:6] & 0) | 27)
                            if phase < 52
                            else (
                                ((phase[:6] & 0) | 47)
                                if phase < 53
                                else ((phase[:6] & 0) | 50)
                            )
                        )
                    )
                )
            )
            if phase < 54
            else (
                (
                    (
                        (
                            ((phase[:6] & 0) | 12)
                            if phase < 55
                            else ((phase[:6] & 0) | 39)
                        )
                        if phase < 56
                        else (
                            ((phase[:6] & 0) | 47)
                            if phase < 57
                            else ((phase[:6] & 0) | 40)
                        )
                    )
                    if phase < 58
                    else (
                        (
                            ((phase[:6] & 0) | 19)
                            if phase < 59
                            else ((phase[:6] & 0) | 40)
                        )
                        if phase < 60
                        else (
                            ((phase[:6] & 0) | 48)
                            if phase < 61
                            else (
                                ((phase[:6] & 0) | 63)
                                if phase < 62
                                else ((phase[:6] & 0) | 29)
                            )
                        )
                    )
                )
                if phase < 63
                else (
                    (
                        (
                            ((phase[:6] & 0) | 11)
                            if phase < 64
                            else ((phase[:6] & 0) | 46)
                        )
                        if phase < 65
                        else (
                            ((phase[:6] & 0) | 31)
                            if phase < 66
                            else ((phase[:6] & 0) | 25)
                        )
                    )
                    if phase < 67
                    else (
                        (
                            ((phase[:6] & 0) | 26)
                            if phase < 68
                            else ((phase[:6] & 0) | 52)
                        )
                        if phase < 69
                        else (
                            ((phase[:6] & 0) | 27)
                            if phase < 70
                            else (
                                ((phase[:6] & 0) | 62)
                                if phase < 71
                                else ((phase[:6] & 0) | 2)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_word = (
        (
            (
                (
                    (
                        (
                            ((phase[:32] & 0) | 1)
                            if phase < 1
                            else ((phase[:32] & 0) | 5)
                        )
                        if phase < 2
                        else (
                            ((phase[:32] & 0) | 15)
                            if phase < 3
                            else ((phase[:32] & 0) | 5)
                        )
                    )
                    if phase < 4
                    else (
                        (
                            ((phase[:32] & 0) | 15)
                            if phase < 6
                            else ((phase[:32] & 0) | 5)
                        )
                        if phase < 7
                        else (
                            ((phase[:32] & 0) | 4294967290)
                            if phase < 8
                            else ((phase[:32] & 0) | 13)
                        )
                    )
                )
                if phase < 9
                else (
                    (
                        (
                            ((phase[:32] & 0) | 14)
                            if phase < 10
                            else ((phase[:32] & 0) | 3754036859)
                        )
                        if phase < 11
                        else (
                            ((phase[:32] & 0) | 1403919598)
                            if phase < 12
                            else ((phase[:32] & 0) | 2926762802)
                        )
                    )
                    if phase < 13
                    else (
                        (
                            ((phase[:32] & 0) | 1113890399)
                            if phase < 14
                            else ((phase[:32] & 0) | 694999540)
                        )
                        if phase < 15
                        else (
                            ((phase[:32] & 0) | 4266986156)
                            if phase < 16
                            else (
                                ((phase[:32] & 0) | 305571149)
                                if phase < 17
                                else ((phase[:32] & 0) | 325335600)
                            )
                        )
                    )
                )
            )
            if phase < 18
            else (
                (
                    (
                        (
                            ((phase[:32] & 0) | 3593616323)
                            if phase < 19
                            else ((phase[:32] & 0) | 4117961120)
                        )
                        if phase < 20
                        else (
                            ((phase[:32] & 0) | 243703728)
                            if phase < 21
                            else ((phase[:32] & 0) | 888916294)
                        )
                    )
                    if phase < 22
                    else (
                        (
                            ((phase[:32] & 0) | 3873077192)
                            if phase < 23
                            else ((phase[:32] & 0) | 2991308240)
                        )
                        if phase < 24
                        else (
                            ((phase[:32] & 0) | 503380)
                            if phase < 25
                            else (
                                ((phase[:32] & 0) | 1382211484)
                                if phase < 26
                                else ((phase[:32] & 0) | 2788168200)
                            )
                        )
                    )
                )
                if phase < 27
                else (
                    (
                        (
                            ((phase[:32] & 0) | 2512366402)
                            if phase < 28
                            else ((phase[:32] & 0) | 3990549830)
                        )
                        if phase < 29
                        else (
                            ((phase[:32] & 0) | 2157990195)
                            if phase < 30
                            else ((phase[:32] & 0) | 2449723019)
                        )
                    )
                    if phase < 31
                    else (
                        (
                            ((phase[:32] & 0) | 1434506738)
                            if phase < 32
                            else ((phase[:32] & 0) | 3443644520)
                        )
                        if phase < 33
                        else (
                            ((phase[:32] & 0) | 98447871)
                            if phase < 34
                            else (
                                ((phase[:32] & 0) | 4067471701)
                                if phase < 35
                                else ((phase[:32] & 0) | 3263590893)
                            )
                        )
                    )
                )
            )
        )
        if phase < 36
        else (
            (
                (
                    (
                        (
                            ((phase[:32] & 0) | 3447847884)
                            if phase < 37
                            else ((phase[:32] & 0) | 201184787)
                        )
                        if phase < 38
                        else (
                            ((phase[:32] & 0) | 3814719064)
                            if phase < 39
                            else ((phase[:32] & 0) | 3703632674)
                        )
                    )
                    if phase < 40
                    else (
                        (
                            ((phase[:32] & 0) | 269042533)
                            if phase < 41
                            else ((phase[:32] & 0) | 1382797676)
                        )
                        if phase < 42
                        else (
                            ((phase[:32] & 0) | 3561830811)
                            if phase < 43
                            else (
                                ((phase[:32] & 0) | 1745751439)
                                if phase < 44
                                else ((phase[:32] & 0) | 1892799311)
                            )
                        )
                    )
                )
                if phase < 45
                else (
                    (
                        (
                            ((phase[:32] & 0) | 859043838)
                            if phase < 46
                            else ((phase[:32] & 0) | 3259639582)
                        )
                        if phase < 47
                        else (
                            ((phase[:32] & 0) | 1986835780)
                            if phase < 48
                            else ((phase[:32] & 0) | 1124470640)
                        )
                    )
                    if phase < 49
                    else (
                        (
                            ((phase[:32] & 0) | 2404736471)
                            if phase < 50
                            else ((phase[:32] & 0) | 2957167329)
                        )
                        if phase < 51
                        else (
                            ((phase[:32] & 0) | 2607881786)
                            if phase < 52
                            else (
                                ((phase[:32] & 0) | 635739090)
                                if phase < 53
                                else ((phase[:32] & 0) | 1985369834)
                            )
                        )
                    )
                )
            )
            if phase < 54
            else (
                (
                    (
                        (
                            ((phase[:32] & 0) | 562129116)
                            if phase < 55
                            else ((phase[:32] & 0) | 3170417776)
                        )
                        if phase < 56
                        else (
                            ((phase[:32] & 0) | 1441029241)
                            if phase < 57
                            else ((phase[:32] & 0) | 2635626304)
                        )
                    )
                    if phase < 58
                    else (
                        (
                            ((phase[:32] & 0) | 2591196455)
                            if phase < 59
                            else ((phase[:32] & 0) | 3038659556)
                        )
                        if phase < 60
                        else (
                            ((phase[:32] & 0) | 504919385)
                            if phase < 61
                            else (
                                ((phase[:32] & 0) | 4159476442)
                                if phase < 62
                                else ((phase[:32] & 0) | 1806230440)
                            )
                        )
                    )
                )
                if phase < 63
                else (
                    (
                        (
                            ((phase[:32] & 0) | 963487888)
                            if phase < 64
                            else ((phase[:32] & 0) | 1305758336)
                        )
                        if phase < 65
                        else (
                            ((phase[:32] & 0) | 199671199)
                            if phase < 66
                            else ((phase[:32] & 0) | 1260488810)
                        )
                    )
                    if phase < 67
                    else (
                        (
                            ((phase[:32] & 0) | 3143114380)
                            if phase < 68
                            else ((phase[:32] & 0) | 3599728621)
                        )
                        if phase < 69
                        else (
                            ((phase[:32] & 0) | 3413265343)
                            if phase < 70
                            else (
                                ((phase[:32] & 0) | 2143795057)
                                if phase < 71
                                else ((phase[:32] & 0) | 5)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_valid = (
        (
            (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 1 else ((phase[:1] & 0) | 1))
                        if phase < 2
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 3
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 4
                    else (
                        (((phase[:1] & 0) | 0) if phase < 6 else ((phase[:1] & 0) | 1))
                        if phase < 7
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 8
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                )
                if phase < 9
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 10 else ((phase[:1] & 0) | 1))
                        if phase < 11
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 12
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 13
                    else (
                        (((phase[:1] & 0) | 0) if phase < 14 else ((phase[:1] & 0) | 1))
                        if phase < 15
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 16
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 17
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 18
            else (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 19 else ((phase[:1] & 0) | 0))
                        if phase < 20
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 21
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 22
                    else (
                        (((phase[:1] & 0) | 1) if phase < 23 else ((phase[:1] & 0) | 0))
                        if phase < 24
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 25
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 26
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 27
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 28 else ((phase[:1] & 0) | 1))
                        if phase < 29
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 30
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 31
                    else (
                        (((phase[:1] & 0) | 0) if phase < 32 else ((phase[:1] & 0) | 1))
                        if phase < 33
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 34
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 35
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 36
        else (
            (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 37 else ((phase[:1] & 0) | 0))
                        if phase < 38
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 39
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 40
                    else (
                        (((phase[:1] & 0) | 1) if phase < 41 else ((phase[:1] & 0) | 0))
                        if phase < 42
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 43
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 44
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 45 else ((phase[:1] & 0) | 0))
                        if phase < 46
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 47
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 48
                    else (
                        (((phase[:1] & 0) | 1) if phase < 49 else ((phase[:1] & 0) | 0))
                        if phase < 50
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 51
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 52
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
            if phase < 53
            else (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 54 else ((phase[:1] & 0) | 1))
                        if phase < 55
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 56
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 57
                    else (
                        (((phase[:1] & 0) | 0) if phase < 58 else ((phase[:1] & 0) | 1))
                        if phase < 59
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 60
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 61
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 62
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 63 else ((phase[:1] & 0) | 0))
                        if phase < 64
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 65
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 66
                    else (
                        (((phase[:1] & 0) | 1) if phase < 67 else ((phase[:1] & 0) | 0))
                        if phase < 68
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 69
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 70
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 72:
            assert dut.op == expected_op, "struct_transform: op"
            assert dut.dst == expected_dst, "struct_transform: dst"
            assert dut.word == expected_word, "struct_transform: word"
            assert dut.valid == expected_valid, "struct_transform: valid"

        log("info", "struct_transform.op", dut.op)
        log("info", "struct_transform.dst", dut.dst)
        log("info", "struct_transform.word", dut.word)
        log("info", "struct_transform.valid", dut.valid)

    check_and_advance()
    advance(phase)

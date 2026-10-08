"""Regular-clock digital_filter scenario; original physical-control oracles remain."""

from example_digital_filter.digital_filter import DigitalFilter
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseDigitalFilter():  # noqa: N802
    phase: bits[64] = 0
    x_in = (
        (
            (
                (
                    (
                        (
                            (
                                ((phase[:16] & 0) | 999)
                                if phase < 1
                                else ((phase[:16] & 0) | 1)
                            )
                            if phase < 2
                            else (
                                ((phase[:16] & 0) | 0)
                                if phase < 9
                                else ((phase[:16] & 0) | 999)
                            )
                        )
                        if phase < 10
                        else (
                            (
                                ((phase[:16] & 0) | 65535)
                                if phase < 11
                                else ((phase[:16] & 0) | 0)
                            )
                            if phase < 18
                            else (
                                ((phase[:16] & 0) | 999)
                                if phase < 19
                                else ((phase[:16] & 0) | 1)
                            )
                        )
                    )
                    if phase < 27
                    else (
                        (
                            (
                                ((phase[:16] & 0) | 999)
                                if phase < 28
                                else ((phase[:16] & 0) | 0)
                            )
                            if phase < 29
                            else (
                                ((phase[:16] & 0) | 1)
                                if phase < 30
                                else ((phase[:16] & 0) | 2)
                            )
                        )
                        if phase < 31
                        else (
                            (
                                ((phase[:16] & 0) | 3)
                                if phase < 32
                                else ((phase[:16] & 0) | 4)
                            )
                            if phase < 33
                            else (
                                ((phase[:16] & 0) | 5)
                                if phase < 34
                                else (
                                    ((phase[:16] & 0) | 6)
                                    if phase < 35
                                    else ((phase[:16] & 0) | 7)
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
                                ((phase[:16] & 0) | 999)
                                if phase < 37
                                else ((phase[:16] & 0) | 100)
                            )
                            if phase < 38
                            else (
                                ((phase[:16] & 0) | 65436)
                                if phase < 39
                                else ((phase[:16] & 0) | 100)
                            )
                        )
                        if phase < 40
                        else (
                            (
                                ((phase[:16] & 0) | 65436)
                                if phase < 41
                                else ((phase[:16] & 0) | 100)
                            )
                            if phase < 42
                            else (
                                ((phase[:16] & 0) | 65436)
                                if phase < 43
                                else ((phase[:16] & 0) | 100)
                            )
                        )
                    )
                    if phase < 44
                    else (
                        (
                            (
                                ((phase[:16] & 0) | 65436)
                                if phase < 45
                                else ((phase[:16] & 0) | 999)
                            )
                            if phase < 46
                            else (
                                ((phase[:16] & 0) | 10000)
                                if phase < 50
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                        if phase < 54
                        else (
                            (
                                ((phase[:16] & 0) | 999)
                                if phase < 55
                                else ((phase[:16] & 0) | 32767)
                            )
                            if phase < 59
                            else (
                                ((phase[:16] & 0) | 0)
                                if phase < 63
                                else (
                                    ((phase[:16] & 0) | 999)
                                    if phase < 64
                                    else ((phase[:16] & 0) | 32768)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 68
            else (
                (
                    (
                        (
                            (
                                ((phase[:16] & 0) | 0)
                                if phase < 72
                                else ((phase[:16] & 0) | 999)
                            )
                            if phase < 73
                            else (
                                ((phase[:16] & 0) | 3)
                                if phase < 74
                                else ((phase[:16] & 0) | 65529)
                            )
                        )
                        if phase < 75
                        else (
                            (
                                ((phase[:16] & 0) | 11)
                                if phase < 76
                                else ((phase[:16] & 0) | 65535)
                            )
                            if phase < 77
                            else (
                                ((phase[:16] & 0) | 61964)
                                if phase < 78
                                else ((phase[:16] & 0) | 58393)
                            )
                        )
                    )
                    if phase < 79
                    else (
                        (
                            (
                                ((phase[:16] & 0) | 54822)
                                if phase < 80
                                else ((phase[:16] & 0) | 51251)
                            )
                            if phase < 81
                            else (
                                ((phase[:16] & 0) | 47680)
                                if phase < 82
                                else ((phase[:16] & 0) | 44109)
                            )
                        )
                        if phase < 83
                        else (
                            (
                                ((phase[:16] & 0) | 40538)
                                if phase < 84
                                else ((phase[:16] & 0) | 36967)
                            )
                            if phase < 85
                            else (
                                ((phase[:16] & 0) | 33396)
                                if phase < 86
                                else (
                                    ((phase[:16] & 0) | 29825)
                                    if phase < 87
                                    else ((phase[:16] & 0) | 26254)
                                )
                            )
                        )
                    )
                )
                if phase < 88
                else (
                    (
                        (
                            (
                                ((phase[:16] & 0) | 22683)
                                if phase < 89
                                else ((phase[:16] & 0) | 19112)
                            )
                            if phase < 90
                            else (
                                ((phase[:16] & 0) | 15541)
                                if phase < 91
                                else ((phase[:16] & 0) | 11970)
                            )
                        )
                        if phase < 92
                        else (
                            (
                                ((phase[:16] & 0) | 8399)
                                if phase < 93
                                else ((phase[:16] & 0) | 4828)
                            )
                            if phase < 94
                            else (
                                ((phase[:16] & 0) | 1257)
                                if phase < 95
                                else (
                                    ((phase[:16] & 0) | 63222)
                                    if phase < 96
                                    else ((phase[:16] & 0) | 59651)
                                )
                            )
                        )
                    )
                    if phase < 97
                    else (
                        (
                            (
                                ((phase[:16] & 0) | 56080)
                                if phase < 98
                                else ((phase[:16] & 0) | 52509)
                            )
                            if phase < 99
                            else (
                                ((phase[:16] & 0) | 48938)
                                if phase < 100
                                else ((phase[:16] & 0) | 45367)
                            )
                        )
                        if phase < 101
                        else (
                            (
                                ((phase[:16] & 0) | 41796)
                                if phase < 102
                                else ((phase[:16] & 0) | 38225)
                            )
                            if phase < 103
                            else (
                                ((phase[:16] & 0) | 34654)
                                if phase < 104
                                else (
                                    ((phase[:16] & 0) | 31083)
                                    if phase < 105
                                    else ((phase[:16] & 0) | 27512)
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 106
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:16] & 0) | 23941)
                                if phase < 107
                                else ((phase[:16] & 0) | 20370)
                            )
                            if phase < 108
                            else (
                                ((phase[:16] & 0) | 65519)
                                if phase < 109
                                else ((phase[:16] & 0) | 4567)
                            )
                        )
                        if phase < 110
                        else (
                            (
                                ((phase[:16] & 0) | 65413)
                                if phase < 111
                                else ((phase[:16] & 0) | 999)
                            )
                            if phase < 112
                            else (
                                ((phase[:16] & 0) | 63826)
                                if phase < 113
                                else ((phase[:16] & 0) | 30387)
                            )
                        )
                    )
                    if phase < 114
                    else (
                        (
                            (
                                ((phase[:16] & 0) | 50398)
                                if phase < 115
                                else ((phase[:16] & 0) | 33299)
                            )
                            if phase < 116
                            else (
                                ((phase[:16] & 0) | 10334)
                                if phase < 117
                                else ((phase[:16] & 0) | 10719)
                            )
                        )
                        if phase < 118
                        else (
                            (
                                ((phase[:16] & 0) | 48105)
                                if phase < 119
                                else ((phase[:16] & 0) | 60486)
                            )
                            if phase < 120
                            else (
                                ((phase[:16] & 0) | 6946)
                                if phase < 121
                                else (
                                    ((phase[:16] & 0) | 54147)
                                    if phase < 122
                                    else ((phase[:16] & 0) | 37803)
                                )
                            )
                        )
                    )
                )
                if phase < 123
                else (
                    (
                        (
                            (
                                ((phase[:16] & 0) | 7304)
                                if phase < 124
                                else ((phase[:16] & 0) | 26616)
                            )
                            if phase < 125
                            else (
                                ((phase[:16] & 0) | 29807)
                                if phase < 126
                                else ((phase[:16] & 0) | 28553)
                            )
                        )
                        if phase < 127
                        else (
                            (
                                ((phase[:16] & 0) | 12806)
                                if phase < 128
                                else ((phase[:16] & 0) | 43721)
                            )
                            if phase < 129
                            else (
                                ((phase[:16] & 0) | 56608)
                                if phase < 130
                                else (
                                    ((phase[:16] & 0) | 57386)
                                    if phase < 131
                                    else ((phase[:16] & 0) | 19363)
                                )
                            )
                        )
                    )
                    if phase < 132
                    else (
                        (
                            (
                                ((phase[:16] & 0) | 22844)
                                if phase < 133
                                else ((phase[:16] & 0) | 11362)
                            )
                            if phase < 134
                            else (
                                ((phase[:16] & 0) | 43463)
                                if phase < 135
                                else ((phase[:16] & 0) | 4671)
                            )
                        )
                        if phase < 136
                        else (
                            (
                                ((phase[:16] & 0) | 57393)
                                if phase < 137
                                else ((phase[:16] & 0) | 34357)
                            )
                            if phase < 138
                            else (
                                ((phase[:16] & 0) | 58816)
                                if phase < 139
                                else (
                                    ((phase[:16] & 0) | 25908)
                                    if phase < 140
                                    else ((phase[:16] & 0) | 65135)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 141
            else (
                (
                    (
                        (
                            (
                                ((phase[:16] & 0) | 26355)
                                if phase < 142
                                else ((phase[:16] & 0) | 37745)
                            )
                            if phase < 143
                            else (
                                ((phase[:16] & 0) | 11555)
                                if phase < 144
                                else ((phase[:16] & 0) | 37019)
                            )
                        )
                        if phase < 145
                        else (
                            (
                                ((phase[:16] & 0) | 14000)
                                if phase < 146
                                else ((phase[:16] & 0) | 60485)
                            )
                            if phase < 147
                            else (
                                ((phase[:16] & 0) | 39150)
                                if phase < 148
                                else ((phase[:16] & 0) | 17002)
                            )
                        )
                    )
                    if phase < 149
                    else (
                        (
                            (
                                ((phase[:16] & 0) | 226)
                                if phase < 150
                                else ((phase[:16] & 0) | 3849)
                            )
                            if phase < 151
                            else (
                                ((phase[:16] & 0) | 18694)
                                if phase < 152
                                else ((phase[:16] & 0) | 55575)
                            )
                        )
                        if phase < 153
                        else (
                            (
                                ((phase[:16] & 0) | 29762)
                                if phase < 154
                                else ((phase[:16] & 0) | 54915)
                            )
                            if phase < 155
                            else (
                                ((phase[:16] & 0) | 11496)
                                if phase < 156
                                else (
                                    ((phase[:16] & 0) | 2840)
                                    if phase < 157
                                    else ((phase[:16] & 0) | 36595)
                                )
                            )
                        )
                    )
                )
                if phase < 158
                else (
                    (
                        (
                            (
                                ((phase[:16] & 0) | 61252)
                                if phase < 159
                                else ((phase[:16] & 0) | 5859)
                            )
                            if phase < 160
                            else (
                                ((phase[:16] & 0) | 6414)
                                if phase < 161
                                else ((phase[:16] & 0) | 39650)
                            )
                        )
                        if phase < 162
                        else (
                            (
                                ((phase[:16] & 0) | 32956)
                                if phase < 163
                                else ((phase[:16] & 0) | 18972)
                            )
                            if phase < 164
                            else (
                                ((phase[:16] & 0) | 39931)
                                if phase < 165
                                else (
                                    ((phase[:16] & 0) | 24174)
                                    if phase < 166
                                    else ((phase[:16] & 0) | 64908)
                                )
                            )
                        )
                    )
                    if phase < 167
                    else (
                        (
                            (
                                ((phase[:16] & 0) | 34517)
                                if phase < 168
                                else ((phase[:16] & 0) | 15545)
                            )
                            if phase < 169
                            else (
                                ((phase[:16] & 0) | 42314)
                                if phase < 170
                                else ((phase[:16] & 0) | 61217)
                            )
                        )
                        if phase < 171
                        else (
                            (
                                ((phase[:16] & 0) | 22763)
                                if phase < 172
                                else ((phase[:16] & 0) | 18600)
                            )
                            if phase < 173
                            else (
                                ((phase[:16] & 0) | 34208)
                                if phase < 174
                                else (
                                    ((phase[:16] & 0) | 53375)
                                    if phase < 175
                                    else ((phase[:16] & 0) | 44191)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    x_valid = (
        (
            (
                (((phase[:1] & 0) | 1) if phase < 76 else ((phase[:1] & 0) | 0))
                if phase < 108
                else (
                    ((phase[:1] & 0) | 1)
                    if phase < 111
                    else (
                        ((phase[:1] & 0) | 0) if phase < 113 else ((phase[:1] & 0) | 1)
                    )
                )
            )
            if phase < 119
            else (
                (
                    ((phase[:1] & 0) | 0)
                    if phase < 120
                    else (
                        ((phase[:1] & 0) | 1) if phase < 126 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 127
                else (
                    ((phase[:1] & 0) | 1)
                    if phase < 133
                    else (
                        ((phase[:1] & 0) | 0) if phase < 134 else ((phase[:1] & 0) | 1)
                    )
                )
            )
        )
        if phase < 140
        else (
            (
                (((phase[:1] & 0) | 0) if phase < 141 else ((phase[:1] & 0) | 1))
                if phase < 147
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 148
                    else (
                        ((phase[:1] & 0) | 1) if phase < 154 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 155
            else (
                (
                    ((phase[:1] & 0) | 1)
                    if phase < 161
                    else (
                        ((phase[:1] & 0) | 0) if phase < 162 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 168
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 169
                    else (
                        ((phase[:1] & 0) | 1) if phase < 175 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
    )
    dut = DigitalFilter(x_in, x_valid)

    expected_y_out = (
        (
            (
                (
                    (
                        (
                            ((phase[:34] & 0) | 0)
                            if phase < 1
                            else (
                                ((phase[:34] & 0) | 999)
                                if phase < 2
                                else ((phase[:34] & 0) | 1999)
                            )
                        )
                        if phase < 3
                        else (
                            (
                                ((phase[:34] & 0) | 2999)
                                if phase < 4
                                else ((phase[:34] & 0) | 3999)
                            )
                            if phase < 5
                            else (
                                ((phase[:34] & 0) | 4)
                                if phase < 6
                                else ((phase[:34] & 0) | 0)
                            )
                        )
                    )
                    if phase < 10
                    else (
                        (
                            (
                                ((phase[:34] & 0) | 999)
                                if phase < 11
                                else ((phase[:34] & 0) | 1997)
                            )
                            if phase < 12
                            else (
                                ((phase[:34] & 0) | 2995)
                                if phase < 13
                                else ((phase[:34] & 0) | 3993)
                            )
                        )
                        if phase < 14
                        else (
                            (
                                ((phase[:34] & 0) | 17179869180)
                                if phase < 15
                                else ((phase[:34] & 0) | 0)
                            )
                            if phase < 19
                            else (
                                ((phase[:34] & 0) | 999)
                                if phase < 20
                                else ((phase[:34] & 0) | 1999)
                            )
                        )
                    )
                )
                if phase < 21
                else (
                    (
                        (
                            (
                                ((phase[:34] & 0) | 3000)
                                if phase < 22
                                else ((phase[:34] & 0) | 4002)
                            )
                            if phase < 23
                            else (
                                ((phase[:34] & 0) | 10)
                                if phase < 28
                                else ((phase[:34] & 0) | 1008)
                            )
                        )
                        if phase < 29
                        else (
                            (
                                ((phase[:34] & 0) | 2005)
                                if phase < 30
                                else ((phase[:34] & 0) | 3002)
                            )
                            if phase < 31
                            else (
                                ((phase[:34] & 0) | 4000)
                                if phase < 32
                                else ((phase[:34] & 0) | 10)
                            )
                        )
                    )
                    if phase < 33
                    else (
                        (
                            (
                                ((phase[:34] & 0) | 20)
                                if phase < 34
                                else ((phase[:34] & 0) | 30)
                            )
                            if phase < 35
                            else (
                                ((phase[:34] & 0) | 40)
                                if phase < 36
                                else ((phase[:34] & 0) | 50)
                            )
                        )
                        if phase < 37
                        else (
                            (
                                ((phase[:34] & 0) | 1051)
                                if phase < 38
                                else ((phase[:34] & 0) | 2143)
                            )
                            if phase < 39
                            else (
                                ((phase[:34] & 0) | 3125)
                                if phase < 40
                                else ((phase[:34] & 0) | 4196)
                            )
                        )
                    )
                )
            )
            if phase < 41
            else (
                (
                    (
                        (
                            ((phase[:34] & 0) | 200)
                            if phase < 42
                            else (
                                ((phase[:34] & 0) | 17179868984)
                                if phase < 43
                                else ((phase[:34] & 0) | 200)
                            )
                        )
                        if phase < 44
                        else (
                            (
                                ((phase[:34] & 0) | 17179868984)
                                if phase < 45
                                else ((phase[:34] & 0) | 200)
                            )
                            if phase < 46
                            else (
                                ((phase[:34] & 0) | 699)
                                if phase < 47
                                else ((phase[:34] & 0) | 12098)
                            )
                        )
                    )
                    if phase < 48
                    else (
                        (
                            (
                                ((phase[:34] & 0) | 32597)
                                if phase < 49
                                else ((phase[:34] & 0) | 63996)
                            )
                            if phase < 50
                            else (
                                ((phase[:34] & 0) | 100000)
                                if phase < 51
                                else ((phase[:34] & 0) | 90000)
                            )
                        )
                        if phase < 52
                        else (
                            (
                                ((phase[:34] & 0) | 70000)
                                if phase < 53
                                else ((phase[:34] & 0) | 40000)
                            )
                            if phase < 54
                            else (
                                ((phase[:34] & 0) | 0)
                                if phase < 55
                                else ((phase[:34] & 0) | 999)
                            )
                        )
                    )
                )
                if phase < 56
                else (
                    (
                        (
                            (
                                ((phase[:34] & 0) | 34765)
                                if phase < 57
                                else ((phase[:34] & 0) | 101298)
                            )
                            if phase < 58
                            else (
                                ((phase[:34] & 0) | 200598)
                                if phase < 59
                                else ((phase[:34] & 0) | 327670)
                            )
                        )
                        if phase < 60
                        else (
                            (
                                ((phase[:34] & 0) | 294903)
                                if phase < 61
                                else ((phase[:34] & 0) | 229369)
                            )
                            if phase < 62
                            else (
                                ((phase[:34] & 0) | 131068)
                                if phase < 63
                                else ((phase[:34] & 0) | 0)
                            )
                        )
                    )
                    if phase < 64
                    else (
                        (
                            (
                                ((phase[:34] & 0) | 999)
                                if phase < 65
                                else ((phase[:34] & 0) | 17179838414)
                            )
                            if phase < 66
                            else (
                                ((phase[:34] & 0) | 17179773877)
                                if phase < 67
                                else ((phase[:34] & 0) | 17179676572)
                            )
                        )
                        if phase < 68
                        else (
                            (
                                ((phase[:34] & 0) | 17179541504)
                                if phase < 69
                                else ((phase[:34] & 0) | 17179574272)
                            )
                            if phase < 70
                            else (
                                ((phase[:34] & 0) | 17179639808)
                                if phase < 71
                                else ((phase[:34] & 0) | 17179738112)
                            )
                        )
                    )
                )
            )
        )
        if phase < 72
        else (
            (
                (
                    (
                        (
                            ((phase[:34] & 0) | 0)
                            if phase < 73
                            else (
                                ((phase[:34] & 0) | 999)
                                if phase < 74
                                else ((phase[:34] & 0) | 2001)
                            )
                        )
                        if phase < 75
                        else (
                            (
                                ((phase[:34] & 0) | 2996)
                                if phase < 76
                                else ((phase[:34] & 0) | 4002)
                            )
                            if phase < 109
                            else (
                                ((phase[:34] & 0) | 17179869180)
                                if phase < 110
                                else ((phase[:34] & 0) | 4538)
                            )
                        )
                    )
                    if phase < 111
                    else (
                        (
                            (
                                ((phase[:34] & 0) | 9004)
                                if phase < 114
                                else ((phase[:34] & 0) | 43774)
                            )
                            if phase < 115
                            else (
                                ((phase[:34] & 0) | 63535)
                                if phase < 116
                                else ((phase[:34] & 0) | 28156)
                            )
                        )
                        if phase < 117
                        else (
                            (
                                ((phase[:34] & 0) | 21994)
                                if phase < 118
                                else ((phase[:34] & 0) | 17179743308)
                            )
                            if phase < 119
                            else (
                                ((phase[:34] & 0) | 17179775245)
                                if phase < 121
                                else ((phase[:34] & 0) | 45577)
                            )
                        )
                    )
                )
                if phase < 122
                else (
                    (
                        (
                            (
                                ((phase[:34] & 0) | 17179862270)
                                if phase < 123
                                else ((phase[:34] & 0) | 17179769787)
                            )
                            if phase < 124
                            else (
                                ((phase[:34] & 0) | 17179814639)
                                if phase < 125
                                else ((phase[:34] & 0) | 17179781653)
                            )
                        )
                        if phase < 126
                        else (
                            (
                                ((phase[:34] & 0) | 17179863203)
                                if phase < 128
                                else ((phase[:34] & 0) | 181484)
                            )
                            if phase < 129
                            else (
                                ((phase[:34] & 0) | 199682)
                                if phase < 130
                                else ((phase[:34] & 0) | 105088)
                            )
                        )
                    )
                    if phase < 131
                    else (
                        (
                            (
                                ((phase[:34] & 0) | 17179828957)
                                if phase < 132
                                else ((phase[:34] & 0) | 17179758203)
                            )
                            if phase < 133
                            else (
                                ((phase[:34] & 0) | 1408)
                                if phase < 135
                                else ((phase[:34] & 0) | 49104)
                            )
                        )
                        if phase < 136
                        else (
                            (
                                ((phase[:34] & 0) | 106509)
                                if phase < 137
                                else ((phase[:34] & 0) | 26356)
                            )
                            if phase < 138
                            else (
                                ((phase[:34] & 0) | 17179747440)
                                if phase < 139
                                else ((phase[:34] & 0) | 17179794361)
                            )
                        )
                    )
                )
            )
            if phase < 140
            else (
                (
                    (
                        (
                            ((phase[:34] & 0) | 17179755543)
                            if phase < 142
                            else (
                                ((phase[:34] & 0) | 17179802479)
                                if phase < 143
                                else ((phase[:34] & 0) | 75763)
                            )
                        )
                        if phase < 144
                        else (
                            (
                                ((phase[:34] & 0) | 138670)
                                if phase < 145
                                else ((phase[:34] & 0) | 16640)
                            )
                            if phase < 146
                            else (
                                ((phase[:34] & 0) | 17179749651)
                                if phase < 147
                                else ((phase[:34] & 0) | 17179852802)
                            )
                        )
                    )
                    if phase < 149
                    else (
                        (
                            (
                                ((phase[:34] & 0) | 17179804016)
                                if phase < 150
                                else ((phase[:34] & 0) | 75077)
                            )
                            if phase < 151
                            else (
                                ((phase[:34] & 0) | 35103)
                                if phase < 152
                                else ((phase[:34] & 0) | 95078)
                            )
                        )
                        if phase < 153
                        else (
                            (
                                ((phase[:34] & 0) | 39878)
                                if phase < 154
                                else ((phase[:34] & 0) | 81318)
                            )
                            if phase < 156
                            else (
                                ((phase[:34] & 0) | 115913)
                                if phase < 157
                                else ((phase[:34] & 0) | 75274)
                            )
                        )
                    )
                )
                if phase < 158
                else (
                    (
                        (
                            (
                                ((phase[:34] & 0) | 130275)
                                if phase < 159
                                else ((phase[:34] & 0) | 17179861522)
                            )
                            if phase < 160
                            else (
                                ((phase[:34] & 0) | 17179791012)
                                if phase < 161
                                else ((phase[:34] & 0) | 17179758700)
                            )
                        )
                        if phase < 163
                        else (
                            (
                                ((phase[:34] & 0) | 17179849873)
                                if phase < 164
                                else ((phase[:34] & 0) | 17179865674)
                            )
                            if phase < 165
                            else (
                                ((phase[:34] & 0) | 17179809439)
                                if phase < 166
                                else ((phase[:34] & 0) | 17179768744)
                            )
                        )
                    )
                    if phase < 167
                    else (
                        (
                            (
                                ((phase[:34] & 0) | 46793)
                                if phase < 168
                                else ((phase[:34] & 0) | 17179807011)
                            )
                            if phase < 170
                            else (
                                ((phase[:34] & 0) | 9552)
                                if phase < 171
                                else ((phase[:34] & 0) | 17179722852)
                            )
                        )
                        if phase < 172
                        else (
                            (
                                ((phase[:34] & 0) | 17179689567)
                                if phase < 173
                                else ((phase[:34] & 0) | 17179827465)
                            )
                            if phase < 174
                            else (
                                ((phase[:34] & 0) | 56885)
                                if phase < 175
                                else ((phase[:34] & 0) | 72035)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_y_valid = (
        (
            (
                (((phase[:1] & 0) | 0) if phase < 1 else ((phase[:1] & 0) | 1))
                if phase < 77
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 109
                    else (
                        ((phase[:1] & 0) | 1) if phase < 112 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 114
            else (
                (
                    ((phase[:1] & 0) | 1)
                    if phase < 120
                    else (
                        ((phase[:1] & 0) | 0) if phase < 121 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 127
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 128
                    else (
                        ((phase[:1] & 0) | 1) if phase < 134 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
        if phase < 135
        else (
            (
                (
                    ((phase[:1] & 0) | 1)
                    if phase < 141
                    else (
                        ((phase[:1] & 0) | 0) if phase < 142 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 148
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 149
                    else (
                        ((phase[:1] & 0) | 1) if phase < 155 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 156
            else (
                (
                    ((phase[:1] & 0) | 1)
                    if phase < 162
                    else (
                        ((phase[:1] & 0) | 0) if phase < 163 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 169
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 170
                    else (
                        ((phase[:1] & 0) | 1) if phase < 176 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 177:
            assert dut.y_out == expected_y_out, "digital_filter: y_out"
            assert dut.y_valid == expected_y_valid, "digital_filter: y_valid"

        log("info", "digital_filter.y_out", dut.y_out)
        log("info", "digital_filter.y_valid", dut.y_valid)

    check_and_advance()
    advance(phase)

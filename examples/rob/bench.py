"""Regular-clock rob scenario; original physical-control oracles remain."""

from example_rob.rob import Rob
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseRob():  # noqa: N802
    phase: bits[64] = 0
    allocate = (
        (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 1
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 6
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 10
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 12
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 15
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 17
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 21
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 24
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 27
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 29
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 30
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 31
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 32
                                else ((phase[:1] & 0) | 1)
                            )
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
                if phase < 37
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 38
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 40
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 42
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 45
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 46
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 47
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 48
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 50
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 57
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 60
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 61
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 62
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 63
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 64
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 65
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 66
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 69
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 70
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 72
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 73
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 74
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 77
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 78
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 80
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 81
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 82
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 83
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 85
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 87
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 89
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 90
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 95
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 98
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 99
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 100
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 102
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 103
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 106
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 110
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 114
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 116
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 119
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 120
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 122
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 123
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 124
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 126
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 130
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 131
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 132
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 133
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 135
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 136
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 137
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 138
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 143
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 144
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 146
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 150
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 151
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 152
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 154
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 160
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 162
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 165
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 167
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 170
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 172
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 173
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 174
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 177
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 178
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 179
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 183
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 184
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 186
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 192
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 195
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 202
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 203
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 205
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 207
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 208
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 209
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 211
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 217
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 221
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 222
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 229
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 231
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 232
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 233
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 235
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 237
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 238
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 239
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 243
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 244
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 246
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 247
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 248
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 252
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 254
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 256
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 257
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 259
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 260
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 263
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 264
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 267
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 268
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 269
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 270
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 271
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 272
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 277
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 279
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 280
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 283
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    allocate_tag = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 1
                                    else ((phase[:8] & 0) | 11)
                                )
                                if phase < 2
                                else (
                                    ((phase[:8] & 0) | 12)
                                    if phase < 3
                                    else ((phase[:8] & 0) | 13)
                                )
                            )
                            if phase < 4
                            else (
                                (
                                    ((phase[:8] & 0) | 14)
                                    if phase < 5
                                    else ((phase[:8] & 0) | 15)
                                )
                                if phase < 6
                                else (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 10
                                    else ((phase[:8] & 0) | 15)
                                )
                            )
                        )
                        if phase < 11
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 16)
                                    if phase < 12
                                    else ((phase[:8] & 0) | 0)
                                )
                                if phase < 15
                                else (
                                    ((phase[:8] & 0) | 17)
                                    if phase < 16
                                    else ((phase[:8] & 0) | 18)
                                )
                            )
                            if phase < 17
                            else (
                                (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 21
                                    else ((phase[:8] & 0) | 21)
                                )
                                if phase < 22
                                else (
                                    ((phase[:8] & 0) | 22)
                                    if phase < 23
                                    else (
                                        ((phase[:8] & 0) | 31)
                                        if phase < 24
                                        else ((phase[:8] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 27
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 199)
                                    if phase < 28
                                    else ((phase[:8] & 0) | 12)
                                )
                                if phase < 29
                                else (
                                    ((phase[:8] & 0) | 225)
                                    if phase < 30
                                    else ((phase[:8] & 0) | 246)
                                )
                            )
                            if phase < 31
                            else (
                                (
                                    ((phase[:8] & 0) | 2)
                                    if phase < 32
                                    else ((phase[:8] & 0) | 69)
                                )
                                if phase < 33
                                else (
                                    ((phase[:8] & 0) | 110)
                                    if phase < 34
                                    else ((phase[:8] & 0) | 1)
                                )
                            )
                        )
                        if phase < 35
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 63)
                                    if phase < 36
                                    else ((phase[:8] & 0) | 17)
                                )
                                if phase < 37
                                else (
                                    ((phase[:8] & 0) | 39)
                                    if phase < 38
                                    else ((phase[:8] & 0) | 165)
                                )
                            )
                            if phase < 39
                            else (
                                (
                                    ((phase[:8] & 0) | 25)
                                    if phase < 40
                                    else ((phase[:8] & 0) | 92)
                                )
                                if phase < 41
                                else (
                                    ((phase[:8] & 0) | 76)
                                    if phase < 42
                                    else (
                                        ((phase[:8] & 0) | 94)
                                        if phase < 43
                                        else ((phase[:8] & 0) | 179)
                                    )
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
                                    ((phase[:8] & 0) | 120)
                                    if phase < 45
                                    else ((phase[:8] & 0) | 215)
                                )
                                if phase < 46
                                else (
                                    ((phase[:8] & 0) | 146)
                                    if phase < 47
                                    else ((phase[:8] & 0) | 133)
                                )
                            )
                            if phase < 48
                            else (
                                (
                                    ((phase[:8] & 0) | 67)
                                    if phase < 49
                                    else ((phase[:8] & 0) | 184)
                                )
                                if phase < 50
                                else (
                                    ((phase[:8] & 0) | 35)
                                    if phase < 51
                                    else ((phase[:8] & 0) | 71)
                                )
                            )
                        )
                        if phase < 52
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 165)
                                    if phase < 53
                                    else ((phase[:8] & 0) | 19)
                                )
                                if phase < 54
                                else (
                                    ((phase[:8] & 0) | 119)
                                    if phase < 55
                                    else ((phase[:8] & 0) | 93)
                                )
                            )
                            if phase < 56
                            else (
                                (
                                    ((phase[:8] & 0) | 5)
                                    if phase < 57
                                    else ((phase[:8] & 0) | 211)
                                )
                                if phase < 58
                                else (
                                    ((phase[:8] & 0) | 24)
                                    if phase < 59
                                    else (
                                        ((phase[:8] & 0) | 81)
                                        if phase < 60
                                        else ((phase[:8] & 0) | 252)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 61
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 127)
                                    if phase < 62
                                    else ((phase[:8] & 0) | 75)
                                )
                                if phase < 63
                                else (
                                    ((phase[:8] & 0) | 6)
                                    if phase < 64
                                    else ((phase[:8] & 0) | 209)
                                )
                            )
                            if phase < 65
                            else (
                                (
                                    ((phase[:8] & 0) | 74)
                                    if phase < 66
                                    else ((phase[:8] & 0) | 206)
                                )
                                if phase < 67
                                else (
                                    ((phase[:8] & 0) | 9)
                                    if phase < 68
                                    else ((phase[:8] & 0) | 19)
                                )
                            )
                        )
                        if phase < 69
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 70
                                    else ((phase[:8] & 0) | 44)
                                )
                                if phase < 71
                                else (
                                    ((phase[:8] & 0) | 138)
                                    if phase < 72
                                    else ((phase[:8] & 0) | 69)
                                )
                            )
                            if phase < 73
                            else (
                                (
                                    ((phase[:8] & 0) | 51)
                                    if phase < 74
                                    else ((phase[:8] & 0) | 19)
                                )
                                if phase < 75
                                else (
                                    ((phase[:8] & 0) | 122)
                                    if phase < 76
                                    else (
                                        ((phase[:8] & 0) | 168)
                                        if phase < 77
                                        else ((phase[:8] & 0) | 82)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 78
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 179)
                                    if phase < 79
                                    else ((phase[:8] & 0) | 232)
                                )
                                if phase < 80
                                else (
                                    ((phase[:8] & 0) | 52)
                                    if phase < 81
                                    else ((phase[:8] & 0) | 210)
                                )
                            )
                            if phase < 82
                            else (
                                (
                                    ((phase[:8] & 0) | 80)
                                    if phase < 83
                                    else ((phase[:8] & 0) | 28)
                                )
                                if phase < 84
                                else (
                                    ((phase[:8] & 0) | 213)
                                    if phase < 85
                                    else ((phase[:8] & 0) | 208)
                                )
                            )
                        )
                        if phase < 86
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 11)
                                    if phase < 87
                                    else ((phase[:8] & 0) | 104)
                                )
                                if phase < 88
                                else (
                                    ((phase[:8] & 0) | 2)
                                    if phase < 89
                                    else ((phase[:8] & 0) | 244)
                                )
                            )
                            if phase < 90
                            else (
                                (
                                    ((phase[:8] & 0) | 109)
                                    if phase < 91
                                    else ((phase[:8] & 0) | 189)
                                )
                                if phase < 92
                                else (
                                    ((phase[:8] & 0) | 180)
                                    if phase < 93
                                    else (
                                        ((phase[:8] & 0) | 101)
                                        if phase < 94
                                        else ((phase[:8] & 0) | 106)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 95
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 221)
                                    if phase < 96
                                    else ((phase[:8] & 0) | 14)
                                )
                                if phase < 97
                                else (
                                    ((phase[:8] & 0) | 77)
                                    if phase < 98
                                    else ((phase[:8] & 0) | 109)
                                )
                            )
                            if phase < 99
                            else (
                                (
                                    ((phase[:8] & 0) | 116)
                                    if phase < 100
                                    else ((phase[:8] & 0) | 91)
                                )
                                if phase < 101
                                else (
                                    ((phase[:8] & 0) | 248)
                                    if phase < 102
                                    else ((phase[:8] & 0) | 39)
                                )
                            )
                        )
                        if phase < 103
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 165)
                                    if phase < 104
                                    else ((phase[:8] & 0) | 178)
                                )
                                if phase < 105
                                else (
                                    ((phase[:8] & 0) | 138)
                                    if phase < 106
                                    else ((phase[:8] & 0) | 22)
                                )
                            )
                            if phase < 107
                            else (
                                (
                                    ((phase[:8] & 0) | 140)
                                    if phase < 108
                                    else ((phase[:8] & 0) | 105)
                                )
                                if phase < 109
                                else (
                                    ((phase[:8] & 0) | 213)
                                    if phase < 110
                                    else (
                                        ((phase[:8] & 0) | 88)
                                        if phase < 111
                                        else ((phase[:8] & 0) | 52)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 112
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 139)
                                    if phase < 113
                                    else ((phase[:8] & 0) | 153)
                                )
                                if phase < 114
                                else (
                                    ((phase[:8] & 0) | 249)
                                    if phase < 115
                                    else ((phase[:8] & 0) | 239)
                                )
                            )
                            if phase < 116
                            else (
                                (
                                    ((phase[:8] & 0) | 134)
                                    if phase < 117
                                    else ((phase[:8] & 0) | 132)
                                )
                                if phase < 118
                                else (
                                    ((phase[:8] & 0) | 198)
                                    if phase < 119
                                    else ((phase[:8] & 0) | 23)
                                )
                            )
                        )
                        if phase < 120
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 11)
                                    if phase < 121
                                    else ((phase[:8] & 0) | 45)
                                )
                                if phase < 122
                                else (
                                    ((phase[:8] & 0) | 115)
                                    if phase < 123
                                    else ((phase[:8] & 0) | 108)
                                )
                            )
                            if phase < 124
                            else (
                                (
                                    ((phase[:8] & 0) | 16)
                                    if phase < 125
                                    else ((phase[:8] & 0) | 84)
                                )
                                if phase < 126
                                else (
                                    ((phase[:8] & 0) | 24)
                                    if phase < 127
                                    else (
                                        ((phase[:8] & 0) | 152)
                                        if phase < 128
                                        else ((phase[:8] & 0) | 66)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 129
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 147)
                                    if phase < 130
                                    else ((phase[:8] & 0) | 125)
                                )
                                if phase < 131
                                else (
                                    ((phase[:8] & 0) | 47)
                                    if phase < 132
                                    else ((phase[:8] & 0) | 253)
                                )
                            )
                            if phase < 133
                            else (
                                (
                                    ((phase[:8] & 0) | 193)
                                    if phase < 134
                                    else ((phase[:8] & 0) | 86)
                                )
                                if phase < 135
                                else (
                                    ((phase[:8] & 0) | 144)
                                    if phase < 136
                                    else (
                                        ((phase[:8] & 0) | 212)
                                        if phase < 137
                                        else ((phase[:8] & 0) | 24)
                                    )
                                )
                            )
                        )
                        if phase < 138
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 252)
                                    if phase < 139
                                    else ((phase[:8] & 0) | 75)
                                )
                                if phase < 140
                                else (
                                    ((phase[:8] & 0) | 14)
                                    if phase < 141
                                    else ((phase[:8] & 0) | 172)
                                )
                            )
                            if phase < 142
                            else (
                                (
                                    ((phase[:8] & 0) | 150)
                                    if phase < 143
                                    else ((phase[:8] & 0) | 137)
                                )
                                if phase < 144
                                else (
                                    ((phase[:8] & 0) | 29)
                                    if phase < 145
                                    else (
                                        ((phase[:8] & 0) | 31)
                                        if phase < 146
                                        else ((phase[:8] & 0) | 154)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 147
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 18)
                                    if phase < 148
                                    else ((phase[:8] & 0) | 56)
                                )
                                if phase < 149
                                else (
                                    ((phase[:8] & 0) | 212)
                                    if phase < 150
                                    else ((phase[:8] & 0) | 153)
                                )
                            )
                            if phase < 151
                            else (
                                (
                                    ((phase[:8] & 0) | 227)
                                    if phase < 152
                                    else ((phase[:8] & 0) | 75)
                                )
                                if phase < 153
                                else (
                                    ((phase[:8] & 0) | 15)
                                    if phase < 154
                                    else ((phase[:8] & 0) | 36)
                                )
                            )
                        )
                        if phase < 155
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 70)
                                    if phase < 156
                                    else ((phase[:8] & 0) | 170)
                                )
                                if phase < 157
                                else (
                                    ((phase[:8] & 0) | 239)
                                    if phase < 158
                                    else ((phase[:8] & 0) | 212)
                                )
                            )
                            if phase < 159
                            else (
                                (
                                    ((phase[:8] & 0) | 5)
                                    if phase < 160
                                    else ((phase[:8] & 0) | 104)
                                )
                                if phase < 161
                                else (
                                    ((phase[:8] & 0) | 150)
                                    if phase < 162
                                    else (
                                        ((phase[:8] & 0) | 100)
                                        if phase < 163
                                        else ((phase[:8] & 0) | 97)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 164
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 207)
                                    if phase < 165
                                    else ((phase[:8] & 0) | 44)
                                )
                                if phase < 166
                                else (
                                    ((phase[:8] & 0) | 248)
                                    if phase < 167
                                    else ((phase[:8] & 0) | 224)
                                )
                            )
                            if phase < 168
                            else (
                                (
                                    ((phase[:8] & 0) | 15)
                                    if phase < 169
                                    else ((phase[:8] & 0) | 80)
                                )
                                if phase < 170
                                else (
                                    ((phase[:8] & 0) | 101)
                                    if phase < 171
                                    else ((phase[:8] & 0) | 187)
                                )
                            )
                        )
                        if phase < 172
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 226)
                                    if phase < 173
                                    else ((phase[:8] & 0) | 128)
                                )
                                if phase < 174
                                else (
                                    ((phase[:8] & 0) | 246)
                                    if phase < 175
                                    else ((phase[:8] & 0) | 25)
                                )
                            )
                            if phase < 176
                            else (
                                (
                                    ((phase[:8] & 0) | 42)
                                    if phase < 177
                                    else ((phase[:8] & 0) | 171)
                                )
                                if phase < 178
                                else (
                                    ((phase[:8] & 0) | 243)
                                    if phase < 179
                                    else (
                                        ((phase[:8] & 0) | 177)
                                        if phase < 180
                                        else ((phase[:8] & 0) | 157)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 181
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 202)
                                    if phase < 182
                                    else ((phase[:8] & 0) | 180)
                                )
                                if phase < 183
                                else (
                                    ((phase[:8] & 0) | 211)
                                    if phase < 184
                                    else ((phase[:8] & 0) | 253)
                                )
                            )
                            if phase < 185
                            else (
                                (
                                    ((phase[:8] & 0) | 203)
                                    if phase < 186
                                    else ((phase[:8] & 0) | 68)
                                )
                                if phase < 187
                                else (
                                    ((phase[:8] & 0) | 141)
                                    if phase < 188
                                    else ((phase[:8] & 0) | 210)
                                )
                            )
                        )
                        if phase < 189
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 178)
                                    if phase < 190
                                    else ((phase[:8] & 0) | 67)
                                )
                                if phase < 191
                                else (
                                    ((phase[:8] & 0) | 14)
                                    if phase < 192
                                    else ((phase[:8] & 0) | 145)
                                )
                            )
                            if phase < 193
                            else (
                                (
                                    ((phase[:8] & 0) | 129)
                                    if phase < 194
                                    else ((phase[:8] & 0) | 208)
                                )
                                if phase < 195
                                else (
                                    ((phase[:8] & 0) | 70)
                                    if phase < 196
                                    else (
                                        ((phase[:8] & 0) | 243)
                                        if phase < 197
                                        else ((phase[:8] & 0) | 88)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 198
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 120)
                                    if phase < 199
                                    else ((phase[:8] & 0) | 25)
                                )
                                if phase < 200
                                else (
                                    ((phase[:8] & 0) | 87)
                                    if phase < 201
                                    else ((phase[:8] & 0) | 126)
                                )
                            )
                            if phase < 202
                            else (
                                (
                                    ((phase[:8] & 0) | 104)
                                    if phase < 203
                                    else ((phase[:8] & 0) | 190)
                                )
                                if phase < 204
                                else (
                                    ((phase[:8] & 0) | 253)
                                    if phase < 205
                                    else ((phase[:8] & 0) | 87)
                                )
                            )
                        )
                        if phase < 206
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 80)
                                    if phase < 207
                                    else ((phase[:8] & 0) | 170)
                                )
                                if phase < 208
                                else (
                                    ((phase[:8] & 0) | 164)
                                    if phase < 209
                                    else ((phase[:8] & 0) | 230)
                                )
                            )
                            if phase < 210
                            else (
                                (
                                    ((phase[:8] & 0) | 31)
                                    if phase < 211
                                    else ((phase[:8] & 0) | 100)
                                )
                                if phase < 212
                                else (
                                    ((phase[:8] & 0) | 237)
                                    if phase < 213
                                    else (
                                        ((phase[:8] & 0) | 145)
                                        if phase < 214
                                        else ((phase[:8] & 0) | 37)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 215
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 210)
                                    if phase < 216
                                    else ((phase[:8] & 0) | 92)
                                )
                                if phase < 217
                                else (
                                    ((phase[:8] & 0) | 119)
                                    if phase < 218
                                    else ((phase[:8] & 0) | 84)
                                )
                            )
                            if phase < 219
                            else (
                                (
                                    ((phase[:8] & 0) | 29)
                                    if phase < 220
                                    else ((phase[:8] & 0) | 99)
                                )
                                if phase < 221
                                else (
                                    ((phase[:8] & 0) | 104)
                                    if phase < 222
                                    else ((phase[:8] & 0) | 144)
                                )
                            )
                        )
                        if phase < 223
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 210)
                                    if phase < 224
                                    else ((phase[:8] & 0) | 140)
                                )
                                if phase < 225
                                else (
                                    ((phase[:8] & 0) | 220)
                                    if phase < 226
                                    else ((phase[:8] & 0) | 148)
                                )
                            )
                            if phase < 227
                            else (
                                (
                                    ((phase[:8] & 0) | 235)
                                    if phase < 228
                                    else ((phase[:8] & 0) | 250)
                                )
                                if phase < 229
                                else (
                                    ((phase[:8] & 0) | 183)
                                    if phase < 230
                                    else (
                                        ((phase[:8] & 0) | 227)
                                        if phase < 231
                                        else ((phase[:8] & 0) | 95)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 232
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 35)
                                    if phase < 233
                                    else ((phase[:8] & 0) | 151)
                                )
                                if phase < 234
                                else (
                                    ((phase[:8] & 0) | 215)
                                    if phase < 235
                                    else ((phase[:8] & 0) | 83)
                                )
                            )
                            if phase < 236
                            else (
                                (
                                    ((phase[:8] & 0) | 64)
                                    if phase < 237
                                    else ((phase[:8] & 0) | 238)
                                )
                                if phase < 238
                                else (
                                    ((phase[:8] & 0) | 153)
                                    if phase < 239
                                    else ((phase[:8] & 0) | 74)
                                )
                            )
                        )
                        if phase < 240
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 90)
                                    if phase < 241
                                    else ((phase[:8] & 0) | 239)
                                )
                                if phase < 242
                                else (
                                    ((phase[:8] & 0) | 20)
                                    if phase < 243
                                    else ((phase[:8] & 0) | 234)
                                )
                            )
                            if phase < 244
                            else (
                                (
                                    ((phase[:8] & 0) | 17)
                                    if phase < 245
                                    else ((phase[:8] & 0) | 210)
                                )
                                if phase < 246
                                else (
                                    ((phase[:8] & 0) | 182)
                                    if phase < 247
                                    else (
                                        ((phase[:8] & 0) | 18)
                                        if phase < 248
                                        else ((phase[:8] & 0) | 17)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 249
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 254)
                                    if phase < 250
                                    else ((phase[:8] & 0) | 169)
                                )
                                if phase < 251
                                else (
                                    ((phase[:8] & 0) | 92)
                                    if phase < 252
                                    else ((phase[:8] & 0) | 76)
                                )
                            )
                            if phase < 253
                            else (
                                (
                                    ((phase[:8] & 0) | 1)
                                    if phase < 254
                                    else ((phase[:8] & 0) | 154)
                                )
                                if phase < 255
                                else (
                                    ((phase[:8] & 0) | 107)
                                    if phase < 256
                                    else ((phase[:8] & 0) | 249)
                                )
                            )
                        )
                        if phase < 257
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 207)
                                    if phase < 258
                                    else ((phase[:8] & 0) | 220)
                                )
                                if phase < 259
                                else (
                                    ((phase[:8] & 0) | 177)
                                    if phase < 260
                                    else ((phase[:8] & 0) | 164)
                                )
                            )
                            if phase < 261
                            else (
                                (
                                    ((phase[:8] & 0) | 100)
                                    if phase < 262
                                    else ((phase[:8] & 0) | 34)
                                )
                                if phase < 263
                                else (
                                    ((phase[:8] & 0) | 217)
                                    if phase < 264
                                    else (
                                        ((phase[:8] & 0) | 183)
                                        if phase < 265
                                        else ((phase[:8] & 0) | 232)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 266
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 104)
                                    if phase < 267
                                    else ((phase[:8] & 0) | 191)
                                )
                                if phase < 268
                                else (
                                    ((phase[:8] & 0) | 181)
                                    if phase < 269
                                    else ((phase[:8] & 0) | 185)
                                )
                            )
                            if phase < 270
                            else (
                                (
                                    ((phase[:8] & 0) | 231)
                                    if phase < 271
                                    else ((phase[:8] & 0) | 237)
                                )
                                if phase < 272
                                else (
                                    ((phase[:8] & 0) | 129)
                                    if phase < 273
                                    else (
                                        ((phase[:8] & 0) | 73)
                                        if phase < 274
                                        else ((phase[:8] & 0) | 229)
                                    )
                                )
                            )
                        )
                        if phase < 275
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 199)
                                    if phase < 276
                                    else ((phase[:8] & 0) | 20)
                                )
                                if phase < 277
                                else (
                                    ((phase[:8] & 0) | 205)
                                    if phase < 278
                                    else ((phase[:8] & 0) | 0)
                                )
                            )
                            if phase < 279
                            else (
                                (
                                    ((phase[:8] & 0) | 145)
                                    if phase < 280
                                    else ((phase[:8] & 0) | 140)
                                )
                                if phase < 281
                                else (
                                    ((phase[:8] & 0) | 27)
                                    if phase < 282
                                    else (
                                        ((phase[:8] & 0) | 7)
                                        if phase < 283
                                        else ((phase[:8] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    complete = (
        (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 1
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 6
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 8
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 9
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 10
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 11
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 14
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 15
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 21
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 22
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 23
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 24
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 26
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 27
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 28
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 32
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 33
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 34
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 41
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 42
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 43
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 44
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 46
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 47
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 51
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 54
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 55
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 59
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 62
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 64
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 65
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 66
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 69
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 73
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 75
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 76
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 77
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 78
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 79
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 80
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 84
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 85
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 89
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 90
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 92
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 93
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 94
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 95
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 97
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 98
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 100
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 104
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 108
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 110
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 111
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 112
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 116
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 119
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 120
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 122
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 124
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 125
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 126
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 127
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 128
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 130
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 132
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 133
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 137
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 139
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 141
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 143
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 145
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 149
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 152
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 156
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 158
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 160
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 165
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 166
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 169
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 170
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 171
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 172
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 175
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 179
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 180
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 181
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 182
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 183
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 184
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 185
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 186
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 188
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 191
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 192
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 195
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 196
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 197
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 198
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 200
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 203
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 204
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 205
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 211
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 213
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 214
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 215
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 216
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 218
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 219
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 223
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 225
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 228
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 229
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 230
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 232
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 235
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 238
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 240
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 245
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 247
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 251
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 255
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 256
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 257
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 258
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 260
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 263
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 266
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 267
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 268
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 269
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 271
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 272
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 273
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 274
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 277
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 278
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 279
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 281
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 282
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 283
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    complete_index = (
        (
            (
                (
                    (
                        (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 7
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 8
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                            if phase < 11
                            else (
                                ((phase[:2] & 0) | 1)
                                if phase < 13
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 15
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                        if phase < 16
                        else (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 17
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 18
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                            if phase < 19
                            else (
                                (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 21
                                    else ((phase[:2] & 0) | 0)
                                )
                                if phase < 27
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 28
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                        )
                    )
                    if phase < 30
                    else (
                        (
                            (
                                ((phase[:2] & 0) | 3)
                                if phase < 32
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 33
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                            if phase < 34
                            else (
                                ((phase[:2] & 0) | 1)
                                if phase < 35
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 36
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                        if phase < 39
                        else (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 40
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 41
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                            if phase < 43
                            else (
                                (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 44
                                    else ((phase[:2] & 0) | 1)
                                )
                                if phase < 46
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 47
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                        )
                    )
                )
                if phase < 48
                else (
                    (
                        (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 50
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 51
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                            if phase < 52
                            else (
                                ((phase[:2] & 0) | 3)
                                if phase < 53
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 54
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                        )
                        if phase < 56
                        else (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 57
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 58
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                            if phase < 59
                            else (
                                (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 60
                                    else ((phase[:2] & 0) | 1)
                                )
                                if phase < 63
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 64
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 66
                    else (
                        (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 67
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 68
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                            if phase < 69
                            else (
                                ((phase[:2] & 0) | 2)
                                if phase < 70
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 71
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                        )
                        if phase < 72
                        else (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 73
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 74
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                            if phase < 75
                            else (
                                (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 76
                                    else ((phase[:2] & 0) | 2)
                                )
                                if phase < 77
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 78
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 79
            else (
                (
                    (
                        (
                            (
                                ((phase[:2] & 0) | 3)
                                if phase < 80
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 82
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                            if phase < 83
                            else (
                                ((phase[:2] & 0) | 2)
                                if phase < 85
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 87
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                        )
                        if phase < 88
                        else (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 89
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 90
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                            if phase < 91
                            else (
                                (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 92
                                    else ((phase[:2] & 0) | 2)
                                )
                                if phase < 93
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 94
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                    )
                    if phase < 95
                    else (
                        (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 96
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 98
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                            if phase < 99
                            else (
                                ((phase[:2] & 0) | 2)
                                if phase < 100
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 102
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                        )
                        if phase < 104
                        else (
                            (
                                ((phase[:2] & 0) | 1)
                                if phase < 105
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 106
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                            if phase < 108
                            else (
                                (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 110
                                    else ((phase[:2] & 0) | 3)
                                )
                                if phase < 111
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 112
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                    )
                )
                if phase < 113
                else (
                    (
                        (
                            (
                                ((phase[:2] & 0) | 1)
                                if phase < 114
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 115
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                            if phase < 117
                            else (
                                ((phase[:2] & 0) | 1)
                                if phase < 118
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 122
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                        )
                        if phase < 123
                        else (
                            (
                                ((phase[:2] & 0) | 3)
                                if phase < 124
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 125
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                            if phase < 126
                            else (
                                (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 128
                                    else ((phase[:2] & 0) | 1)
                                )
                                if phase < 129
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 130
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 131
                    else (
                        (
                            (
                                ((phase[:2] & 0) | 3)
                                if phase < 132
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 133
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                            if phase < 134
                            else (
                                (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 135
                                    else ((phase[:2] & 0) | 0)
                                )
                                if phase < 137
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 138
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                        )
                        if phase < 139
                        else (
                            (
                                ((phase[:2] & 0) | 3)
                                if phase < 140
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 141
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                            if phase < 142
                            else (
                                (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 143
                                    else ((phase[:2] & 0) | 0)
                                )
                                if phase < 145
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 146
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 147
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 149
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 150
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                            if phase < 151
                            else (
                                ((phase[:2] & 0) | 0)
                                if phase < 152
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 153
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                        )
                        if phase < 154
                        else (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 155
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 156
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                            if phase < 158
                            else (
                                (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 159
                                    else ((phase[:2] & 0) | 2)
                                )
                                if phase < 160
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 161
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 162
                    else (
                        (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 163
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 164
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                            if phase < 165
                            else (
                                ((phase[:2] & 0) | 0)
                                if phase < 166
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 168
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                        )
                        if phase < 169
                        else (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 170
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 171
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                            if phase < 172
                            else (
                                (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 175
                                    else ((phase[:2] & 0) | 2)
                                )
                                if phase < 176
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 177
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                    )
                )
                if phase < 178
                else (
                    (
                        (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 179
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 181
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                            if phase < 182
                            else (
                                ((phase[:2] & 0) | 2)
                                if phase < 183
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 184
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                        )
                        if phase < 186
                        else (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 187
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 188
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                            if phase < 189
                            else (
                                (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 190
                                    else ((phase[:2] & 0) | 1)
                                )
                                if phase < 191
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 192
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 193
                    else (
                        (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 196
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 197
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                            if phase < 199
                            else (
                                (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 200
                                    else ((phase[:2] & 0) | 1)
                                )
                                if phase < 201
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 206
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                        )
                        if phase < 207
                        else (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 208
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 209
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                            if phase < 210
                            else (
                                (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 212
                                    else ((phase[:2] & 0) | 3)
                                )
                                if phase < 213
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 215
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 216
            else (
                (
                    (
                        (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 217
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 218
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                            if phase < 219
                            else (
                                ((phase[:2] & 0) | 0)
                                if phase < 220
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 221
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                        if phase < 222
                        else (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 224
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 225
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                            if phase < 229
                            else (
                                (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 230
                                    else ((phase[:2] & 0) | 0)
                                )
                                if phase < 231
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 232
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                    )
                    if phase < 233
                    else (
                        (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 234
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 235
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                            if phase < 237
                            else (
                                ((phase[:2] & 0) | 3)
                                if phase < 239
                                else (
                                    ((phase[:2] & 0) | 0)
                                    if phase < 240
                                    else ((phase[:2] & 0) | 1)
                                )
                            )
                        )
                        if phase < 242
                        else (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 243
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 244
                                    else ((phase[:2] & 0) | 2)
                                )
                            )
                            if phase < 246
                            else (
                                (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 247
                                    else ((phase[:2] & 0) | 0)
                                )
                                if phase < 248
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 249
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                    )
                )
                if phase < 250
                else (
                    (
                        (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 251
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 252
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                            if phase < 255
                            else (
                                ((phase[:2] & 0) | 3)
                                if phase < 256
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 257
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                        )
                        if phase < 258
                        else (
                            (
                                ((phase[:2] & 0) | 1)
                                if phase < 259
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 260
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                            if phase < 261
                            else (
                                (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 262
                                    else ((phase[:2] & 0) | 3)
                                )
                                if phase < 263
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 264
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                    )
                    if phase < 265
                    else (
                        (
                            (
                                ((phase[:2] & 0) | 2)
                                if phase < 266
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 269
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                            if phase < 271
                            else (
                                (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 272
                                    else ((phase[:2] & 0) | 3)
                                )
                                if phase < 273
                                else (
                                    ((phase[:2] & 0) | 2)
                                    if phase < 274
                                    else ((phase[:2] & 0) | 3)
                                )
                            )
                        )
                        if phase < 276
                        else (
                            (
                                ((phase[:2] & 0) | 0)
                                if phase < 278
                                else (
                                    ((phase[:2] & 0) | 1)
                                    if phase < 279
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                            if phase < 280
                            else (
                                (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 281
                                    else ((phase[:2] & 0) | 1)
                                )
                                if phase < 282
                                else (
                                    ((phase[:2] & 0) | 3)
                                    if phase < 283
                                    else ((phase[:2] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    complete_tag = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 6
                                    else ((phase[:8] & 0) | 99)
                                )
                                if phase < 7
                                else (
                                    ((phase[:8] & 0) | 13)
                                    if phase < 8
                                    else ((phase[:8] & 0) | 0)
                                )
                            )
                            if phase < 9
                            else (
                                (
                                    ((phase[:8] & 0) | 11)
                                    if phase < 10
                                    else ((phase[:8] & 0) | 0)
                                )
                                if phase < 11
                                else (
                                    ((phase[:8] & 0) | 12)
                                    if phase < 13
                                    else ((phase[:8] & 0) | 15)
                                )
                            )
                        )
                        if phase < 14
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 15
                                    else ((phase[:8] & 0) | 14)
                                )
                                if phase < 16
                                else (
                                    ((phase[:8] & 0) | 15)
                                    if phase < 17
                                    else ((phase[:8] & 0) | 16)
                                )
                            )
                            if phase < 18
                            else (
                                (
                                    ((phase[:8] & 0) | 17)
                                    if phase < 19
                                    else ((phase[:8] & 0) | 18)
                                )
                                if phase < 21
                                else (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 22
                                    else (
                                        ((phase[:8] & 0) | 21)
                                        if phase < 23
                                        else ((phase[:8] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 24
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 21)
                                    if phase < 25
                                    else ((phase[:8] & 0) | 31)
                                )
                                if phase < 26
                                else (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 27
                                    else ((phase[:8] & 0) | 174)
                                )
                            )
                            if phase < 28
                            else (
                                (
                                    ((phase[:8] & 0) | 224)
                                    if phase < 29
                                    else ((phase[:8] & 0) | 15)
                                )
                                if phase < 30
                                else (
                                    ((phase[:8] & 0) | 112)
                                    if phase < 31
                                    else ((phase[:8] & 0) | 206)
                                )
                            )
                        )
                        if phase < 32
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 107)
                                    if phase < 33
                                    else ((phase[:8] & 0) | 38)
                                )
                                if phase < 34
                                else (
                                    ((phase[:8] & 0) | 8)
                                    if phase < 35
                                    else ((phase[:8] & 0) | 113)
                                )
                            )
                            if phase < 36
                            else (
                                (
                                    ((phase[:8] & 0) | 187)
                                    if phase < 37
                                    else ((phase[:8] & 0) | 222)
                                )
                                if phase < 38
                                else (
                                    ((phase[:8] & 0) | 94)
                                    if phase < 39
                                    else (
                                        ((phase[:8] & 0) | 203)
                                        if phase < 40
                                        else ((phase[:8] & 0) | 174)
                                    )
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
                                (
                                    ((phase[:8] & 0) | 146)
                                    if phase < 42
                                    else ((phase[:8] & 0) | 224)
                                )
                                if phase < 43
                                else (
                                    ((phase[:8] & 0) | 138)
                                    if phase < 44
                                    else ((phase[:8] & 0) | 90)
                                )
                            )
                            if phase < 45
                            else (
                                (
                                    ((phase[:8] & 0) | 23)
                                    if phase < 46
                                    else ((phase[:8] & 0) | 16)
                                )
                                if phase < 47
                                else (
                                    ((phase[:8] & 0) | 164)
                                    if phase < 48
                                    else ((phase[:8] & 0) | 111)
                                )
                            )
                        )
                        if phase < 49
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 45)
                                    if phase < 50
                                    else ((phase[:8] & 0) | 69)
                                )
                                if phase < 51
                                else (
                                    ((phase[:8] & 0) | 183)
                                    if phase < 52
                                    else ((phase[:8] & 0) | 71)
                                )
                            )
                            if phase < 53
                            else (
                                (
                                    ((phase[:8] & 0) | 224)
                                    if phase < 54
                                    else ((phase[:8] & 0) | 52)
                                )
                                if phase < 55
                                else (
                                    ((phase[:8] & 0) | 72)
                                    if phase < 56
                                    else (
                                        ((phase[:8] & 0) | 248)
                                        if phase < 57
                                        else ((phase[:8] & 0) | 55)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 58
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 126)
                                    if phase < 59
                                    else ((phase[:8] & 0) | 215)
                                )
                                if phase < 60
                                else (
                                    ((phase[:8] & 0) | 189)
                                    if phase < 61
                                    else ((phase[:8] & 0) | 67)
                                )
                            )
                            if phase < 62
                            else (
                                (
                                    ((phase[:8] & 0) | 89)
                                    if phase < 63
                                    else ((phase[:8] & 0) | 78)
                                )
                                if phase < 64
                                else (
                                    ((phase[:8] & 0) | 107)
                                    if phase < 65
                                    else (
                                        ((phase[:8] & 0) | 167)
                                        if phase < 66
                                        else ((phase[:8] & 0) | 2)
                                    )
                                )
                            )
                        )
                        if phase < 67
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 242)
                                    if phase < 68
                                    else ((phase[:8] & 0) | 237)
                                )
                                if phase < 69
                                else (
                                    ((phase[:8] & 0) | 121)
                                    if phase < 70
                                    else ((phase[:8] & 0) | 41)
                                )
                            )
                            if phase < 71
                            else (
                                (
                                    ((phase[:8] & 0) | 188)
                                    if phase < 72
                                    else ((phase[:8] & 0) | 143)
                                )
                                if phase < 73
                                else (
                                    ((phase[:8] & 0) | 205)
                                    if phase < 74
                                    else (
                                        ((phase[:8] & 0) | 201)
                                        if phase < 75
                                        else ((phase[:8] & 0) | 43)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 76
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 206)
                                    if phase < 77
                                    else ((phase[:8] & 0) | 39)
                                )
                                if phase < 78
                                else (
                                    ((phase[:8] & 0) | 147)
                                    if phase < 79
                                    else ((phase[:8] & 0) | 119)
                                )
                            )
                            if phase < 80
                            else (
                                (
                                    ((phase[:8] & 0) | 184)
                                    if phase < 81
                                    else ((phase[:8] & 0) | 115)
                                )
                                if phase < 82
                                else (
                                    ((phase[:8] & 0) | 85)
                                    if phase < 83
                                    else ((phase[:8] & 0) | 22)
                                )
                            )
                        )
                        if phase < 84
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 36)
                                    if phase < 85
                                    else ((phase[:8] & 0) | 111)
                                )
                                if phase < 86
                                else (
                                    ((phase[:8] & 0) | 79)
                                    if phase < 87
                                    else ((phase[:8] & 0) | 94)
                                )
                            )
                            if phase < 88
                            else (
                                (
                                    ((phase[:8] & 0) | 85)
                                    if phase < 89
                                    else ((phase[:8] & 0) | 208)
                                )
                                if phase < 90
                                else (
                                    ((phase[:8] & 0) | 131)
                                    if phase < 91
                                    else (
                                        ((phase[:8] & 0) | 44)
                                        if phase < 93
                                        else ((phase[:8] & 0) | 7)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 94
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 30)
                                    if phase < 95
                                    else ((phase[:8] & 0) | 160)
                                )
                                if phase < 96
                                else (
                                    ((phase[:8] & 0) | 16)
                                    if phase < 97
                                    else ((phase[:8] & 0) | 90)
                                )
                            )
                            if phase < 98
                            else (
                                (
                                    ((phase[:8] & 0) | 33)
                                    if phase < 99
                                    else ((phase[:8] & 0) | 216)
                                )
                                if phase < 100
                                else (
                                    ((phase[:8] & 0) | 187)
                                    if phase < 101
                                    else ((phase[:8] & 0) | 228)
                                )
                            )
                        )
                        if phase < 102
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 175)
                                    if phase < 103
                                    else ((phase[:8] & 0) | 5)
                                )
                                if phase < 104
                                else (
                                    ((phase[:8] & 0) | 42)
                                    if phase < 105
                                    else ((phase[:8] & 0) | 126)
                                )
                            )
                            if phase < 106
                            else (
                                (
                                    ((phase[:8] & 0) | 91)
                                    if phase < 107
                                    else ((phase[:8] & 0) | 170)
                                )
                                if phase < 108
                                else (
                                    ((phase[:8] & 0) | 168)
                                    if phase < 109
                                    else (
                                        ((phase[:8] & 0) | 171)
                                        if phase < 110
                                        else ((phase[:8] & 0) | 209)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 111
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 92)
                                    if phase < 112
                                    else ((phase[:8] & 0) | 101)
                                )
                                if phase < 113
                                else (
                                    ((phase[:8] & 0) | 23)
                                    if phase < 114
                                    else ((phase[:8] & 0) | 75)
                                )
                            )
                            if phase < 115
                            else (
                                (
                                    ((phase[:8] & 0) | 78)
                                    if phase < 116
                                    else ((phase[:8] & 0) | 67)
                                )
                                if phase < 117
                                else (
                                    ((phase[:8] & 0) | 155)
                                    if phase < 118
                                    else ((phase[:8] & 0) | 60)
                                )
                            )
                        )
                        if phase < 119
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 15)
                                    if phase < 120
                                    else ((phase[:8] & 0) | 55)
                                )
                                if phase < 121
                                else (
                                    ((phase[:8] & 0) | 93)
                                    if phase < 122
                                    else ((phase[:8] & 0) | 59)
                                )
                            )
                            if phase < 123
                            else (
                                (
                                    ((phase[:8] & 0) | 3)
                                    if phase < 124
                                    else ((phase[:8] & 0) | 76)
                                )
                                if phase < 125
                                else (
                                    ((phase[:8] & 0) | 198)
                                    if phase < 126
                                    else (
                                        ((phase[:8] & 0) | 136)
                                        if phase < 127
                                        else ((phase[:8] & 0) | 32)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 128
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 134)
                                    if phase < 129
                                    else ((phase[:8] & 0) | 220)
                                )
                                if phase < 130
                                else (
                                    ((phase[:8] & 0) | 118)
                                    if phase < 131
                                    else ((phase[:8] & 0) | 142)
                                )
                            )
                            if phase < 132
                            else (
                                (
                                    ((phase[:8] & 0) | 140)
                                    if phase < 133
                                    else ((phase[:8] & 0) | 240)
                                )
                                if phase < 134
                                else (
                                    ((phase[:8] & 0) | 222)
                                    if phase < 135
                                    else (
                                        ((phase[:8] & 0) | 114)
                                        if phase < 136
                                        else ((phase[:8] & 0) | 48)
                                    )
                                )
                            )
                        )
                        if phase < 137
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 107)
                                    if phase < 138
                                    else ((phase[:8] & 0) | 59)
                                )
                                if phase < 139
                                else (
                                    ((phase[:8] & 0) | 60)
                                    if phase < 140
                                    else ((phase[:8] & 0) | 116)
                                )
                            )
                            if phase < 141
                            else (
                                (
                                    ((phase[:8] & 0) | 18)
                                    if phase < 142
                                    else ((phase[:8] & 0) | 149)
                                )
                                if phase < 143
                                else (
                                    ((phase[:8] & 0) | 144)
                                    if phase < 144
                                    else (
                                        ((phase[:8] & 0) | 244)
                                        if phase < 145
                                        else ((phase[:8] & 0) | 223)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 146
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 123)
                                    if phase < 147
                                    else ((phase[:8] & 0) | 21)
                                )
                                if phase < 148
                                else (
                                    ((phase[:8] & 0) | 65)
                                    if phase < 149
                                    else ((phase[:8] & 0) | 222)
                                )
                            )
                            if phase < 150
                            else (
                                (
                                    ((phase[:8] & 0) | 2)
                                    if phase < 151
                                    else ((phase[:8] & 0) | 73)
                                )
                                if phase < 152
                                else (
                                    ((phase[:8] & 0) | 197)
                                    if phase < 153
                                    else ((phase[:8] & 0) | 189)
                                )
                            )
                        )
                        if phase < 154
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 55)
                                    if phase < 155
                                    else ((phase[:8] & 0) | 148)
                                )
                                if phase < 156
                                else (
                                    ((phase[:8] & 0) | 241)
                                    if phase < 157
                                    else ((phase[:8] & 0) | 139)
                                )
                            )
                            if phase < 158
                            else (
                                (
                                    ((phase[:8] & 0) | 197)
                                    if phase < 159
                                    else ((phase[:8] & 0) | 99)
                                )
                                if phase < 160
                                else (
                                    ((phase[:8] & 0) | 24)
                                    if phase < 161
                                    else (
                                        ((phase[:8] & 0) | 2)
                                        if phase < 162
                                        else ((phase[:8] & 0) | 144)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 163
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 36)
                                    if phase < 164
                                    else ((phase[:8] & 0) | 38)
                                )
                                if phase < 165
                                else (
                                    ((phase[:8] & 0) | 9)
                                    if phase < 166
                                    else ((phase[:8] & 0) | 55)
                                )
                            )
                            if phase < 167
                            else (
                                (
                                    ((phase[:8] & 0) | 98)
                                    if phase < 168
                                    else ((phase[:8] & 0) | 85)
                                )
                                if phase < 169
                                else (
                                    ((phase[:8] & 0) | 16)
                                    if phase < 170
                                    else ((phase[:8] & 0) | 234)
                                )
                            )
                        )
                        if phase < 171
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 236)
                                    if phase < 172
                                    else ((phase[:8] & 0) | 69)
                                )
                                if phase < 173
                                else (
                                    ((phase[:8] & 0) | 76)
                                    if phase < 174
                                    else ((phase[:8] & 0) | 42)
                                )
                            )
                            if phase < 175
                            else (
                                (
                                    ((phase[:8] & 0) | 77)
                                    if phase < 176
                                    else ((phase[:8] & 0) | 116)
                                )
                                if phase < 177
                                else (
                                    ((phase[:8] & 0) | 118)
                                    if phase < 178
                                    else (
                                        ((phase[:8] & 0) | 110)
                                        if phase < 179
                                        else ((phase[:8] & 0) | 228)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 180
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 80)
                                    if phase < 181
                                    else ((phase[:8] & 0) | 119)
                                )
                                if phase < 182
                                else (
                                    ((phase[:8] & 0) | 158)
                                    if phase < 183
                                    else ((phase[:8] & 0) | 226)
                                )
                            )
                            if phase < 184
                            else (
                                (
                                    ((phase[:8] & 0) | 72)
                                    if phase < 185
                                    else ((phase[:8] & 0) | 92)
                                )
                                if phase < 186
                                else (
                                    ((phase[:8] & 0) | 204)
                                    if phase < 187
                                    else ((phase[:8] & 0) | 80)
                                )
                            )
                        )
                        if phase < 188
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 71)
                                    if phase < 189
                                    else ((phase[:8] & 0) | 58)
                                )
                                if phase < 190
                                else (
                                    ((phase[:8] & 0) | 77)
                                    if phase < 191
                                    else ((phase[:8] & 0) | 129)
                                )
                            )
                            if phase < 192
                            else (
                                (
                                    ((phase[:8] & 0) | 131)
                                    if phase < 193
                                    else ((phase[:8] & 0) | 128)
                                )
                                if phase < 194
                                else (
                                    ((phase[:8] & 0) | 148)
                                    if phase < 195
                                    else (
                                        ((phase[:8] & 0) | 122)
                                        if phase < 196
                                        else ((phase[:8] & 0) | 21)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 197
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 223)
                                    if phase < 198
                                    else ((phase[:8] & 0) | 114)
                                )
                                if phase < 199
                                else (
                                    ((phase[:8] & 0) | 124)
                                    if phase < 200
                                    else ((phase[:8] & 0) | 200)
                                )
                            )
                            if phase < 201
                            else (
                                (
                                    ((phase[:8] & 0) | 53)
                                    if phase < 202
                                    else ((phase[:8] & 0) | 143)
                                )
                                if phase < 203
                                else (
                                    ((phase[:8] & 0) | 104)
                                    if phase < 204
                                    else (
                                        ((phase[:8] & 0) | 53)
                                        if phase < 205
                                        else ((phase[:8] & 0) | 122)
                                    )
                                )
                            )
                        )
                        if phase < 206
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 137)
                                    if phase < 207
                                    else ((phase[:8] & 0) | 253)
                                )
                                if phase < 208
                                else (
                                    ((phase[:8] & 0) | 222)
                                    if phase < 209
                                    else ((phase[:8] & 0) | 12)
                                )
                            )
                            if phase < 210
                            else (
                                (
                                    ((phase[:8] & 0) | 203)
                                    if phase < 211
                                    else ((phase[:8] & 0) | 231)
                                )
                                if phase < 212
                                else (
                                    ((phase[:8] & 0) | 100)
                                    if phase < 213
                                    else (
                                        ((phase[:8] & 0) | 121)
                                        if phase < 214
                                        else ((phase[:8] & 0) | 244)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 215
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 217)
                                    if phase < 216
                                    else ((phase[:8] & 0) | 22)
                                )
                                if phase < 217
                                else (
                                    ((phase[:8] & 0) | 220)
                                    if phase < 218
                                    else ((phase[:8] & 0) | 155)
                                )
                            )
                            if phase < 219
                            else (
                                (
                                    ((phase[:8] & 0) | 154)
                                    if phase < 220
                                    else ((phase[:8] & 0) | 55)
                                )
                                if phase < 221
                                else (
                                    ((phase[:8] & 0) | 2)
                                    if phase < 222
                                    else ((phase[:8] & 0) | 104)
                                )
                            )
                        )
                        if phase < 223
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 255)
                                    if phase < 224
                                    else ((phase[:8] & 0) | 141)
                                )
                                if phase < 225
                                else (
                                    ((phase[:8] & 0) | 149)
                                    if phase < 226
                                    else ((phase[:8] & 0) | 49)
                                )
                            )
                            if phase < 227
                            else (
                                (
                                    ((phase[:8] & 0) | 155)
                                    if phase < 228
                                    else ((phase[:8] & 0) | 82)
                                )
                                if phase < 229
                                else (
                                    ((phase[:8] & 0) | 193)
                                    if phase < 230
                                    else (
                                        ((phase[:8] & 0) | 181)
                                        if phase < 231
                                        else ((phase[:8] & 0) | 169)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 232
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 136)
                                    if phase < 233
                                    else ((phase[:8] & 0) | 144)
                                )
                                if phase < 234
                                else (
                                    ((phase[:8] & 0) | 68)
                                    if phase < 235
                                    else ((phase[:8] & 0) | 104)
                                )
                            )
                            if phase < 236
                            else (
                                (
                                    ((phase[:8] & 0) | 11)
                                    if phase < 237
                                    else ((phase[:8] & 0) | 122)
                                )
                                if phase < 238
                                else (
                                    ((phase[:8] & 0) | 76)
                                    if phase < 239
                                    else ((phase[:8] & 0) | 24)
                                )
                            )
                        )
                        if phase < 240
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 205)
                                    if phase < 241
                                    else ((phase[:8] & 0) | 176)
                                )
                                if phase < 242
                                else (
                                    ((phase[:8] & 0) | 75)
                                    if phase < 243
                                    else ((phase[:8] & 0) | 175)
                                )
                            )
                            if phase < 244
                            else (
                                (
                                    ((phase[:8] & 0) | 146)
                                    if phase < 245
                                    else ((phase[:8] & 0) | 2)
                                )
                                if phase < 246
                                else (
                                    ((phase[:8] & 0) | 50)
                                    if phase < 247
                                    else (
                                        ((phase[:8] & 0) | 29)
                                        if phase < 248
                                        else ((phase[:8] & 0) | 58)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 249
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 34)
                                    if phase < 250
                                    else ((phase[:8] & 0) | 216)
                                )
                                if phase < 251
                                else (
                                    ((phase[:8] & 0) | 97)
                                    if phase < 252
                                    else ((phase[:8] & 0) | 184)
                                )
                            )
                            if phase < 253
                            else (
                                (
                                    ((phase[:8] & 0) | 45)
                                    if phase < 254
                                    else ((phase[:8] & 0) | 99)
                                )
                                if phase < 255
                                else (
                                    ((phase[:8] & 0) | 139)
                                    if phase < 256
                                    else ((phase[:8] & 0) | 56)
                                )
                            )
                        )
                        if phase < 257
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 235)
                                    if phase < 258
                                    else ((phase[:8] & 0) | 188)
                                )
                                if phase < 259
                                else (
                                    ((phase[:8] & 0) | 126)
                                    if phase < 260
                                    else ((phase[:8] & 0) | 189)
                                )
                            )
                            if phase < 261
                            else (
                                (
                                    ((phase[:8] & 0) | 55)
                                    if phase < 262
                                    else ((phase[:8] & 0) | 138)
                                )
                                if phase < 263
                                else (
                                    ((phase[:8] & 0) | 55)
                                    if phase < 264
                                    else (
                                        ((phase[:8] & 0) | 85)
                                        if phase < 265
                                        else ((phase[:8] & 0) | 171)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 266
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 34)
                                    if phase < 267
                                    else ((phase[:8] & 0) | 245)
                                )
                                if phase < 268
                                else (
                                    ((phase[:8] & 0) | 237)
                                    if phase < 269
                                    else ((phase[:8] & 0) | 185)
                                )
                            )
                            if phase < 270
                            else (
                                (
                                    ((phase[:8] & 0) | 204)
                                    if phase < 271
                                    else ((phase[:8] & 0) | 194)
                                )
                                if phase < 272
                                else (
                                    ((phase[:8] & 0) | 150)
                                    if phase < 273
                                    else (
                                        ((phase[:8] & 0) | 4)
                                        if phase < 274
                                        else ((phase[:8] & 0) | 138)
                                    )
                                )
                            )
                        )
                        if phase < 275
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 276
                                    else ((phase[:8] & 0) | 209)
                                )
                                if phase < 277
                                else (
                                    ((phase[:8] & 0) | 162)
                                    if phase < 278
                                    else ((phase[:8] & 0) | 149)
                                )
                            )
                            if phase < 279
                            else (
                                (
                                    ((phase[:8] & 0) | 113)
                                    if phase < 280
                                    else ((phase[:8] & 0) | 39)
                                )
                                if phase < 281
                                else (
                                    ((phase[:8] & 0) | 56)
                                    if phase < 282
                                    else (
                                        ((phase[:8] & 0) | 207)
                                        if phase < 283
                                        else ((phase[:8] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    complete_value = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 7)
                                    if phase < 1
                                    else ((phase[:16] & 0) | 0)
                                )
                                if phase < 6
                                else (
                                    ((phase[:16] & 0) | 999)
                                    if phase < 7
                                    else ((phase[:16] & 0) | 4915)
                                )
                            )
                            if phase < 8
                            else (
                                (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 9
                                    else ((phase[:16] & 0) | 4369)
                                )
                                if phase < 10
                                else (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 11
                                    else ((phase[:16] & 0) | 4642)
                                )
                            )
                        )
                        if phase < 12
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 65535)
                                    if phase < 13
                                    else ((phase[:16] & 0) | 5461)
                                )
                                if phase < 14
                                else (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 15
                                    else ((phase[:16] & 0) | 5188)
                                )
                            )
                            if phase < 16
                            else (
                                (
                                    ((phase[:16] & 0) | 5461)
                                    if phase < 17
                                    else ((phase[:16] & 0) | 5734)
                                )
                                if phase < 18
                                else (
                                    ((phase[:16] & 0) | 6007)
                                    if phase < 19
                                    else (
                                        ((phase[:16] & 0) | 6280)
                                        if phase < 20
                                        else ((phase[:16] & 0) | 9)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 21
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 22
                                    else ((phase[:16] & 0) | 21)
                                )
                                if phase < 23
                                else (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 24
                                    else ((phase[:16] & 0) | 21)
                                )
                            )
                            if phase < 25
                            else (
                                (
                                    ((phase[:16] & 0) | 12593)
                                    if phase < 26
                                    else ((phase[:16] & 0) | 0)
                                )
                                if phase < 27
                                else (
                                    ((phase[:16] & 0) | 44743)
                                    if phase < 28
                                    else (
                                        ((phase[:16] & 0) | 57356)
                                        if phase < 29
                                        else ((phase[:16] & 0) | 4065)
                                    )
                                )
                            )
                        )
                        if phase < 30
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 28918)
                                    if phase < 31
                                    else ((phase[:16] & 0) | 52738)
                                )
                                if phase < 32
                                else (
                                    ((phase[:16] & 0) | 27461)
                                    if phase < 33
                                    else ((phase[:16] & 0) | 9838)
                                )
                            )
                            if phase < 34
                            else (
                                (
                                    ((phase[:16] & 0) | 2049)
                                    if phase < 35
                                    else ((phase[:16] & 0) | 28991)
                                )
                                if phase < 36
                                else (
                                    ((phase[:16] & 0) | 47889)
                                    if phase < 37
                                    else (
                                        ((phase[:16] & 0) | 56871)
                                        if phase < 38
                                        else ((phase[:16] & 0) | 24229)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 39
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 51993)
                                    if phase < 40
                                    else ((phase[:16] & 0) | 44636)
                                )
                                if phase < 41
                                else (
                                    ((phase[:16] & 0) | 37452)
                                    if phase < 42
                                    else ((phase[:16] & 0) | 57438)
                                )
                            )
                            if phase < 43
                            else (
                                (
                                    ((phase[:16] & 0) | 35507)
                                    if phase < 44
                                    else ((phase[:16] & 0) | 23160)
                                )
                                if phase < 45
                                else (
                                    ((phase[:16] & 0) | 6103)
                                    if phase < 46
                                    else ((phase[:16] & 0) | 4242)
                                )
                            )
                        )
                        if phase < 47
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 42117)
                                    if phase < 48
                                    else ((phase[:16] & 0) | 28483)
                                )
                                if phase < 49
                                else (
                                    ((phase[:16] & 0) | 11704)
                                    if phase < 50
                                    else ((phase[:16] & 0) | 17699)
                                )
                            )
                            if phase < 51
                            else (
                                (
                                    ((phase[:16] & 0) | 46919)
                                    if phase < 52
                                    else ((phase[:16] & 0) | 18341)
                                )
                                if phase < 53
                                else (
                                    ((phase[:16] & 0) | 57363)
                                    if phase < 54
                                    else (
                                        ((phase[:16] & 0) | 13431)
                                        if phase < 55
                                        else ((phase[:16] & 0) | 18525)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 56
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 63493)
                                    if phase < 57
                                    else ((phase[:16] & 0) | 14291)
                                )
                                if phase < 58
                                else (
                                    ((phase[:16] & 0) | 32280)
                                    if phase < 59
                                    else ((phase[:16] & 0) | 55121)
                                )
                            )
                            if phase < 60
                            else (
                                (
                                    ((phase[:16] & 0) | 48636)
                                    if phase < 61
                                    else ((phase[:16] & 0) | 17279)
                                )
                                if phase < 62
                                else (
                                    ((phase[:16] & 0) | 22859)
                                    if phase < 63
                                    else (
                                        ((phase[:16] & 0) | 19974)
                                        if phase < 64
                                        else ((phase[:16] & 0) | 27601)
                                    )
                                )
                            )
                        )
                        if phase < 65
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 42826)
                                    if phase < 66
                                    else ((phase[:16] & 0) | 718)
                                )
                                if phase < 67
                                else (
                                    ((phase[:16] & 0) | 61961)
                                    if phase < 68
                                    else ((phase[:16] & 0) | 60691)
                                )
                            )
                            if phase < 69
                            else (
                                (
                                    ((phase[:16] & 0) | 30976)
                                    if phase < 70
                                    else ((phase[:16] & 0) | 10540)
                                )
                                if phase < 71
                                else (
                                    ((phase[:16] & 0) | 48266)
                                    if phase < 72
                                    else (
                                        ((phase[:16] & 0) | 36677)
                                        if phase < 73
                                        else ((phase[:16] & 0) | 52531)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 74
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 51475)
                                    if phase < 75
                                    else ((phase[:16] & 0) | 11130)
                                )
                                if phase < 76
                                else (
                                    ((phase[:16] & 0) | 52904)
                                    if phase < 77
                                    else ((phase[:16] & 0) | 10066)
                                )
                            )
                            if phase < 78
                            else (
                                (
                                    ((phase[:16] & 0) | 37811)
                                    if phase < 79
                                    else ((phase[:16] & 0) | 30696)
                                )
                                if phase < 80
                                else (
                                    ((phase[:16] & 0) | 47156)
                                    if phase < 81
                                    else ((phase[:16] & 0) | 29650)
                                )
                            )
                        )
                        if phase < 82
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 21840)
                                    if phase < 83
                                    else ((phase[:16] & 0) | 5660)
                                )
                                if phase < 84
                                else (
                                    ((phase[:16] & 0) | 9429)
                                    if phase < 85
                                    else ((phase[:16] & 0) | 28624)
                                )
                            )
                            if phase < 86
                            else (
                                (
                                    ((phase[:16] & 0) | 20235)
                                    if phase < 87
                                    else ((phase[:16] & 0) | 24168)
                                )
                                if phase < 88
                                else (
                                    ((phase[:16] & 0) | 21762)
                                    if phase < 89
                                    else (
                                        ((phase[:16] & 0) | 53492)
                                        if phase < 90
                                        else ((phase[:16] & 0) | 33645)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 91
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 11453)
                                    if phase < 92
                                    else ((phase[:16] & 0) | 11444)
                                )
                                if phase < 93
                                else (
                                    ((phase[:16] & 0) | 1893)
                                    if phase < 94
                                    else ((phase[:16] & 0) | 7786)
                                )
                            )
                            if phase < 95
                            else (
                                (
                                    ((phase[:16] & 0) | 41181)
                                    if phase < 96
                                    else ((phase[:16] & 0) | 4110)
                                )
                                if phase < 97
                                else (
                                    ((phase[:16] & 0) | 23117)
                                    if phase < 98
                                    else (
                                        ((phase[:16] & 0) | 8557)
                                        if phase < 99
                                        else ((phase[:16] & 0) | 55412)
                                    )
                                )
                            )
                        )
                        if phase < 100
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 47963)
                                    if phase < 101
                                    else ((phase[:16] & 0) | 58616)
                                )
                                if phase < 102
                                else (
                                    ((phase[:16] & 0) | 44839)
                                    if phase < 103
                                    else ((phase[:16] & 0) | 1445)
                                )
                            )
                            if phase < 104
                            else (
                                (
                                    ((phase[:16] & 0) | 10930)
                                    if phase < 105
                                    else ((phase[:16] & 0) | 32394)
                                )
                                if phase < 106
                                else (
                                    ((phase[:16] & 0) | 23318)
                                    if phase < 107
                                    else (
                                        ((phase[:16] & 0) | 43660)
                                        if phase < 108
                                        else ((phase[:16] & 0) | 43113)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 109
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 43989)
                                    if phase < 110
                                    else ((phase[:16] & 0) | 53592)
                                )
                                if phase < 111
                                else (
                                    ((phase[:16] & 0) | 23604)
                                    if phase < 112
                                    else ((phase[:16] & 0) | 25995)
                                )
                            )
                            if phase < 113
                            else (
                                (
                                    ((phase[:16] & 0) | 6041)
                                    if phase < 114
                                    else ((phase[:16] & 0) | 19449)
                                )
                                if phase < 115
                                else (
                                    ((phase[:16] & 0) | 20207)
                                    if phase < 116
                                    else ((phase[:16] & 0) | 17286)
                                )
                            )
                        )
                        if phase < 117
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 39812)
                                    if phase < 118
                                    else ((phase[:16] & 0) | 15558)
                                )
                                if phase < 119
                                else (
                                    ((phase[:16] & 0) | 3863)
                                    if phase < 120
                                    else ((phase[:16] & 0) | 14091)
                                )
                            )
                            if phase < 121
                            else (
                                (
                                    ((phase[:16] & 0) | 23853)
                                    if phase < 122
                                    else ((phase[:16] & 0) | 15219)
                                )
                                if phase < 123
                                else (
                                    ((phase[:16] & 0) | 876)
                                    if phase < 124
                                    else (
                                        ((phase[:16] & 0) | 19472)
                                        if phase < 125
                                        else ((phase[:16] & 0) | 50772)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 126
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 34840)
                                    if phase < 127
                                    else ((phase[:16] & 0) | 8344)
                                )
                                if phase < 128
                                else (
                                    ((phase[:16] & 0) | 34370)
                                    if phase < 129
                                    else ((phase[:16] & 0) | 56467)
                                )
                            )
                            if phase < 130
                            else (
                                (
                                    ((phase[:16] & 0) | 30333)
                                    if phase < 131
                                    else ((phase[:16] & 0) | 36399)
                                )
                                if phase < 132
                                else (
                                    ((phase[:16] & 0) | 36093)
                                    if phase < 133
                                    else (
                                        ((phase[:16] & 0) | 61633)
                                        if phase < 134
                                        else ((phase[:16] & 0) | 56918)
                                    )
                                )
                            )
                        )
                        if phase < 135
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 29328)
                                    if phase < 136
                                    else ((phase[:16] & 0) | 12500)
                                )
                                if phase < 137
                                else (
                                    ((phase[:16] & 0) | 27416)
                                    if phase < 138
                                    else ((phase[:16] & 0) | 15356)
                                )
                            )
                            if phase < 139
                            else (
                                (
                                    ((phase[:16] & 0) | 15435)
                                    if phase < 140
                                    else ((phase[:16] & 0) | 29710)
                                )
                                if phase < 141
                                else (
                                    ((phase[:16] & 0) | 4780)
                                    if phase < 142
                                    else (
                                        ((phase[:16] & 0) | 38294)
                                        if phase < 143
                                        else ((phase[:16] & 0) | 37001)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 144
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 62493)
                                    if phase < 145
                                    else ((phase[:16] & 0) | 57119)
                                )
                                if phase < 146
                                else (
                                    ((phase[:16] & 0) | 31642)
                                    if phase < 147
                                    else ((phase[:16] & 0) | 5394)
                                )
                            )
                            if phase < 148
                            else (
                                (
                                    ((phase[:16] & 0) | 16696)
                                    if phase < 149
                                    else ((phase[:16] & 0) | 57044)
                                )
                                if phase < 150
                                else (
                                    ((phase[:16] & 0) | 665)
                                    if phase < 151
                                    else ((phase[:16] & 0) | 18915)
                                )
                            )
                        )
                        if phase < 152
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 50507)
                                    if phase < 153
                                    else ((phase[:16] & 0) | 48399)
                                )
                                if phase < 154
                                else (
                                    ((phase[:16] & 0) | 14116)
                                    if phase < 155
                                    else ((phase[:16] & 0) | 37958)
                                )
                            )
                            if phase < 156
                            else (
                                (
                                    ((phase[:16] & 0) | 61866)
                                    if phase < 157
                                    else ((phase[:16] & 0) | 35823)
                                )
                                if phase < 158
                                else (
                                    ((phase[:16] & 0) | 50644)
                                    if phase < 159
                                    else (
                                        ((phase[:16] & 0) | 25349)
                                        if phase < 160
                                        else ((phase[:16] & 0) | 6248)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 161
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 662)
                                    if phase < 162
                                    else ((phase[:16] & 0) | 36964)
                                )
                                if phase < 163
                                else (
                                    ((phase[:16] & 0) | 9313)
                                    if phase < 164
                                    else ((phase[:16] & 0) | 9935)
                                )
                            )
                            if phase < 165
                            else (
                                (
                                    ((phase[:16] & 0) | 2348)
                                    if phase < 166
                                    else ((phase[:16] & 0) | 14328)
                                )
                                if phase < 167
                                else (
                                    ((phase[:16] & 0) | 25312)
                                    if phase < 168
                                    else (
                                        ((phase[:16] & 0) | 21775)
                                        if phase < 169
                                        else ((phase[:16] & 0) | 4176)
                                    )
                                )
                            )
                        )
                        if phase < 170
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 60005)
                                    if phase < 171
                                    else ((phase[:16] & 0) | 60603)
                                )
                                if phase < 172
                                else (
                                    ((phase[:16] & 0) | 17890)
                                    if phase < 173
                                    else ((phase[:16] & 0) | 19584)
                                )
                            )
                            if phase < 174
                            else (
                                (
                                    ((phase[:16] & 0) | 10998)
                                    if phase < 175
                                    else ((phase[:16] & 0) | 19737)
                                )
                                if phase < 176
                                else (
                                    ((phase[:16] & 0) | 29738)
                                    if phase < 177
                                    else (
                                        ((phase[:16] & 0) | 30379)
                                        if phase < 178
                                        else ((phase[:16] & 0) | 28403)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 179
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 58545)
                                    if phase < 180
                                    else ((phase[:16] & 0) | 20637)
                                )
                                if phase < 181
                                else (
                                    ((phase[:16] & 0) | 30666)
                                    if phase < 182
                                    else ((phase[:16] & 0) | 40628)
                                )
                            )
                            if phase < 183
                            else (
                                (
                                    ((phase[:16] & 0) | 58067)
                                    if phase < 184
                                    else ((phase[:16] & 0) | 18685)
                                )
                                if phase < 185
                                else (
                                    ((phase[:16] & 0) | 23755)
                                    if phase < 186
                                    else ((phase[:16] & 0) | 52292)
                                )
                            )
                        )
                        if phase < 187
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 20621)
                                    if phase < 188
                                    else ((phase[:16] & 0) | 18386)
                                )
                                if phase < 189
                                else (
                                    ((phase[:16] & 0) | 15026)
                                    if phase < 190
                                    else ((phase[:16] & 0) | 19779)
                                )
                            )
                            if phase < 191
                            else (
                                (
                                    ((phase[:16] & 0) | 33038)
                                    if phase < 192
                                    else ((phase[:16] & 0) | 33681)
                                )
                                if phase < 193
                                else (
                                    ((phase[:16] & 0) | 32897)
                                    if phase < 194
                                    else (
                                        ((phase[:16] & 0) | 38096)
                                        if phase < 195
                                        else ((phase[:16] & 0) | 31302)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 196
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 5619)
                                    if phase < 197
                                    else ((phase[:16] & 0) | 57176)
                                )
                                if phase < 198
                                else (
                                    ((phase[:16] & 0) | 29304)
                                    if phase < 199
                                    else ((phase[:16] & 0) | 31769)
                                )
                            )
                            if phase < 200
                            else (
                                (
                                    ((phase[:16] & 0) | 51287)
                                    if phase < 201
                                    else ((phase[:16] & 0) | 13694)
                                )
                                if phase < 202
                                else (
                                    ((phase[:16] & 0) | 36712)
                                    if phase < 203
                                    else (
                                        ((phase[:16] & 0) | 26814)
                                        if phase < 204
                                        else ((phase[:16] & 0) | 13821)
                                    )
                                )
                            )
                        )
                        if phase < 205
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 31319)
                                    if phase < 206
                                    else ((phase[:16] & 0) | 35152)
                                )
                                if phase < 207
                                else (
                                    ((phase[:16] & 0) | 64938)
                                    if phase < 208
                                    else ((phase[:16] & 0) | 56996)
                                )
                            )
                            if phase < 209
                            else (
                                (
                                    ((phase[:16] & 0) | 3302)
                                    if phase < 210
                                    else ((phase[:16] & 0) | 51999)
                                )
                                if phase < 211
                                else (
                                    ((phase[:16] & 0) | 59236)
                                    if phase < 212
                                    else (
                                        ((phase[:16] & 0) | 25837)
                                        if phase < 213
                                        else ((phase[:16] & 0) | 31121)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 214
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 62501)
                                    if phase < 215
                                    else ((phase[:16] & 0) | 55762)
                                )
                                if phase < 216
                                else (
                                    ((phase[:16] & 0) | 5724)
                                    if phase < 217
                                    else ((phase[:16] & 0) | 56439)
                                )
                            )
                            if phase < 218
                            else (
                                (
                                    ((phase[:16] & 0) | 39764)
                                    if phase < 219
                                    else ((phase[:16] & 0) | 39453)
                                )
                                if phase < 220
                                else (
                                    ((phase[:16] & 0) | 14179)
                                    if phase < 221
                                    else ((phase[:16] & 0) | 616)
                                )
                            )
                        )
                        if phase < 222
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 26768)
                                    if phase < 223
                                    else ((phase[:16] & 0) | 65490)
                                )
                                if phase < 224
                                else (
                                    ((phase[:16] & 0) | 36236)
                                    if phase < 225
                                    else ((phase[:16] & 0) | 38364)
                                )
                            )
                            if phase < 226
                            else (
                                (
                                    ((phase[:16] & 0) | 12692)
                                    if phase < 227
                                    else ((phase[:16] & 0) | 39915)
                                )
                                if phase < 228
                                else (
                                    ((phase[:16] & 0) | 21242)
                                    if phase < 229
                                    else (
                                        ((phase[:16] & 0) | 49591)
                                        if phase < 230
                                        else ((phase[:16] & 0) | 46563)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 231
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 43359)
                                    if phase < 232
                                    else ((phase[:16] & 0) | 34851)
                                )
                                if phase < 233
                                else (
                                    ((phase[:16] & 0) | 37015)
                                    if phase < 234
                                    else ((phase[:16] & 0) | 17623)
                                )
                            )
                            if phase < 235
                            else (
                                (
                                    ((phase[:16] & 0) | 26707)
                                    if phase < 236
                                    else ((phase[:16] & 0) | 2880)
                                )
                                if phase < 237
                                else (
                                    ((phase[:16] & 0) | 31470)
                                    if phase < 238
                                    else (
                                        ((phase[:16] & 0) | 19609)
                                        if phase < 239
                                        else ((phase[:16] & 0) | 6218)
                                    )
                                )
                            )
                        )
                        if phase < 240
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 52570)
                                    if phase < 241
                                    else ((phase[:16] & 0) | 45295)
                                )
                                if phase < 242
                                else (
                                    ((phase[:16] & 0) | 19220)
                                    if phase < 243
                                    else ((phase[:16] & 0) | 45034)
                                )
                            )
                            if phase < 244
                            else (
                                (
                                    ((phase[:16] & 0) | 37393)
                                    if phase < 245
                                    else ((phase[:16] & 0) | 722)
                                )
                                if phase < 246
                                else (
                                    ((phase[:16] & 0) | 12982)
                                    if phase < 247
                                    else (
                                        ((phase[:16] & 0) | 7442)
                                        if phase < 248
                                        else ((phase[:16] & 0) | 14865)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 249
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 8958)
                                    if phase < 250
                                    else ((phase[:16] & 0) | 55465)
                                )
                                if phase < 251
                                else (
                                    ((phase[:16] & 0) | 24924)
                                    if phase < 252
                                    else ((phase[:16] & 0) | 47180)
                                )
                            )
                            if phase < 253
                            else (
                                (
                                    ((phase[:16] & 0) | 11521)
                                    if phase < 254
                                    else ((phase[:16] & 0) | 25498)
                                )
                                if phase < 255
                                else (
                                    ((phase[:16] & 0) | 35691)
                                    if phase < 256
                                    else ((phase[:16] & 0) | 14585)
                                )
                            )
                        )
                        if phase < 257
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 60367)
                                    if phase < 258
                                    else ((phase[:16] & 0) | 48348)
                                )
                                if phase < 259
                                else (
                                    ((phase[:16] & 0) | 32433)
                                    if phase < 260
                                    else ((phase[:16] & 0) | 48548)
                                )
                            )
                            if phase < 261
                            else (
                                (
                                    ((phase[:16] & 0) | 14180)
                                    if phase < 262
                                    else ((phase[:16] & 0) | 35362)
                                )
                                if phase < 263
                                else (
                                    ((phase[:16] & 0) | 14297)
                                    if phase < 264
                                    else (
                                        ((phase[:16] & 0) | 21943)
                                        if phase < 265
                                        else ((phase[:16] & 0) | 44008)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 266
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 8808)
                                    if phase < 267
                                    else ((phase[:16] & 0) | 62911)
                                )
                                if phase < 268
                                else (
                                    ((phase[:16] & 0) | 60853)
                                    if phase < 269
                                    else ((phase[:16] & 0) | 47545)
                                )
                            )
                            if phase < 270
                            else (
                                (
                                    ((phase[:16] & 0) | 52455)
                                    if phase < 271
                                    else ((phase[:16] & 0) | 49901)
                                )
                                if phase < 272
                                else (
                                    ((phase[:16] & 0) | 38529)
                                    if phase < 273
                                    else (
                                        ((phase[:16] & 0) | 1097)
                                        if phase < 274
                                        else ((phase[:16] & 0) | 35557)
                                    )
                                )
                            )
                        )
                        if phase < 275
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 199)
                                    if phase < 276
                                    else ((phase[:16] & 0) | 53524)
                                )
                                if phase < 277
                                else (
                                    ((phase[:16] & 0) | 41677)
                                    if phase < 278
                                    else ((phase[:16] & 0) | 38144)
                                )
                            )
                            if phase < 279
                            else (
                                (
                                    ((phase[:16] & 0) | 29073)
                                    if phase < 280
                                    else ((phase[:16] & 0) | 10124)
                                )
                                if phase < 281
                                else (
                                    ((phase[:16] & 0) | 14363)
                                    if phase < 282
                                    else (
                                        ((phase[:16] & 0) | 52999)
                                        if phase < 283
                                        else ((phase[:16] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    retire = (
        (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 1
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 8
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 10
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 11
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 12
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 13
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 21
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 22
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 23
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 24
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 26
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 27
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 29
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 30
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 31
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 34
                                    else ((phase[:1] & 0) | 1)
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
                                ((phase[:1] & 0) | 0)
                                if phase < 38
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 39
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 40
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 41
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 43
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 44
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 46
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 50
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 51
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 53
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 56
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 59
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 60
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 61
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 62
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 63
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 66
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 70
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 71
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 72
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 73
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 74
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 75
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 76
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 78
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 79
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 80
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 81
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 82
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 86
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 87
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 91
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 93
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 94
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 98
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 102
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 104
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 105
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 106
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 108
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 114
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 115
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 116
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 119
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 120
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 122
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 128
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 130
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 132
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 133
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 136
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 137
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 138
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 139
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 141
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 144
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 146
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 147
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 151
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 152
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 153
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 155
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 157
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 158
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 167
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 168
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 169
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 175
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 176
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
                if phase < 177
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 181
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 182
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 185
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 186
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 188
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 190
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 191
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 193
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 194
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 196
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 197
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 199
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 201
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 202
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 204
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 208
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 211
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 212
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 218
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 220
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 223
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 224
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 227
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 228
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 230
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 233
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 234
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 235
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 238
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 239
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 244
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 245
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 247
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 250
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 251
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 252
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 253
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 255
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 256
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 259
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 261
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 262
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 265
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 266
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 267
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 271
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 272
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 273
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 274
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 275
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 276
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 281
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    flush = (
        (
            (((phase[:1] & 0) | 0) if phase < 22 else ((phase[:1] & 0) | 1))
            if phase < 23
            else (((phase[:1] & 0) | 0) if phase < 26 else ((phase[:1] & 0) | 1))
        )
        if phase < 27
        else (
            (((phase[:1] & 0) | 0) if phase < 72 else ((phase[:1] & 0) | 1))
            if phase < 73
            else (((phase[:1] & 0) | 0) if phase < 283 else ((phase[:1] & 0) | 1))
        )
    )
    dut = Rob(
        allocate,
        allocate_tag,
        complete,
        complete_index,
        complete_tag,
        complete_value,
        retire,
        flush,
    )

    expected_allocate_accepted = (
        (
            (
                (
                    ((phase[:1] & 0) | 0)
                    if phase < 1
                    else (((phase[:1] & 0) | 1) if phase < 5 else ((phase[:1] & 0) | 0))
                )
                if phase < 10
                else (
                    ((phase[:1] & 0) | 1)
                    if phase < 12
                    else (
                        ((phase[:1] & 0) | 0) if phase < 15 else ((phase[:1] & 0) | 1)
                    )
                )
            )
            if phase < 17
            else (
                (
                    ((phase[:1] & 0) | 0)
                    if phase < 21
                    else (
                        ((phase[:1] & 0) | 1) if phase < 22 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 23
                else (
                    ((phase[:1] & 0) | 1)
                    if phase < 24
                    else (
                        ((phase[:1] & 0) | 0) if phase < 27 else ((phase[:1] & 0) | 1)
                    )
                )
            )
        )
        if phase < 29
        else (
            (
                (
                    ((phase[:1] & 0) | 0)
                    if phase < 30
                    else (
                        ((phase[:1] & 0) | 1) if phase < 31 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 32
                else (
                    ((phase[:1] & 0) | 1)
                    if phase < 33
                    else (
                        ((phase[:1] & 0) | 0) if phase < 73 else ((phase[:1] & 0) | 1)
                    )
                )
            )
            if phase < 74
            else (
                (
                    ((phase[:1] & 0) | 0)
                    if phase < 77
                    else (
                        ((phase[:1] & 0) | 1) if phase < 78 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 80
                else (
                    (((phase[:1] & 0) | 1) if phase < 81 else ((phase[:1] & 0) | 0))
                    if phase < 82
                    else (
                        ((phase[:1] & 0) | 1) if phase < 83 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
    )
    expected_allocated_index = (
        (
            (
                (((phase[:2] & 0) | 0) if phase < 2 else ((phase[:2] & 0) | 1))
                if phase < 3
                else (
                    ((phase[:2] & 0) | 2)
                    if phase < 4
                    else (((phase[:2] & 0) | 3) if phase < 5 else ((phase[:2] & 0) | 0))
                )
            )
            if phase < 11
            else (
                (
                    ((phase[:2] & 0) | 1)
                    if phase < 12
                    else (
                        ((phase[:2] & 0) | 0) if phase < 15 else ((phase[:2] & 0) | 2)
                    )
                )
                if phase < 16
                else (
                    ((phase[:2] & 0) | 3)
                    if phase < 17
                    else (
                        ((phase[:2] & 0) | 0) if phase < 28 else ((phase[:2] & 0) | 1)
                    )
                )
            )
        )
        if phase < 29
        else (
            (
                (((phase[:2] & 0) | 0) if phase < 30 else ((phase[:2] & 0) | 2))
                if phase < 31
                else (
                    ((phase[:2] & 0) | 0)
                    if phase < 32
                    else (
                        ((phase[:2] & 0) | 3) if phase < 33 else ((phase[:2] & 0) | 0)
                    )
                )
            )
            if phase < 77
            else (
                (
                    ((phase[:2] & 0) | 1)
                    if phase < 78
                    else (
                        ((phase[:2] & 0) | 0) if phase < 80 else ((phase[:2] & 0) | 2)
                    )
                )
                if phase < 81
                else (
                    ((phase[:2] & 0) | 0)
                    if phase < 82
                    else (
                        ((phase[:2] & 0) | 3) if phase < 83 else ((phase[:2] & 0) | 0)
                    )
                )
            )
        )
    )
    expected_complete_accepted = (
        (
            (
                ((phase[:1] & 0) | 0)
                if phase < 7
                else (((phase[:1] & 0) | 1) if phase < 8 else ((phase[:1] & 0) | 0))
            )
            if phase < 9
            else (
                (((phase[:1] & 0) | 1) if phase < 10 else ((phase[:1] & 0) | 0))
                if phase < 11
                else (((phase[:1] & 0) | 1) if phase < 12 else ((phase[:1] & 0) | 0))
            )
        )
        if phase < 13
        else (
            (
                (((phase[:1] & 0) | 1) if phase < 14 else ((phase[:1] & 0) | 0))
                if phase < 15
                else (((phase[:1] & 0) | 1) if phase < 20 else ((phase[:1] & 0) | 0))
            )
            if phase < 25
            else (
                (((phase[:1] & 0) | 1) if phase < 26 else ((phase[:1] & 0) | 0))
                if phase < 50
                else (((phase[:1] & 0) | 1) if phase < 51 else ((phase[:1] & 0) | 0))
            )
        )
    )
    expected_retire_accepted = (
        (
            (((phase[:1] & 0) | 0) if phase < 9 else ((phase[:1] & 0) | 1))
            if phase < 10
            else (
                ((phase[:1] & 0) | 0)
                if phase < 11
                else (((phase[:1] & 0) | 1) if phase < 12 else ((phase[:1] & 0) | 0))
            )
        )
        if phase < 13
        else (
            (
                ((phase[:1] & 0) | 1)
                if phase < 14
                else (((phase[:1] & 0) | 0) if phase < 15 else ((phase[:1] & 0) | 1))
            )
            if phase < 20
            else (
                ((phase[:1] & 0) | 0)
                if phase < 25
                else (((phase[:1] & 0) | 1) if phase < 26 else ((phase[:1] & 0) | 0))
            )
        )
    )
    expected_retired_tag = (
        (
            (
                ((phase[:8] & 0) | 0)
                if phase < 9
                else (((phase[:8] & 0) | 11) if phase < 10 else ((phase[:8] & 0) | 0))
            )
            if phase < 11
            else (
                (((phase[:8] & 0) | 12) if phase < 12 else ((phase[:8] & 0) | 0))
                if phase < 13
                else (((phase[:8] & 0) | 13) if phase < 14 else ((phase[:8] & 0) | 0))
            )
        )
        if phase < 15
        else (
            (
                (((phase[:8] & 0) | 14) if phase < 16 else ((phase[:8] & 0) | 15))
                if phase < 17
                else (((phase[:8] & 0) | 16) if phase < 18 else ((phase[:8] & 0) | 17))
            )
            if phase < 19
            else (
                (((phase[:8] & 0) | 18) if phase < 20 else ((phase[:8] & 0) | 0))
                if phase < 25
                else (((phase[:8] & 0) | 31) if phase < 26 else ((phase[:8] & 0) | 0))
            )
        )
    )
    expected_retired_value = (
        (
            (
                ((phase[:16] & 0) | 0)
                if phase < 9
                else (
                    ((phase[:16] & 0) | 4369) if phase < 10 else ((phase[:16] & 0) | 0)
                )
            )
            if phase < 11
            else (
                (((phase[:16] & 0) | 4642) if phase < 12 else ((phase[:16] & 0) | 0))
                if phase < 13
                else (
                    ((phase[:16] & 0) | 4915) if phase < 14 else ((phase[:16] & 0) | 0)
                )
            )
        )
        if phase < 15
        else (
            (
                (((phase[:16] & 0) | 5188) if phase < 16 else ((phase[:16] & 0) | 5461))
                if phase < 17
                else (
                    ((phase[:16] & 0) | 5734)
                    if phase < 18
                    else ((phase[:16] & 0) | 6007)
                )
            )
            if phase < 19
            else (
                (((phase[:16] & 0) | 6280) if phase < 20 else ((phase[:16] & 0) | 0))
                if phase < 25
                else (
                    ((phase[:16] & 0) | 12593) if phase < 26 else ((phase[:16] & 0) | 0)
                )
            )
        )
    )
    expected_count = (
        (
            (
                (
                    ((phase[:3] & 0) | 0)
                    if phase < 1
                    else (((phase[:3] & 0) | 1) if phase < 2 else ((phase[:3] & 0) | 2))
                )
                if phase < 3
                else (
                    ((phase[:3] & 0) | 3)
                    if phase < 4
                    else (((phase[:3] & 0) | 4) if phase < 9 else ((phase[:3] & 0) | 3))
                )
            )
            if phase < 10
            else (
                (
                    ((phase[:3] & 0) | 4)
                    if phase < 13
                    else (
                        ((phase[:3] & 0) | 3) if phase < 17 else ((phase[:3] & 0) | 2)
                    )
                )
                if phase < 18
                else (
                    ((phase[:3] & 0) | 1)
                    if phase < 19
                    else (
                        ((phase[:3] & 0) | 0) if phase < 21 else ((phase[:3] & 0) | 1)
                    )
                )
            )
        )
        if phase < 22
        else (
            (
                (
                    ((phase[:3] & 0) | 0)
                    if phase < 23
                    else (
                        ((phase[:3] & 0) | 1) if phase < 25 else ((phase[:3] & 0) | 0)
                    )
                )
                if phase < 27
                else (
                    ((phase[:3] & 0) | 1)
                    if phase < 28
                    else (
                        ((phase[:3] & 0) | 2) if phase < 30 else ((phase[:3] & 0) | 3)
                    )
                )
            )
            if phase < 32
            else (
                (
                    ((phase[:3] & 0) | 4)
                    if phase < 72
                    else (
                        ((phase[:3] & 0) | 0) if phase < 73 else ((phase[:3] & 0) | 1)
                    )
                )
                if phase < 77
                else (
                    (((phase[:3] & 0) | 2) if phase < 80 else ((phase[:3] & 0) | 3))
                    if phase < 82
                    else (
                        ((phase[:3] & 0) | 4) if phase < 283 else ((phase[:3] & 0) | 0)
                    )
                )
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 284:
            assert (
                dut.allocate_accepted == expected_allocate_accepted
            ), "rob: allocate_accepted"
            assert (
                dut.allocated_index == expected_allocated_index
            ), "rob: allocated_index"
            assert (
                dut.complete_accepted == expected_complete_accepted
            ), "rob: complete_accepted"
            assert (
                dut.retire_accepted == expected_retire_accepted
            ), "rob: retire_accepted"
            assert dut.retired_tag == expected_retired_tag, "rob: retired_tag"
            assert dut.retired_value == expected_retired_value, "rob: retired_value"
            assert dut.count == expected_count, "rob: count"

        log("info", "rob.allocate_accepted", dut.allocate_accepted)
        log("info", "rob.allocated_index", dut.allocated_index)
        log("info", "rob.complete_accepted", dut.complete_accepted)
        log("info", "rob.retire_accepted", dut.retire_accepted)
        log("info", "rob.retired_tag", dut.retired_tag)
        log("info", "rob.retired_value", dut.retired_value)
        log("info", "rob.count", dut.count)

    check_and_advance()
    advance(phase)

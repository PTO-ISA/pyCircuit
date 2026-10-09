"""Regular-clock counter scenario; original physical-control oracles remain."""

from example_counter.counter import Counter
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseCounter():  # noqa: N802
    phase: bits[64] = 0
    enable = (
        (
            ((phase[:1] & 0) | 1)
            if phase < 256
            else (((phase[:1] & 0) | 0) if phase < 257 else ((phase[:1] & 0) | 1))
        )
        if phase < 259
        else (
            ((phase[:1] & 0) | 0)
            if phase < 261
            else (((phase[:1] & 0) | 1) if phase < 262 else ((phase[:1] & 0) | 0))
        )
    )
    dut = Counter(enable)

    expected_count = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 0)
                                    if phase < 1
                                    else ((phase[:8] & 0) | 1)
                                )
                                if phase < 2
                                else (
                                    ((phase[:8] & 0) | 2)
                                    if phase < 3
                                    else ((phase[:8] & 0) | 3)
                                )
                            )
                            if phase < 4
                            else (
                                (
                                    ((phase[:8] & 0) | 4)
                                    if phase < 5
                                    else ((phase[:8] & 0) | 5)
                                )
                                if phase < 6
                                else (
                                    ((phase[:8] & 0) | 6)
                                    if phase < 7
                                    else ((phase[:8] & 0) | 7)
                                )
                            )
                        )
                        if phase < 8
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 8)
                                    if phase < 9
                                    else ((phase[:8] & 0) | 9)
                                )
                                if phase < 10
                                else (
                                    ((phase[:8] & 0) | 10)
                                    if phase < 11
                                    else ((phase[:8] & 0) | 11)
                                )
                            )
                            if phase < 12
                            else (
                                (
                                    ((phase[:8] & 0) | 12)
                                    if phase < 13
                                    else ((phase[:8] & 0) | 13)
                                )
                                if phase < 14
                                else (
                                    ((phase[:8] & 0) | 14)
                                    if phase < 15
                                    else ((phase[:8] & 0) | 15)
                                )
                            )
                        )
                    )
                    if phase < 16
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 16)
                                    if phase < 17
                                    else ((phase[:8] & 0) | 17)
                                )
                                if phase < 18
                                else (
                                    ((phase[:8] & 0) | 18)
                                    if phase < 19
                                    else ((phase[:8] & 0) | 19)
                                )
                            )
                            if phase < 20
                            else (
                                (
                                    ((phase[:8] & 0) | 20)
                                    if phase < 21
                                    else ((phase[:8] & 0) | 21)
                                )
                                if phase < 22
                                else (
                                    ((phase[:8] & 0) | 22)
                                    if phase < 23
                                    else ((phase[:8] & 0) | 23)
                                )
                            )
                        )
                        if phase < 24
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 24)
                                    if phase < 25
                                    else ((phase[:8] & 0) | 25)
                                )
                                if phase < 26
                                else (
                                    ((phase[:8] & 0) | 26)
                                    if phase < 27
                                    else ((phase[:8] & 0) | 27)
                                )
                            )
                            if phase < 28
                            else (
                                (
                                    ((phase[:8] & 0) | 28)
                                    if phase < 29
                                    else ((phase[:8] & 0) | 29)
                                )
                                if phase < 30
                                else (
                                    ((phase[:8] & 0) | 30)
                                    if phase < 31
                                    else ((phase[:8] & 0) | 31)
                                )
                            )
                        )
                    )
                )
                if phase < 32
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 32)
                                    if phase < 33
                                    else ((phase[:8] & 0) | 33)
                                )
                                if phase < 34
                                else (
                                    ((phase[:8] & 0) | 34)
                                    if phase < 35
                                    else ((phase[:8] & 0) | 35)
                                )
                            )
                            if phase < 36
                            else (
                                (
                                    ((phase[:8] & 0) | 36)
                                    if phase < 37
                                    else ((phase[:8] & 0) | 37)
                                )
                                if phase < 38
                                else (
                                    ((phase[:8] & 0) | 38)
                                    if phase < 39
                                    else ((phase[:8] & 0) | 39)
                                )
                            )
                        )
                        if phase < 40
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 40)
                                    if phase < 41
                                    else ((phase[:8] & 0) | 41)
                                )
                                if phase < 42
                                else (
                                    ((phase[:8] & 0) | 42)
                                    if phase < 43
                                    else ((phase[:8] & 0) | 43)
                                )
                            )
                            if phase < 44
                            else (
                                (
                                    ((phase[:8] & 0) | 44)
                                    if phase < 45
                                    else ((phase[:8] & 0) | 45)
                                )
                                if phase < 46
                                else (
                                    ((phase[:8] & 0) | 46)
                                    if phase < 47
                                    else ((phase[:8] & 0) | 47)
                                )
                            )
                        )
                    )
                    if phase < 48
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 48)
                                    if phase < 49
                                    else ((phase[:8] & 0) | 49)
                                )
                                if phase < 50
                                else (
                                    ((phase[:8] & 0) | 50)
                                    if phase < 51
                                    else ((phase[:8] & 0) | 51)
                                )
                            )
                            if phase < 52
                            else (
                                (
                                    ((phase[:8] & 0) | 52)
                                    if phase < 53
                                    else ((phase[:8] & 0) | 53)
                                )
                                if phase < 54
                                else (
                                    ((phase[:8] & 0) | 54)
                                    if phase < 55
                                    else ((phase[:8] & 0) | 55)
                                )
                            )
                        )
                        if phase < 56
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 56)
                                    if phase < 57
                                    else ((phase[:8] & 0) | 57)
                                )
                                if phase < 58
                                else (
                                    ((phase[:8] & 0) | 58)
                                    if phase < 59
                                    else ((phase[:8] & 0) | 59)
                                )
                            )
                            if phase < 60
                            else (
                                (
                                    ((phase[:8] & 0) | 60)
                                    if phase < 61
                                    else ((phase[:8] & 0) | 61)
                                )
                                if phase < 62
                                else (
                                    ((phase[:8] & 0) | 62)
                                    if phase < 63
                                    else (
                                        ((phase[:8] & 0) | 63)
                                        if phase < 64
                                        else ((phase[:8] & 0) | 64)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 65
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 65)
                                    if phase < 66
                                    else ((phase[:8] & 0) | 66)
                                )
                                if phase < 67
                                else (
                                    ((phase[:8] & 0) | 67)
                                    if phase < 68
                                    else ((phase[:8] & 0) | 68)
                                )
                            )
                            if phase < 69
                            else (
                                (
                                    ((phase[:8] & 0) | 69)
                                    if phase < 70
                                    else ((phase[:8] & 0) | 70)
                                )
                                if phase < 71
                                else (
                                    ((phase[:8] & 0) | 71)
                                    if phase < 72
                                    else ((phase[:8] & 0) | 72)
                                )
                            )
                        )
                        if phase < 73
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 73)
                                    if phase < 74
                                    else ((phase[:8] & 0) | 74)
                                )
                                if phase < 75
                                else (
                                    ((phase[:8] & 0) | 75)
                                    if phase < 76
                                    else ((phase[:8] & 0) | 76)
                                )
                            )
                            if phase < 77
                            else (
                                (
                                    ((phase[:8] & 0) | 77)
                                    if phase < 78
                                    else ((phase[:8] & 0) | 78)
                                )
                                if phase < 79
                                else (
                                    ((phase[:8] & 0) | 79)
                                    if phase < 80
                                    else ((phase[:8] & 0) | 80)
                                )
                            )
                        )
                    )
                    if phase < 81
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 81)
                                    if phase < 82
                                    else ((phase[:8] & 0) | 82)
                                )
                                if phase < 83
                                else (
                                    ((phase[:8] & 0) | 83)
                                    if phase < 84
                                    else ((phase[:8] & 0) | 84)
                                )
                            )
                            if phase < 85
                            else (
                                (
                                    ((phase[:8] & 0) | 85)
                                    if phase < 86
                                    else ((phase[:8] & 0) | 86)
                                )
                                if phase < 87
                                else (
                                    ((phase[:8] & 0) | 87)
                                    if phase < 88
                                    else ((phase[:8] & 0) | 88)
                                )
                            )
                        )
                        if phase < 89
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 89)
                                    if phase < 90
                                    else ((phase[:8] & 0) | 90)
                                )
                                if phase < 91
                                else (
                                    ((phase[:8] & 0) | 91)
                                    if phase < 92
                                    else ((phase[:8] & 0) | 92)
                                )
                            )
                            if phase < 93
                            else (
                                (
                                    ((phase[:8] & 0) | 93)
                                    if phase < 94
                                    else ((phase[:8] & 0) | 94)
                                )
                                if phase < 95
                                else (
                                    ((phase[:8] & 0) | 95)
                                    if phase < 96
                                    else ((phase[:8] & 0) | 96)
                                )
                            )
                        )
                    )
                )
                if phase < 97
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 97)
                                    if phase < 98
                                    else ((phase[:8] & 0) | 98)
                                )
                                if phase < 99
                                else (
                                    ((phase[:8] & 0) | 99)
                                    if phase < 100
                                    else ((phase[:8] & 0) | 100)
                                )
                            )
                            if phase < 101
                            else (
                                (
                                    ((phase[:8] & 0) | 101)
                                    if phase < 102
                                    else ((phase[:8] & 0) | 102)
                                )
                                if phase < 103
                                else (
                                    ((phase[:8] & 0) | 103)
                                    if phase < 104
                                    else ((phase[:8] & 0) | 104)
                                )
                            )
                        )
                        if phase < 105
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 105)
                                    if phase < 106
                                    else ((phase[:8] & 0) | 106)
                                )
                                if phase < 107
                                else (
                                    ((phase[:8] & 0) | 107)
                                    if phase < 108
                                    else ((phase[:8] & 0) | 108)
                                )
                            )
                            if phase < 109
                            else (
                                (
                                    ((phase[:8] & 0) | 109)
                                    if phase < 110
                                    else ((phase[:8] & 0) | 110)
                                )
                                if phase < 111
                                else (
                                    ((phase[:8] & 0) | 111)
                                    if phase < 112
                                    else ((phase[:8] & 0) | 112)
                                )
                            )
                        )
                    )
                    if phase < 113
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 113)
                                    if phase < 114
                                    else ((phase[:8] & 0) | 114)
                                )
                                if phase < 115
                                else (
                                    ((phase[:8] & 0) | 115)
                                    if phase < 116
                                    else ((phase[:8] & 0) | 116)
                                )
                            )
                            if phase < 117
                            else (
                                (
                                    ((phase[:8] & 0) | 117)
                                    if phase < 118
                                    else ((phase[:8] & 0) | 118)
                                )
                                if phase < 119
                                else (
                                    ((phase[:8] & 0) | 119)
                                    if phase < 120
                                    else ((phase[:8] & 0) | 120)
                                )
                            )
                        )
                        if phase < 121
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 121)
                                    if phase < 122
                                    else ((phase[:8] & 0) | 122)
                                )
                                if phase < 123
                                else (
                                    ((phase[:8] & 0) | 123)
                                    if phase < 124
                                    else ((phase[:8] & 0) | 124)
                                )
                            )
                            if phase < 125
                            else (
                                (
                                    ((phase[:8] & 0) | 125)
                                    if phase < 126
                                    else ((phase[:8] & 0) | 126)
                                )
                                if phase < 127
                                else (
                                    ((phase[:8] & 0) | 127)
                                    if phase < 128
                                    else (
                                        ((phase[:8] & 0) | 128)
                                        if phase < 129
                                        else ((phase[:8] & 0) | 129)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 130
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 130)
                                    if phase < 131
                                    else ((phase[:8] & 0) | 131)
                                )
                                if phase < 132
                                else (
                                    ((phase[:8] & 0) | 132)
                                    if phase < 133
                                    else ((phase[:8] & 0) | 133)
                                )
                            )
                            if phase < 134
                            else (
                                (
                                    ((phase[:8] & 0) | 134)
                                    if phase < 135
                                    else ((phase[:8] & 0) | 135)
                                )
                                if phase < 136
                                else (
                                    ((phase[:8] & 0) | 136)
                                    if phase < 137
                                    else ((phase[:8] & 0) | 137)
                                )
                            )
                        )
                        if phase < 138
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 138)
                                    if phase < 139
                                    else ((phase[:8] & 0) | 139)
                                )
                                if phase < 140
                                else (
                                    ((phase[:8] & 0) | 140)
                                    if phase < 141
                                    else ((phase[:8] & 0) | 141)
                                )
                            )
                            if phase < 142
                            else (
                                (
                                    ((phase[:8] & 0) | 142)
                                    if phase < 143
                                    else ((phase[:8] & 0) | 143)
                                )
                                if phase < 144
                                else (
                                    ((phase[:8] & 0) | 144)
                                    if phase < 145
                                    else ((phase[:8] & 0) | 145)
                                )
                            )
                        )
                    )
                    if phase < 146
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 146)
                                    if phase < 147
                                    else ((phase[:8] & 0) | 147)
                                )
                                if phase < 148
                                else (
                                    ((phase[:8] & 0) | 148)
                                    if phase < 149
                                    else ((phase[:8] & 0) | 149)
                                )
                            )
                            if phase < 150
                            else (
                                (
                                    ((phase[:8] & 0) | 150)
                                    if phase < 151
                                    else ((phase[:8] & 0) | 151)
                                )
                                if phase < 152
                                else (
                                    ((phase[:8] & 0) | 152)
                                    if phase < 153
                                    else ((phase[:8] & 0) | 153)
                                )
                            )
                        )
                        if phase < 154
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 154)
                                    if phase < 155
                                    else ((phase[:8] & 0) | 155)
                                )
                                if phase < 156
                                else (
                                    ((phase[:8] & 0) | 156)
                                    if phase < 157
                                    else ((phase[:8] & 0) | 157)
                                )
                            )
                            if phase < 158
                            else (
                                (
                                    ((phase[:8] & 0) | 158)
                                    if phase < 159
                                    else ((phase[:8] & 0) | 159)
                                )
                                if phase < 160
                                else (
                                    ((phase[:8] & 0) | 160)
                                    if phase < 161
                                    else ((phase[:8] & 0) | 161)
                                )
                            )
                        )
                    )
                )
                if phase < 162
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 162)
                                    if phase < 163
                                    else ((phase[:8] & 0) | 163)
                                )
                                if phase < 164
                                else (
                                    ((phase[:8] & 0) | 164)
                                    if phase < 165
                                    else ((phase[:8] & 0) | 165)
                                )
                            )
                            if phase < 166
                            else (
                                (
                                    ((phase[:8] & 0) | 166)
                                    if phase < 167
                                    else ((phase[:8] & 0) | 167)
                                )
                                if phase < 168
                                else (
                                    ((phase[:8] & 0) | 168)
                                    if phase < 169
                                    else ((phase[:8] & 0) | 169)
                                )
                            )
                        )
                        if phase < 170
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 170)
                                    if phase < 171
                                    else ((phase[:8] & 0) | 171)
                                )
                                if phase < 172
                                else (
                                    ((phase[:8] & 0) | 172)
                                    if phase < 173
                                    else ((phase[:8] & 0) | 173)
                                )
                            )
                            if phase < 174
                            else (
                                (
                                    ((phase[:8] & 0) | 174)
                                    if phase < 175
                                    else ((phase[:8] & 0) | 175)
                                )
                                if phase < 176
                                else (
                                    ((phase[:8] & 0) | 176)
                                    if phase < 177
                                    else ((phase[:8] & 0) | 177)
                                )
                            )
                        )
                    )
                    if phase < 178
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 178)
                                    if phase < 179
                                    else ((phase[:8] & 0) | 179)
                                )
                                if phase < 180
                                else (
                                    ((phase[:8] & 0) | 180)
                                    if phase < 181
                                    else ((phase[:8] & 0) | 181)
                                )
                            )
                            if phase < 182
                            else (
                                (
                                    ((phase[:8] & 0) | 182)
                                    if phase < 183
                                    else ((phase[:8] & 0) | 183)
                                )
                                if phase < 184
                                else (
                                    ((phase[:8] & 0) | 184)
                                    if phase < 185
                                    else ((phase[:8] & 0) | 185)
                                )
                            )
                        )
                        if phase < 186
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 186)
                                    if phase < 187
                                    else ((phase[:8] & 0) | 187)
                                )
                                if phase < 188
                                else (
                                    ((phase[:8] & 0) | 188)
                                    if phase < 189
                                    else ((phase[:8] & 0) | 189)
                                )
                            )
                            if phase < 190
                            else (
                                (
                                    ((phase[:8] & 0) | 190)
                                    if phase < 191
                                    else ((phase[:8] & 0) | 191)
                                )
                                if phase < 192
                                else (
                                    ((phase[:8] & 0) | 192)
                                    if phase < 193
                                    else (
                                        ((phase[:8] & 0) | 193)
                                        if phase < 194
                                        else ((phase[:8] & 0) | 194)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 195
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 195)
                                    if phase < 196
                                    else ((phase[:8] & 0) | 196)
                                )
                                if phase < 197
                                else (
                                    ((phase[:8] & 0) | 197)
                                    if phase < 198
                                    else ((phase[:8] & 0) | 198)
                                )
                            )
                            if phase < 199
                            else (
                                (
                                    ((phase[:8] & 0) | 199)
                                    if phase < 200
                                    else ((phase[:8] & 0) | 200)
                                )
                                if phase < 201
                                else (
                                    ((phase[:8] & 0) | 201)
                                    if phase < 202
                                    else ((phase[:8] & 0) | 202)
                                )
                            )
                        )
                        if phase < 203
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 203)
                                    if phase < 204
                                    else ((phase[:8] & 0) | 204)
                                )
                                if phase < 205
                                else (
                                    ((phase[:8] & 0) | 205)
                                    if phase < 206
                                    else ((phase[:8] & 0) | 206)
                                )
                            )
                            if phase < 207
                            else (
                                (
                                    ((phase[:8] & 0) | 207)
                                    if phase < 208
                                    else ((phase[:8] & 0) | 208)
                                )
                                if phase < 209
                                else (
                                    ((phase[:8] & 0) | 209)
                                    if phase < 210
                                    else ((phase[:8] & 0) | 210)
                                )
                            )
                        )
                    )
                    if phase < 211
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 211)
                                    if phase < 212
                                    else ((phase[:8] & 0) | 212)
                                )
                                if phase < 213
                                else (
                                    ((phase[:8] & 0) | 213)
                                    if phase < 214
                                    else ((phase[:8] & 0) | 214)
                                )
                            )
                            if phase < 215
                            else (
                                (
                                    ((phase[:8] & 0) | 215)
                                    if phase < 216
                                    else ((phase[:8] & 0) | 216)
                                )
                                if phase < 217
                                else (
                                    ((phase[:8] & 0) | 217)
                                    if phase < 218
                                    else ((phase[:8] & 0) | 218)
                                )
                            )
                        )
                        if phase < 219
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 219)
                                    if phase < 220
                                    else ((phase[:8] & 0) | 220)
                                )
                                if phase < 221
                                else (
                                    ((phase[:8] & 0) | 221)
                                    if phase < 222
                                    else ((phase[:8] & 0) | 222)
                                )
                            )
                            if phase < 223
                            else (
                                (
                                    ((phase[:8] & 0) | 223)
                                    if phase < 224
                                    else ((phase[:8] & 0) | 224)
                                )
                                if phase < 225
                                else (
                                    ((phase[:8] & 0) | 225)
                                    if phase < 226
                                    else ((phase[:8] & 0) | 226)
                                )
                            )
                        )
                    )
                )
                if phase < 227
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 227)
                                    if phase < 228
                                    else ((phase[:8] & 0) | 228)
                                )
                                if phase < 229
                                else (
                                    ((phase[:8] & 0) | 229)
                                    if phase < 230
                                    else ((phase[:8] & 0) | 230)
                                )
                            )
                            if phase < 231
                            else (
                                (
                                    ((phase[:8] & 0) | 231)
                                    if phase < 232
                                    else ((phase[:8] & 0) | 232)
                                )
                                if phase < 233
                                else (
                                    ((phase[:8] & 0) | 233)
                                    if phase < 234
                                    else ((phase[:8] & 0) | 234)
                                )
                            )
                        )
                        if phase < 235
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 235)
                                    if phase < 236
                                    else ((phase[:8] & 0) | 236)
                                )
                                if phase < 237
                                else (
                                    ((phase[:8] & 0) | 237)
                                    if phase < 238
                                    else ((phase[:8] & 0) | 238)
                                )
                            )
                            if phase < 239
                            else (
                                (
                                    ((phase[:8] & 0) | 239)
                                    if phase < 240
                                    else ((phase[:8] & 0) | 240)
                                )
                                if phase < 241
                                else (
                                    ((phase[:8] & 0) | 241)
                                    if phase < 242
                                    else ((phase[:8] & 0) | 242)
                                )
                            )
                        )
                    )
                    if phase < 243
                    else (
                        (
                            (
                                (
                                    ((phase[:8] & 0) | 243)
                                    if phase < 244
                                    else ((phase[:8] & 0) | 244)
                                )
                                if phase < 245
                                else (
                                    ((phase[:8] & 0) | 245)
                                    if phase < 246
                                    else ((phase[:8] & 0) | 246)
                                )
                            )
                            if phase < 247
                            else (
                                (
                                    ((phase[:8] & 0) | 247)
                                    if phase < 248
                                    else ((phase[:8] & 0) | 248)
                                )
                                if phase < 249
                                else (
                                    ((phase[:8] & 0) | 249)
                                    if phase < 250
                                    else ((phase[:8] & 0) | 250)
                                )
                            )
                        )
                        if phase < 251
                        else (
                            (
                                (
                                    ((phase[:8] & 0) | 251)
                                    if phase < 252
                                    else ((phase[:8] & 0) | 252)
                                )
                                if phase < 253
                                else (
                                    ((phase[:8] & 0) | 253)
                                    if phase < 254
                                    else ((phase[:8] & 0) | 254)
                                )
                            )
                            if phase < 255
                            else (
                                (
                                    ((phase[:8] & 0) | 255)
                                    if phase < 256
                                    else ((phase[:8] & 0) | 0)
                                )
                                if phase < 258
                                else (
                                    ((phase[:8] & 0) | 1)
                                    if phase < 259
                                    else (
                                        ((phase[:8] & 0) | 2)
                                        if phase < 262
                                        else ((phase[:8] & 0) | 3)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 263:
            assert dut.count == expected_count, "counter: count"

        log("info", "counter.count", dut.count)

    check_and_advance()
    advance(phase)

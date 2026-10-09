"""Regular-clock full known-input stream from the retained barrier_pipeline oracle."""

from example_barrier_pipeline.barrier_pipeline import (
    BarrierPipeline,
    LeftToken,
    RightToken,
)
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseBarrierPipeline():  # noqa: N802
    phase: bits[64] = 0
    left_valid = (
        (
            (((phase[:1] & 0) | 0) if phase < 1 else ((phase[:1] & 0) | 1))
            if phase < 4
            else (((phase[:1] & 0) | 0) if phase < 6 else ((phase[:1] & 0) | 1))
        )
        if phase < 14
        else (
            (((phase[:1] & 0) | 0) if phase < 21 else ((phase[:1] & 0) | 1))
            if phase < 328
            else (
                ((phase[:1] & 0) | 0)
                if phase < 336
                else (((phase[:1] & 0) | 1) if phase < 341 else ((phase[:1] & 0) | 0))
            )
        )
    )
    left_value = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 1
                                    else ((phase[:16] & 0) | 42330)
                                )
                                if phase < 2
                                else (
                                    ((phase[:16] & 0) | 42075)
                                    if phase < 3
                                    else (
                                        ((phase[:16] & 0) | 42840)
                                        if phase < 4
                                        else ((phase[:16] & 0) | 42330)
                                    )
                                )
                            )
                            if phase < 5
                            else (
                                (
                                    ((phase[:16] & 0) | 42075)
                                    if phase < 6
                                    else ((phase[:16] & 0) | 41055)
                                )
                                if phase < 7
                                else (
                                    ((phase[:16] & 0) | 41820)
                                    if phase < 8
                                    else (
                                        ((phase[:16] & 0) | 41565)
                                        if phase < 9
                                        else ((phase[:16] & 0) | 44370)
                                    )
                                )
                            )
                        )
                        if phase < 10
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 44115)
                                    if phase < 11
                                    else ((phase[:16] & 0) | 44880)
                                )
                                if phase < 12
                                else (
                                    ((phase[:16] & 0) | 44625)
                                    if phase < 13
                                    else (
                                        ((phase[:16] & 0) | 43350)
                                        if phase < 14
                                        else ((phase[:16] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 21
                            else (
                                (
                                    ((phase[:16] & 0) | 45390)
                                    if phase < 22
                                    else ((phase[:16] & 0) | 45135)
                                )
                                if phase < 23
                                else (
                                    ((phase[:16] & 0) | 45900)
                                    if phase < 24
                                    else (
                                        ((phase[:16] & 0) | 45645)
                                        if phase < 25
                                        else ((phase[:16] & 0) | 48450)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 26
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 48195)
                                    if phase < 27
                                    else ((phase[:16] & 0) | 48960)
                                )
                                if phase < 28
                                else (
                                    ((phase[:16] & 0) | 48705)
                                    if phase < 29
                                    else (
                                        ((phase[:16] & 0) | 34170)
                                        if phase < 30
                                        else ((phase[:16] & 0) | 33915)
                                    )
                                )
                            )
                            if phase < 31
                            else (
                                (
                                    ((phase[:16] & 0) | 34680)
                                    if phase < 32
                                    else ((phase[:16] & 0) | 34425)
                                )
                                if phase < 33
                                else (
                                    ((phase[:16] & 0) | 33150)
                                    if phase < 34
                                    else (
                                        ((phase[:16] & 0) | 32895)
                                        if phase < 35
                                        else ((phase[:16] & 0) | 33660)
                                    )
                                )
                            )
                        )
                        if phase < 36
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 33405)
                                    if phase < 37
                                    else ((phase[:16] & 0) | 36210)
                                )
                                if phase < 38
                                else (
                                    ((phase[:16] & 0) | 35955)
                                    if phase < 39
                                    else (
                                        ((phase[:16] & 0) | 36720)
                                        if phase < 40
                                        else ((phase[:16] & 0) | 36465)
                                    )
                                )
                            )
                            if phase < 41
                            else (
                                (
                                    ((phase[:16] & 0) | 35190)
                                    if phase < 42
                                    else (
                                        ((phase[:16] & 0) | 34935)
                                        if phase < 43
                                        else ((phase[:16] & 0) | 35700)
                                    )
                                )
                                if phase < 44
                                else (
                                    ((phase[:16] & 0) | 35445)
                                    if phase < 45
                                    else (
                                        ((phase[:16] & 0) | 38250)
                                        if phase < 46
                                        else ((phase[:16] & 0) | 37995)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 47
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 38760)
                                    if phase < 48
                                    else ((phase[:16] & 0) | 38505)
                                )
                                if phase < 49
                                else (
                                    ((phase[:16] & 0) | 37230)
                                    if phase < 50
                                    else (
                                        ((phase[:16] & 0) | 36975)
                                        if phase < 51
                                        else ((phase[:16] & 0) | 37740)
                                    )
                                )
                            )
                            if phase < 52
                            else (
                                (
                                    ((phase[:16] & 0) | 37485)
                                    if phase < 53
                                    else ((phase[:16] & 0) | 40290)
                                )
                                if phase < 54
                                else (
                                    ((phase[:16] & 0) | 40800)
                                    if phase < 55
                                    else (
                                        ((phase[:16] & 0) | 0)
                                        if phase < 56
                                        else ((phase[:16] & 0) | 39270)
                                    )
                                )
                            )
                        )
                        if phase < 57
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 39015)
                                    if phase < 58
                                    else ((phase[:16] & 0) | 39780)
                                )
                                if phase < 59
                                else (
                                    ((phase[:16] & 0) | 39525)
                                    if phase < 60
                                    else (
                                        ((phase[:16] & 0) | 58650)
                                        if phase < 61
                                        else ((phase[:16] & 0) | 58395)
                                    )
                                )
                            )
                            if phase < 62
                            else (
                                (
                                    ((phase[:16] & 0) | 59160)
                                    if phase < 63
                                    else ((phase[:16] & 0) | 57630)
                                )
                                if phase < 64
                                else (
                                    ((phase[:16] & 0) | 57375)
                                    if phase < 65
                                    else (
                                        ((phase[:16] & 0) | 58140)
                                        if phase < 66
                                        else ((phase[:16] & 0) | 57885)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 67
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 60690)
                                    if phase < 68
                                    else ((phase[:16] & 0) | 60435)
                                )
                                if phase < 69
                                else (
                                    ((phase[:16] & 0) | 61200)
                                    if phase < 70
                                    else (
                                        ((phase[:16] & 0) | 60945)
                                        if phase < 71
                                        else ((phase[:16] & 0) | 59670)
                                    )
                                )
                            )
                            if phase < 72
                            else (
                                (
                                    ((phase[:16] & 0) | 42330)
                                    if phase < 73
                                    else ((phase[:16] & 0) | 42075)
                                )
                                if phase < 74
                                else (
                                    ((phase[:16] & 0) | 42840)
                                    if phase < 75
                                    else (
                                        ((phase[:16] & 0) | 42585)
                                        if phase < 76
                                        else ((phase[:16] & 0) | 41310)
                                    )
                                )
                            )
                        )
                        if phase < 77
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 41055)
                                    if phase < 78
                                    else ((phase[:16] & 0) | 41820)
                                )
                                if phase < 79
                                else (
                                    ((phase[:16] & 0) | 41565)
                                    if phase < 80
                                    else (
                                        ((phase[:16] & 0) | 44370)
                                        if phase < 81
                                        else ((phase[:16] & 0) | 44115)
                                    )
                                )
                            )
                            if phase < 82
                            else (
                                (
                                    ((phase[:16] & 0) | 44880)
                                    if phase < 83
                                    else (
                                        ((phase[:16] & 0) | 44625)
                                        if phase < 84
                                        else ((phase[:16] & 0) | 43350)
                                    )
                                )
                                if phase < 85
                                else (
                                    ((phase[:16] & 0) | 43095)
                                    if phase < 86
                                    else (
                                        ((phase[:16] & 0) | 43860)
                                        if phase < 87
                                        else ((phase[:16] & 0) | 43605)
                                    )
                                )
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
                            (
                                (
                                    ((phase[:16] & 0) | 46410)
                                    if phase < 89
                                    else ((phase[:16] & 0) | 46155)
                                )
                                if phase < 90
                                else (
                                    ((phase[:16] & 0) | 46920)
                                    if phase < 91
                                    else (
                                        ((phase[:16] & 0) | 46665)
                                        if phase < 92
                                        else ((phase[:16] & 0) | 45390)
                                    )
                                )
                            )
                            if phase < 93
                            else (
                                (
                                    ((phase[:16] & 0) | 45135)
                                    if phase < 94
                                    else ((phase[:16] & 0) | 45900)
                                )
                                if phase < 95
                                else (
                                    ((phase[:16] & 0) | 45645)
                                    if phase < 96
                                    else (
                                        ((phase[:16] & 0) | 48450)
                                        if phase < 97
                                        else ((phase[:16] & 0) | 48195)
                                    )
                                )
                            )
                        )
                        if phase < 98
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 48960)
                                    if phase < 99
                                    else ((phase[:16] & 0) | 48705)
                                )
                                if phase < 100
                                else (
                                    ((phase[:16] & 0) | 47430)
                                    if phase < 101
                                    else (
                                        ((phase[:16] & 0) | 47175)
                                        if phase < 102
                                        else ((phase[:16] & 0) | 47940)
                                    )
                                )
                            )
                            if phase < 103
                            else (
                                (
                                    ((phase[:16] & 0) | 47685)
                                    if phase < 104
                                    else ((phase[:16] & 0) | 34170)
                                )
                                if phase < 105
                                else (
                                    ((phase[:16] & 0) | 33915)
                                    if phase < 106
                                    else (
                                        ((phase[:16] & 0) | 34680)
                                        if phase < 107
                                        else ((phase[:16] & 0) | 34425)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 108
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 33150)
                                    if phase < 109
                                    else ((phase[:16] & 0) | 32895)
                                )
                                if phase < 110
                                else (
                                    ((phase[:16] & 0) | 33660)
                                    if phase < 111
                                    else (
                                        ((phase[:16] & 0) | 33405)
                                        if phase < 112
                                        else ((phase[:16] & 0) | 36210)
                                    )
                                )
                            )
                            if phase < 113
                            else (
                                (
                                    ((phase[:16] & 0) | 35955)
                                    if phase < 114
                                    else ((phase[:16] & 0) | 36720)
                                )
                                if phase < 115
                                else (
                                    ((phase[:16] & 0) | 36465)
                                    if phase < 116
                                    else (
                                        ((phase[:16] & 0) | 35190)
                                        if phase < 117
                                        else ((phase[:16] & 0) | 34935)
                                    )
                                )
                            )
                        )
                        if phase < 118
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 35700)
                                    if phase < 119
                                    else ((phase[:16] & 0) | 35445)
                                )
                                if phase < 120
                                else (
                                    ((phase[:16] & 0) | 38250)
                                    if phase < 121
                                    else (
                                        ((phase[:16] & 0) | 37995)
                                        if phase < 122
                                        else ((phase[:16] & 0) | 38760)
                                    )
                                )
                            )
                            if phase < 123
                            else (
                                (
                                    ((phase[:16] & 0) | 38505)
                                    if phase < 124
                                    else (
                                        ((phase[:16] & 0) | 37230)
                                        if phase < 125
                                        else ((phase[:16] & 0) | 36975)
                                    )
                                )
                                if phase < 126
                                else (
                                    ((phase[:16] & 0) | 37740)
                                    if phase < 127
                                    else (
                                        ((phase[:16] & 0) | 37485)
                                        if phase < 128
                                        else ((phase[:16] & 0) | 40290)
                                    )
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
                                (
                                    ((phase[:16] & 0) | 40035)
                                    if phase < 130
                                    else ((phase[:16] & 0) | 40800)
                                )
                                if phase < 131
                                else (
                                    ((phase[:16] & 0) | 40545)
                                    if phase < 132
                                    else (
                                        ((phase[:16] & 0) | 39270)
                                        if phase < 133
                                        else ((phase[:16] & 0) | 39015)
                                    )
                                )
                            )
                            if phase < 134
                            else (
                                (
                                    ((phase[:16] & 0) | 39780)
                                    if phase < 135
                                    else ((phase[:16] & 0) | 39525)
                                )
                                if phase < 136
                                else (
                                    ((phase[:16] & 0) | 58650)
                                    if phase < 137
                                    else (
                                        ((phase[:16] & 0) | 58395)
                                        if phase < 138
                                        else ((phase[:16] & 0) | 59160)
                                    )
                                )
                            )
                        )
                        if phase < 139
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 58905)
                                    if phase < 140
                                    else ((phase[:16] & 0) | 57630)
                                )
                                if phase < 141
                                else (
                                    ((phase[:16] & 0) | 57375)
                                    if phase < 142
                                    else (
                                        ((phase[:16] & 0) | 58140)
                                        if phase < 143
                                        else ((phase[:16] & 0) | 57885)
                                    )
                                )
                            )
                            if phase < 144
                            else (
                                (
                                    ((phase[:16] & 0) | 60690)
                                    if phase < 145
                                    else ((phase[:16] & 0) | 60435)
                                )
                                if phase < 146
                                else (
                                    ((phase[:16] & 0) | 61200)
                                    if phase < 147
                                    else (
                                        ((phase[:16] & 0) | 60945)
                                        if phase < 148
                                        else ((phase[:16] & 0) | 59670)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 149
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 59415)
                                    if phase < 150
                                    else ((phase[:16] & 0) | 60180)
                                )
                                if phase < 151
                                else (
                                    ((phase[:16] & 0) | 59925)
                                    if phase < 152
                                    else (
                                        ((phase[:16] & 0) | 62730)
                                        if phase < 153
                                        else ((phase[:16] & 0) | 62475)
                                    )
                                )
                            )
                            if phase < 154
                            else (
                                (
                                    ((phase[:16] & 0) | 63240)
                                    if phase < 155
                                    else ((phase[:16] & 0) | 62985)
                                )
                                if phase < 156
                                else (
                                    ((phase[:16] & 0) | 61710)
                                    if phase < 157
                                    else (
                                        ((phase[:16] & 0) | 61455)
                                        if phase < 158
                                        else ((phase[:16] & 0) | 62220)
                                    )
                                )
                            )
                        )
                        if phase < 159
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 61965)
                                    if phase < 160
                                    else ((phase[:16] & 0) | 64770)
                                )
                                if phase < 161
                                else (
                                    ((phase[:16] & 0) | 64515)
                                    if phase < 162
                                    else (
                                        ((phase[:16] & 0) | 65280)
                                        if phase < 163
                                        else ((phase[:16] & 0) | 65025)
                                    )
                                )
                            )
                            if phase < 164
                            else (
                                (
                                    ((phase[:16] & 0) | 63750)
                                    if phase < 165
                                    else (
                                        ((phase[:16] & 0) | 63495)
                                        if phase < 166
                                        else ((phase[:16] & 0) | 64260)
                                    )
                                )
                                if phase < 167
                                else (
                                    ((phase[:16] & 0) | 64005)
                                    if phase < 168
                                    else (
                                        ((phase[:16] & 0) | 50490)
                                        if phase < 169
                                        else ((phase[:16] & 0) | 50235)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 170
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 51000)
                                    if phase < 171
                                    else ((phase[:16] & 0) | 50745)
                                )
                                if phase < 172
                                else (
                                    ((phase[:16] & 0) | 49470)
                                    if phase < 173
                                    else (
                                        ((phase[:16] & 0) | 49215)
                                        if phase < 174
                                        else ((phase[:16] & 0) | 49980)
                                    )
                                )
                            )
                            if phase < 175
                            else (
                                (
                                    ((phase[:16] & 0) | 49725)
                                    if phase < 176
                                    else ((phase[:16] & 0) | 52530)
                                )
                                if phase < 177
                                else (
                                    ((phase[:16] & 0) | 52275)
                                    if phase < 178
                                    else (
                                        ((phase[:16] & 0) | 53040)
                                        if phase < 179
                                        else ((phase[:16] & 0) | 52785)
                                    )
                                )
                            )
                        )
                        if phase < 180
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 51510)
                                    if phase < 181
                                    else ((phase[:16] & 0) | 51255)
                                )
                                if phase < 182
                                else (
                                    ((phase[:16] & 0) | 52020)
                                    if phase < 183
                                    else (
                                        ((phase[:16] & 0) | 51765)
                                        if phase < 184
                                        else ((phase[:16] & 0) | 54570)
                                    )
                                )
                            )
                            if phase < 185
                            else (
                                (
                                    ((phase[:16] & 0) | 54315)
                                    if phase < 186
                                    else ((phase[:16] & 0) | 55080)
                                )
                                if phase < 187
                                else (
                                    ((phase[:16] & 0) | 54825)
                                    if phase < 188
                                    else (
                                        ((phase[:16] & 0) | 53550)
                                        if phase < 189
                                        else ((phase[:16] & 0) | 53295)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 190
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 54060)
                                    if phase < 191
                                    else ((phase[:16] & 0) | 53805)
                                )
                                if phase < 192
                                else (
                                    ((phase[:16] & 0) | 56610)
                                    if phase < 193
                                    else (
                                        ((phase[:16] & 0) | 56355)
                                        if phase < 194
                                        else ((phase[:16] & 0) | 57120)
                                    )
                                )
                            )
                            if phase < 195
                            else (
                                (
                                    ((phase[:16] & 0) | 56865)
                                    if phase < 196
                                    else ((phase[:16] & 0) | 55590)
                                )
                                if phase < 197
                                else (
                                    ((phase[:16] & 0) | 55335)
                                    if phase < 198
                                    else (
                                        ((phase[:16] & 0) | 56100)
                                        if phase < 199
                                        else ((phase[:16] & 0) | 55845)
                                    )
                                )
                            )
                        )
                        if phase < 200
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 9690)
                                    if phase < 201
                                    else ((phase[:16] & 0) | 9435)
                                )
                                if phase < 202
                                else (
                                    ((phase[:16] & 0) | 10200)
                                    if phase < 203
                                    else (
                                        ((phase[:16] & 0) | 9945)
                                        if phase < 204
                                        else ((phase[:16] & 0) | 8670)
                                    )
                                )
                            )
                            if phase < 205
                            else (
                                (
                                    ((phase[:16] & 0) | 8415)
                                    if phase < 206
                                    else (
                                        ((phase[:16] & 0) | 9180)
                                        if phase < 207
                                        else ((phase[:16] & 0) | 8925)
                                    )
                                )
                                if phase < 208
                                else (
                                    ((phase[:16] & 0) | 11730)
                                    if phase < 209
                                    else (
                                        ((phase[:16] & 0) | 11475)
                                        if phase < 210
                                        else ((phase[:16] & 0) | 12240)
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
                                    ((phase[:16] & 0) | 11985)
                                    if phase < 212
                                    else ((phase[:16] & 0) | 10710)
                                )
                                if phase < 213
                                else (
                                    ((phase[:16] & 0) | 10455)
                                    if phase < 214
                                    else (
                                        ((phase[:16] & 0) | 11220)
                                        if phase < 215
                                        else ((phase[:16] & 0) | 10965)
                                    )
                                )
                            )
                            if phase < 216
                            else (
                                (
                                    ((phase[:16] & 0) | 13770)
                                    if phase < 217
                                    else ((phase[:16] & 0) | 13515)
                                )
                                if phase < 218
                                else (
                                    ((phase[:16] & 0) | 14280)
                                    if phase < 219
                                    else (
                                        ((phase[:16] & 0) | 14025)
                                        if phase < 220
                                        else ((phase[:16] & 0) | 12750)
                                    )
                                )
                            )
                        )
                        if phase < 221
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 12495)
                                    if phase < 222
                                    else ((phase[:16] & 0) | 13260)
                                )
                                if phase < 223
                                else (
                                    ((phase[:16] & 0) | 13005)
                                    if phase < 224
                                    else (
                                        ((phase[:16] & 0) | 15810)
                                        if phase < 225
                                        else ((phase[:16] & 0) | 15555)
                                    )
                                )
                            )
                            if phase < 226
                            else (
                                (
                                    ((phase[:16] & 0) | 16320)
                                    if phase < 227
                                    else ((phase[:16] & 0) | 16065)
                                )
                                if phase < 228
                                else (
                                    ((phase[:16] & 0) | 14790)
                                    if phase < 229
                                    else (
                                        ((phase[:16] & 0) | 14535)
                                        if phase < 230
                                        else ((phase[:16] & 0) | 15300)
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
                                    ((phase[:16] & 0) | 15045)
                                    if phase < 232
                                    else ((phase[:16] & 0) | 1530)
                                )
                                if phase < 233
                                else (
                                    ((phase[:16] & 0) | 1275)
                                    if phase < 234
                                    else (
                                        ((phase[:16] & 0) | 2040)
                                        if phase < 235
                                        else ((phase[:16] & 0) | 1785)
                                    )
                                )
                            )
                            if phase < 236
                            else (
                                (
                                    ((phase[:16] & 0) | 510)
                                    if phase < 237
                                    else ((phase[:16] & 0) | 255)
                                )
                                if phase < 238
                                else (
                                    ((phase[:16] & 0) | 1020)
                                    if phase < 239
                                    else (
                                        ((phase[:16] & 0) | 765)
                                        if phase < 240
                                        else ((phase[:16] & 0) | 3570)
                                    )
                                )
                            )
                        )
                        if phase < 241
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 3315)
                                    if phase < 242
                                    else ((phase[:16] & 0) | 4080)
                                )
                                if phase < 243
                                else (
                                    ((phase[:16] & 0) | 3825)
                                    if phase < 244
                                    else (
                                        ((phase[:16] & 0) | 2550)
                                        if phase < 245
                                        else ((phase[:16] & 0) | 2295)
                                    )
                                )
                            )
                            if phase < 246
                            else (
                                (
                                    ((phase[:16] & 0) | 3060)
                                    if phase < 247
                                    else (
                                        ((phase[:16] & 0) | 2805)
                                        if phase < 248
                                        else ((phase[:16] & 0) | 5610)
                                    )
                                )
                                if phase < 249
                                else (
                                    ((phase[:16] & 0) | 5355)
                                    if phase < 250
                                    else (
                                        ((phase[:16] & 0) | 6120)
                                        if phase < 251
                                        else ((phase[:16] & 0) | 5865)
                                    )
                                )
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
                            (
                                (
                                    ((phase[:16] & 0) | 4590)
                                    if phase < 253
                                    else ((phase[:16] & 0) | 4335)
                                )
                                if phase < 254
                                else (
                                    ((phase[:16] & 0) | 5100)
                                    if phase < 255
                                    else (
                                        ((phase[:16] & 0) | 4845)
                                        if phase < 256
                                        else ((phase[:16] & 0) | 7650)
                                    )
                                )
                            )
                            if phase < 257
                            else (
                                (
                                    ((phase[:16] & 0) | 7395)
                                    if phase < 258
                                    else ((phase[:16] & 0) | 8160)
                                )
                                if phase < 259
                                else (
                                    ((phase[:16] & 0) | 7905)
                                    if phase < 260
                                    else (
                                        ((phase[:16] & 0) | 6630)
                                        if phase < 261
                                        else ((phase[:16] & 0) | 6375)
                                    )
                                )
                            )
                        )
                        if phase < 262
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 7140)
                                    if phase < 263
                                    else ((phase[:16] & 0) | 6885)
                                )
                                if phase < 264
                                else (
                                    ((phase[:16] & 0) | 26010)
                                    if phase < 265
                                    else (
                                        ((phase[:16] & 0) | 25755)
                                        if phase < 266
                                        else ((phase[:16] & 0) | 26520)
                                    )
                                )
                            )
                            if phase < 267
                            else (
                                (
                                    ((phase[:16] & 0) | 26265)
                                    if phase < 268
                                    else ((phase[:16] & 0) | 24990)
                                )
                                if phase < 269
                                else (
                                    ((phase[:16] & 0) | 24735)
                                    if phase < 270
                                    else (
                                        ((phase[:16] & 0) | 25500)
                                        if phase < 271
                                        else ((phase[:16] & 0) | 25245)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 272
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 28050)
                                    if phase < 273
                                    else ((phase[:16] & 0) | 27795)
                                )
                                if phase < 274
                                else (
                                    ((phase[:16] & 0) | 28560)
                                    if phase < 275
                                    else (
                                        ((phase[:16] & 0) | 28305)
                                        if phase < 276
                                        else ((phase[:16] & 0) | 27030)
                                    )
                                )
                            )
                            if phase < 277
                            else (
                                (
                                    ((phase[:16] & 0) | 26775)
                                    if phase < 278
                                    else ((phase[:16] & 0) | 27540)
                                )
                                if phase < 279
                                else (
                                    ((phase[:16] & 0) | 27285)
                                    if phase < 280
                                    else (
                                        ((phase[:16] & 0) | 30090)
                                        if phase < 281
                                        else ((phase[:16] & 0) | 29835)
                                    )
                                )
                            )
                        )
                        if phase < 282
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 30600)
                                    if phase < 283
                                    else ((phase[:16] & 0) | 30345)
                                )
                                if phase < 284
                                else (
                                    ((phase[:16] & 0) | 29070)
                                    if phase < 285
                                    else (
                                        ((phase[:16] & 0) | 28815)
                                        if phase < 286
                                        else ((phase[:16] & 0) | 29580)
                                    )
                                )
                            )
                            if phase < 287
                            else (
                                (
                                    ((phase[:16] & 0) | 29325)
                                    if phase < 288
                                    else (
                                        ((phase[:16] & 0) | 32130)
                                        if phase < 289
                                        else ((phase[:16] & 0) | 31875)
                                    )
                                )
                                if phase < 290
                                else (
                                    ((phase[:16] & 0) | 32640)
                                    if phase < 291
                                    else (
                                        ((phase[:16] & 0) | 32385)
                                        if phase < 292
                                        else ((phase[:16] & 0) | 31110)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 293
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 30855)
                                    if phase < 294
                                    else ((phase[:16] & 0) | 31620)
                                )
                                if phase < 295
                                else (
                                    ((phase[:16] & 0) | 31365)
                                    if phase < 296
                                    else (
                                        ((phase[:16] & 0) | 17850)
                                        if phase < 297
                                        else ((phase[:16] & 0) | 17595)
                                    )
                                )
                            )
                            if phase < 298
                            else (
                                (
                                    ((phase[:16] & 0) | 18360)
                                    if phase < 299
                                    else ((phase[:16] & 0) | 18105)
                                )
                                if phase < 300
                                else (
                                    ((phase[:16] & 0) | 16830)
                                    if phase < 301
                                    else (
                                        ((phase[:16] & 0) | 16575)
                                        if phase < 302
                                        else ((phase[:16] & 0) | 17340)
                                    )
                                )
                            )
                        )
                        if phase < 303
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 17085)
                                    if phase < 304
                                    else ((phase[:16] & 0) | 19890)
                                )
                                if phase < 305
                                else (
                                    ((phase[:16] & 0) | 19635)
                                    if phase < 306
                                    else (
                                        ((phase[:16] & 0) | 20400)
                                        if phase < 307
                                        else ((phase[:16] & 0) | 20145)
                                    )
                                )
                            )
                            if phase < 308
                            else (
                                (
                                    ((phase[:16] & 0) | 18870)
                                    if phase < 309
                                    else ((phase[:16] & 0) | 18615)
                                )
                                if phase < 310
                                else (
                                    ((phase[:16] & 0) | 19380)
                                    if phase < 311
                                    else (
                                        ((phase[:16] & 0) | 19125)
                                        if phase < 312
                                        else ((phase[:16] & 0) | 21930)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 313
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 21675)
                                    if phase < 314
                                    else ((phase[:16] & 0) | 22440)
                                )
                                if phase < 315
                                else (
                                    ((phase[:16] & 0) | 22185)
                                    if phase < 316
                                    else (
                                        ((phase[:16] & 0) | 20910)
                                        if phase < 317
                                        else ((phase[:16] & 0) | 20655)
                                    )
                                )
                            )
                            if phase < 318
                            else (
                                (
                                    ((phase[:16] & 0) | 21420)
                                    if phase < 319
                                    else ((phase[:16] & 0) | 21165)
                                )
                                if phase < 320
                                else (
                                    ((phase[:16] & 0) | 23970)
                                    if phase < 321
                                    else (
                                        ((phase[:16] & 0) | 23715)
                                        if phase < 322
                                        else ((phase[:16] & 0) | 24480)
                                    )
                                )
                            )
                        )
                        if phase < 323
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 24225)
                                    if phase < 324
                                    else ((phase[:16] & 0) | 22950)
                                )
                                if phase < 325
                                else (
                                    ((phase[:16] & 0) | 22695)
                                    if phase < 326
                                    else (
                                        ((phase[:16] & 0) | 23460)
                                        if phase < 327
                                        else ((phase[:16] & 0) | 23205)
                                    )
                                )
                            )
                            if phase < 328
                            else (
                                (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 336
                                    else (
                                        ((phase[:16] & 0) | 62730)
                                        if phase < 337
                                        else ((phase[:16] & 0) | 62475)
                                    )
                                )
                                if phase < 338
                                else (
                                    ((phase[:16] & 0) | 63240)
                                    if phase < 339
                                    else (
                                        ((phase[:16] & 0) | 62985)
                                        if phase < 340
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
    left_take = (
        (
            (
                ((phase[:1] & 0) | 0)
                if phase < 6
                else (((phase[:1] & 0) | 1) if phase < 20 else ((phase[:1] & 0) | 0))
            )
            if phase < 26
            else (
                ((phase[:1] & 0) | 1)
                if phase < 29
                else (((phase[:1] & 0) | 0) if phase < 30 else ((phase[:1] & 0) | 1))
            )
        )
        if phase < 54
        else (
            (
                ((phase[:1] & 0) | 0)
                if phase < 55
                else (((phase[:1] & 0) | 1) if phase < 56 else ((phase[:1] & 0) | 0))
            )
            if phase < 63
            else (
                ((phase[:1] & 0) | 1)
                if phase < 336
                else (((phase[:1] & 0) | 0) if phase < 340 else ((phase[:1] & 0) | 1))
            )
        )
    )
    right_valid = (
        (
            ((phase[:1] & 0) | 0)
            if phase < 4
            else (((phase[:1] & 0) | 1) if phase < 14 else ((phase[:1] & 0) | 0))
        )
        if phase < 21
        else (
            (((phase[:1] & 0) | 1) if phase < 328 else ((phase[:1] & 0) | 0))
            if phase < 336
            else (((phase[:1] & 0) | 1) if phase < 341 else ((phase[:1] & 0) | 0))
        )
    )
    right_value = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 0)
                                    if phase < 1
                                    else ((phase[:32] & 0) | 2309737967)
                                )
                                if phase < 2
                                else (
                                    ((phase[:32] & 0) | 2292894958)
                                    if phase < 3
                                    else (
                                        ((phase[:32] & 0) | 2343161837)
                                        if phase < 4
                                        else ((phase[:32] & 0) | 2309737967)
                                    )
                                )
                            )
                            if phase < 5
                            else (
                                (
                                    ((phase[:32] & 0) | 2292894958)
                                    if phase < 6
                                    else ((phase[:32] & 0) | 2360264938)
                                )
                                if phase < 7
                                else (
                                    ((phase[:32] & 0) | 2410531817)
                                    if phase < 8
                                    else (
                                        ((phase[:32] & 0) | 2393688808)
                                        if phase < 9
                                        else ((phase[:32] & 0) | 2174993895)
                                    )
                                )
                            )
                        )
                        if phase < 10
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 2158150886)
                                    if phase < 11
                                    else ((phase[:32] & 0) | 2208417765)
                                )
                                if phase < 12
                                else (
                                    ((phase[:32] & 0) | 2191574756)
                                    if phase < 13
                                    else (
                                        ((phase[:32] & 0) | 2242363875)
                                        if phase < 14
                                        else ((phase[:32] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 21
                            else (
                                (
                                    ((phase[:32] & 0) | 2646596091)
                                    if phase < 22
                                    else ((phase[:32] & 0) | 2629753082)
                                )
                                if phase < 23
                                else (
                                    ((phase[:32] & 0) | 2680019961)
                                    if phase < 24
                                    else (
                                        ((phase[:32] & 0) | 2663176952)
                                        if phase < 25
                                        else ((phase[:32] & 0) | 2444482039)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 26
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 2427639030)
                                    if phase < 27
                                    else ((phase[:32] & 0) | 2477905909)
                                )
                                if phase < 28
                                else (
                                    ((phase[:32] & 0) | 2461062900)
                                    if phase < 29
                                    else (
                                        ((phase[:32] & 0) | 2844519887)
                                        if phase < 30
                                        else ((phase[:32] & 0) | 2827676878)
                                    )
                                )
                            )
                            if phase < 31
                            else (
                                (
                                    ((phase[:32] & 0) | 2877943757)
                                    if phase < 32
                                    else ((phase[:32] & 0) | 2861100748)
                                )
                                if phase < 33
                                else (
                                    ((phase[:32] & 0) | 2911889867)
                                    if phase < 34
                                    else (
                                        ((phase[:32] & 0) | 2895046858)
                                        if phase < 35
                                        else ((phase[:32] & 0) | 2945313737)
                                    )
                                )
                            )
                        )
                        if phase < 36
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 2928470728)
                                    if phase < 37
                                    else ((phase[:32] & 0) | 2709775815)
                                )
                                if phase < 38
                                else (
                                    ((phase[:32] & 0) | 2692932806)
                                    if phase < 39
                                    else (
                                        ((phase[:32] & 0) | 2743199685)
                                        if phase < 40
                                        else ((phase[:32] & 0) | 2726356676)
                                    )
                                )
                            )
                            if phase < 41
                            else (
                                (
                                    ((phase[:32] & 0) | 2777145795)
                                    if phase < 42
                                    else (
                                        ((phase[:32] & 0) | 2760302786)
                                        if phase < 43
                                        else ((phase[:32] & 0) | 2810569665)
                                    )
                                )
                                if phase < 44
                                else (
                                    ((phase[:32] & 0) | 2793726656)
                                    if phase < 45
                                    else (
                                        ((phase[:32] & 0) | 3114008031)
                                        if phase < 46
                                        else ((phase[:32] & 0) | 3097165022)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 47
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 3147431901)
                                    if phase < 48
                                    else ((phase[:32] & 0) | 3130588892)
                                )
                                if phase < 49
                                else (
                                    ((phase[:32] & 0) | 3181378011)
                                    if phase < 50
                                    else (
                                        ((phase[:32] & 0) | 3164535002)
                                        if phase < 51
                                        else ((phase[:32] & 0) | 3214801881)
                                    )
                                )
                            )
                            if phase < 52
                            else (
                                (
                                    ((phase[:32] & 0) | 3197958872)
                                    if phase < 53
                                    else ((phase[:32] & 0) | 2979263959)
                                )
                                if phase < 54
                                else (
                                    ((phase[:32] & 0) | 3012687829)
                                    if phase < 55
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 56
                                        else ((phase[:32] & 0) | 3046633939)
                                    )
                                )
                            )
                        )
                        if phase < 57
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 3029790930)
                                    if phase < 58
                                    else ((phase[:32] & 0) | 3080057809)
                                )
                                if phase < 59
                                else (
                                    ((phase[:32] & 0) | 3063214800)
                                    if phase < 60
                                    else (
                                        ((phase[:32] & 0) | 3387657647)
                                        if phase < 61
                                        else ((phase[:32] & 0) | 3370814638)
                                    )
                                )
                            )
                            if phase < 62
                            else (
                                (
                                    ((phase[:32] & 0) | 3421081517)
                                    if phase < 63
                                    else ((phase[:32] & 0) | 3455027627)
                                )
                                if phase < 64
                                else (
                                    ((phase[:32] & 0) | 3438184618)
                                    if phase < 65
                                    else (
                                        ((phase[:32] & 0) | 3488451497)
                                        if phase < 66
                                        else ((phase[:32] & 0) | 3471608488)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 67
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 3252913575)
                                    if phase < 68
                                    else ((phase[:32] & 0) | 3236070566)
                                )
                                if phase < 69
                                else (
                                    ((phase[:32] & 0) | 3286337445)
                                    if phase < 70
                                    else (
                                        ((phase[:32] & 0) | 3269494436)
                                        if phase < 71
                                        else ((phase[:32] & 0) | 3320283555)
                                    )
                                )
                            )
                            if phase < 72
                            else (
                                (
                                    ((phase[:32] & 0) | 2309737967)
                                    if phase < 73
                                    else ((phase[:32] & 0) | 2292894958)
                                )
                                if phase < 74
                                else (
                                    ((phase[:32] & 0) | 2343161837)
                                    if phase < 75
                                    else (
                                        ((phase[:32] & 0) | 2326318828)
                                        if phase < 76
                                        else ((phase[:32] & 0) | 2377107947)
                                    )
                                )
                            )
                        )
                        if phase < 77
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 2360264938)
                                    if phase < 78
                                    else ((phase[:32] & 0) | 2410531817)
                                )
                                if phase < 79
                                else (
                                    ((phase[:32] & 0) | 2393688808)
                                    if phase < 80
                                    else (
                                        ((phase[:32] & 0) | 2174993895)
                                        if phase < 81
                                        else ((phase[:32] & 0) | 2158150886)
                                    )
                                )
                            )
                            if phase < 82
                            else (
                                (
                                    ((phase[:32] & 0) | 2208417765)
                                    if phase < 83
                                    else (
                                        ((phase[:32] & 0) | 2191574756)
                                        if phase < 84
                                        else ((phase[:32] & 0) | 2242363875)
                                    )
                                )
                                if phase < 85
                                else (
                                    ((phase[:32] & 0) | 2225520866)
                                    if phase < 86
                                    else (
                                        ((phase[:32] & 0) | 2275787745)
                                        if phase < 87
                                        else ((phase[:32] & 0) | 2258944736)
                                    )
                                )
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
                            (
                                (
                                    ((phase[:32] & 0) | 2579226111)
                                    if phase < 89
                                    else ((phase[:32] & 0) | 2562383102)
                                )
                                if phase < 90
                                else (
                                    ((phase[:32] & 0) | 2612649981)
                                    if phase < 91
                                    else (
                                        ((phase[:32] & 0) | 2595806972)
                                        if phase < 92
                                        else ((phase[:32] & 0) | 2646596091)
                                    )
                                )
                            )
                            if phase < 93
                            else (
                                (
                                    ((phase[:32] & 0) | 2629753082)
                                    if phase < 94
                                    else ((phase[:32] & 0) | 2680019961)
                                )
                                if phase < 95
                                else (
                                    ((phase[:32] & 0) | 2663176952)
                                    if phase < 96
                                    else (
                                        ((phase[:32] & 0) | 2444482039)
                                        if phase < 97
                                        else ((phase[:32] & 0) | 2427639030)
                                    )
                                )
                            )
                        )
                        if phase < 98
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 2477905909)
                                    if phase < 99
                                    else ((phase[:32] & 0) | 2461062900)
                                )
                                if phase < 100
                                else (
                                    ((phase[:32] & 0) | 2511852019)
                                    if phase < 101
                                    else (
                                        ((phase[:32] & 0) | 2495009010)
                                        if phase < 102
                                        else ((phase[:32] & 0) | 2545275889)
                                    )
                                )
                            )
                            if phase < 103
                            else (
                                (
                                    ((phase[:32] & 0) | 2528432880)
                                    if phase < 104
                                    else ((phase[:32] & 0) | 2844519887)
                                )
                                if phase < 105
                                else (
                                    ((phase[:32] & 0) | 2827676878)
                                    if phase < 106
                                    else (
                                        ((phase[:32] & 0) | 2877943757)
                                        if phase < 107
                                        else ((phase[:32] & 0) | 2861100748)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 108
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 2911889867)
                                    if phase < 109
                                    else ((phase[:32] & 0) | 2895046858)
                                )
                                if phase < 110
                                else (
                                    ((phase[:32] & 0) | 2945313737)
                                    if phase < 111
                                    else (
                                        ((phase[:32] & 0) | 2928470728)
                                        if phase < 112
                                        else ((phase[:32] & 0) | 2709775815)
                                    )
                                )
                            )
                            if phase < 113
                            else (
                                (
                                    ((phase[:32] & 0) | 2692932806)
                                    if phase < 114
                                    else ((phase[:32] & 0) | 2743199685)
                                )
                                if phase < 115
                                else (
                                    ((phase[:32] & 0) | 2726356676)
                                    if phase < 116
                                    else (
                                        ((phase[:32] & 0) | 2777145795)
                                        if phase < 117
                                        else ((phase[:32] & 0) | 2760302786)
                                    )
                                )
                            )
                        )
                        if phase < 118
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 2810569665)
                                    if phase < 119
                                    else ((phase[:32] & 0) | 2793726656)
                                )
                                if phase < 120
                                else (
                                    ((phase[:32] & 0) | 3114008031)
                                    if phase < 121
                                    else (
                                        ((phase[:32] & 0) | 3097165022)
                                        if phase < 122
                                        else ((phase[:32] & 0) | 3147431901)
                                    )
                                )
                            )
                            if phase < 123
                            else (
                                (
                                    ((phase[:32] & 0) | 3130588892)
                                    if phase < 124
                                    else (
                                        ((phase[:32] & 0) | 3181378011)
                                        if phase < 125
                                        else ((phase[:32] & 0) | 3164535002)
                                    )
                                )
                                if phase < 126
                                else (
                                    ((phase[:32] & 0) | 3214801881)
                                    if phase < 127
                                    else (
                                        ((phase[:32] & 0) | 3197958872)
                                        if phase < 128
                                        else ((phase[:32] & 0) | 2979263959)
                                    )
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
                                (
                                    ((phase[:32] & 0) | 2962420950)
                                    if phase < 130
                                    else ((phase[:32] & 0) | 3012687829)
                                )
                                if phase < 131
                                else (
                                    ((phase[:32] & 0) | 2995844820)
                                    if phase < 132
                                    else (
                                        ((phase[:32] & 0) | 3046633939)
                                        if phase < 133
                                        else ((phase[:32] & 0) | 3029790930)
                                    )
                                )
                            )
                            if phase < 134
                            else (
                                (
                                    ((phase[:32] & 0) | 3080057809)
                                    if phase < 135
                                    else ((phase[:32] & 0) | 3063214800)
                                )
                                if phase < 136
                                else (
                                    ((phase[:32] & 0) | 3387657647)
                                    if phase < 137
                                    else (
                                        ((phase[:32] & 0) | 3370814638)
                                        if phase < 138
                                        else ((phase[:32] & 0) | 3421081517)
                                    )
                                )
                            )
                        )
                        if phase < 139
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 3404238508)
                                    if phase < 140
                                    else ((phase[:32] & 0) | 3455027627)
                                )
                                if phase < 141
                                else (
                                    ((phase[:32] & 0) | 3438184618)
                                    if phase < 142
                                    else (
                                        ((phase[:32] & 0) | 3488451497)
                                        if phase < 143
                                        else ((phase[:32] & 0) | 3471608488)
                                    )
                                )
                            )
                            if phase < 144
                            else (
                                (
                                    ((phase[:32] & 0) | 3252913575)
                                    if phase < 145
                                    else ((phase[:32] & 0) | 3236070566)
                                )
                                if phase < 146
                                else (
                                    ((phase[:32] & 0) | 3286337445)
                                    if phase < 147
                                    else (
                                        ((phase[:32] & 0) | 3269494436)
                                        if phase < 148
                                        else ((phase[:32] & 0) | 3320283555)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 149
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 3303440546)
                                    if phase < 150
                                    else ((phase[:32] & 0) | 3353707425)
                                )
                                if phase < 151
                                else (
                                    ((phase[:32] & 0) | 3336864416)
                                    if phase < 152
                                    else (
                                        ((phase[:32] & 0) | 3657145791)
                                        if phase < 153
                                        else ((phase[:32] & 0) | 3640302782)
                                    )
                                )
                            )
                            if phase < 154
                            else (
                                (
                                    ((phase[:32] & 0) | 3690569661)
                                    if phase < 155
                                    else ((phase[:32] & 0) | 3673726652)
                                )
                                if phase < 156
                                else (
                                    ((phase[:32] & 0) | 3724515771)
                                    if phase < 157
                                    else (
                                        ((phase[:32] & 0) | 3707672762)
                                        if phase < 158
                                        else ((phase[:32] & 0) | 3757939641)
                                    )
                                )
                            )
                        )
                        if phase < 159
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 3741096632)
                                    if phase < 160
                                    else ((phase[:32] & 0) | 3522401719)
                                )
                                if phase < 161
                                else (
                                    ((phase[:32] & 0) | 3505558710)
                                    if phase < 162
                                    else (
                                        ((phase[:32] & 0) | 3555825589)
                                        if phase < 163
                                        else ((phase[:32] & 0) | 3538982580)
                                    )
                                )
                            )
                            if phase < 164
                            else (
                                (
                                    ((phase[:32] & 0) | 3589771699)
                                    if phase < 165
                                    else (
                                        ((phase[:32] & 0) | 3572928690)
                                        if phase < 166
                                        else ((phase[:32] & 0) | 3623195569)
                                    )
                                )
                                if phase < 167
                                else (
                                    ((phase[:32] & 0) | 3606352560)
                                    if phase < 168
                                    else (
                                        ((phase[:32] & 0) | 3922439567)
                                        if phase < 169
                                        else ((phase[:32] & 0) | 3905596558)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 170
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 3955863437)
                                    if phase < 171
                                    else ((phase[:32] & 0) | 3939020428)
                                )
                                if phase < 172
                                else (
                                    ((phase[:32] & 0) | 3989809547)
                                    if phase < 173
                                    else (
                                        ((phase[:32] & 0) | 3972966538)
                                        if phase < 174
                                        else ((phase[:32] & 0) | 4023233417)
                                    )
                                )
                            )
                            if phase < 175
                            else (
                                (
                                    ((phase[:32] & 0) | 4006390408)
                                    if phase < 176
                                    else ((phase[:32] & 0) | 3787695495)
                                )
                                if phase < 177
                                else (
                                    ((phase[:32] & 0) | 3770852486)
                                    if phase < 178
                                    else (
                                        ((phase[:32] & 0) | 3821119365)
                                        if phase < 179
                                        else ((phase[:32] & 0) | 3804276356)
                                    )
                                )
                            )
                        )
                        if phase < 180
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 3855065475)
                                    if phase < 181
                                    else ((phase[:32] & 0) | 3838222466)
                                )
                                if phase < 182
                                else (
                                    ((phase[:32] & 0) | 3888489345)
                                    if phase < 183
                                    else (
                                        ((phase[:32] & 0) | 3871646336)
                                        if phase < 184
                                        else ((phase[:32] & 0) | 4191927711)
                                    )
                                )
                            )
                            if phase < 185
                            else (
                                (
                                    ((phase[:32] & 0) | 4175084702)
                                    if phase < 186
                                    else ((phase[:32] & 0) | 4225351581)
                                )
                                if phase < 187
                                else (
                                    ((phase[:32] & 0) | 4208508572)
                                    if phase < 188
                                    else (
                                        ((phase[:32] & 0) | 4259297691)
                                        if phase < 189
                                        else ((phase[:32] & 0) | 4242454682)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 190
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 4292721561)
                                    if phase < 191
                                    else ((phase[:32] & 0) | 4275878552)
                                )
                                if phase < 192
                                else (
                                    ((phase[:32] & 0) | 4057183639)
                                    if phase < 193
                                    else (
                                        ((phase[:32] & 0) | 4040340630)
                                        if phase < 194
                                        else ((phase[:32] & 0) | 4090607509)
                                    )
                                )
                            )
                            if phase < 195
                            else (
                                (
                                    ((phase[:32] & 0) | 4073764500)
                                    if phase < 196
                                    else ((phase[:32] & 0) | 4124553619)
                                )
                                if phase < 197
                                else (
                                    ((phase[:32] & 0) | 4107710610)
                                    if phase < 198
                                    else (
                                        ((phase[:32] & 0) | 4157977489)
                                        if phase < 199
                                        else ((phase[:32] & 0) | 4141134480)
                                    )
                                )
                            )
                        )
                        if phase < 200
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 153832815)
                                    if phase < 201
                                    else ((phase[:32] & 0) | 136989806)
                                )
                                if phase < 202
                                else (
                                    ((phase[:32] & 0) | 187256685)
                                    if phase < 203
                                    else (
                                        ((phase[:32] & 0) | 170413676)
                                        if phase < 204
                                        else ((phase[:32] & 0) | 221202795)
                                    )
                                )
                            )
                            if phase < 205
                            else (
                                (
                                    ((phase[:32] & 0) | 204359786)
                                    if phase < 206
                                    else (
                                        ((phase[:32] & 0) | 254626665)
                                        if phase < 207
                                        else ((phase[:32] & 0) | 237783656)
                                    )
                                )
                                if phase < 208
                                else (
                                    ((phase[:32] & 0) | 19088743)
                                    if phase < 209
                                    else (
                                        ((phase[:32] & 0) | 2245734)
                                        if phase < 210
                                        else ((phase[:32] & 0) | 52512613)
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
                                    ((phase[:32] & 0) | 35669604)
                                    if phase < 212
                                    else ((phase[:32] & 0) | 86458723)
                                )
                                if phase < 213
                                else (
                                    ((phase[:32] & 0) | 69615714)
                                    if phase < 214
                                    else (
                                        ((phase[:32] & 0) | 119882593)
                                        if phase < 215
                                        else ((phase[:32] & 0) | 103039584)
                                    )
                                )
                            )
                            if phase < 216
                            else (
                                (
                                    ((phase[:32] & 0) | 423320959)
                                    if phase < 217
                                    else ((phase[:32] & 0) | 406477950)
                                )
                                if phase < 218
                                else (
                                    ((phase[:32] & 0) | 456744829)
                                    if phase < 219
                                    else (
                                        ((phase[:32] & 0) | 439901820)
                                        if phase < 220
                                        else ((phase[:32] & 0) | 490690939)
                                    )
                                )
                            )
                        )
                        if phase < 221
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 473847930)
                                    if phase < 222
                                    else ((phase[:32] & 0) | 524114809)
                                )
                                if phase < 223
                                else (
                                    ((phase[:32] & 0) | 507271800)
                                    if phase < 224
                                    else (
                                        ((phase[:32] & 0) | 288576887)
                                        if phase < 225
                                        else ((phase[:32] & 0) | 271733878)
                                    )
                                )
                            )
                            if phase < 226
                            else (
                                (
                                    ((phase[:32] & 0) | 322000757)
                                    if phase < 227
                                    else ((phase[:32] & 0) | 305157748)
                                )
                                if phase < 228
                                else (
                                    ((phase[:32] & 0) | 355946867)
                                    if phase < 229
                                    else (
                                        ((phase[:32] & 0) | 339103858)
                                        if phase < 230
                                        else ((phase[:32] & 0) | 389370737)
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
                                    ((phase[:32] & 0) | 372527728)
                                    if phase < 232
                                    else ((phase[:32] & 0) | 688614735)
                                )
                                if phase < 233
                                else (
                                    ((phase[:32] & 0) | 671771726)
                                    if phase < 234
                                    else (
                                        ((phase[:32] & 0) | 722038605)
                                        if phase < 235
                                        else ((phase[:32] & 0) | 705195596)
                                    )
                                )
                            )
                            if phase < 236
                            else (
                                (
                                    ((phase[:32] & 0) | 755984715)
                                    if phase < 237
                                    else ((phase[:32] & 0) | 739141706)
                                )
                                if phase < 238
                                else (
                                    ((phase[:32] & 0) | 789408585)
                                    if phase < 239
                                    else (
                                        ((phase[:32] & 0) | 772565576)
                                        if phase < 240
                                        else ((phase[:32] & 0) | 553870663)
                                    )
                                )
                            )
                        )
                        if phase < 241
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 537027654)
                                    if phase < 242
                                    else ((phase[:32] & 0) | 587294533)
                                )
                                if phase < 243
                                else (
                                    ((phase[:32] & 0) | 570451524)
                                    if phase < 244
                                    else (
                                        ((phase[:32] & 0) | 621240643)
                                        if phase < 245
                                        else ((phase[:32] & 0) | 604397634)
                                    )
                                )
                            )
                            if phase < 246
                            else (
                                (
                                    ((phase[:32] & 0) | 654664513)
                                    if phase < 247
                                    else (
                                        ((phase[:32] & 0) | 637821504)
                                        if phase < 248
                                        else ((phase[:32] & 0) | 958102879)
                                    )
                                )
                                if phase < 249
                                else (
                                    ((phase[:32] & 0) | 941259870)
                                    if phase < 250
                                    else (
                                        ((phase[:32] & 0) | 991526749)
                                        if phase < 251
                                        else ((phase[:32] & 0) | 974683740)
                                    )
                                )
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
                            (
                                (
                                    ((phase[:32] & 0) | 1025472859)
                                    if phase < 253
                                    else ((phase[:32] & 0) | 1008629850)
                                )
                                if phase < 254
                                else (
                                    ((phase[:32] & 0) | 1058896729)
                                    if phase < 255
                                    else (
                                        ((phase[:32] & 0) | 1042053720)
                                        if phase < 256
                                        else ((phase[:32] & 0) | 823358807)
                                    )
                                )
                            )
                            if phase < 257
                            else (
                                (
                                    ((phase[:32] & 0) | 806515798)
                                    if phase < 258
                                    else ((phase[:32] & 0) | 856782677)
                                )
                                if phase < 259
                                else (
                                    ((phase[:32] & 0) | 839939668)
                                    if phase < 260
                                    else (
                                        ((phase[:32] & 0) | 890728787)
                                        if phase < 261
                                        else ((phase[:32] & 0) | 873885778)
                                    )
                                )
                            )
                        )
                        if phase < 262
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 924152657)
                                    if phase < 263
                                    else ((phase[:32] & 0) | 907309648)
                                )
                                if phase < 264
                                else (
                                    ((phase[:32] & 0) | 1231752495)
                                    if phase < 265
                                    else (
                                        ((phase[:32] & 0) | 1214909486)
                                        if phase < 266
                                        else ((phase[:32] & 0) | 1265176365)
                                    )
                                )
                            )
                            if phase < 267
                            else (
                                (
                                    ((phase[:32] & 0) | 1248333356)
                                    if phase < 268
                                    else ((phase[:32] & 0) | 1299122475)
                                )
                                if phase < 269
                                else (
                                    ((phase[:32] & 0) | 1282279466)
                                    if phase < 270
                                    else (
                                        ((phase[:32] & 0) | 1332546345)
                                        if phase < 271
                                        else ((phase[:32] & 0) | 1315703336)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 272
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 1097008423)
                                    if phase < 273
                                    else ((phase[:32] & 0) | 1080165414)
                                )
                                if phase < 274
                                else (
                                    ((phase[:32] & 0) | 1130432293)
                                    if phase < 275
                                    else (
                                        ((phase[:32] & 0) | 1113589284)
                                        if phase < 276
                                        else ((phase[:32] & 0) | 1164378403)
                                    )
                                )
                            )
                            if phase < 277
                            else (
                                (
                                    ((phase[:32] & 0) | 1147535394)
                                    if phase < 278
                                    else ((phase[:32] & 0) | 1197802273)
                                )
                                if phase < 279
                                else (
                                    ((phase[:32] & 0) | 1180959264)
                                    if phase < 280
                                    else (
                                        ((phase[:32] & 0) | 1501240639)
                                        if phase < 281
                                        else ((phase[:32] & 0) | 1484397630)
                                    )
                                )
                            )
                        )
                        if phase < 282
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 1534664509)
                                    if phase < 283
                                    else ((phase[:32] & 0) | 1517821500)
                                )
                                if phase < 284
                                else (
                                    ((phase[:32] & 0) | 1568610619)
                                    if phase < 285
                                    else (
                                        ((phase[:32] & 0) | 1551767610)
                                        if phase < 286
                                        else ((phase[:32] & 0) | 1602034489)
                                    )
                                )
                            )
                            if phase < 287
                            else (
                                (
                                    ((phase[:32] & 0) | 1585191480)
                                    if phase < 288
                                    else (
                                        ((phase[:32] & 0) | 1366496567)
                                        if phase < 289
                                        else ((phase[:32] & 0) | 1349653558)
                                    )
                                )
                                if phase < 290
                                else (
                                    ((phase[:32] & 0) | 1399920437)
                                    if phase < 291
                                    else (
                                        ((phase[:32] & 0) | 1383077428)
                                        if phase < 292
                                        else ((phase[:32] & 0) | 1433866547)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 293
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 1417023538)
                                    if phase < 294
                                    else ((phase[:32] & 0) | 1467290417)
                                )
                                if phase < 295
                                else (
                                    ((phase[:32] & 0) | 1450447408)
                                    if phase < 296
                                    else (
                                        ((phase[:32] & 0) | 1766534415)
                                        if phase < 297
                                        else ((phase[:32] & 0) | 1749691406)
                                    )
                                )
                            )
                            if phase < 298
                            else (
                                (
                                    ((phase[:32] & 0) | 1799958285)
                                    if phase < 299
                                    else ((phase[:32] & 0) | 1783115276)
                                )
                                if phase < 300
                                else (
                                    ((phase[:32] & 0) | 1833904395)
                                    if phase < 301
                                    else (
                                        ((phase[:32] & 0) | 1817061386)
                                        if phase < 302
                                        else ((phase[:32] & 0) | 1867328265)
                                    )
                                )
                            )
                        )
                        if phase < 303
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 1850485256)
                                    if phase < 304
                                    else ((phase[:32] & 0) | 1631790343)
                                )
                                if phase < 305
                                else (
                                    ((phase[:32] & 0) | 1614947334)
                                    if phase < 306
                                    else (
                                        ((phase[:32] & 0) | 1665214213)
                                        if phase < 307
                                        else ((phase[:32] & 0) | 1648371204)
                                    )
                                )
                            )
                            if phase < 308
                            else (
                                (
                                    ((phase[:32] & 0) | 1699160323)
                                    if phase < 309
                                    else ((phase[:32] & 0) | 1682317314)
                                )
                                if phase < 310
                                else (
                                    ((phase[:32] & 0) | 1732584193)
                                    if phase < 311
                                    else (
                                        ((phase[:32] & 0) | 1715741184)
                                        if phase < 312
                                        else ((phase[:32] & 0) | 2036022559)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 313
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 2019179550)
                                    if phase < 314
                                    else ((phase[:32] & 0) | 2069446429)
                                )
                                if phase < 315
                                else (
                                    ((phase[:32] & 0) | 2052603420)
                                    if phase < 316
                                    else (
                                        ((phase[:32] & 0) | 2103392539)
                                        if phase < 317
                                        else ((phase[:32] & 0) | 2086549530)
                                    )
                                )
                            )
                            if phase < 318
                            else (
                                (
                                    ((phase[:32] & 0) | 2136816409)
                                    if phase < 319
                                    else ((phase[:32] & 0) | 2119973400)
                                )
                                if phase < 320
                                else (
                                    ((phase[:32] & 0) | 1901278487)
                                    if phase < 321
                                    else (
                                        ((phase[:32] & 0) | 1884435478)
                                        if phase < 322
                                        else ((phase[:32] & 0) | 1934702357)
                                    )
                                )
                            )
                        )
                        if phase < 323
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 1917859348)
                                    if phase < 324
                                    else ((phase[:32] & 0) | 1968648467)
                                )
                                if phase < 325
                                else (
                                    ((phase[:32] & 0) | 1951805458)
                                    if phase < 326
                                    else (
                                        ((phase[:32] & 0) | 2002072337)
                                        if phase < 327
                                        else ((phase[:32] & 0) | 1985229328)
                                    )
                                )
                            )
                            if phase < 328
                            else (
                                (
                                    ((phase[:32] & 0) | 0)
                                    if phase < 336
                                    else (
                                        ((phase[:32] & 0) | 3657145791)
                                        if phase < 337
                                        else ((phase[:32] & 0) | 3640302782)
                                    )
                                )
                                if phase < 338
                                else (
                                    ((phase[:32] & 0) | 3690569661)
                                    if phase < 339
                                    else (
                                        ((phase[:32] & 0) | 3673726652)
                                        if phase < 340
                                        else ((phase[:32] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    right_take = (
        (
            (((phase[:1] & 0) | 0) if phase < 6 else ((phase[:1] & 0) | 1))
            if phase < 20
            else (
                ((phase[:1] & 0) | 0)
                if phase < 29
                else (((phase[:1] & 0) | 1) if phase < 56 else ((phase[:1] & 0) | 0))
            )
        )
        if phase < 60
        else (
            (((phase[:1] & 0) | 1) if phase < 63 else ((phase[:1] & 0) | 0))
            if phase < 64
            else (
                ((phase[:1] & 0) | 1)
                if phase < 336
                else (((phase[:1] & 0) | 0) if phase < 340 else ((phase[:1] & 0) | 1))
            )
        )
    )
    left_data = LeftToken(value=left_value)
    right_data = RightToken(value=right_value)
    dut = BarrierPipeline(
        left_valid, left_data, left_take, right_valid, right_data, right_take
    )
    expected_left_ready = (
        (
            ((phase[:1] & 0) | 1)
            if phase < 3
            else (((phase[:1] & 0) | 0) if phase < 5 else ((phase[:1] & 0) | 1))
        )
        if phase < 25
        else (
            (((phase[:1] & 0) | 0) if phase < 29 else ((phase[:1] & 0) | 1))
            if phase < 56
            else (((phase[:1] & 0) | 0) if phase < 63 else ((phase[:1] & 0) | 1))
        )
    )
    expected_left_valid = (
        (
            (((phase[:1] & 0) | 0) if phase < 6 else ((phase[:1] & 0) | 1))
            if phase < 16
            else (((phase[:1] & 0) | 0) if phase < 23 else ((phase[:1] & 0) | 1))
        )
        if phase < 28
        else (
            (((phase[:1] & 0) | 0) if phase < 30 else ((phase[:1] & 0) | 1))
            if phase < 332
            else (
                ((phase[:1] & 0) | 0)
                if phase < 338
                else (((phase[:1] & 0) | 1) if phase < 345 else ((phase[:1] & 0) | 0))
            )
        )
    )
    expected_left_data_value = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 6
                                    else ((phase[:16] & 0) | 42330)
                                )
                                if phase < 7
                                else (
                                    ((phase[:16] & 0) | 42075)
                                    if phase < 8
                                    else ((phase[:16] & 0) | 41055)
                                )
                            )
                            if phase < 9
                            else (
                                (
                                    ((phase[:16] & 0) | 41820)
                                    if phase < 10
                                    else ((phase[:16] & 0) | 41565)
                                )
                                if phase < 11
                                else (
                                    ((phase[:16] & 0) | 44370)
                                    if phase < 12
                                    else (
                                        ((phase[:16] & 0) | 44115)
                                        if phase < 13
                                        else ((phase[:16] & 0) | 44880)
                                    )
                                )
                            )
                        )
                        if phase < 14
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 44625)
                                    if phase < 15
                                    else ((phase[:16] & 0) | 43350)
                                )
                                if phase < 16
                                else (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 23
                                    else (
                                        ((phase[:16] & 0) | 45390)
                                        if phase < 27
                                        else ((phase[:16] & 0) | 45135)
                                    )
                                )
                            )
                            if phase < 28
                            else (
                                (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 30
                                    else ((phase[:16] & 0) | 45900)
                                )
                                if phase < 31
                                else (
                                    ((phase[:16] & 0) | 45645)
                                    if phase < 32
                                    else (
                                        ((phase[:16] & 0) | 34170)
                                        if phase < 33
                                        else ((phase[:16] & 0) | 33915)
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
                                    ((phase[:16] & 0) | 34680)
                                    if phase < 35
                                    else ((phase[:16] & 0) | 34425)
                                )
                                if phase < 36
                                else (
                                    ((phase[:16] & 0) | 33150)
                                    if phase < 37
                                    else (
                                        ((phase[:16] & 0) | 32895)
                                        if phase < 38
                                        else ((phase[:16] & 0) | 33660)
                                    )
                                )
                            )
                            if phase < 39
                            else (
                                (
                                    ((phase[:16] & 0) | 33405)
                                    if phase < 40
                                    else ((phase[:16] & 0) | 36210)
                                )
                                if phase < 41
                                else (
                                    ((phase[:16] & 0) | 35955)
                                    if phase < 42
                                    else (
                                        ((phase[:16] & 0) | 36720)
                                        if phase < 43
                                        else ((phase[:16] & 0) | 36465)
                                    )
                                )
                            )
                        )
                        if phase < 44
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 35190)
                                    if phase < 45
                                    else ((phase[:16] & 0) | 34935)
                                )
                                if phase < 46
                                else (
                                    ((phase[:16] & 0) | 35700)
                                    if phase < 47
                                    else (
                                        ((phase[:16] & 0) | 35445)
                                        if phase < 48
                                        else ((phase[:16] & 0) | 38250)
                                    )
                                )
                            )
                            if phase < 49
                            else (
                                (
                                    ((phase[:16] & 0) | 37995)
                                    if phase < 50
                                    else ((phase[:16] & 0) | 38760)
                                )
                                if phase < 51
                                else (
                                    ((phase[:16] & 0) | 38505)
                                    if phase < 52
                                    else (
                                        ((phase[:16] & 0) | 37230)
                                        if phase < 53
                                        else ((phase[:16] & 0) | 36975)
                                    )
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
                                (
                                    ((phase[:16] & 0) | 37740)
                                    if phase < 56
                                    else ((phase[:16] & 0) | 37485)
                                )
                                if phase < 64
                                else (
                                    ((phase[:16] & 0) | 40290)
                                    if phase < 65
                                    else ((phase[:16] & 0) | 40800)
                                )
                            )
                            if phase < 66
                            else (
                                (
                                    ((phase[:16] & 0) | 0)
                                    if phase < 67
                                    else ((phase[:16] & 0) | 57630)
                                )
                                if phase < 68
                                else (
                                    ((phase[:16] & 0) | 57375)
                                    if phase < 69
                                    else (
                                        ((phase[:16] & 0) | 58140)
                                        if phase < 70
                                        else ((phase[:16] & 0) | 57885)
                                    )
                                )
                            )
                        )
                        if phase < 71
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 60690)
                                    if phase < 72
                                    else ((phase[:16] & 0) | 60435)
                                )
                                if phase < 73
                                else (
                                    ((phase[:16] & 0) | 61200)
                                    if phase < 74
                                    else (
                                        ((phase[:16] & 0) | 60945)
                                        if phase < 75
                                        else ((phase[:16] & 0) | 59670)
                                    )
                                )
                            )
                            if phase < 76
                            else (
                                (
                                    ((phase[:16] & 0) | 42330)
                                    if phase < 77
                                    else ((phase[:16] & 0) | 42075)
                                )
                                if phase < 78
                                else (
                                    ((phase[:16] & 0) | 42840)
                                    if phase < 79
                                    else (
                                        ((phase[:16] & 0) | 42585)
                                        if phase < 80
                                        else ((phase[:16] & 0) | 41310)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 81
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 41055)
                                    if phase < 82
                                    else ((phase[:16] & 0) | 41820)
                                )
                                if phase < 83
                                else (
                                    ((phase[:16] & 0) | 41565)
                                    if phase < 84
                                    else (
                                        ((phase[:16] & 0) | 44370)
                                        if phase < 85
                                        else ((phase[:16] & 0) | 44115)
                                    )
                                )
                            )
                            if phase < 86
                            else (
                                (
                                    ((phase[:16] & 0) | 44880)
                                    if phase < 87
                                    else ((phase[:16] & 0) | 44625)
                                )
                                if phase < 88
                                else (
                                    ((phase[:16] & 0) | 43350)
                                    if phase < 89
                                    else (
                                        ((phase[:16] & 0) | 43095)
                                        if phase < 90
                                        else ((phase[:16] & 0) | 43860)
                                    )
                                )
                            )
                        )
                        if phase < 91
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 43605)
                                    if phase < 92
                                    else ((phase[:16] & 0) | 46410)
                                )
                                if phase < 93
                                else (
                                    ((phase[:16] & 0) | 46155)
                                    if phase < 94
                                    else (
                                        ((phase[:16] & 0) | 46920)
                                        if phase < 95
                                        else ((phase[:16] & 0) | 46665)
                                    )
                                )
                            )
                            if phase < 96
                            else (
                                (
                                    ((phase[:16] & 0) | 45390)
                                    if phase < 97
                                    else ((phase[:16] & 0) | 45135)
                                )
                                if phase < 98
                                else (
                                    ((phase[:16] & 0) | 45900)
                                    if phase < 99
                                    else (
                                        ((phase[:16] & 0) | 45645)
                                        if phase < 100
                                        else ((phase[:16] & 0) | 48450)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 101
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 48195)
                                    if phase < 102
                                    else ((phase[:16] & 0) | 48960)
                                )
                                if phase < 103
                                else (
                                    ((phase[:16] & 0) | 48705)
                                    if phase < 104
                                    else ((phase[:16] & 0) | 47430)
                                )
                            )
                            if phase < 105
                            else (
                                (
                                    ((phase[:16] & 0) | 47175)
                                    if phase < 106
                                    else ((phase[:16] & 0) | 47940)
                                )
                                if phase < 107
                                else (
                                    ((phase[:16] & 0) | 47685)
                                    if phase < 108
                                    else (
                                        ((phase[:16] & 0) | 34170)
                                        if phase < 109
                                        else ((phase[:16] & 0) | 33915)
                                    )
                                )
                            )
                        )
                        if phase < 110
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 34680)
                                    if phase < 111
                                    else ((phase[:16] & 0) | 34425)
                                )
                                if phase < 112
                                else (
                                    ((phase[:16] & 0) | 33150)
                                    if phase < 113
                                    else (
                                        ((phase[:16] & 0) | 32895)
                                        if phase < 114
                                        else ((phase[:16] & 0) | 33660)
                                    )
                                )
                            )
                            if phase < 115
                            else (
                                (
                                    ((phase[:16] & 0) | 33405)
                                    if phase < 116
                                    else ((phase[:16] & 0) | 36210)
                                )
                                if phase < 117
                                else (
                                    ((phase[:16] & 0) | 35955)
                                    if phase < 118
                                    else (
                                        ((phase[:16] & 0) | 36720)
                                        if phase < 119
                                        else ((phase[:16] & 0) | 36465)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 120
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 35190)
                                    if phase < 121
                                    else ((phase[:16] & 0) | 34935)
                                )
                                if phase < 122
                                else (
                                    ((phase[:16] & 0) | 35700)
                                    if phase < 123
                                    else (
                                        ((phase[:16] & 0) | 35445)
                                        if phase < 124
                                        else ((phase[:16] & 0) | 38250)
                                    )
                                )
                            )
                            if phase < 125
                            else (
                                (
                                    ((phase[:16] & 0) | 37995)
                                    if phase < 126
                                    else ((phase[:16] & 0) | 38760)
                                )
                                if phase < 127
                                else (
                                    ((phase[:16] & 0) | 38505)
                                    if phase < 128
                                    else (
                                        ((phase[:16] & 0) | 37230)
                                        if phase < 129
                                        else ((phase[:16] & 0) | 36975)
                                    )
                                )
                            )
                        )
                        if phase < 130
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 37740)
                                    if phase < 131
                                    else ((phase[:16] & 0) | 37485)
                                )
                                if phase < 132
                                else (
                                    ((phase[:16] & 0) | 40290)
                                    if phase < 133
                                    else (
                                        ((phase[:16] & 0) | 40035)
                                        if phase < 134
                                        else ((phase[:16] & 0) | 40800)
                                    )
                                )
                            )
                            if phase < 135
                            else (
                                (
                                    ((phase[:16] & 0) | 40545)
                                    if phase < 136
                                    else ((phase[:16] & 0) | 39270)
                                )
                                if phase < 137
                                else (
                                    ((phase[:16] & 0) | 39015)
                                    if phase < 138
                                    else (
                                        ((phase[:16] & 0) | 39780)
                                        if phase < 139
                                        else ((phase[:16] & 0) | 39525)
                                    )
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
                                (
                                    ((phase[:16] & 0) | 58650)
                                    if phase < 141
                                    else ((phase[:16] & 0) | 58395)
                                )
                                if phase < 142
                                else (
                                    ((phase[:16] & 0) | 59160)
                                    if phase < 143
                                    else (
                                        ((phase[:16] & 0) | 58905)
                                        if phase < 144
                                        else ((phase[:16] & 0) | 57630)
                                    )
                                )
                            )
                            if phase < 145
                            else (
                                (
                                    ((phase[:16] & 0) | 57375)
                                    if phase < 146
                                    else ((phase[:16] & 0) | 58140)
                                )
                                if phase < 147
                                else (
                                    ((phase[:16] & 0) | 57885)
                                    if phase < 148
                                    else (
                                        ((phase[:16] & 0) | 60690)
                                        if phase < 149
                                        else ((phase[:16] & 0) | 60435)
                                    )
                                )
                            )
                        )
                        if phase < 150
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 61200)
                                    if phase < 151
                                    else ((phase[:16] & 0) | 60945)
                                )
                                if phase < 152
                                else (
                                    ((phase[:16] & 0) | 59670)
                                    if phase < 153
                                    else (
                                        ((phase[:16] & 0) | 59415)
                                        if phase < 154
                                        else ((phase[:16] & 0) | 60180)
                                    )
                                )
                            )
                            if phase < 155
                            else (
                                (
                                    ((phase[:16] & 0) | 59925)
                                    if phase < 156
                                    else ((phase[:16] & 0) | 62730)
                                )
                                if phase < 157
                                else (
                                    ((phase[:16] & 0) | 62475)
                                    if phase < 158
                                    else (
                                        ((phase[:16] & 0) | 63240)
                                        if phase < 159
                                        else ((phase[:16] & 0) | 62985)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 160
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 61710)
                                    if phase < 161
                                    else ((phase[:16] & 0) | 61455)
                                )
                                if phase < 162
                                else (
                                    ((phase[:16] & 0) | 62220)
                                    if phase < 163
                                    else (
                                        ((phase[:16] & 0) | 61965)
                                        if phase < 164
                                        else ((phase[:16] & 0) | 64770)
                                    )
                                )
                            )
                            if phase < 165
                            else (
                                (
                                    ((phase[:16] & 0) | 64515)
                                    if phase < 166
                                    else ((phase[:16] & 0) | 65280)
                                )
                                if phase < 167
                                else (
                                    ((phase[:16] & 0) | 65025)
                                    if phase < 168
                                    else (
                                        ((phase[:16] & 0) | 63750)
                                        if phase < 169
                                        else ((phase[:16] & 0) | 63495)
                                    )
                                )
                            )
                        )
                        if phase < 170
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 64260)
                                    if phase < 171
                                    else ((phase[:16] & 0) | 64005)
                                )
                                if phase < 172
                                else (
                                    ((phase[:16] & 0) | 50490)
                                    if phase < 173
                                    else (
                                        ((phase[:16] & 0) | 50235)
                                        if phase < 174
                                        else ((phase[:16] & 0) | 51000)
                                    )
                                )
                            )
                            if phase < 175
                            else (
                                (
                                    ((phase[:16] & 0) | 50745)
                                    if phase < 176
                                    else ((phase[:16] & 0) | 49470)
                                )
                                if phase < 177
                                else (
                                    ((phase[:16] & 0) | 49215)
                                    if phase < 178
                                    else (
                                        ((phase[:16] & 0) | 49980)
                                        if phase < 179
                                        else ((phase[:16] & 0) | 49725)
                                    )
                                )
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
                            (
                                (
                                    ((phase[:16] & 0) | 52530)
                                    if phase < 181
                                    else ((phase[:16] & 0) | 52275)
                                )
                                if phase < 182
                                else (
                                    ((phase[:16] & 0) | 53040)
                                    if phase < 183
                                    else ((phase[:16] & 0) | 52785)
                                )
                            )
                            if phase < 184
                            else (
                                (
                                    ((phase[:16] & 0) | 51510)
                                    if phase < 185
                                    else ((phase[:16] & 0) | 51255)
                                )
                                if phase < 186
                                else (
                                    ((phase[:16] & 0) | 52020)
                                    if phase < 187
                                    else (
                                        ((phase[:16] & 0) | 51765)
                                        if phase < 188
                                        else ((phase[:16] & 0) | 54570)
                                    )
                                )
                            )
                        )
                        if phase < 189
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 54315)
                                    if phase < 190
                                    else ((phase[:16] & 0) | 55080)
                                )
                                if phase < 191
                                else (
                                    ((phase[:16] & 0) | 54825)
                                    if phase < 192
                                    else (
                                        ((phase[:16] & 0) | 53550)
                                        if phase < 193
                                        else ((phase[:16] & 0) | 53295)
                                    )
                                )
                            )
                            if phase < 194
                            else (
                                (
                                    ((phase[:16] & 0) | 54060)
                                    if phase < 195
                                    else ((phase[:16] & 0) | 53805)
                                )
                                if phase < 196
                                else (
                                    ((phase[:16] & 0) | 56610)
                                    if phase < 197
                                    else (
                                        ((phase[:16] & 0) | 56355)
                                        if phase < 198
                                        else ((phase[:16] & 0) | 57120)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 199
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 56865)
                                    if phase < 200
                                    else ((phase[:16] & 0) | 55590)
                                )
                                if phase < 201
                                else (
                                    ((phase[:16] & 0) | 55335)
                                    if phase < 202
                                    else (
                                        ((phase[:16] & 0) | 56100)
                                        if phase < 203
                                        else ((phase[:16] & 0) | 55845)
                                    )
                                )
                            )
                            if phase < 204
                            else (
                                (
                                    ((phase[:16] & 0) | 9690)
                                    if phase < 205
                                    else ((phase[:16] & 0) | 9435)
                                )
                                if phase < 206
                                else (
                                    ((phase[:16] & 0) | 10200)
                                    if phase < 207
                                    else (
                                        ((phase[:16] & 0) | 9945)
                                        if phase < 208
                                        else ((phase[:16] & 0) | 8670)
                                    )
                                )
                            )
                        )
                        if phase < 209
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 8415)
                                    if phase < 210
                                    else ((phase[:16] & 0) | 9180)
                                )
                                if phase < 211
                                else (
                                    ((phase[:16] & 0) | 8925)
                                    if phase < 212
                                    else (
                                        ((phase[:16] & 0) | 11730)
                                        if phase < 213
                                        else ((phase[:16] & 0) | 11475)
                                    )
                                )
                            )
                            if phase < 214
                            else (
                                (
                                    ((phase[:16] & 0) | 12240)
                                    if phase < 215
                                    else ((phase[:16] & 0) | 11985)
                                )
                                if phase < 216
                                else (
                                    ((phase[:16] & 0) | 10710)
                                    if phase < 217
                                    else (
                                        ((phase[:16] & 0) | 10455)
                                        if phase < 218
                                        else ((phase[:16] & 0) | 11220)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 219
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 10965)
                                    if phase < 220
                                    else ((phase[:16] & 0) | 13770)
                                )
                                if phase < 221
                                else (
                                    ((phase[:16] & 0) | 13515)
                                    if phase < 222
                                    else (
                                        ((phase[:16] & 0) | 14280)
                                        if phase < 223
                                        else ((phase[:16] & 0) | 14025)
                                    )
                                )
                            )
                            if phase < 224
                            else (
                                (
                                    ((phase[:16] & 0) | 12750)
                                    if phase < 225
                                    else ((phase[:16] & 0) | 12495)
                                )
                                if phase < 226
                                else (
                                    ((phase[:16] & 0) | 13260)
                                    if phase < 227
                                    else (
                                        ((phase[:16] & 0) | 13005)
                                        if phase < 228
                                        else ((phase[:16] & 0) | 15810)
                                    )
                                )
                            )
                        )
                        if phase < 229
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 15555)
                                    if phase < 230
                                    else ((phase[:16] & 0) | 16320)
                                )
                                if phase < 231
                                else (
                                    ((phase[:16] & 0) | 16065)
                                    if phase < 232
                                    else (
                                        ((phase[:16] & 0) | 14790)
                                        if phase < 233
                                        else ((phase[:16] & 0) | 14535)
                                    )
                                )
                            )
                            if phase < 234
                            else (
                                (
                                    ((phase[:16] & 0) | 15300)
                                    if phase < 235
                                    else ((phase[:16] & 0) | 15045)
                                )
                                if phase < 236
                                else (
                                    ((phase[:16] & 0) | 1530)
                                    if phase < 237
                                    else (
                                        ((phase[:16] & 0) | 1275)
                                        if phase < 238
                                        else ((phase[:16] & 0) | 2040)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 239
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 1785)
                                    if phase < 240
                                    else ((phase[:16] & 0) | 510)
                                )
                                if phase < 241
                                else (
                                    ((phase[:16] & 0) | 255)
                                    if phase < 242
                                    else (
                                        ((phase[:16] & 0) | 1020)
                                        if phase < 243
                                        else ((phase[:16] & 0) | 765)
                                    )
                                )
                            )
                            if phase < 244
                            else (
                                (
                                    ((phase[:16] & 0) | 3570)
                                    if phase < 245
                                    else ((phase[:16] & 0) | 3315)
                                )
                                if phase < 246
                                else (
                                    ((phase[:16] & 0) | 4080)
                                    if phase < 247
                                    else (
                                        ((phase[:16] & 0) | 3825)
                                        if phase < 248
                                        else ((phase[:16] & 0) | 2550)
                                    )
                                )
                            )
                        )
                        if phase < 249
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 2295)
                                    if phase < 250
                                    else ((phase[:16] & 0) | 3060)
                                )
                                if phase < 251
                                else (
                                    ((phase[:16] & 0) | 2805)
                                    if phase < 252
                                    else (
                                        ((phase[:16] & 0) | 5610)
                                        if phase < 253
                                        else ((phase[:16] & 0) | 5355)
                                    )
                                )
                            )
                            if phase < 254
                            else (
                                (
                                    ((phase[:16] & 0) | 6120)
                                    if phase < 255
                                    else ((phase[:16] & 0) | 5865)
                                )
                                if phase < 256
                                else (
                                    ((phase[:16] & 0) | 4590)
                                    if phase < 257
                                    else (
                                        ((phase[:16] & 0) | 4335)
                                        if phase < 258
                                        else ((phase[:16] & 0) | 5100)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 259
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 4845)
                                    if phase < 260
                                    else ((phase[:16] & 0) | 7650)
                                )
                                if phase < 261
                                else (
                                    ((phase[:16] & 0) | 7395)
                                    if phase < 262
                                    else ((phase[:16] & 0) | 8160)
                                )
                            )
                            if phase < 263
                            else (
                                (
                                    ((phase[:16] & 0) | 7905)
                                    if phase < 264
                                    else ((phase[:16] & 0) | 6630)
                                )
                                if phase < 265
                                else (
                                    ((phase[:16] & 0) | 6375)
                                    if phase < 266
                                    else (
                                        ((phase[:16] & 0) | 7140)
                                        if phase < 267
                                        else ((phase[:16] & 0) | 6885)
                                    )
                                )
                            )
                        )
                        if phase < 268
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 26010)
                                    if phase < 269
                                    else ((phase[:16] & 0) | 25755)
                                )
                                if phase < 270
                                else (
                                    ((phase[:16] & 0) | 26520)
                                    if phase < 271
                                    else (
                                        ((phase[:16] & 0) | 26265)
                                        if phase < 272
                                        else ((phase[:16] & 0) | 24990)
                                    )
                                )
                            )
                            if phase < 273
                            else (
                                (
                                    ((phase[:16] & 0) | 24735)
                                    if phase < 274
                                    else ((phase[:16] & 0) | 25500)
                                )
                                if phase < 275
                                else (
                                    ((phase[:16] & 0) | 25245)
                                    if phase < 276
                                    else (
                                        ((phase[:16] & 0) | 28050)
                                        if phase < 277
                                        else ((phase[:16] & 0) | 27795)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 278
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 28560)
                                    if phase < 279
                                    else ((phase[:16] & 0) | 28305)
                                )
                                if phase < 280
                                else (
                                    ((phase[:16] & 0) | 27030)
                                    if phase < 281
                                    else (
                                        ((phase[:16] & 0) | 26775)
                                        if phase < 282
                                        else ((phase[:16] & 0) | 27540)
                                    )
                                )
                            )
                            if phase < 283
                            else (
                                (
                                    ((phase[:16] & 0) | 27285)
                                    if phase < 284
                                    else ((phase[:16] & 0) | 30090)
                                )
                                if phase < 285
                                else (
                                    ((phase[:16] & 0) | 29835)
                                    if phase < 286
                                    else (
                                        ((phase[:16] & 0) | 30600)
                                        if phase < 287
                                        else ((phase[:16] & 0) | 30345)
                                    )
                                )
                            )
                        )
                        if phase < 288
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 29070)
                                    if phase < 289
                                    else ((phase[:16] & 0) | 28815)
                                )
                                if phase < 290
                                else (
                                    ((phase[:16] & 0) | 29580)
                                    if phase < 291
                                    else (
                                        ((phase[:16] & 0) | 29325)
                                        if phase < 292
                                        else ((phase[:16] & 0) | 32130)
                                    )
                                )
                            )
                            if phase < 293
                            else (
                                (
                                    ((phase[:16] & 0) | 31875)
                                    if phase < 294
                                    else ((phase[:16] & 0) | 32640)
                                )
                                if phase < 295
                                else (
                                    ((phase[:16] & 0) | 32385)
                                    if phase < 296
                                    else (
                                        ((phase[:16] & 0) | 31110)
                                        if phase < 297
                                        else ((phase[:16] & 0) | 30855)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 298
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 31620)
                                    if phase < 299
                                    else ((phase[:16] & 0) | 31365)
                                )
                                if phase < 300
                                else (
                                    ((phase[:16] & 0) | 17850)
                                    if phase < 301
                                    else (
                                        ((phase[:16] & 0) | 17595)
                                        if phase < 302
                                        else ((phase[:16] & 0) | 18360)
                                    )
                                )
                            )
                            if phase < 303
                            else (
                                (
                                    ((phase[:16] & 0) | 18105)
                                    if phase < 304
                                    else ((phase[:16] & 0) | 16830)
                                )
                                if phase < 305
                                else (
                                    ((phase[:16] & 0) | 16575)
                                    if phase < 306
                                    else (
                                        ((phase[:16] & 0) | 17340)
                                        if phase < 307
                                        else ((phase[:16] & 0) | 17085)
                                    )
                                )
                            )
                        )
                        if phase < 308
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 19890)
                                    if phase < 309
                                    else ((phase[:16] & 0) | 19635)
                                )
                                if phase < 310
                                else (
                                    ((phase[:16] & 0) | 20400)
                                    if phase < 311
                                    else (
                                        ((phase[:16] & 0) | 20145)
                                        if phase < 312
                                        else ((phase[:16] & 0) | 18870)
                                    )
                                )
                            )
                            if phase < 313
                            else (
                                (
                                    ((phase[:16] & 0) | 18615)
                                    if phase < 314
                                    else ((phase[:16] & 0) | 19380)
                                )
                                if phase < 315
                                else (
                                    ((phase[:16] & 0) | 19125)
                                    if phase < 316
                                    else (
                                        ((phase[:16] & 0) | 21930)
                                        if phase < 317
                                        else ((phase[:16] & 0) | 21675)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 318
                    else (
                        (
                            (
                                (
                                    ((phase[:16] & 0) | 22440)
                                    if phase < 319
                                    else ((phase[:16] & 0) | 22185)
                                )
                                if phase < 320
                                else (
                                    ((phase[:16] & 0) | 20910)
                                    if phase < 321
                                    else (
                                        ((phase[:16] & 0) | 20655)
                                        if phase < 322
                                        else ((phase[:16] & 0) | 21420)
                                    )
                                )
                            )
                            if phase < 323
                            else (
                                (
                                    ((phase[:16] & 0) | 21165)
                                    if phase < 324
                                    else ((phase[:16] & 0) | 23970)
                                )
                                if phase < 325
                                else (
                                    ((phase[:16] & 0) | 23715)
                                    if phase < 326
                                    else (
                                        ((phase[:16] & 0) | 24480)
                                        if phase < 327
                                        else ((phase[:16] & 0) | 24225)
                                    )
                                )
                            )
                        )
                        if phase < 328
                        else (
                            (
                                (
                                    ((phase[:16] & 0) | 22950)
                                    if phase < 329
                                    else ((phase[:16] & 0) | 22695)
                                )
                                if phase < 330
                                else (
                                    ((phase[:16] & 0) | 23460)
                                    if phase < 331
                                    else (
                                        ((phase[:16] & 0) | 23205)
                                        if phase < 332
                                        else ((phase[:16] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 338
                            else (
                                (
                                    ((phase[:16] & 0) | 62730)
                                    if phase < 341
                                    else ((phase[:16] & 0) | 62475)
                                )
                                if phase < 342
                                else (
                                    ((phase[:16] & 0) | 63240)
                                    if phase < 343
                                    else (
                                        ((phase[:16] & 0) | 62985)
                                        if phase < 344
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
    expected_right_ready = (
        (((phase[:1] & 0) | 1) if phase < 25 else ((phase[:1] & 0) | 0))
        if phase < 29
        else (
            ((phase[:1] & 0) | 1)
            if phase < 56
            else (((phase[:1] & 0) | 0) if phase < 63 else ((phase[:1] & 0) | 1))
        )
    )
    expected_right_valid = (
        (
            (((phase[:1] & 0) | 0) if phase < 6 else ((phase[:1] & 0) | 1))
            if phase < 16
            else (((phase[:1] & 0) | 0) if phase < 23 else ((phase[:1] & 0) | 1))
        )
        if phase < 62
        else (
            (((phase[:1] & 0) | 0) if phase < 64 else ((phase[:1] & 0) | 1))
            if phase < 331
            else (
                ((phase[:1] & 0) | 0)
                if phase < 338
                else (((phase[:1] & 0) | 1) if phase < 345 else ((phase[:1] & 0) | 0))
            )
        )
    )
    expected_right_data_value = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 0)
                                    if phase < 6
                                    else ((phase[:32] & 0) | 2309737967)
                                )
                                if phase < 7
                                else (
                                    ((phase[:32] & 0) | 2292894958)
                                    if phase < 8
                                    else ((phase[:32] & 0) | 2360264938)
                                )
                            )
                            if phase < 9
                            else (
                                (
                                    ((phase[:32] & 0) | 2410531817)
                                    if phase < 10
                                    else ((phase[:32] & 0) | 2393688808)
                                )
                                if phase < 11
                                else (
                                    ((phase[:32] & 0) | 2174993895)
                                    if phase < 12
                                    else (
                                        ((phase[:32] & 0) | 2158150886)
                                        if phase < 13
                                        else ((phase[:32] & 0) | 2208417765)
                                    )
                                )
                            )
                        )
                        if phase < 14
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 2191574756)
                                    if phase < 15
                                    else ((phase[:32] & 0) | 2242363875)
                                )
                                if phase < 16
                                else (
                                    ((phase[:32] & 0) | 0)
                                    if phase < 23
                                    else (
                                        ((phase[:32] & 0) | 2646596091)
                                        if phase < 30
                                        else ((phase[:32] & 0) | 2629753082)
                                    )
                                )
                            )
                            if phase < 31
                            else (
                                (
                                    ((phase[:32] & 0) | 2680019961)
                                    if phase < 32
                                    else ((phase[:32] & 0) | 2663176952)
                                )
                                if phase < 33
                                else (
                                    ((phase[:32] & 0) | 2844519887)
                                    if phase < 34
                                    else (
                                        ((phase[:32] & 0) | 2827676878)
                                        if phase < 35
                                        else ((phase[:32] & 0) | 2877943757)
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
                                    ((phase[:32] & 0) | 2861100748)
                                    if phase < 37
                                    else ((phase[:32] & 0) | 2911889867)
                                )
                                if phase < 38
                                else (
                                    ((phase[:32] & 0) | 2895046858)
                                    if phase < 39
                                    else (
                                        ((phase[:32] & 0) | 2945313737)
                                        if phase < 40
                                        else ((phase[:32] & 0) | 2928470728)
                                    )
                                )
                            )
                            if phase < 41
                            else (
                                (
                                    ((phase[:32] & 0) | 2709775815)
                                    if phase < 42
                                    else ((phase[:32] & 0) | 2692932806)
                                )
                                if phase < 43
                                else (
                                    ((phase[:32] & 0) | 2743199685)
                                    if phase < 44
                                    else (
                                        ((phase[:32] & 0) | 2726356676)
                                        if phase < 45
                                        else ((phase[:32] & 0) | 2777145795)
                                    )
                                )
                            )
                        )
                        if phase < 46
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 2760302786)
                                    if phase < 47
                                    else ((phase[:32] & 0) | 2810569665)
                                )
                                if phase < 48
                                else (
                                    ((phase[:32] & 0) | 2793726656)
                                    if phase < 49
                                    else (
                                        ((phase[:32] & 0) | 3114008031)
                                        if phase < 50
                                        else ((phase[:32] & 0) | 3097165022)
                                    )
                                )
                            )
                            if phase < 51
                            else (
                                (
                                    ((phase[:32] & 0) | 3147431901)
                                    if phase < 52
                                    else ((phase[:32] & 0) | 3130588892)
                                )
                                if phase < 53
                                else (
                                    ((phase[:32] & 0) | 3181378011)
                                    if phase < 54
                                    else (
                                        ((phase[:32] & 0) | 3164535002)
                                        if phase < 55
                                        else ((phase[:32] & 0) | 3214801881)
                                    )
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
                                (
                                    ((phase[:32] & 0) | 3197958872)
                                    if phase < 61
                                    else ((phase[:32] & 0) | 2979263959)
                                )
                                if phase < 62
                                else (
                                    ((phase[:32] & 0) | 0)
                                    if phase < 64
                                    else ((phase[:32] & 0) | 3012687829)
                                )
                            )
                            if phase < 65
                            else (
                                (
                                    ((phase[:32] & 0) | 0)
                                    if phase < 66
                                    else ((phase[:32] & 0) | 3455027627)
                                )
                                if phase < 67
                                else (
                                    ((phase[:32] & 0) | 3438184618)
                                    if phase < 68
                                    else (
                                        ((phase[:32] & 0) | 3488451497)
                                        if phase < 69
                                        else ((phase[:32] & 0) | 3471608488)
                                    )
                                )
                            )
                        )
                        if phase < 70
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 3252913575)
                                    if phase < 71
                                    else ((phase[:32] & 0) | 3236070566)
                                )
                                if phase < 72
                                else (
                                    ((phase[:32] & 0) | 3286337445)
                                    if phase < 73
                                    else (
                                        ((phase[:32] & 0) | 3269494436)
                                        if phase < 74
                                        else ((phase[:32] & 0) | 3320283555)
                                    )
                                )
                            )
                            if phase < 75
                            else (
                                (
                                    ((phase[:32] & 0) | 2309737967)
                                    if phase < 76
                                    else ((phase[:32] & 0) | 2292894958)
                                )
                                if phase < 77
                                else (
                                    ((phase[:32] & 0) | 2343161837)
                                    if phase < 78
                                    else (
                                        ((phase[:32] & 0) | 2326318828)
                                        if phase < 79
                                        else ((phase[:32] & 0) | 2377107947)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 80
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 2360264938)
                                    if phase < 81
                                    else ((phase[:32] & 0) | 2410531817)
                                )
                                if phase < 82
                                else (
                                    ((phase[:32] & 0) | 2393688808)
                                    if phase < 83
                                    else (
                                        ((phase[:32] & 0) | 2174993895)
                                        if phase < 84
                                        else ((phase[:32] & 0) | 2158150886)
                                    )
                                )
                            )
                            if phase < 85
                            else (
                                (
                                    ((phase[:32] & 0) | 2208417765)
                                    if phase < 86
                                    else ((phase[:32] & 0) | 2191574756)
                                )
                                if phase < 87
                                else (
                                    ((phase[:32] & 0) | 2242363875)
                                    if phase < 88
                                    else (
                                        ((phase[:32] & 0) | 2225520866)
                                        if phase < 89
                                        else ((phase[:32] & 0) | 2275787745)
                                    )
                                )
                            )
                        )
                        if phase < 90
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 2258944736)
                                    if phase < 91
                                    else ((phase[:32] & 0) | 2579226111)
                                )
                                if phase < 92
                                else (
                                    ((phase[:32] & 0) | 2562383102)
                                    if phase < 93
                                    else (
                                        ((phase[:32] & 0) | 2612649981)
                                        if phase < 94
                                        else ((phase[:32] & 0) | 2595806972)
                                    )
                                )
                            )
                            if phase < 95
                            else (
                                (
                                    ((phase[:32] & 0) | 2646596091)
                                    if phase < 96
                                    else ((phase[:32] & 0) | 2629753082)
                                )
                                if phase < 97
                                else (
                                    ((phase[:32] & 0) | 2680019961)
                                    if phase < 98
                                    else (
                                        ((phase[:32] & 0) | 2663176952)
                                        if phase < 99
                                        else ((phase[:32] & 0) | 2444482039)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 100
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 2427639030)
                                    if phase < 101
                                    else ((phase[:32] & 0) | 2477905909)
                                )
                                if phase < 102
                                else (
                                    ((phase[:32] & 0) | 2461062900)
                                    if phase < 103
                                    else ((phase[:32] & 0) | 2511852019)
                                )
                            )
                            if phase < 104
                            else (
                                (
                                    ((phase[:32] & 0) | 2495009010)
                                    if phase < 105
                                    else ((phase[:32] & 0) | 2545275889)
                                )
                                if phase < 106
                                else (
                                    ((phase[:32] & 0) | 2528432880)
                                    if phase < 107
                                    else (
                                        ((phase[:32] & 0) | 2844519887)
                                        if phase < 108
                                        else ((phase[:32] & 0) | 2827676878)
                                    )
                                )
                            )
                        )
                        if phase < 109
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 2877943757)
                                    if phase < 110
                                    else ((phase[:32] & 0) | 2861100748)
                                )
                                if phase < 111
                                else (
                                    ((phase[:32] & 0) | 2911889867)
                                    if phase < 112
                                    else (
                                        ((phase[:32] & 0) | 2895046858)
                                        if phase < 113
                                        else ((phase[:32] & 0) | 2945313737)
                                    )
                                )
                            )
                            if phase < 114
                            else (
                                (
                                    ((phase[:32] & 0) | 2928470728)
                                    if phase < 115
                                    else ((phase[:32] & 0) | 2709775815)
                                )
                                if phase < 116
                                else (
                                    ((phase[:32] & 0) | 2692932806)
                                    if phase < 117
                                    else (
                                        ((phase[:32] & 0) | 2743199685)
                                        if phase < 118
                                        else ((phase[:32] & 0) | 2726356676)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 119
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 2777145795)
                                    if phase < 120
                                    else ((phase[:32] & 0) | 2760302786)
                                )
                                if phase < 121
                                else (
                                    ((phase[:32] & 0) | 2810569665)
                                    if phase < 122
                                    else (
                                        ((phase[:32] & 0) | 2793726656)
                                        if phase < 123
                                        else ((phase[:32] & 0) | 3114008031)
                                    )
                                )
                            )
                            if phase < 124
                            else (
                                (
                                    ((phase[:32] & 0) | 3097165022)
                                    if phase < 125
                                    else ((phase[:32] & 0) | 3147431901)
                                )
                                if phase < 126
                                else (
                                    ((phase[:32] & 0) | 3130588892)
                                    if phase < 127
                                    else (
                                        ((phase[:32] & 0) | 3181378011)
                                        if phase < 128
                                        else ((phase[:32] & 0) | 3164535002)
                                    )
                                )
                            )
                        )
                        if phase < 129
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 3214801881)
                                    if phase < 130
                                    else ((phase[:32] & 0) | 3197958872)
                                )
                                if phase < 131
                                else (
                                    ((phase[:32] & 0) | 2979263959)
                                    if phase < 132
                                    else (
                                        ((phase[:32] & 0) | 2962420950)
                                        if phase < 133
                                        else ((phase[:32] & 0) | 3012687829)
                                    )
                                )
                            )
                            if phase < 134
                            else (
                                (
                                    ((phase[:32] & 0) | 2995844820)
                                    if phase < 135
                                    else ((phase[:32] & 0) | 3046633939)
                                )
                                if phase < 136
                                else (
                                    ((phase[:32] & 0) | 3029790930)
                                    if phase < 137
                                    else (
                                        ((phase[:32] & 0) | 3080057809)
                                        if phase < 138
                                        else ((phase[:32] & 0) | 3063214800)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 139
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 3387657647)
                                    if phase < 140
                                    else ((phase[:32] & 0) | 3370814638)
                                )
                                if phase < 141
                                else (
                                    ((phase[:32] & 0) | 3421081517)
                                    if phase < 142
                                    else (
                                        ((phase[:32] & 0) | 3404238508)
                                        if phase < 143
                                        else ((phase[:32] & 0) | 3455027627)
                                    )
                                )
                            )
                            if phase < 144
                            else (
                                (
                                    ((phase[:32] & 0) | 3438184618)
                                    if phase < 145
                                    else ((phase[:32] & 0) | 3488451497)
                                )
                                if phase < 146
                                else (
                                    ((phase[:32] & 0) | 3471608488)
                                    if phase < 147
                                    else (
                                        ((phase[:32] & 0) | 3252913575)
                                        if phase < 148
                                        else ((phase[:32] & 0) | 3236070566)
                                    )
                                )
                            )
                        )
                        if phase < 149
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 3286337445)
                                    if phase < 150
                                    else ((phase[:32] & 0) | 3269494436)
                                )
                                if phase < 151
                                else (
                                    ((phase[:32] & 0) | 3320283555)
                                    if phase < 152
                                    else (
                                        ((phase[:32] & 0) | 3303440546)
                                        if phase < 153
                                        else ((phase[:32] & 0) | 3353707425)
                                    )
                                )
                            )
                            if phase < 154
                            else (
                                (
                                    ((phase[:32] & 0) | 3336864416)
                                    if phase < 155
                                    else ((phase[:32] & 0) | 3657145791)
                                )
                                if phase < 156
                                else (
                                    ((phase[:32] & 0) | 3640302782)
                                    if phase < 157
                                    else (
                                        ((phase[:32] & 0) | 3690569661)
                                        if phase < 158
                                        else ((phase[:32] & 0) | 3673726652)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 159
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 3724515771)
                                    if phase < 160
                                    else ((phase[:32] & 0) | 3707672762)
                                )
                                if phase < 161
                                else (
                                    ((phase[:32] & 0) | 3757939641)
                                    if phase < 162
                                    else (
                                        ((phase[:32] & 0) | 3741096632)
                                        if phase < 163
                                        else ((phase[:32] & 0) | 3522401719)
                                    )
                                )
                            )
                            if phase < 164
                            else (
                                (
                                    ((phase[:32] & 0) | 3505558710)
                                    if phase < 165
                                    else ((phase[:32] & 0) | 3555825589)
                                )
                                if phase < 166
                                else (
                                    ((phase[:32] & 0) | 3538982580)
                                    if phase < 167
                                    else (
                                        ((phase[:32] & 0) | 3589771699)
                                        if phase < 168
                                        else ((phase[:32] & 0) | 3572928690)
                                    )
                                )
                            )
                        )
                        if phase < 169
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 3623195569)
                                    if phase < 170
                                    else ((phase[:32] & 0) | 3606352560)
                                )
                                if phase < 171
                                else (
                                    ((phase[:32] & 0) | 3922439567)
                                    if phase < 172
                                    else (
                                        ((phase[:32] & 0) | 3905596558)
                                        if phase < 173
                                        else ((phase[:32] & 0) | 3955863437)
                                    )
                                )
                            )
                            if phase < 174
                            else (
                                (
                                    ((phase[:32] & 0) | 3939020428)
                                    if phase < 175
                                    else ((phase[:32] & 0) | 3989809547)
                                )
                                if phase < 176
                                else (
                                    ((phase[:32] & 0) | 3972966538)
                                    if phase < 177
                                    else (
                                        ((phase[:32] & 0) | 4023233417)
                                        if phase < 178
                                        else ((phase[:32] & 0) | 4006390408)
                                    )
                                )
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
                            (
                                (
                                    ((phase[:32] & 0) | 3787695495)
                                    if phase < 180
                                    else ((phase[:32] & 0) | 3770852486)
                                )
                                if phase < 181
                                else (
                                    ((phase[:32] & 0) | 3821119365)
                                    if phase < 182
                                    else ((phase[:32] & 0) | 3804276356)
                                )
                            )
                            if phase < 183
                            else (
                                (
                                    ((phase[:32] & 0) | 3855065475)
                                    if phase < 184
                                    else ((phase[:32] & 0) | 3838222466)
                                )
                                if phase < 185
                                else (
                                    ((phase[:32] & 0) | 3888489345)
                                    if phase < 186
                                    else (
                                        ((phase[:32] & 0) | 3871646336)
                                        if phase < 187
                                        else ((phase[:32] & 0) | 4191927711)
                                    )
                                )
                            )
                        )
                        if phase < 188
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 4175084702)
                                    if phase < 189
                                    else ((phase[:32] & 0) | 4225351581)
                                )
                                if phase < 190
                                else (
                                    ((phase[:32] & 0) | 4208508572)
                                    if phase < 191
                                    else (
                                        ((phase[:32] & 0) | 4259297691)
                                        if phase < 192
                                        else ((phase[:32] & 0) | 4242454682)
                                    )
                                )
                            )
                            if phase < 193
                            else (
                                (
                                    ((phase[:32] & 0) | 4292721561)
                                    if phase < 194
                                    else ((phase[:32] & 0) | 4275878552)
                                )
                                if phase < 195
                                else (
                                    ((phase[:32] & 0) | 4057183639)
                                    if phase < 196
                                    else (
                                        ((phase[:32] & 0) | 4040340630)
                                        if phase < 197
                                        else ((phase[:32] & 0) | 4090607509)
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
                                    ((phase[:32] & 0) | 4073764500)
                                    if phase < 199
                                    else ((phase[:32] & 0) | 4124553619)
                                )
                                if phase < 200
                                else (
                                    ((phase[:32] & 0) | 4107710610)
                                    if phase < 201
                                    else (
                                        ((phase[:32] & 0) | 4157977489)
                                        if phase < 202
                                        else ((phase[:32] & 0) | 4141134480)
                                    )
                                )
                            )
                            if phase < 203
                            else (
                                (
                                    ((phase[:32] & 0) | 153832815)
                                    if phase < 204
                                    else ((phase[:32] & 0) | 136989806)
                                )
                                if phase < 205
                                else (
                                    ((phase[:32] & 0) | 187256685)
                                    if phase < 206
                                    else (
                                        ((phase[:32] & 0) | 170413676)
                                        if phase < 207
                                        else ((phase[:32] & 0) | 221202795)
                                    )
                                )
                            )
                        )
                        if phase < 208
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 204359786)
                                    if phase < 209
                                    else ((phase[:32] & 0) | 254626665)
                                )
                                if phase < 210
                                else (
                                    ((phase[:32] & 0) | 237783656)
                                    if phase < 211
                                    else (
                                        ((phase[:32] & 0) | 19088743)
                                        if phase < 212
                                        else ((phase[:32] & 0) | 2245734)
                                    )
                                )
                            )
                            if phase < 213
                            else (
                                (
                                    ((phase[:32] & 0) | 52512613)
                                    if phase < 214
                                    else ((phase[:32] & 0) | 35669604)
                                )
                                if phase < 215
                                else (
                                    ((phase[:32] & 0) | 86458723)
                                    if phase < 216
                                    else (
                                        ((phase[:32] & 0) | 69615714)
                                        if phase < 217
                                        else ((phase[:32] & 0) | 119882593)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 218
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 103039584)
                                    if phase < 219
                                    else ((phase[:32] & 0) | 423320959)
                                )
                                if phase < 220
                                else (
                                    ((phase[:32] & 0) | 406477950)
                                    if phase < 221
                                    else (
                                        ((phase[:32] & 0) | 456744829)
                                        if phase < 222
                                        else ((phase[:32] & 0) | 439901820)
                                    )
                                )
                            )
                            if phase < 223
                            else (
                                (
                                    ((phase[:32] & 0) | 490690939)
                                    if phase < 224
                                    else ((phase[:32] & 0) | 473847930)
                                )
                                if phase < 225
                                else (
                                    ((phase[:32] & 0) | 524114809)
                                    if phase < 226
                                    else (
                                        ((phase[:32] & 0) | 507271800)
                                        if phase < 227
                                        else ((phase[:32] & 0) | 288576887)
                                    )
                                )
                            )
                        )
                        if phase < 228
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 271733878)
                                    if phase < 229
                                    else ((phase[:32] & 0) | 322000757)
                                )
                                if phase < 230
                                else (
                                    ((phase[:32] & 0) | 305157748)
                                    if phase < 231
                                    else (
                                        ((phase[:32] & 0) | 355946867)
                                        if phase < 232
                                        else ((phase[:32] & 0) | 339103858)
                                    )
                                )
                            )
                            if phase < 233
                            else (
                                (
                                    ((phase[:32] & 0) | 389370737)
                                    if phase < 234
                                    else ((phase[:32] & 0) | 372527728)
                                )
                                if phase < 235
                                else (
                                    ((phase[:32] & 0) | 688614735)
                                    if phase < 236
                                    else (
                                        ((phase[:32] & 0) | 671771726)
                                        if phase < 237
                                        else ((phase[:32] & 0) | 722038605)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 238
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 705195596)
                                    if phase < 239
                                    else ((phase[:32] & 0) | 755984715)
                                )
                                if phase < 240
                                else (
                                    ((phase[:32] & 0) | 739141706)
                                    if phase < 241
                                    else (
                                        ((phase[:32] & 0) | 789408585)
                                        if phase < 242
                                        else ((phase[:32] & 0) | 772565576)
                                    )
                                )
                            )
                            if phase < 243
                            else (
                                (
                                    ((phase[:32] & 0) | 553870663)
                                    if phase < 244
                                    else ((phase[:32] & 0) | 537027654)
                                )
                                if phase < 245
                                else (
                                    ((phase[:32] & 0) | 587294533)
                                    if phase < 246
                                    else (
                                        ((phase[:32] & 0) | 570451524)
                                        if phase < 247
                                        else ((phase[:32] & 0) | 621240643)
                                    )
                                )
                            )
                        )
                        if phase < 248
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 604397634)
                                    if phase < 249
                                    else ((phase[:32] & 0) | 654664513)
                                )
                                if phase < 250
                                else (
                                    ((phase[:32] & 0) | 637821504)
                                    if phase < 251
                                    else (
                                        ((phase[:32] & 0) | 958102879)
                                        if phase < 252
                                        else ((phase[:32] & 0) | 941259870)
                                    )
                                )
                            )
                            if phase < 253
                            else (
                                (
                                    ((phase[:32] & 0) | 991526749)
                                    if phase < 254
                                    else ((phase[:32] & 0) | 974683740)
                                )
                                if phase < 255
                                else (
                                    ((phase[:32] & 0) | 1025472859)
                                    if phase < 256
                                    else (
                                        ((phase[:32] & 0) | 1008629850)
                                        if phase < 257
                                        else ((phase[:32] & 0) | 1058896729)
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 258
            else (
                (
                    (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 1042053720)
                                    if phase < 259
                                    else ((phase[:32] & 0) | 823358807)
                                )
                                if phase < 260
                                else (
                                    ((phase[:32] & 0) | 806515798)
                                    if phase < 261
                                    else ((phase[:32] & 0) | 856782677)
                                )
                            )
                            if phase < 262
                            else (
                                (
                                    ((phase[:32] & 0) | 839939668)
                                    if phase < 263
                                    else ((phase[:32] & 0) | 890728787)
                                )
                                if phase < 264
                                else (
                                    ((phase[:32] & 0) | 873885778)
                                    if phase < 265
                                    else (
                                        ((phase[:32] & 0) | 924152657)
                                        if phase < 266
                                        else ((phase[:32] & 0) | 907309648)
                                    )
                                )
                            )
                        )
                        if phase < 267
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 1231752495)
                                    if phase < 268
                                    else ((phase[:32] & 0) | 1214909486)
                                )
                                if phase < 269
                                else (
                                    ((phase[:32] & 0) | 1265176365)
                                    if phase < 270
                                    else (
                                        ((phase[:32] & 0) | 1248333356)
                                        if phase < 271
                                        else ((phase[:32] & 0) | 1299122475)
                                    )
                                )
                            )
                            if phase < 272
                            else (
                                (
                                    ((phase[:32] & 0) | 1282279466)
                                    if phase < 273
                                    else ((phase[:32] & 0) | 1332546345)
                                )
                                if phase < 274
                                else (
                                    ((phase[:32] & 0) | 1315703336)
                                    if phase < 275
                                    else (
                                        ((phase[:32] & 0) | 1097008423)
                                        if phase < 276
                                        else ((phase[:32] & 0) | 1080165414)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 277
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 1130432293)
                                    if phase < 278
                                    else ((phase[:32] & 0) | 1113589284)
                                )
                                if phase < 279
                                else (
                                    ((phase[:32] & 0) | 1164378403)
                                    if phase < 280
                                    else (
                                        ((phase[:32] & 0) | 1147535394)
                                        if phase < 281
                                        else ((phase[:32] & 0) | 1197802273)
                                    )
                                )
                            )
                            if phase < 282
                            else (
                                (
                                    ((phase[:32] & 0) | 1180959264)
                                    if phase < 283
                                    else ((phase[:32] & 0) | 1501240639)
                                )
                                if phase < 284
                                else (
                                    ((phase[:32] & 0) | 1484397630)
                                    if phase < 285
                                    else (
                                        ((phase[:32] & 0) | 1534664509)
                                        if phase < 286
                                        else ((phase[:32] & 0) | 1517821500)
                                    )
                                )
                            )
                        )
                        if phase < 287
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 1568610619)
                                    if phase < 288
                                    else ((phase[:32] & 0) | 1551767610)
                                )
                                if phase < 289
                                else (
                                    ((phase[:32] & 0) | 1602034489)
                                    if phase < 290
                                    else (
                                        ((phase[:32] & 0) | 1585191480)
                                        if phase < 291
                                        else ((phase[:32] & 0) | 1366496567)
                                    )
                                )
                            )
                            if phase < 292
                            else (
                                (
                                    ((phase[:32] & 0) | 1349653558)
                                    if phase < 293
                                    else ((phase[:32] & 0) | 1399920437)
                                )
                                if phase < 294
                                else (
                                    ((phase[:32] & 0) | 1383077428)
                                    if phase < 295
                                    else (
                                        ((phase[:32] & 0) | 1433866547)
                                        if phase < 296
                                        else ((phase[:32] & 0) | 1417023538)
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 297
                else (
                    (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 1467290417)
                                    if phase < 298
                                    else ((phase[:32] & 0) | 1450447408)
                                )
                                if phase < 299
                                else (
                                    ((phase[:32] & 0) | 1766534415)
                                    if phase < 300
                                    else (
                                        ((phase[:32] & 0) | 1749691406)
                                        if phase < 301
                                        else ((phase[:32] & 0) | 1799958285)
                                    )
                                )
                            )
                            if phase < 302
                            else (
                                (
                                    ((phase[:32] & 0) | 1783115276)
                                    if phase < 303
                                    else ((phase[:32] & 0) | 1833904395)
                                )
                                if phase < 304
                                else (
                                    ((phase[:32] & 0) | 1817061386)
                                    if phase < 305
                                    else (
                                        ((phase[:32] & 0) | 1867328265)
                                        if phase < 306
                                        else ((phase[:32] & 0) | 1850485256)
                                    )
                                )
                            )
                        )
                        if phase < 307
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 1631790343)
                                    if phase < 308
                                    else ((phase[:32] & 0) | 1614947334)
                                )
                                if phase < 309
                                else (
                                    ((phase[:32] & 0) | 1665214213)
                                    if phase < 310
                                    else (
                                        ((phase[:32] & 0) | 1648371204)
                                        if phase < 311
                                        else ((phase[:32] & 0) | 1699160323)
                                    )
                                )
                            )
                            if phase < 312
                            else (
                                (
                                    ((phase[:32] & 0) | 1682317314)
                                    if phase < 313
                                    else ((phase[:32] & 0) | 1732584193)
                                )
                                if phase < 314
                                else (
                                    ((phase[:32] & 0) | 1715741184)
                                    if phase < 315
                                    else (
                                        ((phase[:32] & 0) | 2036022559)
                                        if phase < 316
                                        else ((phase[:32] & 0) | 2019179550)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 317
                    else (
                        (
                            (
                                (
                                    ((phase[:32] & 0) | 2069446429)
                                    if phase < 318
                                    else ((phase[:32] & 0) | 2052603420)
                                )
                                if phase < 319
                                else (
                                    ((phase[:32] & 0) | 2103392539)
                                    if phase < 320
                                    else (
                                        ((phase[:32] & 0) | 2086549530)
                                        if phase < 321
                                        else ((phase[:32] & 0) | 2136816409)
                                    )
                                )
                            )
                            if phase < 322
                            else (
                                (
                                    ((phase[:32] & 0) | 2119973400)
                                    if phase < 323
                                    else ((phase[:32] & 0) | 1901278487)
                                )
                                if phase < 324
                                else (
                                    ((phase[:32] & 0) | 1884435478)
                                    if phase < 325
                                    else (
                                        ((phase[:32] & 0) | 1934702357)
                                        if phase < 326
                                        else ((phase[:32] & 0) | 1917859348)
                                    )
                                )
                            )
                        )
                        if phase < 327
                        else (
                            (
                                (
                                    ((phase[:32] & 0) | 1968648467)
                                    if phase < 328
                                    else ((phase[:32] & 0) | 1951805458)
                                )
                                if phase < 329
                                else (
                                    ((phase[:32] & 0) | 2002072337)
                                    if phase < 330
                                    else (
                                        ((phase[:32] & 0) | 1985229328)
                                        if phase < 331
                                        else ((phase[:32] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 338
                            else (
                                (
                                    ((phase[:32] & 0) | 3657145791)
                                    if phase < 341
                                    else ((phase[:32] & 0) | 3640302782)
                                )
                                if phase < 342
                                else (
                                    ((phase[:32] & 0) | 3690569661)
                                    if phase < 343
                                    else (
                                        ((phase[:32] & 0) | 3673726652)
                                        if phase < 344
                                        else ((phase[:32] & 0) | 0)
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
    def check():
        if phase < 347:
            assert dut.left_ready == expected_left_ready, "barrier_pipeline: left_ready"
            assert dut.left_valid == expected_left_valid, "barrier_pipeline: left_valid"
            assert (
                dut.left_data.value == expected_left_data_value
            ), "barrier_pipeline: left_data.value"
            assert (
                dut.right_ready == expected_right_ready
            ), "barrier_pipeline: right_ready"
            assert (
                dut.right_valid == expected_right_valid
            ), "barrier_pipeline: right_valid"
            assert (
                dut.right_data.value == expected_right_data_value
            ), "barrier_pipeline: right_data.value"
        log("info", "barrier_pipeline.left_ready", dut.left_ready)
        log("info", "barrier_pipeline.left_valid", dut.left_valid)
        log("info", "barrier_pipeline.left_data.value", dut.left_data.value)
        log("info", "barrier_pipeline.right_ready", dut.right_ready)
        log("info", "barrier_pipeline.right_valid", dut.right_valid)
        log("info", "barrier_pipeline.right_data.value", dut.right_data.value)

    check()
    advance(phase)

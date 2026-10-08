"""Regular-clock calculator scenario; original physical-control oracles remain."""

from example_calculator.calculator import Calculator
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseCalculator():  # noqa: N802
    phase: bits[64] = 0
    key = (
        (
            (
                (
                    (
                        (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 67
                                else ((phase[:5] & 0) | 1)
                            )
                            if phase < 68
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 69
                                else ((phase[:5] & 0) | 2)
                            )
                        )
                        if phase < 70
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 71
                                else ((phase[:5] & 0) | 10)
                            )
                            if phase < 72
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 73
                                else (
                                    ((phase[:5] & 0) | 3)
                                    if phase < 74
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 75
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 4)
                                if phase < 76
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 77
                            else (
                                ((phase[:5] & 0) | 14)
                                if phase < 78
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 79
                                    else ((phase[:5] & 0) | 15)
                                )
                            )
                        )
                        if phase < 80
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 81
                                else ((phase[:5] & 0) | 7)
                            )
                            if phase < 82
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 83
                                else (
                                    ((phase[:5] & 0) | 12)
                                    if phase < 84
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 85
                else (
                    (
                        (
                            (
                                ((phase[:5] & 0) | 6)
                                if phase < 86
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 87
                            else (
                                ((phase[:5] & 0) | 14)
                                if phase < 88
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 89
                                    else ((phase[:5] & 0) | 15)
                                )
                            )
                        )
                        if phase < 90
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 91
                                else ((phase[:5] & 0) | 9)
                            )
                            if phase < 92
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 93
                                else (
                                    ((phase[:5] & 0) | 11)
                                    if phase < 94
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 95
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 4)
                                if phase < 96
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 97
                            else (
                                ((phase[:5] & 0) | 14)
                                if phase < 98
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 99
                                    else ((phase[:5] & 0) | 15)
                                )
                            )
                        )
                        if phase < 100
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 101
                                else ((phase[:5] & 0) | 8)
                            )
                            if phase < 102
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 103
                                else (
                                    ((phase[:5] & 0) | 13)
                                    if phase < 104
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 105
            else (
                (
                    (
                        (
                            (
                                ((phase[:5] & 0) | 14)
                                if phase < 106
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 107
                            else (
                                ((phase[:5] & 0) | 15)
                                if phase < 108
                                else ((phase[:5] & 0) | 0)
                            )
                        )
                        if phase < 109
                        else (
                            (
                                ((phase[:5] & 0) | 1)
                                if phase < 110
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 115
                            else (
                                ((phase[:5] & 0) | 13)
                                if phase < 116
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 117
                                    else ((phase[:5] & 0) | 4)
                                )
                            )
                        )
                    )
                    if phase < 118
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 119
                                else ((phase[:5] & 0) | 14)
                            )
                            if phase < 120
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 121
                                else (
                                    ((phase[:5] & 0) | 15)
                                    if phase < 122
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                        if phase < 123
                        else (
                            (
                                ((phase[:5] & 0) | 1)
                                if phase < 124
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 125
                            else (
                                ((phase[:5] & 0) | 8)
                                if phase < 126
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 127
                                    else ((phase[:5] & 0) | 4)
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
                                ((phase[:5] & 0) | 0)
                                if phase < 129
                                else ((phase[:5] & 0) | 4)
                            )
                            if phase < 130
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 131
                                else (
                                    ((phase[:5] & 0) | 6)
                                    if phase < 132
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                        if phase < 133
                        else (
                            (
                                ((phase[:5] & 0) | 7)
                                if phase < 134
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 135
                            else (
                                ((phase[:5] & 0) | 4)
                                if phase < 136
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 137
                                    else ((phase[:5] & 0) | 4)
                                )
                            )
                        )
                    )
                    if phase < 138
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 141
                                else ((phase[:5] & 0) | 7)
                            )
                            if phase < 142
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 143
                                else (
                                    ((phase[:5] & 0) | 3)
                                    if phase < 144
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                        if phase < 145
                        else (
                            (
                                ((phase[:5] & 0) | 7)
                                if phase < 146
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 149
                            else (
                                ((phase[:5] & 0) | 9)
                                if phase < 150
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 151
                                    else ((phase[:5] & 0) | 5)
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 152
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 153
                                else ((phase[:5] & 0) | 5)
                            )
                            if phase < 154
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 155
                                else ((phase[:5] & 0) | 1)
                            )
                        )
                        if phase < 156
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 157
                                else ((phase[:5] & 0) | 6)
                            )
                            if phase < 158
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 159
                                else (
                                    ((phase[:5] & 0) | 1)
                                    if phase < 160
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 161
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 5)
                                if phase < 162
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 163
                            else (
                                ((phase[:5] & 0) | 13)
                                if phase < 164
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 165
                                    else ((phase[:5] & 0) | 3)
                                )
                            )
                        )
                        if phase < 166
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 167
                                else ((phase[:5] & 0) | 14)
                            )
                            if phase < 168
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 169
                                else (
                                    ((phase[:5] & 0) | 12)
                                    if phase < 170
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 171
                else (
                    (
                        (
                            (
                                ((phase[:5] & 0) | 3)
                                if phase < 172
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 173
                            else (
                                ((phase[:5] & 0) | 14)
                                if phase < 174
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 175
                                    else ((phase[:5] & 0) | 10)
                                )
                            )
                        )
                        if phase < 176
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 177
                                else ((phase[:5] & 0) | 1)
                            )
                            if phase < 178
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 179
                                else (
                                    ((phase[:5] & 0) | 14)
                                    if phase < 180
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 181
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 11)
                                if phase < 182
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 183
                            else (
                                ((phase[:5] & 0) | 1)
                                if phase < 184
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 185
                                    else ((phase[:5] & 0) | 14)
                                )
                            )
                        )
                        if phase < 186
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 187
                                else ((phase[:5] & 0) | 12)
                            )
                            if phase < 188
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 189
                                else (
                                    ((phase[:5] & 0) | 2)
                                    if phase < 190
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 191
            else (
                (
                    (
                        (
                            (
                                ((phase[:5] & 0) | 14)
                                if phase < 192
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 193
                            else (
                                ((phase[:5] & 0) | 15)
                                if phase < 194
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 195
                                    else ((phase[:5] & 0) | 1)
                                )
                            )
                        )
                        if phase < 196
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 197
                                else ((phase[:5] & 0) | 8)
                            )
                            if phase < 198
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 199
                                else (
                                    ((phase[:5] & 0) | 4)
                                    if phase < 200
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 201
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 4)
                                if phase < 202
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 203
                            else (
                                ((phase[:5] & 0) | 6)
                                if phase < 204
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 205
                                    else ((phase[:5] & 0) | 7)
                                )
                            )
                        )
                        if phase < 206
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 207
                                else ((phase[:5] & 0) | 4)
                            )
                            if phase < 208
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 209
                                else (
                                    ((phase[:5] & 0) | 4)
                                    if phase < 210
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 213
                else (
                    (
                        (
                            (
                                ((phase[:5] & 0) | 7)
                                if phase < 214
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 215
                            else (
                                ((phase[:5] & 0) | 3)
                                if phase < 216
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 217
                                    else ((phase[:5] & 0) | 7)
                                )
                            )
                        )
                        if phase < 218
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 221
                                else ((phase[:5] & 0) | 9)
                            )
                            if phase < 222
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 223
                                else (
                                    ((phase[:5] & 0) | 5)
                                    if phase < 224
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 225
                    else (
                        (
                            (
                                ((phase[:5] & 0) | 5)
                                if phase < 226
                                else ((phase[:5] & 0) | 0)
                            )
                            if phase < 227
                            else (
                                ((phase[:5] & 0) | 1)
                                if phase < 228
                                else (
                                    ((phase[:5] & 0) | 0)
                                    if phase < 229
                                    else ((phase[:5] & 0) | 6)
                                )
                            )
                        )
                        if phase < 230
                        else (
                            (
                                ((phase[:5] & 0) | 0)
                                if phase < 231
                                else ((phase[:5] & 0) | 1)
                            )
                            if phase < 232
                            else (
                                ((phase[:5] & 0) | 0)
                                if phase < 233
                                else (
                                    ((phase[:5] & 0) | 5)
                                    if phase < 234
                                    else ((phase[:5] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    key_press = (
        (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 67
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 68
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 69
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 70
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 71
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 72
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 73
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 74
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 75
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 76
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 77
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 78
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 79
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 80
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 81
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 82
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 83
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                            if phase < 84
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 85
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 86
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 87
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 88
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 89
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 90
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 91
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 92
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 93
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 94
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 95
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 96
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 97
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 98
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 99
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 100
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 101
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 102
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 103
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 104
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                            if phase < 105
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 106
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 107
                                    else ((phase[:1] & 0) | 1)
                                )
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
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 109
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 110
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 111
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 112
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 113
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 114
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 115
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 116
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 117
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 118
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 119
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 120
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 121
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 122
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 123
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 124
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 125
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                            if phase < 126
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 127
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 128
                                    else ((phase[:1] & 0) | 0)
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
                                ((phase[:1] & 0) | 1)
                                if phase < 130
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 131
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 132
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 133
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 134
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 135
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 136
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                            if phase < 137
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 138
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 139
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 140
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 141
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 142
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 143
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 144
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 145
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 146
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 147
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                            if phase < 148
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 149
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 150
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 151
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 152
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 153
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 154
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 155
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 156
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 157
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 158
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 159
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 160
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 161
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 162
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 163
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 164
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 165
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 166
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 167
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 168
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                            if phase < 169
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 170
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 171
                                    else ((phase[:1] & 0) | 1)
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
                                ((phase[:1] & 0) | 0)
                                if phase < 173
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 174
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 175
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 176
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 177
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 178
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 179
                                    else ((phase[:1] & 0) | 1)
                                )
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
                    if phase < 183
                    else (
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
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 187
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 188
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 189
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 190
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                            if phase < 191
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 192
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 193
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 194
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 195
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 196
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 197
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 198
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 199
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 200
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 201
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 202
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 203
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 204
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 205
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 206
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 207
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 208
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 209
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 210
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 211
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                            if phase < 212
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
                if phase < 215
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 216
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 217
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 218
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 219
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 220
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 221
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 222
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                            if phase < 223
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 224
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 225
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 226
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 227
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 228
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 229
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 230
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 231
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 232
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 233
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                            if phase < 234
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 235
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 236
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    dut = Calculator(key, key_press)

    expected_display = (
        (
            (
                (
                    (
                        (
                            ((phase[:64] & 0) | 0)
                            if phase < 68
                            else ((phase[:64] & 0) | 1)
                        )
                        if phase < 70
                        else (
                            ((phase[:64] & 0) | 12)
                            if phase < 74
                            else ((phase[:64] & 0) | 3)
                        )
                    )
                    if phase < 76
                    else (
                        (
                            ((phase[:64] & 0) | 34)
                            if phase < 78
                            else ((phase[:64] & 0) | 46)
                        )
                        if phase < 80
                        else (
                            ((phase[:64] & 0) | 0)
                            if phase < 82
                            else (
                                ((phase[:64] & 0) | 7)
                                if phase < 86
                                else ((phase[:64] & 0) | 6)
                            )
                        )
                    )
                )
                if phase < 88
                else (
                    (
                        (
                            ((phase[:64] & 0) | 42)
                            if phase < 90
                            else ((phase[:64] & 0) | 0)
                        )
                        if phase < 92
                        else (
                            ((phase[:64] & 0) | 9)
                            if phase < 96
                            else ((phase[:64] & 0) | 4)
                        )
                    )
                    if phase < 98
                    else (
                        (
                            ((phase[:64] & 0) | 5)
                            if phase < 100
                            else ((phase[:64] & 0) | 0)
                        )
                        if phase < 102
                        else (
                            ((phase[:64] & 0) | 8)
                            if phase < 108
                            else (
                                ((phase[:64] & 0) | 0)
                                if phase < 110
                                else ((phase[:64] & 0) | 1)
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
                            ((phase[:64] & 0) | 10)
                            if phase < 114
                            else ((phase[:64] & 0) | 100)
                        )
                        if phase < 118
                        else (
                            ((phase[:64] & 0) | 4)
                            if phase < 120
                            else ((phase[:64] & 0) | 25)
                        )
                    )
                    if phase < 122
                    else (
                        (
                            ((phase[:64] & 0) | 0)
                            if phase < 124
                            else ((phase[:64] & 0) | 1)
                        )
                        if phase < 126
                        else (
                            ((phase[:64] & 0) | 18)
                            if phase < 128
                            else (
                                ((phase[:64] & 0) | 184)
                                if phase < 130
                                else ((phase[:64] & 0) | 1844)
                            )
                        )
                    )
                )
                if phase < 132
                else (
                    (
                        (
                            ((phase[:64] & 0) | 18446)
                            if phase < 134
                            else ((phase[:64] & 0) | 184467)
                        )
                        if phase < 136
                        else (
                            ((phase[:64] & 0) | 1844674)
                            if phase < 138
                            else (
                                ((phase[:64] & 0) | 18446744)
                                if phase < 140
                                else ((phase[:64] & 0) | 184467440)
                            )
                        )
                    )
                    if phase < 142
                    else (
                        (
                            ((phase[:64] & 0) | 1844674407)
                            if phase < 144
                            else ((phase[:64] & 0) | 18446744073)
                        )
                        if phase < 146
                        else (
                            ((phase[:64] & 0) | 184467440737)
                            if phase < 148
                            else (
                                ((phase[:64] & 0) | 1844674407370)
                                if phase < 150
                                else ((phase[:64] & 0) | 18446744073709)
                            )
                        )
                    )
                )
            )
        )
        if phase < 152
        else (
            (
                (
                    (
                        (
                            ((phase[:64] & 0) | 184467440737095)
                            if phase < 154
                            else ((phase[:64] & 0) | 1844674407370955)
                        )
                        if phase < 156
                        else (
                            ((phase[:64] & 0) | 18446744073709551)
                            if phase < 158
                            else ((phase[:64] & 0) | 184467440737095516)
                        )
                    )
                    if phase < 160
                    else (
                        (
                            ((phase[:64] & 0) | 1844674407370955161)
                            if phase < 162
                            else ((phase[:64] & 0) | 18446744073709551615)
                        )
                        if phase < 166
                        else (
                            ((phase[:64] & 0) | 3)
                            if phase < 168
                            else (
                                ((phase[:64] & 0) | 6148914691236517205)
                                if phase < 172
                                else ((phase[:64] & 0) | 3)
                            )
                        )
                    )
                )
                if phase < 174
                else (
                    (
                        (
                            ((phase[:64] & 0) | 18446744073709551615)
                            if phase < 178
                            else ((phase[:64] & 0) | 1)
                        )
                        if phase < 180
                        else (
                            ((phase[:64] & 0) | 0)
                            if phase < 184
                            else (
                                ((phase[:64] & 0) | 1)
                                if phase < 186
                                else ((phase[:64] & 0) | 18446744073709551615)
                            )
                        )
                    )
                    if phase < 190
                    else (
                        (
                            ((phase[:64] & 0) | 2)
                            if phase < 192
                            else ((phase[:64] & 0) | 18446744073709551614)
                        )
                        if phase < 194
                        else (
                            ((phase[:64] & 0) | 0)
                            if phase < 196
                            else (
                                ((phase[:64] & 0) | 1)
                                if phase < 198
                                else ((phase[:64] & 0) | 18)
                            )
                        )
                    )
                )
            )
            if phase < 200
            else (
                (
                    (
                        (
                            ((phase[:64] & 0) | 184)
                            if phase < 202
                            else ((phase[:64] & 0) | 1844)
                        )
                        if phase < 204
                        else (
                            ((phase[:64] & 0) | 18446)
                            if phase < 206
                            else ((phase[:64] & 0) | 184467)
                        )
                    )
                    if phase < 208
                    else (
                        (
                            ((phase[:64] & 0) | 1844674)
                            if phase < 210
                            else ((phase[:64] & 0) | 18446744)
                        )
                        if phase < 212
                        else (
                            ((phase[:64] & 0) | 184467440)
                            if phase < 214
                            else (
                                ((phase[:64] & 0) | 1844674407)
                                if phase < 216
                                else ((phase[:64] & 0) | 18446744073)
                            )
                        )
                    )
                )
                if phase < 218
                else (
                    (
                        (
                            ((phase[:64] & 0) | 184467440737)
                            if phase < 220
                            else ((phase[:64] & 0) | 1844674407370)
                        )
                        if phase < 222
                        else (
                            ((phase[:64] & 0) | 18446744073709)
                            if phase < 224
                            else (
                                ((phase[:64] & 0) | 184467440737095)
                                if phase < 226
                                else ((phase[:64] & 0) | 1844674407370955)
                            )
                        )
                    )
                    if phase < 228
                    else (
                        (
                            ((phase[:64] & 0) | 18446744073709551)
                            if phase < 230
                            else ((phase[:64] & 0) | 184467440737095516)
                        )
                        if phase < 232
                        else (
                            ((phase[:64] & 0) | 1844674407370955161)
                            if phase < 234
                            else (
                                ((phase[:64] & 0) | 18446744073709551615)
                                if phase < 236
                                else ((phase[:64] & 0) | 18446744073709551606)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_op_pending = (
        (
            (
                ((phase[:2] & 0) | 0)
                if phase < 84
                else (((phase[:2] & 0) | 2) if phase < 90 else ((phase[:2] & 0) | 0))
            )
            if phase < 94
            else (
                (((phase[:2] & 0) | 1) if phase < 100 else ((phase[:2] & 0) | 0))
                if phase < 104
                else (((phase[:2] & 0) | 3) if phase < 108 else ((phase[:2] & 0) | 0))
            )
        )
        if phase < 116
        else (
            (
                (((phase[:2] & 0) | 3) if phase < 122 else ((phase[:2] & 0) | 0))
                if phase < 164
                else (((phase[:2] & 0) | 3) if phase < 170 else ((phase[:2] & 0) | 2))
            )
            if phase < 176
            else (
                (((phase[:2] & 0) | 0) if phase < 182 else ((phase[:2] & 0) | 1))
                if phase < 188
                else (((phase[:2] & 0) | 2) if phase < 194 else ((phase[:2] & 0) | 0))
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 237:
            assert dut.display == expected_display, "calculator: display"
            assert dut.op_pending == expected_op_pending, "calculator: op_pending"

        log("info", "calculator.display", dut.display)
        log("info", "calculator.op_pending", dut.op_pending)

    check_and_advance()
    advance(phase)

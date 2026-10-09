"""Regular-clock full known-input stream from the retained loop_control_pipeline oracle."""

from example_loop_control_pipeline.loop_control_pipeline import (
    LoopControlPipeline,
    LoopToken,
)
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseLoopControlPipeline():  # noqa: N802
    phase: bits[64] = 0
    valid = (
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
                            if phase < 2
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 5
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 6
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 9
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 10
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 13
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 14
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 17
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 18
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 22
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 23
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 27
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 28
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 31
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 32
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 35
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 36
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 41
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 42
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 47
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 48
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 51
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 52
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 55
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 56
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 62
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 63
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 69
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 70
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 73
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 74
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 77
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 78
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 85
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 86
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 93
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 94
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 97
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 98
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 101
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 102
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 110
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 111
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
                            if phase < 123
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 124
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 127
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 128
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 137
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 138
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 147
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 148
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
                    if phase < 155
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 156
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 166
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 167
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 177
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 178
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 181
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 182
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 185
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 186
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 197
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 198
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 209
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 210
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 213
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 214
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 217
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 218
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 230
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 231
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 243
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 244
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 247
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 248
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 251
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 252
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 265
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 266
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 279
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 280
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 283
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 284
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 287
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 288
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 302
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 303
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 317
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 318
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 321
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 322
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 325
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 326
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 341
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 342
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 357
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 358
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 361
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 362
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 365
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 366
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 382
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 383
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 399
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 400
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 403
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 404
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 407
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 408
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 425
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
                if phase < 426
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 443
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 444
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 447
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 448
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 451
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 452
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 470
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 471
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 489
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 490
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 493
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 494
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 497
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 601
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 625
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 630
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    remaining = (
        (
            (
                (
                    (
                        (
                            (
                                ((phase[:4] & 0) | 0)
                                if phase < 17
                                else (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 18
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 22
                            else (
                                (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 23
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 27
                                else (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 28
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                        if phase < 31
                        else (
                            (
                                ((phase[:4] & 0) | 1)
                                if phase < 32
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 35
                                    else ((phase[:4] & 0) | 2)
                                )
                            )
                            if phase < 36
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 41
                                    else ((phase[:4] & 0) | 2)
                                )
                                if phase < 42
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 47
                                    else ((phase[:4] & 0) | 2)
                                )
                            )
                        )
                    )
                    if phase < 48
                    else (
                        (
                            (
                                ((phase[:4] & 0) | 0)
                                if phase < 51
                                else (
                                    ((phase[:4] & 0) | 2)
                                    if phase < 52
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 55
                            else (
                                (
                                    ((phase[:4] & 0) | 3)
                                    if phase < 56
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 62
                                else (
                                    ((phase[:4] & 0) | 3)
                                    if phase < 63
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                        if phase < 69
                        else (
                            (
                                ((phase[:4] & 0) | 3)
                                if phase < 70
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 73
                                    else ((phase[:4] & 0) | 3)
                                )
                            )
                            if phase < 74
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 77
                                    else ((phase[:4] & 0) | 4)
                                )
                                if phase < 78
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 85
                                    else ((phase[:4] & 0) | 4)
                                )
                            )
                        )
                    )
                )
                if phase < 86
                else (
                    (
                        (
                            (
                                ((phase[:4] & 0) | 0)
                                if phase < 93
                                else (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 94
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 97
                            else (
                                (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 98
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 101
                                else (
                                    ((phase[:4] & 0) | 5)
                                    if phase < 102
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                        if phase < 110
                        else (
                            (
                                ((phase[:4] & 0) | 5)
                                if phase < 111
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 119
                                    else ((phase[:4] & 0) | 5)
                                )
                            )
                            if phase < 120
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 123
                                    else ((phase[:4] & 0) | 5)
                                )
                                if phase < 124
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 127
                                    else ((phase[:4] & 0) | 6)
                                )
                            )
                        )
                    )
                    if phase < 128
                    else (
                        (
                            (
                                ((phase[:4] & 0) | 0)
                                if phase < 137
                                else (
                                    ((phase[:4] & 0) | 6)
                                    if phase < 138
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 147
                            else (
                                (
                                    ((phase[:4] & 0) | 6)
                                    if phase < 148
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 151
                                else (
                                    ((phase[:4] & 0) | 6)
                                    if phase < 152
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                        if phase < 155
                        else (
                            (
                                (
                                    ((phase[:4] & 0) | 7)
                                    if phase < 156
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 166
                                else (
                                    ((phase[:4] & 0) | 7)
                                    if phase < 167
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 177
                            else (
                                (
                                    ((phase[:4] & 0) | 7)
                                    if phase < 178
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 181
                                else (
                                    ((phase[:4] & 0) | 7)
                                    if phase < 182
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 185
            else (
                (
                    (
                        (
                            (
                                ((phase[:4] & 0) | 8)
                                if phase < 186
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 197
                                    else ((phase[:4] & 0) | 8)
                                )
                            )
                            if phase < 198
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 209
                                    else ((phase[:4] & 0) | 8)
                                )
                                if phase < 210
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 213
                                    else ((phase[:4] & 0) | 8)
                                )
                            )
                        )
                        if phase < 214
                        else (
                            (
                                ((phase[:4] & 0) | 0)
                                if phase < 217
                                else (
                                    ((phase[:4] & 0) | 9)
                                    if phase < 218
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 230
                            else (
                                (
                                    ((phase[:4] & 0) | 9)
                                    if phase < 231
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 243
                                else (
                                    ((phase[:4] & 0) | 9)
                                    if phase < 244
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 247
                    else (
                        (
                            (
                                ((phase[:4] & 0) | 9)
                                if phase < 248
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 251
                                    else ((phase[:4] & 0) | 10)
                                )
                            )
                            if phase < 252
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 265
                                    else ((phase[:4] & 0) | 10)
                                )
                                if phase < 266
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 279
                                    else ((phase[:4] & 0) | 10)
                                )
                            )
                        )
                        if phase < 280
                        else (
                            (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 283
                                    else ((phase[:4] & 0) | 10)
                                )
                                if phase < 284
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 287
                                    else ((phase[:4] & 0) | 11)
                                )
                            )
                            if phase < 288
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 302
                                    else ((phase[:4] & 0) | 11)
                                )
                                if phase < 303
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 317
                                    else ((phase[:4] & 0) | 11)
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
                                ((phase[:4] & 0) | 0)
                                if phase < 321
                                else (
                                    ((phase[:4] & 0) | 11)
                                    if phase < 322
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 325
                            else (
                                (
                                    ((phase[:4] & 0) | 12)
                                    if phase < 326
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 341
                                else (
                                    ((phase[:4] & 0) | 12)
                                    if phase < 342
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                        if phase < 357
                        else (
                            (
                                ((phase[:4] & 0) | 12)
                                if phase < 358
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 361
                                    else ((phase[:4] & 0) | 12)
                                )
                            )
                            if phase < 362
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 365
                                    else ((phase[:4] & 0) | 13)
                                )
                                if phase < 366
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 382
                                    else ((phase[:4] & 0) | 13)
                                )
                            )
                        )
                    )
                    if phase < 383
                    else (
                        (
                            (
                                ((phase[:4] & 0) | 0)
                                if phase < 399
                                else (
                                    ((phase[:4] & 0) | 13)
                                    if phase < 400
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 403
                            else (
                                (
                                    ((phase[:4] & 0) | 13)
                                    if phase < 404
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 407
                                else (
                                    ((phase[:4] & 0) | 14)
                                    if phase < 408
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                        if phase < 425
                        else (
                            (
                                (
                                    ((phase[:4] & 0) | 14)
                                    if phase < 426
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 443
                                else (
                                    ((phase[:4] & 0) | 14)
                                    if phase < 444
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 447
                            else (
                                (
                                    ((phase[:4] & 0) | 14)
                                    if phase < 448
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 451
                                else (
                                    ((phase[:4] & 0) | 15)
                                    if phase < 452
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 470
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:4] & 0) | 15)
                                if phase < 471
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 489
                                    else ((phase[:4] & 0) | 15)
                                )
                            )
                            if phase < 490
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 493
                                    else ((phase[:4] & 0) | 15)
                                )
                                if phase < 494
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 497
                                    else ((phase[:4] & 0) | 9)
                                )
                            )
                        )
                        if phase < 498
                        else (
                            (
                                ((phase[:4] & 0) | 15)
                                if phase < 499
                                else (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 500
                                    else ((phase[:4] & 0) | 7)
                                )
                            )
                            if phase < 501
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 502
                                    else ((phase[:4] & 0) | 1)
                                )
                                if phase < 503
                                else (
                                    ((phase[:4] & 0) | 2)
                                    if phase < 504
                                    else ((phase[:4] & 0) | 3)
                                )
                            )
                        )
                    )
                    if phase < 505
                    else (
                        (
                            (
                                ((phase[:4] & 0) | 4)
                                if phase < 506
                                else (
                                    ((phase[:4] & 0) | 5)
                                    if phase < 507
                                    else ((phase[:4] & 0) | 6)
                                )
                            )
                            if phase < 508
                            else (
                                (
                                    ((phase[:4] & 0) | 7)
                                    if phase < 509
                                    else ((phase[:4] & 0) | 8)
                                )
                                if phase < 510
                                else (
                                    ((phase[:4] & 0) | 9)
                                    if phase < 511
                                    else ((phase[:4] & 0) | 10)
                                )
                            )
                        )
                        if phase < 512
                        else (
                            (
                                (
                                    ((phase[:4] & 0) | 11)
                                    if phase < 513
                                    else ((phase[:4] & 0) | 12)
                                )
                                if phase < 514
                                else (
                                    ((phase[:4] & 0) | 13)
                                    if phase < 515
                                    else ((phase[:4] & 0) | 14)
                                )
                            )
                            if phase < 516
                            else (
                                (
                                    ((phase[:4] & 0) | 15)
                                    if phase < 517
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 518
                                else (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 519
                                    else ((phase[:4] & 0) | 2)
                                )
                            )
                        )
                    )
                )
                if phase < 520
                else (
                    (
                        (
                            (
                                ((phase[:4] & 0) | 3)
                                if phase < 521
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 522
                                    else ((phase[:4] & 0) | 1)
                                )
                            )
                            if phase < 523
                            else (
                                (
                                    ((phase[:4] & 0) | 2)
                                    if phase < 524
                                    else ((phase[:4] & 0) | 3)
                                )
                                if phase < 525
                                else (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 526
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                        if phase < 527
                        else (
                            (
                                ((phase[:4] & 0) | 1)
                                if phase < 528
                                else (
                                    ((phase[:4] & 0) | 2)
                                    if phase < 529
                                    else ((phase[:4] & 0) | 3)
                                )
                            )
                            if phase < 530
                            else (
                                (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 531
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 532
                                else (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 533
                                    else ((phase[:4] & 0) | 2)
                                )
                            )
                        )
                    )
                    if phase < 534
                    else (
                        (
                            (
                                ((phase[:4] & 0) | 3)
                                if phase < 535
                                else (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 536
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 537
                            else (
                                (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 538
                                    else ((phase[:4] & 0) | 2)
                                )
                                if phase < 539
                                else (
                                    ((phase[:4] & 0) | 3)
                                    if phase < 540
                                    else ((phase[:4] & 0) | 4)
                                )
                            )
                        )
                        if phase < 541
                        else (
                            (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 542
                                    else ((phase[:4] & 0) | 1)
                                )
                                if phase < 543
                                else (
                                    ((phase[:4] & 0) | 2)
                                    if phase < 544
                                    else ((phase[:4] & 0) | 3)
                                )
                            )
                            if phase < 545
                            else (
                                (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 546
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 547
                                else (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 548
                                    else ((phase[:4] & 0) | 2)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 549
            else (
                (
                    (
                        (
                            (
                                ((phase[:4] & 0) | 3)
                                if phase < 550
                                else (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 551
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                            if phase < 552
                            else (
                                (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 553
                                    else ((phase[:4] & 0) | 2)
                                )
                                if phase < 554
                                else (
                                    ((phase[:4] & 0) | 3)
                                    if phase < 555
                                    else ((phase[:4] & 0) | 4)
                                )
                            )
                        )
                        if phase < 556
                        else (
                            (
                                ((phase[:4] & 0) | 0)
                                if phase < 557
                                else (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 558
                                    else ((phase[:4] & 0) | 2)
                                )
                            )
                            if phase < 559
                            else (
                                (
                                    ((phase[:4] & 0) | 3)
                                    if phase < 560
                                    else ((phase[:4] & 0) | 4)
                                )
                                if phase < 561
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 562
                                    else ((phase[:4] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 563
                    else (
                        (
                            (
                                ((phase[:4] & 0) | 2)
                                if phase < 564
                                else (
                                    ((phase[:4] & 0) | 3)
                                    if phase < 565
                                    else ((phase[:4] & 0) | 4)
                                )
                            )
                            if phase < 566
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 567
                                    else ((phase[:4] & 0) | 1)
                                )
                                if phase < 568
                                else (
                                    ((phase[:4] & 0) | 2)
                                    if phase < 569
                                    else ((phase[:4] & 0) | 3)
                                )
                            )
                        )
                        if phase < 570
                        else (
                            (
                                (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 571
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 572
                                else (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 573
                                    else ((phase[:4] & 0) | 2)
                                )
                            )
                            if phase < 574
                            else (
                                (
                                    ((phase[:4] & 0) | 3)
                                    if phase < 575
                                    else ((phase[:4] & 0) | 4)
                                )
                                if phase < 576
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 577
                                    else ((phase[:4] & 0) | 1)
                                )
                            )
                        )
                    )
                )
                if phase < 578
                else (
                    (
                        (
                            (
                                ((phase[:4] & 0) | 2)
                                if phase < 579
                                else (
                                    ((phase[:4] & 0) | 3)
                                    if phase < 580
                                    else ((phase[:4] & 0) | 4)
                                )
                            )
                            if phase < 581
                            else (
                                (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 582
                                    else ((phase[:4] & 0) | 1)
                                )
                                if phase < 583
                                else (
                                    ((phase[:4] & 0) | 2)
                                    if phase < 584
                                    else ((phase[:4] & 0) | 3)
                                )
                            )
                        )
                        if phase < 585
                        else (
                            (
                                ((phase[:4] & 0) | 4)
                                if phase < 586
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 587
                                    else ((phase[:4] & 0) | 1)
                                )
                            )
                            if phase < 588
                            else (
                                (
                                    ((phase[:4] & 0) | 2)
                                    if phase < 589
                                    else ((phase[:4] & 0) | 3)
                                )
                                if phase < 590
                                else (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 591
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 592
                    else (
                        (
                            (
                                ((phase[:4] & 0) | 1)
                                if phase < 593
                                else (
                                    ((phase[:4] & 0) | 2)
                                    if phase < 594
                                    else ((phase[:4] & 0) | 3)
                                )
                            )
                            if phase < 595
                            else (
                                (
                                    ((phase[:4] & 0) | 4)
                                    if phase < 596
                                    else ((phase[:4] & 0) | 0)
                                )
                                if phase < 597
                                else (
                                    ((phase[:4] & 0) | 1)
                                    if phase < 598
                                    else ((phase[:4] & 0) | 2)
                                )
                            )
                        )
                        if phase < 599
                        else (
                            (
                                (
                                    ((phase[:4] & 0) | 3)
                                    if phase < 600
                                    else ((phase[:4] & 0) | 4)
                                )
                                if phase < 601
                                else (
                                    ((phase[:4] & 0) | 0)
                                    if phase < 625
                                    else ((phase[:4] & 0) | 9)
                                )
                            )
                            if phase < 626
                            else (
                                (
                                    ((phase[:4] & 0) | 15)
                                    if phase < 627
                                    else ((phase[:4] & 0) | 4)
                                )
                                if phase < 628
                                else (
                                    ((phase[:4] & 0) | 7)
                                    if phase < 629
                                    else ((phase[:4] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    stop = (
        (
            (
                (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 9
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 10
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 13
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 14
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 27
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 28
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 31
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 32
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 47
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 48
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 51
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 52
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 69
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 70
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 73
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 74
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 93
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 94
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 97
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 98
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 119
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 120
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 123
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 124
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 147
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 148
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 151
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 152
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 177
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 178
                                else ((phase[:1] & 0) | 0)
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
                                ((phase[:1] & 0) | 1)
                                if phase < 182
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 209
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 210
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 213
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 214
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 243
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 244
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 247
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 248
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 279
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 280
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 283
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 284
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 317
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 318
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 321
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 322
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 357
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 358
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 361
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 362
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 399
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 400
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 403
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 404
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 443
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 444
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 447
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 448
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 489
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 490
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 493
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 494
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 497
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 498
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 499
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 501
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 521
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 522
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 524
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 525
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 527
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 528
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 530
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 531
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 533
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 534
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 536
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 537
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 539
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 540
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 542
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 543
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 545
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 546
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 548
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 549
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 551
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 552
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 554
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 555
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 557
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 558
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 560
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 561
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 563
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 564
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 566
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 567
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 569
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 570
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 572
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 573
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 575
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 576
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 578
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 579
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 581
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 582
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 584
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 585
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 587
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 588
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 590
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 591
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 593
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 594
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 596
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 597
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 599
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 600
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 625
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 626
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 627
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 629
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    skip = (
        (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 5
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 6
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 13
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 14
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 22
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 23
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 31
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 32
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 41
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 42
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 51
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 52
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
                            if phase < 73
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 74
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 85
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
                if phase < 86
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 97
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 98
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 110
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 111
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 123
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 124
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 137
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 138
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 151
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 152
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 166
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 167
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 181
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 182
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 197
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 198
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
            if phase < 230
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 231
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 247
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 248
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 265
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 266
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 283
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 284
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 302
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 303
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 321
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 322
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 341
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 342
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 361
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 362
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 382
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 383
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 403
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 404
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 425
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 426
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 447
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 448
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 470
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 471
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 493
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 494
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 498
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 500
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 522
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 523
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 524
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 525
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 526
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 527
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 528
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 529
        else (
            (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 530
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 531
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 532
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 533
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 534
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 535
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 536
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 537
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 538
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 539
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 540
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 541
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 542
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 543
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 544
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 545
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 546
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
                if phase < 547
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 548
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 549
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 550
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 551
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 552
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 553
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 554
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 555
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 556
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 557
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 558
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 559
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 560
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 561
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 562
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 563
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 564
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 565
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 566
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 567
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 568
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 569
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 570
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 571
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 572
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 573
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 574
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 575
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 576
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 577
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 578
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 579
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 580
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 581
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 582
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 583
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 584
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
                if phase < 585
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 586
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 587
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 588
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 589
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 590
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 591
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 592
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 593
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 594
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 595
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 596
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 597
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 598
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 599
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 600
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 601
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 626
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 628
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    take = (
        (
            ((phase[:1] & 0) | 0)
            if phase < 1
            else (((phase[:1] & 0) | 1) if phase < 497 else ((phase[:1] & 0) | 0))
        )
        if phase < 521
        else (
            ((phase[:1] & 0) | 1)
            if phase < 625
            else (((phase[:1] & 0) | 0) if phase < 629 else ((phase[:1] & 0) | 1))
        )
    )
    payload = LoopToken(remaining=remaining, stop=stop, skip=skip)
    dut = LoopControlPipeline(valid, payload, take)
    expected_ready = (
        (
            (
                (
                    (((phase[:1] & 0) | 1) if phase < 501 else ((phase[:1] & 0) | 0))
                    if phase < 522
                    else (
                        ((phase[:1] & 0) | 1) if phase < 525 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 526
                else (
                    (((phase[:1] & 0) | 1) if phase < 527 else ((phase[:1] & 0) | 0))
                    if phase < 529
                    else (
                        ((phase[:1] & 0) | 1)
                        if phase < 532
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 535
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                )
            )
            if phase < 538
            else (
                (
                    (((phase[:1] & 0) | 0) if phase < 542 else ((phase[:1] & 0) | 1))
                    if phase < 544
                    else (
                        ((phase[:1] & 0) | 0)
                        if phase < 545
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 547
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 549
                else (
                    (((phase[:1] & 0) | 1) if phase < 552 else ((phase[:1] & 0) | 0))
                    if phase < 555
                    else (
                        ((phase[:1] & 0) | 1)
                        if phase < 556
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 560
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                )
            )
        )
        if phase < 562
        else (
            (
                (
                    (((phase[:1] & 0) | 0) if phase < 566 else ((phase[:1] & 0) | 1))
                    if phase < 570
                    else (
                        ((phase[:1] & 0) | 0) if phase < 571 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 572
                else (
                    (((phase[:1] & 0) | 0) if phase < 574 else ((phase[:1] & 0) | 1))
                    if phase < 577
                    else (
                        ((phase[:1] & 0) | 0)
                        if phase < 580
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 583
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
            )
            if phase < 587
            else (
                (
                    (((phase[:1] & 0) | 1) if phase < 589 else ((phase[:1] & 0) | 0))
                    if phase < 590
                    else (
                        ((phase[:1] & 0) | 1)
                        if phase < 592
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 594
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                )
                if phase < 597
                else (
                    (((phase[:1] & 0) | 0) if phase < 600 else ((phase[:1] & 0) | 1))
                    if phase < 601
                    else (
                        ((phase[:1] & 0) | 0)
                        if phase < 605
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 629
                            else ((phase[:1] & 0) | 0)
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
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 3
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 4
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 7
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 8
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 11
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 12
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 15
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 16
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 20
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
                                if phase < 25
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 26
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 29
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 30
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 33
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 34
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 39
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                            if phase < 40
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 45
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 46
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 49
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 50
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 53
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 54
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 60
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 61
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 67
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 68
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 71
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 72
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 75
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 76
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 83
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 84
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
                                if phase < 95
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 96
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                            if phase < 99
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 100
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 108
                                    else ((phase[:1] & 0) | 1)
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
                                ((phase[:1] & 0) | 0)
                                if phase < 117
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 118
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
                        if phase < 125
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 126
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 135
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 136
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 145
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 146
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 149
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 150
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 153
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 154
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 164
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 165
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 175
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                            if phase < 176
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 179
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 180
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
                            if phase < 195
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 196
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 207
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 208
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 211
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 212
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 215
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 216
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 228
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 229
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 241
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 242
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 245
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 246
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 249
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 250
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                            if phase < 263
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 264
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 277
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
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
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 281
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 282
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 285
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 286
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 300
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 301
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 315
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 316
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 319
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 320
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 323
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 324
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 339
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 340
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 355
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 356
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 359
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                            if phase < 360
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 363
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 364
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 380
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 381
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 397
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 398
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 401
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 402
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 405
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 406
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 423
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 424
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                    if phase < 441
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 442
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 445
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 446
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 449
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 450
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 468
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 469
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                            if phase < 487
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 488
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 491
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                )
            )
            if phase < 492
            else (
                (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 495
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 496
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 499
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 525
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 526
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 527
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 529
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 532
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 535
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 538
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 542
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 544
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 545
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 547
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 549
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 552
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 555
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                            if phase < 556
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 560
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 562
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
                if phase < 566
                else (
                    (
                        (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 570
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 571
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 572
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 574
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                        if phase < 577
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 580
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 583
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                            if phase < 587
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 589
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 590
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                        )
                    )
                    if phase < 592
                    else (
                        (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 594
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 597
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 600
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 601
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                        if phase < 605
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 607
                                else (
                                    ((phase[:1] & 0) | 0)
                                    if phase < 611
                                    else ((phase[:1] & 0) | 1)
                                )
                            )
                            if phase < 612
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 627
                                else (
                                    ((phase[:1] & 0) | 1)
                                    if phase < 630
                                    else ((phase[:1] & 0) | 0)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    expected_data_remaining = (
        (
            (
                (
                    (
                        (((phase[:4] & 0) | 0) if phase < 29 else ((phase[:4] & 0) | 1))
                        if phase < 30
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 33
                            else (
                                ((phase[:4] & 0) | 1)
                                if phase < 34
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                    )
                    if phase < 49
                    else (
                        (((phase[:4] & 0) | 2) if phase < 50 else ((phase[:4] & 0) | 0))
                        if phase < 53
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 54
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 71
                                else ((phase[:4] & 0) | 3)
                            )
                        )
                    )
                )
                if phase < 72
                else (
                    (
                        (((phase[:4] & 0) | 0) if phase < 75 else ((phase[:4] & 0) | 3))
                        if phase < 76
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 95
                            else (
                                ((phase[:4] & 0) | 4)
                                if phase < 96
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                    )
                    if phase < 99
                    else (
                        (
                            ((phase[:4] & 0) | 4)
                            if phase < 100
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 121
                                else ((phase[:4] & 0) | 5)
                            )
                        )
                        if phase < 122
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 125
                            else (
                                ((phase[:4] & 0) | 5)
                                if phase < 126
                                else ((phase[:4] & 0) | 0)
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
                            ((phase[:4] & 0) | 6)
                            if phase < 150
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 153
                        else (
                            ((phase[:4] & 0) | 6)
                            if phase < 154
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 179
                                else ((phase[:4] & 0) | 7)
                            )
                        )
                    )
                    if phase < 180
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 183
                            else (
                                ((phase[:4] & 0) | 7)
                                if phase < 184
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                        if phase < 211
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 212
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 215
                                else ((phase[:4] & 0) | 8)
                            )
                        )
                    )
                )
                if phase < 216
                else (
                    (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 245
                            else ((phase[:4] & 0) | 9)
                        )
                        if phase < 246
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 249
                            else (
                                ((phase[:4] & 0) | 9)
                                if phase < 250
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                    )
                    if phase < 281
                    else (
                        (
                            ((phase[:4] & 0) | 10)
                            if phase < 282
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 285
                                else ((phase[:4] & 0) | 10)
                            )
                        )
                        if phase < 286
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 319
                            else (
                                ((phase[:4] & 0) | 11)
                                if phase < 320
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 323
        else (
            (
                (
                    (
                        (
                            ((phase[:4] & 0) | 11)
                            if phase < 324
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 359
                        else (
                            ((phase[:4] & 0) | 12)
                            if phase < 360
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 363
                                else ((phase[:4] & 0) | 12)
                            )
                        )
                    )
                    if phase < 364
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 401
                            else ((phase[:4] & 0) | 13)
                        )
                        if phase < 402
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 405
                            else (
                                ((phase[:4] & 0) | 13)
                                if phase < 406
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 445
                else (
                    (
                        (
                            ((phase[:4] & 0) | 14)
                            if phase < 446
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 449
                        else (
                            ((phase[:4] & 0) | 14)
                            if phase < 450
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 491
                                else ((phase[:4] & 0) | 15)
                            )
                        )
                    )
                    if phase < 492
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 495
                            else (
                                ((phase[:4] & 0) | 15)
                                if phase < 496
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                        if phase < 499
                        else (
                            ((phase[:4] & 0) | 9)
                            if phase < 522
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 523
                                else ((phase[:4] & 0) | 4)
                            )
                        )
                    )
                )
            )
            if phase < 524
            else (
                (
                    (
                        (
                            ((phase[:4] & 0) | 7)
                            if phase < 525
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 530
                        else (
                            ((phase[:4] & 0) | 3)
                            if phase < 531
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 536
                                else ((phase[:4] & 0) | 4)
                            )
                        )
                    )
                    if phase < 537
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 546
                            else (
                                ((phase[:4] & 0) | 1)
                                if phase < 547
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                        if phase < 550
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 551
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 567
                                else ((phase[:4] & 0) | 4)
                            )
                        )
                    )
                )
                if phase < 568
                else (
                    (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 575
                            else ((phase[:4] & 0) | 3)
                        )
                        if phase < 576
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 581
                            else (
                                ((phase[:4] & 0) | 4)
                                if phase < 582
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                    )
                    if phase < 591
                    else (
                        (
                            ((phase[:4] & 0) | 1)
                            if phase < 592
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 595
                                else ((phase[:4] & 0) | 4)
                            )
                        )
                        if phase < 596
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 627
                            else (
                                ((phase[:4] & 0) | 9)
                                if phase < 630
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_data_stop = (
        (
            (
                (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 11
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 12
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 15
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 16
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 29
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 30
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 33
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 34
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 49
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 50
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 53
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 54
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 71
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 72
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 75
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 76
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 95
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 96
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 99
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 100
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 121
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 122
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 125
                                else ((phase[:1] & 0) | 1)
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
                            ((phase[:1] & 0) | 0)
                            if phase < 149
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 150
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 153
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 154
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 179
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 180
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 183
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 184
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 211
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 212
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 215
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 216
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 245
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 246
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 249
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 250
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 281
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 282
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 285
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 286
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 319
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 320
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 323
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 324
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 359
        else (
            (
                (
                    (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 360
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 363
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 364
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 401
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 402
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 405
                    else (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 406
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 445
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 446
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 449
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 450
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 491
                else (
                    (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 492
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 495
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 496
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 499
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 522
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 523
                    else (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 525
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 530
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 531
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 536
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 537
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 543
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
            if phase < 544
            else (
                (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 546
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 547
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 550
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 551
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 561
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 562
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 567
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 568
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 569
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 570
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 575
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 576
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 581
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 582
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 588
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 589
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 591
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 592
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 595
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 596
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 606
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 607
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 627
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 630
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_data_skip = (
        (
            (
                (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 7
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 8
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 15
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 16
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
                            ((phase[:1] & 0) | 0)
                            if phase < 33
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 34
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 45
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 46
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 53
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 54
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 67
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 68
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 75
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 76
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
                            if phase < 99
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 100
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 117
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 118
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 125
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 126
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 145
            else (
                (
                    (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 146
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 153
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 154
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
                    if phase < 183
                    else (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 184
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 207
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 208
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 215
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 216
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 241
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 242
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 249
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 250
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 277
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 278
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 285
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 286
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 315
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 316
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 323
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 324
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 355
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 356
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 363
        else (
            (
                (
                    (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 364
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 397
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 398
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 405
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 406
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 441
                    else (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 442
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 449
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 450
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 487
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 488
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 495
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 496
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 522
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 524
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 526
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 527
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 530
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 532
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 536
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 537
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 543
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 544
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 546
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 547
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 551
            else (
                (
                    (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 552
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 560
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 561
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 567
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 568
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                    if phase < 569
                    else (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 570
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 574
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                        if phase < 575
                        else (
                            (
                                ((phase[:1] & 0) | 0)
                                if phase < 580
                                else ((phase[:1] & 0) | 1)
                            )
                            if phase < 581
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 582
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 583
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 587
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 588
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 590
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 591
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 594
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                    if phase < 596
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 600
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 601
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                        if phase < 606
                        else (
                            (
                                ((phase[:1] & 0) | 1)
                                if phase < 607
                                else ((phase[:1] & 0) | 0)
                            )
                            if phase < 611
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 612
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )

    @rule
    def check():
        if phase < 635:
            assert dut.ready == expected_ready, "loop_control_pipeline: ready"
            assert dut.valid == expected_valid, "loop_control_pipeline: valid"
            assert (
                dut.data.remaining == expected_data_remaining
            ), "loop_control_pipeline: data.remaining"
            assert (
                dut.data.stop == expected_data_stop
            ), "loop_control_pipeline: data.stop"
            assert (
                dut.data.skip == expected_data_skip
            ), "loop_control_pipeline: data.skip"
        log("info", "loop_control_pipeline.ready", dut.ready)
        log("info", "loop_control_pipeline.valid", dut.valid)
        log("info", "loop_control_pipeline.data.remaining", dut.data.remaining)
        log("info", "loop_control_pipeline.data.stop", dut.data.stop)
        log("info", "loop_control_pipeline.data.skip", dut.data.skip)

    check()
    advance(phase)

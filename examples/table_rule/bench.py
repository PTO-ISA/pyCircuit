"""Regular-clock full known-input stream from the retained table_rule oracle."""

from example_table_rule.table_rule import TableRule, Entry
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseTableRule():  # noqa: N802
    phase: bits[64] = 0
    valid = (
        (
            (
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
                                    if phase < 5
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 13
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 30
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 38
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 41
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 50
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 53
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 55
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 58
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 60
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 63
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 65
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 68
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 70
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 73
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 75
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 78
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 80
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 83
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 85
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 88
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 90
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 93
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 95
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 98
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 100
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 103
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 105
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 108
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 110
                                        else ((phase[:1] & 0) | 1)
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
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 115
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 118
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 120
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 123
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 125
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 128
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 130
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 133
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 135
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 138
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 140
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 143
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 145
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 148
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 150
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 153
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 155
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 158
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 160
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 163
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 165
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 168
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 170
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 173
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 175
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 178
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 180
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 183
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 185
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 188
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 190
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 193
                                            else ((phase[:1] & 0) | 0)
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
                                        ((phase[:1] & 0) | 1)
                                        if phase < 198
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 200
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 203
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 205
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 208
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 210
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 213
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 215
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 218
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 220
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 223
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 225
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 228
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 230
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 233
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 235
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 238
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 240
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 243
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 245
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 248
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 250
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 253
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 255
                            else (
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
                                if phase < 265
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 268
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 270
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 273
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 275
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 278
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 280
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 283
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 285
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 288
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 290
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 293
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 295
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 298
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 300
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 303
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 305
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 308
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 310
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 313
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 315
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 318
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 320
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 323
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 325
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 328
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 330
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 333
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 335
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 338
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 340
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 343
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 345
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 348
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 350
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 353
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 355
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 358
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 360
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 363
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 365
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 368
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 370
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 373
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 375
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 378
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 380
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 383
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 385
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 388
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 390
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 393
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 395
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 398
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 400
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 403
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 405
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 408
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 410
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 413
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 415
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 418
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 420
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 423
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 425
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 428
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 430
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 433
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 435
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 438
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 440
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 443
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 445
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 448
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 450
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 453
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 455
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 458
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 460
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 463
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 465
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 468
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 470
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 473
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 475
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 478
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 480
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 483
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 485
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 488
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 490
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 493
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 495
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 498
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 500
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 503
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 505
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 508
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 510
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 513
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 515
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 518
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
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
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 523
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 525
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
                                        if phase < 533
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 535
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 538
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 540
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 543
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 545
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 548
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 550
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 553
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 555
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 558
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 560
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 563
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 565
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 568
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 570
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 573
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 575
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 578
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 580
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 583
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 585
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
                                        if phase < 593
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 595
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 598
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 600
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 603
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 605
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 608
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 610
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 613
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 615
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 618
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 620
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 623
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 625
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 628
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 630
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 633
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 635
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 638
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 640
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 643
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 645
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 648
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 650
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 653
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 655
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 658
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 660
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 663
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 665
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 668
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 670
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 673
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 675
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 678
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 680
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 683
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 685
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 688
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 690
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 693
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 695
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 698
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 700
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 703
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 705
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 708
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 710
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 713
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 715
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 718
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 720
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 723
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 725
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 728
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 730
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 733
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 735
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 738
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 740
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 743
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 745
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 748
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 750
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 753
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 755
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 758
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 760
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 763
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 765
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 768
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 770
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 773
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 775
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 778
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 780
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 783
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 785
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 788
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 790
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 793
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 795
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 798
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 800
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 803
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 805
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 808
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 810
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 813
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 815
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 818
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 820
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 823
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 825
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 828
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 830
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 833
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 835
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 838
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 840
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 843
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 845
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 848
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 850
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 853
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 855
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 858
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 860
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 863
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 865
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 868
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 870
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 873
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 875
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 878
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 880
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 883
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 885
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 888
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 890
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 893
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 895
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 898
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 900
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 903
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 905
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 908
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 910
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 913
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 915
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 918
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 920
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 923
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 925
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 928
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 930
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 933
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 935
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 938
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 940
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 943
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 945
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 948
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 950
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 953
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 955
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 958
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 960
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 963
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 965
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 968
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 970
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 973
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 975
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 978
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 980
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 983
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 985
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 988
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 990
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 993
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 995
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 998
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1000
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1003
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1005
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 1008
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1010
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1013
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1015
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1018
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1020
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1023
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1025
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1028
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1030
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1033
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1035
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1038
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1040
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1043
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1045
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 1048
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1050
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1053
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1055
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1058
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1060
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1063
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1065
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1068
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1070
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1073
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1075
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1078
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1080
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1083
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1085
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1088
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1090
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1093
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1095
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1098
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1100
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1103
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1105
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1108
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1110
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1113
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1115
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1118
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1120
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1123
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1125
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 1128
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1130
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1133
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1135
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1138
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1140
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1143
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1145
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1148
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1150
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1153
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1155
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1158
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1160
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1163
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1165
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1168
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1170
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1173
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1175
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1178
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1180
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1183
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1185
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1188
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1190
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1193
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1195
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1198
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1200
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1203
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1205
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1208
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 1210
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1213
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1215
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1218
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1220
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1223
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1225
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1228
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1230
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1233
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1235
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1238
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1240
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1243
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1245
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1248
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1250
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1253
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1255
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1258
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1260
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1263
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1265
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1268
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1270
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1273
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1275
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1278
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1280
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1283
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1285
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1288
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1290
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 1293
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1295
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1298
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1300
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1303
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1305
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1308
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1310
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1313
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1315
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1318
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1320
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1323
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1325
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1328
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1330
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1334
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    index = (
        (
            (
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
                                        if phase < 14
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 15
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 16
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 18
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 19
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 20
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 21
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 22
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 23
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 24
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 25
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 26
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 27
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 28
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 29
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 30
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 39
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 40
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 52
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 53
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 57
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 58
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 62
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 63
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 67
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 68
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 72
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 73
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 77
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 78
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 82
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 83
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 87
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 88
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 92
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 93
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
                            if phase < 102
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 103
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 107
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 108
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 112
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 113
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 117
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 118
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 122
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 123
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 127
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 128
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 132
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 133
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 137
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 138
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 142
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 143
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 147
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 148
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 152
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 153
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 157
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 158
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 162
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
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
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 167
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 168
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 172
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 173
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 177
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 178
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 182
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 183
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 187
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 188
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 192
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 193
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 197
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 198
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 202
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 203
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 207
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 208
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 212
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 213
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 217
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 218
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 222
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 223
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
                                        if phase < 232
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 233
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 237
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 238
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 242
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 243
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 247
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 248
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 252
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 253
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 257
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 258
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 262
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 263
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 267
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 268
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 272
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 273
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 277
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 278
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 282
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 283
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 287
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 288
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 292
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 293
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 297
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 298
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 302
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 303
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 307
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 308
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 312
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 313
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
                                        if phase < 322
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 323
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 327
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 328
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 332
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 333
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 337
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 338
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 342
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 343
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 347
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 348
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 352
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 353
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 357
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 358
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 362
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 363
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 367
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 368
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 372
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 373
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 377
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 378
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 382
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 383
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 387
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 388
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 392
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 393
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 397
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 398
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 402
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 403
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 407
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 408
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 412
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 413
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 417
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 418
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 422
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 423
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 427
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 428
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 432
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 433
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 437
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 438
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 442
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 443
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 447
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 448
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 452
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 453
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 457
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 458
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 462
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 463
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 467
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 468
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 472
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 473
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 477
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 478
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 482
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 483
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 487
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 488
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 492
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 493
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 497
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 498
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 502
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 503
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 507
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 508
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 512
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 513
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 517
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 518
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 522
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 523
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 527
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 528
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 532
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 533
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 537
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 538
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 542
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 543
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 547
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 548
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 552
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 553
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 557
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 558
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 562
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 563
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 567
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 568
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 572
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 573
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 577
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 578
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 582
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 583
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 587
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 588
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 592
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 593
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 597
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 598
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 602
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 603
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 607
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 608
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 612
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 613
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 617
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 618
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 622
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 623
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 627
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 628
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 632
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 633
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 637
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 638
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 642
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 643
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 647
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 648
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 652
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 653
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 657
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 658
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 662
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 663
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 667
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 668
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 672
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 673
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 677
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 678
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 682
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 683
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 687
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 688
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 690
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 692
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 695
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 697
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 700
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 702
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 705
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 707
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 710
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 712
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 715
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 717
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 720
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 722
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 725
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 727
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 730
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 732
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 735
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 737
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 740
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 742
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 745
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 747
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 750
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 752
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 755
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 757
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 760
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 762
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 765
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 767
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 770
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 772
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 775
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 777
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 780
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 782
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 785
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 787
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 790
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 792
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 795
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 797
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 800
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 802
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 805
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 807
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 810
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 812
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 815
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 817
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 820
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 822
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 825
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 827
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 830
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 832
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 835
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 837
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 840
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 842
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 845
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 847
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 850
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 852
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 855
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 857
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 860
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 862
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 865
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 867
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 870
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 872
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 875
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 877
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 880
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 882
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 885
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 887
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 890
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 892
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 895
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 897
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 900
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 902
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 905
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 907
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 910
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 912
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 915
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 917
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 920
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 922
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 925
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 927
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 930
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 932
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 935
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 937
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 940
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 942
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 945
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 947
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 950
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 952
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 955
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 957
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 960
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 962
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 965
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 967
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 970
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 972
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 975
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 977
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 980
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 982
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 985
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 987
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 990
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 992
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 995
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 997
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 1000
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1002
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1005
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1007
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1010
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1012
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1015
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1017
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1020
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1022
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1025
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1027
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1030
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1032
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1035
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1037
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 1040
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1042
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1045
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1047
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1050
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1052
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1055
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1057
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1060
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1062
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1065
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1067
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1070
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1072
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1075
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1077
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1080
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1082
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1085
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1087
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1090
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1092
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1095
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1097
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1100
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1102
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1105
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1107
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1110
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1112
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1115
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1117
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1120
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1122
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1125
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1127
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1130
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1132
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1135
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1137
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1140
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1142
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1145
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1147
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1150
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1152
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1155
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1157
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1160
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1162
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1165
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1167
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1170
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1172
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1175
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1177
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1180
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1182
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1185
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1187
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1190
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1192
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1195
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1197
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1200
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1202
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1205
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 1207
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1210
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1212
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1215
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1217
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1220
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1222
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1225
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1227
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1230
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1232
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1235
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1237
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1240
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1242
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1245
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1247
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1250
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1252
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1255
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1257
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1260
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1262
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1265
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1267
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1270
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1272
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1275
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1277
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1280
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1282
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1285
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1287
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1290
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1292
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1295
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1297
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1300
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1302
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1305
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1307
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1310
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1312
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1315
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1317
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1320
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1322
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1325
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1327
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1331
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1332
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    value = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1
                                            else ((phase[:7] & 0) | 10)
                                        )
                                        if phase < 2
                                        else (
                                            ((phase[:7] & 0) | 20)
                                            if phase < 3
                                            else ((phase[:7] & 0) | 30)
                                        )
                                    )
                                    if phase < 4
                                    else (
                                        (
                                            ((phase[:7] & 0) | 40)
                                            if phase < 5
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 13
                                        else (
                                            ((phase[:7] & 0) | 50)
                                            if phase < 14
                                            else ((phase[:7] & 0) | 60)
                                        )
                                    )
                                )
                                if phase < 15
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 70)
                                            if phase < 16
                                            else ((phase[:7] & 0) | 80)
                                        )
                                        if phase < 17
                                        else (
                                            ((phase[:7] & 0) | 110)
                                            if phase < 18
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 19
                                    else (
                                        (
                                            ((phase[:7] & 0) | 11)
                                            if phase < 20
                                            else ((phase[:7] & 0) | 22)
                                        )
                                        if phase < 21
                                        else (
                                            ((phase[:7] & 0) | 33)
                                            if phase < 22
                                            else ((phase[:7] & 0) | 44)
                                        )
                                    )
                                )
                            )
                            if phase < 23
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 55)
                                            if phase < 24
                                            else ((phase[:7] & 0) | 66)
                                        )
                                        if phase < 25
                                        else (
                                            ((phase[:7] & 0) | 77)
                                            if phase < 26
                                            else ((phase[:7] & 0) | 88)
                                        )
                                    )
                                    if phase < 27
                                    else (
                                        (
                                            ((phase[:7] & 0) | 99)
                                            if phase < 28
                                            else ((phase[:7] & 0) | 110)
                                        )
                                        if phase < 29
                                        else (
                                            ((phase[:7] & 0) | 121)
                                            if phase < 30
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 38
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 50)
                                            if phase < 39
                                            else ((phase[:7] & 0) | 60)
                                        )
                                        if phase < 40
                                        else (
                                            ((phase[:7] & 0) | 70)
                                            if phase < 41
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 51
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 52
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 53
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 55
                                            else ((phase[:7] & 0) | 1)
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
                                            ((phase[:7] & 0) | 3)
                                            if phase < 57
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 58
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 60
                                            else ((phase[:7] & 0) | 2)
                                        )
                                    )
                                    if phase < 61
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 62
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 63
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 65
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 67
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 68
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 70
                                        else (
                                            ((phase[:7] & 0) | 4)
                                            if phase < 71
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 72
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 73
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 75
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 76
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 77
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 78
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 80
                                        else (
                                            ((phase[:7] & 0) | 6)
                                            if phase < 81
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 82
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 83
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 85
                                        else (
                                            ((phase[:7] & 0) | 7)
                                            if phase < 86
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 87
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 88
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 90
                                        else (
                                            ((phase[:7] & 0) | 8)
                                            if phase < 91
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 92
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 93
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 95
                                        else (
                                            ((phase[:7] & 0) | 9)
                                            if phase < 96
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 97
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
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
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 100
                                            else ((phase[:7] & 0) | 10)
                                        )
                                        if phase < 101
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 102
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 103
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 105
                                            else ((phase[:7] & 0) | 11)
                                        )
                                        if phase < 106
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 107
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 108
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 110
                                            else ((phase[:7] & 0) | 12)
                                        )
                                        if phase < 111
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 112
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 113
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 115
                                            else ((phase[:7] & 0) | 13)
                                        )
                                        if phase < 116
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 117
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 118
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 120
                                            else ((phase[:7] & 0) | 14)
                                        )
                                        if phase < 121
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 122
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 123
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 125
                                            else ((phase[:7] & 0) | 15)
                                        )
                                        if phase < 126
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 127
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 128
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 130
                                            else ((phase[:7] & 0) | 16)
                                        )
                                        if phase < 131
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 132
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 133
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 135
                                            else ((phase[:7] & 0) | 17)
                                        )
                                        if phase < 136
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 137
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 138
                                                else ((phase[:7] & 0) | 0)
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
                                            ((phase[:7] & 0) | 18)
                                            if phase < 141
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 142
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 143
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 145
                                    else (
                                        (
                                            ((phase[:7] & 0) | 19)
                                            if phase < 146
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 147
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 148
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 150
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 20)
                                            if phase < 151
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 152
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 153
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 155
                                    else (
                                        (
                                            ((phase[:7] & 0) | 21)
                                            if phase < 156
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 157
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 158
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 160
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 22)
                                            if phase < 161
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 162
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 163
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 165
                                    else (
                                        (
                                            ((phase[:7] & 0) | 23)
                                            if phase < 166
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 167
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 168
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 170
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 24)
                                            if phase < 171
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 172
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 173
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 175
                                    else (
                                        (
                                            ((phase[:7] & 0) | 25)
                                            if phase < 176
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 177
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 178
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 180
                                                else ((phase[:7] & 0) | 26)
                                            )
                                        )
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
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 182
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 183
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 185
                                            else ((phase[:7] & 0) | 27)
                                        )
                                    )
                                    if phase < 186
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 187
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 188
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 190
                                            else ((phase[:7] & 0) | 28)
                                        )
                                    )
                                )
                                if phase < 191
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 192
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 193
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 195
                                            else ((phase[:7] & 0) | 29)
                                        )
                                    )
                                    if phase < 196
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 197
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 198
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 200
                                            else ((phase[:7] & 0) | 30)
                                        )
                                    )
                                )
                            )
                            if phase < 201
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 202
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 203
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 205
                                            else ((phase[:7] & 0) | 31)
                                        )
                                    )
                                    if phase < 206
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 207
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 208
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 210
                                            else ((phase[:7] & 0) | 32)
                                        )
                                    )
                                )
                                if phase < 211
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 212
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 213
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 215
                                            else ((phase[:7] & 0) | 33)
                                        )
                                    )
                                    if phase < 216
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 217
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 218
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 220
                                            else (
                                                ((phase[:7] & 0) | 34)
                                                if phase < 221
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 222
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 223
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 225
                                        else (
                                            ((phase[:7] & 0) | 35)
                                            if phase < 226
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 227
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 228
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 230
                                        else (
                                            ((phase[:7] & 0) | 36)
                                            if phase < 231
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 232
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 233
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 235
                                        else (
                                            ((phase[:7] & 0) | 37)
                                            if phase < 236
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 237
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 238
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 240
                                        else (
                                            ((phase[:7] & 0) | 38)
                                            if phase < 241
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 242
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 243
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 245
                                        else (
                                            ((phase[:7] & 0) | 39)
                                            if phase < 246
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 247
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 248
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 250
                                        else (
                                            ((phase[:7] & 0) | 40)
                                            if phase < 251
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 252
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 253
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 255
                                        else (
                                            ((phase[:7] & 0) | 41)
                                            if phase < 256
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 257
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 258
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 260
                                        else (
                                            ((phase[:7] & 0) | 42)
                                            if phase < 261
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 262
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 263
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 265
                                            else ((phase[:7] & 0) | 43)
                                        )
                                        if phase < 266
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 267
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 268
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 270
                                            else ((phase[:7] & 0) | 44)
                                        )
                                        if phase < 271
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 272
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 273
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 275
                                            else ((phase[:7] & 0) | 45)
                                        )
                                        if phase < 276
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 277
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 278
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 280
                                            else ((phase[:7] & 0) | 46)
                                        )
                                        if phase < 281
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 282
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 283
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 285
                                            else ((phase[:7] & 0) | 47)
                                        )
                                        if phase < 286
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 287
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 288
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 290
                                            else ((phase[:7] & 0) | 48)
                                        )
                                        if phase < 291
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 292
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 293
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 295
                                            else ((phase[:7] & 0) | 49)
                                        )
                                        if phase < 296
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 297
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 298
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 300
                                            else ((phase[:7] & 0) | 50)
                                        )
                                        if phase < 301
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 302
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 303
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 305
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 51)
                                            if phase < 306
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 307
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 308
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 310
                                    else (
                                        (
                                            ((phase[:7] & 0) | 52)
                                            if phase < 311
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 312
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 313
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 315
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 53)
                                            if phase < 316
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 317
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 318
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 320
                                    else (
                                        (
                                            ((phase[:7] & 0) | 54)
                                            if phase < 321
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 322
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 323
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 325
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 55)
                                            if phase < 326
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 327
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 328
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 330
                                    else (
                                        (
                                            ((phase[:7] & 0) | 56)
                                            if phase < 331
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 332
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 333
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 335
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 57)
                                            if phase < 336
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 337
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 338
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 340
                                    else (
                                        (
                                            ((phase[:7] & 0) | 58)
                                            if phase < 341
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 342
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 343
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 345
                                                else ((phase[:7] & 0) | 59)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 346
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 347
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 348
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 350
                                            else ((phase[:7] & 0) | 60)
                                        )
                                    )
                                    if phase < 351
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 352
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 353
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 355
                                            else ((phase[:7] & 0) | 61)
                                        )
                                    )
                                )
                                if phase < 356
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 357
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 358
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 360
                                            else ((phase[:7] & 0) | 62)
                                        )
                                    )
                                    if phase < 361
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 362
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 363
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 365
                                            else ((phase[:7] & 0) | 63)
                                        )
                                    )
                                )
                            )
                            if phase < 366
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 367
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 368
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 370
                                            else ((phase[:7] & 0) | 64)
                                        )
                                    )
                                    if phase < 371
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 372
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 373
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 375
                                            else ((phase[:7] & 0) | 65)
                                        )
                                    )
                                )
                                if phase < 376
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 377
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 378
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 380
                                            else ((phase[:7] & 0) | 66)
                                        )
                                    )
                                    if phase < 381
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 382
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 383
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 385
                                            else ((phase[:7] & 0) | 67)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 386
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 387
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 388
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 390
                                            else ((phase[:7] & 0) | 68)
                                        )
                                    )
                                    if phase < 391
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 392
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 393
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 395
                                            else ((phase[:7] & 0) | 69)
                                        )
                                    )
                                )
                                if phase < 396
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 397
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 398
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 400
                                            else ((phase[:7] & 0) | 70)
                                        )
                                    )
                                    if phase < 401
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 402
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 403
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 405
                                            else ((phase[:7] & 0) | 71)
                                        )
                                    )
                                )
                            )
                            if phase < 406
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 407
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 408
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 410
                                            else ((phase[:7] & 0) | 72)
                                        )
                                    )
                                    if phase < 411
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 412
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 413
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 415
                                            else ((phase[:7] & 0) | 73)
                                        )
                                    )
                                )
                                if phase < 416
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 417
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 418
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 420
                                            else ((phase[:7] & 0) | 74)
                                        )
                                    )
                                    if phase < 421
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 422
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 423
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 425
                                            else (
                                                ((phase[:7] & 0) | 75)
                                                if phase < 426
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 427
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 428
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 430
                                        else (
                                            ((phase[:7] & 0) | 76)
                                            if phase < 431
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 432
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 433
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 435
                                        else (
                                            ((phase[:7] & 0) | 77)
                                            if phase < 436
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 437
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 438
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 440
                                        else (
                                            ((phase[:7] & 0) | 78)
                                            if phase < 441
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 442
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 443
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 445
                                        else (
                                            ((phase[:7] & 0) | 79)
                                            if phase < 446
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 447
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 448
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 450
                                        else (
                                            ((phase[:7] & 0) | 80)
                                            if phase < 451
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 452
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 453
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 455
                                        else (
                                            ((phase[:7] & 0) | 81)
                                            if phase < 456
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 457
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 458
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 460
                                        else (
                                            ((phase[:7] & 0) | 82)
                                            if phase < 461
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 462
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 463
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 465
                                        else (
                                            ((phase[:7] & 0) | 83)
                                            if phase < 466
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 467
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 468
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 470
                                            else ((phase[:7] & 0) | 84)
                                        )
                                        if phase < 471
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 472
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 473
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 475
                                            else ((phase[:7] & 0) | 85)
                                        )
                                        if phase < 476
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 477
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 478
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 480
                                            else ((phase[:7] & 0) | 86)
                                        )
                                        if phase < 481
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 482
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 483
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 485
                                            else ((phase[:7] & 0) | 87)
                                        )
                                        if phase < 486
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 487
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 488
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 490
                                            else ((phase[:7] & 0) | 88)
                                        )
                                        if phase < 491
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 492
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 493
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 495
                                            else ((phase[:7] & 0) | 89)
                                        )
                                        if phase < 496
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 497
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 498
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 500
                                            else ((phase[:7] & 0) | 90)
                                        )
                                        if phase < 501
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 502
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 503
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 505
                                            else ((phase[:7] & 0) | 91)
                                        )
                                        if phase < 506
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 507
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 508
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 510
                else (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 92)
                                            if phase < 511
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 512
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 513
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 515
                                    else (
                                        (
                                            ((phase[:7] & 0) | 93)
                                            if phase < 516
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 517
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 518
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 520
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 94)
                                            if phase < 521
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 522
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 523
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 525
                                    else (
                                        (
                                            ((phase[:7] & 0) | 95)
                                            if phase < 526
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 527
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 528
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 530
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 96)
                                            if phase < 531
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 532
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 533
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 535
                                    else (
                                        (
                                            ((phase[:7] & 0) | 97)
                                            if phase < 536
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 537
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 538
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 540
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 98)
                                            if phase < 541
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 542
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 543
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 545
                                    else (
                                        (
                                            ((phase[:7] & 0) | 99)
                                            if phase < 546
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 547
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 548
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 550
                                                else ((phase[:7] & 0) | 100)
                                            )
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
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 552
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 553
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 555
                                            else ((phase[:7] & 0) | 101)
                                        )
                                    )
                                    if phase < 556
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 557
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 558
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 560
                                            else ((phase[:7] & 0) | 102)
                                        )
                                    )
                                )
                                if phase < 561
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 562
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 563
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 565
                                            else ((phase[:7] & 0) | 103)
                                        )
                                    )
                                    if phase < 566
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 567
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 568
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 570
                                            else ((phase[:7] & 0) | 104)
                                        )
                                    )
                                )
                            )
                            if phase < 571
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 572
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 573
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 575
                                            else ((phase[:7] & 0) | 105)
                                        )
                                    )
                                    if phase < 576
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 577
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 578
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 580
                                            else ((phase[:7] & 0) | 106)
                                        )
                                    )
                                )
                                if phase < 581
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 582
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 583
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 585
                                            else ((phase[:7] & 0) | 107)
                                        )
                                    )
                                    if phase < 586
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 587
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 588
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 590
                                            else (
                                                ((phase[:7] & 0) | 108)
                                                if phase < 591
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 592
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 593
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 595
                                        else (
                                            ((phase[:7] & 0) | 109)
                                            if phase < 596
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 597
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 598
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 600
                                        else (
                                            ((phase[:7] & 0) | 110)
                                            if phase < 601
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 602
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 603
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 605
                                        else (
                                            ((phase[:7] & 0) | 111)
                                            if phase < 606
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 607
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 608
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 610
                                        else (
                                            ((phase[:7] & 0) | 112)
                                            if phase < 611
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 612
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 613
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 615
                                        else (
                                            ((phase[:7] & 0) | 113)
                                            if phase < 616
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 617
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 618
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 620
                                        else (
                                            ((phase[:7] & 0) | 114)
                                            if phase < 621
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 622
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 623
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 625
                                        else (
                                            ((phase[:7] & 0) | 115)
                                            if phase < 626
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 627
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 628
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 630
                                        else (
                                            ((phase[:7] & 0) | 116)
                                            if phase < 631
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 632
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 633
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 635
                                            else ((phase[:7] & 0) | 117)
                                        )
                                        if phase < 636
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 637
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 638
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 640
                                            else ((phase[:7] & 0) | 118)
                                        )
                                        if phase < 641
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 642
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 643
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 645
                                            else ((phase[:7] & 0) | 119)
                                        )
                                        if phase < 646
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 647
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 648
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 650
                                            else ((phase[:7] & 0) | 120)
                                        )
                                        if phase < 651
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 652
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 653
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 655
                                            else ((phase[:7] & 0) | 121)
                                        )
                                        if phase < 656
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 657
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 658
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 660
                                            else ((phase[:7] & 0) | 122)
                                        )
                                        if phase < 661
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 662
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 663
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 665
                                            else ((phase[:7] & 0) | 123)
                                        )
                                        if phase < 666
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 667
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 668
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 670
                                            else ((phase[:7] & 0) | 124)
                                        )
                                        if phase < 671
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 672
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 673
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 675
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 125)
                                            if phase < 676
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 677
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 678
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 680
                                    else (
                                        (
                                            ((phase[:7] & 0) | 126)
                                            if phase < 681
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 682
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 683
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 685
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 127)
                                            if phase < 686
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 687
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 688
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 691
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 692
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 693
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 695
                                            else ((phase[:7] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 696
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 697
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 698
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 700
                                            else ((phase[:7] & 0) | 2)
                                        )
                                    )
                                    if phase < 701
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 702
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 703
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 705
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 707
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 708
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 710
                                        else (
                                            ((phase[:7] & 0) | 4)
                                            if phase < 711
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 712
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 713
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 715
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 716
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 717
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 718
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 720
                                        else (
                                            ((phase[:7] & 0) | 6)
                                            if phase < 721
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 722
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 723
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 725
                                        else (
                                            ((phase[:7] & 0) | 7)
                                            if phase < 726
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 727
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 728
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 730
                                        else (
                                            ((phase[:7] & 0) | 8)
                                            if phase < 731
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 732
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 733
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 735
                                        else (
                                            ((phase[:7] & 0) | 9)
                                            if phase < 736
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 737
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 738
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 740
                                        else (
                                            ((phase[:7] & 0) | 10)
                                            if phase < 741
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 742
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 743
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 745
                                        else (
                                            ((phase[:7] & 0) | 11)
                                            if phase < 746
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 747
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 748
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 750
                                        else (
                                            ((phase[:7] & 0) | 12)
                                            if phase < 751
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 752
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 753
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 755
                                        else (
                                            ((phase[:7] & 0) | 13)
                                            if phase < 756
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 757
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 758
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 760
                                            else ((phase[:7] & 0) | 14)
                                        )
                                        if phase < 761
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 762
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 763
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 765
                                            else ((phase[:7] & 0) | 15)
                                        )
                                        if phase < 766
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 767
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 768
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 770
                                            else ((phase[:7] & 0) | 16)
                                        )
                                        if phase < 771
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 772
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 773
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 775
                                            else ((phase[:7] & 0) | 17)
                                        )
                                        if phase < 776
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 777
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 778
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 780
                                            else ((phase[:7] & 0) | 18)
                                        )
                                        if phase < 781
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 782
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 783
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 785
                                            else ((phase[:7] & 0) | 19)
                                        )
                                        if phase < 786
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 787
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 788
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 790
                                            else ((phase[:7] & 0) | 20)
                                        )
                                        if phase < 791
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 792
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 793
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 795
                                            else ((phase[:7] & 0) | 21)
                                        )
                                        if phase < 796
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 797
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 798
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 800
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 22)
                                            if phase < 801
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 802
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 803
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 805
                                    else (
                                        (
                                            ((phase[:7] & 0) | 23)
                                            if phase < 806
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 807
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 808
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 810
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 24)
                                            if phase < 811
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 812
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 813
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 815
                                    else (
                                        (
                                            ((phase[:7] & 0) | 25)
                                            if phase < 816
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 817
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 818
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 820
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 26)
                                            if phase < 821
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 822
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 823
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 825
                                    else (
                                        (
                                            ((phase[:7] & 0) | 27)
                                            if phase < 826
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 827
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 828
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 830
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 28)
                                            if phase < 831
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 832
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 833
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 835
                                    else (
                                        (
                                            ((phase[:7] & 0) | 29)
                                            if phase < 836
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 837
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 838
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 840
                                                else ((phase[:7] & 0) | 30)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 841
                else (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 842
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 843
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 845
                                            else ((phase[:7] & 0) | 31)
                                        )
                                    )
                                    if phase < 846
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 847
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 848
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 850
                                            else ((phase[:7] & 0) | 32)
                                        )
                                    )
                                )
                                if phase < 851
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 852
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 853
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 855
                                            else ((phase[:7] & 0) | 33)
                                        )
                                    )
                                    if phase < 856
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 857
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 858
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 860
                                            else ((phase[:7] & 0) | 34)
                                        )
                                    )
                                )
                            )
                            if phase < 861
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 862
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 863
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 865
                                            else ((phase[:7] & 0) | 35)
                                        )
                                    )
                                    if phase < 866
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 867
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 868
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 870
                                            else ((phase[:7] & 0) | 36)
                                        )
                                    )
                                )
                                if phase < 871
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 872
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 873
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 875
                                            else ((phase[:7] & 0) | 37)
                                        )
                                    )
                                    if phase < 876
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 877
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 878
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 880
                                            else (
                                                ((phase[:7] & 0) | 38)
                                                if phase < 881
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 882
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 883
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 885
                                        else (
                                            ((phase[:7] & 0) | 39)
                                            if phase < 886
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 887
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 888
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 890
                                        else (
                                            ((phase[:7] & 0) | 40)
                                            if phase < 891
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 892
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 893
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 895
                                        else (
                                            ((phase[:7] & 0) | 41)
                                            if phase < 896
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 897
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 898
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 900
                                        else (
                                            ((phase[:7] & 0) | 42)
                                            if phase < 901
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 902
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 903
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 905
                                        else (
                                            ((phase[:7] & 0) | 43)
                                            if phase < 906
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 907
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 908
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 910
                                        else (
                                            ((phase[:7] & 0) | 44)
                                            if phase < 911
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 912
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 913
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 915
                                        else (
                                            ((phase[:7] & 0) | 45)
                                            if phase < 916
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 917
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 918
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 920
                                        else (
                                            ((phase[:7] & 0) | 46)
                                            if phase < 921
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 922
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 923
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 925
                                            else ((phase[:7] & 0) | 47)
                                        )
                                        if phase < 926
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 927
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 928
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 930
                                            else ((phase[:7] & 0) | 48)
                                        )
                                        if phase < 931
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 932
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 933
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 935
                                            else ((phase[:7] & 0) | 49)
                                        )
                                        if phase < 936
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 937
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 938
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 940
                                            else ((phase[:7] & 0) | 50)
                                        )
                                        if phase < 941
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 942
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 943
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 945
                                            else ((phase[:7] & 0) | 51)
                                        )
                                        if phase < 946
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 947
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 948
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 950
                                            else ((phase[:7] & 0) | 52)
                                        )
                                        if phase < 951
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 952
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 953
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 955
                                            else ((phase[:7] & 0) | 53)
                                        )
                                        if phase < 956
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 957
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 958
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 960
                                            else ((phase[:7] & 0) | 54)
                                        )
                                        if phase < 961
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 962
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 963
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 965
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 55)
                                            if phase < 966
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 967
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 968
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 970
                                    else (
                                        (
                                            ((phase[:7] & 0) | 56)
                                            if phase < 971
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 972
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 973
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 975
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 57)
                                            if phase < 976
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 977
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 978
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 980
                                    else (
                                        (
                                            ((phase[:7] & 0) | 58)
                                            if phase < 981
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 982
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 983
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 985
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 59)
                                            if phase < 986
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 987
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 988
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 990
                                    else (
                                        (
                                            ((phase[:7] & 0) | 60)
                                            if phase < 991
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 992
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 993
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 995
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 61)
                                            if phase < 996
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 997
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 998
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1000
                                    else (
                                        (
                                            ((phase[:7] & 0) | 62)
                                            if phase < 1001
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1002
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1003
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 1005
                                                else ((phase[:7] & 0) | 63)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 1006
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1007
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1008
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1010
                                            else ((phase[:7] & 0) | 64)
                                        )
                                    )
                                    if phase < 1011
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1012
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1013
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1015
                                            else ((phase[:7] & 0) | 65)
                                        )
                                    )
                                )
                                if phase < 1016
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1017
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1018
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1020
                                            else ((phase[:7] & 0) | 66)
                                        )
                                    )
                                    if phase < 1021
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1022
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1023
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1025
                                            else ((phase[:7] & 0) | 67)
                                        )
                                    )
                                )
                            )
                            if phase < 1026
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1027
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1028
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1030
                                            else ((phase[:7] & 0) | 68)
                                        )
                                    )
                                    if phase < 1031
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1032
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1033
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1035
                                            else ((phase[:7] & 0) | 69)
                                        )
                                    )
                                )
                                if phase < 1036
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1037
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1038
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1040
                                            else ((phase[:7] & 0) | 70)
                                        )
                                    )
                                    if phase < 1041
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1042
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1043
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1045
                                            else ((phase[:7] & 0) | 71)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1046
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1047
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1048
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1050
                                            else ((phase[:7] & 0) | 72)
                                        )
                                    )
                                    if phase < 1051
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1052
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1053
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1055
                                            else ((phase[:7] & 0) | 73)
                                        )
                                    )
                                )
                                if phase < 1056
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1057
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1058
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1060
                                            else ((phase[:7] & 0) | 74)
                                        )
                                    )
                                    if phase < 1061
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1062
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1063
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1065
                                            else ((phase[:7] & 0) | 75)
                                        )
                                    )
                                )
                            )
                            if phase < 1066
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1067
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1068
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1070
                                            else ((phase[:7] & 0) | 76)
                                        )
                                    )
                                    if phase < 1071
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1072
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1073
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1075
                                            else ((phase[:7] & 0) | 77)
                                        )
                                    )
                                )
                                if phase < 1076
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1077
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1078
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1080
                                            else ((phase[:7] & 0) | 78)
                                        )
                                    )
                                    if phase < 1081
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1082
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1083
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1085
                                            else (
                                                ((phase[:7] & 0) | 79)
                                                if phase < 1086
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1087
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1088
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1090
                                        else (
                                            ((phase[:7] & 0) | 80)
                                            if phase < 1091
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1092
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1093
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1095
                                        else (
                                            ((phase[:7] & 0) | 81)
                                            if phase < 1096
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 1097
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1098
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1100
                                        else (
                                            ((phase[:7] & 0) | 82)
                                            if phase < 1101
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1102
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1103
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1105
                                        else (
                                            ((phase[:7] & 0) | 83)
                                            if phase < 1106
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 1107
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1108
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1110
                                        else (
                                            ((phase[:7] & 0) | 84)
                                            if phase < 1111
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1112
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1113
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1115
                                        else (
                                            ((phase[:7] & 0) | 85)
                                            if phase < 1116
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 1117
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1118
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1120
                                        else (
                                            ((phase[:7] & 0) | 86)
                                            if phase < 1121
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1122
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1123
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1125
                                        else (
                                            ((phase[:7] & 0) | 87)
                                            if phase < 1126
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 1127
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1128
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1130
                                            else ((phase[:7] & 0) | 88)
                                        )
                                        if phase < 1131
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1132
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1133
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1135
                                            else ((phase[:7] & 0) | 89)
                                        )
                                        if phase < 1136
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1137
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 1138
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1140
                                            else ((phase[:7] & 0) | 90)
                                        )
                                        if phase < 1141
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1142
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1143
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1145
                                            else ((phase[:7] & 0) | 91)
                                        )
                                        if phase < 1146
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1147
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 1148
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1150
                                            else ((phase[:7] & 0) | 92)
                                        )
                                        if phase < 1151
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1152
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1153
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1155
                                            else ((phase[:7] & 0) | 93)
                                        )
                                        if phase < 1156
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1157
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 1158
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1160
                                            else ((phase[:7] & 0) | 94)
                                        )
                                        if phase < 1161
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1162
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1163
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1165
                                            else ((phase[:7] & 0) | 95)
                                        )
                                        if phase < 1166
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1167
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 1168
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1170
                else (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 96)
                                            if phase < 1171
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1172
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1173
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1175
                                    else (
                                        (
                                            ((phase[:7] & 0) | 97)
                                            if phase < 1176
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1177
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1178
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1180
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 98)
                                            if phase < 1181
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1182
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1183
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1185
                                    else (
                                        (
                                            ((phase[:7] & 0) | 99)
                                            if phase < 1186
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1187
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1188
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1190
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 100)
                                            if phase < 1191
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1192
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1193
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1195
                                    else (
                                        (
                                            ((phase[:7] & 0) | 101)
                                            if phase < 1196
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1197
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1198
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1200
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 102)
                                            if phase < 1201
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1202
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1203
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1205
                                    else (
                                        (
                                            ((phase[:7] & 0) | 103)
                                            if phase < 1206
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1207
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1208
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 1210
                                                else ((phase[:7] & 0) | 104)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1211
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1212
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1213
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1215
                                            else ((phase[:7] & 0) | 105)
                                        )
                                    )
                                    if phase < 1216
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1217
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1218
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1220
                                            else ((phase[:7] & 0) | 106)
                                        )
                                    )
                                )
                                if phase < 1221
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1222
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1223
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1225
                                            else ((phase[:7] & 0) | 107)
                                        )
                                    )
                                    if phase < 1226
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1227
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1228
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1230
                                            else ((phase[:7] & 0) | 108)
                                        )
                                    )
                                )
                            )
                            if phase < 1231
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1232
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1233
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1235
                                            else ((phase[:7] & 0) | 109)
                                        )
                                    )
                                    if phase < 1236
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1237
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1238
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1240
                                            else ((phase[:7] & 0) | 110)
                                        )
                                    )
                                )
                                if phase < 1241
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1242
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1243
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1245
                                            else ((phase[:7] & 0) | 111)
                                        )
                                    )
                                    if phase < 1246
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1247
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1248
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1250
                                            else (
                                                ((phase[:7] & 0) | 112)
                                                if phase < 1251
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1252
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1253
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1255
                                        else (
                                            ((phase[:7] & 0) | 113)
                                            if phase < 1256
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1257
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1258
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1260
                                        else (
                                            ((phase[:7] & 0) | 114)
                                            if phase < 1261
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 1262
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1263
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1265
                                        else (
                                            ((phase[:7] & 0) | 115)
                                            if phase < 1266
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1267
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1268
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1270
                                        else (
                                            ((phase[:7] & 0) | 116)
                                            if phase < 1271
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 1272
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1273
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1275
                                        else (
                                            ((phase[:7] & 0) | 117)
                                            if phase < 1276
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1277
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1278
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1280
                                        else (
                                            ((phase[:7] & 0) | 118)
                                            if phase < 1281
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 1282
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1283
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1285
                                        else (
                                            ((phase[:7] & 0) | 119)
                                            if phase < 1286
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1287
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1288
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1290
                                        else (
                                            ((phase[:7] & 0) | 120)
                                            if phase < 1291
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 1292
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1293
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1295
                                            else ((phase[:7] & 0) | 121)
                                        )
                                        if phase < 1296
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1297
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1298
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1300
                                            else ((phase[:7] & 0) | 122)
                                        )
                                        if phase < 1301
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1302
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 1303
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1305
                                            else ((phase[:7] & 0) | 123)
                                        )
                                        if phase < 1306
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1307
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1308
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1310
                                            else ((phase[:7] & 0) | 124)
                                        )
                                        if phase < 1311
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1312
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 1313
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1315
                                            else ((phase[:7] & 0) | 125)
                                        )
                                        if phase < 1316
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1317
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1318
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1320
                                            else ((phase[:7] & 0) | 126)
                                        )
                                        if phase < 1321
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1322
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 1323
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1325
                                            else ((phase[:7] & 0) | 127)
                                        )
                                        if phase < 1326
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1327
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1328
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1330
                                            else ((phase[:7] & 0) | 50)
                                        )
                                        if phase < 1331
                                        else (
                                            ((phase[:7] & 0) | 60)
                                            if phase < 1332
                                            else (
                                                ((phase[:7] & 0) | 70)
                                                if phase < 1333
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
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
            (((phase[:1] & 0) | 0) if phase < 1 else ((phase[:1] & 0) | 1))
            if phase < 13
            else (((phase[:1] & 0) | 0) if phase < 18 else ((phase[:1] & 0) | 1))
        )
        if phase < 38
        else (
            (((phase[:1] & 0) | 0) if phase < 41 else ((phase[:1] & 0) | 1))
            if phase < 1330
            else (((phase[:1] & 0) | 0) if phase < 1333 else ((phase[:1] & 0) | 1))
        )
    )
    payload = Entry(index=index, value=value)
    dut = TableRule(valid, payload, take)
    expected_ready = (
        ((phase[:1] & 0) | 1)
        if phase < 16
        else (((phase[:1] & 0) | 0) if phase < 18 else ((phase[:1] & 0) | 1))
    )
    expected_valid = (
        (
            (
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
                                    if phase < 7
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 15
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 33
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 40
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 44
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 52
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 55
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 57
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 60
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 62
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 65
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 67
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 70
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 72
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 75
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 77
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 80
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 82
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 85
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 87
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 90
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 92
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 95
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 97
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 100
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 102
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 105
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 107
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 110
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 112
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 115
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 117
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 120
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 122
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 125
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 127
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 130
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 132
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 135
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 137
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 140
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 142
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 145
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 147
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 150
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 152
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 155
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 157
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 160
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 162
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 165
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 167
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 170
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 172
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 175
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 177
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 180
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 182
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 185
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 187
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 190
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 192
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 195
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
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
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 200
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 202
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 205
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 207
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 210
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 212
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 215
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 217
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 220
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 222
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 225
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 227
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 230
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 232
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 235
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 237
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 240
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 242
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 245
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 247
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 250
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 252
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 255
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 257
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 260
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 262
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 265
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 267
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 270
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 272
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 275
                                        else ((phase[:1] & 0) | 0)
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
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 280
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 282
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 285
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 287
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 290
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 292
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 295
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 297
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 300
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 302
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 305
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 307
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 310
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 312
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 315
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 317
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 320
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 322
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 325
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 327
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 330
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 332
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 335
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 337
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 340
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 342
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 345
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 347
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 350
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 352
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 355
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 357
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 360
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 362
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 365
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 367
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 370
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 372
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 375
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 377
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 380
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 382
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 385
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 387
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 390
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 392
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 395
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 397
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 400
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 402
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 405
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 407
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 410
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 412
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 415
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 417
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 420
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 422
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 425
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 427
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 430
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 432
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 435
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 437
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 440
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 442
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 445
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 447
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 450
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 452
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 455
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 457
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 460
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 462
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 465
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 467
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 470
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 472
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 475
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 477
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 480
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 482
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 485
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 487
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 490
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 492
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 495
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 497
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 500
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 502
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 505
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 507
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 510
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 512
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 515
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 517
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 520
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 522
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 525
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 527
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 530
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 532
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 535
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 537
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 540
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 542
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 545
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 547
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 550
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 552
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 555
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 557
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 560
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 562
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 565
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 567
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 570
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 572
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 575
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 577
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 580
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 582
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 585
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 587
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 590
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 592
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 595
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 597
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 600
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 602
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 605
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 607
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 610
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 612
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 615
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 617
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 620
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 622
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 625
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 627
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 630
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 632
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 635
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 637
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 640
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 642
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 645
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 647
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 650
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 652
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 655
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 657
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 660
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 662
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 665
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 667
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 670
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 672
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 675
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 677
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 680
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 682
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 685
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 687
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 690
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 692
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 695
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 697
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 700
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 702
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 705
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 707
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 710
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 712
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 715
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 717
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 720
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 722
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 725
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 727
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 730
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 732
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 735
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 737
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 740
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 742
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 745
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 747
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 750
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 752
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 755
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 757
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 760
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 762
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 765
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 767
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 770
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 772
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 775
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 777
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 780
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 782
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 785
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 787
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 790
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 792
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 795
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 797
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 800
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 802
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 805
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 807
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 810
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 812
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 815
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 817
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 820
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 822
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 825
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 827
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 830
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 832
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 835
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 837
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 840
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 842
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 845
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 847
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 850
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 852
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 855
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 857
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 860
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 862
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 865
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 867
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 870
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 872
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 875
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 877
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 880
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 882
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 885
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 887
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 890
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 892
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 895
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 897
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 900
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 902
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 905
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 907
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 910
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 912
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 915
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 917
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 920
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 922
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 925
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 927
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 930
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 932
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 935
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 937
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 940
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 942
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 945
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 947
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 950
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 952
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 955
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 957
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 960
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 962
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 965
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 967
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 970
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 972
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 975
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 977
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 980
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 982
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 985
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 987
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 990
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 992
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 995
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 997
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1000
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1002
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1005
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1007
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 1010
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1012
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1015
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1017
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1020
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1022
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1025
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1027
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1030
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1032
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1035
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1037
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1040
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1042
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1045
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1047
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 1050
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1052
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1055
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1057
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1060
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1062
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1065
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1067
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1070
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1072
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1075
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1077
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1080
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1082
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1085
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1087
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1090
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1092
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1095
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1097
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1100
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1102
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1105
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1107
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1110
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1112
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1115
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1117
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1120
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1122
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1125
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1127
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 1130
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1132
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1135
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1137
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1140
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1142
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1145
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1147
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1150
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1152
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1155
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1157
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1160
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1162
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1165
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1167
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1170
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1172
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1175
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1177
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1180
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1182
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1185
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1187
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1190
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1192
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1195
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1197
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1200
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1202
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1205
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1207
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1210
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 1212
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1215
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1217
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1220
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1222
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1225
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1227
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1230
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1232
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1235
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1237
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1240
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1242
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1245
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1247
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1250
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1252
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1255
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1257
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1260
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1262
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1265
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1267
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1270
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1272
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1275
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1277
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1280
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1282
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1285
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1287
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1290
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1292
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 1295
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1297
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1300
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1302
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1305
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1307
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1310
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1312
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1315
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1317
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1320
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1322
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1325
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1327
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1330
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1332
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1337
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    expected_data_index = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 19
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 20
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
                                        if phase < 24
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 25
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 26
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 27
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 28
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
                                        if phase < 42
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 43
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 54
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 55
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 59
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 60
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 64
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 65
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 69
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 70
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 74
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 75
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 79
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 80
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 84
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 85
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 89
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 90
                                            else ((phase[:1] & 0) | 0)
                                        )
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
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 95
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 99
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 100
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 104
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 105
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 109
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 110
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 114
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 115
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 119
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 120
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 124
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 125
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 129
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 130
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 134
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 135
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 139
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 140
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 144
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 145
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 149
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 150
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 154
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 155
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 159
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 160
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 164
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 165
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 169
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 170
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 174
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 175
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 179
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 180
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 184
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 185
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 189
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 190
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 194
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 195
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 199
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 200
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 204
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 205
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 209
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 210
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 214
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 215
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 219
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 220
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 224
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 225
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 229
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 230
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 234
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 235
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 239
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 240
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 244
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 245
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 249
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 250
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 254
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 255
                                            else ((phase[:1] & 0) | 0)
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
                                        ((phase[:1] & 0) | 1)
                                        if phase < 260
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 264
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 265
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 269
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 270
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 274
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 275
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 279
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 280
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 284
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 285
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 289
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 290
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 294
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 295
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 299
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 300
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 304
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 305
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 309
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 310
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 314
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 315
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 319
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 320
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 324
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 325
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 329
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 330
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 334
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 335
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 339
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 340
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 344
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 345
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 349
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 350
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 354
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 355
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 359
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 360
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 364
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 365
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 369
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 370
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 374
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 375
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 379
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 380
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 384
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 385
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 389
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 390
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 394
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 395
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 399
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 400
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 404
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 405
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 409
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 410
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 414
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 415
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 419
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 420
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 424
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 425
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 429
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 430
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 434
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 435
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 439
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 440
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 444
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 445
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 449
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 450
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 454
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 455
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 459
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 460
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 464
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 465
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 469
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 470
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 474
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 475
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 479
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 480
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 484
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 485
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 489
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 490
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 494
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 495
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 499
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 500
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 504
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 505
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 509
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 510
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 514
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 515
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 519
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 520
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 524
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 525
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 529
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 530
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 534
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 535
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 539
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 540
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 544
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 545
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 549
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 550
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 554
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 555
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 559
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 560
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 564
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 565
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 569
                                        else ((phase[:1] & 0) | 1)
                                    )
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
                                        if phase < 579
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 580
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 584
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 585
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 589
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 590
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 594
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 595
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 599
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 600
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 604
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 605
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 609
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 610
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 614
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 615
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 619
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 620
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 624
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 625
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 629
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 630
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 634
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 635
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 639
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 640
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 644
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 645
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 649
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 650
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 654
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 655
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 659
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 660
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 664
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 665
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 669
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 670
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 674
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 675
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 679
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 680
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 684
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 685
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 689
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 690
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 692
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 694
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 697
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 699
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 702
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 704
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 707
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 709
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 712
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 714
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 717
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 719
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 722
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 724
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 727
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 729
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 732
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 734
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 737
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 739
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 742
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 744
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 747
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 749
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 752
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 754
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 757
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 759
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 762
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 764
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 767
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 769
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 772
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 774
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 777
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 779
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 782
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 784
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 787
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 789
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 792
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 794
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 797
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 799
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 802
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 804
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 807
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 809
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 812
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 814
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 817
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 819
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 822
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 824
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 827
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 829
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 832
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 834
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 837
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 839
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 842
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 844
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 847
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 849
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 852
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 854
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 857
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 859
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 862
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 864
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 867
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 869
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 872
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 874
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 877
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 879
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 882
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 884
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 887
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 889
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 892
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 894
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 897
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 899
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 902
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 904
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 907
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 909
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 912
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 914
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 917
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 919
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 922
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 924
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 927
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 929
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 932
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 934
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 937
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 939
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 942
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 944
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 947
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 949
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 952
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 954
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 957
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 959
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 962
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 964
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 967
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 969
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 972
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 974
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 977
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 979
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 982
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 984
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 987
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 989
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 992
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 994
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 997
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 999
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1002
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 1004
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1007
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1009
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1012
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1014
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1017
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1019
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1022
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1024
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1027
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1029
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1032
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1034
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1037
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1039
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1042
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 1044
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1047
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1049
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1052
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1054
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1057
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1059
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1062
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1064
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1067
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1069
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1072
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1074
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1077
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1079
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1082
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1084
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1087
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1089
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1092
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1094
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1097
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1099
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1102
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1104
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1107
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1109
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1112
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1114
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1117
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1119
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1122
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1124
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 1127
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1129
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1132
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1134
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1137
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1139
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1142
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1144
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1147
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1149
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1152
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1154
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1157
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1159
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1162
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1164
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1167
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1169
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1172
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1174
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1177
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1179
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1182
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1184
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1187
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1189
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1192
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1194
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1197
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1199
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1202
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1204
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1207
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                        )
                        if phase < 1209
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1212
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1214
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1217
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1219
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1222
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1224
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1227
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1229
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1232
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1234
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1237
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1239
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1242
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1244
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1247
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1249
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1252
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1254
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1257
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1259
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1262
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1264
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1267
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1269
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1272
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1274
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1277
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1279
                                        else ((phase[:1] & 0) | 0)
                                    )
                                )
                                if phase < 1282
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1284
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1287
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1289
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1292
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1294
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1297
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1299
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1302
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1304
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1307
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1309
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1312
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1314
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1317
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1319
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1322
                                        else ((phase[:1] & 0) | 1)
                                    )
                                )
                                if phase < 1324
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1327
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1329
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1334
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1335
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    expected_data_value = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 4
                                            else ((phase[:7] & 0) | 10)
                                        )
                                        if phase < 5
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 6
                                            else ((phase[:7] & 0) | 20)
                                        )
                                    )
                                    if phase < 7
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 15
                                            else ((phase[:7] & 0) | 40)
                                        )
                                        if phase < 19
                                        else (
                                            ((phase[:7] & 0) | 30)
                                            if phase < 20
                                            else ((phase[:7] & 0) | 50)
                                        )
                                    )
                                )
                                if phase < 21
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 70)
                                            if phase < 22
                                            else ((phase[:7] & 0) | 60)
                                        )
                                        if phase < 23
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 24
                                            else ((phase[:7] & 0) | 11)
                                        )
                                    )
                                    if phase < 25
                                    else (
                                        (
                                            ((phase[:7] & 0) | 22)
                                            if phase < 26
                                            else ((phase[:7] & 0) | 33)
                                        )
                                        if phase < 27
                                        else (
                                            ((phase[:7] & 0) | 44)
                                            if phase < 28
                                            else ((phase[:7] & 0) | 55)
                                        )
                                    )
                                )
                            )
                            if phase < 29
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 66)
                                            if phase < 30
                                            else ((phase[:7] & 0) | 77)
                                        )
                                        if phase < 31
                                        else (
                                            ((phase[:7] & 0) | 88)
                                            if phase < 32
                                            else ((phase[:7] & 0) | 99)
                                        )
                                    )
                                    if phase < 33
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 40
                                            else ((phase[:7] & 0) | 110)
                                        )
                                        if phase < 42
                                        else (
                                            ((phase[:7] & 0) | 121)
                                            if phase < 43
                                            else ((phase[:7] & 0) | 50)
                                        )
                                    )
                                )
                                if phase < 44
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 52
                                            else ((phase[:7] & 0) | 70)
                                        )
                                        if phase < 53
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 54
                                            else ((phase[:7] & 0) | 60)
                                        )
                                    )
                                    if phase < 55
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 57
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 58
                                        else (
                                            ((phase[:7] & 0) | 1)
                                            if phase < 59
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 60
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 62
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 63
                                        else (
                                            ((phase[:7] & 0) | 2)
                                            if phase < 64
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 65
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 67
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 69
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 70
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 72
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 73
                                            else ((phase[:7] & 0) | 4)
                                        )
                                        if phase < 74
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 75
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 77
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 78
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 80
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 82
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 83
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 6)
                                            if phase < 84
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 85
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 87
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 88
                                    else (
                                        (
                                            ((phase[:7] & 0) | 7)
                                            if phase < 89
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 90
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 92
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 93
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 8)
                                            if phase < 94
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 95
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 97
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 98
                                    else (
                                        (
                                            ((phase[:7] & 0) | 9)
                                            if phase < 99
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 100
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 102
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 103
                                                else ((phase[:7] & 0) | 10)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 104
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 105
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 107
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 108
                                            else ((phase[:7] & 0) | 11)
                                        )
                                    )
                                    if phase < 109
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 110
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 112
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 113
                                            else ((phase[:7] & 0) | 12)
                                        )
                                    )
                                )
                                if phase < 114
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 115
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 117
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 118
                                            else ((phase[:7] & 0) | 13)
                                        )
                                    )
                                    if phase < 119
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 120
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 122
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 123
                                            else ((phase[:7] & 0) | 14)
                                        )
                                    )
                                )
                            )
                            if phase < 124
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 125
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 127
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 128
                                            else ((phase[:7] & 0) | 15)
                                        )
                                    )
                                    if phase < 129
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 130
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 132
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 133
                                            else ((phase[:7] & 0) | 16)
                                        )
                                    )
                                )
                                if phase < 134
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 135
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 137
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 138
                                            else ((phase[:7] & 0) | 17)
                                        )
                                    )
                                    if phase < 139
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 140
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 142
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 143
                                            else (
                                                ((phase[:7] & 0) | 18)
                                                if phase < 144
                                                else ((phase[:7] & 0) | 5)
                                            )
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
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 147
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 148
                                        else (
                                            ((phase[:7] & 0) | 19)
                                            if phase < 149
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 150
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 152
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 153
                                        else (
                                            ((phase[:7] & 0) | 20)
                                            if phase < 154
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 155
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 157
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 158
                                        else (
                                            ((phase[:7] & 0) | 21)
                                            if phase < 159
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 160
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 162
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 163
                                        else (
                                            ((phase[:7] & 0) | 22)
                                            if phase < 164
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 165
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 167
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 168
                                        else (
                                            ((phase[:7] & 0) | 23)
                                            if phase < 169
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 170
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 172
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 173
                                        else (
                                            ((phase[:7] & 0) | 24)
                                            if phase < 174
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 175
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 177
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 178
                                        else (
                                            ((phase[:7] & 0) | 25)
                                            if phase < 179
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 180
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 182
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 183
                                        else (
                                            ((phase[:7] & 0) | 26)
                                            if phase < 184
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 185
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 187
                else (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 188
                                            else ((phase[:7] & 0) | 27)
                                        )
                                        if phase < 189
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 190
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 192
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 193
                                            else ((phase[:7] & 0) | 28)
                                        )
                                        if phase < 194
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 195
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 197
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 198
                                            else ((phase[:7] & 0) | 29)
                                        )
                                        if phase < 199
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 200
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 202
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 203
                                            else ((phase[:7] & 0) | 30)
                                        )
                                        if phase < 204
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 205
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 207
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 208
                                            else ((phase[:7] & 0) | 31)
                                        )
                                        if phase < 209
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 210
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 212
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 213
                                            else ((phase[:7] & 0) | 32)
                                        )
                                        if phase < 214
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 215
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 217
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 218
                                            else ((phase[:7] & 0) | 33)
                                        )
                                        if phase < 219
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 220
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 222
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 223
                                            else ((phase[:7] & 0) | 34)
                                        )
                                        if phase < 224
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 225
                                            else ((phase[:7] & 0) | 0)
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
                                            ((phase[:7] & 0) | 3)
                                            if phase < 228
                                            else ((phase[:7] & 0) | 35)
                                        )
                                        if phase < 229
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 230
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 232
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 233
                                            else ((phase[:7] & 0) | 36)
                                        )
                                        if phase < 234
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 235
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 237
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 238
                                            else ((phase[:7] & 0) | 37)
                                        )
                                        if phase < 239
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 240
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 242
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 243
                                            else ((phase[:7] & 0) | 38)
                                        )
                                        if phase < 244
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 245
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 247
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 248
                                            else ((phase[:7] & 0) | 39)
                                        )
                                        if phase < 249
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 250
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 252
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 253
                                            else ((phase[:7] & 0) | 40)
                                        )
                                        if phase < 254
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 255
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 257
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 258
                                            else ((phase[:7] & 0) | 41)
                                        )
                                        if phase < 259
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 260
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 262
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 263
                                            else ((phase[:7] & 0) | 42)
                                        )
                                        if phase < 264
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 265
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 267
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 268
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 43)
                                            if phase < 269
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 270
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 272
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 273
                                    else (
                                        (
                                            ((phase[:7] & 0) | 44)
                                            if phase < 274
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 275
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 277
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 278
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 45)
                                            if phase < 279
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 280
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 282
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 283
                                    else (
                                        (
                                            ((phase[:7] & 0) | 46)
                                            if phase < 284
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 285
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 287
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 288
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 47)
                                            if phase < 289
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 290
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 292
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 293
                                    else (
                                        (
                                            ((phase[:7] & 0) | 48)
                                            if phase < 294
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 295
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 297
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 298
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 49)
                                            if phase < 299
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 300
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 302
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 303
                                    else (
                                        (
                                            ((phase[:7] & 0) | 50)
                                            if phase < 304
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 305
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 307
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 308
                                                else ((phase[:7] & 0) | 51)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 309
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 310
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 312
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 313
                                            else ((phase[:7] & 0) | 52)
                                        )
                                    )
                                    if phase < 314
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 315
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 317
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 318
                                            else ((phase[:7] & 0) | 53)
                                        )
                                    )
                                )
                                if phase < 319
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 320
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 322
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 323
                                            else ((phase[:7] & 0) | 54)
                                        )
                                    )
                                    if phase < 324
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 325
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 327
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 328
                                            else ((phase[:7] & 0) | 55)
                                        )
                                    )
                                )
                            )
                            if phase < 329
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 330
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 332
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 333
                                            else ((phase[:7] & 0) | 56)
                                        )
                                    )
                                    if phase < 334
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 335
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 337
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 338
                                            else ((phase[:7] & 0) | 57)
                                        )
                                    )
                                )
                                if phase < 339
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 340
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 342
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 343
                                            else ((phase[:7] & 0) | 58)
                                        )
                                    )
                                    if phase < 344
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 345
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 347
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 348
                                            else (
                                                ((phase[:7] & 0) | 59)
                                                if phase < 349
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 350
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 352
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 353
                                        else (
                                            ((phase[:7] & 0) | 60)
                                            if phase < 354
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 355
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 357
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 358
                                        else (
                                            ((phase[:7] & 0) | 61)
                                            if phase < 359
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 360
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 362
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 363
                                        else (
                                            ((phase[:7] & 0) | 62)
                                            if phase < 364
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 365
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 367
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 368
                                        else (
                                            ((phase[:7] & 0) | 63)
                                            if phase < 369
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 370
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 372
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 373
                                        else (
                                            ((phase[:7] & 0) | 64)
                                            if phase < 374
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 375
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 377
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 378
                                        else (
                                            ((phase[:7] & 0) | 65)
                                            if phase < 379
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 380
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 382
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 383
                                        else (
                                            ((phase[:7] & 0) | 66)
                                            if phase < 384
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 385
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 387
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 388
                                        else (
                                            ((phase[:7] & 0) | 67)
                                            if phase < 389
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 390
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 392
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 393
                                        else (
                                            ((phase[:7] & 0) | 68)
                                            if phase < 394
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 395
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 397
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 398
                                        else (
                                            ((phase[:7] & 0) | 69)
                                            if phase < 399
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 400
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 402
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 403
                                        else (
                                            ((phase[:7] & 0) | 70)
                                            if phase < 404
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 405
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 407
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 408
                                        else (
                                            ((phase[:7] & 0) | 71)
                                            if phase < 409
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 410
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 412
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 413
                                        else (
                                            ((phase[:7] & 0) | 72)
                                            if phase < 414
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 415
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 417
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 418
                                        else (
                                            ((phase[:7] & 0) | 73)
                                            if phase < 419
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 420
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 422
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 423
                                        else (
                                            ((phase[:7] & 0) | 74)
                                            if phase < 424
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 425
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 427
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 428
                                        else (
                                            ((phase[:7] & 0) | 75)
                                            if phase < 429
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 430
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 432
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 433
                                            else ((phase[:7] & 0) | 76)
                                        )
                                        if phase < 434
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 435
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 437
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 438
                                            else ((phase[:7] & 0) | 77)
                                        )
                                        if phase < 439
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 440
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 442
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 443
                                            else ((phase[:7] & 0) | 78)
                                        )
                                        if phase < 444
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 445
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 447
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 448
                                            else ((phase[:7] & 0) | 79)
                                        )
                                        if phase < 449
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 450
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 452
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 453
                                            else ((phase[:7] & 0) | 80)
                                        )
                                        if phase < 454
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 455
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 457
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 458
                                            else ((phase[:7] & 0) | 81)
                                        )
                                        if phase < 459
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 460
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 462
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 463
                                            else ((phase[:7] & 0) | 82)
                                        )
                                        if phase < 464
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 465
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 467
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 468
                                            else ((phase[:7] & 0) | 83)
                                        )
                                        if phase < 469
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 470
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 472
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 473
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 84)
                                            if phase < 474
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 475
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 477
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 478
                                    else (
                                        (
                                            ((phase[:7] & 0) | 85)
                                            if phase < 479
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 480
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 482
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 483
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 86)
                                            if phase < 484
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 485
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 487
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 488
                                    else (
                                        (
                                            ((phase[:7] & 0) | 87)
                                            if phase < 489
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 490
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 492
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 493
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 88)
                                            if phase < 494
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 495
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 497
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 498
                                    else (
                                        (
                                            ((phase[:7] & 0) | 89)
                                            if phase < 499
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 500
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 502
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 503
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 90)
                                            if phase < 504
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 505
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 507
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 508
                                    else (
                                        (
                                            ((phase[:7] & 0) | 91)
                                            if phase < 509
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 510
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 512
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 513
                                                else ((phase[:7] & 0) | 92)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 514
                else (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 515
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 517
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 518
                                            else ((phase[:7] & 0) | 93)
                                        )
                                    )
                                    if phase < 519
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 520
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 522
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 523
                                            else ((phase[:7] & 0) | 94)
                                        )
                                    )
                                )
                                if phase < 524
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 525
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 527
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 528
                                            else ((phase[:7] & 0) | 95)
                                        )
                                    )
                                    if phase < 529
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 530
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 532
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 533
                                            else ((phase[:7] & 0) | 96)
                                        )
                                    )
                                )
                            )
                            if phase < 534
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 535
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 537
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 538
                                            else ((phase[:7] & 0) | 97)
                                        )
                                    )
                                    if phase < 539
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 540
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 542
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 543
                                            else ((phase[:7] & 0) | 98)
                                        )
                                    )
                                )
                                if phase < 544
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 545
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 547
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 548
                                            else ((phase[:7] & 0) | 99)
                                        )
                                    )
                                    if phase < 549
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 550
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 552
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 553
                                            else (
                                                ((phase[:7] & 0) | 100)
                                                if phase < 554
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 555
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 557
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 558
                                        else (
                                            ((phase[:7] & 0) | 101)
                                            if phase < 559
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 560
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 562
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 563
                                        else (
                                            ((phase[:7] & 0) | 102)
                                            if phase < 564
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 565
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 567
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 568
                                        else (
                                            ((phase[:7] & 0) | 103)
                                            if phase < 569
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 570
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 572
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 573
                                        else (
                                            ((phase[:7] & 0) | 104)
                                            if phase < 574
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 575
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 577
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 578
                                        else (
                                            ((phase[:7] & 0) | 105)
                                            if phase < 579
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 580
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 582
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 583
                                        else (
                                            ((phase[:7] & 0) | 106)
                                            if phase < 584
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 585
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 587
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 588
                                        else (
                                            ((phase[:7] & 0) | 107)
                                            if phase < 589
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 590
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 592
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 593
                                        else (
                                            ((phase[:7] & 0) | 108)
                                            if phase < 594
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 595
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 597
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 598
                                            else ((phase[:7] & 0) | 109)
                                        )
                                        if phase < 599
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 600
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 602
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 603
                                            else ((phase[:7] & 0) | 110)
                                        )
                                        if phase < 604
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 605
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 607
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 608
                                            else ((phase[:7] & 0) | 111)
                                        )
                                        if phase < 609
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 610
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 612
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 613
                                            else ((phase[:7] & 0) | 112)
                                        )
                                        if phase < 614
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 615
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 617
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 618
                                            else ((phase[:7] & 0) | 113)
                                        )
                                        if phase < 619
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 620
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 622
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 623
                                            else ((phase[:7] & 0) | 114)
                                        )
                                        if phase < 624
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 625
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 627
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 628
                                            else ((phase[:7] & 0) | 115)
                                        )
                                        if phase < 629
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 630
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 632
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 633
                                            else ((phase[:7] & 0) | 116)
                                        )
                                        if phase < 634
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 635
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 637
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 638
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 117)
                                            if phase < 639
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 640
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 642
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 643
                                    else (
                                        (
                                            ((phase[:7] & 0) | 118)
                                            if phase < 644
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 645
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 647
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 648
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 119)
                                            if phase < 649
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 650
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 652
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 653
                                    else (
                                        (
                                            ((phase[:7] & 0) | 120)
                                            if phase < 654
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 655
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 657
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 658
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 121)
                                            if phase < 659
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 660
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 662
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 663
                                    else (
                                        (
                                            ((phase[:7] & 0) | 122)
                                            if phase < 664
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 665
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 667
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 668
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 123)
                                            if phase < 669
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 670
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 672
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 673
                                    else (
                                        (
                                            ((phase[:7] & 0) | 124)
                                            if phase < 674
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 675
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 677
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 678
                                                else ((phase[:7] & 0) | 125)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 679
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 680
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 682
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 683
                                            else ((phase[:7] & 0) | 126)
                                        )
                                    )
                                    if phase < 684
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 685
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 687
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 688
                                            else ((phase[:7] & 0) | 127)
                                        )
                                    )
                                )
                                if phase < 689
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 690
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 692
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 693
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 694
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 695
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 697
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 698
                                            else ((phase[:7] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 699
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 700
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 702
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 703
                                            else ((phase[:7] & 0) | 2)
                                        )
                                    )
                                    if phase < 704
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 705
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 707
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 709
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 710
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 712
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 713
                                        else (
                                            ((phase[:7] & 0) | 4)
                                            if phase < 714
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 715
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 717
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 718
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 720
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 722
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 723
                                            else ((phase[:7] & 0) | 6)
                                        )
                                        if phase < 724
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 725
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 727
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 728
                                            else ((phase[:7] & 0) | 7)
                                        )
                                        if phase < 729
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 730
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 732
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 733
                                            else ((phase[:7] & 0) | 8)
                                        )
                                        if phase < 734
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 735
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 737
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 738
                                            else ((phase[:7] & 0) | 9)
                                        )
                                        if phase < 739
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 740
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 742
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 743
                                            else ((phase[:7] & 0) | 10)
                                        )
                                        if phase < 744
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 745
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 747
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 748
                                            else ((phase[:7] & 0) | 11)
                                        )
                                        if phase < 749
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 750
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 752
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 753
                                            else ((phase[:7] & 0) | 12)
                                        )
                                        if phase < 754
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 755
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 757
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 758
                                            else ((phase[:7] & 0) | 13)
                                        )
                                        if phase < 759
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 760
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 762
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 763
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 14)
                                            if phase < 764
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 765
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 767
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 768
                                    else (
                                        (
                                            ((phase[:7] & 0) | 15)
                                            if phase < 769
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 770
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 772
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 773
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 16)
                                            if phase < 774
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 775
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 777
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 778
                                    else (
                                        (
                                            ((phase[:7] & 0) | 17)
                                            if phase < 779
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 780
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 782
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 783
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 18)
                                            if phase < 784
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 785
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 787
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 788
                                    else (
                                        (
                                            ((phase[:7] & 0) | 19)
                                            if phase < 789
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 790
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 792
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 793
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 20)
                                            if phase < 794
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 795
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 797
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 798
                                    else (
                                        (
                                            ((phase[:7] & 0) | 21)
                                            if phase < 799
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 800
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 802
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 803
                                                else ((phase[:7] & 0) | 22)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 804
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 805
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 807
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 808
                                            else ((phase[:7] & 0) | 23)
                                        )
                                    )
                                    if phase < 809
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 810
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 812
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 813
                                            else ((phase[:7] & 0) | 24)
                                        )
                                    )
                                )
                                if phase < 814
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 815
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 817
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 818
                                            else ((phase[:7] & 0) | 25)
                                        )
                                    )
                                    if phase < 819
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 820
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 822
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 823
                                            else ((phase[:7] & 0) | 26)
                                        )
                                    )
                                )
                            )
                            if phase < 824
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 825
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 827
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 828
                                            else ((phase[:7] & 0) | 27)
                                        )
                                    )
                                    if phase < 829
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 830
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 832
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 833
                                            else ((phase[:7] & 0) | 28)
                                        )
                                    )
                                )
                                if phase < 834
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 835
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 837
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 838
                                            else ((phase[:7] & 0) | 29)
                                        )
                                    )
                                    if phase < 839
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 840
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 842
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 843
                                            else (
                                                ((phase[:7] & 0) | 30)
                                                if phase < 844
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 845
                else (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 847
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 848
                                        else (
                                            ((phase[:7] & 0) | 31)
                                            if phase < 849
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 850
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 852
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 853
                                        else (
                                            ((phase[:7] & 0) | 32)
                                            if phase < 854
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 855
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 857
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 858
                                        else (
                                            ((phase[:7] & 0) | 33)
                                            if phase < 859
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 860
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 862
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 863
                                        else (
                                            ((phase[:7] & 0) | 34)
                                            if phase < 864
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 865
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 867
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 868
                                        else (
                                            ((phase[:7] & 0) | 35)
                                            if phase < 869
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 870
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 872
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 873
                                        else (
                                            ((phase[:7] & 0) | 36)
                                            if phase < 874
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 875
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 877
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 878
                                        else (
                                            ((phase[:7] & 0) | 37)
                                            if phase < 879
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 880
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 882
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 883
                                        else (
                                            ((phase[:7] & 0) | 38)
                                            if phase < 884
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 885
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 887
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 888
                                        else (
                                            ((phase[:7] & 0) | 39)
                                            if phase < 889
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 890
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 892
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 893
                                        else (
                                            ((phase[:7] & 0) | 40)
                                            if phase < 894
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 895
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 897
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 898
                                        else (
                                            ((phase[:7] & 0) | 41)
                                            if phase < 899
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 900
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 902
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 903
                                        else (
                                            ((phase[:7] & 0) | 42)
                                            if phase < 904
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 905
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 907
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 908
                                        else (
                                            ((phase[:7] & 0) | 43)
                                            if phase < 909
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 910
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 912
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 913
                                        else (
                                            ((phase[:7] & 0) | 44)
                                            if phase < 914
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 915
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 917
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 918
                                        else (
                                            ((phase[:7] & 0) | 45)
                                            if phase < 919
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 920
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 922
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 923
                                        else (
                                            ((phase[:7] & 0) | 46)
                                            if phase < 924
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 925
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 927
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 928
                                            else ((phase[:7] & 0) | 47)
                                        )
                                        if phase < 929
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 930
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 932
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 933
                                            else ((phase[:7] & 0) | 48)
                                        )
                                        if phase < 934
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 935
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 937
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 938
                                            else ((phase[:7] & 0) | 49)
                                        )
                                        if phase < 939
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 940
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 942
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 943
                                            else ((phase[:7] & 0) | 50)
                                        )
                                        if phase < 944
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 945
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 947
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 948
                                            else ((phase[:7] & 0) | 51)
                                        )
                                        if phase < 949
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 950
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 952
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 953
                                            else ((phase[:7] & 0) | 52)
                                        )
                                        if phase < 954
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 955
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 957
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 958
                                            else ((phase[:7] & 0) | 53)
                                        )
                                        if phase < 959
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 960
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 962
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 963
                                            else ((phase[:7] & 0) | 54)
                                        )
                                        if phase < 964
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 965
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 967
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 968
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 55)
                                            if phase < 969
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 970
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 972
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 973
                                    else (
                                        (
                                            ((phase[:7] & 0) | 56)
                                            if phase < 974
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 975
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 977
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 978
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 57)
                                            if phase < 979
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 980
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 982
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 983
                                    else (
                                        (
                                            ((phase[:7] & 0) | 58)
                                            if phase < 984
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 985
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 987
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 988
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 59)
                                            if phase < 989
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 990
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 992
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 993
                                    else (
                                        (
                                            ((phase[:7] & 0) | 60)
                                            if phase < 994
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 995
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 997
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 998
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 61)
                                            if phase < 999
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1000
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1002
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1003
                                    else (
                                        (
                                            ((phase[:7] & 0) | 62)
                                            if phase < 1004
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1005
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1007
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 1008
                                                else ((phase[:7] & 0) | 63)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 1009
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1010
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1012
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1013
                                            else ((phase[:7] & 0) | 64)
                                        )
                                    )
                                    if phase < 1014
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1015
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1017
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1018
                                            else ((phase[:7] & 0) | 65)
                                        )
                                    )
                                )
                                if phase < 1019
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1020
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1022
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1023
                                            else ((phase[:7] & 0) | 66)
                                        )
                                    )
                                    if phase < 1024
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1025
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1027
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1028
                                            else ((phase[:7] & 0) | 67)
                                        )
                                    )
                                )
                            )
                            if phase < 1029
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1030
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1032
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1033
                                            else ((phase[:7] & 0) | 68)
                                        )
                                    )
                                    if phase < 1034
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1035
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1037
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1038
                                            else ((phase[:7] & 0) | 69)
                                        )
                                    )
                                )
                                if phase < 1039
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1040
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1042
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1043
                                            else ((phase[:7] & 0) | 70)
                                        )
                                    )
                                    if phase < 1044
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1045
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1047
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1048
                                            else ((phase[:7] & 0) | 71)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1049
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1050
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1052
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1053
                                            else ((phase[:7] & 0) | 72)
                                        )
                                    )
                                    if phase < 1054
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1055
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1057
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1058
                                            else ((phase[:7] & 0) | 73)
                                        )
                                    )
                                )
                                if phase < 1059
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1060
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1062
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1063
                                            else ((phase[:7] & 0) | 74)
                                        )
                                    )
                                    if phase < 1064
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1065
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1067
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1068
                                            else ((phase[:7] & 0) | 75)
                                        )
                                    )
                                )
                            )
                            if phase < 1069
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1070
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1072
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1073
                                            else ((phase[:7] & 0) | 76)
                                        )
                                    )
                                    if phase < 1074
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1075
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1077
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1078
                                            else ((phase[:7] & 0) | 77)
                                        )
                                    )
                                )
                                if phase < 1079
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1080
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1082
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1083
                                            else ((phase[:7] & 0) | 78)
                                        )
                                    )
                                    if phase < 1084
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1085
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1087
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1088
                                            else (
                                                ((phase[:7] & 0) | 79)
                                                if phase < 1089
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1090
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1092
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1093
                                        else (
                                            ((phase[:7] & 0) | 80)
                                            if phase < 1094
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1095
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1097
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1098
                                        else (
                                            ((phase[:7] & 0) | 81)
                                            if phase < 1099
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 1100
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1102
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1103
                                        else (
                                            ((phase[:7] & 0) | 82)
                                            if phase < 1104
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1105
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1107
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1108
                                        else (
                                            ((phase[:7] & 0) | 83)
                                            if phase < 1109
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 1110
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1112
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1113
                                        else (
                                            ((phase[:7] & 0) | 84)
                                            if phase < 1114
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1115
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1117
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1118
                                        else (
                                            ((phase[:7] & 0) | 85)
                                            if phase < 1119
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 1120
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1122
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1123
                                        else (
                                            ((phase[:7] & 0) | 86)
                                            if phase < 1124
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1125
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1127
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1128
                                        else (
                                            ((phase[:7] & 0) | 87)
                                            if phase < 1129
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 1130
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1132
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1133
                                            else ((phase[:7] & 0) | 88)
                                        )
                                        if phase < 1134
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1135
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1137
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1138
                                            else ((phase[:7] & 0) | 89)
                                        )
                                        if phase < 1139
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1140
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1142
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1143
                                            else ((phase[:7] & 0) | 90)
                                        )
                                        if phase < 1144
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1145
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1147
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1148
                                            else ((phase[:7] & 0) | 91)
                                        )
                                        if phase < 1149
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1150
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1152
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1153
                                            else ((phase[:7] & 0) | 92)
                                        )
                                        if phase < 1154
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1155
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1157
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1158
                                            else ((phase[:7] & 0) | 93)
                                        )
                                        if phase < 1159
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1160
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1162
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1163
                                            else ((phase[:7] & 0) | 94)
                                        )
                                        if phase < 1164
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1165
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1167
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1168
                                            else ((phase[:7] & 0) | 95)
                                        )
                                        if phase < 1169
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1170
                                            else (
                                                ((phase[:7] & 0) | 0)
                                                if phase < 1172
                                                else ((phase[:7] & 0) | 3)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1173
                else (
                    (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 96)
                                            if phase < 1174
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1175
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1177
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1178
                                    else (
                                        (
                                            ((phase[:7] & 0) | 97)
                                            if phase < 1179
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1180
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1182
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 1183
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 98)
                                            if phase < 1184
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1185
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1187
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1188
                                    else (
                                        (
                                            ((phase[:7] & 0) | 99)
                                            if phase < 1189
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1190
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1192
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 1193
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 100)
                                            if phase < 1194
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1195
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1197
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1198
                                    else (
                                        (
                                            ((phase[:7] & 0) | 101)
                                            if phase < 1199
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1200
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1202
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                )
                                if phase < 1203
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 102)
                                            if phase < 1204
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1205
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1207
                                            else ((phase[:7] & 0) | 3)
                                        )
                                    )
                                    if phase < 1208
                                    else (
                                        (
                                            ((phase[:7] & 0) | 103)
                                            if phase < 1209
                                            else ((phase[:7] & 0) | 5)
                                        )
                                        if phase < 1210
                                        else (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1212
                                            else (
                                                ((phase[:7] & 0) | 3)
                                                if phase < 1213
                                                else ((phase[:7] & 0) | 104)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1214
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1215
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1217
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1218
                                            else ((phase[:7] & 0) | 105)
                                        )
                                    )
                                    if phase < 1219
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1220
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1222
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1223
                                            else ((phase[:7] & 0) | 106)
                                        )
                                    )
                                )
                                if phase < 1224
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1225
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1227
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1228
                                            else ((phase[:7] & 0) | 107)
                                        )
                                    )
                                    if phase < 1229
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1230
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1232
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1233
                                            else ((phase[:7] & 0) | 108)
                                        )
                                    )
                                )
                            )
                            if phase < 1234
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1235
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1237
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1238
                                            else ((phase[:7] & 0) | 109)
                                        )
                                    )
                                    if phase < 1239
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1240
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1242
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1243
                                            else ((phase[:7] & 0) | 110)
                                        )
                                    )
                                )
                                if phase < 1244
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1245
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1247
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1248
                                            else ((phase[:7] & 0) | 111)
                                        )
                                    )
                                    if phase < 1249
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1250
                                            else ((phase[:7] & 0) | 0)
                                        )
                                        if phase < 1252
                                        else (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1253
                                            else (
                                                ((phase[:7] & 0) | 112)
                                                if phase < 1254
                                                else ((phase[:7] & 0) | 5)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1255
                    else (
                        (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1257
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1258
                                        else (
                                            ((phase[:7] & 0) | 113)
                                            if phase < 1259
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1260
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1262
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1263
                                        else (
                                            ((phase[:7] & 0) | 114)
                                            if phase < 1264
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 1265
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1267
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1268
                                        else (
                                            ((phase[:7] & 0) | 115)
                                            if phase < 1269
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1270
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1272
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1273
                                        else (
                                            ((phase[:7] & 0) | 116)
                                            if phase < 1274
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                            )
                            if phase < 1275
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1277
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1278
                                        else (
                                            ((phase[:7] & 0) | 117)
                                            if phase < 1279
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1280
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1282
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1283
                                        else (
                                            ((phase[:7] & 0) | 118)
                                            if phase < 1284
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                )
                                if phase < 1285
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1287
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1288
                                        else (
                                            ((phase[:7] & 0) | 119)
                                            if phase < 1289
                                            else ((phase[:7] & 0) | 5)
                                        )
                                    )
                                    if phase < 1290
                                    else (
                                        (
                                            ((phase[:7] & 0) | 0)
                                            if phase < 1292
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1293
                                        else (
                                            ((phase[:7] & 0) | 120)
                                            if phase < 1294
                                            else (
                                                ((phase[:7] & 0) | 5)
                                                if phase < 1295
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1297
                        else (
                            (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1298
                                            else ((phase[:7] & 0) | 121)
                                        )
                                        if phase < 1299
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1300
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1302
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1303
                                            else ((phase[:7] & 0) | 122)
                                        )
                                        if phase < 1304
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1305
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1307
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1308
                                            else ((phase[:7] & 0) | 123)
                                        )
                                        if phase < 1309
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1310
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1312
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1313
                                            else ((phase[:7] & 0) | 124)
                                        )
                                        if phase < 1314
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1315
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1317
                            else (
                                (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1318
                                            else ((phase[:7] & 0) | 125)
                                        )
                                        if phase < 1319
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1320
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1322
                                    else (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1323
                                            else ((phase[:7] & 0) | 126)
                                        )
                                        if phase < 1324
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1325
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1327
                                else (
                                    (
                                        (
                                            ((phase[:7] & 0) | 3)
                                            if phase < 1328
                                            else ((phase[:7] & 0) | 127)
                                        )
                                        if phase < 1329
                                        else (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1330
                                            else ((phase[:7] & 0) | 0)
                                        )
                                    )
                                    if phase < 1332
                                    else (
                                        (
                                            ((phase[:7] & 0) | 5)
                                            if phase < 1334
                                            else ((phase[:7] & 0) | 3)
                                        )
                                        if phase < 1335
                                        else (
                                            ((phase[:7] & 0) | 50)
                                            if phase < 1336
                                            else (
                                                ((phase[:7] & 0) | 70)
                                                if phase < 1337
                                                else ((phase[:7] & 0) | 0)
                                            )
                                        )
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
        if phase < 1343:
            assert dut.ready == expected_ready, "table_rule: ready"
            assert dut.valid == expected_valid, "table_rule: valid"
            assert dut.data.index == expected_data_index, "table_rule: data.index"
            assert dut.data.value == expected_data_value, "table_rule: data.value"
        log("info", "table_rule.ready", dut.ready)
        log("info", "table_rule.valid", dut.valid)
        log("info", "table_rule.data.index", dut.data.index)
        log("info", "table_rule.data.value", dut.data.value)

    check()
    advance(phase)

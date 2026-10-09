"""Regular-clock full known-input stream from the retained dma oracle."""

from example_dma.dma import Dma, DmaRequest
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseDma():  # noqa: N802
    phase: bits[64] = 0
    seed_valid = (
        (
            (
                phase[:1] & 0 | 0
                if phase < 1
                else phase[:1] & 0 | 1 if phase < 2 else phase[:1] & 0 | 0
            )
            if phase < 21
            else (
                (phase[:1] & 0 | 1 if phase < 22 else phase[:1] & 0 | 0)
                if phase < 41
                else phase[:1] & 0 | 1 if phase < 52 else phase[:1] & 0 | 0
            )
        )
        if phase < 53
        else (
            (
                (phase[:1] & 0 | 1 if phase < 133 else phase[:1] & 0 | 0)
                if phase < 134
                else phase[:1] & 0 | 1 if phase < 529 else phase[:1] & 0 | 0
            )
            if phase < 540
            else (
                (phase[:1] & 0 | 1 if phase < 541 else phase[:1] & 0 | 0)
                if phase < 558
                else phase[:1] & 0 | 1 if phase < 566 else phase[:1] & 0 | 0
            )
        )
    )
    seed_dram = (
        (
            (
                (phase[:4] & 0 | 0 if phase < 1 else phase[:4] & 0 | 5)
                if phase < 2
                else (
                    phase[:4] & 0 | 0
                    if phase < 21
                    else phase[:4] & 0 | 9 if phase < 22 else phase[:4] & 0 | 0
                )
            )
            if phase < 42
            else (
                (
                    phase[:4] & 0 | 1
                    if phase < 43
                    else phase[:4] & 0 | 2 if phase < 483 else phase[:4] & 0 | 3
                )
                if phase < 487
                else (
                    phase[:4] & 0 | 4
                    if phase < 491
                    else phase[:4] & 0 | 5 if phase < 497 else phase[:4] & 0 | 6
                )
            )
        )
        if phase < 501
        else (
            (
                (
                    phase[:4] & 0 | 7
                    if phase < 505
                    else phase[:4] & 0 | 8 if phase < 509 else phase[:4] & 0 | 9
                )
                if phase < 513
                else (
                    phase[:4] & 0 | 10
                    if phase < 517
                    else phase[:4] & 0 | 11 if phase < 521 else phase[:4] & 0 | 12
                )
            )
            if phase < 525
            else (
                (
                    phase[:4] & 0 | 13
                    if phase < 529
                    else phase[:4] & 0 | 0 if phase < 540 else phase[:4] & 0 | 14
                )
                if phase < 541
                else (
                    phase[:4] & 0 | 0
                    if phase < 558
                    else phase[:4] & 0 | 2 if phase < 566 else phase[:4] & 0 | 0
                )
            )
        )
    )
    seed_sram = (
        (
            (
                (
                    phase[:4] & 0 | 0
                    if phase < 1
                    else phase[:4] & 0 | 3 if phase < 2 else phase[:4] & 0 | 0
                )
                if phase < 21
                else (
                    phase[:4] & 0 | 1
                    if phase < 22
                    else phase[:4] & 0 | 0 if phase < 41 else phase[:4] & 0 | 5
                )
            )
            if phase < 42
            else (
                (
                    phase[:4] & 0 | 6
                    if phase < 43
                    else phase[:4] & 0 | 7 if phase < 483 else phase[:4] & 0 | 8
                )
                if phase < 487
                else (
                    phase[:4] & 0 | 9
                    if phase < 491
                    else phase[:4] & 0 | 10 if phase < 497 else phase[:4] & 0 | 11
                )
            )
        )
        if phase < 501
        else (
            (
                (
                    phase[:4] & 0 | 12
                    if phase < 505
                    else phase[:4] & 0 | 13 if phase < 509 else phase[:4] & 0 | 14
                )
                if phase < 513
                else (
                    phase[:4] & 0 | 15
                    if phase < 517
                    else phase[:4] & 0 | 0 if phase < 521 else phase[:4] & 0 | 1
                )
            )
            if phase < 525
            else (
                (
                    phase[:4] & 0 | 2
                    if phase < 529
                    else phase[:4] & 0 | 0 if phase < 540 else phase[:4] & 0 | 11
                )
                if phase < 541
                else (
                    phase[:4] & 0 | 0
                    if phase < 558
                    else phase[:4] & 0 | 7 if phase < 566 else phase[:4] & 0 | 0
                )
            )
        )
    )
    seed_data = (
        (
            (
                (
                    phase[:16] & 0 | 0
                    if phase < 1
                    else phase[:16] & 0 | 4660 if phase < 2 else phase[:16] & 0 | 0
                )
                if phase < 21
                else (
                    phase[:16] & 0 | 48879
                    if phase < 22
                    else phase[:16] & 0 | 0 if phase < 41 else phase[:16] & 0 | 16384
                )
            )
            if phase < 42
            else (
                (
                    phase[:16] & 0 | 16385
                    if phase < 43
                    else (
                        phase[:16] & 0 | 16386
                        if phase < 483
                        else phase[:16] & 0 | 16387
                    )
                )
                if phase < 487
                else (
                    phase[:16] & 0 | 16388
                    if phase < 491
                    else (
                        phase[:16] & 0 | 16389
                        if phase < 497
                        else phase[:16] & 0 | 16390
                    )
                )
            )
        )
        if phase < 501
        else (
            (
                (
                    phase[:16] & 0 | 16391
                    if phase < 505
                    else (
                        phase[:16] & 0 | 16392
                        if phase < 509
                        else phase[:16] & 0 | 16393
                    )
                )
                if phase < 513
                else (
                    phase[:16] & 0 | 16394
                    if phase < 517
                    else (
                        phase[:16] & 0 | 16395
                        if phase < 521
                        else phase[:16] & 0 | 16396
                    )
                )
            )
            if phase < 525
            else (
                (
                    phase[:16] & 0 | 16397
                    if phase < 529
                    else phase[:16] & 0 | 0 if phase < 540 else phase[:16] & 0 | 30292
                )
                if phase < 541
                else (
                    phase[:16] & 0 | 0
                    if phase < 558
                    else phase[:16] & 0 | 17767 if phase < 566 else phase[:16] & 0 | 0
                )
            )
        )
    )
    seed_tag = (
        (
            (
                (
                    phase[:8] & 0 | 0
                    if phase < 1
                    else phase[:8] & 0 | 1 if phase < 2 else phase[:8] & 0 | 0
                )
                if phase < 21
                else (
                    (phase[:8] & 0 | 4 if phase < 22 else phase[:8] & 0 | 0)
                    if phase < 41
                    else phase[:8] & 0 | 10 if phase < 42 else phase[:8] & 0 | 11
                )
            )
            if phase < 43
            else (
                (
                    (phase[:8] & 0 | 12 if phase < 483 else phase[:8] & 0 | 13)
                    if phase < 487
                    else phase[:8] & 0 | 14 if phase < 491 else phase[:8] & 0 | 15
                )
                if phase < 497
                else (
                    (phase[:8] & 0 | 16 if phase < 501 else phase[:8] & 0 | 17)
                    if phase < 505
                    else phase[:8] & 0 | 18 if phase < 509 else phase[:8] & 0 | 19
                )
            )
        )
        if phase < 513
        else (
            (
                (
                    (phase[:8] & 0 | 20 if phase < 517 else phase[:8] & 0 | 21)
                    if phase < 521
                    else phase[:8] & 0 | 22 if phase < 525 else phase[:8] & 0 | 23
                )
                if phase < 529
                else (
                    (phase[:8] & 0 | 0 if phase < 540 else phase[:8] & 0 | 100)
                    if phase < 541
                    else phase[:8] & 0 | 0 if phase < 558 else phase[:8] & 0 | 110
                )
            )
            if phase < 559
            else (
                (
                    (phase[:8] & 0 | 111 if phase < 560 else phase[:8] & 0 | 112)
                    if phase < 561
                    else phase[:8] & 0 | 113 if phase < 562 else phase[:8] & 0 | 114
                )
                if phase < 563
                else (
                    (phase[:8] & 0 | 115 if phase < 564 else phase[:8] & 0 | 116)
                    if phase < 565
                    else phase[:8] & 0 | 117 if phase < 566 else phase[:8] & 0 | 0
                )
            )
        )
    )
    copy_valid = (
        (
            (
                (phase[:1] & 0 | 0 if phase < 7 else phase[:1] & 0 | 1)
                if phase < 8
                else phase[:1] & 0 | 0 if phase < 27 else phase[:1] & 0 | 1
            )
            if phase < 28
            else (
                (phase[:1] & 0 | 0 if phase < 41 else phase[:1] & 0 | 1)
                if phase < 52
                else phase[:1] & 0 | 0 if phase < 53 else phase[:1] & 0 | 1
            )
        )
        if phase < 133
        else (
            (
                (phase[:1] & 0 | 0 if phase < 134 else phase[:1] & 0 | 1)
                if phase < 463
                else phase[:1] & 0 | 0 if phase < 546 else phase[:1] & 0 | 1
            )
            if phase < 547
            else (
                (phase[:1] & 0 | 0 if phase < 558 else phase[:1] & 0 | 1)
                if phase < 566
                else (
                    phase[:1] & 0 | 0
                    if phase < 567
                    else phase[:1] & 0 | 1 if phase < 568 else phase[:1] & 0 | 0
                )
            )
        )
    )
    copy_dram = (
        (
            (
                (
                    (phase[:4] & 0 | 0 if phase < 7 else phase[:4] & 0 | 5)
                    if phase < 8
                    else phase[:4] & 0 | 0 if phase < 27 else phase[:4] & 0 | 9
                )
                if phase < 28
                else (
                    (phase[:4] & 0 | 0 if phase < 42 else phase[:4] & 0 | 3)
                    if phase < 43
                    else phase[:4] & 0 | 6 if phase < 44 else phase[:4] & 0 | 9
                )
            )
            if phase < 45
            else (
                (
                    (phase[:4] & 0 | 12 if phase < 46 else phase[:4] & 0 | 15)
                    if phase < 48
                    else phase[:4] & 0 | 2 if phase < 53 else phase[:4] & 0 | 5
                )
                if phase < 222
                else (
                    (phase[:4] & 0 | 8 if phase < 226 else phase[:4] & 0 | 11)
                    if phase < 230
                    else phase[:4] & 0 | 14 if phase < 419 else phase[:4] & 0 | 1
                )
            )
        )
        if phase < 423
        else (
            (
                (
                    (phase[:4] & 0 | 4 if phase < 427 else phase[:4] & 0 | 7)
                    if phase < 431
                    else phase[:4] & 0 | 10 if phase < 435 else phase[:4] & 0 | 13
                )
                if phase < 439
                else (
                    (phase[:4] & 0 | 0 if phase < 443 else phase[:4] & 0 | 3)
                    if phase < 447
                    else phase[:4] & 0 | 6 if phase < 451 else phase[:4] & 0 | 9
                )
            )
            if phase < 455
            else (
                (
                    (phase[:4] & 0 | 12 if phase < 459 else phase[:4] & 0 | 15)
                    if phase < 463
                    else phase[:4] & 0 | 0 if phase < 546 else phase[:4] & 0 | 14
                )
                if phase < 547
                else (
                    (phase[:4] & 0 | 0 if phase < 558 else phase[:4] & 0 | 9)
                    if phase < 566
                    else (
                        phase[:4] & 0 | 0
                        if phase < 567
                        else phase[:4] & 0 | 14 if phase < 568 else phase[:4] & 0 | 0
                    )
                )
            )
        )
    )
    copy_sram = (
        (
            (
                (
                    (phase[:4] & 0 | 0 if phase < 7 else phase[:4] & 0 | 3)
                    if phase < 8
                    else phase[:4] & 0 | 0 if phase < 27 else phase[:4] & 0 | 3
                )
                if phase < 28
                else (
                    (phase[:4] & 0 | 0 if phase < 41 else phase[:4] & 0 | 3)
                    if phase < 42
                    else phase[:4] & 0 | 8 if phase < 43 else phase[:4] & 0 | 13
                )
            )
            if phase < 44
            else (
                (
                    (phase[:4] & 0 | 2 if phase < 45 else phase[:4] & 0 | 7)
                    if phase < 46
                    else phase[:4] & 0 | 12 if phase < 48 else phase[:4] & 0 | 1
                )
                if phase < 53
                else (
                    (phase[:4] & 0 | 6 if phase < 222 else phase[:4] & 0 | 11)
                    if phase < 226
                    else (
                        phase[:4] & 0 | 0
                        if phase < 230
                        else phase[:4] & 0 | 5 if phase < 419 else phase[:4] & 0 | 10
                    )
                )
            )
        )
        if phase < 423
        else (
            (
                (
                    (phase[:4] & 0 | 15 if phase < 427 else phase[:4] & 0 | 4)
                    if phase < 431
                    else phase[:4] & 0 | 9 if phase < 435 else phase[:4] & 0 | 14
                )
                if phase < 439
                else (
                    (phase[:4] & 0 | 3 if phase < 443 else phase[:4] & 0 | 8)
                    if phase < 447
                    else phase[:4] & 0 | 13 if phase < 451 else phase[:4] & 0 | 2
                )
            )
            if phase < 455
            else (
                (
                    (phase[:4] & 0 | 7 if phase < 459 else phase[:4] & 0 | 12)
                    if phase < 463
                    else phase[:4] & 0 | 0 if phase < 546 else phase[:4] & 0 | 11
                )
                if phase < 547
                else (
                    (phase[:4] & 0 | 0 if phase < 558 else phase[:4] & 0 | 4)
                    if phase < 566
                    else (
                        phase[:4] & 0 | 0
                        if phase < 567
                        else phase[:4] & 0 | 11 if phase < 568 else phase[:4] & 0 | 0
                    )
                )
            )
        )
    )
    copy_data = (
        (
            (
                (
                    phase[:16] & 0 | 0
                    if phase < 27
                    else phase[:16] & 0 | 30583 if phase < 28 else phase[:16] & 0 | 0
                )
                if phase < 41
                else (
                    phase[:16] & 0 | 40960
                    if phase < 42
                    else (
                        phase[:16] & 0 | 40961 if phase < 43 else phase[:16] & 0 | 40962
                    )
                )
            )
            if phase < 44
            else (
                (
                    phase[:16] & 0 | 40963
                    if phase < 45
                    else (
                        phase[:16] & 0 | 40964 if phase < 46 else phase[:16] & 0 | 40965
                    )
                )
                if phase < 48
                else (
                    (phase[:16] & 0 | 40966 if phase < 53 else phase[:16] & 0 | 40967)
                    if phase < 222
                    else (
                        phase[:16] & 0 | 40968
                        if phase < 226
                        else phase[:16] & 0 | 40969
                    )
                )
            )
        )
        if phase < 230
        else (
            (
                (
                    phase[:16] & 0 | 40970
                    if phase < 419
                    else (
                        phase[:16] & 0 | 40971
                        if phase < 423
                        else phase[:16] & 0 | 40972
                    )
                )
                if phase < 427
                else (
                    phase[:16] & 0 | 40973
                    if phase < 431
                    else (
                        phase[:16] & 0 | 40974
                        if phase < 435
                        else phase[:16] & 0 | 40975
                    )
                )
            )
            if phase < 439
            else (
                (
                    phase[:16] & 0 | 40976
                    if phase < 443
                    else (
                        phase[:16] & 0 | 40977
                        if phase < 447
                        else phase[:16] & 0 | 40978
                    )
                )
                if phase < 451
                else (
                    (phase[:16] & 0 | 40979 if phase < 455 else phase[:16] & 0 | 40980)
                    if phase < 459
                    else phase[:16] & 0 | 40981 if phase < 463 else phase[:16] & 0 | 0
                )
            )
        )
    )
    copy_tag = (
        (
            (
                (
                    (phase[:8] & 0 | 0 if phase < 7 else phase[:8] & 0 | 2)
                    if phase < 8
                    else (
                        phase[:8] & 0 | 0
                        if phase < 27
                        else phase[:8] & 0 | 5 if phase < 28 else phase[:8] & 0 | 0
                    )
                )
                if phase < 41
                else (
                    (phase[:8] & 0 | 24 if phase < 42 else phase[:8] & 0 | 25)
                    if phase < 43
                    else (
                        phase[:8] & 0 | 26
                        if phase < 44
                        else phase[:8] & 0 | 27 if phase < 45 else phase[:8] & 0 | 28
                    )
                )
            )
            if phase < 46
            else (
                (
                    (phase[:8] & 0 | 29 if phase < 48 else phase[:8] & 0 | 30)
                    if phase < 53
                    else (
                        phase[:8] & 0 | 31
                        if phase < 222
                        else phase[:8] & 0 | 32 if phase < 226 else phase[:8] & 0 | 33
                    )
                )
                if phase < 230
                else (
                    (phase[:8] & 0 | 34 if phase < 419 else phase[:8] & 0 | 35)
                    if phase < 423
                    else (
                        phase[:8] & 0 | 36
                        if phase < 427
                        else phase[:8] & 0 | 37 if phase < 431 else phase[:8] & 0 | 38
                    )
                )
            )
        )
        if phase < 435
        else (
            (
                (
                    (phase[:8] & 0 | 39 if phase < 439 else phase[:8] & 0 | 40)
                    if phase < 443
                    else (
                        phase[:8] & 0 | 41
                        if phase < 447
                        else phase[:8] & 0 | 42 if phase < 451 else phase[:8] & 0 | 43
                    )
                )
                if phase < 455
                else (
                    (phase[:8] & 0 | 44 if phase < 459 else phase[:8] & 0 | 45)
                    if phase < 463
                    else (
                        phase[:8] & 0 | 0
                        if phase < 546
                        else phase[:8] & 0 | 101 if phase < 547 else phase[:8] & 0 | 0
                    )
                )
            )
            if phase < 558
            else (
                (
                    (phase[:8] & 0 | 130 if phase < 559 else phase[:8] & 0 | 131)
                    if phase < 560
                    else (
                        phase[:8] & 0 | 132
                        if phase < 561
                        else phase[:8] & 0 | 133 if phase < 562 else phase[:8] & 0 | 134
                    )
                )
                if phase < 563
                else (
                    (
                        phase[:8] & 0 | 135
                        if phase < 564
                        else phase[:8] & 0 | 136 if phase < 565 else phase[:8] & 0 | 137
                    )
                    if phase < 566
                    else (
                        phase[:8] & 0 | 0
                        if phase < 567
                        else phase[:8] & 0 | 180 if phase < 568 else phase[:8] & 0 | 0
                    )
                )
            )
        )
    )
    check_valid = (
        (
            (
                (phase[:1] & 0 | 0 if phase < 16 else phase[:1] & 0 | 1)
                if phase < 17
                else phase[:1] & 0 | 0 if phase < 36 else phase[:1] & 0 | 1
            )
            if phase < 37
            else (
                (phase[:1] & 0 | 0 if phase < 41 else phase[:1] & 0 | 1)
                if phase < 52
                else phase[:1] & 0 | 0 if phase < 53 else phase[:1] & 0 | 1
            )
        )
        if phase < 133
        else (
            (
                (phase[:1] & 0 | 0 if phase < 134 else phase[:1] & 0 | 1)
                if phase < 211
                else phase[:1] & 0 | 0 if phase < 553 else phase[:1] & 0 | 1
            )
            if phase < 554
            else (
                (phase[:1] & 0 | 0 if phase < 558 else phase[:1] & 0 | 1)
                if phase < 566
                else (
                    phase[:1] & 0 | 0
                    if phase < 576
                    else phase[:1] & 0 | 1 if phase < 577 else phase[:1] & 0 | 0
                )
            )
        )
    )
    check_dram = (
        (
            (
                (
                    phase[:4] & 0 | 0
                    if phase < 36
                    else phase[:4] & 0 | 12 if phase < 37 else phase[:4] & 0 | 0
                )
                if phase < 41
                else (
                    (phase[:4] & 0 | 7 if phase < 42 else phase[:4] & 0 | 8)
                    if phase < 43
                    else phase[:4] & 0 | 9 if phase < 44 else phase[:4] & 0 | 10
                )
            )
            if phase < 47
            else (
                (
                    (phase[:4] & 0 | 11 if phase < 50 else phase[:4] & 0 | 12)
                    if phase < 157
                    else phase[:4] & 0 | 13 if phase < 160 else phase[:4] & 0 | 14
                )
                if phase < 163
                else (
                    (phase[:4] & 0 | 15 if phase < 166 else phase[:4] & 0 | 0)
                    if phase < 169
                    else phase[:4] & 0 | 1 if phase < 172 else phase[:4] & 0 | 2
                )
            )
        )
        if phase < 175
        else (
            (
                (
                    phase[:4] & 0 | 3
                    if phase < 178
                    else phase[:4] & 0 | 4 if phase < 181 else phase[:4] & 0 | 5
                )
                if phase < 184
                else (
                    (phase[:4] & 0 | 6 if phase < 187 else phase[:4] & 0 | 7)
                    if phase < 190
                    else phase[:4] & 0 | 8 if phase < 193 else phase[:4] & 0 | 9
                )
            )
            if phase < 196
            else (
                (
                    (phase[:4] & 0 | 10 if phase < 199 else phase[:4] & 0 | 11)
                    if phase < 202
                    else phase[:4] & 0 | 12 if phase < 205 else phase[:4] & 0 | 13
                )
                if phase < 208
                else (
                    (phase[:4] & 0 | 14 if phase < 211 else phase[:4] & 0 | 0)
                    if phase < 553
                    else phase[:4] & 0 | 6 if phase < 554 else phase[:4] & 0 | 0
                )
            )
        )
    )
    check_sram = (
        (
            (
                (
                    (phase[:4] & 0 | 0 if phase < 16 else phase[:4] & 0 | 3)
                    if phase < 17
                    else phase[:4] & 0 | 0 if phase < 36 else phase[:4] & 0 | 3
                )
                if phase < 37
                else (
                    (phase[:4] & 0 | 0 if phase < 41 else phase[:4] & 0 | 3)
                    if phase < 42
                    else (
                        phase[:4] & 0 | 8
                        if phase < 43
                        else phase[:4] & 0 | 13 if phase < 44 else phase[:4] & 0 | 2
                    )
                )
            )
            if phase < 47
            else (
                (
                    (phase[:4] & 0 | 7 if phase < 50 else phase[:4] & 0 | 12)
                    if phase < 157
                    else phase[:4] & 0 | 1 if phase < 160 else phase[:4] & 0 | 6
                )
                if phase < 163
                else (
                    (phase[:4] & 0 | 11 if phase < 166 else phase[:4] & 0 | 0)
                    if phase < 169
                    else (
                        phase[:4] & 0 | 5
                        if phase < 172
                        else phase[:4] & 0 | 10 if phase < 175 else phase[:4] & 0 | 15
                    )
                )
            )
        )
        if phase < 178
        else (
            (
                (
                    (phase[:4] & 0 | 4 if phase < 181 else phase[:4] & 0 | 9)
                    if phase < 184
                    else phase[:4] & 0 | 14 if phase < 187 else phase[:4] & 0 | 3
                )
                if phase < 190
                else (
                    (phase[:4] & 0 | 8 if phase < 193 else phase[:4] & 0 | 13)
                    if phase < 196
                    else (
                        phase[:4] & 0 | 2
                        if phase < 199
                        else phase[:4] & 0 | 7 if phase < 202 else phase[:4] & 0 | 12
                    )
                )
            )
            if phase < 205
            else (
                (
                    (phase[:4] & 0 | 1 if phase < 208 else phase[:4] & 0 | 6)
                    if phase < 211
                    else phase[:4] & 0 | 0 if phase < 553 else phase[:4] & 0 | 11
                )
                if phase < 554
                else (
                    (phase[:4] & 0 | 0 if phase < 558 else phase[:4] & 0 | 3)
                    if phase < 566
                    else (
                        phase[:4] & 0 | 0
                        if phase < 576
                        else phase[:4] & 0 | 11 if phase < 577 else phase[:4] & 0 | 0
                    )
                )
            )
        )
    )
    check_data = (
        (
            (
                (
                    phase[:16] & 0 | 0
                    if phase < 36
                    else phase[:16] & 0 | 21845 if phase < 37 else phase[:16] & 0 | 0
                )
                if phase < 41
                else (
                    (phase[:16] & 0 | 36864 if phase < 42 else phase[:16] & 0 | 36865)
                    if phase < 43
                    else (
                        phase[:16] & 0 | 36866 if phase < 44 else phase[:16] & 0 | 36867
                    )
                )
            )
            if phase < 47
            else (
                (
                    phase[:16] & 0 | 36868
                    if phase < 50
                    else (
                        phase[:16] & 0 | 36869
                        if phase < 157
                        else phase[:16] & 0 | 36870
                    )
                )
                if phase < 160
                else (
                    (phase[:16] & 0 | 36871 if phase < 163 else phase[:16] & 0 | 36872)
                    if phase < 166
                    else (
                        phase[:16] & 0 | 36873
                        if phase < 169
                        else phase[:16] & 0 | 36874
                    )
                )
            )
        )
        if phase < 172
        else (
            (
                (
                    phase[:16] & 0 | 36875
                    if phase < 175
                    else (
                        phase[:16] & 0 | 36876
                        if phase < 178
                        else phase[:16] & 0 | 36877
                    )
                )
                if phase < 181
                else (
                    (phase[:16] & 0 | 36878 if phase < 184 else phase[:16] & 0 | 36879)
                    if phase < 187
                    else (
                        phase[:16] & 0 | 36880
                        if phase < 190
                        else phase[:16] & 0 | 36881
                    )
                )
            )
            if phase < 193
            else (
                (
                    phase[:16] & 0 | 36882
                    if phase < 196
                    else (
                        phase[:16] & 0 | 36883
                        if phase < 199
                        else phase[:16] & 0 | 36884
                    )
                )
                if phase < 202
                else (
                    (phase[:16] & 0 | 36885 if phase < 205 else phase[:16] & 0 | 36886)
                    if phase < 208
                    else phase[:16] & 0 | 36887 if phase < 211 else phase[:16] & 0 | 0
                )
            )
        )
    )
    check_tag = (
        (
            (
                (
                    (phase[:8] & 0 | 0 if phase < 16 else phase[:8] & 0 | 3)
                    if phase < 17
                    else (
                        phase[:8] & 0 | 0
                        if phase < 36
                        else phase[:8] & 0 | 6 if phase < 37 else phase[:8] & 0 | 0
                    )
                )
                if phase < 41
                else (
                    (phase[:8] & 0 | 46 if phase < 42 else phase[:8] & 0 | 47)
                    if phase < 43
                    else (
                        phase[:8] & 0 | 48
                        if phase < 44
                        else phase[:8] & 0 | 49 if phase < 47 else phase[:8] & 0 | 50
                    )
                )
            )
            if phase < 50
            else (
                (
                    (phase[:8] & 0 | 51 if phase < 157 else phase[:8] & 0 | 52)
                    if phase < 160
                    else (
                        phase[:8] & 0 | 53
                        if phase < 163
                        else phase[:8] & 0 | 54 if phase < 166 else phase[:8] & 0 | 55
                    )
                )
                if phase < 169
                else (
                    (
                        phase[:8] & 0 | 56
                        if phase < 172
                        else phase[:8] & 0 | 57 if phase < 175 else phase[:8] & 0 | 58
                    )
                    if phase < 178
                    else (
                        phase[:8] & 0 | 59
                        if phase < 181
                        else phase[:8] & 0 | 60 if phase < 184 else phase[:8] & 0 | 61
                    )
                )
            )
        )
        if phase < 187
        else (
            (
                (
                    (phase[:8] & 0 | 62 if phase < 190 else phase[:8] & 0 | 63)
                    if phase < 193
                    else (
                        phase[:8] & 0 | 64
                        if phase < 196
                        else phase[:8] & 0 | 65 if phase < 199 else phase[:8] & 0 | 66
                    )
                )
                if phase < 202
                else (
                    (
                        phase[:8] & 0 | 67
                        if phase < 205
                        else phase[:8] & 0 | 68 if phase < 208 else phase[:8] & 0 | 69
                    )
                    if phase < 211
                    else (
                        phase[:8] & 0 | 0
                        if phase < 553
                        else phase[:8] & 0 | 102 if phase < 554 else phase[:8] & 0 | 0
                    )
                )
            )
            if phase < 558
            else (
                (
                    (phase[:8] & 0 | 150 if phase < 559 else phase[:8] & 0 | 151)
                    if phase < 560
                    else (
                        phase[:8] & 0 | 152
                        if phase < 561
                        else phase[:8] & 0 | 153 if phase < 562 else phase[:8] & 0 | 154
                    )
                )
                if phase < 563
                else (
                    (
                        phase[:8] & 0 | 155
                        if phase < 564
                        else phase[:8] & 0 | 156 if phase < 565 else phase[:8] & 0 | 157
                    )
                    if phase < 566
                    else (
                        phase[:8] & 0 | 0
                        if phase < 576
                        else phase[:8] & 0 | 181 if phase < 577 else phase[:8] & 0 | 0
                    )
                )
            )
        )
    )
    take_seed = (
        (phase[:1] & 0 | 1 if phase < 41 else phase[:1] & 0 | 0)
        if phase < 493
        else (
            phase[:1] & 0 | 1
            if phase < 558
            else phase[:1] & 0 | 0 if phase < 566 else phase[:1] & 0 | 1
        )
    )
    take_copy = (
        (phase[:1] & 0 | 1 if phase < 41 else phase[:1] & 0 | 0)
        if phase < 413
        else (
            phase[:1] & 0 | 1
            if phase < 558
            else phase[:1] & 0 | 0 if phase < 566 else phase[:1] & 0 | 1
        )
    )
    take_check = (
        (phase[:1] & 0 | 1 if phase < 41 else phase[:1] & 0 | 0)
        if phase < 153
        else (
            phase[:1] & 0 | 1
            if phase < 558
            else phase[:1] & 0 | 0 if phase < 566 else phase[:1] & 0 | 1
        )
    )
    seed = DmaRequest(
        dram_address=seed_dram, sram_address=seed_sram, data=seed_data, tag=seed_tag
    )
    copy = DmaRequest(
        dram_address=copy_dram, sram_address=copy_sram, data=copy_data, tag=copy_tag
    )
    check = DmaRequest(
        dram_address=check_dram, sram_address=check_sram, data=check_data, tag=check_tag
    )
    dut = Dma(
        seed_valid,
        seed,
        copy_valid,
        copy,
        check_valid,
        check,
        take_seed,
        take_copy,
        take_check,
    )
    expected_seed_ready = (
        (
            (
                (
                    phase[:1] & 0 | 1
                    if phase < 43
                    else phase[:1] & 0 | 0 if phase < 482 else phase[:1] & 0 | 1
                )
                if phase < 483
                else (
                    (phase[:1] & 0 | 0 if phase < 486 else phase[:1] & 0 | 1)
                    if phase < 487
                    else phase[:1] & 0 | 0 if phase < 490 else phase[:1] & 0 | 1
                )
            )
            if phase < 491
            else (
                (
                    phase[:1] & 0 | 0
                    if phase < 496
                    else phase[:1] & 0 | 1 if phase < 497 else phase[:1] & 0 | 0
                )
                if phase < 500
                else (
                    (phase[:1] & 0 | 1 if phase < 501 else phase[:1] & 0 | 0)
                    if phase < 504
                    else phase[:1] & 0 | 1 if phase < 505 else phase[:1] & 0 | 0
                )
            )
        )
        if phase < 508
        else (
            (
                (
                    phase[:1] & 0 | 1
                    if phase < 509
                    else phase[:1] & 0 | 0 if phase < 512 else phase[:1] & 0 | 1
                )
                if phase < 513
                else (
                    (phase[:1] & 0 | 0 if phase < 516 else phase[:1] & 0 | 1)
                    if phase < 517
                    else phase[:1] & 0 | 0 if phase < 520 else phase[:1] & 0 | 1
                )
            )
            if phase < 521
            else (
                (
                    phase[:1] & 0 | 0
                    if phase < 524
                    else phase[:1] & 0 | 1 if phase < 525 else phase[:1] & 0 | 0
                )
                if phase < 528
                else (
                    (phase[:1] & 0 | 1 if phase < 529 else phase[:1] & 0 | 0)
                    if phase < 532
                    else phase[:1] & 0 | 1 if phase < 560 else phase[:1] & 0 | 0
                )
            )
        )
    )
    expected_copy_ready = (
        (
            (
                (
                    (phase[:1] & 0 | 1 if phase < 46 else phase[:1] & 0 | 0)
                    if phase < 47
                    else (
                        phase[:1] & 0 | 1
                        if phase < 48
                        else phase[:1] & 0 | 0 if phase < 51 else phase[:1] & 0 | 1
                    )
                )
                if phase < 52
                else (
                    (phase[:1] & 0 | 0 if phase < 221 else phase[:1] & 0 | 1)
                    if phase < 222
                    else (
                        phase[:1] & 0 | 0
                        if phase < 225
                        else phase[:1] & 0 | 1 if phase < 226 else phase[:1] & 0 | 0
                    )
                )
            )
            if phase < 229
            else (
                (
                    (phase[:1] & 0 | 1 if phase < 230 else phase[:1] & 0 | 0)
                    if phase < 418
                    else (
                        phase[:1] & 0 | 1
                        if phase < 419
                        else phase[:1] & 0 | 0 if phase < 422 else phase[:1] & 0 | 1
                    )
                )
                if phase < 423
                else (
                    (phase[:1] & 0 | 0 if phase < 426 else phase[:1] & 0 | 1)
                    if phase < 427
                    else (
                        phase[:1] & 0 | 0
                        if phase < 430
                        else phase[:1] & 0 | 1 if phase < 431 else phase[:1] & 0 | 0
                    )
                )
            )
        )
        if phase < 434
        else (
            (
                (
                    (phase[:1] & 0 | 1 if phase < 435 else phase[:1] & 0 | 0)
                    if phase < 438
                    else (
                        phase[:1] & 0 | 1
                        if phase < 439
                        else phase[:1] & 0 | 0 if phase < 442 else phase[:1] & 0 | 1
                    )
                )
                if phase < 443
                else (
                    (phase[:1] & 0 | 0 if phase < 446 else phase[:1] & 0 | 1)
                    if phase < 447
                    else (
                        phase[:1] & 0 | 0
                        if phase < 450
                        else phase[:1] & 0 | 1 if phase < 451 else phase[:1] & 0 | 0
                    )
                )
            )
            if phase < 454
            else (
                (
                    (phase[:1] & 0 | 1 if phase < 455 else phase[:1] & 0 | 0)
                    if phase < 458
                    else (
                        phase[:1] & 0 | 1
                        if phase < 459
                        else phase[:1] & 0 | 0 if phase < 462 else phase[:1] & 0 | 1
                    )
                )
                if phase < 463
                else (
                    (
                        phase[:1] & 0 | 0
                        if phase < 466
                        else phase[:1] & 0 | 1 if phase < 563 else phase[:1] & 0 | 0
                    )
                    if phase < 564
                    else (
                        phase[:1] & 0 | 1
                        if phase < 565
                        else phase[:1] & 0 | 0 if phase < 568 else phase[:1] & 0 | 1
                    )
                )
            )
        )
    )
    expected_check_ready = (
        (
            (
                (
                    (
                        phase[:1] & 0 | 1
                        if phase < 44
                        else phase[:1] & 0 | 0 if phase < 46 else phase[:1] & 0 | 1
                    )
                    if phase < 47
                    else (
                        phase[:1] & 0 | 0
                        if phase < 49
                        else phase[:1] & 0 | 1 if phase < 50 else phase[:1] & 0 | 0
                    )
                )
                if phase < 156
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 157
                        else phase[:1] & 0 | 0 if phase < 159 else phase[:1] & 0 | 1
                    )
                    if phase < 160
                    else (
                        phase[:1] & 0 | 0
                        if phase < 162
                        else phase[:1] & 0 | 1 if phase < 163 else phase[:1] & 0 | 0
                    )
                )
            )
            if phase < 165
            else (
                (
                    (
                        phase[:1] & 0 | 1
                        if phase < 166
                        else phase[:1] & 0 | 0 if phase < 168 else phase[:1] & 0 | 1
                    )
                    if phase < 169
                    else (
                        phase[:1] & 0 | 0
                        if phase < 171
                        else phase[:1] & 0 | 1 if phase < 172 else phase[:1] & 0 | 0
                    )
                )
                if phase < 174
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 175
                        else phase[:1] & 0 | 0 if phase < 177 else phase[:1] & 0 | 1
                    )
                    if phase < 178
                    else (
                        phase[:1] & 0 | 0
                        if phase < 180
                        else phase[:1] & 0 | 1 if phase < 181 else phase[:1] & 0 | 0
                    )
                )
            )
        )
        if phase < 183
        else (
            (
                (
                    (
                        phase[:1] & 0 | 1
                        if phase < 184
                        else phase[:1] & 0 | 0 if phase < 186 else phase[:1] & 0 | 1
                    )
                    if phase < 187
                    else (
                        phase[:1] & 0 | 0
                        if phase < 189
                        else phase[:1] & 0 | 1 if phase < 190 else phase[:1] & 0 | 0
                    )
                )
                if phase < 192
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 193
                        else phase[:1] & 0 | 0 if phase < 195 else phase[:1] & 0 | 1
                    )
                    if phase < 196
                    else (
                        phase[:1] & 0 | 0
                        if phase < 198
                        else phase[:1] & 0 | 1 if phase < 199 else phase[:1] & 0 | 0
                    )
                )
            )
            if phase < 201
            else (
                (
                    (
                        phase[:1] & 0 | 1
                        if phase < 202
                        else phase[:1] & 0 | 0 if phase < 204 else phase[:1] & 0 | 1
                    )
                    if phase < 205
                    else (
                        phase[:1] & 0 | 0
                        if phase < 207
                        else phase[:1] & 0 | 1 if phase < 208 else phase[:1] & 0 | 0
                    )
                )
                if phase < 210
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 211
                        else phase[:1] & 0 | 0 if phase < 213 else phase[:1] & 0 | 1
                    )
                    if phase < 561
                    else (
                        (phase[:1] & 0 | 0 if phase < 563 else phase[:1] & 0 | 1)
                        if phase < 564
                        else phase[:1] & 0 | 0 if phase < 566 else phase[:1] & 0 | 1
                    )
                )
            )
        )
    )
    expected_seed_valid = (
        (
            (
                (
                    phase[:1] & 0 | 0
                    if phase < 6
                    else phase[:1] & 0 | 1 if phase < 7 else phase[:1] & 0 | 0
                )
                if phase < 26
                else (
                    (phase[:1] & 0 | 1 if phase < 27 else phase[:1] & 0 | 0)
                    if phase < 485
                    else phase[:1] & 0 | 1 if phase < 496 else phase[:1] & 0 | 0
                )
            )
            if phase < 499
            else (
                (
                    (phase[:1] & 0 | 1 if phase < 500 else phase[:1] & 0 | 0)
                    if phase < 503
                    else phase[:1] & 0 | 1 if phase < 504 else phase[:1] & 0 | 0
                )
                if phase < 507
                else (
                    (phase[:1] & 0 | 1 if phase < 508 else phase[:1] & 0 | 0)
                    if phase < 511
                    else phase[:1] & 0 | 1 if phase < 512 else phase[:1] & 0 | 0
                )
            )
        )
        if phase < 515
        else (
            (
                (
                    (phase[:1] & 0 | 1 if phase < 516 else phase[:1] & 0 | 0)
                    if phase < 519
                    else phase[:1] & 0 | 1 if phase < 520 else phase[:1] & 0 | 0
                )
                if phase < 523
                else (
                    (phase[:1] & 0 | 1 if phase < 524 else phase[:1] & 0 | 0)
                    if phase < 527
                    else phase[:1] & 0 | 1 if phase < 528 else phase[:1] & 0 | 0
                )
            )
            if phase < 531
            else (
                (
                    (phase[:1] & 0 | 1 if phase < 532 else phase[:1] & 0 | 0)
                    if phase < 535
                    else phase[:1] & 0 | 1 if phase < 536 else phase[:1] & 0 | 0
                )
                if phase < 539
                else (
                    (phase[:1] & 0 | 1 if phase < 540 else phase[:1] & 0 | 0)
                    if phase < 545
                    else phase[:1] & 0 | 1 if phase < 546 else phase[:1] & 0 | 0
                )
            )
        )
    )
    expected_copy_valid = (
        (
            (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 15
                        else phase[:1] & 0 | 1 if phase < 16 else phase[:1] & 0 | 0
                    )
                    if phase < 35
                    else (
                        phase[:1] & 0 | 1
                        if phase < 36
                        else phase[:1] & 0 | 0 if phase < 221 else phase[:1] & 0 | 1
                    )
                )
                if phase < 416
                else (
                    (
                        phase[:1] & 0 | 0
                        if phase < 418
                        else phase[:1] & 0 | 1 if phase < 419 else phase[:1] & 0 | 0
                    )
                    if phase < 421
                    else (
                        phase[:1] & 0 | 1
                        if phase < 422
                        else phase[:1] & 0 | 0 if phase < 424 else phase[:1] & 0 | 1
                    )
                )
            )
            if phase < 425
            else (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 427
                        else phase[:1] & 0 | 1 if phase < 428 else phase[:1] & 0 | 0
                    )
                    if phase < 430
                    else (
                        phase[:1] & 0 | 1
                        if phase < 431
                        else phase[:1] & 0 | 0 if phase < 433 else phase[:1] & 0 | 1
                    )
                )
                if phase < 434
                else (
                    (
                        phase[:1] & 0 | 0
                        if phase < 436
                        else phase[:1] & 0 | 1 if phase < 437 else phase[:1] & 0 | 0
                    )
                    if phase < 440
                    else (
                        (phase[:1] & 0 | 1 if phase < 441 else phase[:1] & 0 | 0)
                        if phase < 444
                        else phase[:1] & 0 | 1 if phase < 445 else phase[:1] & 0 | 0
                    )
                )
            )
        )
        if phase < 448
        else (
            (
                (
                    (
                        phase[:1] & 0 | 1
                        if phase < 449
                        else phase[:1] & 0 | 0 if phase < 452 else phase[:1] & 0 | 1
                    )
                    if phase < 453
                    else (
                        phase[:1] & 0 | 0
                        if phase < 456
                        else phase[:1] & 0 | 1 if phase < 457 else phase[:1] & 0 | 0
                    )
                )
                if phase < 460
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 461
                        else phase[:1] & 0 | 0 if phase < 464 else phase[:1] & 0 | 1
                    )
                    if phase < 465
                    else (
                        (phase[:1] & 0 | 0 if phase < 468 else phase[:1] & 0 | 1)
                        if phase < 469
                        else phase[:1] & 0 | 0 if phase < 472 else phase[:1] & 0 | 1
                    )
                )
            )
            if phase < 473
            else (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 476
                        else phase[:1] & 0 | 1 if phase < 477 else phase[:1] & 0 | 0
                    )
                    if phase < 480
                    else (
                        phase[:1] & 0 | 1
                        if phase < 481
                        else phase[:1] & 0 | 0 if phase < 484 else phase[:1] & 0 | 1
                    )
                )
                if phase < 485
                else (
                    (
                        phase[:1] & 0 | 0
                        if phase < 554
                        else phase[:1] & 0 | 1 if phase < 555 else phase[:1] & 0 | 0
                    )
                    if phase < 574
                    else (
                        (phase[:1] & 0 | 1 if phase < 575 else phase[:1] & 0 | 0)
                        if phase < 577
                        else phase[:1] & 0 | 1 if phase < 578 else phase[:1] & 0 | 0
                    )
                )
            )
        )
    )
    expected_check_valid = (
        (
            (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 20
                        else phase[:1] & 0 | 1 if phase < 21 else phase[:1] & 0 | 0
                    )
                    if phase < 40
                    else (
                        (phase[:1] & 0 | 1 if phase < 41 else phase[:1] & 0 | 0)
                        if phase < 45
                        else phase[:1] & 0 | 1 if phase < 156 else phase[:1] & 0 | 0
                    )
                )
                if phase < 158
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 159
                        else phase[:1] & 0 | 0 if phase < 161 else phase[:1] & 0 | 1
                    )
                    if phase < 162
                    else (
                        (phase[:1] & 0 | 0 if phase < 164 else phase[:1] & 0 | 1)
                        if phase < 165
                        else phase[:1] & 0 | 0 if phase < 167 else phase[:1] & 0 | 1
                    )
                )
            )
            if phase < 168
            else (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 170
                        else phase[:1] & 0 | 1 if phase < 171 else phase[:1] & 0 | 0
                    )
                    if phase < 173
                    else (
                        (phase[:1] & 0 | 1 if phase < 174 else phase[:1] & 0 | 0)
                        if phase < 176
                        else phase[:1] & 0 | 1 if phase < 177 else phase[:1] & 0 | 0
                    )
                )
                if phase < 179
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 180
                        else phase[:1] & 0 | 0 if phase < 182 else phase[:1] & 0 | 1
                    )
                    if phase < 183
                    else (
                        (phase[:1] & 0 | 0 if phase < 185 else phase[:1] & 0 | 1)
                        if phase < 186
                        else phase[:1] & 0 | 0 if phase < 188 else phase[:1] & 0 | 1
                    )
                )
            )
        )
        if phase < 189
        else (
            (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 191
                        else phase[:1] & 0 | 1 if phase < 192 else phase[:1] & 0 | 0
                    )
                    if phase < 194
                    else (
                        (phase[:1] & 0 | 1 if phase < 195 else phase[:1] & 0 | 0)
                        if phase < 197
                        else phase[:1] & 0 | 1 if phase < 198 else phase[:1] & 0 | 0
                    )
                )
                if phase < 200
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 201
                        else phase[:1] & 0 | 0 if phase < 203 else phase[:1] & 0 | 1
                    )
                    if phase < 204
                    else (
                        (phase[:1] & 0 | 0 if phase < 206 else phase[:1] & 0 | 1)
                        if phase < 207
                        else phase[:1] & 0 | 0 if phase < 209 else phase[:1] & 0 | 1
                    )
                )
            )
            if phase < 210
            else (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 212
                        else phase[:1] & 0 | 1 if phase < 213 else phase[:1] & 0 | 0
                    )
                    if phase < 215
                    else (
                        (phase[:1] & 0 | 1 if phase < 216 else phase[:1] & 0 | 0)
                        if phase < 218
                        else phase[:1] & 0 | 1 if phase < 219 else phase[:1] & 0 | 0
                    )
                )
                if phase < 557
                else (
                    (
                        (phase[:1] & 0 | 1 if phase < 558 else phase[:1] & 0 | 0)
                        if phase < 562
                        else phase[:1] & 0 | 1 if phase < 569 else phase[:1] & 0 | 0
                    )
                    if phase < 571
                    else (
                        (phase[:1] & 0 | 1 if phase < 572 else phase[:1] & 0 | 0)
                        if phase < 580
                        else phase[:1] & 0 | 1 if phase < 581 else phase[:1] & 0 | 0
                    )
                )
            )
        )
    )
    expected_dram_accepted = (
        (
            (
                (
                    (
                        (phase[:2] & 0 | 0 if phase < 2 else phase[:2] & 0 | 2)
                        if phase < 3
                        else (
                            phase[:2] & 0 | 0
                            if phase < 8
                            else phase[:2] & 0 | 1 if phase < 9 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 22
                    else (
                        (
                            phase[:2] & 0 | 2
                            if phase < 23
                            else phase[:2] & 0 | 0 if phase < 28 else phase[:2] & 0 | 1
                        )
                        if phase < 29
                        else (
                            phase[:2] & 0 | 0
                            if phase < 42
                            else phase[:2] & 0 | 1 if phase < 43 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 46
                else (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 47
                            else phase[:2] & 0 | 0 if phase < 50 else phase[:2] & 0 | 1
                        )
                        if phase < 51
                        else (
                            phase[:2] & 0 | 0
                            if phase < 220
                            else phase[:2] & 0 | 1 if phase < 221 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 224
                    else (
                        (
                            phase[:2] & 0 | 1
                            if phase < 225
                            else phase[:2] & 0 | 0 if phase < 228 else phase[:2] & 0 | 1
                        )
                        if phase < 229
                        else (
                            phase[:2] & 0 | 0
                            if phase < 417
                            else phase[:2] & 0 | 1 if phase < 418 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
            if phase < 421
            else (
                (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 422
                            else phase[:2] & 0 | 0 if phase < 425 else phase[:2] & 0 | 1
                        )
                        if phase < 426
                        else (
                            phase[:2] & 0 | 0
                            if phase < 429
                            else phase[:2] & 0 | 1 if phase < 430 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 433
                    else (
                        (
                            phase[:2] & 0 | 1
                            if phase < 434
                            else phase[:2] & 0 | 0 if phase < 437 else phase[:2] & 0 | 1
                        )
                        if phase < 438
                        else (
                            phase[:2] & 0 | 0
                            if phase < 441
                            else phase[:2] & 0 | 1 if phase < 442 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 445
                else (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 446
                            else phase[:2] & 0 | 0 if phase < 449 else phase[:2] & 0 | 1
                        )
                        if phase < 450
                        else (
                            phase[:2] & 0 | 0
                            if phase < 453
                            else phase[:2] & 0 | 1 if phase < 454 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 457
                    else (
                        (
                            phase[:2] & 0 | 1
                            if phase < 458
                            else phase[:2] & 0 | 0 if phase < 461 else phase[:2] & 0 | 1
                        )
                        if phase < 462
                        else (
                            phase[:2] & 0 | 0
                            if phase < 465
                            else phase[:2] & 0 | 1 if phase < 466 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
        )
        if phase < 469
        else (
            (
                (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 470
                            else phase[:2] & 0 | 0 if phase < 473 else phase[:2] & 0 | 1
                        )
                        if phase < 474
                        else (
                            phase[:2] & 0 | 0
                            if phase < 477
                            else phase[:2] & 0 | 1 if phase < 478 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 481
                    else (
                        (
                            phase[:2] & 0 | 2
                            if phase < 482
                            else phase[:2] & 0 | 0 if phase < 485 else phase[:2] & 0 | 2
                        )
                        if phase < 486
                        else (
                            phase[:2] & 0 | 0
                            if phase < 489
                            else phase[:2] & 0 | 2 if phase < 490 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 495
                else (
                    (
                        (
                            phase[:2] & 0 | 2
                            if phase < 496
                            else phase[:2] & 0 | 0 if phase < 499 else phase[:2] & 0 | 2
                        )
                        if phase < 500
                        else (
                            phase[:2] & 0 | 0
                            if phase < 503
                            else phase[:2] & 0 | 2 if phase < 504 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 507
                    else (
                        (
                            phase[:2] & 0 | 2
                            if phase < 508
                            else phase[:2] & 0 | 0 if phase < 511 else phase[:2] & 0 | 2
                        )
                        if phase < 512
                        else (
                            phase[:2] & 0 | 0
                            if phase < 515
                            else phase[:2] & 0 | 2 if phase < 516 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
            if phase < 519
            else (
                (
                    (
                        (
                            phase[:2] & 0 | 2
                            if phase < 520
                            else phase[:2] & 0 | 0 if phase < 523 else phase[:2] & 0 | 2
                        )
                        if phase < 524
                        else (
                            phase[:2] & 0 | 0
                            if phase < 527
                            else phase[:2] & 0 | 2 if phase < 528 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 531
                    else (
                        (
                            phase[:2] & 0 | 2
                            if phase < 532
                            else phase[:2] & 0 | 0 if phase < 535 else phase[:2] & 0 | 2
                        )
                        if phase < 536
                        else (
                            phase[:2] & 0 | 0
                            if phase < 541
                            else phase[:2] & 0 | 2 if phase < 542 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 547
                else (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 548
                            else phase[:2] & 0 | 0 if phase < 559 else phase[:2] & 0 | 1
                        )
                        if phase < 560
                        else (
                            phase[:2] & 0 | 0
                            if phase < 563
                            else phase[:2] & 0 | 1 if phase < 564 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 567
                    else (
                        (
                            phase[:2] & 0 | 1
                            if phase < 568
                            else phase[:2] & 0 | 0 if phase < 573 else phase[:2] & 0 | 1
                        )
                        if phase < 574
                        else (
                            phase[:2] & 0 | 0
                            if phase < 577
                            else phase[:2] & 0 | 1 if phase < 578 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
        )
    )
    expected_dram_enqueued = (
        (
            (
                (
                    (
                        (phase[:2] & 0 | 0 if phase < 5 else phase[:2] & 0 | 2)
                        if phase < 6
                        else (
                            phase[:2] & 0 | 0
                            if phase < 11
                            else phase[:2] & 0 | 1 if phase < 12 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 25
                    else (
                        (
                            phase[:2] & 0 | 2
                            if phase < 26
                            else phase[:2] & 0 | 0 if phase < 31 else phase[:2] & 0 | 1
                        )
                        if phase < 32
                        else (
                            phase[:2] & 0 | 0
                            if phase < 45
                            else phase[:2] & 0 | 1 if phase < 46 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 49
                else (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 50
                            else phase[:2] & 0 | 0 if phase < 219 else phase[:2] & 0 | 1
                        )
                        if phase < 220
                        else (
                            phase[:2] & 0 | 0
                            if phase < 223
                            else phase[:2] & 0 | 1 if phase < 224 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 227
                    else (
                        (
                            phase[:2] & 0 | 1
                            if phase < 228
                            else phase[:2] & 0 | 0 if phase < 416 else phase[:2] & 0 | 1
                        )
                        if phase < 417
                        else (
                            phase[:2] & 0 | 0
                            if phase < 420
                            else phase[:2] & 0 | 1 if phase < 421 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
            if phase < 424
            else (
                (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 425
                            else phase[:2] & 0 | 0 if phase < 428 else phase[:2] & 0 | 1
                        )
                        if phase < 429
                        else (
                            phase[:2] & 0 | 0
                            if phase < 432
                            else phase[:2] & 0 | 1 if phase < 433 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 436
                    else (
                        (
                            phase[:2] & 0 | 1
                            if phase < 437
                            else phase[:2] & 0 | 0 if phase < 440 else phase[:2] & 0 | 1
                        )
                        if phase < 441
                        else (
                            phase[:2] & 0 | 0
                            if phase < 444
                            else phase[:2] & 0 | 1 if phase < 445 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 448
                else (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 449
                            else phase[:2] & 0 | 0 if phase < 452 else phase[:2] & 0 | 1
                        )
                        if phase < 453
                        else (
                            phase[:2] & 0 | 0
                            if phase < 456
                            else phase[:2] & 0 | 1 if phase < 457 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 460
                    else (
                        (
                            phase[:2] & 0 | 1
                            if phase < 461
                            else phase[:2] & 0 | 0 if phase < 464 else phase[:2] & 0 | 1
                        )
                        if phase < 465
                        else (
                            phase[:2] & 0 | 0
                            if phase < 468
                            else phase[:2] & 0 | 1 if phase < 469 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
        )
        if phase < 472
        else (
            (
                (
                    (
                        (phase[:2] & 0 | 1 if phase < 473 else phase[:2] & 0 | 0)
                        if phase < 476
                        else (
                            phase[:2] & 0 | 1
                            if phase < 477
                            else phase[:2] & 0 | 0 if phase < 480 else phase[:2] & 0 | 1
                        )
                    )
                    if phase < 481
                    else (
                        (
                            phase[:2] & 0 | 0
                            if phase < 484
                            else phase[:2] & 0 | 2 if phase < 485 else phase[:2] & 0 | 0
                        )
                        if phase < 488
                        else (
                            phase[:2] & 0 | 2
                            if phase < 489
                            else phase[:2] & 0 | 0 if phase < 494 else phase[:2] & 0 | 2
                        )
                    )
                )
                if phase < 495
                else (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 498
                            else phase[:2] & 0 | 2 if phase < 499 else phase[:2] & 0 | 0
                        )
                        if phase < 502
                        else (
                            phase[:2] & 0 | 2
                            if phase < 503
                            else phase[:2] & 0 | 0 if phase < 506 else phase[:2] & 0 | 2
                        )
                    )
                    if phase < 507
                    else (
                        (
                            phase[:2] & 0 | 0
                            if phase < 510
                            else phase[:2] & 0 | 2 if phase < 511 else phase[:2] & 0 | 0
                        )
                        if phase < 514
                        else (
                            phase[:2] & 0 | 2
                            if phase < 515
                            else phase[:2] & 0 | 0 if phase < 518 else phase[:2] & 0 | 2
                        )
                    )
                )
            )
            if phase < 519
            else (
                (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 522
                            else phase[:2] & 0 | 2 if phase < 523 else phase[:2] & 0 | 0
                        )
                        if phase < 526
                        else (
                            phase[:2] & 0 | 2
                            if phase < 527
                            else phase[:2] & 0 | 0 if phase < 530 else phase[:2] & 0 | 2
                        )
                    )
                    if phase < 531
                    else (
                        (
                            phase[:2] & 0 | 0
                            if phase < 534
                            else phase[:2] & 0 | 2 if phase < 535 else phase[:2] & 0 | 0
                        )
                        if phase < 538
                        else (
                            phase[:2] & 0 | 2
                            if phase < 539
                            else phase[:2] & 0 | 0 if phase < 544 else phase[:2] & 0 | 2
                        )
                    )
                )
                if phase < 545
                else (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 550
                            else phase[:2] & 0 | 1 if phase < 551 else phase[:2] & 0 | 0
                        )
                        if phase < 562
                        else (
                            phase[:2] & 0 | 1
                            if phase < 563
                            else phase[:2] & 0 | 0 if phase < 566 else phase[:2] & 0 | 1
                        )
                    )
                    if phase < 567
                    else (
                        (
                            phase[:2] & 0 | 0
                            if phase < 572
                            else phase[:2] & 0 | 1 if phase < 573 else phase[:2] & 0 | 0
                        )
                        if phase < 576
                        else (
                            phase[:2] & 0 | 1
                            if phase < 577
                            else phase[:2] & 0 | 0 if phase < 581 else phase[:2] & 0 | 1
                        )
                    )
                )
            )
        )
    )
    expected_sram_accepted = (
        (
            (
                (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 12
                            else phase[:2] & 0 | 2 if phase < 13 else phase[:2] & 0 | 0
                        )
                        if phase < 17
                        else (
                            (phase[:2] & 0 | 1 if phase < 18 else phase[:2] & 0 | 0)
                            if phase < 32
                            else phase[:2] & 0 | 2 if phase < 33 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 37
                    else (
                        (
                            (phase[:2] & 0 | 1 if phase < 38 else phase[:2] & 0 | 0)
                            if phase < 42
                            else phase[:2] & 0 | 1 if phase < 43 else phase[:2] & 0 | 0
                        )
                        if phase < 45
                        else (
                            (phase[:2] & 0 | 1 if phase < 46 else phase[:2] & 0 | 0)
                            if phase < 48
                            else phase[:2] & 0 | 1 if phase < 49 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 155
                else (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 156
                            else phase[:2] & 0 | 0 if phase < 158 else phase[:2] & 0 | 1
                        )
                        if phase < 159
                        else (
                            (phase[:2] & 0 | 0 if phase < 161 else phase[:2] & 0 | 1)
                            if phase < 162
                            else phase[:2] & 0 | 0 if phase < 164 else phase[:2] & 0 | 1
                        )
                    )
                    if phase < 165
                    else (
                        (
                            (phase[:2] & 0 | 0 if phase < 167 else phase[:2] & 0 | 1)
                            if phase < 168
                            else phase[:2] & 0 | 0 if phase < 170 else phase[:2] & 0 | 1
                        )
                        if phase < 171
                        else (
                            (phase[:2] & 0 | 0 if phase < 173 else phase[:2] & 0 | 1)
                            if phase < 174
                            else phase[:2] & 0 | 0 if phase < 176 else phase[:2] & 0 | 1
                        )
                    )
                )
            )
            if phase < 177
            else (
                (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 179
                            else phase[:2] & 0 | 1 if phase < 180 else phase[:2] & 0 | 0
                        )
                        if phase < 182
                        else (
                            (phase[:2] & 0 | 1 if phase < 183 else phase[:2] & 0 | 0)
                            if phase < 185
                            else phase[:2] & 0 | 1 if phase < 186 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 188
                    else (
                        (
                            (phase[:2] & 0 | 1 if phase < 189 else phase[:2] & 0 | 0)
                            if phase < 191
                            else phase[:2] & 0 | 1 if phase < 192 else phase[:2] & 0 | 0
                        )
                        if phase < 194
                        else (
                            (phase[:2] & 0 | 1 if phase < 195 else phase[:2] & 0 | 0)
                            if phase < 197
                            else phase[:2] & 0 | 1 if phase < 198 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 200
                else (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 201
                            else phase[:2] & 0 | 0 if phase < 203 else phase[:2] & 0 | 1
                        )
                        if phase < 204
                        else (
                            (phase[:2] & 0 | 0 if phase < 206 else phase[:2] & 0 | 1)
                            if phase < 207
                            else phase[:2] & 0 | 0 if phase < 209 else phase[:2] & 0 | 1
                        )
                    )
                    if phase < 210
                    else (
                        (
                            (phase[:2] & 0 | 0 if phase < 212 else phase[:2] & 0 | 1)
                            if phase < 213
                            else phase[:2] & 0 | 0 if phase < 215 else phase[:2] & 0 | 1
                        )
                        if phase < 216
                        else (
                            (phase[:2] & 0 | 0 if phase < 218 else phase[:2] & 0 | 2)
                            if phase < 219
                            else phase[:2] & 0 | 0 if phase < 221 else phase[:2] & 0 | 2
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
                            phase[:2] & 0 | 0
                            if phase < 224
                            else phase[:2] & 0 | 2 if phase < 225 else phase[:2] & 0 | 0
                        )
                        if phase < 415
                        else (
                            (phase[:2] & 0 | 2 if phase < 416 else phase[:2] & 0 | 0)
                            if phase < 418
                            else phase[:2] & 0 | 2 if phase < 419 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 421
                    else (
                        (
                            (phase[:2] & 0 | 2 if phase < 422 else phase[:2] & 0 | 0)
                            if phase < 424
                            else phase[:2] & 0 | 2 if phase < 425 else phase[:2] & 0 | 0
                        )
                        if phase < 427
                        else (
                            (phase[:2] & 0 | 2 if phase < 428 else phase[:2] & 0 | 0)
                            if phase < 430
                            else phase[:2] & 0 | 2 if phase < 431 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 433
                else (
                    (
                        (
                            phase[:2] & 0 | 2
                            if phase < 434
                            else phase[:2] & 0 | 0 if phase < 437 else phase[:2] & 0 | 2
                        )
                        if phase < 438
                        else (
                            (phase[:2] & 0 | 0 if phase < 441 else phase[:2] & 0 | 2)
                            if phase < 442
                            else phase[:2] & 0 | 0 if phase < 445 else phase[:2] & 0 | 2
                        )
                    )
                    if phase < 446
                    else (
                        (
                            (phase[:2] & 0 | 0 if phase < 449 else phase[:2] & 0 | 2)
                            if phase < 450
                            else phase[:2] & 0 | 0 if phase < 453 else phase[:2] & 0 | 2
                        )
                        if phase < 454
                        else (
                            (phase[:2] & 0 | 0 if phase < 457 else phase[:2] & 0 | 2)
                            if phase < 458
                            else phase[:2] & 0 | 0 if phase < 461 else phase[:2] & 0 | 2
                        )
                    )
                )
            )
            if phase < 462
            else (
                (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 465
                            else phase[:2] & 0 | 2 if phase < 466 else phase[:2] & 0 | 0
                        )
                        if phase < 469
                        else (
                            (phase[:2] & 0 | 2 if phase < 470 else phase[:2] & 0 | 0)
                            if phase < 473
                            else phase[:2] & 0 | 2 if phase < 474 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 477
                    else (
                        (
                            (phase[:2] & 0 | 2 if phase < 478 else phase[:2] & 0 | 0)
                            if phase < 481
                            else phase[:2] & 0 | 2 if phase < 482 else phase[:2] & 0 | 0
                        )
                        if phase < 551
                        else (
                            (phase[:2] & 0 | 2 if phase < 552 else phase[:2] & 0 | 0)
                            if phase < 554
                            else phase[:2] & 0 | 1 if phase < 555 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 559
                else (
                    (
                        (
                            (phase[:2] & 0 | 1 if phase < 560 else phase[:2] & 0 | 0)
                            if phase < 562
                            else phase[:2] & 0 | 1 if phase < 563 else phase[:2] & 0 | 0
                        )
                        if phase < 565
                        else (
                            (phase[:2] & 0 | 1 if phase < 566 else phase[:2] & 0 | 0)
                            if phase < 568
                            else phase[:2] & 0 | 1 if phase < 569 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 571
                    else (
                        (
                            (phase[:2] & 0 | 2 if phase < 572 else phase[:2] & 0 | 0)
                            if phase < 574
                            else phase[:2] & 0 | 2 if phase < 575 else phase[:2] & 0 | 0
                        )
                        if phase < 577
                        else (
                            (phase[:2] & 0 | 1 if phase < 578 else phase[:2] & 0 | 0)
                            if phase < 580
                            else phase[:2] & 0 | 2 if phase < 581 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
        )
    )
    expected_sram_enqueued = (
        (
            (
                (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 14
                            else phase[:2] & 0 | 2 if phase < 15 else phase[:2] & 0 | 0
                        )
                        if phase < 19
                        else (
                            (phase[:2] & 0 | 1 if phase < 20 else phase[:2] & 0 | 0)
                            if phase < 34
                            else phase[:2] & 0 | 2 if phase < 35 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 39
                    else (
                        (
                            phase[:2] & 0 | 1
                            if phase < 40
                            else phase[:2] & 0 | 0 if phase < 44 else phase[:2] & 0 | 1
                        )
                        if phase < 45
                        else (
                            (phase[:2] & 0 | 0 if phase < 47 else phase[:2] & 0 | 1)
                            if phase < 48
                            else phase[:2] & 0 | 0 if phase < 154 else phase[:2] & 0 | 1
                        )
                    )
                )
                if phase < 155
                else (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 157
                            else phase[:2] & 0 | 1 if phase < 158 else phase[:2] & 0 | 0
                        )
                        if phase < 160
                        else (
                            (phase[:2] & 0 | 1 if phase < 161 else phase[:2] & 0 | 0)
                            if phase < 163
                            else phase[:2] & 0 | 1 if phase < 164 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 166
                    else (
                        (
                            (phase[:2] & 0 | 1 if phase < 167 else phase[:2] & 0 | 0)
                            if phase < 169
                            else phase[:2] & 0 | 1 if phase < 170 else phase[:2] & 0 | 0
                        )
                        if phase < 172
                        else (
                            (phase[:2] & 0 | 1 if phase < 173 else phase[:2] & 0 | 0)
                            if phase < 175
                            else phase[:2] & 0 | 1 if phase < 176 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
            if phase < 178
            else (
                (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 179
                            else phase[:2] & 0 | 0 if phase < 181 else phase[:2] & 0 | 1
                        )
                        if phase < 182
                        else (
                            (phase[:2] & 0 | 0 if phase < 184 else phase[:2] & 0 | 1)
                            if phase < 185
                            else phase[:2] & 0 | 0 if phase < 187 else phase[:2] & 0 | 1
                        )
                    )
                    if phase < 188
                    else (
                        (
                            (phase[:2] & 0 | 0 if phase < 190 else phase[:2] & 0 | 1)
                            if phase < 191
                            else phase[:2] & 0 | 0 if phase < 193 else phase[:2] & 0 | 1
                        )
                        if phase < 194
                        else (
                            (phase[:2] & 0 | 0 if phase < 196 else phase[:2] & 0 | 1)
                            if phase < 197
                            else phase[:2] & 0 | 0 if phase < 199 else phase[:2] & 0 | 1
                        )
                    )
                )
                if phase < 200
                else (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 202
                            else phase[:2] & 0 | 1 if phase < 203 else phase[:2] & 0 | 0
                        )
                        if phase < 205
                        else (
                            (phase[:2] & 0 | 1 if phase < 206 else phase[:2] & 0 | 0)
                            if phase < 208
                            else phase[:2] & 0 | 1 if phase < 209 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 211
                    else (
                        (
                            (phase[:2] & 0 | 1 if phase < 212 else phase[:2] & 0 | 0)
                            if phase < 214
                            else phase[:2] & 0 | 1 if phase < 215 else phase[:2] & 0 | 0
                        )
                        if phase < 217
                        else (
                            (phase[:2] & 0 | 1 if phase < 218 else phase[:2] & 0 | 0)
                            if phase < 220
                            else phase[:2] & 0 | 2 if phase < 221 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
        )
        if phase < 223
        else (
            (
                (
                    (
                        (
                            phase[:2] & 0 | 2
                            if phase < 224
                            else phase[:2] & 0 | 0 if phase < 414 else phase[:2] & 0 | 2
                        )
                        if phase < 415
                        else (
                            (phase[:2] & 0 | 0 if phase < 417 else phase[:2] & 0 | 2)
                            if phase < 418
                            else phase[:2] & 0 | 0 if phase < 420 else phase[:2] & 0 | 2
                        )
                    )
                    if phase < 421
                    else (
                        (
                            (phase[:2] & 0 | 0 if phase < 423 else phase[:2] & 0 | 2)
                            if phase < 424
                            else phase[:2] & 0 | 0 if phase < 426 else phase[:2] & 0 | 2
                        )
                        if phase < 427
                        else (
                            (phase[:2] & 0 | 0 if phase < 429 else phase[:2] & 0 | 2)
                            if phase < 430
                            else phase[:2] & 0 | 0 if phase < 432 else phase[:2] & 0 | 2
                        )
                    )
                )
                if phase < 433
                else (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 435
                            else phase[:2] & 0 | 2 if phase < 436 else phase[:2] & 0 | 0
                        )
                        if phase < 439
                        else (
                            (phase[:2] & 0 | 2 if phase < 440 else phase[:2] & 0 | 0)
                            if phase < 443
                            else phase[:2] & 0 | 2 if phase < 444 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 447
                    else (
                        (
                            (phase[:2] & 0 | 2 if phase < 448 else phase[:2] & 0 | 0)
                            if phase < 451
                            else phase[:2] & 0 | 2 if phase < 452 else phase[:2] & 0 | 0
                        )
                        if phase < 455
                        else (
                            (phase[:2] & 0 | 2 if phase < 456 else phase[:2] & 0 | 0)
                            if phase < 459
                            else phase[:2] & 0 | 2 if phase < 460 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
            if phase < 463
            else (
                (
                    (
                        (
                            phase[:2] & 0 | 2
                            if phase < 464
                            else phase[:2] & 0 | 0 if phase < 467 else phase[:2] & 0 | 2
                        )
                        if phase < 468
                        else (
                            (phase[:2] & 0 | 0 if phase < 471 else phase[:2] & 0 | 2)
                            if phase < 472
                            else phase[:2] & 0 | 0 if phase < 475 else phase[:2] & 0 | 2
                        )
                    )
                    if phase < 476
                    else (
                        (
                            (phase[:2] & 0 | 0 if phase < 479 else phase[:2] & 0 | 2)
                            if phase < 480
                            else phase[:2] & 0 | 0 if phase < 483 else phase[:2] & 0 | 2
                        )
                        if phase < 484
                        else (
                            (phase[:2] & 0 | 0 if phase < 553 else phase[:2] & 0 | 2)
                            if phase < 554
                            else phase[:2] & 0 | 0 if phase < 556 else phase[:2] & 0 | 1
                        )
                    )
                )
                if phase < 557
                else (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 561
                            else phase[:2] & 0 | 1 if phase < 562 else phase[:2] & 0 | 0
                        )
                        if phase < 564
                        else (
                            (phase[:2] & 0 | 1 if phase < 565 else phase[:2] & 0 | 0)
                            if phase < 567
                            else phase[:2] & 0 | 1 if phase < 568 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 570
                    else (
                        (
                            (phase[:2] & 0 | 1 if phase < 571 else phase[:2] & 0 | 0)
                            if phase < 573
                            else phase[:2] & 0 | 2 if phase < 574 else phase[:2] & 0 | 0
                        )
                        if phase < 576
                        else (
                            (phase[:2] & 0 | 2 if phase < 577 else phase[:2] & 0 | 0)
                            if phase < 579
                            else phase[:2] & 0 | 1 if phase < 580 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
        )
    )
    expected_seed_response_dram_address = (
        (
            (
                (
                    (phase[:4] & 0 | 0 if phase < 6 else phase[:4] & 0 | 5)
                    if phase < 7
                    else phase[:4] & 0 | 0 if phase < 26 else phase[:4] & 0 | 9
                )
                if phase < 27
                else (
                    (phase[:4] & 0 | 0 if phase < 494 else phase[:4] & 0 | 1)
                    if phase < 495
                    else phase[:4] & 0 | 2 if phase < 496 else phase[:4] & 0 | 0
                )
            )
            if phase < 499
            else (
                (
                    (phase[:4] & 0 | 3 if phase < 500 else phase[:4] & 0 | 0)
                    if phase < 503
                    else phase[:4] & 0 | 4 if phase < 504 else phase[:4] & 0 | 0
                )
                if phase < 507
                else (
                    (phase[:4] & 0 | 5 if phase < 508 else phase[:4] & 0 | 0)
                    if phase < 511
                    else phase[:4] & 0 | 6 if phase < 512 else phase[:4] & 0 | 0
                )
            )
        )
        if phase < 515
        else (
            (
                (
                    (phase[:4] & 0 | 7 if phase < 516 else phase[:4] & 0 | 0)
                    if phase < 519
                    else phase[:4] & 0 | 8 if phase < 520 else phase[:4] & 0 | 0
                )
                if phase < 523
                else (
                    (phase[:4] & 0 | 9 if phase < 524 else phase[:4] & 0 | 0)
                    if phase < 527
                    else phase[:4] & 0 | 10 if phase < 528 else phase[:4] & 0 | 0
                )
            )
            if phase < 531
            else (
                (
                    (phase[:4] & 0 | 11 if phase < 532 else phase[:4] & 0 | 0)
                    if phase < 535
                    else phase[:4] & 0 | 12 if phase < 536 else phase[:4] & 0 | 0
                )
                if phase < 539
                else (
                    (phase[:4] & 0 | 13 if phase < 540 else phase[:4] & 0 | 0)
                    if phase < 545
                    else phase[:4] & 0 | 14 if phase < 546 else phase[:4] & 0 | 0
                )
            )
        )
    )
    expected_seed_response_sram_address = (
        (
            (
                (
                    phase[:4] & 0 | 0
                    if phase < 6
                    else phase[:4] & 0 | 3 if phase < 7 else phase[:4] & 0 | 0
                )
                if phase < 26
                else (
                    (phase[:4] & 0 | 1 if phase < 27 else phase[:4] & 0 | 0)
                    if phase < 485
                    else phase[:4] & 0 | 5 if phase < 494 else phase[:4] & 0 | 6
                )
            )
            if phase < 495
            else (
                (
                    (phase[:4] & 0 | 7 if phase < 496 else phase[:4] & 0 | 0)
                    if phase < 499
                    else phase[:4] & 0 | 8 if phase < 500 else phase[:4] & 0 | 0
                )
                if phase < 503
                else (
                    (phase[:4] & 0 | 9 if phase < 504 else phase[:4] & 0 | 0)
                    if phase < 507
                    else phase[:4] & 0 | 10 if phase < 508 else phase[:4] & 0 | 0
                )
            )
        )
        if phase < 511
        else (
            (
                (
                    (phase[:4] & 0 | 11 if phase < 512 else phase[:4] & 0 | 0)
                    if phase < 515
                    else phase[:4] & 0 | 12 if phase < 516 else phase[:4] & 0 | 0
                )
                if phase < 519
                else (
                    (phase[:4] & 0 | 13 if phase < 520 else phase[:4] & 0 | 0)
                    if phase < 523
                    else phase[:4] & 0 | 14 if phase < 524 else phase[:4] & 0 | 0
                )
            )
            if phase < 527
            else (
                (
                    (phase[:4] & 0 | 15 if phase < 528 else phase[:4] & 0 | 0)
                    if phase < 535
                    else phase[:4] & 0 | 1 if phase < 536 else phase[:4] & 0 | 0
                )
                if phase < 539
                else (
                    (phase[:4] & 0 | 2 if phase < 540 else phase[:4] & 0 | 0)
                    if phase < 545
                    else phase[:4] & 0 | 11 if phase < 546 else phase[:4] & 0 | 0
                )
            )
        )
    )
    expected_seed_response_data = (
        (phase[:16] & 0 | 0 if phase < 507 else phase[:16] & 0 | 4660)
        if phase < 508
        else (
            phase[:16] & 0 | 0
            if phase < 523
            else phase[:16] & 0 | 48879 if phase < 524 else phase[:16] & 0 | 0
        )
    )
    expected_seed_response_tag = (
        (
            (
                (
                    (phase[:8] & 0 | 0 if phase < 6 else phase[:8] & 0 | 1)
                    if phase < 7
                    else phase[:8] & 0 | 0 if phase < 26 else phase[:8] & 0 | 4
                )
                if phase < 27
                else (
                    (phase[:8] & 0 | 0 if phase < 485 else phase[:8] & 0 | 10)
                    if phase < 494
                    else phase[:8] & 0 | 11 if phase < 495 else phase[:8] & 0 | 12
                )
            )
            if phase < 496
            else (
                (
                    (phase[:8] & 0 | 0 if phase < 499 else phase[:8] & 0 | 13)
                    if phase < 500
                    else phase[:8] & 0 | 0 if phase < 503 else phase[:8] & 0 | 14
                )
                if phase < 504
                else (
                    (phase[:8] & 0 | 0 if phase < 507 else phase[:8] & 0 | 15)
                    if phase < 508
                    else phase[:8] & 0 | 0 if phase < 511 else phase[:8] & 0 | 16
                )
            )
        )
        if phase < 512
        else (
            (
                (
                    (phase[:8] & 0 | 0 if phase < 515 else phase[:8] & 0 | 17)
                    if phase < 516
                    else phase[:8] & 0 | 0 if phase < 519 else phase[:8] & 0 | 18
                )
                if phase < 520
                else (
                    (phase[:8] & 0 | 0 if phase < 523 else phase[:8] & 0 | 19)
                    if phase < 524
                    else phase[:8] & 0 | 0 if phase < 527 else phase[:8] & 0 | 20
                )
            )
            if phase < 528
            else (
                (
                    (phase[:8] & 0 | 0 if phase < 531 else phase[:8] & 0 | 21)
                    if phase < 532
                    else phase[:8] & 0 | 0 if phase < 535 else phase[:8] & 0 | 22
                )
                if phase < 536
                else (
                    (phase[:8] & 0 | 0 if phase < 539 else phase[:8] & 0 | 23)
                    if phase < 540
                    else (
                        phase[:8] & 0 | 0
                        if phase < 545
                        else phase[:8] & 0 | 100 if phase < 546 else phase[:8] & 0 | 0
                    )
                )
            )
        )
    )
    expected_copy_response_dram_address = (
        (
            (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 15
                        else phase[:4] & 0 | 5 if phase < 16 else phase[:4] & 0 | 0
                    )
                    if phase < 35
                    else (
                        phase[:4] & 0 | 9
                        if phase < 36
                        else phase[:4] & 0 | 0 if phase < 414 else phase[:4] & 0 | 3
                    )
                )
                if phase < 415
                else (
                    (
                        phase[:4] & 0 | 6
                        if phase < 416
                        else phase[:4] & 0 | 0 if phase < 418 else phase[:4] & 0 | 9
                    )
                    if phase < 419
                    else (
                        phase[:4] & 0 | 0
                        if phase < 421
                        else phase[:4] & 0 | 12 if phase < 422 else phase[:4] & 0 | 0
                    )
                )
            )
            if phase < 424
            else (
                (
                    (
                        phase[:4] & 0 | 15
                        if phase < 425
                        else phase[:4] & 0 | 0 if phase < 427 else phase[:4] & 0 | 2
                    )
                    if phase < 428
                    else (
                        phase[:4] & 0 | 0
                        if phase < 430
                        else phase[:4] & 0 | 5 if phase < 431 else phase[:4] & 0 | 0
                    )
                )
                if phase < 433
                else (
                    (
                        phase[:4] & 0 | 8
                        if phase < 434
                        else phase[:4] & 0 | 0 if phase < 436 else phase[:4] & 0 | 11
                    )
                    if phase < 437
                    else (
                        (phase[:4] & 0 | 0 if phase < 440 else phase[:4] & 0 | 14)
                        if phase < 441
                        else phase[:4] & 0 | 0 if phase < 444 else phase[:4] & 0 | 1
                    )
                )
            )
        )
        if phase < 445
        else (
            (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 448
                        else phase[:4] & 0 | 4 if phase < 449 else phase[:4] & 0 | 0
                    )
                    if phase < 452
                    else (
                        phase[:4] & 0 | 7
                        if phase < 453
                        else phase[:4] & 0 | 0 if phase < 456 else phase[:4] & 0 | 10
                    )
                )
                if phase < 457
                else (
                    (
                        phase[:4] & 0 | 0
                        if phase < 460
                        else phase[:4] & 0 | 13 if phase < 461 else phase[:4] & 0 | 0
                    )
                    if phase < 468
                    else (
                        phase[:4] & 0 | 3
                        if phase < 469
                        else phase[:4] & 0 | 0 if phase < 472 else phase[:4] & 0 | 6
                    )
                )
            )
            if phase < 473
            else (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 476
                        else phase[:4] & 0 | 9 if phase < 477 else phase[:4] & 0 | 0
                    )
                    if phase < 480
                    else (
                        phase[:4] & 0 | 12
                        if phase < 481
                        else phase[:4] & 0 | 0 if phase < 484 else phase[:4] & 0 | 15
                    )
                )
                if phase < 485
                else (
                    (
                        phase[:4] & 0 | 0
                        if phase < 554
                        else phase[:4] & 0 | 14 if phase < 555 else phase[:4] & 0 | 0
                    )
                    if phase < 574
                    else (
                        (phase[:4] & 0 | 9 if phase < 575 else phase[:4] & 0 | 0)
                        if phase < 577
                        else phase[:4] & 0 | 9 if phase < 578 else phase[:4] & 0 | 0
                    )
                )
            )
        )
    )
    expected_copy_response_sram_address = (
        (
            (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 15
                        else phase[:4] & 0 | 3 if phase < 16 else phase[:4] & 0 | 0
                    )
                    if phase < 35
                    else (
                        phase[:4] & 0 | 3
                        if phase < 36
                        else phase[:4] & 0 | 0 if phase < 221 else phase[:4] & 0 | 3
                    )
                )
                if phase < 414
                else (
                    (
                        phase[:4] & 0 | 8
                        if phase < 415
                        else phase[:4] & 0 | 13 if phase < 416 else phase[:4] & 0 | 0
                    )
                    if phase < 418
                    else (
                        phase[:4] & 0 | 2
                        if phase < 419
                        else phase[:4] & 0 | 0 if phase < 421 else phase[:4] & 0 | 7
                    )
                )
            )
            if phase < 422
            else (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 424
                        else phase[:4] & 0 | 12 if phase < 425 else phase[:4] & 0 | 0
                    )
                    if phase < 427
                    else (
                        phase[:4] & 0 | 1
                        if phase < 428
                        else phase[:4] & 0 | 0 if phase < 430 else phase[:4] & 0 | 6
                    )
                )
                if phase < 431
                else (
                    (
                        phase[:4] & 0 | 0
                        if phase < 433
                        else phase[:4] & 0 | 11 if phase < 434 else phase[:4] & 0 | 0
                    )
                    if phase < 440
                    else (
                        (phase[:4] & 0 | 5 if phase < 441 else phase[:4] & 0 | 0)
                        if phase < 444
                        else phase[:4] & 0 | 10 if phase < 445 else phase[:4] & 0 | 0
                    )
                )
            )
        )
        if phase < 448
        else (
            (
                (
                    (
                        phase[:4] & 0 | 15
                        if phase < 449
                        else phase[:4] & 0 | 0 if phase < 452 else phase[:4] & 0 | 4
                    )
                    if phase < 453
                    else (
                        phase[:4] & 0 | 0
                        if phase < 456
                        else phase[:4] & 0 | 9 if phase < 457 else phase[:4] & 0 | 0
                    )
                )
                if phase < 460
                else (
                    (
                        phase[:4] & 0 | 14
                        if phase < 461
                        else phase[:4] & 0 | 0 if phase < 464 else phase[:4] & 0 | 3
                    )
                    if phase < 465
                    else (
                        (phase[:4] & 0 | 0 if phase < 468 else phase[:4] & 0 | 8)
                        if phase < 469
                        else phase[:4] & 0 | 0 if phase < 472 else phase[:4] & 0 | 13
                    )
                )
            )
            if phase < 473
            else (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 476
                        else phase[:4] & 0 | 2 if phase < 477 else phase[:4] & 0 | 0
                    )
                    if phase < 480
                    else (
                        phase[:4] & 0 | 7
                        if phase < 481
                        else phase[:4] & 0 | 0 if phase < 484 else phase[:4] & 0 | 12
                    )
                )
                if phase < 485
                else (
                    (
                        phase[:4] & 0 | 0
                        if phase < 554
                        else phase[:4] & 0 | 11 if phase < 555 else phase[:4] & 0 | 0
                    )
                    if phase < 574
                    else (
                        (phase[:4] & 0 | 4 if phase < 575 else phase[:4] & 0 | 0)
                        if phase < 577
                        else phase[:4] & 0 | 4 if phase < 578 else phase[:4] & 0 | 0
                    )
                )
            )
        )
    )
    expected_copy_response_data = (
        (
            (phase[:16] & 0 | 0 if phase < 35 else phase[:16] & 0 | 4660)
            if phase < 36
            else phase[:16] & 0 | 0 if phase < 221 else phase[:16] & 0 | 48879
        )
        if phase < 414
        else (
            (phase[:16] & 0 | 0 if phase < 476 else phase[:16] & 0 | 48879)
            if phase < 477
            else (
                phase[:16] & 0 | 0
                if phase < 577
                else phase[:16] & 0 | 16393 if phase < 578 else phase[:16] & 0 | 0
            )
        )
    )
    expected_copy_response_tag = (
        (
            (
                (
                    (
                        phase[:8] & 0 | 0
                        if phase < 15
                        else phase[:8] & 0 | 2 if phase < 16 else phase[:8] & 0 | 0
                    )
                    if phase < 35
                    else (
                        phase[:8] & 0 | 5
                        if phase < 36
                        else phase[:8] & 0 | 0 if phase < 221 else phase[:8] & 0 | 24
                    )
                )
                if phase < 414
                else (
                    (
                        phase[:8] & 0 | 25
                        if phase < 415
                        else phase[:8] & 0 | 26 if phase < 416 else phase[:8] & 0 | 0
                    )
                    if phase < 418
                    else (
                        (phase[:8] & 0 | 27 if phase < 419 else phase[:8] & 0 | 0)
                        if phase < 421
                        else phase[:8] & 0 | 28 if phase < 422 else phase[:8] & 0 | 0
                    )
                )
            )
            if phase < 424
            else (
                (
                    (
                        phase[:8] & 0 | 29
                        if phase < 425
                        else phase[:8] & 0 | 0 if phase < 427 else phase[:8] & 0 | 30
                    )
                    if phase < 428
                    else (
                        phase[:8] & 0 | 0
                        if phase < 430
                        else phase[:8] & 0 | 31 if phase < 431 else phase[:8] & 0 | 0
                    )
                )
                if phase < 433
                else (
                    (
                        phase[:8] & 0 | 32
                        if phase < 434
                        else phase[:8] & 0 | 0 if phase < 436 else phase[:8] & 0 | 33
                    )
                    if phase < 437
                    else (
                        (phase[:8] & 0 | 0 if phase < 440 else phase[:8] & 0 | 34)
                        if phase < 441
                        else phase[:8] & 0 | 0 if phase < 444 else phase[:8] & 0 | 35
                    )
                )
            )
        )
        if phase < 445
        else (
            (
                (
                    (
                        phase[:8] & 0 | 0
                        if phase < 448
                        else phase[:8] & 0 | 36 if phase < 449 else phase[:8] & 0 | 0
                    )
                    if phase < 452
                    else (
                        phase[:8] & 0 | 37
                        if phase < 453
                        else phase[:8] & 0 | 0 if phase < 456 else phase[:8] & 0 | 38
                    )
                )
                if phase < 457
                else (
                    (
                        phase[:8] & 0 | 0
                        if phase < 460
                        else phase[:8] & 0 | 39 if phase < 461 else phase[:8] & 0 | 0
                    )
                    if phase < 464
                    else (
                        (phase[:8] & 0 | 40 if phase < 465 else phase[:8] & 0 | 0)
                        if phase < 468
                        else phase[:8] & 0 | 41 if phase < 469 else phase[:8] & 0 | 0
                    )
                )
            )
            if phase < 472
            else (
                (
                    (
                        phase[:8] & 0 | 42
                        if phase < 473
                        else phase[:8] & 0 | 0 if phase < 476 else phase[:8] & 0 | 43
                    )
                    if phase < 477
                    else (
                        (phase[:8] & 0 | 0 if phase < 480 else phase[:8] & 0 | 44)
                        if phase < 481
                        else phase[:8] & 0 | 0 if phase < 484 else phase[:8] & 0 | 45
                    )
                )
                if phase < 485
                else (
                    (
                        phase[:8] & 0 | 0
                        if phase < 554
                        else phase[:8] & 0 | 101 if phase < 555 else phase[:8] & 0 | 0
                    )
                    if phase < 574
                    else (
                        (phase[:8] & 0 | 130 if phase < 575 else phase[:8] & 0 | 0)
                        if phase < 577
                        else phase[:8] & 0 | 131 if phase < 578 else phase[:8] & 0 | 0
                    )
                )
            )
        )
    )
    expected_check_response_dram_address = (
        (
            (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 40
                        else phase[:4] & 0 | 12 if phase < 41 else phase[:4] & 0 | 0
                    )
                    if phase < 45
                    else (
                        phase[:4] & 0 | 7
                        if phase < 154
                        else phase[:4] & 0 | 8 if phase < 155 else phase[:4] & 0 | 9
                    )
                )
                if phase < 156
                else (
                    (
                        phase[:4] & 0 | 0
                        if phase < 158
                        else phase[:4] & 0 | 10 if phase < 159 else phase[:4] & 0 | 0
                    )
                    if phase < 161
                    else (
                        phase[:4] & 0 | 11
                        if phase < 162
                        else phase[:4] & 0 | 0 if phase < 164 else phase[:4] & 0 | 12
                    )
                )
            )
            if phase < 165
            else (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 167
                        else phase[:4] & 0 | 13 if phase < 168 else phase[:4] & 0 | 0
                    )
                    if phase < 170
                    else (
                        phase[:4] & 0 | 14
                        if phase < 171
                        else phase[:4] & 0 | 0 if phase < 173 else phase[:4] & 0 | 15
                    )
                )
                if phase < 174
                else (
                    (
                        phase[:4] & 0 | 0
                        if phase < 179
                        else phase[:4] & 0 | 1 if phase < 180 else phase[:4] & 0 | 0
                    )
                    if phase < 182
                    else (
                        phase[:4] & 0 | 2
                        if phase < 183
                        else phase[:4] & 0 | 0 if phase < 185 else phase[:4] & 0 | 3
                    )
                )
            )
        )
        if phase < 186
        else (
            (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 188
                        else phase[:4] & 0 | 4 if phase < 189 else phase[:4] & 0 | 0
                    )
                    if phase < 191
                    else (
                        phase[:4] & 0 | 5
                        if phase < 192
                        else phase[:4] & 0 | 0 if phase < 194 else phase[:4] & 0 | 6
                    )
                )
                if phase < 195
                else (
                    (
                        phase[:4] & 0 | 0
                        if phase < 197
                        else phase[:4] & 0 | 7 if phase < 198 else phase[:4] & 0 | 0
                    )
                    if phase < 200
                    else (
                        phase[:4] & 0 | 8
                        if phase < 201
                        else phase[:4] & 0 | 0 if phase < 203 else phase[:4] & 0 | 9
                    )
                )
            )
            if phase < 204
            else (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 206
                        else phase[:4] & 0 | 10 if phase < 207 else phase[:4] & 0 | 0
                    )
                    if phase < 209
                    else (
                        phase[:4] & 0 | 11
                        if phase < 210
                        else phase[:4] & 0 | 0 if phase < 212 else phase[:4] & 0 | 12
                    )
                )
                if phase < 213
                else (
                    (
                        phase[:4] & 0 | 0
                        if phase < 215
                        else phase[:4] & 0 | 13 if phase < 216 else phase[:4] & 0 | 0
                    )
                    if phase < 218
                    else (
                        (phase[:4] & 0 | 14 if phase < 219 else phase[:4] & 0 | 0)
                        if phase < 557
                        else phase[:4] & 0 | 6 if phase < 558 else phase[:4] & 0 | 0
                    )
                )
            )
        )
    )
    expected_check_response_sram_address = (
        (
            (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 20
                        else phase[:4] & 0 | 3 if phase < 21 else phase[:4] & 0 | 0
                    )
                    if phase < 40
                    else (
                        (phase[:4] & 0 | 3 if phase < 41 else phase[:4] & 0 | 0)
                        if phase < 45
                        else phase[:4] & 0 | 3 if phase < 154 else phase[:4] & 0 | 8
                    )
                )
                if phase < 155
                else (
                    (
                        phase[:4] & 0 | 13
                        if phase < 156
                        else phase[:4] & 0 | 0 if phase < 158 else phase[:4] & 0 | 2
                    )
                    if phase < 159
                    else (
                        (phase[:4] & 0 | 0 if phase < 161 else phase[:4] & 0 | 7)
                        if phase < 162
                        else phase[:4] & 0 | 0 if phase < 164 else phase[:4] & 0 | 12
                    )
                )
            )
            if phase < 165
            else (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 167
                        else phase[:4] & 0 | 1 if phase < 168 else phase[:4] & 0 | 0
                    )
                    if phase < 170
                    else (
                        (phase[:4] & 0 | 6 if phase < 171 else phase[:4] & 0 | 0)
                        if phase < 173
                        else phase[:4] & 0 | 11 if phase < 174 else phase[:4] & 0 | 0
                    )
                )
                if phase < 179
                else (
                    (
                        phase[:4] & 0 | 5
                        if phase < 180
                        else phase[:4] & 0 | 0 if phase < 182 else phase[:4] & 0 | 10
                    )
                    if phase < 183
                    else (
                        (phase[:4] & 0 | 0 if phase < 185 else phase[:4] & 0 | 15)
                        if phase < 186
                        else phase[:4] & 0 | 0 if phase < 188 else phase[:4] & 0 | 4
                    )
                )
            )
        )
        if phase < 189
        else (
            (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 191
                        else phase[:4] & 0 | 9 if phase < 192 else phase[:4] & 0 | 0
                    )
                    if phase < 194
                    else (
                        (phase[:4] & 0 | 14 if phase < 195 else phase[:4] & 0 | 0)
                        if phase < 197
                        else phase[:4] & 0 | 3 if phase < 198 else phase[:4] & 0 | 0
                    )
                )
                if phase < 200
                else (
                    (
                        phase[:4] & 0 | 8
                        if phase < 201
                        else phase[:4] & 0 | 0 if phase < 203 else phase[:4] & 0 | 13
                    )
                    if phase < 204
                    else (
                        (phase[:4] & 0 | 0 if phase < 206 else phase[:4] & 0 | 2)
                        if phase < 207
                        else phase[:4] & 0 | 0 if phase < 209 else phase[:4] & 0 | 7
                    )
                )
            )
            if phase < 210
            else (
                (
                    (
                        phase[:4] & 0 | 0
                        if phase < 212
                        else phase[:4] & 0 | 12 if phase < 213 else phase[:4] & 0 | 0
                    )
                    if phase < 215
                    else (
                        (phase[:4] & 0 | 1 if phase < 216 else phase[:4] & 0 | 0)
                        if phase < 218
                        else phase[:4] & 0 | 6 if phase < 219 else phase[:4] & 0 | 0
                    )
                )
                if phase < 557
                else (
                    (
                        (phase[:4] & 0 | 11 if phase < 558 else phase[:4] & 0 | 0)
                        if phase < 562
                        else phase[:4] & 0 | 3 if phase < 569 else phase[:4] & 0 | 0
                    )
                    if phase < 571
                    else (
                        (phase[:4] & 0 | 3 if phase < 572 else phase[:4] & 0 | 0)
                        if phase < 580
                        else phase[:4] & 0 | 11 if phase < 581 else phase[:4] & 0 | 0
                    )
                )
            )
        )
    )
    expected_check_response_data = (
        (
            (
                phase[:16] & 0 | 0
                if phase < 20
                else phase[:16] & 0 | 4660 if phase < 21 else phase[:16] & 0 | 0
            )
            if phase < 40
            else (
                phase[:16] & 0 | 48879
                if phase < 41
                else phase[:16] & 0 | 0 if phase < 45 else phase[:16] & 0 | 48879
            )
        )
        if phase < 154
        else (
            (
                phase[:16] & 0 | 0
                if phase < 197
                else phase[:16] & 0 | 48879 if phase < 198 else phase[:16] & 0 | 0
            )
            if phase < 557
            else (
                (phase[:16] & 0 | 30292 if phase < 558 else phase[:16] & 0 | 0)
                if phase < 580
                else phase[:16] & 0 | 30292 if phase < 581 else phase[:16] & 0 | 0
            )
        )
    )
    expected_check_response_tag = (
        (
            (
                (
                    (
                        phase[:8] & 0 | 0
                        if phase < 20
                        else phase[:8] & 0 | 3 if phase < 21 else phase[:8] & 0 | 0
                    )
                    if phase < 40
                    else (
                        (phase[:8] & 0 | 6 if phase < 41 else phase[:8] & 0 | 0)
                        if phase < 45
                        else phase[:8] & 0 | 46 if phase < 154 else phase[:8] & 0 | 47
                    )
                )
                if phase < 155
                else (
                    (
                        (phase[:8] & 0 | 48 if phase < 156 else phase[:8] & 0 | 0)
                        if phase < 158
                        else phase[:8] & 0 | 49 if phase < 159 else phase[:8] & 0 | 0
                    )
                    if phase < 161
                    else (
                        (phase[:8] & 0 | 50 if phase < 162 else phase[:8] & 0 | 0)
                        if phase < 164
                        else phase[:8] & 0 | 51 if phase < 165 else phase[:8] & 0 | 0
                    )
                )
            )
            if phase < 167
            else (
                (
                    (
                        phase[:8] & 0 | 52
                        if phase < 168
                        else phase[:8] & 0 | 0 if phase < 170 else phase[:8] & 0 | 53
                    )
                    if phase < 171
                    else (
                        (phase[:8] & 0 | 0 if phase < 173 else phase[:8] & 0 | 54)
                        if phase < 174
                        else phase[:8] & 0 | 0 if phase < 176 else phase[:8] & 0 | 55
                    )
                )
                if phase < 177
                else (
                    (
                        (phase[:8] & 0 | 0 if phase < 179 else phase[:8] & 0 | 56)
                        if phase < 180
                        else phase[:8] & 0 | 0 if phase < 182 else phase[:8] & 0 | 57
                    )
                    if phase < 183
                    else (
                        (phase[:8] & 0 | 0 if phase < 185 else phase[:8] & 0 | 58)
                        if phase < 186
                        else phase[:8] & 0 | 0 if phase < 188 else phase[:8] & 0 | 59
                    )
                )
            )
        )
        if phase < 189
        else (
            (
                (
                    (
                        phase[:8] & 0 | 0
                        if phase < 191
                        else phase[:8] & 0 | 60 if phase < 192 else phase[:8] & 0 | 0
                    )
                    if phase < 194
                    else (
                        (phase[:8] & 0 | 61 if phase < 195 else phase[:8] & 0 | 0)
                        if phase < 197
                        else phase[:8] & 0 | 62 if phase < 198 else phase[:8] & 0 | 0
                    )
                )
                if phase < 200
                else (
                    (
                        (phase[:8] & 0 | 63 if phase < 201 else phase[:8] & 0 | 0)
                        if phase < 203
                        else phase[:8] & 0 | 64 if phase < 204 else phase[:8] & 0 | 0
                    )
                    if phase < 206
                    else (
                        (phase[:8] & 0 | 65 if phase < 207 else phase[:8] & 0 | 0)
                        if phase < 209
                        else phase[:8] & 0 | 66 if phase < 210 else phase[:8] & 0 | 0
                    )
                )
            )
            if phase < 212
            else (
                (
                    (
                        (phase[:8] & 0 | 67 if phase < 213 else phase[:8] & 0 | 0)
                        if phase < 215
                        else phase[:8] & 0 | 68 if phase < 216 else phase[:8] & 0 | 0
                    )
                    if phase < 218
                    else (
                        (phase[:8] & 0 | 69 if phase < 219 else phase[:8] & 0 | 0)
                        if phase < 557
                        else phase[:8] & 0 | 102 if phase < 558 else phase[:8] & 0 | 0
                    )
                )
                if phase < 562
                else (
                    (
                        (phase[:8] & 0 | 150 if phase < 567 else phase[:8] & 0 | 151)
                        if phase < 568
                        else phase[:8] & 0 | 152 if phase < 569 else phase[:8] & 0 | 0
                    )
                    if phase < 571
                    else (
                        (phase[:8] & 0 | 155 if phase < 572 else phase[:8] & 0 | 0)
                        if phase < 580
                        else phase[:8] & 0 | 181 if phase < 581 else phase[:8] & 0 | 0
                    )
                )
            )
        )
    )

    @rule
    def check():
        if phase < 582:
            assert dut.seed_ready == expected_seed_ready, "dma: seed_ready"
            assert dut.copy_ready == expected_copy_ready, "dma: copy_ready"
            assert dut.check_ready == expected_check_ready, "dma: check_ready"
            assert dut.seed_valid == expected_seed_valid, "dma: seed_valid"
            assert dut.copy_valid == expected_copy_valid, "dma: copy_valid"
            assert dut.check_valid == expected_check_valid, "dma: check_valid"
            assert dut.dram_accepted == expected_dram_accepted, "dma: dram_accepted"
            assert dut.dram_enqueued == expected_dram_enqueued, "dma: dram_enqueued"
            assert dut.sram_accepted == expected_sram_accepted, "dma: sram_accepted"
            assert dut.sram_enqueued == expected_sram_enqueued, "dma: sram_enqueued"
            assert (expected_seed_valid == 0) | (
                dut.seed_response.dram_address == expected_seed_response_dram_address
            ), "dma: seed_response.dram_address"
            assert (expected_seed_valid == 0) | (
                dut.seed_response.sram_address == expected_seed_response_sram_address
            ), "dma: seed_response.sram_address"
            assert (expected_seed_valid == 0) | (
                dut.seed_response.data == expected_seed_response_data
            ), "dma: seed_response.data"
            assert (expected_seed_valid == 0) | (
                dut.seed_response.tag == expected_seed_response_tag
            ), "dma: seed_response.tag"
            assert (expected_copy_valid == 0) | (
                dut.copy_response.dram_address == expected_copy_response_dram_address
            ), "dma: copy_response.dram_address"
            assert (expected_copy_valid == 0) | (
                dut.copy_response.sram_address == expected_copy_response_sram_address
            ), "dma: copy_response.sram_address"
            assert (expected_copy_valid == 0) | (
                dut.copy_response.data == expected_copy_response_data
            ), "dma: copy_response.data"
            assert (expected_copy_valid == 0) | (
                dut.copy_response.tag == expected_copy_response_tag
            ), "dma: copy_response.tag"
            assert (expected_check_valid == 0) | (
                dut.check_response.dram_address == expected_check_response_dram_address
            ), "dma: check_response.dram_address"
            assert (expected_check_valid == 0) | (
                dut.check_response.sram_address == expected_check_response_sram_address
            ), "dma: check_response.sram_address"
            assert (expected_check_valid == 0) | (
                dut.check_response.data == expected_check_response_data
            ), "dma: check_response.data"
            assert (expected_check_valid == 0) | (
                dut.check_response.tag == expected_check_response_tag
            ), "dma: check_response.tag"
        log("info", "dma.seed_ready", dut.seed_ready)
        log("info", "dma.copy_ready", dut.copy_ready)
        log("info", "dma.check_ready", dut.check_ready)
        log("info", "dma.seed_valid", dut.seed_valid)
        log("info", "dma.copy_valid", dut.copy_valid)
        log("info", "dma.check_valid", dut.check_valid)
        log("info", "dma.dram_accepted", dut.dram_accepted)
        log("info", "dma.dram_enqueued", dut.dram_enqueued)
        log("info", "dma.sram_accepted", dut.sram_accepted)
        log("info", "dma.sram_enqueued", dut.sram_enqueued)
        log("info", "dma.seed_response.dram_address", dut.seed_response.dram_address)
        log("info", "dma.seed_response.sram_address", dut.seed_response.sram_address)
        log("info", "dma.seed_response.data", dut.seed_response.data)
        log("info", "dma.seed_response.tag", dut.seed_response.tag)
        log("info", "dma.copy_response.dram_address", dut.copy_response.dram_address)
        log("info", "dma.copy_response.sram_address", dut.copy_response.sram_address)
        log("info", "dma.copy_response.data", dut.copy_response.data)
        log("info", "dma.copy_response.tag", dut.copy_response.tag)
        log("info", "dma.check_response.dram_address", dut.check_response.dram_address)
        log("info", "dma.check_response.sram_address", dut.check_response.sram_address)
        log("info", "dma.check_response.data", dut.check_response.data)
        log("info", "dma.check_response.tag", dut.check_response.tag)

    check()
    advance(phase)

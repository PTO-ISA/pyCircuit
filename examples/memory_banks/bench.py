"""Regular-clock full known-input stream from the retained memory_banks oracle."""

from example_memory_banks.memory_banks import MemoryBanks, BankRequest
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseMemoryBanks():  # noqa: N802
    phase: bits[64] = 0
    valid = (
        (
            (
                (
                    phase[:1] & 0 | 0
                    if phase < 1
                    else phase[:1] & 0 | 1 if phase < 6 else phase[:1] & 0 | 0
                )
                if phase < 13
                else (
                    phase[:1] & 0 | 1
                    if phase < 18
                    else phase[:1] & 0 | 0 if phase < 25 else phase[:1] & 0 | 1
                )
            )
            if phase < 38
            else (
                (
                    phase[:1] & 0 | 0
                    if phase < 39
                    else phase[:1] & 0 | 1 if phase < 122 else phase[:1] & 0 | 0
                )
                if phase < 123
                else (
                    phase[:1] & 0 | 1
                    if phase < 279
                    else phase[:1] & 0 | 0 if phase < 293 else phase[:1] & 0 | 1
                )
            )
        )
        if phase < 297
        else (
            (
                (
                    phase[:1] & 0 | 0
                    if phase < 300
                    else phase[:1] & 0 | 1 if phase < 304 else phase[:1] & 0 | 0
                )
                if phase < 310
                else (
                    phase[:1] & 0 | 1
                    if phase < 410
                    else phase[:1] & 0 | 0 if phase < 411 else phase[:1] & 0 | 1
                )
            )
            if phase < 412
            else (
                (
                    phase[:1] & 0 | 0
                    if phase < 418
                    else phase[:1] & 0 | 1 if phase < 419 else phase[:1] & 0 | 0
                )
                if phase < 425
                else (
                    (phase[:1] & 0 | 1 if phase < 426 else phase[:1] & 0 | 0)
                    if phase < 432
                    else phase[:1] & 0 | 1 if phase < 433 else phase[:1] & 0 | 0
                )
            )
        )
    )
    bank = (
        (
            (
                (
                    (
                        (
                            (phase[:2] & 0 | 0 if phase < 2 else phase[:2] & 0 | 1)
                            if phase < 3
                            else phase[:2] & 0 | 0 if phase < 4 else phase[:2] & 0 | 1
                        )
                        if phase < 5
                        else (
                            (phase[:2] & 0 | 2 if phase < 6 else phase[:2] & 0 | 0)
                            if phase < 13
                            else phase[:2] & 0 | 3 if phase < 15 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 16
                    else (
                        (
                            (phase[:2] & 0 | 1 if phase < 17 else phase[:2] & 0 | 2)
                            if phase < 18
                            else phase[:2] & 0 | 0 if phase < 26 else phase[:2] & 0 | 1
                        )
                        if phase < 27
                        else (
                            (phase[:2] & 0 | 2 if phase < 28 else phase[:2] & 0 | 3)
                            if phase < 29
                            else (
                                phase[:2] & 0 | 0
                                if phase < 30
                                else (
                                    phase[:2] & 0 | 1
                                    if phase < 31
                                    else phase[:2] & 0 | 2
                                )
                            )
                        )
                    )
                )
                if phase < 32
                else (
                    (
                        (
                            (phase[:2] & 0 | 3 if phase < 33 else phase[:2] & 0 | 0)
                            if phase < 34
                            else phase[:2] & 0 | 1 if phase < 35 else phase[:2] & 0 | 2
                        )
                        if phase < 36
                        else (
                            (phase[:2] & 0 | 3 if phase < 37 else phase[:2] & 0 | 0)
                            if phase < 39
                            else (
                                phase[:2] & 0 | 1
                                if phase < 40
                                else (
                                    phase[:2] & 0 | 2
                                    if phase < 41
                                    else phase[:2] & 0 | 3
                                )
                            )
                        )
                    )
                    if phase < 42
                    else (
                        (
                            (phase[:2] & 0 | 0 if phase < 43 else phase[:2] & 0 | 1)
                            if phase < 44
                            else phase[:2] & 0 | 2 if phase < 45 else phase[:2] & 0 | 3
                        )
                        if phase < 46
                        else (
                            (phase[:2] & 0 | 0 if phase < 47 else phase[:2] & 0 | 1)
                            if phase < 48
                            else (
                                phase[:2] & 0 | 2
                                if phase < 49
                                else (
                                    phase[:2] & 0 | 3
                                    if phase < 50
                                    else phase[:2] & 0 | 0
                                )
                            )
                        )
                    )
                )
            )
            if phase < 51
            else (
                (
                    (
                        (
                            (phase[:2] & 0 | 1 if phase < 52 else phase[:2] & 0 | 2)
                            if phase < 53
                            else phase[:2] & 0 | 3 if phase < 54 else phase[:2] & 0 | 0
                        )
                        if phase < 55
                        else (
                            (phase[:2] & 0 | 1 if phase < 56 else phase[:2] & 0 | 2)
                            if phase < 220
                            else phase[:2] & 0 | 3 if phase < 227 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 228
                    else (
                        (
                            (phase[:2] & 0 | 1 if phase < 229 else phase[:2] & 0 | 2)
                            if phase < 230
                            else phase[:2] & 0 | 3 if phase < 231 else phase[:2] & 0 | 0
                        )
                        if phase < 232
                        else (
                            (phase[:2] & 0 | 1 if phase < 233 else phase[:2] & 0 | 2)
                            if phase < 234
                            else (
                                phase[:2] & 0 | 3
                                if phase < 235
                                else (
                                    phase[:2] & 0 | 0
                                    if phase < 236
                                    else phase[:2] & 0 | 1
                                )
                            )
                        )
                    )
                )
                if phase < 237
                else (
                    (
                        (
                            (phase[:2] & 0 | 2 if phase < 238 else phase[:2] & 0 | 3)
                            if phase < 239
                            else phase[:2] & 0 | 0 if phase < 240 else phase[:2] & 0 | 1
                        )
                        if phase < 241
                        else (
                            (phase[:2] & 0 | 2 if phase < 242 else phase[:2] & 0 | 3)
                            if phase < 243
                            else (
                                phase[:2] & 0 | 0
                                if phase < 244
                                else (
                                    phase[:2] & 0 | 1
                                    if phase < 245
                                    else phase[:2] & 0 | 2
                                )
                            )
                        )
                    )
                    if phase < 246
                    else (
                        (
                            (phase[:2] & 0 | 3 if phase < 247 else phase[:2] & 0 | 0)
                            if phase < 248
                            else phase[:2] & 0 | 1 if phase < 249 else phase[:2] & 0 | 2
                        )
                        if phase < 250
                        else (
                            (phase[:2] & 0 | 3 if phase < 251 else phase[:2] & 0 | 0)
                            if phase < 252
                            else (
                                phase[:2] & 0 | 1
                                if phase < 253
                                else (
                                    phase[:2] & 0 | 2
                                    if phase < 254
                                    else phase[:2] & 0 | 3
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 255
        else (
            (
                (
                    (
                        (
                            (phase[:2] & 0 | 0 if phase < 256 else phase[:2] & 0 | 1)
                            if phase < 257
                            else phase[:2] & 0 | 2 if phase < 258 else phase[:2] & 0 | 3
                        )
                        if phase < 259
                        else (
                            (phase[:2] & 0 | 0 if phase < 260 else phase[:2] & 0 | 1)
                            if phase < 261
                            else phase[:2] & 0 | 2 if phase < 262 else phase[:2] & 0 | 3
                        )
                    )
                    if phase < 263
                    else (
                        (
                            (phase[:2] & 0 | 0 if phase < 264 else phase[:2] & 0 | 1)
                            if phase < 265
                            else phase[:2] & 0 | 2 if phase < 266 else phase[:2] & 0 | 3
                        )
                        if phase < 267
                        else (
                            (phase[:2] & 0 | 0 if phase < 268 else phase[:2] & 0 | 1)
                            if phase < 269
                            else (
                                phase[:2] & 0 | 2
                                if phase < 270
                                else (
                                    phase[:2] & 0 | 3
                                    if phase < 271
                                    else phase[:2] & 0 | 0
                                )
                            )
                        )
                    )
                )
                if phase < 272
                else (
                    (
                        (
                            (phase[:2] & 0 | 1 if phase < 273 else phase[:2] & 0 | 2)
                            if phase < 274
                            else phase[:2] & 0 | 3 if phase < 275 else phase[:2] & 0 | 0
                        )
                        if phase < 276
                        else (
                            (phase[:2] & 0 | 1 if phase < 277 else phase[:2] & 0 | 2)
                            if phase < 278
                            else (
                                phase[:2] & 0 | 3
                                if phase < 279
                                else (
                                    phase[:2] & 0 | 0
                                    if phase < 294
                                    else phase[:2] & 0 | 1
                                )
                            )
                        )
                    )
                    if phase < 295
                    else (
                        (
                            (phase[:2] & 0 | 2 if phase < 296 else phase[:2] & 0 | 3)
                            if phase < 297
                            else phase[:2] & 0 | 0 if phase < 301 else phase[:2] & 0 | 1
                        )
                        if phase < 302
                        else (
                            (phase[:2] & 0 | 2 if phase < 303 else phase[:2] & 0 | 3)
                            if phase < 304
                            else (
                                phase[:2] & 0 | 0
                                if phase < 311
                                else (
                                    phase[:2] & 0 | 1
                                    if phase < 312
                                    else phase[:2] & 0 | 2
                                )
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
                            (phase[:2] & 0 | 3 if phase < 314 else phase[:2] & 0 | 0)
                            if phase < 315
                            else phase[:2] & 0 | 1 if phase < 316 else phase[:2] & 0 | 2
                        )
                        if phase < 317
                        else (
                            (phase[:2] & 0 | 3 if phase < 318 else phase[:2] & 0 | 0)
                            if phase < 319
                            else phase[:2] & 0 | 1 if phase < 320 else phase[:2] & 0 | 2
                        )
                    )
                    if phase < 321
                    else (
                        (
                            (phase[:2] & 0 | 3 if phase < 322 else phase[:2] & 0 | 0)
                            if phase < 323
                            else phase[:2] & 0 | 1 if phase < 324 else phase[:2] & 0 | 2
                        )
                        if phase < 325
                        else (
                            (phase[:2] & 0 | 3 if phase < 326 else phase[:2] & 0 | 0)
                            if phase < 327
                            else (
                                phase[:2] & 0 | 1
                                if phase < 328
                                else (
                                    phase[:2] & 0 | 2
                                    if phase < 329
                                    else phase[:2] & 0 | 3
                                )
                            )
                        )
                    )
                )
                if phase < 330
                else (
                    (
                        (
                            (phase[:2] & 0 | 0 if phase < 331 else phase[:2] & 0 | 1)
                            if phase < 332
                            else phase[:2] & 0 | 2 if phase < 333 else phase[:2] & 0 | 3
                        )
                        if phase < 334
                        else (
                            (phase[:2] & 0 | 0 if phase < 335 else phase[:2] & 0 | 1)
                            if phase < 336
                            else (
                                phase[:2] & 0 | 2
                                if phase < 337
                                else (
                                    phase[:2] & 0 | 3
                                    if phase < 338
                                    else phase[:2] & 0 | 0
                                )
                            )
                        )
                    )
                    if phase < 339
                    else (
                        (
                            (phase[:2] & 0 | 1 if phase < 340 else phase[:2] & 0 | 2)
                            if phase < 410
                            else phase[:2] & 0 | 0 if phase < 418 else phase[:2] & 0 | 1
                        )
                        if phase < 419
                        else (
                            (phase[:2] & 0 | 0 if phase < 425 else phase[:2] & 0 | 2)
                            if phase < 426
                            else (
                                phase[:2] & 0 | 0
                                if phase < 432
                                else (
                                    phase[:2] & 0 | 3
                                    if phase < 433
                                    else phase[:2] & 0 | 0
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    offset = (
        (
            (
                (
                    (phase[:4] & 0 | 0 if phase < 1 else phase[:4] & 0 | 3)
                    if phase < 6
                    else (
                        phase[:4] & 0 | 0
                        if phase < 13
                        else phase[:4] & 0 | 3 if phase < 18 else phase[:4] & 0 | 0
                    )
                )
                if phase < 29
                else (
                    (phase[:4] & 0 | 1 if phase < 33 else phase[:4] & 0 | 2)
                    if phase < 37
                    else (
                        phase[:4] & 0 | 3
                        if phase < 42
                        else phase[:4] & 0 | 4 if phase < 46 else phase[:4] & 0 | 5
                    )
                )
            )
            if phase < 50
            else (
                (
                    (phase[:4] & 0 | 6 if phase < 54 else phase[:4] & 0 | 7)
                    if phase < 227
                    else (
                        phase[:4] & 0 | 8
                        if phase < 231
                        else phase[:4] & 0 | 9 if phase < 235 else phase[:4] & 0 | 10
                    )
                )
                if phase < 239
                else (
                    (phase[:4] & 0 | 11 if phase < 243 else phase[:4] & 0 | 12)
                    if phase < 247
                    else (
                        phase[:4] & 0 | 13
                        if phase < 251
                        else phase[:4] & 0 | 14 if phase < 255 else phase[:4] & 0 | 15
                    )
                )
            )
        )
        if phase < 259
        else (
            (
                (
                    (phase[:4] & 0 | 0 if phase < 263 else phase[:4] & 0 | 1)
                    if phase < 267
                    else (
                        phase[:4] & 0 | 2
                        if phase < 271
                        else phase[:4] & 0 | 3 if phase < 275 else phase[:4] & 0 | 4
                    )
                )
                if phase < 279
                else (
                    (phase[:4] & 0 | 0 if phase < 293 else phase[:4] & 0 | 15)
                    if phase < 297
                    else (
                        phase[:4] & 0 | 0
                        if phase < 300
                        else phase[:4] & 0 | 15 if phase < 304 else phase[:4] & 0 | 0
                    )
                )
            )
            if phase < 310
            else (
                (
                    (phase[:4] & 0 | 5 if phase < 410 else phase[:4] & 0 | 0)
                    if phase < 411
                    else (
                        phase[:4] & 0 | 5
                        if phase < 412
                        else phase[:4] & 0 | 0 if phase < 418 else phase[:4] & 0 | 5
                    )
                )
                if phase < 419
                else (
                    (phase[:4] & 0 | 0 if phase < 425 else phase[:4] & 0 | 5)
                    if phase < 426
                    else (
                        phase[:4] & 0 | 0
                        if phase < 432
                        else phase[:4] & 0 | 5 if phase < 433 else phase[:4] & 0 | 0
                    )
                )
            )
        )
    )
    write = (
        (
            (
                (
                    (
                        (phase[:1] & 0 | 0 if phase < 1 else phase[:1] & 0 | 1)
                        if phase < 3
                        else phase[:1] & 0 | 0 if phase < 13 else phase[:1] & 0 | 1
                    )
                    if phase < 14
                    else (
                        (phase[:1] & 0 | 0 if phase < 25 else phase[:1] & 0 | 1)
                        if phase < 26
                        else phase[:1] & 0 | 0 if phase < 27 else phase[:1] & 0 | 1
                    )
                )
                if phase < 29
                else (
                    (
                        (phase[:1] & 0 | 0 if phase < 30 else phase[:1] & 0 | 1)
                        if phase < 32
                        else phase[:1] & 0 | 0 if phase < 33 else phase[:1] & 0 | 1
                    )
                    if phase < 35
                    else (
                        (phase[:1] & 0 | 0 if phase < 36 else phase[:1] & 0 | 1)
                        if phase < 39
                        else phase[:1] & 0 | 0 if phase < 40 else phase[:1] & 0 | 1
                    )
                )
            )
            if phase < 42
            else (
                (
                    (
                        (phase[:1] & 0 | 0 if phase < 43 else phase[:1] & 0 | 1)
                        if phase < 45
                        else phase[:1] & 0 | 0 if phase < 46 else phase[:1] & 0 | 1
                    )
                    if phase < 48
                    else (
                        (phase[:1] & 0 | 0 if phase < 49 else phase[:1] & 0 | 1)
                        if phase < 51
                        else phase[:1] & 0 | 0 if phase < 52 else phase[:1] & 0 | 1
                    )
                )
                if phase < 54
                else (
                    (
                        (phase[:1] & 0 | 0 if phase < 55 else phase[:1] & 0 | 1)
                        if phase < 220
                        else phase[:1] & 0 | 0 if phase < 227 else phase[:1] & 0 | 1
                    )
                    if phase < 229
                    else (
                        (phase[:1] & 0 | 0 if phase < 230 else phase[:1] & 0 | 1)
                        if phase < 232
                        else (
                            phase[:1] & 0 | 0
                            if phase < 233
                            else phase[:1] & 0 | 1 if phase < 235 else phase[:1] & 0 | 0
                        )
                    )
                )
            )
        )
        if phase < 236
        else (
            (
                (
                    (
                        (phase[:1] & 0 | 1 if phase < 238 else phase[:1] & 0 | 0)
                        if phase < 239
                        else phase[:1] & 0 | 1 if phase < 241 else phase[:1] & 0 | 0
                    )
                    if phase < 242
                    else (
                        (phase[:1] & 0 | 1 if phase < 244 else phase[:1] & 0 | 0)
                        if phase < 245
                        else phase[:1] & 0 | 1 if phase < 247 else phase[:1] & 0 | 0
                    )
                )
                if phase < 248
                else (
                    (
                        (phase[:1] & 0 | 1 if phase < 250 else phase[:1] & 0 | 0)
                        if phase < 251
                        else phase[:1] & 0 | 1 if phase < 253 else phase[:1] & 0 | 0
                    )
                    if phase < 254
                    else (
                        (phase[:1] & 0 | 1 if phase < 256 else phase[:1] & 0 | 0)
                        if phase < 257
                        else (
                            phase[:1] & 0 | 1
                            if phase < 259
                            else phase[:1] & 0 | 0 if phase < 260 else phase[:1] & 0 | 1
                        )
                    )
                )
            )
            if phase < 262
            else (
                (
                    (
                        (phase[:1] & 0 | 0 if phase < 263 else phase[:1] & 0 | 1)
                        if phase < 265
                        else phase[:1] & 0 | 0 if phase < 266 else phase[:1] & 0 | 1
                    )
                    if phase < 268
                    else (
                        (phase[:1] & 0 | 0 if phase < 269 else phase[:1] & 0 | 1)
                        if phase < 271
                        else phase[:1] & 0 | 0 if phase < 272 else phase[:1] & 0 | 1
                    )
                )
                if phase < 274
                else (
                    (
                        (phase[:1] & 0 | 0 if phase < 275 else phase[:1] & 0 | 1)
                        if phase < 277
                        else phase[:1] & 0 | 0 if phase < 278 else phase[:1] & 0 | 1
                    )
                    if phase < 279
                    else (
                        (phase[:1] & 0 | 0 if phase < 293 else phase[:1] & 0 | 1)
                        if phase < 297
                        else (
                            phase[:1] & 0 | 0
                            if phase < 310
                            else phase[:1] & 0 | 1 if phase < 410 else phase[:1] & 0 | 0
                        )
                    )
                )
            )
        )
    )
    data = (
        (
            (
                (
                    (
                        (
                            (phase[:16] & 0 | 0 if phase < 1 else phase[:16] & 0 | 41)
                            if phase < 2
                            else (
                                phase[:16] & 0 | 91 if phase < 3 else phase[:16] & 0 | 0
                            )
                        )
                        if phase < 13
                        else (
                            (
                                phase[:16] & 0 | 13107
                                if phase < 14
                                else phase[:16] & 0 | 65535
                            )
                            if phase < 15
                            else (
                                phase[:16] & 0 | 43690
                                if phase < 18
                                else phase[:16] & 0 | 0
                            )
                        )
                    )
                    if phase < 25
                    else (
                        (
                            (
                                phase[:16] & 0 | 20480
                                if phase < 26
                                else phase[:16] & 0 | 20481
                            )
                            if phase < 27
                            else (
                                phase[:16] & 0 | 20482
                                if phase < 28
                                else phase[:16] & 0 | 20483
                            )
                        )
                        if phase < 29
                        else (
                            (
                                phase[:16] & 0 | 20484
                                if phase < 30
                                else phase[:16] & 0 | 20485
                            )
                            if phase < 31
                            else (
                                phase[:16] & 0 | 20486
                                if phase < 32
                                else phase[:16] & 0 | 20487
                            )
                        )
                    )
                )
                if phase < 33
                else (
                    (
                        (
                            (
                                phase[:16] & 0 | 20488
                                if phase < 34
                                else phase[:16] & 0 | 20489
                            )
                            if phase < 35
                            else (
                                phase[:16] & 0 | 20490
                                if phase < 36
                                else phase[:16] & 0 | 20491
                            )
                        )
                        if phase < 37
                        else (
                            (
                                phase[:16] & 0 | 20492
                                if phase < 39
                                else phase[:16] & 0 | 20493
                            )
                            if phase < 40
                            else (
                                phase[:16] & 0 | 20494
                                if phase < 41
                                else phase[:16] & 0 | 20495
                            )
                        )
                    )
                    if phase < 42
                    else (
                        (
                            (
                                phase[:16] & 0 | 20496
                                if phase < 43
                                else phase[:16] & 0 | 20497
                            )
                            if phase < 44
                            else (
                                phase[:16] & 0 | 20498
                                if phase < 45
                                else phase[:16] & 0 | 20499
                            )
                        )
                        if phase < 46
                        else (
                            (
                                phase[:16] & 0 | 20500
                                if phase < 47
                                else phase[:16] & 0 | 20501
                            )
                            if phase < 48
                            else (
                                phase[:16] & 0 | 20502
                                if phase < 49
                                else (
                                    phase[:16] & 0 | 20503
                                    if phase < 50
                                    else phase[:16] & 0 | 20504
                                )
                            )
                        )
                    )
                )
            )
            if phase < 51
            else (
                (
                    (
                        (
                            (
                                phase[:16] & 0 | 20505
                                if phase < 52
                                else phase[:16] & 0 | 20506
                            )
                            if phase < 53
                            else (
                                phase[:16] & 0 | 20507
                                if phase < 54
                                else phase[:16] & 0 | 20508
                            )
                        )
                        if phase < 55
                        else (
                            (
                                phase[:16] & 0 | 20509
                                if phase < 56
                                else phase[:16] & 0 | 20510
                            )
                            if phase < 220
                            else (
                                phase[:16] & 0 | 20511
                                if phase < 227
                                else phase[:16] & 0 | 20512
                            )
                        )
                    )
                    if phase < 228
                    else (
                        (
                            (
                                phase[:16] & 0 | 20513
                                if phase < 229
                                else phase[:16] & 0 | 20514
                            )
                            if phase < 230
                            else (
                                phase[:16] & 0 | 20515
                                if phase < 231
                                else phase[:16] & 0 | 20516
                            )
                        )
                        if phase < 232
                        else (
                            (
                                phase[:16] & 0 | 20517
                                if phase < 233
                                else phase[:16] & 0 | 20518
                            )
                            if phase < 234
                            else (
                                phase[:16] & 0 | 20519
                                if phase < 235
                                else phase[:16] & 0 | 20520
                            )
                        )
                    )
                )
                if phase < 236
                else (
                    (
                        (
                            (
                                phase[:16] & 0 | 20521
                                if phase < 237
                                else phase[:16] & 0 | 20522
                            )
                            if phase < 238
                            else (
                                phase[:16] & 0 | 20523
                                if phase < 239
                                else phase[:16] & 0 | 20524
                            )
                        )
                        if phase < 240
                        else (
                            (
                                phase[:16] & 0 | 20525
                                if phase < 241
                                else phase[:16] & 0 | 20526
                            )
                            if phase < 242
                            else (
                                phase[:16] & 0 | 20527
                                if phase < 243
                                else phase[:16] & 0 | 20528
                            )
                        )
                    )
                    if phase < 244
                    else (
                        (
                            (
                                phase[:16] & 0 | 20529
                                if phase < 245
                                else phase[:16] & 0 | 20530
                            )
                            if phase < 246
                            else (
                                phase[:16] & 0 | 20531
                                if phase < 247
                                else phase[:16] & 0 | 20532
                            )
                        )
                        if phase < 248
                        else (
                            (
                                phase[:16] & 0 | 20533
                                if phase < 249
                                else phase[:16] & 0 | 20534
                            )
                            if phase < 250
                            else (
                                phase[:16] & 0 | 20535
                                if phase < 251
                                else (
                                    phase[:16] & 0 | 20536
                                    if phase < 252
                                    else phase[:16] & 0 | 20537
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 253
        else (
            (
                (
                    (
                        (
                            (
                                phase[:16] & 0 | 20538
                                if phase < 254
                                else phase[:16] & 0 | 20539
                            )
                            if phase < 255
                            else (
                                phase[:16] & 0 | 20540
                                if phase < 256
                                else phase[:16] & 0 | 20541
                            )
                        )
                        if phase < 257
                        else (
                            (
                                phase[:16] & 0 | 20542
                                if phase < 258
                                else phase[:16] & 0 | 20543
                            )
                            if phase < 259
                            else (
                                phase[:16] & 0 | 20544
                                if phase < 260
                                else phase[:16] & 0 | 20545
                            )
                        )
                    )
                    if phase < 261
                    else (
                        (
                            (
                                phase[:16] & 0 | 20546
                                if phase < 262
                                else phase[:16] & 0 | 20547
                            )
                            if phase < 263
                            else (
                                phase[:16] & 0 | 20548
                                if phase < 264
                                else phase[:16] & 0 | 20549
                            )
                        )
                        if phase < 265
                        else (
                            (
                                phase[:16] & 0 | 20550
                                if phase < 266
                                else phase[:16] & 0 | 20551
                            )
                            if phase < 267
                            else (
                                phase[:16] & 0 | 20552
                                if phase < 268
                                else phase[:16] & 0 | 20553
                            )
                        )
                    )
                )
                if phase < 269
                else (
                    (
                        (
                            (
                                phase[:16] & 0 | 20554
                                if phase < 270
                                else phase[:16] & 0 | 20555
                            )
                            if phase < 271
                            else (
                                phase[:16] & 0 | 20556
                                if phase < 272
                                else phase[:16] & 0 | 20557
                            )
                        )
                        if phase < 273
                        else (
                            (
                                phase[:16] & 0 | 20558
                                if phase < 274
                                else phase[:16] & 0 | 20559
                            )
                            if phase < 275
                            else (
                                phase[:16] & 0 | 20560
                                if phase < 276
                                else phase[:16] & 0 | 20561
                            )
                        )
                    )
                    if phase < 277
                    else (
                        (
                            (
                                phase[:16] & 0 | 20562
                                if phase < 278
                                else phase[:16] & 0 | 20563
                            )
                            if phase < 279
                            else (
                                phase[:16] & 0 | 0
                                if phase < 293
                                else phase[:16] & 0 | 33024
                            )
                        )
                        if phase < 294
                        else (
                            (
                                phase[:16] & 0 | 33025
                                if phase < 295
                                else phase[:16] & 0 | 33026
                            )
                            if phase < 296
                            else (
                                phase[:16] & 0 | 33027
                                if phase < 297
                                else (
                                    phase[:16] & 0 | 0
                                    if phase < 300
                                    else phase[:16] & 0 | 4660
                                )
                            )
                        )
                    )
                )
            )
            if phase < 304
            else (
                (
                    (
                        (
                            (
                                phase[:16] & 0 | 0
                                if phase < 310
                                else phase[:16] & 0 | 36864
                            )
                            if phase < 311
                            else (
                                phase[:16] & 0 | 36865
                                if phase < 312
                                else phase[:16] & 0 | 36866
                            )
                        )
                        if phase < 313
                        else (
                            (
                                phase[:16] & 0 | 36867
                                if phase < 314
                                else phase[:16] & 0 | 36868
                            )
                            if phase < 315
                            else (
                                phase[:16] & 0 | 36869
                                if phase < 316
                                else phase[:16] & 0 | 36870
                            )
                        )
                    )
                    if phase < 317
                    else (
                        (
                            (
                                phase[:16] & 0 | 36871
                                if phase < 318
                                else phase[:16] & 0 | 36872
                            )
                            if phase < 319
                            else (
                                phase[:16] & 0 | 36873
                                if phase < 320
                                else phase[:16] & 0 | 36874
                            )
                        )
                        if phase < 321
                        else (
                            (
                                phase[:16] & 0 | 36875
                                if phase < 322
                                else phase[:16] & 0 | 36876
                            )
                            if phase < 323
                            else (
                                phase[:16] & 0 | 36877
                                if phase < 324
                                else phase[:16] & 0 | 36878
                            )
                        )
                    )
                )
                if phase < 325
                else (
                    (
                        (
                            (
                                phase[:16] & 0 | 36879
                                if phase < 326
                                else phase[:16] & 0 | 36880
                            )
                            if phase < 327
                            else (
                                phase[:16] & 0 | 36881
                                if phase < 328
                                else phase[:16] & 0 | 36882
                            )
                        )
                        if phase < 329
                        else (
                            (
                                phase[:16] & 0 | 36883
                                if phase < 330
                                else phase[:16] & 0 | 36884
                            )
                            if phase < 331
                            else (
                                phase[:16] & 0 | 36885
                                if phase < 332
                                else phase[:16] & 0 | 36886
                            )
                        )
                    )
                    if phase < 333
                    else (
                        (
                            (
                                phase[:16] & 0 | 36887
                                if phase < 334
                                else phase[:16] & 0 | 36888
                            )
                            if phase < 335
                            else (
                                phase[:16] & 0 | 36889
                                if phase < 336
                                else phase[:16] & 0 | 36890
                            )
                        )
                        if phase < 337
                        else (
                            (
                                phase[:16] & 0 | 36891
                                if phase < 338
                                else phase[:16] & 0 | 36892
                            )
                            if phase < 339
                            else (
                                phase[:16] & 0 | 36893
                                if phase < 340
                                else (
                                    phase[:16] & 0 | 36894
                                    if phase < 410
                                    else phase[:16] & 0 | 0
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    tag = (
        (
            (
                (
                    (
                        (
                            (phase[:8] & 0 | 0 if phase < 1 else phase[:8] & 0 | 1)
                            if phase < 2
                            else phase[:8] & 0 | 2 if phase < 3 else phase[:8] & 0 | 3
                        )
                        if phase < 4
                        else (
                            (phase[:8] & 0 | 4 if phase < 5 else phase[:8] & 0 | 5)
                            if phase < 6
                            else (
                                phase[:8] & 0 | 0
                                if phase < 13
                                else (
                                    phase[:8] & 0 | 6
                                    if phase < 14
                                    else phase[:8] & 0 | 7
                                )
                            )
                        )
                    )
                    if phase < 15
                    else (
                        (
                            (phase[:8] & 0 | 8 if phase < 16 else phase[:8] & 0 | 9)
                            if phase < 17
                            else phase[:8] & 0 | 10 if phase < 18 else phase[:8] & 0 | 0
                        )
                        if phase < 25
                        else (
                            (phase[:8] & 0 | 20 if phase < 26 else phase[:8] & 0 | 21)
                            if phase < 27
                            else (
                                phase[:8] & 0 | 22
                                if phase < 28
                                else (
                                    phase[:8] & 0 | 23
                                    if phase < 29
                                    else phase[:8] & 0 | 24
                                )
                            )
                        )
                    )
                )
                if phase < 30
                else (
                    (
                        (
                            (phase[:8] & 0 | 25 if phase < 31 else phase[:8] & 0 | 26)
                            if phase < 32
                            else (
                                phase[:8] & 0 | 27 if phase < 33 else phase[:8] & 0 | 28
                            )
                        )
                        if phase < 34
                        else (
                            (phase[:8] & 0 | 29 if phase < 35 else phase[:8] & 0 | 30)
                            if phase < 36
                            else (
                                phase[:8] & 0 | 31
                                if phase < 37
                                else (
                                    phase[:8] & 0 | 32
                                    if phase < 39
                                    else phase[:8] & 0 | 33
                                )
                            )
                        )
                    )
                    if phase < 40
                    else (
                        (
                            (phase[:8] & 0 | 34 if phase < 41 else phase[:8] & 0 | 35)
                            if phase < 42
                            else (
                                phase[:8] & 0 | 36
                                if phase < 43
                                else (
                                    phase[:8] & 0 | 37
                                    if phase < 44
                                    else phase[:8] & 0 | 38
                                )
                            )
                        )
                        if phase < 45
                        else (
                            (phase[:8] & 0 | 39 if phase < 46 else phase[:8] & 0 | 40)
                            if phase < 47
                            else (
                                phase[:8] & 0 | 41
                                if phase < 48
                                else (
                                    phase[:8] & 0 | 42
                                    if phase < 49
                                    else phase[:8] & 0 | 43
                                )
                            )
                        )
                    )
                )
            )
            if phase < 50
            else (
                (
                    (
                        (
                            (phase[:8] & 0 | 44 if phase < 51 else phase[:8] & 0 | 45)
                            if phase < 52
                            else (
                                phase[:8] & 0 | 46 if phase < 53 else phase[:8] & 0 | 47
                            )
                        )
                        if phase < 54
                        else (
                            (phase[:8] & 0 | 48 if phase < 55 else phase[:8] & 0 | 49)
                            if phase < 56
                            else (
                                phase[:8] & 0 | 50
                                if phase < 220
                                else (
                                    phase[:8] & 0 | 51
                                    if phase < 227
                                    else phase[:8] & 0 | 52
                                )
                            )
                        )
                    )
                    if phase < 228
                    else (
                        (
                            (phase[:8] & 0 | 53 if phase < 229 else phase[:8] & 0 | 54)
                            if phase < 230
                            else (
                                phase[:8] & 0 | 55
                                if phase < 231
                                else phase[:8] & 0 | 56
                            )
                        )
                        if phase < 232
                        else (
                            (phase[:8] & 0 | 57 if phase < 233 else phase[:8] & 0 | 58)
                            if phase < 234
                            else (
                                phase[:8] & 0 | 59
                                if phase < 235
                                else (
                                    phase[:8] & 0 | 60
                                    if phase < 236
                                    else phase[:8] & 0 | 61
                                )
                            )
                        )
                    )
                )
                if phase < 237
                else (
                    (
                        (
                            (phase[:8] & 0 | 62 if phase < 238 else phase[:8] & 0 | 63)
                            if phase < 239
                            else (
                                phase[:8] & 0 | 64
                                if phase < 240
                                else phase[:8] & 0 | 65
                            )
                        )
                        if phase < 241
                        else (
                            (phase[:8] & 0 | 66 if phase < 242 else phase[:8] & 0 | 67)
                            if phase < 243
                            else (
                                phase[:8] & 0 | 68
                                if phase < 244
                                else (
                                    phase[:8] & 0 | 69
                                    if phase < 245
                                    else phase[:8] & 0 | 70
                                )
                            )
                        )
                    )
                    if phase < 246
                    else (
                        (
                            (phase[:8] & 0 | 71 if phase < 247 else phase[:8] & 0 | 72)
                            if phase < 248
                            else (
                                phase[:8] & 0 | 73
                                if phase < 249
                                else (
                                    phase[:8] & 0 | 74
                                    if phase < 250
                                    else phase[:8] & 0 | 75
                                )
                            )
                        )
                        if phase < 251
                        else (
                            (phase[:8] & 0 | 76 if phase < 252 else phase[:8] & 0 | 77)
                            if phase < 253
                            else (
                                phase[:8] & 0 | 78
                                if phase < 254
                                else (
                                    phase[:8] & 0 | 79
                                    if phase < 255
                                    else phase[:8] & 0 | 80
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 256
        else (
            (
                (
                    (
                        (
                            (phase[:8] & 0 | 81 if phase < 257 else phase[:8] & 0 | 82)
                            if phase < 258
                            else (
                                phase[:8] & 0 | 83
                                if phase < 259
                                else phase[:8] & 0 | 84
                            )
                        )
                        if phase < 260
                        else (
                            (phase[:8] & 0 | 85 if phase < 261 else phase[:8] & 0 | 86)
                            if phase < 262
                            else (
                                phase[:8] & 0 | 87
                                if phase < 263
                                else (
                                    phase[:8] & 0 | 88
                                    if phase < 264
                                    else phase[:8] & 0 | 89
                                )
                            )
                        )
                    )
                    if phase < 265
                    else (
                        (
                            (phase[:8] & 0 | 90 if phase < 266 else phase[:8] & 0 | 91)
                            if phase < 267
                            else (
                                phase[:8] & 0 | 92
                                if phase < 268
                                else phase[:8] & 0 | 93
                            )
                        )
                        if phase < 269
                        else (
                            (phase[:8] & 0 | 94 if phase < 270 else phase[:8] & 0 | 95)
                            if phase < 271
                            else (
                                phase[:8] & 0 | 96
                                if phase < 272
                                else (
                                    phase[:8] & 0 | 97
                                    if phase < 273
                                    else phase[:8] & 0 | 98
                                )
                            )
                        )
                    )
                )
                if phase < 274
                else (
                    (
                        (
                            (phase[:8] & 0 | 99 if phase < 275 else phase[:8] & 0 | 100)
                            if phase < 276
                            else (
                                phase[:8] & 0 | 101
                                if phase < 277
                                else phase[:8] & 0 | 102
                            )
                        )
                        if phase < 278
                        else (
                            (phase[:8] & 0 | 103 if phase < 279 else phase[:8] & 0 | 0)
                            if phase < 293
                            else (
                                phase[:8] & 0 | 120
                                if phase < 294
                                else (
                                    phase[:8] & 0 | 121
                                    if phase < 295
                                    else phase[:8] & 0 | 122
                                )
                            )
                        )
                    )
                    if phase < 296
                    else (
                        (
                            (phase[:8] & 0 | 123 if phase < 297 else phase[:8] & 0 | 0)
                            if phase < 300
                            else (
                                phase[:8] & 0 | 124
                                if phase < 301
                                else (
                                    phase[:8] & 0 | 125
                                    if phase < 302
                                    else phase[:8] & 0 | 126
                                )
                            )
                        )
                        if phase < 303
                        else (
                            (phase[:8] & 0 | 127 if phase < 304 else phase[:8] & 0 | 0)
                            if phase < 310
                            else (
                                phase[:8] & 0 | 150
                                if phase < 311
                                else (
                                    phase[:8] & 0 | 151
                                    if phase < 312
                                    else phase[:8] & 0 | 152
                                )
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
                            (
                                phase[:8] & 0 | 153
                                if phase < 314
                                else phase[:8] & 0 | 154
                            )
                            if phase < 315
                            else (
                                phase[:8] & 0 | 155
                                if phase < 316
                                else phase[:8] & 0 | 156
                            )
                        )
                        if phase < 317
                        else (
                            (
                                phase[:8] & 0 | 157
                                if phase < 318
                                else phase[:8] & 0 | 158
                            )
                            if phase < 319
                            else (
                                phase[:8] & 0 | 159
                                if phase < 320
                                else (
                                    phase[:8] & 0 | 160
                                    if phase < 321
                                    else phase[:8] & 0 | 161
                                )
                            )
                        )
                    )
                    if phase < 322
                    else (
                        (
                            (
                                phase[:8] & 0 | 162
                                if phase < 323
                                else phase[:8] & 0 | 163
                            )
                            if phase < 324
                            else (
                                phase[:8] & 0 | 164
                                if phase < 325
                                else phase[:8] & 0 | 165
                            )
                        )
                        if phase < 326
                        else (
                            (
                                phase[:8] & 0 | 166
                                if phase < 327
                                else phase[:8] & 0 | 167
                            )
                            if phase < 328
                            else (
                                phase[:8] & 0 | 168
                                if phase < 329
                                else (
                                    phase[:8] & 0 | 169
                                    if phase < 330
                                    else phase[:8] & 0 | 170
                                )
                            )
                        )
                    )
                )
                if phase < 331
                else (
                    (
                        (
                            (
                                phase[:8] & 0 | 171
                                if phase < 332
                                else phase[:8] & 0 | 172
                            )
                            if phase < 333
                            else (
                                phase[:8] & 0 | 173
                                if phase < 334
                                else phase[:8] & 0 | 174
                            )
                        )
                        if phase < 335
                        else (
                            (
                                phase[:8] & 0 | 175
                                if phase < 336
                                else phase[:8] & 0 | 176
                            )
                            if phase < 337
                            else (
                                phase[:8] & 0 | 177
                                if phase < 338
                                else (
                                    phase[:8] & 0 | 178
                                    if phase < 339
                                    else phase[:8] & 0 | 179
                                )
                            )
                        )
                    )
                    if phase < 340
                    else (
                        (
                            (phase[:8] & 0 | 180 if phase < 410 else phase[:8] & 0 | 0)
                            if phase < 411
                            else (
                                phase[:8] & 0 | 200
                                if phase < 412
                                else (
                                    phase[:8] & 0 | 0
                                    if phase < 418
                                    else phase[:8] & 0 | 201
                                )
                            )
                        )
                        if phase < 419
                        else (
                            (phase[:8] & 0 | 0 if phase < 425 else phase[:8] & 0 | 202)
                            if phase < 426
                            else (
                                phase[:8] & 0 | 0
                                if phase < 432
                                else (
                                    phase[:8] & 0 | 203
                                    if phase < 433
                                    else phase[:8] & 0 | 0
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
            phase[:1] & 0 | 1
            if phase < 25
            else phase[:1] & 0 | 0 if phase < 207 else phase[:1] & 0 | 1
        )
        if phase < 297
        else (
            (phase[:1] & 0 | 0 if phase < 299 else phase[:1] & 0 | 1)
            if phase < 310
            else phase[:1] & 0 | 0 if phase < 410 else phase[:1] & 0 | 1
        )
    )
    request = BankRequest(bank=bank, offset=offset, write=write, data=data, tag=tag)
    dut = MemoryBanks(valid, request, take)
    expected_input_ready = (
        (
            (phase[:1] & 0 | 1 if phase < 56 else phase[:1] & 0 | 0)
            if phase < 219
            else phase[:1] & 0 | 1 if phase < 220 else phase[:1] & 0 | 0
        )
        if phase < 226
        else (
            (phase[:1] & 0 | 1 if phase < 340 else phase[:1] & 0 | 0)
            if phase < 422
            else (
                phase[:1] & 0 | 1
                if phase < 426
                else phase[:1] & 0 | 0 if phase < 429 else phase[:1] & 0 | 1
            )
        )
    )
    expected_output_valid = (
        (
            (
                (phase[:1] & 0 | 0 if phase < 7 else phase[:1] & 0 | 1)
                if phase < 9
                else (
                    phase[:1] & 0 | 0
                    if phase < 10
                    else phase[:1] & 0 | 1 if phase < 13 else phase[:1] & 0 | 0
                )
            )
            if phase < 19
            else (
                (phase[:1] & 0 | 1 if phase < 20 else phase[:1] & 0 | 0)
                if phase < 21
                else (
                    phase[:1] & 0 | 1
                    if phase < 25
                    else phase[:1] & 0 | 0 if phase < 31 else phase[:1] & 0 | 1
                )
            )
        )
        if phase < 229
        else (
            (
                (phase[:1] & 0 | 0 if phase < 231 else phase[:1] & 0 | 1)
                if phase < 293
                else (
                    phase[:1] & 0 | 0
                    if phase < 299
                    else phase[:1] & 0 | 1 if phase < 303 else phase[:1] & 0 | 0
                )
            )
            if phase < 306
            else (
                (phase[:1] & 0 | 1 if phase < 310 else phase[:1] & 0 | 0)
                if phase < 316
                else (
                    phase[:1] & 0 | 1
                    if phase < 432
                    else phase[:1] & 0 | 0 if phase < 434 else phase[:1] & 0 | 1
                )
            )
        )
    )
    expected_response_bank = (
        (
            (
                (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 8
                            else phase[:2] & 0 | 1 if phase < 9 else phase[:2] & 0 | 0
                        )
                        if phase < 11
                        else (
                            (phase[:2] & 0 | 1 if phase < 12 else phase[:2] & 0 | 2)
                            if phase < 13
                            else phase[:2] & 0 | 0 if phase < 19 else phase[:2] & 0 | 3
                        )
                    )
                    if phase < 20
                    else (
                        (
                            phase[:2] & 0 | 0
                            if phase < 22
                            else phase[:2] & 0 | 1 if phase < 23 else phase[:2] & 0 | 2
                        )
                        if phase < 24
                        else (
                            (phase[:2] & 0 | 3 if phase < 25 else phase[:2] & 0 | 0)
                            if phase < 208
                            else phase[:2] & 0 | 1 if phase < 209 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 212
                else (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 214
                            else phase[:2] & 0 | 0 if phase < 215 else phase[:2] & 0 | 1
                        )
                        if phase < 216
                        else (
                            (phase[:2] & 0 | 2 if phase < 217 else phase[:2] & 0 | 0)
                            if phase < 218
                            else phase[:2] & 0 | 1 if phase < 219 else phase[:2] & 0 | 2
                        )
                    )
                    if phase < 220
                    else (
                        (
                            (phase[:2] & 0 | 1 if phase < 221 else phase[:2] & 0 | 2)
                            if phase < 223
                            else phase[:2] & 0 | 3 if phase < 224 else phase[:2] & 0 | 2
                        )
                        if phase < 225
                        else (
                            (phase[:2] & 0 | 3 if phase < 227 else phase[:2] & 0 | 2)
                            if phase < 228
                            else phase[:2] & 0 | 3 if phase < 229 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
            if phase < 232
            else (
                (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 233
                            else phase[:2] & 0 | 2 if phase < 234 else phase[:2] & 0 | 3
                        )
                        if phase < 235
                        else (
                            (phase[:2] & 0 | 0 if phase < 236 else phase[:2] & 0 | 1)
                            if phase < 237
                            else phase[:2] & 0 | 2 if phase < 238 else phase[:2] & 0 | 3
                        )
                    )
                    if phase < 239
                    else (
                        (
                            (phase[:2] & 0 | 0 if phase < 240 else phase[:2] & 0 | 1)
                            if phase < 241
                            else phase[:2] & 0 | 2 if phase < 242 else phase[:2] & 0 | 3
                        )
                        if phase < 243
                        else (
                            (phase[:2] & 0 | 0 if phase < 244 else phase[:2] & 0 | 1)
                            if phase < 245
                            else phase[:2] & 0 | 2 if phase < 246 else phase[:2] & 0 | 3
                        )
                    )
                )
                if phase < 247
                else (
                    (
                        (
                            phase[:2] & 0 | 0
                            if phase < 248
                            else phase[:2] & 0 | 1 if phase < 249 else phase[:2] & 0 | 2
                        )
                        if phase < 250
                        else (
                            (phase[:2] & 0 | 3 if phase < 251 else phase[:2] & 0 | 0)
                            if phase < 252
                            else phase[:2] & 0 | 1 if phase < 253 else phase[:2] & 0 | 2
                        )
                    )
                    if phase < 254
                    else (
                        (
                            (phase[:2] & 0 | 3 if phase < 255 else phase[:2] & 0 | 0)
                            if phase < 256
                            else phase[:2] & 0 | 1 if phase < 257 else phase[:2] & 0 | 2
                        )
                        if phase < 258
                        else (
                            (phase[:2] & 0 | 3 if phase < 259 else phase[:2] & 0 | 0)
                            if phase < 260
                            else phase[:2] & 0 | 1 if phase < 261 else phase[:2] & 0 | 2
                        )
                    )
                )
            )
        )
        if phase < 262
        else (
            (
                (
                    (
                        (
                            phase[:2] & 0 | 3
                            if phase < 263
                            else phase[:2] & 0 | 0 if phase < 264 else phase[:2] & 0 | 1
                        )
                        if phase < 265
                        else (
                            (phase[:2] & 0 | 2 if phase < 266 else phase[:2] & 0 | 3)
                            if phase < 267
                            else phase[:2] & 0 | 0 if phase < 268 else phase[:2] & 0 | 1
                        )
                    )
                    if phase < 269
                    else (
                        (
                            (phase[:2] & 0 | 2 if phase < 270 else phase[:2] & 0 | 3)
                            if phase < 271
                            else phase[:2] & 0 | 0 if phase < 272 else phase[:2] & 0 | 1
                        )
                        if phase < 273
                        else (
                            (phase[:2] & 0 | 2 if phase < 274 else phase[:2] & 0 | 3)
                            if phase < 275
                            else phase[:2] & 0 | 0 if phase < 276 else phase[:2] & 0 | 1
                        )
                    )
                )
                if phase < 277
                else (
                    (
                        (
                            phase[:2] & 0 | 2
                            if phase < 278
                            else phase[:2] & 0 | 3 if phase < 279 else phase[:2] & 0 | 0
                        )
                        if phase < 280
                        else (
                            (phase[:2] & 0 | 1 if phase < 281 else phase[:2] & 0 | 2)
                            if phase < 282
                            else phase[:2] & 0 | 3 if phase < 283 else phase[:2] & 0 | 0
                        )
                    )
                    if phase < 284
                    else (
                        (
                            (phase[:2] & 0 | 1 if phase < 285 else phase[:2] & 0 | 2)
                            if phase < 286
                            else phase[:2] & 0 | 3 if phase < 287 else phase[:2] & 0 | 0
                        )
                        if phase < 288
                        else (
                            (phase[:2] & 0 | 1 if phase < 289 else phase[:2] & 0 | 2)
                            if phase < 290
                            else phase[:2] & 0 | 3 if phase < 293 else phase[:2] & 0 | 0
                        )
                    )
                )
            )
            if phase < 300
            else (
                (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 301
                            else phase[:2] & 0 | 2 if phase < 302 else phase[:2] & 0 | 3
                        )
                        if phase < 303
                        else (
                            (phase[:2] & 0 | 0 if phase < 307 else phase[:2] & 0 | 1)
                            if phase < 308
                            else phase[:2] & 0 | 2 if phase < 309 else phase[:2] & 0 | 3
                        )
                    )
                    if phase < 310
                    else (
                        (
                            (phase[:2] & 0 | 0 if phase < 411 else phase[:2] & 0 | 1)
                            if phase < 412
                            else phase[:2] & 0 | 0 if phase < 415 else phase[:2] & 0 | 1
                        )
                        if phase < 417
                        else (
                            (phase[:2] & 0 | 0 if phase < 418 else phase[:2] & 0 | 1)
                            if phase < 419
                            else phase[:2] & 0 | 2 if phase < 420 else phase[:2] & 0 | 0
                        )
                    )
                )
                if phase < 421
                else (
                    (
                        (
                            phase[:2] & 0 | 1
                            if phase < 422
                            else phase[:2] & 0 | 2 if phase < 423 else phase[:2] & 0 | 1
                        )
                        if phase < 424
                        else (
                            (phase[:2] & 0 | 2 if phase < 426 else phase[:2] & 0 | 3)
                            if phase < 427
                            else phase[:2] & 0 | 2 if phase < 428 else phase[:2] & 0 | 3
                        )
                    )
                    if phase < 430
                    else (
                        (
                            (phase[:2] & 0 | 2 if phase < 431 else phase[:2] & 0 | 3)
                            if phase < 432
                            else phase[:2] & 0 | 0 if phase < 435 else phase[:2] & 0 | 1
                        )
                        if phase < 436
                        else (
                            (phase[:2] & 0 | 2 if phase < 437 else phase[:2] & 0 | 3)
                            if phase < 438
                            else phase[:2] & 0 | 0 if phase < 439 else phase[:2] & 0 | 1
                        )
                    )
                )
            )
        )
    )
    expected_response_offset = (
        (
            (
                (
                    (
                        (phase[:4] & 0 | 0 if phase < 7 else phase[:4] & 0 | 3)
                        if phase < 9
                        else phase[:4] & 0 | 0 if phase < 10 else phase[:4] & 0 | 3
                    )
                    if phase < 13
                    else (
                        (phase[:4] & 0 | 0 if phase < 19 else phase[:4] & 0 | 3)
                        if phase < 20
                        else phase[:4] & 0 | 0 if phase < 21 else phase[:4] & 0 | 3
                    )
                )
                if phase < 25
                else (
                    (
                        (phase[:4] & 0 | 0 if phase < 209 else phase[:4] & 0 | 1)
                        if phase < 210
                        else phase[:4] & 0 | 2 if phase < 211 else phase[:4] & 0 | 3
                    )
                    if phase < 212
                    else (
                        (phase[:4] & 0 | 1 if phase < 213 else phase[:4] & 0 | 2)
                        if phase < 214
                        else (
                            phase[:4] & 0 | 4
                            if phase < 215
                            else phase[:4] & 0 | 3 if phase < 216 else phase[:4] & 0 | 0
                        )
                    )
                )
            )
            if phase < 217
            else (
                (
                    (
                        (phase[:4] & 0 | 5 if phase < 218 else phase[:4] & 0 | 4)
                        if phase < 219
                        else phase[:4] & 0 | 1 if phase < 220 else phase[:4] & 0 | 5
                    )
                    if phase < 221
                    else (
                        (phase[:4] & 0 | 2 if phase < 222 else phase[:4] & 0 | 3)
                        if phase < 223
                        else (
                            phase[:4] & 0 | 0
                            if phase < 224
                            else phase[:4] & 0 | 4 if phase < 225 else phase[:4] & 0 | 1
                        )
                    )
                )
                if phase < 226
                else (
                    (
                        (phase[:4] & 0 | 2 if phase < 227 else phase[:4] & 0 | 5)
                        if phase < 228
                        else phase[:4] & 0 | 3 if phase < 229 else phase[:4] & 0 | 0
                    )
                    if phase < 231
                    else (
                        (phase[:4] & 0 | 6 if phase < 234 else phase[:4] & 0 | 4)
                        if phase < 235
                        else (
                            phase[:4] & 0 | 7
                            if phase < 238
                            else phase[:4] & 0 | 5 if phase < 239 else phase[:4] & 0 | 8
                        )
                    )
                )
            )
        )
        if phase < 242
        else (
            (
                (
                    (
                        (phase[:4] & 0 | 6 if phase < 243 else phase[:4] & 0 | 9)
                        if phase < 246
                        else phase[:4] & 0 | 7 if phase < 247 else phase[:4] & 0 | 10
                    )
                    if phase < 250
                    else (
                        (phase[:4] & 0 | 8 if phase < 251 else phase[:4] & 0 | 11)
                        if phase < 254
                        else phase[:4] & 0 | 9 if phase < 255 else phase[:4] & 0 | 12
                    )
                )
                if phase < 258
                else (
                    (
                        (phase[:4] & 0 | 10 if phase < 259 else phase[:4] & 0 | 13)
                        if phase < 262
                        else phase[:4] & 0 | 11 if phase < 263 else phase[:4] & 0 | 14
                    )
                    if phase < 266
                    else (
                        (phase[:4] & 0 | 12 if phase < 267 else phase[:4] & 0 | 15)
                        if phase < 270
                        else (
                            phase[:4] & 0 | 13
                            if phase < 271
                            else (
                                phase[:4] & 0 | 0 if phase < 274 else phase[:4] & 0 | 14
                            )
                        )
                    )
                )
            )
            if phase < 275
            else (
                (
                    (
                        (phase[:4] & 0 | 1 if phase < 278 else phase[:4] & 0 | 15)
                        if phase < 279
                        else phase[:4] & 0 | 2 if phase < 282 else phase[:4] & 0 | 0
                    )
                    if phase < 283
                    else (
                        (phase[:4] & 0 | 3 if phase < 286 else phase[:4] & 0 | 1)
                        if phase < 287
                        else (
                            phase[:4] & 0 | 4
                            if phase < 290
                            else phase[:4] & 0 | 2 if phase < 291 else phase[:4] & 0 | 3
                        )
                    )
                )
                if phase < 292
                else (
                    (
                        (phase[:4] & 0 | 4 if phase < 293 else phase[:4] & 0 | 0)
                        if phase < 299
                        else phase[:4] & 0 | 15 if phase < 303 else phase[:4] & 0 | 0
                    )
                    if phase < 306
                    else (
                        (phase[:4] & 0 | 15 if phase < 310 else phase[:4] & 0 | 0)
                        if phase < 316
                        else (
                            phase[:4] & 0 | 5
                            if phase < 432
                            else phase[:4] & 0 | 0 if phase < 434 else phase[:4] & 0 | 5
                        )
                    )
                )
            )
        )
    )
    expected_response_write = (
        (
            (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 7
                        else phase[:1] & 0 | 1 if phase < 9 else phase[:1] & 0 | 0
                    )
                    if phase < 19
                    else (
                        (phase[:1] & 0 | 1 if phase < 20 else phase[:1] & 0 | 0)
                        if phase < 31
                        else phase[:1] & 0 | 1 if phase < 208 else phase[:1] & 0 | 0
                    )
                )
                if phase < 210
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 214
                        else phase[:1] & 0 | 0 if phase < 216 else phase[:1] & 0 | 1
                    )
                    if phase < 221
                    else (
                        (phase[:1] & 0 | 0 if phase < 222 else phase[:1] & 0 | 1)
                        if phase < 225
                        else phase[:1] & 0 | 0 if phase < 226 else phase[:1] & 0 | 1
                    )
                )
            )
            if phase < 227
            else (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 228
                        else phase[:1] & 0 | 1 if phase < 229 else phase[:1] & 0 | 0
                    )
                    if phase < 231
                    else (
                        (phase[:1] & 0 | 1 if phase < 232 else phase[:1] & 0 | 0)
                        if phase < 233
                        else phase[:1] & 0 | 1 if phase < 234 else phase[:1] & 0 | 0
                    )
                )
                if phase < 236
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 241
                        else phase[:1] & 0 | 0 if phase < 242 else phase[:1] & 0 | 1
                    )
                    if phase < 244
                    else (
                        (phase[:1] & 0 | 0 if phase < 245 else phase[:1] & 0 | 1)
                        if phase < 246
                        else phase[:1] & 0 | 0 if phase < 248 else phase[:1] & 0 | 1
                    )
                )
            )
        )
        if phase < 253
        else (
            (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 254
                        else phase[:1] & 0 | 1 if phase < 256 else phase[:1] & 0 | 0
                    )
                    if phase < 257
                    else (
                        (phase[:1] & 0 | 1 if phase < 258 else phase[:1] & 0 | 0)
                        if phase < 260
                        else phase[:1] & 0 | 1 if phase < 265 else phase[:1] & 0 | 0
                    )
                )
                if phase < 266
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 268
                        else phase[:1] & 0 | 0 if phase < 269 else phase[:1] & 0 | 1
                    )
                    if phase < 270
                    else (
                        (phase[:1] & 0 | 0 if phase < 272 else phase[:1] & 0 | 1)
                        if phase < 277
                        else phase[:1] & 0 | 0 if phase < 278 else phase[:1] & 0 | 1
                    )
                )
            )
            if phase < 280
            else (
                (
                    (
                        phase[:1] & 0 | 0
                        if phase < 281
                        else phase[:1] & 0 | 1 if phase < 282 else phase[:1] & 0 | 0
                    )
                    if phase < 284
                    else (
                        (phase[:1] & 0 | 1 if phase < 289 else phase[:1] & 0 | 0)
                        if phase < 290
                        else phase[:1] & 0 | 1 if phase < 291 else phase[:1] & 0 | 0
                    )
                )
                if phase < 292
                else (
                    (
                        phase[:1] & 0 | 1
                        if phase < 293
                        else phase[:1] & 0 | 0 if phase < 299 else phase[:1] & 0 | 1
                    )
                    if phase < 303
                    else (
                        (phase[:1] & 0 | 0 if phase < 316 else phase[:1] & 0 | 1)
                        if phase < 432
                        else phase[:1] & 0 | 0 if phase < 434 else phase[:1] & 0 | 1
                    )
                )
            )
        )
    )
    expected_response_data = (
        (
            (
                (
                    (
                        (phase[:16] & 0 | 0 if phase < 10 else phase[:16] & 0 | 41)
                        if phase < 11
                        else phase[:16] & 0 | 91 if phase < 12 else phase[:16] & 0 | 0
                    )
                    if phase < 21
                    else (
                        (phase[:16] & 0 | 41 if phase < 22 else phase[:16] & 0 | 91)
                        if phase < 23
                        else (
                            phase[:16] & 0 | 0
                            if phase < 24
                            else (
                                phase[:16] & 0 | 13107
                                if phase < 25
                                else phase[:16] & 0 | 0
                            )
                        )
                    )
                )
                if phase < 211
                else (
                    (
                        (phase[:16] & 0 | 41 if phase < 212 else phase[:16] & 0 | 0)
                        if phase < 215
                        else phase[:16] & 0 | 91 if phase < 216 else phase[:16] & 0 | 0
                    )
                    if phase < 228
                    else (
                        (phase[:16] & 0 | 13107 if phase < 229 else phase[:16] & 0 | 0)
                        if phase < 271
                        else (
                            phase[:16] & 0 | 20480
                            if phase < 272
                            else (
                                phase[:16] & 0 | 0
                                if phase < 273
                                else phase[:16] & 0 | 20482
                            )
                        )
                    )
                )
            )
            if phase < 274
            else (
                (
                    (
                        (phase[:16] & 0 | 0 if phase < 276 else phase[:16] & 0 | 20485)
                        if phase < 277
                        else (
                            phase[:16] & 0 | 20486
                            if phase < 278
                            else phase[:16] & 0 | 0
                        )
                    )
                    if phase < 279
                    else (
                        (
                            phase[:16] & 0 | 20488
                            if phase < 280
                            else phase[:16] & 0 | 20489
                        )
                        if phase < 281
                        else (
                            phase[:16] & 0 | 0
                            if phase < 282
                            else (
                                phase[:16] & 0 | 20483
                                if phase < 283
                                else phase[:16] & 0 | 20492
                            )
                        )
                    )
                )
                if phase < 284
                else (
                    (
                        (phase[:16] & 0 | 91 if phase < 285 else phase[:16] & 0 | 20494)
                        if phase < 286
                        else (
                            phase[:16] & 0 | 0
                            if phase < 288
                            else (
                                phase[:16] & 0 | 20497
                                if phase < 289
                                else phase[:16] & 0 | 20498
                            )
                        )
                    )
                    if phase < 290
                    else (
                        (
                            phase[:16] & 0 | 20491
                            if phase < 291
                            else phase[:16] & 0 | 20495
                        )
                        if phase < 292
                        else (
                            phase[:16] & 0 | 0
                            if phase < 299
                            else (
                                phase[:16] & 0 | 20540
                                if phase < 300
                                else phase[:16] & 0 | 0
                            )
                        )
                    )
                )
            )
        )
        if phase < 301
        else (
            (
                (
                    (
                        (
                            phase[:16] & 0 | 20542
                            if phase < 302
                            else phase[:16] & 0 | 20543
                        )
                        if phase < 303
                        else (
                            phase[:16] & 0 | 0
                            if phase < 306
                            else phase[:16] & 0 | 33024
                        )
                    )
                    if phase < 307
                    else (
                        (
                            phase[:16] & 0 | 33025
                            if phase < 308
                            else phase[:16] & 0 | 33026
                        )
                        if phase < 309
                        else (
                            phase[:16] & 0 | 33027
                            if phase < 310
                            else (
                                phase[:16] & 0 | 0
                                if phase < 316
                                else phase[:16] & 0 | 20500
                            )
                        )
                    )
                )
                if phase < 411
                else (
                    (
                        (
                            phase[:16] & 0 | 20501
                            if phase < 412
                            else phase[:16] & 0 | 36864
                        )
                        if phase < 413
                        else (
                            phase[:16] & 0 | 36868
                            if phase < 414
                            else phase[:16] & 0 | 36872
                        )
                    )
                    if phase < 415
                    else (
                        (
                            phase[:16] & 0 | 36865
                            if phase < 416
                            else phase[:16] & 0 | 36869
                        )
                        if phase < 417
                        else (
                            phase[:16] & 0 | 36876
                            if phase < 418
                            else (
                                phase[:16] & 0 | 36873
                                if phase < 419
                                else phase[:16] & 0 | 0
                            )
                        )
                    )
                )
            )
            if phase < 420
            else (
                (
                    (
                        (
                            phase[:16] & 0 | 36880
                            if phase < 421
                            else phase[:16] & 0 | 36877
                        )
                        if phase < 422
                        else (
                            phase[:16] & 0 | 36866
                            if phase < 423
                            else phase[:16] & 0 | 36881
                        )
                    )
                    if phase < 424
                    else (
                        (
                            phase[:16] & 0 | 36870
                            if phase < 425
                            else phase[:16] & 0 | 36874
                        )
                        if phase < 426
                        else (
                            phase[:16] & 0 | 20503
                            if phase < 427
                            else (
                                phase[:16] & 0 | 36878
                                if phase < 428
                                else phase[:16] & 0 | 36867
                            )
                        )
                    )
                )
                if phase < 429
                else (
                    (
                        (
                            phase[:16] & 0 | 36871
                            if phase < 430
                            else phase[:16] & 0 | 36882
                        )
                        if phase < 431
                        else (
                            phase[:16] & 0 | 36875
                            if phase < 432
                            else (
                                phase[:16] & 0 | 0
                                if phase < 434
                                else phase[:16] & 0 | 36884
                            )
                        )
                    )
                    if phase < 435
                    else (
                        (
                            phase[:16] & 0 | 36885
                            if phase < 436
                            else phase[:16] & 0 | 36886
                        )
                        if phase < 437
                        else (
                            phase[:16] & 0 | 36879
                            if phase < 438
                            else (
                                phase[:16] & 0 | 36888
                                if phase < 439
                                else phase[:16] & 0 | 36889
                            )
                        )
                    )
                )
            )
        )
    )
    expected_response_tag = (
        (
            (
                (
                    (
                        (
                            (phase[:8] & 0 | 0 if phase < 7 else phase[:8] & 0 | 1)
                            if phase < 8
                            else phase[:8] & 0 | 2 if phase < 9 else phase[:8] & 0 | 0
                        )
                        if phase < 10
                        else (
                            (phase[:8] & 0 | 3 if phase < 11 else phase[:8] & 0 | 4)
                            if phase < 12
                            else phase[:8] & 0 | 5 if phase < 13 else phase[:8] & 0 | 0
                        )
                    )
                    if phase < 19
                    else (
                        (
                            (phase[:8] & 0 | 6 if phase < 20 else phase[:8] & 0 | 0)
                            if phase < 21
                            else phase[:8] & 0 | 8 if phase < 22 else phase[:8] & 0 | 9
                        )
                        if phase < 23
                        else (
                            (phase[:8] & 0 | 10 if phase < 24 else phase[:8] & 0 | 7)
                            if phase < 25
                            else (
                                phase[:8] & 0 | 0
                                if phase < 31
                                else (
                                    phase[:8] & 0 | 20
                                    if phase < 208
                                    else phase[:8] & 0 | 21
                                )
                            )
                        )
                    )
                )
                if phase < 209
                else (
                    (
                        (
                            (phase[:8] & 0 | 24 if phase < 210 else phase[:8] & 0 | 28)
                            if phase < 211
                            else (
                                phase[:8] & 0 | 32
                                if phase < 212
                                else phase[:8] & 0 | 25
                            )
                        )
                        if phase < 213
                        else (
                            (phase[:8] & 0 | 29 if phase < 214 else phase[:8] & 0 | 36)
                            if phase < 215
                            else (
                                phase[:8] & 0 | 33
                                if phase < 216
                                else (
                                    phase[:8] & 0 | 22
                                    if phase < 217
                                    else phase[:8] & 0 | 40
                                )
                            )
                        )
                    )
                    if phase < 218
                    else (
                        (
                            (phase[:8] & 0 | 37 if phase < 219 else phase[:8] & 0 | 26)
                            if phase < 220
                            else (
                                phase[:8] & 0 | 41
                                if phase < 221
                                else phase[:8] & 0 | 30
                            )
                        )
                        if phase < 222
                        else (
                            (phase[:8] & 0 | 34 if phase < 223 else phase[:8] & 0 | 23)
                            if phase < 224
                            else (
                                phase[:8] & 0 | 38
                                if phase < 225
                                else (
                                    phase[:8] & 0 | 27
                                    if phase < 226
                                    else phase[:8] & 0 | 31
                                )
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
                            (phase[:8] & 0 | 42 if phase < 228 else phase[:8] & 0 | 35)
                            if phase < 229
                            else (
                                phase[:8] & 0 | 0 if phase < 231 else phase[:8] & 0 | 44
                            )
                        )
                        if phase < 232
                        else (
                            (phase[:8] & 0 | 45 if phase < 233 else phase[:8] & 0 | 46)
                            if phase < 234
                            else (
                                phase[:8] & 0 | 39
                                if phase < 235
                                else phase[:8] & 0 | 48
                            )
                        )
                    )
                    if phase < 236
                    else (
                        (
                            (phase[:8] & 0 | 49 if phase < 237 else phase[:8] & 0 | 50)
                            if phase < 238
                            else (
                                phase[:8] & 0 | 43
                                if phase < 239
                                else phase[:8] & 0 | 52
                            )
                        )
                        if phase < 240
                        else (
                            (phase[:8] & 0 | 53 if phase < 241 else phase[:8] & 0 | 54)
                            if phase < 242
                            else (
                                phase[:8] & 0 | 47
                                if phase < 243
                                else (
                                    phase[:8] & 0 | 56
                                    if phase < 244
                                    else phase[:8] & 0 | 57
                                )
                            )
                        )
                    )
                )
                if phase < 245
                else (
                    (
                        (
                            (phase[:8] & 0 | 58 if phase < 246 else phase[:8] & 0 | 51)
                            if phase < 247
                            else (
                                phase[:8] & 0 | 60
                                if phase < 248
                                else phase[:8] & 0 | 61
                            )
                        )
                        if phase < 249
                        else (
                            (phase[:8] & 0 | 62 if phase < 250 else phase[:8] & 0 | 55)
                            if phase < 251
                            else (
                                phase[:8] & 0 | 64
                                if phase < 252
                                else (
                                    phase[:8] & 0 | 65
                                    if phase < 253
                                    else phase[:8] & 0 | 66
                                )
                            )
                        )
                    )
                    if phase < 254
                    else (
                        (
                            (phase[:8] & 0 | 59 if phase < 255 else phase[:8] & 0 | 68)
                            if phase < 256
                            else (
                                phase[:8] & 0 | 69
                                if phase < 257
                                else phase[:8] & 0 | 70
                            )
                        )
                        if phase < 258
                        else (
                            (phase[:8] & 0 | 63 if phase < 259 else phase[:8] & 0 | 72)
                            if phase < 260
                            else (
                                phase[:8] & 0 | 73
                                if phase < 261
                                else (
                                    phase[:8] & 0 | 74
                                    if phase < 262
                                    else phase[:8] & 0 | 67
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
                            (phase[:8] & 0 | 76 if phase < 264 else phase[:8] & 0 | 77)
                            if phase < 265
                            else (
                                phase[:8] & 0 | 78
                                if phase < 266
                                else phase[:8] & 0 | 71
                            )
                        )
                        if phase < 267
                        else (
                            (phase[:8] & 0 | 80 if phase < 268 else phase[:8] & 0 | 81)
                            if phase < 269
                            else (
                                phase[:8] & 0 | 82
                                if phase < 270
                                else phase[:8] & 0 | 75
                            )
                        )
                    )
                    if phase < 271
                    else (
                        (
                            (phase[:8] & 0 | 84 if phase < 272 else phase[:8] & 0 | 85)
                            if phase < 273
                            else (
                                phase[:8] & 0 | 86
                                if phase < 274
                                else phase[:8] & 0 | 79
                            )
                        )
                        if phase < 275
                        else (
                            (phase[:8] & 0 | 88 if phase < 276 else phase[:8] & 0 | 89)
                            if phase < 277
                            else (
                                phase[:8] & 0 | 90
                                if phase < 278
                                else (
                                    phase[:8] & 0 | 83
                                    if phase < 279
                                    else phase[:8] & 0 | 92
                                )
                            )
                        )
                    )
                )
                if phase < 280
                else (
                    (
                        (
                            (phase[:8] & 0 | 93 if phase < 281 else phase[:8] & 0 | 94)
                            if phase < 282
                            else (
                                phase[:8] & 0 | 87
                                if phase < 283
                                else phase[:8] & 0 | 96
                            )
                        )
                        if phase < 284
                        else (
                            (phase[:8] & 0 | 97 if phase < 285 else phase[:8] & 0 | 98)
                            if phase < 286
                            else (
                                phase[:8] & 0 | 91
                                if phase < 287
                                else (
                                    phase[:8] & 0 | 100
                                    if phase < 288
                                    else phase[:8] & 0 | 101
                                )
                            )
                        )
                    )
                    if phase < 289
                    else (
                        (
                            (phase[:8] & 0 | 102 if phase < 290 else phase[:8] & 0 | 95)
                            if phase < 291
                            else (
                                phase[:8] & 0 | 99
                                if phase < 292
                                else phase[:8] & 0 | 103
                            )
                        )
                        if phase < 293
                        else (
                            (phase[:8] & 0 | 0 if phase < 299 else phase[:8] & 0 | 120)
                            if phase < 300
                            else (
                                phase[:8] & 0 | 121
                                if phase < 301
                                else (
                                    phase[:8] & 0 | 122
                                    if phase < 302
                                    else phase[:8] & 0 | 123
                                )
                            )
                        )
                    )
                )
            )
            if phase < 303
            else (
                (
                    (
                        (
                            (phase[:8] & 0 | 0 if phase < 306 else phase[:8] & 0 | 124)
                            if phase < 307
                            else (
                                phase[:8] & 0 | 125
                                if phase < 308
                                else phase[:8] & 0 | 126
                            )
                        )
                        if phase < 309
                        else (
                            (phase[:8] & 0 | 127 if phase < 310 else phase[:8] & 0 | 0)
                            if phase < 316
                            else (
                                phase[:8] & 0 | 150
                                if phase < 411
                                else phase[:8] & 0 | 151
                            )
                        )
                    )
                    if phase < 412
                    else (
                        (
                            (
                                phase[:8] & 0 | 154
                                if phase < 413
                                else phase[:8] & 0 | 158
                            )
                            if phase < 414
                            else (
                                phase[:8] & 0 | 162
                                if phase < 415
                                else phase[:8] & 0 | 155
                            )
                        )
                        if phase < 416
                        else (
                            (
                                phase[:8] & 0 | 159
                                if phase < 417
                                else phase[:8] & 0 | 166
                            )
                            if phase < 418
                            else (
                                phase[:8] & 0 | 163
                                if phase < 419
                                else (
                                    phase[:8] & 0 | 152
                                    if phase < 420
                                    else phase[:8] & 0 | 170
                                )
                            )
                        )
                    )
                )
                if phase < 421
                else (
                    (
                        (
                            (
                                phase[:8] & 0 | 167
                                if phase < 422
                                else phase[:8] & 0 | 156
                            )
                            if phase < 423
                            else (
                                phase[:8] & 0 | 171
                                if phase < 424
                                else phase[:8] & 0 | 160
                            )
                        )
                        if phase < 425
                        else (
                            (
                                phase[:8] & 0 | 164
                                if phase < 426
                                else phase[:8] & 0 | 153
                            )
                            if phase < 427
                            else (
                                phase[:8] & 0 | 168
                                if phase < 428
                                else (
                                    phase[:8] & 0 | 157
                                    if phase < 429
                                    else phase[:8] & 0 | 161
                                )
                            )
                        )
                    )
                    if phase < 430
                    else (
                        (
                            (
                                phase[:8] & 0 | 172
                                if phase < 431
                                else phase[:8] & 0 | 165
                            )
                            if phase < 432
                            else (
                                phase[:8] & 0 | 0
                                if phase < 434
                                else phase[:8] & 0 | 174
                            )
                        )
                        if phase < 435
                        else (
                            (
                                phase[:8] & 0 | 175
                                if phase < 436
                                else phase[:8] & 0 | 176
                            )
                            if phase < 437
                            else (
                                phase[:8] & 0 | 169
                                if phase < 438
                                else (
                                    phase[:8] & 0 | 178
                                    if phase < 439
                                    else phase[:8] & 0 | 179
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    expected_route = (
        (
            (
                (
                    (
                        (
                            (phase[:4] & 0 | 0 if phase < 2 else phase[:4] & 0 | 1)
                            if phase < 3
                            else phase[:4] & 0 | 2 if phase < 4 else phase[:4] & 0 | 1
                        )
                        if phase < 5
                        else (
                            (phase[:4] & 0 | 2 if phase < 6 else phase[:4] & 0 | 4)
                            if phase < 7
                            else (
                                phase[:4] & 0 | 0
                                if phase < 14
                                else (
                                    phase[:4] & 0 | 8
                                    if phase < 16
                                    else phase[:4] & 0 | 1
                                )
                            )
                        )
                    )
                    if phase < 17
                    else (
                        (
                            (phase[:4] & 0 | 2 if phase < 18 else phase[:4] & 0 | 4)
                            if phase < 19
                            else phase[:4] & 0 | 0 if phase < 26 else phase[:4] & 0 | 1
                        )
                        if phase < 27
                        else (
                            (phase[:4] & 0 | 2 if phase < 28 else phase[:4] & 0 | 4)
                            if phase < 29
                            else (
                                phase[:4] & 0 | 8
                                if phase < 30
                                else (
                                    phase[:4] & 0 | 1
                                    if phase < 31
                                    else phase[:4] & 0 | 2
                                )
                            )
                        )
                    )
                )
                if phase < 32
                else (
                    (
                        (
                            (phase[:4] & 0 | 4 if phase < 33 else phase[:4] & 0 | 8)
                            if phase < 34
                            else phase[:4] & 0 | 1 if phase < 35 else phase[:4] & 0 | 2
                        )
                        if phase < 36
                        else (
                            (phase[:4] & 0 | 4 if phase < 37 else phase[:4] & 0 | 8)
                            if phase < 38
                            else (
                                phase[:4] & 0 | 1
                                if phase < 39
                                else (
                                    phase[:4] & 0 | 0
                                    if phase < 40
                                    else phase[:4] & 0 | 2
                                )
                            )
                        )
                    )
                    if phase < 41
                    else (
                        (
                            (phase[:4] & 0 | 4 if phase < 42 else phase[:4] & 0 | 8)
                            if phase < 43
                            else phase[:4] & 0 | 1 if phase < 44 else phase[:4] & 0 | 2
                        )
                        if phase < 45
                        else (
                            (phase[:4] & 0 | 4 if phase < 46 else phase[:4] & 0 | 8)
                            if phase < 47
                            else (
                                phase[:4] & 0 | 1
                                if phase < 48
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 49
                                    else phase[:4] & 0 | 0
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
                            (phase[:4] & 0 | 4 if phase < 219 else phase[:4] & 0 | 0)
                            if phase < 225
                            else phase[:4] & 0 | 8 if phase < 226 else phase[:4] & 0 | 1
                        )
                        if phase < 227
                        else (
                            (phase[:4] & 0 | 2 if phase < 228 else phase[:4] & 0 | 4)
                            if phase < 229
                            else (
                                phase[:4] & 0 | 8
                                if phase < 230
                                else (
                                    phase[:4] & 0 | 1
                                    if phase < 231
                                    else phase[:4] & 0 | 2
                                )
                            )
                        )
                    )
                    if phase < 232
                    else (
                        (
                            (phase[:4] & 0 | 4 if phase < 233 else phase[:4] & 0 | 8)
                            if phase < 234
                            else phase[:4] & 0 | 1 if phase < 235 else phase[:4] & 0 | 2
                        )
                        if phase < 236
                        else (
                            (phase[:4] & 0 | 4 if phase < 237 else phase[:4] & 0 | 8)
                            if phase < 238
                            else (
                                phase[:4] & 0 | 1
                                if phase < 239
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 240
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                )
                if phase < 241
                else (
                    (
                        (
                            (phase[:4] & 0 | 8 if phase < 242 else phase[:4] & 0 | 1)
                            if phase < 243
                            else phase[:4] & 0 | 2 if phase < 244 else phase[:4] & 0 | 4
                        )
                        if phase < 245
                        else (
                            (phase[:4] & 0 | 8 if phase < 246 else phase[:4] & 0 | 1)
                            if phase < 247
                            else (
                                phase[:4] & 0 | 2
                                if phase < 248
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 249
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                    )
                    if phase < 250
                    else (
                        (
                            (phase[:4] & 0 | 1 if phase < 251 else phase[:4] & 0 | 2)
                            if phase < 252
                            else phase[:4] & 0 | 4 if phase < 253 else phase[:4] & 0 | 8
                        )
                        if phase < 254
                        else (
                            (phase[:4] & 0 | 1 if phase < 255 else phase[:4] & 0 | 2)
                            if phase < 256
                            else (
                                phase[:4] & 0 | 4
                                if phase < 257
                                else (
                                    phase[:4] & 0 | 8
                                    if phase < 258
                                    else phase[:4] & 0 | 1
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
                            (phase[:4] & 0 | 2 if phase < 260 else phase[:4] & 0 | 4)
                            if phase < 261
                            else phase[:4] & 0 | 8 if phase < 262 else phase[:4] & 0 | 1
                        )
                        if phase < 263
                        else (
                            (phase[:4] & 0 | 2 if phase < 264 else phase[:4] & 0 | 4)
                            if phase < 265
                            else (
                                phase[:4] & 0 | 8
                                if phase < 266
                                else (
                                    phase[:4] & 0 | 1
                                    if phase < 267
                                    else phase[:4] & 0 | 2
                                )
                            )
                        )
                    )
                    if phase < 268
                    else (
                        (
                            (phase[:4] & 0 | 4 if phase < 269 else phase[:4] & 0 | 8)
                            if phase < 270
                            else phase[:4] & 0 | 1 if phase < 271 else phase[:4] & 0 | 2
                        )
                        if phase < 272
                        else (
                            (phase[:4] & 0 | 4 if phase < 273 else phase[:4] & 0 | 8)
                            if phase < 274
                            else (
                                phase[:4] & 0 | 1
                                if phase < 275
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 276
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                )
                if phase < 277
                else (
                    (
                        (
                            (phase[:4] & 0 | 8 if phase < 278 else phase[:4] & 0 | 1)
                            if phase < 279
                            else phase[:4] & 0 | 2 if phase < 280 else phase[:4] & 0 | 4
                        )
                        if phase < 281
                        else (
                            (phase[:4] & 0 | 8 if phase < 282 else phase[:4] & 0 | 1)
                            if phase < 283
                            else (
                                phase[:4] & 0 | 2
                                if phase < 284
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 285
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                    )
                    if phase < 286
                    else (
                        (
                            (phase[:4] & 0 | 0 if phase < 294 else phase[:4] & 0 | 1)
                            if phase < 295
                            else phase[:4] & 0 | 2 if phase < 296 else phase[:4] & 0 | 4
                        )
                        if phase < 297
                        else (
                            (phase[:4] & 0 | 8 if phase < 298 else phase[:4] & 0 | 0)
                            if phase < 301
                            else (
                                phase[:4] & 0 | 1
                                if phase < 302
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 303
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                )
            )
            if phase < 304
            else (
                (
                    (
                        (
                            (phase[:4] & 0 | 8 if phase < 305 else phase[:4] & 0 | 0)
                            if phase < 311
                            else phase[:4] & 0 | 1 if phase < 312 else phase[:4] & 0 | 2
                        )
                        if phase < 313
                        else (
                            (phase[:4] & 0 | 4 if phase < 314 else phase[:4] & 0 | 8)
                            if phase < 315
                            else (
                                phase[:4] & 0 | 1
                                if phase < 316
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 317
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                    if phase < 318
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 319 else phase[:4] & 0 | 1)
                            if phase < 320
                            else phase[:4] & 0 | 2 if phase < 321 else phase[:4] & 0 | 4
                        )
                        if phase < 322
                        else (
                            (phase[:4] & 0 | 8 if phase < 323 else phase[:4] & 0 | 1)
                            if phase < 324
                            else (
                                phase[:4] & 0 | 2
                                if phase < 325
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 326
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                    )
                )
                if phase < 327
                else (
                    (
                        (
                            (phase[:4] & 0 | 1 if phase < 328 else phase[:4] & 0 | 2)
                            if phase < 329
                            else phase[:4] & 0 | 4 if phase < 330 else phase[:4] & 0 | 8
                        )
                        if phase < 331
                        else (
                            (phase[:4] & 0 | 1 if phase < 332 else phase[:4] & 0 | 2)
                            if phase < 333
                            else (
                                phase[:4] & 0 | 0
                                if phase < 421
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 422
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                    )
                    if phase < 428
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 429 else phase[:4] & 0 | 1)
                            if phase < 430
                            else (
                                phase[:4] & 0 | 2
                                if phase < 431
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 432
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                        if phase < 433
                        else (
                            (phase[:4] & 0 | 1 if phase < 434 else phase[:4] & 0 | 2)
                            if phase < 435
                            else (
                                phase[:4] & 0 | 4
                                if phase < 436
                                else (
                                    phase[:4] & 0 | 8
                                    if phase < 437
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    expected_accepted = (
        (
            (
                (
                    (
                        (
                            (phase[:4] & 0 | 0 if phase < 3 else phase[:4] & 0 | 1)
                            if phase < 4
                            else phase[:4] & 0 | 2 if phase < 5 else phase[:4] & 0 | 0
                        )
                        if phase < 6
                        else (
                            (phase[:4] & 0 | 1 if phase < 7 else phase[:4] & 0 | 6)
                            if phase < 8
                            else (
                                phase[:4] & 0 | 0
                                if phase < 15
                                else (
                                    phase[:4] & 0 | 8
                                    if phase < 16
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                    )
                    if phase < 17
                    else (
                        (
                            (phase[:4] & 0 | 1 if phase < 18 else phase[:4] & 0 | 10)
                            if phase < 19
                            else phase[:4] & 0 | 4 if phase < 20 else phase[:4] & 0 | 0
                        )
                        if phase < 27
                        else (
                            (phase[:4] & 0 | 1 if phase < 28 else phase[:4] & 0 | 2)
                            if phase < 29
                            else (
                                phase[:4] & 0 | 4
                                if phase < 30
                                else (
                                    phase[:4] & 0 | 8
                                    if phase < 31
                                    else phase[:4] & 0 | 1
                                )
                            )
                        )
                    )
                )
                if phase < 32
                else (
                    (
                        (
                            (phase[:4] & 0 | 2 if phase < 33 else phase[:4] & 0 | 4)
                            if phase < 34
                            else phase[:4] & 0 | 8 if phase < 35 else phase[:4] & 0 | 1
                        )
                        if phase < 36
                        else (
                            (phase[:4] & 0 | 2 if phase < 37 else phase[:4] & 0 | 4)
                            if phase < 38
                            else (
                                phase[:4] & 0 | 8
                                if phase < 39
                                else (
                                    phase[:4] & 0 | 1
                                    if phase < 40
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                    )
                    if phase < 41
                    else (
                        (
                            (phase[:4] & 0 | 2 if phase < 42 else phase[:4] & 0 | 0)
                            if phase < 210
                            else (
                                phase[:4] & 0 | 1
                                if phase < 211
                                else (
                                    phase[:4] & 0 | 0
                                    if phase < 213
                                    else phase[:4] & 0 | 3
                                )
                            )
                        )
                        if phase < 214
                        else (
                            (phase[:4] & 0 | 0 if phase < 216 else phase[:4] & 0 | 2)
                            if phase < 217
                            else (
                                phase[:4] & 0 | 4
                                if phase < 218
                                else (
                                    phase[:4] & 0 | 0
                                    if phase < 220
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                )
            )
            if phase < 221
            else (
                (
                    (
                        (
                            (phase[:4] & 0 | 0 if phase < 223 else phase[:4] & 0 | 4)
                            if phase < 224
                            else phase[:4] & 0 | 8 if phase < 225 else phase[:4] & 0 | 0
                        )
                        if phase < 227
                        else (
                            (phase[:4] & 0 | 9 if phase < 228 else phase[:4] & 0 | 2)
                            if phase < 229
                            else (
                                phase[:4] & 0 | 4
                                if phase < 230
                                else (
                                    phase[:4] & 0 | 8
                                    if phase < 231
                                    else phase[:4] & 0 | 1
                                )
                            )
                        )
                    )
                    if phase < 232
                    else (
                        (
                            (phase[:4] & 0 | 2 if phase < 233 else phase[:4] & 0 | 12)
                            if phase < 234
                            else phase[:4] & 0 | 0 if phase < 235 else phase[:4] & 0 | 1
                        )
                        if phase < 236
                        else (
                            (phase[:4] & 0 | 10 if phase < 237 else phase[:4] & 0 | 4)
                            if phase < 238
                            else (
                                phase[:4] & 0 | 0
                                if phase < 239
                                else (
                                    phase[:4] & 0 | 9
                                    if phase < 240
                                    else phase[:4] & 0 | 2
                                )
                            )
                        )
                    )
                )
                if phase < 241
                else (
                    (
                        (
                            (phase[:4] & 0 | 4 if phase < 242 else phase[:4] & 0 | 0)
                            if phase < 243
                            else phase[:4] & 0 | 9 if phase < 244 else phase[:4] & 0 | 2
                        )
                        if phase < 245
                        else (
                            (phase[:4] & 0 | 4 if phase < 246 else phase[:4] & 0 | 0)
                            if phase < 247
                            else (
                                phase[:4] & 0 | 9
                                if phase < 248
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 249
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                    if phase < 250
                    else (
                        (
                            (phase[:4] & 0 | 0 if phase < 251 else phase[:4] & 0 | 9)
                            if phase < 252
                            else (
                                phase[:4] & 0 | 2
                                if phase < 253
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 254
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                        if phase < 255
                        else (
                            (phase[:4] & 0 | 9 if phase < 256 else phase[:4] & 0 | 2)
                            if phase < 257
                            else (
                                phase[:4] & 0 | 4
                                if phase < 258
                                else (
                                    phase[:4] & 0 | 0
                                    if phase < 259
                                    else phase[:4] & 0 | 9
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 260
        else (
            (
                (
                    (
                        (
                            (phase[:4] & 0 | 2 if phase < 261 else phase[:4] & 0 | 4)
                            if phase < 262
                            else phase[:4] & 0 | 0 if phase < 263 else phase[:4] & 0 | 9
                        )
                        if phase < 264
                        else (
                            (phase[:4] & 0 | 2 if phase < 265 else phase[:4] & 0 | 4)
                            if phase < 266
                            else (
                                phase[:4] & 0 | 0
                                if phase < 267
                                else (
                                    phase[:4] & 0 | 9
                                    if phase < 268
                                    else phase[:4] & 0 | 2
                                )
                            )
                        )
                    )
                    if phase < 269
                    else (
                        (
                            (phase[:4] & 0 | 4 if phase < 270 else phase[:4] & 0 | 0)
                            if phase < 271
                            else phase[:4] & 0 | 9 if phase < 272 else phase[:4] & 0 | 2
                        )
                        if phase < 273
                        else (
                            (phase[:4] & 0 | 4 if phase < 274 else phase[:4] & 0 | 0)
                            if phase < 275
                            else (
                                phase[:4] & 0 | 9
                                if phase < 276
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 277
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                )
                if phase < 278
                else (
                    (
                        (
                            (phase[:4] & 0 | 0 if phase < 279 else phase[:4] & 0 | 9)
                            if phase < 280
                            else phase[:4] & 0 | 2 if phase < 281 else phase[:4] & 0 | 4
                        )
                        if phase < 282
                        else (
                            (phase[:4] & 0 | 0 if phase < 283 else phase[:4] & 0 | 9)
                            if phase < 284
                            else (
                                phase[:4] & 0 | 2
                                if phase < 285
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 286
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                    )
                    if phase < 287
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 288 else phase[:4] & 0 | 0)
                            if phase < 295
                            else (
                                phase[:4] & 0 | 1
                                if phase < 296
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 297
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                        if phase < 298
                        else (
                            (phase[:4] & 0 | 8 if phase < 299 else phase[:4] & 0 | 0)
                            if phase < 302
                            else (
                                phase[:4] & 0 | 1
                                if phase < 303
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 304
                                    else phase[:4] & 0 | 4
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
                            (phase[:4] & 0 | 8 if phase < 306 else phase[:4] & 0 | 0)
                            if phase < 312
                            else phase[:4] & 0 | 1 if phase < 313 else phase[:4] & 0 | 2
                        )
                        if phase < 314
                        else (
                            (phase[:4] & 0 | 4 if phase < 315 else phase[:4] & 0 | 8)
                            if phase < 316
                            else (
                                phase[:4] & 0 | 1
                                if phase < 317
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 318
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                    if phase < 319
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 320 else phase[:4] & 0 | 1)
                            if phase < 321
                            else (
                                phase[:4] & 0 | 2
                                if phase < 322
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 323
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                        if phase < 324
                        else (
                            (phase[:4] & 0 | 1 if phase < 325 else phase[:4] & 0 | 2)
                            if phase < 326
                            else (
                                phase[:4] & 0 | 0
                                if phase < 413
                                else (
                                    phase[:4] & 0 | 1
                                    if phase < 414
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                    )
                )
                if phase < 416
                else (
                    (
                        (
                            (phase[:4] & 0 | 3 if phase < 417 else phase[:4] & 0 | 0)
                            if phase < 419
                            else phase[:4] & 0 | 2 if phase < 420 else phase[:4] & 0 | 4
                        )
                        if phase < 421
                        else (
                            (phase[:4] & 0 | 0 if phase < 423 else phase[:4] & 0 | 4)
                            if phase < 424
                            else (
                                phase[:4] & 0 | 0
                                if phase < 426
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 427
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                    )
                    if phase < 428
                    else (
                        (
                            (phase[:4] & 0 | 0 if phase < 430 else phase[:4] & 0 | 9)
                            if phase < 431
                            else (
                                phase[:4] & 0 | 2
                                if phase < 432
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 433
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                        if phase < 434
                        else (
                            (phase[:4] & 0 | 1 if phase < 435 else phase[:4] & 0 | 2)
                            if phase < 436
                            else (
                                phase[:4] & 0 | 12
                                if phase < 437
                                else (
                                    phase[:4] & 0 | 0
                                    if phase < 439
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    expected_enqueued = (
        (
            (
                (
                    (
                        (
                            (phase[:4] & 0 | 0 if phase < 5 else phase[:4] & 0 | 1)
                            if phase < 6
                            else phase[:4] & 0 | 2 if phase < 7 else phase[:4] & 0 | 0
                        )
                        if phase < 8
                        else (
                            (phase[:4] & 0 | 1 if phase < 9 else phase[:4] & 0 | 6)
                            if phase < 10
                            else (
                                phase[:4] & 0 | 0
                                if phase < 17
                                else (
                                    phase[:4] & 0 | 8
                                    if phase < 18
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                    )
                    if phase < 19
                    else (
                        (
                            (phase[:4] & 0 | 1 if phase < 20 else phase[:4] & 0 | 10)
                            if phase < 21
                            else phase[:4] & 0 | 4 if phase < 22 else phase[:4] & 0 | 0
                        )
                        if phase < 29
                        else (
                            (phase[:4] & 0 | 1 if phase < 30 else phase[:4] & 0 | 2)
                            if phase < 31
                            else (
                                phase[:4] & 0 | 4
                                if phase < 32
                                else (
                                    phase[:4] & 0 | 8
                                    if phase < 33
                                    else phase[:4] & 0 | 1
                                )
                            )
                        )
                    )
                )
                if phase < 34
                else (
                    (
                        (
                            (phase[:4] & 0 | 2 if phase < 35 else phase[:4] & 0 | 4)
                            if phase < 36
                            else phase[:4] & 0 | 8 if phase < 37 else phase[:4] & 0 | 1
                        )
                        if phase < 38
                        else (
                            (phase[:4] & 0 | 2 if phase < 39 else phase[:4] & 0 | 0)
                            if phase < 209
                            else (
                                phase[:4] & 0 | 1
                                if phase < 210
                                else (
                                    phase[:4] & 0 | 0
                                    if phase < 212
                                    else phase[:4] & 0 | 3
                                )
                            )
                        )
                    )
                    if phase < 213
                    else (
                        (
                            (phase[:4] & 0 | 0 if phase < 215 else phase[:4] & 0 | 3)
                            if phase < 216
                            else phase[:4] & 0 | 4 if phase < 217 else phase[:4] & 0 | 0
                        )
                        if phase < 218
                        else (
                            (phase[:4] & 0 | 2 if phase < 219 else phase[:4] & 0 | 4)
                            if phase < 220
                            else (
                                phase[:4] & 0 | 0
                                if phase < 222
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 223
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                    )
                )
            )
            if phase < 224
            else (
                (
                    (
                        (
                            (phase[:4] & 0 | 0 if phase < 225 else phase[:4] & 0 | 4)
                            if phase < 226
                            else phase[:4] & 0 | 8 if phase < 227 else phase[:4] & 0 | 0
                        )
                        if phase < 229
                        else (
                            (phase[:4] & 0 | 9 if phase < 230 else phase[:4] & 0 | 2)
                            if phase < 231
                            else (
                                phase[:4] & 0 | 4
                                if phase < 232
                                else (
                                    phase[:4] & 0 | 8
                                    if phase < 233
                                    else phase[:4] & 0 | 1
                                )
                            )
                        )
                    )
                    if phase < 234
                    else (
                        (
                            (phase[:4] & 0 | 2 if phase < 235 else phase[:4] & 0 | 12)
                            if phase < 236
                            else phase[:4] & 0 | 0 if phase < 237 else phase[:4] & 0 | 1
                        )
                        if phase < 238
                        else (
                            (phase[:4] & 0 | 10 if phase < 239 else phase[:4] & 0 | 4)
                            if phase < 240
                            else (
                                phase[:4] & 0 | 0
                                if phase < 241
                                else (
                                    phase[:4] & 0 | 1
                                    if phase < 242
                                    else phase[:4] & 0 | 10
                                )
                            )
                        )
                    )
                )
                if phase < 243
                else (
                    (
                        (
                            (phase[:4] & 0 | 4 if phase < 244 else phase[:4] & 0 | 0)
                            if phase < 245
                            else (
                                phase[:4] & 0 | 1 if phase < 246 else phase[:4] & 0 | 10
                            )
                        )
                        if phase < 247
                        else (
                            (phase[:4] & 0 | 4 if phase < 248 else phase[:4] & 0 | 0)
                            if phase < 249
                            else (
                                phase[:4] & 0 | 1
                                if phase < 250
                                else (
                                    phase[:4] & 0 | 10
                                    if phase < 251
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                    if phase < 252
                    else (
                        (
                            (phase[:4] & 0 | 0 if phase < 253 else phase[:4] & 0 | 1)
                            if phase < 254
                            else (
                                phase[:4] & 0 | 10
                                if phase < 255
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 256
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                        if phase < 257
                        else (
                            (phase[:4] & 0 | 1 if phase < 258 else phase[:4] & 0 | 10)
                            if phase < 259
                            else (
                                phase[:4] & 0 | 4
                                if phase < 260
                                else (
                                    phase[:4] & 0 | 0
                                    if phase < 261
                                    else phase[:4] & 0 | 1
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 262
        else (
            (
                (
                    (
                        (
                            (phase[:4] & 0 | 10 if phase < 263 else phase[:4] & 0 | 4)
                            if phase < 264
                            else phase[:4] & 0 | 0 if phase < 265 else phase[:4] & 0 | 1
                        )
                        if phase < 266
                        else (
                            (phase[:4] & 0 | 10 if phase < 267 else phase[:4] & 0 | 4)
                            if phase < 268
                            else (
                                phase[:4] & 0 | 0
                                if phase < 269
                                else (
                                    phase[:4] & 0 | 1
                                    if phase < 270
                                    else phase[:4] & 0 | 10
                                )
                            )
                        )
                    )
                    if phase < 271
                    else (
                        (
                            (phase[:4] & 0 | 4 if phase < 272 else phase[:4] & 0 | 0)
                            if phase < 273
                            else (
                                phase[:4] & 0 | 1 if phase < 274 else phase[:4] & 0 | 10
                            )
                        )
                        if phase < 275
                        else (
                            (phase[:4] & 0 | 4 if phase < 276 else phase[:4] & 0 | 0)
                            if phase < 277
                            else (
                                phase[:4] & 0 | 1
                                if phase < 278
                                else (
                                    phase[:4] & 0 | 10
                                    if phase < 279
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                )
                if phase < 280
                else (
                    (
                        (
                            (phase[:4] & 0 | 0 if phase < 281 else phase[:4] & 0 | 1)
                            if phase < 282
                            else (
                                phase[:4] & 0 | 10 if phase < 283 else phase[:4] & 0 | 4
                            )
                        )
                        if phase < 284
                        else (
                            (phase[:4] & 0 | 0 if phase < 285 else phase[:4] & 0 | 1)
                            if phase < 286
                            else (
                                phase[:4] & 0 | 10
                                if phase < 287
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 288
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                    )
                    if phase < 290
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 291 else phase[:4] & 0 | 0)
                            if phase < 297
                            else (
                                phase[:4] & 0 | 1
                                if phase < 298
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 299
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                        if phase < 300
                        else (
                            (phase[:4] & 0 | 8 if phase < 301 else phase[:4] & 0 | 0)
                            if phase < 304
                            else (
                                phase[:4] & 0 | 1
                                if phase < 305
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 306
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                )
            )
            if phase < 307
            else (
                (
                    (
                        (
                            (phase[:4] & 0 | 8 if phase < 308 else phase[:4] & 0 | 0)
                            if phase < 314
                            else phase[:4] & 0 | 1 if phase < 315 else phase[:4] & 0 | 2
                        )
                        if phase < 316
                        else (
                            (phase[:4] & 0 | 4 if phase < 317 else phase[:4] & 0 | 8)
                            if phase < 318
                            else (
                                phase[:4] & 0 | 1
                                if phase < 319
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 320
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                    if phase < 321
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 322 else phase[:4] & 0 | 1)
                            if phase < 323
                            else phase[:4] & 0 | 2 if phase < 324 else phase[:4] & 0 | 0
                        )
                        if phase < 412
                        else (
                            (phase[:4] & 0 | 1 if phase < 413 else phase[:4] & 0 | 0)
                            if phase < 415
                            else (
                                phase[:4] & 0 | 3
                                if phase < 416
                                else (
                                    phase[:4] & 0 | 0
                                    if phase < 418
                                    else phase[:4] & 0 | 3
                                )
                            )
                        )
                    )
                )
                if phase < 419
                else (
                    (
                        (
                            (phase[:4] & 0 | 4 if phase < 420 else phase[:4] & 0 | 0)
                            if phase < 421
                            else phase[:4] & 0 | 2 if phase < 422 else phase[:4] & 0 | 4
                        )
                        if phase < 423
                        else (
                            (phase[:4] & 0 | 0 if phase < 425 else phase[:4] & 0 | 4)
                            if phase < 426
                            else (
                                phase[:4] & 0 | 8
                                if phase < 427
                                else (
                                    phase[:4] & 0 | 0
                                    if phase < 428
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                    if phase < 429
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 430 else phase[:4] & 0 | 0)
                            if phase < 432
                            else (
                                phase[:4] & 0 | 9
                                if phase < 433
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 434
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                        if phase < 435
                        else (
                            (phase[:4] & 0 | 8 if phase < 436 else phase[:4] & 0 | 1)
                            if phase < 437
                            else (
                                phase[:4] & 0 | 2
                                if phase < 438
                                else (
                                    phase[:4] & 0 | 12
                                    if phase < 439
                                    else phase[:4] & 0 | 0
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    expected_response_valid = (
        (
            (
                (
                    (
                        (
                            (phase[:4] & 0 | 0 if phase < 6 else phase[:4] & 0 | 1)
                            if phase < 7
                            else phase[:4] & 0 | 2 if phase < 8 else phase[:4] & 0 | 0
                        )
                        if phase < 9
                        else (
                            (phase[:4] & 0 | 1 if phase < 10 else phase[:4] & 0 | 6)
                            if phase < 11
                            else phase[:4] & 0 | 4 if phase < 12 else phase[:4] & 0 | 0
                        )
                    )
                    if phase < 18
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 19 else phase[:4] & 0 | 0)
                            if phase < 20
                            else phase[:4] & 0 | 1 if phase < 21 else phase[:4] & 0 | 10
                        )
                        if phase < 22
                        else (
                            (phase[:4] & 0 | 12 if phase < 23 else phase[:4] & 0 | 8)
                            if phase < 24
                            else phase[:4] & 0 | 0 if phase < 30 else phase[:4] & 0 | 1
                        )
                    )
                )
                if phase < 31
                else (
                    (
                        (
                            (phase[:4] & 0 | 2 if phase < 32 else phase[:4] & 0 | 4)
                            if phase < 33
                            else (
                                phase[:4] & 0 | 12 if phase < 34 else phase[:4] & 0 | 13
                            )
                        )
                        if phase < 35
                        else (
                            (phase[:4] & 0 | 15 if phase < 211 else phase[:4] & 0 | 14)
                            if phase < 213
                            else (
                                phase[:4] & 0 | 15
                                if phase < 214
                                else phase[:4] & 0 | 14
                            )
                        )
                    )
                    if phase < 215
                    else (
                        (
                            (phase[:4] & 0 | 12 if phase < 216 else phase[:4] & 0 | 15)
                            if phase < 217
                            else (
                                phase[:4] & 0 | 14
                                if phase < 218
                                else phase[:4] & 0 | 12
                            )
                        )
                        if phase < 219
                        else (
                            (phase[:4] & 0 | 14 if phase < 220 else phase[:4] & 0 | 12)
                            if phase < 222
                            else (
                                phase[:4] & 0 | 8
                                if phase < 223
                                else (
                                    phase[:4] & 0 | 12
                                    if phase < 224
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                    )
                )
            )
            if phase < 226
            else (
                (
                    (
                        (
                            (phase[:4] & 0 | 4 if phase < 227 else phase[:4] & 0 | 8)
                            if phase < 228
                            else phase[:4] & 0 | 0 if phase < 230 else phase[:4] & 0 | 9
                        )
                        if phase < 231
                        else (
                            (phase[:4] & 0 | 10 if phase < 232 else phase[:4] & 0 | 12)
                            if phase < 233
                            else phase[:4] & 0 | 8 if phase < 234 else phase[:4] & 0 | 9
                        )
                    )
                    if phase < 235
                    else (
                        (
                            (phase[:4] & 0 | 10 if phase < 236 else phase[:4] & 0 | 12)
                            if phase < 237
                            else phase[:4] & 0 | 8 if phase < 238 else phase[:4] & 0 | 9
                        )
                        if phase < 239
                        else (
                            (phase[:4] & 0 | 10 if phase < 240 else phase[:4] & 0 | 12)
                            if phase < 241
                            else (
                                phase[:4] & 0 | 8
                                if phase < 242
                                else (
                                    phase[:4] & 0 | 9
                                    if phase < 243
                                    else phase[:4] & 0 | 10
                                )
                            )
                        )
                    )
                )
                if phase < 244
                else (
                    (
                        (
                            (phase[:4] & 0 | 12 if phase < 245 else phase[:4] & 0 | 8)
                            if phase < 246
                            else (
                                phase[:4] & 0 | 9 if phase < 247 else phase[:4] & 0 | 10
                            )
                        )
                        if phase < 248
                        else (
                            (phase[:4] & 0 | 12 if phase < 249 else phase[:4] & 0 | 8)
                            if phase < 250
                            else (
                                phase[:4] & 0 | 9 if phase < 251 else phase[:4] & 0 | 10
                            )
                        )
                    )
                    if phase < 252
                    else (
                        (
                            (phase[:4] & 0 | 12 if phase < 253 else phase[:4] & 0 | 8)
                            if phase < 254
                            else (
                                phase[:4] & 0 | 9 if phase < 255 else phase[:4] & 0 | 10
                            )
                        )
                        if phase < 256
                        else (
                            (phase[:4] & 0 | 12 if phase < 257 else phase[:4] & 0 | 8)
                            if phase < 258
                            else (
                                phase[:4] & 0 | 9
                                if phase < 259
                                else (
                                    phase[:4] & 0 | 10
                                    if phase < 260
                                    else phase[:4] & 0 | 12
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 261
        else (
            (
                (
                    (
                        (
                            (phase[:4] & 0 | 8 if phase < 262 else phase[:4] & 0 | 9)
                            if phase < 263
                            else (
                                phase[:4] & 0 | 10
                                if phase < 264
                                else phase[:4] & 0 | 12
                            )
                        )
                        if phase < 265
                        else (
                            (phase[:4] & 0 | 8 if phase < 266 else phase[:4] & 0 | 9)
                            if phase < 267
                            else (
                                phase[:4] & 0 | 10
                                if phase < 268
                                else phase[:4] & 0 | 12
                            )
                        )
                    )
                    if phase < 269
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 270 else phase[:4] & 0 | 9)
                            if phase < 271
                            else (
                                phase[:4] & 0 | 10
                                if phase < 272
                                else phase[:4] & 0 | 12
                            )
                        )
                        if phase < 273
                        else (
                            (phase[:4] & 0 | 8 if phase < 274 else phase[:4] & 0 | 9)
                            if phase < 275
                            else (
                                phase[:4] & 0 | 10
                                if phase < 276
                                else (
                                    phase[:4] & 0 | 12
                                    if phase < 277
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                    )
                )
                if phase < 278
                else (
                    (
                        (
                            (phase[:4] & 0 | 9 if phase < 279 else phase[:4] & 0 | 10)
                            if phase < 280
                            else (
                                phase[:4] & 0 | 12 if phase < 281 else phase[:4] & 0 | 8
                            )
                        )
                        if phase < 282
                        else (
                            (phase[:4] & 0 | 9 if phase < 283 else phase[:4] & 0 | 10)
                            if phase < 284
                            else (
                                phase[:4] & 0 | 12 if phase < 285 else phase[:4] & 0 | 8
                            )
                        )
                    )
                    if phase < 286
                    else (
                        (
                            (phase[:4] & 0 | 9 if phase < 287 else phase[:4] & 0 | 10)
                            if phase < 288
                            else (
                                phase[:4] & 0 | 12 if phase < 289 else phase[:4] & 0 | 8
                            )
                        )
                        if phase < 292
                        else (
                            (phase[:4] & 0 | 0 if phase < 298 else phase[:4] & 0 | 1)
                            if phase < 299
                            else (
                                phase[:4] & 0 | 2
                                if phase < 300
                                else (
                                    phase[:4] & 0 | 4
                                    if phase < 301
                                    else phase[:4] & 0 | 8
                                )
                            )
                        )
                    )
                )
            )
            if phase < 302
            else (
                (
                    (
                        (
                            (phase[:4] & 0 | 0 if phase < 305 else phase[:4] & 0 | 1)
                            if phase < 306
                            else phase[:4] & 0 | 2 if phase < 307 else phase[:4] & 0 | 4
                        )
                        if phase < 308
                        else (
                            (phase[:4] & 0 | 8 if phase < 309 else phase[:4] & 0 | 0)
                            if phase < 315
                            else phase[:4] & 0 | 1 if phase < 316 else phase[:4] & 0 | 2
                        )
                    )
                    if phase < 317
                    else (
                        (
                            (phase[:4] & 0 | 4 if phase < 318 else phase[:4] & 0 | 12)
                            if phase < 319
                            else (
                                phase[:4] & 0 | 13
                                if phase < 320
                                else phase[:4] & 0 | 15
                            )
                        )
                        if phase < 414
                        else (
                            (phase[:4] & 0 | 14 if phase < 416 else phase[:4] & 0 | 15)
                            if phase < 417
                            else (
                                phase[:4] & 0 | 14
                                if phase < 418
                                else (
                                    phase[:4] & 0 | 12
                                    if phase < 419
                                    else phase[:4] & 0 | 15
                                )
                            )
                        )
                    )
                )
                if phase < 420
                else (
                    (
                        (
                            (phase[:4] & 0 | 14 if phase < 421 else phase[:4] & 0 | 12)
                            if phase < 422
                            else (
                                phase[:4] & 0 | 14
                                if phase < 423
                                else phase[:4] & 0 | 12
                            )
                        )
                        if phase < 425
                        else (
                            (phase[:4] & 0 | 8 if phase < 426 else phase[:4] & 0 | 12)
                            if phase < 427
                            else phase[:4] & 0 | 8 if phase < 429 else phase[:4] & 0 | 4
                        )
                    )
                    if phase < 430
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 431 else phase[:4] & 0 | 0)
                            if phase < 433
                            else (
                                phase[:4] & 0 | 9 if phase < 434 else phase[:4] & 0 | 10
                            )
                        )
                        if phase < 435
                        else (
                            (phase[:4] & 0 | 12 if phase < 436 else phase[:4] & 0 | 8)
                            if phase < 437
                            else (
                                phase[:4] & 0 | 9
                                if phase < 438
                                else (
                                    phase[:4] & 0 | 10
                                    if phase < 439
                                    else phase[:4] & 0 | 12
                                )
                            )
                        )
                    )
                )
            )
        )
    )
    expected_merged = (
        (
            (
                (
                    (
                        (
                            (phase[:4] & 0 | 0 if phase < 6 else phase[:4] & 0 | 1)
                            if phase < 7
                            else phase[:4] & 0 | 2 if phase < 8 else phase[:4] & 0 | 0
                        )
                        if phase < 9
                        else (
                            (phase[:4] & 0 | 1 if phase < 10 else phase[:4] & 0 | 2)
                            if phase < 11
                            else phase[:4] & 0 | 4 if phase < 12 else phase[:4] & 0 | 0
                        )
                    )
                    if phase < 18
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 19 else phase[:4] & 0 | 0)
                            if phase < 20
                            else phase[:4] & 0 | 1 if phase < 21 else phase[:4] & 0 | 2
                        )
                        if phase < 22
                        else (
                            (phase[:4] & 0 | 4 if phase < 23 else phase[:4] & 0 | 8)
                            if phase < 24
                            else phase[:4] & 0 | 0 if phase < 30 else phase[:4] & 0 | 1
                        )
                    )
                )
                if phase < 31
                else (
                    (
                        (
                            (phase[:4] & 0 | 2 if phase < 32 else phase[:4] & 0 | 0)
                            if phase < 208
                            else phase[:4] & 0 | 1 if phase < 211 else phase[:4] & 0 | 2
                        )
                        if phase < 213
                        else (
                            (phase[:4] & 0 | 1 if phase < 214 else phase[:4] & 0 | 2)
                            if phase < 215
                            else phase[:4] & 0 | 4 if phase < 216 else phase[:4] & 0 | 1
                        )
                    )
                    if phase < 217
                    else (
                        (
                            (phase[:4] & 0 | 2 if phase < 218 else phase[:4] & 0 | 4)
                            if phase < 219
                            else phase[:4] & 0 | 2 if phase < 220 else phase[:4] & 0 | 4
                        )
                        if phase < 222
                        else (
                            (phase[:4] & 0 | 8 if phase < 223 else phase[:4] & 0 | 4)
                            if phase < 224
                            else phase[:4] & 0 | 8 if phase < 226 else phase[:4] & 0 | 4
                        )
                    )
                )
            )
            if phase < 227
            else (
                (
                    (
                        (
                            (phase[:4] & 0 | 8 if phase < 228 else phase[:4] & 0 | 0)
                            if phase < 230
                            else phase[:4] & 0 | 1 if phase < 231 else phase[:4] & 0 | 2
                        )
                        if phase < 232
                        else (
                            (phase[:4] & 0 | 4 if phase < 233 else phase[:4] & 0 | 8)
                            if phase < 234
                            else phase[:4] & 0 | 1 if phase < 235 else phase[:4] & 0 | 2
                        )
                    )
                    if phase < 236
                    else (
                        (
                            (phase[:4] & 0 | 4 if phase < 237 else phase[:4] & 0 | 8)
                            if phase < 238
                            else phase[:4] & 0 | 1 if phase < 239 else phase[:4] & 0 | 2
                        )
                        if phase < 240
                        else (
                            (phase[:4] & 0 | 4 if phase < 241 else phase[:4] & 0 | 8)
                            if phase < 242
                            else phase[:4] & 0 | 1 if phase < 243 else phase[:4] & 0 | 2
                        )
                    )
                )
                if phase < 244
                else (
                    (
                        (
                            (phase[:4] & 0 | 4 if phase < 245 else phase[:4] & 0 | 8)
                            if phase < 246
                            else phase[:4] & 0 | 1 if phase < 247 else phase[:4] & 0 | 2
                        )
                        if phase < 248
                        else (
                            (phase[:4] & 0 | 4 if phase < 249 else phase[:4] & 0 | 8)
                            if phase < 250
                            else phase[:4] & 0 | 1 if phase < 251 else phase[:4] & 0 | 2
                        )
                    )
                    if phase < 252
                    else (
                        (
                            (phase[:4] & 0 | 4 if phase < 253 else phase[:4] & 0 | 8)
                            if phase < 254
                            else phase[:4] & 0 | 1 if phase < 255 else phase[:4] & 0 | 2
                        )
                        if phase < 256
                        else (
                            (phase[:4] & 0 | 4 if phase < 257 else phase[:4] & 0 | 8)
                            if phase < 258
                            else (
                                phase[:4] & 0 | 1
                                if phase < 259
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 260
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 261
        else (
            (
                (
                    (
                        (
                            (phase[:4] & 0 | 8 if phase < 262 else phase[:4] & 0 | 1)
                            if phase < 263
                            else phase[:4] & 0 | 2 if phase < 264 else phase[:4] & 0 | 4
                        )
                        if phase < 265
                        else (
                            (phase[:4] & 0 | 8 if phase < 266 else phase[:4] & 0 | 1)
                            if phase < 267
                            else phase[:4] & 0 | 2 if phase < 268 else phase[:4] & 0 | 4
                        )
                    )
                    if phase < 269
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 270 else phase[:4] & 0 | 1)
                            if phase < 271
                            else phase[:4] & 0 | 2 if phase < 272 else phase[:4] & 0 | 4
                        )
                        if phase < 273
                        else (
                            (phase[:4] & 0 | 8 if phase < 274 else phase[:4] & 0 | 1)
                            if phase < 275
                            else phase[:4] & 0 | 2 if phase < 276 else phase[:4] & 0 | 4
                        )
                    )
                )
                if phase < 277
                else (
                    (
                        (
                            (phase[:4] & 0 | 8 if phase < 278 else phase[:4] & 0 | 1)
                            if phase < 279
                            else phase[:4] & 0 | 2 if phase < 280 else phase[:4] & 0 | 4
                        )
                        if phase < 281
                        else (
                            (phase[:4] & 0 | 8 if phase < 282 else phase[:4] & 0 | 1)
                            if phase < 283
                            else phase[:4] & 0 | 2 if phase < 284 else phase[:4] & 0 | 4
                        )
                    )
                    if phase < 285
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 286 else phase[:4] & 0 | 1)
                            if phase < 287
                            else phase[:4] & 0 | 2 if phase < 288 else phase[:4] & 0 | 4
                        )
                        if phase < 289
                        else (
                            (phase[:4] & 0 | 8 if phase < 292 else phase[:4] & 0 | 0)
                            if phase < 298
                            else (
                                phase[:4] & 0 | 1
                                if phase < 299
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 300
                                    else phase[:4] & 0 | 4
                                )
                            )
                        )
                    )
                )
            )
            if phase < 301
            else (
                (
                    (
                        (
                            (phase[:4] & 0 | 8 if phase < 302 else phase[:4] & 0 | 0)
                            if phase < 305
                            else phase[:4] & 0 | 1 if phase < 306 else phase[:4] & 0 | 2
                        )
                        if phase < 307
                        else (
                            (phase[:4] & 0 | 4 if phase < 308 else phase[:4] & 0 | 8)
                            if phase < 309
                            else phase[:4] & 0 | 0 if phase < 315 else phase[:4] & 0 | 1
                        )
                    )
                    if phase < 316
                    else (
                        (
                            (phase[:4] & 0 | 2 if phase < 317 else phase[:4] & 0 | 0)
                            if phase < 411
                            else phase[:4] & 0 | 1 if phase < 414 else phase[:4] & 0 | 2
                        )
                        if phase < 416
                        else (
                            (phase[:4] & 0 | 1 if phase < 417 else phase[:4] & 0 | 2)
                            if phase < 418
                            else phase[:4] & 0 | 4 if phase < 419 else phase[:4] & 0 | 1
                        )
                    )
                )
                if phase < 420
                else (
                    (
                        (
                            (phase[:4] & 0 | 2 if phase < 421 else phase[:4] & 0 | 4)
                            if phase < 422
                            else phase[:4] & 0 | 2 if phase < 423 else phase[:4] & 0 | 4
                        )
                        if phase < 425
                        else (
                            (phase[:4] & 0 | 8 if phase < 426 else phase[:4] & 0 | 4)
                            if phase < 427
                            else phase[:4] & 0 | 8 if phase < 429 else phase[:4] & 0 | 4
                        )
                    )
                    if phase < 430
                    else (
                        (
                            (phase[:4] & 0 | 8 if phase < 431 else phase[:4] & 0 | 0)
                            if phase < 433
                            else phase[:4] & 0 | 1 if phase < 434 else phase[:4] & 0 | 2
                        )
                        if phase < 435
                        else (
                            (phase[:4] & 0 | 4 if phase < 436 else phase[:4] & 0 | 8)
                            if phase < 437
                            else (
                                phase[:4] & 0 | 1
                                if phase < 438
                                else (
                                    phase[:4] & 0 | 2
                                    if phase < 439
                                    else phase[:4] & 0 | 4
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
        if phase < 440:
            assert dut.input_ready == expected_input_ready, "memory_banks: input_ready"
            assert (
                dut.output_valid == expected_output_valid
            ), "memory_banks: output_valid"
            assert (expected_output_valid == 0) | (
                dut.response.bank == expected_response_bank
            ), "memory_banks: response.bank"
            assert (expected_output_valid == 0) | (
                dut.response.offset == expected_response_offset
            ), "memory_banks: response.offset"
            assert (expected_output_valid == 0) | (
                dut.response.write == expected_response_write
            ), "memory_banks: response.write"
            assert (expected_output_valid == 0) | (
                dut.response.data == expected_response_data
            ), "memory_banks: response.data"
            assert (expected_output_valid == 0) | (
                dut.response.tag == expected_response_tag
            ), "memory_banks: response.tag"
            assert dut.route == expected_route, "memory_banks: route"
            assert dut.accepted == expected_accepted, "memory_banks: accepted"
            assert dut.enqueued == expected_enqueued, "memory_banks: enqueued"
            assert (
                dut.response_valid == expected_response_valid
            ), "memory_banks: response_valid"
            assert dut.merged == expected_merged, "memory_banks: merged"
        log("info", "memory_banks.input_ready", dut.input_ready)
        log("info", "memory_banks.output_valid", dut.output_valid)
        log("info", "memory_banks.response.bank", dut.response.bank)
        log("info", "memory_banks.response.offset", dut.response.offset)
        log("info", "memory_banks.response.write", dut.response.write)
        log("info", "memory_banks.response.data", dut.response.data)
        log("info", "memory_banks.response.tag", dut.response.tag)
        log("info", "memory_banks.route", dut.route)
        log("info", "memory_banks.accepted", dut.accepted)
        log("info", "memory_banks.enqueued", dut.enqueued)
        log("info", "memory_banks.response_valid", dut.response_valid)
        log("info", "memory_banks.merged", dut.merged)

    check()
    advance(phase)

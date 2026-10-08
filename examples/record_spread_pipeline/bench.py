"""Regular-clock full known-input stream from the retained record_spread_pipeline oracle."""

from example_record_spread_pipeline.record_spread_pipeline import (
    RecordSpreadPipeline,
    Packet,
    Header,
    Payload,
    Patch,
)
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseRecordSpreadPipeline():  # noqa: N802
    phase: bits[64] = 0
    base_valid = (
        (
            (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 3 else ((phase[:1] & 0) | 1))
                        if phase < 4
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 8
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 10
                    else (
                        (((phase[:1] & 0) | 0) if phase < 15 else ((phase[:1] & 0) | 1))
                        if phase < 17
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 22
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                )
                if phase < 24
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 29 else ((phase[:1] & 0) | 1))
                        if phase < 35
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 36
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 37
                    else (
                        (((phase[:1] & 0) | 0) if phase < 40 else ((phase[:1] & 0) | 1))
                        if phase < 41
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 44
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 45
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 48
            else (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 49 else ((phase[:1] & 0) | 0))
                        if phase < 52
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 53
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 56
                    else (
                        (((phase[:1] & 0) | 1) if phase < 57 else ((phase[:1] & 0) | 0))
                        if phase < 60
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 61
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 64
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 65
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 68 else ((phase[:1] & 0) | 1))
                        if phase < 69
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 72
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 73
                    else (
                        (((phase[:1] & 0) | 0) if phase < 76 else ((phase[:1] & 0) | 1))
                        if phase < 77
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 80
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 81
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 84
        else (
            (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 85 else ((phase[:1] & 0) | 0))
                        if phase < 88
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 89
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 92
                    else (
                        (((phase[:1] & 0) | 1) if phase < 93 else ((phase[:1] & 0) | 0))
                        if phase < 96
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 97
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 100
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 101
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 104
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 105
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 108
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 109
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 112
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 113
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 116
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 117
                                else ((phase[:1] & 0) | 0)
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
                            ((phase[:1] & 0) | 1)
                            if phase < 121
                            else ((phase[:1] & 0) | 0)
                        )
                        if phase < 124
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 125
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 128
                    else (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 129
                            else ((phase[:1] & 0) | 0)
                        )
                        if phase < 132
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 133
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 136
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 137
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 140
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 141
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 144
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 145
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 148
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 149
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 152
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 153
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    base_opcode = (
        (
            (
                (
                    (((phase[:4] & 0) | 0) if phase < 2 else ((phase[:4] & 0) | 15))
                    if phase < 3
                    else (
                        ((phase[:4] & 0) | 0) if phase < 8 else ((phase[:4] & 0) | 15)
                    )
                )
                if phase < 11
                else (
                    (((phase[:4] & 0) | 0) if phase < 15 else ((phase[:4] & 0) | 15))
                    if phase < 16
                    else (
                        ((phase[:4] & 0) | 5) if phase < 17 else ((phase[:4] & 0) | 15)
                    )
                )
            )
            if phase < 18
            else (
                (
                    (((phase[:4] & 0) | 0) if phase < 22 else ((phase[:4] & 0) | 5))
                    if phase < 23
                    else (
                        ((phase[:4] & 0) | 8) if phase < 24 else ((phase[:4] & 0) | 5)
                    )
                )
                if phase < 25
                else (
                    (((phase[:4] & 0) | 0) if phase < 30 else ((phase[:4] & 0) | 15))
                    if phase < 32
                    else (
                        ((phase[:4] & 0) | 5) if phase < 34 else ((phase[:4] & 0) | 8)
                    )
                )
            )
        )
        if phase < 35
        else (
            (
                (
                    (((phase[:4] & 0) | 0) if phase < 40 else ((phase[:4] & 0) | 15))
                    if phase < 41
                    else (
                        ((phase[:4] & 0) | 0) if phase < 44 else ((phase[:4] & 0) | 15)
                    )
                )
                if phase < 45
                else (
                    (((phase[:4] & 0) | 0) if phase < 48 else ((phase[:4] & 0) | 5))
                    if phase < 49
                    else (
                        ((phase[:4] & 0) | 0) if phase < 52 else ((phase[:4] & 0) | 8)
                    )
                )
            )
            if phase < 53
            else (
                (
                    (((phase[:4] & 0) | 0) if phase < 140 else ((phase[:4] & 0) | 1))
                    if phase < 141
                    else (
                        ((phase[:4] & 0) | 0) if phase < 144 else ((phase[:4] & 0) | 2)
                    )
                )
                if phase < 145
                else (
                    (((phase[:4] & 0) | 0) if phase < 148 else ((phase[:4] & 0) | 4))
                    if phase < 149
                    else (
                        ((phase[:4] & 0) | 0)
                        if phase < 152
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 153
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
        )
    )
    base_tag = (
        (
            (
                (
                    (((phase[:4] & 0) | 0) if phase < 2 else ((phase[:4] & 0) | 15))
                    if phase < 3
                    else (
                        ((phase[:4] & 0) | 0) if phase < 8 else ((phase[:4] & 0) | 15)
                    )
                )
                if phase < 9
                else (
                    (((phase[:4] & 0) | 8) if phase < 10 else ((phase[:4] & 0) | 15))
                    if phase < 11
                    else (
                        ((phase[:4] & 0) | 0)
                        if phase < 15
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 16
                            else ((phase[:4] & 0) | 10)
                        )
                    )
                )
            )
            if phase < 17
            else (
                (
                    (((phase[:4] & 0) | 8) if phase < 18 else ((phase[:4] & 0) | 0))
                    if phase < 22
                    else (
                        ((phase[:4] & 0) | 10) if phase < 23 else ((phase[:4] & 0) | 1)
                    )
                )
                if phase < 24
                else (
                    (((phase[:4] & 0) | 10) if phase < 25 else ((phase[:4] & 0) | 0))
                    if phase < 30
                    else (
                        ((phase[:4] & 0) | 15)
                        if phase < 31
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 32
                            else ((phase[:4] & 0) | 10)
                        )
                    )
                )
            )
        )
        if phase < 34
        else (
            (
                (
                    (((phase[:4] & 0) | 1) if phase < 35 else ((phase[:4] & 0) | 0))
                    if phase < 40
                    else (
                        ((phase[:4] & 0) | 15) if phase < 41 else ((phase[:4] & 0) | 0)
                    )
                )
                if phase < 44
                else (
                    (((phase[:4] & 0) | 8) if phase < 45 else ((phase[:4] & 0) | 0))
                    if phase < 48
                    else (
                        ((phase[:4] & 0) | 10)
                        if phase < 49
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 52
                            else ((phase[:4] & 0) | 1)
                        )
                    )
                )
            )
            if phase < 53
            else (
                (
                    (((phase[:4] & 0) | 0) if phase < 124 else ((phase[:4] & 0) | 1))
                    if phase < 125
                    else (
                        ((phase[:4] & 0) | 0) if phase < 128 else ((phase[:4] & 0) | 2)
                    )
                )
                if phase < 129
                else (
                    (((phase[:4] & 0) | 0) if phase < 132 else ((phase[:4] & 0) | 4))
                    if phase < 133
                    else (
                        ((phase[:4] & 0) | 0)
                        if phase < 136
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 137
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
        )
    )
    base_data = (
        (
            (
                (
                    (
                        ((phase[:16] & 0) | 0)
                        if phase < 2
                        else (
                            ((phase[:16] & 0) | 65535)
                            if phase < 3
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                    if phase < 8
                    else (
                        (
                            ((phase[:16] & 0) | 65535)
                            if phase < 9
                            else ((phase[:16] & 0) | 32768)
                        )
                        if phase < 10
                        else (
                            ((phase[:16] & 0) | 65535)
                            if phase < 11
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                )
                if phase < 15
                else (
                    (
                        (
                            ((phase[:16] & 0) | 32768)
                            if phase < 16
                            else ((phase[:16] & 0) | 23205)
                        )
                        if phase < 17
                        else (
                            ((phase[:16] & 0) | 32768)
                            if phase < 18
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                    if phase < 22
                    else (
                        (
                            ((phase[:16] & 0) | 23205)
                            if phase < 23
                            else ((phase[:16] & 0) | 1)
                        )
                        if phase < 24
                        else (
                            ((phase[:16] & 0) | 23205)
                            if phase < 25
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                )
            )
            if phase < 30
            else (
                (
                    (
                        ((phase[:16] & 0) | 65535)
                        if phase < 31
                        else (
                            ((phase[:16] & 0) | 32768)
                            if phase < 32
                            else ((phase[:16] & 0) | 23205)
                        )
                    )
                    if phase < 34
                    else (
                        (
                            ((phase[:16] & 0) | 1)
                            if phase < 35
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 40
                        else (
                            ((phase[:16] & 0) | 65535)
                            if phase < 41
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                )
                if phase < 44
                else (
                    (
                        (
                            ((phase[:16] & 0) | 32768)
                            if phase < 45
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 48
                        else (
                            ((phase[:16] & 0) | 23205)
                            if phase < 49
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                    if phase < 52
                    else (
                        (
                            ((phase[:16] & 0) | 1)
                            if phase < 53
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 60
                        else (
                            ((phase[:16] & 0) | 1)
                            if phase < 61
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                )
            )
        )
        if phase < 64
        else (
            (
                (
                    (
                        ((phase[:16] & 0) | 2)
                        if phase < 65
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 68
                            else ((phase[:16] & 0) | 4)
                        )
                    )
                    if phase < 69
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 72
                            else ((phase[:16] & 0) | 8)
                        )
                        if phase < 73
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 76
                            else ((phase[:16] & 0) | 16)
                        )
                    )
                )
                if phase < 77
                else (
                    (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 80
                            else ((phase[:16] & 0) | 32)
                        )
                        if phase < 81
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 84
                            else ((phase[:16] & 0) | 64)
                        )
                    )
                    if phase < 85
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 88
                            else ((phase[:16] & 0) | 128)
                        )
                        if phase < 89
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 92
                            else ((phase[:16] & 0) | 256)
                        )
                    )
                )
            )
            if phase < 93
            else (
                (
                    (
                        ((phase[:16] & 0) | 0)
                        if phase < 96
                        else (
                            ((phase[:16] & 0) | 512)
                            if phase < 97
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                    if phase < 100
                    else (
                        (
                            ((phase[:16] & 0) | 1024)
                            if phase < 101
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 104
                        else (
                            ((phase[:16] & 0) | 2048)
                            if phase < 105
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                )
                if phase < 108
                else (
                    (
                        (
                            ((phase[:16] & 0) | 4096)
                            if phase < 109
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 112
                        else (
                            ((phase[:16] & 0) | 8192)
                            if phase < 113
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                    if phase < 116
                    else (
                        (
                            ((phase[:16] & 0) | 16384)
                            if phase < 117
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 120
                        else (
                            ((phase[:16] & 0) | 32768)
                            if phase < 121
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                )
            )
        )
    )
    base_present = (
        (
            (
                (((phase[:1] & 0) | 0) if phase < 2 else ((phase[:1] & 0) | 1))
                if phase < 3
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 8
                    else (((phase[:1] & 0) | 1) if phase < 9 else ((phase[:1] & 0) | 0))
                )
            )
            if phase < 10
            else (
                (
                    ((phase[:1] & 0) | 1)
                    if phase < 11
                    else (
                        ((phase[:1] & 0) | 0) if phase < 16 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 17
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 22
                    else (
                        ((phase[:1] & 0) | 1) if phase < 23 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
        if phase < 24
        else (
            (
                (
                    ((phase[:1] & 0) | 1)
                    if phase < 25
                    else (
                        ((phase[:1] & 0) | 0) if phase < 30 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 31
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 32
                    else (
                        ((phase[:1] & 0) | 1) if phase < 34 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 40
            else (
                (
                    ((phase[:1] & 0) | 1)
                    if phase < 41
                    else (
                        ((phase[:1] & 0) | 0) if phase < 48 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 49
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 56
                    else (
                        ((phase[:1] & 0) | 1) if phase < 57 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
    )
    header_valid = (
        (
            (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 1 else ((phase[:1] & 0) | 1))
                        if phase < 3
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 10
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 11
                    else (
                        (((phase[:1] & 0) | 0) if phase < 15 else ((phase[:1] & 0) | 1))
                        if phase < 17
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 22
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                )
                if phase < 24
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 29 else ((phase[:1] & 0) | 1))
                        if phase < 35
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 36
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 37
                    else (
                        (((phase[:1] & 0) | 0) if phase < 40 else ((phase[:1] & 0) | 1))
                        if phase < 41
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 44
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 45
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 48
            else (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 49 else ((phase[:1] & 0) | 0))
                        if phase < 52
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 53
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 56
                    else (
                        (((phase[:1] & 0) | 1) if phase < 57 else ((phase[:1] & 0) | 0))
                        if phase < 60
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 61
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 64
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 65
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 68 else ((phase[:1] & 0) | 1))
                        if phase < 69
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 72
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 73
                    else (
                        (((phase[:1] & 0) | 0) if phase < 76 else ((phase[:1] & 0) | 1))
                        if phase < 77
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 80
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 81
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 84
        else (
            (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 85 else ((phase[:1] & 0) | 0))
                        if phase < 88
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 89
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 92
                    else (
                        (((phase[:1] & 0) | 1) if phase < 93 else ((phase[:1] & 0) | 0))
                        if phase < 96
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 97
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 100
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 101
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 104
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 105
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 108
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 109
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 112
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 113
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 116
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 117
                                else ((phase[:1] & 0) | 0)
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
                            ((phase[:1] & 0) | 1)
                            if phase < 121
                            else ((phase[:1] & 0) | 0)
                        )
                        if phase < 124
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 125
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 128
                    else (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 129
                            else ((phase[:1] & 0) | 0)
                        )
                        if phase < 132
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 133
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 136
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 137
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 140
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 141
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 144
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 145
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 148
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 149
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 152
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 153
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    header_opcode = (
        (
            (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 2
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 3
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 8
                    else (
                        ((phase[:4] & 0) | 15)
                        if phase < 11
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 15
                            else ((phase[:4] & 0) | 15)
                        )
                    )
                )
                if phase < 16
                else (
                    (
                        ((phase[:4] & 0) | 5)
                        if phase < 17
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 18
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 22
                    else (
                        ((phase[:4] & 0) | 5)
                        if phase < 23
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 24
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                )
            )
            if phase < 25
            else (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 30
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 32
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                    if phase < 34
                    else (
                        ((phase[:4] & 0) | 8)
                        if phase < 35
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 40
                            else ((phase[:4] & 0) | 15)
                        )
                    )
                )
                if phase < 41
                else (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 44
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 45
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 48
                    else (
                        ((phase[:4] & 0) | 5)
                        if phase < 49
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 52
                            else ((phase[:4] & 0) | 8)
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
                        ((phase[:4] & 0) | 0)
                        if phase < 72
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 73
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 76
                    else (
                        ((phase[:4] & 0) | 2)
                        if phase < 77
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 80
                            else ((phase[:4] & 0) | 4)
                        )
                    )
                )
                if phase < 81
                else (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 84
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 85
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 104
                    else (
                        ((phase[:4] & 0) | 1)
                        if phase < 105
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 108
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                )
            )
            if phase < 109
            else (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 112
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 113
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 116
                    else (
                        ((phase[:4] & 0) | 8)
                        if phase < 117
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 136
                            else ((phase[:4] & 0) | 1)
                        )
                    )
                )
                if phase < 137
                else (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 140
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 141
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 144
                    else (
                        (
                            ((phase[:4] & 0) | 4)
                            if phase < 145
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 148
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 149
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
        )
    )
    header_tag = (
        (
            (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 2
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 3
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 8
                    else (
                        ((phase[:4] & 0) | 15)
                        if phase < 9
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 10
                            else ((phase[:4] & 0) | 15)
                        )
                    )
                )
                if phase < 11
                else (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 16
                        else (
                            ((phase[:4] & 0) | 10)
                            if phase < 17
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 22
                    else (
                        ((phase[:4] & 0) | 10)
                        if phase < 23
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 24
                            else ((phase[:4] & 0) | 10)
                        )
                    )
                )
            )
            if phase < 25
            else (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 30
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 31
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 32
                    else (
                        ((phase[:4] & 0) | 10)
                        if phase < 34
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 35
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
                if phase < 40
                else (
                    (
                        ((phase[:4] & 0) | 15)
                        if phase < 41
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 48
                            else ((phase[:4] & 0) | 10)
                        )
                    )
                    if phase < 49
                    else (
                        (((phase[:4] & 0) | 0) if phase < 52 else ((phase[:4] & 0) | 1))
                        if phase < 53
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 56
                            else ((phase[:4] & 0) | 1)
                        )
                    )
                )
            )
        )
        if phase < 57
        else (
            (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 60
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 61
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 64
                    else (
                        ((phase[:4] & 0) | 4)
                        if phase < 65
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 68
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                )
                if phase < 69
                else (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 88
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 89
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 92
                    else (
                        ((phase[:4] & 0) | 2)
                        if phase < 93
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 96
                            else ((phase[:4] & 0) | 4)
                        )
                    )
                )
            )
            if phase < 97
            else (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 100
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 101
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 120
                    else (
                        ((phase[:4] & 0) | 1)
                        if phase < 121
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 124
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                )
                if phase < 125
                else (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 128
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 129
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 132
                    else (
                        (
                            ((phase[:4] & 0) | 8)
                            if phase < 133
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 152
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 153
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
        )
    )
    payload_valid = (
        (
            (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 1 else ((phase[:1] & 0) | 1))
                        if phase < 3
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 8
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 10
                    else (
                        (((phase[:1] & 0) | 0) if phase < 17 else ((phase[:1] & 0) | 1))
                        if phase < 18
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 22
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                )
                if phase < 24
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 29 else ((phase[:1] & 0) | 1))
                        if phase < 35
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 36
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 37
                    else (
                        (((phase[:1] & 0) | 0) if phase < 40 else ((phase[:1] & 0) | 1))
                        if phase < 41
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 44
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 45
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 48
            else (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 49 else ((phase[:1] & 0) | 0))
                        if phase < 52
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 53
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 56
                    else (
                        (((phase[:1] & 0) | 1) if phase < 57 else ((phase[:1] & 0) | 0))
                        if phase < 60
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 61
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 64
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 65
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 68 else ((phase[:1] & 0) | 1))
                        if phase < 69
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 72
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 73
                    else (
                        (((phase[:1] & 0) | 0) if phase < 76 else ((phase[:1] & 0) | 1))
                        if phase < 77
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 80
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 81
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 84
        else (
            (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 85 else ((phase[:1] & 0) | 0))
                        if phase < 88
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 89
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 92
                    else (
                        (((phase[:1] & 0) | 1) if phase < 93 else ((phase[:1] & 0) | 0))
                        if phase < 96
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 97
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 100
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 101
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 104
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 105
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 108
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 109
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 112
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 113
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 116
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 117
                                else ((phase[:1] & 0) | 0)
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
                            ((phase[:1] & 0) | 1)
                            if phase < 121
                            else ((phase[:1] & 0) | 0)
                        )
                        if phase < 124
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 125
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 128
                    else (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 129
                            else ((phase[:1] & 0) | 0)
                        )
                        if phase < 132
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 133
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 136
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 137
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 140
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 141
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 144
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 145
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 148
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 149
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 152
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 153
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    payload_data = (
        (
            (
                (
                    (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 2
                            else ((phase[:16] & 0) | 65535)
                        )
                        if phase < 3
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 8
                            else ((phase[:16] & 0) | 65535)
                        )
                    )
                    if phase < 9
                    else (
                        (
                            ((phase[:16] & 0) | 32768)
                            if phase < 10
                            else ((phase[:16] & 0) | 65535)
                        )
                        if phase < 11
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 15
                            else (
                                ((phase[:16] & 0) | 32768)
                                if phase < 16
                                else ((phase[:16] & 0) | 23205)
                            )
                        )
                    )
                )
                if phase < 17
                else (
                    (
                        (
                            ((phase[:16] & 0) | 32768)
                            if phase < 18
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 22
                        else (
                            ((phase[:16] & 0) | 23205)
                            if phase < 23
                            else (
                                ((phase[:16] & 0) | 1)
                                if phase < 24
                                else ((phase[:16] & 0) | 23205)
                            )
                        )
                    )
                    if phase < 25
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 30
                            else ((phase[:16] & 0) | 65535)
                        )
                        if phase < 31
                        else (
                            ((phase[:16] & 0) | 32768)
                            if phase < 32
                            else (
                                ((phase[:16] & 0) | 23205)
                                if phase < 34
                                else ((phase[:16] & 0) | 1)
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
                            ((phase[:16] & 0) | 0)
                            if phase < 40
                            else ((phase[:16] & 0) | 65535)
                        )
                        if phase < 41
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 44
                            else (
                                ((phase[:16] & 0) | 32768)
                                if phase < 45
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                    )
                    if phase < 48
                    else (
                        (
                            ((phase[:16] & 0) | 23205)
                            if phase < 49
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 52
                        else (
                            ((phase[:16] & 0) | 1)
                            if phase < 53
                            else (
                                ((phase[:16] & 0) | 0)
                                if phase < 56
                                else ((phase[:16] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 57
                else (
                    (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 60
                            else ((phase[:16] & 0) | 2)
                        )
                        if phase < 61
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 64
                            else (
                                ((phase[:16] & 0) | 4)
                                if phase < 65
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                    )
                    if phase < 68
                    else (
                        (
                            ((phase[:16] & 0) | 8)
                            if phase < 69
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 72
                        else (
                            ((phase[:16] & 0) | 16)
                            if phase < 73
                            else (
                                ((phase[:16] & 0) | 0)
                                if phase < 76
                                else ((phase[:16] & 0) | 32)
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
                            ((phase[:16] & 0) | 0)
                            if phase < 80
                            else ((phase[:16] & 0) | 64)
                        )
                        if phase < 81
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 84
                            else ((phase[:16] & 0) | 128)
                        )
                    )
                    if phase < 85
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 88
                            else ((phase[:16] & 0) | 256)
                        )
                        if phase < 89
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 92
                            else (
                                ((phase[:16] & 0) | 512)
                                if phase < 93
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 96
                else (
                    (
                        (
                            ((phase[:16] & 0) | 1024)
                            if phase < 97
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 100
                        else (
                            ((phase[:16] & 0) | 2048)
                            if phase < 101
                            else (
                                ((phase[:16] & 0) | 0)
                                if phase < 104
                                else ((phase[:16] & 0) | 4096)
                            )
                        )
                    )
                    if phase < 105
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 108
                            else ((phase[:16] & 0) | 8192)
                        )
                        if phase < 109
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 112
                            else (
                                ((phase[:16] & 0) | 16384)
                                if phase < 113
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 116
            else (
                (
                    (
                        (
                            ((phase[:16] & 0) | 32768)
                            if phase < 117
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 120
                        else (
                            ((phase[:16] & 0) | 1)
                            if phase < 121
                            else (
                                ((phase[:16] & 0) | 0)
                                if phase < 124
                                else ((phase[:16] & 0) | 2)
                            )
                        )
                    )
                    if phase < 125
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 128
                            else ((phase[:16] & 0) | 4)
                        )
                        if phase < 129
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 132
                            else (
                                ((phase[:16] & 0) | 8)
                                if phase < 133
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 136
                else (
                    (
                        (
                            ((phase[:16] & 0) | 16)
                            if phase < 137
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 140
                        else (
                            ((phase[:16] & 0) | 32)
                            if phase < 141
                            else (
                                ((phase[:16] & 0) | 0)
                                if phase < 144
                                else ((phase[:16] & 0) | 64)
                            )
                        )
                    )
                    if phase < 145
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 148
                            else ((phase[:16] & 0) | 128)
                        )
                        if phase < 149
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 152
                            else (
                                ((phase[:16] & 0) | 256)
                                if phase < 153
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    patch_valid = (
        (
            (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 1 else ((phase[:1] & 0) | 1))
                        if phase < 3
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 8
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 10
                    else (
                        (((phase[:1] & 0) | 0) if phase < 15 else ((phase[:1] & 0) | 1))
                        if phase < 17
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 24
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                )
                if phase < 25
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 29 else ((phase[:1] & 0) | 1))
                        if phase < 35
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 36
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 37
                    else (
                        (((phase[:1] & 0) | 0) if phase < 40 else ((phase[:1] & 0) | 1))
                        if phase < 41
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 44
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 45
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 48
            else (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 49 else ((phase[:1] & 0) | 0))
                        if phase < 52
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 53
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 56
                    else (
                        (((phase[:1] & 0) | 1) if phase < 57 else ((phase[:1] & 0) | 0))
                        if phase < 60
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 61
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 64
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 65
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 68 else ((phase[:1] & 0) | 1))
                        if phase < 69
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 72
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 73
                    else (
                        (((phase[:1] & 0) | 0) if phase < 76 else ((phase[:1] & 0) | 1))
                        if phase < 77
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 80
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 81
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
        if phase < 84
        else (
            (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 85 else ((phase[:1] & 0) | 0))
                        if phase < 88
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 89
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 92
                    else (
                        (((phase[:1] & 0) | 1) if phase < 93 else ((phase[:1] & 0) | 0))
                        if phase < 96
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 97
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 100
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 101
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 104
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 105
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 108
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 109
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 112
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 113
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 116
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 117
                                else ((phase[:1] & 0) | 0)
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
                            ((phase[:1] & 0) | 1)
                            if phase < 121
                            else ((phase[:1] & 0) | 0)
                        )
                        if phase < 124
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 125
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 128
                    else (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 129
                            else ((phase[:1] & 0) | 0)
                        )
                        if phase < 132
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 133
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 136
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 137
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 140
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 141
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 144
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 145
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 148
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 149
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 152
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 153
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    patch_tag = (
        (
            (
                (
                    (
                        (((phase[:4] & 0) | 0) if phase < 2 else ((phase[:4] & 0) | 15))
                        if phase < 3
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 8
                            else ((phase[:4] & 0) | 15)
                        )
                    )
                    if phase < 11
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 15
                            else ((phase[:4] & 0) | 15)
                        )
                        if phase < 16
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 17
                            else ((phase[:4] & 0) | 15)
                        )
                    )
                )
                if phase < 18
                else (
                    (
                        (((phase[:4] & 0) | 0) if phase < 22 else ((phase[:4] & 0) | 5))
                        if phase < 23
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 24
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                    if phase < 25
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 31
                            else ((phase[:4] & 0) | 15)
                        )
                        if phase < 32
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 33
                            else ((phase[:4] & 0) | 15)
                        )
                    )
                )
            )
            if phase < 34
            else (
                (
                    (
                        (((phase[:4] & 0) | 5) if phase < 35 else ((phase[:4] & 0) | 0))
                        if phase < 40
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 41
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 44
                    else (
                        (
                            ((phase[:4] & 0) | 15)
                            if phase < 45
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 48
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 49
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
                if phase < 52
                else (
                    (
                        (((phase[:4] & 0) | 8) if phase < 53 else ((phase[:4] & 0) | 0))
                        if phase < 60
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 61
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 64
                    else (
                        (((phase[:4] & 0) | 2) if phase < 65 else ((phase[:4] & 0) | 0))
                        if phase < 68
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 69
                            else (
                                ((phase[:4] & 0) | 0)
                                if phase < 72
                                else ((phase[:4] & 0) | 8)
                            )
                        )
                    )
                )
            )
        )
        if phase < 73
        else (
            (
                (
                    (
                        (((phase[:4] & 0) | 0) if phase < 80 else ((phase[:4] & 0) | 1))
                        if phase < 81
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 84
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                    if phase < 85
                    else (
                        (((phase[:4] & 0) | 0) if phase < 88 else ((phase[:4] & 0) | 4))
                        if phase < 89
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 92
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                )
                if phase < 93
                else (
                    (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 100
                            else ((phase[:4] & 0) | 1)
                        )
                        if phase < 101
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 104
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                    if phase < 105
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 108
                            else ((phase[:4] & 0) | 4)
                        )
                        if phase < 109
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 112
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                )
            )
            if phase < 113
            else (
                (
                    (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 120
                            else ((phase[:4] & 0) | 1)
                        )
                        if phase < 121
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 124
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                    if phase < 125
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 128
                            else ((phase[:4] & 0) | 4)
                        )
                        if phase < 129
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 132
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                )
                if phase < 133
                else (
                    (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 140
                            else ((phase[:4] & 0) | 1)
                        )
                        if phase < 141
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 144
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                    if phase < 145
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 148
                            else ((phase[:4] & 0) | 4)
                        )
                        if phase < 149
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 152
                            else (
                                ((phase[:4] & 0) | 8)
                                if phase < 153
                                else ((phase[:4] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    patch_present = (
        (
            (
                (
                    ((phase[:1] & 0) | 0)
                    if phase < 2
                    else (((phase[:1] & 0) | 1) if phase < 3 else ((phase[:1] & 0) | 0))
                )
                if phase < 8
                else (
                    (((phase[:1] & 0) | 1) if phase < 9 else ((phase[:1] & 0) | 0))
                    if phase < 10
                    else (
                        ((phase[:1] & 0) | 1) if phase < 11 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 16
            else (
                (
                    (((phase[:1] & 0) | 1) if phase < 17 else ((phase[:1] & 0) | 0))
                    if phase < 22
                    else (
                        ((phase[:1] & 0) | 1) if phase < 23 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 24
                else (
                    (((phase[:1] & 0) | 1) if phase < 25 else ((phase[:1] & 0) | 0))
                    if phase < 31
                    else (
                        ((phase[:1] & 0) | 1) if phase < 33 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
        if phase < 34
        else (
            (
                (
                    (((phase[:1] & 0) | 1) if phase < 35 else ((phase[:1] & 0) | 0))
                    if phase < 40
                    else (
                        ((phase[:1] & 0) | 1) if phase < 41 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 48
                else (
                    (((phase[:1] & 0) | 1) if phase < 49 else ((phase[:1] & 0) | 0))
                    if phase < 56
                    else (
                        ((phase[:1] & 0) | 1) if phase < 57 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 76
            else (
                (
                    (((phase[:1] & 0) | 1) if phase < 77 else ((phase[:1] & 0) | 0))
                    if phase < 96
                    else (
                        ((phase[:1] & 0) | 1) if phase < 97 else ((phase[:1] & 0) | 0)
                    )
                )
                if phase < 116
                else (
                    (((phase[:1] & 0) | 1) if phase < 117 else ((phase[:1] & 0) | 0))
                    if phase < 136
                    else (
                        ((phase[:1] & 0) | 1) if phase < 137 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
    )
    take = (
        (
            (((phase[:1] & 0) | 0) if phase < 1 else ((phase[:1] & 0) | 1))
            if phase < 7
            else (
                ((phase[:1] & 0) | 0)
                if phase < 8
                else (((phase[:1] & 0) | 1) if phase < 14 else ((phase[:1] & 0) | 0))
            )
        )
        if phase < 15
        else (
            (((phase[:1] & 0) | 1) if phase < 21 else ((phase[:1] & 0) | 0))
            if phase < 22
            else (
                ((phase[:1] & 0) | 1)
                if phase < 28
                else (((phase[:1] & 0) | 0) if phase < 33 else ((phase[:1] & 0) | 1))
            )
        )
    )
    base = Packet(opcode=base_opcode, tag=base_tag, data=base_data, valid=base_present)
    header = Header(opcode=header_opcode, tag=header_tag)
    payload = Payload(data=payload_data)
    patch = Patch(tag=patch_tag, valid=patch_present)
    dut = RecordSpreadPipeline(
        base_valid,
        base,
        header_valid,
        header,
        payload_valid,
        payload,
        patch_valid,
        patch,
        take,
    )
    expected_base_ready = (
        (
            (((phase[:1] & 0) | 1) if phase < 9 else ((phase[:1] & 0) | 0))
            if phase < 11
            else (((phase[:1] & 0) | 1) if phase < 16 else ((phase[:1] & 0) | 0))
        )
        if phase < 18
        else (
            (((phase[:1] & 0) | 1) if phase < 24 else ((phase[:1] & 0) | 0))
            if phase < 25
            else (
                ((phase[:1] & 0) | 1)
                if phase < 31
                else (((phase[:1] & 0) | 0) if phase < 33 else ((phase[:1] & 0) | 1))
            )
        )
    )
    expected_header_ready = (
        (
            (((phase[:1] & 0) | 1) if phase < 2 else ((phase[:1] & 0) | 0))
            if phase < 4
            else (((phase[:1] & 0) | 1) if phase < 16 else ((phase[:1] & 0) | 0))
        )
        if phase < 18
        else (
            (((phase[:1] & 0) | 1) if phase < 24 else ((phase[:1] & 0) | 0))
            if phase < 25
            else (
                ((phase[:1] & 0) | 1)
                if phase < 31
                else (((phase[:1] & 0) | 0) if phase < 33 else ((phase[:1] & 0) | 1))
            )
        )
    )
    expected_payload_ready = (
        (
            (((phase[:1] & 0) | 1) if phase < 2 else ((phase[:1] & 0) | 0))
            if phase < 4
            else (((phase[:1] & 0) | 1) if phase < 9 else ((phase[:1] & 0) | 0))
        )
        if phase < 11
        else (
            (((phase[:1] & 0) | 1) if phase < 24 else ((phase[:1] & 0) | 0))
            if phase < 25
            else (
                ((phase[:1] & 0) | 1)
                if phase < 31
                else (((phase[:1] & 0) | 0) if phase < 33 else ((phase[:1] & 0) | 1))
            )
        )
    )
    expected_patch_ready = (
        (
            (((phase[:1] & 0) | 1) if phase < 2 else ((phase[:1] & 0) | 0))
            if phase < 5
            else (((phase[:1] & 0) | 1) if phase < 9 else ((phase[:1] & 0) | 0))
        )
        if phase < 12
        else (
            (((phase[:1] & 0) | 1) if phase < 16 else ((phase[:1] & 0) | 0))
            if phase < 19
            else (
                ((phase[:1] & 0) | 1)
                if phase < 31
                else (((phase[:1] & 0) | 0) if phase < 33 else ((phase[:1] & 0) | 1))
            )
        )
    )
    expected_valid = (
        (
            (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 6 else ((phase[:1] & 0) | 1))
                        if phase < 7
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 13
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 14
                    else (
                        (((phase[:1] & 0) | 0) if phase < 20 else ((phase[:1] & 0) | 1))
                        if phase < 21
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
                        (((phase[:1] & 0) | 0) if phase < 31 else ((phase[:1] & 0) | 1))
                        if phase < 37
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 38
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 39
                    else (
                        (((phase[:1] & 0) | 0) if phase < 42 else ((phase[:1] & 0) | 1))
                        if phase < 43
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 46
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 47
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 50
            else (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 51 else ((phase[:1] & 0) | 0))
                        if phase < 54
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 55
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 58
                    else (
                        (((phase[:1] & 0) | 1) if phase < 59 else ((phase[:1] & 0) | 0))
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
                if phase < 67
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 70 else ((phase[:1] & 0) | 1))
                        if phase < 71
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 74
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 75
                    else (
                        (((phase[:1] & 0) | 0) if phase < 78 else ((phase[:1] & 0) | 1))
                        if phase < 79
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 82
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 83
                                else ((phase[:1] & 0) | 0)
                            )
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
                        (((phase[:1] & 0) | 1) if phase < 87 else ((phase[:1] & 0) | 0))
                        if phase < 90
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 91
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 94
                    else (
                        (((phase[:1] & 0) | 1) if phase < 95 else ((phase[:1] & 0) | 0))
                        if phase < 98
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 99
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 102
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 103
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 106
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 107
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
                            if phase < 114
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 115
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 118
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 119
                                else ((phase[:1] & 0) | 0)
                            )
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
                        if phase < 126
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 127
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 130
                    else (
                        (
                            ((phase[:1] & 0) | 1)
                            if phase < 131
                            else ((phase[:1] & 0) | 0)
                        )
                        if phase < 134
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 135
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 138
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
                if phase < 139
                else (
                    (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 142
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 143
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 146
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 147
                    else (
                        (
                            ((phase[:1] & 0) | 0)
                            if phase < 150
                            else ((phase[:1] & 0) | 1)
                        )
                        if phase < 151
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 154
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 155
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_data_opcode = (
        (
            (
                (
                    (((phase[:4] & 0) | 0) if phase < 13 else ((phase[:4] & 0) | 15))
                    if phase < 14
                    else (
                        ((phase[:4] & 0) | 0)
                        if phase < 20
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 21
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
                if phase < 26
                else (
                    (
                        ((phase[:4] & 0) | 5)
                        if phase < 27
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 31
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                    if phase < 34
                    else (
                        ((phase[:4] & 0) | 0)
                        if phase < 35
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 36
                            else ((phase[:4] & 0) | 5)
                        )
                    )
                )
            )
            if phase < 37
            else (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 38
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 39
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 46
                    else (
                        ((phase[:4] & 0) | 15)
                        if phase < 47
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 50
                            else ((phase[:4] & 0) | 15)
                        )
                    )
                )
                if phase < 51
                else (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 54
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 55
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 58
                    else (
                        ((phase[:4] & 0) | 8)
                        if phase < 59
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 78
                            else ((phase[:4] & 0) | 1)
                        )
                    )
                )
            )
        )
        if phase < 79
        else (
            (
                (
                    (((phase[:4] & 0) | 0) if phase < 82 else ((phase[:4] & 0) | 2))
                    if phase < 83
                    else (
                        ((phase[:4] & 0) | 0)
                        if phase < 86
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 87
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
                if phase < 90
                else (
                    (
                        ((phase[:4] & 0) | 8)
                        if phase < 91
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 110
                            else ((phase[:4] & 0) | 1)
                        )
                    )
                    if phase < 111
                    else (
                        ((phase[:4] & 0) | 0)
                        if phase < 114
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 115
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
            if phase < 118
            else (
                (
                    (
                        ((phase[:4] & 0) | 4)
                        if phase < 119
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 122
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                    if phase < 123
                    else (
                        ((phase[:4] & 0) | 0)
                        if phase < 142
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 143
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
                if phase < 146
                else (
                    (
                        ((phase[:4] & 0) | 2)
                        if phase < 147
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 150
                            else ((phase[:4] & 0) | 4)
                        )
                    )
                    if phase < 151
                    else (
                        ((phase[:4] & 0) | 0)
                        if phase < 154
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 155
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
        )
    )
    expected_data_tag = (
        (
            (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 13
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 14
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 20
                    else (
                        (
                            ((phase[:4] & 0) | 15)
                            if phase < 21
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 26
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 27
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
                if phase < 35
                else (
                    (
                        ((phase[:4] & 0) | 15)
                        if phase < 36
                        else (
                            ((phase[:4] & 0) | 5)
                            if phase < 37
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 42
                    else (
                        (
                            ((phase[:4] & 0) | 15)
                            if phase < 43
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 46
                        else (
                            ((phase[:4] & 0) | 15)
                            if phase < 47
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
            if phase < 50
            else (
                (
                    (
                        ((phase[:4] & 0) | 5)
                        if phase < 51
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 54
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                    if phase < 55
                    else (
                        (((phase[:4] & 0) | 0) if phase < 62 else ((phase[:4] & 0) | 1))
                        if phase < 63
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 66
                            else ((phase[:4] & 0) | 2)
                        )
                    )
                )
                if phase < 67
                else (
                    (
                        (((phase[:4] & 0) | 0) if phase < 70 else ((phase[:4] & 0) | 4))
                        if phase < 71
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 74
                            else ((phase[:4] & 0) | 8)
                        )
                    )
                    if phase < 75
                    else (
                        (((phase[:4] & 0) | 0) if phase < 82 else ((phase[:4] & 0) | 1))
                        if phase < 83
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 86
                            else ((phase[:4] & 0) | 2)
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
                        ((phase[:4] & 0) | 0)
                        if phase < 90
                        else (
                            ((phase[:4] & 0) | 4)
                            if phase < 91
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 94
                    else (
                        (((phase[:4] & 0) | 8) if phase < 95 else ((phase[:4] & 0) | 0))
                        if phase < 102
                        else (
                            ((phase[:4] & 0) | 1)
                            if phase < 103
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
                if phase < 106
                else (
                    (
                        ((phase[:4] & 0) | 2)
                        if phase < 107
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 110
                            else ((phase[:4] & 0) | 4)
                        )
                    )
                    if phase < 111
                    else (
                        (
                            ((phase[:4] & 0) | 0)
                            if phase < 114
                            else ((phase[:4] & 0) | 8)
                        )
                        if phase < 115
                        else (
                            ((phase[:4] & 0) | 0)
                            if phase < 122
                            else ((phase[:4] & 0) | 1)
                        )
                    )
                )
            )
            if phase < 123
            else (
                (
                    (
                        ((phase[:4] & 0) | 0)
                        if phase < 126
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 127
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 130
                    else (
                        (
                            ((phase[:4] & 0) | 4)
                            if phase < 131
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 134
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 135
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
                if phase < 142
                else (
                    (
                        (
                            ((phase[:4] & 0) | 1)
                            if phase < 143
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 146
                        else (
                            ((phase[:4] & 0) | 2)
                            if phase < 147
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                    if phase < 150
                    else (
                        (
                            ((phase[:4] & 0) | 4)
                            if phase < 151
                            else ((phase[:4] & 0) | 0)
                        )
                        if phase < 154
                        else (
                            ((phase[:4] & 0) | 8)
                            if phase < 155
                            else ((phase[:4] & 0) | 0)
                        )
                    )
                )
            )
        )
    )
    expected_data_data = (
        (
            (
                (
                    (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 13
                            else ((phase[:16] & 0) | 65535)
                        )
                        if phase < 14
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 20
                            else ((phase[:16] & 0) | 32768)
                        )
                    )
                    if phase < 21
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 26
                            else ((phase[:16] & 0) | 23205)
                        )
                        if phase < 27
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 31
                            else ((phase[:16] & 0) | 1)
                        )
                    )
                )
                if phase < 34
                else (
                    (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 35
                            else ((phase[:16] & 0) | 65535)
                        )
                        if phase < 36
                        else (
                            ((phase[:16] & 0) | 23205)
                            if phase < 37
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                    if phase < 38
                    else (
                        (
                            ((phase[:16] & 0) | 1)
                            if phase < 39
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 46
                        else (
                            ((phase[:16] & 0) | 65535)
                            if phase < 47
                            else (
                                ((phase[:16] & 0) | 0)
                                if phase < 50
                                else ((phase[:16] & 0) | 32768)
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
                            ((phase[:16] & 0) | 0)
                            if phase < 54
                            else ((phase[:16] & 0) | 23205)
                        )
                        if phase < 55
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 58
                            else ((phase[:16] & 0) | 1)
                        )
                    )
                    if phase < 59
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 62
                            else ((phase[:16] & 0) | 1)
                        )
                        if phase < 63
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 66
                            else (
                                ((phase[:16] & 0) | 2)
                                if phase < 67
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 70
                else (
                    (
                        (
                            ((phase[:16] & 0) | 4)
                            if phase < 71
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 74
                        else (
                            ((phase[:16] & 0) | 8)
                            if phase < 75
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                    if phase < 78
                    else (
                        (
                            ((phase[:16] & 0) | 16)
                            if phase < 79
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 82
                        else (
                            ((phase[:16] & 0) | 32)
                            if phase < 83
                            else (
                                ((phase[:16] & 0) | 0)
                                if phase < 86
                                else ((phase[:16] & 0) | 64)
                            )
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
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 90
                            else ((phase[:16] & 0) | 128)
                        )
                        if phase < 91
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 94
                            else ((phase[:16] & 0) | 256)
                        )
                    )
                    if phase < 95
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 98
                            else ((phase[:16] & 0) | 512)
                        )
                        if phase < 99
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 102
                            else ((phase[:16] & 0) | 1024)
                        )
                    )
                )
                if phase < 103
                else (
                    (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 106
                            else ((phase[:16] & 0) | 2048)
                        )
                        if phase < 107
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 110
                            else ((phase[:16] & 0) | 4096)
                        )
                    )
                    if phase < 111
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 114
                            else ((phase[:16] & 0) | 8192)
                        )
                        if phase < 115
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 118
                            else (
                                ((phase[:16] & 0) | 16384)
                                if phase < 119
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 122
            else (
                (
                    (
                        (
                            ((phase[:16] & 0) | 32768)
                            if phase < 123
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 126
                        else (
                            ((phase[:16] & 0) | 1)
                            if phase < 127
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                    if phase < 130
                    else (
                        (
                            ((phase[:16] & 0) | 2)
                            if phase < 131
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 134
                        else (
                            ((phase[:16] & 0) | 4)
                            if phase < 135
                            else (
                                ((phase[:16] & 0) | 0)
                                if phase < 138
                                else ((phase[:16] & 0) | 8)
                            )
                        )
                    )
                )
                if phase < 139
                else (
                    (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 142
                            else ((phase[:16] & 0) | 16)
                        )
                        if phase < 143
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 146
                            else ((phase[:16] & 0) | 32)
                        )
                    )
                    if phase < 147
                    else (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 150
                            else ((phase[:16] & 0) | 64)
                        )
                        if phase < 151
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 154
                            else (
                                ((phase[:16] & 0) | 128)
                                if phase < 155
                                else ((phase[:16] & 0) | 0)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_data_valid = (
        (
            (
                (((phase[:1] & 0) | 0) if phase < 13 else ((phase[:1] & 0) | 1))
                if phase < 14
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 26
                    else (
                        ((phase[:1] & 0) | 1) if phase < 27 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 36
            else (
                (((phase[:1] & 0) | 1) if phase < 37 else ((phase[:1] & 0) | 0))
                if phase < 42
                else (
                    ((phase[:1] & 0) | 1)
                    if phase < 43
                    else (
                        ((phase[:1] & 0) | 0) if phase < 50 else ((phase[:1] & 0) | 1)
                    )
                )
            )
        )
        if phase < 51
        else (
            (
                (((phase[:1] & 0) | 0) if phase < 58 else ((phase[:1] & 0) | 1))
                if phase < 59
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 78
                    else (
                        ((phase[:1] & 0) | 1) if phase < 79 else ((phase[:1] & 0) | 0)
                    )
                )
            )
            if phase < 98
            else (
                (
                    ((phase[:1] & 0) | 1)
                    if phase < 99
                    else (
                        ((phase[:1] & 0) | 0) if phase < 118 else ((phase[:1] & 0) | 1)
                    )
                )
                if phase < 119
                else (
                    ((phase[:1] & 0) | 0)
                    if phase < 138
                    else (
                        ((phase[:1] & 0) | 1) if phase < 139 else ((phase[:1] & 0) | 0)
                    )
                )
            )
        )
    )

    @rule
    def check():
        if phase < 158:
            assert (
                dut.base_ready == expected_base_ready
            ), "record_spread_pipeline: base_ready"
            assert (
                dut.header_ready == expected_header_ready
            ), "record_spread_pipeline: header_ready"
            assert (
                dut.payload_ready == expected_payload_ready
            ), "record_spread_pipeline: payload_ready"
            assert (
                dut.patch_ready == expected_patch_ready
            ), "record_spread_pipeline: patch_ready"
            assert dut.valid == expected_valid, "record_spread_pipeline: valid"
            assert (
                dut.data.opcode == expected_data_opcode
            ), "record_spread_pipeline: data.opcode"
            assert dut.data.tag == expected_data_tag, "record_spread_pipeline: data.tag"
            assert (
                dut.data.data == expected_data_data
            ), "record_spread_pipeline: data.data"
            assert (
                dut.data.valid == expected_data_valid
            ), "record_spread_pipeline: data.valid"
        log("info", "record_spread_pipeline.base_ready", dut.base_ready)
        log("info", "record_spread_pipeline.header_ready", dut.header_ready)
        log("info", "record_spread_pipeline.payload_ready", dut.payload_ready)
        log("info", "record_spread_pipeline.patch_ready", dut.patch_ready)
        log("info", "record_spread_pipeline.valid", dut.valid)
        log("info", "record_spread_pipeline.data.opcode", dut.data.opcode)
        log("info", "record_spread_pipeline.data.tag", dut.data.tag)
        log("info", "record_spread_pipeline.data.data", dut.data.data)
        log("info", "record_spread_pipeline.data.valid", dut.data.valid)

    check()
    advance(phase)

"""Complete known-input table from the retained independent cache_params oracle."""

from example_cache_params.cache_params import CacheParams
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseCacheParams():  # noqa: N802
    phase: bits[64] = 0
    addr = (
        0
        if phase == 0
        else (
            2048
            if phase == 1
            else (
                4095
                if phase == 2
                else (
                    4096
                    if phase == 3
                    else (
                        4097
                        if phase == 4
                        else (
                            8191
                            if phase == 5
                            else (
                                8192
                                if phase == 6
                                else (
                                    549755813888
                                    if phase == 7
                                    else (
                                        1099511627775
                                        if phase == 8
                                        else (
                                            78187493530
                                            if phase == 9
                                            else (
                                                737894400291
                                                if phase == 10
                                                else (
                                                    178954240
                                                    if phase == 11
                                                    else (
                                                        89477120
                                                        if phase == 12
                                                        else (
                                                            0
                                                            if phase == 13
                                                            else (phase[:40] & 0)
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
            )
        )
    )
    dut = CacheParams(addr)

    expected_tag = (
        0
        if phase == 0
        else (
            0
            if phase == 1
            else (
                0
                if phase == 2
                else (
                    1
                    if phase == 3
                    else (
                        1
                        if phase == 4
                        else (
                            1
                            if phase == 5
                            else (
                                2
                                if phase == 6
                                else (
                                    134217728
                                    if phase == 7
                                    else (
                                        268435455
                                        if phase == 8
                                        else (
                                            19088743
                                            if phase == 9
                                            else (
                                                180150000
                                                if phase == 10
                                                else (
                                                    43690
                                                    if phase == 11
                                                    else (
                                                        21845
                                                        if phase == 12
                                                        else (
                                                            0
                                                            if phase == 13
                                                            else (phase[:28] & 0)
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
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 14:
            assert dut.tag == expected_tag, "cache_params: tag"
            assert dut.line_words == 8, "cache_params: line words"
            assert dut.tag_bits == 28, "cache_params: tag width"

        log("info", "cache_params.tag", dut.tag)

    check_and_advance()
    advance(phase)

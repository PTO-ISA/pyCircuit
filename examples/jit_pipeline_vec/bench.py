"""Regular-clock jit_pipeline_vec scenario; original physical-control oracles remain."""

from example_jit_pipeline_vec.jit_pipeline_vec import JitPipelineVec
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseJitPipelineVec():  # noqa: N802
    phase: bits[64] = 0
    a = (
        (
            (
                (
                    (
                        (
                            ((phase[:16] & 0) | 1)
                            if phase < 1
                            else ((phase[:16] & 0) | 65535)
                        )
                        if phase < 2
                        else (
                            ((phase[:16] & 0) | 85)
                            if phase < 3
                            else ((phase[:16] & 0) | 3)
                        )
                    )
                    if phase < 4
                    else (
                        (
                            ((phase[:16] & 0) | 65535)
                            if phase < 5
                            else ((phase[:16] & 0) | 1)
                        )
                        if phase < 6
                        else (
                            ((phase[:16] & 0) | 65535)
                            if phase < 9
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                )
                if phase < 10
                else (
                    (
                        (
                            ((phase[:16] & 0) | 32768)
                            if phase < 11
                            else ((phase[:16] & 0) | 85)
                        )
                        if phase < 13
                        else (
                            ((phase[:16] & 0) | 3)
                            if phase < 14
                            else ((phase[:16] & 0) | 1)
                        )
                    )
                    if phase < 15
                    else (
                        (
                            ((phase[:16] & 0) | 65109)
                            if phase < 16
                            else ((phase[:16] & 0) | 4662)
                        )
                        if phase < 17
                        else (
                            ((phase[:16] & 0) | 4964)
                            if phase < 18
                            else (
                                ((phase[:16] & 0) | 54834)
                                if phase < 19
                                else ((phase[:16] & 0) | 62835)
                            )
                        )
                    )
                )
            )
            if phase < 20
            else (
                (
                    (
                        (
                            ((phase[:16] & 0) | 3718)
                            if phase < 21
                            else ((phase[:16] & 0) | 13563)
                        )
                        if phase < 22
                        else (
                            ((phase[:16] & 0) | 59098)
                            if phase < 23
                            else ((phase[:16] & 0) | 45643)
                        )
                    )
                    if phase < 24
                    else (
                        (
                            ((phase[:16] & 0) | 7)
                            if phase < 25
                            else ((phase[:16] & 0) | 21090)
                        )
                        if phase < 26
                        else (
                            ((phase[:16] & 0) | 42544)
                            if phase < 27
                            else ((phase[:16] & 0) | 38335)
                        )
                    )
                )
                if phase < 28
                else (
                    (
                        (
                            ((phase[:16] & 0) | 60890)
                            if phase < 29
                            else ((phase[:16] & 0) | 32928)
                        )
                        if phase < 30
                        else (
                            ((phase[:16] & 0) | 37379)
                            if phase < 31
                            else ((phase[:16] & 0) | 21888)
                        )
                    )
                    if phase < 32
                    else (
                        (
                            ((phase[:16] & 0) | 52545)
                            if phase < 33
                            else ((phase[:16] & 0) | 1502)
                        )
                        if phase < 34
                        else (
                            ((phase[:16] & 0) | 62064)
                            if phase < 35
                            else (
                                ((phase[:16] & 0) | 49798)
                                if phase < 36
                                else ((phase[:16] & 0) | 52609)
                            )
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
                        (
                            ((phase[:16] & 0) | 3069)
                            if phase < 38
                            else ((phase[:16] & 0) | 58207)
                        )
                        if phase < 39
                        else (
                            ((phase[:16] & 0) | 56512)
                            if phase < 40
                            else ((phase[:16] & 0) | 4105)
                        )
                    )
                    if phase < 41
                    else (
                        (
                            ((phase[:16] & 0) | 21099)
                            if phase < 42
                            else ((phase[:16] & 0) | 54349)
                        )
                        if phase < 43
                        else (
                            ((phase[:16] & 0) | 26638)
                            if phase < 44
                            else ((phase[:16] & 0) | 28881)
                        )
                    )
                )
                if phase < 45
                else (
                    (
                        (
                            ((phase[:16] & 0) | 13107)
                            if phase < 46
                            else ((phase[:16] & 0) | 49738)
                        )
                        if phase < 47
                        else (
                            ((phase[:16] & 0) | 30316)
                            if phase < 48
                            else ((phase[:16] & 0) | 17158)
                        )
                    )
                    if phase < 49
                    else (
                        (
                            ((phase[:16] & 0) | 36693)
                            if phase < 50
                            else ((phase[:16] & 0) | 45122)
                        )
                        if phase < 51
                        else (
                            ((phase[:16] & 0) | 39793)
                            if phase < 52
                            else (
                                ((phase[:16] & 0) | 9700)
                                if phase < 53
                                else ((phase[:16] & 0) | 30294)
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
                            ((phase[:16] & 0) | 8577)
                            if phase < 55
                            else ((phase[:16] & 0) | 48376)
                        )
                        if phase < 56
                        else (
                            ((phase[:16] & 0) | 21988)
                            if phase < 57
                            else ((phase[:16] & 0) | 40216)
                        )
                    )
                    if phase < 58
                    else (
                        (
                            ((phase[:16] & 0) | 39538)
                            if phase < 59
                            else ((phase[:16] & 0) | 46366)
                        )
                        if phase < 60
                        else (
                            ((phase[:16] & 0) | 7704)
                            if phase < 61
                            else (
                                ((phase[:16] & 0) | 63468)
                                if phase < 62
                                else ((phase[:16] & 0) | 27560)
                            )
                        )
                    )
                )
                if phase < 63
                else (
                    (
                        (
                            ((phase[:16] & 0) | 14701)
                            if phase < 64
                            else ((phase[:16] & 0) | 19924)
                        )
                        if phase < 65
                        else (
                            ((phase[:16] & 0) | 3046)
                            if phase < 66
                            else ((phase[:16] & 0) | 19233)
                        )
                    )
                    if phase < 67
                    else (
                        (
                            ((phase[:16] & 0) | 47960)
                            if phase < 68
                            else ((phase[:16] & 0) | 54927)
                        )
                        if phase < 69
                        else (
                            ((phase[:16] & 0) | 52082)
                            if phase < 70
                            else (
                                ((phase[:16] & 0) | 32711)
                                if phase < 71
                                else ((phase[:16] & 0) | 1)
                            )
                        )
                    )
                )
            )
        )
    )
    b = (
        (
            (
                (
                    (
                        (
                            ((phase[:16] & 0) | 1)
                            if phase < 2
                            else ((phase[:16] & 0) | 170)
                        )
                        if phase < 3
                        else (
                            ((phase[:16] & 0) | 1)
                            if phase < 8
                            else ((phase[:16] & 0) | 65535)
                        )
                    )
                    if phase < 10
                    else (
                        (
                            ((phase[:16] & 0) | 32768)
                            if phase < 11
                            else ((phase[:16] & 0) | 170)
                        )
                        if phase < 13
                        else (
                            ((phase[:16] & 0) | 1)
                            if phase < 15
                            else ((phase[:16] & 0) | 2716)
                        )
                    )
                )
                if phase < 16
                else (
                    (
                        (
                            ((phase[:16] & 0) | 42315)
                            if phase < 17
                            else ((phase[:16] & 0) | 14894)
                        )
                        if phase < 18
                        else (
                            ((phase[:16] & 0) | 15285)
                            if phase < 19
                            else ((phase[:16] & 0) | 6544)
                        )
                    )
                    if phase < 20
                    else (
                        (
                            ((phase[:16] & 0) | 40879)
                            if phase < 21
                            else ((phase[:16] & 0) | 51522)
                        )
                        if phase < 22
                        else (
                            ((phase[:16] & 0) | 30649)
                            if phase < 23
                            else ((phase[:16] & 0) | 48580)
                        )
                    )
                )
            )
            if phase < 24
            else (
                (
                    (
                        (
                            ((phase[:16] & 0) | 44627)
                            if phase < 25
                            else ((phase[:16] & 0) | 57238)
                        )
                        if phase < 26
                        else (
                            ((phase[:16] & 0) | 4605)
                            if phase < 27
                            else ((phase[:16] & 0) | 43832)
                        )
                    )
                    if phase < 28
                    else (
                        (
                            ((phase[:16] & 0) | 62775)
                            if phase < 29
                            else ((phase[:16] & 0) | 20778)
                        )
                        if phase < 30
                        else (
                            ((phase[:16] & 0) | 52865)
                            if phase < 31
                            else ((phase[:16] & 0) | 54764)
                        )
                    )
                )
                if phase < 32
                else (
                    (
                        (
                            ((phase[:16] & 0) | 55387)
                            if phase < 33
                            else ((phase[:16] & 0) | 12798)
                        )
                        if phase < 34
                        else (
                            ((phase[:16] & 0) | 45381)
                            if phase < 35
                            else ((phase[:16] & 0) | 29152)
                        )
                    )
                    if phase < 36
                    else (
                        (
                            ((phase[:16] & 0) | 64447)
                            if phase < 37
                            else ((phase[:16] & 0) | 54802)
                        )
                        if phase < 38
                        else (
                            ((phase[:16] & 0) | 65097)
                            if phase < 39
                            else ((phase[:16] & 0) | 62228)
                        )
                    )
                )
            )
        )
        if phase < 40
        else (
            (
                (
                    (
                        (
                            ((phase[:16] & 0) | 17251)
                            if phase < 41
                            else ((phase[:16] & 0) | 53606)
                        )
                        if phase < 42
                        else (
                            ((phase[:16] & 0) | 14733)
                            if phase < 43
                            else ((phase[:16] & 0) | 3464)
                        )
                    )
                    if phase < 44
                    else (
                        (
                            ((phase[:16] & 0) | 54087)
                            if phase < 45
                            else ((phase[:16] & 0) | 63482)
                        )
                        if phase < 46
                        else (
                            ((phase[:16] & 0) | 10001)
                            if phase < 47
                            else ((phase[:16] & 0) | 46396)
                        )
                    )
                )
                if phase < 48
                else (
                    (
                        (
                            ((phase[:16] & 0) | 3947)
                            if phase < 49
                            else ((phase[:16] & 0) | 24014)
                        )
                        if phase < 50
                        else (
                            ((phase[:16] & 0) | 51925)
                            if phase < 51
                            else ((phase[:16] & 0) | 7728)
                        )
                    )
                    if phase < 52
                    else (
                        (
                            ((phase[:16] & 0) | 39887)
                            if phase < 53
                            else ((phase[:16] & 0) | 22242)
                        )
                        if phase < 54
                        else (
                            ((phase[:16] & 0) | 26841)
                            if phase < 55
                            else ((phase[:16] & 0) | 48228)
                        )
                    )
                )
            )
            if phase < 56
            else (
                (
                    (
                        (
                            ((phase[:16] & 0) | 23667)
                            if phase < 57
                            else ((phase[:16] & 0) | 30518)
                        )
                        if phase < 58
                        else (
                            ((phase[:16] & 0) | 34077)
                            if phase < 59
                            else ((phase[:16] & 0) | 17368)
                        )
                    )
                    if phase < 60
                    else (
                        (
                            ((phase[:16] & 0) | 30039)
                            if phase < 61
                            else ((phase[:16] & 0) | 37578)
                        )
                        if phase < 62
                        else (
                            ((phase[:16] & 0) | 58273)
                            if phase < 63
                            else ((phase[:16] & 0) | 43148)
                        )
                    )
                )
                if phase < 64
                else (
                    (
                        (
                            ((phase[:16] & 0) | 19067)
                            if phase < 65
                            else ((phase[:16] & 0) | 48542)
                        )
                        if phase < 66
                        else (
                            ((phase[:16] & 0) | 34917)
                            if phase < 67
                            else ((phase[:16] & 0) | 7808)
                        )
                    )
                    if phase < 68
                    else (
                        (
                            ((phase[:16] & 0) | 32735)
                            if phase < 69
                            else ((phase[:16] & 0) | 19378)
                        )
                        if phase < 70
                        else (
                            ((phase[:16] & 0) | 46953)
                            if phase < 71
                            else ((phase[:16] & 0) | 1)
                        )
                    )
                )
            )
        )
    )
    sel = (
        (
            (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 2 else ((phase[:1] & 0) | 0))
                        if phase < 4
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 9
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 10
                    else (
                        (((phase[:1] & 0) | 1) if phase < 11 else ((phase[:1] & 0) | 0))
                        if phase < 12
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 13
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 14
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 15 else ((phase[:1] & 0) | 0))
                        if phase < 16
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 17
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 18
                    else (
                        (((phase[:1] & 0) | 1) if phase < 19 else ((phase[:1] & 0) | 0))
                        if phase < 20
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 21
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
            )
            if phase < 22
            else (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 23 else ((phase[:1] & 0) | 0))
                        if phase < 24
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 25
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 26
                    else (
                        (((phase[:1] & 0) | 1) if phase < 27 else ((phase[:1] & 0) | 0))
                        if phase < 28
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 29
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 30
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 31 else ((phase[:1] & 0) | 0))
                        if phase < 32
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 33
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 34
                    else (
                        (((phase[:1] & 0) | 1) if phase < 35 else ((phase[:1] & 0) | 0))
                        if phase < 36
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 37
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
            )
        )
        if phase < 38
        else (
            (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 39 else ((phase[:1] & 0) | 0))
                        if phase < 40
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 41
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 42
                    else (
                        (((phase[:1] & 0) | 1) if phase < 43 else ((phase[:1] & 0) | 0))
                        if phase < 44
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 45
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 46
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 47 else ((phase[:1] & 0) | 0))
                        if phase < 48
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 49
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 50
                    else (
                        (((phase[:1] & 0) | 1) if phase < 51 else ((phase[:1] & 0) | 0))
                        if phase < 52
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 53
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
            )
            if phase < 54
            else (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 55 else ((phase[:1] & 0) | 0))
                        if phase < 56
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 57
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 58
                    else (
                        (((phase[:1] & 0) | 1) if phase < 59 else ((phase[:1] & 0) | 0))
                        if phase < 60
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 61
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 62
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 63 else ((phase[:1] & 0) | 0))
                        if phase < 64
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 65
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 66
                    else (
                        (((phase[:1] & 0) | 1) if phase < 67 else ((phase[:1] & 0) | 0))
                        if phase < 68
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 69
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 70
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
        )
    )
    dut = JitPipelineVec(a, b, sel)

    expected_tag = (
        (
            (
                ((phase[:1] & 0) | 0)
                if phase < 3
                else (((phase[:1] & 0) | 1) if phase < 4 else ((phase[:1] & 0) | 0))
            )
            if phase < 8
            else (
                ((phase[:1] & 0) | 1)
                if phase < 9
                else (((phase[:1] & 0) | 0) if phase < 11 else ((phase[:1] & 0) | 1))
            )
        )
        if phase < 12
        else (
            (
                ((phase[:1] & 0) | 0)
                if phase < 13
                else (((phase[:1] & 0) | 1) if phase < 14 else ((phase[:1] & 0) | 0))
            )
            if phase < 17
            else (
                ((phase[:1] & 0) | 1)
                if phase < 18
                else (((phase[:1] & 0) | 0) if phase < 74 else ((phase[:1] & 0) | 1))
            )
        )
    )
    expected_data = (
        (
            (
                (
                    (
                        (
                            ((phase[:16] & 0) | 0)
                            if phase < 3
                            else ((phase[:16] & 0) | 2)
                        )
                        if phase < 4
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 5
                            else ((phase[:16] & 0) | 255)
                        )
                    )
                    if phase < 6
                    else (
                        (
                            ((phase[:16] & 0) | 2)
                            if phase < 7
                            else ((phase[:16] & 0) | 0)
                        )
                        if phase < 8
                        else (
                            ((phase[:16] & 0) | 2)
                            if phase < 9
                            else ((phase[:16] & 0) | 0)
                        )
                    )
                )
                if phase < 11
                else (
                    (
                        (
                            ((phase[:16] & 0) | 65534)
                            if phase < 12
                            else ((phase[:16] & 0) | 65535)
                        )
                        if phase < 13
                        else (
                            ((phase[:16] & 0) | 0)
                            if phase < 14
                            else ((phase[:16] & 0) | 255)
                        )
                    )
                    if phase < 16
                    else (
                        (
                            ((phase[:16] & 0) | 2)
                            if phase < 18
                            else ((phase[:16] & 0) | 62665)
                        )
                        if phase < 19
                        else (
                            ((phase[:16] & 0) | 46977)
                            if phase < 20
                            else (
                                ((phase[:16] & 0) | 10570)
                                if phase < 21
                                else ((phase[:16] & 0) | 4583)
                            )
                        )
                    )
                )
            )
            if phase < 22
            else (
                (
                    (
                        (
                            ((phase[:16] & 0) | 60643)
                            if phase < 23
                            else ((phase[:16] & 0) | 44597)
                        )
                        if phase < 24
                        else (
                            ((phase[:16] & 0) | 64953)
                            if phase < 25
                            else ((phase[:16] & 0) | 24211)
                        )
                    )
                    if phase < 26
                    else (
                        (
                            ((phase[:16] & 0) | 3983)
                            if phase < 27
                            else ((phase[:16] & 0) | 44634)
                        )
                        if phase < 28
                        else (
                            ((phase[:16] & 0) | 36340)
                            if phase < 29
                            else (
                                ((phase[:16] & 0) | 47149)
                                if phase < 30
                                else ((phase[:16] & 0) | 16007)
                            )
                        )
                    )
                )
                if phase < 31
                else (
                    (
                        (
                            ((phase[:16] & 0) | 58129)
                            if phase < 32
                            else ((phase[:16] & 0) | 53642)
                        )
                        if phase < 33
                        else (
                            ((phase[:16] & 0) | 24708)
                            if phase < 34
                            else ((phase[:16] & 0) | 32876)
                        )
                    )
                    if phase < 35
                    else (
                        (
                            ((phase[:16] & 0) | 42396)
                            if phase < 36
                            else ((phase[:16] & 0) | 13344)
                        )
                        if phase < 37
                        else (
                            ((phase[:16] & 0) | 41909)
                            if phase < 38
                            else (
                                ((phase[:16] & 0) | 45926)
                                if phase < 39
                                else ((phase[:16] & 0) | 51520)
                            )
                        )
                    )
                )
            )
        )
        if phase < 40
        else (
            (
                (
                    (
                        (
                            ((phase[:16] & 0) | 56815)
                            if phase < 41
                            else ((phase[:16] & 0) | 57768)
                        )
                        if phase < 42
                        else (
                            ((phase[:16] & 0) | 12244)
                            if phase < 43
                            else ((phase[:16] & 0) | 21356)
                        )
                    )
                    if phase < 44
                    else (
                        (
                            ((phase[:16] & 0) | 33549)
                            if phase < 45
                            else ((phase[:16] & 0) | 3546)
                        )
                        if phase < 46
                        else (
                            ((phase[:16] & 0) | 25990)
                            if phase < 47
                            else ((phase[:16] & 0) | 17432)
                        )
                    )
                )
                if phase < 48
                else (
                    (
                        (
                            ((phase[:16] & 0) | 50377)
                            if phase < 49
                            else ((phase[:16] & 0) | 59739)
                        )
                        if phase < 50
                        else (
                            ((phase[:16] & 0) | 50000)
                            if phase < 51
                            else ((phase[:16] & 0) | 21105)
                        )
                    )
                    if phase < 52
                    else (
                        (
                            ((phase[:16] & 0) | 53915)
                            if phase < 53
                            else ((phase[:16] & 0) | 31511)
                        )
                        if phase < 54
                        else (
                            ((phase[:16] & 0) | 34113)
                            if phase < 55
                            else (
                                ((phase[:16] & 0) | 49587)
                                if phase < 56
                                else ((phase[:16] & 0) | 8372)
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
                            ((phase[:16] & 0) | 35418)
                            if phase < 58
                            else ((phase[:16] & 0) | 156)
                        )
                        if phase < 59
                        else (
                            ((phase[:16] & 0) | 45655)
                            if phase < 60
                            else ((phase[:16] & 0) | 59950)
                        )
                    )
                    if phase < 61
                    else (
                        (
                            ((phase[:16] & 0) | 8079)
                            if phase < 62
                            else ((phase[:16] & 0) | 63174)
                        )
                        if phase < 63
                        else (
                            ((phase[:16] & 0) | 37743)
                            if phase < 64
                            else (
                                ((phase[:16] & 0) | 25894)
                                if phase < 65
                                else ((phase[:16] & 0) | 20297)
                            )
                        )
                    )
                )
                if phase < 66
                else (
                    (
                        (
                            ((phase[:16] & 0) | 37345)
                            if phase < 67
                            else ((phase[:16] & 0) | 38991)
                        )
                        if phase < 68
                        else (
                            ((phase[:16] & 0) | 46712)
                            if phase < 69
                            else ((phase[:16] & 0) | 54150)
                        )
                    )
                    if phase < 70
                    else (
                        (
                            ((phase[:16] & 0) | 42456)
                            if phase < 71
                            else ((phase[:16] & 0) | 22126)
                        )
                        if phase < 72
                        else (
                            ((phase[:16] & 0) | 32960)
                            if phase < 73
                            else (
                                ((phase[:16] & 0) | 14128)
                                if phase < 74
                                else ((phase[:16] & 0) | 2)
                            )
                        )
                    )
                )
            )
        )
    )
    expected_lo8 = (
        (
            (
                (
                    (
                        (((phase[:8] & 0) | 0) if phase < 3 else ((phase[:8] & 0) | 2))
                        if phase < 4
                        else (
                            ((phase[:8] & 0) | 0)
                            if phase < 5
                            else ((phase[:8] & 0) | 255)
                        )
                    )
                    if phase < 6
                    else (
                        (((phase[:8] & 0) | 2) if phase < 7 else ((phase[:8] & 0) | 0))
                        if phase < 8
                        else (
                            ((phase[:8] & 0) | 2)
                            if phase < 9
                            else ((phase[:8] & 0) | 0)
                        )
                    )
                )
                if phase < 11
                else (
                    (
                        (
                            ((phase[:8] & 0) | 254)
                            if phase < 12
                            else ((phase[:8] & 0) | 255)
                        )
                        if phase < 13
                        else (
                            ((phase[:8] & 0) | 0)
                            if phase < 14
                            else ((phase[:8] & 0) | 255)
                        )
                    )
                    if phase < 16
                    else (
                        (
                            ((phase[:8] & 0) | 2)
                            if phase < 18
                            else ((phase[:8] & 0) | 201)
                        )
                        if phase < 19
                        else (
                            ((phase[:8] & 0) | 129)
                            if phase < 20
                            else (
                                ((phase[:8] & 0) | 74)
                                if phase < 21
                                else ((phase[:8] & 0) | 231)
                            )
                        )
                    )
                )
            )
            if phase < 22
            else (
                (
                    (
                        (
                            ((phase[:8] & 0) | 227)
                            if phase < 23
                            else ((phase[:8] & 0) | 53)
                        )
                        if phase < 24
                        else (
                            ((phase[:8] & 0) | 185)
                            if phase < 25
                            else ((phase[:8] & 0) | 147)
                        )
                    )
                    if phase < 26
                    else (
                        (
                            ((phase[:8] & 0) | 143)
                            if phase < 27
                            else ((phase[:8] & 0) | 90)
                        )
                        if phase < 28
                        else (
                            ((phase[:8] & 0) | 244)
                            if phase < 29
                            else (
                                ((phase[:8] & 0) | 45)
                                if phase < 30
                                else ((phase[:8] & 0) | 135)
                            )
                        )
                    )
                )
                if phase < 31
                else (
                    (
                        (
                            ((phase[:8] & 0) | 17)
                            if phase < 32
                            else ((phase[:8] & 0) | 138)
                        )
                        if phase < 33
                        else (
                            ((phase[:8] & 0) | 132)
                            if phase < 34
                            else ((phase[:8] & 0) | 108)
                        )
                    )
                    if phase < 35
                    else (
                        (
                            ((phase[:8] & 0) | 156)
                            if phase < 36
                            else ((phase[:8] & 0) | 32)
                        )
                        if phase < 37
                        else (
                            ((phase[:8] & 0) | 181)
                            if phase < 38
                            else (
                                ((phase[:8] & 0) | 102)
                                if phase < 39
                                else ((phase[:8] & 0) | 64)
                            )
                        )
                    )
                )
            )
        )
        if phase < 40
        else (
            (
                (
                    (
                        (
                            ((phase[:8] & 0) | 239)
                            if phase < 41
                            else ((phase[:8] & 0) | 168)
                        )
                        if phase < 42
                        else (
                            ((phase[:8] & 0) | 212)
                            if phase < 43
                            else ((phase[:8] & 0) | 108)
                        )
                    )
                    if phase < 44
                    else (
                        (
                            ((phase[:8] & 0) | 13)
                            if phase < 45
                            else ((phase[:8] & 0) | 218)
                        )
                        if phase < 46
                        else (
                            ((phase[:8] & 0) | 134)
                            if phase < 47
                            else ((phase[:8] & 0) | 24)
                        )
                    )
                )
                if phase < 48
                else (
                    (
                        (
                            ((phase[:8] & 0) | 201)
                            if phase < 49
                            else ((phase[:8] & 0) | 91)
                        )
                        if phase < 50
                        else (
                            ((phase[:8] & 0) | 80)
                            if phase < 51
                            else ((phase[:8] & 0) | 113)
                        )
                    )
                    if phase < 52
                    else (
                        (
                            ((phase[:8] & 0) | 155)
                            if phase < 53
                            else ((phase[:8] & 0) | 23)
                        )
                        if phase < 54
                        else (
                            ((phase[:8] & 0) | 65)
                            if phase < 55
                            else (
                                ((phase[:8] & 0) | 179)
                                if phase < 56
                                else ((phase[:8] & 0) | 180)
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
                            ((phase[:8] & 0) | 90)
                            if phase < 58
                            else ((phase[:8] & 0) | 156)
                        )
                        if phase < 59
                        else (
                            ((phase[:8] & 0) | 87)
                            if phase < 60
                            else ((phase[:8] & 0) | 46)
                        )
                    )
                    if phase < 61
                    else (
                        (
                            ((phase[:8] & 0) | 143)
                            if phase < 62
                            else ((phase[:8] & 0) | 198)
                        )
                        if phase < 63
                        else (
                            ((phase[:8] & 0) | 111)
                            if phase < 64
                            else (
                                ((phase[:8] & 0) | 38)
                                if phase < 65
                                else ((phase[:8] & 0) | 73)
                            )
                        )
                    )
                )
                if phase < 66
                else (
                    (
                        (
                            ((phase[:8] & 0) | 225)
                            if phase < 67
                            else ((phase[:8] & 0) | 79)
                        )
                        if phase < 68
                        else (
                            ((phase[:8] & 0) | 120)
                            if phase < 69
                            else ((phase[:8] & 0) | 134)
                        )
                    )
                    if phase < 70
                    else (
                        (
                            ((phase[:8] & 0) | 216)
                            if phase < 71
                            else ((phase[:8] & 0) | 110)
                        )
                        if phase < 72
                        else (
                            ((phase[:8] & 0) | 192)
                            if phase < 73
                            else (
                                ((phase[:8] & 0) | 48)
                                if phase < 74
                                else ((phase[:8] & 0) | 2)
                            )
                        )
                    )
                )
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 75:
            assert dut.tag == expected_tag, "jit_pipeline_vec: tag"
            assert dut.data == expected_data, "jit_pipeline_vec: data"
            assert dut.lo8 == expected_lo8, "jit_pipeline_vec: lo8"

        log("info", "jit_pipeline_vec.tag", dut.tag)
        log("info", "jit_pipeline_vec.data", dut.data)
        log("info", "jit_pipeline_vec.lo8", dut.lo8)

    check_and_advance()
    advance(phase)

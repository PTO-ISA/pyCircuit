"""Regular-clock pipeline_builder scenario; original physical-control oracles remain."""

from example_pipeline_builder.pipeline_builder import PipelineBuilder
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExercisePipelineBuilder():  # noqa: N802
    phase: bits[64] = 0
    word = (
        (
            (
                (
                    (
                        (
                            ((phase[:32] & 0) | 5)
                            if phase < 1
                            else ((phase[:32] & 0) | 4294967295)
                        )
                        if phase < 2
                        else (
                            ((phase[:32] & 0) | 5)
                            if phase < 3
                            else ((phase[:32] & 0) | 4294967295)
                        )
                    )
                    if phase < 5
                    else (
                        (
                            ((phase[:32] & 0) | 5)
                            if phase < 6
                            else ((phase[:32] & 0) | 4294967280)
                        )
                        if phase < 7
                        else (
                            ((phase[:32] & 0) | 4294967295)
                            if phase < 8
                            else ((phase[:32] & 0) | 0)
                        )
                    )
                )
                if phase < 9
                else (
                    (
                        (
                            ((phase[:32] & 0) | 3754036845)
                            if phase < 10
                            else ((phase[:32] & 0) | 1403919592)
                        )
                        if phase < 11
                        else (
                            ((phase[:32] & 0) | 2926762791)
                            if phase < 12
                            else ((phase[:32] & 0) | 1113890394)
                        )
                    )
                    if phase < 13
                    else (
                        (
                            ((phase[:32] & 0) | 694999537)
                            if phase < 14
                            else ((phase[:32] & 0) | 4266986140)
                        )
                        if phase < 15
                        else (
                            ((phase[:32] & 0) | 305571147)
                            if phase < 16
                            else (
                                ((phase[:32] & 0) | 325335598)
                                if phase < 17
                                else ((phase[:32] & 0) | 3593616309)
                            )
                        )
                    )
                )
            )
            if phase < 18
            else (
                (
                    (
                        (
                            ((phase[:32] & 0) | 4117961104)
                            if phase < 19
                            else ((phase[:32] & 0) | 243703727)
                        )
                        if phase < 20
                        else (
                            ((phase[:32] & 0) | 888916290)
                            if phase < 21
                            else ((phase[:32] & 0) | 3873077177)
                        )
                    )
                    if phase < 22
                    else (
                        (
                            ((phase[:32] & 0) | 2991308228)
                            if phase < 23
                            else ((phase[:32] & 0) | 503379)
                        )
                        if phase < 24
                        else (
                            ((phase[:32] & 0) | 1382211478)
                            if phase < 25
                            else (
                                ((phase[:32] & 0) | 2788168189)
                                if phase < 26
                                else ((phase[:32] & 0) | 2512366392)
                            )
                        )
                    )
                )
                if phase < 27
                else (
                    (
                        (
                            ((phase[:32] & 0) | 3990549815)
                            if phase < 28
                            else ((phase[:32] & 0) | 2157990186)
                        )
                        if phase < 29
                        else (
                            ((phase[:32] & 0) | 2449723009)
                            if phase < 30
                            else ((phase[:32] & 0) | 1434506732)
                        )
                    )
                    if phase < 31
                    else (
                        (
                            ((phase[:32] & 0) | 3443644507)
                            if phase < 32
                            else ((phase[:32] & 0) | 98447870)
                        )
                        if phase < 33
                        else (
                            ((phase[:32] & 0) | 4067471685)
                            if phase < 34
                            else (
                                ((phase[:32] & 0) | 3263590880)
                                if phase < 35
                                else ((phase[:32] & 0) | 3447847871)
                            )
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
                        (
                            ((phase[:32] & 0) | 201184786)
                            if phase < 37
                            else ((phase[:32] & 0) | 3814719049)
                        )
                        if phase < 38
                        else (
                            ((phase[:32] & 0) | 3703632660)
                            if phase < 39
                            else ((phase[:32] & 0) | 269042531)
                        )
                    )
                    if phase < 40
                    else (
                        (
                            ((phase[:32] & 0) | 1382797670)
                            if phase < 41
                            else ((phase[:32] & 0) | 3561830797)
                        )
                        if phase < 42
                        else (
                            ((phase[:32] & 0) | 1745751432)
                            if phase < 43
                            else ((phase[:32] & 0) | 1892799303)
                        )
                    )
                )
                if phase < 44
                else (
                    (
                        (
                            ((phase[:32] & 0) | 859043834)
                            if phase < 45
                            else ((phase[:32] & 0) | 3259639569)
                        )
                        if phase < 46
                        else (
                            ((phase[:32] & 0) | 1986835772)
                            if phase < 47
                            else ((phase[:32] & 0) | 1124470635)
                        )
                    )
                    if phase < 48
                    else (
                        (
                            ((phase[:32] & 0) | 2404736462)
                            if phase < 49
                            else ((phase[:32] & 0) | 2957167317)
                        )
                        if phase < 50
                        else (
                            ((phase[:32] & 0) | 2607881776)
                            if phase < 51
                            else (
                                ((phase[:32] & 0) | 635739087)
                                if phase < 52
                                else ((phase[:32] & 0) | 1985369826)
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
                            ((phase[:32] & 0) | 562129113)
                            if phase < 54
                            else ((phase[:32] & 0) | 3170417764)
                        )
                        if phase < 55
                        else (
                            ((phase[:32] & 0) | 1441029235)
                            if phase < 56
                            else ((phase[:32] & 0) | 2635626294)
                        )
                    )
                    if phase < 57
                    else (
                        (
                            ((phase[:32] & 0) | 2591196445)
                            if phase < 58
                            else ((phase[:32] & 0) | 3038659544)
                        )
                        if phase < 59
                        else (
                            ((phase[:32] & 0) | 504919383)
                            if phase < 60
                            else (
                                ((phase[:32] & 0) | 4159476426)
                                if phase < 61
                                else ((phase[:32] & 0) | 1806230433)
                            )
                        )
                    )
                )
                if phase < 62
                else (
                    (
                        (
                            ((phase[:32] & 0) | 963487884)
                            if phase < 63
                            else ((phase[:32] & 0) | 1305758331)
                        )
                        if phase < 64
                        else (
                            ((phase[:32] & 0) | 199671198)
                            if phase < 65
                            else ((phase[:32] & 0) | 1260488805)
                        )
                    )
                    if phase < 66
                    else (
                        (
                            ((phase[:32] & 0) | 3143114368)
                            if phase < 67
                            else ((phase[:32] & 0) | 3599728607)
                        )
                        if phase < 68
                        else (
                            ((phase[:32] & 0) | 3413265330)
                            if phase < 69
                            else (
                                ((phase[:32] & 0) | 2143795049)
                                if phase < 70
                                else ((phase[:32] & 0) | 5)
                            )
                        )
                    )
                )
            )
        )
    )
    valid = (
        (
            (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 1 else ((phase[:1] & 0) | 0))
                        if phase < 2
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 3
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 5
                    else (
                        (((phase[:1] & 0) | 1) if phase < 6 else ((phase[:1] & 0) | 0))
                        if phase < 7
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 8
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 9
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 10 else ((phase[:1] & 0) | 0))
                        if phase < 11
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 12
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 13
                    else (
                        (((phase[:1] & 0) | 1) if phase < 14 else ((phase[:1] & 0) | 0))
                        if phase < 15
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 16
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 17
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
            if phase < 18
            else (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 19 else ((phase[:1] & 0) | 1))
                        if phase < 20
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 21
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 22
                    else (
                        (((phase[:1] & 0) | 0) if phase < 23 else ((phase[:1] & 0) | 1))
                        if phase < 24
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
                        (((phase[:1] & 0) | 0) if phase < 27 else ((phase[:1] & 0) | 1))
                        if phase < 28
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 29
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 30
                    else (
                        (((phase[:1] & 0) | 0) if phase < 31 else ((phase[:1] & 0) | 1))
                        if phase < 32
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 33
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 34
                                else ((phase[:1] & 0) | 0)
                            )
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
                        (((phase[:1] & 0) | 1) if phase < 36 else ((phase[:1] & 0) | 0))
                        if phase < 37
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 38
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 39
                    else (
                        (((phase[:1] & 0) | 1) if phase < 40 else ((phase[:1] & 0) | 0))
                        if phase < 41
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 42
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 43
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 44 else ((phase[:1] & 0) | 0))
                        if phase < 45
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 46
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 47
                    else (
                        (((phase[:1] & 0) | 1) if phase < 48 else ((phase[:1] & 0) | 0))
                        if phase < 49
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 50
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 51
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
            if phase < 52
            else (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 53 else ((phase[:1] & 0) | 1))
                        if phase < 54
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 55
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 56
                    else (
                        (((phase[:1] & 0) | 0) if phase < 57 else ((phase[:1] & 0) | 1))
                        if phase < 58
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 59
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 60
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 61
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 62 else ((phase[:1] & 0) | 0))
                        if phase < 63
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 64
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 65
                    else (
                        (((phase[:1] & 0) | 1) if phase < 66 else ((phase[:1] & 0) | 0))
                        if phase < 67
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 68
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 69
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
        )
    )
    dut = PipelineBuilder(word, valid)

    expected_word = (
        (
            (
                (
                    (
                        (
                            ((phase[:32] & 0) | 0)
                            if phase < 1
                            else ((phase[:32] & 0) | 1)
                        )
                        if phase < 2
                        else (
                            ((phase[:32] & 0) | 6)
                            if phase < 3
                            else ((phase[:32] & 0) | 0)
                        )
                    )
                    if phase < 4
                    else (
                        (
                            ((phase[:32] & 0) | 6)
                            if phase < 5
                            else ((phase[:32] & 0) | 0)
                        )
                        if phase < 7
                        else (
                            ((phase[:32] & 0) | 6)
                            if phase < 8
                            else ((phase[:32] & 0) | 4294967281)
                        )
                    )
                )
                if phase < 9
                else (
                    (
                        (
                            ((phase[:32] & 0) | 0)
                            if phase < 10
                            else ((phase[:32] & 0) | 1)
                        )
                        if phase < 11
                        else (
                            ((phase[:32] & 0) | 3754036846)
                            if phase < 12
                            else ((phase[:32] & 0) | 1403919593)
                        )
                    )
                    if phase < 13
                    else (
                        (
                            ((phase[:32] & 0) | 2926762792)
                            if phase < 14
                            else ((phase[:32] & 0) | 1113890395)
                        )
                        if phase < 15
                        else (
                            ((phase[:32] & 0) | 694999538)
                            if phase < 16
                            else (
                                ((phase[:32] & 0) | 4266986141)
                                if phase < 17
                                else ((phase[:32] & 0) | 305571148)
                            )
                        )
                    )
                )
            )
            if phase < 18
            else (
                (
                    (
                        (
                            ((phase[:32] & 0) | 325335599)
                            if phase < 19
                            else ((phase[:32] & 0) | 3593616310)
                        )
                        if phase < 20
                        else (
                            ((phase[:32] & 0) | 4117961105)
                            if phase < 21
                            else ((phase[:32] & 0) | 243703728)
                        )
                    )
                    if phase < 22
                    else (
                        (
                            ((phase[:32] & 0) | 888916291)
                            if phase < 23
                            else ((phase[:32] & 0) | 3873077178)
                        )
                        if phase < 24
                        else (
                            ((phase[:32] & 0) | 2991308229)
                            if phase < 25
                            else (
                                ((phase[:32] & 0) | 503380)
                                if phase < 26
                                else ((phase[:32] & 0) | 1382211479)
                            )
                        )
                    )
                )
                if phase < 27
                else (
                    (
                        (
                            ((phase[:32] & 0) | 2788168190)
                            if phase < 28
                            else ((phase[:32] & 0) | 2512366393)
                        )
                        if phase < 29
                        else (
                            ((phase[:32] & 0) | 3990549816)
                            if phase < 30
                            else ((phase[:32] & 0) | 2157990187)
                        )
                    )
                    if phase < 31
                    else (
                        (
                            ((phase[:32] & 0) | 2449723010)
                            if phase < 32
                            else ((phase[:32] & 0) | 1434506733)
                        )
                        if phase < 33
                        else (
                            ((phase[:32] & 0) | 3443644508)
                            if phase < 34
                            else (
                                ((phase[:32] & 0) | 98447871)
                                if phase < 35
                                else ((phase[:32] & 0) | 4067471686)
                            )
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
                        (
                            ((phase[:32] & 0) | 3263590881)
                            if phase < 37
                            else ((phase[:32] & 0) | 3447847872)
                        )
                        if phase < 38
                        else (
                            ((phase[:32] & 0) | 201184787)
                            if phase < 39
                            else ((phase[:32] & 0) | 3814719050)
                        )
                    )
                    if phase < 40
                    else (
                        (
                            ((phase[:32] & 0) | 3703632661)
                            if phase < 41
                            else ((phase[:32] & 0) | 269042532)
                        )
                        if phase < 42
                        else (
                            ((phase[:32] & 0) | 1382797671)
                            if phase < 43
                            else (
                                ((phase[:32] & 0) | 3561830798)
                                if phase < 44
                                else ((phase[:32] & 0) | 1745751433)
                            )
                        )
                    )
                )
                if phase < 45
                else (
                    (
                        (
                            ((phase[:32] & 0) | 1892799304)
                            if phase < 46
                            else ((phase[:32] & 0) | 859043835)
                        )
                        if phase < 47
                        else (
                            ((phase[:32] & 0) | 3259639570)
                            if phase < 48
                            else ((phase[:32] & 0) | 1986835773)
                        )
                    )
                    if phase < 49
                    else (
                        (
                            ((phase[:32] & 0) | 1124470636)
                            if phase < 50
                            else ((phase[:32] & 0) | 2404736463)
                        )
                        if phase < 51
                        else (
                            ((phase[:32] & 0) | 2957167318)
                            if phase < 52
                            else (
                                ((phase[:32] & 0) | 2607881777)
                                if phase < 53
                                else ((phase[:32] & 0) | 635739088)
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
                            ((phase[:32] & 0) | 1985369827)
                            if phase < 55
                            else ((phase[:32] & 0) | 562129114)
                        )
                        if phase < 56
                        else (
                            ((phase[:32] & 0) | 3170417765)
                            if phase < 57
                            else ((phase[:32] & 0) | 1441029236)
                        )
                    )
                    if phase < 58
                    else (
                        (
                            ((phase[:32] & 0) | 2635626295)
                            if phase < 59
                            else ((phase[:32] & 0) | 2591196446)
                        )
                        if phase < 60
                        else (
                            ((phase[:32] & 0) | 3038659545)
                            if phase < 61
                            else (
                                ((phase[:32] & 0) | 504919384)
                                if phase < 62
                                else ((phase[:32] & 0) | 4159476427)
                            )
                        )
                    )
                )
                if phase < 63
                else (
                    (
                        (
                            ((phase[:32] & 0) | 1806230434)
                            if phase < 64
                            else ((phase[:32] & 0) | 963487885)
                        )
                        if phase < 65
                        else (
                            ((phase[:32] & 0) | 1305758332)
                            if phase < 66
                            else ((phase[:32] & 0) | 199671199)
                        )
                    )
                    if phase < 67
                    else (
                        (
                            ((phase[:32] & 0) | 1260488806)
                            if phase < 68
                            else ((phase[:32] & 0) | 3143114369)
                        )
                        if phase < 69
                        else (
                            ((phase[:32] & 0) | 3599728608)
                            if phase < 70
                            else (
                                ((phase[:32] & 0) | 3413265331)
                                if phase < 71
                                else ((phase[:32] & 0) | 2143795050)
                            )
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
                        (((phase[:1] & 0) | 0) if phase < 2 else ((phase[:1] & 0) | 1))
                        if phase < 3
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 4
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 5
                    else (
                        (((phase[:1] & 0) | 0) if phase < 7 else ((phase[:1] & 0) | 1))
                        if phase < 8
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 9
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                )
                if phase < 10
                else (
                    (
                        (((phase[:1] & 0) | 0) if phase < 11 else ((phase[:1] & 0) | 1))
                        if phase < 12
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 13
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 14
                    else (
                        (((phase[:1] & 0) | 0) if phase < 15 else ((phase[:1] & 0) | 1))
                        if phase < 16
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 17
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 18
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
            )
            if phase < 19
            else (
                (
                    (
                        (((phase[:1] & 0) | 1) if phase < 20 else ((phase[:1] & 0) | 0))
                        if phase < 21
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 22
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 23
                    else (
                        (((phase[:1] & 0) | 1) if phase < 24 else ((phase[:1] & 0) | 0))
                        if phase < 25
                        else (
                            ((phase[:1] & 0) | 1)
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
                        (((phase[:1] & 0) | 0) if phase < 29 else ((phase[:1] & 0) | 1))
                        if phase < 30
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 31
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 32
                    else (
                        (((phase[:1] & 0) | 0) if phase < 33 else ((phase[:1] & 0) | 1))
                        if phase < 34
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 35
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 36
                                else ((phase[:1] & 0) | 0)
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
                        (((phase[:1] & 0) | 1) if phase < 38 else ((phase[:1] & 0) | 0))
                        if phase < 39
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 40
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 41
                    else (
                        (((phase[:1] & 0) | 1) if phase < 42 else ((phase[:1] & 0) | 0))
                        if phase < 43
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 44
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                )
                if phase < 45
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 46 else ((phase[:1] & 0) | 0))
                        if phase < 47
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 48
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 49
                    else (
                        (((phase[:1] & 0) | 1) if phase < 50 else ((phase[:1] & 0) | 0))
                        if phase < 51
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 52
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 53
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
            if phase < 54
            else (
                (
                    (
                        (((phase[:1] & 0) | 0) if phase < 55 else ((phase[:1] & 0) | 1))
                        if phase < 56
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 57
                            else ((phase[:1] & 0) | 1)
                        )
                    )
                    if phase < 58
                    else (
                        (((phase[:1] & 0) | 0) if phase < 59 else ((phase[:1] & 0) | 1))
                        if phase < 60
                        else (
                            ((phase[:1] & 0) | 0)
                            if phase < 61
                            else (
                                ((phase[:1] & 0) | 1)
                                if phase < 62
                                else ((phase[:1] & 0) | 0)
                            )
                        )
                    )
                )
                if phase < 63
                else (
                    (
                        (((phase[:1] & 0) | 1) if phase < 64 else ((phase[:1] & 0) | 0))
                        if phase < 65
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 66
                            else ((phase[:1] & 0) | 0)
                        )
                    )
                    if phase < 67
                    else (
                        (((phase[:1] & 0) | 1) if phase < 68 else ((phase[:1] & 0) | 0))
                        if phase < 69
                        else (
                            ((phase[:1] & 0) | 1)
                            if phase < 70
                            else (
                                ((phase[:1] & 0) | 0)
                                if phase < 71
                                else ((phase[:1] & 0) | 1)
                            )
                        )
                    )
                )
            )
        )
    )

    @rule
    def check_and_advance():
        if phase < 72:
            assert dut.word == expected_word, "pipeline_builder: word"
            assert dut.valid == expected_valid, "pipeline_builder: valid"

        log("info", "pipeline_builder.word", dut.word)
        log("info", "pipeline_builder.valid", dut.valid)

    check_and_advance()
    advance(phase)

"""Regular-clock full known-input stream from the retained feedback_pipeline oracle."""

from example_feedback_pipeline.feedback_pipeline import FeedbackPipeline, Item
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseFeedbackPipeline():  # noqa: N802
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
                                    if phase < 2
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 4
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 5
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 8
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 9
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 13
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 14
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 19
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 20
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 26
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 27
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 34
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 35
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 43
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 44
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 53
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 54
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 64
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 65
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 76
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 77
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 89
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 90
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 103
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 104
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 118
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 119
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 134
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 135
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
                                    if phase < 169
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 177
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 247
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 251
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 322
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 323
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 325
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 326
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 329
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 330
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 334
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 335
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 340
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 341
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 347
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 348
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 355
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 356
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 364
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 365
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 374
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 375
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 385
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 386
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 397
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 398
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 410
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 411
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 424
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 425
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 439
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 440
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 455
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 456
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 472
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 473
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 490
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 491
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 493
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 494
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 497
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 498
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 502
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 503
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 508
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 509
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 515
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 516
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 523
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 524
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 532
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 533
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 542
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 543
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 553
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 554
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 565
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 566
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 578
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 579
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
                            if phase < 607
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 608
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 623
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 624
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 640
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 641
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 658
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 659
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 661
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 662
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 665
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 666
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 670
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 671
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 676
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 677
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 683
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 684
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 691
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 692
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 700
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 701
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 710
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 711
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 721
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 722
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 733
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 734
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 746
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 747
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 760
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 761
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 775
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 776
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 791
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 792
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 808
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 809
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 826
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 827
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 829
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 830
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 833
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 834
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 838
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 839
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 844
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 845
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 851
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 852
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 859
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 860
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 868
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 869
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 878
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 879
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 889
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 890
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 901
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 902
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 914
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 915
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 928
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 929
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 943
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 944
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 959
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 960
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 976
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 977
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 994
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 995
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 997
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 998
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1001
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 1002
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1006
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1007
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1012
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1013
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1019
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1020
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1027
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1028
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1036
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 1037
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1046
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1047
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1057
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1058
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1069
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1070
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1082
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1083
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1096
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1097
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1111
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1112
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1127
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1128
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1144
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1145
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1162
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1163
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1165
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1166
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1169
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1170
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1174
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1175
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1180
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1181
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1187
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1188
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1195
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1196
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1204
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1205
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1214
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1215
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1225
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1226
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1237
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1238
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1250
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1251
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1264
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 1265
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1279
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1280
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1295
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1296
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1312
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1313
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1330
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1331
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1333
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1334
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1337
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1338
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1342
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1343
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1348
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1349
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1355
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1356
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1363
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1364
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1372
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 1373
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1382
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1383
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1393
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1394
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1405
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1406
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1418
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1419
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1432
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1433
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1447
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1448
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1463
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1464
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1480
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1481
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1498
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1499
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1501
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1502
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1505
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1506
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1510
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1511
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1516
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1517
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1523
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1524
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1531
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1532
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1540
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1541
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1550
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1551
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1561
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1562
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1573
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1574
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1586
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1587
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1600
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 1601
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1615
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1616
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1631
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1632
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1648
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1649
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1666
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1667
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1669
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1670
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1673
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1674
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1678
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1679
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1684
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1685
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1691
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1692
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1699
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1700
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1708
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1709
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1718
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1719
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1729
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1730
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1741
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1742
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1754
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1755
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1768
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1769
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1783
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1784
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1799
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1800
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1816
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1817
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1834
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1835
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1837
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1838
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1841
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1842
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1846
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1847
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1852
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1853
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1859
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1860
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1867
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1868
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1876
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
        if phase < 1877
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1886
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1887
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1897
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1898
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1909
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1910
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1922
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1923
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1936
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 1937
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1951
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1952
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1967
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1968
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1984
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1985
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2002
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2003
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2005
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2006
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2009
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2010
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2014
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2015
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2020
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2021
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2027
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2028
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2035
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2036
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2044
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2045
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2054
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2055
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2065
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2066
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2077
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2078
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2090
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2091
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2104
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 2105
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2119
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2120
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2135
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2136
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2152
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2153
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2170
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2171
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2173
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 2174
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2177
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2178
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2182
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2183
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2188
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2189
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2195
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2196
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2203
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2204
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2212
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2213
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2222
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2223
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2233
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2234
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2245
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2246
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2258
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2259
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2272
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 2273
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2287
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2288
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2303
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2304
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2320
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2321
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2338
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2339
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2341
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2342
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 2359
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2360
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2362
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2363
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2380
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2381
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2383
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2384
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2401
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2402
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2404
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2405
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2422
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2423
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2425
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2426
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2443
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2444
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2446
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2447
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2464
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2465
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2467
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2468
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2485
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2486
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2488
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2489
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2506
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2507
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2509
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 2510
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2527
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2528
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2530
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2531
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2548
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2549
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2551
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2552
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2569
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2570
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 2572
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2573
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2590
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2591
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2593
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2594
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2611
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2612
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2614
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2615
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2632
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2633
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2635
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2636
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2653
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2654
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2656
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2657
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2674
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2675
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2677
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2678
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2695
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2696
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2698
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2699
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2716
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2717
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2719
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2720
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2737
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2738
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2740
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2741
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2758
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2759
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2761
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2762
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2779
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2780
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2782
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2783
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2800
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 2801
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2803
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2804
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2821
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2822
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2824
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2825
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2842
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2843
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2845
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 2846
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2863
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2864
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2866
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2867
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2884
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2885
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2887
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2888
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2905
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2906
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2908
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2909
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2926
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2927
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2929
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2930
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2947
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2948
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2950
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2951
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2968
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2969
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2971
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2972
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2989
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2990
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2992
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2993
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3010
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3011
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3013
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 3014
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3031
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3032
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3034
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3035
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3052
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3053
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3055
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3056
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3073
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 3074
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3076
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3077
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3094
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3095
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3097
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3098
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3115
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3116
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3118
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3119
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3136
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3137
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3139
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3140
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3157
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3158
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3160
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3161
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3178
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3179
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3181
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 3182
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3199
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3200
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3202
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3203
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3220
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3221
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3223
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3224
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3241
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3242
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 3244
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3245
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3262
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3263
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3265
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3266
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3283
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3284
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3286
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3287
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 3304
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3305
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3307
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3308
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3325
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3326
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3328
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3329
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3346
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3347
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3349
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3350
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3367
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3368
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3370
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3371
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3388
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3389
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3391
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3392
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3409
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3410
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 3412
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3413
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3430
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3431
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3433
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3434
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3451
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3452
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3454
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3455
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3472
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 3473
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3475
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3476
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3493
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3494
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3496
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3497
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3514
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3515
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3517
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 3518
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3535
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3536
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3538
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3539
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3556
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3557
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3559
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3560
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3577
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3578
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3580
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3581
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3598
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3599
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3601
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3602
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3619
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3620
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3622
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3623
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3640
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 3641
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3643
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3644
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3661
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3662
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3664
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3665
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3682
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3683
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3687
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3692
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
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1
                                        else ((phase[:32] & 0) | 4294967295)
                                    )
                                    if phase < 2
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 4
                                        else (
                                            ((phase[:32] & 0) | 4294967294)
                                            if phase < 5
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 8
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967293)
                                        if phase < 9
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 13
                                    else (
                                        ((phase[:32] & 0) | 4294967292)
                                        if phase < 14
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 19
                                            else ((phase[:32] & 0) | 4294967291)
                                        )
                                    )
                                )
                            )
                            if phase < 20
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 26
                                        else ((phase[:32] & 0) | 4294967290)
                                    )
                                    if phase < 27
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 34
                                        else (
                                            ((phase[:32] & 0) | 4294967289)
                                            if phase < 35
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 43
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967288)
                                        if phase < 44
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 53
                                    else (
                                        ((phase[:32] & 0) | 4294967287)
                                        if phase < 54
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 64
                                            else ((phase[:32] & 0) | 4294967286)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 65
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 76
                                        else ((phase[:32] & 0) | 4294967285)
                                    )
                                    if phase < 77
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 89
                                        else (
                                            ((phase[:32] & 0) | 4294967284)
                                            if phase < 90
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 103
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967283)
                                        if phase < 104
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 118
                                    else (
                                        ((phase[:32] & 0) | 4294967282)
                                        if phase < 119
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 134
                                            else ((phase[:32] & 0) | 4294967281)
                                        )
                                    )
                                )
                            )
                            if phase < 135
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 151
                                        else ((phase[:32] & 0) | 4294967280)
                                    )
                                    if phase < 152
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 169
                                        else (
                                            ((phase[:32] & 0) | 10)
                                            if phase < 170
                                            else ((phase[:32] & 0) | 20)
                                        )
                                    )
                                )
                                if phase < 171
                                else (
                                    (
                                        ((phase[:32] & 0) | 30)
                                        if phase < 172
                                        else (
                                            ((phase[:32] & 0) | 40)
                                            if phase < 173
                                            else ((phase[:32] & 0) | 50)
                                        )
                                    )
                                    if phase < 174
                                    else (
                                        ((phase[:32] & 0) | 60)
                                        if phase < 175
                                        else (
                                            ((phase[:32] & 0) | 70)
                                            if phase < 176
                                            else ((phase[:32] & 0) | 100)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 177
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 247
                                        else ((phase[:32] & 0) | 10)
                                    )
                                    if phase < 248
                                    else (
                                        ((phase[:32] & 0) | 20)
                                        if phase < 249
                                        else (
                                            ((phase[:32] & 0) | 30)
                                            if phase < 250
                                            else ((phase[:32] & 0) | 40)
                                        )
                                    )
                                )
                                if phase < 251
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 490
                                        else ((phase[:32] & 0) | 1)
                                    )
                                    if phase < 491
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 493
                                        else (
                                            ((phase[:32] & 0) | 1)
                                            if phase < 494
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 497
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 1)
                                        if phase < 498
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 502
                                    else (
                                        ((phase[:32] & 0) | 1)
                                        if phase < 503
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 508
                                            else ((phase[:32] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 509
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 515
                                        else ((phase[:32] & 0) | 1)
                                    )
                                    if phase < 516
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 523
                                        else (
                                            ((phase[:32] & 0) | 1)
                                            if phase < 524
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 532
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 1)
                                        if phase < 533
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 542
                                    else (
                                        ((phase[:32] & 0) | 1)
                                        if phase < 543
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 553
                                            else ((phase[:32] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 554
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 565
                                        else ((phase[:32] & 0) | 1)
                                    )
                                    if phase < 566
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 578
                                        else (
                                            ((phase[:32] & 0) | 1)
                                            if phase < 579
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 592
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 1)
                                        if phase < 593
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 607
                                    else (
                                        ((phase[:32] & 0) | 1)
                                        if phase < 608
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 623
                                            else ((phase[:32] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 624
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 640
                                        else (
                                            ((phase[:32] & 0) | 1)
                                            if phase < 641
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 658
                                    else (
                                        ((phase[:32] & 0) | 15)
                                        if phase < 659
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 661
                                            else ((phase[:32] & 0) | 15)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 662
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 665
                                        else ((phase[:32] & 0) | 15)
                                    )
                                    if phase < 666
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 670
                                        else (
                                            ((phase[:32] & 0) | 15)
                                            if phase < 671
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 676
                                else (
                                    (
                                        ((phase[:32] & 0) | 15)
                                        if phase < 677
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 683
                                    else (
                                        ((phase[:32] & 0) | 15)
                                        if phase < 684
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 691
                                            else ((phase[:32] & 0) | 15)
                                        )
                                    )
                                )
                            )
                            if phase < 692
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 700
                                        else ((phase[:32] & 0) | 15)
                                    )
                                    if phase < 701
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 710
                                        else (
                                            ((phase[:32] & 0) | 15)
                                            if phase < 711
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 721
                                else (
                                    (
                                        ((phase[:32] & 0) | 15)
                                        if phase < 722
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 733
                                    else (
                                        ((phase[:32] & 0) | 15)
                                        if phase < 734
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 746
                                            else ((phase[:32] & 0) | 15)
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
                                        ((phase[:32] & 0) | 0)
                                        if phase < 760
                                        else ((phase[:32] & 0) | 15)
                                    )
                                    if phase < 761
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 775
                                        else (
                                            ((phase[:32] & 0) | 15)
                                            if phase < 776
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 791
                                else (
                                    (
                                        ((phase[:32] & 0) | 15)
                                        if phase < 792
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 808
                                    else (
                                        ((phase[:32] & 0) | 15)
                                        if phase < 809
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 826
                                            else ((phase[:32] & 0) | 16)
                                        )
                                    )
                                )
                            )
                            if phase < 827
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 829
                                        else ((phase[:32] & 0) | 16)
                                    )
                                    if phase < 830
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 833
                                        else (
                                            ((phase[:32] & 0) | 16)
                                            if phase < 834
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 838
                                else (
                                    (
                                        ((phase[:32] & 0) | 16)
                                        if phase < 839
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 844
                                            else ((phase[:32] & 0) | 16)
                                        )
                                    )
                                    if phase < 845
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 851
                                        else (
                                            ((phase[:32] & 0) | 16)
                                            if phase < 852
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 859
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 16)
                                        if phase < 860
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 868
                                    else (
                                        ((phase[:32] & 0) | 16)
                                        if phase < 869
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 878
                                            else ((phase[:32] & 0) | 16)
                                        )
                                    )
                                )
                                if phase < 879
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 889
                                        else ((phase[:32] & 0) | 16)
                                    )
                                    if phase < 890
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 901
                                        else (
                                            ((phase[:32] & 0) | 16)
                                            if phase < 902
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 914
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 16)
                                        if phase < 915
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 928
                                    else (
                                        ((phase[:32] & 0) | 16)
                                        if phase < 929
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 943
                                            else ((phase[:32] & 0) | 16)
                                        )
                                    )
                                )
                                if phase < 944
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 959
                                        else (
                                            ((phase[:32] & 0) | 16)
                                            if phase < 960
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 976
                                    else (
                                        ((phase[:32] & 0) | 16)
                                        if phase < 977
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 994
                                            else ((phase[:32] & 0) | 255)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 995
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 997
                                        else ((phase[:32] & 0) | 255)
                                    )
                                    if phase < 998
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1001
                                        else (
                                            ((phase[:32] & 0) | 255)
                                            if phase < 1002
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1006
                                else (
                                    (
                                        ((phase[:32] & 0) | 255)
                                        if phase < 1007
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1012
                                    else (
                                        ((phase[:32] & 0) | 255)
                                        if phase < 1013
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1019
                                            else ((phase[:32] & 0) | 255)
                                        )
                                    )
                                )
                            )
                            if phase < 1020
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1027
                                        else ((phase[:32] & 0) | 255)
                                    )
                                    if phase < 1028
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1036
                                        else (
                                            ((phase[:32] & 0) | 255)
                                            if phase < 1037
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1046
                                else (
                                    (
                                        ((phase[:32] & 0) | 255)
                                        if phase < 1047
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1057
                                            else ((phase[:32] & 0) | 255)
                                        )
                                    )
                                    if phase < 1058
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1069
                                        else (
                                            ((phase[:32] & 0) | 255)
                                            if phase < 1070
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
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
                                (
                                    (
                                        ((phase[:32] & 0) | 255)
                                        if phase < 1083
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1096
                                    else (
                                        ((phase[:32] & 0) | 255)
                                        if phase < 1097
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1111
                                            else ((phase[:32] & 0) | 255)
                                        )
                                    )
                                )
                                if phase < 1112
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1127
                                        else ((phase[:32] & 0) | 255)
                                    )
                                    if phase < 1128
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1144
                                        else (
                                            ((phase[:32] & 0) | 255)
                                            if phase < 1145
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1162
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 256)
                                        if phase < 1163
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1165
                                    else (
                                        ((phase[:32] & 0) | 256)
                                        if phase < 1166
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1169
                                            else ((phase[:32] & 0) | 256)
                                        )
                                    )
                                )
                                if phase < 1170
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1174
                                        else ((phase[:32] & 0) | 256)
                                    )
                                    if phase < 1175
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1180
                                        else (
                                            ((phase[:32] & 0) | 256)
                                            if phase < 1181
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1187
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 256)
                                        if phase < 1188
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1195
                                    else (
                                        ((phase[:32] & 0) | 256)
                                        if phase < 1196
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1204
                                            else ((phase[:32] & 0) | 256)
                                        )
                                    )
                                )
                                if phase < 1205
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1214
                                        else ((phase[:32] & 0) | 256)
                                    )
                                    if phase < 1215
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1225
                                        else (
                                            ((phase[:32] & 0) | 256)
                                            if phase < 1226
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1237
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 256)
                                        if phase < 1238
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1250
                                    else (
                                        ((phase[:32] & 0) | 256)
                                        if phase < 1251
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1264
                                            else ((phase[:32] & 0) | 256)
                                        )
                                    )
                                )
                                if phase < 1265
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1279
                                        else (
                                            ((phase[:32] & 0) | 256)
                                            if phase < 1280
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 1295
                                    else (
                                        ((phase[:32] & 0) | 256)
                                        if phase < 1296
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1312
                                            else ((phase[:32] & 0) | 256)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1313
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1330
                                        else ((phase[:32] & 0) | 2147483647)
                                    )
                                    if phase < 1331
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1333
                                        else (
                                            ((phase[:32] & 0) | 2147483647)
                                            if phase < 1334
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1337
                                else (
                                    (
                                        ((phase[:32] & 0) | 2147483647)
                                        if phase < 1338
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1342
                                    else (
                                        ((phase[:32] & 0) | 2147483647)
                                        if phase < 1343
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1348
                                            else ((phase[:32] & 0) | 2147483647)
                                        )
                                    )
                                )
                            )
                            if phase < 1349
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1355
                                        else ((phase[:32] & 0) | 2147483647)
                                    )
                                    if phase < 1356
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1363
                                        else (
                                            ((phase[:32] & 0) | 2147483647)
                                            if phase < 1364
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1372
                                else (
                                    (
                                        ((phase[:32] & 0) | 2147483647)
                                        if phase < 1373
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1382
                                            else ((phase[:32] & 0) | 2147483647)
                                        )
                                    )
                                    if phase < 1383
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1393
                                        else (
                                            ((phase[:32] & 0) | 2147483647)
                                            if phase < 1394
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1405
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 2147483647)
                                        if phase < 1406
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1418
                                    else (
                                        ((phase[:32] & 0) | 2147483647)
                                        if phase < 1419
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1432
                                            else ((phase[:32] & 0) | 2147483647)
                                        )
                                    )
                                )
                                if phase < 1433
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1447
                                        else ((phase[:32] & 0) | 2147483647)
                                    )
                                    if phase < 1448
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1463
                                        else (
                                            ((phase[:32] & 0) | 2147483647)
                                            if phase < 1464
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1480
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 2147483647)
                                        if phase < 1481
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1498
                                    else (
                                        ((phase[:32] & 0) | 2147483648)
                                        if phase < 1499
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1501
                                            else ((phase[:32] & 0) | 2147483648)
                                        )
                                    )
                                )
                                if phase < 1502
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1505
                                        else (
                                            ((phase[:32] & 0) | 2147483648)
                                            if phase < 1506
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 1510
                                    else (
                                        ((phase[:32] & 0) | 2147483648)
                                        if phase < 1511
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1516
                                            else ((phase[:32] & 0) | 2147483648)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1517
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1523
                                        else ((phase[:32] & 0) | 2147483648)
                                    )
                                    if phase < 1524
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1531
                                        else (
                                            ((phase[:32] & 0) | 2147483648)
                                            if phase < 1532
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1540
                                else (
                                    (
                                        ((phase[:32] & 0) | 2147483648)
                                        if phase < 1541
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1550
                                    else (
                                        ((phase[:32] & 0) | 2147483648)
                                        if phase < 1551
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1561
                                            else ((phase[:32] & 0) | 2147483648)
                                        )
                                    )
                                )
                            )
                            if phase < 1562
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1573
                                        else ((phase[:32] & 0) | 2147483648)
                                    )
                                    if phase < 1574
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1586
                                        else (
                                            ((phase[:32] & 0) | 2147483648)
                                            if phase < 1587
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1600
                                else (
                                    (
                                        ((phase[:32] & 0) | 2147483648)
                                        if phase < 1601
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1615
                                    else (
                                        ((phase[:32] & 0) | 2147483648)
                                        if phase < 1616
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1631
                                            else ((phase[:32] & 0) | 2147483648)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1632
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1648
                                        else ((phase[:32] & 0) | 2147483648)
                                    )
                                    if phase < 1649
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1666
                                        else (
                                            ((phase[:32] & 0) | 4294967295)
                                            if phase < 1667
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1669
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 1670
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1673
                                    else (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 1674
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1678
                                            else ((phase[:32] & 0) | 4294967295)
                                        )
                                    )
                                )
                            )
                            if phase < 1679
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1684
                                        else ((phase[:32] & 0) | 4294967295)
                                    )
                                    if phase < 1685
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1691
                                        else (
                                            ((phase[:32] & 0) | 4294967295)
                                            if phase < 1692
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1699
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 1700
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1708
                                            else ((phase[:32] & 0) | 4294967295)
                                        )
                                    )
                                    if phase < 1709
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1718
                                        else (
                                            ((phase[:32] & 0) | 4294967295)
                                            if phase < 1719
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1729
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 1730
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1741
                                    else (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 1742
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1754
                                            else ((phase[:32] & 0) | 4294967295)
                                        )
                                    )
                                )
                                if phase < 1755
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1768
                                        else ((phase[:32] & 0) | 4294967295)
                                    )
                                    if phase < 1769
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1783
                                        else (
                                            ((phase[:32] & 0) | 4294967295)
                                            if phase < 1784
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1799
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 1800
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1816
                                    else (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 1817
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1834
                                            else ((phase[:32] & 0) | 4294967294)
                                        )
                                    )
                                )
                                if phase < 1835
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1837
                                        else (
                                            ((phase[:32] & 0) | 4294967294)
                                            if phase < 1838
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 1841
                                    else (
                                        ((phase[:32] & 0) | 4294967294)
                                        if phase < 1842
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1846
                                            else ((phase[:32] & 0) | 4294967294)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1847
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1852
                                        else ((phase[:32] & 0) | 4294967294)
                                    )
                                    if phase < 1853
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1859
                                        else (
                                            ((phase[:32] & 0) | 4294967294)
                                            if phase < 1860
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1867
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967294)
                                        if phase < 1868
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1876
                                    else (
                                        ((phase[:32] & 0) | 4294967294)
                                        if phase < 1877
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1886
                                            else ((phase[:32] & 0) | 4294967294)
                                        )
                                    )
                                )
                            )
                            if phase < 1887
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1897
                                        else ((phase[:32] & 0) | 4294967294)
                                    )
                                    if phase < 1898
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1909
                                        else (
                                            ((phase[:32] & 0) | 4294967294)
                                            if phase < 1910
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1922
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967294)
                                        if phase < 1923
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1936
                                            else ((phase[:32] & 0) | 4294967294)
                                        )
                                    )
                                    if phase < 1937
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1951
                                        else (
                                            ((phase[:32] & 0) | 4294967294)
                                            if phase < 1952
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
        if phase < 1967
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 4294967294)
                                        if phase < 1968
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1984
                                    else (
                                        ((phase[:32] & 0) | 4294967294)
                                        if phase < 1985
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2002
                                            else ((phase[:32] & 0) | 1431655765)
                                        )
                                    )
                                )
                                if phase < 2003
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2005
                                        else ((phase[:32] & 0) | 1431655765)
                                    )
                                    if phase < 2006
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2009
                                        else (
                                            ((phase[:32] & 0) | 1431655765)
                                            if phase < 2010
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2014
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 1431655765)
                                        if phase < 2015
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2020
                                    else (
                                        ((phase[:32] & 0) | 1431655765)
                                        if phase < 2021
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2027
                                            else ((phase[:32] & 0) | 1431655765)
                                        )
                                    )
                                )
                                if phase < 2028
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2035
                                        else ((phase[:32] & 0) | 1431655765)
                                    )
                                    if phase < 2036
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2044
                                        else (
                                            ((phase[:32] & 0) | 1431655765)
                                            if phase < 2045
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2054
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 1431655765)
                                        if phase < 2055
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2065
                                    else (
                                        ((phase[:32] & 0) | 1431655765)
                                        if phase < 2066
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2077
                                            else ((phase[:32] & 0) | 1431655765)
                                        )
                                    )
                                )
                                if phase < 2078
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2090
                                        else ((phase[:32] & 0) | 1431655765)
                                    )
                                    if phase < 2091
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2104
                                        else (
                                            ((phase[:32] & 0) | 1431655765)
                                            if phase < 2105
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2119
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 1431655765)
                                        if phase < 2120
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2135
                                    else (
                                        ((phase[:32] & 0) | 1431655765)
                                        if phase < 2136
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2152
                                            else ((phase[:32] & 0) | 1431655765)
                                        )
                                    )
                                )
                                if phase < 2153
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2170
                                        else (
                                            ((phase[:32] & 0) | 2863311530)
                                            if phase < 2171
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 2173
                                    else (
                                        ((phase[:32] & 0) | 2863311530)
                                        if phase < 2174
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2177
                                            else ((phase[:32] & 0) | 2863311530)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 2178
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2182
                                        else ((phase[:32] & 0) | 2863311530)
                                    )
                                    if phase < 2183
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2188
                                        else (
                                            ((phase[:32] & 0) | 2863311530)
                                            if phase < 2189
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2195
                                else (
                                    (
                                        ((phase[:32] & 0) | 2863311530)
                                        if phase < 2196
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2203
                                    else (
                                        ((phase[:32] & 0) | 2863311530)
                                        if phase < 2204
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2212
                                            else ((phase[:32] & 0) | 2863311530)
                                        )
                                    )
                                )
                            )
                            if phase < 2213
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2222
                                        else ((phase[:32] & 0) | 2863311530)
                                    )
                                    if phase < 2223
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2233
                                        else (
                                            ((phase[:32] & 0) | 2863311530)
                                            if phase < 2234
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2245
                                else (
                                    (
                                        ((phase[:32] & 0) | 2863311530)
                                        if phase < 2246
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2258
                                    else (
                                        ((phase[:32] & 0) | 2863311530)
                                        if phase < 2259
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2272
                                            else ((phase[:32] & 0) | 2863311530)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2273
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2287
                                        else ((phase[:32] & 0) | 2863311530)
                                    )
                                    if phase < 2288
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2303
                                        else (
                                            ((phase[:32] & 0) | 2863311530)
                                            if phase < 2304
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2320
                                else (
                                    (
                                        ((phase[:32] & 0) | 2863311530)
                                        if phase < 2321
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2338
                                    else (
                                        ((phase[:32] & 0) | 1)
                                        if phase < 2339
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2341
                                            else ((phase[:32] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 2342
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2359
                                        else ((phase[:32] & 0) | 4294967294)
                                    )
                                    if phase < 2360
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2362
                                        else (
                                            ((phase[:32] & 0) | 4294967294)
                                            if phase < 2363
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2380
                                else (
                                    (
                                        ((phase[:32] & 0) | 2)
                                        if phase < 2381
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2383
                                            else ((phase[:32] & 0) | 2)
                                        )
                                    )
                                    if phase < 2384
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2401
                                        else (
                                            ((phase[:32] & 0) | 4294967293)
                                            if phase < 2402
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 2404
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 4294967293)
                                        if phase < 2405
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2422
                                    else (
                                        ((phase[:32] & 0) | 4)
                                        if phase < 2423
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2425
                                            else ((phase[:32] & 0) | 4)
                                        )
                                    )
                                )
                                if phase < 2426
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2443
                                        else ((phase[:32] & 0) | 4294967291)
                                    )
                                    if phase < 2444
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2446
                                        else (
                                            ((phase[:32] & 0) | 4294967291)
                                            if phase < 2447
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2464
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 8)
                                        if phase < 2465
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2467
                                    else (
                                        ((phase[:32] & 0) | 8)
                                        if phase < 2468
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2485
                                            else ((phase[:32] & 0) | 4294967287)
                                        )
                                    )
                                )
                                if phase < 2486
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2488
                                        else ((phase[:32] & 0) | 4294967287)
                                    )
                                    if phase < 2489
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2506
                                        else (
                                            ((phase[:32] & 0) | 16)
                                            if phase < 2507
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2509
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 16)
                                        if phase < 2510
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2527
                                    else (
                                        ((phase[:32] & 0) | 4294967279)
                                        if phase < 2528
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2530
                                            else ((phase[:32] & 0) | 4294967279)
                                        )
                                    )
                                )
                                if phase < 2531
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2548
                                        else ((phase[:32] & 0) | 32)
                                    )
                                    if phase < 2549
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2551
                                        else (
                                            ((phase[:32] & 0) | 32)
                                            if phase < 2552
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2569
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 4294967263)
                                        if phase < 2570
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2572
                                    else (
                                        ((phase[:32] & 0) | 4294967263)
                                        if phase < 2573
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2590
                                            else ((phase[:32] & 0) | 64)
                                        )
                                    )
                                )
                                if phase < 2591
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2593
                                        else (
                                            ((phase[:32] & 0) | 64)
                                            if phase < 2594
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 2611
                                    else (
                                        ((phase[:32] & 0) | 4294967231)
                                        if phase < 2612
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2614
                                            else ((phase[:32] & 0) | 4294967231)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 2615
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2632
                                        else ((phase[:32] & 0) | 128)
                                    )
                                    if phase < 2633
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2635
                                        else (
                                            ((phase[:32] & 0) | 128)
                                            if phase < 2636
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2653
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967167)
                                        if phase < 2654
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2656
                                    else (
                                        ((phase[:32] & 0) | 4294967167)
                                        if phase < 2657
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2674
                                            else ((phase[:32] & 0) | 256)
                                        )
                                    )
                                )
                            )
                            if phase < 2675
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2677
                                        else ((phase[:32] & 0) | 256)
                                    )
                                    if phase < 2678
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2695
                                        else (
                                            ((phase[:32] & 0) | 4294967039)
                                            if phase < 2696
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2698
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967039)
                                        if phase < 2699
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2716
                                            else ((phase[:32] & 0) | 512)
                                        )
                                    )
                                    if phase < 2717
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2719
                                        else (
                                            ((phase[:32] & 0) | 512)
                                            if phase < 2720
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2737
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 4294966783)
                                        if phase < 2738
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2740
                                    else (
                                        ((phase[:32] & 0) | 4294966783)
                                        if phase < 2741
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2758
                                            else ((phase[:32] & 0) | 1024)
                                        )
                                    )
                                )
                                if phase < 2759
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2761
                                        else ((phase[:32] & 0) | 1024)
                                    )
                                    if phase < 2762
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2779
                                        else (
                                            ((phase[:32] & 0) | 4294966271)
                                            if phase < 2780
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2782
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 4294966271)
                                        if phase < 2783
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2800
                                    else (
                                        ((phase[:32] & 0) | 2048)
                                        if phase < 2801
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2803
                                            else ((phase[:32] & 0) | 2048)
                                        )
                                    )
                                )
                                if phase < 2804
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2821
                                        else (
                                            ((phase[:32] & 0) | 4294965247)
                                            if phase < 2822
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 2824
                                    else (
                                        ((phase[:32] & 0) | 4294965247)
                                        if phase < 2825
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2842
                                            else ((phase[:32] & 0) | 4096)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 2843
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2845
                                        else ((phase[:32] & 0) | 4096)
                                    )
                                    if phase < 2846
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2863
                                        else (
                                            ((phase[:32] & 0) | 4294963199)
                                            if phase < 2864
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2866
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294963199)
                                        if phase < 2867
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2884
                                    else (
                                        ((phase[:32] & 0) | 8192)
                                        if phase < 2885
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2887
                                            else ((phase[:32] & 0) | 8192)
                                        )
                                    )
                                )
                            )
                            if phase < 2888
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2905
                                        else ((phase[:32] & 0) | 4294959103)
                                    )
                                    if phase < 2906
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2908
                                        else (
                                            ((phase[:32] & 0) | 4294959103)
                                            if phase < 2909
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2926
                                else (
                                    (
                                        ((phase[:32] & 0) | 16384)
                                        if phase < 2927
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2929
                                    else (
                                        ((phase[:32] & 0) | 16384)
                                        if phase < 2930
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2947
                                            else ((phase[:32] & 0) | 4294950911)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2948
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2950
                                        else ((phase[:32] & 0) | 4294950911)
                                    )
                                    if phase < 2951
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2968
                                        else (
                                            ((phase[:32] & 0) | 32768)
                                            if phase < 2969
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2971
                                else (
                                    (
                                        ((phase[:32] & 0) | 32768)
                                        if phase < 2972
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2989
                                    else (
                                        ((phase[:32] & 0) | 4294934527)
                                        if phase < 2990
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2992
                                            else ((phase[:32] & 0) | 4294934527)
                                        )
                                    )
                                )
                            )
                            if phase < 2993
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3010
                                        else ((phase[:32] & 0) | 65536)
                                    )
                                    if phase < 3011
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3013
                                        else (
                                            ((phase[:32] & 0) | 65536)
                                            if phase < 3014
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3031
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294901759)
                                        if phase < 3032
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3034
                                            else ((phase[:32] & 0) | 4294901759)
                                        )
                                    )
                                    if phase < 3035
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3052
                                        else (
                                            ((phase[:32] & 0) | 131072)
                                            if phase < 3053
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 3055
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 131072)
                                        if phase < 3056
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3073
                                    else (
                                        ((phase[:32] & 0) | 4294836223)
                                        if phase < 3074
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3076
                                            else ((phase[:32] & 0) | 4294836223)
                                        )
                                    )
                                )
                                if phase < 3077
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3094
                                        else ((phase[:32] & 0) | 262144)
                                    )
                                    if phase < 3095
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3097
                                        else (
                                            ((phase[:32] & 0) | 262144)
                                            if phase < 3098
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 3115
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 4294705151)
                                        if phase < 3116
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3118
                                    else (
                                        ((phase[:32] & 0) | 4294705151)
                                        if phase < 3119
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3136
                                            else ((phase[:32] & 0) | 524288)
                                        )
                                    )
                                )
                                if phase < 3137
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3139
                                        else (
                                            ((phase[:32] & 0) | 524288)
                                            if phase < 3140
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 3157
                                    else (
                                        ((phase[:32] & 0) | 4294443007)
                                        if phase < 3158
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3160
                                            else ((phase[:32] & 0) | 4294443007)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3161
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3178
                                        else ((phase[:32] & 0) | 1048576)
                                    )
                                    if phase < 3179
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3181
                                        else (
                                            ((phase[:32] & 0) | 1048576)
                                            if phase < 3182
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3199
                                else (
                                    (
                                        ((phase[:32] & 0) | 4293918719)
                                        if phase < 3200
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3202
                                    else (
                                        ((phase[:32] & 0) | 4293918719)
                                        if phase < 3203
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3220
                                            else ((phase[:32] & 0) | 2097152)
                                        )
                                    )
                                )
                            )
                            if phase < 3221
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3223
                                        else ((phase[:32] & 0) | 2097152)
                                    )
                                    if phase < 3224
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3241
                                        else (
                                            ((phase[:32] & 0) | 4292870143)
                                            if phase < 3242
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3244
                                else (
                                    (
                                        ((phase[:32] & 0) | 4292870143)
                                        if phase < 3245
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3262
                                            else ((phase[:32] & 0) | 4194304)
                                        )
                                    )
                                    if phase < 3263
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3265
                                        else (
                                            ((phase[:32] & 0) | 4194304)
                                            if phase < 3266
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 3283
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 4290772991)
                                        if phase < 3284
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3286
                                    else (
                                        ((phase[:32] & 0) | 4290772991)
                                        if phase < 3287
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3304
                                            else ((phase[:32] & 0) | 8388608)
                                        )
                                    )
                                )
                                if phase < 3305
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3307
                                        else ((phase[:32] & 0) | 8388608)
                                    )
                                    if phase < 3308
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3325
                                        else (
                                            ((phase[:32] & 0) | 4286578687)
                                            if phase < 3326
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 3328
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 4286578687)
                                        if phase < 3329
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3346
                                    else (
                                        ((phase[:32] & 0) | 16777216)
                                        if phase < 3347
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3349
                                            else ((phase[:32] & 0) | 16777216)
                                        )
                                    )
                                )
                                if phase < 3350
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3367
                                        else ((phase[:32] & 0) | 4278190079)
                                    )
                                    if phase < 3368
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3370
                                        else (
                                            ((phase[:32] & 0) | 4278190079)
                                            if phase < 3371
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3388
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 33554432)
                                        if phase < 3389
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3391
                                    else (
                                        ((phase[:32] & 0) | 33554432)
                                        if phase < 3392
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3409
                                            else ((phase[:32] & 0) | 4261412863)
                                        )
                                    )
                                )
                                if phase < 3410
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3412
                                        else ((phase[:32] & 0) | 4261412863)
                                    )
                                    if phase < 3413
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3430
                                        else (
                                            ((phase[:32] & 0) | 67108864)
                                            if phase < 3431
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 3433
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 67108864)
                                        if phase < 3434
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3451
                                    else (
                                        ((phase[:32] & 0) | 4227858431)
                                        if phase < 3452
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3454
                                            else ((phase[:32] & 0) | 4227858431)
                                        )
                                    )
                                )
                                if phase < 3455
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3472
                                        else (
                                            ((phase[:32] & 0) | 134217728)
                                            if phase < 3473
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 3475
                                    else (
                                        ((phase[:32] & 0) | 134217728)
                                        if phase < 3476
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3493
                                            else ((phase[:32] & 0) | 4160749567)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 3494
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3496
                                        else ((phase[:32] & 0) | 4160749567)
                                    )
                                    if phase < 3497
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3514
                                        else (
                                            ((phase[:32] & 0) | 268435456)
                                            if phase < 3515
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3517
                                else (
                                    (
                                        ((phase[:32] & 0) | 268435456)
                                        if phase < 3518
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3535
                                    else (
                                        ((phase[:32] & 0) | 4026531839)
                                        if phase < 3536
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3538
                                            else ((phase[:32] & 0) | 4026531839)
                                        )
                                    )
                                )
                            )
                            if phase < 3539
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3556
                                        else ((phase[:32] & 0) | 536870912)
                                    )
                                    if phase < 3557
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3559
                                        else (
                                            ((phase[:32] & 0) | 536870912)
                                            if phase < 3560
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3577
                                else (
                                    (
                                        ((phase[:32] & 0) | 3758096383)
                                        if phase < 3578
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3580
                                            else ((phase[:32] & 0) | 3758096383)
                                        )
                                    )
                                    if phase < 3581
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3598
                                        else (
                                            ((phase[:32] & 0) | 1073741824)
                                            if phase < 3599
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3601
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 1073741824)
                                        if phase < 3602
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3619
                                    else (
                                        ((phase[:32] & 0) | 3221225471)
                                        if phase < 3620
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3622
                                            else ((phase[:32] & 0) | 3221225471)
                                        )
                                    )
                                )
                                if phase < 3623
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3640
                                        else ((phase[:32] & 0) | 2147483648)
                                    )
                                    if phase < 3641
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3643
                                        else (
                                            ((phase[:32] & 0) | 2147483648)
                                            if phase < 3644
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 3661
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 2147483647)
                                        if phase < 3662
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3664
                                    else (
                                        ((phase[:32] & 0) | 2147483647)
                                        if phase < 3665
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3682
                                            else ((phase[:32] & 0) | 4294967295)
                                        )
                                    )
                                )
                                if phase < 3683
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3687
                                        else (
                                            ((phase[:32] & 0) | 10)
                                            if phase < 3688
                                            else ((phase[:32] & 0) | 20)
                                        )
                                    )
                                    if phase < 3689
                                    else (
                                        ((phase[:32] & 0) | 30)
                                        if phase < 3690
                                        else (
                                            ((phase[:32] & 0) | 40)
                                            if phase < 3691
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
    )
    remaining = (
        (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 4
                                        else ((phase[:4] & 0) | 1)
                                    )
                                    if phase < 5
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 8
                                        else ((phase[:4] & 0) | 2)
                                    )
                                )
                                if phase < 9
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 13
                                        else ((phase[:4] & 0) | 3)
                                    )
                                    if phase < 14
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 19
                                        else ((phase[:4] & 0) | 4)
                                    )
                                )
                            )
                            if phase < 20
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 26
                                        else ((phase[:4] & 0) | 5)
                                    )
                                    if phase < 27
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 34
                                        else ((phase[:4] & 0) | 6)
                                    )
                                )
                                if phase < 35
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 43
                                        else ((phase[:4] & 0) | 7)
                                    )
                                    if phase < 44
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 53
                                        else ((phase[:4] & 0) | 8)
                                    )
                                )
                            )
                        )
                        if phase < 54
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 64
                                        else ((phase[:4] & 0) | 9)
                                    )
                                    if phase < 65
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 76
                                        else ((phase[:4] & 0) | 10)
                                    )
                                )
                                if phase < 77
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 89
                                        else ((phase[:4] & 0) | 11)
                                    )
                                    if phase < 90
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 103
                                        else ((phase[:4] & 0) | 12)
                                    )
                                )
                            )
                            if phase < 104
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 118
                                        else ((phase[:4] & 0) | 13)
                                    )
                                    if phase < 119
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 134
                                        else ((phase[:4] & 0) | 14)
                                    )
                                )
                                if phase < 135
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 151
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 152
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 170
                                        else (
                                            ((phase[:4] & 0) | 3)
                                            if phase < 171
                                            else ((phase[:4] & 0) | 1)
                                        )
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
                                    (
                                        ((phase[:4] & 0) | 2)
                                        if phase < 173
                                        else ((phase[:4] & 0) | 4)
                                    )
                                    if phase < 175
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 248
                                        else ((phase[:4] & 0) | 3)
                                    )
                                )
                                if phase < 249
                                else (
                                    (
                                        ((phase[:4] & 0) | 1)
                                        if phase < 250
                                        else ((phase[:4] & 0) | 2)
                                    )
                                    if phase < 251
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 325
                                        else ((phase[:4] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 326
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 329
                                        else ((phase[:4] & 0) | 2)
                                    )
                                    if phase < 330
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 334
                                        else ((phase[:4] & 0) | 3)
                                    )
                                )
                                if phase < 335
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 340
                                        else ((phase[:4] & 0) | 4)
                                    )
                                    if phase < 341
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 347
                                        else ((phase[:4] & 0) | 5)
                                    )
                                )
                            )
                        )
                        if phase < 348
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 355
                                        else ((phase[:4] & 0) | 6)
                                    )
                                    if phase < 356
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 364
                                        else ((phase[:4] & 0) | 7)
                                    )
                                )
                                if phase < 365
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 374
                                        else ((phase[:4] & 0) | 8)
                                    )
                                    if phase < 375
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 385
                                        else ((phase[:4] & 0) | 9)
                                    )
                                )
                            )
                            if phase < 386
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 397
                                        else ((phase[:4] & 0) | 10)
                                    )
                                    if phase < 398
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 410
                                        else ((phase[:4] & 0) | 11)
                                    )
                                )
                                if phase < 411
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 424
                                        else ((phase[:4] & 0) | 12)
                                    )
                                    if phase < 425
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 439
                                        else (
                                            ((phase[:4] & 0) | 13)
                                            if phase < 440
                                            else ((phase[:4] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 455
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 14)
                                        if phase < 456
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 472
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 473
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 493
                                else (
                                    (
                                        ((phase[:4] & 0) | 1)
                                        if phase < 494
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 497
                                    else (
                                        ((phase[:4] & 0) | 2)
                                        if phase < 498
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 502
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 3)
                                        if phase < 503
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 508
                                    else (
                                        ((phase[:4] & 0) | 4)
                                        if phase < 509
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 515
                                else (
                                    (
                                        ((phase[:4] & 0) | 5)
                                        if phase < 516
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 523
                                    else (
                                        ((phase[:4] & 0) | 6)
                                        if phase < 524
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 532
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 7)
                                        if phase < 533
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 542
                                    else (
                                        ((phase[:4] & 0) | 8)
                                        if phase < 543
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 553
                                else (
                                    (
                                        ((phase[:4] & 0) | 9)
                                        if phase < 554
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 565
                                    else (
                                        ((phase[:4] & 0) | 10)
                                        if phase < 566
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 578
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 11)
                                        if phase < 579
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 592
                                    else (
                                        ((phase[:4] & 0) | 12)
                                        if phase < 593
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 607
                                else (
                                    (
                                        ((phase[:4] & 0) | 13)
                                        if phase < 608
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 623
                                    else (
                                        ((phase[:4] & 0) | 14)
                                        if phase < 624
                                        else (
                                            ((phase[:4] & 0) | 0)
                                            if phase < 640
                                            else ((phase[:4] & 0) | 15)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 641
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 661
                                        else ((phase[:4] & 0) | 1)
                                    )
                                    if phase < 662
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 665
                                        else ((phase[:4] & 0) | 2)
                                    )
                                )
                                if phase < 666
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 670
                                        else ((phase[:4] & 0) | 3)
                                    )
                                    if phase < 671
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 676
                                        else ((phase[:4] & 0) | 4)
                                    )
                                )
                            )
                            if phase < 677
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 683
                                        else ((phase[:4] & 0) | 5)
                                    )
                                    if phase < 684
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 691
                                        else ((phase[:4] & 0) | 6)
                                    )
                                )
                                if phase < 692
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 700
                                        else ((phase[:4] & 0) | 7)
                                    )
                                    if phase < 701
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 710
                                        else (
                                            ((phase[:4] & 0) | 8)
                                            if phase < 711
                                            else ((phase[:4] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 721
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 9)
                                        if phase < 722
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 733
                                    else (
                                        ((phase[:4] & 0) | 10)
                                        if phase < 734
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 746
                                else (
                                    (
                                        ((phase[:4] & 0) | 11)
                                        if phase < 747
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 760
                                    else (
                                        ((phase[:4] & 0) | 12)
                                        if phase < 761
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 775
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 13)
                                        if phase < 776
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 791
                                    else (
                                        ((phase[:4] & 0) | 14)
                                        if phase < 792
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 808
                                else (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 809
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 829
                                    else (
                                        ((phase[:4] & 0) | 1)
                                        if phase < 830
                                        else (
                                            ((phase[:4] & 0) | 0)
                                            if phase < 833
                                            else ((phase[:4] & 0) | 2)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 834
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 838
                                        else ((phase[:4] & 0) | 3)
                                    )
                                    if phase < 839
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 844
                                        else ((phase[:4] & 0) | 4)
                                    )
                                )
                                if phase < 845
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 851
                                        else ((phase[:4] & 0) | 5)
                                    )
                                    if phase < 852
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 859
                                        else ((phase[:4] & 0) | 6)
                                    )
                                )
                            )
                            if phase < 860
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 868
                                        else ((phase[:4] & 0) | 7)
                                    )
                                    if phase < 869
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 878
                                        else ((phase[:4] & 0) | 8)
                                    )
                                )
                                if phase < 879
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 889
                                        else ((phase[:4] & 0) | 9)
                                    )
                                    if phase < 890
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 901
                                        else ((phase[:4] & 0) | 10)
                                    )
                                )
                            )
                        )
                        if phase < 902
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 914
                                        else ((phase[:4] & 0) | 11)
                                    )
                                    if phase < 915
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 928
                                        else ((phase[:4] & 0) | 12)
                                    )
                                )
                                if phase < 929
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 943
                                        else ((phase[:4] & 0) | 13)
                                    )
                                    if phase < 944
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 959
                                        else ((phase[:4] & 0) | 14)
                                    )
                                )
                            )
                            if phase < 960
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 976
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 977
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 997
                                        else ((phase[:4] & 0) | 1)
                                    )
                                )
                                if phase < 998
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1001
                                        else ((phase[:4] & 0) | 2)
                                    )
                                    if phase < 1002
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1006
                                        else (
                                            ((phase[:4] & 0) | 3)
                                            if phase < 1007
                                            else ((phase[:4] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1012
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 4)
                                        if phase < 1013
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1019
                                    else (
                                        ((phase[:4] & 0) | 5)
                                        if phase < 1020
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1027
                                else (
                                    (
                                        ((phase[:4] & 0) | 6)
                                        if phase < 1028
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1036
                                    else (
                                        ((phase[:4] & 0) | 7)
                                        if phase < 1037
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1046
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 8)
                                        if phase < 1047
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1057
                                    else (
                                        ((phase[:4] & 0) | 9)
                                        if phase < 1058
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1069
                                else (
                                    (
                                        ((phase[:4] & 0) | 10)
                                        if phase < 1070
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1082
                                    else (
                                        ((phase[:4] & 0) | 11)
                                        if phase < 1083
                                        else (
                                            ((phase[:4] & 0) | 0)
                                            if phase < 1096
                                            else ((phase[:4] & 0) | 12)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1097
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1111
                                        else ((phase[:4] & 0) | 13)
                                    )
                                    if phase < 1112
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1127
                                        else ((phase[:4] & 0) | 14)
                                    )
                                )
                                if phase < 1128
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1144
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 1145
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1165
                                        else ((phase[:4] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 1166
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1169
                                        else ((phase[:4] & 0) | 2)
                                    )
                                    if phase < 1170
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1174
                                        else ((phase[:4] & 0) | 3)
                                    )
                                )
                                if phase < 1175
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1180
                                        else ((phase[:4] & 0) | 4)
                                    )
                                    if phase < 1181
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1187
                                        else (
                                            ((phase[:4] & 0) | 5)
                                            if phase < 1188
                                            else ((phase[:4] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1195
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 6)
                                        if phase < 1196
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1204
                                    else (
                                        ((phase[:4] & 0) | 7)
                                        if phase < 1205
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1214
                                else (
                                    (
                                        ((phase[:4] & 0) | 8)
                                        if phase < 1215
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1225
                                    else (
                                        ((phase[:4] & 0) | 9)
                                        if phase < 1226
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1237
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 10)
                                        if phase < 1238
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1250
                                    else (
                                        ((phase[:4] & 0) | 11)
                                        if phase < 1251
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1264
                                else (
                                    (
                                        ((phase[:4] & 0) | 12)
                                        if phase < 1265
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1279
                                    else (
                                        ((phase[:4] & 0) | 13)
                                        if phase < 1280
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 1295
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 14)
                                        if phase < 1296
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1312
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 1313
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1333
                                else (
                                    (
                                        ((phase[:4] & 0) | 1)
                                        if phase < 1334
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1337
                                    else (
                                        ((phase[:4] & 0) | 2)
                                        if phase < 1338
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1342
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 3)
                                        if phase < 1343
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1348
                                    else (
                                        ((phase[:4] & 0) | 4)
                                        if phase < 1349
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1355
                                else (
                                    (
                                        ((phase[:4] & 0) | 5)
                                        if phase < 1356
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1363
                                    else (
                                        ((phase[:4] & 0) | 6)
                                        if phase < 1364
                                        else (
                                            ((phase[:4] & 0) | 0)
                                            if phase < 1372
                                            else ((phase[:4] & 0) | 7)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1373
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1382
                                        else ((phase[:4] & 0) | 8)
                                    )
                                    if phase < 1383
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1393
                                        else ((phase[:4] & 0) | 9)
                                    )
                                )
                                if phase < 1394
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1405
                                        else ((phase[:4] & 0) | 10)
                                    )
                                    if phase < 1406
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1418
                                        else ((phase[:4] & 0) | 11)
                                    )
                                )
                            )
                            if phase < 1419
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1432
                                        else ((phase[:4] & 0) | 12)
                                    )
                                    if phase < 1433
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1447
                                        else ((phase[:4] & 0) | 13)
                                    )
                                )
                                if phase < 1448
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1463
                                        else ((phase[:4] & 0) | 14)
                                    )
                                    if phase < 1464
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1480
                                        else (
                                            ((phase[:4] & 0) | 15)
                                            if phase < 1481
                                            else ((phase[:4] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1501
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 1)
                                        if phase < 1502
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1505
                                    else (
                                        ((phase[:4] & 0) | 2)
                                        if phase < 1506
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1510
                                else (
                                    (
                                        ((phase[:4] & 0) | 3)
                                        if phase < 1511
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1516
                                    else (
                                        ((phase[:4] & 0) | 4)
                                        if phase < 1517
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1523
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 5)
                                        if phase < 1524
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1531
                                    else (
                                        ((phase[:4] & 0) | 6)
                                        if phase < 1532
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1540
                                else (
                                    (
                                        ((phase[:4] & 0) | 7)
                                        if phase < 1541
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1550
                                    else (
                                        ((phase[:4] & 0) | 8)
                                        if phase < 1551
                                        else (
                                            ((phase[:4] & 0) | 0)
                                            if phase < 1561
                                            else ((phase[:4] & 0) | 9)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 1562
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1573
                                        else ((phase[:4] & 0) | 10)
                                    )
                                    if phase < 1574
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1586
                                        else ((phase[:4] & 0) | 11)
                                    )
                                )
                                if phase < 1587
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1600
                                        else ((phase[:4] & 0) | 12)
                                    )
                                    if phase < 1601
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1615
                                        else ((phase[:4] & 0) | 13)
                                    )
                                )
                            )
                            if phase < 1616
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1631
                                        else ((phase[:4] & 0) | 14)
                                    )
                                    if phase < 1632
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1648
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                                if phase < 1649
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1669
                                        else ((phase[:4] & 0) | 1)
                                    )
                                    if phase < 1670
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1673
                                        else ((phase[:4] & 0) | 2)
                                    )
                                )
                            )
                        )
                        if phase < 1674
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1678
                                        else ((phase[:4] & 0) | 3)
                                    )
                                    if phase < 1679
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1684
                                        else ((phase[:4] & 0) | 4)
                                    )
                                )
                                if phase < 1685
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1691
                                        else ((phase[:4] & 0) | 5)
                                    )
                                    if phase < 1692
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1699
                                        else ((phase[:4] & 0) | 6)
                                    )
                                )
                            )
                            if phase < 1700
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1708
                                        else ((phase[:4] & 0) | 7)
                                    )
                                    if phase < 1709
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1718
                                        else ((phase[:4] & 0) | 8)
                                    )
                                )
                                if phase < 1719
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1729
                                        else ((phase[:4] & 0) | 9)
                                    )
                                    if phase < 1730
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1741
                                        else (
                                            ((phase[:4] & 0) | 10)
                                            if phase < 1742
                                            else ((phase[:4] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1754
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 11)
                                        if phase < 1755
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1768
                                    else (
                                        ((phase[:4] & 0) | 12)
                                        if phase < 1769
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1783
                                else (
                                    (
                                        ((phase[:4] & 0) | 13)
                                        if phase < 1784
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1799
                                    else (
                                        ((phase[:4] & 0) | 14)
                                        if phase < 1800
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1816
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 1817
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1837
                                    else (
                                        ((phase[:4] & 0) | 1)
                                        if phase < 1838
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1841
                                else (
                                    (
                                        ((phase[:4] & 0) | 2)
                                        if phase < 1842
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1846
                                    else (
                                        ((phase[:4] & 0) | 3)
                                        if phase < 1847
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 1852
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 4)
                                        if phase < 1853
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1859
                                    else (
                                        ((phase[:4] & 0) | 5)
                                        if phase < 1860
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1867
                                else (
                                    (
                                        ((phase[:4] & 0) | 6)
                                        if phase < 1868
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1876
                                    else (
                                        ((phase[:4] & 0) | 7)
                                        if phase < 1877
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 1886
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 8)
                                        if phase < 1887
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1897
                                    else (
                                        ((phase[:4] & 0) | 9)
                                        if phase < 1898
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 1909
                                else (
                                    (
                                        ((phase[:4] & 0) | 10)
                                        if phase < 1910
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 1922
                                    else (
                                        ((phase[:4] & 0) | 11)
                                        if phase < 1923
                                        else (
                                            ((phase[:4] & 0) | 0)
                                            if phase < 1936
                                            else ((phase[:4] & 0) | 12)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1937
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1951
                                        else ((phase[:4] & 0) | 13)
                                    )
                                    if phase < 1952
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1967
                                        else ((phase[:4] & 0) | 14)
                                    )
                                )
                                if phase < 1968
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 1984
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 1985
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2005
                                        else ((phase[:4] & 0) | 1)
                                    )
                                )
                            )
                            if phase < 2006
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2009
                                        else ((phase[:4] & 0) | 2)
                                    )
                                    if phase < 2010
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2014
                                        else ((phase[:4] & 0) | 3)
                                    )
                                )
                                if phase < 2015
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2020
                                        else ((phase[:4] & 0) | 4)
                                    )
                                    if phase < 2021
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2027
                                        else ((phase[:4] & 0) | 5)
                                    )
                                )
                            )
                        )
                        if phase < 2028
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2035
                                        else ((phase[:4] & 0) | 6)
                                    )
                                    if phase < 2036
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2044
                                        else ((phase[:4] & 0) | 7)
                                    )
                                )
                                if phase < 2045
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2054
                                        else ((phase[:4] & 0) | 8)
                                    )
                                    if phase < 2055
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2065
                                        else ((phase[:4] & 0) | 9)
                                    )
                                )
                            )
                            if phase < 2066
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2077
                                        else ((phase[:4] & 0) | 10)
                                    )
                                    if phase < 2078
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2090
                                        else ((phase[:4] & 0) | 11)
                                    )
                                )
                                if phase < 2091
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2104
                                        else ((phase[:4] & 0) | 12)
                                    )
                                    if phase < 2105
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2119
                                        else (
                                            ((phase[:4] & 0) | 13)
                                            if phase < 2120
                                            else ((phase[:4] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 2135
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 14)
                                        if phase < 2136
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2152
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2153
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 2173
                                else (
                                    (
                                        ((phase[:4] & 0) | 1)
                                        if phase < 2174
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2177
                                    else (
                                        ((phase[:4] & 0) | 2)
                                        if phase < 2178
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 2182
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 3)
                                        if phase < 2183
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2188
                                    else (
                                        ((phase[:4] & 0) | 4)
                                        if phase < 2189
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 2195
                                else (
                                    (
                                        ((phase[:4] & 0) | 5)
                                        if phase < 2196
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2203
                                    else (
                                        ((phase[:4] & 0) | 6)
                                        if phase < 2204
                                        else (
                                            ((phase[:4] & 0) | 0)
                                            if phase < 2212
                                            else ((phase[:4] & 0) | 7)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2213
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2222
                                        else ((phase[:4] & 0) | 8)
                                    )
                                    if phase < 2223
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2233
                                        else ((phase[:4] & 0) | 9)
                                    )
                                )
                                if phase < 2234
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2245
                                        else ((phase[:4] & 0) | 10)
                                    )
                                    if phase < 2246
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2258
                                        else ((phase[:4] & 0) | 11)
                                    )
                                )
                            )
                            if phase < 2259
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2272
                                        else ((phase[:4] & 0) | 12)
                                    )
                                    if phase < 2273
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2287
                                        else ((phase[:4] & 0) | 13)
                                    )
                                )
                                if phase < 2288
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2303
                                        else ((phase[:4] & 0) | 14)
                                    )
                                    if phase < 2304
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2320
                                        else (
                                            ((phase[:4] & 0) | 15)
                                            if phase < 2321
                                            else ((phase[:4] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 2341
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2342
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2362
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2363
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 2383
                                else (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2384
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2404
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2405
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 2425
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2426
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2446
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2447
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 2467
                                else (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2468
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2488
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2489
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                        )
                        if phase < 2509
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2510
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2530
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2531
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 2551
                                else (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2552
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2572
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2573
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 2593
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2594
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2614
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2615
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 2635
                                else (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2636
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2656
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2657
                                        else (
                                            ((phase[:4] & 0) | 0)
                                            if phase < 2677
                                            else ((phase[:4] & 0) | 15)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 2678
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2698
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 2699
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2719
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                                if phase < 2720
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2740
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 2741
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2761
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                            )
                            if phase < 2762
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2782
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 2783
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2803
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                                if phase < 2804
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2824
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 2825
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 2845
                                        else (
                                            ((phase[:4] & 0) | 15)
                                            if phase < 2846
                                            else ((phase[:4] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2866
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2867
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2887
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2888
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 2908
                                else (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2909
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2929
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2930
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 2950
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2951
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 2971
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2972
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 2992
                                else (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 2993
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 3013
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 3014
                                        else (
                                            ((phase[:4] & 0) | 0)
                                            if phase < 3034
                                            else ((phase[:4] & 0) | 15)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 3035
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3055
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3056
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3076
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                                if phase < 3077
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3097
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3098
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3118
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                            )
                            if phase < 3119
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3139
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3140
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3160
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                                if phase < 3161
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3181
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3182
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3202
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                            )
                        )
                        if phase < 3203
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3223
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3224
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3244
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                                if phase < 3245
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3265
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3266
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3286
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                            )
                            if phase < 3287
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3307
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3308
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3328
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                                if phase < 3329
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3349
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3350
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3370
                                        else (
                                            ((phase[:4] & 0) | 15)
                                            if phase < 3371
                                            else ((phase[:4] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 3391
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 3392
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 3412
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 3413
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 3433
                                else (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 3434
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 3454
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 3455
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                            )
                            if phase < 3475
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 3476
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 3496
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 3497
                                        else ((phase[:4] & 0) | 0)
                                    )
                                )
                                if phase < 3517
                                else (
                                    (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 3518
                                        else ((phase[:4] & 0) | 0)
                                    )
                                    if phase < 3538
                                    else (
                                        ((phase[:4] & 0) | 15)
                                        if phase < 3539
                                        else (
                                            ((phase[:4] & 0) | 0)
                                            if phase < 3559
                                            else ((phase[:4] & 0) | 15)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3560
                        else (
                            (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3580
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3581
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3601
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                                if phase < 3602
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3622
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3623
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3643
                                        else ((phase[:4] & 0) | 15)
                                    )
                                )
                            )
                            if phase < 3644
                            else (
                                (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3664
                                        else ((phase[:4] & 0) | 15)
                                    )
                                    if phase < 3665
                                    else (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3682
                                        else ((phase[:4] & 0) | 1)
                                    )
                                )
                                if phase < 3683
                                else (
                                    (
                                        ((phase[:4] & 0) | 0)
                                        if phase < 3688
                                        else ((phase[:4] & 0) | 3)
                                    )
                                    if phase < 3689
                                    else (
                                        ((phase[:4] & 0) | 1)
                                        if phase < 3690
                                        else (
                                            ((phase[:4] & 0) | 2)
                                            if phase < 3691
                                            else ((phase[:4] & 0) | 0)
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
            if phase < 169
            else (((phase[:1] & 0) | 0) if phase < 176 else ((phase[:1] & 0) | 1))
        )
        if phase < 247
        else (
            (((phase[:1] & 0) | 0) if phase < 251 else ((phase[:1] & 0) | 1))
            if phase < 3687
            else (((phase[:1] & 0) | 0) if phase < 3691 else ((phase[:1] & 0) | 1))
        )
    )
    payload = Item(value=value, remaining=remaining)
    dut = FeedbackPipeline(valid, payload, take)
    expected_ready = (
        (
            ((phase[:1] & 0) | 1)
            if phase < 173
            else (((phase[:1] & 0) | 0) if phase < 177 else ((phase[:1] & 0) | 1))
        )
        if phase < 251
        else (
            (((phase[:1] & 0) | 0) if phase < 253 else ((phase[:1] & 0) | 1))
            if phase < 3691
            else (((phase[:1] & 0) | 0) if phase < 3693 else ((phase[:1] & 0) | 1))
        )
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
                                if phase < 12
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 13
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 18
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 19
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 25
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 26
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 33
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 34
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 42
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 43
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 52
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 53
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 63
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 64
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 75
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 76
                                            else ((phase[:1] & 0) | 0)
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
                                        ((phase[:1] & 0) | 1)
                                        if phase < 89
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 102
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 103
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 117
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 118
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 133
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 134
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 150
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 151
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 168
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 169
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 171
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 178
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
                                if phase < 182
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 183
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 249
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 252
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 253
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 254
                                            else ((phase[:1] & 0) | 0)
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
                                        ((phase[:1] & 0) | 1)
                                        if phase < 256
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 258
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 259
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 324
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 325
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 328
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 329
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 333
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 334
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 339
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 340
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 346
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 347
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 354
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 355
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 363
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 364
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 373
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 374
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 384
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 385
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 396
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 397
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 409
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 410
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 423
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 424
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 438
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 439
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 454
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 455
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 471
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 472
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
                                        if phase < 492
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 493
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 496
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 497
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 501
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 502
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 507
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 508
                                            else ((phase[:1] & 0) | 0)
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
                                        ((phase[:1] & 0) | 1)
                                        if phase < 515
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 522
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 523
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 531
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 532
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 541
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 542
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 552
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 553
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 564
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 565
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 577
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 578
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
                                        if phase < 606
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 607
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 622
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 623
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 639
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 640
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 657
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 658
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 660
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 661
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 664
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 665
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 669
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 670
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 675
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 676
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 682
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 683
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 690
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 691
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 699
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 700
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 709
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 710
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 720
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 721
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 732
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 733
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 745
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 746
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 759
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 760
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 774
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 775
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 790
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 791
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 807
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 808
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 825
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 826
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 828
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 829
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 832
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 833
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 837
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 838
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 843
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 844
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 850
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 851
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 858
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 859
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 867
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 868
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 877
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 878
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 888
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 889
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 900
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 901
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 913
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 914
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 927
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 928
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 942
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 943
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 958
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 959
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 975
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 976
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 993
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 994
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 996
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 997
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1000
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1001
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1005
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1006
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1011
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1012
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1018
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 1019
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1026
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1027
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1035
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1036
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1045
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1046
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1056
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1057
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1068
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1069
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1081
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1082
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1095
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1096
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1110
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1111
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1126
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1127
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1143
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1144
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1161
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 1162
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1164
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1165
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1168
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1169
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1173
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1174
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1179
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1180
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1186
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1187
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1194
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1195
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1203
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1204
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1213
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1214
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1224
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1225
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1236
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1237
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1249
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 1250
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1263
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1264
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1278
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1279
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1294
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1295
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1311
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1312
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1329
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1330
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1332
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1333
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1336
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1337
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1341
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1342
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1347
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1348
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1354
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1355
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1362
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 1363
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1371
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1372
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1381
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1382
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1392
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1393
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1404
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1405
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1417
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1418
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1431
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1432
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1446
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1447
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1462
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1463
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1479
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1480
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1497
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1498
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1500
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1501
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1504
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1505
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1509
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1510
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1515
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1516
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1522
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1523
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1530
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1531
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1539
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1540
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1549
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1550
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1560
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1561
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1572
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1573
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1585
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1586
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1599
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1600
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1614
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1615
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1630
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1631
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1647
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1648
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1665
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1666
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1668
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1669
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1672
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1673
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1677
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1678
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1683
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1684
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1690
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1691
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1698
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1699
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1707
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1708
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1717
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1718
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1728
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1729
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1740
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1741
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1753
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1754
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1767
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1768
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1782
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1783
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1798
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1799
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1815
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1816
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1833
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 1834
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1836
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1837
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1840
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1841
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1845
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1846
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1851
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 1852
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1858
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1859
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 1866
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1867
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1875
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
        if phase < 1876
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1885
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1886
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1896
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1897
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1908
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1909
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 1921
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1922
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 1935
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 1936
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1950
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 1951
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 1966
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 1967
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1983
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 1984
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2001
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2002
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2004
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2005
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2008
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2009
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2013
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2014
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2019
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2020
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2026
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2027
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2034
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2035
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2043
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 2044
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2053
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2054
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2064
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2065
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2076
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2077
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2089
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2090
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2103
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2104
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 2118
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2119
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2134
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2135
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2151
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2152
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2169
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2170
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2172
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2173
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2176
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 2177
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2181
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2182
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2187
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2188
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2194
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2195
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2202
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2203
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2211
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2212
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2221
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2222
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2232
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2233
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2244
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2245
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2257
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2258
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2271
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2272
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2286
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 2287
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2302
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2303
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2319
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2320
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2337
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2338
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2340
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2341
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2358
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2359
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 2361
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2362
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2379
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2380
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2382
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2383
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2400
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2401
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2403
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2404
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2421
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2422
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2424
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2425
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2442
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2443
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2445
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2446
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2463
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2464
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2466
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2467
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2484
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2485
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2487
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2488
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2505
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2506
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2508
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2509
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2526
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2527
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2529
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2530
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2547
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2548
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2550
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2551
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2568
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2569
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2571
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2572
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2589
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 2590
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2592
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2593
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2610
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2611
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2613
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2614
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2631
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2632
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2634
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2635
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2652
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2653
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2655
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2656
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2673
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2674
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2676
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2677
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2694
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2695
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2697
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2698
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2715
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2716
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2718
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2719
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2736
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2737
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2739
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2740
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2757
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2758
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2760
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2761
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2778
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2779
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2781
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2782
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2799
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2800
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2802
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2803
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2820
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 2821
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2823
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2824
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2841
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2842
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2844
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2845
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2862
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2863
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2865
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 2866
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2883
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2884
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2886
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2887
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2904
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2905
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2907
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 2908
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2925
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2926
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2928
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2929
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 2946
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2947
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2949
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 2950
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2967
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 2968
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 2970
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 2971
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 2988
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 2989
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 2991
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 2992
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3009
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3010
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3012
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3013
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3030
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3031
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3033
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3034
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 3051
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3052
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3054
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3055
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3072
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3073
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3075
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3076
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3093
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3094
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3096
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 3097
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3114
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3115
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3117
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3118
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3135
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3136
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3138
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3139
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3156
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3157
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3159
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3160
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3177
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3178
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3180
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3181
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3198
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3199
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3201
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3202
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3219
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 3220
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3222
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3223
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3240
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3241
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3243
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3244
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3261
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3262
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3264
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3265
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 3282
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3283
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3285
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3286
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3303
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3304
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3306
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3307
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3324
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3325
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3327
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 3328
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3345
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3346
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3348
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3349
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3366
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3367
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3369
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3370
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3387
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3388
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3390
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3391
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3408
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3409
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3411
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3412
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3429
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3430
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3432
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3433
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3450
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 3451
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3453
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3454
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3471
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3472
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3474
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3475
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3492
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3493
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3495
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3496
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 3513
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3514
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3516
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3517
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3534
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3535
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3537
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3538
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3555
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3556
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3558
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 3559
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3576
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3577
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3579
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3580
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3597
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3598
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3600
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3601
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3618
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3619
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3621
                        else (
                            (
                                (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3622
                                        else ((phase[:1] & 0) | 0)
                                    )
                                    if phase < 3639
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3640
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3642
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                                if phase < 3643
                                else (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3660
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3661
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                    if phase < 3663
                                    else (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3664
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3681
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                )
                            )
                            if phase < 3682
                            else (
                                (
                                    (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3685
                                        else ((phase[:1] & 0) | 1)
                                    )
                                    if phase < 3686
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3689
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3692
                                            else ((phase[:1] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3693
                                else (
                                    (
                                        ((phase[:1] & 0) | 1)
                                        if phase < 3694
                                        else (
                                            ((phase[:1] & 0) | 0)
                                            if phase < 3695
                                            else ((phase[:1] & 0) | 1)
                                        )
                                    )
                                    if phase < 3696
                                    else (
                                        ((phase[:1] & 0) | 0)
                                        if phase < 3698
                                        else (
                                            ((phase[:1] & 0) | 1)
                                            if phase < 3699
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
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3
                                        else ((phase[:32] & 0) | 4294967295)
                                    )
                                    if phase < 4
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 7
                                        else (
                                            ((phase[:32] & 0) | 4294967295)
                                            if phase < 8
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 12
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 13
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 18
                                    else (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 19
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 25
                                            else ((phase[:32] & 0) | 4294967295)
                                        )
                                    )
                                )
                            )
                            if phase < 26
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 33
                                        else ((phase[:32] & 0) | 4294967295)
                                    )
                                    if phase < 34
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 42
                                        else (
                                            ((phase[:32] & 0) | 4294967295)
                                            if phase < 43
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 52
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 53
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 63
                                            else ((phase[:32] & 0) | 4294967295)
                                        )
                                    )
                                    if phase < 64
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 75
                                        else (
                                            ((phase[:32] & 0) | 4294967295)
                                            if phase < 76
                                            else ((phase[:32] & 0) | 0)
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
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 89
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 102
                                    else (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 103
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 117
                                            else ((phase[:32] & 0) | 4294967295)
                                        )
                                    )
                                )
                                if phase < 118
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 133
                                        else (
                                            ((phase[:32] & 0) | 4294967295)
                                            if phase < 134
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 150
                                    else (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 151
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 168
                                            else ((phase[:32] & 0) | 4294967295)
                                        )
                                    )
                                )
                            )
                            if phase < 169
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 171
                                        else ((phase[:32] & 0) | 10)
                                    )
                                    if phase < 177
                                    else (
                                        ((phase[:32] & 0) | 23)
                                        if phase < 178
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 179
                                            else ((phase[:32] & 0) | 31)
                                        )
                                    )
                                )
                                if phase < 180
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 182
                                        else (
                                            ((phase[:32] & 0) | 42)
                                            if phase < 183
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 249
                                    else (
                                        ((phase[:32] & 0) | 10)
                                        if phase < 252
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 253
                                            else ((phase[:32] & 0) | 23)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 254
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 255
                                        else ((phase[:32] & 0) | 31)
                                    )
                                    if phase < 256
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 258
                                        else (
                                            ((phase[:32] & 0) | 42)
                                            if phase < 259
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 328
                                else (
                                    (
                                        ((phase[:32] & 0) | 1)
                                        if phase < 329
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 333
                                    else (
                                        ((phase[:32] & 0) | 2)
                                        if phase < 334
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 339
                                            else ((phase[:32] & 0) | 3)
                                        )
                                    )
                                )
                            )
                            if phase < 340
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 346
                                        else ((phase[:32] & 0) | 4)
                                    )
                                    if phase < 347
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 354
                                        else (
                                            ((phase[:32] & 0) | 5)
                                            if phase < 355
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 363
                                else (
                                    (
                                        ((phase[:32] & 0) | 6)
                                        if phase < 364
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 373
                                            else ((phase[:32] & 0) | 7)
                                        )
                                    )
                                    if phase < 374
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 384
                                        else (
                                            ((phase[:32] & 0) | 8)
                                            if phase < 385
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 396
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 9)
                                        if phase < 397
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 409
                                    else (
                                        ((phase[:32] & 0) | 10)
                                        if phase < 410
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 423
                                            else ((phase[:32] & 0) | 11)
                                        )
                                    )
                                )
                                if phase < 424
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 438
                                        else (
                                            ((phase[:32] & 0) | 12)
                                            if phase < 439
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 454
                                    else (
                                        ((phase[:32] & 0) | 13)
                                        if phase < 455
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 471
                                            else ((phase[:32] & 0) | 14)
                                        )
                                    )
                                )
                            )
                            if phase < 472
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 489
                                        else ((phase[:32] & 0) | 15)
                                    )
                                    if phase < 490
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 492
                                        else (
                                            ((phase[:32] & 0) | 1)
                                            if phase < 493
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 496
                                else (
                                    (
                                        ((phase[:32] & 0) | 2)
                                        if phase < 497
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 501
                                            else ((phase[:32] & 0) | 3)
                                        )
                                    )
                                    if phase < 502
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 507
                                        else (
                                            ((phase[:32] & 0) | 4)
                                            if phase < 508
                                            else ((phase[:32] & 0) | 0)
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
                                        ((phase[:32] & 0) | 5)
                                        if phase < 515
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 522
                                    else (
                                        ((phase[:32] & 0) | 6)
                                        if phase < 523
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 531
                                            else ((phase[:32] & 0) | 7)
                                        )
                                    )
                                )
                                if phase < 532
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 541
                                        else ((phase[:32] & 0) | 8)
                                    )
                                    if phase < 542
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 552
                                        else (
                                            ((phase[:32] & 0) | 9)
                                            if phase < 553
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 564
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 10)
                                        if phase < 565
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 577
                                    else (
                                        ((phase[:32] & 0) | 11)
                                        if phase < 578
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 591
                                            else ((phase[:32] & 0) | 12)
                                        )
                                    )
                                )
                                if phase < 592
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 606
                                        else (
                                            ((phase[:32] & 0) | 13)
                                            if phase < 607
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 622
                                    else (
                                        ((phase[:32] & 0) | 14)
                                        if phase < 623
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 639
                                            else ((phase[:32] & 0) | 15)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 640
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 657
                                        else ((phase[:32] & 0) | 16)
                                    )
                                    if phase < 658
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 660
                                        else (
                                            ((phase[:32] & 0) | 15)
                                            if phase < 661
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 664
                                else (
                                    (
                                        ((phase[:32] & 0) | 16)
                                        if phase < 665
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 669
                                            else ((phase[:32] & 0) | 17)
                                        )
                                    )
                                    if phase < 670
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 675
                                        else (
                                            ((phase[:32] & 0) | 18)
                                            if phase < 676
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 682
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 19)
                                        if phase < 683
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 690
                                    else (
                                        ((phase[:32] & 0) | 20)
                                        if phase < 691
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 699
                                            else ((phase[:32] & 0) | 21)
                                        )
                                    )
                                )
                                if phase < 700
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 709
                                        else (
                                            ((phase[:32] & 0) | 22)
                                            if phase < 710
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 720
                                    else (
                                        ((phase[:32] & 0) | 23)
                                        if phase < 721
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 732
                                            else ((phase[:32] & 0) | 24)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 733
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 745
                                        else ((phase[:32] & 0) | 25)
                                    )
                                    if phase < 746
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 759
                                        else (
                                            ((phase[:32] & 0) | 26)
                                            if phase < 760
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 774
                                else (
                                    (
                                        ((phase[:32] & 0) | 27)
                                        if phase < 775
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 790
                                    else (
                                        ((phase[:32] & 0) | 28)
                                        if phase < 791
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 807
                                            else ((phase[:32] & 0) | 29)
                                        )
                                    )
                                )
                            )
                            if phase < 808
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 825
                                        else ((phase[:32] & 0) | 30)
                                    )
                                    if phase < 826
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 828
                                        else (
                                            ((phase[:32] & 0) | 16)
                                            if phase < 829
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 832
                                else (
                                    (
                                        ((phase[:32] & 0) | 17)
                                        if phase < 833
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 837
                                            else ((phase[:32] & 0) | 18)
                                        )
                                    )
                                    if phase < 838
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 843
                                        else (
                                            ((phase[:32] & 0) | 19)
                                            if phase < 844
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 850
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 20)
                                        if phase < 851
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 858
                                    else (
                                        ((phase[:32] & 0) | 21)
                                        if phase < 859
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 867
                                            else ((phase[:32] & 0) | 22)
                                        )
                                    )
                                )
                                if phase < 868
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 877
                                        else (
                                            ((phase[:32] & 0) | 23)
                                            if phase < 878
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 888
                                    else (
                                        ((phase[:32] & 0) | 24)
                                        if phase < 889
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 900
                                            else ((phase[:32] & 0) | 25)
                                        )
                                    )
                                )
                            )
                            if phase < 901
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 913
                                        else ((phase[:32] & 0) | 26)
                                    )
                                    if phase < 914
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 927
                                        else (
                                            ((phase[:32] & 0) | 27)
                                            if phase < 928
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 942
                                else (
                                    (
                                        ((phase[:32] & 0) | 28)
                                        if phase < 943
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 958
                                            else ((phase[:32] & 0) | 29)
                                        )
                                    )
                                    if phase < 959
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 975
                                        else (
                                            ((phase[:32] & 0) | 30)
                                            if phase < 976
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 993
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 31)
                                        if phase < 994
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 996
                                    else (
                                        ((phase[:32] & 0) | 255)
                                        if phase < 997
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1000
                                            else ((phase[:32] & 0) | 256)
                                        )
                                    )
                                )
                                if phase < 1001
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1005
                                        else ((phase[:32] & 0) | 257)
                                    )
                                    if phase < 1006
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1011
                                        else (
                                            ((phase[:32] & 0) | 258)
                                            if phase < 1012
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1018
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 259)
                                        if phase < 1019
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1026
                                    else (
                                        ((phase[:32] & 0) | 260)
                                        if phase < 1027
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1035
                                            else ((phase[:32] & 0) | 261)
                                        )
                                    )
                                )
                                if phase < 1036
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1045
                                        else (
                                            ((phase[:32] & 0) | 262)
                                            if phase < 1046
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 1056
                                    else (
                                        ((phase[:32] & 0) | 263)
                                        if phase < 1057
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1068
                                            else ((phase[:32] & 0) | 264)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1069
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1081
                                        else ((phase[:32] & 0) | 265)
                                    )
                                    if phase < 1082
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1095
                                        else (
                                            ((phase[:32] & 0) | 266)
                                            if phase < 1096
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1110
                                else (
                                    (
                                        ((phase[:32] & 0) | 267)
                                        if phase < 1111
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1126
                                            else ((phase[:32] & 0) | 268)
                                        )
                                    )
                                    if phase < 1127
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1143
                                        else (
                                            ((phase[:32] & 0) | 269)
                                            if phase < 1144
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1161
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 270)
                                        if phase < 1162
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1164
                                    else (
                                        ((phase[:32] & 0) | 256)
                                        if phase < 1165
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1168
                                            else ((phase[:32] & 0) | 257)
                                        )
                                    )
                                )
                                if phase < 1169
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1173
                                        else (
                                            ((phase[:32] & 0) | 258)
                                            if phase < 1174
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 1179
                                    else (
                                        ((phase[:32] & 0) | 259)
                                        if phase < 1180
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1186
                                            else ((phase[:32] & 0) | 260)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1187
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1194
                                        else ((phase[:32] & 0) | 261)
                                    )
                                    if phase < 1195
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1203
                                        else (
                                            ((phase[:32] & 0) | 262)
                                            if phase < 1204
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1213
                                else (
                                    (
                                        ((phase[:32] & 0) | 263)
                                        if phase < 1214
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1224
                                    else (
                                        ((phase[:32] & 0) | 264)
                                        if phase < 1225
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1236
                                            else ((phase[:32] & 0) | 265)
                                        )
                                    )
                                )
                            )
                            if phase < 1237
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1249
                                        else ((phase[:32] & 0) | 266)
                                    )
                                    if phase < 1250
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1263
                                        else (
                                            ((phase[:32] & 0) | 267)
                                            if phase < 1264
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1278
                                else (
                                    (
                                        ((phase[:32] & 0) | 268)
                                        if phase < 1279
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1294
                                            else ((phase[:32] & 0) | 269)
                                        )
                                    )
                                    if phase < 1295
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1311
                                        else (
                                            ((phase[:32] & 0) | 270)
                                            if phase < 1312
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1329
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 271)
                                        if phase < 1330
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1332
                                    else (
                                        ((phase[:32] & 0) | 2147483647)
                                        if phase < 1333
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1336
                                            else ((phase[:32] & 0) | 2147483648)
                                        )
                                    )
                                )
                                if phase < 1337
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1341
                                        else (
                                            ((phase[:32] & 0) | 2147483649)
                                            if phase < 1342
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 1347
                                    else (
                                        ((phase[:32] & 0) | 2147483650)
                                        if phase < 1348
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1354
                                            else ((phase[:32] & 0) | 2147483651)
                                        )
                                    )
                                )
                            )
                            if phase < 1355
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1362
                                        else ((phase[:32] & 0) | 2147483652)
                                    )
                                    if phase < 1363
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1371
                                        else (
                                            ((phase[:32] & 0) | 2147483653)
                                            if phase < 1372
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1381
                                else (
                                    (
                                        ((phase[:32] & 0) | 2147483654)
                                        if phase < 1382
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1392
                                            else ((phase[:32] & 0) | 2147483655)
                                        )
                                    )
                                    if phase < 1393
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1404
                                        else (
                                            ((phase[:32] & 0) | 2147483656)
                                            if phase < 1405
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 1417
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 2147483657)
                                        if phase < 1418
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1431
                                    else (
                                        ((phase[:32] & 0) | 2147483658)
                                        if phase < 1432
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1446
                                            else ((phase[:32] & 0) | 2147483659)
                                        )
                                    )
                                )
                                if phase < 1447
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1462
                                        else ((phase[:32] & 0) | 2147483660)
                                    )
                                    if phase < 1463
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1479
                                        else (
                                            ((phase[:32] & 0) | 2147483661)
                                            if phase < 1480
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1497
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 2147483662)
                                        if phase < 1498
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1500
                                    else (
                                        ((phase[:32] & 0) | 2147483648)
                                        if phase < 1501
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1504
                                            else ((phase[:32] & 0) | 2147483649)
                                        )
                                    )
                                )
                                if phase < 1505
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1509
                                        else (
                                            ((phase[:32] & 0) | 2147483650)
                                            if phase < 1510
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 1515
                                    else (
                                        ((phase[:32] & 0) | 2147483651)
                                        if phase < 1516
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1522
                                            else ((phase[:32] & 0) | 2147483652)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1523
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1530
                                        else ((phase[:32] & 0) | 2147483653)
                                    )
                                    if phase < 1531
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1539
                                        else (
                                            ((phase[:32] & 0) | 2147483654)
                                            if phase < 1540
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1549
                                else (
                                    (
                                        ((phase[:32] & 0) | 2147483655)
                                        if phase < 1550
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1560
                                            else ((phase[:32] & 0) | 2147483656)
                                        )
                                    )
                                    if phase < 1561
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1572
                                        else (
                                            ((phase[:32] & 0) | 2147483657)
                                            if phase < 1573
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1585
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 2147483658)
                                        if phase < 1586
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1599
                                    else (
                                        ((phase[:32] & 0) | 2147483659)
                                        if phase < 1600
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1614
                                            else ((phase[:32] & 0) | 2147483660)
                                        )
                                    )
                                )
                                if phase < 1615
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1630
                                        else (
                                            ((phase[:32] & 0) | 2147483661)
                                            if phase < 1631
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 1647
                                    else (
                                        ((phase[:32] & 0) | 2147483662)
                                        if phase < 1648
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1665
                                            else ((phase[:32] & 0) | 2147483663)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 1666
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1668
                                        else ((phase[:32] & 0) | 4294967295)
                                    )
                                    if phase < 1669
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1677
                                        else (
                                            ((phase[:32] & 0) | 1)
                                            if phase < 1678
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1683
                                else (
                                    (
                                        ((phase[:32] & 0) | 2)
                                        if phase < 1684
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1690
                                            else ((phase[:32] & 0) | 3)
                                        )
                                    )
                                    if phase < 1691
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1698
                                        else (
                                            ((phase[:32] & 0) | 4)
                                            if phase < 1699
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1707
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 5)
                                        if phase < 1708
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1717
                                    else (
                                        ((phase[:32] & 0) | 6)
                                        if phase < 1718
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1728
                                            else ((phase[:32] & 0) | 7)
                                        )
                                    )
                                )
                                if phase < 1729
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1740
                                        else (
                                            ((phase[:32] & 0) | 8)
                                            if phase < 1741
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 1753
                                    else (
                                        ((phase[:32] & 0) | 9)
                                        if phase < 1754
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1767
                                            else ((phase[:32] & 0) | 10)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 1768
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1782
                                        else ((phase[:32] & 0) | 11)
                                    )
                                    if phase < 1783
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1798
                                        else (
                                            ((phase[:32] & 0) | 12)
                                            if phase < 1799
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1815
                                else (
                                    (
                                        ((phase[:32] & 0) | 13)
                                        if phase < 1816
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1833
                                            else ((phase[:32] & 0) | 14)
                                        )
                                    )
                                    if phase < 1834
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1836
                                        else (
                                            ((phase[:32] & 0) | 4294967294)
                                            if phase < 1837
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 1840
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 4294967295)
                                        if phase < 1841
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1851
                                    else (
                                        ((phase[:32] & 0) | 1)
                                        if phase < 1852
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1858
                                            else ((phase[:32] & 0) | 2)
                                        )
                                    )
                                )
                                if phase < 1859
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1866
                                        else (
                                            ((phase[:32] & 0) | 3)
                                            if phase < 1867
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 1875
                                    else (
                                        ((phase[:32] & 0) | 4)
                                        if phase < 1876
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1885
                                            else ((phase[:32] & 0) | 5)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
        )
        if phase < 1886
        else (
            (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1896
                                        else ((phase[:32] & 0) | 6)
                                    )
                                    if phase < 1897
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1908
                                        else (
                                            ((phase[:32] & 0) | 7)
                                            if phase < 1909
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 1921
                                else (
                                    (
                                        ((phase[:32] & 0) | 8)
                                        if phase < 1922
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 1935
                                    else (
                                        ((phase[:32] & 0) | 9)
                                        if phase < 1936
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 1950
                                            else ((phase[:32] & 0) | 10)
                                        )
                                    )
                                )
                            )
                            if phase < 1951
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1966
                                        else ((phase[:32] & 0) | 11)
                                    )
                                    if phase < 1967
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 1983
                                        else (
                                            ((phase[:32] & 0) | 12)
                                            if phase < 1984
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2001
                                else (
                                    (
                                        ((phase[:32] & 0) | 13)
                                        if phase < 2002
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2004
                                            else ((phase[:32] & 0) | 1431655765)
                                        )
                                    )
                                    if phase < 2005
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2008
                                        else (
                                            ((phase[:32] & 0) | 1431655766)
                                            if phase < 2009
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2013
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 1431655767)
                                        if phase < 2014
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2019
                                    else (
                                        ((phase[:32] & 0) | 1431655768)
                                        if phase < 2020
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2026
                                            else ((phase[:32] & 0) | 1431655769)
                                        )
                                    )
                                )
                                if phase < 2027
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2034
                                        else (
                                            ((phase[:32] & 0) | 1431655770)
                                            if phase < 2035
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 2043
                                    else (
                                        ((phase[:32] & 0) | 1431655771)
                                        if phase < 2044
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2053
                                            else ((phase[:32] & 0) | 1431655772)
                                        )
                                    )
                                )
                            )
                            if phase < 2054
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2064
                                        else ((phase[:32] & 0) | 1431655773)
                                    )
                                    if phase < 2065
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2076
                                        else (
                                            ((phase[:32] & 0) | 1431655774)
                                            if phase < 2077
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2089
                                else (
                                    (
                                        ((phase[:32] & 0) | 1431655775)
                                        if phase < 2090
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2103
                                            else ((phase[:32] & 0) | 1431655776)
                                        )
                                    )
                                    if phase < 2104
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2118
                                        else (
                                            ((phase[:32] & 0) | 1431655777)
                                            if phase < 2119
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 2134
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 1431655778)
                                        if phase < 2135
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2151
                                    else (
                                        ((phase[:32] & 0) | 1431655779)
                                        if phase < 2152
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2169
                                            else ((phase[:32] & 0) | 1431655780)
                                        )
                                    )
                                )
                                if phase < 2170
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2172
                                        else ((phase[:32] & 0) | 2863311530)
                                    )
                                    if phase < 2173
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2176
                                        else (
                                            ((phase[:32] & 0) | 2863311531)
                                            if phase < 2177
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2181
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 2863311532)
                                        if phase < 2182
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2187
                                    else (
                                        ((phase[:32] & 0) | 2863311533)
                                        if phase < 2188
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2194
                                            else ((phase[:32] & 0) | 2863311534)
                                        )
                                    )
                                )
                                if phase < 2195
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2202
                                        else (
                                            ((phase[:32] & 0) | 2863311535)
                                            if phase < 2203
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 2211
                                    else (
                                        ((phase[:32] & 0) | 2863311536)
                                        if phase < 2212
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2221
                                            else ((phase[:32] & 0) | 2863311537)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2222
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2232
                                        else ((phase[:32] & 0) | 2863311538)
                                    )
                                    if phase < 2233
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2244
                                        else (
                                            ((phase[:32] & 0) | 2863311539)
                                            if phase < 2245
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2257
                                else (
                                    (
                                        ((phase[:32] & 0) | 2863311540)
                                        if phase < 2258
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2271
                                            else ((phase[:32] & 0) | 2863311541)
                                        )
                                    )
                                    if phase < 2272
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2286
                                        else (
                                            ((phase[:32] & 0) | 2863311542)
                                            if phase < 2287
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2302
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 2863311543)
                                        if phase < 2303
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2319
                                    else (
                                        ((phase[:32] & 0) | 2863311544)
                                        if phase < 2320
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2337
                                            else ((phase[:32] & 0) | 2863311545)
                                        )
                                    )
                                )
                                if phase < 2338
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2340
                                        else (
                                            ((phase[:32] & 0) | 1)
                                            if phase < 2341
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 2358
                                    else (
                                        ((phase[:32] & 0) | 16)
                                        if phase < 2359
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2361
                                            else ((phase[:32] & 0) | 4294967294)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 2362
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2379
                                        else ((phase[:32] & 0) | 13)
                                    )
                                    if phase < 2380
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2382
                                        else (
                                            ((phase[:32] & 0) | 2)
                                            if phase < 2383
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2400
                                else (
                                    (
                                        ((phase[:32] & 0) | 17)
                                        if phase < 2401
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2403
                                    else (
                                        ((phase[:32] & 0) | 4294967293)
                                        if phase < 2404
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2421
                                            else ((phase[:32] & 0) | 12)
                                        )
                                    )
                                )
                            )
                            if phase < 2422
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2424
                                        else ((phase[:32] & 0) | 4)
                                    )
                                    if phase < 2425
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2442
                                        else (
                                            ((phase[:32] & 0) | 19)
                                            if phase < 2443
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2445
                                else (
                                    (
                                        ((phase[:32] & 0) | 4294967291)
                                        if phase < 2446
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2463
                                            else ((phase[:32] & 0) | 10)
                                        )
                                    )
                                    if phase < 2464
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2466
                                        else (
                                            ((phase[:32] & 0) | 8)
                                            if phase < 2467
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2484
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 23)
                                        if phase < 2485
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2487
                                    else (
                                        ((phase[:32] & 0) | 4294967287)
                                        if phase < 2488
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2505
                                            else ((phase[:32] & 0) | 6)
                                        )
                                    )
                                )
                                if phase < 2506
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2508
                                        else (
                                            ((phase[:32] & 0) | 16)
                                            if phase < 2509
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 2526
                                    else (
                                        ((phase[:32] & 0) | 31)
                                        if phase < 2527
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2529
                                            else ((phase[:32] & 0) | 4294967279)
                                        )
                                    )
                                )
                            )
                            if phase < 2530
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2547
                                        else ((phase[:32] & 0) | 4294967294)
                                    )
                                    if phase < 2548
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2550
                                        else (
                                            ((phase[:32] & 0) | 32)
                                            if phase < 2551
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2568
                                else (
                                    (
                                        ((phase[:32] & 0) | 47)
                                        if phase < 2569
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2571
                                            else ((phase[:32] & 0) | 4294967263)
                                        )
                                    )
                                    if phase < 2572
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2589
                                        else (
                                            ((phase[:32] & 0) | 4294967278)
                                            if phase < 2590
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 2592
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 64)
                                        if phase < 2593
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2610
                                    else (
                                        ((phase[:32] & 0) | 79)
                                        if phase < 2611
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2613
                                            else ((phase[:32] & 0) | 4294967231)
                                        )
                                    )
                                )
                                if phase < 2614
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2631
                                        else ((phase[:32] & 0) | 4294967246)
                                    )
                                    if phase < 2632
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2634
                                        else (
                                            ((phase[:32] & 0) | 128)
                                            if phase < 2635
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2652
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 143)
                                        if phase < 2653
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2655
                                    else (
                                        ((phase[:32] & 0) | 4294967167)
                                        if phase < 2656
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2673
                                            else ((phase[:32] & 0) | 4294967182)
                                        )
                                    )
                                )
                                if phase < 2674
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2676
                                        else (
                                            ((phase[:32] & 0) | 256)
                                            if phase < 2677
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 2694
                                    else (
                                        ((phase[:32] & 0) | 271)
                                        if phase < 2695
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2697
                                            else ((phase[:32] & 0) | 4294967039)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2698
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2715
                                        else ((phase[:32] & 0) | 4294967054)
                                    )
                                    if phase < 2716
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2718
                                        else (
                                            ((phase[:32] & 0) | 512)
                                            if phase < 2719
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2736
                                else (
                                    (
                                        ((phase[:32] & 0) | 527)
                                        if phase < 2737
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2739
                                            else ((phase[:32] & 0) | 4294966783)
                                        )
                                    )
                                    if phase < 2740
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2757
                                        else (
                                            ((phase[:32] & 0) | 4294966798)
                                            if phase < 2758
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 2760
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 1024)
                                        if phase < 2761
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2778
                                    else (
                                        ((phase[:32] & 0) | 1039)
                                        if phase < 2779
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2781
                                            else ((phase[:32] & 0) | 4294966271)
                                        )
                                    )
                                )
                                if phase < 2782
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2799
                                        else (
                                            ((phase[:32] & 0) | 4294966286)
                                            if phase < 2800
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 2802
                                    else (
                                        ((phase[:32] & 0) | 2048)
                                        if phase < 2803
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2820
                                            else ((phase[:32] & 0) | 2063)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            )
            if phase < 2821
            else (
                (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2823
                                        else ((phase[:32] & 0) | 4294965247)
                                    )
                                    if phase < 2824
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2841
                                        else (
                                            ((phase[:32] & 0) | 4294965262)
                                            if phase < 2842
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2844
                                else (
                                    (
                                        ((phase[:32] & 0) | 4096)
                                        if phase < 2845
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2862
                                    else (
                                        ((phase[:32] & 0) | 4111)
                                        if phase < 2863
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2865
                                            else ((phase[:32] & 0) | 4294963199)
                                        )
                                    )
                                )
                            )
                            if phase < 2866
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2883
                                        else ((phase[:32] & 0) | 4294963214)
                                    )
                                    if phase < 2884
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2886
                                        else (
                                            ((phase[:32] & 0) | 8192)
                                            if phase < 2887
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 2904
                                else (
                                    (
                                        ((phase[:32] & 0) | 8207)
                                        if phase < 2905
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2907
                                            else ((phase[:32] & 0) | 4294959103)
                                        )
                                    )
                                    if phase < 2908
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2925
                                        else (
                                            ((phase[:32] & 0) | 4294959118)
                                            if phase < 2926
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 2928
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 16384)
                                        if phase < 2929
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 2946
                                    else (
                                        ((phase[:32] & 0) | 16399)
                                        if phase < 2947
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2949
                                            else ((phase[:32] & 0) | 4294950911)
                                        )
                                    )
                                )
                                if phase < 2950
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2967
                                        else (
                                            ((phase[:32] & 0) | 4294950926)
                                            if phase < 2968
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 2970
                                    else (
                                        ((phase[:32] & 0) | 32768)
                                        if phase < 2971
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 2988
                                            else ((phase[:32] & 0) | 32783)
                                        )
                                    )
                                )
                            )
                            if phase < 2989
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 2991
                                        else ((phase[:32] & 0) | 4294934527)
                                    )
                                    if phase < 2992
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3009
                                        else (
                                            ((phase[:32] & 0) | 4294934542)
                                            if phase < 3010
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3012
                                else (
                                    (
                                        ((phase[:32] & 0) | 65536)
                                        if phase < 3013
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3030
                                            else ((phase[:32] & 0) | 65551)
                                        )
                                    )
                                    if phase < 3031
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3033
                                        else (
                                            ((phase[:32] & 0) | 4294901759)
                                            if phase < 3034
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 3051
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 4294901774)
                                        if phase < 3052
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3054
                                    else (
                                        ((phase[:32] & 0) | 131072)
                                        if phase < 3055
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3072
                                            else ((phase[:32] & 0) | 131087)
                                        )
                                    )
                                )
                                if phase < 3073
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3075
                                        else ((phase[:32] & 0) | 4294836223)
                                    )
                                    if phase < 3076
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3093
                                        else (
                                            ((phase[:32] & 0) | 4294836238)
                                            if phase < 3094
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 3096
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 262144)
                                        if phase < 3097
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3114
                                    else (
                                        ((phase[:32] & 0) | 262159)
                                        if phase < 3115
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3117
                                            else ((phase[:32] & 0) | 4294705151)
                                        )
                                    )
                                )
                                if phase < 3118
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3135
                                        else (
                                            ((phase[:32] & 0) | 4294705166)
                                            if phase < 3136
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 3138
                                    else (
                                        ((phase[:32] & 0) | 524288)
                                        if phase < 3139
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3156
                                            else ((phase[:32] & 0) | 524303)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3157
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3159
                                        else ((phase[:32] & 0) | 4294443007)
                                    )
                                    if phase < 3160
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3177
                                        else (
                                            ((phase[:32] & 0) | 4294443022)
                                            if phase < 3178
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3180
                                else (
                                    (
                                        ((phase[:32] & 0) | 1048576)
                                        if phase < 3181
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3198
                                            else ((phase[:32] & 0) | 1048591)
                                        )
                                    )
                                    if phase < 3199
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3201
                                        else (
                                            ((phase[:32] & 0) | 4293918719)
                                            if phase < 3202
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                            if phase < 3219
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 4293918734)
                                        if phase < 3220
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3222
                                    else (
                                        ((phase[:32] & 0) | 2097152)
                                        if phase < 3223
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3240
                                            else ((phase[:32] & 0) | 2097167)
                                        )
                                    )
                                )
                                if phase < 3241
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3243
                                        else (
                                            ((phase[:32] & 0) | 4292870143)
                                            if phase < 3244
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 3261
                                    else (
                                        ((phase[:32] & 0) | 4292870158)
                                        if phase < 3262
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3264
                                            else ((phase[:32] & 0) | 4194304)
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                if phase < 3265
                else (
                    (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3282
                                        else ((phase[:32] & 0) | 4194319)
                                    )
                                    if phase < 3283
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3285
                                        else (
                                            ((phase[:32] & 0) | 4290772991)
                                            if phase < 3286
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3303
                                else (
                                    (
                                        ((phase[:32] & 0) | 4290773006)
                                        if phase < 3304
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3306
                                    else (
                                        ((phase[:32] & 0) | 8388608)
                                        if phase < 3307
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3324
                                            else ((phase[:32] & 0) | 8388623)
                                        )
                                    )
                                )
                            )
                            if phase < 3325
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3327
                                        else ((phase[:32] & 0) | 4286578687)
                                    )
                                    if phase < 3328
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3345
                                        else (
                                            ((phase[:32] & 0) | 4286578702)
                                            if phase < 3346
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3348
                                else (
                                    (
                                        ((phase[:32] & 0) | 16777216)
                                        if phase < 3349
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3366
                                            else ((phase[:32] & 0) | 16777231)
                                        )
                                    )
                                    if phase < 3367
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3369
                                        else (
                                            ((phase[:32] & 0) | 4278190079)
                                            if phase < 3370
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3387
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 4278190094)
                                        if phase < 3388
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3390
                                    else (
                                        ((phase[:32] & 0) | 33554432)
                                        if phase < 3391
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3408
                                            else ((phase[:32] & 0) | 33554447)
                                        )
                                    )
                                )
                                if phase < 3409
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3411
                                        else (
                                            ((phase[:32] & 0) | 4261412863)
                                            if phase < 3412
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 3429
                                    else (
                                        ((phase[:32] & 0) | 4261412878)
                                        if phase < 3430
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3432
                                            else ((phase[:32] & 0) | 67108864)
                                        )
                                    )
                                )
                            )
                            if phase < 3433
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3450
                                        else ((phase[:32] & 0) | 67108879)
                                    )
                                    if phase < 3451
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3453
                                        else (
                                            ((phase[:32] & 0) | 4227858431)
                                            if phase < 3454
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3471
                                else (
                                    (
                                        ((phase[:32] & 0) | 4227858446)
                                        if phase < 3472
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3474
                                            else ((phase[:32] & 0) | 134217728)
                                        )
                                    )
                                    if phase < 3475
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3492
                                        else (
                                            ((phase[:32] & 0) | 134217743)
                                            if phase < 3493
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                    )
                    if phase < 3495
                    else (
                        (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 4160749567)
                                        if phase < 3496
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3513
                                    else (
                                        ((phase[:32] & 0) | 4160749582)
                                        if phase < 3514
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3516
                                            else ((phase[:32] & 0) | 268435456)
                                        )
                                    )
                                )
                                if phase < 3517
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3534
                                        else (
                                            ((phase[:32] & 0) | 268435471)
                                            if phase < 3535
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 3537
                                    else (
                                        ((phase[:32] & 0) | 4026531839)
                                        if phase < 3538
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3555
                                            else ((phase[:32] & 0) | 4026531854)
                                        )
                                    )
                                )
                            )
                            if phase < 3556
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3558
                                        else ((phase[:32] & 0) | 536870912)
                                    )
                                    if phase < 3559
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3576
                                        else (
                                            ((phase[:32] & 0) | 536870927)
                                            if phase < 3577
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3579
                                else (
                                    (
                                        ((phase[:32] & 0) | 3758096383)
                                        if phase < 3580
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3597
                                            else ((phase[:32] & 0) | 3758096398)
                                        )
                                    )
                                    if phase < 3598
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3600
                                        else (
                                            ((phase[:32] & 0) | 1073741824)
                                            if phase < 3601
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                            )
                        )
                        if phase < 3618
                        else (
                            (
                                (
                                    (
                                        ((phase[:32] & 0) | 1073741839)
                                        if phase < 3619
                                        else ((phase[:32] & 0) | 0)
                                    )
                                    if phase < 3621
                                    else (
                                        ((phase[:32] & 0) | 3221225471)
                                        if phase < 3622
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3639
                                            else ((phase[:32] & 0) | 3221225486)
                                        )
                                    )
                                )
                                if phase < 3640
                                else (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3642
                                        else (
                                            ((phase[:32] & 0) | 2147483648)
                                            if phase < 3643
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                    if phase < 3660
                                    else (
                                        ((phase[:32] & 0) | 2147483663)
                                        if phase < 3661
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3663
                                            else ((phase[:32] & 0) | 2147483647)
                                        )
                                    )
                                )
                            )
                            if phase < 3664
                            else (
                                (
                                    (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3681
                                        else ((phase[:32] & 0) | 2147483662)
                                    )
                                    if phase < 3682
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3689
                                        else (
                                            ((phase[:32] & 0) | 10)
                                            if phase < 3692
                                            else ((phase[:32] & 0) | 0)
                                        )
                                    )
                                )
                                if phase < 3693
                                else (
                                    (
                                        ((phase[:32] & 0) | 23)
                                        if phase < 3694
                                        else (
                                            ((phase[:32] & 0) | 0)
                                            if phase < 3695
                                            else ((phase[:32] & 0) | 31)
                                        )
                                    )
                                    if phase < 3696
                                    else (
                                        ((phase[:32] & 0) | 0)
                                        if phase < 3698
                                        else (
                                            ((phase[:32] & 0) | 42)
                                            if phase < 3699
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
    )
    expected_data_remaining = (phase[:4] & 0) | 0

    @rule
    def check():
        if phase < 3763:
            assert dut.ready == expected_ready, "feedback_pipeline: ready"
            assert dut.valid == expected_valid, "feedback_pipeline: valid"
            assert (
                dut.data.value == expected_data_value
            ), "feedback_pipeline: data.value"
            assert (
                dut.data.remaining == expected_data_remaining
            ), "feedback_pipeline: data.remaining"
        log("info", "feedback_pipeline.ready", dut.ready)
        log("info", "feedback_pipeline.valid", dut.valid)
        log("info", "feedback_pipeline.data.value", dut.data.value)
        log("info", "feedback_pipeline.data.remaining", dut.data.remaining)

    check()
    advance(phase)

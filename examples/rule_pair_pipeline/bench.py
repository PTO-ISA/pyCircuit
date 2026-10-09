"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_rule_pair_pipeline.rule_pair_pipeline import RulePairPipeline
from pycircuit import bits, log, rule, struct, system


@struct
class Stimulus:
    left_valid: bits[1]
    left_data: bits[64]
    left_take: bits[1]
    right_valid: bits[1]
    right_data: bits[64]
    right_take: bits[1]
    expected_left_ready: bits[1]
    expected_left_valid: bits[1]
    expected_left_data: bits[64]
    expected_right_ready: bits[1]
    expected_right_valid: bits[1]
    expected_right_data: bits[64]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    left_valid: bits[1] = 0
    left_data: bits[64] = 0
    left_take: bits[1] = 0
    right_valid: bits[1] = 0
    right_data: bits[64] = 0
    right_take: bits[1] = 0
    expected_left_ready: bits[1] = 0
    expected_left_valid: bits[1] = 0
    expected_left_data: bits[64] = 0
    expected_right_ready: bits[1] = 0
    expected_right_valid: bits[1] = 0
    expected_right_data: bits[64] = 0
    if phase == 0:
        left_valid = 1
        right_data = 9223372036854775808
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 1:
        left_valid = 1
        left_data = 18446744073709551615
        right_valid = 1
        right_data = 9223372036854775807
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 2:
        left_valid = 1
        left_data = 18446744073709551614
        right_valid = 1
        right_data = 12297829382473034410
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_ready = 1
    elif phase == 3:
        left_valid = 1
        left_data = 9223372036854775808
        right_data = 6148914691236517205
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 4:
        left_valid = 1
        left_data = 9223372036854775807
        right_valid = 1
        right_data = 1
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 5:
        left_valid = 1
        left_data = 12297829382473034410
        right_valid = 1
        right_data = 2
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 6:
        left_valid = 1
        left_data = 6148914691236517205
        right_data = 4
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 7:
        left_valid = 1
        left_data = 1
        right_valid = 1
        right_data = 8
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 8:
        left_valid = 1
        left_data = 2
        right_valid = 1
        right_data = 16
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 9:
        left_valid = 1
        left_data = 4
        right_data = 32
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 10:
        left_valid = 1
        left_data = 8
        right_valid = 1
        right_data = 64
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 11:
        left_valid = 1
        left_data = 16
        right_valid = 1
        right_data = 128
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 12:
        left_valid = 1
        left_data = 32
        right_data = 256
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 13:
        left_valid = 1
        left_data = 64
        left_take = 1
        right_valid = 1
        right_data = 512
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 14:
        left_valid = 1
        left_data = 128
        left_take = 1
        right_valid = 1
        right_data = 1024
        expected_left_ready = 1
        expected_left_valid = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 15:
        left_valid = 1
        left_data = 256
        left_take = 1
        right_data = 2048
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 65
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 16:
        left_valid = 1
        left_data = 512
        right_valid = 1
        right_data = 4096
        expected_left_valid = 1
        expected_left_data = 129
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 17:
        left_valid = 1
        left_data = 1024
        left_take = 1
        right_valid = 1
        right_data = 8192
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 129
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 18:
        left_valid = 1
        left_data = 2048
        left_take = 1
        right_data = 16384
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 257
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 19:
        left_valid = 1
        left_data = 4096
        left_take = 1
        right_valid = 1
        right_data = 32768
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1025
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 6148914691236517204
    elif phase == 20:
        left_valid = 1
        left_data = 8192
        right_valid = 1
        right_data = 65536
        expected_left_valid = 1
        expected_left_data = 2049
        expected_right_ready = 1
    elif phase == 21:
        left_valid = 1
        left_data = 16384
        left_take = 1
        right_data = 131072
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 2049
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 65536
    elif phase == 22:
        left_valid = 1
        left_data = 32768
        left_take = 1
        right_valid = 1
        right_data = 262144
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 4097
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 131072
    elif phase == 23:
        left_valid = 1
        left_data = 65536
        left_take = 1
        right_valid = 1
        right_data = 524288
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 16385
        expected_right_ready = 1
    elif phase == 24:
        left_valid = 1
        left_data = 131072
        right_data = 1048576
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 32769
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 524288
    elif phase == 25:
        left_valid = 1
        left_data = 262144
        left_take = 1
        right_valid = 1
        right_data = 2097152
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 32769
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 1048576
    elif phase == 26:
        left_valid = 1
        left_data = 524288
        left_take = 1
        right_valid = 1
        right_data = 4194304
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 65537
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 1048576
    elif phase == 27:
        left_valid = 1
        left_data = 1048576
        left_take = 1
        right_data = 8388608
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 262145
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4194304
    elif phase == 28:
        left_valid = 1
        left_data = 2097152
        right_valid = 1
        right_data = 16777216
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 524289
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 8388608
    elif phase == 29:
        left_valid = 1
        left_data = 4194304
        left_take = 1
        right_valid = 1
        right_data = 33554432
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 524289
        expected_right_ready = 1
    elif phase == 30:
        left_valid = 1
        left_data = 8388608
        left_take = 1
        right_data = 67108864
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1048577
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 31:
        left_data = 16777216
        left_take = 1
        right_valid = 1
        right_data = 134217728
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 4194305
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 32:
        left_valid = 1
        left_data = 33554432
        right_valid = 1
        right_data = 268435456
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 8388609
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 67108864
    elif phase == 33:
        left_valid = 1
        left_data = 67108864
        left_take = 1
        right_data = 536870912
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 8388609
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 268435456
    elif phase == 34:
        left_valid = 1
        left_data = 134217728
        left_take = 1
        right_valid = 1
        right_data = 1073741824
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 33554433
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 536870912
    elif phase == 35:
        left_valid = 1
        left_data = 268435456
        left_take = 1
        right_valid = 1
        right_data = 2147483648
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 67108865
        expected_right_ready = 1
    elif phase == 36:
        left_data = 536870912
        right_data = 4294967296
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 134217729
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 2147483648
    elif phase == 37:
        left_valid = 1
        left_data = 1073741824
        left_take = 1
        right_valid = 1
        right_data = 8589934592
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 134217729
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4294967296
    elif phase == 38:
        left_valid = 1
        left_data = 2147483648
        left_take = 1
        right_valid = 1
        right_data = 17179869184
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 268435457
        expected_right_ready = 1
    elif phase == 39:
        left_valid = 1
        left_data = 4294967296
        left_take = 1
        right_data = 34359738368
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1073741825
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 17179869184
    elif phase == 40:
        left_valid = 1
        left_data = 8589934592
        right_valid = 1
        right_data = 68719476736
        expected_left_valid = 1
        expected_left_data = 2147483649
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 41:
        left_data = 17179869184
        left_take = 1
        right_valid = 1
        right_data = 137438953472
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 2147483649
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 42:
        left_valid = 1
        left_data = 34359738368
        left_take = 1
        right_data = 274877906944
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 4294967297
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 43:
        left_valid = 1
        left_data = 68719476736
        left_take = 1
        right_valid = 1
        right_data = 549755813888
        expected_left_ready = 1
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 44:
        left_valid = 1
        left_data = 137438953472
        right_valid = 1
        right_data = 1099511627776
        expected_left_valid = 1
        expected_left_data = 34359738369
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 45:
        left_valid = 1
        left_data = 274877906944
        left_take = 1
        right_data = 2199023255552
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 34359738369
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 46:
        left_data = 549755813888
        left_take = 1
        right_valid = 1
        right_data = 4398046511104
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 68719476737
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 47:
        left_valid = 1
        left_data = 1099511627776
        left_take = 1
        right_valid = 1
        right_data = 8796093022208
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 274877906945
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 48:
        left_valid = 1
        left_data = 2199023255552
        right_data = 17592186044416
        expected_left_ready = 1
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 49:
        left_valid = 1
        left_data = 4398046511104
        right_valid = 1
        right_data = 35184372088832
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 50:
        left_valid = 1
        left_data = 8796093022208
        right_valid = 1
        right_data = 70368744177664
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 51:
        left_data = 17592186044416
        right_data = 140737488355328
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 52:
        left_valid = 1
        left_data = 35184372088832
        right_valid = 1
        right_data = 281474976710656
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 53:
        left_valid = 1
        left_data = 70368744177664
        right_valid = 1
        right_data = 562949953421312
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 54:
        left_valid = 1
        left_data = 140737488355328
        right_data = 1125899906842624
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 55:
        left_valid = 1
        left_data = 281474976710656
        right_valid = 1
        right_data = 2251799813685248
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 56:
        left_data = 562949953421312
        right_valid = 1
        right_data = 4503599627370496
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 57:
        left_valid = 1
        left_data = 1125899906842624
        right_data = 9007199254740992
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 137438953472
    elif phase == 58:
        left_valid = 1
        left_data = 2251799813685248
        right_valid = 1
        right_data = 18014398509481984
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 9007199254740992
    elif phase == 59:
        left_valid = 1
        left_data = 4503599627370496
        right_valid = 1
        right_data = 36028797018963968
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_ready = 1
    elif phase == 60:
        left_valid = 1
        left_data = 9007199254740992
        right_data = 72057594037927936
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_valid = 1
        expected_right_data = 36028797018963968
    elif phase == 61:
        left_data = 18014398509481984
        right_valid = 1
        right_data = 144115188075855872
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 36028797018963968
    elif phase == 62:
        left_valid = 1
        left_data = 36028797018963968
        right_valid = 1
        right_data = 288230376151711744
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 72057594037927936
    elif phase == 63:
        left_valid = 1
        left_data = 72057594037927936
        right_data = 576460752303423488
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 288230376151711744
    elif phase == 64:
        left_valid = 1
        left_take = 1
        right_valid = 1
        right_data = 1152921504606846976
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 576460752303423488
    elif phase == 65:
        left_valid = 1
        left_data = 18446744073709551615
        left_take = 1
        right_valid = 1
        right_data = 2305843009213693952
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 2199023255553
        expected_right_ready = 1
    elif phase == 66:
        left_valid = 1
        left_data = 18446744073709551614
        left_take = 1
        right_valid = 1
        right_data = 4611686018427387904
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 2305843009213693952
    elif phase == 67:
        left_valid = 1
        left_data = 9223372036854775808
        left_take = 1
        right_valid = 1
        right_data = 9223372036854775808
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4611686018427387904
    elif phase == 68:
        left_valid = 1
        left_data = 9223372036854775807
        left_take = 1
        right_valid = 1
        right_data = 1
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 18446744073709551615
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 9223372036854775808
    elif phase == 69:
        left_valid = 1
        left_data = 12297829382473034410
        left_take = 1
        right_valid = 1
        right_data = 2
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 9223372036854775809
        expected_right_ready = 1
        expected_right_valid = 1
    elif phase == 70:
        left_valid = 1
        left_data = 6148914691236517205
        left_take = 1
        right_valid = 1
        right_data = 4
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 9223372036854775808
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 2
    elif phase == 71:
        left_valid = 1
        left_data = 1
        left_take = 1
        right_valid = 1
        right_data = 8
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 12297829382473034411
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4
    elif phase == 72:
        left_valid = 1
        left_data = 2
        left_take = 1
        right_valid = 1
        right_data = 16
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 6148914691236517206
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 8
    elif phase == 73:
        left_valid = 1
        left_data = 4
        left_take = 1
        right_valid = 1
        right_data = 32
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 2
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 16
    elif phase == 74:
        left_valid = 1
        left_data = 8
        left_take = 1
        right_valid = 1
        right_data = 64
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 3
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 32
    elif phase == 75:
        left_valid = 1
        left_data = 16
        left_take = 1
        right_valid = 1
        right_data = 128
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 5
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 64
    elif phase == 76:
        left_valid = 1
        left_data = 32
        left_take = 1
        right_valid = 1
        right_data = 256
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 9
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 128
    elif phase == 77:
        left_valid = 1
        left_data = 64
        left_take = 1
        right_valid = 1
        right_data = 512
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 17
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 256
    elif phase == 78:
        left_valid = 1
        left_data = 128
        left_take = 1
        right_valid = 1
        right_data = 1024
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 33
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 512
    elif phase == 79:
        left_valid = 1
        left_data = 256
        left_take = 1
        right_valid = 1
        right_data = 2048
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 65
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 1024
    elif phase == 80:
        left_valid = 1
        left_data = 512
        left_take = 1
        right_valid = 1
        right_data = 4096
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 129
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 2048
    elif phase == 81:
        left_valid = 1
        left_data = 1024
        left_take = 1
        right_valid = 1
        right_data = 8192
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 257
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4096
    elif phase == 82:
        left_valid = 1
        left_data = 2048
        left_take = 1
        right_valid = 1
        right_data = 16384
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 513
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 8192
    elif phase == 83:
        left_valid = 1
        left_data = 4096
        left_take = 1
        right_valid = 1
        right_data = 32768
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1025
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 16384
    elif phase == 84:
        left_valid = 1
        left_data = 8192
        left_take = 1
        right_valid = 1
        right_data = 65536
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 2049
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 32768
    elif phase == 85:
        left_valid = 1
        left_data = 16384
        left_take = 1
        right_valid = 1
        right_data = 131072
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 4097
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 65536
    elif phase == 86:
        left_valid = 1
        left_data = 32768
        left_take = 1
        right_valid = 1
        right_data = 262144
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 8193
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 131072
    elif phase == 87:
        left_valid = 1
        left_data = 65536
        left_take = 1
        right_valid = 1
        right_data = 524288
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 16385
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 262144
    elif phase == 88:
        left_valid = 1
        left_data = 131072
        left_take = 1
        right_valid = 1
        right_data = 1048576
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 32769
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 524288
    elif phase == 89:
        left_valid = 1
        left_data = 262144
        left_take = 1
        right_valid = 1
        right_data = 2097152
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 65537
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 1048576
    elif phase == 90:
        left_valid = 1
        left_data = 524288
        left_take = 1
        right_valid = 1
        right_data = 4194304
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 131073
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 2097152
    elif phase == 91:
        left_valid = 1
        left_data = 1048576
        left_take = 1
        right_valid = 1
        right_data = 8388608
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 262145
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4194304
    elif phase == 92:
        left_valid = 1
        left_data = 2097152
        left_take = 1
        right_valid = 1
        right_data = 16777216
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 524289
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 8388608
    elif phase == 93:
        left_valid = 1
        left_data = 4194304
        left_take = 1
        right_valid = 1
        right_data = 33554432
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1048577
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 16777216
    elif phase == 94:
        left_valid = 1
        left_data = 8388608
        left_take = 1
        right_valid = 1
        right_data = 67108864
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 2097153
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 95:
        left_valid = 1
        left_data = 16777216
        left_take = 1
        right_valid = 1
        right_data = 134217728
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 4194305
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 67108864
    elif phase == 96:
        left_valid = 1
        left_data = 33554432
        left_take = 1
        right_valid = 1
        right_data = 268435456
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 8388609
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 134217728
    elif phase == 97:
        left_valid = 1
        left_data = 67108864
        left_take = 1
        right_valid = 1
        right_data = 536870912
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 16777217
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 268435456
    elif phase == 98:
        left_valid = 1
        left_data = 134217728
        left_take = 1
        right_valid = 1
        right_data = 1073741824
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 33554433
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 536870912
    elif phase == 99:
        left_valid = 1
        left_data = 268435456
        left_take = 1
        right_valid = 1
        right_data = 2147483648
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 67108865
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 1073741824
    elif phase == 100:
        left_valid = 1
        left_data = 536870912
        left_take = 1
        right_valid = 1
        right_data = 4294967296
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 134217729
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 2147483648
    elif phase == 101:
        left_valid = 1
        left_data = 1073741824
        left_take = 1
        right_valid = 1
        right_data = 8589934592
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 268435457
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4294967296
    elif phase == 102:
        left_valid = 1
        left_data = 2147483648
        left_take = 1
        right_valid = 1
        right_data = 17179869184
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 536870913
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 8589934592
    elif phase == 103:
        left_valid = 1
        left_data = 4294967296
        left_take = 1
        right_valid = 1
        right_data = 34359738368
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1073741825
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 17179869184
    elif phase == 104:
        left_valid = 1
        left_data = 8589934592
        left_take = 1
        right_valid = 1
        right_data = 68719476736
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 2147483649
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 105:
        left_valid = 1
        left_data = 17179869184
        left_take = 1
        right_valid = 1
        right_data = 137438953472
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 4294967297
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 68719476736
    elif phase == 106:
        left_valid = 1
        left_data = 34359738368
        left_take = 1
        right_valid = 1
        right_data = 274877906944
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 8589934593
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 137438953472
    elif phase == 107:
        left_valid = 1
        left_data = 68719476736
        left_take = 1
        right_valid = 1
        right_data = 549755813888
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 17179869185
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 274877906944
    elif phase == 108:
        left_valid = 1
        left_data = 137438953472
        left_take = 1
        right_valid = 1
        right_data = 1099511627776
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 34359738369
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 549755813888
    elif phase == 109:
        left_valid = 1
        left_data = 274877906944
        left_take = 1
        right_valid = 1
        right_data = 2199023255552
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 68719476737
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 1099511627776
    elif phase == 110:
        left_valid = 1
        left_data = 549755813888
        left_take = 1
        right_valid = 1
        right_data = 4398046511104
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 137438953473
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 2199023255552
    elif phase == 111:
        left_valid = 1
        left_data = 1099511627776
        left_take = 1
        right_valid = 1
        right_data = 8796093022208
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 274877906945
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4398046511104
    elif phase == 112:
        left_valid = 1
        left_data = 2199023255552
        left_take = 1
        right_valid = 1
        right_data = 17592186044416
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 549755813889
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 8796093022208
    elif phase == 113:
        left_valid = 1
        left_data = 4398046511104
        left_take = 1
        right_valid = 1
        right_data = 35184372088832
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1099511627777
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 17592186044416
    elif phase == 114:
        left_valid = 1
        left_data = 8796093022208
        left_take = 1
        right_valid = 1
        right_data = 70368744177664
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 2199023255553
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 35184372088832
    elif phase == 115:
        left_valid = 1
        left_data = 17592186044416
        left_take = 1
        right_valid = 1
        right_data = 140737488355328
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 4398046511105
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 70368744177664
    elif phase == 116:
        left_valid = 1
        left_data = 35184372088832
        left_take = 1
        right_valid = 1
        right_data = 281474976710656
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 8796093022209
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 140737488355328
    elif phase == 117:
        left_valid = 1
        left_data = 70368744177664
        left_take = 1
        right_valid = 1
        right_data = 562949953421312
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 17592186044417
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 281474976710656
    elif phase == 118:
        left_valid = 1
        left_data = 140737488355328
        left_take = 1
        right_valid = 1
        right_data = 1125899906842624
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 35184372088833
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 562949953421312
    elif phase == 119:
        left_valid = 1
        left_data = 281474976710656
        left_take = 1
        right_valid = 1
        right_data = 2251799813685248
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 70368744177665
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 1125899906842624
    elif phase == 120:
        left_valid = 1
        left_data = 562949953421312
        left_take = 1
        right_valid = 1
        right_data = 4503599627370496
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 140737488355329
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 2251799813685248
    elif phase == 121:
        left_valid = 1
        left_data = 1125899906842624
        left_take = 1
        right_valid = 1
        right_data = 9007199254740992
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 281474976710657
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4503599627370496
    elif phase == 122:
        left_valid = 1
        left_data = 2251799813685248
        left_take = 1
        right_valid = 1
        right_data = 18014398509481984
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 562949953421313
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 9007199254740992
    elif phase == 123:
        left_valid = 1
        left_data = 4503599627370496
        left_take = 1
        right_valid = 1
        right_data = 36028797018963968
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1125899906842625
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 18014398509481984
    elif phase == 124:
        left_valid = 1
        left_data = 9007199254740992
        left_take = 1
        right_valid = 1
        right_data = 72057594037927936
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 2251799813685249
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 36028797018963968
    elif phase == 125:
        left_valid = 1
        left_data = 18014398509481984
        left_take = 1
        right_valid = 1
        right_data = 144115188075855872
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 4503599627370497
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 72057594037927936
    elif phase == 126:
        left_valid = 1
        left_data = 36028797018963968
        left_take = 1
        right_valid = 1
        right_data = 288230376151711744
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 9007199254740993
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 144115188075855872
    elif phase == 127:
        left_valid = 1
        left_data = 72057594037927936
        left_take = 1
        right_valid = 1
        right_data = 576460752303423488
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 18014398509481985
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 288230376151711744
    elif phase == 128:
        left_valid = 1
        left_data = 144115188075855872
        left_take = 1
        right_valid = 1
        right_data = 1152921504606846976
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 36028797018963969
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 576460752303423488
    elif phase == 129:
        left_valid = 1
        left_data = 288230376151711744
        left_take = 1
        right_valid = 1
        right_data = 2305843009213693952
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 72057594037927937
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 1152921504606846976
    elif phase == 130:
        left_valid = 1
        left_data = 576460752303423488
        left_take = 1
        right_valid = 1
        right_data = 4611686018427387904
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 144115188075855873
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 2305843009213693952
    elif phase == 131:
        left_valid = 1
        left_data = 1152921504606846976
        left_take = 1
        right_valid = 1
        right_data = 9223372036854775808
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 288230376151711745
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4611686018427387904
    elif phase == 132:
        left_valid = 1
        left_data = 2305843009213693952
        left_take = 1
        right_valid = 1
        right_data = 1
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 576460752303423489
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 9223372036854775808
    elif phase == 133:
        left_valid = 1
        left_data = 4611686018427387904
        left_take = 1
        right_valid = 1
        right_data = 2
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 1152921504606846977
        expected_right_ready = 1
        expected_right_valid = 1
    elif phase == 134:
        left_data = 9223372036854775808
        left_take = 1
        right_data = 4
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 2305843009213693953
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 2
    elif phase == 135:
        left_data = 1
        left_take = 1
        right_data = 8
        right_take = 1
        expected_left_ready = 1
        expected_left_valid = 1
        expected_left_data = 4611686018427387905
        expected_right_ready = 1
        expected_right_valid = 1
        expected_right_data = 4
    elif phase == 136:
        left_data = 2
        left_take = 1
        right_data = 16
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 137:
        left_data = 4
        left_take = 1
        right_data = 32
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 138:
        left_data = 8
        left_take = 1
        right_data = 64
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 139:
        left_data = 16
        left_take = 1
        right_data = 128
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 140:
        left_data = 32
        left_take = 1
        right_data = 256
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 141:
        left_data = 64
        left_take = 1
        right_data = 512
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 142:
        left_data = 128
        left_take = 1
        right_data = 1024
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 143:
        left_data = 256
        left_take = 1
        right_data = 2048
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 144:
        left_data = 512
        left_take = 1
        right_data = 4096
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 145:
        left_data = 1024
        left_take = 1
        right_data = 8192
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 146:
        left_data = 2048
        left_take = 1
        right_data = 16384
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 147:
        left_data = 4096
        left_take = 1
        right_data = 32768
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 148:
        left_data = 8192
        left_take = 1
        right_data = 65536
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 149:
        left_data = 16384
        left_take = 1
        right_data = 131072
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 150:
        left_data = 32768
        left_take = 1
        right_data = 262144
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 151:
        left_data = 65536
        left_take = 1
        right_data = 524288
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 152:
        left_data = 131072
        left_take = 1
        right_data = 1048576
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 153:
        left_data = 262144
        left_take = 1
        right_data = 2097152
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 154:
        left_data = 524288
        left_take = 1
        right_data = 4194304
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 155:
        left_data = 1048576
        left_take = 1
        right_data = 8388608
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 156:
        left_data = 2097152
        left_take = 1
        right_data = 16777216
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 157:
        left_data = 4194304
        left_take = 1
        right_data = 33554432
        right_take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    return Stimulus(
        left_valid=left_valid,
        left_data=left_data,
        left_take=left_take,
        right_valid=right_valid,
        right_data=right_data,
        right_take=right_take,
        expected_left_ready=expected_left_ready,
        expected_left_valid=expected_left_valid,
        expected_left_data=expected_left_data,
        expected_right_ready=expected_right_ready,
        expected_right_valid=expected_right_valid,
        expected_right_data=expected_right_data,
    )


@rule
def advance(phase):
    if phase < 157:
        phase = phase + 1


@system
def RulePairPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = RulePairPipeline(
        frame.left_valid,
        frame.left_data,
        frame.left_take,
        frame.right_valid,
        frame.right_data,
        frame.right_take,
    )

    @rule
    def exercise():
        assert (
            dut.left_ready == frame.expected_left_ready
        ), "rule_pair_pipeline left_ready old-state check"
        assert (
            dut.left_valid == frame.expected_left_valid
        ), "rule_pair_pipeline left_valid old-state check"
        assert (
            dut.left_data == frame.expected_left_data
        ), "rule_pair_pipeline left_data old-state check"
        assert (
            dut.right_ready == frame.expected_right_ready
        ), "rule_pair_pipeline right_ready old-state check"
        assert (
            dut.right_valid == frame.expected_right_valid
        ), "rule_pair_pipeline right_valid old-state check"
        assert (
            dut.right_data == frame.expected_right_data
        ), "rule_pair_pipeline right_data old-state check"
        log("info", "phase", phase)
        log("info", "left_ready", dut.left_ready)
        log("info", "left_valid", dut.left_valid)
        log("info", "left_data", dut.left_data)
        log("info", "right_ready", dut.right_ready)
        log("info", "right_valid", dut.right_valid)
        log("info", "right_data", dut.right_data)

    advance(phase)
    exercise()

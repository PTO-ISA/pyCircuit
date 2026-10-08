"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_multi_input_rule_pipeline.multi_input_rule_pipeline import (
    MultiInputRulePipeline,
)
from pycircuit import bits, log, rule, struct, system


@struct
class Stimulus:
    left_valid: bits[1]
    left_data: bits[64]
    right_valid: bits[1]
    right_data: bits[64]
    take: bits[1]
    expected_left_ready: bits[1]
    expected_right_ready: bits[1]
    expected_valid: bits[1]
    expected_data: bits[64]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    left_valid: bits[1] = 0
    left_data: bits[64] = 0
    right_valid: bits[1] = 0
    right_data: bits[64] = 0
    take: bits[1] = 0
    expected_left_ready: bits[1] = 0
    expected_right_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_data: bits[64] = 0
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
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 3:
        left_valid = 1
        left_data = 9223372036854775808
        right_data = 6148914691236517205
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 4:
        left_valid = 1
        left_data = 9223372036854775807
        right_valid = 1
        right_data = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 5:
        left_valid = 1
        left_data = 12297829382473034410
        right_valid = 1
        right_data = 2
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 6:
        left_valid = 1
        left_data = 6148914691236517205
        right_data = 4
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 7:
        left_valid = 1
        left_data = 1
        right_valid = 1
        right_data = 8
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 8:
        left_valid = 1
        left_data = 2
        right_valid = 1
        right_data = 16
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 9:
        left_valid = 1
        left_data = 4
        right_data = 32
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 10:
        left_valid = 1
        left_data = 8
        right_valid = 1
        right_data = 64
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 11:
        left_valid = 1
        left_data = 16
        right_valid = 1
        right_data = 128
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 12:
        left_valid = 1
        left_data = 32
        right_data = 256
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 13:
        left_valid = 1
        left_data = 64
        right_valid = 1
        right_data = 512
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9223372036854775807
    elif phase == 14:
        left_valid = 1
        left_data = 128
        right_valid = 1
        right_data = 1024
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 12297829382473034409
    elif phase == 15:
        left_valid = 1
        left_data = 256
        right_data = 2048
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 18446744073709551615
    elif phase == 16:
        left_valid = 1
        left_data = 512
        right_valid = 1
        right_data = 4096
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 576
    elif phase == 17:
        left_valid = 1
        left_data = 1024
        right_valid = 1
        right_data = 8192
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 576
    elif phase == 18:
        left_valid = 1
        left_data = 2048
        right_data = 16384
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1152
    elif phase == 19:
        left_valid = 1
        left_data = 4096
        right_valid = 1
        right_data = 32768
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4352
    elif phase == 20:
        left_valid = 1
        left_data = 8192
        right_valid = 1
        right_data = 65536
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9216
    elif phase == 21:
        left_valid = 1
        left_data = 16384
        right_data = 131072
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9216
    elif phase == 22:
        left_valid = 1
        left_data = 32768
        right_valid = 1
        right_data = 262144
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 34816
    elif phase == 23:
        left_valid = 1
        left_data = 65536
        right_valid = 1
        right_data = 524288
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 69632
    elif phase == 24:
        left_valid = 1
        left_data = 131072
        right_data = 1048576
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 278528
    elif phase == 25:
        left_valid = 1
        left_data = 262144
        right_valid = 1
        right_data = 2097152
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 278528
    elif phase == 26:
        left_valid = 1
        left_data = 524288
        right_valid = 1
        right_data = 4194304
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 557056
    elif phase == 27:
        left_valid = 1
        left_data = 1048576
        right_data = 8388608
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 2162688
    elif phase == 28:
        left_valid = 1
        left_data = 2097152
        right_valid = 1
        right_data = 16777216
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4456448
    elif phase == 29:
        left_valid = 1
        left_data = 4194304
        right_valid = 1
        right_data = 33554432
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4456448
    elif phase == 30:
        left_valid = 1
        left_data = 8388608
        right_data = 67108864
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 17301504
    elif phase == 31:
        left_data = 16777216
        right_valid = 1
        right_data = 134217728
        take = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 34603008
    elif phase == 32:
        left_valid = 1
        left_data = 33554432
        right_valid = 1
        right_data = 268435456
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 33:
        left_valid = 1
        left_data = 67108864
        right_data = 536870912
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 138412032
    elif phase == 34:
        left_valid = 1
        left_data = 134217728
        right_valid = 1
        right_data = 1073741824
        take = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 276824064
    elif phase == 35:
        left_valid = 1
        left_data = 268435456
        right_valid = 1
        right_data = 2147483648
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 36:
        left_data = 536870912
        right_data = 4294967296
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1107296256
    elif phase == 37:
        left_valid = 1
        left_data = 1073741824
        right_valid = 1
        right_data = 8589934592
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1107296256
    elif phase == 38:
        left_valid = 1
        left_data = 2147483648
        right_valid = 1
        right_data = 17179869184
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 2214592512
    elif phase == 39:
        left_valid = 1
        left_data = 4294967296
        right_data = 34359738368
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 8858370048
    elif phase == 40:
        left_valid = 1
        left_data = 8589934592
        right_valid = 1
        right_data = 68719476736
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 18253611008
    elif phase == 41:
        left_data = 17179869184
        right_valid = 1
        right_data = 137438953472
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 18253611008
    elif phase == 42:
        left_valid = 1
        left_data = 34359738368
        right_data = 274877906944
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 70866960384
    elif phase == 43:
        left_valid = 1
        left_data = 68719476736
        right_valid = 1
        right_data = 549755813888
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 141733920768
    elif phase == 44:
        left_valid = 1
        left_data = 137438953472
        right_valid = 1
        right_data = 1099511627776
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 45:
        left_valid = 1
        left_data = 274877906944
        right_data = 2199023255552
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 584115552256
    elif phase == 46:
        left_data = 549755813888
        right_valid = 1
        right_data = 4398046511104
        take = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1168231104512
    elif phase == 47:
        left_valid = 1
        left_data = 1099511627776
        right_valid = 1
        right_data = 8796093022208
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 48:
        left_valid = 1
        left_data = 2199023255552
        right_data = 17592186044416
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 49:
        left_valid = 1
        left_data = 4398046511104
        right_valid = 1
        right_data = 35184372088832
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 50:
        left_valid = 1
        left_data = 8796093022208
        right_valid = 1
        right_data = 70368744177664
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 51:
        left_data = 17592186044416
        right_data = 140737488355328
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 52:
        left_valid = 1
        left_data = 35184372088832
        right_valid = 1
        right_data = 281474976710656
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 53:
        left_valid = 1
        left_data = 70368744177664
        right_valid = 1
        right_data = 562949953421312
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 54:
        left_valid = 1
        left_data = 140737488355328
        right_data = 1125899906842624
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 55:
        left_valid = 1
        left_data = 281474976710656
        right_valid = 1
        right_data = 2251799813685248
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 56:
        left_data = 562949953421312
        right_valid = 1
        right_data = 4503599627370496
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 57:
        left_valid = 1
        left_data = 1125899906842624
        right_data = 9007199254740992
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 58:
        left_valid = 1
        left_data = 2251799813685248
        right_valid = 1
        right_data = 18014398509481984
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 59:
        left_valid = 1
        left_data = 4503599627370496
        right_valid = 1
        right_data = 36028797018963968
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 60:
        left_valid = 1
        left_data = 9007199254740992
        right_data = 72057594037927936
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 61:
        left_data = 18014398509481984
        right_valid = 1
        right_data = 144115188075855872
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 62:
        left_valid = 1
        left_data = 36028797018963968
        right_valid = 1
        right_data = 288230376151711744
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 63:
        left_valid = 1
        left_data = 72057594037927936
        right_data = 576460752303423488
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 64:
        left_valid = 1
        right_valid = 1
        right_data = 1152921504606846976
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4535485464576
    elif phase == 65:
        left_valid = 1
        left_data = 18446744073709551615
        right_valid = 1
        right_data = 2305843009213693952
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9070970929152
    elif phase == 66:
        left_valid = 1
        left_data = 18446744073709551614
        right_valid = 1
        right_data = 4611686018427387904
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 36283883716608
    elif phase == 67:
        left_valid = 1
        left_data = 9223372036854775808
        right_valid = 1
        right_data = 9223372036854775808
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1152921504606846976
    elif phase == 68:
        left_valid = 1
        left_data = 9223372036854775807
        right_valid = 1
        right_data = 1
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 2305843009213693951
    elif phase == 69:
        left_valid = 1
        left_data = 12297829382473034410
        right_valid = 1
        right_data = 2
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4611686018427387902
    elif phase == 70:
        left_valid = 1
        left_data = 6148914691236517205
        right_valid = 1
        right_data = 4
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
    elif phase == 71:
        left_valid = 1
        left_data = 1
        right_valid = 1
        right_data = 8
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9223372036854775808
    elif phase == 72:
        left_valid = 1
        left_data = 2
        right_valid = 1
        right_data = 16
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 12297829382473034412
    elif phase == 73:
        left_valid = 1
        left_data = 4
        right_valid = 1
        right_data = 32
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 6148914691236517209
    elif phase == 74:
        left_valid = 1
        left_data = 8
        right_valid = 1
        right_data = 64
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9
    elif phase == 75:
        left_valid = 1
        left_data = 16
        right_valid = 1
        right_data = 128
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 18
    elif phase == 76:
        left_valid = 1
        left_data = 32
        right_valid = 1
        right_data = 256
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 36
    elif phase == 77:
        left_valid = 1
        left_data = 64
        right_valid = 1
        right_data = 512
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 72
    elif phase == 78:
        left_valid = 1
        left_data = 128
        right_valid = 1
        right_data = 1024
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 144
    elif phase == 79:
        left_valid = 1
        left_data = 256
        right_valid = 1
        right_data = 2048
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 288
    elif phase == 80:
        left_valid = 1
        left_data = 512
        right_valid = 1
        right_data = 4096
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 576
    elif phase == 81:
        left_valid = 1
        left_data = 1024
        right_valid = 1
        right_data = 8192
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1152
    elif phase == 82:
        left_valid = 1
        left_data = 2048
        right_valid = 1
        right_data = 16384
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 2304
    elif phase == 83:
        left_valid = 1
        left_data = 4096
        right_valid = 1
        right_data = 32768
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4608
    elif phase == 84:
        left_valid = 1
        left_data = 8192
        right_valid = 1
        right_data = 65536
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9216
    elif phase == 85:
        left_valid = 1
        left_data = 16384
        right_valid = 1
        right_data = 131072
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 18432
    elif phase == 86:
        left_valid = 1
        left_data = 32768
        right_valid = 1
        right_data = 262144
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 36864
    elif phase == 87:
        left_valid = 1
        left_data = 65536
        right_valid = 1
        right_data = 524288
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 73728
    elif phase == 88:
        left_valid = 1
        left_data = 131072
        right_valid = 1
        right_data = 1048576
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 147456
    elif phase == 89:
        left_valid = 1
        left_data = 262144
        right_valid = 1
        right_data = 2097152
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 294912
    elif phase == 90:
        left_valid = 1
        left_data = 524288
        right_valid = 1
        right_data = 4194304
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 589824
    elif phase == 91:
        left_valid = 1
        left_data = 1048576
        right_valid = 1
        right_data = 8388608
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1179648
    elif phase == 92:
        left_valid = 1
        left_data = 2097152
        right_valid = 1
        right_data = 16777216
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 2359296
    elif phase == 93:
        left_valid = 1
        left_data = 4194304
        right_valid = 1
        right_data = 33554432
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4718592
    elif phase == 94:
        left_valid = 1
        left_data = 8388608
        right_valid = 1
        right_data = 67108864
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9437184
    elif phase == 95:
        left_valid = 1
        left_data = 16777216
        right_valid = 1
        right_data = 134217728
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 18874368
    elif phase == 96:
        left_valid = 1
        left_data = 33554432
        right_valid = 1
        right_data = 268435456
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 37748736
    elif phase == 97:
        left_valid = 1
        left_data = 67108864
        right_valid = 1
        right_data = 536870912
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 75497472
    elif phase == 98:
        left_valid = 1
        left_data = 134217728
        right_valid = 1
        right_data = 1073741824
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 150994944
    elif phase == 99:
        left_valid = 1
        left_data = 268435456
        right_valid = 1
        right_data = 2147483648
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 301989888
    elif phase == 100:
        left_valid = 1
        left_data = 536870912
        right_valid = 1
        right_data = 4294967296
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 603979776
    elif phase == 101:
        left_valid = 1
        left_data = 1073741824
        right_valid = 1
        right_data = 8589934592
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1207959552
    elif phase == 102:
        left_valid = 1
        left_data = 2147483648
        right_valid = 1
        right_data = 17179869184
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 2415919104
    elif phase == 103:
        left_valid = 1
        left_data = 4294967296
        right_valid = 1
        right_data = 34359738368
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4831838208
    elif phase == 104:
        left_valid = 1
        left_data = 8589934592
        right_valid = 1
        right_data = 68719476736
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9663676416
    elif phase == 105:
        left_valid = 1
        left_data = 17179869184
        right_valid = 1
        right_data = 137438953472
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 19327352832
    elif phase == 106:
        left_valid = 1
        left_data = 34359738368
        right_valid = 1
        right_data = 274877906944
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 38654705664
    elif phase == 107:
        left_valid = 1
        left_data = 68719476736
        right_valid = 1
        right_data = 549755813888
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 77309411328
    elif phase == 108:
        left_valid = 1
        left_data = 137438953472
        right_valid = 1
        right_data = 1099511627776
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 154618822656
    elif phase == 109:
        left_valid = 1
        left_data = 274877906944
        right_valid = 1
        right_data = 2199023255552
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 309237645312
    elif phase == 110:
        left_valid = 1
        left_data = 549755813888
        right_valid = 1
        right_data = 4398046511104
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 618475290624
    elif phase == 111:
        left_valid = 1
        left_data = 1099511627776
        right_valid = 1
        right_data = 8796093022208
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1236950581248
    elif phase == 112:
        left_valid = 1
        left_data = 2199023255552
        right_valid = 1
        right_data = 17592186044416
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 2473901162496
    elif phase == 113:
        left_valid = 1
        left_data = 4398046511104
        right_valid = 1
        right_data = 35184372088832
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4947802324992
    elif phase == 114:
        left_valid = 1
        left_data = 8796093022208
        right_valid = 1
        right_data = 70368744177664
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 9895604649984
    elif phase == 115:
        left_valid = 1
        left_data = 17592186044416
        right_valid = 1
        right_data = 140737488355328
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 19791209299968
    elif phase == 116:
        left_valid = 1
        left_data = 35184372088832
        right_valid = 1
        right_data = 281474976710656
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 39582418599936
    elif phase == 117:
        left_valid = 1
        left_data = 70368744177664
        right_valid = 1
        right_data = 562949953421312
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 79164837199872
    elif phase == 118:
        left_valid = 1
        left_data = 140737488355328
        right_valid = 1
        right_data = 1125899906842624
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 158329674399744
    elif phase == 119:
        left_valid = 1
        left_data = 281474976710656
        right_valid = 1
        right_data = 2251799813685248
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 316659348799488
    elif phase == 120:
        left_valid = 1
        left_data = 562949953421312
        right_valid = 1
        right_data = 4503599627370496
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 633318697598976
    elif phase == 121:
        left_valid = 1
        left_data = 1125899906842624
        right_valid = 1
        right_data = 9007199254740992
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1266637395197952
    elif phase == 122:
        left_valid = 1
        left_data = 2251799813685248
        right_valid = 1
        right_data = 18014398509481984
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 2533274790395904
    elif phase == 123:
        left_valid = 1
        left_data = 4503599627370496
        right_valid = 1
        right_data = 36028797018963968
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 5066549580791808
    elif phase == 124:
        left_valid = 1
        left_data = 9007199254740992
        right_valid = 1
        right_data = 72057594037927936
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 10133099161583616
    elif phase == 125:
        left_valid = 1
        left_data = 18014398509481984
        right_valid = 1
        right_data = 144115188075855872
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 20266198323167232
    elif phase == 126:
        left_valid = 1
        left_data = 36028797018963968
        right_valid = 1
        right_data = 288230376151711744
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 40532396646334464
    elif phase == 127:
        left_valid = 1
        left_data = 72057594037927936
        right_valid = 1
        right_data = 576460752303423488
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 81064793292668928
    elif phase == 128:
        left_valid = 1
        left_data = 144115188075855872
        right_valid = 1
        right_data = 1152921504606846976
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 162129586585337856
    elif phase == 129:
        left_valid = 1
        left_data = 288230376151711744
        right_valid = 1
        right_data = 2305843009213693952
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 324259173170675712
    elif phase == 130:
        left_valid = 1
        left_data = 576460752303423488
        right_valid = 1
        right_data = 4611686018427387904
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 648518346341351424
    elif phase == 131:
        left_valid = 1
        left_data = 1152921504606846976
        right_valid = 1
        right_data = 9223372036854775808
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 1297036692682702848
    elif phase == 132:
        left_valid = 1
        left_data = 2305843009213693952
        right_valid = 1
        right_data = 1
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 2594073385365405696
    elif phase == 133:
        left_valid = 1
        left_data = 4611686018427387904
        right_valid = 1
        right_data = 2
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 5188146770730811392
    elif phase == 134:
        left_data = 9223372036854775808
        right_data = 4
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 10376293541461622784
    elif phase == 135:
        left_data = 1
        right_data = 8
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 2305843009213693953
    elif phase == 136:
        left_data = 2
        right_data = 16
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
        expected_valid = 1
        expected_data = 4611686018427387906
    elif phase == 137:
        left_data = 4
        right_data = 32
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 138:
        left_data = 8
        right_data = 64
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 139:
        left_data = 16
        right_data = 128
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 140:
        left_data = 32
        right_data = 256
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 141:
        left_data = 64
        right_data = 512
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 142:
        left_data = 128
        right_data = 1024
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 143:
        left_data = 256
        right_data = 2048
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 144:
        left_data = 512
        right_data = 4096
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 145:
        left_data = 1024
        right_data = 8192
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 146:
        left_data = 2048
        right_data = 16384
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 147:
        left_data = 4096
        right_data = 32768
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 148:
        left_data = 8192
        right_data = 65536
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 149:
        left_data = 16384
        right_data = 131072
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 150:
        left_data = 32768
        right_data = 262144
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 151:
        left_data = 65536
        right_data = 524288
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 152:
        left_data = 131072
        right_data = 1048576
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 153:
        left_data = 262144
        right_data = 2097152
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 154:
        left_data = 524288
        right_data = 4194304
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 155:
        left_data = 1048576
        right_data = 8388608
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 156:
        left_data = 2097152
        right_data = 16777216
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    elif phase == 157:
        left_data = 4194304
        right_data = 33554432
        take = 1
        expected_left_ready = 1
        expected_right_ready = 1
    return Stimulus(
        left_valid=left_valid,
        left_data=left_data,
        right_valid=right_valid,
        right_data=right_data,
        take=take,
        expected_left_ready=expected_left_ready,
        expected_right_ready=expected_right_ready,
        expected_valid=expected_valid,
        expected_data=expected_data,
    )


@rule
def advance(phase):
    if phase < 157:
        phase = phase + 1


@system
def MultiInputRulePipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = MultiInputRulePipeline(
        frame.left_valid,
        frame.left_data,
        frame.right_valid,
        frame.right_data,
        frame.take,
    )

    @rule
    def exercise():
        assert (
            dut.left_ready == frame.expected_left_ready
        ), "multi_input_rule_pipeline left_ready old-state check"
        assert (
            dut.right_ready == frame.expected_right_ready
        ), "multi_input_rule_pipeline right_ready old-state check"
        assert (
            dut.valid == frame.expected_valid
        ), "multi_input_rule_pipeline valid old-state check"
        assert (
            dut.data == frame.expected_data
        ), "multi_input_rule_pipeline data old-state check"
        log("info", "phase", phase)
        log("info", "left_ready", dut.left_ready)
        log("info", "right_ready", dut.right_ready)
        log("info", "valid", dut.valid)
        log("info", "data", dut.data)

    advance(phase)
    exercise()

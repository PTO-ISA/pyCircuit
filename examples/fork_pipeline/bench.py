"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_fork_pipeline.fork_pipeline import ForkPipeline
from pycircuit import bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    data: bits[64]
    left_take: bits[1]
    right_take: bits[1]
    expected_ready: bits[1]
    expected_left_valid: bits[1]
    expected_left_data: bits[64]
    expected_right_valid: bits[1]
    expected_right_data: bits[64]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    data: bits[64] = 0
    left_take: bits[1] = 0
    right_take: bits[1] = 0
    expected_ready: bits[1] = 0
    expected_left_valid: bits[1] = 0
    expected_left_data: bits[64] = 0
    expected_right_valid: bits[1] = 0
    expected_right_data: bits[64] = 0
    if phase == 0:
        valid = 1
        expected_ready = 1
    elif phase == 1:
        valid = 1
        data = 18446744073709551615
        expected_ready = 1
    elif phase == 2:
        valid = 1
        data = 18446744073709551614
        expected_ready = 1
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 3:
        valid = 1
        data = 9223372036854775808
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 4:
        valid = 1
        data = 9223372036854775807
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 5:
        valid = 1
        data = 12297829382473034410
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 6:
        valid = 1
        data = 6148914691236517205
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 7:
        valid = 1
        data = 1
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 8:
        valid = 1
        data = 2
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 9:
        valid = 1
        data = 4
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 10:
        valid = 1
        data = 8
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 11:
        valid = 1
        data = 16
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 12:
        valid = 1
        data = 32
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 13:
        valid = 1
        data = 64
        left_take = 1
        expected_left_valid = 1
        expected_right_valid = 1
    elif phase == 14:
        valid = 1
        data = 128
        left_take = 1
        expected_left_valid = 1
        expected_left_data = 18446744073709551615
        expected_right_valid = 1
    elif phase == 15:
        valid = 1
        data = 256
        left_take = 1
        expected_left_valid = 1
        expected_left_data = 18446744073709551614
        expected_right_valid = 1
    elif phase == 16:
        valid = 1
        data = 512
        expected_right_valid = 1
    elif phase == 17:
        valid = 1
        data = 1024
        left_take = 1
        expected_right_valid = 1
    elif phase == 18:
        valid = 1
        data = 2048
        left_take = 1
        expected_right_valid = 1
    elif phase == 19:
        valid = 1
        data = 4096
        left_take = 1
        expected_right_valid = 1
    elif phase == 20:
        valid = 1
        data = 8192
        expected_right_valid = 1
    elif phase == 21:
        valid = 1
        data = 16384
        left_take = 1
        expected_right_valid = 1
    elif phase == 22:
        valid = 1
        data = 32768
        left_take = 1
        expected_right_valid = 1
    elif phase == 23:
        valid = 1
        data = 65536
        left_take = 1
        expected_right_valid = 1
    elif phase == 24:
        valid = 1
        data = 131072
        right_take = 1
        expected_ready = 1
        expected_right_valid = 1
    elif phase == 25:
        valid = 1
        data = 262144
        left_take = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551615
    elif phase == 26:
        valid = 1
        data = 524288
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 131072
        expected_right_valid = 1
        expected_right_data = 18446744073709551615
    elif phase == 27:
        valid = 1
        data = 1048576
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 28:
        valid = 1
        data = 2097152
        expected_left_valid = 1
        expected_left_data = 524288
        expected_right_valid = 1
        expected_right_data = 131072
    elif phase == 29:
        valid = 1
        data = 4194304
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 524288
        expected_right_valid = 1
        expected_right_data = 131072
    elif phase == 30:
        valid = 1
        data = 8388608
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 1048576
        expected_right_valid = 1
        expected_right_data = 524288
    elif phase == 31:
        data = 16777216
        left_take = 1
        expected_left_valid = 1
        expected_left_data = 4194304
        expected_right_valid = 1
        expected_right_data = 1048576
    elif phase == 32:
        valid = 1
        data = 33554432
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 8388608
        expected_right_valid = 1
        expected_right_data = 1048576
    elif phase == 33:
        valid = 1
        data = 67108864
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 8388608
        expected_right_valid = 1
        expected_right_data = 4194304
    elif phase == 34:
        valid = 1
        data = 134217728
        left_take = 1
        expected_left_valid = 1
        expected_left_data = 33554432
        expected_right_valid = 1
        expected_right_data = 8388608
    elif phase == 35:
        valid = 1
        data = 268435456
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 67108864
        expected_right_valid = 1
        expected_right_data = 8388608
    elif phase == 36:
        data = 536870912
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 37:
        valid = 1
        data = 1073741824
        left_take = 1
        expected_left_valid = 1
        expected_left_data = 268435456
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 38:
        valid = 1
        data = 2147483648
        left_take = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 39:
        valid = 1
        data = 4294967296
        left_take = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 40:
        valid = 1
        data = 8589934592
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 41:
        data = 17179869184
        left_take = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 42:
        valid = 1
        data = 34359738368
        left_take = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 43:
        valid = 1
        data = 68719476736
        left_take = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 44:
        valid = 1
        data = 137438953472
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 45:
        valid = 1
        data = 274877906944
        left_take = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 46:
        data = 549755813888
        left_take = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 47:
        valid = 1
        data = 1099511627776
        left_take = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 48:
        valid = 1
        data = 2199023255552
        right_take = 1
        expected_ready = 1
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 49:
        valid = 1
        data = 4398046511104
        expected_right_valid = 1
        expected_right_data = 67108864
    elif phase == 50:
        valid = 1
        data = 8796093022208
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
        expected_right_valid = 1
        expected_right_data = 67108864
    elif phase == 51:
        data = 17592186044416
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
        expected_right_valid = 1
        expected_right_data = 268435456
    elif phase == 52:
        valid = 1
        data = 35184372088832
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
        expected_right_valid = 1
        expected_right_data = 2199023255552
    elif phase == 53:
        valid = 1
        data = 70368744177664
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
        expected_right_valid = 1
        expected_right_data = 2199023255552
    elif phase == 54:
        valid = 1
        data = 140737488355328
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
        expected_right_valid = 1
        expected_right_data = 8796093022208
    elif phase == 55:
        valid = 1
        data = 281474976710656
        expected_left_valid = 1
        expected_left_data = 2199023255552
        expected_right_valid = 1
        expected_right_data = 35184372088832
    elif phase == 56:
        data = 562949953421312
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
        expected_right_valid = 1
        expected_right_data = 35184372088832
    elif phase == 57:
        valid = 1
        data = 1125899906842624
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
    elif phase == 58:
        valid = 1
        data = 2251799813685248
        expected_left_valid = 1
        expected_left_data = 2199023255552
    elif phase == 59:
        valid = 1
        data = 4503599627370496
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
    elif phase == 60:
        valid = 1
        data = 9007199254740992
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
    elif phase == 61:
        data = 18014398509481984
        expected_left_valid = 1
        expected_left_data = 2199023255552
    elif phase == 62:
        valid = 1
        data = 36028797018963968
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
    elif phase == 63:
        valid = 1
        data = 72057594037927936
        right_take = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
    elif phase == 64:
        valid = 1
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
    elif phase == 65:
        valid = 1
        data = 18446744073709551615
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 8796093022208
    elif phase == 66:
        valid = 1
        data = 18446744073709551614
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 35184372088832
        expected_right_valid = 1
    elif phase == 67:
        valid = 1
        data = 9223372036854775808
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_right_valid = 1
        expected_right_data = 18446744073709551615
    elif phase == 68:
        valid = 1
        data = 9223372036854775807
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 18446744073709551615
        expected_right_valid = 1
        expected_right_data = 18446744073709551614
    elif phase == 69:
        valid = 1
        data = 12297829382473034410
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 18446744073709551614
        expected_right_valid = 1
        expected_right_data = 9223372036854775808
    elif phase == 70:
        valid = 1
        data = 6148914691236517205
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 9223372036854775808
        expected_right_valid = 1
        expected_right_data = 9223372036854775807
    elif phase == 71:
        valid = 1
        data = 1
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 9223372036854775807
        expected_right_valid = 1
        expected_right_data = 12297829382473034410
    elif phase == 72:
        valid = 1
        data = 2
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 12297829382473034410
        expected_right_valid = 1
        expected_right_data = 6148914691236517205
    elif phase == 73:
        valid = 1
        data = 4
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 6148914691236517205
        expected_right_valid = 1
        expected_right_data = 1
    elif phase == 74:
        valid = 1
        data = 8
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 1
        expected_right_valid = 1
        expected_right_data = 2
    elif phase == 75:
        valid = 1
        data = 16
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2
        expected_right_valid = 1
        expected_right_data = 4
    elif phase == 76:
        valid = 1
        data = 32
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 4
        expected_right_valid = 1
        expected_right_data = 8
    elif phase == 77:
        valid = 1
        data = 64
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 8
        expected_right_valid = 1
        expected_right_data = 16
    elif phase == 78:
        valid = 1
        data = 128
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 16
        expected_right_valid = 1
        expected_right_data = 32
    elif phase == 79:
        valid = 1
        data = 256
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 32
        expected_right_valid = 1
        expected_right_data = 64
    elif phase == 80:
        valid = 1
        data = 512
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 64
        expected_right_valid = 1
        expected_right_data = 128
    elif phase == 81:
        valid = 1
        data = 1024
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 128
        expected_right_valid = 1
        expected_right_data = 256
    elif phase == 82:
        valid = 1
        data = 2048
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 256
        expected_right_valid = 1
        expected_right_data = 512
    elif phase == 83:
        valid = 1
        data = 4096
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 512
        expected_right_valid = 1
        expected_right_data = 1024
    elif phase == 84:
        valid = 1
        data = 8192
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 1024
        expected_right_valid = 1
        expected_right_data = 2048
    elif phase == 85:
        valid = 1
        data = 16384
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2048
        expected_right_valid = 1
        expected_right_data = 4096
    elif phase == 86:
        valid = 1
        data = 32768
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 4096
        expected_right_valid = 1
        expected_right_data = 8192
    elif phase == 87:
        valid = 1
        data = 65536
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 8192
        expected_right_valid = 1
        expected_right_data = 16384
    elif phase == 88:
        valid = 1
        data = 131072
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 16384
        expected_right_valid = 1
        expected_right_data = 32768
    elif phase == 89:
        valid = 1
        data = 262144
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 32768
        expected_right_valid = 1
        expected_right_data = 65536
    elif phase == 90:
        valid = 1
        data = 524288
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 65536
        expected_right_valid = 1
        expected_right_data = 131072
    elif phase == 91:
        valid = 1
        data = 1048576
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 131072
        expected_right_valid = 1
        expected_right_data = 262144
    elif phase == 92:
        valid = 1
        data = 2097152
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 262144
        expected_right_valid = 1
        expected_right_data = 524288
    elif phase == 93:
        valid = 1
        data = 4194304
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 524288
        expected_right_valid = 1
        expected_right_data = 1048576
    elif phase == 94:
        valid = 1
        data = 8388608
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 1048576
        expected_right_valid = 1
        expected_right_data = 2097152
    elif phase == 95:
        valid = 1
        data = 16777216
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2097152
        expected_right_valid = 1
        expected_right_data = 4194304
    elif phase == 96:
        valid = 1
        data = 33554432
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 4194304
        expected_right_valid = 1
        expected_right_data = 8388608
    elif phase == 97:
        valid = 1
        data = 67108864
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 8388608
        expected_right_valid = 1
        expected_right_data = 16777216
    elif phase == 98:
        valid = 1
        data = 134217728
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 16777216
        expected_right_valid = 1
        expected_right_data = 33554432
    elif phase == 99:
        valid = 1
        data = 268435456
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 33554432
        expected_right_valid = 1
        expected_right_data = 67108864
    elif phase == 100:
        valid = 1
        data = 536870912
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 67108864
        expected_right_valid = 1
        expected_right_data = 134217728
    elif phase == 101:
        valid = 1
        data = 1073741824
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 134217728
        expected_right_valid = 1
        expected_right_data = 268435456
    elif phase == 102:
        valid = 1
        data = 2147483648
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 268435456
        expected_right_valid = 1
        expected_right_data = 536870912
    elif phase == 103:
        valid = 1
        data = 4294967296
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 536870912
        expected_right_valid = 1
        expected_right_data = 1073741824
    elif phase == 104:
        valid = 1
        data = 8589934592
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 1073741824
        expected_right_valid = 1
        expected_right_data = 2147483648
    elif phase == 105:
        valid = 1
        data = 17179869184
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2147483648
        expected_right_valid = 1
        expected_right_data = 4294967296
    elif phase == 106:
        valid = 1
        data = 34359738368
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 4294967296
        expected_right_valid = 1
        expected_right_data = 8589934592
    elif phase == 107:
        valid = 1
        data = 68719476736
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 8589934592
        expected_right_valid = 1
        expected_right_data = 17179869184
    elif phase == 108:
        valid = 1
        data = 137438953472
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 17179869184
        expected_right_valid = 1
        expected_right_data = 34359738368
    elif phase == 109:
        valid = 1
        data = 274877906944
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 34359738368
        expected_right_valid = 1
        expected_right_data = 68719476736
    elif phase == 110:
        valid = 1
        data = 549755813888
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 68719476736
        expected_right_valid = 1
        expected_right_data = 137438953472
    elif phase == 111:
        valid = 1
        data = 1099511627776
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 137438953472
        expected_right_valid = 1
        expected_right_data = 274877906944
    elif phase == 112:
        valid = 1
        data = 2199023255552
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 274877906944
        expected_right_valid = 1
        expected_right_data = 549755813888
    elif phase == 113:
        valid = 1
        data = 4398046511104
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 549755813888
        expected_right_valid = 1
        expected_right_data = 1099511627776
    elif phase == 114:
        valid = 1
        data = 8796093022208
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 1099511627776
        expected_right_valid = 1
        expected_right_data = 2199023255552
    elif phase == 115:
        valid = 1
        data = 17592186044416
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2199023255552
        expected_right_valid = 1
        expected_right_data = 4398046511104
    elif phase == 116:
        valid = 1
        data = 35184372088832
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 4398046511104
        expected_right_valid = 1
        expected_right_data = 8796093022208
    elif phase == 117:
        valid = 1
        data = 70368744177664
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 8796093022208
        expected_right_valid = 1
        expected_right_data = 17592186044416
    elif phase == 118:
        valid = 1
        data = 140737488355328
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 17592186044416
        expected_right_valid = 1
        expected_right_data = 35184372088832
    elif phase == 119:
        valid = 1
        data = 281474976710656
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 35184372088832
        expected_right_valid = 1
        expected_right_data = 70368744177664
    elif phase == 120:
        valid = 1
        data = 562949953421312
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 70368744177664
        expected_right_valid = 1
        expected_right_data = 140737488355328
    elif phase == 121:
        valid = 1
        data = 1125899906842624
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 140737488355328
        expected_right_valid = 1
        expected_right_data = 281474976710656
    elif phase == 122:
        valid = 1
        data = 2251799813685248
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 281474976710656
        expected_right_valid = 1
        expected_right_data = 562949953421312
    elif phase == 123:
        valid = 1
        data = 4503599627370496
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 562949953421312
        expected_right_valid = 1
        expected_right_data = 1125899906842624
    elif phase == 124:
        valid = 1
        data = 9007199254740992
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 1125899906842624
        expected_right_valid = 1
        expected_right_data = 2251799813685248
    elif phase == 125:
        valid = 1
        data = 18014398509481984
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2251799813685248
        expected_right_valid = 1
        expected_right_data = 4503599627370496
    elif phase == 126:
        valid = 1
        data = 36028797018963968
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 4503599627370496
        expected_right_valid = 1
        expected_right_data = 9007199254740992
    elif phase == 127:
        valid = 1
        data = 72057594037927936
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 9007199254740992
        expected_right_valid = 1
        expected_right_data = 18014398509481984
    elif phase == 128:
        valid = 1
        data = 144115188075855872
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 18014398509481984
        expected_right_valid = 1
        expected_right_data = 36028797018963968
    elif phase == 129:
        valid = 1
        data = 288230376151711744
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 36028797018963968
        expected_right_valid = 1
        expected_right_data = 72057594037927936
    elif phase == 130:
        valid = 1
        data = 576460752303423488
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 72057594037927936
        expected_right_valid = 1
        expected_right_data = 144115188075855872
    elif phase == 131:
        valid = 1
        data = 1152921504606846976
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 144115188075855872
        expected_right_valid = 1
        expected_right_data = 288230376151711744
    elif phase == 132:
        valid = 1
        data = 2305843009213693952
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 288230376151711744
        expected_right_valid = 1
        expected_right_data = 576460752303423488
    elif phase == 133:
        valid = 1
        data = 4611686018427387904
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 576460752303423488
        expected_right_valid = 1
        expected_right_data = 1152921504606846976
    elif phase == 134:
        data = 9223372036854775808
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 1152921504606846976
        expected_right_valid = 1
        expected_right_data = 2305843009213693952
    elif phase == 135:
        data = 1
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 2305843009213693952
        expected_right_valid = 1
        expected_right_data = 4611686018427387904
    elif phase == 136:
        data = 2
        left_take = 1
        right_take = 1
        expected_ready = 1
        expected_left_valid = 1
        expected_left_data = 4611686018427387904
    elif phase == 137:
        data = 4
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 138:
        data = 8
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 139:
        data = 16
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 140:
        data = 32
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 141:
        data = 64
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 142:
        data = 128
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 143:
        data = 256
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 144:
        data = 512
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 145:
        data = 1024
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 146:
        data = 2048
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 147:
        data = 4096
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 148:
        data = 8192
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 149:
        data = 16384
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 150:
        data = 32768
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 151:
        data = 65536
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 152:
        data = 131072
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 153:
        data = 262144
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 154:
        data = 524288
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 155:
        data = 1048576
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 156:
        data = 2097152
        left_take = 1
        right_take = 1
        expected_ready = 1
    elif phase == 157:
        data = 4194304
        left_take = 1
        right_take = 1
        expected_ready = 1
    return Stimulus(
        valid=valid,
        data=data,
        left_take=left_take,
        right_take=right_take,
        expected_ready=expected_ready,
        expected_left_valid=expected_left_valid,
        expected_left_data=expected_left_data,
        expected_right_valid=expected_right_valid,
        expected_right_data=expected_right_data,
    )


@rule
def advance(phase):
    if phase < 157:
        phase = phase + 1


@system
def ForkPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = ForkPipeline(frame.valid, frame.data, frame.left_take, frame.right_take)

    @rule
    def exercise():
        assert dut.ready == frame.expected_ready, "fork_pipeline ready old-state check"
        assert (
            dut.left_valid == frame.expected_left_valid
        ), "fork_pipeline left_valid old-state check"
        assert (
            dut.left_data == frame.expected_left_data
        ), "fork_pipeline left_data old-state check"
        assert (
            dut.right_valid == frame.expected_right_valid
        ), "fork_pipeline right_valid old-state check"
        assert (
            dut.right_data == frame.expected_right_data
        ), "fork_pipeline right_data old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "left_valid", dut.left_valid)
        log("info", "left_data", dut.left_data)
        log("info", "right_valid", dut.right_valid)
        log("info", "right_data", dut.right_data)

    advance(phase)
    exercise()

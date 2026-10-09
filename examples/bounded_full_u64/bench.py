"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_bounded_full_u64.bounded_full_u64 import BoundedFullU64
from pycircuit import bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    raw: bits[64]
    take_wrapped: bits[1]
    take_saturated: bits[1]
    take_checked_value: bits[1]
    take_checked_flag: bits[1]
    expected_ready: bits[1]
    expected_wrapped_valid: bits[1]
    expected_wrapped: bits[64]
    expected_saturated_valid: bits[1]
    expected_saturated: bits[64]
    expected_checked_value_valid: bits[1]
    expected_checked_value: bits[64]
    expected_checked_flag_valid: bits[1]
    expected_checked_flag: bits[1]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    raw: bits[64] = 0
    take_wrapped: bits[1] = 0
    take_saturated: bits[1] = 0
    take_checked_value: bits[1] = 0
    take_checked_flag: bits[1] = 0
    expected_ready: bits[1] = 0
    expected_wrapped_valid: bits[1] = 0
    expected_wrapped: bits[64] = 0
    expected_saturated_valid: bits[1] = 0
    expected_saturated: bits[64] = 0
    expected_checked_value_valid: bits[1] = 0
    expected_checked_value: bits[64] = 0
    expected_checked_flag_valid: bits[1] = 0
    expected_checked_flag: bits[1] = 0
    if phase == 0:
        valid = 1
        expected_ready = 1
    elif phase == 1:
        valid = 1
        raw = 18446744073709551615
        expected_ready = 1
    elif phase == 2:
        valid = 1
        raw = 18446744073709551614
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 3:
        valid = 1
        raw = 9223372036854775808
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 4:
        valid = 1
        raw = 9223372036854775807
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 5:
        valid = 1
        raw = 12297829382473034410
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 6:
        valid = 1
        raw = 6148914691236517205
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 7:
        valid = 1
        raw = 1
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 8:
        valid = 1
        raw = 2
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 9:
        valid = 1
        raw = 4
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 10:
        valid = 1
        raw = 8
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 11:
        valid = 1
        raw = 16
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 12:
        valid = 1
        raw = 32
        take_wrapped = 1
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 13:
        valid = 1
        raw = 64
        take_wrapped = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 14:
        valid = 1
        raw = 128
        take_wrapped = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 15:
        valid = 1
        raw = 256
        take_saturated = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 16:
        valid = 1
        raw = 512
        take_wrapped = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 17:
        valid = 1
        raw = 1024
        take_wrapped = 1
        take_saturated = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 18:
        valid = 1
        raw = 2048
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 19:
        valid = 1
        raw = 4096
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 20:
        valid = 1
        raw = 8192
        take_saturated = 1
        take_checked_value = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 21:
        valid = 1
        raw = 16384
        take_wrapped = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 22:
        valid = 1
        raw = 32768
        take_wrapped = 1
        take_saturated = 1
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 18446744073709551615
        expected_saturated_valid = 1
        expected_saturated = 18446744073709551615
        expected_checked_value_valid = 1
        expected_checked_value = 18446744073709551615
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 23:
        valid = 1
        raw = 65536
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        expected_ready = 1
        expected_checked_value_valid = 1
        expected_checked_value = 18446744073709551615
    elif phase == 24:
        valid = 1
        raw = 131072
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 16384
        expected_saturated_valid = 1
        expected_saturated = 16384
        expected_checked_value_valid = 1
        expected_checked_value = 16384
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 25:
        valid = 1
        raw = 262144
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 65536
        expected_saturated_valid = 1
        expected_saturated = 65536
        expected_checked_value_valid = 1
        expected_checked_value = 65536
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 26:
        valid = 1
        raw = 524288
        take_wrapped = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 65536
    elif phase == 27:
        valid = 1
        raw = 1048576
        take_wrapped = 1
        take_saturated = 1
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 131072
        expected_saturated_valid = 1
        expected_saturated = 131072
        expected_checked_value_valid = 1
        expected_checked_value = 131072
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 28:
        valid = 1
        raw = 2097152
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        expected_ready = 1
        expected_checked_value_valid = 1
        expected_checked_value = 131072
    elif phase == 29:
        valid = 1
        raw = 4194304
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 524288
        expected_saturated_valid = 1
        expected_saturated = 524288
        expected_checked_value_valid = 1
        expected_checked_value = 524288
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 30:
        valid = 1
        raw = 8388608
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2097152
        expected_saturated_valid = 1
        expected_saturated = 2097152
        expected_checked_value_valid = 1
        expected_checked_value = 2097152
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 31:
        raw = 16777216
        take_wrapped = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2097152
    elif phase == 32:
        valid = 1
        raw = 33554432
        take_saturated = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
        expected_saturated_valid = 1
        expected_saturated = 4194304
        expected_checked_value_valid = 1
        expected_checked_value = 4194304
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 33:
        valid = 1
        raw = 67108864
        take_saturated = 1
        take_checked_value = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
        expected_checked_value_valid = 1
        expected_checked_value = 4194304
    elif phase == 34:
        valid = 1
        raw = 134217728
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
    elif phase == 35:
        valid = 1
        raw = 268435456
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
    elif phase == 36:
        raw = 536870912
        take_checked_value = 1
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
    elif phase == 37:
        valid = 1
        raw = 1073741824
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
    elif phase == 38:
        valid = 1
        raw = 2147483648
        take_checked_value = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
    elif phase == 39:
        valid = 1
        raw = 4294967296
        take_checked_value = 1
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
    elif phase == 40:
        valid = 1
        raw = 8589934592
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
    elif phase == 41:
        raw = 17179869184
        take_wrapped = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
    elif phase == 42:
        valid = 1
        raw = 34359738368
        take_wrapped = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 33554432
        expected_saturated_valid = 1
        expected_saturated = 33554432
        expected_checked_value_valid = 1
        expected_checked_value = 33554432
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 43:
        valid = 1
        raw = 68719476736
        take_wrapped = 1
        expected_saturated_valid = 1
        expected_saturated = 33554432
        expected_checked_value_valid = 1
        expected_checked_value = 33554432
    elif phase == 44:
        valid = 1
        raw = 137438953472
        take_wrapped = 1
        take_saturated = 1
        expected_saturated_valid = 1
        expected_saturated = 33554432
        expected_checked_value_valid = 1
        expected_checked_value = 33554432
    elif phase == 45:
        valid = 1
        raw = 274877906944
        take_saturated = 1
        expected_checked_value_valid = 1
        expected_checked_value = 33554432
    elif phase == 46:
        raw = 549755813888
        take_wrapped = 1
        expected_checked_value_valid = 1
        expected_checked_value = 33554432
    elif phase == 47:
        valid = 1
        raw = 1099511627776
        take_wrapped = 1
        take_saturated = 1
        expected_checked_value_valid = 1
        expected_checked_value = 33554432
    elif phase == 48:
        valid = 1
        raw = 2199023255552
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        expected_ready = 1
        expected_checked_value_valid = 1
        expected_checked_value = 33554432
    elif phase == 49:
        valid = 1
        raw = 4398046511104
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        expected_wrapped_valid = 1
        expected_wrapped = 34359738368
        expected_saturated_valid = 1
        expected_saturated = 34359738368
        expected_checked_value_valid = 1
        expected_checked_value = 34359738368
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 50:
        valid = 1
        raw = 8796093022208
        take_saturated = 1
        take_checked_value = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 51:
        raw = 17592186044416
        take_wrapped = 1
        take_checked_value = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 52:
        valid = 1
        raw = 35184372088832
        take_wrapped = 1
        take_saturated = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 53:
        valid = 1
        raw = 70368744177664
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2199023255552
        expected_saturated_valid = 1
        expected_saturated = 2199023255552
        expected_checked_value_valid = 1
        expected_checked_value = 2199023255552
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 54:
        valid = 1
        raw = 140737488355328
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 55:
        valid = 1
        raw = 281474976710656
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 35184372088832
        expected_saturated_valid = 1
        expected_saturated = 35184372088832
        expected_checked_value_valid = 1
        expected_checked_value = 35184372088832
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 56:
        raw = 562949953421312
        take_wrapped = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 35184372088832
    elif phase == 57:
        valid = 1
        raw = 1125899906842624
        take_wrapped = 1
        take_saturated = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 140737488355328
        expected_saturated_valid = 1
        expected_saturated = 140737488355328
        expected_checked_value_valid = 1
        expected_checked_value = 140737488355328
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 58:
        valid = 1
        raw = 2251799813685248
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        expected_ready = 1
        expected_checked_value_valid = 1
        expected_checked_value = 140737488355328
    elif phase == 59:
        valid = 1
        raw = 4503599627370496
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 1125899906842624
        expected_saturated_valid = 1
        expected_saturated = 1125899906842624
        expected_checked_value_valid = 1
        expected_checked_value = 1125899906842624
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 60:
        valid = 1
        raw = 9007199254740992
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2251799813685248
        expected_saturated_valid = 1
        expected_saturated = 2251799813685248
        expected_checked_value_valid = 1
        expected_checked_value = 2251799813685248
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 61:
        raw = 18014398509481984
        take_wrapped = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2251799813685248
    elif phase == 62:
        valid = 1
        raw = 36028797018963968
        take_wrapped = 1
        take_saturated = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4503599627370496
        expected_saturated_valid = 1
        expected_saturated = 4503599627370496
        expected_checked_value_valid = 1
        expected_checked_value = 4503599627370496
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 63:
        valid = 1
        raw = 72057594037927936
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        expected_ready = 1
        expected_checked_value_valid = 1
        expected_checked_value = 4503599627370496
    elif phase == 64:
        valid = 1
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 36028797018963968
        expected_saturated_valid = 1
        expected_saturated = 36028797018963968
        expected_checked_value_valid = 1
        expected_checked_value = 36028797018963968
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 65:
        valid = 1
        raw = 18446744073709551615
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 72057594037927936
        expected_saturated_valid = 1
        expected_saturated = 72057594037927936
        expected_checked_value_valid = 1
        expected_checked_value = 72057594037927936
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 66:
        valid = 1
        raw = 18446744073709551614
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_saturated_valid = 1
        expected_checked_value_valid = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 67:
        valid = 1
        raw = 9223372036854775808
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 18446744073709551615
        expected_saturated_valid = 1
        expected_saturated = 18446744073709551615
        expected_checked_value_valid = 1
        expected_checked_value = 18446744073709551615
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 68:
        valid = 1
        raw = 9223372036854775807
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 18446744073709551614
        expected_saturated_valid = 1
        expected_saturated = 18446744073709551614
        expected_checked_value_valid = 1
        expected_checked_value = 18446744073709551614
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 69:
        valid = 1
        raw = 12297829382473034410
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 9223372036854775808
        expected_saturated_valid = 1
        expected_saturated = 9223372036854775808
        expected_checked_value_valid = 1
        expected_checked_value = 9223372036854775808
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 70:
        valid = 1
        raw = 6148914691236517205
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 9223372036854775807
        expected_saturated_valid = 1
        expected_saturated = 9223372036854775807
        expected_checked_value_valid = 1
        expected_checked_value = 9223372036854775807
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 71:
        valid = 1
        raw = 1
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 12297829382473034410
        expected_saturated_valid = 1
        expected_saturated = 12297829382473034410
        expected_checked_value_valid = 1
        expected_checked_value = 12297829382473034410
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 72:
        valid = 1
        raw = 2
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 6148914691236517205
        expected_saturated_valid = 1
        expected_saturated = 6148914691236517205
        expected_checked_value_valid = 1
        expected_checked_value = 6148914691236517205
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 73:
        valid = 1
        raw = 4
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 1
        expected_saturated_valid = 1
        expected_saturated = 1
        expected_checked_value_valid = 1
        expected_checked_value = 1
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 74:
        valid = 1
        raw = 8
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2
        expected_saturated_valid = 1
        expected_saturated = 2
        expected_checked_value_valid = 1
        expected_checked_value = 2
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 75:
        valid = 1
        raw = 16
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4
        expected_saturated_valid = 1
        expected_saturated = 4
        expected_checked_value_valid = 1
        expected_checked_value = 4
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 76:
        valid = 1
        raw = 32
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 8
        expected_saturated_valid = 1
        expected_saturated = 8
        expected_checked_value_valid = 1
        expected_checked_value = 8
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 77:
        valid = 1
        raw = 64
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 16
        expected_saturated_valid = 1
        expected_saturated = 16
        expected_checked_value_valid = 1
        expected_checked_value = 16
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 78:
        valid = 1
        raw = 128
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 32
        expected_saturated_valid = 1
        expected_saturated = 32
        expected_checked_value_valid = 1
        expected_checked_value = 32
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 79:
        valid = 1
        raw = 256
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 64
        expected_saturated_valid = 1
        expected_saturated = 64
        expected_checked_value_valid = 1
        expected_checked_value = 64
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 80:
        valid = 1
        raw = 512
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 128
        expected_saturated_valid = 1
        expected_saturated = 128
        expected_checked_value_valid = 1
        expected_checked_value = 128
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 81:
        valid = 1
        raw = 1024
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 256
        expected_saturated_valid = 1
        expected_saturated = 256
        expected_checked_value_valid = 1
        expected_checked_value = 256
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 82:
        valid = 1
        raw = 2048
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 512
        expected_saturated_valid = 1
        expected_saturated = 512
        expected_checked_value_valid = 1
        expected_checked_value = 512
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 83:
        valid = 1
        raw = 4096
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 1024
        expected_saturated_valid = 1
        expected_saturated = 1024
        expected_checked_value_valid = 1
        expected_checked_value = 1024
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 84:
        valid = 1
        raw = 8192
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2048
        expected_saturated_valid = 1
        expected_saturated = 2048
        expected_checked_value_valid = 1
        expected_checked_value = 2048
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 85:
        valid = 1
        raw = 16384
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4096
        expected_saturated_valid = 1
        expected_saturated = 4096
        expected_checked_value_valid = 1
        expected_checked_value = 4096
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 86:
        valid = 1
        raw = 32768
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 8192
        expected_saturated_valid = 1
        expected_saturated = 8192
        expected_checked_value_valid = 1
        expected_checked_value = 8192
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 87:
        valid = 1
        raw = 65536
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 16384
        expected_saturated_valid = 1
        expected_saturated = 16384
        expected_checked_value_valid = 1
        expected_checked_value = 16384
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 88:
        valid = 1
        raw = 131072
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 32768
        expected_saturated_valid = 1
        expected_saturated = 32768
        expected_checked_value_valid = 1
        expected_checked_value = 32768
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 89:
        valid = 1
        raw = 262144
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 65536
        expected_saturated_valid = 1
        expected_saturated = 65536
        expected_checked_value_valid = 1
        expected_checked_value = 65536
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 90:
        valid = 1
        raw = 524288
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 131072
        expected_saturated_valid = 1
        expected_saturated = 131072
        expected_checked_value_valid = 1
        expected_checked_value = 131072
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 91:
        valid = 1
        raw = 1048576
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 262144
        expected_saturated_valid = 1
        expected_saturated = 262144
        expected_checked_value_valid = 1
        expected_checked_value = 262144
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 92:
        valid = 1
        raw = 2097152
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 524288
        expected_saturated_valid = 1
        expected_saturated = 524288
        expected_checked_value_valid = 1
        expected_checked_value = 524288
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 93:
        valid = 1
        raw = 4194304
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 1048576
        expected_saturated_valid = 1
        expected_saturated = 1048576
        expected_checked_value_valid = 1
        expected_checked_value = 1048576
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 94:
        valid = 1
        raw = 8388608
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2097152
        expected_saturated_valid = 1
        expected_saturated = 2097152
        expected_checked_value_valid = 1
        expected_checked_value = 2097152
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 95:
        valid = 1
        raw = 16777216
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4194304
        expected_saturated_valid = 1
        expected_saturated = 4194304
        expected_checked_value_valid = 1
        expected_checked_value = 4194304
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 96:
        valid = 1
        raw = 33554432
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 8388608
        expected_saturated_valid = 1
        expected_saturated = 8388608
        expected_checked_value_valid = 1
        expected_checked_value = 8388608
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 97:
        valid = 1
        raw = 67108864
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 16777216
        expected_saturated_valid = 1
        expected_saturated = 16777216
        expected_checked_value_valid = 1
        expected_checked_value = 16777216
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 98:
        valid = 1
        raw = 134217728
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 33554432
        expected_saturated_valid = 1
        expected_saturated = 33554432
        expected_checked_value_valid = 1
        expected_checked_value = 33554432
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 99:
        valid = 1
        raw = 268435456
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 67108864
        expected_saturated_valid = 1
        expected_saturated = 67108864
        expected_checked_value_valid = 1
        expected_checked_value = 67108864
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 100:
        valid = 1
        raw = 536870912
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 134217728
        expected_saturated_valid = 1
        expected_saturated = 134217728
        expected_checked_value_valid = 1
        expected_checked_value = 134217728
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 101:
        valid = 1
        raw = 1073741824
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 268435456
        expected_saturated_valid = 1
        expected_saturated = 268435456
        expected_checked_value_valid = 1
        expected_checked_value = 268435456
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 102:
        valid = 1
        raw = 2147483648
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 536870912
        expected_saturated_valid = 1
        expected_saturated = 536870912
        expected_checked_value_valid = 1
        expected_checked_value = 536870912
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 103:
        valid = 1
        raw = 4294967296
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 1073741824
        expected_saturated_valid = 1
        expected_saturated = 1073741824
        expected_checked_value_valid = 1
        expected_checked_value = 1073741824
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 104:
        valid = 1
        raw = 8589934592
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2147483648
        expected_saturated_valid = 1
        expected_saturated = 2147483648
        expected_checked_value_valid = 1
        expected_checked_value = 2147483648
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 105:
        valid = 1
        raw = 17179869184
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4294967296
        expected_saturated_valid = 1
        expected_saturated = 4294967296
        expected_checked_value_valid = 1
        expected_checked_value = 4294967296
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 106:
        valid = 1
        raw = 34359738368
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 8589934592
        expected_saturated_valid = 1
        expected_saturated = 8589934592
        expected_checked_value_valid = 1
        expected_checked_value = 8589934592
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 107:
        valid = 1
        raw = 68719476736
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 17179869184
        expected_saturated_valid = 1
        expected_saturated = 17179869184
        expected_checked_value_valid = 1
        expected_checked_value = 17179869184
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 108:
        valid = 1
        raw = 137438953472
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 34359738368
        expected_saturated_valid = 1
        expected_saturated = 34359738368
        expected_checked_value_valid = 1
        expected_checked_value = 34359738368
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 109:
        valid = 1
        raw = 274877906944
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 68719476736
        expected_saturated_valid = 1
        expected_saturated = 68719476736
        expected_checked_value_valid = 1
        expected_checked_value = 68719476736
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 110:
        valid = 1
        raw = 549755813888
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 137438953472
        expected_saturated_valid = 1
        expected_saturated = 137438953472
        expected_checked_value_valid = 1
        expected_checked_value = 137438953472
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 111:
        valid = 1
        raw = 1099511627776
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 274877906944
        expected_saturated_valid = 1
        expected_saturated = 274877906944
        expected_checked_value_valid = 1
        expected_checked_value = 274877906944
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 112:
        valid = 1
        raw = 2199023255552
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 549755813888
        expected_saturated_valid = 1
        expected_saturated = 549755813888
        expected_checked_value_valid = 1
        expected_checked_value = 549755813888
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 113:
        valid = 1
        raw = 4398046511104
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 1099511627776
        expected_saturated_valid = 1
        expected_saturated = 1099511627776
        expected_checked_value_valid = 1
        expected_checked_value = 1099511627776
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 114:
        valid = 1
        raw = 8796093022208
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2199023255552
        expected_saturated_valid = 1
        expected_saturated = 2199023255552
        expected_checked_value_valid = 1
        expected_checked_value = 2199023255552
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 115:
        valid = 1
        raw = 17592186044416
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4398046511104
        expected_saturated_valid = 1
        expected_saturated = 4398046511104
        expected_checked_value_valid = 1
        expected_checked_value = 4398046511104
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 116:
        valid = 1
        raw = 35184372088832
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 8796093022208
        expected_saturated_valid = 1
        expected_saturated = 8796093022208
        expected_checked_value_valid = 1
        expected_checked_value = 8796093022208
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 117:
        valid = 1
        raw = 70368744177664
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 17592186044416
        expected_saturated_valid = 1
        expected_saturated = 17592186044416
        expected_checked_value_valid = 1
        expected_checked_value = 17592186044416
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 118:
        valid = 1
        raw = 140737488355328
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 35184372088832
        expected_saturated_valid = 1
        expected_saturated = 35184372088832
        expected_checked_value_valid = 1
        expected_checked_value = 35184372088832
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 119:
        valid = 1
        raw = 281474976710656
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 70368744177664
        expected_saturated_valid = 1
        expected_saturated = 70368744177664
        expected_checked_value_valid = 1
        expected_checked_value = 70368744177664
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 120:
        valid = 1
        raw = 562949953421312
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 140737488355328
        expected_saturated_valid = 1
        expected_saturated = 140737488355328
        expected_checked_value_valid = 1
        expected_checked_value = 140737488355328
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 121:
        valid = 1
        raw = 1125899906842624
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 281474976710656
        expected_saturated_valid = 1
        expected_saturated = 281474976710656
        expected_checked_value_valid = 1
        expected_checked_value = 281474976710656
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 122:
        valid = 1
        raw = 2251799813685248
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 562949953421312
        expected_saturated_valid = 1
        expected_saturated = 562949953421312
        expected_checked_value_valid = 1
        expected_checked_value = 562949953421312
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 123:
        valid = 1
        raw = 4503599627370496
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 1125899906842624
        expected_saturated_valid = 1
        expected_saturated = 1125899906842624
        expected_checked_value_valid = 1
        expected_checked_value = 1125899906842624
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 124:
        valid = 1
        raw = 9007199254740992
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2251799813685248
        expected_saturated_valid = 1
        expected_saturated = 2251799813685248
        expected_checked_value_valid = 1
        expected_checked_value = 2251799813685248
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 125:
        valid = 1
        raw = 18014398509481984
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4503599627370496
        expected_saturated_valid = 1
        expected_saturated = 4503599627370496
        expected_checked_value_valid = 1
        expected_checked_value = 4503599627370496
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 126:
        valid = 1
        raw = 36028797018963968
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 9007199254740992
        expected_saturated_valid = 1
        expected_saturated = 9007199254740992
        expected_checked_value_valid = 1
        expected_checked_value = 9007199254740992
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 127:
        valid = 1
        raw = 72057594037927936
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 18014398509481984
        expected_saturated_valid = 1
        expected_saturated = 18014398509481984
        expected_checked_value_valid = 1
        expected_checked_value = 18014398509481984
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 128:
        valid = 1
        raw = 144115188075855872
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 36028797018963968
        expected_saturated_valid = 1
        expected_saturated = 36028797018963968
        expected_checked_value_valid = 1
        expected_checked_value = 36028797018963968
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 129:
        valid = 1
        raw = 288230376151711744
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 72057594037927936
        expected_saturated_valid = 1
        expected_saturated = 72057594037927936
        expected_checked_value_valid = 1
        expected_checked_value = 72057594037927936
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 130:
        valid = 1
        raw = 576460752303423488
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 144115188075855872
        expected_saturated_valid = 1
        expected_saturated = 144115188075855872
        expected_checked_value_valid = 1
        expected_checked_value = 144115188075855872
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 131:
        valid = 1
        raw = 1152921504606846976
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 288230376151711744
        expected_saturated_valid = 1
        expected_saturated = 288230376151711744
        expected_checked_value_valid = 1
        expected_checked_value = 288230376151711744
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 132:
        valid = 1
        raw = 2305843009213693952
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 576460752303423488
        expected_saturated_valid = 1
        expected_saturated = 576460752303423488
        expected_checked_value_valid = 1
        expected_checked_value = 576460752303423488
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 133:
        valid = 1
        raw = 4611686018427387904
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 1152921504606846976
        expected_saturated_valid = 1
        expected_saturated = 1152921504606846976
        expected_checked_value_valid = 1
        expected_checked_value = 1152921504606846976
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 134:
        raw = 9223372036854775808
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 2305843009213693952
        expected_saturated_valid = 1
        expected_saturated = 2305843009213693952
        expected_checked_value_valid = 1
        expected_checked_value = 2305843009213693952
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 135:
        raw = 1
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
        expected_wrapped_valid = 1
        expected_wrapped = 4611686018427387904
        expected_saturated_valid = 1
        expected_saturated = 4611686018427387904
        expected_checked_value_valid = 1
        expected_checked_value = 4611686018427387904
        expected_checked_flag_valid = 1
        expected_checked_flag = 1
    elif phase == 136:
        raw = 2
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 137:
        raw = 4
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 138:
        raw = 8
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 139:
        raw = 16
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 140:
        raw = 32
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 141:
        raw = 64
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 142:
        raw = 128
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 143:
        raw = 256
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 144:
        raw = 512
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 145:
        raw = 1024
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 146:
        raw = 2048
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 147:
        raw = 4096
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 148:
        raw = 8192
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 149:
        raw = 16384
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 150:
        raw = 32768
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 151:
        raw = 65536
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 152:
        raw = 131072
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 153:
        raw = 262144
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 154:
        raw = 524288
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 155:
        raw = 1048576
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 156:
        raw = 2097152
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    elif phase == 157:
        raw = 4194304
        take_wrapped = 1
        take_saturated = 1
        take_checked_value = 1
        take_checked_flag = 1
        expected_ready = 1
    return Stimulus(
        valid=valid,
        raw=raw,
        take_wrapped=take_wrapped,
        take_saturated=take_saturated,
        take_checked_value=take_checked_value,
        take_checked_flag=take_checked_flag,
        expected_ready=expected_ready,
        expected_wrapped_valid=expected_wrapped_valid,
        expected_wrapped=expected_wrapped,
        expected_saturated_valid=expected_saturated_valid,
        expected_saturated=expected_saturated,
        expected_checked_value_valid=expected_checked_value_valid,
        expected_checked_value=expected_checked_value,
        expected_checked_flag_valid=expected_checked_flag_valid,
        expected_checked_flag=expected_checked_flag,
    )


@rule
def advance(phase):
    if phase < 157:
        phase = phase + 1


@system
def BoundedFullU64System():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = BoundedFullU64(
        frame.valid,
        frame.raw,
        frame.take_wrapped,
        frame.take_saturated,
        frame.take_checked_value,
        frame.take_checked_flag,
    )

    @rule
    def exercise():
        assert (
            dut.ready == frame.expected_ready
        ), "bounded_full_u64 ready old-state check"
        assert (
            dut.wrapped_valid == frame.expected_wrapped_valid
        ), "bounded_full_u64 wrapped_valid old-state check"
        assert (
            dut.wrapped == frame.expected_wrapped
        ), "bounded_full_u64 wrapped old-state check"
        assert (
            dut.saturated_valid == frame.expected_saturated_valid
        ), "bounded_full_u64 saturated_valid old-state check"
        assert (
            dut.saturated == frame.expected_saturated
        ), "bounded_full_u64 saturated old-state check"
        assert (
            dut.checked_value_valid == frame.expected_checked_value_valid
        ), "bounded_full_u64 checked_value_valid old-state check"
        assert (
            dut.checked_value == frame.expected_checked_value
        ), "bounded_full_u64 checked_value old-state check"
        assert (
            dut.checked_flag_valid == frame.expected_checked_flag_valid
        ), "bounded_full_u64 checked_flag_valid old-state check"
        assert (
            dut.checked_flag == frame.expected_checked_flag
        ), "bounded_full_u64 checked_flag old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "wrapped_valid", dut.wrapped_valid)
        log("info", "wrapped", dut.wrapped)
        log("info", "saturated_valid", dut.saturated_valid)
        log("info", "saturated", dut.saturated)
        log("info", "checked_value_valid", dut.checked_value_valid)
        log("info", "checked_value", dut.checked_value)
        log("info", "checked_flag_valid", dut.checked_flag_valid)
        log("info", "checked_flag", dut.checked_flag)

    advance(phase)
    exercise()

"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_struct_pipeline.struct_pipeline import StructPipeline, Item, StructResult
from pycircuit import bits, enum_to_bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_value: bits[64]
    input_remaining: bits[64]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_value: bits[64]
    expected_remaining: bits[64]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    take: bits[1] = 0
    input_value: bits[64] = 0
    input_remaining: bits[64] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_value: bits[64] = 0
    expected_remaining: bits[64] = 0
    if phase == 0:
        valid = 1
        input_remaining = 9223372036854775808
        expected_ready = 1
    if phase == 1:
        valid = 1
        input_value = 18446744073709551615
        input_remaining = 9223372036854775807
        expected_ready = 1
    if phase == 2:
        valid = 1
        input_value = 18446744073709551614
        input_remaining = 12297829382473034410
        expected_ready = 1
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 3:
        valid = 1
        input_value = 9223372036854775808
        input_remaining = 6148914691236517205
        expected_ready = 1
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 4:
        valid = 1
        input_value = 9223372036854775807
        input_remaining = 1
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 5:
        valid = 1
        input_value = 12297829382473034410
        input_remaining = 2
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 6:
        valid = 1
        input_value = 6148914691236517205
        input_remaining = 4
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 7:
        valid = 1
        input_value = 1
        input_remaining = 8
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 8:
        valid = 1
        input_value = 2
        input_remaining = 16
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 9:
        valid = 1
        input_value = 4
        input_remaining = 32
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 10:
        valid = 1
        input_value = 8
        input_remaining = 64
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 11:
        valid = 1
        input_value = 16
        input_remaining = 128
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 12:
        valid = 1
        input_value = 32
        input_remaining = 256
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 13:
        valid = 1
        take = 1
        input_value = 64
        input_remaining = 512
        expected_ready = 1
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 14:
        valid = 1
        take = 1
        input_value = 128
        input_remaining = 1024
        expected_ready = 1
        expected_valid = 1
        expected_remaining = 9223372036854775806
    if phase == 15:
        valid = 1
        take = 1
        input_value = 256
        input_remaining = 2048
        expected_ready = 1
        expected_valid = 1
        expected_value = 18446744073709551615
        expected_remaining = 12297829382473034409
    if phase == 16:
        valid = 1
        input_value = 512
        input_remaining = 4096
        expected_valid = 1
        expected_value = 9223372036854775809
        expected_remaining = 6148914691236517204
    if phase == 17:
        valid = 1
        take = 1
        input_value = 1024
        input_remaining = 8192
        expected_ready = 1
        expected_valid = 1
        expected_value = 9223372036854775809
        expected_remaining = 6148914691236517204
    if phase == 18:
        valid = 1
        take = 1
        input_value = 2048
        input_remaining = 16384
        expected_ready = 1
        expected_valid = 1
        expected_value = 65
        expected_remaining = 511
    if phase == 19:
        valid = 1
        take = 1
        input_value = 4096
        input_remaining = 32768
        expected_ready = 1
        expected_valid = 1
        expected_value = 129
        expected_remaining = 1023
    if phase == 20:
        valid = 1
        input_value = 8192
        input_remaining = 65536
        expected_valid = 1
        expected_value = 257
        expected_remaining = 2047
    if phase == 21:
        valid = 1
        take = 1
        input_value = 16384
        input_remaining = 131072
        expected_ready = 1
        expected_valid = 1
        expected_value = 257
        expected_remaining = 2047
    if phase == 22:
        valid = 1
        take = 1
        input_value = 32768
        input_remaining = 262144
        expected_ready = 1
        expected_valid = 1
        expected_value = 1025
        expected_remaining = 8191
    if phase == 23:
        valid = 1
        take = 1
        input_value = 65536
        input_remaining = 524288
        expected_ready = 1
        expected_valid = 1
        expected_value = 2049
        expected_remaining = 16383
    if phase == 24:
        valid = 1
        input_value = 131072
        input_remaining = 1048576
        expected_valid = 1
        expected_value = 4097
        expected_remaining = 32767
    if phase == 25:
        valid = 1
        take = 1
        input_value = 262144
        input_remaining = 2097152
        expected_ready = 1
        expected_valid = 1
        expected_value = 4097
        expected_remaining = 32767
    if phase == 26:
        valid = 1
        take = 1
        input_value = 524288
        input_remaining = 4194304
        expected_ready = 1
        expected_valid = 1
        expected_value = 16385
        expected_remaining = 131071
    if phase == 27:
        valid = 1
        take = 1
        input_value = 1048576
        input_remaining = 8388608
        expected_ready = 1
        expected_valid = 1
        expected_value = 32769
        expected_remaining = 262143
    if phase == 28:
        valid = 1
        input_value = 2097152
        input_remaining = 16777216
        expected_valid = 1
        expected_value = 65537
        expected_remaining = 524287
    if phase == 29:
        valid = 1
        take = 1
        input_value = 4194304
        input_remaining = 33554432
        expected_ready = 1
        expected_valid = 1
        expected_value = 65537
        expected_remaining = 524287
    if phase == 30:
        valid = 1
        take = 1
        input_value = 8388608
        input_remaining = 67108864
        expected_ready = 1
        expected_valid = 1
        expected_value = 262145
        expected_remaining = 2097151
    if phase == 31:
        take = 1
        input_value = 16777216
        input_remaining = 134217728
        expected_ready = 1
        expected_valid = 1
        expected_value = 524289
        expected_remaining = 4194303
    if phase == 32:
        valid = 1
        input_value = 33554432
        input_remaining = 268435456
        expected_ready = 1
        expected_valid = 1
        expected_value = 1048577
        expected_remaining = 8388607
    if phase == 33:
        valid = 1
        take = 1
        input_value = 67108864
        input_remaining = 536870912
        expected_ready = 1
        expected_valid = 1
        expected_value = 1048577
        expected_remaining = 8388607
    if phase == 34:
        valid = 1
        take = 1
        input_value = 134217728
        input_remaining = 1073741824
        expected_ready = 1
        expected_valid = 1
        expected_value = 4194305
        expected_remaining = 33554431
    if phase == 35:
        valid = 1
        take = 1
        input_value = 268435456
        input_remaining = 2147483648
        expected_ready = 1
        expected_valid = 1
        expected_value = 8388609
        expected_remaining = 67108863
    if phase == 36:
        input_value = 536870912
        input_remaining = 4294967296
        expected_valid = 1
        expected_value = 33554433
        expected_remaining = 268435455
    if phase == 37:
        valid = 1
        take = 1
        input_value = 1073741824
        input_remaining = 8589934592
        expected_ready = 1
        expected_valid = 1
        expected_value = 33554433
        expected_remaining = 268435455
    if phase == 38:
        valid = 1
        take = 1
        input_value = 2147483648
        input_remaining = 17179869184
        expected_ready = 1
        expected_valid = 1
        expected_value = 67108865
        expected_remaining = 536870911
    if phase == 39:
        valid = 1
        take = 1
        input_value = 4294967296
        input_remaining = 34359738368
        expected_ready = 1
        expected_valid = 1
        expected_value = 134217729
        expected_remaining = 1073741823
    if phase == 40:
        valid = 1
        input_value = 8589934592
        input_remaining = 68719476736
        expected_valid = 1
        expected_value = 268435457
        expected_remaining = 2147483647
    if phase == 41:
        take = 1
        input_value = 17179869184
        input_remaining = 137438953472
        expected_ready = 1
        expected_valid = 1
        expected_value = 268435457
        expected_remaining = 2147483647
    if phase == 42:
        valid = 1
        take = 1
        input_value = 34359738368
        input_remaining = 274877906944
        expected_ready = 1
        expected_valid = 1
        expected_value = 1073741825
        expected_remaining = 8589934591
    if phase == 43:
        valid = 1
        take = 1
        input_value = 68719476736
        input_remaining = 549755813888
        expected_ready = 1
        expected_valid = 1
        expected_value = 2147483649
        expected_remaining = 17179869183
    if phase == 44:
        valid = 1
        input_value = 137438953472
        input_remaining = 1099511627776
        expected_ready = 1
        expected_valid = 1
        expected_value = 4294967297
        expected_remaining = 34359738367
    if phase == 45:
        valid = 1
        take = 1
        input_value = 274877906944
        input_remaining = 2199023255552
        expected_ready = 1
        expected_valid = 1
        expected_value = 4294967297
        expected_remaining = 34359738367
    if phase == 46:
        take = 1
        input_value = 549755813888
        input_remaining = 4398046511104
        expected_ready = 1
        expected_valid = 1
        expected_value = 34359738369
        expected_remaining = 274877906943
    if phase == 47:
        valid = 1
        take = 1
        input_value = 1099511627776
        input_remaining = 8796093022208
        expected_ready = 1
        expected_valid = 1
        expected_value = 68719476737
        expected_remaining = 549755813887
    if phase == 48:
        valid = 1
        input_value = 2199023255552
        input_remaining = 17592186044416
        expected_ready = 1
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 49:
        valid = 1
        input_value = 4398046511104
        input_remaining = 35184372088832
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 50:
        valid = 1
        input_value = 8796093022208
        input_remaining = 70368744177664
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 51:
        input_value = 17592186044416
        input_remaining = 140737488355328
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 52:
        valid = 1
        input_value = 35184372088832
        input_remaining = 281474976710656
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 53:
        valid = 1
        input_value = 70368744177664
        input_remaining = 562949953421312
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 54:
        valid = 1
        input_value = 140737488355328
        input_remaining = 1125899906842624
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 55:
        valid = 1
        input_value = 281474976710656
        input_remaining = 2251799813685248
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 56:
        input_value = 562949953421312
        input_remaining = 4503599627370496
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 57:
        valid = 1
        input_value = 1125899906842624
        input_remaining = 9007199254740992
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 58:
        valid = 1
        input_value = 2251799813685248
        input_remaining = 18014398509481984
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 59:
        valid = 1
        input_value = 4503599627370496
        input_remaining = 36028797018963968
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 60:
        valid = 1
        input_value = 9007199254740992
        input_remaining = 72057594037927936
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 61:
        input_value = 18014398509481984
        input_remaining = 144115188075855872
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 62:
        valid = 1
        input_value = 36028797018963968
        input_remaining = 288230376151711744
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 63:
        valid = 1
        input_value = 72057594037927936
        input_remaining = 576460752303423488
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 64:
        valid = 1
        take = 1
        input_remaining = 9223372036854775808
        expected_ready = 1
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 65:
        valid = 1
        take = 1
        input_value = 18446744073709551615
        input_remaining = 9223372036854775807
        expected_ready = 1
        expected_valid = 1
        expected_value = 274877906945
        expected_remaining = 2199023255551
    if phase == 66:
        valid = 1
        take = 1
        input_value = 18446744073709551614
        input_remaining = 12297829382473034410
        expected_ready = 1
        expected_valid = 1
        expected_value = 1099511627777
        expected_remaining = 8796093022207
    if phase == 67:
        valid = 1
        take = 1
        input_value = 9223372036854775808
        input_remaining = 6148914691236517205
        expected_ready = 1
        expected_valid = 1
        expected_value = 2199023255553
        expected_remaining = 17592186044415
    if phase == 68:
        valid = 1
        take = 1
        input_value = 9223372036854775807
        input_remaining = 1
        expected_ready = 1
        expected_valid = 1
        expected_value = 1
        expected_remaining = 9223372036854775807
    if phase == 69:
        valid = 1
        take = 1
        input_value = 12297829382473034410
        input_remaining = 2
        expected_ready = 1
        expected_valid = 1
        expected_remaining = 9223372036854775806
    if phase == 70:
        valid = 1
        take = 1
        input_value = 6148914691236517205
        input_remaining = 4
        expected_ready = 1
        expected_valid = 1
        expected_value = 18446744073709551615
        expected_remaining = 12297829382473034409
    if phase == 71:
        valid = 1
        take = 1
        input_value = 1
        input_remaining = 8
        expected_ready = 1
        expected_valid = 1
        expected_value = 9223372036854775809
        expected_remaining = 6148914691236517204
    if phase == 72:
        valid = 1
        take = 1
        input_value = 2
        input_remaining = 16
        expected_ready = 1
        expected_valid = 1
        expected_value = 9223372036854775808
    if phase == 73:
        valid = 1
        take = 1
        input_value = 4
        input_remaining = 32
        expected_ready = 1
        expected_valid = 1
        expected_value = 12297829382473034411
        expected_remaining = 1
    if phase == 74:
        valid = 1
        take = 1
        input_value = 8
        input_remaining = 64
        expected_ready = 1
        expected_valid = 1
        expected_value = 6148914691236517206
        expected_remaining = 3
    if phase == 75:
        valid = 1
        take = 1
        input_value = 16
        input_remaining = 128
        expected_ready = 1
        expected_valid = 1
        expected_value = 2
        expected_remaining = 7
    if phase == 76:
        valid = 1
        take = 1
        input_value = 32
        input_remaining = 256
        expected_ready = 1
        expected_valid = 1
        expected_value = 3
        expected_remaining = 15
    if phase == 77:
        valid = 1
        take = 1
        input_value = 64
        input_remaining = 512
        expected_ready = 1
        expected_valid = 1
        expected_value = 5
        expected_remaining = 31
    if phase == 78:
        valid = 1
        take = 1
        input_value = 128
        input_remaining = 1024
        expected_ready = 1
        expected_valid = 1
        expected_value = 9
        expected_remaining = 63
    if phase == 79:
        valid = 1
        take = 1
        input_value = 256
        input_remaining = 2048
        expected_ready = 1
        expected_valid = 1
        expected_value = 17
        expected_remaining = 127
    if phase == 80:
        valid = 1
        take = 1
        input_value = 512
        input_remaining = 4096
        expected_ready = 1
        expected_valid = 1
        expected_value = 33
        expected_remaining = 255
    if phase == 81:
        valid = 1
        take = 1
        input_value = 1024
        input_remaining = 8192
        expected_ready = 1
        expected_valid = 1
        expected_value = 65
        expected_remaining = 511
    if phase == 82:
        valid = 1
        take = 1
        input_value = 2048
        input_remaining = 16384
        expected_ready = 1
        expected_valid = 1
        expected_value = 129
        expected_remaining = 1023
    if phase == 83:
        valid = 1
        take = 1
        input_value = 4096
        input_remaining = 32768
        expected_ready = 1
        expected_valid = 1
        expected_value = 257
        expected_remaining = 2047
    if phase == 84:
        valid = 1
        take = 1
        input_value = 8192
        input_remaining = 65536
        expected_ready = 1
        expected_valid = 1
        expected_value = 513
        expected_remaining = 4095
    if phase == 85:
        valid = 1
        take = 1
        input_value = 16384
        input_remaining = 131072
        expected_ready = 1
        expected_valid = 1
        expected_value = 1025
        expected_remaining = 8191
    if phase == 86:
        valid = 1
        take = 1
        input_value = 32768
        input_remaining = 262144
        expected_ready = 1
        expected_valid = 1
        expected_value = 2049
        expected_remaining = 16383
    if phase == 87:
        valid = 1
        take = 1
        input_value = 65536
        input_remaining = 524288
        expected_ready = 1
        expected_valid = 1
        expected_value = 4097
        expected_remaining = 32767
    if phase == 88:
        valid = 1
        take = 1
        input_value = 131072
        input_remaining = 1048576
        expected_ready = 1
        expected_valid = 1
        expected_value = 8193
        expected_remaining = 65535
    if phase == 89:
        valid = 1
        take = 1
        input_value = 262144
        input_remaining = 2097152
        expected_ready = 1
        expected_valid = 1
        expected_value = 16385
        expected_remaining = 131071
    if phase == 90:
        valid = 1
        take = 1
        input_value = 524288
        input_remaining = 4194304
        expected_ready = 1
        expected_valid = 1
        expected_value = 32769
        expected_remaining = 262143
    if phase == 91:
        valid = 1
        take = 1
        input_value = 1048576
        input_remaining = 8388608
        expected_ready = 1
        expected_valid = 1
        expected_value = 65537
        expected_remaining = 524287
    if phase == 92:
        valid = 1
        take = 1
        input_value = 2097152
        input_remaining = 16777216
        expected_ready = 1
        expected_valid = 1
        expected_value = 131073
        expected_remaining = 1048575
    if phase == 93:
        valid = 1
        take = 1
        input_value = 4194304
        input_remaining = 33554432
        expected_ready = 1
        expected_valid = 1
        expected_value = 262145
        expected_remaining = 2097151
    if phase == 94:
        valid = 1
        take = 1
        input_value = 8388608
        input_remaining = 67108864
        expected_ready = 1
        expected_valid = 1
        expected_value = 524289
        expected_remaining = 4194303
    if phase == 95:
        valid = 1
        take = 1
        input_value = 16777216
        input_remaining = 134217728
        expected_ready = 1
        expected_valid = 1
        expected_value = 1048577
        expected_remaining = 8388607
    if phase == 96:
        valid = 1
        take = 1
        input_value = 33554432
        input_remaining = 268435456
        expected_ready = 1
        expected_valid = 1
        expected_value = 2097153
        expected_remaining = 16777215
    if phase == 97:
        valid = 1
        take = 1
        input_value = 67108864
        input_remaining = 536870912
        expected_ready = 1
        expected_valid = 1
        expected_value = 4194305
        expected_remaining = 33554431
    if phase == 98:
        valid = 1
        take = 1
        input_value = 134217728
        input_remaining = 1073741824
        expected_ready = 1
        expected_valid = 1
        expected_value = 8388609
        expected_remaining = 67108863
    if phase == 99:
        valid = 1
        take = 1
        input_value = 268435456
        input_remaining = 2147483648
        expected_ready = 1
        expected_valid = 1
        expected_value = 16777217
        expected_remaining = 134217727
    if phase == 100:
        valid = 1
        take = 1
        input_value = 536870912
        input_remaining = 4294967296
        expected_ready = 1
        expected_valid = 1
        expected_value = 33554433
        expected_remaining = 268435455
    if phase == 101:
        valid = 1
        take = 1
        input_value = 1073741824
        input_remaining = 8589934592
        expected_ready = 1
        expected_valid = 1
        expected_value = 67108865
        expected_remaining = 536870911
    if phase == 102:
        valid = 1
        take = 1
        input_value = 2147483648
        input_remaining = 17179869184
        expected_ready = 1
        expected_valid = 1
        expected_value = 134217729
        expected_remaining = 1073741823
    if phase == 103:
        valid = 1
        take = 1
        input_value = 4294967296
        input_remaining = 34359738368
        expected_ready = 1
        expected_valid = 1
        expected_value = 268435457
        expected_remaining = 2147483647
    if phase == 104:
        valid = 1
        take = 1
        input_value = 8589934592
        input_remaining = 68719476736
        expected_ready = 1
        expected_valid = 1
        expected_value = 536870913
        expected_remaining = 4294967295
    if phase == 105:
        valid = 1
        take = 1
        input_value = 17179869184
        input_remaining = 137438953472
        expected_ready = 1
        expected_valid = 1
        expected_value = 1073741825
        expected_remaining = 8589934591
    if phase == 106:
        valid = 1
        take = 1
        input_value = 34359738368
        input_remaining = 274877906944
        expected_ready = 1
        expected_valid = 1
        expected_value = 2147483649
        expected_remaining = 17179869183
    if phase == 107:
        valid = 1
        take = 1
        input_value = 68719476736
        input_remaining = 549755813888
        expected_ready = 1
        expected_valid = 1
        expected_value = 4294967297
        expected_remaining = 34359738367
    if phase == 108:
        valid = 1
        take = 1
        input_value = 137438953472
        input_remaining = 1099511627776
        expected_ready = 1
        expected_valid = 1
        expected_value = 8589934593
        expected_remaining = 68719476735
    if phase == 109:
        valid = 1
        take = 1
        input_value = 274877906944
        input_remaining = 2199023255552
        expected_ready = 1
        expected_valid = 1
        expected_value = 17179869185
        expected_remaining = 137438953471
    if phase == 110:
        valid = 1
        take = 1
        input_value = 549755813888
        input_remaining = 4398046511104
        expected_ready = 1
        expected_valid = 1
        expected_value = 34359738369
        expected_remaining = 274877906943
    if phase == 111:
        valid = 1
        take = 1
        input_value = 1099511627776
        input_remaining = 8796093022208
        expected_ready = 1
        expected_valid = 1
        expected_value = 68719476737
        expected_remaining = 549755813887
    if phase == 112:
        valid = 1
        take = 1
        input_value = 2199023255552
        input_remaining = 17592186044416
        expected_ready = 1
        expected_valid = 1
        expected_value = 137438953473
        expected_remaining = 1099511627775
    if phase == 113:
        valid = 1
        take = 1
        input_value = 4398046511104
        input_remaining = 35184372088832
        expected_ready = 1
        expected_valid = 1
        expected_value = 274877906945
        expected_remaining = 2199023255551
    if phase == 114:
        valid = 1
        take = 1
        input_value = 8796093022208
        input_remaining = 70368744177664
        expected_ready = 1
        expected_valid = 1
        expected_value = 549755813889
        expected_remaining = 4398046511103
    if phase == 115:
        valid = 1
        take = 1
        input_value = 17592186044416
        input_remaining = 140737488355328
        expected_ready = 1
        expected_valid = 1
        expected_value = 1099511627777
        expected_remaining = 8796093022207
    if phase == 116:
        valid = 1
        take = 1
        input_value = 35184372088832
        input_remaining = 281474976710656
        expected_ready = 1
        expected_valid = 1
        expected_value = 2199023255553
        expected_remaining = 17592186044415
    if phase == 117:
        valid = 1
        take = 1
        input_value = 70368744177664
        input_remaining = 562949953421312
        expected_ready = 1
        expected_valid = 1
        expected_value = 4398046511105
        expected_remaining = 35184372088831
    if phase == 118:
        valid = 1
        take = 1
        input_value = 140737488355328
        input_remaining = 1125899906842624
        expected_ready = 1
        expected_valid = 1
        expected_value = 8796093022209
        expected_remaining = 70368744177663
    if phase == 119:
        valid = 1
        take = 1
        input_value = 281474976710656
        input_remaining = 2251799813685248
        expected_ready = 1
        expected_valid = 1
        expected_value = 17592186044417
        expected_remaining = 140737488355327
    if phase == 120:
        valid = 1
        take = 1
        input_value = 562949953421312
        input_remaining = 4503599627370496
        expected_ready = 1
        expected_valid = 1
        expected_value = 35184372088833
        expected_remaining = 281474976710655
    if phase == 121:
        valid = 1
        take = 1
        input_value = 1125899906842624
        input_remaining = 9007199254740992
        expected_ready = 1
        expected_valid = 1
        expected_value = 70368744177665
        expected_remaining = 562949953421311
    if phase == 122:
        valid = 1
        take = 1
        input_value = 2251799813685248
        input_remaining = 18014398509481984
        expected_ready = 1
        expected_valid = 1
        expected_value = 140737488355329
        expected_remaining = 1125899906842623
    if phase == 123:
        valid = 1
        take = 1
        input_value = 4503599627370496
        input_remaining = 36028797018963968
        expected_ready = 1
        expected_valid = 1
        expected_value = 281474976710657
        expected_remaining = 2251799813685247
    if phase == 124:
        valid = 1
        take = 1
        input_value = 9007199254740992
        input_remaining = 72057594037927936
        expected_ready = 1
        expected_valid = 1
        expected_value = 562949953421313
        expected_remaining = 4503599627370495
    if phase == 125:
        valid = 1
        take = 1
        input_value = 18014398509481984
        input_remaining = 144115188075855872
        expected_ready = 1
        expected_valid = 1
        expected_value = 1125899906842625
        expected_remaining = 9007199254740991
    if phase == 126:
        valid = 1
        take = 1
        input_value = 36028797018963968
        input_remaining = 288230376151711744
        expected_ready = 1
        expected_valid = 1
        expected_value = 2251799813685249
        expected_remaining = 18014398509481983
    if phase == 127:
        valid = 1
        take = 1
        input_value = 72057594037927936
        input_remaining = 576460752303423488
        expected_ready = 1
        expected_valid = 1
        expected_value = 4503599627370497
        expected_remaining = 36028797018963967
    if phase == 128:
        valid = 1
        take = 1
        input_value = 144115188075855872
        input_remaining = 1152921504606846976
        expected_ready = 1
        expected_valid = 1
        expected_value = 9007199254740993
        expected_remaining = 72057594037927935
    if phase == 129:
        valid = 1
        take = 1
        input_value = 288230376151711744
        input_remaining = 2305843009213693952
        expected_ready = 1
        expected_valid = 1
        expected_value = 18014398509481985
        expected_remaining = 144115188075855871
    if phase == 130:
        valid = 1
        take = 1
        input_value = 576460752303423488
        input_remaining = 4611686018427387904
        expected_ready = 1
        expected_valid = 1
        expected_value = 36028797018963969
        expected_remaining = 288230376151711743
    if phase == 131:
        valid = 1
        take = 1
        input_value = 1152921504606846976
        input_remaining = 9223372036854775808
        expected_ready = 1
        expected_valid = 1
        expected_value = 72057594037927937
        expected_remaining = 576460752303423487
    if phase == 132:
        valid = 1
        take = 1
        input_value = 2305843009213693952
        input_remaining = 1
        expected_ready = 1
        expected_valid = 1
        expected_value = 144115188075855873
        expected_remaining = 1152921504606846975
    if phase == 133:
        valid = 1
        take = 1
        input_value = 4611686018427387904
        input_remaining = 3
        expected_ready = 1
        expected_valid = 1
        expected_value = 288230376151711745
        expected_remaining = 2305843009213693951
    if phase == 134:
        valid = 1
        take = 1
        input_value = 9223372036854775808
        input_remaining = 7
        expected_ready = 1
        expected_valid = 1
        expected_value = 576460752303423489
        expected_remaining = 4611686018427387903
    if phase == 135:
        valid = 1
        take = 1
        input_value = 1
        input_remaining = 15
        expected_ready = 1
        expected_valid = 1
        expected_value = 1152921504606846977
        expected_remaining = 9223372036854775807
    if phase == 136:
        valid = 1
        take = 1
        input_value = 3
        input_remaining = 31
        expected_ready = 1
        expected_valid = 1
        expected_value = 2305843009213693953
    if phase == 137:
        valid = 1
        take = 1
        input_value = 7
        input_remaining = 63
        expected_ready = 1
        expected_valid = 1
        expected_value = 4611686018427387905
        expected_remaining = 2
    if phase == 138:
        valid = 1
        take = 1
        input_value = 15
        input_remaining = 127
        expected_ready = 1
        expected_valid = 1
        expected_value = 9223372036854775809
        expected_remaining = 6
    if phase == 139:
        valid = 1
        take = 1
        input_value = 31
        input_remaining = 255
        expected_ready = 1
        expected_valid = 1
        expected_value = 2
        expected_remaining = 14
    if phase == 140:
        valid = 1
        take = 1
        input_value = 63
        input_remaining = 511
        expected_ready = 1
        expected_valid = 1
        expected_value = 4
        expected_remaining = 30
    if phase == 141:
        valid = 1
        take = 1
        input_value = 127
        input_remaining = 1023
        expected_ready = 1
        expected_valid = 1
        expected_value = 8
        expected_remaining = 62
    if phase == 142:
        valid = 1
        take = 1
        input_value = 255
        input_remaining = 2047
        expected_ready = 1
        expected_valid = 1
        expected_value = 16
        expected_remaining = 126
    if phase == 143:
        valid = 1
        take = 1
        input_value = 511
        input_remaining = 4095
        expected_ready = 1
        expected_valid = 1
        expected_value = 32
        expected_remaining = 254
    if phase == 144:
        valid = 1
        take = 1
        input_value = 1023
        input_remaining = 8191
        expected_ready = 1
        expected_valid = 1
        expected_value = 64
        expected_remaining = 510
    if phase == 145:
        valid = 1
        take = 1
        input_value = 2047
        input_remaining = 16383
        expected_ready = 1
        expected_valid = 1
        expected_value = 128
        expected_remaining = 1022
    if phase == 146:
        valid = 1
        take = 1
        input_value = 4095
        input_remaining = 32767
        expected_ready = 1
        expected_valid = 1
        expected_value = 256
        expected_remaining = 2046
    if phase == 147:
        valid = 1
        take = 1
        input_value = 8191
        input_remaining = 65535
        expected_ready = 1
        expected_valid = 1
        expected_value = 512
        expected_remaining = 4094
    if phase == 148:
        valid = 1
        take = 1
        input_value = 16383
        input_remaining = 131071
        expected_ready = 1
        expected_valid = 1
        expected_value = 1024
        expected_remaining = 8190
    if phase == 149:
        valid = 1
        take = 1
        input_value = 32767
        input_remaining = 262143
        expected_ready = 1
        expected_valid = 1
        expected_value = 2048
        expected_remaining = 16382
    if phase == 150:
        valid = 1
        take = 1
        input_value = 65535
        input_remaining = 524287
        expected_ready = 1
        expected_valid = 1
        expected_value = 4096
        expected_remaining = 32766
    if phase == 151:
        valid = 1
        take = 1
        input_value = 131071
        input_remaining = 1048575
        expected_ready = 1
        expected_valid = 1
        expected_value = 8192
        expected_remaining = 65534
    if phase == 152:
        valid = 1
        take = 1
        input_value = 262143
        input_remaining = 2097151
        expected_ready = 1
        expected_valid = 1
        expected_value = 16384
        expected_remaining = 131070
    if phase == 153:
        valid = 1
        take = 1
        input_value = 524287
        input_remaining = 4194303
        expected_ready = 1
        expected_valid = 1
        expected_value = 32768
        expected_remaining = 262142
    if phase == 154:
        valid = 1
        take = 1
        input_value = 1048575
        input_remaining = 8388607
        expected_ready = 1
        expected_valid = 1
        expected_value = 65536
        expected_remaining = 524286
    if phase == 155:
        valid = 1
        take = 1
        input_value = 2097151
        input_remaining = 16777215
        expected_ready = 1
        expected_valid = 1
        expected_value = 131072
        expected_remaining = 1048574
    if phase == 156:
        valid = 1
        take = 1
        input_value = 4194303
        input_remaining = 33554431
        expected_ready = 1
        expected_valid = 1
        expected_value = 262144
        expected_remaining = 2097150
    if phase == 157:
        valid = 1
        take = 1
        input_value = 8388607
        input_remaining = 67108863
        expected_ready = 1
        expected_valid = 1
        expected_value = 524288
        expected_remaining = 4194302
    if phase == 158:
        valid = 1
        take = 1
        input_value = 16777215
        input_remaining = 134217727
        expected_ready = 1
        expected_valid = 1
        expected_value = 1048576
        expected_remaining = 8388606
    if phase == 159:
        valid = 1
        take = 1
        input_value = 33554431
        input_remaining = 268435455
        expected_ready = 1
        expected_valid = 1
        expected_value = 2097152
        expected_remaining = 16777214
    if phase == 160:
        take = 1
        input_value = 67108863
        input_remaining = 536870911
        expected_ready = 1
        expected_valid = 1
        expected_value = 4194304
        expected_remaining = 33554430
    if phase == 161:
        take = 1
        input_value = 134217727
        input_remaining = 1073741823
        expected_ready = 1
        expected_valid = 1
        expected_value = 8388608
        expected_remaining = 67108862
    if phase == 162:
        take = 1
        input_value = 268435455
        input_remaining = 2147483647
        expected_ready = 1
        expected_valid = 1
        expected_value = 16777216
        expected_remaining = 134217726
    if phase == 163:
        take = 1
        input_value = 536870911
        input_remaining = 4294967295
        expected_ready = 1
        expected_valid = 1
        expected_value = 33554432
        expected_remaining = 268435454
    if phase == 164:
        take = 1
        input_value = 1073741823
        input_remaining = 8589934591
        expected_ready = 1
    if phase == 165:
        take = 1
        input_value = 2147483647
        input_remaining = 17179869183
        expected_ready = 1
    if phase == 166:
        take = 1
        input_value = 4294967295
        input_remaining = 34359738367
        expected_ready = 1
    if phase == 167:
        take = 1
        input_value = 8589934591
        input_remaining = 68719476735
        expected_ready = 1
    if phase == 168:
        take = 1
        input_value = 17179869183
        input_remaining = 137438953471
        expected_ready = 1
    if phase == 169:
        take = 1
        input_value = 34359738367
        input_remaining = 274877906943
        expected_ready = 1
    if phase == 170:
        take = 1
        input_value = 68719476735
        input_remaining = 549755813887
        expected_ready = 1
    if phase == 171:
        take = 1
        input_value = 137438953471
        input_remaining = 1099511627775
        expected_ready = 1
    if phase == 172:
        take = 1
        input_value = 274877906943
        input_remaining = 2199023255551
        expected_ready = 1
    if phase == 173:
        take = 1
        input_value = 549755813887
        input_remaining = 4398046511103
        expected_ready = 1
    if phase == 174:
        take = 1
        input_value = 1099511627775
        input_remaining = 8796093022207
        expected_ready = 1
    if phase == 175:
        take = 1
        input_value = 2199023255551
        input_remaining = 17592186044415
        expected_ready = 1
    if phase == 176:
        take = 1
        input_value = 4398046511103
        input_remaining = 35184372088831
        expected_ready = 1
    if phase == 177:
        take = 1
        input_value = 8796093022207
        input_remaining = 70368744177663
        expected_ready = 1
    if phase == 178:
        take = 1
        input_value = 17592186044415
        input_remaining = 140737488355327
        expected_ready = 1
    if phase == 179:
        take = 1
        input_value = 35184372088831
        input_remaining = 281474976710655
        expected_ready = 1
    if phase == 180:
        take = 1
        input_value = 70368744177663
        input_remaining = 562949953421311
        expected_ready = 1
    if phase == 181:
        take = 1
        input_value = 140737488355327
        input_remaining = 1125899906842623
        expected_ready = 1
    if phase == 182:
        take = 1
        input_value = 281474976710655
        input_remaining = 2251799813685247
        expected_ready = 1
    if phase == 183:
        take = 1
        input_value = 562949953421311
        input_remaining = 4503599627370495
        expected_ready = 1
    return Stimulus(
        valid=valid,
        take=take,
        input_value=input_value,
        input_remaining=input_remaining,
        expected_ready=expected_ready,
        expected_valid=expected_valid,
        expected_value=expected_value,
        expected_remaining=expected_remaining,
    )


@rule
def advance(phase):
    if phase < 183:
        phase = phase + 1


@system
def StructPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Item(value=frame.input_value, remaining=frame.input_remaining)
    dut = StructPipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "struct_pipeline input capacity"
        assert dut.valid == frame.expected_valid, "struct_pipeline result availability"
        assert (
            dut.data.value == frame.expected_value
        ), "struct_pipeline value old-state check"
        assert (
            dut.data.remaining == frame.expected_remaining
        ), "struct_pipeline remaining old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "value", dut.data.value)
        log("info", "remaining", dut.data.remaining)

    advance(phase)
    check()

"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_conditional_pipeline.conditional_pipeline import (
    ConditionalPipeline,
    Item,
    ConditionalResult,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_value: bits[32]
    input_route: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_value: bits[32]
    expected_data_route: bits[1]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    take: bits[1] = 0
    input_value: bits[32] = 0
    input_route: bits[1] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_data_value: bits[32] = 0
    expected_data_route: bits[1] = 0
    if phase == 0:
        valid = 1
        input_route = 1
        expected_ready = 1
    if phase == 1:
        valid = 1
        input_value = 4294967295
        expected_ready = 1
    if phase == 2:
        valid = 1
        input_value = 4294967294
        input_route = 1
        expected_ready = 1
    if phase == 3:
        valid = 1
        input_value = 2147483648
        expected_ready = 1
    if phase == 4:
        valid = 1
        input_value = 2147483647
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 5:
        valid = 1
        input_value = 2863311530
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 6:
        valid = 1
        input_value = 1431655765
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 7:
        valid = 1
        input_value = 1
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 8:
        valid = 1
        input_value = 2
        input_route = 1
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 9:
        valid = 1
        input_value = 4
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 10:
        valid = 1
        input_value = 8
        input_route = 1
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 11:
        valid = 1
        input_value = 16
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 12:
        valid = 1
        input_value = 32
        input_route = 1
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 13:
        valid = 1
        take = 1
        input_value = 64
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 14:
        valid = 1
        take = 1
        input_value = 128
        input_route = 1
        expected_valid = 1
        expected_data_value = 18
        expected_data_route = 1
    if phase == 15:
        valid = 1
        take = 1
        input_value = 256
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2147483667
        expected_data_route = 1
    if phase == 16:
        valid = 1
        input_value = 512
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 9
    if phase == 17:
        valid = 1
        take = 1
        input_value = 1024
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 9
    if phase == 18:
        valid = 1
        take = 1
        input_value = 2048
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2147483658
    if phase == 19:
        valid = 1
        take = 1
        input_value = 4096
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1431655785
        expected_data_route = 1
    if phase == 20:
        valid = 1
        input_value = 8192
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2863311540
    if phase == 21:
        valid = 1
        take = 1
        input_value = 16384
        expected_valid = 1
        expected_data_value = 2863311540
    if phase == 22:
        valid = 1
        take = 1
        input_value = 32768
        input_route = 1
        expected_valid = 1
        expected_data_value = 532
        expected_data_route = 1
    if phase == 23:
        valid = 1
        take = 1
        input_value = 65536
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2068
        expected_data_route = 1
    if phase == 24:
        valid = 1
        input_value = 131072
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 266
    if phase == 25:
        valid = 1
        take = 1
        input_value = 262144
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 266
    if phase == 26:
        valid = 1
        take = 1
        input_value = 524288
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1034
    if phase == 27:
        valid = 1
        take = 1
        input_value = 1048576
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8212
        expected_data_route = 1
    if phase == 28:
        valid = 1
        input_value = 2097152
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4106
    if phase == 29:
        valid = 1
        take = 1
        input_value = 4194304
        expected_valid = 1
        expected_data_value = 4106
    if phase == 30:
        valid = 1
        take = 1
        input_value = 8388608
        input_route = 1
        expected_valid = 1
        expected_data_value = 131092
        expected_data_route = 1
    if phase == 31:
        take = 1
        input_value = 16777216
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 524308
        expected_data_route = 1
    if phase == 32:
        valid = 1
        input_value = 33554432
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 65546
    if phase == 33:
        valid = 1
        take = 1
        input_value = 67108864
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 65546
    if phase == 34:
        valid = 1
        take = 1
        input_value = 134217728
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 262154
    if phase == 35:
        valid = 1
        take = 1
        input_value = 268435456
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2097172
        expected_data_route = 1
    if phase == 36:
        input_value = 536870912
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 33554452
        expected_data_route = 1
    if phase == 37:
        valid = 1
        take = 1
        input_value = 1073741824
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 33554452
        expected_data_route = 1
    if phase == 38:
        valid = 1
        take = 1
        input_value = 2147483648
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 134217748
        expected_data_route = 1
    if phase == 39:
        valid = 1
        take = 1
        input_value = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1048586
    if phase == 40:
        valid = 1
        input_value = 3
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 67108874
    if phase == 41:
        take = 1
        input_value = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 67108874
    if phase == 42:
        valid = 1
        take = 1
        input_value = 15
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 268435466
    if phase == 43:
        valid = 1
        take = 1
        input_value = 31
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2147483668
        expected_data_route = 1
    if phase == 44:
        valid = 1
        input_value = 63
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1073741834
    if phase == 45:
        valid = 1
        take = 1
        input_value = 127
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1073741834
    if phase == 46:
        take = 1
        input_value = 255
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 23
        expected_data_route = 1
    if phase == 47:
        valid = 1
        take = 1
        input_value = 511
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 35
        expected_data_route = 1
    if phase == 48:
        valid = 1
        input_value = 1023
        input_route = 1
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 49:
        valid = 1
        input_value = 2047
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 50:
        valid = 1
        input_value = 4095
        input_route = 1
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 51:
        input_value = 8191
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 52:
        valid = 1
        input_value = 16383
        input_route = 1
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 53:
        valid = 1
        input_value = 32767
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 54:
        valid = 1
        input_value = 65535
        input_route = 1
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 55:
        valid = 1
        input_value = 131071
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 56:
        input_value = 262143
        input_route = 1
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 57:
        valid = 1
        input_value = 524287
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 58:
        valid = 1
        input_value = 1048575
        input_route = 1
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 59:
        valid = 1
        input_value = 2097151
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 60:
        valid = 1
        input_value = 4194303
        input_route = 1
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 61:
        input_value = 8388607
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 62:
        valid = 1
        input_value = 16777215
        input_route = 1
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 63:
        valid = 1
        input_value = 33554431
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 64:
        valid = 1
        take = 1
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 65:
        valid = 1
        take = 1
        input_value = 4294967295
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 11
    if phase == 66:
        valid = 1
        take = 1
        input_value = 4294967294
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 41
    if phase == 67:
        valid = 1
        take = 1
        input_value = 2147483648
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 137
    if phase == 68:
        valid = 1
        take = 1
        input_value = 2147483647
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 521
    if phase == 69:
        valid = 1
        take = 1
        input_value = 2863311530
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 20
        expected_data_route = 1
    if phase == 70:
        valid = 1
        take = 1
        input_value = 1431655765
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 9
    if phase == 71:
        valid = 1
        take = 1
        input_value = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 18
        expected_data_route = 1
    if phase == 72:
        valid = 1
        take = 1
        input_value = 2
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2147483658
    if phase == 73:
        valid = 1
        take = 1
        input_value = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2147483667
        expected_data_route = 1
    if phase == 74:
        valid = 1
        take = 1
        input_value = 8
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2863311540
    if phase == 75:
        valid = 1
        take = 1
        input_value = 16
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1431655785
        expected_data_route = 1
    if phase == 76:
        valid = 1
        take = 1
        input_value = 32
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 11
    if phase == 77:
        valid = 1
        take = 1
        input_value = 64
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 22
        expected_data_route = 1
    if phase == 78:
        valid = 1
        take = 1
        input_value = 128
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 14
    if phase == 79:
        valid = 1
        take = 1
        input_value = 256
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 28
        expected_data_route = 1
    if phase == 80:
        valid = 1
        take = 1
        input_value = 512
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 26
    if phase == 81:
        valid = 1
        take = 1
        input_value = 1024
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 52
        expected_data_route = 1
    if phase == 82:
        valid = 1
        take = 1
        input_value = 2048
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 74
    if phase == 83:
        valid = 1
        take = 1
        input_value = 4096
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 148
        expected_data_route = 1
    if phase == 84:
        valid = 1
        take = 1
        input_value = 8192
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 266
    if phase == 85:
        valid = 1
        take = 1
        input_value = 16384
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 532
        expected_data_route = 1
    if phase == 86:
        valid = 1
        take = 1
        input_value = 32768
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1034
    if phase == 87:
        valid = 1
        take = 1
        input_value = 65536
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2068
        expected_data_route = 1
    if phase == 88:
        valid = 1
        take = 1
        input_value = 131072
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4106
    if phase == 89:
        valid = 1
        take = 1
        input_value = 262144
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8212
        expected_data_route = 1
    if phase == 90:
        valid = 1
        take = 1
        input_value = 524288
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 16394
    if phase == 91:
        valid = 1
        take = 1
        input_value = 1048576
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 32788
        expected_data_route = 1
    if phase == 92:
        valid = 1
        take = 1
        input_value = 2097152
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 65546
    if phase == 93:
        valid = 1
        take = 1
        input_value = 4194304
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 131092
        expected_data_route = 1
    if phase == 94:
        valid = 1
        take = 1
        input_value = 8388608
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 262154
    if phase == 95:
        valid = 1
        take = 1
        input_value = 16777216
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 524308
        expected_data_route = 1
    if phase == 96:
        valid = 1
        take = 1
        input_value = 33554432
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1048586
    if phase == 97:
        valid = 1
        take = 1
        input_value = 67108864
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2097172
        expected_data_route = 1
    if phase == 98:
        valid = 1
        take = 1
        input_value = 134217728
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4194314
    if phase == 99:
        valid = 1
        take = 1
        input_value = 268435456
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8388628
        expected_data_route = 1
    if phase == 100:
        valid = 1
        take = 1
        input_value = 536870912
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 16777226
    if phase == 101:
        valid = 1
        take = 1
        input_value = 1073741824
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 33554452
        expected_data_route = 1
    if phase == 102:
        valid = 1
        take = 1
        input_value = 2147483648
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 67108874
    if phase == 103:
        valid = 1
        take = 1
        input_value = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 134217748
        expected_data_route = 1
    if phase == 104:
        valid = 1
        take = 1
        input_value = 3
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 268435466
    if phase == 105:
        valid = 1
        take = 1
        input_value = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 536870932
        expected_data_route = 1
    if phase == 106:
        valid = 1
        take = 1
        input_value = 15
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1073741834
    if phase == 107:
        valid = 1
        take = 1
        input_value = 31
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2147483668
        expected_data_route = 1
    if phase == 108:
        valid = 1
        take = 1
        input_value = 63
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 11
    if phase == 109:
        valid = 1
        take = 1
        input_value = 127
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 23
        expected_data_route = 1
    if phase == 110:
        valid = 1
        take = 1
        input_value = 255
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 17
    if phase == 111:
        valid = 1
        take = 1
        input_value = 511
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 35
        expected_data_route = 1
    if phase == 112:
        valid = 1
        take = 1
        input_value = 1023
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 41
    if phase == 113:
        valid = 1
        take = 1
        input_value = 2047
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 83
        expected_data_route = 1
    if phase == 114:
        valid = 1
        take = 1
        input_value = 4095
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 137
    if phase == 115:
        valid = 1
        take = 1
        input_value = 8191
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 275
        expected_data_route = 1
    if phase == 116:
        valid = 1
        take = 1
        input_value = 16383
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 521
    if phase == 117:
        valid = 1
        take = 1
        input_value = 32767
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1043
        expected_data_route = 1
    if phase == 118:
        valid = 1
        take = 1
        input_value = 65535
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2057
    if phase == 119:
        valid = 1
        take = 1
        input_value = 131071
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4115
        expected_data_route = 1
    if phase == 120:
        valid = 1
        take = 1
        input_value = 262143
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8201
    if phase == 121:
        valid = 1
        take = 1
        input_value = 524287
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 16403
        expected_data_route = 1
    if phase == 122:
        valid = 1
        take = 1
        input_value = 1048575
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 32777
    if phase == 123:
        valid = 1
        take = 1
        input_value = 2097151
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 65555
        expected_data_route = 1
    if phase == 124:
        valid = 1
        take = 1
        input_value = 4194303
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 131081
    if phase == 125:
        valid = 1
        take = 1
        input_value = 8388607
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 262163
        expected_data_route = 1
    if phase == 126:
        valid = 1
        take = 1
        input_value = 16777215
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 524297
    if phase == 127:
        valid = 1
        take = 1
        input_value = 33554431
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1048595
        expected_data_route = 1
    if phase == 128:
        valid = 1
        take = 1
        input_value = 67108863
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2097161
    if phase == 129:
        valid = 1
        take = 1
        input_value = 134217727
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4194323
        expected_data_route = 1
    if phase == 130:
        valid = 1
        take = 1
        input_value = 268435455
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8388617
    if phase == 131:
        valid = 1
        take = 1
        input_value = 536870911
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 16777235
        expected_data_route = 1
    if phase == 132:
        valid = 1
        take = 1
        input_value = 1073741823
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 33554441
    if phase == 133:
        valid = 1
        take = 1
        input_value = 2147483647
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 67108883
        expected_data_route = 1
    if phase == 134:
        valid = 1
        take = 1
        input_value = 4294967295
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 134217737
    if phase == 135:
        valid = 1
        take = 1
        input_value = 2991959596
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 268435475
        expected_data_route = 1
    if phase == 136:
        valid = 1
        take = 1
        input_value = 856644119
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 536870921
    if phase == 137:
        valid = 1
        take = 1
        input_value = 3015759362
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1073741843
        expected_data_route = 1
    if phase == 138:
        valid = 1
        take = 1
        input_value = 880439789
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2147483657
    if phase == 139:
        valid = 1
        take = 1
        input_value = 3039821272
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 19
        expected_data_route = 1
    if phase == 140:
        valid = 1
        take = 1
        input_value = 903956931
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2991959606
    if phase == 141:
        valid = 1
        take = 1
        input_value = 3063596462
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 856644139
        expected_data_route = 1
    if phase == 142:
        valid = 1
        take = 1
        input_value = 927744409
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3015759372
    if phase == 143:
        valid = 1
        take = 1
        input_value = 3087125892
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 880439809
        expected_data_route = 1
    if phase == 144:
        valid = 1
        take = 1
        input_value = 951826799
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3039821282
    if phase == 145:
        valid = 1
        take = 1
        input_value = 3110876506
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 903956951
        expected_data_route = 1
    if phase == 146:
        valid = 1
        take = 1
        input_value = 975622469
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3063596472
    if phase == 147:
        valid = 1
        take = 1
        input_value = 3134938416
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 927744429
        expected_data_route = 1
    if phase == 148:
        valid = 1
        take = 1
        input_value = 999155995
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3087125902
    if phase == 149:
        valid = 1
        take = 1
        input_value = 3158729990
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 951826819
        expected_data_route = 1
    if phase == 150:
        valid = 1
        take = 1
        input_value = 1022943473
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3110876516
    if phase == 151:
        valid = 1
        take = 1
        input_value = 3182259420
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 975622489
        expected_data_route = 1
    if phase == 152:
        valid = 1
        take = 1
        input_value = 1046976711
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3134938426
    if phase == 153:
        valid = 1
        take = 1
        input_value = 3206026418
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 999156015
        expected_data_route = 1
    if phase == 154:
        valid = 1
        take = 1
        input_value = 1070772381
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3158730000
    if phase == 155:
        valid = 1
        take = 1
        input_value = 3230088328
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1022943493
        expected_data_route = 1
    if phase == 156:
        valid = 1
        take = 1
        input_value = 1094289523
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3182259430
    if phase == 157:
        valid = 1
        take = 1
        input_value = 3253863518
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1046976731
        expected_data_route = 1
    if phase == 158:
        valid = 1
        take = 1
        input_value = 1118077001
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3206026428
    if phase == 159:
        valid = 1
        take = 1
        input_value = 3277392948
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1070772401
        expected_data_route = 1
    if phase == 160:
        take = 1
        input_value = 1142061087
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3230088338
    if phase == 161:
        take = 1
        input_value = 3301307402
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1094289543
        expected_data_route = 1
    if phase == 162:
        take = 1
        input_value = 1165979637
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3253863528
    if phase == 163:
        take = 1
        input_value = 3325107168
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1118077021
        expected_data_route = 1
    if phase == 164:
        take = 1
        input_value = 1189390283
        input_route = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3277392958
    if phase == 165:
        take = 1
        input_value = 3349029814
        expected_ready = 1
    if phase == 166:
        take = 1
        input_value = 1213317025
        input_route = 1
        expected_ready = 1
    if phase == 167:
        take = 1
        input_value = 3372952460
        expected_ready = 1
    if phase == 168:
        take = 1
        input_value = 1237243767
        input_route = 1
        expected_ready = 1
    if phase == 169:
        take = 1
        input_value = 3396359010
        expected_ready = 1
    if phase == 170:
        take = 1
        input_value = 1261162317
        input_route = 1
        expected_ready = 1
    if phase == 171:
        take = 1
        input_value = 3420289848
        expected_ready = 1
    if phase == 172:
        take = 1
        input_value = 1284556579
        input_route = 1
        expected_ready = 1
    if phase == 173:
        take = 1
        input_value = 3444196110
        expected_ready = 1
    if phase == 174:
        take = 1
        input_value = 1308483321
        input_route = 1
        expected_ready = 1
    if phase == 175:
        take = 1
        input_value = 3468118756
        expected_ready = 1
    if phase == 176:
        take = 1
        input_value = 1332360911
        input_route = 1
        expected_ready = 1
    if phase == 177:
        take = 1
        input_value = 3491541690
        expected_ready = 1
    if phase == 178:
        take = 1
        input_value = 1356279461
        input_route = 1
        expected_ready = 1
    if phase == 179:
        take = 1
        input_value = 3515472528
        expected_ready = 1
    if phase == 180:
        take = 1
        input_value = 1379690107
        input_route = 1
        expected_ready = 1
    if phase == 181:
        take = 1
        input_value = 3539395174
        expected_ready = 1
    if phase == 182:
        take = 1
        input_value = 1403616849
        input_route = 1
        expected_ready = 1
    if phase == 183:
        take = 1
        input_value = 3563317820
        expected_ready = 1
    return Stimulus(
        valid=valid,
        take=take,
        input_value=input_value,
        input_route=input_route,
        expected_ready=expected_ready,
        expected_valid=expected_valid,
        expected_data_value=expected_data_value,
        expected_data_route=expected_data_route,
    )


@rule
def advance(phase):
    if phase < 183:
        phase = phase + 1


@system
def ConditionalPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Item(value=frame.input_value, route=frame.input_route)
    dut = ConditionalPipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "conditional_pipeline input capacity"
        assert (
            dut.valid == frame.expected_valid
        ), "conditional_pipeline result availability"
        assert (
            dut.data.value == frame.expected_data_value
        ), "conditional_pipeline value old-state check"
        assert (
            dut.data.route == frame.expected_data_route
        ), "conditional_pipeline route old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "value", dut.data.value)
        log("info", "route", dut.data.route)

    advance(phase)
    check()

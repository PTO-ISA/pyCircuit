"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_popcount_pipeline.popcount_pipeline import PopcountPipeline, Item, Result
from pycircuit import bits, enum_to_bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_value: bits[13]
    input_count: bits[4]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_value: bits[13]
    expected_data_count: bits[4]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    take: bits[1] = 0
    input_value: bits[13] = 0
    input_count: bits[4] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_data_value: bits[13] = 0
    expected_data_count: bits[4] = 0
    if phase == 0:
        valid = 1
        input_count = 3
        expected_ready = 1
    if phase == 1:
        valid = 1
        input_value = 8191
        input_count = 4
        expected_ready = 1
    if phase == 2:
        valid = 1
        input_value = 8190
        input_count = 5
        expected_valid = 1
    if phase == 3:
        valid = 1
        input_value = 4096
        input_count = 6
        expected_valid = 1
    if phase == 4:
        valid = 1
        input_value = 4095
        input_count = 7
        expected_valid = 1
    if phase == 5:
        valid = 1
        input_value = 5461
        input_count = 8
        expected_valid = 1
    if phase == 6:
        valid = 1
        input_value = 2730
        input_count = 9
        expected_valid = 1
    if phase == 7:
        valid = 1
        input_value = 1
        input_count = 10
        expected_valid = 1
    if phase == 8:
        valid = 1
        input_value = 2
        input_count = 11
        expected_valid = 1
    if phase == 9:
        valid = 1
        input_value = 4
        input_count = 12
        expected_valid = 1
    if phase == 10:
        valid = 1
        input_value = 8
        input_count = 13
        expected_valid = 1
    if phase == 11:
        valid = 1
        input_value = 16
        input_count = 14
        expected_valid = 1
    if phase == 12:
        valid = 1
        input_value = 32
        input_count = 15
        expected_valid = 1
    if phase == 13:
        valid = 1
        take = 1
        input_value = 64
        expected_ready = 1
        expected_valid = 1
    if phase == 14:
        valid = 1
        take = 1
        input_value = 128
        input_count = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8191
        expected_data_count = 13
    if phase == 15:
        valid = 1
        take = 1
        input_value = 256
        input_count = 2
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 64
        expected_data_count = 1
    if phase == 16:
        valid = 1
        input_value = 512
        input_count = 3
        expected_valid = 1
        expected_data_value = 128
        expected_data_count = 1
    if phase == 17:
        valid = 1
        take = 1
        input_value = 1024
        input_count = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 128
        expected_data_count = 1
    if phase == 18:
        valid = 1
        take = 1
        input_value = 2048
        input_count = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 256
        expected_data_count = 1
    if phase == 19:
        valid = 1
        take = 1
        input_value = 4096
        input_count = 6
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1024
        expected_data_count = 1
    if phase == 20:
        valid = 1
        input_value = 1
        input_count = 7
        expected_valid = 1
        expected_data_value = 2048
        expected_data_count = 1
    if phase == 21:
        valid = 1
        take = 1
        input_value = 3
        input_count = 8
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2048
        expected_data_count = 1
    if phase == 22:
        valid = 1
        take = 1
        input_value = 7
        input_count = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4096
        expected_data_count = 1
    if phase == 23:
        valid = 1
        take = 1
        input_value = 15
        input_count = 10
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3
        expected_data_count = 2
    if phase == 24:
        valid = 1
        input_value = 31
        input_count = 11
        expected_valid = 1
        expected_data_value = 7
        expected_data_count = 3
    if phase == 25:
        valid = 1
        take = 1
        input_value = 63
        input_count = 12
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7
        expected_data_count = 3
    if phase == 26:
        valid = 1
        take = 1
        input_value = 127
        input_count = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 15
        expected_data_count = 4
    if phase == 27:
        valid = 1
        take = 1
        input_value = 255
        input_count = 14
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 63
        expected_data_count = 6
    if phase == 28:
        valid = 1
        input_value = 511
        input_count = 15
        expected_valid = 1
        expected_data_value = 127
        expected_data_count = 7
    if phase == 29:
        valid = 1
        take = 1
        input_value = 1023
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 127
        expected_data_count = 7
    if phase == 30:
        valid = 1
        take = 1
        input_value = 2047
        input_count = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 255
        expected_data_count = 8
    if phase == 31:
        take = 1
        input_value = 4095
        input_count = 2
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1023
        expected_data_count = 10
    if phase == 32:
        valid = 1
        input_value = 8191
        input_count = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2047
        expected_data_count = 11
    if phase == 33:
        valid = 1
        take = 1
        input_value = 2378
        input_count = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2047
        expected_data_count = 11
    if phase == 34:
        valid = 1
        take = 1
        input_value = 5429
        input_count = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8191
        expected_data_count = 13
    if phase == 35:
        valid = 1
        take = 1
        input_value = 4384
        input_count = 6
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2378
        expected_data_count = 5
    if phase == 36:
        input_value = 3339
        input_count = 7
        expected_valid = 1
        expected_data_value = 5429
        expected_data_count = 7
    if phase == 37:
        valid = 1
        take = 1
        input_value = 6390
        input_count = 8
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5429
        expected_data_count = 7
    if phase == 38:
        valid = 1
        take = 1
        input_value = 1249
        input_count = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4384
        expected_data_count = 3
    if phase == 39:
        valid = 1
        take = 1
        input_value = 204
        input_count = 10
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 6390
        expected_data_count = 8
    if phase == 40:
        valid = 1
        input_value = 7351
        input_count = 11
        expected_valid = 1
        expected_data_value = 1249
        expected_data_count = 5
    if phase == 41:
        take = 1
        input_value = 2210
        input_count = 12
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1249
        expected_data_count = 5
    if phase == 42:
        valid = 1
        take = 1
        input_value = 5261
        input_count = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 204
        expected_data_count = 4
    if phase == 43:
        valid = 1
        take = 1
        input_value = 4216
        input_count = 14
        expected_ready = 1
    if phase == 44:
        valid = 1
        input_value = 3171
        input_count = 15
        expected_valid = 1
        expected_data_value = 5261
        expected_data_count = 6
    if phase == 45:
        valid = 1
        take = 1
        input_value = 6222
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5261
        expected_data_count = 6
    if phase == 46:
        take = 1
        input_value = 1081
        input_count = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4216
        expected_data_count = 5
    if phase == 47:
        valid = 1
        take = 1
        input_value = 36
        input_count = 2
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 6222
        expected_data_count = 6
    if phase == 48:
        valid = 1
        input_value = 7183
        input_count = 3
        expected_ready = 1
    if phase == 49:
        valid = 1
        input_value = 6138
        input_count = 4
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 50:
        valid = 1
        input_value = 5093
        input_count = 5
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 51:
        input_value = 8144
        input_count = 6
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 52:
        valid = 1
        input_value = 3003
        input_count = 7
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 53:
        valid = 1
        input_value = 1958
        input_count = 8
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 54:
        valid = 1
        input_value = 913
        input_count = 9
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 55:
        valid = 1
        input_value = 3964
        input_count = 10
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 56:
        input_value = 7015
        input_count = 11
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 57:
        valid = 1
        input_value = 5970
        input_count = 12
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 58:
        valid = 1
        input_value = 4925
        input_count = 13
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 59:
        valid = 1
        input_value = 7976
        input_count = 14
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 60:
        valid = 1
        input_value = 2835
        input_count = 15
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 61:
        input_value = 1790
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 62:
        valid = 1
        input_value = 745
        input_count = 1
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 63:
        valid = 1
        input_value = 3796
        input_count = 2
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 64:
        valid = 1
        take = 1
        input_count = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 65:
        valid = 1
        take = 1
        input_value = 8191
        input_count = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7183
        expected_data_count = 7
    if phase == 66:
        valid = 1
        take = 1
        input_value = 8190
        input_count = 5
        expected_ready = 1
        expected_valid = 1
    if phase == 67:
        valid = 1
        take = 1
        input_value = 4096
        input_count = 6
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8191
        expected_data_count = 13
    if phase == 68:
        valid = 1
        take = 1
        input_value = 4095
        input_count = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8190
        expected_data_count = 12
    if phase == 69:
        valid = 1
        take = 1
        input_value = 5461
        input_count = 8
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4096
        expected_data_count = 1
    if phase == 70:
        valid = 1
        take = 1
        input_value = 2730
        input_count = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4095
        expected_data_count = 12
    if phase == 71:
        valid = 1
        take = 1
        input_value = 1
        input_count = 10
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5461
        expected_data_count = 7
    if phase == 72:
        valid = 1
        take = 1
        input_value = 2
        input_count = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2730
        expected_data_count = 6
    if phase == 73:
        valid = 1
        take = 1
        input_value = 4
        input_count = 12
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1
        expected_data_count = 1
    if phase == 74:
        valid = 1
        take = 1
        input_value = 8
        input_count = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2
        expected_data_count = 1
    if phase == 75:
        valid = 1
        take = 1
        input_value = 16
        input_count = 14
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4
        expected_data_count = 1
    if phase == 76:
        valid = 1
        take = 1
        input_value = 32
        input_count = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8
        expected_data_count = 1
    if phase == 77:
        valid = 1
        take = 1
        input_value = 64
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 16
        expected_data_count = 1
    if phase == 78:
        valid = 1
        take = 1
        input_value = 128
        input_count = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 32
        expected_data_count = 1
    if phase == 79:
        valid = 1
        take = 1
        input_value = 256
        input_count = 2
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 64
        expected_data_count = 1
    if phase == 80:
        valid = 1
        take = 1
        input_value = 512
        input_count = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 128
        expected_data_count = 1
    if phase == 81:
        valid = 1
        take = 1
        input_value = 1024
        input_count = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 256
        expected_data_count = 1
    if phase == 82:
        valid = 1
        take = 1
        input_value = 2048
        input_count = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 512
        expected_data_count = 1
    if phase == 83:
        valid = 1
        take = 1
        input_value = 4096
        input_count = 6
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1024
        expected_data_count = 1
    if phase == 84:
        valid = 1
        take = 1
        input_value = 1
        input_count = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2048
        expected_data_count = 1
    if phase == 85:
        valid = 1
        take = 1
        input_value = 3
        input_count = 8
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4096
        expected_data_count = 1
    if phase == 86:
        valid = 1
        take = 1
        input_value = 7
        input_count = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1
        expected_data_count = 1
    if phase == 87:
        valid = 1
        take = 1
        input_value = 15
        input_count = 10
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3
        expected_data_count = 2
    if phase == 88:
        valid = 1
        take = 1
        input_value = 31
        input_count = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7
        expected_data_count = 3
    if phase == 89:
        valid = 1
        take = 1
        input_value = 63
        input_count = 12
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 15
        expected_data_count = 4
    if phase == 90:
        valid = 1
        take = 1
        input_value = 127
        input_count = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 31
        expected_data_count = 5
    if phase == 91:
        valid = 1
        take = 1
        input_value = 255
        input_count = 14
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 63
        expected_data_count = 6
    if phase == 92:
        valid = 1
        take = 1
        input_value = 511
        input_count = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 127
        expected_data_count = 7
    if phase == 93:
        valid = 1
        take = 1
        input_value = 1023
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 255
        expected_data_count = 8
    if phase == 94:
        valid = 1
        take = 1
        input_value = 2047
        input_count = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 511
        expected_data_count = 9
    if phase == 95:
        valid = 1
        take = 1
        input_value = 4095
        input_count = 2
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1023
        expected_data_count = 10
    if phase == 96:
        valid = 1
        take = 1
        input_value = 8191
        input_count = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2047
        expected_data_count = 11
    if phase == 97:
        valid = 1
        take = 1
        input_value = 2378
        input_count = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4095
        expected_data_count = 12
    if phase == 98:
        valid = 1
        take = 1
        input_value = 5429
        input_count = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8191
        expected_data_count = 13
    if phase == 99:
        valid = 1
        take = 1
        input_value = 4384
        input_count = 6
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2378
        expected_data_count = 5
    if phase == 100:
        valid = 1
        take = 1
        input_value = 3339
        input_count = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5429
        expected_data_count = 7
    if phase == 101:
        valid = 1
        take = 1
        input_value = 6390
        input_count = 8
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4384
        expected_data_count = 3
    if phase == 102:
        valid = 1
        take = 1
        input_value = 1249
        input_count = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3339
        expected_data_count = 6
    if phase == 103:
        valid = 1
        take = 1
        input_value = 204
        input_count = 10
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 6390
        expected_data_count = 8
    if phase == 104:
        valid = 1
        take = 1
        input_value = 7351
        input_count = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1249
        expected_data_count = 5
    if phase == 105:
        valid = 1
        take = 1
        input_value = 2210
        input_count = 12
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 204
        expected_data_count = 4
    if phase == 106:
        valid = 1
        take = 1
        input_value = 5261
        input_count = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7351
        expected_data_count = 9
    if phase == 107:
        valid = 1
        take = 1
        input_value = 4216
        input_count = 14
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2210
        expected_data_count = 4
    if phase == 108:
        valid = 1
        take = 1
        input_value = 3171
        input_count = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5261
        expected_data_count = 6
    if phase == 109:
        valid = 1
        take = 1
        input_value = 6222
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4216
        expected_data_count = 5
    if phase == 110:
        valid = 1
        take = 1
        input_value = 1081
        input_count = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3171
        expected_data_count = 6
    if phase == 111:
        valid = 1
        take = 1
        input_value = 36
        input_count = 2
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 6222
        expected_data_count = 6
    if phase == 112:
        valid = 1
        take = 1
        input_value = 7183
        input_count = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1081
        expected_data_count = 5
    if phase == 113:
        valid = 1
        take = 1
        input_value = 6138
        input_count = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 36
        expected_data_count = 2
    if phase == 114:
        valid = 1
        take = 1
        input_value = 5093
        input_count = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7183
        expected_data_count = 7
    if phase == 115:
        valid = 1
        take = 1
        input_value = 8144
        input_count = 6
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 6138
        expected_data_count = 10
    if phase == 116:
        valid = 1
        take = 1
        input_value = 3003
        input_count = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5093
        expected_data_count = 8
    if phase == 117:
        valid = 1
        take = 1
        input_value = 1958
        input_count = 8
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 8144
        expected_data_count = 8
    if phase == 118:
        valid = 1
        take = 1
        input_value = 913
        input_count = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3003
        expected_data_count = 9
    if phase == 119:
        valid = 1
        take = 1
        input_value = 3964
        input_count = 10
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1958
        expected_data_count = 7
    if phase == 120:
        valid = 1
        take = 1
        input_value = 7015
        input_count = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 913
        expected_data_count = 5
    if phase == 121:
        valid = 1
        take = 1
        input_value = 5970
        input_count = 12
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3964
        expected_data_count = 9
    if phase == 122:
        valid = 1
        take = 1
        input_value = 4925
        input_count = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7015
        expected_data_count = 9
    if phase == 123:
        valid = 1
        take = 1
        input_value = 7976
        input_count = 14
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5970
        expected_data_count = 7
    if phase == 124:
        valid = 1
        take = 1
        input_value = 2835
        input_count = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4925
        expected_data_count = 8
    if phase == 125:
        valid = 1
        take = 1
        input_value = 1790
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7976
        expected_data_count = 7
    if phase == 126:
        valid = 1
        take = 1
        input_value = 745
        input_count = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2835
        expected_data_count = 6
    if phase == 127:
        valid = 1
        take = 1
        input_value = 3796
        input_count = 2
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1790
        expected_data_count = 9
    if phase == 128:
        valid = 1
        take = 1
        input_value = 6847
        input_count = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 745
        expected_data_count = 6
    if phase == 129:
        valid = 1
        take = 1
        input_value = 5802
        input_count = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3796
        expected_data_count = 7
    if phase == 130:
        valid = 1
        take = 1
        input_value = 4757
        input_count = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 6847
        expected_data_count = 10
    if phase == 131:
        valid = 1
        take = 1
        input_value = 7808
        input_count = 6
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5802
        expected_data_count = 7
    if phase == 132:
        valid = 1
        take = 1
        input_value = 2667
        input_count = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4757
        expected_data_count = 6
    if phase == 133:
        valid = 1
        take = 1
        input_value = 1622
        input_count = 8
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7808
        expected_data_count = 5
    if phase == 134:
        valid = 1
        take = 1
        input_value = 577
        input_count = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2667
        expected_data_count = 7
    if phase == 135:
        valid = 1
        take = 1
        input_value = 3628
        input_count = 10
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1622
        expected_data_count = 6
    if phase == 136:
        valid = 1
        take = 1
        input_value = 6679
        input_count = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 577
        expected_data_count = 3
    if phase == 137:
        valid = 1
        take = 1
        input_value = 5634
        input_count = 12
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3628
        expected_data_count = 6
    if phase == 138:
        valid = 1
        take = 1
        input_value = 4589
        input_count = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 6679
        expected_data_count = 7
    if phase == 139:
        valid = 1
        take = 1
        input_value = 7640
        input_count = 14
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5634
        expected_data_count = 4
    if phase == 140:
        valid = 1
        take = 1
        input_value = 2499
        input_count = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4589
        expected_data_count = 8
    if phase == 141:
        valid = 1
        take = 1
        input_value = 1454
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7640
        expected_data_count = 8
    if phase == 142:
        valid = 1
        take = 1
        input_value = 409
        input_count = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2499
        expected_data_count = 6
    if phase == 143:
        valid = 1
        take = 1
        input_value = 3460
        input_count = 2
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1454
        expected_data_count = 7
    if phase == 144:
        valid = 1
        take = 1
        input_value = 6511
        input_count = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 409
        expected_data_count = 5
    if phase == 145:
        valid = 1
        take = 1
        input_value = 5466
        input_count = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3460
        expected_data_count = 5
    if phase == 146:
        valid = 1
        take = 1
        input_value = 4421
        input_count = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 6511
        expected_data_count = 9
    if phase == 147:
        valid = 1
        take = 1
        input_value = 7472
        input_count = 6
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5466
        expected_data_count = 7
    if phase == 148:
        valid = 1
        take = 1
        input_value = 2331
        input_count = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4421
        expected_data_count = 5
    if phase == 149:
        valid = 1
        take = 1
        input_value = 1286
        input_count = 8
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7472
        expected_data_count = 6
    if phase == 150:
        valid = 1
        take = 1
        input_value = 241
        input_count = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2331
        expected_data_count = 6
    if phase == 151:
        valid = 1
        take = 1
        input_value = 3292
        input_count = 10
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1286
        expected_data_count = 4
    if phase == 152:
        valid = 1
        take = 1
        input_value = 6343
        input_count = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 241
        expected_data_count = 5
    if phase == 153:
        valid = 1
        take = 1
        input_value = 5298
        input_count = 12
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3292
        expected_data_count = 7
    if phase == 154:
        valid = 1
        take = 1
        input_value = 4253
        input_count = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 6343
        expected_data_count = 7
    if phase == 155:
        valid = 1
        take = 1
        input_value = 7304
        input_count = 14
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 5298
        expected_data_count = 6
    if phase == 156:
        valid = 1
        take = 1
        input_value = 2163
        input_count = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 4253
        expected_data_count = 6
    if phase == 157:
        valid = 1
        take = 1
        input_value = 1118
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 7304
        expected_data_count = 5
    if phase == 158:
        valid = 1
        take = 1
        input_value = 73
        input_count = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 2163
        expected_data_count = 6
    if phase == 159:
        valid = 1
        take = 1
        input_value = 3124
        input_count = 2
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 1118
        expected_data_count = 6
    if phase == 160:
        take = 1
        input_value = 6175
        input_count = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 73
        expected_data_count = 3
    if phase == 161:
        take = 1
        input_value = 5130
        input_count = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_value = 3124
        expected_data_count = 5
    if phase == 162:
        take = 1
        input_value = 4085
        input_count = 5
        expected_ready = 1
    if phase == 163:
        take = 1
        input_value = 7136
        input_count = 6
        expected_ready = 1
    if phase == 164:
        take = 1
        input_value = 1995
        input_count = 7
        expected_ready = 1
    if phase == 165:
        take = 1
        input_value = 950
        input_count = 8
        expected_ready = 1
    if phase == 166:
        take = 1
        input_value = 8097
        input_count = 9
        expected_ready = 1
    if phase == 167:
        take = 1
        input_value = 2956
        input_count = 10
        expected_ready = 1
    if phase == 168:
        take = 1
        input_value = 6007
        input_count = 11
        expected_ready = 1
    if phase == 169:
        take = 1
        input_value = 4962
        input_count = 12
        expected_ready = 1
    if phase == 170:
        take = 1
        input_value = 3917
        input_count = 13
        expected_ready = 1
    if phase == 171:
        take = 1
        input_value = 6968
        input_count = 14
        expected_ready = 1
    if phase == 172:
        take = 1
        input_value = 1827
        input_count = 15
        expected_ready = 1
    if phase == 173:
        take = 1
        input_value = 782
        expected_ready = 1
    if phase == 174:
        take = 1
        input_value = 7929
        input_count = 1
        expected_ready = 1
    if phase == 175:
        take = 1
        input_value = 2788
        input_count = 2
        expected_ready = 1
    if phase == 176:
        take = 1
        input_value = 5839
        input_count = 3
        expected_ready = 1
    if phase == 177:
        take = 1
        input_value = 4794
        input_count = 4
        expected_ready = 1
    if phase == 178:
        take = 1
        input_value = 3749
        input_count = 5
        expected_ready = 1
    if phase == 179:
        take = 1
        input_value = 6800
        input_count = 6
        expected_ready = 1
    if phase == 180:
        take = 1
        input_value = 1659
        input_count = 7
        expected_ready = 1
    if phase == 181:
        take = 1
        input_value = 614
        input_count = 8
        expected_ready = 1
    if phase == 182:
        take = 1
        input_value = 7761
        input_count = 9
        expected_ready = 1
    if phase == 183:
        take = 1
        input_value = 2620
        input_count = 10
        expected_ready = 1
    return Stimulus(
        valid=valid,
        take=take,
        input_value=input_value,
        input_count=input_count,
        expected_ready=expected_ready,
        expected_valid=expected_valid,
        expected_data_value=expected_data_value,
        expected_data_count=expected_data_count,
    )


@rule
def advance(phase):
    if phase < 183:
        phase = phase + 1


@system
def PopcountPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Item(value=frame.input_value, count=frame.input_count)
    dut = PopcountPipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "popcount_pipeline input capacity"
        assert (
            dut.valid == frame.expected_valid
        ), "popcount_pipeline result availability"
        assert (
            dut.data.value == frame.expected_data_value
        ), "popcount_pipeline value old-state check"
        assert (
            dut.data.count == frame.expected_data_count
        ), "popcount_pipeline count old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "value", dut.data.value)
        log("info", "count", dut.data.count)

    advance(phase)
    check()

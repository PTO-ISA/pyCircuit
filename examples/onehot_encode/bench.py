"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_onehot_encode.onehot_encode import OnehotEncode, EncodedFlags, Result
from pycircuit import bits, enum_to_bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_flags: bits[8]
    input_index: bits[3]
    input_valid: bits[1]
    input_conflict: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_flags: bits[8]
    expected_data_index: bits[3]
    expected_data_valid: bits[1]
    expected_data_conflict: bits[1]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    take: bits[1] = 0
    input_flags: bits[8] = 0
    input_index: bits[3] = 0
    input_valid: bits[1] = 0
    input_conflict: bits[1] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_data_flags: bits[8] = 0
    expected_data_index: bits[3] = 0
    expected_data_valid: bits[1] = 0
    expected_data_conflict: bits[1] = 0
    if phase == 0:
        valid = 1
        input_index = 3
        input_conflict = 1
        expected_ready = 1
    if phase == 1:
        valid = 1
        input_flags = 255
        input_index = 4
        input_valid = 1
        expected_ready = 1
    if phase == 2:
        valid = 1
        input_flags = 254
        input_index = 5
        input_conflict = 1
        expected_valid = 1
    if phase == 3:
        valid = 1
        input_flags = 128
        input_index = 6
        input_valid = 1
        expected_valid = 1
    if phase == 4:
        valid = 1
        input_flags = 127
        input_index = 7
        input_conflict = 1
        expected_valid = 1
    if phase == 5:
        valid = 1
        input_flags = 170
        input_valid = 1
        expected_valid = 1
    if phase == 6:
        valid = 1
        input_flags = 85
        input_index = 1
        input_conflict = 1
        expected_valid = 1
    if phase == 7:
        valid = 1
        input_flags = 1
        input_index = 2
        input_valid = 1
        expected_valid = 1
    if phase == 8:
        valid = 1
        input_flags = 2
        input_index = 3
        input_conflict = 1
        expected_valid = 1
    if phase == 9:
        valid = 1
        input_flags = 4
        input_index = 4
        input_valid = 1
        expected_valid = 1
    if phase == 10:
        valid = 1
        input_flags = 8
        input_index = 5
        input_conflict = 1
        expected_valid = 1
    if phase == 11:
        valid = 1
        input_flags = 16
        input_index = 6
        input_valid = 1
        expected_valid = 1
    if phase == 12:
        valid = 1
        input_flags = 32
        input_index = 7
        input_conflict = 1
        expected_valid = 1
    if phase == 13:
        valid = 1
        take = 1
        input_flags = 64
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 14:
        valid = 1
        take = 1
        input_flags = 128
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 255
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 15:
        valid = 1
        take = 1
        input_flags = 1
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 64
        expected_data_index = 6
        expected_data_valid = 1
    if phase == 16:
        valid = 1
        input_flags = 3
        input_index = 3
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 128
        expected_data_index = 7
        expected_data_valid = 1
    if phase == 17:
        valid = 1
        take = 1
        input_flags = 7
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 128
        expected_data_index = 7
        expected_data_valid = 1
    if phase == 18:
        valid = 1
        take = 1
        input_flags = 15
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 1
        expected_data_valid = 1
    if phase == 19:
        valid = 1
        take = 1
        input_flags = 31
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 7
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 20:
        valid = 1
        input_flags = 63
        input_index = 7
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 15
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 21:
        valid = 1
        take = 1
        input_flags = 127
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 15
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 22:
        valid = 1
        take = 1
        input_flags = 255
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 31
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 23:
        valid = 1
        take = 1
        input_flags = 28
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 127
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 24:
        valid = 1
        input_flags = 7
        input_index = 3
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 255
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 25:
        valid = 1
        take = 1
        input_flags = 242
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 255
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 26:
        valid = 1
        take = 1
        input_flags = 221
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 28
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 27:
        valid = 1
        take = 1
        input_flags = 200
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 242
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 28:
        valid = 1
        input_flags = 179
        input_index = 7
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 221
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 29:
        valid = 1
        take = 1
        input_flags = 158
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 221
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 30:
        valid = 1
        take = 1
        input_flags = 137
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 200
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 31:
        take = 1
        input_flags = 116
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 158
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 32:
        valid = 1
        input_flags = 95
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 137
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 33:
        valid = 1
        take = 1
        input_flags = 74
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 137
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 34:
        valid = 1
        take = 1
        input_flags = 53
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 95
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 35:
        valid = 1
        take = 1
        input_flags = 32
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 74
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 36:
        input_flags = 11
        input_index = 7
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 53
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 37:
        valid = 1
        take = 1
        input_flags = 246
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 53
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 38:
        valid = 1
        take = 1
        input_flags = 225
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 32
        expected_data_index = 5
        expected_data_valid = 1
    if phase == 39:
        valid = 1
        take = 1
        input_flags = 204
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 246
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 40:
        valid = 1
        input_flags = 183
        input_index = 3
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 225
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 41:
        take = 1
        input_flags = 162
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 225
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 42:
        valid = 1
        take = 1
        input_flags = 141
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 204
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 43:
        valid = 1
        take = 1
        input_flags = 120
        input_index = 6
        input_valid = 1
        expected_ready = 1
    if phase == 44:
        valid = 1
        input_flags = 99
        input_index = 7
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 141
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 45:
        valid = 1
        take = 1
        input_flags = 78
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 141
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 46:
        take = 1
        input_flags = 57
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 120
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 47:
        valid = 1
        take = 1
        input_flags = 36
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 78
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 48:
        valid = 1
        input_flags = 15
        input_index = 3
        input_conflict = 1
        expected_ready = 1
    if phase == 49:
        valid = 1
        input_flags = 250
        input_index = 4
        input_valid = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 50:
        valid = 1
        input_flags = 229
        input_index = 5
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 51:
        input_flags = 208
        input_index = 6
        input_valid = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 52:
        valid = 1
        input_flags = 187
        input_index = 7
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 53:
        valid = 1
        input_flags = 166
        input_valid = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 54:
        valid = 1
        input_flags = 145
        input_index = 1
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 55:
        valid = 1
        input_flags = 124
        input_index = 2
        input_valid = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 56:
        input_flags = 103
        input_index = 3
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 57:
        valid = 1
        input_flags = 82
        input_index = 4
        input_valid = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 58:
        valid = 1
        input_flags = 61
        input_index = 5
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 59:
        valid = 1
        input_flags = 40
        input_index = 6
        input_valid = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 60:
        valid = 1
        input_flags = 19
        input_index = 7
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 61:
        input_flags = 254
        input_valid = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 62:
        valid = 1
        input_flags = 233
        input_index = 1
        input_conflict = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 63:
        valid = 1
        input_flags = 212
        input_index = 2
        input_valid = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 64:
        valid = 1
        take = 1
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 65:
        valid = 1
        take = 1
        input_flags = 1
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 15
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 66:
        valid = 1
        take = 1
        input_flags = 2
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 67:
        valid = 1
        take = 1
        input_flags = 3
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 1
        expected_data_valid = 1
    if phase == 68:
        valid = 1
        take = 1
        input_flags = 4
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 2
        expected_data_index = 1
        expected_data_valid = 1
    if phase == 69:
        valid = 1
        take = 1
        input_flags = 5
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 70:
        valid = 1
        take = 1
        input_flags = 6
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 4
        expected_data_index = 2
        expected_data_valid = 1
    if phase == 71:
        valid = 1
        take = 1
        input_flags = 7
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 5
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 72:
        valid = 1
        take = 1
        input_flags = 8
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 6
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 73:
        valid = 1
        take = 1
        input_flags = 9
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 7
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 74:
        valid = 1
        take = 1
        input_flags = 10
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 8
        expected_data_index = 3
        expected_data_valid = 1
    if phase == 75:
        valid = 1
        take = 1
        input_flags = 11
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 9
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 76:
        valid = 1
        take = 1
        input_flags = 12
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 10
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 77:
        valid = 1
        take = 1
        input_flags = 13
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 11
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 78:
        valid = 1
        take = 1
        input_flags = 14
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 12
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 79:
        valid = 1
        take = 1
        input_flags = 15
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 13
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 80:
        valid = 1
        take = 1
        input_flags = 16
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 14
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 81:
        valid = 1
        take = 1
        input_flags = 17
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 15
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 82:
        valid = 1
        take = 1
        input_flags = 18
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 16
        expected_data_index = 4
        expected_data_valid = 1
    if phase == 83:
        valid = 1
        take = 1
        input_flags = 19
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 17
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 84:
        valid = 1
        take = 1
        input_flags = 20
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 18
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 85:
        valid = 1
        take = 1
        input_flags = 21
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 19
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 86:
        valid = 1
        take = 1
        input_flags = 22
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 20
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 87:
        valid = 1
        take = 1
        input_flags = 23
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 21
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 88:
        valid = 1
        take = 1
        input_flags = 24
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 22
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 89:
        valid = 1
        take = 1
        input_flags = 25
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 23
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 90:
        valid = 1
        take = 1
        input_flags = 26
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 24
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 91:
        valid = 1
        take = 1
        input_flags = 27
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 25
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 92:
        valid = 1
        take = 1
        input_flags = 28
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 26
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 93:
        valid = 1
        take = 1
        input_flags = 29
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 27
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 94:
        valid = 1
        take = 1
        input_flags = 30
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 28
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 95:
        valid = 1
        take = 1
        input_flags = 31
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 29
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 96:
        valid = 1
        take = 1
        input_flags = 32
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 30
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 97:
        valid = 1
        take = 1
        input_flags = 33
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 31
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 98:
        valid = 1
        take = 1
        input_flags = 34
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 32
        expected_data_index = 5
        expected_data_valid = 1
    if phase == 99:
        valid = 1
        take = 1
        input_flags = 35
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 33
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 100:
        valid = 1
        take = 1
        input_flags = 36
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 34
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 101:
        valid = 1
        take = 1
        input_flags = 37
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 35
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 102:
        valid = 1
        take = 1
        input_flags = 38
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 36
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 103:
        valid = 1
        take = 1
        input_flags = 39
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 37
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 104:
        valid = 1
        take = 1
        input_flags = 40
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 38
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 105:
        valid = 1
        take = 1
        input_flags = 41
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 39
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 106:
        valid = 1
        take = 1
        input_flags = 42
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 40
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 107:
        valid = 1
        take = 1
        input_flags = 43
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 41
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 108:
        valid = 1
        take = 1
        input_flags = 44
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 42
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 109:
        valid = 1
        take = 1
        input_flags = 45
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 43
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 110:
        valid = 1
        take = 1
        input_flags = 46
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 44
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 111:
        valid = 1
        take = 1
        input_flags = 47
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 45
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 112:
        valid = 1
        take = 1
        input_flags = 48
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 46
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 113:
        valid = 1
        take = 1
        input_flags = 49
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 47
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 114:
        valid = 1
        take = 1
        input_flags = 50
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 48
        expected_data_index = 4
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 115:
        valid = 1
        take = 1
        input_flags = 51
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 49
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 116:
        valid = 1
        take = 1
        input_flags = 52
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 50
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 117:
        valid = 1
        take = 1
        input_flags = 53
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 51
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 118:
        valid = 1
        take = 1
        input_flags = 54
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 52
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 119:
        valid = 1
        take = 1
        input_flags = 55
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 53
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 120:
        valid = 1
        take = 1
        input_flags = 56
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 54
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 121:
        valid = 1
        take = 1
        input_flags = 57
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 55
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 122:
        valid = 1
        take = 1
        input_flags = 58
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 56
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 123:
        valid = 1
        take = 1
        input_flags = 59
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 57
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 124:
        valid = 1
        take = 1
        input_flags = 60
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 58
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 125:
        valid = 1
        take = 1
        input_flags = 61
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 59
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 126:
        valid = 1
        take = 1
        input_flags = 62
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 60
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 127:
        valid = 1
        take = 1
        input_flags = 63
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 61
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 128:
        valid = 1
        take = 1
        input_flags = 64
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 62
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 129:
        valid = 1
        take = 1
        input_flags = 65
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 63
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 130:
        valid = 1
        take = 1
        input_flags = 66
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 64
        expected_data_index = 6
        expected_data_valid = 1
    if phase == 131:
        valid = 1
        take = 1
        input_flags = 67
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 65
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 132:
        valid = 1
        take = 1
        input_flags = 68
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 66
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 133:
        valid = 1
        take = 1
        input_flags = 69
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 67
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 134:
        valid = 1
        take = 1
        input_flags = 70
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 68
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 135:
        valid = 1
        take = 1
        input_flags = 71
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 69
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 136:
        valid = 1
        take = 1
        input_flags = 72
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 70
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 137:
        valid = 1
        take = 1
        input_flags = 73
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 71
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 138:
        valid = 1
        take = 1
        input_flags = 74
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 72
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 139:
        valid = 1
        take = 1
        input_flags = 75
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 73
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 140:
        valid = 1
        take = 1
        input_flags = 76
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 74
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 141:
        valid = 1
        take = 1
        input_flags = 77
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 75
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 142:
        valid = 1
        take = 1
        input_flags = 78
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 76
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 143:
        valid = 1
        take = 1
        input_flags = 79
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 77
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 144:
        valid = 1
        take = 1
        input_flags = 80
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 78
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 145:
        valid = 1
        take = 1
        input_flags = 81
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 79
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 146:
        valid = 1
        take = 1
        input_flags = 82
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 80
        expected_data_index = 4
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 147:
        valid = 1
        take = 1
        input_flags = 83
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 81
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 148:
        valid = 1
        take = 1
        input_flags = 84
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 82
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 149:
        valid = 1
        take = 1
        input_flags = 85
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 83
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 150:
        valid = 1
        take = 1
        input_flags = 86
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 84
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 151:
        valid = 1
        take = 1
        input_flags = 87
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 85
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 152:
        valid = 1
        take = 1
        input_flags = 88
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 86
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 153:
        valid = 1
        take = 1
        input_flags = 89
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 87
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 154:
        valid = 1
        take = 1
        input_flags = 90
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 88
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 155:
        valid = 1
        take = 1
        input_flags = 91
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 89
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 156:
        valid = 1
        take = 1
        input_flags = 92
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 90
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 157:
        valid = 1
        take = 1
        input_flags = 93
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 91
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 158:
        valid = 1
        take = 1
        input_flags = 94
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 92
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 159:
        valid = 1
        take = 1
        input_flags = 95
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 93
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 160:
        valid = 1
        take = 1
        input_flags = 96
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 94
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 161:
        valid = 1
        take = 1
        input_flags = 97
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 95
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 162:
        valid = 1
        take = 1
        input_flags = 98
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 96
        expected_data_index = 5
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 163:
        valid = 1
        take = 1
        input_flags = 99
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 97
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 164:
        valid = 1
        take = 1
        input_flags = 100
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 98
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 165:
        valid = 1
        take = 1
        input_flags = 101
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 99
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 166:
        valid = 1
        take = 1
        input_flags = 102
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 100
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 167:
        valid = 1
        take = 1
        input_flags = 103
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 101
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 168:
        valid = 1
        take = 1
        input_flags = 104
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 102
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 169:
        valid = 1
        take = 1
        input_flags = 105
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 103
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 170:
        valid = 1
        take = 1
        input_flags = 106
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 104
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 171:
        valid = 1
        take = 1
        input_flags = 107
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 105
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 172:
        valid = 1
        take = 1
        input_flags = 108
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 106
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 173:
        valid = 1
        take = 1
        input_flags = 109
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 107
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 174:
        valid = 1
        take = 1
        input_flags = 110
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 108
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 175:
        valid = 1
        take = 1
        input_flags = 111
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 109
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 176:
        valid = 1
        take = 1
        input_flags = 112
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 110
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 177:
        valid = 1
        take = 1
        input_flags = 113
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 111
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 178:
        valid = 1
        take = 1
        input_flags = 114
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 112
        expected_data_index = 4
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 179:
        valid = 1
        take = 1
        input_flags = 115
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 113
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 180:
        valid = 1
        take = 1
        input_flags = 116
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 114
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 181:
        valid = 1
        take = 1
        input_flags = 117
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 115
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 182:
        valid = 1
        take = 1
        input_flags = 118
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 116
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 183:
        valid = 1
        take = 1
        input_flags = 119
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 117
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 184:
        valid = 1
        take = 1
        input_flags = 120
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 118
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 185:
        valid = 1
        take = 1
        input_flags = 121
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 119
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 186:
        valid = 1
        take = 1
        input_flags = 122
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 120
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 187:
        valid = 1
        take = 1
        input_flags = 123
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 121
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 188:
        valid = 1
        take = 1
        input_flags = 124
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 122
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 189:
        valid = 1
        take = 1
        input_flags = 125
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 123
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 190:
        valid = 1
        take = 1
        input_flags = 126
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 124
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 191:
        valid = 1
        take = 1
        input_flags = 127
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 125
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 192:
        valid = 1
        take = 1
        input_flags = 128
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 126
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 193:
        valid = 1
        take = 1
        input_flags = 129
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 127
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 194:
        valid = 1
        take = 1
        input_flags = 130
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 128
        expected_data_index = 7
        expected_data_valid = 1
    if phase == 195:
        valid = 1
        take = 1
        input_flags = 131
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 129
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 196:
        valid = 1
        take = 1
        input_flags = 132
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 130
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 197:
        valid = 1
        take = 1
        input_flags = 133
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 131
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 198:
        valid = 1
        take = 1
        input_flags = 134
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 132
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 199:
        valid = 1
        take = 1
        input_flags = 135
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 133
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 200:
        valid = 1
        take = 1
        input_flags = 136
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 134
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 201:
        valid = 1
        take = 1
        input_flags = 137
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 135
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 202:
        valid = 1
        take = 1
        input_flags = 138
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 136
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 203:
        valid = 1
        take = 1
        input_flags = 139
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 137
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 204:
        valid = 1
        take = 1
        input_flags = 140
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 138
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 205:
        valid = 1
        take = 1
        input_flags = 141
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 139
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 206:
        valid = 1
        take = 1
        input_flags = 142
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 140
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 207:
        valid = 1
        take = 1
        input_flags = 143
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 141
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 208:
        valid = 1
        take = 1
        input_flags = 144
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 142
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 209:
        valid = 1
        take = 1
        input_flags = 145
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 143
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 210:
        valid = 1
        take = 1
        input_flags = 146
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 144
        expected_data_index = 4
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 211:
        valid = 1
        take = 1
        input_flags = 147
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 145
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 212:
        valid = 1
        take = 1
        input_flags = 148
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 146
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 213:
        valid = 1
        take = 1
        input_flags = 149
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 147
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 214:
        valid = 1
        take = 1
        input_flags = 150
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 148
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 215:
        valid = 1
        take = 1
        input_flags = 151
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 149
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 216:
        valid = 1
        take = 1
        input_flags = 152
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 150
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 217:
        valid = 1
        take = 1
        input_flags = 153
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 151
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 218:
        valid = 1
        take = 1
        input_flags = 154
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 152
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 219:
        valid = 1
        take = 1
        input_flags = 155
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 153
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 220:
        valid = 1
        take = 1
        input_flags = 156
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 154
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 221:
        valid = 1
        take = 1
        input_flags = 157
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 155
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 222:
        valid = 1
        take = 1
        input_flags = 158
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 156
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 223:
        valid = 1
        take = 1
        input_flags = 159
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 157
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 224:
        valid = 1
        take = 1
        input_flags = 160
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 158
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 225:
        valid = 1
        take = 1
        input_flags = 161
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 159
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 226:
        valid = 1
        take = 1
        input_flags = 162
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 160
        expected_data_index = 5
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 227:
        valid = 1
        take = 1
        input_flags = 163
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 161
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 228:
        valid = 1
        take = 1
        input_flags = 164
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 162
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 229:
        valid = 1
        take = 1
        input_flags = 165
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 163
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 230:
        valid = 1
        take = 1
        input_flags = 166
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 164
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 231:
        valid = 1
        take = 1
        input_flags = 167
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 165
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 232:
        valid = 1
        take = 1
        input_flags = 168
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 166
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 233:
        valid = 1
        take = 1
        input_flags = 169
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 167
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 234:
        valid = 1
        take = 1
        input_flags = 170
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 168
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 235:
        valid = 1
        take = 1
        input_flags = 171
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 169
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 236:
        valid = 1
        take = 1
        input_flags = 172
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 170
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 237:
        valid = 1
        take = 1
        input_flags = 173
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 171
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 238:
        valid = 1
        take = 1
        input_flags = 174
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 172
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 239:
        valid = 1
        take = 1
        input_flags = 175
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 173
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 240:
        valid = 1
        take = 1
        input_flags = 176
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 174
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 241:
        valid = 1
        take = 1
        input_flags = 177
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 175
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 242:
        valid = 1
        take = 1
        input_flags = 178
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 176
        expected_data_index = 4
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 243:
        valid = 1
        take = 1
        input_flags = 179
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 177
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 244:
        valid = 1
        take = 1
        input_flags = 180
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 178
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 245:
        valid = 1
        take = 1
        input_flags = 181
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 179
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 246:
        valid = 1
        take = 1
        input_flags = 182
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 180
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 247:
        valid = 1
        take = 1
        input_flags = 183
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 181
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 248:
        valid = 1
        take = 1
        input_flags = 184
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 182
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 249:
        valid = 1
        take = 1
        input_flags = 185
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 183
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 250:
        valid = 1
        take = 1
        input_flags = 186
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 184
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 251:
        valid = 1
        take = 1
        input_flags = 187
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 185
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 252:
        valid = 1
        take = 1
        input_flags = 188
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 186
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 253:
        valid = 1
        take = 1
        input_flags = 189
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 187
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 254:
        valid = 1
        take = 1
        input_flags = 190
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 188
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 255:
        valid = 1
        take = 1
        input_flags = 191
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 189
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 256:
        valid = 1
        take = 1
        input_flags = 192
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 190
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 257:
        valid = 1
        take = 1
        input_flags = 193
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 191
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 258:
        valid = 1
        take = 1
        input_flags = 194
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 192
        expected_data_index = 6
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 259:
        valid = 1
        take = 1
        input_flags = 195
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 193
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 260:
        valid = 1
        take = 1
        input_flags = 196
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 194
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 261:
        valid = 1
        take = 1
        input_flags = 197
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 195
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 262:
        valid = 1
        take = 1
        input_flags = 198
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 196
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 263:
        valid = 1
        take = 1
        input_flags = 199
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 197
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 264:
        valid = 1
        take = 1
        input_flags = 200
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 198
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 265:
        valid = 1
        take = 1
        input_flags = 201
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 199
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 266:
        valid = 1
        take = 1
        input_flags = 202
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 200
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 267:
        valid = 1
        take = 1
        input_flags = 203
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 201
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 268:
        valid = 1
        take = 1
        input_flags = 204
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 202
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 269:
        valid = 1
        take = 1
        input_flags = 205
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 203
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 270:
        valid = 1
        take = 1
        input_flags = 206
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 204
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 271:
        valid = 1
        take = 1
        input_flags = 207
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 205
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 272:
        valid = 1
        take = 1
        input_flags = 208
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 206
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 273:
        valid = 1
        take = 1
        input_flags = 209
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 207
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 274:
        valid = 1
        take = 1
        input_flags = 210
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 208
        expected_data_index = 4
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 275:
        valid = 1
        take = 1
        input_flags = 211
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 209
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 276:
        valid = 1
        take = 1
        input_flags = 212
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 210
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 277:
        valid = 1
        take = 1
        input_flags = 213
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 211
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 278:
        valid = 1
        take = 1
        input_flags = 214
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 212
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 279:
        valid = 1
        take = 1
        input_flags = 215
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 213
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 280:
        valid = 1
        take = 1
        input_flags = 216
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 214
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 281:
        valid = 1
        take = 1
        input_flags = 217
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 215
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 282:
        valid = 1
        take = 1
        input_flags = 218
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 216
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 283:
        valid = 1
        take = 1
        input_flags = 219
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 217
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 284:
        valid = 1
        take = 1
        input_flags = 220
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 218
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 285:
        valid = 1
        take = 1
        input_flags = 221
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 219
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 286:
        valid = 1
        take = 1
        input_flags = 222
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 220
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 287:
        valid = 1
        take = 1
        input_flags = 223
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 221
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 288:
        valid = 1
        take = 1
        input_flags = 224
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 222
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 289:
        valid = 1
        take = 1
        input_flags = 225
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 223
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 290:
        valid = 1
        take = 1
        input_flags = 226
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 224
        expected_data_index = 5
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 291:
        valid = 1
        take = 1
        input_flags = 227
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 225
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 292:
        valid = 1
        take = 1
        input_flags = 228
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 226
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 293:
        valid = 1
        take = 1
        input_flags = 229
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 227
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 294:
        valid = 1
        take = 1
        input_flags = 230
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 228
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 295:
        valid = 1
        take = 1
        input_flags = 231
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 229
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 296:
        valid = 1
        take = 1
        input_flags = 232
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 230
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 297:
        valid = 1
        take = 1
        input_flags = 233
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 231
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 298:
        valid = 1
        take = 1
        input_flags = 234
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 232
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 299:
        valid = 1
        take = 1
        input_flags = 235
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 233
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 300:
        valid = 1
        take = 1
        input_flags = 236
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 234
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 301:
        valid = 1
        take = 1
        input_flags = 237
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 235
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 302:
        valid = 1
        take = 1
        input_flags = 238
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 236
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 303:
        valid = 1
        take = 1
        input_flags = 239
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 237
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 304:
        valid = 1
        take = 1
        input_flags = 240
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 238
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 305:
        valid = 1
        take = 1
        input_flags = 241
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 239
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 306:
        valid = 1
        take = 1
        input_flags = 242
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 240
        expected_data_index = 4
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 307:
        valid = 1
        take = 1
        input_flags = 243
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 241
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 308:
        valid = 1
        take = 1
        input_flags = 244
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 242
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 309:
        valid = 1
        take = 1
        input_flags = 245
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 243
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 310:
        valid = 1
        take = 1
        input_flags = 246
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 244
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 311:
        valid = 1
        take = 1
        input_flags = 247
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 245
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 312:
        valid = 1
        take = 1
        input_flags = 248
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 246
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 313:
        valid = 1
        take = 1
        input_flags = 249
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 247
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 314:
        valid = 1
        take = 1
        input_flags = 250
        input_index = 5
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 248
        expected_data_index = 3
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 315:
        valid = 1
        take = 1
        input_flags = 251
        input_index = 6
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 249
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 316:
        valid = 1
        take = 1
        input_flags = 252
        input_index = 7
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 250
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 317:
        valid = 1
        take = 1
        input_flags = 253
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 251
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 318:
        valid = 1
        take = 1
        input_flags = 254
        input_index = 1
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 252
        expected_data_index = 2
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 319:
        valid = 1
        take = 1
        input_flags = 255
        input_index = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 253
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 320:
        take = 1
        input_index = 3
        input_conflict = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 254
        expected_data_index = 1
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 321:
        take = 1
        input_flags = 1
        input_index = 4
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_flags = 255
        expected_data_valid = 1
        expected_data_conflict = 1
    if phase == 322:
        take = 1
        input_flags = 2
        input_index = 5
        input_conflict = 1
        expected_ready = 1
    if phase == 323:
        take = 1
        input_flags = 3
        input_index = 6
        input_valid = 1
        expected_ready = 1
    if phase == 324:
        take = 1
        input_flags = 4
        input_index = 7
        input_conflict = 1
        expected_ready = 1
    if phase == 325:
        take = 1
        input_flags = 5
        input_valid = 1
        expected_ready = 1
    if phase == 326:
        take = 1
        input_flags = 6
        input_index = 1
        input_conflict = 1
        expected_ready = 1
    if phase == 327:
        take = 1
        input_flags = 7
        input_index = 2
        input_valid = 1
        expected_ready = 1
    if phase == 328:
        take = 1
        input_flags = 8
        input_index = 3
        input_conflict = 1
        expected_ready = 1
    if phase == 329:
        take = 1
        input_flags = 9
        input_index = 4
        input_valid = 1
        expected_ready = 1
    if phase == 330:
        take = 1
        input_flags = 10
        input_index = 5
        input_conflict = 1
        expected_ready = 1
    if phase == 331:
        take = 1
        input_flags = 11
        input_index = 6
        input_valid = 1
        expected_ready = 1
    if phase == 332:
        take = 1
        input_flags = 12
        input_index = 7
        input_conflict = 1
        expected_ready = 1
    if phase == 333:
        take = 1
        input_flags = 13
        input_valid = 1
        expected_ready = 1
    if phase == 334:
        take = 1
        input_flags = 14
        input_index = 1
        input_conflict = 1
        expected_ready = 1
    if phase == 335:
        take = 1
        input_flags = 15
        input_index = 2
        input_valid = 1
        expected_ready = 1
    if phase == 336:
        take = 1
        input_flags = 16
        input_index = 3
        input_conflict = 1
        expected_ready = 1
    if phase == 337:
        take = 1
        input_flags = 17
        input_index = 4
        input_valid = 1
        expected_ready = 1
    if phase == 338:
        take = 1
        input_flags = 18
        input_index = 5
        input_conflict = 1
        expected_ready = 1
    if phase == 339:
        take = 1
        input_flags = 19
        input_index = 6
        input_valid = 1
        expected_ready = 1
    if phase == 340:
        take = 1
        input_flags = 20
        input_index = 7
        input_conflict = 1
        expected_ready = 1
    if phase == 341:
        take = 1
        input_flags = 21
        input_valid = 1
        expected_ready = 1
    if phase == 342:
        take = 1
        input_flags = 22
        input_index = 1
        input_conflict = 1
        expected_ready = 1
    if phase == 343:
        take = 1
        input_flags = 23
        input_index = 2
        input_valid = 1
        expected_ready = 1
    return Stimulus(
        valid=valid,
        take=take,
        input_flags=input_flags,
        input_index=input_index,
        input_valid=input_valid,
        input_conflict=input_conflict,
        expected_ready=expected_ready,
        expected_valid=expected_valid,
        expected_data_flags=expected_data_flags,
        expected_data_index=expected_data_index,
        expected_data_valid=expected_data_valid,
        expected_data_conflict=expected_data_conflict,
    )


@rule
def advance(phase):
    if phase < 343:
        phase = phase + 1


@system
def OnehotEncodeSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = EncodedFlags(
        flags=frame.input_flags,
        index=frame.input_index,
        valid=frame.input_valid,
        conflict=frame.input_conflict,
    )
    dut = OnehotEncode(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "onehot_encode input capacity"
        assert dut.valid == frame.expected_valid, "onehot_encode result availability"
        assert (
            dut.data.flags == frame.expected_data_flags
        ), "onehot_encode flags old-state check"
        assert (
            dut.data.index == frame.expected_data_index
        ), "onehot_encode index old-state check"
        assert (
            dut.data.valid == frame.expected_data_valid
        ), "onehot_encode valid old-state check"
        assert (
            dut.data.conflict == frame.expected_data_conflict
        ), "onehot_encode conflict old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "flags", dut.data.flags)
        log("info", "index", dut.data.index)
        log("info", "valid", dut.data.valid)
        log("info", "conflict", dut.data.conflict)

    advance(phase)
    check()

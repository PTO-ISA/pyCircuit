"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_masked_decode_pipeline.masked_decode_pipeline import (
    MaskedDecodePipeline,
    Instruction,
    Result,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_opcode: bits[4]
    input_is_compute: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_opcode: bits[4]
    expected_data_is_compute: bits[1]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    take: bits[1] = 0
    input_opcode: bits[4] = 0
    input_is_compute: bits[1] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_data_opcode: bits[4] = 0
    expected_data_is_compute: bits[1] = 0
    if phase == 0:
        valid = 1
        input_is_compute = 1
        expected_ready = 1
    if phase == 1:
        valid = 1
        input_opcode = 1
        expected_ready = 1
    if phase == 2:
        valid = 1
        input_opcode = 2
        input_is_compute = 1
        expected_valid = 1
    if phase == 3:
        valid = 1
        input_opcode = 3
        expected_valid = 1
    if phase == 4:
        valid = 1
        input_opcode = 4
        input_is_compute = 1
        expected_valid = 1
    if phase == 5:
        valid = 1
        input_opcode = 5
        expected_valid = 1
    if phase == 6:
        valid = 1
        input_opcode = 6
        input_is_compute = 1
        expected_valid = 1
    if phase == 7:
        valid = 1
        input_opcode = 7
        expected_valid = 1
    if phase == 8:
        valid = 1
        input_opcode = 8
        input_is_compute = 1
        expected_valid = 1
    if phase == 9:
        valid = 1
        input_opcode = 9
        expected_valid = 1
    if phase == 10:
        valid = 1
        input_opcode = 10
        input_is_compute = 1
        expected_valid = 1
    if phase == 11:
        valid = 1
        input_opcode = 11
        expected_valid = 1
    if phase == 12:
        valid = 1
        input_opcode = 12
        input_is_compute = 1
        expected_valid = 1
    if phase == 13:
        valid = 1
        take = 1
        input_opcode = 13
        expected_ready = 1
        expected_valid = 1
    if phase == 14:
        valid = 1
        take = 1
        input_opcode = 14
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 1
    if phase == 15:
        valid = 1
        take = 1
        input_opcode = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 13
    if phase == 16:
        valid = 1
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 14
        expected_data_is_compute = 1
    if phase == 17:
        valid = 1
        take = 1
        input_opcode = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 14
        expected_data_is_compute = 1
    if phase == 18:
        valid = 1
        take = 1
        input_opcode = 2
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 19:
        valid = 1
        take = 1
        input_opcode = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 1
    if phase == 20:
        valid = 1
        input_opcode = 4
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 2
    if phase == 21:
        valid = 1
        take = 1
        input_opcode = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 2
    if phase == 22:
        valid = 1
        take = 1
        input_opcode = 6
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 3
    if phase == 23:
        valid = 1
        take = 1
        input_opcode = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 5
    if phase == 24:
        valid = 1
        input_opcode = 8
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 6
    if phase == 25:
        valid = 1
        take = 1
        input_opcode = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 6
    if phase == 26:
        valid = 1
        take = 1
        input_opcode = 10
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 7
    if phase == 27:
        valid = 1
        take = 1
        input_opcode = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 9
    if phase == 28:
        valid = 1
        input_opcode = 12
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 10
        expected_data_is_compute = 1
    if phase == 29:
        valid = 1
        take = 1
        input_opcode = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 10
        expected_data_is_compute = 1
    if phase == 30:
        valid = 1
        take = 1
        input_opcode = 14
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 11
    if phase == 31:
        take = 1
        input_opcode = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 13
    if phase == 32:
        valid = 1
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 14
        expected_data_is_compute = 1
    if phase == 33:
        valid = 1
        take = 1
        input_opcode = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 14
        expected_data_is_compute = 1
    if phase == 34:
        valid = 1
        take = 1
        input_opcode = 2
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 35:
        valid = 1
        take = 1
        input_opcode = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 1
    if phase == 36:
        input_opcode = 4
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 2
    if phase == 37:
        valid = 1
        take = 1
        input_opcode = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 2
    if phase == 38:
        valid = 1
        take = 1
        input_opcode = 6
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 3
    if phase == 39:
        valid = 1
        take = 1
        input_opcode = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 5
    if phase == 40:
        valid = 1
        input_opcode = 8
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 6
    if phase == 41:
        take = 1
        input_opcode = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 6
    if phase == 42:
        valid = 1
        take = 1
        input_opcode = 10
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 7
    if phase == 43:
        valid = 1
        take = 1
        input_opcode = 11
        expected_ready = 1
    if phase == 44:
        valid = 1
        input_opcode = 12
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 10
        expected_data_is_compute = 1
    if phase == 45:
        valid = 1
        take = 1
        input_opcode = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 10
        expected_data_is_compute = 1
    if phase == 46:
        take = 1
        input_opcode = 14
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 11
    if phase == 47:
        valid = 1
        take = 1
        input_opcode = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 13
    if phase == 48:
        valid = 1
        input_is_compute = 1
        expected_ready = 1
    if phase == 49:
        valid = 1
        input_opcode = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 50:
        valid = 1
        input_opcode = 2
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 51:
        input_opcode = 3
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 52:
        valid = 1
        input_opcode = 4
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 53:
        valid = 1
        input_opcode = 5
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 54:
        valid = 1
        input_opcode = 6
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 55:
        valid = 1
        input_opcode = 7
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 56:
        input_opcode = 8
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 57:
        valid = 1
        input_opcode = 9
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 58:
        valid = 1
        input_opcode = 10
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 59:
        valid = 1
        input_opcode = 11
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 60:
        valid = 1
        input_opcode = 12
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 61:
        input_opcode = 13
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 62:
        valid = 1
        input_opcode = 14
        input_is_compute = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 63:
        valid = 1
        input_opcode = 15
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 64:
        valid = 1
        take = 1
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 65:
        valid = 1
        take = 1
        input_opcode = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 66:
        valid = 1
        take = 1
        input_opcode = 2
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 67:
        valid = 1
        take = 1
        input_opcode = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 1
    if phase == 68:
        valid = 1
        take = 1
        input_opcode = 4
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 2
    if phase == 69:
        valid = 1
        take = 1
        input_opcode = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 3
    if phase == 70:
        valid = 1
        take = 1
        input_opcode = 6
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 4
    if phase == 71:
        valid = 1
        take = 1
        input_opcode = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 5
    if phase == 72:
        valid = 1
        take = 1
        input_opcode = 8
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 6
    if phase == 73:
        valid = 1
        take = 1
        input_opcode = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 7
    if phase == 74:
        valid = 1
        take = 1
        input_opcode = 10
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 8
        expected_data_is_compute = 1
    if phase == 75:
        valid = 1
        take = 1
        input_opcode = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 9
    if phase == 76:
        valid = 1
        take = 1
        input_opcode = 12
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 10
        expected_data_is_compute = 1
    if phase == 77:
        valid = 1
        take = 1
        input_opcode = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 11
    if phase == 78:
        valid = 1
        take = 1
        input_opcode = 14
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 12
        expected_data_is_compute = 1
    if phase == 79:
        valid = 1
        take = 1
        input_opcode = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 13
    if phase == 80:
        valid = 1
        take = 1
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 14
        expected_data_is_compute = 1
    if phase == 81:
        valid = 1
        take = 1
        input_opcode = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 82:
        valid = 1
        take = 1
        input_opcode = 2
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 83:
        valid = 1
        take = 1
        input_opcode = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 1
    if phase == 84:
        valid = 1
        take = 1
        input_opcode = 4
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 2
    if phase == 85:
        valid = 1
        take = 1
        input_opcode = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 3
    if phase == 86:
        valid = 1
        take = 1
        input_opcode = 6
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 4
    if phase == 87:
        valid = 1
        take = 1
        input_opcode = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 5
    if phase == 88:
        valid = 1
        take = 1
        input_opcode = 8
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 6
    if phase == 89:
        valid = 1
        take = 1
        input_opcode = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 7
    if phase == 90:
        valid = 1
        take = 1
        input_opcode = 10
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 8
        expected_data_is_compute = 1
    if phase == 91:
        valid = 1
        take = 1
        input_opcode = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 9
    if phase == 92:
        valid = 1
        take = 1
        input_opcode = 12
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 10
        expected_data_is_compute = 1
    if phase == 93:
        valid = 1
        take = 1
        input_opcode = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 11
    if phase == 94:
        valid = 1
        take = 1
        input_opcode = 14
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 12
        expected_data_is_compute = 1
    if phase == 95:
        valid = 1
        take = 1
        input_opcode = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 13
    if phase == 96:
        valid = 1
        take = 1
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 14
        expected_data_is_compute = 1
    if phase == 97:
        valid = 1
        take = 1
        input_opcode = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 98:
        valid = 1
        take = 1
        input_opcode = 2
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 99:
        valid = 1
        take = 1
        input_opcode = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 1
    if phase == 100:
        valid = 1
        take = 1
        input_opcode = 4
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 2
    if phase == 101:
        valid = 1
        take = 1
        input_opcode = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 3
    if phase == 102:
        valid = 1
        take = 1
        input_opcode = 6
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 4
    if phase == 103:
        valid = 1
        take = 1
        input_opcode = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 5
    if phase == 104:
        valid = 1
        take = 1
        input_opcode = 8
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 6
    if phase == 105:
        valid = 1
        take = 1
        input_opcode = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 7
    if phase == 106:
        valid = 1
        take = 1
        input_opcode = 10
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 8
        expected_data_is_compute = 1
    if phase == 107:
        valid = 1
        take = 1
        input_opcode = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 9
    if phase == 108:
        valid = 1
        take = 1
        input_opcode = 12
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 10
        expected_data_is_compute = 1
    if phase == 109:
        valid = 1
        take = 1
        input_opcode = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 11
    if phase == 110:
        valid = 1
        take = 1
        input_opcode = 14
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 12
        expected_data_is_compute = 1
    if phase == 111:
        valid = 1
        take = 1
        input_opcode = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 13
    if phase == 112:
        valid = 1
        take = 1
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 14
        expected_data_is_compute = 1
    if phase == 113:
        valid = 1
        take = 1
        input_opcode = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 114:
        valid = 1
        take = 1
        input_opcode = 2
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 115:
        valid = 1
        take = 1
        input_opcode = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 1
    if phase == 116:
        valid = 1
        take = 1
        input_opcode = 4
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 2
    if phase == 117:
        valid = 1
        take = 1
        input_opcode = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 3
    if phase == 118:
        valid = 1
        take = 1
        input_opcode = 6
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 4
    if phase == 119:
        valid = 1
        take = 1
        input_opcode = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 5
    if phase == 120:
        valid = 1
        take = 1
        input_opcode = 8
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 6
    if phase == 121:
        valid = 1
        take = 1
        input_opcode = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 7
    if phase == 122:
        valid = 1
        take = 1
        input_opcode = 10
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 8
        expected_data_is_compute = 1
    if phase == 123:
        valid = 1
        take = 1
        input_opcode = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 9
    if phase == 124:
        valid = 1
        take = 1
        input_opcode = 12
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 10
        expected_data_is_compute = 1
    if phase == 125:
        valid = 1
        take = 1
        input_opcode = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 11
    if phase == 126:
        valid = 1
        take = 1
        input_opcode = 14
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 12
        expected_data_is_compute = 1
    if phase == 127:
        valid = 1
        take = 1
        input_opcode = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 13
    if phase == 128:
        valid = 1
        take = 1
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 14
        expected_data_is_compute = 1
    if phase == 129:
        valid = 1
        take = 1
        input_opcode = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 130:
        valid = 1
        take = 1
        input_opcode = 2
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 131:
        valid = 1
        take = 1
        input_opcode = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 1
    if phase == 132:
        valid = 1
        take = 1
        input_opcode = 4
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 2
    if phase == 133:
        valid = 1
        take = 1
        input_opcode = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 3
    if phase == 134:
        valid = 1
        take = 1
        input_opcode = 6
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 4
    if phase == 135:
        valid = 1
        take = 1
        input_opcode = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 5
    if phase == 136:
        valid = 1
        take = 1
        input_opcode = 8
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 6
    if phase == 137:
        valid = 1
        take = 1
        input_opcode = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 7
    if phase == 138:
        valid = 1
        take = 1
        input_opcode = 10
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 8
        expected_data_is_compute = 1
    if phase == 139:
        valid = 1
        take = 1
        input_opcode = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 9
    if phase == 140:
        valid = 1
        take = 1
        input_opcode = 12
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 10
        expected_data_is_compute = 1
    if phase == 141:
        valid = 1
        take = 1
        input_opcode = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 11
    if phase == 142:
        valid = 1
        take = 1
        input_opcode = 14
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 12
        expected_data_is_compute = 1
    if phase == 143:
        valid = 1
        take = 1
        input_opcode = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 13
    if phase == 144:
        valid = 1
        take = 1
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 14
        expected_data_is_compute = 1
    if phase == 145:
        valid = 1
        take = 1
        input_opcode = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 146:
        valid = 1
        take = 1
        input_opcode = 2
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 147:
        valid = 1
        take = 1
        input_opcode = 3
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 1
    if phase == 148:
        valid = 1
        take = 1
        input_opcode = 4
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 2
    if phase == 149:
        valid = 1
        take = 1
        input_opcode = 5
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 3
    if phase == 150:
        valid = 1
        take = 1
        input_opcode = 6
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 4
    if phase == 151:
        valid = 1
        take = 1
        input_opcode = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 5
    if phase == 152:
        valid = 1
        take = 1
        input_opcode = 8
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 6
    if phase == 153:
        valid = 1
        take = 1
        input_opcode = 9
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 7
    if phase == 154:
        valid = 1
        take = 1
        input_opcode = 10
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 8
        expected_data_is_compute = 1
    if phase == 155:
        valid = 1
        take = 1
        input_opcode = 11
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 9
    if phase == 156:
        valid = 1
        take = 1
        input_opcode = 12
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 10
        expected_data_is_compute = 1
    if phase == 157:
        valid = 1
        take = 1
        input_opcode = 13
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 11
    if phase == 158:
        valid = 1
        take = 1
        input_opcode = 14
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 12
        expected_data_is_compute = 1
    if phase == 159:
        valid = 1
        take = 1
        input_opcode = 15
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 13
    if phase == 160:
        take = 1
        input_is_compute = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 14
        expected_data_is_compute = 1
    if phase == 161:
        take = 1
        input_opcode = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_opcode = 15
    if phase == 162:
        take = 1
        input_opcode = 2
        input_is_compute = 1
        expected_ready = 1
    if phase == 163:
        take = 1
        input_opcode = 3
        expected_ready = 1
    if phase == 164:
        take = 1
        input_opcode = 4
        input_is_compute = 1
        expected_ready = 1
    if phase == 165:
        take = 1
        input_opcode = 5
        expected_ready = 1
    if phase == 166:
        take = 1
        input_opcode = 6
        input_is_compute = 1
        expected_ready = 1
    if phase == 167:
        take = 1
        input_opcode = 7
        expected_ready = 1
    if phase == 168:
        take = 1
        input_opcode = 8
        input_is_compute = 1
        expected_ready = 1
    if phase == 169:
        take = 1
        input_opcode = 9
        expected_ready = 1
    if phase == 170:
        take = 1
        input_opcode = 10
        input_is_compute = 1
        expected_ready = 1
    if phase == 171:
        take = 1
        input_opcode = 11
        expected_ready = 1
    if phase == 172:
        take = 1
        input_opcode = 12
        input_is_compute = 1
        expected_ready = 1
    if phase == 173:
        take = 1
        input_opcode = 13
        expected_ready = 1
    if phase == 174:
        take = 1
        input_opcode = 14
        input_is_compute = 1
        expected_ready = 1
    if phase == 175:
        take = 1
        input_opcode = 15
        expected_ready = 1
    if phase == 176:
        take = 1
        input_is_compute = 1
        expected_ready = 1
    if phase == 177:
        take = 1
        input_opcode = 1
        expected_ready = 1
    if phase == 178:
        take = 1
        input_opcode = 2
        input_is_compute = 1
        expected_ready = 1
    if phase == 179:
        take = 1
        input_opcode = 3
        expected_ready = 1
    if phase == 180:
        take = 1
        input_opcode = 4
        input_is_compute = 1
        expected_ready = 1
    if phase == 181:
        take = 1
        input_opcode = 5
        expected_ready = 1
    if phase == 182:
        take = 1
        input_opcode = 6
        input_is_compute = 1
        expected_ready = 1
    if phase == 183:
        take = 1
        input_opcode = 7
        expected_ready = 1
    return Stimulus(
        valid=valid,
        take=take,
        input_opcode=input_opcode,
        input_is_compute=input_is_compute,
        expected_ready=expected_ready,
        expected_valid=expected_valid,
        expected_data_opcode=expected_data_opcode,
        expected_data_is_compute=expected_data_is_compute,
    )


@rule
def advance(phase):
    if phase < 183:
        phase = phase + 1


@system
def MaskedDecodePipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Instruction(opcode=frame.input_opcode, is_compute=frame.input_is_compute)
    dut = MaskedDecodePipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert (
            dut.ready == frame.expected_ready
        ), "masked_decode_pipeline input capacity"
        assert (
            dut.valid == frame.expected_valid
        ), "masked_decode_pipeline result availability"
        assert (
            dut.data.opcode == frame.expected_data_opcode
        ), "masked_decode_pipeline opcode old-state check"
        assert (
            dut.data.is_compute == frame.expected_data_is_compute
        ), "masked_decode_pipeline is_compute old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "opcode", dut.data.opcode)
        log("info", "is_compute", dut.data.is_compute)

    advance(phase)
    check()

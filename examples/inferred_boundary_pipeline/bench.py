"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_inferred_boundary_pipeline.inferred_boundary_pipeline import (
    InferredBoundaryPipeline,
)
from pycircuit import bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    data: bits[8]
    take: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data: bits[8]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    data: bits[8] = 0
    take: bits[1] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_data: bits[8] = 0
    if phase == 0:
        valid = 1
        expected_ready = 1
    if phase == 1:
        valid = 1
        data = 255
        expected_ready = 1
    if phase == 2:
        valid = 1
        data = 254
        expected_valid = 1
        expected_data = 1
    if phase == 3:
        valid = 1
        data = 128
        expected_valid = 1
        expected_data = 1
    if phase == 4:
        valid = 1
        data = 127
        expected_valid = 1
        expected_data = 1
    if phase == 5:
        valid = 1
        data = 170
        expected_valid = 1
        expected_data = 1
    if phase == 6:
        valid = 1
        data = 85
        expected_valid = 1
        expected_data = 1
    if phase == 7:
        valid = 1
        data = 1
        expected_valid = 1
        expected_data = 1
    if phase == 8:
        valid = 1
        data = 2
        expected_valid = 1
        expected_data = 1
    if phase == 9:
        valid = 1
        data = 4
        expected_valid = 1
        expected_data = 1
    if phase == 10:
        valid = 1
        data = 8
        expected_valid = 1
        expected_data = 1
    if phase == 11:
        valid = 1
        data = 16
        expected_valid = 1
        expected_data = 1
    if phase == 12:
        valid = 1
        data = 32
        expected_valid = 1
        expected_data = 1
    if phase == 13:
        valid = 1
        data = 64
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1
    if phase == 14:
        valid = 1
        data = 128
        take = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 15:
        valid = 1
        data = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 65
    if phase == 16:
        valid = 1
        data = 2
        expected_valid = 1
        expected_data = 129
    if phase == 17:
        valid = 1
        data = 4
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 129
    if phase == 18:
        valid = 1
        data = 8
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2
    if phase == 19:
        valid = 1
        data = 16
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 5
    if phase == 20:
        valid = 1
        data = 32
        expected_valid = 1
        expected_data = 9
    if phase == 21:
        valid = 1
        data = 64
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 9
    if phase == 22:
        valid = 1
        data = 128
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 17
    if phase == 23:
        valid = 1
        data = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 65
    if phase == 24:
        valid = 1
        data = 2
        expected_valid = 1
        expected_data = 129
    if phase == 25:
        valid = 1
        data = 4
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 129
    if phase == 26:
        valid = 1
        data = 8
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2
    if phase == 27:
        valid = 1
        data = 16
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 5
    if phase == 28:
        valid = 1
        data = 32
        expected_valid = 1
        expected_data = 9
    if phase == 29:
        valid = 1
        data = 64
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 9
    if phase == 30:
        valid = 1
        data = 128
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 17
    if phase == 31:
        data = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 65
    if phase == 32:
        valid = 1
        data = 2
        expected_ready = 1
        expected_valid = 1
        expected_data = 129
    if phase == 33:
        valid = 1
        data = 4
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 129
    if phase == 34:
        valid = 1
        data = 8
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 3
    if phase == 35:
        valid = 1
        data = 16
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 5
    if phase == 36:
        data = 32
        expected_valid = 1
        expected_data = 9
    if phase == 37:
        valid = 1
        data = 64
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 9
    if phase == 38:
        valid = 1
        data = 128
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 17
    if phase == 39:
        valid = 1
        data = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 65
    if phase == 40:
        valid = 1
        data = 2
        expected_valid = 1
        expected_data = 129
    if phase == 41:
        data = 4
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 129
    if phase == 42:
        valid = 1
        data = 8
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2
    if phase == 43:
        valid = 1
        data = 16
        take = 1
        expected_ready = 1
    if phase == 44:
        valid = 1
        data = 32
        expected_valid = 1
        expected_data = 9
    if phase == 45:
        valid = 1
        data = 64
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 9
    if phase == 46:
        data = 128
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 17
    if phase == 47:
        valid = 1
        data = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 65
    if phase == 48:
        valid = 1
        data = 2
        expected_ready = 1
    if phase == 49:
        valid = 1
        data = 4
        expected_valid = 1
        expected_data = 2
    if phase == 50:
        valid = 1
        data = 8
        expected_valid = 1
        expected_data = 2
    if phase == 51:
        data = 16
        expected_valid = 1
        expected_data = 2
    if phase == 52:
        valid = 1
        data = 32
        expected_valid = 1
        expected_data = 2
    if phase == 53:
        valid = 1
        data = 64
        expected_valid = 1
        expected_data = 2
    if phase == 54:
        valid = 1
        data = 128
        expected_valid = 1
        expected_data = 2
    if phase == 55:
        valid = 1
        data = 1
        expected_valid = 1
        expected_data = 2
    if phase == 56:
        data = 2
        expected_valid = 1
        expected_data = 2
    if phase == 57:
        valid = 1
        data = 4
        expected_valid = 1
        expected_data = 2
    if phase == 58:
        valid = 1
        data = 8
        expected_valid = 1
        expected_data = 2
    if phase == 59:
        valid = 1
        data = 16
        expected_valid = 1
        expected_data = 2
    if phase == 60:
        valid = 1
        data = 32
        expected_valid = 1
        expected_data = 2
    if phase == 61:
        data = 64
        expected_valid = 1
        expected_data = 2
    if phase == 62:
        valid = 1
        data = 128
        expected_valid = 1
        expected_data = 2
    if phase == 63:
        valid = 1
        data = 1
        expected_valid = 1
        expected_data = 2
    if phase == 64:
        valid = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2
    if phase == 65:
        valid = 1
        data = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 3
    if phase == 66:
        valid = 1
        data = 2
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1
    if phase == 67:
        valid = 1
        data = 3
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2
    if phase == 68:
        valid = 1
        data = 4
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 3
    if phase == 69:
        valid = 1
        data = 5
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 4
    if phase == 70:
        valid = 1
        data = 6
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 5
    if phase == 71:
        valid = 1
        data = 7
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 6
    if phase == 72:
        valid = 1
        data = 8
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 7
    if phase == 73:
        valid = 1
        data = 9
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 8
    if phase == 74:
        valid = 1
        data = 10
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 9
    if phase == 75:
        valid = 1
        data = 11
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 10
    if phase == 76:
        valid = 1
        data = 12
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 11
    if phase == 77:
        valid = 1
        data = 13
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 12
    if phase == 78:
        valid = 1
        data = 14
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 13
    if phase == 79:
        valid = 1
        data = 15
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 14
    if phase == 80:
        valid = 1
        data = 16
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 15
    if phase == 81:
        valid = 1
        data = 17
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 16
    if phase == 82:
        valid = 1
        data = 18
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 17
    if phase == 83:
        valid = 1
        data = 19
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 18
    if phase == 84:
        valid = 1
        data = 20
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 19
    if phase == 85:
        valid = 1
        data = 21
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 20
    if phase == 86:
        valid = 1
        data = 22
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 21
    if phase == 87:
        valid = 1
        data = 23
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 22
    if phase == 88:
        valid = 1
        data = 24
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 23
    if phase == 89:
        valid = 1
        data = 25
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 24
    if phase == 90:
        valid = 1
        data = 26
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 25
    if phase == 91:
        valid = 1
        data = 27
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 26
    if phase == 92:
        valid = 1
        data = 28
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 27
    if phase == 93:
        valid = 1
        data = 29
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 28
    if phase == 94:
        valid = 1
        data = 30
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 29
    if phase == 95:
        valid = 1
        data = 31
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 30
    if phase == 96:
        valid = 1
        data = 32
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 31
    if phase == 97:
        valid = 1
        data = 33
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 32
    if phase == 98:
        valid = 1
        data = 34
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 33
    if phase == 99:
        valid = 1
        data = 35
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 34
    if phase == 100:
        valid = 1
        data = 36
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 35
    if phase == 101:
        valid = 1
        data = 37
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 36
    if phase == 102:
        valid = 1
        data = 38
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 37
    if phase == 103:
        valid = 1
        data = 39
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 38
    if phase == 104:
        valid = 1
        data = 40
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 39
    if phase == 105:
        valid = 1
        data = 41
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 40
    if phase == 106:
        valid = 1
        data = 42
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 41
    if phase == 107:
        valid = 1
        data = 43
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 42
    if phase == 108:
        valid = 1
        data = 44
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 43
    if phase == 109:
        valid = 1
        data = 45
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 44
    if phase == 110:
        valid = 1
        data = 46
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 45
    if phase == 111:
        valid = 1
        data = 47
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 46
    if phase == 112:
        valid = 1
        data = 48
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 47
    if phase == 113:
        valid = 1
        data = 49
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 48
    if phase == 114:
        valid = 1
        data = 50
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 49
    if phase == 115:
        valid = 1
        data = 51
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 50
    if phase == 116:
        valid = 1
        data = 52
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 51
    if phase == 117:
        valid = 1
        data = 53
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 52
    if phase == 118:
        valid = 1
        data = 54
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 53
    if phase == 119:
        valid = 1
        data = 55
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 54
    if phase == 120:
        valid = 1
        data = 56
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 55
    if phase == 121:
        valid = 1
        data = 57
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 56
    if phase == 122:
        valid = 1
        data = 58
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 57
    if phase == 123:
        valid = 1
        data = 59
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 58
    if phase == 124:
        valid = 1
        data = 60
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 59
    if phase == 125:
        valid = 1
        data = 61
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 60
    if phase == 126:
        valid = 1
        data = 62
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 61
    if phase == 127:
        valid = 1
        data = 63
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 62
    if phase == 128:
        valid = 1
        data = 64
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 63
    if phase == 129:
        valid = 1
        data = 65
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 64
    if phase == 130:
        valid = 1
        data = 66
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 65
    if phase == 131:
        valid = 1
        data = 67
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 66
    if phase == 132:
        valid = 1
        data = 68
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 67
    if phase == 133:
        valid = 1
        data = 69
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 68
    if phase == 134:
        valid = 1
        data = 70
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 69
    if phase == 135:
        valid = 1
        data = 71
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 70
    if phase == 136:
        valid = 1
        data = 72
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 71
    if phase == 137:
        valid = 1
        data = 73
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 72
    if phase == 138:
        valid = 1
        data = 74
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 73
    if phase == 139:
        valid = 1
        data = 75
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 74
    if phase == 140:
        valid = 1
        data = 76
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 75
    if phase == 141:
        valid = 1
        data = 77
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 76
    if phase == 142:
        valid = 1
        data = 78
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 77
    if phase == 143:
        valid = 1
        data = 79
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 78
    if phase == 144:
        valid = 1
        data = 80
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 79
    if phase == 145:
        valid = 1
        data = 81
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 80
    if phase == 146:
        valid = 1
        data = 82
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 81
    if phase == 147:
        valid = 1
        data = 83
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 82
    if phase == 148:
        valid = 1
        data = 84
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 83
    if phase == 149:
        valid = 1
        data = 85
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 84
    if phase == 150:
        valid = 1
        data = 86
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 85
    if phase == 151:
        valid = 1
        data = 87
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 86
    if phase == 152:
        valid = 1
        data = 88
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 87
    if phase == 153:
        valid = 1
        data = 89
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 88
    if phase == 154:
        valid = 1
        data = 90
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 89
    if phase == 155:
        valid = 1
        data = 91
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 90
    if phase == 156:
        valid = 1
        data = 92
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 91
    if phase == 157:
        valid = 1
        data = 93
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 92
    if phase == 158:
        valid = 1
        data = 94
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 93
    if phase == 159:
        valid = 1
        data = 95
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 94
    if phase == 160:
        valid = 1
        data = 96
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 95
    if phase == 161:
        valid = 1
        data = 97
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 96
    if phase == 162:
        valid = 1
        data = 98
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 97
    if phase == 163:
        valid = 1
        data = 99
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 98
    if phase == 164:
        valid = 1
        data = 100
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 99
    if phase == 165:
        valid = 1
        data = 101
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 100
    if phase == 166:
        valid = 1
        data = 102
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 101
    if phase == 167:
        valid = 1
        data = 103
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 102
    if phase == 168:
        valid = 1
        data = 104
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 103
    if phase == 169:
        valid = 1
        data = 105
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 104
    if phase == 170:
        valid = 1
        data = 106
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 105
    if phase == 171:
        valid = 1
        data = 107
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 106
    if phase == 172:
        valid = 1
        data = 108
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 107
    if phase == 173:
        valid = 1
        data = 109
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 108
    if phase == 174:
        valid = 1
        data = 110
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 109
    if phase == 175:
        valid = 1
        data = 111
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 110
    if phase == 176:
        valid = 1
        data = 112
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 111
    if phase == 177:
        valid = 1
        data = 113
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 112
    if phase == 178:
        valid = 1
        data = 114
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 113
    if phase == 179:
        valid = 1
        data = 115
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 114
    if phase == 180:
        valid = 1
        data = 116
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 115
    if phase == 181:
        valid = 1
        data = 117
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 116
    if phase == 182:
        valid = 1
        data = 118
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 117
    if phase == 183:
        valid = 1
        data = 119
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 118
    if phase == 184:
        valid = 1
        data = 120
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 119
    if phase == 185:
        valid = 1
        data = 121
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 120
    if phase == 186:
        valid = 1
        data = 122
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 121
    if phase == 187:
        valid = 1
        data = 123
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 122
    if phase == 188:
        valid = 1
        data = 124
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 123
    if phase == 189:
        valid = 1
        data = 125
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 124
    if phase == 190:
        valid = 1
        data = 126
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 125
    if phase == 191:
        valid = 1
        data = 127
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 126
    if phase == 192:
        valid = 1
        data = 128
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 127
    if phase == 193:
        valid = 1
        data = 129
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 128
    if phase == 194:
        valid = 1
        data = 130
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 129
    if phase == 195:
        valid = 1
        data = 131
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 130
    if phase == 196:
        valid = 1
        data = 132
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 131
    if phase == 197:
        valid = 1
        data = 133
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 132
    if phase == 198:
        valid = 1
        data = 134
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 133
    if phase == 199:
        valid = 1
        data = 135
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 134
    if phase == 200:
        valid = 1
        data = 136
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 135
    if phase == 201:
        valid = 1
        data = 137
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 136
    if phase == 202:
        valid = 1
        data = 138
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 137
    if phase == 203:
        valid = 1
        data = 139
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 138
    if phase == 204:
        valid = 1
        data = 140
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 139
    if phase == 205:
        valid = 1
        data = 141
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 140
    if phase == 206:
        valid = 1
        data = 142
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 141
    if phase == 207:
        valid = 1
        data = 143
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 142
    if phase == 208:
        valid = 1
        data = 144
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 143
    if phase == 209:
        valid = 1
        data = 145
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 144
    if phase == 210:
        valid = 1
        data = 146
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 145
    if phase == 211:
        valid = 1
        data = 147
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 146
    if phase == 212:
        valid = 1
        data = 148
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 147
    if phase == 213:
        valid = 1
        data = 149
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 148
    if phase == 214:
        valid = 1
        data = 150
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 149
    if phase == 215:
        valid = 1
        data = 151
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 150
    if phase == 216:
        valid = 1
        data = 152
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 151
    if phase == 217:
        valid = 1
        data = 153
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 152
    if phase == 218:
        valid = 1
        data = 154
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 153
    if phase == 219:
        valid = 1
        data = 155
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 154
    if phase == 220:
        valid = 1
        data = 156
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 155
    if phase == 221:
        valid = 1
        data = 157
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 156
    if phase == 222:
        valid = 1
        data = 158
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 157
    if phase == 223:
        valid = 1
        data = 159
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 158
    if phase == 224:
        valid = 1
        data = 160
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 159
    if phase == 225:
        valid = 1
        data = 161
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 160
    if phase == 226:
        valid = 1
        data = 162
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 161
    if phase == 227:
        valid = 1
        data = 163
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 162
    if phase == 228:
        valid = 1
        data = 164
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 163
    if phase == 229:
        valid = 1
        data = 165
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 164
    if phase == 230:
        valid = 1
        data = 166
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 165
    if phase == 231:
        valid = 1
        data = 167
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 166
    if phase == 232:
        valid = 1
        data = 168
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 167
    if phase == 233:
        valid = 1
        data = 169
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 168
    if phase == 234:
        valid = 1
        data = 170
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 169
    if phase == 235:
        valid = 1
        data = 171
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 170
    if phase == 236:
        valid = 1
        data = 172
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 171
    if phase == 237:
        valid = 1
        data = 173
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 172
    if phase == 238:
        valid = 1
        data = 174
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 173
    if phase == 239:
        valid = 1
        data = 175
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 174
    if phase == 240:
        valid = 1
        data = 176
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 175
    if phase == 241:
        valid = 1
        data = 177
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 176
    if phase == 242:
        valid = 1
        data = 178
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 177
    if phase == 243:
        valid = 1
        data = 179
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 178
    if phase == 244:
        valid = 1
        data = 180
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 179
    if phase == 245:
        valid = 1
        data = 181
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 180
    if phase == 246:
        valid = 1
        data = 182
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 181
    if phase == 247:
        valid = 1
        data = 183
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 182
    if phase == 248:
        valid = 1
        data = 184
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 183
    if phase == 249:
        valid = 1
        data = 185
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 184
    if phase == 250:
        valid = 1
        data = 186
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 185
    if phase == 251:
        valid = 1
        data = 187
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 186
    if phase == 252:
        valid = 1
        data = 188
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 187
    if phase == 253:
        valid = 1
        data = 189
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 188
    if phase == 254:
        valid = 1
        data = 190
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 189
    if phase == 255:
        valid = 1
        data = 191
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 190
    if phase == 256:
        valid = 1
        data = 192
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 191
    if phase == 257:
        valid = 1
        data = 193
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 192
    if phase == 258:
        valid = 1
        data = 194
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 193
    if phase == 259:
        valid = 1
        data = 195
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 194
    if phase == 260:
        valid = 1
        data = 196
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 195
    if phase == 261:
        valid = 1
        data = 197
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 196
    if phase == 262:
        valid = 1
        data = 198
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 197
    if phase == 263:
        valid = 1
        data = 199
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 198
    if phase == 264:
        valid = 1
        data = 200
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 199
    if phase == 265:
        valid = 1
        data = 201
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 200
    if phase == 266:
        valid = 1
        data = 202
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 201
    if phase == 267:
        valid = 1
        data = 203
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 202
    if phase == 268:
        valid = 1
        data = 204
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 203
    if phase == 269:
        valid = 1
        data = 205
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 204
    if phase == 270:
        valid = 1
        data = 206
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 205
    if phase == 271:
        valid = 1
        data = 207
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 206
    if phase == 272:
        valid = 1
        data = 208
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 207
    if phase == 273:
        valid = 1
        data = 209
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 208
    if phase == 274:
        valid = 1
        data = 210
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 209
    if phase == 275:
        valid = 1
        data = 211
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 210
    if phase == 276:
        valid = 1
        data = 212
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 211
    if phase == 277:
        valid = 1
        data = 213
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 212
    if phase == 278:
        valid = 1
        data = 214
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 213
    if phase == 279:
        valid = 1
        data = 215
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 214
    if phase == 280:
        valid = 1
        data = 216
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 215
    if phase == 281:
        valid = 1
        data = 217
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 216
    if phase == 282:
        valid = 1
        data = 218
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 217
    if phase == 283:
        valid = 1
        data = 219
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 218
    if phase == 284:
        valid = 1
        data = 220
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 219
    if phase == 285:
        valid = 1
        data = 221
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 220
    if phase == 286:
        valid = 1
        data = 222
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 221
    if phase == 287:
        valid = 1
        data = 223
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 222
    if phase == 288:
        valid = 1
        data = 224
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 223
    if phase == 289:
        valid = 1
        data = 225
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 224
    if phase == 290:
        valid = 1
        data = 226
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 225
    if phase == 291:
        valid = 1
        data = 227
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 226
    if phase == 292:
        valid = 1
        data = 228
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 227
    if phase == 293:
        valid = 1
        data = 229
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 228
    if phase == 294:
        valid = 1
        data = 230
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 229
    if phase == 295:
        valid = 1
        data = 231
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 230
    if phase == 296:
        valid = 1
        data = 232
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 231
    if phase == 297:
        valid = 1
        data = 233
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 232
    if phase == 298:
        valid = 1
        data = 234
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 233
    if phase == 299:
        valid = 1
        data = 235
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 234
    if phase == 300:
        valid = 1
        data = 236
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 235
    if phase == 301:
        valid = 1
        data = 237
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 236
    if phase == 302:
        valid = 1
        data = 238
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 237
    if phase == 303:
        valid = 1
        data = 239
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 238
    if phase == 304:
        valid = 1
        data = 240
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 239
    if phase == 305:
        valid = 1
        data = 241
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 240
    if phase == 306:
        valid = 1
        data = 242
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 241
    if phase == 307:
        valid = 1
        data = 243
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 242
    if phase == 308:
        valid = 1
        data = 244
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 243
    if phase == 309:
        valid = 1
        data = 245
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 244
    if phase == 310:
        valid = 1
        data = 246
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 245
    if phase == 311:
        valid = 1
        data = 247
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 246
    if phase == 312:
        valid = 1
        data = 248
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 247
    if phase == 313:
        valid = 1
        data = 249
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 248
    if phase == 314:
        valid = 1
        data = 250
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 249
    if phase == 315:
        valid = 1
        data = 251
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 250
    if phase == 316:
        valid = 1
        data = 252
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 251
    if phase == 317:
        valid = 1
        data = 253
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 252
    if phase == 318:
        valid = 1
        data = 254
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 253
    if phase == 319:
        valid = 1
        data = 255
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 254
    if phase == 320:
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 255
    if phase == 321:
        data = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
    if phase == 322:
        data = 2
        take = 1
        expected_ready = 1
    if phase == 323:
        data = 3
        take = 1
        expected_ready = 1
    if phase == 324:
        data = 4
        take = 1
        expected_ready = 1
    if phase == 325:
        data = 5
        take = 1
        expected_ready = 1
    if phase == 326:
        data = 6
        take = 1
        expected_ready = 1
    if phase == 327:
        data = 7
        take = 1
        expected_ready = 1
    if phase == 328:
        data = 8
        take = 1
        expected_ready = 1
    if phase == 329:
        data = 9
        take = 1
        expected_ready = 1
    if phase == 330:
        data = 10
        take = 1
        expected_ready = 1
    if phase == 331:
        data = 11
        take = 1
        expected_ready = 1
    if phase == 332:
        data = 12
        take = 1
        expected_ready = 1
    if phase == 333:
        data = 13
        take = 1
        expected_ready = 1
    if phase == 334:
        data = 14
        take = 1
        expected_ready = 1
    if phase == 335:
        data = 15
        take = 1
        expected_ready = 1
    if phase == 336:
        data = 16
        take = 1
        expected_ready = 1
    if phase == 337:
        data = 17
        take = 1
        expected_ready = 1
    if phase == 338:
        data = 18
        take = 1
        expected_ready = 1
    if phase == 339:
        data = 19
        take = 1
        expected_ready = 1
    if phase == 340:
        data = 20
        take = 1
        expected_ready = 1
    if phase == 341:
        data = 21
        take = 1
        expected_ready = 1
    if phase == 342:
        data = 22
        take = 1
        expected_ready = 1
    if phase == 343:
        data = 23
        take = 1
        expected_ready = 1
    return Stimulus(
        valid=valid,
        data=data,
        take=take,
        expected_ready=expected_ready,
        expected_valid=expected_valid,
        expected_data=expected_data,
    )


@rule
def advance(phase):
    if phase < 343:
        phase = phase + 1


@system
def InferredBoundaryPipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = InferredBoundaryPipeline(frame.valid, frame.data, frame.take)

    @rule
    def exercise():
        assert (
            dut.ready == frame.expected_ready
        ), "inferred_boundary_pipeline ready old-state check"
        assert (
            dut.valid == frame.expected_valid
        ), "inferred_boundary_pipeline valid old-state check"
        assert (
            dut.data == frame.expected_data
        ), "inferred_boundary_pipeline data old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "data", dut.data)

    advance(phase)
    exercise()

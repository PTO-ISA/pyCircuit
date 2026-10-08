"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_fifo_loopback.fifo_loopback import FifoLoopback
from pycircuit import bits, log, rule, struct, system


@struct
class Stimulus:
    in_valid: bits[1]
    in_data: bits[8]
    out_ready: bits[1]
    expected_in_ready: bits[1]
    expected_out_valid: bits[1]
    expected_out_data: bits[8]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    in_valid: bits[1] = 0
    in_data: bits[8] = 0
    out_ready: bits[1] = 0
    expected_in_ready: bits[1] = 0
    expected_out_valid: bits[1] = 0
    expected_out_data: bits[8] = 0
    if phase == 0:
        in_valid = 1
        expected_in_ready = 1
    if phase == 1:
        in_valid = 1
        in_data = 255
        expected_in_ready = 1
        expected_out_valid = 1
    if phase == 2:
        in_valid = 1
        in_data = 254
        expected_out_valid = 1
    if phase == 3:
        in_valid = 1
        in_data = 128
        expected_out_valid = 1
    if phase == 4:
        in_valid = 1
        in_data = 127
        expected_out_valid = 1
    if phase == 5:
        in_valid = 1
        in_data = 170
        expected_out_valid = 1
    if phase == 6:
        in_valid = 1
        in_data = 85
        expected_out_valid = 1
    if phase == 7:
        in_valid = 1
        in_data = 1
        expected_out_valid = 1
    if phase == 8:
        in_valid = 1
        in_data = 2
        expected_out_valid = 1
    if phase == 9:
        in_valid = 1
        in_data = 4
        expected_out_valid = 1
    if phase == 10:
        in_valid = 1
        in_data = 8
        expected_out_valid = 1
    if phase == 11:
        in_valid = 1
        in_data = 16
        expected_out_valid = 1
    if phase == 12:
        in_valid = 1
        in_data = 32
        expected_out_valid = 1
    if phase == 13:
        in_valid = 1
        in_data = 64
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
    if phase == 14:
        in_valid = 1
        in_data = 128
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 255
    if phase == 15:
        in_valid = 1
        in_data = 1
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 64
    if phase == 16:
        in_valid = 1
        in_data = 2
        expected_out_valid = 1
        expected_out_data = 128
    if phase == 17:
        in_valid = 1
        in_data = 4
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 128
    if phase == 18:
        in_valid = 1
        in_data = 8
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 19:
        in_valid = 1
        in_data = 16
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 4
    if phase == 20:
        in_valid = 1
        in_data = 32
        expected_out_valid = 1
        expected_out_data = 8
    if phase == 21:
        in_valid = 1
        in_data = 64
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 8
    if phase == 22:
        in_valid = 1
        in_data = 128
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 16
    if phase == 23:
        in_valid = 1
        in_data = 1
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 64
    if phase == 24:
        in_valid = 1
        in_data = 2
        expected_out_valid = 1
        expected_out_data = 128
    if phase == 25:
        in_valid = 1
        in_data = 4
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 128
    if phase == 26:
        in_valid = 1
        in_data = 8
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 27:
        in_valid = 1
        in_data = 16
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 4
    if phase == 28:
        in_valid = 1
        in_data = 32
        expected_out_valid = 1
        expected_out_data = 8
    if phase == 29:
        in_valid = 1
        in_data = 64
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 8
    if phase == 30:
        in_valid = 1
        in_data = 128
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 16
    if phase == 31:
        in_data = 1
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 64
    if phase == 32:
        in_valid = 1
        in_data = 2
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 128
    if phase == 33:
        in_valid = 1
        in_data = 4
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 128
    if phase == 34:
        in_valid = 1
        in_data = 8
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 2
    if phase == 35:
        in_valid = 1
        in_data = 16
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 4
    if phase == 36:
        in_data = 32
        expected_out_valid = 1
        expected_out_data = 8
    if phase == 37:
        in_valid = 1
        in_data = 64
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 8
    if phase == 38:
        in_valid = 1
        in_data = 128
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 16
    if phase == 39:
        in_valid = 1
        in_data = 1
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 64
    if phase == 40:
        in_valid = 1
        in_data = 2
        expected_out_valid = 1
        expected_out_data = 128
    if phase == 41:
        in_data = 4
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 128
    if phase == 42:
        in_valid = 1
        in_data = 8
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 43:
        in_valid = 1
        in_data = 16
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 8
    if phase == 44:
        in_valid = 1
        in_data = 32
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 16
    if phase == 45:
        in_valid = 1
        in_data = 64
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 16
    if phase == 46:
        in_data = 128
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 32
    if phase == 47:
        in_valid = 1
        in_data = 1
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 64
    if phase == 48:
        in_valid = 1
        in_data = 2
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 49:
        in_valid = 1
        in_data = 4
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 50:
        in_valid = 1
        in_data = 8
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 51:
        in_data = 16
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 52:
        in_valid = 1
        in_data = 32
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 53:
        in_valid = 1
        in_data = 64
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 54:
        in_valid = 1
        in_data = 128
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 55:
        in_valid = 1
        in_data = 1
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 56:
        in_data = 2
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 57:
        in_valid = 1
        in_data = 4
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 58:
        in_valid = 1
        in_data = 8
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 59:
        in_valid = 1
        in_data = 16
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 60:
        in_valid = 1
        in_data = 32
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 61:
        in_data = 64
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 62:
        in_valid = 1
        in_data = 128
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 63:
        in_valid = 1
        in_data = 1
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 64:
        in_valid = 1
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 65:
        in_valid = 1
        in_data = 1
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 2
    if phase == 66:
        in_valid = 1
        in_data = 2
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
    if phase == 67:
        in_valid = 1
        in_data = 3
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 1
    if phase == 68:
        in_valid = 1
        in_data = 4
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 2
    if phase == 69:
        in_valid = 1
        in_data = 5
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 3
    if phase == 70:
        in_valid = 1
        in_data = 6
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 4
    if phase == 71:
        in_valid = 1
        in_data = 7
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 5
    if phase == 72:
        in_valid = 1
        in_data = 8
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 6
    if phase == 73:
        in_valid = 1
        in_data = 9
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 7
    if phase == 74:
        in_valid = 1
        in_data = 10
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 8
    if phase == 75:
        in_valid = 1
        in_data = 11
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 9
    if phase == 76:
        in_valid = 1
        in_data = 12
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 10
    if phase == 77:
        in_valid = 1
        in_data = 13
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 11
    if phase == 78:
        in_valid = 1
        in_data = 14
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 12
    if phase == 79:
        in_valid = 1
        in_data = 15
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 13
    if phase == 80:
        in_valid = 1
        in_data = 16
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 14
    if phase == 81:
        in_valid = 1
        in_data = 17
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 15
    if phase == 82:
        in_valid = 1
        in_data = 18
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 16
    if phase == 83:
        in_valid = 1
        in_data = 19
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 17
    if phase == 84:
        in_valid = 1
        in_data = 20
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 18
    if phase == 85:
        in_valid = 1
        in_data = 21
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 19
    if phase == 86:
        in_valid = 1
        in_data = 22
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 20
    if phase == 87:
        in_valid = 1
        in_data = 23
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 21
    if phase == 88:
        in_valid = 1
        in_data = 24
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 22
    if phase == 89:
        in_valid = 1
        in_data = 25
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 23
    if phase == 90:
        in_valid = 1
        in_data = 26
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 24
    if phase == 91:
        in_valid = 1
        in_data = 27
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 25
    if phase == 92:
        in_valid = 1
        in_data = 28
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 26
    if phase == 93:
        in_valid = 1
        in_data = 29
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 27
    if phase == 94:
        in_valid = 1
        in_data = 30
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 28
    if phase == 95:
        in_valid = 1
        in_data = 31
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 29
    if phase == 96:
        in_valid = 1
        in_data = 32
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 30
    if phase == 97:
        in_valid = 1
        in_data = 33
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 31
    if phase == 98:
        in_valid = 1
        in_data = 34
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 32
    if phase == 99:
        in_valid = 1
        in_data = 35
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 33
    if phase == 100:
        in_valid = 1
        in_data = 36
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 34
    if phase == 101:
        in_valid = 1
        in_data = 37
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 35
    if phase == 102:
        in_valid = 1
        in_data = 38
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 36
    if phase == 103:
        in_valid = 1
        in_data = 39
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 37
    if phase == 104:
        in_valid = 1
        in_data = 40
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 38
    if phase == 105:
        in_valid = 1
        in_data = 41
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 39
    if phase == 106:
        in_valid = 1
        in_data = 42
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 40
    if phase == 107:
        in_valid = 1
        in_data = 43
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 41
    if phase == 108:
        in_valid = 1
        in_data = 44
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 42
    if phase == 109:
        in_valid = 1
        in_data = 45
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 43
    if phase == 110:
        in_valid = 1
        in_data = 46
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 44
    if phase == 111:
        in_valid = 1
        in_data = 47
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 45
    if phase == 112:
        in_valid = 1
        in_data = 48
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 46
    if phase == 113:
        in_valid = 1
        in_data = 49
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 47
    if phase == 114:
        in_valid = 1
        in_data = 50
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 48
    if phase == 115:
        in_valid = 1
        in_data = 51
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 49
    if phase == 116:
        in_valid = 1
        in_data = 52
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 50
    if phase == 117:
        in_valid = 1
        in_data = 53
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 51
    if phase == 118:
        in_valid = 1
        in_data = 54
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 52
    if phase == 119:
        in_valid = 1
        in_data = 55
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 53
    if phase == 120:
        in_valid = 1
        in_data = 56
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 54
    if phase == 121:
        in_valid = 1
        in_data = 57
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 55
    if phase == 122:
        in_valid = 1
        in_data = 58
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 56
    if phase == 123:
        in_valid = 1
        in_data = 59
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 57
    if phase == 124:
        in_valid = 1
        in_data = 60
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 58
    if phase == 125:
        in_valid = 1
        in_data = 61
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 59
    if phase == 126:
        in_valid = 1
        in_data = 62
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 60
    if phase == 127:
        in_valid = 1
        in_data = 63
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 61
    if phase == 128:
        in_valid = 1
        in_data = 64
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 62
    if phase == 129:
        in_valid = 1
        in_data = 65
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 63
    if phase == 130:
        in_valid = 1
        in_data = 66
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 64
    if phase == 131:
        in_valid = 1
        in_data = 67
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 65
    if phase == 132:
        in_valid = 1
        in_data = 68
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 66
    if phase == 133:
        in_valid = 1
        in_data = 69
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 67
    if phase == 134:
        in_valid = 1
        in_data = 70
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 68
    if phase == 135:
        in_valid = 1
        in_data = 71
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 69
    if phase == 136:
        in_valid = 1
        in_data = 72
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 70
    if phase == 137:
        in_valid = 1
        in_data = 73
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 71
    if phase == 138:
        in_valid = 1
        in_data = 74
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 72
    if phase == 139:
        in_valid = 1
        in_data = 75
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 73
    if phase == 140:
        in_valid = 1
        in_data = 76
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 74
    if phase == 141:
        in_valid = 1
        in_data = 77
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 75
    if phase == 142:
        in_valid = 1
        in_data = 78
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 76
    if phase == 143:
        in_valid = 1
        in_data = 79
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 77
    if phase == 144:
        in_valid = 1
        in_data = 80
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 78
    if phase == 145:
        in_valid = 1
        in_data = 81
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 79
    if phase == 146:
        in_valid = 1
        in_data = 82
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 80
    if phase == 147:
        in_valid = 1
        in_data = 83
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 81
    if phase == 148:
        in_valid = 1
        in_data = 84
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 82
    if phase == 149:
        in_valid = 1
        in_data = 85
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 83
    if phase == 150:
        in_valid = 1
        in_data = 86
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 84
    if phase == 151:
        in_valid = 1
        in_data = 87
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 85
    if phase == 152:
        in_valid = 1
        in_data = 88
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 86
    if phase == 153:
        in_valid = 1
        in_data = 89
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 87
    if phase == 154:
        in_valid = 1
        in_data = 90
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 88
    if phase == 155:
        in_valid = 1
        in_data = 91
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 89
    if phase == 156:
        in_valid = 1
        in_data = 92
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 90
    if phase == 157:
        in_valid = 1
        in_data = 93
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 91
    if phase == 158:
        in_valid = 1
        in_data = 94
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 92
    if phase == 159:
        in_valid = 1
        in_data = 95
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 93
    if phase == 160:
        in_valid = 1
        in_data = 96
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 94
    if phase == 161:
        in_valid = 1
        in_data = 97
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 95
    if phase == 162:
        in_valid = 1
        in_data = 98
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 96
    if phase == 163:
        in_valid = 1
        in_data = 99
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 97
    if phase == 164:
        in_valid = 1
        in_data = 100
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 98
    if phase == 165:
        in_valid = 1
        in_data = 101
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 99
    if phase == 166:
        in_valid = 1
        in_data = 102
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 100
    if phase == 167:
        in_valid = 1
        in_data = 103
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 101
    if phase == 168:
        in_valid = 1
        in_data = 104
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 102
    if phase == 169:
        in_valid = 1
        in_data = 105
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 103
    if phase == 170:
        in_valid = 1
        in_data = 106
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 104
    if phase == 171:
        in_valid = 1
        in_data = 107
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 105
    if phase == 172:
        in_valid = 1
        in_data = 108
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 106
    if phase == 173:
        in_valid = 1
        in_data = 109
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 107
    if phase == 174:
        in_valid = 1
        in_data = 110
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 108
    if phase == 175:
        in_valid = 1
        in_data = 111
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 109
    if phase == 176:
        in_valid = 1
        in_data = 112
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 110
    if phase == 177:
        in_valid = 1
        in_data = 113
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 111
    if phase == 178:
        in_valid = 1
        in_data = 114
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 112
    if phase == 179:
        in_valid = 1
        in_data = 115
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 113
    if phase == 180:
        in_valid = 1
        in_data = 116
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 114
    if phase == 181:
        in_valid = 1
        in_data = 117
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 115
    if phase == 182:
        in_valid = 1
        in_data = 118
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 116
    if phase == 183:
        in_valid = 1
        in_data = 119
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 117
    if phase == 184:
        in_valid = 1
        in_data = 120
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 118
    if phase == 185:
        in_valid = 1
        in_data = 121
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 119
    if phase == 186:
        in_valid = 1
        in_data = 122
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 120
    if phase == 187:
        in_valid = 1
        in_data = 123
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 121
    if phase == 188:
        in_valid = 1
        in_data = 124
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 122
    if phase == 189:
        in_valid = 1
        in_data = 125
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 123
    if phase == 190:
        in_valid = 1
        in_data = 126
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 124
    if phase == 191:
        in_valid = 1
        in_data = 127
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 125
    if phase == 192:
        in_valid = 1
        in_data = 128
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 126
    if phase == 193:
        in_valid = 1
        in_data = 129
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 127
    if phase == 194:
        in_valid = 1
        in_data = 130
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 128
    if phase == 195:
        in_valid = 1
        in_data = 131
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 129
    if phase == 196:
        in_valid = 1
        in_data = 132
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 130
    if phase == 197:
        in_valid = 1
        in_data = 133
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 131
    if phase == 198:
        in_valid = 1
        in_data = 134
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 132
    if phase == 199:
        in_valid = 1
        in_data = 135
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 133
    if phase == 200:
        in_valid = 1
        in_data = 136
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 134
    if phase == 201:
        in_valid = 1
        in_data = 137
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 135
    if phase == 202:
        in_valid = 1
        in_data = 138
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 136
    if phase == 203:
        in_valid = 1
        in_data = 139
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 137
    if phase == 204:
        in_valid = 1
        in_data = 140
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 138
    if phase == 205:
        in_valid = 1
        in_data = 141
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 139
    if phase == 206:
        in_valid = 1
        in_data = 142
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 140
    if phase == 207:
        in_valid = 1
        in_data = 143
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 141
    if phase == 208:
        in_valid = 1
        in_data = 144
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 142
    if phase == 209:
        in_valid = 1
        in_data = 145
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 143
    if phase == 210:
        in_valid = 1
        in_data = 146
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 144
    if phase == 211:
        in_valid = 1
        in_data = 147
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 145
    if phase == 212:
        in_valid = 1
        in_data = 148
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 146
    if phase == 213:
        in_valid = 1
        in_data = 149
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 147
    if phase == 214:
        in_valid = 1
        in_data = 150
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 148
    if phase == 215:
        in_valid = 1
        in_data = 151
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 149
    if phase == 216:
        in_valid = 1
        in_data = 152
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 150
    if phase == 217:
        in_valid = 1
        in_data = 153
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 151
    if phase == 218:
        in_valid = 1
        in_data = 154
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 152
    if phase == 219:
        in_valid = 1
        in_data = 155
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 153
    if phase == 220:
        in_valid = 1
        in_data = 156
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 154
    if phase == 221:
        in_valid = 1
        in_data = 157
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 155
    if phase == 222:
        in_valid = 1
        in_data = 158
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 156
    if phase == 223:
        in_valid = 1
        in_data = 159
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 157
    if phase == 224:
        in_valid = 1
        in_data = 160
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 158
    if phase == 225:
        in_valid = 1
        in_data = 161
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 159
    if phase == 226:
        in_valid = 1
        in_data = 162
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 160
    if phase == 227:
        in_valid = 1
        in_data = 163
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 161
    if phase == 228:
        in_valid = 1
        in_data = 164
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 162
    if phase == 229:
        in_valid = 1
        in_data = 165
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 163
    if phase == 230:
        in_valid = 1
        in_data = 166
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 164
    if phase == 231:
        in_valid = 1
        in_data = 167
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 165
    if phase == 232:
        in_valid = 1
        in_data = 168
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 166
    if phase == 233:
        in_valid = 1
        in_data = 169
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 167
    if phase == 234:
        in_valid = 1
        in_data = 170
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 168
    if phase == 235:
        in_valid = 1
        in_data = 171
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 169
    if phase == 236:
        in_valid = 1
        in_data = 172
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 170
    if phase == 237:
        in_valid = 1
        in_data = 173
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 171
    if phase == 238:
        in_valid = 1
        in_data = 174
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 172
    if phase == 239:
        in_valid = 1
        in_data = 175
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 173
    if phase == 240:
        in_valid = 1
        in_data = 176
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 174
    if phase == 241:
        in_valid = 1
        in_data = 177
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 175
    if phase == 242:
        in_valid = 1
        in_data = 178
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 176
    if phase == 243:
        in_valid = 1
        in_data = 179
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 177
    if phase == 244:
        in_valid = 1
        in_data = 180
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 178
    if phase == 245:
        in_valid = 1
        in_data = 181
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 179
    if phase == 246:
        in_valid = 1
        in_data = 182
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 180
    if phase == 247:
        in_valid = 1
        in_data = 183
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 181
    if phase == 248:
        in_valid = 1
        in_data = 184
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 182
    if phase == 249:
        in_valid = 1
        in_data = 185
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 183
    if phase == 250:
        in_valid = 1
        in_data = 186
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 184
    if phase == 251:
        in_valid = 1
        in_data = 187
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 185
    if phase == 252:
        in_valid = 1
        in_data = 188
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 186
    if phase == 253:
        in_valid = 1
        in_data = 189
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 187
    if phase == 254:
        in_valid = 1
        in_data = 190
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 188
    if phase == 255:
        in_valid = 1
        in_data = 191
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 189
    if phase == 256:
        in_valid = 1
        in_data = 192
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 190
    if phase == 257:
        in_valid = 1
        in_data = 193
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 191
    if phase == 258:
        in_valid = 1
        in_data = 194
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 192
    if phase == 259:
        in_valid = 1
        in_data = 195
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 193
    if phase == 260:
        in_valid = 1
        in_data = 196
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 194
    if phase == 261:
        in_valid = 1
        in_data = 197
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 195
    if phase == 262:
        in_valid = 1
        in_data = 198
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 196
    if phase == 263:
        in_valid = 1
        in_data = 199
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 197
    if phase == 264:
        in_valid = 1
        in_data = 200
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 198
    if phase == 265:
        in_valid = 1
        in_data = 201
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 199
    if phase == 266:
        in_valid = 1
        in_data = 202
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 200
    if phase == 267:
        in_valid = 1
        in_data = 203
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 201
    if phase == 268:
        in_valid = 1
        in_data = 204
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 202
    if phase == 269:
        in_valid = 1
        in_data = 205
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 203
    if phase == 270:
        in_valid = 1
        in_data = 206
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 204
    if phase == 271:
        in_valid = 1
        in_data = 207
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 205
    if phase == 272:
        in_valid = 1
        in_data = 208
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 206
    if phase == 273:
        in_valid = 1
        in_data = 209
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 207
    if phase == 274:
        in_valid = 1
        in_data = 210
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 208
    if phase == 275:
        in_valid = 1
        in_data = 211
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 209
    if phase == 276:
        in_valid = 1
        in_data = 212
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 210
    if phase == 277:
        in_valid = 1
        in_data = 213
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 211
    if phase == 278:
        in_valid = 1
        in_data = 214
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 212
    if phase == 279:
        in_valid = 1
        in_data = 215
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 213
    if phase == 280:
        in_valid = 1
        in_data = 216
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 214
    if phase == 281:
        in_valid = 1
        in_data = 217
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 215
    if phase == 282:
        in_valid = 1
        in_data = 218
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 216
    if phase == 283:
        in_valid = 1
        in_data = 219
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 217
    if phase == 284:
        in_valid = 1
        in_data = 220
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 218
    if phase == 285:
        in_valid = 1
        in_data = 221
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 219
    if phase == 286:
        in_valid = 1
        in_data = 222
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 220
    if phase == 287:
        in_valid = 1
        in_data = 223
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 221
    if phase == 288:
        in_valid = 1
        in_data = 224
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 222
    if phase == 289:
        in_valid = 1
        in_data = 225
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 223
    if phase == 290:
        in_valid = 1
        in_data = 226
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 224
    if phase == 291:
        in_valid = 1
        in_data = 227
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 225
    if phase == 292:
        in_valid = 1
        in_data = 228
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 226
    if phase == 293:
        in_valid = 1
        in_data = 229
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 227
    if phase == 294:
        in_valid = 1
        in_data = 230
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 228
    if phase == 295:
        in_valid = 1
        in_data = 231
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 229
    if phase == 296:
        in_valid = 1
        in_data = 232
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 230
    if phase == 297:
        in_valid = 1
        in_data = 233
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 231
    if phase == 298:
        in_valid = 1
        in_data = 234
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 232
    if phase == 299:
        in_valid = 1
        in_data = 235
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 233
    if phase == 300:
        in_valid = 1
        in_data = 236
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 234
    if phase == 301:
        in_valid = 1
        in_data = 237
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 235
    if phase == 302:
        in_valid = 1
        in_data = 238
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 236
    if phase == 303:
        in_valid = 1
        in_data = 239
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 237
    if phase == 304:
        in_valid = 1
        in_data = 240
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 238
    if phase == 305:
        in_valid = 1
        in_data = 241
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 239
    if phase == 306:
        in_valid = 1
        in_data = 242
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 240
    if phase == 307:
        in_valid = 1
        in_data = 243
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 241
    if phase == 308:
        in_valid = 1
        in_data = 244
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 242
    if phase == 309:
        in_valid = 1
        in_data = 245
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 243
    if phase == 310:
        in_valid = 1
        in_data = 246
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 244
    if phase == 311:
        in_valid = 1
        in_data = 247
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 245
    if phase == 312:
        in_valid = 1
        in_data = 248
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 246
    if phase == 313:
        in_valid = 1
        in_data = 249
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 247
    if phase == 314:
        in_valid = 1
        in_data = 250
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 248
    if phase == 315:
        in_valid = 1
        in_data = 251
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 249
    if phase == 316:
        in_valid = 1
        in_data = 252
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 250
    if phase == 317:
        in_valid = 1
        in_data = 253
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 251
    if phase == 318:
        in_valid = 1
        in_data = 254
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 252
    if phase == 319:
        in_valid = 1
        in_data = 255
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 253
    if phase == 320:
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 254
    if phase == 321:
        in_data = 1
        out_ready = 1
        expected_in_ready = 1
        expected_out_valid = 1
        expected_out_data = 255
    if phase == 322:
        in_data = 2
        out_ready = 1
        expected_in_ready = 1
    if phase == 323:
        in_data = 3
        out_ready = 1
        expected_in_ready = 1
    if phase == 324:
        in_data = 4
        out_ready = 1
        expected_in_ready = 1
    if phase == 325:
        in_data = 5
        out_ready = 1
        expected_in_ready = 1
    if phase == 326:
        in_data = 6
        out_ready = 1
        expected_in_ready = 1
    if phase == 327:
        in_data = 7
        out_ready = 1
        expected_in_ready = 1
    if phase == 328:
        in_data = 8
        out_ready = 1
        expected_in_ready = 1
    if phase == 329:
        in_data = 9
        out_ready = 1
        expected_in_ready = 1
    if phase == 330:
        in_data = 10
        out_ready = 1
        expected_in_ready = 1
    if phase == 331:
        in_data = 11
        out_ready = 1
        expected_in_ready = 1
    if phase == 332:
        in_data = 12
        out_ready = 1
        expected_in_ready = 1
    if phase == 333:
        in_data = 13
        out_ready = 1
        expected_in_ready = 1
    if phase == 334:
        in_data = 14
        out_ready = 1
        expected_in_ready = 1
    if phase == 335:
        in_data = 15
        out_ready = 1
        expected_in_ready = 1
    if phase == 336:
        in_data = 16
        out_ready = 1
        expected_in_ready = 1
    if phase == 337:
        in_data = 17
        out_ready = 1
        expected_in_ready = 1
    if phase == 338:
        in_data = 18
        out_ready = 1
        expected_in_ready = 1
    if phase == 339:
        in_data = 19
        out_ready = 1
        expected_in_ready = 1
    if phase == 340:
        in_data = 20
        out_ready = 1
        expected_in_ready = 1
    if phase == 341:
        in_data = 21
        out_ready = 1
        expected_in_ready = 1
    if phase == 342:
        in_data = 22
        out_ready = 1
        expected_in_ready = 1
    if phase == 343:
        in_data = 23
        out_ready = 1
        expected_in_ready = 1
    return Stimulus(
        in_valid=in_valid,
        in_data=in_data,
        out_ready=out_ready,
        expected_in_ready=expected_in_ready,
        expected_out_valid=expected_out_valid,
        expected_out_data=expected_out_data,
    )


@rule
def advance(phase):
    if phase < 343:
        phase = phase + 1


@system
def FifoLoopbackSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = FifoLoopback(frame.in_valid, frame.in_data, frame.out_ready)

    @rule
    def exercise():
        assert (
            dut.in_ready == frame.expected_in_ready
        ), "fifo_loopback in_ready old-state check"
        assert (
            dut.out_valid == frame.expected_out_valid
        ), "fifo_loopback out_valid old-state check"
        assert (
            dut.out_data == frame.expected_out_data
        ), "fifo_loopback out_data old-state check"
        log("info", "phase", phase)
        log("info", "in_ready", dut.in_ready)
        log("info", "out_valid", dut.out_valid)
        log("info", "out_data", dut.out_data)

    advance(phase)
    exercise()

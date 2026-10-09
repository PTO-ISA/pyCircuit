"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_issue_queue_2picker.issue_queue_2picker import IssueQueue2Picker
from pycircuit import bits, log, rule, struct, system


@struct
class Stimulus:
    in_valid: bits[1]
    in_data: bits[8]
    out0_ready: bits[1]
    out1_ready: bits[1]
    expected_in_ready: bits[1]
    expected_out0_valid: bits[1]
    expected_out0_data: bits[8]
    expected_out1_valid: bits[1]
    expected_out1_data: bits[8]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    in_valid: bits[1] = 0
    in_data: bits[8] = 0
    out0_ready: bits[1] = 0
    out1_ready: bits[1] = 0
    expected_in_ready: bits[1] = 0
    expected_out0_valid: bits[1] = 0
    expected_out0_data: bits[8] = 0
    expected_out1_valid: bits[1] = 0
    expected_out1_data: bits[8] = 0
    if phase == 0:
        in_valid = 1
        expected_in_ready = 1
    if phase == 1:
        in_valid = 1
        in_data = 255
        expected_in_ready = 1
        expected_out0_valid = 1
    if phase == 2:
        in_valid = 1
        in_data = 254
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 3:
        in_valid = 1
        in_data = 128
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 4:
        in_valid = 1
        in_data = 127
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 5:
        in_valid = 1
        in_data = 170
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 6:
        in_valid = 1
        in_data = 85
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 7:
        in_valid = 1
        in_data = 1
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 8:
        in_valid = 1
        in_data = 2
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 9:
        in_valid = 1
        in_data = 4
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 10:
        in_valid = 1
        in_data = 8
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 11:
        in_valid = 1
        in_data = 16
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 12:
        in_valid = 1
        in_data = 32
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 13:
        in_valid = 1
        in_data = 64
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 255
    if phase == 14:
        in_valid = 1
        in_data = 128
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 255
        expected_out1_valid = 1
        expected_out1_data = 254
    if phase == 15:
        in_valid = 1
        in_data = 1
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 254
        expected_out1_valid = 1
        expected_out1_data = 128
    if phase == 16:
        in_valid = 1
        in_data = 2
        expected_out0_valid = 1
        expected_out0_data = 128
        expected_out1_valid = 1
        expected_out1_data = 64
    if phase == 17:
        in_valid = 1
        in_data = 4
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 128
        expected_out1_valid = 1
        expected_out1_data = 64
    if phase == 18:
        in_valid = 1
        in_data = 8
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 64
        expected_out1_valid = 1
        expected_out1_data = 128
    if phase == 19:
        in_valid = 1
        in_data = 16
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 128
        expected_out1_valid = 1
        expected_out1_data = 1
    if phase == 20:
        in_valid = 1
        in_data = 32
        out1_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 4
    if phase == 21:
        in_valid = 1
        in_data = 64
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 4
    if phase == 22:
        in_valid = 1
        in_data = 128
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 8
        expected_out1_valid = 1
        expected_out1_data = 16
    if phase == 23:
        in_valid = 1
        in_data = 1
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 16
        expected_out1_valid = 1
        expected_out1_data = 64
    if phase == 24:
        in_valid = 1
        in_data = 2
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 128
        expected_out1_valid = 1
        expected_out1_data = 1
    if phase == 25:
        in_valid = 1
        in_data = 4
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 128
        expected_out1_valid = 1
        expected_out1_data = 1
    if phase == 26:
        in_valid = 1
        in_data = 8
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 27:
        in_valid = 1
        in_data = 16
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 4
        expected_out1_valid = 1
        expected_out1_data = 8
    if phase == 28:
        in_valid = 1
        in_data = 32
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 16
        expected_out1_data = 16
    if phase == 29:
        in_valid = 1
        in_data = 64
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 16
        expected_out1_valid = 1
        expected_out1_data = 32
    if phase == 30:
        in_valid = 1
        in_data = 128
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 64
        expected_out1_data = 16
    if phase == 31:
        in_data = 1
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 128
        expected_out1_data = 16
    if phase == 32:
        in_valid = 1
        in_data = 2
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 16
        expected_out1_data = 16
    if phase == 33:
        in_valid = 1
        in_data = 4
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 2
        expected_out1_data = 16
    if phase == 34:
        in_valid = 1
        in_data = 8
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 4
        expected_out1_data = 16
    if phase == 35:
        in_valid = 1
        in_data = 16
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 8
        expected_out1_data = 16
    if phase == 36:
        in_data = 32
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 16
        expected_out1_data = 16
    if phase == 37:
        in_valid = 1
        in_data = 64
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 16
        expected_out1_data = 16
    if phase == 38:
        in_valid = 1
        in_data = 128
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 64
        expected_out1_data = 16
    if phase == 39:
        in_valid = 1
        in_data = 1
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 128
        expected_out1_data = 16
    if phase == 40:
        in_valid = 1
        in_data = 2
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_data = 16
    if phase == 41:
        in_data = 4
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 42:
        in_valid = 1
        in_data = 8
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 16
        expected_out1_data = 16
    if phase == 43:
        in_valid = 1
        in_data = 16
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 8
        expected_out1_data = 16
    if phase == 44:
        in_valid = 1
        in_data = 32
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 16
        expected_out1_data = 16
    if phase == 45:
        in_valid = 1
        in_data = 64
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 16
        expected_out1_valid = 1
        expected_out1_data = 32
    if phase == 46:
        in_data = 128
        out0_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 64
        expected_out1_data = 16
    if phase == 47:
        in_valid = 1
        in_data = 1
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 16
        expected_out1_data = 16
    if phase == 48:
        in_valid = 1
        in_data = 2
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_data = 16
    if phase == 49:
        in_valid = 1
        in_data = 4
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 50:
        in_valid = 1
        in_data = 8
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 51:
        in_data = 16
        out1_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 52:
        in_valid = 1
        in_data = 32
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 53:
        in_valid = 1
        in_data = 64
        out1_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 54:
        in_valid = 1
        in_data = 128
        out1_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 55:
        in_valid = 1
        in_data = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 56:
        in_data = 2
        out1_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 57:
        in_valid = 1
        in_data = 4
        out1_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 58:
        in_valid = 1
        in_data = 8
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 59:
        in_valid = 1
        in_data = 16
        out1_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 60:
        in_valid = 1
        in_data = 32
        out1_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 61:
        in_data = 64
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 62:
        in_valid = 1
        in_data = 128
        out1_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 63:
        in_valid = 1
        in_data = 1
        out1_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 64:
        in_valid = 1
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 1
        expected_out1_valid = 1
        expected_out1_data = 2
    if phase == 65:
        in_valid = 1
        in_data = 1
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 4
        expected_out1_valid = 1
        expected_out1_data = 8
    if phase == 66:
        in_valid = 1
        in_data = 2
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out1_valid = 1
        expected_out1_data = 1
    if phase == 67:
        in_valid = 1
        in_data = 3
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 2
        expected_out1_data = 8
    if phase == 68:
        in_valid = 1
        in_data = 4
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 3
        expected_out1_data = 8
    if phase == 69:
        in_valid = 1
        in_data = 5
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 4
        expected_out1_data = 8
    if phase == 70:
        in_valid = 1
        in_data = 6
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 5
        expected_out1_data = 8
    if phase == 71:
        in_valid = 1
        in_data = 7
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 6
        expected_out1_data = 8
    if phase == 72:
        in_valid = 1
        in_data = 8
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 7
        expected_out1_data = 8
    if phase == 73:
        in_valid = 1
        in_data = 9
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 74:
        in_valid = 1
        in_data = 10
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 9
        expected_out1_data = 8
    if phase == 75:
        in_valid = 1
        in_data = 11
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 10
        expected_out1_data = 8
    if phase == 76:
        in_valid = 1
        in_data = 12
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 11
        expected_out1_data = 8
    if phase == 77:
        in_valid = 1
        in_data = 13
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 12
        expected_out1_data = 8
    if phase == 78:
        in_valid = 1
        in_data = 14
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 13
        expected_out1_data = 8
    if phase == 79:
        in_valid = 1
        in_data = 15
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 14
        expected_out1_data = 8
    if phase == 80:
        in_valid = 1
        in_data = 16
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 15
        expected_out1_data = 8
    if phase == 81:
        in_valid = 1
        in_data = 17
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 16
        expected_out1_data = 8
    if phase == 82:
        in_valid = 1
        in_data = 18
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 17
        expected_out1_data = 8
    if phase == 83:
        in_valid = 1
        in_data = 19
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 18
        expected_out1_data = 8
    if phase == 84:
        in_valid = 1
        in_data = 20
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 19
        expected_out1_data = 8
    if phase == 85:
        in_valid = 1
        in_data = 21
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 20
        expected_out1_data = 8
    if phase == 86:
        in_valid = 1
        in_data = 22
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 21
        expected_out1_data = 8
    if phase == 87:
        in_valid = 1
        in_data = 23
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 22
        expected_out1_data = 8
    if phase == 88:
        in_valid = 1
        in_data = 24
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 23
        expected_out1_data = 8
    if phase == 89:
        in_valid = 1
        in_data = 25
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 24
        expected_out1_data = 8
    if phase == 90:
        in_valid = 1
        in_data = 26
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 25
        expected_out1_data = 8
    if phase == 91:
        in_valid = 1
        in_data = 27
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 26
        expected_out1_data = 8
    if phase == 92:
        in_valid = 1
        in_data = 28
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 27
        expected_out1_data = 8
    if phase == 93:
        in_valid = 1
        in_data = 29
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 28
        expected_out1_data = 8
    if phase == 94:
        in_valid = 1
        in_data = 30
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 29
        expected_out1_data = 8
    if phase == 95:
        in_valid = 1
        in_data = 31
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 30
        expected_out1_data = 8
    if phase == 96:
        in_valid = 1
        in_data = 32
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 31
        expected_out1_data = 8
    if phase == 97:
        in_valid = 1
        in_data = 33
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 32
        expected_out1_data = 8
    if phase == 98:
        in_valid = 1
        in_data = 34
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 33
        expected_out1_data = 8
    if phase == 99:
        in_valid = 1
        in_data = 35
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 34
        expected_out1_data = 8
    if phase == 100:
        in_valid = 1
        in_data = 36
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 35
        expected_out1_data = 8
    if phase == 101:
        in_valid = 1
        in_data = 37
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 36
        expected_out1_data = 8
    if phase == 102:
        in_valid = 1
        in_data = 38
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 37
        expected_out1_data = 8
    if phase == 103:
        in_valid = 1
        in_data = 39
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 38
        expected_out1_data = 8
    if phase == 104:
        in_valid = 1
        in_data = 40
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 39
        expected_out1_data = 8
    if phase == 105:
        in_valid = 1
        in_data = 41
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 40
        expected_out1_data = 8
    if phase == 106:
        in_valid = 1
        in_data = 42
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 41
        expected_out1_data = 8
    if phase == 107:
        in_valid = 1
        in_data = 43
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 42
        expected_out1_data = 8
    if phase == 108:
        in_valid = 1
        in_data = 44
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 43
        expected_out1_data = 8
    if phase == 109:
        in_valid = 1
        in_data = 45
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 44
        expected_out1_data = 8
    if phase == 110:
        in_valid = 1
        in_data = 46
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 45
        expected_out1_data = 8
    if phase == 111:
        in_valid = 1
        in_data = 47
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 46
        expected_out1_data = 8
    if phase == 112:
        in_valid = 1
        in_data = 48
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 47
        expected_out1_data = 8
    if phase == 113:
        in_valid = 1
        in_data = 49
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 48
        expected_out1_data = 8
    if phase == 114:
        in_valid = 1
        in_data = 50
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 49
        expected_out1_data = 8
    if phase == 115:
        in_valid = 1
        in_data = 51
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 50
        expected_out1_data = 8
    if phase == 116:
        in_valid = 1
        in_data = 52
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 51
        expected_out1_data = 8
    if phase == 117:
        in_valid = 1
        in_data = 53
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 52
        expected_out1_data = 8
    if phase == 118:
        in_valid = 1
        in_data = 54
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 53
        expected_out1_data = 8
    if phase == 119:
        in_valid = 1
        in_data = 55
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 54
        expected_out1_data = 8
    if phase == 120:
        in_valid = 1
        in_data = 56
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 55
        expected_out1_data = 8
    if phase == 121:
        in_valid = 1
        in_data = 57
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 56
        expected_out1_data = 8
    if phase == 122:
        in_valid = 1
        in_data = 58
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 57
        expected_out1_data = 8
    if phase == 123:
        in_valid = 1
        in_data = 59
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 58
        expected_out1_data = 8
    if phase == 124:
        in_valid = 1
        in_data = 60
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 59
        expected_out1_data = 8
    if phase == 125:
        in_valid = 1
        in_data = 61
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 60
        expected_out1_data = 8
    if phase == 126:
        in_valid = 1
        in_data = 62
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 61
        expected_out1_data = 8
    if phase == 127:
        in_valid = 1
        in_data = 63
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 62
        expected_out1_data = 8
    if phase == 128:
        in_valid = 1
        in_data = 64
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 63
        expected_out1_data = 8
    if phase == 129:
        in_valid = 1
        in_data = 65
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 64
        expected_out1_data = 8
    if phase == 130:
        in_valid = 1
        in_data = 66
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 65
        expected_out1_data = 8
    if phase == 131:
        in_valid = 1
        in_data = 67
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 66
        expected_out1_data = 8
    if phase == 132:
        in_valid = 1
        in_data = 68
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 67
        expected_out1_data = 8
    if phase == 133:
        in_valid = 1
        in_data = 69
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 68
        expected_out1_data = 8
    if phase == 134:
        in_valid = 1
        in_data = 70
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 69
        expected_out1_data = 8
    if phase == 135:
        in_valid = 1
        in_data = 71
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 70
        expected_out1_data = 8
    if phase == 136:
        in_valid = 1
        in_data = 72
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 71
        expected_out1_data = 8
    if phase == 137:
        in_valid = 1
        in_data = 73
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 72
        expected_out1_data = 8
    if phase == 138:
        in_valid = 1
        in_data = 74
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 73
        expected_out1_data = 8
    if phase == 139:
        in_valid = 1
        in_data = 75
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 74
        expected_out1_data = 8
    if phase == 140:
        in_valid = 1
        in_data = 76
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 75
        expected_out1_data = 8
    if phase == 141:
        in_valid = 1
        in_data = 77
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 76
        expected_out1_data = 8
    if phase == 142:
        in_valid = 1
        in_data = 78
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 77
        expected_out1_data = 8
    if phase == 143:
        in_valid = 1
        in_data = 79
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 78
        expected_out1_data = 8
    if phase == 144:
        in_valid = 1
        in_data = 80
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 79
        expected_out1_data = 8
    if phase == 145:
        in_valid = 1
        in_data = 81
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 80
        expected_out1_data = 8
    if phase == 146:
        in_valid = 1
        in_data = 82
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 81
        expected_out1_data = 8
    if phase == 147:
        in_valid = 1
        in_data = 83
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 82
        expected_out1_data = 8
    if phase == 148:
        in_valid = 1
        in_data = 84
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 83
        expected_out1_data = 8
    if phase == 149:
        in_valid = 1
        in_data = 85
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 84
        expected_out1_data = 8
    if phase == 150:
        in_valid = 1
        in_data = 86
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 85
        expected_out1_data = 8
    if phase == 151:
        in_valid = 1
        in_data = 87
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 86
        expected_out1_data = 8
    if phase == 152:
        in_valid = 1
        in_data = 88
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 87
        expected_out1_data = 8
    if phase == 153:
        in_valid = 1
        in_data = 89
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 88
        expected_out1_data = 8
    if phase == 154:
        in_valid = 1
        in_data = 90
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 89
        expected_out1_data = 8
    if phase == 155:
        in_valid = 1
        in_data = 91
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 90
        expected_out1_data = 8
    if phase == 156:
        in_valid = 1
        in_data = 92
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 91
        expected_out1_data = 8
    if phase == 157:
        in_valid = 1
        in_data = 93
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 92
        expected_out1_data = 8
    if phase == 158:
        in_valid = 1
        in_data = 94
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 93
        expected_out1_data = 8
    if phase == 159:
        in_valid = 1
        in_data = 95
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 94
        expected_out1_data = 8
    if phase == 160:
        in_valid = 1
        in_data = 96
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 95
        expected_out1_data = 8
    if phase == 161:
        in_valid = 1
        in_data = 97
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 96
        expected_out1_data = 8
    if phase == 162:
        in_valid = 1
        in_data = 98
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 97
        expected_out1_data = 8
    if phase == 163:
        in_valid = 1
        in_data = 99
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 98
        expected_out1_data = 8
    if phase == 164:
        in_valid = 1
        in_data = 100
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 99
        expected_out1_data = 8
    if phase == 165:
        in_valid = 1
        in_data = 101
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 100
        expected_out1_data = 8
    if phase == 166:
        in_valid = 1
        in_data = 102
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 101
        expected_out1_data = 8
    if phase == 167:
        in_valid = 1
        in_data = 103
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 102
        expected_out1_data = 8
    if phase == 168:
        in_valid = 1
        in_data = 104
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 103
        expected_out1_data = 8
    if phase == 169:
        in_valid = 1
        in_data = 105
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 104
        expected_out1_data = 8
    if phase == 170:
        in_valid = 1
        in_data = 106
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 105
        expected_out1_data = 8
    if phase == 171:
        in_valid = 1
        in_data = 107
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 106
        expected_out1_data = 8
    if phase == 172:
        in_valid = 1
        in_data = 108
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 107
        expected_out1_data = 8
    if phase == 173:
        in_valid = 1
        in_data = 109
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 108
        expected_out1_data = 8
    if phase == 174:
        in_valid = 1
        in_data = 110
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 109
        expected_out1_data = 8
    if phase == 175:
        in_valid = 1
        in_data = 111
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 110
        expected_out1_data = 8
    if phase == 176:
        in_valid = 1
        in_data = 112
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 111
        expected_out1_data = 8
    if phase == 177:
        in_valid = 1
        in_data = 113
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 112
        expected_out1_data = 8
    if phase == 178:
        in_valid = 1
        in_data = 114
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 113
        expected_out1_data = 8
    if phase == 179:
        in_valid = 1
        in_data = 115
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 114
        expected_out1_data = 8
    if phase == 180:
        in_valid = 1
        in_data = 116
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 115
        expected_out1_data = 8
    if phase == 181:
        in_valid = 1
        in_data = 117
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 116
        expected_out1_data = 8
    if phase == 182:
        in_valid = 1
        in_data = 118
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 117
        expected_out1_data = 8
    if phase == 183:
        in_valid = 1
        in_data = 119
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 118
        expected_out1_data = 8
    if phase == 184:
        in_valid = 1
        in_data = 120
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 119
        expected_out1_data = 8
    if phase == 185:
        in_valid = 1
        in_data = 121
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 120
        expected_out1_data = 8
    if phase == 186:
        in_valid = 1
        in_data = 122
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 121
        expected_out1_data = 8
    if phase == 187:
        in_valid = 1
        in_data = 123
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 122
        expected_out1_data = 8
    if phase == 188:
        in_valid = 1
        in_data = 124
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 123
        expected_out1_data = 8
    if phase == 189:
        in_valid = 1
        in_data = 125
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 124
        expected_out1_data = 8
    if phase == 190:
        in_valid = 1
        in_data = 126
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 125
        expected_out1_data = 8
    if phase == 191:
        in_valid = 1
        in_data = 127
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 126
        expected_out1_data = 8
    if phase == 192:
        in_valid = 1
        in_data = 128
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 127
        expected_out1_data = 8
    if phase == 193:
        in_valid = 1
        in_data = 129
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 128
        expected_out1_data = 8
    if phase == 194:
        in_valid = 1
        in_data = 130
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 129
        expected_out1_data = 8
    if phase == 195:
        in_valid = 1
        in_data = 131
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 130
        expected_out1_data = 8
    if phase == 196:
        in_valid = 1
        in_data = 132
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 131
        expected_out1_data = 8
    if phase == 197:
        in_valid = 1
        in_data = 133
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 132
        expected_out1_data = 8
    if phase == 198:
        in_valid = 1
        in_data = 134
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 133
        expected_out1_data = 8
    if phase == 199:
        in_valid = 1
        in_data = 135
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 134
        expected_out1_data = 8
    if phase == 200:
        in_valid = 1
        in_data = 136
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 135
        expected_out1_data = 8
    if phase == 201:
        in_valid = 1
        in_data = 137
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 136
        expected_out1_data = 8
    if phase == 202:
        in_valid = 1
        in_data = 138
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 137
        expected_out1_data = 8
    if phase == 203:
        in_valid = 1
        in_data = 139
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 138
        expected_out1_data = 8
    if phase == 204:
        in_valid = 1
        in_data = 140
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 139
        expected_out1_data = 8
    if phase == 205:
        in_valid = 1
        in_data = 141
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 140
        expected_out1_data = 8
    if phase == 206:
        in_valid = 1
        in_data = 142
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 141
        expected_out1_data = 8
    if phase == 207:
        in_valid = 1
        in_data = 143
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 142
        expected_out1_data = 8
    if phase == 208:
        in_valid = 1
        in_data = 144
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 143
        expected_out1_data = 8
    if phase == 209:
        in_valid = 1
        in_data = 145
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 144
        expected_out1_data = 8
    if phase == 210:
        in_valid = 1
        in_data = 146
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 145
        expected_out1_data = 8
    if phase == 211:
        in_valid = 1
        in_data = 147
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 146
        expected_out1_data = 8
    if phase == 212:
        in_valid = 1
        in_data = 148
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 147
        expected_out1_data = 8
    if phase == 213:
        in_valid = 1
        in_data = 149
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 148
        expected_out1_data = 8
    if phase == 214:
        in_valid = 1
        in_data = 150
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 149
        expected_out1_data = 8
    if phase == 215:
        in_valid = 1
        in_data = 151
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 150
        expected_out1_data = 8
    if phase == 216:
        in_valid = 1
        in_data = 152
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 151
        expected_out1_data = 8
    if phase == 217:
        in_valid = 1
        in_data = 153
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 152
        expected_out1_data = 8
    if phase == 218:
        in_valid = 1
        in_data = 154
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 153
        expected_out1_data = 8
    if phase == 219:
        in_valid = 1
        in_data = 155
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 154
        expected_out1_data = 8
    if phase == 220:
        in_valid = 1
        in_data = 156
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 155
        expected_out1_data = 8
    if phase == 221:
        in_valid = 1
        in_data = 157
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 156
        expected_out1_data = 8
    if phase == 222:
        in_valid = 1
        in_data = 158
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 157
        expected_out1_data = 8
    if phase == 223:
        in_valid = 1
        in_data = 159
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 158
        expected_out1_data = 8
    if phase == 224:
        in_valid = 1
        in_data = 160
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 159
        expected_out1_data = 8
    if phase == 225:
        in_valid = 1
        in_data = 161
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 160
        expected_out1_data = 8
    if phase == 226:
        in_valid = 1
        in_data = 162
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 161
        expected_out1_data = 8
    if phase == 227:
        in_valid = 1
        in_data = 163
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 162
        expected_out1_data = 8
    if phase == 228:
        in_valid = 1
        in_data = 164
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 163
        expected_out1_data = 8
    if phase == 229:
        in_valid = 1
        in_data = 165
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 164
        expected_out1_data = 8
    if phase == 230:
        in_valid = 1
        in_data = 166
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 165
        expected_out1_data = 8
    if phase == 231:
        in_valid = 1
        in_data = 167
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 166
        expected_out1_data = 8
    if phase == 232:
        in_valid = 1
        in_data = 168
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 167
        expected_out1_data = 8
    if phase == 233:
        in_valid = 1
        in_data = 169
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 168
        expected_out1_data = 8
    if phase == 234:
        in_valid = 1
        in_data = 170
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 169
        expected_out1_data = 8
    if phase == 235:
        in_valid = 1
        in_data = 171
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 170
        expected_out1_data = 8
    if phase == 236:
        in_valid = 1
        in_data = 172
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 171
        expected_out1_data = 8
    if phase == 237:
        in_valid = 1
        in_data = 173
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 172
        expected_out1_data = 8
    if phase == 238:
        in_valid = 1
        in_data = 174
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 173
        expected_out1_data = 8
    if phase == 239:
        in_valid = 1
        in_data = 175
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 174
        expected_out1_data = 8
    if phase == 240:
        in_valid = 1
        in_data = 176
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 175
        expected_out1_data = 8
    if phase == 241:
        in_valid = 1
        in_data = 177
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 176
        expected_out1_data = 8
    if phase == 242:
        in_valid = 1
        in_data = 178
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 177
        expected_out1_data = 8
    if phase == 243:
        in_valid = 1
        in_data = 179
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 178
        expected_out1_data = 8
    if phase == 244:
        in_valid = 1
        in_data = 180
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 179
        expected_out1_data = 8
    if phase == 245:
        in_valid = 1
        in_data = 181
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 180
        expected_out1_data = 8
    if phase == 246:
        in_valid = 1
        in_data = 182
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 181
        expected_out1_data = 8
    if phase == 247:
        in_valid = 1
        in_data = 183
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 182
        expected_out1_data = 8
    if phase == 248:
        in_valid = 1
        in_data = 184
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 183
        expected_out1_data = 8
    if phase == 249:
        in_valid = 1
        in_data = 185
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 184
        expected_out1_data = 8
    if phase == 250:
        in_valid = 1
        in_data = 186
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 185
        expected_out1_data = 8
    if phase == 251:
        in_valid = 1
        in_data = 187
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 186
        expected_out1_data = 8
    if phase == 252:
        in_valid = 1
        in_data = 188
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 187
        expected_out1_data = 8
    if phase == 253:
        in_valid = 1
        in_data = 189
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 188
        expected_out1_data = 8
    if phase == 254:
        in_valid = 1
        in_data = 190
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 189
        expected_out1_data = 8
    if phase == 255:
        in_valid = 1
        in_data = 191
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 190
        expected_out1_data = 8
    if phase == 256:
        in_valid = 1
        in_data = 192
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 191
        expected_out1_data = 8
    if phase == 257:
        in_valid = 1
        in_data = 193
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 192
        expected_out1_data = 8
    if phase == 258:
        in_valid = 1
        in_data = 194
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 193
        expected_out1_data = 8
    if phase == 259:
        in_valid = 1
        in_data = 195
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 194
        expected_out1_data = 8
    if phase == 260:
        in_valid = 1
        in_data = 196
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 195
        expected_out1_data = 8
    if phase == 261:
        in_valid = 1
        in_data = 197
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 196
        expected_out1_data = 8
    if phase == 262:
        in_valid = 1
        in_data = 198
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 197
        expected_out1_data = 8
    if phase == 263:
        in_valid = 1
        in_data = 199
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 198
        expected_out1_data = 8
    if phase == 264:
        in_valid = 1
        in_data = 200
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 199
        expected_out1_data = 8
    if phase == 265:
        in_valid = 1
        in_data = 201
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 200
        expected_out1_data = 8
    if phase == 266:
        in_valid = 1
        in_data = 202
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 201
        expected_out1_data = 8
    if phase == 267:
        in_valid = 1
        in_data = 203
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 202
        expected_out1_data = 8
    if phase == 268:
        in_valid = 1
        in_data = 204
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 203
        expected_out1_data = 8
    if phase == 269:
        in_valid = 1
        in_data = 205
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 204
        expected_out1_data = 8
    if phase == 270:
        in_valid = 1
        in_data = 206
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 205
        expected_out1_data = 8
    if phase == 271:
        in_valid = 1
        in_data = 207
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 206
        expected_out1_data = 8
    if phase == 272:
        in_valid = 1
        in_data = 208
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 207
        expected_out1_data = 8
    if phase == 273:
        in_valid = 1
        in_data = 209
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 208
        expected_out1_data = 8
    if phase == 274:
        in_valid = 1
        in_data = 210
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 209
        expected_out1_data = 8
    if phase == 275:
        in_valid = 1
        in_data = 211
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 210
        expected_out1_data = 8
    if phase == 276:
        in_valid = 1
        in_data = 212
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 211
        expected_out1_data = 8
    if phase == 277:
        in_valid = 1
        in_data = 213
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 212
        expected_out1_data = 8
    if phase == 278:
        in_valid = 1
        in_data = 214
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 213
        expected_out1_data = 8
    if phase == 279:
        in_valid = 1
        in_data = 215
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 214
        expected_out1_data = 8
    if phase == 280:
        in_valid = 1
        in_data = 216
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 215
        expected_out1_data = 8
    if phase == 281:
        in_valid = 1
        in_data = 217
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 216
        expected_out1_data = 8
    if phase == 282:
        in_valid = 1
        in_data = 218
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 217
        expected_out1_data = 8
    if phase == 283:
        in_valid = 1
        in_data = 219
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 218
        expected_out1_data = 8
    if phase == 284:
        in_valid = 1
        in_data = 220
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 219
        expected_out1_data = 8
    if phase == 285:
        in_valid = 1
        in_data = 221
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 220
        expected_out1_data = 8
    if phase == 286:
        in_valid = 1
        in_data = 222
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 221
        expected_out1_data = 8
    if phase == 287:
        in_valid = 1
        in_data = 223
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 222
        expected_out1_data = 8
    if phase == 288:
        in_valid = 1
        in_data = 224
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 223
        expected_out1_data = 8
    if phase == 289:
        in_valid = 1
        in_data = 225
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 224
        expected_out1_data = 8
    if phase == 290:
        in_valid = 1
        in_data = 226
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 225
        expected_out1_data = 8
    if phase == 291:
        in_valid = 1
        in_data = 227
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 226
        expected_out1_data = 8
    if phase == 292:
        in_valid = 1
        in_data = 228
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 227
        expected_out1_data = 8
    if phase == 293:
        in_valid = 1
        in_data = 229
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 228
        expected_out1_data = 8
    if phase == 294:
        in_valid = 1
        in_data = 230
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 229
        expected_out1_data = 8
    if phase == 295:
        in_valid = 1
        in_data = 231
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 230
        expected_out1_data = 8
    if phase == 296:
        in_valid = 1
        in_data = 232
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 231
        expected_out1_data = 8
    if phase == 297:
        in_valid = 1
        in_data = 233
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 232
        expected_out1_data = 8
    if phase == 298:
        in_valid = 1
        in_data = 234
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 233
        expected_out1_data = 8
    if phase == 299:
        in_valid = 1
        in_data = 235
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 234
        expected_out1_data = 8
    if phase == 300:
        in_valid = 1
        in_data = 236
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 235
        expected_out1_data = 8
    if phase == 301:
        in_valid = 1
        in_data = 237
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 236
        expected_out1_data = 8
    if phase == 302:
        in_valid = 1
        in_data = 238
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 237
        expected_out1_data = 8
    if phase == 303:
        in_valid = 1
        in_data = 239
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 238
        expected_out1_data = 8
    if phase == 304:
        in_valid = 1
        in_data = 240
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 239
        expected_out1_data = 8
    if phase == 305:
        in_valid = 1
        in_data = 241
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 240
        expected_out1_data = 8
    if phase == 306:
        in_valid = 1
        in_data = 242
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 241
        expected_out1_data = 8
    if phase == 307:
        in_valid = 1
        in_data = 243
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 242
        expected_out1_data = 8
    if phase == 308:
        in_valid = 1
        in_data = 244
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 243
        expected_out1_data = 8
    if phase == 309:
        in_valid = 1
        in_data = 245
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 244
        expected_out1_data = 8
    if phase == 310:
        in_valid = 1
        in_data = 246
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 245
        expected_out1_data = 8
    if phase == 311:
        in_valid = 1
        in_data = 247
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 246
        expected_out1_data = 8
    if phase == 312:
        in_valid = 1
        in_data = 248
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 247
        expected_out1_data = 8
    if phase == 313:
        in_valid = 1
        in_data = 249
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 248
        expected_out1_data = 8
    if phase == 314:
        in_valid = 1
        in_data = 250
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 249
        expected_out1_data = 8
    if phase == 315:
        in_valid = 1
        in_data = 251
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 250
        expected_out1_data = 8
    if phase == 316:
        in_valid = 1
        in_data = 252
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 251
        expected_out1_data = 8
    if phase == 317:
        in_valid = 1
        in_data = 253
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 252
        expected_out1_data = 8
    if phase == 318:
        in_valid = 1
        in_data = 254
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 253
        expected_out1_data = 8
    if phase == 319:
        in_valid = 1
        in_data = 255
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 254
        expected_out1_data = 8
    if phase == 320:
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_valid = 1
        expected_out0_data = 255
        expected_out1_data = 8
    if phase == 321:
        in_data = 1
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 322:
        in_data = 2
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 323:
        in_data = 3
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 324:
        in_data = 4
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 325:
        in_data = 5
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 326:
        in_data = 6
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 327:
        in_data = 7
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 328:
        in_data = 8
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 329:
        in_data = 9
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 330:
        in_data = 10
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 331:
        in_data = 11
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 332:
        in_data = 12
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 333:
        in_data = 13
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 334:
        in_data = 14
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 335:
        in_data = 15
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 336:
        in_data = 16
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 337:
        in_data = 17
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 338:
        in_data = 18
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 339:
        in_data = 19
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 340:
        in_data = 20
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 341:
        in_data = 21
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 342:
        in_data = 22
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    if phase == 343:
        in_data = 23
        out0_ready = 1
        out1_ready = 1
        expected_in_ready = 1
        expected_out0_data = 8
        expected_out1_data = 8
    return Stimulus(
        in_valid=in_valid,
        in_data=in_data,
        out0_ready=out0_ready,
        out1_ready=out1_ready,
        expected_in_ready=expected_in_ready,
        expected_out0_valid=expected_out0_valid,
        expected_out0_data=expected_out0_data,
        expected_out1_valid=expected_out1_valid,
        expected_out1_data=expected_out1_data,
    )


@rule
def advance(phase):
    if phase < 343:
        phase = phase + 1


@system
def IssueQueue2PickerSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = IssueQueue2Picker(
        frame.in_valid, frame.in_data, frame.out0_ready, frame.out1_ready
    )

    @rule
    def exercise():
        assert (
            dut.in_ready == frame.expected_in_ready
        ), "issue_queue_2picker in_ready old-state check"
        assert (
            dut.out0_valid == frame.expected_out0_valid
        ), "issue_queue_2picker out0_valid old-state check"
        assert (
            dut.out0_data == frame.expected_out0_data
        ), "issue_queue_2picker out0_data old-state check"
        assert (
            dut.out1_valid == frame.expected_out1_valid
        ), "issue_queue_2picker out1_valid old-state check"
        assert (
            dut.out1_data == frame.expected_out1_data
        ), "issue_queue_2picker out1_data old-state check"
        log("info", "phase", phase)
        log("info", "in_ready", dut.in_ready)
        log("info", "out0_valid", dut.out0_valid)
        log("info", "out0_data", dut.out0_data)
        log("info", "out1_valid", dut.out1_valid)
        log("info", "out1_data", dut.out1_data)

    advance(phase)
    exercise()

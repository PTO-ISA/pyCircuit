"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_record_projection_update.record_projection_update import (
    RecordProjectionUpdate,
    Header,
    Packet,
    Result,
)
from pycircuit import bits, enum_to_bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_header_opcode: bits[4]
    input_tag: bits[8]
    input_payload: bits[16]
    input_valid: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data_header_opcode: bits[4]
    expected_data_tag: bits[8]
    expected_data_payload: bits[16]
    expected_data_valid: bits[1]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    take: bits[1] = 0
    input_header_opcode: bits[4] = 0
    input_tag: bits[8] = 0
    input_payload: bits[16] = 0
    input_valid: bits[1] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_data_header_opcode: bits[4] = 0
    expected_data_tag: bits[8] = 0
    expected_data_payload: bits[16] = 0
    expected_data_valid: bits[1] = 0
    if phase == 0:
        valid = 1
        input_tag = 128
        input_payload = 21845
        input_valid = 1
        expected_ready = 1
    if phase == 1:
        valid = 1
        input_header_opcode = 1
        input_tag = 127
        input_payload = 1
        expected_ready = 1
    if phase == 2:
        valid = 1
        input_header_opcode = 2
        input_tag = 170
        input_payload = 2
        input_valid = 1
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 3:
        valid = 1
        input_header_opcode = 3
        input_tag = 85
        input_payload = 4
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 4:
        valid = 1
        input_header_opcode = 4
        input_tag = 1
        input_payload = 8
        input_valid = 1
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 5:
        valid = 1
        input_header_opcode = 5
        input_tag = 2
        input_payload = 16
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 6:
        valid = 1
        input_header_opcode = 6
        input_tag = 4
        input_payload = 32
        input_valid = 1
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 7:
        valid = 1
        input_header_opcode = 7
        input_tag = 8
        input_payload = 64
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 8:
        valid = 1
        input_header_opcode = 8
        input_tag = 16
        input_payload = 128
        input_valid = 1
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 9:
        valid = 1
        input_header_opcode = 9
        input_tag = 32
        input_payload = 256
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 10:
        valid = 1
        input_header_opcode = 10
        input_tag = 64
        input_payload = 512
        input_valid = 1
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 11:
        valid = 1
        input_header_opcode = 11
        input_tag = 128
        input_payload = 1024
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 12:
        valid = 1
        input_header_opcode = 12
        input_tag = 1
        input_payload = 2048
        input_valid = 1
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 13:
        valid = 1
        take = 1
        input_header_opcode = 13
        input_tag = 3
        input_payload = 4096
        expected_ready = 1
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 14:
        valid = 1
        take = 1
        input_header_opcode = 14
        input_tag = 7
        input_payload = 8192
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 1
        expected_data_tag = 127
        expected_data_payload = 1
        expected_data_valid = 1
    if phase == 15:
        valid = 1
        take = 1
        input_header_opcode = 15
        input_tag = 15
        input_payload = 16384
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 13
        expected_data_tag = 3
        expected_data_payload = 4096
        expected_data_valid = 1
    if phase == 16:
        valid = 1
        input_tag = 31
        input_payload = 32768
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 14
        expected_data_tag = 7
        expected_data_payload = 8192
        expected_data_valid = 1
    if phase == 17:
        valid = 1
        take = 1
        input_header_opcode = 1
        input_tag = 63
        input_payload = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 14
        expected_data_tag = 7
        expected_data_payload = 8192
        expected_data_valid = 1
    if phase == 18:
        valid = 1
        take = 1
        input_header_opcode = 2
        input_tag = 127
        input_payload = 3
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 15
        expected_data_payload = 16384
        expected_data_valid = 1
    if phase == 19:
        valid = 1
        take = 1
        input_header_opcode = 3
        input_tag = 255
        input_payload = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 1
        expected_data_tag = 63
        expected_data_payload = 1
        expected_data_valid = 1
    if phase == 20:
        valid = 1
        input_header_opcode = 4
        input_tag = 28
        input_payload = 15
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 2
        expected_data_tag = 127
        expected_data_payload = 3
        expected_data_valid = 1
    if phase == 21:
        valid = 1
        take = 1
        input_header_opcode = 5
        input_tag = 7
        input_payload = 31
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 2
        expected_data_tag = 127
        expected_data_payload = 3
        expected_data_valid = 1
    if phase == 22:
        valid = 1
        take = 1
        input_header_opcode = 6
        input_tag = 242
        input_payload = 63
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 3
        expected_data_tag = 255
        expected_data_payload = 7
        expected_data_valid = 1
    if phase == 23:
        valid = 1
        take = 1
        input_header_opcode = 7
        input_tag = 221
        input_payload = 127
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 5
        expected_data_tag = 7
        expected_data_payload = 31
        expected_data_valid = 1
    if phase == 24:
        valid = 1
        input_header_opcode = 8
        input_tag = 200
        input_payload = 255
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 6
        expected_data_tag = 242
        expected_data_payload = 63
        expected_data_valid = 1
    if phase == 25:
        valid = 1
        take = 1
        input_header_opcode = 9
        input_tag = 179
        input_payload = 511
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 6
        expected_data_tag = 242
        expected_data_payload = 63
        expected_data_valid = 1
    if phase == 26:
        valid = 1
        take = 1
        input_header_opcode = 10
        input_tag = 158
        input_payload = 1023
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 7
        expected_data_tag = 221
        expected_data_payload = 127
        expected_data_valid = 1
    if phase == 27:
        valid = 1
        take = 1
        input_header_opcode = 11
        input_tag = 137
        input_payload = 2047
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 9
        expected_data_tag = 179
        expected_data_payload = 511
        expected_data_valid = 1
    if phase == 28:
        valid = 1
        input_header_opcode = 12
        input_tag = 116
        input_payload = 4095
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 10
        expected_data_tag = 158
        expected_data_payload = 1023
        expected_data_valid = 1
    if phase == 29:
        valid = 1
        take = 1
        input_header_opcode = 13
        input_tag = 95
        input_payload = 8191
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 10
        expected_data_tag = 158
        expected_data_payload = 1023
        expected_data_valid = 1
    if phase == 30:
        valid = 1
        take = 1
        input_header_opcode = 14
        input_tag = 74
        input_payload = 16383
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 11
        expected_data_tag = 137
        expected_data_payload = 2047
        expected_data_valid = 1
    if phase == 31:
        take = 1
        input_header_opcode = 15
        input_tag = 53
        input_payload = 32767
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 13
        expected_data_tag = 95
        expected_data_payload = 8191
        expected_data_valid = 1
    if phase == 32:
        valid = 1
        input_tag = 32
        input_payload = 65535
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 14
        expected_data_tag = 74
        expected_data_payload = 16383
        expected_data_valid = 1
    if phase == 33:
        valid = 1
        take = 1
        input_header_opcode = 1
        input_tag = 11
        input_payload = 8396
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 14
        expected_data_tag = 74
        expected_data_payload = 16383
        expected_data_valid = 1
    if phase == 34:
        valid = 1
        take = 1
        input_header_opcode = 2
        input_tag = 246
        input_payload = 56503
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_tag = 32
        expected_data_payload = 65535
        expected_data_valid = 1
    if phase == 35:
        valid = 1
        take = 1
        input_header_opcode = 3
        input_tag = 225
        input_payload = 26786
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 1
        expected_data_tag = 11
        expected_data_payload = 8396
        expected_data_valid = 1
    if phase == 36:
        input_header_opcode = 4
        input_tag = 204
        input_payload = 62605
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 2
        expected_data_tag = 246
        expected_data_payload = 56503
        expected_data_valid = 1
    if phase == 37:
        valid = 1
        take = 1
        input_header_opcode = 5
        input_tag = 183
        input_payload = 28792
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 2
        expected_data_tag = 246
        expected_data_payload = 56503
        expected_data_valid = 1
    if phase == 38:
        valid = 1
        take = 1
        input_header_opcode = 6
        input_tag = 162
        input_payload = 52323
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 3
        expected_data_tag = 225
        expected_data_payload = 26786
        expected_data_valid = 1
    if phase == 39:
        valid = 1
        take = 1
        input_header_opcode = 7
        input_tag = 141
        input_payload = 22606
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 5
        expected_data_tag = 183
        expected_data_payload = 28792
        expected_data_valid = 1
    if phase == 40:
        valid = 1
        input_header_opcode = 8
        input_tag = 120
        input_payload = 50233
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 6
        expected_data_tag = 162
        expected_data_payload = 52323
        expected_data_valid = 1
    if phase == 41:
        take = 1
        input_header_opcode = 9
        input_tag = 99
        input_payload = 16420
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 6
        expected_data_tag = 162
        expected_data_payload = 52323
        expected_data_valid = 1
    if phase == 42:
        valid = 1
        take = 1
        input_header_opcode = 10
        input_tag = 78
        input_payload = 15375
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 7
        expected_data_tag = 141
        expected_data_payload = 22606
        expected_data_valid = 1
    if phase == 43:
        valid = 1
        take = 1
        input_header_opcode = 11
        input_tag = 57
        input_payload = 47098
        expected_ready = 1
    if phase == 44:
        valid = 1
        input_header_opcode = 12
        input_tag = 36
        input_payload = 21477
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 10
        expected_data_tag = 78
        expected_data_payload = 15375
        expected_data_valid = 1
    if phase == 45:
        valid = 1
        take = 1
        input_header_opcode = 13
        input_tag = 15
        input_payload = 57296
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 10
        expected_data_tag = 78
        expected_data_payload = 15375
        expected_data_valid = 1
    if phase == 46:
        take = 1
        input_header_opcode = 14
        input_tag = 250
        input_payload = 27579
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 11
        expected_data_tag = 57
        expected_data_payload = 47098
        expected_data_valid = 1
    if phase == 47:
        valid = 1
        take = 1
        input_header_opcode = 15
        input_tag = 229
        input_payload = 59302
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 13
        expected_data_tag = 15
        expected_data_payload = 57296
        expected_data_valid = 1
    if phase == 48:
        valid = 1
        input_tag = 208
        input_payload = 25489
        input_valid = 1
        expected_ready = 1
    if phase == 49:
        valid = 1
        input_header_opcode = 1
        input_tag = 187
        input_payload = 61308
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 50:
        valid = 1
        input_header_opcode = 2
        input_tag = 166
        input_payload = 7015
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 51:
        input_header_opcode = 3
        input_tag = 145
        input_payload = 38738
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 52:
        valid = 1
        input_header_opcode = 4
        input_tag = 124
        input_payload = 13117
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 53:
        valid = 1
        input_header_opcode = 5
        input_tag = 103
        input_payload = 48936
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 54:
        valid = 1
        input_header_opcode = 6
        input_tag = 82
        input_payload = 2835
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 55:
        valid = 1
        input_header_opcode = 7
        input_tag = 61
        input_payload = 34558
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 56:
        input_header_opcode = 8
        input_tag = 40
        input_payload = 745
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 57:
        valid = 1
        input_header_opcode = 9
        input_tag = 19
        input_payload = 36564
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 58:
        valid = 1
        input_header_opcode = 10
        input_tag = 254
        input_payload = 64191
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 59:
        valid = 1
        input_header_opcode = 11
        input_tag = 233
        input_payload = 30378
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 60:
        valid = 1
        input_header_opcode = 12
        input_tag = 212
        input_payload = 4757
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 61:
        input_header_opcode = 13
        input_tag = 191
        input_payload = 40576
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 62:
        valid = 1
        input_header_opcode = 14
        input_tag = 170
        input_payload = 10859
        input_valid = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 63:
        valid = 1
        input_header_opcode = 15
        input_tag = 149
        input_payload = 42582
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 64:
        valid = 1
        take = 1
        input_tag = 128
        input_payload = 21845
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 65:
        valid = 1
        take = 1
        input_header_opcode = 1
        input_tag = 127
        input_payload = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_tag = 208
        expected_data_payload = 25489
        expected_data_valid = 1
    if phase == 66:
        valid = 1
        take = 1
        input_header_opcode = 2
        input_tag = 170
        input_payload = 2
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 21845
        expected_data_valid = 1
    if phase == 67:
        valid = 1
        take = 1
        input_header_opcode = 3
        input_tag = 85
        input_payload = 4
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 1
        expected_data_tag = 127
        expected_data_payload = 1
        expected_data_valid = 1
    if phase == 68:
        valid = 1
        take = 1
        input_header_opcode = 4
        input_tag = 1
        input_payload = 8
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 2
        expected_data_tag = 170
        expected_data_payload = 2
        expected_data_valid = 1
    if phase == 69:
        valid = 1
        take = 1
        input_header_opcode = 5
        input_tag = 2
        input_payload = 16
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 3
        expected_data_tag = 85
        expected_data_payload = 4
        expected_data_valid = 1
    if phase == 70:
        valid = 1
        take = 1
        input_header_opcode = 6
        input_tag = 4
        input_payload = 32
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 4
        expected_data_tag = 1
        expected_data_payload = 8
        expected_data_valid = 1
    if phase == 71:
        valid = 1
        take = 1
        input_header_opcode = 7
        input_tag = 8
        input_payload = 64
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 5
        expected_data_tag = 2
        expected_data_payload = 16
        expected_data_valid = 1
    if phase == 72:
        valid = 1
        take = 1
        input_header_opcode = 8
        input_tag = 16
        input_payload = 128
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 6
        expected_data_tag = 4
        expected_data_payload = 32
        expected_data_valid = 1
    if phase == 73:
        valid = 1
        take = 1
        input_header_opcode = 9
        input_tag = 32
        input_payload = 256
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 7
        expected_data_tag = 8
        expected_data_payload = 64
        expected_data_valid = 1
    if phase == 74:
        valid = 1
        take = 1
        input_header_opcode = 10
        input_tag = 64
        input_payload = 512
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 8
        expected_data_tag = 16
        expected_data_payload = 128
        expected_data_valid = 1
    if phase == 75:
        valid = 1
        take = 1
        input_header_opcode = 11
        input_tag = 128
        input_payload = 1024
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 9
        expected_data_tag = 32
        expected_data_payload = 256
        expected_data_valid = 1
    if phase == 76:
        valid = 1
        take = 1
        input_header_opcode = 12
        input_tag = 1
        input_payload = 2048
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 10
        expected_data_tag = 64
        expected_data_payload = 512
        expected_data_valid = 1
    if phase == 77:
        valid = 1
        take = 1
        input_header_opcode = 13
        input_tag = 3
        input_payload = 4096
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 11
        expected_data_tag = 128
        expected_data_payload = 1024
        expected_data_valid = 1
    if phase == 78:
        valid = 1
        take = 1
        input_header_opcode = 14
        input_tag = 7
        input_payload = 8192
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 12
        expected_data_tag = 1
        expected_data_payload = 2048
        expected_data_valid = 1
    if phase == 79:
        valid = 1
        take = 1
        input_header_opcode = 15
        input_tag = 15
        input_payload = 16384
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 13
        expected_data_tag = 3
        expected_data_payload = 4096
        expected_data_valid = 1
    if phase == 80:
        valid = 1
        take = 1
        input_tag = 31
        input_payload = 32768
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 14
        expected_data_tag = 7
        expected_data_payload = 8192
        expected_data_valid = 1
    if phase == 81:
        valid = 1
        take = 1
        input_header_opcode = 1
        input_tag = 63
        input_payload = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 15
        expected_data_payload = 16384
        expected_data_valid = 1
    if phase == 82:
        valid = 1
        take = 1
        input_header_opcode = 2
        input_tag = 127
        input_payload = 3
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_tag = 31
        expected_data_payload = 32768
        expected_data_valid = 1
    if phase == 83:
        valid = 1
        take = 1
        input_header_opcode = 3
        input_tag = 255
        input_payload = 7
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 1
        expected_data_tag = 63
        expected_data_payload = 1
        expected_data_valid = 1
    if phase == 84:
        valid = 1
        take = 1
        input_header_opcode = 4
        input_tag = 28
        input_payload = 15
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 2
        expected_data_tag = 127
        expected_data_payload = 3
        expected_data_valid = 1
    if phase == 85:
        valid = 1
        take = 1
        input_header_opcode = 5
        input_tag = 7
        input_payload = 31
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 3
        expected_data_tag = 255
        expected_data_payload = 7
        expected_data_valid = 1
    if phase == 86:
        valid = 1
        take = 1
        input_header_opcode = 6
        input_tag = 242
        input_payload = 63
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 4
        expected_data_tag = 28
        expected_data_payload = 15
        expected_data_valid = 1
    if phase == 87:
        valid = 1
        take = 1
        input_header_opcode = 7
        input_tag = 221
        input_payload = 127
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 5
        expected_data_tag = 7
        expected_data_payload = 31
        expected_data_valid = 1
    if phase == 88:
        valid = 1
        take = 1
        input_header_opcode = 8
        input_tag = 200
        input_payload = 255
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 6
        expected_data_tag = 242
        expected_data_payload = 63
        expected_data_valid = 1
    if phase == 89:
        valid = 1
        take = 1
        input_header_opcode = 9
        input_tag = 179
        input_payload = 511
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 7
        expected_data_tag = 221
        expected_data_payload = 127
        expected_data_valid = 1
    if phase == 90:
        valid = 1
        take = 1
        input_header_opcode = 10
        input_tag = 158
        input_payload = 1023
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 8
        expected_data_tag = 200
        expected_data_payload = 255
        expected_data_valid = 1
    if phase == 91:
        valid = 1
        take = 1
        input_header_opcode = 11
        input_tag = 137
        input_payload = 2047
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 9
        expected_data_tag = 179
        expected_data_payload = 511
        expected_data_valid = 1
    if phase == 92:
        valid = 1
        take = 1
        input_header_opcode = 12
        input_tag = 116
        input_payload = 4095
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 10
        expected_data_tag = 158
        expected_data_payload = 1023
        expected_data_valid = 1
    if phase == 93:
        valid = 1
        take = 1
        input_header_opcode = 13
        input_tag = 95
        input_payload = 8191
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 11
        expected_data_tag = 137
        expected_data_payload = 2047
        expected_data_valid = 1
    if phase == 94:
        valid = 1
        take = 1
        input_header_opcode = 14
        input_tag = 74
        input_payload = 16383
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 12
        expected_data_tag = 116
        expected_data_payload = 4095
        expected_data_valid = 1
    if phase == 95:
        valid = 1
        take = 1
        input_header_opcode = 15
        input_tag = 53
        input_payload = 32767
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 13
        expected_data_tag = 95
        expected_data_payload = 8191
        expected_data_valid = 1
    if phase == 96:
        valid = 1
        take = 1
        input_tag = 32
        input_payload = 65535
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 14
        expected_data_tag = 74
        expected_data_payload = 16383
        expected_data_valid = 1
    if phase == 97:
        valid = 1
        take = 1
        input_header_opcode = 1
        input_tag = 11
        input_payload = 8396
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 53
        expected_data_payload = 32767
        expected_data_valid = 1
    if phase == 98:
        valid = 1
        take = 1
        input_header_opcode = 2
        input_tag = 246
        input_payload = 56503
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_tag = 32
        expected_data_payload = 65535
        expected_data_valid = 1
    if phase == 99:
        valid = 1
        take = 1
        input_header_opcode = 3
        input_tag = 225
        input_payload = 26786
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 1
        expected_data_tag = 11
        expected_data_payload = 8396
        expected_data_valid = 1
    if phase == 100:
        valid = 1
        take = 1
        input_header_opcode = 4
        input_tag = 204
        input_payload = 62605
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 2
        expected_data_tag = 246
        expected_data_payload = 56503
        expected_data_valid = 1
    if phase == 101:
        valid = 1
        take = 1
        input_header_opcode = 5
        input_tag = 183
        input_payload = 28792
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 3
        expected_data_tag = 225
        expected_data_payload = 26786
        expected_data_valid = 1
    if phase == 102:
        valid = 1
        take = 1
        input_header_opcode = 6
        input_tag = 162
        input_payload = 52323
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 4
        expected_data_tag = 204
        expected_data_payload = 62605
        expected_data_valid = 1
    if phase == 103:
        valid = 1
        take = 1
        input_header_opcode = 7
        input_tag = 141
        input_payload = 22606
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 5
        expected_data_tag = 183
        expected_data_payload = 28792
        expected_data_valid = 1
    if phase == 104:
        valid = 1
        take = 1
        input_header_opcode = 8
        input_tag = 120
        input_payload = 50233
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 6
        expected_data_tag = 162
        expected_data_payload = 52323
        expected_data_valid = 1
    if phase == 105:
        valid = 1
        take = 1
        input_header_opcode = 9
        input_tag = 99
        input_payload = 16420
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 7
        expected_data_tag = 141
        expected_data_payload = 22606
        expected_data_valid = 1
    if phase == 106:
        valid = 1
        take = 1
        input_header_opcode = 10
        input_tag = 78
        input_payload = 15375
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 8
        expected_data_tag = 120
        expected_data_payload = 50233
        expected_data_valid = 1
    if phase == 107:
        valid = 1
        take = 1
        input_header_opcode = 11
        input_tag = 57
        input_payload = 47098
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 9
        expected_data_tag = 99
        expected_data_payload = 16420
        expected_data_valid = 1
    if phase == 108:
        valid = 1
        take = 1
        input_header_opcode = 12
        input_tag = 36
        input_payload = 21477
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 10
        expected_data_tag = 78
        expected_data_payload = 15375
        expected_data_valid = 1
    if phase == 109:
        valid = 1
        take = 1
        input_header_opcode = 13
        input_tag = 15
        input_payload = 57296
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 11
        expected_data_tag = 57
        expected_data_payload = 47098
        expected_data_valid = 1
    if phase == 110:
        valid = 1
        take = 1
        input_header_opcode = 14
        input_tag = 250
        input_payload = 27579
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 12
        expected_data_tag = 36
        expected_data_payload = 21477
        expected_data_valid = 1
    if phase == 111:
        valid = 1
        take = 1
        input_header_opcode = 15
        input_tag = 229
        input_payload = 59302
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 13
        expected_data_tag = 15
        expected_data_payload = 57296
        expected_data_valid = 1
    if phase == 112:
        valid = 1
        take = 1
        input_tag = 208
        input_payload = 25489
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 14
        expected_data_tag = 250
        expected_data_payload = 27579
        expected_data_valid = 1
    if phase == 113:
        valid = 1
        take = 1
        input_header_opcode = 1
        input_tag = 187
        input_payload = 61308
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 229
        expected_data_payload = 59302
        expected_data_valid = 1
    if phase == 114:
        valid = 1
        take = 1
        input_header_opcode = 2
        input_tag = 166
        input_payload = 7015
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_tag = 208
        expected_data_payload = 25489
        expected_data_valid = 1
    if phase == 115:
        valid = 1
        take = 1
        input_header_opcode = 3
        input_tag = 145
        input_payload = 38738
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 1
        expected_data_tag = 187
        expected_data_payload = 61308
        expected_data_valid = 1
    if phase == 116:
        valid = 1
        take = 1
        input_header_opcode = 4
        input_tag = 124
        input_payload = 13117
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 2
        expected_data_tag = 166
        expected_data_payload = 7015
        expected_data_valid = 1
    if phase == 117:
        valid = 1
        take = 1
        input_header_opcode = 5
        input_tag = 103
        input_payload = 48936
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 3
        expected_data_tag = 145
        expected_data_payload = 38738
        expected_data_valid = 1
    if phase == 118:
        valid = 1
        take = 1
        input_header_opcode = 6
        input_tag = 82
        input_payload = 2835
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 4
        expected_data_tag = 124
        expected_data_payload = 13117
        expected_data_valid = 1
    if phase == 119:
        valid = 1
        take = 1
        input_header_opcode = 7
        input_tag = 61
        input_payload = 34558
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 5
        expected_data_tag = 103
        expected_data_payload = 48936
        expected_data_valid = 1
    if phase == 120:
        valid = 1
        take = 1
        input_header_opcode = 8
        input_tag = 40
        input_payload = 745
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 6
        expected_data_tag = 82
        expected_data_payload = 2835
        expected_data_valid = 1
    if phase == 121:
        valid = 1
        take = 1
        input_header_opcode = 9
        input_tag = 19
        input_payload = 36564
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 7
        expected_data_tag = 61
        expected_data_payload = 34558
        expected_data_valid = 1
    if phase == 122:
        valid = 1
        take = 1
        input_header_opcode = 10
        input_tag = 254
        input_payload = 64191
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 8
        expected_data_tag = 40
        expected_data_payload = 745
        expected_data_valid = 1
    if phase == 123:
        valid = 1
        take = 1
        input_header_opcode = 11
        input_tag = 233
        input_payload = 30378
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 9
        expected_data_tag = 19
        expected_data_payload = 36564
        expected_data_valid = 1
    if phase == 124:
        valid = 1
        take = 1
        input_header_opcode = 12
        input_tag = 212
        input_payload = 4757
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 10
        expected_data_tag = 254
        expected_data_payload = 64191
        expected_data_valid = 1
    if phase == 125:
        valid = 1
        take = 1
        input_header_opcode = 13
        input_tag = 191
        input_payload = 40576
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 11
        expected_data_tag = 233
        expected_data_payload = 30378
        expected_data_valid = 1
    if phase == 126:
        valid = 1
        take = 1
        input_header_opcode = 14
        input_tag = 170
        input_payload = 10859
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 12
        expected_data_tag = 212
        expected_data_payload = 4757
        expected_data_valid = 1
    if phase == 127:
        valid = 1
        take = 1
        input_header_opcode = 15
        input_tag = 149
        input_payload = 42582
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 13
        expected_data_tag = 191
        expected_data_payload = 40576
        expected_data_valid = 1
    if phase == 128:
        valid = 1
        take = 1
        input_tag = 128
        input_payload = 8769
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 14
        expected_data_tag = 170
        expected_data_payload = 10859
        expected_data_valid = 1
    if phase == 129:
        valid = 1
        take = 1
        input_header_opcode = 1
        input_tag = 107
        input_payload = 44588
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 149
        expected_data_payload = 42582
        expected_data_valid = 1
    if phase == 130:
        valid = 1
        take = 1
        input_header_opcode = 2
        input_tag = 86
        input_payload = 23063
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_tag = 128
        expected_data_payload = 8769
        expected_data_valid = 1
    if phase == 131:
        valid = 1
        take = 1
        input_header_opcode = 3
        input_tag = 65
        input_payload = 54786
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 1
        expected_data_tag = 107
        expected_data_payload = 44588
        expected_data_valid = 1
    if phase == 132:
        valid = 1
        take = 1
        input_header_opcode = 4
        input_tag = 44
        input_payload = 29165
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 2
        expected_data_tag = 86
        expected_data_payload = 23063
        expected_data_valid = 1
    if phase == 133:
        valid = 1
        take = 1
        input_header_opcode = 5
        input_tag = 23
        input_payload = 64984
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 3
        expected_data_tag = 65
        expected_data_payload = 54786
        expected_data_valid = 1
    if phase == 134:
        valid = 1
        take = 1
        input_header_opcode = 6
        input_tag = 2
        input_payload = 18883
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 4
        expected_data_tag = 44
        expected_data_payload = 29165
        expected_data_valid = 1
    if phase == 135:
        valid = 1
        take = 1
        input_header_opcode = 7
        input_tag = 237
        input_payload = 50606
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 5
        expected_data_tag = 23
        expected_data_payload = 64984
        expected_data_valid = 1
    if phase == 136:
        valid = 1
        take = 1
        input_header_opcode = 8
        input_tag = 216
        input_payload = 16793
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 6
        expected_data_tag = 2
        expected_data_payload = 18883
        expected_data_valid = 1
    if phase == 137:
        valid = 1
        take = 1
        input_header_opcode = 9
        input_tag = 195
        input_payload = 52612
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 7
        expected_data_tag = 237
        expected_data_payload = 50606
        expected_data_valid = 1
    if phase == 138:
        valid = 1
        take = 1
        input_header_opcode = 10
        input_tag = 174
        input_payload = 47471
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 8
        expected_data_tag = 216
        expected_data_payload = 16793
        expected_data_valid = 1
    if phase == 139:
        valid = 1
        take = 1
        input_header_opcode = 11
        input_tag = 153
        input_payload = 13658
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 9
        expected_data_tag = 195
        expected_data_payload = 52612
        expected_data_valid = 1
    if phase == 140:
        valid = 1
        take = 1
        input_header_opcode = 12
        input_tag = 132
        input_payload = 53573
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 10
        expected_data_tag = 174
        expected_data_payload = 47471
        expected_data_valid = 1
    if phase == 141:
        valid = 1
        take = 1
        input_header_opcode = 13
        input_tag = 111
        input_payload = 23856
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 11
        expected_data_tag = 153
        expected_data_payload = 13658
        expected_data_valid = 1
    if phase == 142:
        valid = 1
        take = 1
        input_header_opcode = 14
        input_tag = 90
        input_payload = 59675
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 12
        expected_data_tag = 132
        expected_data_payload = 53573
        expected_data_valid = 1
    if phase == 143:
        valid = 1
        take = 1
        input_header_opcode = 15
        input_tag = 69
        input_payload = 25862
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 13
        expected_data_tag = 111
        expected_data_payload = 23856
        expected_data_valid = 1
    if phase == 144:
        valid = 1
        take = 1
        input_tag = 48
        input_payload = 57585
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 14
        expected_data_tag = 90
        expected_data_payload = 59675
        expected_data_valid = 1
    if phase == 145:
        valid = 1
        take = 1
        input_header_opcode = 1
        input_tag = 27
        input_payload = 27868
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 69
        expected_data_payload = 25862
        expected_data_valid = 1
    if phase == 146:
        valid = 1
        take = 1
        input_header_opcode = 2
        input_tag = 6
        input_payload = 39111
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_tag = 48
        expected_data_payload = 57585
        expected_data_valid = 1
    if phase == 147:
        valid = 1
        take = 1
        input_header_opcode = 3
        input_tag = 241
        input_payload = 5298
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 1
        expected_data_tag = 27
        expected_data_payload = 27868
        expected_data_valid = 1
    if phase == 148:
        valid = 1
        take = 1
        input_header_opcode = 4
        input_tag = 220
        input_payload = 45213
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 2
        expected_data_tag = 6
        expected_data_payload = 39111
        expected_data_valid = 1
    if phase == 149:
        valid = 1
        take = 1
        input_header_opcode = 5
        input_tag = 199
        input_payload = 15496
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 3
        expected_data_tag = 241
        expected_data_payload = 5298
        expected_data_valid = 1
    if phase == 150:
        valid = 1
        take = 1
        input_header_opcode = 6
        input_tag = 178
        input_payload = 34931
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 4
        expected_data_tag = 220
        expected_data_payload = 45213
        expected_data_valid = 1
    if phase == 151:
        valid = 1
        take = 1
        input_header_opcode = 7
        input_tag = 157
        input_payload = 1118
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 5
        expected_data_tag = 199
        expected_data_payload = 15496
        expected_data_valid = 1
    if phase == 152:
        valid = 1
        take = 1
        input_header_opcode = 8
        input_tag = 136
        input_payload = 32841
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 6
        expected_data_tag = 178
        expected_data_payload = 34931
        expected_data_valid = 1
    if phase == 153:
        valid = 1
        take = 1
        input_header_opcode = 9
        input_tag = 115
        input_payload = 3124
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 7
        expected_data_tag = 157
        expected_data_payload = 1118
        expected_data_valid = 1
    if phase == 154:
        valid = 1
        take = 1
        input_header_opcode = 10
        input_tag = 94
        input_payload = 30751
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 8
        expected_data_tag = 136
        expected_data_payload = 32841
        expected_data_valid = 1
    if phase == 155:
        valid = 1
        take = 1
        input_header_opcode = 11
        input_tag = 73
        input_payload = 62474
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 9
        expected_data_tag = 115
        expected_data_payload = 3124
        expected_data_valid = 1
    if phase == 156:
        valid = 1
        take = 1
        input_header_opcode = 12
        input_tag = 52
        input_payload = 28661
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 10
        expected_data_tag = 94
        expected_data_payload = 30751
        expected_data_valid = 1
    if phase == 157:
        valid = 1
        take = 1
        input_header_opcode = 13
        input_tag = 31
        input_payload = 7136
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 11
        expected_data_tag = 73
        expected_data_payload = 62474
        expected_data_valid = 1
    if phase == 158:
        valid = 1
        take = 1
        input_header_opcode = 14
        input_tag = 10
        input_payload = 42955
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 12
        expected_data_tag = 52
        expected_data_payload = 28661
        expected_data_valid = 1
    if phase == 159:
        valid = 1
        take = 1
        input_header_opcode = 15
        input_tag = 245
        input_payload = 9142
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 13
        expected_data_tag = 31
        expected_data_payload = 7136
        expected_data_valid = 1
    if phase == 160:
        take = 1
        input_tag = 224
        input_payload = 49057
        input_valid = 1
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 14
        expected_data_tag = 10
        expected_data_payload = 42955
        expected_data_valid = 1
    if phase == 161:
        take = 1
        input_header_opcode = 1
        input_tag = 203
        input_payload = 11148
        expected_ready = 1
        expected_valid = 1
        expected_data_header_opcode = 15
        expected_data_tag = 245
        expected_data_payload = 9142
        expected_data_valid = 1
    if phase == 162:
        take = 1
        input_header_opcode = 2
        input_tag = 182
        input_payload = 55159
        input_valid = 1
        expected_ready = 1
    if phase == 163:
        take = 1
        input_header_opcode = 3
        input_tag = 161
        input_payload = 21346
        expected_ready = 1
    if phase == 164:
        take = 1
        input_header_opcode = 4
        input_tag = 140
        input_payload = 53069
        input_valid = 1
        expected_ready = 1
    if phase == 165:
        take = 1
        input_header_opcode = 5
        input_tag = 119
        input_payload = 31544
        expected_ready = 1
    if phase == 166:
        take = 1
        input_header_opcode = 6
        input_tag = 98
        input_payload = 50979
        input_valid = 1
        expected_ready = 1
    if phase == 167:
        take = 1
        input_header_opcode = 7
        input_tag = 77
        input_payload = 17166
        expected_ready = 1
    if phase == 168:
        take = 1
        input_header_opcode = 8
        input_tag = 56
        input_payload = 57081
        input_valid = 1
        expected_ready = 1
    if phase == 169:
        take = 1
        input_header_opcode = 9
        input_tag = 35
        input_payload = 19172
        expected_ready = 1
    if phase == 170:
        take = 1
        input_header_opcode = 10
        input_tag = 14
        input_payload = 14031
        input_valid = 1
        expected_ready = 1
    if phase == 171:
        take = 1
        input_header_opcode = 11
        input_tag = 249
        input_payload = 45754
        expected_ready = 1
    if phase == 172:
        take = 1
        input_header_opcode = 12
        input_tag = 228
        input_payload = 11941
        input_valid = 1
        expected_ready = 1
    if phase == 173:
        take = 1
        input_header_opcode = 13
        input_tag = 207
        input_payload = 55952
        expected_ready = 1
    if phase == 174:
        take = 1
        input_header_opcode = 14
        input_tag = 186
        input_payload = 26235
        input_valid = 1
        expected_ready = 1
    if phase == 175:
        take = 1
        input_header_opcode = 15
        input_tag = 165
        input_payload = 57958
        expected_ready = 1
    if phase == 176:
        take = 1
        input_tag = 144
        input_payload = 32337
        input_valid = 1
        expected_ready = 1
    if phase == 177:
        take = 1
        input_header_opcode = 1
        input_tag = 123
        input_payload = 59964
        expected_ready = 1
    if phase == 178:
        take = 1
        input_header_opcode = 2
        input_tag = 102
        input_payload = 5671
        input_valid = 1
        expected_ready = 1
    if phase == 179:
        take = 1
        input_header_opcode = 3
        input_tag = 81
        input_payload = 37394
        expected_ready = 1
    if phase == 180:
        take = 1
        input_header_opcode = 4
        input_tag = 60
        input_payload = 3581
        input_valid = 1
        expected_ready = 1
    if phase == 181:
        take = 1
        input_header_opcode = 5
        input_tag = 39
        input_payload = 47592
        expected_ready = 1
    if phase == 182:
        take = 1
        input_header_opcode = 6
        input_tag = 18
        input_payload = 1491
        input_valid = 1
        expected_ready = 1
    if phase == 183:
        take = 1
        input_header_opcode = 7
        input_tag = 253
        input_payload = 33214
        expected_ready = 1
    return Stimulus(
        valid=valid,
        take=take,
        input_header_opcode=input_header_opcode,
        input_tag=input_tag,
        input_payload=input_payload,
        input_valid=input_valid,
        expected_ready=expected_ready,
        expected_valid=expected_valid,
        expected_data_header_opcode=expected_data_header_opcode,
        expected_data_tag=expected_data_tag,
        expected_data_payload=expected_data_payload,
        expected_data_valid=expected_data_valid,
    )


@rule
def advance(phase):
    if phase < 183:
        phase = phase + 1


@system
def RecordProjectionUpdateSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = Packet(
        header=Header(opcode=frame.input_header_opcode),
        tag=frame.input_tag,
        payload=frame.input_payload,
        valid=frame.input_valid,
    )
    dut = RecordProjectionUpdate(frame.valid, packet, frame.take)

    @rule
    def check():
        assert (
            dut.ready == frame.expected_ready
        ), "record_projection_update input capacity"
        assert (
            dut.valid == frame.expected_valid
        ), "record_projection_update result availability"
        assert (
            dut.data.header.opcode == frame.expected_data_header_opcode
        ), "record_projection_update header.opcode old-state check"
        assert (
            dut.data.tag == frame.expected_data_tag
        ), "record_projection_update tag old-state check"
        assert (
            dut.data.payload == frame.expected_data_payload
        ), "record_projection_update payload old-state check"
        assert (
            dut.data.valid == frame.expected_data_valid
        ), "record_projection_update valid old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "header_opcode", dut.data.header.opcode)
        log("info", "tag", dut.data.tag)
        log("info", "payload", dut.data.payload)
        log("info", "valid", dut.data.valid)

    advance(phase)
    check()

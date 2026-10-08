"""Regular-clock known-state checks; original physical-control oracles remain independent."""

from example_queue_pipeline.queue_pipeline import QueuePipeline
from pycircuit import bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    data: bits[64]
    take: bits[1]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_data: bits[64]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    data: bits[64] = 0
    take: bits[1] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_data: bits[64] = 0
    if phase == 0:
        valid = 1
        expected_ready = 1
    elif phase == 1:
        valid = 1
        data = 18446744073709551615
        expected_ready = 1
    elif phase == 2:
        valid = 1
        data = 18446744073709551614
        expected_ready = 1
        expected_valid = 1
        expected_data = 1
    elif phase == 3:
        valid = 1
        data = 9223372036854775808
        expected_ready = 1
        expected_valid = 1
        expected_data = 1
    elif phase == 4:
        valid = 1
        data = 9223372036854775807
        expected_valid = 1
        expected_data = 1
    elif phase == 5:
        valid = 1
        data = 12297829382473034410
        expected_valid = 1
        expected_data = 1
    elif phase == 6:
        valid = 1
        data = 6148914691236517205
        expected_valid = 1
        expected_data = 1
    elif phase == 7:
        valid = 1
        data = 1
        expected_valid = 1
        expected_data = 1
    elif phase == 8:
        valid = 1
        data = 2
        expected_valid = 1
        expected_data = 1
    elif phase == 9:
        valid = 1
        data = 4
        expected_valid = 1
        expected_data = 1
    elif phase == 10:
        valid = 1
        data = 8
        expected_valid = 1
        expected_data = 1
    elif phase == 11:
        valid = 1
        data = 16
        expected_valid = 1
        expected_data = 1
    elif phase == 12:
        valid = 1
        data = 32
        expected_valid = 1
        expected_data = 1
    elif phase == 13:
        valid = 1
        data = 64
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1
    elif phase == 14:
        valid = 1
        data = 128
        take = 1
        expected_ready = 1
        expected_valid = 1
    elif phase == 15:
        valid = 1
        data = 256
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 18446744073709551615
    elif phase == 16:
        valid = 1
        data = 512
        expected_valid = 1
        expected_data = 9223372036854775809
    elif phase == 17:
        valid = 1
        data = 1024
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 9223372036854775809
    elif phase == 18:
        valid = 1
        data = 2048
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 65
    elif phase == 19:
        valid = 1
        data = 4096
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 129
    elif phase == 20:
        valid = 1
        data = 8192
        expected_valid = 1
        expected_data = 257
    elif phase == 21:
        valid = 1
        data = 16384
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 257
    elif phase == 22:
        valid = 1
        data = 32768
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1025
    elif phase == 23:
        valid = 1
        data = 65536
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2049
    elif phase == 24:
        valid = 1
        data = 131072
        expected_valid = 1
        expected_data = 4097
    elif phase == 25:
        valid = 1
        data = 262144
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 4097
    elif phase == 26:
        valid = 1
        data = 524288
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 16385
    elif phase == 27:
        valid = 1
        data = 1048576
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 32769
    elif phase == 28:
        valid = 1
        data = 2097152
        expected_valid = 1
        expected_data = 65537
    elif phase == 29:
        valid = 1
        data = 4194304
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 65537
    elif phase == 30:
        valid = 1
        data = 8388608
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 262145
    elif phase == 31:
        data = 16777216
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 524289
    elif phase == 32:
        valid = 1
        data = 33554432
        expected_ready = 1
        expected_valid = 1
        expected_data = 1048577
    elif phase == 33:
        valid = 1
        data = 67108864
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1048577
    elif phase == 34:
        valid = 1
        data = 134217728
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 4194305
    elif phase == 35:
        valid = 1
        data = 268435456
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 8388609
    elif phase == 36:
        data = 536870912
        expected_valid = 1
        expected_data = 33554433
    elif phase == 37:
        valid = 1
        data = 1073741824
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 33554433
    elif phase == 38:
        valid = 1
        data = 2147483648
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 67108865
    elif phase == 39:
        valid = 1
        data = 4294967296
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 134217729
    elif phase == 40:
        valid = 1
        data = 8589934592
        expected_valid = 1
        expected_data = 268435457
    elif phase == 41:
        data = 17179869184
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 268435457
    elif phase == 42:
        valid = 1
        data = 34359738368
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1073741825
    elif phase == 43:
        valid = 1
        data = 68719476736
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2147483649
    elif phase == 44:
        valid = 1
        data = 137438953472
        expected_ready = 1
        expected_valid = 1
        expected_data = 4294967297
    elif phase == 45:
        valid = 1
        data = 274877906944
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 4294967297
    elif phase == 46:
        data = 549755813888
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 34359738369
    elif phase == 47:
        valid = 1
        data = 1099511627776
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 68719476737
    elif phase == 48:
        valid = 1
        data = 2199023255552
        expected_ready = 1
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 49:
        valid = 1
        data = 4398046511104
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 50:
        valid = 1
        data = 8796093022208
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 51:
        data = 17592186044416
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 52:
        valid = 1
        data = 35184372088832
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 53:
        valid = 1
        data = 70368744177664
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 54:
        valid = 1
        data = 140737488355328
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 55:
        valid = 1
        data = 281474976710656
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 56:
        data = 562949953421312
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 57:
        valid = 1
        data = 1125899906842624
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 58:
        valid = 1
        data = 2251799813685248
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 59:
        valid = 1
        data = 4503599627370496
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 60:
        valid = 1
        data = 9007199254740992
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 61:
        data = 18014398509481984
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 62:
        valid = 1
        data = 36028797018963968
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 63:
        valid = 1
        data = 72057594037927936
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 64:
        valid = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 65:
        valid = 1
        data = 18446744073709551615
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 274877906945
    elif phase == 66:
        valid = 1
        data = 18446744073709551614
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1099511627777
    elif phase == 67:
        valid = 1
        data = 9223372036854775808
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2199023255553
    elif phase == 68:
        valid = 1
        data = 9223372036854775807
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1
    elif phase == 69:
        valid = 1
        data = 12297829382473034410
        take = 1
        expected_ready = 1
        expected_valid = 1
    elif phase == 70:
        valid = 1
        data = 6148914691236517205
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 18446744073709551615
    elif phase == 71:
        valid = 1
        data = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 9223372036854775809
    elif phase == 72:
        valid = 1
        data = 2
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 9223372036854775808
    elif phase == 73:
        valid = 1
        data = 4
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 12297829382473034411
    elif phase == 74:
        valid = 1
        data = 8
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 6148914691236517206
    elif phase == 75:
        valid = 1
        data = 16
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2
    elif phase == 76:
        valid = 1
        data = 32
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 3
    elif phase == 77:
        valid = 1
        data = 64
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 5
    elif phase == 78:
        valid = 1
        data = 128
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 9
    elif phase == 79:
        valid = 1
        data = 256
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 17
    elif phase == 80:
        valid = 1
        data = 512
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 33
    elif phase == 81:
        valid = 1
        data = 1024
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 65
    elif phase == 82:
        valid = 1
        data = 2048
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 129
    elif phase == 83:
        valid = 1
        data = 4096
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 257
    elif phase == 84:
        valid = 1
        data = 8192
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 513
    elif phase == 85:
        valid = 1
        data = 16384
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1025
    elif phase == 86:
        valid = 1
        data = 32768
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2049
    elif phase == 87:
        valid = 1
        data = 65536
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 4097
    elif phase == 88:
        valid = 1
        data = 131072
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 8193
    elif phase == 89:
        valid = 1
        data = 262144
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 16385
    elif phase == 90:
        valid = 1
        data = 524288
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 32769
    elif phase == 91:
        valid = 1
        data = 1048576
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 65537
    elif phase == 92:
        valid = 1
        data = 2097152
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 131073
    elif phase == 93:
        valid = 1
        data = 4194304
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 262145
    elif phase == 94:
        valid = 1
        data = 8388608
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 524289
    elif phase == 95:
        valid = 1
        data = 16777216
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1048577
    elif phase == 96:
        valid = 1
        data = 33554432
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2097153
    elif phase == 97:
        valid = 1
        data = 67108864
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 4194305
    elif phase == 98:
        valid = 1
        data = 134217728
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 8388609
    elif phase == 99:
        valid = 1
        data = 268435456
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 16777217
    elif phase == 100:
        valid = 1
        data = 536870912
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 33554433
    elif phase == 101:
        valid = 1
        data = 1073741824
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 67108865
    elif phase == 102:
        valid = 1
        data = 2147483648
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 134217729
    elif phase == 103:
        valid = 1
        data = 4294967296
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 268435457
    elif phase == 104:
        valid = 1
        data = 8589934592
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 536870913
    elif phase == 105:
        valid = 1
        data = 17179869184
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1073741825
    elif phase == 106:
        valid = 1
        data = 34359738368
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2147483649
    elif phase == 107:
        valid = 1
        data = 68719476736
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 4294967297
    elif phase == 108:
        valid = 1
        data = 137438953472
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 8589934593
    elif phase == 109:
        valid = 1
        data = 274877906944
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 17179869185
    elif phase == 110:
        valid = 1
        data = 549755813888
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 34359738369
    elif phase == 111:
        valid = 1
        data = 1099511627776
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 68719476737
    elif phase == 112:
        valid = 1
        data = 2199023255552
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 137438953473
    elif phase == 113:
        valid = 1
        data = 4398046511104
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 274877906945
    elif phase == 114:
        valid = 1
        data = 8796093022208
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 549755813889
    elif phase == 115:
        valid = 1
        data = 17592186044416
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1099511627777
    elif phase == 116:
        valid = 1
        data = 35184372088832
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2199023255553
    elif phase == 117:
        valid = 1
        data = 70368744177664
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 4398046511105
    elif phase == 118:
        valid = 1
        data = 140737488355328
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 8796093022209
    elif phase == 119:
        valid = 1
        data = 281474976710656
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 17592186044417
    elif phase == 120:
        valid = 1
        data = 562949953421312
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 35184372088833
    elif phase == 121:
        valid = 1
        data = 1125899906842624
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 70368744177665
    elif phase == 122:
        valid = 1
        data = 2251799813685248
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 140737488355329
    elif phase == 123:
        valid = 1
        data = 4503599627370496
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 281474976710657
    elif phase == 124:
        valid = 1
        data = 9007199254740992
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 562949953421313
    elif phase == 125:
        valid = 1
        data = 18014398509481984
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1125899906842625
    elif phase == 126:
        valid = 1
        data = 36028797018963968
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2251799813685249
    elif phase == 127:
        valid = 1
        data = 72057594037927936
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 4503599627370497
    elif phase == 128:
        valid = 1
        data = 144115188075855872
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 9007199254740993
    elif phase == 129:
        valid = 1
        data = 288230376151711744
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 18014398509481985
    elif phase == 130:
        valid = 1
        data = 576460752303423488
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 36028797018963969
    elif phase == 131:
        valid = 1
        data = 1152921504606846976
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 72057594037927937
    elif phase == 132:
        valid = 1
        data = 2305843009213693952
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 144115188075855873
    elif phase == 133:
        valid = 1
        data = 4611686018427387904
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 288230376151711745
    elif phase == 134:
        data = 9223372036854775808
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 576460752303423489
    elif phase == 135:
        data = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 1152921504606846977
    elif phase == 136:
        data = 2
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 2305843009213693953
    elif phase == 137:
        data = 4
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_data = 4611686018427387905
    elif phase == 138:
        data = 8
        take = 1
        expected_ready = 1
    elif phase == 139:
        data = 16
        take = 1
        expected_ready = 1
    elif phase == 140:
        data = 32
        take = 1
        expected_ready = 1
    elif phase == 141:
        data = 64
        take = 1
        expected_ready = 1
    elif phase == 142:
        data = 128
        take = 1
        expected_ready = 1
    elif phase == 143:
        data = 256
        take = 1
        expected_ready = 1
    elif phase == 144:
        data = 512
        take = 1
        expected_ready = 1
    elif phase == 145:
        data = 1024
        take = 1
        expected_ready = 1
    elif phase == 146:
        data = 2048
        take = 1
        expected_ready = 1
    elif phase == 147:
        data = 4096
        take = 1
        expected_ready = 1
    elif phase == 148:
        data = 8192
        take = 1
        expected_ready = 1
    elif phase == 149:
        data = 16384
        take = 1
        expected_ready = 1
    elif phase == 150:
        data = 32768
        take = 1
        expected_ready = 1
    elif phase == 151:
        data = 65536
        take = 1
        expected_ready = 1
    elif phase == 152:
        data = 131072
        take = 1
        expected_ready = 1
    elif phase == 153:
        data = 262144
        take = 1
        expected_ready = 1
    elif phase == 154:
        data = 524288
        take = 1
        expected_ready = 1
    elif phase == 155:
        data = 1048576
        take = 1
        expected_ready = 1
    elif phase == 156:
        data = 2097152
        take = 1
        expected_ready = 1
    elif phase == 157:
        data = 4194304
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
    if phase < 157:
        phase = phase + 1


@system
def QueuePipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    dut = QueuePipeline(frame.valid, frame.data, frame.take)

    @rule
    def exercise():
        assert dut.ready == frame.expected_ready, "queue_pipeline ready old-state check"
        assert dut.valid == frame.expected_valid, "queue_pipeline valid old-state check"
        assert dut.data == frame.expected_data, "queue_pipeline data old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "data", dut.data)

    advance(phase)
    exercise()

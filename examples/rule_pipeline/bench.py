"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_rule_pipeline.rule_pipeline import RulePipeline, RuleToken, RuleResult
from pycircuit import bits, enum_to_bits, log, rule, struct, system


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_value: bits[16]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_value: bits[16]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    valid: bits[1] = 0
    take: bits[1] = 0
    input_value: bits[16] = 0
    expected_ready: bits[1] = 0
    expected_valid: bits[1] = 0
    expected_value: bits[16] = 0
    if phase == 0:
        valid = 1
        expected_ready = 1
    if phase == 1:
        valid = 1
        input_value = 65535
        expected_ready = 1
    if phase == 2:
        valid = 1
        input_value = 65534
        expected_ready = 1
        expected_valid = 1
        expected_value = 1
    if phase == 3:
        valid = 1
        input_value = 32768
        expected_valid = 1
        expected_value = 1
    if phase == 4:
        valid = 1
        input_value = 32767
        expected_valid = 1
        expected_value = 1
    if phase == 5:
        valid = 1
        input_value = 43690
        expected_valid = 1
        expected_value = 1
    if phase == 6:
        valid = 1
        input_value = 21845
        expected_valid = 1
        expected_value = 1
    if phase == 7:
        valid = 1
        input_value = 1
        expected_valid = 1
        expected_value = 1
    if phase == 8:
        valid = 1
        input_value = 2
        expected_valid = 1
        expected_value = 1
    if phase == 9:
        valid = 1
        input_value = 4
        expected_valid = 1
        expected_value = 1
    if phase == 10:
        valid = 1
        input_value = 8
        expected_valid = 1
        expected_value = 1
    if phase == 11:
        valid = 1
        input_value = 16
        expected_valid = 1
        expected_value = 1
    if phase == 12:
        valid = 1
        input_value = 32
        expected_valid = 1
        expected_value = 1
    if phase == 13:
        valid = 1
        take = 1
        input_value = 64
        expected_ready = 1
        expected_valid = 1
        expected_value = 1
    if phase == 14:
        valid = 1
        take = 1
        input_value = 128
        expected_ready = 1
        expected_valid = 1
    if phase == 15:
        valid = 1
        take = 1
        input_value = 256
        expected_ready = 1
        expected_valid = 1
        expected_value = 65535
    if phase == 16:
        valid = 1
        input_value = 512
        expected_valid = 1
        expected_value = 65
    if phase == 17:
        valid = 1
        take = 1
        input_value = 1024
        expected_ready = 1
        expected_valid = 1
        expected_value = 65
    if phase == 18:
        valid = 1
        take = 1
        input_value = 2048
        expected_ready = 1
        expected_valid = 1
        expected_value = 129
    if phase == 19:
        valid = 1
        take = 1
        input_value = 4096
        expected_ready = 1
        expected_valid = 1
        expected_value = 257
    if phase == 20:
        valid = 1
        input_value = 8192
        expected_valid = 1
        expected_value = 1025
    if phase == 21:
        valid = 1
        take = 1
        input_value = 16384
        expected_ready = 1
        expected_valid = 1
        expected_value = 1025
    if phase == 22:
        valid = 1
        take = 1
        input_value = 32768
        expected_ready = 1
        expected_valid = 1
        expected_value = 2049
    if phase == 23:
        valid = 1
        take = 1
        input_value = 1
        expected_ready = 1
        expected_valid = 1
        expected_value = 4097
    if phase == 24:
        valid = 1
        input_value = 3
        expected_valid = 1
        expected_value = 16385
    if phase == 25:
        valid = 1
        take = 1
        input_value = 7
        expected_ready = 1
        expected_valid = 1
        expected_value = 16385
    if phase == 26:
        valid = 1
        take = 1
        input_value = 15
        expected_ready = 1
        expected_valid = 1
        expected_value = 32769
    if phase == 27:
        valid = 1
        take = 1
        input_value = 31
        expected_ready = 1
        expected_valid = 1
        expected_value = 2
    if phase == 28:
        valid = 1
        input_value = 63
        expected_valid = 1
        expected_value = 8
    if phase == 29:
        valid = 1
        take = 1
        input_value = 127
        expected_ready = 1
        expected_valid = 1
        expected_value = 8
    if phase == 30:
        valid = 1
        take = 1
        input_value = 255
        expected_ready = 1
        expected_valid = 1
        expected_value = 16
    if phase == 31:
        take = 1
        input_value = 511
        expected_ready = 1
        expected_valid = 1
        expected_value = 32
    if phase == 32:
        valid = 1
        input_value = 1023
        expected_ready = 1
        expected_valid = 1
        expected_value = 128
    if phase == 33:
        valid = 1
        take = 1
        input_value = 2047
        expected_ready = 1
        expected_valid = 1
        expected_value = 128
    if phase == 34:
        valid = 1
        take = 1
        input_value = 4095
        expected_ready = 1
        expected_valid = 1
        expected_value = 256
    if phase == 35:
        valid = 1
        take = 1
        input_value = 8191
        expected_ready = 1
        expected_valid = 1
        expected_value = 1024
    if phase == 36:
        input_value = 16383
        expected_valid = 1
        expected_value = 2048
    if phase == 37:
        valid = 1
        take = 1
        input_value = 32767
        expected_ready = 1
        expected_valid = 1
        expected_value = 2048
    if phase == 38:
        valid = 1
        take = 1
        input_value = 65535
        expected_ready = 1
        expected_valid = 1
        expected_value = 4096
    if phase == 39:
        valid = 1
        take = 1
        input_value = 8396
        expected_ready = 1
        expected_valid = 1
        expected_value = 8192
    if phase == 40:
        valid = 1
        input_value = 56503
        expected_valid = 1
        expected_value = 32768
    if phase == 41:
        take = 1
        input_value = 26786
        expected_ready = 1
        expected_valid = 1
        expected_value = 32768
    if phase == 42:
        valid = 1
        take = 1
        input_value = 62605
        expected_ready = 1
        expected_valid = 1
    if phase == 43:
        valid = 1
        take = 1
        input_value = 28792
        expected_ready = 1
        expected_valid = 1
        expected_value = 8397
    if phase == 44:
        valid = 1
        input_value = 52323
        expected_ready = 1
        expected_valid = 1
        expected_value = 62606
    if phase == 45:
        valid = 1
        take = 1
        input_value = 22606
        expected_ready = 1
        expected_valid = 1
        expected_value = 62606
    if phase == 46:
        take = 1
        input_value = 50233
        expected_ready = 1
        expected_valid = 1
        expected_value = 28793
    if phase == 47:
        valid = 1
        take = 1
        input_value = 16420
        expected_ready = 1
        expected_valid = 1
        expected_value = 52324
    if phase == 48:
        valid = 1
        input_value = 15375
        expected_ready = 1
        expected_valid = 1
        expected_value = 22607
    if phase == 49:
        valid = 1
        input_value = 47098
        expected_valid = 1
        expected_value = 22607
    if phase == 50:
        valid = 1
        input_value = 21477
        expected_valid = 1
        expected_value = 22607
    if phase == 51:
        input_value = 57296
        expected_valid = 1
        expected_value = 22607
    if phase == 52:
        valid = 1
        input_value = 27579
        expected_valid = 1
        expected_value = 22607
    if phase == 53:
        valid = 1
        input_value = 59302
        expected_valid = 1
        expected_value = 22607
    if phase == 54:
        valid = 1
        input_value = 25489
        expected_valid = 1
        expected_value = 22607
    if phase == 55:
        valid = 1
        input_value = 61308
        expected_valid = 1
        expected_value = 22607
    if phase == 56:
        input_value = 7015
        expected_valid = 1
        expected_value = 22607
    if phase == 57:
        valid = 1
        input_value = 38738
        expected_valid = 1
        expected_value = 22607
    if phase == 58:
        valid = 1
        input_value = 13117
        expected_valid = 1
        expected_value = 22607
    if phase == 59:
        valid = 1
        input_value = 48936
        expected_valid = 1
        expected_value = 22607
    if phase == 60:
        valid = 1
        input_value = 2835
        expected_valid = 1
        expected_value = 22607
    if phase == 61:
        input_value = 34558
        expected_valid = 1
        expected_value = 22607
    if phase == 62:
        valid = 1
        input_value = 745
        expected_valid = 1
        expected_value = 22607
    if phase == 63:
        valid = 1
        input_value = 36564
        expected_valid = 1
        expected_value = 22607
    if phase == 64:
        valid = 1
        take = 1
        expected_ready = 1
        expected_valid = 1
        expected_value = 22607
    if phase == 65:
        valid = 1
        take = 1
        input_value = 65535
        expected_ready = 1
        expected_valid = 1
        expected_value = 16421
    if phase == 66:
        valid = 1
        take = 1
        input_value = 65534
        expected_ready = 1
        expected_valid = 1
        expected_value = 15376
    if phase == 67:
        valid = 1
        take = 1
        input_value = 32768
        expected_ready = 1
        expected_valid = 1
        expected_value = 1
    if phase == 68:
        valid = 1
        take = 1
        input_value = 32767
        expected_ready = 1
        expected_valid = 1
    if phase == 69:
        valid = 1
        take = 1
        input_value = 43690
        expected_ready = 1
        expected_valid = 1
        expected_value = 65535
    if phase == 70:
        valid = 1
        take = 1
        input_value = 21845
        expected_ready = 1
        expected_valid = 1
        expected_value = 32769
    if phase == 71:
        valid = 1
        take = 1
        input_value = 1
        expected_ready = 1
        expected_valid = 1
        expected_value = 32768
    if phase == 72:
        valid = 1
        take = 1
        input_value = 2
        expected_ready = 1
        expected_valid = 1
        expected_value = 43691
    if phase == 73:
        valid = 1
        take = 1
        input_value = 4
        expected_ready = 1
        expected_valid = 1
        expected_value = 21846
    if phase == 74:
        valid = 1
        take = 1
        input_value = 8
        expected_ready = 1
        expected_valid = 1
        expected_value = 2
    if phase == 75:
        valid = 1
        take = 1
        input_value = 16
        expected_ready = 1
        expected_valid = 1
        expected_value = 3
    if phase == 76:
        valid = 1
        take = 1
        input_value = 32
        expected_ready = 1
        expected_valid = 1
        expected_value = 5
    if phase == 77:
        valid = 1
        take = 1
        input_value = 64
        expected_ready = 1
        expected_valid = 1
        expected_value = 9
    if phase == 78:
        valid = 1
        take = 1
        input_value = 128
        expected_ready = 1
        expected_valid = 1
        expected_value = 17
    if phase == 79:
        valid = 1
        take = 1
        input_value = 256
        expected_ready = 1
        expected_valid = 1
        expected_value = 33
    if phase == 80:
        valid = 1
        take = 1
        input_value = 512
        expected_ready = 1
        expected_valid = 1
        expected_value = 65
    if phase == 81:
        valid = 1
        take = 1
        input_value = 1024
        expected_ready = 1
        expected_valid = 1
        expected_value = 129
    if phase == 82:
        valid = 1
        take = 1
        input_value = 2048
        expected_ready = 1
        expected_valid = 1
        expected_value = 257
    if phase == 83:
        valid = 1
        take = 1
        input_value = 4096
        expected_ready = 1
        expected_valid = 1
        expected_value = 513
    if phase == 84:
        valid = 1
        take = 1
        input_value = 8192
        expected_ready = 1
        expected_valid = 1
        expected_value = 1025
    if phase == 85:
        valid = 1
        take = 1
        input_value = 16384
        expected_ready = 1
        expected_valid = 1
        expected_value = 2049
    if phase == 86:
        valid = 1
        take = 1
        input_value = 32768
        expected_ready = 1
        expected_valid = 1
        expected_value = 4097
    if phase == 87:
        valid = 1
        take = 1
        input_value = 1
        expected_ready = 1
        expected_valid = 1
        expected_value = 8193
    if phase == 88:
        valid = 1
        take = 1
        input_value = 3
        expected_ready = 1
        expected_valid = 1
        expected_value = 16385
    if phase == 89:
        valid = 1
        take = 1
        input_value = 7
        expected_ready = 1
        expected_valid = 1
        expected_value = 32769
    if phase == 90:
        valid = 1
        take = 1
        input_value = 15
        expected_ready = 1
        expected_valid = 1
        expected_value = 2
    if phase == 91:
        valid = 1
        take = 1
        input_value = 31
        expected_ready = 1
        expected_valid = 1
        expected_value = 4
    if phase == 92:
        valid = 1
        take = 1
        input_value = 63
        expected_ready = 1
        expected_valid = 1
        expected_value = 8
    if phase == 93:
        valid = 1
        take = 1
        input_value = 127
        expected_ready = 1
        expected_valid = 1
        expected_value = 16
    if phase == 94:
        valid = 1
        take = 1
        input_value = 255
        expected_ready = 1
        expected_valid = 1
        expected_value = 32
    if phase == 95:
        valid = 1
        take = 1
        input_value = 511
        expected_ready = 1
        expected_valid = 1
        expected_value = 64
    if phase == 96:
        valid = 1
        take = 1
        input_value = 1023
        expected_ready = 1
        expected_valid = 1
        expected_value = 128
    if phase == 97:
        valid = 1
        take = 1
        input_value = 2047
        expected_ready = 1
        expected_valid = 1
        expected_value = 256
    if phase == 98:
        valid = 1
        take = 1
        input_value = 4095
        expected_ready = 1
        expected_valid = 1
        expected_value = 512
    if phase == 99:
        valid = 1
        take = 1
        input_value = 8191
        expected_ready = 1
        expected_valid = 1
        expected_value = 1024
    if phase == 100:
        valid = 1
        take = 1
        input_value = 16383
        expected_ready = 1
        expected_valid = 1
        expected_value = 2048
    if phase == 101:
        valid = 1
        take = 1
        input_value = 32767
        expected_ready = 1
        expected_valid = 1
        expected_value = 4096
    if phase == 102:
        valid = 1
        take = 1
        input_value = 65535
        expected_ready = 1
        expected_valid = 1
        expected_value = 8192
    if phase == 103:
        valid = 1
        take = 1
        input_value = 8396
        expected_ready = 1
        expected_valid = 1
        expected_value = 16384
    if phase == 104:
        valid = 1
        take = 1
        input_value = 56503
        expected_ready = 1
        expected_valid = 1
        expected_value = 32768
    if phase == 105:
        valid = 1
        take = 1
        input_value = 26786
        expected_ready = 1
        expected_valid = 1
    if phase == 106:
        valid = 1
        take = 1
        input_value = 62605
        expected_ready = 1
        expected_valid = 1
        expected_value = 8397
    if phase == 107:
        valid = 1
        take = 1
        input_value = 28792
        expected_ready = 1
        expected_valid = 1
        expected_value = 56504
    if phase == 108:
        valid = 1
        take = 1
        input_value = 52323
        expected_ready = 1
        expected_valid = 1
        expected_value = 26787
    if phase == 109:
        valid = 1
        take = 1
        input_value = 22606
        expected_ready = 1
        expected_valid = 1
        expected_value = 62606
    if phase == 110:
        valid = 1
        take = 1
        input_value = 50233
        expected_ready = 1
        expected_valid = 1
        expected_value = 28793
    if phase == 111:
        valid = 1
        take = 1
        input_value = 16420
        expected_ready = 1
        expected_valid = 1
        expected_value = 52324
    if phase == 112:
        valid = 1
        take = 1
        input_value = 15375
        expected_ready = 1
        expected_valid = 1
        expected_value = 22607
    if phase == 113:
        valid = 1
        take = 1
        input_value = 47098
        expected_ready = 1
        expected_valid = 1
        expected_value = 50234
    if phase == 114:
        valid = 1
        take = 1
        input_value = 21477
        expected_ready = 1
        expected_valid = 1
        expected_value = 16421
    if phase == 115:
        valid = 1
        take = 1
        input_value = 57296
        expected_ready = 1
        expected_valid = 1
        expected_value = 15376
    if phase == 116:
        valid = 1
        take = 1
        input_value = 27579
        expected_ready = 1
        expected_valid = 1
        expected_value = 47099
    if phase == 117:
        valid = 1
        take = 1
        input_value = 59302
        expected_ready = 1
        expected_valid = 1
        expected_value = 21478
    if phase == 118:
        valid = 1
        take = 1
        input_value = 25489
        expected_ready = 1
        expected_valid = 1
        expected_value = 57297
    if phase == 119:
        valid = 1
        take = 1
        input_value = 61308
        expected_ready = 1
        expected_valid = 1
        expected_value = 27580
    if phase == 120:
        valid = 1
        take = 1
        input_value = 7015
        expected_ready = 1
        expected_valid = 1
        expected_value = 59303
    if phase == 121:
        valid = 1
        take = 1
        input_value = 38738
        expected_ready = 1
        expected_valid = 1
        expected_value = 25490
    if phase == 122:
        valid = 1
        take = 1
        input_value = 13117
        expected_ready = 1
        expected_valid = 1
        expected_value = 61309
    if phase == 123:
        valid = 1
        take = 1
        input_value = 48936
        expected_ready = 1
        expected_valid = 1
        expected_value = 7016
    if phase == 124:
        valid = 1
        take = 1
        input_value = 2835
        expected_ready = 1
        expected_valid = 1
        expected_value = 38739
    if phase == 125:
        valid = 1
        take = 1
        input_value = 34558
        expected_ready = 1
        expected_valid = 1
        expected_value = 13118
    if phase == 126:
        valid = 1
        take = 1
        input_value = 745
        expected_ready = 1
        expected_valid = 1
        expected_value = 48937
    if phase == 127:
        valid = 1
        take = 1
        input_value = 36564
        expected_ready = 1
        expected_valid = 1
        expected_value = 2836
    if phase == 128:
        valid = 1
        take = 1
        input_value = 64191
        expected_ready = 1
        expected_valid = 1
        expected_value = 34559
    if phase == 129:
        valid = 1
        take = 1
        input_value = 30378
        expected_ready = 1
        expected_valid = 1
        expected_value = 746
    if phase == 130:
        valid = 1
        take = 1
        input_value = 4757
        expected_ready = 1
        expected_valid = 1
        expected_value = 36565
    if phase == 131:
        valid = 1
        take = 1
        input_value = 40576
        expected_ready = 1
        expected_valid = 1
        expected_value = 64192
    if phase == 132:
        valid = 1
        take = 1
        input_value = 10859
        expected_ready = 1
        expected_valid = 1
        expected_value = 30379
    if phase == 133:
        valid = 1
        take = 1
        input_value = 42582
        expected_ready = 1
        expected_valid = 1
        expected_value = 4758
    if phase == 134:
        valid = 1
        take = 1
        input_value = 8769
        expected_ready = 1
        expected_valid = 1
        expected_value = 40577
    if phase == 135:
        valid = 1
        take = 1
        input_value = 44588
        expected_ready = 1
        expected_valid = 1
        expected_value = 10860
    if phase == 136:
        valid = 1
        take = 1
        input_value = 23063
        expected_ready = 1
        expected_valid = 1
        expected_value = 42583
    if phase == 137:
        valid = 1
        take = 1
        input_value = 54786
        expected_ready = 1
        expected_valid = 1
        expected_value = 8770
    if phase == 138:
        valid = 1
        take = 1
        input_value = 29165
        expected_ready = 1
        expected_valid = 1
        expected_value = 44589
    if phase == 139:
        valid = 1
        take = 1
        input_value = 64984
        expected_ready = 1
        expected_valid = 1
        expected_value = 23064
    if phase == 140:
        valid = 1
        take = 1
        input_value = 18883
        expected_ready = 1
        expected_valid = 1
        expected_value = 54787
    if phase == 141:
        valid = 1
        take = 1
        input_value = 50606
        expected_ready = 1
        expected_valid = 1
        expected_value = 29166
    if phase == 142:
        valid = 1
        take = 1
        input_value = 16793
        expected_ready = 1
        expected_valid = 1
        expected_value = 64985
    if phase == 143:
        valid = 1
        take = 1
        input_value = 52612
        expected_ready = 1
        expected_valid = 1
        expected_value = 18884
    if phase == 144:
        valid = 1
        take = 1
        input_value = 47471
        expected_ready = 1
        expected_valid = 1
        expected_value = 50607
    if phase == 145:
        valid = 1
        take = 1
        input_value = 13658
        expected_ready = 1
        expected_valid = 1
        expected_value = 16794
    if phase == 146:
        valid = 1
        take = 1
        input_value = 53573
        expected_ready = 1
        expected_valid = 1
        expected_value = 52613
    if phase == 147:
        valid = 1
        take = 1
        input_value = 23856
        expected_ready = 1
        expected_valid = 1
        expected_value = 47472
    if phase == 148:
        valid = 1
        take = 1
        input_value = 59675
        expected_ready = 1
        expected_valid = 1
        expected_value = 13659
    if phase == 149:
        valid = 1
        take = 1
        input_value = 25862
        expected_ready = 1
        expected_valid = 1
        expected_value = 53574
    if phase == 150:
        valid = 1
        take = 1
        input_value = 57585
        expected_ready = 1
        expected_valid = 1
        expected_value = 23857
    if phase == 151:
        valid = 1
        take = 1
        input_value = 27868
        expected_ready = 1
        expected_valid = 1
        expected_value = 59676
    if phase == 152:
        valid = 1
        take = 1
        input_value = 39111
        expected_ready = 1
        expected_valid = 1
        expected_value = 25863
    if phase == 153:
        valid = 1
        take = 1
        input_value = 5298
        expected_ready = 1
        expected_valid = 1
        expected_value = 57586
    if phase == 154:
        valid = 1
        take = 1
        input_value = 45213
        expected_ready = 1
        expected_valid = 1
        expected_value = 27869
    if phase == 155:
        valid = 1
        take = 1
        input_value = 15496
        expected_ready = 1
        expected_valid = 1
        expected_value = 39112
    if phase == 156:
        valid = 1
        take = 1
        input_value = 34931
        expected_ready = 1
        expected_valid = 1
        expected_value = 5299
    if phase == 157:
        valid = 1
        take = 1
        input_value = 1118
        expected_ready = 1
        expected_valid = 1
        expected_value = 45214
    if phase == 158:
        valid = 1
        take = 1
        input_value = 32841
        expected_ready = 1
        expected_valid = 1
        expected_value = 15497
    if phase == 159:
        valid = 1
        take = 1
        input_value = 3124
        expected_ready = 1
        expected_valid = 1
        expected_value = 34932
    if phase == 160:
        take = 1
        input_value = 30751
        expected_ready = 1
        expected_valid = 1
        expected_value = 1119
    if phase == 161:
        take = 1
        input_value = 62474
        expected_ready = 1
        expected_valid = 1
        expected_value = 32842
    if phase == 162:
        take = 1
        input_value = 28661
        expected_ready = 1
        expected_valid = 1
        expected_value = 3125
    if phase == 163:
        take = 1
        input_value = 7136
        expected_ready = 1
    if phase == 164:
        take = 1
        input_value = 42955
        expected_ready = 1
    if phase == 165:
        take = 1
        input_value = 9142
        expected_ready = 1
    if phase == 166:
        take = 1
        input_value = 49057
        expected_ready = 1
    if phase == 167:
        take = 1
        input_value = 11148
        expected_ready = 1
    if phase == 168:
        take = 1
        input_value = 55159
        expected_ready = 1
    if phase == 169:
        take = 1
        input_value = 21346
        expected_ready = 1
    if phase == 170:
        take = 1
        input_value = 53069
        expected_ready = 1
    if phase == 171:
        take = 1
        input_value = 31544
        expected_ready = 1
    if phase == 172:
        take = 1
        input_value = 50979
        expected_ready = 1
    if phase == 173:
        take = 1
        input_value = 17166
        expected_ready = 1
    if phase == 174:
        take = 1
        input_value = 57081
        expected_ready = 1
    if phase == 175:
        take = 1
        input_value = 19172
        expected_ready = 1
    if phase == 176:
        take = 1
        input_value = 14031
        expected_ready = 1
    if phase == 177:
        take = 1
        input_value = 45754
        expected_ready = 1
    if phase == 178:
        take = 1
        input_value = 11941
        expected_ready = 1
    if phase == 179:
        take = 1
        input_value = 55952
        expected_ready = 1
    if phase == 180:
        take = 1
        input_value = 26235
        expected_ready = 1
    if phase == 181:
        take = 1
        input_value = 57958
        expected_ready = 1
    if phase == 182:
        take = 1
        input_value = 32337
        expected_ready = 1
    if phase == 183:
        take = 1
        input_value = 59964
        expected_ready = 1
    return Stimulus(
        valid=valid,
        take=take,
        input_value=input_value,
        expected_ready=expected_ready,
        expected_valid=expected_valid,
        expected_value=expected_value,
    )


@rule
def advance(phase):
    if phase < 183:
        phase = phase + 1


@system
def RulePipelineSystem():  # noqa: N802
    phase: bits[16] = 0
    frame = stimulus(phase)
    packet = RuleToken(value=frame.input_value)
    dut = RulePipeline(frame.valid, packet, frame.take)

    @rule
    def check():
        assert dut.ready == frame.expected_ready, "rule_pipeline input capacity"
        assert dut.valid == frame.expected_valid, "rule_pipeline result availability"
        assert (
            dut.data.value == frame.expected_value
        ), "rule_pipeline value old-state check"
        log("info", "phase", phase)
        log("info", "ready", dut.ready)
        log("info", "valid", dut.valid)
        log("info", "value", dut.data.value)

    advance(phase)
    check()

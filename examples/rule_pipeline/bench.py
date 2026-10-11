"""Regular-clock record checks; independent physical-control oracles remain intact."""

from example_rule_pipeline.rule_pipeline import RulePipeline, RuleToken, RuleResult
from pycircuit import bits, enum_to_bits, log, rule, struct, system, table


@struct
class Stimulus:
    valid: bits[1]
    take: bits[1]
    input_value: bits[16]
    expected_ready: bits[1]
    expected_valid: bits[1]
    expected_value: bits[16]


@struct
class Stimuli:
    rows: table[185, Stimulus]


@rule
def stimulus(phase: bits[16]) -> Stimulus:
    # fmt: off
    stimuli = Stimuli(
        rows=(
            Stimulus(valid=1, expected_ready=1),
            Stimulus(valid=1, input_value=65535, expected_ready=1),
            Stimulus(valid=1, input_value=65534, expected_ready=1, expected_valid=1, expected_value=1),
            Stimulus(valid=1, input_value=32768, expected_valid=1, expected_value=1),
            Stimulus(valid=1, input_value=32767, expected_valid=1, expected_value=1),
            Stimulus(valid=1, input_value=43690, expected_valid=1, expected_value=1),
            Stimulus(valid=1, input_value=21845, expected_valid=1, expected_value=1),
            Stimulus(valid=1, input_value=1, expected_valid=1, expected_value=1),
            Stimulus(valid=1, input_value=2, expected_valid=1, expected_value=1),
            Stimulus(valid=1, input_value=4, expected_valid=1, expected_value=1),
            Stimulus(valid=1, input_value=8, expected_valid=1, expected_value=1),
            Stimulus(valid=1, input_value=16, expected_valid=1, expected_value=1),
            Stimulus(valid=1, input_value=32, expected_valid=1, expected_value=1),
            Stimulus(valid=1, take=1, input_value=64, expected_ready=1, expected_valid=1, expected_value=1),
            Stimulus(valid=1, take=1, input_value=128, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_value=256, expected_ready=1, expected_valid=1, expected_value=65535),
            Stimulus(valid=1, input_value=512, expected_valid=1, expected_value=65),
            Stimulus(valid=1, take=1, input_value=1024, expected_ready=1, expected_valid=1, expected_value=65),
            Stimulus(valid=1, take=1, input_value=2048, expected_ready=1, expected_valid=1, expected_value=129),
            Stimulus(valid=1, take=1, input_value=4096, expected_ready=1, expected_valid=1, expected_value=257),
            Stimulus(valid=1, input_value=8192, expected_valid=1, expected_value=1025),
            Stimulus(valid=1, take=1, input_value=16384, expected_ready=1, expected_valid=1, expected_value=1025),
            Stimulus(valid=1, take=1, input_value=32768, expected_ready=1, expected_valid=1, expected_value=2049),
            Stimulus(valid=1, take=1, input_value=1, expected_ready=1, expected_valid=1, expected_value=4097),
            Stimulus(valid=1, input_value=3, expected_valid=1, expected_value=16385),
            Stimulus(valid=1, take=1, input_value=7, expected_ready=1, expected_valid=1, expected_value=16385),
            Stimulus(valid=1, take=1, input_value=15, expected_ready=1, expected_valid=1, expected_value=32769),
            Stimulus(valid=1, take=1, input_value=31, expected_ready=1, expected_valid=1, expected_value=2),
            Stimulus(valid=1, input_value=63, expected_valid=1, expected_value=8),
            Stimulus(valid=1, take=1, input_value=127, expected_ready=1, expected_valid=1, expected_value=8),
            Stimulus(valid=1, take=1, input_value=255, expected_ready=1, expected_valid=1, expected_value=16),
            Stimulus(take=1, input_value=511, expected_ready=1, expected_valid=1, expected_value=32),
            Stimulus(valid=1, input_value=1023, expected_ready=1, expected_valid=1, expected_value=128),
            Stimulus(valid=1, take=1, input_value=2047, expected_ready=1, expected_valid=1, expected_value=128),
            Stimulus(valid=1, take=1, input_value=4095, expected_ready=1, expected_valid=1, expected_value=256),
            Stimulus(valid=1, take=1, input_value=8191, expected_ready=1, expected_valid=1, expected_value=1024),
            Stimulus(input_value=16383, expected_valid=1, expected_value=2048),
            Stimulus(valid=1, take=1, input_value=32767, expected_ready=1, expected_valid=1, expected_value=2048),
            Stimulus(valid=1, take=1, input_value=65535, expected_ready=1, expected_valid=1, expected_value=4096),
            Stimulus(valid=1, take=1, input_value=8396, expected_ready=1, expected_valid=1, expected_value=8192),
            Stimulus(valid=1, input_value=56503, expected_valid=1, expected_value=32768),
            Stimulus(take=1, input_value=26786, expected_ready=1, expected_valid=1, expected_value=32768),
            Stimulus(valid=1, take=1, input_value=62605, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_value=28792, expected_ready=1, expected_valid=1, expected_value=8397),
            Stimulus(valid=1, input_value=52323, expected_ready=1, expected_valid=1, expected_value=62606),
            Stimulus(valid=1, take=1, input_value=22606, expected_ready=1, expected_valid=1, expected_value=62606),
            Stimulus(take=1, input_value=50233, expected_ready=1, expected_valid=1, expected_value=28793),
            Stimulus(valid=1, take=1, input_value=16420, expected_ready=1, expected_valid=1, expected_value=52324),
            Stimulus(valid=1, input_value=15375, expected_ready=1, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=47098, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=21477, expected_valid=1, expected_value=22607),
            Stimulus(input_value=57296, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=27579, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=59302, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=25489, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=61308, expected_valid=1, expected_value=22607),
            Stimulus(input_value=7015, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=38738, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=13117, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=48936, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=2835, expected_valid=1, expected_value=22607),
            Stimulus(input_value=34558, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=745, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, input_value=36564, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, take=1, expected_ready=1, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, take=1, input_value=65535, expected_ready=1, expected_valid=1, expected_value=16421),
            Stimulus(valid=1, take=1, input_value=65534, expected_ready=1, expected_valid=1, expected_value=15376),
            Stimulus(valid=1, take=1, input_value=32768, expected_ready=1, expected_valid=1, expected_value=1),
            Stimulus(valid=1, take=1, input_value=32767, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_value=43690, expected_ready=1, expected_valid=1, expected_value=65535),
            Stimulus(valid=1, take=1, input_value=21845, expected_ready=1, expected_valid=1, expected_value=32769),
            Stimulus(valid=1, take=1, input_value=1, expected_ready=1, expected_valid=1, expected_value=32768),
            Stimulus(valid=1, take=1, input_value=2, expected_ready=1, expected_valid=1, expected_value=43691),
            Stimulus(valid=1, take=1, input_value=4, expected_ready=1, expected_valid=1, expected_value=21846),
            Stimulus(valid=1, take=1, input_value=8, expected_ready=1, expected_valid=1, expected_value=2),
            Stimulus(valid=1, take=1, input_value=16, expected_ready=1, expected_valid=1, expected_value=3),
            Stimulus(valid=1, take=1, input_value=32, expected_ready=1, expected_valid=1, expected_value=5),
            Stimulus(valid=1, take=1, input_value=64, expected_ready=1, expected_valid=1, expected_value=9),
            Stimulus(valid=1, take=1, input_value=128, expected_ready=1, expected_valid=1, expected_value=17),
            Stimulus(valid=1, take=1, input_value=256, expected_ready=1, expected_valid=1, expected_value=33),
            Stimulus(valid=1, take=1, input_value=512, expected_ready=1, expected_valid=1, expected_value=65),
            Stimulus(valid=1, take=1, input_value=1024, expected_ready=1, expected_valid=1, expected_value=129),
            Stimulus(valid=1, take=1, input_value=2048, expected_ready=1, expected_valid=1, expected_value=257),
            Stimulus(valid=1, take=1, input_value=4096, expected_ready=1, expected_valid=1, expected_value=513),
            Stimulus(valid=1, take=1, input_value=8192, expected_ready=1, expected_valid=1, expected_value=1025),
            Stimulus(valid=1, take=1, input_value=16384, expected_ready=1, expected_valid=1, expected_value=2049),
            Stimulus(valid=1, take=1, input_value=32768, expected_ready=1, expected_valid=1, expected_value=4097),
            Stimulus(valid=1, take=1, input_value=1, expected_ready=1, expected_valid=1, expected_value=8193),
            Stimulus(valid=1, take=1, input_value=3, expected_ready=1, expected_valid=1, expected_value=16385),
            Stimulus(valid=1, take=1, input_value=7, expected_ready=1, expected_valid=1, expected_value=32769),
            Stimulus(valid=1, take=1, input_value=15, expected_ready=1, expected_valid=1, expected_value=2),
            Stimulus(valid=1, take=1, input_value=31, expected_ready=1, expected_valid=1, expected_value=4),
            Stimulus(valid=1, take=1, input_value=63, expected_ready=1, expected_valid=1, expected_value=8),
            Stimulus(valid=1, take=1, input_value=127, expected_ready=1, expected_valid=1, expected_value=16),
            Stimulus(valid=1, take=1, input_value=255, expected_ready=1, expected_valid=1, expected_value=32),
            Stimulus(valid=1, take=1, input_value=511, expected_ready=1, expected_valid=1, expected_value=64),
            Stimulus(valid=1, take=1, input_value=1023, expected_ready=1, expected_valid=1, expected_value=128),
            Stimulus(valid=1, take=1, input_value=2047, expected_ready=1, expected_valid=1, expected_value=256),
            Stimulus(valid=1, take=1, input_value=4095, expected_ready=1, expected_valid=1, expected_value=512),
            Stimulus(valid=1, take=1, input_value=8191, expected_ready=1, expected_valid=1, expected_value=1024),
            Stimulus(valid=1, take=1, input_value=16383, expected_ready=1, expected_valid=1, expected_value=2048),
            Stimulus(valid=1, take=1, input_value=32767, expected_ready=1, expected_valid=1, expected_value=4096),
            Stimulus(valid=1, take=1, input_value=65535, expected_ready=1, expected_valid=1, expected_value=8192),
            Stimulus(valid=1, take=1, input_value=8396, expected_ready=1, expected_valid=1, expected_value=16384),
            Stimulus(valid=1, take=1, input_value=56503, expected_ready=1, expected_valid=1, expected_value=32768),
            Stimulus(valid=1, take=1, input_value=26786, expected_ready=1, expected_valid=1),
            Stimulus(valid=1, take=1, input_value=62605, expected_ready=1, expected_valid=1, expected_value=8397),
            Stimulus(valid=1, take=1, input_value=28792, expected_ready=1, expected_valid=1, expected_value=56504),
            Stimulus(valid=1, take=1, input_value=52323, expected_ready=1, expected_valid=1, expected_value=26787),
            Stimulus(valid=1, take=1, input_value=22606, expected_ready=1, expected_valid=1, expected_value=62606),
            Stimulus(valid=1, take=1, input_value=50233, expected_ready=1, expected_valid=1, expected_value=28793),
            Stimulus(valid=1, take=1, input_value=16420, expected_ready=1, expected_valid=1, expected_value=52324),
            Stimulus(valid=1, take=1, input_value=15375, expected_ready=1, expected_valid=1, expected_value=22607),
            Stimulus(valid=1, take=1, input_value=47098, expected_ready=1, expected_valid=1, expected_value=50234),
            Stimulus(valid=1, take=1, input_value=21477, expected_ready=1, expected_valid=1, expected_value=16421),
            Stimulus(valid=1, take=1, input_value=57296, expected_ready=1, expected_valid=1, expected_value=15376),
            Stimulus(valid=1, take=1, input_value=27579, expected_ready=1, expected_valid=1, expected_value=47099),
            Stimulus(valid=1, take=1, input_value=59302, expected_ready=1, expected_valid=1, expected_value=21478),
            Stimulus(valid=1, take=1, input_value=25489, expected_ready=1, expected_valid=1, expected_value=57297),
            Stimulus(valid=1, take=1, input_value=61308, expected_ready=1, expected_valid=1, expected_value=27580),
            Stimulus(valid=1, take=1, input_value=7015, expected_ready=1, expected_valid=1, expected_value=59303),
            Stimulus(valid=1, take=1, input_value=38738, expected_ready=1, expected_valid=1, expected_value=25490),
            Stimulus(valid=1, take=1, input_value=13117, expected_ready=1, expected_valid=1, expected_value=61309),
            Stimulus(valid=1, take=1, input_value=48936, expected_ready=1, expected_valid=1, expected_value=7016),
            Stimulus(valid=1, take=1, input_value=2835, expected_ready=1, expected_valid=1, expected_value=38739),
            Stimulus(valid=1, take=1, input_value=34558, expected_ready=1, expected_valid=1, expected_value=13118),
            Stimulus(valid=1, take=1, input_value=745, expected_ready=1, expected_valid=1, expected_value=48937),
            Stimulus(valid=1, take=1, input_value=36564, expected_ready=1, expected_valid=1, expected_value=2836),
            Stimulus(valid=1, take=1, input_value=64191, expected_ready=1, expected_valid=1, expected_value=34559),
            Stimulus(valid=1, take=1, input_value=30378, expected_ready=1, expected_valid=1, expected_value=746),
            Stimulus(valid=1, take=1, input_value=4757, expected_ready=1, expected_valid=1, expected_value=36565),
            Stimulus(valid=1, take=1, input_value=40576, expected_ready=1, expected_valid=1, expected_value=64192),
            Stimulus(valid=1, take=1, input_value=10859, expected_ready=1, expected_valid=1, expected_value=30379),
            Stimulus(valid=1, take=1, input_value=42582, expected_ready=1, expected_valid=1, expected_value=4758),
            Stimulus(valid=1, take=1, input_value=8769, expected_ready=1, expected_valid=1, expected_value=40577),
            Stimulus(valid=1, take=1, input_value=44588, expected_ready=1, expected_valid=1, expected_value=10860),
            Stimulus(valid=1, take=1, input_value=23063, expected_ready=1, expected_valid=1, expected_value=42583),
            Stimulus(valid=1, take=1, input_value=54786, expected_ready=1, expected_valid=1, expected_value=8770),
            Stimulus(valid=1, take=1, input_value=29165, expected_ready=1, expected_valid=1, expected_value=44589),
            Stimulus(valid=1, take=1, input_value=64984, expected_ready=1, expected_valid=1, expected_value=23064),
            Stimulus(valid=1, take=1, input_value=18883, expected_ready=1, expected_valid=1, expected_value=54787),
            Stimulus(valid=1, take=1, input_value=50606, expected_ready=1, expected_valid=1, expected_value=29166),
            Stimulus(valid=1, take=1, input_value=16793, expected_ready=1, expected_valid=1, expected_value=64985),
            Stimulus(valid=1, take=1, input_value=52612, expected_ready=1, expected_valid=1, expected_value=18884),
            Stimulus(valid=1, take=1, input_value=47471, expected_ready=1, expected_valid=1, expected_value=50607),
            Stimulus(valid=1, take=1, input_value=13658, expected_ready=1, expected_valid=1, expected_value=16794),
            Stimulus(valid=1, take=1, input_value=53573, expected_ready=1, expected_valid=1, expected_value=52613),
            Stimulus(valid=1, take=1, input_value=23856, expected_ready=1, expected_valid=1, expected_value=47472),
            Stimulus(valid=1, take=1, input_value=59675, expected_ready=1, expected_valid=1, expected_value=13659),
            Stimulus(valid=1, take=1, input_value=25862, expected_ready=1, expected_valid=1, expected_value=53574),
            Stimulus(valid=1, take=1, input_value=57585, expected_ready=1, expected_valid=1, expected_value=23857),
            Stimulus(valid=1, take=1, input_value=27868, expected_ready=1, expected_valid=1, expected_value=59676),
            Stimulus(valid=1, take=1, input_value=39111, expected_ready=1, expected_valid=1, expected_value=25863),
            Stimulus(valid=1, take=1, input_value=5298, expected_ready=1, expected_valid=1, expected_value=57586),
            Stimulus(valid=1, take=1, input_value=45213, expected_ready=1, expected_valid=1, expected_value=27869),
            Stimulus(valid=1, take=1, input_value=15496, expected_ready=1, expected_valid=1, expected_value=39112),
            Stimulus(valid=1, take=1, input_value=34931, expected_ready=1, expected_valid=1, expected_value=5299),
            Stimulus(valid=1, take=1, input_value=1118, expected_ready=1, expected_valid=1, expected_value=45214),
            Stimulus(valid=1, take=1, input_value=32841, expected_ready=1, expected_valid=1, expected_value=15497),
            Stimulus(valid=1, take=1, input_value=3124, expected_ready=1, expected_valid=1, expected_value=34932),
            Stimulus(take=1, input_value=30751, expected_ready=1, expected_valid=1, expected_value=1119),
            Stimulus(take=1, input_value=62474, expected_ready=1, expected_valid=1, expected_value=32842),
            Stimulus(take=1, input_value=28661, expected_ready=1, expected_valid=1, expected_value=3125),
            Stimulus(take=1, input_value=7136, expected_ready=1),
            Stimulus(take=1, input_value=42955, expected_ready=1),
            Stimulus(take=1, input_value=9142, expected_ready=1),
            Stimulus(take=1, input_value=49057, expected_ready=1),
            Stimulus(take=1, input_value=11148, expected_ready=1),
            Stimulus(take=1, input_value=55159, expected_ready=1),
            Stimulus(take=1, input_value=21346, expected_ready=1),
            Stimulus(take=1, input_value=53069, expected_ready=1),
            Stimulus(take=1, input_value=31544, expected_ready=1),
            Stimulus(take=1, input_value=50979, expected_ready=1),
            Stimulus(take=1, input_value=17166, expected_ready=1),
            Stimulus(take=1, input_value=57081, expected_ready=1),
            Stimulus(take=1, input_value=19172, expected_ready=1),
            Stimulus(take=1, input_value=14031, expected_ready=1),
            Stimulus(take=1, input_value=45754, expected_ready=1),
            Stimulus(take=1, input_value=11941, expected_ready=1),
            Stimulus(take=1, input_value=55952, expected_ready=1),
            Stimulus(take=1, input_value=26235, expected_ready=1),
            Stimulus(take=1, input_value=57958, expected_ready=1),
            Stimulus(take=1, input_value=32337, expected_ready=1),
            Stimulus(take=1, input_value=59964, expected_ready=1),
            Stimulus(),
        ),
    )
    # fmt: on
    position = phase if phase < 184 else 184
    return stimuli.rows[position]


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

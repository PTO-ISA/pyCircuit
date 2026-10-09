"""Historical first case, then an exhaustive byte decode oracle."""

from example_decode_rules.decode_rules import DecodeRules
from pycircuit import bits, log, rule, system


@rule
def advance(phase):
    phase = phase + 1


@system
def ExerciseDecodeRules():  # noqa: N802
    phase: bits[16] = 0
    insn = 16 if phase == 0 else (phase - 1)[:8]
    dut = DecodeRules(insn)

    expected_op = (
        (
            ((insn[:4] & 0) | 1)
            if insn < 32
            else (((insn[:4] & 0) | 2) if insn < 48 else ((insn[:4] & 0) | 3))
        )
        if (insn >= 16) & (insn < 64)
        else (insn[:4] & 0)
    )
    expected_len = 4 if (insn >= 16) & (insn < 64) else (insn[:3] & 0)

    @rule
    def check_and_advance():
        if phase < 257:
            # Three disjoint numerical intervals define the independent oracle.
            assert dut.op == expected_op, "decode_rules: opcode"
            assert dut.len == expected_len, "decode_rules: instruction length"

        log("info", "decode_rules.op", dut.op)
        log("info", "decode_rules.len", dut.len)

    check_and_advance()
    advance(phase)

# ruff: noqa: F821, F823, F841, N802 -- compiler-owned hardware state and System names.
"""Dense negative FP32 exponential sweep; host math supplies the oracle."""
import pycircuit as ac
from fa2.fa2_numeric import fp_exp_neg


@ac.rule
def advance(phase):
    phase = phase + 1

@ac.module
def extend(phase: ac.u12) -> ac.u32:
    return phase

@ac.system
def ExpSweep():
    phase: ac.u12 = 0
    index = extend(phase)
    argument = 3212836864 + (index << 13)
    result = fp_exp_neg(argument)
    @ac.rule
    def observe():
        ac.log("info", "exp_sweep", argument, result)
    observe()
    advance(phase)

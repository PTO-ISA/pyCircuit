# Captured hardware source: Rules update Module-owned state.
# ruff: noqa: N802, F823, F841
import pycircuit as ac
from checks.counters import CallableCounter


@ac.module
def Caller(counter: CallableCounter):
    @ac.rule
    def advance():
        counter.increment()


@ac.system(samples=[{"clk_fast": 0, "clk_slow": 0, "rst_n": 1}])
def Bench(rst_n: ac.u1):
    fast = ac.domain(clock="clk_fast")
    slow = ac.domain(clock="clk_slow")
    counter = CallableCounter(rst_n, domain=fast)
    Caller(counter, domain=slow)

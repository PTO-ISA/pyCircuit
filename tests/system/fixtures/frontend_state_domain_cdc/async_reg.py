# Captured hardware source: Rules update Module-owned state.
# ruff: noqa: N802, F823, F841
import pycircuit as ac


@ac.module
def Sender(reset: ac.u1) -> ac.u1:
    count: ac.u8 = ac.state(init=0, reset=reset, reset_kind="async")

    @ac.rule
    def advance():
        count = count + 1

    return count < 16


@ac.module
def Receiver(level: ac.u1) -> None:
    @ac.rule
    def observe():
        ac.log("info", "synchronized level", level)


@ac.system(samples=[])
def Bench(reset: ac.u1):
    fast = ac.domain(clock="clk_fast")
    slow = ac.domain(clock="clk_slow")
    synchronized = ac.AsyncReg[ac.u1]()
    synchronized = Sender(reset, domain=fast)
    alias = synchronized
    Receiver(synchronized, domain=slow)
    Receiver(~alias, domain=slow)

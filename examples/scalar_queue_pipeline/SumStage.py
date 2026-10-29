# ruff: noqa: F823 -- Module state is compiler-owned.
import pycircuit as ac


@ac.module
def SumStage(source: ac.bits[8], accumulator: ac.module) -> ac.bits[8]:  # noqa: N802
    accepted: ac.bits[8] = 0

    @ac.rule
    def accumulate(value: ac.bits[8]) -> ac.bits[8]:
        result = accumulator.add(value)
        accepted = accepted + 1
        ac.log("info", "sum", value, result)
        return result

    result = accumulate(source)

    @ac.autorule
    def observe() -> None:
        ac.log("info", "accepted", accepted)

    return result

# ruff: noqa: F823, F841 -- hardware owner writes.
import pycircuit as ac
from pycircuit import method, module


@module
def Counter():
    count: ac.u8 = 5

    @method
    def advance(delta: ac.u8) -> ac.u8:
        proposed = count + delta
        count = proposed
        assert proposed < 16, "counter limit"
        return proposed

    @method(guard=lambda allow: allow)
    def guarded(allow: ac.u1) -> ac.u8:
        return count

    @method
    def read() -> ac.u8:
        return count

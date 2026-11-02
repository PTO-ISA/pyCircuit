import pycircuit as ac
from pycircuit import module, autorule

@ac.struct
class Result:
    value: ac.u1

@module
def Checked(ok: bool) -> Result:
    @autorule
    def inspect_first():
        assert ok, "provider"
        assert True
    @autorule
    def inspect_second():
        assert ok, "provider"
        assert True
    return Result(value=ok)

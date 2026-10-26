import pycircuit as ac
from pycircuit import module, rule

@ac.struct
class Result:
    value: ac.u1

@module
def Checked(ok: bool) -> Result:
    @rule
    def inspect_first():
        assert ok, "provider"
        assert True
    @rule
    def inspect_second():
        assert ok, "provider"
        assert True
    return Result(value=ok)

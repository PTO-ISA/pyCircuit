from pycircuit import module, rule

@module
def Checked(ok: bool) -> {"value": bool}:
    @rule
    def inspect():
        assert ok, "provider"
        assert True
    inspect()
    inspect()
    return {"value": ok}

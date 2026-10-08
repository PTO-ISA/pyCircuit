from pycircuit import module, rule


@module
def Checked(ok: bool) -> {"value": bool}:
    @rule
    def inspect():
        assert ok, "local binding publication"

    inspect()
    return {"value": ok}

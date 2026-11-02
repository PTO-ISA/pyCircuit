from pycircuit import module, autorule


@module
def Checked(ok: bool) -> {"value": bool}:
    @autorule
    def inspect():
        assert ok, "local binding publication"

    return {"value": ok}

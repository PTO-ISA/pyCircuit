from pycircuit import module, autorule

@module
def Top(path: bool, condition: bool) -> {"value": bool}:
    @autorule
    def inspect():
        if path:
            assert condition, "matrix"
        else:
            pass
    return {"value": condition}

from pycircuit import log, module, autorule

@module
def Top(ok: bool) -> {"value": bool}:
    @autorule
    def inspect():
        log("info", "observation", ok)
    return {"value": ok}

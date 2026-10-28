from pycircuit import log, module, rule

@module
def Top(ok: bool) -> {"value": bool}:
    @rule
    def inspect():
        log("info", "observation", ok)
    return {"value": ok}

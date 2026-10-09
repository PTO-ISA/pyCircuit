from pycircuit import module

@module
def Top(ok: bool) -> {"value": bool}:
    return {"value": ok}

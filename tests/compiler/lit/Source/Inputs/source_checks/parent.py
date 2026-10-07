from pycircuit import module, rule
from checks.provider import Checked

@module
def Top(ok: bool) -> {"value": bool}:
    used = Checked()
    unused = Checked()
    @rule
    def bind_used():
        used(ok=ok)
    @rule
    def bind_unused():
        unused(ok=ok)
    bind_used()
    bind_unused()
    return {"value": used.value}

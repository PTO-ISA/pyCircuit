from pycircuit import module
from checks.provider import Checked, Result

@module
def Top(ok: bool) -> Result:
    used = Checked(ok)
    _unused = Checked(ok)
    return used

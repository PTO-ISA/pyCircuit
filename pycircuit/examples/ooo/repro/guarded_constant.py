from pycircuit import ac

@ac.signal
def natural(enabled, values, wanted) -> ac.u32:
    selected = ac.u32(0)
    for i in range(2):
        if enabled[i].value and values[i].value == wanted:
            selected = i + 1
    return selected

@ac.signal
def hoisted(enabled, values, wanted) -> ac.u32:
    lane = ac.u32(wanted)
    selected = ac.u32(0)
    for i in range(2):
        if enabled[i].value and ac.u32(values[i].value) == lane:
            selected = i + 1
    return selected

@ac.module
def Probe():
    enabled = [ac.queue[bool](initial=v) for v in [False, True]]
    values = [ac.queue[bool](initial=v) for v in [False, True]]
    original = natural(enabled, values, True)
    workaround = hoisted(enabled, values, True)

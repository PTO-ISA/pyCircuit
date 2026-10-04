from pycircuit import ac

@ac.module
def Probe():
    wanted = True
    @ac.signal
    def natural(enabled, values) -> ac.u32:
        selected = ac.u32(0)
        for i in range(2):
            if enabled[i].value and values[i].value == wanted:
                selected = i + 1
        return selected

    @ac.signal
    def hoisted(enabled, values) -> ac.u32:
        lane = ac.u32(wanted)
        selected = ac.u32(0)
        for i in range(2):
            if enabled[i].value and ac.u32(values[i].value) == lane:
                selected = i + 1
        return selected

    enabled = [ac.queue[bool](initial=v) for v in [False, True]]
    values = [ac.queue[bool](initial=v) for v in [False, True]]
    original = natural(enabled, values)
    workaround = hoisted(enabled, values)

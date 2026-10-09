from pycircuit import bits, module, rule


@rule
def accumulate(value, enable):
    if enable:
        value = value + 1


@module
def Accumulator(enable: bits[1]) -> {"value": bits[8]}:
    value: bits[8] = 0
    accumulate(value, enable)
    return {"value": value}

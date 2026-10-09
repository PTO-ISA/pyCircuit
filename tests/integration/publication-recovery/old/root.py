from pycircuit import bits, rule, system


@rule
def tick(value):
    value = (value + 1) & 255


@system
def Root():  # noqa: N802
    value: bits[8] = 3
    tick(value)

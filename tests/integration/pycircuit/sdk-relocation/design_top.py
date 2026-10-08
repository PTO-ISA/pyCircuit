from measurement_relocated.counter import Counter
from pycircuit import bits, rule, system


@rule
def transfer_output(outgoing, count):
    outgoing = count  # noqa: F841


@system
def DesignTop():  # noqa: N802
    incoming: bits[8] = 7
    outgoing: bits[8] = 99
    counter = Counter(incoming, outgoing)
    transfer_output(outgoing, counter.count)

from pycircuit import bits, struct


@struct
class CounterResult:
    count: bits[8]

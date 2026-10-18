
from pycircuit import bits, module, struct


@struct
class WordResult:
    q: bits[8]


@module
def Word(a: bits[8]) -> WordResult:
    return WordResult(q=a)

from pycircuit import module
from .facade import ExportedPair
from .provider import Word

class LocalPair:
    first: bool
    second: Word

    def __init__(self, second: Word = 4, first: bool = True):
        self.first = first
        self.second = second

@module
def Top():
    scalar: Word = 11

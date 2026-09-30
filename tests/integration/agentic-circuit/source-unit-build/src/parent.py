from pycircuit import module
from .child import Child
from .types import Word


@module
def Parent():
    source: Word = 3
    sink: Word = 0
    child = Child(source, sink)

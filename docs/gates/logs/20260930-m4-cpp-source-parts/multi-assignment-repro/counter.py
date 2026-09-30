from pycircuit import module, rule
from .types import Word

@module
def Counter(incoming: Word, outgoing: Word):
    count: Word = 0
    @rule
    def tick():
        nonlocal count, outgoing
        count = incoming
        outgoing = count
    tick()

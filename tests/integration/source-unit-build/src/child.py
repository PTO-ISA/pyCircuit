from pycircuit import module, rule

from .types import Word


@module
def Child(source: Word, sink: Word):
    @rule
    def forward():
        nonlocal sink
        sink = source

    forward()

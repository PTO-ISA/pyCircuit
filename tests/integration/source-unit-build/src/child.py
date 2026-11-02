from pycircuit import autorule, module

from .types import Word


@module
def Child(source: Word, sink: Word):
    @autorule
    def forward():
        nonlocal sink
        sink = source

    forward()

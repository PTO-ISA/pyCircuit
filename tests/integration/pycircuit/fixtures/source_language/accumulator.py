"""Approved source fixture; compiler input for the active MLIR route."""

from pycircuit import module, rule

from .packet import Request, Word


@module
class Accumulator:
    def __init__(self, request: Request, result: Word):
        self.request = request
        self.result = result
        self.total: Word = 0
        self.result = self.accumulate(self.request)

    @rule
    def accumulate(self, item: Request) -> Word:
        total = self.total
        if item.valid:
            total = (total + item.value) & 255
            self.total = total
        return total

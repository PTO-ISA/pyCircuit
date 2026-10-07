"""Two independent accumulators with current/next and nonzero root reset."""

from pycircuit import module, rule

from .accumulator import Accumulator
from .packet import Request, Word


@module
class Core:
    def __init__(self):
        self.left_request: Request = Request(1, True)
        self.right_request: Request = Request(2, True)
        self.left_result: Word = 7
        self.right_result: Word = 19
        self.left = Accumulator(self.left_request, self.left_result)
        self.right = Accumulator(self.right_request, self.right_result)
        self.advance()

    @rule
    def advance(self):
        self.left_request = Request(1, not self.left_request.valid)

"""One source definition instantiated with heterogeneous static arguments."""

from pycircuit import module

from .bank import Bank, Word


@module
class BankCore:
    def __init__(self):
        self.input_a: Word = 9
        self.input_b: Word = 13
        self.output_a: Word = 0
        self.output_b: Word = 0
        self.small = Bank(self.input_a, self.output_a, entries=2)
        self.large = Bank(self.input_b, self.output_b, entries=4, initial=5)

"""Approved ordinary static shape/reset parameter fixture."""

from typing import Annotated

from pycircuit import module, rule

Word = Annotated[int, range(256)]


@module
class Bank:
    def __init__(self, incoming: Word, outgoing: Word, entries: int, initial: int = 0):
        self.incoming = incoming
        self.outgoing = outgoing
        self.cells: list[Word] = [initial for _ in range(entries)]
        self.outgoing = self.exchange(self.incoming)

    @rule
    def exchange(self, value: Word) -> Word:
        previous = self.cells[0]
        self.cells[0] = value
        return previous

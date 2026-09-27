"""Approved C1 record source; compiler input, never eagerly imported by tests."""

from typing import Annotated

Word = Annotated[int, range(256)]


class Request:
    value: Word
    valid: bool

    def __init__(self, value: Word = 0, valid: bool = False):
        self.value = value
        self.valid = valid

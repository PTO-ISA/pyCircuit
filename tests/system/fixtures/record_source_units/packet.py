"""Approved source record forms for real source-unit/header tests."""

from typing import Annotated

Word = Annotated[int, range(256)]


class Request:
    value: Word
    valid: bool

    def __init__(self, valid: bool = False, value: Word = 0):
        self.value = value
        self.valid = valid


class Pair:
    left: Word
    right: Word

    def __init__(self, right: Word = 2, left: Word = 1):
        self.left = left
        self.right = right


class Required:
    value: Word

    def __init__(self, value: Word):
        self.value = value


class Modes:
    positional: Word
    keyword: Word

    def __init__(self, positional: Word, /, *, keyword: Word):
        self.positional = positional
        self.keyword = keyword

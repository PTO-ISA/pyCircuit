"""Header-only consumer for Request defaults, kwargs, and field reads."""

from .packet import Pair, Request, Word


def default_request() -> Request:
    return Request()


def keyword_request() -> Request:
    return Request(valid=True, value=7)


def read_value() -> Word:
    return Request(valid=True, value=9).value


def default_pair() -> Pair:
    return Pair()


def keyword_pair() -> Pair:
    return Pair(right=20, left=10)


def read_left() -> Word:
    return Pair(right=20, left=10).left

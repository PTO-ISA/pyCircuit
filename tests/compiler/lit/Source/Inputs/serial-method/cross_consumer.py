from pycircuit import module, rule
from serial_methods.cross_provider import Counter
from serial_methods.cross_relay import Relay


@module
def Top():
    left = Relay()
    right = Relay()
    idle = Counter()  # noqa: F841 -- real instance without an external writer.

    @rule
    def advance_left() -> None:
        left.advance()

    @rule
    def advance_right() -> None:
        right.advance()

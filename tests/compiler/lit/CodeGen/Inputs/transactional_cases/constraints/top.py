# ruff: noqa: F821, F823, F841, F842, N802 -- Compiler-owned instances, state and type constraints.
from enum import Enum

import pycircuit as ac
from constraints.ignored_first import IgnoredFirst
from constraints.provider import Provider
from constraints.typed_first import TypedFirst


@ac.encoding(width=3)
class Mode(Enum):
    IDLE = 0
    ACTIVE = 5


@ac.system
def Top():
    first: Provider
    second: Provider
    ignored: IgnoredFirst
    typed: TypedFirst
    ignored(first)
    typed(second)
    left: ac.u8 = 250
    right: ac.u8 = 10
    captured_a = 254
    captured_b = captured_a + 1

    @ac.method
    def wrap(a: ac.u8, b: ac.u8) -> ac.u16:
        return a + b

    @ac.method
    def shift(a: ac.u8, amount: ac.u16) -> ac.u16:
        return a << amount

    @ac.method
    def binding_wrap() -> ac.u16:
        a = 255
        narrow: ac.u8 = a
        b = a + 1
        return b

    @ac.method
    def sibling_wrap() -> ac.u16:
        a = 255
        unused = a + ac.u8(0)
        b = a + 1
        return b

    @ac.method
    def capture_wrap() -> ac.u16:
        c: ac.u16 = captured_b + 1
        t0 = -captured_a
        t1 = -t0
        t2 = -t1
        t3 = -t2
        t4 = -t3
        t5 = -t4
        t6 = -t5
        t7 = -t6
        t8 = -t7
        t9 = -t8
        narrow: ac.u8 = t9
        return c

    @ac.autorule
    def observe() -> None:
        wide = wrap(left, right)
        bound = binding_wrap()
        sibling = sibling_wrap()
        captured = capture_wrap()
        ac.log("info", "wide", wide, bound, sibling, captured)
        shifted = shift(ac.u8(129), ac.u16(1))
        exhausted = shift(ac.u8(129), ac.u16(256))
        rightward: ac.u16 = ac.u8(129) >> ac.u16(1)
        mode = Mode.ACTIVE
        matched = mode == Mode.ACTIVE
        different = mode == Mode.IDLE
        ac.log("info", "types", shifted, exhausted, rightward, matched, different)

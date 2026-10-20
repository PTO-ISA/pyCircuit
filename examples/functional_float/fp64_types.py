"""Binary64 numerical payload and effects for source-unit composition."""
import pycircuit as ac


@ac.struct
class FloatWordResponse:
    result: ac.u64
    valid: ac.u1
    invalid: ac.u1
    overflow: ac.u1
    underflow: ac.u1
    inexact: ac.u1
    zero: ac.u1
    divide_by_zero: ac.u1


@ac.struct
class FP64Parts:
    sign: ac.u1
    exponent: ac.u11
    mantissa: ac.u53
    zero: ac.u1
    inf: ac.u1
    nan: ac.u1
    snan: ac.u1

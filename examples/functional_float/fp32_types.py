"""Nominal response carrier shared by the FP32 source units."""

import pycircuit as ac


@ac.struct
class FP32Response:
    result: ac.u32
    valid: ac.u1
    invalid: ac.u1
    overflow: ac.u1
    underflow: ac.u1
    inexact: ac.u1
    zero: ac.u1

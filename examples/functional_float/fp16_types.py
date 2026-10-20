"""Nominal response carrier shared by binary16 source units."""

import pycircuit as ac


@ac.struct
class FP16Response:
    result: ac.u16
    valid: ac.u1
    invalid: ac.u1
    overflow: ac.u1
    underflow: ac.u1
    inexact: ac.u1
    zero: ac.u1

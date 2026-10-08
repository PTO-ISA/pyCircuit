"""Closed reductions and zip maps; the test owner renders four literal extents."""

import pycircuit as ac


@ac.struct
class Pair:
    left: ac.bits[9]
    right: ac.bits[3]


@ac.struct
class Result:
    add: ac.bits[9]
    mul: ac.bits[9]
    band: ac.bits[9]
    bor: ac.bits[9]
    bxor: ac.bits[9]
    minimum: ac.bits[9]
    maximum: ac.bits[9]
    complete: ac.u1
    present: ac.u1
    count: ac.bits[2]  # count-width
    identity: ac.table[3, ac.bits[9]]
    zipped: ac.table[3, Pair]
    projected: ac.table[3, ac.bits[9]]
    captured: ac.table[3, ac.bits[9]]
    shadowed: ac.table[3, ac.bits[9]]


@ac.rule
def evaluate(left, right, flags, bias, replacement) -> Result:
    saved = bias
    captured = left.map(lambda lane: lane ^ saved)
    saved = replacement
    zipped = left.map(lambda lane, tag: Pair(left=lane, right=tag), right)
    return Result(
        add=left.fold(kind="add"),
        mul=left.fold(kind="mul"),
        band=left.fold(kind="and"),
        bor=left.fold(kind="or"),
        bxor=left.fold(kind="xor"),
        minimum=left.fold(kind="min"),
        maximum=left.fold(kind="max"),
        complete=flags.all(),
        present=flags.any(),
        count=flags.count(),
        identity=left.map(lambda lane: lane),
        zipped=zipped,
        projected=zipped.map(lambda pair: pair.left),
        captured=captured,
        shadowed=left.map(lambda bias: bias),
    )


@ac.module
def Top(
    left: ac.table[3, ac.bits[9]],
    right: ac.table[3, ac.bits[3]],
    flags: ac.table[3, ac.u1],
    bias: ac.bits[9],
    replacement: ac.bits[9],
) -> Result:
    return evaluate(left, right, flags, bias, replacement)

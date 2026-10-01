"""Independent pipeline transactions; does not call model Work/Rule bodies."""

from examples.reference_common import Transactions, evaluate_common
from .model import Compute


def transformed(value, count):
    # Exponentiate the affine LCG, instead of executing the component's loop.
    multiplier, offset, a, b = 1, 0, 1664525, 1013904223
    while count:
        if count & 1:
            multiplier, offset = multiplier * a & 0xffffffff, (offset * a + b) & 0xffffffff
        a, b = a * a & 0xffffffff, (a * b + b) & 0xffffffff
        count >>= 1
    return (value * multiplier + offset + 1) & 0xffffffff


def evaluate(e, m):
    if evaluate_common(e, m):
        return
    operations = Transactions(e, m)
    read, take, transaction = operations.read, operations.take, operations.transaction
    if isinstance(m, Compute):
        if m.control is not None:
            read(m.control)
        def compute(rid):
            seq, value = take(m.source, rid)
            m.output.propose_push(rid, (seq, transformed(value, m.iterations)))
        transaction(m.rid, compute)
    else:
        raise TypeError(f"no independent behavior model for {type(m).__name__}")

"""Independent lookup transactions; does not call model Work/Rule bodies."""

from examples.reference_common import Transactions, evaluate_common
from .model import Lookup


def evaluate(e, m):
    if evaluate_common(e, m):
        return
    operations = Transactions(e, m)
    read, take, transaction = operations.read, operations.take, operations.transaction
    if isinstance(m, Lookup):
        index = read(m.selector)
        def lookup(rid):
            seq, value = read(m.source)
            coefficient = read(m.table[index])
            take(m.source, rid)
            m.output.propose_push(rid, (seq, value * coefficient % 65536))
        transaction(m.rid, lookup)
    else:
        raise TypeError(f"no independent behavior model for {type(m).__name__}")

"""Independent pairs transactions; does not call model Work/Rule bodies."""

from examples.reference_common import Transactions, evaluate_common
from .model import PairALU


def evaluate(e, m):
    if evaluate_common(e, m):
        return
    operations = Transactions(e, m)
    read, take, transaction = operations.read, operations.take, operations.transaction
    if isinstance(m, PairALU):
        def pair(rid):
            seq, a = take(m.inputs[0], rid)
            m.outputs[0].propose_push(rid, (seq, a))
            e.request_wakeup(rid, m.mid, 3)
            other, b = take(m.inputs[1], rid)
            m.outputs[1].propose_push(rid, (other, (a + b) % 65536))
        transaction(m.rid, pair)
    else:
        raise TypeError(f"no independent behavior model for {type(m).__name__}")

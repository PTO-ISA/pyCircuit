"""Independent feedback transactions; does not call model Work/Rule bodies."""

from examples.reference_common import Transactions, evaluate_common
from .model import Feedback


def evaluate(e, m):
    if evaluate_common(e, m):
        return
    operations = Transactions(e, m)
    read, take, transaction = operations.read, operations.take, operations.transaction
    if isinstance(m, Feedback):
        def hop(rid):
            seq, hops = take(m.source, rid)
            target = m.target if hops else m.output
            target.propose_push(rid, (seq, max(0, hops - 1)))
        transaction(m.rid, hop)
    else:
        raise TypeError(f"no independent behavior model for {type(m).__name__}")

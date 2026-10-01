"""Independent retry transactions; does not call model Work/Rule bodies."""

from examples.reference_common import Transactions, evaluate_common
from .model import Retry


def evaluate(e, m):
    if evaluate_common(e, m):
        return
    operations = Transactions(e, m)
    read, take, transaction = operations.read, operations.take, operations.transaction
    if isinstance(m, Retry):
        def retry(rid):
            value, remaining = read(m.token), read(m.budget)
            m.token.propose_revise(rid, value)
            take(m.token, rid)
            if remaining:
                m.budget.propose_revise(rid, remaining - 1)
                m.token.propose_push(rid, value)
            else:
                m.output.propose_push(rid, value)
        transaction(m.rid, retry)
    else:
        raise TypeError(f"no independent behavior model for {type(m).__name__}")

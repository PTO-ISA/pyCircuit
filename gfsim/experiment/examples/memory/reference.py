"""Independent memory transactions; does not call model Work/Rule bodies."""

from examples.reference_common import Transactions, evaluate_common
from .model import Dispatch, Bank


def evaluate(e, m):
    if evaluate_common(e, m):
        return
    operations = Transactions(e, m)
    read, take, transaction = operations.read, operations.take, operations.transaction
    if isinstance(m, Dispatch):
        def dispatch(rid):
            request = take(m.source, rid)
            m.outputs[request[1] % len(m.outputs)].propose_push(rid, request)
        transaction(m.rid, dispatch)
    elif isinstance(m, Bank):
        e.record_read(m.mid, m.busy.qid)
        if not m.busy.current:
            def service(rid):
                seq, address, write, data = read(m.requests)
                cell = m.cells[address // m.banks]
                (_, count), old = read(cell)
                take(m.requests, rid)
                if write:
                    cell.propose_revise(rid, data, (1,))
                    cell.propose_revise(rid, (True, count + 1), (0,))
                m.busy.propose_push(rid, (e.tick + m.latency, seq, data if write else old))
                e.request_wakeup(rid, m.mid, m.latency)
            transaction(m.service, service)
        else:
            transaction(m.audit, lambda rid: m.busy.propose_revise(rid, read(m.busy)[0], (0,)))
            def finish(rid):
                due, seq, data = read(m.busy)
                if due > e.tick:
                    e.request_wakeup(rid, m.mid, due - e.tick)
                else:
                    take(m.busy, rid)
                    m.output.propose_push(rid, (seq, data))
            transaction(m.finish, finish)
    else:
        raise TypeError(f"no independent behavior model for {type(m).__name__}")

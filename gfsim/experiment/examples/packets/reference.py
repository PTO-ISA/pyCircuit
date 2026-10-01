"""Independent packets transactions; does not call model Work/Rule bodies."""

from examples.reference_common import Transactions, evaluate_common
from .model import PacketLanes


def evaluate(e, m):
    if evaluate_common(e, m):
        return
    operations = Transactions(e, m)
    read, take, transaction = operations.read, operations.take, operations.transaction
    if isinstance(m, PacketLanes):
        mode, bias, _ = read(m.control)
        for lane in (0, 1):
            if mode == 1 - lane:
                continue
            def packet(rid):
                seq, value = take(m.inputs[lane], rid)
                if value % 7:
                    m.outputs[lane].propose_push(rid, (seq, (value + bias) % 65536))
            transaction((m.a, m.b)[lane], packet)
    else:
        raise TypeError(f"no independent behavior model for {type(m).__name__}")

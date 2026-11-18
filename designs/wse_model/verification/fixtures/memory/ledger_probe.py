# ruff: noqa: N802 -- hardware module names.
"""Two-node ledger and native memory transaction admission probe."""

import pycircuit as ac
from wse_calendar.ledger import ReceiveLedger, Request, Result


@ac.module
def LedgerMemoryProbe(node: ac.u6, request: Request, data: ac.u64) -> tuple[ac.u64, Result]:
    ledger = ReceiveLedger(
        NODES=2, EARLY_SLOTS=1, SEEN_SLOTS=2, TOTAL_EARLY=2, TOTAL_SEEN=4, ROW_BITS=44
    )
    memory = ac.byte_mem[ac.u64](depth=128)

    @ac.rule
    def access() -> tuple[ac.u64, Result]:
        address: ac.u7 = node
        address = address * 64
        before = memory.read(address)
        if request.kind == 2:
            decision = ledger.inspect(node, request.segment)
            if decision.status == 2:
                memory.write(address, data)
        result = ledger.process(node, request)
        return before, result

    word, observed = access
    return word, observed

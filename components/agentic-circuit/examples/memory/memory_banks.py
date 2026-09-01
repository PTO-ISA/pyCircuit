"""Statically expanded banked-memory example for contract epoch 0.4.

The companion harness injects five requests before tick 0:

    tag  operation
     1   bank0[3] = 41  (returns old data 0)
     2   bank1[3] = 91  (returns old data 0)
     3   read bank0[3]  (returns 41)
     4   read bank1[3]  (returns 91)
     5   read bank2[3]  (returns 0)

Current gfsim event pipeline (R=route, A=bank accept, P=bank response,
M=priority merge, S=sink):

    tick       0  1  2  3  4  5  6  7  8  9 10 11
    input t1   Q  R  A  .  P  M  S  .  .  .  .  .
    input t2   Q  .  R  A  .  P  M  S  .  .  .  .
    input t3   Q  .  .  R  W  A  .  P  M  S  .  .
    input t4   Q  .  .  .  R  W  A  .  P  M  S  .
    input t5   Q  .  .  .  .  R  A  .  P  W  M  S

Q commits the initially injected token, and W marks waiting caused by either
a busy bank or priority-merge serialization.  Notice that bank1 accepts tag 2
while bank0 is still processing tag 1, and bank1/bank2 accept tags 4/5 in the
same tick.  Requests to one bank remain single-outstanding.
"""

import agentic_circuit as ac


@ac.struct
class BankRequest:
    bank: ac.u2
    offset: ac.u4
    write: ac.u1
    data: ac.u16
    tag: ac.u8


@ac.system
def memory_banks() -> None:
    requests = ac.source(BankRequest, depth=8, latency=1)

    with ac.scope("sram"):
        banks = ac.array(
            4,
            lambda _: ac.memory(ac.u16, entries=16, init=0, latency=2),
        )
        selected = banks.select(
            requests,
            key=lambda request: request.bank,
            depth=2,
            latency=1,
        )
        responses = selected.request(
            address=lambda request: request.offset,
            write=lambda request: request.write,
            data=lambda request: request.data,
            result_field="data",
            depth=2,
            merge_policy="priority",
            merge_depth=2,
            merge_latency=1,
        )

    ac.sink(responses)

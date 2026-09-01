"""Single-memory busy/backpressure example for contract epoch 0.4.

The companion harness injects read A (tag 1) before tick 0 and read B (tag 2)
before tick 1.  Both access zero-initialized locations through one memory with
``latency=3``.

Current gfsim event pipeline (Q=request Queue commit, A=memory accept,
W=waiting while the memory is busy, P=response Queue commit/busy release,
S=sink):

    tick       0  1  2  3  4  5  6  7  8  9
    read A     Q  A  .  .  P  S  .  .  .  .
    read B     .  Q  W  W  W  A  .  .  P  S

Read B remains at the head of the request Queue while A is outstanding.  A's
response releases ``busy`` at tick 4, but the no-same-epoch-reaccept rule means
B can be accepted only at tick 5.  Both responses return old data 0.
"""

import agentic_circuit as ac


@ac.struct
class ReadRequest:
    address: ac.u4
    data: ac.u16
    tag: ac.u8


@ac.system
def memory_busy() -> None:
    sram = ac.memory(ac.u16, entries=16, init=0, latency=3)
    requests = ac.source(ReadRequest, depth=4, latency=1)
    responses = sram.request(
        requests,
        address=lambda request: request.address,
        write=lambda request: False,
        data=lambda request: request.data,
        result_field="data",
        depth=1,
    )
    ac.sink(responses)

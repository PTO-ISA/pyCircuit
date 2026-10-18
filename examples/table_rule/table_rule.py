"""Replace one persistent table entry atomically with a queue transfer."""

import pycircuit as ac


@ac.struct
class Entry:
    index: ac.u1
    value: ac.u7


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: Entry


@ac.module
def TableRule(valid: ac.u1, data: Entry, take: ac.u1) -> Result:  # noqa: N802
    entries = ac.table[2, Entry](init=0)
    ingress = ac.queue[Entry](
        depth=2,
        latency=1,
        ready_policy="downstream_pop",
        in_valid=valid,
        in_data=data,
    )
    egress = ac.queue[Entry](
        depth=1, latency=1, ready_policy="downstream_pop", out_ready=take
    )

    @ac.rule
    def install(entry: Entry) -> Entry:
        old = entries[entry.index]
        entries[entry.index] = entry
        return old

    @ac.rule
    def transfer() -> None:
        incoming = ingress.pop()
        old = install(incoming)
        egress.push(old)

    transfer()
    return Result(ready=ingress.ready, valid=egress.valid, data=egress.data)

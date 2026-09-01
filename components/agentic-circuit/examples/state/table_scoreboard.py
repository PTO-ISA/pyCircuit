"""Contract-epoch 0.4 state Table scoreboard prototype."""

import agentic_circuit as ac


@ac.struct
class Entry:
    valid: bool
    done: bool
    result: ac.u16


@ac.struct
class Update:
    index: ac.u4
    enable: bool
    done: bool
    result: ac.u16


@ac.struct
class Request:
    index: ac.u4
    enable: bool


@ac.system
def table_scoreboard() -> None:
    updates = ac.source(Update, depth=2, latency=1)
    requests = ac.source(Request, depth=2, latency=1)
    scoreboard = ac.table[16, Entry](init=0)

    scoreboard.view(lambda update: update.index).patch(
        updates,
        enable=lambda update: update.enable,
        valid=True,
        done=lambda update: update.done,
        result=lambda update: update.result,
    )
    responses = scoreboard.view(lambda request: request.index).read(
        requests,
        when=lambda request: request.enable,
        depth=1,
        latency=1,
    )

    entry_zero = scoreboard.view(0)
    snapshots = entry_zero.read(when=entry_zero.valid, depth=1, latency=1)

    ac.sink(responses)
    ac.sink(snapshots)

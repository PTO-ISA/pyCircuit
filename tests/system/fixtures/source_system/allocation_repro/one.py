import pycircuit as ac
from allocation_repro.pool import AllocationPool
from allocation_repro.contracts import Request

@ac.system
def Exercise():
    phase: ac.u8 = 0
    owner = AllocationPool()

    @ac.rule
    def send() -> None:
        if phase == 0:
            owner.submit(Request(
                operation=0, group_id=9, entry_id=1,
                requester=2, request_id=3, response_route=4,
                current_position=10, target_position=0, steps=0,
                accepted=False, matched=False, exhausted=False,
                stale=False,
            ))

    @ac.rule
    def recv() -> None:
        r = owner.allocate_response()
        ac.log('info', 'result', r.accepted, r.current_position)

    @ac.rule
    def tick() -> None:
        phase = phase + 1

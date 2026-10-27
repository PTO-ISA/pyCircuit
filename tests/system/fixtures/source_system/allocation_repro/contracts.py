import pycircuit as ac
POOL_CAPACITY = 16
ALLOCATE = 0
LOOKUP = 1
RELEASE = 2

@ac.struct
class Entry:
    group_id: ac.u32
    entry_id: ac.u16
    position: ac.u6
    live: bool

@ac.struct
class Request:
    operation: ac.u8
    group_id: ac.u32
    entry_id: ac.u16
    requester: ac.u8
    request_id: ac.u16
    response_route: ac.u8
    current_position: ac.u6
    target_position: ac.u6
    steps: ac.u6
    accepted: bool
    matched: bool
    exhausted: bool
    stale: bool

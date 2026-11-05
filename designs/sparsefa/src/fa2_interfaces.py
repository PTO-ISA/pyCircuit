"""External protocol for the serial, single-query FA2 kernel.

All addresses are byte addresses. Reads and writes both require one response;
read data occupies the low bits. A request is accepted on valid & ready and
its response cannot arrive before the following cycle. Only one is outstanding.
The host must hold a command until command_ready and accept completion once.
Cancellation is level-sensitive and takes priority over command/request issue.
An offered, unaccepted request is withdrawn on cancel; an accepted request
retains response_ready until its response is drained.
"""

import pycircuit as ac


@ac.struct
class EngineCommand:
    valid: ac.u1
    tag: ac.u32
    q_base: ac.u64
    k_base: ac.u64
    v_base: ac.u64
    o_base: ac.u64
    count_base: ac.u64
    index_base: ac.u64
    k_stride: ac.u64
    v_stride: ac.u64
    key_length: ac.u32
    query_position: ac.u32
    dimensions: ac.u7
    scale: ac.u32
    indexed: ac.u1
    causal: ac.u1
    causal_empty: ac.u1
    bf16: ac.u1


@ac.struct
class MemoryReply:
    request_ready: ac.u1
    valid: ac.u1
    data: ac.u32
    error: ac.u1


@ac.struct
class MemoryRequest:
    valid: ac.u1
    write: ac.u1
    address: ac.u64
    data: ac.u32
    size: ac.u3
    response_ready: ac.u1


@ac.struct
class EngineStatus:
    command_ready: ac.u1
    completion_valid: ac.u1
    tag: ac.u32
    code: ac.u4
    phase: ac.u6

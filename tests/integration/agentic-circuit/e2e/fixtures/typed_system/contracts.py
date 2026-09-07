from enum import Enum

import agentic_circuit as ac


class AccessKind(Enum):
    READ = 0
    WRITE = 1


@ac.struct
class Entry:
    generation: ac.u16
    value: ac.u32
    valid: bool


@ac.struct
class ReadRequest:
    index: ac.u7
    generation: ac.u16


@ac.struct
class WriteRequest:
    index: ac.u7
    generation: ac.u16
    value: ac.u32
    kind: AccessKind
    valid: bool


@ac.struct
class ReadResult:
    request: ReadRequest
    value: ac.u32
    found: bool


@ac.struct
class WriteAck:
    request: WriteRequest
    accepted: bool

"""Compose a complete token using nested records, nominal enums and status."""

from enum import Enum
import pycircuit as ac


@ac.encoding(width=4)
class Opcode(Enum):
    NONE = 0
    READ = 3
    WRITE = 9


@ac.struct
class Header:
    opcode_bits: ac.u4
    tag: ac.u4


@ac.struct
class Patch:
    tag: ac.u4
    valid: ac.u1


@ac.struct
class Packet:
    opcode_bits: ac.u4
    tag: ac.u4
    valid: ac.u1


@ac.struct
class Item:
    header: Header
    patch: Patch
    opcode: Opcode
    flags: ac.u4
    result_tag: ac.u4
    result_valid: ac.u1
    result_opcode: Opcode
    onehot_index: ac.u2
    onehot_valid: ac.u1
    onehot_conflict: ac.u1


@ac.struct
class Result:
    ready: ac.u1
    valid: ac.u1
    data: Item


@ac.rule
def compose(item: Item) -> Item:
    packet = Packet(opcode_bits=item.header.opcode_bits, tag=item.header.tag)
    patched = packet
    patched.tag = item.patch.tag
    patched.valid = item.patch.valid
    index, valid, conflict = ac.onehot_encode(item.flags)
    result = item
    result.result_tag = patched.tag
    result.result_valid = patched.valid
    result.result_opcode = Opcode.WRITE
    result.onehot_index = index
    result.onehot_valid = valid
    result.onehot_conflict = conflict
    return result


@ac.module
def FrontendCompositionPipeline(  # noqa: N802 -- hardware root name.
    valid: ac.u1, data: Item, take: ac.u1
) -> Result:
    ready, available, incoming = ac.queue[Item](
        valid,
        data,
        result_ready,  # noqa: F821 -- forward ready wire.
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    composed = compose(incoming)
    result_ready, out_valid, out_data = ac.queue[Item](
        available,
        composed,
        take,
        depth=1,
        latency=1,
        ready_policy="downstream_pop",
    )
    return Result(ready=ready, valid=out_valid, data=out_data)

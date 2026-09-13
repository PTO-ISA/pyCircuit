"""Record spread, sparse enums, and one-hot status in one executable token."""

from enum import Enum

import agentic_circuit as ac


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
    valid: bool


@ac.struct
class Packet:
    opcode_bits: ac.u4
    tag: ac.u4
    valid: bool


@ac.struct
class Item:
    header: Header
    patch: Patch
    opcode: Opcode
    flags: ac.u4
    result_tag: ac.u4
    result_valid: bool
    result_opcode: Opcode
    onehot_index: ac.u2
    onehot_valid: bool
    onehot_conflict: bool


@ac.rule
def compose(item: Item) -> Item:
    packet = Packet(**item.header, valid=False)
    patched = packet.with_fields(**item.patch)
    return item.with_fields(
        result_tag=patched.tag,
        result_valid=patched.valid,
        result_opcode=Opcode.WRITE,
        onehot_index=ac.onehot_encode(item.flags, order="low").index,
        onehot_valid=ac.onehot_encode(item.flags, order="low").valid,
        onehot_conflict=ac.onehot_encode(item.flags, order="low").conflict,
    )


@ac.system
def frontend_composition_pipeline(incoming: Item) -> Item:
    result = compose(incoming)
    return result


specialization = ac.jit(frontend_composition_pipeline)

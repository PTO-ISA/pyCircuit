"""Canonical bounded PTO execution payload carried as one packed scalar ABI."""

from __future__ import annotations

from enum import Enum

import agentic_circuit as ac


class EngineKind(Enum):
    SCALAR = 0
    VECTOR = 1
    CUBE = 2
    TMA = 3


class DType(Enum):
    U8 = 0
    U16 = 1
    U32 = 2
    U64 = 3
    I8 = 4
    I16 = 5
    I32 = 6
    I64 = 7
    FP16 = 8
    BF16 = 9
    FP32 = 10
    FP64 = 11


class TileLayout(Enum):
    ND = 0
    DN = 1
    NZ = 2
    ZN = 3


@ac.struct
class PTOShape:
    rank: ac.u3
    dimensions: ac.array[5, ac.u16]


@ac.struct
class PTOTileOperand:
    present: bool
    address: ac.u64
    dtype: DType
    layout: TileLayout
    shape: PTOShape


@ac.struct
class PTOScalarOperand:
    present: bool
    dtype: DType
    bits: ac.u64


@ac.struct
class PTOExecutionPayload:
    opcode_id: ac.u16
    engine_kind: EngineKind
    sequence_id: ac.u16
    block_id: ac.u16
    input_tile_count: ac.u3
    input_tiles: ac.array[4, PTOTileOperand]
    scalar_input_count: ac.u3
    scalar_inputs: ac.array[4, PTOScalarOperand]
    output_tile_count: ac.u2
    output_tiles: ac.array[2, PTOTileOperand]


@ac.rule
def advance_block(item):
    return item.with_fields(block_id=item.block_id + 1)


@ac.system
def pto_payload_abi(incoming: PTOExecutionPayload) -> PTOExecutionPayload:
    updated = advance_block(incoming)
    return updated

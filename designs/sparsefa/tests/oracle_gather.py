"""Test-only byte-address oracle; not a bus simulator or accelerator source."""

from __future__ import annotations

from collections.abc import Sequence
from dataclasses import dataclass

from .oracle_attention import AttentionInputError, validate_index_row

ADDRESS_LIMIT = 1 << 64


def _natural(value: int, name: str, *, positive: bool = False) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < int(positive):
        raise AttentionInputError(f"invalid {name}")
    return value


@dataclass(frozen=True)
class ReadView:
    """A strided read-only allocation, with [region_start, region_end) bounds."""

    base: int
    shape: tuple[int, ...]
    strides: tuple[int, ...]
    element_bytes: int
    region_start: int
    region_end: int

    def __post_init__(self) -> None:
        for value, name in (
            (self.base, "base"),
            (self.region_start, "region_start"),
            (self.region_end, "region_end"),
        ):
            _natural(value, name)
        _natural(self.element_bytes, "element_bytes", positive=True)
        if not self.shape or len(self.shape) != len(self.strides):
            raise AttentionInputError("shape/stride rank mismatch")
        for size, stride in zip(self.shape, self.strides, strict=True):
            _natural(size, "dimension", positive=True)
            _natural(stride, "stride")
            if stride % self.element_bytes:
                raise AttentionInputError("unaligned stride")
        if self.base % self.element_bytes:
            raise AttentionInputError("unaligned base")
        if not 0 <= self.region_start < self.region_end <= ADDRESS_LIMIT:
            raise AttentionInputError("invalid readable region")
        # Python integers are intentionally unbounded: detect overflow BEFORE any
        # truncation to the hardware's 64-bit address width.
        last = (
            self.base
            + sum((n - 1) * s for n, s in zip(self.shape, self.strides, strict=True))
            + self.element_bytes
            - 1
        )
        if (
            self.base < self.region_start
            or last >= self.region_end
            or last >= ADDRESS_LIMIT
        ):
            raise AttentionInputError("ADDRESS_RANGE")

    def address(self, *coordinates: int) -> int:
        if len(coordinates) != len(self.shape):
            raise AttentionInputError("coordinate rank mismatch")
        for coordinate, size in zip(coordinates, self.shape, strict=True):
            _natural(coordinate, "coordinate")
            if coordinate >= size:
                raise AttentionInputError("coordinate out of range")
        return self.base + sum(
            c * s for c, s in zip(coordinates, self.strides, strict=True)
        )


@dataclass(frozen=True)
class GatherRow:
    batch: int
    head: int
    query: int
    list_position: int
    token: int
    tile: int
    packed_row: int
    k_addresses: tuple[int, ...]
    v_addresses: tuple[int, ...]


def gather_rows(
    indices: Sequence[int],
    count: int,
    k: ReadView,
    v: ReadView,
    *,
    batch: int = 0,
    head: int = 0,
    query: int = 0,
    tile_rows: int = 16,
) -> list[GatherRow]:
    """Expected element reads, including selected tokens later masked by causal."""
    _natural(tile_rows, "tile_rows", positive=True)
    _natural(query, "query")
    if len(k.shape) != 4 or k.shape != v.shape:
        raise AttentionInputError("P0 requires matching rank-four K/V shapes")
    for view in (k, v):
        if view.element_bytes != 2 or view.strides[-1] != 2:
            raise AttentionInputError("unsupported P0 KV layout")
        # Validate b/h even when count=0 produces no reads.
        view.address(batch, head, 0, 0)
    tokens = validate_index_row(indices, count, k.shape[2])
    return [
        GatherRow(
            batch,
            head,
            query,
            p,
            j,
            p // tile_rows,
            p % tile_rows,
            tuple(k.address(batch, head, j, d) for d in range(k.shape[3])),
            tuple(v.address(batch, head, j, d) for d in range(v.shape[3])),
        )
        for p, j in enumerate(tokens)
    ]


def index_read_addresses(
    indices: ReadView,
    counts: ReadView,
    *,
    batch: int,
    head: int,
    query: int,
    count: int,
    key_length: int,
) -> tuple[int, tuple[int, ...]]:
    """Expected count address and active prefix addresses, without beat padding."""
    if len(indices.shape) != 4 or counts.shape != indices.shape[:3]:
        raise AttentionInputError("Index/IndexCount shape mismatch")
    if (
        indices.element_bytes != 4
        or counts.element_bytes != 4
        or indices.strides[-1] != 4
    ):
        raise AttentionInputError("unsupported index layout")
    _natural(count, "count")
    _natural(key_length, "S_k", positive=True)
    if indices.shape[-1] > 2048 or key_length > 0xFFFFFFFF:
        raise AttentionInputError("UNSUPPORTED_INDEX_CAPACITY or S_k")
    if count > min(indices.shape[-1], key_length):
        raise AttentionInputError("INDEX_COUNT")
    return counts.address(batch, head, query), tuple(
        indices.address(batch, head, query, p) for p in range(count)
    )

"""Test-only binary64 attention oracles, independent of pyCircuit and any DUT.

The full-row reference defines the mathematical result. The online model checks
partitioning, not hardware behavior. Neither models FP16/BF16 conversion, FP32
rounding, approximate exp, or bus timing. D_v may differ from D_k here; hardware
P0 is narrower. All hardware precision and implementation claims remain separate.
"""

from __future__ import annotations

from collections.abc import Sequence
from dataclasses import dataclass
from math import exp, isfinite
from numbers import Real
from typing import Literal

MaskMode = Literal["none", "causal"]
Tensor4 = Sequence[Sequence[Sequence[Sequence[float]]]]
Tensor4List = list[list[list[list[float]]]]
IndexTensor = Sequence[Sequence[Sequence[Sequence[int]]]]
CountTensor = Sequence[Sequence[Sequence[int]]]
KeyRows = list[list[list[list[int]]]]


class AttentionInputError(ValueError):
    """An input or binary64 arithmetic operation violates the model contract."""


@dataclass(frozen=True)
class SparseSelection:
    """External token indices [B,H,Sq,Kmax] and valid counts [B,H,Sq]."""

    indices: IndexTensor
    counts: CountTensor


@dataclass(frozen=True)
class P0Profile:
    """Tiling/mask knobs for the mathematical model, not a hardware FP profile."""

    q_tile_rows: int = 16
    kv_tile_rows: int = 16
    d_tile: int = 16
    causal_offset: int = 0
    mask_mode: MaskMode = "none"

    def __post_init__(self) -> None:
        for name in ("q_tile_rows", "kv_tile_rows", "d_tile"):
            _integer(getattr(self, name), name, minimum=1)
        _mask_config(self.mask_mode, self.causal_offset)


def _integer(value: int, name: str, *, minimum: int | None = None) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise AttentionInputError(f"{name} must be an integer")
    if minimum is not None and value < minimum:
        raise AttentionInputError(f"{name} must be >= {minimum}")
    return value


def _length(value: Sequence, name: str) -> int:
    if isinstance(value, (str, bytes)) or not isinstance(value, Sequence):
        raise AttentionInputError(f"{name} must be a sequence")
    return len(value)


def _finite(value: float, name: str) -> float:
    if isinstance(value, bool) or not isinstance(value, Real):
        raise AttentionInputError(f"{name} must be numeric")
    try:
        converted = float(value)
    except (ValueError, OverflowError) as exc:
        raise AttentionInputError(f"{name} exceeds binary64") from exc
    if not isfinite(converted):
        raise AttentionInputError(f"{name} must be finite")
    return converted


def _mask_config(mode: MaskMode, offset: int) -> None:
    if mode not in ("none", "causal"):
        raise AttentionInputError(f"unsupported mask mode: {mode!r}")
    _integer(offset, "causal_offset")


def _shape4(tensor: Tensor4, name: str) -> tuple[int, int, int, int]:
    """Validate every level, including malformed ranks and ragged rows."""
    b = _length(tensor, name)
    if not b:
        raise AttentionInputError(f"{name} dimensions must be positive")
    h = _length(tensor[0], name)
    if not h:
        raise AttentionInputError(f"{name} dimensions must be positive")
    s = _length(tensor[0][0], name)
    if not s:
        raise AttentionInputError(f"{name} dimensions must be positive")
    d = _length(tensor[0][0][0], name)
    if not d:
        raise AttentionInputError(f"{name} dimensions must be positive")
    for batch in tensor:
        if _length(batch, name) != h:
            raise AttentionInputError(f"{name} has inconsistent heads")
        for head in batch:
            if _length(head, name) != s:
                raise AttentionInputError(f"{name} has inconsistent rows")
            for row in head:
                if _length(row, name) != d:
                    raise AttentionInputError(f"{name} has inconsistent dimensions")
                for value in row:
                    _finite(value, name)
    return b, h, s, d


def _validate_inputs(
    q: Tensor4,
    k: Tensor4,
    v: Tensor4,
    scale: float,
    mask_mode: MaskMode,
    causal_offset: int,
) -> tuple[int, int, int, int, int]:
    qs, ks, vs = _shape4(q, "Q"), _shape4(k, "K"), _shape4(v, "V")
    _finite(scale, "scale")
    _mask_config(mask_mode, causal_offset)
    if qs[:2] != ks[:2] or qs[:2] != vs[:2]:
        raise AttentionInputError("Q, K and V batch/head dimensions must match")
    if ks[2] != vs[2]:
        raise AttentionInputError("K and V sequence dimensions must match")
    if qs[3] != ks[3]:
        raise AttentionInputError("Q and K feature dimensions must match")
    return qs[0], qs[1], qs[2], ks[2], vs[3]


def validate_index_row(
    indices: Sequence[int], count: int, key_length: int
) -> list[int]:
    """Validate an entire row before masking/gathering; ignore padding values."""
    capacity = _length(indices, "Index row")
    _integer(key_length, "S_k", minimum=1)
    if key_length > 0xFFFFFFFF or not 1 <= capacity <= 2048:
        raise AttentionInputError("UNSUPPORTED_INDEX_CAPACITY or S_k")
    _integer(count, "IndexCount", minimum=0)
    if count > min(capacity, key_length):
        raise AttentionInputError("INDEX_COUNT")
    result: list[int] = []
    seen: set[int] = set()
    for value in indices[:count]:
        _integer(value, "Index token", minimum=0)
        if value >= key_length:
            raise AttentionInputError("INDEX_RANGE")
        if value in seen:
            raise AttentionInputError("INDEX_DUPLICATE")
        seen.add(value)
        result.append(value)
    return result


def _selection_rows(
    selection: SparseSelection | None, b: int, h: int, sq: int, sk: int
) -> KeyRows:
    if selection is None:
        return [
            [[list(range(sk)) for _ in range(sq)] for _ in range(h)] for _ in range(b)
        ]
    if not isinstance(selection, SparseSelection):
        raise AttentionInputError("indexed mode requires Index and IndexCount")
    indices, counts = selection.indices, selection.counts
    if _length(indices, "Index") != b or _length(counts, "IndexCount") != b:
        raise AttentionInputError("Index/IndexCount batch mismatch")
    result: KeyRows = []
    capacity: int | None = None
    for bi in range(b):
        if _length(indices[bi], "Index") != h or _length(counts[bi], "IndexCount") != h:
            raise AttentionInputError("Index/IndexCount head mismatch")
        heads: list[list[list[int]]] = []
        for hi in range(h):
            if (
                _length(indices[bi][hi], "Index") != sq
                or _length(counts[bi][hi], "IndexCount") != sq
            ):
                raise AttentionInputError("Index/IndexCount query mismatch")
            rows: list[list[int]] = []
            for qi in range(sq):
                row = indices[bi][hi][qi]
                width = _length(row, "Index")
                if capacity is None:
                    capacity = width
                if width != capacity:
                    raise AttentionInputError("Index K_max must be rectangular")
                rows.append(validate_index_row(row, counts[bi][hi][qi], sk))
            heads.append(rows)
        result.append(heads)
    return result


def _dot(lhs: Sequence[float], rhs: Sequence[float]) -> float:
    total = sum(
        float(left) * float(right) for left, right in zip(lhs, rhs, strict=True)
    )
    return _finite(total, "dot product")


def _tiled_dot(lhs: Sequence[float], rhs: Sequence[float], tile: int) -> float:
    total = 0.0
    for start in range(0, len(lhs), tile):
        partial = 0.0
        for d in range(start, min(start + tile, len(lhs))):
            partial += float(lhs[d]) * float(rhs[d])
        total += partial
    return _finite(total, "tiled dot product")


def dense_attention(
    q: Tensor4,
    k: Tensor4,
    v: Tensor4,
    *,
    scale: float,
    mask_mode: MaskMode = "none",
    causal_offset: int = 0,
    selection: SparseSelection | None = None,
) -> Tensor4List:
    """Full-row mathematical oracle; selection optionally restricts its keys.

    Sorting the set gives this oracle a deterministic reduction order independent
    of the gather order used by the online model. Padding is never evaluated.
    """
    b, h, sq, sk, dv = _validate_inputs(q, k, v, scale, mask_mode, causal_offset)
    rows = _selection_rows(selection, b, h, sq, sk)
    output: Tensor4List = []
    for bi in range(b):
        output_heads = []
        for hi in range(h):
            output_rows = []
            for qi in range(sq):
                allowed = sorted(
                    j
                    for j in rows[bi][hi][qi]
                    if mask_mode == "none" or j <= qi + causal_offset
                )
                if not allowed:
                    output_rows.append([0.0] * dv)
                    continue
                scores = [
                    _finite(float(scale) * _dot(q[bi][hi][qi], k[bi][hi][j]), "score")
                    for j in allowed
                ]
                maximum = max(scores)
                weights = [exp(score - maximum) for score in scores]
                denominator = sum(weights)  # At least one weight is exactly 1.
                output_rows.append(
                    [
                        _finite(
                            sum(
                                (weight / denominator) * float(v[bi][hi][j][d])
                                for weight, j in zip(weights, allowed, strict=True)
                            ),
                            "output",
                        )
                        for d in range(dv)
                    ]
                )
            output_heads.append(output_rows)
        output.append(output_heads)
    return output


def tiled_online_attention(
    q: Tensor4,
    k: Tensor4,
    v: Tensor4,
    *,
    scale: float,
    profile: P0Profile | None = None,
    selection: SparseSelection | None = None,
) -> Tensor4List:
    """Online recurrence over each query's list positions, retaining token IDs."""
    profile = profile or P0Profile()
    b, h, sq, sk, dv = _validate_inputs(
        q, k, v, scale, profile.mask_mode, profile.causal_offset
    )
    rows = _selection_rows(selection, b, h, sq, sk)
    output: Tensor4List = []
    for bi in range(b):
        output_heads = []
        for hi in range(h):
            output_rows = []
            for q_start in range(0, sq, profile.q_tile_rows):
                for qi in range(q_start, min(sq, q_start + profile.q_tile_rows)):
                    maximum, normalizer, accumulator = -float("inf"), 0.0, [0.0] * dv
                    selected = rows[bi][hi][qi]
                    for start in range(0, len(selected), profile.kv_tile_rows):
                        tokens, scores = [], []
                        for j in selected[start : start + profile.kv_tile_rows]:
                            if (
                                profile.mask_mode == "causal"
                                and j > qi + profile.causal_offset
                            ):
                                continue
                            tokens.append(j)
                            scores.append(
                                _finite(
                                    float(scale)
                                    * _tiled_dot(
                                        q[bi][hi][qi], k[bi][hi][j], profile.d_tile
                                    ),
                                    "score",
                                )
                            )
                        if not tokens:
                            continue
                        new_maximum = max(maximum, max(scores))
                        alpha = 0.0 if normalizer == 0.0 else exp(maximum - new_maximum)
                        weights = [exp(score - new_maximum) for score in scores]
                        normalizer = _finite(
                            alpha * normalizer + sum(weights), "row sum"
                        )
                        accumulator = [
                            _finite(
                                alpha * accumulator[d]
                                + sum(
                                    weight * float(v[bi][hi][j][d])
                                    for weight, j in zip(weights, tokens, strict=True)
                                ),
                                "accumulator",
                            )
                            for d in range(dv)
                        ]
                        maximum = new_maximum
                    output_rows.append(
                        [0.0] * dv
                        if normalizer == 0.0
                        else [value / normalizer for value in accumulator]
                    )
            output_heads.append(output_rows)
        output.append(output_heads)
    return output


def max_abs_error(lhs: Tensor4, rhs: Tensor4) -> float:
    if _shape4(lhs, "lhs") != _shape4(rhs, "rhs"):
        raise AttentionInputError("tensor shapes differ")
    return max(
        abs(float(left) - float(right))
        for lb, rb in zip(lhs, rhs, strict=True)
        for lh, rh in zip(lb, rb, strict=True)
        for lr, rr in zip(lh, rh, strict=True)
        for left, right in zip(lr, rr, strict=True)
    )


def p0_layout_address(
    row: int, dimension: int, total_dimension: int, *, lanes: int = 16
) -> tuple[int, int, int]:
    """Return (bank, word, linear_word) for the P0 lane-striped layout."""
    _integer(row, "row", minimum=0)
    _integer(dimension, "dimension", minimum=0)
    _integer(total_dimension, "total_dimension", minimum=1)
    _integer(lanes, "lanes", minimum=1)
    if dimension >= total_dimension:
        raise AttentionInputError("dimension index exceeds total_dimension")
    word = dimension // lanes
    return (
        dimension % lanes,
        word,
        row * ((total_dimension + lanes - 1) // lanes) + word,
    )

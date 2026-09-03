from __future__ import annotations

from collections.abc import Sequence
from typing import Any


def onehot_mux(sel: Sequence[Any], vals: Sequence[Any]) -> Any:
    if len(sel) == 0 or len(vals) == 0:
        raise ValueError("onehot_mux requires non-empty sel/vals")
    if len(sel) != len(vals):
        raise ValueError(f"onehot_mux length mismatch: sel={len(sel)} vals={len(vals)}")

    out = vals[0] ^ vals[0]
    for s, v in zip(sel, vals):
        out = v if s else out
    return out


def priority_pick(
    bits: Sequence[Any], *, n: int = 1
) -> tuple[list[list[Any]], list[Any]]:
    if len(bits) == 0:
        raise ValueError("priority_pick requires at least one bit")
    kcnt = max(1, int(n))

    rem = list(bits)
    sels: list[list[Any]] = []
    valids: list[Any] = []
    for _ in range(kcnt):
        seen = rem[0] ^ rem[0]
        onehot: list[Any] = []
        for b in rem:
            s = b & (1 ^ seen)
            onehot.append(s)
            seen = seen | b
        sels.append(onehot)
        valids.append(seen)
        rem = [rem[i] & (1 ^ onehot[i]) for i in range(len(rem))]
    return sels, valids


def match_any(
    key: Any, keys: Sequence[Any], *, valids: Sequence[Any] | None = None
) -> Any:
    if len(keys) == 0:
        raise ValueError("match_any requires at least one key")
    if valids is not None and len(valids) != len(keys):
        raise ValueError(
            f"match_any length mismatch: keys={len(keys)} valids={len(valids)}"
        )

    out = (key == key) & 0
    for i, k in enumerate(keys):
        v = 1 if valids is None else valids[i]
        out = out | (v & (key == k))
    return out


__all__ = [
    "match_any",
    "onehot_mux",
    "priority_pick",
]

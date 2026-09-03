from __future__ import annotations

from collections.abc import Callable, Mapping, Sequence
from itertools import product as _it_product

from .types import ParamSet, ParamSpace


def _canonical_values(
    mapping: Mapping[str, bool | int | str]
) -> tuple[tuple[str, bool | int | str], ...]:
    vals: list[tuple[str, bool | int | str]] = []
    for k, v in mapping.items():
        key = str(k)
        if not key:
            raise ValueError("parameter key must be non-empty")
        if not isinstance(v, (bool, int, str)):
            raise TypeError(
                f"parameter {key!r} value must be bool/int/str, got {type(v).__name__}"
            )
        vals.append((key, v))
    return tuple(sorted(vals, key=lambda kv: kv[0]))


def named_variant(name: str, **values: bool | int | str) -> ParamSet:
    return ParamSet(values=_canonical_values(values), name=str(name))


def product(space: Mapping[str, Sequence[bool | int | str]]) -> ParamSpace:
    keys = sorted(str(k) for k in space.keys())
    if not keys:
        raise ValueError("product() requires at least one parameter dimension")
    dims: list[Sequence[bool | int | str]] = []
    for k in keys:
        vals = list(space[k])
        if not vals:
            raise ValueError(f"product() parameter {k!r} has no values")
        for v in vals:
            if not isinstance(v, (bool, int, str)):
                raise TypeError(
                    f"product() value for {k!r} must be bool/int/str, got {type(v).__name__}"
                )
        dims.append(tuple(vals))

    variants: list[ParamSet] = []
    for idx, combo in enumerate(_it_product(*dims)):
        vals = tuple((k, v) for k, v in zip(keys, combo))
        variants.append(ParamSet(values=vals, name=f"v{idx}"))
    return ParamSpace(tuple(variants))


def grid(space: Mapping[str, Sequence[bool | int | str]]) -> ParamSpace:
    return product(space)


def filter(space: ParamSpace, pred: Callable[[ParamSet], bool]) -> ParamSpace:
    out = [v for v in space if bool(pred(v))]
    if not out:
        raise ValueError("filter() produced an empty parameter space")
    return ParamSpace(tuple(out))

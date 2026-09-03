"""Deterministic static collection classification."""

from __future__ import annotations

from collections.abc import Sequence
from dataclasses import dataclass
from typing import Literal, TypeAlias

from ._resolve import ResolvedCall


CollectionElement: TypeAlias = ResolvedCall | Sequence["CollectionElement"]


class CollectionError(ValueError):
    """Raised when a static collection has no canonical fixed shape."""


@dataclass(frozen=True, slots=True)
class CollectionPlan:
    kind: Literal["array", "instances"]
    shape: tuple[int, ...]
    elements: tuple[str, ...]
    element_schema: str | None


def _shape_and_flatten(
    element: CollectionElement,
) -> tuple[tuple[int, ...], tuple[ResolvedCall, ...]]:
    if isinstance(element, ResolvedCall):
        return (), (element,)
    if isinstance(element, (str, bytes)) or not isinstance(element, Sequence):
        raise CollectionError("ACPY-STATIC-COLLECTION: invalid collection element")
    if not element:
        return (0,), ()
    children = [_shape_and_flatten(child) for child in element]
    child_shape = children[0][0]
    if any(shape != child_shape for shape, _ in children[1:]):
        raise CollectionError("ACPY-STATIC-COLLECTION: ragged collection")
    flat = tuple(item for _, values in children for item in values)
    return (len(element), *child_shape), flat


def classify_collection(elements: Sequence[CollectionElement]) -> CollectionPlan:
    shape, flat = _shape_and_flatten(elements)
    if not flat:
        return CollectionPlan("instances", shape, (), None)
    specialization = (
        flat[0].schema.identity,
        flat[0].static_arguments,
    )
    homogeneous = all(
        (item.schema.identity, item.static_arguments) == specialization
        for item in flat[1:]
    )
    return CollectionPlan(
        kind="array" if homogeneous else "instances",
        shape=shape,
        elements=tuple(item.entity_key for item in flat),
        element_schema=flat[0].schema.identity if homogeneous else None,
    )

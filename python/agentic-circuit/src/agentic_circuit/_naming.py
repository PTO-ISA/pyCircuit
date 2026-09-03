"""Stable source-ordered instance naming."""

from __future__ import annotations

import re


_SEGMENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")


class StableNameError(ValueError):
    """Raised when an instance name is invalid or collides."""


def normalize_schema_base(schema_base: str) -> str:
    return schema_base.rsplit(".", 1)[-1]


def validate_instance_segment(candidate: str) -> None:
    if not _SEGMENT.fullmatch(candidate):
        raise StableNameError(
            f"ACPY-NAME-001: invalid instance name segment {candidate!r}"
        )


class StableNameAllocator:
    __slots__ = ("_used",)

    def __init__(self) -> None:
        self._used: set[str] = set()

    def allocate(
        self,
        schema_base: str,
        assignment: str | None,
        explicit: str | None,
        source_key: tuple[int, int],
    ) -> str:
        if explicit is not None:
            candidate = explicit
        elif assignment is not None:
            candidate = assignment
        else:
            base = normalize_schema_base(schema_base)
            candidate = f"{base}_{source_key[0]}_{source_key[1]}"
        validate_instance_segment(candidate)
        if candidate in self._used:
            raise StableNameError(
                f"ACPY-NAME-002: duplicate instance name {candidate!r}"
            )
        self._used.add(candidate)
        return candidate

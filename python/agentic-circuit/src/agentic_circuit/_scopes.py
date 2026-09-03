"""Minimal strong-scope capture and escape outlining."""

from __future__ import annotations

from dataclasses import dataclass

from ._normalize import NormalizedProgram
from ._resolve import ResolvedCall, ValueVersion


class ScopeError(ValueError):
    """Raised when a strong-scope ownership boundary is invalid."""


@dataclass(frozen=True, slots=True)
class CaptureBinding:
    value: ValueVersion


@dataclass(frozen=True, slots=True)
class EscapeBinding:
    value: ValueVersion


@dataclass(frozen=True, slots=True)
class OutlinedScope:
    key: str
    name: str
    parent: str | None
    captures: tuple[CaptureBinding, ...]
    escapes: tuple[EscapeBinding, ...]
    body: tuple[ResolvedCall, ...]


def outline_scopes(program: NormalizedProgram) -> tuple[OutlinedScope, ...]:
    calls = {call.entity_key: call for call in program.calls}
    outlined: list[OutlinedScope] = []
    for region in program.scopes:
        internal = set(region.call_keys)
        internal_values = set(region.value_names)
        body = tuple(calls[key] for key in region.call_keys)
        captures: list[CaptureBinding] = []
        captured_names: set[str] = set()
        for call in body:
            for binding in call.inputs:
                value = binding.value
                if value.producer not in internal and value.name not in captured_names:
                    captured_names.add(value.name)
                    captures.append(CaptureBinding(value))

        escaped_names: set[str] = set()
        for call in program.calls:
            if call.entity_key in internal:
                continue
            for binding in call.inputs:
                if (
                    binding.value.producer in internal
                    or binding.value.name in internal_values
                ):
                    escaped_names.add(binding.value.name)
        for value in program.returns:
            if value.producer in internal or value.name in internal_values:
                escaped_names.add(value.name)
        escapes = tuple(
            EscapeBinding(value)
            for value in program.values
            if value.name in escaped_names
        )
        invalid = next(
            (
                binding.value
                for binding in escapes
                if binding.value.category == "resource"
                and binding.value.ownership == "owned"
            ),
            None,
        )
        if invalid is not None:
            raise ScopeError(
                f"ACPY-SCOPE-004: owned resource {invalid.source_name!r} escapes scope {region.name!r}"
            )
        outlined.append(
            OutlinedScope(
                key=region.key,
                name=region.name,
                parent=region.parent,
                captures=tuple(captures),
                escapes=escapes,
                body=body,
            )
        )
    return tuple(outlined)

"""Stable source paths and MLIR locations for the Queue compiler."""

from __future__ import annotations

import re
from collections.abc import Collection

from .._canonical_json import canonical_mlir_string
from .._source_map import SourceFrame
from .errors import QueueFrontendError


_DEFAULT_QUEUE_SOURCE_PATH = "<queue-model>"


def _normalize_queue_source_path(source_path: str | None) -> str:
    """Return a stable, non-absolute source path for display metadata."""

    if source_path is None or source_path == _DEFAULT_QUEUE_SOURCE_PATH:
        return _DEFAULT_QUEUE_SOURCE_PATH
    normalized = source_path.replace("\\", "/")
    parts = [part for part in normalized.split("/") if part not in {"", "."}]
    if (
        normalized[:1] == "/"
        or re.match(r"^[A-Za-z]:/", normalized) is not None
        or ".." in parts
    ):
        return parts[-1] if parts else _DEFAULT_QUEUE_SOURCE_PATH
    result = "/".join(parts) or _DEFAULT_QUEUE_SOURCE_PATH
    if (
        result != _DEFAULT_QUEUE_SOURCE_PATH
        and re.fullmatch(r"[A-Za-z0-9._+@/-]+\.py", result) is None
    ):
        raise QueueFrontendError(
            "ACPY-QUEUE-027: source path contains unsupported characters"
        )
    return result


def _render_source_frame_location(frame: SourceFrame | None) -> str:
    if frame is None:
        return ""
    return (
        " loc(" + canonical_mlir_string(frame.file) + f":{frame.line}:{frame.column})"
    )


def _render_callsite_location(
    definition: SourceFrame | None,
    callsite: SourceFrame | None,
) -> str:
    if definition is None:
        return _render_source_frame_location(callsite)
    if callsite is None or callsite == definition:
        return _render_source_frame_location(definition)
    return (
        " loc(callsite("
        + canonical_mlir_string(definition.file)
        + f":{definition.line}:{definition.column} at "
        + canonical_mlir_string(callsite.file)
        + f":{callsite.line}:{callsite.column}))"
    )


def _render_fused_source_locations(frames: Collection[SourceFrame | None]) -> str:
    unique = tuple(dict.fromkeys(frame for frame in frames if frame is not None))
    if not unique:
        return ""
    if len(unique) == 1:
        return _render_source_frame_location(unique[0])
    return (
        " loc(fused["
        + ", ".join(
            canonical_mlir_string(frame.file) + f":{frame.line}:{frame.column}"
            for frame in unique
        )
        + "])"
    )

"""Strict deterministic packing for committed gfsim event JSONL.

This module is repository tooling, not part of the installed public package.
It validates and wraps existing Chrome Trace Event records without synthesizing
timing, metadata, or ordering.
"""

from __future__ import annotations

import errno
import json
import os
import re
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import NoReturn

from agentic_circuit._canonical_json import JsonValue, canonical_json_bytes


_SAFE_INTEGER_MAX = (1 << 53) - 1
_MAX_DELTA = 1024
_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_.-]*$")
_COMMITTED_ARGUMENTS = {
    "gfsim_epoch_time",
    "gfsim_epoch_delta",
    "gfsim_object_id",
    "gfsim_local_committed_index",
}
_OPTIONAL_COMMITTED_ARGUMENTS = {"gfsim_root_sequence_id"}


@dataclass(frozen=True, slots=True)
class PerfettoTraceLimits:
    max_document_bytes: int = 1 << 26
    max_line_bytes: int = 1 << 20
    max_event_count: int = 1 << 20


class PerfettoTraceError(ValueError):
    """A stable Perfetto packer diagnostic."""

    def __init__(self, code: str, message: str, *, line: int | None = None) -> None:
        self.code = code
        self.line = line
        location = f" line {line}" if line is not None else ""
        super().__init__(f"{code}{location}: {message}")


def _fail(code: str, message: str, *, line: int | None = None) -> NoReturn:
    raise PerfettoTraceError(code, message, line=line)


def _check_limits(limits: PerfettoTraceLimits) -> None:
    for name in limits.__dataclass_fields__:
        value = getattr(limits, name)
        if type(value) is not int or value < 0:
            _fail("ACPERFETTO-LIMIT", f"{name} must be a non-negative integer")


def _pairs(line: int):
    def reject_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in result:
                _fail(
                    "ACPERFETTO-JSON",
                    f"duplicate object member {key!r}",
                    line=line,
                )
            result[key] = value
        return result

    return reject_duplicates


def _exact_object(
    value: object, keys: set[str], *, line: int, description: str
) -> dict[str, object]:
    if type(value) is not dict or set(value) != keys:
        _fail(
            "ACPERFETTO-SCHEMA",
            f"{description} has unknown or missing fields",
            line=line,
        )
    return value


def _safe_uint(value: object, *, line: int, name: str) -> int:
    if type(value) is not int or not 0 <= value <= _SAFE_INTEGER_MAX:
        _fail(
            "ACPERFETTO-SCHEMA",
            f"{name} must be an unsigned portable I-JSON integer",
            line=line,
        )
    return value


def _name(value: object, *, line: int, field: str) -> str:
    if type(value) is not str or not _NAME.fullmatch(value):
        _fail("ACPERFETTO-SCHEMA", f"{field} is not canonical", line=line)
    return value


def _unicode_string(value: object, *, line: int, field: str) -> str:
    if type(value) is not str:
        _fail("ACPERFETTO-SCHEMA", f"{field} must be a string", line=line)
    try:
        value.encode("utf-8", errors="strict")
    except UnicodeEncodeError:
        _fail(
            "ACPERFETTO-SCHEMA",
            f"{field} contains a non-Unicode scalar value",
            line=line,
        )
    return value


def _metadata(value: dict[str, object], *, line: int) -> tuple[str, int | None]:
    name = value.get("name")
    if name == "process_name":
        metadata = _exact_object(
            value,
            {"name", "ph", "pid", "args"},
            line=line,
            description="process metadata",
        )
        thread = None
    elif name == "thread_name":
        metadata = _exact_object(
            value,
            {"name", "ph", "pid", "tid", "args"},
            line=line,
            description="thread metadata",
        )
        thread = _safe_uint(metadata["tid"], line=line, name="tid")
    else:
        _fail("ACPERFETTO-SCHEMA", "metadata name is unsupported", line=line)
    if (
        metadata["ph"] != "M"
        or type(metadata["pid"]) is not int
        or metadata["pid"] != 0
    ):
        _fail("ACPERFETTO-SCHEMA", "metadata identity is invalid", line=line)
    arguments = _exact_object(
        metadata["args"], {"name"}, line=line, description="metadata arguments"
    )
    display_name = _unicode_string(
        arguments["name"], line=line, field="metadata display name"
    )
    if not display_name:
        _fail("ACPERFETTO-SCHEMA", "metadata display name is empty", line=line)
    return name, thread


def _argument_value(value: object, *, line: int, name: str) -> JsonValue:
    if type(value) is bool:
        return value
    if type(value) is str:
        return _unicode_string(value, line=line, field=f"argument {name!r}")
    if type(value) is int and -_SAFE_INTEGER_MAX <= value <= _SAFE_INTEGER_MAX:
        return value
    _fail(
        "ACPERFETTO-SCHEMA",
        f"argument {name!r} is not a runtime scalar",
        line=line,
    )


def _committed_event(
    value: dict[str, object], *, line: int
) -> tuple[int, int, int, int]:
    phase = value.get("ph")
    phase_fields = {
        "i": {"s"},
        "X": {"dur"},
        "C": set(),
        "s": {"id"},
        "f": {"id", "bp"},
    }
    if type(phase) is not str or phase not in phase_fields:
        _fail("ACPERFETTO-SCHEMA", "event phase is unsupported", line=line)
    fields = {"name", "cat", "ph", "ts", "pid", "tid", "args"}
    fields.update(phase_fields[phase])
    event = _exact_object(value, fields, line=line, description="event")
    _name(event["name"], line=line, field="name")
    _name(event["cat"], line=line, field="category")
    if type(event["pid"]) is not int or event["pid"] != 0:
        _fail("ACPERFETTO-SCHEMA", "runtime event pid must be zero", line=line)
    timestamp = _safe_uint(event["ts"], line=line, name="ts")
    thread = _safe_uint(event["tid"], line=line, name="tid")
    arguments = event["args"]
    if type(arguments) is not dict:
        _fail("ACPERFETTO-SCHEMA", "event args must be an object", line=line)
    keys = set(arguments)
    if not _COMMITTED_ARGUMENTS.issubset(keys):
        _fail("ACPERFETTO-SCHEMA", "committed identity is incomplete", line=line)
    reserved = {key for key in keys if key.startswith("gfsim_")}
    if not reserved.issubset(_COMMITTED_ARGUMENTS | _OPTIONAL_COMMITTED_ARGUMENTS):
        _fail("ACPERFETTO-SCHEMA", "unknown runtime argument", line=line)
    for name, argument in arguments.items():
        _name(name, line=line, field="argument name")
        _argument_value(argument, line=line, name=name)

    time = _safe_uint(arguments["gfsim_epoch_time"], line=line, name="epoch time")
    delta = _safe_uint(arguments["gfsim_epoch_delta"], line=line, name="epoch delta")
    owner = _safe_uint(arguments["gfsim_object_id"], line=line, name="object id")
    local_index = _safe_uint(
        arguments["gfsim_local_committed_index"],
        line=line,
        name="owner-local committed index",
    )
    if (
        delta >= _MAX_DELTA
        or owner != thread
        or time > (_SAFE_INTEGER_MAX - delta) // _MAX_DELTA
        or timestamp != time * _MAX_DELTA + delta
    ):
        _fail(
            "ACPERFETTO-SCHEMA",
            "runtime timestamp or object identity is inconsistent",
            line=line,
        )
    if "gfsim_root_sequence_id" in arguments:
        _safe_uint(
            arguments["gfsim_root_sequence_id"],
            line=line,
            name="root sequence id",
        )

    if phase == "i" and event["s"] != "t":
        _fail("ACPERFETTO-SCHEMA", "instant scope must be thread", line=line)
    if phase == "X":
        _safe_uint(event["dur"], line=line, name="duration")
    if phase in {"s", "f"}:
        _safe_uint(event["id"], line=line, name="flow id")
    if phase == "f" and event["bp"] != "e":
        _fail("ACPERFETTO-SCHEMA", "flow end binding point is invalid", line=line)
    return time, delta, owner, local_index


def pack_event_jsonl(
    data: bytes, limits: PerfettoTraceLimits = PerfettoTraceLimits()
) -> bytes:
    """Validate runtime event JSONL and return canonical Perfetto JSON bytes."""

    _check_limits(limits)
    if type(data) is not bytes:
        _fail("ACPERFETTO-SCHEMA", "source must be bytes")
    if len(data) > limits.max_document_bytes:
        _fail("ACPERFETTO-LIMIT", "document byte limit exceeded")
    try:
        text = data.decode("utf-8", errors="strict")
    except UnicodeDecodeError as error:
        _fail("ACPERFETTO-JSON", f"source is not UTF-8 at byte {error.start}")

    events: list[JsonValue] = []
    previous: tuple[int, int, int, int] | None = None
    timeline_started = False
    process_seen = False
    thread_ids: set[int] = set()
    for line_number, line in enumerate(text.splitlines(), 1):
        if not line.strip():
            _fail("ACPERFETTO-JSON", "blank JSONL record", line=line_number)
        if len(line.encode("utf-8")) > limits.max_line_bytes:
            _fail("ACPERFETTO-LIMIT", "line byte limit exceeded", line=line_number)
        if len(events) >= limits.max_event_count:
            _fail("ACPERFETTO-LIMIT", "event count limit exceeded", line=line_number)
        try:
            value = json.loads(
                line,
                object_pairs_hook=_pairs(line_number),
                parse_constant=lambda token: _fail(
                    "ACPERFETTO-JSON",
                    f"non-finite JSON number {token}",
                    line=line_number,
                ),
            )
        except PerfettoTraceError:
            raise
        except (json.JSONDecodeError, RecursionError) as error:
            _fail("ACPERFETTO-JSON", f"invalid JSON: {error}", line=line_number)
        if type(value) is not dict:
            _fail(
                "ACPERFETTO-SCHEMA",
                "event record must be an object",
                line=line_number,
            )

        if value.get("ph") == "M":
            if timeline_started:
                _fail(
                    "ACPERFETTO-ORDER",
                    "metadata must precede committed events",
                    line=line_number,
                )
            kind, thread_id = _metadata(value, line=line_number)
            if kind == "process_name":
                if process_seen:
                    _fail(
                        "ACPERFETTO-ORDER",
                        "duplicate process metadata",
                        line=line_number,
                    )
                process_seen = True
            else:
                assert thread_id is not None
                if thread_id in thread_ids:
                    _fail(
                        "ACPERFETTO-ORDER",
                        "duplicate thread metadata",
                        line=line_number,
                    )
                thread_ids.add(thread_id)
        else:
            timeline_started = True
            key = _committed_event(value, line=line_number)
            if previous is not None and previous >= key:
                _fail(
                    "ACPERFETTO-ORDER",
                    "committed event keys are not strictly increasing",
                    line=line_number,
                )
            previous = key
        events.append(value)
    return canonical_json_bytes({"traceEvents": events}) + b"\n"


def _regular_input(path: Path) -> bytes:
    try:
        if path.is_symlink() or not path.is_file():
            raise OSError("input is not a regular file")
        return path.read_bytes()
    except OSError as error:
        _fail("ACPERFETTO-IO", f"cannot read input: {error}")


def _fsync_directory(path: Path) -> None:
    descriptor = os.open(path, os.O_RDONLY)
    try:
        try:
            os.fsync(descriptor)
        except OSError as error:
            if error.errno not in (errno.EINVAL, errno.ENOTSUP):
                raise
    finally:
        os.close(descriptor)


def publish_perfetto_trace(
    input_path: Path,
    output_path: Path,
    *,
    limits: PerfettoTraceLimits = PerfettoTraceLimits(),
) -> None:
    """Pack one regular input and atomically publish one canonical file."""

    source = input_path.absolute()
    destination = output_path.absolute()
    try:
        source_identity = source.resolve(strict=True)
        destination_identity = destination.resolve(strict=False)
    except OSError as error:
        _fail("ACPERFETTO-IO", f"cannot resolve path: {error}")
    if source_identity == destination_identity:
        _fail("ACPERFETTO-IO", "input and output must be different files")
    parent = destination.parent
    if parent.is_symlink() or not parent.is_dir():
        _fail("ACPERFETTO-IO", "output parent must be an existing directory")
    if destination.exists() and (destination.is_symlink() or not destination.is_file()):
        _fail("ACPERFETTO-IO", "existing output must be a regular file")

    output = pack_event_jsonl(_regular_input(source), limits)
    descriptor = -1
    stage: Path | None = None
    try:
        descriptor, stage_name = tempfile.mkstemp(
            prefix=f".{destination.name}.", suffix=".tmp", dir=parent
        )
        stage = Path(stage_name)
        os.fchmod(descriptor, 0o644)
        with os.fdopen(descriptor, "wb") as stream:
            descriptor = -1
            stream.write(output)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(stage, destination)
        stage = None
        _fsync_directory(parent)
    except OSError as error:
        _fail("ACPERFETTO-IO", f"cannot publish output: {error}")
    finally:
        if descriptor >= 0:
            os.close(descriptor)
        if stage is not None:
            try:
                stage.unlink(missing_ok=True)
            except OSError:
                pass

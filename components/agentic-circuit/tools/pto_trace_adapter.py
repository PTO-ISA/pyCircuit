"""Strict DavinciOO JSONL to canonical PTO trace conversion.

This module is repository tooling, not part of the installed public package.
The accepted source shape is pinned by the workspace design document.
"""

from __future__ import annotations

import hashlib
import errno
import json
import os
import re
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import NoReturn

from agentic_circuit._canonical_json import (
    JsonValue,
    canonical_json_bytes,
    sha256_bytes,
    validate_ijson_value,
)


DAVINCIOO_PRODUCER = "davincioo@e73633301cabed0d871ea5ff66e76a91df870aeb"
PTO_IDENTITY = "pto-isa@f6d0567c1cae2d6a7b0ebaf7ad0e3b93f8a39da3"
DATA_LAYOUT = "davincioo-tile-address-v1"

_SAFE_INTEGER_MAX = (1 << 53) - 1
_UINT64_MAX = (1 << 64) - 1
_RECORD_KEYS = {
    "block_idx",
    "sequence_id",
    "opcode",
    "input_tiles",
    "scalar_inputs",
    "output_tiles",
}
_TILE_KEYS = {"address", "shape", "layout", "dtype"}
_SCALAR_KEYS = {"dtype", "value"}
_OPCODE = re.compile(r"^[A-Za-z_][A-Za-z0-9_.]*$")
_ADDRESS = re.compile(r"^0x(?:0|[1-9a-f][0-9a-f]*)$")


@dataclass(frozen=True, slots=True)
class AdapterLimits:
    max_document_bytes: int = 1 << 24
    max_line_bytes: int = 1 << 20
    max_record_count: int = 65536
    max_tiles_per_record: int = 4096
    max_scalars_per_record: int = 4096
    max_shape_rank: int = 64
    max_string_bytes: int = 1 << 18


class AdapterError(ValueError):
    """A stable adapter diagnostic."""

    def __init__(
        self, code: str, message: str, *, line: int | None = None, pointer: str = ""
    ) -> None:
        self.code = code
        self.line = line
        self.pointer = pointer
        location = ""
        if line is not None:
            location += f" line {line}"
        if pointer:
            location += f" {pointer}"
        super().__init__(f"{code}{location}: {message}")


def _fail(
    code: str, message: str, *, line: int | None = None, pointer: str = ""
) -> NoReturn:
    raise AdapterError(code, message, line=line, pointer=pointer)


def _check_limits(limits: AdapterLimits) -> None:
    for name in limits.__dataclass_fields__:
        value = getattr(limits, name)
        if type(value) is not int or value < 0:
            _fail("ACTRACE-ADAPTER-LIMIT", f"{name} must be a non-negative integer")


def _pairs(line_number: int):
    def reject_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in result:
                _fail(
                    "ACTRACE-ADAPTER-JSON",
                    f"duplicate object member {key!r}",
                    line=line_number,
                )
            result[key] = value
        return result

    return reject_duplicates


def _exact_object(
    value: object, keys: set[str], *, line: int, pointer: str
) -> dict[str, object]:
    if type(value) is not dict:
        _fail(
            "ACTRACE-ADAPTER-SCHEMA",
            "expected an object",
            line=line,
            pointer=pointer,
        )
    if set(value) != keys:
        _fail(
            "ACTRACE-ADAPTER-SCHEMA",
            "object has unknown or missing fields",
            line=line,
            pointer=pointer,
        )
    return value


def _safe_uint(value: object, *, line: int, pointer: str, sequence: bool = False) -> int:
    code = "ACTRACE-ADAPTER-SEQUENCE" if sequence else "ACTRACE-ADAPTER-SCHEMA"
    if type(value) is not int or not 0 <= value <= _SAFE_INTEGER_MAX:
        _fail(
            code,
            "expected an unsigned portable I-JSON integer",
            line=line,
            pointer=pointer,
        )
    return value


def _string(
    value: object,
    limits: AdapterLimits,
    *,
    line: int,
    pointer: str,
    allow_empty: bool = False,
) -> str:
    if type(value) is not str or (not allow_empty and not value):
        _fail(
            "ACTRACE-ADAPTER-SCHEMA",
            "expected a non-empty string" if not allow_empty else "expected a string",
            line=line,
            pointer=pointer,
        )
    try:
        byte_count = len(value.encode("utf-8", errors="strict"))
    except UnicodeEncodeError:
        _fail(
            "ACTRACE-ADAPTER-JSON",
            "string contains a non-Unicode scalar value",
            line=line,
            pointer=pointer,
        )
    if byte_count > limits.max_string_bytes:
        _fail(
            "ACTRACE-ADAPTER-LIMIT",
            "string byte limit exceeded",
            line=line,
            pointer=pointer,
        )
    return value


def _tile(
    value: object, limits: AdapterLimits, *, line: int, pointer: str
) -> dict[str, JsonValue]:
    tile = _exact_object(value, _TILE_KEYS, line=line, pointer=pointer)
    address = _string(tile["address"], limits, line=line, pointer=pointer + "/address")
    if not _ADDRESS.fullmatch(address):
        _fail(
            "ACTRACE-ADAPTER-SCHEMA",
            "address must be canonical lowercase hexadecimal",
            line=line,
            pointer=pointer + "/address",
        )
    if int(address[2:], 16) > _UINT64_MAX:
        _fail(
            "ACTRACE-ADAPTER-SCHEMA",
            "address exceeds unsigned 64-bit range",
            line=line,
            pointer=pointer + "/address",
        )
    shape = tile["shape"]
    if type(shape) is not list:
        _fail(
            "ACTRACE-ADAPTER-SCHEMA",
            "shape must be an array",
            line=line,
            pointer=pointer + "/shape",
        )
    if len(shape) > limits.max_shape_rank:
        _fail(
            "ACTRACE-ADAPTER-LIMIT",
            "shape rank limit exceeded",
            line=line,
            pointer=pointer + "/shape",
        )
    dimensions: list[int] = []
    for index, dimension in enumerate(shape):
        decoded = _safe_uint(
            dimension, line=line, pointer=f"{pointer}/shape/{index}"
        )
        if decoded == 0:
            _fail(
                "ACTRACE-ADAPTER-SCHEMA",
                "shape dimensions must be positive",
                line=line,
                pointer=f"{pointer}/shape/{index}",
            )
        dimensions.append(decoded)
    return {
        "address": address,
        "shape": dimensions,
        "layout": _string(
            tile["layout"], limits, line=line, pointer=pointer + "/layout"
        ),
        "dtype": _string(
            tile["dtype"], limits, line=line, pointer=pointer + "/dtype"
        ),
    }


def _scalar(
    value: object, limits: AdapterLimits, *, line: int, pointer: str
) -> dict[str, JsonValue]:
    scalar = _exact_object(value, _SCALAR_KEYS, line=line, pointer=pointer)
    return {
        "dtype": _string(
            scalar["dtype"], limits, line=line, pointer=pointer + "/dtype"
        ),
        "value": _string(
            scalar["value"],
            limits,
            line=line,
            pointer=pointer + "/value",
            allow_empty=True,
        ),
    }


def _array(
    value: object,
    *,
    maximum: int,
    line: int,
    pointer: str,
    parser: object,
) -> list[dict[str, JsonValue]]:
    if type(value) is not list:
        _fail(
            "ACTRACE-ADAPTER-SCHEMA",
            "expected an array",
            line=line,
            pointer=pointer,
        )
    if len(value) > maximum:
        _fail(
            "ACTRACE-ADAPTER-LIMIT",
            "array element limit exceeded",
            line=line,
            pointer=pointer,
        )
    return [
        parser(item, line=line, pointer=f"{pointer}/{index}")
        for index, item in enumerate(value)
    ]


def _record(value: object, limits: AdapterLimits, line: int) -> dict[str, JsonValue]:
    source = _exact_object(value, _RECORD_KEYS, line=line, pointer="")
    block_idx = _safe_uint(source["block_idx"], line=line, pointer="/block_idx")
    sequence_id = _safe_uint(
        source["sequence_id"], line=line, pointer="/sequence_id", sequence=True
    )
    opcode = _string(source["opcode"], limits, line=line, pointer="/opcode")
    if not _OPCODE.fullmatch(opcode):
        _fail(
            "ACTRACE-ADAPTER-SCHEMA",
            "opcode is not a canonical PTO spelling",
            line=line,
            pointer="/opcode",
        )

    def parse_tile(item, **where):
        return _tile(item, limits, **where)

    def parse_scalar(item, **where):
        return _scalar(item, limits, **where)
    input_tiles = _array(
        source["input_tiles"],
        maximum=limits.max_tiles_per_record,
        line=line,
        pointer="/input_tiles",
        parser=parse_tile,
    )
    scalar_inputs = _array(
        source["scalar_inputs"],
        maximum=limits.max_scalars_per_record,
        line=line,
        pointer="/scalar_inputs",
        parser=parse_scalar,
    )
    output_tiles = _array(
        source["output_tiles"],
        maximum=limits.max_tiles_per_record,
        line=line,
        pointer="/output_tiles",
        parser=parse_tile,
    )
    return {
        "block_idx": block_idx,
        "sequence_id": sequence_id,
        "opcode": opcode,
        "input_tiles": input_tiles,
        "scalar_inputs": scalar_inputs,
        "output_tiles": output_tiles,
    }


def parse_davincioo_jsonl(
    data: bytes, limits: AdapterLimits = AdapterLimits()
) -> tuple[dict[str, JsonValue], ...]:
    """Parse and validate the pinned DavinciOO JSONL record shape."""

    _check_limits(limits)
    if type(data) is not bytes:
        _fail("ACTRACE-ADAPTER-SCHEMA", "source must be bytes")
    if len(data) > limits.max_document_bytes:
        _fail("ACTRACE-ADAPTER-LIMIT", "document byte limit exceeded")
    try:
        text = data.decode("utf-8", errors="strict")
    except UnicodeDecodeError as error:
        _fail(
            "ACTRACE-ADAPTER-JSON",
            f"source is not UTF-8 at byte {error.start}",
        )

    records: list[dict[str, JsonValue]] = []
    for line_number, line in enumerate(text.splitlines(), 1):
        if not line.strip():
            continue
        if len(line.encode("utf-8")) > limits.max_line_bytes:
            _fail(
                "ACTRACE-ADAPTER-LIMIT",
                "line byte limit exceeded",
                line=line_number,
            )
        if len(records) >= limits.max_record_count:
            _fail(
                "ACTRACE-ADAPTER-LIMIT",
                "record count limit exceeded",
                line=line_number,
            )
        try:
            value = json.loads(
                line,
                object_pairs_hook=_pairs(line_number),
                parse_constant=lambda token: _fail(
                    "ACTRACE-ADAPTER-JSON",
                    f"non-finite JSON number {token}",
                    line=line_number,
                ),
            )
        except AdapterError:
            raise
        except (json.JSONDecodeError, RecursionError) as error:
            _fail(
                "ACTRACE-ADAPTER-JSON",
                f"invalid JSON: {error}",
                line=line_number,
            )
        records.append(_record(value, limits, line_number))
    return tuple(records)


def _tile_operand(block_idx: int, tile: dict[str, JsonValue]) -> dict[str, JsonValue]:
    return {
        "kind": "tile",
        "id": f"block/{block_idx}/tile/{tile['address']}",
    }


def _canonical_record(source: dict[str, JsonValue]) -> dict[str, JsonValue]:
    block_idx = source["block_idx"]
    input_tiles = source["input_tiles"]
    scalar_inputs = source["scalar_inputs"]
    output_tiles = source["output_tiles"]
    assert type(block_idx) is int
    assert type(input_tiles) is list
    assert type(scalar_inputs) is list
    assert type(output_tiles) is list

    operands: list[JsonValue] = []
    operands.extend(_tile_operand(block_idx, tile) for tile in input_tiles)
    operands.extend(
        {"kind": "immediate", "type": scalar["dtype"], "value": scalar["value"]}
        for scalar in scalar_inputs
    )
    operands.extend(_tile_operand(block_idx, tile) for tile in output_tiles)
    roles = (
        ["input_tile"] * len(input_tiles)
        + ["scalar_input"] * len(scalar_inputs)
        + ["output_tile"] * len(output_tiles)
    )
    return {
        "sequence_id": source["sequence_id"],
        "opcode": source["opcode"],
        "operands": operands,
        "dependencies": [],
        "attributes": {
            "davincioo": {
                "block_idx": block_idx,
                "input_tiles": input_tiles,
                "scalar_inputs": scalar_inputs,
                "output_tiles": output_tiles,
                "operand_roles": roles,
            }
        },
    }


def _validate_target(document: dict[str, JsonValue]) -> None:
    if set(document) != {"schema", "version", "contract_epoch", "metadata", "records"}:
        _fail("ACTRACE-ADAPTER-TARGET", "canonical trace envelope is not closed")
    if (
        document["schema"] != "pto-trace"
        or document["version"] != "0.1"
        or document["contract_epoch"] != "0.4"
        or type(document["metadata"]) is not dict
        or type(document["records"]) is not list
    ):
        _fail("ACTRACE-ADAPTER-TARGET", "canonical trace identity is invalid")
    try:
        validate_ijson_value(document)
    except ValueError as error:
        _fail("ACTRACE-ADAPTER-TARGET", str(error))


def convert_davincioo_trace(
    data: bytes,
    *,
    source_program: str | None = None,
    limits: AdapterLimits = AdapterLimits(),
) -> bytes:
    """Return one newline-terminated canonical `pto-trace@0.1` document."""

    if source_program is not None and (type(source_program) is not str or not source_program):
        _fail(
            "ACTRACE-ADAPTER-SCHEMA",
            "source_program must be a non-empty string",
            pointer="/metadata/source_program",
        )
    sources = parse_davincioo_jsonl(data, limits)
    previous: int | None = None
    records: list[JsonValue] = []
    for line_number, source in enumerate(sources, 1):
        sequence_id = source["sequence_id"]
        assert type(sequence_id) is int
        if previous is not None and sequence_id <= previous:
            _fail(
                "ACTRACE-ADAPTER-SEQUENCE",
                "sequence_id values must be unique and strictly increasing",
                line=line_number,
                pointer="/sequence_id",
            )
        previous = sequence_id
        records.append(_canonical_record(source))

    content_hash = sha256_bytes(canonical_json_bytes(records))
    identity = source_program or "sha256:" + hashlib.sha256(data).hexdigest()
    document: dict[str, JsonValue] = {
        "schema": "pto-trace",
        "version": "0.1",
        "contract_epoch": "0.4",
        "metadata": {
            "producer": DAVINCIOO_PRODUCER,
            "pto_identity": PTO_IDENTITY,
            "source_program": identity,
            "data_layout": DATA_LAYOUT,
            "record_count": len(records),
            "content_hash": content_hash,
        },
        "records": records,
    }
    _validate_target(document)
    return canonical_json_bytes(document) + b"\n"


def _regular_input(path: Path) -> bytes:
    try:
        if path.is_symlink() or not path.is_file():
            raise OSError("input is not a regular file")
        return path.read_bytes()
    except OSError as error:
        _fail("ACTRACE-ADAPTER-IO", f"cannot read input: {error}")


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


def publish_davincioo_trace(
    input_path: Path,
    output_path: Path,
    *,
    source_program: str | None = None,
    limits: AdapterLimits = AdapterLimits(),
) -> None:
    """Convert one regular input and atomically publish one canonical file."""

    source = input_path.absolute()
    destination = output_path.absolute()
    try:
        source_identity = source.resolve(strict=True)
    except OSError as error:
        _fail("ACTRACE-ADAPTER-IO", f"cannot resolve input: {error}")
    try:
        destination_identity = destination.resolve(strict=False)
    except OSError as error:
        _fail("ACTRACE-ADAPTER-IO", f"cannot resolve output: {error}")
    if source_identity == destination_identity:
        _fail("ACTRACE-ADAPTER-IO", "input and output must be different files")
    parent = destination.parent
    if parent.is_symlink() or not parent.is_dir():
        _fail("ACTRACE-ADAPTER-IO", "output parent must be an existing directory")
    if destination.exists() and (destination.is_symlink() or not destination.is_file()):
        _fail("ACTRACE-ADAPTER-IO", "existing output must be a regular file")

    output = convert_davincioo_trace(
        _regular_input(source), source_program=source_program, limits=limits
    )
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
        _fail("ACTRACE-ADAPTER-IO", f"cannot publish output: {error}")
    finally:
        if descriptor >= 0:
            os.close(descriptor)
        if stage is not None:
            try:
                stage.unlink(missing_ok=True)
            except OSError:
                pass

"""Strict PTO execution-payload ABI descriptor and canonical codec."""

from __future__ import annotations

import json
import math
import re
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import NoReturn

from agentic_circuit._canonical_json import canonical_json_bytes, sha256_bytes

ROOT = Path(__file__).resolve().parents[3]
ABI_PATH = ROOT / "schemas/agentic-circuit/pto-payload-abi.json"

PACKED_BITS = 1258
PACKED_BYTES = 158
PADDING_BITS = PACKED_BYTES * 8 - PACKED_BITS
MAX_SHAPE_RANK = 5
MAX_INPUT_TILES = 4
MAX_SCALAR_INPUTS = 4
MAX_OUTPUT_TILES = 2

ENGINE_KIND = {"scalar": 0, "vector": 1, "cube": 2, "tma": 3}
DTYPE = {
    name: index
    for index, name in enumerate(
        (
            "u8",
            "u16",
            "u32",
            "u64",
            "i8",
            "i16",
            "i32",
            "i64",
            "fp16",
            "bf16",
            "fp32",
            "fp64",
        )
    )
}
TILE_LAYOUT = {"ND": 0, "DN": 1, "NZ": 2, "ZN": 3}
OPCODE_ID = {
    "TASSIGN": 0,
    "TLOAD": 1,
    "TROWMAX": 2,
    "TROWEXPANDSUB": 3,
    "TEXP": 4,
    "TROWSUM": 5,
    "TROWEXPANDDIV": 6,
    "TSTORE": 7,
}

_PATH_PART = re.compile(r"([A-Za-z_][A-Za-z0-9_]*)(?:\[([0-9]+)\])?")


class PayloadABIError(ValueError):
    """Stable PTO payload ABI diagnostic."""

    def __init__(self, code: str, message: str) -> None:
        self.code = code
        super().__init__(f"{code}: {message}")


def _fail(code: str, message: str) -> NoReturn:
    raise PayloadABIError(code, message)


@dataclass(frozen=True, slots=True)
class Field:
    path: str
    width: int
    lsb: int
    ownership: str = "architectural"

    def document(self) -> dict[str, object]:
        return {
            "path": self.path,
            "width": self.width,
            "lsb": self.lsb,
            "ownership": self.ownership,
        }


def _declared_leaves() -> tuple[tuple[str, int, str], ...]:
    leaves: list[tuple[str, int, str]] = [
        ("opcode_id", 16, "architectural"),
        ("engine_kind", 2, "execution"),
        ("sequence_id", 16, "execution"),
        ("block_id", 16, "execution"),
        ("input_tile_count", 3, "architectural"),
    ]

    def tile(prefix: str) -> None:
        leaves.extend(
            (
                (f"{prefix}.present", 1, "architectural"),
                (f"{prefix}.address", 64, "architectural"),
                (f"{prefix}.dtype", 4, "architectural"),
                (f"{prefix}.layout", 2, "architectural"),
                (f"{prefix}.shape.rank", 3, "architectural"),
            )
        )
        leaves.extend(
            (f"{prefix}.shape.dimensions[{index}]", 16, "architectural")
            for index in range(MAX_SHAPE_RANK)
        )

    for index in range(MAX_INPUT_TILES):
        tile(f"input_tiles[{index}]")
    leaves.append(("scalar_input_count", 3, "architectural"))
    for index in range(MAX_SCALAR_INPUTS):
        leaves.extend(
            (
                (f"scalar_inputs[{index}].present", 1, "architectural"),
                (f"scalar_inputs[{index}].dtype", 4, "architectural"),
                (f"scalar_inputs[{index}].bits", 64, "architectural"),
            )
        )
    leaves.append(("output_tile_count", 2, "architectural"))
    for index in range(MAX_OUTPUT_TILES):
        tile(f"output_tiles[{index}]")
    return tuple(leaves)


def fields() -> tuple[Field, ...]:
    cursor = PACKED_BITS
    result: list[Field] = []
    for path, width, ownership in _declared_leaves():
        cursor -= width
        result.append(Field(path, width, cursor, ownership))
    if cursor != 0:
        raise AssertionError(f"PTO payload width mismatch: {cursor}")
    return tuple(result)


FIELDS = fields()


def _layout_descriptor() -> dict[str, object]:
    return {
        "packed_bits": PACKED_BITS,
        "packed_bytes": PACKED_BYTES,
        "aggregate_order": "declaration-msb-first",
        "byte_order": "little",
        "bit_order": "lsb0",
        "padding": {"side": "msb", "zero_bits": PADDING_BITS},
        "bounds": {
            "shape_rank": MAX_SHAPE_RANK,
            "input_tiles": MAX_INPUT_TILES,
            "scalar_inputs": MAX_SCALAR_INPUTS,
            "output_tiles": MAX_OUTPUT_TILES,
        },
        "catalogs": {
            "engine_kind": ENGINE_KIND,
            "dtype": DTYPE,
            "tile_layout": TILE_LAYOUT,
            "opcode_id": OPCODE_ID,
        },
        "fields": [field.document() for field in FIELDS],
        "provider_local_state": {"serialized": False, "fields": []},
    }


def canonical_abi_document() -> dict[str, object]:
    layout = _layout_descriptor()
    return {
        "schema": "agentic-circuit-pto-payload-abi",
        "version": "0.1",
        "contract_epoch": "0.5",
        "layout_fingerprint": sha256_bytes(canonical_json_bytes(layout)),
        **layout,
    }


def _exact_dict(value: object, keys: set[str], label: str) -> dict[str, object]:
    if type(value) is not dict or set(value) != keys:
        _fail("ACABI-SCHEMA", f"{label} has unknown or missing fields")
    return value


def validate_abi(value: object) -> dict[str, object]:
    expected = canonical_abi_document()
    if value != expected:
        _fail("ACABI-LAYOUT", "ABI document differs from the canonical v0.1 layout")
    return expected


def load_abi(path: Path = ABI_PATH) -> dict[str, object]:
    def reject_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in result:
                _fail("ACABI-JSON", f"duplicate object member {key!r}")
            result[key] = value
        return result

    try:
        value = json.loads(path.read_bytes(), object_pairs_hook=reject_duplicates)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as error:
        _fail("ACABI-JSON", f"cannot read ABI document: {error}")
    return validate_abi(value)


def _uint(value: object, width: int, label: str) -> int:
    if type(value) is not int or not 0 <= value < (1 << width):
        _fail("ACABI-VALUE", f"{label} must be u{width}")
    return value


def _enum(value: object, catalog: dict[str, int], label: str) -> int:
    if type(value) is not str or value not in catalog:
        _fail("ACABI-ENUM", f"{label} is not in the frozen catalog")
    return catalog[value]


def _zero_shape() -> dict[str, object]:
    return {"rank": 0, "dimensions": [0] * MAX_SHAPE_RANK}


def empty_tile() -> dict[str, object]:
    return {
        "present": False,
        "address": 0,
        "dtype": "u8",
        "layout": "ND",
        "shape": _zero_shape(),
    }


def empty_scalar() -> dict[str, object]:
    return {"present": False, "dtype": "u8", "bits": 0}


def _shape(value: object, *, present: bool, label: str) -> None:
    shape = _exact_dict(value, {"rank", "dimensions"}, label)
    rank = _uint(shape["rank"], 3, f"{label}.rank")
    if rank > MAX_SHAPE_RANK:
        _fail("ACABI-SHAPE", f"{label}.rank exceeds the static bound")
    dimensions = shape["dimensions"]
    if type(dimensions) is not list or len(dimensions) != MAX_SHAPE_RANK:
        _fail("ACABI-SHAPE", f"{label}.dimensions must contain five values")
    for index, dimension in enumerate(dimensions):
        decoded = _uint(dimension, 16, f"{label}.dimensions[{index}]")
        if index < rank and decoded == 0:
            _fail("ACABI-SHAPE", f"{label} active dimension is zero")
        if index >= rank and decoded != 0:
            _fail("ACABI-SHAPE", f"{label} unused dimension is nonzero")
    if present and rank == 0:
        _fail("ACABI-SHAPE", f"{label} present tile must have nonzero rank")
    if not present and rank != 0:
        _fail("ACABI-UNUSED", f"{label} absent tile shape is nonzero")


def _tile(value: object, *, expected_present: bool, label: str) -> None:
    tile = _exact_dict(value, {"present", "address", "dtype", "layout", "shape"}, label)
    present = tile["present"]
    if type(present) is not bool or present != expected_present:
        _fail("ACABI-PRESENCE", f"{label}.present differs from its count prefix")
    address = _uint(tile["address"], 64, f"{label}.address")
    dtype = _enum(tile["dtype"], DTYPE, f"{label}.dtype")
    layout = _enum(tile["layout"], TILE_LAYOUT, f"{label}.layout")
    _shape(tile["shape"], present=present, label=f"{label}.shape")
    if not present and (address != 0 or dtype != 0 or layout != 0):
        _fail("ACABI-UNUSED", f"{label} absent slot is not canonical zero")


def _scalar(value: object, *, expected_present: bool, label: str) -> None:
    scalar = _exact_dict(value, {"present", "dtype", "bits"}, label)
    present = scalar["present"]
    if type(present) is not bool or present != expected_present:
        _fail("ACABI-PRESENCE", f"{label}.present differs from its count prefix")
    dtype = _enum(scalar["dtype"], DTYPE, f"{label}.dtype")
    bits = _uint(scalar["bits"], 64, f"{label}.bits")
    if not present and (dtype != 0 or bits != 0):
        _fail("ACABI-UNUSED", f"{label} absent slot is not canonical zero")


def validate_payload(value: object) -> dict[str, object]:
    payload = _exact_dict(
        value,
        {
            "opcode_id",
            "engine_kind",
            "sequence_id",
            "block_id",
            "input_tile_count",
            "input_tiles",
            "scalar_input_count",
            "scalar_inputs",
            "output_tile_count",
            "output_tiles",
        },
        "payload",
    )
    opcode = _uint(payload["opcode_id"], 16, "opcode_id")
    if opcode not in OPCODE_ID.values():
        _fail("ACABI-ENUM", "opcode_id is reserved in the current profile")
    _enum(payload["engine_kind"], ENGINE_KIND, "engine_kind")
    _uint(payload["sequence_id"], 16, "sequence_id")
    _uint(payload["block_id"], 16, "block_id")

    for count_name, slots_name, limit, parser in (
        ("input_tile_count", "input_tiles", MAX_INPUT_TILES, _tile),
        ("scalar_input_count", "scalar_inputs", MAX_SCALAR_INPUTS, _scalar),
        ("output_tile_count", "output_tiles", MAX_OUTPUT_TILES, _tile),
    ):
        count = payload[count_name]
        if type(count) is not int or not 0 <= count <= limit:
            _fail("ACABI-COUNT", f"{count_name} exceeds its static bound")
        slots = payload[slots_name]
        if type(slots) is not list or len(slots) != limit:
            _fail("ACABI-COUNT", f"{slots_name} must contain exactly {limit} slots")
        for index, slot in enumerate(slots):
            parser(slot, expected_present=index < count, label=f"{slots_name}[{index}]")
    return payload


def _get_path(value: object, path: str) -> object:
    current = value
    for raw in path.split("."):
        match = _PATH_PART.fullmatch(raw)
        if match is None or type(current) is not dict:
            raise AssertionError(f"invalid ABI path {path}")
        current = current[match.group(1)]
        if match.group(2) is not None:
            if type(current) is not list:
                raise AssertionError(f"invalid ABI array path {path}")
            current = current[int(match.group(2))]
    return current


def flatten_payload(value: object) -> dict[str, int]:
    payload = validate_payload(value)
    result: dict[str, int] = {}
    for field in FIELDS:
        raw = _get_path(payload, field.path)
        if field.path.endswith(".present"):
            encoded = int(raw)
        elif field.path == "engine_kind":
            encoded = ENGINE_KIND[raw]
        elif field.path.endswith(".dtype"):
            encoded = DTYPE[raw]
        elif field.path.endswith(".layout"):
            encoded = TILE_LAYOUT[raw]
        else:
            encoded = raw
        result[field.path] = _uint(encoded, field.width, field.path)
    return result


def pack_payload(value: object) -> bytes:
    flat = flatten_payload(value)
    packed = 0
    for field in FIELDS:
        packed |= flat[field.path] << field.lsb
    return packed.to_bytes(PACKED_BYTES, "little")


def _reverse(catalog: dict[str, int], value: int, label: str) -> str:
    for name, encoding in catalog.items():
        if encoding == value:
            return name
    _fail("ACABI-ENUM", f"{label} uses a reserved encoding")


def _flat_from_bytes(data: object) -> dict[str, int]:
    if type(data) is not bytes or len(data) != PACKED_BYTES:
        _fail("ACABI-SERIALIZATION", f"payload must contain {PACKED_BYTES} bytes")
    packed = int.from_bytes(data, "little")
    if packed >> PACKED_BITS:
        _fail("ACABI-PADDING", "most-significant padding bits must be zero")
    return {
        field.path: (packed >> field.lsb) & ((1 << field.width) - 1) for field in FIELDS
    }


def _decode_shape(flat: dict[str, int], prefix: str) -> dict[str, object]:
    return {
        "rank": flat[f"{prefix}.rank"],
        "dimensions": [
            flat[f"{prefix}.dimensions[{index}]"] for index in range(MAX_SHAPE_RANK)
        ],
    }


def _decode_tile(flat: dict[str, int], prefix: str) -> dict[str, object]:
    return {
        "present": bool(flat[f"{prefix}.present"]),
        "address": flat[f"{prefix}.address"],
        "dtype": _reverse(DTYPE, flat[f"{prefix}.dtype"], f"{prefix}.dtype"),
        "layout": _reverse(TILE_LAYOUT, flat[f"{prefix}.layout"], f"{prefix}.layout"),
        "shape": _decode_shape(flat, f"{prefix}.shape"),
    }


def _decode_scalar(flat: dict[str, int], prefix: str) -> dict[str, object]:
    return {
        "present": bool(flat[f"{prefix}.present"]),
        "dtype": _reverse(DTYPE, flat[f"{prefix}.dtype"], f"{prefix}.dtype"),
        "bits": flat[f"{prefix}.bits"],
    }


def unpack_payload(data: object) -> dict[str, object]:
    flat = _flat_from_bytes(data)
    payload: dict[str, object] = {
        "opcode_id": flat["opcode_id"],
        "engine_kind": _reverse(ENGINE_KIND, flat["engine_kind"], "engine_kind"),
        "sequence_id": flat["sequence_id"],
        "block_id": flat["block_id"],
        "input_tile_count": flat["input_tile_count"],
        "input_tiles": [
            _decode_tile(flat, f"input_tiles[{index}]")
            for index in range(MAX_INPUT_TILES)
        ],
        "scalar_input_count": flat["scalar_input_count"],
        "scalar_inputs": [
            _decode_scalar(flat, f"scalar_inputs[{index}]")
            for index in range(MAX_SCALAR_INPUTS)
        ],
        "output_tile_count": flat["output_tile_count"],
        "output_tiles": [
            _decode_tile(flat, f"output_tiles[{index}]")
            for index in range(MAX_OUTPUT_TILES)
        ],
    }
    return validate_payload(payload)


def canonical_payload_json(value: object) -> bytes:
    return canonical_json_bytes(validate_payload(value))


def _project_tile(value: object) -> dict[str, object]:
    tile = _exact_dict(value, {"address", "shape", "layout", "dtype"}, "trace tile")
    shape = tile["shape"]
    if type(shape) is not list or len(shape) > MAX_SHAPE_RANK:
        _fail("ACABI-PROJECTION", "trace tile shape exceeds the ABI rank")
    dtype_aliases = {"uint64": "u64", "float32": "fp32"}
    dtype = dtype_aliases.get(tile["dtype"], tile["dtype"])
    address = tile["address"]
    if type(address) is not str or not address.startswith("0x"):
        _fail("ACABI-PROJECTION", "trace tile address is not canonical hexadecimal")
    return {
        "present": True,
        "address": int(address, 16),
        "dtype": dtype,
        "layout": tile["layout"],
        "shape": {
            "rank": len(shape),
            "dimensions": [*shape, *([0] * (MAX_SHAPE_RANK - len(shape)))],
        },
    }


def _project_scalar_bits(dtype: str, value: object) -> int:
    if type(value) is not str:
        _fail("ACABI-PROJECTION", "trace scalar value must be a string")
    integer_widths = {
        "u8": (8, False),
        "u16": (16, False),
        "u32": (32, False),
        "u64": (64, False),
        "i8": (8, True),
        "i16": (16, True),
        "i32": (32, True),
        "i64": (64, True),
    }
    if dtype in integer_widths:
        width, signed = integer_widths[dtype]
        try:
            decoded = int(value, 10)
        except ValueError:
            _fail("ACABI-PROJECTION", "trace scalar is not a decimal integer")
        lower = -(1 << (width - 1)) if signed else 0
        upper = (1 << (width - (1 if signed else 0))) - 1
        if not lower <= decoded <= upper:
            _fail("ACABI-PROJECTION", f"trace scalar does not fit {dtype}")
        return decoded & ((1 << width) - 1)

    try:
        decoded_float = float(value)
    except ValueError:
        _fail("ACABI-PROJECTION", "trace scalar is not a decimal float")
    if not math.isfinite(decoded_float):
        _fail("ACABI-PROJECTION", "trace scalar must be finite")
    try:
        if dtype == "fp16":
            return int.from_bytes(struct.pack("<e", decoded_float), "little")
        if dtype == "fp32":
            return int.from_bytes(struct.pack("<f", decoded_float), "little")
        if dtype == "fp64":
            return int.from_bytes(struct.pack("<d", decoded_float), "little")
        if dtype == "bf16":
            fp32 = int.from_bytes(struct.pack("<f", decoded_float), "little")
            return (fp32 + 0x7FFF + ((fp32 >> 16) & 1)) >> 16
    except (OverflowError, struct.error):
        _fail("ACABI-PROJECTION", f"trace scalar does not fit {dtype}")
    _fail("ACABI-PROJECTION", "trace scalar dtype is unsupported")


def project_davincioo_record(
    value: object,
    *,
    opcode_ids: dict[str, int],
    routes: dict[str, int],
) -> dict[str, object]:
    record = _exact_dict(
        value,
        {
            "block_idx",
            "sequence_id",
            "opcode",
            "input_tiles",
            "scalar_inputs",
            "output_tiles",
        },
        "DavinciOO record",
    )
    opcode = record["opcode"]
    if opcode_ids != OPCODE_ID or type(opcode) is not str or opcode not in opcode_ids:
        _fail("ACABI-PROJECTION", "opcode catalog differs from the ABI profile")
    if set(routes) != set(OPCODE_ID) or routes[opcode] not in range(4):
        _fail("ACABI-PROJECTION", "engine route catalog differs from the ABI profile")
    input_tiles = [_project_tile(item) for item in record["input_tiles"]]
    output_tiles = [_project_tile(item) for item in record["output_tiles"]]
    if len(input_tiles) > MAX_INPUT_TILES or len(output_tiles) > MAX_OUTPUT_TILES:
        _fail("ACABI-PROJECTION", "tile operand count exceeds the ABI profile")
    scalar_inputs: list[dict[str, object]] = []
    for raw in record["scalar_inputs"]:
        scalar = _exact_dict(raw, {"dtype", "value"}, "trace scalar")
        dtype_aliases = {
            "uint8": "u8",
            "uint16": "u16",
            "uint32": "u32",
            "uint64": "u64",
            "int8": "i8",
            "int16": "i16",
            "int32": "i32",
            "int64": "i64",
            "float16": "fp16",
            "bfloat16": "bf16",
            "float32": "fp32",
            "float64": "fp64",
        }
        dtype = dtype_aliases.get(scalar["dtype"], scalar["dtype"])
        if type(dtype) is not str or dtype not in DTYPE:
            _fail("ACABI-PROJECTION", "trace scalar dtype is unsupported")
        bits = _project_scalar_bits(dtype, scalar["value"])
        scalar_inputs.append({"present": True, "dtype": dtype, "bits": bits})
    if len(scalar_inputs) > MAX_SCALAR_INPUTS:
        _fail("ACABI-PROJECTION", "scalar operand count exceeds the ABI profile")
    engine_names = tuple(ENGINE_KIND)
    payload = {
        "opcode_id": opcode_ids[opcode],
        "engine_kind": engine_names[routes[opcode]],
        "sequence_id": record["sequence_id"],
        "block_id": record["block_idx"],
        "input_tile_count": len(input_tiles),
        "input_tiles": [
            *input_tiles,
            *(empty_tile() for _ in range(MAX_INPUT_TILES - len(input_tiles))),
        ],
        "scalar_input_count": len(scalar_inputs),
        "scalar_inputs": [
            *scalar_inputs,
            *(empty_scalar() for _ in range(MAX_SCALAR_INPUTS - len(scalar_inputs))),
        ],
        "output_tile_count": len(output_tiles),
        "output_tiles": [
            *output_tiles,
            *(empty_tile() for _ in range(MAX_OUTPUT_TILES - len(output_tiles))),
        ],
    }
    return validate_payload(payload)


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--write-canonical-abi", type=Path)
    arguments = parser.parse_args()
    if arguments.write_canonical_abi is None:
        parser.error("--write-canonical-abi is required")
    arguments.write_canonical_abi.write_bytes(
        canonical_json_bytes(canonical_abi_document()) + b"\n"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

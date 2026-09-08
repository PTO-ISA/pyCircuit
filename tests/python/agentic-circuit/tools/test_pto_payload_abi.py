from __future__ import annotations

import copy
import hashlib
import json
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "compiler/acir/tools"))

from pto_payload_abi import (  # noqa: E402
    FIELDS,
    PACKED_BITS,
    PACKED_BYTES,
    PayloadABIError,
    canonical_abi_document,
    canonical_payload_json,
    empty_scalar,
    empty_tile,
    flatten_payload,
    load_abi,
    pack_payload,
    project_davincioo_record,
    unpack_payload,
)
from pto_trace_adapter import parse_davincioo_jsonl  # noqa: E402

TRACE = (
    ROOT
    / "third_party/references/davincioo-gfsim/upstream/tests/fixtures/traces"
    / "examples_intermediate_softmax.pto.trace"
)
PROJECTION = ROOT / "tests/goldens/agentic-circuit/davincioo/softmax-projection.json"


def _inputs() -> tuple[tuple[dict[str, object], ...], dict[str, object]]:
    return (
        parse_davincioo_jsonl(TRACE.read_bytes()),
        json.loads(PROJECTION.read_bytes()),
    )


def _payload(index: int) -> dict[str, object]:
    records, projection = _inputs()
    return project_davincioo_record(
        records[index],
        opcode_ids=projection["opcode_ids"],
        routes=projection["routes"],
    )


def test_abi_document_is_canonical_complete_and_schema_valid() -> None:
    document = load_abi()
    assert document == canonical_abi_document()
    assert len(document["fields"]) == 79
    assert document["packed_bits"] == PACKED_BITS
    assert document["provider_local_state"] == {"serialized": False, "fields": []}
    assert document["fields"][0] == {
        "path": "opcode_id",
        "width": 16,
        "lsb": 1242,
        "ownership": "architectural",
    }
    assert document["fields"][1] == {
        "path": "engine_kind",
        "width": 2,
        "lsb": 1240,
        "ownership": "execution",
    }
    assert document["fields"][-1] == {
        "path": "output_tiles[1].shape.dimensions[4]",
        "width": 16,
        "lsb": 0,
        "ownership": "architectural",
    }

    from jsonschema.validators import Draft202012Validator

    schema = json.loads(
        (ROOT / "schemas/agentic-circuit/pto-payload-abi.schema.json").read_bytes()
    )
    Draft202012Validator(schema).validate(document)


@pytest.mark.parametrize("record_index", (0, 4))
def test_scalar_and_multi_tile_records_round_trip_canonical_bytes(
    record_index: int,
) -> None:
    payload = _payload(record_index)
    encoded = pack_payload(payload)
    assert len(encoded) == PACKED_BYTES
    assert int.from_bytes(encoded, "little") >> PACKED_BITS == 0
    assert unpack_payload(encoded) == payload
    assert canonical_payload_json(payload) == canonical_payload_json(
        dict(reversed(payload.items()))
    )

    if record_index == 0:
        assert payload["scalar_input_count"] == 1
    else:
        assert payload["input_tile_count"] == 2
        assert payload["output_tile_count"] == 1


def test_each_leaf_uses_the_published_offset_and_width() -> None:
    payload = _payload(4)
    flat = flatten_payload(payload)
    encoded = int.from_bytes(pack_payload(payload), "little")
    assert tuple(field.path for field in FIELDS) == tuple(flat)
    for field in FIELDS:
        assert (encoded >> field.lsb) & ((1 << field.width) - 1) == flat[field.path]


def test_two_input_and_two_output_tile_slots_round_trip() -> None:
    payload = copy.deepcopy(_payload(4))
    second_output = copy.deepcopy(payload["output_tiles"][0])
    second_output["address"] += 0x10000
    payload["output_tile_count"] = 2
    payload["output_tiles"][1] = second_output

    assert payload["input_tile_count"] == 2
    assert unpack_payload(pack_payload(payload)) == payload


@pytest.mark.parametrize(
    ("dtype", "value", "expected_bits"),
    (
        ("int8", "-1", 0xFF),
        ("float32", "1.5", 0x3FC00000),
        ("bfloat16", "1.5", 0x3FC0),
    ),
)
def test_davincioo_projection_encodes_signed_and_float_scalar_bits(
    dtype: str, value: str, expected_bits: int
) -> None:
    records, projection = _inputs()
    record = copy.deepcopy(records[0])
    record["scalar_inputs"] = [{"dtype": dtype, "value": value}]
    payload = project_davincioo_record(
        record,
        opcode_ids=projection["opcode_ids"],
        routes=projection["routes"],
    )

    assert payload["scalar_inputs"][0]["bits"] == expected_bits
    assert unpack_payload(pack_payload(payload)) == payload


def test_davincioo_projection_rejects_scalar_dtype_outside_catalog() -> None:
    records, projection = _inputs()
    record = copy.deepcopy(records[0])
    record["scalar_inputs"] = [{"dtype": "bool", "value": "true"}]
    with pytest.raises(PayloadABIError, match="scalar dtype is unsupported"):
        project_davincioo_record(
            record,
            opcode_ids=projection["opcode_ids"],
            routes=projection["routes"],
        )


def test_serialization_golden_is_stable() -> None:
    assert hashlib.sha256(pack_payload(_payload(4))).hexdigest() == (
        "9b4bc4270e726d73aeee9c4f14c34b812f1ab27e5bfaf107713af10ec8aa80cc"
    )


@pytest.mark.parametrize(
    ("mutation", "code"),
    (
        (lambda value: value.update(provider_cycle=3), "ACABI-SCHEMA"),
        (lambda value: value.update(opcode_id=65535), "ACABI-ENUM"),
        (lambda value: value.update(engine_kind="tensor"), "ACABI-ENUM"),
        (lambda value: value.update(input_tile_count=5), "ACABI-COUNT"),
        (
            lambda value: value["input_tiles"][0]["shape"].update(rank=0),
            "ACABI-SHAPE",
        ),
        (
            lambda value: value["input_tiles"][0]["shape"].update(
                rank=6, dimensions=[1, 1, 1, 1, 1]
            ),
            "ACABI-SHAPE",
        ),
        (
            lambda value: value["input_tiles"][2].update(present=True),
            "ACABI-PRESENCE",
        ),
        (
            lambda value: value["input_tiles"][2].update(address=1),
            "ACABI-UNUSED",
        ),
    ),
)
def test_payload_validation_fails_closed(mutation, code: str) -> None:
    malformed = copy.deepcopy(_payload(4))
    mutation(malformed)
    with pytest.raises(PayloadABIError, match=code):
        pack_payload(malformed)


def test_deserialization_rejects_size_padding_and_reserved_catalog_values() -> None:
    encoded = pack_payload(_payload(4))
    with pytest.raises(PayloadABIError, match="ACABI-SERIALIZATION"):
        unpack_payload(encoded[:-1])

    bad_padding = bytearray(encoded)
    bad_padding[-1] |= 0x80
    with pytest.raises(PayloadABIError, match="ACABI-PADDING"):
        unpack_payload(bytes(bad_padding))

    dtype = next(field for field in FIELDS if field.path == "input_tiles[0].dtype")
    reserved = int.from_bytes(encoded, "little")
    reserved &= ~(((1 << dtype.width) - 1) << dtype.lsb)
    reserved |= 15 << dtype.lsb
    with pytest.raises(PayloadABIError, match="ACABI-ENUM"):
        unpack_payload(reserved.to_bytes(PACKED_BYTES, "little"))

    rank = next(
        field for field in FIELDS if field.path == "input_tiles[0].shape.rank"
    )
    excessive_rank = int.from_bytes(encoded, "little")
    excessive_rank &= ~(((1 << rank.width) - 1) << rank.lsb)
    excessive_rank |= 6 << rank.lsb
    with pytest.raises(PayloadABIError, match="ACABI-SHAPE"):
        unpack_payload(excessive_rank.to_bytes(PACKED_BYTES, "little"))


def test_unused_slot_helpers_are_canonical_zero() -> None:
    assert empty_tile() == {
        "present": False,
        "address": 0,
        "dtype": "u8",
        "layout": "ND",
        "shape": {"rank": 0, "dimensions": [0, 0, 0, 0, 0]},
    }
    assert empty_scalar() == {"present": False, "dtype": "u8", "bits": 0}


def test_abi_loader_rejects_duplicate_or_mutated_layout(tmp_path: Path) -> None:
    duplicate = tmp_path / "duplicate.json"
    duplicate.write_text('{"schema":"first","schema":"second"}')
    with pytest.raises(PayloadABIError, match="ACABI-JSON"):
        load_abi(duplicate)

    mutated = canonical_abi_document()
    mutated["fields"][0]["lsb"] -= 1
    path = tmp_path / "mutated.json"
    path.write_text(json.dumps(mutated))
    with pytest.raises(PayloadABIError, match="ACABI-LAYOUT"):
        load_abi(path)

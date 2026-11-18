"""Interface stability: the artifacts other tools consume.

These tests fail when a schema name, a required field, or an exported symbol
changes, because the route table and the CLI JSON are consumed by the compiler
boundary and by CI gates.
"""

from __future__ import annotations

import json

import pytest

import wse_model
from wse_model.calendar.table import CalendarRouteTable
from wse_model.fixtures import ffn_example

pytestmark = pytest.mark.contract

SCHEMA = "wse-model/calendar-route-table/1"


# -- package surface -------------------------------------------------------


def test_package_exports_are_stable() -> None:
    for name in (
        "MeshTopology",
        "TopologyProfile",
        "topology_profile",
        "CALENDAR_BASELINE",
        "WHITEPAPER_HARDWARE",
        "__version__",
    ):
        assert hasattr(wse_model, name), name
    assert wse_model.__version__ == "0.1.0"


def test_calendar_package_exports_are_stable() -> None:
    from wse_model import calendar

    for name in (
        "RouteBits",
        "BitPair",
        "CalendarRouteEntry",
        "CalendarRouteTable",
        "Collective",
        "RedOp",
        "RouteKey",
        "CalendarKeyRef",
        "CalendarKeyRegistry",
        "EpochCounter",
        "PayloadGeometry",
        "SymmetricArena",
        "ReceiveAccount",
        "validate_route_bits",
    ):
        assert hasattr(calendar, name), name


def test_noc_package_exports_are_stable() -> None:
    from wse_model import noc

    for name in (
        "Noc",
        "SendPlan",
        "SendOutcome",
        "FlitHeader",
        "CalRegImage",
        "CalRegSlot",
        "simulate_multicast",
    ):
        assert hasattr(noc, name), name


# -- route table schema ----------------------------------------------------


def test_route_table_schema_name_is_frozen() -> None:
    assert ffn_example().table.to_dict()["schema"] == SCHEMA


def test_route_table_required_fields_are_present() -> None:
    payload = ffn_example().table.to_dict()
    for field in (
        "schema",
        "topology",
        "layout",
        "versions",
        "key_count",
        "table_bytes",
        "d_cache_lines_per_core",
        "keys",
    ):
        assert field in payload, field
    for field in ("topologyVersion", "calendarVersion", "routeVersion"):
        assert field in payload["versions"], field
    for key in payload["keys"]:
        for field in (
            "key_id",
            "route_key",
            "node_count",
            "member_nodes",
            "groups",
            "self_delivery",
            "arm_lead_cycles",
            "conflict_proof",
            "geometry",
            "entries",
        ):
            assert field in key, field
        for entry in key["entries"]:
            for field in ("node", "route_bits", "land_count", "flags", "expected_rx_bytes"):
                assert field in entry, field


def test_route_table_round_trips_byte_for_byte() -> None:
    table = ffn_example().table
    blob = json.dumps(table.to_dict())
    rebuilt = CalendarRouteTable.from_dict(json.loads(blob))
    assert rebuilt.to_bytes() == table.to_bytes()
    assert rebuilt.layout is table.layout
    assert rebuilt.route_version == table.route_version
    assert rebuilt.topology_version == table.topology_version
    assert rebuilt.calendar_version == table.calendar_version


def test_route_table_round_trip_preserves_every_entry_field() -> None:
    table = ffn_example().table
    rebuilt = CalendarRouteTable.from_dict(json.loads(json.dumps(table.to_dict())))
    for key_id in (0, 1):
        for node in range(table.topology.node_count):
            original = table.entry(key_id, node)
            copy = rebuilt.entry(key_id, node)
            assert copy.route_bits == original.route_bits
            assert copy.expected_rx_bytes == original.expected_rx_bytes
            assert copy.flags == original.flags
            assert copy.land_count == original.land_count


def test_route_table_json_is_deterministic() -> None:
    first = json.dumps(ffn_example().table.to_dict(), sort_keys=True)
    second = json.dumps(ffn_example().table.to_dict(), sort_keys=True)
    assert first == second


def test_route_table_rejects_an_unknown_schema() -> None:
    payload = ffn_example().table.to_dict()
    payload["schema"] = "wse-model/calendar-route-table/2"
    from wse_model.errors import CalendarError

    with pytest.raises(CalendarError):
        CalendarRouteTable.from_dict(payload)


def test_both_layouts_emit_the_same_bytes_in_a_different_order() -> None:
    from wse_model.calendar.table import TableLayout

    key_major = ffn_example(layout=TableLayout.KEY_MAJOR).table
    node_major = ffn_example(layout=TableLayout.NODE_MAJOR).table
    assert key_major.table_bytes == node_major.table_bytes
    assert key_major.d_cache_lines_per_core == 2
    assert node_major.d_cache_lines_per_core == 1
    # The same set of 16 B entries, ordered differently.
    key_entries = sorted(
        key_major.to_bytes()[i : i + 16] for i in range(0, key_major.table_bytes, 16)
    )
    node_entries = sorted(
        node_major.to_bytes()[i : i + 16] for i in range(0, node_major.table_bytes, 16)
    )
    assert key_entries == node_entries


# -- CLI JSON contract -----------------------------------------------------


def test_cli_json_top_level_keys_are_stable(capsys) -> None:
    from wse_model.cli import main

    assert main(["--json", "run", "ffn-allgather"]) == 0
    payload = json.loads(capsys.readouterr().out)
    for field in ("scenario", "table", "phases", "all_complete"):
        assert field in payload, field
    for field in (
        "phase",
        "key_id",
        "opcode",
        "epoch",
        "members",
        "member_count",
        "ok",
        "states",
        "total_wire_bytes",
        "total_payload_bytes",
        "useful_payload_fraction",
    ):
        assert field in payload["phases"][0], field


# -- machine-readable schema ----------------------------------------------


def test_emitted_table_validates_against_the_published_schema() -> None:
    """`schemas/calendar-route-table.schema.json` is part of the contract."""
    jsonschema = pytest.importorskip("jsonschema")
    from pathlib import Path

    schema_path = (
        Path(__file__).resolve().parents[2] / "schemas" / "calendar-route-table.schema.json"
    )
    schema = json.loads(schema_path.read_text(encoding="utf-8"))
    validator = jsonschema.Draft202012Validator(schema)
    errors = sorted(validator.iter_errors(ffn_example().table.to_dict()), key=str)
    assert not errors, "\n".join(error.message for error in errors)


def test_schema_rejects_a_table_with_a_wrong_schema_name() -> None:
    jsonschema = pytest.importorskip("jsonschema")
    from pathlib import Path

    schema_path = (
        Path(__file__).resolve().parents[2] / "schemas" / "calendar-route-table.schema.json"
    )
    schema = json.loads(schema_path.read_text(encoding="utf-8"))
    payload = ffn_example().table.to_dict()
    payload["schema"] = "wse-model/calendar-route-table/2"
    assert list(jsonschema.Draft202012Validator(schema).iter_errors(payload))


def test_schema_rejects_an_entry_with_a_malformed_bitmap() -> None:
    jsonschema = pytest.importorskip("jsonschema")
    from pathlib import Path

    schema_path = (
        Path(__file__).resolve().parents[2] / "schemas" / "calendar-route-table.schema.json"
    )
    schema = json.loads(schema_path.read_text(encoding="utf-8"))
    payload = ffn_example().table.to_dict()
    payload["keys"][0]["entries"][0]["route_bits"] = "not-hex"
    assert list(jsonschema.Draft202012Validator(schema).iter_errors(payload))

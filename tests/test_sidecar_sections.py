from __future__ import annotations

from pycircuit.schedule_ir import schedule_ir_to_sidecar_bytes
from pycircuit.sidecar_sections import (
    SectionKind,
    default_section_registry,
    inspect_sidecar_file,
    render_sidecar_inspect_text,
    verify_schedule_ir_for_sidecar,
)


def _sidecar_schedule_ir() -> dict:
    return {
        "schema": "pycircuit.tb.schedule_ir",
        "version": {"major": 1, "minor": 0, "patch": 0},
        "metadata": {
            "module_name": "ToyReadyValid",
            "schedule_mode": "sidecar",
        },
        "timebase": {
            "reset_cycles": 0,
            "max_cycle": 8,
        },
        "ports": [
            {
                "id": 0,
                "name": "cmd_valid",
                "direction": "input",
                "role": "drive",
                "bit_width": 1,
                "word_count": 1,
                "protocol": "ready_valid",
            },
            {
                "id": 1,
                "name": "cmd_data",
                "direction": "input",
                "role": "drive",
                "bit_width": 16,
                "word_count": 1,
                "protocol": "payload",
            },
            {
                "id": 2,
                "name": "cmd_ready",
                "direction": "output",
                "role": "expect",
                "bit_width": 1,
                "word_count": 1,
                "protocol": "ready_valid",
            },
            {
                "id": 3,
                "name": "result_data",
                "direction": "output",
                "role": "expect",
                "bit_width": 16,
                "word_count": 1,
                "protocol": "payload",
            },
        ],
        "events": [
            {
                "cycle": 1,
                "kind": "expect",
                "phase": "post",
                "port": 2,
                "value": "0x1",
                "message": "cmd_ready is high",
            },
            {
                "cycle": 2,
                "kind": "expect",
                "phase": "post",
                "port": 3,
                "value": "0x1234",
                "message": "payload pass-through",
            },
        ],
        "frames": [
            {
                "cycle": 1,
                "kind": "drive",
                "phase": "pre",
                "items": [
                    {
                        "port": 0,
                        "value": "0x1",
                        "message": "cmd_valid",
                    },
                    {
                        "port": 1,
                        "value": "0x1234",
                        "message": "cmd_data",
                    },
                ],
            }
        ],
        "patterns": [
            {
                "kind": "periodic_drive",
                "port": 0,
                "start_cycle": 3,
                "end_cycle": 8,
                "period": 2,
                "active_cycles": 1,
                "phase_cycle": 3,
                "active_value": "0x1",
                "default_value": "0x0",
            }
        ],
        "stats": {
            "port_count": 4,
            "event_count": 2,
            "frame_count": 1,
            "pattern_count": 1,
            "max_words": 1,
        },
    }


def test_sidecar_registry_has_only_sidecar_sections() -> None:
    registry = default_section_registry()
    descriptors = registry.descriptors()
    names = {section.name for section in descriptors}
    kinds = {section.kind for section in descriptors}

    assert names == {
        "string_table",
        "port_table",
        "event_table",
        "frame_table",
        "pattern_table",
    }
    assert kinds == {
        SectionKind.STRING_TABLE,
        SectionKind.PORT_TABLE,
        SectionKind.EVENT_TABLE,
        SectionKind.FRAME_TABLE,
        SectionKind.PATTERN_TABLE,
    }


def test_sidecar_sidecar_round_trip(tmp_path) -> None:
    schedule_ir = _sidecar_schedule_ir()
    errors = verify_schedule_ir_for_sidecar(schedule_ir)
    assert errors == []

    schedule_path = tmp_path / "schedule.sidecar.bin"
    schedule_path.write_bytes(schedule_ir_to_sidecar_bytes(schedule_ir))

    inspected = inspect_sidecar_file(schedule_path)
    assert inspected["header"]["major"] == 1
    assert inspected["header"]["minor"] == 0
    assert inspected["header"]["max_cycle"] == 8
    sections = inspected["summary"]["sections_by_name"]
    assert sections["port_table"]["count"] == 4
    assert sections["event_table"]["count"] == 2
    assert sections["frame_table"]["count"] == 1
    assert sections["pattern_table"]["count"] == 1

    decoded = inspected["decoded"]
    assert decoded["ports"][0]["name"] == "cmd_valid"
    assert decoded["events"][0]["message"] == "cmd_ready is high"
    assert decoded["frames"][0]["items"][1]["message"] == "cmd_data"
    assert decoded["patterns"][0]["kind"] == "periodic_drive"

    rendered = render_sidecar_inspect_text(inspected)
    assert "SIDECAR file" in rendered
    assert "pattern_table" in rendered


def test_sidecar_verifier_rejects_unknown_port_reference() -> None:
    schedule_ir = _sidecar_schedule_ir()
    schedule_ir["events"][0]["port"] = 99

    errors = verify_schedule_ir_for_sidecar(schedule_ir)
    assert any("references missing port id: 99" in error for error in errors)

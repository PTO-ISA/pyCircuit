from __future__ import annotations

import struct
from collections.abc import Callable, Iterable, Mapping, Sequence
from dataclasses import dataclass
from pathlib import Path
from typing import Any

from .sidecar_sections import (
    PackedSection,
    SectionKind,
    SectionRegistry,
    default_section_registry,
    verify_schedule_ir_for_sidecar,
)


def infer_port_role(name: str, ty: str) -> str:
    nm = str(name).lower()
    if ty == "!pyc.clock" or nm in {"clk", "clock"} or nm.endswith("_clk"):
        return "clock"
    if (
        ty == "!pyc.reset"
        or nm in {"rst", "reset"}
        or nm.endswith("_rst")
        or nm.endswith("_reset")
    ):
        return "reset"
    if nm == "valid" or nm.endswith("_valid"):
        return "valid"
    if nm == "ready" or nm.endswith("_ready"):
        return "ready"
    if nm == "tag" or nm.endswith("_tag"):
        return "tag"
    if nm == "data" or nm.endswith("_data") or nm.endswith("_payload"):
        return "data"
    return "control"


def infer_port_protocol(name: str) -> str | None:
    nm = str(name)
    for suffix in ("_valid", "_ready", "_data", "_payload", "_tag"):
        if nm.endswith(suffix) and len(nm) > len(suffix):
            return nm[: -len(suffix)]
    return None


def _value_from_words(words: Sequence[int]) -> int:
    value = 0
    for idx, word in enumerate(words):
        value |= int(word) << (64 * idx)
    return value


def _expect_events_to_ir(
    *,
    phase: str,
    rows: Iterable[tuple[int, int, str, int, list[int], str]],
) -> list[dict[str, Any]]:
    events: list[dict[str, Any]] = []
    for cyc, pid, _sn, _w, words, msg in rows:
        events.append(
            {
                "kind": "expect",
                "cycle": int(cyc),
                "phase": phase,
                "port": int(pid),
                "value": f"0x{_value_from_words(words):x}",
                "message": str(msg),
            }
        )
    return events


def _drive_frames_to_ir(
    *,
    drive_ports: Sequence[tuple[int, str, int]],
    drive_frame_rows: Iterable[tuple[int, list[int], list[list[int]]]],
) -> list[dict[str, Any]]:
    frames: list[dict[str, Any]] = []
    for cyc, masks, values in drive_frame_rows:
        items: list[dict[str, Any]] = []
        for slot, (pid, _sn, _w) in enumerate(drive_ports):
            if ((int(masks[slot // 64]) >> (slot % 64)) & 1) == 0:
                continue
            items.append(
                {"port": int(pid), "value": f"0x{_value_from_words(values[slot]):x}"}
            )
        frames.append({"kind": "drive_frame", "cycle": int(cyc), "items": items})
    return frames


def _hex_to_int(value: Any) -> int | None:
    if value is None:
        return None
    if isinstance(value, int):
        return int(value)
    text = str(value)
    return int(text, 16) if text.startswith(("0x", "0X")) else int(text)


def _detect_periodic_drive_patterns(
    *,
    ports: Sequence[Mapping[str, Any]],
    frames: Sequence[Mapping[str, Any]],
) -> list[dict[str, Any]]:
    port_by_id = {int(port["id"]): port for port in ports}
    samples_by_port: dict[int, list[tuple[int, int]]] = {}
    for frame in frames:
        cyc = int(frame["cycle"])
        for item in frame.get("items", []):
            if not isinstance(item, Mapping):
                continue
            pid = int(item["port"])
            port = port_by_id.get(pid)
            if port is None:
                continue
            if (
                str(port.get("direction")) != "input"
                or str(port.get("role")) != "ready"
            ):
                continue
            value = _hex_to_int(item.get("value"))
            if value not in {0, 1}:
                continue
            samples_by_port.setdefault(pid, []).append((cyc, int(value)))

    patterns: list[dict[str, Any]] = []

    def zero_runs(candidate: Sequence[tuple[int, int]]) -> list[tuple[int, int]]:
        runs: list[tuple[int, int]] = []
        run_start: int | None = None
        last_cycle: int | None = None
        for cyc, value in candidate:
            if value == 0 and run_start is None:
                run_start = cyc
            if value != 0 and run_start is not None:
                runs.append((run_start, int(last_cycle) + 1))
                run_start = None
            last_cycle = cyc
        if run_start is not None and last_cycle is not None:
            runs.append((run_start, last_cycle + 1))
        return runs

    for pid, samples in sorted(samples_by_port.items()):
        if len(samples) < 16:
            continue
        samples = sorted(samples)
        detected: tuple[int, int, int, int, int, list[tuple[int, int]]] | None = None
        for drop_prefix in range(0, min(4, len(samples) - 16) + 1):
            candidate = samples[drop_prefix:]
            start = candidate[0][0]
            end = candidate[-1][0] + 1
            if len(candidate) != end - start:
                continue
            runs = zero_runs(candidate)
            if len(runs) < 2:
                continue
            ref_run_idx = 1 if len(runs) >= 3 else 0
            period = runs[ref_run_idx + 1][0] - runs[ref_run_idx][0]
            active_cycles = runs[ref_run_idx][1] - runs[ref_run_idx][0]
            if period <= 1 or active_cycles <= 0 or active_cycles >= period:
                continue
            phase_cycle = runs[ref_run_idx][0] % period
            if all(
                value == (0 if ((cyc - phase_cycle) % period) < active_cycles else 1)
                for cyc, value in candidate
            ):
                detected = (period, active_cycles, phase_cycle, start, end, candidate)
                break
        if detected is None:
            continue
        period, active_cycles, phase_cycle, start, end, covered_samples = detected
        port = port_by_id[pid]
        patterns.append(
            {
                "kind": "periodic_drive",
                "name": f"{port.get('name', f'port_{pid}')}_periodic_drive",
                "port": int(pid),
                "start_cycle": int(start),
                "end_cycle": int(end),
                "period": int(period),
                "active_cycles": int(active_cycles),
                "phase_cycle": int(phase_cycle),
                "active_value": "0x0",
                "default_value": "0x1",
                "source": "detected_annotation",
                "covered_samples": len(covered_samples),
                "ignored_setup_samples": len(samples) - len(covered_samples),
            }
        )
    return patterns


def build_sidecar_schedule_ir(
    *,
    top_symbol: str,
    schedule_path: Path,
    ports: Iterable[Mapping[str, Any]],
    timeout_cycles: int,
    reset_cycles: int,
    clocking: str,
    schedule_bytes: int,
    max_event_words: int,
    drive_events: Sequence[tuple[int, int, str, int, list[int]]],
    drive_ports: Sequence[tuple[int, str, int]],
    drive_frame_rows: Sequence[tuple[int, list[int], list[list[int]]]],
    pre_expect_events: Sequence[tuple[int, int, str, int, list[int], str]],
    post_expect_events: Sequence[tuple[int, int, str, int, list[int], str]],
    generate_s: float,
) -> dict[str, Any]:
    events = [
        *_expect_events_to_ir(phase="pre", rows=pre_expect_events),
        *_expect_events_to_ir(phase="post", rows=post_expect_events),
    ]
    frames = _drive_frames_to_ir(
        drive_ports=drive_ports, drive_frame_rows=drive_frame_rows
    )
    port_list = [dict(port) for port in ports]
    patterns = _detect_periodic_drive_patterns(ports=port_list, frames=frames)
    stats = {
        "event_count": len(events),
        "frame_count": len(frames),
        "pattern_count": len(patterns),
        "port_count": len(port_list),
        "max_cycle": int(timeout_cycles),
        "schedule_bytes": int(schedule_bytes),
        "generate_s": float(generate_s),
        "sidecar_source": {
            "drive_events": len(drive_events),
            "drive_frames": len(drive_frame_rows),
            "pre_expect_events": len(pre_expect_events),
            "post_expect_events": len(post_expect_events),
            "max_event_words": int(max_event_words),
        },
    }
    return {
        "schema": "pycircuit.schedule_ir",
        "version": {"major": 1, "minor": 0, "patch": 0},
        "metadata": {
            "case_name": f"tb_{top_symbol}",
            "generator": "pycircuit.sidecar",
            "generator_version": "sidecar",
            "source": str(schedule_path),
            "notes": "Generated for sidecar schedule execution.",
        },
        "timebase": {
            "unit": "cycle",
            "max_cycle": int(timeout_cycles),
            "reset_cycles": int(reset_cycles),
            "clocking": str(clocking),
        },
        "ports": sorted(port_list, key=lambda x: int(x["id"])),
        "events": sorted(
            events,
            key=lambda x: (
                int(x["cycle"]),
                str(x.get("phase", "post")),
                int(x["port"]),
            ),
        ),
        "frames": frames,
        "patterns": patterns,
        "stats": stats,
    }


_SIDECAR_MAGIC = b"SIDECAR\n"
_SIDECAR_HEADER_FMT = "<8sBHHHHQIIQII"
_SIDECAR_DIR_FMT = "<HHIQQQ"
_SIDECAR_PORT_FMT = "<IIBBHIII"
_SIDECAR_EVENT_PREFIX_FMT = "<QBBHIII"
_SIDECAR_FRAME_PREFIX_FMT = "<QBBHI"
_SIDECAR_FRAME_ITEM_PREFIX_FMT = "<IIII"
_SIDECAR_PATTERN_PREFIX_FMT = "<HHIQQQQQII"
_SIDECAR_NONE = 0xFFFFFFFF

_DIRECTION_ID = {"input": 0, "output": 1, "inout": 2}
_ROLE_ID = {
    "unknown": 0,
    "data": 1,
    "valid": 2,
    "ready": 3,
    "tag": 4,
    "control": 5,
    "clock": 6,
    "reset": 7,
}
_EVENT_KIND_ID = {"drive": 0, "expect": 1, "sample": 2, "marker": 3}
_FRAME_KIND_ID = {"drive_frame": 0, "expect_frame": 1}
_PHASE_ID = {"pre": 0, "post": 1}


class _StringTable:
    def __init__(self) -> None:
        self.ids: dict[str, int] = {}
        self.items: list[str] = []

    def add(self, value: str | None) -> int:
        if value is None or value == "":
            return _SIDECAR_NONE
        text = str(value)
        sid = self.ids.get(text)
        if sid is not None:
            return sid
        sid = len(self.items)
        self.ids[text] = sid
        self.items.append(text)
        return sid

    def to_bytes(self) -> bytes:
        blob = bytearray()
        blob.extend(len(self.items).to_bytes(4, "little", signed=False))
        for item in self.items:
            raw = item.encode("utf-8")
            blob.extend(len(raw).to_bytes(4, "little", signed=False))
            blob.extend(raw)
        return bytes(blob)


def _collect_sidecar_strings(schedule_ir: Mapping[str, Any]) -> _StringTable:
    table = _StringTable()
    for port in schedule_ir.get("ports", []):
        if isinstance(port, Mapping):
            table.add(str(port.get("name", "")))
            table.add(
                None if port.get("protocol") is None else str(port.get("protocol"))
            )
    for event in schedule_ir.get("events", []):
        if isinstance(event, Mapping):
            table.add(
                None if event.get("message") is None else str(event.get("message"))
            )
    for frame in schedule_ir.get("frames", []):
        if not isinstance(frame, Mapping):
            continue
        for item in frame.get("items", []):
            if isinstance(item, Mapping):
                table.add(
                    None if item.get("message") is None else str(item.get("message"))
                )
    return table


def _value_words(value: int | None, max_words: int) -> bytes:
    vv = 0 if value is None else int(value)
    blob = bytearray()
    for idx in range(max_words):
        blob.extend(
            ((vv >> (64 * idx)) & 0xFFFFFFFFFFFFFFFF).to_bytes(
                8, "little", signed=False
            )
        )
    return bytes(blob)


def _nwords(value: int | None) -> int:
    if value is None:
        return 0
    return max(1, (int(value).bit_length() + 63) // 64)


def _sidecar_max_words(schedule_ir: Mapping[str, Any]) -> int:
    max_words = 1
    for port in schedule_ir.get("ports", []):
        if isinstance(port, Mapping):
            max_words = max(max_words, int(port.get("word_count", 1)))
    return max_words


def _pack_sidecar_ports(schedule_ir: Mapping[str, Any], strings: _StringTable) -> bytes:
    blob = bytearray()
    for port in schedule_ir.get("ports", []):
        if not isinstance(port, Mapping):
            continue
        blob.extend(
            struct.pack(
                _SIDECAR_PORT_FMT,
                int(port["id"]),
                strings.add(str(port["name"])),
                _DIRECTION_ID.get(str(port.get("direction", "input")), 0),
                _ROLE_ID.get(str(port.get("role", "unknown")), 0),
                0,
                int(port["bit_width"]),
                int(port["word_count"]),
                strings.add(
                    None if port.get("protocol") is None else str(port.get("protocol"))
                ),
            )
        )
    return bytes(blob)


def _pack_sidecar_events(
    schedule_ir: Mapping[str, Any], strings: _StringTable, max_words: int
) -> bytes:
    blob = bytearray()
    for event in schedule_ir.get("events", []):
        if not isinstance(event, Mapping):
            continue
        value = _hex_to_int(event.get("value"))
        mask = _hex_to_int(event.get("mask"))
        port = _SIDECAR_NONE if event.get("port") is None else int(event["port"])
        blob.extend(
            struct.pack(
                _SIDECAR_EVENT_PREFIX_FMT,
                int(event["cycle"]),
                _EVENT_KIND_ID.get(str(event.get("kind", "marker")), 3),
                _PHASE_ID.get(str(event.get("phase", "post")), 1),
                0,
                port,
                _nwords(value),
                strings.add(
                    None if event.get("message") is None else str(event.get("message"))
                ),
            )
        )
        blob.extend(_value_words(value, max_words))
        blob.extend(_value_words(mask, max_words))
    return bytes(blob)


def _pack_sidecar_frames(
    schedule_ir: Mapping[str, Any], strings: _StringTable, max_words: int
) -> bytes:
    blob = bytearray()
    covered: set[tuple[int, int]] = set()
    for pattern in schedule_ir.get("patterns", []):
        if (
            not isinstance(pattern, Mapping)
            or str(pattern.get("kind")) != "periodic_drive"
        ):
            continue
        port = int(pattern["port"])
        for cycle in range(int(pattern["start_cycle"]), int(pattern["end_cycle"])):
            covered.add((cycle, port))
    for frame in schedule_ir.get("frames", []):
        if not isinstance(frame, Mapping):
            continue
        kind = str(frame.get("kind", "drive_frame"))
        cycle = int(frame["cycle"])
        items = [
            item
            for item in frame.get("items", [])
            if isinstance(item, Mapping)
            and not (kind == "drive_frame" and (cycle, int(item["port"])) in covered)
        ]
        if not items:
            continue
        blob.extend(
            struct.pack(
                _SIDECAR_FRAME_PREFIX_FMT,
                cycle,
                _FRAME_KIND_ID.get(kind, 0),
                0 if kind == "drive_frame" else 1,
                0,
                len(items),
            )
        )
        for item in items:
            value = _hex_to_int(item.get("value"))
            mask = _hex_to_int(item.get("mask"))
            blob.extend(
                struct.pack(
                    _SIDECAR_FRAME_ITEM_PREFIX_FMT,
                    int(item["port"]),
                    _nwords(value),
                    strings.add(
                        None
                        if item.get("message") is None
                        else str(item.get("message"))
                    ),
                    0,
                )
            )
            blob.extend(_value_words(value, max_words))
            blob.extend(_value_words(mask, max_words))
    return bytes(blob)


def _pack_sidecar_patterns(schedule_ir: Mapping[str, Any], max_words: int) -> bytes:
    blob = bytearray()
    for pattern in schedule_ir.get("patterns", []):
        if (
            not isinstance(pattern, Mapping)
            or str(pattern.get("kind")) != "periodic_drive"
        ):
            continue
        active = _hex_to_int(pattern.get("active_value"))
        default = _hex_to_int(pattern.get("default_value"))
        blob.extend(
            struct.pack(
                _SIDECAR_PATTERN_PREFIX_FMT,
                1,
                0,
                int(pattern["port"]),
                int(pattern["start_cycle"]),
                int(pattern["end_cycle"]),
                int(pattern["period"]),
                int(pattern["active_cycles"]),
                int(pattern.get("phase_cycle", int(pattern["start_cycle"]))),
                _nwords(active),
                _nwords(default),
            )
        )
        blob.extend(_value_words(active, max_words))
        blob.extend(_value_words(default, max_words))
    return bytes(blob)


def _sidecar_pattern_count(schedule_ir: Mapping[str, Any]) -> int:
    return sum(
        1
        for pattern in schedule_ir.get("patterns", [])
        if isinstance(pattern, Mapping) and str(pattern.get("kind")) == "periodic_drive"
    )


def _sidecar_frame_count_after_pattern_compaction(
    schedule_ir: Mapping[str, Any]
) -> int:
    frame_count = 0
    for frame in schedule_ir.get("frames", []):
        if not isinstance(frame, Mapping):
            continue
        kind = str(frame.get("kind", "drive_frame"))
        cycle = int(frame["cycle"])
        covered = {
            int(pattern["port"])
            for pattern in schedule_ir.get("patterns", [])
            if isinstance(pattern, Mapping)
            and str(pattern.get("kind")) == "periodic_drive"
            and int(pattern["start_cycle"]) <= cycle < int(pattern["end_cycle"])
        }
        items = [
            item
            for item in frame.get("items", [])
            if isinstance(item, Mapping)
            and not (kind == "drive_frame" and int(item["port"]) in covered)
        ]
        if items:
            frame_count += 1
    return frame_count


@dataclass(frozen=True)
class _SidecarSectionBuildState:
    schedule_ir: Mapping[str, Any]
    strings: _StringTable
    max_words: int
    pattern_blob: bytes
    pattern_count: int
    frame_blob: bytes
    frame_count: int


_SidecarSectionEmitResult = tuple[bytes, int] | None
_SidecarSectionEmitter = Callable[
    [_SidecarSectionBuildState], _SidecarSectionEmitResult
]


def _build_sidecar_section_state(
    schedule_ir: Mapping[str, Any], strings: _StringTable, max_words: int
) -> _SidecarSectionBuildState:
    return _SidecarSectionBuildState(
        schedule_ir=schedule_ir,
        strings=strings,
        max_words=max_words,
        pattern_blob=_pack_sidecar_patterns(schedule_ir, max_words),
        pattern_count=_sidecar_pattern_count(schedule_ir),
        frame_blob=_pack_sidecar_frames(schedule_ir, strings, max_words),
        frame_count=_sidecar_frame_count_after_pattern_compaction(schedule_ir),
    )


def _emit_string_table_section(
    state: _SidecarSectionBuildState,
) -> _SidecarSectionEmitResult:
    return state.strings.to_bytes(), len(state.strings.items)


def _emit_port_table_section(
    state: _SidecarSectionBuildState,
) -> _SidecarSectionEmitResult:
    return _pack_sidecar_ports(state.schedule_ir, state.strings), len(
        state.schedule_ir.get("ports", [])
    )


def _emit_event_table_section(
    state: _SidecarSectionBuildState,
) -> _SidecarSectionEmitResult:
    return _pack_sidecar_events(state.schedule_ir, state.strings, state.max_words), len(
        state.schedule_ir.get("events", [])
    )


def _emit_frame_table_section(
    state: _SidecarSectionBuildState,
) -> _SidecarSectionEmitResult:
    return state.frame_blob, state.frame_count


def _emit_pattern_table_section(
    state: _SidecarSectionBuildState,
) -> _SidecarSectionEmitResult:
    return (state.pattern_blob, state.pattern_count) if state.pattern_count else None


_SIDECAR_SECTION_EMITTERS: tuple[tuple[int, _SidecarSectionEmitter], ...] = (
    (int(SectionKind.STRING_TABLE), _emit_string_table_section),
    (int(SectionKind.PORT_TABLE), _emit_port_table_section),
    (int(SectionKind.EVENT_TABLE), _emit_event_table_section),
    (int(SectionKind.FRAME_TABLE), _emit_frame_table_section),
    (int(SectionKind.PATTERN_TABLE), _emit_pattern_table_section),
)


def _packed_section(
    registry: SectionRegistry,
    kind: SectionKind | int,
    data: bytes,
    count: int,
    *,
    flags: int = 1,
) -> PackedSection:
    descriptor = registry.by_kind(int(kind))
    return PackedSection(
        kind=int(kind),
        data=data,
        count=int(count),
        flags=int(flags),
        name=descriptor.name if descriptor is not None else f"unknown_{int(kind)}",
    )


def build_sidecar_section_plan(
    schedule_ir: Mapping[str, Any], strings: _StringTable, max_words: int
) -> list[PackedSection]:
    registry = default_section_registry()
    state = _build_sidecar_section_state(schedule_ir, strings, max_words)
    sections: list[PackedSection] = []
    for kind, emitter in _SIDECAR_SECTION_EMITTERS:
        result = emitter(state)
        if result is None:
            continue
        data, count = result
        sections.append(_packed_section(registry, kind, data, count))
    return sections


def schedule_ir_to_sidecar_bytes(schedule_ir: Mapping[str, Any]) -> bytes:
    validation_errors = verify_schedule_ir_for_sidecar(schedule_ir)
    if validation_errors:
        joined = "\n".join(f"- {item}" for item in validation_errors)
        raise ValueError(f"invalid SIDECAR schedule IR:\n{joined}")
    version = schedule_ir.get("version", {})
    timebase = schedule_ir.get("timebase", {})
    strings = _collect_sidecar_strings(schedule_ir)
    max_words = _sidecar_max_words(schedule_ir)
    sections = build_sidecar_section_plan(schedule_ir, strings, max_words)
    header_size = struct.calcsize(_SIDECAR_HEADER_FMT)
    dir_size = struct.calcsize(_SIDECAR_DIR_FMT)
    offset = header_size + len(sections) * dir_size
    directory = bytearray()
    payload = bytearray()
    for packed in sections:
        directory.extend(
            struct.pack(
                _SIDECAR_DIR_FMT,
                int(packed.kind),
                int(packed.flags),
                0,
                int(offset),
                len(packed.data),
                int(packed.count),
            )
        )
        payload.extend(packed.data)
        offset += len(packed.data)
    header = struct.pack(
        _SIDECAR_HEADER_FMT,
        _SIDECAR_MAGIC,
        1,
        header_size,
        int(version.get("major", 1)),
        int(version.get("minor", 0)),
        int(version.get("patch", 0)),
        0,
        len(sections),
        max_words,
        int(timebase.get("max_cycle", 0)),
        int(timebase.get("reset_cycles", 0)),
        0,
    )
    return header + bytes(directory) + bytes(payload)

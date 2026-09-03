from __future__ import annotations

import struct
from collections.abc import Iterable, Mapping
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path
from typing import Any

SIDECAR_MAGIC = b"SIDECAR\n"
SIDECAR_HEADER_FMT = "<8sBHHHHQIIQII"
SIDECAR_DIR_FMT = "<HHIQQQ"
SIDECAR_PORT_FMT = "<IIBBHIII"
SIDECAR_EVENT_PREFIX_FMT = "<QBBHIII"
SIDECAR_FRAME_PREFIX_FMT = "<QBBHI"
SIDECAR_FRAME_ITEM_PREFIX_FMT = "<IIII"
SIDECAR_PATTERN_PREFIX_FMT = "<HHIQQQQQII"
SIDECAR_HEADER_SIZE = struct.calcsize(SIDECAR_HEADER_FMT)
SIDECAR_DIR_SIZE = struct.calcsize(SIDECAR_DIR_FMT)
SIDECAR_NONE = 0xFFFFFFFF

_DIRECTION_NAME = {0: "input", 1: "output", 2: "inout"}
_ROLE_NAME = {
    0: "unknown",
    1: "data",
    2: "valid",
    3: "ready",
    4: "tag",
    5: "control",
    6: "clock",
    7: "reset",
}
_EVENT_KIND_NAME = {0: "drive", 1: "expect", 2: "sample", 3: "marker"}
_PHASE_NAME = {0: "pre", 1: "post"}
_FRAME_KIND_NAME = {0: "drive_frame", 1: "expect_frame"}
_PATTERN_KIND_NAME = {1: "periodic_drive"}


class SectionKind(IntEnum):
    STRING_TABLE = 1
    PORT_TABLE = 2
    EVENT_TABLE = 3
    FRAME_TABLE = 4
    PATTERN_TABLE = 5


@dataclass(frozen=True)
class SectionDescriptor:
    kind: int
    name: str
    major: int = 0
    minor: int = 1
    required: bool = False
    dependencies: tuple[int, ...] = ()
    runtime_tags: tuple[str, ...] = ()
    summary: str = ""


@dataclass(frozen=True)
class SectionDirectoryEntry:
    kind: int
    flags: int
    offset: int
    size: int
    count: int
    name: str
    known: bool
    required: bool


@dataclass(frozen=True)
class SidecarHeader:
    endian: int
    header_size: int
    major: int
    minor: int
    patch: int
    flags: int
    section_count: int
    max_words: int
    max_cycle: int
    reset_cycles: int
    reserved: int


@dataclass(frozen=True)
class PackedSection:
    kind: int
    data: bytes
    count: int
    flags: int = 1
    name: str = ""


class SectionRegistry:
    def __init__(self, descriptors: Iterable[SectionDescriptor] = ()) -> None:
        self._by_kind: dict[int, SectionDescriptor] = {}
        self._by_name: dict[str, SectionDescriptor] = {}
        for descriptor in descriptors:
            self.register(descriptor)

    def register(self, descriptor: SectionDescriptor) -> None:
        kind = int(descriptor.kind)
        name = str(descriptor.name)
        if kind in self._by_kind:
            raise ValueError(f"duplicate SIDECAR section kind: {kind}")
        if name in self._by_name:
            raise ValueError(f"duplicate SIDECAR section name: {name}")
        self._by_kind[kind] = descriptor
        self._by_name[name] = descriptor

    def by_kind(self, kind: int) -> SectionDescriptor | None:
        return self._by_kind.get(int(kind))

    def by_name(self, name: str) -> SectionDescriptor | None:
        return self._by_name.get(str(name))

    def descriptors(self) -> list[SectionDescriptor]:
        return [self._by_kind[kind] for kind in sorted(self._by_kind)]

    def describe_kind(self, kind: int) -> str:
        descriptor = self.by_kind(kind)
        return descriptor.name if descriptor is not None else f"unknown_{int(kind)}"

    def required_kinds(self) -> set[int]:
        return {
            kind for kind, descriptor in self._by_kind.items() if descriptor.required
        }


def default_section_registry() -> SectionRegistry:
    return SectionRegistry(
        [
            SectionDescriptor(
                kind=SectionKind.STRING_TABLE,
                name="string_table",
                required=True,
                runtime_tags=("container", "sidecar"),
                summary="Global string pool referenced by other sections.",
            ),
            SectionDescriptor(
                kind=SectionKind.PORT_TABLE,
                name="port_table",
                required=True,
                dependencies=(SectionKind.STRING_TABLE,),
                runtime_tags=("sidecar",),
                summary="DUT port ids, names, directions, widths, roles, and protocol hints.",
            ),
            SectionDescriptor(
                kind=SectionKind.EVENT_TABLE,
                name="event_table",
                required=True,
                dependencies=(SectionKind.PORT_TABLE,),
                runtime_tags=("sidecar",),
                summary="Cycle-level expect/sample events.",
            ),
            SectionDescriptor(
                kind=SectionKind.FRAME_TABLE,
                name="frame_table",
                required=True,
                dependencies=(SectionKind.PORT_TABLE,),
                runtime_tags=("sidecar",),
                summary="Cycle-level drive frames.",
            ),
            SectionDescriptor(
                kind=SectionKind.PATTERN_TABLE,
                name="pattern_table",
                dependencies=(SectionKind.PORT_TABLE,),
                runtime_tags=("sidecar",),
                summary="Compact periodic drive/backpressure patterns.",
            ),
        ]
    )


def _read_exact_header(data: bytes) -> tuple[SidecarHeader | None, list[str]]:
    if len(data) < SIDECAR_HEADER_SIZE:
        return None, [f"file too small for SIDECAR header: {len(data)} bytes"]
    unpacked = struct.unpack_from(SIDECAR_HEADER_FMT, data, 0)
    errors: list[str] = []
    if unpacked[0] != SIDECAR_MAGIC:
        errors.append("invalid SIDECAR magic")
    return (
        SidecarHeader(
            endian=int(unpacked[1]),
            header_size=int(unpacked[2]),
            major=int(unpacked[3]),
            minor=int(unpacked[4]),
            patch=int(unpacked[5]),
            flags=int(unpacked[6]),
            section_count=int(unpacked[7]),
            max_words=int(unpacked[8]),
            max_cycle=int(unpacked[9]),
            reset_cycles=int(unpacked[10]),
            reserved=int(unpacked[11]),
        ),
        errors,
    )


def _slice_section(data: bytes, section: SectionDirectoryEntry) -> bytes:
    return data[section.offset : section.offset + section.size]


def _decode_string_ref(strings: list[str], sid: int) -> str | None:
    if sid == SIDECAR_NONE:
        return None
    if 0 <= sid < len(strings):
        return strings[sid]
    return f"<bad-string-ref:{sid}>"


def _decode_string_table(blob: bytes) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    if len(blob) < 4:
        return [], ["string_table too small"]
    count = int.from_bytes(blob[0:4], "little", signed=False)
    pos = 4
    out: list[str] = []
    for idx in range(count):
        if pos + 4 > len(blob):
            errors.append(f"string_table truncated before string length index={idx}")
            break
        n = int.from_bytes(blob[pos : pos + 4], "little", signed=False)
        pos += 4
        if pos + n > len(blob):
            errors.append(f"string_table truncated string index={idx} length={n}")
            break
        out.append(blob[pos : pos + n].decode("utf-8", errors="replace"))
        pos += n
    return out, errors


def _value_from_words(words: list[int]) -> str:
    value = 0
    for idx, word in enumerate(words):
        value |= int(word) << (64 * idx)
    return f"0x{value:x}"


def _decode_ports(
    blob: bytes, count: int, strings: list[str]
) -> tuple[list[dict[str, Any]], list[str]]:
    errors: list[str] = []
    record_size = struct.calcsize(SIDECAR_PORT_FMT)
    out: list[dict[str, Any]] = []
    for idx in range(count):
        pos = idx * record_size
        if pos + record_size > len(blob):
            errors.append(f"port_table truncated at record {idx}")
            break
        (
            port_id,
            name_sid,
            direction,
            role,
            _reserved,
            bit_width,
            word_count,
            protocol_sid,
        ) = struct.unpack_from(SIDECAR_PORT_FMT, blob, pos)
        out.append(
            {
                "id": int(port_id),
                "name": _decode_string_ref(strings, int(name_sid)),
                "direction": _DIRECTION_NAME.get(
                    int(direction), f"unknown_{int(direction)}"
                ),
                "role": _ROLE_NAME.get(int(role), f"unknown_{int(role)}"),
                "bit_width": int(bit_width),
                "word_count": int(word_count),
                "protocol": _decode_string_ref(strings, int(protocol_sid)),
            }
        )
    return out, errors


def _decode_events(
    blob: bytes, count: int, strings: list[str], max_words: int
) -> tuple[list[dict[str, Any]], list[str]]:
    errors: list[str] = []
    prefix_size = struct.calcsize(SIDECAR_EVENT_PREFIX_FMT)
    record_size = prefix_size + max_words * 16
    out: list[dict[str, Any]] = []
    for idx in range(count):
        pos = idx * record_size
        if pos + record_size > len(blob):
            errors.append(f"event_table truncated at record {idx}")
            break
        cycle, kind, phase, _reserved, port, nwords, msg_sid = struct.unpack_from(
            SIDECAR_EVENT_PREFIX_FMT, blob, pos
        )
        pos += prefix_size
        words = [
            int.from_bytes(blob[pos + i * 8 : pos + i * 8 + 8], "little", signed=False)
            for i in range(max_words)
        ]
        pos += max_words * 8
        masks = [
            int.from_bytes(blob[pos + i * 8 : pos + i * 8 + 8], "little", signed=False)
            for i in range(max_words)
        ]
        out.append(
            {
                "cycle": int(cycle),
                "kind": _EVENT_KIND_NAME.get(int(kind), f"unknown_{int(kind)}"),
                "phase": _PHASE_NAME.get(int(phase), f"unknown_{int(phase)}"),
                "port": None if int(port) == SIDECAR_NONE else int(port),
                "nwords": int(nwords),
                "value": _value_from_words(words[: int(nwords)]),
                "mask": _value_from_words(masks),
                "message": _decode_string_ref(strings, int(msg_sid)),
            }
        )
    return out, errors


def _decode_frames(
    blob: bytes, count: int, strings: list[str], max_words: int
) -> tuple[list[dict[str, Any]], list[str]]:
    errors: list[str] = []
    pos = 0
    out: list[dict[str, Any]] = []
    prefix_size = struct.calcsize(SIDECAR_FRAME_PREFIX_FMT)
    item_prefix_size = struct.calcsize(SIDECAR_FRAME_ITEM_PREFIX_FMT)
    for idx in range(count):
        if pos + prefix_size > len(blob):
            errors.append(f"frame_table truncated before frame {idx}")
            break
        cycle, kind, phase, _reserved, item_count = struct.unpack_from(
            SIDECAR_FRAME_PREFIX_FMT, blob, pos
        )
        pos += prefix_size
        items: list[dict[str, Any]] = []
        for item_index in range(int(item_count)):
            if pos + item_prefix_size + max_words * 16 > len(blob):
                errors.append(f"frame_table truncated at frame={idx} item={item_index}")
                break
            port, nwords, msg_sid, _item_reserved = struct.unpack_from(
                SIDECAR_FRAME_ITEM_PREFIX_FMT, blob, pos
            )
            pos += item_prefix_size
            words = [
                int.from_bytes(
                    blob[pos + i * 8 : pos + i * 8 + 8], "little", signed=False
                )
                for i in range(max_words)
            ]
            pos += max_words * 8
            masks = [
                int.from_bytes(
                    blob[pos + i * 8 : pos + i * 8 + 8], "little", signed=False
                )
                for i in range(max_words)
            ]
            pos += max_words * 8
            items.append(
                {
                    "port": int(port),
                    "nwords": int(nwords),
                    "value": _value_from_words(words[: int(nwords)]),
                    "mask": _value_from_words(masks),
                    "message": _decode_string_ref(strings, int(msg_sid)),
                }
            )
        out.append(
            {
                "cycle": int(cycle),
                "kind": _FRAME_KIND_NAME.get(int(kind), f"unknown_{int(kind)}"),
                "phase": _PHASE_NAME.get(int(phase), f"unknown_{int(phase)}"),
                "items": items,
            }
        )
    return out, errors


def _decode_patterns(
    blob: bytes, count: int, max_words: int
) -> tuple[list[dict[str, Any]], list[str]]:
    errors: list[str] = []
    pos = 0
    out: list[dict[str, Any]] = []
    prefix_size = struct.calcsize(SIDECAR_PATTERN_PREFIX_FMT)
    for idx in range(count):
        if pos + prefix_size + max_words * 16 > len(blob):
            errors.append(f"pattern_table truncated at record {idx}")
            break
        (
            kind,
            _reserved,
            port,
            start,
            end,
            period,
            active_cycles,
            phase_cycle,
            active_nwords,
            default_nwords,
        ) = struct.unpack_from(SIDECAR_PATTERN_PREFIX_FMT, blob, pos)
        pos += prefix_size
        active_words = [
            int.from_bytes(blob[pos + i * 8 : pos + i * 8 + 8], "little", signed=False)
            for i in range(max_words)
        ]
        pos += max_words * 8
        default_words = [
            int.from_bytes(blob[pos + i * 8 : pos + i * 8 + 8], "little", signed=False)
            for i in range(max_words)
        ]
        pos += max_words * 8
        out.append(
            {
                "kind": _PATTERN_KIND_NAME.get(int(kind), f"unknown_{int(kind)}"),
                "port": int(port),
                "start_cycle": int(start),
                "end_cycle": int(end),
                "period": int(period),
                "active_cycles": int(active_cycles),
                "phase_cycle": int(phase_cycle),
                "active_value": _value_from_words(active_words[: int(active_nwords)]),
                "default_value": _value_from_words(
                    default_words[: int(default_nwords)]
                ),
            }
        )
    return out, errors


def inspect_sidecar_file(
    path: str | Path, *, registry: SectionRegistry | None = None
) -> dict[str, Any]:
    registry = default_section_registry() if registry is None else registry
    p = Path(path)
    data = p.read_bytes()
    header, errors = _read_exact_header(data)
    warnings: list[str] = []
    sections: list[SectionDirectoryEntry] = []
    if header is None:
        return {
            "path": str(p),
            "size_bytes": len(data),
            "valid": False,
            "errors": errors,
            "warnings": warnings,
            "header": None,
            "sections": [],
            "summary": {},
        }
    if header.endian != 1:
        errors.append(f"unsupported endian marker: {header.endian}")
    if header.header_size < SIDECAR_HEADER_SIZE:
        errors.append(f"header_size too small: {header.header_size}")
    directory_start = header.header_size
    directory_end = directory_start + header.section_count * SIDECAR_DIR_SIZE
    if directory_end > len(data):
        errors.append(
            f"section directory extends past end of file: end={directory_end} file_size={len(data)}"
        )
    else:
        seen_kinds: set[int] = set()
        ranges: list[tuple[int, int, int]] = []
        for idx in range(header.section_count):
            pos = directory_start + idx * SIDECAR_DIR_SIZE
            kind, flags, _reserved, offset, size, count = struct.unpack_from(
                SIDECAR_DIR_FMT, data, pos
            )
            descriptor = registry.by_kind(kind)
            if kind in seen_kinds:
                errors.append(f"duplicate section kind: {kind}")
            seen_kinds.add(kind)
            end = int(offset) + int(size)
            if end > len(data):
                errors.append(
                    f"section kind={kind} extends past end of file: end={end} file_size={len(data)}"
                )
            if int(offset) < directory_end:
                errors.append(
                    f"section kind={kind} overlaps header/directory: offset={offset}"
                )
            ranges.append((int(offset), end, int(kind)))
            sections.append(
                SectionDirectoryEntry(
                    kind=int(kind),
                    flags=int(flags),
                    offset=int(offset),
                    size=int(size),
                    count=int(count),
                    name=(
                        descriptor.name
                        if descriptor is not None
                        else f"unknown_{int(kind)}"
                    ),
                    known=descriptor is not None,
                    required=(
                        bool(descriptor.required) if descriptor is not None else False
                    ),
                )
            )
        for prev, cur in zip(sorted(ranges), sorted(ranges)[1:]):
            if cur[0] < prev[1]:
                errors.append(
                    f"section payload overlap: kind={prev[2]} and kind={cur[2]}"
                )
        present_kinds = {section.kind for section in sections}
        for required_kind in sorted(registry.required_kinds()):
            if required_kind not in present_kinds:
                descriptor = registry.by_kind(required_kind)
                errors.append(
                    f"missing required section: {required_kind} ({descriptor.name if descriptor else required_kind})"
                )
        for section in sections:
            descriptor = registry.by_kind(section.kind)
            if descriptor is None:
                warnings.append(
                    f"unknown section kind={section.kind} will be ignored by framework-level tooling"
                )
                continue
            for dep in descriptor.dependencies:
                if int(dep) not in present_kinds:
                    dep_desc = registry.by_kind(dep)
                    errors.append(
                        f"section {section.name} requires missing dependency {int(dep)} ({dep_desc.name if dep_desc else dep})"
                    )

    section_by_kind = {section.kind: section for section in sections}
    decoded: dict[str, Any] = {}
    if not errors:
        strings: list[str] = []
        string_section = section_by_kind.get(int(SectionKind.STRING_TABLE))
        if string_section is not None:
            strings, decode_errors = _decode_string_table(
                _slice_section(data, string_section)
            )
            errors.extend(decode_errors)
            decoded["strings"] = {"count": len(strings), "preview": strings[:16]}
        port_section = section_by_kind.get(int(SectionKind.PORT_TABLE))
        if port_section is not None:
            ports, decode_errors = _decode_ports(
                _slice_section(data, port_section), port_section.count, strings
            )
            errors.extend(decode_errors)
            decoded["ports"] = ports
        event_section = section_by_kind.get(int(SectionKind.EVENT_TABLE))
        if event_section is not None:
            events, decode_errors = _decode_events(
                _slice_section(data, event_section),
                event_section.count,
                strings,
                header.max_words,
            )
            errors.extend(decode_errors)
            decoded["events"] = events
        frame_section = section_by_kind.get(int(SectionKind.FRAME_TABLE))
        if frame_section is not None:
            frames, decode_errors = _decode_frames(
                _slice_section(data, frame_section),
                frame_section.count,
                strings,
                header.max_words,
            )
            errors.extend(decode_errors)
            decoded["frames"] = frames
        pattern_section = section_by_kind.get(int(SectionKind.PATTERN_TABLE))
        if pattern_section is not None:
            patterns, decode_errors = _decode_patterns(
                _slice_section(data, pattern_section),
                pattern_section.count,
                header.max_words,
            )
            errors.extend(decode_errors)
            decoded["patterns"] = patterns

    section_dicts = [
        {
            "kind": section.kind,
            "name": section.name,
            "flags": section.flags,
            "offset": section.offset,
            "size": section.size,
            "count": section.count,
            "known": section.known,
            "required": section.required,
        }
        for section in sections
    ]
    summary = {
        "section_count": len(sections),
        "known_section_count": sum(1 for section in sections if section.known),
        "unknown_section_count": sum(1 for section in sections if not section.known),
        "total_section_payload_bytes": sum(section.size for section in sections),
        "sections_by_name": {
            section.name: {
                "kind": section.kind,
                "count": section.count,
                "size": section.size,
            }
            for section in sections
        },
    }
    return {
        "path": str(p),
        "size_bytes": len(data),
        "valid": not errors,
        "errors": errors,
        "warnings": warnings,
        "header": {
            "endian": header.endian,
            "header_size": header.header_size,
            "major": header.major,
            "minor": header.minor,
            "patch": header.patch,
            "flags": header.flags,
            "section_count": header.section_count,
            "max_words": header.max_words,
            "max_cycle": header.max_cycle,
            "reset_cycles": header.reset_cycles,
            "reserved": header.reserved,
        },
        "sections": section_dicts,
        "decoded": decoded,
        "summary": summary,
    }


def render_sidecar_inspect_text(report: dict[str, Any]) -> str:
    lines: list[str] = []
    lines.append(f"SIDECAR file: {report.get('path')}")
    lines.append(f"valid: {bool(report.get('valid'))}")
    lines.append(f"size_bytes: {int(report.get('size_bytes', 0))}")
    header = report.get("header")
    if isinstance(header, dict):
        lines.append(
            "container: "
            f"v{header.get('major')}.{header.get('minor')}.{header.get('patch')} "
            f"sections={header.get('section_count')} "
            f"max_words={header.get('max_words')} "
            f"max_cycle={header.get('max_cycle')}"
        )
    errors = report.get("errors") or []
    warnings = report.get("warnings") or []
    if errors:
        lines.append("errors:")
        for item in errors:
            lines.append(f"  - {item}")
    if warnings:
        lines.append("warnings:")
        for item in warnings:
            lines.append(f"  - {item}")
    lines.append("")
    lines.append("sections:")
    lines.append(
        "  kind  name                         count        size  flags  status"
    )
    for section in report.get("sections", []):
        status: list[str] = []
        if section.get("required"):
            status.append("required")
        if not section.get("known"):
            status.append("unknown")
        status_text = ",".join(status) if status else "optional"
        lines.append(
            "  "
            f"{int(section.get('kind', 0)):>4}  "
            f"{str(section.get('name', '')):<28} "
            f"{int(section.get('count', 0)):>8}  "
            f"{int(section.get('size', 0)):>10}  "
            f"0x{int(section.get('flags', 0)):04x}  "
            f"{status_text}"
        )
    decoded = report.get("decoded")
    if isinstance(decoded, dict) and decoded:
        lines.append("")
        if isinstance(decoded.get("ports"), list):
            lines.append(f"decoded_ports: {len(decoded['ports'])}")
            for port in decoded["ports"][:8]:
                lines.append(
                    "  "
                    f"id={port.get('id')} "
                    f"name={port.get('name')} "
                    f"dir={port.get('direction')} "
                    f"role={port.get('role')} "
                    f"width={port.get('bit_width')} "
                    f"protocol={port.get('protocol')}"
                )
        if isinstance(decoded.get("events"), list):
            lines.append(f"events: {len(decoded['events'])}")
        if isinstance(decoded.get("frames"), list):
            lines.append(f"frames: {len(decoded['frames'])}")
        if isinstance(decoded.get("patterns"), list):
            lines.append(f"patterns: {len(decoded['patterns'])}")
    return "\n".join(lines) + "\n"


def _int_or_none(value: Any) -> int | None:
    if value is None:
        return None
    try:
        if isinstance(value, str) and value.startswith(("0x", "0X")):
            return int(value, 16)
        return int(value)
    except (TypeError, ValueError):
        return None


def _mapping_items(value: Any) -> list[Mapping[str, Any]]:
    if not isinstance(value, list):
        return []
    return [item for item in value if isinstance(item, Mapping)]


def verify_schedule_ir_for_sidecar(schedule_ir: Mapping[str, Any]) -> list[str]:
    errors: list[str] = []
    ports = _mapping_items(schedule_ir.get("ports"))
    port_ids: set[int] = set()
    for port in ports:
        pid = _int_or_none(port.get("id"))
        width = _int_or_none(port.get("bit_width"))
        if pid is None:
            errors.append("port entry missing integer id")
            continue
        if pid in port_ids:
            errors.append(f"duplicate port id: {pid}")
        port_ids.add(pid)
        if width is None or width <= 0:
            errors.append(
                f"port id={pid} has invalid bit_width: {port.get('bit_width')!r}"
            )
        if str(port.get("name", "")).strip() == "":
            errors.append(f"port id={pid} has empty name")

    def require_port(section: str, field: str, value: Any) -> None:
        pid = _int_or_none(value)
        if pid is None:
            errors.append(f"{section}.{field} is not an integer port id: {value!r}")
            return
        if pid not in port_ids:
            errors.append(f"{section}.{field} references missing port id: {pid}")

    for event in _mapping_items(schedule_ir.get("events")):
        if event.get("port") is not None:
            require_port("event", "port", event.get("port"))
        if _int_or_none(event.get("cycle")) is None:
            errors.append(f"event has invalid cycle: {event.get('cycle')!r}")

    for frame in _mapping_items(schedule_ir.get("frames")):
        if _int_or_none(frame.get("cycle")) is None:
            errors.append(f"frame has invalid cycle: {frame.get('cycle')!r}")
        for item in _mapping_items(frame.get("items")):
            require_port("frame_item", "port", item.get("port"))

    for pattern in _mapping_items(schedule_ir.get("patterns")):
        require_port("pattern", "port", pattern.get("port"))
        if str(pattern.get("kind", "")) == "periodic_drive":
            period = _int_or_none(pattern.get("period"))
            active_cycles = _int_or_none(pattern.get("active_cycles"))
            if period is None or period <= 0:
                errors.append(
                    f"periodic pattern has invalid period: {pattern.get('period')!r}"
                )
            if active_cycles is None or active_cycles < 0:
                errors.append(
                    f"periodic pattern has invalid active_cycles: {pattern.get('active_cycles')!r}"
                )
            if (
                period is not None
                and active_cycles is not None
                and active_cycles > period
            ):
                errors.append(
                    f"periodic pattern active_cycles exceeds period: {active_cycles} > {period}"
                )

    return errors

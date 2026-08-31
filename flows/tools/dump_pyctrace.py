#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import struct
import sys
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path

MAGIC_V3 = b"PYC6TRC3"


class ParseError(RuntimeError):
    pass


class ChunkType(IntEnum):
    PROBE_DECLARE = 1
    CYCLE_BEGIN = 2
    CYCLE_END = 3
    VALUE_CHANGE = 4
    LOG = 5
    ASSERT = 6
    WRITE = 7
    RESET = 8
    INVALIDATE = 9


class Phase(IntEnum):
    COMB = 0
    TICK = 1
    COMMIT = 2


class LogLevel(IntEnum):
    TRACE = 0
    DEBUG = 1
    INFO = 2
    WARN = 3
    ERROR = 4
    FATAL = 5


@dataclass(frozen=True)
class ProbeDecl:
    probe_id: int
    kind: int
    canonical_path: str
    human_name: str
    type_sig: bytes


@dataclass(frozen=True)
class WriteEv:
    cycle: int
    phase: int
    probe_id: int
    subkind: int
    addr: int | None
    data_width_bits: int
    data_bytes: bytes
    mask_width_bits: int | None
    mask_bytes: bytes | None


@dataclass(frozen=True)
class ValueChangeEv:
    cycle: int
    phase: int
    probe_id: int
    width_bits: int
    value_bytes: bytes
    known_mask_width_bits: int
    known_mask_bytes: bytes
    z_mask_width_bits: int
    z_mask_bytes: bytes


@dataclass(frozen=True)
class ResetEv:
    cycle: int
    phase: int | None
    edge: int
    kind: int
    domain: str


@dataclass(frozen=True)
class InvalidateEv:
    cycle: int
    phase: int | None
    reason: int
    domain: str
    scope: str
    reason_text: str


def _take(buf: memoryview, off: int, n: int) -> tuple[memoryview, int]:
    if off + n > len(buf):
        raise ParseError(f"unexpected EOF at offset {off} need {n} bytes")
    return buf[off : off + n], off + n


def _u8(buf: memoryview, off: int) -> tuple[int, int]:
    b, off = _take(buf, off, 1)
    return int(b[0]), off


def _u16le(buf: memoryview, off: int) -> tuple[int, int]:
    b, off = _take(buf, off, 2)
    return int(struct.unpack_from("<H", b, 0)[0]), off


def _u32le(buf: memoryview, off: int) -> tuple[int, int]:
    b, off = _take(buf, off, 4)
    return int(struct.unpack_from("<I", b, 0)[0]), off


def _u64le(buf: memoryview, off: int) -> tuple[int, int]:
    b, off = _take(buf, off, 8)
    return int(struct.unpack_from("<Q", b, 0)[0]), off


def _bytes(buf: memoryview, off: int, n: int) -> tuple[bytes, int]:
    b, off = _take(buf, off, n)
    return bytes(b), off


def _decode_utf8(b: bytes) -> str:
    try:
        return b.decode("utf-8")
    except UnicodeDecodeError:
        return b.decode("utf-8", errors="replace")


def _load_external_manifest(path: Path) -> tuple[dict[int, str], dict[int, int]]:
    obj = json.loads(path.read_text(encoding="utf-8"))
    pid_to_path: dict[int, str] = {}
    pid_to_width: dict[int, int] = {}
    for p in obj.get("probes", []):
        if not isinstance(p, dict):
            continue
        pid_raw = p.get("probe_id", "")
        if not isinstance(pid_raw, str):
            continue
        pid_raw = pid_raw.strip()
        if pid_raw.startswith("0x") or pid_raw.startswith("0X"):
            try:
                pid = int(pid_raw, 16)
            except ValueError:
                continue
        else:
            try:
                pid = int(pid_raw, 10)
            except ValueError:
                continue
        cpath = p.get("canonical_path", "")
        if isinstance(cpath, str) and cpath:
            pid_to_path[pid] = cpath
        w = p.get("width_bits", 0)
        if isinstance(w, int):
            pid_to_width[pid] = int(w)
    return pid_to_path, pid_to_width


def parse_pyctrace(
    path: Path,
    *,
    external_manifest: Path | None = None,
) -> tuple[
    int,
    int,
    list[ProbeDecl],
    list[ValueChangeEv],
    list[WriteEv],
    list[ResetEv],
    list[InvalidateEv],
]:
    data = memoryview(path.read_bytes())
    off = 0

    magic, off = _bytes(data, off, 8)
    if magic != MAGIC_V3:
        raise ParseError(f"bad magic: got={magic!r} exp={MAGIC_V3!r}")

    schema_version, off = _u32le(data, off)
    flags, off = _u32le(data, off)

    probes: list[ProbeDecl] = []
    pid_to_path: dict[int, str] = {}
    pid_to_width: dict[int, int] = {}
    if external_manifest is not None:
        pid_to_path, pid_to_width = _load_external_manifest(external_manifest)

    evs: list[ValueChangeEv] = []
    writes: list[WriteEv] = []
    resets: list[ResetEv] = []
    invalidates: list[InvalidateEv] = []
    cur_cycle: int | None = None
    cur_phase: int | None = None
    while off < len(data):
        chunk_len, off = _u32le(data, off)
        chunk_ty, off = _u32le(data, off)
        payload, off = _take(data, off, chunk_len)
        poff = 0

        if chunk_ty == int(ChunkType.PROBE_DECLARE):
            pid, poff = _u64le(payload, poff)
            kind, poff = _u8(payload, poff)
            _subkind, poff = _u8(payload, poff)
            path_len, poff = _u32le(payload, poff)
            pbytes, poff = _bytes(payload, poff, path_len)
            cname = _decode_utf8(pbytes)
            human_len, poff = _u32le(payload, poff)
            hbytes, poff = _bytes(payload, poff, human_len)
            human = _decode_utf8(hbytes)
            ts_len, poff = _u32le(payload, poff)
            ts, poff = _bytes(payload, poff, ts_len)
            probes.append(
                ProbeDecl(
                    probe_id=pid,
                    kind=kind,
                    canonical_path=cname,
                    human_name=human,
                    type_sig=ts,
                )
            )
            pid_to_path.setdefault(pid, cname)
            # Width lives in type_sig; we also infer width for Bits from the current minimal encoding.
            if len(ts) >= 6 and ts[0] == 0:
                width_bits = int(struct.unpack_from("<I", ts, 1)[0])
                pid_to_width.setdefault(pid, width_bits)
            continue

        if chunk_ty == int(ChunkType.CYCLE_BEGIN):
            cyc, poff = _u64le(payload, poff)
            ph, poff = _u8(payload, poff)
            cur_cycle = cyc
            cur_phase = ph
            continue

        if chunk_ty == int(ChunkType.CYCLE_END):
            cyc, poff = _u64le(payload, poff)
            ph, poff = _u8(payload, poff)
            if cur_cycle == cyc and cur_phase == ph:
                cur_cycle = None
                cur_phase = None
            continue

        if chunk_ty == int(ChunkType.VALUE_CHANGE):
            pid, poff = _u64le(payload, poff)
            width_bits, poff = _u32le(payload, poff)
            byte_count = (width_bits + 7) // 8 if width_bits > 0 else 0
            vbytes, poff = _bytes(payload, poff, byte_count)
            known_mask_width_bits, poff = _u32le(payload, poff)
            known_n = (
                (known_mask_width_bits + 7) // 8 if known_mask_width_bits > 0 else 0
            )
            known_mask_bytes, poff = _bytes(payload, poff, known_n)
            z_mask_width_bits, poff = _u32le(payload, poff)
            z_n = (z_mask_width_bits + 7) // 8 if z_mask_width_bits > 0 else 0
            z_mask_bytes, poff = _bytes(payload, poff, z_n)
            if cur_cycle is None or cur_phase is None:
                raise ParseError("ValueChange seen without active CycleBegin")
            evs.append(
                ValueChangeEv(
                    cycle=int(cur_cycle),
                    phase=int(cur_phase),
                    probe_id=int(pid),
                    width_bits=int(width_bits),
                    value_bytes=vbytes,
                    known_mask_width_bits=int(known_mask_width_bits),
                    known_mask_bytes=known_mask_bytes,
                    z_mask_width_bits=int(z_mask_width_bits),
                    z_mask_bytes=z_mask_bytes,
                )
            )
            pid_to_width.setdefault(pid, width_bits)
            continue

        if chunk_ty == int(ChunkType.WRITE):
            pid, poff = _u64le(payload, poff)
            subkind, poff = _u8(payload, poff)
            flags, poff = _u8(payload, poff)
            addr: int | None = None
            if flags & 0x1:
                addr, poff = _u64le(payload, poff)
            data_width_bits, poff = _u32le(payload, poff)
            data_n = (data_width_bits + 7) // 8 if data_width_bits > 0 else 0
            data_bytes, poff = _bytes(payload, poff, data_n)
            mask_width_bits: int | None = None
            mask_bytes: bytes | None = None
            if flags & 0x2:
                mw, poff = _u32le(payload, poff)
                mask_width_bits = int(mw)
                mask_n = (mw + 7) // 8 if mw > 0 else 0
                mask_bytes, poff = _bytes(payload, poff, mask_n)
            if cur_cycle is None or cur_phase is None:
                raise ParseError("Write seen without active CycleBegin")
            writes.append(
                WriteEv(
                    cycle=int(cur_cycle),
                    phase=int(cur_phase),
                    probe_id=int(pid),
                    subkind=int(subkind),
                    addr=int(addr) if addr is not None else None,
                    data_width_bits=int(data_width_bits),
                    data_bytes=data_bytes,
                    mask_width_bits=mask_width_bits,
                    mask_bytes=mask_bytes,
                )
            )
            continue

        if chunk_ty == int(ChunkType.LOG):
            # Currently ignored by the dumper output (but validated for structure).
            _level, poff = _u8(payload, poff)
            msg_len, poff = _u32le(payload, poff)
            _msg, poff = _bytes(payload, poff, msg_len)
            continue

        if chunk_ty == int(ChunkType.ASSERT):
            _fatal, poff = _u8(payload, poff)
            msg_len, poff = _u32le(payload, poff)
            _msg, poff = _bytes(payload, poff, msg_len)
            continue

        if chunk_ty == int(ChunkType.RESET):
            cyc, poff = _u64le(payload, poff)
            phase_present, poff = _u8(payload, poff)
            ph, poff = _u8(payload, poff)
            edge, poff = _u8(payload, poff)
            kind, poff = _u8(payload, poff)
            domain_len, poff = _u32le(payload, poff)
            domain_b, poff = _bytes(payload, poff, domain_len)
            resets.append(
                ResetEv(
                    cycle=int(cyc),
                    phase=int(ph) if int(phase_present) else None,
                    edge=int(edge),
                    kind=int(kind),
                    domain=_decode_utf8(domain_b),
                )
            )
            continue

        if chunk_ty == int(ChunkType.INVALIDATE):
            cyc, poff = _u64le(payload, poff)
            phase_present, poff = _u8(payload, poff)
            ph, poff = _u8(payload, poff)
            reason, poff = _u8(payload, poff)
            domain_len, poff = _u32le(payload, poff)
            domain_b, poff = _bytes(payload, poff, domain_len)
            scope_len, poff = _u32le(payload, poff)
            scope_b, poff = _bytes(payload, poff, scope_len)
            reason_text_len, poff = _u32le(payload, poff)
            reason_text_b, poff = _bytes(payload, poff, reason_text_len)
            invalidates.append(
                InvalidateEv(
                    cycle=int(cyc),
                    phase=int(ph) if int(phase_present) else None,
                    reason=int(reason),
                    domain=_decode_utf8(domain_b),
                    scope=_decode_utf8(scope_b),
                    reason_text=_decode_utf8(reason_text_b),
                )
            )
            continue

        # Unknown chunk types are skipped (Decision 0041).
        continue

    return schema_version, flags, probes, evs, writes, resets, invalidates


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Dump a pyCircuit v6 binary trace (.pyctrace)."
    )
    ap.add_argument("path", type=Path)
    ap.add_argument(
        "--manifest",
        type=Path,
        default=None,
        help="External probe_manifest.json (Decision 0037).",
    )
    ap.add_argument("--max-cycles", type=int, default=10)
    ap.add_argument("--max-events", type=int, default=50)
    ap.add_argument("--no-header", action="store_true")
    ns = ap.parse_args()

    p = Path(ns.path).resolve()
    if not p.is_file():
        print(f"error: file not found: {p}", file=sys.stderr)
        return 2

    manifest = Path(ns.manifest).resolve() if ns.manifest is not None else None
    if manifest is not None and not manifest.is_file():
        print(f"error: manifest not found: {manifest}", file=sys.stderr)
        return 2

    try:
        schema_version, flags, probes, evs, writes, resets, invalidates = (
            parse_pyctrace(p, external_manifest=manifest)
        )
    except ParseError as e:
        print(f"error: {p}: {e}", file=sys.stderr)
        return 2

    if not ns.no_header:
        print(f"path: {p}")
        print(f"schema_version: {schema_version}")
        print(f"flags: 0x{flags:08x}")
        print(f"probe_decl_count: {len(probes)}")
        for d in probes[: min(len(probes), 20)]:
            print(f"  - id=0x{d.probe_id:016x} kind={d.kind} path={d.canonical_path!r}")
        if len(probes) > 20:
            print(f"  ... ({len(probes) - 20} more)")

    pid_to_path: dict[int, str] = {}
    pid_to_width: dict[int, int] = {}
    if manifest is not None:
        pid_to_path, pid_to_width = _load_external_manifest(manifest)
    for d in probes:
        pid_to_path.setdefault(d.probe_id, d.canonical_path)
        if len(d.type_sig) >= 6 and d.type_sig[0] == 0:
            pid_to_width.setdefault(
                d.probe_id, int(struct.unpack_from("<I", d.type_sig, 1)[0])
            )

    max_cycles = max(0, int(ns.max_cycles))
    max_events = max(0, int(ns.max_events))

    seen_cycles: list[int] = []
    by_cycle_vc: dict[int, list[ValueChangeEv]] = {}
    by_cycle_wr: dict[int, list[WriteEv]] = {}
    by_cycle_reset: dict[int, list[ResetEv]] = {}
    by_cycle_inval: dict[int, list[InvalidateEv]] = {}

    def _touch_cycle(cyc: int) -> bool:
        if (
            cyc in by_cycle_vc
            or cyc in by_cycle_wr
            or cyc in by_cycle_reset
            or cyc in by_cycle_inval
        ):
            return True
        if len(seen_cycles) >= max_cycles:
            return False
        seen_cycles.append(cyc)
        return True

    for ev in evs:
        if not _touch_cycle(int(ev.cycle)):
            continue
        by_cycle_vc.setdefault(int(ev.cycle), []).append(ev)

    for w in writes:
        if not _touch_cycle(int(w.cycle)):
            continue
        by_cycle_wr.setdefault(int(w.cycle), []).append(w)

    for r in resets:
        if not _touch_cycle(int(r.cycle)):
            continue
        by_cycle_reset.setdefault(int(r.cycle), []).append(r)

    for inv in invalidates:
        if not _touch_cycle(int(inv.cycle)):
            continue
        by_cycle_inval.setdefault(int(inv.cycle), []).append(inv)

    for cyc in seen_cycles:
        cev = by_cycle_vc.get(cyc, [])
        cwr = by_cycle_wr.get(cyc, [])
        crs = by_cycle_reset.get(cyc, [])
        cinv = by_cycle_inval.get(cyc, [])
        print(f"cycle {cyc}: {len(cev)} value-change events, {len(cwr)} write events")

        for w in cwr[: min(len(cwr), max_events)]:
            phase = (
                Phase(w.phase).name.lower()
                if w.phase in {int(x) for x in Phase}
                else str(w.phase)
            )
            path = pid_to_path.get(w.probe_id, f"<unknown:0x{w.probe_id:016x}>")
            sub = {0: "none", 1: "wire", 2: "reg", 3: "mem", 4: "statevar"}.get(
                int(w.subkind), str(w.subkind)
            )
            addr = "" if w.addr is None else f" addr=0x{int(w.addr):x}"
            if w.data_width_bits <= 64:
                dv = int.from_bytes(w.data_bytes, "little", signed=False)
                data_str = f" data=0x{dv:x}"
            else:
                data_str = f" data=[bytes={len(w.data_bytes)}] 0x{w.data_bytes.hex()}"
            mask_str = ""
            if w.mask_bytes is not None and w.mask_width_bits is not None:
                if w.mask_width_bits <= 64:
                    mv = int.from_bytes(w.mask_bytes, "little", signed=False)
                    mask_str = f" mask=0x{mv:x}"
                else:
                    mask_str = (
                        f" mask=[bytes={len(w.mask_bytes)}] 0x{w.mask_bytes.hex()}"
                    )
            print(f"  - ({phase}) WRITE[{sub}] {path}{addr}{data_str}{mask_str}")

        for ev in cev[: min(len(cev), max_events)]:
            phase = (
                Phase(ev.phase).name.lower()
                if ev.phase in {int(x) for x in Phase}
                else str(ev.phase)
            )
            width = pid_to_width.get(ev.probe_id, int(ev.width_bits))
            path = pid_to_path.get(ev.probe_id, f"<unknown:0x{ev.probe_id:016x}>")
            if width <= 64:
                v = int.from_bytes(ev.value_bytes, "little", signed=False)
                k = (
                    int.from_bytes(ev.known_mask_bytes, "little", signed=False)
                    if ev.known_mask_bytes
                    else 0
                )
                z = (
                    int.from_bytes(ev.z_mask_bytes, "little", signed=False)
                    if ev.z_mask_bytes
                    else 0
                )
                print(f"  - ({phase}) {path} = 0x{v:x} known=0x{k:x} z=0x{z:x}")
            else:
                v_hx = ev.value_bytes.hex()
                k_hx = ev.known_mask_bytes.hex()
                z_hx = ev.z_mask_bytes.hex()
                print(
                    f"  - ({phase}) {path} = [bytes={len(ev.value_bytes)}] 0x{v_hx} "
                    f"known=0x{k_hx} z=0x{z_hx}"
                )
        if len(cev) > max_events:
            print(f"  ... ({len(cev) - max_events} more)")

        for r in crs[: min(len(crs), max_events)]:
            phase_name = (
                "commit"
                if r.phase is None
                else (
                    Phase(r.phase).name.lower()
                    if r.phase in {int(x) for x in Phase}
                    else str(r.phase)
                )
            )
            edge_name = {1: "RESET_ASSERT", 2: "RESET_DEASSERT"}.get(
                int(r.edge), f"RESET_{r.edge}"
            )
            kind_name = {1: "warm", 2: "flush"}.get(int(r.kind), str(r.kind))
            print(f"  - ({phase_name}) {edge_name} domain={r.domain} kind={kind_name}")

        for inv in cinv[: min(len(cinv), max_events)]:
            phase_name = (
                "commit"
                if inv.phase is None
                else (
                    Phase(inv.phase).name.lower()
                    if inv.phase in {int(x) for x in Phase}
                    else str(inv.phase)
                )
            )
            reason_name = {1: "warm_reset", 2: "flush_reset", 255: "other"}.get(
                int(inv.reason), str(inv.reason)
            )
            scope = f" scope={inv.scope}" if inv.scope else ""
            reason_text = f" text={inv.reason_text}" if inv.reason_text else ""
            print(
                f"  - ({phase_name}) INVALIDATE domain={inv.domain}{scope} reason={reason_name}{reason_text}"
            )

    if len(seen_cycles) == 0:
        print("no trace events")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
import argparse
import gzip
import json
from dataclasses import dataclass
from pathlib import Path

DEFAULT_COMMIT_SCHEMA_ID = "LC-COMMIT-BUNDLE-V2"

# Decision 0142 minimum commit/retire bundle fields (as used by LinxCore M1):
#   pc/insn/len/next_pc + wb/mem/trap groups.
#
# Decision 0146: groups obey validity gating; unknown fields are ignored.
_BASE_REQUIRED_FIELDS = [
    "cycle",
    "pc",
    "insn",
    "len",
    "next_pc",
    "wb_valid",
    "mem_valid",
    "trap_valid",
]

_WB_FIELDS = ["wb_rd", "wb_data"]
_MEM_FIELDS = ["mem_addr", "mem_wdata", "mem_rdata", "mem_size"]
_TRAP_FIELDS = ["trap_cause"]

# Public compatibility alias consumed by root contract gates.
MANDATORY_FIELDS = [
    "cycle",
    "pc",
    "insn",
    "len",
    "next_pc",
    "src0_valid",
    "src0_reg",
    "src0_data",
    "src1_valid",
    "src1_reg",
    "src1_data",
    "dst_valid",
    "dst_reg",
    "dst_data",
    "wb_valid",
    "wb_rd",
    "wb_data",
    "mem_valid",
    "mem_is_store",
    "mem_addr",
    "mem_wdata",
    "mem_rdata",
    "mem_size",
    "trap_valid",
    "trap_cause",
    "traparg0",
]


@dataclass(frozen=True)
class CommitTrace:
    path: Path
    commit_schema_id: str | None
    rows: list["TraceRec"]


@dataclass(frozen=True)
class TraceRec:
    raw: dict

    def get(self, k: str):
        return self.raw.get(k, None)


def _to_int(v, default: int = 0) -> int:
    try:
        if isinstance(v, bool):
            return 1 if v else 0
        if isinstance(v, int):
            return int(v)
        if isinstance(v, str):
            s = v.strip().lower().replace("_", "")
            if not s:
                return default
            return int(s, 0)
        return int(v)
    except Exception:
        return default


def _is_quiet_commit(r: TraceRec) -> bool:
    return (
        _to_int(r.get("wb_valid")) == 0
        and _to_int(r.get("mem_valid")) == 0
        and _to_int(r.get("trap_valid")) == 0
    )


def _collapse_boundary_selfloops(rows: list[TraceRec]) -> list[TraceRec]:
    """
    Drop synthetic boundary self-loop rows emitted by some producers.

    Pattern dropped:
    - row[i] and row[i+1] have identical pc/insn,
    - row[i].next_pc == row[i].pc.

    Some producers annotate replay/self-loop boundaries with trap metadata while
    others do not. For this normalization mode we drop the first replay row
    regardless of side-effect flags and keep the follow-up commit.
    """
    out: list[TraceRec] = []
    i = 0
    n = len(rows)
    while i < n:
        cur = rows[i]
        if i + 1 < n:
            nxt = rows[i + 1]
            same_pc = _to_int(cur.get("pc")) == _to_int(nxt.get("pc"))
            same_insn = _to_int(cur.get("insn")) == _to_int(nxt.get("insn"))
            self_loop = _to_int(cur.get("next_pc")) == _to_int(cur.get("pc"))
            if same_pc and same_insn and self_loop:
                i += 1
                continue
        out.append(cur)
        i += 1
    return out


def load_jsonl(path: str) -> list[TraceRec]:
    raise RuntimeError("internal: use load_commit_trace()")


def _open_text(path: Path):
    if str(path).endswith(".gz"):
        return gzip.open(path, "rt", encoding="utf-8")
    return open(path, encoding="utf-8")


def _is_start_record(obj: dict) -> bool:
    t = obj.get("type", None)
    if t is None:
        return False
    return str(t).strip().lower() in {"start", "header", "meta"}


def _extract_schema_id(start: dict) -> str | None:
    for k in [
        "commit_schema_id",
        "schema_id",
        "trace_schema_id",
        "schema",
        "version",
        "trace_version",
    ]:
        v = start.get(k, None)
        if v is None:
            continue
        s = str(v).strip()
        if s:
            return s
    return None


def _validate_commit_record(obj: dict, *, path: str, ln: int) -> None:
    missing = [field for field in _BASE_REQUIRED_FIELDS if field not in obj]
    if missing:
        raise SystemExit(
            f"error: {path}:{ln}: missing required fields: {', '.join(missing)}"
        )

    if _to_int(obj.get("wb_valid")) != 0:
        missing_wb = [field for field in _WB_FIELDS if field not in obj]
        if missing_wb:
            raise SystemExit(
                f"error: {path}:{ln}: wb_valid==1 but missing wb fields: {', '.join(missing_wb)}"
            )

    if _to_int(obj.get("mem_valid")) != 0:
        missing_mem = [field for field in _MEM_FIELDS if field not in obj]
        if missing_mem:
            raise SystemExit(
                f"error: {path}:{ln}: mem_valid==1 but missing mem fields: {', '.join(missing_mem)}"
            )

    if _to_int(obj.get("trap_valid")) != 0:
        missing_trap = [field for field in _TRAP_FIELDS if field not in obj]
        if missing_trap:
            raise SystemExit(
                f"error: {path}:{ln}: trap_valid==1 but missing trap fields: {', '.join(missing_trap)}"
            )


def load_commit_trace(
    path: str,
    *,
    assume_schema_id: str | None = None,
    expected_schema_id: str | None = None,
    require_schema_id: bool = False,
) -> CommitTrace:
    p = Path(path).resolve()
    if not p.is_file():
        raise SystemExit(f"error: trace not found: {p}")

    schema_id: str | None = None
    out: list[TraceRec] = []
    with _open_text(p) as f:
        for ln, line in enumerate(f, 1):
            line = line.strip()
            if not line:
                continue
            try:
                obj = json.loads(line)
            except json.JSONDecodeError as e:
                raise SystemExit(f"error: {p}:{ln}: invalid JSON: {e}") from e
            if not isinstance(obj, dict):
                raise SystemExit(f"error: {p}:{ln}: expected JSON object per line")

            if not out and schema_id is None and _is_start_record(obj):
                schema_id = _extract_schema_id(obj)
                continue

            _validate_commit_record(obj, path=str(p), ln=ln)
            out.append(TraceRec(obj))

    if schema_id is None and assume_schema_id is not None:
        schema_id = str(assume_schema_id).strip() or None

    if require_schema_id and schema_id is None:
        raise SystemExit(f"error: {p}: missing start record with commit_schema_id")

    if expected_schema_id is not None:
        exp = str(expected_schema_id).strip()
        if exp and schema_id != exp:
            got = "<missing>" if schema_id is None else schema_id
            raise SystemExit(
                f"error: {p}: schema mismatch: expected={exp!r} got={got!r}"
            )

    return CommitTrace(path=p, commit_schema_id=schema_id, rows=out)


def fmt_hex(v):
    if isinstance(v, int):
        return hex(v)
    return repr(v)


def _write_json(path: Path, obj: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(obj, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _write_jsonl(path: Path, rows: list[TraceRec]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        for r in rows:
            f.write(json.dumps(r.raw, sort_keys=True))
            f.write("\n")


def _default_trace_config_for_cycle(*, cycle: int, pre: int, post: int) -> dict:
    # Decision 0142/0145: generate a trace DSL config that captures a bounded
    # window around a trigger cycle, selecting a conservative default signal set.
    #
    # This config is intentionally broad (ports + standard PV probes), as the
    # mismatch triage path should always work even when probe tags evolve.
    return {
        "version": 1,
        "rules": [
            {"instances": ["**"], "ports": ["*"]},
            {
                "instances": ["**"],
                "probes": {"families": ["pv"], "at": ["tick", "xfer"]},
            },
        ],
        "window": {
            "trigger": {"cycle": int(cycle)},
            "pre": int(pre),
            "post": int(post),
        },
        "note": "auto-generated by linx_trace_diff.py (Decision 0142 mismatch DFX dump request)",
    }


def _dump_mismatch(
    *,
    dump_dir: Path,
    idx: int,
    field: str,
    ref: list[TraceRec],
    dut: list[TraceRec],
    ref_schema_id: str | None,
    dut_schema_id: str | None,
    pre: int,
    post: int,
) -> None:
    dump_dir = Path(dump_dir).resolve()
    dump_dir.mkdir(parents=True, exist_ok=True)

    ra = ref[idx].raw if 0 <= idx < len(ref) else {}
    rb = dut[idx].raw if 0 <= idx < len(dut) else {}
    cyc = _to_int(ra.get("cycle", 0))
    if cyc == 0 and "cycle" in rb:
        cyc = _to_int(rb.get("cycle", 0))

    begin = max(0, int(idx) - int(pre))
    end = min(len(ref), len(dut), int(idx) + int(post) + 1)
    ref_ctx = ref[begin:end]
    dut_ctx = dut[begin:end]

    _write_json(
        dump_dir / "mismatch.json",
        {
            "commit_schema_id": {"ref": ref_schema_id, "dut": dut_schema_id},
            "mismatch": {
                "idx": int(idx),
                "field": str(field),
                "cycle": int(cyc),
                "ref": ra,
                "dut": rb,
            },
            "context": {"begin_idx": int(begin), "end_idx_exclusive": int(end)},
        },
    )
    _write_jsonl(dump_dir / "ref.context.jsonl", ref_ctx)
    _write_jsonl(dump_dir / "dut.context.jsonl", dut_ctx)
    _write_json(
        dump_dir / "trace_config.json",
        _default_trace_config_for_cycle(cycle=cyc, pre=pre, post=post),
    )


def first_mismatch(
    a: list[TraceRec],
    b: list[TraceRec],
    *,
    ignore_fields: set[str],
    limit: int | None = None,
) -> tuple[int, str] | None:
    if limit is not None and limit >= 0:
        a = a[:limit]
        b = b[:limit]
    n = min(len(a), len(b))
    for i in range(n):
        ra = a[i].raw
        rb = b[i].raw
        # Always compare core sequencing fields.
        for k in ["pc", "insn", "len", "next_pc"]:
            if k in ignore_fields:
                continue
            if _to_int(ra.get(k, None), default=-1) != _to_int(
                rb.get(k, None), default=-1
            ):
                return i, k

        # WB fields: rd/data are don't-care when wb_valid==0.
        for k in ["wb_valid"]:
            if k in ignore_fields:
                continue
            if _to_int(ra.get(k, None), default=-1) != _to_int(
                rb.get(k, None), default=-1
            ):
                return i, k
        if _to_int(ra.get("wb_valid", 0)) != 0 and _to_int(rb.get("wb_valid", 0)) != 0:
            for k in ["wb_rd", "wb_data"]:
                if k in ignore_fields:
                    continue
                if _to_int(ra.get(k, None), default=-1) != _to_int(
                    rb.get(k, None), default=-1
                ):
                    return i, k

        # Mem fields: addr/data/size are don't-care when mem_valid==0.
        for k in ["mem_valid"]:
            if k in ignore_fields:
                continue
            if _to_int(ra.get(k, None), default=-1) != _to_int(
                rb.get(k, None), default=-1
            ):
                return i, k
        if (
            _to_int(ra.get("mem_valid", 0)) != 0
            and _to_int(rb.get("mem_valid", 0)) != 0
        ):
            for k in ["mem_addr", "mem_wdata", "mem_rdata", "mem_size"]:
                if k in ignore_fields:
                    continue
                if _to_int(ra.get(k, None), default=-1) != _to_int(
                    rb.get(k, None), default=-1
                ):
                    return i, k

        # Trap fields: cause is don't-care when trap_valid==0.
        for k in ["trap_valid"]:
            if k in ignore_fields:
                continue
            if _to_int(ra.get(k, None), default=-1) != _to_int(
                rb.get(k, None), default=-1
            ):
                return i, k
        if (
            _to_int(ra.get("trap_valid", 0)) != 0
            and _to_int(rb.get("trap_valid", 0)) != 0
        ):
            k = "trap_cause"
            if k not in ignore_fields and _to_int(
                ra.get(k, None), default=-1
            ) != _to_int(rb.get(k, None), default=-1):
                return i, k
    if len(a) != len(b):
        return n, "<length>"
    return None


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Diff LinxISA JSONL commit traces (QEMU vs pyCircuit bring-up)."
    )
    ap.add_argument("ref_jsonl", help="Reference JSONL (typically QEMU)")
    ap.add_argument("dut_jsonl", help="DUT JSONL (typically pyCircuit)")
    ap.add_argument(
        "--expected-schema-id",
        default=None,
        help=f"Expected commit schema id (Decision 0142). Example: {DEFAULT_COMMIT_SCHEMA_ID}",
    )
    ap.add_argument(
        "--assume-schema-id",
        default=None,
        help=f"Assume this schema id when the trace omits a start record (default: {DEFAULT_COMMIT_SCHEMA_ID}).",
    )
    ap.add_argument(
        "--require-schema-id",
        action="store_true",
        help="Require a start record with commit_schema_id in both traces (Decision 0142).",
    )
    ap.add_argument(
        "--dump-dir",
        default=None,
        help="On mismatch, write a DFX dump request (mismatch.json + trace_config.json + trace contexts) to this directory.",
    )
    ap.add_argument(
        "--dump-pre",
        type=int,
        default=8,
        help="Commits to include before the mismatch in dump artifacts (default: 8).",
    )
    ap.add_argument(
        "--dump-post",
        type=int,
        default=16,
        help="Commits to include after the mismatch in dump artifacts (default: 16).",
    )
    ap.add_argument(
        "--ignore",
        action="append",
        default=[],
        help="Ignore a field (repeatable). Example: --ignore mem_rdata",
    )
    ap.add_argument(
        "--drop-boundary-selfloops",
        action="store_true",
        help="Drop synthetic quiet boundary self-loop rows before diffing.",
    )
    ap.add_argument(
        "--limit",
        type=int,
        default=None,
        help="Only compare the first N commits after normalization.",
    )
    args = ap.parse_args()

    ignore_fields = set(args.ignore)

    assume_schema_id = args.assume_schema_id
    if assume_schema_id is None:
        assume_schema_id = DEFAULT_COMMIT_SCHEMA_ID

    ref_trace = load_commit_trace(
        args.ref_jsonl,
        assume_schema_id=assume_schema_id,
        expected_schema_id=args.expected_schema_id,
        require_schema_id=bool(args.require_schema_id),
    )
    dut_trace = load_commit_trace(
        args.dut_jsonl,
        assume_schema_id=assume_schema_id,
        expected_schema_id=args.expected_schema_id,
        require_schema_id=bool(args.require_schema_id),
    )

    if (
        ref_trace.commit_schema_id is not None
        and dut_trace.commit_schema_id is not None
        and ref_trace.commit_schema_id != dut_trace.commit_schema_id
    ):
        raise SystemExit(
            f"error: schema mismatch: ref={ref_trace.commit_schema_id!r} dut={dut_trace.commit_schema_id!r}"
        )

    ref = ref_trace.rows
    dut = dut_trace.rows

    if args.drop_boundary_selfloops:
        ref = _collapse_boundary_selfloops(ref)
        dut = _collapse_boundary_selfloops(dut)

    mm = first_mismatch(ref, dut, ignore_fields=ignore_fields, limit=args.limit)
    if mm is None:
        shown = (
            min(len(ref), args.limit)
            if args.limit is not None and args.limit >= 0
            else len(ref)
        )
        print(f"ok: traces match ({shown} commits)")
        return 0

    idx, field = mm
    if field == "<length>":
        print(
            f"mismatch: length differs: ref={len(ref)} dut={len(dut)} (first extra at idx={idx})"
        )
        if args.dump_dir:
            _dump_mismatch(
                dump_dir=Path(args.dump_dir),
                idx=idx,
                field=field,
                ref=ref,
                dut=dut,
                ref_schema_id=ref_trace.commit_schema_id,
                dut_schema_id=dut_trace.commit_schema_id,
                pre=max(0, int(args.dump_pre)),
                post=max(0, int(args.dump_post)),
            )
            print(f"dfx-dump: {Path(args.dump_dir).resolve()}")
        return 1

    ra = ref[idx].raw if idx < len(ref) else {}
    rb = dut[idx].raw if idx < len(dut) else {}
    print(f"mismatch: idx={idx} field={field}")
    print(f"  ref.{field}={fmt_hex(ra.get(field, None))}")
    print(f"  dut.{field}={fmt_hex(rb.get(field, None))}")
    # Print a short record summary to speed triage.
    for k in [
        "cycle",
        "pc",
        "insn",
        "len",
        "wb_valid",
        "wb_rd",
        "wb_data",
        "mem_valid",
        "mem_addr",
        "mem_wdata",
        "mem_rdata",
        "mem_size",
        "trap_valid",
        "trap_cause",
        "next_pc",
    ]:
        if k in ignore_fields:
            continue
        print(
            f"  ref.{k}={fmt_hex(ra.get(k, None))}  dut.{k}={fmt_hex(rb.get(k, None))}"
        )
    if args.dump_dir:
        _dump_mismatch(
            dump_dir=Path(args.dump_dir),
            idx=idx,
            field=field,
            ref=ref,
            dut=dut,
            ref_schema_id=ref_trace.commit_schema_id,
            dut_schema_id=dut_trace.commit_schema_id,
            pre=max(0, int(args.dump_pre)),
            post=max(0, int(args.dump_post)),
        )
        print(f"dfx-dump: {Path(args.dump_dir).resolve()}")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

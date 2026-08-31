#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

VALID_STATUS = {
    "implemented-verified",
    "implemented-unverified",
    "gap-in-scope",
    "deferred",
}

RFC_HEADER_RE = re.compile(r"^## Decision (\d{4}):", re.MULTILINE)
TABLE_ROW_RE = re.compile(
    r"^\|\s*(\d{4})\s*\|\s*([a-z-]+)\s*\|\s*(.*?)\s*\|\s*(.*?)\s*\|\s*(.*?)\s*\|\s*$"
)


def _split_evidence_paths(evidence: str) -> list[str]:
    if not evidence:
        return []
    parts: list[str] = []
    for piece in re.split(r"[,\s]+", evidence.strip()):
        p = piece.strip()
        if p:
            parts.append(p)
    return parts


def _load_decision_ids(rfc_path: Path) -> list[str]:
    text = rfc_path.read_text(encoding="utf-8")
    ids = sorted(set(RFC_HEADER_RE.findall(text)))
    if not ids:
        raise RuntimeError(f"no decision headers found in {rfc_path}")
    return ids


def _load_status_rows(status_path: Path) -> dict[str, dict[str, str]]:
    rows: dict[str, dict[str, str]] = {}
    for raw in status_path.read_text(encoding="utf-8").splitlines():
        m = TABLE_ROW_RE.match(raw.strip())
        if not m:
            continue
        did, status, evidence, owner, next_action = m.groups()
        if did in rows:
            raise RuntimeError(f"duplicate decision row in status file: {did}")
        rows[did] = {
            "decision": did,
            "status": status,
            "evidence": evidence,
            "owner": owner,
            "next_action": next_action,
        }
    if not rows:
        raise RuntimeError(f"no decision table rows parsed from {status_path}")
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Validate pyc6 decision status coverage and in-scope closure."
    )
    ap.add_argument(
        "--rfc",
        default="docs/rfcs/pyc6-decisions.md",
        help="Decision RFC path",
    )
    ap.add_argument(
        "--status",
        required=True,
        help="Decision status markdown table path",
    )
    ap.add_argument(
        "--out",
        required=True,
        help="Output JSON report path",
    )
    ap.add_argument(
        "--require-no-deferred",
        action="store_true",
        help="Fail when any decision status is 'deferred'.",
    )
    ap.add_argument(
        "--require-all-verified",
        action="store_true",
        help="Fail unless every decision status is 'implemented-verified'.",
    )
    ap.add_argument(
        "--require-concrete-evidence",
        action="store_true",
        help="Fail when any evidence field is empty or contains placeholders like <run-id>.",
    )
    ap.add_argument(
        "--require-existing-evidence",
        action="store_true",
        help="Fail when evidence paths do not exist on disk (paths resolved relative to repo root).",
    )
    ns = ap.parse_args()

    rfc_path = Path(ns.rfc).resolve()
    status_path = Path(ns.status).resolve()
    out_path = Path(ns.out).resolve()
    repo_root = status_path.parents[2]

    decision_ids = _load_decision_ids(rfc_path)
    rows = _load_status_rows(status_path)

    row_ids = sorted(rows.keys())
    missing = [d for d in decision_ids if d not in rows]
    extra = [d for d in row_ids if d not in set(decision_ids)]

    invalid_status = [d for d, row in rows.items() if row["status"] not in VALID_STATUS]
    gap_in_scope = sorted(
        [d for d, row in rows.items() if row["status"] == "gap-in-scope"]
    )
    deferred = sorted([d for d, row in rows.items() if row["status"] == "deferred"])
    non_verified = sorted(
        [d for d, row in rows.items() if row["status"] != "implemented-verified"]
    )

    placeholder_evidence: list[str] = []
    empty_evidence: list[str] = []
    missing_evidence_paths: dict[str, list[str]] = {}
    for did, row in rows.items():
        evidence = row["evidence"].strip()
        if not evidence:
            empty_evidence.append(did)
            continue
        if "<" in evidence or ">" in evidence:
            placeholder_evidence.append(did)
        if ns.require_existing_evidence:
            missing: list[str] = []
            for p in _split_evidence_paths(evidence):
                pp = Path(p)
                if not pp.is_absolute():
                    pp = (repo_root / pp).resolve()
                if not pp.exists():
                    missing.append(p)
            if missing:
                missing_evidence_paths[did] = missing

    require_no_deferred_failed = bool(ns.require_no_deferred and deferred)
    require_all_verified_failed = bool(ns.require_all_verified and non_verified)
    concrete_evidence_failed = bool(
        ns.require_concrete_evidence and (placeholder_evidence or empty_evidence)
    )
    existing_evidence_failed = bool(
        ns.require_existing_evidence and missing_evidence_paths
    )

    ok = (
        not missing
        and not extra
        and not invalid_status
        and not gap_in_scope
        and not require_no_deferred_failed
        and not require_all_verified_failed
        and not concrete_evidence_failed
        and not existing_evidence_failed
    )

    report = {
        "ok": ok,
        "rfc_path": str(rfc_path),
        "status_path": str(status_path),
        "total_decisions": len(decision_ids),
        "rows": len(rows),
        "missing_decisions": missing,
        "extra_decisions": extra,
        "invalid_status_decisions": invalid_status,
        "gap_in_scope_decisions": gap_in_scope,
        "deferred_decisions": deferred,
        "non_verified_decisions": non_verified,
        "placeholder_evidence_decisions": sorted(placeholder_evidence),
        "empty_evidence_decisions": sorted(empty_evidence),
        "missing_evidence_paths": dict(sorted(missing_evidence_paths.items())),
        "checks": {
            "require_no_deferred": bool(ns.require_no_deferred),
            "require_all_verified": bool(ns.require_all_verified),
            "require_concrete_evidence": bool(ns.require_concrete_evidence),
            "require_existing_evidence": bool(ns.require_existing_evidence),
        },
        "status_counts": {
            k: sum(1 for row in rows.values() if row["status"] == k)
            for k in sorted(VALID_STATUS)
        },
        "decisions": [rows[d] for d in sorted(rows.keys()) if d in rows],
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(report, indent=2, sort_keys=True), encoding="utf-8")

    if missing:
        print("error: missing decision rows:", " ".join(missing))
    if extra:
        print("error: unknown decision rows:", " ".join(extra))
    if invalid_status:
        print("error: invalid status values for:", " ".join(invalid_status))
    if gap_in_scope:
        print("error: unresolved in-scope decision gaps:", " ".join(gap_in_scope))
    if require_no_deferred_failed:
        print("error: deferred decisions remain:", " ".join(deferred))
    if require_all_verified_failed:
        print("error: non-verified decisions remain:", " ".join(non_verified))
    if concrete_evidence_failed:
        if empty_evidence:
            print("error: empty evidence fields:", " ".join(sorted(empty_evidence)))
        if placeholder_evidence:
            print(
                "error: placeholder evidence fields:",
                " ".join(sorted(placeholder_evidence)),
            )
    if existing_evidence_failed:
        bad = []
        for did, paths in sorted(missing_evidence_paths.items()):
            bad.append(f"{did}=>{','.join(paths)}")
        print("error: evidence paths do not exist:", " ".join(bad))

    if ok:
        print(
            "ok: decision status coverage validated "
            f"(rows={len(rows)} deferred={report['status_counts']['deferred']})"
        )
        return 0
    return 1


if __name__ == "__main__":
    raise SystemExit(main())

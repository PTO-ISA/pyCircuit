#!/usr/bin/env python3
"""Summarize a docs/gates/logs/<run-id>/ directory for CI Job Summary.

Reads known per-script summary files and optional CLI status rows, then writes
a Markdown gate matrix suitable for GitHub Actions `$GITHUB_STEP_SUMMARY`.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


KNOWN_SUMMARIES: tuple[tuple[str, str, str], ...] = (
    # (filename, gate name, level)
    ("summary.json", "run_examples", "G1"),
    ("semantic_regressions_summary.json", "semantic_regressions_v6", "G1"),
    ("run_sims_summary.json", "run_sims", "G2"),
    ("run_sims_nightly_summary.json", "run_sims_nightly", "G3"),
)


def _status_from_json(path: Path) -> tuple[str, str]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        return "unknown", f"unreadable: {exc}"
    status = str(data.get("status") or data.get("result") or "unknown")
    note = str(data.get("note") or "")
    if "results" in data and isinstance(data["results"], dict):
        # Aggregate matrix-style summary.json used by closure runs.
        results = data["results"]
        if not results:
            # Empty matrix must not collapse to an unconditional pass.
            status = "unknown"
            note = note or "empty results"
        else:
            parts = []
            for name, info in results.items():
                if isinstance(info, dict):
                    parts.append(f"{name}={info.get('status', 'unknown')}")
                else:
                    parts.append(f"{name}={info}")
            note = "; ".join(parts)
            if any(
                isinstance(info, dict)
                and str(info.get("status", "")).startswith("fail")
                for info in results.values()
            ):
                status = "fail"
            elif any(
                isinstance(info, dict) and "partial" in str(info.get("status", ""))
                for info in results.values()
            ):
                status = "partial"
            elif all(
                isinstance(info, dict) and str(info.get("status", "")) == "pass"
                for info in results.values()
            ):
                status = "pass"
            else:
                status = "unknown"
    return status, note


def _infer_file_status(log_dir: Path, stem: str) -> str | None:
    """Infer pass/fail from .rc / presence of stdout when no summary.json exists."""
    rc_path = log_dir / f"{stem}.rc"
    if rc_path.is_file():
        try:
            rc = int(rc_path.read_text(encoding="utf-8").strip() or "1")
        except ValueError:
            return "unknown"
        return "pass" if rc == 0 else "fail"
    if (log_dir / f"{stem}.stdout").is_file() or (log_dir / f"{stem}.stderr").is_file():
        return "ran"
    return None


def collect_rows(
    log_dir: Path,
    extra_rows: list[tuple[str, str, str, str]],
) -> list[tuple[str, str, str, str]]:
    rows: list[tuple[str, str, str, str]] = []
    seen: set[str] = set()

    for filename, gate, level in KNOWN_SUMMARIES:
        path = log_dir / filename
        if not path.is_file():
            continue
        status, note = _status_from_json(path)
        rows.append((gate, level, status, note))
        seen.add(gate)

    for stem, gate, level in (
        ("api_hygiene", "check_api_hygiene", "G0/G1"),
        ("decision_status", "check_decision_status", "G1"),
        ("semantic_regressions", "semantic_regressions_v6", "G1"),
        ("linx_cpu_pyc_cpp", "linx_cpu_pyc_cpp", "G3"),
    ):
        if gate in seen:
            continue
        status = _infer_file_status(log_dir, stem)
        if status is None:
            continue
        rows.append((gate, level, status, ""))
        seen.add(gate)

    for gate, level, status, note in extra_rows:
        # Prefer file-derived rows (richer notes); skip CLI duplicates.
        if gate in seen:
            continue
        rows.append((gate, level, status, note))
        seen.add(gate)

    return rows


def render_markdown(
    run_id: str, log_dir: Path, rows: list[tuple[str, str, str, str]]
) -> str:
    lines = [
        f"## Gate Matrix (run-id: `{run_id}`)",
        "",
        f"Log root: `{log_dir.as_posix()}`",
        "",
        "| Gate | Level | Status | Notes |",
        "|------|-------|--------|-------|",
    ]
    if not rows:
        lines.append("| _(no gate summaries found)_ | — | unknown | — |")
    else:
        for gate, level, status, note in rows:
            note_esc = note.replace("|", "\\|")
            lines.append(f"| `{gate}` | {level} | **{status}** | {note_esc} |")
    lines.extend(
        [
            "",
            f"Artifact name hint: `gate-logs-{run_id}`",
            "",
        ]
    )
    return "\n".join(lines)


def parse_row(raw: str) -> tuple[str, str, str, str]:
    # gate:level:status[:note]
    parts = raw.split(":", 3)
    if len(parts) < 3:
        raise argparse.ArgumentTypeError(
            f"--row expects gate:level:status[:note], got: {raw!r}"
        )
    gate, level, status = parts[0], parts[1], parts[2]
    note = parts[3] if len(parts) > 3 else ""
    return gate, level, status, note


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--run-id", required=True, help="Gate run id (PYC_GATE_RUN_ID)")
    ap.add_argument(
        "--log-root",
        default=str(_repo_root() / "docs" / "gates" / "logs"),
        help="Parent directory of <run-id> logs",
    )
    ap.add_argument(
        "--row",
        action="append",
        default=[],
        type=parse_row,
        help="Extra row gate:level:status[:note] (repeatable)",
    )
    ap.add_argument(
        "--append-to",
        default="",
        help="Optional path to append Markdown (e.g. $GITHUB_STEP_SUMMARY)",
    )
    ap.add_argument(
        "--require-pass",
        action="store_true",
        help="Exit 1 if any collected row status is fail/partial/unknown",
    )
    args = ap.parse_args()

    log_dir = Path(args.log_root) / args.run_id
    if not log_dir.is_dir():
        log_dir.mkdir(parents=True, exist_ok=True)

    rows = collect_rows(log_dir, list(args.row))
    md = render_markdown(args.run_id, log_dir, rows)
    sys.stdout.write(md)

    if args.append_to:
        out = Path(args.append_to)
        with out.open("a", encoding="utf-8") as fh:
            fh.write(md)

    if args.require_pass:
        bad = {"fail", "partial", "partial-timeout", "unknown"}
        if not rows:
            # Empty collected set means gate outputs are missing (wrong run-id
            # or empty log root); treat as failure so the gate is not vacuous.
            print(
                f"error: --require-pass set but no gate rows collected "
                f"(check run-id/log-root under {log_dir.as_posix()})",
                file=sys.stderr,
            )
            return 1
        if any(status in bad for _, _, status, _ in rows):
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

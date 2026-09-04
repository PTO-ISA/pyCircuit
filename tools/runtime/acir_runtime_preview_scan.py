#!/usr/bin/env python3
"""Bounded end-to-end preview matrix scanner for the Agentic Circuit runtime.

The scanner is intentionally a coordinator rather than a second RTL parser.  It
parses the design-class matrix in ``preview.html``, invokes the maintained
pyCircuit v0.14 crawler, and runs each available canonical adapter/configuration
as an isolated WSL process.  A case is checkpointed before it is reported as
complete, so an interrupted run can safely be resumed.

The crawler is discovery-only for classes that do not yet have a canonical
adapter.  Such classes are reported as ``DISCOVERED_NO_ADAPTER`` instead of
being assigned fabricated PPA numbers.
"""

from __future__ import annotations

import argparse
import csv
import html
import json
import re
import shlex
import subprocess
import time
from collections import Counter, defaultdict
from html.parser import HTMLParser
from pathlib import Path
from typing import Any, Iterable, Mapping


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CRAWLER_ROOT = REPO_ROOT / "tools" / "pycircuit_rtl_crawler_v0.14"
DEFAULT_CACHE_WORKDIR = REPO_ROOT / ".pycircuit_out" / "runtime-source-cache-v0.4"
DEFAULT_PREVIEW = REPO_ROOT / "docs" / "runtime" / "preview.html"
DEFAULT_LIBERTY = REPO_ROOT / "build" / "reference_libs" / "nangate45" / "NangateOpenCellLibrary_typical.lib"
WSL_PATH = "/opt/oss-cad/oss-cad-suite/bin:/usr/bin:/bin"


def _safe(value: Any) -> str:
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", str(value)).strip("_") or "unknown"


def _wsl_path(path: Path | str) -> str:
    raw = str(path)
    match = re.match(r"^([A-Za-z]):[\\/](.*)$", raw)
    if match:
        return "/mnt/" + match.group(1).lower() + "/" + match.group(2).replace("\\", "/")
    return raw.replace("\\", "/")


def _json_dump(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def _read_json(path: Path, default: Any = None) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return default


class PreviewMatrixParser(HTMLParser):
    """Parse the table subset of preview.html without BeautifulSoup."""

    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.heading: str = ""
        self.heading_index = 0
        self._in_h2 = False
        self._h2_parts: list[str] = []
        self._in_table = False
        self._in_row = False
        self._in_cell = False
        self._cell_tag = ""
        self._cell_parts: list[str] = []
        self._row: list[str] = []
        self._table_rows: list[list[str]] = []
        self._table_heading = ""
        self.targets: list[dict[str, str]] = []

    @staticmethod
    def _text(parts: Iterable[str]) -> str:
        return re.sub(r"\s+", " ", "".join(parts)).strip()

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        tag = tag.lower()
        if tag == "h2":
            self._in_h2 = True
            self._h2_parts = []
        elif tag == "table":
            self._in_table = True
            self._table_rows = []
            self._table_heading = self.heading
        elif tag == "tr" and self._in_table:
            self._in_row = True
            self._row = []
        elif tag in {"th", "td"} and self._in_row:
            self._in_cell = True
            self._cell_tag = tag
            self._cell_parts = []

    def handle_endtag(self, tag: str) -> None:
        tag = tag.lower()
        if tag == "h2" and self._in_h2:
            text = self._text(self._h2_parts)
            if text:
                self.heading_index += 1
                self.heading = text
            self._in_h2 = False
        elif tag in {"th", "td"} and self._in_cell:
            self._row.append(self._text(self._cell_parts))
            self._in_cell = False
        elif tag == "tr" and self._in_row:
            if self._row:
                self._table_rows.append(self._row)
            self._in_row = False
        elif tag == "table" and self._in_table:
            self._consume_table(self._table_heading, self._table_rows)
            self._in_table = False

    def handle_data(self, data: str) -> None:
        if self._in_h2:
            self._h2_parts.append(data)
        if self._in_cell:
            self._cell_parts.append(data)

    def _consume_table(self, heading: str, rows: list[list[str]]) -> None:
        if not rows:
            return
        header = [item.lower() for item in rows[0]]
        required = {"id", "design class", "level", "current", "priority", "domain"}
        if not required.issubset(set(header)):
            return
        positions = {name: header.index(name) for name in required}
        format_pos = header.index("format / configuration") if "format / configuration" in header else None
        for row in rows[1:]:
            if len(row) < len(header):
                row = row + [""] * (len(header) - len(row))
            item = {
                "family": heading,
                "family_index": str(self.heading_index),
                "target_id": row[positions["id"]],
                "design_class": row[positions["design class"]],
                "format": row[format_pos] if format_pos is not None else "",
                "level": row[positions["level"]],
                "current": row[positions["current"]],
                "priority": row[positions["priority"]],
                "domain": row[positions["domain"]],
            }
            if item["target_id"] and item["design_class"]:
                self.targets.append(item)


def parse_preview(path: Path) -> list[dict[str, str]]:
    parser = PreviewMatrixParser()
    parser.feed(path.read_text(encoding="utf-8", errors="replace"))
    # Keep the first row for duplicate IDs but preserve all distinct design
    # points.  The preview occasionally repeats a class in a second roadmap.
    result: list[dict[str, str]] = []
    seen: set[tuple[str, str, str]] = set()
    for item in parser.targets:
        key = (item["target_id"], item["design_class"], item["format"])
        if key not in seen:
            result.append(item)
            seen.add(key)
    return result


def write_csv(path: Path, rows: list[Mapping[str, Any]], fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def run_wsl(command: str, *, cwd: Path | None, timeout: int) -> dict[str, Any]:
    """Run one bounded command in WSL and retain a short diagnostic tail."""
    # Do not append the inherited Windows PATH here.  WSL interop expands that
    # value into colon-separated Windows entries (some containing spaces),
    # which makes ``export`` see several arguments and can select a host g++.
    shell_command = f"export PATH={WSL_PATH}; {command}"
    argv = ["wsl.exe", "--", "/bin/bash", "-lc", shell_command]
    started = time.monotonic()
    try:
        result = subprocess.run(
            argv,
            cwd=str(cwd) if cwd else None,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=max(1, timeout),
            check=False,
        )
        return {
            "status": "PASS" if result.returncode == 0 else "FAIL",
            "returncode": result.returncode,
            "elapsed_s": round(time.monotonic() - started, 3),
            "stdout": (result.stdout or "")[-12000:],
            "stderr": (result.stderr or "")[-12000:],
            "argv": argv,
        }
    except subprocess.TimeoutExpired as exc:
        return {
            "status": "TIMEOUT",
            "returncode": None,
            "elapsed_s": round(time.monotonic() - started, 3),
            "stdout": str(exc.stdout or "")[-12000:] if exc.stdout else "",
            "stderr": str(exc.stderr or "")[-12000:] if exc.stderr else "",
            "argv": argv,
        }
    except OSError as exc:
        return {"status": "ERROR", "returncode": None, "elapsed_s": round(time.monotonic() - started, 3), "error": str(exc), "argv": argv}


def load_yaml_or_json(path: Path) -> dict[str, Any]:
    text = path.read_text(encoding="utf-8")
    if path.suffix.lower() == ".json":
        value = json.loads(text)
    else:
        try:
            import yaml  # type: ignore
        except ImportError as exc:
            raise RuntimeError("PyYAML is required to read design_class_specs.yaml") from exc
        value = yaml.safe_load(text)
    if not isinstance(value, dict):
        raise RuntimeError(f"expected a mapping in {path}")
    return value


def write_subset_specs(path: Path, specs: Mapping[str, Any], class_id: str, candidate: Mapping[str, Any], config: Mapping[str, Any], profile: str) -> None:
    dc = dict(specs["design_classes"][class_id])
    candidate_copy = dict(candidate)
    # cc_rr_arb_tree itself only needs cc_pkg, but common_cells' static
    # closure also sees the optional assertion-report package, which imports
    # UVM.  UVM is intentionally outside the runtime primitive contract.
    if class_id == "DF-09" and candidate_copy.get("project") == "pulp_common_cells":
        candidate_copy.setdefault("prune_packages", ["assert_rpt_pkg", "uvm_pkg"])
    dc["candidates"] = [candidate_copy]
    dc["profiles"] = {profile: [dict(config)]}
    subset = {"schema_version": specs.get("schema_version", "0.14"), "design_classes": {class_id: dc}}
    _json_dump(path, subset)


def read_crawler_outputs(crawl_dir: Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]]]:
    def read_csv(name: str) -> list[dict[str, Any]]:
        path = crawl_dir / name
        if not path.exists():
            return []
        with path.open(newline="", encoding="utf-8") as stream:
            return list(csv.DictReader(stream))
    return read_csv("candidates_raw.csv"), read_csv("candidate_details.csv")


def merge_sources(base_path: Path, extra_path: Path | None, *, include_disabled: bool, enable_extra: bool, extra_projects: list[str] | None = None, max_extra_sources: int = 0, output: Path) -> Path:
    base = json.loads(base_path.read_text(encoding="utf-8"))
    sources = list(base.get("sources", []))
    if include_disabled:
        for item in sources:
            item["enabled"] = True
    if extra_path and extra_path.exists() and enable_extra:
        extra = json.loads(extra_path.read_text(encoding="utf-8"))
        existing = {item.get("project") for item in sources}
        selected = extra.get("sources", [])
        if extra_projects:
            wanted = set(extra_projects)
            selected = [item for item in selected if item.get("project") in wanted]
        if max_extra_sources:
            selected = selected[:max_extra_sources]
        for item in selected:
            if item.get("project") not in existing:
                enabled_item = dict(item)
                enabled_item["enabled"] = True
                sources.append(enabled_item)
    output.parent.mkdir(parents=True, exist_ok=True)
    _json_dump(output, {"sources": sources})
    return output


def find_report(case_result_root: Path, class_id: str, profile: str) -> Path | None:
    direct = case_result_root / class_id / profile / "comparison_report.json"
    if direct.exists():
        return direct
    matches = list(case_result_root.rglob("comparison_report.json"))
    return matches[0] if matches else None


def case_key(class_id: str, project: str, module: str, config: str) -> str:
    return "|".join((class_id, project, module, config))


def run_ppa_case(
    *,
    args: argparse.Namespace,
    specs: Mapping[str, Any],
    class_id: str,
    candidate: Mapping[str, Any],
    config: Mapping[str, Any],
    output: Path,
    candidate_root: Path,
    liberty: Path | None,
) -> dict[str, Any]:
    project = str(candidate["project"])
    module = str(candidate["module"])
    config_name = str(config["name"])
    key = case_key(class_id, project, module, config_name)
    case_dir = output / "cases" / _safe(class_id) / _safe(project) / _safe(module) / _safe(config_name)
    case_dir.mkdir(parents=True, exist_ok=True)
    checkpoint = case_dir / "case_result.json"
    if args.resume and checkpoint.exists():
        prior = _read_json(checkpoint)
        if isinstance(prior, dict) and prior.get("case_key") == key:
            prior["resumed"] = True
            return prior

    subset_specs = case_dir / "specs.json"
    write_subset_specs(subset_specs, specs, class_id, candidate, config, args.profile)
    result_root = case_dir / "runner"
    result_root.mkdir(parents=True, exist_ok=True)
    command = [
        "python3", "run_design_class.py",
        "--class-id", class_id,
        "--profile", args.profile,
        "--specs", _wsl_path(subset_specs),
        "--sources", _wsl_path(args.sources),
        "--workdir", _wsl_path(args.cache_workdir),
        "--candidate-root", _wsl_path(candidate_root),
        "--output-root", _wsl_path(result_root),
        "--frontend", args.frontend,
    ]
    if candidate.get("source_hint"):
        # source_hint is read from the subset spec by build_candidate.py.
        pass
    if args.cxx:
        command += ["--cxx", args.cxx]
    if args.skip_correctness:
        command.append("--skip-correctness")
    if args.skip_synthesis:
        command.append("--skip-synthesis")
    if args.update:
        command.append("--update")
    if liberty:
        command += ["--liberty", _wsl_path(liberty)]
    command_text = "cd " + shlex.quote(_wsl_path(args.crawler_root)) + " && " + " ".join(shlex.quote(item) for item in command)
    run = run_wsl(command_text, cwd=None, timeout=args.case_timeout)
    (case_dir / "runner_stdout.log").write_text(run.get("stdout", ""), encoding="utf-8")
    (case_dir / "runner_stderr.log").write_text(run.get("stderr", ""), encoding="utf-8")
    row: dict[str, Any] = {
        "case_key": key,
        "class_id": class_id,
        "operation": specs["design_classes"][class_id].get("operation", ""),
        "project": project,
        "module": module,
        "adapter": candidate.get("adapter", ""),
        "config": config_name,
        "parameters": {k: v for k, v in config.items() if k != "name"},
        "runner_status": run["status"],
        "runner_elapsed_s": run.get("elapsed_s"),
        "correctness": "NOT_RUN",
        "synthesis": "NOT_RUN",
        "mapped_area": None,
        "logic_depth": None,
        "cells": None,
        "power": None,
        "power_proxy": None,
        "pareto": False,
        "source_hint": candidate.get("source_hint", ""),
        "provenance_note": "Power is null: no VCD/SAIF or signoff power engine was run; mapped_area is pre-layout Nangate45 area and logic_depth is a Yosys proxy.",
    }
    report_path = find_report(result_root, class_id, args.profile)
    if report_path:
        report = _read_json(report_path, {})
        rows = report.get("rows", []) if isinstance(report, dict) else []
        if rows:
            ppa = rows[0]
            row.update({
                "closure": ppa.get("closure"),
                "build": ppa.get("build"),
                "correctness": ppa.get("correctness"),
                "synthesis": ppa.get("synthesis"),
                "cells": ppa.get("cells"),
                "logic_depth": ppa.get("depth"),
                "mapped_area": ppa.get("area"),
                "runner_report": str(report_path),
            })
            if isinstance(row.get("cells"), int):
                row["power_proxy"] = row["cells"]
    _json_dump(checkpoint, row)
    return row


def mark_pareto(rows: list[dict[str, Any]]) -> None:
    for row in rows:
        row["pareto"] = False
    usable = [
        row for row in rows
        if row.get("correctness") == "PASS" and row.get("synthesis") == "PASS"
        and isinstance(row.get("logic_depth"), (int, float))
        and (isinstance(row.get("mapped_area"), (int, float)) or isinstance(row.get("cells"), (int, float)))
    ]
    def metrics(row: Mapping[str, Any]) -> tuple[float, float]:
        area = row.get("mapped_area") if isinstance(row.get("mapped_area"), (int, float)) else row.get("cells")
        return float(area), float(row["logic_depth"])
    for row in usable:
        a_area, a_depth = metrics(row)
        row["pareto"] = not any(
            metrics(other)[0] <= a_area and metrics(other)[1] <= a_depth
            and metrics(other) != (a_area, a_depth)
            for other in usable if other is not row
        )


def build_html(report: Mapping[str, Any], path: Path) -> None:
    coverage = report.get("coverage", {})
    ppa_rows = report.get("ppa_rows", [])
    target_rows = report.get("targets", [])
    def esc(value: Any) -> str:
        return html.escape("" if value is None else str(value))
    target_html = "".join(
        "<tr>" + "".join(f"<td>{esc(item.get(field, ''))}</td>" for field in ("family", "target_id", "design_class", "format", "priority", "match_status", "candidate_count")) + "</tr>"
        for item in target_rows
    )
    ppa_html = "".join(
        "<tr>" + "".join(f"<td>{esc(item.get(field, ''))}</td>" for field in ("class_id", "project", "module", "config", "correctness", "synthesis", "cells", "logic_depth", "mapped_area", "power", "power_proxy", "pareto")) + "</tr>"
        for item in ppa_rows
    )
    family_html = "".join(f"<li>{esc(name)}: {count}</li>" for name, count in sorted(coverage.get("families", {}).items()))
    doc = f"""<!doctype html>
<html><head><meta charset='utf-8'><title>Agentic Circuit Runtime Preview Scan</title>
<style>body{{font-family:Arial,sans-serif;margin:28px;color:#1f2937}}table{{border-collapse:collapse;width:100%;margin:12px 0 28px}}th,td{{border:1px solid #d1d5db;padding:6px 8px;font-size:13px}}th{{background:#f3f4f6;position:sticky;top:0}}.grid{{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}}.card{{background:#f3f4f6;padding:12px;border-radius:8px}}.note{{background:#fff7ed;padding:12px;border-left:4px solid #f97316}}.pass{{color:#047857}}.fail{{color:#b91c1c}}pre{{background:#111827;color:#e5e7eb;padding:12px;overflow:auto}}</style></head>
<body><h1>Agentic Circuit Runtime Preview Scan</h1>
<p>Generated {esc(report.get('generated_at'))}. Discovery covers preview.html; PPA is executed only for classes with a canonical pyCircuit adapter.</p>
<div class='grid'><div class='card'><b>Preview targets</b><br>{esc(coverage.get('target_count'))}</div><div class='card'><b>Matched</b><br>{esc(coverage.get('matched_targets'))}</div><div class='card'><b>Candidate matches</b><br>{esc(coverage.get('candidate_matches'))}</div><div class='card'><b>PPA rows</b><br>{esc(len(ppa_rows))}</div></div>
<h2>Family coverage</h2><ul>{family_html}</ul>
<h2>Target coverage</h2><table><thead><tr><th>Family</th><th>ID</th><th>Design Class</th><th>Format</th><th>Priority</th><th>Match</th><th>Candidates</th></tr></thead><tbody>{target_html}</tbody></table>
<h2>PPA / parameter sweep</h2><div class='note'>`mapped_area` is pre-layout area from the selected Liberty library. `logic_depth` is a Yosys longest-topological-path proxy. `power` is intentionally null because this flow does not run VCD/SAIF-based dynamic power; `power_proxy` is cell count only and must not be interpreted as watts.</div>
<table><thead><tr><th>Class</th><th>Project</th><th>Module</th><th>Config</th><th>Correctness</th><th>Synthesis</th><th>Cells</th><th>Depth</th><th>Area</th><th>Power</th><th>Power proxy</th><th>Pareto</th></tr></thead><tbody>{ppa_html}</tbody></table>
<h2>Reproducibility</h2><pre>{esc(json.dumps(report.get('commands', []), indent=2, ensure_ascii=False))}</pre>
</body></html>"""
    path.write_text(doc, encoding="utf-8")


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preview", type=Path, default=DEFAULT_PREVIEW)
    parser.add_argument("--crawler-root", type=Path, default=DEFAULT_CRAWLER_ROOT)
    parser.add_argument("--cache-workdir", type=Path, default=DEFAULT_CACHE_WORKDIR)
    parser.add_argument("--sources", type=Path, default=None, help="v0.14 sources.json; defaults to crawler-root/sources.json")
    parser.add_argument("--targets", type=Path, default=None, help="v0.14 targets.json; defaults to crawler-root/targets.json")
    parser.add_argument("--specs", type=Path, default=None, help="design_class_specs.yaml; defaults to crawler-root/design_class_specs.yaml")
    parser.add_argument("--extra-sources", type=Path, default=REPO_ROOT / "tools" / "preview_sources_extended.json")
    parser.add_argument("--enable-extra-sources", action="store_true", help="add optional GitHub/GitLab/Codeberg sources; use --update to clone missing repositories")
    parser.add_argument("--extra-project", action="append", default=[], help="select an optional source project (repeatable); by default all entries in preview_sources_extended.json are selected")
    parser.add_argument("--max-extra-sources", type=int, default=0, help="cap optional sources appended in this run; 0 means no cap")
    parser.add_argument("--include-disabled-sources", action="store_true", help="enable disabled entries in the v0.14 manifest")
    parser.add_argument("--update", action="store_true", help="allow crawler/build_candidate to clone or update repositories")
    parser.add_argument("--profile", choices=("smoke", "standard", "scaling"), default="scaling")
    parser.add_argument("--output", type=Path, default=REPO_ROOT / ".pycircuit_out" / "preview-runtime-scan")
    parser.add_argument("--frontend", choices=("auto", "native", "slang"), default="auto")
    parser.add_argument("--liberty", type=Path, default=DEFAULT_LIBERTY)
    parser.add_argument("--no-liberty", action="store_true")
    parser.add_argument("--skip-ppa", action="store_true")
    parser.add_argument("--skip-correctness", action="store_true")
    parser.add_argument("--skip-synthesis", action="store_true")
    parser.add_argument("--max-workers", type=int, default=1, help="reserved for future parallelism; values >1 are rejected to protect the host")
    parser.add_argument("--max-candidates", type=int, default=0, help="limit PPA candidates per class; 0 means all candidates in specs")
    parser.add_argument("--crawler-timeout", type=int, default=900)
    parser.add_argument("--case-timeout", type=int, default=300)
    # Ubuntu 20.04's g++ 9 does not understand the coroutine flag emitted by
    # the bundled Verilator; g++-10 is installed alongside the OSS CAD suite.
    parser.add_argument("--cxx", default="g++-10")
    parser.add_argument("--resume", action="store_true", default=True)
    parser.add_argument("--no-resume", dest="resume", action="store_false")
    parser.add_argument("--dry-run", action="store_true", help="parse and report planned work without invoking crawler or PPA")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if args.max_workers != 1:
        raise SystemExit("--max-workers must remain 1 in the bounded runner; parallel execution is intentionally disabled")
    args.crawler_root = args.crawler_root.resolve()
    args.cache_workdir = args.cache_workdir.resolve()
    args.sources = (args.sources or args.crawler_root / "sources.json").resolve()
    args.targets = (args.targets or args.crawler_root / "targets.json").resolve()
    args.specs = (args.specs or args.crawler_root / "design_class_specs.yaml").resolve()
    args.preview = args.preview.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    if not args.preview.exists():
        raise SystemExit(f"preview.html not found: {args.preview}")

    matrix = parse_preview(args.preview)
    _json_dump(args.output / "preview_matrix.json", matrix)
    write_csv(args.output / "preview_matrix.csv", matrix, ["family", "family_index", "target_id", "design_class", "format", "level", "current", "priority", "domain"])

    merged_sources = merge_sources(
        args.sources,
        args.extra_sources,
        include_disabled=args.include_disabled_sources,
        enable_extra=args.enable_extra_sources,
        extra_projects=args.extra_project,
        max_extra_sources=max(0, args.max_extra_sources),
        output=args.output / "sources.merged.json",
    )
    coverage: dict[str, Any] = {"target_count": len(matrix), "families": dict(Counter(item["family"] for item in matrix))}
    crawler_result: dict[str, Any] = {"status": "SKIPPED", "output": str(args.output / "crawl")}
    crawl_dir = args.output / "crawl"
    commands: list[str] = []
    if not args.dry_run:
        command = [
            "python3", "crawler.py",
            "--sources", _wsl_path(merged_sources),
            "--targets", _wsl_path(args.targets),
            "--workdir", _wsl_path(args.cache_workdir),
            "--output", _wsl_path(crawl_dir),
        ]
        if not args.update:
            command.append("--no-update")
        crawler_command = "cd " + shlex.quote(_wsl_path(args.crawler_root)) + " && " + " ".join(shlex.quote(item) for item in command)
        commands.append(crawler_command)
        crawler_result = run_wsl(crawler_command, cwd=None, timeout=args.crawler_timeout)
        (args.output / "crawl_stdout.log").write_text(crawler_result.get("stdout", ""), encoding="utf-8")
        (args.output / "crawl_stderr.log").write_text(crawler_result.get("stderr", ""), encoding="utf-8")
    raw_hits, details = read_crawler_outputs(crawl_dir)
    write_csv(args.output / "candidate_inventory.csv", details, list(details[0].keys()) if details else ["source_project", "module", "file"])
    candidate_by_target = Counter(str(item.get("target_id", "")) for item in raw_hits)
    coverage.update({
        "crawler_status": crawler_result.get("status"),
        "candidate_matches": len(raw_hits),
        "candidate_modules": len(details),
        "matched_targets": len(candidate_by_target),
        "unmatched_targets": len(matrix) - len(candidate_by_target),
    })
    target_rows = []
    for item in matrix:
        # targets.json IDs are P0/P1 names, while preview IDs are INT-05 etc.
        matches = [hit for hit in raw_hits if hit.get("gap_id") == item["target_id"] or hit.get("target_id") == item["target_id"]]
        target_rows.append({**item, "match_status": "MATCHED" if matches else "UNMATCHED", "candidate_count": len(matches)})

    specs = load_yaml_or_json(args.specs)
    ppa_rows: list[dict[str, Any]] = []
    available_classes = set(specs.get("design_classes", {}))
    planned: list[dict[str, Any]] = []
    candidate_root = args.output / "candidate-builds"
    liberty = None if args.no_liberty else args.liberty.resolve()
    if liberty and not liberty.exists():
        liberty = None
    for class_id, dc in specs.get("design_classes", {}).items():
        configs = dc.get("profiles", {}).get(args.profile, [])
        candidates = dc.get("candidates", [])
        if args.max_candidates:
            candidates = candidates[: args.max_candidates]
        for candidate in candidates:
            for config in configs:
                planned.append({"class_id": class_id, "project": candidate.get("project"), "module": candidate.get("module"), "config": config.get("name")})
                if args.skip_ppa or args.dry_run:
                    continue
                ppa_rows.append(run_ppa_case(args=args, specs=specs, class_id=class_id, candidate=candidate, config=config, output=args.output, candidate_root=candidate_root, liberty=liberty))
    # Carry immutable source provenance from the crawler into every PPA row.
    # This makes a result auditable without opening the nested runner report.
    detail_index = {(str(item.get("source_project")), str(item.get("module"))): item for item in details}
    for row in ppa_rows:
        meta = detail_index.get((str(row.get("project")), str(row.get("module"))), {})
        row.update({
            "repo_url": meta.get("repo_url", ""),
            "commit_sha": meta.get("commit_sha", ""),
            "branch": meta.get("branch", ""),
            "source_file": meta.get("file", ""),
            "license": meta.get("license", ""),
        })
    # Mark each configuration's frontier independently; configurations with
    # different widths/capacities are never compared against each other.
    grouped: dict[tuple[str, str], list[dict[str, Any]]] = defaultdict(list)
    for row in ppa_rows:
        grouped[(str(row["class_id"]), str(row["config"]))].append(row)
    for group in grouped.values():
        mark_pareto(group)

    write_csv(args.output / "coverage.csv", target_rows, ["family", "target_id", "design_class", "format", "level", "current", "priority", "domain", "match_status", "candidate_count"])
    ppa_fields = ["class_id", "operation", "project", "module", "adapter", "config", "parameters", "runner_status", "closure", "build", "correctness", "synthesis", "cells", "logic_depth", "mapped_area", "power", "power_proxy", "pareto", "repo_url", "commit_sha", "branch", "source_file", "license", "source_hint", "provenance_note"]
    for row in ppa_rows:
        row["parameters"] = json.dumps(row.get("parameters", {}), sort_keys=True)
    write_csv(args.output / "ppa_rows.csv", ppa_rows, ppa_fields)
    _json_dump(args.output / "ppa_summary.json", {"profile": args.profile, "rows": ppa_rows, "planned": planned, "available_adapter_classes": sorted(available_classes), "power_semantics": "power=null; power_proxy=cell count only"})

    report = {
        "schema_version": "0.1",
        "generated_at": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "preview": str(args.preview),
        "crawler_root": str(args.crawler_root),
        "coverage": coverage,
        "targets": target_rows,
        "ppa_rows": ppa_rows,
        "available_adapter_classes": sorted(available_classes),
        "planned_cases": planned,
        "commands": commands,
        "limits": {"max_workers": args.max_workers, "crawler_timeout_s": args.crawler_timeout, "case_timeout_s": args.case_timeout, "resume": args.resume, "update": args.update},
        "power_semantics": "No true dynamic power is measured. power is null; power_proxy is mapped cell count only.",
    }
    _json_dump(args.output / "coverage.json", coverage)
    _json_dump(args.output / "report.json", report)
    build_html(report, args.output / "report.html")
    print(json.dumps({
        "output": str(args.output),
        "preview_targets": len(matrix),
        "matched_targets": coverage.get("matched_targets", 0),
        "candidate_matches": coverage.get("candidate_matches", 0),
        "ppa_rows": len(ppa_rows),
        "planned_cases": len(planned),
        "crawler_status": crawler_result.get("status"),
    }, ensure_ascii=False, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

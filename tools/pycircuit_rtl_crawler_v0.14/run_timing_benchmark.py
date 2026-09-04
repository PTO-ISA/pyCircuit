#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import hashlib
import html
import json
import re
import shutil
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import yaml


SLACK_RE = re.compile(
    r"^\s*([-+]?\d+(?:\.\d+)?)\s+slack\s+\((?:MET|VIOLATED)\)",
    re.MULTILINE,
)
START_RE = re.compile(r"^Startpoint:\s*(.+)$", re.MULTILINE)
END_RE = re.compile(r"^Endpoint:\s*(.+)$", re.MULTILINE)
GROUP_RE = re.compile(r"^Path Group:\s*(.+)$", re.MULTILINE)
OPENSTA_ERROR_RE = re.compile(r"^Error:\s+(.+)$", re.MULTILINE)

RESIDUAL_FORMAL_RE = re.compile(
    r"\b(?:initial|assert\s*\(|assume\s*\(|cover\s*\(|restrict\s*\()"
)


def residual_formal_constructs(path: Path):
    text = path.read_text(encoding="utf-8", errors="replace")
    return [
        text.count("\n", 0, m.start()) + 1
        for m in RESIDUAL_FORMAL_RE.finditer(text)
    ]


def _as_text(data):
    if data is None:
        return ""
    if isinstance(data, bytes):
        return data.decode("utf-8", errors="replace")
    return str(data)


def run(cmd, cwd=None, timeout=None):
    try:
        return subprocess.run(
            cmd,
            cwd=str(cwd) if cwd else None,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as e:
        class TimeoutResult:
            returncode = 124
            stdout = _as_text(e.stdout)
            stderr = _as_text(e.stderr) + f"\nTIMEOUT after {timeout}s"
        return TimeoutResult()


def opensta_errors(text: str):
    return OPENSTA_ERROR_RE.findall(text)


def _cache_key(path: Path) -> str:
    st = path.stat()
    payload = f"{path.resolve()}|{st.st_size}|{st.st_mtime_ns}".encode()
    return hashlib.sha256(payload).hexdigest()[:16]


def stage_liberty_to_linux_fs(liberty: Path, scratch_root: Path) -> Path:
    cache_dir = scratch_root / "liberty_cache"
    cache_dir.mkdir(parents=True, exist_ok=True)

    key = _cache_key(liberty)
    cached = cache_dir / f"{key}_{liberty.name}"
    if not cached.exists() or cached.stat().st_size != liberty.stat().st_size:
        tmp = cached.with_suffix(cached.suffix + ".tmp")
        shutil.copy2(liberty, tmp)
        tmp.replace(cached)
    return cached


def safe_name(s: str) -> str:
    return "".join(c if c.isalnum() or c in "._-" else "_" for c in str(s))


def stage_case_netlist(
    netlist: Path,
    scratch_root: Path,
    class_id: str,
    project: str,
    module: str,
    config: str,
) -> Path:
    case_dir = (
        scratch_root
        / "cases"
        / safe_name(class_id)
        / safe_name(project)
        / safe_name(module)
        / safe_name(config)
    )
    case_dir.mkdir(parents=True, exist_ok=True)
    staged = case_dir / "mapped_netlist.v"
    shutil.copy2(netlist, staged)
    return staged


def parse_timing_report(text: str, period_ns: float):
    slacks = [float(x) for x in SLACK_RE.findall(text)]
    if not slacks:
        return {
            "status": "PARSE_FAIL",
            "worst_slack_ns": None,
            "critical_delay_ns": None,
            "fmax_proxy_mhz": None,
            "startpoint": "",
            "endpoint": "",
            "path_group": "",
        }

    worst_slack = min(slacks)
    delay = period_ns - worst_slack
    fmax = (1000.0 / delay) if delay > 0 else None

    starts = START_RE.findall(text)
    ends = END_RE.findall(text)
    groups = GROUP_RE.findall(text)

    return {
        "status": "PASS",
        "worst_slack_ns": worst_slack,
        "critical_delay_ns": delay,
        "fmax_proxy_mhz": fmax,
        "startpoint": starts[-1].strip() if starts else "",
        "endpoint": ends[-1].strip() if ends else "",
        "path_group": groups[-1].strip() if groups else "",
    }


def _get_ports(pattern: str) -> str:
    # OpenSTA Tcl. Braces protect bus wildcard characters from Tcl expansion.
    return f"[get_ports {{{pattern}}}]"


def write_sta_script(
    path: Path,
    liberty: Path,
    netlist: Path,
    period_ns: float,
    contract: dict,
):
    clock_port = contract.get("clock_port")
    virtual_clock = contract.get("virtual_clock", "vclk")
    timed_inputs = contract.get("timed_inputs", [])
    timed_outputs = contract.get("timed_outputs", [])
    false_inputs = contract.get("false_path_inputs", [])

    lines = [
        f"read_liberty {{{liberty}}}",
        f"read_verilog {{{netlist}}}",
        "link_design pyc_synth_top",
        "",
    ]

    if clock_port:
        clock_name = "clk"
        lines.append(
            f"create_clock -name {clock_name} -period {period_ns:.6f} "
            f"{_get_ports(clock_port)}"
        )
    else:
        # Pure combinational class: use a virtual clock so input-to-output
        # paths receive the same reference budget without inventing a DUT clock.
        clock_name = virtual_clock
        lines.append(
            f"create_clock -name {clock_name} -period {period_ns:.6f}"
        )

    lines += ["", "# Canonical zero-I/O-budget timing contract."]

    for pat in timed_inputs:
        lines.append(
            f"set_input_delay -clock {clock_name} 0.0 {_get_ports(pat)}"
        )

    for pat in timed_outputs:
        lines.append(
            f"set_output_delay -clock {clock_name} 0.0 {_get_ports(pat)}"
        )

    for pat in false_inputs:
        lines.append(
            f"set_false_path -from {_get_ports(pat)}"
        )

    lines += [
        "",
        "report_checks -path_delay max -group_count 1",
        "exit",
        "",
    ]
    path.write_text("\n".join(lines), encoding="utf-8")


def config_key(row: dict) -> str:
    return str(row.get("config", ""))


def pareto_area_delay(rows):
    for r in rows:
        r["area_timing_pareto"] = False

    usable = [
        r for r in rows
        if isinstance(r.get("mapped_area"), float)
        and isinstance(r.get("critical_delay_ns"), float)
        and r.get("timing_status") == "PASS"
    ]

    by_cfg = {}
    for r in usable:
        by_cfg.setdefault(config_key(r), []).append(r)

    for group in by_cfg.values():
        for a in group:
            dominated = False
            for b in group:
                if a is b:
                    continue
                if (
                    b["mapped_area"] <= a["mapped_area"]
                    and b["critical_delay_ns"] <= a["critical_delay_ns"]
                    and (
                        b["mapped_area"] < a["mapped_area"]
                        or b["critical_delay_ns"] < a["critical_delay_ns"]
                    )
                ):
                    dominated = True
                    break
            a["area_timing_pareto"] = not dominated


def best_by_config(rows):
    out = {}
    configs = sorted({config_key(r) for r in rows})
    for cfg in configs:
        good = [
            r for r in rows
            if config_key(r) == cfg
            and r.get("timing_status") == "PASS"
            and isinstance(r.get("mapped_area"), float)
            and isinstance(r.get("critical_delay_ns"), float)
        ]
        if not good:
            continue
        area = min(good, key=lambda r: r["mapped_area"])
        timing = min(good, key=lambda r: r["critical_delay_ns"])
        pareto = [r["record_id"] for r in good if r.get("area_timing_pareto")]
        out[cfg] = {
            "area_winner": area["record_id"],
            "timing_winner": timing["record_id"],
            "pareto_set": pareto,
        }
    return out


def make_html(
    rows,
    out: Path,
    class_id: str,
    operation: str,
    period_ns: float,
    liberty: Path,
    sta_version: str,
    timing_semantics: str,
):
    trs = []
    for r in sorted(
        rows,
        key=lambda x: (
            config_key(x),
            x["mapped_area"] if x["mapped_area"] is not None else 1e30,
        ),
    ):
        area = "" if r["mapped_area"] is None else f"{r['mapped_area']:.4f}"
        delay = (
            ""
            if r["critical_delay_ns"] is None
            else f"{r['critical_delay_ns']:.4f}"
        )
        fmax = (
            ""
            if r["fmax_proxy_mhz"] is None
            else f"{r['fmax_proxy_mhz']:.2f}"
        )
        trs.append(
            "<tr>"
            f"<td>{html.escape(r['config'])}</td>"
            f"<td>{html.escape(r['project'])}</td>"
            f"<td>{html.escape(r['module'])}</td>"
            f"<td>{area}</td>"
            f"<td>{delay}</td>"
            f"<td>{fmax}</td>"
            f"<td>{html.escape(r['timing_status'])}</td>"
            f"<td>{'YES' if r.get('area_timing_pareto') else ''}</td>"
            f"<td>{html.escape(r.get('startpoint',''))}</td>"
            f"<td>{html.escape(r.get('endpoint',''))}</td>"
            "</tr>"
        )

    doc = f"""<!doctype html>
<html><head><meta charset="utf-8">
<title>pyCircuit {html.escape(class_id)} Area × Timing</title>
<style>
body{{font-family:Arial,sans-serif;margin:32px;color:#1f2937;line-height:1.55}}
table{{border-collapse:collapse;width:100%;font-size:13px}}
th,td{{border:1px solid #d1d5db;padding:7px;text-align:left}}
th{{background:#f3f4f6}}
.note{{background:#f8fafc;border-left:4px solid #2563eb;padding:12px 14px;margin:16px 0}}
.warn{{background:#fff7ed;border-left-color:#ea580c}}
</style></head><body>
<h1>{html.escape(class_id)} · {html.escape(operation)}</h1>

<div class="note">
<b>Liberty:</b> {html.escape(str(liberty))}<br>
<b>OpenSTA:</b> {html.escape(sta_version)}<br>
<b>Reference clock:</b> {period_ns:.3f} ns<br>
<b>Timing contract:</b> {html.escape(timing_semantics)}
</div>

<div class="note warn">
<b>Interpretation:</b> Critical delay and Fmax are pre-layout proxies.
The flow uses mapped cells but no SPEF/parasitics, CTS, routing delay,
realistic input slew or output load. Do not report these as signoff Fmax.
</div>

<table>
<thead><tr>
<th>Config</th><th>Project</th><th>Module</th><th>Mapped Area</th>
<th>Critical Delay (ns)</th><th>Fmax Proxy (MHz)</th><th>Timing</th>
<th>Area×Timing Pareto</th><th>Startpoint</th><th>Endpoint</th>
</tr></thead>
<tbody>{''.join(trs)}</tbody>
</table>
</body></html>"""
    out.write_text(doc, encoding="utf-8")


def main():
    ap = argparse.ArgumentParser(
        description="Generic design-class-aware OpenSTA benchmark."
    )
    ap.add_argument("--class-id", required=True)
    ap.add_argument(
        "--profile",
        choices=["smoke", "standard", "scaling"],
        default="scaling",
    )
    ap.add_argument("--specs", default="design_class_specs.yaml")
    ap.add_argument("--liberty", type=Path, required=True)
    ap.add_argument(
        "--period-ns",
        type=float,
        default=None,
        help="Override class timing-contract reference period.",
    )
    ap.add_argument("--sta", default="")
    ap.add_argument(
        "--results-root",
        type=Path,
        default=Path("design_class_results"),
    )
    ap.add_argument("--workers", type=int, default=2)
    ap.add_argument("--timeout-sec", type=int, default=60)
    ap.add_argument(
        "--only-config",
        action="append",
        default=[],
        help="Run only selected config name; repeatable.",
    )
    # Backward compatibility with old DF-09 usage.
    ap.add_argument(
        "--only-n",
        type=int,
        action="append",
        default=[],
        help="Deprecated compatibility alias for DF-09 n<N> configs.",
    )
    ap.add_argument("--resume", action="store_true")
    ap.add_argument(
        "--scratch-root",
        type=Path,
        default=Path("/tmp/pycircuit_sta"),
    )
    ap.add_argument("--no-local-stage", action="store_true")
    args = ap.parse_args()

    spec_file = Path(args.specs)
    specs = yaml.safe_load(spec_file.read_text(encoding="utf-8"))
    dc = specs["design_classes"].get(args.class_id)
    if not dc:
        raise SystemExit(f"Unknown design class: {args.class_id}")

    contract = dc.get("timing_contract")
    if not contract:
        raise SystemExit(
            f"{args.class_id} has no timing_contract in {spec_file}"
        )

    period_ns = (
        args.period_ns
        if args.period_ns is not None
        else float(contract.get("default_period_ns", 100.0))
    )

    liberty = args.liberty.expanduser().resolve()
    if not liberty.exists():
        raise SystemExit(f"Liberty not found: {liberty}")

    sta = args.sta or shutil.which("sta")
    if not sta:
        raise SystemExit("OpenSTA `sta` not found.")

    vp = run([sta, "-version"])
    sta_version = (vp.stdout or vp.stderr or "").strip()

    scratch_root = args.scratch_root.expanduser().resolve()
    scratch_root.mkdir(parents=True, exist_ok=True)
    sta_liberty = (
        liberty
        if args.no_local_stage
        else stage_liberty_to_linux_fs(liberty, scratch_root)
    )

    base = args.results_root / args.class_id / args.profile
    comparison = base / "comparison.csv"
    if not comparison.exists():
        raise SystemExit(
            f"Missing {comparison}. Run mapped synthesis first."
        )

    with comparison.open(newline="", encoding="utf-8") as f:
        src_rows = list(csv.DictReader(f))

    only_configs = set(args.only_config)
    only_configs |= {f"n{n}" for n in args.only_n}

    selected_rows = [
        r for r in src_rows
        if not only_configs or r.get("config") in only_configs
    ]

    print("=== pyCircuit Generic Timing Benchmark v0.14 ===", flush=True)
    print("class    :", args.class_id, dc.get("operation", ""), flush=True)
    print("profile  :", args.profile, flush=True)
    print("configs  :", len({r.get('config') for r in selected_rows}), flush=True)
    print("cases    :", len(selected_rows), flush=True)
    print("sta      :", sta_version, flush=True)
    print("liberty  :", liberty, flush=True)
    print("STA lib  :", sta_liberty, flush=True)
    print("scratch  :", scratch_root, flush=True)
    print("stage    :", "OFF" if args.no_local_stage else "ON", flush=True)
    print("period   :", period_ns, "ns", flush=True)
    print("workers  :", args.workers, flush=True)
    print("timeout  :", args.timeout_sec, "sec/case", flush=True)
    if only_configs:
        print("only cfg :", sorted(only_configs), flush=True)
    print()

    rows = []
    failures = 0

    def run_one_case(s):
        case = base / s["project"] / s["module"] / s["config"]
        netlist = case / "mapped_netlist.v"
        cache = case / "timing_case_report.json"

        area = None
        try:
            if s.get("area") not in ("", None, "None"):
                area = float(s["area"])
        except Exception:
            area = None

        record_id = (
            f"{args.class_id}:{s['project']}:{s['module']}:{s['config']}"
        )

        if args.resume and cache.exists():
            try:
                cached = json.loads(cache.read_text(encoding="utf-8"))
                # Do not reuse an old class-specific timing contract.
                if (
                    cached.get("timing_schema") == "0.12"
                    and cached.get("class_id") == args.class_id
                    and cached.get("config") == s["config"]
                ):
                    cached["cache_hit"] = True
                    return cached
            except Exception:
                pass

        result = {
            "record_id": record_id,
            "class_id": args.class_id,
            "config": s["config"],
            "n": s.get("n", ""),
            "data_width": s.get("data_width", ""),
            "capacity": s.get("capacity", ""),
            "project": s["project"],
            "module": s["module"],
            "mapped_area": area,
            "timing_status": "NOT_RUN",
            "worst_slack_ns": None,
            "critical_delay_ns": None,
            "fmax_proxy_mhz": None,
            "startpoint": "",
            "endpoint": "",
            "path_group": "",
            "seconds": None,
            "cache_hit": False,
            "timing_schema": "0.12",
        }

        if not netlist.exists():
            result["timing_status"] = "MAPPED_NETLIST_MISSING"
            cache.parent.mkdir(parents=True, exist_ok=True)
            cache.write_text(
                json.dumps(result, indent=2) + "\n",
                encoding="utf-8",
            )
            return result

        formal_lines = residual_formal_constructs(netlist)
        if formal_lines:
            result["timing_status"] = "NETLIST_FORMAL_RESIDUE"
            result["formal_residue_lines"] = formal_lines[:50]
            cache.write_text(
                json.dumps(result, indent=2) + "\n",
                encoding="utf-8",
            )
            return result

        if args.no_local_stage:
            run_netlist = netlist.resolve()
            run_dir = case
        else:
            run_netlist = stage_case_netlist(
                netlist.resolve(),
                scratch_root,
                args.class_id,
                s["project"],
                s["module"],
                s["config"],
            )
            run_dir = run_netlist.parent

        run_tcl = run_dir / "timing.tcl"
        write_sta_script(
            run_tcl,
            sta_liberty,
            run_netlist,
            period_ns,
            contract,
        )

        (case / "timing.tcl").write_text(
            run_tcl.read_text(encoding="utf-8"),
            encoding="utf-8",
        )

        t0 = time.time()
        p = run([sta, str(run_tcl)], cwd=run_dir, timeout=args.timeout_sec)
        elapsed = time.time() - t0

        stdout = p.stdout or ""
        stderr = p.stderr or ""
        (case / "sta_stdout.log").write_text(stdout, encoding="utf-8")
        (case / "sta_stderr.log").write_text(stderr, encoding="utf-8")

        combined = stdout + "\n" + stderr
        errors = opensta_errors(combined)

        if p.returncode == 124:
            parsed = parse_timing_report(combined, period_ns)
            parsed["status"] = "TIMEOUT"
        elif errors:
            parsed = {
                "status": "NETLIST_PARSE_FAIL",
                "worst_slack_ns": None,
                "critical_delay_ns": None,
                "fmax_proxy_mhz": None,
                "startpoint": "",
                "endpoint": "",
                "path_group": "",
            }
        else:
            parsed = parse_timing_report(combined, period_ns)
            if p.returncode != 0 and parsed["status"] != "PASS":
                parsed["status"] = "FAIL"

        result.update(
            {
                "timing_status": parsed["status"],
                "worst_slack_ns": parsed["worst_slack_ns"],
                "critical_delay_ns": parsed["critical_delay_ns"],
                "fmax_proxy_mhz": parsed["fmax_proxy_mhz"],
                "startpoint": parsed["startpoint"],
                "endpoint": parsed["endpoint"],
                "path_group": parsed["path_group"],
                "seconds": round(elapsed, 3),
                "opensta_errors": errors,
                "staged": not args.no_local_stage,
                "sta_liberty": str(sta_liberty),
                "sta_netlist": str(run_netlist),
            }
        )

        cache.write_text(
            json.dumps(result, indent=2, ensure_ascii=False) + "\n",
            encoding="utf-8",
        )
        return result

    workers = max(1, min(args.workers, len(selected_rows) or 1))
    with ThreadPoolExecutor(max_workers=workers) as ex:
        fmap = {ex.submit(run_one_case, s): s for s in selected_rows}
        completed = 0
        for fut in as_completed(fmap):
            s = fmap[fut]
            completed += 1
            try:
                result = fut.result()
            except Exception as e:
                result = {
                    "record_id": (
                        f"{args.class_id}:{s['project']}:{s['module']}:{s['config']}"
                    ),
                    "class_id": args.class_id,
                    "config": s["config"],
                    "n": s.get("n", ""),
                    "data_width": s.get("data_width", ""),
                    "capacity": s.get("capacity", ""),
                    "project": s["project"],
                    "module": s["module"],
                    "mapped_area": None,
                    "timing_status": "RUNNER_EXCEPTION",
                    "worst_slack_ns": None,
                    "critical_delay_ns": None,
                    "fmax_proxy_mhz": None,
                    "startpoint": "",
                    "endpoint": "",
                    "path_group": "",
                    "seconds": None,
                    "cache_hit": False,
                    "exception": repr(e),
                    "timing_schema": "0.12",
                }

            if result["timing_status"] != "PASS":
                failures += 1
            rows.append(result)

            print(
                f"[{completed}/{len(selected_rows)}] "
                f"{result['project']}/{result['module']} "
                f"{result['config']} "
                f"| {result['timing_status']} "
                f"| area {result.get('mapped_area')} "
                f"| delay {result.get('critical_delay_ns')} ns "
                f"| fmax~ {result.get('fmax_proxy_mhz')} MHz "
                f"| {result.get('seconds')} s"
                + (" | CACHE" if result.get("cache_hit") else ""),
                flush=True,
            )

    pareto_area_delay(rows)
    selection = best_by_config(rows)

    outdir = base / "timing_analysis"
    outdir.mkdir(parents=True, exist_ok=True)

    fields = [
        "record_id",
        "class_id",
        "config",
        "n",
        "data_width",
        "capacity",
        "project",
        "module",
        "mapped_area",
        "timing_status",
        "worst_slack_ns",
        "critical_delay_ns",
        "fmax_proxy_mhz",
        "area_timing_pareto",
        "path_group",
        "startpoint",
        "endpoint",
        "seconds",
        "cache_hit",
        "opensta_errors",
        "staged",
        "sta_liberty",
        "sta_netlist",
    ]
    with (outdir / "timing_summary.csv").open(
        "w", newline="", encoding="utf-8"
    ) as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows({k: r.get(k, "") for k in fields} for r in rows)

    report = {
        "schema_version": "0.14",
        "class_id": args.class_id,
        "operation": dc.get("operation", ""),
        "profile": args.profile,
        "liberty": str(liberty),
        "sta_version": sta_version,
        "reference_period_ns": period_ns,
        "timing_contract": contract,
        "runtime": {
            "workers": args.workers,
            "timeout_sec": args.timeout_sec,
            "only_configs": sorted(only_configs),
            "resume": args.resume,
            "scratch_root": str(scratch_root),
            "local_stage": not args.no_local_stage,
            "staged_liberty": str(sta_liberty),
        },
        "timing_semantics": {
            "critical_delay_ns": (
                "reference_period - worst_slack under the class timing "
                "contract; mapped-cell, no-parasitic pre-layout proxy"
            ),
            "fmax_proxy_mhz": "1000 / critical_delay_ns; not signoff Fmax",
            "pareto": (
                "mapped area × critical delay, evaluated independently "
                "for each canonical config"
            ),
        },
        "selection": selection,
        "rows": rows,
    }

    (outdir / "timing_report.json").write_text(
        json.dumps(report, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    make_html(
        rows,
        outdir / "area_timing.html",
        args.class_id,
        dc.get("operation", ""),
        period_ns,
        liberty,
        sta_version,
        contract.get("semantics", ""),
    )

    print()
    print("=== Selection by Config ===")
    for cfg, s in selection.items():
        print(
            f"{cfg}: area={s['area_winner']} "
            f"| timing={s['timing_winner']} "
            f"| pareto={','.join(s['pareto_set'])}"
        )

    print()
    print("csv  :", outdir / "timing_summary.csv")
    print("json :", outdir / "timing_report.json")
    print("html :", outdir / "area_timing.html")

    raise SystemExit(1 if failures else 0)


if __name__ == "__main__":
    main()

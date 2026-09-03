#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[3]


def _detect_pyc_compile(root: Path) -> Path:
    env = os.environ.get("PYCC")
    if env:
        p = Path(env)
        if p.is_file() and os.access(p, os.X_OK):
            return p
        raise SystemExit(f"PYCC is set but not executable: {p}")

    toolchain_root = os.environ.get("PYC_TOOLCHAIN_ROOT")
    candidates = [
        (Path(toolchain_root) / "bin" / "pycc") if toolchain_root else None,
        root / ".pycircuit_out" / "toolchain" / "install" / "bin" / "pycc",
        root / "dist" / "pycircuit" / "bin" / "pycc",
        root / "build" / "bin" / "pycc",
        root / "compiler" / "mlir" / "build" / "bin" / "pycc",
        root / "compiler" / "mlir" / "build2" / "bin" / "pycc",
        root / "build-top" / "bin" / "pycc",
    ]
    best: Path | None = None
    best_mtime = -1.0
    for c in candidates:
        if c is None:
            continue
        if c.is_file() and os.access(c, os.X_OK):
            mtime = c.stat().st_mtime
            if mtime > best_mtime:
                best = c
                best_mtime = mtime
    if best is not None:
        return best

    found = shutil.which("pycc")
    if found:
        return Path(found)
    raise SystemExit("missing pycc (set PYCC=... or build it first)")


def _pythonpath(root: Path) -> str:
    parts = [
        str(root / "python" / "pycircuit" / "src"),
        str(root / "integrations" / "linx" / "examples" / "pycircuit"),
        str(root),
    ]
    old = os.environ.get("PYTHONPATH")
    if old:
        parts.append(old)
    return ":".join(parts)


def _run(
    cmd: list[str],
    *,
    cwd: Path,
    env: dict[str, str],
    capture_stdout: bool = False,
) -> tuple[float, str]:
    start = time.perf_counter()
    proc = subprocess.run(
        cmd,
        cwd=str(cwd),
        env=env,
        text=True,
        capture_output=capture_stdout,
        check=True,
    )
    elapsed = time.perf_counter() - start
    out = proc.stdout if capture_stdout else ""
    return elapsed, out


def _run_hygiene(root: Path) -> None:
    cmd = [
        sys.executable,
        str(root / "flows" / "tools" / "check_api_hygiene.py"),
        "python/pycircuit/src/pycircuit",
        "examples/pycircuit",
        "docs",
        "README.md",
    ]
    subprocess.run(cmd, cwd=str(root), check=True)


def _count_lines(path: Path) -> int:
    with path.open("r", encoding="utf-8", errors="ignore") as f:
        return sum(1 for _ in f)


def _parse_cycles(output: str) -> int:
    m = re.search(r"cycles=(\d+)", output)
    if not m:
        return 0
    return int(m.group(1))


def _stats(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def _build_flags(profile: str) -> list[str]:
    if profile == "dev":
        return ["-std=c++17", "-O1"]
    if profile == "release":
        return ["-std=c++17", "-O2", "-DNDEBUG"]
    raise SystemExit(f"unsupported --profile={profile!r} (expected dev|release)")


def _run_linx_case(
    root: Path,
    pyc_compile: Path,
    profile: str,
    logic_depth: int,
    sim_mode: str,
    perf_repeats: int,
    perf_max_cycles: int,
) -> dict[str, Any]:
    out_dir = root / ".pycircuit_out" / "perf" / "linx_cpu"
    out_dir.mkdir(parents=True, exist_ok=True)
    pyc_path = out_dir / "linx_cpu_pyc.pyc"
    cpp_out_dir = out_dir / "cpp"
    manifest_path = cpp_out_dir / "cpp_compile_manifest.json"
    tb_path = out_dir / f"tb_linx_cpu_pyc_cpp_{profile}"
    stats_path = cpp_out_dir / "compile_stats.json"

    env_emit = os.environ.copy()
    env_emit["PYTHONDONTWRITEBYTECODE"] = "1"
    env_emit["PYTHONPATH"] = _pythonpath(root)

    emit_s, _ = _run(
        [
            sys.executable,
            "-m",
            "pycircuit.cli",
            "emit",
            "integrations/linx/examples/pycircuit/linx_cpu_pyc/linx_cpu_pyc.py",
            "--param",
            "mem_bytes=1048576",
            "-o",
            str(pyc_path),
        ],
        cwd=root,
        env=env_emit,
    )

    compile_s, _ = _run(
        [
            str(pyc_compile),
            str(pyc_path),
            "--emit=cpp",
            f"--sim-mode={sim_mode}",
            f"--logic-depth={logic_depth}",
            "--out-dir",
            str(cpp_out_dir),
            "--cpp-split=module",
        ],
        cwd=root,
        env=os.environ.copy(),
    )

    build_cmd = [
        sys.executable,
        str(root / "flows" / "tools" / "build_cpp_manifest.py"),
        "--manifest",
        str(manifest_path),
        "--tb",
        str(
            root
            / "integrations"
            / "linx"
            / "examples"
            / "pycircuit"
            / "linx_cpu_pyc"
            / "tb_linx_cpu_pyc.cpp"
        ),
        "--out",
        str(tb_path),
        "--profile",
        profile,
        "--extra-include",
        str(root / "library"),
    ]

    tb_build_s, _ = _run(
        build_cmd,
        cwd=root,
        env=os.environ.copy(),
    )

    env_run = os.environ.copy()
    env_run.setdefault("PYC_KONATA", "0")
    perf_memh = str(
        root
        / "integrations"
        / "linx"
        / "examples"
        / "pycircuit"
        / "linx_cpu"
        / "programs"
        / "test_csel_fixed.memh"
    )
    sim_s, sim_out = _run(
        [
            str(tb_path),
            "--perf",
            "--perf-repeat",
            str(int(perf_repeats)),
            "--perf-max-cycles",
            str(int(perf_max_cycles)),
            "--perf-memh",
            perf_memh,
        ],
        cwd=root,
        env=env_run,
        capture_stdout=True,
    )
    cycles = _parse_cycles(sim_out)
    end_to_end_s = emit_s + compile_s + tb_build_s + sim_s
    cps = (cycles / sim_s) if sim_s > 0 else 0.0
    split_sources = list(cpp_out_dir.glob("*.cpp"))
    split_headers = list(cpp_out_dir.glob("*.hpp"))
    total_loc = sum(_count_lines(p) for p in [*split_sources, *split_headers])

    return {
        "emit_s": emit_s,
        "compile_s": compile_s,
        "tb_build_s": tb_build_s,
        "sim_s": sim_s,
        "end_to_end_s": end_to_end_s,
        "cycles": cycles,
        "cycles_per_sec": cps,
        "perf_repeats": int(perf_repeats),
        "perf_max_cycles": int(perf_max_cycles),
        "header_loc": total_loc,
        "split_cpp_count": len(split_sources),
        "split_hpp_count": len(split_headers),
        "compile_stats": _stats(stats_path),
    }


def _run_linxcore_case(
    root: Path,
    pyc_compile: Path,
    profile: str,
    logic_depth: int,
    sim_mode: str,
    perf_repeats: int,
    perf_max_cycles: int,
) -> dict[str, Any]:
    out_dir = root / ".pycircuit_out" / "perf" / "linxcore"
    out_dir.mkdir(parents=True, exist_ok=True)
    linxcore = root / "integrations" / "linx" / "designs" / "linxcore"
    bench_build = (
        linxcore / "tools" / "image" / "build_linxisa_benchmarks_memh_compat.sh"
    )
    gen_update = linxcore / "tools" / "generate" / "update_generated_linxcore.sh"
    run_cpp = linxcore / "tools" / "generate" / "run_linxcore_top_cpp.sh"

    def _skipped(reason: str) -> dict[str, Any]:
        return {
            "emit_s": 0.0,
            "compile_s": 0.0,
            "tb_build_s": 0.0,
            "sim_s": 0.0,
            "end_to_end_s": 0.0,
            "cycles": 0,
            "cycles_per_sec": 0.0,
            "perf_repeats": int(perf_repeats),
            "perf_max_cycles": int(perf_max_cycles),
            "header_loc": 0,
            "compile_stats": {},
            "skipped": True,
            "skip_reason": reason,
        }

    for required in (bench_build, gen_update, run_cpp):
        if not required.is_file():
            return _skipped(f"missing script: {required}")

    env_run = os.environ.copy()
    env_run["PYCC"] = str(pyc_compile)
    env_run["PYC_LOGIC_DEPTH"] = str(int(logic_depth))
    env_run.setdefault("PYC_KONATA", "0")
    env_run["PYC_MAX_CYCLES"] = str(int(perf_max_cycles))
    env_run["CORE_ITERATIONS"] = str(int(perf_repeats))
    env_run["DHRY_RUNS"] = str(int(perf_repeats) * 100)

    try:
        emit_s, bench_build_out = _run(
            ["bash", str(bench_build)],
            cwd=root,
            env=env_run,
            capture_stdout=True,
        )
    except subprocess.CalledProcessError as e:
        return _skipped(f"linxcore benchmark build failed (rc={e.returncode})")
    memh_lines = [ln.strip() for ln in bench_build_out.splitlines() if ln.strip()]
    if len(memh_lines) < 2:
        return _skipped("failed to build linxcore benchmark memh")
    perf_memh = memh_lines[1]

    try:
        compile_s, _ = _run(
            ["bash", str(gen_update)],
            cwd=root,
            env=env_run,
        )
    except subprocess.CalledProcessError as e:
        return _skipped(f"linxcore generate step failed (rc={e.returncode})")

    tb_build_s = 0.0
    try:
        sim_s, sim_out = _run(
            ["bash", str(run_cpp), perf_memh],
            cwd=root,
            env=env_run,
            capture_stdout=True,
        )
    except subprocess.CalledProcessError as e:
        return _skipped(f"linxcore simulation failed (rc={e.returncode})")
    cycles = _parse_cycles(sim_out)
    end_to_end_s = emit_s + compile_s + tb_build_s + sim_s
    cps = (cycles / sim_s) if sim_s > 0 else 0.0

    return {
        "emit_s": emit_s,
        "compile_s": compile_s,
        "tb_build_s": tb_build_s,
        "sim_s": sim_s,
        "end_to_end_s": end_to_end_s,
        "cycles": cycles,
        "cycles_per_sec": cps,
        "perf_repeats": int(perf_repeats),
        "perf_max_cycles": int(perf_max_cycles),
        "header_loc": 0,
        "compile_stats": {},
    }


def main() -> int:
    root = _repo_root()
    ap = argparse.ArgumentParser(
        description="Run pyCircuit Linx+LinxCore perf smoke and emit JSON metrics."
    )
    ap.add_argument(
        "--output",
        default=str(root / ".pycircuit_out" / "perf" / "perf_smoke.json"),
        help="Output JSON path",
    )
    ap.add_argument(
        "--profile",
        choices=["dev", "release"],
        default=os.environ.get("PYC_BUILD_PROFILE", "release"),
    )
    ap.add_argument("--logic-depth", type=int, default=32)
    ap.add_argument("--sim-mode", choices=["default", "cpp-only"], default="cpp-only")
    ap.add_argument("--perf-repeats-linx", type=int, default=16)
    ap.add_argument("--perf-repeats-linxcore", type=int, default=16)
    ap.add_argument("--perf-max-cycles", type=int, default=4096)
    args = ap.parse_args()

    _run_hygiene(root)
    pyc_compile = _detect_pyc_compile(root)
    result = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "profile": str(args.profile),
        "sim_mode": str(args.sim_mode),
        "logic_depth": int(args.logic_depth),
        "pyc_compile": str(pyc_compile),
        "cases": {},
    }

    result["cases"]["linx_cpu"] = _run_linx_case(
        root,
        pyc_compile,
        profile=str(args.profile),
        logic_depth=int(args.logic_depth),
        sim_mode=str(args.sim_mode),
        perf_repeats=int(args.perf_repeats_linx),
        perf_max_cycles=int(args.perf_max_cycles),
    )
    result["cases"]["linxcore"] = _run_linxcore_case(
        root,
        pyc_compile,
        profile=str(args.profile),
        logic_depth=int(args.logic_depth),
        sim_mode=str(args.sim_mode),
        perf_repeats=int(args.perf_repeats_linxcore),
        perf_max_cycles=int(args.perf_max_cycles),
    )

    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

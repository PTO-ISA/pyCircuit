#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path
from typing import Any


def _root() -> Path:
    return Path(__file__).resolve().parents[2]


def _load_manifest(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise RuntimeError("manifest must be a JSON object")
    if not isinstance(data.get("sources"), list):
        raise RuntimeError("manifest missing `sources` list")
    return data


def _resolve_dir(base: Path, p: str) -> Path:
    q = Path(p)
    return q if q.is_absolute() else (base / q)


def _need_rebuild(dst: Path, inputs: list[Path]) -> bool:
    if not dst.exists():
        return True
    try:
        t = dst.stat().st_mtime
    except OSError:
        return True
    for src in inputs:
        try:
            if src.stat().st_mtime > t:
                return True
        except OSError:
            return True
    return False


def _run(cmd: list[str]) -> None:
    subprocess.run(cmd, check=True)


def _compile_one(
    cxx: str,
    cflags: list[str],
    include_flags: list[str],
    define_flags: list[str],
    src: Path,
    obj: Path,
) -> None:
    obj.parent.mkdir(parents=True, exist_ok=True)
    cmd = [cxx, *cflags, *include_flags, *define_flags, "-c", str(src), "-o", str(obj)]
    _run(cmd)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Compile/link split C++ artifacts from cpp_compile_manifest.json"
    )
    ap.add_argument(
        "--manifest", required=True, help="Path to cpp_compile_manifest.json"
    )
    ap.add_argument(
        "--tb",
        action="append",
        default=[],
        help="Additional C++ translation unit(s) to compile",
    )
    ap.add_argument("--out", required=True, help="Output executable path")
    ap.add_argument(
        "--profile",
        choices=["dev", "release"],
        default=os.environ.get("PYC_BUILD_PROFILE", "release"),
    )
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 1)))
    ap.add_argument("--cxx", default=os.environ.get("CXX", "clang++"))
    ap.add_argument("--extra-include", action="append", default=[])
    ap.add_argument("--extra-define", action="append", default=[])
    ap.add_argument(
        "--obj-dir", default="", help="Object directory (default: <manifest-dir>/.objs)"
    )
    args = ap.parse_args()

    manifest_path = Path(args.manifest).resolve()
    if not manifest_path.is_file():
        raise SystemExit(f"missing manifest: {manifest_path}")

    manifest = _load_manifest(manifest_path)
    manifest_dir = manifest_path.parent

    std = str(manifest.get("cxx_standard", "c++17"))
    cflags = [f"-std={std}"]
    if args.profile == "dev":
        cflags += ["-O1"]
    else:
        cflags += ["-O2", "-DNDEBUG"]

    include_dirs: list[Path] = []
    for p in manifest.get("include_dirs", []):
        if not isinstance(p, str):
            continue
        include_dirs.append(_resolve_dir(manifest_dir, p))
    runtime = manifest.get("runtime", {})
    if not isinstance(runtime, dict):
        runtime = {}
    for p in runtime.get("include_dirs", []):
        if not isinstance(p, str):
            continue
        include_dirs.append(_resolve_dir(manifest_dir, p))
    for p in manifest.get("runtime_include_dirs", []):
        if not isinstance(p, str):
            continue
        include_dirs.append(_resolve_dir(manifest_dir, p))
    for p in args.extra_include:
        include_dirs.append(Path(p).resolve())

    include_flags: list[str] = []
    seen_inc: set[str] = set()
    for p in include_dirs:
        sp = str(p)
        if sp in seen_inc:
            continue
        seen_inc.add(sp)
        include_flags += ["-I", sp]

    define_flags: list[str] = []
    for d in manifest.get("compile_defines", []):
        if isinstance(d, str) and d:
            define_flags.append(f"-D{d}")
    for d in args.extra_define:
        if d:
            define_flags.append(f"-D{d}")

    srcs: list[Path] = []
    for ent in manifest.get("sources", []):
        if not isinstance(ent, dict):
            continue
        p = ent.get("path")
        if not isinstance(p, str) or not p:
            continue
        srcs.append(_resolve_dir(manifest_dir, p))
    for p in manifest.get("runtime_sources", []):
        if isinstance(p, str) and p:
            srcs.append(_resolve_dir(manifest_dir, p))

    tbs = [Path(p).resolve() for p in args.tb]
    srcs.extend(tbs)

    if not srcs:
        raise SystemExit("manifest has no source files")

    for s in srcs:
        if not s.is_file():
            raise SystemExit(f"missing source file: {s}")

    out_exe = Path(args.out).resolve()
    obj_dir = Path(args.obj_dir).resolve() if args.obj_dir else (manifest_dir / ".objs")
    obj_dir.mkdir(parents=True, exist_ok=True)

    objects: list[Path] = []
    compile_jobs: list[tuple[Path, Path]] = []
    repo = _root()
    for src in srcs:
        try:
            rel = src.relative_to(repo)
        except ValueError:
            rel = src.name
        obj_name = (
            str(rel).replace(os.sep, "__").replace("/", "__").replace(":", "_") + ".o"
        )
        obj = obj_dir / obj_name
        objects.append(obj)
        if _need_rebuild(obj, [src]):
            compile_jobs.append((src, obj))

    if compile_jobs:
        failures = 0
        with ThreadPoolExecutor(max_workers=max(1, int(args.jobs))) as pool:
            futs = {
                pool.submit(
                    _compile_one,
                    args.cxx,
                    cflags,
                    include_flags,
                    define_flags,
                    src,
                    obj,
                ): (src, obj)
                for src, obj in compile_jobs
            }
            for fut in as_completed(futs):
                src, _obj = futs[fut]
                try:
                    fut.result()
                except Exception as e:  # noqa: BLE001
                    failures += 1
                    print(
                        f"[build_cpp_manifest] compile failed: {src}: {e}",
                        file=sys.stderr,
                    )
        if failures:
            return 1

    inputs_for_link = list(objects)
    if _need_rebuild(out_exe, inputs_for_link):
        out_exe.parent.mkdir(parents=True, exist_ok=True)
        link_cmd = [args.cxx, *map(str, objects)]
        runtime_library_files: list[str] = []
        for p in runtime.get("library_files", []):
            if isinstance(p, str) and p:
                runtime_library_files.append(str(_resolve_dir(manifest_dir, p)))
        runtime_lib_dirs: list[str] = []
        for p in runtime.get("lib_dirs", []):
            if isinstance(p, str) and p:
                runtime_lib_dirs.append(str(_resolve_dir(manifest_dir, p)))
        runtime_libs: list[str] = []
        for lib in runtime.get("libs", []):
            if isinstance(lib, str) and lib:
                runtime_libs.append(lib)

        if runtime_library_files:
            link_cmd.extend(runtime_library_files)
        else:
            for lib_dir in runtime_lib_dirs:
                link_cmd.extend(["-L", lib_dir])
            for lib in runtime_libs:
                link_cmd.append(lib if lib.startswith("-l") else f"-l{lib}")

        link_cmd.extend(["-o", str(out_exe)])
        _run(link_cmd)

    print(str(out_exe))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

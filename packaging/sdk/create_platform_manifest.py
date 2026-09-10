#!/usr/bin/env python3
"""Create a deterministic platform SDK archive and its external manifest."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
import re
import shutil
import stat
import subprocess
import tarfile
import tempfile
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]


def digest(path: Path) -> str:
    return "sha256:" + hashlib.sha256(path.read_bytes()).hexdigest()


def kind(path: str) -> str:
    if path.startswith("bin/"):
        return "tool"
    if path.startswith("include/"):
        return "header"
    if path.endswith((".a", ".so", ".dylib")):
        return "library"
    if "/cmake/" in path or path.endswith(".cmake"):
        return "cmake"
    if path.endswith(".schema.json"):
        return "schema"
    if path.endswith(".whl"):
        return "wheel"
    if "/licenses/" in path:
        return "license"
    return "metadata"


def platform_record(identity: str, compiler: str) -> dict[str, Any]:
    version = subprocess.run(
        [compiler, "--version"], text=True, capture_output=True, check=False
    )
    if version.returncode:
        raise ValueError(f"unable to probe C++ compiler {compiler!r}")
    compiler_version = version.stdout.splitlines()[0].strip()
    if identity == "linux-x86_64":
        return {
            "id": identity,
            "os": "linux",
            "architecture": "x86_64",
            "runner": "ubuntu-24.04",
            "host_triple": "x86_64-linux-gnu",
            "minimum_os": "Ubuntu 24.04",
            "minimum_libc": "glibc 2.39",
            "python": "3.11",
            "cxx_standard": "20",
            "cxx_abi": "libstdc++ CXX11 ABI",
            "cxx_compiler": compiler_version,
        }
    if identity == "macos-arm64":
        return {
            "id": identity,
            "os": "macos",
            "architecture": "arm64",
            "runner": "macos-15",
            "host_triple": "arm64-apple-darwin",
            "minimum_os": "macOS 15",
            "minimum_libc": None,
            "python": "3.11",
            "cxx_standard": "20",
            "cxx_abi": "Apple libc++",
            "cxx_compiler": compiler_version,
        }
    raise ValueError(f"unsupported SDK platform: {identity}")


def write_json(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(value, indent=2, sort_keys=False) + "\n", encoding="utf-8"
    )


def deterministic_tar(source: Path, destination: Path) -> None:
    with destination.open("wb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0) as compressed:
            with tarfile.open(fileobj=compressed, mode="w") as archive:
                for path in sorted(source.rglob("*")):
                    relative = path.relative_to(source).as_posix()
                    info = archive.gettarinfo(str(path), arcname=relative)
                    info.uid = info.gid = 0
                    info.uname = info.gname = ""
                    info.mtime = 0
                    if info.isfile():
                        info.mode = (
                            0o755 if path.stat().st_mode & stat.S_IXUSR else 0o644
                        )
                        with path.open("rb") as stream:
                            archive.addfile(info, stream)
                    else:
                        info.mode = 0o755
                        archive.addfile(info)


def native_files(stage: Path) -> list[Path]:
    result: list[Path] = []
    for path in sorted(item for item in stage.rglob("*") if item.is_file()):
        if path.suffix == ".a":
            continue
        identified = subprocess.run(
            ["file", "-b", path], text=True, capture_output=True, check=False
        )
        if identified.returncode == 0 and (
            "Mach-O" in identified.stdout or "ELF" in identified.stdout
        ):
            result.append(path)
    return result


def relocate_native_dependencies(stage: Path, identity: str) -> None:
    binaries = native_files(stage)
    bundled: dict[str, Path] = {}
    for path in binaries:
        current = bundled.get(path.name)
        if current is None or len(path.parts) < len(current.parts):
            bundled[path.name] = path
    if identity == "macos-arm64":
        for path in binaries:
            linked = subprocess.run(
                ["otool", "-L", path], text=True, capture_output=True, check=False
            )
            if linked.returncode:
                raise ValueError(f"otool failed for {path}: {linked.stderr}")
            for raw in linked.stdout.splitlines()[1:]:
                dependency = raw.strip().split(" ", 1)[0]
                if dependency.startswith(("/usr/lib/", "/System/Library/")):
                    continue
                target = bundled.get(Path(dependency).name)
                if target is None:
                    raise ValueError(
                        f"non-system dependency is not bundled: {dependency}"
                    )
                relative = os.path.relpath(target, path.parent)
                replacement = "@loader_path/" + relative
                changed = subprocess.run(
                    ["install_name_tool", "-change", dependency, replacement, path],
                    text=True,
                    capture_output=True,
                    check=False,
                )
                if changed.returncode:
                    raise ValueError(
                        f"install_name_tool failed for {path}: {changed.stderr}"
                    )
            if path.suffix == ".dylib":
                subprocess.run(
                    ["install_name_tool", "-id", "@rpath/" + path.name, path],
                    text=True,
                    capture_output=True,
                    check=True,
                )
        return
    patchelf = shutil.which("patchelf")
    if patchelf is None and binaries:
        raise ValueError("patchelf is required to relocate Linux SDK binaries")
    library_root = stage / "lib"
    for path in binaries:
        relative = os.path.relpath(library_root, path.parent)
        rpath = "$ORIGIN" if relative == "." else "$ORIGIN/" + relative
        subprocess.run([patchelf, "--set-rpath", rpath, path], check=True)
        needed = subprocess.run(
            [patchelf, "--print-needed", path],
            text=True,
            capture_output=True,
            check=True,
        ).stdout.splitlines()
        for dependency in needed:
            if dependency.startswith("/") and Path(dependency).name in bundled:
                subprocess.run(
                    [
                        patchelf,
                        "--replace-needed",
                        dependency,
                        Path(dependency).name,
                        path,
                    ],
                    check=True,
                )


def runtime_dependencies(stage: Path, identity: str) -> list[dict[str, Any]]:
    observed: dict[tuple[str, str], dict[str, Any]] = {}
    binaries = native_files(stage)
    if not binaries:
        name = "glibc" if identity == "linux-x86_64" else "libc++"
        return [
            {
                "name": name,
                "kind": "system",
                "version": "system ABI",
                "sha256": None,
            }
        ]
    if identity == "linux-x86_64":
        release_lines = subprocess.run(
            ["ldd", "--version"], text=True, capture_output=True, check=False
        ).stdout.splitlines()
        release = release_lines[0] if release_lines else "system ABI"
        for path in binaries:
            linked = subprocess.run(
                ["ldd", path], text=True, capture_output=True, check=False
            )
            if linked.returncode:
                raise ValueError(f"ldd failed for {path}: {linked.stderr}")
            for raw in linked.stdout.splitlines():
                fields = raw.replace("=>", " ").split()
                if "not found" in raw:
                    raise ValueError(f"unresolved dependency: {raw.strip()}")
                if not fields:
                    continue
                name = Path(fields[0]).name
                resolved = next(
                    (value for value in fields if value.startswith("/")), None
                )
                if resolved and resolved.startswith(os.fspath(stage.resolve())):
                    record = {
                        "name": name,
                        "kind": "bundled",
                        "version": "bundled",
                        "sha256": digest(Path(resolved)),
                    }
                else:
                    record = {
                        "name": name,
                        "kind": "system",
                        "version": release,
                        "sha256": None,
                    }
                observed[(record["kind"], name)] = record
    else:
        release = subprocess.run(
            ["sw_vers", "-productVersion"],
            text=True,
            capture_output=True,
            check=False,
        ).stdout.strip()
        for path in binaries:
            linked = subprocess.run(
                ["otool", "-L", path], text=True, capture_output=True, check=False
            )
            if linked.returncode:
                raise ValueError(f"otool failed for {path}: {linked.stderr}")
            for raw in linked.stdout.splitlines()[1:]:
                dependency = raw.strip().split(" ", 1)[0]
                name = Path(dependency).name
                system = dependency.startswith(("/usr/lib/", "/System/Library/"))
                record: dict[str, Any] = {
                    "name": name,
                    "kind": "system" if system else "bundled",
                    "version": release or "system ABI",
                    "sha256": None,
                }
                if not system:
                    matches = list(stage.rglob(name))
                    if len(matches) != 1:
                        raise ValueError(
                            "non-system dependency is not bundled exactly once: "
                            + dependency
                        )
                    record["version"] = "bundled"
                    record["sha256"] = digest(matches[0])
                observed[(record["kind"], name)] = record
    return [observed[key] for key in sorted(observed)]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--install-dir", type=Path, required=True)
    parser.add_argument("--wheel", type=Path, action="append", required=True)
    parser.add_argument("--platform", required=True)
    parser.add_argument("--source-revision", required=True)
    parser.add_argument("--out-dir", type=Path, required=True)
    parser.add_argument("--cxx-compiler", default=os.environ.get("CXX", "c++"))
    args = parser.parse_args()
    version_map = json.loads((ROOT / "packaging/sdk/version-map.json").read_text())
    product = version_map["product_version"]
    agentic = version_map["distributions"]["agentic-circuit"]
    wheel_patterns = {
        "pycircuit-hisi": (
            rf"^pycircuit_hisi-{re.escape(product)}-py3-none-linux_x86_64\.whl$"
            if args.platform == "linux-x86_64"
            else rf"^pycircuit_hisi-{re.escape(product)}-py3-none-macosx_[0-9]+_[0-9]+_arm64\.whl$"
        ),
        "pycircuit-semantic-core": rf"^pycircuit_semantic_core-{re.escape(product)}-py3-none-any\.whl$",
        "agentic-circuit": rf"^agentic_circuit-{re.escape(agentic)}-py3-none-any\.whl$",
    }
    observed = {wheel.name: wheel for wheel in args.wheel}
    if len(observed) != len(args.wheel):
        raise ValueError("wheel arguments must have unique filenames")
    for identity, pattern in wheel_patterns.items():
        matches = [name for name in observed if re.fullmatch(pattern, name)]
        if len(matches) != 1:
            raise ValueError(
                f"SDK requires exactly one {identity} wheel; observed {matches}"
            )
    if len(observed) != 3:
        raise ValueError(
            f"SDK requires exactly three wheels; observed {sorted(observed)}"
        )
    args.out_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="pycircuit-sdk-") as raw:
        stage = Path(raw) / "sdk"
        shutil.copytree(
            args.install_dir,
            stage,
            ignore=shutil.ignore_patterns("__pycache__", "*.pyc", "*.pyo"),
        )
        wheelhouse = stage / "python/wheelhouse"
        wheelhouse.mkdir(parents=True, exist_ok=True)
        for name, source in sorted(observed.items()):
            shutil.copyfile(source, wheelhouse / name)
        license_path = stage / "share/pycircuit/licenses/LICENSE"
        license_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / "LICENSE", license_path)
        forbidden = {
            os.fspath(args.install_dir.resolve()).encode(),
            os.fspath(ROOT.resolve()).encode(),
        }
        for path in sorted(item for item in stage.rglob("*") if item.is_file()):
            data = path.read_bytes()
            if any(value in data for value in forbidden):
                raise ValueError(
                    "installed SDK contains a producer absolute path: "
                    + path.relative_to(stage).as_posix()
                )
        relocate_native_dependencies(stage, args.platform)
        self_path = "share/pycircuit/sdk-manifest.json"
        files = []
        for path in sorted(item for item in stage.rglob("*") if item.is_file()):
            relative = path.relative_to(stage).as_posix()
            files.append(
                {
                    "path": relative,
                    "kind": kind(relative),
                    "sha256": digest(path),
                    "size": path.stat().st_size,
                }
            )
        manifest = {
            "schema": "pycircuit-sdk-platform-manifest",
            "version": "1",
            "contract_epoch": version_map["contract_epoch"],
            "product_version": product,
            "source_revision": args.source_revision,
            "platform": platform_record(args.platform, args.cxx_compiler),
            "distributions": version_map["distributions"],
            "abi": version_map["contracts"],
            "capabilities": [
                "cycle-aware-signal",
                "pyc-cpp",
                "pyc-verilog",
                "agentic-model-plan",
                "agentic-model-emit-cpp",
                "gfsim-runtime-v1",
            ],
            "runtime_dependencies": runtime_dependencies(stage, args.platform),
            "self_path": self_path,
            "files": files,
        }
        write_json(stage / self_path, manifest)
        write_json(
            args.out_dir / f"pycircuit-sdk-{product}-{args.platform}.manifest.json",
            manifest,
        )
        deterministic_tar(
            stage, args.out_dir / f"pycircuit-sdk-{product}-{args.platform}.tar.gz"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

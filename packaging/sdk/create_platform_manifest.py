#!/usr/bin/env python3
"""Create a deterministic platform SDK archive and its external manifest."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
import platform
import re
import shutil
import stat
import subprocess
import tarfile
import tempfile
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
SDK_SCHEMA_NAMES = (
    "consumer-lock.schema.json",
    "emitted-cost.schema.json",
    "model-manifest.schema.json",
    "model-plan.schema.json",
    "release-index.schema.json",
    "sdk-manifest.schema.json",
    "sdk-version-map.schema.json",
)

# Windows dependencies whose DLL name ends in one of these prefixes are shipped
# with the operating system (API sets) and are never bundled in the SDK.
WINDOWS_SYSTEM_DLL_PREFIXES = ("api-ms-win-", "ext-ms-win-")
# Windows operating-system DLLs plus the MSVC v143 C/C++ runtime, which the
# platform profile declares as the system C++ ABI (the Windows analogue of
# glibc/libstdc++ on Linux and libc++ on macOS).  Any other DLL import must be
# present inside the SDK or relocation fails closed.
WINDOWS_SYSTEM_DLLS = frozenset(
    {
        "advapi32.dll",
        "bcrypt.dll",
        "bcryptprimitives.dll",
        "cfgmgr32.dll",
        "combase.dll",
        "comctl32.dll",
        "comdlg32.dll",
        "concrt140.dll",
        "crypt32.dll",
        "cryptbase.dll",
        "dbghelp.dll",
        "dnsapi.dll",
        "dsound.dll",
        "dwmapi.dll",
        "gdi32.dll",
        "gdi32full.dll",
        "imm32.dll",
        "iphlpapi.dll",
        "kernel32.dll",
        "kernelbase.dll",
        "mf.dll",
        "mfplat.dll",
        "mpr.dll",
        "msvcp140.dll",
        "msvcp140_1.dll",
        "msvcp140_2.dll",
        "msvcp140_atomic_wait.dll",
        "msvcp140_codecvt_ids.dll",
        "msvcp_win.dll",
        "msvcrt.dll",
        "netapi32.dll",
        "normaliz.dll",
        "ntdll.dll",
        "ole32.dll",
        "oleaut32.dll",
        "opengl32.dll",
        "powrprof.dll",
        "psapi.dll",
        "rpcrt4.dll",
        "sechost.dll",
        "secur32.dll",
        "setupapi.dll",
        "shell32.dll",
        "shlwapi.dll",
        "ucrtbase.dll",
        "user32.dll",
        "userenv.dll",
        "vcruntime140.dll",
        "vcruntime140_1.dll",
        "version.dll",
        "winmm.dll",
        "ws2_32.dll",
        "wtsapi32.dll",
    }
)


def digest(path: Path) -> str:
    return "sha256:" + hashlib.sha256(path.read_bytes()).hexdigest()


def kind(path: str) -> str:
    if path.startswith("bin/") or path.endswith(".exe"):
        return "tool"
    if path.startswith("include/"):
        return "header"
    if path.endswith((".a", ".so", ".dylib", ".dll", ".lib", ".pyd")):
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


def _windows_compiler_version(compiler: str) -> str:
    """Probe the Windows compiler version.

    clang-cl, the driver this profile builds with, implements ``--version``.
    cl.exe does not and only prints a banner when invoked without input, so
    fall back to that banner's first line.
    """
    version = subprocess.run(
        [compiler, "--version"], text=True, capture_output=True, check=False
    )
    if version.returncode == 0 and version.stdout.strip():
        return version.stdout.splitlines()[0].strip()
    probed = subprocess.run([compiler], text=True, capture_output=True, check=False)
    lines = [
        line.strip()
        for line in (probed.stdout + probed.stderr).splitlines()
        if line.strip()
    ]
    if not lines:
        raise ValueError(f"unable to probe C++ compiler {compiler!r}")
    return lines[0]


def platform_record(identity: str, compiler: str) -> dict[str, Any]:
    if identity == "windows-x86_64":
        compiler_version = _windows_compiler_version(compiler)
    else:
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
    if identity == "windows-x86_64":
        return {
            "id": identity,
            "os": "windows",
            "architecture": "x86_64",
            "runner": "windows-2022",
            "host_triple": "x86_64-pc-windows-msvc",
            "minimum_os": "Windows Server 2022",
            "minimum_libc": None,
            "python": "3.11",
            "cxx_standard": "20",
            "cxx_abi": "MSVC v143",
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


def windows_native_files(stage: Path) -> list[Path]:
    """Enumerate Windows PE images without relying on the Unix ``file`` tool."""
    suffixes = {".exe", ".dll", ".pyd"}
    return sorted(
        path
        for path in stage.rglob("*")
        if path.is_file() and path.suffix.lower() in suffixes
    )


def windows_system_dependency(name: str) -> bool:
    lowered = name.lower()
    if lowered.startswith(WINDOWS_SYSTEM_DLL_PREFIXES):
        return True
    return lowered in WINDOWS_SYSTEM_DLLS


def dumpbin_dependents(dumpbin: str, path: Path) -> list[str]:
    completed = subprocess.run(
        [dumpbin, "/nologo", "/dependents", path],
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode:
        raise ValueError(f"dumpbin failed for {path}: {completed.stderr}")
    dependencies: list[str] = []
    in_block = False
    for raw in completed.stdout.splitlines():
        line = raw.strip()
        if line.lower().startswith("image has the following"):
            in_block = True
            continue
        if in_block and line.lower().endswith(".dll"):
            dependencies.append(Path(line).name)
    return list(dict.fromkeys(dependencies))


def require_dumpbin() -> str:
    dumpbin = shutil.which("dumpbin")
    if dumpbin is None:
        raise ValueError(
            "dumpbin is required to verify Windows SDK binary dependencies; "
            "run from a Visual Studio developer environment (vcvars64) or install "
            "the MSVC toolchain"
        )
    return dumpbin


def relocate_native_dependencies(stage: Path, identity: str) -> None:
    binaries = (
        windows_native_files(stage)
        if identity == "windows-x86_64"
        else native_files(stage)
    )
    bundled: dict[str, Path] = {}
    for path in binaries:
        current = bundled.get(path.name)
        if current is None or len(path.parts) < len(current.parts):
            bundled[path.name] = path
    if identity == "windows-x86_64":
        # Windows PE imports are resolved by name from the executable directory
        # or PATH: there is no RPATH to rewrite and no codesign/patchelf step.
        # Relocation therefore fails closed unless every non-system import is
        # already bundled inside the SDK.
        dumpbin = require_dumpbin()
        bundled_names = {name.lower() for name in bundled}
        for path in binaries:
            for dependency in dumpbin_dependents(dumpbin, path):
                if windows_system_dependency(dependency):
                    continue
                if dependency.lower() not in bundled_names:
                    raise ValueError(
                        f"non-system dependency is not bundled: {dependency}"
                    )
        return
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
        for path in binaries:
            subprocess.run(
                ["codesign", "--force", "--sign", "-", "--timestamp=none", path],
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
    binaries = (
        windows_native_files(stage)
        if identity == "windows-x86_64"
        else native_files(stage)
    )
    if not binaries:
        name = {
            "linux-x86_64": "glibc",
            "windows-x86_64": "MSVC v143 runtime",
        }.get(identity, "libc++")
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
                if raw.strip() == "statically linked":
                    continue
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
    elif identity == "windows-x86_64":
        dumpbin = require_dumpbin()
        release = platform.version() or "system ABI"
        bundled = {path.name.lower(): path for path in binaries}
        for path in binaries:
            for dependency in dumpbin_dependents(dumpbin, path):
                if windows_system_dependency(dependency):
                    record = {
                        "name": dependency,
                        "kind": "system",
                        "version": release,
                        "sha256": None,
                    }
                else:
                    target = bundled.get(dependency.lower())
                    if target is None:
                        raise ValueError(
                            f"non-system dependency is not bundled: {dependency}"
                        )
                    record = {
                        "name": dependency,
                        "kind": "bundled",
                        "version": "bundled",
                        "sha256": digest(target),
                    }
                observed[(record["kind"], dependency)] = record
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
    if args.platform == "linux-x86_64":
        hisi_wheel_pattern = (
            rf"^pycircuit_hisi-{re.escape(product)}-py3-none-linux_x86_64\.whl$"
        )
    elif args.platform == "windows-x86_64":
        hisi_wheel_pattern = (
            rf"^pycircuit_hisi-{re.escape(product)}-py3-none-win_amd64\.whl$"
        )
    else:
        hisi_wheel_pattern = rf"^pycircuit_hisi-{re.escape(product)}-py3-none-macosx_[0-9]+_[0-9]+_arm64\.whl$"
    wheel_patterns = {
        "pycircuit-hisi": hisi_wheel_pattern,
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
        schema_root = stage / "share/pycircuit/schemas"
        schema_root.mkdir(parents=True, exist_ok=True)
        for name in SDK_SCHEMA_NAMES:
            shutil.copyfile(ROOT / "schemas/agentic-circuit" / name, schema_root / name)
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

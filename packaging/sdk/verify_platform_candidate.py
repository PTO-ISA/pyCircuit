#!/usr/bin/env python3
"""Verify a relocated SDK, its exact manifest closure, and external model use."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import venv
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
MODEL_CONSUMER = ROOT / "tests/integration/agentic-circuit/model-install-consumer"

# Windows API-set prefixes and operating-system DLL names (plus the MSVC v143
# C/C++ runtime declared by the platform profile).  Kept in step with
# create_platform_manifest.py so generator and verifier classify imports alike.
WINDOWS_SYSTEM_DLL_PREFIXES = ("api-ms-win-", "ext-ms-win-")
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
        "python3.dll",
        "python311.dll",
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


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return "sha256:" + digest.hexdigest()


def load(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"{path}: expected a JSON object")
    return value


def run(
    command: list[os.PathLike[str] | str],
    *,
    cwd: Path,
    env: dict[str, str] | None = None,
) -> None:
    completed = subprocess.run(
        [os.fspath(item) for item in command],
        cwd=cwd,
        env=env,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode:
        raise ValueError(
            f"command failed ({completed.returncode}): "
            + " ".join(map(os.fspath, command))
            + "\n"
            + completed.stdout
            + completed.stderr
        )


def extract(archive: Path, destination: Path) -> None:
    destination.mkdir(parents=True)
    with tarfile.open(archive, "r:gz") as bundle:
        for member in bundle.getmembers():
            path = Path(member.name)
            if (
                path.is_absolute()
                or ".." in path.parts
                or member.issym()
                or member.islnk()
            ):
                raise ValueError(f"unsafe SDK archive member: {member.name}")
        bundle.extractall(destination, filter="data")


def _windows_system_dependency(name: str) -> bool:
    lowered = name.lower()
    if lowered.startswith(WINDOWS_SYSTEM_DLL_PREFIXES):
        return True
    return lowered in WINDOWS_SYSTEM_DLLS


def _native_files(root: Path, manifest: dict[str, Any]) -> list[Path]:
    result: list[Path] = []
    for record in manifest["files"]:
        path = root / record["path"]
        if record["kind"] not in {"tool", "library"} or path.suffix == ".a":
            continue
        if sys.platform == "win32":
            if path.suffix.lower() in {".exe", ".dll", ".pyd"}:
                result.append(path)
            continue
        identified = subprocess.run(
            ["file", "-b", path], text=True, capture_output=True, check=False
        )
        if identified.returncode == 0 and (
            "Mach-O" in identified.stdout or "ELF" in identified.stdout
        ):
            result.append(path)
    return result


def verify_native_closure(root: Path, manifest: dict[str, Any]) -> None:
    root_text = os.fspath(root.resolve())
    bundled_names = {path.name for path in root.rglob("*") if path.is_file()}
    bundled_names_lower = {name.lower() for name in bundled_names}
    observed_dependencies: set[tuple[str, str]] = set()
    native = _native_files(root, manifest)
    if sys.platform == "win32":
        declared_native = [
            record["path"]
            for record in manifest["files"]
            if record["kind"] in {"tool", "library"}
        ]
        if declared_native and not native:
            raise ValueError(
                "no Windows PE binaries were found among the declared SDK tools "
                "and libraries; refusing to accept an unverified native closure"
            )
    for path in native:
        if sys.platform == "win32":
            dumpbin = shutil.which("dumpbin")
            if dumpbin is None:
                raise ValueError(
                    "dumpbin is required to verify Windows native dependencies; "
                    "run from a Visual Studio developer environment"
                )
            linked = subprocess.run(
                [dumpbin, "/nologo", "/dependents", path],
                text=True,
                capture_output=True,
                check=False,
            )
            if linked.returncode:
                raise ValueError(f"dumpbin failed for {path}: {linked.stderr}")
            in_block = False
            for raw in linked.stdout.splitlines():
                line = raw.strip()
                if line.lower().startswith("image has the following"):
                    in_block = True
                    continue
                if not in_block or not line.lower().endswith(".dll"):
                    continue
                dependency = Path(line).name
                if _windows_system_dependency(dependency):
                    observed_dependencies.add(("system", dependency))
                elif dependency.lower() in bundled_names_lower:
                    observed_dependencies.add(("bundled", dependency))
                else:
                    raise ValueError(f"undeclared non-system dependency: {dependency}")
            continue
        if sys.platform == "darwin":
            linked = subprocess.run(
                ["otool", "-L", path], text=True, capture_output=True, check=False
            )
            if linked.returncode:
                raise ValueError(f"otool failed for {path}: {linked.stderr}")
            for raw in linked.stdout.splitlines()[1:]:
                dependency = raw.strip().split(" ", 1)[0]
                if dependency.startswith(("/usr/lib/", "/System/Library/")):
                    observed_dependencies.add(("system", Path(dependency).name))
                    continue
                if dependency.startswith(
                    ("@rpath/", "@loader_path/", "@executable_path/")
                ):
                    if Path(dependency).name not in bundled_names:
                        raise ValueError(
                            f"undeclared non-system dependency: {dependency}"
                        )
                    observed_dependencies.add(("bundled", Path(dependency).name))
                    continue
                if not dependency.startswith(root_text):
                    raise ValueError(f"non-relocatable dependency: {dependency}")
                observed_dependencies.add(("bundled", Path(dependency).name))
            loads = subprocess.run(
                ["otool", "-l", path], text=True, capture_output=True, check=False
            )
            lines = loads.stdout.splitlines()
            for index, line in enumerate(lines):
                if "LC_RPATH" not in line or index + 2 >= len(lines):
                    continue
                rpath = lines[index + 2].strip().split(" ", 2)[1]
                if rpath.startswith("/") and not rpath.startswith(root_text):
                    raise ValueError(f"non-relocatable LC_RPATH: {rpath}")
        else:
            linked = subprocess.run(
                ["ldd", path], text=True, capture_output=True, check=False
            )
            if linked.returncode:
                raise ValueError(f"ldd failed for {path}: {linked.stderr}")
            for raw in linked.stdout.splitlines():
                if raw.strip() == "statically linked":
                    continue
                if "not found" in raw:
                    raise ValueError(f"unresolved dynamic dependency: {raw.strip()}")
                fields = raw.replace("=>", " ").split()
                if fields:
                    dependency_name = Path(fields[0]).name
                else:
                    continue
                resolved = next((item for item in fields if item.startswith("/")), None)
                if resolved and not resolved.startswith(
                    (root_text, "/lib/", "/lib64/", "/usr/lib/")
                ):
                    raise ValueError(f"non-relocatable dependency: {resolved}")
                observed_dependencies.add(
                    (
                        (
                            "bundled"
                            if resolved and resolved.startswith(root_text)
                            else "system"
                        ),
                        dependency_name,
                    )
                )
            dynamic = subprocess.run(
                ["readelf", "-d", path], text=True, capture_output=True, check=False
            )
            for raw in dynamic.stdout.splitlines():
                if "RPATH" not in raw and "RUNPATH" not in raw:
                    continue
                value = raw.rsplit("[", 1)[-1].split("]", 1)[0]
                for rpath in value.split(":"):
                    if rpath.startswith("/") and not rpath.startswith(root_text):
                        raise ValueError(f"non-relocatable RPATH: {rpath}")
    declared_dependencies = {
        (str(record["kind"]), str(record["name"]))
        for record in manifest["runtime_dependencies"]
    }
    if observed_dependencies and observed_dependencies != declared_dependencies:
        raise ValueError(
            "runtime dependency manifest does not match observed native closure; "
            f"declared={sorted(declared_dependencies)}, "
            f"observed={sorted(observed_dependencies)}"
        )


def validate_tree(
    root: Path, external_manifest: Path, forbidden_paths: tuple[str, ...]
) -> tuple[dict[str, Any], list[Path]]:
    manifest = load(external_manifest)
    self_path = str(manifest["self_path"])
    embedded = root / self_path
    if embedded.read_bytes() != external_manifest.read_bytes():
        raise ValueError("embedded and external SDK manifests differ")
    expected = {str(record["path"]) for record in manifest["files"]}
    if self_path in expected:
        raise ValueError("embedded manifest must not hash itself")
    actual = {
        path.relative_to(root).as_posix() for path in root.rglob("*") if path.is_file()
    }
    if actual != expected | {self_path}:
        missing = sorted(expected - actual)
        unlisted = sorted(actual - expected - {self_path})
        raise ValueError(
            f"SDK manifest file-set mismatch; missing={missing}, unlisted={unlisted}"
        )
    encoded_forbidden = tuple(
        value.encode() for value in forbidden_paths if value and value != "/"
    )
    for record in manifest["files"]:
        path = root / record["path"]
        if path.stat().st_size != record["size"] or sha256(path) != record["sha256"]:
            raise ValueError(f"SDK manifest checksum mismatch: {record['path']}")
        data = path.read_bytes()
        if any(value in data for value in encoded_forbidden):
            raise ValueError(f"SDK contains a producer absolute path: {record['path']}")
    wheels = sorted(
        (root / "python/wheelhouse").glob("*.whl"), key=lambda path: path.name
    )
    if len(wheels) != 3:
        raise ValueError(f"SDK wheelhouse must contain exactly three wheels: {wheels}")
    verify_native_closure(root, manifest)
    return manifest, wheels


def write_model_source(source: Path) -> None:
    package = source / "model"
    package.mkdir(parents=True)
    (package / "__init__.py").write_text("", encoding="utf-8")
    (package / "contracts.py").write_text(
        "import agentic_circuit as ac\n\n"
        "@ac.struct\n"
        "class Entry:\n"
        "    sequence: ac.u4\n"
        "    value: ac.u16\n"
        "    done: bool\n",
        encoding="utf-8",
    )
    (package / "top.py").write_text(
        "import agentic_circuit as ac\n"
        "from .contracts import Entry\n\n"
        "@ac.rule\n"
        "def complete(entry):\n"
        "    return entry.with_fields(done=True)\n\n"
        "@ac.system\n"
        "def pipeline(entries: ac.const[int]) -> None:\n"
        "    source = ac.source(Entry, depth=4, latency=1)\n"
        "    completed = complete(source)\n"
        "    ordered = ac.reorder(completed, by=Entry.sequence, "
        "entries=entries, start=0)\n"
        "    ac.sink(ordered)\n",
        encoding="utf-8",
    )
    (source / "model.toml").write_text(
        'version = "1"\n\n[static]\nentries = 8\n', encoding="utf-8"
    )


def find_build_artifact(root: Path, names: tuple[str, ...]) -> Path:
    """Locate exactly one built artifact under ``root``.

    A single-configuration generator writes to the build root, while a
    multi-configuration one (Visual Studio, Xcode) writes to a configuration
    subdirectory such as ``Release/``. Searching recursively covers both, and
    Debug symbol bundles are skipped because they contain a copy of the binary
    under ``*.dSYM/Contents/Resources/DWARF/``.
    """
    found = sorted(
        (
            path
            for name in names
            for path in root.rglob(name)
            if path.is_file() and not any(part.endswith(".dSYM") for part in path.parts)
        ),
        key=lambda path: path.relative_to(root).as_posix(),
    )
    if len(found) != 1:
        raise ValueError(f"expected one {names[0]}, found {found}")
    return found[0]


def verify_only_export(plugin: Path) -> None:
    if sys.platform == "win32":
        dumpbin = shutil.which("dumpbin")
        if dumpbin is None:
            raise ValueError(
                "dumpbin is required to inspect generated Windows model exports"
            )
        completed = subprocess.run(
            [dumpbin, "/nologo", "/exports", plugin],
            text=True,
            capture_output=True,
            check=False,
        )
        if completed.returncode:
            raise ValueError(f"dumpbin failed for {plugin}: {completed.stderr}")
        exported: list[str] = []
        in_table = False
        for raw in completed.stdout.splitlines():
            line = raw.strip()
            if not line:
                continue
            fields = line.split()
            if fields[0].lower() == "ordinal" and "name" in line.lower():
                in_table = True
                continue
            if not in_table:
                continue
            if fields[0].lower() == "summary":
                break
            if fields[0].isdigit():
                exported.append(fields[-1])
        if len(exported) != 1 or not exported[0].endswith("agentic_model_query_v1"):
            raise ValueError("generated model must export only agentic_model_query_v1")
        return
    command = (
        ["nm", "-gU", plugin]
        if platform.system() == "Darwin"
        else ["nm", "-D", "--defined-only", plugin]
    )
    completed = subprocess.run(command, text=True, capture_output=True, check=False)
    defined = [line for line in completed.stdout.splitlines() if line.strip()]
    if (
        completed.returncode
        or len(defined) != 1
        or not defined[0].endswith("agentic_model_query_v1")
    ):
        raise ValueError("generated model must export only agentic_model_query_v1")


def tree_hashes(root: Path) -> tuple[tuple[str, str], ...]:
    return tuple(
        (path.relative_to(root).as_posix(), sha256(path))
        for path in sorted(
            (item for item in root.rglob("*") if item.is_file()),
            key=lambda path: path.relative_to(root).as_posix(),
        )
    )


def installed_smoke(sdk_root: Path, wheels: list[Path], workspace: Path) -> None:
    windows = sys.platform == "win32"
    suffix = ".exe" if windows else ""
    environment = workspace / "venv"
    venv.EnvBuilder(with_pip=True).create(environment)
    python = environment / (
        "Scripts/python.exe" if sys.platform == "win32" else "bin/python"
    )
    commands = environment / ("Scripts" if sys.platform == "win32" else "bin")
    run(
        [python, "-m", "pip", "install", "--no-index", "--no-deps", *wheels],
        cwd=workspace,
    )
    run(
        [python, "-c", "import _pycircuit_semantics, agentic_circuit, pycircuit"],
        cwd=workspace,
    )
    run([commands / f"pycircuit{suffix}", "--help"], cwd=workspace)
    # The Agentic Circuit CLI is a relocatable Python launcher script rather
    # than a native tool, so it keeps the extensionless name the install layout
    # documents on every platform; only the compiled tools gain the .exe
    # suffix. Windows cannot execute a file without a known extension, so the
    # launcher is invoked through the interpreter there.
    cli = sdk_root / "bin/agentic-circuit"
    launcher = [os.fspath(python), os.fspath(cli)] if windows else [os.fspath(cli)]
    for required in (
        cli,
        sdk_root / f"bin/acir-opt{suffix}",
        sdk_root / "lib/cmake/AgenticCircuit/AgenticCircuitConfig.cmake",
    ):
        if not required.is_file():
            raise ValueError(f"relocated SDK is missing required file: {required}")

    source = workspace / "source"
    plan = workspace / "plan"
    generated = workspace / "generated"
    build = workspace / "consumer-build"
    write_model_source(source)
    clean_environment = os.environ.copy()
    clean_environment.pop("PYTHONPATH", None)
    tool_directories = {
        os.fspath(Path(tool).resolve().parent)
        for name in ("cmake", "ninja", "nm", "c++")
        if (tool := shutil.which(name)) is not None
    }
    if windows:
        path_tail = clean_environment.get("PATH", "").split(os.pathsep)
    else:
        path_tail = ["/usr/bin", "/bin", "/usr/sbin", "/sbin"]
    clean_environment["PATH"] = os.pathsep.join(
        [
            os.fspath(commands),
            *sorted(tool_directories),
            *path_tail,
        ]
    )
    plan_command = [
        *launcher,
        "model",
        "plan",
        "--sdk-root",
        sdk_root,
        "--source-root",
        source,
        "--entry",
        "model.top:pipeline",
        "--config",
        source / "model.toml",
        "--out-dir",
        plan,
        "--json",
    ]
    emit_command = [
        *launcher,
        "model",
        "emit-cpp",
        "--sdk-root",
        sdk_root,
        "--plan",
        plan / "model-plan.json",
        "--out-dir",
        generated,
        "--manifest",
        generated / "model-manifest.json",
        "--depfile",
        generated / "model.d",
        "--json",
    ]
    run(plan_command, cwd=source, env=clean_environment)
    first_plan = tree_hashes(plan)
    run(plan_command, cwd=source, env=clean_environment)
    if tree_hashes(plan) != first_plan:
        raise ValueError("incremental model plan is not byte-identical")

    run(emit_command, cwd=plan, env=clean_environment)
    first_generated = tree_hashes(generated)
    run(emit_command, cwd=plan, env=clean_environment)
    if tree_hashes(generated) != first_generated:
        raise ValueError("incremental model emission is not byte-identical")

    parallel = [
        subprocess.Popen(
            [os.fspath(item) for item in emit_command],
            cwd=plan,
            env=clean_environment,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        for _ in range(2)
    ]
    parallel_results = [process.communicate() for process in parallel]
    for process, (stdout, stderr) in zip(parallel, parallel_results, strict=True):
        if process.returncode:
            raise ValueError("parallel same-root emission failed: " + stdout + stderr)
    if tree_hashes(generated) != first_generated:
        raise ValueError("parallel same-root emission changed accepted output")

    rejected_plan = workspace / "mismatched-model-plan.json"
    mismatched = load(plan / "model-plan.json")
    mismatched["sdk"]["source_revision"] = "0" * 40
    rejected_plan.write_text(
        json.dumps(mismatched, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    rejected_root = workspace / "rejected-generated"
    rejected_command = list(emit_command)
    rejected_command[rejected_command.index(plan / "model-plan.json")] = rejected_plan
    rejected_command[rejected_command.index(generated)] = rejected_root
    rejected_command[rejected_command.index(generated / "model-manifest.json")] = (
        rejected_root / "model-manifest.json"
    )
    rejected_command[rejected_command.index(generated / "model.d")] = (
        rejected_root / "model.d"
    )
    rejected = subprocess.run(
        [os.fspath(item) for item in rejected_command],
        cwd=plan,
        env=clean_environment,
        text=True,
        capture_output=True,
        check=False,
    )
    if rejected.returncode == 0 or rejected_root.exists():
        raise ValueError("mismatched SDK identity did not fail before publication")
    if tree_hashes(generated) != first_generated:
        raise ValueError("failed emission did not preserve the prior bundle")

    unsupported_source = source / "model/unsupported.py"
    unsupported_source.write_text(
        "import agentic_circuit as ac\n\n"
        "@ac.system\n"
        "def unsupported(entries: int) -> None:\n"
        "    source = ac.source(int, depth=entries)\n"
        "    ac.sink(source)\n",
        encoding="utf-8",
    )
    unsupported_plan = workspace / "unsupported-plan"
    unsupported_command = list(plan_command)
    unsupported_command[unsupported_command.index("model.top:pipeline")] = (
        "model.unsupported:unsupported"
    )
    unsupported_command[unsupported_command.index(plan)] = unsupported_plan
    unsupported = subprocess.run(
        [os.fspath(item) for item in unsupported_command],
        cwd=source,
        env=clean_environment,
        text=True,
        capture_output=True,
        check=False,
    )
    if unsupported.returncode == 0 or unsupported_plan.exists():
        raise ValueError("unsupported runtime-bound model escaped plan validation")

    top = source / "model/top.py"
    original_top = top.read_text(encoding="utf-8")
    top.write_text(
        original_top.replace(
            "    completed = complete(source)\n",
            "    completed_once = complete(source)\n"
            "    completed = complete(completed_once)\n",
        ),
        encoding="utf-8",
    )
    run(plan_command, cwd=source, env=clean_environment)
    if tree_hashes(plan) == first_plan:
        raise ValueError("topology change did not change the model plan")
    run(emit_command, cwd=plan, env=clean_environment)
    if tree_hashes(generated) == first_generated:
        raise ValueError("topology change did not replace generated bytes")
    run(
        [
            "cmake",
            "-S",
            MODEL_CONSUMER,
            "-B",
            build,
            f"-DCMAKE_PREFIX_PATH={sdk_root}",
            f"-DAGENTIC_MODEL_ROOT={generated}",
            "-DCMAKE_DISABLE_FIND_PACKAGE_LLVM=TRUE",
            "-DCMAKE_DISABLE_FIND_PACKAGE_MLIR=TRUE",
            # The SDK ships Release binaries. A Visual Studio generator defaults
            # to Debug, and MSVC refuses to mix the two: LNK2038 reports
            # _ITERATOR_DEBUG_LEVEL and RuntimeLibrary mismatches against
            # gfsim.lib. Pin the configuration for single- and multi-config
            # generators alike.
            "-DCMAKE_BUILD_TYPE=Release",
        ],
        cwd=workspace,
        env=clean_environment,
    )
    run(
        ["cmake", "--build", build, "--config", "Release"],
        cwd=workspace,
        env=clean_environment,
    )
    plugin_names = (
        ("model-plugin.dll",)
        if windows
        else ("libmodel-plugin.so", "libmodel-plugin.dylib")
    )
    verify_only_export(find_build_artifact(build, plugin_names))
    run(
        [find_build_artifact(build, (f"model-consumer{suffix}",))],
        cwd=workspace,
        env=clean_environment,
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--forbidden-path", action="append", default=[])
    parser.add_argument("--attestation", type=Path)
    parser.add_argument("--candidate-tag")
    parser.add_argument("--release-url")
    parser.add_argument("--skip-install", action="store_true")
    args = parser.parse_args()
    if bool(args.candidate_tag) != bool(args.release_url):
        raise ValueError("candidate tag and release URL must be supplied together")
    external_manifest = load(args.manifest)
    with tempfile.TemporaryDirectory(prefix="pycircuit-sdk-verify-") as raw:
        workspace = Path(raw)
        first = workspace / "relocated-a"
        second = workspace / "relocated-b"
        extract(args.archive, first)
        extract(args.archive, second)
        _, first_wheels = validate_tree(
            first, args.manifest, tuple(args.forbidden_path)
        )
        _, second_wheels = validate_tree(
            second, args.manifest, tuple(args.forbidden_path)
        )
        if [path.name for path in first_wheels] != [
            path.name for path in second_wheels
        ]:
            raise ValueError("relocated SDK wheel closures differ")
        if [sha256(path) for path in first_wheels] != [
            sha256(path) for path in second_wheels
        ]:
            raise ValueError("relocated SDK trees do not preserve wheel bytes")
        if not args.skip_install:
            installed_smoke(second, second_wheels, workspace)
    if args.attestation is not None:
        args.attestation.parent.mkdir(parents=True, exist_ok=True)
        args.attestation.write_text(
            json.dumps(
                {
                    "schema": "pycircuit-platform-verification",
                    "version": "1",
                    "source_revision": external_manifest["source_revision"],
                    "platform": external_manifest["platform"]["id"],
                    "candidate_tag": args.candidate_tag,
                    "release_url": args.release_url,
                    "archive_sha256": sha256(args.archive),
                    "manifest_sha256": sha256(args.manifest),
                    "exact_manifest_closure": True,
                    "native_dependency_closure": True,
                    "relocated_model_consumer": not args.skip_install,
                    "incremental_determinism": not args.skip_install,
                    "topology_change": not args.skip_install,
                    "parallel_same_root": not args.skip_install,
                    "mismatch_rejection": not args.skip_install,
                    "unsupported_boundary_rejection": not args.skip_install,
                    "verified": not args.skip_install,
                },
                indent=2,
                sort_keys=True,
            )
            + "\n",
            encoding="utf-8",
        )
    checks = "exact closure and native dependency relocation"
    if not args.skip_install:
        checks += ", model plan/emit/build/run and adversarial generation"
    sys.stdout.write(f"platform SDK candidate: OK ({checks})\n")
    return 0


def entrypoint() -> int:
    try:
        return main()
    except (
        OSError,
        KeyError,
        TypeError,
        ValueError,
        json.JSONDecodeError,
        subprocess.CalledProcessError,
    ) as error:
        sys.stderr.write(f"error: {error}\n")
        return 1


if __name__ == "__main__":
    raise SystemExit(entrypoint())

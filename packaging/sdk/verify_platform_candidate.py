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


def _native_files(root: Path, manifest: dict[str, Any]) -> list[Path]:
    result: list[Path] = []
    for record in manifest["files"]:
        path = root / record["path"]
        if record["kind"] not in {"tool", "library"} or path.suffix == ".a":
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
    observed_dependencies: set[tuple[str, str]] = set()
    for path in _native_files(root, manifest):
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
    wheels = sorted((root / "python/wheelhouse").glob("*.whl"))
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


def verify_only_export(plugin: Path) -> None:
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
        for path in sorted(item for item in root.rglob("*") if item.is_file())
    )


def installed_smoke(sdk_root: Path, wheels: list[Path], workspace: Path) -> None:
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
    run([commands / "pycircuit", "--help"], cwd=workspace)
    cli = sdk_root / "bin/agentic-circuit"
    for required in (
        cli,
        sdk_root / "bin/acir-opt",
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
    clean_environment["PATH"] = os.pathsep.join(
        [
            os.fspath(commands),
            *sorted(tool_directories),
            "/usr/bin",
            "/bin",
            "/usr/sbin",
            "/sbin",
        ]
    )
    plan_command = [
        cli,
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
        cli,
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
            raise ValueError(
                "parallel same-root emission failed: " + stdout + stderr
            )
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
        ],
        cwd=workspace,
        env=clean_environment,
    )
    run(["cmake", "--build", build], cwd=workspace, env=clean_environment)
    plugins = list(build.glob("libmodel-plugin.*"))
    if len(plugins) != 1:
        raise ValueError(f"expected one generated model plugin, found {plugins}")
    verify_only_export(plugins[0])
    run([build / "model-consumer"], cwd=workspace, env=clean_environment)


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

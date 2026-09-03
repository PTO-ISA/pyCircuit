#!/usr/bin/env python3
"""Build canonical PYC, C++, and Verilog artifacts from frozen Queue ACIR."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _read_json(path: Path) -> dict[str, object]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"JSON document must be an object: {path}")
    return value


def _canonical_json(value: object) -> str:
    return json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n"


def _opcode_inventory(acir: str, catalog: dict[str, object]) -> list[str]:
    entries = catalog.get("entries")
    if not isinstance(entries, list):
        raise ValueError("official opcode catalog entries are invalid")
    official = {
        entry["operation"]
        for entry in entries
        if isinstance(entry, dict) and isinstance(entry.get("operation"), str)
    }
    mentioned = set(re.findall(r"\bac\.[a-z][a-z0-9_.]*\b", acir))
    return sorted(official & mentioned)


def _write_atomic(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(
        dir=path.parent, prefix=f".{path.name}.", text=True
    )
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def _run(command: tuple[str, ...], *, stage: str) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        command,
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"{stage} failed with exit {completed.returncode}:\n{completed.stderr}"
        )
    return completed


def _normalize_cpp_manifest(path: Path) -> None:
    manifest = _read_json(path)
    manifest["include_dirs"] = ["."]
    profile = manifest.get("profile_summary")
    if isinstance(profile, dict):
        profile.pop("pass_time_ms", None)
        profile.pop("pycc_peak_rss_bytes", None)
    runtime = manifest.get("runtime")
    if isinstance(runtime, dict):
        runtime["cmake_config_dir"] = "${PYC_TOOLCHAIN_ROOT}/share/pycircuit/cmake"
        runtime["include_dirs"] = ["${PYC_TOOLCHAIN_ROOT}/include"]
        runtime["lib_dirs"] = ["${PYC_TOOLCHAIN_ROOT}/lib"]
        runtime["library_files"] = ["${PYC_TOOLCHAIN_ROOT}/lib/libpyc6_runtime.a"]
        runtime["toolchain_root_hint"] = "${PYC_TOOLCHAIN_ROOT}"
    path.write_text(
        json.dumps(manifest, sort_keys=True, indent=2) + "\n", encoding="utf-8"
    )


def _artifacts(root: Path, backend: str) -> list[dict[str, object]]:
    result: list[dict[str, object]] = []
    for path in sorted(
        candidate for candidate in root.rglob("*") if candidate.is_file()
    ):
        data = path.read_bytes()
        result.append(
            {
                "backend": backend,
                "path": path.relative_to(root).as_posix(),
                "sha256": _sha256(data),
                "size": len(data),
            }
        )
    return result


def main() -> int:
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("frozen_acir", type=Path)
    parser.add_argument("--pycgen-tool", required=True, type=Path)
    parser.add_argument("--pycc", required=True, type=Path)
    parser.add_argument("--toolchain-lock", required=True, type=Path)
    parser.add_argument("--toolchain-metadata", required=True, type=Path)
    parser.add_argument("--cxx", required=True, type=Path)
    parser.add_argument("--verilator", required=True, type=Path)
    parser.add_argument("--pyc-output", required=True, type=Path)
    parser.add_argument("--cpp-output-dir", required=True, type=Path)
    parser.add_argument("--verilog-output-dir", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    arguments = parser.parse_args()

    for target in (
        arguments.pyc_output,
        arguments.cpp_output_dir,
        arguments.verilog_output_dir,
        arguments.manifest,
    ):
        if target.exists():
            parser.error(f"output already exists: {target}")

    lock = _read_json(arguments.toolchain_lock)
    metadata = _read_json(arguments.toolchain_metadata)
    catalog_path = (
        Path(__file__).resolve().parents[3]
        / "schemas"
        / "agentic-circuit"
        / "opcodes.json"
    )
    catalog = _read_json(catalog_path)
    locked_commit = lock.get("pycircuit_commit")
    if locked_commit != "repo-local" and metadata.get("git_sha") != locked_commit:
        parser.error("pyCircuit commit does not match toolchain lock")
    if locked_commit == "repo-local" and not metadata.get("git_sha"):
        parser.error("repo-local pyCircuit metadata has no git revision")
    if metadata.get("llvm_version") != lock.get("llvm_version"):
        parser.error("pycc LLVM version does not match toolchain lock")

    with tempfile.TemporaryDirectory() as directory:
        temporary = Path(directory)
        pyc = temporary / "model.pyc"
        cpp = temporary / "cpp"
        verilog = temporary / "verilog"
        emitted = _run(
            (str(arguments.pycgen_tool), str(arguments.frozen_acir)),
            stage="ACIR-to-PYC",
        )
        pyc.write_text(emitted.stdout, encoding="utf-8")
        _run(
            (
                str(arguments.pycc),
                str(pyc),
                "--emit=cpp",
                f"--out-dir={cpp}",
                "--cpp-split=module",
                "--cpp-shard-max-ast-nodes=512",
                "--cpp-shard-threshold-lines=1000",
                "--cpp-shard-threshold-bytes=65536",
                "--hierarchy-policy=strict",
                "--inline-policy=off",
                "--build-profile=dev-fast",
            ),
            stage="pycc C++",
        )
        _run(
            (
                str(arguments.pycc),
                str(pyc),
                "--emit=verilog",
                f"--out-dir={verilog}",
                "--hierarchy-policy=strict",
                "--inline-policy=off",
                "--build-profile=dev-fast",
                "--include-primitives",
            ),
            stage="pycc Verilog",
        )
        cpp_manifest = _read_json(cpp / "cpp_compile_manifest.json")
        sources = cpp_manifest.get("sources")
        if not isinstance(sources, list) or not sources:
            raise RuntimeError("pycc C++ manifest contains no sources")
        source_paths = [
            cpp / str(source["path"]) for source in sources if isinstance(source, dict)
        ]
        include_root = arguments.toolchain_metadata.parents[2] / "include"
        _run(
            (
                str(arguments.cxx),
                "-std=c++17",
                "-I",
                str(cpp),
                "-I",
                str(include_root),
                "-fsyntax-only",
                *(str(path) for path in source_paths),
            ),
            stage="generated PYC C++ syntax",
        )

        verilog_manifest = _read_json(verilog / "manifest.json")
        top = str(verilog_manifest.get("top", ""))
        modules = verilog_manifest.get("verilog_modules")
        if not top or not isinstance(modules, list) or not modules:
            raise RuntimeError("pycc Verilog manifest is incomplete")
        _run(
            (
                str(arguments.verilator),
                "--lint-only",
                "--timing",
                "-Wall",
                "-Wno-fatal",
                "--top-module",
                top,
                *(str(verilog / str(module)) for module in modules),
            ),
            stage="Verilator lint",
        )

        # Compile with pycc's real toolchain paths, then remove host-specific
        # paths only from the manifest that is published and fingerprinted.
        _normalize_cpp_manifest(cpp / "cpp_compile_manifest.json")

        pyc_bytes = pyc.read_bytes()
        frozen_acir_bytes = arguments.frozen_acir.read_bytes()
        manifest = {
            "artifacts": [
                *_artifacts(cpp, "cpp"),
                *_artifacts(verilog, "verilog"),
            ],
            "contract_epoch": "0.4",
            "build_profile": "dev-fast",
            "frozen_acir_sha256": _sha256(frozen_acir_bytes),
            "gates": ["pycc-cpp", "pycc-verilog", "cxx-syntax", "verilator-lint"],
            "hierarchy_policy": "strict",
            "inline_policy": "off",
            "llvm_version": metadata["llvm_version"],
            "opcode_catalog_sha256": _sha256(catalog_path.read_bytes()),
            "opcode_lowering_inventory": _opcode_inventory(
                frozen_acir_bytes.decode("utf-8"), catalog
            ),
            "pyc_interface": lock["pyc_interface"],
            "pyc_ir_sha256": _sha256(pyc_bytes),
            "pycc_sha256": _sha256(arguments.pycc.read_bytes()),
            "pycircuit_commit": metadata["git_sha"],
            "schema": "agentic-circuit-pyc-build-manifest",
            "targets": ["cpp", "verilog"],
            "version": "0.4",
        }

        arguments.cpp_output_dir.parent.mkdir(parents=True, exist_ok=True)
        arguments.verilog_output_dir.parent.mkdir(parents=True, exist_ok=True)
        os.replace(cpp, arguments.cpp_output_dir)
        os.replace(verilog, arguments.verilog_output_dir)
        _write_atomic(arguments.pyc_output, pyc.read_text(encoding="utf-8"))
        _write_atomic(arguments.manifest, _canonical_json(manifest))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

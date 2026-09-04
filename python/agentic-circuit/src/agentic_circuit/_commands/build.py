"""Immutable native build orchestration."""

from __future__ import annotations

import json
import platform
import shutil
import subprocess
import sys
from dataclasses import dataclass, replace
from pathlib import Path
from typing import Literal, NoReturn

from .._canonical_json import canonical_json_bytes
from .._diagnostics import Diagnostic
from .._native_api import NativeRequest, native_extension_path, run_native_compiler
from .._output import OutputSink
from .._workspace import UserInputError, WorkspaceConfig
from .check import _has_errors, capture

Profile = Literal["fast", "validated", "custom"]


@dataclass(frozen=True, slots=True)
class BuildOptions:
    profile: Profile
    pass_pipeline: str | None
    verify_after_each: bool
    instrumentation_layers: tuple[str, ...]


@dataclass(frozen=True, slots=True)
class BuildPublication:
    directory: Path
    executable: Path
    manifest: Path
    fingerprint: str
    cache_hit: bool


@dataclass(frozen=True, slots=True)
class BuildAttempt:
    publication: BuildPublication | None
    diagnostics: tuple[Diagnostic, ...]
    exit_code: int
    profile: Profile


def _fail(code: str, message: str) -> NoReturn:
    raise UserInputError(
        Diagnostic(stage="build", code=code, severity="error", message=message)
    )


def build_options(arguments: object, workspace: WorkspaceConfig) -> BuildOptions:
    profile = getattr(arguments, "profile", None) or workspace.build_profile
    pipeline = getattr(arguments, "pass_pipeline", None)
    if profile == "custom" and not pipeline:
        _fail("ACPY-CLI-PIPELINE", "custom requires --pass-pipeline")
    if profile != "custom" and pipeline is not None:
        _fail("ACPY-CLI-PIPELINE", "--pass-pipeline requires profile custom")
    return BuildOptions(
        profile=profile,
        pass_pipeline=pipeline,
        verify_after_each=(
            profile == "validated"
            or bool(getattr(arguments, "verify_after_each", False))
        ),
        instrumentation_layers=workspace.instrumentation_layers,
    )


def _output(arguments: object) -> Path:
    value = getattr(arguments, "output_dir", None)
    if value is None:
        _fail("ACPY-CLI-OUTPUT", "build requires -o/--output-dir")
    return Path(value).resolve()


def _source_files(acpy: bytes) -> list[dict[str, str]]:
    try:
        document = json.loads(acpy)
        values = document["sources"]
        if type(values) is not list:
            raise TypeError("sources is not a list")
        result: list[dict[str, str]] = []
        for value in values:
            if type(value) is not dict or set(value) != {"path", "sha256"}:
                raise TypeError("source identity is not closed")
            path = value["path"]
            sha256 = value["sha256"]
            if type(path) is not str or type(sha256) is not str:
                raise TypeError("source identity fields are not strings")
            result.append({"path": path, "sha256": sha256})
        return result
    except (UnicodeError, json.JSONDecodeError, KeyError, TypeError) as error:
        _fail("ACPY-VERIFY-001", f"ACPy source provenance is invalid: {error}")


def _runtime_linkage() -> tuple[list[str], list[str], list[str]]:
    extension = native_extension_path()
    repository = Path(__file__).resolve().parents[5]
    build_root = extension.parents[2]
    source_include = repository / "simulator" / "gfsim" / "include"
    build_layouts = (
        (
            build_root / "gfsim" / "libgfsim.a",
            build_root / "lib" / "Bindings" / "libACIRBindings.a",
        ),
        (
            build_root / "compiler" / "acir" / "gfsim" / "libgfsim.a",
            build_root / "compiler" / "acir" / "lib" / "Bindings" / "libACIRBindings.a",
        ),
    )
    for libraries in build_layouts:
        if source_include.is_dir() and all(path.is_file() for path in libraries):
            return (
                [source_include.resolve().as_posix()],
                [path.resolve().as_posix() for path in libraries],
                [],
            )

    for root in extension.parents:
        include = root / "include"
        installed = (root / "lib/libgfsim.a", root / "lib/libACIRBindings.a")
        if (include / "gfsim").is_dir() and all(
            library.is_file() for library in installed
        ):
            llvm_libraries = sorted(
                [path for path in (root / "lib").glob("libLLVM.*") if path.is_file()],
                key=lambda path: path.name,
            )
            if not llvm_libraries:
                # Standalone development installs may intentionally omit a
                # bundled LLVM dylib.  Leaving linker_flags empty lets the
                # native bridge inject the exact LLVM link flags recorded at
                # its own build, while the installed gfsim/ACIR libraries and
                # headers remain prefix-local.
                return (
                    [include.resolve().as_posix()],
                    [library.resolve().as_posix() for library in installed],
                    [],
                )
            llvm_library = llvm_libraries[0].resolve()
            library_root = llvm_library.parent
            return (
                [include.resolve().as_posix()],
                [
                    *(library.resolve().as_posix() for library in installed),
                    llvm_library.as_posix(),
                ],
                [f"-Wl,-rpath,{library_root.as_posix()}"],
            )
    raise RuntimeError("Agentic Circuit runtime development files are unavailable")


def _binding_registry(
    component_roots: tuple[Path, ...], *, native_target: str | None = None
) -> bytes:
    candidates: list[object] = []
    requests: list[object] = []
    for root in sorted(component_roots):
        for path in sorted(root.rglob("*.binding.json")):
            try:
                document = json.loads(path.read_text())
            except (OSError, UnicodeError, json.JSONDecodeError) as error:
                _fail("ACLOWER-BINDING-OPTIONS", f"cannot load {path}: {error}")
            if type(document) is not dict or set(document) != {
                "candidates",
                "requests",
            }:
                _fail(
                    "ACLOWER-BINDING-OPTIONS",
                    f"binding registry {path} is not a closed registry",
                )
            if (
                type(document["candidates"]) is not list
                or type(document["requests"]) is not list
            ):
                _fail(
                    "ACLOWER-BINDING-OPTIONS",
                    f"binding registry {path} arrays are invalid",
                )
            for candidate in document["candidates"]:
                if (
                    native_target is not None
                    and type(candidate) is dict
                    and candidate.get("target") == "native"
                ):
                    candidate = dict(candidate)
                    candidate["target"] = native_target
                candidates.append(candidate)
            requests.extend(document["requests"])
    return canonical_json_bytes({"candidates": candidates, "requests": requests})


def _logical_diagnostics(
    diagnostics: tuple[Diagnostic, ...],
) -> tuple[Diagnostic, ...]:
    stages = {
        "acir-parse": "acir-elaboration",
        "acir-verify": "acir-core",
        "acir-normalize": "collection-canonicalization",
        "acir-freeze": "topology-freeze",
        "acsim-lower": "process-state-lowering",
        "acsim-verify": "acsim",
        "cxx-emit": "cxx",
        "cxx-contract": "cxx",
        "compile": "cxx",
        "link": "cxx",
        "publish": "cxx",
    }
    return tuple(
        replace(item, stage=stages.get(item.stage, item.stage)) for item in diagnostics
    )


def _failure_exit(diagnostics: tuple[Diagnostic, ...]) -> int:
    if any(
        item.stage == "cxx"
        or item.code.startswith(("ACBUILD-", "ACLOWER-CXX", "ACLOWER-COMPILE"))
        for item in diagnostics
    ):
        return 4
    if all(
        item.code.startswith(("ACPY-", "ACELAB-", "ACIR-", "ACLOWER-"))
        for item in diagnostics
    ):
        return 2
    return 3


def build_publication(
    arguments: object,
    workspace: WorkspaceConfig,
    *,
    output: Path | None = None,
) -> BuildAttempt:
    options = build_options(arguments, workspace)
    destination = output.resolve() if output is not None else _output(arguments)
    frontend = capture(arguments, workspace)
    if _has_errors(frontend.diagnostics):
        return BuildAttempt(None, frontend.diagnostics, 2, options.profile)
    if frontend.frontend_kind == "queue_rule":
        _fail(
            "ACPY-RULE-006",
            "Queue/rule build uses ac.jit(system).materialize_cpp(...) until "
            "the QueueGraph executable harness is a first-class CLI artifact",
        )
    if frontend.acpy is None or frontend.acir is None:
        _fail("ACPY-VERIFY-001", "frontend produced incomplete build artifacts")
    compiler = shutil.which(workspace.compiler)
    if compiler is None:
        return BuildAttempt(
            None,
            (
                Diagnostic(
                    stage="cxx",
                    code="ACBUILD-COMPILER-001",
                    severity="error",
                    message=f"C++ compiler is unavailable: {workspace.compiler}",
                ),
            ),
            4,
            options.profile,
        )

    try:
        native_target = (
            subprocess.run(
                [compiler, "-dumpmachine"],
                check=True,
                capture_output=True,
                text=True,
            )
            .stdout.splitlines()[0]
            .strip()
        )
    except (OSError, subprocess.CalledProcessError, IndexError) as error:
        return BuildAttempt(
            None,
            (
                Diagnostic(
                    stage="cxx",
                    code="ACBUILD-COMPILER-001",
                    severity="error",
                    message=f"cannot query C++ compiler target: {error}",
                ),
            ),
            4,
            options.profile,
        )

    include_roots, link_inputs, linker_flags = _runtime_linkage()
    native_options: list[tuple[str, object]] = [
        ("profile", options.profile),
        ("binding_lock", b"[]"),
        (
            "binding_registry",
            _binding_registry(workspace.component_roots, native_target=native_target),
        ),
        ("frontend_acpy", frontend.acpy),
        ("frontend_acir", frontend.acir),
        (
            "build",
            {
                "project_name": workspace.project_name,
                "project_identity": (
                    f"project:{workspace.project_name}@{workspace.project_version}"
                ),
                "system_name": getattr(arguments, "system", None)
                or workspace.default_system,
                "system_identity": (
                    "system:"
                    + (getattr(arguments, "system", None) or workspace.default_system)
                ),
                "source_files": _source_files(frontend.acpy),
                "python_version": (
                    f"{sys.implementation.name} {platform.python_version()}"
                ),
                "helper_identities": [],
                "compiler": compiler,
                "standard_library": workspace.standard_library,
                "instrumentation_layers": list(options.instrumentation_layers),
                "output_root": destination.as_posix(),
                "include_roots": include_roots,
                "link_inputs": link_inputs,
                "linker_flags": linker_flags,
            },
        ),
    ]
    if options.pass_pipeline is not None:
        native_options.append(("custom_pipeline", options.pass_pipeline))
    if options.verify_after_each:
        native_options.append(("verify_after_each", True))
    native = run_native_compiler(
        NativeRequest(
            acir=frontend.acir,
            stop_after="publish",
            emits=("executable",),
            options=tuple(native_options),
        )
    )
    diagnostics = _logical_diagnostics(native.diagnostics)
    if _has_errors(diagnostics):
        return BuildAttempt(
            None, diagnostics, _failure_exit(diagnostics), options.profile
        )
    if (
        native.build_directory is None
        or native.executable is None
        or native.build_fingerprint is None
        or native.cache_hit is None
    ):
        raise RuntimeError("native build returned incomplete publication identity")
    publication = BuildPublication(
        directory=Path(native.build_directory),
        executable=Path(native.executable),
        manifest=Path(native.build_directory) / "build-manifest.json",
        fingerprint=native.build_fingerprint,
        cache_hit=native.cache_hit,
    )
    return BuildAttempt(publication, diagnostics, 0, options.profile)


def run(arguments: object, workspace: WorkspaceConfig, sink: OutputSink) -> int:
    attempt = build_publication(arguments, workspace)
    if attempt.publication is None:
        sink.diagnostics(attempt.diagnostics)
        return attempt.exit_code
    publication = attempt.publication
    sink.result(
        {
            "schema": "agentic-circuit-build-result",
            "version": "0.1",
            "contract_epoch": "0.5",
            "status": "passed",
            "profile": attempt.profile,
            "directory": publication.directory.as_posix(),
            "executable": publication.executable.as_posix(),
            "manifest": publication.manifest.as_posix(),
            "build_fingerprint": publication.fingerprint,
            "cache_hit": publication.cache_hit,
        },
        human=f"built {publication.executable}",
    )
    return 0

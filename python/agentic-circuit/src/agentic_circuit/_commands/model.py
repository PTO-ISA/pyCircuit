"""Installed, manifest-bound model planning commands."""

from __future__ import annotations

import ast
import fcntl
import json
import os
import platform
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from importlib.machinery import EXTENSION_SUFFIXES
from pathlib import Path, PurePosixPath
from typing import NoReturn

import tomllib

from .._canonical_json import (
    JsonValue,
    canonical_json_bytes,
    sha256_bytes,
    validate_ijson_value,
)
from .._capture_worker import CaptureWorkerRequest, run_capture_worker
from .._diagnostics import Diagnostic
from .._native_api import NativeRequest, native_extension_path, run_native_compiler
from .._output import OutputSink
from .._source_closure import (
    SourceClosure,
    SourceClosureEntry,
    capture_source_closure,
)
from .._staging import ArtifactStage
from .._workspace import UserInputError

_ENTRY = re.compile(
    r"^(?P<module>[A-Za-z_][A-Za-z0-9_]*(?:\.[A-Za-z_][A-Za-z0-9_]*)*):"
    r"(?P<symbol>[A-Za-z_][A-Za-z0-9_]*)$"
)
_REVISION = re.compile(r"^[0-9a-f]{40}$")
_SHA256 = re.compile(r"^sha256:[0-9a-f]{64}$")
_LOGICAL_PATH = re.compile(
    r"^(?!/)(?![A-Za-z]:)(?!.*\\\\)(?!.*(?:^|/)\.\.?(/|$))(?!.*//)[A-Za-z0-9._+@/-]+$"
)
_PRODUCT_VERSION = "6.0.0"
_DISTRIBUTIONS = {
    "agentic-circuit": "0.1.0",
    "pycircuit-hisi": "6.0.0",
    "pycircuit-semantic-core": "6.0.0",
}
_ABI = {
    "acpy_epoch": "0.5",
    "acir_contract": "0.5",
    "sdk_manifest": "1",
    "release_index": "1",
    "model_plan": "1",
    "model_manifest": "1",
    "generator_abi": "1",
    "runtime_abi": "1",
    "consumer_lock": "1",
}
_SDK_CAPABILITIES = (
    "cycle-aware-signal",
    "pyc-cpp",
    "pyc-verilog",
    "agentic-model-plan",
    "agentic-model-emit-cpp",
    "gfsim-runtime-v1",
)
_PLAN_CAPABILITIES = ("queuegraph-v2", "multi-tu-v1", "runtime-abi-v1")
_PLAN_OUTPUTS = (
    "include/generated/model.h",
    "src/generated/model.cpp",
    "src/generated/queuegraph.cpp",
)
_MANIFEST_KEYS = frozenset(
    {
        "schema",
        "version",
        "contract_epoch",
        "product_version",
        "source_revision",
        "platform",
        "distributions",
        "abi",
        "capabilities",
        "runtime_dependencies",
        "self_path",
        "files",
    }
)
_FILE_KEYS = frozenset({"path", "kind", "sha256", "size"})
_FILE_KINDS = frozenset(
    {"tool", "header", "library", "cmake", "schema", "wheel", "license", "metadata"}
)
_RUNTIME_DEPENDENCY_KEYS = frozenset({"name", "kind", "version", "sha256"})
_CONTRACT_DECORATORS = frozenset(
    {"enum", "interface", "invariant", "packet", "protocol", "struct"}
)


@dataclass(frozen=True, slots=True)
class SdkIdentity:
    root: Path
    manifest_sha256: str
    source_revision: str
    queue_plan_tool: Path
    native_extension: Path
    native_sha256: str
    native_size: int


def _fail(code: str, message: str) -> NoReturn:
    raise UserInputError(
        Diagnostic(
            stage="model-plan",
            code=code,
            severity="error",
            message=message,
        )
    )


def _regular_file(path: Path, *, code: str, label: str) -> Path:
    try:
        resolved = path.expanduser().resolve(strict=True)
    except OSError as error:
        _fail(code, f"{label} is unavailable: {error}")
    if not resolved.is_file():
        _fail(code, f"{label} is not a regular file: {resolved}")
    return resolved


def _closed_dict(value: object, keys: frozenset[str], label: str) -> dict[str, object]:
    if type(value) is not dict or set(value) != keys:
        _fail("ACSDK-PLAN-MANIFEST-001", f"SDK manifest {label} is not closed")
    return value


def _host_platform_id() -> str:
    system = platform.system().lower()
    machine = platform.machine().lower()
    if system == "darwin" and machine in {"arm64", "aarch64"}:
        return "macos-arm64"
    if system == "linux" and machine in {"x86_64", "amd64"}:
        return "linux-x86_64"
    _fail(
        "ACSDK-PLAN-PLATFORM-001", f"unsupported SDK host platform: {system}-{machine}"
    )


def _validate_platform(value: object) -> None:
    common = {
        "cxx_compiler",
        "cxx_standard",
        "python",
        "id",
        "os",
        "architecture",
        "runner",
        "host_triple",
        "minimum_os",
        "minimum_libc",
        "cxx_abi",
    }
    if type(value) is not dict or set(value) != common:
        _fail("ACSDK-PLAN-MANIFEST-001", "SDK platform record is not closed")
    expected: dict[str, object]
    if _host_platform_id() == "macos-arm64":
        expected = {
            "cxx_standard": "20",
            "python": "3.11",
            "id": "macos-arm64",
            "os": "macos",
            "architecture": "arm64",
            "runner": "macos-15",
            "host_triple": "arm64-apple-darwin",
            "minimum_os": "macOS 15",
            "minimum_libc": None,
            "cxx_abi": "Apple libc++",
        }
    else:
        expected = {
            "cxx_standard": "20",
            "python": "3.11",
            "id": "linux-x86_64",
            "os": "linux",
            "architecture": "x86_64",
            "runner": "ubuntu-24.04",
            "host_triple": "x86_64-linux-gnu",
            "minimum_os": "Ubuntu 24.04",
            "minimum_libc": "glibc 2.39",
            "cxx_abi": "libstdc++ CXX11 ABI",
        }
    compiler = value.get("cxx_compiler")
    if (
        sys.version_info[:2] != (3, 11)
        or type(compiler) is not str
        or not compiler
        or any(
            value.get(key) != expected_value for key, expected_value in expected.items()
        )
    ):
        _fail("ACSDK-PLAN-PLATFORM-001", "SDK platform does not match this host")


def _validate_runtime_dependencies(value: object) -> None:
    if type(value) is not list or not value:
        _fail("ACSDK-PLAN-MANIFEST-001", "SDK runtime dependencies are invalid")
    for item in value:
        dependency = _closed_dict(item, _RUNTIME_DEPENDENCY_KEYS, "runtime dependency")
        if (
            type(dependency["name"]) is not str
            or not dependency["name"]
            or type(dependency["version"]) is not str
            or not dependency["version"]
            or dependency["kind"] not in {"bundled", "system"}
            or (
                dependency["kind"] == "bundled"
                and (
                    type(dependency["sha256"]) is not str
                    or not _SHA256.fullmatch(dependency["sha256"])
                )
            )
            or (dependency["kind"] == "system" and dependency["sha256"] is not None)
        ):
            _fail("ACSDK-PLAN-MANIFEST-001", "SDK runtime dependency is invalid")


def _manifest_file(
    root: Path,
    entries: dict[str, dict[str, object]],
    relative: str,
    *,
    kind: str | None = None,
) -> Path:
    entry = entries.get(relative)
    if entry is None:
        _fail("ACSDK-PLAN-MANIFEST-002", f"SDK manifest does not own {relative}")
    if kind is not None and entry["kind"] != kind:
        _fail("ACSDK-PLAN-MANIFEST-002", f"SDK manifest kind is invalid for {relative}")
    path = _regular_file(
        root / relative, code="ACSDK-PLAN-MANIFEST-002", label=relative
    )
    if path != root.joinpath(*PurePosixPath(relative).parts):
        _fail("ACSDK-PLAN-MANIFEST-002", f"SDK file escapes its prefix: {relative}")
    raw = path.read_bytes()
    if entry["size"] != len(raw) or entry["sha256"] != sha256_bytes(raw):
        _fail("ACSDK-PLAN-MANIFEST-003", f"SDK file identity mismatch: {relative}")
    return path


def _sdk_identity(value: object) -> SdkIdentity:
    try:
        root = Path(value).expanduser().resolve(strict=True)
    except (OSError, TypeError) as error:
        _fail("ACSDK-PLAN-ROOT-001", f"SDK root is unavailable: {error}")
    if not root.is_dir():
        _fail("ACSDK-PLAN-ROOT-001", f"SDK root is not a directory: {root}")
    manifest_path = _regular_file(
        root / "share/pycircuit/sdk-manifest.json",
        code="ACSDK-PLAN-MANIFEST-001",
        label="SDK manifest",
    )
    if manifest_path != root / "share/pycircuit/sdk-manifest.json":
        _fail("ACSDK-PLAN-MANIFEST-001", "SDK manifest escapes its prefix")
    raw = manifest_path.read_bytes()
    try:
        manifest = _closed_dict(json.loads(raw), _MANIFEST_KEYS, "document")
    except (UnicodeError, json.JSONDecodeError) as error:
        _fail("ACSDK-PLAN-MANIFEST-001", f"SDK manifest is invalid JSON: {error}")

    capabilities = manifest["capabilities"]
    if (
        manifest["schema"] != "pycircuit-sdk-platform-manifest"
        or manifest["version"] != "1"
        or manifest["contract_epoch"] != "0.5"
        or manifest["product_version"] != _PRODUCT_VERSION
        or manifest["self_path"] != "share/pycircuit/sdk-manifest.json"
        or manifest["distributions"] != _DISTRIBUTIONS
        or manifest["abi"] != _ABI
        or type(capabilities) is not list
        or tuple(capabilities) != _SDK_CAPABILITIES
    ):
        _fail("ACSDK-PLAN-MANIFEST-001", "SDK identity tuple is incompatible")
    source_revision = manifest["source_revision"]
    if type(source_revision) is not str or not _REVISION.fullmatch(source_revision):
        _fail("ACSDK-PLAN-MANIFEST-001", "SDK source revision is invalid")
    _validate_platform(manifest["platform"])
    _validate_runtime_dependencies(manifest["runtime_dependencies"])

    files = manifest["files"]
    if type(files) is not list:
        _fail("ACSDK-PLAN-MANIFEST-001", "SDK manifest files must be an array")
    entries: dict[str, dict[str, object]] = {}
    for value in files:
        entry = _closed_dict(value, _FILE_KEYS, "file entry")
        relative = entry["path"]
        if (
            type(relative) is not str
            or not _LOGICAL_PATH.fullmatch(relative)
            or relative in entries
            or entry["kind"] not in _FILE_KINDS
            or type(entry["size"]) is not int
            or entry["size"] < 0
            or type(entry["sha256"]) is not str
            or not _SHA256.fullmatch(entry["sha256"])
        ):
            _fail("ACSDK-PLAN-MANIFEST-001", "SDK manifest file entry is invalid")
        entries[relative] = entry
    if tuple(entries) != tuple(sorted(entries)):
        _fail("ACSDK-PLAN-MANIFEST-001", "SDK manifest files are not sorted")

    launcher = _manifest_file(root, entries, "bin/agentic-circuit", kind="tool")
    try:
        running_launcher = Path(sys.argv[0]).resolve(strict=True)
    except OSError as error:
        _fail(
            "ACSDK-PLAN-ROOT-002",
            f"running Agentic Circuit launcher is unknown: {error}",
        )
    if launcher != running_launcher:
        _fail("ACSDK-PLAN-ROOT-002", "--sdk-root does not own the running CLI")
    queue_plan = _manifest_file(root, entries, "bin/acir-queue-plan", kind="tool")
    model_plan_schema = _manifest_file(
        root,
        entries,
        "share/pycircuit/schemas/model-plan.schema.json",
        kind="schema",
    )
    try:
        schema = json.loads(model_plan_schema.read_bytes())
    except (UnicodeError, json.JSONDecodeError) as error:
        _fail("ACSDK-PLAN-MANIFEST-002", f"model plan schema is invalid: {error}")
    if (
        type(schema) is not dict
        or schema.get("$id")
        != "https://pto-isa.org/schemas/agentic-circuit/model-plan-v1.schema.json"
    ):
        _fail("ACSDK-PLAN-MANIFEST-002", "model plan schema identity is invalid")
    package_root = Path(__file__).resolve().parents[1]
    try:
        package_root.relative_to(root)
    except ValueError:
        _fail("ACSDK-PLAN-ROOT-002", "--sdk-root does not own the Python package")
    native_candidates = tuple(
        path
        for suffix in EXTENSION_SUFFIXES
        if (path := package_root / ("_native" + suffix)).is_file()
    )
    if len(native_candidates) != 1:
        _fail("ACSDK-PLAN-MANIFEST-002", "SDK native compiler is missing or ambiguous")
    native = native_candidates[0].resolve()
    native_relative = native.relative_to(root).as_posix()
    _manifest_file(root, entries, native_relative, kind="library")
    native_entry = entries[native_relative]
    return SdkIdentity(
        root,
        sha256_bytes(raw),
        source_revision,
        queue_plan,
        native,
        str(native_entry["sha256"]),
        int(native_entry["size"]),
    )


def _entry_path(source_root: Path, entry: str) -> tuple[str, str, Path]:
    match = _ENTRY.fullmatch(entry)
    if match is None:
        _fail("ACSDK-PLAN-ENTRY-001", "--entry must use importable-module:top syntax")
    module = match.group("module")
    symbol = match.group("symbol")
    parts = module.split(".")
    candidates = (
        source_root.joinpath(*parts).with_suffix(".py"),
        source_root.joinpath(*parts, "__init__.py"),
    )
    existing = tuple(path for path in candidates if path.is_file())
    if len(existing) != 1:
        _fail(
            "ACSDK-PLAN-ENTRY-001", f"entry module {module!r} is missing or ambiguous"
        )
    path = _regular_file(existing[0], code="ACSDK-PLAN-ENTRY-001", label="entry module")
    if not path.is_relative_to(source_root):
        _fail("ACSDK-PLAN-ENTRY-001", "entry module escapes --source-root")
    return module, symbol, path


def _config(
    path_value: object, source_root: Path
) -> tuple[Path, str, dict[str, JsonValue], str]:
    candidate = Path(path_value)
    if not candidate.is_absolute():
        candidate = source_root / candidate
    path = _regular_file(candidate, code="ACSDK-PLAN-CONFIG-001", label="model config")
    if not path.is_relative_to(source_root):
        _fail("ACSDK-PLAN-CONFIG-001", "model config escapes --source-root")
    try:
        raw = path.read_bytes()
        document = tomllib.loads(raw.decode("utf-8"))
    except (OSError, UnicodeError, tomllib.TOMLDecodeError) as error:
        _fail("ACSDK-PLAN-CONFIG-001", f"model config is invalid TOML: {error}")
    if set(document) != {"version", "static"} or document["version"] != "1":
        _fail(
            "ACSDK-PLAN-CONFIG-001", "model config must contain version 1 and [static]"
        )
    static = document["static"]
    if type(static) is not dict:
        _fail("ACSDK-PLAN-CONFIG-001", "model config [static] must be a table")
    try:
        validate_ijson_value(static)
    except ValueError as error:
        _fail("ACSDK-PLAN-CONFIG-001", f"model static config is invalid: {error}")
    return path, path.relative_to(source_root).as_posix(), static, sha256_bytes(raw)


def _decorator_name(node: ast.expr) -> str:
    candidate = node.func if isinstance(node, ast.Call) else node
    if isinstance(candidate, ast.Name):
        return candidate.id
    if isinstance(candidate, ast.Attribute):
        return candidate.attr
    return ""


def _source_role(entry: SourceClosureEntry, entry_path: str) -> str:
    if entry.path == entry_path:
        return "entry"
    try:
        tree = ast.parse(Path(entry.source_file).read_text(encoding="utf-8"))
    except (OSError, UnicodeError, SyntaxError) as error:
        _fail("ACSDK-PLAN-SOURCE-001", f"cannot classify source {entry.path}: {error}")
    for node in tree.body:
        decorators = getattr(node, "decorator_list", ())
        if any(_decorator_name(item) in _CONTRACT_DECORATORS for item in decorators):
            return "contract"
    return "import"


def _capture_model_acir(
    entry: Path,
    module_name: str,
    system: str,
    source_root: Path,
    static: dict[str, JsonValue],
    closure: SourceClosure,
) -> bytes:
    with tempfile.TemporaryDirectory(prefix="agentic-model-capture-") as temporary:
        result = run_capture_worker(
            CaptureWorkerRequest(
                python=sys.executable,
                workspace=source_root,
                entry=entry,
                system=system,
                static_arguments=tuple(sorted(static.items())),
                component_roots=(),
                private_output=Path(temporary) / "capture",
                jit_source_closure=True,
                module_name=module_name,
            )
        )
    errors = tuple(item for item in result.diagnostics if item.severity == "error")
    if errors:
        _fail(errors[0].code, errors[0].message)
    if result.frontend_kind != "queue_rule" or result.acir is None:
        _fail("ACSDK-PLAN-FRONTEND-001", "model frontend produced no QueueGraph ACIR")
    try:
        current = capture_source_closure(entry, source_root)
    except (OSError, SyntaxError, ValueError) as error:
        _fail("ACSDK-PLAN-SOURCE-002", f"model source closure changed: {error}")
    if current != closure:
        _fail("ACSDK-PLAN-SOURCE-002", "model source closure changed during capture")
    return result.acir


def _freeze(acir: bytes, sdk: SdkIdentity) -> bytes:
    try:
        native_bytes = sdk.native_extension.read_bytes()
    except OSError as error:
        _fail("ACSDK-PLAN-MANIFEST-002", f"SDK native compiler is unavailable: {error}")
    if (
        len(native_bytes) != sdk.native_size
        or sha256_bytes(native_bytes) != sdk.native_sha256
    ):
        _fail("ACSDK-PLAN-MANIFEST-003", "SDK native compiler identity mismatch")
    result = run_native_compiler(
        NativeRequest(
            acir=acir,
            stop_after="acir-freeze",
            emits=("frozen-acir",),
            options=(("binding_registry", b'{"candidates":[],"requests":[]}'),),
        )
    )
    if native_extension_path() != sdk.native_extension:
        _fail(
            "ACSDK-PLAN-ROOT-002",
            "loaded native compiler does not belong to --sdk-root",
        )
    errors = tuple(item for item in result.diagnostics if item.severity == "error")
    if errors:
        _fail(errors[0].code, errors[0].message)
    artifacts = tuple(
        item.data for item in result.artifacts if item.path == "frozen.ac.mlir"
    )
    if len(artifacts) != 1:
        _fail("ACSDK-PLAN-FREEZE-001", "compiler produced no canonical Frozen ACIR")
    return artifacts[0]


def _queuegraph(tool: Path, frozen: bytes) -> bytes:
    with tempfile.TemporaryDirectory(prefix="agentic-model-plan-") as temporary:
        path = Path(temporary) / "frozen.ac.mlir"
        path.write_bytes(frozen)
        environment = os.environ.copy()
        environment.pop("PYTHONPATH", None)
        try:
            completed = subprocess.run(
                [os.fspath(tool), os.fspath(path)],
                env=environment,
                text=False,
                capture_output=True,
                check=False,
                timeout=60,
            )
        except (OSError, subprocess.TimeoutExpired) as error:
            _fail("ACSDK-PLAN-QUEUEGRAPH-001", f"QueueGraph planner failed: {error}")
    if completed.returncode != 0:
        detail = completed.stderr.decode("utf-8", errors="replace").strip()
        _fail("ACSDK-PLAN-QUEUEGRAPH-001", f"QueueGraph planner failed: {detail}")
    try:
        document = json.loads(completed.stdout)
        if (
            type(document) is not dict
            or document.get("schema") != "agentic-circuit-queue-graph-plan"
            or document.get("version") != "0.5"
        ):
            raise ValueError("unexpected QueueGraph identity")
        return canonical_json_bytes(document) + b"\n"
    except (UnicodeError, json.JSONDecodeError, ValueError) as error:
        _fail("ACSDK-PLAN-QUEUEGRAPH-001", f"QueueGraph output is invalid: {error}")


def _cmake_sources() -> bytes:
    return (
        b"set(AGENTIC_MODEL_GENERATED_SOURCES\n"
        b'  "src/generated/model.cpp"\n'
        b'  "src/generated/queuegraph.cpp"\n'
        b")\n"
        b"set(AGENTIC_MODEL_GENERATED_HEADERS\n"
        b'  "include/generated/model.h"\n'
        b")\n"
        b'set(AGENTIC_MODEL_QUERY_SYMBOL "agentic_model_query_v1")\n'
        b'set(AGENTIC_MODEL_RUNTIME_TARGET "AgenticCircuit::Gfsim")\n'
    )


def _depfile(output: Path, inputs: tuple[Path, ...]) -> bytes:
    def escape(path: Path) -> str:
        return (
            path.as_posix().replace("$", "$$").replace("#", "\\#").replace(" ", "\\ ")
        )

    dependencies = " ".join(escape(path) for path in inputs)
    return f"{escape(output / 'model-plan.json')}: {dependencies}\n".encode()


def _publish(output: Path, artifacts: dict[str, bytes]) -> tuple[str, ...]:
    expected = tuple(sorted(artifacts))
    output.parent.mkdir(parents=True, exist_ok=True)
    lock_path = output.parent / f".{output.name}.lock"
    with lock_path.open("a+b") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if output.is_symlink() or (output.exists() and not output.is_dir()):
            _fail(
                "ACSDK-PLAN-OUTPUT-001",
                "model plan output must be a regular directory",
            )
        entries = tuple(output.rglob("*")) if output.exists() else ()
        if any(item.is_symlink() for item in entries):
            _fail("ACSDK-PLAN-OUTPUT-001", "model plan output contains a symlink")
        stale = tuple(
            item.relative_to(output).as_posix()
            for item in entries
            if item.is_file() and item.relative_to(output).as_posix() not in artifacts
        )
        if stale:
            _fail(
                "ACSDK-PLAN-OUTPUT-001",
                f"model plan output contains stale file: {stale[0]}",
            )
        with ArtifactStage(output, expected=expected) as stage:
            for path in expected:
                stage.write_bytes(path, artifacts[path])
            stage.commit_directory()
    return expected


def _plan(arguments: object, sink: OutputSink) -> int:
    sdk = _sdk_identity(arguments.sdk_root)
    try:
        source_root = Path(arguments.source_root).expanduser().resolve(strict=True)
    except (OSError, TypeError) as error:
        _fail("ACSDK-PLAN-SOURCE-001", f"source root is unavailable: {error}")
    if not source_root.is_dir():
        _fail("ACSDK-PLAN-SOURCE-001", "source root is not a directory")
    entry_text = arguments.entry
    if type(entry_text) is not str:
        _fail("ACSDK-PLAN-ENTRY-001", "model entry is invalid")
    module_name, symbol, entry_path = _entry_path(source_root, entry_text)
    config_path, config_logical, static, config_hash = _config(
        arguments.config, source_root
    )
    try:
        closure = capture_source_closure(entry_path, source_root)
    except (OSError, SyntaxError, ValueError) as error:
        _fail("ACSDK-PLAN-SOURCE-001", f"model source closure is invalid: {error}")
    raw_acir = _capture_model_acir(
        entry_path, module_name, symbol, source_root, static, closure
    )
    if sha256_bytes(config_path.read_bytes()) != config_hash:
        _fail("ACSDK-PLAN-CONFIG-002", "model config changed during capture")
    frozen = _freeze(raw_acir, sdk)
    queuegraph = _queuegraph(sdk.queue_plan_tool, frozen)
    queuegraph_document = json.loads(queuegraph)
    specialization = queuegraph_document.get("specialization")
    if type(specialization) is not str or not _SHA256.fullmatch(specialization):
        _fail("ACSDK-PLAN-QUEUEGRAPH-002", "QueueGraph specialization is invalid")

    cmake = _cmake_sources()
    output_candidate = Path(arguments.out_dir).expanduser()
    if output_candidate.is_symlink():
        _fail("ACSDK-PLAN-OUTPUT-001", "model plan output must not be a symlink")
    output = output_candidate.resolve()
    entry_logical = entry_path.relative_to(source_root).as_posix()
    inputs: list[dict[str, JsonValue]] = [
        {
            "path": item.path,
            "sha256": item.sha256,
            "role": _source_role(item, entry_logical),
        }
        for item in closure.entries
    ]
    inputs.append({"path": config_logical, "sha256": config_hash, "role": "config"})
    inputs.sort(key=lambda item: str(item["path"]))
    plan: dict[str, JsonValue] = {
        "schema": "agentic-circuit-model-plan",
        "version": "1",
        "contract_epoch": "0.5",
        "sdk": {
            "product_version": _PRODUCT_VERSION,
            "source_revision": sdk.source_revision,
            "platform_manifest_sha256": sdk.manifest_sha256,
            "model_plan_abi": "1",
            "generator_abi": "1",
            "runtime_abi": "1",
        },
        "entry": entry_text,
        "specialization": specialization,
        "config": {"path": config_logical, "sha256": config_hash},
        "static_config": static,
        "inputs": inputs,
        "frozen_acir": {"path": "frozen.ac.mlir", "sha256": sha256_bytes(frozen)},
        "queuegraph": {"path": "queuegraph.json", "sha256": sha256_bytes(queuegraph)},
        "outputs": list(_PLAN_OUTPUTS),
        "cmake_sources": {
            "path": "model-sources.cmake",
            "sha256": sha256_bytes(cmake),
        },
        "depfile_path": "model.d",
        "required_runtime": "AgenticCircuit::Gfsim",
        "capabilities": list(_PLAN_CAPABILITIES),
    }
    plan_bytes = canonical_json_bytes(plan) + b"\n"
    dependency_paths = tuple(
        Path(item.source_file).resolve() for item in closure.entries
    ) + (config_path,)
    artifacts = {
        "frozen.ac.mlir": frozen,
        "model-sources.cmake": cmake,
        "model.d": _depfile(output, dependency_paths),
        "model-plan.json": plan_bytes,
        "queuegraph.json": queuegraph,
    }
    published = _publish(output, artifacts)
    sink.result(
        {
            "schema": "agentic-circuit-model-plan-result",
            "version": "1",
            "contract_epoch": "0.5",
            "status": "passed",
            "entry": entry_text,
            "specialization": specialization,
            "output_dir": output.as_posix(),
            "artifacts": list(published),
            "model_plan_sha256": sha256_bytes(plan_bytes),
        },
        human=f"planned model {entry_text} to {output}",
    )
    return 0


def run(arguments: object, sink: OutputSink) -> int:
    if getattr(arguments, "model_command", None) == "plan":
        return _plan(arguments, sink)
    _fail("ACSDK-PLAN-CLI-001", "unknown model command")

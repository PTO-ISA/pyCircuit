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
from .._contract import CONTRACT_EPOCH
from .._exit_codes import ExitCode
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
    "acpy_epoch": CONTRACT_EPOCH,
    "acir_contract": CONTRACT_EPOCH,
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
_PLAN_KEYS = frozenset(
    {
        "schema",
        "version",
        "contract_epoch",
        "sdk",
        "entry",
        "specialization",
        "config",
        "static_config",
        "inputs",
        "frozen_acir",
        "queuegraph",
        "outputs",
        "cmake_sources",
        "depfile_path",
        "required_runtime",
        "capabilities",
    }
)
_PLAN_SDK_KEYS = frozenset(
    {
        "product_version",
        "source_revision",
        "platform_manifest_sha256",
        "model_plan_abi",
        "generator_abi",
        "runtime_abi",
    }
)
_HASHED_PATH_KEYS = frozenset({"path", "sha256"})
_PLAN_INPUT_KEYS = frozenset({"path", "sha256", "role"})
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
    queue_cxxgen_tool: Path
    queue_cxxgen_sha256: str
    queue_cxxgen_size: int
    native_extension: Path
    native_sha256: str
    native_size: int


def _fail(code: str, message: str) -> NoReturn:
    raise UserInputError(
        Diagnostic(
            stage=(
                "model-emit-cpp" if code.startswith("ACSDK-EMIT-") else "model-plan"
            ),
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
        or manifest["contract_epoch"] != CONTRACT_EPOCH
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
    queue_cxxgen = _manifest_file(root, entries, "bin/acir-queue-cxxgen", kind="tool")
    queue_cxxgen_entry = entries["bin/acir-queue-cxxgen"]
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
    model_manifest_schema = _manifest_file(
        root,
        entries,
        "share/pycircuit/schemas/model-manifest.schema.json",
        kind="schema",
    )
    try:
        manifest_schema = json.loads(model_manifest_schema.read_bytes())
    except (UnicodeError, json.JSONDecodeError) as error:
        _fail("ACSDK-PLAN-MANIFEST-002", f"model manifest schema is invalid: {error}")
    if (
        type(manifest_schema) is not dict
        or manifest_schema.get("$id")
        != "https://pto-isa.org/schemas/agentic-circuit/model-manifest-v1.schema.json"
    ):
        _fail("ACSDK-PLAN-MANIFEST-002", "model manifest schema identity is invalid")
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
        queue_cxxgen,
        str(queue_cxxgen_entry["sha256"]),
        int(queue_cxxgen_entry["size"]),
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
        "contract_epoch": CONTRACT_EPOCH,
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
            "contract_epoch": CONTRACT_EPOCH,
            "status": "passed",
            "entry": entry_text,
            "specialization": specialization,
            "output_dir": output.as_posix(),
            "artifacts": list(published),
            "model_plan_sha256": sha256_bytes(plan_bytes),
        },
        human=f"planned model {entry_text} to {output}",
    )
    return ExitCode.SUCCESS


@dataclass(frozen=True, slots=True)
class VerifiedPlan:
    path: Path
    raw: bytes
    document: dict[str, object]
    frozen_path: Path
    frozen: bytes
    queuegraph_path: Path
    queuegraph: bytes
    cmake_sources_path: Path
    cmake_sources: bytes


def _emit_fail(suffix: str, message: str) -> NoReturn:
    _fail(f"ACSDK-EMIT-{suffix}", message)


def _emit_closed(value: object, keys: frozenset[str], label: str) -> dict[str, object]:
    if type(value) is not dict or set(value) != keys:
        _emit_fail("PLAN-001", f"{label} is not a closed record")
    return value


def _emit_logical_path(value: object, label: str) -> str:
    if type(value) is not str or not _LOGICAL_PATH.fullmatch(value):
        _emit_fail("PLAN-001", f"{label} is not a canonical logical path")
    return value


def _plan_artifact(plan_root: Path, value: object, label: str) -> tuple[Path, bytes]:
    record = _emit_closed(value, _HASHED_PATH_KEYS, label)
    logical = _emit_logical_path(record["path"], f"{label}.path")
    expected = record["sha256"]
    if type(expected) is not str or not _SHA256.fullmatch(expected):
        _emit_fail("PLAN-001", f"{label}.sha256 is invalid")
    path = _regular_file(
        plan_root.joinpath(*PurePosixPath(logical).parts),
        code="ACSDK-EMIT-PLAN-001",
        label=label,
    )
    if not path.is_relative_to(plan_root):
        _emit_fail("PLAN-001", f"{label} escapes the plan directory")
    raw = path.read_bytes()
    if sha256_bytes(raw) != expected:
        _emit_fail("HASH-001", f"{label} hash does not match the plan")
    return path, raw


def _verify_plan(value: object, sdk: SdkIdentity) -> VerifiedPlan:
    path = _regular_file(Path(value), code="ACSDK-EMIT-PLAN-001", label="model plan")
    if path.name != "model-plan.json":
        _emit_fail("PLAN-001", "model plan must be named model-plan.json")
    raw = path.read_bytes()
    try:
        document = _emit_closed(json.loads(raw), _PLAN_KEYS, "model plan")
    except (UnicodeError, json.JSONDecodeError) as error:
        _emit_fail("PLAN-001", f"model plan is invalid JSON: {error}")
    try:
        canonical = canonical_json_bytes(document) + b"\n"
    except (TypeError, ValueError) as error:
        _emit_fail("PLAN-001", f"model plan is not canonical I-JSON: {error}")
    if raw != canonical:
        _emit_fail("PLAN-001", "model plan bytes are not canonical")
    if (
        document["schema"] != "agentic-circuit-model-plan"
        or document["version"] != "1"
        or document["contract_epoch"] != CONTRACT_EPOCH
        or document["required_runtime"] != "AgenticCircuit::Gfsim"
        or document["depfile_path"] != "model.d"
        or document["outputs"] != list(_PLAN_OUTPUTS)
        or document["capabilities"] != list(_PLAN_CAPABILITIES)
    ):
        _emit_fail("PLAN-001", "model plan identity or fixed outputs are invalid")
    entry = document["entry"]
    specialization = document["specialization"]
    if type(entry) is not str or _ENTRY.fullmatch(entry) is None:
        _emit_fail("PLAN-001", "model plan entry is invalid")
    if type(specialization) is not str or not _SHA256.fullmatch(specialization):
        _emit_fail("PLAN-001", "model plan specialization is invalid")

    plan_sdk = _emit_closed(document["sdk"], _PLAN_SDK_KEYS, "model plan SDK")
    expected_sdk = {
        "product_version": _PRODUCT_VERSION,
        "source_revision": sdk.source_revision,
        "platform_manifest_sha256": sdk.manifest_sha256,
        "model_plan_abi": "1",
        "generator_abi": "1",
        "runtime_abi": "1",
    }
    if plan_sdk != expected_sdk:
        _emit_fail("SDK-001", "model plan SDK identity does not match --sdk-root")

    config = _emit_closed(document["config"], _HASHED_PATH_KEYS, "model config")
    config_path = _emit_logical_path(config["path"], "model config path")
    config_hash = config["sha256"]
    if type(config_hash) is not str or not _SHA256.fullmatch(config_hash):
        _emit_fail("PLAN-001", "model config hash is invalid")
    static_config = document["static_config"]
    if type(static_config) is not dict:
        _emit_fail("PLAN-001", "model static config is invalid")
    try:
        validate_ijson_value(static_config)
    except ValueError as error:
        _emit_fail("PLAN-001", f"model static config is invalid: {error}")

    inputs = document["inputs"]
    if type(inputs) is not list or not inputs:
        _emit_fail("PLAN-001", "model plan inputs are invalid")
    input_paths: list[str] = []
    config_inputs = 0
    entry_inputs = 0
    entry_path = ""
    source_manifest: list[dict[str, JsonValue]] = []
    for value in inputs:
        item = _emit_closed(value, _PLAN_INPUT_KEYS, "model plan input")
        logical = _emit_logical_path(item["path"], "model plan input path")
        digest = item["sha256"]
        role = item["role"]
        if type(digest) is not str or not _SHA256.fullmatch(digest):
            _emit_fail("PLAN-001", "model plan input hash is invalid")
        if role not in {"entry", "import", "contract", "config"}:
            _emit_fail("PLAN-001", "model plan input role is invalid")
        input_paths.append(logical)
        config_inputs += int(role == "config" and logical == config_path)
        if role == "entry":
            entry_inputs += 1
            entry_path = logical
        if role != "config":
            source_manifest.append({"path": logical, "sha256": digest})
    if (
        input_paths != sorted(input_paths)
        or len(input_paths) != len(set(input_paths))
        or config_inputs != 1
        or entry_inputs != 1
    ):
        _emit_fail("PLAN-001", "model plan input closure is not canonical")
    config_input = next(item for item in inputs if item["role"] == "config")
    if config_input["sha256"] != config_hash:
        _emit_fail("PLAN-001", "model config hash disagrees with the input closure")
    entry_match = _ENTRY.fullmatch(entry)
    assert entry_match is not None
    module_path = entry_match.group("module").replace(".", "/")
    if entry_path not in {module_path + ".py", module_path + "/__init__.py"}:
        _emit_fail("PLAN-001", "model entry path disagrees with entry identity")
    source_closure_sha256 = sha256_bytes(canonical_json_bytes(source_manifest))
    specialization_preimage: dict[str, JsonValue] = {
        "schema": "agentic-circuit-jit-specialization",
        "version": "0.5",
        "system": PurePosixPath(entry_path).with_suffix("").as_posix()
        + "::"
        + entry_match.group("symbol"),
        "source_sha256": None,
        "source_closure_sha256": source_closure_sha256,
        "source_manifest": source_manifest,
        "arguments": static_config,
    }
    if sha256_bytes(canonical_json_bytes(specialization_preimage)) != specialization:
        _emit_fail("PLAN-001", "model specialization provenance is inconsistent")

    plan_root = path.parent.resolve()
    frozen_path, frozen = _plan_artifact(
        plan_root, document["frozen_acir"], "Frozen ACIR"
    )
    queuegraph_path, queuegraph = _plan_artifact(
        plan_root, document["queuegraph"], "QueueGraph"
    )
    cmake_path, cmake = _plan_artifact(
        plan_root, document["cmake_sources"], "CMake source fragment"
    )
    if cmake != _cmake_sources():
        _emit_fail("PLAN-001", "CMake source fragment is not the fixed v1 contract")
    rebuilt_queuegraph = _queuegraph(sdk.queue_plan_tool, frozen)
    if rebuilt_queuegraph != queuegraph:
        _emit_fail("HASH-001", "QueueGraph does not match Frozen ACIR")
    try:
        queuegraph_document = json.loads(queuegraph)
    except (UnicodeError, json.JSONDecodeError) as error:
        _emit_fail("PLAN-001", f"QueueGraph is invalid JSON: {error}")
    if queuegraph_document.get("specialization") != specialization:
        _emit_fail("PLAN-001", "QueueGraph specialization does not match the plan")
    return VerifiedPlan(
        path,
        raw,
        document,
        frozen_path,
        frozen,
        queuegraph_path,
        queuegraph,
        cmake_path,
        cmake,
    )


def _generate_model_sources(sdk: SdkIdentity, plan: VerifiedPlan) -> dict[str, bytes]:
    with tempfile.TemporaryDirectory(prefix="agentic-model-emit-") as temporary:
        temporary_root = Path(temporary)
        output = temporary_root / "bundle"
        frozen = temporary_root / "verified.frozen.ac.mlir"
        frozen.write_bytes(plan.frozen)
        try:
            generator_bytes = sdk.queue_cxxgen_tool.read_bytes()
        except OSError as error:
            _emit_fail("GENERATOR-001", f"model generator is unavailable: {error}")
        if (
            len(generator_bytes) != sdk.queue_cxxgen_size
            or sha256_bytes(generator_bytes) != sdk.queue_cxxgen_sha256
        ):
            _emit_fail(
                "GENERATOR-001", "model generator identity changed after preflight"
            )
        try:
            completed = subprocess.run(
                [
                    os.fspath(sdk.queue_cxxgen_tool),
                    os.fspath(frozen),
                    "--output-root",
                    os.fspath(output),
                    "--sdk-product-version",
                    _PRODUCT_VERSION,
                    "--sdk-source-revision",
                    sdk.source_revision,
                ],
                env={"PATH": os.environ.get("PATH", "")},
                text=True,
                capture_output=True,
                check=False,
                timeout=60,
            )
        except (OSError, subprocess.TimeoutExpired) as error:
            _emit_fail("GENERATOR-001", f"model generator failed: {error}")
        if completed.returncode != 0:
            detail, _ = OutputSink.bounded_capture(completed.stderr or completed.stdout)
            _emit_fail("GENERATOR-001", f"model generator failed: {detail}")
        if not output.is_dir():
            _emit_fail("GENERATOR-001", "model generator produced no bundle")
        found = tuple(
            item.relative_to(output).as_posix()
            for item in sorted(output.rglob("*"))
            if item.is_file()
        )
        if found != _PLAN_OUTPUTS:
            _emit_fail(
                "GENERATOR-001", "model generator produced an unexpected file set"
            )
        artifacts = {path: (output / path).read_bytes() for path in found}
    forbidden = (os.fspath(sdk.root).encode(), os.fspath(plan.path.parent).encode())
    for path, raw in artifacts.items():
        if any(value in raw for value in forbidden):
            _emit_fail(
                "GENERATOR-001", f"generated source leaks a producer path: {path}"
            )
    return artifacts


def _emit_depfile(output: Path, plan: VerifiedPlan) -> bytes:
    def escape(path: Path) -> str:
        return (
            path.as_posix().replace("$", "$$").replace("#", "\\#").replace(" ", "\\ ")
        )

    targets = tuple(
        output / path
        for path in (*_PLAN_OUTPUTS, "model-manifest.json", "model-sources.cmake")
    )
    dependencies = (
        plan.path,
        plan.frozen_path,
        plan.queuegraph_path,
        plan.cmake_sources_path,
    )
    return (
        " ".join(escape(path) for path in targets)
        + ": "
        + " ".join(escape(path) for path in dependencies)
        + "\n"
    ).encode()


def _model_manifest(
    sdk: SdkIdentity, plan: VerifiedPlan, generated: dict[str, bytes]
) -> bytes:
    document = plan.document
    manifest: dict[str, JsonValue] = {
        "schema": "agentic-circuit-model-manifest",
        "version": "1",
        "contract_epoch": CONTRACT_EPOCH,
        "plan": {"path": "model-plan.json", "sha256": sha256_bytes(plan.raw)},
        "sdk": {
            "product_version": _PRODUCT_VERSION,
            "source_revision": sdk.source_revision,
            "platform_manifest_sha256": sdk.manifest_sha256,
        },
        "entry": document["entry"],
        "specialization": document["specialization"],
        "generated_files": [
            {"path": path, "sha256": sha256_bytes(generated[path])}
            for path in _PLAN_OUTPUTS
        ],
        "cmake_sources": {
            "path": "model-sources.cmake",
            "sha256": sha256_bytes(plan.cmake_sources),
        },
        "depfile_path": "model.d",
        "required_runtime": "AgenticCircuit::Gfsim",
        "runtime_abi": "1",
        "exports": {
            "header": "include/gfsim/model_api.h",
            "query_symbol": "agentic_model_query_v1",
            "abi_version": "1",
            "api_struct_size": 80,
            "buffer_struct_size": 16,
            "step_result_struct_size": 24,
        },
    }
    return canonical_json_bytes(manifest) + b"\n"


def _existing_model_files(output: Path) -> frozenset[str]:
    if not output.exists():
        return frozenset()
    if output.is_symlink() or not output.is_dir():
        _emit_fail("OUTPUT-001", "model output must be a regular directory")
    entries = tuple(output.rglob("*"))
    if any(item.is_symlink() for item in entries):
        _emit_fail("OUTPUT-001", "model output contains a symlink")
    files = frozenset(
        item.relative_to(output).as_posix() for item in entries if item.is_file()
    )
    if not files:
        return files
    manifest_path = output / "model-manifest.json"
    if not manifest_path.is_file():
        _emit_fail("OUTPUT-001", "existing model output has no ownership manifest")
    try:
        manifest_raw = manifest_path.read_bytes()
        manifest = json.loads(manifest_raw)
        if canonical_json_bytes(manifest) + b"\n" != manifest_raw:
            _emit_fail("OUTPUT-001", "existing ownership manifest is not canonical")
        if (
            manifest.get("schema") != "agentic-circuit-model-manifest"
            or manifest.get("version") != "1"
            or manifest.get("contract_epoch") != CONTRACT_EPOCH
            or manifest.get("required_runtime") != "AgenticCircuit::Gfsim"
            or manifest.get("runtime_abi") != "1"
        ):
            _emit_fail("OUTPUT-001", "existing ownership manifest identity is invalid")
        generated = manifest["generated_files"]
        if type(generated) is not list:
            _emit_fail("OUTPUT-001", "existing generated file list is invalid")
        cmake_record = _emit_closed(
            manifest["cmake_sources"], _HASHED_PATH_KEYS, "old CMake source"
        )
        cmake_path = cmake_record["path"]
        depfile_path = manifest["depfile_path"]
        generated_paths: list[str] = []
        for value in generated:
            record = _emit_closed(value, _HASHED_PATH_KEYS, "old generated file")
            logical = _emit_logical_path(record["path"], "old generated file path")
            digest = record["sha256"]
            if type(digest) is not str or not _SHA256.fullmatch(digest):
                _emit_fail("OUTPUT-001", "old generated file hash is invalid")
            generated_path = output.joinpath(*PurePosixPath(logical).parts)
            if (
                not generated_path.is_file()
                or sha256_bytes(generated_path.read_bytes()) != digest
            ):
                _emit_fail("OUTPUT-001", "old generated file identity is invalid")
            generated_paths.append(logical)
        cmake_logical = _emit_logical_path(cmake_path, "old CMake source path")
        cmake_digest = cmake_record["sha256"]
        cmake_file = output.joinpath(*PurePosixPath(cmake_logical).parts)
        if (
            type(cmake_digest) is not str
            or not _SHA256.fullmatch(cmake_digest)
            or not cmake_file.is_file()
            or sha256_bytes(cmake_file.read_bytes()) != cmake_digest
        ):
            _emit_fail("OUTPUT-001", "old CMake source identity is invalid")
        owned = {
            "model-manifest.json",
            cmake_logical,
            _emit_logical_path(depfile_path, "old depfile path"),
            *generated_paths,
        }
    except (KeyError, OSError, TypeError, UnicodeError, json.JSONDecodeError) as error:
        _emit_fail("OUTPUT-001", f"existing ownership manifest is invalid: {error}")
    if files != owned:
        _emit_fail("OUTPUT-001", "existing model output contains an unowned file")
    return files


def _publish_model(output: Path, artifacts: dict[str, bytes]) -> tuple[str, ...]:
    expected = tuple(sorted(artifacts))
    output.parent.mkdir(parents=True, exist_ok=True)
    lock_path = output.parent / f".{output.name}.lock"
    with lock_path.open("a+b") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        _existing_model_files(output)
        with ArtifactStage(output, expected=expected) as stage:
            for path in expected:
                stage.write_bytes(path, artifacts[path])
            stage.commit_directory()
    return expected


def _emit(arguments: object, sink: OutputSink) -> int:
    sdk = _sdk_identity(arguments.sdk_root)
    plan = _verify_plan(arguments.plan, sdk)
    output_candidate = Path(arguments.out_dir).expanduser()
    if output_candidate.is_symlink():
        _emit_fail("OUTPUT-001", "model output must not be a symlink")
    output = output_candidate.resolve()
    manifest = Path(arguments.manifest).expanduser().resolve()
    depfile = Path(arguments.depfile).expanduser().resolve()
    if manifest != output / "model-manifest.json":
        _emit_fail("OUTPUT-001", "--manifest must name <out-dir>/model-manifest.json")
    if depfile != output / "model.d":
        _emit_fail("OUTPUT-001", "--depfile must name <out-dir>/model.d")

    generated = _generate_model_sources(sdk, plan)
    artifacts = {
        **generated,
        "model-sources.cmake": plan.cmake_sources,
        "model.d": _emit_depfile(output, plan),
        "model-manifest.json": _model_manifest(sdk, plan, generated),
    }
    published = _publish_model(output, artifacts)
    manifest_bytes = artifacts["model-manifest.json"]
    sink.result(
        {
            "schema": "agentic-circuit-model-emit-result",
            "version": "1",
            "contract_epoch": CONTRACT_EPOCH,
            "status": "passed",
            "entry": plan.document["entry"],
            "specialization": plan.document["specialization"],
            "output_dir": output.as_posix(),
            "artifacts": list(published),
            "model_manifest_sha256": sha256_bytes(manifest_bytes),
        },
        human=f"emitted model sources to {output}",
    )
    return ExitCode.SUCCESS


def run(arguments: object, sink: OutputSink) -> int:
    if getattr(arguments, "model_command", None) == "plan":
        return _plan(arguments, sink)
    if getattr(arguments, "model_command", None) == "emit-cpp":
        return _emit(arguments, sink)
    _emit_fail("CLI-001", "unknown model command")

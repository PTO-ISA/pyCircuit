"""Closed configuration records and deterministic JIT specialization metadata."""

from __future__ import annotations

import ast
import dataclasses
import enum
import inspect
import os
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from types import MappingProxyType
from typing import TYPE_CHECKING, ForwardRef, get_args, get_origin, get_type_hints

from ._canonical_json import canonical_json_bytes, sha256_bytes, validate_ijson_value
from ._definitions import Definition
from ._diagnostics import Diagnostic, DiagnosticRuntimeError, DiagnosticTypeError
from ._package_data import repository_root
from ._source_closure import SourceClosure, SourceClosureEntry, capture_source_closure
from ._static_eval import FrozenMap, StaticValue, static_json_value
from ._types import Static

if TYPE_CHECKING:
    from ._queue_frontend import QueueProgram


def _native_queue_tool(name: str, environment: str) -> Path:
    candidates: list[Path] = []
    configured = os.environ.get(environment)
    if configured:
        candidates.append(Path(configured))
    try:
        repository = repository_root()
    except FileNotFoundError:
        repository = None
    if repository is not None:
        candidates.append(repository / ".pycircuit_out/acir/dev-llvm22/bin" / name)
    candidates.append(Path(sys.prefix) / "bin" / name)
    try:
        from ._native_api import native_extension_path

        for root in native_extension_path().parents:
            candidates.append(root / "bin" / name)
    except (ImportError, RuntimeError):
        pass
    discovered = shutil.which(name)
    if discovered:
        candidates.append(Path(discovered))
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise DiagnosticRuntimeError(
        f"ACPY-JIT-004: native {name} is required for @ac.rule lowering"
    )


def _lower_queue_acir(acir: str, *, optimizer: str | Path | None = None) -> str:
    from ._queue_frontend import RULE_LOWERING_PIPELINE

    selected = (
        Path(optimizer)
        if optimizer is not None
        else _native_queue_tool("acir-opt", "ACIR_OPT")
    )
    if not selected.is_file():
        raise DiagnosticRuntimeError(
            f"ACPY-JIT-004: native acir-opt is unavailable: {selected}"
        )
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        raw = root / "rule.raw.ac.mlir"
        lowered = root / "rule.lowered.ac.mlir"
        raw.write_text(acir, encoding="utf-8")
        optimized = subprocess.run(
            (
                str(selected),
                f"--pass-pipeline={RULE_LOWERING_PIPELINE}",
                str(raw),
                "-o",
                str(lowered),
            ),
            text=True,
            capture_output=True,
            check=False,
        )
        if optimized.returncode != 0:
            raise DiagnosticRuntimeError(
                "ACPY-JIT-004: native rule lowering failed:\n" + optimized.stderr
            )
        return lowered.read_text(encoding="utf-8")


def _lower_acir_to_cpp(acir: str) -> str:
    generator = _native_queue_tool("acir-queue-cxxgen", "ACIR_QUEUE_CXXGEN")
    frozen_acir = _lower_queue_acir(acir)
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        lowered = root / "rule.lowered.ac.mlir"
        lowered.write_text(frozen_acir, encoding="utf-8")
        emitted = subprocess.run(
            (str(generator), str(lowered)),
            text=True,
            capture_output=True,
            check=False,
        )
        if emitted.returncode != 0:
            raise DiagnosticRuntimeError(
                "ACPY-JIT-004: native rule C++ generation failed:\n" + emitted.stderr
            )
        return emitted.stdout


def _lower_rule_program_to_cpp(program: QueueProgram) -> str:
    from ._queue_frontend import lower_queue_program

    return _lower_acir_to_cpp(lower_queue_program(program))


def config(cls: type[object]) -> type[object]:
    """Freeze one closed elaboration-time configuration record."""

    if not isinstance(cls, type):
        raise DiagnosticTypeError("ACPY-JIT-001: config must decorate a class")
    if dataclasses.is_dataclass(cls):
        raise DiagnosticTypeError(
            "ACPY-JIT-001: config class must not already be a dataclass"
        )
    frame = inspect.currentframe()
    caller = None if frame is None else frame.f_back
    module = sys.modules.get(cls.__module__)
    try:
        field_types = get_type_hints(
            cls,
            globalns={} if module is None else vars(module),
            localns={} if caller is None else dict(caller.f_locals),
        )
    except (NameError, TypeError):
        field_types = dict(getattr(cls, "__annotations__", {}))
    finally:
        del frame
        del caller
    frozen = dataclasses.dataclass(frozen=True, slots=True)(cls)
    frozen.__ac_config__ = True  # type: ignore[attr-defined]
    frozen.__ac_config_field_types__ = MappingProxyType(  # type: ignore[attr-defined]
        dict(field_types)
    )
    return frozen


def _is_const_annotation(annotation: object) -> bool:
    if get_origin(annotation) is Static:
        return True
    if isinstance(annotation, str):
        compact = annotation.replace(" ", "")
        return compact.startswith(("const[", "ac.const[", "Static[", "ac.Static["))
    return False


def _const_annotation_target(annotation: object) -> object | str | None:
    if get_origin(annotation) is Static:
        arguments = get_args(annotation)
        if len(arguments) != 1:
            return None
        target = arguments[0]
        return target.__forward_arg__ if isinstance(target, ForwardRef) else target
    if not isinstance(annotation, str):
        return None
    compact = annotation.replace(" ", "")
    prefixes = ("const[", "ac.const[", "Static[", "ac.Static[")
    prefix = next((item for item in prefixes if compact.startswith(item)), None)
    if prefix is None or not compact.endswith("]"):
        return None
    target = compact[len(prefix) : -1]
    try:
        literal = ast.literal_eval(target)
    except (SyntaxError, ValueError):
        return target
    return literal if isinstance(literal, str) and literal else target


def _validate_config_instance(
    value: object,
    expected: type[object],
    *,
    path: str,
    active: set[int] | None = None,
) -> None:
    if type(value) is not expected:
        raise DiagnosticTypeError(f"ACPY-JIT-002: {path} requires {expected.__name__}")
    active = set() if active is None else active
    identity = id(value)
    if identity in active:
        raise DiagnosticTypeError(
            f"ACPY-JIT-002: cyclic config value at {path} is unsupported"
        )
    active.add(identity)
    from ._types import _config_field_types

    for field_name, field_type in _config_field_types(expected).items():
        field_value = getattr(value, field_name)
        if isinstance(field_type, str):
            primitive = {
                "int": int,
                "bool": bool,
                "float": float,
                "str": str,
            }.get(field_type)
            if primitive is None:
                raise DiagnosticTypeError(
                    "ACPY-JIT-002: unresolved config field annotation "
                    f"{field_type!r} at {path}.{field_name}"
                )
            field_type = primitive
        if isinstance(field_type, type) and getattr(field_type, "__ac_config__", False):
            _validate_config_instance(
                field_value,
                field_type,
                path=f"{path}.{field_name}",
                active=active,
            )
            continue
        if (
            field_type in {int, bool, float, str}
            and type(field_value) is not field_type
        ):
            raise DiagnosticTypeError(
                f"ACPY-JIT-002: {path}.{field_name} requires {field_type.__name__}"
            )
    active.remove(identity)


def _validate_const_argument(
    parameter: inspect.Parameter,
    value: object,
    system: Definition,
) -> None:
    target = _const_annotation_target(parameter.annotation)
    if isinstance(target, type) and getattr(target, "__ac_config__", False):
        if type(value) is not target:
            raise DiagnosticTypeError(
                "ACPY-JIT-002: const argument "
                f"{parameter.name!r} requires config type {target.__name__!r}"
            )
        _validate_config_instance(value, target, path=target.__name__)
        return
    if not isinstance(target, str):
        return
    expected_name = target.rsplit(".", 1)[-1]
    annotation_types = dict(system.annotation_types)
    expected_type = annotation_types.get(target) or annotation_types.get(expected_name)
    if expected_type is None:
        expected_type = {
            "bool": bool,
            "float": float,
            "int": int,
            "object": object,
            "str": str,
        }.get(target)
    if expected_type is None:
        raise DiagnosticTypeError(
            "ACPY-JIT-002: const argument "
            f"{parameter.name!r} has unresolved annotation {target!r}"
        )
    if getattr(expected_type, "__ac_config__", False):
        expected_config = expected_type
        if type(value) is not expected_config:
            raise DiagnosticTypeError(
                "ACPY-JIT-002: const argument "
                f"{parameter.name!r} requires config type {expected_name!r}"
            )
        _validate_config_instance(value, expected_config, path=expected_name)
        return
    if getattr(type(value), "__ac_config__", False):
        raise DiagnosticTypeError(
            "ACPY-JIT-002: const argument "
            f"{parameter.name!r} annotation {target!r} is not an @ac.config type"
        )


def _closed(value: object) -> StaticValue:
    if value is None or type(value) in {bool, int, float, str}:
        result = value
    elif isinstance(value, enum.Enum):
        result = _closed(value.value)
    elif dataclasses.is_dataclass(value) and not isinstance(value, type):
        result = FrozenMap(
            tuple(
                sorted(
                    (
                        field.name,
                        _closed(getattr(value, field.name)),
                    )
                    for field in dataclasses.fields(value)
                )
            )
        )
    elif type(value) in {tuple, list}:
        result = tuple(_closed(item) for item in value)
    elif type(value) is dict:
        if any(type(key) is not str or not key for key in value):
            raise DiagnosticTypeError(
                "ACPY-JIT-002: const map keys must be non-empty strings"
            )
        result = FrozenMap(
            tuple(sorted((key, _closed(item)) for key, item in value.items()))
        )
    else:
        raise DiagnosticTypeError(
            f"ACPY-JIT-002: unsupported const value {type(value).__name__}"
        )
    try:
        validate_ijson_value(static_json_value(result))
    except ValueError as error:
        raise DiagnosticTypeError(f"ACPY-JIT-002: {error}") from error
    return result


def _display(value: StaticValue):
    if isinstance(value, FrozenMap):
        return tuple((key, _display(item)) for key, item in value.entries)
    if isinstance(value, tuple):
        return tuple(_display(item) for item in value)
    return value


@dataclass(frozen=True, slots=True)
class JitSpecialization:
    definition: Definition
    arguments: tuple[tuple[str, StaticValue], ...]
    workspace: str | None
    root_system_identity: str
    sources: tuple[SourceClosureEntry, ...]
    source_closure_sha256: str | None
    fingerprint: str
    diagnostics: tuple[Diagnostic, ...] = ()

    @property
    def canonical_arguments(self) -> tuple[tuple[str, object], ...]:
        return tuple((name, _display(value)) for name, value in self.arguments)

    def __repr__(self) -> str:
        return (
            "JitSpecialization("
            f"system={self.root_system_identity!r}, "
            f"fingerprint={self.fingerprint!r})"
        )

    @property
    def source_manifest(self) -> tuple[tuple[str, str], ...]:
        return tuple((source.path, source.sha256) for source in self.sources)

    def _validated_closure(self) -> SourceClosure | None:
        if self.workspace is None:
            return None
        if self.definition.source_file is None:
            raise DiagnosticRuntimeError(
                "ACPY-JIT-003: system has no readable source file"
            )
        try:
            current = capture_source_closure(
                Path(self.definition.source_file), Path(self.workspace)
            )
        except (OSError, ValueError) as error:
            raise DiagnosticRuntimeError(
                f"ACPY-JIT-003: source closure is unavailable: {error}"
            ) from error
        if (
            current.sha256 != self.source_closure_sha256
            or tuple((item.path, item.sha256) for item in current.entries)
            != self.source_manifest
        ):
            raise DiagnosticRuntimeError(
                "ACPY-JIT-003: source closure changed after specialization"
            )
        return current

    def _source(self) -> str:
        closure = self._validated_closure()
        if closure is not None:
            if len(closure.entries) == 1:
                entry = closure.entries[0]
                raw = Path(entry.source_file).read_bytes()
                if sha256_bytes(raw) != entry.sha256:
                    raise DiagnosticRuntimeError(
                        "ACPY-JIT-003: source changed during lowering"
                    )
                return raw.decode("utf-8")
            statements: list[ast.stmt] = []
            for entry in closure.entries:
                source = Path(entry.source_file)
                raw = source.read_bytes()
                if sha256_bytes(raw) != entry.sha256:
                    raise DiagnosticRuntimeError(
                        "ACPY-JIT-003: source changed during lowering"
                    )
                tree = ast.parse(
                    raw.decode("utf-8"), filename=entry.path, type_comments=True
                )
                statements.extend(
                    statement
                    for statement in tree.body
                    if not isinstance(statement, (ast.Import, ast.ImportFrom))
                )
            return ast.unparse(ast.fix_missing_locations(ast.Module(statements, [])))
        if self.definition.source_file is None:
            raise DiagnosticRuntimeError(
                "ACPY-JIT-003: system has no readable source file"
            )
        path = Path(self.definition.source_file)
        if not path.is_file():
            raise DiagnosticRuntimeError(
                "ACPY-JIT-003: system source file is unavailable"
            )
        return path.read_text(encoding="utf-8")

    def _display_source_path(self) -> str | None:
        """Return a stable project-relative path without exposing host paths."""

        if self.definition.source_file is None:
            return None
        source = Path(self.definition.source_file).resolve()
        roots: list[Path] = []
        if self.workspace is not None:
            roots.append(Path(self.workspace).resolve())
        try:
            roots.append(repository_root().resolve())
        except FileNotFoundError:
            pass
        roots.append(Path.cwd().resolve())
        for root in roots:
            try:
                return source.relative_to(root).as_posix()
            except ValueError:
                continue
        return source.name

    def _definition_locations(self) -> dict[str, tuple[str, int, int]]:
        """Retain original definition locations across source-closure merging."""

        closure = self._validated_closure()
        if closure is None:
            return {}
        locations: dict[str, tuple[str, int, int]] = {}
        for entry in closure.entries:
            source = Path(entry.source_file)
            raw = source.read_bytes()
            if sha256_bytes(raw) != entry.sha256:
                raise DiagnosticRuntimeError(
                    "ACPY-JIT-003: source changed during source-map capture"
                )
            tree = ast.parse(
                raw.decode("utf-8"), filename=entry.path, type_comments=True
            )
            for node in tree.body:
                if isinstance(
                    node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)
                ):
                    locations.setdefault(
                        node.name, (entry.path, node.lineno, node.col_offset + 1)
                    )
        return locations

    def _static_assert_locations(
        self,
    ) -> dict[str, tuple[tuple[str, int, int], ...]]:
        """Capture direct assertion spans before source-closure normalization."""

        closure = self._validated_closure()
        if closure is None:
            return {}
        locations: dict[str, tuple[tuple[str, int, int], ...]] = {}
        for entry in closure.entries:
            source = Path(entry.source_file)
            raw = source.read_bytes()
            if sha256_bytes(raw) != entry.sha256:
                raise DiagnosticRuntimeError(
                    "ACPY-JIT-003: source changed during source-map capture"
                )
            tree = ast.parse(
                raw.decode("utf-8"), filename=entry.path, type_comments=True
            )
            for node in tree.body:
                if not isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
                    continue
                assertions: list[tuple[str, int, int]] = []
                for statement in node.body:
                    call = (
                        statement.value
                        if isinstance(statement, ast.Expr)
                        and isinstance(statement.value, ast.Call)
                        else None
                    )
                    name = (
                        call.func.id
                        if call is not None and isinstance(call.func, ast.Name)
                        else call.func.attr
                        if call is not None and isinstance(call.func, ast.Attribute)
                        else ""
                    )
                    if name == "static_assert":
                        assertions.append(
                            (entry.path, statement.lineno, statement.col_offset + 1)
                        )
                if assertions:
                    locations[node.name] = tuple(assertions)
        return locations

    def lower_acir(self) -> str:
        """Materialize the specialization as Queue/Var ACIR text."""

        from ._queue_frontend import lower_queue_source

        return lower_queue_source(
            self._source(),
            self.definition.__name__,
            static_arguments=dict(self.arguments),
            specialization_fingerprint=self.fingerprint,
            source_path=self._display_source_path(),
            definition_locations=self._definition_locations(),
            static_assert_locations=self._static_assert_locations(),
        )

    def lower_cpp(self) -> str:
        """Materialize the specialization as typed queue-wired gfsim C++."""

        from ._queue_codegen import lower_queue_program_to_cpp
        from ._queue_frontend import parse_queue_program

        acir = self.lower_acir()
        if "  ac.system @" in acir:
            return _lower_acir_to_cpp(acir)
        program = parse_queue_program(
            self._source(),
            self.definition.__name__,
            static_arguments=dict(self.arguments),
            specialization_fingerprint=self.fingerprint,
            source_path=self._display_source_path(),
            definition_locations=self._definition_locations(),
            static_assert_locations=self._static_assert_locations(),
        )
        if program.helpers or any(
            queue.rule_name is not None for queue in program.queues
        ):
            return _lower_rule_program_to_cpp(program)
        return lower_queue_program_to_cpp(program)

    def materialize_cpp(
        self,
        cache_root: str | Path,
        *,
        compiler: str | Path | None = None,
    ) -> "JitArtifact":
        """Compile one optimized C++ specialization into a content cache."""

        selected = str(compiler or shutil.which("c++") or "")
        if not selected:
            raise DiagnosticRuntimeError("ACPY-JIT-004: no C++ compiler is available")
        version = subprocess.run(
            (selected, "--version"),
            text=True,
            capture_output=True,
            check=False,
        )
        if version.returncode != 0:
            raise DiagnosticRuntimeError("ACPY-JIT-004: C++ compiler identity failed")
        compiler_identity = version.stdout.splitlines()[0].strip()
        cpp = self.lower_cpp()
        cpp_hash = sha256_bytes(cpp.encode("utf-8"))
        key = sha256_bytes(
            canonical_json_bytes(
                {
                    "schema": "agentic-circuit-provider-specialization",
                    "version": "0.5",
                    "frontend_specialization": self.fingerprint,
                    "backend": "gfsim-cpp",
                    "provider_source_sha256": cpp_hash,
                    "compiler": compiler_identity,
                }
            )
        )
        directory = Path(cache_root) / "gfsim-cpp" / key.removeprefix("sha256:")
        source = directory / "model.cpp"
        artifact = directory / "model.o"
        manifest = directory / "manifest.json"
        if source.is_file() and artifact.is_file() and manifest.is_file():
            return JitArtifact(key, directory, source, artifact, manifest, True)

        directory.mkdir(parents=True, exist_ok=True)
        _write_atomic(source, cpp.encode("utf-8"))
        repository = repository_root()
        include_root = repository / "simulator" / "gfsim" / "include"
        with tempfile.TemporaryDirectory(dir=directory) as temporary:
            candidate = Path(temporary) / "model.o"
            completed = subprocess.run(
                (
                    selected,
                    "-std=c++20",
                    "-O3",
                    "-I",
                    str(include_root),
                    "-c",
                    str(source),
                    "-o",
                    str(candidate),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            if completed.returncode != 0:
                raise DiagnosticRuntimeError(
                    "ACPY-JIT-004: C++ specialization failed:\n" + completed.stderr
                )
            os.replace(candidate, artifact)
        manifest_value = {
            "schema": "agentic-circuit-jit-artifact",
            "version": "0.5",
            "backend": "gfsim-cpp",
            "specialization": key,
            "frontend_specialization": self.fingerprint,
            "compiler": compiler_identity,
            "provider_source_sha256": cpp_hash,
            "artifact_sha256": sha256_bytes(artifact.read_bytes()),
        }
        _write_atomic(manifest, canonical_json_bytes(manifest_value) + b"\n")
        return JitArtifact(key, directory, source, artifact, manifest, False)

    def materialize_pyc(
        self,
        cache_root: str | Path,
        *,
        pycgen_tool: str | Path,
        acir_opt: str | Path | None = None,
        pycc: str | Path,
        toolchain_metadata: str | Path,
        compiler: str | Path | None = None,
        verilator: str | Path | None = None,
    ) -> "JitPycArtifact":
        """Build cached canonical PYC, C++, and Verilog artifacts."""

        repository = repository_root()
        bundle_tool = (
            repository / "compiler" / "acir" / "tools" / "ac-queue-pyc-build.py"
        )
        lock = repository / "toolchains" / "agentic-circuit" / "pyc.lock.json"
        selected_cxx = Path(compiler or shutil.which("c++") or "")
        selected_verilator = Path(verilator or shutil.which("verilator") or "")
        paths = {
            "pycgen": Path(pycgen_tool),
            "acir_opt": Path(acir_opt)
            if acir_opt is not None
            else _native_queue_tool("acir-opt", "ACIR_OPT"),
            "pycc": Path(pycc),
            "metadata": Path(toolchain_metadata),
            "cxx": selected_cxx,
            "verilator": selected_verilator,
            "bundle": bundle_tool,
            "lock": lock,
        }
        for name, path in paths.items():
            if not path.is_file():
                raise DiagnosticRuntimeError(
                    f"ACPY-JIT-005: required {name} path is unavailable: {path}"
                )
        acir = _lower_queue_acir(self.lower_acir(), optimizer=paths["acir_opt"]).encode(
            "utf-8"
        )
        key = sha256_bytes(
            canonical_json_bytes(
                {
                    "schema": "agentic-circuit-provider-specialization",
                    "version": "0.5",
                    "frontend_specialization": self.fingerprint,
                    "backend": "pyc-cpp-verilog",
                    "acir_sha256": sha256_bytes(acir),
                    "tools": {
                        name: sha256_bytes(path.read_bytes())
                        for name, path in sorted(paths.items())
                    },
                }
            )
        )
        directory = Path(cache_root) / "pyc-cpp-verilog" / key.removeprefix("sha256:")
        pyc = directory / "model.pyc"
        cpp = directory / "cpp"
        verilog_output = directory / "verilog"
        bundle_manifest = directory / "bundle-manifest.json"
        manifest = directory / "manifest.json"
        if all(
            path.exists()
            for path in (pyc, cpp, verilog_output, bundle_manifest, manifest)
        ):
            return JitPycArtifact(
                key,
                directory,
                pyc,
                cpp,
                verilog_output,
                bundle_manifest,
                manifest,
                True,
            )

        directory.parent.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=directory.parent) as temporary:
            stage = Path(temporary) / "stage"
            stage.mkdir()
            frozen = stage / "model.ac.mlir"
            _write_atomic(frozen, acir)
            command = (
                sys.executable,
                str(bundle_tool),
                str(frozen),
                "--pycgen-tool",
                str(paths["pycgen"]),
                "--pycc",
                str(paths["pycc"]),
                "--toolchain-lock",
                str(lock),
                "--toolchain-metadata",
                str(paths["metadata"]),
                "--cxx",
                str(paths["cxx"]),
                "--verilator",
                str(paths["verilator"]),
                "--pyc-output",
                str(stage / "model.pyc"),
                "--cpp-output-dir",
                str(stage / "cpp"),
                "--verilog-output-dir",
                str(stage / "verilog"),
                "--manifest",
                str(stage / "bundle-manifest.json"),
            )
            completed = subprocess.run(
                command,
                text=True,
                capture_output=True,
                check=False,
                env={
                    **os.environ,
                    "PYTHONPATH": str(repository / "python/agentic-circuit/src"),
                },
            )
            if completed.returncode != 0:
                raise DiagnosticRuntimeError(
                    "ACPY-JIT-005: PYC specialization failed:\n" + completed.stderr
                )
            manifest_value = {
                "schema": "agentic-circuit-jit-artifact",
                "version": "0.5",
                "backend": "pyc-cpp-verilog",
                "specialization": key,
                "frontend_specialization": self.fingerprint,
                "acir_sha256": sha256_bytes(acir),
                "bundle_manifest_sha256": sha256_bytes(
                    (stage / "bundle-manifest.json").read_bytes()
                ),
            }
            _write_atomic(
                stage / "manifest.json",
                canonical_json_bytes(manifest_value) + b"\n",
            )
            if directory.exists():
                raise DiagnosticRuntimeError(
                    "ACPY-JIT-005: specialization cache entry appeared incomplete"
                )
            os.replace(stage, directory)
        return JitPycArtifact(
            key,
            directory,
            pyc,
            cpp,
            verilog_output,
            bundle_manifest,
            manifest,
            False,
        )


@dataclass(frozen=True, slots=True)
class JitArtifact:
    fingerprint: str
    directory: Path
    source: Path
    artifact: Path
    manifest: Path
    cache_hit: bool


@dataclass(frozen=True, slots=True)
class JitPycArtifact:
    fingerprint: str
    directory: Path
    pyc: Path
    cpp: Path
    verilog: Path
    bundle_manifest: Path
    manifest: Path
    cache_hit: bool


def _write_atomic(path: Path, content: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(dir=path.parent, prefix=f".{path.name}.")
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def jit(
    system: Definition,
    /,
    *,
    workspace: str | Path | None = None,
    **constants: object,
) -> JitSpecialization:
    """Create one deterministic system specialization.

    This captures metadata only.  It deliberately does not execute the system
    body or compile a backend artifact. Only ``ac.const`` parameters are bound
    here; ordinary typed parameters remain runtime values whose Queue
    boundaries are inferred by downstream lowering.
    """

    if not isinstance(system, Definition) or system.kind != "system":
        raise DiagnosticTypeError("ACPY-JIT-001: jit requires an @ac.system definition")
    signature = inspect.signature(system.function)
    if "workspace" in signature.parameters:
        raise DiagnosticTypeError(
            "ACPY-JIT-001: system parameter 'workspace' is reserved by jit"
        )
    parameters = tuple(signature.parameters.values())
    static_parameters = tuple(
        parameter
        for parameter in parameters
        if _is_const_annotation(parameter.annotation)
    )
    static_names = {parameter.name for parameter in static_parameters}
    runtime_names = {
        parameter.name for parameter in parameters if parameter.name not in static_names
    }
    supplied_names = set(constants)
    supplied_runtime = sorted(supplied_names & runtime_names)
    if supplied_runtime:
        raise DiagnosticTypeError(
            "ACPY-JIT-001: runtime system parameter "
            f"{supplied_runtime[0]!r} cannot be specialized"
        )
    unknown = sorted(supplied_names - static_names)
    if unknown:
        raise DiagnosticTypeError(
            f"ACPY-JIT-001: unexpected const argument {unknown[0]!r}"
        )

    arguments: list[tuple[str, StaticValue]] = []
    for parameter in static_parameters:
        if parameter.name in constants:
            value = constants[parameter.name]
        elif parameter.default is not inspect.Parameter.empty:
            value = parameter.default
        else:
            raise DiagnosticTypeError(
                f"ACPY-JIT-001: missing required const argument {parameter.name!r}"
            )
        _validate_const_argument(parameter, value, system)
        arguments.append((parameter.name, _closed(value)))
    frozen_arguments = tuple(arguments)
    source_hash: str | None = None
    source_closure_sha256: str | None = None
    sources: tuple[SourceClosureEntry, ...] = ()
    workspace_value: str | None = None
    root_system_identity = system.qualified_name
    source_file = system.source_file
    if source_file is not None:
        path = Path(source_file)
        if path.is_file():
            source_hash = sha256_bytes(path.read_bytes())
            if workspace is not None:
                workspace_path = Path(workspace).expanduser().resolve(strict=True)
                closure = capture_source_closure(path, workspace_path)
                root = path.resolve(strict=True).relative_to(workspace_path)
                root_module = PurePosixPath(root.as_posix()).with_suffix("").as_posix()
                root_system_identity = f"{root_module}::{system.__name__}"
                workspace_value = str(workspace_path)
                sources = closure.entries
                source_closure_sha256 = closure.sha256
                source_hash = None
    preimage = {
        "schema": "agentic-circuit-jit-specialization",
        "version": "0.5",
        "system": root_system_identity,
        "source_sha256": source_hash,
        "source_closure_sha256": source_closure_sha256,
        "source_manifest": [
            {"path": source.path, "sha256": source.sha256} for source in sources
        ],
        "arguments": {
            name: static_json_value(value) for name, value in frozen_arguments
        },
    }
    return JitSpecialization(
        system,
        frozen_arguments,
        workspace_value,
        root_system_identity,
        sources,
        source_closure_sha256,
        sha256_bytes(canonical_json_bytes(preimage)),
    )

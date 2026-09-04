"""Closed configuration records and deterministic JIT specialization metadata."""

from __future__ import annotations

import dataclasses
import enum
import inspect
import os
from dataclasses import dataclass
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from typing import TYPE_CHECKING, get_origin

from ._canonical_json import canonical_json_bytes, sha256_bytes
from ._definitions import Definition
from ._static_eval import FrozenMap, StaticValue, validate_ijson_value
from ._types import Static

if TYPE_CHECKING:
    from ._queue_frontend import QueueProgram


def _native_queue_tool(name: str, environment: str) -> Path:
    candidates: list[Path] = []
    configured = os.environ.get(environment)
    if configured:
        candidates.append(Path(configured))
    repository = Path(__file__).resolve().parents[4]
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
    raise RuntimeError(
        f"ACPY-JIT-004: native {name} is required for @ac.rule lowering"
    )


def _lower_queue_acir(
    acir: str, *, optimizer: str | Path | None = None
) -> str:
    from ._queue_frontend import RULE_LOWERING_PIPELINE

    selected = Path(optimizer) if optimizer is not None else _native_queue_tool(
        "acir-opt", "ACIR_OPT"
    )
    if not selected.is_file():
        raise RuntimeError(
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
            raise RuntimeError(
                "ACPY-JIT-004: native rule lowering failed:\n" + optimized.stderr
            )
        return lowered.read_text(encoding="utf-8")


def _lower_rule_program_to_cpp(program: QueueProgram) -> str:
    from ._queue_frontend import lower_queue_program

    generator = _native_queue_tool("acir-queue-cxxgen", "ACIR_QUEUE_CXXGEN")
    frozen_acir = _lower_queue_acir(lower_queue_program(program))
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
            raise RuntimeError(
                "ACPY-JIT-004: native rule C++ generation failed:\n"
                + emitted.stderr
            )
        return emitted.stdout


def config(cls: type[object]) -> type[object]:
    """Freeze one closed elaboration-time configuration record."""

    if not isinstance(cls, type):
        raise TypeError("ACPY-JIT-001: config must decorate a class")
    if dataclasses.is_dataclass(cls):
        raise TypeError("ACPY-JIT-001: config class must not already be a dataclass")
    frozen = dataclasses.dataclass(frozen=True, slots=True)(cls)
    setattr(frozen, "__ac_config__", True)
    return frozen


def _is_const_annotation(annotation: object) -> bool:
    if get_origin(annotation) is Static:
        return True
    if isinstance(annotation, str):
        compact = annotation.replace(" ", "")
        return compact.startswith(("const[", "ac.const[", "Static[", "ac.Static["))
    return False


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
            raise TypeError("ACPY-JIT-002: const map keys must be non-empty strings")
        result = FrozenMap(
            tuple(sorted((key, _closed(item)) for key, item in value.items()))
        )
    else:
        raise TypeError(f"ACPY-JIT-002: unsupported const value {type(value).__name__}")
    try:
        validate_ijson_value(result)
    except ValueError as error:
        raise TypeError(f"ACPY-JIT-002: {error}") from error
    return result


def _json_value(value: StaticValue):
    if isinstance(value, FrozenMap):
        return {key: _json_value(item) for key, item in value.entries}
    if isinstance(value, tuple):
        return [_json_value(item) for item in value]
    return value


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
    fingerprint: str

    @property
    def canonical_arguments(self) -> tuple[tuple[str, object], ...]:
        return tuple((name, _display(value)) for name, value in self.arguments)

    def __repr__(self) -> str:
        return (
            "JitSpecialization("
            f"system={self.definition.qualified_name!r}, "
            f"fingerprint={self.fingerprint!r})"
        )

    def _source(self) -> str:
        if self.definition.source_file is None:
            raise RuntimeError("ACPY-JIT-003: system has no readable source file")
        path = Path(self.definition.source_file)
        if not path.is_file():
            raise RuntimeError("ACPY-JIT-003: system source file is unavailable")
        return path.read_text(encoding="utf-8")

    def lower_acir(self) -> str:
        """Materialize the specialization as Queue/Var ACIR text."""

        from ._queue_frontend import lower_queue_source

        return lower_queue_source(
            self._source(),
            self.definition.__name__,
            static_arguments=dict(self.arguments),
            specialization_fingerprint=self.fingerprint,
        )

    def lower_cpp(self) -> str:
        """Materialize the specialization as typed queue-wired gfsim C++."""

        from ._queue_codegen import lower_queue_program_to_cpp
        from ._queue_frontend import parse_queue_program

        program = parse_queue_program(
            self._source(),
            self.definition.__name__,
            static_arguments=dict(self.arguments),
            specialization_fingerprint=self.fingerprint,
        )
        if any(queue.rule_name is not None for queue in program.queues):
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
            raise RuntimeError("ACPY-JIT-004: no C++ compiler is available")
        version = subprocess.run(
            (selected, "--version"),
            text=True,
            capture_output=True,
            check=False,
        )
        if version.returncode != 0:
            raise RuntimeError("ACPY-JIT-004: C++ compiler identity failed")
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
        repository = Path(__file__).resolve().parents[4]
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
                raise RuntimeError(
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

        repository = Path(__file__).resolve().parents[4]
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
                raise RuntimeError(
                    f"ACPY-JIT-005: required {name} path is unavailable: {path}"
                )
        acir = _lower_queue_acir(
            self.lower_acir(), optimizer=paths["acir_opt"]
        ).encode("utf-8")
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
                raise RuntimeError(
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
                raise RuntimeError(
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


def jit(system: Definition, /, **constants: object) -> JitSpecialization:
    """Create one deterministic, const-only system specialization.

    This captures metadata only.  It deliberately does not execute the system
    body or compile a backend artifact; downstream Queue elaboration consumes
    the frozen arguments.
    """

    if not isinstance(system, Definition) or system.kind != "system":
        raise TypeError("ACPY-JIT-001: jit requires an @ac.system definition")
    signature = inspect.signature(system.function)
    for parameter in signature.parameters.values():
        if not _is_const_annotation(parameter.annotation):
            raise TypeError(
                f"ACPY-JIT-001: system parameter {parameter.name!r} must use ac.const"
            )
    try:
        bound = signature.bind(**constants)
    except TypeError as error:
        raise TypeError(f"ACPY-JIT-001: {error}") from error
    bound.apply_defaults()
    arguments = tuple(
        (name, _closed(bound.arguments[name])) for name in signature.parameters
    )
    source_hash: str | None = None
    source_file = system.source_file
    if source_file is not None:
        path = Path(source_file)
        if path.is_file():
            source_hash = sha256_bytes(path.read_bytes())
    preimage = {
        "schema": "agentic-circuit-jit-specialization",
        "version": "0.5",
        "system": system.qualified_name,
        "source_sha256": source_hash,
        "arguments": {name: _json_value(value) for name, value in arguments},
    }
    return JitSpecialization(
        system,
        arguments,
        sha256_bytes(canonical_json_bytes(preimage)),
    )

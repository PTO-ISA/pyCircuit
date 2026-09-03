"""Immutable private Python records for the native compiler bridge."""

from __future__ import annotations

import importlib.machinery
import importlib.util
import sys
from dataclasses import dataclass
from pathlib import Path
from types import ModuleType
from typing import Mapping

from ._canonical_json import JsonValue, validate_ijson_value
from ._diagnostics import Diagnostic, FixIt, RelatedLocation, SourceSpan


@dataclass(frozen=True, slots=True)
class NativeRequest:
    acir: bytes
    stop_after: str | None
    emits: tuple[str, ...]
    options: tuple[tuple[str, object], ...] = ()

    def __post_init__(self) -> None:
        if type(self.acir) is not bytes:
            raise TypeError("native ACIR input must be bytes")
        if self.stop_after is not None and type(self.stop_after) is not str:
            raise TypeError("native stop stage must be a string or None")
        if type(self.emits) is not tuple or not all(
            type(item) is str for item in self.emits
        ):
            raise TypeError("native emits must be a tuple of strings")
        if type(self.options) is not tuple:
            raise TypeError("native options must be a tuple of pairs")
        names: set[str] = set()
        for item in self.options:
            if type(item) is not tuple or len(item) != 2 or type(item[0]) is not str:
                raise TypeError("native options must be string/value pairs")
            if item[0] in names:
                raise ValueError(f"duplicate native option: {item[0]}")
            names.add(item[0])
            value = item[1]
            if item[0] in ("dump_before", "dump_after"):
                if type(value) is not tuple or not all(
                    type(name) is str for name in value
                ):
                    raise TypeError(f"native {item[0]} must be a tuple of strings")
            elif item[0] in (
                "binding_lock",
                "binding_registry",
                "frontend_acpy",
                "frontend_acir",
            ):
                if type(value) is not bytes:
                    raise TypeError(f"native {item[0]} must be bytes")
            else:
                validate_ijson_value(value)  # type: ignore[arg-type]


@dataclass(frozen=True, slots=True)
class NativeArtifact:
    path: str
    kind: str
    data: bytes
    sha256: str


@dataclass(frozen=True, slots=True)
class NativeResult:
    artifacts: tuple[NativeArtifact, ...]
    diagnostics: tuple[Diagnostic, ...]
    build_directory: str | None
    executable: str | None
    build_fingerprint: str | None
    cache_hit: bool | None


@dataclass(frozen=True, slots=True)
class NativeCapabilities:
    compiler_build_id: str
    runtime_build_id: str
    items: tuple[Mapping[str, JsonValue], ...]


_NATIVE: ModuleType | None = None


def _load_native() -> ModuleType:
    global _NATIVE
    if _NATIVE is not None:
        return _NATIVE
    try:
        from . import _native as imported
    except ImportError as first_error:
        for entry in map(Path, sys.path):
            package = entry / "agentic_circuit"
            for suffix in importlib.machinery.EXTENSION_SUFFIXES:
                candidate = package / ("_native" + suffix)
                if not candidate.is_file():
                    continue
                spec = importlib.util.spec_from_file_location(
                    "agentic_circuit._native", candidate
                )
                if spec is None or spec.loader is None:
                    continue
                module = importlib.util.module_from_spec(spec)
                sys.modules[spec.name] = module
                spec.loader.exec_module(module)
                _NATIVE = module
                return module
        raise ImportError(
            "the Agentic Circuit native extension is unavailable"
        ) from first_error
    _NATIVE = imported
    return imported


def native_extension_path() -> Path:
    path = getattr(_load_native(), "__file__", None)
    if type(path) is not str:
        raise RuntimeError("the Agentic Circuit native extension has no file identity")
    return Path(path).resolve()


def _closed_dict(value: object, keys: frozenset[str], label: str) -> dict[str, object]:
    if type(value) is not dict:
        raise TypeError(f"native {label} must be a dictionary")
    result = value
    if set(result) != keys:
        raise ValueError(f"native {label} has unexpected fields")
    return result


def _optional_string(value: object, label: str) -> str | None:
    if value is not None and type(value) is not str:
        raise TypeError(f"native {label} must be a string or None")
    return value


def _source(value: object) -> SourceSpan | None:
    if value is None:
        return None
    item = _closed_dict(value, frozenset({"file", "line", "column"}), "source")
    file = item["file"]
    line = item["line"]
    column = item["column"]
    if type(file) is not str or type(line) is not int or type(column) is not int:
        raise TypeError("native source fields have invalid types")
    return SourceSpan(file, line, column, line, column)


def _diagnostic(value: object) -> Diagnostic:
    item = _closed_dict(
        value,
        frozenset(
            {
                "stage",
                "code",
                "severity",
                "message",
                "source",
                "object_path",
                "expected",
                "actual",
                "related",
                "fixits",
            }
        ),
        "diagnostic",
    )
    related_value = item["related"]
    fixits_value = item["fixits"]
    if type(related_value) is not tuple or type(fixits_value) is not tuple:
        raise TypeError("native diagnostic collections must be tuples")
    related: list[RelatedLocation] = []
    for value in related_value:
        entry = _closed_dict(
            value,
            frozenset({"message", "source", "object_path"}),
            "related location",
        )
        if type(entry["message"]) is not str:
            raise TypeError("native related message must be a string")
        related.append(
            RelatedLocation(
                message=entry["message"],
                source=_source(entry["source"]),
                object_path=_optional_string(entry["object_path"], "object path"),
            )
        )
    fixits: list[FixIt] = []
    for value in fixits_value:
        entry = _closed_dict(value, frozenset({"message"}), "fix-it")
        if type(entry["message"]) is not str:
            raise TypeError("native fix-it message must be a string")
        fixits.append(FixIt(entry["message"]))
    expected = item["expected"]
    actual = item["actual"]
    validate_ijson_value(expected)  # type: ignore[arg-type]
    validate_ijson_value(actual)  # type: ignore[arg-type]
    for name in ("stage", "code", "severity", "message"):
        if type(item[name]) is not str:
            raise TypeError(f"native diagnostic {name} must be a string")
    severity = "note" if item["severity"] == "remark" else item["severity"]
    return Diagnostic(
        stage=item["stage"],
        code=item["code"],
        severity=severity,  # type: ignore[arg-type]
        message=item["message"],
        source=_source(item["source"]),
        object_path=_optional_string(item["object_path"], "object path"),
        expected=expected,  # type: ignore[arg-type]
        actual=actual,  # type: ignore[arg-type]
        related=tuple(related),
        fixits=tuple(fixits),
    )


def _result(value: object) -> NativeResult:
    item = _closed_dict(
        value,
        frozenset(
            {
                "artifacts",
                "diagnostics",
                "build_directory",
                "executable",
                "build_fingerprint",
                "cache_hit",
            }
        ),
        "compiler result",
    )
    artifact_values = item["artifacts"]
    diagnostic_values = item["diagnostics"]
    if type(artifact_values) is not tuple or type(diagnostic_values) is not tuple:
        raise TypeError("native result collections must be tuples")
    artifacts: list[NativeArtifact] = []
    for value in artifact_values:
        artifact = _closed_dict(
            value, frozenset({"path", "kind", "data", "sha256"}), "artifact"
        )
        if not (
            type(artifact["path"]) is str
            and type(artifact["kind"]) is str
            and type(artifact["data"]) is bytes
            and type(artifact["sha256"]) is str
        ):
            raise TypeError("native artifact fields have invalid types")
        artifacts.append(NativeArtifact(**artifact))  # type: ignore[arg-type]
    diagnostics = tuple(
        sorted(map(_diagnostic, diagnostic_values), key=Diagnostic.sort_key)
    )
    cache_hit = item["cache_hit"]
    if cache_hit is not None and type(cache_hit) is not bool:
        raise TypeError("native cache hit must be a boolean or None")
    return NativeResult(
        artifacts=tuple(artifacts),
        diagnostics=diagnostics,
        build_directory=_optional_string(item["build_directory"], "build directory"),
        executable=_optional_string(item["executable"], "executable"),
        build_fingerprint=_optional_string(
            item["build_fingerprint"], "build fingerprint"
        ),
        cache_hit=cache_hit,
    )


def run_native_compiler(request: NativeRequest) -> NativeResult:
    if type(request) is not NativeRequest:
        raise TypeError("request must be an exact NativeRequest")
    raw = _load_native().run_compiler(
        {
            "acir": request.acir,
            "stop_after": request.stop_after,
            "emits": request.emits,
            "options": dict(request.options),
        }
    )
    return _result(raw)


def capabilities() -> NativeCapabilities:
    raw = _closed_dict(
        _load_native().capabilities(),
        frozenset({"compiler_build_id", "runtime_build_id", "items"}),
        "capabilities",
    )
    compiler = raw["compiler_build_id"]
    runtime = raw["runtime_build_id"]
    items = raw["items"]
    if (
        type(compiler) is not str
        or type(runtime) is not str
        or type(items) is not tuple
    ):
        raise TypeError("native capabilities fields have invalid types")
    for item in items:
        validate_ijson_value(item)
    return NativeCapabilities(compiler, runtime, items)  # type: ignore[arg-type]

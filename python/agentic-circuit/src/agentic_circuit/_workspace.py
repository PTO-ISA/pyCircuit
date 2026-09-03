"""Closed workspace discovery and TOML configuration."""

from __future__ import annotations

import re
import tomllib
from dataclasses import dataclass
from pathlib import Path
from typing import Literal, NoReturn

from ._canonical_json import JsonValue, validate_ijson_value
from ._diagnostics import Diagnostic


_NAME = re.compile(r"^[A-Za-z_][A-Za-z0-9_.-]*$")
_TOP_LEVEL = frozenset(
    {"contract_epoch", "project", "providers", "build", "run", "diagnostics"}
)


class UserInputError(Exception):
    """A stable user-facing configuration or command failure."""

    def __init__(self, diagnostic: Diagnostic):
        super().__init__(diagnostic.message)
        self.diagnostic = diagnostic


@dataclass(frozen=True, slots=True)
class WorkspaceConfig:
    root: Path
    project_name: str
    project_version: str
    contract_epoch: str
    architecture: Path
    default_system: str
    standard_library_providers: tuple[str, ...]
    build_profile: Literal["fast", "validated", "custom"]
    compiler: str
    standard_library: str
    component_roots: tuple[Path, ...]
    protocol_roots: tuple[Path, ...]
    trace_roots: tuple[Path, ...]
    build_root: Path
    default_trace: Path | None
    default_run_inputs: tuple[tuple[str, JsonValue], ...]
    diagnostic_format: Literal["text", "json", "jsonl"]
    instrumentation_layers: tuple[str, ...]


def _fail(code: str, message: str) -> NoReturn:
    raise UserInputError(
        Diagnostic(
            stage="workspace",
            code=code,
            severity="error",
            message=message,
        )
    )


def _closed_table(
    value: object,
    *,
    section: str,
    allowed: frozenset[str],
    required: frozenset[str] = frozenset(),
) -> dict[str, object]:
    if type(value) is not dict:
        _fail("ACPY-CONFIG-002", f"[{section}] must be a table")
    table: dict[str, object] = value
    unknown = sorted(set(table) - allowed)
    missing = sorted(required - set(table))
    if unknown:
        _fail(
            "ACPY-CONFIG-002",
            f"[{section}] contains unknown key {unknown[0]!r}",
        )
    if missing:
        _fail(
            "ACPY-CONFIG-002",
            f"[{section}] is missing required key {missing[0]!r}",
        )
    return table


def _string(table: dict[str, object], key: str, section: str) -> str:
    value = table[key]
    if type(value) is not str or not value:
        _fail("ACPY-CONFIG-002", f"[{section}].{key} must be a non-empty string")
    return value


def _string_list(table: dict[str, object], key: str, section: str) -> tuple[str, ...]:
    value = table[key]
    if type(value) is not list or not all(type(item) is str and item for item in value):
        _fail("ACPY-CONFIG-002", f"[{section}].{key} must be an array of strings")
    result = tuple(value)
    if len(set(result)) != len(result):
        _fail("ACPY-CONFIG-004", f"[{section}].{key} contains a duplicate")
    return result


def _workspace_path(root: Path, value: str, label: str) -> Path:
    candidate = Path(value)
    if candidate.is_absolute():
        _fail("ACPY-CONFIG-003", f"{label} must be workspace-relative")
    resolved = (root / candidate).resolve()
    if not resolved.is_relative_to(root):
        _fail("ACPY-CONFIG-003", f"{label} escapes the workspace")
    return resolved


def _workspace_paths(
    root: Path, table: dict[str, object], key: str, section: str
) -> tuple[Path, ...]:
    return tuple(
        _workspace_path(root, value, f"[{section}].{key}")
        for value in _string_list(table, key, section)
    )


def load_workspace(manifest: Path) -> WorkspaceConfig:
    """Load exactly one manifest without current-directory discovery."""

    manifest = manifest.resolve()
    if not manifest.is_file():
        _fail("ACPY-CONFIG-001", f"workspace manifest not found: {manifest}")
    try:
        with manifest.open("rb") as source:
            document = tomllib.load(source)
    except (OSError, tomllib.TOMLDecodeError) as error:
        _fail("ACPY-CONFIG-002", f"workspace manifest is invalid: {error}")
    unknown = sorted(set(document) - _TOP_LEVEL)
    missing = sorted(_TOP_LEVEL - set(document))
    if unknown:
        _fail("ACPY-CONFIG-002", f"workspace contains unknown section {unknown[0]!r}")
    if missing:
        _fail("ACPY-CONFIG-002", f"workspace is missing {missing[0]!r}")

    epoch = document["contract_epoch"]
    if epoch != "0.4":
        _fail("ACPY-CONFIG-005", "contract_epoch must equal 0.4")
    root = manifest.parent.resolve()

    project = _closed_table(
        document["project"],
        section="project",
        allowed=frozenset({"name", "version", "architecture", "system"}),
        required=frozenset({"name", "version", "architecture", "system"}),
    )
    project_name = _string(project, "name", "project")
    project_version = _string(project, "version", "project")
    default_system = _string(project, "system", "project")
    if not _NAME.fullmatch(project_name) or not _NAME.fullmatch(default_system):
        _fail("ACPY-CONFIG-002", "project and system names use invalid syntax")
    architecture = _workspace_path(
        root, _string(project, "architecture", "project"), "[project].architecture"
    )

    providers = _closed_table(
        document["providers"],
        section="providers",
        allowed=frozenset({"standard_library"}),
        required=frozenset({"standard_library"}),
    )
    provider_names = _string_list(providers, "standard_library", "providers")
    if any(name != "ac" for name in provider_names):
        _fail("ACPY-CONFIG-006", "unknown standard-library provider")

    build = _closed_table(
        document["build"],
        section="build",
        allowed=frozenset(
            {
                "profile",
                "compiler",
                "standard_library",
                "component_roots",
                "protocol_roots",
                "build_root",
                "instrumentation_layers",
            }
        ),
        required=frozenset(
            {
                "profile",
                "compiler",
                "standard_library",
                "component_roots",
                "protocol_roots",
                "build_root",
                "instrumentation_layers",
            }
        ),
    )
    profile = _string(build, "profile", "build")
    if profile not in ("fast", "validated", "custom"):
        _fail("ACPY-CONFIG-002", "[build].profile is not supported")
    compiler = _string(build, "compiler", "build")
    standard_library = _string(build, "standard_library", "build")
    if standard_library not in ("libc++", "libstdc++"):
        _fail("ACPY-CONFIG-002", "[build].standard_library is not supported")
    component_roots = _workspace_paths(root, build, "component_roots", "build")
    protocol_roots = _workspace_paths(root, build, "protocol_roots", "build")
    if set(component_roots) & set(protocol_roots):
        _fail("ACPY-CONFIG-004", "component and protocol roots overlap")
    build_root = _workspace_path(
        root, _string(build, "build_root", "build"), "[build].build_root"
    )
    instrumentation = _string_list(build, "instrumentation_layers", "build")

    run = _closed_table(
        document["run"],
        section="run",
        allowed=frozenset({"trace_roots", "default_trace", "inputs"}),
        required=frozenset({"trace_roots", "inputs"}),
    )
    trace_roots = _workspace_paths(root, run, "trace_roots", "run")
    default_trace = (
        _workspace_path(
            root,
            _string(run, "default_trace", "run"),
            "[run].default_trace",
        )
        if "default_trace" in run
        else None
    )
    inputs = run["inputs"]
    if type(inputs) is not dict or not all(type(key) is str for key in inputs):
        _fail("ACPY-CONFIG-002", "[run].inputs must be a string-keyed table")
    try:
        validate_ijson_value(inputs)
    except ValueError as error:
        _fail("ACPY-CONFIG-002", f"[run].inputs is invalid: {error}")
    default_run_inputs = tuple(sorted(inputs.items()))

    diagnostics = _closed_table(
        document["diagnostics"],
        section="diagnostics",
        allowed=frozenset({"format"}),
        required=frozenset({"format"}),
    )
    diagnostic_format = _string(diagnostics, "format", "diagnostics")
    if diagnostic_format not in ("text", "json", "jsonl"):
        _fail("ACPY-CONFIG-002", "[diagnostics].format is not supported")

    return WorkspaceConfig(
        root=root,
        project_name=project_name,
        project_version=project_version,
        contract_epoch="0.4",
        architecture=architecture,
        default_system=default_system,
        standard_library_providers=provider_names,
        build_profile=profile,
        compiler=compiler,
        standard_library=standard_library,
        component_roots=component_roots,
        protocol_roots=protocol_roots,
        trace_roots=trace_roots,
        build_root=build_root,
        default_trace=default_trace,
        default_run_inputs=default_run_inputs,
        diagnostic_format=diagnostic_format,
        instrumentation_layers=instrumentation,
    )


def discover_workspace(start: Path) -> WorkspaceConfig:
    """Search from *start* upward for the nearest workspace manifest."""

    resolved = start.resolve()
    if resolved.is_file():
        resolved = resolved.parent
    for directory in (resolved, *resolved.parents):
        candidate = directory / "agentic-circuit.toml"
        if candidate.is_file():
            return load_workspace(candidate)
    _fail("ACPY-CONFIG-001", "workspace configuration not found")

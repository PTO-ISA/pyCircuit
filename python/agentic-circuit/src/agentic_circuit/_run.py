"""Immutable run bundles and runtime-harness execution."""

from __future__ import annotations

import json
import os
import re
import signal
import shutil
import stat
import subprocess
import tempfile
from dataclasses import dataclass
from pathlib import Path, PurePosixPath
from typing import Literal, NoReturn

from ._canonical_json import sha256_bytes
from ._commands.build import BuildPublication
from ._diagnostics import Diagnostic
from ._staging import ArtifactStage


TerminationKind = Literal["complete", "incomplete", "any"]
RunStatus = Literal["completed", "incomplete", "failed"]
_DOMAIN = re.compile(r"^[A-Za-z_][A-Za-z0-9_.-]*$")
_FINGERPRINT = re.compile(r"^sha256:[0-9a-f]{64}$")
_MANIFEST_KEYS = frozenset(
    {
        "schema",
        "version",
        "contract_epoch",
        "build_manifest",
        "trace",
        "seed",
        "output_directory",
        "deadlock_window",
        "max_ticks",
        "max_domain_cycles",
        "stats_format",
        "event_log",
        "termination_expectation",
    }
)
_RESULT_KEYS = frozenset(
    {
        "schema",
        "version",
        "contract_epoch",
        "run_manifest",
        "status",
        "termination_reason",
        "simulated_ticks",
        "domain_cycles",
        "event_count",
        "trace_position",
        "outputs",
        "validation",
    }
)
_COMPLETED_REASONS = frozenset({"trace_drained", "declared_model_termination"})
_INCOMPLETE_REASONS = frozenset(
    {
        "max_ticks",
        "max_domain_cycles",
        "max_events",
        "max_deltas_per_tick",
        "max_trace_records",
        "max_validation_work",
        "interrupted",
    }
)
_FAILED_REASONS = frozenset(
    {"deadlock", "invariant_violation", "trace_error", "runtime_error"}
)


@dataclass(frozen=True, slots=True)
class RunOptions:
    trace: Path
    output_directory: Path
    seed: int
    deadlock_window: int | None
    max_ticks: int | None
    max_domain_cycles: tuple[tuple[str, int], ...]
    stats_format: Literal["json"]
    event_log: Literal["disabled", "jsonl"]
    termination_kind: TerminationKind


@dataclass(frozen=True, slots=True)
class RunPublication:
    directory: Path
    manifest: Path
    result: Path
    status: RunStatus
    termination_reason: str
    exit_code: int


class RunFailure(Exception):
    def __init__(self, exit_code: int, diagnostic: Diagnostic):
        super().__init__(diagnostic.message)
        self.exit_code = exit_code
        self.diagnostic = diagnostic


def _failure(exit_code: int, code: str, message: str) -> NoReturn:
    raise RunFailure(
        exit_code,
        Diagnostic(stage="runtime", code=code, severity="error", message=message),
    )


def _normalized_relative(value: object, label: str) -> str:
    if type(value) is not str:
        _failure(5, "ACRUN-PREFLIGHT-001", f"{label} must be a relative path")
    path = PurePosixPath(value)
    if (
        path.is_absolute()
        or not path.parts
        or any(part in ("", ".", "..") for part in path.parts)
        or "\\" in value
    ):
        _failure(5, "ACRUN-PREFLIGHT-001", f"{label} is not normalized")
    return value


def _pairs(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate object name {key!r}")
        result[key] = value
    return result


def _json_document(data: bytes, label: str) -> dict[str, object]:
    try:
        value = json.loads(data.decode("utf-8"), object_pairs_hook=_pairs)
    except (UnicodeError, json.JSONDecodeError, ValueError) as error:
        _failure(5, "ACRUN-PREFLIGHT-001", f"{label} is invalid JSON: {error}")
    if type(value) is not dict:
        _failure(5, "ACRUN-PREFLIGHT-001", f"{label} must be a JSON object")
    return value


def _regular_file(path: Path, label: str) -> bytes:
    try:
        if path.is_symlink() or not path.is_file():
            raise OSError("not a regular file")
        return path.read_bytes()
    except OSError as error:
        _failure(5, "ACRUN-PREFLIGHT-001", f"cannot read {label}: {error}")


def _file_hash(value: object, label: str) -> tuple[str, str]:
    if type(value) is not dict or set(value) != {"path", "sha256"}:
        _failure(5, "ACRUN-PREFLIGHT-001", f"{label} is not a file identity")
    path = _normalized_relative(value["path"], f"{label}.path")
    fingerprint = value["sha256"]
    if type(fingerprint) is not str or not _FINGERPRINT.fullmatch(fingerprint):
        _failure(5, "ACRUN-PREFLIGHT-001", f"{label}.sha256 is invalid")
    return path, fingerprint


def _positive_optional(value: object, label: str) -> int | None:
    if value is None:
        return None
    if type(value) is not int or value < 1 or value > (1 << 64) - 1:
        _failure(5, "ACRUN-PREFLIGHT-001", f"{label} must be null or positive")
    return value


def _canonical_bytes(value: dict[str, object]) -> bytes:
    return (
        json.dumps(value, ensure_ascii=False, sort_keys=True, separators=(",", ":"))
        + "\n"
    ).encode("utf-8")


def _validate_trace(data: bytes) -> None:
    document = _json_document(data, "trace")
    if (
        set(document) != {"schema", "version", "contract_epoch", "metadata", "records"}
        or document.get("schema") != "pto-trace"
        or document.get("version") != "0.1"
        or document.get("contract_epoch") != "0.5"
        or type(document.get("metadata")) is not dict
        or type(document.get("records")) is not list
    ):
        _failure(5, "ACTRACE-SCHEMA-001", "trace has an invalid closed envelope")


def create_run_manifest(build: BuildPublication, options: RunOptions) -> bytes:
    if type(options.seed) is not int or not 0 <= options.seed <= (1 << 64) - 1:
        _failure(2, "ACRUN-INPUT-001", "seed must be an unsigned 64-bit integer")
    if options.deadlock_window is not None and (
        type(options.deadlock_window) is not int
        or not 1 <= options.deadlock_window <= (1 << 64) - 1
    ):
        _failure(2, "ACRUN-INPUT-001", "deadlock window must be positive")
    if options.max_ticks is not None and (
        type(options.max_ticks) is not int
        or not 1 <= options.max_ticks <= (1 << 64) - 1
    ):
        _failure(2, "ACRUN-INPUT-001", "max ticks must be positive")
    domains: dict[str, int] = {}
    for name, maximum in options.max_domain_cycles:
        if (
            type(name) is not str
            or not _DOMAIN.fullmatch(name)
            or type(maximum) is not int
            or not 1 <= maximum <= (1 << 64) - 1
            or name in domains
        ):
            _failure(2, "ACRUN-INPUT-001", "domain bounds must be unique and positive")
        domains[name] = maximum
    if (
        options.stats_format != "json"
        or options.event_log not in ("disabled", "jsonl")
        or options.termination_kind not in ("complete", "incomplete", "any")
    ):
        _failure(2, "ACRUN-INPUT-001", "run output or expectation is invalid")
    build_bytes = _regular_file(build.manifest, "build manifest")
    trace_bytes = _regular_file(options.trace, "trace")
    _validate_trace(trace_bytes)
    document: dict[str, object] = {
        "schema": "agentic-circuit-run-manifest",
        "version": "0.1",
        "contract_epoch": "0.5",
        "build_manifest": {
            "path": "build-manifest.json",
            "sha256": sha256_bytes(build_bytes),
        },
        "trace": {
            "path": "trace.json",
            "schema": "pto-trace",
            "version": "0.1",
            "sha256": sha256_bytes(trace_bytes),
        },
        "seed": options.seed,
        "output_directory": "runtime-result",
        "deadlock_window": options.deadlock_window,
        "max_ticks": options.max_ticks,
        "max_domain_cycles": domains,
        "stats_format": options.stats_format,
        "event_log": options.event_log,
        "termination_expectation": {
            "kind": options.termination_kind,
            "reason": None,
        },
    }
    return _canonical_bytes(document)


def _build_executable(
    root: Path, build_document: dict[str, object]
) -> tuple[str, bytes, int]:
    artifacts = build_document.get("artifacts")
    if type(artifacts) is not list:
        _failure(5, "ACRUN-PREFLIGHT-001", "build artifacts are invalid")
    candidates = [
        item
        for item in artifacts
        if type(item) is dict and item.get("kind") == "executable"
    ]
    if len(candidates) != 1:
        _failure(5, "ACRUN-PREFLIGHT-001", "build must name one executable")
    candidate = candidates[0]
    if set(candidate) != {"path", "kind", "sha256"}:
        _failure(5, "ACRUN-PREFLIGHT-001", "build executable identity is invalid")
    path = _normalized_relative(candidate["path"], "build executable.path")
    fingerprint = candidate["sha256"]
    if type(fingerprint) is not str or not _FINGERPRINT.fullmatch(fingerprint):
        _failure(5, "ACRUN-PREFLIGHT-001", "build executable hash is invalid")
    source = root.joinpath(*PurePosixPath(path).parts)
    data = _regular_file(source, "build executable")
    if sha256_bytes(data) != fingerprint:
        _failure(5, "ACRUN-PREFLIGHT-001", "build executable hash does not match")
    mode = stat.S_IMODE(source.stat().st_mode)
    if not mode & stat.S_IXUSR:
        _failure(5, "ACRUN-PREFLIGHT-001", "build executable is not executable")
    return path, data, mode


def _verify_manifest(data: bytes) -> dict[str, object]:
    document = _json_document(data, "run manifest")
    if (
        set(document) != _MANIFEST_KEYS
        or document.get("schema") != "agentic-circuit-run-manifest"
        or document.get("version") != "0.1"
        or document.get("contract_epoch") != "0.5"
    ):
        _failure(5, "ACRUN-PREFLIGHT-001", "run manifest has an invalid envelope")
    _file_hash(document.get("build_manifest"), "build_manifest")
    trace = document.get("trace")
    if type(trace) is not dict or set(trace) != {"path", "schema", "version", "sha256"}:
        _failure(5, "ACRUN-PREFLIGHT-001", "trace identity is invalid")
    _normalized_relative(trace.get("path"), "trace.path")
    if (
        trace.get("schema") != "pto-trace"
        or trace.get("version") != "0.1"
        or type(trace.get("sha256")) is not str
        or not _FINGERPRINT.fullmatch(trace["sha256"])
    ):
        _failure(5, "ACRUN-PREFLIGHT-001", "trace identity is invalid")
    seed = document.get("seed")
    if type(seed) is not int or not 0 <= seed <= (1 << 64) - 1:
        _failure(5, "ACRUN-PREFLIGHT-001", "seed is invalid")
    _normalized_relative(document.get("output_directory"), "output_directory")
    _positive_optional(document.get("deadlock_window"), "deadlock_window")
    _positive_optional(document.get("max_ticks"), "max_ticks")
    domain_limits = document.get("max_domain_cycles")
    if type(domain_limits) is not dict or any(
        type(name) is not str
        or not _DOMAIN.fullmatch(name)
        or type(maximum) is not int
        or not 1 <= maximum <= (1 << 64) - 1
        for name, maximum in domain_limits.items()
    ):
        _failure(5, "ACRUN-PREFLIGHT-001", "domain cycle limits are invalid")
    if document.get("stats_format") != "json" or document.get("event_log") not in (
        "disabled",
        "jsonl",
    ):
        _failure(5, "ACRUN-PREFLIGHT-001", "runtime output format is invalid")
    expectation = document.get("termination_expectation")
    if (
        type(expectation) is not dict
        or set(expectation) != {"kind", "reason"}
        or expectation.get("kind") not in ("complete", "incomplete", "any")
        or expectation.get("reason")
        not in (
            None,
            "trace_drained",
            "max_ticks",
            "max_domain_cycles",
            "declared_model_termination",
        )
    ):
        _failure(5, "ACRUN-PREFLIGHT-001", "termination expectation is invalid")
    reason = expectation.get("reason")
    kind = expectation.get("kind")
    if (
        kind == "complete"
        and reason not in (None, "trace_drained", "declared_model_termination")
    ) or (
        kind == "incomplete" and reason not in (None, "max_ticks", "max_domain_cycles")
    ):
        _failure(5, "ACRUN-PREFLIGHT-001", "termination expectation is inconsistent")
    if data != _canonical_bytes(document):
        _failure(5, "ACRUN-PREFLIGHT-001", "run manifest is not canonical")
    return document


def _uint64(value: object) -> bool:
    return type(value) is int and 0 <= value <= (1 << 64) - 1


def _verify_run_result_impl(
    stage: Path,
    manifest_bytes: bytes,
    return_code: int,
    expected_outputs: frozenset[str],
) -> tuple[RunStatus, str]:
    result_path = stage / "run-result.json"
    data = _regular_file(result_path, "run result")
    document = _json_document(data, "run result")
    if (
        set(document) != _RESULT_KEYS
        or document.get("schema") != "agentic-circuit-run-result"
        or document.get("version") != "0.1"
        or document.get("contract_epoch") != "0.5"
    ):
        _failure(6, "ACRUN-RESULT-001", "run result has an invalid envelope")
    path, fingerprint = _file_hash(document.get("run_manifest"), "run_manifest")
    if path != "run-manifest.json" or fingerprint != sha256_bytes(manifest_bytes):
        _failure(6, "ACRUN-RESULT-001", "run result references another manifest")
    status = document.get("status")
    reason = document.get("termination_reason")
    expected_codes = {"completed": 0, "incomplete": 7, "failed": 6}
    if (
        type(status) is not str
        or status not in expected_codes
        or type(reason) is not str
    ):
        _failure(6, "ACRUN-RESULT-001", "run result status is invalid")
    allowed_reasons = {
        "completed": _COMPLETED_REASONS,
        "incomplete": _INCOMPLETE_REASONS,
        "failed": _FAILED_REASONS,
    }
    if reason not in allowed_reasons[status]:
        _failure(6, "ACRUN-RESULT-001", "run result reason disagrees with status")
    if return_code != expected_codes[status]:
        _failure(6, "ACRUN-RESULT-001", "runtime exit and result status disagree")
    if not _uint64(document.get("simulated_ticks")) or not _uint64(
        document.get("event_count")
    ):
        _failure(6, "ACRUN-RESULT-001", "run counters are invalid")
    domain_cycles = document.get("domain_cycles")
    if type(domain_cycles) is not dict or any(
        type(name) is not str or not _DOMAIN.fullmatch(name) or not _uint64(count)
        for name, count in domain_cycles.items()
    ):
        _failure(6, "ACRUN-RESULT-001", "domain cycle counters are invalid")
    trace_position = document.get("trace_position")
    if (
        type(trace_position) is not dict
        or set(trace_position) != {"next_record_index", "last_committed_sequence_id"}
        or not _uint64(trace_position.get("next_record_index"))
        or (
            trace_position.get("last_committed_sequence_id") is not None
            and not _uint64(trace_position.get("last_committed_sequence_id"))
        )
    ):
        _failure(6, "ACRUN-RESULT-001", "trace position is invalid")
    validation = document.get("validation")
    if type(validation) is not dict or set(validation) != {"status", "report_sha256"}:
        _failure(6, "ACRUN-RESULT-001", "validation result is invalid")
    validation_status = validation.get("status")
    report_hash = validation.get("report_sha256")
    if validation_status not in ("passed", "failed", "not_run") or (
        (validation_status == "not_run" and report_hash is not None)
        or (
            validation_status != "not_run"
            and (
                type(report_hash) is not str or not _FINGERPRINT.fullmatch(report_hash)
            )
        )
    ):
        _failure(6, "ACRUN-RESULT-001", "validation identity is invalid")
    outputs = document.get("outputs")
    if type(outputs) is not list:
        _failure(6, "ACRUN-RESULT-001", "run outputs are invalid")
    paths: list[str] = []
    output_hashes: dict[str, str] = {}
    for item in outputs:
        relative, output_hash = _file_hash(item, "run output")
        output_data = _regular_file(
            stage.joinpath(*PurePosixPath(relative).parts), "run output"
        )
        if sha256_bytes(output_data) != output_hash:
            _failure(6, "ACRUN-RESULT-001", "run output hash does not match")
        paths.append(relative)
        output_hashes[relative] = output_hash
    if paths != sorted(set(paths)) or set(paths) != expected_outputs:
        _failure(6, "ACRUN-RESULT-001", "run outputs are not canonical")
    if report_hash != output_hashes.get("validation-report.json"):
        _failure(6, "ACRUN-RESULT-001", "validation report hash does not match")
    found: set[str] = set()
    for candidate in stage.rglob("*"):
        if candidate.is_symlink():
            _failure(6, "ACRUN-RESULT-001", "runtime result contains a symlink")
        if candidate.is_file():
            found.add(candidate.relative_to(stage).as_posix())
        elif not candidate.is_dir():
            _failure(6, "ACRUN-RESULT-001", "runtime result is not a regular file set")
    if found != expected_outputs | {"run-result.json"}:
        _failure(6, "ACRUN-RESULT-001", "runtime result stage is not closed")
    if data != _canonical_bytes(document):
        _failure(6, "ACRUN-RESULT-001", "run result is not canonical")
    return status, reason


def _verify_run_result(
    stage: Path,
    manifest_bytes: bytes,
    return_code: int,
    expected_outputs: frozenset[str],
) -> tuple[RunStatus, str]:
    try:
        return _verify_run_result_impl(
            stage, manifest_bytes, return_code, expected_outputs
        )
    except RunFailure as error:
        if error.exit_code == 6:
            raise
        _failure(6, "ACRUN-RESULT-001", error.diagnostic.message)


def _execute_bundle(
    *,
    source_root: Path,
    manifest_bytes: bytes,
    output_directory: Path,
) -> RunPublication:
    manifest = _verify_manifest(manifest_bytes)
    build_path, build_hash = _file_hash(manifest["build_manifest"], "build_manifest")
    build_bytes = _regular_file(
        source_root.joinpath(*PurePosixPath(build_path).parts), "build manifest"
    )
    if sha256_bytes(build_bytes) != build_hash:
        _failure(5, "ACRUN-PREFLIGHT-001", "build manifest hash does not match")
    build_document = _json_document(build_bytes, "build manifest")
    executable_path, executable_bytes, executable_mode = _build_executable(
        source_root, build_document
    )
    trace = manifest["trace"]
    assert isinstance(trace, dict)
    trace_path = _normalized_relative(trace["path"], "trace.path")
    trace_bytes = _regular_file(
        source_root.joinpath(*PurePosixPath(trace_path).parts), "trace"
    )
    if sha256_bytes(trace_bytes) != trace["sha256"]:
        _failure(5, "ACRUN-PREFLIGHT-001", "trace hash does not match")
    _validate_trace(trace_bytes)
    if output_directory.exists() and (
        output_directory.is_symlink()
        or not output_directory.is_dir()
        or any(output_directory.iterdir())
    ):
        _failure(5, "ACRUN-PUBLISH-001", "run output directory is not empty")

    runtime_files = ["run-result.json", "stats.json", "validation-report.json"]
    if manifest["event_log"] == "jsonl":
        runtime_files.append("events.jsonl")
    input_paths = {build_path, executable_path, trace_path, "run-manifest.json"}
    if len(input_paths) != 4 or input_paths & set(runtime_files):
        _failure(5, "ACRUN-PREFLIGHT-001", "run bundle input paths collide")
    result_directory = PurePosixPath(str(manifest["output_directory"]))
    bundle_paths = input_paths | set(runtime_files)
    if any(
        result_directory == PurePosixPath(path)
        or result_directory in PurePosixPath(path).parents
        or PurePosixPath(path) in result_directory.parents
        for path in bundle_paths
    ):
        _failure(5, "ACRUN-PREFLIGHT-001", "runtime result stage collides with bundle")
    expected = tuple(
        sorted(
            {
                build_path,
                executable_path,
                trace_path,
                "run-manifest.json",
                *runtime_files,
            }
        )
    )
    with ArtifactStage(output_directory, expected=expected) as stage:
        assert stage.path is not None
        stage.write_bytes(build_path, build_bytes)
        stage.write_bytes(executable_path, executable_bytes)
        stage.path.joinpath(*PurePosixPath(executable_path).parts).chmod(
            executable_mode
        )
        stage.write_bytes(trace_path, trace_bytes)
        stage.write_bytes("run-manifest.json", manifest_bytes)
        result_stage = stage.path / str(manifest["output_directory"])
        completed = subprocess.run(
            [
                os.fspath(stage.path.joinpath(*PurePosixPath(executable_path).parts)),
                "--run-manifest",
                os.fspath(stage.path / "run-manifest.json"),
                "--run-result-stage",
                os.fspath(result_stage),
            ],
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            timeout=None,
        )
        if completed.returncode == 5:
            message = completed.stderr.decode("utf-8", errors="replace").strip()
            _failure(5, "ACRUN-PREFLIGHT-001", message or "runtime preflight failed")
        if completed.returncode in (-signal.SIGINT, 130):
            _failure(130, "ACRUN-INTERRUPTED-001", "runtime execution was interrupted")
        if completed.returncode not in (0, 6, 7):
            _failure(6, "ACRUN-RUNTIME-001", "runtime execution failed")
        status, reason = _verify_run_result(
            result_stage,
            manifest_bytes,
            completed.returncode,
            frozenset(runtime_files) - {"run-result.json"},
        )
        for relative in runtime_files:
            source = result_stage / relative
            destination = stage.path / relative
            os.replace(source, destination)
        shutil.rmtree(result_stage)
        stage.commit()
    return RunPublication(
        directory=output_directory,
        manifest=output_directory / "run-manifest.json",
        result=output_directory / "run-result.json",
        status=status,
        termination_reason=reason,
        exit_code=completed.returncode,
    )


def execute_run(publication: BuildPublication, options: RunOptions) -> RunPublication:
    manifest_bytes = create_run_manifest(publication, options)
    build_bytes = _regular_file(publication.manifest, "build manifest")
    build_document = _json_document(build_bytes, "build manifest")
    executable_path, executable_bytes, executable_mode = _build_executable(
        publication.directory, build_document
    )
    if (
        publication.executable.resolve()
        != publication.directory.joinpath(
            *PurePosixPath(executable_path).parts
        ).resolve()
    ):
        _failure(5, "ACRUN-PREFLIGHT-001", "build publication executable disagrees")
    output = options.output_directory.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(
        prefix=".agentic-run-input-", dir=output.parent
    ) as temporary:
        source_root = Path(temporary)
        source_root.joinpath("build-manifest.json").write_bytes(build_bytes)
        executable = source_root.joinpath(*PurePosixPath(executable_path).parts)
        executable.parent.mkdir(parents=True, exist_ok=True)
        executable.write_bytes(executable_bytes)
        executable.chmod(executable_mode)
        source_root.joinpath("trace.json").write_bytes(
            _regular_file(options.trace, "trace")
        )
        return _execute_bundle(
            source_root=source_root,
            manifest_bytes=manifest_bytes,
            output_directory=output,
        )


def replay_run(manifest_path: Path, output_directory: Path) -> RunPublication:
    if manifest_path.is_symlink():
        _failure(5, "ACRUN-PREFLIGHT-001", "run manifest must not be a symlink")
    resolved = manifest_path.resolve()
    manifest_bytes = _regular_file(resolved, "run manifest")
    return _execute_bundle(
        source_root=resolved.parent,
        manifest_bytes=manifest_bytes,
        output_directory=output_directory.resolve(),
    )

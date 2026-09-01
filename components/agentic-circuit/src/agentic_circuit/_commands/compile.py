"""Deterministic logical-stage compilation and artifact publication."""

from __future__ import annotations

from dataclasses import dataclass, replace
from pathlib import Path
from typing import Literal, NoReturn

from .._canonical_json import sha256_bytes
from .._diagnostics import Diagnostic
from .._native_api import NativeRequest, NativeResult, run_native_compiler
from .._output import OutputSink
from .._staging import ArtifactStage
from .._workspace import UserInputError, WorkspaceConfig
from .check import _has_errors, capture


Emit = Literal["acpy", "acir", "frozen-acir", "acsim", "cpp"]

STAGES = (
    "frontend-capture",
    "acpy-construction",
    "acpy-verify",
    "acir-elaboration",
    "process-construction",
    "collection-canonicalization",
    "acir-core",
    "topology-freeze",
    "process-state-lowering",
    "acsim",
    "cxx",
)
EMITS: tuple[Emit, ...] = ("acpy", "acir", "frozen-acir", "acsim", "cpp")

_EMIT_STAGE = {
    "acpy": "acpy-verify",
    "acir": "acir-core",
    "frozen-acir": "topology-freeze",
    "acsim": "acsim",
    "cpp": "cxx",
}
_NATIVE_STOP = {
    "acir-elaboration": "acir-verify",
    "process-construction": "acir-verify",
    "collection-canonicalization": "acir-verify",
    "acir-core": "acir-verify",
    "topology-freeze": "acir-freeze",
    "process-state-lowering": "acsim-lower",
    "acsim": "acsim-verify",
    "cxx": "cxx-contract",
}
_NATIVE_DIAGNOSTIC_STAGE = {
    "acir-parse": "acir-elaboration",
    "acir-verify": "acir-core",
    "acir-normalize": "collection-canonicalization",
    "acir-freeze": "topology-freeze",
    "acsim-lower": "process-state-lowering",
    "acsim-verify": "acsim",
    "cxx-emit": "cxx",
    "cxx-contract": "cxx",
}


@dataclass(frozen=True, slots=True)
class CompileOptions:
    emits: tuple[Emit, ...]
    stop_after: str | None
    dump_before: tuple[str, ...]
    dump_after: tuple[str, ...]
    dump_after_each: bool
    verify_after_each: bool
    pass_pipeline: str | None

    @property
    def final_stage(self) -> str:
        return self.stop_after or "cxx"


def _fail(code: str, message: str) -> NoReturn:
    raise UserInputError(
        Diagnostic(
            stage="compile",
            code=code,
            severity="error",
            message=message,
        )
    )


def _normalize_emits(value: str | None) -> tuple[Emit, ...]:
    raw = value.split(",") if value is not None else list(EMITS)
    if not raw or any(item not in EMITS for item in raw):
        _fail("ACPY-CLI-EMIT", "compile emits must use exact supported names")
    if len(set(raw)) != len(raw):
        _fail("ACPY-CLI-EMIT", "compile emits must not contain duplicates")
    return tuple(raw)  # type: ignore[return-value]


def _normalize_dumps(
    values: object, *, option: str, final_stage: str
) -> tuple[str, ...]:
    if type(values) is not list or not all(type(item) is str for item in values):
        _fail("ACPY-CLI-DUMP", f"{option} values must be logical stage names")
    result = tuple(values)
    if len(set(result)) != len(result):
        _fail("ACPY-CLI-DUMP", f"{option} contains a duplicate stage")
    final_index = STAGES.index(final_stage)
    for stage in result:
        if stage not in STAGES:
            _fail("ACPY-CLI-DUMP", f"unknown logical dump stage: {stage}")
        if STAGES.index(stage) > final_index:
            _fail("ACPY-CLI-DUMP", f"dump stage is beyond --stop-after: {stage}")
    return result


def normalize_options(arguments: object, profile: str) -> CompileOptions:
    emits = _normalize_emits(getattr(arguments, "emit", None))
    stop_after = getattr(arguments, "stop_after", None)
    if stop_after is not None and stop_after not in STAGES:
        _fail("ACPY-CLI-STAGE", f"unknown compile stage: {stop_after}")
    final_stage = stop_after or "cxx"
    final_index = STAGES.index(final_stage)
    for emit in emits:
        if STAGES.index(_EMIT_STAGE[emit]) > final_index:
            _fail("ACPY-CLI-STAGE", f"emit {emit} is beyond --stop-after")

    pass_pipeline = getattr(arguments, "pass_pipeline", None)
    if profile == "custom" and not pass_pipeline:
        _fail("ACPY-CLI-PIPELINE", "custom requires --pass-pipeline")
    if profile != "custom" and pass_pipeline is not None:
        _fail("ACPY-CLI-PIPELINE", "--pass-pipeline requires profile custom")

    return CompileOptions(
        emits=emits,
        stop_after=stop_after,
        dump_before=_normalize_dumps(
            getattr(arguments, "dump_before", []),
            option="--dump-before",
            final_stage=final_stage,
        ),
        dump_after=_normalize_dumps(
            getattr(arguments, "dump_after", []),
            option="--dump-after",
            final_stage=final_stage,
        ),
        dump_after_each=bool(getattr(arguments, "dump_after_each", False)),
        verify_after_each=(
            profile == "validated"
            or bool(getattr(arguments, "verify_after_each", False))
        ),
        pass_pipeline=pass_pipeline,
    )


def _physical_dump_stage(logical: str, *, before: bool) -> str | None:
    if logical == "topology-freeze":
        return "acir-freeze"
    if logical == "process-state-lowering":
        return "acsim-lower"
    if logical == "acsim":
        return "acsim-verify"
    if logical == "cxx":
        return "cxx-emit" if before else "cxx-contract"
    return None


def _unique_physical(stages: tuple[str, ...], *, before: bool) -> tuple[str, ...]:
    result: list[str] = []
    for logical in stages:
        physical = _physical_dump_stage(logical, before=before)
        if physical is not None and physical not in result:
            result.append(physical)
    return tuple(result)


def _after_stages(options: CompileOptions) -> tuple[str, ...]:
    selected = list(options.dump_after)
    if options.dump_after_each:
        final_index = STAGES.index(options.final_stage)
        selected.extend(STAGES[: final_index + 1])
    return tuple(dict.fromkeys(selected))


def _native_request(
    acir: bytes, options: CompileOptions, profile: str,
    binding_registry: bytes = b'{"candidates":[],"requests":[]}',
) -> NativeRequest:
    native_emits: list[str] = []
    if "frozen-acir" in options.emits:
        native_emits.append("frozen-acir")
    if "acsim" in options.emits:
        native_emits.append("acsim")
    if "cpp" in options.emits:
        native_emits.extend(("cpp-header", "cpp-source"))

    native_options: list[tuple[str, object]] = [
        ("profile", profile),
        ("binding_registry", binding_registry),
    ]
    if options.pass_pipeline is not None:
        native_options.append(("custom_pipeline", options.pass_pipeline))
    before = _unique_physical(options.dump_before, before=True)
    after = _unique_physical(_after_stages(options), before=False)
    if before:
        native_options.append(("dump_before", before))
    if after:
        native_options.append(("dump_after", after))
    if options.verify_after_each:
        native_options.append(("verify_after_each", True))
    return NativeRequest(
        acir=acir,
        stop_after=_NATIVE_STOP[options.final_stage],
        emits=tuple(native_emits),
        options=tuple(native_options),
    )


def _logical_diagnostics(diagnostics: tuple[Diagnostic, ...]) -> tuple[Diagnostic, ...]:
    return tuple(
        replace(item, stage=_NATIVE_DIAGNOSTIC_STAGE.get(item.stage, item.stage))
        for item in diagnostics
    )


def _failure_exit(diagnostics: tuple[Diagnostic, ...]) -> int:
    if any(
        item.stage == "cxx" or item.code.startswith(("ACBUILD-", "ACLOWER-CXX"))
        for item in diagnostics
    ):
        return 4
    if all(
        item.code.startswith(("ACPY-", "ACELAB-", "ACIR-", "ACLOWER-"))
        for item in diagnostics
    ):
        return 2
    return 3


def _frontend_dump(logical: str, acpy: bytes, acir: bytes) -> bytes | None:
    index = STAGES.index(logical)
    if index <= STAGES.index("acpy-verify"):
        return acpy
    if index <= STAGES.index("acir-core"):
        return acir
    return None


def _add_dumps(
    artifacts: dict[str, bytes],
    options: CompileOptions,
    acpy: bytes,
    acir: bytes,
    native: NativeResult | None,
) -> None:
    native_artifacts = {
        item.path: item.data for item in native.artifacts
    } if native is not None else {}
    for label, requested, before in (
        ("before", options.dump_before, True),
        ("after", _after_stages(options), False),
    ):
        for logical in requested:
            data = _frontend_dump(logical, acpy, acir)
            physical = _physical_dump_stage(logical, before=before)
            if physical is not None:
                data = native_artifacts.get(f"dumps/{physical}-{label}.mlir")
            if data is None:
                _fail("ACPY-CLI-DUMP", f"compiler produced no dump for {logical}")
            artifacts[f"dumps/{logical}-{label}.mlir"] = data


def _publish(output: Path, artifacts: dict[str, bytes]) -> tuple[str, ...]:
    ordered = tuple(sorted(artifacts))
    if output.is_symlink():
        _fail("ACPY-CLI-OUTPUT", "compile output must not be a symlink")
    if output.exists() and not output.is_dir():
        _fail("ACPY-CLI-OUTPUT", "compile output must be a directory")
    entries = tuple(output.rglob("*")) if output.exists() else ()
    if any(item.is_symlink() for item in entries):
        _fail("ACPY-CLI-OUTPUT", "compile output must not contain symlinks")
    existing = tuple(item for item in entries if not item.is_dir())
    unexpected = sorted(
        item.relative_to(output).as_posix()
        for item in existing
        if item.relative_to(output).as_posix() not in artifacts
    )
    if unexpected:
        _fail("ACPY-CLI-OUTPUT", f"compile output contains stale artifact: {unexpected[0]}")

    with ArtifactStage(output, expected=ordered) as stage:
        for path in ordered:
            stage.write_bytes(path, artifacts[path])
        stage.commit(
            allow_replace=(path for path in ordered if output.joinpath(path).exists())
        )
    return ordered


def run(arguments: object, workspace: WorkspaceConfig, sink: OutputSink) -> int:
    profile = getattr(arguments, "profile", None) or workspace.build_profile
    options = normalize_options(arguments, profile)
    frontend = capture(arguments, workspace)
    if _has_errors(frontend.diagnostics):
        sink.diagnostics(frontend.diagnostics)
        return 2
    if frontend.acpy is None or frontend.acir is None:
        _fail("ACPY-VERIFY-001", "frontend produced incomplete compile artifacts")

    native: NativeResult | None = None
    if STAGES.index(options.final_stage) > STAGES.index("acpy-verify"):
        from .build import _binding_registry

        native = run_native_compiler(
            _native_request(
                frontend.acir,
                options,
                profile,
                _binding_registry(workspace.component_roots),
            )
        )
        diagnostics = _logical_diagnostics(native.diagnostics)
        if _has_errors(diagnostics):
            sink.diagnostics(diagnostics)
            return _failure_exit(diagnostics)

    artifacts: dict[str, bytes] = {}
    if "acpy" in options.emits:
        artifacts["input/model.acpy.json"] = frontend.acpy
    if "acir" in options.emits:
        artifacts["input/model.ac.mlir"] = frontend.acir
    if native is not None:
        for artifact in native.artifacts:
            if not artifact.path.startswith("dumps/"):
                artifacts[artifact.path] = artifact.data
    _add_dumps(artifacts, options, frontend.acpy, frontend.acir, native)

    output_value = getattr(arguments, "output_dir", None)
    output = (
        Path(output_value).resolve()
        if output_value is not None
        else workspace.build_root / "compile"
    )
    ordered = _publish(output, artifacts)
    sink.result(
        {
            "schema": "agentic-circuit-compile-result",
            "version": "0.1",
            "contract_epoch": "0.4",
            "project": workspace.project_name,
            "system": getattr(arguments, "system", None)
            or workspace.default_system,
            "profile": profile,
            "stage": options.final_stage,
            "status": "passed",
            "output_dir": output.as_posix(),
            "artifacts": list(ordered),
            "artifact_sha256": {
                path: sha256_bytes(artifacts[path]) for path in ordered
            },
        },
        human=f"compiled {len(ordered)} artifacts to {output}",
    )
    return 0

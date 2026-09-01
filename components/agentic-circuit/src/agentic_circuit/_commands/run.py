"""End-to-end run and immutable replay command."""

from __future__ import annotations

import re
from pathlib import Path
from typing import NoReturn

from .._diagnostics import Diagnostic
from .._output import OutputSink
from .._run import RunFailure, RunOptions, RunPublication, execute_run, replay_run
from .._workspace import UserInputError, WorkspaceConfig
from .build import build_publication


_DOMAIN = re.compile(r"^[A-Za-z_][A-Za-z0-9_.-]*$")


def _fail(code: str, message: str) -> NoReturn:
    raise UserInputError(
        Diagnostic(stage="runtime", code=code, severity="error", message=message)
    )


def _output(arguments: object) -> Path:
    value = getattr(arguments, "output_dir", None)
    if value is None:
        _fail("ACRUN-INPUT-001", "run requires --output-dir")
    output = Path(value)
    if output.is_symlink():
        _fail("ACRUN-INPUT-001", "run output must not be a symlink")
    return output.resolve()


def _domain_limits(values: list[str]) -> tuple[tuple[str, int], ...]:
    limits: dict[str, int] = {}
    for value in values:
        name, separator, maximum_text = value.partition("=")
        try:
            maximum = int(maximum_text)
        except ValueError:
            maximum = 0
        if (
            not separator
            or not _DOMAIN.fullmatch(name)
            or not 1 <= maximum <= (1 << 64) - 1
            or name in limits
        ):
            _fail(
                "ACRUN-INPUT-001",
                "--max-domain-cycles requires unique DOMAIN=POSITIVE values",
            )
        limits[name] = maximum
    return tuple(sorted(limits.items()))


def _trace(arguments: object, workspace: WorkspaceConfig) -> Path:
    value = getattr(arguments, "trace", None)
    trace = Path(value) if value is not None else workspace.default_trace
    if trace is None:
        _fail("ACRUN-INPUT-001", "run requires --trace or [run].default_trace")
    if trace.is_symlink():
        _fail("ACRUN-INPUT-001", "trace must not be a symlink")
    resolved = trace.resolve()
    if not any(resolved.is_relative_to(root) for root in workspace.trace_roots):
        _fail("ACRUN-INPUT-001", "trace is outside the configured trace roots")
    return resolved


def _options(arguments: object, workspace: WorkspaceConfig) -> RunOptions:
    seed = getattr(arguments, "seed", None)
    return RunOptions(
        trace=_trace(arguments, workspace),
        output_directory=_output(arguments),
        seed=0 if seed is None else seed,
        deadlock_window=getattr(arguments, "deadlock_window", None),
        max_ticks=getattr(arguments, "max_ticks", None),
        max_domain_cycles=_domain_limits(
            list(getattr(arguments, "max_domain_cycles", ()))
        ),
        stats_format=getattr(arguments, "stats_format", None) or "json",
        event_log=getattr(arguments, "event_log", None) or "disabled",
        termination_kind=(
            "complete" if getattr(arguments, "expect_termination", False) else "any"
        ),
    )


def _result(publication: RunPublication) -> dict[str, object]:
    return {
        "schema": "agentic-circuit-run-command-result",
        "version": "0.1",
        "contract_epoch": "0.4",
        "status": publication.status,
        "termination_reason": publication.termination_reason,
        "directory": publication.directory.as_posix(),
        "manifest": publication.manifest.as_posix(),
        "result": publication.result.as_posix(),
    }


def _reject_replay_overrides(arguments: object) -> None:
    overrides = (
        ("architecture", getattr(arguments, "architecture", None)),
        ("--trace", getattr(arguments, "trace", None)),
        ("--seed", getattr(arguments, "seed", None)),
        ("--deadlock-window", getattr(arguments, "deadlock_window", None)),
        ("--max-ticks", getattr(arguments, "max_ticks", None)),
        ("--max-domain-cycles", getattr(arguments, "max_domain_cycles", [])),
        ("--stats-format", getattr(arguments, "stats_format", None)),
        ("--event-log", getattr(arguments, "event_log", None)),
        ("--expect-termination", getattr(arguments, "expect_termination", False)),
        ("--jobs", getattr(arguments, "jobs", None)),
        ("--project", getattr(arguments, "project", None)),
        ("--system", getattr(arguments, "system", None)),
    )
    for name, value in overrides:
        if value not in (None, False, []):
            _fail("ACRUN-REPLAY-001", f"replay rejects ambient override {name}")


def run(
    arguments: object,
    workspace: WorkspaceConfig | None,
    sink: OutputSink,
) -> int:
    try:
        replay = getattr(arguments, "replay_manifest", None)
        if replay is not None:
            _reject_replay_overrides(arguments)
            publication = replay_run(Path(replay), _output(arguments))
        else:
            if workspace is None:
                raise RuntimeError("normal run requires a workspace")
            options = _options(arguments, workspace)
            system = getattr(arguments, "system", None) or workspace.default_system
            attempt = build_publication(
                arguments,
                workspace,
                output=workspace.build_root / system,
            )
            if attempt.publication is None:
                sink.diagnostics(attempt.diagnostics)
                return attempt.exit_code
            publication = execute_run(attempt.publication, options)
        sink.result(
            _result(publication),
            human=(
                f"run {publication.status}: {publication.termination_reason}"
            ),
        )
        return publication.exit_code
    except RunFailure as error:
        sink.diagnostics((error.diagnostic,))
        return error.exit_code

"""Process-level C3 publication recovery through the public compiler driver."""

from __future__ import annotations

import json
import os
import select
import shutil
import signal
import subprocess
import sys
from pathlib import Path

import pytest

pytestmark = [
    pytest.mark.system,
    pytest.mark.skipif(os.name == "nt", reason="SIGKILL/flock profile is POSIX-only"),
]

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "tests/integration/agentic-circuit/m6-publication"
NATIVE_DEFAULT = ROOT / ".pycircuit_out/m5-root"
PUBLICATION_POINTS = (
    "after_journal_preparing",
    "after_stage_complete",
    "after_journal_prepared",
    "after_previous_saved",
    "after_destination_installed",
    "after_journal_committed",
)
FIRST_PUBLISH_POINTS = tuple(
    point for point in PUBLICATION_POINTS if point != "after_previous_saved"
)


def _environment() -> dict[str, str]:
    native = Path(os.environ.get("PYCIRCUIT_NATIVE_BUILD", NATIVE_DEFAULT)).resolve()
    environment = os.environ.copy()
    environment.update(
        {
            "PYCIRCUIT_NATIVE_BUILD": str(native),
            "ACIR_SOURCE_UNIT_HARNESS": str(native / "bin/acir-source-unit-harness"),
            "ACIR_DESIGN_HARNESS": str(native / "bin/acir-design-harness"),
            "ACIR_CPP_SOURCE_PARTS_HARNESS": str(
                native / "bin/acir-cpp-source-parts-harness"
            ),
        }
    )
    package_root = str(ROOT / "python/pycircuit/src")
    environment["PYTHONPATH"] = os.pathsep.join(
        item for item in (package_root, environment.get("PYTHONPATH", "")) if item
    )
    for variable, path in (
        ("ACIR_SOURCE_UNIT_HARNESS", native / "bin/acir-source-unit-harness"),
        ("ACIR_DESIGN_HARNESS", native / "bin/acir-design-harness"),
        (
            "ACIR_CPP_SOURCE_PARTS_HARNESS",
            native / "bin/acir-cpp-source-parts-harness",
        ),
    ):
        assert path.is_file(), f"{variable} is missing from current build: {path}"
    return environment


def _checked(
    command: list[str],
    *,
    env: dict[str, str],
    cwd: Path,
    timeout: int = 900,
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        cwd=cwd,
        env=env,
        text=True,
        capture_output=True,
        check=False,
        timeout=timeout,
    )
    assert result.returncode == 0, (
        f"command failed ({result.returncode}): {command!r}\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )
    return result


def _cli(*arguments: str) -> list[str]:
    return [sys.executable, "-m", "pycircuit.cli", *arguments]


def _runner(*arguments: str) -> list[str]:
    return [sys.executable, str(FIXTURE / "crash_runner.py"), *arguments]


def _public_arguments(command: list[str]) -> list[str]:
    assert command[:3] == [sys.executable, "-m", "pycircuit.cli"]
    return command[3:]


def _replace(command: list[str]) -> list[str]:
    return command if "--replace" in command else [*command, "--replace"]


def _compile(source: Path, destination: Path, *, replace: bool = False) -> list[str]:
    destination.parent.mkdir(parents=True, exist_ok=True)
    command = _cli(
        "compile",
        "-c",
        str(source / "root.py"),
        "--source-root",
        str(source),
        "--package-prefix",
        "m6",
        "-o",
        str(destination),
    )
    return command + (["--replace"] if replace else [])


def _link(unit: Path, destination: Path, *, replace: bool = False) -> list[str]:
    destination.parent.mkdir(parents=True, exist_ok=True)
    command = _cli(
        "link",
        str(unit),
        "--top",
        "m6.root.Root",
        "-o",
        str(destination),
    )
    return command + (["--replace"] if replace else [])


def _emit(
    program: Path,
    target: str,
    destination: Path,
    *,
    replace: bool = False,
) -> list[str]:
    destination.parent.mkdir(parents=True, exist_ok=True)
    command = _cli(
        "emit",
        str(program),
        "--target",
        target,
        "-o",
        str(destination),
    )
    return command + (["--replace"] if replace else [])


def _snapshot(path: Path) -> dict[str, bytes] | None:
    if path.is_file():
        return {"$file": path.read_bytes()}
    if not path.is_dir():
        return None
    return {
        child.relative_to(path).as_posix(): child.read_bytes()
        for child in sorted(path.rglob("*"))
        if child.is_file()
    }


def _control(destination: Path) -> Path:
    return destination.parent / f".{destination.name}.pycircuit-publication"


def _lock_identity(destination: Path) -> tuple[int, int]:
    lock = _control(destination) / "lock"
    stat = lock.stat()
    return stat.st_dev, stat.st_ino


def _journal(destination: Path) -> dict[str, object]:
    return json.loads(
        (_control(destination) / "journal.json").read_text(encoding="utf-8")
    )


def _assert_control_clean(destination: Path, lock_identity: tuple[int, int]) -> None:
    assert {path.name for path in _control(destination).iterdir()} == {
        "owner.json",
        "lock",
    }
    assert _lock_identity(destination) == lock_identity


def _crash_at(
    command: list[str], point: str, *, env: dict[str, str], cwd: Path
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        _runner("--crash-at", point, *_public_arguments(command)),
        cwd=cwd,
        env=env,
        text=True,
        capture_output=True,
        check=False,
        timeout=900,
    )
    assert result.returncode == -signal.SIGKILL, (
        f"fault point {point} did not self-SIGKILL (status {result.returncode})\n"
        f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
    )
    return result


def _recovery_paths(destination: Path) -> tuple[Path, Path, Path]:
    control = _control(destination)
    return control / "stage", control / "previous", control / "journal.json"


def _assert_replacement_crash_shape(
    destination: Path,
    *,
    artifact: str,
    target: str | None,
    point: str,
    old_bytes: dict[str, bytes],
    new_bytes: dict[str, bytes],
    lock_identity: tuple[int, int],
) -> None:
    journal = _journal(destination)
    assert journal["kind"] == "pycircuit-publication"
    assert journal["artifact"] == artifact
    assert journal["destination"] == destination.name
    assert journal["had_previous"] is True
    if artifact == "generated":
        assert journal["owner"]["target"] == target

    stage, previous, _journal_path = _recovery_paths(destination)
    if point in {"after_journal_preparing", "after_stage_complete"}:
        assert journal["phase"] == "preparing"
    elif point == "after_journal_committed":
        assert journal["phase"] == "committed"
    else:
        assert journal["phase"] == "prepared"

    if point in {
        "after_journal_preparing",
        "after_stage_complete",
        "after_journal_prepared",
    }:
        assert _snapshot(destination) == old_bytes
        assert not previous.exists()
        assert _snapshot(stage) == (
            new_bytes if point != "after_journal_preparing" else None
        )
    elif point == "after_previous_saved":
        assert not destination.exists()
        assert _snapshot(previous) == old_bytes
        assert _snapshot(stage) == new_bytes
    else:
        assert _snapshot(destination) == new_bytes
        assert _snapshot(previous) == old_bytes
        assert not stage.exists()
    assert _lock_identity(destination) == lock_identity


def _prepare_snapshots(tmp_path: Path, env: dict[str, str]) -> dict[str, object]:
    sources: dict[str, Path] = {}
    for revision in ("old", "new"):
        sources[revision] = tmp_path / f"{revision}-source"
        shutil.copytree(FIXTURE / revision, sources[revision])

    units: dict[str, Path] = {}
    programs: dict[str, Path] = {}
    generated: dict[tuple[str, str], Path] = {}
    for revision in ("old", "new"):
        units[revision] = tmp_path / f"baseline-{revision}-unit"
        _checked(_compile(sources[revision], units[revision]), env=env, cwd=tmp_path)
        programs[revision] = tmp_path / f"baseline-{revision}.ac"
        _checked(_link(units[revision], programs[revision]), env=env, cwd=tmp_path)
        for target in ("cpp", "verilog"):
            generated[(revision, target)] = tmp_path / f"baseline-{revision}-{target}"
            _checked(
                _emit(programs[revision], target, generated[(revision, target)]),
                env=env,
                cwd=tmp_path,
            )

    return {
        "sources": sources,
        "units": units,
        "programs": programs,
        "generated": generated,
        "unit_bytes": {key: _snapshot(value) for key, value in units.items()},
        "program_bytes": {key: _snapshot(value) for key, value in programs.items()},
        "generated_bytes": {key: _snapshot(value) for key, value in generated.items()},
    }


def test_public_compile_link_and_both_emitters_recover_replacement_process_crashes(
    tmp_path: Path,
) -> None:
    env = _environment()
    state = _prepare_snapshots(tmp_path, env)
    sources = state["sources"]
    units = state["units"]
    programs = state["programs"]
    expected_programs = state["program_bytes"]
    expected_generated = state["generated_bytes"]

    for operation in ("compile", "link", "emit-cpp", "emit-verilog"):
        artifact = (
            "source-unit"
            if operation == "compile"
            else ("program" if operation == "link" else "generated")
        )
        target = (
            "cpp"
            if operation == "emit-cpp"
            else ("verilog" if operation == "emit-verilog" else None)
        )
        destination = tmp_path / f"interrupted-{operation}"
        if operation == "compile":
            old_command = _compile(sources["old"], destination)
            new_command = _compile(sources["new"], destination, replace=True)
        elif operation == "link":
            old_command = _link(units["old"], destination)
            new_command = _link(units["new"], destination, replace=True)
        else:
            assert target is not None
            old_command = _emit(programs["old"], target, destination)
            new_command = _emit(programs["new"], target, destination, replace=True)
        for point in PUBLICATION_POINTS:
            if destination.exists():
                _checked(_replace(old_command), env=env, cwd=tmp_path)
            else:
                _checked(old_command, env=env, cwd=tmp_path)
            old_bytes = _snapshot(destination)
            assert old_bytes is not None
            _checked(new_command, env=env, cwd=tmp_path)
            new_bytes = _snapshot(destination)
            assert new_bytes is not None
            _checked(_replace(old_command), env=env, cwd=tmp_path)
            assert _snapshot(destination) == old_bytes
            lock_identity = _lock_identity(destination)
            _crash_at(new_command, point, env=env, cwd=tmp_path)
            _assert_replacement_crash_shape(
                destination,
                artifact=artifact,
                target=target,
                point=point,
                old_bytes=old_bytes,
                new_bytes=new_bytes,
                lock_identity=lock_identity,
            )

            committed = point == "after_journal_committed"
            expected_revision = "new" if committed else "old"
            if operation == "compile":
                recovery_output = tmp_path / f"recovered-final-{point}"
                recovery = _link(destination, recovery_output)
                _checked(recovery, env=env, cwd=tmp_path)
                assert _snapshot(destination) == (new_bytes if committed else old_bytes)
                assert (
                    _snapshot(recovery_output) == expected_programs[expected_revision]
                )
            elif operation == "link":
                assert target is None
                recovery_output = tmp_path / f"recovered-link-cpp-{point}"
                recovery = _emit(destination, "cpp", recovery_output)
                _checked(recovery, env=env, cwd=tmp_path)
                assert _snapshot(destination) == (new_bytes if committed else old_bytes)
                assert (
                    _snapshot(recovery_output)
                    == expected_generated[(expected_revision, "cpp")]
                )
            else:
                assert target is not None
                source_program = programs[expected_revision]
                recovery = _emit(source_program, target, destination, replace=True)
                _checked(recovery, env=env, cwd=tmp_path)
                assert _snapshot(destination) == (new_bytes if committed else old_bytes)

            if committed and operation in {"compile", "link"}:
                assert _journal(destination)["phase"] == "committed"
                assert _snapshot(destination) == new_bytes
                assert _snapshot(_control(destination) / "previous") == old_bytes
                # A public reader may leave a valid committed journal and
                # previous backup in place. A later public writer completes it.
                _checked(_replace(old_command), env=env, cwd=tmp_path)
            _assert_control_clean(destination, lock_identity)


def test_first_source_unit_publication_restores_absence_before_commit(
    tmp_path: Path,
) -> None:
    """Bounded first-publish matrix: source-unit compile covers five reachable points."""
    env = _environment()
    state = _prepare_snapshots(tmp_path, env)
    new_source = state["sources"]["new"]
    expected_unit = state["unit_bytes"]["new"]
    expected_program = state["program_bytes"]["new"]
    assert expected_unit is not None and expected_program is not None

    for point in FIRST_PUBLISH_POINTS:
        destination = tmp_path / f"first-publish-{point}"
        command = _compile(new_source, destination)
        _checked(command, env=env, cwd=tmp_path)
        expected_unit = _snapshot(destination)
        assert expected_unit is not None
        shutil.rmtree(destination)
        shutil.rmtree(_control(destination))
        _crash_at(command, point, env=env, cwd=tmp_path)
        journal = _journal(destination)
        assert journal["artifact"] == "source-unit"
        assert journal["had_previous"] is False
        stage, previous, _journal_path = _recovery_paths(destination)

        if point in {"after_journal_preparing", "after_stage_complete"}:
            assert journal["phase"] == "preparing"
        elif point == "after_journal_committed":
            assert journal["phase"] == "committed"
        else:
            assert journal["phase"] == "prepared"
        assert _snapshot(destination) == (
            expected_unit
            if point in {"after_destination_installed", "after_journal_committed"}
            else None
        )
        assert _snapshot(stage) == (
            expected_unit
            if point in {"after_stage_complete", "after_journal_prepared"}
            else None
        )
        assert not previous.exists()

        lock_identity = _lock_identity(destination)
        consumer_output = tmp_path / f"first-publish-consumer-{point}.ac"
        consumer = _link(destination, consumer_output)
        result = subprocess.run(
            consumer,
            cwd=tmp_path,
            env=env,
            text=True,
            capture_output=True,
            check=False,
            timeout=900,
        )
        if point == "after_journal_committed":
            assert result.returncode == 0, result.stdout + result.stderr
            assert _snapshot(destination) == expected_unit
            assert _snapshot(consumer_output) == expected_program
        else:
            assert (
                result.returncode != 0
            ), "link must reject the rolled-back absent unit"
            assert _snapshot(destination) is None
            assert _snapshot(consumer_output) is None
        if point == "after_journal_committed":
            assert _journal(destination)["phase"] == "committed"
            assert not (_control(destination) / "previous").exists()
            _checked(_replace(command), env=env, cwd=tmp_path)
        _assert_control_clean(destination, lock_identity)
        shutil.rmtree(_control(destination))


def test_public_recovery_itself_survives_three_process_interruptions(
    tmp_path: Path,
) -> None:
    env = _environment()
    state = _prepare_snapshots(tmp_path, env)
    sources = state["sources"]
    destination = tmp_path / "reentrant-unit"
    _checked(_compile(sources["old"], destination), env=env, cwd=tmp_path)
    expected_old = _snapshot(destination)
    assert expected_old is not None
    _checked(_compile(sources["new"], destination, replace=True), env=env, cwd=tmp_path)
    expected_new = _snapshot(destination)
    assert expected_new is not None
    _checked(_compile(sources["old"], destination, replace=True), env=env, cwd=tmp_path)
    assert _snapshot(destination) == expected_old
    lock_identity = _lock_identity(destination)
    _crash_at(
        _compile(sources["new"], destination, replace=True),
        "after_destination_installed",
        env=env,
        cwd=tmp_path,
    )

    consumer_output = tmp_path / "reentrant-consumer.ac"
    link_command = _link(destination, consumer_output)
    checkpoints = (
        (
            "after_journal_rollback_restore",
            "rollback_restore",
            expected_new,
            expected_old,
            None,
        ),
        (
            "after_rename:reentrant-unit:stage",
            "rollback_restore",
            None,
            expected_old,
            expected_new,
        ),
        (
            "after_journal_rollback_cleanup",
            "rollback_cleanup",
            expected_old,
            None,
            expected_new,
        ),
    )
    for (
        recovery_point,
        phase,
        destination_state,
        previous_state,
        stage_state,
    ) in checkpoints:
        _crash_at(link_command, recovery_point, env=env, cwd=tmp_path)
        journal = _journal(destination)
        assert journal["phase"] == phase
        stage, previous, _journal_path = _recovery_paths(destination)
        assert _snapshot(destination) == destination_state
        assert _snapshot(previous) == previous_state
        assert _snapshot(stage) == stage_state
        assert _lock_identity(destination) == lock_identity

    _checked(link_command, env=env, cwd=tmp_path)
    assert _snapshot(destination) == expected_old
    assert _snapshot(consumer_output) == state["program_bytes"]["old"]
    _assert_control_clean(destination, lock_identity)


def _read_line(process: subprocess.Popen[bytes], timeout: float) -> bytes:
    assert process.stdout is not None
    readable, _writable, _exceptional = select.select([process.stdout], [], [], timeout)
    assert readable, f"timed out waiting for process marker; status={process.poll()}"
    line = process.stdout.readline()
    assert line, f"process closed marker pipe; status={process.poll()}"
    return line


def test_killed_writer_releases_lock_to_a_waiting_public_reader(tmp_path: Path) -> None:
    import fcntl

    env = _environment()
    state = _prepare_snapshots(tmp_path, env)
    sources = state["sources"]
    expected_old_program = state["program_bytes"]["old"]
    assert expected_old_program is not None

    destination = tmp_path / "locked-unit"
    _checked(_compile(sources["old"], destination), env=env, cwd=tmp_path)
    expected_old = _snapshot(destination)
    assert expected_old is not None
    _checked(_compile(sources["new"], destination, replace=True), env=env, cwd=tmp_path)
    expected_new = _snapshot(destination)
    assert expected_new is not None
    _checked(_compile(sources["old"], destination, replace=True), env=env, cwd=tmp_path)
    assert _snapshot(destination) == expected_old
    lock_identity = _lock_identity(destination)
    writer = subprocess.Popen(
        _runner(
            "--pause-at",
            "after_journal_prepared",
            *_public_arguments(_compile(sources["new"], destination, replace=True)),
        ),
        cwd=tmp_path,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert _read_line(writer, 30).startswith(b"paused-at:after_journal_prepared")

    lock = _control(destination) / "lock"
    with lock.open("rb") as stream:
        try:
            fcntl.flock(stream.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            held = True
        else:
            held = False
            fcntl.flock(stream.fileno(), fcntl.LOCK_UN)
    assert held, "the paused public writer must hold the publication lock"

    consumer_output = tmp_path / "locked-reader.ac"
    reader = subprocess.Popen(
        _runner(
            "--trace-lock",
            str(lock),
            *_public_arguments(_link(destination, consumer_output)),
        ),
        cwd=tmp_path,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert _read_line(reader, 30) == b"attempt:shared\n"
    assert reader.poll() is None

    writer.kill()
    _writer_stdout, writer_stderr = writer.communicate(timeout=30)
    assert writer.returncode == -signal.SIGKILL, writer_stderr.decode(errors="replace")
    reader_stdout, reader_stderr = reader.communicate(timeout=120)
    assert reader.returncode == 0, reader_stdout.decode(
        errors="replace"
    ) + reader_stderr.decode(errors="replace")
    assert b"acquired:shared\n" in reader_stdout
    assert _snapshot(destination) == expected_old
    assert _snapshot(consumer_output) == expected_old_program
    assert _lock_identity(destination) == lock_identity
    assert _snapshot(destination) != expected_new

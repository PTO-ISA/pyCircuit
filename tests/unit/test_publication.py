"""Artifact publication is recoverable and preserves owned outputs."""

from __future__ import annotations

import json
import multiprocessing
import queue
from collections.abc import Mapping
from multiprocessing.queues import Queue
from pathlib import Path

import pytest
from pycircuit._publication import (
    _publication_lock_set,
    _publication_owner_generated,
    _publication_owner_program,
    _publication_owner_source_unit,
    _PublicationCancelledError,
    _PublicationError,
    _PublicationFileSystemError,
    _PublicationInput,
    _PublicationOutput,
    _publish_directory,
    _publish_file,
    _read_published,
    _recover_publication,
)
from pycircuit._publication_fs import _PublicationFileSystem

pytestmark = pytest.mark.unit


class _Crash(BaseException):
    pass


class _Fault:
    def __init__(
        self, point: str, exception: type[BaseException] = _Crash, *, once: bool = True
    ) -> None:
        self.point = point
        self.exception = exception
        self.once = once
        self.hit = False

    def __call__(self, point: str) -> None:
        if point == self.point and (not self.once or not self.hit):
            self.hit = True
            raise self.exception(f"injected fault at {point}")


class _RecordingFileSystem(_PublicationFileSystem):
    def __init__(self) -> None:
        super().__init__()
        self.lock_order: list[tuple[Path, bool]] = []

    def lock(self, path: Path, *, shared: bool):
        self.lock_order.append((path, shared))
        return super().lock(path, shared=shared)


OWNER = _publication_owner_source_unit(package="demo", path="leaf.py")
PROGRAM_OWNER = _publication_owner_program(
    package="demo", path="leaf.py", definition='@"demo.leaf.Leaf"'
)


def _builder(content: str):
    def build(stage: Path) -> None:
        (stage / "artifact.json").write_text(
            json.dumps({"owner": OWNER, "content": content}), encoding="utf-8"
        )

    return build


def _validate(path: Path, owner: Mapping[str, object]) -> None:
    assert {item.name for item in path.iterdir()} == {"artifact.json"}
    receipt = json.loads((path / "artifact.json").read_text(encoding="utf-8"))
    assert receipt["owner"] == owner
    assert type(receipt["content"]) is str


def _content(path: Path) -> str:
    return json.loads((path / "artifact.json").read_text(encoding="utf-8"))["content"]


def _program_builder(content: str):
    def build(stage: Path) -> None:
        stage.write_text(
            json.dumps({"owner": PROGRAM_OWNER, "content": content}),
            encoding="utf-8",
        )

    return build


def _validate_program(path: Path, owner: Mapping[str, object]) -> None:
    value = json.loads(path.read_text(encoding="utf-8"))
    assert value["owner"] == owner
    assert type(value["content"]) is str


def _program_content(path: Path) -> str:
    return json.loads(path.read_text(encoding="utf-8"))["content"]


def _publish_program(path: Path, content: str, **kwargs: object):
    return _publish_file(
        path,
        owner=PROGRAM_OWNER,
        build=_program_builder(content),
        validate=_validate_program,
        **kwargs,
    )


def _process_replace(
    destination: str,
    ready: Queue,
    done: Queue,
) -> None:
    ready.put("ready")
    try:
        _publish(Path(destination), "child", replace=True)
    except BaseException as error:
        done.put(("error", repr(error)))
    else:
        done.put(("done", "child"))


def _publish(path: Path, content: str, **kwargs: object):
    return _publish_directory(
        path,
        owner=OWNER,
        build=_builder(content),
        validate=_validate,
        **kwargs,
    )


def _control(path: Path) -> Path:
    return path.parent / f".{path.name}.pycircuit-publication"


def test_first_publish_uses_only_fixed_control_names(tmp_path: Path) -> None:
    destination = tmp_path / "unit"

    result = _publish(destination, "first")

    assert result.committed
    assert not result.cleanup_pending
    assert _content(destination) == "first"
    control = _control(destination)
    assert {path.name for path in control.iterdir()} == {"owner.json", "lock"}
    assert json.loads((control / "owner.json").read_text(encoding="utf-8")) == {
        "kind": "pycircuit-publication-control",
        "destination": "unit",
    }


@pytest.mark.parametrize("point", ["after_create_lock", "after_flush:owner.json.tmp"])
def test_bootstrap_reenters_fixed_partial_states(tmp_path: Path, point: str) -> None:
    destination = tmp_path / "unit"

    with pytest.raises(_Crash):
        _publish(
            destination,
            "first",
            filesystem=_PublicationFileSystem(_Fault(point)),
        )

    result = _publish(destination, "first")
    assert result.committed
    assert _content(destination) == "first"


def test_initialized_control_never_recreates_missing_lock(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "first")
    (_control(destination) / "lock").unlink()

    with pytest.raises(_PublicationError, match="no valid lock"):
        _recover_publication(destination, validate=_validate)

    assert not (_control(destination) / "lock").exists()
    assert _content(destination) == "first"


def test_single_file_program_uses_the_same_first_publish_and_replace_protocol(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "program.ac"

    first = _publish_program(destination, "first")
    second = _publish_program(destination, "second", replace=True)

    assert first.committed and second.committed
    assert destination.is_file()
    assert _program_content(destination) == "second"
    assert {path.name for path in _control(destination).iterdir()} == {
        "owner.json",
        "lock",
    }


@pytest.mark.parametrize(
    "point",
    [
        "after_journal_preparing",
        "after_stage_complete",
        "after_journal_prepared",
        "after_previous_saved",
        "after_destination_installed",
    ],
)
def test_single_file_program_restores_old_file_before_commit(
    tmp_path: Path, point: str
) -> None:
    destination = tmp_path / "program.ac"
    _publish_program(destination, "old")

    with pytest.raises(_Crash):
        _publish_program(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault(point)),
        )

    _recover_publication(destination, validate=_validate_program)
    assert _program_content(destination) == "old"


def test_single_file_committed_interruption_keeps_new_program(tmp_path: Path) -> None:
    destination = tmp_path / "program.ac"
    _publish_program(destination, "old")

    with pytest.raises(_Crash):
        _publish_program(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_committed")),
        )

    result = _recover_publication(destination, validate=_validate_program)
    assert result.committed
    assert _program_content(destination) == "new"


def test_single_file_cleanup_pending_keeps_committed_program(tmp_path: Path) -> None:
    destination = tmp_path / "program.ac"
    _publish_program(destination, "old")

    result = _publish_program(
        destination,
        "new",
        replace=True,
        filesystem=_PublicationFileSystem(_Fault("before_unlink:previous", OSError)),
    )

    assert result.cleanup_pending
    assert _program_content(destination) == "new"
    assert (_control(destination) / "previous").is_file()
    assert (_control(destination) / "journal.json").is_file()


def test_replace_requires_flag_and_preserves_old_output(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "first")

    with pytest.raises(_PublicationError, match="already exists"):
        _publish(destination, "second")

    assert _content(destination) == "first"
    result = _publish(destination, "second", replace=True)
    assert result.committed
    assert _content(destination) == "second"


def test_replace_rejects_a_different_publication_owner(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "first")
    other = _publication_owner_generated(
        package="demo", path="leaf.py", definition='@"demo.leaf.Leaf"', target="cpp"
    )

    with pytest.raises(_PublicationError, match="validation failed"):
        _publish_directory(
            destination,
            owner=other,
            build=_builder("second"),
            validate=_validate,
            replace=True,
        )

    assert _content(destination) == "first"


@pytest.mark.parametrize(
    "point",
    [
        "after_journal_preparing",
        "after_stage_complete",
        "after_journal_prepared",
        "after_destination_installed",
    ],
)
def test_first_publish_recovers_every_precommit_interruption(
    tmp_path: Path, point: str
) -> None:
    destination = tmp_path / "unit"
    fault = _Fault(point)

    with pytest.raises(_Crash):
        _publish(
            destination,
            "first",
            filesystem=_PublicationFileSystem(fault),
        )

    result = _recover_publication(destination, validate=_validate)
    assert not result.committed
    assert not destination.exists()
    assert {path.name for path in _control(destination).iterdir()} == {
        "owner.json",
        "lock",
    }


@pytest.mark.parametrize(
    "point",
    [
        "after_journal_preparing",
        "after_stage_complete",
        "after_journal_prepared",
        "after_previous_saved",
        "after_destination_installed",
    ],
)
def test_replace_recovers_old_output_before_commit(tmp_path: Path, point: str) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")

    with pytest.raises(_Crash):
        _publish(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault(point)),
        )

    _recover_publication(destination, validate=_validate)
    assert _content(destination) == "old"


def test_committed_interruption_keeps_new_output_and_finishes_cleanup(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")

    with pytest.raises(_Crash):
        _publish(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_committed")),
        )

    assert _content(destination) == "new"
    assert (_control(destination) / "journal.json").is_file()
    result = _recover_publication(destination, validate=_validate)
    assert result.committed
    assert _content(destination) == "new"
    assert {path.name for path in _control(destination).iterdir()} == {
        "owner.json",
        "lock",
    }


@pytest.mark.parametrize(
    ("initial_point", "recovery_point"),
    [
        ("after_journal_prepared", "after_journal_rollback_restore"),
        ("after_destination_installed", "after_rename:previous:unit"),
        ("after_journal_prepared", "after_rmdir_tree:stage"),
    ],
)
def test_precommit_recovery_is_itself_reentrant(
    tmp_path: Path, initial_point: str, recovery_point: str
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    with pytest.raises(_Crash):
        _publish(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault(initial_point)),
        )

    with pytest.raises(_Crash):
        _recover_publication(
            destination,
            validate=_validate,
            filesystem=_PublicationFileSystem(_Fault(recovery_point)),
        )

    _recover_publication(destination, validate=_validate)
    assert _content(destination) == "old"


def test_committed_cleanup_is_reentrant_after_cleanup_interruption(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    with pytest.raises(_Crash):
        _publish(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_committed")),
        )

    with pytest.raises(_Crash):
        _recover_publication(
            destination,
            validate=_validate,
            filesystem=_PublicationFileSystem(_Fault("after_rmdir_tree:previous")),
        )

    result = _recover_publication(destination, validate=_validate)
    assert result.committed
    assert _content(destination) == "new"


def test_prepared_impossible_path_combination_is_rejected(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    with pytest.raises(_Crash):
        _publish(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_prepared")),
        )
    previous = _control(destination) / "previous"
    previous.mkdir()
    _builder("impossible")(previous)

    with pytest.raises(_PublicationError, match="path combination is invalid"):
        _recover_publication(destination, validate=_validate)

    assert _content(destination) == "old"
    assert (_control(destination) / "journal.json").is_file()


def test_committed_missing_destination_is_recovery_error(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    with pytest.raises(_Crash):
        _publish(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_committed")),
        )
    (destination / "artifact.json").unlink()
    destination.rmdir()

    with pytest.raises(_PublicationError, match="artifact has type None"):
        _recover_publication(destination, validate=_validate)

    assert (_control(destination) / "journal.json").is_file()


@pytest.mark.parametrize(
    "point",
    [
        "before_prepare",
        "after_preparing",
        "after_stage",
        "after_prepared",
        "after_previous_saved",
        "after_destination_installed",
    ],
)
def test_precommit_cancellation_rolls_back_and_reports_exit_130(
    tmp_path: Path, point: str
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")

    with pytest.raises(_PublicationCancelledError) as cancelled:
        _publish(
            destination,
            "new",
            replace=True,
            cancelled=lambda current: current == point,
        )

    assert cancelled.value.exit_code == 130
    assert _content(destination) == "old"


def test_postcommit_cancellation_is_committed_success(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")

    result = _publish(
        destination,
        "new",
        replace=True,
        cancelled=lambda point: point == "after_committed",
    )

    assert result.committed
    assert _content(destination) == "new"


def test_observable_builder_failure_restores_old_output(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")

    def fail(_stage: Path) -> None:
        raise RuntimeError("generation failed")

    with pytest.raises(RuntimeError, match="generation failed"):
        _publish_directory(
            destination,
            owner=OWNER,
            build=fail,
            validate=_validate,
            replace=True,
        )

    assert _content(destination) == "old"
    assert {path.name for path in _control(destination).iterdir()} == {
        "owner.json",
        "lock",
    }


def test_cleanup_failure_after_commit_warns_and_reader_can_use_new_output(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    result = _publish(
        destination,
        "new",
        replace=True,
        filesystem=_PublicationFileSystem(
            _Fault("before_rmdir_tree:previous", OSError)
        ),
    )

    assert result.committed
    assert result.warnings == ("publication_cleanup_pending",)
    assert _content(destination) == "new"
    assert (_control(destination) / "journal.json").is_file()
    read = _read_published(
        destination,
        owner=OWNER,
        stable_validate=_validate,
        recovery_validate=_validate,
        read=_content,
    )
    assert read == "new"
    assert (_control(destination) / "previous").is_dir()
    assert (_control(destination) / "journal.json").is_file()


def test_cleanup_pending_read_runs_full_validation_before_stable_projection(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    _publish(
        destination,
        "new",
        replace=True,
        filesystem=_PublicationFileSystem(
            _Fault("before_rmdir_tree:previous", OSError)
        ),
    )
    calls: list[str] = []

    def full(path: Path, owner: Mapping[str, object]) -> None:
        calls.append("full")
        _validate(path, owner)

    def stable(path: Path, owner: Mapping[str, object]) -> None:
        calls.append("stable")
        receipt = json.loads((path / "artifact.json").read_text(encoding="utf-8"))
        assert receipt["owner"] == owner

    value = _read_published(
        destination,
        owner=OWNER,
        stable_validate=stable,
        recovery_validate=full,
        read=_content,
    )

    assert value == "new"
    assert calls == ["full", "stable"]
    assert (_control(destination) / "journal.json").is_file()


def test_cleanup_pending_lock_set_runs_full_before_stable(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    _publish(
        destination,
        "new",
        replace=True,
        filesystem=_PublicationFileSystem(
            _Fault("before_rmdir_tree:previous", OSError)
        ),
    )
    calls: list[str] = []

    def full(path: Path, owner: Mapping[str, object]) -> None:
        calls.append("full")
        _validate(path, owner)

    def stable(path: Path, owner: Mapping[str, object]) -> None:
        calls.append("stable")
        assert (
            json.loads((path / "artifact.json").read_text(encoding="utf-8"))["owner"]
            == owner
        )

    request = _PublicationInput(destination, OWNER, stable, full)
    with _publication_lock_set(inputs=[request]) as locks:
        assert locks.snapshot(destination, _content) == "new"

    assert calls == ["full", "stable"]


def test_error_after_journal_unlink_does_not_report_cleanup_pending(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")

    result = _publish(
        destination,
        "new",
        replace=True,
        filesystem=_PublicationFileSystem(_Fault("after_unlink:journal.json", OSError)),
    )

    assert result.committed
    assert not result.cleanup_pending
    assert not (_control(destination) / "journal.json").exists()
    assert _content(destination) == "new"


def test_new_writer_refuses_when_committed_cleanup_still_fails(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    _publish(
        destination,
        "new",
        replace=True,
        filesystem=_PublicationFileSystem(
            _Fault("before_rmdir_tree:previous", OSError)
        ),
    )
    persistent = _PublicationFileSystem(
        _Fault("before_rmdir_tree:previous", OSError, once=False)
    )

    with pytest.raises(_PublicationError, match="cleanup is still pending"):
        _publish(
            destination,
            "third",
            replace=True,
            filesystem=persistent,
        )

    assert _content(destination) == "new"


def test_stable_reader_and_recovery_use_distinct_validators(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    calls: list[str] = []

    def stable(path: Path, owner: Mapping[str, object]) -> None:
        calls.append("stable")
        receipt = json.loads((path / "artifact.json").read_text(encoding="utf-8"))
        assert receipt["owner"] == owner

    def full(path: Path, owner: Mapping[str, object]) -> None:
        calls.append("full")
        _validate(path, owner)

    with pytest.raises(_Crash):
        _publish(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_prepared")),
        )

    value = _read_published(
        destination,
        owner=OWNER,
        stable_validate=stable,
        recovery_validate=full,
        read=_content,
    )

    assert value == "old"
    assert calls == ["full", "full", "stable"]


def test_no_journal_read_uses_only_stable_projection(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    calls: list[str] = []

    def stable(path: Path, owner: Mapping[str, object]) -> None:
        calls.append("stable")
        assert (
            json.loads((path / "artifact.json").read_text(encoding="utf-8"))["owner"]
            == owner
        )

    def full(_path: Path, _owner: Mapping[str, object]) -> None:
        calls.append("full")

    assert (
        _read_published(
            destination,
            owner=OWNER,
            stable_validate=stable,
            recovery_validate=full,
            read=_content,
        )
        == "old"
    )
    assert calls == ["stable"]


def test_lock_set_rejects_equal_and_ancestor_related_paths(tmp_path: Path) -> None:
    parent = tmp_path / "output"
    parent.mkdir()
    child = parent / "child"
    output = _PublicationOutput(parent, _validate)

    with pytest.raises(_PublicationError, match="ancestor-related"):
        with _publication_lock_set(
            outputs=[output, _PublicationOutput(child, _validate)]
        ):
            pass
    with pytest.raises(_PublicationError, match="ancestor-related"):
        with _publication_lock_set(outputs=[output, output]):
            pass
    with pytest.raises(_PublicationError, match="ancestor-related"):
        with _publication_lock_set(
            inputs=[_PublicationInput(parent, OWNER, _validate, _validate)],
            outputs=[output],
        ):
            pass
    with pytest.raises(_PublicationError, match="ancestor-related"):
        with _publication_lock_set(
            outputs=[
                _PublicationOutput(tmp_path / "Case", _validate),
                _PublicationOutput(tmp_path / "case", _validate),
            ]
        ):
            pass


def test_lock_set_rejects_artifact_control_region_overlap_before_bootstrap(
    tmp_path: Path,
) -> None:
    final = tmp_path / "design.ac"
    _publish(final, "baseline")
    nested_output = _control(final) / "evil"
    snapshot = {
        path.relative_to(tmp_path): path.read_bytes()
        for path in tmp_path.rglob("*")
        if path.is_file()
    }
    managed_input = _PublicationInput(final, OWNER, _validate, _validate)

    with pytest.raises(_PublicationError, match="ancestor-related"):
        with _publication_lock_set(
            inputs=[managed_input],
            outputs=[_PublicationOutput(nested_output, _validate)],
        ):
            pass
    with pytest.raises(_PublicationError, match="ancestor-related"):
        with _publication_lock_set(
            inputs=[_PublicationInput(nested_output, OWNER, _validate, _validate)],
            outputs=[_PublicationOutput(final, _validate)],
        ):
            pass

    current = {
        path.relative_to(tmp_path): path.read_bytes()
        for path in tmp_path.rglob("*")
        if path.is_file()
    }
    assert current == snapshot
    assert not nested_output.exists()
    assert not _control(nested_output).exists()


def test_command_lock_set_acquires_normalized_paths_in_sorted_order(
    tmp_path: Path,
) -> None:
    first = tmp_path / "a"
    middle = tmp_path / "m"
    second = tmp_path / "z"
    filesystem = _RecordingFileSystem()
    _publish(first, "first", filesystem=filesystem)
    _publish(second, "second", filesystem=filesystem)
    filesystem.lock_order.clear()
    requests = [
        _PublicationInput(second, OWNER, _validate, _validate),
        _PublicationInput(first, OWNER, _validate, _validate),
    ]

    with _publication_lock_set(
        inputs=requests,
        outputs=[_PublicationOutput(middle, _validate)],
        filesystem=filesystem,
    ) as locks:
        command_order = [
            (_control(first) / "lock", True),
            (_control(middle) / "lock", False),
            (_control(second) / "lock", True),
        ]
        assert filesystem.lock_order[-3:] == command_order
        assert locks.snapshot(first, _content) == "first"
        result = _publish(middle, "middle", locks=locks)
        assert result.committed
        assert filesystem.lock_order[-3:] == command_order
    assert _content(middle) == "middle"


def test_shared_input_snapshot_blocks_process_writer_until_release(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    process_context = multiprocessing.get_context("spawn")
    ready = process_context.Queue()
    done = process_context.Queue()
    process = process_context.Process(
        target=_process_replace, args=(str(destination), ready, done)
    )
    request = _PublicationInput(destination, OWNER, _validate, _validate)

    try:
        with _publication_lock_set(inputs=[request]) as locks:
            assert locks.snapshot(destination, _content) == "old"
            process.start()
            assert ready.get(timeout=5) == "ready"
            with pytest.raises(queue.Empty):
                done.get(timeout=0.25)
            assert locks.snapshot(destination, _content) == "old"
        assert done.get(timeout=5) == ("done", "child")
        process.join(timeout=5)
        assert process.exitcode == 0
        assert _content(destination) == "child"
    finally:
        if process.is_alive():
            process.terminate()
            process.join(timeout=5)


def test_lock_set_discards_partial_inputs_after_recovery(tmp_path: Path) -> None:
    first = tmp_path / "a"
    interrupted = tmp_path / "z"
    _publish(first, "first")
    _publish(interrupted, "old")
    with pytest.raises(_Crash):
        _publish(
            interrupted,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_prepared")),
        )
    first_validations = 0

    def count_first(path: Path, owner: Mapping[str, object]) -> None:
        nonlocal first_validations
        first_validations += 1
        _validate(path, owner)

    inputs = [
        _PublicationInput(first, OWNER, count_first, _validate),
        _PublicationInput(interrupted, OWNER, _validate, _validate),
    ]
    with _publication_lock_set(inputs=inputs) as locks:
        assert locks.snapshot(first, _content) == "first"
        assert locks.snapshot(interrupted, _content) == "old"

    assert first_validations == 2


def test_formal_journal_wins_over_truncated_temporary(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    with pytest.raises(_Crash):
        _publish(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_prepared")),
        )
    (_control(destination) / "journal.json.tmp").write_text("{", encoding="utf-8")

    _recover_publication(destination, validate=_validate)

    assert _content(destination) == "old"


def test_temporary_journal_without_transaction_is_discarded(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    (_control(destination) / "journal.json.tmp").write_text("{", encoding="utf-8")

    result = _recover_publication(destination, validate=_validate)

    assert not result.committed
    assert _content(destination) == "old"
    assert not (_control(destination) / "journal.json.tmp").exists()


@pytest.mark.parametrize(
    "malformed",
    [
        [],
        {"kind": "pycircuit-publication"},
        {
            "kind": "pycircuit-publication",
            "artifact": "source-unit",
            "destination": "unit",
            "owner": OWNER,
            "had_previous": True,
            "phase": "preparing",
            "extra": 1,
        },
        {
            "kind": "pycircuit-publication",
            "artifact": "source-unit",
            "destination": "unit",
            "owner": OWNER,
            "had_previous": 1,
            "phase": "preparing",
        },
        {
            "kind": "pycircuit-publication",
            "artifact": "source-unit",
            "destination": "unit",
            "owner": OWNER,
            "had_previous": True,
            "phase": [],
        },
        {
            "kind": "pycircuit-publication",
            "artifact": "generated",
            "destination": "unit",
            "owner": OWNER,
            "had_previous": True,
            "phase": "preparing",
        },
        {
            "kind": "pycircuit-publication",
            "artifact": "source-unit",
            "destination": "other",
            "owner": OWNER,
            "had_previous": True,
            "phase": "preparing",
        },
        {
            "kind": "pycircuit-publication",
            "artifact": "program",
            "destination": "unit",
            "owner": {
                "kind": "program",
                "source": {"package": "demo", "path": "leaf.py"},
                "definition": "demo.leaf.Leaf",
            },
            "had_previous": True,
            "phase": "preparing",
        },
    ],
)
def test_malformed_formal_journal_is_rejected_without_guessing(
    tmp_path: Path, malformed: object
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    journal = _control(destination) / "journal.json"
    journal.write_text(json.dumps(malformed), encoding="utf-8")

    with pytest.raises(_PublicationError):
        _recover_publication(destination, validate=_validate)

    assert journal.exists()
    assert _content(destination) == "old"


def test_invalid_formal_journal_is_not_replaced_by_valid_temporary(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    control = _control(destination)
    valid = {
        "kind": "pycircuit-publication",
        "artifact": "source-unit",
        "destination": "unit",
        "owner": OWNER,
        "had_previous": True,
        "phase": "preparing",
    }
    (control / "journal.json").write_text("{}", encoding="utf-8")
    (control / "journal.json.tmp").write_text(json.dumps(valid), encoding="utf-8")

    with pytest.raises(_PublicationError, match="fields are not closed"):
        _recover_publication(destination, validate=_validate)

    assert (control / "journal.json.tmp").exists()
    assert _content(destination) == "old"


def test_duplicate_formal_journal_key_is_rejected(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    journal = _control(destination) / "journal.json"
    journal.write_text(
        '{"kind":"pycircuit-publication",'
        '"kind":"pycircuit-publication",'
        '"artifact":"source-unit","destination":"unit",'
        f'"owner":{json.dumps(OWNER)},'
        '"had_previous":true,"phase":"preparing"}',
        encoding="utf-8",
    )

    with pytest.raises(_PublicationFileSystemError, match="duplicate key"):
        _recover_publication(destination, validate=_validate)

    assert journal.exists()
    assert _content(destination) == "old"


def test_unknown_control_entry_and_destination_symlink_are_rejected(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination, "old")
    (_control(destination) / "foreign").write_text("user", encoding="utf-8")

    with pytest.raises(_PublicationError, match="unknown entries"):
        _recover_publication(destination, validate=_validate)

    other = tmp_path / "other"
    other.mkdir()
    symlink = tmp_path / "linked"
    try:
        symlink.symlink_to(other, target_is_directory=True)
    except (NotImplementedError, OSError):
        pytest.skip("directory symlinks are not available")
    with pytest.raises(_PublicationError, match="type 'link'"):
        _publish(symlink, "unsafe")


@pytest.mark.parametrize(
    "owner",
    [
        {"kind": "source-unit", "source": {"package": "demo", "path": "x.py"}, "x": 1},
        {"kind": "source-unit", "source": {"package": "demo", "path": "./x.py"}},
        {"kind": "source-unit", "source": {"package": "demo", "path": "a//x.py"}},
        {
            "kind": "generated",
            "source": {"package": "demo", "path": "x.py"},
            "definition": "d",
            "target": "vhdl",
        },
        {
            "kind": "generated",
            "source": {"package": "demo", "path": "x.py"},
            "definition": "d",
            "target": [],
        },
    ],
)
def test_publication_owner_schema_is_closed_and_normalized(
    tmp_path: Path, owner: Mapping[str, object]
) -> None:
    with pytest.raises(_PublicationError):
        _publish_directory(
            tmp_path / "unit",
            owner=owner,
            build=_builder("new"),
            validate=_validate,
        )


@pytest.mark.parametrize(
    "definition",
    [
        "demo.leaf.Leaf",
        '@""',
        '@"demo..Leaf"',
        '@"demo.leaf-name.Leaf"',
        '@"demo.leaf.Leaf"suffix',
        '@"demo.leaf.1Leaf"',
        '@"demo.leaf.Leaf\\22"',
    ],
)
def test_qualified_symbol_must_use_canonical_quoted_components(
    tmp_path: Path, definition: str
) -> None:
    owner = _publication_owner_program(
        package="demo", path="leaf.py", definition=definition
    )

    with pytest.raises(_PublicationError, match="definition is not canonical"):
        _publish_file(
            tmp_path / "program.ac",
            owner=owner,
            build=_program_builder("program"),
            validate=_validate_program,
        )

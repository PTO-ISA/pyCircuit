"""C3 directory publication is recoverable and preserves owned outputs."""

from __future__ import annotations

import json
from collections.abc import Mapping
from pathlib import Path

import pytest
from pycircuit._publication import (
    _publication_owner_generated,
    _publication_owner_source_unit,
    _PublicationCancelledError,
    _PublicationError,
    _publish_directory,
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


OWNER = _publication_owner_source_unit(package="demo", path="leaf.py")


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
        package="demo", path="leaf.py", definition="demo.leaf.Leaf", target="cpp"
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
        validate=_validate,
        read=_content,
    )
    assert read == "new"
    assert (_control(destination) / "previous").is_dir()
    assert (_control(destination) / "journal.json").is_file()


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
    with pytest.raises(_PublicationError, match="symlink"):
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

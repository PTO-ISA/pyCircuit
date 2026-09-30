"""Unit oracles for managed source-unit owner discovery.

``_discover_source_unit_owner`` is the first half of the input-recovery defect:
the driver asks it for the owner of a directory *before* any lock set exists, so
it must answer for a managed unit whose publication did not finish (the
destination is absent because the old target was moved aside) and it must keep
refusing a corrupt control state or journal instead of treating the path as an
unmanaged directory.

These are stronger than the system-level oracle because they pin the exact
returned owner and prove that discovery itself changes nothing on disk. The
managed directory, its control marker, its journal and its interrupted
transaction are all produced by the real publication protocol over real files;
no native helper is involved, so this stays in the pure-Python lane.
"""

from __future__ import annotations

import json
from collections.abc import Mapping
from pathlib import Path

import pytest
from pycircuit._publication import (
    _publication_owner_source_unit,
    _PublicationError,
    _publish_directory,
)
from pycircuit._publication_fs import _PublicationFileSystem
from pycircuit._source_unit_files import _discover_source_unit_owner

pytestmark = pytest.mark.unit

OWNER = _publication_owner_source_unit(package="demo", path="leaf.py")
SOURCE = {"package": "demo", "path": "leaf.py"}
FILES = {
    "body": "leaf.ac",
    "interface": "leaf.interface.ac",
    "depfile": "leaf.d",
}


class _Abort(BaseException):
    """Abrupt termination: only a BaseException leaves the transaction behind."""


class _Fault:
    def __init__(self, point: str) -> None:
        self.point = point
        self.hit = False

    def __call__(self, point: str) -> None:
        if point == self.point and not self.hit:
            self.hit = True
            raise _Abort(f"injected fault at {point}")


def _receipt() -> str:
    return (
        json.dumps(
            {"kind": "pycircuit-source-unit", "source": SOURCE, "files": FILES},
            indent=2,
        )
        + "\n"
    )


def _builder(body: str):
    def build(stage: Path) -> None:
        (stage / "leaf.ac").write_text(
            f'module attributes {{ac.source_owner = {{package = "demo", '
            f'path = "leaf.py"}}}} {{\n// {body}\n}}\n',
            encoding="utf-8",
        )
        (stage / "leaf.interface.ac").write_text(
            f'module attributes {{ac.source_owner = {{package = "demo", '
            f'path = "leaf.py"}}}} {{\n// {body}\n}}\n',
            encoding="utf-8",
        )
        (stage / "leaf.d").write_text("leaf.ac: leaf.py\n", encoding="utf-8")
        (stage / "unit.json").write_text(_receipt(), encoding="utf-8")

    return build


def _validate(path: Path, owner: Mapping[str, object]) -> None:
    assert {entry.name for entry in path.iterdir()} == {
        "leaf.ac",
        "leaf.interface.ac",
        "leaf.d",
        "unit.json",
    }
    receipt = json.loads((path / "unit.json").read_text(encoding="utf-8"))
    assert receipt == {
        "kind": "pycircuit-source-unit",
        "source": SOURCE,
        "files": FILES,
    }
    assert owner == OWNER


def _publish(destination: Path, body: str, **kwargs: object):
    return _publish_directory(
        destination,
        owner=OWNER,
        build=_builder(body),
        validate=_validate,
        **kwargs,
    )


def _control(destination: Path) -> Path:
    return destination.parent / f".{destination.name}.pycircuit-publication"


def _interrupt(destination: Path, *, point: str) -> None:
    """Replace the published unit and die at ``point``."""

    with pytest.raises(_Abort):
        _publish(
            destination,
            "new",
            replace=True,
            filesystem=_PublicationFileSystem(_Fault(point)),
        )


def test_owner_discovery_reads_the_validated_journal_of_an_unfinished_transaction(
    tmp_path: Path,
) -> None:
    """A prepared transaction is a managed unit, not a missing input."""

    destination = tmp_path / "leaf"
    _publish(destination, "old")
    _interrupt(destination, point="after_previous_saved")

    control = _control(destination)
    assert not destination.exists()
    assert {entry.name for entry in control.iterdir()} == {
        "journal.json",
        "lock",
        "owner.json",
        "previous",
        "stage",
    }
    assert json.loads((control / "journal.json").read_text(encoding="utf-8"))[
        "phase"
    ] == ("prepared")
    snapshot = {
        str(entry.relative_to(control)): entry.read_bytes()
        for entry in sorted(control.rglob("*"))
        if entry.is_file()
    }

    discovered = _discover_source_unit_owner(destination)

    assert discovered == {
        "kind": "source-unit",
        "source": {"package": "demo", "path": "leaf.py"},
    }
    # Discovery only reads: the transaction is left for the lock set to recover.
    assert not destination.exists()
    assert {
        str(entry.relative_to(control)): entry.read_bytes()
        for entry in sorted(control.rglob("*"))
        if entry.is_file()
    } == snapshot


def test_owner_discovery_refuses_a_corrupt_journal_instead_of_guessing(
    tmp_path: Path,
) -> None:
    """A corrupt journal is named; it is never read as an unmanaged directory."""

    destination = tmp_path / "leaf"
    _publish(destination, "old")
    _interrupt(destination, point="after_previous_saved")
    control = _control(destination)
    journal = control / "journal.json"
    journal.write_text("{}", encoding="utf-8")

    with pytest.raises(_PublicationError) as rejected:
        _discover_source_unit_owner(destination)

    message = str(rejected.value)
    assert "journal" in message, message
    assert "unit.json" not in message, message
    assert journal.read_text(encoding="utf-8") == "{}"
    assert (control / "previous").is_dir()
    assert (control / "stage").is_dir()
    assert not destination.exists()


def test_owner_discovery_refuses_an_unknown_control_entry(tmp_path: Path) -> None:
    """A corrupt control state is a refusal, not an unmanaged path."""

    destination = tmp_path / "leaf"
    _publish(destination, "old")
    _interrupt(destination, point="after_previous_saved")
    control = _control(destination)
    (control / "user-note.txt").write_text("mine\n", encoding="utf-8")

    with pytest.raises(_PublicationError) as rejected:
        _discover_source_unit_owner(destination)

    message = str(rejected.value)
    assert "control" in message, message
    assert "unknown entries" in message, message
    assert (control / "user-note.txt").read_text(encoding="utf-8") == "mine\n"
    assert (control / "previous").is_dir()
    assert (control / "stage").is_dir()


def test_owner_discovery_reads_a_stable_receipt_and_fails_closed(
    tmp_path: Path,
) -> None:
    """The journal fallback must not change the stable path or accept garbage."""

    destination = tmp_path / "leaf"
    _publish(destination, "old")

    assert _discover_source_unit_owner(destination) == {
        "kind": "source-unit",
        "source": {"package": "demo", "path": "leaf.py"},
    }

    unmanaged = tmp_path / "unmanaged"
    unmanaged.mkdir()

    with pytest.raises(_PublicationError):
        _discover_source_unit_owner(unmanaged)

    absent = tmp_path / "absent"

    with pytest.raises(_PublicationError):
        _discover_source_unit_owner(absent)

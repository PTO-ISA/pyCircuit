"""C3 source-unit files are closed, owned, and read through managed views."""

from __future__ import annotations

import json
import re
from collections.abc import Mapping
from pathlib import Path

import pytest
from pycircuit import _source_capture
from pycircuit._publication import (
    _publication_owner_source_unit,
    _PublicationError,
    _PublicationFileSystem,
    _publish_directory,
    _recover_publication,
)
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_unit_files import (
    _load_full_source_unit,
    _load_source_unit_header,
    _validate_full_source_unit,
)

pytestmark = pytest.mark.unit

OWNER = _publication_owner_source_unit(package="demo.units", path="blocks/leaf.py")

_OWNER_ATTRIBUTE = re.compile(
    r'ac\.source_owner = \{package = "([^"]*)", path = "([^"]*)"\}'
)


def _declared_owner(path: Path) -> dict[str, str]:
    """The source owner ``path`` declares: its own attribute, else its receipt."""

    match = _OWNER_ATTRIBUTE.search(path.read_text(encoding="utf-8"))
    if match is not None:
        return {"package": match.group(1), "path": match.group(2)}
    receipt = json.loads((path.parent / "unit.json").read_text(encoding="utf-8"))
    return dict(receipt["source"])


@pytest.fixture(autouse=True)
def _native_owner_seam(monkeypatch: pytest.MonkeyPatch) -> None:
    """Keep this lane on the receipt/file-set/control-state layer.

    The fixtures in this file are synthetic text, not IR, so the native
    verification seam is stubbed with the owner each artifact under test
    declares (its own ``ac.source_owner`` attribute, else the receipt beside
    it). Native verification of real artifacts - parsing, corruption, owner
    disagreement - is covered end to end by
    ``tests/system/test_artifact_verify_recovery.py``. Nothing about the
    receipt, file-set, symlink or control-state checks asserted here is
    relaxed: the stub reports what the artifact declares, so a fixture whose
    declaration changed would still be refused.
    """

    def owners(body: Path, header: Path) -> tuple[dict[str, str], dict[str, str]]:
        return _declared_owner(body), _declared_owner(header)

    monkeypatch.setattr(
        "pycircuit._source_unit_files._native_verify.verify_source_unit_owners",
        owners,
    )


class _Crash(BaseException):
    pass


class _Fault:
    def __init__(self, point: str) -> None:
        self.point = point

    def __call__(self, point: str) -> None:
        if point == self.point:
            raise _Crash(point)


def _control(destination: Path) -> Path:
    return destination.parent / f".{destination.name}.pycircuit-publication"


def _write_unit(stage: Path, *, owner: Mapping[str, object] = OWNER) -> None:
    source = owner["source"]
    assert isinstance(source, Mapping)
    receipt = {
        "kind": "pycircuit-source-unit",
        "source": dict(source),
        "files": {
            "body": "leaf.ac",
            "interface": "leaf.interface.ac",
            "depfile": "leaf.d",
        },
    }
    (stage / "unit.json").write_text(json.dumps(receipt), encoding="utf-8")
    (stage / "leaf.ac").write_text("module { // body\n}\n", encoding="utf-8")
    (stage / "leaf.interface.ac").write_text(
        "module { // interface\n}\n", encoding="utf-8"
    )
    (stage / "leaf.d").write_text("leaf.ac: leaf.py\n", encoding="utf-8")


def _publish(
    destination: Path,
    *,
    replace: bool = False,
    filesystem: _PublicationFileSystem | None = None,
) -> None:
    _publish_directory(
        destination,
        owner=OWNER,
        build=_write_unit,
        validate=_validate_full_source_unit,
        replace=replace,
        filesystem=filesystem,
    )


def test_full_source_unit_reads_closed_receipt_and_all_files(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination)

    unit = _load_full_source_unit(destination, owner=OWNER)

    assert (unit.receipt.package, unit.receipt.path) == (
        "demo.units",
        "blocks/leaf.py",
    )
    assert "body" in unit.body
    assert "interface" in unit.interface
    assert unit.depfile == "leaf.ac: leaf.py\n"


def test_stable_header_view_reads_only_receipt_and_interface(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    destination = tmp_path / "unit"
    _publish(destination)
    reads: list[str] = []
    original = _source_capture._read_stable_file_bytes

    def record(path: Path) -> bytes:
        reads.append(path.name)
        return original(path)

    monkeypatch.setattr("pycircuit._source_unit_files._read_stable_file_bytes", record)

    view = _load_source_unit_header(destination, owner=OWNER)

    assert "interface" in view.interface
    assert reads == ["owner.json", "unit.json", "leaf.interface.ac"]


def test_stable_header_view_accepts_header_only_provider(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination)
    (destination / "leaf.ac").unlink()
    (destination / "leaf.d").unlink()

    view = _load_source_unit_header(destination, owner=OWNER)

    assert view.receipt.interface == "leaf.interface.ac"
    with pytest.raises(_PublicationError, match="file set is not closed"):
        _load_full_source_unit(destination, owner=OWNER)


def test_header_view_does_not_scan_unrelated_ac_files(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination)
    (destination / "unrelated.ac").write_text("invalid", encoding="utf-8")

    assert "interface" in _load_source_unit_header(destination, owner=OWNER).interface
    with pytest.raises(_PublicationError, match="file set is not closed"):
        _load_full_source_unit(destination, owner=OWNER)


def test_unmanaged_header_projection_is_rejected(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    destination.mkdir()
    _write_unit(destination)

    with pytest.raises(_PublicationError, match="unmanaged"):
        _load_source_unit_header(destination, owner=OWNER)


def test_managed_header_projection_rejects_destination_symlink(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination)
    moved = tmp_path / "moved"
    destination.rename(moved)
    try:
        destination.symlink_to(moved, target_is_directory=True)
    except (NotImplementedError, OSError):
        pytest.skip("directory symlinks are not available")

    with pytest.raises(_PublicationError, match="not a directory"):
        _load_source_unit_header(destination, owner=OWNER)


@pytest.mark.parametrize(
    ("receipt_edit", "message"),
    [
        (lambda value: value.update(extra=True), "fields are not closed"),
        (
            lambda value: value["source"].update(package="other"),
            "owner does not match",
        ),
        (
            lambda value: value["source"].update(path="blocks/other.py"),
            "owner does not match",
        ),
        (
            lambda value: value["files"].update(body="other.ac"),
            "file names do not match source stem",
        ),
        (
            lambda value: value["files"].update(interface="../leaf.interface.ac"),
            "file names do not match source stem",
        ),
        (
            lambda value: value["files"].update(depfile="/tmp/leaf.d"),
            "file names do not match source stem",
        ),
    ],
)
def test_receipt_is_closed_owned_and_uses_stem_derived_safe_names(
    tmp_path: Path, receipt_edit: object, message: str
) -> None:
    destination = tmp_path / "unit"
    _publish(destination)
    receipt_path = destination / "unit.json"
    value = json.loads(receipt_path.read_text(encoding="utf-8"))
    receipt_edit(value)  # type: ignore[operator]
    receipt_path.write_text(json.dumps(value), encoding="utf-8")

    with pytest.raises(_PublicationError, match=message):
        _load_source_unit_header(destination, owner=OWNER)


def test_duplicate_json_keys_are_rejected_in_receipt_and_nested_objects(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination)
    (destination / "unit.json").write_text(
        '{"kind":"pycircuit-source-unit","kind":"pycircuit-source-unit",'
        '"source":{"package":"demo.units","path":"blocks/leaf.py"},'
        '"files":{"body":"leaf.ac","interface":"leaf.interface.ac",'
        '"interface":"leaf.interface.ac","depfile":"leaf.d"}}',
        encoding="utf-8",
    )

    with pytest.raises(_PublicationError, match="strict JSON"):
        _load_source_unit_header(destination, owner=OWNER)


@pytest.mark.parametrize(
    "missing", ["unit.json", "leaf.ac", "leaf.interface.ac", "leaf.d"]
)
def test_full_source_unit_rejects_every_missing_file(
    tmp_path: Path, missing: str
) -> None:
    destination = tmp_path / "unit"
    _publish(destination)
    (destination / missing).unlink()

    with pytest.raises(_PublicationError):
        _load_full_source_unit(destination, owner=OWNER)


def test_full_source_unit_rejects_extra_files_and_symlinks(tmp_path: Path) -> None:
    extra_destination = tmp_path / "extra"
    _publish(extra_destination)
    (extra_destination / "foreign").write_text("x", encoding="utf-8")
    with pytest.raises(_PublicationError, match="file set is not closed"):
        _load_full_source_unit(extra_destination, owner=OWNER)

    linked_destination = tmp_path / "linked"
    _publish(linked_destination)
    target = tmp_path / "replacement.ac"
    target.write_text("module {}\n", encoding="utf-8")
    (linked_destination / "leaf.ac").unlink()
    try:
        (linked_destination / "leaf.ac").symlink_to(target)
    except (NotImplementedError, OSError):
        pytest.skip("file symlinks are not available")
    with pytest.raises(_PublicationError, match="unsafe"):
        _load_full_source_unit(linked_destination, owner=OWNER)


def test_precommit_header_read_recovers_before_using_old_target(tmp_path: Path) -> None:
    destination = tmp_path / "unit"
    _publish(destination)
    with pytest.raises(_Crash):
        _publish(
            destination,
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_prepared")),
        )

    view = _load_source_unit_header(destination, owner=OWNER)

    assert "interface" in view.interface
    assert not (_control(destination) / "journal.json").exists()


@pytest.mark.parametrize(
    "recovery_point",
    ["after_journal_rollback_restore", "after_journal_rollback_cleanup"],
)
def test_rollback_header_read_finishes_recovery_before_using_old_target(
    tmp_path: Path, recovery_point: str
) -> None:
    destination = tmp_path / "unit"
    _publish(destination)
    with pytest.raises(_Crash):
        _publish(
            destination,
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_prepared")),
        )
    with pytest.raises(_Crash):
        _recover_publication(
            destination,
            validate=_validate_full_source_unit,
            filesystem=_PublicationFileSystem(_Fault(recovery_point)),
        )

    view = _load_source_unit_header(destination, owner=OWNER)

    assert "interface" in view.interface
    assert not (_control(destination) / "journal.json").exists()


def test_committed_unfinished_header_read_requires_complete_new_target(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "unit"
    _publish(destination)
    with pytest.raises(_Crash):
        _publish(
            destination,
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_journal_committed")),
        )
    (destination / "leaf.d").unlink()

    with pytest.raises(_PublicationError, match="file set is not closed"):
        _load_source_unit_header(destination, owner=OWNER)


def test_corrupted_control_owner_and_journal_are_strictly_rejected(
    tmp_path: Path,
) -> None:
    owner_destination = tmp_path / "owner-corrupt"
    _publish(owner_destination)
    (_control(owner_destination) / "owner.json").write_text(
        '{"kind":"pycircuit-publication-control",'
        '"destination":"owner-corrupt","destination":"owner-corrupt"}',
        encoding="utf-8",
    )
    with pytest.raises(_PublicationError, match="strict JSON"):
        _load_source_unit_header(owner_destination, owner=OWNER)

    journal_destination = tmp_path / "journal-corrupt"
    _publish(journal_destination)
    (_control(journal_destination) / "journal.json").write_text(
        '{"kind":"pycircuit-publication","artifact":"source-unit",'
        '"destination":"journal-corrupt","owner":'
        + json.dumps(OWNER)
        + ',"had_previous":true,"phase":"committed","phase":"committed"}',
        encoding="utf-8",
    )
    with pytest.raises(_PublicationError, match="strict JSON"):
        _load_source_unit_header(journal_destination, owner=OWNER)


def test_capture_uses_one_stable_raw_byte_snapshot_and_preserves_bom(
    tmp_path: Path,
) -> None:
    source = tmp_path / "source.py"
    source.write_bytes(b"\xef\xbb\xbfvalue = 1\r\n")

    captured = _capture_source_file(source, source_root=tmp_path)

    assert captured.source_bytes == b"\xef\xbb\xbfvalue = 1\r\n"
    assert captured.source == "value = 1\n"
    assert captured.encoding == "utf-8-sig"


def test_capture_rejects_source_that_changes_during_snapshot(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    source = tmp_path / "source.py"
    source.write_text("value = 1\n", encoding="utf-8")
    real_fstat = _source_capture.os.fstat
    calls = 0

    def unstable(descriptor: int):
        nonlocal calls
        calls += 1
        value = real_fstat(descriptor)
        if calls == 2:
            source.write_text("value = 2\n", encoding="utf-8")
        return value

    monkeypatch.setattr(_source_capture.os, "fstat", unstable)

    with pytest.raises(ValueError, match="changed while being read"):
        _capture_source_file(source, source_root=tmp_path)

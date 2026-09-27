"""Focused storage-contract tests for the private publication filesystem."""

from __future__ import annotations

import json
import os
from pathlib import Path
from types import SimpleNamespace

import pytest
from pycircuit._publication_fs import (
    _PublicationFileSystem,
    _PublicationFileSystemError,
    _retry_sharing_violations,
)

pytestmark = pytest.mark.unit


class _InjectedCrash(BaseException):
    pass


@pytest.mark.parametrize(
    "payload",
    [
        '{"kind":"first","kind":"second"}',
        '{"owner":{"path":"one.py","path":"two.py"}}',
    ],
)
def test_read_json_rejects_duplicate_keys(tmp_path: Path, payload: str) -> None:
    metadata = tmp_path / "owner.json"
    metadata.write_text(payload, encoding="utf-8")

    with pytest.raises(_PublicationFileSystemError, match="duplicate key"):
        _PublicationFileSystem().read_json(metadata)


def test_read_json_rejects_a_link_without_reading_its_target(tmp_path: Path) -> None:
    target = tmp_path / "outside.json"
    target.write_text(json.dumps({"outside": True}), encoding="utf-8")
    link = tmp_path / "owner.json"
    try:
        link.symlink_to(target)
    except OSError as error:
        pytest.skip(f"local filesystem cannot create symlinks: {error}")

    with pytest.raises((OSError, _PublicationFileSystemError)):
        _PublicationFileSystem().read_json(link)


def test_atomic_json_write_rejects_a_link_without_truncating_target(
    tmp_path: Path,
) -> None:
    target = tmp_path / "outside.json"
    target.write_text("untouched", encoding="utf-8")
    temporary = tmp_path / "owner.json.tmp"
    try:
        temporary.symlink_to(target)
    except OSError as error:
        pytest.skip(f"local filesystem cannot create symlinks: {error}")

    with pytest.raises(_PublicationFileSystemError, match="temporary metadata"):
        _PublicationFileSystem().write_json_atomic(
            tmp_path / "owner.json", temporary, {"kind": "owner"}
        )

    assert target.read_text(encoding="utf-8") == "untouched"


def test_sync_file_rejects_a_link(tmp_path: Path) -> None:
    target = tmp_path / "program.ac.real"
    target.write_text("module", encoding="utf-8")
    link = tmp_path / "program.ac"
    try:
        link.symlink_to(target)
    except OSError as error:
        pytest.skip(f"local filesystem cannot create symlinks: {error}")

    with pytest.raises((OSError, _PublicationFileSystemError)):
        _PublicationFileSystem().sync_file(link)


def test_open_regular_rejects_path_identity_change(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    metadata = tmp_path / "owner.json"
    metadata.write_text("{}", encoding="utf-8")
    actual = os.stat(metadata, follow_symlinks=False)

    monkeypatch.setattr(
        "pycircuit._publication_fs.os.fstat",
        lambda _descriptor: SimpleNamespace(
            st_mode=actual.st_mode,
            st_dev=actual.st_dev,
            st_ino=actual.st_ino + 1,
        ),
    )

    with pytest.raises(_PublicationFileSystemError, match="changed while opening"):
        _PublicationFileSystem().read_json(metadata)


@pytest.mark.parametrize("operation", ["rename", "replace"])
def test_move_flushes_parent_before_reporting_completion_fault(
    tmp_path: Path, operation: str, monkeypatch: pytest.MonkeyPatch
) -> None:
    events: list[str] = []

    def fault(point: str) -> None:
        events.append(point)
        if point.startswith(f"after_{operation}:"):
            raise _InjectedCrash(point)

    filesystem = _PublicationFileSystem(fault)
    monkeypatch.setattr(
        filesystem,
        "sync_directory",
        lambda path: events.append(f"sync_directory:{path.name}"),
    )
    source = tmp_path / "source"
    destination = tmp_path / "destination"
    source.write_text("new", encoding="utf-8")
    if operation == "replace":
        destination.write_text("old", encoding="utf-8")

    with pytest.raises(_InjectedCrash):
        getattr(filesystem, operation)(source, destination)

    assert destination.read_text(encoding="utf-8") == "new"
    assert events[-2:] == [
        f"sync_directory:{tmp_path.name}",
        f"after_{operation}:source:destination",
    ]


def test_sharing_violation_retry_is_bounded_and_delayed() -> None:
    attempts = 0
    delays: list[float] = []

    def operation() -> None:
        nonlocal attempts
        attempts += 1
        if attempts < 3:
            error = OSError("sharing violation")
            error.winerror = 32  # type: ignore[attr-defined]
            raise error

    _retry_sharing_violations(operation, attempts=3, delay=delays.append)

    assert attempts == 3
    assert delays == [0.01, 0.02]


def test_non_sharing_error_is_not_retried() -> None:
    attempts = 0

    def operation() -> None:
        nonlocal attempts
        attempts += 1
        raise OSError("unrelated failure")

    with pytest.raises(OSError, match="unrelated failure"):
        _retry_sharing_violations(operation, attempts=5, delay=lambda _delay: None)

    assert attempts == 1


@pytest.mark.skipif(os.name != "nt", reason="requires real Windows filesystem APIs")
def test_windows_real_rename_replace_and_directory_flush(tmp_path: Path) -> None:
    filesystem = _PublicationFileSystem()
    source = tmp_path / "source"
    destination = tmp_path / "destination"
    source.write_text("first", encoding="utf-8")

    filesystem.rename(source, destination)
    assert destination.read_text(encoding="utf-8") == "first"

    replacement = tmp_path / "replacement"
    replacement.write_text("second", encoding="utf-8")
    filesystem.replace(replacement, destination)
    assert destination.read_text(encoding="utf-8") == "second"
    filesystem.sync_directory(tmp_path)


@pytest.mark.skipif(os.name != "nt", reason="requires Windows reparse-point support")
def test_windows_real_reparse_metadata_open_is_rejected(tmp_path: Path) -> None:
    target = tmp_path / "outside.json"
    target.write_text("{}", encoding="utf-8")
    link = tmp_path / "owner.json"
    try:
        link.symlink_to(target)
    except OSError as error:
        pytest.skip(f"Windows symlink privilege is unavailable: {error}")

    with pytest.raises((OSError, _PublicationFileSystemError)):
        _PublicationFileSystem().read_json(link)

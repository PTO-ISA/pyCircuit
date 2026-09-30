"""Strict validation and managed reads for generated C3 bundles.

These fixtures exercise the private receipt/file protocol only. They are not
claims that the fixture files form a generated design or a usable DUT.
"""

from __future__ import annotations

import copy
import dataclasses
import json
import multiprocessing
import os
import queue
from collections.abc import Mapping
from contextlib import contextmanager
from multiprocessing.queues import Queue
from pathlib import Path

import pycircuit._generated_bundle as generated_bundle
import pytest
from pycircuit._generated_bundle import (
    _GeneratedBundleSnapshot,
    _load_generated_bundle,
    _read_generated_bundle,
    _validate_generated_bundle,
)
from pycircuit._publication import (
    _publication_lock_set,
    _publication_owner_generated,
    _PublicationError,
    _PublicationInput,
    _publish_directory,
)
from pycircuit._publication_fs import _PublicationFileSystem

pytestmark = pytest.mark.unit

OWNER = _publication_owner_generated(
    package="demo", path="root.py", definition='@"demo.root.Root"', target="cpp"
)

# Roles are intentionally not inferred from suffixes or target in this private
# protocol fixture. Source groups are likewise allowed to list paths in any
# order and may contain a header without a companion source.
FILE_BYTES = {
    "a.data": b"header bytes",
    "b.data": b"source bytes",
    "c.data": b"cmake bytes",
    "d.data": b"rtl bytes",
    "e.data": b"runtime bytes",
    "f.data": b"source map bytes",
    "g.data": b"declaration header bytes",
}
FILE_ROLES = {
    "a.data": "header",
    "b.data": "source",
    "c.data": "cmake",
    "d.data": "rtl",
    "e.data": "runtime-glue",
    "f.data": "source-map",
    "g.data": "header",
}


def _receipt(
    owner: Mapping[str, object] = OWNER,
    *,
    arguments: list[object] | None = None,
    files: Mapping[str, str] = FILE_ROLES,
    source_groups: list[dict[str, object]] | None = None,
) -> dict[str, object]:
    source = dict(owner["source"])  # type: ignore[arg-type]
    if source_groups is None:
        # Deliberately reverse owner order and use unsorted group membership.
        source_groups = [
            {"source": {"package": "demo", "path": "types.py"}, "files": ["g.data"]},
            {
                "source": source,
                "files": ["f.data", "b.data", "a.data"],
            },
        ]
    return {
        "kind": "pycircuit-generated",
        "target": owner["target"],
        "entry": {
            "definition": owner["definition"],
            "arguments": [] if arguments is None else arguments,
        },
        "entry_source": source,
        "files": [{"path": path, "role": files[path]} for path in sorted(files)],
        "source_groups": source_groups,
    }


def _write_bundle(
    path: Path,
    *,
    owner: Mapping[str, object] = OWNER,
    receipt: object | None = None,
    receipt_bytes: bytes | None = None,
    files: Mapping[str, bytes] = FILE_BYTES,
) -> bytes:
    path.mkdir(parents=True, exist_ok=True)
    for relative, content in files.items():
        target = path / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(content)
    raw = receipt_bytes
    if raw is None:
        if receipt is None:
            receipt = _receipt(owner)
        raw = json.dumps(receipt, separators=(",", ":")).encode("utf-8")
    (path / "generated.json").write_bytes(raw)
    return raw


def _validate_or_fail(path: Path, owner: Mapping[str, object] = OWNER) -> None:
    with pytest.raises(_PublicationError):
        _validate_generated_bundle(path, owner)


def _builder(receipt: object, files: Mapping[str, bytes] = FILE_BYTES):
    def build(stage: Path) -> None:
        _write_bundle(stage, receipt=receipt, files=files)

    return build


def _publish(
    destination: Path,
    *,
    owner: Mapping[str, object] = OWNER,
    receipt: object | None = None,
    files: Mapping[str, bytes] = FILE_BYTES,
    replace: bool = False,
    filesystem: _PublicationFileSystem | None = None,
):
    return _publish_directory(
        destination,
        owner=owner,
        build=_builder(_receipt(owner) if receipt is None else receipt, files),
        validate=_validate_generated_bundle,
        replace=replace,
        filesystem=filesystem,
    )


@pytest.mark.parametrize("target", ["cpp", "verilog"])
def test_valid_receipt_accepts_header_only_group_and_preserves_snapshot_bytes(
    tmp_path: Path, target: str
) -> None:
    destination = tmp_path / "generated"
    owner = _publication_owner_generated(
        package="demo", path="root.py", definition='@"demo.root.Root"', target=target
    )
    raw_receipt = _write_bundle(destination, owner=owner)

    _validate_generated_bundle(destination, owner)
    snapshot = _read_generated_bundle(destination, owner, _PublicationFileSystem())

    assert isinstance(snapshot, _GeneratedBundleSnapshot)
    assert snapshot.receipt == raw_receipt
    assert snapshot.files == tuple(sorted(FILE_BYTES.items()))
    assert type(snapshot.receipt) is bytes
    assert type(snapshot.files) is tuple
    assert all(
        type(path) is str and type(content) is bytes for path, content in snapshot.files
    )
    with pytest.raises(dataclasses.FrozenInstanceError):
        snapshot.receipt = b"changed"  # type: ignore[misc]
    with pytest.raises(dataclasses.FrozenInstanceError):
        snapshot.files = ()  # type: ignore[misc]

    (destination / "a.data").write_bytes(b"later disk contents")
    assert dict(snapshot.files)["a.data"] == b"header bytes"


def test_nonempty_entry_arguments_fail_closed_in_current_private_profile(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "generated"
    receipt = _receipt(arguments=[{"kind": "integer", "value": "1"}])
    _write_bundle(destination, receipt=receipt)

    with pytest.raises(_PublicationError, match="arguments are not implemented"):
        _validate_generated_bundle(destination, OWNER)


@pytest.mark.parametrize("arguments", [None, False, {}, "[]"])
def test_entry_arguments_require_an_array(tmp_path: Path, arguments: object) -> None:
    destination = tmp_path / "generated"
    receipt = _receipt()
    receipt["entry"]["arguments"] = arguments  # type: ignore[index]
    _write_bundle(destination, receipt=receipt)

    with pytest.raises(_PublicationError, match="arguments must be an array"):
        _validate_generated_bundle(destination, OWNER)


@pytest.mark.parametrize(
    "change",
    [
        lambda receipt: receipt.pop("kind"),
        lambda receipt: receipt.update(extra=True),
        lambda receipt: receipt.update(kind="other"),
        lambda receipt: receipt.update(target="vhdl"),
        lambda receipt: receipt.update(target=[]),
        lambda receipt: receipt["entry"].update(extra=True),
        lambda receipt: receipt["entry"].update(definition='@"demo.other.Other"'),
        lambda receipt: receipt.update(entry=None),
        lambda receipt: receipt.update(
            entry_source={"package": "demo", "path": "other.py"}
        ),
        lambda receipt: receipt.update(entry_source=[]),
        lambda receipt: receipt.update(files="not-an-array"),
        lambda receipt: receipt.update(source_groups={}),
    ],
)
def test_receipt_schema_is_closed_and_owner_bound(tmp_path: Path, change) -> None:
    destination = tmp_path / "generated"
    receipt = copy.deepcopy(_receipt())
    change(receipt)
    _write_bundle(destination, receipt=receipt)

    _validate_or_fail(destination)


@pytest.mark.parametrize("receipt", [[], "not-an-object"])
def test_generated_receipt_must_be_a_json_object(
    tmp_path: Path, receipt: object
) -> None:
    destination = tmp_path / "generated"
    _write_bundle(destination, receipt=receipt)

    _validate_or_fail(destination)


def test_owner_argument_binds_source_definition_and_target(tmp_path: Path) -> None:
    destination = tmp_path / "generated"
    _write_bundle(destination)
    mismatches = [
        _publication_owner_generated(
            package="demo",
            path="other.py",
            definition='@"demo.root.Root"',
            target="cpp",
        ),
        _publication_owner_generated(
            package="demo",
            path="root.py",
            definition='@"demo.other.Other"',
            target="cpp",
        ),
        _publication_owner_generated(
            package="demo",
            path="root.py",
            definition='@"demo.root.Root"',
            target="verilog",
        ),
    ]

    for owner in mismatches:
        _validate_or_fail(destination, owner)


@pytest.mark.parametrize(
    "raw",
    [
        b"\xff",
        json.dumps(_receipt(), separators=(",", ":"))
        .encode("utf-8")
        .replace(
            b'"kind":"pycircuit-generated"',
            b'"kind":"pycircuit-generated","kind":"pycircuit-generated"',
            1,
        ),
        # This otherwise-complete receipt parses without the strict duplicate
        # hook; the duplicate key is nested under entry.
        json.dumps(_receipt(), separators=(",", ":"))
        .encode("utf-8")
        .replace(
            b'"definition":"@\\"demo.root.Root\\""',
            b'"definition":"@\\"demo.root.Root\\"","definition":"@\\"demo.root.Root\\""',
            1,
        ),
    ],
)
def test_malformed_utf8_and_nested_duplicate_keys_are_rejected(
    tmp_path: Path, raw: bytes
) -> None:
    if raw != b"\xff":
        assert type(json.loads(raw)) is dict
        assert (
            raw.count(b'"kind":"pycircuit-generated"') == 2
            or raw.count(b'"definition":"') == 2
        )
    destination = tmp_path / "generated"
    _write_bundle(destination, receipt_bytes=raw)

    _validate_or_fail(destination)


@pytest.mark.parametrize(
    ("unsafe_path", "diagnostic"),
    [
        ("", "invalid"),
        (".", "canonical POSIX"),
        ("./a", "canonical POSIX"),
        ("../escape", "canonical POSIX"),
        ("a/../b", "canonical POSIX"),
        ("/absolute", "canonical POSIX"),
        ("a//b", "canonical POSIX"),
        ("a\\b", "invalid"),
        ("C:/absolute", "drive or stream marker"),
        ("a:b", "drive or stream marker"),
    ],
)
def test_receipt_paths_must_be_safe_relative_posix_paths(
    tmp_path: Path, unsafe_path: str, diagnostic: str
) -> None:
    destination = tmp_path / "generated"
    receipt = _receipt()
    receipt["files"].append({"path": unsafe_path, "role": "header"})  # type: ignore[union-attr]
    receipt["files"].sort(key=lambda item: item["path"])  # type: ignore[union-attr,index]
    _write_bundle(destination, receipt=receipt)

    with pytest.raises(_PublicationError, match=diagnostic):
        _validate_generated_bundle(destination, OWNER)


@pytest.mark.parametrize(
    ("mutate", "diagnostic"),
    [
        (lambda receipt: receipt["files"].reverse(), "unique and sorted"),
        (
            lambda receipt: receipt["files"].insert(
                1, copy.deepcopy(receipt["files"][0])
            ),
            "unique and sorted",
        ),
        (
            lambda receipt: receipt["files"].append(
                {"path": "generated.json", "role": "header"}
            ),
            "cannot list itself",
        ),
        (lambda receipt: receipt["files"][0].update(role="unknown"), "role is invalid"),
        (lambda receipt: receipt["files"][0].update(role=[]), "role is invalid"),
        (lambda receipt: receipt["files"][0].update(role={}), "role is invalid"),
        (lambda receipt: receipt["files"][0].update(extra=True), "is not closed"),
        (lambda receipt: receipt["files"].__setitem__(0, None), "is not closed"),
        (lambda receipt: receipt["files"].__setitem__(0, []), "is not closed"),
    ],
)
def test_file_manifest_is_sorted_unique_safe_and_closed(
    tmp_path: Path, mutate, diagnostic: str
) -> None:
    destination = tmp_path / "generated"
    receipt = copy.deepcopy(_receipt())
    mutate(receipt)
    _write_bundle(destination, receipt=receipt)

    with pytest.raises(_PublicationError, match=diagnostic):
        _validate_generated_bundle(destination, OWNER)


def test_manifest_rejects_file_directory_prefix_collision(tmp_path: Path) -> None:
    destination = tmp_path / "generated"
    receipt = _receipt(files={"a": "header", "a/b": "source"}, source_groups=[])
    _write_bundle(destination, receipt=receipt, files={"a.data": b"actual"})

    with pytest.raises(_PublicationError, match="both file and directory"):
        _validate_generated_bundle(destination, OWNER)


@pytest.mark.parametrize(
    ("groups", "diagnostic"),
    [
        (
            [
                {"source": {"package": "demo", "path": "root.py"}, "files": ["a.data"]},
                {"source": {"package": "demo", "path": "root.py"}, "files": ["g.data"]},
            ],
            "repeats a source group owner",
        ),
        (
            [
                {
                    "source": {"package": "demo", "path": "types.py"},
                    "files": ["a.data"],
                },
                {"source": {"package": "demo", "path": "root.py"}, "files": ["a.data"]},
            ],
            "multiple source-group entries",
        ),
        (
            [{"source": {"package": "demo", "path": "root.py"}, "files": ["unlisted"]}],
            "not declared",
        ),
        (
            [
                {
                    "source": {"package": "demo", "path": "root.py"},
                    "files": ["a.data", "a.data"],
                }
            ],
            "multiple source-group entries",
        ),
        (
            [{"source": {"package": "demo", "path": "./root.py"}, "files": ["a.data"]}],
            "source path is invalid",
        ),
        (
            [
                {
                    "source": {"package": "demo", "path": "root.py", "extra": 1},
                    "files": ["a.data"],
                }
            ],
            "owner fields are not closed",
        ),
        (
            [{"source": {"package": "demo", "path": "root.py"}, "files": "a.data"}],
            "files must be an array",
        ),
        ([None], "is not closed"),
        (
            [
                {
                    "source": {"package": "demo", "path": "root.py"},
                    "files": ["../escape"],
                }
            ],
            "canonical POSIX",
        ),
    ],
)
def test_source_groups_require_unique_owners_and_file_membership(
    tmp_path: Path, groups, diagnostic: str
) -> None:
    destination = tmp_path / "generated"
    _write_bundle(destination, receipt=_receipt(source_groups=groups))

    with pytest.raises(_PublicationError, match=diagnostic):
        _validate_generated_bundle(destination, OWNER)


def test_actual_file_set_must_match_receipt_and_have_no_extra_directories(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "generated"
    _write_bundle(destination)
    (destination / "extra.bin").write_bytes(b"unlisted")
    _validate_or_fail(destination)

    (destination / "extra.bin").unlink()
    (destination / "empty").mkdir()
    _validate_or_fail(destination)


def test_missing_listed_file_is_rejected(tmp_path: Path) -> None:
    destination = tmp_path / "generated"
    _write_bundle(destination)
    (destination / "a.data").unlink()

    _validate_or_fail(destination)


def test_symlink_and_special_files_are_rejected(tmp_path: Path) -> None:
    destination = tmp_path / "generated"
    _write_bundle(destination)
    original = destination / "a.data"
    original.unlink()
    target = tmp_path / "external"
    target.write_bytes(b"outside")
    try:
        original.symlink_to(target)
    except (NotImplementedError, OSError):
        pytest.skip("file symlinks are not available")
    _validate_or_fail(destination)

    original.unlink()
    original.write_bytes(FILE_BYTES["a.data"])
    if not hasattr(os, "mkfifo"):
        pytest.skip("FIFO special files are unavailable")
    original.unlink()
    os.mkfifo(original)
    _validate_or_fail(destination)


def test_publish_replace_validates_old_stage_and_preserves_destination(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "generated"
    first = _publish(destination)
    assert first.committed
    old_receipt = (destination / "generated.json").read_bytes()

    second = _publish(destination, replace=True)
    assert second.committed
    assert (destination / "generated.json").read_bytes() == old_receipt

    invalid_receipt = _receipt()
    invalid_receipt["files"].append({"path": "extra", "role": "source"})  # type: ignore[union-attr]
    invalid_receipt["files"].sort(key=lambda item: item["path"])  # type: ignore[union-attr,index]
    before_invalid_stage = (destination / "generated.json").read_bytes()
    with pytest.raises(_PublicationError, match="directory file set is not closed"):
        _publish(destination, receipt=invalid_receipt, replace=True)
    assert (destination / "generated.json").read_bytes() == before_invalid_stage

    (destination / "generated.json").write_bytes(b"broken old receipt")
    broken_old = (destination / "generated.json").read_bytes()
    with pytest.raises(_PublicationError, match="strict UTF-8 JSON"):
        _publish(destination, replace=True)
    assert (destination / "generated.json").read_bytes() == broken_old


def test_replace_rejects_foreign_generated_owner_without_changing_old_bundle(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "generated"
    _publish(destination)
    previous_receipt = (destination / "generated.json").read_bytes()
    other_owners = [
        _publication_owner_generated(
            package="demo",
            path="root.py",
            definition='@"demo.root.Root"',
            target="verilog",
        ),
        _publication_owner_generated(
            package="demo",
            path="root.py",
            definition='@"demo.other.Other"',
            target="cpp",
        ),
        _publication_owner_generated(
            package="demo",
            path="other.py",
            definition='@"demo.root.Root"',
            target="cpp",
        ),
    ]

    for other in other_owners:
        with pytest.raises(_PublicationError, match="does not match its owner"):
            _publish(destination, owner=other, replace=True)
        assert (destination / "generated.json").read_bytes() == previous_receipt


class _Crash(BaseException):
    pass


class _Fault:
    def __init__(
        self,
        point: str,
        exception: type[BaseException] = _Crash,
        *,
        once: bool = True,
    ) -> None:
        self.point = point
        self.exception = exception
        self.once = once
        self.hit = False

    def __call__(self, point: str) -> None:
        if point == self.point and (not self.once or not self.hit):
            self.hit = True
            raise self.exception(f"injected failure at {point}")


def test_managed_read_recovers_old_bundle_after_previous_was_saved(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "generated"
    _publish(destination)
    old_receipt = (destination / "generated.json").read_bytes()
    old_file = (destination / "a.data").read_bytes()
    new_receipt = _receipt()
    new_receipt["target"] = "cpp"
    changed_files = dict(FILE_BYTES)
    changed_files["a.data"] = b"uncommitted new bytes"

    with pytest.raises(_Crash):
        _publish(
            destination,
            receipt=new_receipt,
            files=changed_files,
            replace=True,
            filesystem=_PublicationFileSystem(_Fault("after_previous_saved")),
        )

    snapshot = _load_generated_bundle(destination, owner=OWNER)
    assert snapshot.receipt == old_receipt
    assert dict(snapshot.files)["a.data"] == old_file
    assert not (
        destination.parent
        / f".{destination.name}.pycircuit-publication"
        / "journal.json"
    ).exists()


def test_committed_cleanup_pending_is_readable_and_blocks_new_writer(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "generated"
    _publish(destination)
    changed = _receipt()
    file_bytes = dict(FILE_BYTES)
    file_bytes["a.data"] = b"new committed bytes"
    pending = _publish(
        destination,
        receipt=changed,
        files=file_bytes,
        replace=True,
        filesystem=_PublicationFileSystem(
            _Fault("before_rmdir_tree:previous", OSError, once=False)
        ),
    )

    assert pending.committed
    assert pending.cleanup_pending
    assert (destination / "a.data").read_bytes() == b"new committed bytes"
    journal = (
        destination.parent
        / f".{destination.name}.pycircuit-publication"
        / "journal.json"
    )
    assert journal.is_file()

    snapshot = _load_generated_bundle(destination, owner=OWNER)
    assert dict(snapshot.files)["a.data"] == b"new committed bytes"
    assert journal.is_file()

    with pytest.raises(_PublicationError, match="cleanup is still pending"):
        _publish(
            destination,
            receipt=changed,
            replace=True,
            filesystem=_PublicationFileSystem(
                _Fault("before_rmdir_tree:previous", OSError, once=False)
            ),
        )
    assert (destination / "a.data").read_bytes() == b"new committed bytes"

    final_bytes = dict(FILE_BYTES)
    final_bytes["a.data"] = b"third committed bytes"
    assert _publish(destination, files=final_bytes, replace=True).committed
    assert (destination / "a.data").read_bytes() == b"third committed bytes"


class _RecordingFileSystem(_PublicationFileSystem):
    def __init__(self) -> None:
        super().__init__()
        self.lock_order: list[tuple[Path, bool]] = []

    def lock(self, path: Path, *, shared: bool):
        self.lock_order.append((path, shared))
        return super().lock(path, shared=shared)


class _ActiveReadFileSystem(_RecordingFileSystem):
    def __init__(self) -> None:
        super().__init__()
        self.shared_lock_active = False

    @contextmanager
    def lock(self, path: Path, *, shared: bool):
        self.lock_order.append((path, shared))
        with super(_RecordingFileSystem, self).lock(path, shared=shared):
            self.shared_lock_active = shared
            try:
                yield
            finally:
                self.shared_lock_active = False


def test_load_generated_bundle_reads_entire_snapshot_under_shared_lock(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    destination = tmp_path / "generated"
    _publish(destination)
    filesystem = _ActiveReadFileSystem()
    original_read = generated_bundle._read_stable_file_bytes
    checked_reads = 0

    def read_while_locked(path: Path) -> bytes:
        nonlocal checked_reads
        assert filesystem.shared_lock_active
        checked_reads += 1
        return original_read(path)

    monkeypatch.setattr(generated_bundle, "_read_stable_file_bytes", read_while_locked)

    snapshot = _load_generated_bundle(destination, owner=OWNER, filesystem=filesystem)

    assert isinstance(snapshot, _GeneratedBundleSnapshot)
    assert checked_reads >= len(FILE_BYTES) + 1
    assert filesystem.lock_order == [
        (
            destination.parent / f".{destination.name}.pycircuit-publication" / "lock",
            True,
        )
    ]


def _process_replace_generated(destination: str, ready: Queue, done: Queue) -> None:
    ready.put("ready")
    try:
        _publish(
            Path(destination),
            files={**FILE_BYTES, "a.data": b"writer bytes"},
            replace=True,
        )
    except BaseException as error:
        done.put(("error", repr(error)))
    else:
        done.put(("done", "writer bytes"))


def test_managed_bundle_reader_holds_shared_lock_against_validating_writer(
    tmp_path: Path,
) -> None:
    destination = tmp_path / "generated"
    _publish(destination)
    filesystem = _RecordingFileSystem()
    request = _PublicationInput(
        destination,
        OWNER,
        _validate_generated_bundle,
        _validate_generated_bundle,
    )
    process_context = multiprocessing.get_context("spawn")
    ready = process_context.Queue()
    done = process_context.Queue()
    process = process_context.Process(
        target=_process_replace_generated, args=(str(destination), ready, done)
    )

    try:
        with _publication_lock_set(inputs=[request], filesystem=filesystem) as locks:
            snapshot = locks.snapshot(
                destination,
                lambda path: _read_generated_bundle(path, OWNER, filesystem),
            )
            assert isinstance(snapshot, _GeneratedBundleSnapshot)
            assert dict(snapshot.files)["a.data"] == FILE_BYTES["a.data"]
            process.start()
            assert ready.get(timeout=5) == "ready"
            with pytest.raises(queue.Empty):
                done.get(timeout=0.25)
            assert dict(snapshot.files)["a.data"] == FILE_BYTES["a.data"]
        assert done.get(timeout=5) == ("done", "writer bytes")
        process.join(timeout=5)
        assert process.exitcode == 0
    finally:
        if process.is_alive():
            process.terminate()
            process.join(timeout=5)

    control_lock = (
        destination.parent / f".{destination.name}.pycircuit-publication" / "lock"
    )
    assert (control_lock, True) in filesystem.lock_order
    assert _load_generated_bundle(destination, owner=OWNER).files
    assert (destination / "a.data").read_bytes() == b"writer bytes"

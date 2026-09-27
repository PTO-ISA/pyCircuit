"""Private crash-recoverable publication of compiler output directories."""

from __future__ import annotations

import os
import unicodedata
from collections.abc import Callable, Iterator, Mapping, Sequence
from contextlib import ExitStack, contextmanager
from dataclasses import dataclass
from pathlib import Path, PurePosixPath

from ._publication_fs import _PublicationFileSystem, _PublicationFileSystemError

_CONTROL_NAMES = {
    "owner.json",
    "owner.json.tmp",
    "lock",
    "journal.json",
    "journal.json.tmp",
    "stage",
    "previous",
}
_PHASES = {
    "preparing",
    "prepared",
    "rollback_restore",
    "rollback_cleanup",
    "committed",
}
_ARTIFACT_KINDS = {"source-unit", "generated", "program"}


class _PublicationError(RuntimeError):
    """The destination is unsafe, damaged, or owned by another artifact."""


class _PublicationCancelledError(_PublicationError):
    """A pre-commit cancellation; command adapters map this to exit 130."""

    exit_code = 130


@dataclass(frozen=True, slots=True)
class _PublicationResult:
    committed: bool
    warnings: tuple[str, ...] = ()

    @property
    def cleanup_pending(self) -> bool:
        return "publication_cleanup_pending" in self.warnings


@dataclass(frozen=True, slots=True)
class _Paths:
    destination: Path
    control: Path
    owner: Path
    owner_tmp: Path
    lock: Path
    journal: Path
    journal_tmp: Path
    stage: Path
    previous: Path


@dataclass(frozen=True, slots=True)
class _PublicationInput:
    destination: str | Path
    owner: Mapping[str, object]
    stable_validate: _Validator
    recovery_validate: _Validator


@dataclass(frozen=True, slots=True)
class _PublicationOutput:
    destination: str | Path
    recovery_validate: _Validator


@dataclass(frozen=True, slots=True)
class _LockEntry:
    paths: _Paths
    exclusive: bool
    owner: _Owner | None
    stable_validate: _Validator | None
    recovery_validate: _Validator


class _PublicationLocks:
    """An active, command-wide set of publication locks."""

    def __init__(
        self, filesystem: _PublicationFileSystem, entries: Sequence[_LockEntry]
    ) -> None:
        self.filesystem = filesystem
        self._entries = {
            _normalized_path_parts(entry.paths.destination): entry for entry in entries
        }
        self._active = True

    def close(self) -> None:
        self._active = False

    def require(self, destination: str | Path, *, exclusive: bool) -> _Paths:
        if not self._active:
            raise _PublicationError("publication lock set is no longer active")
        absolute = self.filesystem.absolute(destination)
        entry = self._entries.get(_normalized_path_parts(absolute))
        if entry is None or (exclusive and not entry.exclusive):
            mode = "exclusive" if exclusive else "shared"
            raise _PublicationError(
                f"publication destination is not held with {mode} access: {absolute}"
            )
        return entry.paths

    def snapshot(
        self, destination: str | Path, read: Callable[[Path], object]
    ) -> object:
        """Read one declared input while the command-wide lock set is held."""

        paths = self.require(destination, exclusive=False)
        entry = self._entries[_normalized_path_parts(paths.destination)]
        if entry.exclusive:
            raise _PublicationError("publication output is not an input snapshot")
        return read(paths.destination)


_Owner = dict[str, object]
_Validator = Callable[[Path, Mapping[str, object]], None]
_Builder = Callable[[Path], None]
_Cancelled = Callable[[str], bool]


def _publication_owner_source_unit(*, package: str, path: str) -> _Owner:
    return {"kind": "source-unit", "source": {"package": package, "path": path}}


def _publication_owner_generated(
    *, package: str, path: str, definition: str, target: str
) -> _Owner:
    return {
        "kind": "generated",
        "source": {"package": package, "path": path},
        "definition": definition,
        "target": target,
    }


def _publication_owner_program(*, package: str, path: str, definition: str) -> _Owner:
    return {
        "kind": "program",
        "source": {"package": package, "path": path},
        "definition": definition,
    }


@contextmanager
def _publication_lock_set(
    *,
    inputs: Sequence[_PublicationInput] = (),
    outputs: Sequence[_PublicationOutput] = (),
    filesystem: _PublicationFileSystem | None = None,
) -> Iterator[_PublicationLocks]:
    """Acquire one command's shared inputs and exclusive outputs in path order."""

    fs = filesystem or _PublicationFileSystem()
    entries = _lock_entries(inputs, outputs, fs)
    while True:
        recover_input: _LockEntry | None = None
        with ExitStack() as stack:
            for entry in entries:
                if entry.exclusive:
                    _bootstrap(entry.paths, fs)
                else:
                    _require_initialized_control(entry.paths, fs)
            for entry in entries:
                stack.enter_context(
                    fs.lock(entry.paths.lock, shared=not entry.exclusive)
                )
            for entry in entries:
                _validate_control(entry.paths, fs, cleanup_temporary=entry.exclusive)
                if entry.exclusive:
                    _recover_locked(entry.paths, entry.recovery_validate, fs)
                    continue
                journal = _read_journal(entry.paths, fs, cleanup_temporary=False)
                if journal is not None and journal["phase"] != "committed":
                    recover_input = entry
                    break
                assert entry.owner is not None
                assert entry.stable_validate is not None
                if journal is not None and journal["owner"] != entry.owner:
                    raise _PublicationError("published input owner does not match")
                _validate_stable_state(entry.paths, journal, fs)
                _validate_stable_artifact(
                    entry.paths.destination,
                    entry.owner,
                    entry.stable_validate,
                    fs,
                )
            if recover_input is None:
                locks = _PublicationLocks(fs, entries)
                try:
                    yield locks
                finally:
                    locks.close()
                return
        assert recover_input is not None
        _recover_publication(
            recover_input.paths.destination,
            validate=recover_input.recovery_validate,
            filesystem=fs,
        )


def _lock_entries(
    inputs: Sequence[_PublicationInput],
    outputs: Sequence[_PublicationOutput],
    fs: _PublicationFileSystem,
) -> tuple[_LockEntry, ...]:
    entries: list[_LockEntry] = []
    for request in inputs:
        owner = _validate_owner(request.owner)
        entries.append(
            _LockEntry(
                paths=_paths_for(request.destination, fs),
                exclusive=False,
                owner=owner,
                stable_validate=request.stable_validate,
                recovery_validate=request.recovery_validate,
            )
        )
    for request in outputs:
        entries.append(
            _LockEntry(
                paths=_paths_for(request.destination, fs),
                exclusive=True,
                owner=None,
                stable_validate=None,
                recovery_validate=request.recovery_validate,
            )
        )
    for index, left in enumerate(entries):
        for right in entries[index + 1 :]:
            if _paths_conflict(left.paths.destination, right.paths.destination):
                raise _PublicationError(
                    "publication lock paths are equal or ancestor-related: "
                    f"{left.paths.destination} and {right.paths.destination}"
                )
    return tuple(
        sorted(
            entries,
            key=lambda entry: _normalized_path_parts(entry.paths.destination),
        )
    )


def _paths_conflict(left: Path, right: Path) -> bool:
    left_parts = _normalized_path_parts(left)
    right_parts = _normalized_path_parts(right)
    limit = min(len(left_parts), len(right_parts))
    return left_parts[:limit] == right_parts[:limit]


def _normalized_path_parts(path: Path) -> tuple[str, ...]:
    return tuple(
        unicodedata.normalize("NFC", os.path.normcase(component)).casefold()
        for component in path.parts
    )


def _publish_directory(
    destination: str | Path,
    *,
    owner: Mapping[str, object],
    build: _Builder,
    validate: _Validator,
    replace: bool = False,
    cancelled: _Cancelled | None = None,
    filesystem: _PublicationFileSystem | None = None,
    locks: _PublicationLocks | None = None,
) -> _PublicationResult:
    """Build and atomically publish one owned directory."""

    expected_owner = _validate_owner(owner)
    if expected_owner["kind"] == "program":
        raise _PublicationError("program publication requires a single file")
    return _publish_artifact(
        destination,
        owner=expected_owner,
        build=build,
        validate=validate,
        replace=replace,
        cancelled=cancelled,
        filesystem=filesystem,
        locks=locks,
    )


def _publish_file(
    destination: str | Path,
    *,
    owner: Mapping[str, object],
    build: _Builder,
    validate: _Validator,
    replace: bool = False,
    cancelled: _Cancelled | None = None,
    filesystem: _PublicationFileSystem | None = None,
    locks: _PublicationLocks | None = None,
) -> _PublicationResult:
    """Build and atomically publish one owned ``program.ac`` file."""

    expected_owner = _validate_owner(owner)
    if expected_owner["kind"] != "program":
        raise _PublicationError("single-file publication requires a program owner")
    return _publish_artifact(
        destination,
        owner=expected_owner,
        build=build,
        validate=validate,
        replace=replace,
        cancelled=cancelled,
        filesystem=filesystem,
        locks=locks,
    )


def _publish_artifact(
    destination: str | Path,
    *,
    owner: Mapping[str, object],
    build: _Builder,
    validate: _Validator,
    replace: bool,
    cancelled: _Cancelled | None,
    filesystem: _PublicationFileSystem | None,
    locks: _PublicationLocks | None = None,
) -> _PublicationResult:
    """Build and atomically publish one artifact using the shared protocol.

    ``build`` must populate the supplied empty fixed stage path. ``validate``
    must verify its receipt, exact file set, and owner. Fault injection should
    raise ``BaseException`` to model abrupt process termination, and ``Exception``
    to model an observable I/O failure that can be rolled back immediately.
    """

    if (
        locks is not None
        and filesystem is not None
        and locks.filesystem is not filesystem
    ):
        raise _PublicationError("publication lock set uses a different filesystem")
    fs = (
        locks.filesystem
        if locks is not None
        else (filesystem or _PublicationFileSystem())
    )
    expected_owner = _validate_owner(owner)
    if locks is None:
        with _publication_lock_set(
            outputs=[_PublicationOutput(destination, validate)], filesystem=fs
        ) as acquired:
            return _publish_artifact(
                destination,
                owner=expected_owner,
                build=build,
                validate=validate,
                replace=replace,
                cancelled=cancelled,
                filesystem=fs,
                locks=acquired,
            )
    paths = locks.require(destination, exclusive=True)
    cancel = cancelled or (lambda _point: False)
    if cancel("before_prepare"):
        raise _PublicationCancelledError("publication cancelled before preparation")

    had_previous = _artifact_exists(paths.destination, expected_owner, fs)
    if had_previous:
        if not replace:
            raise _PublicationError("publication destination already exists")
        _validate_artifact(paths.destination, expected_owner, validate, fs)

    journal = _journal(expected_owner, paths.destination.name, had_previous)
    committed = False
    try:
        _write_journal(paths, journal, fs)
        fs.fault("after_journal_preparing")
        _cancel_or_raise(cancel, "after_preparing")

        if _artifact_kind(expected_owner) == "directory":
            fs.mkdir(paths.stage)
        build(paths.stage)
        _validate_artifact(paths.stage, expected_owner, validate, fs)
        _sync_artifact(paths.stage, expected_owner, fs)
        fs.fault("after_stage_complete")
        _cancel_or_raise(cancel, "after_stage")

        journal["phase"] = "prepared"
        _write_journal(paths, journal, fs)
        fs.fault("after_journal_prepared")
        _cancel_or_raise(cancel, "after_prepared")

        if had_previous:
            fs.rename(paths.destination, paths.previous)
            fs.fault("after_previous_saved")
            _cancel_or_raise(cancel, "after_previous_saved")
        fs.rename(paths.stage, paths.destination)
        fs.fault("after_destination_installed")
        _cancel_or_raise(cancel, "after_destination_installed")
        _validate_artifact(paths.destination, expected_owner, validate, fs)

        journal["phase"] = "committed"
        _write_journal(paths, journal, fs)
        committed = True
        fs.fault("after_journal_committed")
    except Exception:
        authoritative = _read_journal(paths, fs)
        if authoritative is not None and authoritative["phase"] == "committed":
            return _finish_committed(paths, authoritative, validate, fs)
        if not committed:
            _recover_locked(paths, validate, fs)
            raise
    cancel("after_committed")

    return _finish_committed(paths, journal, validate, fs)


def _recover_publication(
    destination: str | Path,
    *,
    validate: _Validator,
    filesystem: _PublicationFileSystem | None = None,
) -> _PublicationResult:
    """Recover any interrupted transaction under the destination's lock."""

    fs = filesystem or _PublicationFileSystem()
    paths = _paths_for(destination, fs)
    _bootstrap(paths, fs)
    with fs.lock(paths.lock, shared=False):
        _validate_control(paths, fs)
        journal = _read_journal(paths, fs)
        if journal is None:
            return _PublicationResult(committed=False)
        was_committed = journal["phase"] == "committed"
        _recover_locked(paths, validate, fs)
        return _PublicationResult(committed=was_committed)


def _read_published(
    destination: str | Path,
    *,
    owner: Mapping[str, object],
    stable_validate: _Validator,
    recovery_validate: _Validator,
    read: Callable[[Path], object],
    filesystem: _PublicationFileSystem | None = None,
) -> object:
    """Validate and read a committed artifact while holding a shared lock."""

    fs = filesystem or _PublicationFileSystem()
    expected_owner = _validate_owner(owner)
    paths = _paths_for(destination, fs)
    _require_initialized_control(paths, fs)

    while True:
        needs_recovery = False
        with fs.lock(paths.lock, shared=True):
            _validate_control(paths, fs, cleanup_temporary=False)
            journal = _read_journal(paths, fs, cleanup_temporary=False)
            if journal is not None and journal["phase"] != "committed":
                needs_recovery = True
            else:
                if journal is not None and journal["owner"] != expected_owner:
                    raise _PublicationError("published artifact owner does not match")
                _validate_stable_state(paths, journal, fs)
                _validate_stable_artifact(
                    paths.destination, expected_owner, stable_validate, fs
                )
                return read(paths.destination)
        if needs_recovery:
            _recover_publication(destination, validate=recovery_validate, filesystem=fs)


def _paths_for(destination: str | Path, fs: _PublicationFileSystem) -> _Paths:
    absolute = fs.absolute(destination)
    if absolute.name in {"", ".", ".."}:
        raise ValueError("publication destination must have a file name")
    if not absolute.parent.is_dir():
        raise _PublicationError("publication parent does not exist")
    fs.assert_no_symlink_chain(absolute.parent)
    control = absolute.parent / f".{absolute.name}.pycircuit-publication"
    return _Paths(
        destination=absolute,
        control=control,
        owner=control / "owner.json",
        owner_tmp=control / "owner.json.tmp",
        lock=control / "lock",
        journal=control / "journal.json",
        journal_tmp=control / "journal.json.tmp",
        stage=control / "stage",
        previous=control / "previous",
    )


def _bootstrap(paths: _Paths, fs: _PublicationFileSystem) -> None:
    control_kind = fs.kind(paths.control)
    if control_kind is None:
        try:
            fs.mkdir(paths.control)
        except FileExistsError:
            pass
        if fs.kind(paths.control) != "directory":
            raise _PublicationError("publication control path is not a directory")
    elif control_kind != "directory":
        raise _PublicationError("publication control path is not a directory")
    fs.assert_no_symlink_chain(paths.control)

    entries = {path.name for path in paths.control.iterdir()}
    unexpected = entries - _CONTROL_NAMES
    if unexpected:
        raise _PublicationError(
            f"publication control contains unknown entries: {sorted(unexpected)!r}"
        )
    owner_kind = fs.kind(paths.owner)
    lock_kind = fs.kind(paths.lock)
    if owner_kind is None:
        if entries - {"lock", "owner.json.tmp"}:
            raise _PublicationError(
                "uninitialized publication control has transaction data"
            )
        if lock_kind is None:
            try:
                fs.create_lock(paths.lock)
            except FileExistsError:
                pass
            if fs.kind(paths.lock) != "file":
                raise _PublicationError("publication lock is not a regular file")
        elif lock_kind != "file":
            raise _PublicationError("publication lock is not a regular file")
        with fs.lock(paths.lock, shared=False):
            if fs.kind(paths.owner) is None:
                marker = {
                    "kind": "pycircuit-publication-control",
                    "destination": paths.destination.name,
                }
                fs.write_json_atomic(
                    paths.owner,
                    paths.owner_tmp,
                    marker,
                    verify_temporary=lambda value: _verify_control_marker(
                        value, marker
                    ),
                )
                if _read_control_owner(paths, fs) != marker:
                    raise _PublicationError(
                        "publication control owner verification failed"
                    )
    else:
        if owner_kind != "file":
            raise _PublicationError("publication control owner is not a regular file")
        if lock_kind != "file":
            raise _PublicationError("initialized publication control has no valid lock")
        _read_control_owner(paths, fs)


def _read_control_owner(paths: _Paths, fs: _PublicationFileSystem) -> dict[str, str]:
    value = fs.read_json(paths.owner)
    expected = {
        "kind": "pycircuit-publication-control",
        "destination": paths.destination.name,
    }
    if value != expected:
        raise _PublicationError("publication control owner is invalid")
    return expected


def _require_initialized_control(paths: _Paths, fs: _PublicationFileSystem) -> None:
    if fs.kind(paths.control) != "directory":
        raise _PublicationError("unmanaged publication input is not accepted")
    fs.assert_no_symlink_chain(paths.control)
    if fs.kind(paths.owner) != "file" or fs.kind(paths.lock) != "file":
        raise _PublicationError("publication input control is not initialized")
    _read_control_owner(paths, fs)


def _verify_control_marker(value: object, expected: dict[str, str]) -> None:
    if value != expected:
        raise _PublicationError("publication control owner verification failed")


def _validate_control(
    paths: _Paths,
    fs: _PublicationFileSystem,
    *,
    cleanup_temporary: bool = True,
) -> None:
    _read_control_owner(paths, fs)
    fs.require_kind(paths.lock, "file")
    entries = {path.name for path in paths.control.iterdir()}
    unexpected = entries - _CONTROL_NAMES
    if unexpected:
        raise _PublicationError(
            f"publication control contains unknown entries: {sorted(unexpected)!r}"
        )
    for path in (paths.owner_tmp, paths.journal, paths.journal_tmp):
        if fs.kind(path) not in {None, "file"}:
            raise _PublicationError(f"publication metadata is not a file: {path.name}")
    for path in (paths.stage, paths.previous):
        if fs.kind(path) not in {None, "directory", "file"}:
            raise _PublicationError(
                f"publication transaction path is unsafe: {path.name}"
            )
    if cleanup_temporary:
        fs.remove_file(paths.owner_tmp)


def _journal(owner: _Owner, destination: str, had_previous: bool) -> _Owner:
    return {
        "kind": "pycircuit-publication",
        "artifact": owner["kind"],
        "destination": destination,
        "owner": owner,
        "had_previous": had_previous,
        "phase": "preparing",
    }


def _write_journal(paths: _Paths, journal: _Owner, fs: _PublicationFileSystem) -> None:
    _validate_journal(journal, paths.destination.name)
    fs.write_json_atomic(paths.journal, paths.journal_tmp, journal)


def _read_journal(
    paths: _Paths, fs: _PublicationFileSystem, *, cleanup_temporary: bool = True
) -> _Owner | None:
    journal_kind = fs.kind(paths.journal)
    tmp_kind = fs.kind(paths.journal_tmp)
    if journal_kind is None:
        if tmp_kind == "file":
            if fs.kind(paths.stage) is not None or fs.kind(paths.previous) is not None:
                raise _PublicationError(
                    "journal temporary exists with transaction directories"
                )
            if cleanup_temporary:
                fs.remove_file(paths.journal_tmp)
        return None
    value = fs.read_json(paths.journal)
    journal = _validate_journal(value, paths.destination.name)
    if tmp_kind == "file" and cleanup_temporary:
        fs.remove_file(paths.journal_tmp)
    return journal


def _validate_journal(value: object, destination: str) -> _Owner:
    if type(value) is not dict:
        raise _PublicationError("publication journal is not an object")
    required = {
        "kind",
        "artifact",
        "destination",
        "owner",
        "had_previous",
        "phase",
    }
    if set(value) != required:
        raise _PublicationError("publication journal fields are not closed")
    if value["kind"] != "pycircuit-publication":
        raise _PublicationError("publication journal kind is invalid")
    if type(value["destination"]) is not str or value["destination"] != destination:
        raise _PublicationError("publication journal destination is invalid")
    if type(value["had_previous"]) is not bool:
        raise _PublicationError("publication journal had_previous is invalid")
    if type(value["phase"]) is not str or value["phase"] not in _PHASES:
        raise _PublicationError("publication journal phase is invalid")
    owner = _validate_owner(value["owner"])
    if (
        type(value["artifact"]) is not str
        or value["artifact"] not in _ARTIFACT_KINDS
        or value["artifact"] != owner["kind"]
    ):
        raise _PublicationError("publication journal artifact is invalid")
    return dict(value)


def _validate_owner(value: object) -> _Owner:
    if type(value) is not dict:
        raise _PublicationError("publication owner is not an object")
    kind = value.get("kind")
    fields = {
        "source-unit": {"kind", "source"},
        "generated": {"kind", "source", "definition", "target"},
        "program": {"kind", "source", "definition"},
    }
    if type(kind) is not str or kind not in fields or set(value) != fields[kind]:
        raise _PublicationError("publication owner fields are not closed")
    source = value["source"]
    if type(source) is not dict or set(source) != {"package", "path"}:
        raise _PublicationError("publication source owner fields are not closed")
    package = source["package"]
    path = source["path"]
    if type(package) is not str or type(path) is not str:
        raise _PublicationError("publication source owner values must be strings")
    _validate_package(package)
    _validate_source_path(path)
    result: _Owner = {"kind": kind, "source": dict(source)}
    if kind in {"generated", "program"}:
        definition = value["definition"]
        if type(definition) is not str:
            raise _PublicationError("publication definition is invalid")
        _validate_qualified_symbol(definition)
        result["definition"] = definition
    if kind == "generated":
        target = value["target"]
        if type(target) is not str or target not in {"cpp", "verilog"}:
            raise _PublicationError("publication target is invalid")
        result["target"] = target
    return result


def _validate_package(package: str) -> None:
    if not package:
        return
    components = package.split(".")
    if any(not component or not component.isidentifier() for component in components):
        raise _PublicationError("publication source package is invalid")


def _validate_source_path(path: str) -> None:
    if not path or "\\" in path or "\x00" in path:
        raise _PublicationError("publication source path is invalid")
    components = path.split("/")
    parsed = PurePosixPath(path)
    if parsed.is_absolute() or parsed.suffix != ".py":
        raise _PublicationError("publication source path is invalid")
    if any(component in {"", ".", ".."} for component in components):
        raise _PublicationError("publication source path is invalid")


def _validate_qualified_symbol(symbol: str) -> None:
    if len(symbol) < 4 or not symbol.startswith('@"') or not symbol.endswith('"'):
        raise _PublicationError("publication definition is not canonical")
    components = symbol[2:-1].split(".")
    if any(not component or not component.isidentifier() for component in components):
        raise _PublicationError("publication definition is not canonical")


def _validate_artifact(
    path: Path,
    owner: Mapping[str, object],
    validate: _Validator,
    fs: _PublicationFileSystem,
) -> None:
    expected_kind = _artifact_kind(owner)
    actual_kind = fs.kind(path)
    if actual_kind != expected_kind:
        raise _PublicationError(
            f"publication artifact has type {actual_kind!r}, "
            f"expected {expected_kind}: {path}"
        )
    if expected_kind == "directory":
        fs.validate_plain_tree(path)
    try:
        validate(path, owner)
    except _PublicationError:
        raise
    except Exception as error:
        raise _PublicationError(
            f"published artifact validation failed: {path}"
        ) from error


def _validate_stable_artifact(
    path: Path,
    owner: Mapping[str, object],
    validate: _Validator,
    fs: _PublicationFileSystem,
) -> None:
    expected_kind = _artifact_kind(owner)
    if fs.kind(path) != expected_kind:
        raise _PublicationError(
            f"published stable artifact is missing or unsafe: {path}"
        )
    try:
        validate(path, owner)
    except _PublicationError:
        raise
    except Exception as error:
        raise _PublicationError(
            f"published stable view validation failed: {path}"
        ) from error


def _validate_stable_state(
    paths: _Paths, journal: _Owner | None, fs: _PublicationFileSystem
) -> None:
    if journal is None:
        if fs.kind(paths.stage) is not None or fs.kind(paths.previous) is not None:
            raise _PublicationError("transaction artifacts exist without journal")
        return
    if journal["phase"] != "committed":
        raise _PublicationError("stable read requires a committed publication")
    if fs.kind(paths.stage) is not None:
        raise _PublicationError("committed publication still has a stage artifact")
    if not journal["had_previous"] and fs.kind(paths.previous) is not None:
        raise _PublicationError("new committed publication has previous output")


def _artifact_kind(owner: Mapping[str, object]) -> str:
    return "file" if owner["kind"] == "program" else "directory"


def _artifact_exists(
    path: Path, owner: Mapping[str, object], fs: _PublicationFileSystem
) -> bool:
    kind = fs.kind(path)
    expected = _artifact_kind(owner)
    if kind not in {None, expected}:
        raise _PublicationError(
            f"publication artifact has type {kind!r}, expected {expected}: {path}"
        )
    return kind == expected


def _sync_artifact(
    path: Path, owner: Mapping[str, object], fs: _PublicationFileSystem
) -> None:
    if _artifact_kind(owner) == "file":
        fs.sync_file(path)
    else:
        fs.sync_tree(path)


def _remove_artifact(
    path: Path, owner: Mapping[str, object], fs: _PublicationFileSystem
) -> None:
    kind = fs.kind(path)
    if kind is None:
        return
    expected = _artifact_kind(owner)
    if kind != expected:
        raise _PublicationError(
            f"publication cleanup has type {kind!r}, expected {expected}: {path}"
        )
    if expected == "file":
        fs.remove_file(path)
    else:
        fs.remove_tree(path)


def _recover_locked(
    paths: _Paths, validate: _Validator, fs: _PublicationFileSystem
) -> None:
    journal = _read_journal(paths, fs)
    if journal is None:
        if fs.kind(paths.stage) is not None or fs.kind(paths.previous) is not None:
            raise _PublicationError("transaction artifacts exist without journal")
        return
    phase = journal["phase"]
    if phase == "committed":
        result = _finish_committed(paths, journal, validate, fs)
        if result.cleanup_pending:
            raise _PublicationError("committed publication cleanup is still pending")
        return
    if phase == "preparing":
        _validate_preparing(paths, journal, validate, fs)
        journal["phase"] = "rollback_cleanup"
        _write_journal(paths, journal, fs)
        fs.fault("after_journal_rollback_cleanup")
    elif phase == "prepared":
        _validate_prepared_combination(paths, journal, fs)
        journal["phase"] = "rollback_restore"
        _write_journal(paths, journal, fs)
        fs.fault("after_journal_rollback_restore")

    if journal["phase"] == "rollback_restore":
        _restore_previous(paths, journal, fs)
        _validate_restored(paths, journal, validate, fs)
        journal["phase"] = "rollback_cleanup"
        _write_journal(paths, journal, fs)
        fs.fault("after_journal_rollback_cleanup")
    if journal["phase"] == "rollback_cleanup":
        _validate_restored(paths, journal, validate, fs)
        _remove_artifact(paths.stage, journal["owner"], fs)
        fs.remove_file(paths.journal_tmp)
        fs.remove_file(paths.journal)


def _validate_preparing(
    paths: _Paths, journal: _Owner, validate: _Validator, fs: _PublicationFileSystem
) -> None:
    had_previous = bool(journal["had_previous"])
    if fs.kind(paths.previous) is not None:
        raise _PublicationError(
            "preparing transaction unexpectedly has previous output"
        )
    destination_exists = _artifact_exists(paths.destination, journal["owner"], fs)
    if destination_exists != had_previous:
        raise _PublicationError("preparing transaction destination state is invalid")
    if destination_exists:
        _validate_artifact(paths.destination, journal["owner"], validate, fs)


def _validate_prepared_combination(
    paths: _Paths, journal: _Owner, fs: _PublicationFileSystem
) -> None:
    had_previous = bool(journal["had_previous"])
    destination = _artifact_exists(paths.destination, journal["owner"], fs)
    stage = _artifact_exists(paths.stage, journal["owner"], fs)
    previous = _artifact_exists(paths.previous, journal["owner"], fs)
    if had_previous:
        valid = (
            (previous and not destination)
            or (previous and destination and not stage)
            or (not previous and destination and stage)
        )
    else:
        valid = not previous and not (destination and stage)
    if not valid:
        raise _PublicationError("prepared transaction path combination is invalid")


def _restore_previous(
    paths: _Paths, journal: _Owner, fs: _PublicationFileSystem
) -> None:
    had_previous = bool(journal["had_previous"])
    destination = _artifact_exists(paths.destination, journal["owner"], fs)
    stage = _artifact_exists(paths.stage, journal["owner"], fs)
    previous = _artifact_exists(paths.previous, journal["owner"], fs)
    if had_previous:
        if previous and not destination:
            fs.rename(paths.previous, paths.destination)
        elif previous and destination and not stage:
            fs.rename(paths.destination, paths.stage)
            fs.rename(paths.previous, paths.destination)
        elif not previous and destination and stage:
            return
        else:
            raise _PublicationError("rollback restore path combination is invalid")
    else:
        if previous:
            raise _PublicationError("new publication unexpectedly has previous output")
        if destination and not stage:
            fs.rename(paths.destination, paths.stage)
        elif not destination:
            return
        else:
            raise _PublicationError("new publication rollback state is invalid")


def _validate_restored(
    paths: _Paths, journal: _Owner, validate: _Validator, fs: _PublicationFileSystem
) -> None:
    had_previous = bool(journal["had_previous"])
    if fs.kind(paths.previous) is not None:
        raise _PublicationError("rollback left a previous artifact")
    destination_exists = _artifact_exists(paths.destination, journal["owner"], fs)
    if destination_exists != had_previous:
        raise _PublicationError("rollback did not restore destination state")
    if destination_exists:
        _validate_artifact(paths.destination, journal["owner"], validate, fs)


def _finish_committed(
    paths: _Paths, journal: _Owner, validate: _Validator, fs: _PublicationFileSystem
) -> _PublicationResult:
    if fs.kind(paths.stage) is not None:
        raise _PublicationError("committed publication still has a stage artifact")
    _validate_artifact(paths.destination, journal["owner"], validate, fs)
    if not journal["had_previous"] and fs.kind(paths.previous) is not None:
        raise _PublicationError("new committed publication has previous output")
    try:
        _remove_artifact(paths.previous, journal["owner"], fs)
        fs.remove_file(paths.journal_tmp)
        fs.remove_file(paths.journal)
    except OSError:
        if fs.kind(paths.journal) is None:
            return _PublicationResult(committed=True)
        return _PublicationResult(
            committed=True, warnings=("publication_cleanup_pending",)
        )
    return _PublicationResult(committed=True)


def _cancel_or_raise(cancelled: _Cancelled, point: str) -> None:
    if cancelled(point):
        raise _PublicationCancelledError(f"publication cancelled at {point}")


__all__ = [
    "_PublicationCancelledError",
    "_PublicationError",
    "_PublicationFileSystem",
    "_PublicationFileSystemError",
    "_PublicationInput",
    "_PublicationLocks",
    "_PublicationOutput",
    "_PublicationResult",
    "_publication_lock_set",
    "_publication_owner_generated",
    "_publication_owner_program",
    "_publication_owner_source_unit",
    "_publish_directory",
    "_publish_file",
    "_read_published",
    "_recover_publication",
]

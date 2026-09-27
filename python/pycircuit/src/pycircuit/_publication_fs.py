"""Filesystem primitives for private directory publication transactions."""

from __future__ import annotations

import errno
import json
import os
import stat
import time
from collections.abc import Callable, Mapping
from contextlib import AbstractContextManager
from pathlib import Path


class _PublicationFileSystemError(RuntimeError):
    """A filesystem object violates the publication storage contract."""


class _FileLock(AbstractContextManager["_FileLock"]):
    def __init__(self, descriptor: int, *, shared: bool) -> None:
        self._descriptor = descriptor
        self._shared = shared

    def __enter__(self) -> _FileLock:
        try:
            if os.name == "nt":
                _lock_windows(self._descriptor, shared=self._shared)
            else:
                import fcntl

                operation = fcntl.LOCK_SH if self._shared else fcntl.LOCK_EX
                fcntl.flock(self._descriptor, operation)
        except BaseException:
            os.close(self._descriptor)
            raise
        return self

    def __exit__(self, *exc_info: object) -> None:
        try:
            if os.name == "nt":
                _unlock_windows(self._descriptor)
            else:
                import fcntl

                fcntl.flock(self._descriptor, fcntl.LOCK_UN)
        finally:
            os.close(self._descriptor)


def _lock_windows(descriptor: int, *, shared: bool) -> None:
    import ctypes
    import msvcrt
    from ctypes import wintypes

    class _Overlapped(ctypes.Structure):
        _fields_ = [
            ("Internal", ctypes.c_void_p),
            ("InternalHigh", ctypes.c_void_p),
            ("Offset", wintypes.DWORD),
            ("OffsetHigh", wintypes.DWORD),
            ("hEvent", wintypes.HANDLE),
        ]

    flags = 0 if shared else 0x00000002  # LOCKFILE_EXCLUSIVE_LOCK
    overlapped = _Overlapped()
    handle = msvcrt.get_osfhandle(descriptor)
    lock_file = ctypes.windll.kernel32.LockFileEx
    lock_file.argtypes = [
        wintypes.HANDLE,
        wintypes.DWORD,
        wintypes.DWORD,
        wintypes.DWORD,
        wintypes.DWORD,
        ctypes.POINTER(_Overlapped),
    ]
    lock_file.restype = wintypes.BOOL
    if not lock_file(handle, flags, 0, 1, 0, ctypes.byref(overlapped)):
        raise ctypes.WinError()


def _unlock_windows(descriptor: int) -> None:
    import ctypes
    import msvcrt
    from ctypes import wintypes

    class _Overlapped(ctypes.Structure):
        _fields_ = [
            ("Internal", ctypes.c_void_p),
            ("InternalHigh", ctypes.c_void_p),
            ("Offset", wintypes.DWORD),
            ("OffsetHigh", wintypes.DWORD),
            ("hEvent", wintypes.HANDLE),
        ]

    overlapped = _Overlapped()
    handle = msvcrt.get_osfhandle(descriptor)
    unlock_file = ctypes.windll.kernel32.UnlockFileEx
    unlock_file.argtypes = [
        wintypes.HANDLE,
        wintypes.DWORD,
        wintypes.DWORD,
        wintypes.DWORD,
        ctypes.POINTER(_Overlapped),
    ]
    unlock_file.restype = wintypes.BOOL
    if not unlock_file(handle, 0, 1, 0, ctypes.byref(overlapped)):
        raise ctypes.WinError()


class _PublicationFileSystem:
    """Strict local-filesystem operations with deterministic fault points."""

    def __init__(self, fault: Callable[[str], None] | None = None) -> None:
        self._inject_fault = fault or (lambda _point: None)

    def fault(self, point: str) -> None:
        self._inject_fault(point)

    def absolute(self, path: str | Path) -> Path:
        value = os.fspath(path)
        if "\x00" in value:
            raise ValueError("publication path contains NUL")
        return Path(os.path.abspath(value))

    def assert_no_symlink_chain(self, path: Path) -> None:
        current = Path(path.anchor)
        for component in path.parts[1:]:
            current /= component
            try:
                info = current.lstat()
            except FileNotFoundError:
                continue
            if stat.S_ISLNK(info.st_mode) or self._is_reparse_point(info):
                raise _PublicationFileSystemError(
                    f"publication path traverses symlink or reparse point: {current}"
                )

    def kind(self, path: Path) -> str | None:
        try:
            info = path.lstat()
        except FileNotFoundError:
            return None
        if stat.S_ISLNK(info.st_mode) or self._is_reparse_point(info):
            return "link"
        if stat.S_ISREG(info.st_mode):
            return "file"
        if stat.S_ISDIR(info.st_mode):
            return "directory"
        return "other"

    def require_kind(self, path: Path, expected: str) -> None:
        actual = self.kind(path)
        if actual != expected:
            raise _PublicationFileSystemError(
                f"publication object has type {actual!r}, expected {expected}: {path}"
            )

    def validate_plain_tree(self, root: Path) -> None:
        self.require_kind(root, "directory")
        for directory, names, files in os.walk(root, followlinks=False):
            directory_path = Path(directory)
            for name in [*names, *files]:
                child = directory_path / name
                kind = self.kind(child)
                if kind not in {"file", "directory"}:
                    raise _PublicationFileSystemError(
                        f"publication tree contains unsupported object: {child}"
                    )

    def mkdir(self, path: Path) -> None:
        self.fault(f"before_mkdir:{path.name}")
        path.mkdir()
        self.fault(f"after_mkdir:{path.name}")
        self.sync_directory(path.parent)

    def create_lock(self, path: Path) -> None:
        self.fault("before_create_lock")
        flags = os.O_CREAT | os.O_EXCL | os.O_RDWR
        flags |= getattr(os, "O_NOFOLLOW", 0)
        descriptor = os.open(path, flags, 0o600)
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)
        self.fault("after_create_lock")
        self.sync_directory(path.parent)

    def lock(self, path: Path, *, shared: bool) -> _FileLock:
        self.require_kind(path, "file")
        flags = os.O_RDONLY if shared else os.O_RDWR
        flags |= getattr(os, "O_NOFOLLOW", 0)
        descriptor = os.open(path, flags)
        opened = os.fstat(descriptor)
        current = path.stat(follow_symlinks=False)
        if (opened.st_dev, opened.st_ino) != (current.st_dev, current.st_ino):
            os.close(descriptor)
            raise _PublicationFileSystemError("publication lock changed while opening")
        return _FileLock(descriptor, shared=shared)

    def read_json(self, path: Path) -> object:
        self.require_kind(path, "file")
        flags = os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0)
        descriptor = os.open(path, flags)
        try:
            with os.fdopen(descriptor, "r", encoding="utf-8") as stream:
                return json.load(stream)
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise _PublicationFileSystemError(
                f"publication metadata is not valid JSON: {path}"
            ) from error

    def write_json_atomic(
        self,
        final: Path,
        temporary: Path,
        value: Mapping[str, object],
        *,
        verify_temporary: Callable[[object], None] | None = None,
    ) -> None:
        temporary_kind = self.kind(temporary)
        if temporary_kind not in {None, "file"}:
            raise _PublicationFileSystemError(
                f"publication temporary metadata is not a file: {temporary}"
            )
        self.fault(f"before_write:{temporary.name}")
        flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC
        flags |= getattr(os, "O_NOFOLLOW", 0)
        descriptor = os.open(temporary, flags, 0o600)
        try:
            encoded = json.dumps(
                value, ensure_ascii=False, separators=(",", ":"), sort_keys=True
            ).encode("utf-8")
            written = 0
            while written < len(encoded):
                count = os.write(descriptor, encoded[written:])
                if count == 0:
                    raise OSError(errno.EIO, "publication metadata write stalled")
                written += count
            os.fsync(descriptor)
        finally:
            os.close(descriptor)
        self.fault(f"after_flush:{temporary.name}")
        if verify_temporary is not None:
            verify_temporary(self.read_json(temporary))
        self.replace(temporary, final)

    def replace(self, source: Path, destination: Path) -> None:
        self.fault(f"before_replace:{source.name}:{destination.name}")
        self._retry_windows(lambda: os.replace(source, destination))
        self.fault(f"after_replace:{source.name}:{destination.name}")
        self.sync_directory(destination.parent)

    def rename(self, source: Path, destination: Path) -> None:
        if self.kind(destination) is not None:
            raise _PublicationFileSystemError(
                f"publication rename destination already exists: {destination}"
            )
        self.fault(f"before_rename:{source.name}:{destination.name}")
        self._retry_windows(lambda: source.rename(destination))
        self.fault(f"after_rename:{source.name}:{destination.name}")
        self.sync_directory(destination.parent)

    def remove_file(self, path: Path) -> None:
        kind = self.kind(path)
        if kind is None:
            return
        if kind != "file":
            raise _PublicationFileSystemError(
                f"publication cleanup expected a file: {path}"
            )
        self.fault(f"before_unlink:{path.name}")
        path.unlink()
        self.fault(f"after_unlink:{path.name}")
        self.sync_directory(path.parent)

    def remove_tree(self, root: Path) -> None:
        kind = self.kind(root)
        if kind is None:
            return
        self.validate_plain_tree(root)
        for directory, names, files in os.walk(root, topdown=False, followlinks=False):
            directory_path = Path(directory)
            for name in files:
                path = directory_path / name
                self.fault(f"before_unlink_tree:{name}")
                path.unlink()
            for name in names:
                path = directory_path / name
                self.require_kind(path, "directory")
                self.fault(f"before_rmdir_tree:{name}")
                path.rmdir()
        self.fault(f"before_rmdir_tree:{root.name}")
        root.rmdir()
        self.fault(f"after_rmdir_tree:{root.name}")
        self.sync_directory(root.parent)

    def sync_tree(self, root: Path) -> None:
        self.validate_plain_tree(root)
        for directory, _names, files in os.walk(root, topdown=False):
            directory_path = Path(directory)
            for name in files:
                descriptor = os.open(
                    directory_path / name,
                    os.O_RDONLY | getattr(os, "O_NOFOLLOW", 0),
                )
                try:
                    os.fsync(descriptor)
                finally:
                    os.close(descriptor)
            self.sync_directory(directory_path)

    def sync_directory(self, path: Path) -> None:
        if os.name == "nt":
            return
        descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)

    @staticmethod
    def _is_reparse_point(info: os.stat_result) -> bool:
        attributes = getattr(info, "st_file_attributes", 0)
        return bool(attributes & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0))

    @staticmethod
    def _retry_windows(operation: Callable[[], None]) -> None:
        attempts = 5 if os.name == "nt" else 1
        for attempt in range(attempts):
            try:
                operation()
                return
            except OSError as error:
                sharing = getattr(error, "winerror", None) in {5, 32, 33}
                if not sharing or attempt + 1 == attempts:
                    raise
                time.sleep(0.01 * (attempt + 1))
        raise OSError(errno.EIO, "unreachable publication rename retry failure")

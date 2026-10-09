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

_WINDOWS_SHARING_VIOLATIONS = {5, 32, 33}
_WINDOWS_RENAME_ATTEMPTS = 5


class _PublicationFileSystemError(RuntimeError):
    """A filesystem object violates the publication storage contract."""


def _same_file_identity(left: os.stat_result, right: os.stat_result) -> bool:
    return (left.st_dev, left.st_ino) == (right.st_dev, right.st_ino)


def _retry_sharing_violations(
    operation: Callable[[], None], *, attempts: int, delay: Callable[[float], None]
) -> None:
    for attempt in range(attempts):
        try:
            operation()
            return
        except OSError as error:
            sharing = getattr(error, "winerror", None) in _WINDOWS_SHARING_VIOLATIONS
            if not sharing or attempt + 1 == attempts:
                raise
            delay(0.01 * (attempt + 1))
    raise OSError(errno.EIO, "unreachable publication rename retry failure")


def _strict_json_object(pairs: list[tuple[str, object]]) -> dict[str, object]:
    result: dict[str, object] = {}
    for key, value in pairs:
        if key in result:
            raise _PublicationFileSystemError(
                f"publication metadata contains duplicate key: {key!r}"
            )
        result[key] = value
    return result


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


def _open_regular_windows(
    path: Path,
    *,
    writable: bool,
    create: bool,
    truncate: bool,
    exclusive: bool,
) -> int:
    import ctypes
    import msvcrt
    from ctypes import wintypes

    generic_read = 0x80000000
    generic_write = 0x40000000
    share_all = 0x00000001 | 0x00000002 | 0x00000004
    create_new = 1
    open_existing = 3
    file_attribute_normal = 0x00000080
    open_reparse_point = 0x00200000
    invalid_handle = wintypes.HANDLE(-1).value

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    create_file = kernel32.CreateFileW
    create_file.argtypes = [
        wintypes.LPCWSTR,
        wintypes.DWORD,
        wintypes.DWORD,
        ctypes.c_void_p,
        wintypes.DWORD,
        wintypes.DWORD,
        wintypes.HANDLE,
    ]
    create_file.restype = wintypes.HANDLE
    close_handle = kernel32.CloseHandle
    close_handle.argtypes = [wintypes.HANDLE]
    close_handle.restype = wintypes.BOOL

    desired_access = generic_read | (generic_write if writable else 0)
    disposition = create_new if create else open_existing
    handle = create_file(
        os.fspath(path),
        desired_access,
        share_all,
        None,
        disposition,
        file_attribute_normal | open_reparse_point,
        None,
    )
    if handle == invalid_handle and create and not exclusive:
        error = ctypes.get_last_error()
        if error in {80, 183}:  # ERROR_FILE_EXISTS, ERROR_ALREADY_EXISTS
            handle = create_file(
                os.fspath(path),
                desired_access,
                share_all,
                None,
                open_existing,
                file_attribute_normal | open_reparse_point,
                None,
            )
    if handle == invalid_handle:
        raise ctypes.WinError(ctypes.get_last_error())

    flags = os.O_RDWR if writable else os.O_RDONLY
    flags |= getattr(os, "O_BINARY", 0)
    try:
        descriptor = msvcrt.open_osfhandle(handle, flags)
    except BaseException:
        close_handle(handle)
        raise
    if truncate:
        try:
            os.ftruncate(descriptor, 0)
        except BaseException:
            os.close(descriptor)
            raise
    return descriptor


def _replace_windows(source: Path, destination: Path) -> None:
    import ctypes
    from ctypes import wintypes

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    replace_file = kernel32.ReplaceFileW
    replace_file.argtypes = [
        wintypes.LPCWSTR,
        wintypes.LPCWSTR,
        wintypes.LPCWSTR,
        wintypes.DWORD,
        ctypes.c_void_p,
        ctypes.c_void_p,
    ]
    replace_file.restype = wintypes.BOOL
    try:
        destination.lstat()
    except FileNotFoundError:
        pass
    else:
        if replace_file(os.fspath(destination), os.fspath(source), None, 0, None, None):
            return
        error = ctypes.get_last_error()
        if error not in {2, 3}:  # raced removal may safely retry as a new move
            raise ctypes.WinError(error)
    _move_file_windows(source, destination, replace=False)


def _rename_windows(source: Path, destination: Path) -> None:
    _move_file_windows(source, destination, replace=False)


def _move_file_windows(source: Path, destination: Path, *, replace: bool) -> None:
    import ctypes
    from ctypes import wintypes

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    move_file = kernel32.MoveFileExW
    move_file.argtypes = [wintypes.LPCWSTR, wintypes.LPCWSTR, wintypes.DWORD]
    move_file.restype = wintypes.BOOL
    flags = 0x00000008  # MOVEFILE_WRITE_THROUGH
    if replace:
        flags |= 0x00000001  # MOVEFILE_REPLACE_EXISTING
    if not move_file(os.fspath(source), os.fspath(destination), flags):
        raise ctypes.WinError(ctypes.get_last_error())


def _windows_handle_identity(kernel32: object, handle: object) -> tuple[int, int]:
    import ctypes
    from ctypes import wintypes

    class _ByHandleFileInformation(ctypes.Structure):
        _fields_ = [
            ("FileAttributes", wintypes.DWORD),
            ("CreationTime", wintypes.FILETIME),
            ("LastAccessTime", wintypes.FILETIME),
            ("LastWriteTime", wintypes.FILETIME),
            ("VolumeSerialNumber", wintypes.DWORD),
            ("FileSizeHigh", wintypes.DWORD),
            ("FileSizeLow", wintypes.DWORD),
            ("NumberOfLinks", wintypes.DWORD),
            ("FileIndexHigh", wintypes.DWORD),
            ("FileIndexLow", wintypes.DWORD),
        ]

    get_information = kernel32.GetFileInformationByHandle
    get_information.argtypes = [
        wintypes.HANDLE,
        ctypes.POINTER(_ByHandleFileInformation),
    ]
    get_information.restype = wintypes.BOOL
    info = _ByHandleFileInformation()
    if not get_information(handle, ctypes.byref(info)):
        raise ctypes.WinError(ctypes.get_last_error())
    file_index = (info.FileIndexHigh << 32) | info.FileIndexLow
    return info.VolumeSerialNumber, file_index


def _sync_directory_windows(path: Path) -> None:
    import ctypes
    from ctypes import wintypes

    generic_read = 0x80000000
    generic_write = 0x40000000
    share_all = 0x00000001 | 0x00000002 | 0x00000004
    open_existing = 3
    backup_semantics = 0x02000000
    open_reparse_point = 0x00200000
    invalid_handle = wintypes.HANDLE(-1).value

    class _FileAttributeTagInfo(ctypes.Structure):
        _fields_ = [
            ("FileAttributes", wintypes.DWORD),
            ("ReparseTag", wintypes.DWORD),
        ]

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    create_file = kernel32.CreateFileW
    create_file.argtypes = [
        wintypes.LPCWSTR,
        wintypes.DWORD,
        wintypes.DWORD,
        ctypes.c_void_p,
        wintypes.DWORD,
        wintypes.DWORD,
        wintypes.HANDLE,
    ]
    create_file.restype = wintypes.HANDLE
    get_file_information = kernel32.GetFileInformationByHandleEx
    get_file_information.argtypes = [
        wintypes.HANDLE,
        ctypes.c_int,
        ctypes.c_void_p,
        wintypes.DWORD,
    ]
    get_file_information.restype = wintypes.BOOL
    flush_file_buffers = kernel32.FlushFileBuffers
    flush_file_buffers.argtypes = [wintypes.HANDLE]
    flush_file_buffers.restype = wintypes.BOOL
    close_handle = kernel32.CloseHandle
    close_handle.argtypes = [wintypes.HANDLE]
    close_handle.restype = wintypes.BOOL
    handle = create_file(
        os.fspath(path),
        generic_read | generic_write,
        share_all,
        None,
        open_existing,
        backup_semantics | open_reparse_point,
        None,
    )
    if handle == invalid_handle:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        info = _FileAttributeTagInfo()
        if not get_file_information(handle, 9, ctypes.byref(info), ctypes.sizeof(info)):
            raise ctypes.WinError(ctypes.get_last_error())
        is_directory = bool(info.FileAttributes & 0x00000010)
        is_reparse = bool(info.FileAttributes & 0x00000400)
        if not is_directory or is_reparse:
            raise _PublicationFileSystemError(
                f"publication directory is unsafe to flush: {path}"
            )
        opened_identity = _windows_handle_identity(kernel32, handle)
        current_handle = create_file(
            os.fspath(path),
            generic_read,
            share_all,
            None,
            open_existing,
            backup_semantics | open_reparse_point,
            None,
        )
        if current_handle == invalid_handle:
            raise ctypes.WinError(ctypes.get_last_error())
        try:
            current_identity = _windows_handle_identity(kernel32, current_handle)
        finally:
            close_handle(current_handle)
        if opened_identity != current_identity:
            raise _PublicationFileSystemError(
                f"publication directory changed while opening: {path}"
            )
        if not flush_file_buffers(handle):
            raise ctypes.WinError(ctypes.get_last_error())
    finally:
        close_handle(handle)


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
        descriptor = self._open_regular(
            path, writable=True, create=True, exclusive=True
        )
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)
        self.sync_directory(path.parent)
        self.fault("after_create_lock")

    def lock(self, path: Path, *, shared: bool) -> _FileLock:
        descriptor = self._open_regular(path, writable=not shared)
        return _FileLock(descriptor, shared=shared)

    def read_json(self, path: Path) -> object:
        descriptor = self._open_regular(path)
        try:
            with os.fdopen(descriptor, "r", encoding="utf-8") as stream:
                return json.load(stream, object_pairs_hook=_strict_json_object)
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
        descriptor = self._open_regular(
            temporary, writable=True, create=True, truncate=True
        )
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
        self._require_same_volume(source, destination)
        if os.name == "nt":
            self._retry_windows(lambda: _replace_windows(source, destination))
        else:
            os.replace(source, destination)
        self._sync_move_parents(source, destination)
        self.fault(f"after_replace:{source.name}:{destination.name}")

    def rename(self, source: Path, destination: Path) -> None:
        if self.kind(destination) is not None:
            raise _PublicationFileSystemError(
                f"publication rename destination already exists: {destination}"
            )
        self.fault(f"before_rename:{source.name}:{destination.name}")
        self._require_same_volume(source, destination)
        if os.name == "nt":
            self._retry_windows(lambda: _rename_windows(source, destination))
        else:
            source.rename(destination)
        self._sync_move_parents(source, destination)
        self.fault(f"after_rename:{source.name}:{destination.name}")

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
                self.sync_file(directory_path / name)
            self.sync_directory(directory_path)

    def sync_file(self, path: Path) -> None:
        """Durably flush one regular file without following links/reparse points."""

        descriptor = self._open_regular(path, writable=True)
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)

    def sync_directory(self, path: Path) -> None:
        if os.name == "nt":
            _sync_directory_windows(path)
            return
        descriptor = os.open(
            path,
            os.O_RDONLY | getattr(os, "O_DIRECTORY", 0) | getattr(os, "O_NOFOLLOW", 0),
        )
        try:
            opened = os.fstat(descriptor)
            current = path.stat(follow_symlinks=False)
            if not stat.S_ISDIR(opened.st_mode) or not _same_file_identity(
                opened, current
            ):
                raise _PublicationFileSystemError(
                    f"publication directory changed while opening: {path}"
                )
            os.fsync(descriptor)
        finally:
            os.close(descriptor)

    def _open_regular(
        self,
        path: Path,
        *,
        writable: bool = False,
        create: bool = False,
        truncate: bool = False,
        exclusive: bool = False,
    ) -> int:
        if os.name == "nt":
            descriptor = _open_regular_windows(
                path,
                writable=writable,
                create=create,
                truncate=truncate,
                exclusive=exclusive,
            )
        else:
            flags = os.O_RDWR if writable else os.O_RDONLY
            flags |= getattr(os, "O_NOFOLLOW", 0)
            if create:
                flags |= os.O_CREAT
            if truncate:
                flags |= os.O_TRUNC
            if exclusive:
                flags |= os.O_EXCL
            descriptor = os.open(path, flags, 0o600)
        try:
            opened = os.fstat(descriptor)
            current = path.stat(follow_symlinks=False)
            if (
                not stat.S_ISREG(opened.st_mode)
                or stat.S_ISLNK(current.st_mode)
                or self._is_reparse_point(current)
                or not _same_file_identity(opened, current)
            ):
                raise _PublicationFileSystemError(
                    f"publication file changed while opening: {path}"
                )
        except BaseException:
            os.close(descriptor)
            raise
        return descriptor

    @staticmethod
    def _require_same_volume(source: Path, destination: Path) -> None:
        source_parent = source.parent.stat(follow_symlinks=False)
        destination_parent = destination.parent.stat(follow_symlinks=False)
        if source_parent.st_dev != destination_parent.st_dev:
            raise _PublicationFileSystemError(
                "publication rename must remain on one filesystem volume"
            )

    def _sync_move_parents(self, source: Path, destination: Path) -> None:
        self.sync_directory(source.parent)
        if destination.parent != source.parent:
            self.sync_directory(destination.parent)

    @staticmethod
    def _is_reparse_point(info: os.stat_result) -> bool:
        attributes = getattr(info, "st_file_attributes", 0)
        return bool(attributes & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0))

    @staticmethod
    def _retry_windows(operation: Callable[[], None]) -> None:
        attempts = _WINDOWS_RENAME_ATTEMPTS if os.name == "nt" else 1
        _retry_sharing_violations(operation, attempts=attempts, delay=time.sleep)

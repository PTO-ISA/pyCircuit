"""Contained sibling staging for atomic artifact publication."""

from __future__ import annotations

import os
import shutil
import tempfile
from pathlib import Path, PurePosixPath
from types import TracebackType
from typing import Iterable


def _relative_path(value: str) -> PurePosixPath:
    path = PurePosixPath(value)
    if (
        path.is_absolute()
        or not path.parts
        or any(part in ("", ".", "..") for part in path.parts)
    ):
        raise ValueError(f"artifact path is not a contained relative path: {value!r}")
    return path


class ArtifactStage:
    """Stage a closed file set beside its destination and publish transactionally."""

    __slots__ = ("destination", "expected", "path", "committed")

    def __init__(self, destination: Path, *, expected: Iterable[str]):
        self.destination = destination.resolve()
        self.expected = tuple(sorted({_relative_path(item) for item in expected}))
        self.path: Path | None = None
        self.committed = False

    def __enter__(self) -> "ArtifactStage":
        self.destination.parent.mkdir(parents=True, exist_ok=True)
        self.path = Path(
            tempfile.mkdtemp(prefix=".agentic-stage-", dir=self.destination.parent)
        )
        return self

    def __exit__(
        self,
        exception_type: type[BaseException] | None,
        exception: BaseException | None,
        traceback: TracebackType | None,
    ) -> None:
        if self.path is not None and self.path.exists():
            shutil.rmtree(self.path)

    def _open_path(self, relative: str) -> Path:
        if self.path is None:
            raise RuntimeError("artifact stage has not been entered")
        normalized = _relative_path(relative)
        target = self.path.joinpath(*normalized.parts)
        target.parent.mkdir(parents=True, exist_ok=True)
        return target

    def write_bytes(self, relative: str, data: bytes) -> None:
        if type(data) is not bytes:
            raise TypeError("staged artifact data must be bytes")
        self._open_path(relative).write_bytes(data)

    def write_text(self, relative: str, text: str) -> None:
        if type(text) is not str:
            raise TypeError("staged artifact text must be a string")
        self.write_bytes(relative, text.encode("utf-8"))

    def _verify(self) -> None:
        if self.path is None:
            raise RuntimeError("artifact stage has not been entered")
        found: list[PurePosixPath] = []
        for candidate in sorted(self.path.rglob("*")):
            if candidate.is_dir():
                continue
            if not candidate.is_file() or candidate.is_symlink():
                raise ValueError("artifact stages may contain regular files only")
            found.append(PurePosixPath(candidate.relative_to(self.path).as_posix()))
        if tuple(found) != self.expected:
            raise ValueError("staged artifact set does not match the declared file set")

    def verify(self) -> None:
        """Validate the staged closed file set without publishing it."""

        self._verify()

    def commit(self, *, allow_replace: Iterable[str] = ()) -> None:
        self._verify()
        assert self.path is not None
        replace = {_relative_path(item) for item in allow_replace}
        if not replace.issubset(set(self.expected)):
            raise ValueError("replacement permission names an undeclared artifact")
        for relative in self.expected:
            target = self.destination.joinpath(*relative.parts)
            if target.exists() and relative not in replace:
                raise FileExistsError(f"artifact already exists: {relative.as_posix()}")

        backup_root = self.path / ".backup"
        published: list[PurePosixPath] = []
        backed_up: list[PurePosixPath] = []
        try:
            self.destination.mkdir(parents=True, exist_ok=True)
            for relative in self.expected:
                source = self.path.joinpath(*relative.parts)
                target = self.destination.joinpath(*relative.parts)
                target.parent.mkdir(parents=True, exist_ok=True)
                if target.exists():
                    backup = backup_root.joinpath(*relative.parts)
                    backup.parent.mkdir(parents=True, exist_ok=True)
                    os.replace(target, backup)
                    backed_up.append(relative)
                os.replace(source, target)
                published.append(relative)
        except BaseException:
            for relative in reversed(published):
                target = self.destination.joinpath(*relative.parts)
                source = self.path.joinpath(*relative.parts)
                source.parent.mkdir(parents=True, exist_ok=True)
                if target.exists():
                    os.replace(target, source)
            for relative in reversed(backed_up):
                backup = backup_root.joinpath(*relative.parts)
                target = self.destination.joinpath(*relative.parts)
                target.parent.mkdir(parents=True, exist_ok=True)
                if backup.exists():
                    os.replace(backup, target)
            raise
        if backup_root.exists():
            shutil.rmtree(backup_root)
        self.committed = True

"""Prune only wholly empty directory trees pre-created by Ninja outputs."""

from __future__ import annotations

import errno
import os
import stat
import sys


def _walk_error(error: OSError) -> None:
    raise error


def _is_reparse(info: os.stat_result) -> bool:
    return bool(
        getattr(info, "st_file_attributes", 0)
        & getattr(stat, "FILE_ATTRIBUTE_REPARSE_POINT", 0)
    )


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: prepare_output.py DESTINATION", file=sys.stderr)
        return 2
    requested = sys.argv[1]
    destination = (
        requested if os.path.isabs(requested) else os.path.join(os.getcwd(), requested)
    )
    ancestors = []
    ancestor = destination
    while True:
        ancestors.append(ancestor)
        parent = os.path.dirname(ancestor)
        if parent == ancestor:
            break
        ancestor = parent
    for ancestor in ancestors:
        try:
            info = os.lstat(ancestor)
        except FileNotFoundError:
            continue
        if stat.S_ISLNK(info.st_mode) or _is_reparse(info):
            raise OSError(errno.ENOTDIR, "symlink in output path", ancestor)
    try:
        destination_info = os.lstat(destination)
    except FileNotFoundError:
        return 0
    if not stat.S_ISDIR(destination_info.st_mode):
        raise OSError(errno.ENOTDIR, "output is not a directory", destination)
    directories = []
    populated = False
    # Preflight every entry before removing anything, including empty siblings.
    for root, names, files in os.walk(
        destination, followlinks=False, onerror=_walk_error
    ):
        directories.append(root)
        for name in (*names, *files):
            path = os.path.join(root, name)
            info = os.lstat(path)
            mode = info.st_mode
            if (
                stat.S_ISLNK(mode)
                or _is_reparse(info)
                or not (stat.S_ISDIR(mode) or stat.S_ISREG(mode))
            ):
                raise OSError(errno.ENOTDIR, "unsafe entry in output tree", path)
            populated |= stat.S_ISREG(mode)
    if populated:
        return 0
    for directory in reversed(directories):
        try:
            os.rmdir(directory)
        except OSError as error:
            # A concurrent new file must be preserved, never recursively removed.
            if error.errno not in {errno.ENOENT, errno.ENOTEMPTY, errno.EEXIST}:
                raise
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

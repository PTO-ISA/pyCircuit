"""Remove only an empty directory pre-created for a Ninja file output."""

from __future__ import annotations

import errno
import os
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: prepare_unit_output.py DESTINATION", file=sys.stderr)
        return 2
    destination = Path(sys.argv[1]).absolute()
    for ancestor in (destination, *destination.parents):
        if ancestor.is_symlink():
            raise OSError(errno.ENOTDIR, "symlink in output path", str(ancestor))
    try:
        os.rmdir(destination)
    except OSError as error:
        if error.errno not in {errno.ENOENT, errno.ENOTEMPTY, errno.EEXIST}:
            raise
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

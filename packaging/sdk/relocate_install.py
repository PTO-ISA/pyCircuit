"""Relocate a CMake install with the shared SDK dependency implementation."""

import sys
from pathlib import Path

from create_platform_manifest import relocate_native_dependencies

if __name__ == "__main__":
    prefix = Path(sys.argv[1])
    relocate_native_dependencies(prefix, sys.argv[2], prefix / sys.argv[3])

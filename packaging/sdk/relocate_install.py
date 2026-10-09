"""Relocate a CMake install with the shared SDK dependency implementation."""

import sys
from pathlib import Path

from create_platform_manifest import relocate_native_dependencies

if __name__ == "__main__":
    relocate_native_dependencies(Path(sys.argv[1]), sys.argv[2])

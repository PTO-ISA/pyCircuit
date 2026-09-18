"""Command-line integration test support."""

from __future__ import annotations

import os
import sys
from pathlib import Path


def cli_test_pythonpath(repository: Path, environment: dict[str, str]) -> str:
    source = repository / "python/agentic-circuit/src"
    toolchain = environment.get("AC_GATE_TOOLCHAIN_ROOT")
    if toolchain is None:
        # The gate may build the Agentic Circuit sources into the integrated
        # toolchain build tree (AC_GATE_BUILD_ROOT) instead of the standalone
        # dev-llvm22 preset. run_agentic_circuit.sh locates the native module at
        # <AC_GATE_BUILD_ROOT>/compiler/acir/python in that case, and both the
        # package copy and _native live there.
        build_root = environment.get("AC_GATE_BUILD_ROOT")
        if build_root is None:
            native = repository / ".pycircuit_out/acir/dev-llvm22/python"
        else:
            native = Path(build_root) / "compiler/acir/python"
    else:
        native = (
            Path(toolchain)
            / "lib"
            / f"python{sys.version_info.major}.{sys.version_info.minor}"
            / "site-packages"
        )
    return os.pathsep.join((source.resolve().as_posix(), native.resolve().as_posix()))

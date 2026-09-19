"""The gate helper must describe the build layout the gate actually used."""

from __future__ import annotations

import os
import sys
import unittest
from pathlib import Path

from cli import cli_test_pythonpath


def entries(environment: dict[str, str]) -> list[str]:
    return cli_test_pythonpath(Path("/repo"), environment).split(os.pathsep)


class GatePythonpathTest(unittest.TestCase):
    def test_follows_the_integrated_build_root(self) -> None:
        """run_agentic_circuit.sh builds into the integrated tree when asked.

        The CLI lane unsets AC_GATE_TOOLCHAIN_ROOT, so a helper that only knows
        the dev-llvm22 preset points the tests at a directory that does not
        exist and every CLI invocation fails with an internal error.
        """

        resolved = entries({"AC_GATE_BUILD_ROOT": "/build"})
        self.assertEqual(resolved[0], Path("/repo/python/agentic-circuit/src").resolve().as_posix())
        self.assertEqual(resolved[1], Path("/build/compiler/acir/python").resolve().as_posix())

    def test_follows_the_installed_toolchain(self) -> None:
        resolved = entries({"AC_GATE_TOOLCHAIN_ROOT": "/sdk"})
        version = f"python{sys.version_info.major}.{sys.version_info.minor}"
        self.assertEqual(
            resolved[1], Path(f"/sdk/lib/{version}/site-packages").resolve().as_posix()
        )

    def test_falls_back_to_the_dev_preset(self) -> None:
        resolved = entries({})
        self.assertEqual(
            resolved[1],
            Path("/repo/.pycircuit_out/acir/dev-llvm22/python").resolve().as_posix(),
        )


if __name__ == "__main__":
    unittest.main()


class GateBuildRootTest(unittest.TestCase):
    def test_prefers_the_integrated_build_root(self) -> None:
        from cli import gate_build_root

        self.assertEqual(
            gate_build_root(Path("/repo"), {"AC_GATE_BUILD_ROOT": "/build"}),
            Path("/build"),
        )

    def test_falls_back_to_the_dev_preset(self) -> None:
        from cli import gate_build_root

        self.assertEqual(
            gate_build_root(Path("/repo"), {}),
            Path("/repo/.pycircuit_out/acir/dev-llvm22"),
        )

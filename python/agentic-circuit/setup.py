from __future__ import annotations

import shutil
from pathlib import Path

from setuptools import setup
from setuptools.command.build_py import build_py


class BuildPyWithSchemas(build_py):
    """Copy repository-owned schemas into the wheel without touching sources."""

    def run(self) -> None:
        super().run()
        repository = Path(__file__).resolve().parents[2]
        source = repository / "schemas/agentic-circuit"
        target = Path(self.build_lib) / "agentic_circuit/_data/schemas/agentic-circuit"
        if not source.is_dir():
            raise RuntimeError(f"Agentic Circuit schema root is unavailable: {source}")
        shutil.copytree(source, target, dirs_exist_ok=True)


setup(cmdclass={"build_py": BuildPyWithSchemas})

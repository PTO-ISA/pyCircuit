"""Pin the current Runtime and CompilerDev component boundary."""

from __future__ import annotations

import re
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]


def test_runtime_and_compilerdev_build_options_are_separate() -> None:
    source = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

    assert "option(PYC_BUILD_COMPILER_DEV" in source
    assert "option(PYC_BUILD_RUNTIME_LIB" in source
    assert "option(PYC_BUILD_TESTING" in source
    compiler_block = source.split("if(PYC_BUILD_COMPILER_DEV)", 1)[1].split(
        "endif()", 1
    )[0]
    assert "find_package(LLVM 22.1.8 EXACT CONFIG REQUIRED)" in compiler_block
    assert "find_package(MLIR 22.1.8 EXACT CONFIG REQUIRED)" in compiler_block
    assert "add_subdirectory(compiler)" in compiler_block
    assert "ACIRSourceCompiler" in compiler_block
    assert "PYC_BUILD_AGENTIC_CIRCUIT" not in source
    assert "PYC_BUILD_PYC" not in source


def test_runtime_only_configuration_does_not_find_compiler_dependencies() -> None:
    source = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    runtime_block = source.split("if(PYC_BUILD_RUNTIME_LIB)", 1)[1].split("endif()", 1)[
        0
    ]

    assert "add_subdirectory(runtime)" in runtime_block
    assert "find_package(LLVM" not in runtime_block
    assert "find_package(MLIR" not in runtime_block


def test_installation_guide_documents_supported_component_names() -> None:
    doc = (ROOT / "docs/getting-started/installation.md").read_text(encoding="utf-8")

    for contract in (
        "PYC_BUILD_COMPILER_DEV=OFF",
        "PYC_BUILD_RUNTIME_LIB=ON",
        "find_package(pycircuit CONFIG REQUIRED COMPONENTS Runtime)",
        "find_package(pycircuit CONFIG REQUIRED COMPONENTS CompilerDev)",
        "pycircuit compile",
        "pycircuit link",
        "pycircuit emit",
    ):
        assert contract in doc
    assert not re.search(r"(?:acc\.py|pycc|agentic-circuit)\s+is the current", doc)


def test_python_only_checkout_path_does_not_require_native_build_options() -> None:
    doc = (ROOT / "docs/getting-started/installation.md").read_text(encoding="utf-8")
    section = doc.split("## Python package", 1)[1]

    assert "python -m pip install -e ." in section
    assert "The public commands are `pycircuit compile`, `pycircuit link`," in section
    assert "pycircuit emit" in section

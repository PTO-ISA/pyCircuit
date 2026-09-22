"""Pin the documented compiler floor and the configure-time GCC guard.

The full toolchain needs libstdc++'s floating-point ``std::to_chars`` overload,
which only GCC 11 and newer provide. Two regressions are cheap to introduce and
invisible on the Clang/AppleClang hosts most development happens on:

* The requirements table quietly dropping the GCC floor again, so a Linux user
  hits a template error dozens of LLVM-linked targets into the build.
* The ``CMAKE_CXX_COMPILER_VERSION`` guard being removed or widened into a
  blanket check that rejects Clang and AppleClang, which do not use libstdc++.

ACIR is not optional either: the pyc dialect embeds ACIR attributes, so the
removed ``PYC_BUILD_AGENTIC_CIRCUIT=OFF`` configuration could never produce a
usable ``pycc``. Pin that the option is gone and the LLVM/MLIR discovery is
unconditional.
"""

from __future__ import annotations

import re
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]

INSTALLATION_DOC = ROOT / "docs/getting-started/installation.md"
TOP_LEVEL_CMAKE = ROOT / "CMakeLists.txt"


def test_installation_docs_name_the_minimum_gcc_version() -> None:
    doc = INSTALLATION_DOC.read_text(encoding="utf-8")

    assert re.search(r"\bGCC 11", doc), doc
    assert "std::to_chars" in doc
    # The floor belongs to the full toolchain column, not to the frontend-only
    # install that never builds C++.
    requirements = doc.split("## Requirements", 1)[1].split("## ", 1)[0]
    assert "GCC 11" in requirements


def test_top_level_cmake_rejects_old_gnu_compilers_at_configure_time() -> None:
    source = TOP_LEVEL_CMAKE.read_text(encoding="utf-8")

    assert 'CMAKE_CXX_COMPILER_ID STREQUAL "GNU"' in source
    assert "CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11" in source
    guard = source.split('CMAKE_CXX_COMPILER_ID STREQUAL "GNU"', 1)[1]
    assert "message(FATAL_ERROR" in guard
    # The guard has to name the requirement, not fail opaquely.
    assert "GCC 11 or newer" in guard

    # Fail before the build graph is populated, so a bad compiler never reaches
    # an LLVM-linked target.
    assert source.index('CMAKE_CXX_COMPILER_ID STREQUAL "GNU"') < source.index(
        "add_subdirectory("
    )


def test_gnu_guard_does_not_constrain_clang_or_appleclang() -> None:
    source = TOP_LEVEL_CMAKE.read_text(encoding="utf-8")

    guard = source.split("if(CMAKE_CXX_COMPILER_ID", 1)[1].split("endif()", 1)[0]
    assert "GNU" in guard
    assert not re.search(r"Clang|AppleClang|MSVC", guard)


def test_acir_build_and_llvm_discovery_are_unconditional() -> None:
    source = TOP_LEVEL_CMAKE.read_text(encoding="utf-8")

    assert not re.search(r"\bPYC_BUILD_AGENTIC_CIRCUIT\b", source)
    assert "PYC_BUILD_AGENTIC_CIRCUIT_TESTS" in source
    assert re.search(
        r"^find_package\(LLVM 22\.1\.8 EXACT CONFIG REQUIRED\)$",
        source,
        re.MULTILINE,
    )
    assert re.search(
        r"^find_package\(MLIR 22\.1\.8 EXACT CONFIG REQUIRED\)$",
        source,
        re.MULTILINE,
    )
    assert "add_subdirectory(compiler/acir)" in source


def test_build_wrappers_no_longer_forward_the_removed_option() -> None:
    for relative in (
        "flows/scripts/pyc",
        "flows/scripts/pyc.ps1",
        "flows/scripts/run_agentic_circuit.sh",
    ):
        text = (ROOT / relative).read_text(encoding="utf-8")
        assert not re.search(r"\bPYC_BUILD_AGENTIC_CIRCUIT\b", text), relative

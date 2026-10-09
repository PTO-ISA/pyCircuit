from __future__ import annotations

import ast
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
pytestmark = pytest.mark.unit

FLOW_TOOLS = {
    "check_api_hygiene.py",
    "check_generated_rtl.py",
    "check_frontend_retirement.py",
    "measure_source_build.py",
    "process_usage.py",
    "measure_build_resources.py",
    "materialize_source_preview.py",
    "summarize_gate_run.py",
}

PYCIRCUIT_TOOLS = {
    "example_catalog.py",
    "generate_source_identifier_unicode.py",
}


def _tracked_paths() -> list[str]:
    return subprocess.run(
        ("git", "ls-files"),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.splitlines()


def test_active_product_roots_are_documented() -> None:
    layout = (ROOT / "docs/development/repository-layout.md").read_text(
        encoding="utf-8"
    )
    for root in (
        "python/pycircuit/",
        "compiler/",
        "simulator/gfsim/",
        "examples/counter/",
        "tests/",
        "flows/",
    ):
        assert f"`{root}`" in layout


def test_flow_and_product_tools_have_distinct_roots() -> None:
    flow_tools = {
        path.name for path in (ROOT / "flows/tools").glob("*.py") if path.is_file()
    }
    pycircuit_tools = {
        path.name for path in (ROOT / "tools").glob("*.py") if path.is_file()
    }

    assert flow_tools == FLOW_TOOLS
    assert pycircuit_tools == PYCIRCUIT_TOOLS


def test_package_discovery_excludes_retired_siblings_and_namespace_debris(
    tmp_path: Path,
) -> None:
    tomllib = pytest.importorskip("tomllib")
    setuptools = pytest.importorskip("setuptools")
    config = tomllib.loads((ROOT / "pyproject.toml").read_text(encoding="utf-8"))
    discovery = config["tool"]["setuptools"]["packages"]["find"]
    source = tmp_path / discovery["where"][0]
    for name in ("pycircuit", "agentic_circuit", "semantic_core"):
        package = source / name
        package.mkdir(parents=True)
        (package / "__init__.py").write_text("", encoding="utf-8")
    debris = source / "pycircuit/src/stale"
    debris.mkdir(parents=True)
    (debris / "old.py").write_text("", encoding="utf-8")
    finder = (
        setuptools.find_namespace_packages
        if discovery.get("namespaces", True)
        else setuptools.find_packages
    )
    packages = finder(
        where=str(source),
        include=discovery.get("include", ["*"]),
        exclude=discovery.get("exclude", []),
    )
    assert packages == ["pycircuit"]


def test_ruff_per_file_ignores_match_existing_python_sources() -> None:
    lines = (ROOT / "pyproject.toml").read_text(encoding="utf-8").splitlines()
    section = "[tool.ruff.lint.per-file-ignores]"
    start = lines.index(section) + 1
    ignores: dict[str, list[str]] = {}
    for line in lines[start:]:
        stripped = line.strip()
        if stripped.startswith("["):
            break
        if not stripped or stripped.startswith("#"):
            continue
        key, separator, value = stripped.partition("=")
        assert separator, f"invalid Ruff per-file ignore entry: {line}"
        pattern = ast.literal_eval(key.strip())
        codes = ast.literal_eval(value.strip())
        assert isinstance(pattern, str)
        assert isinstance(codes, list) and codes
        ignores[pattern] = codes

    assert ignores, "Ruff per-file ignore section has no entries"
    for pattern in ignores:
        matches = [path for path in ROOT.glob(pattern) if path.is_file()]
        assert (
            matches
        ), f"Ruff per-file ignore pattern has no surviving source: {pattern}"
        assert all(path.suffix == ".py" for path in matches), pattern


def test_retired_documentation_is_not_part_of_the_product_tree() -> None:
    for relative in ("docs/acir", "docs/development/acir", "docs/pyc6-plan.md"):
        assert not (ROOT / relative).exists()
    assert (ROOT / "docs/development/known-limitations.md").is_file()


def test_wheel_exposes_only_the_py_circuit_distribution_and_driver() -> None:
    setup = (ROOT / "packaging/wheel/setup.py").read_text(encoding="utf-8")

    assert 'name="pycircuit-hisi"' in setup
    assert (
        'include=[\n            "pycircuit",\n            "pycircuit.*",\n        ]'
        in setup
    )
    assert '"pycircuit=pycircuit.cli:main"' in setup
    assert '"acc=' not in setup
    assert '"pycc=' not in setup
    assert '"agentic-circuit=' not in setup
    assert '"agentic_circuit"' not in setup


def test_host_sources_avoid_unprotected_int128() -> None:
    """MSVC has no __int128, so host sources must stay portable.

    Multi-word runtime arithmetic and the saturating statistics helpers used to
    rely on it, which broke the Windows SDK build with C4235. A guarded use that
    checks __SIZEOF_INT128__ first remains acceptable.
    """
    offenders: list[str] = []
    for directory in (
        "include/gfsim",
        "compiler",
        "simulator/gfsim",
        "tests/runtime",
        "tests/compiler",
    ):
        for path in sorted((ROOT / directory).rglob("*")):
            if path.suffix not in {".hpp", ".h", ".cpp", ".cc"} or not path.is_file():
                continue
            for number, line in enumerate(
                path.read_text(encoding="utf-8", errors="ignore").splitlines(),
                start=1,
            ):
                code = line.split("//", 1)[0]
                if "__int128" in code and "__SIZEOF_INT128__" not in code:
                    offenders.append(f"{path.relative_to(ROOT)}:{number}")
    assert offenders == []


def test_cmake_package_exports_only_the_approved_runtime_and_compiler_components() -> (
    None
):
    config = (ROOT / "cmake/pycircuitConfig.cmake.in").read_text(encoding="utf-8")
    runtime = (ROOT / "simulator/gfsim/CMakeLists.txt").read_text(encoding="utf-8")
    root_cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")

    assert "pycircuit::pyc6_runtime" in runtime
    assert "install(TARGETS ACIRDialect ACIRSourceCompiler" in root_cmake
    assert "EXPORT pycircuitCompilerTargets" in root_cmake
    assert 'component STREQUAL "CompilerDev"' in config
    assert 'component STREQUAL "Runtime"' in config
    assert "pycircuit::AgenticCircuit" not in config
    assert "pycircuit::PYC" not in config


POSIX_ONLY_MODULES = frozenset(
    {
        "crypt",
        "fcntl",
        "grp",
        "nis",
        "posix",
        "pty",
        "pwd",
        "resource",
        "spwd",
        "syslog",
        "termios",
        "tty",
    }
)

PRODUCT_PYTHON_ROOTS = ("python/pycircuit",)


def _platform_guarded(node: object, parents: dict[object, object]) -> bool:
    while node is not None:
        node = parents.get(node)
        if isinstance(node, ast.If):
            test = ast.dump(node.test)
            if "os" in test and "name" in test:
                return True
            if "sys" in test and "platform" in test:
                return True
    return False


def test_product_python_imports_posix_only_modules_conditionally() -> None:
    """The product Python has to import on Windows.

    A POSIX-only import is allowed only inside a platform check, which is how
    transactional publication selects ``msvcrt`` on Windows.
    """
    offenders: list[str] = []
    for root in PRODUCT_PYTHON_ROOTS:
        for path in sorted((ROOT / root).rglob("*.py")):
            tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
            parents: dict[object, object] = {}
            for parent in ast.walk(tree):
                for child in ast.iter_child_nodes(parent):
                    parents[child] = parent
            for node in ast.walk(tree):
                names: list[str] = []
                if isinstance(node, ast.Import):
                    names = [alias.name.split(".")[0] for alias in node.names]
                elif isinstance(node, ast.ImportFrom) and node.module:
                    names = [node.module.split(".")[0]]
                if any(name in POSIX_ONLY_MODULES for name in names):
                    if not _platform_guarded(node, parents):
                        offenders.append(f"{path.relative_to(ROOT)}:{node.lineno}")
    assert offenders == []


COMPILED_TOOL_NAMES = frozenset(
    {
        "pycircuit-source-unit",
        "pycircuit-link",
        "pycircuit-emit",
    }
)


def test_product_python_spells_compiled_tools_with_a_platform_suffix() -> None:
    """Private driver helpers are looked up through the platform-aware installer."""
    offenders: list[str] = []
    for root in PRODUCT_PYTHON_ROOTS:
        for path in sorted((ROOT / root).rglob("*.py")):
            tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
            parents: dict[object, object] = {}
            for parent in ast.walk(tree):
                for child in ast.iter_child_nodes(parent):
                    parents[child] = parent
            for node in ast.walk(tree):
                # A literal fragment of an f-string carries the suffix in a
                # sibling FormattedValue, so only whole literals count.
                if (
                    isinstance(node, ast.Constant)
                    and isinstance(node.value, str)
                    and not isinstance(parents.get(node), ast.JoinedStr)
                ):
                    head, _, tail = node.value.partition("/")
                    if head == "bin" and tail in COMPILED_TOOL_NAMES:
                        offenders.append(f"{path.relative_to(ROOT)}:{node.lineno}")
                if (
                    isinstance(node, ast.BinOp)
                    and isinstance(node.op, ast.Div)
                    and isinstance(node.right, ast.Constant)
                    and node.right.value in COMPILED_TOOL_NAMES
                ):
                    offenders.append(f"{path.relative_to(ROOT)}:{node.lineno}")
    assert offenders == []
    resolver = (ROOT / "python/pycircuit/packaged_toolchain.py").read_text(
        encoding="utf-8"
    )
    assert 'suffixes.insert(0, ".exe")' in resolver
    verifier = (ROOT / "python/pycircuit/_native_verify.py").read_text(encoding="utf-8")
    for helper in COMPILED_TOOL_NAMES:
        assert helper in verifier

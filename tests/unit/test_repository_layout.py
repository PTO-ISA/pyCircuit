from __future__ import annotations

import ast
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
pytestmark = pytest.mark.unit

TOP_LEVEL_ROOTS = {
    ".github",
    "benchmarks",
    "cmake",
    "compiler",
    "docs",
    "examples",
    "flows",
    "library",
    "packaging",
    "python",
    "schemas",
    "simulator",
    "tests",
    "third_party",
    "toolchains",
    "tools",
}

FLOW_TOOLS = {
    "build_cpp_manifest.py",
    "check_api_hygiene.py",
    "check_decision_status.py",
    "discover_examples.py",
    "gen_cmake_from_manifest.py",
    "summarize_gate_run.py",
}

PYCIRCUIT_TOOLS = {
    "check-pyc-inventory.py",
    "dump_pyctrace.py",
    "generate-semantic-primitive-registry.py",
    "pyc_module_graph.py",
    "schematic_view.py",
    "visualize_cpp.py",
}


def _tracked_paths() -> list[str]:
    return subprocess.run(
        ("git", "ls-files"),
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    ).stdout.splitlines()


def test_every_tracked_top_level_root_is_documented() -> None:
    roots = {path.split("/", 1)[0] for path in _tracked_paths() if "/" in path}
    assert roots == TOP_LEVEL_ROOTS

    layout = (ROOT / "docs/development/repository-layout.md").read_text(
        encoding="utf-8"
    )
    for root in sorted(TOP_LEVEL_ROOTS):
        assert f"`{root}/`" in layout


def test_flow_and_product_tools_have_distinct_roots() -> None:
    flow_tools = {
        path.name for path in (ROOT / "flows/tools").glob("*.py") if path.is_file()
    }
    pycircuit_tools = {
        path.name for path in (ROOT / "tools/pycircuit").glob("*.py") if path.is_file()
    }

    assert flow_tools == FLOW_TOOLS
    assert pycircuit_tools == PYCIRCUIT_TOOLS
    assert not list((ROOT / "tools").glob("*.py"))


def test_completed_migration_pages_are_not_active_documents() -> None:
    assert not (ROOT / "docs/acir/migration.md").exists()
    assert not (ROOT / "docs/acir/agentic-circuit-collaboration.md").exists()

    history = (ROOT / "docs/acir/spec/refs/history.md").read_text(encoding="utf-8")
    assert "PTO-ISA/agentic-circuit" in history
    assert "archived" in history


def test_current_generated_dispatch_abi_is_not_named_legacy() -> None:
    current_sources = (
        ROOT / "compiler/acir/lib/CodeGen/EmitCxx.cpp",
        ROOT / "simulator/gfsim/include/gfsim/dispatch.h",
        ROOT / "simulator/gfsim/include/gfsim/object.h",
        ROOT / "simulator/gfsim/system.cpp",
    )
    forbidden = (
        "LegacyDispatch",
        "LegacyActivation",
        "setLegacyDispatch",
        "runLegacy",
    )

    for path in current_sources:
        source = path.read_text(encoding="utf-8")
        for token in forbidden:
            assert token not in source, f"{path.relative_to(ROOT)}: {token}"


def test_acsim_device_kind_never_depends_on_queue_symbol_names() -> None:
    lowering = (
        ROOT / "compiler/acir/lib/Conversion/ACIRToACSim/ACIRToACSim.cpp"
    ).read_text(encoding="utf-8")

    for implicit_name in ('name == "pc"', 'name == "busy"', 'name == "rf"'):
        assert implicit_name not in lowering
    assert 'kind.getValue() == "register"' in lowering
    assert 'kind.getValue() == "regfile"' in lowering


def test_wheel_staging_tool_sources_exist() -> None:
    tree = ast.parse(
        (ROOT / "packaging/wheel/create_wheel.py").read_text(encoding="utf-8")
    )
    assignment = next(
        node
        for node in tree.body
        if isinstance(node, ast.Assign)
        and any(
            getattr(target, "id", None) == "WHEEL_TOOL_SOURCES"
            for target in node.targets
        )
    )
    relative_paths = [
        "/".join(
            node.value
            for node in sorted(
                (node for node in ast.walk(element) if isinstance(node, ast.Constant)),
                key=lambda node: (node.lineno, node.col_offset),
            )
        )
        for element in assignment.value.elts
    ]
    assert relative_paths == [
        "flows/tools/gen_cmake_from_manifest.py",
        "tools/pycircuit/pyc_module_graph.py",
    ]
    for relative in relative_paths:
        assert (ROOT / relative).is_file(), relative


def test_host_sources_avoid_unprotected_int128() -> None:
    """MSVC has no __int128, so host sources must stay portable.

    Multi-word runtime arithmetic and the saturating statistics helpers used to
    rely on it, which broke the Windows SDK build with C4235. A guarded use that
    checks __SIZEOF_INT128__ first remains acceptable.
    """
    offenders: list[str] = []
    for directory in (
        "library/cpp",
        "compiler/mlir",
        "compiler/acir",
        "simulator/gfsim",
        "tests/cpp",
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


def test_python_binding_links_the_windows_import_library_by_name() -> None:
    """The Windows extension must resolve CPython's auto-link directive.

    CPython's PC/pyconfig.h records a pragma-based auto-link directive for an
    import library: ``python3.lib`` under Py_LIMITED_API, otherwise
    ``python<major><minor>.lib``. Up to Python 3.13 that directive is
    unconditional and Python3::Module deliberately links no library, so the
    bindings have to supply the directory and the versioned import library
    themselves. Without it the Windows link fails with
    ``could not open 'python3.lib'``.
    """
    cmake = (ROOT / "compiler/acir/bindings/python/CMakeLists.txt").read_text(
        encoding="utf-8"
    )

    assert "if(WIN32)" in cmake
    assert "python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.lib" in cmake
    assert (
        'target_link_directories(agentic_circuit_native PRIVATE "${_acir_python_libs}")'
        in cmake
    )
    # The limited-API name is staged when the interpreter does not ship it,
    # because PC/pyconfig.h asks for python3.lib unconditionally.
    assert 'configure_file("${_acir_python_import_lib}"' in cmake
    assert '"${_acir_python_abi_dir}/python3.lib" COPYONLY)' in cmake
    # A missing import library must fail at configure time, not at link time.
    assert "Python import library for ${Python3_VERSION} is missing" in cmake


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

PRODUCT_PYTHON_ROOTS = (
    "python/pycircuit/src",
    "python/agentic-circuit/src",
    "python/semantic-core/src",
)


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

    ``fcntl`` is POSIX-only and used to be a bare top-level import in the
    model-plan command, which made the relocated Windows CLI fail its smoke
    test with ModuleNotFoundError. A POSIX-only import is allowed only inside
    a platform check, which is how the publish lock selects ``msvcrt`` on
    Windows.
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
        "acir-build",
        "acir-cxxgen",
        "acir-opcode-catalog",
        "acir-opt",
        "acir-queue-cxxgen",
        "acir-queue-plan",
        "acir-queue-pycgen",
        "pyc-opt",
        "pycc",
    }
)


def test_product_python_spells_compiled_tools_with_a_platform_suffix() -> None:
    """Compiled tools are ``<name>.exe`` on Windows, so a bare name is a bug.

    The model-plan command looked up ``bin/acir-queue-plan`` in the SDK manifest
    and the native tool resolver searched ``<root>/bin/acir-opt``; on Windows
    the manifest and the install tree spell both ``.exe``, so the lookup failed
    with ACSDK-PLAN-MANIFEST-002. The packaged-toolchain resolver already tries
    the suffix, and every other site must do the same.
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

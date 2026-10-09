#!/usr/bin/env python3
"""Check production source/install routes for retired compiler engines."""

from __future__ import annotations

import argparse
import ast
import json
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PRODUCTION_ROOTS = (
    ROOT / "python/pycircuit",
    ROOT / "compiler",
    ROOT / "CMakeLists.txt",
    ROOT / "CMakePresets.json",
    ROOT / "cmake",
    ROOT / "packaging/wheel",
    ROOT / "packaging/sdk/examples",
    ROOT / "benchmarks",
    ROOT / "tools",
    ROOT / ".github/workflows",
    ROOT / "pyproject.toml",
    ROOT / "flows/scripts/pyc",
    ROOT / "flows/scripts/pyc.ps1",
    ROOT / "flows/scripts/lib.sh",
    ROOT / "flows/scripts/install_llvm_and_build.sh",
)
RETIRED_TEXT = (
    (re.compile(r"AgenticCircuit::"), "retired CMake target alias"),
    (
        re.compile(r"^\s*(?:from|import)\s+_?pycircuit_semantics\b"),
        "semantic-core import",
    ),
    (
        re.compile(r"^\s*(?:from|import)\s+agentic_circuit\b"),
        "retired Python namespace import",
    ),
    (
        re.compile(
            r"(?:add_(?:executable|library|subdirectory|custom_target)|install\s*\()[^\n]*(?:semantic-core|agentic-circuit|agentic_circuit|_pycircuit_semantics)"
        ),
        "retired CMake route",
    ),
    (
        re.compile(
            r"(?:python/(?:semantic-core|agentic-circuit)/src|agentic_circuit_native|_pycircuit_semantics)"
        ),
        "retired build or package route",
    ),
    (
        re.compile(
            r"(?:add_(?:executable|custom_target)|install\s*\()[^\n]*\b(?:pycc|pyc-opt|acc\.py)\b"
        ),
        "retired compiler target",
    ),
    (
        re.compile(
            r"(?:QueueGraphGenerator|QueueGraphPlan|QueueGraphPyc|QueueGraphGenerator\.cpp)"
        ),
        "retired QueueGraph engine",
    ),
)
RETIRED_INSTALL_NAMES = {
    "acir-opt",
    "acir-opt.exe",
    "emitted-cost.schema.json",
    "emitted-cost.example.json",
    "pycc",
    "pycc.exe",
    "pyc-opt",
    "pyc-opt.exe",
    "acc",
    "acc.exe",
    "acc.py",
    "agentic-circuit",
    "agentic-circuit.exe",
}
RETIRED_INSTALL_PARTS = {"agentic_circuit", "_pycircuit_semantics"}
REQUIRED_TOOLS = {
    "pycircuit",
    "pycircuit-source-unit",
    "pycircuit-link",
    "pycircuit-emit",
}


def scan_cmake_presets(path: Path) -> list[str]:
    """Check structured references that the CMake line scan cannot see."""
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, ValueError) as error:
        return [f"cannot read CMake presets: {error}"]
    if not isinstance(document, dict):
        return ["CMake presets must be a JSON object"]
    retired_options = {"PYC_BUILD_MLIR_TOOLS", "PYC_BUILD_AGENTIC_CIRCUIT_TESTS"}
    failures: list[str] = []
    for section in ("configurePresets", "buildPresets"):
        presets = document.get(section, [])
        if not isinstance(presets, list):
            failures.append(f"invalid CMake preset section: {section}")
            continue
        for preset in presets:
            if not isinstance(preset, dict):
                failures.append(f"invalid CMake preset in {section}")
                continue
            name = preset.get("name", "<unnamed>")
            if section == "configurePresets":
                variables = preset.get("cacheVariables", {})
                if not isinstance(variables, dict):
                    failures.append(f"invalid cacheVariables in CMake preset {name}")
                    continue
                for option in sorted(retired_options.intersection(variables)):
                    failures.append(f"CMake preset {name}: retired option {option}")
            else:
                targets = preset.get("targets", [])
                if isinstance(targets, str):
                    targets = [targets]
                if not isinstance(targets, list) or not all(
                    isinstance(target, str) for target in targets
                ):
                    failures.append(f"invalid targets in CMake preset {name}")
                    continue
                for target in targets:
                    if target in RETIRED_INSTALL_NAMES:
                        failures.append(f"CMake preset {name}: retired target {target}")
    return failures


def production_files() -> list[Path]:
    files: list[Path] = []
    for item in PRODUCTION_ROOTS:
        if item.is_file():
            files.append(item)
        elif item.is_dir():
            files.extend(
                path
                for path in item.rglob("*")
                if path.is_file()
                and not any(
                    part.lower() in {"test", "tests", "unittests"}
                    for part in path.parts
                )
            )
    return sorted(set(files))


def scan_production() -> list[str]:
    failures = scan_cmake_presets(ROOT / "CMakePresets.json")
    for path in production_files():
        try:
            content = path.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        if path.parent == ROOT / "packaging/sdk/examples" and path.suffix == ".json":
            value = json.loads(content)
            for record in value.get("files", []):
                relative = record.get("path", "")
                if Path(relative).name in RETIRED_INSTALL_NAMES | {
                    "libgfsim.a",
                    "AgenticCircuitConfig.cmake",
                    "acir-opt",
                }:
                    failures.append(f"retired SDK example asset: {relative}")
        for line_number, line in enumerate(content.splitlines(), 1):
            # Exact denial-list declarations are data, not imports/dispatch.
            # Do not exempt the rest of these files from route checks.
            if line.strip() in {
                'RETIRED_PYTHON_TREES = ("_pycircuit_semantics", "agentic_circuit")',
                'for name in ("agentic_circuit/", "_pycircuit_semantics/"):',
            }:
                continue

            if "compiler/CMakeLists.txt" in str(path) and "/tests/" in line:
                continue
            if path.suffix in {".sh", ".ps1"} and re.search(
                r"pyc_find_pycc|(?:^|[ /])(?:pycc|acc\.py)(?: |$)", line
            ):
                failures.append(
                    f"{path.relative_to(ROOT)}:{line_number}: retired shell driver"
                )
            if path.parent == ROOT / ".github/workflows":
                for script in re.findall(
                    r"(?:tools|flows|packaging)/[A-Za-z0-9_./-]+\.(?:py|sh)", line
                ):
                    if not (ROOT / script).is_file():
                        failures.append(f"workflow invokes missing script: {script}")
            # The preserved C ABI symbol is part of the approved C3 surface.
            if "agentic_model_query_v1" in line:
                line = line.replace("agentic_model_query_v1", "")
            for pattern, description in RETIRED_TEXT:
                if pattern.search(line):
                    failures.append(
                        f"{path.relative_to(ROOT)}:{line_number}: {description}"
                    )
    return failures


def scan_public_surface() -> list[str]:
    failures: list[str] = []
    package_init = ROOT / "python/pycircuit/__init__.py"
    cli = ROOT / "python/pycircuit/cli.py"
    if package_init.is_file():
        tree = ast.parse(
            package_init.read_text(encoding="utf-8"), filename=str(package_init)
        )
        exported = None
        for statement in tree.body:
            if isinstance(statement, ast.Assign) and any(
                isinstance(target, ast.Name) and target.id == "__all__"
                for target in statement.targets
            ):
                try:
                    exported = set(ast.literal_eval(statement.value))
                except (ValueError, TypeError):
                    exported = set()
        expected = {"module", "rule", "system", "log", "report"}
        if exported != expected:
            failures.append(
                f"public Python exports differ from the approved source surface: {sorted(exported or ())}"
            )
    if cli.is_file():
        tree = ast.parse(cli.read_text(encoding="utf-8"), filename=str(cli))
        commands = {
            node.args[0].value
            for node in ast.walk(tree)
            if isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute)
            and node.func.attr == "add_parser"
            and node.args
            and isinstance(node.args[0], ast.Constant)
            and isinstance(node.args[0].value, str)
        }
        expected_commands = {"compile", "link", "emit", "run"}
        if commands != expected_commands:
            failures.append(
                f"CLI command set must be compile/link/emit/run, found {sorted(commands)}"
            )
    for runner in (ROOT / "tests").rglob("lit*.py"):
        text = runner.read_text(encoding="utf-8")
        if any(
            token in text
            for token in ("%acc", "%pycc", "%pyc_opt", "acir-queue", "QueueGraph")
        ):
            failures.append(f"retired lit runner remains: {runner.relative_to(ROOT)}")
    return failures


def scan_install(root: Path) -> list[str]:
    failures: list[str] = []
    if not root.is_dir():
        return [f"install root does not exist: {root}"]
    files = [path for path in root.rglob("*") if path.is_file()]
    for path in files:
        relative = path.relative_to(root)
        if path.name in RETIRED_INSTALL_NAMES:
            failures.append(f"retired executable installed: {relative}")
        if any(part in RETIRED_INSTALL_PARTS for part in relative.parts):
            failures.append(f"retired Python package installed: {relative}")
    bindir = root / "bin"
    installed_tools = {path.name.removesuffix(".exe") for path in bindir.glob("*")}
    for name in sorted(REQUIRED_TOOLS - installed_tools):
        failures.append(f"required source compiler tool is missing from bin/: {name}")
    package_configs = list(root.rglob("pycircuitConfig.cmake"))
    if not package_configs:
        failures.append("installed CMake package config is missing")
    for config in package_configs:
        text = config.read_text(encoding="utf-8", errors="replace")
        if "PYCIRCUIT_PYCC_EXECUTABLE" in text or re.search(r"\bpycc\b", text):
            failures.append(
                f"installed CMake config retains retired compiler route: {config.relative_to(root)}"
            )
    if not any(
        path.name.startswith("libpyc6_runtime.") or path.name == "pyc6_runtime.lib"
        for path in files
    ):
        failures.append("installed pyc6_runtime library is missing")
    return failures


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--install-root",
        type=Path,
        default=None,
        help="also scan an installed toolchain (defaults to PYC_TOOLCHAIN_ROOT when set)",
    )
    args = parser.parse_args(argv)
    failures = scan_production() + scan_public_surface()
    install_root = args.install_root or (
        Path(os.environ["PYC_TOOLCHAIN_ROOT"])
        if os.environ.get("PYC_TOOLCHAIN_ROOT")
        else None
    )
    if install_root is not None:
        failures.extend(scan_install(install_root.resolve()))
    if failures:
        print("source compiler retirement check failed:", file=sys.stderr)
        for failure in failures:
            print(f"  {failure}", file=sys.stderr)
        return 1
    print("source compiler retirement check passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

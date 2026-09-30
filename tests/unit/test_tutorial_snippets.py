"""The quickstart source and command examples stay executable under M5."""

from __future__ import annotations

import ast
import os
import re
import subprocess
import sys
import textwrap
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
QUICKSTART = ROOT / "docs/getting-started/quickstart.md"


def _quickstart() -> str:
    return QUICKSTART.read_text(encoding="utf-8")


def _python_source() -> str:
    blocks = re.findall(r"```python\n(.*?)```", _quickstart(), flags=re.DOTALL)
    assert len(blocks) == 1, "quickstart should contain one complete Python source"
    ast.parse(blocks[0])
    return blocks[0]


@pytest.mark.unit
def test_quickstart_source_uses_explicit_m5_rule_registration() -> None:
    source = _python_source()
    tree = ast.parse(source)
    imports = {
        alias.name
        for node in ast.walk(tree)
        if isinstance(node, ast.ImportFrom) and node.module == "pycircuit"
        for alias in node.names
    }

    assert imports == {"module", "rule"}
    assert "@module" in source and "@rule" in source
    assert "tick()" in source
    assert "nonlocal count" in source
    for retired_api in ("build_cycle_aware", "CycleAwareSignal", "agentic_circuit"):
        assert retired_api not in source


@pytest.mark.unit
def test_quickstart_commands_compile_link_and_emit_the_same_final() -> None:
    text = _quickstart()
    commands = text.split("## Compile, link, and emit", 1)[1].split("## Continue", 1)[0]

    assert "pycircuit compile -c src/counter.py" in commands
    assert "pycircuit link .pycircuit_out/units/counter" in commands
    assert "--top demo.counter.Counter" in commands
    assert commands.count("pycircuit emit .pycircuit_out/design_top.ac") == 2
    assert "--target cpp" in commands
    assert "--target verilog" in commands


@pytest.mark.system
def test_quickstart_python_source_compiles_links_and_emits_both_targets(
    tmp_path: Path,
) -> None:
    native = Path(
        os.environ.get("PYCIRCUIT_NATIVE_BUILD", ROOT / ".pycircuit_out/m5-root")
    )
    env = os.environ.copy()
    env.update(
        {
            "PYCIRCUIT_NATIVE_BUILD": str(native),
            "ACIR_SOURCE_UNIT_HARNESS": str(native / "bin/acir-source-unit-harness"),
            "ACIR_DESIGN_HARNESS": str(native / "bin/acir-design-harness"),
            "ACIR_CPP_SOURCE_PARTS_HARNESS": str(
                native / "bin/acir-cpp-source-parts-harness"
            ),
        }
    )
    package_root = str(ROOT / "python/pycircuit/src")
    env["PYTHONPATH"] = os.pathsep.join(
        item for item in (package_root, env.get("PYTHONPATH", "")) if item
    )
    source = tmp_path / "src/counter.py"
    source.parent.mkdir(parents=True)
    source.write_text(textwrap.dedent(_python_source()), encoding="utf-8")
    unit = tmp_path / "units/counter"
    unit.parent.mkdir(parents=True)
    final = tmp_path / "design_top.ac"

    def run(*args: str) -> None:
        command = [sys.executable, "-m", "pycircuit.cli", *args]
        result = subprocess.run(
            command,
            cwd=ROOT,
            env=env,
            text=True,
            capture_output=True,
            check=False,
            timeout=900,
        )
        assert result.returncode == 0, f"{command!r}\n{result.stderr}"

    run(
        "compile",
        "-c",
        str(source),
        "--source-root",
        str(source.parent),
        "--package-prefix",
        "demo",
        "-o",
        str(unit),
    )
    run("link", str(unit), "--top", "demo.counter.Counter", "-o", str(final))
    for target in ("cpp", "verilog"):
        output = tmp_path / target
        run("emit", str(final), "--target", target, "-o", str(output))
        assert (output / "generated.json").is_file()

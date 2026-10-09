"""The quickstart source and command examples stay executable under source compiler."""

from __future__ import annotations

import ast
import os
import re
import shlex
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
def test_quickstart_source_matches_the_runnable_example() -> None:
    documented = ast.parse(_python_source())
    checked_in = ast.parse(
        (ROOT / "examples/hello_counter/hello_counter.py").read_text(encoding="utf-8")
    )
    assert ast.dump(documented) == ast.dump(checked_in)
    assert not any(isinstance(node, ast.Nonlocal) for node in ast.walk(documented))


@pytest.mark.unit
def test_quickstart_commands_compile_link_and_emit_the_same_final() -> None:
    text = _quickstart()
    blocks = re.findall(r"```(?:bash|sh)\n(.*?)```", text, re.DOTALL)
    commands = [
        shlex.split(line)
        for block in blocks
        for line in block.replace("\\\n", " ").splitlines()
        if line.strip().startswith("pycircuit ")
    ]
    compile_commands = [command for command in commands if command[1] == "compile"]
    link_commands = [command for command in commands if command[1] == "link"]
    emit_commands = [command for command in commands if command[1] == "emit"]
    assert len(compile_commands) == len(link_commands) == 1
    assert len(emit_commands) == 2
    source = compile_commands[0][compile_commands[0].index("-c") + 1]
    assert source == "hello_counter.py"
    unit = compile_commands[0][compile_commands[0].index("-o") + 1]
    assert link_commands[0][2] == unit
    package = compile_commands[0][compile_commands[0].index("--package-prefix") + 1]
    tree = ast.parse(_python_source())
    modules = {
        node.name
        for node in tree.body
        if isinstance(node, ast.FunctionDef)
        and any(
            (isinstance(item, ast.Name) and item.id in {"module", "system"})
            or (isinstance(item, ast.Attribute) and item.attr in {"module", "system"})
            for item in node.decorator_list
        )
    }
    top = link_commands[0][link_commands[0].index("--top") + 1]
    assert top in {f"{package}.{Path(source).stem}.{name}" for name in modules}
    final = link_commands[0][link_commands[0].index("-o") + 1]
    assert all(command[2] == final for command in emit_commands)
    assert {command[command.index("--target") + 1] for command in emit_commands} == {
        "cpp",
        "verilog",
    }


@pytest.mark.system
def test_quickstart_python_source_compiles_links_and_emits_both_targets(
    tmp_path: Path,
) -> None:
    native = Path(
        os.environ.get("PYCIRCUIT_NATIVE_BUILD", ROOT / ".pycircuit_out/source-root")
    )
    env = os.environ.copy()
    env.update(
        {
            "PYCIRCUIT_NATIVE_BUILD": str(native),
            "PYCIRCUIT_SOURCE_COMPILER": str(native / "bin/pycircuit-source-unit"),
            "PYCIRCUIT_LINKER": str(native / "bin/pycircuit-link"),
            "PYCIRCUIT_EMITTER": str(native / "bin/pycircuit-emit"),
        }
    )
    package_root = str(ROOT / "python/pycircuit/src")
    env["PYTHONPATH"] = os.pathsep.join(
        item for item in (package_root, env.get("PYTHONPATH", "")) if item
    )
    source = tmp_path / "src/hello_counter.py"
    source.parent.mkdir(parents=True)
    source.write_text(textwrap.dedent(_python_source()), encoding="utf-8")
    unit = tmp_path / "units/hello_counter"
    unit.parent.mkdir(parents=True)
    final = tmp_path / "hello_counter.ac"

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
    run("link", str(unit), "--top", "demo.hello_counter.HelloCounter", "-o", str(final))
    before = final.read_bytes()
    for target in ("cpp", "verilog"):
        output = tmp_path / target
        run("emit", str(final), "--target", target, "-o", str(output))
        assert (output / "generated.json").is_file()
        assert final.read_bytes() == before

"""The getting-started tutorial must elaborate exactly as written.

Issue #228 recorded two defects in the pipeline section, and the same class of
drift appeared in the very first example: the tutorial taught caller-supplied
static geometry (``build_cycle_aware(build, width=8)``) and
``domain.call(..., pc_width=...)`` after Decision 0267 removed caller-inferred
specialization, so a copied snippet raised
``PYC-PY-TYPE: static build arguments require an explicit source-owned
finite-family declaration``. The two-stage MAC snippet had a second, silent
defect: it built the bypass sum after the second ``domain.next()``, which added a
second balancing register on ``c`` and made the emitted hardware compute
``a(t)*b(t) + c(t-1)``.

These tests execute the tutorial's own snippets and pin the register counts the
prose promises, so the document cannot drift away from the frontend again.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

import pytest

pytestmark = pytest.mark.unit

ROOT = Path(__file__).resolve().parents[2]
TUTORIAL = ROOT / "docs/getting-started/tutorial.md"

PREAMBLE = """
from pycircuit import (
    CycleAwareCircuit, CycleAwareDomain,
    build_cycle_aware, cas, mux, submodule_input, u, wire_of,
)
"""


def _read_tutorial() -> str:
    return TUTORIAL.read_text(encoding="utf-8")


def _section(title: str) -> str:
    """Return the body of the ``## title`` section."""
    text = _read_tutorial()
    match = re.search(
        rf"^## {re.escape(title)}$(.*?)(?=^## |\Z)",
        text,
        flags=re.MULTILINE | re.DOTALL,
    )
    assert match is not None, f"tutorial section {title!r} is missing"
    return match.group(1)


def _python_blocks(body: str) -> list[str]:
    return re.findall(r"```python\n(.*?)```", body, flags=re.DOTALL)


def _block_defining(body: str, name: str) -> str:
    for block in _python_blocks(body):
        if re.search(rf"^def {re.escape(name)}\(", block, flags=re.MULTILINE):
            return block
    raise AssertionError(f"no tutorial python block defines {name!r}")


def _execute_snippet(block: str) -> dict[str, Any]:
    namespace: dict[str, Any] = {"__name__": "tutorial_snippet"}
    exec(compile(PREAMBLE + block, "<tutorial-snippet>", "exec"), namespace)
    return namespace


def _kwargs(argument_text: str) -> set[str]:
    """Keyword names passed at the top level of an argument list."""
    names: set[str] = set()
    depth = 0
    for match in re.finditer(
        r"[()\[\]{}]|(?<![=!<>])\b([A-Za-z_][A-Za-z0-9_]*)\s*=(?!=)", argument_text
    ):
        token = match.group(0)
        if token in "([{":
            depth += 1
        elif token in ")]}":
            depth -= 1
        elif depth == 0:
            names.add(match.group(1))
    return names


def _calls(text: str, pattern: str) -> list[str]:
    """Argument text of every call matched by *pattern*, with balanced parens."""
    found: list[str] = []
    for match in re.finditer(pattern, text):
        start = match.end()
        depth = 1
        index = start
        while index < len(text) and depth:
            if text[index] == "(":
                depth += 1
            elif text[index] == ")":
                depth -= 1
            index += 1
        found.append(text[start : index - 1])
    return found


def test_tutorial_never_passes_caller_supplied_static_geometry() -> None:
    """Decision 0267 removed caller-inferred specialization.

    A tutorial snippet that passes ``width=``/``pc_width=``/``data_width=`` to a
    compile entrypoint or to ``domain.call`` sends readers straight into a
    fail-closed diagnostic.
    """

    text = _read_tutorial()

    for pattern in (
        r"(?<![\w.])(?:build|compile)_cycle_aware\(",
        r"\.call\(",
    ):
        allowed = (
            {"inputs", "prefix"}
            if pattern == r"\.call\("
            else {"name", "domain_name", "hierarchical"}
        )
        for arguments in _calls(text, pattern):
            unexpected = _kwargs(arguments) - allowed
            assert not unexpected, (
                f"{pattern} in the tutorial passes {sorted(unexpected)}; "
                "static geometry belongs in the module source and only "
                f"{sorted(allowed)} may be passed"
            )


def test_tutorial_counter_snippet_elaborates() -> None:
    body = _section("第一个设计：计数器")
    namespace = _execute_snippet(_block_defining(body, "build"))
    emitted = namespace["build_cycle_aware"](
        namespace["build"], name="counter"
    ).emit_mlir()
    assert "pyc.reg" in emitted


def test_tutorial_mac2_snippet_balances_the_bypass_once() -> None:
    """The prose promises one balancing register on ``c``, not two.

    Building ``prod + c`` after the second ``domain.next()`` moves the expression
    into cycle 2 and delays ``c`` twice, so the bypass operand is one cycle stale
    and ``out`` is no longer ``(a * b) + c``.
    """

    body = _section("流水线：让编译器插入寄存器")
    namespace = _execute_snippet(_block_defining(body, "mac2"))
    emitted = namespace["build_cycle_aware"](namespace["mac2"], name="mac2").emit_mlir()

    assert emitted.count("pyc.reg") == 3, emitted
    assert "_v6_bal_1" in emitted
    assert "_v6_bal_2" not in emitted, (
        "the bypass operand gained a second balancing register; build the sum in "
        "the cycle where `prod` lives, before domain.next()"
    )


def test_tutorial_mac2_snippet_adds_prod_to_the_first_balancing_stage() -> None:
    body = _section("流水线：让编译器插入寄存器")
    namespace = _execute_snippet(_block_defining(body, "mac2"))
    emitted = namespace["build_cycle_aware"](namespace["mac2"], name="mac2").emit_mlir()

    named_alias = {
        name: operand
        for operand, name in re.findall(
            r"%(\w+) = pyc\.alias %\w+ \{pyc\.name = \"([^\"]+)\"\}", emitted
        )
    }

    # `c` is staged exactly once, and the adder consumes `prod` plus that one
    # staged copy of `c` -- not a second, deeper one.
    assert set(named_alias) >= {"prod", "acc", "_v6_bal_1"}, emitted
    add = re.search(r"pyc\.add %(\w+), %(\w+)", emitted)
    assert add is not None, emitted
    assert set(add.groups()) == {
        named_alias["prod"],
        named_alias["_v6_bal_1"],
    }, emitted

    staged_wire = re.search(r"pyc\.assign %(\w+), %c\b", emitted)
    assert staged_wire is not None, emitted
    staged_register = re.search(
        rf"%(\w+) = pyc\.reg %clk, %rst, %\w+, %{staged_wire.group(1)}, ", emitted
    )
    assert staged_register is not None, emitted
    assert re.search(
        rf"%{named_alias['_v6_bal_1']} = pyc\.alias %{staged_register.group(1)} ",
        emitted,
    ), emitted


def test_tutorial_mini_alu_snippet_is_purely_combinational() -> None:
    body = _section("组合逻辑与多路选择")
    namespace = _execute_snippet(_block_defining(body, "mini_alu"))
    emitted = namespace["build_cycle_aware"](
        namespace["mini_alu"], name="mini_alu"
    ).emit_mlir()
    assert "pyc.reg" not in emitted
    assert "pyc.add" in emitted


def test_tutorial_accumulator_template_supports_both_modes() -> None:
    """The dual-mode template must compose, not only elaborate standalone."""

    body = _section("标准模块模板与双模运行")
    namespace = _execute_snippet(_block_defining(body, "accumulator"))
    accumulator = namespace["accumulator"]
    build_cycle_aware = namespace["build_cycle_aware"]

    standalone = build_cycle_aware(accumulator, name="accumulator").emit_mlir()
    assert "acc_sum" in standalone
    assert "acc_sum_next" in standalone

    def parent(m, domain):
        data_in = namespace["cas"](domain, m.input("data_in", width=32), cycle=0)
        valid = namespace["cas"](domain, m.input("valid", width=1), cycle=0)
        bound = domain.call(
            accumulator,
            inputs={"data_in": data_in, "valid": valid},
            prefix="u_acc",
        )
        m.output("out", namespace["wire_of"](bound["sum"]))

    composed = build_cycle_aware(parent, name="parent", hierarchical=True).emit_mlir()
    assert "pyc.instance" in composed


def test_tutorial_cpu_core_snippet_composes_hierarchically() -> None:
    body = _section("层次化组合：搭一个小 CPU")
    namespace = _execute_snippet(_block_defining(body, "cpu_core"))
    cpu_core = namespace["cpu_core"]
    build_cycle_aware = namespace["build_cycle_aware"]

    flat = build_cycle_aware(cpu_core, name="cpu_core").emit_mlir()
    assert "pyc.instance" not in flat

    hierarchical = build_cycle_aware(
        cpu_core, name="cpu_core", hierarchical=True
    ).emit_mlir()
    assert hierarchical.count("pyc.instance") == 2

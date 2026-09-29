"""Independent R1/M1 lexical source contract tests (V02, V03, V05)."""

from __future__ import annotations

import os
import re
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path

import pytest
from pycircuit._source_capture import _capture_source_file
from pycircuit._source_transport import _emit_source_transport

pytestmark = pytest.mark.system


@dataclass(frozen=True)
class CompiledUnit:
    process: subprocess.CompletedProcess[str]
    body: Path
    interface: Path


def _harness() -> Path:
    candidates: list[str | None] = [os.environ.get("ACIR_SOURCE_UNIT_HARNESS")]
    toolchain = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if toolchain:
        candidates.append(str(Path(toolchain) / "bin/acir-source-unit-harness"))
    candidates.append(shutil.which("acir-source-unit-harness"))
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return Path(candidate).resolve()
    raise AssertionError("set ACIR_SOURCE_UNIT_HARNESS for lexical module tests")


def _compile(
    tmp_path: Path,
    stem: str,
    text: str,
    *,
    headers: tuple[Path, ...] = (),
) -> CompiledUnit:
    source_root = tmp_path / "source"
    source_root.mkdir(exist_ok=True)
    source = source_root / f"{stem}.py"
    source.write_text(text, encoding="utf-8")
    output = tmp_path / f"{stem}-out"
    output.mkdir()
    transport = output / f"{stem}.transport.mlir"
    body = output / f"{stem}.body.mlir"
    interface = output / f"{stem}.interface.mlir"
    transport.write_text(
        _emit_source_transport(_capture_source_file(source, source_root=source_root)),
        encoding="utf-8",
    )
    command = [
        str(_harness()),
        "--capture",
        str(transport),
        "--package",
        "verify",
        "--path",
        source.name,
    ]
    for header in headers:
        command.extend(("--header", str(header)))
    command.extend(("--body-out", str(body), "--interface-out", str(interface)))
    return CompiledUnit(
        subprocess.run(command, text=True, capture_output=True, check=False),
        body,
        interface,
    )


WORD_ALIAS = "from typing import Annotated\nWord = Annotated[int, range(16)]\n"


def _operation_lines(text: str, name: str) -> list[str]:
    quoted = f'"{name}"'
    custom = re.compile(rf"(?:^|\s){re.escape(name)}(?:\s|\()")
    return [
        line.strip()
        for line in text.splitlines()
        if quoted in line or custom.search(line)
    ]


def _dictionary_attr(line: str, name: str) -> str:
    marker = f"{name} = "
    start = line.index(marker) + len(marker)
    assert line[start] == "{", line
    depth = 0
    for index in range(start, len(line)):
        if line[index] == "{":
            depth += 1
        elif line[index] == "}":
            depth -= 1
            if depth == 0:
                return line[start : index + 1]
    raise AssertionError(f"unterminated {name} dictionary: {line}")


def _operation_operands(line: str, name: str) -> list[str]:
    match = re.search(rf'(?:"{re.escape(name)}"|{re.escape(name)})\(([^)]*)\)', line)
    assert match is not None, line
    return [operand.strip() for operand in match.group(1).split(",")]


def _operation_results(line: str, count: int) -> list[str]:
    prefix = line.split("=", 1)[0].strip()
    explicit = [result.strip() for result in prefix.split(",")]
    if len(explicit) == count and all(
        re.fullmatch(r"%[A-Za-z0-9_]+", result) for result in explicit
    ):
        return explicit
    multiple = re.fullmatch(r"%([A-Za-z0-9_]+):(\d+)", prefix)
    if multiple is not None:
        assert int(multiple.group(2)) == count, line
        return [f"%{multiple.group(1)}#{index}" for index in range(count)]
    assert count == 1 and re.fullmatch(r"%[A-Za-z0-9_]+", prefix), line
    return [prefix]


def _rule_text(body: str) -> str:
    return body.split('"ac.rule"', 1)[1].split('"ac.yield"() : () -> ()', 1)[0]


def _rule_arguments(rule: str) -> list[tuple[str, str]]:
    marker = "^bb0("
    start = rule.find(marker)
    assert start >= 0, rule
    opening = start + len(marker) - 1
    depth = 0
    quoted = False
    escaped = False
    closing = -1
    for index in range(opening, len(rule)):
        character = rule[index]
        if quoted:
            if escaped:
                escaped = False
            elif character == "\\":
                escaped = True
            elif character == '"':
                quoted = False
            continue
        if character == '"':
            quoted = True
        elif character == "(":
            depth += 1
        elif character == ")":
            depth -= 1
            if depth == 0:
                closing = index
                break
    assert closing >= 0 and rule[closing + 1 :].lstrip().startswith(":"), rule
    arguments = rule[opening + 1 : closing]
    return re.findall(r"(%[A-Za-z0-9_]+):\s*([^,\s]+)", arguments)


def _yield_operands(rule: str) -> list[str]:
    lines = _operation_lines(rule, "ac.yield")
    assert len(lines) == 1, lines
    return _operation_operands(lines[0], "ac.yield")


def test_v02_function_module_nested_rule_and_bare_return_are_accepted(
    tmp_path: Path,
) -> None:
    unit = _compile(
        tmp_path,
        "counter",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def Counter():\n"
        + "    q: Word = 0\n\n"
        + "    @rule\n"
        + "    def hold():\n"
        + "        nonlocal q\n"
        + "        q = q\n"
        + "        return\n\n"
        + "    hold()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    assert '"ac.module"()' in body
    assert "ac.reg" in body
    assert "ac.rule" in body
    assert "ac.dff" not in body
    assert "ac.dffe" not in body


def test_v02_class_and_self_module_authoring_is_a_hard_error(tmp_path: Path) -> None:
    unit = _compile(
        tmp_path,
        "retired_class",
        WORD_ALIAS
        + "from pycircuit import module\n\n"
        + "@module\n"
        + "class Retired:\n"
        + "    def __init__(self):\n"
        + "        self.q: Word = 0\n",
    )

    assert unit.process.returncode != 0
    assert "class/self authoring has been retired" in unit.process.stderr


def test_v03_nonlocal_write_reads_one_canonical_q_slot(tmp_path: Path) -> None:
    unit = _compile(
        tmp_path,
        "alias_read",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def AliasRead(source: Word, sink: Word):\n"
        + "    alias = source\n\n"
        + "    @rule\n"
        + "    def forward():\n"
        + "        nonlocal sink\n"
        + "        sink = alias\n"
        + "        return\n\n"
        + "    forward()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    rule = _rule_text(unit.body.read_text(encoding="utf-8"))
    assert "(!ac.reg<i4>, !ac.reg<i4>)" in rule
    assert (
        'ac.input_bindings = [{kind = "formal", ordinal, parameter = "source"}]' in rule
    )
    assert (
        'ac.output_bindings = [{kind = "formal", ordinal, parameter = "sink"}]' in rule
    )
    arguments = _rule_arguments(rule)
    assert len(arguments) == 1 and arguments[0][1] == "i4"
    reads = _operation_lines(rule, "ac.source.read")
    uses = _operation_lines(rule, "ac.source.use")
    assert len(reads) == 1
    assert len(uses) == 1
    assert _operation_operands(reads[0], "ac.source.read") == [arguments[0][0]]
    read_result = _operation_results(reads[0], 1)[0]
    use_operands = _operation_operands(uses[0], "ac.source.use")
    assert use_operands[0] == read_result
    identity = _dictionary_attr(uses[0], "id")
    assert 'role = "next"' in identity and "slot = 0 : i32" in identity
    assert _dictionary_attr(uses[0], "source")
    target = _dictionary_attr(uses[0], "target")
    assert 'kind = "next_scalar"' in target
    assert 'kind = "formal"' in target
    assert 'parameter = "sink"' in target
    assert _yield_operands(rule) == _operation_results(uses[0], 2)


def test_v03_assignment_without_nonlocal_is_rejected(tmp_path: Path) -> None:
    unit = _compile(
        tmp_path,
        "missing_nonlocal",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def MissingNonlocal():\n"
        + "    q: Word = 0\n\n"
        + "    @rule\n"
        + "    def update():\n"
        + "        q = q\n"
        + "        return\n\n"
        + "    update()\n",
    )

    assert unit.process.returncode != 0
    assert "local or lacks a nonlocal reg declaration" in unit.process.stderr


def test_v03_local_ssa_can_feed_a_nonlocal_proposal(tmp_path: Path) -> None:
    unit = _compile(
        tmp_path,
        "local_ssa",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def LocalSSA(source: Word, sink: Word):\n"
        + "    @rule\n"
        + "    def forward():\n"
        + "        nonlocal sink\n"
        + "        candidate = source\n"
        + "        sink = candidate\n"
        + "        return\n\n"
        + "    forward()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    rule = _rule_text(unit.body.read_text(encoding="utf-8"))
    arguments = _rule_arguments(rule)
    assert len(arguments) == 1 and arguments[0][1] == "i4"
    reads = _operation_lines(rule, "ac.source.read")
    uses = _operation_lines(rule, "ac.source.use")
    assert len(reads) == 1
    assert len(uses) == 1
    assert _operation_operands(reads[0], "ac.source.read") == [arguments[0][0]]
    read_result = _operation_results(reads[0], 1)[0]
    assert _operation_operands(uses[0], "ac.source.use")[0] == read_result
    assert _yield_operands(rule) == _operation_results(uses[0], 2)


def test_v03_owned_reg_alias_reuses_one_state_for_read_and_write(
    tmp_path: Path,
) -> None:
    unit = _compile(
        tmp_path,
        "owned_alias",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def OwnedAlias():\n"
        + "    q: Word = 0\n"
        + "    alias = q\n\n"
        + "    @rule\n"
        + "    def hold():\n"
        + "        nonlocal alias\n"
        + "        alias = alias\n"
        + "        return\n\n"
        + "    hold()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    assert body.count('"ac.reg"') == 1
    assert "ac.ports = []" in body
    assert "ac.queue" not in body
    rule = body.split('"ac.rule"', 1)[1].split('"ac.yield"() : () -> ()', 1)[0]
    operands = re.search(r"^\((%[^,]+), (%[^)]+)\)", rule)
    assert operands is not None
    assert operands.group(1) == operands.group(2)
    input_binding = rule.split("ac.input_bindings = ", 1)[1].split(
        ", ac.input_types = ", 1
    )[0]
    output_binding = rule.split("ac.output_bindings = ", 1)[1].split(
        ", ac.output_types = ", 1
    )[0]
    assert input_binding == output_binding


def test_v14_conditional_write_uses_condition_as_enable_and_bare_return_holds(
    tmp_path: Path,
) -> None:
    unit = _compile(
        tmp_path,
        "conditional_write",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def ConditionalWrite(source: Word, sink: Word, ready: bool):\n"
        + "    @rule\n"
        + "    def forward():\n"
        + "        nonlocal sink\n"
        + "        if ready:\n"
        + "            sink = source\n"
        + "            return\n"
        + "        return\n\n"
        + "    forward()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    rule = _rule_text(unit.body.read_text(encoding="utf-8"))
    arguments = _rule_arguments(rule)
    ready = [name for name, type_name in arguments if type_name == "i1"]
    assert len(ready) == 1
    reads = _operation_lines(rule, "ac.source.read")
    uses = _operation_lines(rule, "ac.source.use")
    assert len(reads) == 2
    assert len(uses) == 1
    ready_reads = [
        read
        for read in reads
        if _operation_operands(read, "ac.source.read") == [ready[0]]
    ]
    assert len(ready_reads) == 1
    data_reads = [read for read in reads if read not in ready_reads]
    assert len(data_reads) == 1
    use_operands = _operation_operands(uses[0], "ac.source.use")
    assert use_operands[0] == _operation_results(data_reads[0], 1)[0]
    assert use_operands[2] == _operation_results(ready_reads[0], 1)[0]
    assert use_operands[1] != use_operands[2]
    assert _yield_operands(rule) == _operation_results(uses[0], 2)


def test_w04_each_direct_q_read_has_a_distinct_source_occurrence(
    tmp_path: Path,
) -> None:
    unit = _compile(
        tmp_path,
        "repeated_reads",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def RepeatedReads(source: Word, sink: Word):\n"
        + "    @rule\n"
        + "    def forward():\n"
        + "        nonlocal sink\n"
        + "        first = source\n"
        + "        second = source\n"
        + "        sink = second\n"
        + "        return\n\n"
        + "    forward()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    rule = _rule_text(unit.body.read_text(encoding="utf-8"))
    reads = _operation_lines(rule, "ac.source.read")
    assert len(reads) == 2
    arguments = _rule_arguments(rule)
    assert len(arguments) == 1
    assert all(
        _operation_operands(read, "ac.source.read") == [arguments[0][0]]
        for read in reads
    )
    origins = [_dictionary_attr(read, "ac.origin") for read in reads]
    assert origins[0] != origins[1]


def test_w04_local_cache_reuse_does_not_duplicate_source_read(tmp_path: Path) -> None:
    unit = _compile(
        tmp_path,
        "cached_read",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def CachedRead(source: Word, sink: Word):\n"
        + "    @rule\n"
        + "    def forward():\n"
        + "        nonlocal sink\n"
        + "        cached = source\n"
        + "        sink = cached\n"
        + "        return\n\n"
        + "    forward()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    rule = _rule_text(unit.body.read_text(encoding="utf-8"))
    reads = _operation_lines(rule, "ac.source.read")
    assert len(reads) == 1
    assert _dictionary_attr(reads[0], "ac.origin")


def test_w04_each_nonlocal_assignment_uses_canonical_target_and_yield(
    tmp_path: Path,
) -> None:
    unit = _compile(
        tmp_path,
        "two_next_uses",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def TwoNextUses(source: Word, left: Word, right: Word):\n"
        + "    @rule\n"
        + "    def forward():\n"
        + "        nonlocal left, right\n"
        + "        left = source\n"
        + "        right = source\n"
        + "        return\n\n"
        + "    forward()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    rule = _rule_text(unit.body.read_text(encoding="utf-8"))
    uses = _operation_lines(rule, "ac.source.use")
    assert len(uses) == 2
    expected_targets = ("left", "right")
    expected_yield: list[str] = []
    for use, target_name in zip(uses, expected_targets, strict=True):
        identity = _dictionary_attr(use, "id")
        assert 'role = "next"' in identity
        assert "origin = {" in identity
        assert "slot = 0 : i32" in identity
        assert _dictionary_attr(use, "source")
        target = _dictionary_attr(use, "target")
        assert 'kind = "next_scalar"' in target
        assert 'kind = "formal"' in target
        assert f'parameter = "{target_name}"' in target
        expected_yield.extend(_operation_results(use, 2))
    assert _yield_operands(rule) == expected_yield


def test_w04_alias_read_keeps_source_origin_on_the_canonical_q_slot(
    tmp_path: Path,
) -> None:
    unit = _compile(
        tmp_path,
        "alias_source_fact",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def AliasSourceFact(source: Word, sink: Word):\n"
        + "    alias = source\n\n"
        + "    @rule\n"
        + "    def forward():\n"
        + "        nonlocal sink\n"
        + "        sink = alias\n"
        + "        return\n\n"
        + "    forward()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    rule = _rule_text(unit.body.read_text(encoding="utf-8"))
    reads = _operation_lines(rule, "ac.source.read")
    assert len(reads) == 1
    arguments = _rule_arguments(rule)
    assert len(arguments) == 1
    assert _operation_operands(reads[0], "ac.source.read") == [arguments[0][0]]
    assert _dictionary_attr(reads[0], "ac.origin")
    assert 'parameter = "source"' in rule
    assert 'parameter = "alias"' not in rule


def test_w04_conditional_next_use_preserves_path_and_inactive_rule_has_no_facts(
    tmp_path: Path,
) -> None:
    unit = _compile(
        tmp_path,
        "conditional_active_facts",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def ConditionalActiveFacts(source: Word, sink: Word, ready: bool):\n"
        + "    @rule\n"
        + "    def inactive():\n"
        + "        nonlocal sink\n"
        + "        sink = source\n"
        + "        return\n\n"
        + "    @rule\n"
        + "    def active():\n"
        + "        nonlocal sink\n"
        + "        if ready:\n"
        + "            sink = source\n"
        + "            return\n"
        + "        return\n\n"
        + "    active()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    assert body.count('"ac.rule"') == 1
    assert 'name = "active"' in body
    assert 'name = "inactive"' not in body
    rule = _rule_text(body)
    reads = _operation_lines(rule, "ac.source.read")
    uses = _operation_lines(rule, "ac.source.use")
    assert len(reads) == 2
    assert len(uses) == 1
    arguments = _rule_arguments(rule)
    ready = [name for name, type_name in arguments if type_name == "i1"]
    assert len(ready) == 1
    ready_reads = [
        read
        for read in reads
        if _operation_operands(read, "ac.source.read") == [ready[0]]
    ]
    assert len(ready_reads) == 1
    data_reads = [read for read in reads if read not in ready_reads]
    assert len(data_reads) == 1
    use_operands = _operation_operands(uses[0], "ac.source.use")
    assert use_operands[0] == _operation_results(data_reads[0], 1)[0]
    assert use_operands[2] == _operation_results(ready_reads[0], 1)[0]
    assert use_operands[1] != use_operands[2]
    use_results = _operation_results(uses[0], 2)
    assert _yield_operands(rule) == use_results


def test_w04_two_registrations_preserve_rules_and_union_public_origins(
    tmp_path: Path,
) -> None:
    unit = _compile(
        tmp_path,
        "two_registrations",
        "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def TwoRegistrations(source: bool, sink: bool):\n"
        + "    @rule\n"
        + "    def forward():\n"
        + "        nonlocal sink\n"
        + "        sink = source\n"
        + "        return\n\n"
        + "    forward()\n"
        + "    forward()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    rules = _operation_lines(body, "ac.rule")
    reads = _operation_lines(body, "ac.source.read")
    uses = _operation_lines(body, "ac.source.use")
    assert len(rules) == 2
    assert len(reads) == 2
    assert len(uses) == 2
    registrations = [_dictionary_attr(rule, "registration") for rule in rules]
    assert registrations[0] != registrations[1]
    read_origins = [_dictionary_attr(read, "ac.origin") for read in reads]
    use_origins = [
        _dictionary_attr(_dictionary_attr(use, "id"), "origin") for use in uses
    ]
    assert read_origins[0] == read_origins[1]
    assert use_origins[0] == use_origins[1]

    declarations = [
        declaration
        for declaration in _operation_lines(
            unit.interface.read_text(encoding="utf-8"), "ac.module.import"
        )
        if "verify.two_registrations.TwoRegistrations" in declaration
    ]
    assert len(declarations) == 1
    assert declarations[0].count(read_origins[0]) == 1
    assert declarations[0].count(use_origins[0]) == 1


def test_w05_child_effects_use_parent_instance_origin(tmp_path: Path) -> None:
    child = _compile(
        tmp_path,
        "child_effects",
        "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def ChildEffects(source: bool, sink: bool):\n"
        + "    @rule\n"
        + "    def forward():\n"
        + "        nonlocal sink\n"
        + "        sink = source\n"
        + "        return\n\n"
        + "    forward()\n",
    )
    assert child.process.returncode == 0, child.process.stderr

    parent = _compile(
        tmp_path,
        "parent_effects",
        "from pycircuit import module\n"
        + "from .child_effects import ChildEffects\n\n"
        + "@module\n"
        + "def ParentEffects(source: bool, sink: bool):\n"
        + "    child = ChildEffects(source, sink)\n",
        headers=(child.interface,),
    )
    assert parent.process.returncode == 0, parent.process.stderr

    body = parent.body.read_text(encoding="utf-8")
    instances = _operation_lines(body, "ac.instance")
    assert len(instances) == 1
    instance_origin = _dictionary_attr(instances[0], "ac.origin")
    interface = parent.interface.read_text(encoding="utf-8")
    declarations = [
        declaration
        for declaration in _operation_lines(interface, "ac.module.import")
        if "verify.parent_effects.ParentEffects" in declaration
    ]
    assert len(declarations) == 1
    contract_line = declarations[0]
    assert contract_line.count(instance_origin) == 2
    assert "@verify.child_effects.ChildEffects" not in contract_line


@pytest.mark.parametrize("imported", ["Queue", "FIFO", "Queue as LocalQueue"])
def test_v05_framework_storage_identities_are_rejected(
    tmp_path: Path, imported: str
) -> None:
    unit = _compile(
        tmp_path,
        "framework_storage",
        f"from pycircuit import module, {imported}\n\n"
        + "@module\n"
        + "def BadStorage():\n"
        + "    return\n",
    )

    assert unit.process.returncode != 0
    assert "exposes no Queue/FIFO/Interface/Reg/Signal API" in unit.process.stderr


def test_v05_ordinary_queue_fifo_ready_push_names_are_accepted(tmp_path: Path) -> None:
    unit = _compile(
        tmp_path,
        "ordinary_names",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def OrdinaryNames(Queue: Word, FIFO: Word, ready: Word, push: Word):\n"
        + "    @rule\n"
        + "    def copy():\n"
        + "        nonlocal ready, push\n"
        + "        ready = Queue\n"
        + "        push = FIFO\n"
        + "        return\n\n"
        + "    copy()\n",
    )

    assert unit.process.returncode == 0, unit.process.stderr
    body = unit.body.read_text(encoding="utf-8")
    assert body.count("ac.reg") >= 1
    assert "ac.queue" not in body


def test_v02_lexical_parent_rejects_retired_self_child_actual(tmp_path: Path) -> None:
    child = _compile(
        tmp_path,
        "child",
        WORD_ALIAS
        + "from pycircuit import module, rule\n\n"
        + "@module\n"
        + "def Child(source: Word, sink: Word):\n"
        + "    @rule\n"
        + "    def forward():\n"
        + "        nonlocal sink\n"
        + "        sink = source\n"
        + "        return\n\n"
        + "    forward()\n",
    )
    assert child.process.returncode == 0, child.process.stderr

    parent = _compile(
        tmp_path,
        "parent",
        WORD_ALIAS
        + "from pycircuit import module\n"
        + "from .child import Child\n\n"
        + "@module\n"
        + "def Parent():\n"
        + "    source: Word = 0\n"
        + "    sink: Word = 0\n"
        + "    child = Child(self.source, sink)\n",
        headers=(child.interface,),
    )

    assert parent.process.returncode != 0
    assert "previously declared reg or connection alias" in parent.process.stderr


def test_v02_imported_system_header_cannot_be_instantiated_as_child(
    tmp_path: Path,
) -> None:
    root = _compile(
        tmp_path,
        "root_system",
        WORD_ALIAS
        + "from pycircuit import system, rule\n\n"
        + "@system\n"
        + "def RootSystem():\n"
        + "    q: Word = 0\n\n"
        + "    @rule\n"
        + "    def hold():\n"
        + "        nonlocal q\n"
        + "        q = q\n"
        + "        return\n\n"
        + "    hold()\n",
    )
    assert root.process.returncode == 0, root.process.stderr
    assert 'ac.root_kind = "system"' in root.interface.read_text(encoding="utf-8")

    parent = _compile(
        tmp_path,
        "system_parent",
        "from pycircuit import module\n"
        + "from .root_system import RootSystem\n\n"
        + "@module\n"
        + "def SystemParent():\n"
        + "    child = RootSystem()\n",
        headers=(root.interface,),
    )

    assert parent.process.returncode != 0
    assert "cannot be instantiated as a child module" in parent.process.stderr

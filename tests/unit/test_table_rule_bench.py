"""Preserve the table_rule system trace without importing the design source."""

from __future__ import annotations

import ast
import hashlib
import json
from collections import deque
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]
BENCH = ROOT / "examples/table_rule/bench.py"

FIELDS = (
    "valid",
    "index",
    "value",
    "take",
    "expected_ready",
    "expected_valid",
    "expected_data_index",
    "expected_data_value",
)
FIELD_TYPES = {
    "valid": "u1",
    "index": "u1",
    "value": "u7",
    "take": "u1",
    "expected_ready": "u1",
    "expected_valid": "u1",
    "expected_data_index": "u1",
    "expected_data_value": "u7",
}
TRACE_SHA256 = "b6f14b66b0478891bb010275a6212e0afac4ff0ceb522ff69767a0f802c2faa8"


def _tree() -> ast.Module:
    return ast.parse(BENCH.read_text(encoding="utf-8"), filename=str(BENCH))


def _function(tree: ast.Module, name: str) -> ast.FunctionDef:
    functions = [
        node
        for node in tree.body
        if isinstance(node, ast.FunctionDef) and node.name == name
    ]
    assert len(functions) == 1
    return functions[0]


def _assignment(function: ast.FunctionDef, name: str) -> ast.expr:
    values = []
    for statement in function.body:
        if isinstance(statement, ast.Assign) and len(statement.targets) == 1:
            target = statement.targets[0]
            if isinstance(target, ast.Name) and target.id == name:
                values.append(statement.value)
        elif isinstance(statement, ast.AnnAssign):
            if isinstance(statement.target, ast.Name) and statement.target.id == name:
                assert statement.value is not None
                values.append(statement.value)
    assert len(values) == 1
    return values[0]


def _integer(node: ast.expr) -> int:
    assert isinstance(node, ast.Constant) and type(node.value) is int
    return node.value


def _scenario_rows(tree: ast.Module) -> list[tuple[int, ...]]:
    declarations = [
        node
        for node in tree.body
        if isinstance(node, ast.ClassDef) and node.name == "Scenario"
    ]
    assert len(declarations) == 1
    defaults = {}
    for statement in declarations[0].body:
        if isinstance(statement, ast.AnnAssign) and isinstance(
            statement.target, ast.Name
        ):
            if statement.value is not None:
                defaults[statement.target.id] = _integer(statement.value)

    system = _function(tree, "ExerciseTableRule")
    scenarios = _assignment(system, "scenarios")
    assert isinstance(scenarios, ast.Call)
    assert isinstance(scenarios.func, ast.Name) and scenarios.func.id == "Scenarios"
    assert not scenarios.args
    assert [keyword.arg for keyword in scenarios.keywords] == ["rows"]
    literal = scenarios.keywords[0].value
    assert isinstance(literal, ast.Tuple | ast.List)

    rows = []
    for element in literal.elts:
        assert isinstance(element, ast.Call)
        assert isinstance(element.func, ast.Name) and element.func.id == "Scenario"
        assert not element.args
        values = dict(defaults)
        for keyword in element.keywords:
            assert keyword.arg is not None and keyword.arg not in values
            values[keyword.arg] = _integer(keyword.value)
        assert set(values) == set(FIELDS)
        rows.append(tuple(values[field] for field in FIELDS))
    return rows


def _known_stimulus() -> list[tuple[int, int, int, int]]:
    """Translate the retained driver's known rising-edge stream, without resets."""

    rows: list[tuple[int, int, int, int]] = []

    def edge(index: int = 0, value: int = 0, valid: int = 0, take: int = 1):
        rows.append((valid, index, value, take))

    def drain(cycles: int = 8):
        for _ in range(cycles):
            edge()

    def fill():
        edge(0, 50, 1, 0)
        edge(1, 60, 1, 0)
        edge(0, 70, 1, 0)

    edge(take=0)
    for index, value in ((0, 10), (0, 20), (1, 30), (0, 40)):
        edge(index, value, 1)
    drain()
    fill()
    edge(1, 80, 1, 0)
    edge(1, 110, 1, 0)
    for value in range(12):
        edge(value % 2, (value * 11) % 128, 1)
    drain()
    fill()
    edge()
    drain()

    for packed in range(256):
        index, value = packed >> 7, packed & 127
        edge(index, value, 1)
        edge(index, 3, 1)
        edge(index ^ 1, 5, 1)
        edge()
        edge()

    fill()
    edge(valid=1)
    drain(9)  # The authored system retains one final observation.
    assert len(rows) == 1343
    return rows


def _expected_rows() -> list[tuple[int, ...]]:
    """Apply the independent two-input/one-output queue and old-row model."""

    pending: deque[tuple[int, int]] = deque()
    results: deque[tuple[int, int]] = deque()
    stored = [(0, 0), (0, 0)]
    rows = []

    for valid, index, value, take in _known_stimulus():
        pop = bool(results) and bool(take)
        room = not results or pop
        move = bool(pending) and room
        ready = len(pending) < 2 or move
        output = results[0] if results else (0, 0)
        rows.append(
            (
                valid,
                index,
                value,
                take,
                int(ready),
                int(bool(results)),
                output[0],
                output[1],
            )
        )

        if pop:
            results.popleft()
        if move:
            replacement = pending.popleft()
            results.append(stored[replacement[0]])
            stored[replacement[0]] = replacement
        if valid and ready:
            pending.append((index, value))

        assert len(pending) <= 2 and len(results) <= 1

    assert not pending and not results
    return rows


def _selection_value(node: ast.expr, phase: int) -> int:
    if isinstance(node, ast.Constant):
        return _integer(node)
    if isinstance(node, ast.Name):
        assert node.id == "phase"
        return phase
    if isinstance(node, ast.Compare):
        assert len(node.ops) == 1 and isinstance(node.ops[0], ast.Lt)
        assert len(node.comparators) == 1
        return int(
            _selection_value(node.left, phase)
            < _selection_value(node.comparators[0], phase)
        )
    if isinstance(node, ast.IfExp):
        selected = node.body if _selection_value(node.test, phase) else node.orelse
        return _selection_value(selected, phase)
    raise AssertionError(f"unsupported scenario selection node: {ast.dump(node)}")


def test_table_rule_literal_preserves_complete_known_trace():
    actual = _scenario_rows(_tree())
    expected = _expected_rows()

    assert actual == expected
    encoded = json.dumps(actual, separators=(",", ":")).encode()
    assert hashlib.sha256(encoded).hexdigest() == TRACE_SHA256


def test_table_rule_literal_retains_types_and_phase_boundary():
    tree = _tree()
    scenario = next(
        node
        for node in tree.body
        if isinstance(node, ast.ClassDef) and node.name == "Scenario"
    )
    annotations = {
        statement.target.id: ast.unparse(statement.annotation)
        for statement in scenario.body
        if isinstance(statement, ast.AnnAssign)
        and isinstance(statement.target, ast.Name)
    }
    assert annotations == FIELD_TYPES

    scenarios = next(
        node
        for node in tree.body
        if isinstance(node, ast.ClassDef) and node.name == "Scenarios"
    )
    rows = next(
        statement
        for statement in scenarios.body
        if isinstance(statement, ast.AnnAssign)
        and isinstance(statement.target, ast.Name)
        and statement.target.id == "rows"
    )
    assert ast.unparse(rows.annotation) == "table[1343, Scenario]"

    system = _function(tree, "ExerciseTableRule")
    phase = next(
        statement
        for statement in system.body
        if isinstance(statement, ast.AnnAssign)
        and isinstance(statement.target, ast.Name)
        and statement.target.id == "phase"
    )
    assert ast.unparse(phase.annotation) == "bits[64]"
    assert phase.value is not None and _integer(phase.value) == 0

    position = _assignment(system, "position")
    assert [
        _selection_value(position, value) for value in (0, 1342, 1343, 2**64 - 1)
    ] == [
        0,
        1342,
        1342,
        1342,
    ]
    selected = _assignment(system, "scenario")
    assert ast.unparse(selected) == "scenarios.rows[position]"


def test_table_rule_literal_drives_the_existing_checks_and_observations():
    system = _function(_tree(), "ExerciseTableRule")
    assert ast.unparse(_assignment(system, "payload")) == (
        "Entry(index=scenario.index, value=scenario.value)"
    )
    assert ast.unparse(_assignment(system, "dut")) == (
        "TableRule(valid=scenario.valid, data=payload, take=scenario.take)"
    )

    check = next(
        node
        for node in system.body
        if isinstance(node, ast.FunctionDef) and node.name == "check"
    )
    guarded = next(node for node in check.body if isinstance(node, ast.If))
    assert ast.unparse(guarded.test) == "phase < 1343"
    assert [
        ast.unparse(node.test) for node in guarded.body if isinstance(node, ast.Assert)
    ] == [
        "dut.ready == scenario.expected_ready",
        "dut.valid == scenario.expected_valid",
        "dut.data.index == scenario.expected_data_index",
        "dut.data.value == scenario.expected_data_value",
    ]

    logs = [
        node.value
        for node in check.body
        if isinstance(node, ast.Expr)
        and isinstance(node.value, ast.Call)
        and isinstance(node.value.func, ast.Name)
        and node.value.func.id == "log"
    ]
    assert [ast.literal_eval(call.args[1]) for call in logs] == [
        "table_rule.ready",
        "table_rule.valid",
        "table_rule.data.index",
        "table_rule.data.value",
    ]
    assert [ast.unparse(call.args[2]) for call in logs] == [
        "dut.ready",
        "dut.valid",
        "dut.data.index",
        "dut.data.value",
    ]

"""Module ordinary statements match existing helper and Rule semantics."""

from __future__ import annotations

import json
from pathlib import Path

import pytest
from test_source_system_execution import _bundle, _cli, _compile, _ok, _records, _run

pytestmark = pytest.mark.system


def _unit(tmp_path: Path, name: str, source: str) -> Path:
    root = tmp_path / (name + "-source")
    root.mkdir()
    path = root / (name + ".py")
    path.write_text(source, encoding="utf-8")
    unit = tmp_path / (name + "-unit")
    _ok(_compile(root, path.name, unit))
    return unit


def _final(tmp_path: Path, name: str, source: str, top: str = "Exercise") -> Path:
    unit = _unit(tmp_path, name, source)
    final = tmp_path / (name + ".ac")
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            f"checks.{name}.{top}",
            "-o",
            str(final),
        )
    )
    return final


def _system_records(final: Path, tmp_path: Path) -> list[list[dict]]:
    traces = []
    for target in ("cpp", "verilog"):
        binary = _bundle(final, tmp_path, target)
        workers = (1, 2) if target == "cpp" else (1,)
        for worker in workers:
            arguments = [str(binary)] + (
                ["--workers", str(worker)] if target == "cpp" else []
            )
            result = _run(arguments)
            assert result.returncode == 0, result.stdout + result.stderr
            traces.append(_records(result.stdout))
    return traces


IF_PARITY = """import pycircuit as ac
@ac.module
def ModuleIf(value: ac.u8, enabled: ac.u1) -> ac.u8:
    result = value
    if enabled:
        result = result + 1
    return result
def helper_if(value: ac.u8, enabled: ac.u1) -> ac.u8:
    result = value
    if enabled:
        result = result + 1
    return result
@ac.rule
def rule_if(value: ac.u8, enabled: ac.u1) -> ac.u8:
    result = value
    if enabled:
        result = result + 1
    return result
@ac.system(samples=((1, 0),))
def Exercise():
    module_value = ModuleIf(7, 1)
    helper_value = helper_if(7, 1)
    @ac.rule
    def check():
        rule_value = rule_if(7, 1)
        assert module_value == 8
        assert helper_value == 8
        assert rule_value == 8
        ac.log('info', 'if_parity', module_value, helper_value, rule_value)
"""


def test_plain_module_if_matches_helper_and_rule_in_both_backends(
    tmp_path: Path,
) -> None:
    traces = _system_records(_final(tmp_path, "if_parity", IF_PARITY), tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert [value["value"] for value in logs[0][0]["values"]] == ["8", "8", "8"]


def test_pure_module_final_has_no_synthetic_domain_or_storage(tmp_path: Path) -> None:
    unit = _unit(tmp_path, "if_parity", IF_PARITY)
    final = tmp_path / "module-if.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.if_parity.ModuleIf",
            "-o",
            str(final),
        )
    )
    for target in ("cpp", "verilog"):
        output = tmp_path / target
        _ok(_cli("emit", str(final), "--target", target, "-o", str(output)))
        ports = json.loads((output / "ports.json").read_text())
        assert ports["domains"] == {"clock": None, "reset": None}
        assert all(row["name"] not in {"pyc_clk", "pyc_rst"} for row in ports["inputs"])


LOOP_MATCH_PARITY = """import pycircuit as ac
@ac.module
def ModuleCompute(value: ac.u8, selector: ac.u1) -> ac.u8:
    total = value
    for index in range(3):
        total = total + 1
    match selector:
        case 0:
            result = total + 1
        case _:
            result = total + 2
    return result
def helper_compute(value: ac.u8, selector: ac.u1) -> ac.u8:
    total = value
    for index in range(3):
        total = total + 1
    match selector:
        case 0:
            result = total + 1
        case _:
            result = total + 2
    return result
@ac.rule
def rule_compute(value: ac.u8, selector: ac.u1) -> ac.u8:
    total = value
    for index in range(3):
        total = total + 1
    match selector:
        case 0:
            result = total + 1
        case _:
            result = total + 2
    return result
@ac.system(samples=((1, 0),))
def Exercise():
    module_value = ModuleCompute(7, 1)
    helper_value = helper_compute(7, 1)
    @ac.rule
    def check():
        rule_value = rule_compute(7, 1)
        assert module_value == 12
        assert helper_value == 12
        assert rule_value == 12
        ac.log('info', 'loop_match_parity', module_value, helper_value, rule_value)
"""


def test_module_match_and_literal_for_match_helper_rule_both_backends(
    tmp_path: Path,
) -> None:
    traces = _system_records(
        _final(tmp_path, "loop_match_parity", LOOP_MATCH_PARITY), tmp_path
    )
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert [value["value"] for value in logs[0][0]["values"]] == ["12", "12", "12"]


def test_plain_and_child_forced_module_paths_keep_helper_snapshots(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "child_parity",
        """import pycircuit as ac
def add(value: ac.u8, amount: ac.u8) -> ac.u8: return value + amount
@ac.module
def Dummy(value: ac.u8) -> ac.u8: return value
@ac.module
def Plain(value: ac.u8, enabled: ac.u1) -> tuple[ac.u8, ac.u8]:
    snapshot = add(value, 1)
    result = snapshot
    if enabled:
        result = add(result, 1)
    return snapshot, result
@ac.module
def WithChild(value: ac.u8, enabled: ac.u1) -> tuple[ac.u8, ac.u8]:
    child = Dummy(value)
    snapshot = add(child, 1)
    result = snapshot
    if enabled:
        result = add(result, 1)
    return snapshot, result
@ac.system(samples=((1, 0),))
def Exercise():
    plain_before, plain_after = Plain(7, 1)
    child_before, child_after = WithChild(7, 1)
    @ac.rule
    def check():
        assert plain_before == 8
        assert plain_after == 9
        assert child_before == 8
        assert child_after == 9
        ac.log('info', 'child_parity', plain_before, plain_after,
               child_before, child_after)
""",
    )
    traces = _system_records(final, tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]


def test_module_ordinary_values_and_state_writing_rule_keep_one_state_owner(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "stateful",
        """import pycircuit as ac
@ac.module
def Top(incoming: ac.u8) -> ac.u8:
    candidate = incoming + 1
    saved: ac.u8 = 0
    @ac.rule
    def update():
        saved = candidate
    return saved
""",
        "Top",
    )
    text = final.read_text()
    assert text.count('instance_name = "saved"') == 1
    assert 'instance_name = "candidate"' not in text


def test_branch_definitions_require_every_reachable_arm(tmp_path: Path) -> None:
    valid = _final(
        tmp_path,
        "branch_valid",
        """import pycircuit as ac
@ac.module
def Top(value: ac.u8, enabled: ac.u1) -> ac.u8:
    if enabled:
        result = value + 1
    else:
        result = value + 2
    return result
""",
        "Top",
    )
    assert valid.is_file()

    root = tmp_path / "invalid-source"
    root.mkdir()
    path = root / "invalid.py"
    path.write_text(
        """import pycircuit as ac
@ac.module
def Top(value: ac.u8, enabled: ac.u1) -> ac.u8:
    if enabled:
        result = value + 1
    return result
""",
        encoding="utf-8",
    )
    result = _compile(root, path.name, tmp_path / "invalid-unit")
    assert result.returncode != 0
    assert any(
        diagnostic in result.stderr
        for diagnostic in ("branch", "unbound", "unknown hardware value 'result'")
    )


def test_struct_and_table_local_updates_preserve_unmodified_fields(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "aggregate_update",
        """import pycircuit as ac
@ac.struct
class Pair:
    left: ac.u8
    right: ac.u8
@ac.module
def Update(pair: Pair, values: ac.table[2, ac.u8]) -> tuple[Pair, ac.table[2, ac.u8]]:
    updated_pair = pair
    updated_pair.left = pair.left + 1
    updated_values = values
    updated_values[1] = values[1] + 1
    return updated_pair, updated_values
@ac.system(samples=((1, 0),))
def Exercise():
    pair, values = Update(Pair(left=3, right=9), (4, 7))
    @ac.rule
    def check():
        assert pair.left == 4
        assert pair.right == 9
        assert values[0] == 4
        assert values[1] == 8
""",
    )
    traces = _system_records(final, tmp_path)
    assert all(any(row["kind"] == "result" for row in trace) for trace in traces)


def test_scalar_module_annotation_is_state_and_requires_closed_initializer(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "annotation",
        """import pycircuit as ac
@ac.module
def Top() -> ac.u8:
    saved: ac.u8 = 3
    return saved
""",
        "Top",
    )
    assert 'instance_name = "saved"' in final.read_text()

    root = tmp_path / "dynamic-source"
    root.mkdir()
    path = root / "dynamic.py"
    path.write_text(
        """import pycircuit as ac
@ac.module
def Top(value: ac.u8) -> ac.u8:
    saved: ac.u8 = value
    return saved
""",
        encoding="utf-8",
    )
    result = _compile(root, path.name, tmp_path / "dynamic-unit")
    assert result.returncode != 0
    assert "initializer" in result.stderr or "static" in result.stderr


def test_tuple_scalar_name_can_be_reused_for_direct_module_result(
    tmp_path: Path,
) -> None:
    final = _final(
        tmp_path,
        "scalar_rebind",
        """import pycircuit as ac
@ac.module
def Decode(value: ac.u8) -> tuple[ac.u8, ac.u8, ac.u1]:
    return value + 1, value + 2, 1
@ac.module
def HeaderConsumer(tag: ac.u8) -> ac.u8:
    return tag + 10
@ac.module
def Pipeline(value: ac.u8) -> tuple[ac.u8, ac.u8, ac.u8, ac.u1]:
    tag, payload, valid = Decode(value)
    original_tag = tag
    tag = HeaderConsumer(tag)
    return original_tag, tag, payload, valid
@ac.system(samples=((1, 0),))
def Exercise():
    original_tag, tag, payload, valid = Pipeline(7)
    @ac.rule
    def check():
        assert original_tag == 8
        assert tag == 18
        assert payload == 9
        assert valid == 1
        ac.log('info', 'scalar_rebind', original_tag, tag, payload, valid)
""",
    )
    traces = _system_records(final, tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert [value["value"] for value in logs[0][0]["values"]] == [
        "8",
        "18",
        "9",
        True,
    ]


NEGATIVE_MODULES = {
    "direct-state-write": """import pycircuit as ac
@ac.module
def Top(value: ac.u8, enabled: ac.u1) -> ac.u8:
    saved: ac.u8 = 0
    if enabled:
        saved = value
    return saved
""",
    "runtime-allocation": """import pycircuit as ac
@ac.module
def Child(value: ac.u8) -> ac.u8: return value
@ac.module
def Top(value: ac.u8, enabled: ac.u1) -> ac.u8:
    if enabled:
        child = Child(value)
    else:
        child = Child(value)
    return child
""",
    "runtime-registration": """import pycircuit as ac
@ac.rule
def update(saved: ac.u8, value: ac.u8):
    saved = value
@ac.module
def Top(value: ac.u8, enabled: ac.u1) -> ac.u8:
    saved: ac.u8 = 0
    if enabled:
        update(saved, value)
    return saved
""",
    "input-rebind": """import pycircuit as ac
@ac.module
def Top(value: ac.u8) -> ac.u8:
    value = value + 1
    return value
""",
    "import-rebind": """import pycircuit as ac
@ac.module
def Top(value: ac.u8) -> ac.u8:
    ac = value
    return value
""",
    "instance-rebind": """import pycircuit as ac
@ac.module
def Child(value: ac.u8) -> ac.u8: return value
@ac.module
def Top(value: ac.u8) -> ac.u8:
    child = Child(value)
    child = value
    return child
""",
}


@pytest.mark.parametrize("case", sorted(NEGATIVE_MODULES))
def test_module_statement_rebinding_allocation_and_registration_reject(
    tmp_path: Path, case: str
) -> None:
    root = tmp_path / "source"
    root.mkdir()
    path = root / "negative.py"
    path.write_text(NEGATIVE_MODULES[case], encoding="utf-8")
    result = _compile(root, path.name, tmp_path / "unit")
    assert result.returncode != 0
    assert any(
        word in result.stderr
        for word in (
            "persistent",
            "runtime",
            "branch",
            "rebind",
            "replace",
            "instance",
            "Rule call",
        )
    )


def _merge_symbols(left: str, right: str) -> str:
    result = []
    for first, second in zip(left, right, strict=True):
        result.append(first if first == second else "x")
    return "".join(result)


def _bits4(pattern: str) -> dict[str, object]:
    value = known = z = 0
    for symbol in pattern:
        value <<= 1
        known <<= 1
        z <<= 1
        if symbol == "1":
            value |= 1
            known |= 1
        elif symbol == "0":
            known |= 1
        elif symbol == "z":
            z |= 1
    digits = (len(pattern) + 3) // 4
    return {
        "kind": "bits4",
        "width": len(pattern),
        "value": f"{value:0{digits}x}",
        "known": f"{known:0{digits}x}",
        "z": f"{z:0{digits}x}",
    }


def test_module_xz_selector_merges_common_bits_and_z_with_independent_oracle(
    tmp_path: Path,
) -> None:
    left, right = "10z01", "11z00"
    expected = _bits4(_merge_symbols(left, right))
    final = _final(
        tmp_path,
        "xz_select",
        f"""import pycircuit as ac
@ac.module
def Select(selector: ac.u1, left: ac.bits[5], right: ac.bits[5]) -> ac.bits[5]:
    result = left
    if selector:
        result = right
    return result
@ac.system(samples=((1, 0),))
def Exercise():
    selected_x = Select(ac.four_state('x'), ac.four_state({left!r}), ac.four_state({right!r}))
    selected_z = Select(ac.four_state('z'), ac.four_state({left!r}), ac.four_state({right!r}))
    @ac.rule
    def observe():
        ac.log('info', 'selected', selected_x, selected_z)
""",
    )
    traces = _system_records(final, tmp_path)
    logs = [[row for row in trace if row["kind"] == "log"] for trace in traces]
    assert logs[0] == logs[1] == logs[2]
    assert logs[0][0]["values"] == [expected, expected]

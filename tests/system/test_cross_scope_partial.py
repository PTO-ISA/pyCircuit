"""Cross-scope partial writers compose only through the independent disjointness proof."""

from __future__ import annotations

import pytest
from test_source_system_execution import (
    _bundle,
    _cli,
    _compile,
    _ok,
    _records,
    _run,
)

pytestmark = pytest.mark.system


def _provider(local_field: str, local_whole: bool = False) -> str:
    local_update = (
        "            state = Fields(left=state.left, right=state.right + 2)"
        if local_whole
        else f"            state.{local_field} = state.{local_field} + 2"
    )
    return f"""import pycircuit as ac


@ac.struct
class Fields:
    left: ac.u8
    right: ac.u8


@ac.module
def PartialOwner():
    flag: ac.u1 = 0
    state: Fields = Fields(left=0, right=0)

    @ac.method
    def update_left(delta: ac.u8) -> None:
        state.left = state.left + delta

    @ac.method
    def update_right(delta: ac.u8) -> None:
        state.right = state.right + delta

    @ac.method
    def read() -> Fields:
        return state

    @ac.rule
    def local() -> None:
        if flag:
{local_update}

    @ac.rule
    def toggle() -> None:
        flag = ~flag

    local()
    toggle()


@ac.module
def Parent() -> Fields:
    child = PartialOwner()

    @ac.rule
    def drive() -> None:
        child.update_left(1)

    @ac.rule
    def read() -> Fields:
        return child.read()

    drive()
    return read()
"""


BENCH = """import pycircuit as ac
from checks.provider import Parent


@ac.system
def ExerciseXScope():
    parent = Parent()

    @ac.rule
    def observe() -> None:
        ac.log("info", "xscope", parent.left, parent.right)

    @ac.rule
    def tick() -> None:
        pass

    observe()
    tick()
"""


def _program(tmp_path, local_field="right", local_whole=False):
    source = tmp_path / "cross scope sources"
    source.mkdir()
    (source / "provider.py").write_text(_provider(local_field, local_whole))
    (source / "bench.py").write_text(BENCH)
    provider, bench = tmp_path / "provider unit", tmp_path / "bench unit"
    _ok(_compile(source, "provider.py", provider))
    _ok(_compile(source, "bench.py", bench, [provider]))
    final = tmp_path / "cross scope.ac"
    return source, provider, bench, final, _compile


def test_disjoint_cross_scope_field_updates_share_one_transaction_commit(tmp_path):
    source, provider, bench, final, compile_error = _program(tmp_path)
    del source, compile_error
    _ok(
        _cli(
            "link",
            str(provider),
            str(bench),
            "--top",
            "checks.bench.ExerciseXScope",
            "-o",
            str(final),
        )
    )
    expected = []
    left = right = 0
    for _cycle in range(6):
        expected.extend([(left, right)] * 2)
        left = (left + 1) & 255
        if _cycle % 2 == 1:
            right = (right + 2) & 255
    transcripts = []
    cpp_root = tmp_path / "cpp"
    cpp_root.mkdir()
    cpp_binary = _bundle(final, cpp_root, "cpp")
    for workers in (1, 2):
        rows = _records(
            _ok(
                _run(
                    [str(cpp_binary), "--cycles", "6", "--workers", str(workers)],
                    timeout=60,
                )
            )
        )
        events = [
            row
            for row in rows
            if row["kind"] == "log" and row["spec"]["event"] == "xscope"
        ]
        assert [
            tuple(int(value["value"]) for value in row["values"]) for row in events
        ] == expected
        assert [int(row["evaluation_epoch"]) for row in events] == list(range(12))
        terminal = [row for row in rows if row["kind"] == "result"]
        assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"
        assert int(terminal[0]["epoch_time"]) == 12
        transcripts.append([row for row in rows if row["kind"] != "result"])
    verilog_root = tmp_path / "verilog"
    verilog_root.mkdir()
    verilog_binary = _bundle(final, verilog_root, "verilog")
    rows = _records(_ok(_run([str(verilog_binary), "+cycles=6"], timeout=60)))
    events = [
        row for row in rows if row["kind"] == "log" and row["spec"]["event"] == "xscope"
    ]
    assert [
        tuple(int(value["value"]) for value in row["values"]) for row in events
    ] == expected
    transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]


ORDERED_METHOD_PROVIDER = """import pycircuit as ac


@ac.struct
class Fields:
    left: ac.u8
    right: ac.u8


@ac.struct
class Views:
    old: Fields
    ordered: Fields


@ac.module
def Child():
    state: Fields = Fields(left=0, right=7)

    @ac.method
    def update_left(delta: ac.u8) -> None:
        state.left = state.left + delta

    @ac.method
    def read() -> Fields:
        return state


@ac.module
def Parent() -> Views:
    child = Child()

    @ac.rule
    def first() -> None:
        child.update_left(1)

    @ac.rule(after=(first,))
    def second() -> None:
        child.update_left(2)

    @ac.rule
    def read_old() -> Fields:
        return child.read()

    @ac.rule(after=(second,))
    def read_ordered() -> Fields:
        return child.read()

    first()
    second()
    old = read_old()
    ordered = read_ordered()
    return Views(old=old, ordered=ordered)
"""


ORDERED_METHOD_BENCH = """import pycircuit as ac
from checks.ordered_method_provider import Parent


@ac.system
def ExerciseOrderedMethodUpdates():
    views = Parent()

    @ac.rule
    def observe() -> None:
        ac.log(
            "info",
            "ordered method updates",
            views.old.left,
            views.old.right,
            views.ordered.left,
            views.ordered.right,
        )

    observe()
"""


def test_ordered_cross_scope_partial_method_updates_compose(tmp_path):
    source = tmp_path / "ordered method sources"
    source.mkdir()
    (source / "ordered_method_provider.py").write_text(ORDERED_METHOD_PROVIDER)
    (source / "ordered_method_bench.py").write_text(ORDERED_METHOD_BENCH)
    provider, bench = (
        tmp_path / "ordered method provider",
        tmp_path / "ordered method bench",
    )
    _ok(_compile(source, "ordered_method_provider.py", provider))
    _ok(_compile(source, "ordered_method_bench.py", bench, [provider]))
    final = tmp_path / "ordered method.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(bench),
            "--top",
            "checks.ordered_method_bench.ExerciseOrderedMethodUpdates",
            "-o",
            str(final),
        )
    )

    expected = [
        event
        for event in ((0, 7, 3, 7), (3, 7, 6, 7), (6, 7, 9, 7), (9, 7, 12, 7))
        for _ in range(2)
    ]
    transcripts = []
    for target, workers in (("cpp", 1), ("cpp", 2), ("verilog", 1)):
        target_root = tmp_path / f"ordered-method-{target}-{workers}"
        target_root.mkdir()
        binary = _bundle(final, target_root, target)
        arguments = (
            [str(binary), "--cycles", "4", "--workers", str(workers)]
            if target == "cpp"
            else [str(binary), "+cycles=4"]
        )
        rows = _records(_ok(_run(arguments, timeout=60)))
        events = [
            row
            for row in rows
            if row["kind"] == "log" and row["spec"]["event"] == "ordered method updates"
        ]
        assert [
            tuple(int(value["value"]) for value in row["values"]) for row in events
        ] == expected
        transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]


@pytest.mark.parametrize("local_whole", [False, True])
def test_cross_scope_overlapping_writers_reject_without_replacing_output(
    tmp_path, local_whole
):
    _, provider, bench, final, _ = _program(
        tmp_path, local_field="left", local_whole=local_whole
    )
    result = _cli(
        "link",
        str(provider),
        str(bench),
        "--top",
        "checks.bench.ExerciseXScope",
        "-o",
        str(final),
    )
    assert result.returncode != 0
    assert not final.exists()
    assert "disjoint" in (result.stdout + result.stderr)


TABLE_PROVIDER = """import pycircuit as ac


@ac.module
def TableOwner():
    flag: ac.u1 = 0
    rows = ac.table[4, ac.u8](init=0)

    @ac.method
    def write_row(index: ac.u2, value: ac.u8) -> None:
        rows[index] = value

    @ac.method
    def read_row(index: ac.u2) -> ac.u8:
        return rows[index]

    @ac.rule
    def local() -> None:
        if flag:
            write_row(1, read_row(1) + 2)

    @ac.rule
    def toggle() -> None:
        flag = ~flag

    local()
    toggle()


@ac.module
def TableParent() -> ac.u8:
    child = TableOwner()

    @ac.rule
    def drive() -> None:
        child.write_row(0, 1)

    @ac.rule
    def read() -> ac.u8:
        return child.read_row(0) + child.read_row(1)

    drive()
    return read()
"""


TABLE_BENCH = """import pycircuit as ac
from checks.table_provider import TableParent


@ac.system
def ExerciseTableXScope():
    parent = TableParent()

    @ac.rule
    def observe() -> None:
        ac.log("info", "table xscope", parent)

    @ac.rule
    def tick() -> None:
        pass

    observe()
    tick()
"""


def test_disjoint_cross_scope_table_indices_compose_per_element(tmp_path):
    source = tmp_path / "table cross scope sources"
    source.mkdir()
    (source / "table_provider.py").write_text(TABLE_PROVIDER)
    (source / "table_bench.py").write_text(TABLE_BENCH)
    provider, bench = tmp_path / "table provider", tmp_path / "table bench"
    _ok(_compile(source, "table_provider.py", provider))
    _ok(_compile(source, "table_bench.py", bench, [provider]))
    final = tmp_path / "table cross scope.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(bench),
            "--top",
            "checks.table_bench.ExerciseTableXScope",
            "-o",
            str(final),
        )
    )
    # Each cycle writes row zero. Row one receives +2 only when the previous
    # flag was one; both evaluations in an epoch observe the same committed Q.
    row0 = row1 = 0
    expected = []
    for cycle in range(6):
        expected.extend([row0 + row1] * 2)
        row0 = 1
        if cycle % 2 == 1:
            row1 = (row1 + 2) & 255
    transcripts = []
    cpp_root = tmp_path / "table cpp"
    cpp_root.mkdir()
    cpp_binary = _bundle(final, cpp_root, "cpp")
    for workers in (1, 2):
        rows = _records(
            _ok(
                _run(
                    [str(cpp_binary), "--cycles", "6", "--workers", str(workers)],
                    timeout=60,
                )
            )
        )
        events = [
            row
            for row in rows
            if row["kind"] == "log" and row["spec"]["event"] == "table xscope"
        ]
        assert [int(row["values"][0]["value"]) for row in events] == expected
        assert [int(row["evaluation_epoch"]) for row in events] == list(range(12))
        terminal = [row for row in rows if row["kind"] == "result"]
        assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"
        transcripts.append([row for row in rows if row["kind"] != "result"])
    verilog_root = tmp_path / "table verilog"
    verilog_root.mkdir()
    verilog_binary = _bundle(final, verilog_root, "verilog")
    rows = _records(_ok(_run([str(verilog_binary), "+cycles=6"], timeout=60)))
    events = [
        row
        for row in rows
        if row["kind"] == "log" and row["spec"]["event"] == "table xscope"
    ]
    assert [int(row["values"][0]["value"]) for row in events] == expected
    transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]


COMPOSITE_PROVIDER = """from enum import Enum

import pycircuit as ac


@ac.encoding(width=2)
class Tag(Enum):
    ZERO = 0
    ONE = 1


@ac.struct
class Inner:
    value: ac.u8


@ac.struct
class Packet:
    inner: Inner
    flag: ac.u1
    wide: ac.bits[65]
    tag: Tag


@ac.module
def CompositeOwner():
    control: ac.u1 = 0
    state: Packet = Packet(
        inner=Inner(value=0), flag=0, wide=18446744073709551615, tag=Tag.ZERO
    )

    @ac.method
    def update_inner(delta: ac.u8) -> None:
        state.inner.value = state.inner.value + delta

    @ac.method
    def update_rest() -> None:
        state.flag = ~state.flag
        state.wide = state.wide + 3
        state.tag = Tag.ONE if state.tag == Tag.ZERO else Tag.ZERO

    @ac.method
    def read() -> Packet:
        return state

    @ac.rule
    def local() -> None:
        if control:
            update_rest()

    @ac.rule
    def toggle() -> None:
        control = ~control

    local()
    toggle()


@ac.module
def CompositeParent() -> Packet:
    child = CompositeOwner()

    @ac.rule
    def drive() -> None:
        child.update_inner(1)

    @ac.rule
    def read() -> Packet:
        return child.read()

    drive()
    return read()
"""


COMPOSITE_BENCH = """import pycircuit as ac
from checks.composite_provider import CompositeParent


@ac.system
def ExerciseCompositeXScope():
    parent = CompositeParent()

    @ac.rule
    def observe() -> None:
        ac.log(
            "info",
            "composite xscope",
            parent.inner.value,
            ac.enum_to_bits(parent.tag),
            parent.flag,
            parent.wide[:64],
            parent.wide[64:65],
        )

    @ac.rule
    def tick() -> None:
        pass

    observe()
    tick()
"""


def test_cross_scope_partial_name_and_composite_types_compose(tmp_path):
    source = tmp_path / "composite cross scope sources"
    source.mkdir()
    (source / "composite_provider.py").write_text(COMPOSITE_PROVIDER)
    (source / "composite_bench.py").write_text(COMPOSITE_BENCH)
    provider, bench = tmp_path / "composite provider", tmp_path / "composite bench"
    _ok(_compile(source, "composite_provider.py", provider))
    _ok(_compile(source, "composite_bench.py", bench, [provider]))
    final = tmp_path / "composite cross scope.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(bench),
            "--top",
            "checks.composite_bench.ExerciseCompositeXScope",
            "-o",
            str(final),
        )
    )
    inner = tag = flag = 0
    wide = (1 << 64) - 1
    expected = []
    for cycle in range(6):
        expected.extend([(inner, tag, flag, wide & ((1 << 64) - 1), wide >> 64)] * 2)
        inner = (inner + 1) & 255
        if cycle % 2 == 1:
            flag ^= 1
            tag = 1 if tag == 0 else 0
            wide += 3
    transcripts = []
    cpp_root = tmp_path / "composite cpp"
    cpp_root.mkdir()
    cpp_binary = _bundle(final, cpp_root, "cpp")
    for workers in (1, 2):
        rows = _records(
            _ok(
                _run(
                    [str(cpp_binary), "--cycles", "6", "--workers", str(workers)],
                    timeout=60,
                )
            )
        )
        events = [
            row
            for row in rows
            if row["kind"] == "log" and row["spec"]["event"] == "composite xscope"
        ]
        assert [
            tuple(int(value["value"]) for value in row["values"]) for row in events
        ] == expected
        assert [int(row["evaluation_epoch"]) for row in events] == list(range(12))
        terminal = [row for row in rows if row["kind"] == "result"]
        assert len(terminal) == 1 and terminal[0]["status"] == "TERMINATED"
        transcripts.append([row for row in rows if row["kind"] != "result"])
    verilog_root = tmp_path / "composite verilog"
    verilog_root.mkdir()
    verilog_binary = _bundle(final, verilog_root, "verilog")
    rows = _records(_ok(_run([str(verilog_binary), "+cycles=6"], timeout=60)))
    events = [
        row
        for row in rows
        if row["kind"] == "log" and row["spec"]["event"] == "composite xscope"
    ]
    assert [
        tuple(int(value["value"]) for value in row["values"]) for row in events
    ] == expected
    transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]


def test_cross_scope_same_table_index_rejects_without_output(tmp_path):
    source = tmp_path / "table conflict sources"
    source.mkdir()
    (source / "table_provider.py").write_text(
        TABLE_PROVIDER.replace(
            "write_row(1, read_row(1) + 2)",
            "write_row(0, read_row(0) + 2)",
        )
    )
    (source / "table_bench.py").write_text(TABLE_BENCH)
    provider, bench = tmp_path / "conflict provider", tmp_path / "conflict bench"
    _ok(_compile(source, "table_provider.py", provider))
    _ok(_compile(source, "table_bench.py", bench, [provider]))
    final = tmp_path / "table conflict.ac"
    result = _cli(
        "link",
        str(provider),
        str(bench),
        "--top",
        "checks.table_bench.ExerciseTableXScope",
        "-o",
        str(final),
    )
    assert result.returncode != 0
    assert not final.exists()
    assert "collision" in (result.stdout + result.stderr) or "disjoint" in (
        result.stdout + result.stderr
    )


TWO_LEVEL_PROVIDER = """import pycircuit as ac


@ac.struct
class Fields:
    left: ac.u8
    right: ac.u8


@ac.module
def LeafOwner():
    flag: ac.u1 = 0
    state: Fields = Fields(left=0, right=0)

    @ac.method
    def update_left(delta: ac.u8) -> None:
        state.left = state.left + delta

    @ac.method
    def update_right(delta: ac.u8) -> None:
        state.right = state.right + delta

    @ac.method
    def read() -> Fields:
        return state

    @ac.rule
    def local() -> None:
        if flag:
            update_right(2)

    @ac.rule
    def toggle() -> None:
        flag = ~flag

    local()
    toggle()


@ac.module
def RelayOwner():
    leaf = LeafOwner()

    @ac.method
    def update_left(delta: ac.u8) -> None:
        leaf.update_left(delta)

    @ac.method
    def read() -> Fields:
        return leaf.read()


@ac.module
def TopOwner() -> Fields:
    relay = RelayOwner()

    @ac.rule
    def drive() -> None:
        relay.update_left(1)

    @ac.rule
    def read() -> Fields:
        return relay.read()

    drive()
    return read()
"""


TWO_LEVEL_BENCH = """import pycircuit as ac
from checks.two_level_provider import TopOwner


@ac.system
def ExerciseTwoLevelXScope():
    parent = TopOwner()

    @ac.rule
    def observe() -> None:
        ac.log("info", "two level xscope", parent.left, parent.right)

    observe()
"""


def test_two_level_cross_scope_partial_lane_preserves_old_q(tmp_path):
    source = tmp_path / "two level sources"
    source.mkdir()
    (source / "two_level_provider.py").write_text(TWO_LEVEL_PROVIDER)
    (source / "two_level_bench.py").write_text(TWO_LEVEL_BENCH)
    provider, bench = tmp_path / "two level provider", tmp_path / "two level bench"
    _ok(_compile(source, "two_level_provider.py", provider))
    _ok(_compile(source, "two_level_bench.py", bench, [provider]))
    final = tmp_path / "two level.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(bench),
            "--top",
            "checks.two_level_bench.ExerciseTwoLevelXScope",
            "-o",
            str(final),
        )
    )
    expected = []
    left = right = 0
    for cycle in range(6):
        expected.extend([(left, right)] * 2)
        left = (left + 1) & 255
        if cycle % 2 == 1:
            right = (right + 2) & 255
    target_root = tmp_path / "cpp"
    target_root.mkdir()
    binary = _bundle(final, target_root, "cpp")
    for workers in (1, 2):
        rows = _records(
            _ok(
                _run(
                    [str(binary), "--cycles", "6", "--workers", str(workers)],
                    timeout=60,
                )
            )
        )
        events = [
            row
            for row in rows
            if row["kind"] == "log" and row["spec"]["event"] == "two level xscope"
        ]
        assert [
            tuple(int(value["value"]) for value in row["values"]) for row in events
        ] == expected
        assert [int(row["evaluation_epoch"]) for row in events] == list(range(12))


EXCLUSIVE_PROVIDER = """import pycircuit as ac


@ac.struct
class Fields:
    left: ac.u8
    right: ac.u8


@ac.module
def ExclusiveOwner():
    control: ac.u1 = 0
    state: Fields = Fields(left=0, right=0)

    @ac.method
    def update_left(delta: ac.u8) -> None:
        state.left = state.left + delta

    @ac.method
    def read_control() -> ac.u1:
        return control

    @ac.method
    def read() -> Fields:
        return state

    @ac.rule
    def local() -> None:
        if control:
            update_left(2)

    @ac.rule
    def toggle() -> None:
        control = ~control

    local()
    toggle()


@ac.module
def ExclusiveParent() -> Fields:
    child = ExclusiveOwner()

    @ac.rule
    def drive() -> None:
        control = child.read_control()
        if not control:
            child.update_left(1)

    @ac.rule
    def read() -> Fields:
        return child.read()

    drive()
    return read()
"""


EXCLUSIVE_BENCH = """import pycircuit as ac
from checks.exclusive_provider import ExclusiveParent


@ac.system
def ExerciseExclusiveXScope():
    parent = ExclusiveParent()

    @ac.rule
    def observe() -> None:
        ac.log("info", "exclusive xscope", parent.left)

    observe()
"""


def test_cross_scope_same_field_exclusive_paths_share_one_owner(tmp_path):
    source = tmp_path / "exclusive sources"
    source.mkdir()
    (source / "exclusive_provider.py").write_text(EXCLUSIVE_PROVIDER)
    (source / "exclusive_bench.py").write_text(EXCLUSIVE_BENCH)
    provider, bench = tmp_path / "exclusive provider", tmp_path / "exclusive bench"
    _ok(_compile(source, "exclusive_provider.py", provider))
    _ok(_compile(source, "exclusive_bench.py", bench, [provider]))
    final = tmp_path / "exclusive.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(bench),
            "--top",
            "checks.exclusive_bench.ExerciseExclusiveXScope",
            "-o",
            str(final),
        )
    )
    expected = []
    left = 0
    for cycle in range(6):
        expected.extend([left] * 2)
        left = (left + (1 if cycle % 2 == 0 else 2)) & 255
    target_root = tmp_path / "cpp"
    target_root.mkdir()
    binary = _bundle(final, target_root, "cpp")
    rows = _records(
        _ok(_run([str(binary), "--cycles", "6", "--workers", "1"], timeout=60))
    )
    events = [
        row
        for row in rows
        if row["kind"] == "log" and row["spec"]["event"] == "exclusive xscope"
    ]
    assert [int(row["values"][0]["value"]) for row in events] == expected

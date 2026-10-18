"""Plain DFF is selected only from a constant-true committed update.

DFFE remains the default: a guarded or resource-gated state must hold when its
commit enable is not one. Whole-System commit permission stays inside both
storage leaves; this test observes only generated C++/Verilog behavior.
"""

from __future__ import annotations

import re
from pathlib import Path

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


PROVIDER = """from enum import Enum

import pycircuit as ac


@ac.struct
class Pair:
    left: ac.u8
    right: ac.u8


@ac.encoding(width=2)
class Tag(Enum):
    ZERO = 0
    ONE = 1


@ac.struct
class Inner:
    value: ac.u8


@ac.struct
class Nested:
    inner: Inner
    tag: Tag


@ac.module
def FreeCell() -> ac.u8:
    count: ac.u8 = 0

    @ac.rule
    def advance() -> None:
        count = count + 1

    @ac.rule
    def value() -> ac.u8:
        return count

    advance()
    return value()


@ac.module
def GatedCell(enable: ac.u1) -> ac.u8:
    count: ac.u8 = 0

    @ac.rule
    def advance() -> None:
        if enable:
            count = count + 1

    @ac.rule
    def value() -> ac.u8:
        return count

    advance()
    return value()


@ac.module
def TypedCell() -> tuple[bool, ac.bits[65], Nested]:
    flag: bool = False
    wide: ac.bits[65] = 0
    nested: Nested = Nested(inner=Inner(value=0), tag=Tag.ZERO)

    @ac.rule
    def advance() -> None:
        flag = not flag
        wide = wide + 1
        nested = Nested(
            inner=Inner(value=nested.inner.value + 1),
            tag=Tag.ONE if nested.tag == Tag.ZERO else Tag.ZERO,
        )

    @ac.rule
    def value() -> tuple[bool, ac.bits[65], Nested]:
        return flag, wide, nested

    advance()
    result_flag, result_wide, result_nested = value()
    return result_flag, result_wide, result_nested


@ac.module
def GuardedCell(enable: ac.u1) -> ac.u8:
    count: ac.u8 = 0

    @ac.rule(guard=lambda: enable)
    def advance() -> None:
        count = count + 1

    @ac.rule
    def value() -> ac.u8:
        return count

    advance()
    return value()


@ac.module
def WholeCell() -> Pair:
    pair: Pair = Pair(left=0, right=0)

    @ac.rule
    def advance() -> None:
        pair = Pair(left=pair.left + 1, right=pair.right + 1)

    @ac.rule
    def value() -> Pair:
        return pair

    advance()
    return value()


@ac.module
def PartialCell(enable: ac.u1) -> Pair:
    pair: Pair = Pair(left=0, right=0)

    @ac.rule
    def advance() -> None:
        pair.left = pair.left + 1
        if enable:
            pair.right = pair.right + 2

    @ac.rule
    def value() -> Pair:
        return pair

    advance()
    return value()
"""


BENCH = """import pycircuit as ac
from checks.storage_provider import (
    FreeCell,
    GatedCell,
    GuardedCell,
    PartialCell,
    TypedCell,
    WholeCell,
)


@ac.system
def ExerciseStorage():
    enable: ac.u1 = 0
    free = FreeCell()
    gated = GatedCell(enable)
    partial = PartialCell(enable)
    whole = WholeCell()
    guarded = GuardedCell(enable)
    flag, wide, nested = TypedCell()

    @ac.rule
    def observe() -> None:
        ac.log(
            "info",
            "storage",
            free,
            gated,
            partial.left,
            partial.right,
            whole.left,
            guarded,
            flag,
            wide[:64],
            nested.inner.value,
            ac.enum_to_bits(nested.tag),
            enable,
        )

    @ac.rule
    def tick() -> None:
        enable = ~enable

    observe()
    tick()
"""


def _sources(tmp_path: Path) -> tuple[Path, Path, Path]:
    source = tmp_path / "storage selection sources"
    source.mkdir()
    (source / "storage_provider.py").write_text(PROVIDER)
    (source / "storage_bench.py").write_text(BENCH)
    provider, bench = tmp_path / "provider unit", tmp_path / "bench unit"
    _ok(_compile(source, "storage_provider.py", provider))
    _ok(_compile(source, "storage_bench.py", bench, [provider]))
    final = tmp_path / "storage selection.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(bench),
            "--top",
            "checks.storage_bench.ExerciseStorage",
            "-o",
            str(final),
        )
    )
    return source, provider, final


def _module_block(text: str, suffix: str) -> str:
    for match in re.finditer(r"module\s+(\w+)\s*\(.*?endmodule", text, re.S):
        if match.group(1).endswith(suffix):
            return match.group(0)
    raise AssertionError(f"missing generated module ending in {suffix}")


def test_plain_dff_and_guarded_dffe_execute_from_one_final(tmp_path):
    _, _, final = _sources(tmp_path)
    expected = []
    free = gated = left = right = whole = guarded = 0
    flag, wide, inner, tag = 0, 0, 0, 0
    for cycle in range(6):
        enable = cycle % 2
        expected.extend(
            [
                (
                    free,
                    gated,
                    left,
                    right,
                    whole,
                    guarded,
                    flag,
                    wide,
                    inner,
                    tag,
                    enable,
                )
            ]
            * 2
        )
        free = (free + 1) & 255
        left = (left + 1) & 255
        whole = (whole + 1) & 255
        flag ^= 1
        wide += 1
        inner = (inner + 1) & 255
        tag = 1 if tag == 0 else 0
        if enable:
            gated = (gated + 1) & 255
            guarded = (guarded + 1) & 255
            right = (right + 2) & 255

    structure = tmp_path / "structure"
    _ok(_cli("emit", str(final), "--target", "verilog", "-o", str(structure)))
    rtl = next((structure / "sources").glob("**/storage_provider.v"))
    text = rtl.read_text()
    assert " dff #(" in _module_block(text, "FreeCell")
    assert " dffe #(" not in _module_block(text, "FreeCell")
    assert " dffe #(" in _module_block(text, "GatedCell")
    assert " dff #(" in _module_block(text, "WholeCell")
    assert " dffe #(" in _module_block(text, "PartialCell")
    guarded = _module_block(text, "GuardedCell")
    assert " dffe #(" in guarded
    assert " dff #(" not in guarded
    typed = _module_block(text, "TypedCell")
    assert typed.count(" dff #(") == 3
    assert " dffe #(" not in typed

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
            if row["kind"] == "log" and row["spec"]["event"] == "storage"
        ]
        assert [
            tuple(int(value["value"]) for value in row["values"]) for row in events
        ] == expected
        transcripts.append([row for row in rows if row["kind"] != "result"])
    verilog_root = tmp_path / "verilog"
    verilog_root.mkdir()
    verilog_binary = _bundle(final, verilog_root, "verilog")
    rows = _records(_ok(_run([str(verilog_binary), "+cycles=6"], timeout=60)))
    events = [
        row
        for row in rows
        if row["kind"] == "log" and row["spec"]["event"] == "storage"
    ]
    assert [
        tuple(int(value["value"]) for value in row["values"]) for row in events
    ] == expected
    transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]


ORDERED_WRITER_BENCH = """import pycircuit as ac


@ac.system
def ExerciseOrderedStorageWriters():
    count: ac.u8 = 5
    guarded_tail: ac.u8 = 5
    guarded_head: ac.u8 = 5
    old_count = count
    old_guarded_tail = guarded_tail
    old_guarded_head = guarded_head

    @ac.rule
    def unconditional_first() -> None:
        count = count + 1

    @ac.rule(after=(unconditional_first,))
    def unconditional_second() -> None:
        count = count * 2

    @ac.rule
    def guarded_tail_first() -> None:
        guarded_tail = guarded_tail + 1

    @ac.rule(after=(guarded_tail_first,), guard=lambda: guarded_tail < 10)
    def guarded_tail_second() -> None:
        guarded_tail = guarded_tail * 2

    @ac.rule(guard=lambda: guarded_head < 10)
    def guarded_head_first() -> None:
        guarded_head = guarded_head + 1

    @ac.rule(after=(guarded_head_first,))
    def guarded_head_second() -> None:
        guarded_head = guarded_head * 2

    @ac.rule(
        after=(unconditional_second, guarded_tail_second, guarded_head_second)
    )
    def observe() -> None:
        ac.log(
            "info",
            "ordered storage writers",
            old_count,
            count,
            old_guarded_tail,
            guarded_tail,
            old_guarded_head,
            guarded_head,
        )

    unconditional_first()
    unconditional_second()
    guarded_tail_first()
    guarded_tail_second()
    guarded_head_first()
    guarded_head_second()
    observe()
"""


def test_ordered_multi_writer_storage_executes_from_one_final(tmp_path):
    source = tmp_path / "ordered storage writer sources"
    source.mkdir()
    (source / "ordered_storage_bench.py").write_text(ORDERED_WRITER_BENCH)
    unit = tmp_path / "ordered storage writer unit"
    _ok(_compile(source, "ordered_storage_bench.py", unit))
    final = tmp_path / "ordered storage writers.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.ordered_storage_bench.ExerciseOrderedStorageWriters",
            "-o",
            str(final),
        )
    )

    expected = [
        (5, 12, 5, 12, 5, 12),
        (12, 26, 12, 13, 12, 24),
        (26, 54, 13, 14, 24, 48),
        (54, 110, 14, 15, 48, 96),
    ]
    expected = [event for event in expected for _ in range(2)]

    transcripts = []
    for target, workers in (("cpp", 1), ("cpp", 2), ("verilog", 1)):
        target_root = tmp_path / f"ordered-{target}-{workers}"
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
            if row["kind"] == "log"
            and row["spec"]["event"] == "ordered storage writers"
        ]
        assert [
            tuple(int(value["value"]) for value in row["values"]) for row in events
        ] == expected
        transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]


BOOLEAN_OR_STORAGE_BENCH = """import pycircuit as ac


@ac.system
def ExerciseBooleanOrStorage():
    right_true: ac.u8 = 0
    left_true: ac.u8 = 0
    right_false: ac.u8 = 0

    @ac.rule
    def advance_right_true() -> None:
        if (right_true == 0) | True:
            right_true = right_true + 1

    @ac.rule
    def advance_left_true() -> None:
        if True | (left_true == 0):
            left_true = left_true + 1

    @ac.rule
    def advance_right_false() -> None:
        if (right_false == 0) | False:
            right_false = right_false + 1

    @ac.rule
    def observe() -> None:
        ac.log(
            "info",
            "boolean or storage",
            right_true,
            left_true,
            right_false,
        )

    advance_right_true()
    advance_left_true()
    advance_right_false()
    observe()
"""


def test_boolean_or_constant_storage_selection_executes_from_one_final(tmp_path):
    source = tmp_path / "boolean or storage sources"
    source.mkdir()
    (source / "boolean_or_storage_bench.py").write_text(BOOLEAN_OR_STORAGE_BENCH)
    unit = tmp_path / "boolean or storage unit"
    _ok(_compile(source, "boolean_or_storage_bench.py", unit))
    final = tmp_path / "boolean or storage.ac"
    _ok(
        _cli(
            "link",
            str(unit),
            "--top",
            "checks.boolean_or_storage_bench.ExerciseBooleanOrStorage",
            "-o",
            str(final),
        )
    )

    expected = [
        event
        for event in ((0, 0, 0), (1, 1, 1), (2, 2, 1), (3, 3, 1))
        for _ in range(2)
    ]
    transcripts = []
    for target, workers in (("cpp", 1), ("cpp", 2), ("verilog", 1)):
        target_root = tmp_path / f"boolean-or-{target}-{workers}"
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
            if row["kind"] == "log" and row["spec"]["event"] == "boolean or storage"
        ]
        assert [
            tuple(int(value["value"]) for value in row["values"]) for row in events
        ] == expected
        transcripts.append([row for row in rows if row["kind"] != "result"])
    assert transcripts[0] == transcripts[1] == transcripts[2]


PROBE_BENCH = """import pycircuit as ac
from checks.storage_provider import FreeCell


@ac.system
def ExerciseDffProbe():
    fail: ac.u1 = 0
    cell = FreeCell()

    @ac.rule
    def check() -> None:
        assert fail == 0, "deliberate dff failure"

    @ac.rule
    def tick() -> None:
        fail = fail + 1

    check()
    tick()
"""


def test_plain_dff_zero_commit_and_unknown_payload(tmp_path):
    source = tmp_path / "dff probe sources"
    source.mkdir()
    (source / "storage_provider.py").write_text(PROVIDER)
    (source / "probe_bench.py").write_text(PROBE_BENCH)
    provider, bench = tmp_path / "probe provider", tmp_path / "probe bench"
    _ok(_compile(source, "storage_provider.py", provider))
    _ok(_compile(source, "probe_bench.py", bench, [provider]))
    final = tmp_path / "dff probe.ac"
    _ok(
        _cli(
            "link",
            str(provider),
            str(bench),
            "--top",
            "checks.probe_bench.ExerciseDffProbe",
            "-o",
            str(final),
        )
    )
    generated = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(generated)))
    for header in generated.rglob("*.hpp"):
        header.write_text(header.read_text().replace("private:", "public:"))
    (generated / "dff_probe.cpp").write_text(
        r"""
#include "pycircuit_system.hpp"
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <vector>
using Bit = gfsim::wire<gfsim::Bits<1>>;
static Bit bit(unsigned value) {
  if (value < 2) return Bit::known(gfsim::Bits<1>{value});
  return Bit::fromPacked(value == 2 ? gfsim::FourState<1>::unknown()
                                  : gfsim::FourState<1>::highImpedance());
}
static void require(bool value, const char *message) {
  if (!value) throw std::runtime_error(message);
}
template<class Storage, class Output, class Wire>
void inject(Storage &storage, Output &output, Wire value) {
  using Current = std::remove_cvref_t<decltype(storage->current(0))>;
  auto &state = const_cast<Current &>(storage->current(0));
  state.q = value;
  output.element(0) = value;
}
int main() {
  for (unsigned workers : {1u, 2u}) {
    pyc_dut dut(workers);
    auto drive = [&](bool clock) { dut.drive({bit(clock), bit(0)}); };
    drive(false); dut.system().Build();
    auto root = dut.root_->pyc_implementation;
    auto cell = root->pyc_instance_pyc_63656c6c;
    auto count = [&]() {
      const auto &q = cell->pyc_instance_count_state->current(0);
      return std::vector<std::uint64_t>{q.q.packed().value().value(),
        q.q.packed().knownMask().value(), q.q.packed().zMask().value()};
    };
    auto start = [&]() {
      drive(false); dut.system().Reset();
      require(dut.system().cycle() == 0, "reset failed");
    };
    start();
    drive(true); require(dut.system().Step() == gfsim::SimStepResult::Running,
                         "first commit failed");
    require(count() == std::vector<std::uint64_t>{1, 255, 0}, "first count");
    const auto before = count();
    drive(false); drive(true);
    require(dut.system().Step() == gfsim::SimStepResult::Failed,
            "guard failure did not fail the system");
    require(count() == before, "failed epoch changed DFF storage");
    require(dut.system().Step() == gfsim::SimStepResult::Failed, "failure is not sticky");
    require(count() == before, "sticky failure changed DFF storage");
    start();
    auto unknown = gfsim::wire<gfsim::Bits<8>>::fromPacked(
        gfsim::FourState<8>::unknown());
    inject(cell->pyc_instance_count_state, cell->pyc_instance_count_q, unknown);
    drive(true);
    require(dut.system().Step() == gfsim::SimStepResult::Running,
            "unknown DFF payload must commit as data");
    require(count() == std::vector<std::uint64_t>{0, 0, 0} &&
                cell->pyc_instance_count_q.element(0).packed().knownMask().value() == 0 &&
                cell->pyc_instance_count_q.element(0).packed().zMask().value() == 0,
            "DFF did not preserve unknown payload");
  }
  std::cout << "DFF_ZERO_COMMIT_AND_UNKNOWN_OK\n";
}
"""
    )
    with (generated / "CMakeLists.txt").open("a") as cmake:
        cmake.write(
            "\nadd_executable(dff_probe dff_probe.cpp)\n"
            "target_compile_features(dff_probe PRIVATE cxx_std_20)\n"
            "target_link_libraries(dff_probe PRIVATE pycircuit_modules)\n"
        )
    build = tmp_path / "probe build"
    _ok(
        _run(
            [
                "cmake",
                "-S",
                str(generated),
                "-B",
                str(build),
                "-G",
                "Ninja",
                f"-Dpycircuit_DIR={Path(__import__('os').environ['PYC_TOOLCHAIN_ROOT']) / 'share/pycircuit/cmake'}",
            ]
        )
    )
    _ok(
        _run(
            [
                "cmake",
                "--build",
                str(build),
                "--target",
                "dff_probe",
                "--parallel",
                "2",
            ]
        )
    )
    assert _ok(_run([str(build / "dff_probe")], timeout=30)).strip() == (
        "DFF_ZERO_COMMIT_AND_UNKNOWN_OK"
    )

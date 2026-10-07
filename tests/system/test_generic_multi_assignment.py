"""Generic rules keep source-order current/next semantics across assignments.

This regression compiles each Python file independently, links the saved final
program, then asks a fresh process to emit both backends. The child rule writes
its owned ``count`` next from ``incoming`` and then writes the parent-owned
``outgoing`` next from ``count``. The source contract requires that second read to observe the
cycle-start current value, so the observable trace is reset, 0, incoming.
"""

from __future__ import annotations

import json
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

ROOT = Path(__file__).resolve().parents[2]
GFSIM_INCLUDE = ROOT / "include"

TYPES = """\
from typing import Annotated

Word = Annotated[int, range(256)]
Phase = Annotated[int, range(8)]
"""

CHILD = """\
from pycircuit import module, rule
from .types import Word

@module
def Counter(incoming: Word, outgoing: Word):
    count: Word = 0

    @rule
    def tick():
        nonlocal count, outgoing
        count = incoming
        outgoing = count

    tick()
"""

SYSTEM = """\
from pycircuit import system, rule, log, report
from .types import Word, Phase
from .counter import Counter

@system
def TestCounters():
    left_input: Word = 3
    right_input: Word = 10
    left_output: Word = 100
    right_output: Word = 200
    phase: Phase = 0
    left = Counter(left_input, left_output)
    right = Counter(right_input, right_output)

    @rule
    def fixture():
        nonlocal phase
        if phase == 0:
            assert left_output == 100 and right_output == 200, "reset values"
        elif phase == 1:
            assert left_output == 0 and right_output == 0, "same-rule current"
            log("info", "left", left_output)
            log("info", "right", right_output)
        elif phase == 2:
            assert left_output == 3 and right_output == 10, "next commit"
            log("info", "left", left_output)
            log("info", "right", right_output)
            report("completed", 1)
        if phase < 3:
            phase = phase + 1

    fixture()
"""

LOCAL_CANDIDATE_CHILD = CHILD.replace(
    "        nonlocal count, outgoing\n        count = incoming\n        outgoing = count",
    "        nonlocal count, outgoing\n        candidate = incoming\n        count = candidate\n        outgoing = candidate",
)
LOCAL_CANDIDATE_SYSTEM = SYSTEM.replace(
    'left_output == 0 and right_output == 0, "same-rule current"',
    'left_output == 3 and right_output == 10, "local candidate reuse"',
)
ENABLED_CHILD = CHILD.replace(
    "def Counter(incoming: Word, outgoing: Word):",
    "def Counter(incoming: Word, outgoing: Word, enabled: bool):",
).replace(
    "        outgoing = count",
    "        if enabled:\n            outgoing = count",
)
ENABLED_SYSTEM = (
    SYSTEM.replace(
        "    phase: Phase = 0\n    left = Counter(left_input, left_output)\n    right = Counter(right_input, right_output)",
        "    phase: Phase = 0\n    left_enable: bool = True\n    right_enable: bool = False\n"
        "    left = Counter(left_input, left_output, left_enable)\n"
        "    right = Counter(right_input, right_output, right_enable)",
    )
    .replace(
        'left_output == 0 and right_output == 0, "same-rule current"',
        'left_output == 0 and right_output == 200, "independent enables"',
    )
    .replace(
        'left_output == 3 and right_output == 10, "next commit"',
        'left_output == 3 and right_output == 200, "independent enable holds"',
    )
)


@dataclass(frozen=True)
class SourceUnit:
    source: Path
    body: Path
    header: Path


def _tool(env_name: str, name: str) -> str:
    configured = os.environ.get(env_name)
    if configured:
        return configured
    found = shutil.which(name)
    if not found:
        raise AssertionError(f"set {env_name} or put {name} on PATH")
    return found


def _source_parts_harness() -> str:
    return _tool("PYCIRCUIT_EMITTER", "pycircuit-emit")


def _compile(
    source: Path, source_root: Path, output: Path, headers: tuple[Path, ...] = ()
) -> SourceUnit:
    output.mkdir(parents=True, exist_ok=True)
    capture = _capture_source_file(source, source_root=source_root)
    transport = output / f"{source.stem}.transport.mlir"
    body = output / f"{source.stem}.body.mlir"
    header = output / f"{source.stem}.interface.mlir"
    transport.write_text(_emit_source_transport(capture), encoding="utf-8")
    command = [
        _tool("PYCIRCUIT_SOURCE_COMPILER", "pycircuit-source-unit"),
        "--capture",
        str(transport),
        "--package",
        "demo",
        "--path",
        source.relative_to(source_root).as_posix(),
    ]
    for dependency in headers:
        command.extend(("--header", str(dependency)))
    command.extend(("--body-out", str(body), "--interface-out", str(header)))
    completed = subprocess.run(command, text=True, capture_output=True, check=False)
    assert completed.returncode == 0, completed.stderr
    assert body.is_file() and header.is_file()
    return SourceUnit(source, body, header)


def _link(units: list[SourceUnit], final: Path) -> subprocess.CompletedProcess[str]:
    command = [_tool("PYCIRCUIT_LINKER", "pycircuit-link")]
    for unit in units:
        command.extend(("--body", str(unit.body), "--header", str(unit.header)))
    command.extend(
        (
            "--top",
            "demo.test_counters.TestCounters",
            "--target",
            "final",
            "--role",
            "testbench",
            "--output",
            str(final),
        )
    )
    return subprocess.run(command, text=True, capture_output=True, check=False)


def _emit(final: Path, target: str, output: Path) -> subprocess.CompletedProcess[str]:
    # This subprocess reparses the persisted final program; no in-memory linked
    # module is passed from the test process.
    return subprocess.run(
        [
            _tool("PYCIRCUIT_LINKER", "pycircuit-link"),
            "--design",
            str(final),
            "--target",
            target,
            "--role",
            "testbench",
            "--output",
            str(output),
        ],
        text=True,
        capture_output=True,
        check=False,
    )


def _build_final(
    tmp_path: Path, child_source: str = CHILD, system_source: str = SYSTEM
) -> Path:
    source_root = tmp_path / "source"
    source_root.mkdir()
    (source_root / "types.py").write_text(TYPES, encoding="utf-8")
    (source_root / "counter.py").write_text(child_source, encoding="utf-8")
    (source_root / "test_counters.py").write_text(system_source, encoding="utf-8")
    types = _compile(source_root / "types.py", source_root, tmp_path / "units/types")
    counter = _compile(
        source_root / "counter.py",
        source_root,
        tmp_path / "units/counter",
        (types.header,),
    )
    system = _compile(
        source_root / "test_counters.py",
        source_root,
        tmp_path / "units/test_counters",
        (types.header, counter.header),
    )
    final = tmp_path / "test_counters.final.ac"
    linked = _link([types, counter, system], final)
    assert linked.returncode == 0, linked.stderr
    assert final.is_file()
    return final


def _run_cpp(tmp_path: Path, model: Path) -> list[int]:
    driver = tmp_path / "driver.cpp"
    driver.write_text(
        """\
#include \"gfsim/SimSystem.h\"
#include \"model.hpp\"
#include <iostream>
int main() {
  FinalSystem model;
  model.Build();
  for (int run = 0; run != 2; ++run) {
    model.Reset();
    for (int i = 0; i != 4; ++i) {
      auto step = model.Step();
      for (const auto &event : model.Observations().Events())
        std::cout << event.value.bits << \"\\n\";
      if (step == gfsim::SimStepResult::Failed ||
          step == gfsim::SimStepResult::InvalidState) return 5;
      if (step == gfsim::SimStepResult::Quiescent) break;
    }
  }
}
""",
        encoding="utf-8",
    )
    shutil.copy2(model, tmp_path / "model.hpp")
    binary = tmp_path / "model"
    compiled = subprocess.run(
        [
            _tool("CXX", "clang++"),
            "-std=c++20",
            "-I",
            str(GFSIM_INCLUDE),
            str(driver),
            "-o",
            str(binary),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert compiled.returncode == 0, compiled.stderr
    ran = subprocess.run([str(binary)], text=True, capture_output=True, check=False)
    assert ran.returncode == 0, ran.stderr
    return [int(line) for line in ran.stdout.splitlines()]


def _run_verilog(tmp_path: Path, model: Path) -> list[int]:
    testbench = tmp_path / "tb.sv"
    testbench.write_text(
        """\
module tb;
  logic clk = 1'b0;
  logic reset = 1'b1;
  FinalModelSim dut(.clk(clk), .reset(reset));
  task automatic tick;
    begin #1 clk = 1'b1; #1 clk = 1'b0; #1; end
  endtask
  initial begin
    tick(); reset = 1'b0; repeat (5) tick();
    reset = 1'b1; tick(); reset = 1'b0; repeat (5) tick();
    $finish;
  end
endmodule
""",
        encoding="utf-8",
    )
    binary = tmp_path / "rtl.vvp"
    compiled = subprocess.run(
        [
            _tool("IVERILOG", "iverilog"),
            "-g2012",
            "-s",
            "tb",
            "-o",
            str(binary),
            str(model),
            str(testbench),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert compiled.returncode == 0, compiled.stderr
    ran = subprocess.run(
        [_tool("VVP", "vvp"), str(binary)], text=True, capture_output=True, check=False
    )
    assert ran.returncode == 0, ran.stderr
    return [
        int(fields[5])
        for line in ran.stdout.splitlines()
        if len(fields := line.split()) == 8 and fields[0] == "AC_OBS"
    ]


def _run_grouped_cpp(tmp_path: Path, final: Path) -> str:
    emitted = subprocess.run(
        [_source_parts_harness(), str(final)],
        text=True,
        capture_output=True,
        check=False,
    )
    assert emitted.returncode == 0, emitted.stderr
    payload = json.loads(emitted.stdout)
    assert set(payload) == {"support_header", "system_header", "source_groups"}
    output = tmp_path / "grouped"
    output.mkdir()
    (output / "pycircuit_support.hpp").write_text(
        payload["support_header"], encoding="utf-8"
    )
    (output / "pycircuit_system.hpp").write_text(
        payload["system_header"], encoding="utf-8"
    )
    groups = payload["source_groups"]
    owners = [(group["source"]["package"], group["source"]["path"]) for group in groups]
    assert sorted(owners) == [
        ("demo", "counter.py"),
        ("demo", "test_counters.py"),
        ("demo", "types.py"),
    ]
    declaration_group = next(
        group for group in groups if group["source"]["path"] == "types.py"
    )
    assert declaration_group["source_path"] is None
    assert declaration_group["implementation"] is None
    child_group = next(
        group for group in groups if group["source"]["path"] == "counter.py"
    )
    assert len(re.findall(r"SimDFFE<[^>]+>\s+q_count_", child_group["header"])) == 1
    assert "q_outgoing_" not in child_group["header"]
    for group in groups:
        for key in ("header_path", "source_path"):
            if group[key] is None:
                assert key == "source_path" and group["implementation"] is None
                continue
            relative = Path(group[key])
            assert not relative.is_absolute() and ".." not in relative.parts
        for key, field in (
            ("header_path", "header"),
            ("source_path", "implementation"),
        ):
            if group[key] is None:
                assert field == "implementation" and group[field] is None
                continue
            path = output / group[key]
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(group[field], encoding="utf-8")
    includes = ["-I", str(output), "-I", str(GFSIM_INCLUDE)]
    cxx = _tool("CXX", "clang++")
    objects = []
    for index, group in enumerate(groups):
        if group["source_path"] is None:
            assert group["implementation"] is None
            continue
        source = output / group["source_path"]
        obj = tmp_path / f"group-{index}.o"
        compiled = subprocess.run(
            [cxx, "-std=c++20", *includes, "-c", str(source), "-o", str(obj)],
            text=True,
            capture_output=True,
            check=False,
        )
        assert compiled.returncode == 0, compiled.stderr
        objects.append(obj)
    driver = tmp_path / "grouped.cpp"
    driver.write_text(
        """\
#include \"pycircuit_system.hpp\"
#include <iostream>
static int run(FinalSystem &model, const char *tag) {
  model.Reset();
  for (int i = 0; i != 5; ++i) {
    auto step = model.Step();
    for (const auto &event : model.Observations().Events())
      std::cout << tag << \" EV \" << event.value.bits << \"\\n\";
    if (step == gfsim::SimStepResult::Failed ||
        step == gfsim::SimStepResult::InvalidState) return 5;
    if (step == gfsim::SimStepResult::Quiescent) break;
  }
  return 0;
}
int main() {
  FinalSystem model; model.Build();
  if (int rc = run(model, \"A\")) return rc;
  if (int rc = run(model, \"B\")) return rc;
}
""",
        encoding="utf-8",
    )
    binary = tmp_path / "grouped-model"
    compiled = subprocess.run(
        [
            cxx,
            "-std=c++20",
            *includes,
            str(driver),
            *(str(obj) for obj in objects),
            "-o",
            str(binary),
        ],
        text=True,
        capture_output=True,
        check=False,
    )
    assert compiled.returncode == 0, compiled.stderr
    ran = subprocess.run([str(binary)], text=True, capture_output=True, check=False)
    assert ran.returncode == 0, ran.stderr
    return ran.stdout


def _cpp_trace(stdout: str, tag: str) -> list[int]:
    return [
        int(match.group(1))
        for match in re.finditer(rf"^{tag} EV (\d+)$", stdout, re.MULTILINE)
    ]


def _balanced_close(text: str, start: int, opening: str, closing: str) -> int:
    depth = 0
    quoted = False
    escaped = False
    for index in range(start, len(text)):
        char = text[index]
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
            continue
        if char == '"':
            quoted = True
        elif char == opening:
            depth += 1
        elif char == closing:
            depth -= 1
            if depth == 0:
                return index
    raise AssertionError(f"unclosed MLIR attribute beginning at {start}")


def _array_elements(array_text: str) -> list[str]:
    elements = []
    start = 1
    depth = 0
    quoted = False
    escaped = False
    for index, char in enumerate(array_text[1:-1], 1):
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
            continue
        if char == '"':
            quoted = True
        elif char in "{[(":
            depth += 1
        elif char in "}])":
            depth -= 1
        elif char == "," and depth == 0:
            elements.append(array_text[start:index].strip())
            start = index + 1
    tail = array_text[start:-1].strip()
    if tail:
        elements.append(tail)
    return elements


def _multi_use_array(text: str) -> tuple[int, int, list[str]]:
    for match in re.finditer(r"ac.required_uses\s*=\s*\[", text):
        start = text.find("[", match.start())
        end = _balanced_close(text, start, "[", "]")
        array = text[start : end + 1]
        elements = _array_elements(array)
        if len(elements) >= 2 and all('role = "next"' in item for item in elements[:2]):
            return start, end + 1, elements
    raise AssertionError("final has no rule with two generic next-use proofs")


def _nested_field(record: str, field: str) -> str:
    match = re.search(rf"\b{field}\s*=\s*\{{", record)
    assert match, f"missing {field} in {record}"
    start = record.find("{", match.start())
    end = _balanced_close(record, start, "{", "}")
    return record[start : end + 1]


def _corrupt_second_target(text: str, *, duplicate_id: bool) -> str:
    start, end, uses = _multi_use_array(text)
    first, second = uses[0], uses[1]
    if duplicate_id:
        replacement = _nested_field(first, "id")
        old = _nested_field(second, "id")
    else:
        replacement = _nested_field(first, "target")
        old = _nested_field(second, "target")
    assert old != replacement
    mutated_second = second.replace(old, replacement, 1)
    assert mutated_second != second
    updated = text[start:end].replace(second, mutated_second, 1)
    return text[:start] + updated + text[end:]


@pytest.mark.parametrize(
    ("child_source", "system_source", "expected_one_run", "serialized_regs"),
    [
        pytest.param(CHILD, SYSTEM, [0, 0, 3, 10], 6, id="persistent-current-next"),
        pytest.param(
            LOCAL_CANDIDATE_CHILD,
            LOCAL_CANDIDATE_SYSTEM,
            [3, 10, 3, 10],
            6,
            id="local-candidate-reuse",
        ),
        pytest.param(
            ENABLED_CHILD,
            ENABLED_SYSTEM,
            [0, 200, 3, 200],
            8,
            id="independent-yield-enables",
        ),
    ],
)
def test_multi_assignment_saved_final_cpp_and_rtl(
    tmp_path: Path,
    child_source: str,
    system_source: str,
    expected_one_run: list[int],
    serialized_regs: int,
) -> None:
    final = _build_final(tmp_path, child_source, system_source)
    # Five parent registers plus one owned count per child. Aliased output
    # connections must not create relay state; repeated child definitions must
    # retain separate count state and output targets.
    assert final.read_text(encoding="utf-8").count('"ac.reg"') == serialized_regs

    cpp = tmp_path / "model.generated.hpp"
    rtl = tmp_path / "model.sv"
    cpp_emitted = _emit(final, "cpp", cpp)
    assert cpp_emitted.returncode == 0, cpp_emitted.stderr
    rtl_emitted = _emit(final, "verilog", rtl)
    assert rtl_emitted.returncode == 0, rtl_emitted.stderr
    assert _run_cpp(tmp_path, cpp) == expected_one_run * 2
    grouped_out = _run_grouped_cpp(tmp_path, final)
    assert _cpp_trace(grouped_out, "A") == expected_one_run
    assert _cpp_trace(grouped_out, "B") == expected_one_run

    # RTL strobes both log events and the phase-two report gauge per run.
    per_run = expected_one_run + [1]
    rtl_values = _run_verilog(tmp_path, rtl)
    assert rtl_values == per_run * 2, rtl_values


@pytest.mark.parametrize(
    ("label", "duplicate_id"),
    [("redirect-second-target", False), ("duplicate-second-use-id", True)],
)
def test_multi_assignment_use_proofs_fail_closed(
    tmp_path: Path, label: str, duplicate_id: bool
) -> None:
    """A verified multi-use final must reject redirected or duplicate proofs."""
    final = _build_final(tmp_path)
    text = final.read_text(encoding="utf-8")
    mutated = _corrupt_second_target(text, duplicate_id=duplicate_id)
    assert mutated != text
    candidate = tmp_path / f"{label}.final.ac"
    candidate.write_text(mutated, encoding="utf-8")

    output = tmp_path / f"{label}.cpp"
    result = _emit(candidate, "cpp", output)
    assert result.returncode == 1, f"{label}: {result.returncode}: {result.stderr}"
    expected_diagnostic = (
        "generic final RequiredUse is malformed"
        if duplicate_id
        else "generic final YieldBinding is stale or redirected"
    )
    assert expected_diagnostic in result.stderr, result.stderr
    assert not output.exists(), "rejected final must not publish partial output"

    rtl_output = tmp_path / f"{label}.sv"
    rtl_result = _emit(candidate, "verilog", rtl_output)
    assert (
        rtl_result.returncode == 1
    ), f"{label}: {rtl_result.returncode}: {rtl_result.stderr}"
    assert expected_diagnostic in rtl_result.stderr, rtl_result.stderr
    assert not rtl_output.exists(), "rejected final must not publish partial output"

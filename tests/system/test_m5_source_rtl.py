"""Source-owned RTL groups are emitted from one saved final program."""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from pathlib import Path
from typing import Any

import pytest
import test_cpp_source_parts as cpp
import test_m4_preview_workflow as preview
import test_m5_public_emit as public

pytestmark = pytest.mark.system

ROOT = Path(__file__).resolve().parents[2]
_RTL_TB = r"""module tb;
  logic clk = 0;
  logic reset = 1;
  FinalModel dut(.clk(clk), .reset(reset));
  initial begin
    #5; clk = 1; #5;
    if (dut.q0 != 3 || dut.q1 != 10 || dut.q2 != 100 || dut.q3 != 200)
      $fatal(1, "reset image differs");
    clk = 0; reset = 0; #5; clk = 1; #5;
    if (dut.q2 != 0 || dut.q3 != 0)
      $fatal(1, "first source-owned child transfer differs");
    clk = 0; #5; clk = 1; #5;
    if (dut.q2 != 3 || dut.q3 != 10)
      $fatal(1, "repeated child instances did not retain distinct state");
    $finish;
  end
endmodule
"""


def _emit_from_final(final: Path, helper: Path) -> dict[str, Any]:
    completed = subprocess.run(
        [str(helper), str(final), "--runner"],
        text=True,
        capture_output=True,
        check=False,
        timeout=180,
    )
    assert completed.returncode == 0, completed.stderr
    return json.loads(completed.stdout)


def _build_and_run_generated_target(
    generated: Path, target: str, output: Path
) -> bytes:
    install = Path(
        os.environ.get(
            "PYCIRCUIT_COMPILER_INSTALL", ROOT / ".pycircuit_out/m5-candidate-install"
        )
    ).resolve()
    assert (install / "share/pycircuit/cmake/pycircuitConfig.cmake").is_file(), install
    build = output / f"build-{target}"
    configured = subprocess.run(
        [
            "cmake",
            "-S",
            str(generated),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_PREFIX_PATH=" + str(install),
        ],
        text=True,
        capture_output=True,
        check=False,
        timeout=180,
    )
    assert configured.returncode == 0, f"{configured.stdout}\n{configured.stderr}"
    compiled = subprocess.run(
        ["cmake", "--build", str(build), "--parallel", "4"],
        text=True,
        capture_output=True,
        check=False,
        timeout=900,
    )
    assert compiled.returncode == 0, f"{compiled.stdout}\n{compiled.stderr}"
    runner = build / "pycircuit_system"
    if os.name == "nt":
        runner = runner.with_suffix(".exe")
    events = output / f"{target}-events.jsonl"
    run = subprocess.run(
        [
            str(runner),
            "--config",
            str(public.FIXTURE / "configs/three-ticks.json"),
            "--events",
            str(events),
        ],
        text=True,
        capture_output=True,
        check=False,
        timeout=120,
    )
    assert run.returncode == 0, f"{target}: {run.stdout}\n{run.stderr}"
    return events.read_bytes()


def _verilate_groups(
    payload: dict[str, Any], output: Path
) -> subprocess.CompletedProcess[str]:
    verilator = shutil.which("verilator")
    if not verilator:
        pytest.fail("M5 source RTL integration requires Verilator")
    source_paths: list[Path] = []
    for group in payload["rtl_source_groups"]:
        relative = Path(group["path"])
        assert not relative.is_absolute() and ".." not in relative.parts
        path = output / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(group["text"], encoding="utf-8")
        source_paths.append(path)
    core = output / "core.sv"
    core.write_text(payload["rtl_core"], encoding="utf-8")
    testbench = output / "tb.sv"
    testbench.write_text(_RTL_TB, encoding="utf-8")
    build = output / "obj"
    command = [
        verilator,
        "--binary",
        "--timing",
        "-Wno-fatal",
        "--top-module",
        "tb",
        "--Mdir",
        str(build),
        *(str(path) for path in source_paths),
        str(core),
        str(testbench),
    ]
    compiled = subprocess.run(
        command, text=True, capture_output=True, check=False, timeout=300
    )
    assert compiled.returncode == 0, (
        f"Verilator failed ({compiled.returncode}): {command!r}\n"
        f"{compiled.stdout}\n{compiled.stderr}"
    )
    executable = build / "Vtb"
    return subprocess.run(
        [str(executable)], text=True, capture_output=True, check=False, timeout=60
    )


def test_source_compile_link_emits_genuine_rtl_owner_files_and_matching_behavior(
    tmp_path: Path,
) -> None:
    native = preview._require_preview_tools()
    build = preview._build_preview(tmp_path / "source-design", native=native)
    final = build / "linked/design_top.ac"
    final_hash = preview._sha256(final)

    # Ask a fresh process to recover every owner from the final program alone.
    payload = _emit_from_final(final, native / "bin/acir-cpp-source-parts-harness")
    groups = payload["rtl_source_groups"]
    assert [
        (group["source"]["package"], group["source"]["path"]) for group in groups
    ] == [
        ("preview", "counter.py"),
        ("preview", "design_top.py"),
    ]
    assert [group["path"] for group in groups] == [
        "sources/preview/counter.sv",
        "sources/preview/design_top.sv",
    ]
    assert [group["module"] for group in groups] == [
        "ac_preview_counter",
        "ac_preview_design_top",
    ]
    assert "module ac_preview_counter" in groups[0]["text"]
    assert "module ac_preview_design_top" in groups[1]["text"]
    root_instances = re.findall(
        r"\bac_preview_counter\s+(child_\d+)", groups[1]["text"]
    )
    assert len(root_instances) == 2 and len(set(root_instances)) == 2
    # types.py is declaration-only. It has no executable RTL owner group.
    assert all(group["source"]["path"] != "types.py" for group in groups)
    assert "module FinalModel" in payload["rtl_core"]
    assert all(
        re.search(rf"\bmodule\s+{re.escape(group['module'])}\b", group["text"])
        for group in groups
    )

    cpp_events = build / "cpp-source-events.jsonl"
    cpp_result = preview._run_model(
        build,
        "cpp",
        preview.FIXTURE / "configs/three-ticks.json",
        events=cpp_events,
    )
    assert cpp_result.returncode == 0, cpp_result.stderr
    preview._oracle().assert_design_top_run(cpp_events.read_bytes())

    rtl_result = _verilate_groups(payload, tmp_path / "grouped-rtl")
    assert rtl_result.returncode == 0, rtl_result.stdout + rtl_result.stderr
    assert "finish" in rtl_result.stdout.lower()
    assert preview._sha256(final) == final_hash


def test_rtl_source_module_name_collision_fails_without_partial_output(
    tmp_path: Path,
) -> None:
    source_root = tmp_path / "collision-source"
    source_root.mkdir()
    (source_root / "types.py").write_text(cpp.TYPES, encoding="utf-8")
    for filename, definition in (
        ("fooBar.py", "FirstUnit"),
        ("foo_bar.py", "SecondUnit"),
    ):
        (source_root / filename).write_text(
            "from pycircuit import module, rule\n"
            "from .types import Word\n\n"
            "@module\n"
            f"def {definition}(incoming: Word, outgoing: Word):\n"
            "    @rule\n"
            "    def forward():\n"
            "        nonlocal outgoing\n"
            "        outgoing = incoming\n"
            "    forward()\n",
            encoding="utf-8",
        )
    (source_root / "top.py").write_text(
        "from pycircuit import system\n"
        "from .types import Word\n"
        "from .fooBar import FirstUnit\n"
        "from .foo_bar import SecondUnit\n\n"
        "@system\n"
        "def CollisionTop():\n"
        "    first_in: Word = 3\n"
        "    second_in: Word = 10\n"
        "    first_out: Word = 0\n"
        "    second_out: Word = 0\n"
        "    first = FirstUnit(first_in, first_out)\n"
        "    second = SecondUnit(second_in, second_out)\n",
        encoding="utf-8",
    )
    types = cpp._compile_source(
        source_root / "types.py",
        source_root=source_root,
        output_dir=tmp_path / "units/types",
    )
    first = cpp._compile_source(
        source_root / "fooBar.py",
        source_root=source_root,
        output_dir=tmp_path / "units/fooBar",
        headers=(types.header,),
    )
    second = cpp._compile_source(
        source_root / "foo_bar.py",
        source_root=source_root,
        output_dir=tmp_path / "units/foo_bar",
        headers=(types.header,),
    )
    top = cpp._compile_source(
        source_root / "top.py",
        source_root=source_root,
        output_dir=tmp_path / "units/top",
        headers=(types.header, first.header, second.header),
    )
    final = tmp_path / "collision.final.ac"
    linked = cpp._link(
        [types, first, second, top],
        final,
        top="demo.top.CollisionTop",
    )
    assert linked.returncode == 0, linked.stderr

    completed = subprocess.run(
        [
            str(preview._parts_helper(preview._require_preview_tools())),
            str(final),
            "--runner",
        ],
        text=True,
        capture_output=True,
        check=False,
        timeout=180,
    )
    assert completed.returncode == 1
    assert completed.stdout == "", "rejected collision must not publish partial JSON"
    assert re.search(r"colli(sion|de)|same", completed.stderr, re.IGNORECASE)


def test_unicode_scalar_formals_compile_and_match_on_cpp_and_verilog(
    tmp_path: Path,
) -> None:
    source_root = tmp_path / "unicode-source"
    shutil.copytree(public.FIXTURE / "src", source_root)
    counter = source_root / "counter.py"
    text = counter.read_text(encoding="utf-8")
    counter.write_text(
        text.replace("incoming", "输入").replace("outgoing", "输出"),
        encoding="utf-8",
    )
    final = public._compile_link(source_root, tmp_path / "published", "m5_public")
    cpp_output, rtl_output = tmp_path / "unicode-cpp", tmp_path / "unicode-rtl"
    public._emit(final, "cpp", cpp_output)
    public._emit(final, "verilog", rtl_output)

    rtl = "\n".join(
        path.read_text(encoding="utf-8") for path in sorted(rtl_output.rglob("*.sv"))
    )
    assert "输入" not in rtl and "输出" not in rtl

    cpp_events = _build_and_run_generated_target(
        cpp_output, "cpp", tmp_path / "unicode-run"
    )
    rtl_events = _build_and_run_generated_target(
        rtl_output, "verilog", tmp_path / "unicode-run"
    )
    assert cpp_events == rtl_events
    preview._oracle().assert_design_top_run(cpp_events)


def test_verilog_formals_that_collide_after_identifier_legalization_are_rejected(
    tmp_path: Path,
) -> None:
    source_root = tmp_path / "formal-collision-source"
    source_root.mkdir()
    (source_root / "types.py").write_text(cpp.TYPES, encoding="utf-8")
    (source_root / "child.py").write_text(
        "from pycircuit import module, rule\n"
        "from .types import Word\n\n"
        "@module\n"
        "def Child(A: Word, a: Word):\n"
        "    @rule\n"
        "    def combine():\n"
        "        nonlocal a\n"
        "        a = A\n"
        "    combine()\n",
        encoding="utf-8",
    )
    (source_root / "top.py").write_text(
        "from pycircuit import module\n"
        "from .types import Word\n"
        "from .child import Child\n\n"
        "@module\n"
        "def Top():\n"
        "    upper: Word = 3\n"
        "    lower: Word = 5\n"
        "    child = Child(upper, lower)\n",
        encoding="utf-8",
    )
    types = cpp._compile_source(
        source_root / "types.py",
        source_root=source_root,
        output_dir=tmp_path / "units/types",
    )
    child = cpp._compile_source(
        source_root / "child.py",
        source_root=source_root,
        output_dir=tmp_path / "units/child",
        headers=(types.header,),
    )
    top = cpp._compile_source(
        source_root / "top.py",
        source_root=source_root,
        output_dir=tmp_path / "units/top",
        headers=(types.header, child.header),
    )
    final = tmp_path / "formal-collision.final.ac"
    linked = cpp._link([types, child, top], final, top="demo.top.Top", role=None)
    assert linked.returncode == 0, linked.stderr

    output = tmp_path / "rejected-rtl"
    rejected = public._emit(final, "verilog", output, expected=1)
    assert re.search(r"colli(sion|de)", rejected.stderr, re.IGNORECASE)
    assert not output.exists()


def test_verilog_rejects_formal_enable_name_collision_with_owned_state(
    tmp_path: Path,
) -> None:
    source_root = tmp_path / "formal-state-collision-source"
    source_root.mkdir()
    (source_root / "types.py").write_text(cpp.TYPES, encoding="utf-8")
    (source_root / "child.py").write_text(
        "from pycircuit import module, rule\n"
        "from .types import Word\n\n"
        "@module\n"
        "def Child(Q0: Word):\n"
        "    state0: Word = 0\n\n"
        "    @rule\n"
        "    def advance():\n"
        "        nonlocal Q0, state0\n"
        "        Q0 = state0\n"
        "        state0 = Q0\n"
        "    advance()\n",
        encoding="utf-8",
    )
    (source_root / "top.py").write_text(
        "from pycircuit import module\n"
        "from .types import Word\n"
        "from .child import Child\n\n"
        "@module\n"
        "def Top():\n"
        "    value: Word = 0\n"
        "    child = Child(value)\n",
        encoding="utf-8",
    )
    types = cpp._compile_source(
        source_root / "types.py",
        source_root=source_root,
        output_dir=tmp_path / "units/types",
    )
    child = cpp._compile_source(
        source_root / "child.py",
        source_root=source_root,
        output_dir=tmp_path / "units/child",
        headers=(types.header,),
    )
    top = cpp._compile_source(
        source_root / "top.py",
        source_root=source_root,
        output_dir=tmp_path / "units/top",
        headers=(types.header, child.header),
    )
    final = tmp_path / "formal-state-collision.final.ac"
    linked = cpp._link([types, child, top], final, top="demo.top.Top", role=None)
    assert linked.returncode == 0, linked.stderr

    output = tmp_path / "formal-state-collision-rtl"
    rejected = public._emit(final, "verilog", output, expected=1)
    assert re.search(r"colli(sion|de)", rejected.stderr, re.IGNORECASE)
    assert not output.exists()


def test_verilog_rejects_parent_formal_collision_with_child_output_wire(
    tmp_path: Path,
) -> None:
    source_root = tmp_path / "parent-wire-collision-source"
    source_root.mkdir()
    (source_root / "types.py").write_text(cpp.TYPES, encoding="utf-8")
    (source_root / "child.py").write_text(
        "from pycircuit import module, rule\n"
        "from .types import Word\n\n"
        "@module\n"
        "def Child(incoming: Word, outgoing: Word):\n"
        "    @rule\n"
        "    def forward():\n"
        "        nonlocal outgoing\n"
        "        outgoing = incoming\n"
        "    forward()\n",
        encoding="utf-8",
    )
    (source_root / "parent.py").write_text(
        "from pycircuit import module\n"
        "from .types import Word\n"
        "from .child import Child\n\n"
        "@module\n"
        "def Parent(child_0_outgoing: Word, incoming: Word):\n"
        "    child = Child(incoming, child_0_outgoing)\n",
        encoding="utf-8",
    )
    (source_root / "top.py").write_text(
        "from pycircuit import module\n"
        "from .types import Word\n"
        "from .parent import Parent\n\n"
        "@module\n"
        "def Top():\n"
        "    incoming: Word = 3\n"
        "    outgoing: Word = 0\n"
        "    parent = Parent(outgoing, incoming)\n",
        encoding="utf-8",
    )
    types = cpp._compile_source(
        source_root / "types.py",
        source_root=source_root,
        output_dir=tmp_path / "units/types",
    )
    child = cpp._compile_source(
        source_root / "child.py",
        source_root=source_root,
        output_dir=tmp_path / "units/child",
        headers=(types.header,),
    )
    parent = cpp._compile_source(
        source_root / "parent.py",
        source_root=source_root,
        output_dir=tmp_path / "units/parent",
        headers=(types.header, child.header),
    )
    top = cpp._compile_source(
        source_root / "top.py",
        source_root=source_root,
        output_dir=tmp_path / "units/top",
        headers=(types.header, child.header, parent.header),
    )
    final = tmp_path / "parent-wire-collision.final.ac"
    linked = cpp._link(
        [types, child, parent, top], final, top="demo.top.Top", role=None
    )
    assert linked.returncode == 0, linked.stderr

    output = tmp_path / "parent-wire-collision-rtl"
    rejected = public._emit(final, "verilog", output, expected=1)
    assert re.search(r"colli(sion|de)", rejected.stderr, re.IGNORECASE)
    assert not output.exists()


def test_distinct_source_owners_can_each_define_a_child_named_child(
    tmp_path: Path,
) -> None:
    source_root = tmp_path / "same-short-name-source"
    for directory in (source_root / "left", source_root / "right"):
        directory.mkdir(parents=True, exist_ok=True)
    child_source = (
        "from typing import Annotated\n"
        "from pycircuit import module, rule\n\n"
        "Word = Annotated[int, range(256)]\n\n"
        "@module\n"
        "def Child(incoming: Word, outgoing: Word):\n"
        "    @rule\n"
        "    def forward():\n"
        "        nonlocal outgoing\n"
        "        outgoing = incoming\n"
        "    forward()\n"
    )
    for relative in ("left/child.py", "right/child.py"):
        (source_root / relative).write_text(child_source, encoding="utf-8")
    (source_root / "top.py").write_text(
        "from typing import Annotated\n"
        "from pycircuit import module\n"
        "from .left.child import Child as LeftChild\n"
        "from .right.child import Child as RightChild\n\n"
        "Word = Annotated[int, range(256)]\n\n"
        "@module\n"
        "def Top():\n"
        "    left_in: Word = 3\n"
        "    right_in: Word = 10\n"
        "    left_out: Word = 0\n"
        "    right_out: Word = 0\n"
        "    left = LeftChild(left_in, left_out)\n"
        "    right = RightChild(right_in, right_out)\n",
        encoding="utf-8",
    )
    left = cpp._compile_source(
        source_root / "left/child.py",
        source_root=source_root,
        output_dir=tmp_path / "units/left-child",
    )
    right = cpp._compile_source(
        source_root / "right/child.py",
        source_root=source_root,
        output_dir=tmp_path / "units/right-child",
    )
    top = cpp._compile_source(
        source_root / "top.py",
        source_root=source_root,
        output_dir=tmp_path / "units/top",
        headers=(left.header, right.header),
    )
    final = tmp_path / "same-short-name.final.ac"
    linked = cpp._link([left, right, top], final, top="demo.top.Top", role=None)
    assert linked.returncode == 0, linked.stderr

    output = tmp_path / "same-short-name-rtl"
    public._emit(final, "verilog", output)
    receipt = json.loads((output / "generated.json").read_text(encoding="utf-8"))
    groups = {group["source"]["path"]: group for group in receipt["source_groups"]}
    assert "left/child.py" in groups
    assert "right/child.py" in groups
    module_names = []
    for owner in ("left/child.py", "right/child.py"):
        source_files = [path for path in groups[owner]["files"] if path.endswith(".sv")]
        assert len(source_files) == 1
        text = (output / source_files[0]).read_text(encoding="utf-8")
        match = re.search(r"\bmodule\s+([A-Za-z_][A-Za-z0-9_$]*)", text)
        assert match is not None
        module_names.append(match.group(1))
    assert len(set(module_names)) == 2

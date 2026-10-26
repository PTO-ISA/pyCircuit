"""Physical-edge execution observes old state and rejects partial commits."""

from __future__ import annotations

import json
import shutil
from pathlib import Path

import pytest
from test_source_system_execution import ROOT, _cli, _ok, _run

pytestmark = pytest.mark.system

SOURCE = """import pycircuit as ac
from enum import Enum
@ac.encoding(width=2)
class Tag(Enum):
    READY = 2
    DONE = 3
@ac.struct
class Inner:
    tag: Tag
    count: ac.u5
@ac.struct
class Record:
    inner: Inner
    payload: ac.u8
@ac.module
def RecordInitial(incoming: ac.u8) -> Record:
    saved: Record = Record(inner=Inner(tag=Tag.READY, count=17), payload=165)
    @ac.rule
    def hold() -> None:
        saved = saved
    return saved
@ac.rule
def write(saved: ac.u8, incoming: ac.u8, enabled: ac.u1) -> None:
    if enabled:
        saved = incoming
@ac.module
def Cell(incoming: ac.u8, enabled: ac.u1) -> ac.u8:
    saved: ac.u8 = 0
    write(saved, incoming, enabled)
    return saved
@ac.module
def GuardedCell(incoming: ac.u8, enabled: ac.u1) -> ac.u8:
    saved: ac.u8 = 0
    @ac.rule(guard=lambda: enabled)
    def update() -> None:
        saved = incoming
    return saved
@ac.module
def GuardedPair(first: ac.u8, second: ac.u8, enable_first: ac.u1,
                enable_second: ac.u1) -> tuple[ac.u8, ac.u8]:
    return GuardedCell(first, enable_first), GuardedCell(second, enable_second)
@ac.module
def TableState(incoming: ac.u8) -> ac.table[2, ac.u8]:
    entries: ac.table[2, ac.u8] = (1, 2)
    @ac.rule
    def update() -> None:
        entries[0] = incoming
        entries[1] = entries[0] + 1
    return entries
@ac.module
def TableList(incoming: ac.u8) -> ac.table[2, ac.u8]:
    entries: ac.table[2, ac.u8] = [3, 4]
    @ac.rule
    def update() -> None:
        entries[0] = incoming
        entries[1] = entries[0] + 1
    return entries
@ac.module
def Pair(first: ac.u8, second: ac.u8, enable_first: ac.u1,
         enable_second: ac.u1) -> tuple[ac.u8, ac.u8]:
    return Cell(first, enable_first), Cell(second, enable_second)
@ac.rule
def value(incoming: ac.u8) -> ac.u8:
    return incoming
@ac.module
def Stateless(incoming: ac.u8) -> ac.u8:
    return value(incoming)
@ac.module
def WideInitial(incoming: ac.u8) -> tuple[ac.bits[65], ac.bits[130], ac.bits[65]]:
    first: ac.bits[65] = (1 << 64) + 1
    second: ac.bits[130] = (1 << 129) + 3
    low_word: ac.bits[65] = 0xffffffff
    @ac.rule
    def hold() -> None:
        first = first
        second = second
        low_word = low_word
    return first, second, low_word
"""


@pytest.fixture(scope="module")
def physical_designs(tmp_path_factory):
    root = tmp_path_factory.mktemp("physical-export")
    source = root / "design.py"
    source.write_text(SOURCE)
    unit = root / "unit"
    _ok(
        _cli(
            "compile",
            "-c",
            str(source),
            "--source-root",
            str(root),
            "--package-prefix",
            "physical",
            "-o",
            str(unit),
        )
    )
    result = {}
    for name in (
        "Pair",
        "GuardedPair",
        "Stateless",
        "TableState",
        "TableList",
        "WideInitial",
        "RecordInitial",
    ):
        final = root / (name + ".ac")
        _ok(
            _cli(
                "link", str(unit), "--top", "physical.design." + name, "-o", str(final)
            )
        )
        output = root / name
        _ok(
            _cli(
                "emit",
                str(final),
                "--target",
                "verilog",
                "--rtl-mode",
                "physical",
                "-o",
                str(output),
            )
        )
        result[name] = (final, output)
    return result


def simulate(
    output: Path,
    tmp_path: Path,
    body: str,
    *,
    stateless=False,
    reset=True,
    verilator=False,
) -> str:
    assert shutil.which("iverilog") and shutil.which("vvp"), "four-state tools required"
    manifest = json.loads((output / "generated.json").read_text())
    ports = json.loads((output / "ports.json").read_text())
    assert ports["controls"] == [], "physical wrapper exposes no lifecycle controls"
    declarations, bindings = [], []
    domains = ports["domains"]
    for row in ports["inputs"]:
        name = (
            "clk"
            if row["ordinal"] == domains["clock"]
            else "rst" if row["ordinal"] == domains["reset"] else row["name"]
        )
        declarations.append(f"logic [{row['width'] - 1}:0] {name};")
        bindings.append(f".{row['wire']}({name})")
    for index, row in enumerate(ports["outputs"]):
        declarations.append(f"wire [{row['width'] - 1}:0] result_{index};")
        bindings.append(f".{row['wire']}(result_{index})")
    init = (
        "incoming=8'h3c;"
        if stateless
        else ("first=8'h12;second=8'h34;enable_first=1;enable_second=1;")
    )
    tb = tmp_path / "test.sv"
    tb.write_text(
        "module tb;\n"
        + "\n".join(declarations)
        + "\n"
        + manifest["root_rtl_name"]
        + " dut("
        + ",".join(bindings)
        + ");\n"
        + "task edge_up; #2;clk=1;#2;endtask\n"
        "task edge_down; #2;clk=0;#2;endtask\n"
        "initial begin clk=0;rst=1;"
        + init
        + "\n"
        + ("edge_up();edge_down();rst=0;\n" if reset else "#1;rst=0;\n")
        + body
        + '\n$display("PHYSICAL_TEST_COMPLETE");$finish;end\nendmodule\n'
    )
    rtl = [
        str(output / item["path"])
        for item in manifest["files"]
        if item["path"].endswith((".v", ".sv"))
    ]
    binary = tmp_path / "test.vvp"
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-s",
                "tb",
                "-o",
                str(binary),
                *rtl,
                str(ROOT / "include/verilog/dff.v"),
                str(ROOT / "include/verilog/dffe.v"),
                str(tb),
            ]
        )
    )
    transcript = _ok(_run(["vvp", str(binary)]))
    assert "PHYSICAL_TEST_COMPLETE" in transcript
    if verilator:
        build = tmp_path / "verilated"
        _ok(
            _run(
                [
                    "verilator",
                    "--binary",
                    "--timing",
                    "-Wno-fatal",
                    "--top-module",
                    "tb",
                    "--Mdir",
                    str(build),
                    *rtl,
                    str(ROOT / "include/verilog/dff.v"),
                    str(ROOT / "include/verilog/dffe.v"),
                    str(tb),
                ],
                timeout=180,
            )
        )
        known = _ok(_run([str(build / "Vtb")]))
        assert "PHYSICAL_TEST_COMPLETE" in known
        assert "PYCIRCUIT_PHYSICAL_FAULT" not in known
    return transcript


@pytest.mark.parametrize("design", ["Pair", "GuardedPair"])
def test_physical_hold_reset_and_four_state_payload(physical_designs, tmp_path, design):
    body = """
if(result_0!==0 || result_1!==0)$fatal(1,"reset");
edge_up();
if(result_0!==8'h12 || result_1!==8'h34)$fatal(1,"first edge");
rst=1'bx;enable_second=1'bx;#2;
if(result_0!==8'h12 || result_1!==8'h34)$fatal(1,"held edge");
edge_down();rst=0;enable_first=0;enable_second=1;
first=8'hff;second=8'b10xz0011;edge_up();
if(result_0!==8'h12 || result_1!==8'b10xz0011)$fatal(1,"hold/payload transport");
edge_down();rst=1;enable_first=1'bx;enable_second=1'bz;edge_up();
if(result_0!==0 || result_1!==0)$fatal(1,"reset priority");
"""
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in simulate(
        physical_designs[design][1], tmp_path, body
    )


@pytest.mark.parametrize(
    "fault",
    ["guard_x", "guard_z", "reset_x", "reset_z", "clock_x", "clock_z", "high_clock_x"],
)
@pytest.mark.parametrize("design", ["Pair", "GuardedPair"])
def test_physical_fault_freezes_every_owner(physical_designs, tmp_path, fault, design):
    inject = {
        "guard_x": "enable_second=1'bx;edge_up();",
        "guard_z": "enable_second=1'bz;edge_up();",
        "reset_x": "rst=1'bx;edge_up();",
        "reset_z": "rst=1'bz;edge_up();",
        "clock_x": "#2;clk=1'bx;#2;",
        "clock_z": "#2;clk=1'bz;#2;",
        "high_clock_x": "enable_first=0;enable_second=0;edge_up();clk=1'bx;#2;",
    }[fault]
    body = (
        """
edge_up();edge_down();first=8'haa;second=8'hbb;
"""
        + inject
        + """
if(result_0!==8'h12 || result_1!==8'h34)$fatal(1,"partial commit");
clk=0;rst=0;enable_first=1;enable_second=1;#2;edge_up();
if(result_0!==8'h12 || result_1!==8'h34)$fatal(1,"sticky failure");
edge_down();rst=1;edge_up();
if(result_0!==8'h12 || result_1!==8'h34)$fatal(1,"physical reset recovered failure");
"""
    )
    transcript = simulate(physical_designs[design][1], tmp_path, body)
    assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 1


@pytest.mark.parametrize("unknown", ["x", "z"])
def test_stateless_transaction_still_checks_reset(physical_designs, tmp_path, unknown):
    transcript = simulate(
        physical_designs["Stateless"][1],
        tmp_path,
        f"rst=1'b{unknown};edge_up();",
        stateless=True,
    )
    assert transcript.count("PYCIRCUIT_PHYSICAL_FAULT") == 1


def test_uniform_domain_collection_keeps_sequential_state_updates(
    physical_designs, tmp_path
):
    transcript = simulate(
        physical_designs["TableState"][1],
        tmp_path,
        """
if(result_0!==16'h0102)$fatal(1,"collection initialization");
edge_up();
if(result_0!==16'h3c3d)$fatal(1,"collection candidate order");
""",
        stateless=True,
    )
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


def test_declared_wide_initializers_before_any_clock_or_reset(
    physical_designs, tmp_path
):
    transcript = simulate(
        physical_designs["WideInitial"][1],
        tmp_path,
        """
if(result_0!==65'h10000000000000001)$fatal(1,"65 bit startup");
if(result_1!==130'h200000000000000000000000000000003)$fatal(1,"130 bit startup");
if(result_2!==65'h000000000ffffffff)$fatal(1,"unsigned startup extension");
edge_up();
if(result_0!==65'h10000000000000001)$fatal(1,"startup hold");
""",
        stateless=True,
        reset=False,
        verilator=True,
    )
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


def test_nonuniform_table_initializers_before_any_clock(physical_designs, tmp_path):
    transcript = simulate(
        physical_designs["TableState"][1],
        tmp_path,
        """
if(result_0!==16'h0102)$fatal(1,"nonuniform startup");
edge_up();
if(result_0!==16'h3c3d)$fatal(1,"table first edge");
""",
        stateless=True,
        reset=False,
        verilator=True,
    )
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


@pytest.mark.parametrize(
    ("design", "expected"), [("TableList", "16'h0304"), ("RecordInitial", "15'h51a5")]
)
def test_list_and_nested_nominal_startup(physical_designs, tmp_path, design, expected):
    transcript = simulate(
        physical_designs[design][1],
        tmp_path,
        f'if(result_0!=={expected})$fatal(1,"declared startup packing");',
        stateless=True,
        reset=False,
        verilator=True,
    )
    assert "PYCIRCUIT_PHYSICAL_FAULT" not in transcript


def test_physical_option_rejects_cpp_without_replacing_output(
    physical_designs, tmp_path
):
    final, _ = physical_designs["Pair"]
    output = tmp_path / "cpp"
    _ok(_cli("emit", str(final), "--target", "cpp", "-o", str(output)))
    before = {
        p.relative_to(output): p.read_bytes() for p in output.rglob("*") if p.is_file()
    }
    rejected = _cli(
        "emit",
        str(final),
        "--target",
        "cpp",
        "--rtl-mode",
        "physical",
        "-o",
        str(output),
        "--replace",
    )
    assert rejected.returncode != 0
    assert before == {
        p.relative_to(output): p.read_bytes() for p in output.rglob("*") if p.is_file()
    }


def test_physical_synthesis_elaborates_without_fault_history(
    physical_designs, tmp_path
):
    output = physical_designs["Pair"][1]
    manifest = json.loads((output / "generated.json").read_text())
    rtl = [
        str(output / row["path"])
        for row in manifest["files"]
        if row["path"].endswith((".v", ".sv"))
    ]
    rtl += [str(ROOT / "include/verilog/dff.v"), str(ROOT / "include/verilog/dffe.v")]
    _ok(
        _run(
            [
                "iverilog",
                "-g2012",
                "-DSYNTHESIS",
                "-s",
                manifest["root_rtl_name"],
                "-o",
                str(tmp_path / "synthesis.vvp"),
                *rtl,
            ]
        )
    )
    preprocessed = tmp_path / "synthesis.sv"
    _ok(
        _run(["iverilog", "-g2012", "-DSYNTHESIS", "-E", "-o", str(preprocessed), *rtl])
    )
    assert "pyc_physical_fault" not in preprocessed.read_text()


@pytest.mark.parametrize("image", ["(1,)", "[1, 2, 3]", "(1, 256)", "(1, incoming)"])
def test_invalid_complete_table_initializers_preserve_unit(tmp_path, image):
    source = tmp_path / "table.py"
    good = """import pycircuit as ac
@ac.module
def Top(incoming: ac.u8) -> ac.table[2, ac.u8]:
    entries: ac.table[2, ac.u8] = (1, 2)
    return entries
"""
    source.write_text(good)
    unit = tmp_path / "unit"
    arguments = (
        "compile",
        "-c",
        str(source),
        "--source-root",
        str(tmp_path),
        "--package-prefix",
        "table_init",
        "-o",
        str(unit),
    )
    _ok(_cli(*arguments))
    before = {
        p.relative_to(unit): p.read_bytes() for p in unit.rglob("*") if p.is_file()
    }
    source.write_text(good.replace("= (1, 2)", "= " + image))
    rejected = _cli(*arguments, "--replace")
    assert rejected.returncode != 0
    assert before == {
        p.relative_to(unit): p.read_bytes() for p in unit.rglob("*") if p.is_file()
    }

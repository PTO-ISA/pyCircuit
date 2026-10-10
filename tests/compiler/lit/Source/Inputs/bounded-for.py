"""Public bounded-loop compile, rejection, rollback and execution regression."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

parser = argparse.ArgumentParser()
for name in (
    "repo",
    "source-compiler",
    "linker",
    "emitter",
    "cxx",
    "verilator",
    "iverilog",
    "vvp",
    "scratch",
):
    parser.add_argument("--" + name, required=True)
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
build = Path(tempfile.mkdtemp(prefix="bounded-for-", dir=scratch))
env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def run(command, *, accepted=True, cwd=repo):
    command = list(map(str, command))
    started = time.monotonic()
    result = subprocess.run(
        command,
        cwd=cwd,
        env=env,
        text=True,
        capture_output=True,
        timeout=600,
    )
    commands.append(
        {
            "command": command,
            "exit_status": result.returncode,
            "elapsed_seconds": time.monotonic() - started,
            "timeout_seconds": 600,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
    )
    (build / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if accepted else 1), commands[-1]
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted=accepted)


def snapshot(path):
    return {
        item.relative_to(path).as_posix(): item.read_bytes()
        for item in path.rglob("*")
        if item.is_file()
    }


def binary(value, width):
    return format(value & ((1 << width) - 1), f"0{width}b")


def planes(symbols):
    value = "".join(
        "1" if symbol == "1" or (symbol in "xz" and (len(symbols) - index - 1) % 3 == 0) else "0"
        for index, symbol in enumerate(symbols)
    )
    known = "".join("1" if symbol in "01" else "0" for symbol in symbols)
    z = "".join("1" if symbol == "z" else "0" for symbol in symbols)
    return value, known, z


def oracle(row):
    values = [row["first"], row["second"], row["third"]]
    prefixes = []
    accumulator = 0
    pair_first = row["seed"]
    pair_second = row["seed"]
    pair_prefixes = []
    for value in values:
        accumulator = (accumulator - value) & 0xFF
        prefixes.append(accumulator)
        pair_first = (pair_first + value) & 0xFF
        pair_prefixes.append((pair_first, pair_second))
    wide_last = sum(row[f"wide{index}"] for index in range(5)) & 0x1FFF
    conditional = 0
    for value in values:
        conditional = 1 if value == 0 else (conditional + value) & 0xFF
    aggregate_lanes = [row["wide0"], row["wide1"], row["wide2"]]
    aggregate_snapshot = list(aggregate_lanes)
    alternative_lanes = [row["wide2"], row["wide3"], row["wide4"]]
    aggregate_marker = row["seed"]
    for value in values:
        aggregate_lanes = [0, 0, 0] if value == 0 else list(alternative_lanes)
        aggregate_marker = (aggregate_marker + value) & 0xFF
    shared_child = (row["first"], row["second"])
    alternative_child = (row["second"], row["third"])
    box = (shared_child, shared_child)
    for value in values:
        box = box if value == 0 else (alternative_child, shared_child)
    wide_flag = (
        "0"
        if "1" in row["carrier"]
        else "1"
        if set(row["carrier"]) == {"0"}
        else "x"
    )
    fields = (
        binary(row["one_value"], 3),
        binary(prefixes[0], 8),
        binary(prefixes[2], 8),
        binary(wide_last, 13),
        binary(pair_prefixes[2][0], 8),
        binary(pair_prefixes[2][1], 8),
        binary(0, 8),
        binary(0, 8),
        binary(3, 8),
        binary(2, 8),
        binary(1, 8),
        binary(12, 8),
        binary(row["seed"], 8),
        binary(conditional, 8),
        binary(aggregate_lanes[1], 13),
        binary(aggregate_marker, 8),
        binary(aggregate_snapshot[1], 13),
        binary(row["seed"], 8),
        binary(box[0][0], 8),
        binary(box[1][1], 8),
        wide_flag,
        binary(0 if row["first"] == 0 else 8, 8),
        binary(row["seed"], 8),
        row["carrier"],
    )
    return "".join(fields)


carrier_values = [
    binary(0, 65),
    binary((1 << 65) - 1, 65),
    "01" * 32 + "1",
    "x" * 65,
    "z" * 65,
    "01xz" * 16 + "0",
]
rows = []
for index, carrier in enumerate(carrier_values):
    rows.append(
        {
            "one_value": index & 7,
            "first": 0 if index == 0 else (index * 73 + 1) & 0xFF,
            "second": 0 if index in (0, 2) else (index * 31 + 2) & 0xFF,
            "third": 0 if index == 0 else (index * 127 + 3) & 0xFF,
            "wide0": index,
            "wide1": 8191 - index,
            "wide2": index * 97,
            "wide3": index * 509,
            "wide4": index * 1021,
            "seed": (index * 29 + 7) & 0xFF,
            "carrier": carrier,
        }
    )
expected = [oracle(row) for row in rows]
result_width = 252
assert all(len(value) == result_width for value in expected)

source = build / "source"
source.mkdir()
design = source / "design.py"
original = (fixtures / "bounded-for-design.py").read_text()
design.write_text(original)
system_source = source / "system.py"
original_system = (fixtures / "bounded-for-system.py").read_text()
system_source.write_text(original_system)
unit = build / "unit"


def compile_source(output, *, accepted=True, replace=False):
    arguments = [
        "compile",
        "-c",
        design,
        "--source-root",
        source,
        "--package-prefix",
        "bounded_for",
        "-o",
        output,
    ]
    if replace:
        arguments.append("--replace")
    return cli(*arguments, accepted=accepted)


# The old compiler fails here with the bounded-loop diagnostic, establishing RED.
compile_source(unit)
published = (unit / "design.ac").read_text()
assert 'kind = "iteration"' in published
for ordinal in range(5):
    if ordinal in (0, 1, 2, 4):
        assert f"ordinal = {ordinal}" in published or f"ordinal = {ordinal} : i64" in published

final = build / "top.ac"
cli("link", unit, "--top", "bounded_for.design.Top", "-o", final)
for target in ("cpp", "verilog"):
    cli("emit", final, "--target", target, "-o", build / target)

header = [
    f"constexpr unsigned row_count={len(rows)}, result_width={result_width};",
    "constexpr std::string_view expected_symbols[]={"
    + ",".join(json.dumps(value) for value in expected)
    + "};",
    "void drive_inputs(pyc_dut::Inputs &inputs,unsigned row){switch(row){",
]
for index, row in enumerate(rows):
    header.append(f"case {index}:")
    for name, width in (
        ("one_value", 3),
        ("first", 8),
        ("second", 8),
        ("third", 8),
        ("wide0", 13),
        ("wide1", 13),
        ("wide2", 13),
        ("wide3", 13),
        ("wide4", 13),
        ("seed", 8),
    ):
        header.append(f"inputs.{name}=known<{width}>({row[name]});")
    header.append(f"inputs.carrier=bits<65>({json.dumps(row['carrier'])});break;")
header.append("default:require(false);}}")
(build / "bounded-for-vectors.hpp").write_text("\n".join(header) + "\n")

sv = [
    f"localparam integer row_count={len(rows)},result_width={result_width};",
    "logic [2:0] one_value=0;",
    "logic [7:0] first=0,second=0,third=0,seed=0;",
    "logic [12:0] wide0=0,wide1=0,wide2=0,wide3=0,wide4=0;",
    "logic [64:0] carrier=0;",
    f"wire [{result_width-1}:0] result;",
    "task drive(input integer row);case(row)",
]
for index, row in enumerate(rows):
    sv.append(f"{index}:begin")
    for name, width in (
        ("one_value", 3),
        ("first", 8),
        ("second", 8),
        ("third", 8),
        ("wide0", 13),
        ("wide1", 13),
        ("wide2", 13),
        ("wide3", 13),
        ("wide4", 13),
        ("seed", 8),
    ):
        sv.append(f"{name}={width}'d{row[name]};")
    sv.append(f"carrier=65'b{row['carrier']};end")
sv.append("endcase endtask")
sv.append("function automatic bit known_row(input integer row);known_row=(row<3);endfunction")
sv.append(f"function automatic logic[{result_width-1}:0] golden(input integer row);case(row)")
for index, value in enumerate(expected):
    sv.append(f"{index}:golden={result_width}'b{value};")
sv.append("default:golden='x;endcase endfunction")
(build / "bounded-for-vectors.svh").write_text("\n".join(sv) + "\n")

toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next(
    path
    for path in (
        toolroot / "simulator/gfsim/libpyc6_runtime.a",
        toolroot / "lib/libpyc6_runtime.a",
    )
    if path.is_file()
)
receipt = json.loads((build / "cpp/generated.json").read_text())
cpp_sources = [
    build / "cpp" / row["path"]
    for row in receipt["files"]
    if row["path"].endswith(".cpp")
]
runner = build / "runner"
run(
    [
        args.cxx,
        "-std=c++20",
        "-pthread",
        "-I" + str(repo / "include"),
        "-I" + str(build / "cpp"),
        "-I" + str(build),
        fixtures / "bounded-for.cpp",
        *cpp_sources,
        runtime,
        "-o",
        runner,
    ]
)
config = build / "config.json"
config.write_text(
    '{"deadlock_window":null,"max_domain_cycles":{},"max_ticks":64,'
    '"schema":"pycircuit-model-config","version":"1"}\n'
)
native_traces = []
for workers in (1, 2):
    trace = run([runner, "--workers", workers, "--config", config]).stdout
    native_traces.append([line for line in trace.splitlines() if line.startswith("WORK ")])
assert native_traces[0] == native_traces[1] == [
    f"WORK {index} {value}" for index, value in enumerate(expected)
]

rtl_receipt = json.loads((build / "verilog/generated.json").read_text())
rtl = [
    build / "verilog" / row["path"]
    for row in rtl_receipt["files"]
    if row["role"] == "rtl"
]
rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
verilator_build = build / "verilator"
run(
    [
        args.verilator,
        "--binary",
        "--timing",
        "--top-module",
        "tb",
        "--prefix",
        "Vbounded_for",
        "--Mdir",
        verilator_build,
        "-j",
        "2",
        "-Wno-fatal",
        "-I" + str(build),
        *rtl,
        fixtures / "bounded-for.sv",
    ]
)
verilator_trace = run([verilator_build / "Vbounded_for"]).stdout
assert [line for line in verilator_trace.splitlines() if line.startswith("WORK ")] == native_traces[0][:3]
icarus = build / "icarus"
run(
    [
        args.iverilog,
        "-g2012",
        "-DBOUNDED_FOR_FOUR_STATE",
        "-I" + str(build),
        "-s",
        "tb",
        "-o",
        icarus,
        *rtl,
        fixtures / "bounded-for.sv",
    ]
)
icarus_trace = run([args.vvp, icarus]).stdout
assert [line for line in icarus_trace.splitlines() if line.startswith("WORK ")] == native_traces[0]

# A real depth-32 nominal chain exercises recursive descriptor Work separately
# from the 32-bit repeated-child Box above. Both conditional outcomes share the
# exact Node32 type and are observed through the complete public flow.
deep_source = source / "deep.py"
deep_lines = [
    "from pycircuit import module, rule, struct, u1",
    "@struct",
    "class Node0:",
    "    value: u1",
]
for depth in range(1, 33):
    deep_lines.extend(
        ["@struct", f"class Node{depth}:", f"    child: Node{depth - 1}"]
    )
projection = ".child" * 32 + ".value"
deep_lines.extend(
    [
        "@struct",
        "class Envelope:",
        "    child: Node32",
        "    sibling: u1",
        "@struct",
        "class Out:",
        "    value: u1",
        "@rule",
        "def choose(select: u1, left: Node32, right: Node32) -> Out:",
        "    chosen = left",
        "    for index in range(1):",
        "        chosen = left if select else right",
        f"    return Out(value=chosen{projection})",
        "@module",
        "def Top(select: u1, left: Node32, right: Node32) -> Out:",
        "    return choose(select, left, right)",
    ]
)
deep_source.write_text("\n".join(deep_lines) + "\n")
deep_unit = build / "deep-unit"
cli(
    "compile",
    "-c",
    deep_source,
    "--source-root",
    source,
    "--package-prefix",
    "bounded_for_deep",
    "-o",
    deep_unit,
)
deep_final = build / "deep.ac"
cli("link", deep_unit, "--top", "bounded_for_deep.deep.Top", "-o", deep_final)
for target in ("cpp", "verilog"):
    cli("emit", deep_final, "--target", target, "-o", build / ("deep-" + target))

deep_cpp = build / "deep-cpp"
deep_receipt = json.loads((deep_cpp / "generated.json").read_text())
deep_sources = [
    deep_cpp / row["path"]
    for row in deep_receipt["files"]
    if row["path"].endswith(".cpp")
]
deep_driver = build / "deep-driver.cpp"
deep_initialization = [
    "using Node0=::bounded_for_deep::deep::Node0;",
    "using Node32=::bounded_for_deep::deep::Node32;",
    "using Envelope=::bounded_for_deep::deep::Envelope;",
    "static_assert(std::is_aggregate_v<Node0>);",
    "static_assert(std::is_aggregate_v<Node32>);",
    "static_assert(std::is_aggregate_v<Envelope>);",
    "Node0 node0{gfsim::Bits<1>{1}};",
]
for depth in range(1, 33):
    deep_initialization.append(
        f"::bounded_for_deep::deep::Node{depth} node{depth}{{node{depth - 1}}};"
    )
deep_initialization.extend(
    [
        "Node0 partial{};",
        "Node32 value{};",
        "Node0 bare_leaf;",
        "Node32 defaulted;",
        "Envelope partial_record{node32};",
        "if(gfsim::hardware_traits<Node0>::pack(partial)!=gfsim::Bits<1>{0})std::abort();",
        "if(gfsim::hardware_traits<Node32>::pack(value)!=gfsim::Bits<1>{0})std::abort();",
        "if(gfsim::hardware_traits<Node0>::pack(bare_leaf)!=gfsim::Bits<1>{0})std::abort();",
        "if(gfsim::hardware_traits<Node32>::pack(defaulted)!=gfsim::Bits<1>{0})std::abort();",
        "if(gfsim::hardware_traits<Node32>::pack(node32)!=gfsim::Bits<1>{1})std::abort();",
        "if(gfsim::hardware_traits<Node32>::pack(partial_record.child)!=gfsim::Bits<1>{1})std::abort();",
        "if(partial_record.sibling!=gfsim::Bits<1>{0})std::abort();",
        "if(gfsim::hardware_traits<Envelope>::pack(partial_record)!=gfsim::Bits<2>{2})std::abort();",
    ]
)
deep_driver.write_text(
    r'''#include "gfsim/SystemRunner.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <type_traits>
template<unsigned W> auto known(unsigned value){return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});}
struct C{pyc_dut&d;unsigned n=0;static void init(void*p){drive(p,0);}static bool drive(void*p,std::uint64_t e){auto&c=*static_cast<C*>(p);if(e==2)return false;pyc_dut::Inputs i;i.select=known<1>(e==0);i.left=decltype(i.left)::fromPacked(known<1>(1).packed());i.right=decltype(i.right)::fromPacked(known<1>(0).packed());c.d.drive(i);return true;}static void sample(void*p,std::uint64_t e){auto&c=*static_cast<C*>(p);auto r=c.d.sample().result.packed();if(e!=c.n+1||!r.isFullyKnown()||r.value().bit(0)!=(c.n==0))std::abort();std::cout<<"DEEP "<<c.n<<' '<<r.value().bit(0)<<'\n';++c.n;}};
int main(int argc,char**argv){@DEEP_INIT@ gfsim::SystemRunner r(argc,argv);if(!r.ready())return 2;pyc_dut d(r.workers());C c{d};gfsim::RunnerCallbacks cb{&c,&C::init,&C::drive,&C::sample};int s=r.Run(d.system(),d.observations(),{},cb);if(s||c.n!=2)std::abort();return s;}
'''
    .replace("@DEEP_INIT@", "".join(deep_initialization))
)
deep_runner = build / "deep-runner"
run(
    [
        args.cxx,
        "-std=c++20",
        "-pthread",
        "-I" + str(repo / "include"),
        "-I" + str(deep_cpp),
        deep_driver,
        *deep_sources,
        runtime,
        "-o",
        deep_runner,
    ]
)
deep_native = []
for workers in (1, 2):
    trace = run([deep_runner, "--workers", workers, "--config", config]).stdout
    deep_native.append([line for line in trace.splitlines() if line.startswith("DEEP ")])
assert deep_native[0] == deep_native[1] == ["DEEP 0 1", "DEEP 1 0"]

deep_sv = build / "deep.sv"
deep_sv.write_text(
    "module tb;logic select=0,left=1,right=0;wire result;pyc_root dut(.*);"
    "initial begin select=1;#1;if(result!==1)$fatal; $display(\"DEEP 0 %0d\",result);"
    "select=0;#1;if(result!==0)$fatal;$display(\"DEEP 1 %0d\",result);$finish;end endmodule\n"
)
deep_rtl_receipt = json.loads((build / "deep-verilog/generated.json").read_text())
deep_rtl = [
    build / "deep-verilog" / row["path"]
    for row in deep_rtl_receipt["files"]
    if row["role"] == "rtl"
]
deep_rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
deep_verilator = build / "deep-verilator"
run(
    [
        args.verilator,
        "--binary",
        "--timing",
        "--top-module",
        "tb",
        "--prefix",
        "Vdeep",
        "--Mdir",
        deep_verilator,
        "-j",
        "2",
        "-Wno-fatal",
        *deep_rtl,
        deep_sv,
    ]
)
assert [
    line for line in run([deep_verilator / "Vdeep"]).stdout.splitlines() if line.startswith("DEEP ")
] == deep_native[0]

protected_unit = snapshot(unit)
protected_final = final.read_bytes()
protected_outputs = {target: snapshot(build / target) for target in ("cpp", "verilog")}


def minimal(body, *, formal="value: ac.u8", call="value"):
    prefix, result = body.rsplit("    return ", 1)
    body = prefix + "    return Out(value=" + result + ")"
    return f"""import pycircuit as ac
from pycircuit import module, rule, struct
@struct
class Out:
    value: ac.u8
@rule
def evaluate({formal}) -> Out:
{body}
@module
def Top(value: ac.u8) -> Out:
    return evaluate({call})
"""


# The negative scaffold itself is admitted before any one mutation below.
design.write_text(
    minimal("    for index in range(1):\n        value = value + 1\n    return value")
)
compile_source(build / "negative-control")
design.write_text(
    minimal("    for index in range(1):\n        if value == 0:\n            joined = index\n        else:\n            joined = value\n    return joined")
)
compile_source(build / "closed-fixed-peer-control")
design.write_text(original)

bookkeeping_body = "".join(
    f"    local{index}: ac.u8 = value\n" for index in range(240)
)
bookkeeping_body += "    for index in range(100):\n"
for depth in range(8):
    bookkeeping_body += "    " * (depth + 2) + f"if value == {depth}:\n"
bookkeeping_body += "    " * 10 + "pass\n"
for depth in reversed(range(8)):
    bookkeeping_body += "    " * (depth + 2) + "else:\n"
    bookkeeping_body += "    " * (depth + 3) + "pass\n"
bookkeeping_body += "    return value"


negative_cases = {
    "boolean-bound": minimal("    for index in range(True):\n        pass\n    return value"),
    "negative-bound": minimal("    for index in range(-1):\n        pass\n    return value"),
    "named-bound": minimal("    count = 3\n    for index in range(count):\n        pass\n    return value"),
    "arithmetic-bound": minimal("    for index in range(1 + 2):\n        pass\n    return value"),
    "start-stop": minimal("    for index in range(0, 3):\n        pass\n    return value"),
    "keyword-bound": minimal("    for index in range(stop=3):\n        pass\n    return value"),
    "starred-bound": minimal("    bounds = (3,)\n    for index in range(*bounds):\n        pass\n    return value"),
    "unpack-target": minimal("    for index, peer in range(3):\n        pass\n    return value"),
    "range-alias": minimal("    alias = range\n    for index in alias(3):\n        pass\n    return value"),
    "qualified-range": """import builtins
import pycircuit as ac
from pycircuit import module, rule, struct
@struct
class Out:
    value: ac.u8
@rule
def evaluate(value: ac.u8) -> Out:
    for index in builtins.range(3):
        value = value + 1
    return Out(value=value)
@module
def Top(value: ac.u8) -> Out:
    return evaluate(value)
""",
    "shadowed-formal": minimal("    for index in range(3):\n        pass\n    return value", formal="value: ac.u8, range: ac.u8", call="value, value"),
    "shadowed-late": minimal("    for index in range(3):\n        pass\n    range = value\n    return value"),
    "shadowed-target": minimal("    for range in range(3):\n        pass\n    return value"),
    "fresh-predicate": minimal("    for index in range(3):\n        if index:\n            value = value + 1\n    return value"),
    "changed-branch-annotation": minimal("    current: ac.u8 = value\n    for index in range(1):\n        if value == 0:\n            current: ac.u13 = 0\n    return current"),
    "branch-kind-conflict": minimal("    for index in range(1):\n        if value == 0:\n            joined = True\n        else:\n            joined = index\n    return joined"),
    "branch-fixed-logical-conflict": minimal("    logical = 1 if value == 0 else 2\n    for index in range(1):\n        if value == 0:\n            joined = logical\n        else:\n            joined = value\n    return joined"),
    "zero-unbound-name": minimal("    for created in range(0):\n        created = value\n    return created"),
    "nested-for": minimal("    for index in range(3):\n        for peer in range(1):\n            pass\n    return value"),
    "zero-nested-for": minimal("    for index in range(0):\n        for peer in range(1):\n            pass\n    return value"),
    "while-body": minimal("    for index in range(3):\n        while False:\n            pass\n    return value"),
    "break-body": minimal("    for index in range(3):\n        break\n    return value"),
    "continue-body": minimal("    for index in range(3):\n        continue\n    return value"),
    "for-else": minimal("    for index in range(3):\n        pass\n    else:\n        value = 1\n    return value"),
    "return-body": minimal("    for index in range(3):\n        return value\n    return value"),
    "assert-body": minimal("    for index in range(3):\n        assert value == 0\n    return value"),
    "log-body": minimal("    for index in range(3):\n        ac.log('info', 'bad', value)\n    return value"),
    "zero-log-body": minimal("    for index in range(0):\n        ac.log('info', 'bad', value)\n    return value"),
    "zero-unary-expression": minimal("    for index in range(0):\n        value = -value\n    return value"),
    "unary-minus-expression": minimal("    for index in range(1):\n        value = -value\n    return value"),
    "zero-unary-plus-expression": minimal("    for index in range(0):\n        value = +value\n    return value"),
    "unary-plus-expression": minimal("    for index in range(1):\n        value = +value\n    return value"),
    "table-method": minimal("    entries: ac.table[3, ac.u8] = (1, 2, 3)\n    for index in range(3):\n        value = entries.map(lambda lane: lane)[index]\n    return value"),
    "shift-expression": minimal("    for index in range(3):\n        value = value << 1\n    return value"),
    "division-expression": minimal("    for index in range(3):\n        value = value // 2\n    return value"),
    "later-oob": minimal("    entries: ac.table[2, ac.u8] = (1, 2)\n    for index in range(3):\n        value = entries[index]\n    return value"),
    "resource-bound": minimal("    for index in range(1000000):\n        value = value + 1\n    return value"),
    "bookkeeping-resource": minimal(bookkeeping_body),
    "literal-overflow": minimal("    for index in range(999999999999999999999999999999999999999999999999999999999999999999):\n        pass\n    return value"),
    "persistent-write": """import pycircuit as ac
from pycircuit import module, rule, struct
@struct
class Out:
    value: ac.u8
@rule
def evaluate(owner):
    for index in range(3):
        owner = owner + 1
@module
def Top() -> Out:
    owner: ac.u8 = 0
    evaluate(owner)
    return Out(value=owner)
""",
    "zero-persistent-write": """import pycircuit as ac
from pycircuit import module, rule, struct
@struct
class Out:
    value: ac.u8
@rule
def evaluate(owner):
    for index in range(0):
        owner = owner + 1
@module
def Top() -> Out:
    owner: ac.u8 = 0
    evaluate(owner)
    return Out(value=owner)
""",
    "transitive-helper-write": """import pycircuit as ac
from pycircuit import module, rule, struct
@struct
class Out:
    value: ac.u8
@rule
def write(owner):
    owner = owner + 1
@rule
def evaluate(owner):
    for index in range(3):
        write(owner)
@module
def Top() -> Out:
    owner: ac.u8 = 0
    evaluate(owner)
    return Out(value=owner)
""",
    "zero-transitive-helper-write": """import pycircuit as ac
from pycircuit import module, rule, struct
@struct
class Out:
    value: ac.u8
@rule
def write(owner):
    owner = owner + 1
@rule
def evaluate(owner):
    for index in range(0):
        write(owner)
@module
def Top() -> Out:
    owner: ac.u8 = 0
    evaluate(owner)
    return Out(value=owner)
""",
}

rejections = {}
for name, invalid in negative_cases.items():
    design.write_text(invalid)
    rejected = compile_source(unit, accepted=False, replace=True)
    assert rejected.stderr.strip(), name
    assert snapshot(unit) == protected_unit, name
    assert final.read_bytes() == protected_final, name
    assert all(snapshot(build / target) == value for target, value in protected_outputs.items()), name
    if name == "later-oob":
        assert "bounded for reachable iteration 2 failed" in rejected.stderr
        assert "table index complete width is not proven in range" in rejected.stderr
    if name in ("resource-bound", "bookkeeping-resource", "literal-overflow"):
        assert any(word in rejected.stderr.lower() for word in ("budget", "resource", "limit", "literal"))
    if name == "bookkeeping-resource":
        assert any(word in rejected.stderr.lower() for word in ("branch", "join"))
    rejections[name] = rejected.stderr
design.write_text(original)

# Loop values may feed a supported assertion outside the loop. This fixture
# proves the failed step and absence of observations; the existing system-
# execution discard gate owns direct register/clock zero-commit inspection.
system_unit = build / "system-unit"
cli(
    "compile",
    "-c",
    system_source,
    "--source-root",
    source,
    "--package-prefix",
    "bounded_for",
    "-o",
    system_unit,
)
system_final = build / "assertion-system.ac"
cli(
    "link",
    system_unit,
    "--top",
    "bounded_for.system.LoopAssertionSystem",
    "-o",
    system_final,
)
system_cpp = build / "assertion-cpp"
cli("emit", system_final, "--target", "cpp", "-o", system_cpp)
system_receipt = json.loads((system_cpp / "generated.json").read_text())
system_sources = [
    system_cpp / row["path"]
    for row in system_receipt["files"]
    if row["path"].endswith(".cpp") and row["path"] != "simulation_main.cpp"
]
system_binary = build / "assertion-runner"
run(
    [
        args.cxx,
        "-std=c++20",
        "-pthread",
        "-I" + str(repo / "include"),
        "-I" + str(system_cpp),
        system_cpp / "simulation_main.cpp",
        *system_sources,
        runtime,
        "-o",
        system_binary,
    ]
)
failed = run([system_binary, "--cycles", "4", "--workers", "1"], accepted=False)
records = [json.loads(line) for line in failed.stdout.splitlines() if line.startswith("{")]
assert not any(record.get("kind") == "log" for record in records)
terminal = [record for record in records if record.get("kind") == "result"]
assert len(terminal) == 1 and terminal[0]["status"] == "FAILED"
assert terminal[0]["error"]["code"] == "source_check_failed"
assert terminal[0]["error"]["message"] == "bounded_loop_outside_assert"
assert int(terminal[0]["epoch_time"]) == 0
assert "iteration" not in json.dumps(terminal[0]["error"])

(build / "candidate.json").write_text(
    json.dumps(
        {
            "design_sha256": hashlib.sha256(original.encode()).hexdigest(),
            "system_sha256": hashlib.sha256(original_system.encode()).hexdigest(),
            "known_and_four_state_rows": len(rows),
            "result_width": result_width,
            "workers": [1, 2],
            "verilator": True,
            "icarus_four_state": True,
            "deep32_native_workers": [1, 2],
            "deep32_verilator": True,
            "deep32_aggregate_initialization": "default, partial leaf, explicit nested one",
            "deep32_icarus": "not run in semantic gate; scaling probe tracked in issue 265",
            "negative_cases": sorted(rejections),
            "replacement_publication_unchanged": True,
            "outside_loop_assert_failed_step_no_observations": True,
            "whole_system_state_discard": "existing source_system_execution gate",
            "resource_internal_fact_maps": "not directly observable; covered only through publication boundary and diagnostics",
        },
        indent=2,
    )
    + "\n"
)
print(f"bounded-for public evidence: {build}")

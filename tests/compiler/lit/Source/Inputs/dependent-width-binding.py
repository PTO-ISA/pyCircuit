"""Resolved dependent widths stay a bounded check, and two instances stay two.

Task A drives the real source-compile CLI over an independent accept/reject
matrix: an instantiated symbolic port width may be resolved so that it compares
equal to the concrete SSA type it came from, but a missing, wrong, unclosable,
illegally spelled or nominally different actual must still be refused.

Task B drives a minimal two-instance design natively. Both counters are
observed as separate record fields in explicit Work/Xfer epochs under
workers=1 and workers=2, and the emitted RTL is checked for one register in the
module definition with two instantiations. The C++ driver is compiled inside the
harness; no new executable target is registered.
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

parser = argparse.ArgumentParser()
for option in ("repo", "source-compiler", "linker", "emitter", "opt", "cxx", "scratch"):
    parser.add_argument("--" + option, required=True)
parser.add_argument("--verilator", default=shutil.which("verilator"))
args = parser.parse_args()
repo = Path(args.repo).resolve()
fixtures = Path(__file__).resolve().parent
scratch = Path(args.scratch).resolve()
# lit's %t.dir is reused between runs; recreate every publication below.
work = scratch / "dependent-width-binding"
shutil.rmtree(work, ignore_errors=True)
source = work / "source"
source.mkdir(parents=True)
sys.path.insert(0, str(repo / "python/pycircuit/src"))
# Load the requested checkout after argument parsing, never an installed copy.
from pycircuit._source_capture import _capture_source_file  # noqa: E402
from pycircuit._source_transport import _emit_source_transport  # noqa: E402

env = dict(
    os.environ,
    PYTHONPATH=str(repo / "python/pycircuit/src"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
    PYCIRCUIT_EMITTER=args.emitter,
)
commands = []


def run(command, success=True, timeout=240):
    command = list(map(str, command))
    result = subprocess.run(
        command, env=env, cwd=repo, text=True, capture_output=True, timeout=timeout
    )
    commands.append(
        {
            "command": command,
            "exit_status": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
    )
    (work / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if success else 1), commands[-1]
    assert (
        "Assertion failed" not in result.stderr and "Traceback" not in result.stderr
    ), commands[-1]
    return result


def cli(*options, success=True):
    return run([sys.executable, "-m", "pycircuit.cli", *options], success)


def compile_source(path, output, success=True):
    # No --replace: a rejected source must not be able to publish or overwrite.
    return cli(
        "compile",
        "-c",
        path,
        "--source-root",
        source,
        "--package-prefix",
        "width_suite",
        "-o",
        output,
        success=success,
    )


def analyze(path, success=True):
    capture = work / (path.stem + ".capture.mlir")
    capture.write_text(
        _emit_source_transport(_capture_source_file(path, source_root=source))
    )
    return run(
        [
            args.opt,
            capture,
            "--ac-analyze-rule-writes",
            "-o",
            work / (path.stem + ".analyzed.mlir"),
        ],
        success,
    )


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(name, text):
    path = source / name
    path.write_text(text)
    return path


def leaf(leaf_width, payload, returned="d"):
    """One standard leaf: declared payload width `leaf_width`, module payload
    port `payload`. Acceptance needs the resolved leaf width to match."""
    return (
        "from typing import Annotated\n"
        "import pycircuit as ac\n"
        "from pycircuit import dff\n"
        "@ac.module\n"
        "def Holder(clk: bool, rst: bool, d: "
        + payload
        + ') -> {"q": '
        + payload
        + "}:\n"
        "    state = dff(T=" + leaf_width + ")\n"
        "    @ac.rule\n"
        "    def bind():\n"
        "        state(clk=clk, rst=rst, d=d, init=0)\n"
        "    bind()\n"
        '    return {"q": ' + returned + "}\n"
    )


NOMINAL_SAME = (
    "import pycircuit as ac\n"
    "from pycircuit import dff\n"
    "@ac.struct\n"
    "class Left:\n    value: ac.u8\n"
    "@ac.struct\n"
    "class Right:\n    value: ac.u8\n"
    "@ac.module\n"
    'def Holder(clk: bool, rst: bool, l: Left) -> {"q": Left}:\n'
    "    state = dff(T=Left)\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        state(clk=clk, rst=rst, d=l, init=0)\n"
    "    bind()\n"
    '    return {"q": state.q}\n'
)

NOMINAL_CROSS = NOMINAL_SAME.replace(
    'l: Left) -> {"q": Left}', 'r: Right) -> {"q": Left}'
).replace("d=l, init=0", "d=r, init=0")

TYPE_ARGUMENTS = (
    "from typing import Annotated\n"
    "import pycircuit as ac\n"
    "from pycircuit import dff\n"
    "@ac.module\n"
    "def Holder(clk: bool, rst: bool, "
    "d: Annotated[int, range(1 << (4 + 4))], "
    "e: Annotated[int, range(1 << 12)]) -> "
    '{"q": Annotated[int, range(1 << (4 + 4))], '
    '"r": Annotated[int, range(1 << 12)]}:\n'
    "    narrow = dff(T=Annotated[int, range(1 << (4 + 4))])\n"
    "    wide = dff(T=Annotated[int, range(1 << 12)])\n"
    "    @ac.rule\n"
    "    def bindnarrow():\n"
    "        narrow(clk=clk, rst=rst, d=d, init=0)\n"
    "    @ac.rule\n"
    "    def bindwide():\n"
    "        wide(clk=clk, rst=rst, d=e, init=0)\n"
    "    bindnarrow()\n"
    "    bindwide()\n"
    '    return {"q": d, "r": e}\n'
)

TYPE_ARGUMENT_MISMATCH = (
    "from typing import Annotated\n"
    "import pycircuit as ac\n"
    "from pycircuit import dff\n"
    "@ac.module\n"
    "def Holder(clk: bool, rst: bool, "
    "d: Annotated[int, range(1 << 12)]) -> "
    '{"q": Annotated[int, range(1 << 12)]}:\n'
    "    narrow = dff(T=Annotated[int, range(1 << (4 + 4))])\n"
    "    @ac.rule\n"
    "    def bindnarrow():\n"
    "        narrow(clk=clk, rst=rst, d=d, init=0)\n"
    "    bindnarrow()\n"
    '    return {"q": d}\n'
)

MISSING_ACTUAL = (
    "from typing import Annotated\n"
    "import pycircuit as ac\n"
    "from pycircuit import dff\n"
    "@ac.module\n"
    "def Holder(clk: bool, rst: bool, "
    "d: Annotated[int, range(1 << (4 + 4))]) -> "
    '{"q": Annotated[int, range(1 << (4 + 4))]}:\n'
    "    state = dff(T=Annotated[int, range(1 << (4 + 4))])\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        state(clk=clk, rst=rst, init=0)\n"
    "    bind()\n"
    '    return {"q": d}\n'
)

BOOLEAN_COMPUTED = (
    "from typing import Annotated\n"
    "import pycircuit as ac\n"
    "from pycircuit import dff\n"
    "@ac.module\n"
    "def Holder(clk: bool, rst: bool, d: bool) -> "
    '{"q": Annotated[int, range(1 << (2 - 1))]}:\n'
    "    state = dff(T=Annotated[int, range(1 << (2 - 1))])\n"
    "    @ac.rule\n"
    "    def bind():\n"
    "        state(clk=clk, rst=rst, d=d, init=0)\n"
    "    bind()\n"
    '    return {"q": state.q}\n'
)


# An accepted source must analyze, lower and publish; a rejected one must
# produce a matching diagnostic and leave no output unit behind.
def integer_to_fixed(spelling):
    return (
        "from typing import Annotated\nimport pycircuit as ac\n"
        "@ac.struct\nclass Out:\n    value: " + spelling + "\n"
        "@ac.module\ndef Top(value: Annotated[int, range(1 << 8)]) -> Out:\n"
        "    return Out(value=value)\n"
    )


accepted = {
    "integer-to-fixed-literal": integer_to_fixed("ac.u8"),
    "integer-to-fixed-computed": integer_to_fixed("ac.bits[4 + 4]"),
    "computed-width": leaf(
        "Annotated[int, range(1 << (4 + 4))]", "Annotated[int, range(1 << (4 + 4))]"
    ),
    "closed-output-computed": leaf("ac.bits[4 + 4]", "ac.u8", "state.q"),
    "closed-output-literal": leaf("ac.u8", "ac.bits[4 + 4]", "state.q"),
    "literal-width": leaf(
        "Annotated[int, range(1 << 8)]", "Annotated[int, range(1 << 8)]"
    ),
    "type-arguments": TYPE_ARGUMENTS,
    "output-port": leaf(
        "Annotated[int, range(1 << (4 + 4))]",
        "Annotated[int, range(1 << (4 + 4))]",
        "state.q",
    ),
    "legal-one-bit": leaf("Annotated[int, range(1 << (2 - 1))]", "ac.u1"),
    "nominal-same": NOMINAL_SAME,
}
rejected = {
    "missing-actual": (MISSING_ACTUAL, r"missing instance input"),
    "wrong-actual": (
        leaf("Annotated[int, range(1 << (4 + 4))]", "Annotated[int, range(1 << 9)]"),
        r"not proven within destination bounds",
    ),
    "unclosed-reference": (
        leaf("Annotated[int, range(1 << W)]", "ac.u8"),
        r"boundary|unresolved|mathematical",
    ),
    "type-argument-mismatch": (
        TYPE_ARGUMENT_MISMATCH,
        r"not proven within destination bounds",
    ),
    "output-narrow": (
        leaf(
            "Annotated[int, range(1 << 9)]",
            "Annotated[int, range(1 << (4 + 4))]",
            "state.q",
        ),
        r"not proven within destination bounds",
    ),
    "boolean-width": (
        leaf("Annotated[int, range(1 << True)]", "ac.u1"),
        r"mathematical integer",
    ),
    "zero-width": (leaf("ac.bits[0]", "ac.u1"), r"must be positive"),
    "nominal-cross": (NOMINAL_CROSS, r"boundary|nominal|struct|kind"),
    "boolean-computed": (BOOLEAN_COMPUTED, r"Boolean and Integer"),
}

for name, text in accepted.items():
    path = write(name.replace("-", "_") + ".py", text)
    analyze(path)
    unit = work / ("unit-" + name)
    compile_source(path, unit)
    assert (unit / "unit.json").is_file(), name

for name, (text, expected) in rejected.items():
    path = write(name.replace("-", "_") + ".py", text)
    absent = work / ("invalid-" + name)
    rejected_run = compile_source(path, absent, success=False)
    assert re.search(expected, rejected_run.stderr, re.I), (name, rejected_run.stderr)
    assert not absent.exists(), name + " published an output unit"

# The per-binding resolved width must reach the emitted hardware, not just the
# accept/reject decision: two leaves, two type arguments, two payload widths.
types_final = work / "type-arguments.ac"
cli(
    "link",
    work / "unit-type-arguments",
    "--top",
    "width_suite.type_arguments.Holder",
    "-o",
    types_final,
)
cli("emit", types_final, "--target", "verilog", "-o", work / "type-arguments-verilog")
types_receipt = json.loads((work / "type-arguments-verilog/generated.json").read_text())
types_rtl = "\n".join(
    (work / "type-arguments-verilog" / item["path"]).read_text()
    for item in types_receipt["files"]
    if item["role"] == "rtl"
)
leaf_widths = {
    instance: int(width)
    for _, width, instance in re.findall(
        r"(\w+) #\( \.T\(logic \[\((\d+)\)-1:0\]\) \) (pyc_instance_\w+)", types_rtl
    )
}
assert leaf_widths.get("pyc_instance_narrow") == 8, leaf_widths
assert leaf_widths.get("pyc_instance_wide") == 12, leaf_widths

# --- Task B: two instances of one stateful module own separate state --------
IDENTITY_SOURCE = """import pycircuit as ac


@ac.struct
class Counted:
    count: ac.u8


@ac.struct
class Pair:
    a: ac.u8
    b: ac.u8


@ac.rule
def advance(count, enable, delta) -> Counted:
    result = Counted(count=count)
    count = (count + delta) if enable else count
    return result


@ac.module
def Counter(enable: ac.u1, delta: ac.u8) -> Counted:
    count: ac.u8 = 0
    return advance(count, enable, delta)


@ac.module
def Top(en_a: ac.u1, en_b: ac.u1, da: ac.u8, db: ac.u8) -> Pair:
    first = Counter(en_a, da)
    second = Counter(en_b, db)
    return Pair(a=first.count, b=second.count)
"""
design = write("instance_identity.py", IDENTITY_SOURCE)
unit = work / "unit-identity"
compile_source(design, unit)
final = work / "identity.ac"
cli("link", unit, "--top", "width_suite.instance_identity.Top", "-o", final)
common = final.read_text()
# One leaf register instance inside the child definition plus two call sites.
assert common.count('"ac.instance"') == 3, common.count('"ac.instance"')
for target in ("cpp", "verilog"):
    cli("emit", final, "--target", target, "-o", work / target)

# The generated leaf definition owns exactly one register storage; the parent
# owns two distinct instance objects, one per call site.
cpp_receipt = json.loads((work / "cpp/generated.json").read_text())
cpp = [
    work / "cpp" / item["path"]
    for item in cpp_receipt["files"]
    if item["role"] == "source"
]
header = (
    work
    / "cpp"
    / next(item["path"] for item in cpp_receipt["files"] if item["role"] == "header")
)
generated = header.read_text()
assert (
    generated.count("std::make_shared<gfsim::collection_storage<::gfsim::dffe_kernel")
    == 1
), "the module definition must own exactly one register storage"
instance_members = re.findall(
    r"std::shared_ptr<::[\w:]+::pyc_family_Counter<pyc_count>> pyc_instance_(\w+);",
    generated,
)
assert len(instance_members) == 2 and len(set(instance_members)) == 2, instance_members
assert sorted(
    bytes.fromhex(name.rsplit("pyc_", 1)[1]).decode() for name in instance_members
) == ["__pyc_call_0", "__pyc_call_1"], instance_members

# The RTL must place the one register inside the module definition and
# instantiate that definition twice under distinct instance names.
verilog_receipt = json.loads((work / "verilog/generated.json").read_text())
rtl_paths = sorted(
    work / "verilog" / item["path"]
    for item in verilog_receipt["files"]
    if item["role"] == "rtl"
)
rtl = "\n".join(path.read_text() for path in rtl_paths)
assert rtl.count("dffe #(") == 1, "one register must exist in the whole design"
bodies = {
    match.group(1): match.group(0)
    for match in re.finditer(r"^module (\w+) \(.*?^endmodule", rtl, re.S | re.M)
}
leaf_modules = [name for name, body in bodies.items() if "dffe #(" in body]
assert len(leaf_modules) == 1, leaf_modules
leaf_module = leaf_modules[0]
parents = {
    name: re.findall(re.escape(leaf_module) + r"\s+pyc_instance_(\w+) \(", body)
    for name, body in bodies.items()
    if name != leaf_module
}
parents = {name: names for name, names in parents.items() if names}
assert len(parents) == 1, parents
parent_module, instances = next(iter(parents.items()))
assert len(instances) == 2 and len(set(instances)) == 2, instances
call_names = [bytes.fromhex(name.rsplit("pyc_", 1)[1]).decode() for name in instances]
assert sorted(call_names) == ["__pyc_call_0", "__pyc_call_1"], call_names
blocks = re.findall(
    re.escape(leaf_module) + r"\s+pyc_instance_\w+ \(([^;]*?)\);", rtl, re.S
)
assert len(blocks) == 2, blocks
result_nets = [re.search(r"\.result\((\w+)\)", block).group(1) for block in blocks]
assert len(set(result_nets)) == 2, result_nets

runtime = next(
    (
        path
        for path in (
            Path(args.source_compiler).resolve().parent.parent
            / "simulator/gfsim/libpyc6_runtime.a",
            Path(args.source_compiler).resolve().parent.parent
            / "lib/libpyc6_runtime.a",
        )
        if path.is_file()
    ),
    None,
)
assert runtime, "candidate Runtime archive missing"
runner = work / "runner"
run(
    [
        args.cxx,
        "-std=c++20",
        "-pthread",
        "-I" + str(repo / "include"),
        "-I" + str(work / "cpp"),
        fixtures / "dependent-width-binding.cpp",
        *cpp,
        runtime,
        "-o",
        runner,
    ]
)
traces = {}
for workers in (1, 2):
    trace = run([runner, workers]).stdout
    assert trace.splitlines()[-1] == "PASS", trace
    (work / f"native-{workers}.stdout").write_text(trace)
    traces[workers] = [line for line in trace.splitlines() if line.startswith("WORK ")]
    assert len(traces[workers]) == 23, len(traces[workers])
assert traces[1] == traces[2], "worker count changed the committed trace"
observed = [tuple(int(field) for field in line.split()[2:4]) for line in traces[1]]
assert len(set(observed)) > 6, "the two fields must not move as one value"
assert any(a != b for a, b in observed), "the two fields were never distinct"

# Exercise the new representation-only identity through real state, including
# all three four-state planes. Reuse this fixture's driver and command owner.
width_runtime = []
for case in ("closed-output-computed", "closed-output-literal"):
    final_width = work / (case + ".ac")
    cli(
        "link",
        work / ("unit-" + case),
        "--top",
        "width_suite." + case.replace("-", "_") + ".Holder",
        "-o",
        final_width,
    )
    cpp_dir, rtl_dir = work / (case + "-cpp"), work / (case + "-rtl")
    cli("emit", final_width, "--target", "cpp", "-o", cpp_dir)
    cli("emit", final_width, "--target", "verilog", "-o", rtl_dir)
    receipt = json.loads((cpp_dir / "generated.json").read_text())
    sources = [
        cpp_dir / item["path"] for item in receipt["files"] if item["role"] == "source"
    ]
    binary = work / (case + "-runner")
    run(
        [
            args.cxx,
            "-std=c++20",
            "-pthread",
            "-DCLOSED_WIDTH_IDENTITY=1",
            "-I" + str(repo / "include"),
            "-I" + str(cpp_dir),
            fixtures / "dependent-width-binding.cpp",
            *sources,
            runtime,
            "-o",
            binary,
        ]
    )
    for workers in (1, 2):
        result = run([binary, workers])
        assert result.stdout.strip() == "PASS width identity known/X/Z"
    iverilog, vvp = shutil.which("iverilog"), shutil.which("vvp")
    rtl_checked = False
    if iverilog and vvp:
        tb = work / (case + "-tb.sv")
        tb.write_text(
            """module tb;
  logic clk=0, rst=0; logic [7:0] d=0; wire [7:0] q;
  pyc_root dut(.*);
  initial begin
    #1; rst=1; clk=1; #1; clk=0; rst=0;
    d=8'h96; #1; clk=1; #1; clk=0; #1;
    if(q !== 8'h96) $fatal(1, "known width identity");
    d=8'b1010xxzz; #1; clk=1; #1; clk=0; #1;
    if(q !== 8'b1010xxzz) $fatal(1, "four-state width identity");
    $display("PASS width identity known/X/Z"); $finish;
  end
endmodule
"""
        )
        receipt = json.loads((rtl_dir / "generated.json").read_text())
        rtl_files = [
            rtl_dir / row["path"] for row in receipt["files"] if row["role"] == "rtl"
        ]
        rtl_files.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
        rtl_binary = work / (case + "-rtl-runner")
        run(
            [
                iverilog,
                "-g2012",
                "-s",
                "tb",
                "-o",
                rtl_binary,
                *sorted((repo / "include/verilog").glob("*.v")),
                *rtl_files,
                tb,
            ]
        )
        assert "PASS width identity known/X/Z" in run([vvp, rtl_binary]).stdout
        rtl_checked = True
    width_runtime.append(
        {
            "case": case,
            "native_workers": [1, 2],
            "known_x_z_identity": True,
            "rtl_four_state": rtl_checked,
        }
    )

(work / "candidate.json").write_text(
    json.dumps(
        {
            "inputs": {
                (
                    str(path.relative_to(repo))
                    if path.is_relative_to(repo)
                    else path.name
                ): digest(path)
                for path in [
                    fixtures / "dependent-width-binding.py",
                    fixtures / "dependent-width-binding.cpp",
                    fixtures.parent / "dependent-width-binding.test",
                    design,
                ]
            },
            "accepted": sorted(accepted),
            "rejected": sorted(rejected),
            "type_argument_widths": leaf_widths,
            "closed_width_runtime": width_runtime,
            "instance_identity": {
                "design": "width_suite.instance_identity.Top",
                "work_epochs": len(traces[1]),
                "workers": [1, 2],
                "rtl_leaf_module": leaf_module,
                "rtl_parent_module": parent_module,
                "rtl_instance_names": call_names,
                "rtl_result_nets": result_nets,
                "cpp_instance_members": instance_members,
                "verilator": args.verilator,
            },
        },
        indent=2,
    )
    + "\n"
)
sys.stdout.write(
    "dependent-width gate passed: resolved widths stay bounded and two module "
    "calls keep two registers\n"
)

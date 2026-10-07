"""Explicit fixed destinations widen without changing source kind or X/Z."""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("repo", "source-compiler", "linker", "emitter", "cxx", "verilator", "scratch"):
    parser.add_argument("--" + name, required=True)
for name in ("iverilog", "vvp"):
    parser.add_argument("--" + name, default=shutil.which(name))
parser.add_argument("--baseline-rejection", action="store_true")
args = parser.parse_args()
repo = Path(args.repo).resolve()
scratch = Path(args.scratch).resolve()
scratch.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, PYTHONPATH=str(repo / "python/pycircuit/src"),
           PYTHONDONTWRITEBYTECODE="1", PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
           PYCIRCUIT_LINKER=args.linker, PYCIRCUIT_EMITTER=args.emitter)
commands = []


def run(command, accepted=True):
    command = list(map(str, command))
    result = subprocess.run(command, env=env, cwd=repo, capture_output=True,
                            text=True, timeout=180)
    commands.append({"command": command, "exit_status": result.returncode,
                     "stdout": result.stdout, "stderr": result.stderr})
    (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == (0 if accepted else 1), commands[-1]
    assert "Assertion failed" not in result.stderr and "Traceback" not in result.stderr, commands[-1]
    return result


def cli(*arguments, accepted=True):
    return run([sys.executable, "-m", "pycircuit.cli", *arguments], accepted)


def snapshot(directory):
    return {path.relative_to(directory).as_posix(): path.read_bytes()
            for path in directory.rglob("*") if path.is_file()}


DESIGN = """import pycircuit as ac

@ac.struct
class WideField:
    value: ac.u32

@ac.struct
class ChildResult:
    value: ac.u9

@ac.struct
class ProbeResult:
    local: ac.u32
    constructed: ac.u32
    updated: ac.u32
    argument: ac.u9
    wide: ac.bits[65]
    alias: ac.u4
    alias_copy: ac.bits[4]

@ac.rule
def build_result(n4, n9, child) -> ProbeResult:
    local: ac.u32 = n4
    constructed = WideField(value=n4)
    updated = WideField()
    updated.value = n4
    wide: ac.bits[65] = n9
    equal_alias: ac.bits[2 + 2] = n4
    snapshot = equal_alias
    return ProbeResult(local=local, constructed=constructed.value,
                       updated=updated.value, argument=child.value, wide=wide,
                       alias=equal_alias, alias_copy=snapshot)

@ac.module
def Child(value: ac.u9) -> ChildResult:
    return ChildResult(value=value)

@ac.module
def Top(n4: ac.u4, n3: ac.u3, n9: ac.bits[9]) -> ProbeResult:
    child = Child(n3)
    return build_result(n4, n9, child)

@ac.rule
def mapped_return(value):
    return {"out": value}

@ac.module
def NamedFixed(value: ac.u3) -> {"out": ac.bits[9]}:
    return mapped_return(value)
"""

CPP = r'''#include "gfsim/SimExecutor.h"
#include "pycircuit_system.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

void require(bool ok) { if (!ok) std::abort(); }
template<unsigned W> auto known(unsigned value) {
  return gfsim::wire<gfsim::Bits<W>>::known(gfsim::Bits<W>{value});
}
template<unsigned W> auto fromText(std::string_view bits) {
  require(bits.size() == W);
  unsigned value = 0, knownMask = 0, zMask = 0;
  for (unsigned index = 0; index != W; ++index) {
    const unsigned bit = 1u << (W - index - 1);
    // Set a latent one behind X/Z as well: native transport must preserve all planes.
    if (bits[index] != '0') value |= bit;
    if (bits[index] == '0' || bits[index] == '1') knownMask |= bit;
    if (bits[index] == 'z') zMask |= bit;
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(
      gfsim::Bits<W>{value}, gfsim::Bits<W>{knownMask}, gfsim::Bits<W>{zMask}));
}
template<unsigned Out, unsigned In, typename Packet>
void check(const Packet &packet, unsigned low, const gfsim::wire<gfsim::Bits<In>> &input) {
  const auto actual = gfsim::extract<Out>(packet.packed(), low);
  const auto &expected = input.packed();
  for (unsigned bit = 0; bit != Out; ++bit) {
    require(actual.value().bit(bit) == (bit < In && expected.value().bit(bit)));
    require(actual.knownMask().bit(bit) == (bit >= In || expected.knownMask().bit(bit)));
    require(actual.zMask().bit(bit) == (bit < In && expected.zMask().bit(bit)));
  }
}
template<unsigned W, typename Packet> std::string text(const Packet &packet) {
  const auto &packed = packet.packed();
  std::string result;
  for (unsigned index = 0; index != W; ++index) {
    const unsigned bit = W - index - 1;
    result += packed.zMask().bit(bit) ? 'z' : !packed.knownMask().bit(bit) ? 'x'
                                          : packed.value().bit(bit) ? '1' : '0';
  }
  return result;
}
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(static_cast<unsigned>(std::stoul(argv[1])));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                                config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  auto sample = [&](const pyc_dut::Inputs &inputs) {
    dut.drive(inputs);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
#ifdef NAMED_FIXED
    const auto output = dut.sample().out;
    check<9, 3>(output, 0, inputs.value);
#else
    const auto output = dut.sample().result;
    check<32, 4>(output, 146, inputs.n4);
    check<32, 4>(output, 114, inputs.n4);
    check<32, 4>(output, 82, inputs.n4);
    check<9, 3>(output, 73, inputs.n3);
    check<65, 9>(output, 8, inputs.n9);
    check<4, 4>(output, 4, inputs.n4);
    check<4, 4>(output, 0, inputs.n4);
#endif
    return output;
  };
#ifdef NAMED_FIXED
  for (unsigned value = 0; value != 8; ++value) {
    pyc_dut::Inputs inputs;
    inputs.value = known<3>(value);
    std::cout << "WORK " << text<9>(sample(inputs)) << '\n';
  }
  for (const std::string_view value : {"xz1", "zzz"}) {
    pyc_dut::Inputs inputs;
    inputs.value = fromText<3>(value);
    std::cout << "MASK " << text<9>(sample(inputs)) << '\n';
    inputs.value = known<3>(7);
    require(sample(inputs).isFullyKnown());
  }
#else
  constexpr unsigned wideValues[] = {0, 1, 511, 256, 341, 170, 7, 255,
                                     128, 510, 9, 64, 33, 3, 400, 17};
  for (unsigned value = 0; value != 16; ++value) {
    pyc_dut::Inputs inputs;
    inputs.n4 = known<4>(value);
    inputs.n3 = known<3>(value % 8);
    inputs.n9 = known<9>(wideValues[value]);
    std::cout << "WORK " << text<178>(sample(inputs)) << '\n';
  }
  struct UnknownCase { std::string_view n4, n3, n9; };
  constexpr UnknownCase cases[] = {{"1x0z", "zx1", "1zx0010xz"},
                                   {"xxxx", "xxx", "xxxxxxxxx"},
                                   {"zzzz", "zzz", "zzzzzzzzz"},
                                   {"0z10", "101", "0z101x010"}};
  for (const auto &row : cases) {
    pyc_dut::Inputs inputs;
    inputs.n4 = fromText<4>(row.n4);
    inputs.n3 = fromText<3>(row.n3);
    inputs.n9 = fromText<9>(row.n9);
    std::cout << "MASK " << text<178>(sample(inputs)) << '\n';
    inputs.n4 = known<4>(15);
    inputs.n3 = known<3>(7);
    inputs.n9 = known<9>(511);
    require(sample(inputs).isFullyKnown());
  }
#endif
}
'''

RTL = '''module tb;
`ifdef NAMED_FIXED
  logic [2:0] value;
  wire [8:0] out;
  pyc_root dut(.*);
  task row(input logic [2:0] v,input bit unknown_case);
    value=v; #1;
    if (out !== {6'b0,v}) $fatal(1,"named fixed return zero extension failed");
    if (unknown_case) $display("MASK %b",out);
    else $display("WORK %b",out);
  endtask
  initial begin
    for(integer index=0;index<8;index=index+1) row(index[2:0],0);
`ifdef WIDENING_FOUR_STATE
    row(3'bxz1,1); row(3'bzzz,1);
    value=3'd7; #1;
    if(out !== 9'd7) $fatal(1,"named fixed immediate recovery failed");
`endif
    $finish;
  end
`else
  logic [3:0] n4;
  logic [2:0] n3;
  logic [8:0] n9;
  wire [177:0] result;
  logic [177:0] expected;
  pyc_root dut(.*);
  function logic [8:0] wide_value(input integer index);
    case(index)
      0:wide_value=0; 1:wide_value=1; 2:wide_value=511; 3:wide_value=256;
      4:wide_value=341; 5:wide_value=170; 6:wide_value=7; 7:wide_value=255;
      8:wide_value=128; 9:wide_value=510; 10:wide_value=9; 11:wide_value=64;
      12:wide_value=33; 13:wide_value=3; 14:wide_value=400; 15:wide_value=17;
    endcase
  endfunction
  task row(input logic [3:0] a,input logic [2:0] b,input logic [8:0] c,input bit unknown_case);
    n4=a; n3=b; n9=c; #1;
    expected={28'b0,a,28'b0,a,28'b0,a,6'b0,b,56'b0,c,a,a};
    if(result !== expected) $fatal(1,"explicit widening high-zero/low-XZ/alias golden failed");
    if(unknown_case) $display("MASK %b",result);
    else $display("WORK %b",result);
  endtask
  initial begin
    for(integer index=0;index<16;index=index+1)
      row(index[3:0],index[2:0],wide_value(index),0);
`ifdef WIDENING_FOUR_STATE
    row(4'b1x0z,3'bzx1,9'b1zx0010xz,1);
    row(4'bxxxx,3'bxxx,9'bxxxxxxxxx,1);
    row(4'bzzzz,3'bzzz,9'bzzzzzzzzz,1);
    row(4'b0z10,3'b101,9'b0z101x010,1);
    n4=15; n3=7; n9=511; #1;
    expected={28'b0,n4,28'b0,n4,28'b0,n4,6'b0,n3,56'b0,n9,n4,n4};
    if(result !== expected) $fatal(1,"explicit widening immediate recovery failed");
`endif
    $finish;
  end
`endif
endmodule
'''

PREFIX = "import pycircuit as ac\n"
CASES = {
    "shrinking-small-mask": PREFIX + """@ac.struct
class Result:
    value: ac.u4
@ac.module
def Top(value: ac.u9) -> Result:
    return Result(value=value & 1)
""",
    "shrinking-rule-local": PREFIX + """@ac.struct
class Result:
    value: ac.u4
@ac.rule
def evaluate(value) -> Result:
    small: ac.u4 = value & 1
    return Result(value=small)
@ac.module
def Top(value: ac.u9) -> Result:
    return evaluate(value)
""",
    "mixed-arithmetic": PREFIX + """@ac.struct
class Result:
    value: ac.u9
@ac.module
def Top(left: ac.u3, right: ac.u9) -> Result:
    return Result(value=left + right)
""",
    "boolean-to-wide": PREFIX + """@ac.struct
class Result:
    value: ac.u9
@ac.module
def Top(left: ac.u3, right: ac.u3) -> Result:
    return Result(value=left == right)
""",
    "fixed-to-math": "from typing import Annotated\n" + PREFIX + """@ac.rule
def mapped_return(value):
    return {"out": value}
@ac.module
def Top(value: ac.u3) -> {"out": Annotated[int, range(1 << 9)]}:
    return mapped_return(value)
""",
    "structural-named": PREFIX + """@ac.module
def Top(value: ac.u3) -> {"out": ac.bits[9]}:
    return {"out": value}
""",
    "rule-formal-width": PREFIX + """@ac.struct
class Result:
    value: ac.u9
@ac.rule
def evaluate(value: ac.u9) -> Result:
    return Result(value=value)
@ac.module
def Top(value: ac.u3) -> Result:
    return evaluate(value)
""",
    "nominal-mismatch": PREFIX + """@ac.struct
class Left:
    value: ac.u4
@ac.struct
class Right:
    value: ac.u4
@ac.struct
class Result:
    nested: Left
@ac.module
def Top(value: ac.u4) -> Result:
    return Result(nested=Right(value=value))
""",
    "runtime-boolean-fixed-peer": PREFIX + """@ac.struct
class Result:
    value: ac.u9
@ac.module
def Top(value: ac.u1, choose: ac.u1) -> Result:
    unproved = value if choose else (value == value)
    return Result(value=unproved)
""",
}

with tempfile.TemporaryDirectory(prefix="fixed-widening-", dir=scratch) as temporary:
    build = Path(temporary)
    source = build / "source"
    source.mkdir()
    design = source / "design.py"
    design.write_text(DESIGN)
    (scratch / "design.py").write_text(DESIGN)
    (scratch / "fixture.sha256").write_text(hashlib.sha256(Path(__file__).read_bytes()).hexdigest() + "\n")
    unit = build / "unit"

    def compile_source(path, output, accepted=True, replace=False, interfaces=()):
        arguments = ["compile", "-c", path, "--source-root", source,
                     "--package-prefix", "widening", "-o", output]
        if replace:
            arguments.append("--replace")
        for interface in interfaces:
            arguments.extend(["-I", interface])
        return cli(*arguments, accepted=accepted)

    if args.baseline_rejection:
        rejected = compile_source(design, unit, accepted=False)
        assert "unsigned boundary hardware type mismatch" in rejected.stderr, rejected.stderr
        assert not unit.exists()
        sys.stdout.write("fixed widening pre-fix baseline rejected positive destination widening\n")
        sys.exit(0)
    compile_source(design, unit)
    toolroot = Path(args.source_compiler).resolve().parent.parent
    runtime = next((path for path in (toolroot / "simulator/gfsim/libpyc6_runtime.a",
                                     toolroot / "lib/libpyc6_runtime.a") if path.is_file()), None)
    assert runtime is not None, "Runtime archive missing from this build/install"

    products = []
    for top, named, frames, masks in (("Top", False, 16, 4), ("NamedFixed", True, 8, 2)):
        output = build / top
        output.mkdir()
        final = output / "design_top.ac"
        cli("link", unit, "--top", "widening.design." + top, "-o", final)
        for target in ("cpp", "verilog"):
            cli("emit", final, "--target", target, "-o", output / target)
        receipt = json.loads((output / "cpp/generated.json").read_text())
        cpp = [output / "cpp" / row["path"] for row in receipt["files"]
               if row["path"].endswith(".cpp")]
        assert cpp, "Generated source-owned C++ translation unit missing"
        runner_source = output / "runner.cpp"
        runner_source.write_text(CPP)
        runner = output / "runner"
        defines = ["-DNAMED_FIXED"] if named else []
        run([args.cxx, "-std=c++20", "-pthread", *defines, "-I" + str(repo / "include"),
             "-I" + str(output / "cpp"), runner_source, *cpp, runtime, "-o", runner])
        traces = []
        mask_traces = []
        for workers in (1, 2):
            trace = run([runner, str(workers)]).stdout
            (scratch / f"{top}-workers-{workers}.stdout").write_text(trace)
            traces.append([row for row in trace.splitlines() if row.startswith("WORK ")])
            mask_traces.append([row for row in trace.splitlines() if row.startswith("MASK ")])
        assert len(traces[0]) == frames and traces[0] == traces[1]
        assert len(mask_traces[0]) == masks and mask_traces[0] == mask_traces[1]
        receipt = json.loads((output / "verilog/generated.json").read_text())
        rtl = [output / "verilog" / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
        rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
        bench = output / "tb.sv"
        bench.write_text(RTL)
        rtl_build = output / "rtl-build"
        run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vwidening",
             "--Mdir", rtl_build, "-j", "2", "-Wno-fatal", *defines, *rtl, bench])
        rtl_trace = run([rtl_build / "Vwidening"]).stdout
        (scratch / f"{top}-rtl.stdout").write_text(rtl_trace)
        assert [row for row in rtl_trace.splitlines() if row.startswith("WORK ")] == traces[0]
        if args.iverilog and args.vvp:
            rtl_runner = output / "rtl-four-state"
            run([args.iverilog, "-g2012", "-DWIDENING_FOUR_STATE", *defines, "-s", "tb",
                 "-o", rtl_runner, *rtl, bench])
            four_state_trace = run([args.vvp, rtl_runner]).stdout
            (scratch / f"{top}-rtl-four-state.stdout").write_text(four_state_trace)
            assert [row for row in four_state_trace.splitlines() if row.startswith("WORK ")] == traces[0]
            assert [row for row in four_state_trace.splitlines() if row.startswith("MASK ")] == mask_traces[0]
        products.extend([final, output / "cpp", output / "verilog"])

    protected_unit = snapshot(unit)
    protected_products = {path: snapshot(path) if path.is_dir() else path.read_bytes()
                          for path in products}
    for name, text in CASES.items():
        design.write_text(text)
        (scratch / (name + ".py")).write_text(text)
        absent = build / ("bad-" + name)
        rejected = compile_source(design, absent, accepted=False)
        assert any(word in rejected.stderr.lower() for word in
                   ("width", "narrow", "type", "fixed", "provenance", "boolean", "integer", "kind")), rejected.stderr
        if name == "runtime-boolean-fixed-peer":
            assert "fixed branch peer requires a closed source Integer or Boolean constant" in rejected.stderr, rejected.stderr
        assert not absent.exists()
        compile_source(design, unit, accepted=False, replace=True)
        assert snapshot(unit) == protected_unit
        assert all((snapshot(path) if path.is_dir() else path.read_bytes()) == before
                   for path, before in protected_products.items())

sys.stdout.write("fixed widening gate passed: 24 C++ worker-1/2 and RTL frames; "
                 "6 native X/Z frames; 18 protected rejections; " +
                 ("6 Icarus X/Z frames\n" if args.iverilog and args.vvp else "Icarus X/Z unavailable\n"))

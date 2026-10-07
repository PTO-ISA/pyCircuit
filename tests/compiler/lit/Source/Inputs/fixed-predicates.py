"""Independent Boolean facts, fixed-bit boundaries and protected rejections."""

import argparse
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
class Flag:
    value: ac.u1

@ac.struct
class ProbeResult:
    inline: ac.u2
    selected: ac.u2
    wrapped: ac.u1
    direct: ac.u2
    and_left: ac.u2
    and_right: ac.u2
    or_left: ac.u2
    or_right: ac.u2
    xor_left: ac.u2
    xor_right: ac.u2
    and_next_left: ac.u1
    and_next_right: ac.u1
    or_next_left: ac.u1
    or_next_right: ac.u1
    xor_next_left: ac.u1
    xor_next_right: ac.u1

@ac.module
def Top(x: ac.u2, y: ac.u2, condition: ac.u1) -> ProbeResult:
    inline = 1 if x == y else 3
    masked = x & 3
    equal = masked == 1
    selected = 1 if equal else 3
    flag = Flag(value=x == y)
    wrapped = flag.value + 1
    direct = 3 if condition else 1
    predicate = x == y
    and_left = predicate & condition
    and_right = condition & predicate
    or_left = predicate | condition
    or_right = condition | predicate
    xor_left = predicate ^ condition
    xor_right = condition ^ predicate
    return ProbeResult(
        inline=inline, selected=selected, wrapped=wrapped, direct=direct,
        and_left=1 if and_left else 3, and_right=1 if and_right else 3,
        or_left=1 if or_left else 3, or_right=1 if or_right else 3,
        xor_left=1 if xor_left else 3, xor_right=1 if xor_right else 3,
        and_next_left=and_left + 1, and_next_right=and_right + 1,
        or_next_left=or_left + 1, or_next_right=or_right + 1,
        xor_next_left=xor_left + 1, xor_next_right=xor_right + 1)
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
  unsigned value = 0, mask = 0, z = 0;
  for (unsigned index = 0; index != W; ++index) {
    const unsigned bit = 1u << (W - index - 1);
    if (bits[index] == '0' || bits[index] == '1') mask |= bit;
    if (bits[index] == '1') value |= bit;
    if (bits[index] == 'z') z |= bit;
  }
  return gfsim::wire<gfsim::Bits<W>>::fromPacked(gfsim::FourState<W>::fromMasks(
      gfsim::Bits<W>{value}, gfsim::Bits<W>{mask}, gfsim::Bits<W>{z}));
}
int main(int argc, char **argv) {
  require(argc == 2);
  pyc_dut dut(static_cast<unsigned>(std::stoul(argv[1])));
  gfsim::SimExecutor executor(dut.system(), dut.observations(), {});
  constexpr std::string_view config = "{}";
  require(executor.ConfigureJson(reinterpret_cast<const std::uint8_t *>(config.data()),
                                config.size()) == PYCIRCUIT_MODEL_STATUS_V1_OK);
  require(executor.Reset() == PYCIRCUIT_MODEL_STATUS_V1_OK);
  // All two-bit input pairs and both independent one-bit controls.
  for (unsigned x = 0; x != 4; ++x)
    for (unsigned y = 0; y != 4; ++y)
      for (unsigned condition = 0; condition != 2; ++condition) {
        pyc_dut::Inputs inputs;
        inputs.x = known<2>(x);
        inputs.y = known<2>(y);
        inputs.condition = known<1>(condition);
        dut.drive(inputs);
        PycircuitModelStepResultV1 status{sizeof(status)};
        require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
        require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
        const auto output = dut.sample().result;
        const auto inline_value = gfsim::wire<gfsim::Bits<2>>::fromPacked(
            gfsim::extract<2>(output.packed(), 23));
        const auto selected = gfsim::wire<gfsim::Bits<2>>::fromPacked(
            gfsim::extract<2>(output.packed(), 21));
        const auto wrapped = gfsim::wire<gfsim::Bits<1>>::fromPacked(
            gfsim::extract<1>(output.packed(), 20));
        const auto direct = gfsim::wire<gfsim::Bits<2>>::fromPacked(
            gfsim::extract<2>(output.packed(), 18));
        // Equality's explicit u1 value plus one wraps: equal -> 0, unequal -> 1.
        require(inline_value.isFullyKnown() && inline_value.value().value() == (x == y ? 1u : 3u));
        require(selected.isFullyKnown() && selected.value().value() == (x == 1 ? 1u : 3u));
        require(wrapped.isFullyKnown() && wrapped.value().value() == (x == y ? 0u : 1u));
        require(direct.isFullyKnown() && direct.value().value() == (condition ? 3u : 1u));
        const bool predicate = x == y;
        const bool mixed[] = {predicate && condition, predicate && condition,
                              predicate || condition, predicate || condition,
                              predicate != bool(condition), predicate != bool(condition)};
        unsigned selections[6], increments[6];
        for (unsigned index = 0; index != 6; ++index) {
          const auto selection = gfsim::extract<2>(output.packed(), 16 - 2 * index);
          const auto increment = gfsim::extract<1>(output.packed(), 5 - index);
          require(selection.isFullyKnown() && increment.isFullyKnown());
          selections[index] = static_cast<unsigned>(selection.value().value());
          increments[index] = static_cast<unsigned>(increment.value().value());
          require(selections[index] == (mixed[index] ? 1u : 3u));
          require(increments[index] == (mixed[index] ? 0u : 1u));
        }
        std::cout << "WORK " << inline_value.value().value() << ' '
                  << selected.value().value() << ' '
                  << wrapped.value().value() << ' ' << direct.value().value();
        for (const auto selection : selections) std::cout << ' ' << selection;
        for (const auto increment : increments) std::cout << ' ' << increment;
        std::cout << '\n';
      }
  struct UnknownCase { std::string_view x, y, condition, expected; };
  // Independent bitwise annihilators and mux merging: unknown selects produce x1;
  // u1 arithmetic on unknown produces x. Both operand orders share each golden.
  constexpr UnknownCase cases[] = {
    {"01", "01", "x", "x1x10101x1x1xx00xx"},
    {"01", "01", "z", "x1x10101x1x1xx00xx"},
    {"01", "00", "x", "1111x1x1x1x111xxxx"},
    {"01", "00", "z", "1111x1x1x1x111xxxx"},
    {"0x", "01", "0", "1111x1x1x1x111xxxx"},
    {"0z", "01", "0", "1111x1x1x1x111xxxx"},
    {"0x", "01", "1", "x1x10101x1x1xx00xx"},
    {"0z", "01", "1", "x1x10101x1x1xx00xx"},
    {"0x", "01", "x", "x1x1x1x1x1x1xxxxxx"},
    {"0z", "01", "z", "x1x1x1x1x1x1xxxxxx"}
  };
  for (const auto &row : cases) {
    pyc_dut::Inputs inputs;
    inputs.x = fromText<2>(row.x);
    inputs.y = fromText<2>(row.y);
    inputs.condition = fromText<1>(row.condition);
    dut.drive(inputs);
    PycircuitModelStepResultV1 status{sizeof(status)};
    require(executor.Step(&status) == PYCIRCUIT_MODEL_STATUS_V1_OK);
    require(status.state == PYCIRCUIT_MODEL_STEP_V1_RUNNING);
    const auto mixed = gfsim::extract<18>(dut.sample().result.packed(), 0);
    std::string actual;
    for (unsigned index = 0; index != 18; ++index) {
      const unsigned bit = 17 - index;
      actual += mixed.zMask().bit(bit) ? 'z' : !mixed.knownMask().bit(bit) ? 'x'
                                        : mixed.value().bit(bit) ? '1' : '0';
    }
    require(actual == row.expected);
    std::cout << "MASK " << row.x << ' ' << row.y << ' ' << row.condition << ' ' << actual << '\n';
  }
}
'''

RTL = '''module tb;
  logic [1:0] x, y;
  logic condition;
  wire [24:0] result;
  pyc_root dut(.*);
  logic [5:0] mixed;
  task masks(input logic [1:0] xv,yv,input logic control,input logic [17:0] expected);
    x=xv; y=yv; condition=control; #1;
    if (result[17:0] !== expected) $fatal(1,"mixed Boolean/u1 X/Z golden failed");
    $display("MASK %b %b %b %b",x,y,condition,result[17:0]);
  endtask
  initial begin
    for (integer xv=0; xv<4; xv=xv+1)
      for (integer yv=0; yv<4; yv=yv+1)
        for (integer control=0; control<2; control=control+1) begin
          x=xv[1:0]; y=yv[1:0]; condition=control[0]; #1;
          if (!(result[24:23] === (xv==yv ? 2'd1 : 2'd3) &&
                result[22:21] === (xv==1 ? 2'd1 : 2'd3) &&
                result[20] === (xv==yv ? 1'b0 : 1'b1) &&
                result[19:18] === (control ? 2'd3 : 2'd1)))
            $fatal(1,"fixed predicate kind/boundary truth table failed");
          mixed={(xv==yv && control!=0),(xv==yv && control!=0),
                 (xv==yv || control!=0),(xv==yv || control!=0),
                 ((xv==yv)!=(control!=0)),((xv==yv)!=(control!=0))};
          for(integer index=0;index<6;index=index+1) begin
            if (!(result[17-2*index -:2] === (mixed[5-index] ? 2'd1 : 2'd3) &&
                  result[5-index] === (mixed[5-index] ? 1'b0 : 1'b1)))
              $fatal(1,"mixed bitwise operand-order/selection/arithmetic golden failed");
          end
          $write("WORK %0d %0d %0d %0d",result[24:23],result[22:21],result[20],result[19:18]);
          for(integer index=0;index<6;index=index+1) $write(" %0d",result[17-2*index -:2]);
          for(integer index=0;index<6;index=index+1) $write(" %0d",result[5-index]);
          $write("\\n");
        end
`ifdef PREDICATES_FOUR_STATE
    masks(2'b01,2'b01,1'bx,18'bx1x10101x1x1xx00xx);
    masks(2'b01,2'b01,1'bz,18'bx1x10101x1x1xx00xx);
    masks(2'b01,2'b00,1'bx,18'b1111x1x1x1x111xxxx);
    masks(2'b01,2'b00,1'bz,18'b1111x1x1x1x111xxxx);
    masks(2'b0x,2'b01,1'b0,18'b1111x1x1x1x111xxxx);
    masks(2'b0z,2'b01,1'b0,18'b1111x1x1x1x111xxxx);
    masks(2'b0x,2'b01,1'b1,18'bx1x10101x1x1xx00xx);
    masks(2'b0z,2'b01,1'b1,18'bx1x10101x1x1xx00xx);
    masks(2'b0x,2'b01,1'bx,18'bx1x1x1x1x1x1xxxxxx);
    masks(2'b0z,2'b01,1'bz,18'bx1x1x1x1x1x1xxxxxx);
`endif
    $finish;
  end
endmodule
'''

toolroot = Path(args.source_compiler).resolve().parent.parent
runtime = next((path for path in (toolroot / "simulator/gfsim/libpyc6_runtime.a",
                                 toolroot / "lib/libpyc6_runtime.a") if path.is_file()), None)
assert runtime is not None, "Runtime archive missing from this build/install"
with tempfile.TemporaryDirectory(prefix="fixed-predicates-", dir=scratch) as temporary:
    build = Path(temporary)
    source = build / "source"
    source.mkdir()
    design = source / "design.py"
    design.write_text(DESIGN)
    unit = build / "unit"

    def compile_source(output, accepted=True, replace=False):
        arguments = ["compile", "-c", design, "--source-root", source,
                     "--package-prefix", "fixed_predicates", "-o", output]
        if replace:
            arguments.append("--replace")
        return cli(*arguments, accepted=accepted)

    compile_source(unit)
    final = build / "design_top.ac"
    cli("link", unit, "--top", "fixed_predicates.design.Top", "-o", final)
    for target in ("cpp", "verilog"):
        cli("emit", final, "--target", target, "-o", build / target)
    receipt = json.loads((build / "cpp/generated.json").read_text())
    cpp = [build / "cpp" / row["path"] for row in receipt["files"]
           if row["path"].endswith(".cpp")]
    assert cpp, "Generated source-owned C++ translation unit missing"
    runner_source = build / "runner.cpp"
    runner_source.write_text(CPP)
    runner = build / "runner"
    run([args.cxx, "-std=c++20", "-pthread", "-I" + str(repo / "include"),
         "-I" + str(build / "cpp"), runner_source, *cpp, runtime, "-o", runner])
    traces = []
    mask_traces = []
    for workers in (1, 2):
        trace = run([runner, str(workers)]).stdout
        (scratch / f"workers-{workers}.stdout").write_text(trace)
        traces.append([row for row in trace.splitlines() if row.startswith("WORK ")])
        mask_traces.append([row for row in trace.splitlines() if row.startswith("MASK ")])
    assert len(traces[0]) == 32 and traces[0] == traces[1]
    assert len(mask_traces[0]) == 10 and mask_traces[0] == mask_traces[1]
    receipt = json.loads((build / "verilog/generated.json").read_text())
    rtl = [build / "verilog" / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
    rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
    bench = build / "tb.sv"
    bench.write_text(RTL)
    rtl_build = build / "rtl-build"
    run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vpredicates",
         "--Mdir", rtl_build, "-j", "2", "-Wno-fatal", *rtl, bench])
    rtl_trace = run([rtl_build / "Vpredicates"]).stdout
    (scratch / "rtl.stdout").write_text(rtl_trace)
    assert [row for row in rtl_trace.splitlines() if row.startswith("WORK ")] == traces[0]

    if args.iverilog and args.vvp:
        rtl_runner = build / "rtl-four-state"
        run([args.iverilog, "-g2012", "-DPREDICATES_FOUR_STATE", "-s", "tb",
             "-o", rtl_runner, *rtl, bench])
        four_state_trace = run([args.vvp, rtl_runner]).stdout
        (scratch / "rtl-four-state.stdout").write_text(four_state_trace)
        assert [row for row in four_state_trace.splitlines() if row.startswith("WORK ")] == traces[0]
        assert [row for row in four_state_trace.splitlines() if row.startswith("MASK ")] == mask_traces[0]

    before = snapshot(unit)
    final_before = final.read_bytes()
    emitted_before = {target: snapshot(build / target) for target in ("cpp", "verilog")}
    cases = {
        "boolean-arithmetic": (DESIGN.replace("selected: ac.u2", "selected: ac.u1")
                               .replace("selected = 1 if equal else 3",
                                        "selected = (x == y) + 1"),
                               ("boolean", "kind", "arithmetic")),
        "boolean-and-arithmetic": (DESIGN.replace("selected: ac.u2", "selected: ac.u1")
                                   .replace("selected = 1 if equal else 3",
                                            "selected = ((x == y) & (x == 1)) + 1"),
                                   ("boolean", "kind", "arithmetic")),
        "boolean-or-arithmetic": (DESIGN.replace("selected: ac.u2", "selected: ac.u1")
                                  .replace("selected = 1 if equal else 3",
                                           "selected = ((x == y) | (x == 1)) + 1"),
                                  ("boolean", "kind", "arithmetic")),
        "boolean-xor-arithmetic": (DESIGN.replace("selected: ac.u2", "selected: ac.u1")
                                   .replace("selected = 1 if equal else 3",
                                            "selected = ((x == y) ^ (x == 1)) + 1"),
                                   ("boolean", "kind", "arithmetic")),
        "wide-condition": (DESIGN.replace("condition: ac.u1", "condition: ac.u2"),
                           ("boolean", "condition", "one-bit", "kind")),
    }
    for name, (text, diagnostics) in cases.items():
        design.write_text(text)
        absent = build / ("bad-" + name)
        rejected = compile_source(absent, accepted=False)
        assert any(word in rejected.stderr.lower() for word in diagnostics), rejected.stderr
        assert not absent.exists()
        compile_source(unit, accepted=False, replace=True)
        assert snapshot(unit) == before
        assert final.read_bytes() == final_before
        assert all(snapshot(build / target) == emitted_before[target] for target in emitted_before)

sys.stdout.write("fixed predicates gate passed: 32 C++ worker-1/2 and RTL frames; "
                 "10 native X/Z frames; 10 protected rejections; " +
                 ("10 Icarus X/Z frames\n" if args.iverilog and args.vvp else "Icarus X/Z unavailable\n"))

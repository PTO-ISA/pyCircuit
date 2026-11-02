#!/usr/bin/env python3
"""Replay independently checked external stimuli against the mapped RTL.

The test adapter contains clock/reset, input vectors and exact port comparisons,
no instruction decoder or CPU calculation. The source trace must first pass the
separate Spike/spec retirement, GPR and RAM verifier.
"""
from __future__ import annotations
import argparse
import json
import re
from pathlib import Path
import subprocess


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ["generated", "mapped", "trace", "cells", "primitives", "output"]:
        p.add_argument("--" + name, type=Path, required=True)
    p.add_argument("--netlist-json", type=Path)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    check = json.loads((a.trace / "verification.json").read_text())
    if check["status"] != "PASS" or check["basis"] != "spike":
        raise ValueError("verified independent-reference trace required")
    ports = json.loads((a.generated / "ports.json").read_text())
    receipt = json.loads((a.generated / "generated.json").read_text())
    domains = ports["domains"]
    incoming = next(
        row for row in ports["inputs"] if row["ordinal"] not in domains.values()
    )
    packet, state = ports["outputs"]
    clock, reset = [
        ports["inputs"][domains[name]]["wire"] for name in ["clock", "reset"]
    ]
    vectors = (a.trace / "inputs.mem").read_text().splitlines()
    results = (a.trace / "outputs.mem").read_text().splitlines()
    if len(vectors) != len(results) or not vectors:
        raise ValueError("incomplete stimulus/expected pair")
    if check["executed_ticks"] != check["model_cycles"]:
        raise ValueError("use a full timing trace for cycle replay")
    final = (a.trace / "state.bits").read_text().strip()
    (a.output / "inputs.mem").write_text("\n".join(vectors) + "\n")
    (a.output / "outputs.mem").write_text("\n".join(results) + "\n")
    count = len(vectors)
    logical = {row["wire"]: row for row in ports["inputs"] + ports["outputs"]}
    bindings = []
    if a.netlist_json:
        physical = json.loads(a.netlist_json.read_text())["modules"][
            receipt["root_rtl_name"]
        ]["ports"]
        for name, pin in physical.items():
            if name in logical:
                expression = name
            else:
                match = re.fullmatch(r"(.+)\[(\d+)\]", name)
                if not match or match[1] not in logical or len(pin["bits"]) != 1:
                    raise ValueError("unrecognized physical scalar port")
                expression = f"{match[1]}[{int(match[2])}]"
            identifier = (
                name
                if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name)
                else "\\" + name + " "
            )
            bindings.append(f".{identifier}({expression})")
    else:
        bindings = [f".{name}({name})" for name in logical]
    tb = f"""`timescale 1ns/1ps
module mapped_test;
reg {clock}=0,{reset}=1;
reg [{incoming['width']-1}:0] {incoming['wire']}=0;
wire [{packet['width']-1}:0] {packet['wire']};
wire [{state['width']-1}:0] {state['wire']};
{receipt['root_rtl_name']} dut({','.join(bindings)});
reg [{incoming['width']-1}:0] stimuli[0:{count-1}];
reg [{packet['width']-1}:0] expected[0:{count-1}];
integer cycle;
initial begin
 $readmemb("inputs.mem",stimuli);$readmemb("outputs.mem",expected);
 // ASIC contract requires reset. The generous test clock is functional-only,
 // without a frequency or static-timing claim.
 repeat(2)begin #1000;{clock}=1;#1000;{clock}=0;#1000;end
 {reset}=0;
 for(cycle=0;cycle<{count};cycle=cycle+1)begin
  if(cycle % 64 == 0)begin $display("MAPPED_PROGRESS cycle=%0d",cycle);$fflush();end
  {incoming['wire']}=stimuli[cycle];#1000;
  if({packet['wire']}!==expected[cycle])begin
   $display("MAPPED_DIFFER cycle=%0d expected=%h actual=%h",cycle,expected[cycle],{packet['wire']});$fatal(1,"mapped output differs");end
  {clock}=1;#1000;{clock}=0;#1000;
 end
 if({state['wire']}!=={state['width']}'b{final})$fatal(1,"mapped final complete state differs");
 $display("MAPPED_PASS cycles={count} retired={check['retired']} independent_reference=Spike");$finish;
end
endmodule
"""
    (a.output / "mapped_test.sv").write_text(tb)
    command = [
        "iverilog",
        "-g2012",
        "-DFUNCTIONAL",
        "-DUNIT_DELAY=#1",
        "-s",
        "mapped_test",
        "-o",
        str((a.output / "mapped.vvp").resolve()),
        str(a.mapped.resolve()),
        str(a.primitives.resolve()),
        str(a.cells.resolve()),
        str((a.output / "mapped_test.sv").resolve()),
    ]
    with (a.output / "compile.log").open("w") as log:
        run = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, timeout=180)
    if run.returncode:
        raise RuntimeError("mapped Icarus compile failed; see compile.log")
    try:
        with (a.output / "simulation.log").open("w") as log:
            simulation = subprocess.run(
                ["vvp", "mapped.vvp"],
                cwd=a.output,
                stdout=log,
                stderr=subprocess.STDOUT,
                timeout=180,
            )
    except subprocess.TimeoutExpired as error:
        raise RuntimeError(
            "mapped simulation timed out; partial simulation.log retained"
        ) from error
    text = (a.output / "simulation.log").read_text()
    if simulation.returncode or "MAPPED_PASS" not in text:
        raise RuntimeError("mapped simulation failed; see simulation.log")
    report = {
        "status": "PASS",
        "cycles": count,
        "retired": check["retired"],
        "reference": "same ELF/image independently checked with fixed Spike before mapped cycle replay",
        "command": command,
        "returncode": simulation.returncode,
        "timing_signoff": "NOT_RUN",
    }
    (a.output / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
    print(text, end="")


if __name__ == "__main__":
    main()

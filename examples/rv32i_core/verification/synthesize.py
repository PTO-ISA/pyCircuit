#!/usr/bin/env python3
"""Map an unmodified pyCircuit physical artifact to an explicit Liberty library."""
from __future__ import annotations
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import subprocess


def quoted(path):
    return '"' + str(path).replace("\\", "\\\\").replace('"', '\\"') + '"'


def connectivity(module, logical_ports):
    nets = defaultdict(list)
    for name, port in module["ports"].items():
        for index, bit in enumerate(port["bits"]):
            base, ordinal = name, index
            if name not in logical_ports:
                match = re.fullmatch(r"(.+)\[(\d+)\]", name)
                if not match or match[1] not in logical_ports:
                    raise ValueError("unexpected normalized port")
                base, ordinal = match[1], int(match[2])
            nets[bit].append(("port", base, port["direction"], ordinal))
    for name, cell in module["cells"].items():
        for pin, bits in cell["connections"].items():
            for index, bit in enumerate(bits):
                nets[bit].append(("cell", name, pin, index))
    return sorted(
        (
            ("constant", bit) if isinstance(bit, str) else ("net",),
            tuple(sorted(endpoints)),
        )
        for bit, endpoints in nets.items()
    )


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ["generated", "toolchain", "sv2v", "yosys", "liberty", "output"]:
        p.add_argument("--" + name, type=Path, required=True)
    p.add_argument("--lef", type=Path)
    a = p.parse_args()
    for name in vars(a):
        if getattr(a, name) is not None:
            setattr(a, name, getattr(a, name).resolve())
    if a.output.exists() and any(a.output.iterdir()):
        raise ValueError("choose an empty synthesis output directory")
    a.output.mkdir(parents=True, exist_ok=True)
    receipt = json.loads((a.generated / "generated.json").read_text())
    if receipt["target"] != "verilog" or receipt.get("rtl_mode") != "physical":
        raise ValueError("physical generated Verilog required")
    rtl = [
        a.generated / row["path"] for row in receipt["files"] if row["role"] == "rtl"
    ]
    rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
    rtl += [
        a.toolchain / "include/verilog/dff.v",
        a.toolchain / "include/verilog/dffe.v",
    ]
    converted = a.output / "synthesis.v"
    command = [
        str(a.sv2v),
        "-DSYNTHESIS",
        "--top=" + receipt["root_rtl_name"],
        *map(str, rtl),
    ]
    with converted.open("w") as out, (a.output / "sv2v.log").open("w") as log:
        run = subprocess.run(command, stdout=out, stderr=log, timeout=180)
    if run.returncode:
        raise RuntimeError("sv2v failed; see sv2v.log")
    # All architecture remains in the generated file. SYNTHESIS is the normal
    # compiler-owned boundary for simulation-only four-state fault monitors.
    script = (
        "\n".join(
            [
                "read_liberty -lib -ignore_miss_func " + quoted(a.liberty),
                "read_verilog " + quoted(converted),
                "hierarchy -check -top " + receipt["root_rtl_name"],
                "synth -top " + receipt["root_rtl_name"] + " -flatten",
                "check -assert",
                "scc -expect 0",
                "select -assert-none t:$_DLATCH* t:$dlatch*",
                "dffunmap",
                "dfflibmap -liberty " + quoted(a.liberty),
                "abc -liberty " + quoted(a.liberty),
                "clean",
                "write_json mapped-before-normalization.json",
                "clean -purge",
                "splitnets -ports",
                "check -assert",
                "select -assert-none t:$*",
                "tee -o stat.json stat -json -liberty " + quoted(a.liberty),
                "write_json mapped.json",
                "write_verilog -noattr -noexpr -simple-lhs mapped.v",
            ]
        )
        + "\n"
    )
    (a.output / "synth.ys").write_text(script)
    command2 = [str(a.yosys), "-s", str(a.output / "synth.ys")]
    with (a.output / "yosys.log").open("w") as log:
        run = subprocess.run(
            command2, stdout=log, stderr=subprocess.STDOUT, timeout=600, cwd=a.output
        )
    if run.returncode:
        raise RuntimeError("Yosys/ABC failed; see yosys.log")
    net = json.loads((a.output / "mapped.json").read_text())["modules"][
        receipt["root_rtl_name"]
    ]
    before = json.loads((a.output / "mapped-before-normalization.json").read_text())[
        "modules"
    ][receipt["root_rtl_name"]]
    if {n: (c["type"], c.get("parameters")) for n, c in before["cells"].items()} != {
        n: (c["type"], c.get("parameters")) for n, c in net["cells"].items()
    } or connectivity(before, before["ports"]) != connectivity(net, before["ports"]):
        raise ValueError("net normalization changed cell or port connectivity")
    (a.output / "normalization.json").write_text(
        json.dumps(
            {
                "status": "GRAPH_IDENTICAL",
                "cells": len(net["cells"]),
                "scope": "exact cell/type/parameter and logical-port/pin connectivity; scalar physical port serialization",
            },
            indent=2,
        )
        + "\n"
    )
    library = a.liberty.read_text()
    entries = re.split(r'\n    cell \("?([^"\)]+)"?\) \{', library)
    cells = {}
    for n in range(1, len(entries), 2):
        area = re.search(r"\barea\s*:\s*([0-9.]+)", entries[n + 1])
        if area:
            cells[entries[n]] = {
                "area": float(area[1]),
                "sequential": bool(re.search(r"\b(?:ff|latch)\s*\(", entries[n + 1])),
            }
    counts = {}
    for cell in net["cells"].values():
        counts[cell["type"]] = counts.get(cell["type"], 0) + 1
    if not counts or any(name not in cells for name in counts):
        raise ValueError("missing library area or unmapped/unknown cell")
    sequential = sum(
        count for name, count in counts.items() if cells[name]["sequential"]
    )
    if sequential == 0:
        raise ValueError("no retained sequential architecture")
    area_unit = "library area units; no geometry cross-check supplied"
    if a.lef:
        geometry = {}
        for entry in re.split(r"(?m)^MACRO ", a.lef.read_text())[1:]:
            name = entry.split()[0]
            size = re.search(r"\bSIZE ([0-9.]+) BY ([0-9.]+)", entry)
            if size:
                geometry[name] = float(size[1]) * float(size[2])
        for name in counts:
            if (
                name not in geometry
                or abs(geometry[name] - cells[name]["area"]) > 0.000001
            ):
                raise ValueError(
                    "Liberty area does not match selected LEF geometry: " + name
                )
        area_unit = "um^2; all used Liberty cells match owning LEF width*height"
    report = {
        "status": "MAPPED",
        "root": receipt["root_rtl_name"],
        "liberty": str(a.liberty),
        "liberty_sha256": hashlib.sha256(a.liberty.read_bytes()).hexdigest(),
        "cells": sum(counts.values()),
        "sequential_cells": sequential,
        "combinational_cells": sum(counts.values()) - sequential,
        "liberty_cell_area": sum(
            cells[name]["area"] * count for name, count in counts.items()
        ),
        "area_unit": area_unit,
        "sequential_area": sum(
            cells[name]["area"] * count
            for name, count in counts.items()
            if cells[name]["sequential"]
        ),
        "combinational_area": sum(
            cells[name]["area"] * count
            for name, count in counts.items()
            if not cells[name]["sequential"]
        ),
        "netlist_interface": "scalar ports; logical packing from original ports.json, physical pins from mapped.json",
        "cell_counts": counts,
        "commands": [command, command2],
        "clock_timing": "NOT_RUN",
        "placement_routing": "NOT_RUN",
        "mapped_simulation": "NOT_RUN",
        "boundary": "core including GPR file, pipeline/control and observation cones; excludes external instruction/data memories and test adapter",
    }
    (a.output / "area.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()

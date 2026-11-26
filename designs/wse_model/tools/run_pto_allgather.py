"""Compile AllGather programs and run them on one unchanged generic WSE binary.

Real PyPTO and PTOAS compile each workload. Only the resulting program, sparse
memory image and readback ranges change between executions. Expected rank-major
bytes are built from original inputs, independently of the generated commands.
"""

from __future__ import annotations

import argparse
import json
import os
import struct
from pathlib import Path

from machine_common import digest, execute_program, run, verify_build

from wse_model.programs.abi import Command, Kind, MemoryPatch, MemoryRegion, Program

ROOT = Path(__file__).resolve().parents[1]


def compile_workload(args, directory, parameters):
    compiler = directory / "compiler"
    compiler.mkdir(parents=True)
    stages = {}
    command = [
        args.pypto_python.absolute(),
        ROOT / "examples/pto_allgather.py",
        "--output",
        compiler,
    ]
    for name, value in parameters.items():
        command += ["--" + name.replace("_", "-"), str(value)]
    stages["pypto"] = run(
        command, directory, "pypto", dict(os.environ, PYPTO_SKIP_RUNTIME_PIN_CHECK="1")
    )
    for phase in ("row", "column"):
        stages["ptoas-" + phase] = run(
            [
                args.ptoas_python.absolute(),
                args.ptoas_root.resolve() / "bin/ptoas",
                compiler / f"{phase}.pto",
                "--pto-arch=a3",
                "--pto-level=level3",
                "--enable-insert-sync",
                "--emit-pto-ir",
                "-o",
                compiler / f"{phase}.lowered.pto",
            ],
            directory,
            "ptoas-" + phase,
        )
    env = dict(
        os.environ, PYTHONPATH=os.pathsep.join((str(args.ptoas_root.resolve()), str(ROOT / "src")))
    )
    for tool in ("lower_pto_allgather", "verify_pto_allgather_ir"):
        stages[tool] = run(
            [args.ptoas_python.absolute(), ROOT / f"tools/{tool}.py", "--compiler-dir", compiler],
            directory,
            tool,
            env,
        )
    manifest = json.loads((compiler / "program.json").read_text())
    raw = (compiler / "machine.bin").read_bytes()
    program = Program(tuple(Command.unpack(raw[i : i + 64]) for i in range(0, len(raw), 64)))
    if program.pack() != raw:
        raise AssertionError("machine program changed on ABI round trip")
    return compiler, manifest, program, stages


def inputs(manifest, seed):
    members = manifest["members"]
    first, last = manifest["phases"][0], manifest["phases"][-1]
    originals = {
        node: b"".join(
            struct.pack("<f", float(seed * 400000 + node * 10000 + i))
            for i in range(first["words"])
        )
        for node in members
    }
    expected_rank = b"".join(originals[node] for node in members)
    patches = tuple(MemoryPatch(node, first["source_base"], originals[node]) for node in members)
    ranges = tuple(MemoryRegion(node, last["target_base"], len(expected_rank)) for node in members)
    return patches, ranges, expected_rank * len(members)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pypto-python", type=Path, required=True)
    parser.add_argument("--ptoas-python", type=Path, required=True)
    parser.add_argument("--ptoas-root", type=Path, required=True)
    parser.add_argument("--simulator", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--profile", choices=("baseline-32", "variant-4"))
    parser.add_argument(
        "--baseline-only", action="store_true", help="Run only seed-1 without permit stalls"
    )
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    simulator = args.simulator.resolve()
    source_paths = [
        *sorted((ROOT / "src/wse_model").rglob("*.py")),
        Path(__file__).resolve(),
        ROOT / "tools/machine_common.py",
        ROOT / "examples/pto_allgather.py",
        ROOT / "tools/lower_pto_allgather.py",
        ROOT / "tools/verify_pto_allgather_ir.py",
    ]
    source_hashes = {str(path.relative_to(ROOT)): digest(path) for path in source_paths}
    receipt = {
        "status": "INCOMPLETE",
        "build": verify_build(simulator),
        "sources": source_hashes,
        "workloads": [],
    }

    def save():
        (output / "verification.json").write_text(json.dumps(receipt, indent=2) + "\n")

    save()
    try:
        profiles = (
            ("baseline-32", {}),
            (
                "variant-4",
                {
                    "rows": 2,
                    "cols": 2,
                    "words": 16,
                    "fragments": 8,
                    "source_base": 512,
                    "row_base": 8192,
                    "output_base": 32768,
                },
            ),
        )
        for label, parameters in profiles:
            if args.profile and label != args.profile:
                continue
            directory = output / label
            compiler, manifest, program, stages = compile_workload(args, directory, parameters)
            workload = {"name": label, "parameters": parameters, "stages": stages, "runs": []}
            receipt["workloads"].append(workload)
            lowering = json.loads((compiler / "lowering.json").read_text())
            expected_reads = sum(
                p["source_read_bytes"] for p in lowering["static_traffic"]["phases"]
            )
            expected_writes = sum(
                p["destination_write_bytes"] for p in lowering["static_traffic"]["phases"]
            )
            copy_pc, copy = next(
                (i, c)
                for i, c in enumerate(program.commands)
                if c.kind == Kind.COPY and c.nbytes >= 16
            )
            remote = next(node for node in range(40) if (copy.route >> (2 * node)) & 1)
            beat_bytes = receipt["build"]["receipt"]["profile"]["beat_bytes"]
            if (
                copy.src % beat_bytes == 0
                and copy.dst % beat_bytes == 0
                and copy.nbytes >= beat_bytes
            ):
                stall_offset = beat_bytes if copy.nbytes > beat_bytes else 0
            else:
                stall_offset = 8
            for name, seed, stall in (
                ("baseline", 1, None),
                ("new-data", 7, None),
                ("stall", 1, {"pc": copy_pc, "node": remote, "offset": stall_offset, "cycles": 3}),
            ):
                if args.baseline_only and name != "baseline":
                    continue
                patches, ranges, expected = inputs(manifest, seed)
                result = execute_program(
                    simulator, program, patches, ranges, directory / name, stall=stall
                )
                if Path(result["output"]).read_bytes() != expected:
                    raise AssertionError(
                        f"{label}/{name}: payload differs from original-input concatenation"
                    )
                # Flat original input supports the independent opt-in byte oracle.
                original_path = directory / name / "input.bin"
                original_path.write_bytes(b"".join(patch.data for patch in patches))
                result["original_input_sha256"] = digest(original_path)
                simulation = result["simulation"]
                if (simulation["source_read_bytes"], simulation["destination_write_bytes"]) != (
                    expected_reads,
                    expected_writes,
                ):
                    raise AssertionError("DUT traffic differs from compiled transfer accounting")
                workload["runs"].append(
                    {
                        "case": name,
                        "seed": seed,
                        "checked_bytes": len(expected),
                        "status": "PASS",
                        **result,
                    }
                )
                save()
                print(f"{label}/{name}: PASS", flush=True)
            if args.baseline_only:
                workload["status"] = "PASS"
                workload["control_comparison"] = "NOT RUN (baseline only)"
                save()
                continue
            baseline, changed, stalled = workload["runs"]
            cycles = [r["simulation"]["execution_cycles"] for r in workload["runs"]]
            if cycles[1] != cycles[0] or cycles[2] != cycles[0] + 3:
                raise AssertionError("data-only/three-cycle-permit controlled comparison failed")
            if (
                baseline["inputs"] != stalled["inputs"]
                or baseline["output_sha256"] != stalled["output_sha256"]
            ):
                raise AssertionError("stall changed program/data/readback/output")
            if changed["output_sha256"] == baseline["output_sha256"]:
                raise AssertionError("new input data did not change output")
            snapshots = stalled["simulation"]["stall_observations"]
            if len(snapshots) != 3 or any(s != snapshots[0] for s in snapshots):
                raise AssertionError("stalled beat advanced state or byte counters")
            workload["status"] = "PASS"
            workload["control_comparison"] = {"new_data_cycle_delta": 0, "stall_cycle_delta": 3}
            save()
        hashes = {r["simulator_sha256"] for w in receipt["workloads"] for r in w["runs"]}
        if (
            hashes != {receipt["build"]["receipt"]["runner_sha256"]}
            or digest(simulator) not in hashes
        ):
            raise AssertionError("workloads did not use one unchanged compiled machine")
        receipt["conditions"] = {
            "backend": "cpp",
            "execution": "serialized-functional",
            "hardware_nodes": 40,
            "bank_bytes_per_node": 65536,
            "payload_beat_bytes": receipt["build"]["receipt"]["profile"]["beat_bytes"],
            "workers": 1,
            "changed_workload_fields": ["members", "length", "addresses", "data", "program"],
            "controlled_fields": ["final ACIR", "simulator binary", "device capacities"],
            "cycle_boundary": "after start, before readback; functional cycles, not physical latency",
        }
        if {str(path.relative_to(ROOT)): digest(path) for path in source_paths} != source_hashes:
            raise ValueError("program compiler or oracle source changed during acceptance")
        verify_build(simulator)
        receipt["status"] = "PASS"
    except Exception as error:
        receipt["error"] = str(error)
        raise
    finally:
        save()


if __name__ == "__main__":
    main()

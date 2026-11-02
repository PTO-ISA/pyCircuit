#!/usr/bin/env python3
"""Compile finite RV32I tests and record independent Spike expectations."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import subprocess
from programs import assembly_cases, C_DEMO
from verify import elf_image, oracle


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--clang", type=Path, required=True)
    p.add_argument("--spike", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--case", action="append")
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).resolve().parents[1]
    cases = assembly_cases()
    cases["c-array"] = None
    reports = []
    for name, text in cases.items():
        if args.case and name not in args.case:
            continue
        out = (args.output / name).resolve()
        out.mkdir(parents=True, exist_ok=True)
        if text is not None:
            file = out / "program.S"
            file.write_text(text)
            sources = [file]
        else:
            file = out / "program.c"
            file.write_text(C_DEMO)
            start = out / "start.S"
            start.write_text(
                ".section .text.init\n.globl _start\n_start:\nlui sp,0x80010\ncall main\nebreak\n"
            )
            sources = [start, file]
        elf = out / "program.elf"
        command = [
            str(args.clang),
            "--target=riscv32-unknown-elf",
            "-march=rv32i",
            "-mabi=ilp32",
            "-mno-relax",
            "-O2",
            "-ffreestanding",
            "-fno-builtin",
            "-nostdlib",
            "-fuse-ld=lld",
            "-Wl,-T," + str(source / "link.ld"),
            *[str(s) for s in sources],
            "-o",
            str(elf),
        ]
        run = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=30,
        )
        (out / "compile.log").write_text(run.stdout)
        if run.returncode:
            raise RuntimeError(f"{name}: compile failed: {run.stdout}")
        (out / "compile.json").write_text(
            json.dumps({"command": command, "returncode": run.returncode}, indent=2)
            + "\n"
        )
        dump = args.clang.parent / "llvm-objdump"
        (out / "program.dis").write_text(
            subprocess.check_output([str(dump), "-d", str(elf)], text=True)
        )
        (out / "image.bin").write_bytes(elf_image(elf))
        cfg = json.loads((source / "simulation.json").read_text())
        (out / "simulation.json").write_text(json.dumps(cfg, indent=2) + "\n")
        ref = oracle(elf, args.spike, out)
        if name == "c-array":
            symbols = subprocess.check_output(
                [str(args.clang.parent / "llvm-nm"), "-S", str(elf)], text=True
            )
            symbol = next(
                line.split()[0]
                for line in symbols.splitlines()
                if line.split()[-1] == "signature"
            )
            values = [(n << 2) ^ 0x135 for n in range(32)]
            mix = 0x12345678
            for value in values:
                mix = ((mix << 5) ^ (mix >> 27) ^ value) & 0xFFFFFFFF
            signature = {
                "address": int(symbol, 16),
                "words": [sum(values), mix, values[0], values[-1]],
                "basis": "independent integer evaluation of C source, not Spike or DUT output",
            }
            (out / "c-signature.json").write_text(
                json.dumps(signature, indent=2) + "\n"
            )
        basis = "spike"
        if name == "illegal_fence_i":
            # RV32I2.1 moved FENCE.I into Zifencei. This fixed Spike build accepts
            # it under RV32I; retain that raw disagreement, never learn from DUT.
            spec = {
                "events": [
                    {
                        "pc": 0x80000000,
                        "instruction": 0x100F,
                        "trap": 1,
                        "cause": 2,
                        "tval": 0x100F,
                        "rd_write": 0,
                        "load": 0,
                        "store": 0,
                    }
                ],
                "registers": [0] * 32,
                "authority": "RV32I2.1 v20260120: FENCE.I belongs to Zifencei; closed demo EEI traps unavailable encodings",
            }
            (out / "spec-oracle.json").write_text(json.dumps(spec, indent=2) + "\n")
            (out / "spec-memory.bin").write_bytes(elf_image(elf))
            basis = "spec"
        reports.append(
            {
                "case": name,
                "basis": basis,
                "retired": len(ref["events"]) - 1,
                "trap": ref["events"][-1],
            }
        )
        print(name, len(ref["events"]) - 1, ref["events"][-1]["cause"], flush=True)
    (args.output / "reference-summary.json").write_text(
        json.dumps(reports, indent=2) + "\n"
    )


if __name__ == "__main__":
    main()

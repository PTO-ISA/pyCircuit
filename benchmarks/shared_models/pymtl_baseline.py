#!/usr/bin/env python3
"""PyMTL3 pure 16-bit normalized-word reference (Python or imported RTL).

Run from an isolated output directory. This adapter records its mode; its
Python rate is not comparable to pyCircuit native rates. No ratio is computed.
"""

import argparse
import importlib.metadata
import json
import statistics
import time
from pathlib import Path

from pymtl3 import Bits1, Bits16, Component, DefaultPassGroup, InPort, OutPort, update


class Normalize(Component):
    def construct(s):  # noqa: N805
        s.value = InPort(Bits16)
        s.mask = InPort(Bits16)
        s.result = OutPort(Bits16)
        s.valid = OutPort(Bits1)

        @update
        def normalize():
            s.result @= s.value ^ s.mask
            s.valid @= s.mask != 0


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--mode", choices=("python", "verilator"), default="python")
    p.add_argument("--samples", type=int, default=5)
    p.add_argument("--iterations", type=int, default=10000)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    report = {
        "adapter": "PyMTL3 Normalize16",
        "version": importlib.metadata.version("pymtl3"),
        "mode": args.mode,
        "status": "NOT_RUN",
        "comparison": "No cross-tool ranking; standalone combinational kernel, excludes pyCircuit queues/protocol/state",
        "samples": [],
    }
    try:
        assert report["version"] == "3.1.17"
        model = Normalize()
        if args.mode == "verilator":
            from pymtl3.passes.backends.verilog import VerilogTranslationImportPass

            model.elaborate()
            model.set_metadata(VerilogTranslationImportPass.enable, True)
            model = VerilogTranslationImportPass()(model)
        model.apply(DefaultPassGroup())
        vectors = [
            (0x1234, 0x00FF, 4811),
            (0xABCD, 0xF0F0, 23357),
            (0xFFF0, 0x00F0, 65280),
        ]
        for trial in range(args.samples + 1):
            model.sim_reset()
            started = time.perf_counter()
            for n in range(args.iterations):
                value, mask, expected = vectors[n % 3]
                model.value @= value
                model.mask @= mask
                model.sim_eval_combinational()
                assert int(model.result) == expected and int(model.valid) == 1
            elapsed = time.perf_counter() - started
            if trial:
                report["samples"].append(elapsed)
        report.update(
            status="passed",
            iterations=args.iterations,
            median_seconds=statistics.median(report["samples"]),
            scope="host loop + typed I/O + per-value validation; one warmup excluded",
        )
    except Exception as error:
        report["reason"] = str(error)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    if report["status"] != "passed":
        raise SystemExit(1)


if __name__ == "__main__":
    main()

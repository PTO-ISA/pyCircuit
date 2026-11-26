"""Two rank-major AllGather kernels for the WSE Calendar functional profile.

Run with the separate PyPTO environment, not the WSE/pyCircuit environment.
The distributed runtime is not invoked: these are real compiler inputs.
"""

import argparse
import hashlib
import json
from pathlib import Path

import pypto.language as pl
import pypto.language.distributed as pld
from pypto import backend, codegen, ir
from pypto.backend import BackendType
from pypto.ir import OptimizationStrategy, PassManager


def allgather_kernel(ranks: int, words: int):
    @pl.program
    class AllGather:
        @pl.function(type=pl.FunctionType.InCore)
        def gather(
            self,
            source: pl.Tensor[[1, words], pl.FP32],
            target: pl.InOut[pld.DistributedTensor[[ranks, words], pl.FP32]],
            signal: pl.InOut[pld.DistributedTensor[[ranks, 1], pl.INT32]],
        ) -> pld.DistributedTensor[[ranks, words], pl.FP32]:
            return pld.tensor.allgather(source, target, signal)

    return AllGather


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--rows", type=int, default=4)
    parser.add_argument("--cols", type=int, default=8)
    parser.add_argument("--words", type=int, default=384)
    parser.add_argument("--fragments", type=int, default=8)
    parser.add_argument("--source-base", type=int, default=0)
    parser.add_argument("--row-base", type=int, default=4096)
    parser.add_argument("--output-base", type=int, default=16384)
    args = parser.parse_args()
    if not 1 <= args.rows <= 5 or not 1 <= args.cols <= 8:
        parser.error("groups must fit the explicit 5x8 topology")
    if args.words <= 0 or args.fragments <= 0 or args.words * 4 % (8 * args.fragments):
        parser.error("source words must partition into aligned 8B fragments")
    args.output.mkdir(parents=True, exist_ok=True)
    backend.set_backend_type(BackendType.Ascend910)
    phases = []
    for name, ranks, words, groups, source_base, target_base in (
        (
            "row",
            args.cols,
            args.words,
            [list(range(r * 8, r * 8 + args.cols)) for r in range(args.rows)],
            args.source_base,
            args.row_base,
        ),
        (
            "column",
            args.rows,
            args.cols * args.words,
            [list(range(c, args.rows * 8, 8)) for c in range(args.cols)],
            args.row_base,
            args.output_base,
        ),
    ):
        program = allgather_kernel(ranks, words)
        lowered = PassManager.get_strategy(OptimizationStrategy.Default).run_passes(program)
        text = codegen.PTOCodegen().generate(lowered, emit_tile_addr=True)
        (args.output / f"{name}.pto").write_text(text)
        (args.output / f"{name}.pypto-ir.py").write_text(ir.python_print(lowered))
        phases.append(
            {
                "name": name,
                "kernel": "gather",
                "ranks": ranks,
                "words": words,
                "dtype": "f32",
                "groups": groups,
                "source_base": source_base,
                "target_base": target_base,
                "fragments": args.fragments,
                "input_shape": [1, words],
                "output_shape": [ranks, words],
                "layout": "rank-major-contiguous",
                "raw_pto_sha256": hashlib.sha256(text.encode()).hexdigest(),
            }
        )
    manifest = {
        "schema": "wse-pypto-allgather/1",
        "topology": [5, 8],
        "members": [r * 8 + c for r in range(args.rows) for c in range(args.cols)],
        "bank_bytes": 65536,
        "phases": phases,
        "dependency": {
            "producer": "row.target",
            "consumer": "column.source",
            "view": "contiguous-flatten",
            "bytes": args.cols * args.words * 4,
        },
        "source_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    }
    (args.output / "program.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(args.output)


if __name__ == "__main__":
    main()

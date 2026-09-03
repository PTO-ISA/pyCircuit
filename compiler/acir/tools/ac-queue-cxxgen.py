#!/usr/bin/env python3
"""Generate one canonical typed gfsim C++ file from serial Queue Python."""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import tempfile

from agentic_circuit._queue_codegen import lower_queue_program_to_cpp
from agentic_circuit._queue_frontend import lower_queue_program, parse_queue_program


def _write_atomic(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(
        dir=path.parent, prefix=f".{path.name}.", text=True
    )
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as stream:
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)


def main() -> int:
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("source", type=Path)
    parser.add_argument("--system", required=True)
    parser.add_argument("-o", "--output", required=True, type=Path)
    parser.add_argument("--acir-output", type=Path)
    parser.add_argument("--plan-output", type=Path)
    parser.add_argument("--acir-opt", type=Path)
    parser.add_argument("--queue-plan-tool", type=Path)
    parser.add_argument("--queue-cxxgen-tool", type=Path)
    arguments = parser.parse_args()
    artifact_options = (
        arguments.acir_output,
        arguments.plan_output,
        arguments.acir_opt,
        arguments.queue_plan_tool,
        arguments.queue_cxxgen_tool,
    )
    if any(value is not None for value in artifact_options) and any(
        value is None for value in artifact_options
    ):
        parser.error(
            "--acir-output, --plan-output, --acir-opt, --queue-plan-tool, and "
            "--queue-cxxgen-tool must be provided together"
        )
    program = parse_queue_program(
        arguments.source.read_text(encoding="utf-8"), arguments.system
    )
    generated = lower_queue_program_to_cpp(program)
    canonical_acir: str | None = None
    queue_plan: str | None = None
    if arguments.acir_output is not None:
        assert arguments.acir_opt is not None
        assert arguments.queue_plan_tool is not None
        assert arguments.queue_cxxgen_tool is not None
        with tempfile.TemporaryDirectory() as directory:
            raw = Path(directory) / "model.ac.mlir"
            raw.write_text(lower_queue_program(program), encoding="utf-8")
            optimized = subprocess.run(
                (
                    str(arguments.acir_opt),
                    "--canonicalize",
                    "--cse",
                    str(raw),
                ),
                text=True,
                capture_output=True,
                check=False,
            )
            if optimized.returncode != 0:
                parser.error(f"acir-opt rejected generated ACIR: {optimized.stderr}")
            canonical_acir = optimized.stdout
            frozen = Path(directory) / "model.frozen.ac.mlir"
            frozen.write_text(canonical_acir, encoding="utf-8")
            planned = subprocess.run(
                (str(arguments.queue_plan_tool), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            if planned.returncode != 0:
                parser.error(f"QueueGraph planning failed: {planned.stderr}")
            queue_plan = planned.stdout
            emitted = subprocess.run(
                (str(arguments.queue_cxxgen_tool), str(frozen)),
                text=True,
                capture_output=True,
                check=False,
            )
            if emitted.returncode != 0:
                parser.error(f"native Queue C++ generation failed: {emitted.stderr}")
            generated = emitted.stdout
    _write_atomic(arguments.output, generated)
    if canonical_acir is not None and queue_plan is not None:
        assert arguments.acir_output is not None
        assert arguments.plan_output is not None
        _write_atomic(arguments.acir_output, canonical_acir)
        _write_atomic(arguments.plan_output, queue_plan)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

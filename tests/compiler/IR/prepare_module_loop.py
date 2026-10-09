"""Build the public source closure consumed by the task-partition API tests."""

import argparse
import os
import subprocess
import sys
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("repo", "source-compiler", "linker", "output"):
    parser.add_argument("--" + name, required=True)
args = parser.parse_args()
repo = Path(args.repo).resolve()
output = Path(args.output).resolve()
output.parent.mkdir(parents=True, exist_ok=True)
units = output.parent / "module-loop-units"
units.mkdir(parents=True, exist_ok=True)
source = repo / "examples/module_loop"
env = os.environ.copy()
env.update(
    PYTHONPATH=str(repo / "python"),
    PYTHONDONTWRITEBYTECODE="1",
    PYCIRCUIT_SOURCE_COMPILER=args.source_compiler,
    PYCIRCUIT_LINKER=args.linker,
)


def run(*arguments):
    subprocess.run(
        [sys.executable, "-m", "pycircuit.cli", *map(str, arguments)],
        env=env,
        check=True,
    )


for name in ("child", "module_loop"):
    imports = [] if name == "child" else ["-I", units / "child"]
    run(
        "compile",
        "-c",
        source / (name + ".py"),
        "--source-root",
        source,
        "--package-prefix",
        "example_loop",
        *imports,
        "-o",
        units / name,
        "--replace",
    )
run(
    "link",
    units / "child",
    units / "module_loop",
    "--top",
    "example_loop.module_loop.Top",
    "-o",
    output,
    "--replace",
)

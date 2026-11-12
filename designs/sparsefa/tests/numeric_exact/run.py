"""Run exact arithmetic vectors through the public compile/link/emit workflow."""

import json
import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[4]
INSTALL = Path(
    os.environ.get("PYC_TOOLCHAIN_ROOT", ROOT / ".pycircuit_out/toolchain/install")
)
OUT = ROOT / ".pycircuit_out/fa2-numeric-exact"
LOG = ROOT / "docs/gates/logs/fa2-p0-completion"
ENV = dict(os.environ)
ENV["PYTHONPATH"] = str(INSTALL / "share/pycircuit/python")
ENV["LD_LIBRARY_PATH"] = (
    f"/home/xiekunpeng/micromamba/envs/acir/lib:{INSTALL}/lib:"
    + ENV.get("LD_LIBRARY_PATH", "")
)
DRIVER = INSTALL / "bin/pycircuit"
OUT.mkdir(parents=True, exist_ok=True)
LOG.mkdir(parents=True, exist_ok=True)


def run(name, arguments):
    argv = [str(value) for value in arguments]
    with (LOG / f"exact-{name}.log").open("w") as stream:
        result = subprocess.run(argv, env=ENV, stdout=stream, stderr=subprocess.STDOUT)
    with (LOG / "exact-commands.jsonl").open("a") as stream:
        stream.write(json.dumps({"command": argv, "exit": result.returncode}) + "\n")
    if result.returncode:
        raise SystemExit(f"{name}: exit {result.returncode}; see {LOG}")


src = ROOT / "designs/sparsefa/src"
run(
    "numeric-compile",
    [
        DRIVER,
        "compile",
        "-c",
        src / "fa2_numeric.py",
        "--source-root",
        src,
        "--package-prefix",
        "fa2",
        "-o",
        OUT / "numeric",
        "--replace",
    ],
)
run(
    "bench-compile",
    [
        DRIVER,
        "compile",
        "-c",
        Path(__file__).with_name("bench.py"),
        "--source-root",
        Path(__file__).parent,
        "--package-prefix",
        "exact",
        "-I",
        OUT / "numeric",
        "-o",
        OUT / "bench",
        "--replace",
    ],
)
run(
    "link",
    [
        DRIVER,
        "link",
        OUT / "numeric",
        OUT / "bench",
        "--top",
        "exact.bench.ExactSystem",
        "-o",
        OUT / "final.ac",
        "--replace",
    ],
)
for target in ("cpp", "verilog"):
    run(
        f"emit-{target}",
        [
            DRIVER,
            "emit",
            OUT / "final.ac",
            "--target",
            target,
            "-o",
            OUT / target,
            "--replace",
        ],
    )
compiler = os.environ.get("CXX") or shutil.which("x86_64-conda-linux-gnu-g++")
if not compiler:
    candidate = Path(
        "/home/xiekunpeng/micromamba/envs/acir/bin/x86_64-conda-linux-gnu-g++"
    )
    compiler = str(candidate) if candidate.exists() else "c++"
run(
    "configure",
    [
        "cmake",
        "-S",
        OUT / "cpp",
        "-B",
        OUT / "build",
        f"-DCMAKE_PREFIX_PATH={INSTALL}",
        f"-DCMAKE_CXX_COMPILER={compiler}",
    ],
)
run("build", ["cmake", "--build", OUT / "build", "-j2"])
for workers in (1, 2):
    run(
        f"workers{workers}",
        [OUT / "build/bin/pycircuit_sim", "--cycles", "32", "--workers", workers],
    )
print(
    "Exact arithmetic: 32 cases passed generated C++ workers 1/2; RTL emitted."
)  # noqa: T201

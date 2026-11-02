#!/usr/bin/env python3
"""Build the exact public tool-PR dependency in a new local output prefix."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--llvm-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    pin = json.loads(
        (Path(__file__).resolve().parents[1] / "pycircuit-dependency.json").read_text()
    )
    output = args.output.resolve()
    if output.exists() and any(output.iterdir()):
        raise ValueError("choose a new empty SDK output prefix")
    output.mkdir(parents=True, exist_ok=True)
    source, build, install = [output / name for name in ("source", "build", "install")]
    env = {
        key: value
        for key, value in os.environ.items()
        if not key.startswith(("PYC_", "PYCIRCUIT_"))
        and key not in {"PYTHONPATH", "PYTHONHOME", "CMAKE_PREFIX_PATH"}
    }
    commands = [
        ["git", "init", str(source)],
        ["git", "-C", str(source), "remote", "add", "origin", pin["repository"]],
        ["git", "-C", str(source), "fetch", "--depth", "1", "origin", pin["commit"]],
        ["git", "-C", str(source), "checkout", "--detach", "FETCH_HEAD"],
    ]
    for command in commands:
        subprocess.run(command, env=env, check=True)
    actual = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
    ).strip()
    if actual != pin["commit"]:
        raise ValueError(
            "public dependency ref moved; require the documented exact commit"
        )
    subprocess.run(
        [
            "cmake",
            "-S",
            str(source),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DCMAKE_INSTALL_PREFIX={install}",
            "-DPYC_BUILD_COMPILER_DEV=ON",
            "-DPYC_BUILD_RUNTIME_LIB=ON",
            "-DPYC_BUILD_TESTING=OFF",
            "-DPYC_INSTALL_PYTHON=ON",
            f"-DLLVM_DIR={args.llvm_root.resolve()}/lib/cmake/llvm",
            f"-DMLIR_DIR={args.llvm_root.resolve()}/lib/cmake/mlir",
        ],
        env=env,
        check=True,
    )
    subprocess.run(
        ["cmake", "--build", str(build), "--parallel", str(args.jobs)],
        env=env,
        check=True,
    )
    subprocess.run(["cmake", "--install", str(build)], env=env, check=True)
    print(f"SDK installed: {install}")


if __name__ == "__main__":
    main()

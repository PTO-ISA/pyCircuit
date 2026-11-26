"""Build the conservative packet timing projection with the pinned public tools."""

import argparse
import hashlib
import json
import os
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    toolchain = Path(os.environ["PYC_TOOLCHAIN_ROOT"])
    compiler = toolchain / "bin/pycircuit"
    source = ROOT / "models/pto_allgather/timing.py"
    driver = source.with_name("timing_driver.cpp")
    receipt = {
        "status": "INCOMPLETE",
        "sources": {str(p): digest(p) for p in (source, driver, Path(__file__).resolve())},
        "toolchain": json.loads(
            (toolchain / "share/pycircuit/toolchain-metadata.json").read_text()
        ),
        "compiler": {"path": str(compiler), "sha256": digest(compiler)},
        "native_helpers": {
            name: digest(toolchain / "bin" / name)
            for name in ("pycircuit-source-unit", "pycircuit-link", "pycircuit-emit")
        },
        "conditions": {
            "optimization": "O1",
            "build_parallelism": 1,
            "workers": 1,
            "cxx": os.environ.get("CXX"),
        },
        "stages": [],
    }

    def save():
        (output / "build.json").write_text(json.dumps(receipt, indent=2) + "\n")

    def run(name, command):
        entry = {"name": name, "command": [str(v) for v in command]}
        receipt["stages"].append(entry)
        save()
        start = time.monotonic()
        with (output / f"{name}.log").open("w") as log:
            result = subprocess.run(
                entry["command"], stdout=log, stderr=subprocess.STDOUT, timeout=1800, check=False
            )
        entry.update(returncode=result.returncode, seconds=time.monotonic() - start)
        save()
        print(f"{name}: {result.returncode}", flush=True)
        if result.returncode:
            raise RuntimeError(f"{name} failed; see {output / (name + '.log')}")

    try:
        run(
            "compile",
            [
                compiler,
                "compile",
                "-c",
                source,
                "--source-root",
                source.parent,
                "--package-prefix",
                "wse_timing",
                "-o",
                output / "unit",
            ],
        )
        run(
            "link",
            [
                compiler,
                "link",
                output / "unit",
                "--top",
                "wse_timing.timing.PacketTiming",
                "-o",
                output / "final.ac",
            ],
        )
        receipt["final_sha256"] = digest(output / "final.ac")
        run(
            "emit-cpp",
            [compiler, "emit", output / "final.ac", "--target", "cpp", "-o", output / "cpp"],
        )
        consumer = output / "consumer"
        consumer.mkdir()
        (consumer / "CMakeLists.txt").write_text(
            "cmake_minimum_required(VERSION 3.25)\nproject(WseTiming LANGUAGES C CXX)\n"
            f'add_subdirectory("{output / "cpp"}" generated)\n'
            f'add_executable(timing_runner "{driver}")\n'
            "target_compile_features(timing_runner PRIVATE cxx_std_20)\n"
            "target_link_libraries(timing_runner PRIVATE pycircuit_modules)\n"
        )
        run(
            "configure",
            [
                "cmake",
                "-S",
                consumer,
                "-B",
                output / "build",
                "-G",
                "Ninja",
                f"-DCMAKE_PREFIX_PATH={toolchain}",
                "-DCMAKE_BUILD_TYPE=Release",
                "-DCMAKE_CXX_FLAGS_RELEASE=-O1 -DNDEBUG",
            ],
        )
        run("build", ["cmake", "--build", output / "build", "--parallel", "1"])
        receipt["runner"] = str(output / "build/timing_runner")
        receipt["runner_sha256"] = digest(output / "build/timing_runner")
        receipt["status"] = "PASS"
    except Exception as error:
        receipt["status"] = "FAIL"
        receipt["error"] = str(error)
        raise
    finally:
        save()


if __name__ == "__main__":
    main()

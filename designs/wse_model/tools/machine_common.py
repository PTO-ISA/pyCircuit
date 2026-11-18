"""Host-side loading and provenance for the runtime-programmed WSE machine."""

from __future__ import annotations

import hashlib
import json
import subprocess
import time
from pathlib import Path

from wse_model.programs.abi import Program, encode_image, encode_readback


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, directory: Path, label: str, env=None, *, reject=False) -> dict:
    started = time.monotonic()
    with (directory / f"{label}.log").open("w") as log:
        result = subprocess.run(
            [str(value) for value in command],
            env=env,
            stdout=log,
            stderr=subprocess.STDOUT,
            timeout=21600,
            check=False,
        )
    receipt = {
        "command": [str(value) for value in command],
        "exit_code": result.returncode,
        "wall_seconds": time.monotonic() - started,
        "log": str(directory / f"{label}.log"),
    }
    (directory / f"{label}.stage.json").write_text(json.dumps(receipt, indent=2) + "\n")
    if (reject and result.returncode == 0) or (not reject and result.returncode != 0):
        raise RuntimeError(f"{label}: unexpected exit {result.returncode}; see {receipt['log']}")
    return receipt


def verify_build(simulator: Path) -> dict:
    path = simulator.parent.parent / "build.json"
    build = json.loads(path.read_text())
    if build["status"] != "PASS" or build["runner_sha256"] != digest(simulator):
        raise ValueError("simulator does not match its successful recorded build")
    for source, expected in build["sources"].items():
        if digest(Path(source)) != expected:
            raise ValueError(f"source changed since the recorded build: {source}")
    if digest(path.parent / "final.ac") != build["final_sha256"]:
        raise ValueError("final hardware ACIR changed since build")
    return {"path": str(path), "sha256": digest(path), "receipt": build}


def execute_program(
    simulator: Path,
    program: Program,
    patches,
    readback,
    directory: Path,
    *,
    stall: dict | None = None,
    expect_fault: bool = False,
    timing_config: bytes | None = None,
) -> dict:
    """Load only inputs; byte/count expectations are deliberately caller-owned."""
    directory.mkdir(parents=True, exist_ok=False)
    (directory / "program.bin").write_bytes(program.pack())
    (directory / "program.json").write_text(json.dumps(program.manifest(), indent=2) + "\n")
    (directory / "image.bin").write_bytes(encode_image(patches, program.profile))
    (directory / "readback.bin").write_bytes(encode_readback(readback, program.profile))
    names = ("program.bin", "program.json", "image.bin", "readback.bin")
    if timing_config is not None:
        (directory / "timing.bin").write_bytes(timing_config)
        names += ("timing.bin",)
    before = {name: digest(directory / name) for name in names}
    command = [
        simulator,
        directory / "program.bin",
        directory / "image.bin",
        directory / "readback.bin",
        directory / "output.bin",
        directory / "simulation.json",
    ]
    if timing_config is not None:
        command += ["--config", directory / "timing.bin", "--events", directory / "events.jsonl"]
    for key, value in (stall or {}).items():
        command += ["--stall-" + key.replace("_", "-"), str(value)]
    simulator_before = digest(simulator)
    stage = run(command, directory, "simulation", reject=expect_fault)
    if digest(simulator) != simulator_before:
        raise ValueError("simulator changed during execution")
    simulation = json.loads((directory / "simulation.json").read_text())
    expected_status = "FAULT" if expect_fault else "EXECUTED"
    if simulation["status"] != expected_status:
        raise AssertionError(f"expected {expected_status}, got {simulation['status']}")
    if simulation.get("abi_version") != 1:
        raise AssertionError("simulator/program interface version mismatch")
    after = {name: digest(directory / name) for name in names}
    if after != before:
        raise ValueError("runtime program or host input changed during execution")
    result = {
        "stage": stage,
        "simulation": simulation,
        "inputs": before,
        "simulator_sha256": simulator_before,
        "inputs_unchanged": True,
    }
    if expect_fault:
        if (directory / "output.bin").exists():
            raise AssertionError("fault published a successful output file")
        actual_path = directory / "output.bin.fault.bin"
    else:
        actual_path = directory / "output.bin"
    result.update(output=str(actual_path), output_sha256=digest(actual_path))
    if timing_config is not None:
        result.update(
            events=str(directory / "events.jsonl"), events_sha256=digest(directory / "events.jsonl")
        )
    (directory / "execution.json").write_text(json.dumps(result, indent=2) + "\n")
    return result

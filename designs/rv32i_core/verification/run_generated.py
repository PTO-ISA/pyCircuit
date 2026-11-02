#!/usr/bin/env python3
"""Compile and run the checked-in generated DUT with a blocking memory EEI.

The platform only packs ports and serves word-memory requests. Instruction
semantics and architectural state come from the compiled C++ or Icarus DUT.
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import select
import subprocess
import time

from verify import compare


def invoke(command, log, *, timeout=180, cwd=None):
    with log.open("w") as stream:
        result = subprocess.run(
            command, stdout=stream, stderr=subprocess.STDOUT, timeout=timeout, cwd=cwd
        )
    log.with_suffix(".command.json").write_text(json.dumps(command, indent=2) + "\n")
    if cwd is not None:
        log.with_suffix(".cwd").write_text(str(cwd) + "\n")
    log.with_suffix(".rc").write_text(str(result.returncode) + "\n")
    if result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}); see {log}")


def pack(values, port):
    value = 0
    for field in port["fields"]:
        name = field["path"][0]
        raw = values.get(name, 0)
        if not 0 <= raw < 1 << field["width"]:
            raise ValueError(f"memory input {name} exceeds its width")
        value |= raw << field["low"]
    return format(value, f"0{port['width']}b")


def unpack(bits, port):
    if len(bits) != port["width"] or set(bits) - {"0", "1"}:
        raise ValueError("active output contains an unknown or malformed value")
    raw = int(bits, 2)
    return {
        field["path"][0]: (raw >> field["low"]) & ((1 << field["width"]) - 1)
        for field in port["fields"]
    }


class Driver:
    def __init__(self, command, log):
        self.log = log.open("w")
        self.process = subprocess.Popen(
            command,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=self.log,
            text=True,
            bufsize=1,
        )

    def command(self, operation, data=None):
        self.process.stdin.write(
            operation + (" " + data if data is not None else "") + "\n"
        )
        self.process.stdin.flush()
        if not select.select([self.process.stdout], [], [], 30)[0]:
            raise RuntimeError("generated DUT response timed out")
        line = self.process.stdout.readline().strip()
        if not line or line.startswith("FAIL") or "PYCIRCUIT_PHYSICAL_FAULT" in line:
            raise RuntimeError("generated DUT failed: " + line)
        return line

    def close(self):
        self.process.stdin.close()
        result = self.process.wait(timeout=30)
        self.log.close()
        if result:
            raise RuntimeError(f"generated DUT exited with {result}")


class MemoryChannel:
    """Hold a complete accepted request until its declared completion cycle."""

    def __init__(self, latency):
        if not latency or any(type(n) is not int or n < 0 for n in latency):
            raise ValueError("memory latency must be a finite nonnegative sequence")
        self.latency, self.count, self.pending = latency, 0, None

    def ready(self, command, cycle):
        if self.pending is not None:
            return command is not None and cycle >= self.pending[1]
        return command is not None and self.latency[self.count % len(self.latency)] == 0

    def finish(self, command, ready, cycle, cancel=False):
        if cancel:
            self.pending = None
        elif self.pending is not None and command != self.pending[0]:
            raise ValueError("memory request changed before completion")
        elif command is not None:
            if ready:
                self.pending = None
                self.count += 1
            elif self.pending is None:
                self.pending = (
                    command,
                    cycle + self.latency[self.count % len(self.latency)],
                )


def memory_command(pins, channel):
    if not pins[channel["valid"]]:
        return None
    names = (
        ["address", "write", "mask", "write_data"]
        if "write" in channel
        else ["address"]
    )
    return tuple(pins[channel[name]] for name in names)


def execute(command, ports, config, output):
    output.mkdir()
    incoming = next(
        p for p in ports["inputs"] if p["ordinal"] not in ports["domains"].values()
    )
    packet = ports["outputs"][0]
    image = Path(config["image"]).read_bytes()
    base, size = config["memory_base"], config["memory_bytes"]
    if len(image) != size or config.get("reset_at"):
        raise ValueError(
            "this artifact consumer requires its declared image and initial-only reset"
        )
    memory = bytearray(image)
    channels = [
        MemoryChannel(config[name]) for name in ["instruction_latency", "data_latency"]
    ]
    driver = Driver(command, output / "driver.log")
    rows, events, stores, vectors, outputs = [], [], [], [], []
    started = time.monotonic()
    try:
        for _ in range(config.get("reset_cycles", 2)):
            driver.command("R", pack({}, incoming))
        for cycle in range(config["max_cycles"]):
            if time.monotonic() - started >= 180:
                raise RuntimeError("generated DUT exceeded 180 seconds")
            response = {}
            for _ in range(8):
                request = unpack(driver.command("Q", pack(response, incoming)), packet)
                following = {}
                for (channel, data), timing in zip(
                    [
                        (config["instruction"], image),
                        (config["data"], memory),
                    ],
                    channels,
                    strict=True,
                ):
                    active = request[channel["valid"]]
                    address = request[channel["address"]]
                    ready = timing.ready(memory_command(request, channel), cycle)
                    valid = (
                        base <= address
                        and address + 4 <= base + size
                        and address % 4 == 0
                    )
                    following[channel["ready"]] = int(ready)
                    following[channel["fault"]] = int(ready and not valid)
                    following[channel["response"]] = (
                        int.from_bytes(
                            data[address - base : address - base + 4], "little"
                        )
                        if ready and active and valid
                        else 0
                    )
                if following == response:
                    break
                response = following
            else:
                raise RuntimeError("blocking memory feedback did not settle")
            raw = pack(response, incoming)
            line = driver.command("T", raw).split()
            pins = unpack(line[0], packet)
            data = config["data"]
            if (
                pins[data["valid"]]
                and response[data["ready"]]
                and pins[data["write"]]
                and not response[data["fault"]]
            ):
                address, value, mask = [
                    pins[data[key]] for key in ["address", "write_data", "mask"]
                ]
                for lane in range(4):
                    if mask >> lane & 1:
                        memory[address - base + lane] = value >> (lane * 8) & 255
                stores.append(
                    {"cycle": cycle, "address": address, "mask": mask, "data": value}
                )
            row = {
                "cycle": cycle,
                "epoch": 0,
                "reset": False,
                "outputs": pins,
                "unchanged": line[1] == "1",
            }
            rows.append(row)
            vectors.append(raw)
            outputs.append(line[0])
            if pins[config["event"]["valid"]]:
                events.append(row)
            terminal = pins[config["event"]["valid"]] and pins[config["event"]["stop"]]
            for channel, timing in zip(
                [config["instruction"], config["data"]], channels, strict=True
            ):
                fired = bool(pins[channel["valid"]] and response[channel["ready"]])
                timing.finish(
                    memory_command(pins, channel),
                    fired,
                    cycle,
                    cancel=bool(
                        terminal and channel is config["instruction"] and not fired
                    ),
                )
            if terminal:
                break
        else:
            raise RuntimeError(
                "program did not terminate within the declared model cycles"
            )
        state = driver.command("S")
    finally:
        driver.close()
    for name, records in [("cycles.jsonl", rows), ("events.jsonl", events)]:
        (output / name).write_text("".join(json.dumps(row) + "\n" for row in records))
    (output / "inputs.mem").write_text("\n".join(vectors) + "\n")
    (output / "outputs.mem").write_text("\n".join(outputs) + "\n")
    (output / "state.bits").write_text(state + "\n")
    (output / "memory.bin").write_bytes(memory)
    (output / "execution.json").write_text(
        json.dumps(
            {
                "model_cycles": len(rows),
                "executed_ticks": len(rows),
                "skipped_idle_cycles": 0,
                "stores": stores,
            },
            indent=2,
        )
        + "\n"
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--program", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.prefix = args.prefix.resolve()
    args.program = args.program.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    source = Path(__file__).resolve().parents[1]
    # Verify committed payloads before building, without recapture or emission.
    manifest = json.loads((source / "generated/artifacts.json").read_text())
    import hashlib

    for row in manifest["files"]:
        path = source / row["path"]
        if hashlib.sha256(path.read_bytes()).hexdigest() != row["sha256"]:
            raise ValueError("artifact checksum mismatch: " + row["path"])
    build = args.output / "cpp-build"
    invoke(
        [
            "cmake",
            "-S",
            str(source / "verification/artifact_driver"),
            "-B",
            str(build),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            "-Dpycircuit_DIR=" + str(args.prefix / "share/pycircuit/cmake"),
        ],
        args.output / "configure.log",
    )
    invoke(
        ["cmake", "--build", str(build), "--parallel", "2"],
        args.output / "cpp-build.log",
    )
    rtl = source / "generated/rtl"
    receipt = json.loads((rtl / "generated.json").read_text())
    files = [rtl / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
    executable = args.output / "bridge.vvp"
    invoke(
        [
            "iverilog",
            "-g2012",
            "-s",
            "bridge",
            "-o",
            str(executable),
            *map(str, files),
            str(args.prefix / "include/verilog/dff.v"),
            str(args.prefix / "include/verilog/dffe.v"),
            str(source / "verification/artifact_driver/bridge.sv"),
        ],
        args.output / "rtl-compile.log",
    )
    config = json.loads((args.program / "simulation.json").read_text())
    config["image"] = str((args.program / config["image"]).resolve())
    reports = []
    for strategy in ["timing", "functional"]:
        for target in ["cpp", "verilog"]:
            generated = source / "generated" / ("cpp" if target == "cpp" else "rtl")
            descriptor = (
                generated / "generated.json"
                if strategy == "timing"
                else source / f"generated/profiles/functional-{target}.json"
            )
            assert (
                json.loads(descriptor.read_text())["execution_model"]["strategy"]
                == strategy
            )
            ports = json.loads((generated / "ports.json").read_text())
            output = args.output / f"{strategy}-{target}"
            command = (
                [str(build / "rv32i_artifact")]
                if target == "cpp"
                else ["vvp", str(executable)]
            )
            execute(command, ports, config, output)
            result = compare(args.program, output, generated / "ports.json")
            result.update(
                execution=strategy,
                target=target,
                basis="spike",
                executed_ticks=result["model_cycles"],
            )
            (output / "verification.json").write_text(
                json.dumps(result, indent=2) + "\n"
            )
            reports.append(result)
            print(
                strategy,
                target,
                result["status"],
                result["retired"],
                result["model_cycles"],
                flush=True,
            )
    reference = (args.output / "timing-cpp/state.bits").read_bytes()
    for row in reports:
        assert (
            args.output / f"{row['execution']}-{row['target']}/state.bits"
        ).read_bytes() == reference
    (args.output / "verification.json").write_text(
        json.dumps(
            {
                "status": "PASS",
                "scope": "Checked-in generated payloads, fast memory EEI, fresh independent program reference",
                "views": reports,
            },
            indent=2,
        )
        + "\n"
    )


if __name__ == "__main__":
    main()

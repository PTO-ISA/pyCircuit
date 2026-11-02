"""Explicit blocking-word-memory adapter and fixed-point replay.

Addresses, byte masks and values come only from the generated DUT. This module
has no instruction decoder, register arithmetic or CPU-name dispatch.
"""

from __future__ import annotations

import json
import time
from pathlib import Path

from ._clocked import ClockedBridge, build_bridge
from ._driver import _DriverError


def fields(port):
    result = {}
    for field in port["fields"]:
        if len(field["path"]) != 1:
            raise _DriverError("word-memory IO requires flat records")
        result[field["path"][0]] = field
    return result


def pack(values, port):
    result = 0
    for name, field in fields(port).items():
        value = values.get(name, 0)
        if type(value) is not int or value < 0 or value >= 1 << field["width"]:
            raise _DriverError(f"input {name} does not fit its declared width")
        result |= value << field["low"]
    return format(result, f"0{port['width']}b")


def unpack(bits, port):
    if set(bits) - {"0", "1"}:
        raise _DriverError("available level/event output contains unknown bits")
    value = int(bits, 2)
    return {
        name: value >> field["low"] & ((1 << field["width"]) - 1)
        for name, field in fields(port).items()
    }


def load_config(path):
    path = Path(path).resolve()
    data = json.loads(path.read_text())
    if data.get("schema") != "pycircuit-clocked-memory-v1":
        raise _DriverError("unsupported clocked memory schema")
    data["image"] = (path.parent / data["image"]).resolve()
    return data


class PendingMemory:
    def __init__(self, latency):
        self.latency = latency
        if not latency or any(type(n) is not int or n < 0 for n in latency):
            raise _DriverError(
                "latency must be a nonempty finite nonnegative cycle sequence"
            )
        self.pending = None
        self.count = 0

    def response(self, command, cycle):
        if self.pending is not None:
            return command is not None and cycle >= self.pending[1]
        return command is not None and self.latency[self.count % len(self.latency)] == 0

    def finish(self, command, ready, cycle, cancel=False):
        if cancel:
            self.pending = None
            return
        if self.pending is not None and command != self.pending[0]:
            raise _DriverError("request withdrawn or changed before completion")
        if command is not None:
            if ready:
                self.pending = None
                self.count += 1
            elif self.pending is None:
                self.pending = (
                    command,
                    cycle + self.latency[self.count % len(self.latency)],
                )


def run_memory(generated, config, output, prefix, timeout=180):
    generated, output = Path(generated).resolve(), Path(output).resolve()
    cfg = load_config(config)
    output.mkdir(parents=True, exist_ok=True)
    descriptor = json.loads((generated / "generated.json").read_text())[
        "execution_model"
    ]
    if descriptor["protocol"] != "clocked_fixedpoint_v1":
        raise _DriverError("memory execution requires a checked fixed-point artifact")
    functional = descriptor["strategy"] == "functional"
    executable, ports = build_bridge(
        generated, output / "bridge", prefix, descriptor["state_output"], timeout
    )
    inputs = next(
        p for p in ports["inputs"] if p["ordinal"] not in ports["domains"].values()
    )
    pins = ports["outputs"][1 - descriptor["state_output"]]
    im, dm, event = cfg["instruction"], cfg["data"], cfg["event"]
    if {im[k] for k in ("ready", "response", "fault")} & {
        dm[k] for k in ("ready", "response", "fault")
    }:
        raise _DriverError(
            "independent memory channels require distinct response fields"
        )
    for role, mapping in (("instruction", im), ("data", dm)):
        in_keys = ["ready", "response", "fault"]
        for key, name in mapping.items():
            if name not in fields(inputs if key in in_keys else pins):
                raise _DriverError(f"{role}.{key}: field absent from generated IO")
    for name in event.values():
        if name not in fields(pins):
            raise _DriverError("event field absent from output")
    image = cfg["image"].read_bytes()
    size, base = cfg["memory_bytes"], cfg["memory_base"]
    if len(image) != size:
        raise _DriverError("image size differs from memory contract")
    data = bytearray(image)
    instruction = PendingMemory(cfg["instruction_latency"])
    memory = PendingMemory(cfg["data_latency"])
    driver = ClockedBridge(executable)
    records, events, vectors, outputs, stores = [], [], [], [], []
    status = "timeout"
    try:
        zero = pack({}, inputs)
        for _ in range(cfg.get("reset_cycles", 2)):
            driver.command("R", zero)
        epoch = 0
        cycle = 0
        skipped = []
        deadline = time.monotonic() + timeout
        while cycle < cfg["max_cycles"]:
            if time.monotonic() >= deadline:
                raise _DriverError("clocked execution exceeded its wall-time budget")
            if cycle in cfg.get("reset_at", []):
                packet, stable = driver.command("R", zero).split()
                instruction.finish(None, False, cycle, cancel=True)
                memory.finish(None, False, cycle, cancel=True)
                records.append(
                    {
                        "cycle": cycle,
                        "epoch": epoch,
                        "outputs": unpack(packet, pins),
                        "unchanged": stable == "1",
                        "environment_event": True,
                        "reset": True,
                    }
                )
                vectors.append(zero)
                outputs.append(packet)
                epoch += 1
                cycle += 1
                continue
            response = {}
            # Valid may depend on downstream ready. Pure Q queries settle the
            # finite backwards-ready chain without committing any proposal.
            for _iteration in range(8):
                raw = pack(response, inputs)
                out = unpack(driver.command("Q", raw), pins)
                icmd = (out[im["address"]],) if out[im["valid"]] else None
                dcmd = (
                    (
                        out[dm["address"]],
                        out[dm["write"]],
                        out[dm["mask"]],
                        out[dm["write_data"]],
                    )
                    if out[dm["valid"]]
                    else None
                )
                iready = instruction.response(icmd, cycle)
                dready = memory.response(dcmd, cycle)
                following = {}
                for channel, command, ready, bytes_ in (
                    (im, icmd, iready, image),
                    (dm, dcmd, dready, data),
                ):
                    following[channel["ready"]] = int(ready)
                    address = command[0] if command else base
                    valid = (
                        base <= address
                        and address + 4 <= base + size
                        and address % 4 == 0
                    )
                    following[channel["fault"]] = int(ready and not valid)
                    following[channel["response"]] = (
                        int.from_bytes(
                            bytes_[address - base : address - base + 4], "little"
                        )
                        if ready and valid
                        else 0
                    )
                if following == response:
                    break
                response = following
            else:
                raise _DriverError("ready/valid combinational query did not settle")
            raw = pack(response, inputs)
            line = driver.command("T", raw).split()
            packet, unchanged = line[0], line[1] == "1"
            out = unpack(packet, pins)
            i_fire = bool(out[im["valid"]] and response[im["ready"]])
            d_fire = bool(out[dm["valid"]] and response[dm["ready"]])
            if d_fire:
                if out[dm["write"]] and not response[dm["fault"]]:
                    address, value, mask = (
                        out[dm["address"]],
                        out[dm["write_data"]],
                        out[dm["mask"]],
                    )
                    for lane in range(4):
                        if mask >> lane & 1:
                            data[address - base + lane] = value >> (lane * 8) & 255
                    stores.append(
                        {
                            "cycle": cycle,
                            "address": address,
                            "mask": mask,
                            "data": value,
                        }
                    )
            event_fire = bool(out[event["valid"]])
            terminal = event_fire and out[event["stop"]]
            instruction.finish(
                icmd, i_fire, cycle, cancel=bool(terminal and not i_fire)
            )
            memory.finish(dcmd, d_fire, cycle)
            record = {
                "cycle": cycle,
                "epoch": epoch,
                "reset": False,
                "outputs": out,
                "unchanged": unchanged,
                "environment_event": i_fire or d_fire or event_fire,
            }
            records.append(record)
            vectors.append(raw)
            outputs.append(packet)
            if event_fire:
                events.append(record)
            if terminal:
                status = "event_stop"
                cycle += 1
                break
            following = cycle + 1
            if functional and unchanged and not record["environment_event"]:
                deadlines = [
                    item.pending[1]
                    for item in (instruction, memory)
                    if item.pending is not None and item.pending[1] > cycle
                ]
                deadlines += [n for n in cfg.get("reset_at", []) if n > cycle]
                following = min(deadlines + [cfg["max_cycles"]])
                if following > cycle + 1:
                    skipped.append(
                        {
                            "after_cycle": cycle,
                            "until_cycle": following,
                            "idle_cycles": following - cycle - 1,
                            "complete_state_unchanged": True,
                            "no_environment_event": True,
                            "input_bits": raw,
                        }
                    )
            cycle = following
        final_state = driver.command("S")
    finally:
        driver.close()
    (output / "cycles.jsonl").write_text(
        "".join(json.dumps(row) + "\n" for row in records)
    )
    (output / "events.jsonl").write_text(
        "".join(json.dumps(row) + "\n" for row in events)
    )
    (output / "inputs.mem").write_text("\n".join(vectors) + "\n")
    (output / "outputs.mem").write_text("\n".join(outputs) + "\n")
    (output / "state.bits").write_text(final_state + "\n")
    (output / "memory.bin").write_bytes(data)
    report = {
        "schema": "pycircuit-clocked-memory-run-v1",
        "status": status,
        "strategy": descriptor["strategy"],
        "model_cycles": cycle,
        "executed_ticks": len(records),
        "skipped_idle_cycles": sum(row["idle_cycles"] for row in skipped),
        "skips": skipped,
        "events": len(events),
        "stores": stores,
        "instruction_transactions": instruction.count,
        "data_transactions": memory.count,
        "validation": "execution only; independent expected events and state belong to verifier",
    }
    (output / "execution.json").write_text(json.dumps(report, indent=2) + "\n")
    if status == "timeout":
        raise _DriverError(
            "clocked model reached cycle budget without the declared stop event"
        )
    return report

#!/usr/bin/env python3
"""Test-owned ELF/Spike retirement oracle. Never imported by generated hardware."""
from __future__ import annotations
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess

BASE = 0x80000000
SIZE = 65536
ABI = "zero ra sp gp tp t0 t1 t2 s0 s1 a0 a1 a2 a3 a4 a5 a6 a7 s2 s3 s4 s5 s6 s7 s8 s9 s10 s11 t3 t4 t5 t6".split()


def elf_image(path):
    raw = Path(path).read_bytes()
    if raw[:7] != b"\x7fELF\x01\x01\x01" or struct.unpack_from("<H", raw, 18)[0] != 243:
        raise ValueError("expected RV32 little-endian ELF")
    image = bytearray(SIZE)
    phoff = struct.unpack_from("<I", raw, 28)[0]
    entsize, count = struct.unpack_from("<HH", raw, 42)
    for n in range(count):
        kind, offset, address, _, filesz, memsz, _, _ = struct.unpack_from(
            "<8I", raw, phoff + n * entsize
        )
        if kind != 1 or memsz == 0:
            continue
        if address < BASE or address + memsz > BASE + SIZE:
            raise ValueError("ELF segment outside declared EEI")
        image[address - BASE : address - BASE + filesz] = raw[offset : offset + filesz]
    return image


def oracle(elf, spike, output):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    commands = output / "spike.cmd"
    commands.write_text(
        "until pc 0 0\nreg 0 mcause\nreg 0 mepc\nreg 0 mtval\nreg 0\nq\n"
    )
    command = [
        str(spike),
        "--isa=RV32I",
        "--priv=m",
        "--pcs=0:0x80000000",
        "-m0x80000000:0x10000",
        "-l",
        "--log-commits",
        "-d",
        "--debug-cmd=" + str(commands),
        str(elf),
    ]
    run = subprocess.run(
        command, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=30
    )
    (output / "spike.log").write_text(run.stdout)
    if run.returncode:
        raise RuntimeError("Spike failed: " + str(run.returncode))
    image = elf_image(elf)
    events = []
    for line in run.stdout.splitlines():
        match = re.match(r"core\s+0:\s+3\s+0x([0-9a-f]+)\s+\(0x([0-9a-f]+)\)(.*)", line)
        if not match:
            continue
        pc, inst = [int(v, 16) for v in match.group(1, 2)]
        suffix = match[3]
        row = {
            "pc": pc,
            "instruction": inst,
            "trap": 0,
            "rd_write": 0,
            "load": 0,
            "store": 0,
        }
        rd = re.search(r"\bx(\d+)\s+0x([0-9a-f]+)", suffix)
        if rd and int(rd[1]):
            row.update(rd_write=1, rd=int(rd[1]), value=int(rd[2], 16))
        memory = re.search(r"\bmem 0x([0-9a-f]+)(?: 0x([0-9a-f]+))?", suffix)
        if memory:
            address = int(memory[1], 16)
            width = 1 << ((inst >> 12) & 3)
            mask = ((1 << width) - 1) << (address & 3)
            row.update(address=address, mask=mask)
            if memory[2] is not None:
                value = int(memory[2], 16) & ((1 << (8 * width)) - 1)
                row.update(store=1, mem_data=value)
                image[address - BASE : address - BASE + width] = value.to_bytes(
                    width, "little"
                )
            else:
                row.update(
                    load=1,
                    mem_data=int.from_bytes(
                        image[address - BASE : address - BASE + width], "little"
                    ),
                )
        events.append(row)
    csrs = re.findall(r"^0x([0-9a-f]+)\s*$", run.stdout, re.M)
    if len(csrs) != 3:
        raise ValueError("Spike did not produce the precise trap CSR triple")
    cause, pc, tval = [int(v, 16) for v in csrs]
    original = elf_image(elf)
    inst = (
        int.from_bytes(original[pc - BASE : pc - BASE + 4], "little")
        if BASE <= pc < BASE + SIZE
        else 0
    )
    events.append(
        {
            "pc": pc,
            "instruction": inst,
            "trap": 1,
            "cause": cause,
            "tval": tval,
            "rd_write": 0,
            "load": 0,
            "store": 0,
        }
    )
    registers = {
        ABI.index(name): int(value, 16)
        for name, value in re.findall(r"\b([a-z][a-z0-9]*): 0x([0-9a-f]+)", run.stdout)
        if name in ABI
    }
    if len(registers) != 32:
        raise ValueError("incomplete Spike register dump")
    report = {
        "events": events,
        "registers": [registers[n] for n in range(32)],
        "command": command,
        "returncode": run.returncode,
    }
    (output / "oracle.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "oracle-memory.bin").write_bytes(image)
    return report


def compare(expected, actual, ports_path, *, basis="spike", epoch=None):
    expected, actual = Path(expected), Path(actual)
    ref = json.loads(
        (
            expected / ("spec-oracle.json" if basis == "spec" else "oracle.json")
        ).read_text()
    )
    rows = [
        json.loads(line) for line in (actual / "events.jsonl").read_text().splitlines()
    ]
    if epoch is not None:
        rows = [row for row in rows if row["epoch"] == epoch]
    if len(rows) != len(ref["events"]):
        raise AssertionError(
            f'retirement count: expected {len(ref["events"])}, got {len(rows)}'
        )
    for n, (want, record) in enumerate(zip(ref["events"], rows, strict=True)):
        got = record["outputs"]
        for key, value in want.items():
            if got["retire_" + key] != value:
                raise AssertionError(
                    f'first retirement difference #{n} cycle {record["cycle"]} {key}: expected {value:#x}, got {got["retire_"+key]:#x}; pc={want["pc"]:#x}'
                )
        if got["retire_order"] != n:
            raise AssertionError("retirement order is not contiguous")
    stores = json.loads((actual / "execution.json").read_text())["stores"]
    expected_stores = [row for row in ref["events"] if row.get("store")]
    if len(stores) != len(expected_stores):
        raise AssertionError("external stores are not exactly once per retired store")
    for got, want in zip(stores, expected_stores, strict=True):
        lane_mask = sum(255 << (8 * n) for n in range(4) if want["mask"] >> n & 1)
        shifted = want["mem_data"] << (8 * (want["address"] & 3))
        if (got["address"], got["mask"], got["data"] & lane_mask) != (
            want["address"] & ~3,
            want["mask"],
            shifted & lane_mask,
        ):
            raise AssertionError(
                "external store differs from independently retired bytes"
            )
    if (
        expected / ("spec-memory.bin" if basis == "spec" else "oracle-memory.bin")
    ).read_bytes() != (actual / "memory.bin").read_bytes():
        raise AssertionError("final RAM differs from independent Spike stores")
    if (expected / "c-signature.json").exists():
        signature = json.loads((expected / "c-signature.json").read_text())
        offset = signature["address"] - BASE
        want = b"".join(value.to_bytes(4, "little") for value in signature["words"])
        if (actual / "memory.bin").read_bytes()[offset : offset + len(want)] != want:
            raise AssertionError(
                "compiled C signature differs from independent C-source math"
            )
    ports = json.loads(Path(ports_path).read_text())
    state = ports["outputs"][1]
    bits = (actual / "state.bits").read_text().strip()
    if set(bits) - {"0", "1"}:
        raise AssertionError("final architectural state contains X/Z")
    packed = int(bits, 2)
    registers = {}
    for field in state["fields"]:
        path = field["path"]
        if path == ["registers"] and field["width"] == 1024:
            for n in range(32):
                registers[n] = packed >> (field["low"] + (31 - n) * 32) & 0xFFFFFFFF
    architectural = {}
    for field in state["fields"]:
        architectural[tuple(field["path"])] = packed >> field["low"] & (
            (1 << field["width"]) - 1
        )
    for field, value in {
        ("arch_pc",): ref["events"][-1]["pc"],
        ("order",): len(ref["events"]),
        ("halted",): 1,
        ("stopping",): 1,
        ("decode_stage", "valid"): 0,
        ("execute_stage", "valid"): 0,
        ("memory_stage", "valid"): 0,
        ("writeback_stage", "valid"): 0,
        ("fetch_buffer", "valid"): 0,
        ("fetch_busy",): 0,
    }.items():
        if architectural.get(field) != value:
            raise AssertionError(
                "final architectural/control state differs: " + ".".join(field)
            )
    if len(registers) != 32:
        raise AssertionError("complete register witness absent")
    if [registers[n] for n in range(32)] != ref["registers"]:
        raise AssertionError("final GPRs differ from Spike")
    cycles = [
        json.loads(line) for line in (actual / "cycles.jsonl").read_text().splitlines()
    ]
    return {
        "status": "PASS",
        "retired": len(rows) - 1,
        "traps": 1,
        "model_cycles": json.loads((actual / "execution.json").read_text())[
            "model_cycles"
        ],
        "max_occupied": max(
            sum(row["outputs"]["stage_" + s] for s in ["if", "id", "ex", "mem", "wb"])
            for row in cycles
        ),
        "load_use_cycles": sum(row["outputs"]["load_use"] for row in cycles),
        "memory_wait_cycles": sum(row["outputs"]["memory_wait"] for row in cycles),
        "redirects": sum(row["outputs"]["redirected"] for row in cycles),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    p = sub.add_parser("oracle")
    p.add_argument("elf", type=Path)
    p.add_argument("--spike", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p = sub.add_parser("compare")
    p.add_argument("expected", type=Path)
    p.add_argument("actual", type=Path)
    p.add_argument("--ports", type=Path, required=True)
    args = parser.parse_args()
    if args.command == "oracle":
        oracle(args.elf, args.spike, args.output)
    else:
        print(json.dumps(compare(args.expected, args.actual, args.ports), indent=2))


if __name__ == "__main__":
    main()

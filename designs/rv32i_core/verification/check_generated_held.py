#!/usr/bin/env python3
"""Apply main's independent held-operand fixtures to the published DUT files."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from run_generated import execute
from test_held_operands import BASE, DATA, LOAD_VALUE, PROGRAMS, SENTINEL


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--compiled",
        type=Path,
        required=True,
        help="Successful run_generated.py output with its compiled DUTs",
    )
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.compiled = args.compiled.resolve()
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    assert (
        json.loads((args.compiled / "verification.json").read_text())["status"]
        == "PASS"
    )
    source = Path(__file__).resolve().parents[1]
    reports = []
    for case, (words, expected_x1, expected_x4, store_value) in PROGRAMS.items():
        directory = args.output / case
        directory.mkdir()
        image = bytearray(65536)
        for index, word in enumerate(words):
            image[index * 4 : index * 4 + 4] = word.to_bytes(4, "little")
        image[0x8000:0x8004] = LOAD_VALUE.to_bytes(4, "little")
        image[0x8004:0x8008] = SENTINEL.to_bytes(4, "little")
        expected_memory = bytearray(image)
        if store_value is not None:
            expected_memory[0x8004:0x8008] = store_value.to_bytes(4, "little")
        (directory / "image.bin").write_bytes(image)
        config = json.loads((source / "simulation.json").read_text())
        config.update(
            image=str(directory / "image.bin"),
            max_cycles=100,
            instruction_latency=[0],
            data_latency=[2],
        )
        states = []
        for strategy in ["timing", "functional"]:
            for target in ["cpp", "verilog"]:
                generated = source / "generated" / ("cpp" if target == "cpp" else "rtl")
                ports = json.loads((generated / "ports.json").read_text())
                command = (
                    [str(args.compiled / "cpp-build/rv32i_artifact")]
                    if target == "cpp"
                    else ["vvp", str(args.compiled / "bridge.vvp")]
                )
                output = directory / f"{strategy}-{target}"
                execute(command, ports, config, output)
                events = [
                    json.loads(line)["outputs"]
                    for line in (output / "events.jsonl").read_text().splitlines()
                ]
                assert [row["retire_instruction"] for row in events] == words
                assert [row["retire_pc"] for row in events] == [
                    BASE + 4 * n for n in range(5)
                ]
                assert [row["retire_order"] for row in events] == list(range(5))
                assert [row["retire_trap"] for row in events] == [0, 0, 0, 0, 1]
                assert events[-1]["retire_cause"] == 3
                assert events[-1]["retire_tval"] == BASE + 16
                assert events[2]["retire_value"] == LOAD_VALUE
                state = ports["outputs"][1]
                bits = (output / "state.bits").read_text().strip()
                assert len(bits) == state["width"] and not set(bits) - {"0", "1"}
                values = {
                    tuple(f["path"]): (int(bits, 2) >> f["low"])
                    & ((1 << f["width"]) - 1)
                    for f in state["fields"]
                }
                gprs = [
                    values[("registers",)] >> ((31 - i) * 32) & 0xFFFFFFFF
                    for i in range(32)
                ]
                expected_gprs = [0] * 32
                (
                    expected_gprs[1],
                    expected_gprs[2],
                    expected_gprs[4],
                    expected_gprs[20],
                ) = (expected_x1, LOAD_VALUE, expected_x4, DATA)
                assert gprs == expected_gprs
                assert values[("arch_pc",)] == BASE + 16 and values[("halted",)] == 1
                assert (output / "memory.bin").read_bytes() == expected_memory
                execution = json.loads((output / "execution.json").read_text())
                assert len(execution["stores"]) == (0 if store_value is None else 1)
                if store_value is not None:
                    store = execution["stores"][0]
                    assert (store["address"], store["mask"], store["data"]) == (
                        DATA + 4,
                        15,
                        store_value,
                    )
                cycles = [
                    json.loads(line)
                    for line in (output / "cycles.jsonl").read_text().splitlines()
                ]
                waits = sum(row["outputs"]["memory_wait"] for row in cycles)
                assert waits >= 2
                states.append(bits)
                reports.append(
                    {
                        "case": case,
                        "strategy": strategy,
                        "target": target,
                        "status": "PASS",
                        "x4": gprs[4],
                        "memory_wait_cycles": waits,
                    }
                )
                print(case, strategy, target, "PASS", flush=True)
        assert len(set(states)) == 1
    (args.output / "verification.json").write_text(
        json.dumps(
            {
                "status": "PASS",
                "scope": "Current main held-operand fixtures against checked-in generated DUTs",
                "views": reports,
            },
            indent=2,
        )
        + "\n"
    )


if __name__ == "__main__":
    main()

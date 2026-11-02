"""Generated-DUT forwarding regression with independently encoded RV32I words.

Run with PYC_TOOLCHAIN_ROOT selecting an installed compiler/runtime. No RISC-V
assembler or Spike is required: the finite programs and expected values below
are test-owned and never imported into the hardware or compiler.
"""

import json
import os
from pathlib import Path
import shutil
import subprocess

import pytest

BASE = 0x80000000
DATA = BASE + 0x8000
LOAD_VALUE = 0x12345678
SENTINEL = 0xDEADBEEF
SOURCE = Path(__file__).resolve().parents[1]

# LUI x20,0x80008; producer; LW x2,0(x20); consumer; EBREAK.
# Fast instruction fetch places the consumer in EX before producer retirement.
# A delayed load holds EX after the producer has disappeared from WB.
PROGRAMS = {
    "alu": ([0x80008A37, 0x00500093, 0x000A2103, 0x00008233, 0x00100073], 5, 5, None),
    "store_data": (
        [0x80008A37, 0x00500093, 0x000A2103, 0x001A2223, 0x00100073],
        5,
        0,
        5,
    ),
    "store_address": (
        [0x80008A37, 0x004A0093, 0x000A2103, 0x0000A023, 0x00100073],
        DATA + 4,
        0,
        0,
    ),
}


@pytest.fixture(scope="module")
def installation(tmp_path_factory):
    prefix = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if not prefix:
        pytest.skip("PYC_TOOLCHAIN_ROOT must select an installed compiler/runtime")
    prefix = Path(prefix).resolve()
    assert (prefix / "bin/pycircuit").is_file(), "installed pycircuit CLI is absent"
    work = tmp_path_factory.mktemp("held-operands")
    project = work / "project"
    project.mkdir()
    for source in SOURCE.glob("*.py"):
        shutil.copy2(source, project / source.name)
    shutil.copy2(SOURCE / "CMakeLists.txt", project / "CMakeLists.txt")
    return prefix, work, project


def state_fields(build, target):
    ports = json.loads((build / target / "ports.json").read_text())
    receipt = json.loads((build / target / "generated.json").read_text())
    state = ports["outputs"][receipt["execution_model"]["state_output"]]
    bits = (build / "clocked-run/state.bits").read_text().strip()
    assert len(bits) == state["width"] and not set(bits) - {"0", "1"}
    value = int(bits, 2)
    fields = {
        tuple(field["path"]): (value >> field["low"]) & ((1 << field["width"]) - 1)
        for field in state["fields"]
    }
    registers = next(
        field for field in state["fields"] if field["path"] == ["registers"]
    )
    assert registers["width"] == 32 * 32
    packed = fields[("registers",)]
    # The public Table packing contract puts index zero in the high element.
    gprs = [(packed >> ((31 - index) * 32)) & 0xFFFFFFFF for index in range(32)]
    return bits, fields, gprs


@pytest.mark.parametrize("case", PROGRAMS)
def test_held_writeback_operands(installation, case):
    prefix, work, project = installation
    words, expected_x1, expected_x4, store_value = PROGRAMS[case]
    case_dir = work / case
    case_dir.mkdir()
    image = bytearray(65536)
    for index, word in enumerate(words):
        image[index * 4 : index * 4 + 4] = word.to_bytes(4, "little")
    image[0x8000:0x8004] = LOAD_VALUE.to_bytes(4, "little")
    image[0x8004:0x8008] = SENTINEL.to_bytes(4, "little")
    (case_dir / "image.bin").write_bytes(image)
    expected_memory = bytearray(image)
    if store_value is not None:
        expected_memory[0x8004:0x8008] = store_value.to_bytes(4, "little")
    config = json.loads((SOURCE / "simulation.json").read_text())
    config.update(
        image="image.bin",
        max_cycles=100,
        instruction_latency=[0],
        data_latency=[2],
    )
    config_path = case_dir / "simulation.json"
    config_path.write_text(json.dumps(config, indent=2) + "\n")
    states = []
    for strategy in ("timing", "functional"):
        for target in ("cpp", "verilog"):
            view = f"{strategy}-{target}"
            build = work / "build" / view
            command = [
                str(prefix / "bin/pycircuit"),
                "run",
                str(project),
                "--model",
                "rv32i_core.core.Core",
                "--execution",
                strategy,
                "--target",
                target,
                "--toolchain",
                str(prefix),
                "--memory",
                str(config_path),
                "--build-dir",
                str(build),
                "--timeout",
                "180",
            ]
            run = subprocess.run(command, capture_output=True, text=True, timeout=540)
            (case_dir / f"{view}.log").write_text(run.stdout + run.stderr)
            assert run.returncode == 0, run.stdout[-2000:] + run.stderr[-2000:]
            output = build / "clocked-run"
            shutil.copytree(
                output, case_dir / view, ignore=shutil.ignore_patterns("bridge")
            )
            events = [
                json.loads(line)["outputs"]
                for line in (output / "events.jsonl").read_text().splitlines()
            ]
            assert len(events) == len(words)
            assert [event["retire_order"] for event in events] == list(range(5))
            assert [event["retire_pc"] for event in events] == [
                BASE + 4 * n for n in range(5)
            ]
            assert [event["retire_instruction"] for event in events] == words
            assert [event["retire_trap"] for event in events] == [0, 0, 0, 0, 1]
            assert events[-1]["retire_cause"] == 3
            assert events[-1]["retire_tval"] == BASE + 16
            assert events[2]["retire_load"] == 1
            assert events[2]["retire_value"] == LOAD_VALUE
            assert events[2]["retire_address"] == DATA
            if store_value is not None:
                assert events[3]["retire_store"] == 1
                assert events[3]["retire_address"] == DATA + 4
                assert events[3]["retire_mem_data"] == store_value
            else:
                assert events[3]["retire_rd"] == 4
                assert events[3]["retire_rd_write"] == 1
                assert events[3]["retire_value"] == 5
            execution = json.loads((output / "execution.json").read_text())
            stores = execution["stores"]
            assert len(stores) == (0 if store_value is None else 1)
            if stores:
                assert stores[0]["address"] == DATA + 4
                assert stores[0]["mask"] == 15
                assert stores[0]["data"] == store_value
            assert (output / "memory.bin").read_bytes() == expected_memory
            cycles = [
                json.loads(line)
                for line in (output / "cycles.jsonl").read_text().splitlines()
            ]
            assert sum(row["outputs"]["memory_wait"] for row in cycles) >= 2
            bits, fields, gprs = state_fields(build, target)
            expected_gprs = [0] * 32
            expected_gprs[1] = expected_x1
            expected_gprs[2] = LOAD_VALUE
            expected_gprs[4] = expected_x4
            expected_gprs[20] = DATA
            assert gprs == expected_gprs
            assert fields[("arch_pc",)] == BASE + 16
            assert fields[("halted",)] == 1
            states.append(bits)
        assert (work / "build" / f"{strategy}-cpp/core.ac").read_bytes() == (
            work / "build" / f"{strategy}-verilog/core.ac"
        ).read_bytes()
    assert len(set(states)) == 1

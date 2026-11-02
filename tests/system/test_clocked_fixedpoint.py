"""Installed common-IR finite-state profile, independent non-CPU expectations."""

import json
import os
import shutil
import subprocess
from pathlib import Path

import pytest

pytestmark = pytest.mark.system
ROOT = Path(__file__).resolve().parents[2]


def test_clocked_hierarchy_and_complete_witness(tmp_path):
    prefix = os.environ.get("PYC_TOOLCHAIN_ROOT")
    if not prefix:
        pytest.skip("installed compiler/runtime required")
    cli = str(Path(prefix) / "bin/pycircuit")
    project = tmp_path / "project"
    shutil.copytree(ROOT / "examples/clocked_accumulator", project)
    (project / "image.bin").write_bytes(
        b"".join(n.to_bytes(4, "little") for n in [3, 5, 7, 11])
    )
    views = {}
    for strategy in ("functional", "timing"):
        for target in ("cpp", "verilog"):
            build = tmp_path / f"{strategy}-{target}"
            run = subprocess.run(
                [
                    cli,
                    "run",
                    str(project),
                    "--model",
                    "clocked_accumulator.model.Accumulator",
                    "--execution",
                    strategy,
                    "--target",
                    target,
                    "--memory",
                    str(project / "simulation.json"),
                    "--build-dir",
                    str(build),
                ],
                text=True,
                capture_output=True,
                timeout=180,
            )
            (tmp_path / f"{strategy}-{target}.log").write_text(run.stdout + run.stderr)
            assert run.returncode == 0, run.stdout[-2000:] + run.stderr[-2000:]
            output = build / "clocked-run"
            trace = [
                json.loads(line)
                for line in (output / "events.jsonl").read_text().splitlines()
            ]
            assert [row["cycle"] for row in trace] == [4, 13, 18, 27]
            assert [row["outputs"]["total"] for row in trace] == [4, 10, 18, 30]
            assert [row["outputs"]["address"] for row in trace] == [0, 4, 8, 12]
            report = json.loads((output / "execution.json").read_text())
            assert report["model_cycles"] == 28
            assert report["executed_ticks"] == (8 if strategy == "functional" else 28)
            assert report["stores"] == [] and report["data_transactions"] == 0
            witness = int((output / "state.bits").read_text(), 2)
            assert witness & ((1 << 32) - 1) == 30
            assert witness >> 32 & 7 == 4
            assert [witness >> (35 + (3 - n) * 32) & 0xFFFFFFFF for n in range(4)] == [
                4,
                6,
                8,
                12,
            ]
            views[strategy, target] = trace, witness
    assert all(view == views["timing", "cpp"] for view in views.values())

    # A helper-only source unit emits a header, not a phantom .cpp. Re-running
    # the same installed project must reuse its verified generated computation.
    build = tmp_path / "timing-cpp"
    tracked = [build / "cpp/generated.json", build / "clocked-run/bridge/build/clocked"]
    before_times = [path.stat().st_mtime_ns for path in tracked]
    repeat = subprocess.run(
        [
            cli,
            "run",
            str(project),
            "--model",
            "clocked_accumulator.model.Accumulator",
            "--execution",
            "timing",
            "--target",
            "cpp",
            "--memory",
            str(project / "simulation.json"),
            "--build-dir",
            str(build),
        ],
        text=True,
        capture_output=True,
        timeout=180,
    )
    assert repeat.returncode == 0, repeat.stdout + repeat.stderr
    assert [path.stat().st_mtime_ns for path in tracked] == before_times

    # Actual generated System lifecycle: inactive unknown data cannot commit,
    # and an active unknown guard fails before any owner state changes.
    build = tmp_path / "timing-cpp"
    ports = json.loads((build / "cpp/ports.json").read_text())
    incoming = ports["inputs"][0]
    fields = {field["path"][0]: field for field in incoming["fields"]}
    zero = "0" * incoming["width"]

    def unknown(name):
        result = list(zero)
        field = fields[name]
        for bit in range(field["low"], field["low"] + field["width"]):
            result[-1 - bit] = "x"
        return "".join(result)

    process = subprocess.Popen(
        [str(build / "clocked-run/bridge/build/clocked")],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    def command(text):
        process.stdin.write(text + "\n")
        process.stdin.flush()
        return process.stdout.readline().strip()

    command("R " + zero)
    before = command("S")
    inactive = command("T " + unknown("data"))
    assert not inactive.startswith("FAIL") and command("S") == before
    failed = command("T " + unknown("ready"))
    assert failed == "FAIL " + before
    assert command("S") == before
    process.stdin.close()
    assert process.wait(timeout=30) == 0

    # The same inactive/active unknown boundary is exercised on real physical
    # RTL, observing the complete committed witness rather than accepting a log.
    rtl_build = tmp_path / "timing-verilog"
    generated = rtl_build / "verilog"
    receipt = json.loads((generated / "generated.json").read_text())
    rtl_ports = json.loads((generated / "ports.json").read_text())
    domain = rtl_ports["domains"]
    clock, reset = [
        rtl_ports["inputs"][domain[name]]["wire"] for name in ("clock", "reset")
    ]
    incoming = rtl_ports["inputs"][0]
    packet, state = rtl_ports["outputs"]
    ready = next(field for field in incoming["fields"] if field["path"] == ["ready"])
    data = next(field for field in incoming["fields"] if field["path"] == ["data"])
    probe = tmp_path / "unknown_probe.sv"
    probe.write_text(
        f"""module unknown_probe;
reg {clock}=0,{reset}=1;
reg [{incoming['width']-1}:0] {incoming['wire']}=0;
wire [{packet['width']-1}:0] {packet['wire']};
wire [{state['width']-1}:0] {state['wire']};
reg [{state['width']-1}:0] before_state;
{receipt['root_rtl_name']} dut(.{clock}({clock}),.{reset}({reset}),.{incoming['wire']}({incoming['wire']}),.{packet['wire']}({packet['wire']}),.{state['wire']}({state['wire']}));
initial begin
 #10;{clock}=1;#10;{clock}=0;{reset}=0;#10;
 before_state={state['wire']};
 {incoming['wire']}[{data['low']} +: {data['width']}]='x;
 #10;{clock}=1;#10;{clock}=0;#10;
 if({state['wire']}!==before_state)$fatal(1,"inactive unknown committed state");
 {incoming['wire']}=0;{incoming['wire']}[{ready['low']}]='x;
 #10;{clock}=1;#10;{clock}=0;#10;
 if({state['wire']}!==before_state)$fatal(1,"active unknown committed state");
 $display("CLOCKED_UNKNOWN_ZERO_EFFECT_PASS");$finish;
end
endmodule
"""
    )
    rtl_files = [
        generated / row["path"] for row in receipt["files"] if row["role"] == "rtl"
    ]
    rtl_files.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
    compile_probe = subprocess.run(
        [
            "iverilog",
            "-g2012",
            "-s",
            "unknown_probe",
            "-o",
            str(tmp_path / "unknown.vvp"),
            *map(str, rtl_files),
            str(Path(prefix) / "include/verilog/dff.v"),
            str(Path(prefix) / "include/verilog/dffe.v"),
            str(probe),
        ],
        text=True,
        capture_output=True,
        timeout=60,
    )
    assert compile_probe.returncode == 0, compile_probe.stderr
    simulation = subprocess.run(
        ["vvp", str(tmp_path / "unknown.vvp")],
        text=True,
        capture_output=True,
        timeout=60,
    )
    (tmp_path / "unknown-rtl.log").write_text(simulation.stdout + simulation.stderr)
    assert simulation.returncode == 0
    assert "PYCIRCUIT_PHYSICAL_FAULT" in simulation.stdout
    assert "CLOCKED_UNKNOWN_ZERO_EFFECT_PASS" in simulation.stdout

    # Same normal capture path, but a partial projection is not a full witness.
    source = project / "model.py"
    original_source = source.read_text()
    source.write_text(
        source.read_text()
        .replace("tuple[Output, State]", "tuple[Output, ac.u32]")
        .replace("return advance, state", "return advance, state.total")
    )
    unit = tmp_path / "unsafe-unit"
    capture = subprocess.run(
        [
            cli,
            "compile",
            "-c",
            str(source),
            "--source-root",
            str(project),
            "--package-prefix",
            "clocked_accumulator",
            "-I",
            str(build / "units/provider"),
            "-o",
            str(unit),
        ],
        text=True,
        capture_output=True,
        timeout=60,
    )
    assert capture.returncode == 0, capture.stderr
    rejected = subprocess.run(
        [
            cli,
            "link",
            str(build / "units/provider"),
            str(unit),
            "--top",
            "clocked_accumulator.model.Accumulator",
            "--execution",
            "functional",
            "-o",
            str(tmp_path / "unsafe.ac"),
        ],
        text=True,
        capture_output=True,
        timeout=60,
    )
    assert rejected.returncode != 0 and not (tmp_path / "unsafe.ac").exists()

    # A stateful descendant cannot hide behind the root's complete local witness.
    source.write_text(
        original_source.replace(
            "def Increment(value: ac.u32) -> ac.u32:\n",
            "def Increment(value: ac.u32) -> ac.u32:\n    count: ac.u32 = 0\n",
        ).replace(
            "        return increment(value)",
            "        count = count + 1\n        return increment(value)",
        )
    )
    hidden = tmp_path / "hidden-unit"
    capture = subprocess.run(
        [
            cli,
            "compile",
            "-c",
            str(source),
            "--source-root",
            str(project),
            "--package-prefix",
            "clocked_accumulator",
            "-I",
            str(build / "units/provider"),
            "-o",
            str(hidden),
        ],
        text=True,
        capture_output=True,
        timeout=60,
    )
    assert capture.returncode == 0, capture.stderr
    rejected = subprocess.run(
        [
            cli,
            "link",
            str(build / "units/provider"),
            str(hidden),
            "--top",
            "clocked_accumulator.model.Accumulator",
            "--execution",
            "functional",
            "-o",
            str(tmp_path / "hidden.ac"),
        ],
        text=True,
        capture_output=True,
        timeout=60,
    )
    assert rejected.returncode != 0 and not (tmp_path / "hidden.ac").exists()

    failed_run = subprocess.run(
        [
            cli,
            "run",
            str(project),
            "--model",
            "clocked_accumulator.model.Accumulator",
            "--execution",
            "timing",
            "--target",
            "cpp",
            "--memory",
            str(project / "simulation.json"),
            "--build-dir",
            str(build),
        ],
        text=True,
        capture_output=True,
        timeout=180,
    )
    assert failed_run.returncode != 0
    incomplete = json.loads((build / "clocked-run/execution.json").read_text())
    assert incomplete["status"] == "incomplete"
    assert incomplete["events"] is None and incomplete["stores"] is None

    # A visible architectural timer belongs to the full witness and cannot be
    # silently folded, even when every memory/retirement event is otherwise idle.
    source.write_text(
        original_source.replace(
            "    total: ac.u32\n", "    total: ac.u32\n    age: ac.u32\n", 1
        ).replace(
            "    def advance() -> Output:\n",
            "    def advance() -> Output:\n        state.age = state.age + 1\n",
        )
    )
    timer_build = tmp_path / "timer-functional"
    timer = subprocess.run(
        [
            cli,
            "run",
            str(project),
            "--model",
            "clocked_accumulator.model.Accumulator",
            "--execution",
            "functional",
            "--target",
            "cpp",
            "--memory",
            str(project / "simulation.json"),
            "--build-dir",
            str(timer_build),
        ],
        text=True,
        capture_output=True,
        timeout=180,
    )
    assert timer.returncode == 0, timer.stdout + timer.stderr
    timing = json.loads((timer_build / "clocked-run/execution.json").read_text())
    assert timing["model_cycles"] == timing["executed_ticks"] == 28
    assert timing["skipped_idle_cycles"] == 0
    timer_ports = json.loads((timer_build / "cpp/ports.json").read_text())
    age = next(
        field
        for field in timer_ports["outputs"][1]["fields"]
        if field["path"] == ["age"]
    )
    state = int((timer_build / "clocked-run/state.bits").read_text(), 2)
    assert state >> age["low"] & ((1 << age["width"]) - 1) == 28

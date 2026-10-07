"""Independent auxiliary encoder and short genuine-Icarus clock verification."""

import argparse
import hashlib
import json
import shutil
import subprocess
import tempfile
import time
from pathlib import Path

parser = argparse.ArgumentParser()
for name in ("driver", "unit", "source", "build", "include", "runtime", "cxx", "verilator"):
    parser.add_argument("--" + name, required=True)
parser.add_argument("--main-build")
for name in ("iverilog", "vvp"):
    parser.add_argument("--" + name, default=shutil.which(name))
args = parser.parse_args()
if not args.iverilog or not args.vvp:
    parser.error("complete clock/encoder four-state evidence requires genuine iverilog and vvp")
source, unit, build = (Path(getattr(args, name)).resolve() for name in ("source", "unit", "build"))
include, runtime = Path(args.include).resolve(), Path(args.runtime).resolve()
main_build = Path(args.main_build).resolve() if args.main_build else build.parent
build.mkdir(parents=True, exist_ok=True)
assert unit.is_dir() and runtime.is_file()
commands = []


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory(path):
    return {p.relative_to(path).as_posix(): digest(p) for p in sorted(path.rglob("*")) if p.is_file()}


def run(command, label):
    command = list(map(str, command))
    start = time.monotonic()
    result = subprocess.run(command, cwd=build, capture_output=True, text=True, timeout=240)
    (build / (label + ".stdout")).write_text(result.stdout)
    (build / (label + ".stderr")).write_text(result.stderr)
    commands.append({"command": command, "exit_status": result.returncode,
                     "elapsed_seconds": time.monotonic() - start, "timeout_seconds": 240})
    (build / "execution.json").write_text(json.dumps(commands, indent=2) + "\n")
    assert result.returncode == 0, (label, result.returncode, result.stdout, result.stderr)
    return [line for line in result.stdout.splitlines() if line.startswith(("WORK ", "MASK ", "FOUR "))]


proof = build / "verification.json"
proof.unlink(missing_ok=True)
unit_before = inventory(unit)
test_inputs = {name: digest(source / name) for name in
               ("driver.cpp", "rtl_tb.sv", "config.json", "encoder_driver.cpp", "encoder_tb.sv", "verify_encoder.py")}
final = build / "digital_clock.ac"
run([args.driver, "link", unit, "--top", "example_digital_clock.digital_clock.EncodeBcd",
     "-o", final, "--replace"], "link")
for target in ("cpp", "verilog"):
    run([args.driver, "emit", final, "--target", target, "-o", build / target, "--replace"], "emit-" + target)
receipt = json.loads((build / "cpp/generated.json").read_text())
cpp = [build / "cpp" / row["path"] for row in receipt["files"] if row["path"].endswith(".cpp")]
assert cpp
runner = build / "encoder_runner"
run([args.cxx, "-O3", "-DNDEBUG", "-std=c++20", "-pthread", "-I" + str(include),
     "-I" + str(build / "cpp"), source / "encoder_driver.cpp", *cpp, runtime, "-o", runner], "native-build")
serial = run([runner, "1"], "serial")
parallel = run([runner, "2"], "parallel")
assert len(serial) == 4096 and serial == parallel
known = [line for line in serial if line.startswith("WORK ")]
assert len(known) == 64 and sum(line.startswith("MASK ") for line in serial) == 4032
receipt = json.loads((build / "verilog/generated.json").read_text())
rtl = [build / "verilog" / row["path"] for row in receipt["files"] if row["role"] == "rtl"]
rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
primitives = sorted((include / "verilog").glob("*.v"))
with tempfile.TemporaryDirectory(prefix="pycircuit-clock-encoder-rtl-") as temporary:
    rtl_build = Path(temporary).resolve()
    run([args.verilator, "--binary", "--timing", "--top-module", "tb", "--prefix", "Vencoder",
         "--Mdir", rtl_build, "-j", "2", "-Wno-fatal", *primitives, *rtl, source / "encoder_tb.sv"], "rtl-build")
    assert run([rtl_build / "Vencoder"], "rtl-run") == known
icarus = build / "encoder-four-state.vvp"
run([args.iverilog, "-g2012", "-DPYC_ENCODER_FOUR_STATE", "-s", "tb", "-o", icarus,
     *primitives, *rtl, source / "encoder_tb.sv"], "icarus-build")
assert run([args.vvp, icarus], "icarus-run") == serial
main_receipt = main_build / "verilog/generated.json"
assert main_receipt.is_file(), "main DigitalClock generated RTL required for short Icarus gate"
main_rtl = [main_build / "verilog" / row["path"] for row in json.loads(main_receipt.read_text())["files"] if row["role"] == "rtl"]
main_rtl.sort(key=lambda path: (path.name != "design_top.sv", str(path)))
main_before = {str(path): digest(path) for path in [main_receipt, *main_rtl]}
main_icarus = build / "clock-four-state.vvp"
run([args.iverilog, "-g2012", "-DPYC_DIGITAL_CLOCK_FOUR_STATE", "-s", "tb", "-o", main_icarus,
     *primitives, *main_rtl, source / "rtl_tb.sv"], "clock-icarus-build")
clock_trace = run([args.vvp, main_icarus], "clock-icarus-run")
assert sum(line.startswith("FOUR ") for line in clock_trace) == 10
assert inventory(unit) == unit_before, "auxiliary-root link/emit changed the published source unit"
assert test_inputs == {name: digest(source / name) for name in test_inputs}, "test inputs changed during verification"
assert main_before == {name: digest(Path(name)) for name in main_before}, "main RTL changed during short Icarus verification"
proof.write_text(json.dumps({
    "source_unit": str(unit), "source_unit_inputs": unit_before, "test_inputs": test_inputs,
    "final_sha256": digest(final), "runner_sha256": digest(runner), "runtime_sha256": digest(runtime),
    "generated": {target: inventory(build / target) for target in ("cpp", "verilog")},
    "execution_sha256": digest(build / "execution.json"), "patterns": 4096, "known": 64,
    "unknown": 4032, "known_recovery": 4032, "workers": [1, 2],
    "rtl_known": "verilator", "rtl_four_state": "icarus", "clock_short_four_state_cases": 10,
    "main_rtl_inputs": main_before,
    "trace_sha256": hashlib.sha256(("\n".join(serial) + "\n").encode()).hexdigest(),
}, indent=2) + "\n")
print("4096 encoder patterns agree: 64 known, 4032 X/Z and recoveries; workers1/2, Verilator, Icarus; short clock Icarus10 cases")  # noqa: T201 - CLI gate status

"""Independent owning-RTL register/FIFO gates; T2 memory cases are unfinished."""

import argparse
import hashlib
import importlib.util
import json
import subprocess
from pathlib import Path


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("repo", "scratch", "iverilog", "vvp", "verilator"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument(
        "--case",
        required=True,
        choices=("registers", "fifo", "byte_mem", "sync_mem", "all"),
    )
    parser.add_argument("--cxx")
    args = parser.parse_args()
    if args.case == "fifo":
        if not args.cxx:
            parser.error("FIFO native-kernel oracle requires --cxx")
        path = Path(__file__).with_name("managed-fifo.py")
        spec = importlib.util.spec_from_file_location("managed_fifo_oracle", path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        module.run_fifo(args)
        return
    if args.case != "registers":
        parser.error(
            "T2 implements registers/fifo; requested memory/all selector is unfinished"
        )
    repo = Path(args.repo).resolve()
    scratch = Path(args.scratch).resolve()
    scratch.mkdir(parents=True, exist_ok=True)
    bench = Path(__file__).with_suffix(".sv").resolve()
    leaves = [repo / "include/verilog" / name for name in ("dff.v", "dffe.v")]
    inputs = [bench, *leaves]
    before = {str(path): digest(path) for path in inputs}
    commands = []
    cases = []

    def run(command, label, rejection=False, marker=None):
        result = subprocess.run(
            list(map(str, command)),
            cwd=repo,
            capture_output=True,
            text=True,
            timeout=180,
        )
        row = {
            "label": label,
            "command": list(map(str, command)),
            "exit_status": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
        commands.append(row)
        (scratch / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        assert (result.returncode != 0) if rejection else (result.returncode == 0), row
        if marker:
            assert marker in result.stdout + result.stderr, row
        return result

    run([args.iverilog, "-V"], "iverilog-version")
    run([args.vvp, "-V"], "vvp-version")
    run([args.verilator, "--version"], "verilator-version")

    # Four-state cases execute the exact owning files, without rewriting them.
    for width, structured in (
        (1, False),
        (13, False),
        (65, False),
        (130, False),
        (13, True),
        (65, True),
    ):
        name = f"registers-{width}-{'struct' if structured else 'bits'}"
        executable = scratch / (name + ".vvp")
        defines = ["-DSTRUCT_PAYLOAD"] if structured else []
        run(
            [
                args.iverilog,
                "-g2012",
                "-s",
                "tb",
                "-P",
                f"tb.WIDTH={width}",
                *defines,
                "-o",
                executable,
                bench,
                *leaves,
            ],
            name + "-iverilog-build",
        )
        result = run(
            [args.vvp, executable],
            name + "-iverilog-run",
            marker=f"PASS registers width={width}",
        )
        assert "four_state=8" in result.stdout, result.stdout
        cases.append({"case": name, "engine": "Icarus", "four_state": True})
        if width == 13 and not structured:
            for argument, diagnostic in (
                ("BAD_AUTONOMOUS_RESET", "reset must be known"),
                ("BAD_AUTONOMOUS_ENABLE", "enable must be known"),
            ):
                run(
                    [args.vvp, executable, "+" + argument],
                    name + "-" + argument,
                    rejection=True,
                    marker=diagnostic,
                )
        directory = scratch / (name + "-verilator")
        run(
            [
                args.verilator,
                "--binary",
                "--timing",
                "-Wno-fatal",
                "--top-module",
                "tb",
                "--Mdir",
                directory,
                f"-GWIDTH={width}",
                *defines,
                bench,
                *leaves,
            ],
            name + "-verilator-build",
        )
        result = run(
            [directory / "Vtb"],
            name + "-verilator-run",
            marker=f"PASS registers width={width}",
        )
        assert "four_state=0" in result.stdout, result.stdout
        cases.append({"case": name, "engine": "Verilator", "four_state": False})

    # Type admission relies on Icarus preserving X through the actual typedef.
    executable = scratch / "two-state-payload.vvp"
    run(
        [
            args.iverilog,
            "-g2012",
            "-s",
            "tb",
            "-DTWO_STATE_PAYLOAD",
            "-o",
            executable,
            bench,
            *leaves,
        ],
        "two-state-payload-build",
    )
    run(
        [args.vvp, executable],
        "two-state-payload-rejected",
        rejection=True,
        marker="four-state",
    )

    for width in (13, 65):
        name = f"synthesis-{width}"
        executable = scratch / (name + ".vvp")
        run(
            [
                args.iverilog,
                "-g2012",
                "-DSYNTHESIS",
                "-s",
                "tb_synthesis",
                "-P",
                f"tb_synthesis.WIDTH={width}",
                "-o",
                executable,
                bench,
                *leaves,
            ],
            name + "-iverilog-build",
        )
        run(
            [args.vvp, executable],
            name + "-iverilog-run",
            marker=f"PASS synthesis registers width={width}",
        )
        directory = scratch / (name + "-verilator")
        run(
            [
                args.verilator,
                "--binary",
                "--timing",
                "-Wno-fatal",
                "-DSYNTHESIS",
                "--top-module",
                "tb_synthesis",
                "--Mdir",
                directory,
                f"-GWIDTH={width}",
                bench,
                *leaves,
            ],
            name + "-verilator-build",
        )
        run(
            [directory / "Vtb_synthesis"],
            name + "-verilator-run",
            marker=f"PASS synthesis registers width={width}",
        )
        cases.append(
            {
                "case": name,
                "engines": ["Icarus", "Verilator"],
                "synthesis_projection": True,
            }
        )

    assert {
        str(path): digest(path) for path in inputs
    } == before, "RTL/test input drift"
    (scratch / "results.json").write_text(
        json.dumps(
            {
                "scope": "T2 registers only; no FIFO/memory/whole-DUT acceptance",
                "input_sha256": before,
                "cases": cases,
                "two_state_payload_rejected": True,
                "autonomous_invalid_controls_rejected": ["reset", "enable"],
                "remaining_cases": ["fifo", "byte_mem", "sync_mem", "all"],
                "historical_roots_closed": 0,
            },
            indent=2,
        )
        + "\n"
    )
    print(
        "PASS T2 registers: owning RTL managed/autonomous/four-state/synthesis; FIFO/memory not validated in this run"
    )  # noqa: T201 - standalone gate receipt


if __name__ == "__main__":
    main()

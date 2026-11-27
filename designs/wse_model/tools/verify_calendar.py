"""Run the current pyCircuit Calendar consumer and retain execution evidence.

Use a fresh output directory. This runner never skips a missing tool or treats
source parsing as backend execution. The source System owns literal assertions.
"""

from __future__ import annotations

import argparse
import ast
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from models.calendar.host_checks import (  # noqa: E402
    check_allgather,
    check_allgather_control,
    check_multinode,
    check_node,
)

PROJECT = ROOT / "models" / "calendar"
SUITES = {
    "capacity-positive": (
        3,
        "capacity_checked",
        "isolated capacity check; space available for three cycles",
    ),
    "capacity-exhausted": (
        3,
        "capacity_checked",
        "isolated capacity check; third cycle must fail before publication",
    ),
    "forward": (9, "forward_checked", "40-node 5x8 mesh; nine literal single-hop scenarios"),
    "route": (4, "route_checked", "40-node entry decode; nodes 0,31,32,39; four literal rows"),
    "receive": (12, "receive_checked", "8-bit epoch; one context; 2 early and 2 seen slots"),
    "multinode": (
        12,
        "multinode_result",
        "40-node profile; connected five-node tree; 2 epochs; 2 early/seen slots per node; one segment at a time",
    ),
    "allgather": (
        516,
        "allgather_result",
        "40-node profile; 32 resident receivers; row/column AllGather; 8-bit epochs; "
        "2 early/56 seen slots; serialized one-segment functional schedule",
    ),
    "allgather-control": (
        523,
        "allgather_control_result",
        "40-node carrier; 32 configured receivers with private 8-bit epoch banks; "
        "completion-driven row/column phases; 2 early/56 seen slots; serialized multicast",
    ),
    "allgather-delayed": (
        529,
        "allgather_control_result",
        "same closed loop; only node 31 post and node 30 landing permits delayed "
        "by three cycles each; identical routes, payload geometry and capacities",
    ),
    "node": (
        13,
        "node_result",
        "40-node profile; source 1 to node 9; installation/epoch/decode/forward/receive "
        "integration; 2 early/seen slots; two 8-byte rounds",
    ),
    "control": (
        261,
        "control_checked",
        "8-bit epochs; opcode isolation; CalReg installation admission",
    ),
}


def core_expectations(suite: str) -> list[list[int]] | None:
    """Test-owned oracle; never called by DUT capture, lowering or execution."""
    if suite in ("allgather-control", "allgather-delayed"):
        from allgather_control_reference import expectations

        return expectations(delayed=suite == "allgather-delayed")[0]
    if suite == "node":
        from node_reference import expectations

        return expectations()
    if suite == "allgather":
        from allgather_reference import expectations

        return expectations()
    if suite == "multinode":
        from multinode_reference import expectations

        return expectations()
    if suite == "control":
        from wse_model.calendar.epoch import EpochTracker
        from wse_model.errors import WseModelError
        from wse_model.noc.calreg import CalRegBank, CalRegSlot, StopTheWorldWindow

        epochs, bank = EpochTracker(width_bits=8), CalRegBank(0)
        window = StopTheWorldWindow(0)
        history = []
        for phase in range(261):
            epoch_fault = False
            opcode = 2 if phase in (255, 259) else 0 if phase == 260 else 1
            if phase == 260:
                try:
                    epochs.next_epoch(opcode)
                except WseModelError:
                    value = 0  # The absent/reserved table slot retains reset zero.
                    epoch_fault = True
                else:
                    raise AssertionError("reserved opcode unexpectedly accepted by core")
            else:
                if phase != 257:
                    try:
                        epochs.next_epoch(opcode, drained=phase == 258)
                    except WseModelError:
                        epoch_fault = True
                        if phase != 256:
                            raise
                value = epochs.counter(opcode).current
            config_fault = False
            slot = 7 if phase == 7 else 2 if phase in (8, 9) else 1
            if phase == 4:
                window.enter()
            elif phase == 5:
                window.drain()
            try:
                if phase in (0, 2, 6, 9):
                    bank.lookup(slot)
                elif phase in (1, 3, 7, 8):
                    window.require_drained(action="install")
                    bank.install(CalRegSlot(opcode=slot, content=b"test"))
                elif phase in (4, 5):
                    window.require_drained(action="clear")
                    bank.clear()
            except WseModelError:
                config_fault = True
            history.append(
                [
                    phase,
                    value,
                    sum(1 << code for code in bank.installed),
                    int(epoch_fault),
                    int(config_fault),
                    int(phase < 10 and not config_fault),
                ]
            )
        return history
    if suite == "forward":
        from wse_model.calendar.route_bits import RouteBits
        from wse_model.noc.forward import simulate_multicast
        from wse_model.topology import CALENDAR_BASELINE

        # Read only the authored literal stimulus table, without executing DSL functions.
        tree = ast.parse((PROJECT / "bench_forward.py").read_text())
        vectors = next(
            n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "vectors"
        )
        rows = next(n.value for n in vectors.body if isinstance(n, ast.AnnAssign))
        history = []
        for phase, literal in enumerate(rows.elts):
            row = {kw.arg: ast.literal_eval(kw.value) for kw in literal.keywords}
            route = RouteBits.from_int(row["route"], 40)
            fault = bool(route.illegal_nodes())
            egress, lands = 0, False
            if not fault:
                hop = simulate_multicast(
                    CALENDAR_BASELINE.topology, route, source=row["node"]
                ).hops[0]
                egress = sum(
                    1 << peer
                    for peer in hop.egress
                    if not row.get("has_ingress", False) or peer != row["ingress"]
                )
                lands = hop.lands
            assert (egress, lands, fault) == (
                row.get("egress", 0),
                row.get("lands", False),
                row.get("fault", False),
            ), f"literal hop vector {phase} disagrees with the Python core"
            history.append([phase, egress])
        return history
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--toolchain", type=Path, required=True)
    parser.add_argument("--pycircuit-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--target", choices=("cpp", "verilog", "both"), default="both")
    parser.add_argument("--suite", choices=tuple(SUITES), default="route")
    parser.add_argument(
        "--timeout", type=int, default=1800, help="Seconds per compiler/build/run stage"
    )
    parser.add_argument(
        "--cpp-opt-level",
        type=int,
        choices=(0, 1, 2, 3),
        default=0,
        help="Host C++ optimization level; functional checks default to 0",
    )
    parser.add_argument(
        "--stack-kib", type=int, help="Required inherited POSIX host stack allowance"
    )
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    (output / "runner.py").write_bytes(Path(__file__).read_bytes())
    toolchain = args.toolchain.resolve()
    cycles, event_name, scope = SUITES[args.suite]
    receipt = {
        "status": "INCOMPLETE",
        "suite": args.suite,
        "scope": scope,
        "cycles": cycles,
        "stage_timeout_seconds": args.timeout,
        "cpp_optimization": args.cpp_opt_level,
        "host_stack_kib": args.stack_kib,
        "python": sys.version,
        "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "multinode_reference_sha256": hashlib.sha256(
            (ROOT / "tools/multinode_reference.py").read_bytes()
        ).hexdigest(),
        "allgather_reference_sha256": hashlib.sha256(
            (ROOT / "tools/allgather_reference.py").read_bytes()
        ).hexdigest(),
        "allgather_control_reference_sha256": hashlib.sha256(
            (ROOT / "tools/allgather_control_reference.py").read_bytes()
        ).hexdigest(),
        "node_reference_sha256": hashlib.sha256(
            (ROOT / "tools/node_reference.py").read_bytes()
        ).hexdigest(),
        "python_core_sha256": {
            str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted((ROOT / "src/wse_model").rglob("*.py"))
        },
        "source_sha256": {
            str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(PROJECT.rglob("*"))
            if path.is_file()
            and ".pycircuit_out" not in path.parts
            and "__pycache__" not in path.parts
        },
        "runs": [],
    }
    receipt_path = output / "verification.json"

    def save() -> None:
        receipt_path.write_text(json.dumps(receipt, indent=2) + "\n")

    save()
    try:
        if args.stack_kib is not None:
            import resource

            requested = args.stack_kib * 1024
            soft, hard = resource.getrlimit(resource.RLIMIT_STACK)
            receipt["host_stack_actual_bytes"] = soft
            receipt["host_stack_hard_bytes"] = hard
            if requested <= 0 or (hard != resource.RLIM_INFINITY and requested > hard):
                raise RuntimeError("requested host stack is outside the operating-system limit")
            if soft != resource.RLIM_INFINITY and soft < requested:
                raise RuntimeError(
                    f"start this runner from a shell with 'ulimit -s {args.stack_kib}'; "
                    "the inherited host stack allowance is too small"
                )
        revision = subprocess.check_output(
            ["git", "-C", str(args.pycircuit_root), "rev-parse", "HEAD"], text=True
        ).strip()
        receipt["pycircuit_revision"] = revision
        pin = json.loads((PROJECT / "pycircuit.lock.json").read_text())
        if revision != pin["revision"]:
            raise RuntimeError("pyCircuit checkout does not match pycircuit.lock.json")
        dirty = subprocess.check_output(
            ["git", "-C", str(args.pycircuit_root), "status", "--porcelain", "-uno"], text=True
        ).strip()
        if dirty:
            raise RuntimeError(
                "pyCircuit has tracked modifications; pin/build provenance is unclear"
            )
        metadata = json.loads((toolchain / "share/pycircuit/toolchain-metadata.json").read_text())
        receipt["toolchain"] = metadata
        if metadata["git_sha"] != revision or metadata["llvm_version"] != pin["llvm_mlir"]:
            raise RuntimeError("installed toolchain does not match the pinned source and LLVM")
        expected = core_expectations(args.suite)
        targets = ("cpp", "verilog") if args.target == "both" else (args.target,)
        for target in targets:
            command = [
                sys.executable,
                "-m",
                "pycircuit.cli",
                "run",
                str(PROJECT),
                "--target",
                target,
                "--toolchain",
                str(toolchain),
                "--build-dir",
                str(output / target),
                "--cycles",
                str(cycles),
                "--timeout",
                str(args.timeout),
            ]
            run = {"target": target, "command": command, "status": "INCOMPLETE"}
            receipt["runs"].append(run)
            save()
            env = dict(os.environ)
            env["WSE_CALENDAR_SUITE"] = args.suite
            if sys.prefix != sys.base_prefix:
                env["VIRTUAL_ENV"] = sys.prefix
            env["PATH"] = str(Path(sys.executable).parent) + os.pathsep + env.get("PATH", "")
            with (output / f"{target}.log").open("w") as log:
                if target == "cpp":
                    # Configure the generated simulator through its public CMake
                    # project. Only host optimization changes; final ACIR and
                    # generated DUT/System sources remain untouched.
                    package = toolchain / "share/pycircuit/cmake"
                    build = output / target
                    env["PYC_TOOLCHAIN_ROOT"] = str(toolchain)
                    for name in (
                        "PYCIRCUIT_SOURCE_COMPILER",
                        "PYCIRCUIT_LINKER",
                        "PYCIRCUIT_EMITTER",
                        "PYCIRCUIT_NATIVE_BUILD",
                    ):
                        env.pop(name, None)
                    preparation = [
                        [
                            "cmake",
                            "-S",
                            str(PROJECT),
                            "-B",
                            str(build),
                            "-G",
                            "Ninja",
                            "-DCMAKE_BUILD_TYPE=Release",
                            f"-Dpycircuit_DIR={package}",
                            "-DPYC_EXAMPLE_VERIFY=OFF",
                            "-DPYC_EXAMPLE_RUN_TARGET=cpp",
                        ],
                        [
                            "cmake",
                            "--build",
                            str(build),
                            "--target",
                            "pycircuit_simulation_artifacts",
                        ],
                        [
                            "cmake",
                            "-S",
                            str(build / "cpp"),
                            "-B",
                            str(build / "simulation/cpp"),
                            "-G",
                            "Ninja",
                            "-DCMAKE_BUILD_TYPE=Release",
                            f"-Dpycircuit_DIR={package}",
                            f"-DCMAKE_CXX_FLAGS_RELEASE=-O{args.cpp_opt_level} -DNDEBUG",
                        ],
                    ]
                    run["preparation"] = []
                    for prepare in preparation:
                        prepared = subprocess.run(
                            prepare,
                            stdout=log,
                            stderr=subprocess.STDOUT,
                            env=env,
                            timeout=args.timeout,
                        )
                        run["preparation"].append(
                            {"command": prepare, "exit_code": prepared.returncode}
                        )
                        save()
                        if prepared.returncode:
                            raise RuntimeError(f"C++ preparation failed; see {output / 'cpp.log'}")
                result = subprocess.run(
                    command,
                    stdout=log,
                    stderr=subprocess.STDOUT,
                    env=env,
                    timeout=8 * args.timeout,
                )
            run["exit_code"] = result.returncode
            final_artifacts = list((output / target).glob("*.ac"))
            if len(final_artifacts) == 1:
                run["final_artifact_sha256"] = hashlib.sha256(
                    final_artifacts[0].read_bytes()
                ).hexdigest()
            if args.suite == "capacity-exhausted":
                log_text = (output / f"{target}.log").read_text()
                records = [
                    json.loads(line) for line in log_text.splitlines() if line.startswith("{")
                ]
                observations = [
                    event
                    for event in records
                    if event.get("kind") == "log"
                    and event.get("spec", {}).get("event") == event_name
                ]
                values = [[int(v["value"]) for v in e["values"]] for e in observations]
                if result.returncode == 0 or values != [[0, 1], [0, 1], [1, 2], [1, 2]]:
                    raise RuntimeError(f"{target}: expected capacity rejection after two cycles")
                executable = output / target / "simulation" / target / "bin/pycircuit_sim"
                if not executable.is_file():
                    raise RuntimeError(
                        f"{target}: no simulator executable; build failure is not rejection evidence"
                    )
                if target == "cpp":
                    terminal = [r for r in records if r.get("kind") == "result"]
                    if (
                        len(terminal) != 1
                        or terminal[0].get("status") != "FAILED"
                        or terminal[0].get("epoch_time") != "4"
                        or terminal[0].get("error", {}).get("message")
                        != "WSE_MODEL_CAPACITY: dedup identity slots exhausted"
                    ):
                        raise RuntimeError(
                            "C++ failed for a reason other than the capacity assertion"
                        )
                    run["terminal_result"] = terminal[0]
                else:
                    if "source or primitive check failed at epoch 4" not in log_text:
                        raise RuntimeError("Verilog did not report the expected check boundary")
                    run["diagnostic_limit"] = (
                        "RTL top reports a generic check failure, not assertion text"
                    )
                run["checker_observations"] = observations
                run["status"] = "PASS_EXPECTED_FAILURE"
                save()
                continue
            if result.returncode:
                raise RuntimeError(f"{target} failed; see {output / (target + '.log')}")
            execution = output / target / "run-execution.json"
            if not execution.is_file():
                raise RuntimeError(f"{target} produced no execution receipt")
            run["execution"] = json.loads(execution.read_text())
            if run["execution"].get("status") != "success":
                raise RuntimeError(f"{target}: execution did not succeed")
            if run["execution"].get("sampling_epochs") != 2 * cycles:
                raise RuntimeError(f"{target}: incomplete simulation sample count")
            # Every low/high sample executes the assertion-bearing checker.
            lines = (output / f"{target}.log").read_text().splitlines()
            events = []
            state_events = []
            for line in lines:
                if line.startswith("{"):
                    event = json.loads(line)
                    if (
                        event.get("kind") == "log"
                        and event.get("spec", {}).get("event") == event_name
                    ):
                        events.append(event)
                    elif (
                        event.get("kind") == "log"
                        and event.get("spec", {}).get("event") == "allgather_control_state"
                    ):
                        state_events.append(event)
            phases = [int(event["values"][0]["value"]) for event in events]
            multiplicity = (
                64 if args.suite.startswith("allgather") else 10 if args.suite == "multinode" else 2
            )
            if phases != [phase for phase in range(cycles) for _ in range(multiplicity)]:
                raise RuntimeError(
                    f"{target}: checker phases are incomplete or reordered: {phases}"
                )
            run["checker_observations"] = events
            if expected is not None:
                actual = [
                    [
                        (
                            1
                            if item["value"] == "true"
                            else 0 if item["value"] == "false" else int(item["value"])
                        )
                        for item in event["values"]
                    ]
                    for event in events
                ]
                wanted = [row for row in expected for _ in range(2)]
                if args.suite == "multinode":
                    check_multinode(events, actual, wanted, cycles, target)
                elif args.suite == "allgather":
                    check_allgather(events, actual, wanted, cycles, target)
                elif args.suite in ("allgather-control", "allgather-delayed"):
                    from allgather_control_reference import expectations

                    expected_states = expectations(delayed=args.suite == "allgather-delayed")[1]
                    check_allgather_control(
                        events, actual, wanted, state_events, expected_states, cycles, target
                    )
                    run["control_observations"] = state_events
                elif args.suite == "node":
                    check_node(events, actual, wanted, cycles, target)
                if actual != wanted:
                    raise RuntimeError(f"{target}: observations differ from the Python core")
                run["python_core_observations"] = "PASS"
            run["status"] = "PASS"
            save()
        final_hashes = [run.get("final_artifact_sha256") for run in receipt["runs"]]
        if None in final_hashes or len(set(final_hashes)) != 1:
            raise RuntimeError("backend final artifacts are missing or differ")
        for relative, digest in receipt["source_sha256"].items():
            if hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() != digest:
                raise RuntimeError(f"source changed during verification: {relative}")
        receipt["status"] = "PASS"
        save()
        return 0
    except Exception as error:
        receipt["error"] = str(error)
        save()
        raise


if __name__ == "__main__":
    raise SystemExit(main())

"""Run real payloads on the generic discrete-event pyCircuit WSE machine."""

from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path

from machine_common import digest, execute_program, verify_build
from run_pto_allgather import compile_workload, inputs
from timed_machine_cases import (
    branching_multicast,
    check_default_open_events,
    disjoint_links,
    one_hop,
    open_calendar,
    shared_link,
    two_hop_pipeline,
)
from verify_wse_machine import check_case

from wse_model.programs.timing import encode_config, load_timing_profile, timed_program

ROOT = Path(__file__).resolve().parents[1]


def read_events(result):
    events = [json.loads(line) for line in Path(result["events"]).read_text().splitlines()]
    if not events:
        raise AssertionError("timed execution must publish actual events")
    if [e["event"] for e in events] != list(range(1, len(events) + 1)):
        raise AssertionError("event trace has a gap")
    if any(b["noc_cycle"] < a["noc_cycle"] for a, b in zip(events, events[1:], strict=False)):
        raise AssertionError("model time went backwards")
    return events


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--simulator", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--suite", choices=("smoke", "allgather"), default="smoke")
    parser.add_argument("--profile", type=Path, default=ROOT / "models/machine/timing_profile.json")
    parser.add_argument("--pypto-python", type=Path)
    parser.add_argument("--ptoas-python", type=Path)
    parser.add_argument("--ptoas-root", type=Path)
    parser.add_argument("--seed", type=int, default=1)
    args = parser.parse_args()
    output, simulator = args.output.resolve(), args.simulator.resolve()
    output.mkdir(parents=True, exist_ok=False)
    profile = load_timing_profile(args.profile)
    build = verify_build(simulator)
    if build["receipt"].get("model") != "timed":
        raise ValueError("requires a timed WSE machine build")
    receipt = {
        "status": "INCOMPLETE",
        "hardware_calibrated": False,
        "build": build,
        "profile": profile,
        "profile_sha256": digest(args.profile),
        "suite": args.suite,
        "runs": [],
    }

    def save():
        (output / "verification.json").write_text(json.dumps(receipt, indent=2) + "\n")

    def run_case(case, label, parameters, calendar, *, stall=None, oracle=False):
        result = execute_program(
            simulator,
            case.program,
            case.patches,
            case.readback,
            output / label,
            timing_config=encode_config(parameters, calendar=calendar),
            stall=stall,
        )
        check_case(case, result)
        events = read_events(result)
        if oracle:
            check_default_open_events(case, events)
        record = {"name": label, "status": "PASS", "profile": parameters, **result}
        receipt["runs"].append(record)
        save()
        print(
            f"{label}: PASS, {result['simulation']['model_time_noc_cycles']} NoC cycles", flush=True
        )
        return result

    save()
    try:
        if args.suite == "smoke":
            baseline = None
            for case in (
                one_hop(),
                disjoint_links(),
                branching_multicast(),
                shared_link(),
                two_hop_pipeline(),
            ):
                result = run_case(case, case.name, profile, open_calendar(), oracle=True)
                if baseline is None:
                    baseline = result
            varied = copy.deepcopy(profile)
            varied["parameters"]["hop_noc_cycles"]["value"] = 3
            result = run_case(one_hop(), "hop-three-cycles", varied, open_calendar())
            if (
                result["simulation"]["model_time_noc_cycles"]
                != baseline["simulation"]["model_time_noc_cycles"] + 2
            ):
                raise AssertionError(
                    "changing only one-hop latency by two must delay final completion by two"
                )
            calendar = open_calendar()
            link_base = (38 * 4 + 2) * 8
            calendar.update({link_base + slot: (1 << 38 if slot == 1 else 0) for slot in range(8)})
            gated = run_case(one_hop(), "calendar-slot-one", profile, calendar)
            if gated["simulation"]["model_time_noc_cycles"] != 1253:
                raise AssertionError("slot-one hand-derived completion must be 1253")
            stalled = run_case(
                one_hop(),
                "landing-permit-stall",
                profile,
                open_calendar(),
                stall={"node": 39, "at": 430, "length": 3},
            )
            if stalled["simulation"]["model_time_noc_cycles"] != 434:
                raise AssertionError(
                    "three cycles of landing denial must delay completion by three"
                )
            if (
                stalled["inputs"] != baseline["inputs"]
                or stalled["output_sha256"] != baseline["output_sha256"]
            ):
                raise AssertionError("permit-only test changed input or output bytes")
        else:
            if any(
                value is None for value in (args.pypto_python, args.ptoas_python, args.ptoas_root)
            ):
                raise ValueError("AllGather requires the three explicit PTO tool paths")
            compiler, manifest, original, stages = compile_workload(args, output / "workload", {})
            program = timed_program(original)
            patches, ranges, expected = inputs(manifest, args.seed)
            result = execute_program(
                simulator,
                program,
                patches,
                ranges,
                output / "allgather",
                timing_config=encode_config(profile),
            )
            if Path(result["output"]).read_bytes() != expected:
                raise AssertionError(
                    "timed AllGather bytes differ from original source concatenation"
                )
            original_path = output / "allgather/input.bin"
            original_path.write_bytes(b"".join(p.data for p in patches))
            lowering = json.loads((compiler / "lowering.json").read_text())
            traffic = lowering["static_traffic"]["phases"]
            for key, field in (
                ("source_read_bytes", "source_read_bytes"),
                ("destination_write_bytes", "destination_write_bytes"),
            ):
                if result["simulation"][key] != sum(p[field] for p in traffic):
                    raise AssertionError(
                        "timed actual traffic disagrees with program payload accounting"
                    )
            events = read_events(result)
            from wse_model.programs.abi import Kind

            barriers = [
                i for i, command in enumerate(program.commands) if command.kind == Kind.BARRIER
            ]
            if len(barriers) != 2:
                raise AssertionError("PTO two-phase workload must contain two explicit barriers")
            row_writes = [
                e["noc_cycle"] for e in events if e["kind"] == 5 and e["pc"] <= barriers[1]
            ]
            column_reads = [
                e["noc_cycle"] for e in events if e["kind"] == 1 and e["pc"] > barriers[1]
            ]
            if len(row_writes) != 256 * 8 or len(column_reads) != 256:
                raise AssertionError(
                    "executed per-packet read/write event counts differ from the workload"
                )
            if max(row_writes) >= min(column_reads):
                raise AssertionError("column read preceded completion of required row data")
            receipt["phase_dependency"] = {
                "last_row_write_cycle": max(row_writes),
                "first_column_read_complete_cycle": min(column_reads),
            }
            receipt["runs"].append(
                {
                    "name": "allgather-32",
                    "status": "PASS",
                    "seed": args.seed,
                    "checked_bytes": len(expected),
                    "original_input_sha256": digest(original_path),
                    "stages": stages,
                    **result,
                }
            )
            print(
                f"allgather-32: PASS, {result['simulation']['model_time_noc_cycles']} NoC cycles",
                flush=True,
            )
        verify_build(simulator)
        receipt["status"] = "PASS"
    except Exception as error:
        receipt["error"] = str(error)
        raise
    finally:
        save()


if __name__ == "__main__":
    main()

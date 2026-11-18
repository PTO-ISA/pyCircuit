"""Run generated pyCircuit timing against the verified PTOAS workload.

The independent Python model supplies comparison values, never DUT timestamps.
Each run supplies only service parameters and uses the same compiled simulator.
"""

import argparse
import copy
import json
from pathlib import Path

from analyze_pto_allgather_timing import load_workload
from machine_common import digest, run, verify_build

from wse_model.analysis.allgather_schedule import prepare_timing

ROOT = Path(__file__).resolve().parents[1]


def expected_stages(phase, profile):
    p = {key: entry["value"] for key, entry in profile["parameters"].items()}
    names = {"tx_cdc": "tx_crossing", "rx_cdc": "rx_crossing"}
    result = {names.get(k, k): v * phase["ranks"] for k, v in phase["stage_durations"].items()}
    result.update(
        alignment=p["alignment_noc_cycles"],
        arm=p["arm_noc_cycles"],
        slot_idle=phase["slot_idle_cycles"],
    )
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler-dir", type=Path, required=True)
    parser.add_argument(
        "--profile", type=Path, default=ROOT / "models/pto_allgather/timing_profile.json"
    )
    parser.add_argument("--simulator", type=Path, required=True)
    parser.add_argument("--functional-verification", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    receipt = {"status": "INCOMPLETE", "runs": [], "hardware_calibrated": False}
    try:
        simulator = args.simulator.resolve()
        receipt["build"] = verify_build(simulator)
        program, descriptors, binary = load_workload(args.compiler_dir)
        receipt["commands_sha256"] = digest(args.compiler_dir / "commands.bin")
        profile = json.loads(args.profile.read_text())
        receipt["profile_sha256"] = digest(args.profile)
        if args.functional_verification:
            previous = json.loads(args.functional_verification.read_text())
            root = args.functional_verification.resolve().parent
            if previous["status"] != "PASS":
                raise ValueError("functional verification did not pass")
            matches = [
                workload
                for workload in previous["workloads"]
                if binary == (root / workload["name"] / "compiler/commands.bin").read_bytes()
            ]
            if len(matches) != 1:
                raise ValueError("functional evidence does not identify the same workload")
            workload = matches[0]
            verify_build(Path(previous["build"]["receipt"]["runner"]))
            for case in workload["runs"]:
                directory = root / workload["name"] / case["case"]
                for name, expected in case["inputs"].items():
                    if digest(directory / name) != expected:
                        raise ValueError("functional input changed after verification")
                if digest(directory / "output.bin") != case["output_sha256"]:
                    raise ValueError("functional output changed after verification")
            receipt["functional_evidence"] = {
                "path": str(args.functional_verification.resolve()),
                "sha256": digest(args.functional_verification),
                "status": "REUSED_SAME_WORKLOAD_AND_GENERIC_MACHINE",
                "workload": workload["name"],
                "cases": [case["case"] for case in workload["runs"]],
            }
        cases = (
            ("baseline", {}, None),
            ("hop-2", {"hop_noc_cycles": 2}, "baseline"),
            ("slot-1200", {"slot_noc_cycles": 1200}, "baseline"),
            (
                "ub-latency-150",
                {
                    "slot_noc_cycles": 1200,
                    "ub_read_first_beat_ns": 150,
                    "ub_write_first_beat_ns": 150,
                },
                "slot-1200",
            ),
            (
                "ub-bandwidth-64",
                {
                    "slot_noc_cycles": 1200,
                    "ub_read_bytes_per_core_cycle": 64,
                    "ub_write_bytes_per_core_cycle": 64,
                },
                "slot-1200",
            ),
        )
        for name, changes, control in cases:
            case_profile = copy.deepcopy(profile)
            for key, value in changes.items():
                case_profile["parameters"][key].update(
                    value=value, source="controlled sensitivity experiment"
                )
            reference = prepare_timing(program, descriptors, case_profile)
            directory = output / name
            directory.mkdir()
            (directory / "profile.json").write_text(json.dumps(case_profile, indent=2) + "\n")
            (directory / "reference.json").write_text(json.dumps(reference, indent=2) + "\n")
            config = directory / "input.txt"
            config.write_text(" ".join(map(str, reference["abi_values"])) + "\n")
            stage = run(
                [str(simulator), str(config), str(directory / "simulation.json")],
                directory,
                "simulation",
            )
            observed = json.loads((directory / "simulation.json").read_text())
            actual = [observed["row_cycles"], observed["column_cycles"]]
            if actual != [phase["phase_cycles"] for phase in reference["phases"]]:
                raise AssertionError(f"{name}: generated phase timing disagrees with reference")
            if observed["total_cycles"] != reference["total_noc_cycles"]:
                raise AssertionError(f"{name}: generated total timing disagrees with reference")
            wanted = [expected_stages(phase, case_profile) for phase in reference["phases"]]
            if observed["stage_ticks"] != wanted:
                raise AssertionError(f"{name}: generated stage accounting disagrees with reference")
            receipt["runs"].append(
                {
                    "case": name,
                    "status": "PASS",
                    "control": control,
                    "changes": changes,
                    "total_noc_cycles": observed["total_cycles"],
                    "predicted_seconds": reference["predicted_seconds"],
                    "stage": stage,
                }
            )
        # One core contract boundary: the generated model must reject a packet
        # that cannot fit the declared fixed Calendar window.
        bad = output / "slot-overflow"
        bad.mkdir()
        abi = prepare_timing(program, descriptors, profile)["abi_values"]
        abi[-1] = 128
        (bad / "input.txt").write_text(" ".join(map(str, abi)) + "\n")
        stage = run(
            [str(simulator), str(bad / "input.txt"), str(bad / "simulation.json")],
            bad,
            "simulation",
            reject=True,
        )
        receipt["overflow_rejection"] = stage
        log = (bad / "simulation.log").read_text()
        if "slot" not in log.lower() or "source_check_failed" not in log:
            raise AssertionError("overflow did not fail at the intended generated slot assertion")
        receipt["status"] = "PASS_PROVISIONAL_TIMING"
    except Exception as error:
        receipt["error"] = str(error)
        raise
    finally:
        receipt["sources"] = {
            str(path.relative_to(ROOT)): digest(path)
            for path in (
                Path(__file__).resolve(),
                ROOT / "tools/analyze_pto_allgather_timing.py",
                ROOT / "src/wse_model/analysis/allgather_schedule.py",
            )
        }
        (output / "verification.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(output / "verification.json")


if __name__ == "__main__":
    main()

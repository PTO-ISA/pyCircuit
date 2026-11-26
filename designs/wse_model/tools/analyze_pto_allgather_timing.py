"""Attach existing analytical wire timing to a verified PyPTO/PTOAS workload."""

import argparse
import hashlib
import json
from pathlib import Path

from wse_model.analysis.allgather_schedule import prepare_timing
from wse_model.analysis.allgather_timing import link_occupancy_bounds
from wse_model.analysis.platform import AICORE_CLOCK_HZ, NOC_CLOCK_HZ, NOC_LINK_WIDTH_BYTES
from wse_model.compiler.allgather import lower_calendar

ROOT = Path(__file__).resolve().parents[1]


def load_workload(directory):
    """Reconstruct the exact verified descriptor stream, checking its PTO inputs."""
    program = json.loads((directory / "program.json").read_text())
    lowering = json.loads((directory / "lowering.json").read_text())
    if lowering["status"] != "PASS":
        raise ValueError("requires successfully analyzed PTOAS output")
    for phase, analysis in zip(program["phases"], lowering["analysis"], strict=True):
        for suffix, expected in (
            (".pto", phase["raw_pto_sha256"]),
            (".lowered.pto", analysis["lowered_pto_sha256"]),
        ):
            if (
                hashlib.sha256((directory / (phase["name"] + suffix)).read_bytes()).hexdigest()
                != expected
            ):
                raise ValueError("PTO artifact digest mismatch")
    descriptors = lower_calendar(program, [a["transfers"] for a in lowering["analysis"]])
    binary = (directory / "commands.bin").read_bytes()
    if binary != b"".join(d.pack() for d in descriptors):
        raise ValueError("descriptors differ from the verified lowering")
    return program, descriptors, binary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler-dir", type=Path, required=True)
    parser.add_argument(
        "--profile", type=Path, default=ROOT / "models/pto_allgather/timing_profile.json"
    )
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    directory = args.compiler_dir
    program, descriptors, binary = load_workload(directory)
    profile = json.loads(args.profile.read_text())
    timing = prepare_timing(program, descriptors, profile)
    report = {
        "status": timing["status"],
        "hardware_total_seconds": None,
        "provisional_total_seconds": timing["predicted_seconds"],
        "timing": timing,
        "profile_path": str(args.profile.resolve()),
        "profile_sha256": hashlib.sha256(args.profile.read_bytes()).hexdigest(),
        "timing_model_sha256": hashlib.sha256(
            (ROOT / "src/wse_model/analysis/allgather_schedule.py").read_bytes()
        ).hexdigest(),
        "compiler_directory": str(directory.resolve()),
        "commands_sha256": hashlib.sha256(binary).hexdigest(),
        "analyzer_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "specified": {
            "noc_clock_hz": NOC_CLOCK_HZ,
            "aicore_clock_hz": AICORE_CLOCK_HZ,
            "link_bytes_per_cycle_per_direction": NOC_LINK_WIDTH_BYTES,
        },
        "conditions": "two sequential phases; per-flit header; full directional link capacity; no protocol control traffic",
        "pending": [
            key for key, entry in profile["parameters"].items() if entry["status"] == "pending"
        ],
        "scenarios": [
            link_occupancy_bounds(descriptors, epoch_tag_bits=bits, packet_boundary=boundary)
            for bits in (8, 16)
            for boundary in ("source", "fragment")
        ],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(args.output)


if __name__ == "__main__":
    main()

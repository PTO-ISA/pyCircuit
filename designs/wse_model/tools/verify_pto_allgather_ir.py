"""Hold the generated kernel fixed and mutate one semantic field per probe."""

import argparse
import json
import re
from pathlib import Path

from lower_pto_allgather import analyze


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler-dir", type=Path, required=True)
    args = parser.parse_args()
    directory = args.compiler_dir
    manifest = json.loads((directory / "program.json").read_text())
    checks = []
    for phase in manifest["phases"]:
        original = (directory / (phase["name"] + ".lowered.pto")).read_text()
        analyze(original, phase)
        checks.append({"phase": phase["name"], "case": "original", "status": "PASS"})
        swapped = re.sub(
            r"pto\.comm\.tput\((%[\w-]+), (%[\w-]+)", r"pto.comm.tput(\2, \1", original, count=1
        )
        removed_fence = original.replace("pto.fence.barrier_all <gm>", "", 1)
        wrong_signal = re.sub(
            r"(pto\.comm\.tnotify\(%[\w-]+, )%c1_i32", r"\1%c-1_i32", original, count=1
        )
        narrow_tile = re.sub(rf"(valid_col = )%c{phase['words']}\b", r"\1%c1", original, count=1)
        wrong_context = original.replace("pto.load %arg4[", "pto.load %arg3[")
        no_pipe_flags = re.sub(
            r"^\s*pto\.(?:set|wait)_flag\[.*\]\s*$", "", original, flags=re.MULTILINE
        )
        no_barriers = re.sub(r"^\s*pto\.barrier .*?$", "", original, flags=re.MULTILINE)
        narrow_index = re.sub(
            r"(%[\w-]+) = arith.index_cast (%[\w-]+) : i64 to index",
            r"%probe_index = arith.index_cast \2 : i64 to index\n"
            r"      %probe_narrow = arith.index_cast %probe_index : index to i16\n"
            r"      \1 = arith.index_cast %probe_narrow : i16 to index",
            original,
            count=1,
        )
        overflow_rank = re.sub(
            r"(%[\w-]+) = arith.maxsi (%[\w-]+), %c0 : index",
            r"%probe_scale = arith.constant 4611686018427387904 : index\n"
            r"      %probe_product = arith.muli \2, %probe_scale : index\n"
            r"      %probe_rank = arith.divsi %probe_product, %probe_scale : index\n"
            r"      \1 = arith.maxsi %probe_rank, %c0 : index",
            original,
            count=1,
        )
        unsigned_shift = original.replace(
            "%c-1_i32 = arith.constant -1 : i32",
            "%c-1_i32 = arith.constant -1 : i32\n"
            "    %probe31 = arith.constant 31 : i32\n"
            "    %probe0 = arith.constant 0 : i32\n"
            "    %probe_shift = arith.shrui %c-1_i32, %probe31 : i32\n"
            "    %probe_credit = arith.subi %probe0, %probe_shift : i32",
            1,
        )
        unsigned_shift = re.sub(
            r"(pto\.comm\.tnotify\(%[\w-]+, )%c1_i32",
            r"\1%probe_credit",
            unsigned_shift,
            count=1,
        )
        for label, changed in (
            ("reverse-copy", swapped),
            ("remove-fence", removed_fence),
            ("wrong-signal-credit", wrong_signal),
            ("narrow-staging-valid-shape", narrow_tile),
            ("wrong-window-context", wrong_context),
            ("remove-pipe-flags", no_pipe_flags),
            ("remove-barriers", no_barriers),
            ("narrow-symbolic-index", narrow_index),
            ("overflow-rank-arithmetic", overflow_rank),
            ("negative-unsigned-shift", unsigned_shift),
        ):
            if changed == original:
                raise RuntimeError(f"probe {label} did not change its input")
            try:
                analyze(changed, phase)
            except ValueError as error:
                checks.append(
                    {
                        "phase": phase["name"],
                        "case": label,
                        "status": "EXPECTED_REJECTION",
                        "diagnostic": str(error),
                    }
                )
            else:
                raise AssertionError(f"semantic mutation accepted: {phase['name']} {label}")
    (directory / "ir-checks.json").write_text(json.dumps(checks, indent=2) + "\n")
    print(f"{len(checks)} real-IR checks passed")


if __name__ == "__main__":
    main()

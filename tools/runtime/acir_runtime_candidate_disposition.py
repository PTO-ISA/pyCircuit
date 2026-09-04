#!/usr/bin/env python3
"""Reconcile structural candidate adapters with the accepted runtime catalog."""

from __future__ import annotations

import argparse
import json
from collections import Counter
from pathlib import Path
from typing import Any


ALIASES = {
    "candidate/pulp_common_cells/stream_arbiter": "pulp-stream-arbiter",
    "candidate/pulp_common_cells/stream_demux": "pulp-stream-demux",
    "candidate/pulp_common_cells/stream_mux": "pulp-stream-mux",
    "candidate/pulp_common_cells/stream_fork": "pulp-stream-fork",
    "candidate/pulp_common_cells/stream_join": "pulp-stream-join",
    "candidate/pulp_common_cells/stream_register": "pulp-stream-register",
    "candidate/pulp_common_cells/spill_register": "pulp-spill-register",
    "candidate/pulp_common_cells/spill_register_flushable": "pulp-spill-register-flushable",
    "candidate/pulp_common_cells/isochronous_spill_register": "pulp-isochronous-spill-register",
    "candidate/pulp_common_cells/fall_through_register": "pulp-fall-through-register",
    "candidate/pulp_common_cells/stream_fork_dynamic": "pulp-stream-fork-dynamic",
    "candidate/pulp_common_cells/stream_join_dynamic": "pulp-stream-join-dynamic",
    "candidate/pulp_common_cells/lzc": "pulp-lzc",
    "candidate/pulp_common_cells/popcount": "pulp-cc-popcount",
    "candidate/pulp_common_cells/rr_arb_tree": "pulp-rr-arb-tree",
    "candidate/basejump_stl/bsg_arb_round_robin": "basejump-rr-arbiter",
    "candidate/basejump_stl/bsg_arb_round_robin_composable": "basejump-rr-composable",
    "candidate/basejump_stl/bsg_arb_round_robin_two_level": "basejump-rr-two-level",
    "candidate/basejump_stl/bsg_round_robin_1_to_n": "basejump-rr-1-to-n",
    "candidate/basejump_stl/bsg_round_robin_n_to_1": "basejump-rr-n-to-1",
    "candidate/basejump_stl/bsg_round_robin_2_to_2": "basejump-rr-2-to-2",
    "candidate/basejump_stl/bsg_round_robin_fifo_to_fifo": "basejump-rr-fifo-to-fifo",
    "candidate/basejump_stl/bsg_popcount": "basejump-popcount",
    "candidate/basejump_stl/bsg_counter_clear_up_saturating": "basejump-counter-clear-up-saturating",
    "candidate/basejump_stl/bsg_counting_leading_zeros": "basejump-clz",
    "candidate/opentitan/prim_sum_tree": "opentitan-sum-tree",
    "candidate/opentitan/prim_max_tree": "opentitan-max-tree",
    "candidate/vortex/VX_popcount": "vortex-popcount",
    "candidate/vortex/VX_rr_arbiter": "vortex-rr-arbiter",
    "candidate/vortex/VX_lzc": "vortex-lzc",
    "candidate/vortex/VX_skid_buffer": "vortex-skid-buffer",
    "candidate/vortex/VX_stream_fork": "vortex-stream-fork",
    "candidate/vortex/VX_stream_join": "vortex-stream-join",
    "candidate/pulp_common_cells/stream_arbiter_flushable": "pulp-stream-arbiter-flushable",
    # Fixed-width Vortex helpers are already exercised through the generic
    # parameterized popcount wrapper and are not separate public primitives.
    "candidate/vortex/VX_sum33": "vortex-popcount",
    "candidate/vortex/VX_popcount32": "vortex-popcount",
    "candidate/vortex/VX_popcount63": "vortex-popcount",
    # These three modules are implementation stages of the FIFO-to-FIFO
    # engine; exposing them independently would freeze an internal interface.
    "candidate/basejump_stl/bsg_rr_f2f_input": "basejump-rr-fifo-to-fifo",
    "candidate/basejump_stl/bsg_rr_f2f_middle": "basejump-rr-fifo-to-fifo",
    "candidate/basejump_stl/bsg_rr_f2f_output": "basejump-rr-fifo-to-fifo",
    "candidate/basejump_stl/bsg_mem_banked_crossbar_control_o_by_i": "basejump-crossbar-control",
    "candidate/vortex/bf16_to_fp32": "vortex-bf16-to-fp32",
}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--candidates", type=Path, default=Path(".pycircuit_out/runtime-candidate-adapters-final15/catalog.json"))
    parser.add_argument("--catalog", type=Path, default=Path("library/verilog/catalog.json"))
    parser.add_argument("--output", type=Path, default=Path(".pycircuit_out/runtime-candidate-disposition-v25/report.json"))
    args = parser.parse_args()
    candidate_doc: dict[str, Any] = json.loads(args.candidates.read_text(encoding="utf-8"))
    runtime_doc: dict[str, Any] = json.loads(args.catalog.read_text(encoding="utf-8"))
    accepted = {str(entry.get("name")): entry for entry in runtime_doc.get("entries", []) if entry.get("status") == "accepted"}
    rows = []
    for candidate in candidate_doc.get("entries", []):
        name = str(candidate.get("name")); runtime_name = ALIASES.get(name)
        if runtime_name not in accepted:
            implementation = str(candidate.get("implementation", ""))
            source_file = str(candidate.get("provenance", {}).get("source_file", ""))
            source_base = source_file.rsplit("/", 1)[-1].rsplit("\\", 1)[-1]
            source_stem = source_base.rsplit(".", 1)[0]
            for accepted_name, accepted_entry in accepted.items():
                accepted_impl = str(accepted_entry.get("implementation", ""))
                if implementation and implementation in {accepted_impl, source_stem}:
                    runtime_name = accepted_name
                    break
        if runtime_name in accepted:
            status = "accepted_runtime"
        elif candidate.get("status") == "staged_structural":
            status = "structural_only_pending_semantic_oracle"
        else:
            status = "blocked_or_unresolved"
        covered_by = runtime_name if name in ALIASES and name != runtime_name else None
        disposition = "covered_by_parameterized_runtime" if covered_by else "independent_runtime"
        rows.append({"candidate": name, "implementation": candidate.get("implementation"), "status": status, "runtime": runtime_name, "covered_by": covered_by, "disposition": disposition, "structural_configs": len(candidate.get("sweep_configs", [])), "structural_status": candidate.get("validation", {}).get("structural_status"), "verilator": all(c.get("verilator_lint") == "PASS" for c in candidate.get("sweep_configs", [])), "simulation": all(c.get("simulation") == "PASS" for c in candidate.get("sweep_configs", [])), "synthesis": all(c.get("synthesis") in {"PASS", "NOT_APPLICABLE_NON_SYNTH"} for c in candidate.get("sweep_configs", []))})
    counts = Counter(row["status"] for row in rows)
    report = {"schema": "acir-runtime-candidate-disposition-v0.1", "source_candidates": str(args.candidates), "runtime_catalog": str(args.catalog), "summary": {"candidates": len(rows), **dict(counts)}, "entries": rows, "next_work": [row["candidate"] for row in rows if row["status"] != "accepted_runtime"]}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(json.dumps(report["summary"], ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

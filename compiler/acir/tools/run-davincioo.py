#!/usr/bin/env python3
"""Run the generated DavinciOO gfsim on one canonical PTO trace."""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from html import escape
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "python/semantic-core/src"))
sys.path.insert(0, str(ROOT / "python/agentic-circuit/src"))
sys.path.insert(0, str(ROOT / "examples/agentic-circuit/architecture"))
sys.path.insert(0, str(ROOT / "compiler/acir/tools"))

from agentic_circuit._canonical_json import (  # noqa: E402
    canonical_json_bytes,
    sha256_bytes,
)
from davincioo_jit import specialization  # noqa: E402
from pto_trace_adapter import convert_davincioo_trace  # noqa: E402
from pto_trace_oracle import compare_results, make_result, publish_report  # noqa: E402

DEFAULT_TRACE = (
    ROOT
    / "third_party/references/davincioo-gfsim/upstream/tests/fixtures/traces"
    / "examples_intermediate_softmax.pto.trace"
)
DEFAULT_PROJECTION = (
    ROOT / "tests/goldens/agentic-circuit/davincioo/softmax-projection.json"
)
HARNESS = ROOT / "examples/agentic-circuit/architecture/davincioo_trace_harness.cpp"
REFERENCE_SOURCE = ROOT / "third_party/references/davincioo-gfsim/SOURCE.json"


def fixture_header(trace: dict[str, object], projection: dict[str, object]) -> str:
    records = trace["records"]
    waits = projection["waits_for"]
    routes = projection["routes"]
    costs = projection["model_cost"]
    values = projection["architectural_values"]
    if not all(isinstance(item, list) for item in (records, waits, values)):
        raise ValueError("trace/projection arrays are malformed")
    if len(records) != len(waits) or len(records) != len(values):
        raise ValueError("trace and projection record counts differ")
    if not isinstance(routes, dict) or not isinstance(costs, dict):
        raise ValueError("projection route/cost maps are malformed")

    tokens: list[str] = []
    opcodes: list[str] = []
    for index, raw in enumerate(records):
        if not isinstance(raw, dict):
            raise ValueError("trace record is not an object")
        sequence = raw.get("sequence_id")
        opcode = raw.get("opcode")
        if sequence != index or not isinstance(opcode, str):
            raise ValueError("trace sequence/opcode contract is not canonical")
        route = routes.get(opcode)
        cost = costs.get(opcode)
        wait = waits[index]
        value = values[index]
        if not all(isinstance(item, int) for item in (route, cost, wait, value)):
            raise ValueError(f"projection is incomplete for opcode {opcode}")
        if not (0 <= route < 4 and 0 < cost < (1 << 16)):
            raise ValueError(f"projection values are out of range for {opcode}")
        input_value = (value - 1) & 0xFFFFFFFF
        tokens.append(
            "      ac_generated::PTOInst{"
            f"{sequence}, {wait}, {route}, {cost}, {input_value}"
            "}"
        )
        opcodes.append(json.dumps(opcode))

    return "\n".join(
        (
            "#pragma once",
            "",
            "#include <array>",
            "#include <string_view>",
            "",
            "namespace davincioo_fixture {",
            f"inline constexpr std::array<ac_generated::PTOInst, {len(tokens)}> kTokens{{{{",
            ",\n".join(tokens),
            "}};",
            f"inline constexpr std::array<std::string_view, {len(opcodes)}> kOpcodes{{{{",
            "      " + ", ".join(opcodes),
            "}};",
            "} // namespace davincioo_fixture",
            "",
        )
    )


def parse_harness_output(text: str) -> dict[str, object]:
    result: dict[str, object] = {"spans": []}
    for line in text.splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == "SUMMARY" and len(fields) == 3:
            result["cycles"] = int(fields[1])
            result["retired_count"] = int(fields[2])
        elif fields[0] == "COMPLETION":
            result["completion_order"] = [int(item) for item in fields[1:]]
        elif fields[0] == "RETIREMENT":
            result["retirement_order"] = [int(item) for item in fields[1:]]
        elif fields[0] == "VALUES":
            result["architectural_values"] = [int(item) for item in fields[1:]]
        elif fields[0] == "SPAN" and len(fields) == 6:
            spans = result["spans"]
            assert isinstance(spans, list)
            spans.append(
                {
                    "sequence": int(fields[1]),
                    "opcode": fields[2],
                    "stage": fields[3],
                    "begin": int(fields[4]),
                    "end": int(fields[5]),
                }
            )
    required = {
        "cycles",
        "retired_count",
        "completion_order",
        "retirement_order",
        "architectural_values",
        "spans",
    }
    if set(result) != required:
        raise ValueError(
            f"harness output is incomplete: {sorted(set(result) ^ required)}"
        )
    return result


def parse_reference_cycle_output(
    text: str, records: list[dict[str, object]]
) -> dict[str, object]:
    observations: list[dict[str, object]] = []
    for line in text.splitlines():
        if not line.startswith("retire_index="):
            continue
        fields: dict[str, str] = {}
        for token in line.split():
            if "=" in token:
                key, value = token.split("=", 1)
                fields[key] = value
        required = {
            "retire_index",
            "sequence_id",
            "opcode",
            "alloc_cycle",
            "rename_cycle",
            "dispatch_cycle",
            "issue_cycle",
            "engine_pop_cycle",
            "engine_complete_cycle",
            "retire_cycle",
        }
        if not required <= set(fields):
            raise ValueError("reference cycle replay record is incomplete")
        sequence = int(fields["sequence_id"])
        if (
            not 0 <= sequence < len(records)
            or fields["opcode"] != records[sequence]["opcode"]
        ):
            raise ValueError("reference cycle replay identity differs from trace")
        observations.append(
            {
                "sequence_id": sequence,
                "opcode": fields["opcode"],
                "retirement_ordinal": int(fields["retire_index"]),
                "timestamps": {
                    "allocate": int(fields["alloc_cycle"]),
                    "rename": int(fields["rename_cycle"]),
                    "dispatch": int(fields["dispatch_cycle"]),
                    "issue": int(fields["issue_cycle"]),
                    "engine_pop": int(fields["engine_pop_cycle"]),
                    "complete": int(fields["engine_complete_cycle"]),
                    "retire": int(fields["retire_cycle"]),
                },
            }
        )
    if len(observations) != len(records):
        raise ValueError("reference cycle replay record count differs from trace")
    retirement_order = [item["sequence_id"] for item in observations]
    completion_order = [
        item["sequence_id"]
        for item in sorted(
            observations,
            key=lambda item: (
                item["timestamps"]["complete"],
                item["sequence_id"],
            ),
        )
    ]
    return {
        "schema": "agentic-circuit-davincioo-reference-observations",
        "version": "0.1",
        "contract_epoch": "0.5",
        "records": observations,
        "completion_order": completion_order,
        "retirement_order": retirement_order,
    }


def render_svg(run: dict[str, object], opcodes: list[str]) -> str:
    cycles = int(run["cycles"])
    spans = run["spans"]
    assert isinstance(spans, list)
    lane_height = 25
    left = 180
    top = 62
    scale = max(1.5, min(4.0, 1200.0 / max(1, cycles)))
    width = int(left + cycles * scale + 30)
    height = top + len(opcodes) * lane_height + 64
    colors = {
        "source_wait": "#d1d5db",
        "incoming": "#93c5fd",
        "decoded": "#60a5fa",
        "pipelined": "#818cf8",
        "dispatch_scalar": "#fbbf24",
        "dispatch_vector": "#f59e0b",
        "dispatch_cube": "#d97706",
        "dispatch_tma": "#b45309",
        "dispatched": "#f97316",
        "schedule_execute": "#ef4444",
        "completed": "#34d399",
        "rob_wait": "#10b981",
        "retired": "#059669",
    }
    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#ffffff"/>',
        "<style>text{font-family:ui-monospace,SFMono-Regular,Menlo,monospace;font-size:11px}.title{font-size:15px;font-weight:600}</style>",
        f'<text class="title" x="12" y="20">Generated DavinciOO PTO trace swimlane — {cycles} cycles</text>',
    ]
    tick_step = max(1, cycles // 10)
    for tick in range(0, cycles + 1, tick_step):
        x = left + tick * scale
        lines.append(
            f'<line x1="{x:.1f}" y1="{top - 14}" x2="{x:.1f}" y2="{height - 28}" stroke="#e5e7eb"/>'
        )
        lines.append(f'<text x="{x + 2:.1f}" y="{top - 20}">{tick}</text>')
    for sequence, opcode in enumerate(opcodes):
        y = top + sequence * lane_height
        lines.append(f'<text x="8" y="{y + 15}">{sequence:02d} {escape(opcode)}</text>')
        lines.append(
            f'<line x1="{left}" y1="{y + lane_height - 3}" x2="{width - 20}" y2="{y + lane_height - 3}" stroke="#f3f4f6"/>'
        )
    for raw in spans:
        assert isinstance(raw, dict)
        sequence = int(raw["sequence"])
        begin = int(raw["begin"])
        end = int(raw["end"])
        stage = str(raw["stage"])
        if end <= begin:
            continue
        x = left + begin * scale
        y = top + sequence * lane_height + 3
        span_width = max(1.0, (end - begin) * scale)
        color = colors.get(stage, "#a78bfa")
        lines.append(
            f'<rect x="{x:.1f}" y="{y}" width="{span_width:.1f}" height="17" rx="2" fill="{color}"><title>{escape(stage)} [{begin},{end})</title></rect>'
        )
    legend = [
        ("frontend", "#60a5fa"),
        ("dispatch", "#f59e0b"),
        ("schedule/execute", "#ef4444"),
        ("ROB wait", "#10b981"),
        ("retired", "#059669"),
    ]
    legend_x = left
    legend_y = height - 30
    for label, color in legend:
        lines.append(
            f'<rect x="{legend_x}" y="{legend_y - 12}" width="12" height="12" rx="2" fill="{color}"/>'
        )
        lines.append(f'<text x="{legend_x + 17}" y="{legend_y - 2}">{label}</text>')
        legend_x += 125
    lines.append(f'<text x="{left}" y="{top - 36}">cycle</text>')
    lines.append("</svg>")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--trace", type=Path, default=DEFAULT_TRACE)
    parser.add_argument("--projection", type=Path, default=DEFAULT_PROJECTION)
    parser.add_argument(
        "--output-dir", type=Path, default=ROOT / "build/davincioo-trace"
    )
    parser.add_argument("--cxx", default=shutil.which("c++") or "c++")
    parser.add_argument("--reference-executable", type=Path, required=True)
    arguments = parser.parse_args()

    output = arguments.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    raw_trace_bytes = arguments.trace.read_bytes()
    reference_input = output / "reference-input.jsonl"
    reference_input.write_bytes(raw_trace_bytes)
    canonical_bytes = convert_davincioo_trace(
        raw_trace_bytes, source_program="davincioo-softmax"
    )
    canonical = json.loads(canonical_bytes)
    projection = json.loads(arguments.projection.read_text(encoding="utf-8"))
    reference_source = json.loads(REFERENCE_SOURCE.read_text(encoding="utf-8"))
    if projection["source"]["repository_commit"] != reference_source["commit"]:
        parser.error("projection and imported reference revisions differ")
    records = canonical["records"]
    opcodes = [str(record["opcode"]) for record in records]

    reference_summary_path = output / "reference-runtime-summary.json"
    reference = subprocess.run(
        (
            str(arguments.reference_executable.resolve()),
            "simulate",
            "--trace",
            str(reference_input),
            "--summary-out",
            str(reference_summary_path),
            "--dump-cycles",
        ),
        text=True,
        capture_output=True,
        check=False,
    )
    if reference.returncode != 0:
        parser.error(
            "DavinciOO reference execution failed "
            f"({reference.returncode}):\n{reference.stderr}"
        )
    reference_summary = json.loads(reference_summary_path.read_bytes())
    try:
        reference_observations = parse_reference_cycle_output(reference.stdout, records)
    except (KeyError, TypeError, ValueError) as error:
        parser.error(f"DavinciOO reference cycle replay is invalid: {error}")
    if (
        reference_summary.get("record_count") != len(records)
        or reference_summary.get("opcode_counts") != projection["opcode_counts"]
        or reference_summary.get("simulated_cycles")
        != projection["simulated_cycles"]
        or reference_observations["completion_order"]
        != projection["completion_order"]
        or reference_observations["retirement_order"]
        != projection["retirement_order"]
    ):
        parser.error("live DavinciOO reference differs from the pinned projection")
    reference_observations["raw_trace_sha256"] = sha256_bytes(raw_trace_bytes)
    reference_observations["trace_content_hash"] = canonical["metadata"][
        "content_hash"
    ]
    reference_observations["reference_revision"] = reference_source["commit"]
    (output / "reference-runtime-observations.json").write_bytes(
        canonical_json_bytes(reference_observations) + b"\n"
    )

    (output / "canonical-trace.json").write_bytes(canonical_bytes)
    (output / "davincioo.generated.cpp").write_text(
        specialization.lower_cpp(), encoding="utf-8"
    )
    (output / "davincioo_trace_fixture.hpp").write_text(
        fixture_header(canonical, projection), encoding="utf-8"
    )
    executable = output / "davincioo-trace-sim"
    build = subprocess.run(
        (
            arguments.cxx,
            "-std=c++20",
            "-O2",
            "-I",
            str(ROOT / "simulator/gfsim/include"),
            "-I",
            str(output),
            str(HARNESS),
            "-o",
            str(executable),
        ),
        text=True,
        capture_output=True,
        check=False,
    )
    if build.returncode != 0:
        parser.error(f"generated gfsim compilation failed:\n{build.stderr}")
    completed = subprocess.run(
        (str(executable),), text=True, capture_output=True, check=False
    )
    if completed.returncode != 0:
        parser.error(
            "generated gfsim execution failed "
            f"({completed.returncode}):\n{completed.stderr}"
        )
    run = parse_harness_output(completed.stdout)
    for key in ("completion_order", "retirement_order", "architectural_values"):
        if run[key] != projection[key]:
            parser.error(f"generated gfsim {key} differs from projection")
    if run["retired_count"] != len(records):
        parser.error("generated gfsim retired count differs from trace")

    report = {
        "schema": "agentic-circuit-davincioo-run",
        "version": "0.1",
        "contract_epoch": "0.5",
        "specialization": specialization.fingerprint,
        "trace_content_hash": canonical["metadata"]["content_hash"],
        "record_count": len(records),
        "reference_cycles": projection["simulated_cycles"],
        **run,
    }
    (output / "run.json").write_bytes(canonical_json_bytes(report) + b"\n")
    reference_model = make_result(
        trace_content_hash=canonical["metadata"]["content_hash"],
        model={
            "kind": "davincioo-pinned-reference-projection",
            "revision": projection["source"]["repository_commit"],
            "specialization": "basic-core-model",
        },
        trace_records=records,
        architectural_values=projection["architectural_values"],
        completion_order=projection["completion_order"],
        retirement_order=projection["retirement_order"],
        run_timestamps={"complete": projection["simulated_cycles"]},
    )
    candidate_model = make_result(
        trace_content_hash=canonical["metadata"]["content_hash"],
        model={
            "kind": "frozen-acir-generated-gfsim",
            "revision": "agentic-circuit-contract-0.5",
            "specialization": specialization.fingerprint,
        },
        trace_records=records,
        architectural_values=run["architectural_values"],
        completion_order=run["completion_order"],
        retirement_order=run["retirement_order"],
        run_timestamps={"complete": run["cycles"]},
    )
    oracle_report = compare_results(reference_model, candidate_model)
    (output / "reference-result.json").write_bytes(
        canonical_json_bytes(reference_model) + b"\n"
    )
    (output / "candidate-result.json").write_bytes(
        canonical_json_bytes(candidate_model) + b"\n"
    )
    publish_report(output / "oracle-report.json", oracle_report)
    if oracle_report["status"] != "passed":
        parser.error("generated gfsim differs from the structured PTO trace oracle")
    (output / "swimlane.svg").write_text(render_svg(report, opcodes), encoding="utf-8")
    sys.stdout.write(
        f"generated_cycles={report['cycles']} "
        f"reference_cycles={report['reference_cycles']} "
        f"records={report['record_count']} "
        "reference_runtime_verified=true\n"
        f"{output / 'run.json'}\n"
        f"{output / 'swimlane.svg'}\n"
        f"{output / 'oracle-report.json'}\n"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

from __future__ import annotations

import importlib.util
import json
from pathlib import Path

import pytest

pytestmark = pytest.mark.unit
ROOT = Path(__file__).resolve().parents[2]


@pytest.fixture
def inventory_module():
    spec = importlib.util.spec_from_file_location(
        "benchmark_inventory", ROOT / "benchmark/inventory.py"
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def configure(module, tmp_path: Path) -> Path:
    module.ROOT = tmp_path
    module.BENCHMARK = tmp_path / "benchmark"
    module.UPSTREAM = module.BENCHMARK / "upstream"
    module.CATALOG = module.BENCHMARK / "catalog.jsonl"
    module.INVENTORY = module.BENCHMARK / "inventory.json"
    module.BENCHMARK.mkdir()
    return module.UPSTREAM


def test_verilogeval_v2_retains_distinct_variants_for_same_ids(
    inventory_module, tmp_path: Path
) -> None:
    upstream = configure(inventory_module, tmp_path)
    for directory, interface in (
        ("dataset_spec-to-rtl", False),
        ("dataset_code-complete-iccad2023", True),
    ):
        root = upstream / "verilogeval-v2" / directory
        root.mkdir(parents=True)
        (root / "Prob001_zero_prompt.txt").write_text("Drive output low.\n")
        (root / "Prob001_zero_ref.sv").write_text("module top; endmodule\n")
        (root / "Prob001_zero_test.sv").write_text("module tb; endmodule\n")
        if interface:
            (root / "Prob001_zero_ifc.txt").write_text("module top(output zero);\n")

    rows = inventory_module.verilogeval_v2()

    assert [(row["task_id"], row["variant"], row["task_kind"]) for row in rows] == [
        ("Prob001_zero", "spec-to-rtl", "rtl_generation"),
        ("Prob001_zero", "code-complete-iccad2023", "completion"),
    ]
    assert all("prompt" not in row for row in rows)
    assert all(row["source_sha256"] for row in rows)


def test_generic_jsonl_deduplicates_solution_copies_and_keeps_prompt_pointer(
    inventory_module, tmp_path: Path
) -> None:
    upstream = configure(inventory_module, tmp_path)
    data = upstream / "cvdp" / "data"
    data.mkdir(parents=True)
    payload = {
        "id": "arbiter-1",
        "prompt": "Implement an arbiter",
        "categories": ["protocol"],
    }
    (data / "tasks.jsonl").write_text(json.dumps(payload) + "\n")
    (data / "tasks_with_solutions.jsonl").write_text(
        json.dumps({**payload, "solution": "hidden"}) + "\n"
    )

    rows = inventory_module.generic_jsonl_suite(
        "cvdp", [data], "rtl_generation", "original harness"
    )

    assert len(rows) == 1
    assert rows[0]["task_id"] == "arbiter-1"
    assert rows[0]["prompt_field"] == "prompt"
    assert rows[0]["category"] == "protocol"
    assert "solution" not in rows[0]


def test_rebuild_is_deterministic_and_reports_missing_payloads(
    inventory_module, tmp_path: Path
) -> None:
    configure(inventory_module, tmp_path)
    source = inventory_module.BENCHMARK / "question.txt"
    source.write_text("Build a counter.\n")
    item = inventory_module.row(
        "suite-a",
        "counter",
        "spec",
        "rtl_generation",
        source,
        "simulation",
        description=source,
        prompt="Build a counter.",
    )
    inventory_module.EXTRACTORS = (lambda: [item],)
    inventory_module.EXPECTED_SUITES = ("suite-a", "suite-b")

    first_rows, first_report = inventory_module.rebuild()
    first_catalog = inventory_module.CATALOG.read_bytes()
    second_rows, second_report = inventory_module.rebuild()

    assert first_rows == second_rows
    assert first_report == second_report
    assert inventory_module.CATALOG.read_bytes() == first_catalog
    assert first_report["sources"] == [
        {"suite": "suite-a", "status": "available", "catalog_rows": 1},
        {"suite": "suite-b", "status": "missing_question_payload", "catalog_rows": 0},
    ]


def test_hwe_uses_shards_for_lfs_pointer_and_prefers_materialized_aggregate(
    inventory_module, tmp_path: Path
) -> None:
    upstream = configure(inventory_module, tmp_path)
    data = upstream / "hf-hwe-bench"
    data.mkdir(parents=True)
    pointer = (
        "version https://git-lfs.github.com/spec/v1\n"
        + "oid sha256:"
        + "0" * 64
        + "\nsize 100\n"
    )
    full = data / "hwe_bench_full.jsonl"
    full.write_text(pointer)
    first = {
        "org": "lowRISC",
        "repo": "ibex",
        "number": 1,
        "problem_statement": "Fix counter reset",
    }
    second = {
        "org": "openhwgroup",
        "repo": "cva6",
        "number": 2,
        "problem_statement": "Fix bus handshake",
    }
    (data / "lowRISC__ibex.jsonl").write_text(json.dumps(first) + "\n")
    (data / "openhwgroup__cva6.jsonl").write_text(json.dumps(second) + "\n")

    assert len(inventory_module.hwe_bench()) == 2

    full.write_text(json.dumps(first) + "\n" + json.dumps(second) + "\n")
    rows = inventory_module.hwe_bench()
    assert len(rows) == 2
    assert {row["source_path"] for row in rows} == {
        "benchmark/upstream/hf-hwe-bench/hwe_bench_full.jsonl"
    }


def test_cvdp_uses_only_latest_dataset_release(
    inventory_module, tmp_path: Path
) -> None:
    upstream = configure(inventory_module, tmp_path)
    data = upstream / "hf-cvdp"
    data.mkdir(parents=True)
    old = {"id": "old", "prompt": "old question"}
    current = {"id": "current", "prompt": "current question"}
    (data / "cvdp_v1.0.4_nonagentic_code_generation.jsonl").write_text(
        json.dumps(old) + "\n"
    )
    (data / "cvdp_v1.1.0_nonagentic_code_generation.jsonl").write_text(
        json.dumps(current) + "\n"
    )

    rows = inventory_module.cvdp()

    assert [row["task_id"] for row in rows] == ["current"]


def test_hwfix_question_pointer_does_not_select_fix_summary(inventory_module, tmp_path):
    upstream = configure(inventory_module, tmp_path)
    data = upstream / "hf-hwfix"
    data.mkdir(parents=True)
    (data / "issue_benchmark.json").write_text(
        json.dumps(
            [
                {
                    "repo": "core",
                    "issue_number": 4,
                    "body": "The counter stalls.",
                    "description": "The reference fix changes the reset gate.",
                    "files": [
                        {
                            "base": "original source",
                            "modified": "reference answer",
                            "patch": "answer patch",
                        }
                    ],
                }
            ]
        )
    )
    row = inventory_module.hwfix()[0]
    assert row["prompt_field"] == "body"
    assert row["task_kind"] == "issue_analysis"
    assert "reference answer" not in json.dumps(row)


def test_cvdp_preserves_verification_and_repair_categories(inventory_module, tmp_path):
    upstream = configure(inventory_module, tmp_path)
    data = upstream / "hf-cvdp"
    data.mkdir(parents=True)
    records = [
        {
            "id": "stimulus",
            "prompt": "Write a test driver",
            "categories": ["cid012", "easy"],
        },
        {
            "id": "fix",
            "prompt": "Correct the existing RTL",
            "categories": ["cid016", "hard"],
        },
    ]
    (data / "cvdp_v1.1.0_agentic_code_generation_no_commercial.jsonl").write_text(
        "".join(json.dumps(row) + "\n" for row in records)
    )
    rows = inventory_module.cvdp()
    assert {row["task_id"]: row["task_kind"] for row in rows} == {
        "stimulus": "verification_stimulus",
        "fix": "repair",
    }

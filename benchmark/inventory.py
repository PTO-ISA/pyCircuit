#!/usr/bin/env python3
"""Build a deterministic inventory of locally available benchmark questions."""

from __future__ import annotations

import argparse
import ast
import functools
import hashlib
import json
import re
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BENCHMARK = ROOT / "benchmark"
UPSTREAM = BENCHMARK / "upstream"
CATALOG = BENCHMARK / "catalog.jsonl"
INVENTORY = BENCHMARK / "inventory.json"

EXPECTED_SUITES = (
    "verilogeval-2023",
    "verilogeval-v2",
    "cvdp",
    "hwe-bench",
    "realbench",
    "chipverilog",
    "resbench",
    "rtl-repo",
    "notsotiny-framework",
    "hsco-bench",
    "cirfix",
    "hwfix",
)


def relative(path: Path) -> str:
    return path.resolve().relative_to(ROOT).as_posix()


@functools.cache
def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def json_lines(path: Path) -> list[dict]:
    if path.read_bytes()[:42] == b"version https://git-lfs.github.com/spec/v1":
        return []
    return [json.loads(line) for line in path.read_text().splitlines() if line.strip()]


def is_lfs_pointer(path: Path) -> bool:
    return (
        path.is_file()
        and path.read_bytes()[:42] == b"version https://git-lfs.github.com/spec/v1"
    )


def classify(
    text: str, path: str = "", metadata: str | None = None
) -> tuple[str, str, str]:
    """Return hardware category, confidence, and evidence source."""

    normalized_metadata = (metadata or "").lower().replace("-", "_").replace(" ", "_")
    for explicit in (
        "combinational",
        "sequential",
        "fsm",
        "arithmetic",
        "memory",
        "protocol",
        "complex_ip",
    ):
        if explicit in normalized_metadata:
            return explicit, "high", "metadata"
    haystack = re.sub(r"[_/\\-]+", " ", " ".join((metadata or "", path, text)).lower())
    rules = (
        ("memory", r"\b(memory|ram|sram|rom|cache|fifo|register file|buffer)\b"),
        (
            "protocol",
            r"\b(i2c|wishbone|axi|uart|spi|bus|arbiter|handshake|controller)\b",
        ),
        ("fsm", r"\b(fsm|finite state|state machine)\b"),
        (
            "arithmetic",
            r"\b(alu|adder|subtract|multiply|divider|division|fpu|cordic|crc|aes|sha|parity|popcount|polynomial|math)\b",
        ),
        (
            "sequential",
            r"\b(counter|flip.flop|shift register|pipeline|clock|lfsr|sequential)\b",
        ),
        (
            "combinational",
            r"\b(mux|multiplexer|decoder|encoder|gate|combinational|truth table)\b",
        ),
        (
            "complex_ip",
            r"\b(cpu|processor|soc|core|mmu|dma|accelerator|neural|bert|vision|system.level)\b",
        ),
    )
    for category, pattern in rules:
        if re.search(pattern, haystack):
            source = (
                "metadata"
                if metadata and re.search(pattern, metadata.lower())
                else "prompt_path_keyword"
            )
            return category, "high" if source == "metadata" else "medium", source
    return "other", "low", "no_specific_hardware_signal"


def row(
    suite: str,
    task_id: str,
    variant: str,
    task_kind: str,
    source: Path,
    evaluation: str,
    *,
    description: Path | None = None,
    prompt_field: str | None = None,
    prompt: str = "",
    metadata: str | None = None,
    porting_status: str = "not_ported",
    reason: str = "Question is locally available but has no pyCircuit port.",
) -> dict:
    category, confidence, category_source = classify(prompt, relative(source), metadata)
    result = {
        "suite": suite,
        "task_id": str(task_id),
        "variant": variant,
        "task_kind": task_kind,
        "category": category,
        "category_confidence": confidence,
        "category_source": category_source,
        "source_path": relative(source),
        "source_sha256": digest(source),
        "evaluation_protocol": evaluation,
        "porting_status": porting_status,
        "reason": reason,
    }
    if description is not None:
        result["description_path"] = relative(description)
        result["description_sha256"] = digest(description)
    if prompt_field is not None:
        result["prompt_field"] = prompt_field
    return result


def verilogeval_2023() -> list[dict]:
    base = UPSTREAM / "verilogeval-2023/data"
    rows = []
    for variant, filename in (
        ("human", "VerilogEval_Human.jsonl"),
        ("machine", "VerilogEval_Machine.jsonl"),
    ):
        path = base / filename
        for item in json_lines(path):
            rows.append(
                row(
                    "verilogeval-2023",
                    item["task_id"],
                    variant,
                    "rtl_generation",
                    path,
                    "Original VerilogEval compile/simulation test field; canonical solution remains hidden.",
                    prompt_field="prompt",
                    prompt=item.get("prompt", ""),
                )
            )
    return rows


def verilogeval_v2() -> list[dict]:
    rows = []
    for variant, dirname, kind in (
        ("spec-to-rtl", "dataset_spec-to-rtl", "rtl_generation"),
        ("code-complete-iccad2023", "dataset_code-complete-iccad2023", "completion"),
    ):
        base = UPSTREAM / "verilogeval-v2" / dirname
        for prompt in sorted(base.glob("*_prompt.txt")):
            task_id = prompt.name.removesuffix("_prompt.txt")
            rows.append(
                row(
                    "verilogeval-v2",
                    task_id,
                    variant,
                    kind,
                    prompt,
                    "Original per-problem testbench and reference RTL protocol.",
                    description=prompt,
                    prompt=prompt.read_text(errors="replace"),
                )
            )
    return rows


def generic_jsonl_suite(
    suite: str, roots: list[Path], task_kind: str, evaluation: str, include=None
) -> list[dict]:
    rows, seen = [], set()
    for root in roots:
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*.jsonl")):
            if include is not None and not include(path):
                continue
            if "with_solutions" in path.name:
                continue
            for index, item in enumerate(json_lines(path)):
                task_id = next(
                    (
                        item.get(key)
                        for key in (
                            "task_id",
                            "id",
                            "instance_id",
                            "problem_id",
                            "name",
                        )
                        if item.get(key) is not None
                    ),
                    f"row-{index:06d}",
                )
                prompt_field = next(
                    (
                        key
                        for key in (
                            "prompt",
                            "input",
                            "problem",
                            "problem_statement",
                            "description",
                            "instruction",
                        )
                        if key in item
                    ),
                    None,
                )
                variant = str(item.get("variant") or item.get("task_type") or path.stem)
                key = (str(task_id), variant)
                if key in seen:
                    continue
                seen.add(key)
                metadata = json.dumps(
                    item.get("categories", item.get("category", "")), sort_keys=True
                )
                prompt_value = item.get(prompt_field, "") if prompt_field else ""
                if isinstance(prompt_value, dict):
                    prompt_value = prompt_value.get(
                        "prompt", json.dumps(prompt_value, sort_keys=True)
                    )
                rows.append(
                    row(
                        suite,
                        str(task_id),
                        variant,
                        task_kind,
                        path,
                        evaluation,
                        prompt_field=prompt_field,
                        prompt=str(prompt_value),
                        metadata=metadata,
                        porting_status="requires_original_protocol",
                        reason="Evaluation depends on the suite's repository/container harness.",
                    )
                )
    return rows


# Task semantics from pinned cvdp/src/model_helpers.py category_guidance,
# rather than inferring every code_generation partition to be a new RTL DUT.
CVDP_TASK_KINDS = {
    "cid002": "completion",
    "cid003": "rtl_generation",
    "cid004": "rtl_modification",
    "cid005": "rtl_generation",
    "cid006": "code_correspondence",
    "cid007": "rtl_optimization",
    "cid008": "testbench_correspondence",
    "cid009": "rtl_qa",
    "cid010": "verification_qa",
    "cid012": "verification_stimulus",
    "cid013": "verification_checker",
    "cid014": "verification_assertion",
    "cid016": "repair",
}


def cvdp_categories(rows):
    definitions = {}
    for source in {item["source_path"] for item in rows}:
        for item in json_lines(ROOT / source):
            definitions[(source, str(item.get("id", item.get("task_id", ""))))] = (
                item.get("categories", [])
            )
    for item in rows:
        categories = definitions.get((item["source_path"], item["task_id"]), [])
        item["upstream_categories"] = categories
        category = next((name for name in categories if name in CVDP_TASK_KINDS), None)
        if category:
            item["task_kind"] = CVDP_TASK_KINDS[category]
        elif "comprehension" in item["variant"]:
            item["task_kind"] = "code_comprehension"
    return rows


def cvdp() -> list[dict]:
    data_roots = [UPSTREAM / "cvdp/data", UPSTREAM / "hf-cvdp"]
    full = generic_jsonl_suite(
        "cvdp",
        data_roots,
        "rtl_generation",
        "CVDP per-row harness/container protocol.",
        include=lambda path: path.name.startswith("cvdp_v1.1.0_"),
    )
    if full:
        return cvdp_categories(full)
    rows = generic_jsonl_suite(
        "cvdp",
        [UPSTREAM / "cvdp/example_dataset"],
        "rtl_generation",
        "CVDP example harness only; this is not the full dataset.",
    )
    for item in rows:
        item["variant"] = "example-only:" + item["variant"]
        if "comprehension" in item["variant"]:
            item["task_kind"] = "issue_detection"
        item["reason"] = (
            "Only a framework example question is present; the full Hugging Face payload is absent."
        )
    return rows


def realbench() -> list[dict]:
    path = UPSTREAM / "realbench/benchmark_info.py"
    tree = ast.parse(path.read_text())
    values = {}
    for statement in tree.body:
        if isinstance(statement, ast.Assign) and isinstance(
            statement.targets[0], ast.Name
        ):
            if statement.targets[0].id in {"benchmark_info", "system_info"}:
                values[statement.targets[0].id] = ast.literal_eval(statement.value)
    rows = []
    for family, tasks in values["benchmark_info"].items():
        for task in tasks:
            description = UPSTREAM / "realbench" / family / task / f"{task}.md.gpg"
            family_category = {
                "sdc": "protocol",
                "aes": "arithmetic",
                "e203_hbirdv2": "complex_ip",
            }[family]
            rows.append(
                row(
                    "realbench",
                    task,
                    "module",
                    "rtl_generation",
                    path,
                    "RealBench encrypted-description module simulation/formal protocol.",
                    description=description,
                    prompt=task,
                    metadata=family_category,
                    porting_status="requires_original_protocol",
                    reason="Description is encrypted and multi-file project dependencies require the original protocol.",
                )
            )
    for task in values["system_info"]:
        family = "sdc" if "sdc" in task else "aes" if "aes" in task else "e203_hbirdv2"
        description = UPSTREAM / "realbench" / family / task / f"{task}.md.gpg"
        family_category = {
            "sdc": "protocol",
            "aes": "arithmetic",
            "e203_hbirdv2": "complex_ip",
        }[family]
        rows.append(
            row(
                "realbench",
                task,
                "system",
                "rtl_generation",
                path,
                "RealBench encrypted-description system simulation/formal protocol.",
                description=description,
                prompt=task,
                metadata=family_category,
                porting_status="requires_original_protocol",
                reason="Description is encrypted and system-level multi-file IP requires the original protocol.",
            )
        )
    return rows


def chipverilog() -> list[dict]:
    rows = []
    for description in sorted((UPSTREAM / "chipverilog/Src").rglob("description.txt")):
        task_id = description.parent.name
        rows.append(
            row(
                "chipverilog",
                task_id,
                "description",
                "rtl_generation",
                description,
                "ChipVerilog functional checking and formal-equivalence protocol.",
                description=description,
                prompt=description.read_text(errors="replace"),
                porting_status="requires_original_protocol",
                reason="Reference RTL and equivalence protocol must remain hidden/original.",
            )
        )
    return rows


def resbench() -> list[dict]:
    path = UPSTREAM / "resbench/problems.json"
    data = json.loads(path.read_text())
    rows = []
    for group, tasks in data.items():
        for item in tasks:
            rows.append(
                row(
                    "resbench",
                    item["module"],
                    group.lower().replace(" ", "-"),
                    "resource_optimization",
                    path,
                    "Original functional testbench followed by FPGA resource analysis.",
                    prompt_field="Problem",
                    prompt=item.get("Problem", ""),
                    metadata=group,
                    porting_status="requires_original_protocol",
                    reason="Functional correctness and FPGA resource metrics use the original testbench/tool flow.",
                )
            )
    return rows


def hsco() -> list[dict]:
    rows = []
    for case in sorted((UPSTREAM / "hsco-bench/hsco_testcases").glob("*_testcase")):
        description = case / "README.md"
        task_id = case.name.removesuffix("_vc707_clear_testcase")
        rows.append(
            row(
                "hsco-bench",
                task_id,
                "vc707",
                "hw_sw_codesign",
                description,
                "HSCo ESP/Catapult/FPGA campaign protocol.",
                description=description,
                prompt=description.read_text(errors="replace"),
                metadata="complex_ip accelerator",
                porting_status="requires_original_protocol",
                reason="Task spans C/C++, HLS, RTL, drivers, and FPGA build infrastructure.",
            )
        )
    return rows


def cirfix() -> list[dict]:
    rows = []
    base = UPSTREAM / "cirfix/benchmarks"
    for buggy in sorted(base.rglob("*buggy*.v")):
        rel = buggy.relative_to(base)
        task_id = (
            rel.parent.as_posix()
            + "/"
            + re.sub(r"_(?:wadden|kgoliya|ssscrazy|super)?_?buggy.*$", "", buggy.stem)
        )
        rows.append(
            row(
                "cirfix",
                task_id,
                buggy.stem,
                "repair",
                buggy,
                "CirFix compile/simulation oracle protocol.",
                description=buggy,
                prompt=buggy.stem,
                metadata=rel.as_posix(),
                porting_status="requires_original_protocol",
                reason="Repair task requires the buggy RTL, original testbench, and oracle files.",
            )
        )
    return rows


def hwe_bench() -> list[dict]:
    roots = [UPSTREAM / "hwe-bench/datasets", UPSTREAM / "hf-hwe-bench"]
    files = []
    for root in roots:
        if not root.is_dir():
            continue
        full = root / "hwe_bench_full.jsonl"
        if full.is_file() and json_lines(full):
            files.append(full)
        else:
            files.extend(
                path
                for path in sorted(root.glob("*.jsonl"))
                if path.name != "hwe_bench_full.jsonl"
            )
    rows, seen = [], set()
    for path in files:
        for item in json_lines(path):
            task_id = f"{item['org']}/{item['repo']}#{item['number']}"
            if task_id in seen:
                continue
            seen.add(task_id)
            prompt = (
                item.get("problem_statement")
                or item.get("title")
                or item.get("body", "")
            )
            rows.append(
                row(
                    "hwe-bench",
                    task_id,
                    path.stem,
                    "repair",
                    path,
                    "HWE-Bench fail-to-pass container protocol.",
                    prompt_field="problem_statement",
                    prompt=prompt,
                    metadata=json.dumps(item.get("labels", []), sort_keys=True),
                    porting_status="requires_original_protocol",
                    reason="Repair requires the pinned repository image and fail-to-pass tests.",
                )
            )
    return rows


def rtl_repo() -> list[dict]:
    return generic_jsonl_suite(
        "rtl-repo",
        [UPSTREAM / "rtl-repo/data", UPSTREAM / "hf-rtl-repo"],
        "completion",
        "RTL-Repo repository-context completion protocol.",
    )


def notsotiny() -> list[dict]:
    return generic_jsonl_suite(
        "notsotiny-framework",
        [UPSTREAM / "notsotiny-framework/data", UPSTREAM / "hf-notsotiny"],
        "completion",
        "NotSoTiny/TuRTLe module-completion protocol.",
        include=lambda path: path.name == "NotSoTiny-25-12-cvdp.jsonl",
    )


def hwfix() -> list[dict]:
    path = UPSTREAM / "hf-hwfix/issue_benchmark.json"
    if not path.is_file() or is_lfs_pointer(path):
        return []
    payload = json.loads(path.read_text())
    if isinstance(payload, dict):
        items = next(
            (
                value
                for key, value in payload.items()
                if key in {"tasks", "data", "examples"} and isinstance(value, list)
            ),
            [
                (
                    dict(value, id=key)
                    if isinstance(value, dict)
                    else {"id": key, "input": value}
                )
                for key, value in payload.items()
            ],
        )
    else:
        items = payload
    rows = []
    for index, item in enumerate(items):
        if item.get("repo") and item.get("issue_number") is not None:
            task_id = f"{item['repo']}#{item['issue_number']}"
        else:
            task_id = next(
                (
                    item.get(key)
                    for key in ("task_id", "id", "issue_id", "name")
                    if item.get(key) is not None
                ),
                f"row-{index:06d}",
            )
        # The downloaded description summarizes the fix; files.modified/patch
        # are reference answers. Authoring context is the original issue body
        # (or title) plus files.base, never those answer-bearing fields.
        prompt_field = "body" if item.get("body") else "title"
        prompt = item.get(prompt_field, "")
        if isinstance(prompt, dict):
            prompt = prompt.get("prompt", json.dumps(prompt, sort_keys=True))
        rows.append(
            row(
                "hwfix",
                str(task_id),
                "issue-benchmark",
                "issue_analysis",
                path,
                "Original issue/code corpus; executable scoring protocol not established.",
                prompt_field=prompt_field,
                prompt=str(prompt),
                metadata=json.dumps(
                    item.get("category", item.get("labels", "")), sort_keys=True
                ),
                porting_status="requires_original_protocol",
                reason="Use original issue body/title and files.base; description/modified/patch are reference material. No verified executable repair oracle.",
            )
        )
    return rows


EXTRACTORS = (
    verilogeval_2023,
    verilogeval_v2,
    cvdp,
    hwe_bench,
    realbench,
    chipverilog,
    resbench,
    rtl_repo,
    notsotiny,
    hsco,
    cirfix,
    hwfix,
)


def rebuild() -> tuple[list[dict], dict]:
    rows = [item for extractor in EXTRACTORS for item in extractor()]
    rows.sort(
        key=lambda item: (
            item["suite"],
            item["task_id"],
            item["variant"],
            item["source_path"],
        )
    )
    identities = [(item["suite"], item["task_id"], item["variant"]) for item in rows]
    if len(identities) != len(set(identities)):
        duplicates = [key for key, count in Counter(identities).items() if count > 1]
        raise ValueError(f"duplicate benchmark identities: {duplicates[:10]}")
    missing_prompts = [
        (item["suite"], item["task_id"], item["variant"])
        for item in rows
        if "description_path" not in item and "prompt_field" not in item
    ]
    if missing_prompts:
        raise ValueError(
            f"benchmark rows lack a question pointer: {missing_prompts[:10]}"
        )
    catalog_text = "".join(json.dumps(item, sort_keys=True) + "\n" for item in rows)
    CATALOG.write_text(catalog_text)
    by_suite = Counter(item["suite"] for item in rows)
    by_kind = Counter(item["task_kind"] for item in rows)
    by_category = Counter(item["category"] for item in rows)
    by_suite_variant = Counter((item["suite"], item["variant"]) for item in rows)
    sources = []
    for suite in EXPECTED_SUITES:
        count = by_suite[suite]
        status = "available" if count else "missing_question_payload"
        if (
            suite == "cvdp"
            and count
            and all(
                item["variant"].startswith("example-only:")
                for item in rows
                if item["suite"] == suite
            )
        ):
            status = "example_only_not_full_dataset"
        if suite == "rtl-repo" and not count:
            parquet = sorted((UPSTREAM / "hf-rtl-repo/data").glob("*.parquet"))
            if any(path.is_file() and not is_lfs_pointer(path) for path in parquet):
                status = "payload_present_parquet_reader_unavailable"
        if suite == "hwfix" and not count:
            payload = UPSTREAM / "hf-hwfix/issue_benchmark.json"
            if is_lfs_pointer(payload):
                status = "lfs_payload_not_materialized"
        sources.append({"suite": suite, "status": status, "catalog_rows": count})
    report = {
        "schema": 1,
        "catalog": relative(CATALOG),
        "catalog_sha256": hashlib.sha256(catalog_text.encode()).hexdigest(),
        "total_rows": len(rows),
        "unique_task_ids": len({(item["suite"], item["task_id"]) for item in rows}),
        "counts_by_suite": dict(sorted(by_suite.items())),
        "counts_by_suite_variant": {
            f"{suite}:{variant}": count
            for (suite, variant), count in sorted(by_suite_variant.items())
        },
        "counts_by_task_kind": dict(sorted(by_kind.items())),
        "counts_by_category": dict(sorted(by_category.items())),
        "sources": sources,
    }
    lock = BENCHMARK / "sources.lock.json"
    if lock.is_file():
        report["sources_lock"] = relative(lock)
        report["sources_lock_sha256"] = digest(lock)
    INVENTORY.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    return rows, report


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=("rebuild",), nargs="?", default="rebuild")
    args = parser.parse_args()
    if args.command == "rebuild":
        rows, report = rebuild()
        print(f"wrote {len(rows)} rows to {CATALOG}")  # noqa: T201 - CLI status
        print(  # noqa: T201 - CLI status
            json.dumps(report["counts_by_suite"], sort_keys=True)
        )


if __name__ == "__main__":
    main()

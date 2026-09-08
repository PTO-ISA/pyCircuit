from __future__ import annotations

import argparse
import ast
import hashlib
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

BASE_REVISION = "ed9e4226eafcf1e5b46245fe1a6f7c373049d14d"
PYTHON_SNIPPET = """
import agentic_circuit as ac
from agentic_circuit._jit import _lower_acir_to_cpp
from designs.davincioo.spe.iex.i2 import i2_system
raw = ac.jit(i2_system, workspace='.').lower_acir()
print(raw if MODE == 'raw' else _lower_acir_to_cpp(raw), end='')
"""
PIPELINE = (
    "builtin.module(ac-lower-value-contracts,ac-lower-variable-state,"
    "ac-lower-rules,canonicalize,cse,ac-verify-rule-closure,ac-freeze-topology)"
)


def source_metrics(source: str) -> dict[str, int]:
    tree = ast.parse(source)
    rules = [
        node
        for node in ast.walk(tree)
        if isinstance(node, ast.FunctionDef)
        and any(
            isinstance(decorator, ast.Attribute) and decorator.attr == "rule"
            for decorator in node.decorator_list
        )
    ]
    names = {rule.name for rule in rules}
    return {
        "lines": len(source.splitlines()),
        "rules": len(rules),
        "rule_parameters": sum(len(rule.args.args) for rule in rules),
        "rule_call_arguments": sum(
            len(node.args)
            for node in ast.walk(tree)
            if isinstance(node, ast.Call)
            and isinstance(node.func, ast.Name)
            and node.func.id in names
        ),
    }


def generate(root: Path, tool_root: Path, mode: str) -> str:
    environment = os.environ.copy()
    environment.update(
        {
            "ACIR_OPT": str(tool_root / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"),
            "ACIR_QUEUE_CXXGEN": str(
                tool_root / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-cxxgen"
            ),
            "PYTHONPATH": ":".join(
                (
                    str(root),
                    str(root / "python/agentic-circuit/src"),
                    str(root / "python/semantic-core/src"),
                )
            ),
        }
    )
    snippet = f"MODE = {mode!r}\n" + PYTHON_SNIPPET
    return subprocess.check_output(
        [sys.executable, "-c", snippet], cwd=root, env=environment, text=True
    )


def plan_metrics(plan: dict[str, object]) -> dict[str, int]:
    specialization = plan["module_specializations"][0]
    blocks = specialization["blocks"]
    return {
        "blocks": len(blocks),
        "queues": len(specialization["queues"]),
        "tables": len(specialization["tables"]),
        "state_writes": sum(len(block.get("state_writes", ())) for block in blocks),
        "state_reservations": sum(
            len(block.get("state_reservations", ())) for block in blocks
        ),
        "outputs": sum(len(block.get("outputs", ())) for block in blocks),
        "expressions": sum(len(block.get("expressions", ())) for block in blocks),
    }


def normalized_plan_hash(text: str) -> str:
    text = text.replace("__ac_nested_i2_", "")
    text = re.sub(r"sha256:[0-9a-f]{64}", "sha256:<fingerprint>", text)
    return hashlib.sha256(text.encode()).hexdigest()


def normalized_cpp_metrics(text: str) -> dict[str, int | str]:
    normalized = re.sub(r"I2_[0-9a-f]{64}", "I2_<fingerprint>", text)
    return {
        "bytes": len(text.encode()),
        "normalized_sha256": hashlib.sha256(normalized.encode()).hexdigest(),
        "table_scan_loops": text.count("for (std::size_t index = 0; index < table"),
        "priority_encode_calls": text.count("gfsim::priorityEncode("),
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--base-root", type=Path, default=Path(".pycircuit_out/issue63-opt02-base")
    )
    arguments = parser.parse_args()
    root = Path.cwd().resolve()
    base_root = arguments.base_root.resolve()
    if not base_root.is_dir():
        raise SystemExit(
            f"missing base worktree {base_root}; create it at {BASE_REVISION}"
        )
    sources = {
        "before": (base_root / "designs/davincioo/spe/iex/i2.py").read_text(),
        "after": (root / "designs/davincioo/spe/iex/i2.py").read_text(),
    }
    raw = {
        "before": generate(base_root, root, "raw"),
        "after": generate(root, root, "raw"),
    }
    cpp = {
        "before": generate(base_root, root, "cpp"),
        "after": generate(root, root, "cpp"),
    }
    with tempfile.TemporaryDirectory(prefix="pyc-opt02-equivalence-") as directory:
        temporary = Path(directory)
        plans: dict[str, str] = {}
        optimizer = root / ".pycircuit_out/acir/dev-llvm22/bin/acir-opt"
        planner = root / ".pycircuit_out/acir/dev-llvm22/bin/acir-queue-plan"
        for name, text in raw.items():
            raw_path = temporary / f"{name}.raw.mlir"
            frozen_path = temporary / f"{name}.frozen.mlir"
            raw_path.write_text(text)
            subprocess.run(
                [
                    str(optimizer),
                    f"--pass-pipeline={PIPELINE}",
                    str(raw_path),
                    "-o",
                    str(frozen_path),
                ],
                check=True,
            )
            plans[name] = subprocess.check_output(
                [str(planner), str(frozen_path)], text=True
            )
    raw_needles = {
        "rules": "ac.rule ",
        "state_declarations": "ac.var.decl ",
        "state_reads": "ac.var.read ",
        "state_assignments": "ac.var.assign ",
        "state_element_assignments": "ac.var.assign_element ",
        "instances": "ac.instance ",
        "sources": "ac.source ",
        "sinks": "ac.sink ",
    }
    result = {
        "base_revision": BASE_REVISION,
        "source": {name: source_metrics(text) for name, text in sources.items()},
        "raw_acir": {
            name: {
                "bytes": len(text.encode()),
                **{key: text.count(needle) for key, needle in raw_needles.items()},
            }
            for name, text in raw.items()
        },
        "queuegraph": {
            name: plan_metrics(json.loads(text)) for name, text in plans.items()
        }
        | {
            "normalized_sha256": {
                name: normalized_plan_hash(text) for name, text in plans.items()
            }
        },
        "generated_cpp": {
            name: normalized_cpp_metrics(text) for name, text in cpp.items()
        },
    }
    sys.stdout.write(json.dumps(result, indent=2, sort_keys=True) + "\n")


if __name__ == "__main__":
    main()

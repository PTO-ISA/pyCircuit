#!/usr/bin/env python3
from __future__ import annotations

import argparse
import ast
import json
import sys
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class ExampleCase:
    name: str
    design: Path
    tb: Path
    config: Path
    tier: str


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _parse_sim_tier(cfg_path: Path) -> str:
    text = cfg_path.read_text(encoding="utf-8")
    tree = ast.parse(text, filename=str(cfg_path))
    tier = None
    for node in tree.body:
        if not isinstance(node, ast.Assign):
            continue
        for tgt in node.targets:
            if isinstance(tgt, ast.Name) and tgt.id == "SIM_TIER":
                if isinstance(node.value, ast.Constant) and isinstance(
                    node.value.value, str
                ):
                    tier = node.value.value
    if tier not in {"normal", "heavy"}:
        raise RuntimeError(f'{cfg_path}: SIM_TIER must be "normal" or "heavy"')
    return str(tier)


def _parse_pyc_name(design_path: Path) -> str | None:
    text = design_path.read_text(encoding="utf-8")
    tree = ast.parse(text, filename=str(design_path))
    for node in tree.body:
        if not isinstance(node, ast.Assign):
            continue
        if not isinstance(node.value, ast.Constant) or not isinstance(
            node.value.value, str
        ):
            continue
        for tgt in node.targets:
            if (
                isinstance(tgt, ast.Attribute)
                and isinstance(tgt.value, ast.Name)
                and tgt.value.id == "build"
                and tgt.attr == "__pycircuit_name__"
            ):
                return str(node.value.value)
    return None


def _looks_like_design(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    return "@module" in text and "def build(" in text


def _discover(root: Path) -> list[ExampleCase]:
    if not root.exists():
        raise RuntimeError(f"examples root not found: {root}")

    errs: list[str] = []
    cases: list[ExampleCase] = []
    names: set[str] = set()

    for d in sorted(p for p in root.rglob("*") if p.is_dir()):
        if d.name == "__pycache__":
            continue
        name = d.name
        design = d / f"{name}.py"
        tb = d / f"tb_{name}.py"
        cfg = d / f"{name}_config.py"

        present = [design.exists(), tb.exists(), cfg.exists()]
        # Cycle-aware examples use their own compile_cycle_aware entrypoint and
        # are not consumable by the @module-based folderized-example runner.
        if design.exists() and not _looks_like_design(design):
            continue
        if any(present) and not all(present):
            errs.append(
                f"{d}: malformed example folder (requires {name}.py, tb_{name}.py, {name}_config.py)"
            )
            continue
        if not all(present):
            continue

        if name in names:
            errs.append(f"{d}: duplicate example name {name!r}")
            continue
        names.add(name)

        pyc_name = _parse_pyc_name(design)
        if pyc_name != name:
            errs.append(
                f"{design}: build.__pycircuit_name__ must be {name!r}, got {pyc_name!r}"
            )
            continue

        try:
            tier = _parse_sim_tier(cfg)
        except Exception as e:
            errs.append(str(e))
            continue

        cases.append(
            ExampleCase(name=name, design=design, tb=tb, config=cfg, tier=tier)
        )

    # Enforce hard-break layout: every design module under examples/ must belong to a discovered case.
    case_designs = {c.design.resolve() for c in cases}
    for py in sorted(root.rglob("*.py")):
        if "__pycache__" in py.parts:
            continue
        if py.name == "__init__.py":
            continue
        if py.name.startswith("tb_") or py.name.endswith("_config.py"):
            continue
        if py.name.startswith("emulate_"):
            continue
        if not _looks_like_design(py):
            continue
        if py.resolve() not in case_designs:
            errs.append(f"{py}: design module is outside required folderized layout")

    if errs:
        raise RuntimeError("\n".join(errs))
    return sorted(cases, key=lambda c: c.name)


def _emit_json(cases: list[ExampleCase]) -> None:
    payload = [
        {
            "name": c.name,
            "design": str(c.design),
            "tb": str(c.tb),
            "config": str(c.config),
            "tier": c.tier,
        }
        for c in cases
    ]
    print(json.dumps(payload, indent=2, sort_keys=True))


def _emit_tsv(cases: list[ExampleCase]) -> None:
    for c in cases:
        print(f"{c.name}\t{c.design}\t{c.tb}\t{c.config}\t{c.tier}")


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(
        description="Discover and validate folderized pyCircuit examples."
    )
    ap.add_argument(
        "--root",
        default=str(_repo_root() / "examples" / "pycircuit"),
        help="Examples root directory",
    )
    ap.add_argument(
        "--tier",
        choices=["all", "normal", "heavy"],
        default="all",
        help="Filter by simulation tier",
    )
    ap.add_argument(
        "--format",
        choices=["json", "tsv"],
        default="json",
        help="Output format",
    )
    ns = ap.parse_args(argv)

    try:
        cases = _discover(Path(ns.root).resolve())
    except Exception as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    if ns.tier != "all":
        cases = [c for c in cases if c.tier == ns.tier]

    if ns.format == "tsv":
        _emit_tsv(cases)
    else:
        _emit_json(cases)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

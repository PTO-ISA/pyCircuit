#!/usr/bin/env python3
"""Check the current pyCircuit Python source and active route for stale APIs.

The previous frontend token registry belonged to the retired builder frontend.
This gate now parses the current public package and delegates retired-route
checks to the source compiler inventory checker. Positional paths remain accepted for
pre-commit and release workflow compatibility; documentation and history are
not treated as executable API surfaces.
"""

from __future__ import annotations

import argparse
import ast
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from check_frontend_retirement import scan_production  # noqa: E402

DEFAULT_TARGETS = ("python/pycircuit",)


def python_sources(path: Path):
    if path.is_file():
        return [path] if path.suffix == ".py" else []
    if not path.is_dir():
        return []
    return sorted(
        item
        for item in path.rglob("*.py")
        if not any(
            part in {"__pycache__", ".pycircuit_out", "build"} for part in item.parts
        )
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scan-root", type=Path, default=ROOT)
    parser.add_argument("targets", nargs="*", default=list(DEFAULT_TARGETS))
    args = parser.parse_args(argv)
    root = args.scan_root.resolve()

    failures = scan_production()
    for target in args.targets:
        candidate = Path(target)
        path = (
            candidate.resolve()
            if candidate.is_absolute()
            else (root / candidate).resolve()
        )
        for source in python_sources(path):
            try:
                ast.parse(source.read_text(encoding="utf-8"), filename=str(source))
            except (OSError, UnicodeDecodeError, SyntaxError) as error:
                failures.append(f"{source}: Python source does not parse: {error}")

    if failures:
        print("pyCircuit API hygiene failed:", file=sys.stderr)
        for item in failures:
            print(f"  {item}", file=sys.stderr)
        return 1
    print("ok: current pyCircuit API hygiene passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

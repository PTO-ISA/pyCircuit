#!/usr/bin/env python3
"""Convert pinned DavinciOO PTO JSONL into canonical `pto-trace@0.1`."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from pto_trace_adapter import AdapterError, publish_davincioo_trace


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(
        description="convert DavinciOO PTO JSONL to canonical pto-trace@0.1",
        usage="%(prog)s INPUT OUTPUT [--source-program ID]",
    )
    result.add_argument("input", metavar="INPUT", type=Path)
    result.add_argument("output", metavar="OUTPUT", type=Path)
    result.add_argument(
        "--source-program",
        metavar="ID",
        help="stable source-program identity (default: raw input SHA-256)",
    )
    return result


def main(arguments: list[str] | None = None) -> int:
    options = parser().parse_args(arguments)
    try:
        publish_davincioo_trace(
            options.input,
            options.output,
            source_program=options.source_program,
        )
    except AdapterError as error:
        print(error, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

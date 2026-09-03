#!/usr/bin/env python3
"""Pack committed gfsim event JSONL into canonical Perfetto trace JSON."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from perfetto_trace import PerfettoTraceError, publish_perfetto_trace


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(
        description="pack committed gfsim event JSONL for Perfetto",
        usage="%(prog)s INPUT OUTPUT",
    )
    result.add_argument("input", metavar="INPUT", type=Path)
    result.add_argument("output", metavar="OUTPUT", type=Path)
    return result


def main(arguments: list[str] | None = None) -> int:
    options = parser().parse_args(arguments)
    try:
        publish_perfetto_trace(options.input, options.output)
    except PerfettoTraceError as error:
        print(error, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

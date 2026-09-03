#!/usr/bin/env python3
"""Materialize an ACSim fixture for the active compiler target triple."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--compiler", required=True, type=Path)
    arguments = parser.parse_args()

    target = subprocess.run(
        (str(arguments.compiler), "-dumpmachine"),
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()
    canonical = json.dumps(target, ensure_ascii=False, separators=(",", ":"))
    fingerprint = "sha256:" + hashlib.sha256(canonical.encode()).hexdigest()

    source = arguments.input.read_text(encoding="utf-8")
    updated, replacements = re.subn(
        r'(toolchain\s*=\s*")sha256:[0-9a-f]{64}("\s*)',
        rf"\g<1>{fingerprint}\g<2>",
        source,
        count=1,
    )
    if replacements != 1:
        parser.error("input does not contain exactly one toolchain fingerprint")
    arguments.output.write_text(updated, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

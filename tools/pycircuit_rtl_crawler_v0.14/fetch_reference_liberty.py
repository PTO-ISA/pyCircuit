#!/usr/bin/env python3
from __future__ import annotations
import argparse
import hashlib
import urllib.request
from pathlib import Path

NANGATE45_URL = (
    "https://raw.githubusercontent.com/The-OpenROAD-Project/"
    "OpenROAD-flow-scripts/master/flow/platforms/nangate45/lib/"
    "NangateOpenCellLibrary_typical.lib"
)

def sha256(path: Path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()

def main():
    ap = argparse.ArgumentParser(description="Fetch a public reference Liberty.")
    ap.add_argument("--library", choices=["nangate45"], default="nangate45")
    ap.add_argument("--outdir", type=Path, default=Path("reference_libs/nangate45"))
    ap.add_argument("--force", action="store_true")
    args = ap.parse_args()

    args.outdir.mkdir(parents=True, exist_ok=True)
    out = args.outdir / "NangateOpenCellLibrary_typical.lib"

    if out.exists() and not args.force:
        print("exists :", out)
        print("sha256 :", sha256(out))
        return

    print("fetching:", NANGATE45_URL)
    with urllib.request.urlopen(NANGATE45_URL) as r:
        data = r.read()
    out.write_bytes(data)

    print("saved  :", out)
    print("bytes  :", len(data))
    print("sha256 :", sha256(out))

if __name__ == "__main__":
    main()

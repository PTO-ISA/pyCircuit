#!/usr/bin/env python3
from __future__ import annotations
import argparse
import re
import shutil
import subprocess
from pathlib import Path

ERR_RE = re.compile(r"^Error:\\s+(.+)$", re.MULTILINE)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("netlist", type=Path)
    ap.add_argument("--liberty", type=Path, required=True)
    ap.add_argument("--sta", default="")
    args = ap.parse_args()

    sta = args.sta or shutil.which("sta")
    if not sta:
        raise SystemExit("sta not found")

    net = args.netlist.resolve()
    lib = args.liberty.resolve()

    tcl = net.parent / "_parse_only.tcl"
    tcl.write_text(
        f"""read_liberty {{{lib}}}
read_verilog {{{net}}}
link_design pyc_synth_top
puts "PYC_NETLIST_PARSE_DONE"
exit
""",
        encoding="utf-8",
    )

    p = subprocess.run(
        [sta, str(tcl)],
        cwd=net.parent,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    text = (p.stdout or "") + "\\n" + (p.stderr or "")
    errors = ERR_RE.findall(text)

    if errors:
        print("NETLIST_PARSE_FAIL")
        for e in errors:
            print(" -", e)
        raise SystemExit(2)

    print("NETLIST_PARSE_PASS")
    raise SystemExit(0)

if __name__ == "__main__":
    main()

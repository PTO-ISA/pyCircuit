#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
import shutil
import subprocess
import tempfile
from pathlib import Path

def run(cmd, cwd=None):
    return subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

def main():
    ap = argparse.ArgumentParser(description="Preflight OpenSTA with a Liberty + tiny mapped netlist.")
    ap.add_argument("liberty", type=Path)
    ap.add_argument("--sta", default="")
    args = ap.parse_args()

    lib = args.liberty.expanduser().resolve()
    if not lib.exists():
        raise SystemExit(f"Liberty not found: {lib}")

    sta = args.sta or shutil.which("sta")
    if not sta:
        raise SystemExit("OpenSTA executable `sta` not found.")

    vp = run([sta, "-version"])
    version = (vp.stdout or vp.stderr or "").strip()

    with tempfile.TemporaryDirectory(prefix="pyc_sta_preflight_") as td:
        td = Path(td)
        net = td / "tiny_mapped.v"
        # This uses common Nangate45 cell names. It is intentionally tiny and
        # tests only read_liberty/read_verilog/link_design/report_checks.
        net.write_text(
            """module tiny_mapped(input clk_i, input a, output y);
  wire q;
  DFF_X1 u_ff (.D(a), .CK(clk_i), .Q(q), .QN());
  BUF_X1 u_buf (.A(q), .Z(y));
endmodule
""",
            encoding="utf-8",
        )

        tcl = td / "preflight.tcl"
        tcl.write_text(
            f"""read_liberty {{{lib}}}
read_verilog {{{net}}}
link_design tiny_mapped
create_clock -name clk -period 100.0 [get_ports clk_i]
set_input_delay -clock clk 0.0 [get_ports a]
set_output_delay -clock clk 0.0 [get_ports y]
report_checks -path_delay max
exit
""",
            encoding="utf-8",
        )
        p = run([sta, str(tcl)], cwd=td)
        text = (p.stdout or "") + "\n" + (p.stderr or "")
        ok = p.returncode == 0 and "Startpoint:" in text and "Endpoint:" in text

    report = {
        "sta": sta,
        "version": version,
        "liberty": str(lib),
        "basic_timing_flow": "PASS" if ok else "FAIL",
    }
    print(json.dumps(report, indent=2))

    if not ok:
        print("\n--- OpenSTA stdout ---")
        print((p.stdout or "")[-5000:])
        print("\n--- OpenSTA stderr ---")
        print((p.stderr or "")[-5000:])

    raise SystemExit(0 if ok else 2)

if __name__ == "__main__":
    main()

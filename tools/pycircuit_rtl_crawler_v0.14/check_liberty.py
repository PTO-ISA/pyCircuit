#!/usr/bin/env python3
from __future__ import annotations
import argparse
import json
import shutil
import subprocess
import tempfile
from pathlib import Path

def run(cmd):
    return subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def main():
    ap = argparse.ArgumentParser(description="Preflight a Liberty for pyCircuit technology mapping.")
    ap.add_argument("liberty", type=Path)
    args = ap.parse_args()

    lib = args.liberty.expanduser().resolve()
    if not lib.exists():
        raise SystemExit(f"FAIL: Liberty not found: {lib}")

    yosys = shutil.which("yosys")
    if not yosys:
        raise SystemExit("FAIL: yosys not found")

    # 1) Verify Yosys can parse the Liberty.
    p = run([yosys, "-Q", "-p", f"read_liberty -lib {lib}; stat"])
    parse_ok = p.returncode == 0

    # 2) Verify ABC mapping can actually consume it using a tiny combinational DUT.
    with tempfile.TemporaryDirectory(prefix="pyc_libcheck_") as td:
        td = Path(td)
        v = td / "tiny.v"
        v.write_text(
            "module tiny(input a,b,c, output y); assign y=(a&b)|c; endmodule\n",
            encoding="utf-8"
        )
        ys = td / "tiny.ys"
        ys.write_text(
            f"""read_verilog {v}
hierarchy -top tiny
synth -top tiny -flatten
abc -liberty {lib}
clean
stat -liberty {lib}
write_verilog -noattr {td/'tiny_mapped.v'}
""",
            encoding="utf-8"
        )
        q = run([yosys, "-s", str(ys)])
        abc_ok = q.returncode == 0 and (td / "tiny_mapped.v").exists()

    report = {
        "liberty": str(lib),
        "yosys": yosys,
        "read_liberty": "PASS" if parse_ok else "FAIL",
        "abc_mapping": "PASS" if abc_ok else "FAIL",
    }
    print(json.dumps(report, indent=2))

    if not parse_ok:
        print("\n--- read_liberty stderr ---")
        print(p.stderr[-3000:])
    if not abc_ok:
        print("\n--- abc mapping stderr ---")
        print(q.stderr[-3000:])
        print("\n--- abc mapping stdout tail ---")
        print(q.stdout[-3000:])

    raise SystemExit(0 if parse_ok and abc_ok else 2)

if __name__ == "__main__":
    main()

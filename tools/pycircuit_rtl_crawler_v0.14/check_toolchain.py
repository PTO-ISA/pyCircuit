#!/usr/bin/env python3
from __future__ import annotations
import argparse
import os
import shutil
import subprocess

def check(cxx):
    p = subprocess.run(
        [cxx, "-std=c++20", "-fcoroutines", "-x", "c++", "-", "-fsyntax-only"],
        input="int main(){return 0;}\n",
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return p

def version(cxx):
    p = subprocess.run([cxx, "--version"], text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return (p.stdout or "").splitlines()[0] if p.stdout else ""

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cxx", default="")
    args = ap.parse_args()
    cxx = args.cxx or os.environ.get("CXX", "") or shutil.which("g++")
    if not cxx:
        raise SystemExit("FAIL: no C++ compiler found")

    print("CXX     :", cxx)
    print("version :", version(cxx))
    p = check(cxx)
    if p.returncode == 0:
        print("C++20 coroutine preflight: PASS")
        raise SystemExit(0)

    print("C++20 coroutine preflight: FAIL")
    print((p.stderr or p.stdout).strip())
    print()
    print("Look for an installed GCC >= 10 with:")
    print("  ls -1 /usr/bin/g++-* 2>/dev/null")
    print("Then test one explicitly, e.g.:")
    print("  python check_toolchain.py --cxx /usr/bin/g++-11")
    raise SystemExit(2)

if __name__ == "__main__":
    main()

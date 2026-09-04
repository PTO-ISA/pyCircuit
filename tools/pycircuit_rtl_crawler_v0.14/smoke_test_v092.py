#!/usr/bin/env python3
from pathlib import Path
import importlib.util
import subprocess

def load(path):
    spec = importlib.util.spec_from_file_location("rtb", path)
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

def main():
    m = load("run_timing_benchmark.py")
    assert m._as_text(b"abc") == "abc"
    assert m._as_text("abc") == "abc"
    assert m._as_text(None) == ""

    # Force a timeout and confirm it is classified without bytes/str crash.
    r = m.run(
        ["python", "-c", "import time; print('hello'); time.sleep(2)"],
        timeout=1
    )
    assert r.returncode == 124
    assert isinstance(r.stdout, str)
    assert isinstance(r.stderr, str)
    assert "TIMEOUT after 1s" in r.stderr

    src = Path("run_timing_benchmark.py").read_text(encoding="utf-8")
    assert "default=1" in src
    assert "default=600" in src
    print("smoke_test_v0.9.2: PASS")

if __name__ == "__main__":
    main()

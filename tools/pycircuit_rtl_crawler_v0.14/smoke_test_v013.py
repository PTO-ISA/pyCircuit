#!/usr/bin/env python3
from pathlib import Path
import ast

def main():
    for f in [
        "build_runtime_catalog.py",
        "validate_runtime_catalog.py",
        "build_library_index.py",
    ]:
        p = Path(f)
        assert p.exists()
        ast.parse(p.read_text(encoding="utf-8"))

    src = Path("build_runtime_catalog.py").read_text(encoding="utf-8")
    assert "selection_complete" in src
    assert "data_width" in src
    assert "capacity" in src
    assert "configuration" in src
    assert "qor_formal_sanitized" in src

    print("smoke_test_v0.13: PASS")

if __name__ == "__main__":
    main()

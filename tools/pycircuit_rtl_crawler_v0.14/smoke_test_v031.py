#!/usr/bin/env python3
from build_candidate import parse_verilator_messages

def main():
    sample = """%Warning-WIDTHTRUNC: foo.sv:10:2: width problem
%Warning-UNUSEDPARAM: foo.sv:12:1: unused parameter
%Error-MODNOTFOUND: foo.sv:20:3: missing module
"""
    r = parse_verilator_messages(sample)
    assert r["warning_codes"] == {"UNUSEDPARAM": 1, "WIDTHTRUNC": 1}, r
    assert r["error_codes"] == {"MODNOTFOUND": 1}, r
    print("smoke_test_v0.3.1: PASS")

if __name__ == "__main__":
    main()

# PYC priority RTL vertical slice

Decision 0161 admits the vendor-neutral `pyc.priority_encode.v1` semantic
primitive and one BSD-3-Clause implementation candidate.

## Source qualification

The selected source is `library/verilog/pyc_priority_encode.v`, SHA-256
`54d01766a023f77e8a91eef527fba4e9b997e58c241ca63c11c0ec7e865045fe`.
It is repository-owned BSD-3-Clause code.  The Solderpad-licensed BaseJump
sources found in PR #29 were evaluated but not imported.

## Commands

```text
verilator --lint-only --timing -Wall -Wno-fatal --top-module primitive_priority_encode_tb library/verilog/pyc_priority_encode.v tests/verilog/primitive_priority_encode_tb.sv
iverilog -g2012 -s primitive_priority_encode_tb -o .pycircuit_out/primitive-tests/priority_encode.vvp library/verilog/pyc_priority_encode.v tests/verilog/primitive_priority_encode_tb.sv
vvp .pycircuit_out/primitive-tests/priority_encode.vvp
```

## Result

- Verilator lint: passed.
- Icarus elaboration and execution: passed.
- Widths: 1, 4, 8, and 13.
- Orders: low-first and high-first.
- Zero, one-hot, and multi-hot cases: passed.

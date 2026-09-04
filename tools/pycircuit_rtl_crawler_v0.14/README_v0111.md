# pyCircuit v0.11.1 — FIFO Dependency Closure Fixes

This patch fixes two dependency patterns exposed by the second Design Class.

## PULP: direct package scope references

`cc_fifo.sv` uses:

```systemverilog
cc_pkg::cnt_width(Depth)
cc_pkg::idx_width(Depth)
```

without requiring an explicit `import cc_pkg::*;`.

The crawler now recognizes both:

```text
import foo_pkg::*;
foo_pkg::symbol
```

as package dependencies.

## OpenTitan: static parser versus configured generate branches

The FIFO benchmark fixes:

```text
Secure = 0
```

but source-level scanning still sees `prim_count` / `prim_flop` in
Secure-only generate branches.

v0.11.1 allows candidate-specific `prune_modules` during static closure.
This is not a bypass of validation:

```text
static closure prune
        ↓
canonical wrapper sets Secure=0
        ↓
Configured Verilator Build Gate
        ↓
if dependency is actually reachable → build FAIL
```

The parser also filters SystemVerilog end-keywords such as `endfunction`,
which previously could be misidentified as submodule names.

## Rerun

```bash
python run_fifo_benchmark.py \
  --profile smoke \
  --workdir ../pycircuit_rtl_crawler_v0.9.3/work \
  --cxx /usr/bin/g++-10
```

Expected direction:

```text
PULP       cc_pkg now included
BaseJump   remains PASS
OpenTitan  closure should move PARTIAL → COMPLETE,
           then Configured Build becomes the real test
```

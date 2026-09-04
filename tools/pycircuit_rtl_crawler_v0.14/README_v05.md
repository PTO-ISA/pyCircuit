# pyCircuit Yosys Synthesis / QoR Harness v0.5

v0.5 starts the quantitative design-comparison stage.

## What v0.5 measures

### Generic / technology-independent QoR

- Yosys synthesized cell count
- wires / wire bits
- cell-type histogram
- longest topological path (`ltp -noff`) as a logic-depth proxy

### Optional technology-mapped area

If a Liberty file is supplied:

- `dfflibmap -liberty`
- `abc -liberty`
- `stat -liberty`
- mapped standard-cell area when exposed by the Yosys JSON report

## What v0.5 does NOT call full PPA

- `logic_depth` is not Fmax
- Liberty mapped area is pre-place-and-route
- power is not measured

A later timing/power stage should use a fixed Liberty/PDK + timing constraints +
OpenSTA/OpenROAD (or equivalent).

## Gate hierarchy so far

```text
Discovery
  ↓
Dependency Closure
  ↓
Verilator Compile
  ↓
Combinational Correctness
  ↓
Stateful Correctness
  ↓
Yosys Synthesis/QoR  ← v0.5
```

## Local setup

Reuse the existing pyCircuit root `.venv`.

Copy the already-qualified repository/candidates:

```bash
cp -a ../pycircuit_rtl_crawler_v0.4.1/work ./
cp -a ../pycircuit_rtl_crawler_v0.3.1/candidates ./
```

Check Yosys:

```bash
yosys -V
```

Because these PULP candidates use real SystemVerilog constructs (packages/types),
a recent Yosys build is strongly preferred. If an old distro Yosys fails in
`read_verilog -sv`, use a current Yosys/OSS CAD Suite before modifying the RTL.

## Smoke test

```bash
python smoke_test_v05.py
```

Expected:

```text
smoke_test_v0.5: PASS
```

## First synthesis smoke

```bash
python run_synthesis.py cc_lzc --profile smoke
python run_synthesis.py cc_popcount --profile smoke
python run_synthesis.py cc_rr_arb_tree --profile smoke
```

Then standard:

```bash
python run_synthesis.py cc_lzc --profile standard
python run_synthesis.py cc_popcount --profile standard
python run_synthesis.py cc_rr_arb_tree --profile standard
```

Inspect:

```bash
python inspect_synthesis.py cc_lzc --profile standard
python inspect_synthesis.py cc_popcount --profile standard
python inspect_synthesis.py cc_rr_arb_tree --profile standard
```

## Scaling experiment

```bash
python run_synthesis.py cc_lzc --profile scaling
python run_synthesis.py cc_popcount --profile scaling
python run_synthesis.py cc_rr_arb_tree --profile scaling
```

This is the first direct input to the later ranking/score-card layer.

## Optional Liberty area

```bash
python run_synthesis.py cc_popcount \
  --profile standard \
  --liberty /path/to/your_cells.lib
```

Important: compare implementations only when tool version, synthesis flow,
parameter configuration, and Liberty library are identical.

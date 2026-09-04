# pyCircuit v0.14 — Third Design Class + 12-Family Expansion Roadmap

## Third Design Class: INT-11 Popcount

The third validated class targets a pure combinational reduction datapath:

```text
Population Count / Hamming Weight
```

Candidates:

```text
PULP      cc_popcount
BaseJump  bsg_popcount
Vortex    VX_popcount
```

Canonical interface:

```text
data_i[WIDTH-1:0]
        ↓
    POPCOUNT
        ↓
count_o[ceil(log2(WIDTH+1))-1:0]
```

Scaling:

```text
WIDTH = 8 / 16 / 32 / 64
```

This class complements the two existing stateful classes:

```text
DF-09      arbitration/control state
FIFO-SYNC  queue/storage state + datapath
INT-11     pure combinational reduction datapath
```

## Vortex synthesis-path correctness

`VX_popcount` contains a simulation-only `$countones` shortcut when
`SYNTHESIS` is not defined.

The benchmark therefore records:

```yaml
defines:
  - SYNTHESIS
```

and applies candidate-specific defines consistently to:

```text
Configured Verilator Build
Correctness simulation
Yosys/Slang synthesis
```

This ensures the correctness test exercises the synthesizable Vortex
microarchitecture rather than its simulation shortcut.

## Combinational Timing Contract

INT-11 has no DUT clock.

The generic timing backend now supports a virtual clock:

```text
virtual clock vclk
input delay  = 0
output delay = 0
```

so OpenSTA can compare input-to-output mapped-cell critical delay without
inventing a physical clock pin.

## First run

Reuse the existing work directory. PULP and BaseJump are already present;
Vortex will be shallow-cloned there when first requested.

```bash
python smoke_test_v014.py

python run_popcount_benchmark.py \
  --profile smoke \
  --workdir ../pycircuit_rtl_crawler_v0.9.3/work \
  --cxx /usr/bin/g++-10
```

Expected shape:

```text
pulp_common_cells/cc_popcount
  w8 | closure PASS | build PASS | corr PASS | synth PASS

basejump_stl/bsg_popcount
  w8 | closure PASS | build PASS | corr PASS | synth PASS

vortex/VX_popcount
  w8 | closure PASS | build PASS | corr PASS | synth PASS
```

## Scaling + Nangate45

After smoke:

```bash
python run_popcount_benchmark.py \
  --profile scaling \
  --workdir ../pycircuit_rtl_crawler_v0.9.3/work \
  --cxx /usr/bin/g++-10 \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib
```

Then:

```bash
python run_popcount_timing.py \
  --profile scaling \
  --results-root design_class_results \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --workers 2 \
  --timeout-sec 30
```

Then build catalog:

```bash
python build_runtime_catalog.py \
  --class-id INT-11 \
  --profile scaling \
  --results-root design_class_results \
  --liberty ../pycircuit_rtl_crawler_v0.9.3/reference_libs/nangate45/NangateOpenCellLibrary_typical.lib

python validate_runtime_catalog.py \
  runtime_design_library/INT-11/scaling/runtime_catalog.json \
  --require-complete-selection
```

## Long-term expansion

`design_family_roadmap.yaml` now tracks all 12 Design Families.

Current validated / active trajectory:

```text
DF-09      Round-Robin Arbiter     VALIDATED
FIFO-SYNC  Synchronous FIFO        VALIDATED
INT-11     Popcount                IN PROGRESS
                       ↓
then expand representative Design Classes
across all 12 Design Families
                       ↓
pyCircuit Runtime Hardware Design Library
```

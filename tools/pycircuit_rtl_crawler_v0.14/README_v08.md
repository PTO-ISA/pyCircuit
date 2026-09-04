# pyCircuit v0.8 — Technology-Aware Area Benchmark

## Goal

Upgrade the current DF-09 metrics from:

```text
generic cell count
logic-depth proxy
```

to the first technology-aware metric:

```text
mapped standard-cell area
```

All implementations are mapped with the same Liberty library.

## Reference library

For methodology bring-up, v0.8 includes a helper that downloads the public
Nangate45 typical Liberty distributed in the OpenROAD Flow Scripts repository:

```bash
python fetch_reference_liberty.py
```

Output:

```text
reference_libs/nangate45/NangateOpenCellLibrary_typical.lib
```

This library is used only as a common open reference technology for fair
comparison. It is not meant to model the user's final target process.

## Preflight

```bash
python check_liberty.py \
  reference_libs/nangate45/NangateOpenCellLibrary_typical.lib
```

This checks both:

```text
Yosys read_liberty
ABC technology mapping
```

before launching the full benchmark.

## Run DF-09 scaling with mapped area

```bash
python run_technology_benchmark.py \
  --class-id DF-09 \
  --profile scaling \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --cxx /usr/bin/g++-10
```

The existing design-class runner performs:

```text
canonical adapter
        ↓
correctness
        ↓
generic synthesis
        ↓
dfflibmap
        ↓
abc -liberty
        ↓
stat -liberty
        ↓
mapped area
```

## Output

```text
design_class_results/DF-09/scaling/
├── comparison.csv
├── comparison.html
├── ...
└── mapped_area_analysis/
    ├── mapped_area_summary.csv
    ├── mapped_area_summary.json
    └── mapped_area.html
```

## Metric semantics

`mapped_area` is:

```text
sum of standard-cell areas from the Liberty
after Yosys/ABC mapping
```

It is not:

```text
placed-and-routed die/core area
```

because placement utilization, buffering, clock tree, routing and physical
design have not yet been included.

## Next milestone

After the same-library mapped-area flow is stable:

```text
mapped netlist
     ↓
same Liberty
     ↓
same SDC
     ↓
OpenSTA
     ↓
critical-path delay / slack
     ↓
technology-aware Area × Timing Pareto
```

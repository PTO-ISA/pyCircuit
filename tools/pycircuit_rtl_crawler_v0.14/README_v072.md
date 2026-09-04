# pyCircuit v0.7.2 — DF-09 Scaling / Pareto Analyzer

The DF-09 scaling benchmark now has 12 passing design points:

```text
3 implementations × N={2,4,8,16}
```

v0.7.2 adds automatic post-processing rather than relying on manual reading
of `comparison.csv`.

## New command

After running:

```bash
python run_design_class.py \
  --class-id DF-09 \
  --profile scaling \
  --cxx /usr/bin/g++-10
```

analyze it with:

```bash
python analyze_design_class.py \
  design_class_results/DF-09/scaling/comparison.csv
```

Outputs:

```text
design_class_results/DF-09/scaling/analysis/
├── analysis.html
├── analysis.json
├── pareto_points.csv
└── scaling_summary.csv
```

## Important metric semantics

Current metrics are:

```text
generic cells
logic depth proxy
```

They are not:

```text
mapped ASIC area
Fmax
power
```

Also, the canonical adapter is part of the synthesized top. Therefore this
benchmark should be labeled:

```text
Canonical Integration QoR
```

This is useful for pyCircuit runtime selection because adapter/integration cost
is real cost, but it should not be mislabeled as raw native-core QoR.

## Current measured DF-09 observation

With the current measured dataset:

```text
N=2   Pareto: BaseJump
N=4   Pareto: BaseJump
N=8   Pareto: BaseJump
N=16  Pareto: BaseJump + OpenTitan
```

At N=16:

```text
BaseJump   197 cells, depth 9
OpenTitan  187 cells, depth 14
PULP       270 cells, depth 17
```

The first meaningful trade-off crossover therefore appears at N=16:
OpenTitan has fewer generic cells while BaseJump has substantially shallower
logic depth.

## Next technology-aware milestone

The next major stage should use one fixed Liberty library and fixed timing
constraints for all three candidates:

```text
canonical correctness
        ↓
same synthesis frontend
        ↓
same Liberty
        ↓
mapped area
        ↓
same clock/timing constraints
        ↓
critical-path delay / Fmax
        ↓
technology-aware Pareto ranking
```

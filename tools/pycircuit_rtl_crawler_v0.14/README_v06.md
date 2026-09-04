# pyCircuit RTL Crawler / Benchmark Framework v0.6

## Milestone

v0.6 changes the workflow from **single-candidate bring-up** to the first
**batch candidate orchestration** layer.

Previous versions already provided:

```text
Discovery
→ Structural Parsing
→ Recursive Dependency Closure
→ Verilator Compile/Lint Gate
→ Functional Correctness
→ Stateful Correctness
→ Slang + Yosys Synthesis/QoR
```

v0.6 adds:

```text
                         ┌─ cc_lzc
Design Target / Gap ID ──┼─ cc_popcount
                         └─ cc_rr_arb_tree
                               ↓
                      Batch Orchestrator
                               ↓
             Build / Correctness / Stateful / Synth
                               ↓
              pipeline_report.json / csv / html
```

## Key architectural rule

**Discovery support and benchmark support are different things.**

The crawler may discover 30+ RTL candidates. A candidate for which no
correctness/synthesis adapter has been implemented is reported as:

```text
UNSUPPORTED
```

not:

```text
FAIL
```

This prevents the framework from confusing "our benchmark harness does not
understand this design yet" with "the RTL design is wrong".

That distinction becomes essential when adding BaseJump, OpenTitan, Gemmini,
Vortex, NVDLA, ZipCPU, etc.

## New files

### `benchmark_registry.yaml`

Defines which discovered modules currently have:

- build/lint support
- correctness adapter
- stateful adapter
- synthesis adapter
- Gap ID / target ID mapping

### `run_pipeline.py`

Batch orchestrator.

Examples:

Run all currently benchmark-supported candidates:

```bash
python run_pipeline.py \
  --all-supported \
  --correctness-profile smoke \
  --stateful-profile smoke \
  --synthesis-profile smoke
```

Run one module:

```bash
python run_pipeline.py \
  --module cc_lzc \
  --synthesis-profile smoke
```

Run everything discovered for a Gap Matrix ID:

```bash
python run_pipeline.py \
  --gap-id DF-09 \
  --synthesis-profile smoke
```

Run crawler first, then execute the selected target:

```bash
python run_pipeline.py \
  --discover \
  --gap-id DF-09
```

Only build/lint + synthesis:

```bash
python run_pipeline.py \
  --module cc_rr_arb_tree \
  --stages build,synthesis
```

Technology-mapped synthesis:

```bash
python run_pipeline.py \
  --all-supported \
  --liberty /path/to/standard_cells.lib
```

## Outputs

```text
pipeline_results/
├── pipeline_report.json
├── pipeline_summary.csv
├── pipeline_report.html
└── logs/
```

The report provides one row per candidate and separates:

```text
PASS
FAIL
BLOCKED
UNSUPPORTED
NOT_RUN
```

## Recommended v0.6 bring-up

Reuse the already downloaded repository and current candidate folders from
v0.5.2:

```bash
cp -a ../pycircuit_rtl_crawler_v0.5.2/work ./
cp -a ../pycircuit_rtl_crawler_v0.5.2/candidates ./
```

Optionally reuse previous results:

```bash
cp -a ../pycircuit_rtl_crawler_v0.5.2/correctness_results ./ 2>/dev/null || true
cp -a ../pycircuit_rtl_crawler_v0.5.2/stateful_results ./ 2>/dev/null || true
cp -a ../pycircuit_rtl_crawler_v0.5.2/synthesis_results ./ 2>/dev/null || true
```

Then run:

```bash
python run_pipeline.py \
  --all-supported \
  --correctness-profile smoke \
  --stateful-profile smoke \
  --synthesis-profile smoke
```

## Next milestone after v0.6

v0.7 should focus on **Design-Class Benchmark Adapters** instead of module-name
specific adapters.

Example:

```text
DF-09 Round-Robin Arbiter Benchmark Adapter
                 ↓
        canonical req/grant interface
                 ↓
  ┌──────────────┼──────────────┐
  ↓              ↓              ↓
PULP          BaseJump       OpenTitan
wrapper A      wrapper B       wrapper C
  └──────────────┼──────────────┘
                 ↓
         same test profile
         same parameters
         same Yosys/Liberty
                 ↓
          fair QoR ranking
```

That is the key step toward automatic multi-repository comparison.

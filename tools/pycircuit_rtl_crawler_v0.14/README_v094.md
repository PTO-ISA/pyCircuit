# pyCircuit v0.9.4 — WSL-Native OpenSTA Staging

## Root cause

The same N=16 OpenSTA analysis showed two radically different runtimes:

```text
Files under /mnt/e/...    → timeout
Files copied to /tmp/...  → ~1 second
```

The RTL, Liberty and STA commands are otherwise equivalent.

This is a WSL cross-filesystem I/O issue. Linux tools can be much slower when
working directly on Windows-mounted drives such as `/mnt/c` or `/mnt/e`.

## v0.9.4 solution

Persistent project/results remain on:

```text
/mnt/e/desktop/GitHubProjects/pyCircuit/...
```

but OpenSTA execution is staged to:

```text
/tmp/pycircuit_sta/
```

Flow:

```text
Nangate45 Liberty on /mnt/e
        ↓ copied once
/tmp/pycircuit_sta/liberty_cache/

mapped_netlist.v on /mnt/e
        ↓ tiny per-case copy
/tmp/pycircuit_sta/cases/<project>/<module>/<config>/
        ↓
OpenSTA runs entirely on Linux-native filesystem
        ↓
logs / CSV / JSON remain stored in project on /mnt/e
```

## Recommended run

```bash
python run_timing_benchmark.py \
  --class-id DF-09 \
  --profile scaling \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --only-n 16 \
  --workers 1 \
  --timeout-sec 30
```

You should see:

```text
stage   : ON
STA lib : /tmp/pycircuit_sta/liberty_cache/...
```

If the manual A/B result generalizes, these few-hundred-cell cases should be
seconds-level rather than minute-level.

## Full run

After N=16 succeeds:

```bash
python run_timing_benchmark.py \
  --class-id DF-09 \
  --profile scaling \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --workers 2 \
  --timeout-sec 60
```

`--workers 2` is a reasonable next step after local staging; there is no need
to jump immediately to high parallelism.

## Diagnostic opt-out

To intentionally reproduce the old behavior:

```bash
--no-local-stage
```

This makes OpenSTA read directly from the persistent project filesystem.

# pyCircuit v0.9.1 — Faster OpenSTA Timing Benchmark

The original timing flow ran 12 OpenSTA jobs serially. Every job reparsed the
same Nangate45 Liberty and reported more paths than the benchmark needed.

v0.9.1 adds:

- `--workers N`: parallel independent OpenSTA cases (default 3)
- `--timeout-sec N`: timeout per case (default 180 s)
- `--only-n N`: run only selected NumIn values
- `--resume`: reuse `timing_case_report.json`
- one worst path only: `-group_count 1 -endpoint_count 1`
- flushed progress output after each completed case

Fast validation first:

```bash
python run_timing_benchmark.py \
  --class-id DF-09 \
  --profile scaling \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --only-n 16 \
  --workers 3 \
  --timeout-sec 120
```

Then full run:

```bash
python run_timing_benchmark.py \
  --class-id DF-09 \
  --profile scaling \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --workers 3 \
  --timeout-sec 180 \
  --resume
```

Do not blindly use 12 workers: every OpenSTA process loads the Liberty
independently, so memory usage also increases.

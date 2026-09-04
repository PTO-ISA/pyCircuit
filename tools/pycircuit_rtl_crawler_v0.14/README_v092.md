# pyCircuit v0.9.2 — OpenSTA Timeout Fix

## Root cause

v0.9.1 reported:

```text
RUNNER_EXCEPTION
TypeError('data must be str, not bytes')
```

All three N=16 OpenSTA jobs had actually exceeded the 120 s timeout.

Python's `subprocess.TimeoutExpired.stdout/stderr` may be bytes even when
`text=True` was used. v0.9.1 concatenated that byte string with normal text.

v0.9.2:

```text
bytes → UTF-8 decode(errors="replace")
timeout → TIMEOUT
```

instead of turning the timeout into a Python runner exception.

## Conservative timing defaults

The installed Ubuntu OpenSTA 2.0.17 is old and every process independently
parses the large Nangate45 Liberty.

Defaults are therefore changed to:

```text
workers     = 1
timeout     = 600 s/case
```

First retry only N=16:

```bash
python run_timing_benchmark.py \
  --class-id DF-09 \
  --profile scaling \
  --liberty reference_libs/nangate45/NangateOpenCellLibrary_typical.lib \
  --only-n 16 \
  --workers 1 \
  --timeout-sec 600
```

If that works, try `--workers 2` for the full run. Do not use 3+ until runtime
and memory behavior are known.

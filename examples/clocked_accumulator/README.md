# Clocked finite-state accumulator

This small model exercises the checked complete-state execution profile:
one complete state owner, an owned stateless component and a separately compiled
pure provider. No second functional model or private driver is authored.

The input memory contains four unsigned words, `3, 5, 7, 11`. `Increment` uses
`provider.increment` and the owner saves `4, 6, 8, 12` in its Table. Completion
totals are independently expected to be `4, 10, 18, 30`; final index is 4.
With the declared alternating4/8-cycle memory waits, completion cycles are
4,13,18,27. Timing executes28 ticks. Functional executes8 ticks and skips20
strict fixed-point idle cycles, retaining28 virtual cycles and all four effects.

After installing this candidate, set `PYC_TOOLCHAIN_ROOT` to its prefix:

```sh
python3 - <<'PY'
from pathlib import Path
Path('examples/clocked_accumulator/image.bin').write_bytes(
    b''.join(n.to_bytes(4, 'little') for n in [3, 5, 7, 11]))
PY
"$PYC_TOOLCHAIN_ROOT/bin/pycircuit" run examples/clocked_accumulator \
  --model clocked_accumulator.model.Accumulator --execution functional \
  --target cpp --memory examples/clocked_accumulator/simulation.json \
  --build-dir .pycircuit_out/acc-functional
```

Choose `timing` independently of `cpp|verilog`. Verilog uses physical export and
real Icarus simulation. The memory adapter and complete-witness requirements are
in the [contract](../../docs/development/clocked-model-execution.md). The installed
regression is `tests/system/test_clocked_fixedpoint.py`: raw/state/timing checks,
inactive unknown data, active unknown rollback, incomplete-witness rejection
and reuse of a helper-only unit without repeated emission or native compilation.

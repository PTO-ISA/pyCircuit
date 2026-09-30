# Diagnostics and rejection boundary

The active driver reports errors for capture, unit publication, link
validation, final-design verification, and backend emission. Unsupported source
constructs and incomplete or inconsistent unit closures must fail with a
nonzero result and a source-oriented diagnostic. No retired compiler is used as
a fallback.

The M5 profile supports portless function `@module`, nested `@rule`, one
default clock, finite scalar state, and empty static arguments. A diagnostic must
not be interpreted as a promise that other source constructs are supported.
Exact stable diagnostic codes are documented only when the active driver
contract defines them; older `PYC-*`, `ACPY-*`, JIT, and `pycc` code tables
belong to retired routes.

## Failure-preserving outputs

Compile, link, and emit publish managed outputs. Invalid inputs or failed
verification must return nonzero and preserve the prior valid output. Receipts
and publication metadata manage files; they are not semantic authorities.

## Current commands

```bash
mkdir -p .pycircuit_out/units
pycircuit compile -c src/top.py --source-root src \
  --package-prefix demo -o .pycircuit_out/units/top
pycircuit link .pycircuit_out/units/top --top demo.top.Top \
  -o .pycircuit_out/design_top.ac
pycircuit emit .pycircuit_out/design_top.ac --target cpp \
  -o .pycircuit_out/cpp
```

See the [M5 migration guide](../development/m5-migration.md) for imports,
complete unit closure, Runtime profiles, and the current capability boundary.

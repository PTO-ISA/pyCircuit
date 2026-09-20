# Diagnostics

pyCircuit uses one structured, source-located diagnostic style across:

- API hygiene scan (`flows/tools/check_api_hygiene.py`)
- CLI pre-JIT contract scan (`pycircuit emit/build`)
- JIT elaboration errors (Python frontend)
- MLIR pass errors in `pycc` (backend)

## Format

Human-readable diagnostics generally look like:

- `path:line:col: [CODE] message`
- `stage=<stage>`
- optional source snippet
- optional `hint: ...`

## Common stages

- `api-hygiene`: repository/static scan
- `api-contract`: CLI pre-JIT scan of entry file + local imports
- `family`: typed finite-family declaration and lowering errors
- MLIR pass errors from `pycc` (for example `pyc-check-frontend-contract`)

## Frontend contract marker

All frontend-emitted `.pyc` files are stamped with a required module attribute:

- `pyc.frontend.contract = "pycircuit"`

If the backend sees a missing/mismatched contract marker, `pycc` fails early.

## Queue frontend integer conversion

| Code | Trigger |
| --- | --- |
| `ACPY-CAST-001` | `ac.zext` / `ac.sext` / `ac.truncate` called with a keyword argument, without a concrete `ac.uN` / `ac.sN` / `ac.bits[N]` target type, with a target that is not strictly wider (`zext`, `sext`) or strictly narrower (`truncate`), or on a source that is not a bits value. |
| `ACPY-RANGE-001` | A bounded-range target cannot represent the requested constant or conversion. |
| `ACPY-MODULE-001` | Module boundary shape mismatch, including a result payload whose type does not match the declared return type. A same-width product returned as a wider type belongs here; convert explicitly with `ac.sext` / `ac.zext`. |

## Useful commands

Run hygiene scan (from repository root):

```bash
REPO=/path/to/pyCircuit
python3 "$REPO/flows/tools/check_api_hygiene.py"
```

Emit + compile one module:

```bash
REPO=/path/to/pyCircuit
export PYTHONPATH="$REPO/python/pycircuit/src"
python3 -m pycircuit.cli emit "$REPO/examples/pycircuit/basics/counter/counter.py" -o /tmp/counter.pyc

export PYC_TOOLCHAIN_ROOT="$REPO/.pycircuit_out/toolchain/install"
"$PYC_TOOLCHAIN_ROOT/bin/pycc" /tmp/counter.pyc --emit=cpp --out-dir /tmp/counter_cpp
```

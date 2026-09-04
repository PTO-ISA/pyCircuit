# pyCircuit Stateful Correctness Harness v0.4.1

v0.4 established combinational correctness for:

- `cc_lzc`
- `cc_popcount`
- `cc_rr_arb_tree` with `ExtPrio=1`

v0.4.1 adds temporal/stateful verification for `cc_rr_arb_tree` with `ExtPrio=0`.

## Covered modes

- `fair_internal`
- `unfair_internal`
- `backpressure_hold`
- `reset_clear`
- `lockin`
- `axi_vld_rdy`

## Source semantics used

The DUT source documents/implements:

- async active-low reset `rst_ni`
- synchronous high clear `clr_i`
- internal `rr_q` when `ExtPrio=0`
- state update only on `gnt_i && req_o`
- `FairArb=1`: next state jumps to next active request with higher index, wrapping
- `FairArb=0`: next state increments modulo `NumIn`
- `LockIn=1`: arbitration request snapshot is retained while output is not granted
- `AxiVldRdy=1`: req/gnt are treated as valid/ready

## Local migration

Reuse the same root `.venv`.

Copy existing repo and candidate:

```bash
cp -a ../pycircuit_rtl_crawler_v0.4/work ./
cp -a ../pycircuit_rtl_crawler_v0.3.1/candidates ./
```

If your v0.4 `candidates/cc_rr_arb_tree` already exists and passed Hard Gate,
copying from v0.4 is also fine.

## Smoke test

```bash
python smoke_test_v041.py
```

Expected:

```text
smoke_test_v0.4.1: PASS
```

## First run

```bash
python run_stateful_correctness.py --profile smoke
```

Then:

```bash
python run_stateful_correctness.py --profile standard
```

Inspect:

```bash
python inspect_stateful.py --profile standard
```

## Interpretation

A PASS is scoped to the listed mode/configuration set. It is not an unlimited
formal proof of all parameter combinations.

After v0.4.1 standard passes, the planned next stage is v0.5 synthesis/PPA.

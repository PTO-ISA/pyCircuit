# Circular ROB repair — c0b42d91

Base: `0e014154`. Clang 22/Python 3.11 via `$pyc`, built in the current checkout.
Decisions 0176, 0177, 0189, 0194 and 0196; source-order SSA under 0221.

## Root cause

The frontend flattened a trailing blocking `if`, processed its body, then
rewrote the guard using the final local versions. Allocation tested `count + 1`
against four; retirement tested `count - 1` against zero. The first wrong form
was raw ACIR; subsequent lowering and runtime preserved those legal operands.
The fix captures the condition at its source position before body rebinding.
Two adjacent raw-IR diffs preserve this before/after evidence.

## Behavioral evidence

- Single ROB stopped at count/tail 3/3 instead of 4/0. It now fills, retains the
  fifth request, wraps, retires in order, rejects stale generation and recovers.
- Dual ROB consumed completions and set done, but failed to retire the final
  entries. It now retires 100/200 and preserves instance isolation.
- Equivalence timed out waiting for a result despite matching scheduler state;
  exit 6 did not establish activation divergence. The matched-boundary run now
  completes with unchanged counters: scan 1769, incremental 182, activation
  215, closure 511 (`rob-counters.stdout`).
- `before.log` and `before-diagnostics.log` preserve failures; `final-rob.log`
  records all three ROB tests plus host backpressure passing.
- At this stage: 73 QueueBlocks C++ and 6 focused lit tests passed; frontend
  225 passed / 4 skipped; typed transactions 1 passed; contracts 42 passed;
  CLI 53 passed. Full Queue codegen: 30 passed / 2 failures / 1 fixture skip.

## Unresolved at this stage

`baseline-non-rob.log` independently reproduces two pre-existing structural
assertion failures: ISQ snapshot-set occurrence count 2 versus 0, and same-owner
write-policy count 1 versus 3. These are not proven harmless and were not relaxed.
Strict decision status reported 35 missing historical paths (0176–0210).
Pre-commit was unavailable at this stage; only direct formatting/hygiene checks
ran. No PYC/RTL/Verilator or release-closure claim is made.

Final evidence and reproduction commands are centralized in
[the migration report](../20260908-replay-main-migration/summary.md) and
[commands](../20260908-replay-main-migration/commands.md). The shared
[decision report](../20260908-replay-main-migration/decision_status_report.json)
checks the final curated tree, not this historical stage. Earlier reports and
repeated logs were archived locally before curation; they remain recoverable
from pre-curation commit `9ddb1c72`.

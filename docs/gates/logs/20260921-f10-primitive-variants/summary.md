# F10 primitive variants, qualification, and PPA report

## Scope

P11 requires the semantic primitive registry and the implementation catalog to
stay separate, at least two legal implementations of one semantic primitive with
legality/latency/II/port/bank/depth metadata, deterministic fail-closed
selection, and structural estimates recorded as evidence without inventing
regression thresholds.

## Variants

`pyc.priority_encode.v1` now has two qualified combinational implementations:

| implementation | module | structure | priority |
| --- | --- | --- | --- |
| `pyc.bsd.priority_encode.v1` | `pyc_priority_encode` | serial priority loop over the selected order | 100 |
| `pyc.tree.priority_encode.v1` | `pyc_priority_encode_tree` | two's-complement isolation of the selected bit plus an OR-encode of the one-hot result | 90 |

Both declare the same semantic contract: one `in_value` input, `index` and
`valid` outputs, the `WIDTH`/`ORDER_LOW` parameter bindings, `min_width = 1`,
`max_width = 64`, and a metadata block with `latency_cycles`,
`initiation_interval`, `pipeline_depth`, `banks`, `depth_entries`, `storage`,
and a documented structural estimate. The tree variant is lower priority, so the
default catalog selection is unchanged and the existing selection tests keep
passing.

## Qualification evidence

- **Observation equivalence.** `tests/system/priority_variant_parity_tb.sv`
  instantiates both implementations in both orders and compares `index`/`valid`
  against an in-testbench golden priority encoder, at widths 1, 2, 3, 4, 8, 16,
  32 and 64. Widths up to 8 are swept exhaustively; wider widths get zero,
  all-ones, every single-bit input, and a deterministic random sweep. The
  comparison count is measured, not hardcoded, and the run prints
  `priority variant parity PASS comparisons=2344`, so an empty loop cannot
  report a pass.
- **Deterministic selection.** The selector picks the single highest-priority
  legal implementation for the requested width and reports
  `RTL selection is ambiguous at priority N` when two legal implementations
  share the top priority, so a catalog cannot silently depend on entry order. A
  system test writes the same catalog twice with the implementation list in both
  orders and asserts the selected output is byte-identical, so order
  independence is regression-tested rather than argued from the pass source.
- **Fail-closed legality.** A catalog whose implementation is outside the
  semantic registry width range still rejects with
  `outside the semantic registry`, and an unqualified or malformed entry still
  rejects before emit.

## PPA report

Structural estimates are recorded in the catalog and reported, not gated.
`flows/tools/report_primitive_ppa.py` renders them as the
`pyc-primitive-ppa-report-v1` JSON report with `gating: false`: the loop
implementation costs `WIDTH` serial priority comparisons, and the tree
implementation costs one isolation term plus `WIDTH` OR-encode terms. Every
entry is combinational with `latency_cycles = 0`, `initiation_interval = 1`,
`pipeline_depth = 0`, and no storage (`banks = 1`, `depth_entries = 1`,
`storage = "none"`).

```
python3 flows/tools/report_primitive_ppa.py --out docs/gates/logs/20260921-f10-primitive-variants/ppa_report.json
```

No regression threshold is added in this stage: the brief requires thresholds
only after stable baselines exist, so the report stays advisory and the only
hard gate is the selection contract itself.

# P12 consumer-neutral MiniOOO acceptance

## Fixture

`tests/mlir/agentic-circuit/CodeGen/miniooo-acceptance.mlir` is a vendor-neutral
reduced MiniOOO, not a product Core and not a consumer model: 4-wide dispatch
with one committed reservation group, two execution resource classes
(`ac.multi_allocator`), age-ordered retirement (`ac.age_select_k`), a ready and
dependency update (`ac.dependency_set`), one memory ordering edge plus a load
disposition, a small versioned reorder window, a versioned completion, branch
recovery (`ac.recovery.event` plus `ac.kill_set`), and `ac.terminal_transaction`
for ordered retirement. It imports no product module, no SSM header, and no
consumer-specific semantic.

## Generated artifacts

One frozen ACIR is lowered through the whole chain in a single lit test, and
each stage is checked rather than merely produced:

| artifact | check |
| --- | --- |
| verified ACIR | `ac-verify-rule-closure`, `ac-verify-value-constraints`, `ac-freeze-topology` plus the `CLOSED` checks |
| Rule Effect Graph | `-ac-build-rule-effect-graph` JSON plus the `EFFECT` checks over `arbitration_domain`, `state_footprint`, `interaction`, `obligation_linkage`, `recovery_domain`, `rule` |
| QueueGraphPlan | plan JSON plus the `PLAN` checks over the lane-algebra expression kinds and the versioned stale obligation IDs |
| obligation report | the PYC `obligation_kind`/`obligation_id` records plus the generated C++ coverage counters |
| C++ model | gfsim C++ and PYC C++ compile with `-fsyntax-only` |
| Verilog and SVA | PYC Verilog plus the `SVA` checks over `assert property` and `cover property` |
| rule/blocker coverage | rule effect graph interaction/state-footprint/obligation nodes, per-rule plan guards and activation records, and the IR coverage ledger |
| deterministic parity | two emissions of PYC, gfsim C++, and Verilog are byte-diffed |
| deterministic stress | executed C++ harness and executed RTL testbench |

## Executed stress

`tests/mlir/agentic-circuit/CodeGen/miniooo-stress-harness.cpp` drives bounded
cycles against the generated PYC C++ model through the real ready/valid
handshakes, and it reads the generated versioned-window entry directly (slot 0
`valid`/`generation`/`recovery_epoch`/`attempt`/`payload`). Reading the window is
what makes the run able to tell a committed update from a rejected one: the
`no_stale_update` coverage condition is `requested && !qualified`, so that
counter counts *rejected stale* updates and grows fastest when the window is
wedged. Asserting commit liveness on it would be backwards, which is exactly the
defect the first version of this harness had.

The run is split into phases, each asserted on an observable that can fail:

| phase | cycles | what it drives | what is asserted |
| --- | --- | --- | --- |
| commit | 24 | one dispatch plus one completion per cycle, completion version matching the published one | the window payload advances on 21 of the cycles, so the window keeps applying qualified updates instead of wedging |
| stale | 12 | completions whose version no longer matches, drawn from a disjoint high payload band | no stale payload ever reaches the window (0), while the `no_stale_update` counter reports the rejections, which pins the counter's meaning |
| stale recovery | 1 | a killing invalidation with a stale version | the recover obligation reports a stale-mutation attempt *and* the slot 0 entry is still valid, so a rejected invalidation cannot be confused with a committed one |
| recover | 1 | the same killing invalidation with the live version | the entry was valid beforehand and the slot 0 valid bit drops, so the invalidation commits |
| resume | 8 | matching completions again | the window accepts qualified updates again with further payload advances |

Observed result:

```
miniOOO stress PASS cycles=46 commit_advances=21 resume_advances=6 stale_commits=0
  stale_rejections=134 commit_rejections=0 stale_invalidations=1 stale_kept_valid=1
  invalidations=1 retire_coverage=169 recover_coverage=12 stale_coverage=42 failures=0
```

The commit payloads stay below `0x80` and the stale payloads start at `0x80`, so
a high-band value in the window payload is by itself proof that a stale
completion was applied; the two bands have to stay disjoint for that check to
mean anything.

Ablations confirm the assertions are live rather than decorative. Forcing every
commit-phase completion to a stale version freezes the window and the harness
fails with `commit_advances=0`. Presenting the killing invalidation with the live
version on both occasions fails with `stale_kept_valid=0`, which is the case a
version-check regression in the recover path would produce. Removing the live
invalidation fails with `recovery branch mismatch ... committed=0`. Making the
stale completions qualify fails with `stale completions reached the window
count=10`.

`tests/mlir/agentic-circuit/CodeGen/miniooo-tb.sv` replays the same
one-dispatch-per-cycle cadence in RTL through `iverilog`/`vvp` — 48 cycles with
one killing recovery, not a port of the C++ phase schedule — and requires every
driven source to be accepted inside the handshake bound:

```
miniOOO rtl stress PASS cycles=48 dispatched=48 completed=47 recovered=1
```

The bounded run is the deterministic acceptance evidence; long random runs stay
in nightly and release lanes, and full SSM validation remains in the SSM
repository against a pinned pyCircuit revision. The RTL run compiles with
`-DSYNTHESIS` because `iverilog` cannot elaborate concurrent assertions, so it
proves cadence and liveness while the emitted SVA is checked by the `SVA` checks
and the RTL audit rather than by RTL assertion evaluation.

## Obligation evidence boundary

The executed `failures == 0` assertion is deliberately weak for the window
obligations and must not be read as runtime enforcement. In
`compiler/acir/lib/CodeGen/QueueGraphPyc.cpp` the `no_stale_update` guard is

```
stale     = requested && !qualified
selected  = requested && qualified
assert !(stale && selected)  cover stale
```

so the assertion term is orthogonal by construction and cannot fail on any
stimulus while the emitted write enable keeps excluding the stale set. Its value
is a lowering-invariant canary: it fires if a future change stops excluding the
stale set from `selected`. The tautology is visible by inspection — the operand
is `!(X & ~X)` — and an independent verification round additionally modelled the
emitted Verilog assign graph with a bit-vector solver and found all nine
generated obligation assertions
(`no_stale_response:issue_q:disposition0` and the eight
`no_stale_update:window:{retire,recover}:slot0..3`) tautologically true while the
three coverage operands are not; that solver model lives in the review record,
not in this repository. The stimulus-dependent evidence is
therefore the `cover` conditions and their counters, which is why the executed
stress is asserted on window state and coverage rather than on assertion
failures, and why the forged-lowering negative test recorded as an open item in
Decision 0281 remains the missing defense-in-depth step.

## Gate evidence

```
cmake --build .pycircuit_out/build-llvm22 --target check-acir
ctest --test-dir .pycircuit_out/build-llvm22 -R 'ACIRTypesTests|ACIROpsTests|ACIRModelAnalysisTests|CodeGenTests|CompilerTests|GfsimTests'
python3 -m pytest tests/unit -m unit -q
python3 -m pytest tests/system -m system -q
python3 -m unittest discover -s tests/python/agentic-circuit/contracts -p 'test_*.py'
python3 -m unittest discover -s tests/python/agentic-circuit/python_frontend -p 'test_*.py'
python3 -m unittest discover -s tests/python/agentic-circuit/cli -p 'test_*.py'
python3 tools/agentic-circuit/check-contracts.py
python3 tools/pycircuit/check-pyc-inventory.py
python3 tools/agentic-circuit/check-ir-coverage.py
python3 flows/tools/check_api_hygiene.py
python3 flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md \
  --status docs/gates/decision_status_v6.md \
  --out docs/gates/logs/20260921-f10-primitive-variants/decision_status_report.json \
  --require-no-deferred --require-concrete-evidence --require-existing-evidence
mkdocs build --strict
pre-commit run --files <changed files>
git diff --check
```

## Open items

- Forged-lowering negative test for the `no_stale_update` window canary
  (carried from Decision 0281).
- Edge-kind dominance, type-enforced Decision 0279 identity derivation, lane
  vector kill, and gfsim C++ obligation assertion materialization (carried from
  Decision 0281).
- Regression thresholds for the PPA report remain absent until a stable baseline
  exists; the report stays advisory.
- `schemas/primitives/acir_semantic_registry.json` is declarative: no compiler or
  flow tool consumes it yet, and its dialect binding is checked in one direction
  only (every declared `operation` must exist in `ACIROps.td`). A reverse
  coverage check and a build-time consumer are open items.
- The generated `no_stale_update` guard is emitted as `std::abort()`/`$fatal`, so
  the tautology open item has to be scheduled before any downstream claim that
  obligations are enforced at runtime.

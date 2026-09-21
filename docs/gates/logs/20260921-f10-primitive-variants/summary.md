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
  for every 8-bit input: 512 vectors, `priority variant parity PASS 512`. The
  testbench is driven from the system suite.
- **Deterministic selection.** The selector picks the single highest-priority
  legal implementation for the requested width and reports
  `RTL selection is ambiguous at priority N` when two legal implementations
  share the top priority, so a catalog cannot silently depend on entry order.
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

`tests/mlir/agentic-circuit/CodeGen/miniooo-stress-harness.cpp` drives 48 bounded
cycles against the generated PYC C++ model through the real ready/valid
handshakes. Each cycle presents one dispatch and, except for the first cycle, one
versioned completion; the first cycle presents a killing recovery instead, which
invalidates the window version the first commit publishes. Every eighth cycle
drops the identity match, and the alias/disjoint/executed/killed qualifications
rotate so the wait, bypass, forward and replay dispositions all stay reachable.

The versioned window advances its commitment version on every qualified update,
so a completion only commits when it carries the version the current commit
publishes; the harness walks the version counter across the run and asserts that
the halfway coverage counter is strictly smaller than the final one, which fails
if the window commits once and then wedges. Observed result:

```
miniOOO stress PASS cycles=48 stale_trials=6 dispatched=48 completed=47 recovered=1
  retire_coverage=454 retire_at_half=229 recover_coverage=14 stale_coverage=73 failures=0
```

`tests/mlir/agentic-circuit/CodeGen/miniooo-tb.sv` replays the same cadence in
RTL through `iverilog`/`vvp` with an independently drawn payload stream
(splitmix64 seeded differently from the C++ harness) and requires every driven
source to be accepted inside the handshake bound:

```
miniOOO rtl stress PASS cycles=48 dispatched=48 completed=47 recovered=1
```

The bounded run is the deterministic acceptance evidence; long random runs stay
in nightly and release lanes, and full SSM validation remains in the SSM
repository against a pinned pyCircuit revision.

## Obligation evidence boundary

The executed `failures == 0` assertion is deliberately weak for the two window
obligations and must not be read as runtime enforcement. In
`compiler/acir/lib/CodeGen/QueueGraphPyc.cpp` the `no_stale_update` guard is

```
stale     = requested && !qualified
selected  = requested &&  qualified
assert !(stale && selected)  cover stale
```

so the assertion term is orthogonal by construction and cannot fail on any
stimulus while the emitted write enable keeps excluding the stale set. Its value
is a lowering-invariant canary: it fires if a future change stops excluding the
stale set from `selected`. The stimulus-dependent evidence is the `cover stale`
condition, which is why the stress and the RTL fixture are asserted on coverage
counters rather than on assertion failures, and why the forged-lowering negative
test recorded as an open item in Decision 0281 remains the missing
defense-in-depth step. The recovery coverage is reached the same way: the
recovery carries an attempt that no longer matches the committed entry, so the
invalidation is reported as a stale-mutation attempt rather than being silently
applied.

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

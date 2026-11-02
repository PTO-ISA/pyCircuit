# Native NDF gate and comparison scope

This workflow implements the pinned NDF [§8 gate](https://github.com/PTO-ISA/normative_language/blob/5e3e6b5f09e06b09aaf52844c4b37e38362ac097/normative_language.md?plain=1#L530-L534):
refresh indexes before graph queries, check before commit, inspect semantic diff
and report coverage impact. `spec-check` invokes the actual NDF CLI; it does not
replace these commands with its structural binding check.

## Reproduce from distinct source snapshots

The comparison starts at `f7e29346137cad86a36c93dab29e27a1f66afe8f`, the first
revision-1 draft containing the design input, Spec and source/test bindings.
It has no approved-baseline status. The current draft additionally contains the
recorded execution evidence. Both snapshots are rebuilt with the same pinned
NDF and current binding adapter, so adapter-version differences do not stand in
for specification changes. Authored files retain their relative paths in the
projection; unchanged clauses are not reported as moved.

Input indexes record the actual source revisions. Generated bound indexes are
derived artifacts without their own checkout, so their native revision is
UNKNOWN (the native zero marker), not the surrounding output repository HEAD.
The build records its controlled Git-discovery environment.

Use this example's Git history and the NDF install from [the workflow guide](README.md).
The detached worktree is a source snapshot, not another running CPU build:

```sh
export FIRST_SPEC="$PWD/.pycircuit_out/rv32i-first-spec"
git worktree add --detach "$FIRST_SPEC" f7e29346137cad86a36c93dab29e27a1f66afe8f
git -C "$NDF_SOURCE" worktree add --detach \
  "$FIRST_SPEC/examples/rv32i_core/.pycircuit_out/ndf" \
  5e3e6b5f09e06b09aaf52844c4b37e38362ac097
export NDF_GATE_OUT="$PWD/.pycircuit_out/rv32i-native-ndf-gate"
"$PYC_TOOLCHAIN_ROOT/bin/pycircuit" spec-check "$DEMO" \
  --before "$FIRST_SPEC/examples/rv32i_core" --ndf "$NDF" --output "$NDF_GATE_OUT"
```

Use new worktree/output paths, or reuse an existing exact clean baseline source.
The full execution entry also accepts `--before`; the command above checks only
NDF workflow evidence and does not rerun ISA, RTL or synthesis experiments.

## Native commands and interpretation

`spec-check` first executes native build/check/export for the inputs and
build/check for each bound graph. Every command saves argv, stdout, stderr and
its own exit code. After those refreshes these are the actual query forms:

```sh
"$NDF" diff "$NDF_GATE_OUT/before/spec.sqlite" "$NDF_GATE_OUT/spec.sqlite"
"$NDF" diff "$NDF_GATE_OUT/before/bound.sqlite" "$NDF_GATE_OUT/bound.sqlite"
"$NDF" report coverage --index "$NDF_GATE_OUT/before/bound.sqlite"
"$NDF" report coverage --index "$NDF_GATE_OUT/bound.sqlite"
"$NDF" trace ndf://pycircuit-rv32i-demo/SRC-TRACE-ENTRY \
  --index "$NDF_GATE_OUT/bound.sqlite" \
  --edge references --edge implements --edge verifies \
  --edge refines --edge evidenced-by
```

The version diff is distinct from `binding-diff.json`, which compares the
current authored input graph against its enriched implementation/test graph.
The latter describes graph enrichment and cannot establish a historical
normative change.

All 24 recorded native commands returned rc 0. The actual draft-version diff
adds one execution-evidence node and four
`evidenced-by` edges. No normative clause is modified, moved, removed or
superseded. The enriched current graph has 39 nodes and 76 edges, including
16 bound definitions and one information-only trace entry. Original clause
bodies and lifecycle remain unchanged.

## Coverage is N/A for this draft

The pinned native implementation queries only active must L1 requirements and
direct incoming `verifies` edges. Source/test nodes are not its denominator,
regardless of their lifecycle. It does not measure executed instruction or
microarchitectural coverage.

Both snapshots contain **zero active must L1 requirements** and **five draft
must L1 requirements**. Thus both native reports have empty `unverified`,
`open_items` and `tbd` arrays, but the coverage interpretation is **N/A**, not
PASS or 100%. No lifecycle is promoted to improve this statistic. A native query
inventory and the unchanged draft eligibility are stored beside both reports.
The adapter's seven L2 implementation bindings and four bound verification
criteria are a separate structural result, not native coverage or Spec approval.

## One real trace query

The native trace command follows outgoing edges. `refines` points toward the
higher-level intent, while implementation/test edges point toward clauses.
The generated information entry uses standard `references` edges to the intent
and actual bound definitions; it does not reverse these relationships.

The query returns 35 nodes and 76 edges, with no cycles. Among its actual paths:

- `SRC-TRACE-ENTRY` references `RV32-INTENT-001` and the `core.py:Core` source node.
- That source node implements `RV32-SPEC-STATE-001`, which refines the pipeline
  and effect requirements, each refining the same intent.
- The entry references `verification/check_results.py:main`; it verifies
  `RV32-VER-VIEWS-001`, which links to the Spec and execution-evidence node.

The receipt gives the exact native IDs for each Python definition. A navigation
entry is not an additional architectural requirement or a conformance proof.
See [recorded native results](native-gate-results.json) for the actual comparison,
coverage scope, trace excerpt and exit codes. Independent review and Spec
approval remain pending; the previous simulation and mapping results are retained
with their original execution scope.

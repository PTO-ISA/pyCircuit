# Agentic Circuit open design resolutions

This page records the resolution for the open Agentic Circuit design, refactor,
and roadmap issues: what is already true in the implementation today, what
remains, and the decision that follows from it. Each section cites the evidence
that the status was measured, not assumed, so a reviewer can re-run it.

A resolution here is either a **decision** (the issue asked for a design ruling
before implementation) or a **status** (the issue's own acceptance criteria are
now met). Feature work that is genuinely unimplemented is listed as such rather
than described as done.

| Issue | Topic | Resolution |
| --- | --- | --- |
| [#147](https://github.com/PTO-ISA/pyCircuit/issues/147) | split `_queue_frontend.py` | status: substantially complete; `parser.py` remains |
| [#149](https://github.com/PTO-ISA/pyCircuit/issues/149) | scalar persistent state primitive | decision: keep the Table-only plan, add no scalar-state primitive |
| [#150](https://github.com/PTO-ISA/pyCircuit/issues/150) | unify static expansion on `ac.list` | decision: `ac.list` is the target; staged plan recorded |
| [#152](https://github.com/PTO-ISA/pyCircuit/issues/152) | unify `if`/`elif`/`else` conditional lowering | decision: shared predicate/effect model, staged plan recorded |
| [#153](https://github.com/PTO-ISA/pyCircuit/issues/153) | Table allocation replacement vs generation init | answer recorded; no feature request |
| [#128](https://github.com/PTO-ISA/pyCircuit/issues/128) | primitives/analysis/GFSim roadmap | tracker: current checkbox state re-verified |
| [#180](https://github.com/PTO-ISA/pyCircuit/issues/180) | hierarchical instances and GFSIM source bundles | status: native bundle exists, Python API and manifest missing |

## Split `_queue_frontend.py` (#147)

**Resolved as a status.** The refactor the issue proposes has happened.

Measured on the current revision:

```text
python/agentic-circuit/src/agentic_circuit/_queue_frontend.py        546 lines
python/agentic-circuit/src/agentic_circuit/_queue_compiler/         24,555 lines
  parser.py           4,961
  modules.py          4,423
  expressions.py      3,877
  lower_acir.py       3,781
  static_types.py     1,465
  state_statements.py   947
  model.py              765
  graph_statements.py   750
  definitions.py        735
  normalize.py          657
  state_semantics.py    640
  memory_statements.py  514
  statement_common.py   205
  parser_context.py     150
  source.py / provenance.py / acir_text.py / source_unit.py  < 200 each
```

The issue's proposed final structure is present, including `model.py`,
`source.py`, `normalize.py`, `static_types.py`, `parser_context.py`,
`expressions.py`, and `lower_acir.py`. `_queue_frontend.py` is now a 546-line
compatibility and orchestration entry that re-exports the stable entry points,
which is criterion 1 and criterion 2 of the issue met as written.

**What remains**, and the reason this is a status rather than a completed task:

- `parser.py` is still the largest module at 4,961 lines and still owns general
  assignment/pipeline parsing; the issue's remaining-work note names exactly
  this.
- Rule invocation and effect analysis are not yet separated into distinct
  modules; the resource-specific statement handlers (`*_statements.py`,
  `state_semantics.py`) split them by resource rather than by phase.
- The issue's last two acceptance boxes are marked partial because the full
  CLI/ACIR-verifier/GFSim lane needs a complete native SDK; that is an
  environment limit, not a code gap, and the repository's own CI lane
  (`flows/scripts/run_agentic_circuit.sh`) is where it is verified.

**Recommended next step:** a follow-up issue scoped to moving pipeline and
assignment parsing out of `parser.py`, keeping the existing
`model <- normalize/static_types <- parser -> effects -> lowering` direction. Do
not re-open #147; the structural goal it describes is met.

## Scalar persistent state primitive (#149)

**Decided: keep the Table-only plan. Do not add `ac.cell` / `ac.scalar_state`.**

The issue offers two options: introduce a design-neutral scalar committed-state
primitive, or let the frontend produce `ac.table[1]` and delete the
`ac.var.decl/read/assign` intermediate layer. The second is already what the
compiler does, and it is verified:

- `tests/mlir/agentic-circuit/Transforms/variable-state-lowering.mlir` declares
  `ac.var.decl @count type i8 init 0 : i8 owner "/" stable_id "var/count"` and
  asserts `STORAGE: ac.table @count entry i8 entries 1 init 0` together with
  `STORAGE-NOT: ac.var.decl`, `STORAGE-NOT: ac.var.read`, and
  `STORAGE-NOT: ac.var.assign`.
- `ac.var<T>` is documented as an immutable, zero-delay combinational value; the
  persistent use of `ac.var.decl/read/assign` exists only in raw ACIR and is
  eliminated before the frozen form.

So the state layering the issue asks for already holds: `!ac.var<T>` means an
immutable combinational value, and **one** committed-state owner form,
`ac.table`, carries persistent scalar and indexed state through one
snapshot/proposal/arbitration/reset/commit contract. Adding a second
scalar-state primitive would duplicate that contract, which the issue itself
warns against.

**Consequence for the issue's expectation:** the Raw-versus-Frozen state boundary
is the storage-selection boundary, and no new primitive is needed. Documented
here rather than in a new decision because the behaviour is unchanged; if the
`ac.var.decl/read/assign` raw form is later removed outright, that is a separate
IR-surface decision.

## Unify static expansion on `ac.list` (#150)

**Decided: `ac.list` is the single static-collection surface. The migration is
staged, and the feature itself is not implemented yet.**

Current measured state:

- `ac.list` does **not** exist (`hasattr(agentic_circuit, "list")` is `False`),
  and it is not in `CAPTURE_ONLY_API`.
- The collection surfaces the issue wants removed are all still present:
  `ac.array`, `ac.map`, and `ac.set` exist and are listed in
  `CAPTURE_ONLY_API`; `ac.instances` is still exported.
- `ac.array[N, T]` remains the runtime fixed-length numeric array, lowered to
  `!ac.value_array<N x T>`, which the issue keeps.

**Recommended implementation order** (each step independently reviewable):

1. Add `ac.list[V]` for module inputs and outputs, with the length derived
   statically from the argument or the returned construction and folded into
   specialization identity exactly like a dependent family parameter.
2. Expand list inputs and outputs into ordered, stably named individual ports so
   no runtime list container exists in ACIR or GFSim.
3. Support `len`, static indexing, slicing, `for`, and `enumerate`; reject
   dynamic indexing, dynamic length, heterogeneous elements, and mutation.
4. Reuse the conditional model from #152 for statically expanded dynamic
   conditions, generating a guarded operation per lane and adding no implicit
   single- or multi-election semantics.
5. Delete the other public static-collection surfaces with a migration
   diagnostic that points at `ac.list`. `ac.instances` is the first candidate
   and is already being removed on its own (see that issue).

Steps 1-3 must land before step 4 has anything to reuse, and step 4 must land
before step 5 removes `ac.array`'s topology-collection overload. Until then the
existing surfaces stay; there is no compatibility mode to preserve, only a
migration to complete.

## Unify `if`/`elif`/`else` conditional lowering (#152)

**Decided: adopt the shared path-predicate / conditional-effect model described
in the issue. The refactor is not implemented yet.**

The issue is explicit that it adds no user-facing syntax and asks for one
internal model instead of the current per-object paths. The current
implementation handles static configuration branches, rule-local value joins,
scalar and Table conditional writes, early return, optional output, Slot
release, and restricted Queue `route`/`apply`/`merge` through separate
recognizers, which is what the issue describes.

**Recorded decisions:**

- Compile-time boolean conditions select a branch and never enter the runtime
  model.
- A runtime condition is computed once; nested paths compose through a shared
  predicate DAG, with `path0 = a`, `path1 = !a && b`, `path2 = !a && !b`.
- Pure total combinational computation stays ordinary SSA; a fully defined
  branch value joins with `ac.var.select`.
- A value that exists on only some paths keeps value plus presence and may not
  escape unguarded.
- `VarWrite`, `TableWrite/Replace`, `SlotRelease`, and `OutputProduce` carry the
  same path-presence representation, and mutually exclusive effects on one owner
  merge once; provably unsafe overlap stays rejected.
- Diagnostics keep the original source location.
- Path-dependent Queue consumption needs its own resource semantics and must
  land in ACIR/verifier plus every backend, never in the frontend or one
  backend alone.

**Staging:** (1) introduce the predicate/effect structures behind an adapter and
keep representative raw ACIR byte-identical; (2) migrate rule-local SSA, var,
Table, Slot, early return, and output presence onto them and delete the
duplicated analyses; (3) add sharing, exclusivity, and size bounds to the
predicate DAG; (4) design path-dependent Queue readiness with the ACIR verifier
first and then all backends; (5) make #150's static expansion reuse the model.

Criterion for step 1 is the same one this repository applies elsewhere:
representative raw ACIR must not change, so the refactor can be reviewed as a
refactor.

## Table allocation replacement and generation initialization (#153)

**Answer recorded. No feature request, so no code change.**

The issue distinguishes four ways to express one logical ROB entry and asks
which are supported. The answer for the current contract:

- **Parallel tables per writer** is a frontend conflict-avoidance technique, not
  a requirement. Field-level proposal is available, so a single logical Table
  can carry disjoint field writers; `write_fields` keeps the accurate
  per-endpoint footprint instead of degrading to whole-entry replacement.
- **Whole-entry allocation replacement** genuinely overlaps the other field
  writers. That overlap is real semantics, not a misclassification: if the model
  requires allocation to initialize the whole entry, the mutual exclusion,
  priority, or arbitration belongs in the model, and the frontend rejects a
  provably unsafe overlap rather than guessing.
- **Generation-tagged initialization** avoids the replacement by invalidating
  the old generation logically, which keeps the per-writer footprints disjoint;
  every read must then check the generation, and a late response carrying a
  stale generation cannot pollute the new entry. This is the documented way to
  keep one Table without a whole-entry writer.
- **Physical storage is independent of the logical Table.** One logical
  `ac.table[N, Entry]` does not require one wide SRAM; the backend may split
  narrow arrays, keep lifecycle bits in registers, or add ports/arbitration for
  genuinely overlapping writes.

This matches the issue's own summary, so the discussion's conclusion is now
recorded in the repository instead of only in the issue thread.

## Primitives, analysis, and GFSim roadmap (#128)

**Tracker, updated on the current revision.** The issue is an umbrella with
per-item checkboxes and remains the right place to track the remaining work. The
items still unchecked are the typed positive-latency feedback edge (`J`),
cross-owner transaction groups (`K`), mutually-exclusive state-write mux
synthesis, unchanged-store elimination, invariant proof propagation (`L04`,
`L05`, `L07`), and part of the cross-layer verification matrix (`V04`, `V05`,
`V07`, `V08`), several of which depend on the design work recorded above for
issues #150 and #152. Three matrix items are verified on the current revision,
each with a recorded run:

- `V01` (frontend positive/negative examples, public API, and typecheck):
  417 tracked frontend tests with 304 `assertRaises` call sites, 31 public-API
  tests, 29 contract tests, and the contract plus diagnostic-catalog gates. See
  `docs/gates/logs/20260922-v01-v02-cross-layer-audit/`.
- `V02` (Frozen ACIR verifier/lit coverage and the QueueGraph schema round trip):
  95 lit fixtures run `ac-freeze-topology` and 53 run `ac-verify-rule-closure`,
  and `QueueGraphPlanTest` covers 110 cases including byte-identical canonical
  JSON and the re-parsed source-map JSON. See the same log.
- `V03` (no residual `scf.*`/`index` after ACIR to PYC lowering, PYC verifier
  accepts): `tests/python/agentic-circuit/python_frontend/test_pyc_lowering_closure.py`
  freezes five frontend-published designs, lowers each to PYC, and requires
  `pycc` to accept the result. See `docs/gates/logs/20260922-v03-pyc-closure/`.

Two items deserve emphasis because they are the ones a reader would otherwise
mistake for complete:

- `J`/`K` are the same family as the path-dependent Queue work in #152: both need
  an ACIR/verifier owner with cross-backend semantics before any frontend
  surface. They are not frontend-only changes.
- `V08` requires a fixed revision in the consumer checkout and stays a
  consumer-side obligation under Decisions 0158 and 0235. `V04` and `V07` need a
  Verilog toolchain, the simulation lanes, and strict documentation, none of
  which are available in this environment, so neither is claimed. `V05` is
  partially satisfied: `test_source_map_goldens.py` pins the helper and
  projection source maps against the published schema
  (`docs/gates/logs/20260922-v05-source-map-goldens/`), while the module and
  specialization constructs are now all pinned. The linked package flow landed
  with #258, publication provenance with #259, and the goldens live in
  `tests/goldens/agentic-circuit/source-map/`: `helper.json` and
  `projection.json` for the flat constructs, `module.json` for the linked
  three-file hierarchy's placement provenance, and `specialization.json` for a
  parameterized family carried across units (the test also proves the published
  unit keeps both declared cases and their static arguments).

## Hierarchical instances and structured GFSIM source bundles (#180)

**Partly implemented. The Python API and the module/instance manifest are
missing; the issue's API reference is stale.**

The native bundle already exists: `acc -emit-cpp-bundle` publishes a deterministic
multi-file directory with a generated `CMakeLists.txt`. Verified by running it on
a compiled `.ac` unit whose plan is flat:

```text
CMakeLists.txt
include/generated/model.h            # opaque lifecycle ABI
src/generated/model.cpp
src/generated/queuegraph.cpp
share/generated/cost-report.json
share/generated/source-map.json
```

For a **structured** plan the emitter additionally publishes one source group per
module, which is the per-module hierarchy preservation the issue asks for
(`compiler/acir/lib/CodeGen/QueueGraphGenerator.cpp`): `include/generated/dut.h`
as the typed root interface, `include/generated/interfaces/<stem>.hpp`,
`include/generated/modules/<module>.hpp`, and
`src/generated/modules/<module>.cpp`. `dut.h` is not published for a flat plan,
which is why the specification scopes its description to "a structured C++
bundle".

The **multi-unit `--header-output` flow** that reaches a structured bundle is
now exercised end to end.
`tests/python/agentic-circuit/python_frontend/test_multi_unit_package.py`
compiles a three-file hierarchy (`source/child_a.py`, `source/child_b.py`,
`source/core.py`) into per-source AC units and interface headers, links the
directory with `acc -c <package> -verify`, and emits the structured bundle with
`acc -c <package> -emit-cpp-bundle`: `include/generated/dut.h`, one
`include/generated/modules/<module>.hpp` and `src/generated/modules/<module>.cpp`
per module, and the published source map. Evidence:
`docs/gates/logs/20260922-multi-unit-package/`.

Making that flow link required three frontend properties that were previously
wrong or absent: a declaration owns the nominal declaration inventory its ports
need (so `ac.module.import` carries the provider's inventory), an implementation
whose symbol carries a declaration entry publishes the declared interface
skeleton (so the import and provider schemas compare equal), and every nominal
declaration names its owning Python file with `ac.source_file` (so the compiler
splits one interface unit per source file and a shared nominal is declared once).

What is genuinely missing:

- **No Python/JIT API.** `lower_cpp()` does not exist in this tree, so the issue's
  `ac.jit(...).lower_cpp()` reference is stale: the authoring flow is
  `acc.py -c <source>.py -o <unit>.ac` followed by the native `acc`. There is no
  `lower_sources(...)` equivalent that hands a Python caller the generated source
  files or a manifest. (Identity is structural under Decision 0267, so a bundle
  carries no content identity to expose either way.)
- **No Python API yet.** The manifest artifact exists: a structured bundle
  publishes `share/generated/module-manifest.json`
  (`agentic-circuit-module-manifest`, version 0.1, schema
  `schemas/agentic-circuit/module-manifest.schema.json`) with one entry per
  module family (symbol, owning files, interface ports, static parameters,
  declared cases, concrete case signatures) and one per placement (definition,
  scope, ordered typed static arguments, source provenance), so two placements of
  one definition are distinguishable without generated names. What is still
  missing is a Python caller that returns the generated sources plus that
  manifest: today a consumer drives `acc.py -c <source>.py -o <unit>.ac
  [--header-output <header>.ac]`, `acc.py -c <core>.py --unit interfaces`, and
  `acc -c <package> [--emit-cpp-bundle]` itself.

**Recommended shape** for the remaining work, consistent with the module family
work that has landed since the issue was filed: derive the manifest from the typed
`ac.module` / `ac.instance` structure rather than from generated C++ names, and key
repeated instances by the `(definition symbol, ordered typed static arguments)`
pair that Decisions 0277/0278 already fix as the specialization identity. Two
placements of one definition must share implementation code and own independent
state, which is the same property the family codegen now verifies.

This is a feature sized like the finite-family work, not a bug fix; it needs its
own design pass and gate evidence. Recording the corrected starting point here so
the issue is not read as untouched.

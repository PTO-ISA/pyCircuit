# NDF specifications and Python traceability

`pycircuit spec-check PROJECT --ndf NDF --output NEW_DIRECTORY` checks a model's
NDF documents and their Python definition bindings. It is an optional workflow;
it does not change the hardware language or approve a specification.

## Format and dependency

The supported input is the real NDF 0.2 Markdown/project/lock format from
[`PTO-ISA/normative_language` at 5e3e6b5](https://github.com/PTO-ISA/normative_language/tree/5e3e6b5f09e06b09aaf52844c4b37e38362ac097).
Use its `ndf.yaml`, `ndf.lock`, document front matter, stable clause IDs and
canonical `kind`, `modality`, `refinement`, `status` and relationship fields.
NDF metadata must immediately follow its clause heading. A successful NDF
check with only document nodes does not establish requirement coverage.

The CLI executable is supplied explicitly. Pin its source in the project's
lock, build that revision with its locked Cargo workflow and retain the command
output. NDF checks dependency identity and graph consistency. pyCircuit does
not install or fetch this dependency implicitly.

## Design and implementation

Keep design intent (L0), externally observable requirements (L1), implementation
constraints (L2) and verification criteria (L3) present together. Connect them
with `refines` and `verifies`. Derive requirements from the design's architectural
authority and selected environment before choosing implementation mechanisms.

Attach standard NDF source comments to named Python definitions, immediately
before their decorators or declaration:

```python
# ndf: implements=COUNT-STATE-001
@ac.module
def Counter(value: ac.u8) -> ac.u8:
    ...

# ndf: verifies=COUNT-VER-001
def test_counter():
    ...
```

Qualified `ndf://PROJECT/ID` targets in the owning project are also accepted. `implements` targets
requirement clauses; `verifies` targets verification clauses. Bind the actual
state-owning Module, atomic transaction and pure calculation where appropriate.
A test binding points to the independent assertion's entry, not its expected
result generator inside a DUT.

## What is checked

The command builds fresh source and bound indexes before graph queries, checks
both projects, and exports the source Spec with the external NDF CLI. It resolves Python comment tokens to actual definitions, checks all L2
requirements have implementation bindings and all verification criteria have
test bindings, and checks L1/L2 coverage through verification/refinement edges.
Empty requirement graphs, dangling targets and unattached annotations fail.

The pinned NDF CLI does not expose its Python repository adapter. This bridge
therefore generates ordinary NDF Markdown source/test nodes and checks that
bound graph with the same CLI. Original Spec and NDF JSON evidence files are
copied byte-for-byte at their original relative paths, so projection does not
create false clause moves in native diff. The generated index is disposable. The receipt records each qualified symbol,
source location and content digest. It is not injected into calculation IR.

`status: draft` remains draft after a passing check. The receipt explicitly
separates traceability PASS, approval NOT_RECORDED and behavioral validation
NOT_RUN. Independent tests still run through public compile/link/emit/run;
their candidate-bound results are separate evidence. Comments alone cannot
prove instruction semantics, timing, effects or RTL correctness.

Output must be a new directory. Failures retain an INCOMPLETE receipt and the
external command's stdout/stderr/exit code rather than a previous success.

## Native build/check/diff/coverage gate

The pinned [NDF format contract, §8](https://github.com/PTO-ISA/normative_language/blob/5e3e6b5f09e06b09aaf52844c4b37e38362ac097/normative_language.md?plain=1#L530-L534)
requires index refresh before graph-dependent work, check before commit,
semantic diff inspection and a coverage-impact report. `spec-check` now invokes
those native commands and saves each argument vector, stdout/stderr and exit
code. It also runs a native trace query.

Two comparisons have different meanings:

- `binding-diff.json` compares the authored Spec/evidence index with the same
  inputs enriched by actual Python bindings and navigation. It describes graph
  enrichment, not a historical specification change or approval.
- `--before PREVIOUS_PROJECT` rebuilds a distinct previous source snapshot and
  records `version-diff.json` for the authored Spec and `bound-version-diff.json`
  for both bound graphs. The caller states the revision and authority of that
  starting point; a first draft is not an approved baseline. Both snapshots use
  the same pinned NDF and current binding adapter.

Input indexes retain their real Git source revisions. Generated projections have
no Git checkout of their own; their native revision is UNKNOWN (the NDF zero
marker), rather than inheriting an unrelated output-directory repository. The
controlled Git-discovery environment is recorded with that build command.

```sh
pycircuit spec-check project --before previous-project \
  --ndf "$NDF" --output "$OUT"
"$NDF" diff "$OUT/before/spec.sqlite" "$OUT/spec.sqlite"
"$NDF" report coverage --index "$OUT/before/bound.sqlite"
"$NDF" report coverage --index "$OUT/bound.sqlite"
```

Native coverage in this exact version checks only `kind=requirement`,
`status=active`, `modality=must`, `refinement=L1` and direct incoming `verifies`
edges. It does not count source/test nodes or measure execution coverage.
The adapter records a native query inventory beside the report. Zero eligible
requirements is **N/A**, not coverage PASS or 100%; draft L1 requirements are
listed separately and are never activated to improve the statistic. The
adapter's transitive L1/L2 binding checks remain a separate structural result.
An active requirement with no direct verification is reported as a native gap,
even if the structural refinement check succeeds.

Native trace follows outgoing edges. `refines` points toward intent, while
`implements` and `verifies` point from definitions/checks toward clauses. A
single generated `information` node, `SRC-TRACE-ENTRY`, uses standard
`references` links to the real intent and bound definitions. It is a navigation
entry, not a normative requirement or evidence of conformance. No existing edge
is reversed and no clause content is rewritten.

```sh
"$NDF" trace ndf://PROJECT/SRC-TRACE-ENTRY --index "$OUT/bound.sqlite" \
  --edge references --edge implements --edge verifies \
  --edge refines --edge evidenced-by
```

The result includes intent, implementation, verification and available evidence
in their actual edge directions. Each binding receipt gives the native node ID
and its Python definition. Build/check and the distinct comparison indexes are
prerequisites for interpreting these query results.

## Verification and review

Use independent results and state/effect checks, then link evidence using the
NDF 0.1 evidence schema. Record exact source and dependency revisions, selected
programs, conditions and unexecuted criteria. Do not turn a subset into a full
conformance claim. Review and any required approval follow
[project governance](project-governance.md#independent-review); graph validity
and a passing execution do not supply either.

Candidate-bound checks are recorded in [validation results](model-spec-validation.md).

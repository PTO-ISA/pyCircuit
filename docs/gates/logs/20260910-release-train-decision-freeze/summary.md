# pyCircuit 6.0.0 release-train decision freeze

## Scope

This change freezes the remaining open-issue contracts before implementation.
It does not add frontend, verifier, runtime, backend, packaging, or release
workflow behavior.

## Independent architecture selection

An independent third-party architecture review selected the following
boundaries:

- keep `@ac.rule` as the only public scheduling entry;
- resolve same-field conflicts by explicit pre-prepare branch arbitration while
  preserving Decision 0156's replace-over-field policy;
- represent every multidimensional `TableChoice.index` as one canonical
  row-major flattened scalar;
- represent `Table.choose(..., count=N>1)` as a static tuple while preserving
  the scalar `count=1` path;
- commit every formed ordered valid prefix atomically;
- realize the admitted Table PYC profile as an explicit register bank rather
  than `sync_mem`;
- publish two platform-specific `pycircuit-hisi` wheels and two universal
  wheels; and
- run candidate acceptance, annotated tagging, accepted-byte publication, and
  stable-URL verification in one source-SHA-pinned manual workflow.

The review verdict was `APPROVE WITH REQUIRED CHANGES`. Decisions 0232 and 0234
now incorporate those release changes.

The focused raw addendum is retained in
`advisor-addendum.raw.md`; it also fixes multidimensional choice indices as
flattened scalars and same-field arbitration as pre-prepare branch selection.

## Decision map

| Decision | Issue | Frozen contract | Status after this change |
| --- | --- | --- | --- |
| 0236 | #28 | compiler-owned atomic `@ac.rule` transactions | `deferred` |
| 0237 | #25 | same-field writer proof and deterministic arbitration | `deferred` |
| 0238 | #23 | multidimensional Table layout, initialization, and masks | `deferred` |
| 0239 | #24 | static-tuple multi-selection and atomic valid prefix | `deferred` |
| 0240 | #21 | ordered multi-lane Queue identity and atomic transfer | `deferred` |
| 0241 | #22 | bounded register-bank Table PYC/RTL realization | `deferred` |

Decision 0232 remains deferred until the exact four-wheel SDK layout and both
platform relocation gates have concrete implementation evidence. Decision 0234
remains deferred until its Part A builder, validators, promotion DAG, and
workflow-DAG tests have concrete repository evidence. Part B is a mandatory
per-release external attestation and is not a pre-candidate source-gate input.

## Validation

The following documentation-only checks passed from this checkout:

- `git diff --check`
- `python3 flows/tools/check_decision_status.py --rfc docs/rfcs/pyc6-decisions.md --status docs/gates/decision_status_v6.md --out <temporary-path>/decision_status_report.json --require-concrete-evidence --require-existing-evidence`
  (`rows=241`, `deferred=8`)
- `mkdocs build --strict`

The eight deferred rows are Decisions 0232, 0234, and 0236–0241. The strict
release flags `--require-no-deferred --require-all-verified` are intentionally
not used for this decision-only slice; they must fail until implementation and
concrete gate evidence exist. No implementation or release claim is made by
this evidence file.

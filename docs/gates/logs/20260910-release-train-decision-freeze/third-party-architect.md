# Independent Third-Party Architect Decision Record

## Scope

The user required an independent third-party architect to select the public
semantics and release protocol before implementation. The configured DeepSeek
architecture-advisor lane reviewed the iteration-3 PRD, test specification,
current decisions, ACIR/PYC definitions, release schemas, wheel builder, and
release workflow in read-only mode.

## Selected Decisions

### Public scheduling surface

**Selected:** `@ac.rule` remains the only public scheduling boundary.
`ac.transition`, `ac.branch`, dynamic checks, prepare/publish, and reservations
are compiler-internal.

**Rejected:** public `@ac.transition`, `safe=True`, explicit Queue commit
mechanics, and compatibility aliases.

**Fail closed:** unproven overlap fails in the verifier and cannot be
user-overridden.

Evidence reviewed: `docs/acir/spec/agentic-circuit.md:906-908` and the existing
rule/commit decisions in `docs/rfcs/pyc6-decisions.md`.

### Table `count > 1` representation

**Selected:** omitted `count` and `count=1` preserve scalar `TableChoice`.
`count=N>1` returns a static N-element tuple. Frozen ACIR returns exactly
`[index_0..index_N-1, valid_0..valid_N-1]`.

**Rejected:** runtime collections, opaque iterables, and per-lane partial
acceptance.

**Fail closed:** count, result arity/order/types, static-only access, invalid
lane zero indices, and contiguous-prefix validity are verified.

Evidence reviewed:
`.omx/plans/prd-complete-open-issues-release-6-0-0.md:360-383` and the current
scalar `TableChooseOp` contract.

### Shared lane atomicity

**Selected:** validity is a contiguous prefix. Once formed, the complete valid
prefix transfers and commits atomically. Partial output readiness stalls the
whole prefix.

**Rejected:** independent per-lane commit and scalarization into unrelated
Queues.

**Fail closed:** unsupported lane/rate/shape combinations fail before backend
lowering.

Evidence reviewed:
`.omx/plans/prd-complete-open-issues-release-6-0-0.md:380-382,405-417` and
`compiler/acir/lib/CodeGen/QueueGraphPyc.cpp:840-842`.

### Table PYC/RTL realization

**Selected:** admit only the bounded Table profile and realize it as explicit
per-entry `pyc.reg` old-state/next-state register banks.

**Rejected:** current `sync_mem` / `sync_mem_dp` prototypes and open-ended
Table generalization.

**Fail closed:** rank, flattened-entry, element-width, total-state, and writer
limits are independently verified before PYC lowering. Unsupported residual
Table state retains an explicit diagnostic.

Evidence reviewed:
`.omx/plans/prd-complete-open-issues-release-6-0-0.md:436-463` and
`compiler/acir/lib/CodeGen/QueueGraphPyc.cpp:585`.

### Cross-platform wheel topology

**Selected:** publish four wheel assets: platform-specific `pycircuit-hisi`
wheels for Linux x86_64 and macOS arm64, plus universal
`pycircuit-semantic-core` and `agentic-circuit` wheels.

**Rejected:** one unkeyed wheel per distribution and ambiguous platform
mapping.

**Fail closed:** missing, duplicate, wrongly keyed, or identity-mismatched
wheels invalidate candidate aggregation.

Evidence reviewed: `packaging/wheel/create_wheel.py:95-120`, which copies the
native toolchain into `pycircuit-hisi` and emits a platform-tagged wheel, plus
the pure-Python distribution metadata under `python/semantic-core/` and
`python/agentic-circuit/`.

### Candidate, tag, publish, and post-check protocol

**Selected:** one source-SHA-pinned workflow run builds and accepts exact
candidate bytes, creates the annotated tag only after acceptance, publishes the
retained bytes, then redownloads stable release URLs. Tag push never rebuilds
or republishes.

**Rejected:** tag-triggered rebuild, publication directly from build jobs, and
post-checks that use Actions artifact URLs.

**Fail closed:** every publish job depends on one candidate-acceptance barrier;
stable-URL verification is mandatory for the release instance.

Evidence reviewed: `docs/rfcs/pyc6-decisions.md:8319-8340` and
`.omx/plans/prd-complete-open-issues-release-6-0-0.md:497-503`.

## Independent Addendum

The advisor performed a second focused review after the decision-freeze reviewer
identified two missing choices.

### Multidimensional Table canonicalization

**Selected:** a Table has a non-empty static shape, row-major storage with the
rightmost axis varying fastest, per-axis bounds, a typed versioned initialization
image, and masks over the row-major projected domain.

**Rejected:** backend-dependent layout, initialization that infers shape from
values, producer-dependent images, and partial mutation after bounds/image
failure.

**Fail closed:** static bounds fail verification; dynamic bounds and invalid
images fail before proposal formation.

Evidence reviewed: Decision 0238 and the rank/bounds/image cases in the release
train test specification.

### Same-field writer arbitration

**Selected:** same-field writers require verifier proof or a deterministic
high-level policy with stable endpoint identity. Source order and
last-writer-wins are forbidden.

**Rejected:** implicit execution-order priority and policies that bypass type,
scope, ownership, or atomicity verification.

**Fail closed:** conflicting candidates are resolved into an explicit branch
before resource preparation. A non-selected candidate never forms a committed
transition, and none of its Queue or state effects publish. Decision 0156's
replace-over-field ordering remains the built-in policy for its admitted
single-replace case.

Evidence reviewed: `docs/rfcs/pyc6-decisions.md:3824-3847` and Decision 0237.

### Cross-contract multidimensional selection index

**Selected:** every `TableChoice.index` is the canonical row-major flattened
unsigned scalar for the full Table domain, so Frozen ACIR retains exactly
`2*N` results. A `TableChoice.coordinates` convenience is not required for
6.0.0; if introduced later, it may only be a pure static tuple derived from the
scalar by constant row-major div/rem and cannot become an additional choose
result or runtime collection. Rank-one `.index` behavior is unchanged.

**Rejected:** coordinate tuples as direct choose results, which change result
arity by rank, and backend-specific index encodings.

**Fail closed:** the verifier checks the flattened domain width. Runtime
collections, rank-dependent choose result arity, and backend-specific index
encodings are rejected.

**Addendum verdict:** APPROVE the multidimensional and arbitration contracts
with these explicit clarifications; `TableChoice.coordinates` is NOT REQUIRED.

Raw advisor records:

- `docs/gates/logs/20260910-release-train-decision-freeze/advisor-addendum.raw.md`
- `.omx/artifacts/ask-deepseek-release-architect-20260910.md`
- `.omx/artifacts/ask-deepseek-release-architect-addendum-20260910.md`

## Verdict

**APPROVE WITH REQUIRED CHANGES**

The selected architecture matches the iteration-3 plan. The required changes
are implementation deliverables already assigned to issue #61:

1. Add a Decision-0234-compliant `candidate-acceptance` dependency barrier.
2. Add stable-URL post-publication verification and an immutable check-run
   attestation outside the release asset set.
3. Implement the platform-keyed four-wheel schemas, manifests, release index,
   consumer lock, and validators.

No product file was modified by the third-party advisor.

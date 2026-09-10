# Issue 23 Table layout implementation evidence

## Scope

The frontend, ACIR verifier, QueueGraph planner/generator, and typed gfsim now
implement the non-PYC portion of Decision 0238:

- non-empty positive static shapes with checked flattened products;
- canonical per-axis widths and row-major version 1 layout identity;
- zero shorthand and versioned typed scalar or aggregate initial images;
- `ac.table.index` per-axis bounds and canonical flattened indices;
- direct reuse of a same-Table `TableChoice.index` with strict provenance;
- explicit full and projected match domains, including empty projections;
- deterministic row-major access, projected masks, old-state observation,
  next-image publication, and typed-image reset in gfsim; and
- independent QueueGraph revalidation of schema digests, domains, widths, and
  typed image metadata.

Malformed rank, extent, product, width, layout, digest, image, projection,
index provenance, or bounds metadata fails before mutation.

## Review closure

Final evidence was collected after independent review findings were fixed:

- empty projected domains now validate their fixed offset;
- power-of-two Table bounds retain an out-of-range runtime sentinel without
  truncating it to the legal index width;
- QueueGraph recomputes and verifies typed `schema_id` values while preserving
  canonical rank-one compatibility; and
- generated aggregate Tables retain their typed initial image across reset.

## Evidence

- LLVM22 current-checkout build: PASS.
- `check-acir`: PASS, 194 of 194 tests.
- Native ACIR/QueueGraph/gfsim lane: PASS, 3 of 3 tests.
- Python public API, Queue frontend, and multidimensional Table tests: PASS,
  176 of 176 tests.
- Agentic repository contracts: PASS, 15 public schemas and 35 stdlib
  components.
- Focused layout/OOB/init/projection tests: PASS, all 22 `RUN` directives in
  two lit tests.
- Strict MkDocs build: PASS.
- Decision-status validation with concrete existing evidence: PASS.
- `git diff --check`: PASS.

Raw stdout, stderr, and return-code files are stored beside this summary. Exact
commands are in `commands.txt`.

## Remaining verification boundary

This evidence does not close issue #23 or make Decision 0238
implemented-verified. The decision explicitly requires parity on the later
admitted PYC C++/Verilog Table path. Decision 0241 has not admitted Table into
canonical PYC, and PYC/RTL therefore continues to reject Table with
`unsupported provisional Table`.

Decision 0238 is recorded as implemented-unverified. Promotion requires the
Decision 0241 Table admission and cross-backend parity evidence.

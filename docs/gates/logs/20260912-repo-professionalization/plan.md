# Repository professionalization cleanup plan

## Target outcome

- Keep every top-level directory responsibility explicit and testable.
- Separate user-facing pyCircuit utilities from build/gate implementation.
- Remove completed migration pages and current APIs that still present
  themselves as legacy compatibility paths.
- Make the root README a concise project hub with consistent, linked badges.
- Make getting-started task-oriented, current-checkout reproducible, and free
  of obsolete phase labels or generated output under tracked-looking roots.

## Behavior lock

- Preserve Decision 0157 source ownership: `python`, `compiler`, `library`, and
  `simulator` remain the implementation roots.
- Preserve public Python imports, CLI entrypoints, IR names, contract epochs,
  trace schemas, runtime semantics, and backend behavior.
- Preserve historical decisions, release tags, and immutable gate evidence.
- Preserve deliberate negative tests and migration diagnostics that reject
  removed source spellings.

## Change slices

1. Add repository-layout documentation and tests before moving utilities.
2. Move pyCircuit-facing tools under `tools/pycircuit`; keep flow-only helpers
   under `flows/tools`.
3. Rename the current generated opaque dispatch ABI so active code no longer
   calls itself legacy; add no compatibility alias.
4. Consolidate completed repository migration history into the existing
   historical reference page and remove obsolete active navigation entries.
5. Rewrite README and getting-started pages as hub-and-spoke onboarding.
6. Partition PR, author, nightly, and release gates; remove recursive closure
   calls and lock single-execution ownership with a unit contract.
7. Run focused runtime/codegen tests, repository/Python contracts, link and
   layout checks, strict docs, and decision-status validation.

## Stop conditions

- No active source, generated code, README, or getting-started page uses a
  legacy phase/path label for a current contract.
- Every tracked top-level directory is documented in the repository layout.
- Root README commands and both frontend quickstarts execute from the current
  checkout.
- PR CI remains Python-only, nightly runs only heavy diagnostics, and release
  invokes every full-closure lane exactly once against one integrated build.
- Required CI and targeted native evidence pass before merge.

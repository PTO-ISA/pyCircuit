# C3 private publication engine independent review A

Independent Sol high `code-reviewer`, 2026-09-28, reviewed isolated commit
`8c91bd0c` against approved C3-C publication sections. Three committed files
have content-manifest SHA-256
`be5a8e20d702ed16e5f3f517d2a6c6c5e3f6233e477bfd28bc970b556bf14e9d`.
Verdict: **REQUEST CHANGES**; the private engine is not accepted.

Four high-severity gaps:

1. The transaction shape only supports directories, while C3 requires the
   same protocol for single-file `program.ac`.
2. Locking one destination at a time cannot implement the approved sorted
   all-input shared/all-output exclusive lock set and conflict rejection.
3. Stable HeaderView and recovery share one artifact validator; the former
   must read only receipt/header, while recovery needs full validation.
4. Windows rename/replace has no durable directory flush before the approved
   journal phase transition.

Four medium-severity gaps: QualifiedSymbol owner validation admits invalid
names; an error after journal removal can report cleanup pending without a
journal to drive later cleanup; Windows metadata opens can race a reparse
substitution after a separate path check; and mandatory file-publication,
lock-set concurrency, Windows and HeaderView/recovery oracles are missing.

Focused tests passed 38/38. Full unit suite gave 300 pass/4 fail; all four
failures reproduce at the parent commit and are unrelated baseline failures.
Ruff, Python compilation and diff checks passed. These results do not close
the contract gaps; implementation and independent re-review are required.

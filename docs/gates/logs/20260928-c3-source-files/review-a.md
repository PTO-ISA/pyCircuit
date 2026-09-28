# C3 stable source and source-unit files independent review A

Independent Sol high `code-reviewer`, 2026-09-28, reviewed exact P2 commit
`136cb84d` against approved C3-C and C2 source ownership. Three reviewed
files have content-manifest SHA-256
`73ce7e0fee7b5cf4905b9675c227740524f55d7060764e45fc45405aa6851bfc`.
Verdict: **REQUEST CHANGES**.

One high-severity finding: source containment is checked by resolving an
absolute path, then reopened later with only final-component no-follow. A
deterministic ancestor symlink swap caused capture of outside content while
retaining the in-root path. The raw-byte helper also fails to require a
regular file, accepting `/dev/null` and risking a FIFO stall. Open relative
components from a trusted source-root descriptor and validate the final
regular-file handle; unsafe Windows paths must fail closed until a platform
implementation is verified.

One medium-severity finding: source-unit readers keep a separate
single-destination lock/recovery loop rather than P1's split-validator,
command-wide lock set. Sequential `-I` inputs could come from different
publication epochs. Use P1's bounded stable/header validator and full
recovery validator under a whole-command lock set, with multiple-input
writer-blocking regression.

Focused P2 tests passed 25/25; broader publication/capture tests passed 147
with three Windows-only skips. Ruff and Python compilation passed. These
results do not close the two review findings; P2 remains unaccepted.

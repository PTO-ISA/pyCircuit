# C3 private publication transaction independent review B

Independent Sol high `code-reviewer`, 2026-09-28, re-reviewed transaction
repair `1db7101d` in the seven-file combined P1/P2 context. Scoped textual
content-manifest SHA-256:
`eab33bd0b3c73748cd317ccb8cac895db59d32586388b18d3aacd3f59391c9d5`.
Verdict: **REQUEST CHANGES**, one high-severity residual.

Seven of the eight [review-A findings](review-a.md) were repaired:
single-file program artifacts share the directory transaction state machine;
command-wide locks are sorted and reject equal/ancestor conflicts; no-journal
HeaderView uses a separate stable validator; qualified definitions are
canonical; cleanup after terminal journal removal is classified correctly;
file/concurrency/schema/recovery tests were added. Platform code inspection
does not replace Windows runtime validation.

The remaining high issue is committed-but-unclean publication reading:
generic `_read_published` and command-wide input locking invoke only
`stable_validate` when a formal `committed` journal exists. C3 requires full
validation of that unfinished new target before projection. A cleanup-pending
probe observed validator calls `['stable']`. P2's special source-unit reader
does full-validate this state, but generic generated/program readers do not.

Focused tests passed 106 with two Windows-only skips. Full unit tests gave
368 pass, four previously reproduced baseline failures, two platform skips;
format/lint/compilation/diff checks passed. The transaction author is
repairing this residual with a full-before-projection regression.

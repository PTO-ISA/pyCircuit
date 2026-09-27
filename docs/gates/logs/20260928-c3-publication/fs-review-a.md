# C3 filesystem repair independent review A

Independent Sol high `code-reviewer`, 2026-09-28, reviewed exact filesystem
repair commit `2e6c4982` against approved C3-C. Two committed files have
content-manifest SHA-256
`f275ccc2e2058c6270d1535ede165c4572fd63a760b40af2a1cec878b2550eb0`.
Verdict: **REQUEST CHANGES**.

One high-severity finding: cross-parent transaction rename (`stage` ↔ target
and target ↔ `previous`) flushes only the destination parent; both changed
directories must be durable before reporting completion. The same concern
applies to any cross-parent replace unless that primitive is restricted to
same-directory metadata. One medium-severity finding: the Windows directory
flush rejects a reparse handle but does not compare the opened handle identity
with the current path before `FlushFileBuffers`.

Focused macOS tests passed 10/10 with two Windows-only cases skipped;
related source-unit tests passed 35 with two platform skips. Ruff, Python
compilation and diff checks passed. Windows runtime behavior remains
unvalidated. The filesystem owner is repairing both findings and adding
cross-parent and Windows identity oracles before re-review.

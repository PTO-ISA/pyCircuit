# Issue #63 OPT-03 same-snapshot Table match fusion

Generated gfsim C++ previously emitted two complete `entries` scans for WBA
`cancel_unpublished`: one mask for matching incomplete rows and one for matching
completed rows. The shared expression emitter now groups required inline
`table_match` expressions only when they name the same Table, carry identical
captured operands, use captures defined before the first match, contain only
plan-classified effect-free expressions, and own no snapshot-set reservation.
All other matches retain independent scans.

The fused loop preserves one mask per original match. It canonicalizes the
exact typed predicate DAG, rewrites operands to shared values, and creates fresh
internal names so local SSA identities from separate predicate regions cannot
collide. The WBA pair computes `entry.valid`, `entry.completed`, and `item.key`
once while retaining separate first selections and reservation footprints.
Array masks use the original word/index write, including index 64.

Negative coverage keeps different captures, different Tables, late captures,
nested TableGet, and snapshot-set effects on the old paths. Root-yield
predicates, duplicate local names, a dominating block-local capture, and a
65-entry high-word mask compile; the wide executable selects index 64 in its
first mask while the second mask remains invalid.

WBA behavior includes simultaneous completed/incomplete rows with the same key,
duplicate incomplete rows with distinguishable publication state, no match,
generation-sensitive identity, cancellation, apply/retry, drain and
backpressure. Both flat CLI and hierarchy-preserving generated C++ compile and
execute. The provisional Table family remains rejected by PYC/RTL under
Decision 0155.

For WBA H3, generated source changes from 237,663 to 232,221 bytes and
`table_entries` scans from 14 to 13. A five-run Apple clang `-O3` diagnostic
benchmark of the eight-entry predicate changes median time from 3.24676 to
2.50232 ns/iteration with identical checksums. These timings are not semantic
pass thresholds.

The isolated `uv` contracts command encountered two PyPI timeout failures. The
same contract script then passed with the current-checkout
`.pycircuit_out/contracts-venv`; no toolchain or artifact was copied from
another checkout.

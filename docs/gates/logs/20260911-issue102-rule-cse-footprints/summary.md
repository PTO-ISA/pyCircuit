# Issue 102 live rule-footprint evidence

Decision 0221 requires derived rule summaries to describe live IR. The
`ac-lower-rules` pipeline now runs CSE immediately after canonicalization and
before effect/footprint inference. Identical live `ac.table.get` operations are
therefore merged before `ac.rule_footprints` and `ac.state_accesses` are built;
the strict closure verifier remains unchanged.

The issue reproducer lowers to one live `ac.table.get @tail`, one matching read
footprint, and two write footprints. The full ACIR lit suite passes.

See `commands.txt` and the adjacent stdout/stderr/return-code files for exact
evidence.

# Stateful Issue Table

`issue.py` is the single public Python example in this directory. It combines:

- two field-disjoint operand wakeup writers;
- minimum-age selection and grant-driven read;
- scalar `valid` clear after issue;
- explicit empty-slot match/choose followed by complete Entry allocation;
- allocation backpressure while no old-state empty slot is available.

All Table expressions observe the old committed image. Consequently a wakeup
becomes selectable on the following tick, and a slot cleared by issue becomes
visible to the empty-slot selector on the following tick. Allocation remains a
state-driven scalar endpoint and replaces the complete selected Entry. The
example intentionally updates only entries resident when a wakeup is consumed.
It does not persist global tag-ready state or re-query later allocations, so it
is not the lost-wakeup solution tracked by issue #11.

The checked multi-writer Frozen ACIR fixture remains
`table_multi_writer_issue.mlir`. Additional Python inputs used only for E2E
regression coverage live under `tests/e2e/fixtures/table_examples/` and are not
public examples.

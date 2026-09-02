# Stateful Table scoreboard

`table_scoreboard.py` is the contract-epoch `0.4` vertical Table example. It
uses one zero-initialized, one-dimensional Table with a single patch writer,
one Queue-driven reader, and one state-driven reader.

The patch preserves unspecified fields by lowering to `ac.table.get` followed
by ordinary `ac.var.with` operations before `ac.table.write`. Reads observe the
old committed Entry when a write is proposed in the same tick; the update is
visible after tick commit. Queue backpressure keeps an already-produced Entry
stable.

This example targets the typed gfsim C++ provider. The PYC provider diagnoses
the provisional Table as unsupported.

`table_issue_slot.py` covers the provisional committed-request flow:
`ac.slot` capture, `table.match`, `table.choose(policy="min")`, a state-driven
patch, and release without same-cycle refill.

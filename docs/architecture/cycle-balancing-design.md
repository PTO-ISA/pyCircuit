# Cycle-balancing design history

This page formerly described automatic CycleAwareSignal delay insertion. That
feature and its authoring API are retired; the design notes below are not an
active implementation contract.

The supported profile has one default clock, finite scalar state, and
current/next semantics. It has no public logical-cycle tags, explicit cycle
domains, or automatic balancing API. A future timing feature needs its own
approved contract and independent verification. Do not restore the prior API
as a compatibility layer.

For current behavior, read the [language reference](../reference/language.md).
For accepted scope and candidate status, read the
[M5 migration guide](../development/m5-migration.md).

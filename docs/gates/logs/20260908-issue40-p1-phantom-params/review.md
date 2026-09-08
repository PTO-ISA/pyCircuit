# Issue #40 P1 phantom parameter removal

`Circuit.create_domain()` and `CycleAwareCircuit.create_domain()` now accept
only the domain name. `CycleAwareDomain.create_const()` accepts value, width,
and signedness. Removed frequency, reset-polarity, and constant-name keywords
raise Python `TypeError` instead of being silently ignored.

The specification assigns frequency and external reset polarity to integration
constraints. Signature and negative-keyword tests lock the hard break, while
existing JIT, direct builder, hierarchical, enum, and constant consumers remain
on the supported signatures.

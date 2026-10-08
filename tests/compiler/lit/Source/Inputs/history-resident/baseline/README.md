# Inert resident-queue baselines

These text assets retain exact Git bytes for the original two-instance circular
ROB, oldest-ready ISQ, and their strong integration tests. `manifest.json` records
the resolved revisions, original paths, and SHA-256 hashes. They are reference
assets for the current source implementation and independent host oracles;
the retired compilation routes in the historical test source are never run.

The strong ROB test compares every tick, including ten boundary queues, eight
scalar state values, eight entries, input transactions, and stale-completion
state-commit behavior. It covers output holding, full capacity, wraparound,
generations, recovery epochs, stale completion consumption, and both instances.

The strong ISQ test compares six boundary queues, eight entries, and 128
persistent readiness bits. It includes readiness/dispatch lost-wakeup cases,
full residents and blocked output, per-instance readiness, clearing a ready tag
on an issue opportunity, and concurrent unrelated readiness writes. Its exact
readiness snapshot-set competition must be retained; output order alone does
not establish equivalent behavior.

Restoring these assets does not establish current compiler or simulation
closure. Current-source admission and complete independent snapshot checks
must be recorded separately.

# F7 recovery and versioned identity

## Decision

- Decision 0279 defines explicit `TransactionRef(slot, generation,
  recovery_epoch)`, `ExecutionAttempt`, RecoveryDomain, RecoveryEvent, KillSet,
  VersionedTable, Checkpoint, and RetainedResult contracts.
- Identity is structural and explicit. No content/hash identity, backend-only
  predicate, compatibility path, or unqualified versioned write is admitted.
- The profile remains compiler-internal; no Python surface was added.

## Implementation

- ACIR adds closed transaction/attempt types and verified recovery, identity,
  checkpoint, retained-result, recovery-event, KillSet, qualified lookup, and
  qualified proposal operations.
- Versioned Tables require a complete nominal entry contract with distinct
  valid/generation/epoch/attempt/payload fields and exact symbolic bindings.
- QueueGraph retains the exact ref operands, action, bindings, and stable
  no-stale obligation ID in canonical JSON.
- gfsim C++ and PYC conjoin committed valid/generation/epoch/attempt equality
  before update or invalidation. Stale input commits consume-only.
- Retained-result `retain` writes only an invalid entry; duplicate acceptance
  is covered and cannot overwrite the held value. Matched `consume` releases
  the entry for the next execution attempt.
- PYC assertions use separate safety and cover conditions. C++ increments the
  exact stale-event coverage counter; RTL covers that same stale predicate.

## Reduced fixture

The two-entry fixture proves:

1. allocate `slot0/gen0/epoch4`;
2. reuse it as `slot0/gen1/epoch5`;
3. consume the old completion without changing payload `0x22`;
4. accept the fresh completion and publish `0xbb`;
5. lower a recovery event and `epoch_mismatch_or_younger` KillSet;
6. allocate `slot0/gen2/epoch6` after recovery;
7. reject the pre-recovery completion and preserve payload `0xcc`;
8. perform only generation/epoch/attempt-qualified lookup.

The sequence passes in generated PYC C++ and generated RTL (Icarus). Generated
gfsim C++ compiles, and Verilator accepts the RTL structure and SVA.

## Evidence

See `verification.log` and `decision_status_report.json` in this directory.

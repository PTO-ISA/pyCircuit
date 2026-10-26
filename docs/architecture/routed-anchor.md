# Routed ACIR anchor

The native compiler has a verified boundary between source transactions and
physical storage refinement. Both stages use ACIR and the existing module graph.
Public link produces hardware final by default, and both current emitters consume
that same final artifact. The optional [saved-anchor workflow](saved-anchor.md)
publishes and reloads the routed boundary independently. A high-level emitter and
general extern implementation bindings remain separate work.

## Stages and entrypoints

| Boundary | Native owner | Required contents |
| --- | --- | --- |
| Source | Source-unit admission and transaction analysis | Authenticated bodies/interfaces; source state, Methods and transactions may remain. |
| Routed | `ac-route-transactions`, `verifyRoutedPackage`, `ac-verify-routed` | Resolved typed hierarchy, normalized Rules, logical state/update pairs and complete routing, availability and effect evidence. |
| Hardware final | `ac-lower-transactions`, `verifyHardwarePackage` | Physical storage and verified pins; no logical state/update or unresolved source effects. |

A routed top-level package has `ac.stage = "routed"` and
`ac.routed_contract = 2 : i64`, or contract 3 when it has a verified
[selected Module entry boundary](entry-boundary.md). Contract 3 requires that
boundary; contract 2 forbids it. Version 1 and unknown or malformed markers
require rebuilding from source. The marker cannot appear in a source unit or on
a nested operation.
Hardware refinement preserves the marker while changing the stage to `final`.
Independently authored final hardware may omit routed provenance; a present
marker must still be valid. The local attribute hook validates its shape, while
standalone package preflight enforces root placement after parsing.

Contract 2 includes lifted lexical registration identities and authenticated
public-result return routes. Contract 1 artifacts must be regenerated; there is
no reader that ignores the new ordering or availability semantics.

The routing pass accepts an admitted merged source package. It does not replace
source-unit authentication. Repeating it on a routed package validates without
adding ports or rewriting the graph. Hardware refinement requires routed input;
source callers explicitly run routing first. A final package may be revalidated.

The internal `linkSourceUnits` API selects `SourceLinkTarget::Routed` or
`SourceLinkTarget::Hardware` (default). `SourceLinkStages` records
`beforeTransactions`, `routedTransactions` and `afterTransactions` only after the
entire requested link succeeds. Routed-only linking leaves the last string
empty. Saved publication adds its own versioned manifest, closed definition
inventory and independent loader over this native boundary.

## Logical state and candidate updates

`ac.state(clock, reset, initial) -> T` retains the exact payload, declaration
occurrence and committed value Q. One direct same-module operation binds its
candidate and intent:

```text
ac.state.update(Q, candidate, intent) : (T, T, Bits1) -> ()
```

The update has no results, regions or independent owner attributes. Q must be the
direct declaration result, not a type-compatible alias or another child's value.
Every logical declaration has exactly one update. An unwritten declaration binds
Q with false intent. Pairing is checked through Q's actual SSA users, including
the case where one hold update uses Q twice.

An update describes pending state; it does not mutate committed storage during
Work. False intent holds, and accepted true intent uses the candidate under the
existing sampled domain/reset policy. Candidate visibility, saved snapshots,
explicit ordering, conflicts, checks and observations keep their existing
contracts. A failed epoch commits no partial state, clock history or observations.

Bits, Enum, Struct and admitted Table payloads retain their existing type,
constraint and finite-layout requirements. Multiple occurrences of one definition
have independent state. Explicit DFF/DFFE, Queue and Memory instances remain
ordinary primitive instances; no occurrence may also own a second logical state.

Q is a temporal dependency cut. Its initializer and domains, update candidates
and intents, and dead combinational computations are still checked. A stateful
instance does not automatically cut its ready/backpressure dependencies.

## Routing and verification authority

The anchor reuses `ac.internal_ports`, transaction/resource wiring, sampled
admission and availability records. Original interfaces are exact prefixes;
every instance binds the complete extended signature. Routes identify actual
SSA endpoints and definition-relative occurrences. No serialized pointer,
preparation index or root-absolute identity supplies semantic authority.
Within each route category, suffix ordinals follow deterministic first discovery
through ordered source and occurrence/resource traversal. Lookup tables only
deduplicate; pointer/hash iteration cannot select serialized order. Identical
inputs reproduce the same routes. Reordering independent source declarations may
change private ordinals, without changing ownership or transaction behavior.

Transactions calling siblings can be normalized at their common ordinary
ancestor. Their definitions and storage allocations remain in place. A lifted
Rule carries `ac.registration_path`, `ac.lexical_registration` and
`ac.registration_owner`; its ordinary occurrence is reconstructed from the allocation
path and original registration using canonical call-expansion frames. The
standalone verifier resolves every allocation and checks the invocation prefix
against that identity. Receiver paths are relative to the routing definition;
successive calls may target different siblings.

`ac.transaction_returns` describes public result and availability routes. Each
incoming row identifies an owner path, lexical registration, public-result
ordinal and appended data/enable input pair. Each writer row binds a lifted
registration and lexical owner path to its public results. The enable must be
that Rule's whole fire result. Intermediate definitions relay the exact pair
through actual child operands; every incoming pair has one authenticated
producer. The selected root has no unbound incoming return inputs. Value
availability derives from this verified pair, not an independent validity flag.

Declaration order is closed before calls are mapped to actual instances, then
merged with explicit transaction order. Possible conditional calls take part
statically. Successor guards and bodies use candidate entry values gated by
whole predecessor fire and state write intent. Nonwriting intermediate
transactions preserve the latest eligible candidate. Ordering never selects a
winner between otherwise unordered conflicting writers.

Source bodies cannot provide these normalized records. Routed packages cannot
retain Methods/calls, transactions, state reads/writes, unresolved source branches
or unbound resource directions. The standalone verifier checks the closed graph,
nominal types, concrete layouts, ownership, update inventory, dependencies,
writer/admission proofs, resource configuration and observations without Python
or source-analysis objects.

A shared read-only state endpoint view derives logical state from declaration and
update, physical state from verified primitive pins, and Queue/Memory behavior
from their existing contracts. It is not an independently editable behavior tag.

Two transformation proofs remain separate from standalone legality:

1. Source-to-routed authentication checks the original invocation, snapshots,
   guards, candidate versions, effects, routing and generated operation inventory.
2. Routed-to-final authentication independently reconstructs SSA correspondence,
   checks every physical allocation/control splat and verifies unchanged content.
   Strict final verification then checks executable hardware completeness.

A standalone-valid graph can still be rejected because it differs from the
particular source being compiled. Each pass publishes a detached candidate only
after its own proofs succeed. Public link works on a detached graph and preserves
source inputs and published outputs if any stage fails. Two manually sequenced
passes do not roll back an earlier successful pass when a later pass fails.

Hardware-only graphs retain normal hardware verification limits; they do not
inherit transaction preparation limits merely by passing through the new stages.
Logical-state preparation/refinement retains its bounded work and allocation
checks. Missing physical storage declarations can leave a valid logical anchor,
but must reject requested hardware refinement without changing that anchor.

## Constrained ordinary extern declarations

A routed ordinary hardware import may have this exact declaration contract:

```text
ac.external_contract = {
  state = "none", effects = [], latency = 0 : i64,
  availability = "all_inputs", value_domain = "four_state", failure = "none"
}
```

All keys are required; extra keys and different values are unsupported. It
promises a deterministic total combinational function of declared inputs and
bound parameters, without hidden state/environment, effects or failure. The
ordinary import cannot also claim primitive or transaction semantics.

Its exact dependency summary describes payload dependence. Output availability
is the conjunction of all input availability, including inputs omitted from
payload dependencies; zero-input availability is true. Availability-only cycles
remain illegal even when no payload cycle exists.

This establishes declaration legality, not an implementation or binary ABI.
Current executable refinement/emission rejects an ordinary extern without a
supported implementation. No provider is loaded or called during graph
verification. Stateful/effectful externs and binding protocols remain outside
this first profile.

## Verification coverage

`ACIRRoutedAnchorTests` contains independent hand-authored local and standalone
oracles, including fresh-context reparsing, ownership/marker mutations, Q cuts,
dead cycles and extern availability. Existing transaction tests retain original
source-authentication mutations and separately exercise physical refinement.
Generated native and four-state RTL checks still use the same refined artifact.
See [testing and gates](../development/testing-and-gates.md) for candidate-bound
commands and evidence; this contract is not a claim that a particular run passed.

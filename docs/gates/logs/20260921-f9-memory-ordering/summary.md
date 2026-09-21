# F9 Core-local memory ordering

## Decision

- Decision 0281 freezes one typed ordering relation and one closed load
  disposition for Core-local memory ordering, and states explicitly that it
  reuses Decision 0279 identity, Decision 0280 lane algebra, and the existing
  `ac.dependency_set` and `ac.kill_set` mechanisms rather than adding a second
  identity, stale-set, or tracking mechanism. The identity input is a typed mask
  the consumer must derive from the Decision 0279 hierarchy; this bounded profile
  does not type-enforce that derivation, which is recorded as an open item.
- `ac.memory_order_edge` carries producer and consumer lane masks, an optional
  typed proof mask, and one closed kind in `{older_than, must_wait, may_bypass,
  must_forward, must_replay_if, visibility_before}`. The kind is a closed
  classification and the extent is the mask intersection. Edge kinds are not yet
  linked to the disposition endpoint, so `must_wait` dominance over `may_bypass`
  is a frozen requirement that this bounded profile does not enforce or test; it
  is recorded as an open item rather than claimed as implemented.
- `ac.load_disposition` produces exactly one of `{wait, bypass, forward, replay}`
  per qualified lane plus a separate stale mask. `bypass` requires the disjoint
  proof and requires the lane not to alias, `forward` requires alias with ready
  data on a not-yet-executed load, `replay` requires alias on an already
  executed load, and `wait` is the unproven remainder.
- A flush-invalidated outstanding load is stale regardless of its response
  identity, so a killed lane can never reach a live disposition.
- The closed obligation kind `no_stale_response` proves that the stale mask and
  every live disposition are disjoint and covers the stale event, carrying one
  stable ID through PYC, C++, and RTL.
- The profile stays compiler-internal. The address and alias function,
  store-buffer depth, coherence, replacement, and the ISA memory-consistency
  model stay in the consumer; the framework consumes typed masks and never
  infers them from a design name.

## Implementation

- ACIR adds the closed `MemoryOrderKind` attribute, `ac.memory_order_edge`, and
  `ac.load_disposition` with exact lane-width verification on every operand and
  result, including the lane invalidation mask.
- The QueueGraph plan extracts one typed expression per endpoint and re-verifies
  lane count, operand widths, closed kinds, and the disposition operand shape
  fail-closed before any backend emission.
- PYC, gfsim C++, and RTL lower the same algebra: `stale = pending &&
  (!identity || killed)`, `qualified = pending && !stale`, `forward = qualified
  && alias && ready && !executed`, `replay = qualified && alias && executed`,
  `bypass = qualified && disjoint && !alias`, and `wait` as the remainder.
- The new closed obligation kind `no_stale_response` is emitted once per
  disposition with the stable ID `no_stale_response:<anchor>:disposition<ordinal>`,
  so two dispositions in one rule cannot collide. PYC, generated PYC C++, and
  RTL carry the same ID, kind, severity, `pre_publish` sampling point, anchor,
  and message; the PYC C++ path gets a `_coverage` counter and RTL a cover
  property for the stale event. Because the live and stale masks are computed
  from the same qualified intermediate, the assertion condition is a structural
  self-consistency guard that cannot fail on its own; its value is the cover
  condition and the counter, and legality is enforced at compile time by the op
  verifier and the QueueGraph plan re-verification, not by the runtime
  assertion.

## Fixture and oracle

The reduced fixture executes in generated C++ and in Icarus RTL. Its oracle
derives `alias`, `disjoint`, and `data_ready` from a real address and store model
rather than from the DUT: an alias requires a resolved older store at the same
address, and a disjoint proof requires every older store to be resolved and at a
different address. The flush model likewise invalidates outstanding loads
independently. The disposition rule itself is the frozen specification restated
in the oracle, so an error inside that rule is caught by the executed value
comparison against three deliberately wrong rules during review rather than by
oracle independence. Both the C++ harness and the RTL testbench check the DUT's
own output for one live disposition per qualified lane and a stale-only
disposition otherwise, and the fixture fails when any disposition never occurs.

Six directed cases run on both sides and pin the boundaries: unknown address
waits, a resolved non-alias store discharges, an alias with ready data forwards,
an alias on an already executed load replays, an identity-mismatched response is
dropped, and a flush-invalidated outstanding load is consumed as stale even
though its response identity still matches.

Observed disposition counts on the final run: C++ `wait=45 bypass=68 forward=4
replay=5 stale=276`; Icarus `wait=83 bypass=73 forward=4 replay=5 stale=246`.
Every disposition is exercised on both sides.

## Known limits

- `ac.kill_set` is scalar (`!ac.var<i1>`) and cannot express a lane-vector
  flush, so this profile takes the invalidated lane mask as a typed input. A
  lane-vector kill form is an explicit open item and was not emulated by
  overloading an unrelated operation.
- The pending-load set is fixture/consumer-owned state in this profile; the
  framework does not add a memory queue and this is not a full ISA
  memory-consistency model.
- The gfsim C++ obligation path computes the stale predicate and the plan
  verifies the expression shape, but only the PYC C++ and RTL paths materialize
  the matched assertion and coverage, as in the earlier stages. The generated
  gfsim C++ is syntax-checked rather than executed, and Verilator is used for
  lint only; execution is the PYC C++ binary and Icarus.

## Verification

See `verification.log` for the exact commands and results.

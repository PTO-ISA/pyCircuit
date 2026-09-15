# Stateful Agentic Circuit examples

These public examples cover the supported state-authoring paths without also
serving as compiler regression fixtures. Focused branch, presence, arbitration,
and backend cases live under
`tests/integration/agentic-circuit/e2e/fixtures/state/`.

| Example | What it teaches |
| --- | --- |
| `table_rule.py` | Minimal explicit Table replacement committed atomically with Queue input and output. |
| `issue.py` | Explicit Table match/choose, field-disjoint updates, retained slot inputs, and old-image semantics. |
| `inferred_stateful_module.py` | A normal annotated Python variable becoming instance-local committed module state. |
| `inferred_nested_rule.py` | A nested rule capturing lexical state while repeated module placements stay isolated. |
| `reusable_circular_rob.py` | A reusable multi-rule ROB with heterogeneous atomic state, recovery, and stale-completion rejection. |
| `reusable_oldest_ready_isq.py` | Explicit Table `.find`, read-only state dependencies, and reusable oldest-ready scheduling. |
| `slot_rule_mailbox.py` | Explicit-argument and nested-capture forms of transactional rule-owned slot release. |

## State and transaction model

Rules and stateful modules observe one committed snapshot at activation. Their
selected Queue transfers, Table or lexical-state updates, outputs, and slot
releases prepare together and publish at the tick edge. Backpressure or a state
conflict leaves the complete transaction unchanged.

Slots remain topology-owned resources declared in a module or system. A rule
may read `slot.valid` and `slot.value`; exactly one rule or standalone endpoint
may own release. See `slot_rule_mailbox.py` for both supported module forms.

`issue.py` intentionally demonstrates resident-entry Table behavior rather
than a complete lost-wakeup solution. `reusable_oldest_ready_isq.py` shows the
Pythonic persistent-readiness design that handles wakeup-before-dispatch.

// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/no-initial.mlir 2>&1 | %FileCheck %s --check-prefix=NO-INITIAL
// RUN: %not %acir_opt %t/multiple-initial.mlir 2>&1 | %FileCheck %s --check-prefix=MULTI-INITIAL
// RUN: %not %acir_opt %t/unresolved-state.mlir 2>&1 | %FileCheck %s --check-prefix=STATE
// RUN: %not %acir_opt %t/unresolved-event.mlir 2>&1 | %FileCheck %s --check-prefix=EVENT
// RUN: %not %acir_opt %t/ambiguous.mlir 2>&1 | %FileCheck %s --check-prefix=AMBIG
// RUN: %not %acir_opt %t/duplicate-priority.mlir 2>&1 | %FileCheck %s --check-prefix=PRIORITY
// RUN: %not %acir_opt %t/impure-guard.mlir 2>&1 | %FileCheck %s --check-prefix=GUARD
// RUN: %not %acir_opt %t/bad-backpressure.mlir 2>&1 | %FileCheck %s --check-prefix=BACKPRESSURE
// RUN: %not %acir_opt %t/bad-ordering.mlir 2>&1 | %FileCheck %s --check-prefix=ORDERING
// RUN: %not %acir_opt %t/bad-delivery.mlir 2>&1 | %FileCheck %s --check-prefix=DELIVERY
// RUN: %not %acir_opt %t/bad-completion.mlir 2>&1 | %FileCheck %s --check-prefix=COMPLETION
// RUN: %not %acir_opt %t/unknown-guarantee.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %not %acir_opt %t/unstable-pending.mlir 2>&1 | %FileCheck %s --check-prefix=STABLE
// RUN: %not %acir_opt %t/bad-max-inflight.mlir 2>&1 | %FileCheck %s --check-prefix=INFLIGHT
// RUN: %not %acir_opt %t/correlation.mlir 2>&1 | %FileCheck %s --check-prefix=CORRELATION
// RUN: %not %acir_opt %t/lost-offer.mlir 2>&1 | %FileCheck %s --check-prefix=LOST
// RUN: %not %acir_opt %t/duplicate-state.mlir 2>&1 | %FileCheck %s --check-prefix=DUP-STATE
// RUN: %not %acir_opt %t/duplicate-event.mlir 2>&1 | %FileCheck %s --check-prefix=DUP-EVENT
// RUN: %not %acir_opt %t/event-role.mlir 2>&1 | %FileCheck %s --check-prefix=EVENT-ROLE
// RUN: %not %acir_opt %t/event-payload.mlir 2>&1 | %FileCheck %s --check-prefix=EVENT-PAYLOAD
// RUN: %not %acir_opt %t/event-action.mlir 2>&1 | %FileCheck %s --check-prefix=EVENT-ACTION
// RUN: %not %acir_opt %t/duplicate-guarantee.mlir 2>&1 | %FileCheck %s --check-prefix=DUP-GUARANTEE
// RUN: %not %acir_opt %t/retained-no-resolution.mlir 2>&1 | %FileCheck %s --check-prefix=NO-RESOLUTION
// RUN: %not %acir_opt %t/bad-correlation.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-CORRELATION
// RUN: %not %acir_opt %t/missing-correlation-field.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING-CORRELATION
// RUN: %not %acir_opt %t/custom-missing-contract.mlir 2>&1 | %FileCheck %s --check-prefix=CUSTOM
// RUN: %not %acir_opt %t/per-key-no-correlation.mlir 2>&1 | %FileCheck %s --check-prefix=PER-KEY
// RUN: %not %acir_opt %t/terminal-completion.mlir 2>&1 | %FileCheck %s --check-prefix=TERMINAL
// RUN: %not %acir_opt %t/unresolved-target.mlir 2>&1 | %FileCheck %s --check-prefix=TARGET
// RUN: %not %acir_opt %t/negative-priority.mlir 2>&1 | %FileCheck %s --check-prefix=NEGATIVE-PRIORITY

// NO-INITIAL: protocol requires exactly one initial state, found 0
// MULTI-INITIAL: protocol requires exactly one initial state, found 2
// STATE: unresolved transition source state '@missing'
// EVENT: unresolved transition event '@missing'
// AMBIG: overlapping transitions require explicit priority
// PRIORITY: overlapping transitions require unique priority
// GUARD: guard operation 'ac.type_scope' is not in the pure expression allowlist
// BACKPRESSURE: unsupported backpressure value 'stall'
// ORDERING: unsupported ordering value 'global'
// DELIVERY: unsupported delivery value 'maybe'
// COMPLETION: unsupported completion value 'eventually'
// UNKNOWN: unknown mandatory protocol guarantee 'magic'
// STABLE: retained pending offer requires stable_pending = true
// INFLIGHT: max_inflight requires a positive i64 value
// CORRELATION: max_inflight greater than one requires correlation
// LOST: offer transition must transfer or retain ownership
// DUP-STATE: redefinition of symbol named 's'
// DUP-EVENT: redefinition of symbol named 'e'
// EVENT-ROLE: unresolved event source role '@missing'
// EVENT-PAYLOAD: event payload type must be a normative ACIR value type
// EVENT-ACTION: unsupported event action 'drop'
// DUP-GUARANTEE: duplicate protocol guarantee 'ordering'
// NO-RESOLUTION: pending ownership reaches state '@pending' with no outgoing transition
// BAD-CORRELATION: correlation requires a non-empty field name
// MISSING-CORRELATION: correlation field 'missing' is missing from reachable offer/response payload
// CUSTOM: custom backpressure requires a custom_backpressure declaration
// PER-KEY: per_key ordering requires correlation
// TERMINAL: on_terminal_phase completion requires a reachable terminal state
// TARGET: unresolved transition target state '@missing'
// NEGATIVE-PRIORITY: transition priority must be a non-negative i64 value

//--- no-initial.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = false, terminal = true}> : () -> ()
  }) : () -> ()
}

//--- multiple-initial.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "a", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "b", initial = true, terminal = false}> : () -> ()
  }) : () -> ()
}

//--- unresolved-state.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.transition"() <{source = @missing, target = @s, event = @e}> ({}) : () -> ()
  }) : () -> ()
}

//--- unresolved-event.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @missing}> ({}) : () -> ()
  }) : () -> ()
}

//--- ambiguous.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e}> ({}) : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e}> ({}) : () -> ()
  }) : () -> ()
}

//--- duplicate-priority.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e, priority = 1 : i64}> ({}) : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e, priority = 1 : i64}> ({}) : () -> ()
  }) : () -> ()
}

//--- impure-guard.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e}> ({
      "ac.type_scope"() <{sym_name = "side_effect"}> ({}) : () -> ()
    }) : () -> ()
  }) : () -> ()
}

//--- bad-backpressure.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "backpressure", value = "stall"}> : () -> ()
  }) : () -> ()
}

//--- bad-ordering.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "ordering", value = "global"}> : () -> ()
  }) : () -> ()
}

//--- bad-delivery.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "delivery", value = "maybe"}> : () -> ()
  }) : () -> ()
}

//--- bad-completion.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "completion", value = "eventually"}> : () -> ()
  }) : () -> ()
}

//--- unknown-guarantee.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "magic", value = "unknown"}> : () -> ()
  }) : () -> ()
}

//--- unstable-pending.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e, retain = true}> ({}) : () -> ()
  }) : () -> ()
}

//--- bad-max-inflight.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "max_inflight", value = 0 : i64}> : () -> ()
  }) : () -> ()
}

//--- correlation.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "max_inflight", value = 2 : i64}> : () -> ()
  }) : () -> ()
}

//--- lost-offer.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e}> ({}) : () -> ()
  }) : () -> ()
}

//--- duplicate-state.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = false, terminal = true}> : () -> ()
  }) : () -> ()
}

//--- duplicate-event.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @b, to = @a, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
}

//--- event-role.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @missing, to = @b, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
}

//--- event-payload.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = !ac.endpoint<@I, @a>, action = "notify"}> : () -> ()
  }) : () -> ()
}

//--- event-action.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "drop"}> : () -> ()
  }) : () -> ()
}

//--- duplicate-guarantee.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "ordering", value = "fifo"}> : () -> ()
    "ac.guarantee"() <{kind = "ordering", value = "unordered"}> : () -> ()
  }) : () -> ()
}

//--- retained-no-resolution.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "pending", initial = false, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.transition"() <{source = @s, target = @pending, event = @e, retain = true}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "stable_pending", value = true}> : () -> ()
  }) : () -> ()
}

//--- bad-correlation.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "correlation", value = ""}> : () -> ()
  }) : () -> ()
}

//--- missing-correlation-field.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "tag", type = i8}]}> : () -> ()
  }) : () -> ()
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = !ac.transaction<@types::@T>, action = "offer"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e, transfer = true}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "correlation", value = "missing"}> : () -> ()
  }) : () -> ()
}

//--- custom-missing-contract.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "backpressure", value = "custom"}> : () -> ()
  }) : () -> ()
}

//--- per-key-no-correlation.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "ordering", value = "per_key"}> : () -> ()
  }) : () -> ()
}

//--- terminal-completion.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.guarantee"() <{kind = "completion", value = "on_terminal_phase"}> : () -> ()
  }) : () -> ()
}

//--- unresolved-target.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @missing, event = @e}> ({}) : () -> ()
  }) : () -> ()
}

//--- negative-priority.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @e, priority = -1 : i64}> ({}) : () -> ()
  }) : () -> ()
}

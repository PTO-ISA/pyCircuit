// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/transfer-retain.mlir 2>&1 | %FileCheck %s --check-prefix=TRANSFER-RETAIN
// RUN: %not %acir_opt %t/orphan-transfer.mlir 2>&1 | %FileCheck %s --check-prefix=ORPHAN
// RUN: %not %acir_opt %t/retry-no-retain.mlir 2>&1 | %FileCheck %s --check-prefix=RETRY
// RUN: %not %acir_opt %t/drop-branch.mlir 2>&1 | %FileCheck %s --check-prefix=DROP
// RUN: %not %acir_opt %t/pending-terminal.mlir 2>&1 | %FileCheck %s --check-prefix=PENDING-TERMINAL
// RUN: %not %acir_opt %t/conflicting-join.mlir 2>&1 | %FileCheck %s --check-prefix=JOIN
// RUN: %not %acir_opt %t/scf-guard.mlir 2>&1 | %FileCheck %s --check-prefix=SCF
// RUN: %not %acir_opt %t/unknown-pure-guard.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN-GUARD
// RUN: %not %acir_opt %t/container-guard.mlir 2>&1 | %FileCheck %s --check-prefix=CONTAINER-GUARD
// RUN: %not %acir_opt %t/interface-child.mlir 2>&1 | %FileCheck %s --check-prefix=INTERFACE-CHILD
// RUN: %not %acir_opt %t/protocol-child.mlir 2>&1 | %FileCheck %s --check-prefix=PROTOCOL-CHILD
// RUN: %not %acir_opt %t/dual-cardinality.mlir 2>&1 | %FileCheck %s --check-prefix=DUAL-CARDINALITY
// RUN: %not %acir_opt %t/event-same-role.mlir 2>&1 | %FileCheck %s --check-prefix=EVENT-DIRECTION
// RUN: %not %acir_opt %t/event-target.mlir 2>&1 | %FileCheck %s --check-prefix=EVENT-TARGET

// TRANSFER-RETAIN: transition cannot both transfer and retain ownership
// ORPHAN: ownership transfer requires a pending offer
// RETRY: retry transition must retain the pending offer
// DROP: pending ownership reaches state '@drop' with no outgoing transition
// PENDING-TERMINAL: terminal state '@done' is reachable with pending ownership
// JOIN: ownership state conflict at join state '@join'
// SCF: guard operation 'scf.yield' is not in the pure expression allowlist
// UNKNOWN-GUARD: guard operation 'builtin.unrealized_conversion_cast' is not in the pure expression allowlist
// CONTAINER-GUARD: guard operation 'ac.type_scope' is not in the pure expression allowlist
// INTERFACE-CHILD: interface body only permits ac.role and ac.port, found ac.state
// PROTOCOL-CHILD: protocol body contains unsupported operation ac.port
// DUAL-CARDINALITY: dual roles must have matching cardinality
// EVENT-DIRECTION: event source and target roles must differ
// EVENT-TARGET: unresolved event target role '@missing'

//--- transfer-retain.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "offer", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @offer, transfer = true, retain = true}> ({}) : () -> ()
  }) : () -> ()
}

//--- orphan-transfer.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "done", initial = false, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "accept", from = @b, to = @a, payload = i8, action = "accept"}> : () -> ()
    "ac.transition"() <{source = @s, target = @done, event = @accept, transfer = true}> ({}) : () -> ()
  }) : () -> ()
}

//--- retry-no-retain.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "idle", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "pending", initial = false, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "offer", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.event"() <{sym_name = "retry", from = @a, to = @b, payload = i8, action = "retry"}> : () -> ()
    "ac.transition"() <{source = @idle, target = @pending, event = @offer, retain = true}> ({}) : () -> ()
    "ac.transition"() <{source = @pending, target = @pending, event = @retry}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "stable_pending", value = true}> : () -> ()
  }) : () -> ()
}

//--- drop-branch.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "idle", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "pending", initial = false, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "drop", initial = false, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "done", initial = false, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "offer", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.event"() <{sym_name = "cancel", from = @a, to = @b, payload = i8, action = "cancel"}> : () -> ()
    "ac.event"() <{sym_name = "tick", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @idle, target = @pending, event = @offer, retain = true}> ({}) : () -> ()
    "ac.transition"() <{source = @pending, target = @done, event = @cancel}> ({}) : () -> ()
    "ac.transition"() <{source = @pending, target = @drop, event = @tick}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "stable_pending", value = true}> : () -> ()
  }) : () -> ()
}

//--- pending-terminal.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "idle", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "done", initial = false, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "offer", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.transition"() <{source = @idle, target = @done, event = @offer, retain = true}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "stable_pending", value = true}> : () -> ()
  }) : () -> ()
}

//--- conflicting-join.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "idle", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "join", initial = false, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "done", initial = false, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "offer", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.event"() <{sym_name = "tick", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.event"() <{sym_name = "cancel", from = @a, to = @b, payload = i8, action = "cancel"}> : () -> ()
    "ac.transition"() <{source = @idle, target = @join, event = @offer, retain = true}> ({}) : () -> ()
    "ac.transition"() <{source = @idle, target = @join, event = @tick}> ({}) : () -> ()
    "ac.transition"() <{source = @join, target = @done, event = @cancel}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "stable_pending", value = true}> : () -> ()
  }) : () -> ()
}

//--- scf-guard.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "tick", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @tick}> ({
      %true = "arith.constant"() <{value = true}> : () -> i1
      "scf.if"(%true) ({
        "scf.yield"() : () -> ()
      }, {
        "scf.yield"() : () -> ()
      }) : (i1) -> ()
    }) : () -> ()
  }) : () -> ()
}

//--- unknown-pure-guard.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "tick", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @tick}> ({
      %x = "builtin.unrealized_conversion_cast"() : () -> i1
    }) : () -> ()
  }) : () -> ()
}

//--- container-guard.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "tick", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @s, target = @s, event = @tick}> ({
      "ac.type_scope"() <{sym_name = "bad"}> ({}) : () -> ()
    }) : () -> ()
  }) : () -> ()
}

//--- interface-child.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.state"() <{sym_name = "bad", initial = true, terminal = false}> : () -> ()
  }) : () -> ()
}

//--- protocol-child.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.port"() <{sym_name = "bad", type = i8, from = @a, to = @b, protocol_from = @a, protocol_to = @b}> : () -> ()
  }) : () -> ()
}

//--- dual-cardinality.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "shared"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
  }) : () -> ()
}

//--- event-same-role.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @a, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
}

//--- event-target.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @missing, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
}

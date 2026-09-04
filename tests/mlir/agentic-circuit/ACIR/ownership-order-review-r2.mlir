// RUN: %split_file %s %t
// RUN: %acir_opt %t/forward.mlir > %t/forward.out
// RUN: %acir_opt %t/reverse.mlir > %t/reverse.out
// RUN: %acir_opt %t/legal-cycle.mlir > /dev/null
// RUN: %not %acir_opt %t/illegal-cycle.mlir 2>&1 | %FileCheck %s --check-prefix=CYCLE
// RUN: %not %acir_opt %t/conflict-forward.mlir 2>&1 | %FileCheck %s --check-prefix=CONFLICT
// RUN: %not %acir_opt %t/conflict-reverse.mlir 2>&1 | %FileCheck %s --check-prefix=CONFLICT

// CONFLICT: ownership state conflict at join state '@join'
// CYCLE: ownership state conflict at join state '@start'

//--- forward.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s0", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "s1", initial = false, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "s2", initial = false, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "step", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.transition"() <{source = @s0, target = @s1, event = @step}> ({}) : () -> ()
    "ac.transition"() <{source = @s1, target = @s2, event = @step}> ({}) : () -> ()
  }) : () -> ()
}

//--- reverse.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s0", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "s1", initial = false, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "s2", initial = false, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "step", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.transition"() <{source = @s1, target = @s2, event = @step}> ({}) : () -> ()
    "ac.transition"() <{source = @s0, target = @s1, event = @step}> ({}) : () -> ()
  }) : () -> ()
}

//--- legal-cycle.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s0", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "s1", initial = false, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "step", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.transition"() <{source = @s1, target = @s0, event = @step}> ({}) : () -> ()
    "ac.transition"() <{source = @s0, target = @s1, event = @step}> ({}) : () -> ()
  }) : () -> ()
}

//--- conflict-forward.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "start", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "pending", initial = false, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "clear", initial = false, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "join", initial = false, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "offer", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.event"() <{sym_name = "step", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @start, target = @pending, event = @offer, retain = true}> ({}) : () -> ()
    "ac.transition"() <{source = @start, target = @clear, event = @step}> ({}) : () -> ()
    "ac.transition"() <{source = @pending, target = @join, event = @step}> ({}) : () -> ()
    "ac.transition"() <{source = @clear, target = @join, event = @step}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "stable_pending", value = true}> : () -> ()
  }) : () -> ()
}

//--- illegal-cycle.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "start", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "pending", initial = false, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "offer", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.event"() <{sym_name = "step", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @start, target = @pending, event = @offer, retain = true}> ({}) : () -> ()
    "ac.transition"() <{source = @pending, target = @start, event = @step}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "stable_pending", value = true}> : () -> ()
  }) : () -> ()
}

//--- conflict-reverse.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "start", initial = true, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "pending", initial = false, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "clear", initial = false, terminal = false}> : () -> ()
    "ac.state"() <{sym_name = "join", initial = false, terminal = false}> : () -> ()
    "ac.event"() <{sym_name = "offer", from = @a, to = @b, payload = i8, action = "offer"}> : () -> ()
    "ac.event"() <{sym_name = "step", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
    "ac.transition"() <{source = @clear, target = @join, event = @step}> ({}) : () -> ()
    "ac.transition"() <{source = @pending, target = @join, event = @step}> ({}) : () -> ()
    "ac.transition"() <{source = @start, target = @clear, event = @step}> ({}) : () -> ()
    "ac.transition"() <{source = @start, target = @pending, event = @offer, retain = true}> ({}) : () -> ()
    "ac.guarantee"() <{kind = "stable_pending", value = true}> : () -> ()
  }) : () -> ()
}

// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/missing.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING
// RUN: %not %acir_opt %t/missing-to.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING-TO
// RUN: %not %acir_opt %t/unresolved.mlir 2>&1 | %FileCheck %s --check-prefix=UNRESOLVED
// RUN: %not %acir_opt %t/nondual.mlir 2>&1 | %FileCheck %s --check-prefix=NONDUAL
// RUN: %not %acir_opt %t/cardinality.mlir 2>&1 | %FileCheck %s --check-prefix=CARDINALITY

// MISSING: requires attribute 'protocol_from'
// MISSING-TO: requires attribute 'protocol_to'
// UNRESOLVED: unresolved mapped protocol source role '@missing'
// NONDUAL: mapped protocol roles must be dual
// CARDINALITY: interface and mapped protocol roles must have matching cardinality

//--- missing.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @a, to = @b}> : () -> ()
  }) : () -> ()
}

//--- missing-to.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @a, to = @b, protocol_from = @a}> : () -> ()
  }) : () -> ()
}

//--- unresolved.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "source", dual = @sink, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "sink", dual = @source, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @source, to = @sink, protocol_from = @missing, protocol_to = @b}> : () -> ()
  }) : () -> ()
}

//--- nondual.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "c", dual = @d, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "d", dual = @c, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @a, to = @b, protocol_from = @a, protocol_to = @c}> : () -> ()
  }) : () -> ()
}

//--- cardinality.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "exclusive"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "exclusive"}> : () -> ()
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.event"() <{sym_name = "e", from = @a, to = @b, payload = i8, action = "notify"}> : () -> ()
  }) : () -> ()
  "ac.interface"() <{sym_name = "I"}> ({
    "ac.role"() <{sym_name = "a", dual = @b, cardinality = "shared"}> : () -> ()
    "ac.role"() <{sym_name = "b", dual = @a, cardinality = "shared"}> : () -> ()
    "ac.port"() <{sym_name = "x", type = !ac.channel<i8, @p>, from = @a, to = @b, protocol_from = @a, protocol_to = @b}> : () -> ()
  }) : () -> ()
}

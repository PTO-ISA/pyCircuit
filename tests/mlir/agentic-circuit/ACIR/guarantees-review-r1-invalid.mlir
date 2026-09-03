// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/string-kind.mlir 2>&1 | %FileCheck %s --check-prefix=STRING
// RUN: %not %acir_opt %t/stable.mlir 2>&1 | %FileCheck %s --check-prefix=STABLE
// RUN: %not %acir_opt %t/inflight.mlir 2>&1 | %FileCheck %s --check-prefix=INFLIGHT
// RUN: %not %acir_opt %t/correlation.mlir 2>&1 | %FileCheck %s --check-prefix=CORRELATION
// RUN: %not %acir_opt %t/custom.mlir 2>&1 | %FileCheck %s --check-prefix=CUSTOM

// STRING: unsupported backpressure value '<non-string>'
// STABLE: stable_pending requires a boolean value
// INFLIGHT: max_inflight requires a positive i64 value
// CORRELATION: correlation requires a non-empty field name
// CUSTOM: custom_backpressure requires a non-empty declarative contract

//--- string-kind.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "backpressure", value = 1 : i64}> : () -> ()
  }) : () -> ()
}

//--- stable.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "stable_pending", value = 1 : i64}> : () -> ()
  }) : () -> ()
}

//--- inflight.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "max_inflight", value = "many"}> : () -> ()
  }) : () -> ()
}

//--- correlation.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "correlation", value = 1 : i64}> : () -> ()
  }) : () -> ()
}

//--- custom.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.protocol"() <{sym_name = "p"}> ({
    "ac.state"() <{sym_name = "s", initial = true, terminal = true}> : () -> ()
    "ac.guarantee"() <{kind = "custom_backpressure", value = true}> : () -> ()
  }) : () -> ()
}

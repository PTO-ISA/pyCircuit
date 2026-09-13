// RUN: %split_file %s %t
// RUN: %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/valid.mlir | %FileCheck %s --check-prefix=VALID
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/bad-bounds.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-BOUNDS
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/bad-constant.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-CONSTANT
// RUN: %not %acir_opt --pass-pipeline='builtin.module(verify-ac-file)' %t/bad-ingress.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-INGRESS
// RUN: %not %acir_opt %t/bad-checked.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-CHECKED
// RUN: %not %acir_opt %t/bad-add.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-ADD
// RUN: %not %acir_opt %t/bad-sub.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-SUB
// RUN: %not %acir_opt %t/add-overflow.mlir 2>&1 | %FileCheck %s --check-prefix=ADD-OVERFLOW
// RUN: %not %acir_opt %t/bad-range-bits.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-RANGE-BITS
// RUN: %not %acir_opt %t/bad-dynamic-array-bound.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-ARRAY-BOUND
// RUN: %acir_opt %t/unproven-dynamic-array.mlir -ac-verify-value-constraints -verify-diagnostics
// RUN: %acir_opt %t/range-refine-proof.mlir -ac-verify-value-constraints -verify-diagnostics
// RUN: %not %acir_opt %t/bad-table-bound.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-TABLE-BOUND
// RUN: %not %acir_opt %t/bad-nested-zero.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-NESTED-ZERO
// RUN: %not %acir_opt %t/bad-table-zero.mlir 2>&1 | %FileCheck %s --check-prefix=BAD-TABLE-ZERO

// VALID: !ac.range<0, 4>
// VALID: !ac.range<4, 8>
// BAD-BOUNDS: range lower bound must not exceed upper bound
// BAD-CONSTANT: range constant must use its storage width and lie within bounds
// BAD-INGRESS: external source cannot carry an undecoded bounded range
// BAD-CHECKED: checked range validity must be !ac.var<i1>
// BAD-ADD: bounded arithmetic result range is inconsistent
// BAD-SUB: bounded subtraction may produce a negative result
// ADD-OVERFLOW: bounded addition exceeds the u64 domain
// BAD-RANGE-BITS: range_bits result must be the exact unsigned storage width
// BAD-ARRAY-BOUND: bounded index exceeds the value_array length
// BAD-TABLE-BOUND: bounded table index exceeds the Table domain
// BAD-NESTED-ZERO: init must match value type or be the zero image for a struct or enum
// BAD-TABLE-ZERO: zero-initialized Table entry type does not admit a zero image

//--- valid.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "range_types"} {
  func.func private @constants(%raw: !ac.var<i8>) -> (!ac.var<!ac.range<0, 4>>, !ac.var<!ac.range<4, 8>>) {
    %index = ac.var.constant 4 : i3 as !ac.var<!ac.range<0, 4>>
    %window = ac.var.constant 8 : i4 as !ac.var<!ac.range<4, 8>>
    %single = ac.var.constant 0 : i1 as !ac.var<!ac.range<0, 0>>
    %wrapped = ac.var.range_wrap %raw : !ac.var<i8> -> !ac.var<!ac.range<0, 4>>
    %saturated = ac.var.range_saturate %raw : !ac.var<i8> -> !ac.var<!ac.range<4, 8>>
    %checked, %valid = ac.var.range_checked %raw : !ac.var<i8> -> !ac.var<!ac.range<0, 4>>, !ac.var<i1>
    %bits = ac.var.range_bits %checked : !ac.var<!ac.range<0, 4>> -> !ac.var<i3>
    %refined = ac.var.range_refine %bits : !ac.var<i3> -> !ac.var<!ac.range<0, 7>>
    %one = ac.var.constant 1 : i1 as !ac.var<!ac.range<1, 1>>
    %advanced = ac.var.range_add %wrapped, %one : !ac.var<!ac.range<0, 4>>, !ac.var<!ac.range<1, 1>> -> !ac.var<!ac.range<1, 5>>
    %back = ac.var.range_sub %advanced, %one : !ac.var<!ac.range<1, 5>>, !ac.var<!ac.range<1, 1>> -> !ac.var<!ac.range<0, 4>>
    %ordered = ac.var.range_cmp "ult" %back, %saturated : !ac.var<!ac.range<0, 4>>, !ac.var<!ac.range<4, 8>> -> !ac.var<i1>
    %a = ac.var.constant 1 : i8 as !ac.var<i8>
    %array = ac.var.array %a, %a, %a, %a, %a : !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8> -> !ac.var<!ac.value_array<5 x i8>>
    %selected = ac.var.dynamic_element %array at %checked : !ac.var<!ac.value_array<5 x i8>>, !ac.var<!ac.range<0, 4>> -> !ac.var<i8>
    return %index, %window : !ac.var<!ac.range<0, 4>>, !ac.var<!ac.range<4, 8>>
  }
}

//--- bad-bounds.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "range_types"} {
  func.func private @bad() -> !ac.var<!ac.range<5, 4>> {
    %value = ac.var.constant 4 : i3 as !ac.var<!ac.range<5, 4>>
    return %value : !ac.var<!ac.range<5, 4>>
  }
}

//--- bad-constant.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "range_types"} {
  func.func private @bad() -> !ac.var<!ac.range<0, 4>> {
    %value = ac.var.constant 5 : i3 as !ac.var<!ac.range<0, 4>>
    return %value : !ac.var<!ac.range<0, 4>>
  }
}

//--- bad-ingress.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "range_types"} {
  ac.type_scope @types {
    ac.struct @Nested fields [{name = "values", type = !ac.value_array<2 x tuple<i8, !ac.range<0, 4>>>}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@Nested> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 3 : i64}>}
  %input = ac.source depth 1 latency 1 {ac.name = "input"} : !ac.queue<!ac.struct<@types::@Nested>>
  ac.sink %input {ac.name = "sink"} : !ac.queue<!ac.struct<@types::@Nested>>
}

//--- bad-checked.mlir
module attributes {ac.contract_epoch = "0.5"} {
  func.func private @bad(%raw: !ac.var<i8>) {
    %value, %valid = ac.var.range_checked %raw : !ac.var<i8> -> !ac.var<!ac.range<0, 4>>, !ac.var<i2>
    return
  }
}

//--- bad-add.mlir
module attributes {ac.contract_epoch = "0.5"} {
  func.func private @bad(%left: !ac.var<!ac.range<0, 4>>, %right: !ac.var<!ac.range<1, 1>>) {
    %value = ac.var.range_add %left, %right : !ac.var<!ac.range<0, 4>>, !ac.var<!ac.range<1, 1>> -> !ac.var<!ac.range<0, 4>>
    return
  }
}

//--- bad-sub.mlir
module attributes {ac.contract_epoch = "0.5"} {
  func.func private @bad(%left: !ac.var<!ac.range<0, 4>>, %right: !ac.var<!ac.range<1, 1>>) {
    %value = ac.var.range_sub %left, %right : !ac.var<!ac.range<0, 4>>, !ac.var<!ac.range<1, 1>> -> !ac.var<!ac.range<0, 3>>
    return
  }
}

//--- add-overflow.mlir
module attributes {ac.contract_epoch = "0.5"} {
  func.func private @bad(%left: !ac.var<!ac.range<18446744073709551614, 18446744073709551615>>, %right: !ac.var<!ac.range<1, 1>>) {
    %value = ac.var.range_add %left, %right : !ac.var<!ac.range<18446744073709551614, 18446744073709551615>>, !ac.var<!ac.range<1, 1>> -> !ac.var<!ac.range<0, 0>>
    return
  }
}

//--- bad-range-bits.mlir
module attributes {ac.contract_epoch = "0.5"} {
  func.func private @bad(%value: !ac.var<!ac.range<4, 8>>) {
    %bits = ac.var.range_bits %value : !ac.var<!ac.range<4, 8>> -> !ac.var<i3>
    return
  }
}

//--- bad-dynamic-array-bound.mlir
module attributes {ac.contract_epoch = "0.5"} {
  func.func private @bad(%array: !ac.var<!ac.value_array<5 x i8>>, %index: !ac.var<!ac.range<0, 5>>) {
    %value = ac.var.dynamic_element %array at %index : !ac.var<!ac.value_array<5 x i8>>, !ac.var<!ac.range<0, 5>> -> !ac.var<i8>
    return
  }
}

//--- unproven-dynamic-array.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "unsafe_array"} {
  %input = ac.source depth 1 latency 1 : !ac.queue<i3>
  %output = ac.transform %input depths [1] latencies [1] {
  ^body(%index: !ac.var<i3>):
    %a = ac.var.constant 1 : i8 as !ac.var<i8>
    %array = ac.var.array %a, %a, %a, %a, %a : !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8>, !ac.var<i8> -> !ac.var<!ac.value_array<5 x i8>>
    // expected-error @+1 {{cannot prove value_array index is within [0, 4]; inferred interval[0,7]}}
    %value = ac.var.dynamic_element %array at %index : !ac.var<!ac.value_array<5 x i8>>, !ac.var<i3> -> !ac.var<i8>
    ac.transform.yield %value : !ac.var<i8>
  } : (!ac.queue<i3>) -> !ac.queue<i8>
  ac.sink %output : !ac.queue<i8>
}

//--- bad-table-bound.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "bad_table"} {
  ac.table @state entry i8 entries 5 init 0 owner "/" stable_id "table/state"
  %index = ac.var.constant 0 : i3 as !ac.var<!ac.range<0, 5>>
  %value = ac.table.get @state[%index] : !ac.var<!ac.range<0, 5>> -> !ac.var<i8>
}

//--- range-refine-proof.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "refine"} {
  %safe = ac.source depth 1 latency 1 : !ac.queue<i2>
  %safe_output = ac.transform %safe depths [1] latencies [1] {
  ^body(%raw: !ac.var<i2>):
    %value = ac.var.range_refine %raw : !ac.var<i2> -> !ac.var<!ac.range<0, 4>>
    ac.transform.yield %value : !ac.var<!ac.range<0, 4>>
  } : (!ac.queue<i2>) -> !ac.queue<!ac.range<0, 4>>
  ac.sink %safe_output : !ac.queue<!ac.range<0, 4>>

  %unsafe = ac.source depth 1 latency 1 : !ac.queue<i3>
  %unsafe_output = ac.transform %unsafe depths [1] latencies [1] {
  ^body(%raw: !ac.var<i3>):
    // expected-error @+1 {{cannot prove strict range refinement is within [0, 4]; inferred interval[0,7]}}
    %value = ac.var.range_refine %raw : !ac.var<i3> -> !ac.var<!ac.range<0, 4>>
    ac.transform.yield %value : !ac.var<!ac.range<0, 4>>
  } : (!ac.queue<i3>) -> !ac.queue<!ac.range<0, 4>>
  ac.sink %unsafe_output : !ac.queue<!ac.range<0, 4>>
}

//--- bad-nested-zero.mlir
module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.struct @State fields [{name = "window", type = !ac.range<4, 8>}]
  } {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@State> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}>}
  ac.var.decl @state type !ac.struct<@types::@State> init 0 : i64 owner "/" stable_id "var/state"
}

//--- bad-table-zero.mlir
module attributes {ac.contract_epoch = "0.5", ac.model_kind = "queue_graph", ac.queue_graph_domain = "cycle", ac.system = "bad_table_zero"} {
  ac.table @state entry !ac.range<4, 8> entries 5 init 0 owner "/" stable_id "table/state"
}

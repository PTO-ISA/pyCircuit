// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/constant.mlir 2>&1 | %FileCheck %s --check-prefix=CONSTANT
// RUN: %not %acir_opt %t/binary.mlir 2>&1 | %FileCheck %s --check-prefix=BINARY
// RUN: %not %acir_opt %t/get-field.mlir 2>&1 | %FileCheck %s --check-prefix=GET-FIELD
// RUN: %not %acir_opt %t/get-result.mlir 2>&1 | %FileCheck %s --check-prefix=GET-RESULT
// RUN: %not %acir_opt %t/with-result.mlir 2>&1 | %FileCheck %s --check-prefix=WITH-RESULT
// RUN: %not %acir_opt %t/with-value.mlir 2>&1 | %FileCheck %s --check-prefix=WITH-VALUE
// RUN: %not %acir_opt %t/sub-nonnumeric.mlir 2>&1 | %FileCheck %s --check-prefix=SUB-NONNUMERIC
// RUN: %not %acir_opt %t/mul-nonnumeric.mlir 2>&1 | %FileCheck %s --check-prefix=MUL-NONNUMERIC
// RUN: %not %acir_opt %t/cmp-predicate.mlir 2>&1 | %FileCheck %s --check-prefix=CMP-PREDICATE
// RUN: %not %acir_opt %t/popcount-width.mlir 2>&1 | %FileCheck %s --check-prefix=POPCOUNT-WIDTH
// RUN: %not %acir_opt %t/popcount-input.mlir 2>&1 | %FileCheck %s --check-prefix=POPCOUNT-INPUT

// CONSTANT: error: 'ac.var.constant' op attribute type must match Var element type
// BINARY: error: use of value '%right' expects different type than prior uses
// GET-FIELD: error: 'ac.var.get' op unknown field 'missing'
// GET-RESULT: error: 'ac.var.get' op field 'value' result must be '!ac.var<i64>'
// WITH-RESULT: error: 'ac.var.with' op must preserve record Var identity
// WITH-VALUE: error: 'ac.var.with' op field 'value' expects '!ac.var<i64>'
// SUB-NONNUMERIC: error: 'ac.var.sub' op arithmetic Var element must be an integer or float
// MUL-NONNUMERIC: error: 'ac.var.mul' op arithmetic Var element must be an integer or float
// CMP-PREDICATE: error: 'ac.var.cmp' op predicate must be eq, ne, slt, sle, sgt, or sge
// POPCOUNT-WIDTH: error: 'ac.var.popcount' op result width must be ceil(log2(input_width + 1)) = 4
// POPCOUNT-INPUT: error: 'ac.var.popcount' op input width must be in [1, 64]

//--- constant.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %bad = ac.var.constant 1 : i16 as !ac.var<i32>
}

//--- sub-nonnumeric.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.optional<i32>>
  %bad = ac.var.sub %value, %value : !ac.var<!ac.optional<i32>>
}

//--- mul-nonnumeric.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.optional<i32>>
  %bad = ac.var.mul %value, %value : !ac.var<!ac.optional<i32>>
}

//--- popcount-width.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<i8>
  %bad = ac.var.popcount %value : !ac.var<i8> -> !ac.var<i3>
}

//--- popcount-input.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %value = "builtin.unrealized_conversion_cast"() : () -> !ac.var<i128>
  %bad = ac.var.popcount %value : !ac.var<i128> -> !ac.var<i8>
}

//--- cmp-predicate.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %left = ac.var.constant 1 : i64 as !ac.var<i64>
  %right = ac.var.constant 2 : i64 as !ac.var<i64>
  %bad = ac.var.cmp "random" %left, %right : !ac.var<i64> -> !ac.var<i1>
}

//--- binary.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %left = ac.var.constant 1 : i32 as !ac.var<i32>
  %right = ac.var.constant 1 : i16 as !ac.var<i16>
  %bad = ac.var.add %left, %right : !ac.var<i32>
}

//--- get-field.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "Item", fields = [{name = "value", type = i64}]}> : () -> ()
  }) : () -> ()
  %item = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.transaction<@types::@Item>>
  %bad = ac.var.get %item field "missing" : !ac.var<!ac.transaction<@types::@Item>> -> !ac.var<i64>
}

//--- get-result.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "Item", fields = [{name = "value", type = i64}]}> : () -> ()
  }) : () -> ()
  %item = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.transaction<@types::@Item>>
  %bad = ac.var.get %item field "value" : !ac.var<!ac.transaction<@types::@Item>> -> !ac.var<i16>
}

//--- with-result.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "A", fields = [{name = "value", type = i64}]}> : () -> ()
    "ac.transaction"() <{sym_name = "B", fields = [{name = "value", type = i64}]}> : () -> ()
  }) : () -> ()
  %item = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.transaction<@types::@A>>
  %value = ac.var.constant 1 : i64 as !ac.var<i64>
  %bad = ac.var.with %item, %value field "value" : !ac.var<!ac.transaction<@types::@A>>, !ac.var<i64> -> !ac.var<!ac.transaction<@types::@B>>
}

//--- with-value.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "Item", fields = [{name = "value", type = i64}]}> : () -> ()
  }) : () -> ()
  %item = "builtin.unrealized_conversion_cast"() : () -> !ac.var<!ac.transaction<@types::@Item>>
  %value = ac.var.constant 1 : i16 as !ac.var<i16>
  %bad = ac.var.with %item, %value field "value" : !ac.var<!ac.transaction<@types::@Item>>, !ac.var<i16> -> !ac.var<!ac.transaction<@types::@Item>>
}

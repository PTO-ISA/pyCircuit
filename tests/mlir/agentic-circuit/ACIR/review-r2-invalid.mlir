// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/malformed-field.mlir 2>&1 | %FileCheck %s --check-prefix=MALFORMED
// RUN: %not %acir_opt %t/missing-name.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING-NAME
// RUN: %not %acir_opt %t/missing-type.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING-TYPE
// RUN: %not %acir_opt %t/wrong-typed-non-list-bound.mlir 2>&1 | %FileCheck %s --check-prefix=WRONG-BOUND
// RUN: %not %acir_opt %t/wrong-typed-list-bound.mlir 2>&1 | %FileCheck %s --check-prefix=WRONG-LIST-BOUND
// RUN: %not %acir_opt %t/noncanonical-list-bound.mlir 2>&1 | %FileCheck %s --check-prefix=NONCANONICAL-LIST-BOUND
// RUN: %not %acir_opt %t/nested-list-no-bound.mlir 2>&1 | %FileCheck %s --check-prefix=NESTED-LIST
// RUN: %not %acir_opt %t/union-discriminator-type.mlir 2>&1 | %FileCheck %s --check-prefix=UNION-TYPE
// RUN: %not %acir_opt %t/create-unresolved.mlir 2>&1 | %FileCheck %s --check-prefix=CREATE-UNRESOLVED
// RUN: %not %acir_opt %t/create-non-record.mlir 2>&1 | %FileCheck %s --check-prefix=CREATE-NON-RECORD
// RUN: %not %acir_opt %t/create-field-order.mlir 2>&1 | %FileCheck %s --check-prefix=CREATE-ORDER
// RUN: %not %acir_opt %t/serialize-flat-attr.mlir 2>&1 | %FileCheck %s --check-prefix=SERIALIZE-FLAT
// RUN: %not %acir_opt %t/deserialize-flat-attr.mlir 2>&1 | %FileCheck %s --check-prefix=DESERIALIZE-FLAT
// RUN: %not %acir_opt %t/deserialize-element.mlir 2>&1 | %FileCheck %s --check-prefix=DESERIALIZE-ELEMENT

// MALFORMED: error: {{.*}}field metadata requires string 'name' and type 'type'
// MISSING-NAME: error: {{.*}}field metadata requires string 'name' and type 'type'
// MISSING-TYPE: error: {{.*}}field metadata requires string 'name' and type 'type'
// WRONG-BOUND: error: {{.*}}non-list field 'value' cannot declare max_length
// WRONG-LIST-BOUND: error: {{.*}}list field 'items' requires a finite positive max_length
// NONCANONICAL-LIST-BOUND: error: {{.*}}list field 'items' requires a finite positive max_length
// NESTED-LIST: error: {{.*}}list field 'items' requires a finite positive max_length
// UNION-TYPE: error: {{.*}}union discriminator 'tag' must name an integer or enum field
// CREATE-UNRESOLVED: error: {{.*}}record.create result must resolve to a record declaration
// CREATE-NON-RECORD: error: {{.*}}record.create result must resolve to a record declaration
// CREATE-ORDER: error: {{.*}}record.create fields must exactly match declaration
// SERIALIZE-FLAT: error: {{.*}}named data references require a qualified symbol such as '@types::@S'
// DESERIALIZE-FLAT: error: {{.*}}named data references require a qualified symbol such as '@types::@S'
// DESERIALIZE-ELEMENT: error: {{.*}}packet.deserialize operand must be an i8 byte vector

//--- malformed-field.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = ["oops"]}> : () -> ()
  }) : () -> ()
}

//--- missing-name.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{type = i8}]}> : () -> ()
  }) : () -> ()
}

//--- missing-type.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "value"}]}> : () -> ()
  }) : () -> ()
}

//--- wrong-typed-non-list-bound.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "value", type = i8, max_length = "oops"}]}> : () -> ()
  }) : () -> ()
}

//--- wrong-typed-list-bound.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "items", type = !ac.list<i8>, max_length = "oops"}]}> : () -> ()
  }) : () -> ()
}

//--- nested-list-no-bound.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "items", type = !ac.optional<!ac.vector<2 x !ac.list<i8>>>}]}> : () -> ()
  }) : () -> ()
}

//--- noncanonical-list-bound.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "items", type = !ac.list<i8>, max_length = 4 : i32}]}> : () -> ()
  }) : () -> ()
}

//--- union-discriminator-type.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.union"() <{sym_name = "U", fields = [{name = "tag", type = f32}], discriminator = "tag"}> : () -> ()
  }) : () -> ()
}

//--- create-unresolved.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %value = "builtin.unrealized_conversion_cast"() : () -> i8
  %record = "ac.record.create"(%value) <{field_names = ["value"]}> : (i8) -> !ac.transaction<@types::@Missing>
}

//--- create-non-record.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %record = "ac.record.create"() <{field_names = []}> : () -> i8
}

//--- create-field-order.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "first", type = i8}, {name = "second", type = i8}]}> : () -> ()
  }) : () -> ()
  %first = "builtin.unrealized_conversion_cast"() : () -> i8
  %second = "builtin.unrealized_conversion_cast"() : () -> i8
  %record = "ac.record.create"(%first, %second) <{field_names = ["second", "first"]}> : (i8, i8) -> !ac.transaction<@types::@T>
}

//--- serialize-flat-attr.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %packet = "builtin.unrealized_conversion_cast"() : () -> !ac.packet<@types::@P>
  %bytes = "ac.packet.serialize"(%packet) <{packet = @P}> : (!ac.packet<@types::@P>) -> !ac.vector<8 x i8>
}

//--- deserialize-flat-attr.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  %bytes = "builtin.unrealized_conversion_cast"() : () -> !ac.vector<8 x i8>
  %packet = "ac.packet.deserialize"(%bytes) <{packet = @P}> : (!ac.vector<8 x i8>) -> !ac.packet<@types::@P>
}

//--- deserialize-element.mlir
builtin.module attributes {ac.contract_epoch = "0.5"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.packet"() <{sym_name = "P", fields = []}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<!ac.packet<@types::@P> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, serialization_width = 8 : i64, size = 8 : i64}>} : () -> ()
  %bytes = "builtin.unrealized_conversion_cast"() : () -> !ac.vector<8 x i16>
  %packet = "ac.packet.deserialize"(%bytes) <{packet = @types::@P}> : (!ac.vector<8 x i16>) -> !ac.packet<@types::@P>
}

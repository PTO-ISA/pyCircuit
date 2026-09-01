// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/missing-field.mlir 2>&1 | %FileCheck %s --check-prefix=MISSING
// RUN: %not %acir_opt %t/create-type.mlir 2>&1 | %FileCheck %s --check-prefix=CREATE-TYPE
// RUN: %not %acir_opt %t/get-type.mlir 2>&1 | %FileCheck %s --check-prefix=GET-TYPE
// RUN: %not %acir_opt %t/with-identity.mlir 2>&1 | %FileCheck %s --check-prefix=IDENTITY
// RUN: %not %acir_opt %t/serialize-kind.mlir 2>&1 | %FileCheck %s --check-prefix=SERIALIZE-KIND
// RUN: %not %acir_opt %t/serialize-width.mlir 2>&1 | %FileCheck %s --check-prefix=SERIALIZE-WIDTH
// RUN: %not %acir_opt %t/deserialize-identity.mlir 2>&1 | %FileCheck %s --check-prefix=DESERIALIZE-ID

// MISSING: error: {{.*}}record.create fields must exactly match declaration
// CREATE-TYPE: error: {{.*}}field 'x' expects 'i8' but received 'i16'
// GET-TYPE: error: {{.*}}field 'x' has type 'i8' but operation returns 'i16'
// IDENTITY: error: {{.*}}record.with must preserve record identity
// SERIALIZE-KIND: error: {{.*}}packet.serialize requires a packet operand
// SERIALIZE-WIDTH: error: {{.*}}serialized byte vector width must equal packet serialization width 4
// DESERIALIZE-ID: error: {{.*}}packet.deserialize result identity does not match serialization contract

//--- missing-field.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "Pair", fields = [{name = "x", type = i8}, {name = "y", type = i8}]}> : () -> ()
  }) : () -> ()
  %x = "builtin.unrealized_conversion_cast"() : () -> i8
  %v = "ac.record.create"(%x) <{field_names = ["x"]}> : (i8) -> !ac.transaction<@types::@Pair>
}

//--- create-type.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "One", fields = [{name = "x", type = i8}]}> : () -> ()
  }) : () -> ()
  %x = "builtin.unrealized_conversion_cast"() : () -> i16
  %v = "ac.record.create"(%x) <{field_names = ["x"]}> : (i16) -> !ac.transaction<@types::@One>
}

//--- get-type.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "One", fields = [{name = "x", type = i8}]}> : () -> ()
  }) : () -> ()
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.transaction<@types::@One>
  %x = "ac.record.get"(%v) <{field = "x"}> : (!ac.transaction<@types::@One>) -> i16
}

//--- with-identity.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "A", fields = [{name = "x", type = i8}]}> : () -> ()
    "ac.transaction"() <{sym_name = "B", fields = [{name = "x", type = i8}]}> : () -> ()
  }) : () -> ()
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.transaction<@types::@A>
  %x = "builtin.unrealized_conversion_cast"() : () -> i8
  %bad = "ac.record.with"(%v, %x) <{field = "x"}> : (!ac.transaction<@types::@A>, i8) -> !ac.transaction<@types::@B>
}

//--- serialize-kind.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = []}> : () -> ()
  }) : () -> ()
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.transaction<@types::@T>
  %bytes = "ac.packet.serialize"(%v) <{packet = @types::@T}> : (!ac.transaction<@types::@T>) -> !ac.vector<1 x i8>
}

//--- serialize-width.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.packet"() <{sym_name = "P", fields = []}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<!ac.packet<@types::@P> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, serialization_width = 4 : i64, size = 8 : i64}>} : () -> ()
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.packet<@types::@P>
  %bytes = "ac.packet.serialize"(%v) <{packet = @types::@P}> : (!ac.packet<@types::@P>) -> !ac.vector<8 x i8>
}

//--- deserialize-identity.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.packet"() <{sym_name = "P", fields = []}> : () -> ()
    "ac.packet"() <{sym_name = "Q", fields = []}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<
    !ac.packet<@types::@P> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, serialization_width = 4 : i64, size = 4 : i64},
    !ac.packet<@types::@Q> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, serialization_width = 4 : i64, size = 4 : i64}
  >} : () -> ()
  %bytes = "builtin.unrealized_conversion_cast"() : () -> !ac.vector<4 x i8>
  %v = "ac.packet.deserialize"(%bytes) <{packet = @types::@P}> : (!ac.vector<4 x i8>) -> !ac.packet<@types::@Q>
}

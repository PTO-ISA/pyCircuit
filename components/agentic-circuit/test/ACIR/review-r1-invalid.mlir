// RUN: %split_file %s %t
// RUN: %not %acir_opt %t/declaration-placement.mlir 2>&1 | %FileCheck %s --check-prefix=PLACEMENT
// RUN: %not %acir_opt %t/flat-external.mlir 2>&1 | %FileCheck %s --check-prefix=FLAT
// RUN: %not %acir_opt %t/function-field.mlir 2>&1 | %FileCheck %s --check-prefix=FUNCTION
// RUN: %not %acir_opt %t/channel-field.mlir 2>&1 | %FileCheck %s --check-prefix=CHANNEL
// RUN: %not %acir_opt %t/capability-field.mlir 2>&1 | %FileCheck %s --check-prefix=CAPABILITY
// RUN: %not %acir_opt %t/none-field.mlir 2>&1 | %FileCheck %s --check-prefix=NONE
// RUN: %not %acir_opt %t/list-bound-missing.mlir 2>&1 | %FileCheck %s --check-prefix=BOUND-MISSING
// RUN: %not %acir_opt %t/list-bound-zero.mlir 2>&1 | %FileCheck %s --check-prefix=BOUND-ZERO
// RUN: %not %acir_opt %t/list-bound-inconsistent.mlir 2>&1 | %FileCheck %s --check-prefix=BOUND-INCONSISTENT
// RUN: %not %acir_opt %t/layout-missing.mlir 2>&1 | %FileCheck %s --check-prefix=LAYOUT-MISSING
// RUN: %not %acir_opt %t/layout-invalid.mlir 2>&1 | %FileCheck %s --check-prefix=LAYOUT-INVALID
// RUN: %not %acir_opt %t/packet-width-missing.mlir 2>&1 | %FileCheck %s --check-prefix=PACKET-WIDTH-MISSING
// RUN: %not %acir_opt %t/union-discriminator-missing.mlir 2>&1 | %FileCheck %s --check-prefix=UNION-MISSING
// RUN: %not %acir_opt %t/get-field.mlir 2>&1 | %FileCheck %s --check-prefix=GET-FIELD
// RUN: %not %acir_opt %t/get-kind.mlir 2>&1 | %FileCheck %s --check-prefix=GET-KIND
// RUN: %not %acir_opt %t/with-field.mlir 2>&1 | %FileCheck %s --check-prefix=WITH-FIELD
// RUN: %not %acir_opt %t/with-kind.mlir 2>&1 | %FileCheck %s --check-prefix=WITH-KIND
// RUN: %not %acir_opt %t/with-value.mlir 2>&1 | %FileCheck %s --check-prefix=WITH-VALUE
// RUN: %not %acir_opt %t/serialize-identity.mlir 2>&1 | %FileCheck %s --check-prefix=SER-ID
// RUN: %not %acir_opt %t/serialize-unresolved.mlir 2>&1 | %FileCheck %s --check-prefix=SER-UNRESOLVED
// RUN: %not %acir_opt %t/serialize-element.mlir 2>&1 | %FileCheck %s --check-prefix=SER-ELEMENT
// RUN: %not %acir_opt %t/deserialize-unresolved.mlir 2>&1 | %FileCheck %s --check-prefix=DESER-UNRESOLVED
// RUN: %not %acir_opt %t/deserialize-width.mlir 2>&1 | %FileCheck %s --check-prefix=DESER-WIDTH

// PLACEMENT: error: {{.*}}named data declarations must be direct children of ac.type_scope
// FLAT: error: {{.*}}named data references require a qualified symbol such as '@types::@S'
// FUNCTION: error: {{.*}}field 'bad' has non-value type
// CHANNEL: error: {{.*}}field 'bad' has non-value type
// CAPABILITY: error: {{.*}}field 'bad' has non-value type
// NONE: error: {{.*}}field 'bad' has non-value type
// BOUND-MISSING: error: {{.*}}list field 'items' requires a finite positive max_length
// BOUND-ZERO: error: {{.*}}list field 'items' requires a finite positive max_length
// BOUND-INCONSISTENT: error: {{.*}}non-list field 'value' cannot declare max_length
// LAYOUT-MISSING: error: {{.*}}missing DLTI layout entry for '!ac.struct<@types::@S>'
// LAYOUT-INVALID: error: {{.*}}layout entry requires positive size/alignment and explicit endianness
// PACKET-WIDTH-MISSING: error: packet layout entry requires positive serialization_width
// UNION-MISSING: error: {{.*}}union discriminator 'missing' does not name a field
// GET-FIELD: error: {{.*}}unknown record field 'missing'
// GET-KIND: error: {{.*}}record.get requires a record-like operand
// WITH-FIELD: error: {{.*}}unknown record field 'missing'
// WITH-KIND: error: {{.*}}record.with requires a record-like operand
// WITH-VALUE: error: {{.*}}field 'value' expects 'i32' but received 'i8'
// SER-ID: error: {{.*}}packet.serialize identity does not match packet operand
// SER-UNRESOLVED: error: {{.*}}packet.serialize packet declaration is unresolved
// SER-ELEMENT: error: {{.*}}packet.serialize result must be an i8 byte vector
// DESER-UNRESOLVED: error: {{.*}}packet.deserialize packet declaration is unresolved
// DESER-WIDTH: error: {{.*}}serialized byte vector width must equal packet serialization width 8

//--- declaration-placement.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.struct"() <{sym_name = "S", fields = []}> : () -> ()
}

//--- flat-external.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.struct<@S>
  %x = "ac.record.get"(%v) <{field = "x"}> : (!ac.struct<@S>) -> i8
}

//--- function-field.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.struct"() <{sym_name = "S", fields = [{name = "bad", type = (i8) -> i8}]}> : () -> ()
  }) : () -> ()
}

//--- channel-field.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.struct"() <{sym_name = "S", fields = [{name = "bad", type = !ac.channel<i8, @p>}]}> : () -> ()
  }) : () -> ()
}

//--- capability-field.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "bad", type = !ac.resource_token<@r>}]}> : () -> ()
  }) : () -> ()
}

//--- none-field.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "bad", type = none}]}> : () -> ()
  }) : () -> ()
}

//--- list-bound-missing.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.struct"() <{sym_name = "S", fields = [{name = "items", type = !ac.list<i8>}]}> : () -> ()
  }) : () -> ()
}

//--- list-bound-zero.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.struct"() <{sym_name = "S", fields = [{name = "items", type = !ac.list<i8>, max_length = 0 : i64}]}> : () -> ()
  }) : () -> ()
}

//--- list-bound-inconsistent.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.struct"() <{sym_name = "S", fields = [{name = "value", type = i8, max_length = 4 : i64}]}> : () -> ()
  }) : () -> ()
}

//--- layout-missing.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.struct"() <{sym_name = "S", fields = []}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<>} : () -> ()
}

//--- layout-invalid.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.struct"() <{sym_name = "S", fields = []}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<!ac.struct<@types::@S> = {abi_alignment = 0 : i64, endianness = "middle", preferred_alignment = 0 : i64, size = 0 : i64}>} : () -> ()
}

//--- packet-width-missing.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.packet"() <{sym_name = "P", fields = []}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<!ac.packet<@types::@P> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 8 : i64}>} : () -> ()
}

//--- union-discriminator-missing.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.union"() <{sym_name = "U", fields = [{name = "tag", type = i8}], discriminator = "missing"}> : () -> ()
  }) : () -> ()
}

//--- get-field.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "value", type = i32}]}> : () -> ()
  }) : () -> ()
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.transaction<@types::@T>
  %x = "ac.record.get"(%v) <{field = "missing"}> : (!ac.transaction<@types::@T>) -> i32
}

//--- get-kind.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %v = "builtin.unrealized_conversion_cast"() : () -> i32
  %x = "ac.record.get"(%v) <{field = "x"}> : (i32) -> i8
}

//--- with-field.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "value", type = i32}]}> : () -> ()
  }) : () -> ()
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.transaction<@types::@T>
  %x = "builtin.unrealized_conversion_cast"() : () -> i32
  %r = "ac.record.with"(%v, %x) <{field = "missing"}> : (!ac.transaction<@types::@T>, i32) -> !ac.transaction<@types::@T>
}

//--- with-kind.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %v = "builtin.unrealized_conversion_cast"() : () -> i32
  %x = "builtin.unrealized_conversion_cast"() : () -> i8
  %r = "ac.record.with"(%v, %x) <{field = "x"}> : (i32, i8) -> i32
}

//--- with-value.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.transaction"() <{sym_name = "T", fields = [{name = "value", type = i32}]}> : () -> ()
  }) : () -> ()
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.transaction<@types::@T>
  %x = "builtin.unrealized_conversion_cast"() : () -> i8
  %r = "ac.record.with"(%v, %x) <{field = "value"}> : (!ac.transaction<@types::@T>, i8) -> !ac.transaction<@types::@T>
}

//--- serialize-identity.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.packet<@types::@P>
  %r = "ac.packet.serialize"(%v) <{packet = @types::@Q}> : (!ac.packet<@types::@P>) -> !ac.vector<8 x i8>
}

//--- serialize-unresolved.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.packet<@types::@Missing>
  %r = "ac.packet.serialize"(%v) <{packet = @types::@Missing}> : (!ac.packet<@types::@Missing>) -> !ac.vector<8 x i8>
}

//--- serialize-element.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.packet"() <{sym_name = "P", fields = []}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<!ac.packet<@types::@P> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, serialization_width = 8 : i64, size = 8 : i64}>} : () -> ()
  %v = "builtin.unrealized_conversion_cast"() : () -> !ac.packet<@types::@P>
  %r = "ac.packet.serialize"(%v) <{packet = @types::@P}> : (!ac.packet<@types::@P>) -> !ac.vector<8 x i16>
}

//--- deserialize-unresolved.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  %b = "builtin.unrealized_conversion_cast"() : () -> !ac.vector<8 x i8>
  %r = "ac.packet.deserialize"(%b) <{packet = @types::@Missing}> : (!ac.vector<8 x i8>) -> !ac.packet<@types::@Missing>
}

//--- deserialize-width.mlir
builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.packet"() <{sym_name = "P", fields = []}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<!ac.packet<@types::@P> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, serialization_width = 8 : i64, size = 8 : i64}>} : () -> ()
  %b = "builtin.unrealized_conversion_cast"() : () -> !ac.vector<4 x i8>
  %r = "ac.packet.deserialize"(%b) <{packet = @types::@P}> : (!ac.vector<4 x i8>) -> !ac.packet<@types::@P>
}

// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt %s | %acir_opt | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.4"} {
  "ac.type_scope"() <{sym_name = "types"}> ({
    "ac.struct"() <{sym_name = "Header", fields = [{name = "opcode", type = i8}, {name = "tag", type = i16}]}> : () -> ()
    "ac.packet"() <{sym_name = "Request", fields = [{name = "opcode", type = i8}, {name = "payload", type = i32}]}> : () -> ()
    "ac.transaction"() <{sym_name = "Dma", fields = [{name = "request", type = !ac.packet<@types::@Request>}, {name = "tag", type = i16}]}> : () -> ()
  }) {dlti.dl_spec = #dlti.dl_spec<
    !ac.struct<@types::@Header> = {abi_alignment = 2 : i64, endianness = "little", preferred_alignment = 2 : i64, size = 4 : i64},
    !ac.packet<@types::@Request> = {abi_alignment = 4 : i64, endianness = "little", preferred_alignment = 4 : i64, serialization_width = 8 : i64, size = 8 : i64}
  >} : () -> ()

  %opcode = "builtin.unrealized_conversion_cast"() : () -> i8
  %tag = "builtin.unrealized_conversion_cast"() : () -> i16
  %payload = "builtin.unrealized_conversion_cast"() : () -> i32
  %header = "ac.record.create"(%opcode, %tag) <{field_names = ["opcode", "tag"]}> : (i8, i16) -> !ac.struct<@types::@Header>
  %got = "ac.record.get"(%header) <{field = "opcode"}> : (!ac.struct<@types::@Header>) -> i8
  %updated = "ac.record.with"(%header, %got) <{field = "opcode"}> : (!ac.struct<@types::@Header>, i8) -> !ac.struct<@types::@Header>
  %packet = "ac.record.create"(%opcode, %payload) <{field_names = ["opcode", "payload"]}> : (i8, i32) -> !ac.packet<@types::@Request>
  %packet2 = "ac.record.with"(%packet, %payload) <{field = "payload"}> : (!ac.packet<@types::@Request>, i32) -> !ac.packet<@types::@Request>
  %bytes = "ac.packet.serialize"(%packet2) <{packet = @types::@Request}> : (!ac.packet<@types::@Request>) -> !ac.vector<8 x i8>
  %copy = "ac.packet.deserialize"(%bytes) <{packet = @types::@Request}> : (!ac.vector<8 x i8>) -> !ac.packet<@types::@Request>
  %tx = "ac.record.create"(%copy, %tag) <{field_names = ["request", "tag"]}> : (!ac.packet<@types::@Request>, i16) -> !ac.transaction<@types::@Dma>
}

// CHECK: "ac.record.create"
// CHECK: "ac.record.get"
// CHECK: "ac.record.with"
// CHECK: "ac.packet.serialize"
// CHECK: "ac.packet.deserialize"
// CHECK-SAME: !ac.packet<@types::@Request>

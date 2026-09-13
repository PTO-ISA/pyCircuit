// RUN: %acir_opt %s | %FileCheck %s
// RUN: %acir_opt --emit-bytecode -o %t.bc %s
// RUN: %acir_opt %t.bc | %FileCheck %s

builtin.module attributes {ac.contract_epoch = "0.5"} {
  ac.type_scope @types {
    ac.enum @Mode enumerants ["idle", "run", "wait"]
    ac.enum @Opcode enumerants ["none", "read", "write"] values [0 : i64, 3 : i64, 9 : i64] width 4
    ac.enum @Wide enumerants ["low", "high", "max"] values [9223372036854775807 : i64, 9223372036854775808 : i64, 18446744073709551615 : i64] width 64
  } {dlti.dl_spec = #dlti.dl_spec<!ac.enum<@types::@Mode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}, !ac.enum<@types::@Opcode> = {abi_alignment = 1 : i64, endianness = "little", preferred_alignment = 1 : i64, size = 1 : i64}, !ac.enum<@types::@Wide> = {abi_alignment = 8 : i64, endianness = "little", preferred_alignment = 8 : i64, size = 8 : i64}>}
  %run = ac.var.enum @types::@Mode "run" : !ac.var<!ac.enum<@types::@Mode>>
  %wait = ac.var.enum @types::@Mode "wait" : !ac.var<!ac.enum<@types::@Mode>>
  %different = ac.var.cmp "ne" %run, %wait : !ac.var<!ac.enum<@types::@Mode>> -> !ac.var<i1>
  %write = ac.var.enum @types::@Opcode "write" : !ac.var<!ac.enum<@types::@Opcode>>
}

// CHECK: ac.enum @Opcode enumerants ["none", "read", "write"] values [0, 3, 9] width 4
// CHECK: ac.enum @Wide enumerants ["low", "high", "max"] values [9223372036854775807, -9223372036854775808, -1] width 64
// CHECK: ac.var.enum @types::@Mode "run"
// CHECK: ac.var.enum @types::@Mode "wait"
// CHECK: ac.var.cmp "ne"
// CHECK: ac.var.enum @types::@Opcode "write"

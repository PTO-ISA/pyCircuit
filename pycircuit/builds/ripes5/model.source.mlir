#loc55 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.5.16:index")
#loc56 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.5.16:memory")
#loc57 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.5.16:writeback")
#loc63 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.6.23:original")
#loc64 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.6.23:source")
#loc65 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.6.23:memory")
#loc66 = loc("acir.binding:top.cpu_body.0.call.5.9.0.call.6.23:writeback")
#loc72 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124:lhs")
#loc73 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124:rhs")
#loc77 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172:value")
#loc78 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172:amount")
#loc85 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:instruction")
#loc86 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:pc")
#loc87 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:lhs")
#loc88 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:rhs")
#loc131 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.11.30:instruction")
#loc132 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.11.30:lhs")
#loc133 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.11.30:rhs")
#loc151 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298:value")
#loc152 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298:bits")
#loc157 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6:word")
#loc247 = loc("acir.binding:top.cpu_body.0.call.23.34:control")
#loc266 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6.0.call.17.49:index")
#loc267 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6.0.call.17.49:writeback")
#loc274 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3.0.call.21.18:index")
#loc275 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3.0.call.21.18:value")
#loc279 = loc("acir.binding:top.cpu.fetch_body.0.call.13.2:control")
#loc295 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2:source")
#loc299 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:source")
#loc300 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:control")
#loc301 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:writeback")
#loc320 = loc("acir.binding:top.cpu_body.0.call.4.4:source")
#loc321 = loc("acir.binding:top.cpu_body.0.call.4.4:instruction")
#loc330 = loc("acir.binding:top.cpu_body.0.call.5.9:source")
#loc331 = loc("acir.binding:top.cpu_body.0.call.5.9:older")
#loc332 = loc("acir.binding:top.cpu_body.0.call.5.9:retiring")
#loc345 = loc("acir.binding:top.cpu_body.0.call.8.12:source")
#loc346 = loc("acir.binding:top.cpu_body.0.call.8.12:operands")
#loc369 = loc("acir.binding:top.cpu.execute_body.0.call.19.2:packet")
#loc371 = loc("acir.binding:top.cpu.memory_body.0.call.20.2:source")
#loc398 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3:source")
#loc399 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3:code_size")
module attributes {acir.model = {config = [], constants = {}, format_version = 9 : i64, top = "Demo", type_order = ["src_isa_e31afbf3.Instruction", "src_isa_e31afbf3.Fetched", "src_isa_e31afbf3.Decoded", "src_isa_e31afbf3.Executed", "src_isa_e31afbf3.Completed", "src_isa_e31afbf3.Control", "src_isa_e31afbf3.ForwardedOperands", "src_isa_e31afbf3.ExecutionResult", "src_isa_e31afbf3.Retirement", "src_isa_e31afbf3.StoreEvent"], types = {src_isa_e31afbf3.Completed = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = false, name = "stalled", payload_type = i1, type = "bool"}], src_isa_e31afbf3.Control = [{default = false, name = "stall", payload_type = i1, type = "bool"}, {default = false, name = "redirect", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "target", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "forward_a", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "forward_b", payload_type = i32, type = "u32"}], src_isa_e31afbf3.Decoded = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default, name = "instruction", payload_type = !acir.struct<"src_isa_e31afbf3.Instruction">, type = "src_isa_e31afbf3.Instruction"}, {default = 0 : i64, name = "lhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rhs", payload_type = i32, type = "u32"}, {default = false, name = "stalled", payload_type = i1, type = "bool"}], src_isa_e31afbf3.Executed = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = false, name = "load", payload_type = i1, type = "bool"}, {default = false, name = "store", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "alu", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = false, name = "stalled", payload_type = i1, type = "bool"}], src_isa_e31afbf3.ExecutionResult = [{default, name = "packet", payload_type = !acir.struct<"src_isa_e31afbf3.Executed">, type = "src_isa_e31afbf3.Executed"}, {default = false, name = "redirect", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "target", payload_type = i32, type = "u32"}], src_isa_e31afbf3.Fetched = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}], src_isa_e31afbf3.ForwardedOperands = [{default = 0 : i64, name = "lhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "source_a", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "source_b", payload_type = i32, type = "u32"}], src_isa_e31afbf3.Instruction = [{default = 0 : i64, name = "opcode", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "funct3", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "funct7", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "immediate", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = false, name = "load", payload_type = i1, type = "bool"}, {default = false, name = "store", payload_type = i1, type = "bool"}], src_isa_e31afbf3.Retirement = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}], src_isa_e31afbf3.StoreEvent = [{default = false, name = "valid", payload_type = i1, type = "bool"}, {default = 0 : i64, name = "address", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}]}}} {
  func.func @ac_init_top.cpu.rf.registers.0() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.0", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c0_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.1() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.1", params = [], result = "u32"}} {
    %c1_i32 = arith.constant 1 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c1_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.2() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.2", params = [], result = "u32"}} {
    %c2_i32 = arith.constant 2 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c2_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.3() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.3", params = [], result = "u32"}} {
    %c3_i32 = arith.constant 3 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c3_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.4() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.4", params = [], result = "u32"}} {
    %c4_i32 = arith.constant 4 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c4_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.5() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.5", params = [], result = "u32"}} {
    %c5_i32 = arith.constant 5 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c5_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.6() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.6", params = [], result = "u32"}} {
    %c6_i32 = arith.constant 6 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c6_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.7() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.7", params = [], result = "u32"}} {
    %c7_i32 = arith.constant 7 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c7_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.8() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.8", params = [], result = "u32"}} {
    %c8_i32 = arith.constant 8 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c8_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.9() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.9", params = [], result = "u32"}} {
    %c9_i32 = arith.constant 9 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c9_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.10() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.10", params = [], result = "u32"}} {
    %c10_i32 = arith.constant 10 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c10_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.11() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.11", params = [], result = "u32"}} {
    %c11_i32 = arith.constant 11 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c11_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.12() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.12", params = [], result = "u32"}} {
    %c12_i32 = arith.constant 12 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c12_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.13() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.13", params = [], result = "u32"}} {
    %c13_i32 = arith.constant 13 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c13_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.14() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.14", params = [], result = "u32"}} {
    %c14_i32 = arith.constant 14 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c14_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.15() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.15", params = [], result = "u32"}} {
    %c15_i32 = arith.constant 15 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c15_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.16() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.16", params = [], result = "u32"}} {
    %c16_i32 = arith.constant 16 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c16_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.17() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.17", params = [], result = "u32"}} {
    %c17_i32 = arith.constant 17 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c17_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.18() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.18", params = [], result = "u32"}} {
    %c18_i32 = arith.constant 18 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c18_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.19() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.19", params = [], result = "u32"}} {
    %c19_i32 = arith.constant 19 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c19_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.20() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.20", params = [], result = "u32"}} {
    %c20_i32 = arith.constant 20 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c20_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.21() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.21", params = [], result = "u32"}} {
    %c21_i32 = arith.constant 21 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c21_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.22() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.22", params = [], result = "u32"}} {
    %c22_i32 = arith.constant 22 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c22_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.23() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.23", params = [], result = "u32"}} {
    %c23_i32 = arith.constant 23 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c23_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.24() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.24", params = [], result = "u32"}} {
    %c24_i32 = arith.constant 24 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c24_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.25() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.25", params = [], result = "u32"}} {
    %c25_i32 = arith.constant 25 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c25_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.26() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.26", params = [], result = "u32"}} {
    %c26_i32 = arith.constant 26 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c26_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.27() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.27", params = [], result = "u32"}} {
    %c27_i32 = arith.constant 27 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c27_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.28() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.28", params = [], result = "u32"}} {
    %c28_i32 = arith.constant 28 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c28_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.29() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.29", params = [], result = "u32"}} {
    %c29_i32 = arith.constant 29 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c29_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.30() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.30", params = [], result = "u32"}} {
    %c30_i32 = arith.constant 30 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c30_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.rf.registers.31() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.rf.registers.31", params = [], result = "u32"}} {
    %c31_i32 = arith.constant 31 : i32 loc(#loc1)
    %0 = call @construct.0.call.1.1() {acir.id = "v2", acir.source_type = "array<u32;32>", role = "helper"} : () -> !acir.array<i32, [32]> loc(#loc2)
    %1 = "acir.array.index"(%0, %c31_i32) {signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc3)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched"> attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.if_id", params = [], result = "src_isa_e31afbf3.Fetched"}} {
    %false = arith.constant false loc(#loc4)
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate(%false, %c0_i32, %c0_i32) : (i1, i32, i32) -> !acir.struct<"src_isa_e31afbf3.Fetched"> loc(#loc4)
    return %0 : !acir.struct<"src_isa_e31afbf3.Fetched">
  }
  func.func @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded"> attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.id_ex", params = [], result = "src_isa_e31afbf3.Decoded"}} {
    %false = arith.constant false
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate() : () -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc5)
    %1 = acir.aggregate(%false, %c0_i32, %c0_i32, %0, %c0_i32, %c0_i32, %false) : (i1, i32, i32, !acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc5)
    return %1 : !acir.struct<"src_isa_e31afbf3.Decoded">
  }
  func.func @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed"> attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.ex_mem", params = [], result = "src_isa_e31afbf3.Executed"}} {
    %false = arith.constant false
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %false, %false, %false, %c0_i32, %c0_i32, %c0_i32, %false) : (i1, i32, i32, i32, i1, i1, i1, i32, i32, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc6)
    return %0 : !acir.struct<"src_isa_e31afbf3.Executed">
  }
  func.func @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed"> attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.mem_wb", params = [], result = "src_isa_e31afbf3.Completed"}} {
    %false = arith.constant false
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %false, %c0_i32, %false) : (i1, i32, i32, i32, i1, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Completed"> loc(#loc7)
    return %0 : !acir.struct<"src_isa_e31afbf3.Completed">
  }
  func.func @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]> attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.fetch.rom", params = [], result = "array<u32;256>"}} {
    %0 = call @construct.0.call.0.0() {acir.id = "v1", acir.source_type = "array<u32;256>", role = "helper"} : () -> !acir.array<i32, [256]> loc(#loc8)
    return %0 : !acir.array<i32, [256]>
  }
  func.func @ac_init_top.cpu.fetch.pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.fetch.pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc9)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.cpu.memory.words.0() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.0", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c0_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.1() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.1", params = [], result = "u32"}} {
    %c1_i32 = arith.constant 1 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c1_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.2() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.2", params = [], result = "u32"}} {
    %c2_i32 = arith.constant 2 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c2_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.3() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.3", params = [], result = "u32"}} {
    %c3_i32 = arith.constant 3 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c3_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.4() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.4", params = [], result = "u32"}} {
    %c4_i32 = arith.constant 4 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c4_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.5() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.5", params = [], result = "u32"}} {
    %c5_i32 = arith.constant 5 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c5_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.6() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.6", params = [], result = "u32"}} {
    %c6_i32 = arith.constant 6 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c6_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.7() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.7", params = [], result = "u32"}} {
    %c7_i32 = arith.constant 7 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c7_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.8() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.8", params = [], result = "u32"}} {
    %c8_i32 = arith.constant 8 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c8_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.9() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.9", params = [], result = "u32"}} {
    %c9_i32 = arith.constant 9 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c9_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.10() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.10", params = [], result = "u32"}} {
    %c10_i32 = arith.constant 10 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c10_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.11() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.11", params = [], result = "u32"}} {
    %c11_i32 = arith.constant 11 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c11_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.12() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.12", params = [], result = "u32"}} {
    %c12_i32 = arith.constant 12 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c12_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.13() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.13", params = [], result = "u32"}} {
    %c13_i32 = arith.constant 13 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c13_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.14() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.14", params = [], result = "u32"}} {
    %c14_i32 = arith.constant 14 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c14_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.15() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.15", params = [], result = "u32"}} {
    %c15_i32 = arith.constant 15 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c15_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.16() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.16", params = [], result = "u32"}} {
    %c16_i32 = arith.constant 16 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c16_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.17() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.17", params = [], result = "u32"}} {
    %c17_i32 = arith.constant 17 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c17_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.18() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.18", params = [], result = "u32"}} {
    %c18_i32 = arith.constant 18 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c18_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.19() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.19", params = [], result = "u32"}} {
    %c19_i32 = arith.constant 19 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c19_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.20() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.20", params = [], result = "u32"}} {
    %c20_i32 = arith.constant 20 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c20_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.21() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.21", params = [], result = "u32"}} {
    %c21_i32 = arith.constant 21 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c21_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.22() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.22", params = [], result = "u32"}} {
    %c22_i32 = arith.constant 22 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c22_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.23() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.23", params = [], result = "u32"}} {
    %c23_i32 = arith.constant 23 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c23_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.24() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.24", params = [], result = "u32"}} {
    %c24_i32 = arith.constant 24 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c24_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.25() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.25", params = [], result = "u32"}} {
    %c25_i32 = arith.constant 25 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c25_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.26() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.26", params = [], result = "u32"}} {
    %c26_i32 = arith.constant 26 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c26_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.27() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.27", params = [], result = "u32"}} {
    %c27_i32 = arith.constant 27 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c27_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.28() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.28", params = [], result = "u32"}} {
    %c28_i32 = arith.constant 28 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c28_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.29() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.29", params = [], result = "u32"}} {
    %c29_i32 = arith.constant 29 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c29_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.30() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.30", params = [], result = "u32"}} {
    %c30_i32 = arith.constant 30 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c30_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.31() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.31", params = [], result = "u32"}} {
    %c31_i32 = arith.constant 31 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c31_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.32() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.32", params = [], result = "u32"}} {
    %c32_i32 = arith.constant 32 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c32_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.33() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.33", params = [], result = "u32"}} {
    %c33_i32 = arith.constant 33 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c33_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.34() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.34", params = [], result = "u32"}} {
    %c34_i32 = arith.constant 34 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c34_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.35() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.35", params = [], result = "u32"}} {
    %c35_i32 = arith.constant 35 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c35_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.36() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.36", params = [], result = "u32"}} {
    %c36_i32 = arith.constant 36 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c36_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.37() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.37", params = [], result = "u32"}} {
    %c37_i32 = arith.constant 37 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c37_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.38() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.38", params = [], result = "u32"}} {
    %c38_i32 = arith.constant 38 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c38_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.39() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.39", params = [], result = "u32"}} {
    %c39_i32 = arith.constant 39 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c39_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.40() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.40", params = [], result = "u32"}} {
    %c40_i32 = arith.constant 40 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c40_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.41() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.41", params = [], result = "u32"}} {
    %c41_i32 = arith.constant 41 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c41_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.42() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.42", params = [], result = "u32"}} {
    %c42_i32 = arith.constant 42 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c42_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.43() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.43", params = [], result = "u32"}} {
    %c43_i32 = arith.constant 43 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c43_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.44() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.44", params = [], result = "u32"}} {
    %c44_i32 = arith.constant 44 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c44_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.45() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.45", params = [], result = "u32"}} {
    %c45_i32 = arith.constant 45 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c45_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.46() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.46", params = [], result = "u32"}} {
    %c46_i32 = arith.constant 46 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c46_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.47() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.47", params = [], result = "u32"}} {
    %c47_i32 = arith.constant 47 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c47_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.48() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.48", params = [], result = "u32"}} {
    %c48_i32 = arith.constant 48 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c48_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.49() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.49", params = [], result = "u32"}} {
    %c49_i32 = arith.constant 49 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c49_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.50() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.50", params = [], result = "u32"}} {
    %c50_i32 = arith.constant 50 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c50_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.51() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.51", params = [], result = "u32"}} {
    %c51_i32 = arith.constant 51 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c51_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.52() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.52", params = [], result = "u32"}} {
    %c52_i32 = arith.constant 52 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c52_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.53() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.53", params = [], result = "u32"}} {
    %c53_i32 = arith.constant 53 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c53_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.54() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.54", params = [], result = "u32"}} {
    %c54_i32 = arith.constant 54 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c54_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.55() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.55", params = [], result = "u32"}} {
    %c55_i32 = arith.constant 55 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c55_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.56() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.56", params = [], result = "u32"}} {
    %c56_i32 = arith.constant 56 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c56_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.57() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.57", params = [], result = "u32"}} {
    %c57_i32 = arith.constant 57 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c57_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.58() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.58", params = [], result = "u32"}} {
    %c58_i32 = arith.constant 58 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c58_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.59() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.59", params = [], result = "u32"}} {
    %c59_i32 = arith.constant 59 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c59_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.60() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.60", params = [], result = "u32"}} {
    %c60_i32 = arith.constant 60 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c60_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.61() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.61", params = [], result = "u32"}} {
    %c61_i32 = arith.constant 61 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c61_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.62() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.62", params = [], result = "u32"}} {
    %c62_i32 = arith.constant 62 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c62_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.words.63() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.words.63", params = [], result = "u32"}} {
    %c63_i32 = arith.constant 63 : i32 loc(#loc10)
    %0 = call @construct.0.call.2.2() {acir.id = "v3", acir.source_type = "array<u32;64>", role = "helper"} : () -> !acir.array<i32, [64]> loc(#loc11)
    %1 = "acir.array.index"(%0, %c63_i32) {signed_index = false} : (!acir.array<i32, [64]>, i32) -> i32 loc(#loc12)
    return %1 : i32
  }
  func.func @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent"> attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.memory.store_event", params = [], result = "src_isa_e31afbf3.StoreEvent"}} {
    %false = arith.constant false loc(#loc13)
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate(%false, %c0_i32, %c0_i32) : (i1, i32, i32) -> !acir.struct<"src_isa_e31afbf3.StoreEvent"> loc(#loc13)
    return %0 : !acir.struct<"src_isa_e31afbf3.StoreEvent">
  }
  func.func @ac_init_top.cpu.writeback.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.writeback.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc14)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement"> attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.writeback.retirement", params = [], result = "src_isa_e31afbf3.Retirement"}} {
    %false = arith.constant false
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %false, %c0_i32) : (i1, i32, i32, i32, i1, i32) -> !acir.struct<"src_isa_e31afbf3.Retirement"> loc(#loc15)
    return %0 : !acir.struct<"src_isa_e31afbf3.Retirement">
  }
  func.func @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control"> attributes {acir.function = {kind = "helper", name = "ac_init_top.cpu.last_control", params = [], result = "src_isa_e31afbf3.Control"}} {
    %false = arith.constant false
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate(%false, %false, %c0_i32, %c0_i32, %c0_i32) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_isa_e31afbf3.Control"> loc(#loc16)
    return %0 : !acir.struct<"src_isa_e31afbf3.Control">
  }
  func.func @construct.0.call.0.0() -> !acir.array<i32, [256]> attributes {acir.function = {definition = "src_demo_7dc0ad19.program_image", kind = "helper", name = "construct.0.call.0.0", params = [], result = "array<u32;256>"}} {
    %c20_i32 = arith.constant 20 : i32 loc(#loc17)
    %c19_i32 = arith.constant 19 : i32
    %c18_i32 = arith.constant 18 : i32 loc(#loc18)
    %c111_i32 = arith.constant 111 : i32 loc(#loc19)
    %c17_i32 = arith.constant 17 : i32 loc(#loc20)
    %c2146435091_i32 = arith.constant 2146435091 : i32 loc(#loc21)
    %c16_i32 = arith.constant 16 : i32 loc(#loc22)
    %c5579299_i32 = arith.constant 5579299 : i32 loc(#loc23)
    %c15_i32 = arith.constant 15 : i32 loc(#loc24)
    %c14_i32 = arith.constant 14 : i32 loc(#loc25)
    %c8388719_i32 = arith.constant 8388719 : i32 loc(#loc26)
    %c13_i32 = arith.constant 13 : i32 loc(#loc27)
    %c1180307_i32 = arith.constant 1180307 : i32 loc(#loc28)
    %c12_i32 = arith.constant 12 : i32 loc(#loc29)
    %c1185379_i32 = arith.constant 1185379 : i32 loc(#loc30)
    %c11_i32 = arith.constant 11 : i32 loc(#loc31)
    %c336387_i32 = arith.constant 336387 : i32 loc(#loc32)
    %c10_i32 = arith.constant 10 : i32 loc(#loc33)
    %c103811731_i32 = arith.constant 103811731 : i32 loc(#loc34)
    %c9_i32 = arith.constant 9 : i32 loc(#loc35)
    %c8_i32 = arith.constant 8 : i32 loc(#loc36)
    %c2201187_i32 = arith.constant 2201187 : i32 loc(#loc37)
    %c7_i32 = arith.constant 7 : i32 loc(#loc38)
    %c1114515_i32 = arith.constant 1114515 : i32 loc(#loc39)
    %c6_i32 = arith.constant 6 : i32 loc(#loc40)
    %c103811603_i32 = arith.constant 103811603 : i32 loc(#loc41)
    %c5_i32 = arith.constant 5 : i32 loc(#loc42)
    %c335907_i32 = arith.constant 335907 : i32
    %c4_i32 = arith.constant 4 : i32 loc(#loc43)
    %c1115747_i32 = arith.constant 1115747 : i32 loc(#loc44)
    %c3_i32 = arith.constant 3 : i32 loc(#loc45)
    %c336131_i32 = arith.constant 336131 : i32 loc(#loc46)
    %c2_i32 = arith.constant 2 : i32 loc(#loc47)
    %c1384483_i32 = arith.constant 1384483 : i32 loc(#loc48)
    %c1_i32 = arith.constant 1 : i32 loc(#loc49)
    %c17825939_i32 = arith.constant 17825939 : i32 loc(#loc50)
    %c5431_i32 = arith.constant 5431 : i32 loc(#loc51)
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %1 = acir.update %0, %c5431_i32[%c0_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %2 = acir.update %1, %c17825939_i32[%c1_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %3 = acir.update %2, %c1384483_i32[%c2_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %4 = acir.update %3, %c336131_i32[%c3_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %5 = acir.update %4, %c1115747_i32[%c4_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %6 = acir.update %5, %c335907_i32[%c5_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %7 = acir.update %6, %c103811603_i32[%c6_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %8 = acir.update %7, %c1114515_i32[%c7_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %9 = acir.update %8, %c2201187_i32[%c8_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %10 = acir.update %9, %c335907_i32[%c9_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %11 = acir.update %10, %c103811731_i32[%c10_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %12 = acir.update %11, %c336387_i32[%c11_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %13 = acir.update %12, %c1185379_i32[%c12_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %14 = acir.update %13, %c1180307_i32[%c13_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %15 = acir.update %14, %c8388719_i32[%c14_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %16 = acir.update %15, %c335907_i32[%c15_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %17 = acir.update %16, %c5579299_i32[%c16_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %18 = acir.update %17, %c2146435091_i32[%c17_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %19 = acir.update %18, %c111_i32[%c18_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %20 = acir.update %19, %c19_i32[%c19_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    %21 = acir.update %20, %c19_i32[%c20_i32] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [256]>, i32, i32) -> !acir.array<i32, [256]> loc(#loc52)
    return %21 : !acir.array<i32, [256]>
  }
  func.func @construct.0.call.1.1() -> !acir.array<i32, [32]> attributes {acir.function = {definition = "src_demo_7dc0ad19.zero_registers", kind = "helper", name = "construct.0.call.1.1", params = [], result = "array<u32;32>"}} {
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [32]> loc(#loc53)
    return %0 : !acir.array<i32, [32]>
  }
  func.func @construct.0.call.2.2() -> !acir.array<i32, [64]> attributes {acir.function = {definition = "src_demo_7dc0ad19.zero_data", kind = "helper", name = "construct.0.call.2.2", params = [], result = "array<u32;64>"}} {
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [64]> loc(#loc54)
    return %0 : !acir.array<i32, [64]>
  }
  func.func @top.cpu_body.0.call.5.9.0.call.5.16(%arg0: i32 loc("acir.binding:top.cpu_body.0.call.5.9.0.call.5.16:index"), %arg1: !acir.struct<"src_isa_e31afbf3.Executed"> loc("acir.binding:top.cpu_body.0.call.5.9.0.call.5.16:memory"), %arg2: !acir.struct<"src_isa_e31afbf3.Completed"> loc("acir.binding:top.cpu_body.0.call.5.9.0.call.5.16:writeback")) -> i32 attributes {acir.function = {definition = "src_isa_e31afbf3.forwarding", kind = "helper", name = "top.cpu_body.0.call.5.9.0.call.5.16", params = [{id = "v1", name = "index", targets = [], type = "u32"}, {id = "v2", name = "memory", targets = [], type = "src_isa_e31afbf3.Executed"}, {id = "v3", name = "writeback", targets = [], type = "src_isa_e31afbf3.Completed"}], result = "u32"}} {
    %c2_i32 = arith.constant 2 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32 = arith.constant 0 : i32
    %false = arith.constant false
    %true = arith.constant true
    %0 = acir.aggregate() : () -> i32
    %1 = arith.cmpi ne, %arg0, %c0_i32 : i32 loc(#loc58)
    %2:2 = scf.if %1 -> (i1, i32) {
      %4 = acir.extract %arg1 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc59)
      %5 = scf.if %4 -> (i1) {
        %9 = acir.extract %arg1 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc60)
        %10 = arith.cmpi eq, %9, %arg0 : i32 loc(#loc60)
        scf.yield %10 : i1
      } else {
        scf.yield %false : i1
      }
      %6 = arith.xori %5, %true : i1
      %7 = arith.select %5, %c1_i32, %0 : i32
      %8:2 = scf.if %5 -> (i1, i32) {
        scf.yield %6, %7 : i1, i32
      } else {
        %9 = acir.extract %arg2 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i1 loc(#loc61)
        %10 = scf.if %9 -> (i1) {
          %13 = acir.extract %arg2 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc62)
          %14 = arith.cmpi eq, %13, %arg0 : i32 loc(#loc62)
          scf.yield %14 : i1
        } else {
          scf.yield %false : i1
        }
        %11 = arith.select %10, %false, %6 : i1
        %12 = arith.select %10, %c2_i32, %7 : i32
        scf.yield %11, %12 : i1, i32
      }
      scf.yield %8#0, %8#1 : i1, i32
    } else {
      scf.yield %true, %0 : i1, i32
    }
    %3 = arith.select %2#0, %c0_i32, %2#1 : i32
    return %3 : i32
  }
  func.func @top.cpu_body.0.call.5.9.0.call.6.23(%arg0: i32 loc("acir.binding:top.cpu_body.0.call.5.9.0.call.6.23:original"), %arg1: i32 loc("acir.binding:top.cpu_body.0.call.5.9.0.call.6.23:source"), %arg2: !acir.struct<"src_isa_e31afbf3.Executed"> loc("acir.binding:top.cpu_body.0.call.5.9.0.call.6.23:memory"), %arg3: !acir.struct<"src_isa_e31afbf3.Completed"> loc("acir.binding:top.cpu_body.0.call.5.9.0.call.6.23:writeback")) -> i32 attributes {acir.function = {definition = "src_isa_e31afbf3.forwarded", kind = "helper", name = "top.cpu_body.0.call.5.9.0.call.6.23", params = [{id = "v1", name = "original", targets = [], type = "u32"}, {id = "v2", name = "source", targets = [], type = "u32"}, {id = "v3", name = "memory", targets = [], type = "src_isa_e31afbf3.Executed"}, {id = "v4", name = "writeback", targets = [], type = "src_isa_e31afbf3.Completed"}], result = "u32"}} {
    %c2_i32 = arith.constant 2 : i32
    %c1_i32 = arith.constant 1 : i32 loc(#loc67)
    %0 = acir.aggregate() : () -> i32
    %1 = arith.cmpi eq, %arg1, %c1_i32 : i32 loc(#loc68)
    %2 = arith.cmpi ne, %arg1, %c1_i32 : i32 loc(#loc68)
    %3 = scf.if %1 -> (i32) {
      %5 = acir.extract %arg2 {field = "alu"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc69)
      scf.yield %5 : i32
    } else {
      scf.yield %0 : i32
    }
    %4 = scf.if %2 -> (i32) {
      %5 = arith.cmpi eq, %arg1, %c2_i32 : i32 loc(#loc70)
      %6 = arith.cmpi ne, %arg1, %c2_i32 : i32 loc(#loc70)
      %7 = scf.if %5 -> (i32) {
        %9 = acir.extract %arg3 {field = "value"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc71)
        scf.yield %9 : i32
      } else {
        scf.yield %3 : i32
      }
      %8 = arith.select %6, %arg0, %7 : i32
      scf.yield %8 : i32
    } else {
      scf.yield %3 : i32
    }
    return %4 : i32
  }
  func.func @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(%arg0: i32 loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124:lhs"), %arg1: i32 loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124:rhs")) -> i1 attributes {acir.function = {definition = "src_isa_e31afbf3.less_signed", kind = "helper", name = "top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124", params = [{id = "v1", name = "lhs", targets = [], type = "u32"}, {id = "v2", name = "rhs", targets = [], type = "u32"}], result = "bool"}} {
    %c-2147483648_i32 = arith.constant -2147483648 : i32
    %0 = arith.xori %arg0, %c-2147483648_i32 : i32 loc(#loc74)
    %1 = arith.xori %arg1, %c-2147483648_i32 : i32 loc(#loc75)
    %2 = arith.cmpi ult, %0, %1 : i32 loc(#loc76)
    return %2 : i1
  }
  func.func @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(%arg0: i32 loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172:value"), %arg1: i32 loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172:amount")) -> i32 attributes {acir.function = {definition = "src_isa_e31afbf3.arithmetic_shift", kind = "helper", name = "top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172", params = [{id = "v1", name = "value", targets = [], type = "u32"}, {id = "v2", name = "amount", targets = [], type = "u32"}], result = "u32"}} {
    %c32_i32 = arith.constant 32 : i32
    %c-1_i32 = arith.constant -1 : i32
    %c-2147483648_i32 = arith.constant -2147483648 : i32
    %c0_i32 = arith.constant 0 : i32
    %false = arith.constant false
    %0 = call @ac_builtin_shr_u32(%arg0, %arg1) {acir.id = "v6", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc79)
    %1 = arith.cmpi ne, %arg1, %c0_i32 : i32 loc(#loc80)
    %2 = scf.if %1 -> (i1) {
      %4 = arith.andi %arg0, %c-2147483648_i32 : i32 loc(#loc81)
      %5 = arith.cmpi ne, %4, %c0_i32 : i32 loc(#loc82)
      scf.yield %5 : i1
    } else {
      scf.yield %false : i1
    }
    %3 = scf.if %2 -> (i32) {
      %4 = arith.subi %c32_i32, %arg1 : i32 loc(#loc83)
      %5 = func.call @ac_builtin_shl_u32(%c-1_i32, %4) {acir.id = "v21", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc84)
      %6 = arith.ori %0, %5 : i32 loc(#loc79)
      scf.yield %6 : i32
    } else {
      scf.yield %0 : i32
    } loc(#loc79)
    return %3 : i32
  }
  func.func @top.cpu_body.0.call.8.12.0.call.8.16(%arg0: !acir.struct<"src_isa_e31afbf3.Instruction"> loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:instruction"), %arg1: i32 loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:pc"), %arg2: i32 loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:lhs"), %arg3: i32 loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:rhs")) -> i32 attributes {acir.function = {definition = "src_isa_e31afbf3.alu", kind = "helper", name = "top.cpu_body.0.call.8.12.0.call.8.16", params = [{id = "v1", name = "instruction", targets = [], type = "src_isa_e31afbf3.Instruction"}, {id = "v2", name = "pc", targets = [], type = "u32"}, {id = "v3", name = "lhs", targets = [], type = "u32"}, {id = "v4", name = "rhs", targets = [], type = "u32"}], result = "u32"}} {
    %c6_i32 = arith.constant 6 : i32
    %c5_i32 = arith.constant 5 : i32
    %c4_i32 = arith.constant 4 : i32
    %c2_i32 = arith.constant 2 : i32
    %c31_i32 = arith.constant 31 : i32
    %c1_i32 = arith.constant 1 : i32
    %c32_i32 = arith.constant 32 : i32
    %c0_i32 = arith.constant 0 : i32
    %c19_i32 = arith.constant 19 : i32
    %c51_i32 = arith.constant 51 : i32
    %c103_i32 = arith.constant 103 : i32
    %c35_i32 = arith.constant 35 : i32
    %c3_i32 = arith.constant 3 : i32
    %c99_i32 = arith.constant 99 : i32
    %c111_i32 = arith.constant 111 : i32
    %c23_i32 = arith.constant 23 : i32
    %c55_i32 = arith.constant 55 : i32 loc(#loc89)
    %false = arith.constant false
    %true = arith.constant true
    %0 = acir.aggregate() : () -> i32
    %1 = acir.extract %arg0 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc90)
    %2 = arith.cmpi eq, %1, %c55_i32 : i32 loc(#loc91)
    %3 = arith.cmpi ne, %1, %c55_i32 : i32 loc(#loc91)
    %4 = scf.if %2 -> (i32) {
      %7 = acir.extract %arg0 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc92)
      scf.yield %7 : i32
    } else {
      scf.yield %0 : i32
    }
    %5:2 = scf.if %3 -> (i1, i32) {
      %7 = arith.cmpi eq, %1, %c23_i32 : i32 loc(#loc93)
      %8 = scf.if %7 -> (i1) {
        scf.yield %true : i1
      } else {
        %13 = arith.cmpi eq, %1, %c111_i32 : i32 loc(#loc94)
        scf.yield %13 : i1
      }
      %9 = scf.if %8 -> (i1) {
        scf.yield %true : i1
      } else {
        %13 = arith.cmpi eq, %1, %c99_i32 : i32 loc(#loc95)
        scf.yield %13 : i1
      }
      %10 = arith.xori %9, %true : i1
      %11 = arith.select %9, %10, %false : i1
      %12 = scf.if %9 -> (i32) {
        %13 = acir.extract %arg0 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc96)
        %14 = arith.addi %arg1, %13 : i32 loc(#loc97)
        scf.yield %14 : i32
      } else {
        %13 = arith.cmpi eq, %1, %c3_i32 : i32 loc(#loc98)
        %14 = scf.if %13 -> (i1) {
          scf.yield %true : i1
        } else {
          %19 = arith.cmpi eq, %1, %c35_i32 : i32 loc(#loc99)
          scf.yield %19 : i1
        }
        %15 = scf.if %14 -> (i1) {
          scf.yield %true : i1
        } else {
          %19 = arith.cmpi eq, %1, %c103_i32 : i32 loc(#loc100)
          scf.yield %19 : i1
        }
        %16 = arith.select %15, %false, %10 : i1
        %17 = scf.if %15 -> (i32) {
          %19 = acir.extract %arg0 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc101)
          %20 = arith.addi %arg2, %19 : i32 loc(#loc102)
          scf.yield %20 : i32
        } else {
          scf.yield %4 : i32
        }
        %18 = scf.if %16 -> (i32) {
          %19 = arith.cmpi eq, %1, %c51_i32 : i32 loc(#loc103)
          %20 = scf.if %19 -> (i1) {
            scf.yield %true : i1
          } else {
            %23 = arith.cmpi eq, %1, %c19_i32 : i32 loc(#loc104)
            scf.yield %23 : i1
          }
          %21 = scf.if %20 -> (i32) {
            %23 = arith.cmpi eq, %1, %c19_i32 : i32 loc(#loc105)
            %24 = scf.if %23 -> (i32) {
              %30 = acir.extract %arg0 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc107)
              scf.yield %30 : i32
            } else {
              scf.yield %arg3 : i32
            } loc(#loc106)
            %25 = acir.extract %arg0 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc108)
            %26 = arith.cmpi eq, %25, %c0_i32 : i32 loc(#loc109)
            %27 = arith.cmpi ne, %25, %c0_i32 : i32 loc(#loc109)
            %28 = scf.if %26 -> (i32) {
              %30 = scf.if %19 -> (i1) {
                %32 = acir.extract %arg0 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc110)
                %33 = arith.cmpi eq, %32, %c32_i32 : i32 loc(#loc110)
                scf.yield %33 : i1
              } else {
                scf.yield %false : i1
              }
              %31 = scf.if %30 -> (i32) {
                %32 = arith.subi %arg2, %24 : i32 loc(#loc111)
                scf.yield %32 : i32
              } else {
                %32 = arith.addi %arg2, %24 : i32 loc(#loc112)
                scf.yield %32 : i32
              }
              scf.yield %31 : i32
            } else {
              scf.yield %17 : i32
            }
            %29 = scf.if %27 -> (i32) {
              %30 = arith.cmpi eq, %25, %c1_i32 : i32 loc(#loc113)
              %31 = arith.cmpi ne, %25, %c1_i32 : i32 loc(#loc113)
              %32 = scf.if %30 -> (i32) {
                %34 = arith.andi %24, %c31_i32 : i32 loc(#loc114)
                %35 = func.call @ac_builtin_shl_u32(%arg2, %34) {acir.id = "v114", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc115)
                scf.yield %35 : i32
              } else {
                scf.yield %28 : i32
              }
              %33 = scf.if %31 -> (i32) {
                %34 = arith.cmpi eq, %25, %c2_i32 : i32 loc(#loc116)
                %35 = arith.cmpi ne, %25, %c2_i32 : i32 loc(#loc116)
                %36 = scf.if %34 -> (i32) {
                  %38 = func.call @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(%arg2, %24) {acir.id = "v125", acir.source_type = "bool", role = "helper"} : (i32, i32) -> i1 loc(#loc117)
                  %39 = arith.extui %38 : i1 to i32
                  scf.yield %39 : i32
                } else {
                  scf.yield %32 : i32
                }
                %37 = scf.if %35 -> (i32) {
                  %38 = arith.cmpi eq, %25, %c3_i32 : i32 loc(#loc118)
                  %39 = arith.cmpi ne, %25, %c3_i32 : i32 loc(#loc118)
                  %40 = scf.if %38 -> (i32) {
                    %42 = arith.cmpi ult, %arg2, %24 : i32 loc(#loc119)
                    %43 = arith.extui %42 : i1 to i32
                    scf.yield %43 : i32
                  } else {
                    scf.yield %36 : i32
                  }
                  %41 = scf.if %39 -> (i32) {
                    %42 = arith.cmpi eq, %25, %c4_i32 : i32 loc(#loc120)
                    %43 = arith.cmpi ne, %25, %c4_i32 : i32 loc(#loc120)
                    %44 = scf.if %42 -> (i32) {
                      %46 = arith.xori %arg2, %24 : i32 loc(#loc121)
                      scf.yield %46 : i32
                    } else {
                      scf.yield %40 : i32
                    }
                    %45 = scf.if %43 -> (i32) {
                      %46 = arith.cmpi eq, %25, %c5_i32 : i32 loc(#loc122)
                      %47 = arith.cmpi ne, %25, %c5_i32 : i32 loc(#loc122)
                      %48 = scf.if %46 -> (i32) {
                        %50 = acir.extract %arg0 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc123)
                        %51 = arith.cmpi eq, %50, %c32_i32 : i32 loc(#loc123)
                        %52 = scf.if %51 -> (i32) {
                          %53 = arith.andi %24, %c31_i32 : i32 loc(#loc124)
                          %54 = func.call @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(%arg2, %53) {acir.id = "v173", acir.source_type = "u32", role = "helper"} : (i32, i32) -> i32 loc(#loc125)
                          scf.yield %54 : i32
                        } else {
                          %53 = arith.andi %24, %c31_i32 : i32 loc(#loc126)
                          %54 = func.call @ac_builtin_shr_u32(%arg2, %53) {acir.id = "v176", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc127)
                          scf.yield %54 : i32
                        }
                        scf.yield %52 : i32
                      } else {
                        scf.yield %44 : i32
                      }
                      %49 = scf.if %47 -> (i32) {
                        %50 = arith.cmpi eq, %25, %c6_i32 : i32 loc(#loc128)
                        %51 = arith.cmpi ne, %25, %c6_i32 : i32 loc(#loc128)
                        %52 = scf.if %50 -> (i32) {
                          %54 = arith.ori %arg2, %24 : i32 loc(#loc129)
                          scf.yield %54 : i32
                        } else {
                          scf.yield %48 : i32
                        }
                        %53 = scf.if %51 -> (i32) {
                          %54 = arith.andi %arg2, %24 : i32 loc(#loc130)
                          scf.yield %54 : i32
                        } else {
                          scf.yield %52 : i32
                        }
                        scf.yield %53 : i32
                      } else {
                        scf.yield %48 : i32
                      }
                      scf.yield %49 : i32
                    } else {
                      scf.yield %44 : i32
                    }
                    scf.yield %45 : i32
                  } else {
                    scf.yield %40 : i32
                  }
                  scf.yield %41 : i32
                } else {
                  scf.yield %36 : i32
                }
                scf.yield %37 : i32
              } else {
                scf.yield %32 : i32
              }
              scf.yield %33 : i32
            } else {
              scf.yield %28 : i32
            }
            scf.yield %29 : i32
          } else {
            scf.yield %17 : i32
          }
          %22 = arith.select %20, %21, %c0_i32 : i32
          scf.yield %22 : i32
        } else {
          scf.yield %17 : i32
        }
        scf.yield %18 : i32
      }
      scf.yield %11, %12 : i1, i32
    } else {
      scf.yield %false, %4 : i1, i32
    }
    %6 = arith.xori %5#0, %true : i1
    acir.check %6 {acir.source_type = "void"}
    return %5#1 : i32
  }
  func.func @top.cpu_body.0.call.8.12.0.call.11.30(%arg0: !acir.struct<"src_isa_e31afbf3.Instruction"> loc("acir.binding:top.cpu_body.0.call.8.12.0.call.11.30:instruction"), %arg1: i32 loc("acir.binding:top.cpu_body.0.call.8.12.0.call.11.30:lhs"), %arg2: i32 loc("acir.binding:top.cpu_body.0.call.8.12.0.call.11.30:rhs")) -> i1 attributes {acir.function = {definition = "src_isa_e31afbf3.branch_taken", kind = "helper", name = "top.cpu_body.0.call.8.12.0.call.11.30", params = [{id = "v1", name = "instruction", targets = [], type = "src_isa_e31afbf3.Instruction"}, {id = "v2", name = "lhs", targets = [], type = "u32"}, {id = "v3", name = "rhs", targets = [], type = "u32"}], result = "bool"}} {
    %true = arith.constant true
    %c7_i32 = arith.constant 7 : i32
    %c6_i32 = arith.constant 6 : i32
    %c5_i32 = arith.constant 5 : i32
    %c4_i32 = arith.constant 4 : i32
    %c1_i32 = arith.constant 1 : i32
    %c0_i32 = arith.constant 0 : i32
    %c99_i32 = arith.constant 99 : i32 loc(#loc134)
    %false = arith.constant false
    %0 = acir.aggregate() : () -> i1
    %1 = acir.extract %arg0 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc135)
    %2 = arith.cmpi ne, %1, %c99_i32 : i32 loc(#loc135)
    %3 = arith.cmpi eq, %1, %c99_i32 : i32 loc(#loc135)
    %4 = arith.select %2, %false, %0 : i1
    %5 = scf.if %3 -> (i1) {
      %6 = acir.extract %arg0 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc136)
      %7 = arith.cmpi eq, %6, %c0_i32 : i32 loc(#loc137)
      %8 = arith.cmpi ne, %6, %c0_i32 : i32 loc(#loc137)
      %9 = scf.if %7 -> (i1) {
        %11 = arith.cmpi eq, %arg1, %arg2 : i32 loc(#loc138)
        scf.yield %11 : i1
      } else {
        scf.yield %4 : i1
      }
      %10 = scf.if %8 -> (i1) {
        %11 = arith.cmpi eq, %6, %c1_i32 : i32 loc(#loc139)
        %12 = arith.cmpi ne, %6, %c1_i32 : i32 loc(#loc139)
        %13 = scf.if %11 -> (i1) {
          %15 = arith.cmpi ne, %arg1, %arg2 : i32 loc(#loc140)
          scf.yield %15 : i1
        } else {
          scf.yield %9 : i1
        }
        %14 = scf.if %12 -> (i1) {
          %15 = arith.cmpi eq, %6, %c4_i32 : i32 loc(#loc141)
          %16 = arith.cmpi ne, %6, %c4_i32 : i32 loc(#loc141)
          %17 = scf.if %15 -> (i1) {
            %19 = func.call @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(%arg1, %arg2) {acir.id = "v45", acir.source_type = "bool", role = "helper"} : (i32, i32) -> i1 loc(#loc142)
            scf.yield %19 : i1
          } else {
            scf.yield %13 : i1
          }
          %18 = scf.if %16 -> (i1) {
            %19 = arith.cmpi eq, %6, %c5_i32 : i32 loc(#loc143)
            %20 = arith.cmpi ne, %6, %c5_i32 : i32 loc(#loc143)
            %21 = scf.if %19 -> (i1) {
              %23 = func.call @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(%arg1, %arg2) {acir.id = "v56", acir.source_type = "bool", role = "helper"} : (i32, i32) -> i1 loc(#loc144)
              %24 = arith.xori %23, %true : i1 loc(#loc145)
              scf.yield %24 : i1
            } else {
              scf.yield %17 : i1
            }
            %22 = scf.if %20 -> (i1) {
              %23 = arith.cmpi eq, %6, %c6_i32 : i32 loc(#loc146)
              %24 = arith.cmpi ne, %6, %c6_i32 : i32 loc(#loc146)
              %25 = scf.if %23 -> (i1) {
                %27 = arith.cmpi ult, %arg1, %arg2 : i32 loc(#loc147)
                scf.yield %27 : i1
              } else {
                scf.yield %21 : i1
              }
              %26 = scf.if %24 -> (i1) {
                %27 = arith.cmpi eq, %6, %c7_i32 : i32 loc(#loc148)
                acir.check %27 {acir.id = "v79", acir.source_type = "void"} loc(#loc149)
                %28 = arith.cmpi uge, %arg1, %arg2 : i32 loc(#loc150)
                scf.yield %28 : i1
              } else {
                scf.yield %25 : i1
              }
              scf.yield %26 : i1
            } else {
              scf.yield %21 : i1
            }
            scf.yield %22 : i1
          } else {
            scf.yield %17 : i1
          }
          scf.yield %18 : i1
        } else {
          scf.yield %13 : i1
        }
        scf.yield %14 : i1
      } else {
        scf.yield %9 : i1
      }
      scf.yield %10 : i1
    } else {
      scf.yield %4 : i1
    }
    return %5 : i1
  }
  func.func @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(%arg0: i32 loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298:value"), %arg1: i32 loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298:bits")) -> i32 attributes {acir.function = {definition = "src_isa_e31afbf3.sign_extend", kind = "helper", name = "top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298", params = [{id = "v1", name = "value", targets = [], type = "u32"}, {id = "v2", name = "bits", targets = [], type = "u32"}], result = "u32"}} {
    %c1_i32 = arith.constant 1 : i32
    %0 = arith.subi %arg1, %c1_i32 : i32 loc(#loc153)
    %1 = call @ac_builtin_shl_u32(%c1_i32, %0) {acir.id = "v9", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc154)
    %2 = arith.xori %arg0, %1 : i32 loc(#loc155)
    %3 = arith.subi %2, %1 : i32 loc(#loc156)
    return %3 : i32
  }
  func.func @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(%arg0: i32 loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6:word")) -> !acir.struct<"src_isa_e31afbf3.Instruction"> attributes {acir.function = {definition = "src_isa_e31afbf3.decode", kind = "helper", name = "top.cpu.decode_stage_body.0.call.14.2.0.call.14.6", params = [{id = "v1", name = "word", targets = [], type = "u32"}], result = "src_isa_e31afbf3.Instruction"}} {
    %c32_i32 = arith.constant 32 : i32
    %c2_i32 = arith.constant 2 : i32
    %c13_i32 = arith.constant 13 : i32
    %c8_i32 = arith.constant 8 : i32
    %c5_i32 = arith.constant 5 : i32
    %c63_i32 = arith.constant 63 : i32
    %c1023_i32 = arith.constant 1023 : i32
    %c21_i32 = arith.constant 21 : i32
    %c11_i32 = arith.constant 11 : i32
    %c1_i32 = arith.constant 1 : i32
    %c255_i32 = arith.constant 255 : i32
    %c-4096_i32 = arith.constant -4096 : i32
    %c103_i32 = arith.constant 103 : i32
    %c111_i32 = arith.constant 111 : i32
    %c23_i32 = arith.constant 23 : i32
    %c55_i32 = arith.constant 55 : i32
    %c3_i32 = arith.constant 3 : i32
    %c19_i32 = arith.constant 19 : i32
    %c20_i32 = arith.constant 20 : i32
    %c99_i32 = arith.constant 99 : i32
    %c35_i32 = arith.constant 35 : i32
    %c51_i32 = arith.constant 51 : i32
    %c15_i32 = arith.constant 15 : i32
    %c31_i32 = arith.constant 31 : i32
    %c25_i32 = arith.constant 25 : i32
    %c7_i32 = arith.constant 7 : i32
    %c12_i32 = arith.constant 12 : i32
    %c127_i32 = arith.constant 127 : i32
    %c0_i32 = arith.constant 0 : i32
    %false = arith.constant false
    %true = arith.constant true
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %false, %false, %false) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
    %1 = arith.cmpi ne, %arg0, %c0_i32 : i32 loc(#loc159)
    %2 = scf.if %1 -> (!acir.struct<"src_isa_e31afbf3.Instruction">) {
      %3 = arith.andi %arg0, %c127_i32 : i32 loc(#loc160)
      %4 = acir.extract %0 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc161)
      %5 = acir.extract %0 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc161)
      %6 = acir.extract %0 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc161)
      %7 = acir.extract %0 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc161)
      %8 = acir.extract %0 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc161)
      %9 = acir.extract %0 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc161)
      %10 = acir.extract %0 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc161)
      %11 = acir.extract %0 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc161)
      %12 = acir.extract %0 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc161)
      %13 = acir.aggregate(%3, %4, %5, %6, %7, %8, %9, %10, %11, %12) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
      %14 = func.call @ac_builtin_shr_u32(%arg0, %c12_i32) {acir.id = "v37", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc162)
      %15 = arith.andi %14, %c7_i32 : i32 loc(#loc162)
      %16 = acir.extract %13 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc163)
      %17 = acir.extract %13 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc163)
      %18 = acir.extract %13 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc163)
      %19 = acir.extract %13 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc163)
      %20 = acir.extract %13 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc163)
      %21 = acir.extract %13 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc163)
      %22 = acir.extract %13 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc163)
      %23 = acir.extract %13 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc163)
      %24 = acir.extract %13 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc163)
      %25 = acir.aggregate(%16, %15, %17, %18, %19, %20, %21, %22, %23, %24) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
      %26 = func.call @ac_builtin_shr_u32(%arg0, %c25_i32) {acir.id = "v54", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc164)
      %27 = acir.extract %25 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc165)
      %28 = acir.extract %25 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc165)
      %29 = acir.extract %25 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc165)
      %30 = acir.extract %25 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc165)
      %31 = acir.extract %25 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc165)
      %32 = acir.extract %25 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc165)
      %33 = acir.extract %25 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc165)
      %34 = acir.extract %25 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc165)
      %35 = acir.extract %25 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc165)
      %36 = acir.aggregate(%27, %28, %26, %29, %30, %31, %32, %33, %34, %35) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
      %37 = func.call @ac_builtin_shr_u32(%arg0, %c7_i32) {acir.id = "v69", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc166)
      %38 = arith.andi %37, %c31_i32 : i32 loc(#loc166)
      %39 = acir.extract %36 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc167)
      %40 = acir.extract %36 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc167)
      %41 = acir.extract %36 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc167)
      %42 = acir.extract %36 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc167)
      %43 = acir.extract %36 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc167)
      %44 = acir.extract %36 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc167)
      %45 = acir.extract %36 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc167)
      %46 = acir.extract %36 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc167)
      %47 = acir.extract %36 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc167)
      %48 = acir.aggregate(%39, %40, %41, %38, %42, %43, %44, %45, %46, %47) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
      %49 = func.call @ac_builtin_shr_u32(%arg0, %c15_i32) {acir.id = "v86", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc168)
      %50 = arith.andi %49, %c31_i32 : i32 loc(#loc168)
      %51 = acir.extract %48 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc169)
      %52 = acir.extract %48 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc169)
      %53 = acir.extract %48 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc169)
      %54 = acir.extract %48 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc169)
      %55 = acir.extract %48 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc169)
      %56 = acir.extract %48 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc169)
      %57 = acir.extract %48 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc169)
      %58 = acir.extract %48 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc169)
      %59 = acir.extract %48 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc169)
      %60 = acir.aggregate(%51, %52, %53, %54, %50, %55, %56, %57, %58, %59) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
      %61 = arith.cmpi eq, %3, %c51_i32 : i32 loc(#loc170)
      %62 = scf.if %61 -> (i1) {
        scf.yield %true : i1
      } else {
        %112 = arith.cmpi eq, %3, %c35_i32 : i32 loc(#loc171)
        scf.yield %112 : i1
      }
      %63 = scf.if %62 -> (i1) {
        scf.yield %true : i1
      } else {
        %112 = arith.cmpi eq, %3, %c99_i32 : i32 loc(#loc172)
        scf.yield %112 : i1
      }
      %64 = scf.if %63 -> (!acir.struct<"src_isa_e31afbf3.Instruction">) {
        %112 = func.call @ac_builtin_shr_u32(%arg0, %c20_i32) {acir.id = "v117", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc173)
        %113 = arith.andi %112, %c31_i32 : i32 loc(#loc173)
        %114 = acir.extract %60 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc174)
        %115 = acir.extract %60 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc174)
        %116 = acir.extract %60 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc174)
        %117 = acir.extract %60 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc174)
        %118 = acir.extract %60 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc174)
        %119 = acir.extract %60 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc174)
        %120 = acir.extract %60 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc174)
        %121 = acir.extract %60 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc174)
        %122 = acir.extract %60 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc174)
        %123 = acir.aggregate(%114, %115, %116, %117, %118, %113, %119, %120, %121, %122) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
        scf.yield %123 : !acir.struct<"src_isa_e31afbf3.Instruction">
      } else {
        scf.yield %60 : !acir.struct<"src_isa_e31afbf3.Instruction">
      } loc(#loc158)
      %65 = scf.if %61 -> (i1) {
        scf.yield %true : i1
      } else {
        %112 = arith.cmpi eq, %3, %c19_i32 : i32 loc(#loc175)
        scf.yield %112 : i1
      }
      %66 = scf.if %65 -> (i1) {
        scf.yield %true : i1
      } else {
        %112 = arith.cmpi eq, %3, %c3_i32 : i32 loc(#loc176)
        scf.yield %112 : i1
      }
      %67 = scf.if %66 -> (i1) {
        scf.yield %true : i1
      } else {
        %112 = arith.cmpi eq, %3, %c55_i32 : i32 loc(#loc177)
        scf.yield %112 : i1
      }
      %68 = scf.if %67 -> (i1) {
        scf.yield %true : i1
      } else {
        %112 = arith.cmpi eq, %3, %c23_i32 : i32 loc(#loc178)
        scf.yield %112 : i1
      }
      %69 = scf.if %68 -> (i1) {
        scf.yield %true : i1
      } else {
        %112 = arith.cmpi eq, %3, %c111_i32 : i32 loc(#loc179)
        scf.yield %112 : i1
      }
      %70 = scf.if %69 -> (i1) {
        scf.yield %true : i1
      } else {
        %112 = arith.cmpi eq, %3, %c103_i32 : i32 loc(#loc180)
        scf.yield %112 : i1
      }
      %71 = acir.extract %64 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc181)
      %72 = acir.extract %64 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc181)
      %73 = acir.extract %64 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc181)
      %74 = acir.extract %64 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc181)
      %75 = acir.extract %64 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc181)
      %76 = acir.extract %64 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc181)
      %77 = acir.extract %64 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc181)
      %78 = acir.extract %64 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc181)
      %79 = acir.extract %64 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc181)
      %80 = acir.aggregate(%71, %72, %73, %74, %75, %76, %77, %70, %78, %79) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
      %81 = arith.cmpi eq, %3, %c3_i32 : i32 loc(#loc182)
      %82 = acir.extract %80 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc183)
      %83 = acir.extract %80 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc183)
      %84 = acir.extract %80 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc183)
      %85 = acir.extract %80 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc183)
      %86 = acir.extract %80 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc183)
      %87 = acir.extract %80 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc183)
      %88 = acir.extract %80 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc183)
      %89 = acir.extract %80 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc183)
      %90 = acir.extract %80 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc183)
      %91 = acir.aggregate(%82, %83, %84, %85, %86, %87, %88, %89, %81, %90) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
      %92 = arith.cmpi eq, %3, %c35_i32 : i32 loc(#loc184)
      %93 = acir.extract %91 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc185)
      %94 = acir.extract %91 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc185)
      %95 = acir.extract %91 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc185)
      %96 = acir.extract %91 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc185)
      %97 = acir.extract %91 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc185)
      %98 = acir.extract %91 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc185)
      %99 = acir.extract %91 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc185)
      %100 = acir.extract %91 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc185)
      %101 = acir.extract %91 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc185)
      %102 = acir.aggregate(%93, %94, %95, %96, %97, %98, %99, %100, %101, %92) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
      %103 = arith.cmpi eq, %3, %c55_i32 : i32 loc(#loc186)
      %104 = scf.if %103 -> (i1) {
        scf.yield %true : i1
      } else {
        %112 = arith.cmpi eq, %3, %c23_i32 : i32 loc(#loc187)
        scf.yield %112 : i1
      }
      %105 = scf.if %104 -> (!acir.struct<"src_isa_e31afbf3.Instruction">) {
        %112 = acir.extract %102 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc188)
        %113 = acir.extract %102 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc188)
        %114 = acir.extract %102 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc188)
        %115 = acir.extract %102 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc188)
        %116 = acir.extract %102 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc188)
        %117 = acir.extract %102 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc188)
        %118 = acir.extract %102 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc188)
        %119 = acir.extract %102 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc188)
        %120 = acir.extract %102 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc188)
        %121 = acir.aggregate(%112, %113, %114, %115, %c0_i32, %116, %117, %118, %119, %120) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
        %122 = arith.andi %arg0, %c-4096_i32 : i32 loc(#loc189)
        %123 = acir.extract %121 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc190)
        %124 = acir.extract %121 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc190)
        %125 = acir.extract %121 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc190)
        %126 = acir.extract %121 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc190)
        %127 = acir.extract %121 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc190)
        %128 = acir.extract %121 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc190)
        %129 = acir.extract %121 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc190)
        %130 = acir.extract %121 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc190)
        %131 = acir.extract %121 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc190)
        %132 = acir.aggregate(%123, %124, %125, %126, %127, %128, %122, %129, %130, %131) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
        scf.yield %132 : !acir.struct<"src_isa_e31afbf3.Instruction">
      } else {
        %112 = arith.cmpi eq, %3, %c111_i32 : i32 loc(#loc191)
        %113 = scf.if %112 -> (!acir.struct<"src_isa_e31afbf3.Instruction">) {
          %114 = acir.extract %102 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc192)
          %115 = acir.extract %102 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc192)
          %116 = acir.extract %102 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc192)
          %117 = acir.extract %102 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc192)
          %118 = acir.extract %102 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc192)
          %119 = acir.extract %102 {field = "immediate"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc192)
          %120 = acir.extract %102 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc192)
          %121 = acir.extract %102 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc192)
          %122 = acir.extract %102 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc192)
          %123 = acir.aggregate(%114, %115, %116, %117, %c0_i32, %118, %119, %120, %121, %122) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
          %124 = func.call @ac_builtin_shr_u32(%arg0, %c31_i32) {acir.id = "v272", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc193)
          %125 = func.call @ac_builtin_shl_u32(%124, %c20_i32) {acir.id = "v274", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc193)
          %126 = func.call @ac_builtin_shr_u32(%arg0, %c12_i32) {acir.id = "v276", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc194)
          %127 = arith.andi %126, %c255_i32 : i32 loc(#loc194)
          %128 = func.call @ac_builtin_shl_u32(%127, %c12_i32) {acir.id = "v280", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc195)
          %129 = arith.ori %125, %128 : i32 loc(#loc196)
          %130 = func.call @ac_builtin_shr_u32(%arg0, %c20_i32) {acir.id = "v283", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc197)
          %131 = arith.andi %130, %c1_i32 : i32 loc(#loc197)
          %132 = func.call @ac_builtin_shl_u32(%131, %c11_i32) {acir.id = "v287", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc198)
          %133 = arith.ori %129, %132 : i32 loc(#loc196)
          %134 = func.call @ac_builtin_shr_u32(%arg0, %c21_i32) {acir.id = "v290", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc199)
          %135 = arith.andi %134, %c1023_i32 : i32 loc(#loc199)
          %136 = func.call @ac_builtin_shl_u32(%135, %c1_i32) {acir.id = "v294", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc200)
          %137 = arith.ori %133, %136 : i32 loc(#loc201)
          %138 = func.call @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(%137, %c21_i32) {acir.id = "v299", acir.source_type = "u32", role = "helper"} : (i32, i32) -> i32 loc(#loc202)
          %139 = acir.extract %123 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc203)
          %140 = acir.extract %123 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc203)
          %141 = acir.extract %123 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc203)
          %142 = acir.extract %123 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc203)
          %143 = acir.extract %123 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc203)
          %144 = acir.extract %123 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc203)
          %145 = acir.extract %123 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc203)
          %146 = acir.extract %123 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc203)
          %147 = acir.extract %123 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc203)
          %148 = acir.aggregate(%139, %140, %141, %142, %143, %144, %138, %145, %146, %147) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
          scf.yield %148 : !acir.struct<"src_isa_e31afbf3.Instruction">
        } else {
          %114 = arith.cmpi eq, %3, %c99_i32 : i32 loc(#loc204)
          %115 = scf.if %114 -> (!acir.struct<"src_isa_e31afbf3.Instruction">) {
            %116 = func.call @ac_builtin_shr_u32(%arg0, %c31_i32) {acir.id = "v315", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc205)
            %117 = func.call @ac_builtin_shl_u32(%116, %c12_i32) {acir.id = "v317", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc205)
            %118 = func.call @ac_builtin_shr_u32(%arg0, %c7_i32) {acir.id = "v319", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc206)
            %119 = arith.andi %118, %c1_i32 : i32 loc(#loc206)
            %120 = func.call @ac_builtin_shl_u32(%119, %c11_i32) {acir.id = "v323", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc207)
            %121 = arith.ori %117, %120 : i32 loc(#loc208)
            %122 = func.call @ac_builtin_shr_u32(%arg0, %c25_i32) {acir.id = "v326", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc209)
            %123 = arith.andi %122, %c63_i32 : i32 loc(#loc209)
            %124 = func.call @ac_builtin_shl_u32(%123, %c5_i32) {acir.id = "v330", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc210)
            %125 = arith.ori %121, %124 : i32 loc(#loc208)
            %126 = func.call @ac_builtin_shr_u32(%arg0, %c8_i32) {acir.id = "v333", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc211)
            %127 = arith.andi %126, %c15_i32 : i32 loc(#loc211)
            %128 = func.call @ac_builtin_shl_u32(%127, %c1_i32) {acir.id = "v337", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc212)
            %129 = arith.ori %125, %128 : i32 loc(#loc201)
            %130 = func.call @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(%129, %c13_i32) {acir.id = "v342", acir.source_type = "u32", role = "helper"} : (i32, i32) -> i32 loc(#loc213)
            %131 = acir.extract %102 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc214)
            %132 = acir.extract %102 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc214)
            %133 = acir.extract %102 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc214)
            %134 = acir.extract %102 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc214)
            %135 = acir.extract %102 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc214)
            %136 = acir.extract %102 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc214)
            %137 = acir.extract %102 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc214)
            %138 = acir.extract %102 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc214)
            %139 = acir.extract %102 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc214)
            %140 = acir.aggregate(%131, %132, %133, %134, %135, %136, %130, %137, %138, %139) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
            scf.yield %140 : !acir.struct<"src_isa_e31afbf3.Instruction">
          } else {
            %116 = scf.if %92 -> (!acir.struct<"src_isa_e31afbf3.Instruction">) {
              %117 = func.call @ac_builtin_shr_u32(%arg0, %c25_i32) {acir.id = "v359", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc215)
              %118 = func.call @ac_builtin_shl_u32(%117, %c5_i32) {acir.id = "v361", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc215)
              %119 = func.call @ac_builtin_shr_u32(%arg0, %c7_i32) {acir.id = "v363", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc216)
              %120 = arith.andi %119, %c31_i32 : i32 loc(#loc216)
              %121 = arith.ori %118, %120 : i32 loc(#loc217)
              %122 = func.call @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(%121, %c12_i32) {acir.id = "v368", acir.source_type = "u32", role = "helper"} : (i32, i32) -> i32 loc(#loc218)
              %123 = acir.extract %102 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc219)
              %124 = acir.extract %102 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc219)
              %125 = acir.extract %102 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc219)
              %126 = acir.extract %102 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc219)
              %127 = acir.extract %102 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc219)
              %128 = acir.extract %102 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc219)
              %129 = acir.extract %102 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc219)
              %130 = acir.extract %102 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc219)
              %131 = acir.extract %102 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc219)
              %132 = acir.aggregate(%123, %124, %125, %126, %127, %128, %122, %129, %130, %131) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
              scf.yield %132 : !acir.struct<"src_isa_e31afbf3.Instruction">
            } else {
              %117 = arith.cmpi eq, %3, %c19_i32 : i32 loc(#loc220)
              %118 = scf.if %117 -> (i1) {
                scf.yield %true : i1
              } else {
                scf.yield %81 : i1
              }
              %119 = scf.if %118 -> (i1) {
                scf.yield %true : i1
              } else {
                %121 = arith.cmpi eq, %3, %c103_i32 : i32 loc(#loc221)
                scf.yield %121 : i1
              }
              %120 = scf.if %119 -> (!acir.struct<"src_isa_e31afbf3.Instruction">) {
                %121 = func.call @ac_builtin_shr_u32(%arg0, %c20_i32) {acir.id = "v397", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc222)
                %122 = func.call @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(%121, %c12_i32) {acir.id = "v399", acir.source_type = "u32", role = "helper"} : (i32, i32) -> i32 loc(#loc223)
                %123 = acir.extract %102 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc224)
                %124 = acir.extract %102 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc224)
                %125 = acir.extract %102 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc224)
                %126 = acir.extract %102 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc224)
                %127 = acir.extract %102 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc224)
                %128 = acir.extract %102 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc224)
                %129 = acir.extract %102 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc224)
                %130 = acir.extract %102 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc224)
                %131 = acir.extract %102 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc224)
                %132 = acir.aggregate(%123, %124, %125, %126, %127, %128, %122, %129, %130, %131) : (i32, i32, i32, i32, i32, i32, i32, i1, i1, i1) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc158)
                scf.yield %132 : !acir.struct<"src_isa_e31afbf3.Instruction">
              } else {
                acir.check %61 {acir.id = "v414", acir.source_type = "void"} loc(#loc225)
                scf.yield %102 : !acir.struct<"src_isa_e31afbf3.Instruction">
              } loc(#loc158)
              scf.yield %120 : !acir.struct<"src_isa_e31afbf3.Instruction">
            } loc(#loc158)
            scf.yield %116 : !acir.struct<"src_isa_e31afbf3.Instruction">
          } loc(#loc158)
          scf.yield %115 : !acir.struct<"src_isa_e31afbf3.Instruction">
        } loc(#loc158)
        scf.yield %113 : !acir.struct<"src_isa_e31afbf3.Instruction">
      } loc(#loc158)
      %106 = acir.extract %105 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc226)
      %107 = scf.if %106 -> (i1) {
        scf.yield %true : i1
      } else {
        %112 = acir.extract %105 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc227)
        scf.yield %112 : i1
      }
      scf.if %107 {
        %112 = acir.extract %105 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc228)
        %113 = arith.cmpi eq, %112, %c2_i32 : i32 loc(#loc228)
        acir.check %113 {acir.id = "v449", acir.source_type = "void"} loc(#loc229)
      }
      scf.if %61 {
        %112 = acir.extract %105 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc230)
        %113 = arith.cmpi eq, %112, %c0_i32 : i32 loc(#loc230)
        %114 = scf.if %113 -> (i1) {
          scf.yield %true : i1
        } else {
          %115 = arith.cmpi eq, %112, %c32_i32 : i32 loc(#loc231)
          %116 = scf.if %115 -> (i1) {
            %117 = acir.extract %105 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc232)
            %118 = arith.cmpi eq, %117, %c0_i32 : i32 loc(#loc232)
            %119 = scf.if %118 -> (i1) {
              scf.yield %true : i1
            } else {
              %120 = arith.cmpi eq, %117, %c5_i32 : i32 loc(#loc233)
              scf.yield %120 : i1
            }
            scf.yield %119 : i1
          } else {
            scf.yield %false : i1
          }
          scf.yield %116 : i1
        }
        acir.check %114 {acir.id = "v479", acir.source_type = "void"} loc(#loc234)
      }
      %108 = arith.cmpi eq, %3, %c103_i32 : i32 loc(#loc235)
      scf.if %108 {
        %112 = acir.extract %105 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc236)
        %113 = arith.cmpi eq, %112, %c0_i32 : i32 loc(#loc236)
        acir.check %113 {acir.id = "v488", acir.source_type = "void"} loc(#loc237)
      }
      %109 = arith.cmpi eq, %3, %c19_i32 : i32 loc(#loc238)
      %110 = scf.if %109 -> (i1) {
        %112 = acir.extract %105 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc239)
        %113 = arith.cmpi eq, %112, %c1_i32 : i32 loc(#loc239)
        scf.yield %113 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %110 {
        %112 = acir.extract %105 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc240)
        %113 = arith.cmpi eq, %112, %c0_i32 : i32 loc(#loc240)
        acir.check %113 {acir.id = "v504", acir.source_type = "void"} loc(#loc241)
      }
      %111 = scf.if %109 -> (i1) {
        %112 = acir.extract %105 {field = "funct3"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc242)
        %113 = arith.cmpi eq, %112, %c5_i32 : i32 loc(#loc242)
        scf.yield %113 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %111 {
        %112 = acir.extract %105 {field = "funct7"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc243)
        %113 = arith.cmpi eq, %112, %c0_i32 : i32 loc(#loc243)
        %114 = scf.if %113 -> (i1) {
          scf.yield %true : i1
        } else {
          %115 = arith.cmpi eq, %112, %c32_i32 : i32 loc(#loc244)
          scf.yield %115 : i1
        }
        acir.check %114 {acir.id = "v527", acir.source_type = "void"} loc(#loc245)
      }
      scf.yield %105 : !acir.struct<"src_isa_e31afbf3.Instruction">
    } else {
      scf.yield %0 : !acir.struct<"src_isa_e31afbf3.Instruction">
    } loc(#loc158)
    return %2 : !acir.struct<"src_isa_e31afbf3.Instruction">
  }
  func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  acir.module @top attributes {acir.instance = {definition = "src_demo_7dc0ad19.Demo", name = "top", parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    func.func private @ac_init_top.cpu.rf.registers.0() -> i32
    func.func private @ac_init_top.cpu.rf.registers.1() -> i32
    func.func private @ac_init_top.cpu.rf.registers.2() -> i32
    func.func private @ac_init_top.cpu.rf.registers.3() -> i32
    func.func private @ac_init_top.cpu.rf.registers.4() -> i32
    func.func private @ac_init_top.cpu.rf.registers.5() -> i32
    func.func private @ac_init_top.cpu.rf.registers.6() -> i32
    func.func private @ac_init_top.cpu.rf.registers.7() -> i32
    func.func private @ac_init_top.cpu.rf.registers.8() -> i32
    func.func private @ac_init_top.cpu.rf.registers.9() -> i32
    func.func private @ac_init_top.cpu.rf.registers.10() -> i32
    func.func private @ac_init_top.cpu.rf.registers.11() -> i32
    func.func private @ac_init_top.cpu.rf.registers.12() -> i32
    func.func private @ac_init_top.cpu.rf.registers.13() -> i32
    func.func private @ac_init_top.cpu.rf.registers.14() -> i32
    func.func private @ac_init_top.cpu.rf.registers.15() -> i32
    func.func private @ac_init_top.cpu.rf.registers.16() -> i32
    func.func private @ac_init_top.cpu.rf.registers.17() -> i32
    func.func private @ac_init_top.cpu.rf.registers.18() -> i32
    func.func private @ac_init_top.cpu.rf.registers.19() -> i32
    func.func private @ac_init_top.cpu.rf.registers.20() -> i32
    func.func private @ac_init_top.cpu.rf.registers.21() -> i32
    func.func private @ac_init_top.cpu.rf.registers.22() -> i32
    func.func private @ac_init_top.cpu.rf.registers.23() -> i32
    func.func private @ac_init_top.cpu.rf.registers.24() -> i32
    func.func private @ac_init_top.cpu.rf.registers.25() -> i32
    func.func private @ac_init_top.cpu.rf.registers.26() -> i32
    func.func private @ac_init_top.cpu.rf.registers.27() -> i32
    func.func private @ac_init_top.cpu.rf.registers.28() -> i32
    func.func private @ac_init_top.cpu.rf.registers.29() -> i32
    func.func private @ac_init_top.cpu.rf.registers.30() -> i32
    func.func private @ac_init_top.cpu.rf.registers.31() -> i32
    func.func private @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched">
    func.func private @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded">
    func.func private @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed">
    func.func private @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed">
    func.func private @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]>
    func.func private @ac_init_top.cpu.fetch.pc() -> i32
    func.func private @ac_init_top.cpu.memory.words.0() -> i32
    func.func private @ac_init_top.cpu.memory.words.1() -> i32
    func.func private @ac_init_top.cpu.memory.words.2() -> i32
    func.func private @ac_init_top.cpu.memory.words.3() -> i32
    func.func private @ac_init_top.cpu.memory.words.4() -> i32
    func.func private @ac_init_top.cpu.memory.words.5() -> i32
    func.func private @ac_init_top.cpu.memory.words.6() -> i32
    func.func private @ac_init_top.cpu.memory.words.7() -> i32
    func.func private @ac_init_top.cpu.memory.words.8() -> i32
    func.func private @ac_init_top.cpu.memory.words.9() -> i32
    func.func private @ac_init_top.cpu.memory.words.10() -> i32
    func.func private @ac_init_top.cpu.memory.words.11() -> i32
    func.func private @ac_init_top.cpu.memory.words.12() -> i32
    func.func private @ac_init_top.cpu.memory.words.13() -> i32
    func.func private @ac_init_top.cpu.memory.words.14() -> i32
    func.func private @ac_init_top.cpu.memory.words.15() -> i32
    func.func private @ac_init_top.cpu.memory.words.16() -> i32
    func.func private @ac_init_top.cpu.memory.words.17() -> i32
    func.func private @ac_init_top.cpu.memory.words.18() -> i32
    func.func private @ac_init_top.cpu.memory.words.19() -> i32
    func.func private @ac_init_top.cpu.memory.words.20() -> i32
    func.func private @ac_init_top.cpu.memory.words.21() -> i32
    func.func private @ac_init_top.cpu.memory.words.22() -> i32
    func.func private @ac_init_top.cpu.memory.words.23() -> i32
    func.func private @ac_init_top.cpu.memory.words.24() -> i32
    func.func private @ac_init_top.cpu.memory.words.25() -> i32
    func.func private @ac_init_top.cpu.memory.words.26() -> i32
    func.func private @ac_init_top.cpu.memory.words.27() -> i32
    func.func private @ac_init_top.cpu.memory.words.28() -> i32
    func.func private @ac_init_top.cpu.memory.words.29() -> i32
    func.func private @ac_init_top.cpu.memory.words.30() -> i32
    func.func private @ac_init_top.cpu.memory.words.31() -> i32
    func.func private @ac_init_top.cpu.memory.words.32() -> i32
    func.func private @ac_init_top.cpu.memory.words.33() -> i32
    func.func private @ac_init_top.cpu.memory.words.34() -> i32
    func.func private @ac_init_top.cpu.memory.words.35() -> i32
    func.func private @ac_init_top.cpu.memory.words.36() -> i32
    func.func private @ac_init_top.cpu.memory.words.37() -> i32
    func.func private @ac_init_top.cpu.memory.words.38() -> i32
    func.func private @ac_init_top.cpu.memory.words.39() -> i32
    func.func private @ac_init_top.cpu.memory.words.40() -> i32
    func.func private @ac_init_top.cpu.memory.words.41() -> i32
    func.func private @ac_init_top.cpu.memory.words.42() -> i32
    func.func private @ac_init_top.cpu.memory.words.43() -> i32
    func.func private @ac_init_top.cpu.memory.words.44() -> i32
    func.func private @ac_init_top.cpu.memory.words.45() -> i32
    func.func private @ac_init_top.cpu.memory.words.46() -> i32
    func.func private @ac_init_top.cpu.memory.words.47() -> i32
    func.func private @ac_init_top.cpu.memory.words.48() -> i32
    func.func private @ac_init_top.cpu.memory.words.49() -> i32
    func.func private @ac_init_top.cpu.memory.words.50() -> i32
    func.func private @ac_init_top.cpu.memory.words.51() -> i32
    func.func private @ac_init_top.cpu.memory.words.52() -> i32
    func.func private @ac_init_top.cpu.memory.words.53() -> i32
    func.func private @ac_init_top.cpu.memory.words.54() -> i32
    func.func private @ac_init_top.cpu.memory.words.55() -> i32
    func.func private @ac_init_top.cpu.memory.words.56() -> i32
    func.func private @ac_init_top.cpu.memory.words.57() -> i32
    func.func private @ac_init_top.cpu.memory.words.58() -> i32
    func.func private @ac_init_top.cpu.memory.words.59() -> i32
    func.func private @ac_init_top.cpu.memory.words.60() -> i32
    func.func private @ac_init_top.cpu.memory.words.61() -> i32
    func.func private @ac_init_top.cpu.memory.words.62() -> i32
    func.func private @ac_init_top.cpu.memory.words.63() -> i32
    func.func private @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent">
    func.func private @ac_init_top.cpu.writeback.retired() -> i32
    func.func private @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement">
    func.func private @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control">
    func.func private @construct.0.call.0.0() -> !acir.array<i32, [256]>
    func.func private @construct.0.call.1.1() -> !acir.array<i32, [32]>
    func.func private @construct.0.call.2.2() -> !acir.array<i32, [64]>
    func.func private @top.cpu_body.0.call.5.9.0.call.5.16(i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.5.9.0.call.6.23(i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(i32, i32) -> i1
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.11.30(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(i32, i32) -> i32
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(i32) -> !acir.struct<"src_isa_e31afbf3.Instruction">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant true
      acir.module.call @top.cpu loc(#loc246)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.cpu attributes {acir.instance = {definition = "src_model_bb8d42ed.Ripes5", name = "top.cpu", parent = "top", rules = ["observe"]}} {
    acir.contract {relations = [], rules = ["observe"]}
    acir.cell @top.cpu.if_id : !acir.struct<"src_isa_e31afbf3.Fetched"> {initial_fn = "ac_init_top.cpu.if_id", name = "top.cpu.if_id", type = "src_isa_e31afbf3.Fetched"}
    acir.cell @top.cpu.id_ex : !acir.struct<"src_isa_e31afbf3.Decoded"> {initial_fn = "ac_init_top.cpu.id_ex", name = "top.cpu.id_ex", type = "src_isa_e31afbf3.Decoded"}
    acir.cell @top.cpu.ex_mem : !acir.struct<"src_isa_e31afbf3.Executed"> {initial_fn = "ac_init_top.cpu.ex_mem", name = "top.cpu.ex_mem", type = "src_isa_e31afbf3.Executed"}
    acir.cell @top.cpu.mem_wb : !acir.struct<"src_isa_e31afbf3.Completed"> {initial_fn = "ac_init_top.cpu.mem_wb", name = "top.cpu.mem_wb", type = "src_isa_e31afbf3.Completed"}
    acir.cell @top.cpu.last_control : !acir.struct<"src_isa_e31afbf3.Control"> {initial_fn = "ac_init_top.cpu.last_control", name = "top.cpu.last_control", type = "src_isa_e31afbf3.Control"}
    func.func private @ac_init_top.cpu.rf.registers.0() -> i32
    func.func private @ac_init_top.cpu.rf.registers.1() -> i32
    func.func private @ac_init_top.cpu.rf.registers.2() -> i32
    func.func private @ac_init_top.cpu.rf.registers.3() -> i32
    func.func private @ac_init_top.cpu.rf.registers.4() -> i32
    func.func private @ac_init_top.cpu.rf.registers.5() -> i32
    func.func private @ac_init_top.cpu.rf.registers.6() -> i32
    func.func private @ac_init_top.cpu.rf.registers.7() -> i32
    func.func private @ac_init_top.cpu.rf.registers.8() -> i32
    func.func private @ac_init_top.cpu.rf.registers.9() -> i32
    func.func private @ac_init_top.cpu.rf.registers.10() -> i32
    func.func private @ac_init_top.cpu.rf.registers.11() -> i32
    func.func private @ac_init_top.cpu.rf.registers.12() -> i32
    func.func private @ac_init_top.cpu.rf.registers.13() -> i32
    func.func private @ac_init_top.cpu.rf.registers.14() -> i32
    func.func private @ac_init_top.cpu.rf.registers.15() -> i32
    func.func private @ac_init_top.cpu.rf.registers.16() -> i32
    func.func private @ac_init_top.cpu.rf.registers.17() -> i32
    func.func private @ac_init_top.cpu.rf.registers.18() -> i32
    func.func private @ac_init_top.cpu.rf.registers.19() -> i32
    func.func private @ac_init_top.cpu.rf.registers.20() -> i32
    func.func private @ac_init_top.cpu.rf.registers.21() -> i32
    func.func private @ac_init_top.cpu.rf.registers.22() -> i32
    func.func private @ac_init_top.cpu.rf.registers.23() -> i32
    func.func private @ac_init_top.cpu.rf.registers.24() -> i32
    func.func private @ac_init_top.cpu.rf.registers.25() -> i32
    func.func private @ac_init_top.cpu.rf.registers.26() -> i32
    func.func private @ac_init_top.cpu.rf.registers.27() -> i32
    func.func private @ac_init_top.cpu.rf.registers.28() -> i32
    func.func private @ac_init_top.cpu.rf.registers.29() -> i32
    func.func private @ac_init_top.cpu.rf.registers.30() -> i32
    func.func private @ac_init_top.cpu.rf.registers.31() -> i32
    func.func private @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched">
    func.func private @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded">
    func.func private @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed">
    func.func private @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed">
    func.func private @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]>
    func.func private @ac_init_top.cpu.fetch.pc() -> i32
    func.func private @ac_init_top.cpu.memory.words.0() -> i32
    func.func private @ac_init_top.cpu.memory.words.1() -> i32
    func.func private @ac_init_top.cpu.memory.words.2() -> i32
    func.func private @ac_init_top.cpu.memory.words.3() -> i32
    func.func private @ac_init_top.cpu.memory.words.4() -> i32
    func.func private @ac_init_top.cpu.memory.words.5() -> i32
    func.func private @ac_init_top.cpu.memory.words.6() -> i32
    func.func private @ac_init_top.cpu.memory.words.7() -> i32
    func.func private @ac_init_top.cpu.memory.words.8() -> i32
    func.func private @ac_init_top.cpu.memory.words.9() -> i32
    func.func private @ac_init_top.cpu.memory.words.10() -> i32
    func.func private @ac_init_top.cpu.memory.words.11() -> i32
    func.func private @ac_init_top.cpu.memory.words.12() -> i32
    func.func private @ac_init_top.cpu.memory.words.13() -> i32
    func.func private @ac_init_top.cpu.memory.words.14() -> i32
    func.func private @ac_init_top.cpu.memory.words.15() -> i32
    func.func private @ac_init_top.cpu.memory.words.16() -> i32
    func.func private @ac_init_top.cpu.memory.words.17() -> i32
    func.func private @ac_init_top.cpu.memory.words.18() -> i32
    func.func private @ac_init_top.cpu.memory.words.19() -> i32
    func.func private @ac_init_top.cpu.memory.words.20() -> i32
    func.func private @ac_init_top.cpu.memory.words.21() -> i32
    func.func private @ac_init_top.cpu.memory.words.22() -> i32
    func.func private @ac_init_top.cpu.memory.words.23() -> i32
    func.func private @ac_init_top.cpu.memory.words.24() -> i32
    func.func private @ac_init_top.cpu.memory.words.25() -> i32
    func.func private @ac_init_top.cpu.memory.words.26() -> i32
    func.func private @ac_init_top.cpu.memory.words.27() -> i32
    func.func private @ac_init_top.cpu.memory.words.28() -> i32
    func.func private @ac_init_top.cpu.memory.words.29() -> i32
    func.func private @ac_init_top.cpu.memory.words.30() -> i32
    func.func private @ac_init_top.cpu.memory.words.31() -> i32
    func.func private @ac_init_top.cpu.memory.words.32() -> i32
    func.func private @ac_init_top.cpu.memory.words.33() -> i32
    func.func private @ac_init_top.cpu.memory.words.34() -> i32
    func.func private @ac_init_top.cpu.memory.words.35() -> i32
    func.func private @ac_init_top.cpu.memory.words.36() -> i32
    func.func private @ac_init_top.cpu.memory.words.37() -> i32
    func.func private @ac_init_top.cpu.memory.words.38() -> i32
    func.func private @ac_init_top.cpu.memory.words.39() -> i32
    func.func private @ac_init_top.cpu.memory.words.40() -> i32
    func.func private @ac_init_top.cpu.memory.words.41() -> i32
    func.func private @ac_init_top.cpu.memory.words.42() -> i32
    func.func private @ac_init_top.cpu.memory.words.43() -> i32
    func.func private @ac_init_top.cpu.memory.words.44() -> i32
    func.func private @ac_init_top.cpu.memory.words.45() -> i32
    func.func private @ac_init_top.cpu.memory.words.46() -> i32
    func.func private @ac_init_top.cpu.memory.words.47() -> i32
    func.func private @ac_init_top.cpu.memory.words.48() -> i32
    func.func private @ac_init_top.cpu.memory.words.49() -> i32
    func.func private @ac_init_top.cpu.memory.words.50() -> i32
    func.func private @ac_init_top.cpu.memory.words.51() -> i32
    func.func private @ac_init_top.cpu.memory.words.52() -> i32
    func.func private @ac_init_top.cpu.memory.words.53() -> i32
    func.func private @ac_init_top.cpu.memory.words.54() -> i32
    func.func private @ac_init_top.cpu.memory.words.55() -> i32
    func.func private @ac_init_top.cpu.memory.words.56() -> i32
    func.func private @ac_init_top.cpu.memory.words.57() -> i32
    func.func private @ac_init_top.cpu.memory.words.58() -> i32
    func.func private @ac_init_top.cpu.memory.words.59() -> i32
    func.func private @ac_init_top.cpu.memory.words.60() -> i32
    func.func private @ac_init_top.cpu.memory.words.61() -> i32
    func.func private @ac_init_top.cpu.memory.words.62() -> i32
    func.func private @ac_init_top.cpu.memory.words.63() -> i32
    func.func private @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent">
    func.func private @ac_init_top.cpu.writeback.retired() -> i32
    func.func private @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement">
    func.func private @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control">
    func.func private @construct.0.call.0.0() -> !acir.array<i32, [256]>
    func.func private @construct.0.call.1.1() -> !acir.array<i32, [32]>
    func.func private @construct.0.call.2.2() -> !acir.array<i32, [64]>
    func.func private @top.cpu_body.0.call.5.9.0.call.5.16(i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.5.9.0.call.6.23(i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(i32, i32) -> i1
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.11.30(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(i32, i32) -> i32
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(i32) -> !acir.struct<"src_isa_e31afbf3.Instruction">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.cpu_body.0.call.23.34 : (!acir.struct<"src_isa_e31afbf3.Control">) -> () {acir.function = {contract = "Module:top.cpu", definition = "observe", instance = "top.cpu", interface = "Module:top.cpu:observe", kind = "rule", name = "top.cpu_body.0.call.23.34", output_type = "void", owner = "top.cpu", params = [{id = "v1", name = "control", targets = [], type = "src_isa_e31afbf3.Control"}], public = true, result = "void", rule = "observe"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_e31afbf3.Control"> loc("acir.binding:top.cpu_body.0.call.23.34:control")):
      %0 = acir.ref @top.cpu::@top.cpu.last_control : !acir.cell<!acir.struct<"src_isa_e31afbf3.Control">>
      acir.cell.write %0, %arg0 {acir.id = "v6", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Control">>, !acir.struct<"src_isa_e31afbf3.Control"> loc(#loc248)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.cpu_body", owner = "top.cpu", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.rule.call @top.cpu.decode_stage::@top.cpu_body.0.call.3.1() {acir.id = "v2", acir.source_type = "src_isa_e31afbf3.Instruction", contract = "Module:top.cpu.decode_stage", instance = "top.cpu.decode_stage", interface = "Module:top.cpu.decode_stage:inspect", role = "rule", rule = "inspect"} : () -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc249)
      %1 = acir.ref @top.cpu::@top.cpu.id_ex : !acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">>
      %2 = acir.rule.call @top.cpu.hazard_unit::@top.cpu_body.0.call.4.4(%1, %0) {acir.id = "v5", acir.source_type = "bool", contract = "Module:top.cpu.hazard_unit", instance = "top.cpu.hazard_unit", interface = "Module:top.cpu.hazard_unit:load_use", role = "rule", rule = "load_use"} : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">>, !acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc250)
      %3 = acir.ref @top.cpu::@top.cpu.id_ex : !acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">>
      %4 = acir.ref @top.cpu::@top.cpu.ex_mem : !acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">>
      %5 = acir.ref @top.cpu::@top.cpu.mem_wb : !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>
      %6 = acir.rule.call @top.cpu.forwarding_unit::@top.cpu_body.0.call.5.9(%3, %4, %5) {acir.id = "v10", acir.source_type = "src_isa_e31afbf3.ForwardedOperands", contract = "Module:top.cpu.forwarding_unit", instance = "top.cpu.forwarding_unit", interface = "Module:top.cpu.forwarding_unit:resolve", role = "rule", rule = "resolve"} : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">>, !acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">>, !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>) -> !acir.struct<"src_isa_e31afbf3.ForwardedOperands"> loc(#loc251)
      %7 = acir.ref @top.cpu::@top.cpu.id_ex : !acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">>
      %8 = acir.rule.call @top.cpu.execute::@top.cpu_body.0.call.8.12(%7, %6) {acir.id = "v13", acir.source_type = "src_isa_e31afbf3.ExecutionResult", contract = "Module:top.cpu.execute", instance = "top.cpu.execute", interface = "Module:top.cpu.execute:resolve", role = "rule", rule = "resolve"} : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">>, !acir.struct<"src_isa_e31afbf3.ForwardedOperands">) -> !acir.struct<"src_isa_e31afbf3.ExecutionResult"> loc(#loc252)
      %9 = acir.extract %8 {field = "redirect"} : (!acir.struct<"src_isa_e31afbf3.ExecutionResult">) -> i1 loc(#loc253)
      %10 = acir.extract %8 {field = "target"} : (!acir.struct<"src_isa_e31afbf3.ExecutionResult">) -> i32 loc(#loc254)
      %11 = acir.extract %6 {field = "source_a"} : (!acir.struct<"src_isa_e31afbf3.ForwardedOperands">) -> i32 loc(#loc255)
      %12 = acir.extract %6 {field = "source_b"} : (!acir.struct<"src_isa_e31afbf3.ForwardedOperands">) -> i32 loc(#loc256)
      %13 = acir.aggregate(%2, %9, %10, %11, %12) : (i1, i1, i32, i32, i32) -> !acir.struct<"src_isa_e31afbf3.Control"> loc(#loc257)
      %14 = acir.export %13 {acir.id = "v21", acir.source_type = "src_isa_e31afbf3.Control"} : !acir.struct<"src_isa_e31afbf3.Control"> -> !acir.struct<"src_isa_e31afbf3.Control">
      acir.module.call @top.cpu.fetch loc(#loc258)
      %15 = acir.ref @top.cpu::@top.cpu.if_id : !acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">>
      %16 = acir.ref @top.cpu::@top.cpu.mem_wb : !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>
      acir.module.call @top.cpu.decode_stage loc(#loc259)
      %17 = acir.extract %8 {field = "packet"} : (!acir.struct<"src_isa_e31afbf3.ExecutionResult">) -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc260)
      %18 = acir.export %17 {acir.id = "v27", acir.source_type = "src_isa_e31afbf3.Executed"} : !acir.struct<"src_isa_e31afbf3.Executed"> -> !acir.struct<"src_isa_e31afbf3.Executed">
      acir.module.call @top.cpu.execute loc(#loc261)
      %19 = acir.ref @top.cpu::@top.cpu.ex_mem : !acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">>
      acir.module.call @top.cpu.memory loc(#loc262)
      %20 = acir.ref @top.cpu::@top.cpu.mem_wb : !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>
      %c84_i32 = arith.constant 84 : i32 loc(#loc263)
      %21 = acir.export %c84_i32 {acir.id = "v33", acir.source_type = "u32"} : i32 -> i32
      acir.module.call @top.cpu.writeback loc(#loc264)
      acir.rule.call @top.cpu::@top.cpu_body.0.call.23.34(%13) {acir.id = "v35", acir.source_type = "void", contract = "Module:top.cpu", instance = "top.cpu", interface = "Module:top.cpu:observe", role = "rule", rule = "observe"} : (!acir.struct<"src_isa_e31afbf3.Control">) -> () loc(#loc265)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.cpu.rf attributes {acir.instance = {definition = "src_model_bb8d42ed.RegisterFile", name = "top.cpu.rf", parent = "top.cpu", rules = ["read", "write"]}} {
    acir.contract {relations = [], rules = ["read", "write"]}
    acir.cell @top.cpu.rf.registers.0 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.0", name = "top.cpu.rf.registers.0", type = "u32"}
    acir.cell @top.cpu.rf.registers.1 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.1", name = "top.cpu.rf.registers.1", type = "u32"}
    acir.cell @top.cpu.rf.registers.2 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.2", name = "top.cpu.rf.registers.2", type = "u32"}
    acir.cell @top.cpu.rf.registers.3 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.3", name = "top.cpu.rf.registers.3", type = "u32"}
    acir.cell @top.cpu.rf.registers.4 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.4", name = "top.cpu.rf.registers.4", type = "u32"}
    acir.cell @top.cpu.rf.registers.5 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.5", name = "top.cpu.rf.registers.5", type = "u32"}
    acir.cell @top.cpu.rf.registers.6 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.6", name = "top.cpu.rf.registers.6", type = "u32"}
    acir.cell @top.cpu.rf.registers.7 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.7", name = "top.cpu.rf.registers.7", type = "u32"}
    acir.cell @top.cpu.rf.registers.8 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.8", name = "top.cpu.rf.registers.8", type = "u32"}
    acir.cell @top.cpu.rf.registers.9 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.9", name = "top.cpu.rf.registers.9", type = "u32"}
    acir.cell @top.cpu.rf.registers.10 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.10", name = "top.cpu.rf.registers.10", type = "u32"}
    acir.cell @top.cpu.rf.registers.11 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.11", name = "top.cpu.rf.registers.11", type = "u32"}
    acir.cell @top.cpu.rf.registers.12 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.12", name = "top.cpu.rf.registers.12", type = "u32"}
    acir.cell @top.cpu.rf.registers.13 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.13", name = "top.cpu.rf.registers.13", type = "u32"}
    acir.cell @top.cpu.rf.registers.14 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.14", name = "top.cpu.rf.registers.14", type = "u32"}
    acir.cell @top.cpu.rf.registers.15 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.15", name = "top.cpu.rf.registers.15", type = "u32"}
    acir.cell @top.cpu.rf.registers.16 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.16", name = "top.cpu.rf.registers.16", type = "u32"}
    acir.cell @top.cpu.rf.registers.17 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.17", name = "top.cpu.rf.registers.17", type = "u32"}
    acir.cell @top.cpu.rf.registers.18 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.18", name = "top.cpu.rf.registers.18", type = "u32"}
    acir.cell @top.cpu.rf.registers.19 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.19", name = "top.cpu.rf.registers.19", type = "u32"}
    acir.cell @top.cpu.rf.registers.20 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.20", name = "top.cpu.rf.registers.20", type = "u32"}
    acir.cell @top.cpu.rf.registers.21 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.21", name = "top.cpu.rf.registers.21", type = "u32"}
    acir.cell @top.cpu.rf.registers.22 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.22", name = "top.cpu.rf.registers.22", type = "u32"}
    acir.cell @top.cpu.rf.registers.23 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.23", name = "top.cpu.rf.registers.23", type = "u32"}
    acir.cell @top.cpu.rf.registers.24 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.24", name = "top.cpu.rf.registers.24", type = "u32"}
    acir.cell @top.cpu.rf.registers.25 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.25", name = "top.cpu.rf.registers.25", type = "u32"}
    acir.cell @top.cpu.rf.registers.26 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.26", name = "top.cpu.rf.registers.26", type = "u32"}
    acir.cell @top.cpu.rf.registers.27 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.27", name = "top.cpu.rf.registers.27", type = "u32"}
    acir.cell @top.cpu.rf.registers.28 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.28", name = "top.cpu.rf.registers.28", type = "u32"}
    acir.cell @top.cpu.rf.registers.29 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.29", name = "top.cpu.rf.registers.29", type = "u32"}
    acir.cell @top.cpu.rf.registers.30 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.30", name = "top.cpu.rf.registers.30", type = "u32"}
    acir.cell @top.cpu.rf.registers.31 : i32 {initial_fn = "ac_init_top.cpu.rf.registers.31", name = "top.cpu.rf.registers.31", type = "u32"}
    func.func private @ac_init_top.cpu.rf.registers.0() -> i32
    func.func private @ac_init_top.cpu.rf.registers.1() -> i32
    func.func private @ac_init_top.cpu.rf.registers.2() -> i32
    func.func private @ac_init_top.cpu.rf.registers.3() -> i32
    func.func private @ac_init_top.cpu.rf.registers.4() -> i32
    func.func private @ac_init_top.cpu.rf.registers.5() -> i32
    func.func private @ac_init_top.cpu.rf.registers.6() -> i32
    func.func private @ac_init_top.cpu.rf.registers.7() -> i32
    func.func private @ac_init_top.cpu.rf.registers.8() -> i32
    func.func private @ac_init_top.cpu.rf.registers.9() -> i32
    func.func private @ac_init_top.cpu.rf.registers.10() -> i32
    func.func private @ac_init_top.cpu.rf.registers.11() -> i32
    func.func private @ac_init_top.cpu.rf.registers.12() -> i32
    func.func private @ac_init_top.cpu.rf.registers.13() -> i32
    func.func private @ac_init_top.cpu.rf.registers.14() -> i32
    func.func private @ac_init_top.cpu.rf.registers.15() -> i32
    func.func private @ac_init_top.cpu.rf.registers.16() -> i32
    func.func private @ac_init_top.cpu.rf.registers.17() -> i32
    func.func private @ac_init_top.cpu.rf.registers.18() -> i32
    func.func private @ac_init_top.cpu.rf.registers.19() -> i32
    func.func private @ac_init_top.cpu.rf.registers.20() -> i32
    func.func private @ac_init_top.cpu.rf.registers.21() -> i32
    func.func private @ac_init_top.cpu.rf.registers.22() -> i32
    func.func private @ac_init_top.cpu.rf.registers.23() -> i32
    func.func private @ac_init_top.cpu.rf.registers.24() -> i32
    func.func private @ac_init_top.cpu.rf.registers.25() -> i32
    func.func private @ac_init_top.cpu.rf.registers.26() -> i32
    func.func private @ac_init_top.cpu.rf.registers.27() -> i32
    func.func private @ac_init_top.cpu.rf.registers.28() -> i32
    func.func private @ac_init_top.cpu.rf.registers.29() -> i32
    func.func private @ac_init_top.cpu.rf.registers.30() -> i32
    func.func private @ac_init_top.cpu.rf.registers.31() -> i32
    func.func private @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched">
    func.func private @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded">
    func.func private @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed">
    func.func private @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed">
    func.func private @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]>
    func.func private @ac_init_top.cpu.fetch.pc() -> i32
    func.func private @ac_init_top.cpu.memory.words.0() -> i32
    func.func private @ac_init_top.cpu.memory.words.1() -> i32
    func.func private @ac_init_top.cpu.memory.words.2() -> i32
    func.func private @ac_init_top.cpu.memory.words.3() -> i32
    func.func private @ac_init_top.cpu.memory.words.4() -> i32
    func.func private @ac_init_top.cpu.memory.words.5() -> i32
    func.func private @ac_init_top.cpu.memory.words.6() -> i32
    func.func private @ac_init_top.cpu.memory.words.7() -> i32
    func.func private @ac_init_top.cpu.memory.words.8() -> i32
    func.func private @ac_init_top.cpu.memory.words.9() -> i32
    func.func private @ac_init_top.cpu.memory.words.10() -> i32
    func.func private @ac_init_top.cpu.memory.words.11() -> i32
    func.func private @ac_init_top.cpu.memory.words.12() -> i32
    func.func private @ac_init_top.cpu.memory.words.13() -> i32
    func.func private @ac_init_top.cpu.memory.words.14() -> i32
    func.func private @ac_init_top.cpu.memory.words.15() -> i32
    func.func private @ac_init_top.cpu.memory.words.16() -> i32
    func.func private @ac_init_top.cpu.memory.words.17() -> i32
    func.func private @ac_init_top.cpu.memory.words.18() -> i32
    func.func private @ac_init_top.cpu.memory.words.19() -> i32
    func.func private @ac_init_top.cpu.memory.words.20() -> i32
    func.func private @ac_init_top.cpu.memory.words.21() -> i32
    func.func private @ac_init_top.cpu.memory.words.22() -> i32
    func.func private @ac_init_top.cpu.memory.words.23() -> i32
    func.func private @ac_init_top.cpu.memory.words.24() -> i32
    func.func private @ac_init_top.cpu.memory.words.25() -> i32
    func.func private @ac_init_top.cpu.memory.words.26() -> i32
    func.func private @ac_init_top.cpu.memory.words.27() -> i32
    func.func private @ac_init_top.cpu.memory.words.28() -> i32
    func.func private @ac_init_top.cpu.memory.words.29() -> i32
    func.func private @ac_init_top.cpu.memory.words.30() -> i32
    func.func private @ac_init_top.cpu.memory.words.31() -> i32
    func.func private @ac_init_top.cpu.memory.words.32() -> i32
    func.func private @ac_init_top.cpu.memory.words.33() -> i32
    func.func private @ac_init_top.cpu.memory.words.34() -> i32
    func.func private @ac_init_top.cpu.memory.words.35() -> i32
    func.func private @ac_init_top.cpu.memory.words.36() -> i32
    func.func private @ac_init_top.cpu.memory.words.37() -> i32
    func.func private @ac_init_top.cpu.memory.words.38() -> i32
    func.func private @ac_init_top.cpu.memory.words.39() -> i32
    func.func private @ac_init_top.cpu.memory.words.40() -> i32
    func.func private @ac_init_top.cpu.memory.words.41() -> i32
    func.func private @ac_init_top.cpu.memory.words.42() -> i32
    func.func private @ac_init_top.cpu.memory.words.43() -> i32
    func.func private @ac_init_top.cpu.memory.words.44() -> i32
    func.func private @ac_init_top.cpu.memory.words.45() -> i32
    func.func private @ac_init_top.cpu.memory.words.46() -> i32
    func.func private @ac_init_top.cpu.memory.words.47() -> i32
    func.func private @ac_init_top.cpu.memory.words.48() -> i32
    func.func private @ac_init_top.cpu.memory.words.49() -> i32
    func.func private @ac_init_top.cpu.memory.words.50() -> i32
    func.func private @ac_init_top.cpu.memory.words.51() -> i32
    func.func private @ac_init_top.cpu.memory.words.52() -> i32
    func.func private @ac_init_top.cpu.memory.words.53() -> i32
    func.func private @ac_init_top.cpu.memory.words.54() -> i32
    func.func private @ac_init_top.cpu.memory.words.55() -> i32
    func.func private @ac_init_top.cpu.memory.words.56() -> i32
    func.func private @ac_init_top.cpu.memory.words.57() -> i32
    func.func private @ac_init_top.cpu.memory.words.58() -> i32
    func.func private @ac_init_top.cpu.memory.words.59() -> i32
    func.func private @ac_init_top.cpu.memory.words.60() -> i32
    func.func private @ac_init_top.cpu.memory.words.61() -> i32
    func.func private @ac_init_top.cpu.memory.words.62() -> i32
    func.func private @ac_init_top.cpu.memory.words.63() -> i32
    func.func private @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent">
    func.func private @ac_init_top.cpu.writeback.retired() -> i32
    func.func private @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement">
    func.func private @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control">
    func.func private @construct.0.call.0.0() -> !acir.array<i32, [256]>
    func.func private @construct.0.call.1.1() -> !acir.array<i32, [32]>
    func.func private @construct.0.call.2.2() -> !acir.array<i32, [64]>
    func.func private @top.cpu_body.0.call.5.9.0.call.5.16(i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.5.9.0.call.6.23(i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(i32, i32) -> i1
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.11.30(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(i32, i32) -> i32
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(i32) -> !acir.struct<"src_isa_e31afbf3.Instruction">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.cpu.decode_stage_body.0.call.17.6.0.call.17.49 : (i32, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32 {acir.function = {contract = "Module:top.cpu.rf", definition = "read", instance = "top.cpu.rf", interface = "Module:top.cpu.rf:read", kind = "rule", name = "top.cpu.decode_stage_body.0.call.17.6.0.call.17.49", output_type = "u32", owner = "top.cpu.rf", params = [{id = "v1", name = "index", targets = [], type = "u32"}, {id = "v2", name = "writeback", targets = [], type = "src_isa_e31afbf3.Completed"}], public = true, result = "u32", rule = "read"}} body {
    ^bb0(%arg0: i32 loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6.0.call.17.49:index"), %arg1: !acir.struct<"src_isa_e31afbf3.Completed"> loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6.0.call.17.49:writeback")):
      %true = arith.constant true
      %c0_i32 = arith.constant 0 : i32
      %false = arith.constant false
      %0 = acir.aggregate() : () -> i32
      %1 = arith.cmpi eq, %arg0, %c0_i32 : i32 loc(#loc268)
      %2 = arith.cmpi ne, %arg0, %c0_i32 : i32 loc(#loc268)
      %3 = arith.select %1, %c0_i32, %0 : i32
      %4 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.0 : !acir.cell<i32>
      %5 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.1 : !acir.cell<i32>
      %6 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.2 : !acir.cell<i32>
      %7 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.3 : !acir.cell<i32>
      %8 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.4 : !acir.cell<i32>
      %9 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.5 : !acir.cell<i32>
      %10 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.6 : !acir.cell<i32>
      %11 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.7 : !acir.cell<i32>
      %12 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.8 : !acir.cell<i32>
      %13 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.9 : !acir.cell<i32>
      %14 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.10 : !acir.cell<i32>
      %15 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.11 : !acir.cell<i32>
      %16 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.12 : !acir.cell<i32>
      %17 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.13 : !acir.cell<i32>
      %18 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.14 : !acir.cell<i32>
      %19 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.15 : !acir.cell<i32>
      %20 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.16 : !acir.cell<i32>
      %21 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.17 : !acir.cell<i32>
      %22 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.18 : !acir.cell<i32>
      %23 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.19 : !acir.cell<i32>
      %24 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.20 : !acir.cell<i32>
      %25 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.21 : !acir.cell<i32>
      %26 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.22 : !acir.cell<i32>
      %27 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.23 : !acir.cell<i32>
      %28 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.24 : !acir.cell<i32>
      %29 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.25 : !acir.cell<i32>
      %30 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.26 : !acir.cell<i32>
      %31 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.27 : !acir.cell<i32>
      %32 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.28 : !acir.cell<i32>
      %33 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.29 : !acir.cell<i32>
      %34 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.30 : !acir.cell<i32>
      %35 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.31 : !acir.cell<i32>
      %36 = acir.aggregate(%4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16, %17, %18, %19, %20, %21, %22, %23, %24, %25, %26, %27, %28, %29, %30, %31, %32, %33, %34, %35) {array_name = "top.cpu.rf.registers"} : (!acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>) -> !acir.array<!acir.cell<i32>, [32]> loc(#loc269)
      %37:2 = scf.if %2 -> (i1, i32) {
        %39 = acir.extract %arg1 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i1 loc(#loc270)
        %40 = scf.if %39 -> (i1) {
          %44 = acir.extract %arg1 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc271)
          %45 = arith.cmpi eq, %44, %arg0 : i32 loc(#loc271)
          scf.yield %45 : i1
        } else {
          scf.yield %false : i1
        }
        %41 = arith.xori %40, %true : i1
        %42 = arith.select %40, %41, %false : i1
        %43 = scf.if %40 -> (i32) {
          %44 = acir.extract %arg1 {field = "value"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc272)
          scf.yield %44 : i32
        } else {
          %44 = "acir.array.index"(%36, %arg0) {signed_index = false} : (!acir.array<!acir.cell<i32>, [32]>, i32) -> !acir.cell<i32> loc(#loc273)
          %45 = acir.cell.read %44 {acir.id = "v69", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc273)
          scf.yield %45 : i32
        }
        scf.yield %42, %43 : i1, i32
      } else {
        scf.yield %false, %3 : i1, i32
      }
      %38 = arith.xori %37#0, %true : i1
      acir.check %38 {acir.source_type = "void"}
      acir.return %37#1 : i32 {acir.source_type = "void"}
    }
    acir.rule @top.cpu.writeback_body.0.call.21.3.0.call.21.18 : (i32, i32) -> () {acir.function = {contract = "Module:top.cpu.rf", definition = "write", instance = "top.cpu.rf", interface = "Module:top.cpu.rf:write", kind = "rule", name = "top.cpu.writeback_body.0.call.21.3.0.call.21.18", output_type = "void", owner = "top.cpu.rf", params = [{id = "v1", name = "index", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "write"}} body {
    ^bb0(%arg0: i32 loc("acir.binding:top.cpu.writeback_body.0.call.21.3.0.call.21.18:index"), %arg1: i32 loc("acir.binding:top.cpu.writeback_body.0.call.21.3.0.call.21.18:value")):
      %c0_i32 = arith.constant 0 : i32 loc(#loc276)
      %0 = arith.cmpi ne, %arg0, %c0_i32 : i32 loc(#loc277)
      %1 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.0 : !acir.cell<i32>
      %2 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.1 : !acir.cell<i32>
      %3 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.2 : !acir.cell<i32>
      %4 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.3 : !acir.cell<i32>
      %5 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.4 : !acir.cell<i32>
      %6 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.5 : !acir.cell<i32>
      %7 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.6 : !acir.cell<i32>
      %8 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.7 : !acir.cell<i32>
      %9 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.8 : !acir.cell<i32>
      %10 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.9 : !acir.cell<i32>
      %11 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.10 : !acir.cell<i32>
      %12 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.11 : !acir.cell<i32>
      %13 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.12 : !acir.cell<i32>
      %14 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.13 : !acir.cell<i32>
      %15 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.14 : !acir.cell<i32>
      %16 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.15 : !acir.cell<i32>
      %17 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.16 : !acir.cell<i32>
      %18 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.17 : !acir.cell<i32>
      %19 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.18 : !acir.cell<i32>
      %20 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.19 : !acir.cell<i32>
      %21 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.20 : !acir.cell<i32>
      %22 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.21 : !acir.cell<i32>
      %23 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.22 : !acir.cell<i32>
      %24 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.23 : !acir.cell<i32>
      %25 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.24 : !acir.cell<i32>
      %26 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.25 : !acir.cell<i32>
      %27 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.26 : !acir.cell<i32>
      %28 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.27 : !acir.cell<i32>
      %29 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.28 : !acir.cell<i32>
      %30 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.29 : !acir.cell<i32>
      %31 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.30 : !acir.cell<i32>
      %32 = acir.ref @top.cpu.rf::@top.cpu.rf.registers.31 : !acir.cell<i32>
      %33 = acir.aggregate(%1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16, %17, %18, %19, %20, %21, %22, %23, %24, %25, %26, %27, %28, %29, %30, %31, %32) {array_name = "top.cpu.rf.registers"} : (!acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>) -> !acir.array<!acir.cell<i32>, [32]> loc(#loc269)
      scf.if %0 {
        %34 = "acir.array.index"(%33, %arg0) {signed_index = false} : (!acir.array<!acir.cell<i32>, [32]>, i32) -> !acir.cell<i32> loc(#loc278)
        acir.cell.write %34, %arg1 {acir.id = "v42", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc278)
      }
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.cpu.fetch attributes {acir.instance = {definition = "src_model_bb8d42ed.Fetch", name = "top.cpu.fetch", parent = "top.cpu", rules = ["advance"]}} {
    acir.contract {relations = [], rules = ["advance"]}
    acir.cell @top.cpu.fetch.rom : !acir.array<i32, [256]> {initial_fn = "ac_init_top.cpu.fetch.rom", name = "top.cpu.fetch.rom", type = "array<u32;256>"}
    acir.cell @top.cpu.fetch.pc : i32 {initial_fn = "ac_init_top.cpu.fetch.pc", name = "top.cpu.fetch.pc", type = "u32"}
    func.func private @ac_init_top.cpu.rf.registers.0() -> i32
    func.func private @ac_init_top.cpu.rf.registers.1() -> i32
    func.func private @ac_init_top.cpu.rf.registers.2() -> i32
    func.func private @ac_init_top.cpu.rf.registers.3() -> i32
    func.func private @ac_init_top.cpu.rf.registers.4() -> i32
    func.func private @ac_init_top.cpu.rf.registers.5() -> i32
    func.func private @ac_init_top.cpu.rf.registers.6() -> i32
    func.func private @ac_init_top.cpu.rf.registers.7() -> i32
    func.func private @ac_init_top.cpu.rf.registers.8() -> i32
    func.func private @ac_init_top.cpu.rf.registers.9() -> i32
    func.func private @ac_init_top.cpu.rf.registers.10() -> i32
    func.func private @ac_init_top.cpu.rf.registers.11() -> i32
    func.func private @ac_init_top.cpu.rf.registers.12() -> i32
    func.func private @ac_init_top.cpu.rf.registers.13() -> i32
    func.func private @ac_init_top.cpu.rf.registers.14() -> i32
    func.func private @ac_init_top.cpu.rf.registers.15() -> i32
    func.func private @ac_init_top.cpu.rf.registers.16() -> i32
    func.func private @ac_init_top.cpu.rf.registers.17() -> i32
    func.func private @ac_init_top.cpu.rf.registers.18() -> i32
    func.func private @ac_init_top.cpu.rf.registers.19() -> i32
    func.func private @ac_init_top.cpu.rf.registers.20() -> i32
    func.func private @ac_init_top.cpu.rf.registers.21() -> i32
    func.func private @ac_init_top.cpu.rf.registers.22() -> i32
    func.func private @ac_init_top.cpu.rf.registers.23() -> i32
    func.func private @ac_init_top.cpu.rf.registers.24() -> i32
    func.func private @ac_init_top.cpu.rf.registers.25() -> i32
    func.func private @ac_init_top.cpu.rf.registers.26() -> i32
    func.func private @ac_init_top.cpu.rf.registers.27() -> i32
    func.func private @ac_init_top.cpu.rf.registers.28() -> i32
    func.func private @ac_init_top.cpu.rf.registers.29() -> i32
    func.func private @ac_init_top.cpu.rf.registers.30() -> i32
    func.func private @ac_init_top.cpu.rf.registers.31() -> i32
    func.func private @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched">
    func.func private @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded">
    func.func private @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed">
    func.func private @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed">
    func.func private @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]>
    func.func private @ac_init_top.cpu.fetch.pc() -> i32
    func.func private @ac_init_top.cpu.memory.words.0() -> i32
    func.func private @ac_init_top.cpu.memory.words.1() -> i32
    func.func private @ac_init_top.cpu.memory.words.2() -> i32
    func.func private @ac_init_top.cpu.memory.words.3() -> i32
    func.func private @ac_init_top.cpu.memory.words.4() -> i32
    func.func private @ac_init_top.cpu.memory.words.5() -> i32
    func.func private @ac_init_top.cpu.memory.words.6() -> i32
    func.func private @ac_init_top.cpu.memory.words.7() -> i32
    func.func private @ac_init_top.cpu.memory.words.8() -> i32
    func.func private @ac_init_top.cpu.memory.words.9() -> i32
    func.func private @ac_init_top.cpu.memory.words.10() -> i32
    func.func private @ac_init_top.cpu.memory.words.11() -> i32
    func.func private @ac_init_top.cpu.memory.words.12() -> i32
    func.func private @ac_init_top.cpu.memory.words.13() -> i32
    func.func private @ac_init_top.cpu.memory.words.14() -> i32
    func.func private @ac_init_top.cpu.memory.words.15() -> i32
    func.func private @ac_init_top.cpu.memory.words.16() -> i32
    func.func private @ac_init_top.cpu.memory.words.17() -> i32
    func.func private @ac_init_top.cpu.memory.words.18() -> i32
    func.func private @ac_init_top.cpu.memory.words.19() -> i32
    func.func private @ac_init_top.cpu.memory.words.20() -> i32
    func.func private @ac_init_top.cpu.memory.words.21() -> i32
    func.func private @ac_init_top.cpu.memory.words.22() -> i32
    func.func private @ac_init_top.cpu.memory.words.23() -> i32
    func.func private @ac_init_top.cpu.memory.words.24() -> i32
    func.func private @ac_init_top.cpu.memory.words.25() -> i32
    func.func private @ac_init_top.cpu.memory.words.26() -> i32
    func.func private @ac_init_top.cpu.memory.words.27() -> i32
    func.func private @ac_init_top.cpu.memory.words.28() -> i32
    func.func private @ac_init_top.cpu.memory.words.29() -> i32
    func.func private @ac_init_top.cpu.memory.words.30() -> i32
    func.func private @ac_init_top.cpu.memory.words.31() -> i32
    func.func private @ac_init_top.cpu.memory.words.32() -> i32
    func.func private @ac_init_top.cpu.memory.words.33() -> i32
    func.func private @ac_init_top.cpu.memory.words.34() -> i32
    func.func private @ac_init_top.cpu.memory.words.35() -> i32
    func.func private @ac_init_top.cpu.memory.words.36() -> i32
    func.func private @ac_init_top.cpu.memory.words.37() -> i32
    func.func private @ac_init_top.cpu.memory.words.38() -> i32
    func.func private @ac_init_top.cpu.memory.words.39() -> i32
    func.func private @ac_init_top.cpu.memory.words.40() -> i32
    func.func private @ac_init_top.cpu.memory.words.41() -> i32
    func.func private @ac_init_top.cpu.memory.words.42() -> i32
    func.func private @ac_init_top.cpu.memory.words.43() -> i32
    func.func private @ac_init_top.cpu.memory.words.44() -> i32
    func.func private @ac_init_top.cpu.memory.words.45() -> i32
    func.func private @ac_init_top.cpu.memory.words.46() -> i32
    func.func private @ac_init_top.cpu.memory.words.47() -> i32
    func.func private @ac_init_top.cpu.memory.words.48() -> i32
    func.func private @ac_init_top.cpu.memory.words.49() -> i32
    func.func private @ac_init_top.cpu.memory.words.50() -> i32
    func.func private @ac_init_top.cpu.memory.words.51() -> i32
    func.func private @ac_init_top.cpu.memory.words.52() -> i32
    func.func private @ac_init_top.cpu.memory.words.53() -> i32
    func.func private @ac_init_top.cpu.memory.words.54() -> i32
    func.func private @ac_init_top.cpu.memory.words.55() -> i32
    func.func private @ac_init_top.cpu.memory.words.56() -> i32
    func.func private @ac_init_top.cpu.memory.words.57() -> i32
    func.func private @ac_init_top.cpu.memory.words.58() -> i32
    func.func private @ac_init_top.cpu.memory.words.59() -> i32
    func.func private @ac_init_top.cpu.memory.words.60() -> i32
    func.func private @ac_init_top.cpu.memory.words.61() -> i32
    func.func private @ac_init_top.cpu.memory.words.62() -> i32
    func.func private @ac_init_top.cpu.memory.words.63() -> i32
    func.func private @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent">
    func.func private @ac_init_top.cpu.writeback.retired() -> i32
    func.func private @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement">
    func.func private @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control">
    func.func private @construct.0.call.0.0() -> !acir.array<i32, [256]>
    func.func private @construct.0.call.1.1() -> !acir.array<i32, [32]>
    func.func private @construct.0.call.2.2() -> !acir.array<i32, [64]>
    func.func private @top.cpu_body.0.call.5.9.0.call.5.16(i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.5.9.0.call.6.23(i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(i32, i32) -> i1
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.11.30(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(i32, i32) -> i32
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(i32) -> !acir.struct<"src_isa_e31afbf3.Instruction">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.cpu.fetch_body.0.call.13.2 : (!acir.struct<"src_isa_e31afbf3.Control">) -> !acir.struct<"src_isa_e31afbf3.Fetched"> {acir.function = {contract = "Module:top.cpu.fetch", definition = "advance", instance = "top.cpu.fetch", interface = "Module:top.cpu.fetch:advance", kind = "rule", name = "top.cpu.fetch_body.0.call.13.2", output_type = "cell<src_isa_e31afbf3.Fetched>", owner = "top.cpu.fetch", params = [{id = "v1", name = "control", targets = [], type = "src_isa_e31afbf3.Control"}], public = true, result = "src_isa_e31afbf3.Fetched", rule = "advance"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_e31afbf3.Control"> loc("acir.binding:top.cpu.fetch_body.0.call.13.2:control")):
      %c4_i32 = arith.constant 4 : i32
      %c2_i32 = arith.constant 2 : i32
      %c3_i32 = arith.constant 3 : i32
      %c0_i32 = arith.constant 0 : i32
      %false = arith.constant false
      %true = arith.constant true
      %0 = acir.aggregate() : () -> !acir.struct<"src_isa_e31afbf3.Fetched">
      %1 = acir.extract %arg0 {field = "stall"} : (!acir.struct<"src_isa_e31afbf3.Control">) -> i1 loc(#loc280)
      %2 = arith.xori %1, %true : i1
      %3 = acir.ref @top.cpu.fetch::@top.cpu.fetch.pc : !acir.cell<i32>
      %4 = acir.ref @top.cpu.fetch::@top.cpu.fetch.rom : !acir.cell<!acir.array<i32, [256]>>
      %5:2 = scf.if %1 -> (i1, !acir.struct<"src_isa_e31afbf3.Fetched">) {
        scf.yield %false, %0 : i1, !acir.struct<"src_isa_e31afbf3.Fetched">
      } else {
        %7 = acir.extract %arg0 {field = "redirect"} : (!acir.struct<"src_isa_e31afbf3.Control">) -> i1 loc(#loc281)
        %8 = arith.select %7, %false, %2 : i1
        %9 = scf.if %7 -> (!acir.struct<"src_isa_e31afbf3.Fetched">) {
          %12 = acir.extract %arg0 {field = "target"} : (!acir.struct<"src_isa_e31afbf3.Control">) -> i32 loc(#loc282)
          acir.cell.write %3, %12 {acir.id = "v16", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc283)
          %13 = acir.aggregate(%false, %c0_i32, %c0_i32) : (i1, i32, i32) -> !acir.struct<"src_isa_e31afbf3.Fetched"> loc(#loc284)
          scf.yield %13 : !acir.struct<"src_isa_e31afbf3.Fetched">
        } else {
          scf.yield %0 : !acir.struct<"src_isa_e31afbf3.Fetched">
        }
        %10 = arith.select %8, %true, %7 : i1
        %11 = scf.if %8 -> (!acir.struct<"src_isa_e31afbf3.Fetched">) {
          %12 = acir.cell.read %3 {acir.id = "v30", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc285)
          %13 = arith.andi %12, %c3_i32 : i32 loc(#loc285)
          %14 = arith.cmpi eq, %13, %c0_i32 : i32 loc(#loc285)
          acir.check %14 {acir.id = "v35", acir.source_type = "void"} loc(#loc286)
          %15 = acir.cell.read %3 {acir.id = "v38", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc287)
          %16 = acir.cell.read %4 {acir.id = "v40", acir.source_type = "array<u32;256>", role = "observe"} : !acir.cell<!acir.array<i32, [256]>> -> !acir.array<i32, [256]> loc(#loc288)
          %17 = acir.cell.read %3 {acir.id = "v42", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc289)
          %18 = func.call @ac_builtin_shr_u32(%17, %c2_i32) {acir.id = "v44", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc289)
          %19 = "acir.array.index"(%16, %18) {signed_index = false} : (!acir.array<i32, [256]>, i32) -> i32 loc(#loc288)
          %20 = acir.aggregate(%true, %15, %19) : (i1, i32, i32) -> !acir.struct<"src_isa_e31afbf3.Fetched"> loc(#loc290)
          %21 = acir.cell.read %3 {acir.id = "v49", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc291)
          %22 = arith.addi %21, %c4_i32 : i32 loc(#loc291)
          acir.cell.write %3, %22 {acir.id = "v53", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc292)
          scf.yield %20 : !acir.struct<"src_isa_e31afbf3.Fetched">
        } else {
          scf.yield %9 : !acir.struct<"src_isa_e31afbf3.Fetched">
        }
        scf.yield %10, %11 : i1, !acir.struct<"src_isa_e31afbf3.Fetched">
      }
      %6 = "acir.optional"(%5#1, %5#0) : (!acir.struct<"src_isa_e31afbf3.Fetched">, i1) -> !acir.struct<"src_isa_e31afbf3.Fetched">
      acir.return %6 : !acir.struct<"src_isa_e31afbf3.Fetched"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.cpu.fetch_body", owner = "top.cpu.fetch", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.external "top.cpu_body" "v21" {acir.source_type = "src_isa_e31afbf3.Control"} : !acir.struct<"src_isa_e31afbf3.Control">
      %1 = acir.rule.call @top.cpu.fetch::@top.cpu.fetch_body.0.call.13.2(%0) {acir.id = "v3", acir.source_type = "src_isa_e31afbf3.Fetched", contract = "Module:top.cpu.fetch", instance = "top.cpu.fetch", interface = "Module:top.cpu.fetch:advance", output_binding = @top.cpu::@top.cpu.if_id, role = "rule", rule = "advance"} : (!acir.struct<"src_isa_e31afbf3.Control">) -> !acir.struct<"src_isa_e31afbf3.Fetched"> loc(#loc293)
      %2 = acir.ref @top.cpu::@top.cpu.if_id : !acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">>
      %false = arith.constant false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.cpu.decode_stage attributes {acir.instance = {definition = "src_model_bb8d42ed.Decode", name = "top.cpu.decode_stage", parent = "top.cpu", rules = ["advance", "decode_instruction", "inspect"]}} {
    acir.contract {relations = [], rules = ["advance", "decode_instruction", "inspect"]}
    acir.signal @top.cpu.decode_stage.instruction : !acir.struct<"src_isa_e31afbf3.Instruction"> {name = "top.cpu.decode_stage.instruction", type = "src_isa_e31afbf3.Instruction"}
    func.func private @ac_init_top.cpu.rf.registers.0() -> i32
    func.func private @ac_init_top.cpu.rf.registers.1() -> i32
    func.func private @ac_init_top.cpu.rf.registers.2() -> i32
    func.func private @ac_init_top.cpu.rf.registers.3() -> i32
    func.func private @ac_init_top.cpu.rf.registers.4() -> i32
    func.func private @ac_init_top.cpu.rf.registers.5() -> i32
    func.func private @ac_init_top.cpu.rf.registers.6() -> i32
    func.func private @ac_init_top.cpu.rf.registers.7() -> i32
    func.func private @ac_init_top.cpu.rf.registers.8() -> i32
    func.func private @ac_init_top.cpu.rf.registers.9() -> i32
    func.func private @ac_init_top.cpu.rf.registers.10() -> i32
    func.func private @ac_init_top.cpu.rf.registers.11() -> i32
    func.func private @ac_init_top.cpu.rf.registers.12() -> i32
    func.func private @ac_init_top.cpu.rf.registers.13() -> i32
    func.func private @ac_init_top.cpu.rf.registers.14() -> i32
    func.func private @ac_init_top.cpu.rf.registers.15() -> i32
    func.func private @ac_init_top.cpu.rf.registers.16() -> i32
    func.func private @ac_init_top.cpu.rf.registers.17() -> i32
    func.func private @ac_init_top.cpu.rf.registers.18() -> i32
    func.func private @ac_init_top.cpu.rf.registers.19() -> i32
    func.func private @ac_init_top.cpu.rf.registers.20() -> i32
    func.func private @ac_init_top.cpu.rf.registers.21() -> i32
    func.func private @ac_init_top.cpu.rf.registers.22() -> i32
    func.func private @ac_init_top.cpu.rf.registers.23() -> i32
    func.func private @ac_init_top.cpu.rf.registers.24() -> i32
    func.func private @ac_init_top.cpu.rf.registers.25() -> i32
    func.func private @ac_init_top.cpu.rf.registers.26() -> i32
    func.func private @ac_init_top.cpu.rf.registers.27() -> i32
    func.func private @ac_init_top.cpu.rf.registers.28() -> i32
    func.func private @ac_init_top.cpu.rf.registers.29() -> i32
    func.func private @ac_init_top.cpu.rf.registers.30() -> i32
    func.func private @ac_init_top.cpu.rf.registers.31() -> i32
    func.func private @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched">
    func.func private @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded">
    func.func private @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed">
    func.func private @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed">
    func.func private @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]>
    func.func private @ac_init_top.cpu.fetch.pc() -> i32
    func.func private @ac_init_top.cpu.memory.words.0() -> i32
    func.func private @ac_init_top.cpu.memory.words.1() -> i32
    func.func private @ac_init_top.cpu.memory.words.2() -> i32
    func.func private @ac_init_top.cpu.memory.words.3() -> i32
    func.func private @ac_init_top.cpu.memory.words.4() -> i32
    func.func private @ac_init_top.cpu.memory.words.5() -> i32
    func.func private @ac_init_top.cpu.memory.words.6() -> i32
    func.func private @ac_init_top.cpu.memory.words.7() -> i32
    func.func private @ac_init_top.cpu.memory.words.8() -> i32
    func.func private @ac_init_top.cpu.memory.words.9() -> i32
    func.func private @ac_init_top.cpu.memory.words.10() -> i32
    func.func private @ac_init_top.cpu.memory.words.11() -> i32
    func.func private @ac_init_top.cpu.memory.words.12() -> i32
    func.func private @ac_init_top.cpu.memory.words.13() -> i32
    func.func private @ac_init_top.cpu.memory.words.14() -> i32
    func.func private @ac_init_top.cpu.memory.words.15() -> i32
    func.func private @ac_init_top.cpu.memory.words.16() -> i32
    func.func private @ac_init_top.cpu.memory.words.17() -> i32
    func.func private @ac_init_top.cpu.memory.words.18() -> i32
    func.func private @ac_init_top.cpu.memory.words.19() -> i32
    func.func private @ac_init_top.cpu.memory.words.20() -> i32
    func.func private @ac_init_top.cpu.memory.words.21() -> i32
    func.func private @ac_init_top.cpu.memory.words.22() -> i32
    func.func private @ac_init_top.cpu.memory.words.23() -> i32
    func.func private @ac_init_top.cpu.memory.words.24() -> i32
    func.func private @ac_init_top.cpu.memory.words.25() -> i32
    func.func private @ac_init_top.cpu.memory.words.26() -> i32
    func.func private @ac_init_top.cpu.memory.words.27() -> i32
    func.func private @ac_init_top.cpu.memory.words.28() -> i32
    func.func private @ac_init_top.cpu.memory.words.29() -> i32
    func.func private @ac_init_top.cpu.memory.words.30() -> i32
    func.func private @ac_init_top.cpu.memory.words.31() -> i32
    func.func private @ac_init_top.cpu.memory.words.32() -> i32
    func.func private @ac_init_top.cpu.memory.words.33() -> i32
    func.func private @ac_init_top.cpu.memory.words.34() -> i32
    func.func private @ac_init_top.cpu.memory.words.35() -> i32
    func.func private @ac_init_top.cpu.memory.words.36() -> i32
    func.func private @ac_init_top.cpu.memory.words.37() -> i32
    func.func private @ac_init_top.cpu.memory.words.38() -> i32
    func.func private @ac_init_top.cpu.memory.words.39() -> i32
    func.func private @ac_init_top.cpu.memory.words.40() -> i32
    func.func private @ac_init_top.cpu.memory.words.41() -> i32
    func.func private @ac_init_top.cpu.memory.words.42() -> i32
    func.func private @ac_init_top.cpu.memory.words.43() -> i32
    func.func private @ac_init_top.cpu.memory.words.44() -> i32
    func.func private @ac_init_top.cpu.memory.words.45() -> i32
    func.func private @ac_init_top.cpu.memory.words.46() -> i32
    func.func private @ac_init_top.cpu.memory.words.47() -> i32
    func.func private @ac_init_top.cpu.memory.words.48() -> i32
    func.func private @ac_init_top.cpu.memory.words.49() -> i32
    func.func private @ac_init_top.cpu.memory.words.50() -> i32
    func.func private @ac_init_top.cpu.memory.words.51() -> i32
    func.func private @ac_init_top.cpu.memory.words.52() -> i32
    func.func private @ac_init_top.cpu.memory.words.53() -> i32
    func.func private @ac_init_top.cpu.memory.words.54() -> i32
    func.func private @ac_init_top.cpu.memory.words.55() -> i32
    func.func private @ac_init_top.cpu.memory.words.56() -> i32
    func.func private @ac_init_top.cpu.memory.words.57() -> i32
    func.func private @ac_init_top.cpu.memory.words.58() -> i32
    func.func private @ac_init_top.cpu.memory.words.59() -> i32
    func.func private @ac_init_top.cpu.memory.words.60() -> i32
    func.func private @ac_init_top.cpu.memory.words.61() -> i32
    func.func private @ac_init_top.cpu.memory.words.62() -> i32
    func.func private @ac_init_top.cpu.memory.words.63() -> i32
    func.func private @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent">
    func.func private @ac_init_top.cpu.writeback.retired() -> i32
    func.func private @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement">
    func.func private @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control">
    func.func private @construct.0.call.0.0() -> !acir.array<i32, [256]>
    func.func private @construct.0.call.1.1() -> !acir.array<i32, [32]>
    func.func private @construct.0.call.2.2() -> !acir.array<i32, [64]>
    func.func private @top.cpu_body.0.call.5.9.0.call.5.16(i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.5.9.0.call.6.23(i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(i32, i32) -> i1
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.11.30(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(i32, i32) -> i32
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(i32) -> !acir.struct<"src_isa_e31afbf3.Instruction">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.cpu_body.0.call.3.1 : () -> !acir.struct<"src_isa_e31afbf3.Instruction"> {acir.function = {contract = "Module:top.cpu.decode_stage", definition = "inspect", instance = "top.cpu.decode_stage", interface = "Module:top.cpu.decode_stage:inspect", kind = "rule", name = "top.cpu_body.0.call.3.1", output_type = "src_isa_e31afbf3.Instruction", owner = "top.cpu.decode_stage", params = [], public = true, result = "src_isa_e31afbf3.Instruction", rule = "inspect"}} body {
      %0 = acir.ref @top.cpu.decode_stage::@top.cpu.decode_stage.instruction : !acir.signal<!acir.struct<"src_isa_e31afbf3.Instruction">>
      %1 = acir.signal.get %0 {acir.id = "v7", acir.source_type = "src_isa_e31afbf3.Instruction", role = "observe"} : !acir.signal<!acir.struct<"src_isa_e31afbf3.Instruction">> -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc294)
      acir.return %1 : !acir.struct<"src_isa_e31afbf3.Instruction"> {acir.source_type = "void"}
    }
    acir.rule @top.cpu.decode_stage_body.0.call.14.2 : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">>) -> () {acir.function = {contract = "Module:top.cpu.decode_stage", definition = "decode_instruction", instance = "top.cpu.decode_stage", interface = "Module:top.cpu.decode_stage:decode_instruction", kind = "rule", name = "top.cpu.decode_stage_body.0.call.14.2", output_type = "void", owner = "top.cpu.decode_stage", params = [{id = "v1", name = "source", targets = ["top.cpu.if_id"], type = "cell<src_isa_e31afbf3.Fetched>"}], public = true, result = "void", rule = "decode_instruction"}} body {
    ^bb0(%arg0: !acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">> loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2:source")):
      %0 = acir.cell.read %arg0 {acir.id = "v5", acir.source_type = "src_isa_e31afbf3.Fetched", role = "observe"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">> -> !acir.struct<"src_isa_e31afbf3.Fetched"> loc(#loc296)
      %1 = acir.extract %0 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Fetched">) -> i32 loc(#loc296)
      %2 = func.call @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(%1) {acir.id = "v7", acir.source_type = "src_isa_e31afbf3.Instruction", role = "helper"} : (i32) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc297)
      %3 = acir.ref @top.cpu.decode_stage::@top.cpu.decode_stage.instruction : !acir.signal<!acir.struct<"src_isa_e31afbf3.Instruction">>
      acir.signal.set %3, %2 {acir.id = "v9", acir.source_type = "void"} : !acir.signal<!acir.struct<"src_isa_e31afbf3.Instruction">>, !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc298)
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.cpu.decode_stage_body.0.call.17.6 : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">>, !acir.struct<"src_isa_e31afbf3.Control">, !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>) -> !acir.struct<"src_isa_e31afbf3.Decoded"> {acir.function = {contract = "Module:top.cpu.decode_stage", definition = "advance", instance = "top.cpu.decode_stage", interface = "Module:top.cpu.decode_stage:advance", kind = "rule", name = "top.cpu.decode_stage_body.0.call.17.6", output_type = "cell<src_isa_e31afbf3.Decoded>", owner = "top.cpu.decode_stage", params = [{id = "v1", name = "source", targets = ["top.cpu.if_id"], type = "cell<src_isa_e31afbf3.Fetched>"}, {id = "v2", name = "control", targets = [], type = "src_isa_e31afbf3.Control"}, {id = "v3", name = "writeback", targets = ["top.cpu.mem_wb"], type = "cell<src_isa_e31afbf3.Completed>"}], public = true, result = "src_isa_e31afbf3.Decoded", rule = "advance"}} body {
    ^bb0(%arg0: !acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">> loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:source"), %arg1: !acir.struct<"src_isa_e31afbf3.Control"> loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:control"), %arg2: !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">> loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:writeback")):
      %c0_i32 = arith.constant 0 : i32
      %false = arith.constant false
      %true = arith.constant true
      %0 = acir.extract %arg1 {field = "stall"} : (!acir.struct<"src_isa_e31afbf3.Control">) -> i1 loc(#loc302)
      %1 = scf.if %0 -> (i1) {
        scf.yield %true : i1
      } else {
        %4 = acir.extract %arg1 {field = "redirect"} : (!acir.struct<"src_isa_e31afbf3.Control">) -> i1 loc(#loc303)
        scf.yield %4 : i1
      }
      %2 = scf.if %1 -> (!acir.struct<"src_isa_e31afbf3.Decoded">) {
        %4 = acir.aggregate() : () -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc304)
        %5 = acir.aggregate(%false, %c0_i32, %c0_i32, %4, %c0_i32, %c0_i32, %false) : (i1, i32, i32, !acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc305)
        %6 = acir.extract %5 {field = "valid"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i1 loc(#loc306)
        %7 = acir.extract %5 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc306)
        %8 = acir.extract %5 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc306)
        %9 = acir.extract %5 {field = "instruction"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc306)
        %10 = acir.extract %5 {field = "lhs"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc306)
        %11 = acir.extract %5 {field = "rhs"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc306)
        %12 = acir.aggregate(%6, %7, %8, %9, %10, %11, %0) : (i1, i32, i32, !acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc305)
        scf.yield %12 : !acir.struct<"src_isa_e31afbf3.Decoded">
      } else {
        %4 = acir.cell.read %arg0 {acir.id = "v43", acir.source_type = "src_isa_e31afbf3.Fetched", role = "observe"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">> -> !acir.struct<"src_isa_e31afbf3.Fetched"> loc(#loc307)
        %5 = acir.cell.read %arg2 {acir.id = "v45", acir.source_type = "src_isa_e31afbf3.Completed", role = "observe"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">> -> !acir.struct<"src_isa_e31afbf3.Completed"> loc(#loc308)
        %6 = acir.rule.call @top.cpu.decode_stage::@top.cpu_body.0.call.3.1() {acir.id = "v47", acir.source_type = "src_isa_e31afbf3.Instruction", contract = "Module:top.cpu.decode_stage", instance = "top.cpu.decode_stage", interface = "Module:top.cpu.decode_stage:inspect", role = "rule", rule = "inspect"} : () -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc309)
        %7 = acir.extract %6 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc310)
        %8 = acir.rule.call @top.cpu.rf::@top.cpu.decode_stage_body.0.call.17.6.0.call.17.49(%7, %5) {acir.id = "v50", acir.source_type = "u32", contract = "Module:top.cpu.rf", instance = "top.cpu.rf", interface = "Module:top.cpu.rf:read", role = "rule", rule = "read"} : (i32, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc311)
        %9 = acir.extract %6 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc312)
        %10 = acir.rule.call @top.cpu.rf::@top.cpu.decode_stage_body.0.call.17.6.0.call.17.49(%9, %5) {acir.id = "v53", acir.source_type = "u32", contract = "Module:top.cpu.rf", instance = "top.cpu.rf", interface = "Module:top.cpu.rf:read", role = "rule", rule = "read"} : (i32, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc313)
        %11 = acir.extract %4 {field = "valid"} : (!acir.struct<"src_isa_e31afbf3.Fetched">) -> i1 loc(#loc314)
        %12 = acir.extract %4 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Fetched">) -> i32 loc(#loc315)
        %13 = acir.extract %4 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Fetched">) -> i32 loc(#loc316)
        %14 = acir.aggregate(%11, %12, %13, %6, %8, %10, %false) : (i1, i32, i32, !acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc317)
        scf.yield %14 : !acir.struct<"src_isa_e31afbf3.Decoded">
      }
      %3 = "acir.optional"(%2, %true) : (!acir.struct<"src_isa_e31afbf3.Decoded">, i1) -> !acir.struct<"src_isa_e31afbf3.Decoded">
      acir.return %3 : !acir.struct<"src_isa_e31afbf3.Decoded"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.cpu.decode_stage_body", owner = "top.cpu.decode_stage", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.ref @top.cpu::@top.cpu.if_id : !acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">>
      acir.rule.call @top.cpu.decode_stage::@top.cpu.decode_stage_body.0.call.14.2(%0) {acir.id = "v3", acir.source_type = "void", contract = "Module:top.cpu.decode_stage", instance = "top.cpu.decode_stage", interface = "Module:top.cpu.decode_stage:decode_instruction", role = "rule", rule = "decode_instruction"} : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">>) -> () loc(#loc318)
      %1 = acir.ref @top.cpu::@top.cpu.if_id : !acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">>
      %2 = acir.external "top.cpu_body" "v21" {acir.source_type = "src_isa_e31afbf3.Control"} : !acir.struct<"src_isa_e31afbf3.Control">
      %3 = acir.ref @top.cpu::@top.cpu.mem_wb : !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>
      %4 = acir.rule.call @top.cpu.decode_stage::@top.cpu.decode_stage_body.0.call.17.6(%1, %2, %3) {acir.id = "v7", acir.source_type = "src_isa_e31afbf3.Decoded", contract = "Module:top.cpu.decode_stage", instance = "top.cpu.decode_stage", interface = "Module:top.cpu.decode_stage:advance", output_binding = @top.cpu::@top.cpu.id_ex, role = "rule", rule = "advance"} : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Fetched">>, !acir.struct<"src_isa_e31afbf3.Control">, !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>) -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc319)
      %5 = acir.ref @top.cpu::@top.cpu.id_ex : !acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">>
      %false = arith.constant false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.cpu.hazard_unit attributes {acir.instance = {definition = "src_model_bb8d42ed.HazardUnit", name = "top.cpu.hazard_unit", parent = "top.cpu", rules = ["load_use"]}} {
    acir.contract {relations = [], rules = ["load_use"]}
    func.func private @ac_init_top.cpu.rf.registers.0() -> i32
    func.func private @ac_init_top.cpu.rf.registers.1() -> i32
    func.func private @ac_init_top.cpu.rf.registers.2() -> i32
    func.func private @ac_init_top.cpu.rf.registers.3() -> i32
    func.func private @ac_init_top.cpu.rf.registers.4() -> i32
    func.func private @ac_init_top.cpu.rf.registers.5() -> i32
    func.func private @ac_init_top.cpu.rf.registers.6() -> i32
    func.func private @ac_init_top.cpu.rf.registers.7() -> i32
    func.func private @ac_init_top.cpu.rf.registers.8() -> i32
    func.func private @ac_init_top.cpu.rf.registers.9() -> i32
    func.func private @ac_init_top.cpu.rf.registers.10() -> i32
    func.func private @ac_init_top.cpu.rf.registers.11() -> i32
    func.func private @ac_init_top.cpu.rf.registers.12() -> i32
    func.func private @ac_init_top.cpu.rf.registers.13() -> i32
    func.func private @ac_init_top.cpu.rf.registers.14() -> i32
    func.func private @ac_init_top.cpu.rf.registers.15() -> i32
    func.func private @ac_init_top.cpu.rf.registers.16() -> i32
    func.func private @ac_init_top.cpu.rf.registers.17() -> i32
    func.func private @ac_init_top.cpu.rf.registers.18() -> i32
    func.func private @ac_init_top.cpu.rf.registers.19() -> i32
    func.func private @ac_init_top.cpu.rf.registers.20() -> i32
    func.func private @ac_init_top.cpu.rf.registers.21() -> i32
    func.func private @ac_init_top.cpu.rf.registers.22() -> i32
    func.func private @ac_init_top.cpu.rf.registers.23() -> i32
    func.func private @ac_init_top.cpu.rf.registers.24() -> i32
    func.func private @ac_init_top.cpu.rf.registers.25() -> i32
    func.func private @ac_init_top.cpu.rf.registers.26() -> i32
    func.func private @ac_init_top.cpu.rf.registers.27() -> i32
    func.func private @ac_init_top.cpu.rf.registers.28() -> i32
    func.func private @ac_init_top.cpu.rf.registers.29() -> i32
    func.func private @ac_init_top.cpu.rf.registers.30() -> i32
    func.func private @ac_init_top.cpu.rf.registers.31() -> i32
    func.func private @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched">
    func.func private @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded">
    func.func private @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed">
    func.func private @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed">
    func.func private @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]>
    func.func private @ac_init_top.cpu.fetch.pc() -> i32
    func.func private @ac_init_top.cpu.memory.words.0() -> i32
    func.func private @ac_init_top.cpu.memory.words.1() -> i32
    func.func private @ac_init_top.cpu.memory.words.2() -> i32
    func.func private @ac_init_top.cpu.memory.words.3() -> i32
    func.func private @ac_init_top.cpu.memory.words.4() -> i32
    func.func private @ac_init_top.cpu.memory.words.5() -> i32
    func.func private @ac_init_top.cpu.memory.words.6() -> i32
    func.func private @ac_init_top.cpu.memory.words.7() -> i32
    func.func private @ac_init_top.cpu.memory.words.8() -> i32
    func.func private @ac_init_top.cpu.memory.words.9() -> i32
    func.func private @ac_init_top.cpu.memory.words.10() -> i32
    func.func private @ac_init_top.cpu.memory.words.11() -> i32
    func.func private @ac_init_top.cpu.memory.words.12() -> i32
    func.func private @ac_init_top.cpu.memory.words.13() -> i32
    func.func private @ac_init_top.cpu.memory.words.14() -> i32
    func.func private @ac_init_top.cpu.memory.words.15() -> i32
    func.func private @ac_init_top.cpu.memory.words.16() -> i32
    func.func private @ac_init_top.cpu.memory.words.17() -> i32
    func.func private @ac_init_top.cpu.memory.words.18() -> i32
    func.func private @ac_init_top.cpu.memory.words.19() -> i32
    func.func private @ac_init_top.cpu.memory.words.20() -> i32
    func.func private @ac_init_top.cpu.memory.words.21() -> i32
    func.func private @ac_init_top.cpu.memory.words.22() -> i32
    func.func private @ac_init_top.cpu.memory.words.23() -> i32
    func.func private @ac_init_top.cpu.memory.words.24() -> i32
    func.func private @ac_init_top.cpu.memory.words.25() -> i32
    func.func private @ac_init_top.cpu.memory.words.26() -> i32
    func.func private @ac_init_top.cpu.memory.words.27() -> i32
    func.func private @ac_init_top.cpu.memory.words.28() -> i32
    func.func private @ac_init_top.cpu.memory.words.29() -> i32
    func.func private @ac_init_top.cpu.memory.words.30() -> i32
    func.func private @ac_init_top.cpu.memory.words.31() -> i32
    func.func private @ac_init_top.cpu.memory.words.32() -> i32
    func.func private @ac_init_top.cpu.memory.words.33() -> i32
    func.func private @ac_init_top.cpu.memory.words.34() -> i32
    func.func private @ac_init_top.cpu.memory.words.35() -> i32
    func.func private @ac_init_top.cpu.memory.words.36() -> i32
    func.func private @ac_init_top.cpu.memory.words.37() -> i32
    func.func private @ac_init_top.cpu.memory.words.38() -> i32
    func.func private @ac_init_top.cpu.memory.words.39() -> i32
    func.func private @ac_init_top.cpu.memory.words.40() -> i32
    func.func private @ac_init_top.cpu.memory.words.41() -> i32
    func.func private @ac_init_top.cpu.memory.words.42() -> i32
    func.func private @ac_init_top.cpu.memory.words.43() -> i32
    func.func private @ac_init_top.cpu.memory.words.44() -> i32
    func.func private @ac_init_top.cpu.memory.words.45() -> i32
    func.func private @ac_init_top.cpu.memory.words.46() -> i32
    func.func private @ac_init_top.cpu.memory.words.47() -> i32
    func.func private @ac_init_top.cpu.memory.words.48() -> i32
    func.func private @ac_init_top.cpu.memory.words.49() -> i32
    func.func private @ac_init_top.cpu.memory.words.50() -> i32
    func.func private @ac_init_top.cpu.memory.words.51() -> i32
    func.func private @ac_init_top.cpu.memory.words.52() -> i32
    func.func private @ac_init_top.cpu.memory.words.53() -> i32
    func.func private @ac_init_top.cpu.memory.words.54() -> i32
    func.func private @ac_init_top.cpu.memory.words.55() -> i32
    func.func private @ac_init_top.cpu.memory.words.56() -> i32
    func.func private @ac_init_top.cpu.memory.words.57() -> i32
    func.func private @ac_init_top.cpu.memory.words.58() -> i32
    func.func private @ac_init_top.cpu.memory.words.59() -> i32
    func.func private @ac_init_top.cpu.memory.words.60() -> i32
    func.func private @ac_init_top.cpu.memory.words.61() -> i32
    func.func private @ac_init_top.cpu.memory.words.62() -> i32
    func.func private @ac_init_top.cpu.memory.words.63() -> i32
    func.func private @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent">
    func.func private @ac_init_top.cpu.writeback.retired() -> i32
    func.func private @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement">
    func.func private @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control">
    func.func private @construct.0.call.0.0() -> !acir.array<i32, [256]>
    func.func private @construct.0.call.1.1() -> !acir.array<i32, [32]>
    func.func private @construct.0.call.2.2() -> !acir.array<i32, [64]>
    func.func private @top.cpu_body.0.call.5.9.0.call.5.16(i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.5.9.0.call.6.23(i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(i32, i32) -> i1
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.11.30(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(i32, i32) -> i32
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(i32) -> !acir.struct<"src_isa_e31afbf3.Instruction">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.cpu_body.0.call.4.4 : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">>, !acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 {acir.function = {contract = "Module:top.cpu.hazard_unit", definition = "load_use", instance = "top.cpu.hazard_unit", interface = "Module:top.cpu.hazard_unit:load_use", kind = "rule", name = "top.cpu_body.0.call.4.4", output_type = "bool", owner = "top.cpu.hazard_unit", params = [{id = "v1", name = "source", targets = ["top.cpu.id_ex"], type = "cell<src_isa_e31afbf3.Decoded>"}, {id = "v2", name = "instruction", targets = [], type = "src_isa_e31afbf3.Instruction"}], public = true, result = "bool", rule = "load_use"}} body {
    ^bb0(%arg0: !acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">> loc("acir.binding:top.cpu_body.0.call.4.4:source"), %arg1: !acir.struct<"src_isa_e31afbf3.Instruction"> loc("acir.binding:top.cpu_body.0.call.4.4:instruction")):
      %c0_i32 = arith.constant 0 : i32
      %false = arith.constant false
      %true = arith.constant true
      %0 = acir.cell.read %arg0 {acir.id = "v8", acir.source_type = "src_isa_e31afbf3.Decoded", role = "observe"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">> -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc322)
      %1 = acir.extract %0 {field = "instruction"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc323)
      %2 = acir.extract %1 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc324)
      %3 = scf.if %2 -> (i1) {
        %5 = acir.extract %1 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc325)
        %6 = arith.cmpi ne, %5, %c0_i32 : i32 loc(#loc325)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      %4 = scf.if %3 -> (i1) {
        %5 = acir.extract %1 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc326)
        %6 = acir.extract %arg1 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc327)
        %7 = arith.cmpi eq, %5, %6 : i32 loc(#loc326)
        %8 = scf.if %7 -> (i1) {
          scf.yield %true : i1
        } else {
          %9 = acir.extract %arg1 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc328)
          %10 = arith.cmpi eq, %5, %9 : i32 loc(#loc329)
          scf.yield %10 : i1
        }
        scf.yield %8 : i1
      } else {
        scf.yield %false : i1
      }
      acir.return %4 : i1 {acir.source_type = "void"}
    }
  }
  acir.module @top.cpu.forwarding_unit attributes {acir.instance = {definition = "src_model_bb8d42ed.ForwardingUnit", name = "top.cpu.forwarding_unit", parent = "top.cpu", rules = ["resolve"]}} {
    acir.contract {relations = [], rules = ["resolve"]}
    func.func private @ac_init_top.cpu.rf.registers.0() -> i32
    func.func private @ac_init_top.cpu.rf.registers.1() -> i32
    func.func private @ac_init_top.cpu.rf.registers.2() -> i32
    func.func private @ac_init_top.cpu.rf.registers.3() -> i32
    func.func private @ac_init_top.cpu.rf.registers.4() -> i32
    func.func private @ac_init_top.cpu.rf.registers.5() -> i32
    func.func private @ac_init_top.cpu.rf.registers.6() -> i32
    func.func private @ac_init_top.cpu.rf.registers.7() -> i32
    func.func private @ac_init_top.cpu.rf.registers.8() -> i32
    func.func private @ac_init_top.cpu.rf.registers.9() -> i32
    func.func private @ac_init_top.cpu.rf.registers.10() -> i32
    func.func private @ac_init_top.cpu.rf.registers.11() -> i32
    func.func private @ac_init_top.cpu.rf.registers.12() -> i32
    func.func private @ac_init_top.cpu.rf.registers.13() -> i32
    func.func private @ac_init_top.cpu.rf.registers.14() -> i32
    func.func private @ac_init_top.cpu.rf.registers.15() -> i32
    func.func private @ac_init_top.cpu.rf.registers.16() -> i32
    func.func private @ac_init_top.cpu.rf.registers.17() -> i32
    func.func private @ac_init_top.cpu.rf.registers.18() -> i32
    func.func private @ac_init_top.cpu.rf.registers.19() -> i32
    func.func private @ac_init_top.cpu.rf.registers.20() -> i32
    func.func private @ac_init_top.cpu.rf.registers.21() -> i32
    func.func private @ac_init_top.cpu.rf.registers.22() -> i32
    func.func private @ac_init_top.cpu.rf.registers.23() -> i32
    func.func private @ac_init_top.cpu.rf.registers.24() -> i32
    func.func private @ac_init_top.cpu.rf.registers.25() -> i32
    func.func private @ac_init_top.cpu.rf.registers.26() -> i32
    func.func private @ac_init_top.cpu.rf.registers.27() -> i32
    func.func private @ac_init_top.cpu.rf.registers.28() -> i32
    func.func private @ac_init_top.cpu.rf.registers.29() -> i32
    func.func private @ac_init_top.cpu.rf.registers.30() -> i32
    func.func private @ac_init_top.cpu.rf.registers.31() -> i32
    func.func private @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched">
    func.func private @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded">
    func.func private @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed">
    func.func private @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed">
    func.func private @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]>
    func.func private @ac_init_top.cpu.fetch.pc() -> i32
    func.func private @ac_init_top.cpu.memory.words.0() -> i32
    func.func private @ac_init_top.cpu.memory.words.1() -> i32
    func.func private @ac_init_top.cpu.memory.words.2() -> i32
    func.func private @ac_init_top.cpu.memory.words.3() -> i32
    func.func private @ac_init_top.cpu.memory.words.4() -> i32
    func.func private @ac_init_top.cpu.memory.words.5() -> i32
    func.func private @ac_init_top.cpu.memory.words.6() -> i32
    func.func private @ac_init_top.cpu.memory.words.7() -> i32
    func.func private @ac_init_top.cpu.memory.words.8() -> i32
    func.func private @ac_init_top.cpu.memory.words.9() -> i32
    func.func private @ac_init_top.cpu.memory.words.10() -> i32
    func.func private @ac_init_top.cpu.memory.words.11() -> i32
    func.func private @ac_init_top.cpu.memory.words.12() -> i32
    func.func private @ac_init_top.cpu.memory.words.13() -> i32
    func.func private @ac_init_top.cpu.memory.words.14() -> i32
    func.func private @ac_init_top.cpu.memory.words.15() -> i32
    func.func private @ac_init_top.cpu.memory.words.16() -> i32
    func.func private @ac_init_top.cpu.memory.words.17() -> i32
    func.func private @ac_init_top.cpu.memory.words.18() -> i32
    func.func private @ac_init_top.cpu.memory.words.19() -> i32
    func.func private @ac_init_top.cpu.memory.words.20() -> i32
    func.func private @ac_init_top.cpu.memory.words.21() -> i32
    func.func private @ac_init_top.cpu.memory.words.22() -> i32
    func.func private @ac_init_top.cpu.memory.words.23() -> i32
    func.func private @ac_init_top.cpu.memory.words.24() -> i32
    func.func private @ac_init_top.cpu.memory.words.25() -> i32
    func.func private @ac_init_top.cpu.memory.words.26() -> i32
    func.func private @ac_init_top.cpu.memory.words.27() -> i32
    func.func private @ac_init_top.cpu.memory.words.28() -> i32
    func.func private @ac_init_top.cpu.memory.words.29() -> i32
    func.func private @ac_init_top.cpu.memory.words.30() -> i32
    func.func private @ac_init_top.cpu.memory.words.31() -> i32
    func.func private @ac_init_top.cpu.memory.words.32() -> i32
    func.func private @ac_init_top.cpu.memory.words.33() -> i32
    func.func private @ac_init_top.cpu.memory.words.34() -> i32
    func.func private @ac_init_top.cpu.memory.words.35() -> i32
    func.func private @ac_init_top.cpu.memory.words.36() -> i32
    func.func private @ac_init_top.cpu.memory.words.37() -> i32
    func.func private @ac_init_top.cpu.memory.words.38() -> i32
    func.func private @ac_init_top.cpu.memory.words.39() -> i32
    func.func private @ac_init_top.cpu.memory.words.40() -> i32
    func.func private @ac_init_top.cpu.memory.words.41() -> i32
    func.func private @ac_init_top.cpu.memory.words.42() -> i32
    func.func private @ac_init_top.cpu.memory.words.43() -> i32
    func.func private @ac_init_top.cpu.memory.words.44() -> i32
    func.func private @ac_init_top.cpu.memory.words.45() -> i32
    func.func private @ac_init_top.cpu.memory.words.46() -> i32
    func.func private @ac_init_top.cpu.memory.words.47() -> i32
    func.func private @ac_init_top.cpu.memory.words.48() -> i32
    func.func private @ac_init_top.cpu.memory.words.49() -> i32
    func.func private @ac_init_top.cpu.memory.words.50() -> i32
    func.func private @ac_init_top.cpu.memory.words.51() -> i32
    func.func private @ac_init_top.cpu.memory.words.52() -> i32
    func.func private @ac_init_top.cpu.memory.words.53() -> i32
    func.func private @ac_init_top.cpu.memory.words.54() -> i32
    func.func private @ac_init_top.cpu.memory.words.55() -> i32
    func.func private @ac_init_top.cpu.memory.words.56() -> i32
    func.func private @ac_init_top.cpu.memory.words.57() -> i32
    func.func private @ac_init_top.cpu.memory.words.58() -> i32
    func.func private @ac_init_top.cpu.memory.words.59() -> i32
    func.func private @ac_init_top.cpu.memory.words.60() -> i32
    func.func private @ac_init_top.cpu.memory.words.61() -> i32
    func.func private @ac_init_top.cpu.memory.words.62() -> i32
    func.func private @ac_init_top.cpu.memory.words.63() -> i32
    func.func private @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent">
    func.func private @ac_init_top.cpu.writeback.retired() -> i32
    func.func private @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement">
    func.func private @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control">
    func.func private @construct.0.call.0.0() -> !acir.array<i32, [256]>
    func.func private @construct.0.call.1.1() -> !acir.array<i32, [32]>
    func.func private @construct.0.call.2.2() -> !acir.array<i32, [64]>
    func.func private @top.cpu_body.0.call.5.9.0.call.5.16(i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.5.9.0.call.6.23(i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(i32, i32) -> i1
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.11.30(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(i32, i32) -> i32
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(i32) -> !acir.struct<"src_isa_e31afbf3.Instruction">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.cpu_body.0.call.5.9 : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">>, !acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">>, !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>) -> !acir.struct<"src_isa_e31afbf3.ForwardedOperands"> {acir.function = {contract = "Module:top.cpu.forwarding_unit", definition = "resolve", instance = "top.cpu.forwarding_unit", interface = "Module:top.cpu.forwarding_unit:resolve", kind = "rule", name = "top.cpu_body.0.call.5.9", output_type = "src_isa_e31afbf3.ForwardedOperands", owner = "top.cpu.forwarding_unit", params = [{id = "v1", name = "source", targets = ["top.cpu.id_ex"], type = "cell<src_isa_e31afbf3.Decoded>"}, {id = "v2", name = "older", targets = ["top.cpu.ex_mem"], type = "cell<src_isa_e31afbf3.Executed>"}, {id = "v3", name = "retiring", targets = ["top.cpu.mem_wb"], type = "cell<src_isa_e31afbf3.Completed>"}], public = true, result = "src_isa_e31afbf3.ForwardedOperands", rule = "resolve"}} body {
    ^bb0(%arg0: !acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">> loc("acir.binding:top.cpu_body.0.call.5.9:source"), %arg1: !acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">> loc("acir.binding:top.cpu_body.0.call.5.9:older"), %arg2: !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">> loc("acir.binding:top.cpu_body.0.call.5.9:retiring")):
      %0 = acir.cell.read %arg0 {acir.id = "v9", acir.source_type = "src_isa_e31afbf3.Decoded", role = "observe"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">> -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc333)
      %1 = acir.cell.read %arg1 {acir.id = "v11", acir.source_type = "src_isa_e31afbf3.Executed", role = "observe"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">> -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc334)
      %2 = acir.cell.read %arg2 {acir.id = "v13", acir.source_type = "src_isa_e31afbf3.Completed", role = "observe"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">> -> !acir.struct<"src_isa_e31afbf3.Completed"> loc(#loc335)
      %3 = acir.extract %0 {field = "instruction"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc336)
      %4 = acir.extract %3 {field = "rs1"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc336)
      %5 = func.call @top.cpu_body.0.call.5.9.0.call.5.16(%4, %1, %2) {acir.id = "v17", acir.source_type = "u32", role = "helper"} : (i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc337)
      %6 = acir.extract %3 {field = "rs2"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc338)
      %7 = func.call @top.cpu_body.0.call.5.9.0.call.5.16(%6, %1, %2) {acir.id = "v21", acir.source_type = "u32", role = "helper"} : (i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc339)
      %8 = acir.extract %0 {field = "lhs"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc340)
      %9 = func.call @top.cpu_body.0.call.5.9.0.call.6.23(%8, %5, %1, %2) {acir.id = "v24", acir.source_type = "u32", role = "helper"} : (i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc341)
      %10 = acir.extract %0 {field = "rhs"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc342)
      %11 = func.call @top.cpu_body.0.call.5.9.0.call.6.23(%10, %7, %1, %2) {acir.id = "v27", acir.source_type = "u32", role = "helper"} : (i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc343)
      %12 = acir.aggregate(%9, %11, %5, %7) : (i32, i32, i32, i32) -> !acir.struct<"src_isa_e31afbf3.ForwardedOperands"> loc(#loc344)
      acir.return %12 : !acir.struct<"src_isa_e31afbf3.ForwardedOperands"> {acir.source_type = "void"}
    }
  }
  acir.module @top.cpu.execute attributes {acir.instance = {definition = "src_model_bb8d42ed.Execute", name = "top.cpu.execute", parent = "top.cpu", rules = ["advance", "resolve"]}} {
    acir.contract {relations = [], rules = ["advance", "resolve"]}
    func.func private @ac_init_top.cpu.rf.registers.0() -> i32
    func.func private @ac_init_top.cpu.rf.registers.1() -> i32
    func.func private @ac_init_top.cpu.rf.registers.2() -> i32
    func.func private @ac_init_top.cpu.rf.registers.3() -> i32
    func.func private @ac_init_top.cpu.rf.registers.4() -> i32
    func.func private @ac_init_top.cpu.rf.registers.5() -> i32
    func.func private @ac_init_top.cpu.rf.registers.6() -> i32
    func.func private @ac_init_top.cpu.rf.registers.7() -> i32
    func.func private @ac_init_top.cpu.rf.registers.8() -> i32
    func.func private @ac_init_top.cpu.rf.registers.9() -> i32
    func.func private @ac_init_top.cpu.rf.registers.10() -> i32
    func.func private @ac_init_top.cpu.rf.registers.11() -> i32
    func.func private @ac_init_top.cpu.rf.registers.12() -> i32
    func.func private @ac_init_top.cpu.rf.registers.13() -> i32
    func.func private @ac_init_top.cpu.rf.registers.14() -> i32
    func.func private @ac_init_top.cpu.rf.registers.15() -> i32
    func.func private @ac_init_top.cpu.rf.registers.16() -> i32
    func.func private @ac_init_top.cpu.rf.registers.17() -> i32
    func.func private @ac_init_top.cpu.rf.registers.18() -> i32
    func.func private @ac_init_top.cpu.rf.registers.19() -> i32
    func.func private @ac_init_top.cpu.rf.registers.20() -> i32
    func.func private @ac_init_top.cpu.rf.registers.21() -> i32
    func.func private @ac_init_top.cpu.rf.registers.22() -> i32
    func.func private @ac_init_top.cpu.rf.registers.23() -> i32
    func.func private @ac_init_top.cpu.rf.registers.24() -> i32
    func.func private @ac_init_top.cpu.rf.registers.25() -> i32
    func.func private @ac_init_top.cpu.rf.registers.26() -> i32
    func.func private @ac_init_top.cpu.rf.registers.27() -> i32
    func.func private @ac_init_top.cpu.rf.registers.28() -> i32
    func.func private @ac_init_top.cpu.rf.registers.29() -> i32
    func.func private @ac_init_top.cpu.rf.registers.30() -> i32
    func.func private @ac_init_top.cpu.rf.registers.31() -> i32
    func.func private @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched">
    func.func private @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded">
    func.func private @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed">
    func.func private @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed">
    func.func private @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]>
    func.func private @ac_init_top.cpu.fetch.pc() -> i32
    func.func private @ac_init_top.cpu.memory.words.0() -> i32
    func.func private @ac_init_top.cpu.memory.words.1() -> i32
    func.func private @ac_init_top.cpu.memory.words.2() -> i32
    func.func private @ac_init_top.cpu.memory.words.3() -> i32
    func.func private @ac_init_top.cpu.memory.words.4() -> i32
    func.func private @ac_init_top.cpu.memory.words.5() -> i32
    func.func private @ac_init_top.cpu.memory.words.6() -> i32
    func.func private @ac_init_top.cpu.memory.words.7() -> i32
    func.func private @ac_init_top.cpu.memory.words.8() -> i32
    func.func private @ac_init_top.cpu.memory.words.9() -> i32
    func.func private @ac_init_top.cpu.memory.words.10() -> i32
    func.func private @ac_init_top.cpu.memory.words.11() -> i32
    func.func private @ac_init_top.cpu.memory.words.12() -> i32
    func.func private @ac_init_top.cpu.memory.words.13() -> i32
    func.func private @ac_init_top.cpu.memory.words.14() -> i32
    func.func private @ac_init_top.cpu.memory.words.15() -> i32
    func.func private @ac_init_top.cpu.memory.words.16() -> i32
    func.func private @ac_init_top.cpu.memory.words.17() -> i32
    func.func private @ac_init_top.cpu.memory.words.18() -> i32
    func.func private @ac_init_top.cpu.memory.words.19() -> i32
    func.func private @ac_init_top.cpu.memory.words.20() -> i32
    func.func private @ac_init_top.cpu.memory.words.21() -> i32
    func.func private @ac_init_top.cpu.memory.words.22() -> i32
    func.func private @ac_init_top.cpu.memory.words.23() -> i32
    func.func private @ac_init_top.cpu.memory.words.24() -> i32
    func.func private @ac_init_top.cpu.memory.words.25() -> i32
    func.func private @ac_init_top.cpu.memory.words.26() -> i32
    func.func private @ac_init_top.cpu.memory.words.27() -> i32
    func.func private @ac_init_top.cpu.memory.words.28() -> i32
    func.func private @ac_init_top.cpu.memory.words.29() -> i32
    func.func private @ac_init_top.cpu.memory.words.30() -> i32
    func.func private @ac_init_top.cpu.memory.words.31() -> i32
    func.func private @ac_init_top.cpu.memory.words.32() -> i32
    func.func private @ac_init_top.cpu.memory.words.33() -> i32
    func.func private @ac_init_top.cpu.memory.words.34() -> i32
    func.func private @ac_init_top.cpu.memory.words.35() -> i32
    func.func private @ac_init_top.cpu.memory.words.36() -> i32
    func.func private @ac_init_top.cpu.memory.words.37() -> i32
    func.func private @ac_init_top.cpu.memory.words.38() -> i32
    func.func private @ac_init_top.cpu.memory.words.39() -> i32
    func.func private @ac_init_top.cpu.memory.words.40() -> i32
    func.func private @ac_init_top.cpu.memory.words.41() -> i32
    func.func private @ac_init_top.cpu.memory.words.42() -> i32
    func.func private @ac_init_top.cpu.memory.words.43() -> i32
    func.func private @ac_init_top.cpu.memory.words.44() -> i32
    func.func private @ac_init_top.cpu.memory.words.45() -> i32
    func.func private @ac_init_top.cpu.memory.words.46() -> i32
    func.func private @ac_init_top.cpu.memory.words.47() -> i32
    func.func private @ac_init_top.cpu.memory.words.48() -> i32
    func.func private @ac_init_top.cpu.memory.words.49() -> i32
    func.func private @ac_init_top.cpu.memory.words.50() -> i32
    func.func private @ac_init_top.cpu.memory.words.51() -> i32
    func.func private @ac_init_top.cpu.memory.words.52() -> i32
    func.func private @ac_init_top.cpu.memory.words.53() -> i32
    func.func private @ac_init_top.cpu.memory.words.54() -> i32
    func.func private @ac_init_top.cpu.memory.words.55() -> i32
    func.func private @ac_init_top.cpu.memory.words.56() -> i32
    func.func private @ac_init_top.cpu.memory.words.57() -> i32
    func.func private @ac_init_top.cpu.memory.words.58() -> i32
    func.func private @ac_init_top.cpu.memory.words.59() -> i32
    func.func private @ac_init_top.cpu.memory.words.60() -> i32
    func.func private @ac_init_top.cpu.memory.words.61() -> i32
    func.func private @ac_init_top.cpu.memory.words.62() -> i32
    func.func private @ac_init_top.cpu.memory.words.63() -> i32
    func.func private @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent">
    func.func private @ac_init_top.cpu.writeback.retired() -> i32
    func.func private @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement">
    func.func private @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control">
    func.func private @construct.0.call.0.0() -> !acir.array<i32, [256]>
    func.func private @construct.0.call.1.1() -> !acir.array<i32, [32]>
    func.func private @construct.0.call.2.2() -> !acir.array<i32, [64]>
    func.func private @top.cpu_body.0.call.5.9.0.call.5.16(i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.5.9.0.call.6.23(i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(i32, i32) -> i1
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.11.30(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(i32, i32) -> i32
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(i32) -> !acir.struct<"src_isa_e31afbf3.Instruction">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.cpu_body.0.call.8.12 : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">>, !acir.struct<"src_isa_e31afbf3.ForwardedOperands">) -> !acir.struct<"src_isa_e31afbf3.ExecutionResult"> {acir.function = {contract = "Module:top.cpu.execute", definition = "resolve", instance = "top.cpu.execute", interface = "Module:top.cpu.execute:resolve", kind = "rule", name = "top.cpu_body.0.call.8.12", output_type = "src_isa_e31afbf3.ExecutionResult", owner = "top.cpu.execute", params = [{id = "v1", name = "source", targets = ["top.cpu.id_ex"], type = "cell<src_isa_e31afbf3.Decoded>"}, {id = "v2", name = "operands", targets = [], type = "src_isa_e31afbf3.ForwardedOperands"}], public = true, result = "src_isa_e31afbf3.ExecutionResult", rule = "resolve"}} body {
    ^bb0(%arg0: !acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">> loc("acir.binding:top.cpu_body.0.call.8.12:source"), %arg1: !acir.struct<"src_isa_e31afbf3.ForwardedOperands"> loc("acir.binding:top.cpu_body.0.call.8.12:operands")):
      %c0_i32 = arith.constant 0 : i32
      %c4_i32 = arith.constant 4 : i32
      %c103_i32 = arith.constant 103 : i32
      %c111_i32 = arith.constant 111 : i32 loc(#loc347)
      %true = arith.constant true
      %0 = acir.cell.read %arg0 {acir.id = "v8", acir.source_type = "src_isa_e31afbf3.Decoded", role = "observe"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Decoded">> -> !acir.struct<"src_isa_e31afbf3.Decoded"> loc(#loc348)
      %1 = acir.extract %0 {field = "instruction"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> !acir.struct<"src_isa_e31afbf3.Instruction"> loc(#loc349)
      %2 = acir.extract %arg1 {field = "lhs"} : (!acir.struct<"src_isa_e31afbf3.ForwardedOperands">) -> i32 loc(#loc350)
      %3 = acir.extract %arg1 {field = "rhs"} : (!acir.struct<"src_isa_e31afbf3.ForwardedOperands">) -> i32 loc(#loc351)
      %4 = acir.extract %0 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc352)
      %5 = func.call @top.cpu_body.0.call.8.12.0.call.8.16(%1, %4, %2, %3) {acir.id = "v17", acir.source_type = "u32", role = "helper"} : (!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32 loc(#loc353)
      %6 = acir.extract %1 {field = "opcode"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc354)
      %7 = arith.cmpi eq, %6, %c111_i32 : i32 loc(#loc354)
      %8 = scf.if %7 -> (i1) {
        scf.yield %true : i1
      } else {
        %20 = arith.cmpi eq, %6, %c103_i32 : i32 loc(#loc356)
        scf.yield %20 : i1
      } loc(#loc355)
      %9:2 = scf.if %8 -> (i1, i32) {
        %20 = arith.addi %4, %c4_i32 : i32 loc(#loc358)
        scf.yield %true, %20 : i1, i32
      } else {
        %20 = func.call @top.cpu_body.0.call.8.12.0.call.11.30(%1, %2, %3) {acir.id = "v31", acir.source_type = "bool", role = "helper"} : (!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1 loc(#loc359)
        scf.yield %20, %5 : i1, i32
      } loc(#loc357)
      %10 = acir.extract %0 {field = "valid"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i1 loc(#loc360)
      %11 = acir.extract %0 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i32 loc(#loc361)
      %12 = acir.extract %1 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i32 loc(#loc362)
      %13 = acir.extract %1 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc363)
      %14 = acir.extract %1 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc364)
      %15 = acir.extract %1 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Instruction">) -> i1 loc(#loc365)
      %16 = acir.extract %0 {field = "stalled"} : (!acir.struct<"src_isa_e31afbf3.Decoded">) -> i1 loc(#loc366)
      %17 = acir.aggregate(%10, %4, %11, %12, %13, %14, %15, %5, %3, %9#1, %16) : (i1, i32, i32, i32, i1, i1, i1, i32, i32, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc367)
      %18 = arith.select %9#0, %5, %c0_i32 : i32
      %19 = acir.aggregate(%17, %9#0, %18) : (!acir.struct<"src_isa_e31afbf3.Executed">, i1, i32) -> !acir.struct<"src_isa_e31afbf3.ExecutionResult"> loc(#loc368)
      acir.return %19 : !acir.struct<"src_isa_e31afbf3.ExecutionResult"> {acir.source_type = "void"}
    }
    acir.rule @top.cpu.execute_body.0.call.19.2 : (!acir.struct<"src_isa_e31afbf3.Executed">) -> !acir.struct<"src_isa_e31afbf3.Executed"> {acir.function = {contract = "Module:top.cpu.execute", definition = "advance", instance = "top.cpu.execute", interface = "Module:top.cpu.execute:advance", kind = "rule", name = "top.cpu.execute_body.0.call.19.2", output_type = "cell<src_isa_e31afbf3.Executed>", owner = "top.cpu.execute", params = [{id = "v1", name = "packet", targets = [], type = "src_isa_e31afbf3.Executed"}], public = true, result = "src_isa_e31afbf3.Executed", rule = "advance"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_e31afbf3.Executed"> loc("acir.binding:top.cpu.execute_body.0.call.19.2:packet")):
      %true = arith.constant true
      %0 = "acir.optional"(%arg0, %true) : (!acir.struct<"src_isa_e31afbf3.Executed">, i1) -> !acir.struct<"src_isa_e31afbf3.Executed">
      acir.return %0 : !acir.struct<"src_isa_e31afbf3.Executed"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.cpu.execute_body", owner = "top.cpu.execute", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.external "top.cpu_body" "v27" {acir.source_type = "src_isa_e31afbf3.Executed"} : !acir.struct<"src_isa_e31afbf3.Executed">
      %1 = acir.rule.call @top.cpu.execute::@top.cpu.execute_body.0.call.19.2(%0) {acir.id = "v3", acir.source_type = "src_isa_e31afbf3.Executed", contract = "Module:top.cpu.execute", instance = "top.cpu.execute", interface = "Module:top.cpu.execute:advance", output_binding = @top.cpu::@top.cpu.ex_mem, role = "rule", rule = "advance"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc370)
      %2 = acir.ref @top.cpu::@top.cpu.ex_mem : !acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">>
      %false = arith.constant false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.cpu.memory attributes {acir.instance = {definition = "src_model_bb8d42ed.Memory", name = "top.cpu.memory", parent = "top.cpu", rules = ["advance"]}} {
    acir.contract {relations = [], rules = ["advance"]}
    acir.cell @top.cpu.memory.words.0 : i32 {initial_fn = "ac_init_top.cpu.memory.words.0", name = "top.cpu.memory.words.0", type = "u32"}
    acir.cell @top.cpu.memory.words.1 : i32 {initial_fn = "ac_init_top.cpu.memory.words.1", name = "top.cpu.memory.words.1", type = "u32"}
    acir.cell @top.cpu.memory.words.2 : i32 {initial_fn = "ac_init_top.cpu.memory.words.2", name = "top.cpu.memory.words.2", type = "u32"}
    acir.cell @top.cpu.memory.words.3 : i32 {initial_fn = "ac_init_top.cpu.memory.words.3", name = "top.cpu.memory.words.3", type = "u32"}
    acir.cell @top.cpu.memory.words.4 : i32 {initial_fn = "ac_init_top.cpu.memory.words.4", name = "top.cpu.memory.words.4", type = "u32"}
    acir.cell @top.cpu.memory.words.5 : i32 {initial_fn = "ac_init_top.cpu.memory.words.5", name = "top.cpu.memory.words.5", type = "u32"}
    acir.cell @top.cpu.memory.words.6 : i32 {initial_fn = "ac_init_top.cpu.memory.words.6", name = "top.cpu.memory.words.6", type = "u32"}
    acir.cell @top.cpu.memory.words.7 : i32 {initial_fn = "ac_init_top.cpu.memory.words.7", name = "top.cpu.memory.words.7", type = "u32"}
    acir.cell @top.cpu.memory.words.8 : i32 {initial_fn = "ac_init_top.cpu.memory.words.8", name = "top.cpu.memory.words.8", type = "u32"}
    acir.cell @top.cpu.memory.words.9 : i32 {initial_fn = "ac_init_top.cpu.memory.words.9", name = "top.cpu.memory.words.9", type = "u32"}
    acir.cell @top.cpu.memory.words.10 : i32 {initial_fn = "ac_init_top.cpu.memory.words.10", name = "top.cpu.memory.words.10", type = "u32"}
    acir.cell @top.cpu.memory.words.11 : i32 {initial_fn = "ac_init_top.cpu.memory.words.11", name = "top.cpu.memory.words.11", type = "u32"}
    acir.cell @top.cpu.memory.words.12 : i32 {initial_fn = "ac_init_top.cpu.memory.words.12", name = "top.cpu.memory.words.12", type = "u32"}
    acir.cell @top.cpu.memory.words.13 : i32 {initial_fn = "ac_init_top.cpu.memory.words.13", name = "top.cpu.memory.words.13", type = "u32"}
    acir.cell @top.cpu.memory.words.14 : i32 {initial_fn = "ac_init_top.cpu.memory.words.14", name = "top.cpu.memory.words.14", type = "u32"}
    acir.cell @top.cpu.memory.words.15 : i32 {initial_fn = "ac_init_top.cpu.memory.words.15", name = "top.cpu.memory.words.15", type = "u32"}
    acir.cell @top.cpu.memory.words.16 : i32 {initial_fn = "ac_init_top.cpu.memory.words.16", name = "top.cpu.memory.words.16", type = "u32"}
    acir.cell @top.cpu.memory.words.17 : i32 {initial_fn = "ac_init_top.cpu.memory.words.17", name = "top.cpu.memory.words.17", type = "u32"}
    acir.cell @top.cpu.memory.words.18 : i32 {initial_fn = "ac_init_top.cpu.memory.words.18", name = "top.cpu.memory.words.18", type = "u32"}
    acir.cell @top.cpu.memory.words.19 : i32 {initial_fn = "ac_init_top.cpu.memory.words.19", name = "top.cpu.memory.words.19", type = "u32"}
    acir.cell @top.cpu.memory.words.20 : i32 {initial_fn = "ac_init_top.cpu.memory.words.20", name = "top.cpu.memory.words.20", type = "u32"}
    acir.cell @top.cpu.memory.words.21 : i32 {initial_fn = "ac_init_top.cpu.memory.words.21", name = "top.cpu.memory.words.21", type = "u32"}
    acir.cell @top.cpu.memory.words.22 : i32 {initial_fn = "ac_init_top.cpu.memory.words.22", name = "top.cpu.memory.words.22", type = "u32"}
    acir.cell @top.cpu.memory.words.23 : i32 {initial_fn = "ac_init_top.cpu.memory.words.23", name = "top.cpu.memory.words.23", type = "u32"}
    acir.cell @top.cpu.memory.words.24 : i32 {initial_fn = "ac_init_top.cpu.memory.words.24", name = "top.cpu.memory.words.24", type = "u32"}
    acir.cell @top.cpu.memory.words.25 : i32 {initial_fn = "ac_init_top.cpu.memory.words.25", name = "top.cpu.memory.words.25", type = "u32"}
    acir.cell @top.cpu.memory.words.26 : i32 {initial_fn = "ac_init_top.cpu.memory.words.26", name = "top.cpu.memory.words.26", type = "u32"}
    acir.cell @top.cpu.memory.words.27 : i32 {initial_fn = "ac_init_top.cpu.memory.words.27", name = "top.cpu.memory.words.27", type = "u32"}
    acir.cell @top.cpu.memory.words.28 : i32 {initial_fn = "ac_init_top.cpu.memory.words.28", name = "top.cpu.memory.words.28", type = "u32"}
    acir.cell @top.cpu.memory.words.29 : i32 {initial_fn = "ac_init_top.cpu.memory.words.29", name = "top.cpu.memory.words.29", type = "u32"}
    acir.cell @top.cpu.memory.words.30 : i32 {initial_fn = "ac_init_top.cpu.memory.words.30", name = "top.cpu.memory.words.30", type = "u32"}
    acir.cell @top.cpu.memory.words.31 : i32 {initial_fn = "ac_init_top.cpu.memory.words.31", name = "top.cpu.memory.words.31", type = "u32"}
    acir.cell @top.cpu.memory.words.32 : i32 {initial_fn = "ac_init_top.cpu.memory.words.32", name = "top.cpu.memory.words.32", type = "u32"}
    acir.cell @top.cpu.memory.words.33 : i32 {initial_fn = "ac_init_top.cpu.memory.words.33", name = "top.cpu.memory.words.33", type = "u32"}
    acir.cell @top.cpu.memory.words.34 : i32 {initial_fn = "ac_init_top.cpu.memory.words.34", name = "top.cpu.memory.words.34", type = "u32"}
    acir.cell @top.cpu.memory.words.35 : i32 {initial_fn = "ac_init_top.cpu.memory.words.35", name = "top.cpu.memory.words.35", type = "u32"}
    acir.cell @top.cpu.memory.words.36 : i32 {initial_fn = "ac_init_top.cpu.memory.words.36", name = "top.cpu.memory.words.36", type = "u32"}
    acir.cell @top.cpu.memory.words.37 : i32 {initial_fn = "ac_init_top.cpu.memory.words.37", name = "top.cpu.memory.words.37", type = "u32"}
    acir.cell @top.cpu.memory.words.38 : i32 {initial_fn = "ac_init_top.cpu.memory.words.38", name = "top.cpu.memory.words.38", type = "u32"}
    acir.cell @top.cpu.memory.words.39 : i32 {initial_fn = "ac_init_top.cpu.memory.words.39", name = "top.cpu.memory.words.39", type = "u32"}
    acir.cell @top.cpu.memory.words.40 : i32 {initial_fn = "ac_init_top.cpu.memory.words.40", name = "top.cpu.memory.words.40", type = "u32"}
    acir.cell @top.cpu.memory.words.41 : i32 {initial_fn = "ac_init_top.cpu.memory.words.41", name = "top.cpu.memory.words.41", type = "u32"}
    acir.cell @top.cpu.memory.words.42 : i32 {initial_fn = "ac_init_top.cpu.memory.words.42", name = "top.cpu.memory.words.42", type = "u32"}
    acir.cell @top.cpu.memory.words.43 : i32 {initial_fn = "ac_init_top.cpu.memory.words.43", name = "top.cpu.memory.words.43", type = "u32"}
    acir.cell @top.cpu.memory.words.44 : i32 {initial_fn = "ac_init_top.cpu.memory.words.44", name = "top.cpu.memory.words.44", type = "u32"}
    acir.cell @top.cpu.memory.words.45 : i32 {initial_fn = "ac_init_top.cpu.memory.words.45", name = "top.cpu.memory.words.45", type = "u32"}
    acir.cell @top.cpu.memory.words.46 : i32 {initial_fn = "ac_init_top.cpu.memory.words.46", name = "top.cpu.memory.words.46", type = "u32"}
    acir.cell @top.cpu.memory.words.47 : i32 {initial_fn = "ac_init_top.cpu.memory.words.47", name = "top.cpu.memory.words.47", type = "u32"}
    acir.cell @top.cpu.memory.words.48 : i32 {initial_fn = "ac_init_top.cpu.memory.words.48", name = "top.cpu.memory.words.48", type = "u32"}
    acir.cell @top.cpu.memory.words.49 : i32 {initial_fn = "ac_init_top.cpu.memory.words.49", name = "top.cpu.memory.words.49", type = "u32"}
    acir.cell @top.cpu.memory.words.50 : i32 {initial_fn = "ac_init_top.cpu.memory.words.50", name = "top.cpu.memory.words.50", type = "u32"}
    acir.cell @top.cpu.memory.words.51 : i32 {initial_fn = "ac_init_top.cpu.memory.words.51", name = "top.cpu.memory.words.51", type = "u32"}
    acir.cell @top.cpu.memory.words.52 : i32 {initial_fn = "ac_init_top.cpu.memory.words.52", name = "top.cpu.memory.words.52", type = "u32"}
    acir.cell @top.cpu.memory.words.53 : i32 {initial_fn = "ac_init_top.cpu.memory.words.53", name = "top.cpu.memory.words.53", type = "u32"}
    acir.cell @top.cpu.memory.words.54 : i32 {initial_fn = "ac_init_top.cpu.memory.words.54", name = "top.cpu.memory.words.54", type = "u32"}
    acir.cell @top.cpu.memory.words.55 : i32 {initial_fn = "ac_init_top.cpu.memory.words.55", name = "top.cpu.memory.words.55", type = "u32"}
    acir.cell @top.cpu.memory.words.56 : i32 {initial_fn = "ac_init_top.cpu.memory.words.56", name = "top.cpu.memory.words.56", type = "u32"}
    acir.cell @top.cpu.memory.words.57 : i32 {initial_fn = "ac_init_top.cpu.memory.words.57", name = "top.cpu.memory.words.57", type = "u32"}
    acir.cell @top.cpu.memory.words.58 : i32 {initial_fn = "ac_init_top.cpu.memory.words.58", name = "top.cpu.memory.words.58", type = "u32"}
    acir.cell @top.cpu.memory.words.59 : i32 {initial_fn = "ac_init_top.cpu.memory.words.59", name = "top.cpu.memory.words.59", type = "u32"}
    acir.cell @top.cpu.memory.words.60 : i32 {initial_fn = "ac_init_top.cpu.memory.words.60", name = "top.cpu.memory.words.60", type = "u32"}
    acir.cell @top.cpu.memory.words.61 : i32 {initial_fn = "ac_init_top.cpu.memory.words.61", name = "top.cpu.memory.words.61", type = "u32"}
    acir.cell @top.cpu.memory.words.62 : i32 {initial_fn = "ac_init_top.cpu.memory.words.62", name = "top.cpu.memory.words.62", type = "u32"}
    acir.cell @top.cpu.memory.words.63 : i32 {initial_fn = "ac_init_top.cpu.memory.words.63", name = "top.cpu.memory.words.63", type = "u32"}
    acir.cell @top.cpu.memory.store_event : !acir.struct<"src_isa_e31afbf3.StoreEvent"> {initial_fn = "ac_init_top.cpu.memory.store_event", name = "top.cpu.memory.store_event", type = "src_isa_e31afbf3.StoreEvent"}
    func.func private @ac_init_top.cpu.rf.registers.0() -> i32
    func.func private @ac_init_top.cpu.rf.registers.1() -> i32
    func.func private @ac_init_top.cpu.rf.registers.2() -> i32
    func.func private @ac_init_top.cpu.rf.registers.3() -> i32
    func.func private @ac_init_top.cpu.rf.registers.4() -> i32
    func.func private @ac_init_top.cpu.rf.registers.5() -> i32
    func.func private @ac_init_top.cpu.rf.registers.6() -> i32
    func.func private @ac_init_top.cpu.rf.registers.7() -> i32
    func.func private @ac_init_top.cpu.rf.registers.8() -> i32
    func.func private @ac_init_top.cpu.rf.registers.9() -> i32
    func.func private @ac_init_top.cpu.rf.registers.10() -> i32
    func.func private @ac_init_top.cpu.rf.registers.11() -> i32
    func.func private @ac_init_top.cpu.rf.registers.12() -> i32
    func.func private @ac_init_top.cpu.rf.registers.13() -> i32
    func.func private @ac_init_top.cpu.rf.registers.14() -> i32
    func.func private @ac_init_top.cpu.rf.registers.15() -> i32
    func.func private @ac_init_top.cpu.rf.registers.16() -> i32
    func.func private @ac_init_top.cpu.rf.registers.17() -> i32
    func.func private @ac_init_top.cpu.rf.registers.18() -> i32
    func.func private @ac_init_top.cpu.rf.registers.19() -> i32
    func.func private @ac_init_top.cpu.rf.registers.20() -> i32
    func.func private @ac_init_top.cpu.rf.registers.21() -> i32
    func.func private @ac_init_top.cpu.rf.registers.22() -> i32
    func.func private @ac_init_top.cpu.rf.registers.23() -> i32
    func.func private @ac_init_top.cpu.rf.registers.24() -> i32
    func.func private @ac_init_top.cpu.rf.registers.25() -> i32
    func.func private @ac_init_top.cpu.rf.registers.26() -> i32
    func.func private @ac_init_top.cpu.rf.registers.27() -> i32
    func.func private @ac_init_top.cpu.rf.registers.28() -> i32
    func.func private @ac_init_top.cpu.rf.registers.29() -> i32
    func.func private @ac_init_top.cpu.rf.registers.30() -> i32
    func.func private @ac_init_top.cpu.rf.registers.31() -> i32
    func.func private @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched">
    func.func private @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded">
    func.func private @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed">
    func.func private @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed">
    func.func private @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]>
    func.func private @ac_init_top.cpu.fetch.pc() -> i32
    func.func private @ac_init_top.cpu.memory.words.0() -> i32
    func.func private @ac_init_top.cpu.memory.words.1() -> i32
    func.func private @ac_init_top.cpu.memory.words.2() -> i32
    func.func private @ac_init_top.cpu.memory.words.3() -> i32
    func.func private @ac_init_top.cpu.memory.words.4() -> i32
    func.func private @ac_init_top.cpu.memory.words.5() -> i32
    func.func private @ac_init_top.cpu.memory.words.6() -> i32
    func.func private @ac_init_top.cpu.memory.words.7() -> i32
    func.func private @ac_init_top.cpu.memory.words.8() -> i32
    func.func private @ac_init_top.cpu.memory.words.9() -> i32
    func.func private @ac_init_top.cpu.memory.words.10() -> i32
    func.func private @ac_init_top.cpu.memory.words.11() -> i32
    func.func private @ac_init_top.cpu.memory.words.12() -> i32
    func.func private @ac_init_top.cpu.memory.words.13() -> i32
    func.func private @ac_init_top.cpu.memory.words.14() -> i32
    func.func private @ac_init_top.cpu.memory.words.15() -> i32
    func.func private @ac_init_top.cpu.memory.words.16() -> i32
    func.func private @ac_init_top.cpu.memory.words.17() -> i32
    func.func private @ac_init_top.cpu.memory.words.18() -> i32
    func.func private @ac_init_top.cpu.memory.words.19() -> i32
    func.func private @ac_init_top.cpu.memory.words.20() -> i32
    func.func private @ac_init_top.cpu.memory.words.21() -> i32
    func.func private @ac_init_top.cpu.memory.words.22() -> i32
    func.func private @ac_init_top.cpu.memory.words.23() -> i32
    func.func private @ac_init_top.cpu.memory.words.24() -> i32
    func.func private @ac_init_top.cpu.memory.words.25() -> i32
    func.func private @ac_init_top.cpu.memory.words.26() -> i32
    func.func private @ac_init_top.cpu.memory.words.27() -> i32
    func.func private @ac_init_top.cpu.memory.words.28() -> i32
    func.func private @ac_init_top.cpu.memory.words.29() -> i32
    func.func private @ac_init_top.cpu.memory.words.30() -> i32
    func.func private @ac_init_top.cpu.memory.words.31() -> i32
    func.func private @ac_init_top.cpu.memory.words.32() -> i32
    func.func private @ac_init_top.cpu.memory.words.33() -> i32
    func.func private @ac_init_top.cpu.memory.words.34() -> i32
    func.func private @ac_init_top.cpu.memory.words.35() -> i32
    func.func private @ac_init_top.cpu.memory.words.36() -> i32
    func.func private @ac_init_top.cpu.memory.words.37() -> i32
    func.func private @ac_init_top.cpu.memory.words.38() -> i32
    func.func private @ac_init_top.cpu.memory.words.39() -> i32
    func.func private @ac_init_top.cpu.memory.words.40() -> i32
    func.func private @ac_init_top.cpu.memory.words.41() -> i32
    func.func private @ac_init_top.cpu.memory.words.42() -> i32
    func.func private @ac_init_top.cpu.memory.words.43() -> i32
    func.func private @ac_init_top.cpu.memory.words.44() -> i32
    func.func private @ac_init_top.cpu.memory.words.45() -> i32
    func.func private @ac_init_top.cpu.memory.words.46() -> i32
    func.func private @ac_init_top.cpu.memory.words.47() -> i32
    func.func private @ac_init_top.cpu.memory.words.48() -> i32
    func.func private @ac_init_top.cpu.memory.words.49() -> i32
    func.func private @ac_init_top.cpu.memory.words.50() -> i32
    func.func private @ac_init_top.cpu.memory.words.51() -> i32
    func.func private @ac_init_top.cpu.memory.words.52() -> i32
    func.func private @ac_init_top.cpu.memory.words.53() -> i32
    func.func private @ac_init_top.cpu.memory.words.54() -> i32
    func.func private @ac_init_top.cpu.memory.words.55() -> i32
    func.func private @ac_init_top.cpu.memory.words.56() -> i32
    func.func private @ac_init_top.cpu.memory.words.57() -> i32
    func.func private @ac_init_top.cpu.memory.words.58() -> i32
    func.func private @ac_init_top.cpu.memory.words.59() -> i32
    func.func private @ac_init_top.cpu.memory.words.60() -> i32
    func.func private @ac_init_top.cpu.memory.words.61() -> i32
    func.func private @ac_init_top.cpu.memory.words.62() -> i32
    func.func private @ac_init_top.cpu.memory.words.63() -> i32
    func.func private @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent">
    func.func private @ac_init_top.cpu.writeback.retired() -> i32
    func.func private @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement">
    func.func private @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control">
    func.func private @construct.0.call.0.0() -> !acir.array<i32, [256]>
    func.func private @construct.0.call.1.1() -> !acir.array<i32, [32]>
    func.func private @construct.0.call.2.2() -> !acir.array<i32, [64]>
    func.func private @top.cpu_body.0.call.5.9.0.call.5.16(i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.5.9.0.call.6.23(i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(i32, i32) -> i1
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.11.30(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(i32, i32) -> i32
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(i32) -> !acir.struct<"src_isa_e31afbf3.Instruction">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.cpu.memory_body.0.call.20.2 : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">>) -> !acir.struct<"src_isa_e31afbf3.Completed"> {acir.function = {contract = "Module:top.cpu.memory", definition = "advance", instance = "top.cpu.memory", interface = "Module:top.cpu.memory:advance", kind = "rule", name = "top.cpu.memory_body.0.call.20.2", output_type = "cell<src_isa_e31afbf3.Completed>", owner = "top.cpu.memory", params = [{id = "v1", name = "source", targets = ["top.cpu.ex_mem"], type = "cell<src_isa_e31afbf3.Executed>"}], public = true, result = "src_isa_e31afbf3.Completed", rule = "advance"}} body {
    ^bb0(%arg0: !acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">> loc("acir.binding:top.cpu.memory_body.0.call.20.2:source")):
      %c2_i32 = arith.constant 2 : i32
      %c256_i32 = arith.constant 256 : i32
      %c3_i32 = arith.constant 3 : i32
      %c4096_i32 = arith.constant 4096 : i32 loc(#loc372)
      %c0_i32 = arith.constant 0 : i32
      %false = arith.constant false
      %true = arith.constant true
      %0 = acir.cell.read %arg0 {acir.id = "v7", acir.source_type = "src_isa_e31afbf3.Executed", role = "observe"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">> -> !acir.struct<"src_isa_e31afbf3.Executed"> loc(#loc373)
      %1 = acir.extract %0 {field = "value"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc374)
      %2 = acir.aggregate(%false, %c0_i32, %c0_i32) : (i1, i32, i32) -> !acir.struct<"src_isa_e31afbf3.StoreEvent"> loc(#loc375)
      %3 = acir.extract %0 {field = "load"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc376)
      %4 = scf.if %3 -> (i1) {
        scf.yield %true : i1
      } else {
        %82 = acir.extract %0 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc377)
        scf.yield %82 : i1
      }
      %5 = acir.ref @top.cpu.memory::@top.cpu.memory.words.0 : !acir.cell<i32>
      %6 = acir.ref @top.cpu.memory::@top.cpu.memory.words.1 : !acir.cell<i32>
      %7 = acir.ref @top.cpu.memory::@top.cpu.memory.words.2 : !acir.cell<i32>
      %8 = acir.ref @top.cpu.memory::@top.cpu.memory.words.3 : !acir.cell<i32>
      %9 = acir.ref @top.cpu.memory::@top.cpu.memory.words.4 : !acir.cell<i32>
      %10 = acir.ref @top.cpu.memory::@top.cpu.memory.words.5 : !acir.cell<i32>
      %11 = acir.ref @top.cpu.memory::@top.cpu.memory.words.6 : !acir.cell<i32>
      %12 = acir.ref @top.cpu.memory::@top.cpu.memory.words.7 : !acir.cell<i32>
      %13 = acir.ref @top.cpu.memory::@top.cpu.memory.words.8 : !acir.cell<i32>
      %14 = acir.ref @top.cpu.memory::@top.cpu.memory.words.9 : !acir.cell<i32>
      %15 = acir.ref @top.cpu.memory::@top.cpu.memory.words.10 : !acir.cell<i32>
      %16 = acir.ref @top.cpu.memory::@top.cpu.memory.words.11 : !acir.cell<i32>
      %17 = acir.ref @top.cpu.memory::@top.cpu.memory.words.12 : !acir.cell<i32>
      %18 = acir.ref @top.cpu.memory::@top.cpu.memory.words.13 : !acir.cell<i32>
      %19 = acir.ref @top.cpu.memory::@top.cpu.memory.words.14 : !acir.cell<i32>
      %20 = acir.ref @top.cpu.memory::@top.cpu.memory.words.15 : !acir.cell<i32>
      %21 = acir.ref @top.cpu.memory::@top.cpu.memory.words.16 : !acir.cell<i32>
      %22 = acir.ref @top.cpu.memory::@top.cpu.memory.words.17 : !acir.cell<i32>
      %23 = acir.ref @top.cpu.memory::@top.cpu.memory.words.18 : !acir.cell<i32>
      %24 = acir.ref @top.cpu.memory::@top.cpu.memory.words.19 : !acir.cell<i32>
      %25 = acir.ref @top.cpu.memory::@top.cpu.memory.words.20 : !acir.cell<i32>
      %26 = acir.ref @top.cpu.memory::@top.cpu.memory.words.21 : !acir.cell<i32>
      %27 = acir.ref @top.cpu.memory::@top.cpu.memory.words.22 : !acir.cell<i32>
      %28 = acir.ref @top.cpu.memory::@top.cpu.memory.words.23 : !acir.cell<i32>
      %29 = acir.ref @top.cpu.memory::@top.cpu.memory.words.24 : !acir.cell<i32>
      %30 = acir.ref @top.cpu.memory::@top.cpu.memory.words.25 : !acir.cell<i32>
      %31 = acir.ref @top.cpu.memory::@top.cpu.memory.words.26 : !acir.cell<i32>
      %32 = acir.ref @top.cpu.memory::@top.cpu.memory.words.27 : !acir.cell<i32>
      %33 = acir.ref @top.cpu.memory::@top.cpu.memory.words.28 : !acir.cell<i32>
      %34 = acir.ref @top.cpu.memory::@top.cpu.memory.words.29 : !acir.cell<i32>
      %35 = acir.ref @top.cpu.memory::@top.cpu.memory.words.30 : !acir.cell<i32>
      %36 = acir.ref @top.cpu.memory::@top.cpu.memory.words.31 : !acir.cell<i32>
      %37 = acir.ref @top.cpu.memory::@top.cpu.memory.words.32 : !acir.cell<i32>
      %38 = acir.ref @top.cpu.memory::@top.cpu.memory.words.33 : !acir.cell<i32>
      %39 = acir.ref @top.cpu.memory::@top.cpu.memory.words.34 : !acir.cell<i32>
      %40 = acir.ref @top.cpu.memory::@top.cpu.memory.words.35 : !acir.cell<i32>
      %41 = acir.ref @top.cpu.memory::@top.cpu.memory.words.36 : !acir.cell<i32>
      %42 = acir.ref @top.cpu.memory::@top.cpu.memory.words.37 : !acir.cell<i32>
      %43 = acir.ref @top.cpu.memory::@top.cpu.memory.words.38 : !acir.cell<i32>
      %44 = acir.ref @top.cpu.memory::@top.cpu.memory.words.39 : !acir.cell<i32>
      %45 = acir.ref @top.cpu.memory::@top.cpu.memory.words.40 : !acir.cell<i32>
      %46 = acir.ref @top.cpu.memory::@top.cpu.memory.words.41 : !acir.cell<i32>
      %47 = acir.ref @top.cpu.memory::@top.cpu.memory.words.42 : !acir.cell<i32>
      %48 = acir.ref @top.cpu.memory::@top.cpu.memory.words.43 : !acir.cell<i32>
      %49 = acir.ref @top.cpu.memory::@top.cpu.memory.words.44 : !acir.cell<i32>
      %50 = acir.ref @top.cpu.memory::@top.cpu.memory.words.45 : !acir.cell<i32>
      %51 = acir.ref @top.cpu.memory::@top.cpu.memory.words.46 : !acir.cell<i32>
      %52 = acir.ref @top.cpu.memory::@top.cpu.memory.words.47 : !acir.cell<i32>
      %53 = acir.ref @top.cpu.memory::@top.cpu.memory.words.48 : !acir.cell<i32>
      %54 = acir.ref @top.cpu.memory::@top.cpu.memory.words.49 : !acir.cell<i32>
      %55 = acir.ref @top.cpu.memory::@top.cpu.memory.words.50 : !acir.cell<i32>
      %56 = acir.ref @top.cpu.memory::@top.cpu.memory.words.51 : !acir.cell<i32>
      %57 = acir.ref @top.cpu.memory::@top.cpu.memory.words.52 : !acir.cell<i32>
      %58 = acir.ref @top.cpu.memory::@top.cpu.memory.words.53 : !acir.cell<i32>
      %59 = acir.ref @top.cpu.memory::@top.cpu.memory.words.54 : !acir.cell<i32>
      %60 = acir.ref @top.cpu.memory::@top.cpu.memory.words.55 : !acir.cell<i32>
      %61 = acir.ref @top.cpu.memory::@top.cpu.memory.words.56 : !acir.cell<i32>
      %62 = acir.ref @top.cpu.memory::@top.cpu.memory.words.57 : !acir.cell<i32>
      %63 = acir.ref @top.cpu.memory::@top.cpu.memory.words.58 : !acir.cell<i32>
      %64 = acir.ref @top.cpu.memory::@top.cpu.memory.words.59 : !acir.cell<i32>
      %65 = acir.ref @top.cpu.memory::@top.cpu.memory.words.60 : !acir.cell<i32>
      %66 = acir.ref @top.cpu.memory::@top.cpu.memory.words.61 : !acir.cell<i32>
      %67 = acir.ref @top.cpu.memory::@top.cpu.memory.words.62 : !acir.cell<i32>
      %68 = acir.ref @top.cpu.memory::@top.cpu.memory.words.63 : !acir.cell<i32>
      %69 = acir.aggregate(%5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16, %17, %18, %19, %20, %21, %22, %23, %24, %25, %26, %27, %28, %29, %30, %31, %32, %33, %34, %35, %36, %37, %38, %39, %40, %41, %42, %43, %44, %45, %46, %47, %48, %49, %50, %51, %52, %53, %54, %55, %56, %57, %58, %59, %60, %61, %62, %63, %64, %65, %66, %67, %68) {array_name = "top.cpu.memory.words"} : (!acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>) -> !acir.array<!acir.cell<i32>, [64]> loc(#loc378)
      %70:2 = scf.if %4 -> (!acir.struct<"src_isa_e31afbf3.StoreEvent">, i32) {
        %82 = acir.extract %0 {field = "alu"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc379)
        %83 = arith.andi %82, %c3_i32 : i32 loc(#loc379)
        %84 = arith.cmpi eq, %83, %c0_i32 : i32 loc(#loc379)
        %85 = scf.if %84 -> (i1) {
          %92 = arith.cmpi uge, %82, %c4096_i32 : i32 loc(#loc380)
          scf.yield %92 : i1
        } else {
          scf.yield %false : i1
        }
        %86 = scf.if %85 -> (i1) {
          %92 = arith.subi %82, %c4096_i32 : i32 loc(#loc381)
          %93 = arith.cmpi ult, %92, %c256_i32 : i32 loc(#loc381)
          scf.yield %93 : i1
        } else {
          scf.yield %false : i1
        }
        acir.check %86 {acir.id = "v42", acir.source_type = "void"} loc(#loc382)
        %87 = arith.subi %82, %c4096_i32 : i32 loc(#loc383)
        %88 = func.call @ac_builtin_shr_u32(%87, %c2_i32) {acir.id = "v46", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc384)
        %89 = scf.if %3 -> (i32) {
          %92 = "acir.array.index"(%69, %88) {signed_index = false} : (!acir.array<!acir.cell<i32>, [64]>, i32) -> !acir.cell<i32> loc(#loc385)
          %93 = acir.cell.read %92 {acir.id = "v115", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc374)
          scf.yield %93 : i32
        } else {
          scf.yield %1 : i32
        } loc(#loc374)
        %90 = acir.extract %0 {field = "store"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc386)
        %91 = scf.if %90 -> (!acir.struct<"src_isa_e31afbf3.StoreEvent">) {
          %92 = acir.extract %0 {field = "store_value"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc387)
          %93 = "acir.array.index"(%69, %88) {signed_index = false} : (!acir.array<!acir.cell<i32>, [64]>, i32) -> !acir.cell<i32> loc(#loc388)
          acir.cell.write %93, %92 {acir.id = "v125", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc388)
          %94 = acir.aggregate(%true, %82, %92) : (i1, i32, i32) -> !acir.struct<"src_isa_e31afbf3.StoreEvent"> loc(#loc375)
          scf.yield %94 : !acir.struct<"src_isa_e31afbf3.StoreEvent">
        } else {
          scf.yield %2 : !acir.struct<"src_isa_e31afbf3.StoreEvent">
        } loc(#loc375)
        scf.yield %91, %89 : !acir.struct<"src_isa_e31afbf3.StoreEvent">, i32
      } else {
        scf.yield %2, %1 : !acir.struct<"src_isa_e31afbf3.StoreEvent">, i32
      }
      %71 = acir.bind %70#0 : (!acir.struct<"src_isa_e31afbf3.StoreEvent">) -> !acir.struct<"src_isa_e31afbf3.StoreEvent"> loc(#loc375)
      %72 = acir.bind %70#1 : (i32) -> i32 loc(#loc374)
      %73 = acir.ref @top.cpu.memory::@top.cpu.memory.store_event : !acir.cell<!acir.struct<"src_isa_e31afbf3.StoreEvent">>
      acir.cell.write %73, %71 {acir.id = "v144", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.StoreEvent">>, !acir.struct<"src_isa_e31afbf3.StoreEvent"> loc(#loc389)
      %74 = acir.extract %0 {field = "valid"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc390)
      %75 = acir.extract %0 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc391)
      %76 = acir.extract %0 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc392)
      %77 = acir.extract %0 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i32 loc(#loc393)
      %78 = acir.extract %0 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc394)
      %79 = acir.extract %0 {field = "stalled"} : (!acir.struct<"src_isa_e31afbf3.Executed">) -> i1 loc(#loc395)
      %80 = acir.aggregate(%74, %75, %76, %77, %78, %72, %79) : (i1, i32, i32, i32, i1, i32, i1) -> !acir.struct<"src_isa_e31afbf3.Completed"> loc(#loc396)
      %81 = "acir.optional"(%80, %true) : (!acir.struct<"src_isa_e31afbf3.Completed">, i1) -> !acir.struct<"src_isa_e31afbf3.Completed">
      acir.return %81 : !acir.struct<"src_isa_e31afbf3.Completed"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.cpu.memory_body", owner = "top.cpu.memory", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.ref @top.cpu::@top.cpu.ex_mem : !acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">>
      %1 = acir.rule.call @top.cpu.memory::@top.cpu.memory_body.0.call.20.2(%0) {acir.id = "v3", acir.source_type = "src_isa_e31afbf3.Completed", contract = "Module:top.cpu.memory", instance = "top.cpu.memory", interface = "Module:top.cpu.memory:advance", output_binding = @top.cpu::@top.cpu.mem_wb, role = "rule", rule = "advance"} : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Executed">>) -> !acir.struct<"src_isa_e31afbf3.Completed"> loc(#loc397)
      %2 = acir.ref @top.cpu::@top.cpu.mem_wb : !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>
      %false = arith.constant false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.cpu.writeback attributes {acir.instance = {definition = "src_model_bb8d42ed.Writeback", name = "top.cpu.writeback", parent = "top.cpu", rules = ["advance"]}} {
    acir.contract {relations = [], rules = ["advance"]}
    acir.cell @top.cpu.writeback.retired : i32 {initial_fn = "ac_init_top.cpu.writeback.retired", name = "top.cpu.writeback.retired", type = "u32"}
    acir.cell @top.cpu.writeback.retirement : !acir.struct<"src_isa_e31afbf3.Retirement"> {initial_fn = "ac_init_top.cpu.writeback.retirement", name = "top.cpu.writeback.retirement", type = "src_isa_e31afbf3.Retirement"}
    func.func private @ac_init_top.cpu.rf.registers.0() -> i32
    func.func private @ac_init_top.cpu.rf.registers.1() -> i32
    func.func private @ac_init_top.cpu.rf.registers.2() -> i32
    func.func private @ac_init_top.cpu.rf.registers.3() -> i32
    func.func private @ac_init_top.cpu.rf.registers.4() -> i32
    func.func private @ac_init_top.cpu.rf.registers.5() -> i32
    func.func private @ac_init_top.cpu.rf.registers.6() -> i32
    func.func private @ac_init_top.cpu.rf.registers.7() -> i32
    func.func private @ac_init_top.cpu.rf.registers.8() -> i32
    func.func private @ac_init_top.cpu.rf.registers.9() -> i32
    func.func private @ac_init_top.cpu.rf.registers.10() -> i32
    func.func private @ac_init_top.cpu.rf.registers.11() -> i32
    func.func private @ac_init_top.cpu.rf.registers.12() -> i32
    func.func private @ac_init_top.cpu.rf.registers.13() -> i32
    func.func private @ac_init_top.cpu.rf.registers.14() -> i32
    func.func private @ac_init_top.cpu.rf.registers.15() -> i32
    func.func private @ac_init_top.cpu.rf.registers.16() -> i32
    func.func private @ac_init_top.cpu.rf.registers.17() -> i32
    func.func private @ac_init_top.cpu.rf.registers.18() -> i32
    func.func private @ac_init_top.cpu.rf.registers.19() -> i32
    func.func private @ac_init_top.cpu.rf.registers.20() -> i32
    func.func private @ac_init_top.cpu.rf.registers.21() -> i32
    func.func private @ac_init_top.cpu.rf.registers.22() -> i32
    func.func private @ac_init_top.cpu.rf.registers.23() -> i32
    func.func private @ac_init_top.cpu.rf.registers.24() -> i32
    func.func private @ac_init_top.cpu.rf.registers.25() -> i32
    func.func private @ac_init_top.cpu.rf.registers.26() -> i32
    func.func private @ac_init_top.cpu.rf.registers.27() -> i32
    func.func private @ac_init_top.cpu.rf.registers.28() -> i32
    func.func private @ac_init_top.cpu.rf.registers.29() -> i32
    func.func private @ac_init_top.cpu.rf.registers.30() -> i32
    func.func private @ac_init_top.cpu.rf.registers.31() -> i32
    func.func private @ac_init_top.cpu.if_id() -> !acir.struct<"src_isa_e31afbf3.Fetched">
    func.func private @ac_init_top.cpu.id_ex() -> !acir.struct<"src_isa_e31afbf3.Decoded">
    func.func private @ac_init_top.cpu.ex_mem() -> !acir.struct<"src_isa_e31afbf3.Executed">
    func.func private @ac_init_top.cpu.mem_wb() -> !acir.struct<"src_isa_e31afbf3.Completed">
    func.func private @ac_init_top.cpu.fetch.rom() -> !acir.array<i32, [256]>
    func.func private @ac_init_top.cpu.fetch.pc() -> i32
    func.func private @ac_init_top.cpu.memory.words.0() -> i32
    func.func private @ac_init_top.cpu.memory.words.1() -> i32
    func.func private @ac_init_top.cpu.memory.words.2() -> i32
    func.func private @ac_init_top.cpu.memory.words.3() -> i32
    func.func private @ac_init_top.cpu.memory.words.4() -> i32
    func.func private @ac_init_top.cpu.memory.words.5() -> i32
    func.func private @ac_init_top.cpu.memory.words.6() -> i32
    func.func private @ac_init_top.cpu.memory.words.7() -> i32
    func.func private @ac_init_top.cpu.memory.words.8() -> i32
    func.func private @ac_init_top.cpu.memory.words.9() -> i32
    func.func private @ac_init_top.cpu.memory.words.10() -> i32
    func.func private @ac_init_top.cpu.memory.words.11() -> i32
    func.func private @ac_init_top.cpu.memory.words.12() -> i32
    func.func private @ac_init_top.cpu.memory.words.13() -> i32
    func.func private @ac_init_top.cpu.memory.words.14() -> i32
    func.func private @ac_init_top.cpu.memory.words.15() -> i32
    func.func private @ac_init_top.cpu.memory.words.16() -> i32
    func.func private @ac_init_top.cpu.memory.words.17() -> i32
    func.func private @ac_init_top.cpu.memory.words.18() -> i32
    func.func private @ac_init_top.cpu.memory.words.19() -> i32
    func.func private @ac_init_top.cpu.memory.words.20() -> i32
    func.func private @ac_init_top.cpu.memory.words.21() -> i32
    func.func private @ac_init_top.cpu.memory.words.22() -> i32
    func.func private @ac_init_top.cpu.memory.words.23() -> i32
    func.func private @ac_init_top.cpu.memory.words.24() -> i32
    func.func private @ac_init_top.cpu.memory.words.25() -> i32
    func.func private @ac_init_top.cpu.memory.words.26() -> i32
    func.func private @ac_init_top.cpu.memory.words.27() -> i32
    func.func private @ac_init_top.cpu.memory.words.28() -> i32
    func.func private @ac_init_top.cpu.memory.words.29() -> i32
    func.func private @ac_init_top.cpu.memory.words.30() -> i32
    func.func private @ac_init_top.cpu.memory.words.31() -> i32
    func.func private @ac_init_top.cpu.memory.words.32() -> i32
    func.func private @ac_init_top.cpu.memory.words.33() -> i32
    func.func private @ac_init_top.cpu.memory.words.34() -> i32
    func.func private @ac_init_top.cpu.memory.words.35() -> i32
    func.func private @ac_init_top.cpu.memory.words.36() -> i32
    func.func private @ac_init_top.cpu.memory.words.37() -> i32
    func.func private @ac_init_top.cpu.memory.words.38() -> i32
    func.func private @ac_init_top.cpu.memory.words.39() -> i32
    func.func private @ac_init_top.cpu.memory.words.40() -> i32
    func.func private @ac_init_top.cpu.memory.words.41() -> i32
    func.func private @ac_init_top.cpu.memory.words.42() -> i32
    func.func private @ac_init_top.cpu.memory.words.43() -> i32
    func.func private @ac_init_top.cpu.memory.words.44() -> i32
    func.func private @ac_init_top.cpu.memory.words.45() -> i32
    func.func private @ac_init_top.cpu.memory.words.46() -> i32
    func.func private @ac_init_top.cpu.memory.words.47() -> i32
    func.func private @ac_init_top.cpu.memory.words.48() -> i32
    func.func private @ac_init_top.cpu.memory.words.49() -> i32
    func.func private @ac_init_top.cpu.memory.words.50() -> i32
    func.func private @ac_init_top.cpu.memory.words.51() -> i32
    func.func private @ac_init_top.cpu.memory.words.52() -> i32
    func.func private @ac_init_top.cpu.memory.words.53() -> i32
    func.func private @ac_init_top.cpu.memory.words.54() -> i32
    func.func private @ac_init_top.cpu.memory.words.55() -> i32
    func.func private @ac_init_top.cpu.memory.words.56() -> i32
    func.func private @ac_init_top.cpu.memory.words.57() -> i32
    func.func private @ac_init_top.cpu.memory.words.58() -> i32
    func.func private @ac_init_top.cpu.memory.words.59() -> i32
    func.func private @ac_init_top.cpu.memory.words.60() -> i32
    func.func private @ac_init_top.cpu.memory.words.61() -> i32
    func.func private @ac_init_top.cpu.memory.words.62() -> i32
    func.func private @ac_init_top.cpu.memory.words.63() -> i32
    func.func private @ac_init_top.cpu.memory.store_event() -> !acir.struct<"src_isa_e31afbf3.StoreEvent">
    func.func private @ac_init_top.cpu.writeback.retired() -> i32
    func.func private @ac_init_top.cpu.writeback.retirement() -> !acir.struct<"src_isa_e31afbf3.Retirement">
    func.func private @ac_init_top.cpu.last_control() -> !acir.struct<"src_isa_e31afbf3.Control">
    func.func private @construct.0.call.0.0() -> !acir.array<i32, [256]>
    func.func private @construct.0.call.1.1() -> !acir.array<i32, [32]>
    func.func private @construct.0.call.2.2() -> !acir.array<i32, [64]>
    func.func private @top.cpu_body.0.call.5.9.0.call.5.16(i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.5.9.0.call.6.23(i32, i32, !acir.struct<"src_isa_e31afbf3.Executed">, !acir.struct<"src_isa_e31afbf3.Completed">) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.8.124(i32, i32) -> i1
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172(i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.8.16(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32, i32) -> i32
    func.func private @top.cpu_body.0.call.8.12.0.call.11.30(!acir.struct<"src_isa_e31afbf3.Instruction">, i32, i32) -> i1
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298(i32, i32) -> i32
    func.func private @top.cpu.decode_stage_body.0.call.14.2.0.call.14.6(i32) -> !acir.struct<"src_isa_e31afbf3.Instruction">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.cpu.writeback_body.0.call.21.3 : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>, i32) -> () {acir.function = {contract = "Module:top.cpu.writeback", definition = "advance", instance = "top.cpu.writeback", interface = "Module:top.cpu.writeback:advance", kind = "rule", name = "top.cpu.writeback_body.0.call.21.3", output_type = "void", owner = "top.cpu.writeback", params = [{id = "v1", name = "source", targets = ["top.cpu.mem_wb"], type = "cell<src_isa_e31afbf3.Completed>"}, {id = "v2", name = "code_size", targets = [], type = "u32"}], public = true, result = "void", rule = "advance"}} body {
    ^bb0(%arg0: !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">> loc("acir.binding:top.cpu.writeback_body.0.call.21.3:source"), %arg1: i32 loc("acir.binding:top.cpu.writeback_body.0.call.21.3:code_size")):
      %c1_i32 = arith.constant 1 : i32
      %c0_i32 = arith.constant 0 : i32
      %false = arith.constant false
      %true = arith.constant true
      %0 = acir.cell.read %arg0 {acir.id = "v6", acir.source_type = "src_isa_e31afbf3.Completed", role = "observe"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">> -> !acir.struct<"src_isa_e31afbf3.Completed"> loc(#loc400)
      %1 = acir.aggregate(%false, %c0_i32, %c0_i32, %c0_i32, %false, %c0_i32) : (i1, i32, i32, i32, i1, i32) -> !acir.struct<"src_isa_e31afbf3.Retirement"> loc(#loc401)
      %2 = acir.extract %0 {field = "writes"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i1 loc(#loc402)
      scf.if %2 {
        %8 = acir.extract %0 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc403)
        %9 = acir.extract %0 {field = "value"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc404)
        acir.rule.call @top.cpu.rf::@top.cpu.writeback_body.0.call.21.3.0.call.21.18(%8, %9) {acir.id = "v19", acir.source_type = "void", contract = "Module:top.cpu.rf", instance = "top.cpu.rf", interface = "Module:top.cpu.rf:write", role = "rule", rule = "write"} : (i32, i32) -> () loc(#loc405)
      }
      %3 = acir.extract %0 {field = "valid"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i1 loc(#loc406)
      %4 = scf.if %3 -> (i1) {
        %8 = acir.extract %0 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc407)
        %9 = arith.cmpi ult, %8, %arg1 : i32 loc(#loc407)
        scf.yield %9 : i1
      } else {
        scf.yield %false : i1
      }
      %5 = acir.ref @top.cpu.writeback::@top.cpu.writeback.retired : !acir.cell<i32>
      %6 = scf.if %4 -> (!acir.struct<"src_isa_e31afbf3.Retirement">) {
        %8 = acir.cell.read %5 {acir.id = "v31", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc408)
        %9 = arith.addi %8, %c1_i32 : i32 loc(#loc408)
        acir.cell.write %5, %9 {acir.id = "v35", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc409)
        %10 = acir.extract %0 {field = "pc"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc410)
        %11 = acir.extract %0 {field = "word"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc411)
        %12 = acir.extract %0 {field = "rd"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc412)
        %13 = scf.if %2 -> (i1) {
          %16 = arith.cmpi ne, %12, %c0_i32 : i32 loc(#loc413)
          scf.yield %16 : i1
        } else {
          scf.yield %false : i1
        }
        %14 = acir.extract %0 {field = "value"} : (!acir.struct<"src_isa_e31afbf3.Completed">) -> i32 loc(#loc414)
        %15 = acir.aggregate(%true, %10, %11, %12, %13, %14) : (i1, i32, i32, i32, i1, i32) -> !acir.struct<"src_isa_e31afbf3.Retirement"> loc(#loc401)
        scf.yield %15 : !acir.struct<"src_isa_e31afbf3.Retirement">
      } else {
        scf.yield %1 : !acir.struct<"src_isa_e31afbf3.Retirement">
      } loc(#loc401)
      %7 = acir.ref @top.cpu.writeback::@top.cpu.writeback.retirement : !acir.cell<!acir.struct<"src_isa_e31afbf3.Retirement">>
      acir.cell.write %7, %6 {acir.id = "v57", acir.source_type = "void"} : !acir.cell<!acir.struct<"src_isa_e31afbf3.Retirement">>, !acir.struct<"src_isa_e31afbf3.Retirement"> loc(#loc415)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.cpu.writeback_body", owner = "top.cpu.writeback", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.ref @top.cpu::@top.cpu.mem_wb : !acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>
      %1 = acir.external "top.cpu_body" "v33" {acir.source_type = "u32"} : i32
      acir.rule.call @top.cpu.writeback::@top.cpu.writeback_body.0.call.21.3(%0, %1) {acir.id = "v4", acir.source_type = "void", contract = "Module:top.cpu.writeback", instance = "top.cpu.writeback", interface = "Module:top.cpu.writeback:advance", role = "rule", rule = "advance"} : (!acir.cell<!acir.struct<"src_isa_e31afbf3.Completed">>, i32) -> () loc(#loc416)
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("-":16:45)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":47:39)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":16:37)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":199:42)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":200:42)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":201:44)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":202:45)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":47:22)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":37:38)
#loc10 = loc("-":149:45)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":47:57)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":149:37)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":151:51)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":176:43)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":177:50)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":210:49)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":32:11)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":30:11)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":30:17)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":29:11)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":29:17)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":28:11)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":28:17)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":27:11)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":26:11)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":26:17)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":25:11)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":25:17)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":24:11)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":24:17)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":23:11)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":23:17)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":22:11)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":22:17)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":21:11)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":20:11)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":20:16)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":19:11)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":19:16)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":18:11)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":18:16)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":17:11)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":16:11)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":16:16)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":15:11)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":15:16)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":14:11)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":14:16)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":13:11)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":13:16)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":12:16)
#loc52 = loc("acir.binding:construct.0.call.0.0:words")
#loc53 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":37:12)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":41:12)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":210:8)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":211:12)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":211:30)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":213:12)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":213:33)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":219:18)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":219:8)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":220:16)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":221:8)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":222:16)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":152:13)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":152:34)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":152:12)
#loc79 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16.0.call.9.172:result")
#loc80 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":157:8)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":157:25)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":157:24)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":158:43)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":158:28)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":164:18)
#loc90 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:opcode")
#loc91 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":164:8)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":165:16)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":166:8)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":166:24)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":166:41)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":167:21)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":167:16)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":168:8)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":168:23)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":168:39)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":169:22)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":169:16)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":170:8)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":170:24)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":171:44)
#loc106 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:operand")
#loc107 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":171:19)
#loc108 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.8.16:function")
#loc109 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":173:12)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":174:54)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":174:20)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":174:84)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":175:12)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":176:28)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":176:20)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":177:12)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":178:25)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":179:12)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":180:25)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":181:12)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":182:20)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":183:12)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":184:59)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":184:42)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":184:20)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":184:97)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":184:89)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":185:12)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":186:20)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":187:16)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":192:30)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":192:8)
#loc136 = loc("acir.binding:top.cpu_body.0.call.8.12.0.call.11.30:function")
#loc137 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":195:8)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":196:16)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":197:8)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":198:16)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":199:8)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":200:16)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":201:8)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":202:20)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":202:16)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":203:8)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":204:16)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":205:12)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":205:5)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":206:12)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":104:18)
#loc154 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6.0.call.14.298:sign")
#loc155 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":105:13)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":105:12)
#loc158 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6:instruction")
#loc159 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":110:8)
#loc160 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6:opcode")
#loc161 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":112:9)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":113:30)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":113:9)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":114:30)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":114:9)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":115:26)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":115:9)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":116:27)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":116:9)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":117:12)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":117:28)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":117:44)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":118:31)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":118:13)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:46)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:62)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:77)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:93)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:109)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:126)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":119:9)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":120:28)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":120:9)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":121:29)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":121:9)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":122:12)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":122:28)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":123:13)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":124:37)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":124:13)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":125:14)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":126:13)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:26)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:48)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:47)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:25)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:77)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:76)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:104)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":127:103)
#loc201 = loc("acir.binding:top.cpu.decode_stage_body.0.call.14.2.0.call.14.6:immediate")
#loc202 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":128:37)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":128:13)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":129:14)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:26)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:48)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:47)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:25)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:74)
#loc210 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:73)
#loc211 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:101)
#loc212 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":130:100)
#loc213 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":131:37)
#loc214 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":131:13)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":133:50)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":133:70)
#loc217 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":133:49)
#loc218 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":133:37)
#loc219 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":133:13)
#loc220 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":134:14)
#loc221 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":134:45)
#loc222 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":135:49)
#loc223 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":135:37)
#loc224 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":135:13)
#loc225 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":137:13)
#loc226 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":138:12)
#loc227 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":138:32)
#loc228 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":139:20)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":139:13)
#loc230 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":141:20)
#loc231 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":141:48)
#loc232 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":141:78)
#loc233 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":141:105)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":141:13)
#loc235 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":142:12)
#loc236 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":143:20)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":143:13)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":144:12)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":144:29)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":145:20)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":145:13)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":146:29)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":147:20)
#loc244 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":147:47)
#loc245 = loc("/home/lc/tmp/pycircuit/examples/ripes5/isa.py":147:13)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":50:9)
#loc248 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":214:9)
#loc249 = loc("acir.binding:top.cpu_body:instruction")
#loc250 = loc("acir.binding:top.cpu_body:stall")
#loc251 = loc("acir.binding:top.cpu_body:operands")
#loc252 = loc("acir.binding:top.cpu_body:result")
#loc253 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":222:34)
#loc254 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":222:51)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":222:66)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":222:85)
#loc257 = loc("acir.binding:top.cpu_body:control")
#loc258 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":225:17)
#loc259 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":226:17)
#loc260 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":227:26)
#loc261 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":227:18)
#loc262 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":228:18)
#loc263 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":47:96)
#loc264 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":229:9)
#loc265 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":230:9)
#loc268 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":21:12)
#loc269 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":15:61)
#loc270 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":23:12)
#loc271 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":23:33)
#loc272 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":24:20)
#loc273 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":25:16)
#loc276 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":29:21)
#loc277 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":29:12)
#loc278 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":30:13)
#loc280 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":41:12)
#loc281 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":43:12)
#loc282 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":44:18)
#loc283 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":44:13)
#loc284 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":45:20)
#loc285 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":46:16)
#loc286 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":46:9)
#loc287 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":47:32)
#loc288 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":47:36)
#loc289 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":47:40)
#loc290 = loc("acir.binding:top.cpu.fetch_body.0.call.13.2:packet")
#loc291 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":48:14)
#loc292 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":48:9)
#loc293 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":52:16)
#loc294 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":66:16)
#loc296 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":62:30)
#loc297 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":62:23)
#loc298 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":62:9)
#loc302 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":71:12)
#loc303 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":71:29)
#loc304 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":72:22)
#loc305 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:bubble")
#loc306 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":73:13)
#loc307 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:packet")
#loc308 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:completed")
#loc309 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:decoded")
#loc310 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":78:23)
#loc311 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:lhs")
#loc312 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":79:23)
#loc313 = loc("acir.binding:top.cpu.decode_stage_body.0.call.17.6:rhs")
#loc314 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":80:24)
#loc315 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":80:38)
#loc316 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":80:49)
#loc317 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":80:16)
#loc318 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":84:9)
#loc319 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":85:16)
#loc322 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":95:21)
#loc323 = loc("acir.binding:top.cpu_body.0.call.4.4:executing")
#loc324 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":96:16)
#loc325 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":96:35)
#loc326 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":97:13)
#loc327 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":97:29)
#loc328 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":97:64)
#loc329 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":97:48)
#loc333 = loc("acir.binding:top.cpu_body.0.call.5.9:packet")
#loc334 = loc("acir.binding:top.cpu_body.0.call.5.9:memory")
#loc335 = loc("acir.binding:top.cpu_body.0.call.5.9:writeback")
#loc336 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":111:31)
#loc337 = loc("acir.binding:top.cpu_body.0.call.5.9:source_a")
#loc338 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":112:31)
#loc339 = loc("acir.binding:top.cpu_body.0.call.5.9:source_b")
#loc340 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":113:25)
#loc341 = loc("acir.binding:top.cpu_body.0.call.5.9:lhs")
#loc342 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":114:25)
#loc343 = loc("acir.binding:top.cpu_body.0.call.5.9:rhs")
#loc344 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":115:16)
#loc347 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":130:36)
#loc348 = loc("acir.binding:top.cpu_body.0.call.8.12:packet")
#loc349 = loc("acir.binding:top.cpu_body.0.call.8.12:executing")
#loc350 = loc("acir.binding:top.cpu_body.0.call.8.12:lhs")
#loc351 = loc("acir.binding:top.cpu_body.0.call.8.12:rhs")
#loc352 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":129:33)
#loc353 = loc("acir.binding:top.cpu_body.0.call.8.12:result")
#loc354 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":130:16)
#loc355 = loc("acir.binding:top.cpu_body.0.call.8.12:jump")
#loc356 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":130:43)
#loc357 = loc("acir.binding:top.cpu_body.0.call.8.12:value")
#loc358 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":132:17)
#loc359 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":131:28)
#loc360 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":133:32)
#loc361 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":133:57)
#loc362 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":133:70)
#loc363 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":133:84)
#loc364 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":134:32)
#loc365 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":134:48)
#loc366 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":134:85)
#loc367 = loc("acir.binding:top.cpu_body.0.call.8.12:next_packet")
#loc368 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":135:16)
#loc370 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":142:16)
#loc372 = loc("/home/lc/tmp/pycircuit/examples/ripes5/demo.py":47:80)
#loc373 = loc("acir.binding:top.cpu.memory_body.0.call.20.2:packet")
#loc374 = loc("acir.binding:top.cpu.memory_body.0.call.20.2:value")
#loc375 = loc("acir.binding:top.cpu.memory_body.0.call.20.2:event")
#loc376 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":158:12)
#loc377 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":158:27)
#loc378 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":148:57)
#loc379 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":159:20)
#loc380 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":159:44)
#loc381 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":159:67)
#loc382 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":159:13)
#loc383 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":160:22)
#loc384 = loc("acir.binding:top.cpu.memory_body.0.call.20.2:index")
#loc385 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":162:25)
#loc386 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":163:16)
#loc387 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":164:32)
#loc388 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":164:17)
#loc389 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":166:9)
#loc390 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:26)
#loc391 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:40)
#loc392 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:51)
#loc393 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:64)
#loc394 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:75)
#loc395 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:97)
#loc396 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":167:16)
#loc397 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":170:16)
#loc400 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3:packet")
#loc401 = loc("acir.binding:top.cpu.writeback_body.0.call.21.3:event")
#loc402 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":183:12)
#loc403 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":184:22)
#loc404 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":184:33)
#loc405 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":184:13)
#loc406 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":185:12)
#loc407 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":185:29)
#loc408 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":186:23)
#loc409 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":186:13)
#loc410 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":187:38)
#loc411 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":187:49)
#loc412 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":187:62)
#loc413 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":187:91)
#loc414 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":187:107)
#loc415 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":188:9)
#loc416 = loc("/home/lc/tmp/pycircuit/examples/ripes5/model.py":191:9)

#loc14 = loc("acir.binding:top.decode_stage_body.0.call.1.3.0.call.1.49:value")
#loc17 = loc("acir.binding:top.decode_stage_body.0.call.1.3:packet")
#loc63 = loc("acir.binding:top.decode_stage_body.0.call.3.8:inst")
#loc74 = loc("acir.binding:top.decode_stage_body.0.call.4.15.0.call.4.10:index")
#loc78 = loc("acir.binding:top.decode_stage_body.0.call.4.15.0.call.5.21:inst")
#loc83 = loc("acir.binding:top.writeback_body.0.call.11.2.0.call.11.6:inst")
#loc90 = loc("acir.binding:top.memory_stage_body.0.call.8.2.0.call.8.14:address")
#loc99 = loc("acir.binding:top.memory_stage_body.0.call.8.2.0.call.9.21:address")
#loc100 = loc("acir.binding:top.memory_stage_body.0.call.8.2.0.call.9.21:value")
#loc109 = loc("acir.binding:top.fetch_body.0.call.0.6:current_pc")
#loc122 = loc("acir.binding:top.decode_stage_body.0.call.4.15:message")
#loc123 = loc("acir.binding:top.decode_stage_body.0.call.4.15:decoded")
#loc145 = loc("acir.binding:top.execute_body.0.call.7.2:message")
#loc161 = loc("acir.binding:top.memory_stage_body.0.call.8.2:message")
#loc175 = loc("acir.binding:top.writeback_body.0.call.11.2:message")
module attributes {acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, format_version = 9 : i64, top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "immediate", payload_type = i32, type = "u32"}, {default = false, name = "immediate_rhs", payload_type = i1, type = "bool"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "lhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}]}}} {
  func.func @ac_init_top.rf.registers.0() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.0", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.1() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.1", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.2() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.2", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.3() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.3", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.4() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.4", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.5() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.5", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.6() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.6", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.7() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.7", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.8() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.8", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.9() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.9", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.10() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.10", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.11() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.11", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.12() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.12", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.13() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.13", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.14() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.14", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.15() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.15", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.16() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.16", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.17() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.17", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.18() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.18", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.19() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.19", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.20() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.20", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.21() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.21", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.22() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.22", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.23() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.23", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.24() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.24", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.25() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.25", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.26() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.26", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.27() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.27", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.28() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.28", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.29() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.29", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.30() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.30", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.registers.31() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers.31", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc1)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.rf.pending.0() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.0", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.1() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.1", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.2() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.2", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.3() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.3", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.4() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.4", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.5() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.5", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.6() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.6", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.7() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.7", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.8() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.8", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.9() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.9", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.10() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.10", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.11() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.11", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.12() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.12", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.13() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.13", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.14() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.14", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.15() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.15", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.16() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.16", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.17() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.17", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.18() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.18", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.19() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.19", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.20() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.20", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.21() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.21", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.22() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.22", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.23() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.23", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.24() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.24", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.25() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.25", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.26() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.26", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.27() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.27", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.28() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.28", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.29() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.29", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.30() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.30", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.rf.pending.31() -> i1 attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending.31", params = [], result = "bool"}} {
    %false = arith.constant false loc(#loc2)
    return %false : i1
  }
  func.func @ac_init_top.memory.words() -> !acir.array<i32, [16]> attributes {acir.function = {kind = "helper", name = "ac_init_top.memory.words", params = [], result = "array<u32;16>"}} {
    %c0_i32 = arith.constant 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [16]> loc(#loc3)
    return %0 : !acir.array<i32, [16]>
  }
  func.func @ac_init_top.if_id_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.if_id_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.if_id() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.if_id", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.id_ex_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.id_ex_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.id_ex() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.id_ex", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.ex_mem_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.ex_mem_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.ex_mem() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.ex_mem", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.mem_wb_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.mem_wb_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.mem_wb() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.mem_wb", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.fetch.rom() -> !acir.array<i32, [6]> attributes {acir.function = {kind = "helper", name = "ac_init_top.fetch.rom", params = [], result = "array<u32;6>"}} {
    %c7340179_i32 = arith.constant 7340179 : i32 loc(#loc4)
    %c5243155_i32 = arith.constant 5243155 : i32 loc(#loc5)
    %c2130355_i32 = arith.constant 2130355 : i32 loc(#loc6)
    %c3153955_i32 = arith.constant 3153955 : i32 loc(#loc7)
    %c8707_i32 = arith.constant 8707 : i32 loc(#loc8)
    %c1075970739_i32 = arith.constant 1075970739 : i32 loc(#loc9)
    %0 = acir.aggregate(%c7340179_i32, %c5243155_i32, %c2130355_i32, %c3153955_i32, %c8707_i32, %c1075970739_i32) : (i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [6]> loc(#loc10)
    return %0 : !acir.array<i32, [6]>
  }
  func.func @ac_init_top.fetch.pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.fetch.pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc11)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.writeback.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.writeback.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc12)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.writeback.last_pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.writeback.last_pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant 0 : i32 loc(#loc13)
    return %c0_i32 : i32
  }
  func.func @top.decode_stage_body.0.call.1.3.0.call.1.49(%arg0: i32 loc("acir.binding:top.decode_stage_body.0.call.1.3.0.call.1.49:value")) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "top.decode_stage_body.0.call.1.3.0.call.1.49", params = [{id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 : i32 loc(#loc15)
    %1 = arith.subi %0, %c2048_i32 : i32 loc(#loc16)
    return %1 : i32
  }
  func.func @top.decode_stage_body.0.call.1.3(%arg0: !acir.struct<"src_isa_f0424bc8.Fetched"> loc("acir.binding:top.decode_stage_body.0.call.1.3:packet")) -> !acir.struct<"src_isa_f0424bc8.Decoded"> attributes {acir.function = {definition = "src_isa_f0424bc8.decode", kind = "helper", name = "top.decode_stage_body.0.call.1.3", params = [{id = "v1", name = "packet", targets = [], type = "src_isa_f0424bc8.Fetched"}], result = "src_isa_f0424bc8.Decoded"}} {
    %c5_i32 = arith.constant 5 : i32
    %c35_i32 = arith.constant 35 : i32
    %c2_i32 = arith.constant 2 : i32
    %c3_i32 = arith.constant 3 : i32
    %c32_i32 = arith.constant 32 : i32
    %c51_i32 = arith.constant 51 : i32
    %c19_i32 = arith.constant 19 : i32 loc(#loc18)
    %c0_i32 = arith.constant 0 : i32
    %c20_i32 = arith.constant 20 : i32
    %c15_i32 = arith.constant 15 : i32 loc(#loc19)
    %c31_i32 = arith.constant 31 : i32
    %c25_i32 = arith.constant 25 : i32
    %c7_i32 = arith.constant 7 : i32
    %c12_i32 = arith.constant 12 : i32 loc(#loc20)
    %c127_i32 = arith.constant 127 : i32 loc(#loc21)
    %false = arith.constant false
    %true = arith.constant true
    %0 = acir.extract %arg0 {field = "word"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc22)
    %1 = arith.andi %0, %c127_i32 : i32 loc(#loc23)
    %2 = call @ac_builtin_shr_u32(%0, %c12_i32) {acir.id = "v11", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc24)
    %3 = arith.andi %2, %c7_i32 : i32 loc(#loc25)
    %4 = call @ac_builtin_shr_u32(%0, %c25_i32) {acir.id = "v16", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc26)
    %5 = call @ac_builtin_shr_u32(%0, %c7_i32) {acir.id = "v19", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc27)
    %6 = arith.andi %5, %c31_i32 : i32 loc(#loc28)
    %7 = call @ac_builtin_shr_u32(%0, %c15_i32) {acir.id = "v24", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc29)
    %8 = arith.andi %7, %c31_i32 : i32 loc(#loc30)
    %9 = call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v29", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc31)
    %10 = arith.andi %9, %c31_i32 : i32 loc(#loc32)
    %11 = arith.cmpi eq, %1, %c19_i32 : i32 loc(#loc33)
    %12:6 = scf.if %11 -> (i32, i1, i32, i32, i32, i1) {
      %21 = arith.cmpi eq, %3, %c0_i32 : i32 loc(#loc34)
      acir.check %21 {acir.id = "v45", acir.source_type = "void"} loc(#loc35)
      %22 = func.call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v49", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc36)
      %23 = func.call @top.decode_stage_body.0.call.1.3.0.call.1.49(%22) {acir.id = "v50", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc37)
      scf.yield %23, %true, %c0_i32, %6, %c0_i32, %true : i32, i1, i32, i32, i32, i1
    } else {
      %21 = arith.cmpi eq, %1, %c51_i32 : i32 loc(#loc38)
      %22:6 = scf.if %21 -> (i32, i1, i32, i32, i32, i1) {
        %29 = arith.cmpi eq, %3, %c0_i32 : i32 loc(#loc39)
        %30 = scf.if %29 -> (i1) {
          %33 = arith.cmpi eq, %4, %c0_i32 : i32 loc(#loc40)
          %34 = scf.if %33 -> (i1) {
            scf.yield %true : i1
          } else {
            %35 = arith.cmpi eq, %4, %c32_i32 : i32 loc(#loc41)
            scf.yield %35 : i1
          }
          scf.yield %34 : i1
        } else {
          scf.yield %false : i1
        }
        acir.check %30 {acir.id = "v70", acir.source_type = "void"} loc(#loc42)
        %31 = arith.cmpi eq, %4, %c32_i32 : i32 loc(#loc43)
        %32 = arith.extui %31 : i1 to i32 loc(#loc44)
        scf.yield %c0_i32, %false, %32, %6, %10, %true : i32, i1, i32, i32, i32, i1
      } else {
        %29 = arith.cmpi eq, %1, %c3_i32 : i32 loc(#loc45)
        %30 = arith.select %29, %c2_i32, %c3_i32 : i32
        %31 = arith.select %29, %6, %c0_i32 : i32
        %32 = arith.select %29, %c0_i32, %10 : i32
        %33 = scf.if %29 -> (i32) {
          %40 = arith.cmpi eq, %3, %c2_i32 : i32 loc(#loc46)
          acir.check %40 {acir.id = "v84", acir.source_type = "void"} loc(#loc47)
          %41 = func.call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v90", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc48)
          %42 = func.call @top.decode_stage_body.0.call.1.3.0.call.1.49(%41) {acir.id = "v91", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc37)
          scf.yield %42 : i32
        } else {
          %40 = arith.cmpi eq, %1, %c35_i32 : i32 loc(#loc49)
          %41 = scf.if %40 -> (i1) {
            %48 = arith.cmpi eq, %3, %c2_i32 : i32 loc(#loc50)
            scf.yield %48 : i1
          } else {
            scf.yield %false : i1
          }
          acir.check %41 {acir.id = "v103", acir.source_type = "void"} loc(#loc51)
          %42 = func.call @ac_builtin_shr_u32(%0, %c25_i32) {acir.id = "v109", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc52)
          %43 = func.call @ac_builtin_shl_u32(%42, %c5_i32) {acir.id = "v111", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc52)
          %44 = func.call @ac_builtin_shr_u32(%0, %c7_i32) {acir.id = "v113", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc53)
          %45 = arith.andi %44, %c31_i32 : i32 loc(#loc53)
          %46 = arith.ori %43, %45 : i32 loc(#loc52)
          %47 = func.call @top.decode_stage_body.0.call.1.3.0.call.1.49(%46) {acir.id = "v117", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc37)
          scf.yield %47 : i32
        }
        %34 = acir.bind %33 : (i32) -> i32 loc(#loc37)
        %35 = acir.bind %true : (i1) -> i1 loc(#loc54)
        %36 = acir.bind %30 : (i32) -> i32 loc(#loc44)
        %37 = acir.bind %31 : (i32) -> i32 loc(#loc28)
        %38 = acir.bind %32 : (i32) -> i32 loc(#loc32)
        %39 = acir.bind %29 : (i1) -> i1 loc(#loc55)
        scf.yield %34, %35, %36, %37, %38, %39 : i32, i1, i32, i32, i32, i1
      }
      %23 = acir.bind %22#0 : (i32) -> i32 loc(#loc37)
      %24 = acir.bind %22#1 : (i1) -> i1 loc(#loc54)
      %25 = acir.bind %22#2 : (i32) -> i32 loc(#loc44)
      %26 = acir.bind %22#3 : (i32) -> i32 loc(#loc28)
      %27 = acir.bind %22#4 : (i32) -> i32 loc(#loc32)
      %28 = acir.bind %22#5 : (i1) -> i1 loc(#loc55)
      scf.yield %23, %24, %25, %26, %27, %28 : i32, i1, i32, i32, i32, i1
    }
    %13 = acir.bind %12#0 : (i32) -> i32 loc(#loc37)
    %14 = acir.bind %12#1 : (i1) -> i1 loc(#loc54)
    %15 = acir.bind %12#2 : (i32) -> i32 loc(#loc44)
    %16 = acir.bind %12#3 : (i32) -> i32 loc(#loc28)
    %17 = acir.bind %12#4 : (i32) -> i32 loc(#loc32)
    %18 = acir.bind %12#5 : (i1) -> i1 loc(#loc55)
    %19 = acir.extract %arg0 {field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc56)
    %20 = acir.aggregate(%19, %0, %15, %16, %8, %17, %13, %14, %18) : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc57)
    return %20 : !acir.struct<"src_isa_f0424bc8.Decoded">
  }
  func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  acir.module @top attributes {acir.instance = {definition = "src_model_8294833e.Pipeline", name = "top", parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.if_id : !acir.struct<"src_isa_f0424bc8.Fetched"> {capacity_fn = "ac_init_top.if_id_cap", initial_fn = "ac_init_top.if_id", name = "top.if_id", type = "src_isa_f0424bc8.Fetched"}
    acir.queue @top.id_ex : !acir.struct<"src_isa_f0424bc8.Operands"> {capacity_fn = "ac_init_top.id_ex_cap", initial_fn = "ac_init_top.id_ex", name = "top.id_ex", type = "src_isa_f0424bc8.Operands"}
    acir.queue @top.ex_mem : !acir.struct<"src_isa_f0424bc8.Executed"> {capacity_fn = "ac_init_top.ex_mem_cap", initial_fn = "ac_init_top.ex_mem", name = "top.ex_mem", type = "src_isa_f0424bc8.Executed"}
    acir.queue @top.mem_wb : !acir.struct<"src_isa_f0424bc8.Retired"> {capacity_fn = "ac_init_top.mem_wb_cap", initial_fn = "ac_init_top.mem_wb", name = "top.mem_wb", type = "src_isa_f0424bc8.Retired"}
    func.func private @ac_init_top.rf.registers.0() -> i32
    func.func private @ac_init_top.rf.registers.1() -> i32
    func.func private @ac_init_top.rf.registers.2() -> i32
    func.func private @ac_init_top.rf.registers.3() -> i32
    func.func private @ac_init_top.rf.registers.4() -> i32
    func.func private @ac_init_top.rf.registers.5() -> i32
    func.func private @ac_init_top.rf.registers.6() -> i32
    func.func private @ac_init_top.rf.registers.7() -> i32
    func.func private @ac_init_top.rf.registers.8() -> i32
    func.func private @ac_init_top.rf.registers.9() -> i32
    func.func private @ac_init_top.rf.registers.10() -> i32
    func.func private @ac_init_top.rf.registers.11() -> i32
    func.func private @ac_init_top.rf.registers.12() -> i32
    func.func private @ac_init_top.rf.registers.13() -> i32
    func.func private @ac_init_top.rf.registers.14() -> i32
    func.func private @ac_init_top.rf.registers.15() -> i32
    func.func private @ac_init_top.rf.registers.16() -> i32
    func.func private @ac_init_top.rf.registers.17() -> i32
    func.func private @ac_init_top.rf.registers.18() -> i32
    func.func private @ac_init_top.rf.registers.19() -> i32
    func.func private @ac_init_top.rf.registers.20() -> i32
    func.func private @ac_init_top.rf.registers.21() -> i32
    func.func private @ac_init_top.rf.registers.22() -> i32
    func.func private @ac_init_top.rf.registers.23() -> i32
    func.func private @ac_init_top.rf.registers.24() -> i32
    func.func private @ac_init_top.rf.registers.25() -> i32
    func.func private @ac_init_top.rf.registers.26() -> i32
    func.func private @ac_init_top.rf.registers.27() -> i32
    func.func private @ac_init_top.rf.registers.28() -> i32
    func.func private @ac_init_top.rf.registers.29() -> i32
    func.func private @ac_init_top.rf.registers.30() -> i32
    func.func private @ac_init_top.rf.registers.31() -> i32
    func.func private @ac_init_top.rf.pending.0() -> i1
    func.func private @ac_init_top.rf.pending.1() -> i1
    func.func private @ac_init_top.rf.pending.2() -> i1
    func.func private @ac_init_top.rf.pending.3() -> i1
    func.func private @ac_init_top.rf.pending.4() -> i1
    func.func private @ac_init_top.rf.pending.5() -> i1
    func.func private @ac_init_top.rf.pending.6() -> i1
    func.func private @ac_init_top.rf.pending.7() -> i1
    func.func private @ac_init_top.rf.pending.8() -> i1
    func.func private @ac_init_top.rf.pending.9() -> i1
    func.func private @ac_init_top.rf.pending.10() -> i1
    func.func private @ac_init_top.rf.pending.11() -> i1
    func.func private @ac_init_top.rf.pending.12() -> i1
    func.func private @ac_init_top.rf.pending.13() -> i1
    func.func private @ac_init_top.rf.pending.14() -> i1
    func.func private @ac_init_top.rf.pending.15() -> i1
    func.func private @ac_init_top.rf.pending.16() -> i1
    func.func private @ac_init_top.rf.pending.17() -> i1
    func.func private @ac_init_top.rf.pending.18() -> i1
    func.func private @ac_init_top.rf.pending.19() -> i1
    func.func private @ac_init_top.rf.pending.20() -> i1
    func.func private @ac_init_top.rf.pending.21() -> i1
    func.func private @ac_init_top.rf.pending.22() -> i1
    func.func private @ac_init_top.rf.pending.23() -> i1
    func.func private @ac_init_top.rf.pending.24() -> i1
    func.func private @ac_init_top.rf.pending.25() -> i1
    func.func private @ac_init_top.rf.pending.26() -> i1
    func.func private @ac_init_top.rf.pending.27() -> i1
    func.func private @ac_init_top.rf.pending.28() -> i1
    func.func private @ac_init_top.rf.pending.29() -> i1
    func.func private @ac_init_top.rf.pending.30() -> i1
    func.func private @ac_init_top.rf.pending.31() -> i1
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, [16]>
    func.func private @ac_init_top.if_id_cap() -> i32
    func.func private @ac_init_top.if_id() -> tuple<>
    func.func private @ac_init_top.id_ex_cap() -> i32
    func.func private @ac_init_top.id_ex() -> tuple<>
    func.func private @ac_init_top.ex_mem_cap() -> i32
    func.func private @ac_init_top.ex_mem() -> tuple<>
    func.func private @ac_init_top.mem_wb_cap() -> i32
    func.func private @ac_init_top.mem_wb() -> tuple<>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, [6]>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.49(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant true
      acir.module.call @top.fetch loc(#loc58)
      %0 = acir.ref @top::@top.if_id : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
      %1 = acir.ref @top::@top.mem_wb : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      acir.module.call @top.decode_stage loc(#loc59)
      %2 = acir.ref @top::@top.id_ex : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
      acir.module.call @top.execute loc(#loc60)
      %3 = acir.ref @top::@top.ex_mem : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
      acir.module.call @top.memory_stage loc(#loc61)
      %4 = acir.ref @top::@top.mem_wb : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      acir.module.call @top.writeback loc(#loc62)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rf attributes {acir.instance = {definition = "src_model_8294833e.RegisterFile", name = "top.rf", parent = "top", rules = ["commit", "read", "ready", "reserve"]}} {
    acir.contract {relations = [], rules = ["commit", "read", "ready", "reserve"]}
    acir.cell @top.rf.registers.0 : i32 {initial_fn = "ac_init_top.rf.registers.0", name = "top.rf.registers.0", type = "u32"}
    acir.cell @top.rf.registers.1 : i32 {initial_fn = "ac_init_top.rf.registers.1", name = "top.rf.registers.1", type = "u32"}
    acir.cell @top.rf.registers.2 : i32 {initial_fn = "ac_init_top.rf.registers.2", name = "top.rf.registers.2", type = "u32"}
    acir.cell @top.rf.registers.3 : i32 {initial_fn = "ac_init_top.rf.registers.3", name = "top.rf.registers.3", type = "u32"}
    acir.cell @top.rf.registers.4 : i32 {initial_fn = "ac_init_top.rf.registers.4", name = "top.rf.registers.4", type = "u32"}
    acir.cell @top.rf.registers.5 : i32 {initial_fn = "ac_init_top.rf.registers.5", name = "top.rf.registers.5", type = "u32"}
    acir.cell @top.rf.registers.6 : i32 {initial_fn = "ac_init_top.rf.registers.6", name = "top.rf.registers.6", type = "u32"}
    acir.cell @top.rf.registers.7 : i32 {initial_fn = "ac_init_top.rf.registers.7", name = "top.rf.registers.7", type = "u32"}
    acir.cell @top.rf.registers.8 : i32 {initial_fn = "ac_init_top.rf.registers.8", name = "top.rf.registers.8", type = "u32"}
    acir.cell @top.rf.registers.9 : i32 {initial_fn = "ac_init_top.rf.registers.9", name = "top.rf.registers.9", type = "u32"}
    acir.cell @top.rf.registers.10 : i32 {initial_fn = "ac_init_top.rf.registers.10", name = "top.rf.registers.10", type = "u32"}
    acir.cell @top.rf.registers.11 : i32 {initial_fn = "ac_init_top.rf.registers.11", name = "top.rf.registers.11", type = "u32"}
    acir.cell @top.rf.registers.12 : i32 {initial_fn = "ac_init_top.rf.registers.12", name = "top.rf.registers.12", type = "u32"}
    acir.cell @top.rf.registers.13 : i32 {initial_fn = "ac_init_top.rf.registers.13", name = "top.rf.registers.13", type = "u32"}
    acir.cell @top.rf.registers.14 : i32 {initial_fn = "ac_init_top.rf.registers.14", name = "top.rf.registers.14", type = "u32"}
    acir.cell @top.rf.registers.15 : i32 {initial_fn = "ac_init_top.rf.registers.15", name = "top.rf.registers.15", type = "u32"}
    acir.cell @top.rf.registers.16 : i32 {initial_fn = "ac_init_top.rf.registers.16", name = "top.rf.registers.16", type = "u32"}
    acir.cell @top.rf.registers.17 : i32 {initial_fn = "ac_init_top.rf.registers.17", name = "top.rf.registers.17", type = "u32"}
    acir.cell @top.rf.registers.18 : i32 {initial_fn = "ac_init_top.rf.registers.18", name = "top.rf.registers.18", type = "u32"}
    acir.cell @top.rf.registers.19 : i32 {initial_fn = "ac_init_top.rf.registers.19", name = "top.rf.registers.19", type = "u32"}
    acir.cell @top.rf.registers.20 : i32 {initial_fn = "ac_init_top.rf.registers.20", name = "top.rf.registers.20", type = "u32"}
    acir.cell @top.rf.registers.21 : i32 {initial_fn = "ac_init_top.rf.registers.21", name = "top.rf.registers.21", type = "u32"}
    acir.cell @top.rf.registers.22 : i32 {initial_fn = "ac_init_top.rf.registers.22", name = "top.rf.registers.22", type = "u32"}
    acir.cell @top.rf.registers.23 : i32 {initial_fn = "ac_init_top.rf.registers.23", name = "top.rf.registers.23", type = "u32"}
    acir.cell @top.rf.registers.24 : i32 {initial_fn = "ac_init_top.rf.registers.24", name = "top.rf.registers.24", type = "u32"}
    acir.cell @top.rf.registers.25 : i32 {initial_fn = "ac_init_top.rf.registers.25", name = "top.rf.registers.25", type = "u32"}
    acir.cell @top.rf.registers.26 : i32 {initial_fn = "ac_init_top.rf.registers.26", name = "top.rf.registers.26", type = "u32"}
    acir.cell @top.rf.registers.27 : i32 {initial_fn = "ac_init_top.rf.registers.27", name = "top.rf.registers.27", type = "u32"}
    acir.cell @top.rf.registers.28 : i32 {initial_fn = "ac_init_top.rf.registers.28", name = "top.rf.registers.28", type = "u32"}
    acir.cell @top.rf.registers.29 : i32 {initial_fn = "ac_init_top.rf.registers.29", name = "top.rf.registers.29", type = "u32"}
    acir.cell @top.rf.registers.30 : i32 {initial_fn = "ac_init_top.rf.registers.30", name = "top.rf.registers.30", type = "u32"}
    acir.cell @top.rf.registers.31 : i32 {initial_fn = "ac_init_top.rf.registers.31", name = "top.rf.registers.31", type = "u32"}
    acir.cell @top.rf.pending.0 : i1 {initial_fn = "ac_init_top.rf.pending.0", name = "top.rf.pending.0", type = "bool"}
    acir.cell @top.rf.pending.1 : i1 {initial_fn = "ac_init_top.rf.pending.1", name = "top.rf.pending.1", type = "bool"}
    acir.cell @top.rf.pending.2 : i1 {initial_fn = "ac_init_top.rf.pending.2", name = "top.rf.pending.2", type = "bool"}
    acir.cell @top.rf.pending.3 : i1 {initial_fn = "ac_init_top.rf.pending.3", name = "top.rf.pending.3", type = "bool"}
    acir.cell @top.rf.pending.4 : i1 {initial_fn = "ac_init_top.rf.pending.4", name = "top.rf.pending.4", type = "bool"}
    acir.cell @top.rf.pending.5 : i1 {initial_fn = "ac_init_top.rf.pending.5", name = "top.rf.pending.5", type = "bool"}
    acir.cell @top.rf.pending.6 : i1 {initial_fn = "ac_init_top.rf.pending.6", name = "top.rf.pending.6", type = "bool"}
    acir.cell @top.rf.pending.7 : i1 {initial_fn = "ac_init_top.rf.pending.7", name = "top.rf.pending.7", type = "bool"}
    acir.cell @top.rf.pending.8 : i1 {initial_fn = "ac_init_top.rf.pending.8", name = "top.rf.pending.8", type = "bool"}
    acir.cell @top.rf.pending.9 : i1 {initial_fn = "ac_init_top.rf.pending.9", name = "top.rf.pending.9", type = "bool"}
    acir.cell @top.rf.pending.10 : i1 {initial_fn = "ac_init_top.rf.pending.10", name = "top.rf.pending.10", type = "bool"}
    acir.cell @top.rf.pending.11 : i1 {initial_fn = "ac_init_top.rf.pending.11", name = "top.rf.pending.11", type = "bool"}
    acir.cell @top.rf.pending.12 : i1 {initial_fn = "ac_init_top.rf.pending.12", name = "top.rf.pending.12", type = "bool"}
    acir.cell @top.rf.pending.13 : i1 {initial_fn = "ac_init_top.rf.pending.13", name = "top.rf.pending.13", type = "bool"}
    acir.cell @top.rf.pending.14 : i1 {initial_fn = "ac_init_top.rf.pending.14", name = "top.rf.pending.14", type = "bool"}
    acir.cell @top.rf.pending.15 : i1 {initial_fn = "ac_init_top.rf.pending.15", name = "top.rf.pending.15", type = "bool"}
    acir.cell @top.rf.pending.16 : i1 {initial_fn = "ac_init_top.rf.pending.16", name = "top.rf.pending.16", type = "bool"}
    acir.cell @top.rf.pending.17 : i1 {initial_fn = "ac_init_top.rf.pending.17", name = "top.rf.pending.17", type = "bool"}
    acir.cell @top.rf.pending.18 : i1 {initial_fn = "ac_init_top.rf.pending.18", name = "top.rf.pending.18", type = "bool"}
    acir.cell @top.rf.pending.19 : i1 {initial_fn = "ac_init_top.rf.pending.19", name = "top.rf.pending.19", type = "bool"}
    acir.cell @top.rf.pending.20 : i1 {initial_fn = "ac_init_top.rf.pending.20", name = "top.rf.pending.20", type = "bool"}
    acir.cell @top.rf.pending.21 : i1 {initial_fn = "ac_init_top.rf.pending.21", name = "top.rf.pending.21", type = "bool"}
    acir.cell @top.rf.pending.22 : i1 {initial_fn = "ac_init_top.rf.pending.22", name = "top.rf.pending.22", type = "bool"}
    acir.cell @top.rf.pending.23 : i1 {initial_fn = "ac_init_top.rf.pending.23", name = "top.rf.pending.23", type = "bool"}
    acir.cell @top.rf.pending.24 : i1 {initial_fn = "ac_init_top.rf.pending.24", name = "top.rf.pending.24", type = "bool"}
    acir.cell @top.rf.pending.25 : i1 {initial_fn = "ac_init_top.rf.pending.25", name = "top.rf.pending.25", type = "bool"}
    acir.cell @top.rf.pending.26 : i1 {initial_fn = "ac_init_top.rf.pending.26", name = "top.rf.pending.26", type = "bool"}
    acir.cell @top.rf.pending.27 : i1 {initial_fn = "ac_init_top.rf.pending.27", name = "top.rf.pending.27", type = "bool"}
    acir.cell @top.rf.pending.28 : i1 {initial_fn = "ac_init_top.rf.pending.28", name = "top.rf.pending.28", type = "bool"}
    acir.cell @top.rf.pending.29 : i1 {initial_fn = "ac_init_top.rf.pending.29", name = "top.rf.pending.29", type = "bool"}
    acir.cell @top.rf.pending.30 : i1 {initial_fn = "ac_init_top.rf.pending.30", name = "top.rf.pending.30", type = "bool"}
    acir.cell @top.rf.pending.31 : i1 {initial_fn = "ac_init_top.rf.pending.31", name = "top.rf.pending.31", type = "bool"}
    func.func private @ac_init_top.rf.registers.0() -> i32
    func.func private @ac_init_top.rf.registers.1() -> i32
    func.func private @ac_init_top.rf.registers.2() -> i32
    func.func private @ac_init_top.rf.registers.3() -> i32
    func.func private @ac_init_top.rf.registers.4() -> i32
    func.func private @ac_init_top.rf.registers.5() -> i32
    func.func private @ac_init_top.rf.registers.6() -> i32
    func.func private @ac_init_top.rf.registers.7() -> i32
    func.func private @ac_init_top.rf.registers.8() -> i32
    func.func private @ac_init_top.rf.registers.9() -> i32
    func.func private @ac_init_top.rf.registers.10() -> i32
    func.func private @ac_init_top.rf.registers.11() -> i32
    func.func private @ac_init_top.rf.registers.12() -> i32
    func.func private @ac_init_top.rf.registers.13() -> i32
    func.func private @ac_init_top.rf.registers.14() -> i32
    func.func private @ac_init_top.rf.registers.15() -> i32
    func.func private @ac_init_top.rf.registers.16() -> i32
    func.func private @ac_init_top.rf.registers.17() -> i32
    func.func private @ac_init_top.rf.registers.18() -> i32
    func.func private @ac_init_top.rf.registers.19() -> i32
    func.func private @ac_init_top.rf.registers.20() -> i32
    func.func private @ac_init_top.rf.registers.21() -> i32
    func.func private @ac_init_top.rf.registers.22() -> i32
    func.func private @ac_init_top.rf.registers.23() -> i32
    func.func private @ac_init_top.rf.registers.24() -> i32
    func.func private @ac_init_top.rf.registers.25() -> i32
    func.func private @ac_init_top.rf.registers.26() -> i32
    func.func private @ac_init_top.rf.registers.27() -> i32
    func.func private @ac_init_top.rf.registers.28() -> i32
    func.func private @ac_init_top.rf.registers.29() -> i32
    func.func private @ac_init_top.rf.registers.30() -> i32
    func.func private @ac_init_top.rf.registers.31() -> i32
    func.func private @ac_init_top.rf.pending.0() -> i1
    func.func private @ac_init_top.rf.pending.1() -> i1
    func.func private @ac_init_top.rf.pending.2() -> i1
    func.func private @ac_init_top.rf.pending.3() -> i1
    func.func private @ac_init_top.rf.pending.4() -> i1
    func.func private @ac_init_top.rf.pending.5() -> i1
    func.func private @ac_init_top.rf.pending.6() -> i1
    func.func private @ac_init_top.rf.pending.7() -> i1
    func.func private @ac_init_top.rf.pending.8() -> i1
    func.func private @ac_init_top.rf.pending.9() -> i1
    func.func private @ac_init_top.rf.pending.10() -> i1
    func.func private @ac_init_top.rf.pending.11() -> i1
    func.func private @ac_init_top.rf.pending.12() -> i1
    func.func private @ac_init_top.rf.pending.13() -> i1
    func.func private @ac_init_top.rf.pending.14() -> i1
    func.func private @ac_init_top.rf.pending.15() -> i1
    func.func private @ac_init_top.rf.pending.16() -> i1
    func.func private @ac_init_top.rf.pending.17() -> i1
    func.func private @ac_init_top.rf.pending.18() -> i1
    func.func private @ac_init_top.rf.pending.19() -> i1
    func.func private @ac_init_top.rf.pending.20() -> i1
    func.func private @ac_init_top.rf.pending.21() -> i1
    func.func private @ac_init_top.rf.pending.22() -> i1
    func.func private @ac_init_top.rf.pending.23() -> i1
    func.func private @ac_init_top.rf.pending.24() -> i1
    func.func private @ac_init_top.rf.pending.25() -> i1
    func.func private @ac_init_top.rf.pending.26() -> i1
    func.func private @ac_init_top.rf.pending.27() -> i1
    func.func private @ac_init_top.rf.pending.28() -> i1
    func.func private @ac_init_top.rf.pending.29() -> i1
    func.func private @ac_init_top.rf.pending.30() -> i1
    func.func private @ac_init_top.rf.pending.31() -> i1
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, [16]>
    func.func private @ac_init_top.if_id_cap() -> i32
    func.func private @ac_init_top.if_id() -> tuple<>
    func.func private @ac_init_top.id_ex_cap() -> i32
    func.func private @ac_init_top.id_ex() -> tuple<>
    func.func private @ac_init_top.ex_mem_cap() -> i32
    func.func private @ac_init_top.ex_mem() -> tuple<>
    func.func private @ac_init_top.mem_wb_cap() -> i32
    func.func private @ac_init_top.mem_wb() -> tuple<>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, [6]>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.49(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.decode_stage_body.0.call.3.8 : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 {acir.function = {contract = "Module:top.rf", definition = "ready", instance = "top.rf", interface = "Module:top.rf:ready", kind = "rule", name = "top.decode_stage_body.0.call.3.8", output_type = "bool", owner = "top.rf", params = [{id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], public = true, result = "bool", rule = "ready"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded"> loc("acir.binding:top.decode_stage_body.0.call.3.8:inst")):
      %false = arith.constant false
      %true = arith.constant true
      %0 = acir.ref @top.rf::@top.rf.pending.0 : !acir.cell<i1>
      %1 = acir.ref @top.rf::@top.rf.pending.1 : !acir.cell<i1>
      %2 = acir.ref @top.rf::@top.rf.pending.2 : !acir.cell<i1>
      %3 = acir.ref @top.rf::@top.rf.pending.3 : !acir.cell<i1>
      %4 = acir.ref @top.rf::@top.rf.pending.4 : !acir.cell<i1>
      %5 = acir.ref @top.rf::@top.rf.pending.5 : !acir.cell<i1>
      %6 = acir.ref @top.rf::@top.rf.pending.6 : !acir.cell<i1>
      %7 = acir.ref @top.rf::@top.rf.pending.7 : !acir.cell<i1>
      %8 = acir.ref @top.rf::@top.rf.pending.8 : !acir.cell<i1>
      %9 = acir.ref @top.rf::@top.rf.pending.9 : !acir.cell<i1>
      %10 = acir.ref @top.rf::@top.rf.pending.10 : !acir.cell<i1>
      %11 = acir.ref @top.rf::@top.rf.pending.11 : !acir.cell<i1>
      %12 = acir.ref @top.rf::@top.rf.pending.12 : !acir.cell<i1>
      %13 = acir.ref @top.rf::@top.rf.pending.13 : !acir.cell<i1>
      %14 = acir.ref @top.rf::@top.rf.pending.14 : !acir.cell<i1>
      %15 = acir.ref @top.rf::@top.rf.pending.15 : !acir.cell<i1>
      %16 = acir.ref @top.rf::@top.rf.pending.16 : !acir.cell<i1>
      %17 = acir.ref @top.rf::@top.rf.pending.17 : !acir.cell<i1>
      %18 = acir.ref @top.rf::@top.rf.pending.18 : !acir.cell<i1>
      %19 = acir.ref @top.rf::@top.rf.pending.19 : !acir.cell<i1>
      %20 = acir.ref @top.rf::@top.rf.pending.20 : !acir.cell<i1>
      %21 = acir.ref @top.rf::@top.rf.pending.21 : !acir.cell<i1>
      %22 = acir.ref @top.rf::@top.rf.pending.22 : !acir.cell<i1>
      %23 = acir.ref @top.rf::@top.rf.pending.23 : !acir.cell<i1>
      %24 = acir.ref @top.rf::@top.rf.pending.24 : !acir.cell<i1>
      %25 = acir.ref @top.rf::@top.rf.pending.25 : !acir.cell<i1>
      %26 = acir.ref @top.rf::@top.rf.pending.26 : !acir.cell<i1>
      %27 = acir.ref @top.rf::@top.rf.pending.27 : !acir.cell<i1>
      %28 = acir.ref @top.rf::@top.rf.pending.28 : !acir.cell<i1>
      %29 = acir.ref @top.rf::@top.rf.pending.29 : !acir.cell<i1>
      %30 = acir.ref @top.rf::@top.rf.pending.30 : !acir.cell<i1>
      %31 = acir.ref @top.rf::@top.rf.pending.31 : !acir.cell<i1>
      %32 = acir.aggregate(%0, %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16, %17, %18, %19, %20, %21, %22, %23, %24, %25, %26, %27, %28, %29, %30, %31) {array_name = "top.rf.pending"} : (!acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>) -> !acir.array<!acir.cell<i1>, [32]> loc(#loc64)
      %33 = acir.extract %arg0 {field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc65)
      %34 = "acir.array.index"(%32, %33) {signed_index = false} : (!acir.array<!acir.cell<i1>, [32]>, i32) -> !acir.cell<i1> loc(#loc66)
      %35 = acir.cell.read %34 {acir.id = "v42", acir.source_type = "bool", role = "observe"} : !acir.cell<i1> -> i1 loc(#loc66)
      %36 = scf.if %35 -> (i1) {
        scf.yield %false : i1
      } else {
        %38 = acir.extract %arg0 {field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc67)
        %39 = "acir.array.index"(%32, %38) {signed_index = false} : (!acir.array<!acir.cell<i1>, [32]>, i32) -> !acir.cell<i1> loc(#loc68)
        %40 = acir.cell.read %39 {acir.id = "v46", acir.source_type = "bool", role = "observe"} : !acir.cell<i1> -> i1 loc(#loc68)
        %41 = arith.xori %40, %true : i1 loc(#loc69)
        scf.yield %41 : i1
      }
      %37 = scf.if %36 -> (i1) {
        %38 = acir.extract %arg0 {field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc70)
        %39 = scf.if %38 -> (i1) {
          %40 = acir.extract %arg0 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc71)
          %41 = "acir.array.index"(%32, %40) {signed_index = false} : (!acir.array<!acir.cell<i1>, [32]>, i32) -> !acir.cell<i1> loc(#loc72)
          %42 = acir.cell.read %41 {acir.id = "v57", acir.source_type = "bool", role = "observe"} : !acir.cell<i1> -> i1 loc(#loc72)
          %43 = arith.xori %42, %true : i1 loc(#loc73)
          scf.yield %43 : i1
        } else {
          scf.yield %true : i1
        }
        scf.yield %39 : i1
      } else {
        scf.yield %false : i1
      }
      acir.return %37 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.decode_stage_body.0.call.4.15.0.call.4.10 : (i32) -> i32 {acir.function = {contract = "Module:top.rf", definition = "read", instance = "top.rf", interface = "Module:top.rf:read", kind = "rule", name = "top.decode_stage_body.0.call.4.15.0.call.4.10", output_type = "u32", owner = "top.rf", params = [{id = "v1", name = "index", targets = [], type = "u32"}], public = true, result = "u32", rule = "read"}} body {
    ^bb0(%arg0: i32 loc("acir.binding:top.decode_stage_body.0.call.4.15.0.call.4.10:index")):
      %c0_i32 = arith.constant 0 : i32
      %0 = acir.aggregate() : () -> i32
      %1 = arith.cmpi eq, %arg0, %c0_i32 : i32 loc(#loc75)
      %2 = arith.cmpi ne, %arg0, %c0_i32 : i32 loc(#loc75)
      %3 = arith.select %1, %c0_i32, %0 : i32
      %4 = acir.ref @top.rf::@top.rf.registers.0 : !acir.cell<i32>
      %5 = acir.ref @top.rf::@top.rf.registers.1 : !acir.cell<i32>
      %6 = acir.ref @top.rf::@top.rf.registers.2 : !acir.cell<i32>
      %7 = acir.ref @top.rf::@top.rf.registers.3 : !acir.cell<i32>
      %8 = acir.ref @top.rf::@top.rf.registers.4 : !acir.cell<i32>
      %9 = acir.ref @top.rf::@top.rf.registers.5 : !acir.cell<i32>
      %10 = acir.ref @top.rf::@top.rf.registers.6 : !acir.cell<i32>
      %11 = acir.ref @top.rf::@top.rf.registers.7 : !acir.cell<i32>
      %12 = acir.ref @top.rf::@top.rf.registers.8 : !acir.cell<i32>
      %13 = acir.ref @top.rf::@top.rf.registers.9 : !acir.cell<i32>
      %14 = acir.ref @top.rf::@top.rf.registers.10 : !acir.cell<i32>
      %15 = acir.ref @top.rf::@top.rf.registers.11 : !acir.cell<i32>
      %16 = acir.ref @top.rf::@top.rf.registers.12 : !acir.cell<i32>
      %17 = acir.ref @top.rf::@top.rf.registers.13 : !acir.cell<i32>
      %18 = acir.ref @top.rf::@top.rf.registers.14 : !acir.cell<i32>
      %19 = acir.ref @top.rf::@top.rf.registers.15 : !acir.cell<i32>
      %20 = acir.ref @top.rf::@top.rf.registers.16 : !acir.cell<i32>
      %21 = acir.ref @top.rf::@top.rf.registers.17 : !acir.cell<i32>
      %22 = acir.ref @top.rf::@top.rf.registers.18 : !acir.cell<i32>
      %23 = acir.ref @top.rf::@top.rf.registers.19 : !acir.cell<i32>
      %24 = acir.ref @top.rf::@top.rf.registers.20 : !acir.cell<i32>
      %25 = acir.ref @top.rf::@top.rf.registers.21 : !acir.cell<i32>
      %26 = acir.ref @top.rf::@top.rf.registers.22 : !acir.cell<i32>
      %27 = acir.ref @top.rf::@top.rf.registers.23 : !acir.cell<i32>
      %28 = acir.ref @top.rf::@top.rf.registers.24 : !acir.cell<i32>
      %29 = acir.ref @top.rf::@top.rf.registers.25 : !acir.cell<i32>
      %30 = acir.ref @top.rf::@top.rf.registers.26 : !acir.cell<i32>
      %31 = acir.ref @top.rf::@top.rf.registers.27 : !acir.cell<i32>
      %32 = acir.ref @top.rf::@top.rf.registers.28 : !acir.cell<i32>
      %33 = acir.ref @top.rf::@top.rf.registers.29 : !acir.cell<i32>
      %34 = acir.ref @top.rf::@top.rf.registers.30 : !acir.cell<i32>
      %35 = acir.ref @top.rf::@top.rf.registers.31 : !acir.cell<i32>
      %36 = acir.aggregate(%4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16, %17, %18, %19, %20, %21, %22, %23, %24, %25, %26, %27, %28, %29, %30, %31, %32, %33, %34, %35) {array_name = "top.rf.registers"} : (!acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>) -> !acir.array<!acir.cell<i32>, [32]> loc(#loc76)
      %37 = scf.if %2 -> (i32) {
        %38 = "acir.array.index"(%36, %arg0) {signed_index = false} : (!acir.array<!acir.cell<i32>, [32]>, i32) -> !acir.cell<i32> loc(#loc77)
        %39 = acir.cell.read %38 {acir.id = "v52", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc77)
        scf.yield %39 : i32
      } else {
        scf.yield %3 : i32
      }
      acir.return %37 : i32 {acir.source_type = "void"}
    }
    acir.rule @top.decode_stage_body.0.call.4.15.0.call.5.21 : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> () {acir.function = {contract = "Module:top.rf", definition = "reserve", instance = "top.rf", interface = "Module:top.rf:reserve", kind = "rule", name = "top.decode_stage_body.0.call.4.15.0.call.5.21", output_type = "void", owner = "top.rf", params = [{id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], public = true, result = "void", rule = "reserve"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded"> loc("acir.binding:top.decode_stage_body.0.call.4.15.0.call.5.21:inst")):
      %false = arith.constant false
      %c0_i32 = arith.constant 0 : i32
      %true = arith.constant true
      %0 = acir.extract %arg0 {field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc79)
      %1 = scf.if %0 -> (i1) {
        %35 = acir.extract %arg0 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc80)
        %36 = arith.cmpi ne, %35, %c0_i32 : i32 loc(#loc80)
        scf.yield %36 : i1
      } else {
        scf.yield %false : i1
      }
      %2 = acir.ref @top.rf::@top.rf.pending.0 : !acir.cell<i1>
      %3 = acir.ref @top.rf::@top.rf.pending.1 : !acir.cell<i1>
      %4 = acir.ref @top.rf::@top.rf.pending.2 : !acir.cell<i1>
      %5 = acir.ref @top.rf::@top.rf.pending.3 : !acir.cell<i1>
      %6 = acir.ref @top.rf::@top.rf.pending.4 : !acir.cell<i1>
      %7 = acir.ref @top.rf::@top.rf.pending.5 : !acir.cell<i1>
      %8 = acir.ref @top.rf::@top.rf.pending.6 : !acir.cell<i1>
      %9 = acir.ref @top.rf::@top.rf.pending.7 : !acir.cell<i1>
      %10 = acir.ref @top.rf::@top.rf.pending.8 : !acir.cell<i1>
      %11 = acir.ref @top.rf::@top.rf.pending.9 : !acir.cell<i1>
      %12 = acir.ref @top.rf::@top.rf.pending.10 : !acir.cell<i1>
      %13 = acir.ref @top.rf::@top.rf.pending.11 : !acir.cell<i1>
      %14 = acir.ref @top.rf::@top.rf.pending.12 : !acir.cell<i1>
      %15 = acir.ref @top.rf::@top.rf.pending.13 : !acir.cell<i1>
      %16 = acir.ref @top.rf::@top.rf.pending.14 : !acir.cell<i1>
      %17 = acir.ref @top.rf::@top.rf.pending.15 : !acir.cell<i1>
      %18 = acir.ref @top.rf::@top.rf.pending.16 : !acir.cell<i1>
      %19 = acir.ref @top.rf::@top.rf.pending.17 : !acir.cell<i1>
      %20 = acir.ref @top.rf::@top.rf.pending.18 : !acir.cell<i1>
      %21 = acir.ref @top.rf::@top.rf.pending.19 : !acir.cell<i1>
      %22 = acir.ref @top.rf::@top.rf.pending.20 : !acir.cell<i1>
      %23 = acir.ref @top.rf::@top.rf.pending.21 : !acir.cell<i1>
      %24 = acir.ref @top.rf::@top.rf.pending.22 : !acir.cell<i1>
      %25 = acir.ref @top.rf::@top.rf.pending.23 : !acir.cell<i1>
      %26 = acir.ref @top.rf::@top.rf.pending.24 : !acir.cell<i1>
      %27 = acir.ref @top.rf::@top.rf.pending.25 : !acir.cell<i1>
      %28 = acir.ref @top.rf::@top.rf.pending.26 : !acir.cell<i1>
      %29 = acir.ref @top.rf::@top.rf.pending.27 : !acir.cell<i1>
      %30 = acir.ref @top.rf::@top.rf.pending.28 : !acir.cell<i1>
      %31 = acir.ref @top.rf::@top.rf.pending.29 : !acir.cell<i1>
      %32 = acir.ref @top.rf::@top.rf.pending.30 : !acir.cell<i1>
      %33 = acir.ref @top.rf::@top.rf.pending.31 : !acir.cell<i1>
      %34 = acir.aggregate(%2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16, %17, %18, %19, %20, %21, %22, %23, %24, %25, %26, %27, %28, %29, %30, %31, %32, %33) {array_name = "top.rf.pending"} : (!acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>) -> !acir.array<!acir.cell<i1>, [32]> loc(#loc64)
      scf.if %1 {
        %35 = acir.extract %arg0 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc81)
        %36 = "acir.array.index"(%34, %35) {signed_index = false} : (!acir.array<!acir.cell<i1>, [32]>, i32) -> !acir.cell<i1> loc(#loc82)
        acir.cell.write %36, %true {acir.id = "v49", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc82)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.writeback_body.0.call.11.2.0.call.11.6 : (!acir.struct<"src_isa_f0424bc8.Retired">) -> () {acir.function = {contract = "Module:top.rf", definition = "commit", instance = "top.rf", interface = "Module:top.rf:commit", kind = "rule", name = "top.writeback_body.0.call.11.2.0.call.11.6", output_type = "void", owner = "top.rf", params = [{id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Retired"}], public = true, result = "void", rule = "commit"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Retired"> loc("acir.binding:top.writeback_body.0.call.11.2.0.call.11.6:inst")):
      %false = arith.constant false
      %c0_i32 = arith.constant 0 : i32
      %0 = acir.extract %arg0 {field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc84)
      %1 = scf.if %0 -> (i1) {
        %68 = acir.extract %arg0 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc85)
        %69 = arith.cmpi ne, %68, %c0_i32 : i32 loc(#loc85)
        scf.yield %69 : i1
      } else {
        scf.yield %false : i1
      }
      %2 = acir.ref @top.rf::@top.rf.registers.0 : !acir.cell<i32>
      %3 = acir.ref @top.rf::@top.rf.registers.1 : !acir.cell<i32>
      %4 = acir.ref @top.rf::@top.rf.registers.2 : !acir.cell<i32>
      %5 = acir.ref @top.rf::@top.rf.registers.3 : !acir.cell<i32>
      %6 = acir.ref @top.rf::@top.rf.registers.4 : !acir.cell<i32>
      %7 = acir.ref @top.rf::@top.rf.registers.5 : !acir.cell<i32>
      %8 = acir.ref @top.rf::@top.rf.registers.6 : !acir.cell<i32>
      %9 = acir.ref @top.rf::@top.rf.registers.7 : !acir.cell<i32>
      %10 = acir.ref @top.rf::@top.rf.registers.8 : !acir.cell<i32>
      %11 = acir.ref @top.rf::@top.rf.registers.9 : !acir.cell<i32>
      %12 = acir.ref @top.rf::@top.rf.registers.10 : !acir.cell<i32>
      %13 = acir.ref @top.rf::@top.rf.registers.11 : !acir.cell<i32>
      %14 = acir.ref @top.rf::@top.rf.registers.12 : !acir.cell<i32>
      %15 = acir.ref @top.rf::@top.rf.registers.13 : !acir.cell<i32>
      %16 = acir.ref @top.rf::@top.rf.registers.14 : !acir.cell<i32>
      %17 = acir.ref @top.rf::@top.rf.registers.15 : !acir.cell<i32>
      %18 = acir.ref @top.rf::@top.rf.registers.16 : !acir.cell<i32>
      %19 = acir.ref @top.rf::@top.rf.registers.17 : !acir.cell<i32>
      %20 = acir.ref @top.rf::@top.rf.registers.18 : !acir.cell<i32>
      %21 = acir.ref @top.rf::@top.rf.registers.19 : !acir.cell<i32>
      %22 = acir.ref @top.rf::@top.rf.registers.20 : !acir.cell<i32>
      %23 = acir.ref @top.rf::@top.rf.registers.21 : !acir.cell<i32>
      %24 = acir.ref @top.rf::@top.rf.registers.22 : !acir.cell<i32>
      %25 = acir.ref @top.rf::@top.rf.registers.23 : !acir.cell<i32>
      %26 = acir.ref @top.rf::@top.rf.registers.24 : !acir.cell<i32>
      %27 = acir.ref @top.rf::@top.rf.registers.25 : !acir.cell<i32>
      %28 = acir.ref @top.rf::@top.rf.registers.26 : !acir.cell<i32>
      %29 = acir.ref @top.rf::@top.rf.registers.27 : !acir.cell<i32>
      %30 = acir.ref @top.rf::@top.rf.registers.28 : !acir.cell<i32>
      %31 = acir.ref @top.rf::@top.rf.registers.29 : !acir.cell<i32>
      %32 = acir.ref @top.rf::@top.rf.registers.30 : !acir.cell<i32>
      %33 = acir.ref @top.rf::@top.rf.registers.31 : !acir.cell<i32>
      %34 = acir.aggregate(%2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13, %14, %15, %16, %17, %18, %19, %20, %21, %22, %23, %24, %25, %26, %27, %28, %29, %30, %31, %32, %33) {array_name = "top.rf.registers"} : (!acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>, !acir.cell<i32>) -> !acir.array<!acir.cell<i32>, [32]> loc(#loc76)
      %35 = acir.ref @top.rf::@top.rf.pending.0 : !acir.cell<i1>
      %36 = acir.ref @top.rf::@top.rf.pending.1 : !acir.cell<i1>
      %37 = acir.ref @top.rf::@top.rf.pending.2 : !acir.cell<i1>
      %38 = acir.ref @top.rf::@top.rf.pending.3 : !acir.cell<i1>
      %39 = acir.ref @top.rf::@top.rf.pending.4 : !acir.cell<i1>
      %40 = acir.ref @top.rf::@top.rf.pending.5 : !acir.cell<i1>
      %41 = acir.ref @top.rf::@top.rf.pending.6 : !acir.cell<i1>
      %42 = acir.ref @top.rf::@top.rf.pending.7 : !acir.cell<i1>
      %43 = acir.ref @top.rf::@top.rf.pending.8 : !acir.cell<i1>
      %44 = acir.ref @top.rf::@top.rf.pending.9 : !acir.cell<i1>
      %45 = acir.ref @top.rf::@top.rf.pending.10 : !acir.cell<i1>
      %46 = acir.ref @top.rf::@top.rf.pending.11 : !acir.cell<i1>
      %47 = acir.ref @top.rf::@top.rf.pending.12 : !acir.cell<i1>
      %48 = acir.ref @top.rf::@top.rf.pending.13 : !acir.cell<i1>
      %49 = acir.ref @top.rf::@top.rf.pending.14 : !acir.cell<i1>
      %50 = acir.ref @top.rf::@top.rf.pending.15 : !acir.cell<i1>
      %51 = acir.ref @top.rf::@top.rf.pending.16 : !acir.cell<i1>
      %52 = acir.ref @top.rf::@top.rf.pending.17 : !acir.cell<i1>
      %53 = acir.ref @top.rf::@top.rf.pending.18 : !acir.cell<i1>
      %54 = acir.ref @top.rf::@top.rf.pending.19 : !acir.cell<i1>
      %55 = acir.ref @top.rf::@top.rf.pending.20 : !acir.cell<i1>
      %56 = acir.ref @top.rf::@top.rf.pending.21 : !acir.cell<i1>
      %57 = acir.ref @top.rf::@top.rf.pending.22 : !acir.cell<i1>
      %58 = acir.ref @top.rf::@top.rf.pending.23 : !acir.cell<i1>
      %59 = acir.ref @top.rf::@top.rf.pending.24 : !acir.cell<i1>
      %60 = acir.ref @top.rf::@top.rf.pending.25 : !acir.cell<i1>
      %61 = acir.ref @top.rf::@top.rf.pending.26 : !acir.cell<i1>
      %62 = acir.ref @top.rf::@top.rf.pending.27 : !acir.cell<i1>
      %63 = acir.ref @top.rf::@top.rf.pending.28 : !acir.cell<i1>
      %64 = acir.ref @top.rf::@top.rf.pending.29 : !acir.cell<i1>
      %65 = acir.ref @top.rf::@top.rf.pending.30 : !acir.cell<i1>
      %66 = acir.ref @top.rf::@top.rf.pending.31 : !acir.cell<i1>
      %67 = acir.aggregate(%35, %36, %37, %38, %39, %40, %41, %42, %43, %44, %45, %46, %47, %48, %49, %50, %51, %52, %53, %54, %55, %56, %57, %58, %59, %60, %61, %62, %63, %64, %65, %66) {array_name = "top.rf.pending"} : (!acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>, !acir.cell<i1>) -> !acir.array<!acir.cell<i1>, [32]> loc(#loc64)
      scf.if %1 {
        %68 = acir.extract %arg0 {field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc86)
        %69 = acir.extract %arg0 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc87)
        %70 = "acir.array.index"(%34, %69) {signed_index = false} : (!acir.array<!acir.cell<i32>, [32]>, i32) -> !acir.cell<i32> loc(#loc88)
        acir.cell.write %70, %68 {acir.id = "v49", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc88)
        %71 = "acir.array.index"(%67, %69) {signed_index = false} : (!acir.array<!acir.cell<i1>, [32]>, i32) -> !acir.cell<i1> loc(#loc89)
        acir.cell.write %71, %false {acir.id = "v86", acir.source_type = "void"} : !acir.cell<i1>, i1 loc(#loc89)
      }
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.memory attributes {acir.instance = {definition = "src_model_8294833e.DataMemory", name = "top.memory", parent = "top", rules = ["load", "store"]}} {
    acir.contract {relations = [], rules = ["load", "store"]}
    acir.cell @top.memory.words : !acir.array<i32, [16]> {initial_fn = "ac_init_top.memory.words", name = "top.memory.words", type = "array<u32;16>"}
    func.func private @ac_init_top.rf.registers.0() -> i32
    func.func private @ac_init_top.rf.registers.1() -> i32
    func.func private @ac_init_top.rf.registers.2() -> i32
    func.func private @ac_init_top.rf.registers.3() -> i32
    func.func private @ac_init_top.rf.registers.4() -> i32
    func.func private @ac_init_top.rf.registers.5() -> i32
    func.func private @ac_init_top.rf.registers.6() -> i32
    func.func private @ac_init_top.rf.registers.7() -> i32
    func.func private @ac_init_top.rf.registers.8() -> i32
    func.func private @ac_init_top.rf.registers.9() -> i32
    func.func private @ac_init_top.rf.registers.10() -> i32
    func.func private @ac_init_top.rf.registers.11() -> i32
    func.func private @ac_init_top.rf.registers.12() -> i32
    func.func private @ac_init_top.rf.registers.13() -> i32
    func.func private @ac_init_top.rf.registers.14() -> i32
    func.func private @ac_init_top.rf.registers.15() -> i32
    func.func private @ac_init_top.rf.registers.16() -> i32
    func.func private @ac_init_top.rf.registers.17() -> i32
    func.func private @ac_init_top.rf.registers.18() -> i32
    func.func private @ac_init_top.rf.registers.19() -> i32
    func.func private @ac_init_top.rf.registers.20() -> i32
    func.func private @ac_init_top.rf.registers.21() -> i32
    func.func private @ac_init_top.rf.registers.22() -> i32
    func.func private @ac_init_top.rf.registers.23() -> i32
    func.func private @ac_init_top.rf.registers.24() -> i32
    func.func private @ac_init_top.rf.registers.25() -> i32
    func.func private @ac_init_top.rf.registers.26() -> i32
    func.func private @ac_init_top.rf.registers.27() -> i32
    func.func private @ac_init_top.rf.registers.28() -> i32
    func.func private @ac_init_top.rf.registers.29() -> i32
    func.func private @ac_init_top.rf.registers.30() -> i32
    func.func private @ac_init_top.rf.registers.31() -> i32
    func.func private @ac_init_top.rf.pending.0() -> i1
    func.func private @ac_init_top.rf.pending.1() -> i1
    func.func private @ac_init_top.rf.pending.2() -> i1
    func.func private @ac_init_top.rf.pending.3() -> i1
    func.func private @ac_init_top.rf.pending.4() -> i1
    func.func private @ac_init_top.rf.pending.5() -> i1
    func.func private @ac_init_top.rf.pending.6() -> i1
    func.func private @ac_init_top.rf.pending.7() -> i1
    func.func private @ac_init_top.rf.pending.8() -> i1
    func.func private @ac_init_top.rf.pending.9() -> i1
    func.func private @ac_init_top.rf.pending.10() -> i1
    func.func private @ac_init_top.rf.pending.11() -> i1
    func.func private @ac_init_top.rf.pending.12() -> i1
    func.func private @ac_init_top.rf.pending.13() -> i1
    func.func private @ac_init_top.rf.pending.14() -> i1
    func.func private @ac_init_top.rf.pending.15() -> i1
    func.func private @ac_init_top.rf.pending.16() -> i1
    func.func private @ac_init_top.rf.pending.17() -> i1
    func.func private @ac_init_top.rf.pending.18() -> i1
    func.func private @ac_init_top.rf.pending.19() -> i1
    func.func private @ac_init_top.rf.pending.20() -> i1
    func.func private @ac_init_top.rf.pending.21() -> i1
    func.func private @ac_init_top.rf.pending.22() -> i1
    func.func private @ac_init_top.rf.pending.23() -> i1
    func.func private @ac_init_top.rf.pending.24() -> i1
    func.func private @ac_init_top.rf.pending.25() -> i1
    func.func private @ac_init_top.rf.pending.26() -> i1
    func.func private @ac_init_top.rf.pending.27() -> i1
    func.func private @ac_init_top.rf.pending.28() -> i1
    func.func private @ac_init_top.rf.pending.29() -> i1
    func.func private @ac_init_top.rf.pending.30() -> i1
    func.func private @ac_init_top.rf.pending.31() -> i1
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, [16]>
    func.func private @ac_init_top.if_id_cap() -> i32
    func.func private @ac_init_top.if_id() -> tuple<>
    func.func private @ac_init_top.id_ex_cap() -> i32
    func.func private @ac_init_top.id_ex() -> tuple<>
    func.func private @ac_init_top.ex_mem_cap() -> i32
    func.func private @ac_init_top.ex_mem() -> tuple<>
    func.func private @ac_init_top.mem_wb_cap() -> i32
    func.func private @ac_init_top.mem_wb() -> tuple<>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, [6]>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.49(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.memory_stage_body.0.call.8.2.0.call.8.14 : (i32) -> i32 {acir.function = {contract = "Module:top.memory", definition = "load", instance = "top.memory", interface = "Module:top.memory:load", kind = "rule", name = "top.memory_stage_body.0.call.8.2.0.call.8.14", output_type = "u32", owner = "top.memory", params = [{id = "v1", name = "address", targets = [], type = "u32"}], public = true, result = "u32", rule = "load"}} body {
    ^bb0(%arg0: i32 loc("acir.binding:top.memory_stage_body.0.call.8.2.0.call.8.14:address")):
      %c2_i32 = arith.constant 2 : i32 loc(#loc91)
      %c64_i32 = arith.constant 64 : i32
      %c0_i32 = arith.constant 0 : i32 loc(#loc92)
      %c3_i32 = arith.constant 3 : i32 loc(#loc93)
      %false = arith.constant false
      %0 = arith.andi %arg0, %c3_i32 : i32 loc(#loc94)
      %1 = arith.cmpi eq, %0, %c0_i32 : i32 loc(#loc94)
      %2 = scf.if %1 -> (i1) {
        %7 = arith.cmpi ult, %arg0, %c64_i32 : i32 loc(#loc95)
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      acir.check %2 {acir.id = "v17", acir.source_type = "void"} loc(#loc96)
      %3 = acir.ref @top.memory::@top.memory.words : !acir.cell<!acir.array<i32, [16]>>
      %4 = acir.cell.read %3 {acir.id = "v19", acir.source_type = "array<u32;16>", role = "observe"} : !acir.cell<!acir.array<i32, [16]>> -> !acir.array<i32, [16]> loc(#loc97)
      %5 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v21", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc98)
      %6 = "acir.array.index"(%4, %5) {signed_index = false} : (!acir.array<i32, [16]>, i32) -> i32 loc(#loc97)
      acir.return %6 : i32 {acir.source_type = "void"}
    }
    acir.rule @top.memory_stage_body.0.call.8.2.0.call.9.21 : (i32, i32) -> () {acir.function = {contract = "Module:top.memory", definition = "store", instance = "top.memory", interface = "Module:top.memory:store", kind = "rule", name = "top.memory_stage_body.0.call.8.2.0.call.9.21", output_type = "void", owner = "top.memory", params = [{id = "v1", name = "address", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "store"}} body {
    ^bb0(%arg0: i32 loc("acir.binding:top.memory_stage_body.0.call.8.2.0.call.9.21:address"), %arg1: i32 loc("acir.binding:top.memory_stage_body.0.call.8.2.0.call.9.21:value")):
      %c2_i32 = arith.constant 2 : i32 loc(#loc101)
      %false = arith.constant false
      %c64_i32 = arith.constant 64 : i32
      %c0_i32 = arith.constant 0 : i32 loc(#loc102)
      %c3_i32 = arith.constant 3 : i32 loc(#loc103)
      %0 = arith.andi %arg0, %c3_i32 : i32 loc(#loc104)
      %1 = arith.cmpi eq, %0, %c0_i32 : i32 loc(#loc104)
      %2 = scf.if %1 -> (i1) {
        %7 = arith.cmpi ult, %arg0, %c64_i32 : i32 loc(#loc105)
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      acir.check %2 {acir.id = "v16", acir.source_type = "void"} loc(#loc106)
      %3 = acir.ref @top.memory::@top.memory.words : !acir.cell<!acir.array<i32, [16]>>
      %4 = acir.cell.read %3 {acir.id = "v20", acir.source_type = "array<u32;16>", role = "observe"} : !acir.cell<!acir.array<i32, [16]>> -> !acir.array<i32, [16]> loc(#loc107)
      %5 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v22", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc108)
      %6 = acir.update %4, %arg1[%5] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [16]>, i32, i32) -> !acir.array<i32, [16]> loc(#loc107)
      acir.cell.write %3, %6 {acir.id = "v24", acir.source_type = "void"} : !acir.cell<!acir.array<i32, [16]>>, !acir.array<i32, [16]> loc(#loc107)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.fetch attributes {acir.instance = {definition = "src_model_8294833e.Fetch", name = "top.fetch", parent = "top", rules = ["fetch"]}} {
    acir.contract {relations = [], rules = ["fetch"]}
    acir.cell @top.fetch.rom : !acir.array<i32, [6]> {initial_fn = "ac_init_top.fetch.rom", name = "top.fetch.rom", type = "array<u32;6>"}
    acir.cell @top.fetch.pc : i32 {initial_fn = "ac_init_top.fetch.pc", name = "top.fetch.pc", type = "u32"}
    func.func private @ac_init_top.rf.registers.0() -> i32
    func.func private @ac_init_top.rf.registers.1() -> i32
    func.func private @ac_init_top.rf.registers.2() -> i32
    func.func private @ac_init_top.rf.registers.3() -> i32
    func.func private @ac_init_top.rf.registers.4() -> i32
    func.func private @ac_init_top.rf.registers.5() -> i32
    func.func private @ac_init_top.rf.registers.6() -> i32
    func.func private @ac_init_top.rf.registers.7() -> i32
    func.func private @ac_init_top.rf.registers.8() -> i32
    func.func private @ac_init_top.rf.registers.9() -> i32
    func.func private @ac_init_top.rf.registers.10() -> i32
    func.func private @ac_init_top.rf.registers.11() -> i32
    func.func private @ac_init_top.rf.registers.12() -> i32
    func.func private @ac_init_top.rf.registers.13() -> i32
    func.func private @ac_init_top.rf.registers.14() -> i32
    func.func private @ac_init_top.rf.registers.15() -> i32
    func.func private @ac_init_top.rf.registers.16() -> i32
    func.func private @ac_init_top.rf.registers.17() -> i32
    func.func private @ac_init_top.rf.registers.18() -> i32
    func.func private @ac_init_top.rf.registers.19() -> i32
    func.func private @ac_init_top.rf.registers.20() -> i32
    func.func private @ac_init_top.rf.registers.21() -> i32
    func.func private @ac_init_top.rf.registers.22() -> i32
    func.func private @ac_init_top.rf.registers.23() -> i32
    func.func private @ac_init_top.rf.registers.24() -> i32
    func.func private @ac_init_top.rf.registers.25() -> i32
    func.func private @ac_init_top.rf.registers.26() -> i32
    func.func private @ac_init_top.rf.registers.27() -> i32
    func.func private @ac_init_top.rf.registers.28() -> i32
    func.func private @ac_init_top.rf.registers.29() -> i32
    func.func private @ac_init_top.rf.registers.30() -> i32
    func.func private @ac_init_top.rf.registers.31() -> i32
    func.func private @ac_init_top.rf.pending.0() -> i1
    func.func private @ac_init_top.rf.pending.1() -> i1
    func.func private @ac_init_top.rf.pending.2() -> i1
    func.func private @ac_init_top.rf.pending.3() -> i1
    func.func private @ac_init_top.rf.pending.4() -> i1
    func.func private @ac_init_top.rf.pending.5() -> i1
    func.func private @ac_init_top.rf.pending.6() -> i1
    func.func private @ac_init_top.rf.pending.7() -> i1
    func.func private @ac_init_top.rf.pending.8() -> i1
    func.func private @ac_init_top.rf.pending.9() -> i1
    func.func private @ac_init_top.rf.pending.10() -> i1
    func.func private @ac_init_top.rf.pending.11() -> i1
    func.func private @ac_init_top.rf.pending.12() -> i1
    func.func private @ac_init_top.rf.pending.13() -> i1
    func.func private @ac_init_top.rf.pending.14() -> i1
    func.func private @ac_init_top.rf.pending.15() -> i1
    func.func private @ac_init_top.rf.pending.16() -> i1
    func.func private @ac_init_top.rf.pending.17() -> i1
    func.func private @ac_init_top.rf.pending.18() -> i1
    func.func private @ac_init_top.rf.pending.19() -> i1
    func.func private @ac_init_top.rf.pending.20() -> i1
    func.func private @ac_init_top.rf.pending.21() -> i1
    func.func private @ac_init_top.rf.pending.22() -> i1
    func.func private @ac_init_top.rf.pending.23() -> i1
    func.func private @ac_init_top.rf.pending.24() -> i1
    func.func private @ac_init_top.rf.pending.25() -> i1
    func.func private @ac_init_top.rf.pending.26() -> i1
    func.func private @ac_init_top.rf.pending.27() -> i1
    func.func private @ac_init_top.rf.pending.28() -> i1
    func.func private @ac_init_top.rf.pending.29() -> i1
    func.func private @ac_init_top.rf.pending.30() -> i1
    func.func private @ac_init_top.rf.pending.31() -> i1
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, [16]>
    func.func private @ac_init_top.if_id_cap() -> i32
    func.func private @ac_init_top.if_id() -> tuple<>
    func.func private @ac_init_top.id_ex_cap() -> i32
    func.func private @ac_init_top.id_ex() -> tuple<>
    func.func private @ac_init_top.ex_mem_cap() -> i32
    func.func private @ac_init_top.ex_mem() -> tuple<>
    func.func private @ac_init_top.mem_wb_cap() -> i32
    func.func private @ac_init_top.mem_wb() -> tuple<>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, [6]>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.49(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.fetch_body.0.call.0.6 : (i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> {acir.function = {contract = "Module:top.fetch", definition = "fetch", instance = "top.fetch", interface = "Module:top.fetch:fetch", kind = "rule", name = "top.fetch_body.0.call.0.6", output_type = "queue<src_isa_f0424bc8.Fetched>", owner = "top.fetch", params = [{id = "v1", name = "current_pc", targets = [], type = "u32"}], public = true, result = "src_isa_f0424bc8.Fetched", rule = "fetch"}} body {
    ^bb0(%arg0: i32 loc("acir.binding:top.fetch_body.0.call.0.6:current_pc")):
      %c4_i32 = arith.constant 4 : i32 loc(#loc110)
      %c2_i32 = arith.constant 2 : i32 loc(#loc111)
      %true = arith.constant true
      %0 = acir.ref @top.fetch::@top.fetch.rom : !acir.cell<!acir.array<i32, [6]>>
      %1 = acir.cell.read %0 {acir.id = "v8", acir.source_type = "array<u32;6>", role = "observe"} : !acir.cell<!acir.array<i32, [6]>> -> !acir.array<i32, [6]> loc(#loc112)
      %2 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v10", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc113)
      %3 = "acir.array.index"(%1, %2) {signed_index = false} : (!acir.array<i32, [6]>, i32) -> i32 loc(#loc114)
      %4 = arith.addi %arg0, %c4_i32 : i32 loc(#loc115)
      %5 = acir.ref @top.fetch::@top.fetch.pc : !acir.cell<i32>
      acir.cell.write %5, %4 {acir.id = "v16", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc116)
      %6 = acir.aggregate(%arg0, %3) : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc117)
      %7 = "acir.optional"(%6, %true) : (!acir.struct<"src_isa_f0424bc8.Fetched">, i1) -> !acir.struct<"src_isa_f0424bc8.Fetched">
      acir.return %7 : !acir.struct<"src_isa_f0424bc8.Fetched"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.fetch_body", owner = "top.fetch", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.ref @top.fetch::@top.fetch.pc : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc118)
      %c24_i32 = arith.constant 24 : i32 loc(#loc119)
      %2 = arith.cmpi ult, %1, %c24_i32 : i32 loc(#loc120)
      %3 = acir.ref @top::@top.if_id : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
      %4 = scf.if %2 -> (i1) {
        %5 = acir.rule.call @top.fetch::@top.fetch_body.0.call.0.6(%1) {acir.id = "v7", acir.source_type = "src_isa_f0424bc8.Fetched", contract = "Module:top.fetch", instance = "top.fetch", interface = "Module:top.fetch:fetch", output_binding = @top::@top.if_id, role = "rule", rule = "fetch"} : (i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc121)
        %false = arith.constant false
        scf.yield %false : i1
      } else {
        scf.yield %true : i1
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.decode_stage attributes {acir.instance = {definition = "src_model_8294833e.DecodeStage", name = "top.decode_stage", parent = "top", rules = ["read_operands"]}} {
    acir.contract {relations = [], rules = ["read_operands"]}
    func.func private @ac_init_top.rf.registers.0() -> i32
    func.func private @ac_init_top.rf.registers.1() -> i32
    func.func private @ac_init_top.rf.registers.2() -> i32
    func.func private @ac_init_top.rf.registers.3() -> i32
    func.func private @ac_init_top.rf.registers.4() -> i32
    func.func private @ac_init_top.rf.registers.5() -> i32
    func.func private @ac_init_top.rf.registers.6() -> i32
    func.func private @ac_init_top.rf.registers.7() -> i32
    func.func private @ac_init_top.rf.registers.8() -> i32
    func.func private @ac_init_top.rf.registers.9() -> i32
    func.func private @ac_init_top.rf.registers.10() -> i32
    func.func private @ac_init_top.rf.registers.11() -> i32
    func.func private @ac_init_top.rf.registers.12() -> i32
    func.func private @ac_init_top.rf.registers.13() -> i32
    func.func private @ac_init_top.rf.registers.14() -> i32
    func.func private @ac_init_top.rf.registers.15() -> i32
    func.func private @ac_init_top.rf.registers.16() -> i32
    func.func private @ac_init_top.rf.registers.17() -> i32
    func.func private @ac_init_top.rf.registers.18() -> i32
    func.func private @ac_init_top.rf.registers.19() -> i32
    func.func private @ac_init_top.rf.registers.20() -> i32
    func.func private @ac_init_top.rf.registers.21() -> i32
    func.func private @ac_init_top.rf.registers.22() -> i32
    func.func private @ac_init_top.rf.registers.23() -> i32
    func.func private @ac_init_top.rf.registers.24() -> i32
    func.func private @ac_init_top.rf.registers.25() -> i32
    func.func private @ac_init_top.rf.registers.26() -> i32
    func.func private @ac_init_top.rf.registers.27() -> i32
    func.func private @ac_init_top.rf.registers.28() -> i32
    func.func private @ac_init_top.rf.registers.29() -> i32
    func.func private @ac_init_top.rf.registers.30() -> i32
    func.func private @ac_init_top.rf.registers.31() -> i32
    func.func private @ac_init_top.rf.pending.0() -> i1
    func.func private @ac_init_top.rf.pending.1() -> i1
    func.func private @ac_init_top.rf.pending.2() -> i1
    func.func private @ac_init_top.rf.pending.3() -> i1
    func.func private @ac_init_top.rf.pending.4() -> i1
    func.func private @ac_init_top.rf.pending.5() -> i1
    func.func private @ac_init_top.rf.pending.6() -> i1
    func.func private @ac_init_top.rf.pending.7() -> i1
    func.func private @ac_init_top.rf.pending.8() -> i1
    func.func private @ac_init_top.rf.pending.9() -> i1
    func.func private @ac_init_top.rf.pending.10() -> i1
    func.func private @ac_init_top.rf.pending.11() -> i1
    func.func private @ac_init_top.rf.pending.12() -> i1
    func.func private @ac_init_top.rf.pending.13() -> i1
    func.func private @ac_init_top.rf.pending.14() -> i1
    func.func private @ac_init_top.rf.pending.15() -> i1
    func.func private @ac_init_top.rf.pending.16() -> i1
    func.func private @ac_init_top.rf.pending.17() -> i1
    func.func private @ac_init_top.rf.pending.18() -> i1
    func.func private @ac_init_top.rf.pending.19() -> i1
    func.func private @ac_init_top.rf.pending.20() -> i1
    func.func private @ac_init_top.rf.pending.21() -> i1
    func.func private @ac_init_top.rf.pending.22() -> i1
    func.func private @ac_init_top.rf.pending.23() -> i1
    func.func private @ac_init_top.rf.pending.24() -> i1
    func.func private @ac_init_top.rf.pending.25() -> i1
    func.func private @ac_init_top.rf.pending.26() -> i1
    func.func private @ac_init_top.rf.pending.27() -> i1
    func.func private @ac_init_top.rf.pending.28() -> i1
    func.func private @ac_init_top.rf.pending.29() -> i1
    func.func private @ac_init_top.rf.pending.30() -> i1
    func.func private @ac_init_top.rf.pending.31() -> i1
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, [16]>
    func.func private @ac_init_top.if_id_cap() -> i32
    func.func private @ac_init_top.if_id() -> tuple<>
    func.func private @ac_init_top.id_ex_cap() -> i32
    func.func private @ac_init_top.id_ex() -> tuple<>
    func.func private @ac_init_top.ex_mem_cap() -> i32
    func.func private @ac_init_top.ex_mem() -> tuple<>
    func.func private @ac_init_top.mem_wb_cap() -> i32
    func.func private @ac_init_top.mem_wb() -> tuple<>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, [6]>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.49(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.decode_stage_body.0.call.4.15 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Decoded">) -> !acir.struct<"src_isa_f0424bc8.Operands"> {acir.function = {contract = "Module:top.decode_stage", definition = "read_operands", instance = "top.decode_stage", interface = "Module:top.decode_stage:read_operands", kind = "rule", name = "top.decode_stage_body.0.call.4.15", output_type = "queue<src_isa_f0424bc8.Operands>", owner = "top.decode_stage", params = [{id = "v1", name = "message", targets = ["top.if_id"], type = "queue<src_isa_f0424bc8.Fetched>"}, {id = "v2", name = "decoded", targets = [], type = "src_isa_f0424bc8.Decoded"}], public = true, result = "src_isa_f0424bc8.Operands", rule = "read_operands"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> loc("acir.binding:top.decode_stage_body.0.call.4.15:message"), %arg1: !acir.struct<"src_isa_f0424bc8.Decoded"> loc("acir.binding:top.decode_stage_body.0.call.4.15:decoded")):
      %true = arith.constant true
      %0 = acir.queue.peek %arg0 {acir.id = "v8", acir.source_type = "src_isa_f0424bc8.Fetched", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc124)
      %1 = acir.extract %arg1 {field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc125)
      %2 = acir.rule.call @top.rf::@top.decode_stage_body.0.call.4.15.0.call.4.10(%1) {acir.id = "v11", acir.source_type = "u32", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:read", role = "rule", rule = "read"} : (i32) -> i32 loc(#loc126)
      %3 = acir.extract %arg1 {field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc127)
      %4 = acir.rule.call @top.rf::@top.decode_stage_body.0.call.4.15.0.call.4.10(%3) {acir.id = "v14", acir.source_type = "u32", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:read", role = "rule", rule = "read"} : (i32) -> i32 loc(#loc128)
      %5 = acir.extract %arg1 {field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc129)
      %6 = scf.if %5 -> (i32) {
        %14 = acir.extract %arg1 {field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc131)
        scf.yield %14 : i32
      } else {
        scf.yield %4 : i32
      } loc(#loc130)
      acir.rule.call @top.rf::@top.decode_stage_body.0.call.4.15.0.call.5.21(%arg1) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:reserve", role = "rule", rule = "reserve"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> () loc(#loc132)
      %7 = acir.extract %arg1 {field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc133)
      %8 = acir.extract %arg1 {field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc134)
      %9 = acir.extract %arg1 {field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc135)
      %10 = acir.extract %arg1 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc136)
      %11 = acir.extract %arg1 {field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc137)
      %12 = acir.aggregate(%7, %8, %9, %10, %2, %6, %4, %11) : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc138)
      %13 = "acir.optional"(%12, %true) : (!acir.struct<"src_isa_f0424bc8.Operands">, i1) -> !acir.struct<"src_isa_f0424bc8.Operands">
      acir.return %13 : !acir.struct<"src_isa_f0424bc8.Operands"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.decode_stage_body", owner = "top.decode_stage", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.ref @top::@top.if_id : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
      %1 = acir.queue.peek %0 {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Fetched", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc139)
      %2 = func.call @top.decode_stage_body.0.call.1.3(%1) {acir.id = "v4", acir.source_type = "src_isa_f0424bc8.Decoded", role = "helper"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc140)
      %3 = acir.ref @top::@top.mem_wb : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      %4 = acir.ref @top::@top.mem_wb : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      %5 = acir.queue.empty %4 {acir.id = "v8", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> i1 loc(#loc141)
      %6 = scf.if %5 -> (i1) {
        %10 = acir.rule.call @top.rf::@top.decode_stage_body.0.call.3.8(%2) {acir.id = "v9", acir.source_type = "bool", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:ready", role = "rule", rule = "ready"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc143)
        scf.yield %10 : i1
      } else {
        %false = arith.constant false
        scf.yield %false : i1
      } loc(#loc142)
      %7 = acir.ref @top::@top.if_id : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
      %8 = acir.ref @top::@top.id_ex : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
      %9 = scf.if %6 -> (i1) {
        %10 = acir.rule.call @top.decode_stage::@top.decode_stage_body.0.call.4.15(%7, %2) {acir.id = "v16", acir.source_type = "src_isa_f0424bc8.Operands", contract = "Module:top.decode_stage", instance = "top.decode_stage", interface = "Module:top.decode_stage:read_operands", output_binding = @top::@top.id_ex, role = "rule", rule = "read_operands"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Decoded">) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc144)
        %false = arith.constant false
        scf.yield %false : i1
      } else {
        scf.yield %true : i1
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.execute attributes {acir.instance = {definition = "src_model_8294833e.Execute", name = "top.execute", parent = "top", rules = ["execute"]}} {
    acir.contract {relations = [], rules = ["execute"]}
    func.func private @ac_init_top.rf.registers.0() -> i32
    func.func private @ac_init_top.rf.registers.1() -> i32
    func.func private @ac_init_top.rf.registers.2() -> i32
    func.func private @ac_init_top.rf.registers.3() -> i32
    func.func private @ac_init_top.rf.registers.4() -> i32
    func.func private @ac_init_top.rf.registers.5() -> i32
    func.func private @ac_init_top.rf.registers.6() -> i32
    func.func private @ac_init_top.rf.registers.7() -> i32
    func.func private @ac_init_top.rf.registers.8() -> i32
    func.func private @ac_init_top.rf.registers.9() -> i32
    func.func private @ac_init_top.rf.registers.10() -> i32
    func.func private @ac_init_top.rf.registers.11() -> i32
    func.func private @ac_init_top.rf.registers.12() -> i32
    func.func private @ac_init_top.rf.registers.13() -> i32
    func.func private @ac_init_top.rf.registers.14() -> i32
    func.func private @ac_init_top.rf.registers.15() -> i32
    func.func private @ac_init_top.rf.registers.16() -> i32
    func.func private @ac_init_top.rf.registers.17() -> i32
    func.func private @ac_init_top.rf.registers.18() -> i32
    func.func private @ac_init_top.rf.registers.19() -> i32
    func.func private @ac_init_top.rf.registers.20() -> i32
    func.func private @ac_init_top.rf.registers.21() -> i32
    func.func private @ac_init_top.rf.registers.22() -> i32
    func.func private @ac_init_top.rf.registers.23() -> i32
    func.func private @ac_init_top.rf.registers.24() -> i32
    func.func private @ac_init_top.rf.registers.25() -> i32
    func.func private @ac_init_top.rf.registers.26() -> i32
    func.func private @ac_init_top.rf.registers.27() -> i32
    func.func private @ac_init_top.rf.registers.28() -> i32
    func.func private @ac_init_top.rf.registers.29() -> i32
    func.func private @ac_init_top.rf.registers.30() -> i32
    func.func private @ac_init_top.rf.registers.31() -> i32
    func.func private @ac_init_top.rf.pending.0() -> i1
    func.func private @ac_init_top.rf.pending.1() -> i1
    func.func private @ac_init_top.rf.pending.2() -> i1
    func.func private @ac_init_top.rf.pending.3() -> i1
    func.func private @ac_init_top.rf.pending.4() -> i1
    func.func private @ac_init_top.rf.pending.5() -> i1
    func.func private @ac_init_top.rf.pending.6() -> i1
    func.func private @ac_init_top.rf.pending.7() -> i1
    func.func private @ac_init_top.rf.pending.8() -> i1
    func.func private @ac_init_top.rf.pending.9() -> i1
    func.func private @ac_init_top.rf.pending.10() -> i1
    func.func private @ac_init_top.rf.pending.11() -> i1
    func.func private @ac_init_top.rf.pending.12() -> i1
    func.func private @ac_init_top.rf.pending.13() -> i1
    func.func private @ac_init_top.rf.pending.14() -> i1
    func.func private @ac_init_top.rf.pending.15() -> i1
    func.func private @ac_init_top.rf.pending.16() -> i1
    func.func private @ac_init_top.rf.pending.17() -> i1
    func.func private @ac_init_top.rf.pending.18() -> i1
    func.func private @ac_init_top.rf.pending.19() -> i1
    func.func private @ac_init_top.rf.pending.20() -> i1
    func.func private @ac_init_top.rf.pending.21() -> i1
    func.func private @ac_init_top.rf.pending.22() -> i1
    func.func private @ac_init_top.rf.pending.23() -> i1
    func.func private @ac_init_top.rf.pending.24() -> i1
    func.func private @ac_init_top.rf.pending.25() -> i1
    func.func private @ac_init_top.rf.pending.26() -> i1
    func.func private @ac_init_top.rf.pending.27() -> i1
    func.func private @ac_init_top.rf.pending.28() -> i1
    func.func private @ac_init_top.rf.pending.29() -> i1
    func.func private @ac_init_top.rf.pending.30() -> i1
    func.func private @ac_init_top.rf.pending.31() -> i1
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, [16]>
    func.func private @ac_init_top.if_id_cap() -> i32
    func.func private @ac_init_top.if_id() -> tuple<>
    func.func private @ac_init_top.id_ex_cap() -> i32
    func.func private @ac_init_top.id_ex() -> tuple<>
    func.func private @ac_init_top.ex_mem_cap() -> i32
    func.func private @ac_init_top.ex_mem() -> tuple<>
    func.func private @ac_init_top.mem_wb_cap() -> i32
    func.func private @ac_init_top.mem_wb() -> tuple<>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, [6]>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.49(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.execute_body.0.call.7.2 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) -> !acir.struct<"src_isa_f0424bc8.Executed"> {acir.function = {contract = "Module:top.execute", definition = "execute", instance = "top.execute", interface = "Module:top.execute:execute", kind = "rule", name = "top.execute_body.0.call.7.2", output_type = "queue<src_isa_f0424bc8.Executed>", owner = "top.execute", params = [{id = "v1", name = "message", targets = ["top.id_ex"], type = "queue<src_isa_f0424bc8.Operands>"}], public = true, result = "src_isa_f0424bc8.Executed", rule = "execute"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> loc("acir.binding:top.execute_body.0.call.7.2:message")):
      %c1_i32 = arith.constant 1 : i32 loc(#loc146)
      %true = arith.constant true
      %0 = acir.queue.peek %arg0 {acir.id = "v7", acir.source_type = "src_isa_f0424bc8.Operands", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc147)
      %1 = acir.extract %0 {field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc148)
      %2 = arith.cmpi eq, %1, %c1_i32 : i32 loc(#loc148)
      %3 = scf.if %2 -> (i32) {
        %11 = acir.extract %0 {field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc150)
        %12 = acir.extract %0 {field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc151)
        %13 = arith.subi %11, %12 : i32 loc(#loc150)
        scf.yield %13 : i32
      } else {
        %11 = acir.extract %0 {field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc152)
        %12 = acir.extract %0 {field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc153)
        %13 = arith.addi %11, %12 : i32 loc(#loc152)
        scf.yield %13 : i32
      } loc(#loc149)
      %4 = acir.extract %0 {field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc154)
      %5 = acir.extract %0 {field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc155)
      %6 = acir.extract %0 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc156)
      %7 = acir.extract %0 {field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc157)
      %8 = acir.extract %0 {field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc158)
      %9 = acir.aggregate(%4, %5, %1, %6, %3, %7, %8) : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc159)
      %10 = "acir.optional"(%9, %true) : (!acir.struct<"src_isa_f0424bc8.Executed">, i1) -> !acir.struct<"src_isa_f0424bc8.Executed">
      acir.return %10 : !acir.struct<"src_isa_f0424bc8.Executed"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.execute_body", owner = "top.execute", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.ref @top::@top.id_ex : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
      %1 = acir.rule.call @top.execute::@top.execute_body.0.call.7.2(%0) {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Executed", contract = "Module:top.execute", instance = "top.execute", interface = "Module:top.execute:execute", output_binding = @top::@top.ex_mem, role = "rule", rule = "execute"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc160)
      %2 = acir.ref @top::@top.ex_mem : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
      %false = arith.constant false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.memory_stage attributes {acir.instance = {definition = "src_model_8294833e.MemoryStage", name = "top.memory_stage", parent = "top", rules = ["access"]}} {
    acir.contract {relations = [], rules = ["access"]}
    func.func private @ac_init_top.rf.registers.0() -> i32
    func.func private @ac_init_top.rf.registers.1() -> i32
    func.func private @ac_init_top.rf.registers.2() -> i32
    func.func private @ac_init_top.rf.registers.3() -> i32
    func.func private @ac_init_top.rf.registers.4() -> i32
    func.func private @ac_init_top.rf.registers.5() -> i32
    func.func private @ac_init_top.rf.registers.6() -> i32
    func.func private @ac_init_top.rf.registers.7() -> i32
    func.func private @ac_init_top.rf.registers.8() -> i32
    func.func private @ac_init_top.rf.registers.9() -> i32
    func.func private @ac_init_top.rf.registers.10() -> i32
    func.func private @ac_init_top.rf.registers.11() -> i32
    func.func private @ac_init_top.rf.registers.12() -> i32
    func.func private @ac_init_top.rf.registers.13() -> i32
    func.func private @ac_init_top.rf.registers.14() -> i32
    func.func private @ac_init_top.rf.registers.15() -> i32
    func.func private @ac_init_top.rf.registers.16() -> i32
    func.func private @ac_init_top.rf.registers.17() -> i32
    func.func private @ac_init_top.rf.registers.18() -> i32
    func.func private @ac_init_top.rf.registers.19() -> i32
    func.func private @ac_init_top.rf.registers.20() -> i32
    func.func private @ac_init_top.rf.registers.21() -> i32
    func.func private @ac_init_top.rf.registers.22() -> i32
    func.func private @ac_init_top.rf.registers.23() -> i32
    func.func private @ac_init_top.rf.registers.24() -> i32
    func.func private @ac_init_top.rf.registers.25() -> i32
    func.func private @ac_init_top.rf.registers.26() -> i32
    func.func private @ac_init_top.rf.registers.27() -> i32
    func.func private @ac_init_top.rf.registers.28() -> i32
    func.func private @ac_init_top.rf.registers.29() -> i32
    func.func private @ac_init_top.rf.registers.30() -> i32
    func.func private @ac_init_top.rf.registers.31() -> i32
    func.func private @ac_init_top.rf.pending.0() -> i1
    func.func private @ac_init_top.rf.pending.1() -> i1
    func.func private @ac_init_top.rf.pending.2() -> i1
    func.func private @ac_init_top.rf.pending.3() -> i1
    func.func private @ac_init_top.rf.pending.4() -> i1
    func.func private @ac_init_top.rf.pending.5() -> i1
    func.func private @ac_init_top.rf.pending.6() -> i1
    func.func private @ac_init_top.rf.pending.7() -> i1
    func.func private @ac_init_top.rf.pending.8() -> i1
    func.func private @ac_init_top.rf.pending.9() -> i1
    func.func private @ac_init_top.rf.pending.10() -> i1
    func.func private @ac_init_top.rf.pending.11() -> i1
    func.func private @ac_init_top.rf.pending.12() -> i1
    func.func private @ac_init_top.rf.pending.13() -> i1
    func.func private @ac_init_top.rf.pending.14() -> i1
    func.func private @ac_init_top.rf.pending.15() -> i1
    func.func private @ac_init_top.rf.pending.16() -> i1
    func.func private @ac_init_top.rf.pending.17() -> i1
    func.func private @ac_init_top.rf.pending.18() -> i1
    func.func private @ac_init_top.rf.pending.19() -> i1
    func.func private @ac_init_top.rf.pending.20() -> i1
    func.func private @ac_init_top.rf.pending.21() -> i1
    func.func private @ac_init_top.rf.pending.22() -> i1
    func.func private @ac_init_top.rf.pending.23() -> i1
    func.func private @ac_init_top.rf.pending.24() -> i1
    func.func private @ac_init_top.rf.pending.25() -> i1
    func.func private @ac_init_top.rf.pending.26() -> i1
    func.func private @ac_init_top.rf.pending.27() -> i1
    func.func private @ac_init_top.rf.pending.28() -> i1
    func.func private @ac_init_top.rf.pending.29() -> i1
    func.func private @ac_init_top.rf.pending.30() -> i1
    func.func private @ac_init_top.rf.pending.31() -> i1
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, [16]>
    func.func private @ac_init_top.if_id_cap() -> i32
    func.func private @ac_init_top.if_id() -> tuple<>
    func.func private @ac_init_top.id_ex_cap() -> i32
    func.func private @ac_init_top.id_ex() -> tuple<>
    func.func private @ac_init_top.ex_mem_cap() -> i32
    func.func private @ac_init_top.ex_mem() -> tuple<>
    func.func private @ac_init_top.mem_wb_cap() -> i32
    func.func private @ac_init_top.mem_wb() -> tuple<>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, [6]>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.49(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.memory_stage_body.0.call.8.2 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) -> !acir.struct<"src_isa_f0424bc8.Retired"> {acir.function = {contract = "Module:top.memory_stage", definition = "access", instance = "top.memory_stage", interface = "Module:top.memory_stage:access", kind = "rule", name = "top.memory_stage_body.0.call.8.2", output_type = "queue<src_isa_f0424bc8.Retired>", owner = "top.memory_stage", params = [{id = "v1", name = "message", targets = ["top.ex_mem"], type = "queue<src_isa_f0424bc8.Executed>"}], public = true, result = "src_isa_f0424bc8.Retired", rule = "access"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> loc("acir.binding:top.memory_stage_body.0.call.8.2:message")):
      %c3_i32 = arith.constant 3 : i32
      %c2_i32 = arith.constant 2 : i32 loc(#loc162)
      %true = arith.constant true
      %0 = acir.queue.peek %arg0 {acir.id = "v7", acir.source_type = "src_isa_f0424bc8.Executed", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc163)
      %1 = acir.extract %0 {field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc164)
      %2 = acir.extract %0 {field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc165)
      %3 = arith.cmpi eq, %2, %c2_i32 : i32 loc(#loc165)
      %4 = scf.if %3 -> (i32) {
        %11 = acir.rule.call @top.memory::@top.memory_stage_body.0.call.8.2.0.call.8.14(%1) {acir.id = "v15", acir.source_type = "u32", contract = "Module:top.memory", instance = "top.memory", interface = "Module:top.memory:load", role = "rule", rule = "load"} : (i32) -> i32 loc(#loc164)
        scf.yield %11 : i32
      } else {
        %11 = arith.cmpi eq, %2, %c3_i32 : i32 loc(#loc166)
        scf.if %11 {
          %12 = acir.extract %0 {field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc167)
          acir.rule.call @top.memory::@top.memory_stage_body.0.call.8.2.0.call.9.21(%1, %12) {acir.id = "v22", acir.source_type = "void", contract = "Module:top.memory", instance = "top.memory", interface = "Module:top.memory:store", role = "rule", rule = "store"} : (i32, i32) -> () loc(#loc168)
        }
        scf.yield %1 : i32
      } loc(#loc164)
      %5 = acir.extract %0 {field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc169)
      %6 = acir.extract %0 {field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc170)
      %7 = acir.extract %0 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc171)
      %8 = acir.extract %0 {field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc172)
      %9 = acir.aggregate(%5, %6, %7, %4, %8) : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc173)
      %10 = "acir.optional"(%9, %true) : (!acir.struct<"src_isa_f0424bc8.Retired">, i1) -> !acir.struct<"src_isa_f0424bc8.Retired">
      acir.return %10 : !acir.struct<"src_isa_f0424bc8.Retired"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.memory_stage_body", owner = "top.memory_stage", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.ref @top::@top.ex_mem : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
      %1 = acir.rule.call @top.memory_stage::@top.memory_stage_body.0.call.8.2(%0) {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Retired", contract = "Module:top.memory_stage", instance = "top.memory_stage", interface = "Module:top.memory_stage:access", output_binding = @top::@top.mem_wb, role = "rule", rule = "access"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc174)
      %2 = acir.ref @top::@top.mem_wb : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      %false = arith.constant false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.writeback attributes {acir.instance = {definition = "src_model_8294833e.Writeback", name = "top.writeback", parent = "top", rules = ["commit"]}} {
    acir.contract {relations = [], rules = ["commit"]}
    acir.cell @top.writeback.retired : i32 {initial_fn = "ac_init_top.writeback.retired", name = "top.writeback.retired", type = "u32"}
    acir.cell @top.writeback.last_pc : i32 {initial_fn = "ac_init_top.writeback.last_pc", name = "top.writeback.last_pc", type = "u32"}
    func.func private @ac_init_top.rf.registers.0() -> i32
    func.func private @ac_init_top.rf.registers.1() -> i32
    func.func private @ac_init_top.rf.registers.2() -> i32
    func.func private @ac_init_top.rf.registers.3() -> i32
    func.func private @ac_init_top.rf.registers.4() -> i32
    func.func private @ac_init_top.rf.registers.5() -> i32
    func.func private @ac_init_top.rf.registers.6() -> i32
    func.func private @ac_init_top.rf.registers.7() -> i32
    func.func private @ac_init_top.rf.registers.8() -> i32
    func.func private @ac_init_top.rf.registers.9() -> i32
    func.func private @ac_init_top.rf.registers.10() -> i32
    func.func private @ac_init_top.rf.registers.11() -> i32
    func.func private @ac_init_top.rf.registers.12() -> i32
    func.func private @ac_init_top.rf.registers.13() -> i32
    func.func private @ac_init_top.rf.registers.14() -> i32
    func.func private @ac_init_top.rf.registers.15() -> i32
    func.func private @ac_init_top.rf.registers.16() -> i32
    func.func private @ac_init_top.rf.registers.17() -> i32
    func.func private @ac_init_top.rf.registers.18() -> i32
    func.func private @ac_init_top.rf.registers.19() -> i32
    func.func private @ac_init_top.rf.registers.20() -> i32
    func.func private @ac_init_top.rf.registers.21() -> i32
    func.func private @ac_init_top.rf.registers.22() -> i32
    func.func private @ac_init_top.rf.registers.23() -> i32
    func.func private @ac_init_top.rf.registers.24() -> i32
    func.func private @ac_init_top.rf.registers.25() -> i32
    func.func private @ac_init_top.rf.registers.26() -> i32
    func.func private @ac_init_top.rf.registers.27() -> i32
    func.func private @ac_init_top.rf.registers.28() -> i32
    func.func private @ac_init_top.rf.registers.29() -> i32
    func.func private @ac_init_top.rf.registers.30() -> i32
    func.func private @ac_init_top.rf.registers.31() -> i32
    func.func private @ac_init_top.rf.pending.0() -> i1
    func.func private @ac_init_top.rf.pending.1() -> i1
    func.func private @ac_init_top.rf.pending.2() -> i1
    func.func private @ac_init_top.rf.pending.3() -> i1
    func.func private @ac_init_top.rf.pending.4() -> i1
    func.func private @ac_init_top.rf.pending.5() -> i1
    func.func private @ac_init_top.rf.pending.6() -> i1
    func.func private @ac_init_top.rf.pending.7() -> i1
    func.func private @ac_init_top.rf.pending.8() -> i1
    func.func private @ac_init_top.rf.pending.9() -> i1
    func.func private @ac_init_top.rf.pending.10() -> i1
    func.func private @ac_init_top.rf.pending.11() -> i1
    func.func private @ac_init_top.rf.pending.12() -> i1
    func.func private @ac_init_top.rf.pending.13() -> i1
    func.func private @ac_init_top.rf.pending.14() -> i1
    func.func private @ac_init_top.rf.pending.15() -> i1
    func.func private @ac_init_top.rf.pending.16() -> i1
    func.func private @ac_init_top.rf.pending.17() -> i1
    func.func private @ac_init_top.rf.pending.18() -> i1
    func.func private @ac_init_top.rf.pending.19() -> i1
    func.func private @ac_init_top.rf.pending.20() -> i1
    func.func private @ac_init_top.rf.pending.21() -> i1
    func.func private @ac_init_top.rf.pending.22() -> i1
    func.func private @ac_init_top.rf.pending.23() -> i1
    func.func private @ac_init_top.rf.pending.24() -> i1
    func.func private @ac_init_top.rf.pending.25() -> i1
    func.func private @ac_init_top.rf.pending.26() -> i1
    func.func private @ac_init_top.rf.pending.27() -> i1
    func.func private @ac_init_top.rf.pending.28() -> i1
    func.func private @ac_init_top.rf.pending.29() -> i1
    func.func private @ac_init_top.rf.pending.30() -> i1
    func.func private @ac_init_top.rf.pending.31() -> i1
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, [16]>
    func.func private @ac_init_top.if_id_cap() -> i32
    func.func private @ac_init_top.if_id() -> tuple<>
    func.func private @ac_init_top.id_ex_cap() -> i32
    func.func private @ac_init_top.id_ex() -> tuple<>
    func.func private @ac_init_top.ex_mem_cap() -> i32
    func.func private @ac_init_top.ex_mem() -> tuple<>
    func.func private @ac_init_top.mem_wb_cap() -> i32
    func.func private @ac_init_top.mem_wb() -> tuple<>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, [6]>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.49(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.writeback_body.0.call.11.2 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) -> () {acir.function = {contract = "Module:top.writeback", definition = "commit", instance = "top.writeback", interface = "Module:top.writeback:commit", kind = "rule", name = "top.writeback_body.0.call.11.2", output_type = "void", owner = "top.writeback", params = [{id = "v1", name = "message", targets = ["top.mem_wb"], type = "queue<src_isa_f0424bc8.Retired>"}], public = true, result = "void", rule = "commit"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> loc("acir.binding:top.writeback_body.0.call.11.2:message")):
      %c1_i32 = arith.constant 1 : i32 loc(#loc176)
      %0 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "src_isa_f0424bc8.Retired", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc177)
      acir.rule.call @top.rf::@top.writeback_body.0.call.11.2.0.call.11.6(%0) {acir.id = "v7", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:commit", role = "rule", rule = "commit"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> () loc(#loc178)
      %1 = acir.ref @top.writeback::@top.writeback.retired : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v9", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc179)
      %3 = arith.addi %2, %c1_i32 : i32 loc(#loc179)
      acir.cell.write %1, %3 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc180)
      %4 = acir.extract %0 {field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc181)
      %5 = acir.ref @top.writeback::@top.writeback.last_pc : !acir.cell<i32>
      acir.cell.write %5, %4 {acir.id = "v16", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc182)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.writeback_body", owner = "top.writeback", params = [], result = "void"}} {
      %true = arith.constant true
      %0 = acir.ref @top::@top.mem_wb : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      acir.rule.call @top.writeback::@top.writeback_body.0.call.11.2(%0) {acir.id = "v3", acir.source_type = "void", contract = "Module:top.writeback", instance = "top.writeback", interface = "Module:top.writeback:commit", role = "rule", rule = "commit"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) -> () loc(#loc183)
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:37)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":17:35)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:64)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":61:62)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":61:71)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":61:80)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":61:89)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":61:98)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":61:104)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":61:61)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:38)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":134:43)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":135:43)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:13)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:12)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:18)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:19)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:22)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:21)
#loc22 = loc("acir.binding:top.decode_stage_body.0.call.1.3:word")
#loc23 = loc("acir.binding:top.decode_stage_body.0.call.1.3:opcode")
#loc24 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:14)
#loc25 = loc("acir.binding:top.decode_stage_body.0.call.1.3:funct3")
#loc26 = loc("acir.binding:top.decode_stage_body.0.call.1.3:funct7")
#loc27 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:10)
#loc28 = loc("acir.binding:top.decode_stage_body.0.call.1.3:rd")
#loc29 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:11)
#loc30 = loc("acir.binding:top.decode_stage_body.0.call.1.3:rs1")
#loc31 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":59:11)
#loc32 = loc("acir.binding:top.decode_stage_body.0.call.1.3:rs2")
#loc33 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:8)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:16)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:9)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:28)
#loc37 = loc("acir.binding:top.decode_stage_body.0.call.1.3:immediate")
#loc38 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":69:10)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:16)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:33)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:48)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:9)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":71:12)
#loc44 = loc("acir.binding:top.decode_stage_body.0.call.1.3:kind")
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:10)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:16)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:9)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:28)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:16)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:33)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:9)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:28)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:46)
#loc54 = loc("acir.binding:top.decode_stage_body.0.call.1.3:immediate_rhs")
#loc55 = loc("acir.binding:top.decode_stage_body.0.call.1.3:writes")
#loc56 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:20)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:12)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":164:17)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":165:17)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":166:18)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":167:18)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":168:9)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":16:57)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:28)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:20)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:55)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:47)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:43)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:75)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:102)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:94)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:90)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":26:12)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:61)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:16)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":32:12)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":32:28)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:21)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:13)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":37:12)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":37:28)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":38:34)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":38:23)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":38:13)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":39:13)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:33)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":49:31)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":49:26)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":49:16)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":49:37)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":49:9)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:16)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:22)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":55:26)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":54:31)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":54:26)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":54:16)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":54:37)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":54:9)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":55:9)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":55:15)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":67:27)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":66:34)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":66:16)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":66:20)
#loc114 = loc("acir.binding:top.fetch_body.0.call.0.6:word")
#loc115 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":67:14)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":67:9)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":68:16)
#loc118 = loc("acir.binding:top.fetch_body:current_pc")
#loc119 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":72:25)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":72:12)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":73:20)
#loc124 = loc("acir.binding:top.decode_stage_body.0.call.4.15:consumed")
#loc125 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:23)
#loc126 = loc("acir.binding:top.decode_stage_body.0.call.4.15:lhs")
#loc127 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":85:31)
#loc128 = loc("acir.binding:top.decode_stage_body.0.call.4.15:store_value")
#loc129 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:36)
#loc130 = loc("acir.binding:top.decode_stage_body.0.call.4.15:rhs")
#loc131 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:15)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":87:9)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:25)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:37)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:51)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:65)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:100)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:16)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":91:23)
#loc140 = loc("acir.binding:top.decode_stage_body:inst")
#loc141 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:19)
#loc142 = loc("acir.binding:top.decode_stage_body:enabled")
#loc143 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:42)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":94:20)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:54)
#loc147 = loc("acir.binding:top.execute_body.0.call.7.2:inst")
#loc148 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:41)
#loc149 = loc("acir.binding:top.execute_body.0.call.7.2:result")
#loc150 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:18)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:29)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:63)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:74)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:25)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:34)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:56)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:73)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:91)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:16)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":109:16)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:25)
#loc163 = loc("acir.binding:top.memory_stage_body.0.call.8.2:inst")
#loc164 = loc("acir.binding:top.memory_stage_body.0.call.8.2:result")
#loc165 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:12)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:14)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:38)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:13)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:24)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:33)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:44)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:61)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:16)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":128:16)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:29)
#loc177 = loc("acir.binding:top.writeback_body.0.call.11.2:inst")
#loc178 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":140:9)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:19)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:9)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":142:19)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":142:9)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":145:9)

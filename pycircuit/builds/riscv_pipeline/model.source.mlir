module attributes {acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, format_version = 9 : i64, top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "immediate", payload_type = i32, type = "u32"}, {default = false, name = "immediate_rhs", payload_type = i1, type = "bool"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "lhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}]}}} {
  func.func @ac_init_top.rf.registers() -> !acir.array<i32, [32]> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers", params = [], result = "array<u32;32>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc1)
    %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc2)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc3)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc4)
    %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc5)
    %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc6)
    %c0_i32_5 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc7)
    %c0_i32_6 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc8)
    %c0_i32_7 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc9)
    %c0_i32_8 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc10)
    %c0_i32_9 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc11)
    %c0_i32_10 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc12)
    %c0_i32_11 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc13)
    %c0_i32_12 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc14)
    %c0_i32_13 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc15)
    %c0_i32_14 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc16)
    %c0_i32_15 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc17)
    %c0_i32_16 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc18)
    %c0_i32_17 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc19)
    %c0_i32_18 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc20)
    %c0_i32_19 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc21)
    %c0_i32_20 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc22)
    %c0_i32_21 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc23)
    %c0_i32_22 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc24)
    %c0_i32_23 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc25)
    %c0_i32_24 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc26)
    %c0_i32_25 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc27)
    %c0_i32_26 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc28)
    %c0_i32_27 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc29)
    %c0_i32_28 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc30)
    %c0_i32_29 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc31)
    %c0_i32_30 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc32)
    %0 = acir.aggregate(%c0_i32, %c0_i32_0, %c0_i32_1, %c0_i32_2, %c0_i32_3, %c0_i32_4, %c0_i32_5, %c0_i32_6, %c0_i32_7, %c0_i32_8, %c0_i32_9, %c0_i32_10, %c0_i32_11, %c0_i32_12, %c0_i32_13, %c0_i32_14, %c0_i32_15, %c0_i32_16, %c0_i32_17, %c0_i32_18, %c0_i32_19, %c0_i32_20, %c0_i32_21, %c0_i32_22, %c0_i32_23, %c0_i32_24, %c0_i32_25, %c0_i32_26, %c0_i32_27, %c0_i32_28, %c0_i32_29, %c0_i32_30) {acir.id = "v33", acir.source_type = "array<u32;32>"} : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [32]> loc(#loc33)
    return %0 : !acir.array<i32, [32]>
  }
  func.func @ac_init_top.rf.pending() -> !acir.array<i1, [32]> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending", params = [], result = "array<bool;32>"}} {
    %false = arith.constant {acir.source_type = "bool"} false loc(#loc34)
    %false_0 = arith.constant {acir.source_type = "bool"} false loc(#loc35)
    %false_1 = arith.constant {acir.source_type = "bool"} false loc(#loc36)
    %false_2 = arith.constant {acir.source_type = "bool"} false loc(#loc37)
    %false_3 = arith.constant {acir.source_type = "bool"} false loc(#loc38)
    %false_4 = arith.constant {acir.source_type = "bool"} false loc(#loc39)
    %false_5 = arith.constant {acir.source_type = "bool"} false loc(#loc40)
    %false_6 = arith.constant {acir.source_type = "bool"} false loc(#loc41)
    %false_7 = arith.constant {acir.source_type = "bool"} false loc(#loc42)
    %false_8 = arith.constant {acir.source_type = "bool"} false loc(#loc43)
    %false_9 = arith.constant {acir.source_type = "bool"} false loc(#loc44)
    %false_10 = arith.constant {acir.source_type = "bool"} false loc(#loc45)
    %false_11 = arith.constant {acir.source_type = "bool"} false loc(#loc46)
    %false_12 = arith.constant {acir.source_type = "bool"} false loc(#loc47)
    %false_13 = arith.constant {acir.source_type = "bool"} false loc(#loc48)
    %false_14 = arith.constant {acir.source_type = "bool"} false loc(#loc49)
    %false_15 = arith.constant {acir.source_type = "bool"} false loc(#loc50)
    %false_16 = arith.constant {acir.source_type = "bool"} false loc(#loc51)
    %false_17 = arith.constant {acir.source_type = "bool"} false loc(#loc52)
    %false_18 = arith.constant {acir.source_type = "bool"} false loc(#loc53)
    %false_19 = arith.constant {acir.source_type = "bool"} false loc(#loc54)
    %false_20 = arith.constant {acir.source_type = "bool"} false loc(#loc55)
    %false_21 = arith.constant {acir.source_type = "bool"} false loc(#loc56)
    %false_22 = arith.constant {acir.source_type = "bool"} false loc(#loc57)
    %false_23 = arith.constant {acir.source_type = "bool"} false loc(#loc58)
    %false_24 = arith.constant {acir.source_type = "bool"} false loc(#loc59)
    %false_25 = arith.constant {acir.source_type = "bool"} false loc(#loc60)
    %false_26 = arith.constant {acir.source_type = "bool"} false loc(#loc61)
    %false_27 = arith.constant {acir.source_type = "bool"} false loc(#loc62)
    %false_28 = arith.constant {acir.source_type = "bool"} false loc(#loc63)
    %false_29 = arith.constant {acir.source_type = "bool"} false loc(#loc64)
    %false_30 = arith.constant {acir.source_type = "bool"} false loc(#loc65)
    %0 = acir.aggregate(%false, %false_0, %false_1, %false_2, %false_3, %false_4, %false_5, %false_6, %false_7, %false_8, %false_9, %false_10, %false_11, %false_12, %false_13, %false_14, %false_15, %false_16, %false_17, %false_18, %false_19, %false_20, %false_21, %false_22, %false_23, %false_24, %false_25, %false_26, %false_27, %false_28, %false_29, %false_30) {acir.id = "v67", acir.source_type = "array<bool;32>"} : (i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1) -> !acir.array<i1, [32]> loc(#loc66)
    return %0 : !acir.array<i1, [32]>
  }
  func.func @ac_init_top.memory.words() -> !acir.array<i32, [16]> attributes {acir.function = {kind = "helper", name = "ac_init_top.memory.words", params = [], result = "array<u32;16>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc67)
    %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc68)
    %c0_i32_1 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc69)
    %c0_i32_2 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc70)
    %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc71)
    %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc72)
    %c0_i32_5 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc73)
    %c0_i32_6 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc74)
    %c0_i32_7 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc75)
    %c0_i32_8 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc76)
    %c0_i32_9 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc77)
    %c0_i32_10 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc78)
    %c0_i32_11 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc79)
    %c0_i32_12 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc80)
    %c0_i32_13 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc81)
    %c0_i32_14 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc82)
    %0 = acir.aggregate(%c0_i32, %c0_i32_0, %c0_i32_1, %c0_i32_2, %c0_i32_3, %c0_i32_4, %c0_i32_5, %c0_i32_6, %c0_i32_7, %c0_i32_8, %c0_i32_9, %c0_i32_10, %c0_i32_11, %c0_i32_12, %c0_i32_13, %c0_i32_14) {acir.id = "v85", acir.source_type = "array<u32;16>"} : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [16]> loc(#loc83)
    return %0 : !acir.array<i32, [16]>
  }
  func.func @ac_init_top.if_id_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.if_id_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.if_id() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.if_id", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.id_ex_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.id_ex_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.id_ex() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.id_ex", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.ex_mem_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.ex_mem_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.ex_mem() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.ex_mem", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.mem_wb_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.mem_wb_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.mem_wb() -> tuple<> attributes {acir.function = {kind = "helper", name = "ac_init_top.mem_wb", params = [], result = "tuple<>"}} {
    %0 = acir.aggregate() {acir.source_type = "tuple<>"} : () -> tuple<>
    return %0 : tuple<>
  }
  func.func @ac_init_top.fetch.rom() -> !acir.array<i32, [6]> attributes {acir.function = {kind = "helper", name = "ac_init_top.fetch.rom", params = [], result = "array<u32;6>"}} {
    %c7340179_i32 = arith.constant {acir.source_type = "u32"} 7340179 : i32 loc(#loc84)
    %c5243155_i32 = arith.constant {acir.source_type = "u32"} 5243155 : i32 loc(#loc85)
    %c2130355_i32 = arith.constant {acir.source_type = "u32"} 2130355 : i32 loc(#loc86)
    %c3153955_i32 = arith.constant {acir.source_type = "u32"} 3153955 : i32 loc(#loc87)
    %c8707_i32 = arith.constant {acir.source_type = "u32"} 8707 : i32 loc(#loc88)
    %c1075970739_i32 = arith.constant {acir.source_type = "u32"} 1075970739 : i32 loc(#loc89)
    %0 = acir.aggregate(%c7340179_i32, %c5243155_i32, %c2130355_i32, %c3153955_i32, %c8707_i32, %c1075970739_i32) {acir.id = "v101", acir.source_type = "array<u32;6>"} : (i32, i32, i32, i32, i32, i32) -> !acir.array<i32, [6]> loc(#loc90)
    return %0 : !acir.array<i32, [6]>
  }
  func.func @ac_init_top.fetch.pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.fetch.pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc91)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.writeback.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.writeback.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc92)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.writeback.last_pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.writeback.last_pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc93)
    return %c0_i32 : i32
  }
  func.func @top.decode_stage_body.0.call.1.3.0.call.1.37(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "top.decode_stage_body.0.call.1.3.0.call.1.37", params = [{id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
    %false = arith.constant {acir.source_type = "bool"} false
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32 loc(#loc94)
    %1 = arith.xori %arg0, %c2048_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc95)
    %c2048_i32_0 = arith.constant {acir.source_type = "u32"} 2048 : i32 loc(#loc96)
    %2 = arith.subi %1, %c2048_i32_0 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc97)
    %true_1 = arith.constant {acir.source_type = "bool"} true
    %false_2 = arith.constant {acir.source_type = "bool"} false
    %3 = acir.unary %false_2 "not" {acir.source_type = "bool"} : i1 -> i1
    acir.check %3 {acir.source_type = "void"}
    return %2 : i32
  }
  func.func @top.decode_stage_body.0.call.1.3(%arg0: !acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded"> attributes {acir.function = {definition = "src_isa_f0424bc8.decode", kind = "helper", name = "top.decode_stage_body.0.call.1.3", params = [{id = "v1", name = "packet", targets = [], type = "src_isa_f0424bc8.Fetched"}], result = "src_isa_f0424bc8.Decoded"}} {
    %true = arith.constant {acir.source_type = "bool"} true
    %0 = acir.aggregate() {acir.source_type = "src_isa_f0424bc8.Decoded"} : () -> !acir.struct<"src_isa_f0424bc8.Decoded">
    %false = arith.constant {acir.source_type = "bool"} false
    %1 = acir.extract %arg0 {acir.id = "v5", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc98)
    %c127_i32 = arith.constant {acir.source_type = "u32"} 127 : i32 loc(#loc99)
    %2 = arith.andi %1, %c127_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc100)
    %c12_i32 = arith.constant {acir.source_type = "u32"} 12 : i32 loc(#loc101)
    %3 = call @ac_builtin_shr_u32(%1, %c12_i32) {acir.id = "v9", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc102)
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc103)
    %4 = arith.andi %3, %c7_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc102)
    %c25_i32 = arith.constant {acir.source_type = "u32"} 25 : i32 loc(#loc104)
    %5 = call @ac_builtin_shr_u32(%1, %c25_i32) {acir.id = "v13", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc105)
    %c7_i32_0 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc106)
    %6 = call @ac_builtin_shr_u32(%1, %c7_i32_0) {acir.id = "v15", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc107)
    %c31_i32 = arith.constant {acir.source_type = "u32"} 31 : i32 loc(#loc108)
    %7 = arith.andi %6, %c31_i32 {acir.id = "v17", acir.source_type = "u32"} : i32 loc(#loc107)
    %c15_i32 = arith.constant {acir.source_type = "u32"} 15 : i32 loc(#loc109)
    %8 = call @ac_builtin_shr_u32(%1, %c15_i32) {acir.id = "v19", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc110)
    %c31_i32_1 = arith.constant {acir.source_type = "u32"} 31 : i32 loc(#loc111)
    %9 = arith.andi %8, %c31_i32_1 {acir.id = "v21", acir.source_type = "u32"} : i32 loc(#loc110)
    %c20_i32 = arith.constant {acir.source_type = "u32"} 20 : i32 loc(#loc112)
    %10 = call @ac_builtin_shr_u32(%1, %c20_i32) {acir.id = "v23", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc113)
    %c31_i32_2 = arith.constant {acir.source_type = "u32"} 31 : i32 loc(#loc114)
    %11 = arith.andi %10, %c31_i32_2 {acir.id = "v25", acir.source_type = "u32"} : i32 loc(#loc113)
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc115)
    %c0_i32_3 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc116)
    %false_4 = arith.constant {acir.source_type = "bool"} false loc(#loc117)
    %true_5 = arith.constant {acir.source_type = "bool"} true loc(#loc118)
    %c19_i32 = arith.constant {acir.source_type = "u32"} 19 : i32 loc(#loc119)
    %12 = arith.cmpi eq, %2, %c19_i32 {acir.id = "v31", acir.source_type = "bool"} : i32 loc(#loc120)
    %13:6 = scf.if %12 -> (i32, i1, i32, i32, i32, i1) {
      %c0_i32_8 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc121)
      %17 = arith.cmpi eq, %4, %c0_i32_8 {acir.id = "v33", acir.source_type = "bool"} : i32 loc(#loc122)
      acir.check %17 {acir.id = "v34", acir.source_type = "void"} loc(#loc123)
      %c0_i32_9 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc124)
      %c20_i32_10 = arith.constant {acir.source_type = "u32"} 20 : i32 loc(#loc125)
      %18 = func.call @ac_builtin_shr_u32(%1, %c20_i32_10) {acir.id = "v37", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc126)
      %19 = func.call @top.decode_stage_body.0.call.1.3.0.call.1.37(%18) {acir.id = "v38", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc127)
      %true_11 = arith.constant {acir.source_type = "bool"} true loc(#loc128)
      scf.yield %19, %true_11, %c0_i32, %7, %c0_i32_9, %true_5 : i32, i1, i32, i32, i32, i1
    } else {
      %c51_i32 = arith.constant {acir.source_type = "u32"} 51 : i32 loc(#loc129)
      %17 = arith.cmpi eq, %2, %c51_i32 {acir.id = "v41", acir.source_type = "bool"} : i32 loc(#loc130)
      %18:6 = scf.if %17 -> (i32, i1, i32, i32, i32, i1) {
        %c0_i32_8 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc131)
        %19 = arith.cmpi eq, %4, %c0_i32_8 {acir.id = "v43", acir.source_type = "bool"} : i32 loc(#loc132)
        %20 = scf.if %19 -> (i1) {
          %c0_i32_9 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc133)
          %23 = arith.cmpi eq, %5, %c0_i32_9 {acir.id = "v45", acir.source_type = "bool"} : i32 loc(#loc134)
          %24 = scf.if %23 -> (i1) {
            %true_10 = arith.constant {acir.source_type = "bool"} true
            scf.yield %true_10 : i1
          } else {
            %c32_i32_10 = arith.constant {acir.source_type = "u32"} 32 : i32 loc(#loc135)
            %25 = arith.cmpi eq, %5, %c32_i32_10 {acir.id = "v48", acir.source_type = "bool"} : i32 loc(#loc136)
            scf.yield %25 : i1
          }
          scf.yield %24 : i1
        } else {
          %false_9 = arith.constant {acir.source_type = "bool"} false
          scf.yield %false_9 : i1
        }
        acir.check %20 {acir.id = "v56", acir.source_type = "void"} loc(#loc137)
        %c32_i32 = arith.constant {acir.source_type = "u32"} 32 : i32 loc(#loc138)
        %21 = arith.cmpi eq, %5, %c32_i32 {acir.id = "v58", acir.source_type = "bool"} : i32 loc(#loc139)
        %22 = scf.if %21 -> (i32) {
          %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc140)
          scf.yield %c1_i32 : i32
        } else {
          scf.yield %c0_i32 : i32
        }
        scf.yield %c0_i32_3, %false_4, %22, %7, %11, %true_5 : i32, i1, i32, i32, i32, i1
      } else {
        %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc141)
        %19 = arith.cmpi eq, %2, %c3_i32 {acir.id = "v65", acir.source_type = "bool"} : i32 loc(#loc142)
        %20:6 = scf.if %19 -> (i32, i1, i32, i32, i32, i1) {
          %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc143)
          %21 = arith.cmpi eq, %4, %c2_i32 {acir.id = "v67", acir.source_type = "bool"} : i32 loc(#loc144)
          acir.check %21 {acir.id = "v68", acir.source_type = "void"} loc(#loc145)
          %c2_i32_8 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc146)
          %c0_i32_9 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc147)
          %c20_i32_10 = arith.constant {acir.source_type = "u32"} 20 : i32 loc(#loc148)
          %22 = func.call @ac_builtin_shr_u32(%1, %c20_i32_10) {acir.id = "v72", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc149)
          %23 = func.call @top.decode_stage_body.0.call.1.3.0.call.1.37(%22) {acir.id = "v73", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc150)
          %true_11 = arith.constant {acir.source_type = "bool"} true loc(#loc151)
          scf.yield %23, %true_11, %c2_i32_8, %7, %c0_i32_9, %true_5 : i32, i1, i32, i32, i32, i1
        } else {
          %c35_i32 = arith.constant {acir.source_type = "u32"} 35 : i32 loc(#loc152)
          %21 = arith.cmpi eq, %2, %c35_i32 {acir.id = "v76", acir.source_type = "bool"} : i32 loc(#loc153)
          %22 = scf.if %21 -> (i1) {
            %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc154)
            %29 = arith.cmpi eq, %4, %c2_i32 {acir.id = "v78", acir.source_type = "bool"} : i32 loc(#loc155)
            scf.yield %29 : i1
          } else {
            %false_15 = arith.constant {acir.source_type = "bool"} false
            scf.yield %false_15 : i1
          }
          acir.check %22 {acir.id = "v83", acir.source_type = "void"} loc(#loc156)
          %c3_i32_8 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc157)
          %c0_i32_9 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc158)
          %c25_i32_10 = arith.constant {acir.source_type = "u32"} 25 : i32 loc(#loc159)
          %23 = func.call @ac_builtin_shr_u32(%1, %c25_i32_10) {acir.id = "v87", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc160)
          %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32 loc(#loc161)
          %24 = func.call @ac_builtin_shl_u32(%23, %c5_i32) {acir.id = "v89", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc160)
          %c7_i32_11 = arith.constant {acir.source_type = "u32"} 7 : i32 loc(#loc162)
          %25 = func.call @ac_builtin_shr_u32(%1, %c7_i32_11) {acir.id = "v91", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc163)
          %c31_i32_12 = arith.constant {acir.source_type = "u32"} 31 : i32 loc(#loc164)
          %26 = arith.andi %25, %c31_i32_12 {acir.id = "v93", acir.source_type = "u32"} : i32 loc(#loc163)
          %27 = arith.ori %24, %26 {acir.id = "v94", acir.source_type = "u32"} : i32 loc(#loc160)
          %28 = func.call @top.decode_stage_body.0.call.1.3.0.call.1.37(%27) {acir.id = "v95", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc165)
          %true_13 = arith.constant {acir.source_type = "bool"} true loc(#loc166)
          %false_14 = arith.constant {acir.source_type = "bool"} false loc(#loc167)
          scf.yield %28, %true_13, %c3_i32_8, %c0_i32_9, %11, %false_14 : i32, i1, i32, i32, i32, i1
        }
        scf.yield %20#0, %20#1, %20#2, %20#3, %20#4, %20#5 : i32, i1, i32, i32, i32, i1
      }
      scf.yield %18#0, %18#1, %18#2, %18#3, %18#4, %18#5 : i32, i1, i32, i32, i32, i1
    }
    %14 = acir.extract %arg0 {acir.id = "v125", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc168)
    %15 = acir.aggregate(%14, %1, %13#2, %13#3, %9, %13#4, %13#0, %13#1, %13#5) {acir.id = "v126", acir.source_type = "src_isa_f0424bc8.Decoded"} : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc169)
    %true_6 = arith.constant {acir.source_type = "bool"} true
    %false_7 = arith.constant {acir.source_type = "bool"} false
    %16 = acir.unary %false_7 "not" {acir.source_type = "bool"} : i1 -> i1
    acir.check %16 {acir.source_type = "void"}
    return %15 : !acir.struct<"src_isa_f0424bc8.Decoded">
  }
  func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  acir.module @top attributes {acir.instance = {definition = "src_model_8294833e.Pipeline", name = "top", parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.if_id : !acir.struct<"src_isa_f0424bc8.Fetched"> {capacity_fn = "ac_init_top.if_id_cap", initial_fn = "ac_init_top.if_id", name = "top.if_id", type = "src_isa_f0424bc8.Fetched"}
    acir.queue @top.id_ex : !acir.struct<"src_isa_f0424bc8.Operands"> {capacity_fn = "ac_init_top.id_ex_cap", initial_fn = "ac_init_top.id_ex", name = "top.id_ex", type = "src_isa_f0424bc8.Operands"}
    acir.queue @top.ex_mem : !acir.struct<"src_isa_f0424bc8.Executed"> {capacity_fn = "ac_init_top.ex_mem_cap", initial_fn = "ac_init_top.ex_mem", name = "top.ex_mem", type = "src_isa_f0424bc8.Executed"}
    acir.queue @top.mem_wb : !acir.struct<"src_isa_f0424bc8.Retired"> {capacity_fn = "ac_init_top.mem_wb_cap", initial_fn = "ac_init_top.mem_wb", name = "top.mem_wb", type = "src_isa_f0424bc8.Retired"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [32]>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, [32]>
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
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.37(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      acir.module.call @top.fetch loc(#loc170)
      %0 = acir.ref @top::@top.if_id {acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
      %1 = acir.ref @top::@top.mem_wb {acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      acir.module.call @top.decode_stage loc(#loc171)
      %2 = acir.ref @top::@top.id_ex {acir.source_type = "queue<src_isa_f0424bc8.Operands>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
      acir.module.call @top.execute loc(#loc172)
      %3 = acir.ref @top::@top.ex_mem {acir.source_type = "queue<src_isa_f0424bc8.Executed>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
      acir.module.call @top.memory_stage loc(#loc173)
      %4 = acir.ref @top::@top.mem_wb {acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      acir.module.call @top.writeback loc(#loc174)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rf attributes {acir.instance = {definition = "src_model_8294833e.RegisterFile", name = "top.rf", parent = "top", rules = ["commit", "read", "ready", "reserve"]}} {
    acir.contract {relations = [], rules = ["commit", "read", "ready", "reserve"]}
    acir.cell @top.rf.registers : !acir.array<i32, [32]> {initial_fn = "ac_init_top.rf.registers", name = "top.rf.registers", type = "array<u32;32>"}
    acir.cell @top.rf.pending : !acir.array<i1, [32]> {initial_fn = "ac_init_top.rf.pending", name = "top.rf.pending", type = "array<bool;32>"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [32]>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, [32]>
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
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.37(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.decode_stage_body.0.call.3.7 : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 {acir.function = {contract = "Module:top.rf", definition = "ready", instance = "top.rf", interface = "Module:top.rf:ready", kind = "rule", name = "top.decode_stage_body.0.call.3.7", output_type = "bool", owner = "top.rf", params = [{id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], public = true, result = "bool", rule = "ready"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "bool"} : () -> i1
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool;32>>"} : !acir.cell<!acir.array<i1, [32]>>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "array<bool;32>", role = "observe"} : !acir.cell<!acir.array<i1, [32]>> -> !acir.array<i1, [32]> loc(#loc175)
      %3 = acir.extract %arg0 {acir.id = "v9", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc176)
      %4 = "acir.array.index"(%2, %3) {acir.id = "v10", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [32]>, i32) -> i1 loc(#loc175)
      %5 = acir.unary %4 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc177)
      %6 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool;32>>"} : !acir.cell<!acir.array<i1, [32]>>
      %7 = scf.if %5 -> (i1) {
        %11 = acir.cell.read %6 {acir.id = "v13", acir.source_type = "array<bool;32>", role = "observe"} : !acir.cell<!acir.array<i1, [32]>> -> !acir.array<i1, [32]> loc(#loc178)
        %12 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc179)
        %13 = "acir.array.index"(%11, %12) {acir.id = "v15", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [32]>, i32) -> i1 loc(#loc178)
        %14 = acir.unary %13 "not" {acir.id = "v16", acir.source_type = "bool"} : i1 -> i1 loc(#loc180)
        scf.yield %14 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %8 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool;32>>"} : !acir.cell<!acir.array<i1, [32]>>
      %9 = scf.if %7 -> (i1) {
        %11 = acir.extract %arg0 {acir.id = "v21", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc181)
        %12 = acir.unary %11 "not" {acir.id = "v22", acir.source_type = "bool"} : i1 -> i1 loc(#loc182)
        %13 = scf.if %12 -> (i1) {
          %true_2 = arith.constant {acir.source_type = "bool"} true
          scf.yield %true_2 : i1
        } else {
          %14 = acir.cell.read %8 {acir.id = "v25", acir.source_type = "array<bool;32>", role = "observe"} : !acir.cell<!acir.array<i1, [32]>> -> !acir.array<i1, [32]> loc(#loc183)
          %15 = acir.extract %arg0 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc184)
          %16 = "acir.array.index"(%14, %15) {acir.id = "v27", acir.source_type = "bool", signed_index = false} : (!acir.array<i1, [32]>, i32) -> i1 loc(#loc183)
          %17 = acir.unary %16 "not" {acir.id = "v28", acir.source_type = "bool"} : i1 -> i1 loc(#loc185)
          scf.yield %17 : i1
        }
        scf.yield %13 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %10 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %10 {acir.source_type = "void"}
      acir.return %9 : i1 {acir.source_type = "void"}
    }
    acir.rule @top.decode_stage_body.0.call.4.13.0.call.4.9 : (i32) -> i32 {acir.function = {contract = "Module:top.rf", definition = "read", instance = "top.rf", interface = "Module:top.rf:read", kind = "rule", name = "top.decode_stage_body.0.call.4.13.0.call.4.9", output_type = "u32", owner = "top.rf", params = [{id = "v1", name = "index", targets = [], type = "u32"}], public = true, result = "u32", rule = "read"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc186)
      %1 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc187)
      %2:3 = scf.if %1 -> (i1, i1, i32) {
        %c0_i32_0 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc188)
        %true_1 = arith.constant {acir.source_type = "bool"} true
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2, %true_1, %c0_i32_0 : i1, i1, i32
      } else {
        scf.yield %true, %false, %0 : i1, i1, i32
      }
      %3 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;32>>"} : !acir.cell<!acir.array<i32, [32]>>
      %4:3 = scf.if %2#0 -> (i1, i1, i32) {
        %6 = acir.cell.read %3 {acir.id = "v19", acir.source_type = "array<u32;32>", role = "observe"} : !acir.cell<!acir.array<i32, [32]>> -> !acir.array<i32, [32]> loc(#loc189)
        %7 = "acir.array.index"(%6, %arg0) {acir.id = "v20", acir.source_type = "u32", signed_index = false} : (!acir.array<i32, [32]>, i32) -> i32 loc(#loc189)
        %true_0 = arith.constant {acir.source_type = "bool"} true
        %false_1 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_1, %true_0, %7 : i1, i1, i32
      } else {
        scf.yield %2#0, %2#1, %2#2 : i1, i1, i32
      }
      %5 = acir.unary %4#0 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %5 {acir.source_type = "void"}
      acir.return %4#2 : i32 {acir.source_type = "void"}
    }
    acir.rule @top.decode_stage_body.0.call.4.13.0.call.5.17 : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> () {acir.function = {contract = "Module:top.rf", definition = "reserve", instance = "top.rf", interface = "Module:top.rf:reserve", kind = "rule", name = "top.decode_stage_body.0.call.4.13.0.call.5.17", output_type = "void", owner = "top.rf", params = [{id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], public = true, result = "void", rule = "reserve"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.extract %arg0 {acir.id = "v5", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc190)
      %1 = scf.if %0 -> (i1) {
        %5 = acir.extract %arg0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc191)
        %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc192)
        %6 = arith.cmpi ne, %5, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc191)
        scf.yield %6 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %2 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool;32>>"} : !acir.cell<!acir.array<i1, [32]>>
      %3 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool;32>>"} : !acir.cell<!acir.array<i1, [32]>>
      %4 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool;32>>"} : !acir.cell<!acir.array<i1, [32]>>
      scf.if %1 {
        %true_0 = arith.constant {acir.source_type = "bool"} true loc(#loc193)
        %5 = acir.cell.read %4 {acir.id = "v17", acir.source_type = "array<bool;32>", role = "observe"} : !acir.cell<!acir.array<i1, [32]>> -> !acir.array<i1, [32]> loc(#loc194)
        %6 = acir.extract %arg0 {acir.id = "v18", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc195)
        %7 = acir.update %5, %true_0[%6] {acir.id = "v19", acir.source_type = "array<bool;32>", path = [unit], signed_indices = [false]} : (!acir.array<i1, [32]>, i1, i32) -> !acir.array<i1, [32]> loc(#loc194)
        acir.cell.write %4, %7 {acir.id = "v20", acir.source_type = "void"} : !acir.cell<!acir.array<i1, [32]>>, !acir.array<i1, [32]> loc(#loc194)
      } else {
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.writeback_body.0.call.11.2.0.call.11.5 : (!acir.struct<"src_isa_f0424bc8.Retired">) -> () {acir.function = {contract = "Module:top.rf", definition = "commit", instance = "top.rf", interface = "Module:top.rf:commit", kind = "rule", name = "top.writeback_body.0.call.11.2.0.call.11.5", output_type = "void", owner = "top.rf", params = [{id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Retired"}], public = true, result = "void", rule = "commit"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Retired">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.extract %arg0 {acir.id = "v5", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc196)
      %1 = scf.if %0 -> (i1) {
        %8 = acir.extract %arg0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc197)
        %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc198)
        %9 = arith.cmpi ne, %8, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc197)
        scf.yield %9 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %2 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;32>>"} : !acir.cell<!acir.array<i32, [32]>>
      %3 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;32>>"} : !acir.cell<!acir.array<i32, [32]>>
      %4 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32;32>>"} : !acir.cell<!acir.array<i32, [32]>>
      %5 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool;32>>"} : !acir.cell<!acir.array<i1, [32]>>
      %6 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool;32>>"} : !acir.cell<!acir.array<i1, [32]>>
      %7 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool;32>>"} : !acir.cell<!acir.array<i1, [32]>>
      scf.if %1 {
        %8 = acir.extract %arg0 {acir.id = "v14", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc199)
        %9 = acir.cell.read %4 {acir.id = "v17", acir.source_type = "array<u32;32>", role = "observe"} : !acir.cell<!acir.array<i32, [32]>> -> !acir.array<i32, [32]> loc(#loc200)
        %10 = acir.extract %arg0 {acir.id = "v18", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc201)
        %11 = acir.update %9, %8[%10] {acir.id = "v19", acir.source_type = "array<u32;32>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [32]>, i32, i32) -> !acir.array<i32, [32]> loc(#loc200)
        acir.cell.write %4, %11 {acir.id = "v20", acir.source_type = "void"} : !acir.cell<!acir.array<i32, [32]>>, !acir.array<i32, [32]> loc(#loc200)
        %false = arith.constant {acir.source_type = "bool"} false loc(#loc202)
        %12 = acir.cell.read %7 {acir.id = "v25", acir.source_type = "array<bool;32>", role = "observe"} : !acir.cell<!acir.array<i1, [32]>> -> !acir.array<i1, [32]> loc(#loc203)
        %13 = acir.extract %arg0 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc204)
        %14 = acir.update %12, %false[%13] {acir.id = "v27", acir.source_type = "array<bool;32>", path = [unit], signed_indices = [false]} : (!acir.array<i1, [32]>, i1, i32) -> !acir.array<i1, [32]> loc(#loc203)
        acir.cell.write %7, %14 {acir.id = "v28", acir.source_type = "void"} : !acir.cell<!acir.array<i1, [32]>>, !acir.array<i1, [32]> loc(#loc203)
      } else {
      }
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.memory attributes {acir.instance = {definition = "src_model_8294833e.DataMemory", name = "top.memory", parent = "top", rules = ["load", "store"]}} {
    acir.contract {relations = [], rules = ["load", "store"]}
    acir.cell @top.memory.words : !acir.array<i32, [16]> {initial_fn = "ac_init_top.memory.words", name = "top.memory.words", type = "array<u32;16>"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [32]>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, [32]>
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
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.37(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.memory_stage_body.0.call.8.2.0.call.8.12 : (i32) -> i32 {acir.function = {contract = "Module:top.memory", definition = "load", instance = "top.memory", interface = "Module:top.memory:load", kind = "rule", name = "top.memory_stage_body.0.call.8.2.0.call.8.12", output_type = "u32", owner = "top.memory", params = [{id = "v1", name = "address", targets = [], type = "u32"}], public = true, result = "u32", rule = "load"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
      %false = arith.constant {acir.source_type = "bool"} false
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc205)
      %1 = arith.andi %arg0, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc206)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc207)
      %2 = arith.cmpi eq, %1, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc206)
      %3 = scf.if %2 -> (i1) {
        %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32 loc(#loc208)
        %9 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc209)
        scf.yield %9 : i1
      } else {
        %false_2 = arith.constant {acir.source_type = "bool"} false
        scf.yield %false_2 : i1
      }
      acir.check %3 {acir.id = "v17", acir.source_type = "void"} loc(#loc210)
      %4 = acir.ref @top.memory::@top.memory.words {acir.source_type = "cell<array<u32;16>>"} : !acir.cell<!acir.array<i32, [16]>>
      %5 = acir.cell.read %4 {acir.id = "v19", acir.source_type = "array<u32;16>", role = "observe"} : !acir.cell<!acir.array<i32, [16]>> -> !acir.array<i32, [16]> loc(#loc211)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc212)
      %6 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v21", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc213)
      %7 = "acir.array.index"(%5, %6) {acir.id = "v22", acir.source_type = "u32", signed_index = false} : (!acir.array<i32, [16]>, i32) -> i32 loc(#loc211)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %8 = acir.unary %false_1 "not" {acir.source_type = "bool"} : i1 -> i1
      acir.check %8 {acir.source_type = "void"}
      acir.return %7 : i32 {acir.source_type = "void"}
    }
    acir.rule @top.memory_stage_body.0.call.8.2.0.call.9.18 : (i32, i32) -> () {acir.function = {contract = "Module:top.memory", definition = "store", instance = "top.memory", interface = "Module:top.memory:store", kind = "rule", name = "top.memory_stage_body.0.call.8.2.0.call.9.18", output_type = "void", owner = "top.memory", params = [{id = "v1", name = "address", targets = [], type = "u32"}, {id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "store"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc214)
      %0 = arith.andi %arg0, %c3_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc215)
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc216)
      %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc215)
      %2 = scf.if %1 -> (i1) {
        %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32 loc(#loc217)
        %9 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc218)
        scf.yield %9 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      acir.check %2 {acir.id = "v16", acir.source_type = "void"} loc(#loc219)
      %3 = acir.ref @top.memory::@top.memory.words {acir.source_type = "cell<array<u32;16>>"} : !acir.cell<!acir.array<i32, [16]>>
      %4 = acir.ref @top.memory::@top.memory.words {acir.source_type = "cell<array<u32;16>>"} : !acir.cell<!acir.array<i32, [16]>>
      %5 = acir.ref @top.memory::@top.memory.words {acir.source_type = "cell<array<u32;16>>"} : !acir.cell<!acir.array<i32, [16]>>
      %6 = acir.cell.read %5 {acir.id = "v20", acir.source_type = "array<u32;16>", role = "observe"} : !acir.cell<!acir.array<i32, [16]>> -> !acir.array<i32, [16]> loc(#loc220)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc221)
      %7 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v22", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc222)
      %8 = acir.update %6, %arg1[%7] {acir.id = "v23", acir.source_type = "array<u32;16>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [16]>, i32, i32) -> !acir.array<i32, [16]> loc(#loc220)
      acir.cell.write %5, %8 {acir.id = "v24", acir.source_type = "void"} : !acir.cell<!acir.array<i32, [16]>>, !acir.array<i32, [16]> loc(#loc220)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.fetch attributes {acir.instance = {definition = "src_model_8294833e.Fetch", name = "top.fetch", parent = "top", rules = ["fetch"]}} {
    acir.contract {relations = [], rules = ["fetch"]}
    acir.cell @top.fetch.rom : !acir.array<i32, [6]> {initial_fn = "ac_init_top.fetch.rom", name = "top.fetch.rom", type = "array<u32;6>"}
    acir.cell @top.fetch.pc : i32 {initial_fn = "ac_init_top.fetch.pc", name = "top.fetch.pc", type = "u32"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [32]>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, [32]>
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
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.37(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.fetch_body.0.call.0.5 : (i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> {acir.function = {contract = "Module:top.fetch", definition = "fetch", instance = "top.fetch", interface = "Module:top.fetch:fetch", kind = "rule", name = "top.fetch_body.0.call.0.5", output_type = "queue<src_isa_f0424bc8.Fetched>", owner = "top.fetch", params = [{id = "v1", name = "current_pc", targets = [], type = "u32"}], public = true, result = "src_isa_f0424bc8.Fetched", rule = "fetch"}} body {
    ^bb0(%arg0: i32):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_isa_f0424bc8.Fetched"} : () -> !acir.struct<"src_isa_f0424bc8.Fetched">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.ref @top.fetch::@top.fetch.rom {acir.source_type = "cell<array<u32;6>>"} : !acir.cell<!acir.array<i32, [6]>>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "array<u32;6>", role = "observe"} : !acir.cell<!acir.array<i32, [6]>> -> !acir.array<i32, [6]> loc(#loc223)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc224)
      %3 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v10", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc225)
      %4 = "acir.array.index"(%2, %3) {acir.id = "v11", acir.source_type = "u32", signed_index = false} : (!acir.array<i32, [6]>, i32) -> i32 loc(#loc223)
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc226)
      %5 = arith.addi %arg0, %c4_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc227)
      %6 = acir.ref @top.fetch::@top.fetch.pc {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %6, %5 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc228)
      %7 = acir.aggregate(%arg0, %4) {acir.id = "v16", acir.source_type = "src_isa_f0424bc8.Fetched"} : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc229)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %8 = "acir.optional"(%7, %true_0) {acir.source_type = "src_isa_f0424bc8.Fetched"} : (!acir.struct<"src_isa_f0424bc8.Fetched">, i1) -> !acir.struct<"src_isa_f0424bc8.Fetched">
      acir.return %8 : !acir.struct<"src_isa_f0424bc8.Fetched"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.fetch_body", owner = "top.fetch", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top.fetch::@top.fetch.pc {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v3", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc230)
      %c24_i32 = arith.constant {acir.source_type = "u32"} 24 : i32 loc(#loc231)
      %2 = arith.cmpi ult, %1, %c24_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc232)
      %3 = acir.ref @top::@top.if_id {acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
      %4 = scf.if %2 -> (i1) {
        %5 = acir.rule.call @top.fetch::@top.fetch_body.0.call.0.5(%1) {acir.id = "v6", acir.source_type = "src_isa_f0424bc8.Fetched", contract = "Module:top.fetch", instance = "top.fetch", interface = "Module:top.fetch:fetch", output_binding = @top::@top.if_id, role = "rule", rule = "fetch"} : (i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc233)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        scf.yield %true : i1
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.decode_stage attributes {acir.instance = {definition = "src_model_8294833e.DecodeStage", name = "top.decode_stage", parent = "top", rules = ["read_operands"]}} {
    acir.contract {relations = [], rules = ["read_operands"]}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [32]>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, [32]>
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
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.37(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.decode_stage_body.0.call.4.13 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Decoded">) -> !acir.struct<"src_isa_f0424bc8.Operands"> {acir.function = {contract = "Module:top.decode_stage", definition = "read_operands", instance = "top.decode_stage", interface = "Module:top.decode_stage:read_operands", kind = "rule", name = "top.decode_stage_body.0.call.4.13", output_type = "queue<src_isa_f0424bc8.Operands>", owner = "top.decode_stage", params = [{id = "v1", name = "message", targets = ["top.if_id"], type = "queue<src_isa_f0424bc8.Fetched>"}, {id = "v2", name = "decoded", targets = [], type = "src_isa_f0424bc8.Decoded"}], public = true, result = "src_isa_f0424bc8.Operands", rule = "read_operands"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, %arg1: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_isa_f0424bc8.Operands"} : () -> !acir.struct<"src_isa_f0424bc8.Operands">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.queue.peek %arg0 {acir.id = "v8", acir.source_type = "src_isa_f0424bc8.Fetched", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc234)
      %2 = acir.extract %arg1 {acir.id = "v9", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc235)
      %3 = acir.rule.call @top.rf::@top.decode_stage_body.0.call.4.13.0.call.4.9(%2) {acir.id = "v10", acir.source_type = "u32", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:read", role = "rule", rule = "read"} : (i32) -> i32 loc(#loc236)
      %4 = acir.extract %arg1 {acir.id = "v11", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc237)
      %5 = acir.rule.call @top.rf::@top.decode_stage_body.0.call.4.13.0.call.4.9(%4) {acir.id = "v12", acir.source_type = "u32", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:read", role = "rule", rule = "read"} : (i32) -> i32 loc(#loc238)
      %6 = acir.extract %arg1 {acir.id = "v13", acir.source_type = "bool", field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc239)
      %7 = scf.if %6 -> (i32) {
        %15 = acir.extract %arg1 {acir.id = "v14", acir.source_type = "u32", field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc240)
        scf.yield %15 : i32
      } else {
        scf.yield %5 : i32
      }
      acir.rule.call @top.rf::@top.decode_stage_body.0.call.4.13.0.call.5.17(%arg1) {acir.id = "v18", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:reserve", role = "rule", rule = "reserve"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> () loc(#loc241)
      %8 = acir.extract %arg1 {acir.id = "v19", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc242)
      %9 = acir.extract %arg1 {acir.id = "v20", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc243)
      %10 = acir.extract %arg1 {acir.id = "v21", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc244)
      %11 = acir.extract %arg1 {acir.id = "v22", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc245)
      %12 = acir.extract %arg1 {acir.id = "v23", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc246)
      %13 = acir.aggregate(%8, %9, %10, %11, %3, %7, %5, %12) {acir.id = "v24", acir.source_type = "src_isa_f0424bc8.Operands"} : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc247)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %14 = "acir.optional"(%13, %true_0) {acir.source_type = "src_isa_f0424bc8.Operands"} : (!acir.struct<"src_isa_f0424bc8.Operands">, i1) -> !acir.struct<"src_isa_f0424bc8.Operands">
      acir.return %14 : !acir.struct<"src_isa_f0424bc8.Operands"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.decode_stage_body", owner = "top.decode_stage", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.if_id {acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
      %1 = acir.queue.peek %0 {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Fetched", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc248)
      %2 = func.call @top.decode_stage_body.0.call.1.3(%1) {acir.id = "v4", acir.source_type = "src_isa_f0424bc8.Decoded", role = "helper"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc249)
      %3 = acir.ref @top::@top.mem_wb {acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      %4 = acir.ref @top::@top.mem_wb {acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      %5 = acir.queue.empty %4 {acir.id = "v7", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> i1 loc(#loc250)
      %6 = scf.if %5 -> (i1) {
        %10 = acir.rule.call @top.rf::@top.decode_stage_body.0.call.3.7(%2) {acir.id = "v8", acir.source_type = "bool", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:ready", role = "rule", rule = "ready"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc251)
        scf.yield %10 : i1
      } else {
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      }
      %7 = acir.ref @top::@top.if_id {acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
      %8 = acir.ref @top::@top.id_ex {acir.source_type = "queue<src_isa_f0424bc8.Operands>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
      %9 = scf.if %6 -> (i1) {
        %10 = acir.rule.call @top.decode_stage::@top.decode_stage_body.0.call.4.13(%7, %2) {acir.id = "v14", acir.source_type = "src_isa_f0424bc8.Operands", contract = "Module:top.decode_stage", instance = "top.decode_stage", interface = "Module:top.decode_stage:read_operands", output_binding = @top::@top.id_ex, role = "rule", rule = "read_operands"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Decoded">) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc252)
        %false = arith.constant {acir.source_type = "bool"} false
        scf.yield %false : i1
      } else {
        scf.yield %true : i1
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.execute attributes {acir.instance = {definition = "src_model_8294833e.Execute", name = "top.execute", parent = "top", rules = ["execute"]}} {
    acir.contract {relations = [], rules = ["execute"]}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [32]>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, [32]>
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
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.37(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.execute_body.0.call.7.2 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) -> !acir.struct<"src_isa_f0424bc8.Executed"> {acir.function = {contract = "Module:top.execute", definition = "execute", instance = "top.execute", interface = "Module:top.execute:execute", kind = "rule", name = "top.execute_body.0.call.7.2", output_type = "queue<src_isa_f0424bc8.Executed>", owner = "top.execute", params = [{id = "v1", name = "message", targets = ["top.id_ex"], type = "queue<src_isa_f0424bc8.Operands>"}], public = true, result = "src_isa_f0424bc8.Executed", rule = "execute"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_isa_f0424bc8.Executed"} : () -> !acir.struct<"src_isa_f0424bc8.Executed">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.queue.peek %arg0 {acir.id = "v7", acir.source_type = "src_isa_f0424bc8.Operands", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc253)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc254)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc255)
      %3 = arith.cmpi eq, %2, %c1_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc254)
      %4 = scf.if %3 -> (i32) {
        %13 = acir.extract %1 {acir.id = "v11", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc256)
        %14 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc257)
        %15 = arith.subi %13, %14 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc256)
        scf.yield %15 : i32
      } else {
        %13 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc258)
        %14 = acir.extract %1 {acir.id = "v15", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc259)
        %15 = arith.addi %13, %14 {acir.id = "v16", acir.source_type = "u32"} : i32 loc(#loc258)
        scf.yield %15 : i32
      }
      %5 = acir.extract %1 {acir.id = "v20", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc260)
      %6 = acir.extract %1 {acir.id = "v21", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc261)
      %7 = acir.extract %1 {acir.id = "v22", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc262)
      %8 = acir.extract %1 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc263)
      %9 = acir.extract %1 {acir.id = "v24", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc264)
      %10 = acir.extract %1 {acir.id = "v25", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc265)
      %11 = acir.aggregate(%5, %6, %7, %8, %4, %9, %10) {acir.id = "v26", acir.source_type = "src_isa_f0424bc8.Executed"} : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc266)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %12 = "acir.optional"(%11, %true_0) {acir.source_type = "src_isa_f0424bc8.Executed"} : (!acir.struct<"src_isa_f0424bc8.Executed">, i1) -> !acir.struct<"src_isa_f0424bc8.Executed">
      acir.return %12 : !acir.struct<"src_isa_f0424bc8.Executed"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.execute_body", owner = "top.execute", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.id_ex {acir.source_type = "queue<src_isa_f0424bc8.Operands>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
      %1 = acir.rule.call @top.execute::@top.execute_body.0.call.7.2(%0) {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Executed", contract = "Module:top.execute", instance = "top.execute", interface = "Module:top.execute:execute", output_binding = @top::@top.ex_mem, role = "rule", rule = "execute"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc267)
      %2 = acir.ref @top::@top.ex_mem {acir.source_type = "queue<src_isa_f0424bc8.Executed>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
      %false = arith.constant {acir.source_type = "bool"} false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.memory_stage attributes {acir.instance = {definition = "src_model_8294833e.MemoryStage", name = "top.memory_stage", parent = "top", rules = ["access"]}} {
    acir.contract {relations = [], rules = ["access"]}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [32]>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, [32]>
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
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.37(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.memory_stage_body.0.call.8.2 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) -> !acir.struct<"src_isa_f0424bc8.Retired"> {acir.function = {contract = "Module:top.memory_stage", definition = "access", instance = "top.memory_stage", interface = "Module:top.memory_stage:access", kind = "rule", name = "top.memory_stage_body.0.call.8.2", output_type = "queue<src_isa_f0424bc8.Retired>", owner = "top.memory_stage", params = [{id = "v1", name = "message", targets = ["top.ex_mem"], type = "queue<src_isa_f0424bc8.Executed>"}], public = true, result = "src_isa_f0424bc8.Retired", rule = "access"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.aggregate() {acir.source_type = "src_isa_f0424bc8.Retired"} : () -> !acir.struct<"src_isa_f0424bc8.Retired">
      %false = arith.constant {acir.source_type = "bool"} false
      %1 = acir.queue.peek %arg0 {acir.id = "v7", acir.source_type = "src_isa_f0424bc8.Executed", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc268)
      %2 = acir.extract %1 {acir.id = "v8", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc269)
      %3 = acir.extract %1 {acir.id = "v9", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc270)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc271)
      %4 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc270)
      %5 = scf.if %4 -> (i32) {
        %12 = acir.extract %1 {acir.id = "v12", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc272)
        %13 = acir.rule.call @top.memory::@top.memory_stage_body.0.call.8.2.0.call.8.12(%12) {acir.id = "v13", acir.source_type = "u32", contract = "Module:top.memory", instance = "top.memory", interface = "Module:top.memory:load", role = "rule", rule = "load"} : (i32) -> i32 loc(#loc273)
        scf.yield %13 : i32
      } else {
        %12 = acir.extract %1 {acir.id = "v14", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc274)
        %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc275)
        %13 = arith.cmpi eq, %12, %c3_i32 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc274)
        scf.if %13 {
          %14 = acir.extract %1 {acir.id = "v17", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc276)
          %15 = acir.extract %1 {acir.id = "v18", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc277)
          acir.rule.call @top.memory::@top.memory_stage_body.0.call.8.2.0.call.9.18(%14, %15) {acir.id = "v19", acir.source_type = "void", contract = "Module:top.memory", instance = "top.memory", interface = "Module:top.memory:store", role = "rule", rule = "store"} : (i32, i32) -> () loc(#loc278)
        } else {
        }
        scf.yield %2 : i32
      }
      %6 = acir.extract %1 {acir.id = "v27", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc279)
      %7 = acir.extract %1 {acir.id = "v28", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc280)
      %8 = acir.extract %1 {acir.id = "v29", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc281)
      %9 = acir.extract %1 {acir.id = "v30", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc282)
      %10 = acir.aggregate(%6, %7, %8, %5, %9) {acir.id = "v31", acir.source_type = "src_isa_f0424bc8.Retired"} : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc283)
      %true_0 = arith.constant {acir.source_type = "bool"} true
      %false_1 = arith.constant {acir.source_type = "bool"} false
      %11 = "acir.optional"(%10, %true_0) {acir.source_type = "src_isa_f0424bc8.Retired"} : (!acir.struct<"src_isa_f0424bc8.Retired">, i1) -> !acir.struct<"src_isa_f0424bc8.Retired">
      acir.return %11 : !acir.struct<"src_isa_f0424bc8.Retired"> {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.memory_stage_body", owner = "top.memory_stage", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.ex_mem {acir.source_type = "queue<src_isa_f0424bc8.Executed>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
      %1 = acir.rule.call @top.memory_stage::@top.memory_stage_body.0.call.8.2(%0) {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Retired", contract = "Module:top.memory_stage", instance = "top.memory_stage", interface = "Module:top.memory_stage:access", output_binding = @top::@top.mem_wb, role = "rule", rule = "access"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc284)
      %2 = acir.ref @top::@top.mem_wb {acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      %false = arith.constant {acir.source_type = "bool"} false
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.writeback attributes {acir.instance = {definition = "src_model_8294833e.Writeback", name = "top.writeback", parent = "top", rules = ["commit"]}} {
    acir.contract {relations = [], rules = ["commit"]}
    acir.cell @top.writeback.retired : i32 {initial_fn = "ac_init_top.writeback.retired", name = "top.writeback.retired", type = "u32"}
    acir.cell @top.writeback.last_pc : i32 {initial_fn = "ac_init_top.writeback.last_pc", name = "top.writeback.last_pc", type = "u32"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, [32]>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, [32]>
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
    func.func private @top.decode_stage_body.0.call.1.3.0.call.1.37(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.3(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.writeback_body.0.call.11.2 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) -> () {acir.function = {contract = "Module:top.writeback", definition = "commit", instance = "top.writeback", interface = "Module:top.writeback:commit", kind = "rule", name = "top.writeback_body.0.call.11.2", output_type = "void", owner = "top.writeback", params = [{id = "v1", name = "message", targets = ["top.mem_wb"], type = "queue<src_isa_f0424bc8.Retired>"}], public = true, result = "void", rule = "commit"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>):
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "src_isa_f0424bc8.Retired", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc285)
      acir.rule.call @top.rf::@top.writeback_body.0.call.11.2.0.call.11.5(%0) {acir.id = "v6", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:commit", role = "rule", rule = "commit"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> () loc(#loc286)
      %1 = acir.ref @top.writeback::@top.writeback.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v8", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc287)
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc288)
      %3 = arith.addi %2, %c1_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc287)
      %4 = acir.ref @top.writeback::@top.writeback.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %4, %3 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc289)
      %5 = acir.extract %0 {acir.id = "v13", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc290)
      %6 = acir.ref @top.writeback::@top.writeback.last_pc {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %6, %5 {acir.id = "v15", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc291)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.writeback_body", owner = "top.writeback", params = [], result = "void"}} {
      %true = arith.constant {acir.source_type = "bool"} true
      %0 = acir.ref @top::@top.mem_wb {acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      acir.rule.call @top.writeback::@top.writeback_body.0.call.11.2(%0) {acir.id = "v3", acir.source_type = "void", contract = "Module:top.writeback", instance = "top.writeback", interface = "Module:top.writeback:commit", role = "rule", rule = "commit"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) -> () loc(#loc292)
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:69)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:72)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:75)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:78)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:81)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:84)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:87)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:90)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:93)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:96)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:99)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:102)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:105)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:108)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:111)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:114)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:117)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:120)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:123)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:126)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:129)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:132)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:135)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:138)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:141)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:144)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:147)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:150)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:153)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:156)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:159)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:162)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:68)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:65)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:72)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:79)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:86)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:93)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:100)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:107)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:114)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:121)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:128)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:135)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:142)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:149)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:156)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:163)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:170)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:177)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:184)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:191)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:198)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:205)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:212)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:219)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:226)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:233)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:240)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:247)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:254)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:261)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:268)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:275)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:282)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:64)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:65)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:68)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:71)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:74)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:77)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:80)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:83)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:86)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:89)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:92)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:95)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:98)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:101)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:104)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:107)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:110)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:64)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:62)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:71)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:80)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:89)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:98)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:104)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:61)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":58:38)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":130:43)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":131:43)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:21)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:13)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:29)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:12)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":53:12)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:21)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:14)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:22)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:14)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:27)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":56:22)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":56:14)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:18)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:10)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:22)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:19)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:11)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:24)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":59:19)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":59:11)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":59:24)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":60:12)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":61:17)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":62:21)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":63:14)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:18)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:8)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:26)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:16)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:9)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":66:15)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:36)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:28)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:21)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":68:25)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":69:20)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":69:10)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:26)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:16)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:43)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:33)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:58)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:48)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:9)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":71:22)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":71:12)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":72:20)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:20)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:10)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:26)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:16)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:9)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":75:16)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":76:15)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:36)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:28)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:21)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":78:25)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:26)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:16)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:43)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:33)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:9)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":81:16)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":82:14)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:36)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:28)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:42)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:54)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:46)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:58)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:21)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":84:25)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":85:18)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:20)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:12)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":160:17)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":161:17)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":162:18)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":163:18)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":164:9)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:20)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:28)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:16)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:47)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:55)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:43)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:75)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:71)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:94)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:102)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:90)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:21)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:12)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":23:20)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:16)
#loc190 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:12)
#loc191 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:28)
#loc192 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:39)
#loc193 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:32)
#loc194 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:13)
#loc195 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:21)
#loc196 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:12)
#loc197 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:28)
#loc198 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:39)
#loc199 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:34)
#loc200 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:13)
#loc201 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:23)
#loc202 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:32)
#loc203 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:13)
#loc204 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:21)
#loc205 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:26)
#loc206 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:16)
#loc207 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:31)
#loc208 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:47)
#loc209 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:37)
#loc210 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:9)
#loc211 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:16)
#loc212 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:33)
#loc213 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:22)
#loc214 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:26)
#loc215 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:16)
#loc216 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:31)
#loc217 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:47)
#loc218 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:37)
#loc219 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:9)
#loc220 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:9)
#loc221 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:26)
#loc222 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:15)
#loc223 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:16)
#loc224 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:34)
#loc225 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:20)
#loc226 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:27)
#loc227 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:14)
#loc228 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:9)
#loc229 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:16)
#loc230 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":67:22)
#loc231 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":68:25)
#loc232 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":68:12)
#loc233 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:20)
#loc234 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":79:20)
#loc235 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":80:23)
#loc236 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":80:15)
#loc237 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:31)
#loc238 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:23)
#loc239 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:36)
#loc240 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:15)
#loc241 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:9)
#loc242 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:25)
#loc243 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:37)
#loc244 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:51)
#loc245 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:65)
#loc246 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:100)
#loc247 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:16)
#loc248 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":87:23)
#loc249 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":87:16)
#loc250 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:19)
#loc251 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:42)
#loc252 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:20)
#loc253 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":100:16)
#loc254 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:41)
#loc255 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:54)
#loc256 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:18)
#loc257 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:29)
#loc258 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:63)
#loc259 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:74)
#loc260 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:25)
#loc261 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:34)
#loc262 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:45)
#loc263 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:56)
#loc264 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:73)
#loc265 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:91)
#loc266 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:16)
#loc267 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:16)
#loc268 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":115:16)
#loc269 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":116:18)
#loc270 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":117:12)
#loc271 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":117:25)
#loc272 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":118:34)
#loc273 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":118:22)
#loc274 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":119:14)
#loc275 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":119:27)
#loc276 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:26)
#loc277 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:38)
#loc278 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:13)
#loc279 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:24)
#loc280 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:33)
#loc281 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:44)
#loc282 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:61)
#loc283 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:16)
#loc284 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:16)
#loc285 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":135:16)
#loc286 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":136:9)
#loc287 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:19)
#loc288 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:29)
#loc289 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:9)
#loc290 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:19)
#loc291 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:9)
#loc292 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:9)

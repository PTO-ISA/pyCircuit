module attributes {acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, exports = {retirement = "retirement"}, format_version = 7 : i64, top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}, {default = 0 : i64, name = "immediate", type = "u32"}, {default = false, name = "immediate_rhs", type = "bool"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "lhs", type = "u32"}, {default = 0 : i64, name = "rhs", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = false, name = "writes", type = "bool"}]}}} {
  func.func @ac_init_rf.registers() -> !acir.array<i32, 32> attributes {acir.function = {kind = "helper", name = "ac_init_rf.registers", params = [], result = "array<u32,32>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v33", acir.source_type = "array<u32,32>"} : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 32> loc(#loc1)
    return %0 : !acir.array<i32, 32>
  }
  func.func @ac_init_rf.pending() -> !acir.array<i1, 32> attributes {acir.function = {kind = "helper", name = "ac_init_rf.pending", params = [], result = "array<bool,32>"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.aggregate(%false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false) {acir.id = "v67", acir.source_type = "array<bool,32>"} : (i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1) -> !acir.array<i1, 32> loc(#loc2)
    return %0 : !acir.array<i1, 32>
  }
  func.func @ac_init_memory.words() -> !acir.array<i32, 16> attributes {acir.function = {kind = "helper", name = "ac_init_memory.words", params = [], result = "array<u32,16>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v85", acir.source_type = "array<u32,16>"} : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 16> loc(#loc3)
    return %0 : !acir.array<i32, 16>
  }
  func.func @ac_init_retirement_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_retirement_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">> attributes {acir.function = {kind = "helper", name = "ac_init_retirement", params = [], result = "vector<src_isa_f0424bc8.Retired>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Retired>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
  }
  func.func @ac_init_fetch.rom() -> !acir.array<i32, 6> attributes {acir.function = {kind = "helper", name = "ac_init_fetch.rom", params = [], result = "array<u32,6>"}} {
    %c7340179_i32 = arith.constant {acir.source_type = "u32"} 7340179 : i32 loc(#loc4)
    %c5243155_i32 = arith.constant {acir.source_type = "u32"} 5243155 : i32 loc(#loc5)
    %c2130355_i32 = arith.constant {acir.source_type = "u32"} 2130355 : i32 loc(#loc6)
    %c3153955_i32 = arith.constant {acir.source_type = "u32"} 3153955 : i32 loc(#loc7)
    %c8707_i32 = arith.constant {acir.source_type = "u32"} 8707 : i32 loc(#loc8)
    %c1075970739_i32 = arith.constant {acir.source_type = "u32"} 1075970739 : i32 loc(#loc9)
    %0 = acir.aggregate(%c7340179_i32, %c5243155_i32, %c2130355_i32, %c3153955_i32, %c8707_i32, %c1075970739_i32) {acir.id = "v95", acir.source_type = "array<u32,6>"} : (i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 6> loc(#loc10)
    return %0 : !acir.array<i32, 6>
  }
  func.func @ac_init_fetch.pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_fetch.pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc11)
    return %c0_i32 : i32
  }
  func.func @ac_init_fetch.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_fetch.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">> attributes {acir.function = {kind = "helper", name = "ac_init_fetch.out", params = [], result = "vector<src_isa_f0424bc8.Fetched>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Fetched>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
  }
  func.func @ac_init_decode_stage.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_decode_stage.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">> attributes {acir.function = {kind = "helper", name = "ac_init_decode_stage.out", params = [], result = "vector<src_isa_f0424bc8.Operands>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Operands>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
  }
  func.func @ac_init_execute.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_execute.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">> attributes {acir.function = {kind = "helper", name = "ac_init_execute.out", params = [], result = "vector<src_isa_f0424bc8.Executed>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Executed>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
  }
  func.func @ac_init_writeback.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_writeback.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc12)
    return %c0_i32 : i32
  }
  func.func @ac_init_writeback.last_pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_writeback.last_pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc13)
    return %c0_i32 : i32
  }
  func.func @decode_stage_Work.0.call.0.2.0.call.0.35(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "decode_stage_Work.0.call.0.2.0.call.0.35", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc14)
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc15)
    return %1 : i32 loc(#loc16)
  }
  func.func @decode_stage_Work.0.call.0.2.0.call.1.85(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "decode_stage_Work.0.call.0.2.0.call.1.85", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc14)
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc15)
    return %1 : i32 loc(#loc16)
  }
  func.func @decode_stage_Work.0.call.0.2.0.call.2.108(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "decode_stage_Work.0.call.0.2.0.call.2.108", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc14)
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc15)
    return %1 : i32 loc(#loc16)
  }
  func.func @decode_stage_Work.0.call.0.2(%arg0: !acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded"> attributes {acir.function = {definition = "src_isa_f0424bc8.decode", kind = "helper", name = "decode_stage_Work.0.call.0.2", params = [{consume = false, id = "v1", name = "packet", targets = [], type = "src_isa_f0424bc8.Fetched"}], result = "src_isa_f0424bc8.Decoded"}} {
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %c35_i32 = arith.constant {acir.source_type = "u32"} 35 : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c32_i32 = arith.constant {acir.source_type = "u32"} 32 : i32
    %c51_i32 = arith.constant {acir.source_type = "u32"} 51 : i32
    %c19_i32 = arith.constant {acir.source_type = "u32"} 19 : i32 loc(#loc17)
    %true = arith.constant {acir.source_type = "bool"} true
    %false = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c20_i32 = arith.constant {acir.source_type = "u32"} 20 : i32
    %c15_i32 = arith.constant {acir.source_type = "u32"} 15 : i32 loc(#loc18)
    %c31_i32 = arith.constant {acir.source_type = "u32"} 31 : i32
    %c25_i32 = arith.constant {acir.source_type = "u32"} 25 : i32
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    %c12_i32 = arith.constant {acir.source_type = "u32"} 12 : i32 loc(#loc19)
    %c127_i32 = arith.constant {acir.source_type = "u32"} 127 : i32 loc(#loc20)
    %0 = acir.extract %arg0 {acir.id = "v2", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc21)
    %1 = arith.andi %0, %c127_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc22)
    %2 = call @ac_builtin_shr_u32(%0, %c12_i32) {acir.id = "v6", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc23)
    %3 = arith.andi %2, %c7_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc24)
    %4 = call @ac_builtin_shr_u32(%0, %c25_i32) {acir.id = "v10", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc25)
    %5 = call @ac_builtin_shr_u32(%0, %c7_i32) {acir.id = "v12", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc26)
    %6 = arith.andi %5, %c31_i32 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc27)
    %7 = call @ac_builtin_shr_u32(%0, %c15_i32) {acir.id = "v16", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc28)
    %8 = arith.andi %7, %c31_i32 {acir.id = "v18", acir.source_type = "u32"} : i32 loc(#loc29)
    %9 = call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v20", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc30)
    %10 = arith.andi %9, %c31_i32 {acir.id = "v22", acir.source_type = "u32"} : i32 loc(#loc31)
    %11 = arith.cmpi eq, %1, %c19_i32 {acir.id = "v28", acir.source_type = "bool"} : i32 loc(#loc32)
    %12:6 = scf.if %11 -> (i32, i1, i32, i32, i32, i1) {
      %15 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v31", acir.source_type = "bool"} : i32 loc(#loc33)
      acir.check %15 {acir.id = "v32", acir.source_type = "void"} loc(#loc34)
      %16 = func.call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v35", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc35)
      %17 = func.call @decode_stage_Work.0.call.0.2.0.call.0.35(%16) {acir.id = "v36", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc36)
      scf.yield %17, %true, %c0_i32, %6, %c0_i32, %true : i32, i1, i32, i32, i32, i1
    } else {
      %15 = arith.cmpi eq, %1, %c51_i32 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc37)
      %16 = arith.cmpi ne, %1, %c51_i32 : i32 loc(#loc37)
      %17:5 = scf.if %15 -> (i32, i32, i32, i32, i1) {
        %18 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v42", acir.source_type = "bool"} : i32 loc(#loc38)
        %19 = scf.if %18 -> (i1) {
          %22 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v45", acir.source_type = "bool"} : i32 loc(#loc39)
          %23 = scf.if %22 -> (i1) {
            scf.yield %true : i1
          } else {
            %24 = arith.cmpi eq, %4, %c32_i32 {acir.id = "v49", acir.source_type = "bool"} : i32 loc(#loc40)
            scf.yield %24 : i1
          }
          scf.yield %23 : i1
        } else {
          scf.yield %false : i1
        }
        acir.check %19 {acir.id = "v57", acir.source_type = "void"} loc(#loc41)
        %20 = arith.cmpi eq, %4, %c32_i32 {acir.id = "v59", acir.source_type = "bool"} : i32 loc(#loc42)
        %21 = arith.extui %20 : i1 to i32
        scf.yield %c0_i32, %21, %6, %10, %true : i32, i32, i32, i32, i1
      } else {
        %18 = arith.cmpi eq, %1, %c3_i32 {acir.id = "v77", acir.source_type = "bool"} : i32 loc(#loc43)
        %19 = arith.select %18, %c2_i32, %c3_i32 : i32
        %20 = arith.select %18, %6, %c0_i32 : i32
        %21 = arith.select %18, %c0_i32, %10 : i32
        %22 = scf.if %18 -> (i32) {
          %23 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v80", acir.source_type = "bool"} : i32 loc(#loc44)
          acir.check %23 {acir.id = "v81", acir.source_type = "void"} loc(#loc45)
          %24 = func.call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v85", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc46)
          %25 = func.call @decode_stage_Work.0.call.0.2.0.call.1.85(%24) {acir.id = "v86", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc47)
          scf.yield %25 : i32
        } else {
          %23 = arith.cmpi eq, %1, %c35_i32 {acir.id = "v89", acir.source_type = "bool"} : i32 loc(#loc48)
          %24 = scf.if %23 -> (i1) {
            %31 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v92", acir.source_type = "bool"} : i32 loc(#loc49)
            scf.yield %31 : i1
          } else {
            scf.yield %false : i1
          }
          acir.check %24 {acir.id = "v97", acir.source_type = "void"} loc(#loc50)
          %25 = func.call @ac_builtin_shr_u32(%0, %c25_i32) {acir.id = "v101", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc51)
          %26 = func.call @ac_builtin_shl_u32(%25, %c5_i32) {acir.id = "v103", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc52)
          %27 = func.call @ac_builtin_shr_u32(%0, %c7_i32) {acir.id = "v105", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc53)
          %28 = arith.andi %27, %c31_i32 {acir.id = "v107", acir.source_type = "u32"} : i32 loc(#loc54)
          %29 = arith.ori %26, %28 {acir.id = "v108", acir.source_type = "u32"} : i32 loc(#loc55)
          %30 = func.call @decode_stage_Work.0.call.0.2.0.call.2.108(%29) {acir.id = "v109", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc56)
          scf.yield %30 : i32
        }
        scf.yield %22, %19, %20, %21, %18 : i32, i32, i32, i32, i1
      }
      scf.yield %17#0, %16, %17#1, %17#2, %17#3, %17#4 : i32, i1, i32, i32, i32, i1
    }
    %13 = acir.extract %arg0 {acir.id = "v154", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc57)
    %14 = acir.aggregate(%13, %0, %12#2, %12#3, %8, %12#4, %12#0, %12#1, %12#5) {acir.id = "v155", acir.source_type = "src_isa_f0424bc8.Decoded"} : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc58)
    return %14 : !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc59)
  }
  func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  acir.module @rf attributes {acir.instance = {contract = "Module:rf", definition = "src_model_8294833e.RegisterFile", inputs = {}, interfaces = {commit = "Module:rf:commit", read = "Module:rf:read", ready = "Module:rf:ready", reserve = "Module:rf:reserve"}, members = {pending = {resources = ["rf.pending"], type = "cell<array<bool,32>>", value}, registers = {resources = ["rf.registers"], type = "cell<array<u32,32>>", value}}, methods = ["commit", "read", "ready", "reserve"], name = "rf", outputs = {resources = ["rf.registers"], type = "cell<array<u32,32>>", value}, parent = "top"}} {
    acir.interface @commit {implementations = [@rf::@writeback.commit.0.call.10.4, @rf::@ac_summary_rf.commit.0.call.14.1], orders = []}
    acir.interface @read {implementations = [@rf::@decode_stage.read_operands.0.call.5.6, @rf::@decode_stage.read_operands.0.call.6.8, @rf::@ac_summary_rf.read.0.call.12.1], orders = []}
    acir.interface @ready {implementations = [@rf::@decode_stage_Work.0.call.4.6, @rf::@ac_summary_rf.ready.0.call.11.1], orders = []}
    acir.interface @reserve {implementations = [@rf::@decode_stage.read_operands.0.call.7.15, @rf::@ac_summary_rf.reserve.0.call.13.1], orders = []}
    acir.cell @rf.registers : !acir.array<i32, 32> {initial_fn = "ac_init_rf.registers", name = "rf.registers", type = "array<u32,32>"}
    acir.cell @rf.pending : !acir.array<i1, 32> {initial_fn = "ac_init_rf.pending", name = "rf.pending", type = "array<bool,32>"}
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_retirement_cap() -> i32
    func.func private @ac_init_retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_fetch.pc() -> i32
    func.func private @ac_init_fetch.out_cap() -> i32
    func.func private @ac_init_fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_decode_stage.out_cap() -> i32
    func.func private @ac_init_decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_execute.out_cap() -> i32
    func.func private @ac_init_execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_writeback.retired() -> i32
    func.func private @ac_init_writeback.last_pc() -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.0.35(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.1.85(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.2.108(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.method @decode_stage_Work.0.call.4.6 : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 {acir.function = {contract = "Module:rf", definition = "ready", instance = "rf", interface = "Module:rf:ready", kind = "method", method = "ready", name = "decode_stage_Work.0.call.4.6", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "bool"}, interface_ref = @rf::@ready} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rf::@rf.pending {acir.id = "v4", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc61)
      %2 = acir.extract %arg0 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc62)
      %3 = acir.extract %1[%2] {acir.id = "v7", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc61)
      %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc63)
      %5 = scf.if %4 -> (i1) {
        %7 = acir.cell.read %0 {acir.id = "v10", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc64)
        %8 = acir.extract %arg0 {acir.id = "v11", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc65)
        %9 = acir.extract %7[%8] {acir.id = "v12", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc64)
        %10 = acir.unary %9 "not" {acir.id = "v13", acir.source_type = "bool"} : i1 -> i1 loc(#loc66)
        scf.yield %10 : i1
      } else {
        scf.yield %false : i1
      }
      %6 = scf.if %5 -> (i1) {
        %7 = acir.extract %arg0 {acir.id = "v19", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc67)
        %8 = acir.unary %7 "not" {acir.id = "v20", acir.source_type = "bool"} : i1 -> i1 loc(#loc68)
        %9 = scf.if %8 -> (i1) {
          scf.yield %true : i1
        } else {
          %10 = acir.cell.read %0 {acir.id = "v23", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc69)
          %11 = acir.extract %arg0 {acir.id = "v24", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc70)
          %12 = acir.extract %10[%11] {acir.id = "v25", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc69)
          %13 = acir.unary %12 "not" {acir.id = "v26", acir.source_type = "bool"} : i1 -> i1 loc(#loc71)
          scf.yield %13 : i1
        }
        scf.yield %9 : i1
      } else {
        scf.yield %false : i1
      }
      acir.return %6 : i1 {acir.id = "v34", acir.source_type = "void"} loc(#loc72)
    }
    acir.method @decode_stage.read_operands.0.call.5.6 : (i32) -> i32 {acir.function = {contract = "Module:rf", definition = "read", instance = "rf", interface = "Module:rf:read", kind = "method", method = "read", name = "decode_stage.read_operands.0.call.5.6", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "u32"}, interface_ref = @rf::@read} body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc73)
      %1 = acir.ref @rf::@rf.registers {acir.id = "v11", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>> loc(#loc74)
      cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      acir.return %c0_i32 : i32 {acir.id = "v8", acir.source_type = "void"} loc(#loc75)
    ^bb2(%2: i32):  // pred: ^bb0
      %3 = acir.cell.read %1 {acir.id = "v12", acir.source_type = "array<u32,32>", role = "observe"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc76)
      %4 = acir.extract %3[%2] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc76)
      acir.return %4 : i32 {acir.id = "v14", acir.source_type = "void"} loc(#loc77)
    }
    acir.method @decode_stage.read_operands.0.call.6.8 : (i32) -> i32 {acir.function = {contract = "Module:rf", definition = "read", instance = "rf", interface = "Module:rf:read", kind = "method", method = "read", name = "decode_stage.read_operands.0.call.6.8", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "u32"}, interface_ref = @rf::@read} body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc73)
      %1 = acir.ref @rf::@rf.registers {acir.id = "v11", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>> loc(#loc74)
      cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      acir.return %c0_i32 : i32 {acir.id = "v8", acir.source_type = "void"} loc(#loc75)
    ^bb2(%2: i32):  // pred: ^bb0
      %3 = acir.cell.read %1 {acir.id = "v12", acir.source_type = "array<u32,32>", role = "observe"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc76)
      %4 = acir.extract %3[%2] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc76)
      acir.return %4 : i32 {acir.id = "v14", acir.source_type = "void"} loc(#loc77)
    }
    acir.method @decode_stage.read_operands.0.call.7.15 : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> () {acir.function = {contract = "Module:rf", definition = "reserve", instance = "rf", interface = "Module:rf:reserve", kind = "method", method = "reserve", name = "decode_stage.read_operands.0.call.7.15", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "void"}, interface_ref = @rf::@reserve} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc78)
      %1 = acir.ref @rf::@rf.pending {acir.id = "v14", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
      %2 = scf.if %0 -> (i1) {
        %3 = acir.extract %arg0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc79)
        %4 = arith.cmpi ne, %3, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc79)
        scf.yield %4 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %2 {
        %3 = acir.extract %arg0 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc80)
        %4 = acir.cell.read %1 {acir.id = "v17", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc81)
        %5 = acir.update %4, %true[%3] {acir.id = "v18", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc81)
        acir.cell.write %1, %5 {acir.id = "v19", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32> loc(#loc81)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @writeback.commit.0.call.10.4 : (!acir.struct<"src_isa_f0424bc8.Retired">) -> () {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "writeback.commit.0.call.10.4", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Retired"}], result = "void"}, interface_ref = @rf::@commit} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Retired">):
      %false = arith.constant {acir.source_type = "bool"} false
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc82)
      %1 = acir.ref @rf::@rf.registers {acir.id = "v14", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>> loc(#loc74)
      %2 = acir.ref @rf::@rf.pending {acir.id = "v20", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
      %3 = scf.if %0 -> (i1) {
        %4 = acir.extract %arg0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc83)
        %5 = arith.cmpi ne, %4, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc83)
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %3 {
        %4 = acir.extract %arg0 {acir.id = "v15", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc84)
        %5 = acir.extract %arg0 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc85)
        %6 = acir.cell.read %1 {acir.id = "v17", acir.source_type = "array<u32,32>", role = "observe"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc86)
        %7 = acir.update %6, %4[%5] {acir.id = "v18", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32> loc(#loc86)
        acir.cell.write %1, %7 {acir.id = "v19", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 32>>, !acir.array<i32, 32> loc(#loc86)
        %8 = acir.extract %arg0 {acir.id = "v22", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc87)
        %9 = acir.cell.read %2 {acir.id = "v23", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc88)
        %10 = acir.update %9, %false[%8] {acir.id = "v24", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc88)
        acir.cell.write %2, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32> loc(#loc88)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rf.ready.0.call.11.1 : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 {acir.function = {contract = "Module:rf", definition = "ready", instance = "rf", interface = "Module:rf:ready", kind = "method", method = "ready", name = "ac_summary_rf.ready.0.call.11.1", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "bool", summary = true}, interface_ref = @rf::@ready} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @rf::@rf.pending {acir.id = "v4", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc61)
      %2 = acir.extract %arg0 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc62)
      %3 = acir.extract %1[%2] {acir.id = "v7", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc61)
      %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc63)
      %5 = scf.if %4 -> (i1) {
        %7 = acir.cell.read %0 {acir.id = "v10", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc64)
        %8 = acir.extract %arg0 {acir.id = "v11", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc65)
        %9 = acir.extract %7[%8] {acir.id = "v12", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc64)
        %10 = acir.unary %9 "not" {acir.id = "v13", acir.source_type = "bool"} : i1 -> i1 loc(#loc66)
        scf.yield %10 : i1
      } else {
        scf.yield %false : i1
      }
      %6 = scf.if %5 -> (i1) {
        %7 = acir.extract %arg0 {acir.id = "v19", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc67)
        %8 = acir.unary %7 "not" {acir.id = "v20", acir.source_type = "bool"} : i1 -> i1 loc(#loc68)
        %9 = scf.if %8 -> (i1) {
          scf.yield %true : i1
        } else {
          %10 = acir.cell.read %0 {acir.id = "v23", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc69)
          %11 = acir.extract %arg0 {acir.id = "v24", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc70)
          %12 = acir.extract %10[%11] {acir.id = "v25", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc69)
          %13 = acir.unary %12 "not" {acir.id = "v26", acir.source_type = "bool"} : i1 -> i1 loc(#loc71)
          scf.yield %13 : i1
        }
        scf.yield %9 : i1
      } else {
        scf.yield %false : i1
      }
      acir.return %6 : i1 {acir.id = "v34", acir.source_type = "void"} loc(#loc72)
    }
    acir.method @ac_summary_rf.read.0.call.12.1 : (i32) -> i32 {acir.function = {contract = "Module:rf", definition = "read", instance = "rf", interface = "Module:rf:read", kind = "method", method = "read", name = "ac_summary_rf.read.0.call.12.1", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], result = "u32", summary = true}, interface_ref = @rf::@read} body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc73)
      %1 = acir.ref @rf::@rf.registers {acir.id = "v11", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>> loc(#loc74)
      cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      acir.return %c0_i32 : i32 {acir.id = "v8", acir.source_type = "void"} loc(#loc75)
    ^bb2(%2: i32):  // pred: ^bb0
      %3 = acir.cell.read %1 {acir.id = "v12", acir.source_type = "array<u32,32>", role = "observe"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc76)
      %4 = acir.extract %3[%2] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc76)
      acir.return %4 : i32 {acir.id = "v14", acir.source_type = "void"} loc(#loc77)
    }
    acir.method @ac_summary_rf.reserve.0.call.13.1 : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> () {acir.function = {contract = "Module:rf", definition = "reserve", instance = "rf", interface = "Module:rf:reserve", kind = "method", method = "reserve", name = "ac_summary_rf.reserve.0.call.13.1", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "void", summary = true}, interface_ref = @rf::@reserve} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc78)
      %1 = acir.ref @rf::@rf.pending {acir.id = "v14", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
      %2 = scf.if %0 -> (i1) {
        %3 = acir.extract %arg0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc79)
        %4 = arith.cmpi ne, %3, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc79)
        scf.yield %4 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %2 {
        %3 = acir.extract %arg0 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc80)
        %4 = acir.cell.read %1 {acir.id = "v17", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc81)
        %5 = acir.update %4, %true[%3] {acir.id = "v18", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc81)
        acir.cell.write %1, %5 {acir.id = "v19", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32> loc(#loc81)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_rf.commit.0.call.14.1 : (!acir.struct<"src_isa_f0424bc8.Retired">) -> () {acir.function = {contract = "Module:rf", definition = "commit", instance = "rf", interface = "Module:rf:commit", kind = "method", method = "commit", name = "ac_summary_rf.commit.0.call.14.1", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Retired"}], result = "void", summary = true}, interface_ref = @rf::@commit} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Retired">):
      %false = arith.constant {acir.source_type = "bool"} false
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc82)
      %1 = acir.ref @rf::@rf.registers {acir.id = "v14", acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>> loc(#loc74)
      %2 = acir.ref @rf::@rf.pending {acir.id = "v20", acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>> loc(#loc60)
      %3 = scf.if %0 -> (i1) {
        %4 = acir.extract %arg0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc83)
        %5 = arith.cmpi ne, %4, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc83)
        scf.yield %5 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %3 {
        %4 = acir.extract %arg0 {acir.id = "v15", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc84)
        %5 = acir.extract %arg0 {acir.id = "v16", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc85)
        %6 = acir.cell.read %1 {acir.id = "v17", acir.source_type = "array<u32,32>", role = "observe"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc86)
        %7 = acir.update %6, %4[%5] {acir.id = "v18", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32> loc(#loc86)
        acir.cell.write %1, %7 {acir.id = "v19", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 32>>, !acir.array<i32, 32> loc(#loc86)
        %8 = acir.extract %arg0 {acir.id = "v22", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc87)
        %9 = acir.cell.read %2 {acir.id = "v23", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc88)
        %10 = acir.update %9, %false[%8] {acir.id = "v24", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc88)
        acir.cell.write %2, %10 {acir.id = "v25", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32> loc(#loc88)
      }
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @memory attributes {acir.instance = {contract = "Module:memory", definition = "src_model_8294833e.DataMemory", inputs = {}, interfaces = {load = "Module:memory:load", store = "Module:memory:store"}, members = {words = {resources = ["memory.words"], type = "cell<array<u32,16>>", value}}, methods = ["load", "store"], name = "memory", outputs = {resources = ["memory.words"], type = "cell<array<u32,16>>", value}, parent = "top"}} {
    acir.interface @load {implementations = [@memory::@memory_stage.access.0.call.8.10, @memory::@ac_summary_memory.load.0.call.15.1], orders = []}
    acir.interface @store {implementations = [@memory::@memory_stage.access.0.call.9.17, @memory::@ac_summary_memory.store.0.call.16.2], orders = []}
    acir.cell @memory.words : !acir.array<i32, 16> {initial_fn = "ac_init_memory.words", name = "memory.words", type = "array<u32,16>"}
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_retirement_cap() -> i32
    func.func private @ac_init_retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_fetch.pc() -> i32
    func.func private @ac_init_fetch.out_cap() -> i32
    func.func private @ac_init_fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_decode_stage.out_cap() -> i32
    func.func private @ac_init_decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_execute.out_cap() -> i32
    func.func private @ac_init_execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_writeback.retired() -> i32
    func.func private @ac_init_writeback.last_pc() -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.0.35(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.1.85(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.2.108(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.method @memory_stage.access.0.call.8.10 : (i32) -> i32 {acir.function = {contract = "Module:memory", definition = "load", instance = "memory", interface = "Module:memory:load", kind = "method", method = "load", name = "memory_stage.access.0.call.8.10", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}], result = "u32"}, interface_ref = @memory::@load} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc89)
      %false = arith.constant {acir.source_type = "bool"} false
      %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc90)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc91)
      %0 = arith.andi %arg0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc92)
      %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc93)
      %2 = acir.ref @memory::@memory.words {acir.id = "v16", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>> loc(#loc94)
      %3 = scf.if %1 -> (i1) {
        %7 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc95)
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      acir.check %3 {acir.id = "v15", acir.source_type = "void"} loc(#loc96)
      %4 = acir.cell.read %2 {acir.id = "v17", acir.source_type = "array<u32,16>", role = "observe"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc97)
      %5 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v19", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc98)
      %6 = acir.extract %4[%5] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32 loc(#loc97)
      acir.return %6 : i32 {acir.id = "v21", acir.source_type = "void"} loc(#loc99)
    }
    acir.method @memory_stage.access.0.call.9.17 : (i32, i32) -> () {acir.function = {contract = "Module:memory", definition = "store", instance = "memory", interface = "Module:memory:store", kind = "method", method = "store", name = "memory_stage.access.0.call.9.17", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void"}, interface_ref = @memory::@store} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc100)
      %false = arith.constant {acir.source_type = "bool"} false
      %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc101)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc102)
      %0 = arith.andi %arg0, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc103)
      %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc104)
      %2 = acir.ref @memory::@memory.words {acir.id = "v17", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>> loc(#loc94)
      %3 = scf.if %1 -> (i1) {
        %7 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc105)
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      acir.check %3 {acir.id = "v16", acir.source_type = "void"} loc(#loc106)
      %4 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v19", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc107)
      %5 = acir.cell.read %2 {acir.id = "v20", acir.source_type = "array<u32,16>", role = "observe"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc108)
      %6 = acir.update %5, %arg1[%4] {acir.id = "v21", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16> loc(#loc108)
      acir.cell.write %2, %6 {acir.id = "v22", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 16>>, !acir.array<i32, 16> loc(#loc108)
      acir.return {acir.source_type = "void"}
    }
    acir.method @ac_summary_memory.load.0.call.15.1 : (i32) -> i32 {acir.function = {contract = "Module:memory", definition = "load", instance = "memory", interface = "Module:memory:load", kind = "method", method = "load", name = "ac_summary_memory.load.0.call.15.1", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}], result = "u32", summary = true}, interface_ref = @memory::@load} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc89)
      %false = arith.constant {acir.source_type = "bool"} false
      %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc90)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc91)
      %0 = arith.andi %arg0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc92)
      %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc93)
      %2 = acir.ref @memory::@memory.words {acir.id = "v16", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>> loc(#loc94)
      %3 = scf.if %1 -> (i1) {
        %7 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc95)
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      acir.check %3 {acir.id = "v15", acir.source_type = "void"} loc(#loc96)
      %4 = acir.cell.read %2 {acir.id = "v17", acir.source_type = "array<u32,16>", role = "observe"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc97)
      %5 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v19", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc98)
      %6 = acir.extract %4[%5] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32 loc(#loc97)
      acir.return %6 : i32 {acir.id = "v21", acir.source_type = "void"} loc(#loc99)
    }
    acir.method @ac_summary_memory.store.0.call.16.2 : (i32, i32) -> () {acir.function = {contract = "Module:memory", definition = "store", instance = "memory", interface = "Module:memory:store", kind = "method", method = "store", name = "ac_summary_memory.store.0.call.16.2", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], result = "void", summary = true}, interface_ref = @memory::@store} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc100)
      %false = arith.constant {acir.source_type = "bool"} false
      %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc101)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc102)
      %0 = arith.andi %arg0, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc103)
      %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc104)
      %2 = acir.ref @memory::@memory.words {acir.id = "v17", acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>> loc(#loc94)
      %3 = scf.if %1 -> (i1) {
        %7 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc105)
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      acir.check %3 {acir.id = "v16", acir.source_type = "void"} loc(#loc106)
      %4 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v19", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc107)
      %5 = acir.cell.read %2 {acir.id = "v20", acir.source_type = "array<u32,16>", role = "observe"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc108)
      %6 = acir.update %5, %arg1[%4] {acir.id = "v21", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16> loc(#loc108)
      acir.cell.write %2, %6 {acir.id = "v22", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 16>>, !acir.array<i32, 16> loc(#loc108)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @fetch attributes {acir.instance = {contract = "Module:fetch", definition = "src_model_8294833e.Fetch", inputs = {}, interfaces = {}, members = {out = {resources = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>", value}, pc = {resources = ["fetch.pc"], type = "cell<u32>", value}, rom = {resources = ["fetch.rom"], type = "cell<array<u32,6>>", value}}, methods = [], name = "fetch", outputs = {resources = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>", value}, parent = "top"}} {
    acir.cell @fetch.rom : !acir.array<i32, 6> {initial_fn = "ac_init_fetch.rom", name = "fetch.rom", type = "array<u32,6>"}
    acir.cell @fetch.pc : i32 {initial_fn = "ac_init_fetch.pc", name = "fetch.pc", type = "u32"}
    acir.queue @fetch.out : !acir.struct<"src_isa_f0424bc8.Fetched"> {capacity_fn = "ac_init_fetch.out_cap", initial_fn = "ac_init_fetch.out", name = "fetch.out", type = "src_isa_f0424bc8.Fetched"}
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_retirement_cap() -> i32
    func.func private @ac_init_retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_fetch.pc() -> i32
    func.func private @ac_init_fetch.out_cap() -> i32
    func.func private @ac_init_fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_decode_stage.out_cap() -> i32
    func.func private @ac_init_decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_execute.out_cap() -> i32
    func.func private @ac_init_execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_writeback.retired() -> i32
    func.func private @ac_init_writeback.last_pc() -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.0.35(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.1.85(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.2.108(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @fetch.fetch : (i32) -> () {acir.function = {kind = "rule", name = "fetch.fetch", owner = "fetch", params = [{consume = false, id = "v1", name = "current_pc", targets = [], type = "u32"}], result = "void", rule = "fetch"}, output_types = [!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>]} body {
    ^bb0(%arg0: i32):
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc109)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc110)
      %0 = acir.ref @fetch::@fetch.rom {acir.id = "v4", acir.source_type = "cell<array<u32,6>>"} : !acir.cell<!acir.array<i32, 6>> loc(#loc111)
      %1 = acir.cell.read %0 {acir.id = "v5", acir.source_type = "array<u32,6>", role = "observe"} : !acir.cell<!acir.array<i32, 6>> -> !acir.array<i32, 6> loc(#loc112)
      %2 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v7", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc113)
      %3 = acir.extract %1[%2] {acir.id = "v8", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 6>, i32) -> i32 loc(#loc112)
      %4 = acir.ref @fetch::@fetch.pc {acir.id = "v9", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc114)
      %5 = arith.addi %arg0, %c4_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc115)
      acir.cell.write %4, %5 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc116)
      %6 = acir.aggregate(%arg0, %3) {acir.id = "v13", acir.source_type = "src_isa_f0424bc8.Fetched"} : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc117)
      acir.return %6 : !acir.struct<"src_isa_f0424bc8.Fetched"> {acir.id = "v14", acir.source_type = "void", slots = [0]} loc(#loc118)
    }
    acir.work attributes {acir.function = {kind = "work", name = "fetch_Work", owner = "fetch", params = [], result = "void"}} {
      %c24_i32 = arith.constant {acir.source_type = "u32"} 24 : i32 loc(#loc119)
      %0 = acir.ref @fetch::@fetch.pc {acir.id = "v1", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc114)
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc120)
      %2 = arith.cmpi ult, %1, %c24_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc121)
      scf.if %2 {
        acir.rule.call @fetch::@fetch.fetch(%1) {acir.id = "v6", acir.source_type = "void", output_bindings = [@fetch::@fetch.out]} : (i32) -> () loc(#loc122)
      }
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @decode_stage attributes {acir.instance = {contract = "Module:decode_stage", definition = "src_model_8294833e.DecodeStage", inputs = {retirement = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, rf = {instance = "rf"}, source = {resources = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>", value}}, interfaces = {}, members = {out = {resources = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>", value}, retirement = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, rf = {instance = "rf"}, source = {resources = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>", value}}, methods = [], name = "decode_stage", outputs = {resources = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>", value}, parent = "top"}} {
    acir.instance @rf = @rf
    acir.queue @decode_stage.out : !acir.struct<"src_isa_f0424bc8.Operands"> {capacity_fn = "ac_init_decode_stage.out_cap", initial_fn = "ac_init_decode_stage.out", name = "decode_stage.out", type = "src_isa_f0424bc8.Operands"}
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_retirement_cap() -> i32
    func.func private @ac_init_retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_fetch.pc() -> i32
    func.func private @ac_init_fetch.out_cap() -> i32
    func.func private @ac_init_fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_decode_stage.out_cap() -> i32
    func.func private @ac_init_decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_execute.out_cap() -> i32
    func.func private @ac_init_execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_writeback.retired() -> i32
    func.func private @ac_init_writeback.last_pc() -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.0.35(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.1.85(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.2.108(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @decode_stage.read_operands : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Decoded">) -> () {acir.function = {kind = "rule", name = "decode_stage.read_operands", owner = "decode_stage", params = [{consume = true, id = "v1", name = "message", targets = ["fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>"}, {consume = false, id = "v2", name = "decoded", targets = [], type = "src_isa_f0424bc8.Decoded"}], result = "void", rule = "read_operands"}, output_types = [!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>]} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, %arg1: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %0 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "src_isa_f0424bc8.Fetched", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc123)
      %1 = acir.extract %arg1 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc124)
      %2 = acir.method.call @rf::@decode_stage.read_operands.0.call.5.6(%1) {acir.id = "v7", acir.source_type = "u32", contract = "Module:rf", instance = "rf", interface = "Module:rf:read", method = "read", role = "method"} : (i32) -> i32 loc(#loc125)
      %3 = acir.extract %arg1 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc126)
      %4 = acir.method.call @rf::@decode_stage.read_operands.0.call.6.8(%3) {acir.id = "v9", acir.source_type = "u32", contract = "Module:rf", instance = "rf", interface = "Module:rf:read", method = "read", role = "method"} : (i32) -> i32 loc(#loc127)
      %5 = acir.extract %arg1 {acir.id = "v10", acir.source_type = "bool", field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc128)
      %6 = scf.if %5 -> (i32) {
        %13 = acir.extract %arg1 {acir.id = "v12", acir.source_type = "u32", field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc129)
        scf.yield %13 : i32
      } else {
        scf.yield %4 : i32
      }
      acir.method.call @rf::@decode_stage.read_operands.0.call.7.15(%arg1) {acir.id = "v16", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:reserve", method = "reserve", role = "method"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> () loc(#loc130)
      %7 = acir.extract %arg1 {acir.id = "v17", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc131)
      %8 = acir.extract %arg1 {acir.id = "v18", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc132)
      %9 = acir.extract %arg1 {acir.id = "v19", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc133)
      %10 = acir.extract %arg1 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc134)
      %11 = acir.extract %arg1 {acir.id = "v21", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc135)
      %12 = acir.aggregate(%7, %8, %9, %10, %2, %6, %4, %11) {acir.id = "v22", acir.source_type = "src_isa_f0424bc8.Operands"} : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc136)
      acir.return %12 : !acir.struct<"src_isa_f0424bc8.Operands"> {acir.id = "v23", acir.source_type = "void", slots = [0]} loc(#loc137)
    }
    acir.work attributes {acir.function = {kind = "work", name = "decode_stage_Work", owner = "decode_stage", params = [], result = "void"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @fetch::@fetch.out {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> loc(#loc138)
      %1 = acir.queue.peek %0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Fetched", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc139)
      %2 = func.call @decode_stage_Work.0.call.0.2(%1) {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Decoded", role = "helper"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc140)
      %3 = acir.ref @top::@retirement {acir.id = "v4", acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> loc(#loc141)
      %4 = acir.queue.empty %3 {acir.id = "v5", acir.source_type = "bool"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> i1 loc(#loc142)
      %5 = scf.if %4 -> (i1) {
        %6 = acir.method.call @rf::@decode_stage_Work.0.call.4.6(%2) {acir.id = "v7", acir.source_type = "bool", contract = "Module:rf", instance = "rf", interface = "Module:rf:ready", method = "ready", role = "method"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc143)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %5 {
        acir.rule.call @decode_stage::@decode_stage.read_operands(%0, %2) {acir.id = "v13", acir.source_type = "void", output_bindings = [@decode_stage::@decode_stage.out]} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Decoded">) -> () loc(#loc144)
      }
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @execute attributes {acir.instance = {contract = "Module:execute", definition = "src_model_8294833e.Execute", inputs = {source = {resources = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>", value}}, interfaces = {}, members = {out = {resources = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>", value}, source = {resources = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>", value}}, methods = [], name = "execute", outputs = {resources = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>", value}, parent = "top"}} {
    acir.queue @execute.out : !acir.struct<"src_isa_f0424bc8.Executed"> {capacity_fn = "ac_init_execute.out_cap", initial_fn = "ac_init_execute.out", name = "execute.out", type = "src_isa_f0424bc8.Executed"}
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_retirement_cap() -> i32
    func.func private @ac_init_retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_fetch.pc() -> i32
    func.func private @ac_init_fetch.out_cap() -> i32
    func.func private @ac_init_fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_decode_stage.out_cap() -> i32
    func.func private @ac_init_decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_execute.out_cap() -> i32
    func.func private @ac_init_execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_writeback.retired() -> i32
    func.func private @ac_init_writeback.last_pc() -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.0.35(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.1.85(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.2.108(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @execute.execute : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) -> () {acir.function = {kind = "rule", name = "execute.execute", owner = "execute", params = [{consume = true, id = "v1", name = "message", targets = ["decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>"}], result = "void", rule = "execute"}, output_types = [!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>]} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>):
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc145)
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_isa_f0424bc8.Operands", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc146)
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc147)
      %2 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc147)
      %3 = scf.if %2 -> (i32) {
        %11 = acir.extract %0 {acir.id = "v9", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc148)
        %12 = acir.extract %0 {acir.id = "v10", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc149)
        %13 = arith.subi %11, %12 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc148)
        scf.yield %13 : i32
      } else {
        %11 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc150)
        %12 = acir.extract %0 {acir.id = "v13", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc151)
        %13 = arith.addi %11, %12 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc150)
        scf.yield %13 : i32
      }
      %4 = acir.extract %0 {acir.id = "v18", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc152)
      %5 = acir.extract %0 {acir.id = "v19", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc153)
      %6 = acir.extract %0 {acir.id = "v20", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc154)
      %7 = acir.extract %0 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc155)
      %8 = acir.extract %0 {acir.id = "v22", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc156)
      %9 = acir.extract %0 {acir.id = "v23", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc157)
      %10 = acir.aggregate(%4, %5, %6, %7, %3, %8, %9) {acir.id = "v24", acir.source_type = "src_isa_f0424bc8.Executed"} : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc158)
      acir.return %10 : !acir.struct<"src_isa_f0424bc8.Executed"> {acir.id = "v25", acir.source_type = "void", slots = [0]} loc(#loc159)
    }
    acir.work attributes {acir.function = {kind = "work", name = "execute_Work", owner = "execute", params = [], result = "void"}} {
      %0 = acir.ref @decode_stage::@decode_stage.out {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Operands>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> loc(#loc160)
      acir.rule.call @execute::@execute.execute(%0) {acir.id = "v2", acir.source_type = "void", output_bindings = [@execute::@execute.out]} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) -> () loc(#loc161)
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @memory_stage attributes {acir.instance = {contract = "Module:memory_stage", definition = "src_model_8294833e.MemoryStage", inputs = {memory = {instance = "memory"}, out = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, source = {resources = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>", value}}, interfaces = {}, members = {memory = {instance = "memory"}, out = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, source = {resources = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>", value}}, methods = [], name = "memory_stage", outputs = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, parent = "top"}} {
    acir.instance @memory = @memory
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_retirement_cap() -> i32
    func.func private @ac_init_retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_fetch.pc() -> i32
    func.func private @ac_init_fetch.out_cap() -> i32
    func.func private @ac_init_fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_decode_stage.out_cap() -> i32
    func.func private @ac_init_decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_execute.out_cap() -> i32
    func.func private @ac_init_execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_writeback.retired() -> i32
    func.func private @ac_init_writeback.last_pc() -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.0.35(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.1.85(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.2.108(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @memory_stage.access : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) -> () {acir.function = {kind = "rule", name = "memory_stage.access", owner = "memory_stage", params = [{consume = true, id = "v1", name = "message", targets = ["execute.out"], type = "queue<src_isa_f0424bc8.Executed>"}], result = "void", rule = "access"}, output_types = [!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>]} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>):
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc162)
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_isa_f0424bc8.Executed", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc163)
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc164)
      %2 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc165)
      %3 = arith.cmpi eq, %2, %c2_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc165)
      %4 = scf.if %3 -> (i32) {
        %10 = acir.extract %0 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc166)
        %11 = acir.method.call @memory::@memory_stage.access.0.call.8.10(%10) {acir.id = "v11", acir.source_type = "u32", contract = "Module:memory", instance = "memory", interface = "Module:memory:load", method = "load", role = "method"} : (i32) -> i32 loc(#loc167)
        scf.yield %11 : i32
      } else {
        %10 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc168)
        %11 = arith.cmpi eq, %10, %c3_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc168)
        scf.if %11 {
          %12 = acir.extract %0 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc169)
          %13 = acir.extract %0 {acir.id = "v17", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc170)
          acir.method.call @memory::@memory_stage.access.0.call.9.17(%12, %13) {acir.id = "v18", acir.source_type = "void", contract = "Module:memory", instance = "memory", interface = "Module:memory:store", method = "store", role = "method"} : (i32, i32) -> () loc(#loc171)
        }
        scf.yield %1 : i32
      }
      %5 = acir.extract %0 {acir.id = "v29", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc172)
      %6 = acir.extract %0 {acir.id = "v30", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc173)
      %7 = acir.extract %0 {acir.id = "v31", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc174)
      %8 = acir.extract %0 {acir.id = "v32", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc175)
      %9 = acir.aggregate(%5, %6, %7, %4, %8) {acir.id = "v33", acir.source_type = "src_isa_f0424bc8.Retired"} : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc176)
      acir.return %9 : !acir.struct<"src_isa_f0424bc8.Retired"> {acir.id = "v34", acir.source_type = "void", slots = [0]} loc(#loc177)
    }
    acir.work attributes {acir.function = {kind = "work", name = "memory_stage_Work", owner = "memory_stage", params = [], result = "void"}} {
      %0 = acir.ref @execute::@execute.out {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Executed>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> loc(#loc178)
      acir.rule.call @memory_stage::@memory_stage.access(%0) {acir.id = "v2", acir.source_type = "void", output_bindings = [@top::@retirement]} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) -> () loc(#loc179)
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @writeback attributes {acir.instance = {contract = "Module:writeback", definition = "src_model_8294833e.Writeback", inputs = {rf = {instance = "rf"}, source = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}}, interfaces = {}, members = {last_pc = {resources = ["writeback.last_pc"], type = "cell<u32>", value}, retired = {resources = ["writeback.retired"], type = "cell<u32>", value}, rf = {instance = "rf"}, source = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}}, methods = [], name = "writeback", outputs = [{resources = ["writeback.retired"], type = "cell<u32>", value}, {resources = ["writeback.last_pc"], type = "cell<u32>", value}], parent = "top"}} {
    acir.instance @rf = @rf
    acir.cell @writeback.retired : i32 {initial_fn = "ac_init_writeback.retired", name = "writeback.retired", type = "u32"}
    acir.cell @writeback.last_pc : i32 {initial_fn = "ac_init_writeback.last_pc", name = "writeback.last_pc", type = "u32"}
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_retirement_cap() -> i32
    func.func private @ac_init_retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_fetch.pc() -> i32
    func.func private @ac_init_fetch.out_cap() -> i32
    func.func private @ac_init_fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_decode_stage.out_cap() -> i32
    func.func private @ac_init_decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_execute.out_cap() -> i32
    func.func private @ac_init_execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_writeback.retired() -> i32
    func.func private @ac_init_writeback.last_pc() -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.0.35(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.1.85(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.2.108(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @writeback.commit : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) -> () {acir.function = {kind = "rule", name = "writeback.commit", owner = "writeback", params = [{consume = true, id = "v1", name = "message", targets = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>"}], result = "void", rule = "commit"}, output_types = []} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>):
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc180)
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_isa_f0424bc8.Retired", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc181)
      acir.method.call @rf::@writeback.commit.0.call.10.4(%0) {acir.id = "v5", acir.source_type = "void", contract = "Module:rf", instance = "rf", interface = "Module:rf:commit", method = "commit", role = "method"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> () loc(#loc182)
      %1 = acir.ref @writeback::@writeback.retired {acir.id = "v6", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc183)
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc184)
      %3 = arith.addi %2, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc184)
      acir.cell.write %1, %3 {acir.id = "v10", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc185)
      %4 = acir.ref @writeback::@writeback.last_pc {acir.id = "v11", acir.source_type = "cell<u32>"} : !acir.cell<i32> loc(#loc186)
      %5 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc187)
      acir.cell.write %4, %5 {acir.id = "v13", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc188)
      acir.return {acir.source_type = "void", slots = []}
    }
    acir.work attributes {acir.function = {kind = "work", name = "writeback_Work", owner = "writeback", params = [], result = "void"}} {
      %0 = acir.ref @top::@retirement {acir.id = "v1", acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> loc(#loc141)
      acir.rule.call @writeback::@writeback.commit(%0) {acir.id = "v2", acir.source_type = "void", output_bindings = []} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) -> () loc(#loc189)
      acir.work.end {acir.source_type = "void"}
    }
  }
  acir.module @top attributes {acir.instance = {contract = "Module:top", definition = "src_model_8294833e.Pipeline", inputs = {}, interfaces = {}, members = {decode_stage = {instance = "decode_stage"}, execute = {instance = "execute"}, fetch = {instance = "fetch"}, memory = {instance = "memory"}, memory_stage = {instance = "memory_stage"}, retirement = {resources = ["retirement"], type = "queue<src_isa_f0424bc8.Retired>", value}, rf = {instance = "rf"}, writeback = {instance = "writeback"}}, methods = [], name = "top", outputs = [{resources = ["rf.registers"], type = "cell<array<u32,32>>", value}, {resources = ["memory.words"], type = "cell<array<u32,16>>", value}, [{resources = ["writeback.retired"], type = "cell<u32>", value}, {resources = ["writeback.last_pc"], type = "cell<u32>", value}]], parent}} {
    acir.instance @rf = @rf
    acir.instance @memory = @memory
    acir.instance @fetch = @fetch
    acir.instance @decode_stage = @decode_stage
    acir.instance @execute = @execute
    acir.instance @memory_stage = @memory_stage
    acir.instance @writeback = @writeback
    acir.queue @retirement : !acir.struct<"src_isa_f0424bc8.Retired"> {capacity_fn = "ac_init_retirement_cap", initial_fn = "ac_init_retirement", name = "retirement", type = "src_isa_f0424bc8.Retired"}
    func.func private @ac_init_rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_retirement_cap() -> i32
    func.func private @ac_init_retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_fetch.pc() -> i32
    func.func private @ac_init_fetch.out_cap() -> i32
    func.func private @ac_init_fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_decode_stage.out_cap() -> i32
    func.func private @ac_init_decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_execute.out_cap() -> i32
    func.func private @ac_init_execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_writeback.retired() -> i32
    func.func private @ac_init_writeback.last_pc() -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.0.35(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.1.85(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2.0.call.2.108(i32) -> i32
    func.func private @decode_stage_Work.0.call.0.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":12:55)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":16:51)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:51)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":73:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":73:21)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":73:33)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":74:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":74:21)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":74:33)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":72:48)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":76:34)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":149:39)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":150:39)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:13)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:12)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:5)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":72:18)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":66:20)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":63:23)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":62:21)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":61:12)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":62:14)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":63:15)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":63:14)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:14)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:11)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:10)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":66:12)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":66:11)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:12)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:11)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":72:8)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:16)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:9)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":75:28)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":75:21)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:10)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":78:16)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":78:33)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":78:48)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":78:9)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":79:12)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":81:10)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":82:16)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":82:9)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":85:28)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":85:21)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":88:16)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":88:35)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":88:9)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:30)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:29)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:52)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:51)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:28)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":91:21)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":94:20)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":94:12)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":94:5)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":16:15)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":25:21)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":25:35)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":25:17)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":26:25)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":26:39)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":26:21)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":27:26)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":27:22)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":27:45)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":27:59)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":27:41)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":25:9)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":31:12)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":12:17)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":32:13)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:16)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:9)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":37:12)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":37:28)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":38:27)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":38:13)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":42:12)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":42:28)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":43:40)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":43:29)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":43:13)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":44:27)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":44:13)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":58:39)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:33)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:27)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:17)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:16)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:13)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:39)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:9)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":58:16)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":58:28)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":58:9)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:32)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:33)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:27)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:17)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:16)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:39)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:9)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:21)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:9)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:33)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:40)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":72:11)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:16)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:26)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":76:10)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:20)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:9)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:16)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:9)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:21)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":85:18)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:8)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":87:15)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":97:20)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":98:23)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":98:15)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":99:31)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":99:23)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":100:36)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":100:15)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:9)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:25)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:37)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:51)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:65)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":103:48)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:16)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:9)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:12)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":107:19)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":107:12)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":167:18)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":109:15)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":109:38)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":111:15)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:54)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:16)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:41)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:18)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:29)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:63)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:74)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:25)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:34)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:45)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:56)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:33)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:51)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:16)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:9)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":112:12)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":126:11)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:25)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":135:16)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":136:18)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:12)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:34)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:22)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":139:14)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":140:26)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":140:38)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":140:13)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:24)
#loc173 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:33)
#loc174 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:44)
#loc175 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:61)
#loc176 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:16)
#loc177 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:9)
#loc178 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":127:12)
#loc179 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":143:11)
#loc180 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":156:41)
#loc181 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":154:16)
#loc182 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":155:9)
#loc183 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":149:15)
#loc184 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":156:25)
#loc185 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":156:9)
#loc186 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":150:15)
#loc187 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":157:25)
#loc188 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":157:9)
#loc189 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":159:5)


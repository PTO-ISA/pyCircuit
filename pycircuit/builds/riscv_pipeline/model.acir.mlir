module attributes {acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, exports = {}, format_version = 8 : i64, top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}, {default = 0 : i64, name = "immediate", type = "u32"}, {default = false, name = "immediate_rhs", type = "bool"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "lhs", type = "u32"}, {default = 0 : i64, name = "rhs", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = false, name = "writes", type = "bool"}]}}} {
  func.func @ac_init_top.rf.registers() -> !acir.array<i32, 32> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.registers", params = [], result = "array<u32,32>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v33", acir.source_type = "array<u32,32>"} : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 32> loc(#loc1)
    return %0 : !acir.array<i32, 32>
  }
  func.func @ac_init_top.rf.pending() -> !acir.array<i1, 32> attributes {acir.function = {kind = "helper", name = "ac_init_top.rf.pending", params = [], result = "array<bool,32>"}} {
    %false = arith.constant {acir.source_type = "bool"} false
    %0 = acir.aggregate(%false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false, %false) {acir.id = "v67", acir.source_type = "array<bool,32>"} : (i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1, i1) -> !acir.array<i1, 32> loc(#loc2)
    return %0 : !acir.array<i1, 32>
  }
  func.func @ac_init_top.memory.words() -> !acir.array<i32, 16> attributes {acir.function = {kind = "helper", name = "ac_init_top.memory.words", params = [], result = "array<u32,16>"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %0 = acir.aggregate(%c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32, %c0_i32) {acir.id = "v85", acir.source_type = "array<u32,16>"} : (i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 16> loc(#loc3)
    return %0 : !acir.array<i32, 16>
  }
  func.func @ac_init_top.retirement_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.retirement_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">> attributes {acir.function = {kind = "helper", name = "ac_init_top.retirement", params = [], result = "vector<src_isa_f0424bc8.Retired>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Retired>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
  }
  func.func @ac_init_top.fetch.rom() -> !acir.array<i32, 6> attributes {acir.function = {kind = "helper", name = "ac_init_top.fetch.rom", params = [], result = "array<u32,6>"}} {
    %c7340179_i32 = arith.constant {acir.source_type = "u32"} 7340179 : i32 loc(#loc4)
    %c5243155_i32 = arith.constant {acir.source_type = "u32"} 5243155 : i32 loc(#loc5)
    %c2130355_i32 = arith.constant {acir.source_type = "u32"} 2130355 : i32 loc(#loc6)
    %c3153955_i32 = arith.constant {acir.source_type = "u32"} 3153955 : i32 loc(#loc7)
    %c8707_i32 = arith.constant {acir.source_type = "u32"} 8707 : i32 loc(#loc8)
    %c1075970739_i32 = arith.constant {acir.source_type = "u32"} 1075970739 : i32 loc(#loc9)
    %0 = acir.aggregate(%c7340179_i32, %c5243155_i32, %c2130355_i32, %c3153955_i32, %c8707_i32, %c1075970739_i32) {acir.id = "v95", acir.source_type = "array<u32,6>"} : (i32, i32, i32, i32, i32, i32) -> !acir.array<i32, 6> loc(#loc10)
    return %0 : !acir.array<i32, 6>
  }
  func.func @ac_init_top.fetch.pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.fetch.pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc11)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.fetch.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.fetch.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">> attributes {acir.function = {kind = "helper", name = "ac_init_top.fetch.out", params = [], result = "vector<src_isa_f0424bc8.Fetched>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Fetched>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
  }
  func.func @ac_init_top.decode_stage.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.decode_stage.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">> attributes {acir.function = {kind = "helper", name = "ac_init_top.decode_stage.out", params = [], result = "vector<src_isa_f0424bc8.Operands>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Operands>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
  }
  func.func @ac_init_top.execute.out_cap() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.execute.out_cap", params = [], result = "u32"}} {
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
    return %c1_i32 : i32
  }
  func.func @ac_init_top.execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">> attributes {acir.function = {kind = "helper", name = "ac_init_top.execute.out", params = [], result = "vector<src_isa_f0424bc8.Executed>"}} {
    %0 = acir.aggregate() {acir.source_type = "vector<src_isa_f0424bc8.Executed>"} : () -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    return %0 : !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
  }
  func.func @ac_init_top.writeback.retired() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.writeback.retired", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc12)
    return %c0_i32 : i32
  }
  func.func @ac_init_top.writeback.last_pc() -> i32 attributes {acir.function = {kind = "helper", name = "ac_init_top.writeback.last_pc", params = [], result = "u32"}} {
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc13)
    return %c0_i32 : i32
  }
  func.func @top.decode_stage_body.0.call.1.2.0.call.1.35(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "top.decode_stage_body.0.call.1.2.0.call.1.35", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc14)
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc15)
    return %1 : i32 loc(#loc16)
  }
  func.func @top.decode_stage_body.0.call.1.2.0.call.2.85(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "top.decode_stage_body.0.call.1.2.0.call.2.85", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc14)
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc15)
    return %1 : i32 loc(#loc16)
  }
  func.func @top.decode_stage_body.0.call.1.2.0.call.3.108(%arg0: i32) -> i32 attributes {acir.function = {definition = "src_isa_f0424bc8.sext12", kind = "helper", name = "top.decode_stage_body.0.call.1.2.0.call.3.108", params = [{consume = false, id = "v1", name = "value", targets = [], type = "u32"}], result = "u32"}} {
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %0 = arith.xori %arg0, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc14)
    %1 = arith.subi %0, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc15)
    return %1 : i32 loc(#loc16)
  }
  func.func @top.decode_stage_body.0.call.1.2(%arg0: !acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded"> attributes {acir.function = {definition = "src_isa_f0424bc8.decode", kind = "helper", name = "top.decode_stage_body.0.call.1.2", params = [{consume = false, id = "v1", name = "packet", targets = [], type = "src_isa_f0424bc8.Fetched"}], result = "src_isa_f0424bc8.Decoded"}} {
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
    %3 = arith.andi %2, %c7_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc23)
    %4 = call @ac_builtin_shr_u32(%0, %c25_i32) {acir.id = "v10", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc24)
    %5 = call @ac_builtin_shr_u32(%0, %c7_i32) {acir.id = "v12", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc25)
    %6 = arith.andi %5, %c31_i32 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc25)
    %7 = call @ac_builtin_shr_u32(%0, %c15_i32) {acir.id = "v16", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc26)
    %8 = arith.andi %7, %c31_i32 {acir.id = "v18", acir.source_type = "u32"} : i32 loc(#loc26)
    %9 = call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v20", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc27)
    %10 = arith.andi %9, %c31_i32 {acir.id = "v22", acir.source_type = "u32"} : i32 loc(#loc27)
    %11 = arith.cmpi eq, %1, %c19_i32 {acir.id = "v28", acir.source_type = "bool"} : i32 loc(#loc28)
    %12:6 = scf.if %11 -> (i32, i1, i32, i32, i32, i1) {
      %15 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v31", acir.source_type = "bool"} : i32 loc(#loc29)
      acir.check %15 {acir.id = "v32", acir.source_type = "void"} loc(#loc30)
      %16 = func.call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v35", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc31)
      %17 = func.call @top.decode_stage_body.0.call.1.2.0.call.1.35(%16) {acir.id = "v36", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc32)
      scf.yield %17, %true, %c0_i32, %6, %c0_i32, %true : i32, i1, i32, i32, i32, i1
    } else {
      %15 = arith.cmpi eq, %1, %c51_i32 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc33)
      %16 = arith.cmpi ne, %1, %c51_i32 : i32 loc(#loc33)
      %17:5 = scf.if %15 -> (i32, i32, i32, i32, i1) {
        %18 = arith.cmpi eq, %3, %c0_i32 {acir.id = "v42", acir.source_type = "bool"} : i32 loc(#loc34)
        %19 = scf.if %18 -> (i1) {
          %22 = arith.cmpi eq, %4, %c0_i32 {acir.id = "v45", acir.source_type = "bool"} : i32 loc(#loc35)
          %23 = scf.if %22 -> (i1) {
            scf.yield %true : i1
          } else {
            %24 = arith.cmpi eq, %4, %c32_i32 {acir.id = "v49", acir.source_type = "bool"} : i32 loc(#loc36)
            scf.yield %24 : i1
          }
          scf.yield %23 : i1
        } else {
          scf.yield %false : i1
        }
        acir.check %19 {acir.id = "v57", acir.source_type = "void"} loc(#loc37)
        %20 = arith.cmpi eq, %4, %c32_i32 {acir.id = "v59", acir.source_type = "bool"} : i32 loc(#loc38)
        %21 = arith.extui %20 : i1 to i32
        scf.yield %c0_i32, %21, %6, %10, %true : i32, i32, i32, i32, i1
      } else {
        %18 = arith.cmpi eq, %1, %c3_i32 {acir.id = "v77", acir.source_type = "bool"} : i32 loc(#loc39)
        %19 = arith.select %18, %c2_i32, %c3_i32 : i32
        %20 = arith.select %18, %6, %c0_i32 : i32
        %21 = arith.select %18, %c0_i32, %10 : i32
        %22 = scf.if %18 -> (i32) {
          %23 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v80", acir.source_type = "bool"} : i32 loc(#loc40)
          acir.check %23 {acir.id = "v81", acir.source_type = "void"} loc(#loc41)
          %24 = func.call @ac_builtin_shr_u32(%0, %c20_i32) {acir.id = "v85", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc42)
          %25 = func.call @top.decode_stage_body.0.call.1.2.0.call.2.85(%24) {acir.id = "v86", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc43)
          scf.yield %25 : i32
        } else {
          %23 = arith.cmpi eq, %1, %c35_i32 {acir.id = "v89", acir.source_type = "bool"} : i32 loc(#loc44)
          %24 = scf.if %23 -> (i1) {
            %31 = arith.cmpi eq, %3, %c2_i32 {acir.id = "v92", acir.source_type = "bool"} : i32 loc(#loc45)
            scf.yield %31 : i1
          } else {
            scf.yield %false : i1
          }
          acir.check %24 {acir.id = "v97", acir.source_type = "void"} loc(#loc46)
          %25 = func.call @ac_builtin_shr_u32(%0, %c25_i32) {acir.id = "v101", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc47)
          %26 = func.call @ac_builtin_shl_u32(%25, %c5_i32) {acir.id = "v103", acir.source_type = "u32", operator = "<<"} : (i32, i32) -> i32 loc(#loc47)
          %27 = func.call @ac_builtin_shr_u32(%0, %c7_i32) {acir.id = "v105", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc48)
          %28 = arith.andi %27, %c31_i32 {acir.id = "v107", acir.source_type = "u32"} : i32 loc(#loc48)
          %29 = arith.ori %26, %28 {acir.id = "v108", acir.source_type = "u32"} : i32 loc(#loc47)
          %30 = func.call @top.decode_stage_body.0.call.1.2.0.call.3.108(%29) {acir.id = "v109", acir.source_type = "u32", role = "helper"} : (i32) -> i32 loc(#loc49)
          scf.yield %30 : i32
        }
        scf.yield %22, %19, %20, %21, %18 : i32, i32, i32, i32, i1
      }
      scf.yield %17#0, %16, %17#1, %17#2, %17#3, %17#4 : i32, i1, i32, i32, i32, i1
    }
    %13 = acir.extract %arg0 {acir.id = "v154", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc50)
    %14 = acir.aggregate(%13, %0, %12#2, %12#3, %8, %12#4, %12#0, %12#1, %12#5) {acir.id = "v155", acir.source_type = "src_isa_f0424bc8.Decoded"} : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc51)
    return %14 : !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc52)
  }
  func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
  acir.module @top attributes {acir.instance = {definition = "src_model_8294833e.Pipeline", inputs = {}, members = {}, name = "top", outputs, parent, rules = []}} {
    acir.contract {relations = [], rules = []}
    acir.queue @top.retirement : !acir.struct<"src_isa_f0424bc8.Retired"> {capacity_fn = "ac_init_top.retirement_cap", initial_fn = "ac_init_top.retirement", name = "top.retirement", type = "src_isa_f0424bc8.Retired"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_top.retirement_cap() -> i32
    func.func private @ac_init_top.retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.fetch.out_cap() -> i32
    func.func private @ac_init_top.fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_top.decode_stage.out_cap() -> i32
    func.func private @ac_init_top.decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_top.execute.out_cap() -> i32
    func.func private @ac_init_top.execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.1.35(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.2.85(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.3.108(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.body attributes {acir.function = {kind = "body", name = "top_body", owner = "top", params = [], result = "void"}} {
      %0 = acir.ref @top.fetch::@top.fetch.out {acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
      %1 = acir.queue.peek %0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Fetched", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc53)
      %2 = acir.ref @top.decode_stage::@top.decode_stage.out {acir.source_type = "queue<src_isa_f0424bc8.Operands>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
      %3 = acir.queue.peek %2 {acir.id = "v6", acir.source_type = "src_isa_f0424bc8.Operands", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc54)
      %4 = acir.ref @top.execute::@top.execute.out {acir.source_type = "queue<src_isa_f0424bc8.Executed>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
      %5 = acir.queue.peek %4 {acir.id = "v9", acir.source_type = "src_isa_f0424bc8.Executed", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc55)
      %6 = acir.ref @top::@top.retirement {acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      %7 = acir.queue.peek %6 {acir.id = "v13", acir.source_type = "src_isa_f0424bc8.Retired", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc56)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.rf attributes {acir.instance = {definition = "src_model_8294833e.RegisterFile", inputs = {}, members = {}, name = "top.rf", outputs, parent = "top", rules = ["commit", "read", "ready", "reserve"]}} {
    acir.contract {relations = [], rules = ["commit", "read", "ready", "reserve"]}
    acir.cell @top.rf.registers : !acir.array<i32, 32> {initial_fn = "ac_init_top.rf.registers", name = "top.rf.registers", type = "array<u32,32>"}
    acir.cell @top.rf.pending : !acir.array<i1, 32> {initial_fn = "ac_init_top.rf.pending", name = "top.rf.pending", type = "array<bool,32>"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_top.retirement_cap() -> i32
    func.func private @ac_init_top.retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.fetch.out_cap() -> i32
    func.func private @ac_init_top.fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_top.decode_stage.out_cap() -> i32
    func.func private @ac_init_top.decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_top.execute.out_cap() -> i32
    func.func private @ac_init_top.execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.1.35(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.2.85(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.3.108(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.decode_stage_body.0.call.5.7 : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 {acir.function = {contract = "Module:top.rf", definition = "ready", instance = "top.rf", interface = "Module:top.rf:ready", kind = "rule", name = "top.decode_stage_body.0.call.5.7", output_type = "bool", owner = "top.rf", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], public = true, result = "bool", rule = "ready"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>>
      %1 = acir.extract %arg0 {acir.id = "v5", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc57)
      %2 = acir.cell.read %0 {acir.id = "v6", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc58)
      %3 = acir.extract %2[%1] {acir.id = "v7", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc58)
      %4 = acir.unary %3 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc59)
      %5 = scf.if %4 -> (i1) {
        %7 = acir.extract %arg0 {acir.id = "v11", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc60)
        %8 = acir.cell.read %0 {acir.id = "v12", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc61)
        %9 = acir.extract %8[%7] {acir.id = "v13", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc61)
        %10 = acir.unary %9 "not" {acir.id = "v14", acir.source_type = "bool"} : i1 -> i1 loc(#loc62)
        scf.yield %10 : i1
      } else {
        scf.yield %false : i1
      }
      %6 = scf.if %5 -> (i1) {
        %7 = acir.extract %arg0 {acir.id = "v20", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc63)
        %8 = acir.unary %7 "not" {acir.id = "v21", acir.source_type = "bool"} : i1 -> i1 loc(#loc64)
        %9 = scf.if %8 -> (i1) {
          scf.yield %true : i1
        } else {
          %10 = acir.extract %arg0 {acir.id = "v25", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc65)
          %11 = acir.cell.read %0 {acir.id = "v26", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc66)
          %12 = acir.extract %11[%10] {acir.id = "v27", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc66)
          %13 = acir.unary %12 "not" {acir.id = "v28", acir.source_type = "bool"} : i1 -> i1 loc(#loc67)
          scf.yield %13 : i1
        }
        scf.yield %9 : i1
      } else {
        scf.yield %false : i1
      }
      acir.return %6 : i1 {acir.id = "v36", acir.source_type = "void"} loc(#loc68)
    }
    acir.rule @top.decode_stage_body.0.call.6.14.0.call.6.6 : (i32) -> i32 {acir.function = {contract = "Module:top.rf", definition = "read", instance = "top.rf", interface = "Module:top.rf:read", kind = "rule", name = "top.decode_stage_body.0.call.6.14.0.call.6.6", output_type = "u32", owner = "top.rf", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], public = true, result = "u32", rule = "read"}} body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc69)
      cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      acir.return %c0_i32 : i32 {acir.id = "v8", acir.source_type = "void"} loc(#loc70)
    ^bb2(%1: i32):  // pred: ^bb0
      %2 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>>
      %3 = acir.cell.read %2 {acir.id = "v12", acir.source_type = "array<u32,32>", role = "observe"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc71)
      %4 = acir.extract %3[%1] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc71)
      acir.return %4 : i32 {acir.id = "v14", acir.source_type = "void"} loc(#loc72)
    }
    acir.rule @top.decode_stage_body.0.call.6.14.0.call.7.8 : (i32) -> i32 {acir.function = {contract = "Module:top.rf", definition = "read", instance = "top.rf", interface = "Module:top.rf:read", kind = "rule", name = "top.decode_stage_body.0.call.6.14.0.call.7.8", output_type = "u32", owner = "top.rf", params = [{consume = false, id = "v1", name = "index", targets = [], type = "u32"}], public = true, result = "u32", rule = "read"}} body {
    ^bb0(%arg0: i32):
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = arith.cmpi eq, %arg0, %c0_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc69)
      cf.cond_br %0, ^bb1, ^bb2(%arg0 : i32)
    ^bb1:  // pred: ^bb0
      acir.return %c0_i32 : i32 {acir.id = "v8", acir.source_type = "void"} loc(#loc70)
    ^bb2(%1: i32):  // pred: ^bb0
      %2 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>>
      %3 = acir.cell.read %2 {acir.id = "v12", acir.source_type = "array<u32,32>", role = "observe"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc71)
      %4 = acir.extract %3[%1] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc71)
      acir.return %4 : i32 {acir.id = "v14", acir.source_type = "void"} loc(#loc72)
    }
    acir.rule @top.decode_stage_body.0.call.6.14.0.call.8.15 : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> () {acir.function = {contract = "Module:top.rf", definition = "reserve", instance = "top.rf", interface = "Module:top.rf:reserve", kind = "rule", name = "top.decode_stage_body.0.call.6.14.0.call.8.15", output_type = "void", owner = "top.rf", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Decoded"}], public = true, result = "void", rule = "reserve"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %true = arith.constant {acir.source_type = "bool"} true
      %false = arith.constant {acir.source_type = "bool"} false
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc73)
      %1 = scf.if %0 -> (i1) {
        %2 = acir.extract %arg0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc74)
        %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc74)
        scf.yield %3 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %1 {
        %2 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>>
        %3 = acir.cell.read %2 {acir.id = "v18", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc75)
        %4 = acir.cell.read %2 {acir.id = "v22", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc75)
        %5 = acir.extract %arg0 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc76)
        %6 = acir.update %4, %true[%5] {acir.id = "v24", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc75)
        acir.cell.write %2, %6 {acir.id = "v26", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32> loc(#loc75)
      }
      acir.return {acir.source_type = "void"}
    }
    acir.rule @top.writeback_body.0.call.14.1.0.call.14.4 : (!acir.struct<"src_isa_f0424bc8.Retired">) -> () {acir.function = {contract = "Module:top.rf", definition = "commit", instance = "top.rf", interface = "Module:top.rf:commit", kind = "rule", name = "top.writeback_body.0.call.14.1.0.call.14.4", output_type = "void", owner = "top.rf", params = [{consume = false, id = "v1", name = "inst", targets = [], type = "src_isa_f0424bc8.Retired"}], public = true, result = "void", rule = "commit"}} body {
    ^bb0(%arg0: !acir.struct<"src_isa_f0424bc8.Retired">):
      %false = arith.constant {acir.source_type = "bool"} false
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
      %0 = acir.extract %arg0 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc77)
      %1 = scf.if %0 -> (i1) {
        %2 = acir.extract %arg0 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc78)
        %3 = arith.cmpi ne, %2, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc78)
        scf.yield %3 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %1 {
        %2 = acir.extract %arg0 {acir.id = "v15", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc79)
        %3 = acir.ref @top.rf::@top.rf.registers {acir.source_type = "cell<array<u32,32>>"} : !acir.cell<!acir.array<i32, 32>>
        %4 = acir.cell.read %3 {acir.id = "v18", acir.source_type = "array<u32,32>", role = "observe"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc80)
        %5 = acir.cell.read %3 {acir.id = "v22", acir.source_type = "array<u32,32>", role = "observe"} : !acir.cell<!acir.array<i32, 32>> -> !acir.array<i32, 32> loc(#loc80)
        %6 = acir.extract %arg0 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc81)
        %7 = acir.update %5, %2[%6] {acir.id = "v24", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32> loc(#loc80)
        acir.cell.write %3, %7 {acir.id = "v26", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 32>>, !acir.array<i32, 32> loc(#loc80)
        %8 = acir.ref @top.rf::@top.rf.pending {acir.source_type = "cell<array<bool,32>>"} : !acir.cell<!acir.array<i1, 32>>
        %9 = acir.cell.read %8 {acir.id = "v31", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc82)
        %10 = acir.cell.read %8 {acir.id = "v35", acir.source_type = "array<bool,32>", role = "observe"} : !acir.cell<!acir.array<i1, 32>> -> !acir.array<i1, 32> loc(#loc82)
        %11 = acir.extract %arg0 {acir.id = "v36", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc83)
        %12 = acir.update %10, %false[%11] {acir.id = "v37", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc82)
        acir.cell.write %8, %12 {acir.id = "v39", acir.source_type = "void"} : !acir.cell<!acir.array<i1, 32>>, !acir.array<i1, 32> loc(#loc82)
      }
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.memory attributes {acir.instance = {definition = "src_model_8294833e.DataMemory", inputs = {}, members = {}, name = "top.memory", outputs, parent = "top", rules = ["load", "store"]}} {
    acir.contract {relations = [], rules = ["load", "store"]}
    acir.cell @top.memory.words : !acir.array<i32, 16> {initial_fn = "ac_init_top.memory.words", name = "top.memory.words", type = "array<u32,16>"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_top.retirement_cap() -> i32
    func.func private @ac_init_top.retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.fetch.out_cap() -> i32
    func.func private @ac_init_top.fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_top.decode_stage.out_cap() -> i32
    func.func private @ac_init_top.decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_top.execute.out_cap() -> i32
    func.func private @ac_init_top.execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.1.35(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.2.85(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.3.108(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.memory_stage_body.0.call.11.1.0.call.11.10 : (i32) -> i32 {acir.function = {contract = "Module:top.memory", definition = "load", instance = "top.memory", interface = "Module:top.memory:load", kind = "rule", name = "top.memory_stage_body.0.call.11.1.0.call.11.10", output_type = "u32", owner = "top.memory", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}], public = true, result = "u32", rule = "load"}} body {
    ^bb0(%arg0: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc84)
      %false = arith.constant {acir.source_type = "bool"} false
      %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc85)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc86)
      %0 = arith.andi %arg0, %c3_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc87)
      %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc87)
      %2 = scf.if %1 -> (i1) {
        %7 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc88)
        scf.yield %7 : i1
      } else {
        scf.yield %false : i1
      }
      acir.check %2 {acir.id = "v15", acir.source_type = "void"} loc(#loc89)
      %3 = acir.ref @top.memory::@top.memory.words {acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>>
      %4 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v18", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc90)
      %5 = acir.cell.read %3 {acir.id = "v19", acir.source_type = "array<u32,16>", role = "observe"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc91)
      %6 = acir.extract %5[%4] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32 loc(#loc91)
      acir.return %6 : i32 {acir.id = "v21", acir.source_type = "void"} loc(#loc92)
    }
    acir.rule @top.memory_stage_body.0.call.11.1.0.call.12.17 : (i32, i32) -> () {acir.function = {contract = "Module:top.memory", definition = "store", instance = "top.memory", interface = "Module:top.memory:store", kind = "rule", name = "top.memory_stage_body.0.call.11.1.0.call.12.17", output_type = "void", owner = "top.memory", params = [{consume = false, id = "v1", name = "address", targets = [], type = "u32"}, {consume = false, id = "v2", name = "value", targets = [], type = "u32"}], public = true, result = "void", rule = "store"}} body {
    ^bb0(%arg0: i32, %arg1: i32):
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
      %false = arith.constant {acir.source_type = "bool"} false
      %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
      %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc93)
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc94)
      %0 = arith.andi %arg0, %c3_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc95)
      %1 = arith.cmpi eq, %0, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc95)
      %2 = scf.if %1 -> (i1) {
        %9 = arith.cmpi ult, %arg0, %c64_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc96)
        scf.yield %9 : i1
      } else {
        scf.yield %false : i1
      }
      acir.check %2 {acir.id = "v16", acir.source_type = "void"} loc(#loc97)
      %3 = acir.ref @top.memory::@top.memory.words {acir.source_type = "cell<array<u32,16>>"} : !acir.cell<!acir.array<i32, 16>>
      %4 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v20", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc98)
      %5 = acir.cell.read %3 {acir.id = "v21", acir.source_type = "array<u32,16>", role = "observe"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc99)
      %6 = acir.cell.read %3 {acir.id = "v25", acir.source_type = "array<u32,16>", role = "observe"} : !acir.cell<!acir.array<i32, 16>> -> !acir.array<i32, 16> loc(#loc99)
      %7 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v27", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc98)
      %8 = acir.update %6, %arg1[%7] {acir.id = "v28", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16> loc(#loc99)
      acir.cell.write %3, %8 {acir.id = "v30", acir.source_type = "void"} : !acir.cell<!acir.array<i32, 16>>, !acir.array<i32, 16> loc(#loc99)
      acir.return {acir.source_type = "void"}
    }
  }
  acir.module @top.fetch attributes {acir.instance = {definition = "src_model_8294833e.Fetch", inputs = {}, members = {}, name = "top.fetch", outputs, parent = "top", rules = ["fetch"]}} {
    acir.contract {relations = [], rules = ["fetch"]}
    acir.cell @top.fetch.rom : !acir.array<i32, 6> {initial_fn = "ac_init_top.fetch.rom", name = "top.fetch.rom", type = "array<u32,6>"}
    acir.cell @top.fetch.pc : i32 {initial_fn = "ac_init_top.fetch.pc", name = "top.fetch.pc", type = "u32"}
    acir.queue @top.fetch.out : !acir.struct<"src_isa_f0424bc8.Fetched"> {capacity_fn = "ac_init_top.fetch.out_cap", initial_fn = "ac_init_top.fetch.out", name = "top.fetch.out", type = "src_isa_f0424bc8.Fetched"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_top.retirement_cap() -> i32
    func.func private @ac_init_top.retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.fetch.out_cap() -> i32
    func.func private @ac_init_top.fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_top.decode_stage.out_cap() -> i32
    func.func private @ac_init_top.decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_top.execute.out_cap() -> i32
    func.func private @ac_init_top.execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.1.35(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.2.85(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.3.108(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.fetch_body.0.call.0.5 : (i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> {acir.function = {contract = "Module:top.fetch", definition = "fetch", instance = "top.fetch", interface = "Module:top.fetch:fetch", kind = "rule", name = "top.fetch_body.0.call.0.5", output_type = "queue<src_isa_f0424bc8.Fetched>", owner = "top.fetch", params = [{consume = false, id = "v1", name = "current_pc", targets = [], type = "u32"}], public = true, result = "src_isa_f0424bc8.Fetched", rule = "fetch"}} body {
    ^bb0(%arg0: i32):
      %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc100)
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc101)
      %0 = acir.ref @top.fetch::@top.fetch.rom {acir.source_type = "cell<array<u32,6>>"} : !acir.cell<!acir.array<i32, 6>>
      %1 = func.call @ac_builtin_shr_u32(%arg0, %c2_i32) {acir.id = "v6", acir.source_type = "u32", operator = ">>"} : (i32, i32) -> i32 loc(#loc102)
      %2 = acir.cell.read %0 {acir.id = "v7", acir.source_type = "array<u32,6>", role = "observe"} : !acir.cell<!acir.array<i32, 6>> -> !acir.array<i32, 6> loc(#loc103)
      %3 = acir.extract %2[%1] {acir.id = "v8", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 6>, i32) -> i32 loc(#loc103)
      %4 = arith.addi %arg0, %c4_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc104)
      %5 = acir.ref @top.fetch::@top.fetch.pc {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %5, %4 {acir.id = "v12", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc105)
      %6 = acir.aggregate(%arg0, %3) {acir.id = "v13", acir.source_type = "src_isa_f0424bc8.Fetched"} : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc106)
      acir.return %6 : !acir.struct<"src_isa_f0424bc8.Fetched"> {acir.id = "v14", acir.source_type = "void"} loc(#loc107)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.fetch_body", owner = "top.fetch", params = [], result = "void"}} {
      %c24_i32 = arith.constant {acir.source_type = "u32"} 24 : i32 loc(#loc108)
      %0 = acir.ref @top.fetch::@top.fetch.pc {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %1 = acir.cell.read %0 {acir.id = "v2", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc109)
      %2 = arith.cmpi ult, %1, %c24_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc110)
      scf.if %2 {
        %3 = acir.rule.call @top.fetch::@top.fetch_body.0.call.0.5(%1) {acir.id = "v6", acir.source_type = "src_isa_f0424bc8.Fetched", contract = "Module:top.fetch", instance = "top.fetch", interface = "Module:top.fetch:fetch", output_binding = @top.fetch::@top.fetch.out, role = "rule", rule = "fetch"} : (i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc111)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.decode_stage attributes {acir.instance = {definition = "src_model_8294833e.DecodeStage", inputs = {}, members = {}, name = "top.decode_stage", outputs, parent = "top", rules = ["read_operands"]}} {
    acir.contract {relations = [], rules = ["read_operands"]}
    acir.queue @top.decode_stage.out : !acir.struct<"src_isa_f0424bc8.Operands"> {capacity_fn = "ac_init_top.decode_stage.out_cap", initial_fn = "ac_init_top.decode_stage.out", name = "top.decode_stage.out", type = "src_isa_f0424bc8.Operands"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_top.retirement_cap() -> i32
    func.func private @ac_init_top.retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.fetch.out_cap() -> i32
    func.func private @ac_init_top.fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_top.decode_stage.out_cap() -> i32
    func.func private @ac_init_top.decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_top.execute.out_cap() -> i32
    func.func private @ac_init_top.execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.1.35(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.2.85(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.3.108(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.decode_stage_body.0.call.6.14 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Decoded">) -> !acir.struct<"src_isa_f0424bc8.Operands"> {acir.function = {contract = "Module:top.decode_stage", definition = "read_operands", instance = "top.decode_stage", interface = "Module:top.decode_stage:read_operands", kind = "rule", name = "top.decode_stage_body.0.call.6.14", output_type = "queue<src_isa_f0424bc8.Operands>", owner = "top.decode_stage", params = [{consume = false, id = "v1", name = "message", targets = ["top.fetch.out"], type = "queue<src_isa_f0424bc8.Fetched>"}, {consume = false, id = "v2", name = "decoded", targets = [], type = "src_isa_f0424bc8.Decoded"}], public = true, result = "src_isa_f0424bc8.Operands", rule = "read_operands"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, %arg1: !acir.struct<"src_isa_f0424bc8.Decoded">):
      %0 = acir.queue.peek %arg0 {acir.id = "v5", acir.source_type = "src_isa_f0424bc8.Fetched", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc112)
      %1 = acir.extract %arg1 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc113)
      %2 = acir.rule.call @top.rf::@top.decode_stage_body.0.call.6.14.0.call.6.6(%1) {acir.id = "v7", acir.source_type = "u32", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:read", role = "rule", rule = "read"} : (i32) -> i32 loc(#loc114)
      %3 = acir.extract %arg1 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc115)
      %4 = acir.rule.call @top.rf::@top.decode_stage_body.0.call.6.14.0.call.7.8(%3) {acir.id = "v9", acir.source_type = "u32", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:read", role = "rule", rule = "read"} : (i32) -> i32 loc(#loc116)
      %5 = acir.extract %arg1 {acir.id = "v10", acir.source_type = "bool", field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc117)
      %6 = scf.if %5 -> (i32) {
        %13 = acir.extract %arg1 {acir.id = "v12", acir.source_type = "u32", field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc118)
        scf.yield %13 : i32
      } else {
        scf.yield %4 : i32
      }
      acir.rule.call @top.rf::@top.decode_stage_body.0.call.6.14.0.call.8.15(%arg1) {acir.id = "v16", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:reserve", role = "rule", rule = "reserve"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> () loc(#loc119)
      %7 = acir.extract %arg1 {acir.id = "v17", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc120)
      %8 = acir.extract %arg1 {acir.id = "v18", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc121)
      %9 = acir.extract %arg1 {acir.id = "v19", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc122)
      %10 = acir.extract %arg1 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc123)
      %11 = acir.extract %arg1 {acir.id = "v21", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc124)
      %12 = acir.aggregate(%7, %8, %9, %10, %2, %6, %4, %11) {acir.id = "v22", acir.source_type = "src_isa_f0424bc8.Operands"} : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc125)
      acir.return %12 : !acir.struct<"src_isa_f0424bc8.Operands"> {acir.id = "v23", acir.source_type = "void"} loc(#loc126)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.decode_stage_body", owner = "top.decode_stage", params = [], result = "void"}} {
      %false = arith.constant {acir.source_type = "bool"} false
      %0 = acir.ref @top.fetch::@top.fetch.out {acir.source_type = "queue<src_isa_f0424bc8.Fetched>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>
      %1 = acir.queue.peek %0 {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Fetched", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">> -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc127)
      %2 = func.call @top.decode_stage_body.0.call.1.2(%1) {acir.id = "v3", acir.source_type = "src_isa_f0424bc8.Decoded", role = "helper"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc128)
      %3 = acir.ref @top::@top.retirement {acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      %4 = acir.queue.empty %3 {acir.id = "v6", acir.source_type = "bool", role = "observe"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> i1 loc(#loc129)
      %5 = scf.if %4 -> (i1) {
        %6 = acir.rule.call @top.rf::@top.decode_stage_body.0.call.5.7(%2) {acir.id = "v8", acir.source_type = "bool", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:ready", role = "rule", rule = "ready"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc130)
        scf.yield %6 : i1
      } else {
        scf.yield %false : i1
      }
      scf.if %5 {
        %6 = acir.rule.call @top.decode_stage::@top.decode_stage_body.0.call.6.14(%0, %2) {acir.id = "v15", acir.source_type = "src_isa_f0424bc8.Operands", contract = "Module:top.decode_stage", instance = "top.decode_stage", interface = "Module:top.decode_stage:read_operands", output_binding = @top.decode_stage::@top.decode_stage.out, role = "rule", rule = "read_operands"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Fetched">>, !acir.struct<"src_isa_f0424bc8.Decoded">) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc131)
      }
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.execute attributes {acir.instance = {definition = "src_model_8294833e.Execute", inputs = {}, members = {}, name = "top.execute", outputs, parent = "top", rules = ["execute"]}} {
    acir.contract {relations = [], rules = ["execute"]}
    acir.queue @top.execute.out : !acir.struct<"src_isa_f0424bc8.Executed"> {capacity_fn = "ac_init_top.execute.out_cap", initial_fn = "ac_init_top.execute.out", name = "top.execute.out", type = "src_isa_f0424bc8.Executed"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_top.retirement_cap() -> i32
    func.func private @ac_init_top.retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.fetch.out_cap() -> i32
    func.func private @ac_init_top.fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_top.decode_stage.out_cap() -> i32
    func.func private @ac_init_top.decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_top.execute.out_cap() -> i32
    func.func private @ac_init_top.execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.1.35(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.2.85(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.3.108(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.execute_body.0.call.10.1 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) -> !acir.struct<"src_isa_f0424bc8.Executed"> {acir.function = {contract = "Module:top.execute", definition = "execute", instance = "top.execute", interface = "Module:top.execute:execute", kind = "rule", name = "top.execute_body.0.call.10.1", output_type = "queue<src_isa_f0424bc8.Executed>", owner = "top.execute", params = [{consume = false, id = "v1", name = "message", targets = ["top.decode_stage.out"], type = "queue<src_isa_f0424bc8.Operands>"}], public = true, result = "src_isa_f0424bc8.Executed", rule = "execute"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>):
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc132)
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_isa_f0424bc8.Operands", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">> -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc133)
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc134)
      %2 = arith.cmpi eq, %1, %c1_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc134)
      %3 = scf.if %2 -> (i32) {
        %11 = acir.extract %0 {acir.id = "v9", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc135)
        %12 = acir.extract %0 {acir.id = "v10", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc136)
        %13 = arith.subi %11, %12 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc135)
        scf.yield %13 : i32
      } else {
        %11 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc137)
        %12 = acir.extract %0 {acir.id = "v13", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc138)
        %13 = arith.addi %11, %12 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc137)
        scf.yield %13 : i32
      }
      %4 = acir.extract %0 {acir.id = "v18", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc139)
      %5 = acir.extract %0 {acir.id = "v19", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc140)
      %6 = acir.extract %0 {acir.id = "v20", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc141)
      %7 = acir.extract %0 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc142)
      %8 = acir.extract %0 {acir.id = "v22", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc143)
      %9 = acir.extract %0 {acir.id = "v23", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc144)
      %10 = acir.aggregate(%4, %5, %6, %7, %3, %8, %9) {acir.id = "v24", acir.source_type = "src_isa_f0424bc8.Executed"} : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc145)
      acir.return %10 : !acir.struct<"src_isa_f0424bc8.Executed"> {acir.id = "v25", acir.source_type = "void"} loc(#loc146)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.execute_body", owner = "top.execute", params = [], result = "void"}} {
      %0 = acir.ref @top.decode_stage::@top.decode_stage.out {acir.source_type = "queue<src_isa_f0424bc8.Operands>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>
      %1 = acir.rule.call @top.execute::@top.execute_body.0.call.10.1(%0) {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Executed", contract = "Module:top.execute", instance = "top.execute", interface = "Module:top.execute:execute", output_binding = @top.execute::@top.execute.out, role = "rule", rule = "execute"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Operands">>) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc147)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.memory_stage attributes {acir.instance = {definition = "src_model_8294833e.MemoryStage", inputs = {}, members = {}, name = "top.memory_stage", outputs, parent = "top", rules = ["access"]}} {
    acir.contract {relations = [], rules = ["access"]}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_top.retirement_cap() -> i32
    func.func private @ac_init_top.retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.fetch.out_cap() -> i32
    func.func private @ac_init_top.fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_top.decode_stage.out_cap() -> i32
    func.func private @ac_init_top.decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_top.execute.out_cap() -> i32
    func.func private @ac_init_top.execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.1.35(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.2.85(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.3.108(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.memory_stage_body.0.call.11.1 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) -> !acir.struct<"src_isa_f0424bc8.Retired"> {acir.function = {contract = "Module:top.memory_stage", definition = "access", instance = "top.memory_stage", interface = "Module:top.memory_stage:access", kind = "rule", name = "top.memory_stage_body.0.call.11.1", output_type = "queue<src_isa_f0424bc8.Retired>", owner = "top.memory_stage", params = [{consume = false, id = "v1", name = "message", targets = ["top.execute.out"], type = "queue<src_isa_f0424bc8.Executed>"}], public = true, result = "src_isa_f0424bc8.Retired", rule = "access"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>):
      %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
      %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc148)
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_isa_f0424bc8.Executed", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">> -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc149)
      %1 = acir.extract %0 {acir.id = "v5", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc150)
      %2 = acir.extract %0 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc151)
      %3 = arith.cmpi eq, %2, %c2_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc151)
      %4 = scf.if %3 -> (i32) {
        %10 = acir.extract %0 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc152)
        %11 = acir.rule.call @top.memory::@top.memory_stage_body.0.call.11.1.0.call.11.10(%10) {acir.id = "v11", acir.source_type = "u32", contract = "Module:top.memory", instance = "top.memory", interface = "Module:top.memory:load", role = "rule", rule = "load"} : (i32) -> i32 loc(#loc153)
        scf.yield %11 : i32
      } else {
        %10 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc154)
        %11 = arith.cmpi eq, %10, %c3_i32 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc154)
        scf.if %11 {
          %12 = acir.extract %0 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc155)
          %13 = acir.extract %0 {acir.id = "v17", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc156)
          acir.rule.call @top.memory::@top.memory_stage_body.0.call.11.1.0.call.12.17(%12, %13) {acir.id = "v18", acir.source_type = "void", contract = "Module:top.memory", instance = "top.memory", interface = "Module:top.memory:store", role = "rule", rule = "store"} : (i32, i32) -> () loc(#loc157)
        }
        scf.yield %1 : i32
      }
      %5 = acir.extract %0 {acir.id = "v29", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc158)
      %6 = acir.extract %0 {acir.id = "v30", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc159)
      %7 = acir.extract %0 {acir.id = "v31", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc160)
      %8 = acir.extract %0 {acir.id = "v32", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc161)
      %9 = acir.aggregate(%5, %6, %7, %4, %8) {acir.id = "v33", acir.source_type = "src_isa_f0424bc8.Retired"} : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc162)
      acir.return %9 : !acir.struct<"src_isa_f0424bc8.Retired"> {acir.id = "v34", acir.source_type = "void"} loc(#loc163)
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.memory_stage_body", owner = "top.memory_stage", params = [], result = "void"}} {
      %0 = acir.ref @top.execute::@top.execute.out {acir.source_type = "queue<src_isa_f0424bc8.Executed>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>
      %1 = acir.rule.call @top.memory_stage::@top.memory_stage_body.0.call.11.1(%0) {acir.id = "v2", acir.source_type = "src_isa_f0424bc8.Retired", contract = "Module:top.memory_stage", instance = "top.memory_stage", interface = "Module:top.memory_stage:access", output_binding = @top::@top.retirement, role = "rule", rule = "access"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Executed">>) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc164)
      acir.body.end {acir.source_type = "void"}
    }
  }
  acir.module @top.writeback attributes {acir.instance = {definition = "src_model_8294833e.Writeback", inputs = {}, members = {}, name = "top.writeback", outputs, parent = "top", rules = ["commit"]}} {
    acir.contract {relations = [], rules = ["commit"]}
    acir.cell @top.writeback.retired : i32 {initial_fn = "ac_init_top.writeback.retired", name = "top.writeback.retired", type = "u32"}
    acir.cell @top.writeback.last_pc : i32 {initial_fn = "ac_init_top.writeback.last_pc", name = "top.writeback.last_pc", type = "u32"}
    func.func private @ac_init_top.rf.registers() -> !acir.array<i32, 32>
    func.func private @ac_init_top.rf.pending() -> !acir.array<i1, 32>
    func.func private @ac_init_top.memory.words() -> !acir.array<i32, 16>
    func.func private @ac_init_top.retirement_cap() -> i32
    func.func private @ac_init_top.retirement() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Retired">>
    func.func private @ac_init_top.fetch.rom() -> !acir.array<i32, 6>
    func.func private @ac_init_top.fetch.pc() -> i32
    func.func private @ac_init_top.fetch.out_cap() -> i32
    func.func private @ac_init_top.fetch.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Fetched">>
    func.func private @ac_init_top.decode_stage.out_cap() -> i32
    func.func private @ac_init_top.decode_stage.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Operands">>
    func.func private @ac_init_top.execute.out_cap() -> i32
    func.func private @ac_init_top.execute.out() -> !acir.vector<!acir.struct<"src_isa_f0424bc8.Executed">>
    func.func private @ac_init_top.writeback.retired() -> i32
    func.func private @ac_init_top.writeback.last_pc() -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.1.35(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.2.85(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2.0.call.3.108(i32) -> i32
    func.func private @top.decode_stage_body.0.call.1.2(!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    func.func private @ac_builtin_shl_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shl", kind = "intrinsic", name = "ac_builtin_shl_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    func.func private @ac_builtin_shr_u32(i32, i32) -> i32 attributes {acir.function = {intrinsic = "shr", kind = "intrinsic", name = "ac_builtin_shr_u32", params = [{name = "a", type = "u32"}, {name = "b", type = "u32"}], result = "u32"}}
    acir.rule @top.writeback_body.0.call.14.1 : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) -> () {acir.function = {contract = "Module:top.writeback", definition = "commit", instance = "top.writeback", interface = "Module:top.writeback:commit", kind = "rule", name = "top.writeback_body.0.call.14.1", output_type = "void", owner = "top.writeback", params = [{consume = false, id = "v1", name = "message", targets = ["top.retirement"], type = "queue<src_isa_f0424bc8.Retired>"}], public = true, result = "void", rule = "commit"}} body {
    ^bb0(%arg0: !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>):
      %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc165)
      %0 = acir.queue.peek %arg0 {acir.id = "v4", acir.source_type = "src_isa_f0424bc8.Retired", role = "consume"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">> -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc166)
      acir.rule.call @top.rf::@top.writeback_body.0.call.14.1.0.call.14.4(%0) {acir.id = "v5", acir.source_type = "void", contract = "Module:top.rf", instance = "top.rf", interface = "Module:top.rf:commit", role = "rule", rule = "commit"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> () loc(#loc167)
      %1 = acir.ref @top.writeback::@top.writeback.retired {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      %2 = acir.cell.read %1 {acir.id = "v7", acir.source_type = "u32", role = "observe"} : !acir.cell<i32> -> i32 loc(#loc168)
      %3 = arith.addi %2, %c1_i32 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc168)
      acir.cell.write %1, %3 {acir.id = "v11", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc169)
      %4 = acir.extract %0 {acir.id = "v12", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc170)
      %5 = acir.ref @top.writeback::@top.writeback.last_pc {acir.source_type = "cell<u32>"} : !acir.cell<i32>
      acir.cell.write %5, %4 {acir.id = "v14", acir.source_type = "void"} : !acir.cell<i32>, i32 loc(#loc171)
      acir.return {acir.source_type = "void"}
    }
    acir.body attributes {acir.function = {kind = "body", name = "top.writeback_body", owner = "top.writeback", params = [], result = "void"}} {
      %0 = acir.ref @top::@top.retirement {acir.source_type = "queue<src_isa_f0424bc8.Retired>"} : !acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>
      acir.rule.call @top.writeback::@top.writeback_body.0.call.14.1(%0) {acir.id = "v2", acir.source_type = "void", contract = "Module:top.writeback", instance = "top.writeback", interface = "Module:top.writeback:commit", role = "rule", rule = "commit"} : (!acir.queue<!acir.struct<"src_isa_f0424bc8.Retired">>) -> () loc(#loc172)
      acir.body.end {acir.source_type = "void"}
    }
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:59)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":14:55)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":41:55)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:53)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:62)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:71)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:80)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:89)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:95)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":57:52)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":58:38)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":135:43)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":136:43)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:13)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:12)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:5)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:18)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:19)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:22)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:21)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":53:12)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:14)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:14)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":56:14)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:10)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:11)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":59:11)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:8)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:16)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:9)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:28)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:21)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":69:10)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:16)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:33)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:48)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:9)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":71:12)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:10)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:16)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:9)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:28)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:21)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:16)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:33)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:9)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:28)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:46)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:21)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:20)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:12)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:5)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":163:9)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":164:9)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":165:9)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":166:9)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:28)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:20)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:16)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:55)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:47)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:43)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:75)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:71)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:102)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:94)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:90)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:9)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:12)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":23:13)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:16)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:9)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:12)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:28)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:13)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:21)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:12)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:28)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:34)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:13)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:23)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:13)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:21)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:33)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:31)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:26)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:16)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:37)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:9)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:22)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:16)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:9)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:31)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:26)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:16)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:37)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:9)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:15)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:9)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:27)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:34)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:20)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:16)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:14)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:9)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":65:16)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":65:9)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:25)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":68:22)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:12)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":70:19)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:20)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:23)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:15)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:31)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:23)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:36)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:15)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":85:9)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:25)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:37)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:51)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:65)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:100)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:16)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:9)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":89:23)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":89:16)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:19)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:42)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:19)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:54)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":103:16)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:41)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:18)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:29)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:63)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:74)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:25)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:34)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:45)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:56)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:73)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:91)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:16)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:9)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":108:15)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:25)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":119:16)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:18)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:12)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:34)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:22)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:14)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:26)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:38)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:13)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:24)
#loc159 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:33)
#loc160 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:44)
#loc161 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:61)
#loc162 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:16)
#loc163 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:9)
#loc164 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":128:15)
#loc165 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":142:29)
#loc166 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":140:16)
#loc167 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:9)
#loc168 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":142:19)
#loc169 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":142:9)
#loc170 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":143:19)
#loc171 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":143:9)
#loc172 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":146:9)

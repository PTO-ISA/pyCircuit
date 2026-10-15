#loc = loc(unknown)
module attributes {acir.instances = [{definition = "src_model_8294833e.Pipeline", name = "top", parent, rules = []}, {definition = "src_model_8294833e.RegisterFile", name = "top.rf", parent = "top", rules = ["commit", "read", "ready", "reserve"]}, {definition = "src_model_8294833e.DataMemory", name = "top.memory", parent = "top", rules = ["load", "store"]}, {definition = "src_model_8294833e.Fetch", name = "top.fetch", parent = "top", rules = ["fetch"]}, {definition = "src_model_8294833e.DecodeStage", name = "top.decode_stage", parent = "top", rules = ["read_operands"]}, {definition = "src_model_8294833e.Execute", name = "top.execute", parent = "top", rules = ["execute"]}, {definition = "src_model_8294833e.MemoryStage", name = "top.memory_stage", parent = "top", rules = ["access"]}, {definition = "src_model_8294833e.Writeback", name = "top.writeback", parent = "top", rules = ["commit"]}], acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, format_version = 9 : i64, top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "immediate", payload_type = i32, type = "u32"}, {default = false, name = "immediate_rhs", payload_type = i1, type = "bool"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "lhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}]}}, acir.stage = "transaction"} {
  %c64_i32 = arith.constant 64 : i32
  %c1_i32 = arith.constant 1 : i32
  %c31_i64 = arith.constant 31 : i64
  %c30_i64 = arith.constant 30 : i64
  %c29_i64 = arith.constant 29 : i64
  %c28_i64 = arith.constant 28 : i64
  %c27_i64 = arith.constant 27 : i64
  %c26_i64 = arith.constant 26 : i64
  %c25_i64 = arith.constant 25 : i64
  %c24_i64 = arith.constant 24 : i64
  %c23_i64 = arith.constant 23 : i64
  %c22_i64 = arith.constant 22 : i64
  %c21_i64 = arith.constant 21 : i64
  %c20_i64 = arith.constant 20 : i64
  %c19_i64 = arith.constant 19 : i64
  %c18_i64 = arith.constant 18 : i64
  %c17_i64 = arith.constant 17 : i64
  %c16_i64 = arith.constant 16 : i64
  %c15_i64 = arith.constant 15 : i64
  %c14_i64 = arith.constant 14 : i64
  %c13_i64 = arith.constant 13 : i64
  %c12_i64 = arith.constant 12 : i64
  %c11_i64 = arith.constant 11 : i64
  %c10_i64 = arith.constant 10 : i64
  %c9_i64 = arith.constant 9 : i64
  %c8_i64 = arith.constant 8 : i64
  %c7_i64 = arith.constant 7 : i64
  %c5_i64 = arith.constant 5 : i64
  %c4_i64 = arith.constant 4 : i64
  %c3_i64 = arith.constant 3 : i64
  %c2_i64 = arith.constant 2 : i64
  %c1_i64 = arith.constant 1 : i64
  %c32_i64 = arith.constant 32 : i64
  %c2048_i32 = arith.constant 2048 : i32
  %c127_i32 = arith.constant 127 : i32
  %c12_i32 = arith.constant 12 : i32
  %c7_i32 = arith.constant 7 : i32
  %c25_i32 = arith.constant 25 : i32
  %c31_i32 = arith.constant 31 : i32
  %c15_i32 = arith.constant 15 : i32
  %c20_i32 = arith.constant 20 : i32
  %c0_i32 = arith.constant 0 : i32
  %c19_i32 = arith.constant 19 : i32
  %c51_i32 = arith.constant 51 : i32
  %c32_i32 = arith.constant 32 : i32
  %c3_i32 = arith.constant 3 : i32
  %c35_i32 = arith.constant 35 : i32
  %c5_i32 = arith.constant 5 : i32
  %c0_i64 = arith.constant 0 : i64
  %c6_i64 = arith.constant 6 : i64
  %c2_i32 = arith.constant 2 : i32
  %c4_i32 = arith.constant 4 : i32
  %c24_i32 = arith.constant 24 : i32
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.ex_mem", source_branch = 205 : i64, target_branches = [204], to = 2 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.ex_mem", source_branch = 205 : i64, target_branches = [204], to = 2 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.id_ex", source_branch = 203 : i64, target_branches = [202], to = 1 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.id_ex", source_branch = 203 : i64, target_branches = [202], to = 1 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.if_id", source_branch = 102 : i64, target_branches = [2], to = 0 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.if_id", source_branch = 102 : i64, target_branches = [2], to = 0 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mem_wb", source_branch = 209 : i64, target_branches = [208], to = 3 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mem_wb", source_branch = 209 : i64, target_branches = [208], to = 3 : i64}], schedule = [4, 3, 2, 1, 0], stage = "transaction"} {
    acir.state @top.ex_mem : !acir.struct<"src_isa_f0424bc8.Executed"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.fetch.pc : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.fetch.rom : !acir.array<i32, [6]> {initial = [7340179 : i32, 5243155 : i32, 2130355 : i32, 3153955 : i32, 8707 : i32, 1075970739 : i32], kind = "cell"}
    acir.state @top.id_ex : !acir.struct<"src_isa_f0424bc8.Operands"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.if_id : !acir.struct<"src_isa_f0424bc8.Fetched"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.mem_wb : !acir.struct<"src_isa_f0424bc8.Retired"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.memory.words : !acir.array<i32, [16]> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rf.pending.0 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.1 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.10 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.11 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.12 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.13 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.14 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.15 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.16 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.17 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.18 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.19 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.2 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.20 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.21 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.22 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.23 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.24 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.25 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.26 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.27 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.28 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.29 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.3 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.30 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.31 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.4 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.5 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.6 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.7 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.8 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.pending.9 : i1 {initial = false, kind = "cell"}
    acir.state @top.rf.registers.0 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.1 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.10 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.11 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.12 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.13 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.14 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.15 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.16 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.17 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.18 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.19 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.2 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.20 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.21 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.22 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.23 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.24 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.25 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.26 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.27 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.28 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.29 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.3 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.30 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.31 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.4 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.5 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.6 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.7 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.8 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.rf.registers.9 : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.writeback.last_pc : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.writeback.retired : i32 {initial = 0 : i32, kind = "cell"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64}
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.fetch.pc", txn = -1 : i64} : (i1) -> (i32, i1)
    %0 = acir.bind %value : (i32) -> i32 loc(#loc1)
    %1 = arith.cmpi ult, %0, %c24_i32 : i32 loc(#loc2)
    acir.path %1 {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc3)
    acir.path %1 {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc3)
    %value_0, %ready_1 = acir.read(%1) {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "read", port = 0 : i64, resource = "top.fetch.rom", txn = 0 : i64} : (i1) -> (!acir.array<i32, [6]>, i1) loc(#loc4)
    %2 = arith.shrui %0, %c2_i32 : i32 loc(#loc5)
    %3 = arith.extui %2 : i32 to i64
    %4 = arith.cmpi ult, %3, %c6_i64 : i64
    %5 = arith.select %4, %3, %c0_i64 : i64
    %6 = "acir.array.index"(%value_0, %5) : (!acir.array<i32, [6]>, i64) -> i32 loc(#loc6)
    %7 = arith.addi %0, %c4_i32 : i32 loc(#loc7)
    %8 = acir.aggregate(%0, %6) : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc8)
    %9 = arith.andi %1, %4 : i1
    acir.path %9 {branch = 2 : i64, kind = "branch", txn = 0 : i64}
    %value_2, %ready_3 = acir.read(%9) {branch = 2 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.if_id", txn = 0 : i64} : (i1) -> (i1, i1)
    %10 = arith.cmpi uge, %3, %c6_i64 : i64
    %11 = arith.xori %value_2, %true : i1
    %12 = arith.ori %10, %11 : i1
    %13 = arith.andi %1, %12 : i1
    %14 = acir.fire "top.fetch.fetch#0" %13 {successors = [], txn = 0 : i64}
    %15 = arith.andi %14, %1 : i1
    acir.effect %15(%7) {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "write", port = 0 : i64, resource = "top.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc9)
    %16 = arith.andi %14, %9 : i1
    acir.effect %16(%8) {branch = 2 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.if_id", txn = 0 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Fetched">) -> ()
    %value_4, %ready_5 = acir.read(%true) {branch = 0 : i64, frames = [], method = "peek", port = 0 : i64, resource = "top.if_id", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Fetched">, i1) loc(#loc10)
    %17 = acir.bind %value_4 : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc117)
    %18 = acir.extract %17 {field = "word"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc118)
    %19 = arith.andi %18, %c127_i32 : i32 loc(#loc119)
    %20 = arith.shrui %18, %c12_i32 : i32 loc(#loc120)
    %21 = arith.andi %20, %c7_i32 : i32 loc(#loc121)
    %22 = arith.shrui %18, %c25_i32 : i32 loc(#loc122)
    %23 = arith.shrui %18, %c7_i32 : i32 loc(#loc123)
    %24 = arith.andi %23, %c31_i32 : i32 loc(#loc124)
    %25 = arith.shrui %18, %c15_i32 : i32 loc(#loc125)
    %26 = arith.andi %25, %c31_i32 : i32 loc(#loc126)
    %27 = arith.shrui %18, %c20_i32 : i32 loc(#loc127)
    %28 = arith.andi %27, %c31_i32 : i32 loc(#loc128)
    %29 = arith.cmpi eq, %19, %c19_i32 : i32 loc(#loc129)
    %30 = arith.cmpi eq, %19, %c51_i32 : i32 loc(#loc130)
    %31 = arith.cmpi eq, %19, %c3_i32 : i32 loc(#loc131)
    %32 = arith.select %31, %c2_i32, %c3_i32 : i32 loc(#loc132)
    %33 = arith.select %31, %24, %c0_i32 : i32 loc(#loc124)
    %34 = arith.select %31, %c0_i32, %28 : i32 loc(#loc128)
    %35 = arith.cmpi eq, %19, %c35_i32 : i32 loc(#loc133)
    %36 = arith.cmpi eq, %21, %c2_i32 : i32 loc(#loc134)
    %37 = arith.andi %35, %36 : i1 loc(#loc135)
    %38 = arith.cmpi ne, %19, %c35_i32 : i32 loc(#loc149)
    %39 = arith.ori %38, %ready_5 : i1 loc(#loc135)
    %40 = arith.andi %ready_5, %39 : i1 loc(#loc135)
    %41 = acir.assert %37 loc(#loc136)
    %42 = arith.andi %40, %41 : i1 loc(#loc136)
    %43 = arith.shli %22, %c5_i32 : i32 loc(#loc137)
    %44 = arith.ori %43, %24 : i32 loc(#loc150)
    %45 = arith.xori %44, %c2048_i32 : i32 loc(#loc151)
    %46 = arith.subi %45, %c2048_i32 : i32 loc(#loc138)
    %47 = acir.assert %36 loc(#loc139)
    %48 = arith.andi %ready_5, %47 : i1 loc(#loc139)
    %49 = arith.xori %27, %c2048_i32 : i32 loc(#loc151)
    %50 = arith.subi %49, %c2048_i32 : i32 loc(#loc138)
    %51 = arith.select %31, %48, %42 : i1 loc(#loc135)
    %52 = arith.andi %ready_5, %51 : i1 loc(#loc135)
    %53 = arith.select %31, %50, %46 : i32 loc(#loc138)
    %54 = arith.cmpi eq, %21, %c0_i32 : i32 loc(#loc140)
    %55 = arith.cmpi eq, %22, %c0_i32 : i32 loc(#loc141)
    %56 = arith.cmpi eq, %22, %c32_i32 : i32 loc(#loc142)
    %57 = arith.ori %55, %56 : i1 loc(#loc135)
    %58 = arith.ori %55, %ready_5 : i1 loc(#loc135)
    %59 = arith.andi %ready_5, %58 : i1 loc(#loc135)
    %60 = arith.cmpi ne, %21, %c0_i32 : i32 loc(#loc152)
    %61 = arith.ori %60, %ready_5 : i1 loc(#loc135)
    %62 = arith.andi %ready_5, %61 : i1 loc(#loc135)
    %63 = arith.andi %54, %57 : i1 loc(#loc135)
    %64 = arith.ori %60, %59 : i1 loc(#loc135)
    %65 = arith.andi %ready_5, %64 : i1 loc(#loc135)
    %66 = acir.assert %63 loc(#loc143)
    %67 = arith.andi %65, %66 : i1 loc(#loc143)
    %68 = arith.andi %62, %67 : i1 loc(#loc143)
    %69 = arith.extui %56 : i1 to i32 loc(#loc132)
    %70 = arith.select %30, %68, %52 : i1 loc(#loc135)
    %71 = arith.andi %ready_5, %70 : i1 loc(#loc135)
    %72 = arith.select %30, %c0_i32, %53 : i32 loc(#loc138)
    %73 = arith.cmpi ne, %19, %c51_i32 : i32 loc(#loc153)
    %74 = arith.select %30, %69, %32 : i32 loc(#loc132)
    %75 = arith.select %30, %24, %33 : i32 loc(#loc124)
    %76 = arith.select %30, %28, %34 : i32 loc(#loc128)
    %77 = arith.ori %30, %31 : i1 loc(#loc131)
    %78 = arith.andi %71, %71 : i1 loc(#loc135)
    %79 = acir.assert %54 loc(#loc144)
    %80 = arith.andi %ready_5, %79 : i1 loc(#loc144)
    %81 = arith.select %29, %80, %71 : i1 loc(#loc135)
    %82 = arith.andi %ready_5, %81 : i1 loc(#loc135)
    %83 = arith.select %29, %50, %72 : i32 loc(#loc138)
    %84 = arith.select %29, %80, %78 : i1 loc(#loc135)
    %85 = arith.andi %ready_5, %84 : i1 loc(#loc135)
    %86 = arith.ori %29, %73 : i1 loc(#loc145)
    %87 = arith.select %29, %c0_i32, %74 : i32 loc(#loc132)
    %88 = arith.select %29, %24, %75 : i32 loc(#loc124)
    %89 = arith.select %29, %c0_i32, %76 : i32 loc(#loc128)
    %90 = arith.ori %29, %77 : i1 loc(#loc131)
    %91 = acir.extract %17 {field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc146)
    %92 = arith.andi %82, %85 : i1 loc(#loc147)
    %93 = acir.aggregate(%91, %18, %87, %88, %26, %89, %83, %86, %90) : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc43)
    %94 = arith.andi %92, %82 : i1 loc(#loc135)
    %value_6, %ready_7 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.mem_wb", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc44)
    acir.path %value_6 {branch = 3 : i64, kind = "branch", txn = -1 : i64} loc(#loc45)
    acir.path %value_6 {branch = 3 : i64, frames = ["top.rf:ready"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc45)
    "acir.resource.array"() <{name = "top.rf.pending", resources = ["top.rf.pending.0", "top.rf.pending.1", "top.rf.pending.2", "top.rf.pending.3", "top.rf.pending.4", "top.rf.pending.5", "top.rf.pending.6", "top.rf.pending.7", "top.rf.pending.8", "top.rf.pending.9", "top.rf.pending.10", "top.rf.pending.11", "top.rf.pending.12", "top.rf.pending.13", "top.rf.pending.14", "top.rf.pending.15", "top.rf.pending.16", "top.rf.pending.17", "top.rf.pending.18", "top.rf.pending.19", "top.rf.pending.20", "top.rf.pending.21", "top.rf.pending.22", "top.rf.pending.23", "top.rf.pending.24", "top.rf.pending.25", "top.rf.pending.26", "top.rf.pending.27", "top.rf.pending.28", "top.rf.pending.29", "top.rf.pending.30", "top.rf.pending.31"], shape = [32]}> : () -> () loc(#loc46)
    %95 = acir.extract %93 {field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc47)
    %96 = arith.extui %95 : i32 to i64 loc(#loc48)
    %97 = arith.cmpi ult, %96, %c32_i64 : i64 loc(#loc48)
    %98 = arith.andi %94, %97 : i1 loc(#loc48)
    %value_8, %ready_9 = acir.read(%value_6, %96) {array = "top.rf.pending", branch = 3 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.pending.0", resources = ["top.rf.pending.0", "top.rf.pending.1", "top.rf.pending.2", "top.rf.pending.3", "top.rf.pending.4", "top.rf.pending.5", "top.rf.pending.6", "top.rf.pending.7", "top.rf.pending.8", "top.rf.pending.9", "top.rf.pending.10", "top.rf.pending.11", "top.rf.pending.12", "top.rf.pending.13", "top.rf.pending.14", "top.rf.pending.15", "top.rf.pending.16", "top.rf.pending.17", "top.rf.pending.18", "top.rf.pending.19", "top.rf.pending.20", "top.rf.pending.21", "top.rf.pending.22", "top.rf.pending.23", "top.rf.pending.24", "top.rf.pending.25", "top.rf.pending.26", "top.rf.pending.27", "top.rf.pending.28", "top.rf.pending.29", "top.rf.pending.30", "top.rf.pending.31"], txn = -1 : i64} : (i1, i64) -> (i1, i1) loc(#loc48)
    %99 = arith.cmpi eq, %96, %c0_i64 : i64 loc(#loc48)
    %100 = arith.andi %value_6, %99 : i1 loc(#loc48)
    acir.path %100 {branch = 4 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %101 = arith.cmpi eq, %96, %c1_i64 : i64 loc(#loc48)
    %102 = arith.andi %value_6, %101 : i1 loc(#loc48)
    acir.path %102 {branch = 5 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %103 = arith.cmpi eq, %96, %c2_i64 : i64 loc(#loc48)
    %104 = arith.andi %value_6, %103 : i1 loc(#loc48)
    acir.path %104 {branch = 6 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %105 = arith.cmpi eq, %96, %c3_i64 : i64 loc(#loc48)
    %106 = arith.andi %value_6, %105 : i1 loc(#loc48)
    acir.path %106 {branch = 7 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %107 = arith.cmpi eq, %96, %c4_i64 : i64 loc(#loc48)
    %108 = arith.andi %value_6, %107 : i1 loc(#loc48)
    acir.path %108 {branch = 8 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %109 = arith.cmpi eq, %96, %c5_i64 : i64 loc(#loc48)
    %110 = arith.andi %value_6, %109 : i1 loc(#loc48)
    acir.path %110 {branch = 9 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %111 = arith.cmpi eq, %96, %c6_i64 : i64 loc(#loc48)
    %112 = arith.andi %value_6, %111 : i1 loc(#loc48)
    acir.path %112 {branch = 10 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %113 = arith.cmpi eq, %96, %c7_i64 : i64 loc(#loc48)
    %114 = arith.andi %value_6, %113 : i1 loc(#loc48)
    acir.path %114 {branch = 11 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %115 = arith.cmpi eq, %96, %c8_i64 : i64 loc(#loc48)
    %116 = arith.andi %value_6, %115 : i1 loc(#loc48)
    acir.path %116 {branch = 12 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %117 = arith.cmpi eq, %96, %c9_i64 : i64 loc(#loc48)
    %118 = arith.andi %value_6, %117 : i1 loc(#loc48)
    acir.path %118 {branch = 13 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %119 = arith.cmpi eq, %96, %c10_i64 : i64 loc(#loc48)
    %120 = arith.andi %value_6, %119 : i1 loc(#loc48)
    acir.path %120 {branch = 14 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %121 = arith.cmpi eq, %96, %c11_i64 : i64 loc(#loc48)
    %122 = arith.andi %value_6, %121 : i1 loc(#loc48)
    acir.path %122 {branch = 15 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %123 = arith.cmpi eq, %96, %c12_i64 : i64 loc(#loc48)
    %124 = arith.andi %value_6, %123 : i1 loc(#loc48)
    acir.path %124 {branch = 16 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %125 = arith.cmpi eq, %96, %c13_i64 : i64 loc(#loc48)
    %126 = arith.andi %value_6, %125 : i1 loc(#loc48)
    acir.path %126 {branch = 17 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %127 = arith.cmpi eq, %96, %c14_i64 : i64 loc(#loc48)
    %128 = arith.andi %value_6, %127 : i1 loc(#loc48)
    acir.path %128 {branch = 18 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %129 = arith.cmpi eq, %96, %c15_i64 : i64 loc(#loc48)
    %130 = arith.andi %value_6, %129 : i1 loc(#loc48)
    acir.path %130 {branch = 19 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %131 = arith.cmpi eq, %96, %c16_i64 : i64 loc(#loc48)
    %132 = arith.andi %value_6, %131 : i1 loc(#loc48)
    acir.path %132 {branch = 20 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %133 = arith.cmpi eq, %96, %c17_i64 : i64 loc(#loc48)
    %134 = arith.andi %value_6, %133 : i1 loc(#loc48)
    acir.path %134 {branch = 21 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %135 = arith.cmpi eq, %96, %c18_i64 : i64 loc(#loc48)
    %136 = arith.andi %value_6, %135 : i1 loc(#loc48)
    acir.path %136 {branch = 22 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %137 = arith.cmpi eq, %96, %c19_i64 : i64 loc(#loc48)
    %138 = arith.andi %value_6, %137 : i1 loc(#loc48)
    acir.path %138 {branch = 23 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %139 = arith.cmpi eq, %96, %c20_i64 : i64 loc(#loc48)
    %140 = arith.andi %value_6, %139 : i1 loc(#loc48)
    acir.path %140 {branch = 24 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %141 = arith.cmpi eq, %96, %c21_i64 : i64 loc(#loc48)
    %142 = arith.andi %value_6, %141 : i1 loc(#loc48)
    acir.path %142 {branch = 25 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %143 = arith.cmpi eq, %96, %c22_i64 : i64 loc(#loc48)
    %144 = arith.andi %value_6, %143 : i1 loc(#loc48)
    acir.path %144 {branch = 26 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %145 = arith.cmpi eq, %96, %c23_i64 : i64 loc(#loc48)
    %146 = arith.andi %value_6, %145 : i1 loc(#loc48)
    acir.path %146 {branch = 27 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %147 = arith.cmpi eq, %96, %c24_i64 : i64 loc(#loc48)
    %148 = arith.andi %value_6, %147 : i1 loc(#loc48)
    acir.path %148 {branch = 28 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %149 = arith.cmpi eq, %96, %c25_i64 : i64 loc(#loc48)
    %150 = arith.andi %value_6, %149 : i1 loc(#loc48)
    acir.path %150 {branch = 29 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %151 = arith.cmpi eq, %96, %c26_i64 : i64 loc(#loc48)
    %152 = arith.andi %value_6, %151 : i1 loc(#loc48)
    acir.path %152 {branch = 30 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %153 = arith.cmpi eq, %96, %c27_i64 : i64 loc(#loc48)
    %154 = arith.andi %value_6, %153 : i1 loc(#loc48)
    acir.path %154 {branch = 31 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %155 = arith.cmpi eq, %96, %c28_i64 : i64 loc(#loc48)
    %156 = arith.andi %value_6, %155 : i1 loc(#loc48)
    acir.path %156 {branch = 32 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %157 = arith.cmpi eq, %96, %c29_i64 : i64 loc(#loc48)
    %158 = arith.andi %value_6, %157 : i1 loc(#loc48)
    acir.path %158 {branch = 33 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %159 = arith.cmpi eq, %96, %c30_i64 : i64 loc(#loc48)
    %160 = arith.andi %value_6, %159 : i1 loc(#loc48)
    acir.path %160 {branch = 34 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %161 = arith.cmpi eq, %96, %c31_i64 : i64 loc(#loc48)
    %162 = arith.andi %value_6, %161 : i1 loc(#loc48)
    acir.path %162 {branch = 35 : i64, kind = "branch", txn = -1 : i64} loc(#loc48)
    %163 = arith.xori %value_8, %true : i1
    %164 = arith.andi %value_6, %163 : i1
    %165 = acir.extract %93 {field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc49)
    %166 = arith.extui %165 : i32 to i64 loc(#loc50)
    %167 = arith.cmpi ult, %166, %c32_i64 : i64 loc(#loc50)
    %168 = arith.andi %94, %167 : i1 loc(#loc50)
    acir.path %164 {branch = 36 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %value_10, %ready_11 = acir.read(%164, %166) {array = "top.rf.pending", branch = 36 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.pending.0", resources = ["top.rf.pending.0", "top.rf.pending.1", "top.rf.pending.2", "top.rf.pending.3", "top.rf.pending.4", "top.rf.pending.5", "top.rf.pending.6", "top.rf.pending.7", "top.rf.pending.8", "top.rf.pending.9", "top.rf.pending.10", "top.rf.pending.11", "top.rf.pending.12", "top.rf.pending.13", "top.rf.pending.14", "top.rf.pending.15", "top.rf.pending.16", "top.rf.pending.17", "top.rf.pending.18", "top.rf.pending.19", "top.rf.pending.20", "top.rf.pending.21", "top.rf.pending.22", "top.rf.pending.23", "top.rf.pending.24", "top.rf.pending.25", "top.rf.pending.26", "top.rf.pending.27", "top.rf.pending.28", "top.rf.pending.29", "top.rf.pending.30", "top.rf.pending.31"], txn = -1 : i64} : (i1, i64) -> (i1, i1) loc(#loc50)
    %169 = arith.cmpi eq, %166, %c0_i64 : i64 loc(#loc50)
    %170 = arith.andi %164, %169 : i1 loc(#loc50)
    acir.path %170 {branch = 37 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %171 = arith.cmpi eq, %166, %c1_i64 : i64 loc(#loc50)
    %172 = arith.andi %164, %171 : i1 loc(#loc50)
    acir.path %172 {branch = 38 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %173 = arith.cmpi eq, %166, %c2_i64 : i64 loc(#loc50)
    %174 = arith.andi %164, %173 : i1 loc(#loc50)
    acir.path %174 {branch = 39 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %175 = arith.cmpi eq, %166, %c3_i64 : i64 loc(#loc50)
    %176 = arith.andi %164, %175 : i1 loc(#loc50)
    acir.path %176 {branch = 40 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %177 = arith.cmpi eq, %166, %c4_i64 : i64 loc(#loc50)
    %178 = arith.andi %164, %177 : i1 loc(#loc50)
    acir.path %178 {branch = 41 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %179 = arith.cmpi eq, %166, %c5_i64 : i64 loc(#loc50)
    %180 = arith.andi %164, %179 : i1 loc(#loc50)
    acir.path %180 {branch = 42 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %181 = arith.cmpi eq, %166, %c6_i64 : i64 loc(#loc50)
    %182 = arith.andi %164, %181 : i1 loc(#loc50)
    acir.path %182 {branch = 43 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %183 = arith.cmpi eq, %166, %c7_i64 : i64 loc(#loc50)
    %184 = arith.andi %164, %183 : i1 loc(#loc50)
    acir.path %184 {branch = 44 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %185 = arith.cmpi eq, %166, %c8_i64 : i64 loc(#loc50)
    %186 = arith.andi %164, %185 : i1 loc(#loc50)
    acir.path %186 {branch = 45 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %187 = arith.cmpi eq, %166, %c9_i64 : i64 loc(#loc50)
    %188 = arith.andi %164, %187 : i1 loc(#loc50)
    acir.path %188 {branch = 46 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %189 = arith.cmpi eq, %166, %c10_i64 : i64 loc(#loc50)
    %190 = arith.andi %164, %189 : i1 loc(#loc50)
    acir.path %190 {branch = 47 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %191 = arith.cmpi eq, %166, %c11_i64 : i64 loc(#loc50)
    %192 = arith.andi %164, %191 : i1 loc(#loc50)
    acir.path %192 {branch = 48 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %193 = arith.cmpi eq, %166, %c12_i64 : i64 loc(#loc50)
    %194 = arith.andi %164, %193 : i1 loc(#loc50)
    acir.path %194 {branch = 49 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %195 = arith.cmpi eq, %166, %c13_i64 : i64 loc(#loc50)
    %196 = arith.andi %164, %195 : i1 loc(#loc50)
    acir.path %196 {branch = 50 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %197 = arith.cmpi eq, %166, %c14_i64 : i64 loc(#loc50)
    %198 = arith.andi %164, %197 : i1 loc(#loc50)
    acir.path %198 {branch = 51 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %199 = arith.cmpi eq, %166, %c15_i64 : i64 loc(#loc50)
    %200 = arith.andi %164, %199 : i1 loc(#loc50)
    acir.path %200 {branch = 52 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %201 = arith.cmpi eq, %166, %c16_i64 : i64 loc(#loc50)
    %202 = arith.andi %164, %201 : i1 loc(#loc50)
    acir.path %202 {branch = 53 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %203 = arith.cmpi eq, %166, %c17_i64 : i64 loc(#loc50)
    %204 = arith.andi %164, %203 : i1 loc(#loc50)
    acir.path %204 {branch = 54 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %205 = arith.cmpi eq, %166, %c18_i64 : i64 loc(#loc50)
    %206 = arith.andi %164, %205 : i1 loc(#loc50)
    acir.path %206 {branch = 55 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %207 = arith.cmpi eq, %166, %c19_i64 : i64 loc(#loc50)
    %208 = arith.andi %164, %207 : i1 loc(#loc50)
    acir.path %208 {branch = 56 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %209 = arith.cmpi eq, %166, %c20_i64 : i64 loc(#loc50)
    %210 = arith.andi %164, %209 : i1 loc(#loc50)
    acir.path %210 {branch = 57 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %211 = arith.cmpi eq, %166, %c21_i64 : i64 loc(#loc50)
    %212 = arith.andi %164, %211 : i1 loc(#loc50)
    acir.path %212 {branch = 58 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %213 = arith.cmpi eq, %166, %c22_i64 : i64 loc(#loc50)
    %214 = arith.andi %164, %213 : i1 loc(#loc50)
    acir.path %214 {branch = 59 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %215 = arith.cmpi eq, %166, %c23_i64 : i64 loc(#loc50)
    %216 = arith.andi %164, %215 : i1 loc(#loc50)
    acir.path %216 {branch = 60 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %217 = arith.cmpi eq, %166, %c24_i64 : i64 loc(#loc50)
    %218 = arith.andi %164, %217 : i1 loc(#loc50)
    acir.path %218 {branch = 61 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %219 = arith.cmpi eq, %166, %c25_i64 : i64 loc(#loc50)
    %220 = arith.andi %164, %219 : i1 loc(#loc50)
    acir.path %220 {branch = 62 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %221 = arith.cmpi eq, %166, %c26_i64 : i64 loc(#loc50)
    %222 = arith.andi %164, %221 : i1 loc(#loc50)
    acir.path %222 {branch = 63 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %223 = arith.cmpi eq, %166, %c27_i64 : i64 loc(#loc50)
    %224 = arith.andi %164, %223 : i1 loc(#loc50)
    acir.path %224 {branch = 64 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %225 = arith.cmpi eq, %166, %c28_i64 : i64 loc(#loc50)
    %226 = arith.andi %164, %225 : i1 loc(#loc50)
    acir.path %226 {branch = 65 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %227 = arith.cmpi eq, %166, %c29_i64 : i64 loc(#loc50)
    %228 = arith.andi %164, %227 : i1 loc(#loc50)
    acir.path %228 {branch = 66 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %229 = arith.cmpi eq, %166, %c30_i64 : i64 loc(#loc50)
    %230 = arith.andi %164, %229 : i1 loc(#loc50)
    acir.path %230 {branch = 67 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %231 = arith.cmpi eq, %166, %c31_i64 : i64 loc(#loc50)
    %232 = arith.andi %164, %231 : i1 loc(#loc50)
    acir.path %232 {branch = 68 : i64, kind = "branch", txn = -1 : i64} loc(#loc50)
    %233 = arith.xori %value_10, %true : i1 loc(#loc51)
    %234 = arith.andi %163, %233 : i1
    %235 = arith.ori %value_8, %168 : i1
    %236 = arith.andi %98, %235 : i1
    %237 = arith.andi %value_6, %234 : i1
    %238 = acir.extract %93 {field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc52)
    %239 = arith.andi %237, %238 : i1
    %240 = acir.extract %93 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc53)
    %241 = arith.extui %240 : i32 to i64 loc(#loc54)
    %242 = arith.cmpi ult, %241, %c32_i64 : i64 loc(#loc54)
    %243 = arith.andi %94, %242 : i1 loc(#loc54)
    acir.path %239 {branch = 69 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %value_12, %ready_13 = acir.read(%239, %241) {array = "top.rf.pending", branch = 69 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.pending.0", resources = ["top.rf.pending.0", "top.rf.pending.1", "top.rf.pending.2", "top.rf.pending.3", "top.rf.pending.4", "top.rf.pending.5", "top.rf.pending.6", "top.rf.pending.7", "top.rf.pending.8", "top.rf.pending.9", "top.rf.pending.10", "top.rf.pending.11", "top.rf.pending.12", "top.rf.pending.13", "top.rf.pending.14", "top.rf.pending.15", "top.rf.pending.16", "top.rf.pending.17", "top.rf.pending.18", "top.rf.pending.19", "top.rf.pending.20", "top.rf.pending.21", "top.rf.pending.22", "top.rf.pending.23", "top.rf.pending.24", "top.rf.pending.25", "top.rf.pending.26", "top.rf.pending.27", "top.rf.pending.28", "top.rf.pending.29", "top.rf.pending.30", "top.rf.pending.31"], txn = -1 : i64} : (i1, i64) -> (i1, i1) loc(#loc54)
    %244 = arith.cmpi eq, %241, %c0_i64 : i64 loc(#loc54)
    %245 = arith.andi %239, %244 : i1 loc(#loc54)
    acir.path %245 {branch = 70 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %246 = arith.cmpi eq, %241, %c1_i64 : i64 loc(#loc54)
    %247 = arith.andi %239, %246 : i1 loc(#loc54)
    acir.path %247 {branch = 71 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %248 = arith.cmpi eq, %241, %c2_i64 : i64 loc(#loc54)
    %249 = arith.andi %239, %248 : i1 loc(#loc54)
    acir.path %249 {branch = 72 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %250 = arith.cmpi eq, %241, %c3_i64 : i64 loc(#loc54)
    %251 = arith.andi %239, %250 : i1 loc(#loc54)
    acir.path %251 {branch = 73 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %252 = arith.cmpi eq, %241, %c4_i64 : i64 loc(#loc54)
    %253 = arith.andi %239, %252 : i1 loc(#loc54)
    acir.path %253 {branch = 74 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %254 = arith.cmpi eq, %241, %c5_i64 : i64 loc(#loc54)
    %255 = arith.andi %239, %254 : i1 loc(#loc54)
    acir.path %255 {branch = 75 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %256 = arith.cmpi eq, %241, %c6_i64 : i64 loc(#loc54)
    %257 = arith.andi %239, %256 : i1 loc(#loc54)
    acir.path %257 {branch = 76 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %258 = arith.cmpi eq, %241, %c7_i64 : i64 loc(#loc54)
    %259 = arith.andi %239, %258 : i1 loc(#loc54)
    acir.path %259 {branch = 77 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %260 = arith.cmpi eq, %241, %c8_i64 : i64 loc(#loc54)
    %261 = arith.andi %239, %260 : i1 loc(#loc54)
    acir.path %261 {branch = 78 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %262 = arith.cmpi eq, %241, %c9_i64 : i64 loc(#loc54)
    %263 = arith.andi %239, %262 : i1 loc(#loc54)
    acir.path %263 {branch = 79 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %264 = arith.cmpi eq, %241, %c10_i64 : i64 loc(#loc54)
    %265 = arith.andi %239, %264 : i1 loc(#loc54)
    acir.path %265 {branch = 80 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %266 = arith.cmpi eq, %241, %c11_i64 : i64 loc(#loc54)
    %267 = arith.andi %239, %266 : i1 loc(#loc54)
    acir.path %267 {branch = 81 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %268 = arith.cmpi eq, %241, %c12_i64 : i64 loc(#loc54)
    %269 = arith.andi %239, %268 : i1 loc(#loc54)
    acir.path %269 {branch = 82 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %270 = arith.cmpi eq, %241, %c13_i64 : i64 loc(#loc54)
    %271 = arith.andi %239, %270 : i1 loc(#loc54)
    acir.path %271 {branch = 83 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %272 = arith.cmpi eq, %241, %c14_i64 : i64 loc(#loc54)
    %273 = arith.andi %239, %272 : i1 loc(#loc54)
    acir.path %273 {branch = 84 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %274 = arith.cmpi eq, %241, %c15_i64 : i64 loc(#loc54)
    %275 = arith.andi %239, %274 : i1 loc(#loc54)
    acir.path %275 {branch = 85 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %276 = arith.cmpi eq, %241, %c16_i64 : i64 loc(#loc54)
    %277 = arith.andi %239, %276 : i1 loc(#loc54)
    acir.path %277 {branch = 86 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %278 = arith.cmpi eq, %241, %c17_i64 : i64 loc(#loc54)
    %279 = arith.andi %239, %278 : i1 loc(#loc54)
    acir.path %279 {branch = 87 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %280 = arith.cmpi eq, %241, %c18_i64 : i64 loc(#loc54)
    %281 = arith.andi %239, %280 : i1 loc(#loc54)
    acir.path %281 {branch = 88 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %282 = arith.cmpi eq, %241, %c19_i64 : i64 loc(#loc54)
    %283 = arith.andi %239, %282 : i1 loc(#loc54)
    acir.path %283 {branch = 89 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %284 = arith.cmpi eq, %241, %c20_i64 : i64 loc(#loc54)
    %285 = arith.andi %239, %284 : i1 loc(#loc54)
    acir.path %285 {branch = 90 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %286 = arith.cmpi eq, %241, %c21_i64 : i64 loc(#loc54)
    %287 = arith.andi %239, %286 : i1 loc(#loc54)
    acir.path %287 {branch = 91 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %288 = arith.cmpi eq, %241, %c22_i64 : i64 loc(#loc54)
    %289 = arith.andi %239, %288 : i1 loc(#loc54)
    acir.path %289 {branch = 92 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %290 = arith.cmpi eq, %241, %c23_i64 : i64 loc(#loc54)
    %291 = arith.andi %239, %290 : i1 loc(#loc54)
    acir.path %291 {branch = 93 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %292 = arith.cmpi eq, %241, %c24_i64 : i64 loc(#loc54)
    %293 = arith.andi %239, %292 : i1 loc(#loc54)
    acir.path %293 {branch = 94 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %294 = arith.cmpi eq, %241, %c25_i64 : i64 loc(#loc54)
    %295 = arith.andi %239, %294 : i1 loc(#loc54)
    acir.path %295 {branch = 95 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %296 = arith.cmpi eq, %241, %c26_i64 : i64 loc(#loc54)
    %297 = arith.andi %239, %296 : i1 loc(#loc54)
    acir.path %297 {branch = 96 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %298 = arith.cmpi eq, %241, %c27_i64 : i64 loc(#loc54)
    %299 = arith.andi %239, %298 : i1 loc(#loc54)
    acir.path %299 {branch = 97 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %300 = arith.cmpi eq, %241, %c28_i64 : i64 loc(#loc54)
    %301 = arith.andi %239, %300 : i1 loc(#loc54)
    acir.path %301 {branch = 98 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %302 = arith.cmpi eq, %241, %c29_i64 : i64 loc(#loc54)
    %303 = arith.andi %239, %302 : i1 loc(#loc54)
    acir.path %303 {branch = 99 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %304 = arith.cmpi eq, %241, %c30_i64 : i64 loc(#loc54)
    %305 = arith.andi %239, %304 : i1 loc(#loc54)
    acir.path %305 {branch = 100 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %306 = arith.cmpi eq, %241, %c31_i64 : i64 loc(#loc54)
    %307 = arith.andi %239, %306 : i1 loc(#loc54)
    acir.path %307 {branch = 101 : i64, kind = "branch", txn = -1 : i64} loc(#loc54)
    %308 = arith.xori %value_12, %true : i1 loc(#loc55)
    %309 = arith.xori %238, %true : i1
    %310 = arith.ori %309, %308 : i1
    %311 = arith.ori %309, %243 : i1
    %312 = arith.andi %94, %311 : i1
    %313 = arith.xori %234, %true : i1
    %314 = arith.ori %313, %94 : i1
    %315 = arith.andi %236, %314 : i1
    %316 = arith.andi %234, %310 : i1
    %317 = arith.ori %313, %312 : i1
    %318 = arith.andi %236, %317 : i1
    %319 = arith.andi %318, %315 : i1
    %320 = acir.export %316 : i1 -> i1
    %321 = acir.export %319 : i1 -> i1
    %322 = arith.andi %value_6, %320 : i1 loc(#loc56)
    acir.path %322 {branch = 102 : i64, kind = "branch", txn = 1 : i64} loc(#loc57)
    acir.path %322 {branch = 102 : i64, frames = ["top.decode_stage:read_operands"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc57)
    acir.path %322 {branch = 102 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64}
    %323 = acir.aggregate() : () -> i32
    %324 = arith.cmpi eq, %95, %c0_i32 : i32 loc(#loc58)
    %325 = arith.cmpi ne, %95, %c0_i32 : i32 loc(#loc58)
    %326 = arith.select %324, %c0_i32, %323 : i32
    "acir.resource.array"() <{name = "top.rf.registers", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7", "top.rf.registers.8", "top.rf.registers.9", "top.rf.registers.10", "top.rf.registers.11", "top.rf.registers.12", "top.rf.registers.13", "top.rf.registers.14", "top.rf.registers.15", "top.rf.registers.16", "top.rf.registers.17", "top.rf.registers.18", "top.rf.registers.19", "top.rf.registers.20", "top.rf.registers.21", "top.rf.registers.22", "top.rf.registers.23", "top.rf.registers.24", "top.rf.registers.25", "top.rf.registers.26", "top.rf.registers.27", "top.rf.registers.28", "top.rf.registers.29", "top.rf.registers.30", "top.rf.registers.31"], shape = [32]}> : () -> () loc(#loc59)
    %327 = arith.andi %322, %325 : i1
    acir.path %327 {branch = 103 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %value_14, %ready_15 = acir.read(%327, %96) {array = "top.rf.registers", branch = 103 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.registers.0", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7", "top.rf.registers.8", "top.rf.registers.9", "top.rf.registers.10", "top.rf.registers.11", "top.rf.registers.12", "top.rf.registers.13", "top.rf.registers.14", "top.rf.registers.15", "top.rf.registers.16", "top.rf.registers.17", "top.rf.registers.18", "top.rf.registers.19", "top.rf.registers.20", "top.rf.registers.21", "top.rf.registers.22", "top.rf.registers.23", "top.rf.registers.24", "top.rf.registers.25", "top.rf.registers.26", "top.rf.registers.27", "top.rf.registers.28", "top.rf.registers.29", "top.rf.registers.30", "top.rf.registers.31"], txn = 1 : i64} : (i1, i64) -> (i32, i1) loc(#loc60)
    %328 = arith.andi %327, %99 : i1 loc(#loc60)
    acir.path %328 {branch = 104 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %329 = arith.andi %327, %101 : i1 loc(#loc60)
    acir.path %329 {branch = 105 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %330 = arith.andi %327, %103 : i1 loc(#loc60)
    acir.path %330 {branch = 106 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %331 = arith.andi %327, %105 : i1 loc(#loc60)
    acir.path %331 {branch = 107 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %332 = arith.andi %327, %107 : i1 loc(#loc60)
    acir.path %332 {branch = 108 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %333 = arith.andi %327, %109 : i1 loc(#loc60)
    acir.path %333 {branch = 109 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %334 = arith.andi %327, %111 : i1 loc(#loc60)
    acir.path %334 {branch = 110 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %335 = arith.andi %327, %113 : i1 loc(#loc60)
    acir.path %335 {branch = 111 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %336 = arith.andi %327, %115 : i1 loc(#loc60)
    acir.path %336 {branch = 112 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %337 = arith.andi %327, %117 : i1 loc(#loc60)
    acir.path %337 {branch = 113 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %338 = arith.andi %327, %119 : i1 loc(#loc60)
    acir.path %338 {branch = 114 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %339 = arith.andi %327, %121 : i1 loc(#loc60)
    acir.path %339 {branch = 115 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %340 = arith.andi %327, %123 : i1 loc(#loc60)
    acir.path %340 {branch = 116 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %341 = arith.andi %327, %125 : i1 loc(#loc60)
    acir.path %341 {branch = 117 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %342 = arith.andi %327, %127 : i1 loc(#loc60)
    acir.path %342 {branch = 118 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %343 = arith.andi %327, %129 : i1 loc(#loc60)
    acir.path %343 {branch = 119 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %344 = arith.andi %327, %131 : i1 loc(#loc60)
    acir.path %344 {branch = 120 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %345 = arith.andi %327, %133 : i1 loc(#loc60)
    acir.path %345 {branch = 121 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %346 = arith.andi %327, %135 : i1 loc(#loc60)
    acir.path %346 {branch = 122 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %347 = arith.andi %327, %137 : i1 loc(#loc60)
    acir.path %347 {branch = 123 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %348 = arith.andi %327, %139 : i1 loc(#loc60)
    acir.path %348 {branch = 124 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %349 = arith.andi %327, %141 : i1 loc(#loc60)
    acir.path %349 {branch = 125 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %350 = arith.andi %327, %143 : i1 loc(#loc60)
    acir.path %350 {branch = 126 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %351 = arith.andi %327, %145 : i1 loc(#loc60)
    acir.path %351 {branch = 127 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %352 = arith.andi %327, %147 : i1 loc(#loc60)
    acir.path %352 {branch = 128 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %353 = arith.andi %327, %149 : i1 loc(#loc60)
    acir.path %353 {branch = 129 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %354 = arith.andi %327, %151 : i1 loc(#loc60)
    acir.path %354 {branch = 130 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %355 = arith.andi %327, %153 : i1 loc(#loc60)
    acir.path %355 {branch = 131 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %356 = arith.andi %327, %155 : i1 loc(#loc60)
    acir.path %356 {branch = 132 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %357 = arith.andi %327, %157 : i1 loc(#loc60)
    acir.path %357 {branch = 133 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %358 = arith.andi %327, %159 : i1 loc(#loc60)
    acir.path %358 {branch = 134 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %359 = arith.andi %327, %161 : i1 loc(#loc60)
    acir.path %359 {branch = 135 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %360 = arith.select %325, %value_14, %326 : i32 loc(#loc61)
    %361 = arith.select %325, %98, %94 : i1
    %362 = arith.andi %94, %361 : i1
    acir.path %322 {branch = 102 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64}
    %363 = arith.cmpi eq, %165, %c0_i32 : i32 loc(#loc58)
    %364 = arith.cmpi ne, %165, %c0_i32 : i32 loc(#loc58)
    %365 = arith.select %363, %c0_i32, %323 : i32
    "acir.resource.array"() <{name = "top.rf.registers", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7", "top.rf.registers.8", "top.rf.registers.9", "top.rf.registers.10", "top.rf.registers.11", "top.rf.registers.12", "top.rf.registers.13", "top.rf.registers.14", "top.rf.registers.15", "top.rf.registers.16", "top.rf.registers.17", "top.rf.registers.18", "top.rf.registers.19", "top.rf.registers.20", "top.rf.registers.21", "top.rf.registers.22", "top.rf.registers.23", "top.rf.registers.24", "top.rf.registers.25", "top.rf.registers.26", "top.rf.registers.27", "top.rf.registers.28", "top.rf.registers.29", "top.rf.registers.30", "top.rf.registers.31"], shape = [32]}> : () -> () loc(#loc59)
    %366 = arith.andi %322, %364 : i1
    acir.path %366 {branch = 136 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %value_16, %ready_17 = acir.read(%366, %166) {array = "top.rf.registers", branch = 136 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "read", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.registers.0", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7", "top.rf.registers.8", "top.rf.registers.9", "top.rf.registers.10", "top.rf.registers.11", "top.rf.registers.12", "top.rf.registers.13", "top.rf.registers.14", "top.rf.registers.15", "top.rf.registers.16", "top.rf.registers.17", "top.rf.registers.18", "top.rf.registers.19", "top.rf.registers.20", "top.rf.registers.21", "top.rf.registers.22", "top.rf.registers.23", "top.rf.registers.24", "top.rf.registers.25", "top.rf.registers.26", "top.rf.registers.27", "top.rf.registers.28", "top.rf.registers.29", "top.rf.registers.30", "top.rf.registers.31"], txn = 1 : i64} : (i1, i64) -> (i32, i1) loc(#loc60)
    %367 = arith.andi %366, %169 : i1 loc(#loc60)
    acir.path %367 {branch = 137 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %368 = arith.andi %366, %171 : i1 loc(#loc60)
    acir.path %368 {branch = 138 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %369 = arith.andi %366, %173 : i1 loc(#loc60)
    acir.path %369 {branch = 139 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %370 = arith.andi %366, %175 : i1 loc(#loc60)
    acir.path %370 {branch = 140 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %371 = arith.andi %366, %177 : i1 loc(#loc60)
    acir.path %371 {branch = 141 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %372 = arith.andi %366, %179 : i1 loc(#loc60)
    acir.path %372 {branch = 142 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %373 = arith.andi %366, %181 : i1 loc(#loc60)
    acir.path %373 {branch = 143 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %374 = arith.andi %366, %183 : i1 loc(#loc60)
    acir.path %374 {branch = 144 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %375 = arith.andi %366, %185 : i1 loc(#loc60)
    acir.path %375 {branch = 145 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %376 = arith.andi %366, %187 : i1 loc(#loc60)
    acir.path %376 {branch = 146 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %377 = arith.andi %366, %189 : i1 loc(#loc60)
    acir.path %377 {branch = 147 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %378 = arith.andi %366, %191 : i1 loc(#loc60)
    acir.path %378 {branch = 148 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %379 = arith.andi %366, %193 : i1 loc(#loc60)
    acir.path %379 {branch = 149 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %380 = arith.andi %366, %195 : i1 loc(#loc60)
    acir.path %380 {branch = 150 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %381 = arith.andi %366, %197 : i1 loc(#loc60)
    acir.path %381 {branch = 151 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %382 = arith.andi %366, %199 : i1 loc(#loc60)
    acir.path %382 {branch = 152 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %383 = arith.andi %366, %201 : i1 loc(#loc60)
    acir.path %383 {branch = 153 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %384 = arith.andi %366, %203 : i1 loc(#loc60)
    acir.path %384 {branch = 154 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %385 = arith.andi %366, %205 : i1 loc(#loc60)
    acir.path %385 {branch = 155 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %386 = arith.andi %366, %207 : i1 loc(#loc60)
    acir.path %386 {branch = 156 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %387 = arith.andi %366, %209 : i1 loc(#loc60)
    acir.path %387 {branch = 157 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %388 = arith.andi %366, %211 : i1 loc(#loc60)
    acir.path %388 {branch = 158 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %389 = arith.andi %366, %213 : i1 loc(#loc60)
    acir.path %389 {branch = 159 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %390 = arith.andi %366, %215 : i1 loc(#loc60)
    acir.path %390 {branch = 160 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %391 = arith.andi %366, %217 : i1 loc(#loc60)
    acir.path %391 {branch = 161 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %392 = arith.andi %366, %219 : i1 loc(#loc60)
    acir.path %392 {branch = 162 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %393 = arith.andi %366, %221 : i1 loc(#loc60)
    acir.path %393 {branch = 163 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %394 = arith.andi %366, %223 : i1 loc(#loc60)
    acir.path %394 {branch = 164 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %395 = arith.andi %366, %225 : i1 loc(#loc60)
    acir.path %395 {branch = 165 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %396 = arith.andi %366, %227 : i1 loc(#loc60)
    acir.path %396 {branch = 166 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %397 = arith.andi %366, %229 : i1 loc(#loc60)
    acir.path %397 {branch = 167 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %398 = arith.andi %366, %231 : i1 loc(#loc60)
    acir.path %398 {branch = 168 : i64, kind = "branch", txn = 1 : i64} loc(#loc60)
    %399 = arith.select %364, %value_16, %365 : i32 loc(#loc62)
    %400 = arith.select %364, %168, %94 : i1
    %401 = arith.andi %94, %400 : i1
    %402 = acir.extract %93 {field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc63)
    %403 = acir.extract %93 {field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc64)
    %404 = arith.select %402, %403, %399 : i32 loc(#loc65)
    %405 = arith.select %402, %94, %401 : i1
    %406 = arith.andi %94, %405 : i1
    acir.path %322 {branch = 102 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc66)
    %407 = arith.cmpi ne, %240, %c0_i32 : i32 loc(#loc67)
    %408 = arith.andi %238, %407 : i1
    "acir.resource.array"() <{name = "top.rf.pending", resources = ["top.rf.pending.0", "top.rf.pending.1", "top.rf.pending.2", "top.rf.pending.3", "top.rf.pending.4", "top.rf.pending.5", "top.rf.pending.6", "top.rf.pending.7", "top.rf.pending.8", "top.rf.pending.9", "top.rf.pending.10", "top.rf.pending.11", "top.rf.pending.12", "top.rf.pending.13", "top.rf.pending.14", "top.rf.pending.15", "top.rf.pending.16", "top.rf.pending.17", "top.rf.pending.18", "top.rf.pending.19", "top.rf.pending.20", "top.rf.pending.21", "top.rf.pending.22", "top.rf.pending.23", "top.rf.pending.24", "top.rf.pending.25", "top.rf.pending.26", "top.rf.pending.27", "top.rf.pending.28", "top.rf.pending.29", "top.rf.pending.30", "top.rf.pending.31"], shape = [32]}> : () -> () loc(#loc46)
    %409 = arith.andi %322, %408 : i1
    acir.path %409 {branch = 169 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %410 = arith.andi %409, %244 : i1 loc(#loc68)
    acir.path %410 {branch = 170 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %411 = arith.andi %409, %246 : i1 loc(#loc68)
    acir.path %411 {branch = 171 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %412 = arith.andi %409, %248 : i1 loc(#loc68)
    acir.path %412 {branch = 172 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %413 = arith.andi %409, %250 : i1 loc(#loc68)
    acir.path %413 {branch = 173 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %414 = arith.andi %409, %252 : i1 loc(#loc68)
    acir.path %414 {branch = 174 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %415 = arith.andi %409, %254 : i1 loc(#loc68)
    acir.path %415 {branch = 175 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %416 = arith.andi %409, %256 : i1 loc(#loc68)
    acir.path %416 {branch = 176 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %417 = arith.andi %409, %258 : i1 loc(#loc68)
    acir.path %417 {branch = 177 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %418 = arith.andi %409, %260 : i1 loc(#loc68)
    acir.path %418 {branch = 178 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %419 = arith.andi %409, %262 : i1 loc(#loc68)
    acir.path %419 {branch = 179 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %420 = arith.andi %409, %264 : i1 loc(#loc68)
    acir.path %420 {branch = 180 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %421 = arith.andi %409, %266 : i1 loc(#loc68)
    acir.path %421 {branch = 181 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %422 = arith.andi %409, %268 : i1 loc(#loc68)
    acir.path %422 {branch = 182 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %423 = arith.andi %409, %270 : i1 loc(#loc68)
    acir.path %423 {branch = 183 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %424 = arith.andi %409, %272 : i1 loc(#loc68)
    acir.path %424 {branch = 184 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %425 = arith.andi %409, %274 : i1 loc(#loc68)
    acir.path %425 {branch = 185 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %426 = arith.andi %409, %276 : i1 loc(#loc68)
    acir.path %426 {branch = 186 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %427 = arith.andi %409, %278 : i1 loc(#loc68)
    acir.path %427 {branch = 187 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %428 = arith.andi %409, %280 : i1 loc(#loc68)
    acir.path %428 {branch = 188 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %429 = arith.andi %409, %282 : i1 loc(#loc68)
    acir.path %429 {branch = 189 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %430 = arith.andi %409, %284 : i1 loc(#loc68)
    acir.path %430 {branch = 190 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %431 = arith.andi %409, %286 : i1 loc(#loc68)
    acir.path %431 {branch = 191 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %432 = arith.andi %409, %288 : i1 loc(#loc68)
    acir.path %432 {branch = 192 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %433 = arith.andi %409, %290 : i1 loc(#loc68)
    acir.path %433 {branch = 193 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %434 = arith.andi %409, %292 : i1 loc(#loc68)
    acir.path %434 {branch = 194 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %435 = arith.andi %409, %294 : i1 loc(#loc68)
    acir.path %435 {branch = 195 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %436 = arith.andi %409, %296 : i1 loc(#loc68)
    acir.path %436 {branch = 196 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %437 = arith.andi %409, %298 : i1 loc(#loc68)
    acir.path %437 {branch = 197 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %438 = arith.andi %409, %300 : i1 loc(#loc68)
    acir.path %438 {branch = 198 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %439 = arith.andi %409, %302 : i1 loc(#loc68)
    acir.path %439 {branch = 199 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %440 = arith.andi %409, %304 : i1 loc(#loc68)
    acir.path %440 {branch = 200 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %441 = arith.andi %409, %306 : i1 loc(#loc68)
    acir.path %441 {branch = 201 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    %442 = acir.extract %93 {field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc69)
    %443 = acir.extract %93 {field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc70)
    %444 = acir.extract %93 {field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc71)
    %445 = arith.andi %362, %406 : i1 loc(#loc72)
    %446 = arith.andi %445, %401 : i1 loc(#loc72)
    %447 = acir.aggregate(%442, %443, %444, %240, %360, %404, %399, %238) : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc72)
    %448 = arith.andi %322, %446 : i1
    acir.path %448 {branch = 202 : i64, kind = "branch", txn = 1 : i64}
    %value_18, %ready_19 = acir.read(%448) {branch = 202 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.id_ex", txn = 1 : i64} : (i1) -> (i1, i1)
    %449 = arith.andi %ready_5, %29 : i1
    %450 = arith.andi %308, %449 : i1
    %451 = arith.select %238, %450, %449 : i1
    %452 = arith.andi %233, %451 : i1
    %453 = arith.andi %163, %452 : i1
    %454 = arith.andi %value_6, %453 : i1
    %455 = arith.select %454, %79, %true : i1 loc(#loc144)
    %456 = arith.cmpi ne, %19, %c19_i32 : i32 loc(#loc129)
    %457 = arith.andi %456, %30 : i1
    %458 = arith.andi %ready_5, %457 : i1
    %459 = arith.andi %308, %458 : i1
    %460 = arith.select %238, %459, %458 : i1
    %461 = arith.andi %233, %460 : i1
    %462 = arith.andi %163, %461 : i1
    %463 = arith.andi %value_6, %462 : i1
    %464 = arith.select %463, %66, %true : i1 loc(#loc143)
    %465 = arith.andi %73, %31 : i1
    %466 = arith.andi %456, %465 : i1
    %467 = arith.andi %ready_5, %466 : i1
    %468 = arith.andi %308, %467 : i1
    %469 = arith.select %238, %468, %467 : i1
    %470 = arith.andi %233, %469 : i1
    %471 = arith.andi %163, %470 : i1
    %472 = arith.andi %value_6, %471 : i1
    %473 = arith.select %472, %47, %true : i1 loc(#loc139)
    %474 = arith.cmpi ne, %19, %c3_i32 : i32 loc(#loc131)
    %475 = arith.andi %73, %474 : i1
    %476 = arith.andi %456, %475 : i1
    %477 = arith.andi %ready_5, %476 : i1
    %478 = arith.andi %308, %477 : i1
    %479 = arith.select %238, %478, %477 : i1
    %480 = arith.andi %233, %479 : i1
    %481 = arith.andi %163, %480 : i1
    %482 = arith.andi %value_6, %481 : i1
    %483 = arith.select %482, %41, %true : i1 loc(#loc136)
    %484 = arith.xori %value_18, %true : i1
    %485 = arith.andi %242, %484 : i1
    %486 = arith.andi %167, %485 : i1
    %487 = arith.andi %97, %486 : i1
    %488 = arith.andi %ready_5, %487 : i1
    %489 = arith.andi %308, %488 : i1
    %490 = arith.andi %167, %484 : i1
    %491 = arith.andi %97, %490 : i1
    %492 = arith.andi %ready_5, %491 : i1
    %493 = arith.select %238, %489, %492 : i1
    %494 = arith.andi %233, %493 : i1
    %495 = arith.andi %163, %494 : i1
    %496 = arith.andi %value_6, %495 : i1
    %497 = acir.fire "top.decode_stage.read_operands#1" %496 checks(%455, %464, %473, %483) {successors = [0], txn = 1 : i64}
    %498 = arith.andi %233, %310 : i1
    %499 = arith.andi %163, %498 : i1
    %500 = arith.andi %value_6, %499 : i1
    %501 = arith.andi %497, %500 : i1
    acir.effect %501() {branch = 102 : i64, frames = ["top.decode_stage:read_operands"], method = "pop", port = 0 : i64, resource = "top.if_id", txn = 1 : i64} : (i1) -> ()
    %502 = arith.andi %308, %407 : i1
    %503 = arith.andi %238, %502 : i1
    %504 = arith.andi %233, %503 : i1
    %505 = arith.andi %163, %504 : i1
    %506 = arith.andi %value_6, %505 : i1
    %507 = arith.andi %497, %506 : i1
    acir.effect %507(%true, %241) {array = "top.rf.pending", branch = 169 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "write", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.pending.0", resources = ["top.rf.pending.0", "top.rf.pending.1", "top.rf.pending.2", "top.rf.pending.3", "top.rf.pending.4", "top.rf.pending.5", "top.rf.pending.6", "top.rf.pending.7", "top.rf.pending.8", "top.rf.pending.9", "top.rf.pending.10", "top.rf.pending.11", "top.rf.pending.12", "top.rf.pending.13", "top.rf.pending.14", "top.rf.pending.15", "top.rf.pending.16", "top.rf.pending.17", "top.rf.pending.18", "top.rf.pending.19", "top.rf.pending.20", "top.rf.pending.21", "top.rf.pending.22", "top.rf.pending.23", "top.rf.pending.24", "top.rf.pending.25", "top.rf.pending.26", "top.rf.pending.27", "top.rf.pending.28", "top.rf.pending.29", "top.rf.pending.30", "top.rf.pending.31"], txn = 1 : i64} : (i1, i1, i64) -> () loc(#loc68)
    %508 = arith.ori %363, %167 : i1
    %509 = arith.andi %97, %508 : i1
    %510 = arith.select %325, %509, %508 : i1
    %511 = arith.andi %79, %510 : i1
    %512 = arith.andi %66, %510 : i1
    %513 = arith.andi %47, %510 : i1
    %514 = arith.andi %41, %510 : i1
    %515 = arith.select %31, %513, %514 : i1
    %516 = arith.select %30, %512, %515 : i1
    %517 = arith.select %29, %511, %516 : i1
    %518 = arith.andi %ready_5, %517 : i1
    %519 = arith.andi %308, %518 : i1
    %520 = arith.select %238, %519, %518 : i1
    %521 = arith.andi %233, %520 : i1
    %522 = arith.andi %163, %521 : i1
    %523 = arith.andi %value_6, %522 : i1
    %524 = arith.andi %497, %523 : i1
    acir.effect %524(%447) {branch = 202 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.id_ex", txn = 1 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Operands">) -> ()
    acir.path %true {branch = 203 : i64, kind = "branch", txn = 2 : i64} loc(#loc73)
    acir.path %true {branch = 203 : i64, frames = ["top.execute:execute"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc73)
    %value_20, %ready_21 = acir.read(%true) {branch = 203 : i64, frames = ["top.execute:execute"], method = "peek", port = 0 : i64, resource = "top.id_ex", txn = 2 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Operands">, i1)
    %525 = acir.bind %value_20 : (!acir.struct<"src_isa_f0424bc8.Operands">) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc74)
    %526 = acir.extract %525 {field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc75)
    %527 = arith.cmpi eq, %526, %c1_i32 : i32 loc(#loc75)
    %528 = acir.extract %525 {field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc76)
    %529 = acir.extract %525 {field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc77)
    %530 = arith.addi %528, %529 : i32 loc(#loc76)
    %531 = arith.subi %528, %529 : i32 loc(#loc78)
    %532 = arith.select %527, %531, %530 : i32 loc(#loc79)
    %533 = acir.extract %525 {field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc80)
    %534 = acir.extract %525 {field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc81)
    %535 = acir.extract %525 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc82)
    %536 = acir.extract %525 {field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc83)
    %537 = acir.extract %525 {field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc84)
    %538 = acir.aggregate(%533, %534, %526, %535, %532, %536, %537) : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc85)
    acir.path %ready_21 {branch = 204 : i64, kind = "branch", txn = 2 : i64}
    %value_22, %ready_23 = acir.read(%ready_21) {branch = 204 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.ex_mem", txn = 2 : i64} : (i1) -> (i1, i1)
    %539 = arith.xori %value_22, %true : i1
    %540 = arith.andi %ready_21, %539 : i1
    %541 = acir.fire "top.execute.execute#2" %540 {successors = [1], txn = 2 : i64}
    acir.effect %541() {branch = 203 : i64, frames = ["top.execute:execute"], method = "pop", port = 0 : i64, resource = "top.id_ex", txn = 2 : i64} : (i1) -> ()
    %542 = arith.andi %541, %ready_21 : i1
    acir.effect %542(%538) {branch = 204 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.ex_mem", txn = 2 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Executed">) -> ()
    acir.path %true {branch = 205 : i64, kind = "branch", txn = 3 : i64} loc(#loc86)
    acir.path %true {branch = 205 : i64, frames = ["top.memory_stage:access"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc86)
    %value_24, %ready_25 = acir.read(%true) {branch = 205 : i64, frames = ["top.memory_stage:access"], method = "peek", port = 0 : i64, resource = "top.ex_mem", txn = 3 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Executed">, i1)
    %543 = acir.bind %value_24 : (!acir.struct<"src_isa_f0424bc8.Executed">) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc87)
    %544 = acir.extract %543 {field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc88)
    %545 = acir.extract %543 {field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc89)
    %546 = arith.cmpi eq, %545, %c2_i32 : i32 loc(#loc89)
    %547 = arith.cmpi ne, %545, %c2_i32 : i32 loc(#loc89)
    %548 = arith.ori %546, %ready_25 : i1
    %549 = arith.cmpi eq, %545, %c3_i32 : i32 loc(#loc90)
    %550 = arith.andi %548, %ready_25 : i1
    %551 = arith.andi %547, %549 : i1
    %552 = acir.extract %543 {field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc91)
    acir.path %551 {branch = 206 : i64, kind = "branch", txn = 3 : i64} loc(#loc92)
    acir.path %551 {branch = 206 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc92)
    %553 = arith.andi %544, %c3_i32 : i32 loc(#loc93)
    %554 = arith.cmpi eq, %553, %c0_i32 : i32 loc(#loc93)
    %555 = arith.cmpi ult, %544, %c64_i32 : i32 loc(#loc94)
    %556 = arith.andi %554, %555 : i1
    %value_26, %ready_27 = acir.read(%551) {branch = 206 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "read", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1) -> (!acir.array<i32, [16]>, i1) loc(#loc95)
    %557 = arith.shrui %544, %c2_i32 : i32 loc(#loc96)
    %558 = arith.extui %557 : i32 to i64 loc(#loc95)
    %559 = arith.cmpi ult, %558, %c16_i64 : i64 loc(#loc95)
    %560 = arith.andi %ready_25, %559 : i1 loc(#loc95)
    %561 = arith.select %559, %558, %c0_i64 : i64 loc(#loc95)
    %562 = acir.update %value_26, %552[%561] {path = [unit], signed_indices = [false]} : (!acir.array<i32, [16]>, i32, i64) -> !acir.array<i32, [16]> loc(#loc95)
    acir.path %546 {branch = 207 : i64, kind = "branch", txn = 3 : i64}
    acir.path %546 {branch = 207 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64}
    %value_28, %ready_29 = acir.read(%546) {branch = 207 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "read", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1) -> (!acir.array<i32, [16]>, i1) loc(#loc97)
    %563 = "acir.array.index"(%value_28, %561) : (!acir.array<i32, [16]>, i64) -> i32 loc(#loc88)
    %564 = arith.select %546, %563, %544 : i32 loc(#loc88)
    %565 = arith.select %546, %560, %ready_25 : i1
    %566 = arith.andi %ready_25, %565 : i1
    %567 = acir.extract %543 {field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc98)
    %568 = acir.extract %543 {field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc99)
    %569 = acir.extract %543 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc100)
    %570 = acir.extract %543 {field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc101)
    %571 = acir.aggregate(%567, %568, %569, %564, %570) : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc102)
    %572 = arith.andi %566, %550 : i1
    acir.path %572 {branch = 208 : i64, kind = "branch", txn = 3 : i64}
    %value_30, %ready_31 = acir.read(%572) {branch = 208 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.mem_wb", txn = 3 : i64} : (i1) -> (i1, i1)
    %573 = arith.cmpi uge, %558, %c16_i64 : i64 loc(#loc95)
    %574 = arith.xori %value_30, %true : i1
    %575 = arith.ori %573, %574 : i1
    %576 = arith.andi %559, %574 : i1
    %577 = arith.select %549, %576, %574 : i1
    %578 = arith.select %546, %575, %577 : i1
    %579 = arith.andi %ready_25, %578 : i1
    %580 = acir.fire "top.memory_stage.access#3" %579 {successors = [2], txn = 3 : i64}
    acir.effect %580() {branch = 205 : i64, frames = ["top.memory_stage:access"], method = "pop", port = 0 : i64, resource = "top.ex_mem", txn = 3 : i64} : (i1) -> ()
    %581 = arith.andi %580, %551 : i1
    acir.effect %581(%556) {branch = 206 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc103)
    acir.effect %581(%562) {branch = 206 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "write", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1, !acir.array<i32, [16]>) -> () loc(#loc95)
    %582 = arith.andi %580, %546 : i1
    acir.effect %582(%556) {branch = 207 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc104)
    %583 = arith.ori %547, %559 : i1
    %584 = arith.andi %ready_25, %583 : i1
    %585 = arith.andi %580, %584 : i1
    acir.effect %585(%571) {branch = 208 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.mem_wb", txn = 3 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Retired">) -> ()
    acir.path %true {branch = 209 : i64, kind = "branch", txn = 4 : i64} loc(#loc105)
    acir.path %true {branch = 209 : i64, frames = ["top.writeback:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc105)
    %value_32, %ready_33 = acir.read(%true) {branch = 209 : i64, frames = ["top.writeback:commit"], method = "peek", port = 0 : i64, resource = "top.mem_wb", txn = 4 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Retired">, i1)
    %586 = acir.bind %value_32 : (!acir.struct<"src_isa_f0424bc8.Retired">) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc106)
    acir.path %true {branch = 209 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc107)
    %587 = acir.extract %586 {field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc108)
    %588 = acir.extract %586 {field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc109)
    %589 = arith.cmpi ne, %588, %c0_i32 : i32 loc(#loc109)
    %590 = arith.andi %587, %589 : i1
    "acir.resource.array"() <{name = "top.rf.registers", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7", "top.rf.registers.8", "top.rf.registers.9", "top.rf.registers.10", "top.rf.registers.11", "top.rf.registers.12", "top.rf.registers.13", "top.rf.registers.14", "top.rf.registers.15", "top.rf.registers.16", "top.rf.registers.17", "top.rf.registers.18", "top.rf.registers.19", "top.rf.registers.20", "top.rf.registers.21", "top.rf.registers.22", "top.rf.registers.23", "top.rf.registers.24", "top.rf.registers.25", "top.rf.registers.26", "top.rf.registers.27", "top.rf.registers.28", "top.rf.registers.29", "top.rf.registers.30", "top.rf.registers.31"], shape = [32]}> : () -> () loc(#loc59)
    "acir.resource.array"() <{name = "top.rf.pending", resources = ["top.rf.pending.0", "top.rf.pending.1", "top.rf.pending.2", "top.rf.pending.3", "top.rf.pending.4", "top.rf.pending.5", "top.rf.pending.6", "top.rf.pending.7", "top.rf.pending.8", "top.rf.pending.9", "top.rf.pending.10", "top.rf.pending.11", "top.rf.pending.12", "top.rf.pending.13", "top.rf.pending.14", "top.rf.pending.15", "top.rf.pending.16", "top.rf.pending.17", "top.rf.pending.18", "top.rf.pending.19", "top.rf.pending.20", "top.rf.pending.21", "top.rf.pending.22", "top.rf.pending.23", "top.rf.pending.24", "top.rf.pending.25", "top.rf.pending.26", "top.rf.pending.27", "top.rf.pending.28", "top.rf.pending.29", "top.rf.pending.30", "top.rf.pending.31"], shape = [32]}> : () -> () loc(#loc46)
    %591 = acir.extract %586 {field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc110)
    %592 = arith.extui %588 : i32 to i64 loc(#loc111)
    %593 = arith.cmpi ult, %592, %c32_i64 : i64 loc(#loc111)
    acir.path %590 {branch = 210 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %594 = arith.cmpi eq, %592, %c0_i64 : i64 loc(#loc111)
    %595 = arith.andi %590, %594 : i1 loc(#loc111)
    acir.path %595 {branch = 211 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %596 = arith.cmpi eq, %592, %c1_i64 : i64 loc(#loc111)
    %597 = arith.andi %590, %596 : i1 loc(#loc111)
    acir.path %597 {branch = 212 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %598 = arith.cmpi eq, %592, %c2_i64 : i64 loc(#loc111)
    %599 = arith.andi %590, %598 : i1 loc(#loc111)
    acir.path %599 {branch = 213 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %600 = arith.cmpi eq, %592, %c3_i64 : i64 loc(#loc111)
    %601 = arith.andi %590, %600 : i1 loc(#loc111)
    acir.path %601 {branch = 214 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %602 = arith.cmpi eq, %592, %c4_i64 : i64 loc(#loc111)
    %603 = arith.andi %590, %602 : i1 loc(#loc111)
    acir.path %603 {branch = 215 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %604 = arith.cmpi eq, %592, %c5_i64 : i64 loc(#loc111)
    %605 = arith.andi %590, %604 : i1 loc(#loc111)
    acir.path %605 {branch = 216 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %606 = arith.cmpi eq, %592, %c6_i64 : i64 loc(#loc111)
    %607 = arith.andi %590, %606 : i1 loc(#loc111)
    acir.path %607 {branch = 217 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %608 = arith.cmpi eq, %592, %c7_i64 : i64 loc(#loc111)
    %609 = arith.andi %590, %608 : i1 loc(#loc111)
    acir.path %609 {branch = 218 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %610 = arith.cmpi eq, %592, %c8_i64 : i64 loc(#loc111)
    %611 = arith.andi %590, %610 : i1 loc(#loc111)
    acir.path %611 {branch = 219 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %612 = arith.cmpi eq, %592, %c9_i64 : i64 loc(#loc111)
    %613 = arith.andi %590, %612 : i1 loc(#loc111)
    acir.path %613 {branch = 220 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %614 = arith.cmpi eq, %592, %c10_i64 : i64 loc(#loc111)
    %615 = arith.andi %590, %614 : i1 loc(#loc111)
    acir.path %615 {branch = 221 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %616 = arith.cmpi eq, %592, %c11_i64 : i64 loc(#loc111)
    %617 = arith.andi %590, %616 : i1 loc(#loc111)
    acir.path %617 {branch = 222 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %618 = arith.cmpi eq, %592, %c12_i64 : i64 loc(#loc111)
    %619 = arith.andi %590, %618 : i1 loc(#loc111)
    acir.path %619 {branch = 223 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %620 = arith.cmpi eq, %592, %c13_i64 : i64 loc(#loc111)
    %621 = arith.andi %590, %620 : i1 loc(#loc111)
    acir.path %621 {branch = 224 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %622 = arith.cmpi eq, %592, %c14_i64 : i64 loc(#loc111)
    %623 = arith.andi %590, %622 : i1 loc(#loc111)
    acir.path %623 {branch = 225 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %624 = arith.cmpi eq, %592, %c15_i64 : i64 loc(#loc111)
    %625 = arith.andi %590, %624 : i1 loc(#loc111)
    acir.path %625 {branch = 226 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %626 = arith.cmpi eq, %592, %c16_i64 : i64 loc(#loc111)
    %627 = arith.andi %590, %626 : i1 loc(#loc111)
    acir.path %627 {branch = 227 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %628 = arith.cmpi eq, %592, %c17_i64 : i64 loc(#loc111)
    %629 = arith.andi %590, %628 : i1 loc(#loc111)
    acir.path %629 {branch = 228 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %630 = arith.cmpi eq, %592, %c18_i64 : i64 loc(#loc111)
    %631 = arith.andi %590, %630 : i1 loc(#loc111)
    acir.path %631 {branch = 229 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %632 = arith.cmpi eq, %592, %c19_i64 : i64 loc(#loc111)
    %633 = arith.andi %590, %632 : i1 loc(#loc111)
    acir.path %633 {branch = 230 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %634 = arith.cmpi eq, %592, %c20_i64 : i64 loc(#loc111)
    %635 = arith.andi %590, %634 : i1 loc(#loc111)
    acir.path %635 {branch = 231 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %636 = arith.cmpi eq, %592, %c21_i64 : i64 loc(#loc111)
    %637 = arith.andi %590, %636 : i1 loc(#loc111)
    acir.path %637 {branch = 232 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %638 = arith.cmpi eq, %592, %c22_i64 : i64 loc(#loc111)
    %639 = arith.andi %590, %638 : i1 loc(#loc111)
    acir.path %639 {branch = 233 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %640 = arith.cmpi eq, %592, %c23_i64 : i64 loc(#loc111)
    %641 = arith.andi %590, %640 : i1 loc(#loc111)
    acir.path %641 {branch = 234 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %642 = arith.cmpi eq, %592, %c24_i64 : i64 loc(#loc111)
    %643 = arith.andi %590, %642 : i1 loc(#loc111)
    acir.path %643 {branch = 235 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %644 = arith.cmpi eq, %592, %c25_i64 : i64 loc(#loc111)
    %645 = arith.andi %590, %644 : i1 loc(#loc111)
    acir.path %645 {branch = 236 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %646 = arith.cmpi eq, %592, %c26_i64 : i64 loc(#loc111)
    %647 = arith.andi %590, %646 : i1 loc(#loc111)
    acir.path %647 {branch = 237 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %648 = arith.cmpi eq, %592, %c27_i64 : i64 loc(#loc111)
    %649 = arith.andi %590, %648 : i1 loc(#loc111)
    acir.path %649 {branch = 238 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %650 = arith.cmpi eq, %592, %c28_i64 : i64 loc(#loc111)
    %651 = arith.andi %590, %650 : i1 loc(#loc111)
    acir.path %651 {branch = 239 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %652 = arith.cmpi eq, %592, %c29_i64 : i64 loc(#loc111)
    %653 = arith.andi %590, %652 : i1 loc(#loc111)
    acir.path %653 {branch = 240 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %654 = arith.cmpi eq, %592, %c30_i64 : i64 loc(#loc111)
    %655 = arith.andi %590, %654 : i1 loc(#loc111)
    acir.path %655 {branch = 241 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    %656 = arith.cmpi eq, %592, %c31_i64 : i64 loc(#loc111)
    %657 = arith.andi %590, %656 : i1 loc(#loc111)
    acir.path %657 {branch = 242 : i64, kind = "branch", txn = 4 : i64} loc(#loc111)
    acir.path %595 {branch = 243 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %597 {branch = 244 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %599 {branch = 245 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %601 {branch = 246 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %603 {branch = 247 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %605 {branch = 248 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %607 {branch = 249 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %609 {branch = 250 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %611 {branch = 251 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %613 {branch = 252 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %615 {branch = 253 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %617 {branch = 254 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %619 {branch = 255 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %621 {branch = 256 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %623 {branch = 257 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %625 {branch = 258 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %627 {branch = 259 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %629 {branch = 260 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %631 {branch = 261 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %633 {branch = 262 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %635 {branch = 263 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %637 {branch = 264 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %639 {branch = 265 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %641 {branch = 266 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %643 {branch = 267 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %645 {branch = 268 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %647 {branch = 269 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %649 {branch = 270 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %651 {branch = 271 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %653 {branch = 272 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %655 {branch = 273 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    acir.path %657 {branch = 274 : i64, kind = "branch", txn = 4 : i64} loc(#loc112)
    %value_34, %ready_35 = acir.read(%true) {branch = 209 : i64, frames = ["top.writeback:commit"], method = "read", port = 0 : i64, resource = "top.writeback.retired", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc113)
    %658 = arith.addi %value_34, %c1_i32 : i32 loc(#loc113)
    %659 = acir.extract %586 {field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc114)
    %660 = arith.xori %587, %true : i1 loc(#loc111)
    %661 = arith.cmpi eq, %588, %c0_i32 : i32 loc(#loc148)
    %662 = arith.ori %661, %593 : i1 loc(#loc111)
    %663 = arith.ori %660, %662 : i1 loc(#loc111)
    %664 = arith.andi %ready_33, %663 : i1 loc(#loc111)
    %665 = acir.fire "top.writeback.commit#4" %664 {successors = [3], txn = 4 : i64}
    acir.effect %665() {branch = 209 : i64, frames = ["top.writeback:commit"], method = "pop", port = 0 : i64, resource = "top.mem_wb", txn = 4 : i64} : (i1) -> ()
    %666 = arith.andi %665, %590 : i1
    acir.effect %666(%591, %592) {array = "top.rf.registers", branch = 210 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "write", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.registers.0", resources = ["top.rf.registers.0", "top.rf.registers.1", "top.rf.registers.2", "top.rf.registers.3", "top.rf.registers.4", "top.rf.registers.5", "top.rf.registers.6", "top.rf.registers.7", "top.rf.registers.8", "top.rf.registers.9", "top.rf.registers.10", "top.rf.registers.11", "top.rf.registers.12", "top.rf.registers.13", "top.rf.registers.14", "top.rf.registers.15", "top.rf.registers.16", "top.rf.registers.17", "top.rf.registers.18", "top.rf.registers.19", "top.rf.registers.20", "top.rf.registers.21", "top.rf.registers.22", "top.rf.registers.23", "top.rf.registers.24", "top.rf.registers.25", "top.rf.registers.26", "top.rf.registers.27", "top.rf.registers.28", "top.rf.registers.29", "top.rf.registers.30", "top.rf.registers.31"], txn = 4 : i64} : (i1, i32, i64) -> () loc(#loc111)
    acir.effect %666(%false, %592) {array = "top.rf.pending", branch = 210 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "write", port = 0 : i64, ports = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], resource = "top.rf.pending.0", resources = ["top.rf.pending.0", "top.rf.pending.1", "top.rf.pending.2", "top.rf.pending.3", "top.rf.pending.4", "top.rf.pending.5", "top.rf.pending.6", "top.rf.pending.7", "top.rf.pending.8", "top.rf.pending.9", "top.rf.pending.10", "top.rf.pending.11", "top.rf.pending.12", "top.rf.pending.13", "top.rf.pending.14", "top.rf.pending.15", "top.rf.pending.16", "top.rf.pending.17", "top.rf.pending.18", "top.rf.pending.19", "top.rf.pending.20", "top.rf.pending.21", "top.rf.pending.22", "top.rf.pending.23", "top.rf.pending.24", "top.rf.pending.25", "top.rf.pending.26", "top.rf.pending.27", "top.rf.pending.28", "top.rf.pending.29", "top.rf.pending.30", "top.rf.pending.31"], txn = 4 : i64} : (i1, i1, i64) -> () loc(#loc112)
    acir.effect %665(%658) {branch = 209 : i64, frames = ["top.writeback:commit"], method = "write", port = 0 : i64, resource = "top.writeback.retired", txn = 4 : i64} : (i1, i32) -> () loc(#loc115)
    acir.effect %665(%659) {branch = 209 : i64, frames = ["top.writeback:commit"], method = "write", port = 0 : i64, resource = "top.writeback.last_pc", txn = 4 : i64} : (i1, i32) -> () loc(#loc116)
  }
}
#loc1 = loc("acir.binding:top.fetch_body:current_pc")
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":72:12)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":73:20)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":66:16)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":66:20)
#loc6 = loc("acir.binding:top.fetch_body.0.call.0.6:word")
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":67:14)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":68:16)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":67:9)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":91:23)
#loc11 = loc("acir.binding:top.decode_stage_body.0.call.1.3:packet")
#loc12 = loc("acir.binding:top.decode_stage_body.0.call.1.3:word")
#loc13 = loc("acir.binding:top.decode_stage_body.0.call.1.3:opcode")
#loc14 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:14)
#loc15 = loc("acir.binding:top.decode_stage_body.0.call.1.3:funct3")
#loc16 = loc("acir.binding:top.decode_stage_body.0.call.1.3:funct7")
#loc17 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:10)
#loc18 = loc("acir.binding:top.decode_stage_body.0.call.1.3:rd")
#loc19 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:11)
#loc20 = loc("acir.binding:top.decode_stage_body.0.call.1.3:rs1")
#loc21 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":59:11)
#loc22 = loc("acir.binding:top.decode_stage_body.0.call.1.3:rs2")
#loc23 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:8)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":69:10)
#loc25 = loc("acir.binding:top.decode_stage_body.0.call.1.3:writes")
#loc26 = loc("acir.binding:top.decode_stage_body.0.call.1.3:kind")
#loc27 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:16)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:33)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:9)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:28)
#loc31 = loc("acir.binding:top.decode_stage_body.0.call.1.3.0.call.1.49:value")
#loc32 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:13)
#loc33 = loc("acir.binding:top.decode_stage_body.0.call.1.3:immediate")
#loc34 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:9)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:16)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:33)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:48)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:9)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:9)
#loc40 = loc("acir.binding:top.decode_stage_body.0.call.1.3:immediate_rhs")
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:20)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:12)
#loc43 = loc("acir.binding:top.decode_stage_body:inst")
#loc44 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:19)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:42)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":16:57)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:28)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:20)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:55)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:47)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:43)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:75)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:102)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:94)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:90)
#loc56 = loc("acir.binding:top.decode_stage_body:enabled")
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":94:20)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":26:12)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":13:61)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:16)
#loc61 = loc("acir.binding:top.decode_stage_body.0.call.4.15:lhs")
#loc62 = loc("acir.binding:top.decode_stage_body.0.call.4.15:store_value")
#loc63 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:36)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:15)
#loc65 = loc("acir.binding:top.decode_stage_body.0.call.4.15:rhs")
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":87:9)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":32:28)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:13)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:25)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:37)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:51)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:16)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":109:16)
#loc74 = loc("acir.binding:top.execute_body.0.call.7.2:inst")
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:41)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:63)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:74)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:18)
#loc79 = loc("acir.binding:top.execute_body.0.call.7.2:result")
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:25)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:34)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:56)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:73)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:91)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":106:16)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":128:16)
#loc87 = loc("acir.binding:top.memory_stage_body.0.call.8.2:inst")
#loc88 = loc("acir.binding:top.memory_stage_body.0.call.8.2:result")
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:12)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:14)
#loc91 = loc("acir.binding:top.memory_stage_body.0.call.8.2.0.call.9.21:value")
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:13)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":54:16)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":54:37)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":55:9)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":55:15)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:16)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:24)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:33)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:44)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:61)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:16)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":54:9)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":49:9)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":145:9)
#loc106 = loc("acir.binding:top.writeback_body.0.call.11.2:inst")
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":140:9)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":37:12)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":37:28)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":38:34)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":38:13)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":39:13)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:19)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":142:19)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:9)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":142:9)
#loc117 = loc(callsite(#loc11 at #loc))
#loc118 = loc(callsite(#loc12 at #loc))
#loc119 = loc(callsite(#loc13 at #loc))
#loc120 = loc(callsite(#loc14 at #loc))
#loc121 = loc(callsite(#loc15 at #loc))
#loc122 = loc(callsite(#loc16 at #loc))
#loc123 = loc(callsite(#loc17 at #loc))
#loc124 = loc(callsite(#loc18 at #loc))
#loc125 = loc(callsite(#loc19 at #loc))
#loc126 = loc(callsite(#loc20 at #loc))
#loc127 = loc(callsite(#loc21 at #loc))
#loc128 = loc(callsite(#loc22 at #loc))
#loc129 = loc(callsite(#loc23 at #loc))
#loc130 = loc(callsite(#loc24 at #loc))
#loc131 = loc(callsite(#loc25 at #loc))
#loc132 = loc(callsite(#loc26 at #loc))
#loc133 = loc(callsite(#loc27 at #loc))
#loc134 = loc(callsite(#loc28 at #loc))
#loc135 = loc(callsite(#loc at #loc))
#loc136 = loc(callsite(#loc29 at #loc))
#loc137 = loc(callsite(#loc30 at #loc))
#loc138 = loc(callsite(#loc33 at #loc))
#loc139 = loc(callsite(#loc34 at #loc))
#loc140 = loc(callsite(#loc35 at #loc))
#loc141 = loc(callsite(#loc36 at #loc))
#loc142 = loc(callsite(#loc37 at #loc))
#loc143 = loc(callsite(#loc38 at #loc))
#loc144 = loc(callsite(#loc39 at #loc))
#loc145 = loc(callsite(#loc40 at #loc))
#loc146 = loc(callsite(#loc41 at #loc))
#loc147 = loc(callsite(#loc42 at #loc))
#loc148 = loc(fused[#loc111, #loc109])
#loc149 = loc(fused[#loc135, #loc133])
#loc150 = loc(callsite(#loc31 at #loc135))
#loc151 = loc(callsite(#loc32 at #loc135))
#loc152 = loc(fused[#loc135, #loc140])
#loc153 = loc(fused[#loc135, #loc130])

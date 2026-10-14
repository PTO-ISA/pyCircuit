module attributes {acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, format_version = 9 : i64, top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs1", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rs2", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "immediate", payload_type = i32, type = "u32"}, {default = false, name = "immediate_rhs", payload_type = i1, type = "bool"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "kind", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "lhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rhs", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "store_value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "word", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "rd", payload_type = i32, type = "u32"}, {default = 0 : i64, name = "value", payload_type = i32, type = "u32"}, {default = false, name = "writes", payload_type = i1, type = "bool"}]}}, acir.stage = "transaction"} {
  %c16_i64 = arith.constant 16 : i64
  %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
  %c32_i64 = arith.constant 32 : i64
  %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32
  %c32_i32 = arith.constant {acir.source_type = "u32"} 32 : i32
  %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
  %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
  %c35_i32 = arith.constant {acir.source_type = "u32"} 35 : i32
  %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
  %c51_i32 = arith.constant {acir.source_type = "u32"} 51 : i32
  %c19_i32 = arith.constant {acir.source_type = "u32"} 19 : i32
  %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
  %c20_i32 = arith.constant {acir.source_type = "u32"} 20 : i32
  %c15_i32 = arith.constant {acir.source_type = "u32"} 15 : i32
  %c31_i32 = arith.constant {acir.source_type = "u32"} 31 : i32
  %c25_i32 = arith.constant {acir.source_type = "u32"} 25 : i32
  %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
  %c12_i32 = arith.constant {acir.source_type = "u32"} 12 : i32
  %c127_i32 = arith.constant {acir.source_type = "u32"} 127 : i32
  %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32
  %c0_i64 = arith.constant 0 : i64
  %c6_i64 = arith.constant 6 : i64
  %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
  %c24_i32 = arith.constant {acir.source_type = "u32"} 24 : i32
  %false = arith.constant false
  %true = arith.constant true
  acir.circuit attributes {conflicts = [], edges = [{from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.ex_mem", source_branch = 13 : i64, target_branches = [12], to = 2 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.ex_mem", source_branch = 13 : i64, target_branches = [12], to = 2 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.id_ex", source_branch = 11 : i64, target_branches = [10], to = 1 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.id_ex", source_branch = 11 : i64, target_branches = [10], to = 1 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.if_id", source_branch = 6 : i64, target_branches = [2], to = 0 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.if_id", source_branch = 6 : i64, target_branches = [2], to = 0 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mem_wb", source_branch = 17 : i64, target_branches = [16], to = 3 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mem_wb", source_branch = 17 : i64, target_branches = [16], to = 3 : i64}], schedule = [4, 3, 2, 1, 0], stage = "transaction"} {
    acir.state @top.ex_mem : !acir.struct<"src_isa_f0424bc8.Executed"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.fetch.pc : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.fetch.rom : !acir.array<i32, [6]> {initial = [7340179 : i32, 5243155 : i32, 2130355 : i32, 3153955 : i32, 8707 : i32, 1075970739 : i32], kind = "cell"}
    acir.state @top.id_ex : !acir.struct<"src_isa_f0424bc8.Operands"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.if_id : !acir.struct<"src_isa_f0424bc8.Fetched"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.mem_wb : !acir.struct<"src_isa_f0424bc8.Retired"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.memory.words : !acir.array<i32, [16]> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rf.pending : !acir.array<i1, [32]> {initial = [false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false], kind = "cell"}
    acir.state @top.rf.registers : !acir.array<i32, [32]> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.writeback.last_pc : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.writeback.retired : i32 {initial = 0 : i32, kind = "cell"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc1)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.fetch.pc", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc1)
    %0 = arith.cmpi ult, %value, %c24_i32 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc2)
    %1 = arith.cmpi uge, %value, %c24_i32 : i32 loc(#loc2)
    %2 = arith.ori %1, %ready : i1
    %3 = arith.andi %2, %ready : i1 loc(#loc3)
    %4 = arith.andi %0, %3 : i1 loc(#loc3)
    acir.path %0 {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc3)
    acir.path %0 {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc3)
    %5 = arith.ori %1, %2 : i1 loc(#loc3)
    %value_0, %ready_1 = acir.read(%0) {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "read", port = 0 : i64, resource = "top.fetch.rom", txn = 0 : i64} : (i1) -> (!acir.array<i32, [6]>, i1) loc(#loc4)
    %6 = arith.andi %5, %ready_1 : i1 loc(#loc4)
    %7 = arith.ori %1, %6 : i1 loc(#loc4)
    %8 = arith.andi %4, %7 : i1 loc(#loc4)
    %9 = arith.shrui %value, %c2_i32 : i32 loc(#loc5)
    %10 = arith.andi %ready_1, %4 : i1 loc(#loc4)
    %11 = arith.extui %9 : i32 to i64 loc(#loc4)
    %12 = arith.cmpi ult, %11, %c6_i64 : i64 loc(#loc4)
    %13 = arith.select %12, %11, %c0_i64 : i64 loc(#loc4)
    %14 = arith.andi %10, %12 : i1 loc(#loc4)
    %15 = "acir.array.index"(%value_0, %13) : (!acir.array<i32, [6]>, i64) -> i32 loc(#loc4)
    %16 = arith.addi %value, %c4_i32 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc6)
    %17 = arith.andi %5, %4 : i1 loc(#loc7)
    %18 = arith.ori %1, %17 : i1 loc(#loc7)
    %19 = arith.andi %8, %18 : i1 loc(#loc7)
    %20 = arith.andi %4, %14 : i1 loc(#loc8)
    %21 = acir.aggregate(%value, %15) {acir.id = "v16", acir.source_type = "src_isa_f0424bc8.Fetched"} : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc8)
    %22 = arith.andi %5, %20 : i1
    %23 = arith.andi %0, %22 : i1
    %24 = arith.xori %23, %true : i1
    %25 = arith.ori %24, %2 : i1
    %26 = arith.andi %19, %25 : i1
    acir.path %23 {branch = 2 : i64, kind = "branch", txn = 0 : i64}
    %value_2, %ready_3 = acir.read(%23) {branch = 2 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.if_id", txn = 0 : i64} : (i1) -> (i1, i1)
    %27 = arith.andi %2, %ready_3 : i1
    %28 = arith.ori %24, %27 : i1
    %29 = arith.andi %26, %28 : i1
    %30 = arith.xori %value_2, %true : i1
    %31 = arith.andi %2, %30 : i1
    %32 = arith.ori %24, %31 : i1
    %33 = arith.andi %29, %32 : i1
    %34 = acir.fire "top.fetch.fetch#0" %33 {successors = [], txn = 0 : i64}
    %35 = arith.andi %34, %0 : i1
    acir.effect %35(%16) {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "write", port = 0 : i64, resource = "top.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc7)
    %36 = arith.andi %34, %23 : i1
    acir.effect %36(%21) {branch = 2 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.if_id", txn = 0 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Fetched">) -> ()
    %value_4, %ready_5 = acir.read(%true) {branch = 0 : i64, frames = [], method = "peek", port = 0 : i64, resource = "top.if_id", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Fetched">, i1) loc(#loc9)
    %37 = acir.extract %value_4 {acir.id = "v5", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc10)
    %38 = arith.andi %37, %c127_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc11)
    %39 = arith.shrui %37, %c12_i32 : i32 loc(#loc12)
    %40 = arith.andi %39, %c7_i32 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc12)
    %41 = arith.shrui %37, %c25_i32 : i32 loc(#loc13)
    %42 = arith.shrui %37, %c7_i32 : i32 loc(#loc14)
    %43 = arith.andi %42, %c31_i32 {acir.id = "v17", acir.source_type = "u32"} : i32 loc(#loc14)
    %44 = arith.shrui %37, %c15_i32 : i32 loc(#loc15)
    %45 = arith.andi %44, %c31_i32 {acir.id = "v21", acir.source_type = "u32"} : i32 loc(#loc15)
    %46 = arith.shrui %37, %c20_i32 : i32 loc(#loc16)
    %47 = arith.andi %46, %c31_i32 {acir.id = "v25", acir.source_type = "u32"} : i32 loc(#loc16)
    %48 = arith.cmpi eq, %38, %c19_i32 {acir.id = "v31", acir.source_type = "bool"} : i32 loc(#loc17)
    %49 = arith.cmpi ne, %38, %c19_i32 : i32 loc(#loc17)
    %50 = arith.cmpi eq, %38, %c19_i32 : i32 loc(#loc17)
    %51 = arith.ori %50, %ready_5 : i1
    %52 = arith.cmpi eq, %38, %c51_i32 {acir.id = "v41", acir.source_type = "bool"} : i32 loc(#loc18)
    %53 = arith.andi %49, %52 : i1
    %54 = arith.andi %51, %ready_5 : i1
    %55 = arith.cmpi ne, %38, %c51_i32 : i32 loc(#loc18)
    %56 = arith.andi %49, %55 : i1
    %57 = arith.xori %56, %true : i1
    %58 = arith.ori %57, %54 : i1
    %59 = arith.cmpi eq, %38, %c3_i32 {acir.id = "v65", acir.source_type = "bool"} : i32 loc(#loc19)
    %60 = arith.andi %56, %59 : i1
    %61 = arith.andi %58, %ready_5 : i1
    %62 = arith.cmpi ne, %38, %c3_i32 : i32 loc(#loc19)
    %63 = arith.andi %56, %62 : i1
    %64 = arith.xori %63, %true : i1 loc(#loc20)
    %65 = arith.ori %64, %61 : i1
    %66 = arith.cmpi eq, %38, %c35_i32 {acir.id = "v76", acir.source_type = "bool"} : i32 loc(#loc21)
    %67 = arith.andi %63, %66 : i1
    %68 = arith.andi %65, %ready_5 : i1
    %69 = arith.cmpi ne, %38, %c35_i32 : i32 loc(#loc21)
    %70 = arith.andi %63, %69 : i1
    %71 = arith.xori %70, %true : i1
    %72 = arith.ori %71, %68 : i1
    %73 = arith.xori %67, %true : i1
    %74 = arith.ori %73, %68 : i1
    %75 = arith.cmpi eq, %40, %c2_i32 {acir.id = "v78", acir.source_type = "bool"} : i32 loc(#loc22)
    %76 = arith.andi %70, %72 : i1
    %77 = arith.andi %74, %ready_5 : i1
    %78 = arith.andi %67, %77 : i1
    %79 = arith.ori %76, %78 : i1
    %80 = arith.select %70, %false, %75 : i1
    %81 = arith.ori %64, %65 : i1
    %82 = acir.assert %80 loc(#loc20)
    %83 = arith.andi %79, %82 : i1 loc(#loc20)
    %84 = arith.ori %64, %83 : i1 loc(#loc20)
    %85 = arith.shli %41, %c5_i32 : i32 loc(#loc23)
    %86 = arith.andi %42, %c31_i32 {acir.id = "v93", acir.source_type = "u32"} : i32 loc(#loc24)
    %87 = arith.ori %85, %86 {acir.id = "v94", acir.source_type = "u32"} : i32 loc(#loc23)
    %88 = arith.ori %64, %81 : i1 loc(#loc25)
    %89 = arith.andi %81, %ready_5 : i1 loc(#loc25)
    %90 = arith.andi %63, %89 : i1 loc(#loc25)
    %91 = arith.xori %87, %c2048_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc26)
    %92 = arith.subi %91, %c2048_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc27)
    %93 = acir.unary %false "not" {acir.source_type = "bool"} : i1 -> i1
    %94 = acir.assert %93
    %95 = arith.ori %64, %94 : i1
    %96 = arith.andi %88, %90 : i1
    %97 = arith.andi %63, %96 : i1
    %98 = arith.andi %97, %95 : i1
    %99 = arith.xori %60, %true : i1 loc(#loc28)
    %100 = arith.ori %99, %61 : i1
    %101 = arith.cmpi eq, %40, %c2_i32 {acir.id = "v67", acir.source_type = "bool"} : i32 loc(#loc29)
    %102 = acir.assert %101 loc(#loc28)
    %103 = arith.andi %ready_5, %102 : i1 loc(#loc28)
    %104 = arith.ori %99, %103 : i1 loc(#loc28)
    %105 = arith.andi %84, %104 : i1 loc(#loc28)
    %106 = arith.ori %99, %100 : i1 loc(#loc30)
    %107 = arith.andi %100, %ready_5 : i1 loc(#loc30)
    %108 = arith.andi %60, %107 : i1 loc(#loc30)
    %109 = arith.xori %46, %c2048_i32 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc26)
    %110 = arith.subi %109, %c2048_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc27)
    %111 = acir.assert %93
    %112 = arith.ori %99, %111 : i1
    %113 = arith.andi %106, %108 : i1
    %114 = arith.andi %60, %113 : i1
    %115 = arith.andi %114, %112 : i1
    %116 = arith.andi %81, %98 : i1
    %117 = arith.andi %63, %116 : i1
    %118 = arith.andi %100, %115 : i1
    %119 = arith.andi %60, %118 : i1
    %120 = arith.ori %117, %119 : i1
    %121 = arith.select %63, %92, %110 : i32
    %122 = arith.andi %63, %81 : i1
    %123 = arith.andi %60, %100 : i1
    %124 = arith.ori %122, %123 : i1
    %125 = arith.select %63, %c3_i32, %c2_i32 : i32
    %126 = arith.ori %122, %108 : i1
    %127 = arith.select %63, %c0_i32, %43 : i32
    %128 = arith.ori %90, %123 : i1
    %129 = arith.select %63, %47, %c0_i32 : i32
    %130 = arith.ori %57, %58 : i1
    %131 = arith.xori %53, %true : i1 loc(#loc31)
    %132 = arith.ori %131, %54 : i1
    %133 = arith.cmpi eq, %40, %c0_i32 {acir.id = "v43", acir.source_type = "bool"} : i32 loc(#loc32)
    %134 = arith.andi %53, %133 : i1
    %135 = arith.andi %132, %ready_5 : i1
    %136 = arith.cmpi ne, %40, %c0_i32 : i32 loc(#loc32)
    %137 = arith.andi %53, %136 : i1
    %138 = arith.xori %137, %true : i1
    %139 = arith.ori %138, %135 : i1
    %140 = arith.xori %134, %true : i1
    %141 = arith.ori %140, %135 : i1
    %142 = arith.cmpi eq, %41, %c0_i32 {acir.id = "v45", acir.source_type = "bool"} : i32 loc(#loc33)
    %143 = arith.andi %134, %142 : i1
    %144 = arith.andi %141, %ready_5 : i1
    %145 = arith.cmpi ne, %41, %c0_i32 : i32 loc(#loc33)
    %146 = arith.andi %134, %145 : i1
    %147 = arith.xori %146, %true : i1
    %148 = arith.ori %147, %144 : i1
    %149 = arith.cmpi eq, %41, %c32_i32 {acir.id = "v48", acir.source_type = "bool"} : i32 loc(#loc34)
    %150 = arith.xori %143, %true : i1
    %151 = arith.ori %150, %144 : i1
    %152 = arith.andi %148, %ready_5 : i1
    %153 = arith.andi %146, %152 : i1
    %154 = arith.andi %143, %151 : i1
    %155 = arith.ori %153, %154 : i1
    %156 = arith.select %146, %149, %true : i1
    %157 = arith.ori %140, %141 : i1
    %158 = arith.andi %137, %139 : i1
    %159 = arith.andi %157, %155 : i1
    %160 = arith.andi %134, %159 : i1
    %161 = arith.ori %158, %160 : i1
    %162 = arith.select %137, %false, %156 : i1
    %163 = arith.ori %131, %132 : i1
    %164 = acir.assert %162 loc(#loc31)
    %165 = arith.andi %161, %164 : i1 loc(#loc31)
    %166 = arith.ori %131, %165 : i1 loc(#loc31)
    %167 = arith.andi %105, %166 : i1 loc(#loc31)
    %168 = arith.cmpi eq, %41, %c32_i32 {acir.id = "v58", acir.source_type = "bool"} : i32 loc(#loc35)
    %169 = arith.andi %53, %168 : i1
    %170 = arith.andi %163, %ready_5 : i1
    %171 = arith.cmpi ne, %41, %c32_i32 : i32 loc(#loc35)
    %172 = arith.andi %53, %171 : i1
    %173 = arith.xori %172, %true : i1
    %174 = arith.ori %173, %170 : i1
    %175 = arith.xori %169, %true : i1
    %176 = arith.ori %175, %170 : i1
    %177 = arith.andi %172, %174 : i1
    %178 = arith.andi %169, %176 : i1
    %179 = arith.ori %177, %178 : i1
    %180 = arith.extui %173 : i1 to i32
    %181 = arith.ori %131, %163 : i1
    %182 = arith.andi %130, %120 : i1
    %183 = arith.andi %56, %182 : i1
    %184 = arith.andi %53, %181 : i1
    %185 = arith.ori %183, %184 : i1
    %186 = arith.select %56, %121, %c0_i32 : i32
    %187 = arith.andi %130, %124 : i1
    %188 = arith.andi %56, %187 : i1
    %189 = arith.ori %188, %184 : i1
    %190 = arith.andi %181, %179 : i1
    %191 = arith.andi %53, %190 : i1
    %192 = arith.ori %188, %191 : i1
    %193 = arith.select %56, %125, %180 : i32
    %194 = arith.andi %130, %126 : i1
    %195 = arith.andi %56, %194 : i1
    %196 = arith.andi %181, %ready_5 : i1
    %197 = arith.andi %53, %196 : i1
    %198 = arith.ori %195, %197 : i1
    %199 = arith.select %56, %127, %43 : i32
    %200 = arith.andi %130, %128 : i1
    %201 = arith.andi %56, %200 : i1
    %202 = arith.ori %201, %197 : i1
    %203 = arith.select %56, %129, %47 : i32
    %204 = arith.select %56, %64, %true : i1
    %205 = arith.ori %50, %51 : i1
    %206 = arith.ori %49, %ready_5 : i1
    %207 = arith.cmpi eq, %40, %c0_i32 {acir.id = "v33", acir.source_type = "bool"} : i32 loc(#loc36)
    %208 = acir.assert %207 loc(#loc37)
    %209 = arith.andi %ready_5, %208 : i1 loc(#loc37)
    %210 = arith.ori %49, %209 : i1 loc(#loc37)
    %211 = arith.andi %167, %210 : i1 loc(#loc37)
    %212 = arith.ori %49, %206 : i1 loc(#loc38)
    %213 = arith.andi %206, %ready_5 : i1 loc(#loc38)
    %214 = arith.andi %48, %213 : i1 loc(#loc38)
    %215 = acir.assert %93
    %216 = arith.ori %49, %215 : i1
    %217 = arith.andi %212, %214 : i1
    %218 = arith.andi %48, %217 : i1
    %219 = arith.andi %218, %216 : i1
    %220 = arith.andi %205, %185 : i1
    %221 = arith.andi %49, %220 : i1
    %222 = arith.andi %206, %219 : i1
    %223 = arith.andi %48, %222 : i1
    %224 = arith.ori %221, %223 : i1
    %225 = arith.select %49, %186, %110 : i32
    %226 = arith.andi %205, %189 : i1
    %227 = arith.andi %49, %226 : i1
    %228 = arith.andi %48, %206 : i1
    %229 = arith.ori %227, %228 : i1
    %230 = arith.select %49, %56, %true : i1
    %231 = arith.andi %205, %192 : i1
    %232 = arith.andi %49, %231 : i1
    %233 = arith.ori %232, %228 : i1
    %234 = arith.select %49, %193, %c0_i32 : i32
    %235 = arith.andi %205, %198 : i1
    %236 = arith.andi %49, %235 : i1
    %237 = arith.ori %236, %214 : i1
    %238 = arith.select %49, %199, %43 : i32
    %239 = arith.andi %205, %202 : i1
    %240 = arith.andi %49, %239 : i1
    %241 = arith.ori %240, %228 : i1
    %242 = arith.select %49, %203, %c0_i32 : i32
    %243 = arith.select %48, %true, %204 : i1
    %244 = acir.extract %value_4 {acir.id = "v125", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc39)
    %245 = arith.andi %ready_5, %233 : i1 loc(#loc40)
    %246 = arith.andi %245, %237 : i1 loc(#loc40)
    %247 = arith.andi %246, %ready_5 : i1 loc(#loc40)
    %248 = arith.andi %247, %241 : i1 loc(#loc40)
    %249 = arith.andi %248, %224 : i1 loc(#loc40)
    %250 = arith.andi %249, %229 : i1 loc(#loc40)
    %251 = arith.andi %250, %229 : i1 loc(#loc40)
    %252 = acir.aggregate(%244, %37, %234, %238, %45, %242, %225, %230, %243) {acir.id = "v126", acir.source_type = "src_isa_f0424bc8.Decoded"} : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc40)
    %253 = acir.assert %93
    %254 = arith.andi %211, %253 : i1
    %255 = arith.andi %251, %254 : i1
    %value_6, %ready_7 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.mem_wb", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc41)
    %256 = arith.xori %value_6, %true : i1 loc(#loc42)
    %257 = arith.ori %value_6, %ready_7 : i1
    %258 = arith.ori %256, %ready_7 : i1
    acir.path %value_6 {branch = 3 : i64, kind = "branch", txn = -1 : i64} loc(#loc42)
    acir.path %value_6 {branch = 3 : i64, frames = ["top.rf:ready"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc42)
    %259 = arith.ori %256, %258 : i1 loc(#loc42)
    %260 = arith.andi %258, %255 : i1 loc(#loc42)
    %261 = arith.andi %value_6, %260 : i1 loc(#loc42)
    %value_8, %ready_9 = acir.read(%value_6) {branch = 3 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = -1 : i64} : (i1) -> (!acir.array<i1, [32]>, i1) loc(#loc43)
    %262 = acir.extract %252 {acir.id = "v9", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc44)
    %263 = arith.andi %ready_9, %261 : i1 loc(#loc43)
    %264 = arith.extui %262 : i32 to i64 loc(#loc43)
    %265 = arith.cmpi ult, %264, %c32_i64 : i64 loc(#loc43)
    %266 = arith.select %265, %264, %c0_i64 : i64 loc(#loc43)
    %267 = arith.andi %263, %265 : i1 loc(#loc43)
    %268 = "acir.array.index"(%value_8, %266) : (!acir.array<i1, [32]>, i64) -> i1 loc(#loc43)
    %269 = acir.unary %268 "not" {acir.id = "v11", acir.source_type = "bool"} : i1 -> i1 loc(#loc45)
    %270 = arith.andi %value_6, %269 : i1
    %271 = arith.andi %259, %267 : i1
    %272 = arith.xori %269, %true : i1
    %273 = arith.andi %value_6, %272 : i1
    %274 = arith.xori %273, %true : i1
    %275 = arith.ori %274, %271 : i1
    %276 = arith.xori %270, %true : i1
    %277 = arith.ori %276, %271 : i1
    acir.path %270 {branch = 4 : i64, kind = "branch", txn = -1 : i64} loc(#loc46)
    %value_10, %ready_11 = acir.read(%270) {branch = 4 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = -1 : i64} : (i1) -> (!acir.array<i1, [32]>, i1) loc(#loc46)
    %278 = acir.extract %252 {acir.id = "v14", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc47)
    %279 = arith.andi %ready_11, %261 : i1 loc(#loc46)
    %280 = arith.extui %278 : i32 to i64 loc(#loc46)
    %281 = arith.cmpi ult, %280, %c32_i64 : i64 loc(#loc46)
    %282 = arith.select %281, %280, %c0_i64 : i64 loc(#loc46)
    %283 = arith.andi %279, %281 : i1 loc(#loc46)
    %284 = "acir.array.index"(%value_10, %282) : (!acir.array<i1, [32]>, i64) -> i1 loc(#loc46)
    %285 = acir.unary %284 "not" {acir.id = "v16", acir.source_type = "bool"} : i1 -> i1 loc(#loc48)
    %286 = arith.andi %273, %275 : i1
    %287 = arith.andi %277, %283 : i1
    %288 = arith.andi %270, %287 : i1
    %289 = arith.ori %286, %288 : i1
    %290 = arith.select %273, %false, %285 : i1
    %291 = arith.ori %256, %259 : i1
    %292 = arith.andi %value_6, %290 : i1
    %293 = arith.andi %291, %289 : i1
    %294 = arith.xori %290, %true : i1
    %295 = arith.andi %value_6, %294 : i1
    %296 = arith.xori %295, %true : i1
    %297 = arith.ori %296, %293 : i1
    %298 = arith.xori %292, %true : i1
    %299 = arith.ori %298, %293 : i1
    %300 = acir.extract %252 {acir.id = "v21", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc49)
    %301 = acir.unary %300 "not" {acir.id = "v22", acir.source_type = "bool"} : i1 -> i1 loc(#loc50)
    %302 = arith.andi %292, %301 : i1
    %303 = arith.andi %299, %261 : i1
    %304 = arith.xori %301, %true : i1
    %305 = arith.andi %292, %304 : i1
    %306 = arith.xori %305, %true : i1
    %307 = arith.ori %306, %303 : i1
    acir.path %305 {branch = 5 : i64, kind = "branch", txn = -1 : i64} loc(#loc51)
    %value_12, %ready_13 = acir.read(%305) {branch = 5 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = -1 : i64} : (i1) -> (!acir.array<i1, [32]>, i1) loc(#loc51)
    %308 = acir.extract %252 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc52)
    %309 = arith.andi %ready_13, %261 : i1 loc(#loc51)
    %310 = arith.extui %308 : i32 to i64 loc(#loc51)
    %311 = arith.cmpi ult, %310, %c32_i64 : i64 loc(#loc51)
    %312 = arith.select %311, %310, %c0_i64 : i64 loc(#loc51)
    %313 = arith.andi %309, %311 : i1 loc(#loc51)
    %314 = "acir.array.index"(%value_12, %312) : (!acir.array<i1, [32]>, i64) -> i1 loc(#loc51)
    %315 = acir.unary %314 "not" {acir.id = "v28", acir.source_type = "bool"} : i1 -> i1 loc(#loc53)
    %316 = arith.xori %302, %true : i1
    %317 = arith.ori %316, %303 : i1
    %318 = arith.andi %307, %313 : i1
    %319 = arith.andi %305, %318 : i1
    %320 = arith.andi %302, %317 : i1
    %321 = arith.ori %319, %320 : i1
    %322 = arith.select %305, %315, %true : i1
    %323 = arith.ori %298, %299 : i1
    %324 = arith.andi %295, %297 : i1
    %325 = arith.andi %323, %321 : i1
    %326 = arith.andi %292, %325 : i1
    %327 = arith.ori %324, %326 : i1
    %328 = arith.select %295, %false, %322 : i1
    %329 = arith.ori %256, %291 : i1
    %330 = acir.assert %93
    %331 = arith.ori %256, %330 : i1
    %332 = arith.andi %329, %327 : i1
    %333 = arith.andi %value_6, %332 : i1
    %334 = arith.andi %333, %331 : i1
    %335 = arith.andi %256, %257 : i1
    %336 = arith.andi %258, %334 : i1
    %337 = arith.andi %value_6, %336 : i1
    %338 = arith.ori %335, %337 : i1
    %339 = arith.select %value_6, %328, %false : i1
    %340 = arith.xori %339, %true : i1 loc(#loc54)
    %341 = arith.ori %340, %338 : i1
    %342 = arith.andi %341, %255 : i1 loc(#loc54)
    %343 = arith.andi %339, %342 : i1 loc(#loc54)
    acir.path %339 {branch = 6 : i64, kind = "branch", txn = 1 : i64} loc(#loc54)
    acir.path %339 {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc54)
    %344 = arith.ori %340, %341 : i1 loc(#loc54)
    %value_14, %ready_15 = acir.read(%339) {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "peek", port = 0 : i64, resource = "top.if_id", txn = 1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Fetched">, i1) loc(#loc55)
    %345 = arith.andi %344, %ready_15 : i1 loc(#loc55)
    %346 = arith.ori %340, %345 : i1 loc(#loc55)
    %347 = arith.andi %343, %346 : i1 loc(#loc55)
    %348 = arith.andi %347, %346 : i1 loc(#loc55)
    %value_16, %ready_17 = acir.read(%339) {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "empty", port = 0 : i64, resource = "top.if_id", txn = 1 : i64} : (i1) -> (i1, i1) loc(#loc55)
    %349 = arith.andi %344, %ready_17 : i1 loc(#loc55)
    %350 = arith.ori %340, %349 : i1 loc(#loc55)
    %351 = arith.andi %348, %350 : i1 loc(#loc55)
    %352 = arith.xori %value_16, %true : i1 loc(#loc55)
    %353 = arith.andi %344, %352 : i1 loc(#loc55)
    %354 = arith.ori %340, %353 : i1 loc(#loc55)
    %355 = arith.andi %351, %354 : i1 loc(#loc55)
    acir.path %339 {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc56)
    %356 = arith.ori %340, %344 : i1 loc(#loc56)
    %357 = arith.andi %344, %343 : i1 loc(#loc56)
    %358 = arith.andi %339, %357 : i1 loc(#loc56)
    %359 = acir.aggregate() {acir.source_type = "u32"} : () -> i32
    %360 = arith.cmpi eq, %262, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc57)
    %361 = arith.andi %356, %358 : i1
    %362 = arith.ori %340, %361 : i1
    %363 = arith.andi %355, %362 : i1
    %364 = arith.andi %339, %360 : i1
    %365 = arith.cmpi ne, %262, %c0_i32 : i32 loc(#loc57)
    %366 = arith.andi %339, %365 : i1
    %367 = arith.xori %366, %true : i1 loc(#loc58)
    %368 = arith.ori %367, %361 : i1
    %369 = arith.xori %364, %true : i1
    %370 = arith.ori %369, %361 : i1
    %371 = arith.andi %366, %368 : i1
    %372 = arith.andi %364, %370 : i1
    %373 = arith.ori %371, %372 : i1
    %374 = arith.select %366, %359, %c0_i32 : i32
    %375 = arith.ori %340, %356 : i1 loc(#loc59)
    %376 = arith.andi %375, %373 : i1
    %377 = arith.ori %340, %376 : i1
    %378 = arith.andi %363, %377 : i1
    %379 = arith.andi %339, %367 : i1
    %380 = arith.xori %379, %true : i1
    %381 = arith.ori %380, %376 : i1
    %382 = arith.ori %367, %376 : i1
    acir.path %366 {branch = 7 : i64, kind = "branch", txn = 1 : i64} loc(#loc58)
    %value_18, %ready_19 = acir.read(%366) {branch = 7 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 1 : i64} : (i1) -> (!acir.array<i32, [32]>, i1) loc(#loc58)
    %383 = arith.andi %382, %ready_19 : i1 loc(#loc58)
    %384 = arith.ori %367, %383 : i1 loc(#loc58)
    %385 = arith.andi %378, %384 : i1 loc(#loc58)
    %386 = arith.andi %ready_19, %358 : i1 loc(#loc58)
    %387 = arith.andi %386, %265 : i1 loc(#loc58)
    %388 = "acir.array.index"(%value_18, %266) : (!acir.array<i32, [32]>, i64) -> i32 loc(#loc58)
    %389 = arith.andi %381, %373 : i1
    %390 = arith.andi %379, %389 : i1
    %391 = arith.andi %366, %382 : i1
    %392 = arith.ori %390, %391 : i1
    %393 = arith.select %379, %366, %false : i1
    %394 = arith.andi %382, %387 : i1
    %395 = arith.andi %366, %394 : i1
    %396 = arith.ori %390, %395 : i1
    %397 = arith.select %379, %374, %388 : i32
    %398 = arith.ori %340, %375 : i1
    %399 = acir.unary %393 "not" {acir.source_type = "bool"} : i1 -> i1
    %400 = arith.andi %398, %392 : i1
    %401 = arith.ori %340, %400 : i1
    %402 = arith.andi %385, %401 : i1
    %403 = arith.andi %398, %396 : i1
    %404 = arith.andi %339, %403 : i1
    %405 = acir.extract %252 {acir.id = "v11", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc60)
    acir.path %339 {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc61)
    %406 = arith.cmpi eq, %405, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc57)
    %407 = arith.andi %402, %362 : i1
    %408 = arith.andi %339, %406 : i1
    %409 = arith.cmpi ne, %405, %c0_i32 : i32 loc(#loc57)
    %410 = arith.andi %339, %409 : i1
    %411 = arith.xori %410, %true : i1 loc(#loc58)
    %412 = arith.ori %411, %361 : i1
    %413 = arith.xori %408, %true : i1
    %414 = arith.ori %413, %361 : i1
    %415 = arith.andi %410, %412 : i1
    %416 = arith.andi %408, %414 : i1
    %417 = arith.ori %415, %416 : i1
    %418 = arith.select %410, %359, %c0_i32 : i32
    %419 = arith.andi %375, %417 : i1
    %420 = arith.ori %340, %419 : i1
    %421 = arith.andi %407, %420 : i1
    %422 = arith.andi %339, %411 : i1
    %423 = arith.xori %422, %true : i1
    %424 = arith.ori %423, %419 : i1
    %425 = arith.ori %411, %419 : i1
    acir.path %410 {branch = 8 : i64, kind = "branch", txn = 1 : i64} loc(#loc58)
    %value_20, %ready_21 = acir.read(%410) {branch = 8 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 1 : i64} : (i1) -> (!acir.array<i32, [32]>, i1) loc(#loc58)
    %426 = arith.andi %425, %ready_21 : i1 loc(#loc58)
    %427 = arith.ori %411, %426 : i1 loc(#loc58)
    %428 = arith.andi %421, %427 : i1 loc(#loc58)
    %429 = arith.andi %ready_21, %358 : i1 loc(#loc58)
    %430 = arith.extui %405 : i32 to i64 loc(#loc58)
    %431 = arith.cmpi ult, %430, %c32_i64 : i64 loc(#loc58)
    %432 = arith.select %431, %430, %c0_i64 : i64 loc(#loc58)
    %433 = arith.andi %429, %431 : i1 loc(#loc58)
    %434 = "acir.array.index"(%value_20, %432) : (!acir.array<i32, [32]>, i64) -> i32 loc(#loc58)
    %435 = arith.andi %424, %417 : i1
    %436 = arith.andi %422, %435 : i1
    %437 = arith.andi %410, %425 : i1
    %438 = arith.ori %436, %437 : i1
    %439 = arith.select %422, %410, %false : i1
    %440 = arith.andi %425, %433 : i1
    %441 = arith.andi %410, %440 : i1
    %442 = arith.ori %436, %441 : i1
    %443 = arith.select %422, %418, %434 : i32
    %444 = acir.unary %439 "not" {acir.source_type = "bool"} : i1 -> i1
    %445 = arith.andi %398, %438 : i1
    %446 = arith.ori %340, %445 : i1
    %447 = arith.andi %428, %446 : i1
    %448 = arith.andi %398, %442 : i1
    %449 = arith.andi %339, %448 : i1
    %450 = acir.extract %252 {acir.id = "v13", acir.source_type = "bool", field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc62)
    %451 = arith.ori %340, %357 : i1
    %452 = arith.andi %447, %451 : i1
    %453 = arith.andi %339, %450 : i1
    %454 = arith.xori %450, %true : i1
    %455 = arith.andi %339, %454 : i1
    %456 = arith.xori %455, %true : i1
    %457 = arith.ori %456, %357 : i1
    %458 = arith.xori %453, %true : i1
    %459 = arith.ori %458, %357 : i1
    %460 = acir.extract %252 {acir.id = "v14", acir.source_type = "u32", field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc63)
    %461 = arith.andi %457, %449 : i1
    %462 = arith.andi %455, %461 : i1
    %463 = arith.andi %459, %343 : i1
    %464 = arith.andi %453, %463 : i1
    %465 = arith.ori %462, %464 : i1
    %466 = arith.select %455, %443, %460 : i32
    acir.path %339 {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc59)
    %467 = arith.andi %356, %343 : i1 loc(#loc59)
    %468 = arith.andi %339, %467 : i1 loc(#loc59)
    %469 = acir.extract %252 {acir.id = "v5", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc64)
    %470 = arith.andi %375, %468 : i1
    %471 = arith.ori %340, %470 : i1
    %472 = arith.andi %452, %471 : i1
    %473 = arith.andi %339, %469 : i1
    %474 = arith.xori %469, %true : i1
    %475 = arith.andi %339, %474 : i1
    %476 = arith.xori %475, %true : i1
    %477 = arith.ori %476, %470 : i1
    %478 = arith.xori %473, %true : i1
    %479 = arith.ori %478, %470 : i1
    %480 = acir.extract %252 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc65)
    %481 = arith.cmpi ne, %480, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc65)
    %482 = arith.andi %475, %477 : i1
    %483 = arith.andi %479, %468 : i1
    %484 = arith.andi %473, %483 : i1
    %485 = arith.ori %482, %484 : i1
    %486 = arith.select %475, %false, %481 : i1
    %487 = arith.andi %398, %485 : i1
    %488 = arith.ori %340, %487 : i1
    %489 = arith.andi %472, %488 : i1
    %490 = arith.andi %339, %486 : i1
    %491 = arith.xori %490, %true : i1 loc(#loc66)
    %492 = arith.ori %491, %487 : i1
    acir.path %490 {branch = 9 : i64, kind = "branch", txn = 1 : i64} loc(#loc66)
    %value_22, %ready_23 = acir.read(%490) {branch = 9 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = 1 : i64} : (i1) -> (!acir.array<i1, [32]>, i1) loc(#loc66)
    %493 = arith.andi %492, %ready_23 : i1 loc(#loc66)
    %494 = arith.ori %491, %493 : i1 loc(#loc66)
    %495 = arith.andi %489, %494 : i1 loc(#loc66)
    %496 = acir.extract %252 {acir.id = "v18", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc67)
    %497 = arith.andi %ready_23, %468 : i1 loc(#loc66)
    %498 = arith.extui %496 : i32 to i64 loc(#loc66)
    %499 = arith.cmpi ult, %498, %c32_i64 : i64 loc(#loc66)
    %500 = arith.andi %497, %499 : i1 loc(#loc66)
    %501 = arith.select %499, %498, %c0_i64 : i64 loc(#loc66)
    %502 = acir.update %value_22, %true[%501] {acir.id = "v19", acir.source_type = "array<bool;32>", path = [unit], signed_indices = [false]} : (!acir.array<i1, [32]>, i1, i64) -> !acir.array<i1, [32]> loc(#loc66)
    %503 = arith.andi %492, %500 : i1 loc(#loc66)
    %504 = arith.ori %491, %503 : i1 loc(#loc66)
    %505 = arith.andi %495, %504 : i1 loc(#loc66)
    %506 = acir.extract %252 {acir.id = "v19", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc68)
    %507 = acir.extract %252 {acir.id = "v20", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc69)
    %508 = acir.extract %252 {acir.id = "v21", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc70)
    %509 = acir.extract %252 {acir.id = "v22", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc71)
    %510 = acir.extract %252 {acir.id = "v23", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc72)
    %511 = arith.andi %343, %404 : i1 loc(#loc73)
    %512 = arith.andi %511, %465 : i1 loc(#loc73)
    %513 = arith.andi %512, %449 : i1 loc(#loc73)
    %514 = arith.andi %513, %343 : i1 loc(#loc73)
    %515 = acir.aggregate(%506, %507, %508, %509, %397, %466, %443, %510) {acir.id = "v24", acir.source_type = "src_isa_f0424bc8.Operands"} : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc73)
    %516 = arith.andi %356, %514 : i1
    %517 = arith.andi %339, %516 : i1
    %518 = arith.xori %517, %true : i1
    %519 = arith.ori %518, %341 : i1
    %520 = arith.andi %505, %519 : i1
    acir.path %517 {branch = 10 : i64, kind = "branch", txn = 1 : i64}
    %value_24, %ready_25 = acir.read(%517) {branch = 10 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.id_ex", txn = 1 : i64} : (i1) -> (i1, i1)
    %521 = arith.andi %341, %ready_25 : i1
    %522 = arith.ori %518, %521 : i1
    %523 = arith.andi %520, %522 : i1
    %524 = arith.xori %value_24, %true : i1
    %525 = arith.andi %341, %524 : i1
    %526 = arith.ori %518, %525 : i1
    %527 = arith.andi %523, %526 : i1
    %528 = acir.fire "top.decode_stage.read_operands#1" %527 {successors = [0], txn = 1 : i64}
    %529 = arith.andi %528, %339 : i1
    acir.effect %529() {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "pop", port = 0 : i64, resource = "top.if_id", txn = 1 : i64} : (i1) -> () loc(#loc55)
    acir.effect %529(%399) {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> ()
    acir.effect %529(%444) {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "check", port = 0 : i64, resource = "", txn = 1 : i64} : (i1, i1) -> ()
    %530 = arith.andi %528, %490 : i1
    acir.effect %530(%502) {branch = 9 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "write", port = 0 : i64, resource = "top.rf.pending", txn = 1 : i64} : (i1, !acir.array<i1, [32]>) -> () loc(#loc66)
    %531 = arith.andi %528, %517 : i1
    acir.effect %531(%515) {branch = 10 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.id_ex", txn = 1 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Operands">) -> ()
    acir.path %true {branch = 11 : i64, kind = "branch", txn = 2 : i64} loc(#loc74)
    acir.path %true {branch = 11 : i64, frames = ["top.execute:execute"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc74)
    %value_26, %ready_27 = acir.read(%true) {branch = 11 : i64, frames = ["top.execute:execute"], method = "peek", port = 0 : i64, resource = "top.id_ex", txn = 2 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Operands">, i1) loc(#loc75)
    %value_28, %ready_29 = acir.read(%true) {branch = 11 : i64, frames = ["top.execute:execute"], method = "empty", port = 0 : i64, resource = "top.id_ex", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc75)
    %532 = arith.andi %ready_27, %ready_29 : i1 loc(#loc75)
    %533 = arith.xori %value_28, %true : i1 loc(#loc75)
    %534 = arith.andi %532, %533 : i1 loc(#loc75)
    %535 = acir.extract %value_26 {acir.id = "v8", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc76)
    %536 = arith.cmpi eq, %535, %c1_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc76)
    %537 = arith.andi %534, %ready_27 : i1
    %538 = arith.cmpi ne, %535, %c1_i32 : i32 loc(#loc76)
    %539 = arith.cmpi eq, %535, %c1_i32 : i32 loc(#loc76)
    %540 = arith.ori %539, %ready_27 : i1
    %541 = acir.extract %value_26 {acir.id = "v14", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc77)
    %542 = acir.extract %value_26 {acir.id = "v15", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc78)
    %543 = arith.addi %541, %542 {acir.id = "v16", acir.source_type = "u32"} : i32 loc(#loc77)
    %544 = arith.ori %538, %ready_27 : i1
    %545 = acir.extract %value_26 {acir.id = "v11", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc79)
    %546 = acir.extract %value_26 {acir.id = "v12", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc80)
    %547 = arith.subi %545, %546 {acir.id = "v13", acir.source_type = "u32"} : i32 loc(#loc79)
    %548 = arith.andi %540, %ready_27 : i1
    %549 = arith.andi %538, %548 : i1
    %550 = arith.andi %544, %ready_27 : i1
    %551 = arith.andi %536, %550 : i1
    %552 = arith.ori %549, %551 : i1
    %553 = arith.select %538, %543, %547 : i32
    %554 = acir.extract %value_26 {acir.id = "v20", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc81)
    %555 = acir.extract %value_26 {acir.id = "v21", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc82)
    %556 = acir.extract %value_26 {acir.id = "v22", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc83)
    %557 = acir.extract %value_26 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc84)
    %558 = acir.extract %value_26 {acir.id = "v24", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc85)
    %559 = acir.extract %value_26 {acir.id = "v25", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc86)
    %560 = arith.andi %ready_27, %552 : i1 loc(#loc87)
    %561 = acir.aggregate(%554, %555, %556, %557, %553, %558, %559) {acir.id = "v26", acir.source_type = "src_isa_f0424bc8.Executed"} : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc87)
    acir.path %560 {branch = 12 : i64, kind = "branch", txn = 2 : i64}
    %value_30, %ready_31 = acir.read(%560) {branch = 12 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.ex_mem", txn = 2 : i64} : (i1) -> (i1, i1)
    %562 = arith.xori %560, %true : i1
    %563 = arith.ori %562, %ready_31 : i1
    %564 = arith.andi %537, %563 : i1
    %565 = arith.xori %value_30, %true : i1
    %566 = arith.ori %562, %565 : i1
    %567 = arith.andi %564, %566 : i1
    %568 = acir.fire "top.execute.execute#2" %567 {successors = [1], txn = 2 : i64}
    acir.effect %568() {branch = 11 : i64, frames = ["top.execute:execute"], method = "pop", port = 0 : i64, resource = "top.id_ex", txn = 2 : i64} : (i1) -> () loc(#loc75)
    %569 = arith.andi %568, %560 : i1
    acir.effect %569(%561) {branch = 12 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.ex_mem", txn = 2 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Executed">) -> ()
    acir.path %true {branch = 13 : i64, kind = "branch", txn = 3 : i64} loc(#loc88)
    acir.path %true {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc88)
    %value_32, %ready_33 = acir.read(%true) {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "peek", port = 0 : i64, resource = "top.ex_mem", txn = 3 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Executed">, i1) loc(#loc89)
    %value_34, %ready_35 = acir.read(%true) {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "empty", port = 0 : i64, resource = "top.ex_mem", txn = 3 : i64} : (i1) -> (i1, i1) loc(#loc89)
    %570 = arith.andi %ready_33, %ready_35 : i1 loc(#loc89)
    %571 = arith.xori %value_34, %true : i1 loc(#loc89)
    %572 = arith.andi %570, %571 : i1 loc(#loc89)
    %573 = acir.extract %value_32 {acir.id = "v8", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc90)
    %574 = acir.extract %value_32 {acir.id = "v9", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc91)
    %575 = arith.cmpi eq, %574, %c2_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc91)
    %576 = arith.andi %572, %ready_33 : i1
    %577 = arith.cmpi ne, %574, %c2_i32 : i32 loc(#loc91)
    %578 = arith.cmpi eq, %574, %c2_i32 : i32 loc(#loc91)
    %579 = arith.ori %578, %ready_33 : i1
    %580 = acir.extract %value_32 {acir.id = "v14", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc92)
    %581 = arith.cmpi eq, %580, %c3_i32 {acir.id = "v16", acir.source_type = "bool"} : i32 loc(#loc92)
    %582 = arith.andi %579, %ready_33 : i1
    %583 = arith.ori %578, %582 : i1
    %584 = arith.andi %576, %583 : i1
    %585 = arith.andi %577, %581 : i1
    %586 = arith.xori %585, %true : i1 loc(#loc93)
    %587 = arith.ori %586, %582 : i1
    %588 = acir.extract %value_32 {acir.id = "v17", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc94)
    %589 = acir.extract %value_32 {acir.id = "v18", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc95)
    acir.path %585 {branch = 14 : i64, kind = "branch", txn = 3 : i64} loc(#loc93)
    acir.path %585 {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc93)
    %590 = arith.ori %586, %587 : i1 loc(#loc93)
    %591 = arith.andi %587, %ready_33 : i1 loc(#loc93)
    %592 = arith.andi %585, %591 : i1 loc(#loc93)
    %593 = arith.andi %588, %c3_i32 {acir.id = "v7", acir.source_type = "u32"} : i32 loc(#loc96)
    %594 = arith.cmpi eq, %593, %c0_i32 {acir.id = "v9", acir.source_type = "bool"} : i32 loc(#loc96)
    %595 = arith.andi %590, %592 : i1
    %596 = arith.ori %586, %595 : i1
    %597 = arith.andi %584, %596 : i1
    %598 = arith.andi %585, %594 : i1
    %599 = arith.cmpi ne, %593, %c0_i32 : i32 loc(#loc96)
    %600 = arith.andi %585, %599 : i1
    %601 = arith.xori %600, %true : i1
    %602 = arith.ori %601, %595 : i1
    %603 = arith.xori %598, %true : i1
    %604 = arith.ori %603, %595 : i1
    %605 = arith.cmpi ult, %588, %c64_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc97)
    %606 = arith.andi %600, %602 : i1
    %607 = arith.andi %604, %592 : i1
    %608 = arith.andi %598, %607 : i1
    %609 = arith.ori %606, %608 : i1
    %610 = arith.select %600, %false, %605 : i1
    %611 = arith.ori %586, %590 : i1
    %612 = arith.andi %611, %609 : i1 loc(#loc98)
    %613 = arith.ori %586, %612 : i1 loc(#loc98)
    %614 = arith.andi %597, %613 : i1 loc(#loc98)
    %value_36, %ready_37 = acir.read(%585) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "read", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1) -> (!acir.array<i32, [16]>, i1) loc(#loc99)
    %615 = arith.andi %611, %ready_37 : i1 loc(#loc99)
    %616 = arith.ori %586, %615 : i1 loc(#loc99)
    %617 = arith.andi %614, %616 : i1 loc(#loc99)
    %618 = arith.shrui %588, %c2_i32 : i32 loc(#loc100)
    %619 = arith.andi %ready_37, %592 : i1 loc(#loc99)
    %620 = arith.andi %619, %592 : i1 loc(#loc99)
    %621 = arith.extui %618 : i32 to i64 loc(#loc99)
    %622 = arith.cmpi ult, %621, %c16_i64 : i64 loc(#loc99)
    %623 = arith.andi %620, %622 : i1 loc(#loc99)
    %624 = arith.select %622, %621, %c0_i64 : i64 loc(#loc99)
    %625 = acir.update %value_36, %589[%624] {acir.id = "v23", acir.source_type = "array<u32;16>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [16]>, i32, i64) -> !acir.array<i32, [16]> loc(#loc99)
    %626 = arith.andi %611, %623 : i1 loc(#loc99)
    %627 = arith.ori %586, %626 : i1 loc(#loc99)
    %628 = arith.andi %617, %627 : i1 loc(#loc99)
    %629 = arith.ori %577, %ready_33 : i1
    %630 = acir.extract %value_32 {acir.id = "v12", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc101)
    acir.path %575 {branch = 15 : i64, kind = "branch", txn = 3 : i64} loc(#loc102)
    acir.path %575 {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc102)
    %631 = arith.ori %577, %629 : i1 loc(#loc102)
    %632 = arith.andi %629, %ready_33 : i1 loc(#loc102)
    %633 = arith.andi %575, %632 : i1 loc(#loc102)
    %634 = arith.andi %630, %c3_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc103)
    %635 = arith.cmpi eq, %634, %c0_i32 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc103)
    %636 = arith.andi %631, %633 : i1
    %637 = arith.ori %577, %636 : i1
    %638 = arith.andi %628, %637 : i1
    %639 = arith.andi %575, %635 : i1
    %640 = arith.cmpi ne, %634, %c0_i32 : i32 loc(#loc103)
    %641 = arith.andi %575, %640 : i1
    %642 = arith.xori %641, %true : i1
    %643 = arith.ori %642, %636 : i1
    %644 = arith.xori %639, %true : i1
    %645 = arith.ori %644, %636 : i1
    %646 = arith.cmpi ult, %630, %c64_i32 {acir.id = "v12", acir.source_type = "bool"} : i32 loc(#loc104)
    %647 = arith.andi %641, %643 : i1
    %648 = arith.andi %645, %633 : i1
    %649 = arith.andi %639, %648 : i1
    %650 = arith.ori %647, %649 : i1
    %651 = arith.select %641, %false, %646 : i1
    %652 = arith.ori %577, %631 : i1
    %653 = arith.andi %652, %650 : i1 loc(#loc105)
    %654 = arith.ori %577, %653 : i1 loc(#loc105)
    %655 = arith.andi %638, %654 : i1 loc(#loc105)
    %value_38, %ready_39 = acir.read(%575) {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "read", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1) -> (!acir.array<i32, [16]>, i1) loc(#loc106)
    %656 = arith.andi %652, %ready_39 : i1 loc(#loc106)
    %657 = arith.ori %577, %656 : i1 loc(#loc106)
    %658 = arith.andi %655, %657 : i1 loc(#loc106)
    %659 = arith.shrui %630, %c2_i32 : i32 loc(#loc107)
    %660 = arith.andi %ready_39, %633 : i1 loc(#loc106)
    %661 = arith.extui %659 : i32 to i64 loc(#loc106)
    %662 = arith.cmpi ult, %661, %c16_i64 : i64 loc(#loc106)
    %663 = arith.select %662, %661, %c0_i64 : i64 loc(#loc106)
    %664 = arith.andi %660, %662 : i1 loc(#loc106)
    %665 = "acir.array.index"(%value_38, %663) : (!acir.array<i32, [16]>, i64) -> i32 loc(#loc106)
    %666 = arith.ori %577, %652 : i1
    %667 = arith.andi %658, %666 : i1
    %668 = arith.andi %652, %664 : i1
    %669 = arith.andi %575, %668 : i1
    %670 = arith.andi %577, %582 : i1
    %671 = arith.andi %629, %669 : i1
    %672 = arith.andi %575, %671 : i1
    %673 = arith.ori %670, %672 : i1
    %674 = arith.select %577, %573, %665 : i32
    %675 = acir.extract %value_32 {acir.id = "v27", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc108)
    %676 = acir.extract %value_32 {acir.id = "v28", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc109)
    %677 = acir.extract %value_32 {acir.id = "v29", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc110)
    %678 = acir.extract %value_32 {acir.id = "v30", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc111)
    %679 = arith.andi %ready_33, %673 : i1 loc(#loc112)
    %680 = acir.aggregate(%675, %676, %677, %674, %678) {acir.id = "v31", acir.source_type = "src_isa_f0424bc8.Retired"} : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc112)
    acir.path %679 {branch = 16 : i64, kind = "branch", txn = 3 : i64}
    %value_40, %ready_41 = acir.read(%679) {branch = 16 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.mem_wb", txn = 3 : i64} : (i1) -> (i1, i1)
    %681 = arith.xori %679, %true : i1
    %682 = arith.ori %681, %ready_41 : i1
    %683 = arith.andi %667, %682 : i1
    %684 = arith.xori %value_40, %true : i1
    %685 = arith.ori %681, %684 : i1
    %686 = arith.andi %683, %685 : i1
    %687 = acir.fire "top.memory_stage.access#3" %686 {successors = [2], txn = 3 : i64}
    acir.effect %687() {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "pop", port = 0 : i64, resource = "top.ex_mem", txn = 3 : i64} : (i1) -> () loc(#loc89)
    %688 = arith.andi %687, %585 : i1
    acir.effect %688(%610) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc98)
    acir.effect %688(%625) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "write", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1, !acir.array<i32, [16]>) -> () loc(#loc99)
    %689 = arith.andi %687, %575 : i1
    acir.effect %689(%651) {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc105)
    acir.effect %689(%93) {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> ()
    %690 = arith.andi %687, %679 : i1
    acir.effect %690(%680) {branch = 16 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.mem_wb", txn = 3 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Retired">) -> ()
    acir.path %true {branch = 17 : i64, kind = "branch", txn = 4 : i64} loc(#loc113)
    acir.path %true {branch = 17 : i64, frames = ["top.writeback:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc113)
    %value_42, %ready_43 = acir.read(%true) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "peek", port = 0 : i64, resource = "top.mem_wb", txn = 4 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Retired">, i1) loc(#loc114)
    %value_44, %ready_45 = acir.read(%true) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "empty", port = 0 : i64, resource = "top.mem_wb", txn = 4 : i64} : (i1) -> (i1, i1) loc(#loc114)
    %691 = arith.andi %ready_43, %ready_45 : i1 loc(#loc114)
    %692 = arith.xori %value_44, %true : i1 loc(#loc114)
    %693 = arith.andi %691, %692 : i1 loc(#loc114)
    acir.path %true {branch = 17 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc115)
    %694 = acir.extract %value_42 {acir.id = "v5", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc116)
    %695 = arith.andi %693, %ready_43 : i1
    %696 = arith.xori %694, %true : i1
    %697 = arith.ori %694, %ready_43 : i1
    %698 = arith.ori %696, %ready_43 : i1
    %699 = acir.extract %value_42 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc117)
    %700 = arith.cmpi ne, %699, %c0_i32 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc117)
    %701 = arith.andi %696, %697 : i1
    %702 = arith.andi %698, %ready_43 : i1
    %703 = arith.andi %694, %702 : i1
    %704 = arith.ori %701, %703 : i1
    %705 = arith.select %694, %700, %false : i1
    %706 = arith.andi %695, %704 : i1
    %707 = arith.xori %705, %true : i1 loc(#loc118)
    %708 = arith.ori %707, %704 : i1
    %709 = acir.extract %value_42 {acir.id = "v14", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc119)
    acir.path %705 {branch = 18 : i64, kind = "branch", txn = 4 : i64} loc(#loc118)
    %value_46, %ready_47 = acir.read(%705) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 4 : i64} : (i1) -> (!acir.array<i32, [32]>, i1) loc(#loc118)
    %710 = arith.andi %708, %ready_47 : i1 loc(#loc118)
    %711 = arith.ori %707, %710 : i1 loc(#loc118)
    %712 = arith.andi %706, %711 : i1 loc(#loc118)
    %713 = acir.extract %value_42 {acir.id = "v18", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc120)
    %714 = arith.andi %ready_47, %ready_43 : i1 loc(#loc118)
    %715 = arith.extui %713 : i32 to i64 loc(#loc118)
    %716 = arith.cmpi ult, %715, %c32_i64 : i64 loc(#loc118)
    %717 = arith.andi %714, %716 : i1 loc(#loc118)
    %718 = arith.select %716, %715, %c0_i64 : i64 loc(#loc118)
    %719 = acir.update %value_46, %709[%718] {acir.id = "v19", acir.source_type = "array<u32;32>", path = [unit], signed_indices = [false]} : (!acir.array<i32, [32]>, i32, i64) -> !acir.array<i32, [32]> loc(#loc118)
    %720 = arith.andi %708, %717 : i1 loc(#loc118)
    %721 = arith.ori %707, %720 : i1 loc(#loc118)
    %722 = arith.andi %712, %721 : i1 loc(#loc118)
    %value_48, %ready_49 = acir.read(%705) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = 4 : i64} : (i1) -> (!acir.array<i1, [32]>, i1) loc(#loc121)
    %723 = arith.andi %708, %ready_49 : i1 loc(#loc121)
    %724 = arith.ori %707, %723 : i1 loc(#loc121)
    %725 = arith.andi %722, %724 : i1 loc(#loc121)
    %726 = acir.extract %value_42 {acir.id = "v26", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc122)
    %727 = arith.andi %ready_49, %ready_43 : i1 loc(#loc121)
    %728 = arith.extui %726 : i32 to i64 loc(#loc121)
    %729 = arith.cmpi ult, %728, %c32_i64 : i64 loc(#loc121)
    %730 = arith.andi %727, %729 : i1 loc(#loc121)
    %731 = arith.select %729, %728, %c0_i64 : i64 loc(#loc121)
    %732 = acir.update %value_48, %false[%731] {acir.id = "v27", acir.source_type = "array<bool;32>", path = [unit], signed_indices = [false]} : (!acir.array<i1, [32]>, i1, i64) -> !acir.array<i1, [32]> loc(#loc121)
    %733 = arith.andi %708, %730 : i1 loc(#loc121)
    %734 = arith.ori %707, %733 : i1 loc(#loc121)
    %735 = arith.andi %725, %734 : i1 loc(#loc121)
    %value_50, %ready_51 = acir.read(%true) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "read", port = 0 : i64, resource = "top.writeback.retired", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc123)
    %736 = arith.andi %735, %ready_51 : i1 loc(#loc123)
    %737 = arith.addi %value_50, %c1_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc123)
    %738 = acir.extract %value_42 {acir.id = "v13", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc124)
    %739 = arith.andi %736, %ready_43 : i1 loc(#loc125)
    %740 = acir.fire "top.writeback.commit#4" %739 {successors = [3], txn = 4 : i64}
    acir.effect %740() {branch = 17 : i64, frames = ["top.writeback:commit"], method = "pop", port = 0 : i64, resource = "top.mem_wb", txn = 4 : i64} : (i1) -> () loc(#loc114)
    %741 = arith.andi %740, %705 : i1
    acir.effect %741(%719) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.registers", txn = 4 : i64} : (i1, !acir.array<i32, [32]>) -> () loc(#loc118)
    acir.effect %741(%732) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.pending", txn = 4 : i64} : (i1, !acir.array<i1, [32]>) -> () loc(#loc121)
    acir.effect %740(%737) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "write", port = 0 : i64, resource = "top.writeback.retired", txn = 4 : i64} : (i1, i32) -> () loc(#loc126)
    acir.effect %740(%738) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "write", port = 0 : i64, resource = "top.writeback.last_pc", txn = 4 : i64} : (i1, i32) -> () loc(#loc125)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":67:22)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":68:12)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:20)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:16)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:20)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:14)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:9)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:16)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":87:23)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":53:12)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:14)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:14)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":56:14)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:10)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:11)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":59:11)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:8)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":69:10)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:10)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:9)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:16)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:33)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:28)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:46)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:21)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:13)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:12)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:9)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:16)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:21)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:9)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:16)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:33)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:48)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":71:12)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:16)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:9)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:21)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:20)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:12)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:19)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:42)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:20)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:28)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:16)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:47)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:55)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:43)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:75)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:71)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:94)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:102)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:90)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:20)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":79:20)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":80:15)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:12)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:16)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:9)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:31)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:23)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:36)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:15)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:12)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:28)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:13)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:21)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:25)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:37)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:51)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:65)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:100)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:16)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:16)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":100:16)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:41)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:63)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:74)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:18)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:29)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:25)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:34)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:45)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:56)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:73)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:91)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:16)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:16)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":115:16)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":116:18)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":117:12)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":119:14)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:13)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:26)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:38)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:16)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:37)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:9)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:9)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:15)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":118:34)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":118:22)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:16)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:37)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:9)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:16)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:22)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:24)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:33)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:44)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:61)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:16)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:9)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":135:16)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":136:9)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:12)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:28)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:13)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:34)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:23)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:13)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:21)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:19)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:19)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:9)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:9)

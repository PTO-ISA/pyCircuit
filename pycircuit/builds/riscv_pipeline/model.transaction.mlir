module attributes {acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, exports = {}, format_version = 8 : i64, top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}, {default = 0 : i64, name = "immediate", type = "u32"}, {default = false, name = "immediate_rhs", type = "bool"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "lhs", type = "u32"}, {default = 0 : i64, name = "rhs", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = false, name = "writes", type = "bool"}]}}, acir.stage = "transaction"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.ex_mem", source_branch = 13 : i64, target_branches = [12], to = 2 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.ex_mem", source_branch = 13 : i64, target_branches = [12], to = 2 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.id_ex", source_branch = 11 : i64, target_branches = [10], to = 1 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.id_ex", source_branch = 11 : i64, target_branches = [10], to = 1 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.if_id", source_branch = 6 : i64, target_branches = [2], to = 0 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.if_id", source_branch = 6 : i64, target_branches = [2], to = 0 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mem_wb", source_branch = 17 : i64, target_branches = [16], to = 3 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mem_wb", source_branch = 17 : i64, target_branches = [16], to = 3 : i64}], schedule = [4, 3, 2, 1, 0], stage = "transaction"} {
    %true = arith.constant true
    %false = arith.constant false
    acir.state @top.ex_mem : !acir.struct<"src_isa_f0424bc8.Executed"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.fetch.pc : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.fetch.rom : !acir.array<i32, 6> {initial = [7340179 : i32, 5243155 : i32, 2130355 : i32, 3153955 : i32, 8707 : i32, 1075970739 : i32], kind = "cell"}
    acir.state @top.id_ex : !acir.struct<"src_isa_f0424bc8.Operands"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.if_id : !acir.struct<"src_isa_f0424bc8.Fetched"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.mem_wb : !acir.struct<"src_isa_f0424bc8.Retired"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.memory.words : !acir.array<i32, 16> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.rf.pending : !acir.array<i1, 32> {initial = [false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false], kind = "cell"}
    acir.state @top.rf.registers : !acir.array<i32, 32> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.writeback.last_pc : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.writeback.retired : i32 {initial = 0 : i32, kind = "cell"}
    %c24_i32 = arith.constant {acir.source_type = "u32"} 24 : i32 loc(#loc1)
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc2)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.fetch.pc", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc2)
    %0 = arith.cmpi ult, %value, %c24_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc3)
    %1 = arith.xori %0, %true : i1
    %2 = arith.xori %1, %true : i1
    %3 = arith.ori %2, %ready : i1
    %4 = arith.xori %0, %true : i1
    %5 = arith.ori %4, %ready : i1
    %6 = arith.andi %5, %ready : i1 loc(#loc4)
    %7 = arith.andi %0, %6 : i1 loc(#loc4)
    acir.path %0 {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc4)
    acir.path %0 {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc4)
    %8 = arith.xori %0, %true : i1 loc(#loc4)
    %9 = arith.ori %8, %5 : i1 loc(#loc4)
    %10 = arith.andi %5, %ready : i1 loc(#loc4)
    %11 = arith.andi %0, %10 : i1 loc(#loc4)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc5)
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc6)
    %12 = arith.shrui %value, %c2_i32 : i32 loc(#loc7)
    %value_0, %ready_1 = acir.read(%0) {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "read", port = 0 : i64, resource = "top.fetch.rom", txn = 0 : i64} : (i1) -> (!acir.array<i32, 6>, i1) loc(#loc8)
    %13 = arith.xori %0, %true : i1 loc(#loc8)
    %14 = arith.andi %9, %ready_1 : i1 loc(#loc8)
    %15 = arith.ori %13, %14 : i1 loc(#loc8)
    %16 = arith.andi %7, %15 : i1 loc(#loc8)
    %17 = arith.andi %ready_1, %11 : i1 loc(#loc8)
    %18 = acir.extract %value_0[%12] {acir.id = "v8", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 6>, i32) -> i32 loc(#loc8)
    %19 = arith.addi %value, %c4_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc9)
    %20 = arith.xori %0, %true : i1 loc(#loc10)
    %21 = arith.andi %9, %11 : i1 loc(#loc10)
    %22 = arith.ori %20, %21 : i1 loc(#loc10)
    %23 = arith.andi %16, %22 : i1 loc(#loc10)
    %24 = arith.andi %11, %17 : i1 loc(#loc11)
    %25 = acir.aggregate(%value, %18) {acir.id = "v13", acir.source_type = "src_isa_f0424bc8.Fetched"} : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc11)
    %26 = arith.andi %9, %24 : i1 loc(#loc12)
    %27 = arith.andi %0, %26 : i1 loc(#loc12)
    %28 = arith.andi %0, %27 : i1 loc(#loc12)
    %29 = arith.xori %28, %true : i1 loc(#loc12)
    %30 = arith.ori %29, %5 : i1 loc(#loc12)
    %31 = arith.andi %23, %30 : i1 loc(#loc12)
    acir.path %28 {branch = 2 : i64, kind = "branch", txn = 0 : i64} loc(#loc12)
    %value_2, %ready_3 = acir.read(%28) {branch = 2 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.if_id", txn = 0 : i64} : (i1) -> (i1, i1) loc(#loc12)
    %32 = arith.xori %28, %true : i1 loc(#loc12)
    %33 = arith.andi %5, %ready_3 : i1 loc(#loc12)
    %34 = arith.ori %32, %33 : i1 loc(#loc12)
    %35 = arith.andi %31, %34 : i1 loc(#loc12)
    %36 = arith.xori %value_2, %true : i1 loc(#loc12)
    %37 = arith.xori %28, %true : i1 loc(#loc12)
    %38 = arith.andi %5, %36 : i1 loc(#loc12)
    %39 = arith.ori %37, %38 : i1 loc(#loc12)
    %40 = arith.andi %35, %39 : i1 loc(#loc12)
    %41 = acir.fire "top.fetch.fetch#0" %40 {successors = [], txn = 0 : i64} loc(#loc12)
    %42 = arith.andi %41, %0 : i1 loc(#loc12)
    acir.effect %42(%19) {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "write", port = 0 : i64, resource = "top.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc10)
    %43 = arith.andi %41, %28 : i1 loc(#loc12)
    acir.effect %43(%25) {branch = 2 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.if_id", txn = 0 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Fetched">) -> () loc(#loc12)
    %44 = arith.andi %1, %3 : i1 loc(#loc13)
    %45 = arith.andi %0, %5 : i1 loc(#loc13)
    %46 = arith.ori %44, %45 : i1 loc(#loc13)
    %false_4 = arith.constant {acir.source_type = "bool"} false
    %value_5, %ready_6 = acir.read(%true) {branch = 0 : i64, frames = [], method = "peek", port = 0 : i64, resource = "top.if_id", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Fetched">, i1) loc(#loc14)
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %c35_i32 = arith.constant {acir.source_type = "u32"} 35 : i32
    %c2_i32_7 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c32_i32 = arith.constant {acir.source_type = "u32"} 32 : i32
    %c51_i32 = arith.constant {acir.source_type = "u32"} 51 : i32
    %c19_i32 = arith.constant {acir.source_type = "u32"} 19 : i32 loc(#loc15)
    %true_8 = arith.constant {acir.source_type = "bool"} true
    %false_9 = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c20_i32 = arith.constant {acir.source_type = "u32"} 20 : i32
    %c15_i32 = arith.constant {acir.source_type = "u32"} 15 : i32 loc(#loc16)
    %c31_i32 = arith.constant {acir.source_type = "u32"} 31 : i32
    %c25_i32 = arith.constant {acir.source_type = "u32"} 25 : i32
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    %c12_i32 = arith.constant {acir.source_type = "u32"} 12 : i32 loc(#loc17)
    %c127_i32 = arith.constant {acir.source_type = "u32"} 127 : i32 loc(#loc18)
    %47 = acir.extract %value_5 {acir.id = "v2", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc19)
    %48 = arith.andi %47, %c127_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc20)
    %49 = arith.shrui %47, %c12_i32 : i32 loc(#loc21)
    %50 = arith.andi %49, %c7_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc21)
    %51 = arith.shrui %47, %c25_i32 : i32 loc(#loc22)
    %52 = arith.shrui %47, %c7_i32 : i32 loc(#loc23)
    %53 = arith.andi %52, %c31_i32 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc23)
    %54 = arith.shrui %47, %c15_i32 : i32 loc(#loc24)
    %55 = arith.andi %54, %c31_i32 {acir.id = "v18", acir.source_type = "u32"} : i32 loc(#loc24)
    %56 = arith.shrui %47, %c20_i32 : i32 loc(#loc25)
    %57 = arith.andi %56, %c31_i32 {acir.id = "v22", acir.source_type = "u32"} : i32 loc(#loc25)
    %58 = arith.cmpi eq, %48, %c19_i32 {acir.id = "v28", acir.source_type = "bool"} : i32 loc(#loc26)
    %59 = arith.xori %58, %true : i1
    %60 = arith.xori %59, %true : i1
    %61 = arith.ori %60, %ready_6 : i1
    %62 = arith.cmpi eq, %48, %c51_i32 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc27)
    %63 = arith.cmpi ne, %48, %c51_i32 : i32 loc(#loc27)
    %64 = arith.andi %59, %62 : i1
    %65 = arith.andi %61, %ready_6 : i1
    %66 = arith.xori %62, %true : i1
    %67 = arith.andi %59, %66 : i1
    %68 = arith.andi %61, %ready_6 : i1
    %69 = arith.xori %67, %true : i1
    %70 = arith.ori %69, %68 : i1
    %71 = arith.cmpi eq, %48, %c3_i32 {acir.id = "v77", acir.source_type = "bool"} : i32 loc(#loc28)
    %72 = arith.select %71, %c2_i32_7, %c3_i32 : i32
    %73 = arith.andi %ready_6, %ready_6 : i1
    %74 = arith.select %71, %53, %c0_i32 : i32
    %75 = arith.andi %ready_6, %ready_6 : i1
    %76 = arith.select %71, %c0_i32, %57 : i32
    %77 = arith.andi %67, %71 : i1
    %78 = arith.andi %70, %ready_6 : i1
    %79 = arith.xori %71, %true : i1
    %80 = arith.andi %67, %79 : i1
    %81 = arith.andi %70, %ready_6 : i1
    %82 = arith.xori %80, %true : i1
    %83 = arith.ori %82, %81 : i1
    %84 = arith.cmpi eq, %48, %c35_i32 {acir.id = "v89", acir.source_type = "bool"} : i32 loc(#loc29)
    %85 = arith.andi %80, %84 : i1
    %86 = arith.andi %83, %ready_6 : i1
    %87 = arith.xori %84, %true : i1
    %88 = arith.andi %80, %87 : i1
    %89 = arith.andi %83, %ready_6 : i1
    %90 = arith.xori %88, %true : i1
    %91 = arith.ori %90, %89 : i1
    %92 = arith.xori %85, %true : i1
    %93 = arith.ori %92, %86 : i1
    %94 = arith.cmpi eq, %50, %c2_i32_7 {acir.id = "v92", acir.source_type = "bool"} : i32 loc(#loc30)
    %95 = arith.xori %88, %true : i1
    %96 = arith.ori %95, %91 : i1
    %97 = arith.ori %88, %85 : i1
    %98 = arith.xori %85, %true : i1
    %99 = arith.ori %98, %93 : i1
    %100 = arith.andi %96, %99 : i1
    %101 = arith.andi %88, %91 : i1
    %102 = arith.andi %93, %ready_6 : i1
    %103 = arith.andi %85, %102 : i1
    %104 = arith.ori %101, %103 : i1
    %105 = arith.select %88, %false_9, %94 : i1
    %106 = arith.xori %80, %true : i1
    %107 = arith.ori %106, %83 : i1
    %108 = acir.assert %105 loc(#loc31)
    %109 = arith.xori %80, %true : i1 loc(#loc31)
    %110 = arith.andi %104, %108 : i1 loc(#loc31)
    %111 = arith.ori %109, %110 : i1 loc(#loc31)
    %112 = arith.shrui %47, %c25_i32 : i32 loc(#loc32)
    %113 = arith.shli %112, %c5_i32 : i32 loc(#loc32)
    %114 = arith.shrui %47, %c7_i32 : i32 loc(#loc33)
    %115 = arith.andi %114, %c31_i32 {acir.id = "v107", acir.source_type = "u32"} : i32 loc(#loc33)
    %116 = arith.andi %ready_6, %ready_6 : i1 loc(#loc32)
    %117 = arith.ori %113, %115 {acir.id = "v108", acir.source_type = "u32"} : i32 loc(#loc32)
    %118 = arith.xori %80, %true : i1 loc(#loc34)
    %119 = arith.ori %118, %107 : i1 loc(#loc34)
    %120 = arith.andi %107, %116 : i1 loc(#loc34)
    %121 = arith.andi %80, %120 : i1 loc(#loc34)
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %122 = arith.xori %117, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc35)
    %123 = arith.subi %122, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc36)
    %124 = arith.andi %119, %121 : i1 loc(#loc37)
    %125 = arith.andi %80, %124 : i1 loc(#loc37)
    %126 = arith.xori %77, %true : i1
    %127 = arith.ori %126, %78 : i1
    %128 = arith.cmpi eq, %50, %c2_i32_7 {acir.id = "v80", acir.source_type = "bool"} : i32 loc(#loc38)
    %129 = acir.assert %128 loc(#loc39)
    %130 = arith.xori %77, %true : i1 loc(#loc39)
    %131 = arith.andi %ready_6, %129 : i1 loc(#loc39)
    %132 = arith.ori %130, %131 : i1 loc(#loc39)
    %133 = arith.andi %111, %132 : i1 loc(#loc39)
    %134 = arith.shrui %47, %c20_i32 : i32 loc(#loc40)
    %135 = arith.xori %77, %true : i1 loc(#loc41)
    %136 = arith.ori %135, %127 : i1 loc(#loc41)
    %137 = arith.andi %127, %ready_6 : i1 loc(#loc41)
    %138 = arith.andi %77, %137 : i1 loc(#loc41)
    %c2048_i32_10 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %139 = arith.xori %134, %c2048_i32_10 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc35)
    %140 = arith.subi %139, %c2048_i32_10 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc36)
    %141 = arith.andi %136, %138 : i1 loc(#loc37)
    %142 = arith.andi %77, %141 : i1 loc(#loc37)
    %143 = arith.xori %80, %true : i1
    %144 = arith.ori %143, %107 : i1
    %145 = arith.ori %80, %77 : i1
    %146 = arith.xori %77, %true : i1
    %147 = arith.ori %146, %127 : i1
    %148 = arith.andi %144, %147 : i1
    %149 = arith.andi %107, %125 : i1
    %150 = arith.andi %80, %149 : i1
    %151 = arith.andi %127, %142 : i1
    %152 = arith.andi %77, %151 : i1
    %153 = arith.ori %150, %152 : i1
    %154 = arith.select %80, %123, %140 : i32
    %155 = arith.xori %67, %true : i1
    %156 = arith.ori %155, %70 : i1
    %157 = arith.andi %153, %ready_6 : i1
    %158 = arith.andi %157, %73 : i1
    %159 = arith.andi %158, %75 : i1
    %160 = arith.andi %159, %ready_6 : i1
    %161 = arith.xori %64, %true : i1
    %162 = arith.ori %161, %65 : i1
    %163 = arith.cmpi eq, %50, %c0_i32 {acir.id = "v42", acir.source_type = "bool"} : i32 loc(#loc42)
    %164 = arith.andi %64, %163 : i1
    %165 = arith.andi %162, %ready_6 : i1
    %166 = arith.xori %163, %true : i1
    %167 = arith.andi %64, %166 : i1
    %168 = arith.andi %162, %ready_6 : i1
    %169 = arith.xori %167, %true : i1
    %170 = arith.ori %169, %168 : i1
    %171 = arith.xori %164, %true : i1
    %172 = arith.ori %171, %165 : i1
    %173 = arith.cmpi eq, %51, %c0_i32 {acir.id = "v45", acir.source_type = "bool"} : i32 loc(#loc43)
    %174 = arith.andi %164, %173 : i1
    %175 = arith.andi %172, %ready_6 : i1
    %176 = arith.xori %173, %true : i1
    %177 = arith.andi %164, %176 : i1
    %178 = arith.andi %172, %ready_6 : i1
    %179 = arith.xori %177, %true : i1
    %180 = arith.ori %179, %178 : i1
    %181 = arith.cmpi eq, %51, %c32_i32 {acir.id = "v49", acir.source_type = "bool"} : i32 loc(#loc44)
    %182 = arith.xori %174, %true : i1
    %183 = arith.ori %182, %175 : i1
    %184 = arith.xori %177, %true : i1
    %185 = arith.ori %184, %180 : i1
    %186 = arith.ori %177, %174 : i1
    %187 = arith.xori %174, %true : i1
    %188 = arith.ori %187, %183 : i1
    %189 = arith.andi %185, %188 : i1
    %190 = arith.andi %180, %ready_6 : i1
    %191 = arith.andi %177, %190 : i1
    %192 = arith.andi %174, %183 : i1
    %193 = arith.ori %191, %192 : i1
    %194 = arith.select %177, %181, %true_8 : i1
    %195 = arith.xori %164, %true : i1
    %196 = arith.ori %195, %172 : i1
    %197 = arith.xori %167, %true : i1
    %198 = arith.ori %197, %170 : i1
    %199 = arith.ori %167, %164 : i1
    %200 = arith.xori %164, %true : i1
    %201 = arith.ori %200, %196 : i1
    %202 = arith.andi %198, %201 : i1
    %203 = arith.andi %167, %170 : i1
    %204 = arith.andi %196, %193 : i1
    %205 = arith.andi %164, %204 : i1
    %206 = arith.ori %203, %205 : i1
    %207 = arith.select %167, %false_9, %194 : i1
    %208 = arith.xori %64, %true : i1
    %209 = arith.ori %208, %162 : i1
    %210 = acir.assert %207 loc(#loc45)
    %211 = arith.xori %64, %true : i1 loc(#loc45)
    %212 = arith.andi %206, %210 : i1 loc(#loc45)
    %213 = arith.ori %211, %212 : i1 loc(#loc45)
    %214 = arith.andi %133, %213 : i1 loc(#loc45)
    %215 = arith.cmpi eq, %51, %c32_i32 {acir.id = "v59", acir.source_type = "bool"} : i32 loc(#loc46)
    %216 = arith.extui %215 : i1 to i32
    %217 = arith.andi %ready_6, %ready_6 : i1
    %218 = arith.andi %217, %ready_6 : i1
    %219 = arith.xori %67, %true : i1
    %220 = arith.ori %219, %156 : i1
    %221 = arith.ori %67, %64 : i1
    %222 = arith.xori %64, %true : i1
    %223 = arith.ori %222, %209 : i1
    %224 = arith.andi %220, %223 : i1
    %225 = arith.andi %156, %153 : i1
    %226 = arith.andi %67, %225 : i1
    %227 = arith.andi %64, %209 : i1
    %228 = arith.ori %226, %227 : i1
    %229 = arith.select %67, %154, %c0_i32 : i32
    %230 = arith.andi %156, %ready_6 : i1
    %231 = arith.andi %67, %230 : i1
    %232 = arith.andi %209, %ready_6 : i1
    %233 = arith.andi %64, %232 : i1
    %234 = arith.ori %231, %233 : i1
    %235 = arith.select %67, %72, %216 : i32
    %236 = arith.andi %156, %73 : i1
    %237 = arith.andi %67, %236 : i1
    %238 = arith.andi %209, %ready_6 : i1
    %239 = arith.andi %64, %238 : i1
    %240 = arith.ori %237, %239 : i1
    %241 = arith.select %67, %74, %53 : i32
    %242 = arith.andi %156, %75 : i1
    %243 = arith.andi %67, %242 : i1
    %244 = arith.andi %209, %ready_6 : i1
    %245 = arith.andi %64, %244 : i1
    %246 = arith.ori %243, %245 : i1
    %247 = arith.select %67, %76, %57 : i32
    %248 = arith.andi %156, %ready_6 : i1
    %249 = arith.andi %67, %248 : i1
    %250 = arith.andi %64, %209 : i1
    %251 = arith.ori %249, %250 : i1
    %252 = arith.select %67, %71, %true_8 : i1
    %253 = arith.xori %59, %true : i1
    %254 = arith.ori %253, %61 : i1
    %255 = arith.andi %228, %ready_6 : i1
    %256 = arith.andi %255, %234 : i1
    %257 = arith.andi %256, %240 : i1
    %258 = arith.andi %257, %246 : i1
    %259 = arith.andi %258, %251 : i1
    %260 = arith.xori %58, %true : i1
    %261 = arith.ori %260, %ready_6 : i1
    %262 = arith.cmpi eq, %50, %c0_i32 {acir.id = "v31", acir.source_type = "bool"} : i32 loc(#loc47)
    %263 = acir.assert %262 loc(#loc48)
    %264 = arith.xori %58, %true : i1 loc(#loc48)
    %265 = arith.andi %ready_6, %263 : i1 loc(#loc48)
    %266 = arith.ori %264, %265 : i1 loc(#loc48)
    %267 = arith.andi %214, %266 : i1 loc(#loc48)
    %268 = arith.shrui %47, %c20_i32 : i32 loc(#loc49)
    %269 = arith.xori %58, %true : i1 loc(#loc50)
    %270 = arith.ori %269, %261 : i1 loc(#loc50)
    %271 = arith.andi %261, %ready_6 : i1 loc(#loc50)
    %272 = arith.andi %58, %271 : i1 loc(#loc50)
    %c2048_i32_11 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %273 = arith.xori %268, %c2048_i32_11 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc35)
    %274 = arith.subi %273, %c2048_i32_11 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc36)
    %275 = arith.andi %270, %272 : i1 loc(#loc37)
    %276 = arith.andi %58, %275 : i1 loc(#loc37)
    %277 = arith.andi %276, %ready_6 : i1
    %278 = arith.xori %59, %true : i1
    %279 = arith.ori %278, %254 : i1
    %280 = arith.ori %59, %58 : i1
    %281 = arith.xori %58, %true : i1
    %282 = arith.ori %281, %261 : i1
    %283 = arith.andi %279, %282 : i1
    %284 = arith.andi %254, %228 : i1
    %285 = arith.andi %59, %284 : i1
    %286 = arith.andi %261, %276 : i1
    %287 = arith.andi %58, %286 : i1
    %288 = arith.ori %285, %287 : i1
    %289 = arith.select %59, %229, %274 : i32
    %290 = arith.andi %254, %ready_6 : i1
    %291 = arith.andi %59, %290 : i1
    %292 = arith.andi %58, %261 : i1
    %293 = arith.ori %291, %292 : i1
    %294 = arith.select %59, %63, %true_8 : i1
    %295 = arith.andi %254, %234 : i1
    %296 = arith.andi %59, %295 : i1
    %297 = arith.andi %58, %261 : i1
    %298 = arith.ori %296, %297 : i1
    %299 = arith.select %59, %235, %c0_i32 : i32
    %300 = arith.andi %254, %240 : i1
    %301 = arith.andi %59, %300 : i1
    %302 = arith.andi %261, %ready_6 : i1
    %303 = arith.andi %58, %302 : i1
    %304 = arith.ori %301, %303 : i1
    %305 = arith.select %59, %241, %53 : i32
    %306 = arith.andi %254, %246 : i1
    %307 = arith.andi %59, %306 : i1
    %308 = arith.andi %58, %261 : i1
    %309 = arith.ori %307, %308 : i1
    %310 = arith.select %59, %247, %c0_i32 : i32
    %311 = arith.andi %254, %251 : i1
    %312 = arith.andi %59, %311 : i1
    %313 = arith.andi %58, %261 : i1
    %314 = arith.ori %312, %313 : i1
    %315 = arith.select %59, %252, %true_8 : i1
    %316 = acir.extract %value_5 {acir.id = "v154", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc51)
    %317 = arith.andi %ready_6, %ready_6 : i1 loc(#loc52)
    %318 = arith.andi %317, %298 : i1 loc(#loc52)
    %319 = arith.andi %318, %304 : i1 loc(#loc52)
    %320 = arith.andi %319, %ready_6 : i1 loc(#loc52)
    %321 = arith.andi %320, %309 : i1 loc(#loc52)
    %322 = arith.andi %321, %288 : i1 loc(#loc52)
    %323 = arith.andi %322, %293 : i1 loc(#loc52)
    %324 = arith.andi %323, %314 : i1 loc(#loc52)
    %325 = acir.aggregate(%316, %47, %299, %305, %55, %310, %289, %294, %315) {acir.id = "v155", acir.source_type = "src_isa_f0424bc8.Decoded"} : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc52)
    %326 = arith.andi %324, %267 : i1 loc(#loc53)
    %value_12, %ready_13 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.mem_wb", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc54)
    %327 = arith.xori %value_12, %true : i1
    %328 = arith.xori %value_12, %true : i1
    %329 = arith.ori %328, %ready_13 : i1
    acir.path %value_12 {branch = 3 : i64, kind = "branch", txn = -1 : i64} loc(#loc55)
    acir.path %value_12 {branch = 3 : i64, frames = ["top.rf:ready"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc55)
    %330 = arith.xori %value_12, %true : i1 loc(#loc55)
    %331 = arith.ori %330, %329 : i1 loc(#loc55)
    %332 = arith.andi %329, %326 : i1 loc(#loc55)
    %333 = arith.andi %value_12, %332 : i1 loc(#loc55)
    %true_14 = arith.constant {acir.source_type = "bool"} true
    %false_15 = arith.constant {acir.source_type = "bool"} false
    %334 = acir.extract %325 {acir.id = "v5", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc56)
    %value_16, %ready_17 = acir.read(%value_12) {branch = 3 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = -1 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc57)
    %335 = arith.andi %ready_17, %333 : i1 loc(#loc57)
    %336 = acir.extract %value_16[%334] {acir.id = "v7", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc57)
    %337 = acir.unary %336 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc58)
    %338 = arith.andi %value_12, %337 : i1
    %339 = arith.andi %331, %335 : i1
    %340 = arith.xori %337, %true : i1
    %341 = arith.andi %value_12, %340 : i1
    %342 = arith.andi %331, %335 : i1
    %343 = arith.xori %341, %true : i1
    %344 = arith.ori %343, %342 : i1
    %345 = arith.xori %338, %true : i1
    %346 = arith.ori %345, %339 : i1
    %347 = acir.extract %325 {acir.id = "v11", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc59)
    acir.path %338 {branch = 4 : i64, kind = "branch", txn = -1 : i64} loc(#loc60)
    %value_18, %ready_19 = acir.read(%338) {branch = 4 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = -1 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc60)
    %348 = arith.andi %ready_19, %333 : i1 loc(#loc60)
    %349 = acir.extract %value_18[%347] {acir.id = "v13", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc60)
    %350 = acir.unary %349 "not" {acir.id = "v14", acir.source_type = "bool"} : i1 -> i1 loc(#loc61)
    %351 = arith.xori %341, %true : i1
    %352 = arith.ori %351, %344 : i1
    %353 = arith.ori %341, %338 : i1
    %354 = arith.xori %338, %true : i1
    %355 = arith.ori %354, %346 : i1
    %356 = arith.andi %352, %355 : i1
    %357 = arith.andi %341, %344 : i1
    %358 = arith.andi %346, %348 : i1
    %359 = arith.andi %338, %358 : i1
    %360 = arith.ori %357, %359 : i1
    %361 = arith.select %341, %false_15, %350 : i1
    %362 = arith.xori %value_12, %true : i1
    %363 = arith.ori %362, %331 : i1
    %364 = arith.andi %value_12, %361 : i1
    %365 = arith.andi %363, %360 : i1
    %366 = arith.xori %361, %true : i1
    %367 = arith.andi %value_12, %366 : i1
    %368 = arith.andi %363, %360 : i1
    %369 = arith.xori %367, %true : i1
    %370 = arith.ori %369, %368 : i1
    %371 = arith.xori %364, %true : i1
    %372 = arith.ori %371, %365 : i1
    %373 = acir.extract %325 {acir.id = "v20", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc62)
    %374 = acir.unary %373 "not" {acir.id = "v21", acir.source_type = "bool"} : i1 -> i1 loc(#loc63)
    %375 = arith.andi %364, %374 : i1
    %376 = arith.andi %372, %333 : i1
    %377 = arith.xori %374, %true : i1
    %378 = arith.andi %364, %377 : i1
    %379 = arith.andi %372, %333 : i1
    %380 = arith.xori %378, %true : i1
    %381 = arith.ori %380, %379 : i1
    %382 = acir.extract %325 {acir.id = "v25", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc64)
    acir.path %378 {branch = 5 : i64, kind = "branch", txn = -1 : i64} loc(#loc65)
    %value_20, %ready_21 = acir.read(%378) {branch = 5 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = -1 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc65)
    %383 = arith.andi %ready_21, %333 : i1 loc(#loc65)
    %384 = acir.extract %value_20[%382] {acir.id = "v27", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc65)
    %385 = acir.unary %384 "not" {acir.id = "v28", acir.source_type = "bool"} : i1 -> i1 loc(#loc66)
    %386 = arith.xori %375, %true : i1
    %387 = arith.ori %386, %376 : i1
    %388 = arith.xori %378, %true : i1
    %389 = arith.ori %388, %381 : i1
    %390 = arith.ori %378, %375 : i1
    %391 = arith.xori %375, %true : i1
    %392 = arith.ori %391, %387 : i1
    %393 = arith.andi %389, %392 : i1
    %394 = arith.andi %381, %383 : i1
    %395 = arith.andi %378, %394 : i1
    %396 = arith.andi %375, %387 : i1
    %397 = arith.ori %395, %396 : i1
    %398 = arith.select %378, %385, %true_14 : i1
    %399 = arith.xori %364, %true : i1
    %400 = arith.ori %399, %372 : i1
    %401 = arith.xori %367, %true : i1
    %402 = arith.ori %401, %370 : i1
    %403 = arith.ori %367, %364 : i1
    %404 = arith.xori %364, %true : i1
    %405 = arith.ori %404, %400 : i1
    %406 = arith.andi %402, %405 : i1
    %407 = arith.andi %367, %370 : i1
    %408 = arith.andi %400, %397 : i1
    %409 = arith.andi %364, %408 : i1
    %410 = arith.ori %407, %409 : i1
    %411 = arith.select %367, %false_15, %398 : i1
    %412 = arith.xori %value_12, %true : i1
    %413 = arith.ori %412, %363 : i1
    %414 = arith.andi %413, %410 : i1 loc(#loc67)
    %415 = arith.andi %value_12, %414 : i1 loc(#loc67)
    %416 = arith.xori %327, %true : i1
    %417 = arith.ori %416, %ready_13 : i1
    %418 = arith.ori %327, %value_12 : i1
    %419 = arith.xori %value_12, %true : i1
    %420 = arith.ori %419, %329 : i1
    %421 = arith.andi %417, %420 : i1
    %422 = arith.andi %327, %ready_13 : i1
    %423 = arith.andi %329, %415 : i1
    %424 = arith.andi %value_12, %423 : i1
    %425 = arith.ori %422, %424 : i1
    %426 = arith.select %327, %false_4, %411 : i1
    %427 = arith.xori %426, %true : i1
    %428 = arith.xori %427, %true : i1
    %429 = arith.ori %428, %425 : i1
    %430 = arith.xori %426, %true : i1
    %431 = arith.ori %430, %425 : i1
    %432 = arith.andi %431, %326 : i1 loc(#loc68)
    %433 = arith.andi %426, %432 : i1 loc(#loc68)
    acir.path %426 {branch = 6 : i64, kind = "branch", txn = 1 : i64} loc(#loc68)
    acir.path %426 {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc68)
    %434 = arith.xori %426, %true : i1 loc(#loc68)
    %435 = arith.ori %434, %431 : i1 loc(#loc68)
    %436 = arith.andi %426, %431 : i1 loc(#loc68)
    %437 = arith.andi %431, %326 : i1 loc(#loc68)
    %438 = arith.andi %426, %437 : i1 loc(#loc68)
    %value_22, %ready_23 = acir.read(%426) {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "peek", port = 0 : i64, resource = "top.if_id", txn = 1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Fetched">, i1) loc(#loc69)
    %439 = arith.xori %426, %true : i1 loc(#loc69)
    %440 = arith.andi %435, %ready_23 : i1 loc(#loc69)
    %441 = arith.ori %439, %440 : i1 loc(#loc69)
    %442 = arith.andi %433, %441 : i1 loc(#loc69)
    %443 = arith.xori %426, %true : i1 loc(#loc69)
    %444 = arith.andi %435, %ready_23 : i1 loc(#loc69)
    %445 = arith.ori %443, %444 : i1 loc(#loc69)
    %446 = arith.andi %442, %445 : i1 loc(#loc69)
    %value_24, %ready_25 = acir.read(%426) {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "empty", port = 0 : i64, resource = "top.if_id", txn = 1 : i64} : (i1) -> (i1, i1) loc(#loc69)
    %447 = arith.xori %426, %true : i1 loc(#loc69)
    %448 = arith.andi %435, %ready_25 : i1 loc(#loc69)
    %449 = arith.ori %447, %448 : i1 loc(#loc69)
    %450 = arith.andi %446, %449 : i1 loc(#loc69)
    %451 = arith.xori %value_24, %true : i1 loc(#loc69)
    %452 = arith.xori %426, %true : i1 loc(#loc69)
    %453 = arith.andi %435, %451 : i1 loc(#loc69)
    %454 = arith.ori %452, %453 : i1 loc(#loc69)
    %455 = arith.andi %450, %454 : i1 loc(#loc69)
    %456 = acir.extract %325 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc70)
    acir.path %426 {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc71)
    %457 = arith.xori %426, %true : i1 loc(#loc71)
    %458 = arith.ori %457, %435 : i1 loc(#loc71)
    %459 = arith.andi %435, %438 : i1 loc(#loc71)
    %460 = arith.andi %426, %459 : i1 loc(#loc71)
    %c0_i32_26 = arith.constant {acir.source_type = "u32"} 0 : i32
    %461 = arith.cmpi eq, %456, %c0_i32_26 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc72)
    %462 = arith.andi %460, %460 : i1
    %463 = arith.xori %426, %true : i1
    %464 = arith.andi %458, %460 : i1
    %465 = arith.ori %463, %464 : i1
    %466 = arith.andi %455, %465 : i1
    %467 = arith.andi %426, %461 : i1
    %468 = arith.andi %458, %460 : i1
    %469 = arith.xori %461, %true : i1
    %470 = arith.andi %426, %469 : i1
    %471 = arith.andi %458, %460 : i1
    %472 = arith.xori %470, %true : i1
    %473 = arith.ori %472, %471 : i1
    %474 = arith.andi %471, %460 : i1
    %475 = arith.andi %470, %474 : i1
    acir.path %470 {branch = 7 : i64, kind = "branch", txn = 1 : i64} loc(#loc73)
    %value_27, %ready_28 = acir.read(%470) {branch = 7 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 1 : i64} : (i1) -> (!acir.array<i32, 32>, i1) loc(#loc73)
    %476 = arith.xori %470, %true : i1 loc(#loc73)
    %477 = arith.andi %473, %ready_28 : i1 loc(#loc73)
    %478 = arith.ori %476, %477 : i1 loc(#loc73)
    %479 = arith.andi %466, %478 : i1 loc(#loc73)
    %480 = arith.andi %ready_28, %475 : i1 loc(#loc73)
    %481 = acir.extract %value_27[%456] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc73)
    %482 = arith.xori %467, %true : i1 loc(#loc74)
    %483 = arith.ori %482, %468 : i1 loc(#loc74)
    %484 = arith.andi %473, %480 : i1 loc(#loc75)
    %485 = arith.andi %470, %484 : i1 loc(#loc75)
    %486 = arith.andi %467, %483 : i1 loc(#loc75)
    %487 = arith.ori %485, %486 : i1 loc(#loc75)
    %488 = arith.select %470, %481, %c0_i32_26 : i32 loc(#loc75)
    %489 = acir.extract %325 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc76)
    acir.path %426 {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc77)
    %490 = arith.xori %426, %true : i1 loc(#loc77)
    %491 = arith.ori %490, %435 : i1 loc(#loc77)
    %492 = arith.andi %435, %438 : i1 loc(#loc77)
    %493 = arith.andi %426, %492 : i1 loc(#loc77)
    %c0_i32_29 = arith.constant {acir.source_type = "u32"} 0 : i32
    %494 = arith.cmpi eq, %489, %c0_i32_29 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc72)
    %495 = arith.andi %493, %493 : i1
    %496 = arith.xori %426, %true : i1
    %497 = arith.andi %491, %493 : i1
    %498 = arith.ori %496, %497 : i1
    %499 = arith.andi %479, %498 : i1
    %500 = arith.andi %426, %494 : i1
    %501 = arith.andi %491, %493 : i1
    %502 = arith.xori %494, %true : i1
    %503 = arith.andi %426, %502 : i1
    %504 = arith.andi %491, %493 : i1
    %505 = arith.xori %503, %true : i1
    %506 = arith.ori %505, %504 : i1
    %507 = arith.andi %504, %493 : i1
    %508 = arith.andi %503, %507 : i1
    acir.path %503 {branch = 8 : i64, kind = "branch", txn = 1 : i64} loc(#loc73)
    %value_30, %ready_31 = acir.read(%503) {branch = 8 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 1 : i64} : (i1) -> (!acir.array<i32, 32>, i1) loc(#loc73)
    %509 = arith.xori %503, %true : i1 loc(#loc73)
    %510 = arith.andi %506, %ready_31 : i1 loc(#loc73)
    %511 = arith.ori %509, %510 : i1 loc(#loc73)
    %512 = arith.andi %499, %511 : i1 loc(#loc73)
    %513 = arith.andi %ready_31, %508 : i1 loc(#loc73)
    %514 = acir.extract %value_30[%489] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc73)
    %515 = arith.xori %500, %true : i1 loc(#loc74)
    %516 = arith.ori %515, %501 : i1 loc(#loc74)
    %517 = arith.andi %506, %513 : i1 loc(#loc75)
    %518 = arith.andi %503, %517 : i1 loc(#loc75)
    %519 = arith.andi %500, %516 : i1 loc(#loc75)
    %520 = arith.ori %518, %519 : i1 loc(#loc75)
    %521 = arith.select %503, %514, %c0_i32_29 : i32 loc(#loc75)
    %522 = acir.extract %325 {acir.id = "v10", acir.source_type = "bool", field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc78)
    %523 = arith.xori %426, %true : i1
    %524 = arith.andi %435, %438 : i1
    %525 = arith.ori %523, %524 : i1
    %526 = arith.andi %512, %525 : i1
    %527 = arith.andi %426, %522 : i1
    %528 = arith.andi %435, %438 : i1
    %529 = arith.xori %522, %true : i1
    %530 = arith.andi %426, %529 : i1
    %531 = arith.andi %435, %438 : i1
    %532 = arith.xori %530, %true : i1
    %533 = arith.ori %532, %531 : i1
    %534 = arith.xori %527, %true : i1
    %535 = arith.ori %534, %528 : i1
    %536 = acir.extract %325 {acir.id = "v12", acir.source_type = "u32", field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc79)
    %537 = arith.xori %530, %true : i1
    %538 = arith.ori %537, %533 : i1
    %539 = arith.ori %530, %527 : i1
    %540 = arith.xori %527, %true : i1
    %541 = arith.ori %540, %535 : i1
    %542 = arith.andi %538, %541 : i1
    %543 = arith.andi %533, %520 : i1
    %544 = arith.andi %530, %543 : i1
    %545 = arith.andi %535, %438 : i1
    %546 = arith.andi %527, %545 : i1
    %547 = arith.ori %544, %546 : i1
    %548 = arith.select %530, %521, %536 : i32
    %549 = arith.xori %426, %true : i1
    %550 = arith.ori %549, %435 : i1
    acir.path %426 {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc80)
    %551 = arith.xori %426, %true : i1 loc(#loc80)
    %552 = arith.ori %551, %550 : i1 loc(#loc80)
    %553 = arith.andi %550, %438 : i1 loc(#loc80)
    %554 = arith.andi %426, %553 : i1 loc(#loc80)
    %true_32 = arith.constant {acir.source_type = "bool"} true
    %false_33 = arith.constant {acir.source_type = "bool"} false
    %c0_i32_34 = arith.constant {acir.source_type = "u32"} 0 : i32
    %555 = acir.extract %325 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc81)
    %556 = arith.xori %426, %true : i1
    %557 = arith.andi %552, %554 : i1
    %558 = arith.ori %556, %557 : i1
    %559 = arith.andi %526, %558 : i1
    %560 = arith.andi %426, %555 : i1
    %561 = arith.andi %552, %554 : i1
    %562 = arith.xori %555, %true : i1
    %563 = arith.andi %426, %562 : i1
    %564 = arith.andi %552, %554 : i1
    %565 = arith.xori %563, %true : i1
    %566 = arith.ori %565, %564 : i1
    %567 = arith.xori %560, %true : i1
    %568 = arith.ori %567, %561 : i1
    %569 = acir.extract %325 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc82)
    %570 = arith.cmpi ne, %569, %c0_i32_34 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc82)
    %571 = arith.xori %563, %true : i1
    %572 = arith.ori %571, %566 : i1
    %573 = arith.ori %563, %560 : i1
    %574 = arith.xori %560, %true : i1
    %575 = arith.ori %574, %568 : i1
    %576 = arith.andi %572, %575 : i1
    %577 = arith.andi %563, %566 : i1
    %578 = arith.andi %568, %554 : i1
    %579 = arith.andi %560, %578 : i1
    %580 = arith.ori %577, %579 : i1
    %581 = arith.select %563, %false_33, %570 : i1
    %582 = arith.xori %426, %true : i1
    %583 = arith.ori %582, %552 : i1
    %584 = arith.xori %426, %true : i1
    %585 = arith.andi %583, %580 : i1
    %586 = arith.ori %584, %585 : i1
    %587 = arith.andi %559, %586 : i1
    %588 = arith.andi %426, %581 : i1
    %589 = arith.andi %583, %580 : i1
    %590 = arith.xori %581, %true : i1
    %591 = arith.andi %426, %590 : i1
    %592 = arith.andi %583, %580 : i1
    %593 = arith.xori %588, %true : i1
    %594 = arith.ori %593, %589 : i1
    acir.path %588 {branch = 9 : i64, kind = "branch", txn = 1 : i64} loc(#loc83)
    %value_35, %ready_36 = acir.read(%588) {branch = 9 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = 1 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc83)
    %595 = arith.xori %588, %true : i1 loc(#loc83)
    %596 = arith.andi %594, %ready_36 : i1 loc(#loc83)
    %597 = arith.ori %595, %596 : i1 loc(#loc83)
    %598 = arith.andi %587, %597 : i1 loc(#loc83)
    %value_37, %ready_38 = acir.read(%588) {branch = 9 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = 1 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc83)
    %599 = arith.xori %588, %true : i1 loc(#loc83)
    %600 = arith.andi %594, %ready_38 : i1 loc(#loc83)
    %601 = arith.ori %599, %600 : i1 loc(#loc83)
    %602 = arith.andi %598, %601 : i1 loc(#loc83)
    %603 = acir.extract %325 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc84)
    %604 = arith.andi %ready_38, %554 : i1 loc(#loc83)
    %605 = acir.update %value_37, %true_32[%603] {acir.id = "v24", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc83)
    %606 = arith.xori %588, %true : i1 loc(#loc83)
    %607 = arith.andi %594, %604 : i1 loc(#loc83)
    %608 = arith.ori %606, %607 : i1 loc(#loc83)
    %609 = arith.andi %602, %608 : i1 loc(#loc83)
    %610 = arith.xori %591, %true : i1
    %611 = arith.ori %610, %592 : i1
    %612 = arith.ori %591, %588 : i1
    %613 = arith.xori %588, %true : i1
    %614 = arith.ori %613, %594 : i1
    %615 = arith.andi %611, %614 : i1
    %616 = arith.andi %426, %583 : i1
    %617 = acir.extract %325 {acir.id = "v17", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc85)
    %618 = acir.extract %325 {acir.id = "v18", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc86)
    %619 = acir.extract %325 {acir.id = "v19", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc87)
    %620 = acir.extract %325 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc88)
    %621 = acir.extract %325 {acir.id = "v21", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc89)
    %622 = arith.andi %438, %438 : i1 loc(#loc90)
    %623 = arith.andi %622, %438 : i1 loc(#loc90)
    %624 = arith.andi %623, %438 : i1 loc(#loc90)
    %625 = arith.andi %624, %487 : i1 loc(#loc90)
    %626 = arith.andi %625, %547 : i1 loc(#loc90)
    %627 = arith.andi %626, %520 : i1 loc(#loc90)
    %628 = arith.andi %627, %438 : i1 loc(#loc90)
    %629 = acir.aggregate(%617, %618, %619, %620, %488, %548, %521, %621) {acir.id = "v22", acir.source_type = "src_isa_f0424bc8.Operands"} : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc90)
    %630 = arith.andi %550, %628 : i1 loc(#loc91)
    %631 = arith.andi %426, %630 : i1 loc(#loc91)
    %632 = arith.andi %426, %631 : i1 loc(#loc91)
    %633 = arith.xori %632, %true : i1 loc(#loc91)
    %634 = arith.ori %633, %431 : i1 loc(#loc91)
    %635 = arith.andi %609, %634 : i1 loc(#loc91)
    acir.path %632 {branch = 10 : i64, kind = "branch", txn = 1 : i64} loc(#loc91)
    %value_39, %ready_40 = acir.read(%632) {branch = 10 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.id_ex", txn = 1 : i64} : (i1) -> (i1, i1) loc(#loc91)
    %636 = arith.xori %632, %true : i1 loc(#loc91)
    %637 = arith.andi %431, %ready_40 : i1 loc(#loc91)
    %638 = arith.ori %636, %637 : i1 loc(#loc91)
    %639 = arith.andi %635, %638 : i1 loc(#loc91)
    %640 = arith.xori %value_39, %true : i1 loc(#loc91)
    %641 = arith.xori %632, %true : i1 loc(#loc91)
    %642 = arith.andi %431, %640 : i1 loc(#loc91)
    %643 = arith.ori %641, %642 : i1 loc(#loc91)
    %644 = arith.andi %639, %643 : i1 loc(#loc91)
    %645 = acir.fire "top.decode_stage.read_operands#1" %644 {successors = [0], txn = 1 : i64} loc(#loc91)
    %646 = arith.andi %645, %426 : i1 loc(#loc91)
    acir.effect %646() {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "pop", port = 0 : i64, resource = "top.if_id", txn = 1 : i64} : (i1) -> () loc(#loc69)
    %647 = arith.andi %645, %588 : i1 loc(#loc91)
    acir.effect %647(%605) {branch = 9 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "write", port = 0 : i64, resource = "top.rf.pending", txn = 1 : i64} : (i1, !acir.array<i1, 32>) -> () loc(#loc83)
    %648 = arith.andi %645, %632 : i1 loc(#loc91)
    acir.effect %648(%629) {branch = 10 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.id_ex", txn = 1 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Operands">) -> () loc(#loc91)
    %649 = arith.andi %427, %429 : i1 loc(#loc92)
    %650 = arith.andi %426, %431 : i1 loc(#loc92)
    %651 = arith.ori %649, %650 : i1 loc(#loc92)
    acir.path %true {branch = 11 : i64, kind = "branch", txn = 2 : i64} loc(#loc93)
    acir.path %true {branch = 11 : i64, frames = ["top.execute:execute"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc93)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc94)
    %value_41, %ready_42 = acir.read(%true) {branch = 11 : i64, frames = ["top.execute:execute"], method = "peek", port = 0 : i64, resource = "top.id_ex", txn = 2 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Operands">, i1) loc(#loc95)
    %652 = arith.andi %ready_42, %ready_42 : i1 loc(#loc95)
    %value_43, %ready_44 = acir.read(%true) {branch = 11 : i64, frames = ["top.execute:execute"], method = "empty", port = 0 : i64, resource = "top.id_ex", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc95)
    %653 = arith.andi %652, %ready_44 : i1 loc(#loc95)
    %654 = arith.xori %value_43, %true : i1 loc(#loc95)
    %655 = arith.andi %653, %654 : i1 loc(#loc95)
    %656 = acir.extract %value_41 {acir.id = "v5", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc96)
    %657 = arith.cmpi eq, %656, %c1_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc96)
    %658 = arith.andi %655, %ready_42 : i1
    %659 = arith.xori %657, %true : i1
    %660 = arith.xori %659, %true : i1
    %661 = arith.ori %660, %ready_42 : i1
    %662 = acir.extract %value_41 {acir.id = "v12", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc97)
    %663 = acir.extract %value_41 {acir.id = "v13", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc98)
    %664 = arith.andi %ready_42, %ready_42 : i1 loc(#loc97)
    %665 = arith.addi %662, %663 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc97)
    %666 = arith.xori %657, %true : i1
    %667 = arith.ori %666, %ready_42 : i1
    %668 = acir.extract %value_41 {acir.id = "v9", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc99)
    %669 = acir.extract %value_41 {acir.id = "v10", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc100)
    %670 = arith.andi %ready_42, %ready_42 : i1 loc(#loc99)
    %671 = arith.subi %668, %669 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc99)
    %672 = arith.xori %659, %true : i1
    %673 = arith.ori %672, %661 : i1
    %674 = arith.ori %659, %657 : i1
    %675 = arith.xori %657, %true : i1
    %676 = arith.ori %675, %667 : i1
    %677 = arith.andi %673, %676 : i1
    %678 = arith.andi %661, %664 : i1
    %679 = arith.andi %659, %678 : i1
    %680 = arith.andi %667, %670 : i1
    %681 = arith.andi %657, %680 : i1
    %682 = arith.ori %679, %681 : i1
    %683 = arith.select %659, %665, %671 : i32
    %684 = acir.extract %value_41 {acir.id = "v18", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc101)
    %685 = acir.extract %value_41 {acir.id = "v19", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc102)
    %686 = acir.extract %value_41 {acir.id = "v20", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc103)
    %687 = acir.extract %value_41 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc104)
    %688 = acir.extract %value_41 {acir.id = "v22", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc105)
    %689 = acir.extract %value_41 {acir.id = "v23", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc106)
    %690 = arith.andi %ready_42, %ready_42 : i1 loc(#loc107)
    %691 = arith.andi %690, %ready_42 : i1 loc(#loc107)
    %692 = arith.andi %691, %ready_42 : i1 loc(#loc107)
    %693 = arith.andi %692, %682 : i1 loc(#loc107)
    %694 = arith.andi %693, %ready_42 : i1 loc(#loc107)
    %695 = arith.andi %694, %ready_42 : i1 loc(#loc107)
    %696 = acir.aggregate(%684, %685, %686, %687, %683, %688, %689) {acir.id = "v24", acir.source_type = "src_isa_f0424bc8.Executed"} : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc107)
    %697 = arith.xori %695, %true : i1 loc(#loc108)
    acir.path %695 {branch = 12 : i64, kind = "branch", txn = 2 : i64} loc(#loc108)
    %value_45, %ready_46 = acir.read(%695) {branch = 12 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.ex_mem", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc108)
    %698 = arith.xori %695, %true : i1 loc(#loc108)
    %699 = arith.ori %698, %ready_46 : i1 loc(#loc108)
    %700 = arith.andi %658, %699 : i1 loc(#loc108)
    %701 = arith.xori %value_45, %true : i1 loc(#loc108)
    %702 = arith.xori %695, %true : i1 loc(#loc108)
    %703 = arith.ori %702, %701 : i1 loc(#loc108)
    %704 = arith.andi %700, %703 : i1 loc(#loc108)
    %705 = acir.fire "top.execute.execute#2" %704 {successors = [1], txn = 2 : i64} loc(#loc108)
    acir.effect %705() {branch = 11 : i64, frames = ["top.execute:execute"], method = "pop", port = 0 : i64, resource = "top.id_ex", txn = 2 : i64} : (i1) -> () loc(#loc95)
    %706 = arith.andi %705, %695 : i1 loc(#loc108)
    acir.effect %706(%696) {branch = 12 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.ex_mem", txn = 2 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Executed">) -> () loc(#loc108)
    acir.path %true {branch = 13 : i64, kind = "branch", txn = 3 : i64} loc(#loc109)
    acir.path %true {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc109)
    %c3_i32_47 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c2_i32_48 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc110)
    %value_49, %ready_50 = acir.read(%true) {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "peek", port = 0 : i64, resource = "top.ex_mem", txn = 3 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Executed">, i1) loc(#loc111)
    %707 = arith.andi %ready_50, %ready_50 : i1 loc(#loc111)
    %value_51, %ready_52 = acir.read(%true) {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "empty", port = 0 : i64, resource = "top.ex_mem", txn = 3 : i64} : (i1) -> (i1, i1) loc(#loc111)
    %708 = arith.andi %707, %ready_52 : i1 loc(#loc111)
    %709 = arith.xori %value_51, %true : i1 loc(#loc111)
    %710 = arith.andi %708, %709 : i1 loc(#loc111)
    %711 = acir.extract %value_49 {acir.id = "v5", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc112)
    %712 = acir.extract %value_49 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc113)
    %713 = arith.cmpi eq, %712, %c2_i32_48 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc113)
    %714 = arith.andi %710, %ready_50 : i1
    %715 = arith.xori %713, %true : i1
    %716 = arith.xori %715, %true : i1
    %717 = arith.ori %716, %ready_50 : i1
    %718 = acir.extract %value_49 {acir.id = "v12", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc114)
    %719 = arith.cmpi eq, %718, %c3_i32_47 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc114)
    %720 = arith.xori %715, %true : i1
    %721 = arith.andi %717, %ready_50 : i1
    %722 = arith.ori %720, %721 : i1
    %723 = arith.andi %714, %722 : i1
    %724 = arith.andi %715, %719 : i1
    %725 = arith.andi %717, %ready_50 : i1
    %726 = arith.xori %719, %true : i1
    %727 = arith.andi %715, %726 : i1
    %728 = arith.andi %717, %ready_50 : i1
    %729 = arith.xori %724, %true : i1
    %730 = arith.ori %729, %725 : i1
    %731 = acir.extract %value_49 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc115)
    %732 = acir.extract %value_49 {acir.id = "v17", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc116)
    %733 = arith.andi %ready_50, %ready_50 : i1 loc(#loc117)
    acir.path %724 {branch = 14 : i64, kind = "branch", txn = 3 : i64} loc(#loc117)
    acir.path %724 {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc117)
    %734 = arith.xori %724, %true : i1 loc(#loc117)
    %735 = arith.ori %734, %730 : i1 loc(#loc117)
    %736 = arith.andi %730, %ready_50 : i1 loc(#loc117)
    %737 = arith.andi %724, %736 : i1 loc(#loc117)
    %738 = arith.andi %730, %ready_50 : i1 loc(#loc117)
    %739 = arith.andi %724, %738 : i1 loc(#loc117)
    %c2_i32_53 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false_54 = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32_55 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc118)
    %c3_i32_56 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc119)
    %740 = arith.andi %731, %c3_i32_56 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc120)
    %741 = arith.cmpi eq, %740, %c0_i32_55 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc120)
    %742 = arith.xori %724, %true : i1
    %743 = arith.andi %735, %737 : i1
    %744 = arith.ori %742, %743 : i1
    %745 = arith.andi %723, %744 : i1
    %746 = arith.andi %724, %741 : i1
    %747 = arith.andi %735, %737 : i1
    %748 = arith.xori %741, %true : i1
    %749 = arith.andi %724, %748 : i1
    %750 = arith.andi %735, %737 : i1
    %751 = arith.xori %749, %true : i1
    %752 = arith.ori %751, %750 : i1
    %753 = arith.xori %746, %true : i1
    %754 = arith.ori %753, %747 : i1
    %755 = arith.cmpi ult, %731, %c64_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc121)
    %756 = arith.xori %749, %true : i1
    %757 = arith.ori %756, %752 : i1
    %758 = arith.ori %749, %746 : i1
    %759 = arith.xori %746, %true : i1
    %760 = arith.ori %759, %754 : i1
    %761 = arith.andi %757, %760 : i1
    %762 = arith.andi %749, %752 : i1
    %763 = arith.andi %754, %737 : i1
    %764 = arith.andi %746, %763 : i1
    %765 = arith.ori %762, %764 : i1
    %766 = arith.select %749, %false_54, %755 : i1
    %767 = arith.xori %724, %true : i1
    %768 = arith.ori %767, %735 : i1
    %769 = arith.xori %724, %true : i1 loc(#loc122)
    %770 = arith.andi %768, %765 : i1 loc(#loc122)
    %771 = arith.ori %769, %770 : i1 loc(#loc122)
    %772 = arith.andi %745, %771 : i1 loc(#loc122)
    %773 = arith.shrui %731, %c2_i32_53 : i32 loc(#loc123)
    %value_57, %ready_58 = acir.read(%724) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "read", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1) -> (!acir.array<i32, 16>, i1) loc(#loc124)
    %774 = arith.xori %724, %true : i1 loc(#loc124)
    %775 = arith.andi %768, %ready_58 : i1 loc(#loc124)
    %776 = arith.ori %774, %775 : i1 loc(#loc124)
    %777 = arith.andi %772, %776 : i1 loc(#loc124)
    %value_59, %ready_60 = acir.read(%724) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "read", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1) -> (!acir.array<i32, 16>, i1) loc(#loc124)
    %778 = arith.xori %724, %true : i1 loc(#loc124)
    %779 = arith.andi %768, %ready_60 : i1 loc(#loc124)
    %780 = arith.ori %778, %779 : i1 loc(#loc124)
    %781 = arith.andi %777, %780 : i1 loc(#loc124)
    %782 = arith.shrui %731, %c2_i32_53 : i32 loc(#loc123)
    %783 = arith.andi %ready_60, %739 : i1 loc(#loc124)
    %784 = arith.andi %783, %737 : i1 loc(#loc124)
    %785 = acir.update %value_59, %732[%782] {acir.id = "v28", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16> loc(#loc124)
    %786 = arith.xori %724, %true : i1 loc(#loc124)
    %787 = arith.andi %768, %784 : i1 loc(#loc124)
    %788 = arith.ori %786, %787 : i1 loc(#loc124)
    %789 = arith.andi %781, %788 : i1 loc(#loc124)
    %790 = arith.andi %724, %768 : i1
    %791 = arith.xori %727, %true : i1
    %792 = arith.ori %791, %728 : i1
    %793 = arith.ori %727, %724 : i1
    %794 = arith.xori %724, %true : i1
    %795 = arith.ori %794, %730 : i1
    %796 = arith.andi %792, %795 : i1
    %797 = arith.xori %713, %true : i1
    %798 = arith.ori %797, %ready_50 : i1
    %799 = acir.extract %value_49 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc125)
    acir.path %713 {branch = 15 : i64, kind = "branch", txn = 3 : i64} loc(#loc126)
    acir.path %713 {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc126)
    %800 = arith.xori %713, %true : i1 loc(#loc126)
    %801 = arith.ori %800, %798 : i1 loc(#loc126)
    %802 = arith.andi %798, %ready_50 : i1 loc(#loc126)
    %803 = arith.andi %713, %802 : i1 loc(#loc126)
    %c2_i32_61 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc127)
    %false_62 = arith.constant {acir.source_type = "bool"} false
    %c64_i32_63 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32_64 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc128)
    %c3_i32_65 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc129)
    %804 = arith.andi %799, %c3_i32_65 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc130)
    %805 = arith.cmpi eq, %804, %c0_i32_64 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc130)
    %806 = arith.xori %713, %true : i1
    %807 = arith.andi %801, %803 : i1
    %808 = arith.ori %806, %807 : i1
    %809 = arith.andi %789, %808 : i1
    %810 = arith.andi %713, %805 : i1
    %811 = arith.andi %801, %803 : i1
    %812 = arith.xori %805, %true : i1
    %813 = arith.andi %713, %812 : i1
    %814 = arith.andi %801, %803 : i1
    %815 = arith.xori %813, %true : i1
    %816 = arith.ori %815, %814 : i1
    %817 = arith.xori %810, %true : i1
    %818 = arith.ori %817, %811 : i1
    %819 = arith.cmpi ult, %799, %c64_i32_63 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc131)
    %820 = arith.xori %813, %true : i1
    %821 = arith.ori %820, %816 : i1
    %822 = arith.ori %813, %810 : i1
    %823 = arith.xori %810, %true : i1
    %824 = arith.ori %823, %818 : i1
    %825 = arith.andi %821, %824 : i1
    %826 = arith.andi %813, %816 : i1
    %827 = arith.andi %818, %803 : i1
    %828 = arith.andi %810, %827 : i1
    %829 = arith.ori %826, %828 : i1
    %830 = arith.select %813, %false_62, %819 : i1
    %831 = arith.xori %713, %true : i1
    %832 = arith.ori %831, %801 : i1
    %833 = arith.xori %713, %true : i1 loc(#loc132)
    %834 = arith.andi %832, %829 : i1 loc(#loc132)
    %835 = arith.ori %833, %834 : i1 loc(#loc132)
    %836 = arith.andi %809, %835 : i1 loc(#loc132)
    %837 = arith.shrui %799, %c2_i32_61 : i32 loc(#loc133)
    %value_66, %ready_67 = acir.read(%713) {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "read", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1) -> (!acir.array<i32, 16>, i1) loc(#loc134)
    %838 = arith.xori %713, %true : i1 loc(#loc134)
    %839 = arith.andi %832, %ready_67 : i1 loc(#loc134)
    %840 = arith.ori %838, %839 : i1 loc(#loc134)
    %841 = arith.andi %836, %840 : i1 loc(#loc134)
    %842 = arith.andi %ready_67, %803 : i1 loc(#loc134)
    %843 = acir.extract %value_66[%837] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32 loc(#loc134)
    %844 = arith.andi %832, %842 : i1 loc(#loc135)
    %845 = arith.andi %713, %844 : i1 loc(#loc135)
    %846 = arith.xori %715, %true : i1
    %847 = arith.ori %846, %717 : i1
    %848 = arith.ori %715, %713 : i1
    %849 = arith.xori %713, %true : i1
    %850 = arith.ori %849, %798 : i1
    %851 = arith.andi %847, %850 : i1
    %852 = arith.andi %717, %ready_50 : i1
    %853 = arith.andi %715, %852 : i1
    %854 = arith.andi %798, %845 : i1
    %855 = arith.andi %713, %854 : i1
    %856 = arith.ori %853, %855 : i1
    %857 = arith.select %715, %711, %843 : i32
    %858 = acir.extract %value_49 {acir.id = "v29", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc136)
    %859 = acir.extract %value_49 {acir.id = "v30", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc137)
    %860 = acir.extract %value_49 {acir.id = "v31", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc138)
    %861 = acir.extract %value_49 {acir.id = "v32", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc139)
    %862 = arith.andi %ready_50, %ready_50 : i1 loc(#loc140)
    %863 = arith.andi %862, %ready_50 : i1 loc(#loc140)
    %864 = arith.andi %863, %856 : i1 loc(#loc140)
    %865 = arith.andi %864, %ready_50 : i1 loc(#loc140)
    %866 = acir.aggregate(%858, %859, %860, %857, %861) {acir.id = "v33", acir.source_type = "src_isa_f0424bc8.Retired"} : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc140)
    %867 = arith.xori %865, %true : i1 loc(#loc141)
    acir.path %865 {branch = 16 : i64, kind = "branch", txn = 3 : i64} loc(#loc141)
    %value_68, %ready_69 = acir.read(%865) {branch = 16 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.mem_wb", txn = 3 : i64} : (i1) -> (i1, i1) loc(#loc141)
    %868 = arith.xori %865, %true : i1 loc(#loc141)
    %869 = arith.ori %868, %ready_69 : i1 loc(#loc141)
    %870 = arith.andi %841, %869 : i1 loc(#loc141)
    %871 = arith.xori %value_68, %true : i1 loc(#loc141)
    %872 = arith.xori %865, %true : i1 loc(#loc141)
    %873 = arith.ori %872, %871 : i1 loc(#loc141)
    %874 = arith.andi %870, %873 : i1 loc(#loc141)
    %875 = acir.fire "top.memory_stage.access#3" %874 {successors = [2], txn = 3 : i64} loc(#loc141)
    acir.effect %875() {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "pop", port = 0 : i64, resource = "top.ex_mem", txn = 3 : i64} : (i1) -> () loc(#loc111)
    %876 = arith.andi %875, %724 : i1 loc(#loc141)
    acir.effect %876(%766) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc122)
    %877 = arith.andi %875, %724 : i1 loc(#loc141)
    acir.effect %877(%785) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "write", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1, !acir.array<i32, 16>) -> () loc(#loc124)
    %878 = arith.andi %875, %713 : i1 loc(#loc141)
    acir.effect %878(%830) {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc132)
    %879 = arith.andi %875, %865 : i1 loc(#loc141)
    acir.effect %879(%866) {branch = 16 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.mem_wb", txn = 3 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Retired">) -> () loc(#loc141)
    acir.path %true {branch = 17 : i64, kind = "branch", txn = 4 : i64} loc(#loc142)
    acir.path %true {branch = 17 : i64, frames = ["top.writeback:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc142)
    %c1_i32_70 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc143)
    %value_71, %ready_72 = acir.read(%true) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "peek", port = 0 : i64, resource = "top.mem_wb", txn = 4 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Retired">, i1) loc(#loc144)
    %880 = arith.andi %ready_72, %ready_72 : i1 loc(#loc144)
    %value_73, %ready_74 = acir.read(%true) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "empty", port = 0 : i64, resource = "top.mem_wb", txn = 4 : i64} : (i1) -> (i1, i1) loc(#loc144)
    %881 = arith.andi %880, %ready_74 : i1 loc(#loc144)
    %882 = arith.xori %value_73, %true : i1 loc(#loc144)
    %883 = arith.andi %881, %882 : i1 loc(#loc144)
    acir.path %true {branch = 17 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc145)
    %false_75 = arith.constant {acir.source_type = "bool"} false
    %c0_i32_76 = arith.constant {acir.source_type = "u32"} 0 : i32
    %884 = acir.extract %value_71 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc146)
    %885 = arith.andi %883, %ready_72 : i1
    %886 = arith.xori %884, %true : i1
    %887 = arith.xori %886, %true : i1
    %888 = arith.ori %887, %ready_72 : i1
    %889 = arith.xori %884, %true : i1
    %890 = arith.ori %889, %ready_72 : i1
    %891 = acir.extract %value_71 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc147)
    %892 = arith.cmpi ne, %891, %c0_i32_76 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc147)
    %893 = arith.xori %886, %true : i1
    %894 = arith.ori %893, %888 : i1
    %895 = arith.ori %886, %884 : i1
    %896 = arith.xori %884, %true : i1
    %897 = arith.ori %896, %890 : i1
    %898 = arith.andi %894, %897 : i1
    %899 = arith.andi %886, %888 : i1
    %900 = arith.andi %890, %ready_72 : i1
    %901 = arith.andi %884, %900 : i1
    %902 = arith.ori %899, %901 : i1
    %903 = arith.select %886, %false_75, %892 : i1
    %904 = arith.andi %885, %902 : i1
    %905 = arith.xori %903, %true : i1
    %906 = arith.xori %903, %true : i1
    %907 = arith.ori %906, %902 : i1
    %908 = acir.extract %value_71 {acir.id = "v15", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc148)
    acir.path %903 {branch = 18 : i64, kind = "branch", txn = 4 : i64} loc(#loc149)
    %value_77, %ready_78 = acir.read(%903) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 4 : i64} : (i1) -> (!acir.array<i32, 32>, i1) loc(#loc149)
    %909 = arith.xori %903, %true : i1 loc(#loc149)
    %910 = arith.andi %907, %ready_78 : i1 loc(#loc149)
    %911 = arith.ori %909, %910 : i1 loc(#loc149)
    %912 = arith.andi %904, %911 : i1 loc(#loc149)
    %value_79, %ready_80 = acir.read(%903) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 4 : i64} : (i1) -> (!acir.array<i32, 32>, i1) loc(#loc149)
    %913 = arith.xori %903, %true : i1 loc(#loc149)
    %914 = arith.andi %907, %ready_80 : i1 loc(#loc149)
    %915 = arith.ori %913, %914 : i1 loc(#loc149)
    %916 = arith.andi %912, %915 : i1 loc(#loc149)
    %917 = acir.extract %value_71 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc150)
    %918 = arith.andi %ready_80, %ready_72 : i1 loc(#loc149)
    %919 = arith.andi %918, %ready_72 : i1 loc(#loc149)
    %920 = acir.update %value_79, %908[%917] {acir.id = "v24", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32> loc(#loc149)
    %921 = arith.xori %903, %true : i1 loc(#loc149)
    %922 = arith.andi %907, %919 : i1 loc(#loc149)
    %923 = arith.ori %921, %922 : i1 loc(#loc149)
    %924 = arith.andi %916, %923 : i1 loc(#loc149)
    %value_81, %ready_82 = acir.read(%903) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = 4 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc151)
    %925 = arith.xori %903, %true : i1 loc(#loc151)
    %926 = arith.andi %907, %ready_82 : i1 loc(#loc151)
    %927 = arith.ori %925, %926 : i1 loc(#loc151)
    %928 = arith.andi %924, %927 : i1 loc(#loc151)
    %value_83, %ready_84 = acir.read(%903) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = 4 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc151)
    %929 = arith.xori %903, %true : i1 loc(#loc151)
    %930 = arith.andi %907, %ready_84 : i1 loc(#loc151)
    %931 = arith.ori %929, %930 : i1 loc(#loc151)
    %932 = arith.andi %928, %931 : i1 loc(#loc151)
    %933 = acir.extract %value_71 {acir.id = "v36", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc152)
    %934 = arith.andi %ready_84, %ready_72 : i1 loc(#loc151)
    %935 = acir.update %value_83, %false_75[%933] {acir.id = "v37", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc151)
    %936 = arith.xori %903, %true : i1 loc(#loc151)
    %937 = arith.andi %907, %934 : i1 loc(#loc151)
    %938 = arith.ori %936, %937 : i1 loc(#loc151)
    %939 = arith.andi %932, %938 : i1 loc(#loc151)
    %940 = arith.xori %905, %true : i1
    %941 = arith.ori %940, %902 : i1
    %942 = arith.ori %905, %903 : i1
    %943 = arith.xori %903, %true : i1
    %944 = arith.ori %943, %907 : i1
    %945 = arith.andi %941, %944 : i1
    %value_85, %ready_86 = acir.read(%true) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "read", port = 0 : i64, resource = "top.writeback.retired", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc153)
    %946 = arith.andi %939, %ready_86 : i1 loc(#loc153)
    %947 = arith.addi %value_85, %c1_i32_70 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc153)
    %948 = arith.andi %946, %ready_86 : i1 loc(#loc154)
    %949 = acir.extract %value_71 {acir.id = "v12", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc155)
    %950 = arith.andi %948, %ready_72 : i1 loc(#loc156)
    %951 = acir.fire "top.writeback.commit#4" %950 {successors = [3], txn = 4 : i64}
    acir.effect %951() {branch = 17 : i64, frames = ["top.writeback:commit"], method = "pop", port = 0 : i64, resource = "top.mem_wb", txn = 4 : i64} : (i1) -> () loc(#loc144)
    %952 = arith.andi %951, %903 : i1
    acir.effect %952(%920) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.registers", txn = 4 : i64} : (i1, !acir.array<i32, 32>) -> () loc(#loc149)
    %953 = arith.andi %951, %903 : i1
    acir.effect %953(%935) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.pending", txn = 4 : i64} : (i1, !acir.array<i1, 32>) -> () loc(#loc151)
    acir.effect %951(%947) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "write", port = 0 : i64, resource = "top.writeback.retired", txn = 4 : i64} : (i1, i32) -> () loc(#loc154)
    acir.effect %951(%949) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "write", port = 0 : i64, resource = "top.writeback.last_pc", txn = 4 : i64} : (i1, i32) -> () loc(#loc156)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":68:25)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":67:22)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":68:12)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:20)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:27)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:34)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:20)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:16)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:14)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:9)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:16)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:9)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:13)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":87:23)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:18)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:19)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:22)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:21)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":53:12)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:14)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:14)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":56:14)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:10)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:11)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":59:11)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:8)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":69:10)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:10)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:16)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:33)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:9)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:28)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:46)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:21)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:13)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:12)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:5)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:16)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:9)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:28)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:21)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:16)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:33)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:48)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:9)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":71:12)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:16)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:9)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:28)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:21)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:20)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:12)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:5)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:19)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:42)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:28)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:20)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:16)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:55)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:47)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:43)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:75)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:71)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:102)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:94)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:90)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:9)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:20)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":79:20)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":80:23)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":80:15)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:12)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:16)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:9)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":23:13)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:31)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:23)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:36)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:15)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:9)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:12)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:28)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:13)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:21)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:25)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:37)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:51)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:65)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:100)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:16)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:9)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:13)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:16)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:54)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":100:16)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:41)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:63)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:74)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:18)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:29)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:25)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:34)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:45)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:56)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:73)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:91)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:16)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:9)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:16)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":117:25)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":115:16)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":116:18)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":117:12)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":119:14)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:26)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:38)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:13)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:31)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:26)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:16)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:37)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:9)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:15)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:9)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":118:34)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":118:22)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:33)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:31)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:26)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:16)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:37)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:9)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:22)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:16)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:9)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:24)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:33)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:44)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:61)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:16)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:9)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:9)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:29)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":135:16)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":136:9)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:12)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:28)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:34)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:13)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:23)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:13)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:21)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:19)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:9)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:19)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:9)

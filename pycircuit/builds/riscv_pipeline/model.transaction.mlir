module attributes {acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, exports = {}, format_version = 8 : i64, top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}, {default = 0 : i64, name = "immediate", type = "u32"}, {default = false, name = "immediate_rhs", type = "bool"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "lhs", type = "u32"}, {default = 0 : i64, name = "rhs", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = false, name = "writes", type = "bool"}]}}, acir.stage = "transaction"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.decode_stage.out", source_branch = 11 : i64, target_branches = [10], to = 1 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.decode_stage.out", source_branch = 11 : i64, target_branches = [10], to = 1 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.execute.out", source_branch = 13 : i64, target_branches = [12], to = 2 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.execute.out", source_branch = 13 : i64, target_branches = [12], to = 2 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.fetch.out", source_branch = 6 : i64, target_branches = [2], to = 0 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.fetch.out", source_branch = 6 : i64, target_branches = [2], to = 0 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.retirement", source_branch = 17 : i64, target_branches = [16], to = 3 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.retirement", source_branch = 17 : i64, target_branches = [16], to = 3 : i64}], schedule = [4, 3, 2, 1, 0], stage = "transaction"} {
    %true = arith.constant true
    %false = arith.constant false
    acir.state @top.decode_stage.out : !acir.struct<"src_isa_f0424bc8.Operands"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.execute.out : !acir.struct<"src_isa_f0424bc8.Executed"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.fetch.out : !acir.struct<"src_isa_f0424bc8.Fetched"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.fetch.pc : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.fetch.rom : !acir.array<i32, 6> {initial = [7340179 : i32, 5243155 : i32, 2130355 : i32, 3153955 : i32, 8707 : i32, 1075970739 : i32], kind = "cell"}
    acir.state @top.memory.words : !acir.array<i32, 16> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.retirement : !acir.struct<"src_isa_f0424bc8.Retired"> {capacity = 1 : i64, initial = [], kind = "queue"}
    acir.state @top.rf.pending : !acir.array<i1, 32> {initial = [false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false], kind = "cell"}
    acir.state @top.rf.registers : !acir.array<i32, 32> {initial = [0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32, 0 : i32], kind = "cell"}
    acir.state @top.writeback.last_pc : i32 {initial = 0 : i32, kind = "cell"}
    acir.state @top.writeback.retired : i32 {initial = 0 : i32, kind = "cell"}
    acir.path %true {branch = 0 : i64, kind = "branch", txn = -1 : i64} loc(#loc1)
    %value, %ready = acir.read(%true) {branch = 0 : i64, frames = [], method = "peek", port = 0 : i64, resource = "top.fetch.out", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Fetched">, i1) loc(#loc1)
    %value_0, %ready_1 = acir.read(%true) {branch = 0 : i64, frames = [], method = "peek", port = 0 : i64, resource = "top.decode_stage.out", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Operands">, i1) loc(#loc2)
    %value_2, %ready_3 = acir.read(%true) {branch = 0 : i64, frames = [], method = "peek", port = 0 : i64, resource = "top.execute.out", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Executed">, i1) loc(#loc3)
    %value_4, %ready_5 = acir.read(%true) {branch = 0 : i64, frames = [], method = "peek", port = 0 : i64, resource = "top.retirement", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Retired">, i1) loc(#loc4)
    %c24_i32 = arith.constant {acir.source_type = "u32"} 24 : i32 loc(#loc5)
    %value_6, %ready_7 = acir.read(%true) {branch = 0 : i64, frames = [], method = "read", port = 0 : i64, resource = "top.fetch.pc", txn = -1 : i64} : (i1) -> (i32, i1) loc(#loc6)
    %0 = arith.cmpi ult, %value_6, %c24_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc7)
    %1 = arith.xori %0, %true : i1
    %2 = arith.xori %0, %true : i1
    %3 = arith.ori %2, %ready_7 : i1
    %4 = arith.andi %3, %ready_7 : i1 loc(#loc8)
    %5 = arith.andi %0, %4 : i1 loc(#loc8)
    acir.path %0 {branch = 1 : i64, kind = "branch", txn = 0 : i64} loc(#loc8)
    acir.path %0 {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 0 : i64} loc(#loc8)
    %6 = arith.xori %0, %true : i1 loc(#loc8)
    %7 = arith.ori %6, %3 : i1 loc(#loc8)
    %8 = arith.andi %3, %ready_7 : i1 loc(#loc8)
    %9 = arith.andi %0, %8 : i1 loc(#loc8)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc9)
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc10)
    %10 = arith.shrui %value_6, %c2_i32 : i32 loc(#loc11)
    %value_8, %ready_9 = acir.read(%0) {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "read", port = 0 : i64, resource = "top.fetch.rom", txn = 0 : i64} : (i1) -> (!acir.array<i32, 6>, i1) loc(#loc12)
    %11 = arith.xori %0, %true : i1 loc(#loc12)
    %12 = arith.andi %7, %ready_9 : i1 loc(#loc12)
    %13 = arith.ori %11, %12 : i1 loc(#loc12)
    %14 = arith.andi %5, %13 : i1 loc(#loc12)
    %15 = arith.andi %ready_9, %9 : i1 loc(#loc12)
    %16 = acir.extract %value_8[%10] {acir.id = "v8", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 6>, i32) -> i32 loc(#loc12)
    %17 = arith.addi %value_6, %c4_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc13)
    %18 = arith.xori %0, %true : i1 loc(#loc14)
    %19 = arith.andi %7, %9 : i1 loc(#loc14)
    %20 = arith.ori %18, %19 : i1 loc(#loc14)
    %21 = arith.andi %14, %20 : i1 loc(#loc14)
    %22 = arith.andi %9, %15 : i1 loc(#loc15)
    %23 = acir.aggregate(%value_6, %16) {acir.id = "v13", acir.source_type = "src_isa_f0424bc8.Fetched"} : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc15)
    %24 = arith.andi %7, %22 : i1 loc(#loc16)
    %25 = arith.andi %0, %24 : i1 loc(#loc16)
    %26 = arith.andi %0, %25 : i1 loc(#loc16)
    %27 = arith.xori %26, %true : i1 loc(#loc16)
    %28 = arith.ori %27, %3 : i1 loc(#loc16)
    %29 = arith.andi %21, %28 : i1 loc(#loc16)
    acir.path %26 {branch = 2 : i64, kind = "branch", txn = 0 : i64} loc(#loc16)
    %value_10, %ready_11 = acir.read(%26) {branch = 2 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.fetch.out", txn = 0 : i64} : (i1) -> (i1, i1) loc(#loc16)
    %30 = arith.xori %26, %true : i1 loc(#loc16)
    %31 = arith.andi %3, %ready_11 : i1 loc(#loc16)
    %32 = arith.ori %30, %31 : i1 loc(#loc16)
    %33 = arith.andi %29, %32 : i1 loc(#loc16)
    %34 = arith.xori %value_10, %true : i1 loc(#loc16)
    %35 = arith.xori %26, %true : i1 loc(#loc16)
    %36 = arith.andi %3, %34 : i1 loc(#loc16)
    %37 = arith.ori %35, %36 : i1 loc(#loc16)
    %38 = arith.andi %33, %37 : i1 loc(#loc16)
    %39 = acir.fire "top.fetch.fetch#0" %38 {successors = [], txn = 0 : i64} loc(#loc16)
    %40 = arith.andi %39, %0 : i1 loc(#loc16)
    acir.effect %40(%17) {branch = 1 : i64, frames = ["top.fetch:fetch"], method = "write", port = 0 : i64, resource = "top.fetch.pc", txn = 0 : i64} : (i1, i32) -> () loc(#loc14)
    %41 = arith.andi %39, %26 : i1 loc(#loc16)
    acir.effect %41(%23) {branch = 2 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.fetch.out", txn = 0 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Fetched">) -> () loc(#loc16)
    %42 = arith.xori %1, %true : i1
    %43 = arith.ori %42, %ready_7 : i1
    %44 = arith.ori %1, %0 : i1
    %45 = arith.xori %0, %true : i1
    %46 = arith.ori %45, %3 : i1
    %47 = arith.andi %43, %46 : i1
    %false_12 = arith.constant {acir.source_type = "bool"} false
    %value_13, %ready_14 = acir.read(%true) {branch = 0 : i64, frames = [], method = "peek", port = 0 : i64, resource = "top.fetch.out", txn = -1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Fetched">, i1) loc(#loc17)
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %c35_i32 = arith.constant {acir.source_type = "u32"} 35 : i32
    %c2_i32_15 = arith.constant {acir.source_type = "u32"} 2 : i32
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c32_i32 = arith.constant {acir.source_type = "u32"} 32 : i32
    %c51_i32 = arith.constant {acir.source_type = "u32"} 51 : i32
    %c19_i32 = arith.constant {acir.source_type = "u32"} 19 : i32 loc(#loc18)
    %true_16 = arith.constant {acir.source_type = "bool"} true
    %false_17 = arith.constant {acir.source_type = "bool"} false
    %c0_i32 = arith.constant {acir.source_type = "u32"} 0 : i32
    %c20_i32 = arith.constant {acir.source_type = "u32"} 20 : i32
    %c15_i32 = arith.constant {acir.source_type = "u32"} 15 : i32 loc(#loc19)
    %c31_i32 = arith.constant {acir.source_type = "u32"} 31 : i32
    %c25_i32 = arith.constant {acir.source_type = "u32"} 25 : i32
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    %c12_i32 = arith.constant {acir.source_type = "u32"} 12 : i32 loc(#loc20)
    %c127_i32 = arith.constant {acir.source_type = "u32"} 127 : i32 loc(#loc21)
    %48 = acir.extract %value_13 {acir.id = "v2", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc22)
    %49 = arith.andi %48, %c127_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc23)
    %50 = arith.shrui %48, %c12_i32 : i32 loc(#loc24)
    %51 = arith.andi %50, %c7_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc24)
    %52 = arith.shrui %48, %c25_i32 : i32 loc(#loc25)
    %53 = arith.shrui %48, %c7_i32 : i32 loc(#loc26)
    %54 = arith.andi %53, %c31_i32 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc26)
    %55 = arith.shrui %48, %c15_i32 : i32 loc(#loc27)
    %56 = arith.andi %55, %c31_i32 {acir.id = "v18", acir.source_type = "u32"} : i32 loc(#loc27)
    %57 = arith.shrui %48, %c20_i32 : i32 loc(#loc28)
    %58 = arith.andi %57, %c31_i32 {acir.id = "v22", acir.source_type = "u32"} : i32 loc(#loc28)
    %59 = arith.cmpi eq, %49, %c19_i32 {acir.id = "v28", acir.source_type = "bool"} : i32 loc(#loc29)
    %60 = arith.xori %59, %true : i1
    %61 = arith.xori %60, %true : i1
    %62 = arith.ori %61, %ready_14 : i1
    %63 = arith.cmpi eq, %49, %c51_i32 {acir.id = "v39", acir.source_type = "bool"} : i32 loc(#loc30)
    %64 = arith.cmpi ne, %49, %c51_i32 : i32 loc(#loc30)
    %65 = arith.andi %60, %63 : i1
    %66 = arith.andi %62, %ready_14 : i1
    %67 = arith.xori %63, %true : i1
    %68 = arith.andi %60, %67 : i1
    %69 = arith.andi %62, %ready_14 : i1
    %70 = arith.xori %68, %true : i1
    %71 = arith.ori %70, %69 : i1
    %72 = arith.cmpi eq, %49, %c3_i32 {acir.id = "v77", acir.source_type = "bool"} : i32 loc(#loc31)
    %73 = arith.select %72, %c2_i32_15, %c3_i32 : i32
    %74 = arith.andi %ready_14, %ready_14 : i1
    %75 = arith.select %72, %54, %c0_i32 : i32
    %76 = arith.andi %ready_14, %ready_14 : i1
    %77 = arith.select %72, %c0_i32, %58 : i32
    %78 = arith.andi %68, %72 : i1
    %79 = arith.andi %71, %ready_14 : i1
    %80 = arith.xori %72, %true : i1
    %81 = arith.andi %68, %80 : i1
    %82 = arith.andi %71, %ready_14 : i1
    %83 = arith.xori %81, %true : i1
    %84 = arith.ori %83, %82 : i1
    %85 = arith.cmpi eq, %49, %c35_i32 {acir.id = "v89", acir.source_type = "bool"} : i32 loc(#loc32)
    %86 = arith.andi %81, %85 : i1
    %87 = arith.andi %84, %ready_14 : i1
    %88 = arith.xori %85, %true : i1
    %89 = arith.andi %81, %88 : i1
    %90 = arith.andi %84, %ready_14 : i1
    %91 = arith.xori %89, %true : i1
    %92 = arith.ori %91, %90 : i1
    %93 = arith.xori %86, %true : i1
    %94 = arith.ori %93, %87 : i1
    %95 = arith.cmpi eq, %51, %c2_i32_15 {acir.id = "v92", acir.source_type = "bool"} : i32 loc(#loc33)
    %96 = arith.xori %89, %true : i1
    %97 = arith.ori %96, %92 : i1
    %98 = arith.ori %89, %86 : i1
    %99 = arith.xori %86, %true : i1
    %100 = arith.ori %99, %94 : i1
    %101 = arith.andi %97, %100 : i1
    %102 = arith.andi %89, %92 : i1
    %103 = arith.andi %94, %ready_14 : i1
    %104 = arith.andi %86, %103 : i1
    %105 = arith.ori %102, %104 : i1
    %106 = arith.select %89, %false_17, %95 : i1
    %107 = arith.xori %81, %true : i1
    %108 = arith.ori %107, %84 : i1
    %109 = acir.assert %106 loc(#loc34)
    %110 = arith.xori %81, %true : i1 loc(#loc34)
    %111 = arith.andi %105, %109 : i1 loc(#loc34)
    %112 = arith.ori %110, %111 : i1 loc(#loc34)
    %113 = arith.shrui %48, %c25_i32 : i32 loc(#loc35)
    %114 = arith.shli %113, %c5_i32 : i32 loc(#loc35)
    %115 = arith.shrui %48, %c7_i32 : i32 loc(#loc36)
    %116 = arith.andi %115, %c31_i32 {acir.id = "v107", acir.source_type = "u32"} : i32 loc(#loc36)
    %117 = arith.andi %ready_14, %ready_14 : i1 loc(#loc35)
    %118 = arith.ori %114, %116 {acir.id = "v108", acir.source_type = "u32"} : i32 loc(#loc35)
    %119 = arith.xori %81, %true : i1 loc(#loc37)
    %120 = arith.ori %119, %108 : i1 loc(#loc37)
    %121 = arith.andi %108, %117 : i1 loc(#loc37)
    %122 = arith.andi %81, %121 : i1 loc(#loc37)
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %123 = arith.xori %118, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc38)
    %124 = arith.subi %123, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc39)
    %125 = arith.andi %120, %122 : i1 loc(#loc40)
    %126 = arith.andi %81, %125 : i1 loc(#loc40)
    %127 = arith.xori %78, %true : i1
    %128 = arith.ori %127, %79 : i1
    %129 = arith.cmpi eq, %51, %c2_i32_15 {acir.id = "v80", acir.source_type = "bool"} : i32 loc(#loc41)
    %130 = acir.assert %129 loc(#loc42)
    %131 = arith.xori %78, %true : i1 loc(#loc42)
    %132 = arith.andi %ready_14, %130 : i1 loc(#loc42)
    %133 = arith.ori %131, %132 : i1 loc(#loc42)
    %134 = arith.andi %112, %133 : i1 loc(#loc42)
    %135 = arith.shrui %48, %c20_i32 : i32 loc(#loc43)
    %136 = arith.xori %78, %true : i1 loc(#loc44)
    %137 = arith.ori %136, %128 : i1 loc(#loc44)
    %138 = arith.andi %128, %ready_14 : i1 loc(#loc44)
    %139 = arith.andi %78, %138 : i1 loc(#loc44)
    %c2048_i32_18 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %140 = arith.xori %135, %c2048_i32_18 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc38)
    %141 = arith.subi %140, %c2048_i32_18 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc39)
    %142 = arith.andi %137, %139 : i1 loc(#loc40)
    %143 = arith.andi %78, %142 : i1 loc(#loc40)
    %144 = arith.xori %81, %true : i1
    %145 = arith.ori %144, %108 : i1
    %146 = arith.ori %81, %78 : i1
    %147 = arith.xori %78, %true : i1
    %148 = arith.ori %147, %128 : i1
    %149 = arith.andi %145, %148 : i1
    %150 = arith.andi %108, %126 : i1
    %151 = arith.andi %81, %150 : i1
    %152 = arith.andi %128, %143 : i1
    %153 = arith.andi %78, %152 : i1
    %154 = arith.ori %151, %153 : i1
    %155 = arith.select %81, %124, %141 : i32
    %156 = arith.xori %68, %true : i1
    %157 = arith.ori %156, %71 : i1
    %158 = arith.andi %154, %ready_14 : i1
    %159 = arith.andi %158, %74 : i1
    %160 = arith.andi %159, %76 : i1
    %161 = arith.andi %160, %ready_14 : i1
    %162 = arith.xori %65, %true : i1
    %163 = arith.ori %162, %66 : i1
    %164 = arith.cmpi eq, %51, %c0_i32 {acir.id = "v42", acir.source_type = "bool"} : i32 loc(#loc45)
    %165 = arith.andi %65, %164 : i1
    %166 = arith.andi %163, %ready_14 : i1
    %167 = arith.xori %164, %true : i1
    %168 = arith.andi %65, %167 : i1
    %169 = arith.andi %163, %ready_14 : i1
    %170 = arith.xori %168, %true : i1
    %171 = arith.ori %170, %169 : i1
    %172 = arith.xori %165, %true : i1
    %173 = arith.ori %172, %166 : i1
    %174 = arith.cmpi eq, %52, %c0_i32 {acir.id = "v45", acir.source_type = "bool"} : i32 loc(#loc46)
    %175 = arith.andi %165, %174 : i1
    %176 = arith.andi %173, %ready_14 : i1
    %177 = arith.xori %174, %true : i1
    %178 = arith.andi %165, %177 : i1
    %179 = arith.andi %173, %ready_14 : i1
    %180 = arith.xori %178, %true : i1
    %181 = arith.ori %180, %179 : i1
    %182 = arith.cmpi eq, %52, %c32_i32 {acir.id = "v49", acir.source_type = "bool"} : i32 loc(#loc47)
    %183 = arith.xori %175, %true : i1
    %184 = arith.ori %183, %176 : i1
    %185 = arith.xori %178, %true : i1
    %186 = arith.ori %185, %181 : i1
    %187 = arith.ori %178, %175 : i1
    %188 = arith.xori %175, %true : i1
    %189 = arith.ori %188, %184 : i1
    %190 = arith.andi %186, %189 : i1
    %191 = arith.andi %181, %ready_14 : i1
    %192 = arith.andi %178, %191 : i1
    %193 = arith.andi %175, %184 : i1
    %194 = arith.ori %192, %193 : i1
    %195 = arith.select %178, %182, %true_16 : i1
    %196 = arith.xori %165, %true : i1
    %197 = arith.ori %196, %173 : i1
    %198 = arith.xori %168, %true : i1
    %199 = arith.ori %198, %171 : i1
    %200 = arith.ori %168, %165 : i1
    %201 = arith.xori %165, %true : i1
    %202 = arith.ori %201, %197 : i1
    %203 = arith.andi %199, %202 : i1
    %204 = arith.andi %168, %171 : i1
    %205 = arith.andi %197, %194 : i1
    %206 = arith.andi %165, %205 : i1
    %207 = arith.ori %204, %206 : i1
    %208 = arith.select %168, %false_17, %195 : i1
    %209 = arith.xori %65, %true : i1
    %210 = arith.ori %209, %163 : i1
    %211 = acir.assert %208 loc(#loc48)
    %212 = arith.xori %65, %true : i1 loc(#loc48)
    %213 = arith.andi %207, %211 : i1 loc(#loc48)
    %214 = arith.ori %212, %213 : i1 loc(#loc48)
    %215 = arith.andi %134, %214 : i1 loc(#loc48)
    %216 = arith.cmpi eq, %52, %c32_i32 {acir.id = "v59", acir.source_type = "bool"} : i32 loc(#loc49)
    %217 = arith.extui %216 : i1 to i32
    %218 = arith.andi %ready_14, %ready_14 : i1
    %219 = arith.andi %218, %ready_14 : i1
    %220 = arith.xori %68, %true : i1
    %221 = arith.ori %220, %157 : i1
    %222 = arith.ori %68, %65 : i1
    %223 = arith.xori %65, %true : i1
    %224 = arith.ori %223, %210 : i1
    %225 = arith.andi %221, %224 : i1
    %226 = arith.andi %157, %154 : i1
    %227 = arith.andi %68, %226 : i1
    %228 = arith.andi %65, %210 : i1
    %229 = arith.ori %227, %228 : i1
    %230 = arith.select %68, %155, %c0_i32 : i32
    %231 = arith.andi %157, %ready_14 : i1
    %232 = arith.andi %68, %231 : i1
    %233 = arith.andi %210, %ready_14 : i1
    %234 = arith.andi %65, %233 : i1
    %235 = arith.ori %232, %234 : i1
    %236 = arith.select %68, %73, %217 : i32
    %237 = arith.andi %157, %74 : i1
    %238 = arith.andi %68, %237 : i1
    %239 = arith.andi %210, %ready_14 : i1
    %240 = arith.andi %65, %239 : i1
    %241 = arith.ori %238, %240 : i1
    %242 = arith.select %68, %75, %54 : i32
    %243 = arith.andi %157, %76 : i1
    %244 = arith.andi %68, %243 : i1
    %245 = arith.andi %210, %ready_14 : i1
    %246 = arith.andi %65, %245 : i1
    %247 = arith.ori %244, %246 : i1
    %248 = arith.select %68, %77, %58 : i32
    %249 = arith.andi %157, %ready_14 : i1
    %250 = arith.andi %68, %249 : i1
    %251 = arith.andi %65, %210 : i1
    %252 = arith.ori %250, %251 : i1
    %253 = arith.select %68, %72, %true_16 : i1
    %254 = arith.xori %60, %true : i1
    %255 = arith.ori %254, %62 : i1
    %256 = arith.andi %229, %ready_14 : i1
    %257 = arith.andi %256, %235 : i1
    %258 = arith.andi %257, %241 : i1
    %259 = arith.andi %258, %247 : i1
    %260 = arith.andi %259, %252 : i1
    %261 = arith.xori %59, %true : i1
    %262 = arith.ori %261, %ready_14 : i1
    %263 = arith.cmpi eq, %51, %c0_i32 {acir.id = "v31", acir.source_type = "bool"} : i32 loc(#loc50)
    %264 = acir.assert %263 loc(#loc51)
    %265 = arith.xori %59, %true : i1 loc(#loc51)
    %266 = arith.andi %ready_14, %264 : i1 loc(#loc51)
    %267 = arith.ori %265, %266 : i1 loc(#loc51)
    %268 = arith.andi %215, %267 : i1 loc(#loc51)
    %269 = arith.shrui %48, %c20_i32 : i32 loc(#loc52)
    %270 = arith.xori %59, %true : i1 loc(#loc53)
    %271 = arith.ori %270, %262 : i1 loc(#loc53)
    %272 = arith.andi %262, %ready_14 : i1 loc(#loc53)
    %273 = arith.andi %59, %272 : i1 loc(#loc53)
    %c2048_i32_19 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %274 = arith.xori %269, %c2048_i32_19 {acir.id = "v3", acir.source_type = "u32"} : i32 loc(#loc38)
    %275 = arith.subi %274, %c2048_i32_19 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc39)
    %276 = arith.andi %271, %273 : i1 loc(#loc40)
    %277 = arith.andi %59, %276 : i1 loc(#loc40)
    %278 = arith.andi %277, %ready_14 : i1
    %279 = arith.xori %60, %true : i1
    %280 = arith.ori %279, %255 : i1
    %281 = arith.ori %60, %59 : i1
    %282 = arith.xori %59, %true : i1
    %283 = arith.ori %282, %262 : i1
    %284 = arith.andi %280, %283 : i1
    %285 = arith.andi %255, %229 : i1
    %286 = arith.andi %60, %285 : i1
    %287 = arith.andi %262, %277 : i1
    %288 = arith.andi %59, %287 : i1
    %289 = arith.ori %286, %288 : i1
    %290 = arith.select %60, %230, %275 : i32
    %291 = arith.andi %255, %ready_14 : i1
    %292 = arith.andi %60, %291 : i1
    %293 = arith.andi %59, %262 : i1
    %294 = arith.ori %292, %293 : i1
    %295 = arith.select %60, %64, %true_16 : i1
    %296 = arith.andi %255, %235 : i1
    %297 = arith.andi %60, %296 : i1
    %298 = arith.andi %59, %262 : i1
    %299 = arith.ori %297, %298 : i1
    %300 = arith.select %60, %236, %c0_i32 : i32
    %301 = arith.andi %255, %241 : i1
    %302 = arith.andi %60, %301 : i1
    %303 = arith.andi %262, %ready_14 : i1
    %304 = arith.andi %59, %303 : i1
    %305 = arith.ori %302, %304 : i1
    %306 = arith.select %60, %242, %54 : i32
    %307 = arith.andi %255, %247 : i1
    %308 = arith.andi %60, %307 : i1
    %309 = arith.andi %59, %262 : i1
    %310 = arith.ori %308, %309 : i1
    %311 = arith.select %60, %248, %c0_i32 : i32
    %312 = arith.andi %255, %252 : i1
    %313 = arith.andi %60, %312 : i1
    %314 = arith.andi %59, %262 : i1
    %315 = arith.ori %313, %314 : i1
    %316 = arith.select %60, %253, %true_16 : i1
    %317 = acir.extract %value_13 {acir.id = "v154", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc54)
    %318 = arith.andi %ready_14, %ready_14 : i1 loc(#loc55)
    %319 = arith.andi %318, %299 : i1 loc(#loc55)
    %320 = arith.andi %319, %305 : i1 loc(#loc55)
    %321 = arith.andi %320, %ready_14 : i1 loc(#loc55)
    %322 = arith.andi %321, %310 : i1 loc(#loc55)
    %323 = arith.andi %322, %289 : i1 loc(#loc55)
    %324 = arith.andi %323, %294 : i1 loc(#loc55)
    %325 = arith.andi %324, %315 : i1 loc(#loc55)
    %326 = acir.aggregate(%317, %48, %300, %306, %56, %311, %290, %295, %316) {acir.id = "v155", acir.source_type = "src_isa_f0424bc8.Decoded"} : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded"> loc(#loc55)
    %327 = arith.andi %325, %268 : i1 loc(#loc56)
    %value_20, %ready_21 = acir.read(%true) {branch = 0 : i64, frames = [], method = "empty", port = 0 : i64, resource = "top.retirement", txn = -1 : i64} : (i1) -> (i1, i1) loc(#loc57)
    %328 = arith.xori %value_20, %true : i1
    %329 = arith.xori %328, %true : i1
    %330 = arith.ori %329, %ready_21 : i1
    %331 = arith.xori %value_20, %true : i1
    %332 = arith.ori %331, %ready_21 : i1
    acir.path %value_20 {branch = 3 : i64, kind = "branch", txn = -1 : i64} loc(#loc58)
    acir.path %value_20 {branch = 3 : i64, frames = ["top.rf:ready"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = -1 : i64} loc(#loc58)
    %333 = arith.xori %value_20, %true : i1 loc(#loc58)
    %334 = arith.ori %333, %332 : i1 loc(#loc58)
    %335 = arith.andi %332, %327 : i1 loc(#loc58)
    %336 = arith.andi %value_20, %335 : i1 loc(#loc58)
    %true_22 = arith.constant {acir.source_type = "bool"} true
    %false_23 = arith.constant {acir.source_type = "bool"} false
    %337 = acir.extract %326 {acir.id = "v5", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc59)
    %value_24, %ready_25 = acir.read(%value_20) {branch = 3 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = -1 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc60)
    %338 = arith.andi %ready_25, %336 : i1 loc(#loc60)
    %339 = acir.extract %value_24[%337] {acir.id = "v7", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc60)
    %340 = acir.unary %339 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1 loc(#loc61)
    %341 = arith.andi %value_20, %340 : i1
    %342 = arith.andi %334, %338 : i1
    %343 = arith.xori %340, %true : i1
    %344 = arith.andi %value_20, %343 : i1
    %345 = arith.andi %334, %338 : i1
    %346 = arith.xori %344, %true : i1
    %347 = arith.ori %346, %345 : i1
    %348 = arith.xori %341, %true : i1
    %349 = arith.ori %348, %342 : i1
    %350 = acir.extract %326 {acir.id = "v11", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc62)
    acir.path %341 {branch = 4 : i64, kind = "branch", txn = -1 : i64} loc(#loc63)
    %value_26, %ready_27 = acir.read(%341) {branch = 4 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = -1 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc63)
    %351 = arith.andi %ready_27, %336 : i1 loc(#loc63)
    %352 = acir.extract %value_26[%350] {acir.id = "v13", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc63)
    %353 = acir.unary %352 "not" {acir.id = "v14", acir.source_type = "bool"} : i1 -> i1 loc(#loc64)
    %354 = arith.xori %344, %true : i1
    %355 = arith.ori %354, %347 : i1
    %356 = arith.ori %344, %341 : i1
    %357 = arith.xori %341, %true : i1
    %358 = arith.ori %357, %349 : i1
    %359 = arith.andi %355, %358 : i1
    %360 = arith.andi %344, %347 : i1
    %361 = arith.andi %349, %351 : i1
    %362 = arith.andi %341, %361 : i1
    %363 = arith.ori %360, %362 : i1
    %364 = arith.select %344, %false_23, %353 : i1
    %365 = arith.xori %value_20, %true : i1
    %366 = arith.ori %365, %334 : i1
    %367 = arith.andi %value_20, %364 : i1
    %368 = arith.andi %366, %363 : i1
    %369 = arith.xori %364, %true : i1
    %370 = arith.andi %value_20, %369 : i1
    %371 = arith.andi %366, %363 : i1
    %372 = arith.xori %370, %true : i1
    %373 = arith.ori %372, %371 : i1
    %374 = arith.xori %367, %true : i1
    %375 = arith.ori %374, %368 : i1
    %376 = acir.extract %326 {acir.id = "v20", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc65)
    %377 = acir.unary %376 "not" {acir.id = "v21", acir.source_type = "bool"} : i1 -> i1 loc(#loc66)
    %378 = arith.andi %367, %377 : i1
    %379 = arith.andi %375, %336 : i1
    %380 = arith.xori %377, %true : i1
    %381 = arith.andi %367, %380 : i1
    %382 = arith.andi %375, %336 : i1
    %383 = arith.xori %381, %true : i1
    %384 = arith.ori %383, %382 : i1
    %385 = acir.extract %326 {acir.id = "v25", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc67)
    acir.path %381 {branch = 5 : i64, kind = "branch", txn = -1 : i64} loc(#loc68)
    %value_28, %ready_29 = acir.read(%381) {branch = 5 : i64, frames = ["top.rf:ready"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = -1 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc68)
    %386 = arith.andi %ready_29, %336 : i1 loc(#loc68)
    %387 = acir.extract %value_28[%385] {acir.id = "v27", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc68)
    %388 = acir.unary %387 "not" {acir.id = "v28", acir.source_type = "bool"} : i1 -> i1 loc(#loc69)
    %389 = arith.xori %378, %true : i1
    %390 = arith.ori %389, %379 : i1
    %391 = arith.xori %381, %true : i1
    %392 = arith.ori %391, %384 : i1
    %393 = arith.ori %381, %378 : i1
    %394 = arith.xori %378, %true : i1
    %395 = arith.ori %394, %390 : i1
    %396 = arith.andi %392, %395 : i1
    %397 = arith.andi %384, %386 : i1
    %398 = arith.andi %381, %397 : i1
    %399 = arith.andi %378, %390 : i1
    %400 = arith.ori %398, %399 : i1
    %401 = arith.select %381, %388, %true_22 : i1
    %402 = arith.xori %367, %true : i1
    %403 = arith.ori %402, %375 : i1
    %404 = arith.xori %370, %true : i1
    %405 = arith.ori %404, %373 : i1
    %406 = arith.ori %370, %367 : i1
    %407 = arith.xori %367, %true : i1
    %408 = arith.ori %407, %403 : i1
    %409 = arith.andi %405, %408 : i1
    %410 = arith.andi %370, %373 : i1
    %411 = arith.andi %403, %400 : i1
    %412 = arith.andi %367, %411 : i1
    %413 = arith.ori %410, %412 : i1
    %414 = arith.select %370, %false_23, %401 : i1
    %415 = arith.xori %value_20, %true : i1
    %416 = arith.ori %415, %366 : i1
    %417 = arith.andi %416, %413 : i1 loc(#loc70)
    %418 = arith.andi %value_20, %417 : i1 loc(#loc70)
    %419 = arith.xori %328, %true : i1
    %420 = arith.ori %419, %330 : i1
    %421 = arith.ori %328, %value_20 : i1
    %422 = arith.xori %value_20, %true : i1
    %423 = arith.ori %422, %332 : i1
    %424 = arith.andi %420, %423 : i1
    %425 = arith.andi %328, %330 : i1
    %426 = arith.andi %332, %418 : i1
    %427 = arith.andi %value_20, %426 : i1
    %428 = arith.ori %425, %427 : i1
    %429 = arith.select %328, %false_12, %414 : i1
    %430 = arith.xori %429, %true : i1
    %431 = arith.xori %429, %true : i1
    %432 = arith.ori %431, %428 : i1
    %433 = arith.andi %432, %327 : i1 loc(#loc71)
    %434 = arith.andi %429, %433 : i1 loc(#loc71)
    acir.path %429 {branch = 6 : i64, kind = "branch", txn = 1 : i64} loc(#loc71)
    acir.path %429 {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc71)
    %435 = arith.xori %429, %true : i1 loc(#loc71)
    %436 = arith.ori %435, %432 : i1 loc(#loc71)
    %437 = arith.andi %429, %432 : i1 loc(#loc71)
    %438 = arith.andi %432, %327 : i1 loc(#loc71)
    %439 = arith.andi %429, %438 : i1 loc(#loc71)
    %value_30, %ready_31 = acir.read(%429) {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "peek", port = 0 : i64, resource = "top.fetch.out", txn = 1 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Fetched">, i1) loc(#loc72)
    %440 = arith.xori %429, %true : i1 loc(#loc72)
    %441 = arith.andi %436, %ready_31 : i1 loc(#loc72)
    %442 = arith.ori %440, %441 : i1 loc(#loc72)
    %443 = arith.andi %434, %442 : i1 loc(#loc72)
    %444 = arith.xori %429, %true : i1 loc(#loc72)
    %445 = arith.andi %436, %ready_31 : i1 loc(#loc72)
    %446 = arith.ori %444, %445 : i1 loc(#loc72)
    %447 = arith.andi %443, %446 : i1 loc(#loc72)
    %value_32, %ready_33 = acir.read(%429) {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "empty", port = 0 : i64, resource = "top.fetch.out", txn = 1 : i64} : (i1) -> (i1, i1) loc(#loc72)
    %448 = arith.xori %429, %true : i1 loc(#loc72)
    %449 = arith.andi %436, %ready_33 : i1 loc(#loc72)
    %450 = arith.ori %448, %449 : i1 loc(#loc72)
    %451 = arith.andi %447, %450 : i1 loc(#loc72)
    %452 = arith.xori %value_32, %true : i1 loc(#loc72)
    %453 = arith.xori %429, %true : i1 loc(#loc72)
    %454 = arith.andi %436, %452 : i1 loc(#loc72)
    %455 = arith.ori %453, %454 : i1 loc(#loc72)
    %456 = arith.andi %451, %455 : i1 loc(#loc72)
    %457 = acir.extract %326 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc73)
    acir.path %429 {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc74)
    %458 = arith.xori %429, %true : i1 loc(#loc74)
    %459 = arith.ori %458, %436 : i1 loc(#loc74)
    %460 = arith.andi %436, %439 : i1 loc(#loc74)
    %461 = arith.andi %429, %460 : i1 loc(#loc74)
    %c0_i32_34 = arith.constant {acir.source_type = "u32"} 0 : i32
    %462 = arith.cmpi eq, %457, %c0_i32_34 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc75)
    %463 = arith.andi %461, %461 : i1
    %464 = arith.xori %429, %true : i1
    %465 = arith.andi %459, %461 : i1
    %466 = arith.ori %464, %465 : i1
    %467 = arith.andi %456, %466 : i1
    %468 = arith.andi %429, %462 : i1
    %469 = arith.andi %459, %461 : i1
    %470 = arith.xori %462, %true : i1
    %471 = arith.andi %429, %470 : i1
    %472 = arith.andi %459, %461 : i1
    %473 = arith.xori %471, %true : i1
    %474 = arith.ori %473, %472 : i1
    %475 = arith.andi %472, %461 : i1
    %476 = arith.andi %471, %475 : i1
    acir.path %471 {branch = 7 : i64, kind = "branch", txn = 1 : i64} loc(#loc76)
    %value_35, %ready_36 = acir.read(%471) {branch = 7 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 1 : i64} : (i1) -> (!acir.array<i32, 32>, i1) loc(#loc76)
    %477 = arith.xori %471, %true : i1 loc(#loc76)
    %478 = arith.andi %474, %ready_36 : i1 loc(#loc76)
    %479 = arith.ori %477, %478 : i1 loc(#loc76)
    %480 = arith.andi %467, %479 : i1 loc(#loc76)
    %481 = arith.andi %ready_36, %476 : i1 loc(#loc76)
    %482 = acir.extract %value_35[%457] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc76)
    %483 = arith.xori %468, %true : i1 loc(#loc77)
    %484 = arith.ori %483, %469 : i1 loc(#loc77)
    %485 = arith.andi %474, %481 : i1 loc(#loc78)
    %486 = arith.andi %471, %485 : i1 loc(#loc78)
    %487 = arith.andi %468, %484 : i1 loc(#loc78)
    %488 = arith.ori %486, %487 : i1 loc(#loc78)
    %489 = arith.select %471, %482, %c0_i32_34 : i32 loc(#loc78)
    %490 = acir.extract %326 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc79)
    acir.path %429 {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc80)
    %491 = arith.xori %429, %true : i1 loc(#loc80)
    %492 = arith.ori %491, %436 : i1 loc(#loc80)
    %493 = arith.andi %436, %439 : i1 loc(#loc80)
    %494 = arith.andi %429, %493 : i1 loc(#loc80)
    %c0_i32_37 = arith.constant {acir.source_type = "u32"} 0 : i32
    %495 = arith.cmpi eq, %490, %c0_i32_37 {acir.id = "v5", acir.source_type = "bool"} : i32 loc(#loc75)
    %496 = arith.andi %494, %494 : i1
    %497 = arith.xori %429, %true : i1
    %498 = arith.andi %492, %494 : i1
    %499 = arith.ori %497, %498 : i1
    %500 = arith.andi %480, %499 : i1
    %501 = arith.andi %429, %495 : i1
    %502 = arith.andi %492, %494 : i1
    %503 = arith.xori %495, %true : i1
    %504 = arith.andi %429, %503 : i1
    %505 = arith.andi %492, %494 : i1
    %506 = arith.xori %504, %true : i1
    %507 = arith.ori %506, %505 : i1
    %508 = arith.andi %505, %494 : i1
    %509 = arith.andi %504, %508 : i1
    acir.path %504 {branch = 8 : i64, kind = "branch", txn = 1 : i64} loc(#loc76)
    %value_38, %ready_39 = acir.read(%504) {branch = 8 : i64, frames = ["top.decode_stage:read_operands", "top.rf:read"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 1 : i64} : (i1) -> (!acir.array<i32, 32>, i1) loc(#loc76)
    %510 = arith.xori %504, %true : i1 loc(#loc76)
    %511 = arith.andi %507, %ready_39 : i1 loc(#loc76)
    %512 = arith.ori %510, %511 : i1 loc(#loc76)
    %513 = arith.andi %500, %512 : i1 loc(#loc76)
    %514 = arith.andi %ready_39, %509 : i1 loc(#loc76)
    %515 = acir.extract %value_38[%490] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc76)
    %516 = arith.xori %501, %true : i1 loc(#loc77)
    %517 = arith.ori %516, %502 : i1 loc(#loc77)
    %518 = arith.andi %507, %514 : i1 loc(#loc78)
    %519 = arith.andi %504, %518 : i1 loc(#loc78)
    %520 = arith.andi %501, %517 : i1 loc(#loc78)
    %521 = arith.ori %519, %520 : i1 loc(#loc78)
    %522 = arith.select %504, %515, %c0_i32_37 : i32 loc(#loc78)
    %523 = acir.extract %326 {acir.id = "v10", acir.source_type = "bool", field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc81)
    %524 = arith.xori %429, %true : i1
    %525 = arith.andi %436, %439 : i1
    %526 = arith.ori %524, %525 : i1
    %527 = arith.andi %513, %526 : i1
    %528 = arith.andi %429, %523 : i1
    %529 = arith.andi %436, %439 : i1
    %530 = arith.xori %523, %true : i1
    %531 = arith.andi %429, %530 : i1
    %532 = arith.andi %436, %439 : i1
    %533 = arith.xori %531, %true : i1
    %534 = arith.ori %533, %532 : i1
    %535 = arith.xori %528, %true : i1
    %536 = arith.ori %535, %529 : i1
    %537 = acir.extract %326 {acir.id = "v12", acir.source_type = "u32", field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc82)
    %538 = arith.xori %531, %true : i1
    %539 = arith.ori %538, %534 : i1
    %540 = arith.ori %531, %528 : i1
    %541 = arith.xori %528, %true : i1
    %542 = arith.ori %541, %536 : i1
    %543 = arith.andi %539, %542 : i1
    %544 = arith.andi %534, %521 : i1
    %545 = arith.andi %531, %544 : i1
    %546 = arith.andi %536, %439 : i1
    %547 = arith.andi %528, %546 : i1
    %548 = arith.ori %545, %547 : i1
    %549 = arith.select %531, %522, %537 : i32
    %550 = arith.xori %429, %true : i1
    %551 = arith.ori %550, %436 : i1
    acir.path %429 {branch = 6 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 1 : i64} loc(#loc83)
    %552 = arith.xori %429, %true : i1 loc(#loc83)
    %553 = arith.ori %552, %551 : i1 loc(#loc83)
    %554 = arith.andi %551, %439 : i1 loc(#loc83)
    %555 = arith.andi %429, %554 : i1 loc(#loc83)
    %true_40 = arith.constant {acir.source_type = "bool"} true
    %false_41 = arith.constant {acir.source_type = "bool"} false
    %c0_i32_42 = arith.constant {acir.source_type = "u32"} 0 : i32
    %556 = acir.extract %326 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc84)
    %557 = arith.xori %429, %true : i1
    %558 = arith.andi %553, %555 : i1
    %559 = arith.ori %557, %558 : i1
    %560 = arith.andi %527, %559 : i1
    %561 = arith.andi %429, %556 : i1
    %562 = arith.andi %553, %555 : i1
    %563 = arith.xori %556, %true : i1
    %564 = arith.andi %429, %563 : i1
    %565 = arith.andi %553, %555 : i1
    %566 = arith.xori %564, %true : i1
    %567 = arith.ori %566, %565 : i1
    %568 = arith.xori %561, %true : i1
    %569 = arith.ori %568, %562 : i1
    %570 = acir.extract %326 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc85)
    %571 = arith.cmpi ne, %570, %c0_i32_42 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc85)
    %572 = arith.xori %564, %true : i1
    %573 = arith.ori %572, %567 : i1
    %574 = arith.ori %564, %561 : i1
    %575 = arith.xori %561, %true : i1
    %576 = arith.ori %575, %569 : i1
    %577 = arith.andi %573, %576 : i1
    %578 = arith.andi %564, %567 : i1
    %579 = arith.andi %569, %555 : i1
    %580 = arith.andi %561, %579 : i1
    %581 = arith.ori %578, %580 : i1
    %582 = arith.select %564, %false_41, %571 : i1
    %583 = arith.xori %429, %true : i1
    %584 = arith.ori %583, %553 : i1
    %585 = arith.xori %429, %true : i1
    %586 = arith.andi %584, %581 : i1
    %587 = arith.ori %585, %586 : i1
    %588 = arith.andi %560, %587 : i1
    %589 = arith.andi %429, %582 : i1
    %590 = arith.andi %584, %581 : i1
    %591 = arith.xori %582, %true : i1
    %592 = arith.andi %429, %591 : i1
    %593 = arith.andi %584, %581 : i1
    %594 = arith.xori %589, %true : i1
    %595 = arith.ori %594, %590 : i1
    acir.path %589 {branch = 9 : i64, kind = "branch", txn = 1 : i64} loc(#loc86)
    %value_43, %ready_44 = acir.read(%589) {branch = 9 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = 1 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc86)
    %596 = arith.xori %589, %true : i1 loc(#loc86)
    %597 = arith.andi %595, %ready_44 : i1 loc(#loc86)
    %598 = arith.ori %596, %597 : i1 loc(#loc86)
    %599 = arith.andi %588, %598 : i1 loc(#loc86)
    %value_45, %ready_46 = acir.read(%589) {branch = 9 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = 1 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc86)
    %600 = arith.xori %589, %true : i1 loc(#loc86)
    %601 = arith.andi %595, %ready_46 : i1 loc(#loc86)
    %602 = arith.ori %600, %601 : i1 loc(#loc86)
    %603 = arith.andi %599, %602 : i1 loc(#loc86)
    %604 = acir.extract %326 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc87)
    %605 = arith.andi %ready_46, %555 : i1 loc(#loc86)
    %606 = acir.update %value_45, %true_40[%604] {acir.id = "v24", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc86)
    %607 = arith.xori %589, %true : i1 loc(#loc86)
    %608 = arith.andi %595, %605 : i1 loc(#loc86)
    %609 = arith.ori %607, %608 : i1 loc(#loc86)
    %610 = arith.andi %603, %609 : i1 loc(#loc86)
    %611 = arith.xori %592, %true : i1
    %612 = arith.ori %611, %593 : i1
    %613 = arith.ori %592, %589 : i1
    %614 = arith.xori %589, %true : i1
    %615 = arith.ori %614, %595 : i1
    %616 = arith.andi %612, %615 : i1
    %617 = arith.andi %429, %584 : i1
    %618 = acir.extract %326 {acir.id = "v17", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc88)
    %619 = acir.extract %326 {acir.id = "v18", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc89)
    %620 = acir.extract %326 {acir.id = "v19", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc90)
    %621 = acir.extract %326 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc91)
    %622 = acir.extract %326 {acir.id = "v21", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc92)
    %623 = arith.andi %439, %439 : i1 loc(#loc93)
    %624 = arith.andi %623, %439 : i1 loc(#loc93)
    %625 = arith.andi %624, %439 : i1 loc(#loc93)
    %626 = arith.andi %625, %488 : i1 loc(#loc93)
    %627 = arith.andi %626, %548 : i1 loc(#loc93)
    %628 = arith.andi %627, %521 : i1 loc(#loc93)
    %629 = arith.andi %628, %439 : i1 loc(#loc93)
    %630 = acir.aggregate(%618, %619, %620, %621, %489, %549, %522, %622) {acir.id = "v22", acir.source_type = "src_isa_f0424bc8.Operands"} : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc93)
    %631 = arith.andi %551, %629 : i1 loc(#loc94)
    %632 = arith.andi %429, %631 : i1 loc(#loc94)
    %633 = arith.andi %429, %632 : i1 loc(#loc94)
    %634 = arith.xori %633, %true : i1 loc(#loc94)
    %635 = arith.ori %634, %432 : i1 loc(#loc94)
    %636 = arith.andi %610, %635 : i1 loc(#loc94)
    acir.path %633 {branch = 10 : i64, kind = "branch", txn = 1 : i64} loc(#loc94)
    %value_47, %ready_48 = acir.read(%633) {branch = 10 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.decode_stage.out", txn = 1 : i64} : (i1) -> (i1, i1) loc(#loc94)
    %637 = arith.xori %633, %true : i1 loc(#loc94)
    %638 = arith.andi %432, %ready_48 : i1 loc(#loc94)
    %639 = arith.ori %637, %638 : i1 loc(#loc94)
    %640 = arith.andi %636, %639 : i1 loc(#loc94)
    %641 = arith.xori %value_47, %true : i1 loc(#loc94)
    %642 = arith.xori %633, %true : i1 loc(#loc94)
    %643 = arith.andi %432, %641 : i1 loc(#loc94)
    %644 = arith.ori %642, %643 : i1 loc(#loc94)
    %645 = arith.andi %640, %644 : i1 loc(#loc94)
    %646 = acir.fire "top.decode_stage.read_operands#1" %645 {successors = [0], txn = 1 : i64} loc(#loc94)
    %647 = arith.andi %646, %429 : i1 loc(#loc94)
    acir.effect %647() {branch = 6 : i64, frames = ["top.decode_stage:read_operands"], method = "pop", port = 0 : i64, resource = "top.fetch.out", txn = 1 : i64} : (i1) -> () loc(#loc72)
    %648 = arith.andi %646, %589 : i1 loc(#loc94)
    acir.effect %648(%606) {branch = 9 : i64, frames = ["top.decode_stage:read_operands", "top.rf:reserve"], method = "write", port = 0 : i64, resource = "top.rf.pending", txn = 1 : i64} : (i1, !acir.array<i1, 32>) -> () loc(#loc86)
    %649 = arith.andi %646, %633 : i1 loc(#loc94)
    acir.effect %649(%630) {branch = 10 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.decode_stage.out", txn = 1 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Operands">) -> () loc(#loc94)
    %650 = arith.xori %430, %true : i1
    %651 = arith.ori %650, %428 : i1
    %652 = arith.ori %430, %429 : i1
    %653 = arith.xori %429, %true : i1
    %654 = arith.ori %653, %432 : i1
    %655 = arith.andi %651, %654 : i1
    acir.path %true {branch = 11 : i64, kind = "branch", txn = 2 : i64} loc(#loc95)
    acir.path %true {branch = 11 : i64, frames = ["top.execute:execute"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 2 : i64} loc(#loc95)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc96)
    %value_49, %ready_50 = acir.read(%true) {branch = 11 : i64, frames = ["top.execute:execute"], method = "peek", port = 0 : i64, resource = "top.decode_stage.out", txn = 2 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Operands">, i1) loc(#loc97)
    %656 = arith.andi %ready_50, %ready_50 : i1 loc(#loc97)
    %value_51, %ready_52 = acir.read(%true) {branch = 11 : i64, frames = ["top.execute:execute"], method = "empty", port = 0 : i64, resource = "top.decode_stage.out", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc97)
    %657 = arith.andi %656, %ready_52 : i1 loc(#loc97)
    %658 = arith.xori %value_51, %true : i1 loc(#loc97)
    %659 = arith.andi %657, %658 : i1 loc(#loc97)
    %660 = acir.extract %value_49 {acir.id = "v5", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc98)
    %661 = arith.cmpi eq, %660, %c1_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc98)
    %662 = arith.andi %659, %ready_50 : i1
    %663 = arith.xori %661, %true : i1
    %664 = arith.xori %663, %true : i1
    %665 = arith.ori %664, %ready_50 : i1
    %666 = acir.extract %value_49 {acir.id = "v12", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc99)
    %667 = acir.extract %value_49 {acir.id = "v13", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc100)
    %668 = arith.andi %ready_50, %ready_50 : i1 loc(#loc99)
    %669 = arith.addi %666, %667 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc99)
    %670 = arith.xori %661, %true : i1
    %671 = arith.ori %670, %ready_50 : i1
    %672 = acir.extract %value_49 {acir.id = "v9", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc101)
    %673 = acir.extract %value_49 {acir.id = "v10", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc102)
    %674 = arith.andi %ready_50, %ready_50 : i1 loc(#loc101)
    %675 = arith.subi %672, %673 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc101)
    %676 = arith.xori %663, %true : i1
    %677 = arith.ori %676, %665 : i1
    %678 = arith.ori %663, %661 : i1
    %679 = arith.xori %661, %true : i1
    %680 = arith.ori %679, %671 : i1
    %681 = arith.andi %677, %680 : i1
    %682 = arith.andi %665, %668 : i1
    %683 = arith.andi %663, %682 : i1
    %684 = arith.andi %671, %674 : i1
    %685 = arith.andi %661, %684 : i1
    %686 = arith.ori %683, %685 : i1
    %687 = arith.select %663, %669, %675 : i32
    %688 = acir.extract %value_49 {acir.id = "v18", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc103)
    %689 = acir.extract %value_49 {acir.id = "v19", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc104)
    %690 = acir.extract %value_49 {acir.id = "v20", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc105)
    %691 = acir.extract %value_49 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc106)
    %692 = acir.extract %value_49 {acir.id = "v22", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc107)
    %693 = acir.extract %value_49 {acir.id = "v23", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc108)
    %694 = arith.andi %ready_50, %ready_50 : i1 loc(#loc109)
    %695 = arith.andi %694, %ready_50 : i1 loc(#loc109)
    %696 = arith.andi %695, %ready_50 : i1 loc(#loc109)
    %697 = arith.andi %696, %686 : i1 loc(#loc109)
    %698 = arith.andi %697, %ready_50 : i1 loc(#loc109)
    %699 = arith.andi %698, %ready_50 : i1 loc(#loc109)
    %700 = acir.aggregate(%688, %689, %690, %691, %687, %692, %693) {acir.id = "v24", acir.source_type = "src_isa_f0424bc8.Executed"} : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc109)
    %701 = arith.xori %699, %true : i1 loc(#loc110)
    acir.path %699 {branch = 12 : i64, kind = "branch", txn = 2 : i64} loc(#loc110)
    %value_53, %ready_54 = acir.read(%699) {branch = 12 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.execute.out", txn = 2 : i64} : (i1) -> (i1, i1) loc(#loc110)
    %702 = arith.xori %699, %true : i1 loc(#loc110)
    %703 = arith.ori %702, %ready_54 : i1 loc(#loc110)
    %704 = arith.andi %662, %703 : i1 loc(#loc110)
    %705 = arith.xori %value_53, %true : i1 loc(#loc110)
    %706 = arith.xori %699, %true : i1 loc(#loc110)
    %707 = arith.ori %706, %705 : i1 loc(#loc110)
    %708 = arith.andi %704, %707 : i1 loc(#loc110)
    %709 = acir.fire "top.execute.execute#2" %708 {successors = [1], txn = 2 : i64} loc(#loc110)
    acir.effect %709() {branch = 11 : i64, frames = ["top.execute:execute"], method = "pop", port = 0 : i64, resource = "top.decode_stage.out", txn = 2 : i64} : (i1) -> () loc(#loc97)
    %710 = arith.andi %709, %699 : i1 loc(#loc110)
    acir.effect %710(%700) {branch = 12 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.execute.out", txn = 2 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Executed">) -> () loc(#loc110)
    acir.path %true {branch = 13 : i64, kind = "branch", txn = 3 : i64} loc(#loc111)
    acir.path %true {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc111)
    %c3_i32_55 = arith.constant {acir.source_type = "u32"} 3 : i32
    %c2_i32_56 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc112)
    %value_57, %ready_58 = acir.read(%true) {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "peek", port = 0 : i64, resource = "top.execute.out", txn = 3 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Executed">, i1) loc(#loc113)
    %711 = arith.andi %ready_58, %ready_58 : i1 loc(#loc113)
    %value_59, %ready_60 = acir.read(%true) {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "empty", port = 0 : i64, resource = "top.execute.out", txn = 3 : i64} : (i1) -> (i1, i1) loc(#loc113)
    %712 = arith.andi %711, %ready_60 : i1 loc(#loc113)
    %713 = arith.xori %value_59, %true : i1 loc(#loc113)
    %714 = arith.andi %712, %713 : i1 loc(#loc113)
    %715 = acir.extract %value_57 {acir.id = "v5", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc114)
    %716 = acir.extract %value_57 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc115)
    %717 = arith.cmpi eq, %716, %c2_i32_56 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc115)
    %718 = arith.andi %714, %ready_58 : i1
    %719 = arith.xori %717, %true : i1
    %720 = arith.xori %719, %true : i1
    %721 = arith.ori %720, %ready_58 : i1
    %722 = acir.extract %value_57 {acir.id = "v12", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc116)
    %723 = arith.cmpi eq, %722, %c3_i32_55 {acir.id = "v14", acir.source_type = "bool"} : i32 loc(#loc116)
    %724 = arith.xori %719, %true : i1
    %725 = arith.andi %721, %ready_58 : i1
    %726 = arith.ori %724, %725 : i1
    %727 = arith.andi %718, %726 : i1
    %728 = arith.andi %719, %723 : i1
    %729 = arith.andi %721, %ready_58 : i1
    %730 = arith.xori %723, %true : i1
    %731 = arith.andi %719, %730 : i1
    %732 = arith.andi %721, %ready_58 : i1
    %733 = arith.xori %728, %true : i1
    %734 = arith.ori %733, %729 : i1
    %735 = acir.extract %value_57 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc117)
    %736 = acir.extract %value_57 {acir.id = "v17", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc118)
    %737 = arith.andi %ready_58, %ready_58 : i1 loc(#loc119)
    acir.path %728 {branch = 14 : i64, kind = "branch", txn = 3 : i64} loc(#loc119)
    acir.path %728 {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc119)
    %738 = arith.xori %728, %true : i1 loc(#loc119)
    %739 = arith.ori %738, %734 : i1 loc(#loc119)
    %740 = arith.andi %734, %ready_58 : i1 loc(#loc119)
    %741 = arith.andi %728, %740 : i1 loc(#loc119)
    %742 = arith.andi %734, %ready_58 : i1 loc(#loc119)
    %743 = arith.andi %728, %742 : i1 loc(#loc119)
    %c2_i32_61 = arith.constant {acir.source_type = "u32"} 2 : i32
    %false_62 = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32_63 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc120)
    %c3_i32_64 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc121)
    %744 = arith.andi %735, %c3_i32_64 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc122)
    %745 = arith.cmpi eq, %744, %c0_i32_63 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc122)
    %746 = arith.xori %728, %true : i1
    %747 = arith.andi %739, %741 : i1
    %748 = arith.ori %746, %747 : i1
    %749 = arith.andi %727, %748 : i1
    %750 = arith.andi %728, %745 : i1
    %751 = arith.andi %739, %741 : i1
    %752 = arith.xori %745, %true : i1
    %753 = arith.andi %728, %752 : i1
    %754 = arith.andi %739, %741 : i1
    %755 = arith.xori %753, %true : i1
    %756 = arith.ori %755, %754 : i1
    %757 = arith.xori %750, %true : i1
    %758 = arith.ori %757, %751 : i1
    %759 = arith.cmpi ult, %735, %c64_i32 {acir.id = "v11", acir.source_type = "bool"} : i32 loc(#loc123)
    %760 = arith.xori %753, %true : i1
    %761 = arith.ori %760, %756 : i1
    %762 = arith.ori %753, %750 : i1
    %763 = arith.xori %750, %true : i1
    %764 = arith.ori %763, %758 : i1
    %765 = arith.andi %761, %764 : i1
    %766 = arith.andi %753, %756 : i1
    %767 = arith.andi %758, %741 : i1
    %768 = arith.andi %750, %767 : i1
    %769 = arith.ori %766, %768 : i1
    %770 = arith.select %753, %false_62, %759 : i1
    %771 = arith.xori %728, %true : i1
    %772 = arith.ori %771, %739 : i1
    %773 = arith.xori %728, %true : i1 loc(#loc124)
    %774 = arith.andi %772, %769 : i1 loc(#loc124)
    %775 = arith.ori %773, %774 : i1 loc(#loc124)
    %776 = arith.andi %749, %775 : i1 loc(#loc124)
    %777 = arith.shrui %735, %c2_i32_61 : i32 loc(#loc125)
    %value_65, %ready_66 = acir.read(%728) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "read", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1) -> (!acir.array<i32, 16>, i1) loc(#loc126)
    %778 = arith.xori %728, %true : i1 loc(#loc126)
    %779 = arith.andi %772, %ready_66 : i1 loc(#loc126)
    %780 = arith.ori %778, %779 : i1 loc(#loc126)
    %781 = arith.andi %776, %780 : i1 loc(#loc126)
    %value_67, %ready_68 = acir.read(%728) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "read", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1) -> (!acir.array<i32, 16>, i1) loc(#loc126)
    %782 = arith.xori %728, %true : i1 loc(#loc126)
    %783 = arith.andi %772, %ready_68 : i1 loc(#loc126)
    %784 = arith.ori %782, %783 : i1 loc(#loc126)
    %785 = arith.andi %781, %784 : i1 loc(#loc126)
    %786 = arith.shrui %735, %c2_i32_61 : i32 loc(#loc125)
    %787 = arith.andi %ready_68, %743 : i1 loc(#loc126)
    %788 = arith.andi %787, %741 : i1 loc(#loc126)
    %789 = acir.update %value_67, %736[%786] {acir.id = "v28", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16> loc(#loc126)
    %790 = arith.xori %728, %true : i1 loc(#loc126)
    %791 = arith.andi %772, %788 : i1 loc(#loc126)
    %792 = arith.ori %790, %791 : i1 loc(#loc126)
    %793 = arith.andi %785, %792 : i1 loc(#loc126)
    %794 = arith.andi %728, %772 : i1
    %795 = arith.xori %731, %true : i1
    %796 = arith.ori %795, %732 : i1
    %797 = arith.ori %731, %728 : i1
    %798 = arith.xori %728, %true : i1
    %799 = arith.ori %798, %734 : i1
    %800 = arith.andi %796, %799 : i1
    %801 = arith.xori %717, %true : i1
    %802 = arith.ori %801, %ready_58 : i1
    %803 = acir.extract %value_57 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc127)
    acir.path %717 {branch = 15 : i64, kind = "branch", txn = 3 : i64} loc(#loc128)
    acir.path %717 {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 3 : i64} loc(#loc128)
    %804 = arith.xori %717, %true : i1 loc(#loc128)
    %805 = arith.ori %804, %802 : i1 loc(#loc128)
    %806 = arith.andi %802, %ready_58 : i1 loc(#loc128)
    %807 = arith.andi %717, %806 : i1 loc(#loc128)
    %c2_i32_69 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc129)
    %false_70 = arith.constant {acir.source_type = "bool"} false
    %c64_i32_71 = arith.constant {acir.source_type = "u32"} 64 : i32
    %c0_i32_72 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc130)
    %c3_i32_73 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc131)
    %808 = arith.andi %803, %c3_i32_73 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc132)
    %809 = arith.cmpi eq, %808, %c0_i32_72 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc132)
    %810 = arith.xori %717, %true : i1
    %811 = arith.andi %805, %807 : i1
    %812 = arith.ori %810, %811 : i1
    %813 = arith.andi %793, %812 : i1
    %814 = arith.andi %717, %809 : i1
    %815 = arith.andi %805, %807 : i1
    %816 = arith.xori %809, %true : i1
    %817 = arith.andi %717, %816 : i1
    %818 = arith.andi %805, %807 : i1
    %819 = arith.xori %817, %true : i1
    %820 = arith.ori %819, %818 : i1
    %821 = arith.xori %814, %true : i1
    %822 = arith.ori %821, %815 : i1
    %823 = arith.cmpi ult, %803, %c64_i32_71 {acir.id = "v10", acir.source_type = "bool"} : i32 loc(#loc133)
    %824 = arith.xori %817, %true : i1
    %825 = arith.ori %824, %820 : i1
    %826 = arith.ori %817, %814 : i1
    %827 = arith.xori %814, %true : i1
    %828 = arith.ori %827, %822 : i1
    %829 = arith.andi %825, %828 : i1
    %830 = arith.andi %817, %820 : i1
    %831 = arith.andi %822, %807 : i1
    %832 = arith.andi %814, %831 : i1
    %833 = arith.ori %830, %832 : i1
    %834 = arith.select %817, %false_70, %823 : i1
    %835 = arith.xori %717, %true : i1
    %836 = arith.ori %835, %805 : i1
    %837 = arith.xori %717, %true : i1 loc(#loc134)
    %838 = arith.andi %836, %833 : i1 loc(#loc134)
    %839 = arith.ori %837, %838 : i1 loc(#loc134)
    %840 = arith.andi %813, %839 : i1 loc(#loc134)
    %841 = arith.shrui %803, %c2_i32_69 : i32 loc(#loc135)
    %value_74, %ready_75 = acir.read(%717) {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "read", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1) -> (!acir.array<i32, 16>, i1) loc(#loc136)
    %842 = arith.xori %717, %true : i1 loc(#loc136)
    %843 = arith.andi %836, %ready_75 : i1 loc(#loc136)
    %844 = arith.ori %842, %843 : i1 loc(#loc136)
    %845 = arith.andi %840, %844 : i1 loc(#loc136)
    %846 = arith.andi %ready_75, %807 : i1 loc(#loc136)
    %847 = acir.extract %value_74[%841] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32 loc(#loc136)
    %848 = arith.andi %836, %846 : i1 loc(#loc137)
    %849 = arith.andi %717, %848 : i1 loc(#loc137)
    %850 = arith.xori %719, %true : i1
    %851 = arith.ori %850, %721 : i1
    %852 = arith.ori %719, %717 : i1
    %853 = arith.xori %717, %true : i1
    %854 = arith.ori %853, %802 : i1
    %855 = arith.andi %851, %854 : i1
    %856 = arith.andi %721, %ready_58 : i1
    %857 = arith.andi %719, %856 : i1
    %858 = arith.andi %802, %849 : i1
    %859 = arith.andi %717, %858 : i1
    %860 = arith.ori %857, %859 : i1
    %861 = arith.select %719, %715, %847 : i32
    %862 = acir.extract %value_57 {acir.id = "v29", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc138)
    %863 = acir.extract %value_57 {acir.id = "v30", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc139)
    %864 = acir.extract %value_57 {acir.id = "v31", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc140)
    %865 = acir.extract %value_57 {acir.id = "v32", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc141)
    %866 = arith.andi %ready_58, %ready_58 : i1 loc(#loc142)
    %867 = arith.andi %866, %ready_58 : i1 loc(#loc142)
    %868 = arith.andi %867, %860 : i1 loc(#loc142)
    %869 = arith.andi %868, %ready_58 : i1 loc(#loc142)
    %870 = acir.aggregate(%862, %863, %864, %861, %865) {acir.id = "v33", acir.source_type = "src_isa_f0424bc8.Retired"} : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc142)
    %871 = arith.xori %869, %true : i1 loc(#loc143)
    acir.path %869 {branch = 16 : i64, kind = "branch", txn = 3 : i64} loc(#loc143)
    %value_76, %ready_77 = acir.read(%869) {branch = 16 : i64, frames = [], method = "full", port = 0 : i64, resource = "top.retirement", txn = 3 : i64} : (i1) -> (i1, i1) loc(#loc143)
    %872 = arith.xori %869, %true : i1 loc(#loc143)
    %873 = arith.ori %872, %ready_77 : i1 loc(#loc143)
    %874 = arith.andi %845, %873 : i1 loc(#loc143)
    %875 = arith.xori %value_76, %true : i1 loc(#loc143)
    %876 = arith.xori %869, %true : i1 loc(#loc143)
    %877 = arith.ori %876, %875 : i1 loc(#loc143)
    %878 = arith.andi %874, %877 : i1 loc(#loc143)
    %879 = acir.fire "top.memory_stage.access#3" %878 {successors = [2], txn = 3 : i64} loc(#loc143)
    acir.effect %879() {branch = 13 : i64, frames = ["top.memory_stage:access"], method = "pop", port = 0 : i64, resource = "top.execute.out", txn = 3 : i64} : (i1) -> () loc(#loc113)
    %880 = arith.andi %879, %728 : i1 loc(#loc143)
    acir.effect %880(%770) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc124)
    %881 = arith.andi %879, %728 : i1 loc(#loc143)
    acir.effect %881(%789) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "write", port = 0 : i64, resource = "top.memory.words", txn = 3 : i64} : (i1, !acir.array<i32, 16>) -> () loc(#loc126)
    %882 = arith.andi %879, %717 : i1 loc(#loc143)
    acir.effect %882(%834) {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc134)
    %883 = arith.andi %879, %869 : i1 loc(#loc143)
    acir.effect %883(%870) {branch = 16 : i64, frames = [], method = "push", port = 0 : i64, resource = "top.retirement", txn = 3 : i64} : (i1, !acir.struct<"src_isa_f0424bc8.Retired">) -> () loc(#loc143)
    acir.path %true {branch = 17 : i64, kind = "branch", txn = 4 : i64} loc(#loc144)
    acir.path %true {branch = 17 : i64, frames = ["top.writeback:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc144)
    %c1_i32_78 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc145)
    %value_79, %ready_80 = acir.read(%true) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "peek", port = 0 : i64, resource = "top.retirement", txn = 4 : i64} : (i1) -> (!acir.struct<"src_isa_f0424bc8.Retired">, i1) loc(#loc146)
    %884 = arith.andi %ready_80, %ready_80 : i1 loc(#loc146)
    %value_81, %ready_82 = acir.read(%true) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "empty", port = 0 : i64, resource = "top.retirement", txn = 4 : i64} : (i1) -> (i1, i1) loc(#loc146)
    %885 = arith.andi %884, %ready_82 : i1 loc(#loc146)
    %886 = arith.xori %value_81, %true : i1 loc(#loc146)
    %887 = arith.andi %885, %886 : i1 loc(#loc146)
    acir.path %true {branch = 17 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "callsite", port = 0 : i64, resource = "Module contract", txn = 4 : i64} loc(#loc147)
    %false_83 = arith.constant {acir.source_type = "bool"} false
    %c0_i32_84 = arith.constant {acir.source_type = "u32"} 0 : i32
    %888 = acir.extract %value_79 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc148)
    %889 = arith.andi %887, %ready_80 : i1
    %890 = arith.xori %888, %true : i1
    %891 = arith.xori %890, %true : i1
    %892 = arith.ori %891, %ready_80 : i1
    %893 = arith.xori %888, %true : i1
    %894 = arith.ori %893, %ready_80 : i1
    %895 = acir.extract %value_79 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc149)
    %896 = arith.cmpi ne, %895, %c0_i32_84 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc149)
    %897 = arith.xori %890, %true : i1
    %898 = arith.ori %897, %892 : i1
    %899 = arith.ori %890, %888 : i1
    %900 = arith.xori %888, %true : i1
    %901 = arith.ori %900, %894 : i1
    %902 = arith.andi %898, %901 : i1
    %903 = arith.andi %890, %892 : i1
    %904 = arith.andi %894, %ready_80 : i1
    %905 = arith.andi %888, %904 : i1
    %906 = arith.ori %903, %905 : i1
    %907 = arith.select %890, %false_83, %896 : i1
    %908 = arith.andi %889, %906 : i1
    %909 = arith.xori %907, %true : i1
    %910 = arith.xori %907, %true : i1
    %911 = arith.ori %910, %906 : i1
    %912 = acir.extract %value_79 {acir.id = "v15", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc150)
    acir.path %907 {branch = 18 : i64, kind = "branch", txn = 4 : i64} loc(#loc151)
    %value_85, %ready_86 = acir.read(%907) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 4 : i64} : (i1) -> (!acir.array<i32, 32>, i1) loc(#loc151)
    %913 = arith.xori %907, %true : i1 loc(#loc151)
    %914 = arith.andi %911, %ready_86 : i1 loc(#loc151)
    %915 = arith.ori %913, %914 : i1 loc(#loc151)
    %916 = arith.andi %908, %915 : i1 loc(#loc151)
    %value_87, %ready_88 = acir.read(%907) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.registers", txn = 4 : i64} : (i1) -> (!acir.array<i32, 32>, i1) loc(#loc151)
    %917 = arith.xori %907, %true : i1 loc(#loc151)
    %918 = arith.andi %911, %ready_88 : i1 loc(#loc151)
    %919 = arith.ori %917, %918 : i1 loc(#loc151)
    %920 = arith.andi %916, %919 : i1 loc(#loc151)
    %921 = acir.extract %value_79 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc152)
    %922 = arith.andi %ready_88, %ready_80 : i1 loc(#loc151)
    %923 = arith.andi %922, %ready_80 : i1 loc(#loc151)
    %924 = acir.update %value_87, %912[%921] {acir.id = "v24", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32> loc(#loc151)
    %925 = arith.xori %907, %true : i1 loc(#loc151)
    %926 = arith.andi %911, %923 : i1 loc(#loc151)
    %927 = arith.ori %925, %926 : i1 loc(#loc151)
    %928 = arith.andi %920, %927 : i1 loc(#loc151)
    %value_89, %ready_90 = acir.read(%907) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = 4 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc153)
    %929 = arith.xori %907, %true : i1 loc(#loc153)
    %930 = arith.andi %911, %ready_90 : i1 loc(#loc153)
    %931 = arith.ori %929, %930 : i1 loc(#loc153)
    %932 = arith.andi %928, %931 : i1 loc(#loc153)
    %value_91, %ready_92 = acir.read(%907) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "read", port = 0 : i64, resource = "top.rf.pending", txn = 4 : i64} : (i1) -> (!acir.array<i1, 32>, i1) loc(#loc153)
    %933 = arith.xori %907, %true : i1 loc(#loc153)
    %934 = arith.andi %911, %ready_92 : i1 loc(#loc153)
    %935 = arith.ori %933, %934 : i1 loc(#loc153)
    %936 = arith.andi %932, %935 : i1 loc(#loc153)
    %937 = acir.extract %value_79 {acir.id = "v36", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc154)
    %938 = arith.andi %ready_92, %ready_80 : i1 loc(#loc153)
    %939 = acir.update %value_91, %false_83[%937] {acir.id = "v37", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc153)
    %940 = arith.xori %907, %true : i1 loc(#loc153)
    %941 = arith.andi %911, %938 : i1 loc(#loc153)
    %942 = arith.ori %940, %941 : i1 loc(#loc153)
    %943 = arith.andi %936, %942 : i1 loc(#loc153)
    %944 = arith.xori %909, %true : i1
    %945 = arith.ori %944, %906 : i1
    %946 = arith.ori %909, %907 : i1
    %947 = arith.xori %907, %true : i1
    %948 = arith.ori %947, %911 : i1
    %949 = arith.andi %945, %948 : i1
    %value_93, %ready_94 = acir.read(%true) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "read", port = 0 : i64, resource = "top.writeback.retired", txn = 4 : i64} : (i1) -> (i32, i1) loc(#loc155)
    %950 = arith.andi %943, %ready_94 : i1 loc(#loc155)
    %951 = arith.addi %value_93, %c1_i32_78 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc155)
    %952 = arith.andi %950, %ready_94 : i1 loc(#loc156)
    %953 = acir.extract %value_79 {acir.id = "v12", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc157)
    %954 = arith.andi %952, %ready_80 : i1 loc(#loc158)
    %955 = acir.fire "top.writeback.commit#4" %954 {successors = [3], txn = 4 : i64}
    acir.effect %955() {branch = 17 : i64, frames = ["top.writeback:commit"], method = "pop", port = 0 : i64, resource = "top.retirement", txn = 4 : i64} : (i1) -> () loc(#loc146)
    %956 = arith.andi %955, %907 : i1
    acir.effect %956(%924) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.registers", txn = 4 : i64} : (i1, !acir.array<i32, 32>) -> () loc(#loc151)
    %957 = arith.andi %955, %907 : i1
    acir.effect %957(%939) {branch = 18 : i64, frames = ["top.writeback:commit", "top.rf:commit"], method = "write", port = 0 : i64, resource = "top.rf.pending", txn = 4 : i64} : (i1, !acir.array<i1, 32>) -> () loc(#loc153)
    acir.effect %955(%951) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "write", port = 0 : i64, resource = "top.writeback.retired", txn = 4 : i64} : (i1, i32) -> () loc(#loc156)
    acir.effect %955(%953) {branch = 17 : i64, frames = ["top.writeback:commit"], method = "write", port = 0 : i64, resource = "top.writeback.last_pc", txn = 4 : i64} : (i1, i32) -> () loc(#loc158)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":163:9)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":164:9)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":165:9)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":166:9)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:25)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":68:22)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:12)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":70:19)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:27)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:34)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:20)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:16)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:14)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:9)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":65:16)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":65:9)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":89:23)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:18)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:19)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:22)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:21)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":53:12)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:14)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:14)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":56:14)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":57:10)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:11)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":59:11)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:8)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":69:10)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":73:10)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:16)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:33)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:9)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:28)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:46)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:21)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:13)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:12)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":50:5)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:16)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:9)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:28)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:21)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:16)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:33)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:48)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:9)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":71:12)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:16)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:9)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:28)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:21)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:20)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:12)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:5)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:19)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:42)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:28)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:20)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:16)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:55)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:47)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:43)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:75)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:71)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:102)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:94)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:90)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:9)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":92:19)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:20)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:23)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:15)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":22:12)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:16)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:9)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":23:13)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:31)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:23)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:36)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:15)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":85:9)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:12)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:28)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:13)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:21)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:25)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:37)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:51)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:65)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:100)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:16)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":86:9)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":108:15)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:54)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":103:16)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:41)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:63)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:74)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:18)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":104:29)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:25)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:34)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:45)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:56)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:73)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:91)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:16)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":105:9)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":128:15)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:25)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":119:16)
#loc114 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:18)
#loc115 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:12)
#loc116 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":123:14)
#loc117 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:26)
#loc118 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:38)
#loc119 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":124:13)
#loc120 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:31)
#loc121 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:26)
#loc122 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:16)
#loc123 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:37)
#loc124 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:9)
#loc125 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:15)
#loc126 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:9)
#loc127 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:34)
#loc128 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":122:22)
#loc129 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:33)
#loc130 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:31)
#loc131 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:26)
#loc132 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:16)
#loc133 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:37)
#loc134 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:9)
#loc135 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:22)
#loc136 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:16)
#loc137 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:9)
#loc138 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:24)
#loc139 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:33)
#loc140 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:44)
#loc141 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:61)
#loc142 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:16)
#loc143 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":125:9)
#loc144 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":146:9)
#loc145 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":142:29)
#loc146 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":140:16)
#loc147 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":141:9)
#loc148 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:12)
#loc149 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:28)
#loc150 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:34)
#loc151 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:13)
#loc152 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:23)
#loc153 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:13)
#loc154 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:21)
#loc155 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":142:19)
#loc156 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":142:9)
#loc157 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":143:19)
#loc158 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":143:9)

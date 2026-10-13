module attributes {acir.model = {config = [], constants = {src_isa_f0424bc8.ADD = 0 : i64, src_isa_f0424bc8.LOAD = 2 : i64, src_isa_f0424bc8.STORE = 3 : i64, src_isa_f0424bc8.SUB = 1 : i64}, exports = {}, format_version = 8 : i64, top = "Pipeline", type_order = ["src_isa_f0424bc8.Fetched", "src_isa_f0424bc8.Decoded", "src_isa_f0424bc8.Operands", "src_isa_f0424bc8.Executed", "src_isa_f0424bc8.Retired"], types = {src_isa_f0424bc8.Decoded = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "rs1", type = "u32"}, {default = 0 : i64, name = "rs2", type = "u32"}, {default = 0 : i64, name = "immediate", type = "u32"}, {default = false, name = "immediate_rhs", type = "bool"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Executed = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Fetched = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}], src_isa_f0424bc8.Operands = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "kind", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "lhs", type = "u32"}, {default = 0 : i64, name = "rhs", type = "u32"}, {default = 0 : i64, name = "store_value", type = "u32"}, {default = false, name = "writes", type = "bool"}], src_isa_f0424bc8.Retired = [{default = 0 : i64, name = "pc", type = "u32"}, {default = 0 : i64, name = "word", type = "u32"}, {default = 0 : i64, name = "rd", type = "u32"}, {default = 0 : i64, name = "value", type = "u32"}, {default = false, name = "writes", type = "bool"}]}}, acir.stage = "rtl"} {
  acir.circuit attributes {conflicts = [], edges = [{from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.ex_mem", source_branch = 13 : i64, target_branches = [12], to = 2 : i64}, {from = 3 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.ex_mem", source_branch = 13 : i64, target_branches = [12], to = 2 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.id_ex", source_branch = 11 : i64, target_branches = [10], to = 1 : i64}, {from = 2 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.id_ex", source_branch = 11 : i64, target_branches = [10], to = 1 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.if_id", source_branch = 6 : i64, target_branches = [2], to = 0 : i64}, {from = 1 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.if_id", source_branch = 6 : i64, target_branches = [2], to = 0 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mem_wb", source_branch = 17 : i64, target_branches = [16], to = 3 : i64}, {from = 4 : i64, relation = "Queue.pop \E2\86\92 push", resource = "top.mem_wb", source_branch = 17 : i64, target_branches = [16], to = 3 : i64}], schedule = [4, 3, 2, 1, 0], stage = "rtl"} {
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
    %true = arith.constant true
    %false = arith.constant false
    %0 = acir.current {aspect = "value", resource = "top.fetch.pc"} : i32 loc(#loc1)
    %c24_i32 = arith.constant {acir.source_type = "u32"} 24 : i32 loc(#loc2)
    %1 = arith.cmpi ult, %0, %c24_i32 {acir.id = "v4", acir.source_type = "bool"} : i32 loc(#loc2)
    %true_0 = arith.constant true
    %2 = arith.xori %1, %true_0 : i1
    %3 = arith.ori %2, %true : i1
    %4 = arith.andi %3, %true : i1
    %5 = arith.andi %1, %4 : i1
    %6 = arith.xori %1, %true_0 : i1 loc(#loc3)
    %7 = arith.xori %1, %true_0 : i1 loc(#loc4)
    %8 = arith.ori %7, %3 : i1 loc(#loc4)
    %9 = acir.current {aspect = "value", resource = "top.fetch.rom"} : !acir.array<i32, 6> loc(#loc3)
    %10 = arith.andi %8, %true : i1 loc(#loc3)
    %11 = arith.ori %6, %10 : i1 loc(#loc3)
    %12 = arith.andi %5, %11 : i1 loc(#loc3)
    %13 = arith.xori %1, %true_0 : i1 loc(#loc5)
    %14 = arith.andi %3, %true : i1 loc(#loc4)
    %15 = arith.andi %1, %14 : i1 loc(#loc4)
    %16 = arith.andi %8, %15 : i1 loc(#loc4)
    %17 = arith.ori %13, %16 : i1 loc(#loc4)
    %18 = arith.andi %12, %17 : i1 loc(#loc4)
    %19 = arith.andi %true, %15 : i1 loc(#loc3)
    %20 = arith.andi %15, %19 : i1 loc(#loc3)
    %21 = arith.andi %8, %20 : i1 loc(#loc3)
    %22 = arith.andi %1, %21 : i1 loc(#loc3)
    %23 = arith.andi %1, %22 : i1 loc(#loc3)
    %24 = arith.xori %23, %true_0 : i1 loc(#loc3)
    %25 = arith.ori %24, %3 : i1 loc(#loc3)
    %26 = arith.andi %18, %25 : i1 loc(#loc3)
    %27 = arith.xori %23, %true_0 : i1 loc(#loc6)
    %28 = acir.current {aspect = "count", resource = "top.if_id"} : i32 loc(#loc6)
    %29 = acir.current {aspect = "count", resource = "top.mem_wb"} : i32 loc(#loc7)
    %c0_i32 = arith.constant 0 : i32 loc(#loc7)
    %30 = arith.cmpi eq, %29, %c0_i32 : i32 loc(#loc7)
    %31 = arith.xori %30, %true_0 : i1 loc(#loc7)
    %false_1 = arith.constant {acir.source_type = "bool"} false
    %32 = acir.current {aspect = "value", resource = "top.rf.pending"} : !acir.array<i1, 32> loc(#loc8)
    %33 = acir.current {aspect = "storage", resource = "top.if_id"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1> loc(#loc9)
    %c0_i32_2 = arith.constant 0 : i32 loc(#loc9)
    %34 = acir.extract %33[%c0_i32_2] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc9)
    %c0_i32_3 = arith.constant 0 : i32 loc(#loc9)
    %35 = arith.cmpi ne, %28, %c0_i32_3 : i32 loc(#loc9)
    %36 = acir.extract %34 {acir.id = "v154", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc9)
    %37 = acir.extract %34 {acir.id = "v2", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> i32 loc(#loc10)
    %c127_i32 = arith.constant {acir.source_type = "u32"} 127 : i32 loc(#loc11)
    %38 = arith.andi %37, %c127_i32 {acir.id = "v4", acir.source_type = "u32"} : i32 loc(#loc11)
    %c19_i32 = arith.constant {acir.source_type = "u32"} 19 : i32 loc(#loc12)
    %39 = arith.cmpi eq, %38, %c19_i32 {acir.id = "v28", acir.source_type = "bool"} : i32 loc(#loc12)
    %40 = arith.xori %39, %true_0 : i1 loc(#loc12)
    %c51_i32 = arith.constant {acir.source_type = "u32"} 51 : i32
    %41 = arith.cmpi eq, %38, %c51_i32 {acir.id = "v39", acir.source_type = "bool"} : i32
    %42 = arith.xori %41, %true_0 : i1
    %43 = arith.andi %40, %42 : i1
    %c3_i32 = arith.constant {acir.source_type = "u32"} 3 : i32
    %44 = arith.cmpi eq, %38, %c3_i32 {acir.id = "v77", acir.source_type = "bool"} : i32
    %c2_i32 = arith.constant {acir.source_type = "u32"} 2 : i32
    %45 = arith.select %44, %c2_i32, %c3_i32 : i32
    %c25_i32 = arith.constant {acir.source_type = "u32"} 25 : i32
    %46 = arith.shrui %37, %c25_i32 : i32
    %c32_i32 = arith.constant {acir.source_type = "u32"} 32 : i32
    %47 = arith.cmpi eq, %46, %c32_i32 {acir.id = "v59", acir.source_type = "bool"} : i32
    %48 = arith.extui %47 : i1 to i32
    %49 = arith.select %43, %45, %48 : i32
    %c0_i32_4 = arith.constant {acir.source_type = "u32"} 0 : i32
    %50 = arith.select %40, %49, %c0_i32_4 : i32
    %c7_i32 = arith.constant {acir.source_type = "u32"} 7 : i32
    %51 = arith.shrui %37, %c7_i32 : i32
    %c31_i32 = arith.constant {acir.source_type = "u32"} 31 : i32
    %52 = arith.andi %51, %c31_i32 {acir.id = "v14", acir.source_type = "u32"} : i32
    %53 = arith.select %44, %52, %c0_i32_4 : i32
    %54 = arith.select %43, %53, %52 : i32
    %55 = arith.select %40, %54, %52 : i32
    %c15_i32 = arith.constant {acir.source_type = "u32"} 15 : i32 loc(#loc13)
    %56 = arith.shrui %37, %c15_i32 : i32 loc(#loc13)
    %57 = arith.andi %56, %c31_i32 {acir.id = "v18", acir.source_type = "u32"} : i32 loc(#loc13)
    %c20_i32 = arith.constant {acir.source_type = "u32"} 20 : i32
    %58 = arith.shrui %37, %c20_i32 : i32
    %59 = arith.andi %58, %c31_i32 {acir.id = "v22", acir.source_type = "u32"} : i32
    %60 = arith.select %44, %c0_i32_4, %59 : i32
    %61 = arith.select %43, %60, %59 : i32
    %62 = arith.select %40, %61, %c0_i32_4 : i32
    %63 = arith.xori %44, %true_0 : i1
    %64 = arith.andi %43, %63 : i1
    %65 = arith.shrui %37, %c25_i32 : i32 loc(#loc14)
    %c5_i32 = arith.constant {acir.source_type = "u32"} 5 : i32
    %66 = arith.shli %65, %c5_i32 : i32
    %67 = arith.shrui %37, %c7_i32 : i32 loc(#loc15)
    %68 = arith.andi %67, %c31_i32 {acir.id = "v107", acir.source_type = "u32"} : i32 loc(#loc15)
    %69 = arith.ori %66, %68 {acir.id = "v108", acir.source_type = "u32"} : i32 loc(#loc15)
    %c2048_i32 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %70 = arith.xori %69, %c2048_i32 {acir.id = "v3", acir.source_type = "u32"} : i32
    %71 = arith.subi %70, %c2048_i32 {acir.id = "v5", acir.source_type = "u32"} : i32
    %72 = arith.shrui %37, %c20_i32 : i32 loc(#loc16)
    %c2048_i32_5 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %73 = arith.xori %72, %c2048_i32_5 {acir.id = "v3", acir.source_type = "u32"} : i32
    %74 = arith.subi %73, %c2048_i32_5 {acir.id = "v5", acir.source_type = "u32"} : i32
    %75 = arith.select %64, %71, %74 : i32
    %76 = arith.select %43, %75, %c0_i32_4 : i32
    %77 = arith.shrui %37, %c20_i32 : i32 loc(#loc17)
    %c2048_i32_6 = arith.constant {acir.source_type = "u32"} 2048 : i32
    %78 = arith.xori %77, %c2048_i32_6 {acir.id = "v3", acir.source_type = "u32"} : i32
    %79 = arith.subi %78, %c2048_i32_6 {acir.id = "v5", acir.source_type = "u32"} : i32
    %80 = arith.select %40, %76, %79 : i32
    %81 = arith.cmpi ne, %38, %c51_i32 : i32 loc(#loc18)
    %true_7 = arith.constant {acir.source_type = "bool"} true
    %82 = arith.select %40, %81, %true_7 : i1
    %83 = arith.select %43, %44, %true_7 : i1
    %84 = arith.select %40, %83, %true_7 : i1
    %85 = acir.aggregate(%36, %37, %50, %55, %57, %62, %80, %82, %84) {acir.id = "v155", acir.source_type = "src_isa_f0424bc8.Decoded"} : (i32, i32, i32, i32, i32, i32, i32, i1, i1) -> !acir.struct<"src_isa_f0424bc8.Decoded">
    %86 = acir.extract %85 {acir.id = "v5", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32
    %87 = acir.extract %32[%86] {acir.id = "v7", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1
    %88 = acir.unary %87 "not" {acir.id = "v8", acir.source_type = "bool"} : i1 -> i1
    %89 = arith.xori %88, %true_0 : i1
    %90 = arith.andi %30, %89 : i1
    %false_8 = arith.constant {acir.source_type = "bool"} false
    %91 = acir.extract %85 {acir.id = "v11", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc19)
    %92 = acir.extract %32[%91] {acir.id = "v13", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc19)
    %93 = acir.unary %92 "not" {acir.id = "v14", acir.source_type = "bool"} : i1 -> i1 loc(#loc19)
    %94 = arith.select %90, %false_8, %93 : i1 loc(#loc19)
    %95 = arith.xori %94, %true_0 : i1 loc(#loc19)
    %96 = arith.andi %30, %95 : i1 loc(#loc19)
    %97 = arith.andi %30, %94 : i1
    %98 = acir.extract %85 {acir.id = "v20", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc20)
    %99 = acir.unary %98 "not" {acir.id = "v21", acir.source_type = "bool"} : i1 -> i1 loc(#loc20)
    %100 = arith.xori %99, %true_0 : i1 loc(#loc20)
    %101 = arith.andi %97, %100 : i1 loc(#loc20)
    %102 = acir.extract %85 {acir.id = "v25", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc21)
    %103 = acir.extract %32[%102] {acir.id = "v27", acir.source_type = "bool", indexed = true} : (!acir.array<i1, 32>, i32) -> i1 loc(#loc21)
    %104 = acir.unary %103 "not" {acir.id = "v28", acir.source_type = "bool"} : i1 -> i1 loc(#loc21)
    %true_9 = arith.constant {acir.source_type = "bool"} true
    %105 = arith.select %101, %104, %true_9 : i1
    %106 = arith.select %96, %false_8, %105 : i1
    %107 = arith.select %31, %false_1, %106 : i1
    %108 = arith.xori %107, %true_0 : i1
    %109 = arith.andi %31, %true : i1
    %110 = arith.xori %30, %true_0 : i1
    %111 = arith.ori %110, %true : i1
    %112 = arith.xori %30, %true_0 : i1
    %113 = arith.xori %30, %true_0 : i1
    %114 = arith.xori %30, %true_0 : i1 loc(#loc22)
    %115 = arith.ori %114, %111 : i1 loc(#loc22)
    %116 = arith.ori %113, %115 : i1 loc(#loc22)
    %117 = arith.ori %112, %116 : i1 loc(#loc22)
    %118 = arith.xori %96, %true_0 : i1
    %119 = arith.xori %90, %true_0 : i1
    %120 = arith.andi %35, %35 : i1 loc(#loc23)
    %121 = arith.xori %40, %true_0 : i1
    %122 = arith.xori %40, %true_0 : i1
    %123 = arith.ori %122, %35 : i1
    %124 = arith.ori %121, %123 : i1
    %125 = arith.xori %43, %true_0 : i1
    %126 = arith.xori %43, %true_0 : i1
    %127 = arith.andi %123, %35 : i1
    %128 = arith.ori %126, %127 : i1
    %129 = arith.ori %125, %128 : i1
    %130 = arith.andi %129, %35 : i1
    %131 = arith.andi %43, %130 : i1
    %132 = arith.andi %40, %41 : i1
    %133 = arith.xori %132, %true_0 : i1
    %134 = arith.xori %132, %true_0 : i1
    %135 = arith.andi %123, %35 : i1
    %136 = arith.ori %134, %135 : i1
    %137 = arith.ori %133, %136 : i1
    %138 = arith.andi %137, %35 : i1
    %139 = arith.andi %132, %138 : i1
    %140 = arith.ori %131, %139 : i1
    %141 = arith.andi %124, %140 : i1
    %142 = arith.andi %40, %141 : i1
    %143 = arith.xori %39, %true_0 : i1
    %144 = arith.ori %143, %35 : i1
    %145 = arith.andi %39, %144 : i1
    %146 = arith.ori %142, %145 : i1
    %147 = arith.andi %120, %146 : i1
    %148 = arith.andi %35, %35 : i1
    %149 = arith.andi %129, %148 : i1
    %150 = arith.andi %43, %149 : i1
    %151 = arith.andi %137, %35 : i1
    %152 = arith.andi %132, %151 : i1
    %153 = arith.ori %150, %152 : i1
    %154 = arith.andi %124, %153 : i1
    %155 = arith.andi %40, %154 : i1
    %156 = arith.andi %144, %35 : i1
    %157 = arith.andi %39, %156 : i1
    %158 = arith.ori %155, %157 : i1
    %159 = arith.andi %147, %158 : i1
    %160 = arith.andi %159, %35 : i1
    %161 = arith.andi %35, %35 : i1
    %162 = arith.andi %129, %161 : i1
    %163 = arith.andi %43, %162 : i1
    %164 = arith.andi %137, %35 : i1
    %165 = arith.andi %132, %164 : i1
    %166 = arith.ori %163, %165 : i1
    %167 = arith.andi %124, %166 : i1
    %168 = arith.andi %40, %167 : i1
    %169 = arith.andi %39, %144 : i1
    %170 = arith.ori %168, %169 : i1
    %171 = arith.andi %160, %170 : i1
    %172 = arith.xori %64, %true_0 : i1
    %173 = arith.xori %64, %true_0 : i1
    %174 = arith.andi %128, %35 : i1
    %175 = arith.ori %173, %174 : i1
    %176 = arith.ori %172, %175 : i1
    %177 = arith.xori %64, %true_0 : i1 loc(#loc24)
    %178 = arith.ori %177, %176 : i1 loc(#loc24)
    %179 = arith.andi %35, %35 : i1 loc(#loc14)
    %180 = arith.andi %176, %179 : i1 loc(#loc14)
    %181 = arith.andi %64, %180 : i1 loc(#loc14)
    %182 = arith.andi %178, %181 : i1 loc(#loc14)
    %183 = arith.andi %64, %182 : i1 loc(#loc14)
    %184 = arith.andi %176, %183 : i1 loc(#loc14)
    %185 = arith.andi %64, %184 : i1 loc(#loc14)
    %186 = arith.andi %43, %44 : i1
    %187 = arith.xori %186, %true_0 : i1
    %188 = arith.andi %128, %35 : i1
    %189 = arith.ori %187, %188 : i1
    %190 = arith.xori %186, %true_0 : i1 loc(#loc25)
    %191 = arith.ori %190, %189 : i1 loc(#loc25)
    %192 = arith.andi %189, %35 : i1 loc(#loc25)
    %193 = arith.andi %186, %192 : i1 loc(#loc25)
    %194 = arith.andi %191, %193 : i1 loc(#loc25)
    %195 = arith.andi %186, %194 : i1 loc(#loc25)
    %196 = arith.andi %189, %195 : i1 loc(#loc25)
    %197 = arith.andi %186, %196 : i1 loc(#loc25)
    %198 = arith.ori %185, %197 : i1 loc(#loc25)
    %199 = arith.andi %129, %198 : i1 loc(#loc25)
    %200 = arith.andi %43, %199 : i1 loc(#loc25)
    %201 = arith.andi %132, %137 : i1
    %202 = arith.ori %200, %201 : i1
    %203 = arith.andi %124, %202 : i1
    %204 = arith.andi %40, %203 : i1
    %205 = arith.xori %39, %true_0 : i1 loc(#loc26)
    %206 = arith.ori %205, %144 : i1 loc(#loc26)
    %207 = arith.andi %144, %35 : i1 loc(#loc26)
    %208 = arith.andi %39, %207 : i1 loc(#loc26)
    %209 = arith.andi %206, %208 : i1 loc(#loc26)
    %210 = arith.andi %39, %209 : i1 loc(#loc26)
    %211 = arith.andi %144, %210 : i1 loc(#loc26)
    %212 = arith.andi %39, %211 : i1 loc(#loc26)
    %213 = arith.ori %204, %212 : i1 loc(#loc26)
    %214 = arith.andi %171, %213 : i1 loc(#loc26)
    %215 = arith.andi %124, %35 : i1
    %216 = arith.andi %40, %215 : i1
    %217 = arith.andi %39, %144 : i1
    %218 = arith.ori %216, %217 : i1
    %219 = arith.andi %214, %218 : i1
    %220 = arith.andi %129, %35 : i1
    %221 = arith.andi %43, %220 : i1
    %222 = arith.andi %132, %137 : i1
    %223 = arith.ori %221, %222 : i1
    %224 = arith.andi %124, %223 : i1
    %225 = arith.andi %40, %224 : i1
    %226 = arith.andi %39, %144 : i1
    %227 = arith.ori %225, %226 : i1
    %228 = arith.andi %219, %227 : i1
    %229 = arith.xori %64, %true_0 : i1 loc(#loc27)
    %c35_i32 = arith.constant {acir.source_type = "u32"} 35 : i32
    %230 = arith.cmpi eq, %38, %c35_i32 {acir.id = "v89", acir.source_type = "bool"} : i32
    %231 = arith.xori %230, %true_0 : i1
    %232 = arith.andi %64, %231 : i1
    %233 = arith.xori %232, %true_0 : i1
    %234 = arith.andi %175, %35 : i1
    %235 = arith.ori %233, %234 : i1
    %236 = arith.andi %232, %235 : i1
    %237 = arith.andi %64, %230 : i1
    %238 = arith.xori %237, %true_0 : i1
    %239 = arith.andi %175, %35 : i1
    %240 = arith.ori %238, %239 : i1
    %241 = arith.andi %240, %35 : i1
    %242 = arith.andi %237, %241 : i1
    %243 = arith.ori %236, %242 : i1
    %false_10 = arith.constant {acir.source_type = "bool"} false
    %c12_i32 = arith.constant {acir.source_type = "u32"} 12 : i32 loc(#loc28)
    %244 = arith.shrui %37, %c12_i32 : i32 loc(#loc28)
    %245 = arith.andi %244, %c7_i32 {acir.id = "v8", acir.source_type = "u32"} : i32 loc(#loc28)
    %246 = arith.cmpi eq, %245, %c2_i32 {acir.id = "v92", acir.source_type = "bool"} : i32 loc(#loc28)
    %247 = arith.select %232, %false_10, %246 : i1 loc(#loc28)
    %248 = acir.assert %247 loc(#loc28)
    %249 = arith.andi %243, %248 : i1 loc(#loc28)
    %250 = arith.ori %229, %249 : i1 loc(#loc28)
    %251 = arith.xori %186, %true_0 : i1 loc(#loc29)
    %252 = arith.cmpi eq, %245, %c2_i32 {acir.id = "v80", acir.source_type = "bool"} : i32 loc(#loc30)
    %253 = acir.assert %252 loc(#loc30)
    %254 = arith.andi %35, %253 : i1 loc(#loc30)
    %255 = arith.ori %251, %254 : i1 loc(#loc30)
    %256 = arith.andi %250, %255 : i1 loc(#loc30)
    %257 = arith.xori %132, %true_0 : i1 loc(#loc31)
    %258 = arith.cmpi eq, %245, %c0_i32_4 {acir.id = "v42", acir.source_type = "bool"} : i32 loc(#loc32)
    %259 = arith.xori %258, %true_0 : i1 loc(#loc32)
    %260 = arith.andi %132, %259 : i1 loc(#loc32)
    %261 = arith.xori %260, %true_0 : i1
    %262 = arith.andi %136, %35 : i1
    %263 = arith.ori %261, %262 : i1
    %264 = arith.andi %260, %263 : i1
    %265 = arith.andi %132, %258 : i1
    %266 = arith.xori %265, %true_0 : i1
    %267 = arith.xori %265, %true_0 : i1
    %268 = arith.andi %136, %35 : i1
    %269 = arith.ori %267, %268 : i1
    %270 = arith.ori %266, %269 : i1
    %271 = arith.cmpi eq, %46, %c0_i32_4 {acir.id = "v45", acir.source_type = "bool"} : i32 loc(#loc33)
    %272 = arith.xori %271, %true_0 : i1 loc(#loc33)
    %273 = arith.andi %265, %272 : i1 loc(#loc33)
    %274 = arith.xori %273, %true_0 : i1
    %275 = arith.andi %269, %35 : i1
    %276 = arith.ori %274, %275 : i1
    %277 = arith.andi %276, %35 : i1
    %278 = arith.andi %273, %277 : i1
    %279 = arith.andi %265, %271 : i1
    %280 = arith.xori %279, %true_0 : i1
    %281 = arith.andi %269, %35 : i1
    %282 = arith.ori %280, %281 : i1
    %283 = arith.andi %279, %282 : i1
    %284 = arith.ori %278, %283 : i1
    %285 = arith.andi %270, %284 : i1
    %286 = arith.andi %265, %285 : i1
    %287 = arith.ori %264, %286 : i1
    %288 = arith.cmpi eq, %46, %c32_i32 {acir.id = "v49", acir.source_type = "bool"} : i32 loc(#loc34)
    %289 = arith.select %273, %288, %true_7 : i1 loc(#loc34)
    %290 = arith.select %260, %false_10, %289 : i1 loc(#loc34)
    %291 = acir.assert %290 loc(#loc34)
    %292 = arith.andi %287, %291 : i1 loc(#loc34)
    %293 = arith.ori %257, %292 : i1 loc(#loc34)
    %294 = arith.andi %256, %293 : i1 loc(#loc34)
    %295 = arith.xori %39, %true_0 : i1 loc(#loc35)
    %296 = arith.cmpi eq, %245, %c0_i32_4 {acir.id = "v31", acir.source_type = "bool"} : i32 loc(#loc36)
    %297 = acir.assert %296 loc(#loc36)
    %298 = arith.andi %35, %297 : i1 loc(#loc36)
    %299 = arith.ori %295, %298 : i1 loc(#loc36)
    %300 = arith.andi %294, %299 : i1 loc(#loc36)
    %301 = arith.andi %228, %300 : i1 loc(#loc36)
    %302 = arith.andi %111, %301 : i1 loc(#loc36)
    %303 = arith.andi %30, %302 : i1 loc(#loc36)
    %304 = arith.andi %true, %303 : i1 loc(#loc36)
    %305 = arith.andi %115, %304 : i1 loc(#loc36)
    %306 = arith.ori %119, %305 : i1 loc(#loc36)
    %307 = arith.andi %90, %306 : i1 loc(#loc36)
    %308 = arith.andi %30, %88 : i1
    %309 = arith.xori %308, %true_0 : i1
    %310 = arith.andi %115, %304 : i1
    %311 = arith.ori %309, %310 : i1
    %312 = arith.andi %true, %303 : i1 loc(#loc37)
    %313 = arith.andi %311, %312 : i1 loc(#loc37)
    %314 = arith.andi %308, %313 : i1 loc(#loc37)
    %315 = arith.ori %307, %314 : i1 loc(#loc37)
    %316 = arith.andi %116, %315 : i1 loc(#loc37)
    %317 = arith.ori %118, %316 : i1 loc(#loc37)
    %318 = arith.andi %96, %317 : i1 loc(#loc37)
    %319 = arith.xori %97, %true_0 : i1
    %320 = arith.xori %97, %true_0 : i1
    %321 = arith.andi %116, %315 : i1
    %322 = arith.ori %320, %321 : i1
    %323 = arith.ori %319, %322 : i1
    %324 = arith.xori %101, %true_0 : i1
    %325 = arith.andi %322, %303 : i1
    %326 = arith.ori %324, %325 : i1
    %327 = arith.andi %true, %303 : i1 loc(#loc38)
    %328 = arith.andi %326, %327 : i1 loc(#loc38)
    %329 = arith.andi %101, %328 : i1 loc(#loc38)
    %330 = arith.andi %97, %99 : i1
    %331 = arith.xori %330, %true_0 : i1
    %332 = arith.andi %322, %303 : i1
    %333 = arith.ori %331, %332 : i1
    %334 = arith.andi %330, %333 : i1
    %335 = arith.ori %329, %334 : i1
    %336 = arith.andi %323, %335 : i1
    %337 = arith.andi %97, %336 : i1
    %338 = arith.ori %318, %337 : i1
    %339 = arith.andi %117, %338 : i1
    %340 = arith.andi %30, %339 : i1
    %341 = arith.andi %111, %340 : i1
    %342 = arith.andi %30, %341 : i1
    %343 = arith.ori %109, %342 : i1
    %344 = arith.ori %108, %343 : i1
    %345 = arith.andi %344, %301 : i1
    %346 = arith.andi %107, %345 : i1
    %347 = arith.xori %107, %true_0 : i1 loc(#loc39)
    %348 = arith.xori %107, %true_0 : i1 loc(#loc40)
    %349 = arith.ori %348, %344 : i1 loc(#loc40)
    %c0_i32_11 = arith.constant 0 : i32 loc(#loc39)
    %350 = acir.extract %33[%c0_i32_11] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc39)
    %c0_i32_12 = arith.constant 0 : i32 loc(#loc39)
    %351 = arith.cmpi ne, %28, %c0_i32_12 : i32 loc(#loc39)
    %352 = arith.andi %349, %351 : i1 loc(#loc39)
    %353 = arith.ori %347, %352 : i1 loc(#loc39)
    %354 = arith.andi %346, %353 : i1 loc(#loc39)
    %355 = arith.xori %107, %true_0 : i1 loc(#loc39)
    %356 = arith.andi %349, %351 : i1 loc(#loc39)
    %357 = arith.ori %355, %356 : i1 loc(#loc39)
    %358 = arith.andi %354, %357 : i1 loc(#loc39)
    %359 = arith.xori %107, %true_0 : i1 loc(#loc39)
    %c0_i32_13 = arith.constant 0 : i32 loc(#loc39)
    %360 = arith.cmpi eq, %28, %c0_i32_13 : i32 loc(#loc39)
    %361 = arith.andi %349, %true : i1 loc(#loc39)
    %362 = arith.ori %359, %361 : i1 loc(#loc39)
    %363 = arith.andi %358, %362 : i1 loc(#loc39)
    %364 = arith.xori %107, %true_0 : i1 loc(#loc39)
    %365 = arith.xori %360, %true_0 : i1 loc(#loc39)
    %366 = arith.andi %349, %365 : i1 loc(#loc39)
    %367 = arith.ori %364, %366 : i1 loc(#loc39)
    %368 = arith.andi %363, %367 : i1 loc(#loc39)
    %369 = arith.xori %107, %true_0 : i1
    %370 = arith.xori %107, %true_0 : i1 loc(#loc41)
    %371 = arith.ori %370, %349 : i1 loc(#loc41)
    %372 = arith.andi %344, %301 : i1 loc(#loc40)
    %373 = arith.andi %107, %372 : i1 loc(#loc40)
    %374 = arith.andi %349, %373 : i1 loc(#loc40)
    %375 = arith.andi %107, %374 : i1 loc(#loc40)
    %376 = arith.andi %371, %375 : i1 loc(#loc40)
    %377 = arith.ori %369, %376 : i1 loc(#loc40)
    %378 = arith.andi %368, %377 : i1 loc(#loc40)
    %379 = acir.extract %85 {acir.id = "v6", acir.source_type = "u32", field = "rs1"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc42)
    %c0_i32_14 = arith.constant {acir.source_type = "u32"} 0 : i32
    %380 = arith.cmpi eq, %379, %c0_i32_14 {acir.id = "v5", acir.source_type = "bool"} : i32
    %381 = arith.xori %380, %true_0 : i1
    %382 = arith.andi %107, %381 : i1
    %383 = arith.xori %382, %true_0 : i1
    %384 = arith.xori %382, %true_0 : i1
    %385 = arith.andi %371, %375 : i1
    %386 = arith.ori %384, %385 : i1
    %387 = acir.current {aspect = "value", resource = "top.rf.registers"} : !acir.array<i32, 32> loc(#loc43)
    %388 = arith.andi %386, %true : i1 loc(#loc43)
    %389 = arith.ori %383, %388 : i1 loc(#loc43)
    %390 = arith.andi %378, %389 : i1 loc(#loc43)
    %391 = arith.xori %107, %true_0 : i1
    %392 = arith.xori %107, %true_0 : i1 loc(#loc44)
    %393 = arith.ori %392, %349 : i1 loc(#loc44)
    %394 = arith.andi %349, %373 : i1 loc(#loc44)
    %395 = arith.andi %107, %394 : i1 loc(#loc44)
    %396 = arith.andi %393, %395 : i1 loc(#loc44)
    %397 = arith.ori %391, %396 : i1 loc(#loc44)
    %398 = arith.andi %390, %397 : i1 loc(#loc44)
    %399 = acir.extract %85 {acir.id = "v8", acir.source_type = "u32", field = "rs2"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc45)
    %c0_i32_15 = arith.constant {acir.source_type = "u32"} 0 : i32
    %400 = arith.cmpi eq, %399, %c0_i32_15 {acir.id = "v5", acir.source_type = "bool"} : i32
    %401 = arith.xori %400, %true_0 : i1
    %402 = arith.andi %107, %401 : i1
    %403 = arith.xori %402, %true_0 : i1
    %404 = arith.xori %402, %true_0 : i1
    %405 = arith.andi %393, %395 : i1
    %406 = arith.ori %404, %405 : i1
    %407 = arith.andi %406, %true : i1 loc(#loc43)
    %408 = arith.ori %403, %407 : i1 loc(#loc43)
    %409 = arith.andi %398, %408 : i1 loc(#loc43)
    %410 = arith.xori %107, %true_0 : i1
    %411 = arith.andi %349, %373 : i1
    %412 = arith.ori %410, %411 : i1
    %413 = arith.andi %409, %412 : i1
    %414 = arith.xori %107, %true_0 : i1
    %415 = arith.xori %107, %true_0 : i1 loc(#loc46)
    %416 = arith.xori %107, %true_0 : i1
    %417 = arith.ori %416, %349 : i1
    %418 = arith.ori %415, %417 : i1
    %419 = arith.andi %417, %373 : i1 loc(#loc46)
    %420 = arith.andi %107, %419 : i1 loc(#loc46)
    %421 = arith.andi %418, %420 : i1 loc(#loc46)
    %422 = arith.ori %414, %421 : i1 loc(#loc46)
    %423 = arith.andi %413, %422 : i1 loc(#loc46)
    %424 = arith.xori %107, %true_0 : i1
    %425 = arith.xori %107, %true_0 : i1
    %426 = arith.ori %425, %418 : i1
    %427 = acir.extract %85 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc47)
    %428 = arith.xori %427, %true_0 : i1 loc(#loc47)
    %429 = arith.andi %107, %428 : i1 loc(#loc47)
    %430 = arith.xori %429, %true_0 : i1
    %431 = arith.andi %418, %420 : i1
    %432 = arith.ori %430, %431 : i1
    %433 = arith.andi %429, %432 : i1
    %434 = arith.andi %107, %427 : i1
    %435 = arith.xori %434, %true_0 : i1
    %436 = arith.andi %418, %420 : i1
    %437 = arith.ori %435, %436 : i1
    %438 = arith.andi %437, %420 : i1
    %439 = arith.andi %434, %438 : i1
    %440 = arith.ori %433, %439 : i1
    %441 = arith.andi %426, %440 : i1
    %442 = arith.ori %424, %441 : i1
    %443 = arith.andi %423, %442 : i1
    %false_16 = arith.constant {acir.source_type = "bool"} false
    %444 = acir.extract %85 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc48)
    %c0_i32_17 = arith.constant {acir.source_type = "u32"} 0 : i32
    %445 = arith.cmpi ne, %444, %c0_i32_17 {acir.id = "v8", acir.source_type = "bool"} : i32
    %446 = arith.select %429, %false_16, %445 : i1
    %447 = arith.andi %107, %446 : i1
    %448 = arith.xori %447, %true_0 : i1
    %449 = arith.xori %447, %true_0 : i1
    %450 = arith.andi %426, %440 : i1
    %451 = arith.ori %449, %450 : i1
    %452 = arith.andi %451, %true : i1 loc(#loc49)
    %453 = arith.ori %448, %452 : i1 loc(#loc49)
    %454 = arith.andi %443, %453 : i1 loc(#loc49)
    %455 = arith.xori %447, %true_0 : i1 loc(#loc49)
    %456 = arith.andi %451, %true : i1 loc(#loc49)
    %457 = arith.ori %455, %456 : i1 loc(#loc49)
    %458 = arith.andi %454, %457 : i1 loc(#loc49)
    %459 = arith.xori %447, %true_0 : i1 loc(#loc49)
    %460 = arith.andi %true, %420 : i1 loc(#loc49)
    %461 = arith.andi %451, %460 : i1 loc(#loc49)
    %462 = arith.ori %459, %461 : i1 loc(#loc49)
    %463 = arith.andi %458, %462 : i1 loc(#loc49)
    %464 = arith.andi %373, %373 : i1 loc(#loc50)
    %465 = arith.andi %464, %373 : i1 loc(#loc50)
    %466 = arith.andi %465, %373 : i1 loc(#loc50)
    %467 = arith.andi %385, %375 : i1
    %468 = arith.andi %382, %467 : i1
    %469 = arith.andi %true, %468 : i1
    %470 = arith.andi %386, %469 : i1
    %471 = arith.andi %382, %470 : i1
    %472 = arith.andi %107, %380 : i1
    %473 = arith.xori %472, %true_0 : i1 loc(#loc51)
    %474 = arith.andi %371, %375 : i1
    %475 = arith.ori %473, %474 : i1
    %476 = arith.andi %472, %475 : i1
    %477 = arith.ori %471, %476 : i1
    %478 = arith.andi %466, %477 : i1
    %479 = acir.extract %85 {acir.id = "v10", acir.source_type = "bool", field = "immediate_rhs"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc52)
    %480 = arith.xori %479, %true_0 : i1 loc(#loc52)
    %481 = arith.andi %107, %480 : i1 loc(#loc52)
    %482 = arith.xori %481, %true_0 : i1
    %483 = arith.andi %349, %373 : i1
    %484 = arith.ori %482, %483 : i1
    %485 = arith.andi %405, %395 : i1
    %486 = arith.andi %402, %485 : i1
    %487 = arith.andi %true, %486 : i1
    %488 = arith.andi %406, %487 : i1
    %489 = arith.andi %402, %488 : i1
    %490 = arith.andi %107, %400 : i1
    %491 = arith.xori %490, %true_0 : i1 loc(#loc51)
    %492 = arith.andi %393, %395 : i1
    %493 = arith.ori %491, %492 : i1
    %494 = arith.andi %490, %493 : i1
    %495 = arith.ori %489, %494 : i1
    %496 = arith.andi %484, %495 : i1
    %497 = arith.andi %481, %496 : i1
    %498 = arith.andi %107, %479 : i1
    %499 = arith.xori %498, %true_0 : i1
    %500 = arith.andi %349, %373 : i1
    %501 = arith.ori %499, %500 : i1
    %502 = arith.andi %501, %373 : i1
    %503 = arith.andi %498, %502 : i1
    %504 = arith.ori %497, %503 : i1
    %505 = arith.andi %478, %504 : i1
    %506 = arith.andi %505, %495 : i1
    %507 = arith.andi %506, %373 : i1
    %508 = arith.andi %417, %507 : i1
    %509 = arith.andi %107, %508 : i1
    %510 = arith.andi %107, %509 : i1
    %511 = arith.xori %510, %true_0 : i1
    %512 = arith.ori %511, %344 : i1
    %513 = arith.andi %463, %512 : i1
    %514 = arith.xori %510, %true_0 : i1 loc(#loc53)
    %515 = acir.current {aspect = "count", resource = "top.id_ex"} : i32 loc(#loc53)
    %516 = acir.current {aspect = "storage", resource = "top.id_ex"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1> loc(#loc54)
    %c0_i32_18 = arith.constant 0 : i32 loc(#loc54)
    %517 = acir.extract %516[%c0_i32_18] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc54)
    %c0_i32_19 = arith.constant 0 : i32 loc(#loc54)
    %518 = arith.cmpi ne, %515, %c0_i32_19 : i32 loc(#loc54)
    %519 = arith.andi %518, %518 : i1 loc(#loc54)
    %c0_i32_20 = arith.constant 0 : i32 loc(#loc54)
    %520 = arith.cmpi eq, %515, %c0_i32_20 : i32 loc(#loc54)
    %521 = arith.andi %519, %true : i1 loc(#loc54)
    %522 = arith.xori %520, %true_0 : i1 loc(#loc54)
    %523 = arith.andi %521, %522 : i1 loc(#loc54)
    %524 = arith.andi %523, %518 : i1 loc(#loc54)
    %525 = arith.andi %518, %518 : i1 loc(#loc55)
    %526 = arith.andi %525, %518 : i1 loc(#loc55)
    %527 = arith.andi %526, %518 : i1 loc(#loc55)
    %528 = acir.extract %517 {acir.id = "v5", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc56)
    %c1_i32 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc57)
    %529 = arith.cmpi eq, %528, %c1_i32 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc57)
    %530 = arith.xori %529, %true_0 : i1 loc(#loc57)
    %531 = arith.xori %530, %true_0 : i1
    %532 = arith.ori %531, %518 : i1
    %533 = arith.andi %518, %518 : i1 loc(#loc58)
    %534 = arith.andi %532, %533 : i1 loc(#loc58)
    %535 = arith.andi %530, %534 : i1 loc(#loc58)
    %536 = arith.xori %529, %true_0 : i1
    %537 = arith.ori %536, %518 : i1
    %538 = arith.andi %518, %518 : i1 loc(#loc59)
    %539 = arith.andi %537, %538 : i1 loc(#loc59)
    %540 = arith.andi %529, %539 : i1 loc(#loc59)
    %541 = arith.ori %535, %540 : i1 loc(#loc59)
    %542 = arith.andi %527, %541 : i1 loc(#loc59)
    %543 = arith.andi %542, %518 : i1 loc(#loc59)
    %544 = arith.andi %543, %518 : i1 loc(#loc59)
    %545 = arith.xori %544, %true_0 : i1 loc(#loc59)
    %546 = acir.current {aspect = "count", resource = "top.ex_mem"} : i32 loc(#loc60)
    %547 = acir.current {aspect = "storage", resource = "top.ex_mem"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1> loc(#loc61)
    %c0_i32_21 = arith.constant 0 : i32 loc(#loc61)
    %548 = acir.extract %547[%c0_i32_21] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc61)
    %c0_i32_22 = arith.constant 0 : i32 loc(#loc61)
    %549 = arith.cmpi ne, %546, %c0_i32_22 : i32 loc(#loc61)
    %550 = arith.andi %549, %549 : i1 loc(#loc61)
    %c0_i32_23 = arith.constant 0 : i32 loc(#loc61)
    %551 = arith.cmpi eq, %546, %c0_i32_23 : i32 loc(#loc61)
    %552 = arith.andi %550, %true : i1 loc(#loc61)
    %553 = arith.xori %551, %true_0 : i1 loc(#loc61)
    %554 = arith.andi %552, %553 : i1 loc(#loc61)
    %555 = arith.andi %554, %549 : i1 loc(#loc61)
    %556 = acir.extract %548 {acir.id = "v6", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc62)
    %c2_i32_24 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc63)
    %557 = arith.cmpi eq, %556, %c2_i32_24 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc63)
    %558 = arith.xori %557, %true_0 : i1 loc(#loc63)
    %559 = arith.xori %558, %true_0 : i1 loc(#loc63)
    %560 = arith.xori %558, %true_0 : i1
    %561 = arith.ori %560, %549 : i1
    %562 = arith.andi %561, %549 : i1
    %563 = arith.ori %559, %562 : i1
    %564 = arith.andi %555, %563 : i1
    %565 = acir.extract %548 {acir.id = "v12", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc64)
    %c3_i32_25 = arith.constant {acir.source_type = "u32"} 3 : i32
    %566 = arith.cmpi eq, %565, %c3_i32_25 {acir.id = "v14", acir.source_type = "bool"} : i32
    %567 = arith.andi %558, %566 : i1
    %568 = arith.xori %567, %true_0 : i1
    %569 = arith.xori %567, %true_0 : i1 loc(#loc65)
    %570 = arith.xori %567, %true_0 : i1
    %571 = arith.andi %561, %549 : i1
    %572 = arith.ori %570, %571 : i1
    %573 = arith.ori %569, %572 : i1
    %574 = arith.andi %572, %549 : i1 loc(#loc65)
    %575 = arith.andi %567, %574 : i1 loc(#loc65)
    %576 = arith.andi %573, %575 : i1 loc(#loc65)
    %577 = arith.ori %568, %576 : i1 loc(#loc65)
    %578 = arith.andi %564, %577 : i1 loc(#loc65)
    %579 = arith.xori %567, %true_0 : i1 loc(#loc66)
    %580 = arith.xori %567, %true_0 : i1
    %581 = arith.ori %580, %573 : i1
    %582 = acir.extract %548 {acir.id = "v16", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc67)
    %c3_i32_26 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc68)
    %583 = arith.andi %582, %c3_i32_26 {acir.id = "v6", acir.source_type = "u32"} : i32 loc(#loc68)
    %c0_i32_27 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc69)
    %584 = arith.cmpi eq, %583, %c0_i32_27 {acir.id = "v8", acir.source_type = "bool"} : i32 loc(#loc69)
    %585 = arith.xori %584, %true_0 : i1 loc(#loc69)
    %586 = arith.andi %567, %585 : i1 loc(#loc69)
    %587 = arith.xori %586, %true_0 : i1
    %588 = arith.andi %573, %575 : i1
    %589 = arith.ori %587, %588 : i1
    %590 = arith.andi %586, %589 : i1
    %591 = arith.andi %567, %584 : i1
    %592 = arith.xori %591, %true_0 : i1
    %593 = arith.andi %573, %575 : i1
    %594 = arith.ori %592, %593 : i1
    %595 = arith.andi %594, %575 : i1
    %596 = arith.andi %591, %595 : i1
    %597 = arith.ori %590, %596 : i1
    %598 = arith.andi %581, %597 : i1
    %599 = arith.ori %579, %598 : i1
    %600 = arith.andi %578, %599 : i1
    %601 = arith.xori %567, %true_0 : i1 loc(#loc70)
    %602 = acir.current {aspect = "value", resource = "top.memory.words"} : !acir.array<i32, 16> loc(#loc70)
    %603 = arith.andi %581, %true : i1 loc(#loc70)
    %604 = arith.ori %601, %603 : i1 loc(#loc70)
    %605 = arith.andi %600, %604 : i1 loc(#loc70)
    %606 = arith.xori %567, %true_0 : i1 loc(#loc70)
    %607 = arith.andi %581, %true : i1 loc(#loc70)
    %608 = arith.ori %606, %607 : i1 loc(#loc70)
    %609 = arith.andi %605, %608 : i1 loc(#loc70)
    %610 = arith.xori %567, %true_0 : i1 loc(#loc70)
    %611 = arith.andi %572, %549 : i1 loc(#loc65)
    %612 = arith.andi %567, %611 : i1 loc(#loc65)
    %613 = arith.andi %true, %612 : i1 loc(#loc65)
    %614 = arith.andi %613, %575 : i1 loc(#loc65)
    %615 = arith.andi %581, %614 : i1 loc(#loc65)
    %616 = arith.ori %610, %615 : i1 loc(#loc65)
    %617 = arith.andi %609, %616 : i1 loc(#loc65)
    %618 = arith.xori %557, %true_0 : i1
    %619 = arith.xori %557, %true_0 : i1 loc(#loc71)
    %620 = arith.xori %557, %true_0 : i1
    %621 = arith.ori %620, %549 : i1
    %622 = arith.ori %619, %621 : i1
    %623 = arith.andi %621, %549 : i1 loc(#loc71)
    %624 = arith.andi %557, %623 : i1 loc(#loc71)
    %625 = arith.andi %622, %624 : i1 loc(#loc71)
    %626 = arith.ori %618, %625 : i1 loc(#loc71)
    %627 = arith.andi %617, %626 : i1 loc(#loc71)
    %628 = arith.xori %557, %true_0 : i1 loc(#loc72)
    %629 = arith.xori %557, %true_0 : i1
    %630 = arith.ori %629, %622 : i1
    %631 = acir.extract %548 {acir.id = "v10", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc73)
    %c3_i32_28 = arith.constant {acir.source_type = "u32"} 3 : i32 loc(#loc74)
    %632 = arith.andi %631, %c3_i32_28 {acir.id = "v5", acir.source_type = "u32"} : i32 loc(#loc74)
    %c0_i32_29 = arith.constant {acir.source_type = "u32"} 0 : i32 loc(#loc75)
    %633 = arith.cmpi eq, %632, %c0_i32_29 {acir.id = "v7", acir.source_type = "bool"} : i32 loc(#loc75)
    %634 = arith.xori %633, %true_0 : i1 loc(#loc75)
    %635 = arith.andi %557, %634 : i1 loc(#loc75)
    %636 = arith.xori %635, %true_0 : i1
    %637 = arith.andi %622, %624 : i1
    %638 = arith.ori %636, %637 : i1
    %639 = arith.andi %635, %638 : i1
    %640 = arith.andi %557, %633 : i1
    %641 = arith.xori %640, %true_0 : i1
    %642 = arith.andi %622, %624 : i1
    %643 = arith.ori %641, %642 : i1
    %644 = arith.andi %643, %624 : i1
    %645 = arith.andi %640, %644 : i1
    %646 = arith.ori %639, %645 : i1
    %647 = arith.andi %630, %646 : i1
    %648 = arith.ori %628, %647 : i1
    %649 = arith.andi %627, %648 : i1
    %650 = arith.xori %557, %true_0 : i1 loc(#loc76)
    %651 = arith.andi %630, %true : i1 loc(#loc76)
    %652 = arith.ori %650, %651 : i1 loc(#loc76)
    %653 = arith.andi %649, %652 : i1 loc(#loc76)
    %654 = arith.andi %549, %549 : i1 loc(#loc77)
    %655 = arith.andi %654, %549 : i1 loc(#loc77)
    %656 = arith.andi %561, %549 : i1
    %657 = arith.andi %558, %656 : i1
    %658 = arith.andi %true, %624 : i1 loc(#loc76)
    %659 = arith.andi %630, %658 : i1 loc(#loc76)
    %660 = arith.andi %557, %659 : i1 loc(#loc76)
    %661 = arith.andi %621, %660 : i1 loc(#loc76)
    %662 = arith.andi %557, %661 : i1 loc(#loc76)
    %663 = arith.ori %657, %662 : i1 loc(#loc76)
    %664 = arith.andi %655, %663 : i1 loc(#loc76)
    %665 = arith.andi %664, %549 : i1 loc(#loc76)
    %666 = arith.xori %665, %true_0 : i1 loc(#loc76)
    %667 = acir.current {aspect = "storage", resource = "top.mem_wb"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1> loc(#loc78)
    %c0_i32_30 = arith.constant 0 : i32 loc(#loc78)
    %668 = acir.extract %667[%c0_i32_30] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc78)
    %c0_i32_31 = arith.constant 0 : i32 loc(#loc78)
    %669 = arith.cmpi ne, %29, %c0_i32_31 : i32 loc(#loc78)
    %670 = arith.andi %669, %669 : i1 loc(#loc78)
    %c0_i32_32 = arith.constant 0 : i32 loc(#loc78)
    %671 = arith.cmpi eq, %29, %c0_i32_32 : i32 loc(#loc78)
    %672 = arith.andi %670, %true : i1 loc(#loc78)
    %673 = arith.xori %671, %true_0 : i1 loc(#loc78)
    %674 = arith.andi %672, %673 : i1 loc(#loc78)
    %675 = arith.andi %674, %669 : i1 loc(#loc78)
    %676 = acir.extract %668 {acir.id = "v4", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i1 loc(#loc79)
    %677 = arith.xori %676, %true_0 : i1 loc(#loc79)
    %678 = arith.xori %677, %true_0 : i1
    %679 = arith.ori %678, %669 : i1
    %680 = arith.andi %677, %679 : i1
    %681 = arith.xori %676, %true_0 : i1
    %682 = arith.ori %681, %669 : i1
    %683 = arith.andi %682, %669 : i1
    %684 = arith.andi %676, %683 : i1
    %685 = arith.ori %680, %684 : i1
    %686 = arith.andi %675, %685 : i1
    %false_33 = arith.constant {acir.source_type = "bool"} false
    %687 = acir.extract %668 {acir.id = "v6", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc80)
    %c0_i32_34 = arith.constant {acir.source_type = "u32"} 0 : i32
    %688 = arith.cmpi ne, %687, %c0_i32_34 {acir.id = "v8", acir.source_type = "bool"} : i32
    %689 = arith.select %677, %false_33, %688 : i1
    %690 = arith.xori %689, %true_0 : i1
    %691 = arith.xori %689, %true_0 : i1
    %692 = arith.ori %691, %685 : i1
    %693 = arith.andi %692, %true : i1 loc(#loc81)
    %694 = arith.ori %690, %693 : i1 loc(#loc81)
    %695 = arith.andi %686, %694 : i1 loc(#loc81)
    %696 = arith.xori %689, %true_0 : i1 loc(#loc81)
    %697 = arith.andi %692, %true : i1 loc(#loc81)
    %698 = arith.ori %696, %697 : i1 loc(#loc81)
    %699 = arith.andi %695, %698 : i1 loc(#loc81)
    %700 = arith.xori %689, %true_0 : i1 loc(#loc81)
    %701 = arith.andi %true, %669 : i1 loc(#loc81)
    %702 = arith.andi %701, %669 : i1 loc(#loc81)
    %703 = arith.andi %692, %702 : i1 loc(#loc81)
    %704 = arith.ori %700, %703 : i1 loc(#loc81)
    %705 = arith.andi %699, %704 : i1 loc(#loc81)
    %706 = arith.xori %689, %true_0 : i1 loc(#loc82)
    %707 = arith.andi %692, %true : i1 loc(#loc82)
    %708 = arith.ori %706, %707 : i1 loc(#loc82)
    %709 = arith.andi %705, %708 : i1 loc(#loc82)
    %710 = arith.xori %689, %true_0 : i1 loc(#loc82)
    %711 = arith.andi %692, %true : i1 loc(#loc82)
    %712 = arith.ori %710, %711 : i1 loc(#loc82)
    %713 = arith.andi %709, %712 : i1 loc(#loc82)
    %714 = arith.xori %689, %true_0 : i1 loc(#loc82)
    %715 = arith.andi %true, %669 : i1 loc(#loc82)
    %716 = arith.andi %692, %715 : i1 loc(#loc82)
    %717 = arith.ori %714, %716 : i1 loc(#loc82)
    %718 = arith.andi %713, %717 : i1 loc(#loc82)
    %719 = acir.current {aspect = "value", resource = "top.writeback.retired"} : i32 loc(#loc83)
    %720 = arith.andi %718, %true : i1 loc(#loc83)
    %721 = arith.andi %720, %true : i1 loc(#loc83)
    %722 = arith.andi %721, %669 : i1 loc(#loc83)
    %723 = acir.fire "top.writeback.commit#4" %722 {successors = [3], txn = 4 : i64} loc(#loc83)
    %c0_i32_35 = arith.constant 0 : i32 loc(#loc83)
    %c1_i32_36 = arith.constant 1 : i32 loc(#loc83)
    %c0_i32_37 = arith.constant 0 : i32 loc(#loc83)
    %724 = arith.select %723, %c1_i32_36, %c0_i32_37 : i32 loc(#loc83)
    %725 = arith.addi %c0_i32_35, %724 : i32 loc(#loc83)
    %726 = arith.subi %29, %725 : i32 loc(#loc84)
    %c1_i32_38 = arith.constant 1 : i32 loc(#loc83)
    %727 = arith.cmpi uge, %726, %c1_i32_38 : i32 loc(#loc83)
    %728 = arith.ori %666, %true : i1 loc(#loc83)
    %729 = arith.andi %653, %728 : i1 loc(#loc83)
    %730 = arith.xori %665, %true_0 : i1 loc(#loc84)
    %731 = arith.xori %727, %true_0 : i1 loc(#loc84)
    %732 = arith.ori %730, %731 : i1 loc(#loc84)
    %733 = arith.andi %729, %732 : i1 loc(#loc84)
    %734 = acir.fire "top.memory_stage.access#3" %733 {successors = [2], txn = 3 : i64} loc(#loc84)
    %c0_i32_39 = arith.constant 0 : i32 loc(#loc84)
    %c1_i32_40 = arith.constant 1 : i32 loc(#loc84)
    %c0_i32_41 = arith.constant 0 : i32 loc(#loc84)
    %735 = arith.select %734, %c1_i32_40, %c0_i32_41 : i32 loc(#loc84)
    %736 = arith.addi %c0_i32_39, %735 : i32 loc(#loc84)
    %737 = arith.subi %546, %736 : i32 loc(#loc60)
    %c1_i32_42 = arith.constant 1 : i32 loc(#loc84)
    %738 = arith.cmpi uge, %737, %c1_i32_42 : i32 loc(#loc84)
    %739 = arith.ori %545, %true : i1 loc(#loc84)
    %740 = arith.andi %524, %739 : i1 loc(#loc84)
    %741 = arith.xori %544, %true_0 : i1 loc(#loc60)
    %742 = arith.xori %738, %true_0 : i1 loc(#loc60)
    %743 = arith.ori %741, %742 : i1 loc(#loc60)
    %744 = arith.andi %740, %743 : i1 loc(#loc60)
    %745 = acir.fire "top.execute.execute#2" %744 {successors = [1], txn = 2 : i64} loc(#loc60)
    %c0_i32_43 = arith.constant 0 : i32 loc(#loc60)
    %c1_i32_44 = arith.constant 1 : i32 loc(#loc60)
    %c0_i32_45 = arith.constant 0 : i32 loc(#loc60)
    %746 = arith.select %745, %c1_i32_44, %c0_i32_45 : i32 loc(#loc60)
    %747 = arith.addi %c0_i32_43, %746 : i32 loc(#loc60)
    %748 = arith.subi %515, %747 : i32 loc(#loc53)
    %c1_i32_46 = arith.constant 1 : i32 loc(#loc60)
    %749 = arith.cmpi uge, %748, %c1_i32_46 : i32 loc(#loc60)
    %750 = arith.andi %344, %true : i1 loc(#loc60)
    %751 = arith.ori %514, %750 : i1 loc(#loc60)
    %752 = arith.andi %513, %751 : i1 loc(#loc60)
    %753 = arith.xori %510, %true_0 : i1 loc(#loc53)
    %754 = arith.xori %749, %true_0 : i1 loc(#loc53)
    %755 = arith.andi %344, %754 : i1 loc(#loc53)
    %756 = arith.ori %753, %755 : i1 loc(#loc53)
    %757 = arith.andi %752, %756 : i1 loc(#loc53)
    %758 = acir.fire "top.decode_stage.read_operands#1" %757 {successors = [0], txn = 1 : i64} loc(#loc53)
    %759 = arith.andi %758, %107 : i1 loc(#loc53)
    %c0_i32_47 = arith.constant 0 : i32 loc(#loc53)
    %c1_i32_48 = arith.constant 1 : i32 loc(#loc53)
    %c0_i32_49 = arith.constant 0 : i32 loc(#loc53)
    %760 = arith.select %759, %c1_i32_48, %c0_i32_49 : i32 loc(#loc53)
    %761 = arith.addi %c0_i32_47, %760 : i32 loc(#loc53)
    %762 = arith.subi %28, %761 : i32 loc(#loc6)
    %c1_i32_50 = arith.constant 1 : i32 loc(#loc53)
    %763 = arith.cmpi uge, %762, %c1_i32_50 : i32 loc(#loc53)
    %764 = arith.andi %3, %true : i1 loc(#loc53)
    %765 = arith.ori %27, %764 : i1 loc(#loc53)
    %766 = arith.andi %26, %765 : i1 loc(#loc53)
    %767 = arith.xori %23, %true_0 : i1 loc(#loc6)
    %768 = arith.xori %763, %true_0 : i1 loc(#loc6)
    %769 = arith.andi %3, %768 : i1 loc(#loc6)
    %770 = arith.ori %767, %769 : i1 loc(#loc6)
    %771 = arith.andi %766, %770 : i1 loc(#loc6)
    %772 = acir.fire "top.fetch.fetch#0" %771 {successors = [], txn = 0 : i64} loc(#loc6)
    %c0_i32_51 = arith.constant 0 : i32 loc(#loc6)
    %c1_i32_52 = arith.constant 1 : i32 loc(#loc6)
    %c0_i32_53 = arith.constant 0 : i32 loc(#loc6)
    %773 = arith.select %734, %c1_i32_52, %c0_i32_53 : i32 loc(#loc6)
    %774 = arith.addi %c0_i32_51, %773 : i32 loc(#loc6)
    %775 = arith.subi %546, %774 : i32 loc(#loc6)
    %776 = acir.aggregate() : () -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc6)
    %c0_i32_54 = arith.constant 0 : i32 loc(#loc6)
    %777 = acir.extract %547[%c0_i32_54] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc6)
    %c0_i32_55 = arith.constant 0 : i32 loc(#loc6)
    %778 = arith.cmpi eq, %774, %c0_i32_55 : i32 loc(#loc6)
    %779 = arith.select %778, %777, %776 : !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc6)
    %780 = acir.aggregate(%779) : (!acir.struct<"src_isa_f0424bc8.Executed">) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1> loc(#loc6)
    %781 = arith.andi %745, %544 : i1 loc(#loc60)
    %782 = acir.extract %517 {acir.id = "v18", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc85)
    %783 = acir.extract %517 {acir.id = "v19", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc86)
    %784 = acir.extract %517 {acir.id = "v20", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc87)
    %785 = acir.extract %517 {acir.id = "v21", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc88)
    %786 = acir.extract %517 {acir.id = "v12", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc58)
    %787 = acir.extract %517 {acir.id = "v13", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc89)
    %788 = arith.addi %786, %787 {acir.id = "v14", acir.source_type = "u32"} : i32 loc(#loc89)
    %789 = acir.extract %517 {acir.id = "v9", acir.source_type = "u32", field = "lhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc59)
    %790 = acir.extract %517 {acir.id = "v10", acir.source_type = "u32", field = "rhs"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc90)
    %791 = arith.subi %789, %790 {acir.id = "v11", acir.source_type = "u32"} : i32 loc(#loc90)
    %792 = arith.select %530, %788, %791 : i32 loc(#loc90)
    %793 = acir.extract %517 {acir.id = "v22", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i32 loc(#loc91)
    %794 = acir.extract %517 {acir.id = "v23", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Operands">) -> i1 loc(#loc92)
    %795 = acir.aggregate(%782, %783, %784, %785, %792, %793, %794) {acir.id = "v24", acir.source_type = "src_isa_f0424bc8.Executed"} : (i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Executed"> loc(#loc92)
    %c0_i32_56 = arith.constant 0 : i32 loc(#loc92)
    %796 = arith.select %781, %775, %c0_i32_56 : i32 loc(#loc92)
    %797 = acir.update %780, %795[%796] {path = [unit]} : (!acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1>, !acir.struct<"src_isa_f0424bc8.Executed">, i32) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1> loc(#loc92)
    %798 = arith.select %781, %797, %780 : !acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1> loc(#loc92)
    %c1_i32_57 = arith.constant 1 : i32 loc(#loc92)
    %c0_i32_58 = arith.constant 0 : i32 loc(#loc92)
    %799 = arith.select %781, %c1_i32_57, %c0_i32_58 : i32 loc(#loc92)
    %800 = arith.addi %775, %799 : i32 loc(#loc92)
    acir.next %798 {aspect = "storage", resource = "top.ex_mem"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Executed">, 1> loc(#loc92)
    acir.next %800 {aspect = "count", resource = "top.ex_mem"} : i32 loc(#loc92)
    %801 = arith.andi %772, %1 : i1 loc(#loc6)
    %c4_i32 = arith.constant {acir.source_type = "u32"} 4 : i32 loc(#loc93)
    %802 = arith.addi %0, %c4_i32 {acir.id = "v10", acir.source_type = "u32"} : i32 loc(#loc93)
    %803 = arith.select %801, %802, %0 : i32 loc(#loc93)
    acir.next %803 {aspect = "value", resource = "top.fetch.pc"} : i32 loc(#loc93)
    acir.next %9 {aspect = "value", resource = "top.fetch.rom"} : !acir.array<i32, 6> loc(#loc93)
    %c0_i32_59 = arith.constant 0 : i32 loc(#loc93)
    %c1_i32_60 = arith.constant 1 : i32 loc(#loc93)
    %c0_i32_61 = arith.constant 0 : i32 loc(#loc93)
    %804 = arith.select %745, %c1_i32_60, %c0_i32_61 : i32 loc(#loc93)
    %805 = arith.addi %c0_i32_59, %804 : i32 loc(#loc93)
    %806 = arith.subi %515, %805 : i32 loc(#loc93)
    %807 = acir.aggregate() : () -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc93)
    %c0_i32_62 = arith.constant 0 : i32 loc(#loc93)
    %808 = acir.extract %516[%c0_i32_62] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc93)
    %c0_i32_63 = arith.constant 0 : i32 loc(#loc93)
    %809 = arith.cmpi eq, %805, %c0_i32_63 : i32 loc(#loc93)
    %810 = arith.select %809, %808, %807 : !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc93)
    %811 = acir.aggregate(%810) : (!acir.struct<"src_isa_f0424bc8.Operands">) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1> loc(#loc93)
    %812 = arith.andi %758, %510 : i1 loc(#loc53)
    %813 = acir.extract %85 {acir.id = "v17", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc94)
    %814 = acir.extract %85 {acir.id = "v18", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc95)
    %815 = acir.extract %85 {acir.id = "v19", acir.source_type = "u32", field = "kind"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc96)
    %816 = acir.extract %85 {acir.id = "v20", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc97)
    %817 = acir.extract %387[%379] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc43)
    %818 = arith.select %382, %817, %c0_i32_14 : i32 loc(#loc43)
    %819 = acir.extract %387[%399] {acir.id = "v13", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 32>, i32) -> i32 loc(#loc43)
    %820 = arith.select %402, %819, %c0_i32_15 : i32 loc(#loc43)
    %821 = acir.extract %85 {acir.id = "v12", acir.source_type = "u32", field = "immediate"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc98)
    %822 = arith.select %481, %820, %821 : i32 loc(#loc98)
    %823 = acir.extract %85 {acir.id = "v21", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i1 loc(#loc99)
    %824 = acir.aggregate(%813, %814, %815, %816, %818, %822, %820, %823) {acir.id = "v22", acir.source_type = "src_isa_f0424bc8.Operands"} : (i32, i32, i32, i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Operands"> loc(#loc99)
    %c0_i32_64 = arith.constant 0 : i32 loc(#loc99)
    %825 = arith.select %812, %806, %c0_i32_64 : i32 loc(#loc99)
    %826 = acir.update %811, %824[%825] {path = [unit]} : (!acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1>, !acir.struct<"src_isa_f0424bc8.Operands">, i32) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1> loc(#loc99)
    %827 = arith.select %812, %826, %811 : !acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1> loc(#loc99)
    %c1_i32_65 = arith.constant 1 : i32 loc(#loc99)
    %c0_i32_66 = arith.constant 0 : i32 loc(#loc99)
    %828 = arith.select %812, %c1_i32_65, %c0_i32_66 : i32 loc(#loc99)
    %829 = arith.addi %806, %828 : i32 loc(#loc99)
    acir.next %827 {aspect = "storage", resource = "top.id_ex"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Operands">, 1> loc(#loc99)
    acir.next %829 {aspect = "count", resource = "top.id_ex"} : i32 loc(#loc99)
    %c0_i32_67 = arith.constant 0 : i32 loc(#loc99)
    %c1_i32_68 = arith.constant 1 : i32 loc(#loc99)
    %c0_i32_69 = arith.constant 0 : i32 loc(#loc99)
    %830 = arith.select %759, %c1_i32_68, %c0_i32_69 : i32 loc(#loc99)
    %831 = arith.addi %c0_i32_67, %830 : i32 loc(#loc99)
    %832 = arith.subi %28, %831 : i32 loc(#loc99)
    %833 = acir.aggregate() : () -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc99)
    %c0_i32_70 = arith.constant 0 : i32 loc(#loc99)
    %834 = acir.extract %33[%c0_i32_70] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc99)
    %c0_i32_71 = arith.constant 0 : i32 loc(#loc99)
    %835 = arith.cmpi eq, %831, %c0_i32_71 : i32 loc(#loc99)
    %836 = arith.select %835, %834, %833 : !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc99)
    %837 = acir.aggregate(%836) : (!acir.struct<"src_isa_f0424bc8.Fetched">) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1> loc(#loc99)
    %838 = arith.andi %772, %23 : i1 loc(#loc6)
    %c2_i32_72 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc100)
    %839 = arith.shrui %0, %c2_i32_72 : i32 loc(#loc100)
    %840 = acir.extract %9[%839] {acir.id = "v8", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 6>, i32) -> i32 loc(#loc100)
    %841 = acir.aggregate(%0, %840) {acir.id = "v13", acir.source_type = "src_isa_f0424bc8.Fetched"} : (i32, i32) -> !acir.struct<"src_isa_f0424bc8.Fetched"> loc(#loc100)
    %c0_i32_73 = arith.constant 0 : i32 loc(#loc100)
    %842 = arith.select %838, %832, %c0_i32_73 : i32 loc(#loc100)
    %843 = acir.update %837, %841[%842] {path = [unit]} : (!acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1>, !acir.struct<"src_isa_f0424bc8.Fetched">, i32) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1> loc(#loc100)
    %844 = arith.select %838, %843, %837 : !acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1> loc(#loc100)
    %c1_i32_74 = arith.constant 1 : i32 loc(#loc100)
    %c0_i32_75 = arith.constant 0 : i32 loc(#loc100)
    %845 = arith.select %838, %c1_i32_74, %c0_i32_75 : i32 loc(#loc100)
    %846 = arith.addi %832, %845 : i32 loc(#loc100)
    acir.next %844 {aspect = "storage", resource = "top.if_id"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Fetched">, 1> loc(#loc100)
    acir.next %846 {aspect = "count", resource = "top.if_id"} : i32 loc(#loc100)
    %c0_i32_76 = arith.constant 0 : i32 loc(#loc100)
    %c1_i32_77 = arith.constant 1 : i32 loc(#loc100)
    %c0_i32_78 = arith.constant 0 : i32 loc(#loc100)
    %847 = arith.select %723, %c1_i32_77, %c0_i32_78 : i32 loc(#loc100)
    %848 = arith.addi %c0_i32_76, %847 : i32 loc(#loc100)
    %849 = arith.subi %29, %848 : i32 loc(#loc100)
    %850 = acir.aggregate() : () -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc100)
    %c0_i32_79 = arith.constant 0 : i32 loc(#loc100)
    %851 = acir.extract %667[%c0_i32_79] : (!acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1>, i32) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc100)
    %c0_i32_80 = arith.constant 0 : i32 loc(#loc100)
    %852 = arith.cmpi eq, %848, %c0_i32_80 : i32 loc(#loc100)
    %853 = arith.select %852, %851, %850 : !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc100)
    %854 = acir.aggregate(%853) : (!acir.struct<"src_isa_f0424bc8.Retired">) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1> loc(#loc100)
    %855 = arith.andi %734, %665 : i1 loc(#loc84)
    %856 = acir.extract %548 {acir.id = "v29", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc101)
    %857 = acir.extract %548 {acir.id = "v30", acir.source_type = "u32", field = "word"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc102)
    %858 = acir.extract %548 {acir.id = "v31", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc103)
    %859 = acir.extract %548 {acir.id = "v5", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc104)
    %c2_i32_81 = arith.constant {acir.source_type = "u32"} 2 : i32 loc(#loc105)
    %860 = arith.shrui %631, %c2_i32_81 : i32 loc(#loc105)
    %861 = acir.extract %602[%860] {acir.id = "v20", acir.source_type = "u32", indexed = true} : (!acir.array<i32, 16>, i32) -> i32 loc(#loc105)
    %862 = arith.select %558, %859, %861 : i32 loc(#loc105)
    %863 = acir.extract %548 {acir.id = "v32", acir.source_type = "bool", field = "writes"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i1 loc(#loc106)
    %864 = acir.aggregate(%856, %857, %858, %862, %863) {acir.id = "v33", acir.source_type = "src_isa_f0424bc8.Retired"} : (i32, i32, i32, i32, i1) -> !acir.struct<"src_isa_f0424bc8.Retired"> loc(#loc106)
    %c0_i32_82 = arith.constant 0 : i32 loc(#loc106)
    %865 = arith.select %855, %849, %c0_i32_82 : i32 loc(#loc106)
    %866 = acir.update %854, %864[%865] {path = [unit]} : (!acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1>, !acir.struct<"src_isa_f0424bc8.Retired">, i32) -> !acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1> loc(#loc106)
    %867 = arith.select %855, %866, %854 : !acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1> loc(#loc106)
    %c1_i32_83 = arith.constant 1 : i32 loc(#loc106)
    %c0_i32_84 = arith.constant 0 : i32 loc(#loc106)
    %868 = arith.select %855, %c1_i32_83, %c0_i32_84 : i32 loc(#loc106)
    %869 = arith.addi %849, %868 : i32 loc(#loc106)
    acir.next %867 {aspect = "storage", resource = "top.mem_wb"} : !acir.array<!acir.struct<"src_isa_f0424bc8.Retired">, 1> loc(#loc106)
    acir.next %869 {aspect = "count", resource = "top.mem_wb"} : i32 loc(#loc106)
    %870 = arith.andi %734, %567 : i1 loc(#loc84)
    %871 = acir.extract %548 {acir.id = "v17", acir.source_type = "u32", field = "store_value"} : (!acir.struct<"src_isa_f0424bc8.Executed">) -> i32 loc(#loc107)
    %c2_i32_85 = arith.constant {acir.source_type = "u32"} 2 : i32
    %872 = arith.shrui %582, %c2_i32_85 : i32
    %873 = acir.update %602, %871[%872] {acir.id = "v28", acir.source_type = "array<u32,16>", path = [unit]} : (!acir.array<i32, 16>, i32, i32) -> !acir.array<i32, 16>
    %874 = arith.select %870, %873, %602 : !acir.array<i32, 16>
    acir.next %874 {aspect = "value", resource = "top.memory.words"} : !acir.array<i32, 16>
    %875 = arith.andi %723, %689 : i1
    %876 = acir.extract %668 {acir.id = "v36", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc108)
    %877 = acir.update %32, %false_33[%876] {acir.id = "v37", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc108)
    %878 = arith.select %875, %877, %32 : !acir.array<i1, 32> loc(#loc108)
    %879 = arith.andi %758, %447 : i1 loc(#loc53)
    %true_86 = arith.constant {acir.source_type = "bool"} true
    %880 = acir.extract %85 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Decoded">) -> i32 loc(#loc109)
    %881 = acir.update %32, %true_86[%880] {acir.id = "v24", acir.source_type = "array<bool,32>", path = [unit]} : (!acir.array<i1, 32>, i1, i32) -> !acir.array<i1, 32> loc(#loc109)
    %882 = arith.select %879, %881, %878 : !acir.array<i1, 32> loc(#loc109)
    acir.next %882 {aspect = "value", resource = "top.rf.pending"} : !acir.array<i1, 32> loc(#loc109)
    %883 = arith.andi %723, %689 : i1
    %884 = acir.extract %668 {acir.id = "v15", acir.source_type = "u32", field = "value"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc110)
    %885 = acir.extract %668 {acir.id = "v23", acir.source_type = "u32", field = "rd"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc111)
    %886 = acir.update %387, %884[%885] {acir.id = "v24", acir.source_type = "array<u32,32>", path = [unit]} : (!acir.array<i32, 32>, i32, i32) -> !acir.array<i32, 32> loc(#loc111)
    %887 = arith.select %883, %886, %387 : !acir.array<i32, 32> loc(#loc111)
    acir.next %887 {aspect = "value", resource = "top.rf.registers"} : !acir.array<i32, 32> loc(#loc111)
    %888 = acir.current {aspect = "value", resource = "top.writeback.last_pc"} : i32 loc(#loc111)
    %889 = acir.extract %668 {acir.id = "v12", acir.source_type = "u32", field = "pc"} : (!acir.struct<"src_isa_f0424bc8.Retired">) -> i32 loc(#loc112)
    %890 = arith.select %723, %889, %888 : i32 loc(#loc112)
    acir.next %890 {aspect = "value", resource = "top.writeback.last_pc"} : i32 loc(#loc112)
    %c1_i32_87 = arith.constant {acir.source_type = "u32"} 1 : i32 loc(#loc113)
    %891 = arith.addi %719, %c1_i32_87 {acir.id = "v9", acir.source_type = "u32"} : i32 loc(#loc113)
    %892 = arith.select %723, %891, %719 : i32 loc(#loc113)
    acir.next %892 {aspect = "value", resource = "top.writeback.retired"} : i32 loc(#loc113)
    %893 = arith.andi %734, %567 : i1 loc(#loc84)
    %false_88 = arith.constant {acir.source_type = "bool"} false
    %c64_i32 = arith.constant {acir.source_type = "u32"} 64 : i32
    %894 = arith.cmpi ult, %582, %c64_i32 {acir.id = "v11", acir.source_type = "bool"} : i32
    %895 = arith.select %586, %false_88, %894 : i1
    acir.effect %893(%895) {branch = 14 : i64, frames = ["top.memory_stage:access", "top.memory:store"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc66)
    %896 = arith.andi %734, %557 : i1 loc(#loc84)
    %false_89 = arith.constant {acir.source_type = "bool"} false
    %c64_i32_90 = arith.constant {acir.source_type = "u32"} 64 : i32
    %897 = arith.cmpi ult, %631, %c64_i32_90 {acir.id = "v10", acir.source_type = "bool"} : i32
    %898 = arith.select %635, %false_89, %897 : i1
    acir.effect %896(%898) {branch = 15 : i64, frames = ["top.memory_stage:access", "top.memory:load"], method = "check", port = 0 : i64, resource = "", txn = 3 : i64} : (i1, i1) -> () loc(#loc72)
  }
}
#loc1 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":67:22)
#loc2 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":68:25)
#loc3 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:16)
#loc4 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":69:20)
#loc5 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:9)
#loc6 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":64:9)
#loc7 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:19)
#loc8 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:20)
#loc9 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":87:23)
#loc10 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":53:12)
#loc11 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":54:21)
#loc12 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":64:18)
#loc13 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":58:19)
#loc14 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:28)
#loc15 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:46)
#loc16 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:28)
#loc17 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:28)
#loc18 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":69:10)
#loc19 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:55)
#loc20 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:75)
#loc21 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:102)
#loc22 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":88:42)
#loc23 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":86:12)
#loc24 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":83:21)
#loc25 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":77:21)
#loc26 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":67:21)
#loc27 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":80:9)
#loc28 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":55:22)
#loc29 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:9)
#loc30 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":74:16)
#loc31 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:9)
#loc32 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:16)
#loc33 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:33)
#loc34 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":70:48)
#loc35 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:9)
#loc36 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/isa.py":65:16)
#loc37 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:47)
#loc38 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":18:94)
#loc39 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":79:20)
#loc40 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":90:20)
#loc41 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":80:15)
#loc42 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":80:23)
#loc43 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:16)
#loc44 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:23)
#loc45 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":81:31)
#loc46 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":83:9)
#loc47 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:12)
#loc48 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":28:28)
#loc49 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:13)
#loc50 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:16)
#loc51 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":24:9)
#loc52 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:36)
#loc53 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:9)
#loc54 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":100:16)
#loc55 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:16)
#loc56 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:41)
#loc57 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:54)
#loc58 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:63)
#loc59 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:18)
#loc60 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:9)
#loc61 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":115:16)
#loc62 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":117:12)
#loc63 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":117:25)
#loc64 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":119:14)
#loc65 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:13)
#loc66 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:9)
#loc67 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:26)
#loc68 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:26)
#loc69 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":50:31)
#loc70 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":51:9)
#loc71 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":118:22)
#loc72 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:9)
#loc73 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":118:34)
#loc74 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:26)
#loc75 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":45:31)
#loc76 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:16)
#loc77 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:16)
#loc78 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":135:16)
#loc79 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:12)
#loc80 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":33:28)
#loc81 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:13)
#loc82 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:13)
#loc83 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:19)
#loc84 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:9)
#loc85 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:25)
#loc86 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:34)
#loc87 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:45)
#loc88 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:56)
#loc89 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:74)
#loc90 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":101:29)
#loc91 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:73)
#loc92 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":102:91)
#loc93 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":63:27)
#loc94 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:25)
#loc95 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:37)
#loc96 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:51)
#loc97 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:65)
#loc98 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":82:15)
#loc99 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":84:100)
#loc100 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":62:34)
#loc101 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:24)
#loc102 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:33)
#loc103 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:44)
#loc104 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":116:18)
#loc105 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":46:33)
#loc106 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":121:61)
#loc107 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":120:38)
#loc108 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":35:21)
#loc109 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":29:21)
#loc110 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:34)
#loc111 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":34:23)
#loc112 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":138:19)
#loc113 = loc("/home/lc/tmp/pycircuit/examples/riscv_pipeline/model.py":137:29)
